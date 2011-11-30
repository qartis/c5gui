#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_draw.H>
#include <FL/names.h>
#include <time.h>
#ifndef WIN32
#include <sys/time.h>
#endif

#include "common.h"
#include "gui.h"

#ifdef WIN32
//struct timeval { int tv_sec, tv_usec; };
int gettimeofday(struct timeval *tv, struct timezone *tz){
    DWORD t = timeGetTime ();

    tv->tv_sec = t / 1000;
    tv->tv_usec = (t % 1000) * 1000;
	return 0;
}
#endif

int gui_t::handle(int event){
    int len = w();
    if (h() < len){
            len =  h();
    }
    square_dim = len / BOARD_DIM;
    int event_x = Fl::event_x() / square_dim;
    int event_y = Fl::event_y() / square_dim;
    enum drop_type drop_type;
    switch(event){
    case FL_LEAVE:
        cursor_x = -1;
        cursor_y = -1;
        redraw();
        return 1;
    case FL_MOVE:
        if (canclick_func(game_obj, event_x, event_y)){
            if (event_x != cursor_x || event_y != cursor_y){
                cursor_x = event_x;
                cursor_y = event_y;
                redraw();
            }
        } else {
            cursor_x = -1;
            cursor_y = -1;
            redraw();
        }
        return 1;
    case FL_PUSH:
        if (Fl::event_button() != FL_LEFT_MOUSE){
            return 0;
        }
        if (disabled){
            return 1;
        }
        drop_type = canclick_func(game_obj, event_x, event_y);
        if (drop_type == DROP_NONE){
            return 1;
        }
        if (drop_type == DROP_FLOATER && !first_click && (floater_hold_x != event_x || floater_hold_y != event_y)){
            if (floater_hold_x == -1 || floater_hold_y == -1){
                floater_hold_x = event_x;
                floater_hold_y = event_y;
            } else {
                floater_hold_x = -1;
                floater_hold_y = -1;
            }
            redraw();
            return 1;
        }
        floater_hold_x = -1;
        floater_hold_y = -1;
        printf("clicked\n");
        onclick_func(game_obj, event_x, event_y);
        disabled = 1;
        return 1;
    case FL_SHORTCUT:
        if ((Fl::event_state() & FL_SHIFT) && (Fl::event_key() == FL_Delete)){
            resetgame_func(game_obj);
            return 1;
        }
        //fall through
    default:
        return Fl_Double_Window::handle(event);
    }
}

void gui_t::draw_anim_win(struct anim_t *anim, float elapsed){
    (void)anim;
    (void)elapsed;
    //TODO
    printf("WIN\n");
}

void gui_t::draw_anim_floater_click(struct anim_t *anim, float elapsed){
    (void)anim;
    (void)elapsed;
    //TODO
    printf("floater click\n");
}

void gui_t::draw_anim_drop(struct anim_t *anim, float elapsed){
    int draw_size = 20;
    int size_increment;
    fl_color(anim->color);

    int draw_x = anim->x * square_dim;
    int draw_y = anim->y * square_dim;
    switch (anim->drop_type){
    case DROP_LEFT:
        draw_x = (int)bounceOut(elapsed, 0, anim->x * square_dim, ANIM_LEN);
        break;
    case DROP_RIGHT:
        draw_x = w() - (int)bounceOut(elapsed, 0, w() - anim->x * square_dim, ANIM_LEN);
        break;
    case DROP_TOP:
        draw_y = (int)bounceOut(elapsed, 0, anim->y * square_dim, ANIM_LEN);
        break;
    case DROP_BOTTOM:
        draw_y = h() - (int)bounceOut(elapsed, 0, h() - anim->y * square_dim, ANIM_LEN);
        break;
    case DROP_FLOATER:
#define BIG 600
        size_increment = BIG - (int)bounceOut(elapsed, 0, BIG, ANIM_LEN);
        draw_size = draw_size + size_increment;
        draw_x -= size_increment / 2;
        draw_y -= size_increment / 2;
        break;
    case DROP_NONE:
    default:
        draw_x = (int)bounceOut(elapsed, 0, anim->x * square_dim, ANIM_LEN);
        draw_y = (int)bounceOut(elapsed, 0, anim->y * square_dim, ANIM_LEN);
    }
    fl_rectf(draw_x, draw_y, draw_size, draw_size);
}

