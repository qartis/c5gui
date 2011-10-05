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
CFLAGS=$(WARNINGS) -g -I/Volumes/abf/fltk-1.3.0/ -L/Volumes/abf/fltk-1.3.0/lib -rdynamic -I../../../fltk-1.3.0 -L../../../fltk-solaris/lib -I../../../fltk-solaris -I/usr/local/include/freetype2 -I/usr/local/include -I/usr/openwin/include -fpermissive -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -D_THREAD_SAFE -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -L../../../fltk-solaris/lib -L/usr/openwin/lib -R/usr/openwin/lib -lfltk -lXext -lXft -lfontconfig -lXinerama -lpthread -lm -lX11 -lsocket -lnsl
all: red

red: main.o net.o game.o gui.o
	g++ main.o net.o game.o gui.o -lfltk -lcurl -o red $(CFLAGS)

game.o: game.cpp game.h
	g++ $(CFLAGS) -c game.cpp -o game.o

main.o: main.cpp
	g++ $(CFLAGS) -c main.cpp -o main.o

net.o: net.cpp net.h
	g++ $(CFLAGS) -c net.cpp -o net.o

gui.o: gui.cpp gui.h
	g++ $(CFLAGS) -c gui.cpp -o gui.o

clean:
	rm -f *.o red a.out

mac: main.o game.o net.o gui.o
	g++ -g -rdynamic main.o game.o net.o gui.o -o red -framework cocoa -L/Volumes/abf/fltk-1.3.0/lib -lfltk -lcurl
