MEG-4 RetroArch Port
====================

This directory contains the main shared library for the [RetroArch](https://www.retroarch.com) platform using the
[libretro](https://www.libretro.com) API.

Compilation options
-------------------

Only depends on the libretro headers.

| Command               | Description                                                |
|-----------------------|------------------------------------------------------------|
| `make`                | Compile the `meg4_libretro.so` plugin for RetroArch        |
| `make libretro`       | Download the latest libretro headers                       |
| `make clean`          | Clean the platform, but do not touch libmeg4               |
| `make distclean`      | Clean everything                                           |
| `DEBUG=1 make`        | Compile with debug information                             |

To start, you'll have to install RetroArch and use

```
retroarch -L meg4_libretro.so
```

This port doesn't use open file modals, floppies are looked for in the `RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY` (if that works).

If you really want to enjoy MEG-4, then **I do not recommend** this port. RetroArch is buggy like hell, inconvenient to use (really
bad, non-user-friendly UI), leaks memory, and libretro lacks dozens of features: has no API to hide or show the mouse cursor, no
fullscreen toggle API, forget copy'n'paste or drag'n'drop, no button release events whatsoever, it's unable to handle keyboard
properly (yeah, the plugin can't say "Hi, I need keyboard events!", try setting `input_auto_game_focus = "2"` in *retroarch.cfg*,
that might help), and in overall, inefficient and very poorly documented API with non-commented tutorials, etc. etc. etc.

In a nutshell, libretro suxx, avoid it whenever you can. Otherwise contributions on libretro workarounds are always welcome!
