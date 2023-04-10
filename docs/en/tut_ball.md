Bouncing Ball
=============

In this tutorial we'll create a program that bounces a ball on the screen.

Displaying the Ball
-------------------

First things first, start `meg4` and select the [Sprite Editor] (press <kbd>F3</kbd>). Select the first sprite on the right,
and edit it on the left.

<imgc ../img/tut_ball1.png><fig>Drawing the ball</fig>

Now go to the [Code Editor] (press <kbd>F2</kbd>). At first, our program will be an empty skeleton.

```c
#!c

void setup()
{
  /* Things to do on startup */
}

void loop()
{
  /* Things to run for every frame, at 60 FPS */
}
```

Let's place this newly drawn sprite on screen! You can do this by calling the [spr] function. Go to the body of the `setup()`
function, start typing, and at the bottom in the statusbar you'll see the required parameters.

<imgc ../img/tut_ball2.png>

We can see that the first two arguments are `x, y`, the screen coordinate where we want to place the sprite (if it's not obvious
from the name what a parameter does, just press <kbd>F1</kbd> and a detailed help will show up. Pressing <kbd>Esc</kbd> there
will bring you back to the code editor). The screen is 320 pixels wide and 200 pixels tall, so in order to place at the centre,
let's use `160, 100`. Next argument is the `sprite`. We've drawn our ball at the 0th sprite, so use `0`. After comes `sw`
(sprite width) and `sh` (sprite height), which tells how many sprites we want to display. Our ball only occupies 1 sprite, so
simply write `1, 1`. Then comes `scale`, but we don't want to magnify our ball, so just use `1` here too. Finally, the last
parameter is `type`, which can be used to display the sprite transformed. We don't want any transformation, so just use `0`.

```c
#!c

void setup()
{
  /* Things to do on startup */
  <hl>spr(160, 100, 0, 1, 1, 1, 0);</hl>
}

void loop()
{
  /* Things to run for every frame, at 60 FPS */
}
```

Now try running this code by pressing <kbd>Ctrl</kbd>+<kbd>R</kbd>, and let's see what happens. If you have made some mistake
by typing the code, an error message will show up in the status bar, and the cursor will be positioned to the faulting part.

If everything went well, then the editor screen will disappear, and black screen with the ball in the middle will appear instead.
Our ball isn't exactly at the centre, because we forgot to subtract the half of the sprite's size from the coordinates (we are
displaying only one sprite here (sw = 1 and sh = 1), so 8 x 8 pixels in total, half of that is 4). Press <kbd>F2</kbd> to go back
to the editor, and let's fix this.

```c
#!c

void setup()
{
  /* Things to do on startup */
  spr(<hl>156, 96</hl>, 0, 1, 1, 1, 0);
}

void loop()
{
  /* Things to run for every frame, at 60 FPS */
}
```

Let's run this again! We have the ball at proper position, but we can still see it at the wrong position! That's because we
haven't cleared the screen. We can do that by calling [cls], so let's add that before drawing the sprite.

```c
#!c

void setup()
{
  /* Things to do on startup */
  <hl>cls(0);</hl>
  spr(156, 96, 0, 1, 1, 1, 0);
}

void loop()
{
  /* Things to run for every frame, at 60 FPS */
}
```

Now everything is fine, our ball shown at exactly the centre of the screen.

Moving the Ball
---------------

There's a problem in our code. We display the ball in `setup()` which only runs once when our program starts. To move the ball,
we have to display it over and over again, at different positions. To achieve this, let's move the code that displays our ball
into the `loop()` function. This runs every time when the screen is redrawn.

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
  spr(156, 96, 0, 1, 1, 1, 0);</hm>
}
```

If we press <kbd>Ctrl</kbd>+<kbd>R</kbd> now, then the ball will be displayed exactly as before. What we can't see is that
the ball is now drawn not only once, but over and over again.

Let's move that ball! It is displayed at the same position, because we have used constant coordinates. Let's fix this by
introducing two variables, which will store the ball's current position on screen.

```c
#!c

<hl>int x, y;</hl>

void setup()
{
  /* Things to do on startup */
}

void loop()
{
  /* Things to run for every frame, at 60 FPS */
  cls(0);
  spr(156, 96, 0, 1, 1, 1, 0);
}
```

Replace the coordinates in drawing call to these variables, and let's assign them values on program start.

```c
#!c

int x, y;

void setup()
{
  /* Things to do on startup */
<hm>  x = 156;
  y = 96;</hm>
}

