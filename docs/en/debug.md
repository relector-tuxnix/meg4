Debugger
========

<imgt ../img/debug.png> Click on the ladybug icon (or press <kbd>F10</kbd>) to examine your program's machine code.

Code has three sub-pages, one where you can see your program's machine code (this one), the [Code Editor] where you can write
the source code as text, and the [Visual Editor], where you can do the same using structograms.

WARN: The debugger only works with the built-in languages. It is not available with third party languages like Lua for example,
those are not, and cannot be supported.

Here you can see how the CPU sees your program. By pressing <kbd>Space</kbd> you can do a step by step execution and see
the registers and the memory change. Clicking on the <ui1>Code</ui1> / <ui1>Data</ui1> button in the menu (or pressing the
<kbd>Tab</kbd> key) will switch between code and data views.

<imgc ../img/debugscr.png><fig>Debugger</fig>

Code View
---------

On the left you can see the callstack. This is used to backtrace function calls. It also displays the corresponding source line
where the function was called. This is a link, clicking on it will bring up the [Code Editor], positioned at the line in question.
The top of the list is always the line which is currently being executed.

On the right is the list of the bytecode instructions in [Assembly] that the CPU actually executes.

Data View
---------

On the left is the list of your global variables with their actual values.

On the right you can see the stack, which is splitted into separate parts. Everything above the BP register is the argument
list to the currently running function, and everything below that but above the SP register is the area for the local variables.

Registers
---------

Independently to which view is active, you can always see the CPU registers at the bottom. With third party languages, only the
FLG, TMR and PC registers are available. See [mnemonics] for more details on each register.
