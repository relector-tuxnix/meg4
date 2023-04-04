# Memóriatérkép

## Általános

Minden érték kicsi elöl (little endian), azaz a kissebb helyiértékű bájt van az alacsonyabb címen.

| Cím    | Méret      | Leírás                                                             |
|--------|-----------:|--------------------------------------------------------------------|
|  00000 |          1 | MEG-4 förmver verzió major (fő verzió)                             |
|  00001 |          1 | MEG-4 förmver verzió minor (alverzió)                              |
|  00002 |          1 | MEG-4 förmver verzió bugfix (hibajavító verzió)                    |
|  00003 |          1 | performancia számláló, eltöltetlen idő 1/1000 másodpercekben       |
|  00004 |          4 | bekapcsolás óta eltelt idő 1/1000 másodpercekben                   |
|  00008 |          8 | UTC unix időbélyeg                                                 |
|  00010 |          2 | kiválaszott lokál, nyelvkód                                        |

A performancia számláló azt mutatja, a legutóbbi képkocka generálásakor mennyi idő maradt kihasználatlanul. Ha ez nulla vagy
negatív, akkor az azt jelzi, mennyivel lépte túl a loop() függvényed a rendelkezésre álló időkeretet.

## Mutató

| Cím    | Méret      | Leírás                                                             |
|--------|-----------:|--------------------------------------------------------------------|
|  00012 |          2 | mutató (egér) gombok állapota (lásd [getbtn] és [getclk])          |
|  00014 |          2 | mutató szprájt index                                               |
|  00016 |          2 | mutató X koordináta                                                |
|  00018 |          2 | mutató Y koordináta                                                |

A mutató egérgombjai a következők:

| Define  | Bitmaszk  | Leírás                                                             |
|---------|----------:|--------------------------------------------------------------------|
| `BTN_L` |         1 | Bal egérgomb (left)                                                |
| `BTN_M` |         2 | Középső egérgomb (middle)                                          |
| `BTN_R` |         4 | Jobb egérgomb (right)                                              |
| `SCR_U` |         8 | Szkrollozás fel (up)                                               |
| `SCR_D` |        16 | Szkrollozás le (down)                                              |
| `SCR_L` |        32 | Szkrollozás balra (left)                                           |
| `SCR_R` |        64 | Szkrollozás jobbra (right)                                         |

A mutató szprájt felső bitjei adják meg a kurzor eltolását: bit 13-15 Y eltolás, bit 10-12 X eltolás, bit 0-9 szprájt.
Van néhány beépített, előre definiált kurzor:

| Define       | Érték     | Leírás                                                        |
|--------------|----------:|---------------------------------------------------------------|
| `PTR_NORM`   |      03fb | Normál (nyíl) mutató                                          |
| `PTR_TEXT`   |      03fc | Szöveg mutató                                                 |
| `PTR_HAND`   |      0bfd | Hivatkozás mutató                                             |
| `PTR_ERR`    |      93fe | Hiba történt mutató                                           |
| `PTR_NONE`   |      ffff | Elrejti a mutatót                                             |

## Billentyűzet

| Cím    | Méret      | Leírás                                                             |
|--------|-----------:|--------------------------------------------------------------------|
|  0001A |          1 | billentyűsor farok                                                 |
|  0001B |          1 | billentyűsor fej                                                   |
|  0001C |         64 | billentyűsor, 16 elem, egyenként 4 bájt (lásd [popkey])            |
|  0005C |         18 | lenyomott billentyű állapotok, szkenkódonként (lásd [getkey])      |

A billentyűsorból kivett gombok UTF-8-ban vannak ábrázolva. Néhány érvénytelen UTF-8 sorozat
speciális (nem-megjeleníthető) gombnak felel meg, például:

| Gombkód | Leírás                                          |
|---------|-------------------------------------------------|
| `\x8`   | A 8-as karakter, <kbd>Backspace</kbd> gomb      |
| `\x9`   | A 9-es karakter, <kbd>Tab</kbd> gomb            |
| `\n`    | A 10-es karakter, <kbd>⏎Enter</kbd> gomb        |
| `\x1b`  | A 27-es karakter, <kbd>Esc</kbd> gomb           |
| `Del`   | A <kbd>Del</kbd> (törlés előre) gomb            |
| `Up`    | A kurzornyíl <kbd>▴</kbd> gomb                  |
| `Down`  | A kurzornyíl <kbd>▾</kbd> gomb                  |
| `Left`  | A kurzornyíl <kbd>◂</kbd> gomb                  |
| `Rght`  | A kurzornyíl <kbd>▸</kbd> gomb                  |
| `Cut`   | Kivág gomb (vagy <kbd>Ctrl</kbd>+<kbd>X</kbd>)  |
| `Cpy`   | Másol gomb (vagy <kbd>Ctrl</kbd>+<kbd>C</kbd>)  |
| `Pst`   | Beilleszt gomb (vagy <kbd>Ctrl</kbd>+<kbd>V</kbd>) |

A szkenkódok a következők:

| Szkenkód | Cím     | Bitmaszk| Define            |
|---------:|---------|--------:|-------------------|
|        0 |   0005C |       1 | `KEY_CHEAT`       |
|        1 |   0005C |       2 | `KEY_F1`          |
|        2 |   0005C |       4 | `KEY_F2`          |
|        3 |   0005C |       8 | `KEY_F3`          |
|        4 |   0005C |      16 | `KEY_F4`          |
|        5 |   0005C |      32 | `KEY_F5`          |
|        6 |   0005C |      64 | `KEY_F6`          |
|        7 |   0005C |     128 | `KEY_F7`          |
|        8 |   0005D |       1 | `KEY_F8`          |
|        9 |   0005D |       2 | `KEY_F9`          |
|       10 |   0005D |       4 | `KEY_F10`         |
|       11 |   0005D |       8 | `KEY_F11`         |
|       12 |   0005D |      16 | `KEY_F12`         |
|       13 |   0005D |      32 | `KEY_PRSCR`       |
|       14 |   0005D |      64 | `KEY_SCRLOCK`     |
|       15 |   0005D |     128 | `KEY_PAUSE`       |
|       16 |   0005E |       1 | `KEY_BACKQUOTE`   |
|       17 |   0005E |       2 | `KEY_1`           |
|       18 |   0005E |       4 | `KEY_2`           |
|       19 |   0005E |       8 | `KEY_3`           |
|       20 |   0005E |      16 | `KEY_4`           |
|       21 |   0005E |      32 | `KEY_5`           |
|       22 |   0005E |      64 | `KEY_6`           |
|       23 |   0005E |     128 | `KEY_7`           |
|       24 |   0005F |       1 | `KEY_8`           |
|       25 |   0005F |       2 | `KEY_9`           |
|       26 |   0005F |       4 | `KEY_0`           |
|       27 |   0005F |       8 | `KEY_MINUS`       |
|       28 |   0005F |      16 | `KEY_EQUAL`       |
|       29 |   0005F |      32 | `KEY_BACKSPACE`   |
|       30 |   0005F |      64 | `KEY_TAB`         |
|       31 |   0005F |     128 | `KEY_Q`           |
|       32 |   00060 |       1 | `KEY_W`           |
|       33 |   00060 |       2 | `KEY_E`           |
|       34 |   00060 |       4 | `KEY_R`           |
|       35 |   00060 |       8 | `KEY_T`           |
|       36 |   00060 |      16 | `KEY_Y`           |
|       37 |   00060 |      32 | `KEY_U`           |
|       38 |   00060 |      64 | `KEY_I`           |
|       39 |   00060 |     128 | `KEY_O`           |
|       40 |   00061 |       1 | `KEY_P`           |
|       41 |   00061 |       2 | `KEY_LBRACKET`    |
|       42 |   00061 |       4 | `KEY_RBRACKET`    |
|       43 |   00061 |       8 | `KEY_ENTER`       |
|       44 |   00061 |      16 | `KEY_CAPSLOCK`    |
|       45 |   00061 |      32 | `KEY_A`           |
|       46 |   00061 |      64 | `KEY_S`           |
|       47 |   00061 |     128 | `KEY_D`           |
|       48 |   00062 |       1 | `KEY_F`           |
|       49 |   00062 |       2 | `KEY_G`           |
|       50 |   00062 |       4 | `KEY_H`           |
|       51 |   00062 |       8 | `KEY_J`           |
|       52 |   00062 |      16 | `KEY_K`           |
|       53 |   00062 |      32 | `KEY_L`           |
|       54 |   00062 |      64 | `KEY_SEMICOLON`   |
|       55 |   00062 |     128 | `KEY_APOSTROPHE`  |
|       56 |   00063 |       1 | `KEY_BACKSLASH`   |
|       57 |   00063 |       2 | `KEY_LSHIFT`      |
|       58 |   00063 |       4 | `KEY_LESS`        |
|       59 |   00063 |       8 | `KEY_Z`           |
|       60 |   00063 |      16 | `KEY_X`           |
|       61 |   00063 |      32 | `KEY_C`           |
|       62 |   00063 |      64 | `KEY_V`           |
|       63 |   00063 |     128 | `KEY_B`           |
|       64 |   00064 |       1 | `KEY_N`           |
|       65 |   00064 |       2 | `KEY_M`           |
|       66 |   00064 |       4 | `KEY_COMMA`       |
|       67 |   00064 |       8 | `KEY_PERIOD`      |
|       68 |   00064 |      16 | `KEY_SLASH`       |
|       69 |   00064 |      32 | `KEY_RSHIFT`      |
|       70 |   00064 |      64 | `KEY_LCTRL`       |
|       71 |   00064 |     128 | `KEY_LSUPER`      |
|       72 |   00065 |       1 | `KEY_LALT`        |
|       73 |   00065 |       2 | `KEY_SPACE`       |
|       74 |   00065 |       4 | `KEY_RALT`        |
|       75 |   00065 |       8 | `KEY_RSUPER`      |
|       76 |   00065 |      16 | `KEY_MENU`        |
|       77 |   00065 |      32 | `KEY_RCTRL`       |
|       78 |   00065 |      64 | `KEY_INS`         |
|       79 |   00065 |     128 | `KEY_HOME`        |
|       80 |   00066 |       1 | `KEY_PGUP`        |
|       81 |   00066 |       2 | `KEY_DEL`         |
|       82 |   00066 |       4 | `KEY_END`         |
|       83 |   00066 |       8 | `KEY_PGDN`        |
|       84 |   00066 |      16 | `KEY_UP`          |
|       85 |   00066 |      32 | `KEY_LEFT`        |
|       86 |   00066 |      64 | `KEY_DOWN`        |
|       87 |   00066 |     128 | `KEY_RIGHT`       |
|       88 |   00067 |       1 | `KEY_NUMLOCK`     |
|       89 |   00067 |       2 | `KEY_KP_DIV`      |
|       90 |   00067 |       4 | `KEY_KP_MUL`      |
|       91 |   00067 |       8 | `KEY_KP_SUB`      |
|       92 |   00067 |      16 | `KEY_KP_7`        |
|       93 |   00067 |      32 | `KEY_KP_8`        |
|       94 |   00067 |      64 | `KEY_KP_9`        |
|       95 |   00067 |     128 | `KEY_KP_ADD`      |
|       96 |   00068 |       1 | `KEY_KP_4`        |
|       97 |   00068 |       2 | `KEY_KP_5`        |
|       98 |   00068 |       4 | `KEY_KP_6`        |
|       99 |   00068 |       8 | `KEY_KP_1`        |
|      100 |   00068 |      16 | `KEY_KP_2`        |
|      101 |   00068 |      32 | `KEY_KP_3`        |
|      102 |   00068 |      64 | `KEY_KP_ENTER`    |
|      103 |   00068 |     128 | `KEY_KP_0`        |
|      104 |   00069 |       1 | `KEY_KP_DEC`      |
|      105 |   00069 |       2 | `KEY_INT1`        |
|      106 |   00069 |       4 | `KEY_INT2`        |
|      107 |   00069 |       8 | `KEY_INT3`        |
|      108 |   00069 |      16 | `KEY_INT4`        |
|      109 |   00069 |      32 | `KEY_INT5`        |
|      110 |   00069 |      64 | `KEY_INT6`        |
|      111 |   00069 |     128 | `KEY_INT7`        |
|      112 |   0006A |       1 | `KEY_INT8`        |
|      113 |   0006A |       2 | `KEY_LNG1`        |
|      114 |   0006A |       4 | `KEY_LNG2`        |
|      115 |   0006A |       8 | `KEY_LNG3`        |
|      116 |   0006A |      16 | `KEY_LNG4`        |
|      117 |   0006A |      32 | `KEY_LNG5`        |
|      118 |   0006A |      64 | `KEY_LNG6`        |
|      119 |   0006A |     128 | `KEY_LNG7`        |
|      120 |   0006B |       1 | `KEY_LNG8`        |
|      121 |   0006B |       2 | `KEY_APP`         |
|      122 |   0006B |       4 | `KEY_POWER`       |
|      123 |   0006B |       8 | `KEY_KP_EQUAL`    |
|      124 |   0006B |      16 | `KEY_EXEC`        |
|      125 |   0006B |      32 | `KEY_HELP`        |
|      126 |   0006B |      64 | `KEY_SELECT`      |
|      127 |   0006B |     128 | `KEY_STOP`        |
|      128 |   0006C |       1 | `KEY_AGAIN`       |
|      129 |   0006C |       2 | `KEY_UNDO`        |
|      130 |   0006C |       4 | `KEY_CUT`         |
|      131 |   0006C |       8 | `KEY_COPY`        |
|      132 |   0006C |      16 | `KEY_PASTE`       |
|      133 |   0006C |      32 | `KEY_FIND`        |
|      134 |   0006C |      64 | `KEY_MUTE`        |
|      135 |   0006C |     128 | `KEY_VOLUP`       |
|      136 |   0006D |       1 | `KEY_VOLDN`       |


