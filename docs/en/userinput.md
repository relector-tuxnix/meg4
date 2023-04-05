User Input
==========

<h2 ui_gp>Gamepad</h2>

The first gamepad's buttons and joysticks are mapped to the keyboard, they are working simultaniously. For example it doesn't
matter if you press Ⓧ on the controller, or <kbd>X</kbd> on the keyboard, in both case both gamepad button flag and keyboard
state flag will be set. The mapping can be changed by writing the keyboard scancodes to MEG-4's memory, see [memory map] for
details. The default mapping goes like cursor arrows are the directions ◁, △, ▽, ▷; the <kbd>Space</kbd> is the primary
button Ⓐ, <kbd>C</kbd> is the secondary button Ⓑ and <kbd>X</kbd> is Ⓧ, <kbd>Z</kbd> is Ⓨ. The Konami Code is working too
(see `KEY_CHEAT` scancode).

<h2 ui_ptr>Pointer</h2>

Coordinates and button pressed states can be easily accessed from the MEG-4's memory. Scrolling (both vertical and if supported,
horizontal) handled as if your mouse had up / down or left / right buttons.

<h2 ui_kbd>Keyboard</h2>

For convenience, it has several shortcuts and multiple input methods.

| Key Combination              | Description                                                                                  |
|------------------------------|----------------------------------------------------------------------------------------------|
| <kbd>GUI</kbd>               | Or Super, sometimes has a <imgt ../img/gui.png> logo on it. UNICODE codepoint input mode.    |
| <kbd>AltGr</kbd>             | The Alt key right to the space key, activates Compose mode.                                  |
| <kbd>Alt</kbd>+<kbd>U</kbd>  | In case your keyboard lacks the <kbd>GUI</kbd> key, UNICODE input mode too.                  |
| <kbd>Alt</kbd>+<kbd>Space</kbd> | Fallback Compose, for keyboards without the <kbd>AltGr</kbd> key.                         |
| <kbd>Alt</kbd>+<kbd>I</kbd>  | Enter icon (emoticons) input mode.                                                           |
| <kbd>Alt</kbd>+<kbd>K</kbd>  | Enter Katakana input mode.                                                                   |
| <kbd>Alt</kbd>+<kbd>J</kbd>  | Enter Hiragana input mode.                                                                   |
| <kbd>Alt</kbd>+<kbd>A</kbd>  | For keyboards without such key, works as a `&` key (ampersand).                              |
| <kbd>Alt</kbd>+<kbd>H</kbd>  | For keyboards without such key, works as a `#` key (hashmark).                               |
| <kbd>Alt</kbd>+<kbd>S</kbd>  | For keyboards without such key, works as a `$` key (dollar).                                 |
| <kbd>Alt</kbd>+<kbd>L</kbd>  | For keyboards without such key, works as a `£` key (pound).                                  |
| <kbd>Alt</kbd>+<kbd>E</kbd>  | For keyboards without such key, works as a `€` key (euro).                                   |
| <kbd>Alt</kbd>+<kbd>R</kbd>  | For keyboards without such key, works as a `₹` key (rupee).                                  |
| <kbd>Alt</kbd>+<kbd>Y</kbd>  | For keyboards without such key, works as a `¥` key (yen).                                    |
| <kbd>Alt</kbd>+<kbd>N</kbd>  | For keyboards without such key, works as a `元` key (yuan).                                   |
| <kbd>Alt</kbd>+<kbd>B</kbd>  | For keyboards without such key, works as a `₿` key (bitcoin).                                |
| <kbd>Ctrl</kbd>+<kbd>S</kbd> | Save floppy.                                                                                 |
| <kbd>Ctrl</kbd>+<kbd>L</kbd> | Load floppy.                                                                                 |
| <kbd>Ctrl</kbd>+<kbd>R</kbd> | Run your program.                                                                            |
| <kbd>Ctrl</kbd>+<kbd>⏎Enter</kbd> | Toggle fullscreen mode.                                                                 |
| <kbd>Ctrl</kbd>+<kbd>A</kbd> | Select all.                                                                                  |
| <kbd>Ctrl</kbd>+<kbd>I</kbd> | Invert selection.                                                                            |
| <kbd>Ctrl</kbd>+<kbd>X</kbd> | Cut, copy to clipboard and delete.                                                           |
| <kbd>Ctrl</kbd>+<kbd>C</kbd> | Copy to clipboard.                                                                           |
| <kbd>Ctrl</kbd>+<kbd>V</kbd> | Paste from clipboard.                                                                        |
| <kbd>Ctrl</kbd>+<kbd>Z</kbd> | Undo.                                                                                        |
| <kbd>Ctrl</kbd>+<kbd>Y</kbd> | Redo.                                                                                        |
| <kbd>F1</kbd>                | Built-in help pages (the API Reference in this manual, see [interface]).                     |
| <kbd>F2</kbd>                | [Code Editor]                                                                                |
| <kbd>F3</kbd>                | [Sprite Editor]                                                                              |
| <kbd>F4</kbd>                | [Map Editor]                                                                                 |
| <kbd>F5</kbd>                | [Font Editor]                                                                                |
| <kbd>F6</kbd>                | [Sound Effects]                                                                              |
| <kbd>F7</kbd>                | [Music Tracks]                                                                               |
| <kbd>F8</kbd>                | [Memory Overlays] Editor                                                                     |
| <kbd>F9</kbd>                | [Visual Editor]                                                                              |
| <kbd>F10</kbd>               | [Debugger]                                                                                   |
| <kbd>F11</kbd>               | Toggle fullscreen mode.                                                                      |
| <kbd>F12</kbd>               | Save screen as `meg4_scr_(unix timestamp).png`.                                              |

