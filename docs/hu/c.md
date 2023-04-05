C
=

Ha ezt a nyelvet választod, akkor kezd a programodat a `#!c` sorral.

<h2 ex_c>Példa program</h2>

```c
#!c

/* globális változók */
int számláló = 123;
float szám = 3.1415;
addr_t cím = 0x0048C;
str_t sztring = "valami";
uint32_t tömb[10];

/* Induláskor lefuttatandó dolgok */
void setup()
{
    /* lokális változók */
    int lokálisvagyok = 123;
}

/* Minden képkockánál lefuttatandó dolgok, 60 FPS */
void loop()
{
    /* MEG-4 stílusú kimenet */
    printf("a számláló %d, balshift %d\n", számláló, getkey(KEY_LSHIFT));
}
```

Leírás
------

A konzol alapértelmezett nyelve a **MEG-4 C**. Ez, bár maga a nyelv roppant egyszerű, mégis valamivel haladóbb programozóknak
való. Ha abszolút kezdő vagy, akkor javaslom inkább a [BASIC] nyelv használatát.

Szándékosan egy lebutított ANSI C-nek lett megalkotva, hogy megkönnyítse a programozás tanulását. Emiatt eléggé korlátozott,
nem tud mindent, amit az ANSI C elvár, azonban ha lecseréled a

```
#!c
```

sort erre

```c
#include <stdint.h>
typedef char* str_t;
typedef void* addr_t;
```

akkor egy MEG-4 C forrás minden probléma nélkül le fog fordulni bármelyik szabványos ANSI C fordítóval (gcc, clang, tcc stb.).

Van egy nem szabványos kulcsszava, a `debug;`, amit akárhová elhelyezhetsz a programodban, és ami meghívja a beépített
[debuggoló]t. Ezt követően lépésről lépésre hajthatod végre a programodat, ellenőrizve, hogy épp mit csinál.

A továbbiakban a C nyelv laza bemutatása következik, arra fókuszálva, hogy miben speciális a MEG-4 C.

Előfordító
----------

Mivel csak egyetlen forrásfájl van, és a rendszerfüggvények prototípusai csont nélkül támogatottak, ezért nincs szükség fejlécfájlokra.
Emiatt az előfordító csak a nagyon egyszerű (nem makró) define-okat, valamint  a feltételes kódblokkokat támogatja csak.

```c
/* lecseréli a (defvált) összes előfordulását a (kifejezés)-re */
#define (defvált) (kifejezés)
/* kódblokk használata, ha a (defvált) lett definiálva */
#ifdef (defvált)
/* kódblokk használata, ha a (defvált) nem lett definiálva */
#ifndef (defvált)
/* kódblokk használata, ha a (kifejezés) igaz */
#if (kifejezés)
/* egyébként blokk */
#else
/* lezárja a feltételesen behúzandó kódblokkot */
#endif
```

Létrehozhatsz felsorolást az `enum` kulcsszóval, vesszővel elválasztva, kapcsoszárójelek között. Minden elemnek eggyel nagyobb
lesz az értéke, mint az azt megelőzőé. Ez pontosan úgy működik, mintha több define sort írtál volna be. Például a következő két
kód ugyanazt csinálja:

```c
#define A 0
#define B 1
#define C 2
```

```c
enum { A, B, C };
```

Továbbá lehetséges direkt értéket is hozzárendelni az egyenlőségjellel, például:

```c
enum { EGY = 1, KETTŐ, HÁROM, ÖT = 5, HAT };
```

<h2 c_lit>Literálok</h2>

A MEG-4 C elfogad tízes számrendszerbeli számokat (akár integer egészszámok, vagy lebegőpontosak, tudományos jelöléssel vagy
annélkül). Hexadecimális számokat a `0x` előtaggal kell kezdeni, a binárisokat `0b`-vel, a nyolcas számrendszerbelieket `0`-val;
a karaktereket aposztrófok közé, a sztringeket pedig macskakörömbe kell tenni:

```c
42
3.1415
.123e-10
0777
0b00100110
0x0048C
'á'
"Viszlát és kösz a halakat!\n"
```

<h2 c_var>Változók</h2>

A BASIC-el ellentétben a változókat itt deklarálni kell. Ezt két helyen lehet megtenni: a legfelső szinten, vagy minden funkció
elején. Az előbbiekből lesznek a globális változók (amiket minden funkció elér), utóbbiakból pedig a lokális változók, amiket
csakis az a funkció lát, amelyiknek a törzsében deklarálva lettek. Mégegy különbség, hogy a globális változók inicializálhatók
(induláskor érték adható nekik a `=` után), míg a lokális változók nem, azoknak külön, kifejezett paranccsal kell értéket adni.

