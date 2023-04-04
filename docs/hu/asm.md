Assembly
========

Ha ezt a nyelvet választod, akkor kezd a programodat az `#!asm` sorral.

<h2 ex_asm>Példa program</h2>

```bc
#!asm

    .data
/* globális változók */
számláló:   di 123
szám:       df 3.1415
cím:        di 0x0048C
sztring:    db "valami"
tömb:       di 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
fmt:        db "a számláló %d, balshift %d\n"

    .code
/* Induláskor lefuttatandó dolgok */
setup:
    /* lokális változó (igazából nem, csak helyet foglalunk a veremben) */
    sp -4
    ret

/* Minden képkockánál lefuttatandó dolgok, 60 FPS */
loop:
    /* MEG-4 stílusú kimenet */
    pshci KEY_LSHIFT
    scall getkey
    sp 4
    pushi
    ci számláló
    ldi
    pushi
    pshci fmt
    scall printf
    sp 12
    ret
```

<h2 asm_desc>Leírás</h2>

Ez igazából nem egy programozási nyelv. Amikor valamelyik beépített nyelvet lefordítod, akkor a fordító olyan bájtkódot generál,
amit aztán a CPU végrehajt. Az Assembly ezeknek a bájtkódoknak az egy-az-egybeni, ember számára is olvasható, szöveges mnemonik
átirata. Két szekciója van, adat és kód, mindkettő cimkék és utasítások tetszőleges sorozatából áll. Az utasítás egy mnemonik
opcionális paraméterrel.

Játszhatsz és kísérletezhetsz ezzel, ha bátornak érzed magad.

<h2 asm_lit>Literálok</h2>

