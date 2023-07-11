Interface
=========

Game Screen
-----------

By default, you'll see the game screen, with your game running (or the MEG-4 Floppy Drive if there's no game loaded). When you
press <kbd>Esc</kbd> on this screen, then you'll switch to editor mode.

Editor Screens
--------------

If you press <kbd>Esc</kbd> (no re-compilation) or <kbd>Ctrl</kbd>+<kbd>R</kbd> (re-compiles your program) in any of the editor
modes then you'll return to the game screen.

All editors are themable, to recolor the entire interface, just drag'n'drop a GIMP Palette file into the screen. See
[other formats] for details.

<imgc ../img/help.png><fig>Help and the Menu</fig>

All editors have a menu on the top. By clicking on the <imgt ../img/menu.png> icon, a pop up menu will appear, from where you can
access various functions, most of which also accessible through keyboard shortcuts (see [keyboard](#ui_kbd)). You can also access
the built-in help pages from here, however that's always accessible in all editors by pressing <kbd>F1</kbd> too.

Help Pages
----------

On the help pages screen, you can click on the links, and you can also press <kbd>Backspace</kbd> to return to the previous
help page in history. If the history is empty, then this will return to the Table of Contents page. The help screen is the
exception to the rule, because pressing <kbd>Esc</kbd> here does not always return to the game screen, instead it returns to the
screen where it was invoked from.

To start a search, you can click on the search input box on the top right, or just start typing what you're looking for.

The built-in help pages reader is actually a very minimal MarkDown viewer, and it shows exactly the same info that were used
to generate this manual (but this manual you're reading now has more sections, the built-in one is limited to the API Reference).
