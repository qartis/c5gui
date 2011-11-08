#include <stdio.h>
#include <string.h>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_draw.H>
#include <curl/curl.h>
#include <ctype.h>

#include "common.h"
#include "net.h"
#include "game.h"
#include "gui.h"

Fl_Color progname_to_color(const char *progname){
    const char *lastslash = strrchr(progname, '/');
    if (lastslash){
        progname = lastslash + strlen("/");
    }
	
    lastslash = strrchr(progname, '\\');
    if (lastslash){
        progname = lastslash + strlen("\\");
    }

    const char *c5 = strstr(progname, "c5");
    if (c5){
        progname = c5 + strlen("c5");
    }

    if (isspace(progname[0])){
        progname = progname + 1;
    }

    if (startswith(progname, "red"))    return FL_RED;
    if (startswith(progname, "blue"))   return FL_BLUE;
    if (startswith(progname, "green"))  return FL_GREEN;
    if (startswith(progname, "yellow")) return FL_YELLOW;

    unsigned char r;
    unsigned char g;
    unsigned char b;
    if (sscanf(progname, "%02x%02x%02x", &r, &g, &b) == 3){
        return fl_rgb_color(r,g,b);
    }
    return FL_RED;
}

int main(int argc, char **argv) {
    Fl_Color my_color = progname_to_color(argv[0]);

    net_t net;
    game_t game(my_color);
    gui_t gui(my_color, 500, 500, "c5");

    game.net_obj = (void *)&net;
    game.gui_obj = (void *)&gui;
    game.sendtxt_func = &net_t::send;
    game.droppiece_func = &gui_t::drop_piece;
    game.resetgui_func = &gui_t::reset_board;
    game.gameover_func = &gui_t::game_over;

    gui.game_obj = (void *)&game;
    gui.onclick_func = &game_t::local_click;
    gui.canclick_func = &game_t::drop_available;
    gui.resetgame_func = &game_t::send_reset;

    net.add_packet_handler(&game, &game_t::parse_clk);
    net.add_packet_handler(&game, &game_t::parse_cls);
    net.connect();

    gui.show(argc, argv);
    return Fl::run();
}
