BASIC
=====

Ha ezt a nyelvet választod, akkor kezd a programodat a `#!bas` sorral.

<h2 ex_bas>Példa program</h2>

```bas
#!bas

REM globális változók
LET számláló% = 123
LET szám = 3.1415
LET cím% = $0048C
LET sztring$ = "valami"
DIM tömb(10)

REM Induláskor lefuttatandó dolgok
SUB setup
    REM lokális változók
    LET lokálisvagyok = 123
END SUB

REM Minden képkockánál lefuttatandó dolgok, 60 FPS
SUB loop
    REM BASIC stílusú print
    PRINT "Épp"; " futok"
    REM MEG-4 stílusú kimenet
    GOSUB printf("a számláló %d, balshift %d\n", számláló%, getkey%(KEY_LSHIFT))
END SUB
```

Dialektus
---------

A BASIC egy mozaikszó, Beginners' All-purpose Symbolic Instruction Code, ami annyit tesz, Kezdők Általános célú Szimbólikus
Utasítás Kódja. Kifejezetten azzal a céllal hozta létre Kemény János 1963-ban, hogy gyerekeket tanítsanak vele programozni. A
**MEG-4 BASIC** azért ennél kicsit modernebb, tudja a teljes ANSI X3.60-1978 (ECMA-55) szabványt és még jónéhány dolgot az ANSI
X3.113-1987 (ECMA-116) szabványból is, apró eltérésekkel. Az azonosítók hosszabbak lehetnek két karakternél, és a lebegőpontos
valamint integer aritmetikája is 32 bites. A legfontosabb különbségek: nincs interaktív mód, így az utasításokat nem kell
sorszámozni (helyette lehet cimkéket használni), illetve a BASIC kulcsszavak kis-nagybetű érzéketlenek, ahogy azt a specifikáció
elvárja, ellenben a változó és függvénynevek *kis-nagybetű érzékenyek*. Az összes MEG-4 API függvényhívás kisbetűs; a többit
szabadon választhatod, de azok is kis-nagybetű érzékenyek (például az `ALMA`, `Alma` és az `alma` három különböző változót takar).

Van egy nem szabványos kulcsszava, a `DEBUG`, amit akárhová elhelyezhetsz a programodban, és ami meghívja a beépített
[debuggoló]t.  Ezt követően lépésről lépésre hajthatod végre a programodat, ellenőrizve, hogy épp mit csinál.

Következzen a részletes leírás példákkal és minden eltérés kiemelve.

<h2 bas_lit>Literálok</h2>

A MEG-4 BASIC elfogad tízes számrendszerbeli számokat (akár integer egészszámok, vagy lebegőpontosak, tudományos jelöléssel vagy
annélkül). Hexadecimális számokat a `$` karakterrel kell kezdeni (nincs a specifikációban, de így csinálta a Commodore BASIC és
nagyjából az összes többi 80-as évekbeli dialektus is), a sztringeket pedig macskakörömbe kell tenni:

```bas
42
3.1415
.123e-10
$0048C
"Viszlát és kösz a halakat!\n"
```

WARN: A specikiáció szerint 7 bites ASCII-nak kellene lennie, de a MEG-4 BASIC nullával lezárt UTF-8 kódolt sztringeket használ.
Elfogad továbbá C-beli kiemelő kódokat is (például `\"` a macskaköröm, `\t` a tab, `\n` az újsor karakter), és a sztringek
maximális hossza 255 bájtnyi (a specifikáció 18 bájtot vár el).

<h2 bas_var>Változók</h2>

A változókat nem kell deklarálni, helyette az azonosítójuk utolsó betűje adja meg a típusukat. Ez lehet `%` egészszámoknál, `$`
sztringeknél, és ez nincs a specifikációban, de a MEG-4 BASIC esetében a `!` bájtot, a `#` pedig duplabájtot jelent (szó). Bármi
más lebegőpontos számot tároló változót eredményez.

```bas
LET A% = 42
LET B! = $FF
LET C# = $FFFF
LET D$ = "sztring"
LET E = 3.1415
```

A konverzió bájt, integer és a lebegőpontos számok között teljesen automatikus. Azonban ha egy sztringet akarsz szám változóba
tenni, vagy szám literált egy sztring változóba, az hibát eredményez (ezekhez használnod kell a `STR$` illetve `VAL` hívásokat).