struct anim_t gui_t::remove_animation(int idx){
    struct anim_t anim = anims[idx];
    memmove(&(anims[idx]), &(anims[idx+1]), (num_anims-idx-1) * sizeof(anims[0]));
    num_anims--;
    return anim;
}

void gui_t::draw() {
    Fl_Double_Window::draw();
    fl_line_style(FL_SOLID);
    fl_color(0, 0, 0);
    square_dim = w() / BOARD_DIM;
    square_dim = h() / BOARD_DIM;
    fl_rectf(0, 0, square_dim * BOARD_DIM, square_dim * BOARD_DIM, BOARD_EMPTY);
    int i, j;

    if (cursor_x > -1 && cursor_y > -1){
        fl_color(fl_lighter(my_color));
        fl_rectf(cursor_x * square_dim + 1, cursor_y * square_dim + 1, square_dim - 1, square_dim - 1);
    }

    if (floater_hold_x != -1 && floater_hold_y != -1){
        fl_color(fl_lighter(my_color));
        fl_rectf(floater_hold_x * square_dim + 1, floater_hold_y * square_dim + 1, square_dim - 1, square_dim - 1);
    }

    for(i=0;i<BOARD_DIM;i++){
        for(j=0;j<BOARD_DIM;j++){
            if (clicks[i][j] != BOARD_EMPTY){
                fl_rectf(i * square_dim + 1, j * square_dim + 1, square_dim - 1, square_dim - 1, clicks[i][j]);
            }
        }
    }

    struct timeval now;


    gettimeofday(&now, NULL);
    animating = 0;
    for(i=0;i<num_anims;i++){
        struct anim_t *anim = &(anims[i]);
        float elapsed = time_diff(&(anim->start), &now);
        if (anim->len > 0 && (elapsed > anim->len)){
            struct anim_t removed_anim = remove_animation(i);
            i--;
            if (removed_anim.type == ANIM_DROP){
                clicks[removed_anim.x][removed_anim.y] = removed_anim.color;
                fl_color(removed_anim.color);
                fl_rectf(removed_anim.x * square_dim, removed_anim.y * square_dim, 20, 20);
                fl_color(FL_BLACK);
                fl_rect(removed_anim.x * square_dim, removed_anim.y * square_dim, 20, 20);
            } 
        } else {
            (this->*(anim->draw_func))(anim, elapsed);
            animating = 1;
        }
    }

    fl_rect(0,0,square_dim * BOARD_DIM,square_dim * BOARD_DIM, FL_BLACK);
    for(i=0;i<BOARD_DIM;i++){
        fl_line(square_dim * i, 0, square_dim * i, square_dim * BOARD_DIM - 1);
    }
    for(i=0;i<BOARD_DIM;i++){
        fl_line(0, square_dim * i, square_dim*BOARD_DIM - 1, square_dim * i);
    }

    if (state != STATE_PLAYING){
        const char *msg;
        if (state == STATE_WON){
            msg = "You Won";
            fl_color(FL_GREEN);
        } else {
            msg = "You Lost";
            fl_color(FL_RED);
        }
        fl_font(FL_HELVETICA, 100);
        fl_draw(msg, 0, 0, w(), h(), FL_ALIGN_CENTER, NULL, 0);
    }
}

void gui_t::reset_board(void *obj){
    gui_t *that = (gui_t *)obj;

    that->num_anims = 0;
    that->animating = 0;
    that->state = STATE_PLAYING;
    that->floater_hold_x = -1;
    that->floater_hold_y = -1;
    that->first_click = 1;
    that->disabled = 0;
    that->last_x = -1;
    that->last_y = -1;

    int i, j;
    for(i=0;i<BOARD_DIM;i++){
        for(j=0;j<BOARD_DIM;j++){
            that->clicks[i][j] = BOARD_EMPTY;
        }
    }

    that->redraw();
}

