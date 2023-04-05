Felhasználói bemenetek
======================

<h2 ui_gp>Játékpad</h2>

Az első játékpad és joystick leképezésre kerül a billentyűzetre, és együtt működnek. Például nem számít, hogy a kontrolleren
nyomod-e le a Ⓧ gombot, vagy a billentyűzeten a <kbd>X</kbd> billentyűt, mindkét esetben mind a játékpad lenyomott jelzője,
mind a billentyű lenyomott jelzője beállításra kerül. A leképezés megváltoztatható szkenkódok MEG-4 memóriába írásával, bővebb
infóért lásd a [memóriatérkép] fejezetet. Az alapértelmezett leképezés a kurzornyilak az irányok ◁, △, ▽, ▷; a
<kbd>Szóköz</kbd> az Ⓐ elsődleges gomb, a <kbd>C</kbd> a Ⓑ másodlagos gomb és az <kbd>X</kbd> az Ⓧ, <kbd>Y</kbd> az Ⓨ. A
Konami Kód is működik (lásd `KEY_CHEAT` szkenkód).

<h2 ui_ptr>Mutató</h2>

A koordináták és az egérgombok lenyomott állapota könnyedén lekérdezhető a MEG-4 memóriájából. A görgetés (mind a függőleges, és
ha van, vízszintes támogatott), úgy van kezelve, mintha fel / le vagy balra / jobbra egérgombok lennének.

<h2 ui_kbd>Billentyűzet</h2>

A kényelmed érdekében számos gyorsbillentyűvel és beviteli metódussal szolgál.

| Billentyűkombináció          | Leírás                                                                                       |
|------------------------------|----------------------------------------------------------------------------------------------|
| <kbd>GUI</kbd>               | Vagy Super, néha <imgt ../img/gui.png> logó van rajta. UNICODE kódpont beviteli mód.         |
| <kbd>AltGr</kbd>             | A szóköztől jobbra lévő Alt billentyű, Kompozit beviteli mód.                                |
| <kbd>Alt</kbd>+<kbd>U</kbd>  | Ha a billentyűzeteden nem lenne <kbd>GUI</kbd> gomb, ez is UNICODE kódpont beviteli mód.     |
| <kbd>Alt</kbd>+<kbd>Space</kbd> | Kompozit vésztartalék, az <kbd>AltGr</kbd> gomb nélküli billentyűzetehez.                 |
| <kbd>Alt</kbd>+<kbd>I</kbd>  | Ikon (emoji) beviteli mód.                                                                   |
| <kbd>Alt</kbd>+<kbd>K</kbd>  | Katakana beviteli mód.                                                                       |
| <kbd>Alt</kbd>+<kbd>J</kbd>  | Hiragana beviteli mód.                                                                       |
| <kbd>Alt</kbd>+<kbd>A</kbd>  | Ha nem lenne a billentyűzeten ilyen gomb, `&` (ampersand).                                   |
| <kbd>Alt</kbd>+<kbd>H</kbd>  | Ha nem lenne a billentyűzeten ilyen gomb, `#` (hashmark).                                    |
| <kbd>Alt</kbd>+<kbd>S</kbd>  | Ha nem lenne a billentyűzeten ilyen gomb, `$` (dollar).                                      |
| <kbd>Alt</kbd>+<kbd>L</kbd>  | Ha nem lenne a billentyűzeten ilyen gomb, `£` (pound).                                       |
| <kbd>Alt</kbd>+<kbd>E</kbd>  | Ha nem lenne a billentyűzeten ilyen gomb, `€` (euro).                                        |
| <kbd>Alt</kbd>+<kbd>R</kbd>  | Ha nem lenne a billentyűzeten ilyen gomb, `₹` (rupee).                                       |
| <kbd>Alt</kbd>+<kbd>Y</kbd>  | Ha nem lenne a billentyűzeten ilyen gomb, `¥` (yen).                                         |
| <kbd>Alt</kbd>+<kbd>N</kbd>  | Ha nem lenne a billentyűzeten ilyen gomb, `元` (yuan).                                        |
| <kbd>Alt</kbd>+<kbd>B</kbd>  | Ha nem lenne a billentyűzeten ilyen gomb, `₿` (bitcoin).                                     |
| <kbd>Ctrl</kbd>+<kbd>S</kbd> | Flopi mentése.                                                                               |
| <kbd>Ctrl</kbd>+<kbd>L</kbd> | Flopi betöltése.                                                                             |
| <kbd>Ctrl</kbd>+<kbd>R</kbd> | Programod futtatása.                                                                         |
| <kbd>Ctrl</kbd>+<kbd>⏎Enter</kbd> | Teljesképernyős mód váltogatása.                                                        |
| <kbd>Ctrl</kbd>+<kbd>A</kbd> | Mindent kijelöl.                                                                             |
| <kbd>Ctrl</kbd>+<kbd>I</kbd> | Kijelölés megfordítása.                                                                      |
| <kbd>Ctrl</kbd>+<kbd>X</kbd> | Kivágás, másolás vágólapra majd törlés.                                                      |
| <kbd>Ctrl</kbd>+<kbd>C</kbd> | Másolás vágólapra.                                                                           |
| <kbd>Ctrl</kbd>+<kbd>V</kbd> | Beillesztés vágólapról.                                                                      |
| <kbd>Ctrl</kbd>+<kbd>Z</kbd> | Visszavonás.                                                                                 |
| <kbd>Ctrl</kbd>+<kbd>Y</kbd> | Újrabeillesztés.                                                                             |
| <kbd>F1</kbd>                | Beépített súgó oldalak (a kézikönyv API Referencia fejezete, lásd [interfész]).              |
| <kbd>F2</kbd>                | [Kód Szerkesztő]                                                                             |
| <kbd>F3</kbd>                | [Szprájt Szerkesztő]                                                                         |
| <kbd>F4</kbd>                | [Térkép Szerkesztő]                                                                          |
| <kbd>F5</kbd>                | [Betű Szerkesztő]                                                                            |
| <kbd>F6</kbd>                | [Hangeffektek]                                                                               |
| <kbd>F7</kbd>                | [Zenesávok]                                                                                  |
| <kbd>F8</kbd>                | [Memóriaátfedők]                                                                             |
| <kbd>F9</kbd>                | [Vizuális Szerkesztő]                                                                        |
| <kbd>F10</kbd>               | [Debuggoló]                                                                                  |
| <kbd>F11</kbd>               | Teljesképernyős mód váltogatása.                                                             |
| <kbd>F12</kbd>               | Képernyő mentése `meg4_scr_(unix időbélyeg).png` néven.                                      |

