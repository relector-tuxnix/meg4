MEG-4 Documentation
===================

The user's manual is created with [gendoc](https://bztsrc.gitlab.io/gendoc). You'll have to download and compile that tool.
```
wget https://gitlab.com/bztsrc/gendoc/-/raw/main/gendoc.c
gcc gendoc.c -o gendoc
```
Simple as that. Once you have gendoc, you can create the manual by going into one of the language code directories and running
```
cd docs/en
gendoc ../../pages/manual_en.html manual.xml
```

Translating the User's Manual
-----------------------------

1. Copy the `docs/en` directory to the ISO-639-1 language code of the new language, for example `docs/hu`.
2. There are some translateable texts in the doc (like "Search Results" and other captions, labels of the "Previous" and "Next"
   page buttons etc.), these are specified in `docs/hu/manual.xml` between the `<doc>` ... `</doc>` tag.
3. Everything else is in MarkDown files, which you can translate freely.
4. Copy the API reference in [src/lang/en.md](../src/lang/en.md) to `src/lang/hu.md`.
5. Be careful when you translate the latter, uses a special, minimalistic subset of MarkDown, because the API reference is
   embedded in the MEG-4 executable too. Watch out for the linebreaks, because the built-in help system does not break long lines
   automatically.

Translating MEG-4
-----------------

1. Increase the `NUMLANGS` define in [src/lang.h](../src/lang.h).
2. Copy [src/lang/en.h](../src/lang/en.h) to a new file, for example `src/lang/hu.h`.
3. This contains only strings, translate them. The first is the two-letter ISO-639-1 language code. The keys can be found in `lang.h`.
4. Include this new dictionary header in [meg4.c](../src/meg4.c#L31) in the `dict` array.

If you have used a new writing system (or most notably any CJK), then in the [src/misc](../src/misc) directory run
```
make langchk
```
This will check if the default font has all the glyphs for the new translation. If it reports errors, then use
[SFNEdit](https://gitlab.com/bztsrc/scalable-font2) on `src/misc/default.sfn` and add the missing glyphs.

Finally, run `make distclean all` in the [src](../src) directory. This will parse the assets (including the new translation
files) and regenerate `data.c` to be embedded in MEG-4 (a clean recompilation should take no more than a minute).
