Zenesávok
=========

<imgt ../img/music.png> Kattints a hangjegy ikonra (vagy <kbd>F7</kbd>) a zenék szerkesztéséhez.

Az itt szerkesztett zenesávokat a programodban a [music] paranccsal tudod lejátszani.

Látni fogsz öt oszlopot, és alul egy zongorát.

<imgc ../img/musicscr.png><fig>Zenesávok</fig>

## Sávok

Balra az első oszlopban lehet kiválasztani, melyik zenei sávot szeretnénk szerkeszteni.

| Billentyűkombináció          | Leírás                                                                                       |
|------------------------------|----------------------------------------------------------------------------------------------|
| <kbd>Page Up</kbd>           | Előző sávra vált.                                                                            |
| <kbd>Page Down</kbd>         | Következő sávra vált.                                                                        |
| <kbd>Szóköz</kbd>            | Elindítja / leállítja a sáv lejátszását.                                                     |

A sávválasztó alatt látszanak a DSP státusz regiszterek, de ez a blokk csak akkor kel életre, ha folyamatban van a lejátszás.

## Csatornák

Mellette négy hasonló oszlop található, mindegyikben hangjegyek. Ez az a négy csatorna, amit a zenelejátszó egyidejűleg képes
megszólaltatni. Ez hasonló a kottához, bővebb infóért lásd a [General MIDI] fejezetet alább.

| Billentyűkombináció          | Leírás                                                                                       |
|------------------------------|----------------------------------------------------------------------------------------------|
| <kbd>◂</kbd>                 | Előző csatornára vált.                                                                       |
| <kbd>▸</kbd>                 | Következő csatornára vált.                                                                   |
| <kbd>▴</kbd>                 | Egy sorral feljebb lép.                                                                      |
| <kbd>▾</kbd>                 | Egy sorral lejjebb lép.                                                                      |
| <kbd>Home</kbd>              | Első sorra ugrik.                                                                            |
| <kbd>End</kbd>               | Utolsó sorra ugrik.                                                                          |
| <kbd>Ins</kbd>               | Sor beszúrása. Minden, ami alatta van, eggyel lejjebb kerül.                                 |
| <kbd>Del</kbd>               | Sor törlése. Minden, ami alatta van, eggyel feljebb kerül.                                   |
| <kbd>Backspace</kbd>         | Hangjegy törlése.                                                                            |

## Hangjegyszerkesztő

A csatornák alatt látható a hangjegyszerkesztő, néhány gombbal balra és egy nagy zongorával jobbra.

A hangjegyeknek három részük van, az első a hangmagasság (a szerkesztőben felül), ami újabb három alrészből áll. Az első a hang
maga, mint pl. `C` vagy `D`. Aztán a `-` karakter ha egészhang, illetve `#` félhangok esetében. A harmadik rész pedig simán az
oktáv, `0`-tól (legmélyebb) `7`-ig (legmagasabb). A 440 Hz normál zenei A hang például ezért úgy van írva, hogy `A-4`. A zongora
segítségével könnyedén választhatsz hangmagasságot.

A hangmagasság után következik a korábban említett hangszer (a hangjegy középső része), ami a kívánt hullámmintát választja ki,
`01`-től `1F`-ig. A 0-ás érték `..`-ként jelenik meg, és azt jelenti, használd a korábban beállított hangmintát tovább.

