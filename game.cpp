#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <FL/Enumerations.H>

#include "common.h"
#include "game.h"

game_t::game_t(Fl_Color color){
    my_color = color;
    printf("My color: %s\n", colorname(color));
    grid_dim = BOARD_DIM;
    reset();
}

void game_t::reset(void){
    int i, j;
    i_follow_color = BOARD_EMPTY;
    line_color = BOARD_EMPTY;
    line_len = -1;
    state = STATE_INIT;
    i_used_floater = false;
    num_in_order = 0;

    for(i=0;i<grid_dim;i++){
        for(j=0;j<grid_dim;j++){
            board[i][j] = BOARD_EMPTY;
        }
    }
}

enum drop_type game_t::get_drop_type(int x, int y){
    int i;
    int can_fall_from_left = true;
    int can_fall_from_right = true;
    int can_fall_from_top = true;
    int can_fall_from_bottom = true;

    if (board[x][y] != BOARD_EMPTY){
        return DROP_NONE;
    }

    //fall from left
    if (x < grid_dim - 1 && board[x+1][y] == BOARD_EMPTY){
        can_fall_from_left = false;
    } else {
        for(i=0;i<x;i++){
            if (board[i][y] != BOARD_EMPTY){
                can_fall_from_left = false;
                break;
            }
        }
    }

    if (can_fall_from_left){
        return DROP_LEFT;
    }
    
    //fall from right
    if (x > 0 && board[x-1][y] == BOARD_EMPTY){
        can_fall_from_right = false;
    } else {
        for(i=x+1;i<grid_dim;i++){
            if (board[i][y] != BOARD_EMPTY){
                can_fall_from_right = false;
                break;
            }
        }
    }

    if (can_fall_from_right){
        return DROP_RIGHT;
    }
    
    //fall from top
    if (y < grid_dim - 1 && board[x][y+1] == BOARD_EMPTY){
        can_fall_from_top = false;
    } else {
        for(i=0;i<y;i++){
            if (board[x][i] != BOARD_EMPTY){
                can_fall_from_top = false;
                break;
            }
        }
    }

    if (can_fall_from_top){
        return DROP_TOP;
    }

    //fall from bottom
    if (y > 0 && board[x][y-1] == BOARD_EMPTY){
        can_fall_from_bottom = false;
    } else {
        for(i=y+1;i<grid_dim;i++){
            if (board[x][i] != BOARD_EMPTY){
                can_fall_from_bottom = false;
                break;
            }
        }
    }

    if (can_fall_from_bottom){
        return DROP_BOTTOM;
    }

    return DROP_FLOATER;
}

bool game_t::stonify(int x, int y){
    int i;

    int left = 0;
    for(i=x-1;i>=0 && board[i][y] == board[x][y];i--){
        left++;
    }

    int right = 0;
    for(i=x+1;i<grid_dim && board[i][y] == board[x][y];i++){
        right++;
    }

    if (left + right + 1 >= 5){
        for(i=x-left;i<x+right+1;i++){
            set_piece(i, y, BOARD_STONE, DROP_FLOATER);
        }
        return true;
    }

    int up = 0;
    for(i=y-1;i>=0 && board[x][i] == board[x][y];i--){
        up++;
    }

    int down = 0;
    for(i=y+1;i<grid_dim && board[x][i] == board[x][y];i++){
        down++;
    }

    if (up + down + 1 >= 5){
        for(i=y-up;i<y+down+1;i++){
            set_piece(x, i, BOARD_STONE, DROP_FLOATER);
        }
        return true;
    }

    int topleft = 0;
    for(i=1;i<grid_dim && x-i >= 0 && y-i >= 0 && board[x-i][y-i] == board[x][y];i++){
        topleft++;
    }

    int topright = 0;
    for(i=1;i<grid_dim && x+i < grid_dim && y-i >= 0 && board[x+i][y-i] == board[x][y];i++){
        topright++;
    }

    int botleft = 0;
    for(i=1;i<grid_dim && x-i >= 0 && y+i < grid_dim && board[x-i][y+i] == board[x][y];i++){
        botleft++;
    }

    int botright = 0;
    for(i=1;i<grid_dim && x+i < grid_dim && y+i < grid_dim && board[x+i][y+i] == board[x][y];i++){
        botright++;
    }

    if (topleft + botright + 1 >= 5){
        for(i=0;i<topleft+1;i++){
            set_piece(x-i, y-i, BOARD_STONE, DROP_FLOATER);
        }
        for(i=1;i<botright+1;i++){
			set_piece(x+i, y+i, BOARD_STONE, DROP_FLOATER);
        }
        return true;
    }

    if (topright + botleft + 1 >= 5){
        for(i=0;i<topright+1;i++){
            set_piece(x+i, y-i, BOARD_STONE, DROP_FLOATER);
        }
        for(i=1;i<botleft+1;i++){
            set_piece(x-i, y+i, BOARD_STONE, DROP_FLOATER);
        }
        return true;
    }

    return false;
}