Amikor értéket adsz egy változónak, a `LET` parancs elhagyható.

Literálok megadhatók még a programodban a `DATA` állítások segítségével is, amiket aztán a `READ` utasítás rendel változókhoz. A
`READ` pontosan annyi adatot olvas be, ahány változó meg lett adva a paramétereként, és többször is hívható. Hogy visszaállítsuk,
hogy a legelső `DATA` utasítástól kezdve olvasson a `READ` megint, ahhoz a `RESTORE` parancsot kell kiadni.

```bas
RESTORE
READ név$, jövedelem
DATA "Jóska", 1234
DATA "Pista", 2345
```

Van pár speciális változó, amiket a rendszer biztosít. Az `RND` egy lebegőpontos véletlenszámot ad vissza, 0 és 1 között,
az `INKEY$` visszaadja a felhasználó által leütött billentyűt vagy üres sztringet, `TIME` a gép bekapcsolása óta eltelt
ezredmásodpercek számát, míg a `NOW%` a greenwichi középidő szerinti 1970 január 1.-e éjfél óta eltelt másodperceket adja vissza.

<h2 bas_arr>Tömbök</h2>

Több, azonos típusú elem rendelhető egyetlen változóhoz, ezeket hívjuk tömböknek.

```bas
DIM A(10)
DIM B(10, 10)
DIM C(1 TO 6)
```

A BASIC specifikáció elvárja a kétdimenziós tömbök kezelését, de a MEG-4 BASIC 4 dimenziót is támogat. A tömb elemei lehetnek bájtok,
egészszámok, lebegőpontos számok vagy sztringek. Dinamikusan nem lehet átméretezni őket a `REDIM`-el, minden tömb statikusan lesz
lefoglalva akkorára, amekkora a `DIM`-nél meg lett adva. Ha a méret nincs kiírva, akkor egy dimenziót és 10 elemet feltételez.

WARN: Az elemek indexe 1-től indul (mint az ANSI szerint, és nem 0-tól, mint az ECMA-55 szerint). Az `OPTION BASE` utasítás
nem támogatott, viszont az induló index átállítható minden tömb esetén a `TO` kulcsszóval.

<h2 bas_op>Műveletek</h2>

Precedenciájuk szerint csökkenő sorrendben:

<p>**Aritmetikai**
```
^
* / MOD
+ -
```
Az első a hatványozás (pl. b a négyzeten `b^2`), aztán szorzás, osztás, a `MOD` pedig a modulus (azaz osztás utáni maradék, pl.
`10 MOD 3` az 1), összeadás, kivonás.</p>

<p>**Relációs**
```
<> =
<= >= < >
```
Nem egyenlő, egyenlő, kissebb egyenlő, nagyobb egyenlő, kissebb, nagyobb.</p>

<p>**Logikai**
```
NOT
AND
OR
```
Tagadás (0-ból 1 lesz, minden nem nullából pedig 0), és (csak akkor 1, ha mindkét paramétere nem 0), vagy (legalább az egyik
paramétere nem 0).</p>

Van egy nem szabványos operátor, a `@` a változó címét adja vissza. Ez akkor használatos, ha valahol a MEG-4 API `addr_t` cím
paramétert vár.

Az operátorok a precedenciasorrendjük szerint hajtódnak végre, például a `1+2*3` két operátort tartalmaz, a `+`-t és `*`-t,
azonban a `*` precedenciája magasabb, így először a `2*3` számítódik ki, és csak azután az `1+6`. Ezért a végeredmény 7 és nem 9.

<h2 bas_flow>Vezérlésirányítás</h2>

Az `END` utasítás leállítja a vezérlést (kilép a programod).

A MEG-4 BASIC nem használ már sorszámokat, helyette cimékkel támogatja a `GOTO` parancsot, például:

```bas
ez_egy_cimke:

GOTO ez_egy_cimke
```

WARN: Néhány BASIC dialektus megengedi, hogy több `:`-el elválasztott parancsot egy sorba írj. MEG-4 BASIC alatt a `:` a cimkét
jelenti, ezért minden parancsot külön sorba kell írni (ahogy azt az ECMA-55 is elvárja).

A feltételes vezérlésátadások szintén cimkéket használnak:

```bas
IF a$ <> b$ THEN ez_egy_cimke
ON a GOTO cimke1, cimke2, cimke3 ELSE egyébcimke
```

