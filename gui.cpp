#include <stdio.h>
#include <string.h>
#include <time.h>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_draw.H>
#include <FL/names.h>
#ifndef WIN32
#include <sys/time.h>
#endif

#include "common.h"
#include "gui.h"

#ifdef WIN32
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    DWORD t = timeGetTime();

    tv->tv_sec = t / 1000;
    tv->tv_usec = (t % 1000) * 1000;
    return 0;
}
#endif

int gui_t::handle(int event)
{
    int len = w();
    if (h() < len) {
        len = h();
    }
    square_dim = len / BOARD_DIM;
    struct cell_t event_cell;
    event_cell.x = Fl::event_x() / square_dim;
    event_cell.y = Fl::event_y() / square_dim;
    switch (event) {
    case FL_LEAVE:
        cursor_cell = INVALID_CELL;
        redraw();
        return 1;
    case FL_MOVE:
        return handle_mouse_move(event_cell);
        return 1;
    case FL_PUSH:
        if (Fl::event_button() != FL_LEFT_MOUSE) {
            return 0;
        }
        return handle_mouse_click(event_cell);
        break;
    case FL_SHORTCUT:
        if (handle_shortcut(event)) {
            redraw();
            return 1;
        }
        //fall through
    default:
        return Fl_Double_Window::handle(event);
    }
    return 0;
}

int gui_t::handle_mouse_move(struct cell_t event_cell)
{
    if (canclick_func(game_obj, event_cell)) {
        if (cellcmp(cursor_cell, event_cell)) {
            cursor_cell = event_cell;
            redraw();
        }
    } else {
        cursor_cell = INVALID_CELL;
        redraw();
    }

    return 1;
}

int gui_t::handle_mouse_click(struct cell_t event_cell)
{
    enum drop_type drop_type = canclick_func(game_obj, event_cell);

    if (disabled) {
        return 1;
    }
    if (valid(floater_hold_cell) && cellcmp(floater_hold_cell, event_cell)) {
        floater_hold_cell = INVALID_CELL;
        redraw();
        return 1;
    }
    if (drop_type == DROP_NONE) {
        return 1;
    }
    if (drop_type == DROP_FLOATER && !first_click
        && cellcmp(floater_hold_cell, event_cell)) {
        floater_hold_cell = event_cell;
        redraw();
        return 1;
    }
    floater_hold_cell = INVALID_CELL;
    onclick_func(game_obj, event_cell);
    disabled = 1;
    redraw();
    return 1;
}

int gui_t::handle_shortcut(int event)
{
    if ((Fl::event_state() & FL_SHIFT) && (Fl::event_key() == FL_Delete)) {
        resetgame_func(game_obj);
        return 1;
    }

    if ((Fl::event_state() & FL_CTRL) && (Fl::event_key() == 'z')) {
        if (valid(floater_hold_cell)) {
            floater_hold_cell = INVALID_CELL;
        } else {
            undo_func(game_obj);
        }
        return 1;
    }

    return 0;
}

void gui_t::draw_anim_win(struct anim_t *anim, float elapsed)
{
    (void)anim;
    (void)elapsed;
    //TODO
    printf("WIN\n");
}

void gui_t::draw_anim_floater_click(struct anim_t *anim, float elapsed)
{
    (void)anim;
    (void)elapsed;
    //TODO
    printf("floater click\n");
}

void gui_t::draw_anim_drop(struct anim_t *anim, float elapsed)
{
    int draw_size = 20;
    int size_increment;
    fl_color(anim->color);

    int draw_x = anim->cell.x * square_dim;
    int draw_y = anim->cell.y * square_dim;
    switch (anim->drop_type) {
    case DROP_LEFT:
        draw_x =
            (int)bounceOut(elapsed, 0, anim->cell.x * square_dim, ANIM_LEN);
        break;
    case DROP_RIGHT:
        draw_x =
            w() - (int)bounceOut(elapsed, 0, w() - anim->cell.x * square_dim,
                                 ANIM_LEN);
        break;
    case DROP_TOP:
        draw_y =
            (int)bounceOut(elapsed, 0, anim->cell.y * square_dim, ANIM_LEN);
        break;
    case DROP_BOTTOM:
        draw_y =
            h() - (int)bounceOut(elapsed, 0, h() - anim->cell.y * square_dim,
                                 ANIM_LEN);
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
        draw_x =
            (int)bounceOut(elapsed, 0, anim->cell.x * square_dim, ANIM_LEN);
        draw_y =
            (int)bounceOut(elapsed, 0, anim->cell.y * square_dim, ANIM_LEN);
    }
    fl_rectf(draw_x, draw_y, draw_size, draw_size);
}