## Játékpad

| Cím    | Méret      | Leírás                                                             |
|--------|-----------:|--------------------------------------------------------------------|
|  0006E |          2 | játékpad joystick határérték (alapból 8000)                        |
|  00070 |          8 | elsődleges játékpad - [billentyűzet] szkenkód leképezések          |
|  00078 |          4 | 4 játékpad gombjainak lenyomott állapota (lásd [getpad])           |

A játékpad gombok a következők:

| Define  | Bitmaszk  | Leírás                                                             |
|---------|----------:|--------------------------------------------------------------------|
| `BTN_L` |         1 | A `◁` gomb vagy joystick balra                                    |
| `BTN_U` |         2 | A `△` gomb vagy joystick felfele                                  |
| `BTN_R` |         4 | A `▷` gomb vagy joystick jobbra                                   |
| `BTN_D` |         8 | A `▽` gomb vagy joystick lefele                                   |
| `BTN_A` |        16 | Az `Ⓐ` gomb                                                        |
| `BTN_B` |        32 | A `Ⓑ` gomb                                                         |
| `BTN_X` |        64 | A `Ⓧ` gomb                                                         |
| `BTN_Y` |       128 | A `Ⓨ` gomb                                                         |

## Grafikus Feldolgozó Egység

| Cím    | Méret      | Leírás                                                             |
|--------|-----------:|--------------------------------------------------------------------|
|  0007E |          1 | UNICODE kódpont felső bájtja a glifablakhoz                        |
|  0007F |          1 | szprájtbank választó a térképhez                                   |
|  00080 |       1024 | paletta, 256 szín, egyenként 4 bájt, RGBA                          |
|  00480 |          2 | x0, vágás X kezdete pixelben (minden rajzoló funkció esetén)       |
|  00482 |          2 | x1, vágás X vége pixelben                                          |
|  00484 |          2 | y0, vágás Y kezdete pixelben                                       |
|  00486 |          2 | y1, vágás Y vége pixelben                                          |
|  00488 |          2 | megjelenített vram X offszetje pixelben vagy 0xffff                |
|  0048A |          2 | megjelenített vram Y offszetje pixelben vagy 0xffff                |
|  0048C |          1 | teknős farok lent állapot (lásd [up], [down])                      |
|  0048D |          1 | teknős farok színe, paletta index 0-tól 255-ig (lásd [color])      |
|  0048E |          2 | teknős irány fokokban, 0-tól 359-ig (lásd [left], [right])         |
|  00490 |          2 | teknős X koordináta pixelben (lásd [move])                         |
|  00492 |          2 | teknős Y koordináta pixelben                                       |
|  00494 |          2 | útvesztő haladási sebesség 1/128-ad csempében (lásd [maze])        |
|  00496 |          2 | útvesztő forgási sebesség fokokban (1-től 90-ig)                   |
|  00498 |          2 | kamera X koordináta pixelben (lásd [tri3d])                        |
|  0049A |          2 | kamera Y koordináta pixelben                                       |
|  0049C |          2 | kamera Z koordináta pixelben                                       |
|  0049E |          1 | konzol betűszíne, paletta index 0-tól 255-ig (lásd [printf])       |
|  0049F |          1 | konzol háttérszíne, paletta index 0-tól 255-ig                     |
|  004A0 |          2 | konzol X koordináta pixelben                                       |
|  004A2 |          2 | konzol Y koordináta pixelben                                       |
|  00600 |      64000 | térkép, 320 x 200 szprájt index (lásd [map] és [maze])             |
|  10000 |      65536 | szprájtok, 256 x 256 paletta index, 1024 8 x 8 pixel (lásd [spr])  |
|  28000 |      32768 | csúszóablak 4096 betűglifhez (lásd 0007E, [width] és [text])       |

## Digitális Szignálfeldolgozó Processzor

| Cím    | Méret      | Leírás                                                             |
|--------|-----------:|--------------------------------------------------------------------|
|  0007C |          1 | hullámminta bank választó (1-től 31-ig)                            |
|  0007D |          1 | zenesáv bank választó (0-tól 7-ig)                                 |
|  004BA |          1 | aktuális tempó (soronkénti tikkszám, csak olvasható)               |
|  004BB |          1 | aktuális sáv, amit épp játszik (csak olvasható)                    |
|  004BC |          2 | aktuális sor, amit épp játszik (csak olvasható)                    |
|  004BE |          2 | aktuális sáv sorainak száma (csak olvasható)                       |
|  004C0 |         64 | 16 csatorna státusz regisztere, egyenként 4 bájt (csak olvasható)  |
|  00500 |        256 | 64 hangeffekt, egyenként 4 bájt                                    |
|  20000 |      16384 | ablak a hullámmintára (lásd 0007C)                                 |
|  24000 |      16384 | ablak a zenesáv mintákra (lásd 0007D)                              |

Az összes DSP státusz regiszter csak olvasható, és a csatornák regiszterei a következők:

| Cím    | Méret      | Leírás                                                             |
|--------|-----------:|--------------------------------------------------------------------|
|      0 |          2 | aktuális pozíció az épp lejátszott hullámmintában                  |
|      2 |          1 | aktuális hullámminta (1-től 31-ig, 0 ha a csatorna nem szól)       |
|      3 |          1 | aktuális hangerő (0 ha a csatorna ki van kapcsolva)                |

