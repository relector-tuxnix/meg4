Sétáló
======

Ebben a példában egy sétáló karaktert fogunk megjeleníteni szprájtokkal. Ez az alapja a legtöbb kaland és kósza-típusú játéknak.

Szprájtlap beszerzése
---------------------

Meg is rajzolhattuk volna magunk, de az egyszerűség kedvéért letöltöttem egy szabadon felhasználható lapot az internetről.
Ez három animációs fázist tartalmaz minden sorban, és mind a négy irányhoz van egy-egy sor. Rengeteg ilyen szprájtlapot találsz
a neten, mivel ez a népszerű RPG Maker szprájt elrendezése.

WARN: Mindig ellenőrizd az internetről letöltött kellékek licenszfeltételeit. Ne használd a kelléket, ha nem vagy teljesen
biztos a felhasználás feltételeiben.

A letöltött képet nem fogod tudni egy-az-egyben beimportálni. Előbb a kép dimenzióját 256 x 256 pixelesre kell állítani. Ne
használd az átméretezést, helyette a GIMP-ben válaszd az `Image` > `Canvas size...` menüpontot, majd a felugró ablakban a
szélesség magassághoz írd be a 256-ot. Így a szprájtlap mérete marad az, ami volt, csak ki lesz egészítve átlátszó pixelekkel.
Indítsd el a `meg4`-et, és húzd rá az ablakára ezt a 256 x 256 pixeles képet az importáláshoz.

<imgc ../img/tut_walk1.png><fig>A beimportált szprájtlap</fig>

Amint látható, egy karakterszprájt 4 x 4 szprájtból áll. Jelenítsük meg! Üsd le az <kbd>F2</kbd>-őt, hogy behozd a kódszerkesztőt.

Karakter megjelenítése
----------------------

A szokásos vázzal indítunk. Az előző példából tudjuk, hogy le kell törölni a képernyőt és a szprájtot az [spr] hívással, a
`loop()` funkcióban kell megjelenítenünk, mivel az animálás folyamatos újrarajzolást igényel.

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
  spr(144, 84, 0, 4, 4, 0, 0);</hm>
}
```
A képernyő közepe 160, 100-nál van, de a karakterünk 4 x 4 szprájtból áll (32 x 32 pixeles), így ennek a felét le kell vonni.
Ezt követő 0 a `sprite`, ami az első szprájtot jelenti, 4, 4-el folytatva, mivel összesen ennyi szprájtot akarunk kirakni. Az
utolsó két paraméter 0, 0, mivel nem akarunk sem nagyítani, sem transzformációt használni. Úgy akarjuk a szprájtokat megjeleníteni,
ahogy azok a szprájtszerkesztőben látszanak.

Irányváltoztatás
----------------

Tegyük lehetővé, hogy megváltoztatható legyen, merre néz a karakter. Ehhez a [getpad] hívást fogjuk használni, ami visszaadja a
játékpad állapotát. Mivel az elsődleges játékpad a billentyűzetre is le van képezve, ezért ez a kurzornyilakkal is működni fog.
Hogy letároljuk az épp aktuális irányt, ehhez szükségünk lesz egy változóra, ami kiválasztja, hogy melyik szprájtot is kell
megjelenítenünk.

```c
#!c

<hl>int irány;</hl>

void setup()
{
  /* Induláskor lefuttatandó dolgok */
}

void loop()
{
  /* Felhasználói bemenet */
<hm>  if(getpad(0, BTN_D)) {
    irány = 0;
  } else
  if(getpad(0, BTN_L)) {
    irány = 128;
  } else
  if(getpad(0, BTN_R)) {
    irány = 256;
  } else
  if(getpad(0, BTN_U)) {
    irány = 384;
  }</hm>
  /* Karakter megjelenítése */
  cls(0);
  spr(144, 84, <hl>irány</hl>, 4, 4, 0, 0);
}
```

Leütheted az <kbd>F3</kbd>-at és a szprájtválasztóban a karakter bal felső szprátjára kattintva kiírja az adott irányhoz
tartozó szprájt sorszámát. Ezt az `irány` változóban fogjuk tárolni, és ezt a változót használjuk a `sprite` paraméter helyén.

Próbáljuk ki! Látni fogjuk, hogy a kurzornyilak lenyomására irányt vált a karakterünk.

Animálás hozzáadása
-------------------

A karakterünk még nem sétál. Javítsuk ki! Azt szeretnénk, hogy a gomb (vagy kurzornyíl) lenyomásakor a karakter sétáljon, és
megálljon, amikor felengedjük azt. Ehhez szükségünk lesz egy változóra, ami tárolja, hogy épp van-e lenyomva gomb. Kelleni fog
mégegy változó, ami pedig azt mondja meg, melyik képkockát kell épp kirajzolni. Használhatnánk valami vicces képletet is, de
egyszerűbb egy tömbben letárolni, hogy melyik képkockához melyik szprájt tartozik. Mivel a karakterünk 4 x 4 szprájtból áll, ezért
eleve fel is szorozhatjuk ezeket a szprájt azonosító eltolásokat a tömbben. Még egy valami, csak három szprájtunk van, így a
középső szprájtot kétszer is meg kell jelenítenünk, hogy rendes oda-vissza lábmozgást animációt kapjunk.

```c
#!c

