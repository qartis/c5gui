#define _GNU_SOURCE

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

    unsigned rgb;
    if (sscanf(progname, "%06x", &rgb)){
        unsigned char r = (rgb >>  0) & 0xFF;
        unsigned char g = (rgb >>  8) & 0xFF;
        unsigned char b = (rgb >> 16) & 0xFF;
        return fl_rgb_color(r,g,b);
    }
    return FL_RED;
}

int main(int argc, char **argv) {
    Fl_Color my_color = progname_to_color(argv[0]);

    net_t net;
    game_t game(my_color);
    gui_t gui(my_color, 500, 500, "c5");

    game.set_sendtxt_func(&net, &net_t::send);
    game.set_droppiece_func(&gui, &gui_t::drop_piece);
    game.set_resetgui_func(&gui, &gui_t::reset_board);
    game.set_gameover_func(&gui, &gui_t::game_over);

    gui.set_onclick_func(&game, &game_t::local_click);
    gui.set_canclick_func(&game, &game_t::drop_available);
    gui.set_resetgame_func(&game, &game_t::send_reset);

    net.add_packet_handler(&game, &game_t::parse_clk);
    net.add_packet_handler(&game, &game_t::parse_cls);
    net.connect();

    gui.show(argc, argv);
    return Fl::run();
}