Az első 4 csatorna a zenéé, a többi a hangeffekteké.

A hullámminták esetén a 0-ás index nincs tárolva, mivel azt jelenti, "használd a korábbi mintát", ezért ez nem használható
a választóban. Az összes többi hullámminta formátuma:

| Cím    | Méret      | Leírás                                                             |
|--------|-----------:|--------------------------------------------------------------------|
|      0 |          2 | minták száma                                                       |
|      2 |          2 | ismétlés kezdete                                                   |
|      4 |          2 | ismétlés hossza                                                    |
|      6 |          1 | finomhangolás, -8-tól 7-ig                                         |
|      7 |          1 | hangerő, 0-tól 64-ig                                               |
|      8 |      16376 | előjeles 8-bites mono minták                                       |

A hangeffektek és a zenei sávok formátuma ugyanaz, csak annyi a különbség, hogy a zenéknél 4 hangjegy van soronként, minden
csatornához egy-egy és 1024 sor van összesen; míg a hangeffekteknél csak egy hangjegy van és 64 sor.

| Cím    | Méret      | Leírás                                                             |
|--------|-----------:|--------------------------------------------------------------------|
|      0 |          1 | hangjegy, lásd `NOTE_x` defineok, 0-tól 96-ig                      |
|      1 |          1 | hullámminta index, 0-tól 31-ig                                     |
|      2 |          1 | effekt típusa, 0-tól 255-ig                                        |
|      3 |          1 | effekt paraméter                                                   |

A hangjegy sorszáma a következő: a 0 azt jelenti, nincs beállítva. A többi pedig 8 oktávonként 12 érték, azaz az 1-es a C-0,
12-es a B-0 (a legmélyebb oktávon), 13-as a C-1 (egy oktávval magasabb), a 14-es pedig a C#1 (cisz, fél hanggal magasabb).
A D hang a 4. oktávon tehát 1 + 4\*12 + 2 = 51. A B-7 a 96-os, a legmagasabb hang a legmagasabb oktávon. De vannak előredefiniált
define-ok hozzájuk, például a C-1 a `NOTE_C_1` és a C#1 az `NOTE_Cs1`, ha nem akarsz számolni, akkor használhatod ezeket is a
programodban.

Az egyszerűség kedvéért a MEG-4 ugyanazokat az effektkódokat használja, mint az Amiga MOD fájlok (így ugyanazt látod a beépített
zeneszerkesztőben mint egy külsős trackerben), de nem támogatja az összeset. Mint korábban említettük, ezek a kódok három hexa
számból állnak, az első a típus `t`, az utolsó kettő pedig a paraméter, `xy` (vagy `xx`). Az `E1`-től `ED`-ig mind a típus
bájtban van tárolva, annak ellenére, hogy úgy látszik, egy tetrádja a paraméterbe lóg, pedig nem is.

| Effekt | Kód  | Leírás                                                     |
|--------|------|------------------------------------------------------------|
| ...    | 000  | Nincs effekt                                               |
| Arp    | 0xy  | Arpeggio, játszd le a hangot, hang+x, hang+y félhangot is  |
| Po/    | 1xx  | Portamento fel, periódus csúsztatása x-el felfelé          |
| Po\\   | 2xx  | Portamento le, periódus csúsztatása x-el lefelé            |
| Ptn    | 3xx  | Tone portamento, periódus csúsztatása x-re                 |
| Vib    | 4xy  | Vibrato, y félhanggal oszcillálja a magasságot x freken    |
| Ctv    | 5xy  | Tone portamento folyt. + hangerőcsúsztatás x fel vagy y le |
| Cvv    | 6xy  | Vibrato folyt. + hangerőcsúsztatás x fel vagy y le         |
| Trm    | 7xy  | Tremolo, y amplitudóval oszcillálja a hangerőt x freken    |
| Ofs    | 9xx  | Hanghullám minta kezdjen x \* 256 pozíción                 |
| Vls    | Axy  | Hangerőcsúsztatás x fel vagy y le                          |
| Jmp    | Bxx  | Pozícióugrás, a x \* 64 -dik sorra                         |
| Vol    | Cxx  | Hangerő beállítása x-re (0 és 64 közötti)                  |
| Fp/    | E1x  | Finom portamento fel, periódus növelése x-el               |
| Fp\\   | E2x  | Finom portamento le, periódus csökkentése x-el             |
| Svw    | E4x  | Vibrato hullámtípusa, 0 szinusz, 1 fűrész, 2 négyzet, 3 zaj|
| Ftn    | E5x  | Finomhangolás, tunningolás beállítása x-re (-8-tól 7-ig)   |
| Stw    | E7x  | Tremolo hullámtípusa, 0 szinusz, 1 fűrész, 2 négyzet, 3 zaj|
| Rtg    | E9x  | Hang újrázás, a hullám újrakezdése x tikkenként            |
| Fv/    | EAx  | Finom hangerő csúsztatás felfelé, x-el                     |
| Fv\\   | EBx  | Finom hangerő csúsztatás lefelé, x-el                      |
| Cut    | ECx  | Hang elvágása x tikknél                                    |
| Dly    | EDx  | Hang késleltetése x tikkel                                 |
| Tpr    | Fxx  | Soronkénti tikkszám beállítása x-re (alapból 6)            |

## Felhasználói memória

A 00000-tól 2FFFF-ig terjedő memóriacímek az MMIO-é, minden más fölötte (a 30000 címtől avagy `MEM_USER`-tól kezdve) szabadon
hasznosítható felhasználói memória.

| Cím    | Méret      | Leírás                                                             |
|--------|-----------:|--------------------------------------------------------------------|
|  30000 |          4 | (csak BASIC) a DATA címe                                           |
|  30004 |          4 | (csak BASIC) aktulális READ számláló                               |
|  30008 |          4 | (csak BASIC) maximum READ számláló, DATA elemszáma                 |

Ezt követi a globális változók blokkja, amiket a programodban deklaráltál, azt pedig a konstansok, mint például a sztring
literálok. A BASIC nyelv esetén ezután jönnek a tényleges DATA rekordok.

Az inicializált adatok feletti memóriacímeket dinamikusan allokálhatod és felszabadíthatod a programodból a
[malloc] és [free] hívásokkal.

Végezetül pedig a verem, a memória legtetején (a C0000-ás címtől avagy `MEM_LIMIT`-től kezdődően), ami *lefele* növekszik. A
programod lokális változói (amiket függvényeken belül deklaráltál) ide kerülnek. A verem mérete folyton változik, attól függően,
hogy épp melyik függvény hív melyik másik függvényt a programodban.

Amennyiben a dinamikusan allokált memória teteje és a verem alja összeérne, akkor a MEG-4 egy "Nincs elég memória" hibaüzenetet
dob.

## Formázó sztring

Néhány függvény, a [printf], [sprintf] és a [trace] formázó sztringet használ, amiben speciális karakterek lehetnek, amik a
paraméterekre hivatkoznak és előírják, hogyan kell azokat megjeleníteni. Ezek a következők:

| Kód  | Leírás                                                     |
|------|------------------------------------------------------------|
| `%%` | A `%` karakter maga                                        |
| `%d` | A soronkövetkező paramétert decimális számként írja ki     |
| `%u` | A soronkövetkező paramétert pozitív számként írja ki       |
| `%x` | A soronkövetkező paramétert hexadecimális számként írja ki |
| `%o` | A soronkövetkező paramétert oktális számként írja ki       |
| `%b` | A soronkövetkező paramétert bináris számként írja ki       |
| `%f` | A soronkövetkező paramétert lebegőpontos számként írja ki  |
| `%s` | A soronkövetkező paramétert sztringként kell kiírni        |
| `%c` | A soronkövetkező paramétert UTF-8 karakterként kell kiírni |
| `%p` | A soronkövetkező paramétert címként írja ki (pointer)      |
| `\t` | Tab, igazítsd vízszintesen a pozíciót kiírás előtt         |
| `\n` | Kezd új sorban a kiírást                                   |

Megadható kitöltés a `%` és a kód közötti méret megadásával. Ha ez `0`-val kezdődik, akkor nullával tölt ki, egyébként szóközzel.
Például a `%4d` jobbra fogja igazítani az értéket szóközökkel, míg a `%04x` nullákkal teszi ezt. Az `f` elfogad pontot és egy
számot utána, ami a tizedesjegyek számát adja meg (egészen 8-ig), például `%.6f`.

# Konzol

## putc

```c
void putc(uint32_t chr)
```
<dt>Leírás</dt><dd>
Kiír egy karaktert a képernyőre.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| chr | UTF-8 karakter |
</dd>
<hr>
## printf

```c
void printf(str_t fmt, ...)
```
<dt>Leírás</dt><dd>
Kiír egy szöveget a képernyőre.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| fmt | megjelenítendő [formázó sztring] |
| ... | opcionális paraméterek |
</dd>
<hr>
## getc

```c
uint32_t getc(void)
```
<dt>Leírás</dt><dd>
Beolvas egy karaktert a konzolról, blokkolódik, ha nincs mit.
</dd>
<dt>Visszatérési érték</dt><dd>
Egy UTF-8 karakter, amit a felhasználó leütött.
</dd>
<dt>Lásd még</dt><dd>
[popkey]
</dd>
<hr>
## gets