int irány<hl>, lenyomva, képkocka;</hl>
<hl>int anim[4] = { 4, 8, 4, 0 };</hl>

void setup()
{
  /* Induláskor lefuttatandó dolgok */
}

void loop()
{
  /* Felhasználói bemenet */
  <hl>lenyomva = 0;</hl>
  if(getpad(0, BTN_D)) {
    irány = 0; <hl>lenyomva = 1;</hl>
  } else
  if(getpad(0, BTN_L)) {
    irány = 128; <hl>lenyomva = 1;</hl>
  } else
  if(getpad(0, BTN_R)) {
    irány = 256; <hl>lenyomva = 1;</hl>
  } else
  if(getpad(0, BTN_U)) {
    irány = 384; <hl>lenyomva = 1;</hl>
  }
  /* Karakter megjelenítése */
  <hl>képkocka = lenyomva ? (képkocka + 1) & 3 : 0;</hl>
  cls(0);
  spr(144, 84, irány <hl>+ anim[képkocka]</hl>, 4, 4, 0, 0);
}
```

Először is, töröljük a `lenyomva` változót. Aztán minden if blokkban beállítjuk 1-re. Ennek hatására ha lenyomjuk valamelyik
gombot, akkor a változó értéke 1 lesz, de amint felengedjük azt a gombot, akkor egyből visszaáll 0-ra.

Ezután, kiszámoljuk, melyik képkockára van szükségünk, de csak akkor, ha van gomb lenyomva. Ha nincs, akkor konstans 0-át használunk,
ami az első képkockát jelenti. Egyébként megnöveljük a `képkocka` változó értékét, hogy következő képkockát vegyük, és logikai ÉS-t
használunk a túlcsordulás elkerülésére. Amikor a `képkocka` értéke 4 lesz (ami 0b100) és ezt ÉS-eljük 3-al (0b011), akkor az eredmény
0 lesz, azaz a képkocka számláló körbefordul. Használhattunk volna "modulo képkockák száma" kifejezést is, de ez gyorsabb. Végül
az így kapott képkockának megfelelő szprájt azonosító eltolást (amit az `anim` tömbben tárolunk), hozzáadjuk az `irány` változóhoz,
hogy megkapjuk, ténylegesen melyik szprájtot is kell kirajzolni.

Próbáljuk ki, üsd le a <kbd>Ctrl</kbd>+<kbd>R</kbd>-t! Remekül működik, azonban az animáció túl gyors. Ez azért van, mert minden
frissítésnél növeljük a `képkocka` változót, tehát másodpercenként 60-szor. Hogy ezt elkerüljük, az MMIO-ból kell kiolvasni a
tikkeket és a képkockát a frissítéstől függetlenül kell meghatározni. A tikkszámláló azonban ezredmásodperces, így le kell osztanunk.
Ha 100-al osztanánk, akkor másodpercenként 10 képkockát kapnánk. Ehelyett 7 bittel jobbra eltolást fogunk használni, ami 128-al való
osztásnak felel meg. Először is, üsd le az <kbd>F1</kbd>-et és kattints az [Általános]-ra. Láthatjuk, hogy a tikkszámláló címe a
0x4, és 4 bájt hosszú (ezért az [ini] hívást kell használnunk). Menjünk vissza a kódra, és a képkockaszámítást cseréljük le erre.

```c
#!c

int irány, lenyomva, képkocka;
int anim[4] = { 4, 8, 4, 0 };

void setup()
{
  /* Induláskor lefuttatandó dolgok */
}

void loop()
{
  /* Felhasználói bemenet */
  lenyomva = 0;
  if(getpad(0, BTN_D)) {
    irány = 0; lenyomva = 1;
  } else
  if(getpad(0, BTN_L)) {
    irány = 128; lenyomva = 1;
  } else
  if(getpad(0, BTN_R)) {
    irány = 256; lenyomva = 1;
  } else
  if(getpad(0, BTN_U)) {
    irány = 384; lenyomva = 1;
  }
  /* Karakter megjelenítése */
  képkocka = lenyomva ? (<hl>ini(0x4) >> 7</hl>) & 3 : 0;
  cls(0);
  spr(144, 84, irány + anim[képkocka], 4, 4, 0, 0);
}
```

És készen vagyunk! Van egy szépen sétáló karakter animációnk, amit mi irányíthatunk a játékban. Mozgathatod magát a karaktert is a
képernyőn, ha változókra cseréled az x, y paramétereket, de az ilyen játékoknál sokkal elterjedtebb, hogy inkább a térképet
mozgatják a karakter talpa alatt az ellenkező irányba.
