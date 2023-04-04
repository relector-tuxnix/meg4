Térkép Szerkesztő
=================

<imgt ../img/map.png> A kirakós ikonra kattintva (vagy <kbd>F4</kbd>) felhozza a térkép szerkesztőt. Itt egy térképen helyezheted
el a szprájtokat.

Az itt összerakott térképet (vagy egy részét) a [map] illetve a [maze] (lásd alább) paranncsal tudod megjeleníteni a képernyőn.

<imgc ../img/mapscr.png><fig>Térkép Szerkesztő</fig>

A térkép különleges abból a szempontból, hogy egyszerre csak 256 különböző szprájtot képes megjeleníteni a összes rendelkezésre
álló 1024 szprájtból. Minden szprájtbank esetén a legeslegelső szprájt fenn van tartva a "nincs térképelem" számára, így a 0-ás,
256-os, 512-es valamint a 768-as szprájt nem használható a térképen.

<h2 map_box>Térkép Szerkesztő Mező</h2>

Felül a nagy részen láthatod és szerkesztheted a térképet. Egy nagy térképként látszik, 320 oszloppal és 200 sorral. Használhatod
a nagyítás és kicsinyítés gombokat az eszköztáron, vagy az <mbw> egérgörgőket is a nagyításhoz. A <mbr> jobb egérgombot lenyomva
tartva és az egeret mozgatva tologathatod a térképet, vagy használhatod a görgetőket is jobbra illetve alul.

A <mbl> balklikkel szprájtokat helyezhetsz el a térképen. A legelső szprájt kiválasztásával lehet törölni a térképen (a <mbr>
jobb egérgomb itt nem töröl, hanem mozgatja a térképet).

<h2 map_tools>Eszköztár</h2>

A térképszerkesztő mező alatt található az eszköztár, ugyanaz, mint a [szprájt szerkesztő] képernyőn, ugyanazokkal a funkciókkal
és ugyanazokkal a gyorsgombokkal (csak itt használhatók szprájtminták is, lásd alább). Az eszközgombok mellett vannak a nagyítás
és kicsinyítés gombok, ezek után pedig a térképválasztó. Ez utóbbi a térkép szprájtbankját választja (de csak a szerkesztőben.
Amikor a programod fut, akkor a 0007F címen lévő bájtot kell állítani a bankok váltásához, lásd [Grafikus Feldolgozó Egység]).

<h2 map_sprs>Szprájtpaletta</h2>

A gomboktól jobbra helyezkedik el a szprájtválasztó paletta, ahol kiválaszthatod, melyik szprájttal akarsz rajzolni. Amint korábban
már megjegyeztük, a paletta legelső eleme minden 256-os szprájtbankból nem használható, az a "nincs térképelem" számára van
fenntartva. Ha törölni szerenél a térképen, akkor válaszd ezt az első elemet, és "rajzolj" azzal.

A szprájtszerkesztővel ellentétben (ahol csak egyetlen színt választhatsz a palettáról), itt, a térképen több, egymásmelleti
szprájt is kiválasztható a palettáról egyszerre. Festéskor mindet rá fogja tenni a térképre, (pont úgy, mintha a beillesztést
használnád), ezen kívül a kitöltés is ecsetként fogja használni őket, egy nagy, többszprájtos mintával kitöltve a kijelölt
területet.

Sőt, mi több, a <imgt ../img/fill.png> kitöltésnél <kbd>Shift</kbd>+<mbl>-el kattintva, véletlenszerűen fog választani a kijelölt
szprájtok közül. Például, tegyük fel, hogy van 4 féle különböző szprájtod fákkal. Ha kijelölöd öket, majd kitöltöd az erdőnek
szánt részt a térképen, akkor ezek a szprájtok libasorban, egymásután ismételve kerülnek elhelyezésre, ami nem néz ki túl jól
erdőként. Viszont ha lenyomva tartod a <kbd>Shift</kbd>-et, miközben a kitöltéssel kattintasz, akkor minden mezőhöz
véletlenszerűen választ egyet a kijelölt 4 fa-szprájt közül, ami már sokkal inkább néz ki igazi erdőnek.

3D-s útvesztő
-------------

A térkép 3 dimenziós labirintusként is megjeleníthető a [maze] funkcióval. Ehhez a teknőc pozícióját és irányát használja fel,
mint a játékos nézőpontját az útvesztőre, de hogy a csempén belüli pozicíókat is kezelni tudja, a teknőc koordinátái ilyenkor
128-al fel vannak szorozva (eredetileg 8-at használtam, hogy egyezzen a térkép pixelszámával, de a mozgás túl darabos volt úgy).
Tehát például a (64,64) a térkép bal felső mezőjének közepét jelenti, a (320,192) pedig a harmadik oszlop második sor közepe.

