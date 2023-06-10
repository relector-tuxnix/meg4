Pattogó labda
=============

Ebben a példában egy olyan programot fogunk készíteni, ami egy labdát pattogtat a képernyőn.

Labda megjelenítése
-------------------

Először is indítsuk el a `meg4` programot, és hozzuk be a [Szprájt szerkesztő]t (üsd le az <kbd>F3</kbd>-at). Válasszuk ki az
első szprájtot szerkesztésre jobbra, és rajzoljuk meg a labdát a bal oldali szerkesztőben.

<imgc ../img/tut_ball1.png><fig>A labda megrajzolása</fig>

Most menjünk a [Kód szerkesztő]be (üsd le az <kbd>F2</kbd>-őt). Elsőre a programunk egy üres váz lesz.

```c
#!c

void setup()
{
  /* Induláskor lefuttatandó dolgok */
}

void loop()
{
  /* Minden képkockánál lefuttatandó dolgok, 60 FPS */
}
```

Jelenítsük meg az újonnan rajzolt szprájtunkat! Ezt az [spr] hívással tehetjük meg. Menjünk a `setup()` függvény törzsébe,
kezdjük el beírni, és alul a státuszsorban megjelennek a paraméterei.

<imgc ../img/tut_ball2.png>

Láthatjuk, hogy az első két paraméter az `x, y`, a képernyő koordináta, ahová ki akarjuk rakni a szprájtot (ha nem lenne
egyértelmű a paraméter nevéből, hogy mit takar, akkor az <kbd>F1</kbd> leütésével bejön a súgó részletes leírással, majd ott
<kbd>Esc</kbd>-et ütve visszakerülsz ide a kódszerkesztésre). A képernyő 320 pixel széles és 200 pixel magas, ezért ha középre
akarjuk elhelyezni, adjunk meg a `160, 100` értékeket. A következő paraméter a `sprite`. Mivel a legelső, nulladik szprájtra
rajzoltunk, ezért ez `0`. Az ezt követő paraméterek az `sw` (szprájt szélesség) és `sh` (szprájt magasság). Mivel a labdánk
csupán egyetlen szprájtba belefér, azért írjunk `1, 1`-et. Ezt követi a `scale` (átméretezés), de nem szeretnénk felnagyítani,
így ide is írjunk csak `1`-et. Végül az utolsó paraméter a `type` (megjelenés típusa), amivel transzformálhatjuk a szprájtot.
Mivel ezt sem szeretnénk, azért adjunk csak meg `0`-át.

```c
#!c

void setup()
{
  /* Induláskor lefuttatandó dolgok */
  <hl>spr(160, 100, 0, 1, 1, 1, 0);</hl>
}

void loop()
{
  /* Minden képkockánál lefuttatandó dolgok, 60 FPS */
}
```

Próbáljuk meg lefuttatni ezt a programot a <kbd>Ctrl</kbd>+<kbd>R</kbd> leütésével, és lássuk mi történik. Ha valamilyen hibát
vétettél a begépeléskor, akkor alul a státuszsorban egy hibaüzenet fog megjelenni, és a kurzor a hibát kiváltó részre fog ugrani.

Ha minden rendben volt, akkor a szerkesztőképernyő el fog tűnni, és helyette egy fekete képernyő, közepén a labdával fog
megjelenni. Azonban a labdánk nem teljesen középre került, mivel elfelejtettük kivonni a szprájt méretének felét a koordinátákból
(egy szprájtot jelenítünk meg itt (sw = 1 és sh = 1), szóval 8 x 8 pixelt, aminek a fele 4). Üssük le az <kbd>F2</kbd>-őt, hogy
visszatérjünk a szerkesztőbe, és javítsuk ki.

```c
#!c

void setup()
{
  /* Induláskor lefuttatandó dolgok */
  spr(<hl>156, 96</hl>, 0, 1, 1, 1, 0);
}

void loop()
{
  /* Minden képkockánál lefuttatandó dolgok, 60 FPS */
}
```

Futtassuk le újra! A labdánk megjelenik a jó helyen, de a korábbi helyén is ottmaradt! Ez azért van, mert nem töröltük le a
képernyőt. Ezt a [cls] (clear screen) paranccsal tehetjük meg, ezért ezt írjuk be a szprájtmegjelenítés elé.

```c
#!c

void setup()
{
  /* Induláskor lefuttatandó dolgok */
  <hl>cls(0);</hl>
  spr(156, 96, 0, 1, 1, 1, 0);
}

void loop()
{
  /* Minden képkockánál lefuttatandó dolgok, 60 FPS */
}
```

Most már minden rendben, pontosan a képernyő közepén megjelenik a labda.

Labda mozgatása
---------------

