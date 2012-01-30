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

const char *strrstr(const char *haystack, const char *needle)
{
	const char *prev = NULL;
	const char *next;
	if (*needle == '\0')
		return strchr(haystack, '\0');

	while ((next = strstr(haystack, needle)) != NULL) {
		prev = next;
		haystack = next + 1;
	}
	return prev;
}

const char *skippast(const char *haystack, const char *needle)
{
    const char *pos = strrstr(haystack, needle);
    if (pos){
        return pos + strlen(needle);
    } else {
        return haystack;
    }
}

Fl_Color exename_to_color(const char *exename)
{
    exename = skippast(exename, "/");
    exename = skippast(exename, "\\");
    exename = skippast(exename, "c5");

    if (isspace(*exename)) {
        exename++;
    }

    if (startswith(exename, "red"))
        return FL_RED;
    if (startswith(exename, "blue"))
        return FL_BLUE;
    if (startswith(exename, "green"))
        return FL_GREEN;
    if (startswith(exename, "yellow"))
        return FL_YELLOW;
    if (startswith(exename, "purple"))
        return fl_rgb_color(0x7d, 0x26, 0xcd);

    unsigned r;
    unsigned g;
    unsigned b;
    if (sscanf(exename, "%02x%02x%02x", &r, &g, &b) == 3) {
        return fl_rgb_color((unsigned char)r, (unsigned char)g,
                            (unsigned char)b);
    }
    return FL_RED;
}

int main(int argc, char **argv)
{
    Fl_Color my_color = exename_to_color(argv[0]);

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
    gui.undo_func = &game_t::undo;

    net.add_packet_handler(&game, &game_t::parse_clk);
    net.add_packet_handler(&game, &game_t::parse_cls);
    net.add_packet_handler(&game, &game_t::parse_undo);
    net.connect();

    gui.show(argc, argv);
    return Fl::run();
}