struct anim_t gui_t::remove_animation(int idx)
{
    struct anim_t anim = anims[idx];
    memmove(&(anims[idx]), &(anims[idx + 1]),
            (num_anims - idx - 1) * sizeof(anims[0]));
    num_anims--;
    return anim;
}

void gui_t::draw()
{
    Fl_Double_Window::draw();
    fl_line_style(FL_SOLID);
    fl_color(0, 0, 0);
    square_dim = w() / BOARD_DIM;
    square_dim = h() / BOARD_DIM;
    fl_rectf(0, 0, square_dim * BOARD_DIM, square_dim * BOARD_DIM, BOARD_EMPTY);
    int i, j;

    if (valid(cursor_cell)) {
        fl_color(fl_lighter(my_color));
        fl_rectf(cursor_cell.x * square_dim + 1, cursor_cell.y * square_dim + 1,
                 square_dim - 1, square_dim - 1);
    }

    if (valid(floater_hold_cell)) {
        fl_color(fl_lighter(fl_lighter(my_color)));
        fl_rectf(floater_hold_cell.x * square_dim + 1,
                 floater_hold_cell.y * square_dim + 1, square_dim - 1,
                 square_dim - 1);
    }

    for (i = 0; i < BOARD_DIM; i++) {
        for (j = 0; j < BOARD_DIM; j++) {
            if (clicks[i][j] != BOARD_EMPTY) {
                fl_rectf(i * square_dim + 1, j * square_dim + 1, square_dim - 1,
                         square_dim - 1, clicks[i][j]);
            }
        }
    }

    struct timeval now;

    gettimeofday(&now, NULL);
    animating = 0;
    for (i = 0; i < num_anims; i++) {
        struct anim_t *anim = &(anims[i]);
        float elapsed = time_diff(&(anim->start), &now);
        if (anim->len > 0 && (elapsed > anim->len)) {
            struct anim_t removed_anim = remove_animation(i);
            i--;
            if (removed_anim.type == ANIM_DROP) {
                /* This piece might already have been colored due to the
                   highlight being removed from it */
                if (clicks[removed_anim.cell.x][removed_anim.cell.y] ==
                    BOARD_EMPTY) {
                    clicks[removed_anim.cell.x][removed_anim.cell.y] =
                        removed_anim.color;
                }
                fl_color(clicks[removed_anim.cell.x][removed_anim.cell.y]);
                fl_rectf(removed_anim.cell.x * square_dim,
                         removed_anim.cell.y * square_dim, 20, 20);
                fl_color(FL_BLACK);
                fl_rect(removed_anim.cell.x * square_dim,
                        removed_anim.cell.y * square_dim, 20, 20);
            }
        } else {
            (this->*(anim->draw_func)) (anim, elapsed);
            animating = 1;
        }
    }

    fl_rect(0, 0, square_dim * BOARD_DIM, square_dim * BOARD_DIM, FL_BLACK);
    for (i = 0; i < BOARD_DIM; i++) {
        fl_line(square_dim * i, 0, square_dim * i, square_dim * BOARD_DIM - 1);
    }
    for (i = 0; i < BOARD_DIM; i++) {
        fl_line(0, square_dim * i, square_dim * BOARD_DIM - 1, square_dim * i);
    }

    if (state != STATE_PLAYING) {
        const char *msg;
        switch (state) {
        case STATE_WON:
            msg = "You Won";
            fl_color(FL_GREEN);
            break;
        case STATE_LOST:
            msg = "You Lost";
            fl_color(FL_RED);
            break;
        case STATE_OVER:
            msg = "Game Over";
            fl_color(FL_BLUE);
            break;
        }
        fl_font(FL_HELVETICA, 75);
        fl_draw(msg, 0, 0, w(), h(), FL_ALIGN_CENTER, NULL, 0);
    }
}

