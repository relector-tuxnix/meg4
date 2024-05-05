MEG-4 Linux FrameBuffer + ALSA Port
===================================

This directory contains the main executable for a backend that does not need any library, as it uses the Linux kernel
interfaces directly. This allows this port to be used as a [MEG-4 Operating System](../../docs/MEG-4_OS.md).

Compilation options
-------------------

| Command               | Description                                                |
|-----------------------|------------------------------------------------------------|
| `make`                | Compile the `meg4` executable for this platform            |
| `make install`        | Install the compiled executable                            |
| `make package`        | Create a package from the compiled executable              |
| `make clean`          | Clean the platform, but do not touch libmeg4               |
| `make distclean`      | Clean everything                                           |
| `USE_INIT=1 make`     | Compile code to make it suitable for `/sbin/init`          |
| `FLOPPYDEV=xx make`   | Select the floppy device (default `/dev/sda1`, init only)  |
| `LANG=xx make`        | Select interface's language (default `en`, init only)      |
| `KBDMAP=xx make`      | Select keyboard layout map (default `us`)                  |
| `DEBUG=1 make`        | Compile with debug information                             |

You must run this `meg4` with the `-d` flag and specify a directory where the floppies are stored. With `USE_INIT=1`,
there's no command line, so this has to be hardcoded, use the `FLOPPYDEV` environment variable to change the default.
This is not an actual floppy device file (like /dev/fd0), rather it should point to a partition device with a file system that the
Linux kernel supports, and with a directory named `MEG-4` on that file system where the floppy images are kept. Because for init
there are no environment variables either, the interface's language (specified in the `LANG` environment variable) is also taken
and hardcoded into the binary at compilation time.

The Linux kernel's event interface does not and cannot support switchable X11 keyboard layouts, so you must compile in
a keyboard mapping manually. You can choose any of the existing header files, for example `KBDMAP=gb make`. To create a
new layout mapping, just copy us.h and edit it. You only have to fill in keys for which you want an UTF-8 character
to be produced, special keys like <kbd>Esc</kbd>, <kbd>Enter</kbd>, <kbd>F1</kbd> etc. are handled regardless. If not
specified, defaults to US / International keyboard mapping.

No matter what layout you choose, you can always switch to US / International layout by pressing <kbd>Shift</kbd>+<kbd>Alt</kbd>
in run-time (that layout has Latin letters and all the symbols necessary for programming in the C and BASIC languages).

To sum it up: `USE_INIT=1 KBDMAP=hu LANG=hu FLOPPYDEV=/dev/sda1 make`

Then copy the `meg4` binary to the root fs as `init` and get it loaded by a Linux kernel as the first and only user
space process.
