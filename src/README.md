libmeg4.a
=========

The source is structured like a real motherboard. Each source file corresponds to a specific chip or peripheral, except
"ROM content" is splitted into several files for maintainability. API functions are typically in the same file as the
peripheral for which they provide access, and they are collected into api.h by bin2h.c during compilation.

- bin2h.c - is a tool that generates api.h, data.c and data.h (which is analogous to read-only ROM data).

- comp.c - common compiler functions (part of ROM code).

- comp_asm.c, comp_bas.c, comp_c.c, comp_lua.c - language specific parts of the compiler (ROM code).

- cons.c - console peripheral: high level user input / output API.

- cpu.c - central processing unit: the bytecode VM.

- dsp.c - digital signal processor: sound effects and music playback.

- floppy.h - low level serialization and deserialization routines (the floppy's "file system" in a manner of speaking, ROM code).

- gpu.c - graphics processing unit: displaying sprites, drawing primitives, things like that.

- inp.c - user input peripherals: low level handling of gamepads, mouse, keyboard.

- lang.h - language dictionaries (see also [lang/](lang) directory, part of read-only ROM data).

- math.c - mathematical functions (ROM code).

- meg4.c - public interface of libmeg4.a, like meg4_poweron() and meg4_poweroff().

- mem.c - memory management unit: functions to access MMIO, overlay API, memcpy, strcmp, things like that (ROM code).

- editors/ - optional built-in applications (rest of ROM code, everything except the compiler and the API).
