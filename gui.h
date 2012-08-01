typedef void (*onclick_func_t) (void *obj, struct cell_t cell);
typedef enum drop_type (*canclick_func_t) (void *obj, struct cell_t cell);
typedef void (*resetgame_func_t) (void *obj);
typedef void (*undo_func_t) (void *obj);

class gui_t;
typedef void (gui_t::*draw_func_t) (struct anim_t * anim, float elapsed);

#define ANIM_LEN 0.6f
#define ANIM_FPS 1.0f/90.0f

enum anim_type {
    ANIM_DROP,
    ANIM_FLOATER_CLICK
};

struct anim_t {
    enum anim_type type;
    struct timeval start;
    float len;
    draw_func_t draw_func;

    enum drop_type drop_type;
    Fl_Color color;
    struct cell_t cell;
};

class gui_t:public Fl_Double_Window {
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
    int get_animation_for_cell(struct cell_t cell);
    int valid(struct cell_t cell);
    int handle_shortcut(int event);
    int handle_mouse_click(struct cell_t event_cell);
    int handle_mouse_move(struct cell_t event_cell);

    enum game_state state;

    Fl_Color clicks[BOARD_DIM][BOARD_DIM];
    struct anim_t anims[MAX];
    int num_anims;
    Fl_Color my_color;
    int animating;
    int square_dim;
    struct cell_t cursor_cell;
    struct cell_t prev_cursor_cell;
    int skip_animations;
    struct cell_t floater_hold_cell;
    int first_click;
    int disabled;
    struct cell_t last_click_cell;
    Fl_Color last_color;

public:
     gui_t(Fl_Color my_color, int w, int h, const char *l);

    static void set_active(void *obj, bool active);
    static void drop_piece(void *obj, struct cell_t cell, Fl_Color c,
                           enum drop_type type);
    static void reset_board(void *obj);
    static void process_anims(void *obj);
    static void enable_animations(void *obj);
    static void game_over(void *obj, enum game_state state);

    void *game_obj;
    onclick_func_t onclick_func;
    canclick_func_t canclick_func;
    resetgame_func_t resetgame_func;
    undo_func_t undo_func;
};