Pontosan ugyanazok, mint a [MEG-4 C literáljai](#c_lit).

<h2 asm_var>Változók</h2>

Assemblyben nincs olyan, hogy változó. Helyette a `.data` kulcsszó indítja az adat szekciót, amibe csak adatokat pakolsz a
`db` (bájt), `dw` (word, szó), `di` (integer, egészszám), `df` (float, lebegőpontos szám) utasítások paramétereként. Ebbe az
adatfolyamba az utasítások elé rakhatsz címkéket, amik az adott adat címét fogják jelenteni. A betöltéshez a kód szekcióban,
előbb be kell tölteni az akkumulátor regiszterbe egy ilyen címkét a `ci` utasítással, majd pedig kiadni a `ldb`
(load, bájt betöltése), `ldw` (szó betöltése), `ldi` (egészszám betöltése) vagy `ldf` (lebegőpontos szám betöltése) utasítás
valamelyikét.

<h2 asm_flow>Vezérlésirányítás</h2>

Minden, ami a `.code` kulcsszó után kerül, kód lesz. Nincs vezérlésirányítás, minden utasítás sorjában, egymás után kerül
végrehajtásra; kézzel kell átállítanod a PC (program counter, programszámláló) regisztert, hogy befolyásold ezt. Ehhez a `jmp`
(jump, ugrás), `jz` (jump if zero, ugrás ha nulla), `jnz` (jump if not zero, ugrás ha nem nulla) vagy az `sw` (switch,
esetválasztás) utasításokat használhatod.

<h2 asm_func>Függvények</h2>

Nincs olyan, hogy függvénydeklaráció. Csak megadsz egy címkét, hogy megjelöld a kód adott pontját. Hívásnál lerakod a paramétereket
a verembe *fordított sorrendben* a `pushi` és `pushf` utasításokkal, majd a `call` mnemoniknak paraméterül ezt a címkét adod.
A függvényen belül a paraméterek címét az `adr` (address, cím) utasítással lehet lekérni, aminek a paramétere a függvényparaméter
sorszáma szorozva néggyel. Például `adr 0` az első függvényparaméter címét rakja az akkumulátor regiszterbe, az `adr 4` a
másodikét. Visszatérni egy függvényből a `ret` utasítással lehet. A visszatérési érték az akkumulátor regiszterben adódik át,
amit beállíthatsz direktben a `ci` (constant integer, egészszám konstans) és `cf` (constant float, lebegőpontos konstans)
utasításokkal, illetve indirektben a `popi`, `popf`, `ldb`, `ldw`, `ldi` és `ldf` utasításokkal. Hívás után mindig a hívó fél
felelőssége a paraméterek eltávolítása a veremből az `sp`+paraméterek száma szorozva néggyel utasítással.

<h2 asm_api>Elérhető függvények</h2>

Minden MEG-4 API függvény a rendelkezésedre áll; pontosan azokkal a nevekkel, ahogy ebben a leírásban szerepelnek.

Lerakod a paramétereket a verembe *fordított sorrendben*, majd az `scall` (system call, rendszerhívás) mnemoniknak az egyik MEG-4
API funkció nevét adod paraméterül. Hívás után mindig a hívó fél felelőssége a paraméterek eltávolítása a veremből.

Mnemonikok
----------

NOTE: Ha extra szószátyár `meg4 -vvv` módban indítottad, akkor minden éppen végrehajtott utasítás kiíródik a szabványos kimenetre.

Mielőtt belemennénk a részletekbe, muszáj pár szót ejteni a MEG-4 CPU specifikációjáról.

A MEG-4 CPU egy 32 bites, kicsi elöl (little endian) CPU. Minden érték a memóriában úgy van tárolva, hogy az alacsonyabb
helyiérték van az alacsonyabb címen. Képes műveleteket végezni 8 bites, 16 bites és 32 bites egészszámokkal (integer, előjeles
és előjel nélküli is), valamint 32 bites lebegőpontos számokkal.

A memóriamodellje sík, ami azt jelenti, hogy minden adat elérhető egy egyszerű offszettel. Nincs lapozás se virtuális
címfordítás, se szegmentálás, kivéve, hogy az adat- és kódszegmens hivatkozás implicit (azaz nem kell előtagot kiírni, a
szegmensre hivatkozás automatikus).

Biztonsági okokból a kódszegmens és az adatszegmens el van különítve, akárcsak a hívásverem és az adatverem. Veremtúlcsordulásos
támadás és egyéb buffertúlcsordulásos kód injektálás egyszerűen nem lehetséges ezen a CPU-n, ami különösen biztonságossá és
hülyebiztossá teszi (továbbá a kódszeparálás nélkül lehetetlen lenne harmadik fél bájtkódját integrálni, lásd [Lua]). E
tekintetben inkább Harvard architektúrájú, viszont minden másban inkább Neumann architektúrájú.

A CPU a következő regiszterekkel rendelkezik:

- AC: akkumulátor regiszter, egészszám értékkel
- AF: akkumulátor regiszter, lebegőpontos értékkel
- FLG: a processzor jelzőbitjei (setup lefutott, B/K-ra illetve időzítőre várakozik, futtatás megszakítva)
- TMR: az időzítő regiszter aktuális értéke
- DP: adatmutató (data pointer), a felhasznált globális változó memória tetejére mutat
- BP: bázismutató (base pointer), ez jelzi, hol van a függvényveremkeret teteje
- SP: veremmutató (stack pointer), ez mutatja a verem alját
- CP: hívásveremmutató (callstack pointer), a hívásverem tetejére mutat
- PC: programszámláló (program counter), ez az éppen aktuálisan végrehajtott utasításra mutat

Az adatszegmens bájt alapú, azaz a DP, BP és az SP regiszterek 8 bites egységekre mutatnak. Az adatszegmensre a `db` (8 bit),
`dw` (16 bit), `di` (32 bit) és `df` (32 bit float, lebegőpontos) menmonikokkal lehet adatokat elhelyezni. Ezeknek egy vagy akár
több vesszővel elválaszott paraméterük is lehet, továbbá a `db` esetén sztring literál és karakter literál is használható.

A kódszegmens felbontása 32 bit, azaz ez a legkissebb címezhető egység. Emiatt a PC ilyen 32 bites egységekre mutat és nem
bájtokra. A kódszegmensre a következő mnemonikokkal lehet utasításokat elhelyezni:

| Mnemonik | Paraméter               | Leírás                                                                              |
|----------|-------------------------|-------------------------------------------------------------------------------------|
| `debug`  |                         | Meghívja a beépített [debuggoló]t (MEG-4 PRO alatt nop)                             |
| `ret`    |                         | Visszatérés `call` hívásból (return), kivesz a hívásveremből                        |
| `scall`  | MEG-4 API funckció      | Rendszerhívás (system call)                                                         |
| `call`   | cím/kódcimke            | Beteszi a pozíciót a hívásverembe, majd meghívja a függvényt                        |
| `jmp`    | cím/kódcimke            | Ugrás a megadott címre                                                              |
| `jz`     | cím/kódcimke            | Ugrás a megadott címre, ha az akkumulátor nulla                                     |
| `jnz`    | cím/kódcimke            | Ugrás a megadott címre, ha az akkumulátor nem nulla                                 |
| `js`     | cím/kódcimke            | A veremből kivett értéket előjeligazítja, és ugrik, ha negatív vagy nulla           |
| `jns`    | cím/kódcimke            | A veremből kivett értéket előjeligazítja, és ugrik, ha pozitív                      |
| `sw`     | szám,cím,cím0,cím1...   | Esetválasztás (switch, lásd alább)                                                  |
| `ci`     | szám/adat cimke         | Egészszámot helyez az akkumulátorba                                                 |
| `cf`     | szám                    | Lebegőpontos számot helyez az akkumulátorba                                         |
| `bnd`    | szám                    | Ellenőrzi, hogy az akkumulátor tartalma 0 és szám közötti-e                         |
| `lea`    | szám                    | A DP + szám címet tölti az akkumulátorba                                            |
| `adr`    | szám                    | A BP + szám címet tölti az akkumulátorba                                            |
| `sp`     | szám                    | Hozzáadja a számot az SP regiszterhez                                               |
| `pshci`  | szám/adat cimke         | Egészszám konstanst tol az adatverembe                                              |
| `pshcf`  | szám                    | Lebegőpontos konstanst tol az adatverembe                                           |
| `pushi`  |                         | Az egészszám akkumulátor értékét az adatverembe tolja                               |
| `pushf`  |                         | Az lebegőpontos akkumulátor értékét az adatverembe tolja                            |
| `popi`   |                         | Kiveszi az adatverem legfelső elemét az egészszám akkumulátorba                     |
| `popf`   |                         | Kiveszi az adatverem legfelső elemét a lebegőpontos akkumulátorba                   |
| `cnvi`   |                         | Az adatverem legfelső elemét egészszámmá konvertálja                                |
| `cnvf`   |                         | Az adatverem legfelső elemét lebegőpontos számmá konvertálja                        |
| `ldb`    |                         | Az akkumulátor által mutatott adatcímről egy bájtot tölt be                         |
| `ldw`    |                         | Az akkumulátor által mutatott adatcímről egy szót tölt be                           |
| `ldi`    |                         | Az akkumulátor által mutatott adatcímről egy egészszámot tölt be                    |
| `ldf`    |                         | Az akkumulátor által mutatott adatcímről egy lebegőpontost tölt be                  |
| `stb`    |                         | Kiveszi a címet a veremből, és egy bájtot rak oda az akkumulátorból                 |
| `stw`    |                         | Kiveszi a címet a veremből, és egy szót rak oda az akkumulátorból                   |
| `sti`    |                         | Kiveszi a címet a veremből, és egy egészszámot rak oda az akkumulátorból            |
| `stf`    |                         | Kiveszi a címet a veremből, és egy lebegőpontost rak oda az akkumulátorból          |
| `incb`   | szám                    | Kiveszi a címet a veremből, és számmal növeli a bájtot a címen                      |
| `incw`   | szám                    | Kiveszi a címet a veremből, és számmal növeli a szót a címen                        |
| `inci`   | szám                    | Kiveszi a címet a veremből, és számmal növeli az egészszámot a címen                |
| `decb`   | szám                    | Kiveszi a címet a veremből, és számmal csökkenti a bájtot a címen                   |
| `decw`   | szám                    | Kiveszi a címet a veremből, és számmal csökkenti a szót a címen                     |
| `deci`   | szám                    | Kiveszi a címet a veremből, és számmal csökkenti az egészszámot a címen             |
| `not`    |                         | Logikai NEM művelet végzése az akkumulátoron                                        |
| `neg`    |                         | Bitenkénti NEM művelet végzése az akkumulátoron (negálás)                           |
| `or`     |                         | A veremből kivett értékkel bitenkénti VAGY végzése az akkumulátoron                 |
| `xor`    |                         | A veremből kivett értékkel KIZÁRÓ VAGY végzése az akkumulátoron                     |
| `and`    |                         | A veremből kivett értékkel bitenkénti ÉS végzése az akkumulátoron                   |
| `shl`    |                         | A veremből kivett értéket eltolja akkumlátornyi bittel balra, az eredmény az akkumulátorba kerül |
| `shr`    |                         | A veremből kivett értéket eltolja akkumlátornyi bittel jobbra, az eredmény az akkumulátorba kerül |
| `eq`     |                         | Beállítja az akkumulátort ha veremből kivett érték egyezik az akkumulátorral        |
| `ne`     |                         | Beállítja az akkumulátort ha veremből kivett érték nem egyezik az akkumulátorral    |
| `lts`    |                         | Beállítja az akkumulátort ha veremből kivett érték előjelesen kissebb               |
| `gts`    |                         | Beállítja az akkumulátort ha veremből kivett érték előjelesen nagyobb               |
| `les`    |                         | Beállítja az akkumulátort ha veremből kivett érték előjelesen kissebb vagy egyenlő  |
| `ges`    |                         | Beállítja az akkumulátort ha veremből kivett érték előjelesen nagyobb vagy egyenlő  |
| `ltu`    |                         | Beállítja az akkumulátort ha veremből kivett érték előjel nélkül kissebb            |
| `gtu`    |                         | Beállítja az akkumulátort ha veremből kivett érték előjel nélkül nagyobb            |
| `leu`    |                         | Beállítja az akkumulátort ha veremből kivett érték előjel nélkül kissebb vagy egyenlő |
| `geu`    |                         | Beállítja az akkumulátort ha veremből kivett érték előjel nélkül nagyobb vagy egyenlő |
| `ltf`    |                         | Beállítja az akkumulátort ha veremből kivett érték lebegőpontosan kissebb           |
| `gtf`    |                         | Beállítja az akkumulátort ha veremből kivett érték lebegőpontosan nagyobb           |
| `lef`    |                         | Beállítja az akkumulátort ha veremből kivett érték lebegőpontosan kissebb vagy egyenlő |
| `gef`    |                         | Beállítja az akkumulátort ha veremből kivett érték lebegőpontosan nagyobb vagy egyenlő |
| `addi`   |                         | Kivesz a veremből és hozzáadja az akkumulátort, egészszámként                       |
| `subi`   |                         | Kivesz a veremből és kivonja belőle az akkumulátort, egészszámként                  |
| `muli`   |                         | Kivesz a veremből és megszorozza az akkumulátorral, egészszámként                   |
| `divi`   |                         | Kivesz a veremből és elosztja az akkumulátorral, egészszámként                      |
| `modi`   |                         | Kivesz a veremből, elosztja és a maradék az akkumulátorba kerül, egészszámként      |
| `powi`   |                         | Kivesz a veremből és az akkumulátoradik hatványra emeli, egészszámként              |
| `addf`   |                         | Kivesz a veremből és hozzáadja az akkumulátort, lebegőpontosként                    |
| `subf`   |                         | Kivesz a veremből és kivonja belőle az akkumulátort, lebegőpontosként               |
| `mulf`   |                         | Kivesz a veremből és megszorozza az akkumulátorral, lebegőpontosként                |
| `divf`   |                         | Kivesz a veremből és elosztja az akkumulátorral, lebegőpontosként                   |
| `modf`   |                         | Kivesz a veremből, elosztja és a tizedes az akkumulátorba kerül, lebegőpontosként   |
| `powf`   |                         | Kivesz a veremből és az akkumulátoradik hatványra emeli, lebegőpontosként           |

Az `sw` mnemonik változó számú (de legalább három) paraméterrel rendelkezik. Az első paramétere egy szám, a második egy kódcimke,
és a többi is mind kódcimke. Az akkumulátorból kivonja a megadott számot, majd ellenőrzi, hogy az eredmény pozitív-e és kissebb,
mint a megadott cimkék száma. Ha nem, akkor a második paraméterként megadott, első cimkére ugrik. Ha igen, akkor pedig a harmadik
paramétertől (második cimkétől) kezdve az akkumulátoradik cimkét veszi, és oda ugrik.

Szóval dióhéjban

```
sw (érték), (cimke ahová egyébként ugrik),
  (cimke ahová ugrik ha az akkumulátor egyenlő értékkel),
  (cimke ahová ugrik ha az akkumulátor egyenlő érték + 1-el),
  (cimke ahová ugrik ha az akkumulátor egyenlő érték + 2-vel),
  (cimke ahová ugrik ha az akkumulátor egyenlő érték + 3-al),
  ...
  (cimke ahová ugrik ha az akkumulátor egyenlő érték + N-el)
```

Összesen 256 érték címkéje lehet minden `sw` mnemoniknak.