gui_t::gui_t(Fl_Color _my_color, int width, int height, const char *title) : Fl_Double_Window(width, height, title){
    end();
    cursor_x = -1;
    cursor_y = -1;
    skip_animations = true;
    Fl::add_timeout(ANIM_LEN, &gui_t::enable_animations, this);
    my_color = _my_color;
    reset_board(this);
}

int gui_t::get_animation_for_cell(int x, int y){
    int i;
    for(i=0;i<num_anims;i++){
        if (anims[i].x == x && anims[i].y == y){
            return i;
        }
    }
    return -1;
}

void gui_t::drop_piece(void *obj, int x, int y, Fl_Color color, enum drop_type type){
    gui_t *that = (gui_t *)obj;
    that->first_click = 0;
    that->disabled = 0;
    if (that->last_x != -1 && that->last_y != -1){
        that->clicks[that->last_x][that->last_y] = that->last_color;
    }
    that->last_x = x;
    that->last_y = y;
    that->last_color = color;
    if (color != BOARD_STONE){
        color = fl_darker(color);
    }
    if (that->skip_animations){
        that->clicks[x][y] = color;
        that->redraw();
        return;
    }
    int idx = that->get_animation_for_cell(x, y);
    if (idx == -1 || color != BOARD_STONE){
        idx = that->num_anims;
        that->num_anims++;
    }
    struct anim_t *anim = &(that->anims[idx]);
    anim->drop_type = type;
    anim->type = ANIM_DROP;
    anim->draw_func = &gui_t::draw_anim_drop;
    anim->x = x;
    anim->y = y;
    anim->color = color;
    anim->len = ANIM_LEN;
    gettimeofday(&(anim->start), NULL);
    that->animating = 1;
    Fl::remove_timeout(&gui_t::process_anims);
    Fl::add_timeout(0, &gui_t::process_anims, obj);

    /* Stone pieces cover up any existing pieces */
    if (color == BOARD_STONE){
        that->clicks[x][y] = BOARD_EMPTY;
    }
}

float gui_t::time_diff(struct timeval *first, struct timeval *second){
    time_t sec = second->tv_sec - first->tv_sec;
    long usec = second->tv_usec - first->tv_usec;

    if (usec < 0){
        usec += 1000000;
        sec -= 1;
    }

    return (float)sec + (float)usec/1000000.0f;
}

float gui_t::bounceOut(float t, float b, float c, float d) {
    //current time t, original value b, total change c, total duration d
    float tweak = 7.5625f;
    if ((t/=d) < (1/2.75f)) {
        return c*(tweak*t*t) + b;
    } else if (t < (2/2.75f)) {
        float postFix = t-=(1.5f/2.75f);
        return c*(tweak*(postFix)*t + .75f) + b;
    } else if (t < (2.5/2.75)) {
        float postFix = t-=(2.25f/2.75f);
        return c*(tweak*(postFix)*t + .9375f) + b;
    } else {
        float postFix = t-=(2.625f/2.75f);
        return c*(tweak*(postFix)*t + .984375f) + b;
    }
}

void gui_t::process_anims(void *obj){
    gui_t *that = (gui_t *)obj;
    that->redraw();
    if (that->animating){
        Fl::repeat_timeout(ANIM_FPS, &gui_t::process_anims, obj);
    }
}

void gui_t::enable_animations(void *obj){
    gui_t *that = (gui_t *)obj;

    that->skip_animations = 0;
}

void gui_t::game_over(void *obj, bool won){
    gui_t *that = (gui_t *)obj;

    if (won){
        printf("I WON!\n");
        that->state = STATE_WON;
    } else {
        printf("I LOST\n");
        that->state = STATE_LOST;
    }
    that->redraw();
}
