enum drop_type {
    DROP_NONE,
    DROP_LEFT,
    DROP_RIGHT,
    DROP_TOP,
    DROP_BOTTOM,
    DROP_FLOATER
};

#define BOARD_EMPTY FL_WHITE
#define BOARD_STONE FL_BLACK

#define BOARD_DIM 25

inline int startswith(const char *a, const char *b)
{
    return strncmp(a, b, strlen(b)) == 0;
}

inline const char *colorname(Fl_Color color)
{
    if (color == FL_RED) {
        return "red";
    } else if (color == FL_GREEN) {
        return "green";
    } else if (color == FL_YELLOW) {
        return "yellow";
    } else if (color == FL_BLUE) {
        return "blue";
    } else if (color == BOARD_EMPTY) {
        return "blank";
    } else if (color == BOARD_STONE) {
        return "stone";
    } else {
        return "unknown_color";
    }
}

#ifdef WIN32
#define __attribute__(x)
#endif