```c
str_t gets(void)
```
<dt>Leírás</dt><dd>
Bekér egy újsor karakterrel lezárt szöveget a felhasználótól (az újsor karaktert nem adja vissza).
</dd>
<dt>Visszatérési érték</dt><dd>
A beolvasott bájtok egy sztringben.
</dd>
<hr>
## trace

```c
void trace(str_t fmt, ...)
```
<dt>Leírás</dt><dd>
A futás jelzése a kimenetre való írással. Csak akkor működik, ha a `meg4` a `-v` kapcsolóval lett indítva.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| fmt | [formázó sztring] |
| ... | opcionális paraméterek |
</dd>
<hr>
## delay

```c
void delay(uint16_t msec)
```
<dt>Leírás</dt><dd>
Késlelteti a programod végrehajtását.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| msec | késleltetés ezredmásodpercekben |
</dd>
<hr>
## exit

```c
void exit(void)
```
<dt>Leírás</dt><dd>
Kilép a programból.
</dd>

# Audió

## sfx

```c
void sfx(uint8_t sfx, uint8_t channel, uint8_t volume)
```
<dt>Leírás</dt><dd>
Lejátszik egy hangeffektet.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| sfx | a hangeffekt indexe, 0-tól 63-ig |
| channel | használni kívánt csatorna, 0-tól 11-ig |
| volume | hangerő, 0-tól 255-ig, 0 kikapcsolja a csatornát |
</dd>
<hr>
## music

```c
void music(uint8_t track, uint16_t row, uint8_t volume)
```
<dt>Leírás</dt><dd>
Lejátszik egy zenesávot.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| track | a zenesáv indexe, 0-tól 7-ig |
| row | amelyik sortól kezdve kell lejátszani, 0-tól 1023-ig (max sávhossz) |
| volume | hangerő, 0-tól 255-ig, 0 kikapcsolja a zenét |
</dd>

# Grafika

## cls

```c
void cls(uint8_t palidx)
```
<dt>Leírás</dt><dd>
Törli a képernyőt és alaphelyzetbe állítja a megjelenítő ablakát.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| palidx | szín, paletta index 0-tól 255-ig |
</dd>
<dt>Lásd még</dt><dd>
[pget], [pset]
</dd>
<hr>
## cget

```c
uint32_t cget(uint16_t x, uint16_t y)
```
<dt>Leírás</dt><dd>
Kiolvassa a megadott koordinátán lévő pixelt, és RGBA színt ad vissza.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| x | X koordináta pixelben |
| y | Y koordináta pixelben |
</dd>
<dt>Visszatérési érték</dt><dd>
Egy csomagolt színkód, RGBA csatornákkal (piros a legalacsonyabb bájtban).
</dd>
<dt>Lásd még</dt><dd>
[cls], [pget], [pset]
</dd>
<hr>
## pget

```c
uint8_t pget(uint16_t x, uint16_t y)
```
<dt>Leírás</dt><dd>
Kiolvassa a megadott koordinátán lévő pixelt, és paletta indexet ad vissza.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| x | X koordináta pixelben |
| y | Y koordináta pixelben |
</dd>
<dt>Visszatérési érték</dt><dd>
Szín paletta indexe, 0-tól 255-ig.
</dd>
<dt>Lásd még</dt><dd>
[cls], [pset], [cget]
</dd>
<hr>
## pset

```c
void pset(uint8_t palidx, uint16_t x, uint16_t y)
```
<dt>Leírás</dt><dd>
Kirak egy pixelt a megadott koordinátára.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| palidx | szín, paletta index 0-tól 255-ig |
| x | X koordináta pixelben |
| y | Y koordináta pixelben |
</dd>
<dt>Lásd még</dt><dd>
[cls], [pget]
</dd>
<hr>
## width

```c
uint16_t width(int8_t type, str_t str)
```
<dt>Leírás</dt><dd>
Visszaadja a megjelenítendő szöveg szélességét pixelekben.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| type | betűtípus, -4-től 4-ig |
| str | megmérendő sztring |
</dd>
<dt>Visszatérési érték</dt><dd>
A szöveg megjelenítéshez szükséges pixelek száma.
</dd>
<dt>Lásd még</dt><dd>
[text]
</dd>
<hr>
## text

```c
void text(uint8_t palidx, int16_t x, int16_t y, int8_t type, uint8_t shidx, uint8_t sha, str_t str)
```
<dt>Leírás</dt><dd>
Szöveget ír ki a képernyőre.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| palidx | szín, paletta index 0-tól 255-ig |
| x | X koordináta pixelben |
| y | Y koordináta pixelben |
| type | betűtípus, -4-től -1-ig monospace, 1-től 4-ig proporcionális |
| shidx | árnyék színe, paletta index 0-tól 255-ig |
| sha | árnyék átlátszósága, 0-tól (teljesen átlátszó) 255-ig (semennyire) |
| str | megjelenítendő sztring |
</dd>
<dt>Lásd még</dt><dd>
[width]
</dd>
<hr>
## line

```c
void line(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
```
<dt>Leírás</dt><dd>
Húz egy anti-aliasolt vonalat.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| palidx | szín, paletta index 0-tól 255-ig |
| x0 | induló X koordináta pixelekben |
| y0 | induló Y koordináta pixelekben |
| x1 | vége X koordináta pixelekben |
| y1 | vége Y koordináta pixelekben |
</dd>
<dt>Lásd még</dt><dd>
[qbez], [cbez]
</dd>
<hr>
## qbez

```c
void qbez(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1,
    int16_t cx, int16_t cy)
```
<dt>Leírás</dt><dd>
Négyzetes (quadratic) Bezier ív húzása.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| palidx | szín, paletta index 0-tól 255-ig |
| x0 | induló X koordináta pixelekben |
| y0 | induló Y koordináta pixelekben |
| x1 | vége X koordináta pixelekben |
| y1 | vége Y koordináta pixelekben |
| cx | kontrollpont X koordináta pixelekben |
| cy | kontrollpont Y koordináta pixelekben |
</dd>
<dt>Lásd még</dt><dd>
[line], [cbez]
</dd>
<hr>
## cbez

```c
void cbez(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1,
    int16_t cx0, int16_t cy0, int16_t cx1, int16_t cy1)
```
<dt>Leírás</dt><dd>
Köbös (cubic) Bezier ív húzása.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| palidx | szín, paletta index 0-tól 255-ig |
| x0 | induló X koordináta pixelekben |
| y0 | induló Y koordináta pixelekben |
| x1 | vége X koordináta pixelekben |
| y1 | vége Y koordináta pixelekben |
| cx0 | első kontrollpont X koordinátája pixelekben |
| cy0 | első kontrollpont Y koordinátája pixelekben |
| cx1 | második kontrollpont X koordinátája pixelekben |
| cy1 | második kontrollpont Y koordinátája pixelekben |
</dd>
<dt>Lásd még</dt><dd>
[line], [qbez]
</dd>
<hr>
## tri

```c
void tri(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2)
```
<dt>Leírás</dt><dd>
Kirajzol egy háromszöget.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| palidx | szín, paletta index 0-tól 255-ig |
| x0 | első csúcs X koordináta pixelekben |
| y0 | első csúcs Y koordináta pixelekben |
| x1 | második csúcs X koordináta pixelekben |
| y1 | második csúcs Y koordináta pixelekben |
| x2 | harmadik csúcs X koordináta pixelekben |
| y2 | harmadik csúcs Y koordináta pixelekben |
</dd>
<dt>Lásd még</dt><dd>
[ftri], [tri2d], [tri3d]
</dd>
<hr>
## ftri

```c
void ftri(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2)
```
<dt>Leírás</dt><dd>
Kirajzol egy kitöltött háromszöget.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| palidx | szín, paletta index 0-tól 255-ig |
| x0 | első csúcs X koordináta pixelekben |
| y0 | első csúcs Y koordináta pixelekben |
| x1 | második csúcs X koordináta pixelekben |
| y1 | második csúcs Y koordináta pixelekben |
| x2 | harmadik csúcs X koordináta pixelekben |
| y2 | harmadik csúcs Y koordináta pixelekben |
</dd>
<dt>Lásd még</dt><dd>
[tri], [tri2d], [tri3d]
</dd>
<hr>
## tri2d

```c
void tri2d(uint8_t pi0, int16_t x0, int16_t y0,
    uint8_t pi1, int16_t x1, int16_t y1,
    uint8_t pi2, int16_t x2, int16_t y2)
```
<dt>Leírás</dt><dd>
Kirajzol egy kitöltött háromszöget színátmenetekkel.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| pi0 | első csúcs színe, paletta index 0-tól 255-ig |
| x0 | első csúcs X koordináta pixelekben |
| y0 | első csúcs Y koordináta pixelekben |
| pi1 | második csúcs színe, paletta index 0-tól 255-ig |
| x1 | második csúcs X koordináta pixelekben |
| y1 | második csúcs Y koordináta pixelekben |
| pi2 | harmadik csúcs színe, paletta index 0-tól 255-ig |
| x2 | harmadik csúcs X koordináta pixelekben |
| y2 | harmadik csúcs Y koordináta pixelekben |
</dd>
<dt>Lásd még</dt><dd>
[tri], [ftri], [tri3d]
</dd>
<hr>
## tri3d

