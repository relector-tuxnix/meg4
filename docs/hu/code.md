Kód Szerkesztő
==============

<imgt ../img/code.png> Kattints a ceruza ikonra (vagy <kbd>F2</kbd>) a program forráskódjának írásához.

A kód három alrészből áll, az egyiken szövegesen tudod a forráskódot szerkeszteni (ez), a [Vizuális Szerkesztő]ben ugyanezt
struktrogrammokkal teheted meg, a programod gépi kódú megfelelőjét pedig a [debuggoló]ban láthatod.

Itt az egész felület egy nagy forráskód szerkesztő felület. Alul látható a státuszsor, az aktuális sor és oszlop értékekkel,
valamint a kurzor alatti karakter UNICODE kódpontjával, továbbá ha épp egy API funkció paraméterei között áll a kurzor, akkor
egy gyorssúgóval a paraméterek listájáról (ami érvényes minden programozási nyelv esetén).

<imgc ../img/codescr.png><fig>Kód Szerkesztő</fig>

Programozási nyelv
------------------

A programnak egy speciális sorral kell kezdődnie, az első két karakter `#!`, amit a használni kívánt nyelv kódja zár. Az
alapértelmezett a [MEG-4 C](#c) (az ANSI C egyszerűsített változata), de több nyelv is a rendelkezésedre áll. A teljes listát
balra, a tartalomjegyzékben a "Programozás" címszó alatt találod.

Felhasználói funkciók
---------------------

A választott nyelvtől függetlenül van két funkció, amit neked kell implementálni. Nincs paraméterük, sem visszatérési értékük.

- `setup` funkció elhagyható, és csak egyszer fut le, amikor a programod betöltődik.
- `loop` funkció kötelező, és mindig lefut, amikor egy képkocka generálódik. 60 FPS (képkocka per másodperc) mellett ez azt
  jelenti, hogy a futására 16.6 milliszekundum áll rendelkezésre, de ebből a MEG-4 "hardver" elvesz 2-3 milliszekundumot, így
  12-13 ezredmásodperced marad, amit a függvényed kitölthet. Ezt lekérdezheted a performancia számláló MMIO regiszterből, lásd
  [memóriatérkép]. Ha ennél tovább tart a `loop` funkció futása, akkor a képernyő töredezni fog, és az emulátor kevésbé lesz
  reszponzív, mint lenni szokott.

Plusz billentyűkombók
---------------------

A normál billentyűkön és beviteli módokon túl (lásd [billentyűzet](#ui_kbd)), a kódszerkesztéskor még ezek is elérhetők:

| Billentyűkombináció          | Leírás                                                                                       |
|------------------------------|----------------------------------------------------------------------------------------------|
| <kbd>Ctrl</kbd>+<kbd>F</kbd> | Sztring keresése.                                                                            |
| <kbd>Ctrl</kbd>+<kbd>G</kbd> | Következő keresése.                                                                          |
| <kbd>Ctrl</kbd>+<kbd>H</kbd> | Keresés és csere (a kijelöltben, vagy ha az nincs, akkor az egész forrásban).                |
| <kbd>Ctrl</kbd>+<kbd>J</kbd> | Megadott sorra ugrás.                                                                        |
| <kbd>Ctrl</kbd>+<kbd>D</kbd> | Funkció definíciójához ugrás.                                                                |
| <kbd>Ctrl</kbd>+<kbd>N</kbd> | Könyvjelzők listája.                                                                         |
| <kbd>Ctrl</kbd>+<kbd>B</kbd> | Könyvjelző ki/bekapcsolása az aktuális soron.                                                |
| <kbd>Ctrl</kbd>+<kbd>▴</kbd> | Előző könyvjelzőhöz ugrás.                                                                   |
| <kbd>Ctrl</kbd>+<kbd>▾</kbd> | Következő könyvjelzőhöz ugrás.                                                               |
| <kbd>Home</kbd>              | Kurzor mozgatása a sor elejére.                                                              |
| <kbd>End</kbd>               | Kurzor mozgatása a sor végége.                                                               |
| <kbd>Page Up</kbd>           | Kurzor mozgatása 42 sorral (egy oldallal) feljebb.                                           |
| <kbd>Page Down</kbd>         | Kurzor mozgatása 42 sorral (egy oldallal) lejjebb.                                           |
| <kbd>F1</kbd>                | Ha a kurzor egy API paraméterlistáján áll, akkor az API funkció súgójára visz.               |

A menüből ugyancsak eléhető a keresés, következő, sorraugrás, visszavonás, újrabeillesztés, valamint a könyvjelzők és funkciók
definíciójának listája is.
