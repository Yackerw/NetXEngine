# NetXEngine

A version of NXEngine designed to add online multiplayer. Based off of the [NXEngine-Evo refactor.](https://github.com/nxengine/nxengine-evo)  
Currently only for Windows, XP and up.

## Features

* The ability to host, or join, online games
* Various skins you can use on your character online
* Support for Cave Story+ graphics
* Support for Cave Story Switch music

## Compiling
NetXEngine can be compiled on Visual Studio 2017 and up (or according to base nxengine-evo, 2010 and up),  
MinGW-w64 (tested with TDM-GCC version 10.3.0), and MinGW32.

### Visual Studio
Follow the instructions on [nxengine-evo's wiki.](https://github.com/nxengine/nxengine-evo/wiki/Building-on-Windows)

### TDM-GCC / MinGW-w64
Enter the base directory in command line, then run `mingw32-make --file=Makefile.mgw` to build. Add DEBUG=1 to create a debug build for use with gdb.
### MinGW32
Follow the instructions for w64, but add USE_MINGW32=1 onto the mingw32-make command.  
* Builds using 32 will lack debug logging.