Az `ON` .. `GOTO` mindenképp egy numerikus kifejezést vár, és az annyadik cimkére ugrik, 1-től kezdve (ha a kifejezés nulla
vagy negatív, az mindenképpen az `ELSE` ágnak minősül). Nincs `ON` .. `GOSUB`, mert a GOSUB nem fogad el cimkéket a MEG-4
BASIC-ben.

WARN: A `GOSUB` utasítás nem fogad el cimkéket, és kicsit másképp működik MEG-4 BASIC alatt, lásd alább.

Az `IF` utasítás elfogad egyaránt numerikus és relációs kifejezést (minden nem nulla eredményt igaznak vesz), sőt mi több, a
többsoros `IF` .. `THEN` .. `ELSE` .. `END IF` blokkok is támogatottak (de a `SELECT CASE` nem).

```bas
IF var >= 0 THEN
 PRINT "var értéke pozitív"
ELSE
 PRINT "var értéke negatív"
END IF
```

Kivételként egy parancs lehet az egysoros `IF` után, amennyiben az a `GOTO` vagy az `END`:

```bas
IF a < 0 THEN GOTO cimke
IF b > 42 THEN END
```

Az ismétlő utasítás, a számlálósciklus elöltesztelős (nem futtatja le a blokkot, ha az indulóérték nagyobb (vagy kissebb), mint
a határ), és így néz ki:

```bas
FOR i = 1 TO 100 STEP 2
...
NEXT i
```

Ez a `FOR` .. `NEXT` lényegében pontosan ugyanaz, mint:

```bas
LET i = 1
LET lim = 100
LET inc = 2
sor1:
IF (i - lim) * SGN(inc) > 0 THEN sor2
...
LET i = i + inc
GOTO sor1
sor2:
```

A ciklusváltozónak lebegőpontos típusú változónak kell lennie. A `STEP` opcionális (ha nincs megadva, akkor 1.0), utánna a léptetés
lehet egy lebegőpontos literál vagy egy másik lebegőpontos változó. A kiindulási érték és a határ lehet összetettebb kifejezés is,
de azoknak is lebegőpontos számot kell visszaadniuk.

```bas
FOR i = (1+2+a)*3 TO 4*(5+6)/b+c STEP j
...
NEXT i
```

WARN: A specifikációval ellentétben, ami több változót is megenged a `NEXT` után, a MEG-4 BASIC *csak egyet* fogad el. Ezért ha
egymásbaágyazott ciklusaid vannak, akkor több `NEXT` parancsot kell használnod, pontosan annyit, mint ahány `FOR` utasításod van.

```bas
FOR y = 1 TO 10
 FOR x = 1 TO 100
 ...
 NEXT x
NEXT y
```

A MEG-4 BASIC nem ismer többfajta ciklust mint például a C, de egy nem-számlálós elöltesztelős ciklus helyett írhatod ezt:

```bas
megint:
IF a > 0 THEN
 ...
 GOTO megint
END IF
```

A hátultesztelős ciklus helyett pedig ezt:

```bas
megint:
 ...
IF a > 0 THEN megint
```

<h2 bas_funcs>Alrutinok és függvények</h2>

INFO: Azok az utasítások, amik nincsenek egyetlen alrutinban sem, úgy lesznek kezelve, mintha a `setup` alrutinban lennének.

A programodat feldarabolhatod kissebb programokra, amiket aztán többször is lefuttathatsz, ezeket hívjuk alrutinoknak. Ezeket
`SUB` és `END SUB` közötti blokkban kell elhelyezni. Kettő ezek közül, a `setup` és a `loop` különleges jelentéssel bír, bővebben
lásd a [kód szerkesztő]-nél. Ahogy már megjegyeztük, a MEG-4 BASIC-ben a `GOSUB` nem fogad el cimkéket, mégpedig azért, mert itt
csakis ilyen alrutinneveket lehet paraméterül adni neki.

```bas
SUB alrutinom
 PRINT "valami, amit többször is szeretnél lefuttatni"
END SUB

REM egyszer
GOSUB alrutinom
REM aztán mégegyszer
GOSUB alrutinom
```

A vezérlés a `GOSUB` sornál adódik át, és a `GOSUB` utánni sorra tér vissza, amikor `END SUB` (vagy az opcionális `RETURN`)
parancshoz ér. Az alrutinok elérik a globális változókat és saját paramétereik is lehetnek.