### UNICODE Codepoint Mode

In this mode you can enter hex numbers (`0` to `9` and `A` to `F`). Instead of these separately pressed keys, the codepoint they
describe will be added as if your keyboard had that key. For example the sequence <kbd>GUI</kbd>, <kbd>2</kbd>, <kbd>e</kbd>,
<kbd>⏎Enter</kbd> will add a `.` dot, because codepoint `U+0002E` is the `.` dot character.

NOTE: Only the Basic Multilingual Plane (`U+00000` to `U+0FFFF`) supported, with some exceptions for the emoticons range starting
at `U+1F600`. Other codepoints will be simply skipped.

This mode automatically quits after the characterer is entered.

### Compose Mode

In compose mode you can add acute, umlaut, tilde etc. to the characters. For example the sequence <kbd>AltGr</kbd>, <kbd>a</kbd>,
<kbd>'</kbd> will add `á`, or <kbd>AltGr</kbd>, <kbd>s</kbd>, <kbd>s</kbd> will add `ß`, and another example, <kbd>AltGr</kbd>,
<kbd>c</kbd>, <kbd>,</kbd> will add `ç`, etc. You can use <kbd>Shift</kbd> in combination with the letter to get the uppercase
variants.

This mode automatically quits after the characterer is entered.

### Icon Mode

In icon mode you can add special icon characters, representing the emulator's input (like the sequence <kbd>Alt</kbd>+<kbd>I</kbd>,
<kbd>m</kbd> will add the `🖱` mouse icon, and <kbd>Alt</kbd>+<kbd>I</kbd>, <kbd>a</kbd> will add the icon of the gamepad's
`Ⓐ` button) as well as emoji icons (like <kbd>Alt</kbd>+<kbd>I</kbd>, <kbd>;</kbd>, <kbd>)</kbd> will add the character `😉`,
or <kbd>Alt</kbd>+<kbd>I</kbd>, <kbd><</kbd>, <kbd>3</kbd> will produce `❤`).

This mode remains active after the characterer is entered, press <kbd>Esc</kbd> to return to normal input mode.

### Katakana and Hiragana Modes

Similar to icon mode, but here you can type the Romaji letters of pronounced sound to get the character. For example the sequence
<kbd>Alt</kbd>+<kbd>K</kbd>, <kbd>n</kbd>, <kbd>a</kbd>, <kbd>n</kbd>, <kbd>i</kbd>, <kbd>k</kbd>, <kbd>a</kbd> is interpreted
as three sounds, and therefore will add the three characters `ナニヵ`. Also, punctation works as expected, for example
<kbd>Alt</kbd>+<kbd>K</kbd>, <kbd>.</kbd> will produce Japanese full stop character `。`.

You can use <kbd>Shift</kbd> in combination with the first letter to get the uppercase variants, for example
<kbd>Alt</kbd>+<kbd>K</kbd>, <kbd>Shift</kbd>+<kbd>s</kbd>, <kbd>u</kbd> will produce `ス` and not `ㇲ`.

This mode remains active after the characterer is entered, press <kbd>Esc</kbd> to return to normal input mode.

NOTE: This feature is implemented using data tables, new combinations and even new writing systems can be added to `src/inp.c`
any time without coding skills.