A deklaráció két részből áll: egy típusból és egy névből. A MEG-4 C támogat minden ANSI C típust: `char` (előjeles bájt), `short`
(előjeles szó), `int` (előjeles egészszám), `float` (lebegőpontos szám). Ezek elé odarakható az `unsigned`, aminek hatására
előjel nélküli lesz a típus. ANSI C alatt az `int` elhagyható a `short` után, de MEG-4 alatt el kell hagyni. Tehát a `short int`
nem érvényes típus, a `short` önmagában viszont az. Továbbá a MEG-4 C nemcsak támogatja, de kifejezetten preferálja a szabványos
egészszám típusokat (amiket ANSI C alatt az stdint.h rögzít). Ezeknek pár egyszerű szabálya van: ha a típus előjel nélküli,
akkor `u` betűvel kezdődik; aztán az `int` jelenti, hogy egészszám, amit a tároláshoz használt bitek száma követ, majd végül
a `_t` zárja, ami arra utal, hogy típus. Például az `int` és az `int32_t` ugyanaz, akárcsak az `unsigned short` és az `uint16_t`.
Példák:

```c
int a = 42;
uint8_t b = 0xFF;
short c = 0xFFFF;
str_t d = "Valami";
float e = 3.1415;
```

Az ANSI C-vel ellentétben, ami csakis angol betűket fogad el változónevekben, a MEG-4 C bármit megenged, ha az nem számmal
kezdődik és nem egy kulcsszó. Például, az `int déjà_vu;` teljesen érvényes (vedd észre a nem angol karaktereket a névben).

<h2 c_arr>Tömbök és mutatók</h2>

Több, azonos típusú elem rendelhető egyetlen változóhoz, ezeket hívjuk tömböknek. A mutató olyan speciális változó, amiben
egy olyan memóriacím van, ami azonos típusú elemek listájára mutat. A hasonlóság e kettő között nem véletlen, de vannak apró
eltérések.

Tömbökre nincs külön parancs, mint BASIC esetében, egyszerűen csak meg kell adni az elemszámot `[` és `]` között a név után.

```c
int tömb[10];
```

A tömb elemeire hivatkozni ugyancsak a `[` és `]` között megadott indexszel lehet. Az index 0-tól indul, és ellenőrzésre kerül,
hogy a deklarációban megadott méreten belüli-e. A MEG-4 C összesen 4 dimenziót támogat.

Mutatót úgy kell deklarálni, hogy a név elé `*`-ot teszünk. A C nem ismer sztring típust, és mivel a sztringek igazából
karakterek egymásután a memóriában, ezért helyette karaktermutatót `char*` használunk. Ez furcsa lehet elsőre, ezért a MEG-4 C
definiál egy `str_t` típust, de ez igazából pont ugyanaz, mintha `char*`-ot írtunk volna.

Mivel a mutató mindig egy címet tartalmaz, ezért értékül egy címet kell neki adni (a `&` a változók címét jelenti), és a mutató
egy címet fog visszaadni. Ahhoz, hogy címen lévő értéket kapjuk, fel kell oldani a mutatót. Ez kétféleképp tehető meg: vagy
`*`-ot kell elé írni, vagy pedig utánna `[` és `]` között egy indexet, pont úgy, mint a tömbök esetében. Például a következő kettő
hivatkozás a második printf-ben ugyanaz:

```c
int változó = 1;
int *mutató = &változó;

printf("mutató értéke (cím): %x\n", mutató);
printf("mutatott érték: %x %x\n", *mutató, mutató[0]);
```

A mutatók és a tömbök nem keverhetők, mivel az nem lenne egyértelmű. Például

```c
int *a[10];
```
Jelentheti az, hogy egy darab mutató, ami 10 egymásutánni egészszámra mutat (`*(int[10])`), vagy jelenthetné azt is, hogy 10 darab
független mutató, amik mind egy-egy, nem feltétlenül egymásutánni egészszámra mutatnak (`(*int)[10]`). Nem egyértelmű melyik,
ezért a mutatók és a tömbök nem vegyíthetők egy deklaráción belül.

WARN: A tömbökkel ellentétben a mutató feloldásakor nincs méretellenőrzés!

<h2 c_op>Műveletek</h2>

Precedenciájuk szerint csökkenő sorrendben:

<p>**Aritmetikai**
```
* / %
+ -
```
Az első a szorzás, osztás, a `%` pedig a modulus (azaz osztás utáni maradék, pl. `10 % 3` az 1), összeadás, kivonás.</p>

<p>**Relációs**
```
!= ==
<= >= < >
```
Nem egyenlő, egyenlő, kissebb egyenlő, nagyobb egyenlő, kissebb, nagyobb.</p>

