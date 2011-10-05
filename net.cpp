#include <ctype.h>
#include <stdarg.h>
#include <curl/curl.h>
#include <FL/Fl.H>

#include "common.h"
#include "net.h"

net_t::net_t(void){
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

void net_t::connect(void){
    char buf[128];
    sprintf(buf, "%s?offset=%lu", get_url, offset);
    new_conn(buf, 1);
}

void net_t::add_packet_handler(void *obj, packet_handler_t handler){
    packet_handler_objs[num_packet_handlers] = obj;
    packet_handler_funcs[num_packet_handlers] = handler;
    num_packet_handlers++;
}

void net_t::send(void *obj, const char *fmt, ...){
    net_t *net = (net_t *)obj;
    va_list args;
    char *url;
    char *msg = NULL;
    int n;
    const char *prefix = "http://qartis.com/cgi-bin/c5_put.cgi?";

    va_start(args, fmt);
    n = 1 + vsnprintf(msg, 0, fmt, args);
    va_end(args);

    msg = (char *)malloc(n);

    va_start(args, fmt);
    vsnprintf(msg, n, fmt, args);
    va_end(args);

    url = (char *)malloc(3 * n + strlen(prefix) + 1);
    strcpy(url, prefix);
    char *endp = url + strlen(prefix);
    char *msgp = msg;

    while(*msgp){
        if (!isalnum(*msgp)){
            sprintf(endp, "%%%2X", *msgp);
            endp += 3;
        } else {
            *endp++ = *msgp;
        }
        msgp++;
    }
    *endp = '\0';
    net->new_conn(url, 0);
    free(url);
    free(msg);
}

size_t net_t::ignore_data(void *buf, size_t size, size_t nmemb, void *data){
    (void)buf;
    (void)data;
    /* TODO handle this data reply */
    return size * nmemb;
}

size_t net_t::parse_packet_cb(void *buf, size_t size, size_t nmemb, void *data){
    struct ConnInfo *conn = (struct ConnInfo *)data;
    net_t *net = conn->global;
    unsigned tmp_offset = 0;
    char *packetbuf = (char *)malloc(size * nmemb + strlen(net->netbuf) + 1);
    strcpy(packetbuf, net->netbuf);
    strncat(packetbuf, (const char *)buf, size * nmemb);
    char *start = packetbuf;
    for(;;){
        if (sscanf(start, "%u", &tmp_offset) != 1){
            break;
        }
        char *nl = strchr(start, '\n');
        if (!nl){
            break;
        }
        *nl = '\0';
        if (tmp_offset > 0){
            char *space = strchr(start, ' ');
            if (!space){
                break;
            }
            start = space + 1;
        }
        net->dispatch_packet(start);
        start = nl + 1;
        net->offset = tmp_offset;
    }
    strcpy(net->netbuf, start);
    free(packetbuf);

    return size * nmemb;
}

void net_t::dispatch_packet(const char *buf){
    int i;
    for(i=0;i<num_packet_handlers;i++){
        if (packet_handler_funcs[i](packet_handler_objs[i], buf)) return;
    }
    printf("error: got an unhandled packet '%s'\n", buf);
}

void net_t::mcode_or_die(const char *where, CURLMcode code) {
    if (code == CURLM_OK){
        return;
    }
    const char *s;
    switch (code) {
    case     CURLM_CALL_MULTI_PERFORM: s="CURLM_CALL_MULTI_PERFORM"; break;
    case     CURLM_BAD_HANDLE:         s="CURLM_BAD_HANDLE";         break;
    case     CURLM_BAD_EASY_HANDLE:    s="CURLM_BAD_EASY_HANDLE";    break;
    case     CURLM_OUT_OF_MEMORY:      s="CURLM_OUT_OF_MEMORY";      break;
    case     CURLM_INTERNAL_ERROR:     s="CURLM_INTERNAL_ERROR";     break;
    case     CURLM_BAD_SOCKET:         s="CURLM_BAD_SOCKET";         break;
    case     CURLM_UNKNOWN_OPTION:     s="CURLM_UNKNOWN_OPTION";     break;
    case     CURLM_LAST:               s="CURLM_LAST";               break;
    case     CURLM_OK:                 s="CURLM_OK";                 break;
    default: s="CURLM_unknown";
    }
    printf("ERROR: %s returns %s\n", where, s);
    exit(code);
}

void net_t::timer_cb(void *data){
    net_t *net = (net_t *)data;
    CURLMcode rc = curl_multi_socket_action(net->multi,
                                            CURL_SOCKET_TIMEOUT,
                                            0,
                                            &net->still_running);
    net->mcode_or_die("timer_cb: curl_multi_socket_action", rc);
    net->check_multi_info();
}

void net_t::perform_reconnects_cb(void *data){
    net_t *net = (net_t *)data;
    struct ConnInfo *conn;
    struct ConnInfo *next;

    for (conn = net->reconnect_list; conn; conn = next){
        char *url = conn->url;
        if (conn->is_poll){
            char buf[128];
            sprintf(buf, "%s?offset=%lu", get_url, net->offset);
            url = buf;
        }
        net->new_conn(url, conn->is_poll);
        next = conn->next;
        free(conn->url);
        free(conn);
    }
    net->reconnect_list = NULL;
}

void net_t::cleanup_completed_transfer(CURLMsg *msg){
    struct ConnInfo *conn;
    CURL *easy = msg->easy_handle;
    CURLcode res = msg->data.result;
    curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
    curl_multi_remove_handle(multi, easy);
    curl_easy_cleanup(easy);

    if (res != CURLE_OK){
        printf("died: %s%s\n", curl_easy_strerror(res), conn->is_poll ? " (poll)" : " (upload)");
        conn->next = reconnect_list;
        reconnect_list = conn;
        Fl::remove_timeout(perform_reconnects_cb);
        Fl::add_timeout(1.0, perform_reconnects_cb, (void*)this);
        return;
    }

    if (conn->is_poll){
        printf("connection died!\n");
        Fl::remove_timeout(timer_cb);
        char buf[128];
        sprintf(buf, "%s?offset=%lu", get_url, offset);
        new_conn(buf, 1);
        timer_cb((void*)this);
    }
    free(conn->url);
    free(conn);
}

void net_t::check_multi_info(){
    CURLMsg *msg;
    int msgs_left;

    while ((msg = curl_multi_info_read(multi, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            cleanup_completed_transfer(msg);
        }
    }
}

int net_t::update_timeout_cb(CURLM *multi, long timeout_ms, void *userp){
    (void)multi;
    double t = (double)timeout_ms / 1000.0;
    net_t *net=(net_t *)userp;

    if (timeout_ms > -1){
        Fl::add_timeout(t, timer_cb, (void *)net);
    }
    return 0;
}

void net_t::event_cb(int fd, void *data){
    net_t *net = (net_t*) data;
    CURLMcode rc;

    rc = curl_multi_socket_action(net->multi, fd, 0, &net->still_running);
    net->mcode_or_die("event_cb: curl_multi_socket_action", rc);

    net->check_multi_info();
    if(!net->still_running) {
        Fl::remove_timeout(timer_cb);
    }
}

void net_t::remsock(struct SockInfo *f){
    if (!f){
        return;
    }
    Fl::remove_fd(f->fd);
    free(f);
}

void net_t::setsock(struct SockInfo *f, curl_socket_t s, CURL*e, int act){
    int when = (act&CURL_POLL_IN?FL_READ:0)|(act&CURL_POLL_OUT?FL_WRITE:0);

    f->sockfd = s;
    f->action = act;
    f->easy = e;
    Fl::remove_fd(f->fd);
    Fl::add_fd(f->fd, when, event_cb, (void *)this);
}

void net_t::addsock(curl_socket_t s, CURL *easy, int action) {
    struct SockInfo *fdp = (struct SockInfo *)malloc(sizeof(struct SockInfo));
    memset(fdp, sizeof(struct SockInfo), '\0');

    fdp->global = this;
    fdp->fd = s;
    setsock(fdp, s, easy, action);
    curl_multi_assign(multi, s, fdp);
}

int net_t::sock_cb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp){
    net_t *net = (net_t*) cbp;
    struct SockInfo *fdp = (struct SockInfo*) sockp;

    if (what == CURL_POLL_REMOVE) {
        net->remsock(fdp);
    } else if (!fdp) {
        net->addsock(s, e, what);
    } else {
        net->setsock(fdp, s, e, what);
    }
    return 0;
}

void net_t::new_conn(const char *url, bool is_poll){
    CURLMcode rc;

    struct ConnInfo *conn = (struct ConnInfo *)malloc(sizeof(struct ConnInfo));
    memset(conn, sizeof(struct ConnInfo), '\0');

    conn->is_poll = is_poll;

    conn->easy = curl_easy_init();
    if (!conn->easy) {
        printf("curl_easy_init() failed, exiting!\n");
        exit(2);
    }
    conn->global = this;
    conn->url = strdup(url);
    curl_easy_setopt(conn->easy, CURLOPT_URL, conn->url);
    if (conn->is_poll){
        curl_easy_setopt(conn->easy, CURLOPT_WRITEFUNCTION, parse_packet_cb);
    } else {
        curl_easy_setopt(conn->easy, CURLOPT_WRITEFUNCTION, ignore_data);
    }
    curl_easy_setopt(conn->easy, CURLOPT_WRITEDATA, conn);
    curl_easy_setopt(conn->easy, CURLOPT_PRIVATE, conn);
    curl_easy_setopt(conn->easy, CURLOPT_NOPROGRESS, 1L);
    //curl_easy_setopt(conn->easy, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(conn->easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(conn->easy, CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_TIME, 30L);

    rc = curl_multi_add_handle(multi, conn->easy);
    mcode_or_die("new_conn: curl_multi_add_handle", rc);
}