```c
void tri3d(uint8_t pi0, int16_t x0, int16_t y0, int16_t z0,
    uint8_t pi1, int16_t x1, int16_t y1, int16_t z1,
    uint8_t pi2, int16_t x2, int16_t y2, int16_t z2)
```
<dt>Leírás</dt><dd>
Kirajzol egy háromszöget színátmenetekkel 3D-s térben, a kamera szemszögéből (lásd [Grafikus Feldolgozó Egység] 00498-as cím).
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| pi0 | első csúcs színe, paletta index 0-tól 255-ig |
| x0 | első csúcs X koordináta pixelekben |
| y0 | első csúcs Y koordináta pixelekben |
| z0 | első csúcs Z koordináta pixelekben |
| pi1 | második csúcs színe, paletta index 0-tól 255-ig |
| x1 | második csúcs X koordináta pixelekben |
| y1 | második csúcs Y koordináta pixelekben |
| z1 | második csúcs Z koordináta pixelekben |
| pi2 | harmadik csúcs színe, paletta index 0-tól 255-ig |
| x2 | harmadik csúcs X koordináta pixelekben |
| y2 | harmadik csúcs Y koordináta pixelekben |
| z2 | harmadik csúcs Z koordináta pixelekben |
</dd>
<dt>Lásd még</dt><dd>
[tri], [ftri], [tri2d]
</dd>
<hr>
## rect

```c
void rect(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
```
<dt>Leírás</dt><dd>
Kirajzol egy téglalapot.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| palidx | szín, paletta index 0-tól 255-ig |
| x0 | bal felső sarok X koordináta pixelekben |
| y0 | bal felső sarok Y koordináta pixelekben |
| x1 | jobb alsó sarok X koordináta pixelekben |
| y1 | jobb alsó sarok Y koordináta pixelekben |
</dd>
<dt>Lásd még</dt><dd>
[frect]
</dd>
<hr>
## frect

```c
void frect(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
```
<dt>Leírás</dt><dd>
Kirajzol egy kitöltött téglalapot.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| palidx | szín, paletta index 0-tól 255-ig |
| x0 | bal felső sarok X koordináta pixelekben |
| y0 | bal felső sarok Y koordináta pixelekben |
| x1 | jobb alsó sarok X koordináta pixelekben |
| y1 | jobb alsó sarok Y koordináta pixelekben |
</dd>
<dt>Lásd még</dt><dd>
[rect]
</dd>
<hr>
## circ

```c
void circ(uint8_t palidx, int16_t x, int16_t y, uint16_t r)
```
<dt>Leírás</dt><dd>
Kirajzol egy kört.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| palidx | szín, paletta index 0-tól 255-ig |
| x | középpont X koordináta pixelekben |
| y | középpont Y koordináta pixelekben |
| r | sugár pixelekben |
</dd>
<dt>Lásd még</dt><dd>
[fcirc], [ellip], [fellip]
</dd>
<hr>
## fcirc

```c
void fcirc(uint8_t palidx, int16_t x, int16_t y, uint16_t r)
```
<dt>Leírás</dt><dd>
Kirajzol egy kitöltött kört.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| palidx | szín, paletta index 0-tól 255-ig |
| x | középpont X koordináta pixelekben |
| y | középpont Y koordináta pixelekben |
| r | sugár pixelekben |
</dd>
<dt>Lásd még</dt><dd>
[circ], [ellip], [fellip]
</dd>
<hr>
## ellip

```c
void ellip(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
```
<dt>Leírás</dt><dd>
Kirajzol egy ellipszist.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| palidx | szín, paletta index 0-tól 255-ig |
| x0 | bal felső sarok X koordináta pixelekben |
| y0 | bal felső sarok Y koordináta pixelekben |
| x1 | jobb alsó sarok X koordináta pixelekben |
| y1 | jobb alsó sarok Y koordináta pixelekben |
</dd>
<dt>Lásd még</dt><dd>
[circ], [fcirc], [fellip]
</dd>
<hr>
## fellip

```c
void fellip(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
```
<dt>Leírás</dt><dd>
Kirajzol egy kitöltött ellipszist.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| palidx | szín, paletta index 0-tól 255-ig |
| x0 | bal felső sarok X koordináta pixelekben |
| y0 | bal felső sarok Y koordináta pixelekben |
| x1 | jobb alsó sarok X koordináta pixelekben |
| y1 | jobb alsó sarok Y koordináta pixelekben |
</dd>
<dt>Lásd még</dt><dd>
[circ], [fcirc], [ellip]
</dd>
<hr>
## move

```c
void move(int16_t x, int16_t y, uint16_t deg)
```
<dt>Leírás</dt><dd>
Elhelyezi a teknőst a képernyőn vagy az útvesztőben.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| x | X koordináta pixelben (vagy 1/128-ad csempében [maze] esetén) |
| y | Y koordináta pixelben |
| deg | irány fokokban, 0-tól 359-ig, 0 fok felfele van, 90 fok jobbra |
</dd>
<dt>Lásd még</dt><dd>
[left], [right], [up], [down], [color], [forw], [back]
</dd>
<hr>
## left

```c
void left(uint16_t deg)
```
<dt>Leírás</dt><dd>
Balra forgatja a teknőst.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| deg | változás fokokban, 0-tól 359-ig |
</dd>
<dt>Lásd még</dt><dd>
[move], [right], [up], [down], [color], [forw], [back]
</dd>
<hr>
## right

```c
void right(uint16_t deg)
```
<dt>Leírás</dt><dd>
Jobbra forgatja a teknőst.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| deg | változás fokokban, 0-tól 359-ig |
</dd>
<dt>Lásd még</dt><dd>
[move], [left], [up], [down], [color], [forw], [back]
</dd>
<hr>
## up

```c
void up(void)
```
<dt>Leírás</dt><dd>
Felemeli a teknős farkát. Ezután a teknős úgy mozog, hogy nem húz vonalat.
</dd>
<dt>Lásd még</dt><dd>
[move], [left], [right], [down], [color], [forw], [back]
</dd>
<hr>
## down

```c
void down(void)
```
<dt>Leírás</dt><dd>
Leteszi a teknős farkát. Ezután amikor a teknős mozog, vonalat húz (lásd [color]).
</dd>
<dt>Lásd még</dt><dd>
[move], [left], [right], [up], [color], [forw], [back]
</dd>
<hr>
## color

```c
void color(uint8_t palidx)
```
<dt>Leírás</dt><dd>
Beállítja a teknős farkának színét, amikor a teknős mozog, ilyen színű vonalat húz maga után.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| palidx | szín, paletta index 0-tól 255-ig |
</dd>
<dt>Lásd még</dt><dd>
[move], [left], [right], [up], [down], [forw], [back]
</dd>
<hr>
## forw

```c
void forw(uint16_t cnt)
```
<dt>Leírás</dt><dd>
Előre mozgatja a teknőst.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| cnt | mennyit, pixelekben (vagy 1/128-ad csempében [maze] esetén) |
</dd>
<dt>Lásd még</dt><dd>
[move], [left], [right], [up], [down], [color], [back]
</dd>
<hr>
## back

```c
void back(uint16_t cnt)
```
<dt>Leírás</dt><dd>
Hátra mozgatja a teknőst.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| cnt | mennyit, pixelekben (vagy 1/128-ad csempében [maze] esetén) |
</dd>
<dt>Lásd még</dt><dd>
[move], [left], [right], [up], [down], [color], [forw]
</dd>
<hr>
## spr

```c
void spr(int16_t x, int16_t y, uint16_t sprite, uint8_t sw, uint8_t sh, int8_t scale, uint8_t type)
```
<dt>Leírás</dt><dd>
Megjelenít egy, vagy akár több, egymásmelletti szprájtot.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| x | X koordináta pixelben |
| y | Y koordináta pixelben |
| sprite | szprájt index, 0-tól 1023-ig |
| sw | vízszintes szprájtok száma |
| sh | függőleges szprájtok száma |
| scale | nagyítás, -3-tól 4-ig |
| type | típus, 0=normális, 1=tükrözés függőlegesen, 2=tükrözés vízszintesen, 3=forgatás 90, 4=forgatás 180, 5=forgatás 270 |
</dd>
<dt>Lásd még</dt><dd>
[dlg], [stext]
</dd>
<hr>
## dlg

```c
void dlg(int16_t x, int16_t y, uint16_t w, uint16_t h, int8_t scale,
    uint16_t tl, uint16_t tm, uint16_t tr,
    uint16_t ml, uint16_t bg, uint16_t mr,
    uint16_t bl, uint16_t bm, uint16_t br)
```
<dt>Leírás</dt><dd>
Megjelenít egy dialógusablakot szprájtok használatával.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| x | X koordináta pixelben |
| y | Y koordináta pixelben |
| w | dialógusablak szélessége pixelekben |
| h | dialógusablak magassága pixelekben |
| scale | nagyítás, -3-tól 4-ig |
| tl | bal felső sarok szprájt id |
| tm | középső felső szprájt id |
| tr | jobb felső sarok szprájt id |
| ml | középső bal oldal szprájt id |
| bg | háttér szprájt id |
| mr | középső jobb oldal szprájt id |
| bl | bal alsó sarok szprájt id |
| bm | középső alsó szprájt id |
| br | jobb alsó sarok szprájt id |
</dd>
<dt>Lásd még</dt><dd>
[spr], [stext]
</dd>
<hr>
## stext

```c
void stext(int16_t x, int16_t y, uint16_t fs, uint16_t fu, uint8_t sw, uint8_t sh, int8_t scale, str_t str)
```
<dt>Leírás</dt><dd>
Szöveg megjelenítése a képernyőn szprájtok használatával.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| x | X koordináta pixelben |
| y | Y koordináta pixelben |
| fs | az első megjelenítendő szprájt indexe |
| fu | az első UNICODE (legkissebb lehetséges karakter) a sztringben |
| sw | vízszintes szprájtok száma |
| sh | függőleges szprájtok száma |
| scale | nagyítás, -3-tól 4-ig |
| str | nullával lezárt UTF-8 sztring |
</dd>
<dt>Lásd még</dt><dd>
[spr], [dlg]
</dd>
<hr>
## remap

