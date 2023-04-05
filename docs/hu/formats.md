Fájlformátumok
==============

Habár a beépített szerkesztők elég királyak, ennek ellenére a MEG-4 mindenféle formátum importját és exportját is támogatja,
hogy segítse a tartalomkészítést, és ténylegesen szórakoztatóvá tegye a MEG-4 használatát.

Flopik
------

A MEG-4 a programokat "flopi" lemezeken tárolja. Ilyen formátumban menthetsz a <kbd>Ctrl</kbd>+<kbd>S</kbd> leütésével, vagy
a `MEG-4` > `Elment` menüpont kiválasztásával (lásd [interfész]). Be fogja kérni a lemez feliratát, ami egyben a programod neve
is lesz. Ilyen flopilemezek betöltéséhez üsd le a <kbd>Ctrl</kbd>+<kbd>L</kbd>-t, vagy csak simán húzd rá ezeket a képfájlokat a
MEG-4 Flopi Meghajtóra.

Az alacsonyszintű specifikáció [itt](https://gitlab.com/bztsrc/meg4/blob/main/docs/floppy.md) érhető el (angol nyelvű).

Projektformátum
---------------

Kényelmed érdekében van egy projektformátum is, ami egy tömörített zip a konzol adataival, benne kizárólag jólismert és
általánosan támogatott formátumú fájlokkal, hogy a kedvenc programoddal vagy eszközöddel is módosíthasd őket. Ebben a formátumban
való mentéshez válaszd a `MEG-4` > `ZIP Export` menüpontot. A betöltéshez csak simán húzd rá az ilyen zip fájlt a MEG-4 Flopi
Meghajtóra.

A benne lévő fájlok:

### metainfo.txt

Sima szöveges fájl, a MEG-4 Förmver verziójával és a programod nevével.

### program.X

A programod forráskódja, amit a [kód szerkesztő]vel hoztál létre, sima szöveges fájl. Bármelyik szövegszerkesztővel módosíthatod.
Exportáláskor a sorvégejelek CRLF-re fordítódnak, hogy Windowson is működjön, importáláskor pedig teljesen mindegy, hogy a sorvége
NL vagy CRLF, mindkettő működik.

A forráskódnak egy speciális sorral kell kezdődnie, a `#!` karakterekkel, amiket a használt programozási nyelv neve követ, például
`#!c` vagy `#!lua`. Ennek a nyelvkódnak meg kell egyeznie a kiterjesztéssel a fájlnévben, pl.: *program.c* vagy *program.lua*.

### sprites.png

Egy indexelt, 256 x 256 pixeles PNG fájl, mind a 256 színnel és egyenként 8 x 8 pixeles 1024 szprájttal, amit a
[szprájt szerkesztő]ben csináltál. Ez a képfájl szerkeszthető a következő programokkal: [GrafX2](http://grafx2.chez.com),
[GIMP](https://www.gimp.org), Paint stb. Importálásnál truecolor képek is betölthetők, ezek az alap MEG-4 palettára lesznek
konvertálva, a legkissebb súlyozott sRGB távolság metódussal. Ez működik, de nem néz ki túl jól, ezért inkább azt javaslom, hogy
eleve palettás képeket importálj be. Szprájtok beolvashatók még [Truevision TARGA](http://www.gamers.org/dEngine/quake3/TGA.txt)
(.tga) képformátumban is, ha indexáltak és a méretük a megfelelő 256 x 256 pixeles.

### map.tmx

A [térkép szerkesztő]ben létrehozott térkép, olyan formátumban, ami használható a [Tiled MapEditor](https://www.mapeditor.org)
programmal. Csakis CSV kódolt *.tmx* kerül mentésre, de importálásnál kezeli a base64 kódolt és tömörített *.tmx* fájlokat is
(mindenfélét, kivéve zstd). Továbbá PNG és TARGA képek is használhatók betöltéskor, amennyiben indexáltak és a méretük megfelel
az elvárt 320 x 200 pixelesnek. Az ilyen képek palettája nem használt, kivéve, hogy a szprájtbank szelektor értéke a paletta első
elemének (0-ás index) alfa csatornájában tárolódik.

### font.bdf

A betűkészlet, amit a [betű szerkesztő]ben hoztál létre, olyan formátumban, amit sok program ismer, például xmbdfed vagy gbdfed.
A módosításhoz nyilvánvalóan inkább a saját [SFNEdit](https://gitlab.com/bztsrc/scalable-font2) programomat ajánlom elsősorban, de
a [FontForge](https://fontforge.org) is tökéletesen megfelel a célnak. Betöltésnél az
[X11 Bitmap Distribution Format](https://www.x.org/docs/BDF/bdf.pdf) (.bdf) formátumon túl támogatott még a
[PC Screen Font 2](https://www.win.tue.nl/~aeb/linux/kbd/font-formats-1.html) (.psfu, .psf2),
[Scalable Screen Font](https://gitlab.com/bztsrc/scalable-font2/blob/master/docs/sfn_format.md) (.sfn), és a FontForge saját
natív [SplineFontDB](https://fontforge.org/docs/techref/sfdformat.html) (.sfd, csak a bitmapes változat) formátuma is.

### sounds.mod

Az általad létrehozott [hangeffektek], Amiga MOD formátumban. Lásd a zenesávokat alább. A zene nevének `MEG-4 SFX`-nek kell lennie.

Mind a 31 hullámminta ebben a fájlban tárolódik, a patternekből viszont csak az első, és abból is csak egy csatorna használt
(64 hangjegy összesen).

### musicXX.mod

A létrehozott [zenesávok], Amiga MOD formátumban. Az *XX* szám a fájlnévben egy kétjegyű hexadecimális szám, ami a sáv számának
felel meg (`00`-tól `07`-ig). A zene nevének `MEG-4 TRACKXX`-nek kell lennie, ahol az *XX* ugyanaz a hexa szám, mint a fájlnévben.
Rengeteg programmal lehet szerkeszteni ezeket a fájlokat, csak guglizz a "[music tracker](https://en.wikipedia.org/wiki/Music_tracker)"-re,
például [MilkyTracker](https://milkytracker.org) vagy [OpenMPT](https://openmpt.org), de az igazi retro életérzéshez ajánlom
inkább a [FastTracker II](https://github.com/8bitbubsy/ft2-clone) modernizált klónját, ami fut Linuxon és Windowson egyaránt.

A zenefájlok hullámmintáiból csak azok kerülnek betöltésre, amikre a zene kottája hivatkozik.

Egy hatalmas adatbázis található letölthető Amiga MOD fájlokkal a [modarchive.org](https://modarchive.org)-on. De nem minden fájl
*.mod*, amit letöltesz onnan (néhány *.xm*, *.it* vagy *.s3m* stb.); ezeket először be kell töltened egy trackerbe és *.mod*-ként
lementeni.

WARN: Az Amiga MOD formátum sokkal többet tud, mint amire a MEG-4 [DSP](#digitalis_szignalfeldolgozo_processzor) csipje képes.
Tartsd ezt észben, amikor külsős programmal szerkeszted a *.mod* fájlokat! A hullámok nem lehetnek hosszabbak 16376 mintánál,
és a 16 patternnél (1024 sornál) hosszabb zenék csonkolódnak importáláskor. A pattern order lineárissá lesz konvertálva, és bár a
pattern break 0xD le van kezelve, az összes többi pattern parancsot egyszerűen figyelmen kívül hagyja. Továbbá ha több sávot is be
akarsz importálni, akkor azok osztozni fognak a 31 közös hullámmintán.

Zenét be lehet még importálni MIDI fájlokból is, de ezek a fájlok csak a kottát tartalmazzák, az Amiga MOD fájlokkal ellentétben a
hullámmintákat nem. A [General MIDI] Patch szabványosította azonban a hangszerkódokat, és a MEG-4 tartalmaz néhány alapértelmezett
hangmintát ezekhez, de a helyszűke miatt nem épp a legjobb minőségűek, emiatt a saját hullámminták betöltése erősen ajánlott, ha
MIDI fájlt akarsz importálni.

### memXX.txt

A [memóriaátfedők] hexdumpjai (amik jellegükből adódóan bináris adatok). Itt az *XX* egy kétjegyű hexadecimális szám, `00`-tól
`FF`-ig, ami az átfedő számát jelenti. Maga a formátum egyszerű, ugyanaz, mint a [hexdump -C](https://en.wikipedia.org/wiki/Hex_dump)
kimenete, és sima szövegszerkesztővel módosítható. Importálásnál bináris fájlokat is elfogad, amennyiben a nevük *memXX.bin*.
Például, ha szeretnél egy fájlt beágyazni a MEG-4 programodba, akkor csak nevezd át *mem00.bin*-re, húzd rá az emulátorra, és
ezután bármikor betöltheted a programodban a [memload] funkció hívásával.

Egyéb formátumok
----------------

Általában a hullámminták automatikusan betöltődnek az Amiga MOD fájlokból, de a hangmintákat külön-külön is importálhatod és
exportálhatod *.wav* (RIFF Wave 44100 Hz, 8-bit, mono) formátumban. Ezek szerkeszthetők például az [Audacity](https://www.audacityteam.org)
programmal. Amennyiben az importálandó fájlneve *dspXX.wav*, ahol az *XX* egy hexadecimális szám `01` és `1F` között, akkor a
hullámminta az adott pozícióra töltődik be, egyébként az első szabad helyre.

Betölthetsz MEG-4 Színtémákat "GIMP Palette" fájlokból. Ezek sima szöveges fájlok, egy rövid fejléccel, és minden sorban piros,
zöld és kék numerikus színértékekkel. Mindegyik színsor az interfész egy adott elemének színéért felelős, példának lásd a
[src/misc/theme.gpl](https://gitlab.com/bztsrc/meg4/blob/main/src/misc/theme.gpl) alap téma definícióját. A témafájlok
szerkeszthetők még vizuálisan a [GIMP](https://www.gimp.org) és a [Gpick](http://www.gpick.org) programokkal is.

Továbbá betölthetők PICO-8 kertridzsek (mindkét *.p8* és *.p8.png* formátumban) valamint TIC-80 kertridzsek (mindkét *.tic* és
*.tic.png* formátumban) is, de a beimportált forráskódot kézzel kell majd szerkesztened, mivel a címkiosztásuk és a függvényeik
eltérnek a MEG-4-étől. De legalább a többi kelléked meglesz. A TIC-80 projektfurmátuma nem támogatott, mivel az ilyen fájlok
felismerhetetlenek. Ha mégis ilyen fájlt szeretnél beimportálni, akkor előbb át kell konvertálnod a `prj2tic` programmal, ami
megtalálható a TIC-80 forrás repójában.

Exportálni kertridzsekbe nem lehet, mivel a MEG-4 sokkal többet tud, mint a vetélytársai. Egyszerűen nincs hely minden MEG-4
funkciónak azokban a fájlokban.
