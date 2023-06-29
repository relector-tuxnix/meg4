MEG-4 raylib Port
=================

This directory contains the main executable for the [raylib](https://www.raylib.com/) backend.

Compilation options
-------------------

To compile statically, just add `raylib` directory with the repository.

| Command               | Description                                                |
|-----------------------|------------------------------------------------------------|
| `make`                | Compile the `meg4` executable for this platform            |
| `make static-raylib`  | Download raylib for static linking                         |
| `make install`        | Install the compiled executable                            |
| `make package`        | Create a package from the compiled executable              |
| `make clean`          | Clean the platform, but do not touch libmeg4               |
| `make distclean`      | Clean everything                                           |
| `DEBUG=1 make`        | Compile with debug information                             |
| `EMBED=1 make`        | Compile without OS modal support, no import / export       |

In embedded mode there are no open file or save file modals, everything is handled in-window. So you must run `meg4` with
the `-d` flag and specify a directory where the floppies are stored. The contents of that directory (and only that one) will
be displayed by the built-in lister, without using any OS-specific modal windows.
