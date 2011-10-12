typedef void (*onclick_func_t)(void *obj, int x, int y);
class gui_t;

typedef enum drop_type (*canclick_func_t)(void *obj, int x, int y);
typedef void (*resetgame_func_t)(void *obj);
typedef void (gui_t::*draw_func_t)(struct anim_t *anim, float elapsed);

#define ANIM_LEN 0.7f
#define ANIM_FPS 1.0f/60.0f

enum anim_type {
    ANIM_DROP,
    ANIM_WIN,
    ANIM_FLOATER_CLICK
};

struct anim_t {
    enum anim_type type;
    struct timeval start;
    float len;
    draw_func_t draw_func;

    enum drop_type drop_type;
    Fl_Color color;
    int x, y;

    bool is_win;
};

class gui_t : public Fl_Double_Window {
private:
    int handle(int event);
    void draw(void);
    void draw_anim(struct anim_t *anim);
    float bounceOut(float t, float b, float c, float d);
    float time_diff(struct timeval *first, struct timeval *second);
    void draw_anim_win(struct anim_t *anim, float elapsed);
    void draw_anim_drop(struct anim_t *anim, float elapsed);
    void draw_anim_floater_click(struct anim_t *anim, float elapsed);
    struct anim_t remove_animation(int idx);
    int get_animation_for_cell(int x, int y);

    enum {
        STATE_PLAYING,
        STATE_WON,
        STATE_LOST
    } state;

    Fl_Color clicks[BOARD_DIM][BOARD_DIM];
    struct anim_t anims[32];
    int num_anims;
    Fl_Color my_color;
    int animating;
    int square_dim;
    int cursor_x;
    int cursor_y;
    int prev_cursor_x;
    int prev_cursor_y;
    int skip_animations;
    int floater_hold_x;
    int floater_hold_y;
    int first_click;

    void *onclick_obj;
    void *canclick_obj;
    void *resetgame_obj;
    onclick_func_t onclick_func;
    canclick_func_t canclick_func;
    resetgame_func_t resetgame_func;

public:
    gui_t(Fl_Color my_color, int w, int h, const char *l);

    static void set_active(void *obj, bool active);
	static void drop_piece(void *obj, int x, int y, Fl_Color c, enum drop_type type);
    static void reset_board(void *obj);
    static void process_anims(void *obj);
    static void enable_animations(void *obj);
    static void game_over(void *obj, bool won);

    void set_onclick_func(void *obj, onclick_func_t func);
    void set_canclick_func(void *obj, canclick_func_t func);
    void set_resetgame_func(void *obj, resetgame_func_t func);
};