<p>**Logikai**
```
!
&&
||
```
Tagadás (0-ból 1 lesz, minden nem nullából pedig 0), és (csak akkor 1, ha mindkét paramétere nem 0), vagy (legalább az egyik
paramétere nem 0).</p>

<p>**Bitenkénti**
```
~
&
|
<< >>
```
Bitenkénti tagadás, bitenkénti és, bitenkénti vagy, bitenkénti eltolás balra, illetve jobbra. Vegyük észre, hogy ezek ugyanazok
a műveletek, mint a logikaiak, azonban míg azok az egész értékre vonatkoznak, ezek minden kettes számrendszerbeli helyiértékre
külön-külön. Például a logikai tagadás `!0x0100 == 0`, míg a bitenkénti tagadás `~0x0100 == 0x1011`. A bitenkénti eltolás
pedig a kettőhatvánnyal való szorzásnak illetve osztásnak felel meg. Ha például egy értéket 1-el balra eltolunk, az pont ugyanaz,
mintha 2-vel felszoroztuk volna.</p>

<p>**Léptető**
```
++ --
```
Növelés és csökkentés. Ezeket előtagként és utótagként is lehet használni. Ha előtag, például `++a`, akkor megnöveli a változó
értékét, majd visszaadja a megnövelt értéket. Ha azonban utótag `a++`, akkor szintén megnöveli az értéket, de a nővelés *előtti*
értéket adja vissza.</p>

```c
a = 0; b = ++a * 3;     /* a == 1, b == 3 */
a = 0; b = a++ * 3;     /* a == 1, b == 0 */
```

<p>**Feltételes**
```
?:
```
Ez egy háromértékes kifejezés, három operandussal, amiket `?` és `:` választ el. Ha az első operandus igaz, akkor az egész
kifejezés lecserélődik a második operandusra, egyébként a harmadikra. Emiatt a második és a harmadik operandus típusának
meg kell egyeznie. Például `a >= 0 ? "pozitív" : "negatív"`.</p>

<p>**Értékadás**
```
= *= /= %= += -= ~= &= |= <<= >>=
```
Az első elvégzi a műveletet, majd az eredményt a változóba helyezi. A többi ezen a változón végzi el a műveletet, majd az
eredményt ugyanebbe a változóba helyezi. Például e kettő ugyanaz: `a *= 3;` és `a = a * 3;`.</p>

A többi nyelvvel ellentétben a C-ben az értékadás is egy művelet. Ez azt jelenti, hogy bárhol előfordulhatnak egy kifejezésben,
például `a > 0 && (b = 2)`. Ezért van az, hogy az értékadás a `=`, míg a logikai egyenlő `==`, hogy lehessen használni mindkettőt
ugyanabban a kifejezésben.

Van még a címe operátor, a `&`, ami a változó címét adja vissza. Ez akkor használatos, ha valahol a MEG-4 API `addr_t` cím
paramétert vár.

Az operátorok a precedenciasorrendjük szerint hajtódnak végre, például a `1+2*3` két operátort tartalmaz, a `+`-t és `*`-t,
azonban a `*` precedenciája magasabb, így először a `2*3` számítódik ki, és csak azután az `1+6`. Ezért a végeredmény 7 és nem 9.

<h2 c_flow>Vezérlésirányítás</h2>

Míg a BASIC alatt a vezérlésátadás elsősorban címkékkel történik, addig a C ún. strukturált nyelv, azaz inkább az utasításokat
megfelelő blokkokba helyezve tagolja a programot. Ha egynél több utasítást akarunk egyben kezelni, akkor azokat `{` és `}` közé
kell tenni (de lehet egy utasítás is a blokkon belül).

Mint minden más, a feltételes vezérlésátadások is ilyen blokkokat használnak:

```c
if(a != b) {
    printf("a nem egyenlő b-vel\n");
} else {
    printf("a egyenlő b-vel\n");
}
```

Megadható egy `else` ág, ami akkor fut le, ha a kifejezés értéke hamis, de az `else` használata nem kötelező.

Többféle lehetséges érték esetén használható a `switch` utasítás. Itt minden `case` úgy viselkedik, mint egy cimke, és attól
függően lesz kiválasztva, hogy a kifejezés értéke mennyi.

```c
switch(a) {
    case 1: printf("a értéke 1.\n");
    case 2: printf("a értéke vagy 1 vagy 2.\n"); break;
    case 3: printf("a értéke 3.\n"); break;
    default: printf("a értéke valami más.\n"); break;
}
```