```c
void remap(addr_t replace)
```
<dt>Leírás</dt><dd>
Lecseréli a csempéket a térképen. Használható arra, hogy meganimáljuk a térképet.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| replace | egy 256 elemű, szprájt idkat tartalmazó tömb |
</dd>
<dt>Lásd még</dt><dd>
[mget], [mset], [map], [maze]
</dd>
<hr>
## mget

```c
uint16_t mget(uint16_t mx, uint16_t my)
```
<dt>Leírás</dt><dd>
Visszaadja a térkép egy csempéjét.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| mx | X térképkoordináta csempékben |
| my | Y térképkoordináta csempékben |
</dd>
<dt>Visszatérési érték</dt><dd>
A megadott koordinátán lévő szprájt indexe.
</dd>
<dt>Lásd még</dt><dd>
[remap], [mset], [map], [maze]
</dd>
<hr>
## mset

```c
void mset(uint16_t mx, uint16_t my, uint16_t sprite)
```
<dt>Leírás</dt><dd>
Beállítja a térkép egy csempéjét.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| mx | X térképkoordináta csempékben |
| my | Y térképkoordináta csempékben |
| sprite | szprájt index, 0-tól 1023-ig |
</dd>
<dt>Lásd még</dt><dd>
[remap], [mget], [map], [maze]
</dd>
<hr>
## map

```c
void map(int16_t x, int16_t y, uint16_t mx, uint16_t my, uint16_t mw, uint16_t mh, int8_t scale)
```
<dt>Leírás</dt><dd>
Kirajzolja a térképet (vagy egy részét).
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| x | X koordináta pixelben |
| y | Y koordináta pixelben |
| mx | X térképkoordináta csempékben |
| my | Y térképkoordináta csempékben |
| mw | vízszintes csempék száma |
| mh | függőleges csempék száma |
| scale | nagyítás, -3-tól 4-ig |
</dd>
<dt>Lásd még</dt><dd>
[remap], [mget], [mset], [maze]
</dd>
<hr>
## maze

```c
void maze(uint16_t mx, uint16_t my, uint16_t mw, uint16_t mh, uint8_t scale,
    uint16_t sky, uint16_t grd, uint16_t door, uint16_t wall, uint16_t obj, uint8_t numnpc, addr_t npc)
```
<dt>Leírás</dt><dd>
A teknőc pozícióját használva 3D-s útvesztőként jeleníti meg a térképet.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| mx | X térképkoordináta csempékben |
| my | Y térképkoordináta csempékben |
| mw | vízszintes csempék száma |
| mh | függőleges csempék száma |
| scale | csempénkénti szprájtszám kettő hatványban, 0-tól 3-ig |
| sky | ég csempe index |
| grd | föld csempe index |
| door | első ajtó csempe indexe |
| wall | első fal csempe indexe |
| obj | első tárgy csempe indexe |
| numnpc | NJK rekordok száma |
| npc | uint32_t tömb, numnpc darab x,y,csempe index hármas |
</dd>
<dt>Lásd még</dt><dd>
[remap], [mget], [mset], [map]
</dd>

# Bemenet

## getpad

```c
int getpad(int pad, int btn)
```
<dt>Leírás</dt><dd>
Visszaadja az egyik játékpad egy gombjának állapotát.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| pad | játékpad index, 0-tól 3-ig |
| btn | az egyik [játékpad] gomb, `BTN_` |
</dd>
<dt>Visszatérési érték</dt><dd>
Nulla ha nincs lenyomva, nem nulla ha le van nyomva.
</dd>
<dt>Lásd még</dt><dd>
[prspad], [relpad], [getbtn], [getclk], [getkey]
</dd>
<hr>
## prspad

```c
int prspad(int pad, int btn)
```
<dt>Leírás</dt><dd>
Igaz értékkel tér vissza, ha a legutóbbi hívás óta le lett nyomva a játékpad gombja (press).
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| pad | játékpad index, 0-tól 3-ig |
| btn | az egyik [játékpad] gomb, `BTN_` |
</dd>
<dt>Visszatérési érték</dt><dd>
Nulla ha nem lett lenyomva, nem nulla ha le lett nyomva.
</dd>
<dt>Lásd még</dt><dd>
[relpad], [getpad], [getbtn], [getclk], [getkey]
</dd>
<hr>
## relpad

```c
int relpad(int pad, int btn)
```
<dt>Leírás</dt><dd>
Igaz értékkel tér vissza, ha a legutóbbi hívás óta fel lett engedve a játékpad gombja (release).
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| pad | játékpad index, 0-tól 3-ig |
| btn | az egyik [játékpad] gomb, `BTN_` |
</dd>
<dt>Visszatérési érték</dt><dd>
Nulla ha nem lett felengedve, nem nulla ha fel lett engedve.
</dd>
<dt>Lásd még</dt><dd>
[prspad], [getpad], [getbtn], [getclk], [getkey]
</dd>
<hr>
## getbtn

```c
int getbtn(int btn)
```
<dt>Leírás</dt><dd>
Visszaadja az egyik egérgomb állapotát.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| btn | az egyik [mutató] gomb, `BTN_` vagy `SCR_` |
</dd>
<dt>Visszatérési érték</dt><dd>
Nulla ha nincs lenyomva, nem nulla ha le van nyomva.
</dd>
<dt>Lásd még</dt><dd>
[prspad], [relpad], [getpad], [getclk], [getkey]
</dd>
<hr>
## getclk

```c
int getclk(int btn)
```
<dt>Leírás</dt><dd>
Visszaadja az egérgomb kattintást (click).
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| btn | az egyik [mutató] gomb, `BTN_` |
</dd>
<dt>Visszatérési érték</dt><dd>
Nulla ha nem kattintottak vele, nem nulla ha volt kattinttás.
</dd>
<dt>Lásd még</dt><dd>
[prspad], [relpad], [getpad], [getbtn], [getkey]
</dd>
<hr>
## getkey

```c
int getkey(int sc)
```
<dt>Leírás</dt><dd>
Visszaadja az egyik billentyű állapotát.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| sc | szkenkód, 1-től 144-ig, lásd [billentyűzet] |
</dd>
<dt>Visszatérési érték</dt><dd>
Nulla ha nincs lenyomva, nem nulla ha le van nyomva.
</dd>
<dt>Lásd még</dt><dd>
[prspad], [relpad], [getpad], [getbtn], [getclk]
</dd>
<hr>
## popkey

```c
uint32_t popkey(void)
```
<dt>Leírás</dt><dd>
Visszaadja a következő UTF-8 gombot a billentyűsorból. Lásd [billentyűzet].
</dd>
<dt>Visszatérési érték</dt><dd>
A gomb UTF-8 reprezentációja, vagy 0 ha a sor üres volt.
</dd>
<dt>Lásd még</dt><dd>
[pendkey], [lenkey], [speckey]
</dd>
<hr>
## pendkey

```c
int pendkey(void)
```
<dt>Leírás</dt><dd>
Igaz értékkel tér vissza, ha legalább egy billentyű várakozik a sorban (de benthagyja a sorban, nem veszi ki).
</dd>
<dt>Visszatérési érték</dt><dd>
1 ha van kiolvasatlan billentyű a sorban, egyébként 0 ha a sor üres.
</dd>
<dt>Lásd még</dt><dd>
[pendkey], [lenkey], [speckey]
</dd>
<hr>
## lenkey

```c
int lenkey(uint32_t key)
```
<dt>Leírás</dt><dd>
Visszaadja egy UTF-8 gomb hosszát bájtokban.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| key | a gomb, ahogy a billentyűsorból kijött |
</dd>
<dt>Visszatérési érték</dt><dd>
UTF-8 reprezentáció hossza bájtokban.
</dd>
<dt>Lásd még</dt><dd>
[popkey], [pendkey], [speckey]
</dd>
<hr>
## speckey

```c
int speckey(uint32_t key)
```
<dt>Leírás</dt><dd>
Igaz értékkel tér vissza, ha a gomb speciális.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| key | a gomb, ahogy a billentyűsorból kijött |
</dd>
<dt>Visszatérési érték</dt><dd>
1 ha a gomb speciális, és 0 ha megjeleníthető.
</dd>
<dt>Lásd még</dt><dd>
[popkey], [pendkey], [lenkey]
</dd>

# Matematika

## rand

```c
uint32_t rand(void)
```
<dt>Leírás</dt><dd>
Véletlenszám. A `%` modulo használatával méretezhető, például `1 + rand() % 6` 1 és 6 között ad vissza értéket, mint egy dobókocka.
</dd>
<dt>Visszatérési érték</dt><dd>
Egy véletlen szám 0 és 2^^32^^-1 (4294967295) között.
</dd>
<dt>Lásd még</dt><dd>
[rnd]
</dd>
<hr>
## rnd

```c
float rnd(void)
```
<dt>Leírás</dt><dd>
Véletlenszám. Ugyanaz, mint a [rand], csak lebegőpontos számot ad vissza.
</dd>
<dt>Visszatérési érték</dt><dd>
Egy véletlen szám 0.0 és 1.0 között.
</dd>
<dt>Lásd még</dt><dd>
[rand]
</dd>
<hr>
## float

```c
float float(int val)
```
<dt>Leírás</dt><dd>
Egy egészszám lebegőpontos alakját adja vissza.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| val | érték |
</dd>
<dt>Visszatérési érték</dt><dd>
Lebegőpontos szám.
</dd>
<dt>Lásd még</dt><dd>
[int]
</dd>
<hr>
## int