void loop()
{
  /* Things to run for every frame, at 60 FPS */
  cls(0);
  spr(<hl>x, y</hl>, 0, 1, 1, 1, 0);
}
```

If we run our program now, there'll be still no change. However using variables we can now change the ball's position in
run-time, let's do that.

```c
#!c

int x, y;

void setup()
{
  /* Things to do on startup */
  x = 156;
  y = 96;
}

void loop()
{
  /* Things to run for every frame, at 60 FPS */
  cls(0);
  spr(x, y, 0, 1, 1, 1, 0);
<hm>  x = x + 1;
  y = y + 1;</hm>
}
```

Run this program, and you'll see the ball moving!

Bouncing the Ball
-----------------

We are not ready yet, because our ball disappears pretty quickly from the screen. This is because we constantly increasing its
coordinates, and we don't change its direction when it reaches the edge of the screen.

Just like as we did with the coordinates at first, we are using a constant and now we want to change the direction in our program
dynamically. The solution is the same, we replace the constants with two new variables.

```c
#!c

int x, y<hl>, dx, dy</hl>;

void setup()
{
  /* Things to do on startup */
  x = 156;
  y = 96;
  <hl>dx = dy = 1;</hl>
}

void loop()
{
  /* Things to run for every frame, at 60 FPS */
  cls(0);
  spr(x, y, 0, 1, 1, 1, 0);
  x = x + <hl>dx</hl>;
  y = y + <hl>dy</hl>;
}
```

Great! As mentioned before, the screen is 320 pixels wide and 200 pixels tall. This means that the valid values for the `x`
coordinate are between 0 and 319, and for `y` between 0 and 199. But we don't want our ball to disappear from the screen, so
we have to subtract the sprite's size (8 x 8 pixels) from these. This gives 0 to 311 for `x`, and 0 to 191 for `y`. If our
ball's coordinate reaches one of these values, then we must change it's direction so that it won't leave the screen.

```c
#!c

int x, y, dx, dy;

void setup()
{
  /* Things to do on startup */
  x = 156;
  y = 96;
  dx = dy = 1;
}

void loop()
{
  /* Things to run for every frame, at 60 FPS */
  cls(0);
  spr(x, y, 0, 1, 1, 1, 0);
  x = x + dx;
  y = y + dy;
<hm>  if(x == 0 || x == 311) dx = -dx;
  if(y == 0 || y == 191) dy = -dy;</hm>
}
```

Run this program by pressing <kbd>Ctrl</kbd>+<kbd>R</kbd>. Congratulations, you have a moving ball that bounces off the screen
edges!

Adding a Bat
------------

A game that we can't play with isn't very interesing. So we'll add a bat that the player can control.

Go to the [Sprite Editor] (press <kbd>F3</kbd>) and let's draw the bat. This time we'll make it three sprites wide. You can draw
these one by one, or you can select multiple sprites on the right and edit them together on the left.

<imgc ../img/tut_ball3.png><fig>Drawing the bat</fig>

Just like with the ball, we'll use [spr] to display it on screen. But we also know that we'll need a variable to store its position,
so let's add that too at once.

```c
#!c

int x, y, dx, dy<hl>, px</hl>;

void setup()
{
  /* Things to do on startup */
  x = 156;
  y = 96;
  dx = dy = 1;
}

void loop()
{
  /* Things to run for every frame, at 60 FPS */
  cls(0);
  spr(x, y, 0, 1, 1, 1, 0);
  <hl>spr(px, 191, 1, 3, 1, 1, 0);</hl>
  x = x + dx;
  y = y + dy;
  if(x == 0 || x == 311) dx = -dx;
  if(y == 0 || y == 191) dy = -dy;
}
```

Because we draw the bat at the 1st sprite, the `sprite` parameter is `1`. And because it is three sprites wide, `sw` is `3`. We
don't want to move the bat vertically, just horizontally, so the `y` argument is a constant.

So far so good, but how will the player move this bat with the mouse? We'll set the `x` parameter to the mouse coordinate for that.
If you go the memory map, then under [pointer] you can see that the mouse's X coordinate is stored on 2 bytes at address `00016`.
To get this value, we use the [inw] function (word, because we need 2 bytes). But we also must not allow moving the bat off screen,
so we only do this if the coordinate is less than the screen size minus the bat's size (which is three sprites, so 24 pixels). One
more thing, the offsets in the memory map are given in hexadecimal, so we need the `0x` prefix to tell the compiler that this is a
hexadecimal number.

```c
#!c

int x, y, dx, dy, px;

