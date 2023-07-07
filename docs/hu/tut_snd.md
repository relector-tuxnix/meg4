Hangeffekt
==========

Ebben az oktatóanyagban előkészítünk és beimportálunk egy hangeffektet. Ehhez az Audacity programot fogjuk használni, de bármelyik másik hullámszerkesztő is megfelel a célra.

Hullám betöltése
----------------

Először is, töltsük be a hanghullám fájlt az Audacity-be.

<imgc ../img/tut_snd1.png><fig>Hanghullám megnyitása Audacity-ben</fig>

Egyből látjuk, hogy két hullám is van, ami azt jelenti, hogy a mintánk sztereó. A MEG-4 csak mono mintákat kezel, ezért
válasszuk a `Tracks` > `Mix` > `Mix Stereo Down to Mono` menüpontot a konvertáláshoz. Ha eleve csak egy hullámod van, akkor ez a lépés kihagyható.

Hangmagasság és hangerő
-----------------------

Mivel a MEG-4 magától hangolja a mintákat, ezért az importált mintának egy adott hangmagasságúnak kell lennie. Valamiért a hangmagasságérzékelés bugos az Audacity-ben,
ezért kézzel kell elvégezni. Üsd le a <kbd>Ctrl</kbd>+<kbd>A</kbd>-t a teljes hullám kijelöléséhez, majd menj az `Analyze` > `Plot Spectrum...` menüpontra.

<imgc ../img/tut_snd2.png><fig>Spektrumanalízis</fig>

Mozgasd az egeredet a legnagyobb csúcs fölé, és az aktuális hangmagasság alul meg fog jelenni (a példánkban ez `C3`). Ha a hangjegy nem `C4`, akkor válaszd az `Effect` >
`Pitch and Tempo` > `Change Pitch...` menüpontot.

<imgc ../img/tut_snd3.png><fig>Hangmagasság állítása</fig>

A "from" mezőbe írd azt a hangjegyet amit a spektrumanalízisnél láttál, a "to" mezőt pedig állítsd a C-4 hangra, majd kattints az "Apply" gombra.

<imgc ../img/tut_snd4.png><fig>Hangerő állítása</fig>

A következő a hangerő normalizálása. Menj az `Effect` > `Volume and Compression` > `Amplify...` menüpontra. A felugró ablakban kattints az "Apply" gombra
(minden beállítást helyesen detektál, nem kell állítani semmit).

Minták száma
------------

A MEG-4 nem kezel több, mint 16376 mintát egy hullámban. Ha eleve ennél kevesebb mintából áll a hullámod, akkor ez a lépés kihagyható.

A hullámminta képe alatt láthatod a kijelölést századmásodpercekben, kattints itt az "s"-re, és váltsd át "samples"-re.

<imgc ../img/tut_snd5.png><fig>Mértékegység megváltoztatása</fig>

A példánkban ez több, mint a megengedett maximum. A minták száma úgy számítódik, hogy a mintavételezési ráta értéket megszorozzuk a hullámunk időtartamával. Szóval
hogy csökkentsük a mintaszámot, vagy csökkentjük a rátát, vagy lecsapunk a hullámunk végéből. Ebben az oktatóanyagban mindkettőt megtesszük.

Kijelöltem mindent mondjuk úgy 1.0 után, majd a <kbd>Del</kbd> leütésével töröltem. Ez működik, viszont a hullám vége hirtelen vágódik el és csúnya hangja lesz. Hogy ezt
kiigazítsuk, jelöljünk ki a hullám végéből egy valamekkora darabot, majd válasszuk az `Effect` > `Fading` > `Fade Out` menüpontot. Ennek hatására a hullám vége szépen elhalkul.

<imgc ../img/tut_snd6.png><fig>Hullám levágása és a vég elhalkítása</fig>

A hullámunk még mindig túl hosszú (44380 minta), de többet már nem lehet levágni belőle annélkül, hogy elrontanánk. Itt jön képbe a mintavételezési ráta. Az Audacity korábbi
verzióiban ez kényelmesen alul az eszköztáron látszott, mint "Project Rate (Hz)". De többé már nem, az újabb Audacity-nél ez sokkal macerásabb lett. Először is kattintsunk
az `Audio Setup` gombra az eszköztáron, majd válasszuk ki a `Audio Settings...`-et. A felugró ablakban keressük meg a "Quality" / "Project Sample Rate" sort, és mellette állítsuk
az opciólistát "Other..."-re, hogy a tényleges mező szerkeszthetővé váljon.

WARNING: Bizonyosodj meg róla, hogy jó számot adsz meg! Az Audacity nem képes visszavonni ezt a lépést, ezért nem próbálkozhatsz újra!

Ide egy olyan számot kell beírni, ami 16376 osztva a hullámunk időtartamával (1.01 másodperc a példánkban), majd kattintsunk az "OK"-ra.

<imgc ../img/tut_snd7.png><fig>Mintaszám csökkentése</fig>

Jelöljünk ki az egész hullámmintát (<kbd>Ctrl</kbd>+<kbd>A</kbd> leütésével), és alul a kijelölés végénél az kell látnunk, hogy az kevesebb, mint 16376.

Elmentés és beimportálás
------------------------

Végezetül, mentsd el a módosított hullámmintát, válaszd a `File` > `Export` > `Export as WAV` menüpontot. Győzödj meg róla, hogy a kódolás (Encoding)
"Unsigned 8-bit PCM"-re van állítva. Fájlnévnek `dspXX.wav`-t adj meg, ahol az `XX` egy hexa szám, `01` és `1F` között, az a MEG-4 hullámhely, ahová be szeretnéd importálni
(használhatsz más nevet is, ekkor a legelső szabad helyre fog betöltődni).

<imgc ../img/tut_snd8.png><fig>A beimportált hullámminta</fig>

Amint megvan a fájl, egyszerűen csak húzd rá a MEG-4 ablakára és kész is.
