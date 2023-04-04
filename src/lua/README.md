This is a stripped down and security hardened Lua. **DO NOT** use the stock Lua repo. I repeat **DO NOT** use the stock Lua repo,
that's full of security holes.

If you have to update Lua, then keep THIS luaconf.h (otherwise make sure it is using snprintf and NOT sprintf, and that the stack
is reasonably limited), delete lua.c, liolib.c, loadlib.c, loslib.c, ltests.c; remove *all* file operations (like Lua_doFile and
friends) and *all* pipe and exec functionality: edit lauxlib.c and remove luaL_fileresult, luaL_execresult, LoadF, getF, errfile,
skipBOM, skipcomment, luaL_loadfilex functions; edit lbaselib.c and remove luaB_loadfile, luaB_doFile, edit linit.c and from the
table remove luaopen_package, luaopen_coroutine, luaopen_io, luaopen_os. Remove the missing .o files and all libs from MYLIBS in
the makefile, plus also remove any functions that may depend on functions in the deleted files or on the functions listed above.
See [hardened.diff] for reference.

Also remove static from str_format because we'll gonna need it.

Original README follows:

-----------------------------------------------------------------------


# Lua

This is the repository of Lua development code, as seen by the Lua team. It contains the full history of all commits but is mirrored irregularly. For complete information about Lua, visit [Lua.org](https://www.lua.org/).

Please **do not** send pull requests. To report issues, post a message to the [Lua mailing list](https://www.lua.org/lua-l.html).

Download official Lua releases from [Lua.org](https://www.lua.org/download.html).
