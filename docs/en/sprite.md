Sprite Editor
=============

<imgt ../img/sprite.png> Click on the stamp icon (or press <kbd>F3</kbd>) to modify the sprites.

The sprites you create here can be displayed using [spr], and also used by [stext] when it displays text on screen.

The editor has three main boxes, two on the top, and one below.

<imgc ../img/spritescr.png><fig>Sprite Editor</fig>

<h2 spr_edit>Sprite Editor Box</h2>

The one on the left is the sprite editor box. This is where you can draw to modify the sprite. <mbl> places the selected
pixel on the sprite, <mbr> clears it to empty.

<h2 spr_sprs>Sprite Selector</h2>

On the right you can see the sprite selector. The sprite you select here will be editable on the left. You can select
multiple adjacent sprites and edit them together.

<h2 spr_pal>Palette</h2>

Below you can see the palette. The first item cannot be set, because that's for erase. If you select any other color, then
the <imgt ../img/hsv.png> palette button will become active. Clicking on it will bring up the HSV color picker, and with that
you can modify the color for that palette entry.

The default MEG-4 palette uses 32 colors of the DawnBringer32 palette, 8 grayscale gradients, and 6 x 6 x 6 RGB combinations.

<h2 spr_tools>Toolbar</h2>

<imgc ../img/toolbox.png><fig>Sprite Toolbar</fig>

Under the sprite editor, you see the tool buttons. With these you can easily modify the sprite. Shifting in different
directions, rotating clock-wise, flipping etc. If there's an active selection, then they operate on the selection only,
otherwise on the entire sprite. For the rotation, you must select an area which has the same width as height, otherwise
rotation would be impossible.

The flood fill tool only fills adjacent same pixels, unless there's a selection. With a selection the entire selected area
is filled, no matter what pixels the selection contains.

<h2 spr_sel>Selections</h2>

There are two kinds of selections: rectangle selection and fuzzy select. The former selects a rectangular area, the other
selects continous regions with the same pixel. You can press and hold <kbd>Shift</kbd> to expand the selection, and
<kbd>Ctrl</kbd> to intersect and make it smaller.

Pressing <kbd>Ctrl</kbd>+<kbd>A</kbd> will select all, and <kbd>Ctrl</kbd>+<kbd>I</kbd> will invert the current selection.

When a selection is active, you can press <kbd>Ctrl</kbd>+<kbd>C</kbd> to copy the area to the clipboard. Later, you can press
<kbd>Ctrl</kbd>+<kbd>V</kbd> to paste it. Pasting works exactly like the pencil tool, except now you can paint with the whole
copied area like a brush. It worth noting that empty pixels are copied to. If you don't want your brush to clear the area, then
only select non-empty pixels (you can use <kbd>Ctrl</kbd>+<imgt ../img/fuzzy.png> to deselect the empty areas) before copy, that
way the empty pixels won't be copied to the clipboard, and in return won't be used as part of the brush.