void game_t::unmatched_line(Fl_Color color){
    if (color == line_color){
        line_color = BOARD_EMPTY;
        line_len = -1;
    } else if (color == my_color){
        state = STATE_GAMEOVER;
        gameover_func(gui_obj, false);
    } else {
        Fl_Color new_follow_color = remove_from_order(color);
        if (i_follow_color == color){
            i_follow_color = new_follow_color;
            most_recent_color = i_follow_color;
        }
        if (num_in_order == 1){
            if (order[0] == my_color){
                gameover_func(gui_obj, true);
            }
            state = STATE_GAMEOVER;
        }
    }
}

void game_t::set_piece(int x, int y, Fl_Color color, enum drop_type type){
    board[x][y] = color;
    if (color == BOARD_STONE){
        droppiece_func(gui_obj, x, y, color, type);
    } else {
        bool already_became_stone = stonify(x, y);
        if (!already_became_stone){
            droppiece_func(gui_obj, x, y, color, type);
            if (line_color != BOARD_EMPTY){
                //someone just lost
                unmatched_line(color);
            }
        } else if (line_color == BOARD_EMPTY){
            line_color = color;
        }
    }
}

enum drop_type game_t::drop_available(void *obj, int x, int y){
    game_t *that = (game_t *)obj;

    enum drop_type type = that->get_drop_type(x, y);

    if (type == DROP_NONE){
        return DROP_NONE;
    }

    switch (that->state){
        case STATE_INIT:
            return DROP_FLOATER;
            break;
        case STATE_PLAYING:
            if (that->most_recent_color != that->i_follow_color){
                return DROP_NONE;
            }
            if ((that->get_drop_type(x, y) == DROP_FLOATER) && that->i_used_floater){
                return DROP_NONE;
            }
            return type;
            break;
        case STATE_SET_ORDER:
            if (that->i_follow_color != BOARD_EMPTY){
                return DROP_NONE;
            }
            return type;
            break;
        case STATE_GAMEOVER:
        default:
            return DROP_NONE;
    }

    return DROP_NONE;
}

bool game_t::parse_cls(void *obj, const char *packet){
    game_t *that = (game_t *)obj;

    if (strcmp(packet, "0")){
        return false;
    }

    that->reset();
    that->resetgui_func(that->gui_obj);

    return true;
}

Fl_Color game_t::remove_from_order(Fl_Color color){
    int i, j;
    for(i=0;i<num_in_order;i++){
        if (order[i] == color){
            for(j=i;j<num_in_order;j++){
                order[j] = order[j+1];
            }
            num_in_order--;
            if (i == 0){
                return order[num_in_order - 1];
            } else {
                return order[i-1];
            }
        }
    }
    return BOARD_EMPTY;
}
            

void game_t::add_to_order(Fl_Color color){
    order[num_in_order] = color;
    num_in_order++;
}

void game_t::print_order(){
    int i;
    printf("order: {");
    for(i=0;i<num_in_order;i++){
        printf("%s,", colorname(order[i]));
    }
    printf("}\n");
}

bool game_t::parse_clk(void *obj, const char *packet){
    game_t *that = (game_t *)obj;

    if (!startswith(packet, "clk ")){
        return false;
    }

    int x, y;
    unsigned c, is_floater;
    Fl_Color color;
    if (sscanf(packet, "clk %d %d %u %u", &x, &y, &c, &is_floater) != 4){
        printf("couldn't parse a clk out of '%s'\n", packet);
        return true;
    }
    color = (Fl_Color)c;

    enum drop_type type = that->get_drop_type(x, y);

    if (color == that->my_color && type == DROP_FLOATER && that->state != STATE_INIT){
        that->i_used_floater = true;
    }

    switch (that->state){
    case STATE_INIT:
        that->state = STATE_SET_ORDER;
        that->add_to_order(color);
        break;
    case STATE_SET_ORDER:
        if (color == that->my_color){
            that->i_follow_color = that->most_recent_color;
        }
        if (color == that->order[0]){
            that->state = STATE_PLAYING;
        } else {
            that->add_to_order(color);
        }
        break;
    case STATE_PLAYING:
    case STATE_GAMEOVER:
    default:
        break;
    }
    that->most_recent_color = color;

    that->set_piece(x, y, color, type);
    return true;
}

void game_t::local_click(void *obj, int x, int y){
    game_t *that = (game_t *)obj;

    if (drop_available(obj, x, y) == DROP_NONE){
        return;
    }

    int is_floater = 0;
    if (that->get_drop_type(x, y) == DROP_FLOATER){
        is_floater = 1;
    }

    that->sendtxt_func(that->net_obj, "clk %d %d %u %u", x, y, that->my_color, is_floater);
}

void game_t::send_reset(void *obj){
    game_t *that = (game_t *)obj;

    that->sendtxt_func(that->net_obj, "cls");
}
