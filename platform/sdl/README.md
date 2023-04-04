MEG-4 SDL Port
==============

This directory contains the main executable for the [SDL](https://libsdl.org) backend.

Compilation options
-------------------

To compile statically, just add a `SDL3` or `SDL2` directory with the SDL's repository.

| Command               | Description                                                |
|-----------------------|------------------------------------------------------------|
| `make`                | Compile the `meg4` executable for this platform            |
| `make static-sdl3`    | Download the latest SDL3 for static linking                |
| `make static-sdl2`    | Download and patch the legacy SDL2 for static linking      |
| `make install`        | Install the compiled executable                            |
| `make package`        | Create a package from the compiled executable              |
| `make deb`            | Create a debian package from the compiled executable       |
| `make clean`          | Clean the platform, but do not touch libmega4              |
| `make distclean`      | Clean everything                                           |
| `DEBUG=1 make`        | Compile with debug information                             |
| `EMBED=1 make`        | Compile without OS modal support, no import / export       |
| `FINGEREVENTS=1 make` | Assume SDL does not simulate finger events as mouse events |
| `USE_EMCC=1 make`     | Compile with emscripten (used by the WebAssembly port)     |

In embedded mode there are no open file or save file modals, everything is handled in-window. So you must run `meg4` with
the `-d` flag and specify a directory where the floppies are stored. The contents of that directory (and only that one) will
be displayed by the built-in lister, without using any OS-specific modal windows.
