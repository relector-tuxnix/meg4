BASIC
=====

If you want to use this language, then start your program with a `#!bas` line.

<h2 ex_bas>Example Program</h2>

```bas
#!bas

REM global variables
LET acounter% = 123
LET anumber = 3.1415
LET anaddress% = $0048C
LET astring$ = "something"
DIM anarray(10)

REM Things to do on startup
SUB setup
  REM local variables
  LET iamlocal = 123
END SUB

REM Things to run for every frame, at 60 FPS
SUB loop
  REM BASIC style print
  PRINT "I"; " am"; " running"
  REM Get MEG-4 style outputs
  GOSUB printf("a counter %d, left shift %d\n", acounter%, getkey%(KEY_LSHIFT))
END SUB
```

Dialect
-------

BASIC stands for Beginners' All-purpose Symbolic Instruction Code. It was created by John Kemeny in 1963 with the explicit goal
to teach students programming with it. The **MEG-4 BASIC** is a bit more modern than that, it supports all of ANSI X3.60-1978
(ECMA-55) and many featues from ANSI X3.113-1987 (ECMA-116) too, with minor deviations. It allows longer than 2 characters
identifiers, and its floating point as well as integer arithmetics are 32 bit. Most important differences: no interactive mode,
therefore you don't have to number each instruction any more (you can use labels instead), and all BASIC keywords are
case-insensitive as in the specification, but variable and function names are *case-sensitive*. The MEG-4 API function calls
must be in lower-case; as for the rest it is up to you, but those are case-sensitive too (for example `APPLE`, `Apple` and
`apple` are three distinct variables).

It has one non-standard keyword, the `DEBUG`, which you can place anywhere in your code and will invoke the built-in
[debugger]. After this, you can execute your code step by step, watching what it is doing.

Here goes a detailed description with examples, and all differences noted.

<h2 bas_lit>Literals</h2>