```c
int int(float val)
```
<dt>Leírás</dt><dd>
Egy lebegőpontos szám egészszám alakos változatát adja vissza.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| val | érték |
</dd>
<dt>Visszatérési érték</dt><dd>
Egészszám.
</dd>
<dt>Lásd még</dt><dd>
[float]
</dd>
<hr>
## floor

```c
float floor(float val)
```
<dt>Leírás</dt><dd>
Visszaadja a legnagyobb olyan egész lebegőpontos számot, ami még kissebb a megadottnál.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| val | érték |
</dd>
<dt>Visszatérési érték</dt><dd>
A szám lefele kerekítve.
</dd>
<dt>Lásd még</dt><dd>
[ceil]
</dd>
<hr>
## ceil

```c
float ceil(float val)
```
<dt>Leírás</dt><dd>
Visszaadja a legkissebb olyan egész lebegőpontos számot, ami már nagyobb a megadottnál.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| val | érték |
</dd>
<dt>Visszatérési érték</dt><dd>
A szám felfele kerekítve.
</dd>
<dt>Lásd még</dt><dd>
[floor]
</dd>
<hr>
## sgn

```c
float sgn(float val)
```
<dt>Leírás</dt><dd>
Visszaadja a szám előjelét.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| val | érték |
</dd>
<dt>Visszatérési érték</dt><dd>
Vagy 1.0 vagy -1.0.
</dd>
<dt>Lásd még</dt><dd>
[abs]
</dd>
<hr>
## abs

```c
float abs(float val)
```
<dt>Leírás</dt><dd>
Visszaadja a lebegőpontos szám abszolút értékét.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| val | érték |
</dd>
<dt>Visszatérési érték</dt><dd>
Vagy az érték, vagy -érték, mindig pozitív.
</dd>
<dt>Lásd még</dt><dd>
[sgn]
</dd>
<hr>
## exp

```c
float exp(float val)
```
<dt>Leírás</dt><dd>
Visszaadja az érték exponenciálisát, azaz a természetes alapú logaritmus értékedik hatványát.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| val | érték |
</dd>
<dt>Visszatérési érték</dt><dd>
Az e^^val^^ értéke.
</dd>
<dt>Lásd még</dt><dd>
[log], [pow]
</dd>
<hr>
## log

```c
float log(float val)
```
<dt>Leírás</dt><dd>
Visszaadja az érték természetes alapú logaritmusát.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| val | érték |
</dd>
<dt>Visszatérési érték</dt><dd>
A val természetes alapú logaritmusa.
</dd>
<dt>Lásd még</dt><dd>
[exp]
</dd>
<hr>
## pow

```c
float pow(float val, float exp)
```
<dt>Leírás</dt><dd>
Visszaadja egy szám hatványát. Ez nagyon lassú, próbáld elkerülni a használatát.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| val | érték |
| exp | kitevő |
</dd>
<dt>Visszatérési érték</dt><dd>
A val^^exp^^ értéke.
</dd>
<dt>Lásd még</dt><dd>
[exp], [sqrt], [rsqrt]
</dd>
<hr>
## sqrt

```c
float sqrt(float val)
```
<dt>Leírás</dt><dd>
Visszaadja egy szám négyzetgyökét. Ez nagyon lassú, próbáld elkerülni a használatát.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| val | érték |
</dd>
<dt>Visszatérési érték</dt><dd>
Az érték négyzetgyöke.
</dd>
<dt>Lásd még</dt><dd>
[pow], [rsqrt]
</dd>
<hr>
## rsqrt

```c
float rsqrt(float val)
```
<dt>Leírás</dt><dd>
Visszaadja egy szám négyzetgyökének reciprokát (1/sqrt(val)). John Carmack féle gyors metódust használ.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| val | érték |
</dd>
<dt>Visszatérési érték</dt><dd>
Az érték négyzetgyökének reciproka.
</dd>
<dt>Lásd még</dt><dd>
[pow], [sqrt]
</dd>
<hr>
## pi

```c
float pi(void)
```
<dt>Leírás</dt><dd>
Visszaadja a π értékét lebegőpontos számként.
</dd>
<dt>Visszatérési érték</dt><dd>
A 3.14159265358979323846 érték.
</dd>
<hr>
## cos

```c
float cos(uint16_t deg)
```
<dt>Leírás</dt><dd>
Visszaadja a koszinuszt.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| deg | fok, 0-tól 359-ig, 0 felfele, 90 jobbra |
</dd>
<dt>Visszatérési érték</dt><dd>
A fok koszinusza, -1.0 és 1.0 közötti érték.
</dd>
<dt>Lásd még</dt><dd>
[sin], [tan], [acos], [asin], [atan], [atan2]
</dd>
<hr>
## sin

```c
float sin(uint16_t deg)
```
<dt>Leírás</dt><dd>
Visszaadja a szinuszt.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| deg | fok, 0-tól 359-ig, 0 felfele, 90 jobbra |
</dd>
<dt>Visszatérési érték</dt><dd>
A fok szinusza, -1.0 és 1.0 közötti érték.
</dd>
<dt>Lásd még</dt><dd>
[cos], [tan], [acos], [asin], [atan], [atan2]
</dd>
<hr>
## tan

```c
float tan(uint16_t deg)
```
<dt>Leírás</dt><dd>
Visszaadja a tangenst.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| deg | fok, 0-tól 359-ig, 0 felfele, 90 jobbra |
</dd>
<dt>Visszatérési érték</dt><dd>
A fok tangense, -1.0 és 1.0 közötti érték.
</dd>
<dt>Lásd még</dt><dd>
[cos], [sin], [acos], [asin], [atan], [atan2]
</dd>
<hr>
## acos

```c
uint16_t acos(float val)
```
<dt>Leírás</dt><dd>
Visszaadja az arkuszkoszinuszt.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| val | érték, -1.0 és 1.0 között |
</dd>
<dt>Visszatérési érték</dt><dd>
Arkuszkoszinusz fokokban, 0-tól 359-ig, 0 felfele, 90 jobbra.
</dd>
<dt>Lásd még</dt><dd>
[cos], [sin], [tan], [asin], [atan], [atan2]
</dd>
<hr>
## asin

```c
uint16_t asin(float val)
```
<dt>Leírás</dt><dd>
Visszaadja az arkuszszinuszt.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| val | érték, -1.0 és 1.0 között |
</dd>
<dt>Visszatérési érték</dt><dd>
Arkuszszinusz fokokban, 0-tól 359-ig, 0 felfele, 90 jobbra.
</dd>
<dt>Lásd még</dt><dd>
[cos], [sin], [tan], [acos], [atan], [atan2]
</dd>
<hr>
## atan

```c
uint16_t atan(float val)
```
<dt>Leírás</dt><dd>
Visszaadja az arkusztangenst.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| val | érték, -1.0 és 1.0 között |
</dd>
<dt>Visszatérési érték</dt><dd>
Arkusztangens fokokban, 0-tól 359-ig, 0 felfele, 90 jobbra.
</dd>
<dt>Lásd még</dt><dd>
[cos], [sin], [tan], [acos], [asin], [atan2]
</dd>
<hr>
## atan2

```c
uint16_t atan2(float y, float x)
```
<dt>Leírás</dt><dd>
Visszaadja y/x arkusztangensét, figyelembe véve az y és x előjelét a kvadráns meghatározásánál.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| y | Y koordináta |
| x | X koordináta |
</dd>
<dt>Visszatérési érték</dt><dd>
Arkusztangens fokokban, 0-tól 359-ig, 0 felfele, 90 jobbra.
</dd>
<dt>Lásd még</dt><dd>
[cos], [sin], [tan], [acos], [asin]
</dd>

# Memória

## inb

```c
uint8_t inb(addr_t src)
```
<dt>Leírás</dt><dd>
Beolvas egy bájtot a memóriából.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| src | cím, 0x00000-tól 0xBFFFF-ig |
</dd>
<dt>Visszatérési érték</dt><dd>
Visszaadja az értéket az adott címről.
</dd>
<hr>
## inw

```c
uint16_t inw(addr_t src)
```
<dt>Leírás</dt><dd>
Beolvas egy szót (word, 2 bájt) a memóriából.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| src | cím, 0x00000-tól 0xBFFFE-ig |
</dd>
<dt>Visszatérési érték</dt><dd>
Visszaadja az értéket az adott címről.
</dd>
<hr>
## ini

```c
uint32_t ini(addr_t src)
```
<dt>Leírás</dt><dd>
Beolvas egy egészszámot (integer, 4 bájt) a memóriából.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| src | cím, 0x00000-tól 0xBFFFC-ig |
</dd>
<dt>Visszatérési érték</dt><dd>
Visszaadja az értéket az adott címről.
</dd>
<hr>
## outb

```c
void outb(addr_t dst, uint8_t value)
```
<dt>Leírás</dt><dd>
Kiír egy bájtot a memóriába.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| dst | cím, 0x00000-tól 0xBFFFF-ig |
| value | beállítandó érték, 0-tól 255-ig |
</dd>
<hr>
## outw

```c
void outw(addr_t dst, uint16_t value)
```
<dt>Leírás</dt><dd>
Kiír egy szót (word, 2 bájt) a memóriába.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| dst | cím, 0x00000-tól 0xBFFFE-ig |
| value | beállítandó érték, 0-tól 65536-ig |
</dd>
<hr>
## outl