void gui_t::reset_board(void *obj)
{
    gui_t *that = (gui_t *) obj;

    that->num_anims = 0;
    that->animating = 0;
    that->state = STATE_PLAYING;
    that->floater_hold_cell = INVALID_CELL;
    that->first_click = 1;
    that->disabled = 0;
    that->last_click_cell = INVALID_CELL;

    int i, j;
    for (i = 0; i < BOARD_DIM; i++) {
        for (j = 0; j < BOARD_DIM; j++) {
            that->clicks[i][j] = BOARD_EMPTY;
        }
    }

    that->redraw();
}

gui_t::gui_t(Fl_Color _my_color, int width, int height,
             const char *title):Fl_Double_Window(width, height, title)
{
    end();
    cursor_cell = INVALID_CELL;
    skip_animations = true;
    Fl::add_timeout(ANIM_LEN, &gui_t::enable_animations, this);
    my_color = _my_color;
    reset_board(this);
}

int gui_t::get_animation_for_cell(struct cell_t cell)
{
    int i;
    for (i = 0; i < num_anims; i++) {
        if (cellcmp(anims[i].cell, cell) == 0) {
            return i;
        }
    }
    return -1;
}

int gui_t::valid(struct cell_t cell)
{
    return (cell.x != -1 && cell.y != -1);
}

void gui_t::drop_piece(void *obj, struct cell_t cell, Fl_Color color,
                       enum drop_type type)
{
    gui_t *that = (gui_t *) obj;
    that->first_click = 0;
    that->disabled = 0;
    if (that->valid(that->last_click_cell)) {
        that->clicks[that->last_click_cell.x][that->last_click_cell.y] =
            that->last_color;
    }
    that->last_click_cell = cell;
    that->last_color = color;
    if (color != BOARD_STONE) {
        color = fl_darker(color);
    }
    if (that->skip_animations) {
        that->clicks[cell.x][cell.y] = color;
        that->redraw();
        return;
    }
    int idx = that->get_animation_for_cell(cell);
    if (idx == -1 || color != BOARD_STONE) {
        idx = that->num_anims;
        that->num_anims++;
    }
    struct anim_t *anim = &(that->anims[idx]);
    anim->drop_type = type;
    anim->type = ANIM_DROP;
    anim->draw_func = &gui_t::draw_anim_drop;
    anim->cell = cell;
    anim->color = color;
    anim->len = ANIM_LEN;
    gettimeofday(&(anim->start), NULL);
    that->animating = 1;
    Fl::remove_timeout(&gui_t::process_anims);
    Fl::add_timeout(0, &gui_t::process_anims, obj);

    /* Stone pieces cover up any existing pieces */
    if (color == BOARD_STONE) {
        that->clicks[cell.x][cell.y] = BOARD_EMPTY;
    }
}

float gui_t::time_diff(struct timeval *first, struct timeval *second)
{
    time_t sec = second->tv_sec - first->tv_sec;
    long usec = second->tv_usec - first->tv_usec;

    if (usec < 0) {
        usec += 1000000;
        sec -= 1;
    }

    return (float)sec + (float)usec / 1000000.0f;
}

float gui_t::bounceOut(float t, float b, float c, float d)
{
    //current time t, original value b, total change c, total duration d
    float tweak = 7.5625f;
    if ((t /= d) < (1 / 2.75f)) {
        return c * (tweak * t * t) + b;
    } else if (t < (2 / 2.75f)) {
        float postFix = t -= (1.5f / 2.75f);
        return c * (tweak * (postFix) * t + .75f) + b;
    } else if (t < (2.5 / 2.75)) {
        float postFix = t -= (2.25f / 2.75f);
        return c * (tweak * (postFix) * t + .9375f) + b;
    } else {
        float postFix = t -= (2.625f / 2.75f);
        return c * (tweak * (postFix) * t + .984375f) + b;
    }
}

void gui_t::process_anims(void *obj)
{
    gui_t *that = (gui_t *) obj;
    that->redraw();
    if (that->animating) {
        Fl::repeat_timeout(ANIM_FPS, &gui_t::process_anims, obj);
    }
}

void gui_t::enable_animations(void *obj)
{
    gui_t *that = (gui_t *) obj;

    that->skip_animations = 0;
}

void gui_t::game_over(void *obj, enum game_state state)
{
    gui_t *that = (gui_t *) obj;

    that->state = state;

    that->redraw();
}
