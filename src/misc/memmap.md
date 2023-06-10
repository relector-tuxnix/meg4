# Memory Map

## Misc

All values are little endian, so the smaller digit is stored on the smaller address.

| Offset | Size       | Description                                                        |
|--------|-----------:|--------------------------------------------------------------------|
|  00000 |          1 | MEG-4 firmware version major                                       |
|  00001 |          1 | MEG-4 firmware version minor                                       |
|  00002 |          1 | MEG-4 firmware version bugfix                                      |
|  00003 |          1 | performance counter, last frame's unspent time in 1/1000th secs    |
|  00004 |          4 | number of 1/1000th second ticks since power on                     |
|  00008 |          8 | UTC unix timestamp                                                 |
|  00010 |          2 | current locale                                                     |

The performance counter shows the time unspent when the last frame was generated. If this is zero or negative, then it means
how much your loop() function has overstepped its available timeframe.

## Pointer

| Offset | Size       | Description                                                        |
|--------|-----------:|--------------------------------------------------------------------|
|  00012 |          2 | pointer buttons state (see [getbtn] and [getclk])                  |
|  00014 |          2 | pointer sprite index                                               |
|  00016 |          2 | pointer X coordinate                                               |
|  00018 |          2 | pointer Y coordinate                                               |

The pointer buttons are as follows:

| Define  | Bitmask   | Description                                                        |
|---------|----------:|--------------------------------------------------------------------|
| `BTN_L` |         1 | Left mouse button                                                  |
| `BTN_M` |         2 | Middle mouse button                                                |
| `BTN_R` |         4 | Right mouse button                                                 |
| `SCR_U` |         8 | Scroll up                                                          |
| `SCR_D` |        16 | Scroll down                                                        |
| `SCR_L` |        32 | Scroll left                                                        |
| `SCR_R` |        64 | Scroll right                                                       |

The upper bits of the pointer sprite index are used for hotspots: bit 13-15 hotspot Y, bit 10-12 hotspot X, bit 0-9 sprite.
There are some predefined built-in cursors:

| Define       | Value     | Description                                                   |
|--------------|----------:|---------------------------------------------------------------|
| `PTR_NORM`   |      03fb | Normal (arrow) pointer                                        |
| `PTR_TEXT`   |      03fc | Text pointer                                                  |
| `PTR_HAND`   |      0bfd | Link pointer                                                  |
| `PTR_ERR`    |      93fe | Error pointer                                                 |
| `PTR_NONE`   |      ffff | The pointer is hidden                                         |

## Keyboard

| Offset | Size       | Description                                                        |
|--------|-----------:|--------------------------------------------------------------------|
|  0001A |          1 | keyboard queue tail                                                |
|  0001B |          1 | keyboard queue head                                                |
|  0001C |         64 | keyboard queue, 16 elements, each 4 bytes (see [popkey])           |
|  0005C |         18 | keyboard keys pressed state by scancodes (see [getkey])            |

The keys popped from the queue are represented in UTF-8. Some invalid UTF-8 sequences represent special (non-printable)
keys, for example:

| Keycode | Description                                     |
|---------|-------------------------------------------------|
| `\x8`   | The character 8, <kbd>Backspace</kbd> key       |
| `\x9`   | The character 9, <kbd>Tab</kbd> key             |
| `\n`    | The character 10, <kbd>⏎Enter</kbd> key         |
| `\x1b`  | The character 27, <kbd>Esc</kbd> key            |
| `Del`   | The <kbd>Del</kbd> key                          |
| `Up`    | The cursor arrow <kbd>▴</kbd> key               |
| `Down`  | The cursor arrow <kbd>▾</kbd> key               |
| `Left`  | The cursor arrow <kbd>◂</kbd> key               |
| `Rght`  | The cursor arrow <kbd>▸</kbd> key               |
| `Cut`   | The Cut key (or <kbd>Ctrl</kbd>+<kbd>X</kbd>)   |
| `Cpy`   | The Copy key (or <kbd>Ctrl</kbd>+<kbd>C</kbd>)  |
| `Pst`   | The Paste key (or <kbd>Ctrl</kbd>+<kbd>V</kbd>) |

The scancodes are as follows:

| ScanCode | Address | Bitmask | Define            |
|---------:|---------|--------:|-------------------|
@SCANCODES@