```c
void outi(addr_t dst, uint32_t value)
```
<dt>Leírás</dt><dd>
Kiír egy egészszámot (integer, 4 bájt) a memóriába.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| dst | cím, 0x00000-tól 0xBFFFC-ig |
| value | beállítandó érték, 0-tól 4294967295-ig |
</dd>
<hr>
## memsave

```c
int memsave(uint8_t overlay, addr_t src, uint32_t size)
```
<dt>Leírás</dt><dd>
Elmenti a megadott memória tartalmát egy átfedőbe.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| overlay | cél átfedő indexe, 0-tól 255-ig |
| src | elmentendő memória címe, 0x00000-tól 0xBFFFF-ig |
| size | elmentendő bájtok száma |
</dd>
<dt>Visszatérési érték</dt><dd>
1-et ad vissza ha sikerült, 0-át hiba esetén.
</dd>
<dt>Lásd még</dt><dd>
[memload]
</dd>
<hr>
## memload

```c
int memload(addr_t dst, uint8_t overlay, uint32_t maxsize)
```
<dt>Leírás</dt><dd>
Betölti egy átfedő tartalmát a memória megadott címére.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| dst | cél memória terület címe, 0x00000-tól 0xBFFFF-ig |
| overlay | betöltendő átfedő indexe, 0-tól 255-ig |
| maxsize | betöltendő bájtok maximális száma |
</dd>
<dt>Visszatérési érték</dt><dd>
Visszaadja, hogy hány bájtot töltött végül be (ami lehet kevesebb, mint a maxsize).
</dd>
<dt>Lásd még</dt><dd>
[memsave]
</dd>
<hr>
## memcpy

```c
void memcpy(addr_t dst, addr_t src, uint32_t len)
```
<dt>Leírás</dt><dd>
Memóriaterületek másolása.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| dst | cél címe, 0x00000-tól 0xBFFFF-ig |
| src | forrás címe, 0x00000-tól 0xBFFFF-ig |
| len | átmásolandó bájtok száma |
</dd>
<hr>
## memset

```c
void memset(addr_t dst, uint8_t value, uint32_t len)
```
<dt>Leírás</dt><dd>
Memóriaterület feltöltése adott bájttal.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| dst | cél címe, 0x00000-tól 0xBFFFF-ig |
| value | beállítandó érték, 0-tól 255-ig |
| len | beállítandó bájtok száma |
</dd>
<hr>
## memcmp

```c
int memcmp(addr_t addr0, addr_t addr1, uint32_t len)
```
<dt>Leírás</dt><dd>
Két memóriaterület összehasonlítása.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| addr0 | első cím, 0x00000-tól 0xBFFFF-ig |
| addr1 | második cím, 0x00000-tól 0xBFFFF-ig |
| len | összehasonlítandó bájtok száma |
</dd>
<dt>Visszatérési érték</dt><dd>
Visszaadja a különbséget, azaz 0-át, ha a két terület tartalma megegyezik.
</dd>
<hr>
## deflate

```c
int deflate(addr_t dst, addr_t src, uint32_t len)
```
<dt>Leírás</dt><dd>
Betömörít (összezsugorít) egy adatbuffert RFC1950 deflate (zlib) használatával.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| dst | cél címe, 0x30000-tól 0xBFFFF-ig |
| src | forrás címe, 0x30000-tól 0xBFFFF-ig |
| len | tömörítendő bájtok száma |
</dd>
<dt>Visszatérési érték</dt><dd>
0 vagy negatív hiba esetén, egyébként a betömörített bájtok száma és a betömörített adat a dst-ben.
</dd>
<dt>Lásd még</dt><dd>
[inflate]
</dd>
<hr>
## inflate

```c
int inflate(addr_t dst, addr_t src, uint32_t len)
```
<dt>Leírás</dt><dd>
Kitömörít (felfúj) egy RFC1950 deflate (zlib) tömörített buffert.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| dst | cél címe, 0x30000-tól 0xBFFFF-ig |
| src | forrás címe, 0x30000-tól 0xBFFFF-ig |
| len | betömörített bájtok száma |
</dd>
<dt>Visszatérési érték</dt><dd>
0 vagy negatív hiba esetén, egyébként a kitömörített bájtok száma és a kitömörített adat a dst-ben.
</dd>
<dt>Lásd még</dt><dd>
[deflate]
</dd>
<hr>
## time

```c
float time(void)
```
<dt>Leírás</dt><dd>
Visszaadja a bekapcsolás óta eltelt tikkek számát.
</dd>
<dt>Visszatérési érték</dt><dd>
A bekapcsolás óta eltelt idő ezredmásodpercekben.
</dd>
<dt>Lásd még</dt><dd>
[now]
</dd>
<hr>
## now

```c
uint32_t now(void)
```
<dt>Leírás</dt><dd>
Visszaadja a UNIX időbélyeget. A 0000C címen lévő bájttal ellenőrizheted, hogy túlcsordult-e.
</dd>
<dt>Visszatérési érték</dt><dd>
A greenwichi középidő szerinti 1970. január 1.-e éjfél óta eltelt másodpercek száma.
</dd>
<dt>Lásd még</dt><dd>
[time]
</dd>
<hr>
## atoi

```c
int atoi(str_t src)
```
<dt>Leírás</dt><dd>
Egy ASCII decimális számot alakít át egészszámmá.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| src | sztring címe, 0x00000-tól 0xBFFFF-ig |
</dd>
<dt>Visszatérési érték</dt><dd>
A sztringbeli szám egészszám megfelelője.
</dd>
<dt>Lásd még</dt><dd>
[itoa], [str], [val]
</dd>
<hr>
## itoa

```c
str_t itoa(int value)
```
<dt>Leírás</dt><dd>
Egy egésszámot alakít ASCII decimális karaktersorozattá, sztringé.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| value | az érték, -2147483648-tól 2147483647-ig |
</dd>
<dt>Visszatérési érték</dt><dd>
A szám sztringbe kiírt változata.
</dd>
<dt>Lásd még</dt><dd>
[atoi], [str], [val]
</dd>
<hr>
## val

```c
float val(str_t src)
```
<dt>Leírás</dt><dd>
Egy ASCII decimális számot alakít át lebegőpontos számmá.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| src | sztring címe, 0x00000-tól 0xBFFFF-ig |
</dd>
<dt>Visszatérési érték</dt><dd>
A sztringbeli szám lebegőpontos szám megfelelője.
</dd>
<dt>Lásd még</dt><dd>
[itoa], [atoi], [str]
</dd>
<hr>
## str

```c
str_t str(float value)
```
<dt>Leírás</dt><dd>
Egy lebegőpontos számot alakít ASCII decimális karaktersorozattá, sztringé.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| value | a szám |
</dd>
<dt>Visszatérési érték</dt><dd>
A szám sztringbe kiírt változata.
</dd>
<dt>Lásd még</dt><dd>
[atoi], [itoa], [val]
</dd>
<hr>
## sprintf

```c
str_t sprintf(str_t fmt, ...)
```
<dt>Leírás</dt><dd>
Kigenerál egy nullával lezárt, UTF-8 sztringet a megadott formázás és paraméterek alapján.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| fmt | [formázó sztring] |
| ... | opcionális paraméterek |
</dd>
<dt>Visszatérési érték</dt><dd>
A megformázott sztring.
</dd>
<hr>
## strlen

```c
int strlen(str_t src)
```
<dt>Leírás</dt><dd>
Visszaadja, mennyi bájtból áll a sztring (a lezáró nulla nélkül).
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| src | sztring címe, 0x00000-tól 0xBFFFF-ig |
</dd>
<dt>Visszatérési érték</dt><dd>
A sztringben lévő bájtok száma.
</dd>
<dt>Lásd még</dt><dd>
[mblen]
</dd>
<hr>
## mblen

```c
int mblen(str_t src)
```
<dt>Leírás</dt><dd>
Visszaadja, mennyi UTF-8 multibájt karakterből áll a sztring (a lezáró nulla nélkül).
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| src | sztring címe, 0x00000-tól 0xBFFFF-ig |
</dd>
<dt>Visszatérési érték</dt><dd>
A sztringben lévő karakterek száma.
</dd>
<dt>Lásd még</dt><dd>
[strlen]
</dd>
<hr>
## malloc

```c
addr_t malloc(uint32_t size)
```
<dt>Leírás</dt><dd>
Dinamikusan lefoglalja a memória egy részét.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| size | lefoglalandó bájtok száma |
</dd>
<dt>Visszatérési érték</dt><dd>
Az újonnan allokált buffer címe, vagy NULL hiba esetén.
</dd>
<dt>Lásd még</dt><dd>
[realloc], [free]
</dd>
<hr>
## realloc

```c
addr_t realloc(addr_t addr, uint32_t size)
```
<dt>Leírás</dt><dd>
Átméretez egy korábban lefoglalt buffert.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| addr | az allokált buffer címe |
| size | bájtok száma, amire átméretez |
</dd>
<dt>Visszatérési érték</dt><dd>
Az újonnan allokált buffer címe, vagy NULL hiba esetén.
</dd>
<dt>Lásd még</dt><dd>
[malloc], [free]
</dd>
<hr>
## free

```c
int free(addr_t addr)
```
<dt>Leírás</dt><dd>
Felszabadítja a dinamikusan lefoglalt memóriát.
</dd>
<dt>Paraméterek</dt><dd>
| Paraméter | Leírás |
| addr | az allokált buffer címe |
</dd>
<dt>Visszatérési érték</dt><dd>
1 siker esetén, 0 ha hiba történt.
</dd>
<dt>Lásd még</dt><dd>
[malloc], [realloc]
</dd>