MEG-4 BASIC understands decimal based numbers (either integer or floating point with or without scientific notation). Hexadecimal
numbers must be prefixed by `$` (not in the specification, but this was usual in Commodore BASIC and pretty much in every other
dialects of the '80s) and strings must be enclosed in double quotes:

```bas
42
3.1415
.123e-10
$0048C
"Goodbye and thanks for all the fish!\n"
```

WARN: The specification expects 7-bit ASCII, but MEG-4 BASIC uses zero terminated UTF-8 encoding. It also accepts C-like escape
sequences (eg. `\"` is double quotes, `\t` is tab, `\n` is the newline character), and the string's maximum size is limited to
255 bytes (the specification requires 18 bytes).

<h2 bas_var>Variables</h2>

Variables aren't declared, instead the last letter in their name identifies their type. This can be `%` for integers, `$` for
strings, and not in the specification, but MEG-4 BASIC accepts `!` for bytes and `#` for double bytes (word). Anything else is
interpreted as a floating point variable.

```bas
LET A% = 42
LET B! = $FF
LET C# = $FFFF
LET D$ = "string"
LET E = 3.1415
```

The conversion between byte, integer and floating point is automatic and fully transparent. However trying to use a string literal
in a number variable or storing a number literal in a string variable would reasult in an error (you must explicitly use `STR$`
and `VAL`).

When you assign values to variables, the `LET` command can be omited.

Literals can also be added to your program using the `DATA` statement, and then can be assigned to variables with the `READ`
command. `READ` reads in as many data literals as many variables its argument has, and can be called repeatedly. To reset to the
first `DATA` statement where `READ` reads from, use `RESTORE`.

```bas
RESTORE
READ name$, income
DATA "Joe", 1234
DATA "John", 2345
```

There are a few special variables, provided by the system. `RND` returns a floating point random number between 0 and 1,
`INKEY$` returns the key the user has pressed or an empty string, `TIME` returns the number of ticks (1/1000th seconds) since
power on, and finally `NOW%` returns the number of elapsed seconds since Jan 1, 1970 midnight in Greenwich Mean Time.

<h2 bas_arr>Arrays</h2>

Multiple elements of the same type can be assigned to a single variable, which is called an array.

```bas
DIM A(10)
DIM B(10, 10)
DIM C(1 TO 6)
```

The BASIC specification expects two-dimensional arrays to be handled, but MEG-4 BASIC supports up to 4 dimensions. Array elements
can be bytes, integers, numbers or strings. Dynamically resizing arrays with `REDIM` is not possible, all are statically allocated
using `DIM`. When the size isn't given, one dimension and 10 elements are assumed.

WARN: Index starts at 1 (as in ANSI and not at 0 as in ECMA-55). The `OPTION BASE` statement isn't supported, but you can set the
first index of each array with the `TO` keyword.

<h2 bas_op>Operators</h2>

In descending order of precedence:

<p>**Arithmetic**
```
^
* / MOD
+ -
```
The first one is for exponentiation (eg. b squared is `b^2`), multiplication, division and `MOD` is modulus (aka. remainder after
division, eg. `10 MOD 3` is 1), addition and subtraction.</p>

<p>**Relational**
```
<> =
<= >= < >
```
Not equal, equal, less or equal, greater or equal, less than, greater than.</p>

<p>**Logical**
```
NOT
AND
OR
```
Logical negation (0 becomes 1, everything non-zero becomes 0), and (returns 1 if both arguments are non-zero), or (returns
1 if at least one of the arguments are non-zero).</p>

There's one non-standard operator, the `@` returns the address of a variable. This is usable when the MEG-4 API expects an
`addr_t` address parameter.

Operators are executed in precedence order, for example in `1+2*3` we have two operators, `+` and `*`, but `*` has the higher
precedence, therefore first `2*3` is calculated, and then `1+6`. That's why the result is 7 and not 9.

<h2 bas_flow>Control Flow</h2>

The statement `END` stops control flow (exists your program).

MEG-4 BASIC does not use line numbers any more, instead it supports `GOTO` with labels, for example:

```bas
this_is_a_label:

GOTO this_is_a_label
```

WARN: Some BASIC dialects allow you to use multiple commands separated by `:` in one line. In MEG-4 BASIC `:` identifies
a label, so you must use one command per line (as expected by ECMA-55).

Conditional jumps use labels too:

```bas
IF a$ <> b$ THEN this_is_a_label
ON a GOTO label1, label2, label3 ELSE labelother
```

The `ON` .. `GOTO` always needs a numerical expression, and chooses the label accordingly, starting from 1 (if the expression
is zero or negative, that always jumps to the `ELSE` label). There's no `ON` .. `GOSUB`, because GOSUB does not accept labels in
MEG-4 BASIC.

WARN: The `GOSUB` statement does not accept labels at all, and its semantics are a bit changed in MEG-4 BASIC, see below.

For `IF`, both numerical and relational expressions are accepted (every non-zero expression considered true), and what's more,
multi line `IF` .. `THEN` .. `ELSE` .. `END IF` blocks are supported too (but no `SELECT CASE`).

```bas
IF var >= 0 THEN
 PRINT "var is positive"
ELSE
 PRINT "var is negative"
END IF
```

As an exception, one command is allowed in a single line `IF`, provided that's a `GOTO` or an `END`:

```bas
IF a < 0 THEN GOTO label
IF b > 42 THEN END
```

For iterations, the counting loop checks its condition before the iteration (does not execute the block if the initial value is
greater (or less) than the limit), and looks like this:

```bas
FOR i = 1 TO 100 STEP 2
...
NEXT i
```

This `FOR` .. `NEXT` is essentially the same as:

```bas
LET i = 1
LET lim = 100
LET inc = 2
line1:
IF (i - lim) * SGN(inc) > 0 THEN line2
...
LET i = i + inc
GOTO line1
line2:
```

The loop variable must be of float type. The `STEP` is optional (defaults to 1.0), and the expression after that can be a float
literal or another float variable. The from and limit both can be more complex expressions, but they too must return a floating
point value.

```bas
FOR i = (1+2+a)*3 TO 4*(5+6)/b+c STEP j
...
NEXT i
```

WARN: Unlike the specification, which allows multiple variables after `NEXT`, MEG-4 BASIC accepts *exactly one*. So for
nested loops you'll have to use multiple `NEXT` commands, exactly as many as `FOR` statements there are.

```bas
FOR y = 1 TO 10
 FOR x = 1 TO 100
 ...
 NEXT x
NEXT y
```

MEG-4 BASIC has no other kind of loops like C, but if you want a non-counting pre-testing loop, you can do this:

```bas
again:
IF a > 0 THEN
 ...
 GOTO again
END IF
```

And instead of a post-testing loop:

```bas
again:
 ...
IF a > 0 THEN again
```

<h2 bas_funcs>Subroutines and Functions</h2>

INFO: Statements not inside of any subroutine are simply threated as if they were inside the `setup` subroutine.

You can divide your programs into smaller programs which might be called multiple times, these are called subroutines. They are
defined between `SUB` and `END SUB` blocks. Two of these, `setup` and `loop` has special meaning, see [code editor]. As mentioned
earlier, in MEG-4 BASIC `GOSUB` does not accept labels, and that's because here it accepts subroutine names only.

```bas
SUB mysubroutine
 PRINT "do something that you want to do multiple times"
END SUB

REM do it once
GOSUB mysubroutine
REM then do it again
GOSUB mysubroutine
```

Control is transferred on `GOSUB`, and it is returned to the line after `GOSUB` when `END SUB` (or the optional `RETURN`) reached.
Subroutines can access global variables and they might have parameters.

```bas
SUB are_we_there_yet(A)
 IF A > 0 THEN
  PRINT "Not yet"
  RETURN
 END IF
PRINT "YES! Do things we wanted to do on arrival"
END SUB

REM do it once
GOSUB are_we_there_yet(1)
REM then do it again
GOSUB are_we_there_yet(0)
```

Functions are very similar, but they must have a `RETURN` and their `RETURN` statement must contain a returned value, same type
as identified by the function's name. Functions are simply called from programs by their names, followed by their argument list
in parenthesis (the parenthesis is mandatory, even if the argument list is empty). For example:

```bas
FUNCTION mystringfunc$()
 RETURN "a string"
END FUNCTION

LET a$ = mystringfunc$()
```

<h2 bas_print>Print Statement</h2>

```
PRINT expression [;|,] [expression [;|,] [expression [;|,]]] ...
```

Prints one or more exporession on screen. If the expressions are separated by `;` semi-colon, then tightly one after another.
If by `,` colon, then the output will be splitted into coloumns. Numbers are always prefixed by a space, and if the command ends
in an expression (not in `;` nor in `,`), then a newline character will be printed at the end as well.

<h2 bas_input>Input Statement</h2>

```
INPUT "prompt" [;|,] variable
```

Prints out prompt, then reads in a value from user and stores it in the given variable. If the prompt and the variable is
spearated by a `,` colon, then it also prints a `?` question-mark after the prompt.

WARN: The ECMA-55 specification allows multiple variables to be specified, but the MEG-4 BASIC allows only one.

<h2 bas_spec>Peek and Poke</h2>

<p>You can directly access the MEG-4 memory with these commands, even the MMIO area.</p>

**Read**
```
variable = PEEK(address)
```

Reads in the byte at the given address, converts it into a floating point and stores it in the given variable.

For example, to check if the keyboard queue is not empty:
```bas
IF PEEK($1A) <> PEEK($1B) THEN
```

**Store**
```
POKE address, expression
```

Calculates the expression, converts it into a byte and stores that byte value at the given address.

For example, to set the palette for color index 1:
```bas
REM red component
POKE $84, 10
REM green component
POKE $85, 10
REM blue component
POKE $86, 10
REM alpha (transparency)
POKE $87, 255
```

<h2 bas_api>Provided Functions</h2>

Some MEG-4 API are provided as system variables, `RND` ([rnd]), `TIME` ([time]), `NOW%` ([now]), and `INKEY$` ([getc]).

Others are provided as commands, `INPUT` ([gets] + [val]), `PRINT` ([printf]), `PEEK` ([inb]), `POKE` ([outb]).

As to comply with ECMA-55, two functions are renamed: `SQR` ([sqrt]) and `ATN%` ([atan]). All the rest is used as they appear
in this documentation, except they are properly suffixed according their return types (for example, [str] returns a string, so
it is called `STR$`).

Note that ECMA-55 expects that trigonometrical functions use radians by default (with an `OPTION` command to switch to degrees),
however the MEG-4 API always uses degrees, 0 to 359, where 0 is up, and 90 is to the right. That's why the `ATN%` gets an
integer type suffix for example, as it returns degrees in an integer.
