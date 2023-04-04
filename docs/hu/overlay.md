Memóriaátfedők
==============

<imgt ../img/overlay.png> Kattints a RAM ikonra (vagy <kbd>F8</kbd>) a memóriaátfedők szerkesztéséhez.

Az átfedők nagyon hasznosak, mivel lehetővé teszik, hogy a RAM bizonyos része több különböző, egymást átfedő adatot tartalmazzon,
és így több adatot legyen képes kezelni, mint amennyi a memóriába beleférne. Ezekkel dinamikusan, futás közben betölthetsz
szprájtokat, térképeket, betűkészleteket vagy épp bármilyen tetszőleges adatot a [memload] funkció hívásával.

Van egy másik, nagyon hasznos tulajdonságuk is: ha meghívod a [memsave] funkciót a programodból, akkor az átfedő adatai a felhasználó
számítógépére kerülnek lementésre. A `memload` legközelebbi hívásakor a betöltendő adatok már nem a flopidról, hanem a felhasználó
számítógépéről fognak érkezni. Ezzel könnyedén egy permanens tároló alakítható ki, például az elért pontszámok tárolására.

<imgc ../img/overlayscr.png><fig>Memóriaátfedők</fig>

Átfedőválasztó
--------------

Az oldal tetején látható a memóriaátfedő kiválasztó, minden egyes átfedő méretével. A sötétebb bejegyzések azt jelzik, hogy az
adott átfedő nincs megadva. Összesen 256 átfedő áll rendelkezésedre, 00-tól FF-ig.

Átfedő tartalma
---------------

A tábla alatt láthatod az átfedő adatait hexdump formájában (csak akkor, ha nem üres átfedő van épp kiválasztva).

A hexdump egy pofonegyszerű formátum, az első oszlop tartalmazza a címet, ami mindig 16-al osztható. Ezt követi az adott
címen található 16 bájt hexa formában, majd ugyanaz a 16 bájt karakterként. Ennyi.

Átfedő menü
-----------

A menüsorban fent, megadhatsz egy memóriacímet és egy méretet, majd az <ui1>Elment</ui1> gombra kattintva le fogja tárolni a
memória adatait a kijelölt átfedőbe. A <ui1>Betölt</ui1> gomb használatával visszatölthető a memória tartalma a megadott címre,
de ekkor a méret azt jelenti, maximum ennyi bájtot szabad betölteni.

Az <ui1>Export</ui1> gombra kattintva megjelenik a fájlmentés ablak, ahol lementheted és módosíthatod a bináris tartalmat egy
külsős programmal. Az átfedő visszatöltéséhez a fájlt úgy kell elnevezni, hogy `memXX.bin`, ahol az `XX` a használni kívánt átfedő
száma 00-tól FF-ig, és csak simán rá kell húzni ezt a fájlt a MEG-4 ablakára.