void setup()
{
  /* Things to do on startup */
  x = 156;
  y = 96;
  dx = dy = 1;
}

void loop()
{
  /* Things to run for every frame, at 60 FPS */
  cls(0);
  spr(x, y, 0, 1, 1, 1, 0);
<hm>  px = inw(0x16);
  if(px > 296) px = 296;</hm>
  spr(px, 191, 1, 3, 1, 1, 0);
  x = x + dx;
  y = y + dy;
  if(x == 0 || x == 311) dx = -dx;
  if(y == 0 || y == 191) dy = -dy;
}
```

Let's run this program! We can see the bat and we can also move it around with the mouse. But there's a problem. The ball doesn't
care where the bat is. Let's fix this by modifying the screen bottom check to a bat position check so that it would bounce on the
bat only. Don't forget that the ball is 8 pixels tall, so we must check at a smaller coordinate.

```c
#!c

int x, y, dx, dy, px;

void setup()
{
  /* Things to do on startup */
  x = 156;
  y = 96;
  dx = dy = 1;
}

void loop()
{
  /* Things to run for every frame, at 60 FPS */
  cls(0);
  spr(x, y, 0, 1, 1, 1, 0);
  px = inw(0x16);
  if(px > 296) px = 296;
  spr(px, 191, 1, 3, 1, 1, 0);
  x = x + dx;
  y = y + dy;
  if(x == 0 || x == 311) dx = -dx;
  if(y == 0 || <hl>(y == 183 && x >= px && x <= px + 24)</hl>) dy = -dy;
}
```

This means the `y` coordinate is zero, or it is `183` and at the same time the `x` coordinate is between `px` and `px + 24` (where
`px` is the bat's position).

Game Over
---------

Now that we have changed the bottom check, we have to add a new check to see if the ball has left the screen. Of course this would
mean game over.

There are three things that we want to do if this happens. First, remember that `loop()` runs constantly, so to stop moving the
ball any further, we set the `dx` and `dy` variables to zero. Second, we want to display a game over message. And last, if the
player clicks, we want to restart the game.

```c
#!c

int x, y, dx, dy, px;
<hm>str_t msg = "GAME OVER!";</hm>

void setup()
{
  /* Things to do on startup */
  x = 156;
  y = 96;
  dx = dy = 1;
}

void loop()
{
  /* Things to run for every frame, at 60 FPS */
  cls(0);
  spr(x, y, 0, 1, 1, 1, 0);
  px = inw(0x16);
  if(px > 296) px = 296;
  spr(px, 191, 1, 3, 1, 1, 0);
  x = x + dx;
  y = y + dy;
  if(x == 0 || x == 311) dx = -dx;
  if(y == 0 || (y == 183 && x >= px && x <= px + 24)) dy = -dy;
<hm>  if(y > 199) {
    dx = dy = 0;
    text(184, (320 - width(2, msg)) / 2, 90, 2, 112, 128, msg);
    if(getclk(BTN_L)) setup();
  }</hm>
}
```

We can display text on screen using the [text] function. At the bottom, the quick help shows what arguments it has. The first one
is `palidx`, which is a palette index, the color of the text. Press <kbd>F3</kbd> and click on the desired color. At the bottom,
we'll see the index in hexadecimal and in parenthesis in decimal as well.

<imgc ../img/tut_ball4.png>

I have choosen a red color, by the index `B8` or `184` in decimal. Let's go back to the code editor by pressing <kbd>F2</kbd>, and
enter this number. The next two arguments are `x` and `y`, the position on screen. We could have count the number of pixels in the
text, but we are lazy, so we use the [width] function for that. Subtracting this from the width of the screen and dividing by two
will place the text exactly at the middle. Because we are too lazy to type the text twice, we've also used a string variable `msg`
to store the message. Therefore we use `msg` once when we calculate its size, and also when we pass it to the display function to
say what to print. After the coordinates comes the `type`, which is the font's type, or more specifically its size. We want big
letters, so we've used `2`, double size. After this comes the shadow, `shidx`, which is also a color index. I've choosen a darker
red here. For a shadow, it is important how transparent it is, we can specify this in the `sha` argument. That is an alpha
channel value from 0 (fully transparent) to 255 (fully opaque). By using `128`, which is half way between these values, I've told
to use half transparent. And finally in the `str` argument we specify which text to display.

To query if the user has clicked, we use the [getclk] function, with a `BTN_L` argument, meaning is the left button clicked. We've
used the `setup()` function to set the default values for our little bouncing ball game. This is very convenient, because calling
`setup()` now will therefore simply reset our game.
