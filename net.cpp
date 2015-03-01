#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <curl/curl.h>
#include <FL/Fl.H>

#include "common.h"
#include "net.h"

#define ARRAY_SIZE 256

net_t::net_t(void)
{
    num_packet_handlers = 0;
    offset = 0;
    reconnect_list = NULL;
    netbuf[0] = '\0';

    curl_global_init(CURL_GLOBAL_ALL);
    multi = curl_multi_init();

    curl_multi_setopt(multi, CURLMOPT_SOCKETFUNCTION, sock_cb);
    curl_multi_setopt(multi, CURLMOPT_TIMERFUNCTION, update_timeout_cb);
    curl_multi_setopt(multi, CURLMOPT_SOCKETDATA, this);
    curl_multi_setopt(multi, CURLMOPT_TIMERDATA, this);
}

void net_t::connect(void)
{
    char buf[ARRAY_SIZE];

    snprintf(buf, sizeof(buf), "%s?offset=%lu", HTTP_GET_URL, offset);
    new_conn(buf, 1);
}

void net_t::add_packet_handler(void *obj, packet_handler_t handler)
{
    packet_handler_objs[num_packet_handlers] = obj;
    packet_handler_funcs[num_packet_handlers] = handler;
    num_packet_handlers++;
}

void net_t::send(void *obj, const char *fmt, ...)
{
    net_t *net = (net_t *) obj;
    va_list args;
    char url[ARRAY_SIZE];
    char msg[ARRAY_SIZE];
    const char *prefix = HTTP_POST_URL;

    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    snprintf(url, sizeof(url), "%s", prefix);
    char *endp = url + strlen(prefix);
    char *msgp = msg;

    while (*msgp) {
        snprintf(url + strlen(url), sizeof(url) - strlen(url),
                isalnum(*msgp) ? "%c" : "%%%2x", *msgp);
        *msgp++;
    }
    net->new_conn(url, 0);
}

size_t net_t::ignore_data(void *buf, size_t size, size_t nmemb, void *data)
{
    (void)buf;
    (void)data;
    /* TODO handle this data reply */
    return size * nmemb;
}

size_t net_t::parse_packet_cb(void *buf, size_t size, size_t nmemb, void *data)
{
    struct ConnInfo *conn = (struct ConnInfo *)data;
    char *packetbuf;
    char *packet_start;
    net_t *net;

    net = conn->global;
    packetbuf = (char *)malloc(size * nmemb + strlen(net->netbuf) + 1);
    strcpy(packetbuf, net->netbuf);
    strncat(packetbuf, (const char *)buf, size * nmemb);
    packet_start = packetbuf;
    for (;;) {
        /* Make sure we have a full line of data */
        char *nl = strchr(packet_start, '\n');
        if (!nl) {
            break;
        }
        *nl = '\0';

        /* If the game board was just cleared, then this string will be "0" */
        net->dispatch_packet(packet_start);

        if (strcmp(packet_start, "cls") == 0) {
            net->offset = 0;
        } else {
            net->offset += strlen(packet_start) + 1;
        }

        packet_start = nl + 1;
    }

    strcpy(net->netbuf, packet_start);
    free(packetbuf);

    return size * nmemb;
}

void net_t::dispatch_packet(const char *buf)
{
    int i;
    for (i = 0; i < num_packet_handlers; i++) {
        if (packet_handler_funcs[i] (packet_handler_objs[i], buf))
            return;
    }
    printf("error: got an unhandled packet '%s'\n", buf);
    exit(1);
}

void net_t::timer_cb(void *data)
{
    net_t *net = (net_t *) data;
    CURLMcode rc = curl_multi_socket_action(net->multi,
                                            CURL_SOCKET_TIMEOUT,
                                            0,
                                            &net->still_running);
    if (rc != CURLM_OK) {
        perror("curl_multi_socket_action");
        exit(1);
    }
    net->check_multi_info();
}

void net_t::perform_reconnects_cb(void *data)
{
    net_t *net = (net_t *) data;
    struct ConnInfo *conn;
    struct ConnInfo *next;

    for (conn = net->reconnect_list; conn; conn = next) {
        char buf[ARRAY_SIZE];
        char *url = conn->url;
        if (conn->is_poll) {
            sprintf(buf, "%s?offset=%lu", HTTP_GET_URL, net->offset);
            url = buf;
        }
        net->new_conn(url, conn->is_poll);
        next = conn->next;
        free(conn->url);
        free(conn);
    }
    net->reconnect_list = NULL;
}

