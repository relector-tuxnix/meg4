Debuggoló
=========

<imgt ../img/debug.png> Kattints a katica ikonra (vagy <kbd>F10</kbd>) a programod gépi kódú ellenőrzéséhez.

A kód három alrészből áll, az egyik a programod gépi kódú változatát mutatja (ez), a forráskód szöveges szerkesztése
a [Kód Szerkesztő]ben lehetséges, míg a [Vizuális Szerkesztő]ben ugyanezt struktrogrammokkal teheted meg.

WARN: A debuggoló csak a beépített nyelvek esetén működik. Nem elérhető olyan harmadik fél által fejlesztett nyelvek esetén, mint
például a Lua, ezeknél nincs és nem is lehetséges a támogatás.

Itt megtekinthető, hogy a CPU miként látja a programodat. A <kbd>Space</kbd> leütésével lépésenként hajthatod végre a programodat,
és közben láthatod, hogy változnak a regiszterek és a memória. A menüben a <ui1>Kód</ui1> / <ui1>Adat</ui1> gombbal (vagy a
<kbd>Tab</kbd> billentyűvel) váltogathatsz kód- és adatnézet között.

<imgc ../img/debugscr.png><fig>Debuggoló</fig>

Kódnézet
--------

Balra láthatod a hívásvermet. Ez arra használatos, hogy nyomonkövessük a függvényhívásokat. Tartalmazza azt a forráskód sort is,
ahonnan a függvény meghívódott. Ez egy hivatkozás, rákattintva előjön a [Kód Szerkesztő], a kérdéses sorra pozícionálva. A lista
tetején mindig az a sor látható, ami jelen pillanatban épp végrehajtódik.

Jobbra van az utasítás bájtkódlista [Assembly]ben, amiket a CPU ténylegesen végrehajt.

Adatnézet
---------

Balra látható a programod globális változóinak listája, az épp aktuális értékeikkel.

Jobbra van a verem, ami több részre oszlik. Minden, ami a BP regiszter felett helyezkedik el, az az éppen futó program
paraméterlistája, és minden ami ezalatt, de még az SP regiszter fölött található, azok meg a lokális változók.

Regiszterek
-----------

Függetlenül attól, hogy melyik nézet az aktív, a CPU regiszterei mindig látszanak alul. Harmadik fél által biztosított nyelvek
esetén csak az FLG, TMR és a PC regiszter elérhető. Bővebb leírást az egyes regiszterekről a [mnemonikok] címszó alatt találsz.