## Gamepad

| Offset | Size       | Description                                                        |
|--------|-----------:|--------------------------------------------------------------------|
|  0006E |          2 | gamepad joystick treshold (defaults to 8000)                       |
|  00070 |          8 | primary gamepad - keyboard scancode mappings (see [keyboard])      |
|  00078 |          4 | 4 gamepads button pressed state (see [getpad])                     |

The gamepad buttons are as follows:

| Define  | Bitmask   | Description                                                        |
|---------|----------:|--------------------------------------------------------------------|
| `BTN_L` |         1 | The `◁` button or joystick left                                   |
| `BTN_U` |         2 | The `△` button or joystick up                                     |
| `BTN_R` |         4 | The `▷` button or joystick right                                  |
| `BTN_D` |         8 | The `▽` button or joystick down                                   |
| `BTN_A` |        16 | The `Ⓐ` button                                                     |
| `BTN_B` |        32 | The `Ⓑ` button                                                     |
| `BTN_X` |        64 | The `Ⓧ` button                                                     |
| `BTN_Y` |       128 | The `Ⓨ` button                                                     |

The `△△▽▽◁▷◁▷ⒷⒶ` sequence makes the `KEY_CHEAT` "key" pressed.

## Graphics Processing Unit

| Offset | Size       | Description                                                        |
|--------|-----------:|--------------------------------------------------------------------|
|  0007E |          1 | UNICODE code point upper bits for font glyph mapping               |
|  0007F |          1 | sprite bank selector for the map                                   |
|  00080 |       1024 | palette, 256 colors, each entry 4 bytes, RGBA                      |
|  00480 |          2 | x0, crop area X start in pixels (for all drawing functions)        |
|  00482 |          2 | x1, crop area X end in pixels                                      |
|  00484 |          2 | y0, crop area Y start in pixels                                    |
|  00486 |          2 | y1, crop area Y end in pixels                                      |
|  00488 |          2 | displayed vram X offset in pixels or 0xffff                        |
|  0048A |          2 | displayed vram Y offset in pixels or 0xffff                        |
|  0048C |          1 | turtle pen down flag (see [up], [down])                            |
|  0048D |          1 | turtle pen color, palette index 0 to 255 (see [color])             |
|  0048E |          2 | turtle direction in degrees, 0 to 359 (see [left], [right])        |
|  00490 |          2 | turtle X offset in pixels (see [move])                             |
|  00492 |          2 | turtle Y offset in pixels                                          |
|  00494 |          2 | maze walking speed in 1/128 tiles (see [maze])                     |
|  00496 |          2 | maze rotating speed in degrees (1 to 90)                           |
|  00498 |          1 | console foreground color, palette index 0 to 255 (see [printf])    |
|  00499 |          1 | console background color, palette index 0 to 255                   |
|  0049A |          2 | console X offset in pixels                                         |
|  0049C |          2 | console Y offset in pixels                                         |
|  0049E |          2 | camera X offset in [3D space] (see [tri3d], [tritx], [mesh])       |
|  004A0 |          2 | camera Y offset                                                    |
|  004A2 |          2 | camera Z offset                                                    |
|  004A4 |          2 | camera direction, pitch (0 up, 90 forward)                         |
|  004A6 |          2 | camera direction, yaw (0 left, 90 forward)                         |
|  004A8 |          1 | camera field of view in angles (45, negative gives orthographic)   |
|  004AA |          2 | light source position X offset (see [tri3d], [tritx], [mesh])      |
|  004AC |          2 | light source position Y offset                                     |
|  004AE |          2 | light source position Z offset                                     |
|  00600 |      64000 | map, 320 x 200 sprite indeces (see [map] and [maze])               |
|  10000 |      65536 | sprites, 256 x 256 palette indeces, 1024 8 x 8 pixels (see [spr])  |
|  28000 |       2048 | window for 4096 font glyphs (see 0007E, [width] and [text])        |

## Digital Signal Processor