### UNICODE Kódpont beviteli mód

Ebben a módban hexa számok adhatók meg (`0`-tól `9`-ig és `A`-tól `F`-ig). Ahelyett, hogy külön-külön vinné be ezeket a gombokat,
az általuk leírt kódpontot viszi be, mintha az a billentyűzeten egy különálló gomb lett volna. Például a következő sorozat
<kbd>GUI</kbd>, <kbd>2</kbd>, <kbd>e</kbd>, <kbd>⏎Enter</kbd> egy pontot `.` visz be, mivel a `U+0002E` kódpont az a `.` pont
karakter.

NOTE: Csak az Alap Többnyelvű Sík (Basic Multilingual Plane, `U+00000`-tól `U+0FFFF`-ig) támogatott, néhány kivétellel az emoji
tartományára a `U+1F600`-tól kezdve. Minden más kódpontot egyszerűen figyelmen kívül hagy.

Ez a beviteli mód automatikusan kilép bevitel után.

### Kompozit mód

Kompozit módban ékezet, kettőspont, hullám, hurok stb. adható a karakterekhez. Például a következő sorozat <kbd>AltGr</kbd>,
<kbd>a</kbd>, <kbd>'</kbd> egy `á`-t, vagy az <kbd>AltGr</kbd>, <kbd>s</kbd>, <kbd>s</kbd> egy `ß`-t, míg az <kbd>AltGr</kbd>,
<kbd>c</kbd>, <kbd>,</kbd> egy `ç`-t, stb. eredményez. Használható a <kbd>Shift</kbd> a betűvel együtt, hogy a nagybetűs formát
adja végeredményül.

Ez a beviteli mód automatikusan kilép bevitel után.

### Ikon mód

Ikon módban olyan speciális ikon karakterek vihetők be, amik az emulátor bevitelét ábrázolják (például a következő sorozat
<kbd>Alt</kbd>+<kbd>I</kbd>, <kbd>m</kbd> egy `🖱` egér (mouse) ikont, vagy az <kbd>Alt</kbd>+<kbd>I</kbd>, <kbd>a</kbd> a játékpad
`Ⓐ` gombjának ikonját) valamint emoji ikonokat ábrázolnak (például <kbd>Alt</kbd>+<kbd>I</kbd>, <kbd>;</kbd>, <kbd>)</kbd> egy
`😉` karaktert fog bevinni, míg az <kbd>Alt</kbd>+<kbd>I</kbd>, <kbd><</kbd>, <kbd>3</kbd> egy `❤` karaktert eredményez).

Ez a beviteli mód aktív marad bevitel után is, üss <kbd>Esc</kbd> gombot a normál beviteli módhoz való visszatéréshez.

### Katakana és Hiragana mód

Hasonló az ikon módhoz, de itt a kiejtett hangok Romaji betűit kell beírni egy ázsiai karakterhez. Például a következő sorozat
<kbd>Alt</kbd>+<kbd>K</kbd>, <kbd>n</kbd>, <kbd>a</kbd>, <kbd>n</kbd>, <kbd>i</kbd>, <kbd>k</kbd>, <kbd>a</kbd> úgy értelemződik,
mint három hang, ezért három karaktert fog bevinni, `ナニヵ`. Továbbá az írásjelek is úgy működnek, ahogy elvárjuk, például az
<kbd>Alt</kbd>+<kbd>K</kbd>, <kbd>.</kbd> a japán teljes állj `。` karaktert illeszti be.

Használható a <kbd>Shift</kbd> az első betűvel együtt, hogy a végeredmény a nagybetűs változat legyen, például
<kbd>Alt</kbd>+<kbd>K</kbd>, <kbd>Shift</kbd>+<kbd>s</kbd>, <kbd>u</kbd> eredménye `ス` és nem `ㇲ`.

Ez a beviteli mód aktív marad bevitel után is, üss <kbd>Esc</kbd> gombot a normál beviteli módhoz való visszatéréshez.

NOTE: Ez a funkció adattáblákkal került implementálásra, újabb kombinációk vagy akár újabb írásrendszerek könnyedén, bármikor
hozzáadhatók a `src/inp.c` fájlhoz programozói ismeretek nélkül is.
