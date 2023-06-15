MEG-4 libmeg4.a API
==================

This is the API of the `libmeg4.a` library, needed to know when you're porting it to new platforms. For the API used by your
MEG-4 programs, see the [MEG-4 User's Manual](https://bztsrc.gitlab.io/meg4/manual_en.html).

There are two kinds of functions. The first group has to be implemented in the platform application, and called from libmeg4.a
as hooks. These always start with `main_`. The second group are functions provided by libmeg4.a, always starting with `meg4_`.
Hereafter these will be referenced from the platform application's perspective, eg. exported functions are the ones that the
platform application provides, and imported ones are the functions provided by the libmeg4.a library.

NOTE: Most of the `main_` functions are OS dependent, and already implemented in `platform/common.h`.

Exported Functions
------------------

```c
void main_log(int lvl, const char* fmt, ...);
```

A logger function.

```c
uint8_t* main_readfile(char *file, int *size);
```

Should read in and return the `file`'s contents in a malloc'd buffer. If `file` not found, should return `NULL`. Filename is
always UTF-8.

```c
int  main_writefile(char *file, uint8_t *buf, int size);
```

Writes a `buf` buffer of `size` bytes to `file`. Filename is always UTF-8.

```c
void main_openfile(void);
```

Should open the "Open File Modal" native on the platform. When the user selects a file, it should call `meg4_insert`.

```c
int  main_savefile(const char *name, uint8_t *buf, int len);
```

If floppy directory given on the command line, then it should save the file there. Otherwise it should open the "Save File Modal"
native on the platform, and when user selects a file, it should save `buf` buffer of `len` bytes into it, and return the specified
filename in `name`. Returns 1 on success, 0 on error.

```c
char **main_getfloppies(void);
```

If floppy directory given on the command line, then it should return a `NULL` terminated list with UTF-8 absoulte paths of floppy
images in that directory, otherwise `NULL`. (Note the difference: `NULL` returned means floppy directory not set, but a pointer
returned which is pointing to a `NULL` means directory set but no floppies in it).

```c
uint8_t *main_cfgload(char *cfg, int *len);
```

Should load a configuration binary blob by the key `cfg`, and return its contents in a malloc'd buffer. If configuration with
the given key not found, returns `NULL`.

```c
int  main_cfgsave(char *cfg, uint8_t *buf, int len);
```

Should save a configuration `buf` of `len` bytes under the key `cfg`. Returns 1 on success, 0 on error.

```c
char *main_getclipboard(void);
```

Should return the OS native clipboard's zero terminated, UTF-8 text in a malloc'd buffer, or `NULL` if the clipboard is empty.

```c
void main_setclipboard(char *str);
```

Should set the OS native clipboard's text to the given zero terminated, UTF-8 string in `str`.

```c
void main_osk_show(void);
```

This is optional, and only needed for platforms available on handheld devices. It should display the OS native On-Screen Keyboard.

```c
void main_osk_hide(void);
```

This is optional, and only needed for platforms available on handheld devices. It should hide the OS native On-Screen Keyboard.

```c
void main_fullscreen(void);
```

This function should toggle between fullscreen and windowed modes.

```c
void main_focus(void);
```

This should raise the window and get focus for it.

Imported Functions
------------------

```c
void meg4_poweron(char *region);
```

Turns the MEG-4 emulator on. The `region` parameter is a two letter ISO-639-1 language code, it selects the emulator's preferred
language and fallbacks to English if that not found.

```c
void meg4_poweroff(void);
```

Turns the MEG-4 emulator off, frees all internal resources.

```c
void meg4_reset(void);
```

Resets the MEG-4 emulator to its default state.

```c
void meg4_insert(char *name, uint8_t *buf, int len);
```

Insert a floppy disk (or a file in any other supported formats) into the MEG-4 Floppy Drive.

```c
int meg4_load(uint8_t *buf, int len);
```

Parses a MEG-4 Floppy image. Do not use this function unless you know what you're doing. Use `meg4_insert` instead.

```c
uint8_t *meg4_save(int *len);
```

Saves a MEG-4 Floppy image and returns it in a malloc'd buffer.

```c
void meg4_run(void);
```

Runs the emulator. Call this periodically, at 60 FPS.

```c
void meg4_clrpad(int pad, int btn);
```

Clear a gamepad button's pressed state. MEG-4 supports 4 gamepads and 8 buttons on each.

```c
void meg4_setpad(int pad, int btn);
```

Set a gamepad button's pressed state.

```c
void meg4_setptr(int x, int y);
```

Set mouse pointer coordinates in pixels.

```c
void meg4_setscr(int u, int d, int l, int r);
```

Set mouse scroll button's pressed state.

```c
void meg4_clrbtn(int btn);
```

Clear a mouse button's pressed state.

```c
void meg4_setbtn(int btn);
```

Set a mouse button's pressed state.

```c
void meg4_clrkey(int sc);
```

Clear a keyboard key's pressed state (indexed by MEG-4 scancode).

```c
void meg4_setkey(int sc);
```

Set a keyboard key's pressed state.

```c
void meg4_pushkey(char *key);
```

Push an UTF-8 representation of a keyboard key into the keyboard queue. Call this when a keyboard pressed state flag changes
from clear to set. NOTE: special keys (like <kbd>Alt</kbd>+<kbd>I</kbd> or <kbd>Ctrl</kbd>+<kbd>C</kbd> are pushed automatically,
you don't have to worry about these. Your platform probably returns normal keys as UTF-8, just push those as-is, which leaves only
a few keys like <kbd>F1</kbd>-<kbd>F12</kbd>, <kbd>Backspace</kbd>, <kbd>Enter</kbd> etc. to be handled manually.)

```c
void meg4_audiofeed(int8_t *buf, int len);
```

Call when your audio playback needs to fill the audio buffer with data.

```c
void meg4_redraw(uint32_t *dst, int dw, int dh, int dp);
```

Update your platform's framebuffer with the MEG-4 screen.

Platform Main Loop
------------------

A typical platform application's main loop looks like this:
```c
    meg4_poweron();
    while(platform_running) {
        loopstart = platform_current_time();
        /* parse events, call meg4_setpad, meg4_setbtn, meg4_pushkey, etc. accordingly */
        while(platform_hasevents) { ... }
        /* run the emulator */
        meg4_run();
        /* update your 32 bit RGBA framebuffer with the emulator's screen */
        meg4_redraw(myfb, 640, 400, 2560);
        /* render your framebuffer using your platform's render function */
        platform_render(myfb, 0, 0, meg4.screen.w, meg4.screen.h);
        /* wait a bit so that this loop runs at 60 FPS */
        platform_msec_delay((1000/60) - (platform_current_time() - loopstart));
    }
    meg4_poweroff();
```

That's about it. For actual examples, you can take a look at the sources under the `platform` directory, or see `tests/runner`
for an absoultely bare minimum platform that does not even have an interface.
