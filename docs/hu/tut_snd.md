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

Mivel a MEG-4 magától hangolja a mintákat, ezért az importált mintának egy adott hangmagasságúnak kell lennie. Üsd le a <kbd>Ctrl</kbd>+<kbd>A</kbd>-t
a teljes hullám kijelöléséhez, majd menj az `Analyze` > `Plot Spectrum...` menüpontra.

<imgc ../img/tut_snd2.png><fig>Spektrumanalízis</fig>

Valamiért a csúcsérzékelés az Audacity-ben bugos, ezért kézzel kell elvégezni. Mozgasd az egeredet a legnagyobb csúcs fölé, és az aktuális hangmagasság alul
meg fog jelenni (a példánkban ez C3). Ha a hangjegy nem C-4, akkor válaszd az `Effect` > `Pitch and Tempo` > `Change Pitch...` menüpontot.

<imgc ../img/tut_snd3.png><fig>Hangmagasság állítása</fig>

A "from" mezőbe írd azt a hangjegyet amit a spektrumanalízisnél láttál, a "to" mezőt pedig állítsd a C-4 hangra, majd kattints az "Apply" gombra.

<imgc ../img/tut_snd4.png><fig>Hangerő állítása</fig>

A következő a hangerő normalizálása. Menj az `Effect` > `Volume and Compression` > `Amplify...` menüpontra. a felugró ablakban kattints az "Apply" gombra
(minden beállítást helyesen detektál, nem kell állítani semmit).

Minták száma
------------

A MEG-4 nem kezel több, mint 16376 mintát egy hullámban. A hullámminta képe alatt láthatod a kijelölést századmásodpercekben, kattints itt az "s"-re, és váltsd át "samples"-re.

<imgc ../img/tut_snd5.png><fig>Mértékegység megváltoztatása</fig>

A példánkban ez több, mint a megengedett maximum. A minták száma úgy számítódik, hogy a "Project Rate (Hz)" alatti értéket megszorozzuk a hullámunk időtartamával. Szóval
hogy csökkentsük a mintaszámot, vagy csökkentjük a frekvenciát, vagy lecsapunk a hullámunk végéből. Ebben az oktatóanyagban mindkettőt megtesszük.

Kijelöltem mindent 1.0 után, majd a <kbd>Del</kbd> leütésével töröltem. Ez működik, viszont a hullám vége hirtelen vágódik el és csúnya hangja lesz. Hogy ezt kiigazítsuk,
jelöljünk ki a hullám végéből egy valamekkora darabot, majd válasszuk az `Effect` > `Fading` > `Fade Out` menüpontot. Ennek hatására a hullám vége szépen elhalkul.

<imgc ../img/tut_snd6.png><fig>Hullám levágása és a vég elhalkítása</fig>

A hullámunk még mindig túl hosszú, de többet már nem lehet levágni belőle. Itt jön a "Project Rate (Hz)" a képbe. Csökkentsük addig, míg a minták száma nem csökken 16376 alá.

<imgc ../img/tut_snd7.png><fig>Mintaszám csökkentése</fig>

Ha ennél kevesebb mintából áll a hullámod, akkor ez a lépés kihagyható.

Elmentés és beimportálás
------------------------

Végezetül, mentsd el a módosított hullámmintát, válaszd a `File` > `Export` > `Export as WAV` menüpontot. Győzödj meg róla, hogy a kódolás (Encoding)
"Unsigned 8-bit PCM"-re van állítva. Fájlnévnek `dspXX.wav`-t adj meg, ahol az `XX` egy hexa szám, `01` és `1F` között, az a MEG-4 hullámhely, ahová be szeretnéd importálni
(használhatsz más nevet is, ekkor a legelső szabad helyre fog betöltődni).

<imgc ../img/tut_snd8.png><fig>A beimportált hullámminta</fig>

Amint megvan a fájl, egyszerűen csak húzd rá a MEG-4 ablakára és kész is.