Van egy speciális cimke, a `default` (jelentése alapértelmezett), amire akkor ugrik a program, ha az értékhez nincs külön `case`.
Ezek a blokkok összefüggőek, azaz ha a vezérlés az egyik `case` cimkére adódik, akkor az a blokk, illetve minden további, azt
követő blokk is végrehajtódik. A fenti példában ha az `a` értéke 1, akkor mindkét printf le fog futni. Ha ezt nem akarjuk, akkor
a `break` utasítást kell használni, hogy kilépjünk a `switch`-ből.

A C nyelv háromféle ismétléstípust ismer: elöltesztelős ciklus, hátultesztelős ciklus és számlálóciklus.

Az elöltesztelős ciklus előbb ellenőrzi a kifejezés értékét, és csak akkor futtatja le a ciklusmagot, ha az nem hamis.

```c
while(a != 1) {
    ...
}
```

A hátultesztelős ciklus mindenképp lefuttatja a ciklusmagot legalább egyszer, csak ezt követően ellenőrzi a kifejezést és akkor
ismétel, ha az igaz.

```c
do {
    ...
} while(a != 1);
```

A számlálóciklus C-ben eléggé univerzális. Három kifejezést vár, rendre: inicialiálás, feltétel, léptetés. Mivel szabadon adhatod
meg ezeket, ezért lehetséges akár több változót is használni vagy bármilyen kifejezést megadni (nem feltétlenül számlálóst).
Példa:

```c
for(a = 0; a < 10; a++) {
    ...
}
```

Ez pontosan ugyanaz, mintha ezt írtuk volna:

```c
a = 0;
while(a < 10) {
    ...
    a++;
}
```

A ciklusmag blokkjából is ki lehet lépni a `break` (megszakít) utasítással, de itt használható még a `countinue` (folytatás) is,
ami ugyancsak megszakítja a ciklusmag blokkjának futtatását, de ahelyett, hogy kilépne, a következő iterációtól folytatja.

```c
for(a = 0; a < 100; a++) {
    if(a < 10) continue;
    if(a > 50) break;
    printf("a értéke 10 és 50 között: %d\n", a);
}
```

<h2 c_funcs>Függvények</h2>

INFO: Csak függvénytörzsön belül használhatók utasítások.

A programodat fel kell darabolnod kissebb programokra, amiket aztán többször is lefuttathatsz, ezeket hívjuk függvényeknek. A
deklarálásukhoz meg kell adni a visszatérési értékük típusát, a nevüket, a paramétereik listáját `(` és `)` zárójelek között,
majd pedig a függvénytörzsüket egy `{` és `}` közötti blokkban. Kettő ezek közül, a `setup` és a `loop` különleges jelentéssel
bír, bővebben lásd a [kód szerkesztő]-nél. A C nyelv nem tesz különbséget alrutinok és függvények között; itt minden függvény.
Az egyetlen különbség az, hogy azok a függvények, amik nem adnak vissza semmit, a visszatérési értékük típusa `void` (semmi).

```c
void ott_vagyunk_már(int A)
{
 if(A > 0) {
  printf("Nem, még nem\n");
  return;
 }
 printf("IGEN! Dolgok, amiker érkezéskor akartál megtenni\n");
 return;
}

void setup()
{
 /* egyszer */
 ott_vagyunk_már(1);
 /* aztán mégegyszer */
 ott_vagyunk_már(0);
}
```

A függvényeket a programodból simán a nevükkel hívhatod, zárójelben a paramétereikkel (a zárójel mindenképp kötelező, akkor is,
ha nincs paramétere). Nincs külön parancs, és nincs különbség a hívásban aközött, ha van visszatérési érték, vagy ha nincs.

A függvényből visszatérni a hívóra a `return;` utasítással kell. Amennyiben a függvénynek van visszatérési értéke, akkor
azt a `return` után kell egy kifejezésben megadni, aminek a típusa pont ugyanolyan kell legyen, mint a függvényé. A `return`
használata (akár van visszatérési érték, akár nincs) kötelező.

```c
str_t sztringesfüggvényem()
{
 return "egy sztring";
}

void setup()
{
 a = sztringesfüggvényem();
}
```

<h2 c_api>Elérhető függvények</h2>

C nyelvben nincs speciális kiíró vagy bekérő parancs; egész egyszerűen csak MEG-4 API hívásokat kell meghívni. A [getc] egy
karaktert olvas be, a [gets] egy sztringet, kiírni sztringeket pedig a [printf]-el lehet.

A MEG-4 C pontosan úgy használja a MEG-4 API függvényeit, ahogy azok ebben a leírásban szerepelnek, nincs semmiféle trükközés,
nincs átnevezés, se utótagok sem konverzió.
