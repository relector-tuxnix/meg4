File Formats
============

Although the built-in editors are pretty awesome, MEG-4 is capable of handling various file formats both on export and import
side to ease creation of content, and make the use of MEG-4 actual fun.

Floppies
--------

MEG-4 stores programs in "floppies". These are image files that look like a real floppy disk. You can save in this format by
pressing <kbd>Ctrl</kbd>+<kbd>S</kbd>, or by selecting <imgt ../img/menu.png> > `Save` from the menu (see [interface]). You'll be
prompted to give a label to the floppy, which will be your program's title as well. To load such floppies, press
<kbd>Ctrl</kbd>+<kbd>L</kbd>, or just simply drag'n'drop these image files onto the MEG-4 Floppy Drive.

The low level specification for this format can be found [here](https://gitlab.com/bztsrc/meg4/blob/main/docs/floppy.md).

Project Format
--------------

For convenience, there's also a project format, which is a zip archive containing the console's data in only common and well-known
formats so that you can use your favourite editor or tool to modify them. You can save in this format by selecting the
<imgt ../img/menu.png> > `Export ZIP` menu option. To load, you can simply drag'n'drop such zip files into the MEG-4 Floppy Drive.

HINT: One of the test tools, the [converter](https://gitlab.com/bztsrc/meg4/blob/main/tests/converter) can be used to convert
floppies into zip archives.

Files in the archive:

### metainfo.txt

A plain text file with the MEG-4 firmware version and the program's title.

### program.X

Your program's source code, created in the [code editor], as a plain text file. You can use any text editor to modify. On export,
newlines are replaced with CRLF for Windows compatibility, on import it doesn't matter if lines are ended with NL or with CRLF,
both supported.

The source code must start with a special first line, a `#!` shebang followed by the programming language used. For example
`#!c` or `#!lua`. This language code must also match the extension in the file name, eg: *program.c* or *program.lua*.

### sprites.png

A 256 x 256 pixels indexed PNG file, containing the 256 colors palette and all of the 1024 sprites, each 8 x 8 pixels, created
in the [sprite editor]. This image file is editable by [GrafX2](http://grafx2.chez.com), [GIMP](https://www.gimp.org), Paint etc.
On import, true color images will be converted to the default MEG-4 palette using the smallest weighted sRGB distance method. This
works, but not looking particularly good, therefore it is recommended to save and import paletted PNG images instead. Sprites can
also be imported from [Truevision TARGA](http://www.gamers.org/dEngine/quake3/TGA.txt) (.tga) images, if they are indexed and have
the correct dimensions of 256 x 256 pixels.

### map.tmx

The map, created in the [map editor], in a format that can be used with the [Tiled MapEditor](https://www.mapeditor.org). Only
CSV encoded *.tmx* is generated, but on import base64 encoded and compressed *.tmx* files can be loaded too (everything except
zstd). Furthermore PNG or TARGA images are supported on import too, provided they are indexed and their dimensions match the
required 320 x 200 pixels. The palette in these images aren't used, except the spritebank selector is stored in the first entry
(index 0) alpha channel.

### font.bdf

The font, created with the [font editor], in a format that can be edited by many tools, like xmbdfed or gbdfed. Obviously I'd
recommend my [SFNEdit](https://gitlab.com/bztsrc/scalable-font2#font-editor) instead, and [FontForge](https://fontforge.org) works prefectly
too. On import, besides of [X11 Bitmap Distribution Format](https://www.x.org/docs/BDF/bdf.pdf) (.bdf),
[PC Screen Font 2](https://www.win.tue.nl/~aeb/linux/kbd/font-formats-1.html) (.psfu, .psf2),
[Scalable Screen Font](https://gitlab.com/bztsrc/scalable-font2/blob/master/docs/sfn_format.md) (.sfn) and FontForge's native
[SplineFontDB](https://fontforge.org/docs/techref/sfdformat.html) (.sfd, bitmap variant only) supported too.

### sounds.mod

The [sound effects] you created in the editor in Amiga MOD format. See music below. The song must be named `MEG-4 SFX`.

All 31 waveforms are stored in this file, and only the first pattern used, and only one channel (64 notes in total).

### musicXX.mod

The [music tracks] you created in the editor in Amiga MOD format. The *XX* number in the file name is a two digit hexadecimal
number, which represents the track number (from `00` to `07`). The song must be named `MEG-4 TRACKXX`, where *XX* must be the
same hexadecimal number as in the filename. There are dozens of third party programs to edit these files, just google
"[music tracker](https://en.wikipedia.org/wiki/Music_tracker)", for example [MilkyTracker](https://milkytracker.org) or
[OpenMPT](https://openmpt.org), but for a true retro feeling, I'd recommend the moderized clone of
[FastTracker II](https://github.com/8bitbubsy/ft2-clone), available for both Linux and Windows.

From music files, only those waveforms are loaded that are referenced from the notes.

You can find a huge database of downloadable Amiga MOD files at [modarchive.org](https://modarchive.org). But not all files are
actually in *.mod* format on that site (some are *.xm*, *.it* or *.s3m* etc.); you'll have to load those in a tracker and save
them as a *.mod* first.

WARN: The Amiga MOD format is a lot more featureful than what the MEG-4 [DSP](#digital_signal_processor) chip can handle. Keep
this in mind when you edit *.mod* files in third party trackers! Waveforms can't be longer than 16376 samples and songs longer
than 16 patterns (1024 rows) will be truncated on import. Pattern order will be linearized, and although pattern break 0xD
handled, other pattern commands are simply skipped. Also if you import multiple tracks, then they will share the same 31 samples.

Music can also be imported from MIDI files, but these files only store the musical note sheet, and not the waveforms like Amiga
MOD files do. [General MIDI] Patch has standardized the instrument codes though, and MEG-4 has some built-in wavepatterns for
these, but due to size constraints those are not the best quality, so importing your own wavepatterns with MIDI files is strongly
recommended.

### memXX.txt

Hexdump of the [memory overlays] data (which are binary blobs by nature). Here *XX* is a hexadecimal number from `00` to `FF`. The
format is the same as [hexdump -C](https://en.wikipedia.org/wiki/Hex_dump)'s, you can edit these with a text editor. On import,
binary files by the name *memXX.bin* also supported. For example, if you want to embed a file into your MEG-4 program, then name
it *mem00.bin*, drap'n'drop it into the emulator, and afterwards you'll be able to load it in your program with the [memload]
function.

Other Formats
-------------

Normally waveforms are automatically loaded from Amiga MOD files, but you can also individually import and export wave patterns
in *.wav* (RIFF Wave 44100 Hz, 8-bit, mono) format. These are editable with [Audacity](https://www.audacityteam.org) for example.
If the imported file is named as *dspXX.wav*, where *XX* is a hexadecimal number between `01` and `1F`, then the waveform is
loaded at that position, otherwise at the first empty slot.

You can import MEG-4 Color Themes from "GIMP Palette" files. These are simple text files with a little header, and in each line
red, green and blue numeric values. Each color entry line defines the color of a specific UI element, see the default theme
[src/misc/theme.gpl](https://gitlab.com/bztsrc/meg4/blob/main/src/misc/theme.gpl) for an example. Theme files can also be edited
visually using [GIMP](https://www.gimp.org) or [Gpick](http://www.gpick.org) programs too.

Furthermore, you can import PICO-8 cartridges (both in *.p8* and *.p8.png* formats) and TIC-80 cartridges (both in *.tic* and
*.tic.png* formats), however you'll have to adjust the imported source code, because their memory layouts and API calls are
different to MEG-4's. But at least you'll get their assets properly. The TIC-80 project format isn't supported because those
files are unidentifiable. If you really want to import such a file, then first you'll have to convert it using the `prj2tic`
tool, which can be found in the TIC-80 source repo.

Exporting into these cartridges is not possible, because the MEG-4 is lot more featureful than the competition. There's simply
no place for all the MEG-4 features in those files.
