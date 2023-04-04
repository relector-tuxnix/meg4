MEG-4 MOD Player
================

This is a little test tool I wrote to check if Amiga MOD playback / export works as expected.

Usage
-----

```
./modplayer <in.mod> [out.mod]
```

IMPORTANT: it does not play the .mod file as-is. Instead it imports the file into MEG-4's internal format, and then it uses the
MEG-4 DSP chip to play the converted data. If everything went well, you should hear something *VERY* close to the original file,
but not identical (for example .mod stores stereo wave samples up to 64k, but MEG-4 DSP only supports mono samples no bigger
than 16k each; stereo balance (0x8 and 0xE8), pattern command effects (0xE6, 0xEE, 0xEF) simply skipped, and position jump (0xB)
interpreted as jump to the x*64th row (which could be off if pattern break 0xD also used), etc. etc. etc.)
