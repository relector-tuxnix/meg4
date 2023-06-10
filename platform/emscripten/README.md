MEG-4 Emscripten Port
=====================

This directory contains the main executable for the WebAssembly port, using the [SDL2](https://libsdl.org) backend.

Compilation options
-------------------

| Command               | Description                                                |
|-----------------------|------------------------------------------------------------|
| `make`                | Compile the `meg4` executable for this platform            |
| `make package`        | Create a package from the compiled executable              |
| `make clean`          | Clean the platform, but do not touch libmeg4               |
| `make distclean`      | Clean everything                                           |
| `DEBUG=1 make`        | Compile with debug information                             |
| `NOEDITORS=1 make`    | Compile a bare emulator without the built-in editors       |

To embed the emulator on a website, all you need is in the HTML:

```html
<canvas id="canvas"></canvas>
<script src="meg4.js"></script>
```

Yeah, really that's it. It will autodetect the browser's language, and will instantiate the wasm binary, no other steps
required on your side. You can place the canvas however you prefer, but for the best looking results I suggest to make its
size multiple of 320 x 200.

To embed your game on a website, you'll need MEG-4 PRO, which exports your entire project as a standalone wasm file, without
the editors, and also autostarts your program.
