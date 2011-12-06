typedef void (*sendtxt_func_t)(void *obj, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
typedef void (*droppiece_func_t)(void *obj, int x, int y, Fl_Color c, enum drop_type type);
typedef void (*resetgui_func_t)(void *obj);
typedef void (*gameover_func_t)(void *obj, bool won);

struct order_node {
    Fl_Color color;
    struct order_node *next;
};

class game_t {
private:
    int grid_dim;
    enum game_state {
        STATE_INIT,
        STATE_SET_ORDER,
        STATE_PLAYING,
        STATE_GAMEOVER
    } state;
    Fl_Color board[BOARD_DIM][BOARD_DIM];
    bool is_my_turn;
    bool clicked_during_set_order;
    bool used_floater_this_game;
    int stonify(int x, int y);
    void reset(void);
    enum drop_type get_drop_type(int x, int y);
    void print_order();
    void add_to_order(Fl_Color color);
    Fl_Color remove_from_order(Fl_Color color);
    void set_piece(int x, int y, Fl_Color color, enum drop_type type);
    void unmatched_line(Fl_Color color);
    Fl_Color my_color;
    Fl_Color i_follow_color;
    Fl_Color most_recent_color;
    int i_used_floater;
    Fl_Color cur_line_color;
    int cur_line_len;
    Fl_Color order[32];
    int num_in_order;


public:
    game_t(Fl_Color my_color);
    
    static bool parse_clk(void *obj, const char *s);
    static bool parse_cls(void *obj, const char *s);
    static void local_click(void *obj, int x, int y);
    static enum drop_type drop_available(void *obj, int x, int y);
    static void send_reset(void *obj);

    void *net_obj;
    void *gui_obj;
    sendtxt_func_t sendtxt_func;
    droppiece_func_t droppiece_func;
    resetgui_func_t resetgui_func;
    gameover_func_t gameover_func;
};
