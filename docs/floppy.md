MEG-4 Floppy Format
===================

Floppies are 210 pixels wide and 220 pixels tall PNG images, with a special `flPy` chunk in them. For integrity and corruption
detection, the PNG chunk has a CRC value, which is verified on load. The contents of that latter PNG chunk is zlib deflate
compressed. Once uncompressed, it contains a series of further blocks (MEG-4 chunks), each with a common 4 bytes long header:

| Offset | Size  | Description                                            |
|-------:|------:|--------------------------------------------------------|
|      0 |     1 | Magic byte, MEG-4 chunk type                           |
|      1 |     3 | Little endian size of the chunk, including this header |

Single chunks may occur zero or one time, while multiple chunks may occur zero or multiple times. Multiple chunks have an
index byte in them, which must be different for every chunk of the same type. Code that handles these chunks is located in
[src/floppy.h](../src/floppy.h).

META Info
---------

This is a mandatory single chunk and it always must be the very first.

| Offset | Size  | Description                                            |
|-------:|------:|--------------------------------------------------------|
|      0 |     1 | Magic 0, `MEG4_CHUNK_META`                             |
|      1 |     3 | 136                                                    |
|      4 |     3 | The MEG-4 Firmware's version that saved this floppy    |
|      8 |     1 | must be zero                                           |
|      8 |    64 | Zero terminated, UTF-8 program title                   |
|     72 |    64 | Zero terminated, UTF-8 author                          |

Checking this chunk's size is part of the magic, because further info might be added in the future, however that alone won't
change the format. For now, firmware version is useless, but will be useful for backward compatibility if / when the floppy
format changes in the future.

The author is only saved by the MEG-4 PRO, otherwise it's all zeros.

Data
----

This is an optional single chunk.

| Offset | Size  | Description                                            |
|-------:|------:|--------------------------------------------------------|
|      0 |     1 | Magic 1, `MEG4_CHUNK_DATA`                             |
|      1 |     3 | at least 5                                             |
|      4 |     x | bytes copied to free RAM on load (initialized data)    |

Only MEG-4 PRO version saves this chunk, and only needed with bytecode code block (see below). Normally code is stored
as plain text source code which is compiled upon load, so there's no need to store the initialized data redundantly.

Code
----

This is an optional single chunk.

| Offset | Size  | Description                                            |
|-------:|------:|--------------------------------------------------------|
|      0 |     1 | Magic 2, `MEG4_CHUNK_CODE`                             |
|      1 |     3 | at least 5                                             |
|      4 |     x | program's source code                                  |

The MEG-4 PRO version saves a language type byte and compiled bytecode in this chunk when it exports standalone WebAssembly games.

Otherwise it's plain text source code, which always starts with a shebang `#!(language)` line.

Palette
-------

This is an optional single chunk.

| Offset | Size  | Description                                            |
|-------:|------:|--------------------------------------------------------|
|      0 |     1 | Magic 3, `MEG4_CHUNK_PAL`                              |
|      1 |     3 | 1028                                                   |
|      4 |  1024 | RGBA pixels (red channel is the least significant)     |

Sprites
-------

This is an optional single chunk.

| Offset | Size  | Description                                            |
|-------:|------:|--------------------------------------------------------|
|      0 |     1 | Magic 4, `MEG4_CHUNK_SPRITES`                          |
|      1 |     3 | at least 5                                             |
|      4 |     x | RLE compressed palette indexes of a 256 x 256 image    |

RLE packets start with a header byte, which encodes N (= header & 0x7F). If header bit 7 is set, then next byte should be
repeated N+1 times, if not, then the next N+1 bytes are all different indeces. Indeces refer to colors in the palette.

Map
---

This is an optional single chunk.

| Offset | Size  | Description                                            |
|-------:|------:|--------------------------------------------------------|
|      0 |     1 | Magic 5, `MEG4_CHUNK_MAP`                              |
|      1 |     3 | at least 6                                             |
|      4 |     1 | map sprite selector (valid values 0 - 3)               |
|      5 |     x | RLE compressed sprite indexes of a 320 x 200 image     |

RLE packets start with a header byte, same encoding as with sprites. Indeces refer to sprites (map selector * 256 + index).

Font
----

This is an optional single chunk.

| Offset | Size  | Description                                            |
|-------:|------:|--------------------------------------------------------|
|      0 |     1 | Magic 6, `MEG4_CHUNK_FONT`                             |
|      1 |     3 | at least 5                                             |
|      4 |     x | RLE compressed bitmap packets                          |

The font data contains packets, each with a signed byte header. If the header is positive (0 - 127), then (header + 1) * 8
bytes follow, the bitmap data. If it's negative, then no data follows, and -header many codepoints skipped.

Each 8 bytes block is a bitmap, with 1 byte per row (giving the total of 8 x 8 pixels per glyph).

Waveform
--------

This is an optional multiple chunk.

| Offset | Size  | Description                                            |
|-------:|------:|--------------------------------------------------------|
|      0 |     1 | Magic 7, `MEG4_CHUNK_WAVE`                             |
|      1 |     3 | at least 14                                            |
|      4 |     1 | waveform index (valid values 1 - 31)                   |
|      5 |     1 | waveform format (only 0 is valid for now, 8-bit mono)  |
|      6 |     2 | number of samples                                      |
|      8 |     2 | loop start                                             |
|     10 |     2 | loop length                                            |
|     12 |     1 | finetune                                               |
|     13 |     1 | volume                                                 |
|     14 |     x | interleaved waveform data                              |

Note that waveform index 0 is reserved (on music tracks, that means keep using the currently selected waveform).

Sounds
------

This is an optional single chunk.

| Offset | Size  | Description                                            |
|-------:|------:|--------------------------------------------------------|
|      0 |     1 | Magic 8, `MEG4_CHUNK_SFX`                              |
|      1 |     3 | 4 to 260                                               |
|      4 |     x | sound data, 4 bytes for each 64 bank (see below)       |

Music Tracks
------------

This is an optional multiple chunk.

| Offset | Size  | Description                                            |
|-------:|------:|--------------------------------------------------------|
|      0 |     1 | Magic 10, `MEG4_CHUNK_TRACK`                           |
|      1 |     3 | 5 to 16389                                             |
|      4 |     1 | track index (valid values 0 - 7)                       |
|      5 |     x | row data, 4 x 4 bytes each                             |

Both sounds and music tracks encode notes on 4 bytes as

| Offset | Size  | Description                                            |
|-------:|------:|--------------------------------------------------------|
|      0 |     1 | pitch, valid values 0 (C-0) - 96 (B-7)                 |
|      1 |     1 | waveform index, 0 - 31                                 |
|      2 |     1 | effect type, 0 - 255                                   |
|      3 |     1 | effect parameter                                       |

Sounds chunk has one note per row, music tracks have 4 notes per row. Waveform index 0 and some effects are only valid for music
tracks. The full list of effect type codes can be found in the memory map documentation, under section Digital Signal Processor.

Overlays
--------

This is an optional multiple chunk.

| Offset | Size  | Description                                            |
|-------:|------:|--------------------------------------------------------|
|      0 |     1 | Magic 11, `MEG4_CHUNK_OVL`                             |
|      1 |     3 | at least 5                                             |
|      4 |     1 | overlay index (valid values 0 - 255)                   |
|      5 |     x | overlay data                                           |

Overlays are very similar to `MEG4_CHUNK_DATA`, but programs can load them to arbitrary positions dynamically in run-time using
the `memload` function.
