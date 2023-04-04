Szprájt Szerkesztő
==================

<imgt ../img/sprite.png> A bélyegző ikonra kattintva (vagy <kbd>F3</kbd>) jön elő a szprájtok szerkesztése.

Az itt szerkesztett szprájtokat az [spr] paranccsal jelenítheted meg, illetve ezeket használva az [stext] szöveget ír ki.

A szerkesztőnek három fő mezője van, kettő felül, egy pedig alul.

<imgc ../img/spritescr.png><fig>Szprájt Szerkesztő</fig>

<h2 spr_edit>Szprájt Szerkesztő Mező</h2>

Balra fent van a szerkesztő mező. Itt tudod módosítani a szprájtot. A <mbl> elhelyezi a kijelölt pixelt a szprájton, míg
a <mbr> törli őket.

<h2 spr_sprs>Szprájtválasztó</h2>

Jobbra mellette látható a szprájtválasztó. Az a szprájt, amit itt kiválasztasz lesz szerkeszthető balra. Egyszerre több
egymásmelletti szprájt is kiválasztható, és ilyenkor egyszerre, együtt szerkesztheted őket.

<h2 spr_pal>Paletta</h2>

Alul helyezkedik el a pixel paletta. Az első elem nem módosítható, mert az a törlésre van fenntartva. Ha azonban bármelyik másik
pixelt választod, akkor a <imgt ../img/hsv.png> gomb a színpaletta ikonnal aktívvá válik. Erre kattintva felugrik a HSV
színválasztó ablak, ahol megadhatod a kiválasztott palettaelem színét.

Az alapértelmezett MEG-4 paletta 32 színt használ a DawnBringer32 palettáról, 8 szürkeárnyalatot, valamint 6 x 6 x 6 piros,
zöld, kék átmenetkombinációt.

<h2 spr_tools>Eszköztár</h2>

<imgc ../img/toolbox.png><fig>Szprájt eszköztár</fig>

A szerkesztőmező alatt található az eszköztár a gombokkal. Ezekkel könnyedén módosíthatod a szprájtot, eltolhatod különböző
irányokba, elforgathatod órajárással megegyező irányban, tükrözheted, stb. Ha van aktív kijelölés, akkor ezek az eszközök csak
a kijelölt pixelekre hatnak, egyébként az egész szprájtra. A forgatáshoz úgy kell kijelölni, hogy a kijelölés ugyanolyan magas
legyen, mint széles különben a forgatás nem fog működni.

A kitöltés csak a szomszédos, azonos színű pixelekre vonatkozik, hacsak nincs kijelölés. Kijelöléssel a teljes kijelölt terület
kitöltésre kerül, függetlenül attól, hogy milyen pixelek lettek kijelölve.

<h2 spr_sel>Kijelölések</h2>

Kétfajta kijelölés van: doboz és varázspálca. Az előbbi négyzet alakban jelöl ki, utóbbi minden szomszédos, azonos színű pixelt
kijelöl. A <kbd>Shift</kbd> lenyomva tartásával bővíthető a kijelölés, míg a <kbd>Ctrl</kbd> lenyomva tartásával kivághatsz a
kijelölésből, és csökkentheted.

A <kbd>Ctrl</kbd>+<kbd>A</kbd> lenyomásával minden kijelölődik, a <kbd>Ctrl</kbd>+<kbd>I</kbd> pedig invertálja (megfordítja)
a kijelölést, azaz ami eddig nem volt kijelölve, az ki lesz, ami meg ki volt, az nem lesz.

Amikor van aktív kijelölés, akkor a <kbd>Ctrl</kbd>+<kbd>C</kbd> lenyomásával a kijelölt terület a vágólapra másolható. Később
lenyomhatod a <kbd>Ctrl</kbd>+<kbd>V</kbd> gombokat a beillesztéshez. A beillesztés pontosan úgy működik, mint a festés, csak
itt egy pixel helyett a vágólapod tartalma egy nagy ecset lesz. Érdemes megjegyezni, hogy a vágólapra másolt üres pixelek is az
ecset részét képezik. Ha nem szeretnéd, hogy az ecset töröljön, akkor csak a nem üres pixeleket jelöld ki (használhatod a
<kbd>Ctrl</kbd>+<imgt ../img/fuzzy.png> kombót az üres pixelek kijelölésének megszüntetéséhez) a vágólapra másolás előtt, így az
üres pixelek nem kerülnek a vágólapra, és emiatt az ecsetben sem fognak megjelenni.