Végezetül adhatsz speciális effekteket a hangjegyhez (a hangjegy utolsó része), mint például arpeggio (akkordként szólal meg),
portamento, vibrato, tremolo stb. A teljes listát megtalálod a [memóriatérkép](#digitalis_szignalfeldolgozo_processzor)nél. Ennek
van egy numerikus paramétere, ami általában azt jelenti, hogy "mennyire". Három hexa számként jelenik meg, ahol az első az effekt
típusa, a maradék kettő pedig a paraméter, vagy pedig `...` ha nincs beállítva. Például a `C00` azt jelenti, állítsd a hangerőt
nullára, azaz némítsd el a csatornát.

<h3 mus_kbd>Billentyűzet</h3>

<imgc ../img/pianokbd.png><fig>Zongora billentyűzetkiosztás</fig>

1. hullámminta választók
2. effekt választók
3. oktáv választók
4. zongora klaviatúra

| Billentyűkombináció          | Leírás                                                                                       |
|------------------------------|----------------------------------------------------------------------------------------------|
| <kbd>1</kbd> - <kbd>0</kbd>  | Hullámminta választás 1-től 10-ig (illetve <kbd>Shift</kbd>-el 11-től 20-ig).                |
| <kbd>Q</kbd>                 | Minden effekt törlése a hangjegyen (de a hangmagasságot és a hullámmintát ne).               |
| <kbd>W</kbd>                 | Arpeggio dúr.                                                                                |
| <kbd>E</kbd>                 | Arpeggio moll.                                                                               |
| <kbd>R</kbd>                 | Csúsztatás egész hanggal felfelé.                                                            |
| <kbd>T</kbd>                 | Csúsztatás fél hanggal felfelé.                                                              |
| <kbd>Y</kbd>                 | Csúsztatás fél hanggal lefelé.                                                               |
| <kbd>U</kbd>                 | Csúsztatás egész hanggal felfelé.                                                            |
| <kbd>I</kbd>                 | Vibrato kicsi.                                                                               |
| <kbd>O</kbd>                 | Vibrato nagy.                                                                                |
| <kbd>P</kbd>                 | Tremolo kicsi.                                                                               |
| <kbd>\[</kbd>                | Tremolo nagy.                                                                                |
| <kbd>\]</kbd>                | Csatorna elnémítása "effekt".                                                                |
| <kbd>Z</kbd>                 | Egy oktávval méllyebbre lép.                                                                 |
| <kbd>.</kbd>                 | Egy oktávval magasabbra lép.                                                                 |
| <kbd>X</kbd>                 | C hang az aktuális oktávon.                                                                  |
| <kbd>D</kbd>                 | C# hang az aktuális oktávon.                                                                 |
| <kbd>C</kbd>                 | D hang az aktuális oktávon.                                                                  |
| <kbd>F</kbd>                 | D# hang az aktuális oktávon.                                                                 |
| <kbd>V</kbd>                 | E hang az aktuális oktávon.                                                                  |
| <kbd>B</kbd>                 | F hang az aktuális oktávon.                                                                  |
| <kbd>H</kbd>                 | F# hang az aktuális oktávon.                                                                 |
| <kbd>N</kbd>                 | G hang az aktuális oktávon.                                                                  |
| <kbd>J</kbd>                 | G# hang az aktuális oktávon.                                                                 |
| <kbd>M</kbd>                 | A hang az aktuális oktávon.                                                                  |
| <kbd>K</kbd>                 | A# hang az aktuális oktávon.                                                                 |
| <kbd>,</kbd>                 | H (B) hang az aktuális oktávon.                                                              |

NOTE: Angol kiosztás billentyűit tartalmazza ez a táblázat. Azonban nemigazán számít, milyen billentyűzetkiosztást használsz,
csak az számít, ezek a gombok hol vannak az angolon. Például, ha AZERTY kiosztásod van, akkor neked az <kbd>A</kbd> gomb lesz az
effektek törlése funkció, QWERTZ esetén pedig a <kbd>Z</kbd> lefele csúsztatja a hangot, és a <kbd>Y</kbd> fog oktávot váltani.

Fontos megjegyezni, hogy nem minden funkciónak van gyorsbillentyűje. Például 31 hullámmintád lehet, de csak az első 20 érhető
el gyorsgombokkal. Ugyanígy effektekből is nagyságrendekkel több van, mint ahánynak gyorsgombja van.

## General MIDI

Zenék (legalábbis a kottáik) importálhatók MIDI fájlokból. Nagyon leegyszerűsítve a dolgot, amikor egy klasszikus zenei kottát
számítógépen tárolnak digitalizált formában, akkor ahhoz a MIDI formátumot használják. Namost ezek alkalmasak egy hangszerhez
vagy akár egy komplett zenekarnak is, szóval sokkal több mindent tudnak tárolni, mint amire a MEG-4 képes, emiatt

WARN: Nem minden MIDI fájlt lehet tökéletesen beimportálni.

Mielőtt továbbmennék, muszáj ejteni pár szót a kifejezésekről, mivel sajnálatos módon a MIDI specifikáció és a MEG-4 is ugyanazt
a nevezéktant használja - csak épp tök mást értenek alatta.

- MIDI dal: egy *.mid* fájl (az SMF2 formátum nem támogatott).
- MIDI sáv: egy sor a klasszikus kottán.
- MIDI csatorna: csak azért létezik, mert a MIDI-t idióták alkották, akik azt hitték, jó poén mindent egy sávba zsúfolni amikor
    a fájlokat mentik, egyébként pontosan és egy-az-egyben ugyanaz, mint a sáv. 16 ilyened lehet.
- MIDI hangszer: egy kód (amit a General MIDI Patch szabványosított), ez írja le, hogy melyik hangszert használja egy adott
    csatorna (kivéve, ha az a 10-es csatorna, ne is kérdezd miért), összesen 128 féle hangszerkód van belőle.
- MIDI hang: egy hang, C a -1. oktávontól a G a 9. oktávonig tartományban, 128 különböző hangjegy összesen.
- MIDI konkurrens hangok: azon hangok száma, amik egyszerre szólhatnak egy adott pillanatban. Mivel 16 csatorna és 128 hangjegy
    van a MIDI-ben, így ez 2048.
- MEG-4 sáv: egy dal, ezekből 8 lehet összesen.
- MEG-4 minta: a hulláminta mint PCM adatok sorozata, megfeleltethető a hangszereknek, összesen 31 ilyened lehet.
- MEG-4 csatorna: azon hangok száma, amik egyszerre szólhatnak egy adott pillanatban, ez 4.
- MEG-4 hang: egy hang, C a 0. oktávontól a B a 7. oktávonig tartományban, 96 különböző hangjegy összesen.

A félreértések elkerülése végett csakis egy MEG-4 sávról lesz szó, és a továbbiakban a "sáv" a MIDI csatornákat jelenti, a
"csatorna" pedig a MEG-4 csatornáit.

### Hangszerek

Ami a hangszereket illeti, összesen 16 család van, 8 hangszerrel mindegyikben. A MEG-4 nem tud ilyen sok, 128 különböző
hullámmintát tárolni, ezért családonként csak kettőt rendel hozzá (a 15. és 16. család a speciális effekteké, nem használt):

| Család     | SF | Patch | Hogy kéne szólnia     | SF | Patch | Hogy kéne szólnia     |
|------------|---:|-------|-----------------------|---:|-------|-----------------------|
| Piano      | 01 |   1-4 | Akusztikus zongora    | 02 |   5-8 | Elektromos zongora    |
| Chromatic  | 03 |  9-12 | Csemballó             | 04 | 13-16 | Csőharangok           |
| Organ      | 05 | 17-20 | Templomi orgona       | 06 | 21-24 | Harmónika             |
| Guitar     | 07 | 25-28 | Akusztikus gitár      | 08 | 29-32 | Elektromos gitár      |
| Bass       | 09 | 33-36 | Akusztikus Basszus    | 0A | 37-40 | Basszusgitár          |
| Strings    | 0B | 41-44 | Hegedű                | 0C | 45-48 | Zenekari hárfa        |
| Ensemble   | 0D | 49-52 | Vonósegyüttes         | 0E | 53-56 | Kórus ááá             |
| Brass      | 0F | 57-60 | Trombita              | 10 | 61-64 | Kürt                  |
| Reed       | 11 | 65-68 | Szaxofon              | 12 | 69-72 | Oboa                  |
| Pipe       | 13 | 73-76 | Fuvola                | 14 | 77-80 | Fújt üveg             |
| Synth Lead | 15 | 81-84 | Szintetizátor 1       | 16 | 85-88 | Szintetizátor 2       |
| Synth Pad  | 17 | 89-92 | Szintetizátor 3       | 18 | 93-96 | Szintetizátor 3       |

Általánosítva, a General MIDI hangszerből a `(patch - 1) / 4 + 1`-dik soundfont hullámminta lesz.

Fontos megjegyezni, hogy a MEG-4 dinamikusan osztja ki a hullámmintákat, szóval ezek a számok a soundfontbeli hullámok sorszámai.
Ha például a MIDI fájlod csak két hangszert használ, zongorát és elektromos gitárt, akkor a zongora kerül az 1-es hullámhelyre,
és a gitár a 2-esre. Előre betölthetsz minden hangmintát a soundfontból a [hangeffektek] szerkesztőben, ekkor a beimportált MIDI
fájlod pontosan ezeket a hullám sorszámokat fogja használni.

### Kották

A MEG-4 mintái megfeleltethetők a klasszikus zenei kottának, de amíg a klasszikus kottán az idő balról jobbra halad és egy
MIDI sáv csak egy hangszert jelenhet, addig a MEG-4 kottán az idő fentről lefelé halad és szabadon változtathatod, egy adott
csatorna mikor melyik hullámot használja. Itt egy konkrét példa (a General MIDI specifikációból):

<imgc ../img/notes.png><fig>Klasszikus kotta balra, MEG-4 kotta megfelelője jobbra</fig>

Balra három sávunk van, Electric Piano 2 (elektromos zongora), Harp (hárfa) és Bassoon (fagott). Az első hangjegy, amit le kell
játszani a fagotton, mindjárt két hangjegy egyszerre. Vegyük észre a kottán a basszuskulcsot, így ezek a hangok a C a 3. oktávon
és C a 4. oktávon, és mindkettő 4 negyedes, azaz teljes egész hangok.

Jobbra van a MEG-4 kotta megfelelője. Az első sorban látható ez a két fagott hang: `C-3` az első csatornán és `C-4` a másodikon.
A minta `12` (hexa, feltéve, hogy a soundfontot előre betöltöttük kézzel, egyébként a szám más lenne) az oboa hullámmintát
választja, ami nem igazán fagott, de ez a legközelebbi, amink van a soundfontban. A `C30` rész jelenti a gyorsulást, amivel
megszólaltatják a hangot, ez nálunk megfeleltethető a hangerőnek (minnél erősebben csapsz egy zongora billentyűre, annál hangosabb
lesz a hangja). A MEG-4 hangonkénti hangereje 0-tól (csend) 64-ig (40 hexában, teljes hangerő) terjed. Ezért a 30 hexában a teljes
hangerő 75%-a.

A következő lejátszandó hang a hárfán található, negyed hanggal a fagott után, G a 4. oktávon és 3 negyedig tart. A MEG-4 kottán
ezt `G-4`-ként látod, a negyedik sorban (mivel akkor kezdődik), és mivel az 1-es 2-es csatornán még szól a fagott, ezért a 3-as
csatornán. Ha ezt a hangot az 1-es vagy 2-es csatornára raktuk volna, akkor a csatornán épp lejátszott hang elhallgatott volna,
és lecserélődött volna az újra. A minta itt a `0C` (hexában), ami a zenekari hárfa hullámmintáját jelenti.

Az utolsó hang a legelső MIDI sávon található, félhanggal az indulás után kezdődik, egy E az 5. oktávon és 2 negyedes, azaz
félhangnyi időtartamú. Mivel félhanggal később kezdődik, ezért az `E-5`-öt a 8. sorban találod, és mivel már van három hangunk
amik még tartanak, ezért a 4. csatornára került. A minta `02` jelentése válaszd az elektromos zongorát, ami nem ugyanaz, mint
a MIDI Electric Piano 2, de elég közel áll hozzá.

Namost van két egész hangunk, egy háromnegyedes és egy feles; ezek rendre a sáv elején, negyeddel később illetve fél hanggal
később kezdődtek. Ez azt jelenti, hogy mindnek egyszerre kell, hogy vége legyen. Ezt a 16. sorban (10 hexában) láthatod, minden
csatornán van egy `C00` "állítsd a hangerőt 0-ra" parancs.

### Tempó

A MIDI suttyomban 120 BPM-et (beat per minute, percenkénti ütésszám) feltételez, és csak egy negyedhang osztót ad meg. Aztán
megadhatja a negyedhang hosszát milliomodmásodpercekben, vagy nem. A lényeg, hogy bonyolult, és nem minden kombináció fordítható
át értelmesen a MEG-4-re. Az importálót úgy írtam meg, hogy eldobja a felhalmozódó kerekítési hibákat, és csakis a két egymásutáni
hang relatív időközével foglalkozik. Emiatt a MEG-4 dal tempója sosem lesz pontosan ugyanaz, mint a MIDI dalé, de nagyon hasonlóan
kell szólnia, és túlságosan eltérnie sem szabad tőle.

A MEG-4 tempója sokkal egyszerűbb. Fixen 3000 tikked van percenként, és az alapértelmezett tempó 6 tikk soronként. Ez azt jelenti,
hogy 125 BPM-hez minden negyedik sorba kell hangjegyeket rakni (mivel 3000 / 6 / 4 = 125). Ha a tempót átállítod (lásd `Fxx`
parancs) ennek a felére, 3 tikkre soronként, akkor minden sor fele annyi ideig fog kitartani, ezért 250 BPM-et kapsz ha minden
negyedik sort használod. 3-as tempó mellett minden nyolcadik sorba kell a hangjegyeket rakni a 125 BPM-hez. Ha a tempót 12-re
állítod, akkor minden sor kétszer annyi ideig fog tartani, ezért minden negyedik sor 62.5 BPM-et jelent, és minden második sort
kell használni a 125 BPM-hez. Remélem világos.

Ez csak azt állítja, hogy mikortól szólaljon meg egy hang, és teljesen független attól, hogy az mennyi ideig fog szólni. Ez
utóbbihoz vagy egy új hangot kell használnod ugyanazon a csatornán, vagy pedig egy `C00` "állítsd a hangerőt 0-ra" effektet,
pont annál a sornál, amikor a hangot el akarod vágni. Ha e kettő között átállítod a tempót, az nem fogja a hangot befolyásolni,
csakis azt, hogy meddig szóljon (mivel másik időpontban fogja elérni azt a sort, amiben a kikapcsolás van).

Azonban a hangok akkor is elhalnak, ha a hullámmintájuk véget ér. Hogy ez mikor következik be, az függ a hangmagasságtól és a
minták számától is (a C-4 hangmagasság tikkenként 882 mintát kell küldjön a hangszóróra). Van azonban egy trükk: a hullámmintán
megadhatsz egy ún. "loop"-ot, ami azt jelenti, miután minden minta elfogyott, akkor a kijelölt tartományt fogja ismételni a
végtelenségig (szóval mindenképp el kell vágnod, kölönben a hang tényleg sosem fog elhallgatni).
