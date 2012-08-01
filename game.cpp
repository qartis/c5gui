#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <FL/Enumerations.H>

#include "common.h"
#include "game.h"

game_t::game_t(Fl_Color color)
{
    my_color = color;
    printf("My color: %s\n", colorname(color));
    grid_dim = BOARD_DIM;
    reset();
}

void game_t::reset(void)
{
    int i, j;
    i_follow_color = BOARD_EMPTY;
    cur_line_color = BOARD_EMPTY;
    cur_line_len = -1;
    state = STATE_INIT;
    i_used_floater = false;
    num_in_order = 0;
    num_turns = 0;

    for (i = 0; i < grid_dim; i++) {
        for (j = 0; j < grid_dim; j++) {
            board[i][j] = BOARD_EMPTY;
        }
    }
}

enum drop_type game_t::get_drop_type(struct cell_t cell)
{
    int i;
    int can_fall_from_left = true;
    int can_fall_from_right = true;
    int can_fall_from_top = true;
    int can_fall_from_bottom = true;

    int x = cell.x;
    int y = cell.y;

    if (board[x][y] != BOARD_EMPTY) {
        return DROP_NONE;
    }
    //fall from left
    if (x < grid_dim - 1 && board[x + 1][y] == BOARD_EMPTY) {
        can_fall_from_left = false;
    } else {
        for (i = 0; i < x; i++) {
            if (board[i][y] != BOARD_EMPTY) {
                can_fall_from_left = false;
                break;
            }
        }
    }

    if (can_fall_from_left) {
        return DROP_LEFT;
    }
    //fall from right
    if (x > 0 && board[x - 1][y] == BOARD_EMPTY) {
        can_fall_from_right = false;
    } else {
        for (i = x + 1; i < grid_dim; i++) {
            if (board[i][y] != BOARD_EMPTY) {
                can_fall_from_right = false;
                break;
            }
        }
    }

    if (can_fall_from_right) {
        return DROP_RIGHT;
    }
    //fall from top
    if (y < grid_dim - 1 && board[x][y + 1] == BOARD_EMPTY) {
        can_fall_from_top = false;
    } else {
        for (i = 0; i < y; i++) {
            if (board[x][i] != BOARD_EMPTY) {
                can_fall_from_top = false;
                break;
            }
        }
    }

    if (can_fall_from_top) {
        return DROP_TOP;
    }
    //fall from bottom
    if (y > 0 && board[x][y - 1] == BOARD_EMPTY) {
        can_fall_from_bottom = false;
    } else {
        for (i = y + 1; i < grid_dim; i++) {
            if (board[x][i] != BOARD_EMPTY) {
                can_fall_from_bottom = false;
                break;
            }
        }
    }

    if (can_fall_from_bottom) {
        return DROP_BOTTOM;
    }

    return DROP_FLOATER;
}

bool game_t::valid(int x, int y)
{
    return (0 <= x && x < grid_dim) && (0 <= y && y < grid_dim);
}

int game_t::stonify(struct cell_t cell)
{
    int i;
    int len = 0;

    int left = 0;
    int right = 0;
    int up = 0;
    int down = 0;
    int topleft = 0;
    int topright = 0;
    int botleft = 0;
    int botright = 0;

    int x = cell.x;
    int y = cell.y;

    for (i = x - 1; valid(i, y) && board[i][y] == board[x][y]; i--) {
        left++;
    }

    for (i = x + 1; valid(i, y) && board[i][y] == board[x][y]; i++) {
        right++;
    }

    for (i = y - 1; valid(x, y) && board[x][i] == board[x][y]; i--) {
        up++;
    }

    for (i = y + 1; valid(x, i) && board[x][i] == board[x][y]; i++) {
        down++;
    }

    for (i = 1; valid(x - i, y - i) && board[x - i][y - i] == board[x][y]; i++) {
        topleft++;
    }

    for (i = 1; valid(x + i, y - i) && board[x + i][y - i] == board[x][y]; i++) {
        topright++;
    }

    for (i = 1; valid(x - i, y + i) && board[x - i][y + i] == board[x][y]; i++) {
        botleft++;
    }

    for (i = 1; valid(x + i, y + i) && board[x + i][y + i] == board[x][y]; i++) {
        botright++;
    }

    if (left + right + 1 >= 5) {
        for (i = 0; i < left; i++) {
            set_piece(x - (i + 1), y, BOARD_STONE, DROP_FLOATER);
            len++;
        }
        for (i = 0; i < right; i++) {
            set_piece(x + (i + 1), y, BOARD_STONE, DROP_FLOATER);
            len++;
        }
    }

    if (up + down + 1 >= 5) {
        for (i = 0; i < up; i++) {
            set_piece(x, y - (i + 1), BOARD_STONE, DROP_FLOATER);
            len++;
        }
        for (i = 0; i < down; i++) {
            set_piece(x, y + (i + 1), BOARD_STONE, DROP_FLOATER);
            len++;
        }
    }

    if (topleft + botright + 1 >= 5) {
        for (i = 0; i < topleft; i++) {
            set_piece(x - (i + 1), y - (i + 1), BOARD_STONE, DROP_FLOATER);
            len++;
        }
        for (i = 0; i < botright; i++) {
            set_piece(x + (i + 1), y + (i + 1), BOARD_STONE, DROP_FLOATER);
            len++;
        }
    }

    if (topright + botleft + 1 >= 5) {
        for (i = 0; i < topright; i++) {
            set_piece(x + (i + 1), y - (i + 1), BOARD_STONE, DROP_FLOATER);
            len++;
        }
        for (i = 0; i < botleft; i++) {
            set_piece(x - (i + 1), y + (i + 1), BOARD_STONE, DROP_FLOATER);
            len++;
        }
    }

    if (len > 0) {
        set_piece(x, y, BOARD_STONE, DROP_FLOATER);
        len++;
    }

    return len;
}