Van egy kis probléma a kódunkkal. A labdát a `setup()` függvényben jelenítjük meg, de ez csak egyszer fut le, amikor a programunk
elindul. Ahhoz, hogy mozogni lássuk a labdát, újra és újra ki kell rajzolnunk, mindig más pozícióra. Hogy ezt elérjük, tegyük át a
labdánk megjelenítését a `loop()` függvénybe. Ez minden egyes alkalommal lefut, amikor a képernyő frissül.

```c
#!c

void setup()
{
  /* Induláskor lefuttatandó dolgok */
}

void loop()
{
  /* Minden képkockánál lefuttatandó dolgok, 60 FPS */
<hm>  cls(0);
  spr(156, 96, 0, 1, 1, 1, 0);</hm>
}
```

Ha most leütjük a <kbd>Ctrl</kbd>+<kbd>R</kbd>-t, akkor a labda pontosan ugyanúgy fog megjelenni, mint eddig. Amit nem látunk,
az az, hogy most nemcsak egyszer, hanem újra és újra kirajzolódik.

Mozgassuk meg a labdát! Most mindig ugyanott jelenik meg, mivel konstans koordinátákat használtunk. Javítsunk ezen úgy, hogy
bevezetünk két változót, amik a labda épp aktuális pozícióját fogják tárolni a képernyőn.

```c
#!c

<hl>int x, y;</hl>

void setup()
{
  /* Induláskor lefuttatandó dolgok */
}

void loop()
{
  /* Minden képkockánál lefuttatandó dolgok, 60 FPS */
  cls(0);
  spr(156, 96, 0, 1, 1, 1, 0);
}
```

Cseréljük le a koordinátákat a kirajzolásnál ezekre a változókra, és adjunk nekik induló értéket a program indulásakor.

```c
#!c

int x, y;

void setup()
{
  /* Induláskor lefuttatandó dolgok */
<hm>  x = 156;
  y = 96;</hm>
}

void loop()
{
  /* Minden képkockánál lefuttatandó dolgok, 60 FPS */
  cls(0);
  spr(<hl>x, y</hl>, 0, 1, 1, 1, 0);
}
```

Ha most lefuttatjuk a programunkat, akkor még mindig nem fogunk semmi változást látni. Azonban annak köszönhetően, hogy
most már változókat használunk, futás közben tudjuk változtatni a pozíciót.

```c
#!c

int x, y;

void setup()
{
  /* Induláskor lefuttatandó dolgok */
  x = 156;
  y = 96;
}

void loop()
{
  /* Minden képkockánál lefuttatandó dolgok, 60 FPS */
  cls(0);
  spr(x, y, 0, 1, 1, 1, 0);
<hm>  x = x + 1;
  y = y + 1;</hm>
}
```

Futtasuk le így, és látni fogjuk a labdát mozogni!

Labda pattogás
--------------

Még nem vagyunk készen, mert a labdánk nagyon gyorsan eltűnik a képernyőről. Ez azért van, mert folyton növeljük a koordinátáit,
és nem fordítjuk meg az irányát, amikor a képernyő szélére ér.

Akárcsak a koordináták esetében először, most is konstanst használunk, és szeretnénk az irányt dinamikusan változtatni. A megoldás
most is ugyanaz, lecseréljük a konstansokat két új változóra.

```c
#!c

int x, y<hl>, dx, dy</hl>;

void setup()
{
  /* Induláskor lefuttatandó dolgok */
  x = 156;
  y = 96;
  <hl>dx = dy = 1;</hl>
}

void loop()
{
  /* Minden képkockánál lefuttatandó dolgok, 60 FPS */
  cls(0);
  spr(x, y, 0, 1, 1, 1, 0);
  x = x + <hl>dx</hl>;
  y = y + <hl>dy</hl>;
}
```

Nagyszerű! Mint korábban említettük, a képernyő 320 pixel széles, és 200 pixel magas. Ez azt jelenti, hogy a lehetséges `x`
értékek 0 és 319 között vannak, `y` esetén pedig 0 és 199 között. Azonban nem akarjuk, hogy a labdánk eltűnjön a képernyőről, így
ebből ki kell vonni a szprájt méretét (8 x 8 pixel). Így végül azt kapjuk, hogy az `x` 0 és 311 közötti, az `y` pedig 0 és 191
közötti. Amikor a labdánk pozíciója eléri ezeket az értékeket, akkor meg fell fordítanunk az irányát, hogy mindig a képernyőn
belül maradjon.

```c
#!c

int x, y, dx, dy;

void setup()
{
  /* Induláskor lefuttatandó dolgok */
  x = 156;
  y = 96;
  dx = dy = 1;
}

void loop()
{
  /* Minden képkockánál lefuttatandó dolgok, 60 FPS */
  cls(0);
  spr(x, y, 0, 1, 1, 1, 0);
  x = x + dx;
  y = y + dy;
<hm>  if(x == 0 || x == 311) dx = -dx;
  if(y == 0 || y == 191) dy = -dy;</hm>
}
```

