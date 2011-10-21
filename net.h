typedef bool (*packet_handler_t)(void *obj, const char *buf);

#define get_url "http://qartis.com/cgi-bin/c5_get.cgi"
#define NUM_HANDLERS 32

class net_t;

struct ConnInfo {
    CURL *easy;
    char *url;
    net_t *global;
    bool is_poll;
    struct ConnInfo *next;
};

struct SockInfo {
    curl_socket_t sockfd;
    CURL *easy;
    int action;
    long timeout;
    int fd;
    net_t *global;
};

class net_t {
private:
    packet_handler_t packet_handler_funcs[NUM_HANDLERS];
    void *packet_handler_objs[NUM_HANDLERS];
    char netbuf[128];
    int num_packet_handlers;
    CURLM *multi;
    int still_running;
    struct ConnInfo *reconnect_list;
    long unsigned offset;

    void remsock(struct SockInfo *f);
    void setsock(struct SockInfo *f, curl_socket_t s, CURL *e, int act);
    void addsock(curl_socket_t s, CURL *e, int act);
    void check_multi_info(void);
    void new_conn(const char *url, bool is_poll);
    void mcode_or_die(const char *where, CURLMcode code);
    void cleanup_completed_transfer(CURLMsg *msg);
    void dispatch_packet(const char *);


public:
    net_t(void);
    void connect(void);
    void add_packet_handler(void *obj, packet_handler_t);
    static void send(void *obj, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
    static size_t parse_packet_cb(void *buffer, size_t size, size_t nmemb, void *data);
    static size_t ignore_data(void *buf, size_t size, size_t nmemb, void *data);
    static void timer_cb(void *data);
    static void perform_reconnects_cb(void *data);
    static int update_timeout_cb(CURLM *multi, long timeout_ms, void *data);
    static void event_cb(int fd, void *data);
    static int sock_cb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp);
};
