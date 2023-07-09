Lua
===

If you want to use this language, then start your program with a `#!lua` line.

<h2 ex_lua>Example Program</h2>

```lua
#!lua

-- global variables
acounter = 123
anumber = 3.1415
anaddress = 0x0048C
astring = "something"
anarray = {}

-- Things to do on startup
function setup()
  -- local variables
  iamlocal = 234
end

-- Things to run for every frame, at 60 FPS
function loop()
  -- Lua style print
  print("I", "am", "running")
  -- Get MEG-4 style outputs
  printf("a counter %d, left shift %d\n", acounter, getkey(KEY_LSHIFT))
end
```

Further Info
------------

Unlike the other languages, this isn't integral part of MEG-4, rather provided by a thrid party library. Due to that it does
not (and cannot) have perfect integration (no debugger and no translated error messages for example). Its runner is bloated
and much slower compared to the other languages, but it works, and you can use it.

The embedded version is Lua 5.4.6, with modifications. For security reasons it lacks concurrency, as well as module loading, file
access, pipes, command execution. The `coroutine`, `io` and `os` modules and their functions aren't available (but the language
features and all the other parts of the baselib are still there). Instead of these, it has the MEG-4 API, which can be used as in
any other language, with some slight, minor differences for better integration.

If you're interested in this language then you can find more information in the [Programming in Lua](https://www.lua.org/pil)
documentation.

API Differences
---------------

- [memsave] can accept either a MEG-4 memory address (integer) or a Lua table with integers (supposed to be a byte array).
- [memload] on call, a valid MEG-4 memory address must be passed, but then it returns a Lua table anyway. If you don't want to
    load to a specific MMIO area, then specify `MEM_USER` (0x30000) and just simply use the data in the returned Lua table.
- [memcpy] parameters can be two MEG-4 memory addresses as usual, or one of them can be a Lua table (just one, not both). You
    can use this function to copy data between MEG-4 memory and Lua (but [inb] and [outb] also works).
- [remap] only accepts a Lua table (with 256 integer values).
- [maze] instead of the last two parameters (`numnpc` and `npc`) one single Lua table (with each element being another table) can be used.
- [printf], [sprintf] and [trace] does not use MEG-4's [format string] rules, but Lua's (however these two are almost entirely identical).