Futtasuk le ezt a prgoramot a <kbd>Ctrl</kbd>+<kbd>R</kbd>-el. Gratulálunk, van egy mozgó labdád, ami megpattan a képernyő
szélein!

Ütő hozzáadása
--------------

Az a játék, amit nem tudunk befolyásolni, nem túl izgalmas. Ezért hozzáadunk egy ütőt, amit a játékos irányíthat.

Menjünk a [Szprájt szerkesztő]be (üsd le az <kbd>F3</kbd>-at), és rajzoljuk meg az ütőt. Ez most három szprájt széles lesz.
Megrajzolhatod egyenként, vagy akár kijelölhetsz több szprájtot is jobbra, és együtt szerkesztheted őket.

<imgc ../img/tut_ball3.png><fig>Az ütő megrajzolása</fig>

Hasonlóan a labdához, ezt is az [spr] paranccsal fogjuk megjeleníteni. Azt azonban már tudjuk, hogy szükségünk lesz egy változóra,
ami a pozícióját tárolja, így adjuk hozzá egyből ezt is.

```c
#!c

int x, y, dx, dy<hl>, px</hl>;

void setup()
{
  /* Induláskor lefuttatandó dolgok */
  x = 156;
  y = 96;
  dx = dy = 1;
}

void loop()
{
  /* Minden képkockánál lefuttatandó dolgok, 60 FPS */
  cls(0);
  spr(x, y, 0, 1, 1, 1, 0);
  <hl>spr(px, 191, 1, 3, 1, 1, 0);</hl>
  x = x + dx;
  y = y + dy;
  if(x == 0 || x == 311) dx = -dx;
  if(y == 0 || y == 191) dy = -dy;
}
```

Mivel az ütőt az egyes szprájtra rajzoltuk, ezért a `sprite` paraméter most `1`, és mivel három szprájt széles, ezért az `sw`
pedig `3`. Az `y` koordináta konstans, mivel függőlegesen nem fogjuk mozgatni az ütőt, csak vízszintesen.

Eddig jó, de hogy fogja tudni mozgatni a játékos az ütőt az egérrel? Hát úgy, hogy az egér koordinátáját adjuk az `x` paraméternek.
Ha megnézed a memóriatérképet, akkor a [mutató] fejezetben azt látod, hogy az egér X koordintája 2 bájton, a `00016`-os címen
található. Hogy ezt lekérdezzük, az [inw] függvényt fogjuk használni (word, azaz szó, mivel két bájtra van szükségünk). De arra
is figyelnünk kell, hogy az ütő a képernyőn maradjon, ezért csak azokat a koordinátákat használjuk, amik kissebbek, mint a
képernyő szélessége mínusz az ütő szélessége (ami három szprájtnyi, tehát 24 pixeles). Még egy dolog, a memóriatérképen minden cím
tizenhatos számrendszerben van megadva, így a `0x` előtagot kell használnunk, hogy jelezzük a fordítóprogramnak, egy hexadecimális
szám következik.

```c
#!c

int x, y, dx, dy, px;

void setup()
{
  /* Induláskor lefuttatandó dolgok */
  x = 156;
  y = 96;
  dx = dy = 1;
}

void loop()
{
  /* Minden képkockánál lefuttatandó dolgok, 60 FPS */
  cls(0);
  spr(x, y, 0, 1, 1, 1, 0);
<hm>  px = inw(0x16);
  if(px > 296) px = 296;</hm>
  spr(px, 191, 1, 3, 1, 1, 0);
  x = x + dx;
  y = y + dy;
  if(x == 0 || x == 311) dx = -dx;
  if(y == 0 || y == 191) dy = -dy;
}
```

Futtassuk le a programot! Megjelenik az ütő, és mozgatni is tudjuk az egérrel. Azonban akad egy kis bökkenő, a labdát nem érdekli,
hol van az ütő. Írjuk át a programot, hogy a képernyő alja helyett csak akkor pattanjon a labda, ha épp az ütőre esik. A labda
magassága 8 pixel, ezért ennyivel feljebb kell az ellenőrzést elvégezni.

```c
#!c

int x, y, dx, dy, px;

void setup()
{
  /* Induláskor lefuttatandó dolgok */
  x = 156;
  y = 96;
  dx = dy = 1;
}

void loop()
{
  /* Minden képkockánál lefuttatandó dolgok, 60 FPS */
  cls(0);
  spr(x, y, 0, 1, 1, 1, 0);
  px = inw(0x16);
  if(px > 296) px = 296;
  spr(px, 191, 1, 3, 1, 1, 0);
  x = x + dx;
  y = y + dy;
  if(x == 0 || x == 311) dx = -dx;
  if(y == 0 || <hl>(y == 183 && x >= px && x <= px + 24)</hl>) dy = -dy;
}
```