| Offset | Size       | Description                                                        |
|--------|-----------:|--------------------------------------------------------------------|
|  0007C |          1 | waveform bank selector (1 to 31)                                   |
|  0007D |          1 | music track bank selector (0 to 7)                                 |
|  004BA |          1 | current tempo (in ticks per row, read-only)                        |
|  004BB |          1 | current track being played (read-only)                             |
|  004BC |          2 | current row being played (read-only)                               |
|  004BE |          2 | number of total rows in current track (read-only)                  |
|  004C0 |         64 | 16 channel status registers, each 4 bytes (read-only)              |
|  00500 |        256 | 64 sound effects, each 4 bytes                                     |
|  20000 |      16384 | window for waveform samples (see 0007C)                            |
|  24000 |      16384 | window for music patterns (see 0007D)                              |

The DSP status registers are all read-only, and for each channel they look like:

| Offset | Size       | Description                                                        |
|--------|-----------:|--------------------------------------------------------------------|
|      0 |          2 | current position in the waveform being played                      |
|      2 |          1 | current waveform (1 to 31, 0 if the channel is silent)             |
|      3 |          1 | current volume (0 means channel is turned off)                     |

The first 4 channels are for the music, the rest for the sound effects.

Note that the waveform index 0 means "use the previous waveform", so index 0 cannot be used in the selector.
The format of every other waveform:

| Offset | Size       | Description                                                        |
|--------|-----------:|--------------------------------------------------------------------|
|      0 |          2 | number of samples                                                  |
|      2 |          2 | loop start                                                         |
|      4 |          2 | loop length                                                        |
|      6 |          1 | finetune value, -8 to 7                                            |
|      7 |          1 | volume, 0 to 64                                                    |
|      8 |      16376 | signed 8-bit mono samples                                          |

The format of the sound effects and the music tracks are the same, the only difference is, music tracks have 4 notes per row,
one for each channel, and there are 1024 rows; while for sound effects there's only one note and 64 rows.

| Offset | Size       | Description                                                        |
|--------|-----------:|--------------------------------------------------------------------|
|      0 |          1 | note number, see `NOTE_x` defines, 0 to 96                         |
|      1 |          1 | waveform index, 0 to 31                                            |
|      2 |          1 | effect type, 0 to 255                                              |
|      3 |          1 | effect parameter                                                   |

The counting of notes goes as follows: 0 means no note set. Followed by 8 octaves, each with 12 notes, so 1 equals to C-0,
12 is B-0 (on the lowest octave), 13 is C-1 (one octave higher) and 14 is C#1 (C sharp, semitone higher). For example the
D note on the 4th octave would be 1 + 4\*12 + 2 = 51. The B-7 is 96, the highest note on the highest octave. You also have
built-in defines, for example C-1 is `NOTE_C_1` and C#1 is `NOTE_Cs1`, if you don't want to count then you can use these as well
in your program.

For simplicity, MEG-4 uses the same codes as the Amiga MOD file (this way you'll see the same in the built-in editor as well
as in a third party music tracker), but it does not support all of them. As said earlier, these codes are represented by three
hex numbers, the first being the type `t`, and the last two the parameter, `xy` (or `xx`). The types `E1` to `ED` are all stored
in the type byte, although it looks like one of their nibble might belong to the parameter, but it's not.

| Effect | Code | Description                                                |
|--------|------|------------------------------------------------------------|
| ...    | 000  | No effect                                                  |
| Arp    | 0xy  | Arpeggio, play note, note+x semitone and note+y semitone   |
| Po/    | 1xx  | Portamento up, slide period by x up                        |
| Po\\   | 2xx  | Portamento down, slide period by x up                      |
| Ptn    | 3xx  | Tone portamento, slide period to period x                  |
| Vib    | 4xy  | Vibrato, oscillate the pitch by y semitones at x freq      |
| Ctv    | 5xy  | Continue Tone portamento + volume slide by x up or y down  |
| Cvv    | 6xy  | Continue Vibrato + volume slide by x up or by y down       |
| Trm    | 7xy  | Tremolo, oscillate the volume by y amplitude at x freq     |
| Ofs    | 9xx  | Set sample offset to x \* 256                              |
| Vls    | Axy  | Slide volume by x up or by y down                          |
| Jmp    | Bxx  | Position jump, set row to x \* 64                          |
| Vol    | Cxx  | Set volume to x                                            |
| Fp/    | E1x  | Fine portamento up, increase period by x                   |
| Fp\\   | E2x  | Fine portamento down, decrease period by x                 |
| Svw    | E4x  | Set vibrato waveform, 0 sine, 1 saw, 2 square, 3 noise     |
| Ftn    | E5x  | Set finetune, change tuning by x (-8 to 7)                 |
| Stw    | E7x  | Set tremolo waveform, 0 sine, 1 saw, 2 square, 3 noise     |
| Rtg    | E9x  | Retrigger note, trigger current sample every x ticks       |
| Fv/    | EAx  | Fine volume slide up, increase by x                        |
| Fv\\   | EBx  | Fine volume slide down, decrease by x                      |
| Cut    | ECx  | Cut note in x ticks                                        |
| Dly    | EDx  | Delay note in x ticks                                      |
| Tpr    | Fxx  | Set number of ticks per row to x (tick defalts to 6)       |

