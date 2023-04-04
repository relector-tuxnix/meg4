Getting Started
===============

In Your Browser
---------------

If you don't want to install anything, just visit the [website](https://bztsrc.gitlab.io/meg4), it has an emulator running
in your browser.

Installing
----------

Go to the [repository](https://gitlab.com/bztsrc/meg4/tree/binaries) and download the archive for your operating system.

### Windows

1. download [meg4-i686-win-sdl.zip](https://gitlab.com/bztsrc/meg4/raw/binaries/tngp-i686-win-sdl.zip)
2. extract it to `C:\Program Files` directory and enjoy!

This is a portable executable, no actual installation required.

### Linux

1. download [meg4-x86_64-linux-sdl.tgz](https://gitlab.com/bztsrc/meg4/raw/binaries/meg4-x86_64-linux-sdl.tgz)
2. extract it to the `/usr` directory and enjoy!

Alternatively you can download the deb version for [Ubuntu](https://gitlab.com/bztsrc/meg4/raw/binaries/meg4_0.0.1-amd64.deb)
or [RaspiOS](https://gitlab.com/bztsrc/meg4/raw/binaries/meg4_0.0.1-armhf.deb) and install that with
```
sudo dpkg -i meg4_*.deb
```

Running
-------

When you run the MEG-4 emulator, your machine's localization will be autodetected, and if possible, the emulator will greet you
in your own language. The first screen it will show you is the "MEG-4 Floppy Drive" screen.

<imgc ../img/load.png><fig>MEG-4 Floppy Drive</fig>

Just drag'n'drop a floppy file onto the drive, or <mbl> left click on the drive to open the file selector. These floppies are PNG
images with additional data, and an empty one looks like this:

<imgc ../../src/misc/floppy.png><fig>MEG-4 Floppy</fig>

Other formats are supported too, see [file formats] for more details on what else can be imported. Once your floppy is loaded,
the screen will automatically change to the [game screen].

Alternatively you can also press <kbd>Esc</kbd> here to get to the [editor screens] and start creating contents from ground up.

### Command Line Options

On Windows, replace `-` with `/` for the flags (because that's the Windows' way of specifying flags, for example `/n`, `/vv`),
otherwise all options are identical.

Use <mbr> right-click on `meg4.exe`, and from the popup menu select *Create shortcut*. Then <mbr> right-click on the newly created
shortcut file, and from the popup menu choose *Properties*.
<imgc ../img/winshortcut.png><fig>Setting command line options under Windows</fig>
A window will appear, where you can specify the command line options in the *Target* field. Press *OK* to save it. After this,
don't click on the program, rather click on this shortcut, and the program will start with those command line options. If you
wish, you can have multiple shortcuts with different options.

```
meg4 [-L <xx>] [-z] [-n] [-v|-vv] [-d <dir>] [floppy]
```

| Option     | Description |
|------------|-------------|
| `-L <xx>`  | The argument of this flag can be "en", "es", "de", "fr" etc. Using this flag forces a specific language dictionary for the emulator and avoids automatic detection. If there's no such dictionary, then English is used. |
| `-z`       | On Linux by default, the GTK libraries are run-time linked to get the open file modal. Using this flag will make it call `zenity` instead (requires zenity to be installed on your computer). |
| `-n`       | Force using the "nearest" interpolation method. By default, it is only used if the screen size is multiple of 320 x 200. |
| `-v, -vv`  | Enable verbose mode. `meg4` will print out detailed information to the standard output (as well as your script's [trace] calls), so run this from a terminal. |
| `-d <dir>` | Optional, if given, then floppies will be stored in this directory and no open file modal is used. |
| `floppy`   | If this parameter is given, then the floppy (or any other supported format) is automatically loaded on start. |
