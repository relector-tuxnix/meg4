MEG-4 sokol Port
=================

This directory contains the main executable for the [sokol](https://github.com/floooh/sokol) backend.

Compilation options
-------------------

Only static linking supported, as sokol is a header library. On the other hand sokol does have some dependencies of its own,
which are all dynamically linked.

| Command               | Description                                                |
|-----------------------|------------------------------------------------------------|
| `make`                | Compile the `meg4` executable for this platform            |
| `make sokol`          | Download sokol headers for linking                         |
| `make install`        | Install the compiled executable                            |
| `make package`        | Create a package from the compiled executable              |
| `make clean`          | Clean the platform, but do not touch libmeg4               |
| `make distclean`      | Clean everything                                           |
| `DEBUG=1 make`        | Compile with debug information                             |
| `EMBED=1 make`        | Compile without OS modal support, no import / export       |

In embedded mode there are no open file or save file modals, everything is handled in-window. So you must run `meg4` with
the `-d` flag and specify a directory where the floppies are stored. The contents of that directory (and only that one) will
be displayed by the built-in lister, without using any OS-specific modal windows.
