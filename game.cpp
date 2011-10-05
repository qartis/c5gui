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

void game_t::set_piece(int x, int y, Fl_Color color, enum drop_type type){
    board[x][y] = color;
    if (color == BOARD_EMPTY){
        printf("WTF\nWTF\nWTF\n");
    }
    if (color == BOARD_STONE || color == BOARD_EMPTY){
        droppiece_func(droppiece_obj, x, y, color, type);
    } else {
        bool already_became_stone = stonify(x, y);
        printf("%s clicked%s\n", colorname(color), already_became_stone ? " (STONE)" : "");
        if (!already_became_stone){
            droppiece_func(droppiece_obj, x, y, color, type);
            if (line_color != BOARD_EMPTY){
                //someone just lost
                if (color == line_color){
                    printf("line round over\n");
                    line_color = BOARD_EMPTY;
                } else if (color == my_color){
                    state = STATE_GAMEOVER;
                    gameover_func(gameover_obj, false);
                } else {
                    Fl_Color new_follow_color = remove_from_order(color);
                    if (i_follow_color == color){
                        i_follow_color = new_follow_color;
                        printf("I (%s) was following %s, now I follow %s\n", colorname(my_color), colorname(color), colorname(new_follow_color));
                    }
                    printf("%s removed from order, new order:\n", colorname(line_color));
                    print_order();
                    if (num_in_order == 1){
                        gameover_func(gameover_obj, true);
                        state = STATE_GAMEOVER;
                    }
                }
            }
        } else {
            if (line_color == BOARD_EMPTY){
                line_color = color;
                printf("%s just got a line\n", colorname(color));
            } else {
                printf("%s matched the line\n", colorname(color));
            }
        }
    }
    if (color == my_color && type == DROP_FLOATER){
        i_used_floater = true;
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
printf("WTF WTF WTF\n");

    return DROP_NONE;
}

bool game_t::parse_cls(void *obj, const char *packet){
    game_t *that = (game_t *)obj;

    if (strcmp(packet, "0")){
        return false;
    }

    that->reset();
    that->resetgui_func(that->resetgui_obj);

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
    printf("Couldn't find that person in the order! color %d\n", color);
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
that->print_order();
    int x, y;
    unsigned c, is_floater;
    Fl_Color color;
    if (sscanf(packet, "clk %d %d %u %u", &x, &y, &c, &is_floater) != 4){
        printf("couldn't parse a clk out of '%s'\n", packet);
        return true;
    }
    color = (Fl_Color)c;

    enum drop_type type = that->get_drop_type(x, y);

    that->set_piece(x, y, color, type);

    switch (that->state){
    case STATE_INIT:
        //this is the first click of the game. record the color to know when the order is set
        that->state = STATE_SET_ORDER;
        that->add_to_order(color);
        break;
    case STATE_SET_ORDER:
        if (color == that->order[0]){
            //the order picking is over. there should be at least 2 people in the order but we don't care here
            that->state = STATE_PLAYING;
        } else {
            that->add_to_order(color);
        }
        //a new person joined the order. if it was us, then we follow the previous person
        if (color == that->my_color){
            that->i_follow_color = that->most_recent_color;
        }
        break;
    case STATE_PLAYING:
        if (color == that->my_color){
            //that->myturn_func(that->myturn_obj, true);
        }
        //TODO check for win condition here
        break;
    case STATE_GAMEOVER:
    default:
        //this won't happen, so ignore it
        break;
    }
    that->most_recent_color = color;
    return true;
}

void game_t::set_sendtxt_func(void *obj, sendtxt_func_t func){
    sendtxt_obj = obj;
    sendtxt_func = func;
}

void game_t::set_droppiece_func(void *obj, droppiece_func_t func){
    droppiece_obj = obj;
    droppiece_func = func;
}

void game_t::set_resetgui_func(void *obj, resetgui_func_t func){
    resetgui_obj = obj;
    resetgui_func = func;
}

void game_t::local_click(void *obj, int x, int y){
    game_t *that = (game_t *)obj;

    if (drop_available(obj, x, y) == DROP_NONE){
        printf("=============================wtf?\n");
        return;
    }

    int is_floater = 0;
    if (that->get_drop_type(x, y) == DROP_FLOATER){
        is_floater = 1;
    }

    that->sendtxt_func(that->sendtxt_obj, "clk %d %d %u %u", x, y, that->my_color, is_floater);
}

void game_t::send_reset(void *obj){
    game_t *that = (game_t *)obj;

    that->sendtxt_func(that->sendtxt_obj, "cls");
}

void game_t::set_gameover_func(void *obj, gameover_func_t func){
    gameover_obj = obj;
    gameover_func = func;
}