Ez azt jelenti, hogy az `y` koordináta nulla, vagy `183` és ugyanakkor az `x` pedig `px` és `px + 24` közötti (ahol a `px` az ütő
pozíciója).

Játék vége
----------

Most, hogy átírtuk az alsó pattanást, szükségünk van egy újabb ellenőrzésre, hogy a labda alul kiment-e a képernyőről. Ez ugye
a játék végét fogja jelenteni.

Ilyenkor három dolgot is szeretnénk. Először is ne felejtsük el, hogy a `loop()` továbbra is többször lefut, ezért hogy a labda
ne mozogjon tovább, a `dx` és `dy` változókat nullára állítjuk. Másodszor, meg akarunk jeleníteni egy vége a játéknak üzenetet.
Végezetül pedig, ha a játékos kattint egyet, akkor újra akarjuk indítani a játékot.

```c
#!c

int x, y, dx, dy, px;
<hm>str_t msg = "VÉGE A JÁTÉKNAK!";</hm>

void setup()
{
  /* Induláskor lefuttatandó dolgok */
  x = 156;
  y = 96;
  dx = dy = 1;
}

void loop()
{
  /* Minden képkockánál lefuttatandó dolgok, 60 FPS */
  cls(0);
  spr(x, y, 0, 1, 1, 1, 0);
  px = inw(0x16);
  if(px > 296) px = 296;
  spr(px, 191, 1, 3, 1, 1, 0);
  x = x + dx;
  y = y + dy;
  if(x == 0 || x == 311) dx = -dx;
  if(y == 0 || (y == 183 && x >= px && x <= px + 24)) dy = -dy;
<hm>  if(y > 199) {
    dx = dy = 0;
    text(184, (320 - width(2, msg)) / 2, 90, 2, 112, 128, msg);
    if(getclk(BTN_L)) setup();
  }</hm>
}
```

Szöveget a [text] funkcióval lehet kiírni a képernyőre. Alul a gyorssúgó mutatja, milyen paraméterei vannak. A legelső a `palidx`,
ami egy paletta index, a szöveg színe. Üssük le az <kbd>F3</kbd> gombot, és palettán kattintsunk a kívánt színre. Alul meg fog
jelenni a sorszáma tizenhatos számrendszerben, illetve zárójelben tízesben.

<imgc ../img/tut_ball4.png>

Én egy piros színt választottam, aminek a sorszáma `B8`, azaz `184` tízes számrendszerben. Menjünk vissza a kódszerkesztőbe az
<kbd>F2</kbd> leütésével, és írjuk be a szín sorszámát. A következő két paraméter az `x` és az `y`, a képernyő pozíció. Megtehettük
volna, hogy megszámoljuk, mennyi pixel a szöveg, de lusták vagyunk, ezért inkább a [width] funkcióval kiszámoltatjuk. Ezt kivonva
a képernyő szélességéből és elosztva kettővel pont középen fog megjelenni a szöveg. Mivel ahhoz is listák vagyunk, hogy kétszer
írjuk be a szöveget, ezért azt egy `msg` nevű sztring változóba raktuk. Így `msg`-ként hivatkozunk rá egyszer, amikor a méretét
lekérjük, és akkor is, amikor a kiírásnak megadjuk, mit kell kiírni. A koordináták után jön a `type`, ami a betűtípus, illetve
pontosabban itt most a mérete. Mivel nagy betűkkel szeretnénk kiírni, ezért `2` lett megadva, dupla méret. Ezt követi az `shidx`,
ami megint egy szín sorszám, az árnyék színe. Ide egy sötétebb pirosat választottam. Az árnyéknál fontos azonban az is, hogy
mennyire átlátszó, ezt az `sha` paraméterben adhatjuk meg, ami egy alfa csatorna érték 0-tól (teljesen átlátszó) 255-ig
(semennyire sem). Ide a `128`-at választottam, ami félúton van a két érték között, tehát félig átlátszó. Végezetül pedig `str`-ben
kell megadni, hogy melyik szöveget szeretnénk kiíratni.

Hogy megtudjuk, kattintott-e a felhasználó, arra a [getclk] (get click) funkciót használjuk, egy `BTN_L` (button left) paraméterrel,
azaz kattintottak-e a bal egérgommbal. A pattogó labda játékunk alapértékeit a `setup()` függvényben állítottuk be, ami nagyon
kapóra jön, mivel ha itt most újra meghívjuk a `setup()`-ot, akkor a játék egyszerűen visszaáll az alaphelyzetébe.
