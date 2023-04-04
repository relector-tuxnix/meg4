MEG-4 Script Runner
===================

Smallest possible MEG-4 "platform", just to be able to batch test running scripts. Mostly used for testing. Not really usable
for any other purpose.

Usage
-----

```
./runner [-d|-v|-r] <script>
````

This will try to import `script` (must start with a `#!c`, `#!bas`, `#!asm` or `#!lua` line), compiles it and then runs it a
couple of times.

If `-d` given, then it does not execute the script, instead it disassembles bytecode and displays it to stdout.

If `-v` given, then it shows the instructions as they are executed.

With `-r` it recompiles the source and re-runs it (testing that Lua hasn't lost MEG-4 API context).
