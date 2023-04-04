Map Editor
==========

<imgt ../img/map.png> Clicking on the jigsaw icon (or pressing <kbd>F4</kbd>) will bring up the map editor. Here you can place
sprites as tiles on the map.

The map you've created here (or any parts of it) can be put on screen using the [map] or with the [maze] commands (see below).

<imgc ../img/mapscr.png><fig>Map Editor</fig>

The map is special in a way that it can only display 256 different tiles at once out of the 1024 sprites. For each sprite bank,
the first sprite is always reserved for the empty tile, so sprites 0, 256, 512, and 768 cannot be used on maps.

<h2 map_box>Map Editor Box</h2>

On top the big area is where you can see and edit the maps. It is shown as one big map, 320 columns wide and 200 rows high.
You can use the zoom in and zoom out buttons on the toolbar, or the <mbw> mouse wheel to zoom. By pressing <mbr> right click
and holding it down, you can drag the map, but you can also use the scrollbars on the right and on the bottom.

Clicking with <mbl> left button will set the selected sprite on the map. Select the first sprite to clear the map (<mbr> right
click does not clear here, instead it moves the map).

<h2 map_tools>Toolbar</h2>

Below the map editor area is the toolbar, same as on the [sprite editor] page, with exactly the same functionality and same
keyboard shortcuts (but it can use sprite patterns too, see below). Next to the tool buttons you can find the zoom buttons and
the map selector. This latter selects which sprite bank is used on the map (just for the editor. When your game runs, you'll
have to set the byte at offset 0007F to change the map's sprite bank, see [Graphics Processing Unit]).

<h2 map_sprs>Sprite Palette</h2>

On the right to the buttons is the sprite selector, where you can select the sprite you want to draw with. As said earlier, the
first sprite in every 256 sprites bank is unusable, reserved to the empty map tile.

The difference to the sprite editor is (where you can choose just one color from the palette), here on the map you can select
multiple adjacent sprites at once. With paint, all of them will be painted at once (exactly the same way as if they were pasted
from the clipboard), and what's more, flood fill will use them as a brush too, filling the selected area with that multi-sprite
pattern.

Even more, clicking <kbd>Shift</kbd>+<mbl> with the <imgt ../img/fill.png> fill tool, it will choose one sprite from the pattern
randomly. For example, let's assume you have 4 sprites with different looking trees. If you select all of them and fill an area
on the map, then those sprites will be placed always in the same order, repeating one after another, which isn't looking good for
a forest. But if you press and hold down <kbd>Shift</kbd> during clicking with the fill tool, then each tile will be choosen
randomly from those selected 4 tree sprites, which looks much more a real forest.

3D Maze
-------

You can also display the map as a 3D maze with the [maze] function. For this, the turtle position and direction is used as the
player's view point to the maze, but to accomodate sub-tile positions, here the turtle's coordinate is multiplied by 128
(originally I've used 8 to match pixels on the map, but movement was too blocky that way). So for example (64,64) is the centre
of the top left field on the map, and (320,192) is the centre of the third coloumn and second row.

Here `scale` parameter acts differently too: when set to 0, then the maze will use 32 x 8 tiles as seen on the sprite palette,
one for each tile, each 8 x 8 pixels in size. When set to 1, then there will be 16 x 16 tiles, each tile will use 2 x 2 sprites,
so 16 x 16 pixels in size. With 3, you'll have 4 x 4 kinds of tiles, so 16 different tiles in total, each 64 x 64 pixels. In this
case the map will select these larger tiles, so tile id only equals with the sprite id if the scale is 0. For example if map has
the id of 1 with scale set to 1, then instead of sprite id 1, this will select sprites 2, 3, 34, 35.

```
Tile id 1 with scale 0          Tile id 1 with scale 1
+---+===+---+---+-              +---+---+===+===+-
|  0|::1|  2|  3| ...           |  0|  1|::2|::3| ...
+---+===+---+---+-              +---+---+===+===+-
| 32| 33| 34| 35| ...           | 32| 33|:34|:35| ...
+---+---+---+---+-              +---+---+===+===+-
```

Regardless you'll have to place sprite id 1 on the map from the palette to get these sprites. Tile id 0 as usual means empty.

If `sky` is set, then that tile will be displayed as a ceiling to the maze. On the other hand, `grd` is only displayed as floor
where the map is empty. When you call the maze, you separate the tile ids into ranges, and this specifies how a tile is displayed
(floor, wall or sprite). Everything greater than or equal to `wall` will be non-walkable and displayed as a cube, with the
selected tile sprites on the cube's sides without transparency. Tiles greater than or equal to `obj` will be non-walkable too,
but displayed as a properly scaled 2D sprite always facing towards the player (the turtle's position) and with alpha channel
applied, so unlike the walls the objects can be transparent.

| Tile id              | Description                                          |
|----------------------|------------------------------------------------------|
| 0                    | Always walkable, `grd` displayed as floor instead    |
| 1 <= x < `door`      | Walkable, displayed as floor                         |
| `door` <= x < `wall` | Displayed as a wall, but walkable                    |
| `wall` <= x < `obj`  | Non-walkable, displayed as a wall                    |
| `obj` <= x           | Non-walkable, displayed as an object sprite          |

You can also add non-player characters (or other objects) to the maze independently to the map (in an int array with x, y, tile id
triplets, where the coordinates are multiplied by 128). These will be walkable and will be displayed the same as object sprites;
collision detection, movement and all the other aspects have to be implemented in your game by you. The `maze` command just diplays
these. It does a favour for you though, if the given NPC can directly see the player, then the tile id's most significant 4 bits
in the array will be set. Which bits depends on their distance to each other: the most significant bit (0x80000000) is set if their
distance is smaller than 8 map fields, next bit (0x40000000) if less than 4 fields, next bit (0x20000000) if less than 2 map fields,
and finally last bit (0x10000000) if they are on the same field or neightbouring map fields.

Furthermore this command also takes care of navigation in the maze, <kbd>▴</kbd> / △ moves the turtle forward, <kbd>▾</kbd> / ▽
backward; <kbd>◂</kbd> / ◁ turns left, and <kbd>▸</kbd> / ▷ turns right (keyboard mappings for the gamepad can be changed, see
[memory map] for details). Handling all the other gamepad buttons and interactions are up to you to code in your game, the `maze`
only helps you with moving the player and handling collisions with the walls.

NOTE: Don't forget that you always have to divide the turtle's position by 128 to get the player's position on the map.