void game_t::unmatched_line(Fl_Color color)
{
    if (color == cur_line_color) {
        cur_line_color = BOARD_EMPTY;
        cur_line_len = -1;
    } else if (color == my_color) {
        state = STATE_LOST;
        gameover_func(gui_obj, STATE_LOST);
    } else {
        Fl_Color new_follow_color = remove_from_order(color);
        if (i_follow_color == color) {
            i_follow_color = new_follow_color;
            most_recent_color = i_follow_color;
        }
        if (num_in_order == 1) {
            if (order[0] == my_color) {
                state = STATE_WON;
            } else {
                state = STATE_OVER;
            }
            gameover_func(gui_obj, state);
        }
    }
}

void game_t::set_piece(int x, int y, Fl_Color color, enum drop_type type)
{
    struct cell_t cell;
    cell.x = x;
    cell.y = y;

    board[cell.x][cell.y] = color;

    turns[num_turns].cell = cell;
    turns[num_turns].color = color;
    num_turns++;

    if (color == BOARD_STONE) {
        droppiece_func(gui_obj, cell, color, type);
        return;
    }

    int line_len = stonify(cell);
    if (line_len == 0) {
        droppiece_func(gui_obj, cell, color, type);
        if (cur_line_len > 0) {
            unmatched_line(color);
        }
    } else if (cur_line_len == -1 || color == cur_line_color) {
        cur_line_color = color;
        cur_line_len = line_len;
    } else if (line_len < cur_line_len) {
        unmatched_line(color);
    }
}

enum drop_type game_t::drop_available(void *obj, struct cell_t cell)
{
    game_t *that = (game_t *) obj;

    enum drop_type type = that->get_drop_type(cell);

    if (type == DROP_NONE) {
        return DROP_NONE;
    }

    switch (that->state) {
    case STATE_INIT:
        return DROP_FLOATER;
        break;
    case STATE_PLAYING:
        if (that->most_recent_color != that->i_follow_color) {
            return DROP_NONE;
        }
        if ((type == DROP_FLOATER) && that->i_used_floater) {
            return DROP_NONE;
        }
        return type;
        break;
    case STATE_SET_ORDER:
        if (that->i_follow_color != BOARD_EMPTY) {
            return DROP_NONE;
        }
        return type;
        break;
    default:
        return DROP_NONE;
    }

    return DROP_NONE;
}

bool game_t::parse_cls(void *obj, const char *packet)
{
    game_t *that = (game_t *) obj;

    if (strcmp(packet, "0")) {
        return false;
    }

    that->reset();
    that->resetgui_func(that->gui_obj);

    return true;
}

bool game_t::parse_undo(void *obj, const char *packet)
{
    game_t *that = (game_t *) obj;

    if (!startswith(packet, "undo")) {
        return false;
    }

    that->reset();
    that->resetgui_func(that->gui_obj);

    return true;
}

Fl_Color game_t::remove_from_order(Fl_Color color)
{
    int i, j;
    for (i = 0; i < num_in_order; i++) {
        if (order[i] == color) {
            for (j = i; j < num_in_order; j++) {
                order[j] = order[j + 1];
            }
            num_in_order--;
            if (i == 0) {
                return order[num_in_order - 1];
            } else {
                return order[i - 1];
            }
        }
    }
    return BOARD_EMPTY;
}

void game_t::add_to_order(Fl_Color color)
{
    order[num_in_order] = color;
    num_in_order++;
}

void game_t::print_order()
{
    int i;
    printf("order: {");
    for (i = 0; i < num_in_order; i++) {
        printf("%s,", colorname(order[i]));
    }
    printf("}\n");
}

bool game_t::parse_clk(void *obj, const char *packet)
{
    game_t *that = (game_t *) obj;

    if (!startswith(packet, "clk ")) {
        return false;
    }

    int x, y;
    unsigned c, is_floater;
    Fl_Color color;
    struct cell_t cell;
    if (sscanf(packet, "clk %d %d %u %u", &x, &y, &c, &is_floater) != 4) {
        printf("couldn't parse a clk out of '%s'\n", packet);
        return true;
    }
    color = (Fl_Color) c;
    cell.x = x;
    cell.y = y;

    enum drop_type type = that->get_drop_type(cell);

    if (color == that->my_color && type == DROP_FLOATER
        && that->state != STATE_INIT) {
        that->i_used_floater = true;
    }

    switch (that->state) {
    case STATE_INIT:
        that->state = STATE_SET_ORDER;
        that->add_to_order(color);
        break;
    case STATE_SET_ORDER:
        if (color == that->my_color) {
            that->i_follow_color = that->most_recent_color;
        }
        if (color == that->order[0]) {
            that->state = STATE_PLAYING;
        } else {
            that->add_to_order(color);
        }
        break;
    default:
        break;
    }
    that->most_recent_color = color;

    that->set_piece(cell.x, cell.y, color, type);
    return true;
}

void game_t::local_click(void *obj, struct cell_t cell)
{
    game_t *that = (game_t *) obj;

    if (drop_available(obj, cell) == DROP_NONE) {
        return;
    }

    int is_floater = 0;
    if (that->get_drop_type(cell) == DROP_FLOATER) {
        is_floater = 1;
    }

    that->sendtxt_func(that->net_obj, "clk %d %d %u %u", cell.x, cell.y,
                       that->my_color, is_floater);
}

void game_t::send_reset(void *obj)
{
    game_t *that = (game_t *) obj;

    that->sendtxt_func(that->net_obj, "cls");
}

void game_t::undo(void *obj)
{
    game_t *that = (game_t *) obj;

    that->sendtxt_func(that->net_obj, "undo");
}