```bas
SUB ott_vagyunk_már(A)
 IF A > 0 THEN
  PRINT "Nem, még nem"
  RETURN
 END IF
PRINT "IGEN! Dolgok, amiker érkezéskor akartál megtenni"
END SUB

REM egyszer
GOSUB ott_vagyunk_már(1)
REM aztán mégegyszer
GOSUB ott_vagyunk_már(0)
```

A függvények roppant hasonlók, de ott mindenképp kell a `RETURN` és a `RETURN` utasításnak kell hogy legyen egy paramétere,
méghozzá pont olyan típusú, mint amit a függvény neve jelez. A függvényeket a programodból simán a nevükkel hívhatod, zárójelben
a paramétereikkel (a zárójel mindenképp kötelező, akkor is, ha nincs paramétere). Például:

```bas
FUNCTION sztringesfüggvényem$()
 RETURN "egy sztring"
END FUNCTION

LET a$ = sztringesfüggvényem$()
```

<h2 bas_print>Print utasítás</h2>

```
PRINT kifejezés [;|,] [kifejezés [;|,] [kifejezés [;|,]]] ...
```

Kiír egy vagy több kifejezést a képernyőre. Ha a kifejezések `;` kettősponttal vannak elválasztva, akkor szorosan egymásután.
Ha `,` vesszővel, akkor oszlopokra tagoltan. A számok elé mindenképp kitesz egy szóközt, és ha a lista egy kifejezéssel zárul
(azaz nem `;` és nem `,` az utasítás vége), akkor egy újsor karaktert is kiír a végére.

<h2 bas_input>Input utasítás</h2>

```
INPUT "kérdés" [;|,] változó
```

Kiírja a kérdést, majd bekér egy értéket a felhasználótól, és letárolja azt a megadott változóban. Ha a kérdés és a változó
`,` vesszővel van elválasztva, akkor a kérdés után egy `?` kérdőjelet is kitesz.

WARN: Az ECMA-55 specifiákció több változót is megenged, de a MEG-4 BASIC csak egyet fogad el.

<h2 bas_spec>Peek és Poke</h2>

<p>Ezekkel a parancsokkal direktben elérhető a MEG-4 memóriája, így az MMIO terület is.</p>

**Beolvasás**
```
változó = PEEK(cím)
```

Beolvassa a megadott címen lévő bájtot, lebegőpontos számmá alakítja, és elhelyezi a megadott változóban.

Például, hogy lekérdezzük, a billentyűzetsor üres-e:
```bas
IF PEEK($1A) <> PEEK($1B) THEN
```

**Kíírás**
```
POKE cím, kifejezés
```

Kiszámítja a kifjezés értékét, bájttá alakítja, majd azt a bájtértéket kiírja a megadott memóriacímre.

Például, hogy átállítsuk a palettát az 1-es színkód esetén:
```bas
REM piros összetevő
POKE $84, 10
REM zöld összetevő
POKE $85, 10
REM kék összetevő
POKE $86, 10
REM alfa (átlátszóság)
POKE $87, 255
```

<h2 bas_api>Elérhető függvények</h2>

Néhány MEG-4 API hívás rendszerváltozóként érhető el, `RND` ([rnd]), `TIME` ([time]), `NOW%` ([now]), és `INKEY$` ([getc]).

Néhány másik parancsként, `INPUT` ([gets] + [val]), `PRINT` ([printf]), `PEEK` ([inb]), `POKE` ([outb]).

Hogy megfeleljen az ECMA-55 szabványnak, két további függvény át lett nevezve: `SQR` ([sqrt]) és `ATN%` ([atan]). Ezeken kívül
minden más pontosan úgy használható, ahogy ebben a dokumentációban szerepelnek, kivéve, hogy a visszatérési értékük típusának
megfelelő utótagot kaptak (például az [str] sztringet ad vissza, ezért `STR$` lett belőle).

Fontos, hogy az ECMA-55 a trigonometrikus függvényeknél radiánt vár alapból (és egy `OPTION` paranccsal lehet fokokra váltani),
de a MEG-4 API mindig fokokat használ, 0-tól 359-ig, ahol a 0 a felfele irány és 90 a jobbra. Ezért van az, hogy az `ATN%` egy
egészszám típus utótagot kap például, mivel fokokat ad vissza egészszámban.