## User Memory

Memory addresses from 00000 to 2FFFF belong to the MMIO, everything above (starting from 30000 or `MEM_USER`) is freely usable
user memory.

| Offset | Size       | Description                                                        |
|--------|-----------:|--------------------------------------------------------------------|
|  30000 |          4 | (BASIC only) offset of DATA                                        |
|  30004 |          4 | (BASIC only) current READ counter                                  |
|  30008 |          4 | (BASIC only) maximum READ, number of DATA                          |

This is followed by the global variables that you have declared in your program, followed by the constants, like string literals.
In case of BASIC, then this is followed by the actual DATA records.

Memory addresses above the initialized data can be dynamically allocated and freed in your program via the [malloc] and [free] calls.

Lastly the stack, which is at the top (starting from C0000 or `MEM_LIMIT`) and growing *downwards*. Your program's local variables
(that you declared inside a function) go here. The size of the stack always changes depending on which function calls which other
function in your program.

If by any chance the top of the dynamically allocated data and the bottom of the stack would overlap, then MEG-4 throws an
"Out of memory" error.

## Format String

Some functions, [printf], [sprintf] and [trace] use a format string that may contain special characters to reference arguments
and to describe how to display them. These are:

| Code | Description                                                |
|------|------------------------------------------------------------|
| `%%` | The `%` character                                          |
| `%d` | Print the next parameter as a decimal number               |
| `%u` | Print the next parameter as an unsigned decimal number     |
| `%x` | Print the next parameter as a hexadecimal number           |
| `%o` | Print the next parameter as an octal number                |
| `%b` | Print the next parameter as a binary number                |
| `%f` | Print the next parameter as a floating point number        |
| `%s` | Print the next parameter as a string                       |
| `%c` | Print the next parameter as an UTF-8 character             |
| `%p` | Print the next parameter as an address (pointer)           |
| `\t` | Tab, fix vertical position before continue printing        |
| `\n` | Start a new line for printing                              |

You can add padding by specifying the length between `%` and the code. If that starts with `0`, then value will be padded
with zeros, otherwise with spaces. For example `%4d` will pad the value to the right with spaces, and `%04x` with zeros.
The `f` accepts a number after a dot, which tells the number of digits in the fractional part (up to 8), eg. `%.6f`.

## 3D Space

In MEG-4, the 3 dimensional space is handled according to the right-hand rule: +X is on the right, +Y is up, and +Z is towards the
viewer.

```
  +Y
   |
   |__ +X
  /
+Z
```

Each point must be placed in the range -32767 to +32767. How this 3D world is projected to your 2D screen depends on how you
configure the camera (see [Graphics Processing Unit] address 0049E). Of course, you have to place the camera in the world, with
X, Y, Z coordinates. Then you have to tell where the camera is looking at, using pitch and yaw. Finally you also have to tell what
kind of lens the camera has, by specifying the field of view angle. That latter normally should be between 30 (very narrow) and
180 degrees (like fish and birds). MEG-4 supports up to 127 degrees, but there's a trick. Positive FOV values will be projected as
perspective (the farther the object is, the smaller it is), but negative values also handled, just with orthographic projection
(no matter the distance, the object's size will be the same). Perspective is used in FPS games, while the orthographic projection
is mostly preferred by strategy games.

You can display a set of triangles (a complete 3D model) using the [mesh] function efficiently. Because models probably have local
coordinates, that would draw all models one on top of another around the origo. So if you want to dispay multiple models in the
world, first you should translate them (place them) into world coordinates using [trns], and then use the translated vertex cloud
with [mesh] (moving and rotating the model around won't change the triangles, just their vertex coordinates).