Itt a `scale` nagyítás paraméter is másképp értelmeződik: amikor 0, akkor a labirintus 32 x 8 csempét tud használni, ahogy az a
szprájtpalettán látszik, csempénként egy szprájt, 8 x 8 pixel méretben. Ha 1, akkor viszont 16 x 16 csempét, ahol minden csempe
2 x 2 szprájt, tehát 16 x 16 pixel méretű. 3 esetén 4 x 4 fajta csempéd lehet, azaz összesen 16 féle, mindegyik 64 x 64 pixeles.
Ebben az esetben a térkép ezeket a nagyobb csempéket választja, ezért a csempe sorszáma és a szprájt sorszáma csak akkor egyezik
meg, ha a nagyítás értéke 0. Például ha a térképen az 1-es id van és a nagyítás is 1-es, akkor az 1-es szprájt helyett ez a 2-es,
3-as, 34-es, 35-ös szprájtokat jelenti.

```
Csempe id 1 scale 0 esetén      Csempe id 1 scale 1 esetén
+---+===+---+---+-              +---+---+===+===+-
|  0|::1|  2|  3| ...           |  0|  1|::2|::3| ...
+---+===+---+---+-              +---+---+===+===+-
| 32| 33| 34| 35| ...           | 32| 33|:34|:35| ...
+---+---+---+---+-              +---+---+===+===+-
```

Ettől függetlenül a térképre az 1-es szprájtot kell a palettáról elhelyezned, hogy ezeket a szprájtokat kapd. Szokás szerint a
0-ás csempe a nincs térképelemet jelenti.

Ha a `sky` (ég) paraméter meg van adva, akkor az a csempe, mint a labirintus mennyezete lesz megjelenítve. Másrészről a `grd`
(ground, föld) csak ott jelenik meg talajként, ahol nincs térképelem megadva. A csempeazonosítókat zónákra oszthatod híváskor,
ettől függ, hogy az adott csempe miképp jelenik meg (talaj, fal vagy szprájt). Minden olyan csempe, ami a `wall`-nál nagyobb
vagy azzal egyenlő, nem lesz átjárható, és olyan kockaként jelenik meg, ahol a csempe szprájtjai kerülnek a kocka
oldalaira, átlátszóság nélkül. Azok a csempék, amik nagyobbak vagy egyenlőek az `obj`-ban (tárgy) megadottnál, szintén
átjárhatatlanok lesznek, de ezek megfelelőre méretezett 2D-s szprájtként jelennek meg, arccal mindig a játékos (a teknőc
pozíciója) felé fordulva, és ezeknél értelmezett az alfa csatorna, szóval a falakkal ellentétben a tárgyak lehetnek átlátszóak.

| Csempe id            | Leírás                                               |
|----------------------|------------------------------------------------------|
| 0                    | Mindig járható, `grd` talajként jelenik meg helyette |
| 1 <= x < `door`      | Járható, talajként jelenik meg                       |
| `door` <= x < `wall` | Falként jelenik meg, mégis átjárható (ajtó)          |
| `wall` <= x < `obj`  | Nem átjárható, falként jelenik meg                   |
| `obj` <= x           | Nem átjárható, tárgy szprájtként jelenik meg         |

Hozzáadhatsz nem játékos karaktereket (illetve további tárgyakat) is a labirintushoz a térképtől függetlenül (egy int tömbben, ami
x, y, csempe id hármasokat tartalmaz, a koordináták 128-al felszorozva). Ezek mind átjárhatóak lesznek, és pont úgy jelennek meg,
mint a tárgy szprájtok; az ezekkel való ütközést, mozgatásukat és minden egyebet neked kell implementálni a játékodban. A `maze`
parancs csupán megjeleníti ezeket. Annyi szívességet azonban megtesz neked, hogyha az adott NJK direktben látja a játékost akkor
a tömbben a csempe id legmagasabb 4 bitjét beállítja. Hogy melyiket, az a kettejük távolságától függ: a legmagasabb bit
(0x80000000) azt jelzi, közelebb vannak, mint 8 térképmező, a következő bit (0x40000000) azt, hogy közelebb, mint 4 mező, a
következő (0x20000000) azt, hogy közelebb, mint 2 mező, és végül a legutolsó bit (0x10000000) azt, hogy ugyanazon vagy szomszédos
térképmezőn állnak.

Mindezeken felül ez a parancs gondoskodik a labirintusban való navigálásról is, a <kbd>▴</kbd> / △ előre mozgatja a teknőst, a
<kbd>▾</kbd> / ▽ hátra; a <kbd>◂</kbd> / ◁ balra fordul, a <kbd>▸</kbd> / ▷ jobbra fordul (a játékpad billentyűzetkiosztása
megváltoztatható, lásd [memóriatérkép]). Az összes többi játékpad gomb lekezelése és az interakciók kivitelezése a játékodtól
függ, így a Te feladatod leprogramozni ezeket, a `maze` csak a játékos mozgatásában és a falakkal való ütközésben segít.

NOTE: Ne feledd, hogy a teknőc pozícióját mindig 128-al le kell osztani, hogy megkapd, a térkép melyik mezőjén tartózkodik épp
a játékos.
