c5gui
=============

This is a minimal GUI for the board game c5, written in C++/FLTK. It's about as minimal as possible, with no external GUI components except for the game board itself. It currently uses libcheapsockets for networking, which means the server can run on a shared web host which restrict the user to running a single webserver socket for incoming connections.

Features
----------
The goals of this project are reliability and simplicity, so there are very few features by design. I wanted to make a c5 client which would probably still be working 10 years from now, like the game winmine.exe that is included in Windows. The features which do exist are:

- server retains all state, so clients can leave and join at any time
- simple design and minimal code
- multiplayer support up to 32 players
- binary is statically linked by default

Usage
------
As there is no interface per se, the only configurable option that the user is given is their choice of color. This is controlled by the name of the executable itself. The user renames the executable binary to the color of their choice, with an optional "c5" prefix. Some possible file names include:

- red.exe
- blue.exe
- c5 green.exe
- c5 1a90ff.exe

This also works on Unix architectures with no .exe file extension for executable files.

License
----------
This project is released to the public domain.