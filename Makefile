WARNINGS=    -Wall -Wextra -pedantic  \
             -pedantic-errors -Wextra  -Wall -Waggregate-return -Wcast-align \
             -Wcast-qual  -Wchar-subscripts  -Wcomment -Wconversion \
             -Wdisabled-optimization \
             -Wfloat-equal  -Wformat  -Wformat=2 \
             -Wformat-nonliteral -Wformat-security  \
             -Wformat-y2k \
             -Wimport  -Winit-self  -Winline \
             -Winvalid-pch   \
             -Wlong-long -Wmissing-braces \
             -Wmissing-format-attribute   \
             -Wmissing-noreturn \
             -Wpacked  -Wparentheses  -Wpointer-arith \
             -Wredundant-decls -Wreturn-type \
             -Wshadow -Wsign-compare  \
             -Wstrict-aliasing -Wstrict-aliasing=2 -Wswitch  -Wswitch-default \
             -Wswitch-enum -Wtrigraphs  -Wuninitialized \
             -Wunknown-pragmas  -Wunreachable-code -Wunused \
             -Wunused-function  -Wunused-label  -Wunused-parameter \
             -Wunused-value  -Wunused-variable  \
             -Wwrite-strings
FLAGS=-g -static
CFLAGS=$(WARNINGS) $(FLAGS)
LDFLAGS=$(FLAGS) -lfltk -lcurl
all: red

red: main.o net.o game.o gui.o
	g++ $(LDFLAGS) main.o net.o game.o gui.o -o red

game.o: game.cpp game.h
	g++ $(CFLAGS) -c game.cpp -o game.o

main.o: main.cpp
	g++ $(CFLAGS) -c main.cpp -o main.o

net.o: net.cpp net.h
	g++ $(CFLAGS) -c net.cpp -o net.o

gui.o: gui.cpp gui.h
	g++ $(CFLAGS) -c gui.cpp -o gui.o

clean:
	rm -f *.o red
