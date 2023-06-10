Walking
=======

In this tutorial we'll create a walking character using sprites. This is the basis of many adventure and rouge-like games.

Get the Spritesheet
-------------------

We could draw the sprites ourselves, but for simplicity I've downloaded a public domain sheet from the internet. This contains
three animation phases in every line, and has one line per all 4 directions. There are lots of spritesheets on the net like this,
because this is the popular RPG Maker's sprite layout.

WARN: Always check the licensing terms when you use assets downloaded from the internet. Do not use the asset if you're
unsure about its terms of use.

The downloaded image can't be imported as-is. You'll have to change the image's dimensions to 256 x 256 pixels. Do not use resize,
rather in GIMP select `Image` > `Canvas size...`, and in the popup window set width and height to 256. This way the spritesheet's
size will be kept intact, and will be expanded with transparent pixels. Start `meg4` and drag'n'drop this 256 x 256 image on the
window to import.

<imgc ../img/tut_walk1.png><fig>The imported sprite sheet</fig>

As you can see, one character sprite is made up of 4 x 4 sprites. Let's display that! Press <kbd>F2</kbd> to go to the code editor.

Display character
-----------------

We start with the usual skeleton. We know from the previous tutorial that we'll have to clear the screen and display the sprites
using [spr] in the `loop()` function, because animation requires constant redrawing.

```c
#!c

void setup()
{
  /* Things to do on startup */
}

void loop()
{
  /* Things to run for every frame, at 60 FPS */
<hm>  cls(0);
  spr(144, 84, 0, 4, 4, 0, 0);</hm>
}
```

The centre of the screen is at 160, 100 but our character is 4 x 4 sprites in size (32 x 32 pixels), so we have to subtract the
half of that. Then comes 0 for `sprite`, meaning the first sprite, followed by 4, 4 because we want to display that many sprites.
The last two parameters are 0, 0 because we don't want to scale nor transform. We want the sprites to be display exactly as they
appear in the sprite editor.

Changing directions
-------------------

Next, let's allow changing the direction in which the character is pointing to. For that, we'll use [getpad], which returns the
gamepad's state. The primary gamepad controller is mapped to the keyboard, so pressing the cursor arrow keys will work too. To
handle the direction, we'll need a variable to store the current direction, and this should select the sprite we draw.

```c
#!c

<hl>int dir;</hl>

void setup()
{
  /* Things to do on startup */
}

void loop()
{
  /* Get user input */
<hm>  if(getpad(0, BTN_D)) {
    dir = 0;
  } else
  if(getpad(0, BTN_L)) {
    dir = 128;
  } else
  if(getpad(0, BTN_R)) {
    dir = 256;
  } else
  if(getpad(0, BTN_U)) {
    dir = 384;
  }</hm>
  /* Display the character */
  cls(0);
  spr(144, 84, <hl>dir</hl>, 4, 4, 0, 0);
}
```

You can press <kbd>F3</kbd> and click on the top left sprite of the character sprite on the selector to get the sprite id for that
direction. We set these in the `dir` variable, and we'll use this variable in place of the `sprite` parameter.

Try it out! You'll see that by pressing the arrows our character will change directions.

Adding animation
----------------

Our character doesn't walk yet. Let's fix it! We want our character to walk when a button (or arrow key) is pressed, and stop
when that's released. For that, we'll need a variable to keep track if the button is currently pressed. We'll also need a variable
to tell which animation frame to display. We could have used some funky expression to get the sprite id for the frame, but it
is a lot easier to use an array instead. Because our character sprite is 4 x 4 sprites big, we can also precalculate these sprite
id offsets in this array. One more thing, we have three animation sprites, but we'll have to display the sprite in the middle twice
to get a proper back and forth aimation for moving the legs.

```c
#!c

int dir<hl>, pressed, frame;</hl>
<hl>int anim[4] = { 4, 8, 4, 0 };</hl>

void setup()
{
  /* Things to do on startup */
}

void loop()
{
  /* Get user input */
  <hl>pressed = 0;</hl>
  if(getpad(0, BTN_D)) {
    dir = 0; <hl>pressed = 1;</hl>
  } else
  if(getpad(0, BTN_L)) {
    dir = 128; <hl>pressed = 1;</hl>
  } else
  if(getpad(0, BTN_R)) {
    dir = 256; <hl>pressed = 1;</hl>
  } else
  if(getpad(0, BTN_U)) {
    dir = 384; <hl>pressed = 1;</hl>
  }
  /* Display the character */
  <hl>frame = pressed ? (frame + 1) & 3 : 0;</hl>
  cls(0);
  spr(144, 84, dir <hl>+ anim[frame]</hl>, 4, 4, 0, 0);
}
```

First, we clear the `pressed` variable. Then in the if blocks we set it to 1. This way when we press a button, the variable
becomes 1, but as soon as we release the button, it will be cleared to 0.

Next, we calculate which animation frame to display, but only if a button is pressed. If not, then we use a constant 0, meaning
the first frame. Otherwise we increase `frame` to get the next frame, and we use bitwise AND to avoid overflow. When `frame`
becomes 4 (which is 0b100 in binary) and we AND that with 3 (0b011), then the result will be 0, so the frame counter wraps around.
We could have used "modulo number of frames" as well, but this is faster. Finally, we get the sprite id offset for this animation
frame (stored in the `anim` array) and we add that to the `dir` variable to get which sprite to display.

Try it out, press <kbd>Ctrl</kbd>+<kbd>R</kbd>! It works fine, except our character is animated way too fast. That's because we
increase the `frame` counter on every refresh, so 60 times per second. To fix this, we should get the ticks from the MMIO and
calculate the frame independently to the refresh rate. However ticks counter is in millisec, so we should divide it. If we would
divide that by 100, then we'd get 10 frames per second. We'll use shifting to the right 7 bits instead, which is equivalent of
diving by 128. So first, press <kbd>F1</kbd>, and click on [Misc]. We can see that the ticks counter is at address 0x4, and it
is 4 bytes long (so we'll have to use [ini]). Go back to the code and replace the frame calculation with this.

```c
#!c

int dir, pressed, frame;
int anim[4] = { 4, 8, 4, 0 };

void setup()
{
  /* Things to do on startup */
}

void loop()
{
  /* Get user input */
  pressed = 0;
  if(getpad(0, BTN_D)) {
    dir = 0; pressed = 1;
  } else
  if(getpad(0, BTN_L)) {
    dir = 128; pressed = 1;
  } else
  if(getpad(0, BTN_R)) {
    dir = 256; pressed = 1;
  } else
  if(getpad(0, BTN_U)) {
    dir = 384; pressed = 1;
  }
  /* Display the character */
  frame = pressed ? (<hl>ini(0x4) >> 7</hl>) & 3 : 0;
  cls(0);
  spr(144, 84, dir + anim[frame], 4, 4, 0, 0);
}
```

And we're done! We have a nicely walking character animation that we can control in our game. You could also move the character
on screen by using variables for the x, y arguments, but it is very common in such games to move the map under the character in
the opposite direction instead.
