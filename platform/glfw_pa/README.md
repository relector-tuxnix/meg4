MEG-4 GLFW Port
===============

This directory contains the main executable for the [GLFW](https://www.glfw.org) and [PortAudio](https://www.portaudio.com) backend.

Compilation options
-------------------

To compile statically, just add `glfw3` or `portaudio` directories with their repositories.

| Command               | Description                                                |
|-----------------------|------------------------------------------------------------|
| `make`                | Compile the `meg4` executable for this platform            |
| `make static-glfw3`   | Download glfw3 for static linking                          |
| `make static-pa`      | Download portaudio for static linking                      |
| `make install`        | Install the compiled executable                            |
| `make package`        | Create a package from the compiled executable              |
| `make clean`          | Clean the platform, but do not touch libmeg4               |
| `make distclean`      | Clean everything                                           |
| `DEBUG=1 make`        | Compile with debug information                             |
| `EMBED=1 make`        | Compile without OS modal support, no import / export       |
| `NOGLES=1 make`       | Compile with old-school OpenGL (no shaders, *MUCH* faster) |
| `JOYFALLBACK=1 make`  | Compile with support for the old glfw joystick API         |

In embedded mode there are no open file or save file modals, everything is handled in-window. So you must run `meg4` with
the `-d` flag and specify a directory where the floppies are stored. The contents of that directory (and only that one) will
be displayed by the built-in lister, without using any OS-specific modal windows.
