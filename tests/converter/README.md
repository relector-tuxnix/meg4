MEG-4 Converter
===============

This is a little test tool I wrote to check if import / export works as expected.

Usage
-----

```
./converter <somefile>
````

This will try to import `somefile` in any of the supported formats using MEG-4 importer, and then outputs `output.zip` with only
well-known and common file formats in it. This can be used to convert MEG-4 floppy disks, but also PSFU files into BDF files, PNGs
into Tiled TMX, MIDI to Amiga MOD, hexdumps into binaries for example. Accidentally also useful to rip PICO-8 or TIC-80 cartridges...

NOTE: This tool was written to test MEG-4 functionality, and it was never intended to be a standalone, fully featured conversion
tool, although in most cases it might just work as such. It is dependency-free, requires nothing besides libc.
