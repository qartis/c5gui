#include <stdio.h>
#include <string.h>
#include <FL/Enumerations.H>
#include "util.h"

int startswith(const char *a, const char *b){
    return strncmp(a, b, strlen(b)) == 0;
}

Fl_Color exe_to_color(char *exename){
    char *lastslash = strrchr(exename, '/');
    if (lastslash){
        exename = lastslash + 1;
    }
    if (startswith(exename, "red"))    return FL_RED;
    if (startswith(exename, "blue"))   return FL_BLUE;
    if (startswith(exename, "green"))  return FL_GREEN;
    if (startswith(exename, "yellow")) return FL_YELLOW;

    unsigned char r, g, b;
    if (sscanf(exename, "%02hhx%02hhx%02hhx", &r, &g, &b) == 3){
        return fl_rgb_color(r,g,b);
    }
    return FL_RED;
}
