Lua
===

Ha ezt a nyelvet választod, akkor kezd a programodat a `#!lua` sorral.

<h2 ex_lua>Példa program</h2>

```lua
#!lua

-- globális változók
szamlalo = 123
szam = 3.1415
cim = 0x0048C
sztring = "valami"
tomb = {}

-- Induláskor lefuttatandó dolgok
function setup()
  -- lokális változók
  lokalisvagyok = 234
end

-- Minden képkockánál lefuttatandó dolgok, 60 FPS
function loop()
  -- Lua stílusú print
  print("Épp", "futok")
  -- MEG-4 stílusú kimenet
  printf("a számláló %d, balshift %d\n", szamlalo, getkey(KEY_LSHIFT))
end
```

További információk
-------------------

A többi nyelvvel ellentétben ez nem szerves része a MEG-4-nek, hanem egy külsős szoftver. Emiatt nincs (és nem is lehet)
tökéletesen integrálva (például nincs debuggolója és a hibaüzenetek sincsenek lefordítva). Maga a futtató a többi nyelvhez képest
iszonyat lassú, de működik, használható.

A beágyazott verzió Lua 5.4.5, több módosítással. A biztonság érdekében a nyelvből kikerült a konkurencia, valamint a dinamikus
modulkezelés, fájlelérés, csővezetékek, parancsfuttatás. A `coroutine`, `io` és `os` függvénycsomagok és a függvényeik nincsenek
(de a nyelv eszköztára és a baselib összes többi része továbbra is elérhető). Bekerült ezek helyett a MEG-4 API, ami pár apró,
lényegtelen eltéréssel a jobb integráció kedvéért ugyanúgy használható, mint a többi nyelvnél.

Amennyiben érdekel ez a nyelv, magyarul [itt találsz](http://nyelvek.inf.elte.hu/leirasok/Lua) róla bővebb információt, illetve a
hivatalos [Programming in Lua](https://www.lua.org/pil) útmutató (angolul).

API Eltérések
-------------

- [memsave] egyformánt elfogad MEG-4 memória címet (integer) vagy Lua táblát integer számokkal (ami egy bájttömb akar lenni).
- [memload] híváskor mindenképp egy érvényes MEG-4 memória címet vár, de aztán Lua táblát ad vissza. Ha nem speciális MMIO
    területre akarsz betölteni, akkor `MEM_USER`-t (0x30000-et) adj meg, és csak használd az adatokat a visszaadott Lua táblában.
- [memcpy] paramétere lehet két MEG-4 memória cím, ahogy megszokott, de az egyik lehet Lua tábla is (de csak az egyik, mindkettő
    nem). Ezzel a funkcióval lehet adatokat másolni a MEG-4 memória és a Lua között (de az [inb] és az [outb] is működik).
- [remap] csak Lua táblát fogad el (amiben 256 integer számnak kell lennie).
- [maze] utolsó két paramétere (`numnpc` és `npc`) helyett lehet használni egy darab Lua táblát (amiben minden elem egy újabb Lua tábla).
- [printf], [sprintf] és [trace] esetén nem a MEG-4 szabályait követi a [formázó sztring], hanem a Lua-ét (habár e kettő majdnem teljesen ugyanaz).