void net_t::cleanup_completed_transfer(CURLMsg * msg)
{
    struct ConnInfo *conn;
    CURL *easy = msg->easy_handle;
    CURLcode res = msg->data.result;
    curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
    curl_multi_remove_handle(multi, easy);
    curl_easy_cleanup(easy);

    if (res != CURLE_OK) {
        printf("died: %s%s\n", curl_easy_strerror(res),
               conn->is_poll ? " (poll)" : " (upload)");
        conn->next = reconnect_list;
        reconnect_list = conn;
        Fl::remove_timeout(perform_reconnects_cb);
        Fl::add_timeout(1.0, perform_reconnects_cb, (void *)this);
        return;
    }

    if (conn->is_poll) {
        printf("connection died!\n");
        Fl::remove_timeout(timer_cb);
        char buf[ARRAY_SIZE];
        sprintf(buf, "%s?offset=%lu", HTTP_GET_URL, offset);
        new_conn(buf, 1);
        timer_cb((void *)this);
    }
    free(conn->url);
    free(conn);
}

void net_t::check_multi_info()
{
    CURLMsg *msg;
    int msgs_left;

    while ((msg = curl_multi_info_read(multi, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            cleanup_completed_transfer(msg);
        }
    }
}

int net_t::update_timeout_cb(CURLM * multi, long timeout_ms, void *userp)
{
    (void)multi;
    double t = (double)timeout_ms / 1000.0;
    net_t *net = (net_t *) userp;

    if (timeout_ms > -1) {
        Fl::add_timeout(t, timer_cb, (void *)net);
    }
    return 0;
}

void net_t::event_cb(int fd, void *data)
{
    net_t *net = (net_t *) data;
    CURLMcode rc;

    rc = curl_multi_socket_action(net->multi, fd, 0, &net->still_running);
    if (rc != CURLM_OK) {
        perror("curl_multi_socket_action");
        exit(1);
    }

    net->check_multi_info();
    if (!net->still_running) {
        Fl::remove_timeout(timer_cb);
    }
}

void net_t::remsock(struct SockInfo *f)
{
    if (!f) {
        return;
    }
    Fl::remove_fd(f->fd);
    free(f);
}

void net_t::setsock(struct SockInfo *f, curl_socket_t s, CURL * e, int act)
{
    int when =
        (act & CURL_POLL_IN ? FL_READ : 0) | (act & CURL_POLL_OUT ? FL_WRITE :
                                              0);

    f->sockfd = s;
    f->action = act;
    f->easy = e;
    Fl::remove_fd(f->fd);
    Fl::add_fd(f->fd, when, event_cb, (void *)this);
}

void net_t::addsock(curl_socket_t s, CURL * easy, int action)
{
    struct SockInfo *fdp = (struct SockInfo *)malloc(sizeof(struct SockInfo));
    memset(fdp, '\0', sizeof(struct SockInfo));

    fdp->global = this;
    fdp->fd = s;
    setsock(fdp, s, easy, action);
    curl_multi_assign(multi, s, fdp);
}

int net_t::sock_cb(CURL * e, curl_socket_t s, int what, void *cbp, void *sockp)
{
    net_t *net = (net_t *) cbp;
    struct SockInfo *fdp = (struct SockInfo *)sockp;

    if (what == CURL_POLL_REMOVE) {
        net->remsock(fdp);
    } else if (!fdp) {
        net->addsock(s, e, what);
    } else {
        net->setsock(fdp, s, e, what);
    }
    return 0;
}

void net_t::new_conn(const char *url, bool is_poll)
{
    CURLMcode rc;

    struct ConnInfo *conn = (struct ConnInfo *)malloc(sizeof(struct ConnInfo));
    memset(conn, '\0', sizeof(struct ConnInfo));

    conn->is_poll = is_poll;

    conn->easy = curl_easy_init();
    if (!conn->easy) {
        printf("curl_easy_init() failed, exiting!\n");
        exit(2);
    }
    conn->global = this;
    conn->url = strdup(url);
    curl_easy_setopt(conn->easy, CURLOPT_URL, conn->url);
    if (conn->is_poll) {
        curl_easy_setopt(conn->easy, CURLOPT_WRITEFUNCTION, parse_packet_cb);
    } else {
        curl_easy_setopt(conn->easy, CURLOPT_WRITEFUNCTION, ignore_data);
        curl_easy_setopt(conn->easy, CURLOPT_POST, 1L);
        curl_easy_setopt(conn->easy, CURLOPT_POSTFIELDS, "");
    }

    curl_easy_setopt(conn->easy, CURLOPT_WRITEDATA, conn);
    curl_easy_setopt(conn->easy, CURLOPT_PRIVATE, conn);
    curl_easy_setopt(conn->easy, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(conn->easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(conn->easy, CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_TIME, 30L);

    rc = curl_multi_add_handle(multi, conn->easy);
    if (rc != CURLM_OK) {
        perror("curl_multi_add_handle");
        exit(1);
        //TODO error
    }
}
