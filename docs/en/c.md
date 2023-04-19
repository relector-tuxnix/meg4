C
=

If you want to use this language, then start your program with a `#!c` line.

<h2 ex_c>Example Program</h2>

```c
#!c

/* global variables */
int acounter = 123;
float anumber = 3.1415;
addr_t anaddress = 0x0048C;
str_t astring = "something";
uint32_t anarray[10];

/* Things to do on startup */
void setup()
{
  /* local variables */
  int iamlocal = 123;
}

/* Things to run for every frame, at 60 FPS */
void loop()
{
  /* Get MEG-4 style outputs */
  printf("a counter %d, left shift %d\n", acounter, getkey(KEY_LSHIFT));
}
```

Description
-----------

The default language of the console is **MEG-4 C**. Despite being a very simple language, it is for somewhat intermediate
programmers. If you're a total beginner, then I'd recommend using [BASIC] instead.

It was created as a deliberately simplified ANSI C to help learning programming. Hence it is limited, not everything is
supported that ANSI C expects, but replacing

```
#!c
```

with

```c
#include <stdint.h>
typedef char* str_t;
typedef void* addr_t;
```

will make the MEG-4 C source to compile with any standard ANSI C compiler (gcc, clang, tcc etc.).

It has one non-standard keyword, the `debug;`, which you can place anywhere in your code and will invoke the built-in
[debugger]. After this, you can execute your code step by step, watching what it is doing.

Here comes a gentle introduction to the C language, focusing on what's special in MEG-4 C.

Pre-compiler
------------

Because there's only one source, and system function prototypes are supported out-of-the-box, there's no need for header files.
The pre-compiler therefore is limited to simple (non-macro) defines and conditional source code blocks only.

```c
/* replace all occurance of (defvar) with (expression) */
#define (defvar) (expression)
/* include code block if (defvar) is defined */
#ifdef (defvar)
/* include code block if (defvar) is not defined */
#ifndef (defvar)
/* include code block if (expression) is true */
#if (expression)
/* else block */
#else
/* end of conditional code block inclusion */
#endif
```

You can use enumerations with the `enum` keyword, separated by commas and enclosed in curly brackets. Each element will be one
bigger than the previous one. These act as if you had multiple rows of defines. For example the following two are identical:

```c
#define A 0
#define B 1
#define C 2
```

```c
enum { A, B, C };
```

It is also possible to assign direct values using the equal sign, for example:

```c
enum { ONE = 1, TWO, THREE, FIVE = 5, SIX };
```

<h2 c_lit>Literals</h2>

MEG-4 C understands decimal based numbers (either integer or floating point with or without scientific notation). Hexadecimal
numbers must be prefixed by `0x`, binary by `0b`, octal by `0`; characters must be surrounded by apostrophes and strings must
be enclosed in double quotes:

```c
42
3.1415
.123e-10
0777
0b00100110
0x0048C
'á'
"Goodbye and thanks for all the fish!\n"
```

<h2 c_var>Variables</h2>

Unlike in BASIC, variables must be declared. You can place these declaration in two places: at the top level, or at the
beginning of a function body. The former become global variable (accessible by all functions), while the latter will
be a local variable, accessible only to the function where it was declared. Another difference, that global variables
can be initialized (a value assigned to them using `=`), while local variables cannot be, you must explicitly write code
to set their values.

A declaration consist of two things: a type and a name. MEG-4 C supports all ANSI C types: `char` (signed byte), `short`
(signed word), `int` (signed integer), `float` (floating point). You might also put `unsigned` in front of these to make
them, well, unsigned. In ANSI C `int` can be omitted with `short`, but in MEG-4 you must not use it. So `short int` isn't a
valid type, `short` in itself is. Furthermore MEG-4 C supports and prefers standard types instead (defined in stdint.h under
ANSI C). These have some simple rules: if they are unsigned, then they start with the letter `u`; then `int` means integer
type, followed by the number of bits they occupy, and finally suffixed by a `_t` which stands for type. For example,
`int` is the same as `int32_t` and `unsigned short` is the same as `uint16_t`. Examples:

```c
int a = 42;
uint8_t b = 0xFF;
short c = 0xFFFF;
str_t d = "Something";
float e = 3.1415;
```

Unlike in ANSI C, which allows only English letters in variable names, MEG-4 C allows anything that does not start with a
number and isn't a keyword. For example, `int déjà_vu;` is perfectly valid (note how the name contains non-English letters).

<h2 c_arr>Arrays and Pointers</h2>

Multiple elements of the same type can be assigned to a single variable, which is called an array. Pointers are special variables
that contain a memory address which points to a list of variables of the same type. The similarity between these two isn't a
coincidence, but there are subtle differences.

There's no special command for arrays like in BASIC, instead you just specify the number of elements between `[` and `]` after
the name.

```c
int anarray[10];
```

Referencing an array's value happens similarily, with an index between `[` and `]`. The index starts at 0, and array bounds are
checked. MEG-4 supports up to 4 dimensions.

To declare a pointer, one has to prefix the variable name with a `*`. Because C does not recognize string type, and strings are
actually just bytes one after another in memory, therefore we use `char*` pointer. This might be strange at first, so MEG-4 C
defines the `str_t` type, but this is actually the same as `char*`.

Because pointers hold an address, you must give an address as their value (using `&` returns the address of the variables), and
pointers always return an address. In order to get the value at that address, you must de-reference the pointer. You have two
options to do this: either prefix it with `*`, or suffix it with an index between `[` and `]`, just like with arrays. For example
the two reference in the second printf are the same:

```c
int variable = 1;
int *pointer = &variable;

printf("pointer's value (address): %x\n", pointer);
printf("pointed value: %x %x\n", *pointer, pointer[0]);
```

You cannot mix pointers with arrays, because that would be ambiguous. For example

```c
int *a[10];
```
Could mean a single pointer that points to a list of 10 contiguous integers (`*(int[10])`), but it could also mean 10 independent
pointers each pointing to a single, not neccessarily contiguous integers (`(*int)[10]`). Not obvious which one, so pointers and
arrays cannot be mixed within the same declaration.

WARN: Unlike arrays, there's no bound check with pointers!

<h2 c_op>Operators</h2>

In descending order of precedence:

<p>**Arithmetic**
```
* / %
+ -
```
The first one is multiplication, division and `%` is modulus (aka. remainder after division, eg. `10 % 3` is 1), addition and
subtraction.</p>

<p>**Relational**
```
!= ==
<= >= < >
```
Not equal, equal, less or equal, greater or equal, less than, greater than.</p>

<p>**Logical**
```
!
&&
||
```
Logical negation (0 becomes 1, everything non-zero becomes 0), and (returns 1 if both arguments are non-zero), or (returns
1 if at least one of the arguments are non-zero).</p>

<p>**Bitwise**
```
~
&
|
<< >>
```
Bitwise negation, bitwise and, bitwise or, bitwise shift to the left and right. Note that these are the same operators like
logical ones, but instead of the whole value, they operate on each binary digits individually. For example logical not
`!0x0100 == 0`, but the bitwise not `~0x0100 == 0x1011`. Bitwise shifting is the same as multiplying or dividing by power of
two. For example shifting a value to the left by 1 is the same as multipying it by 2.</p>

<p>**Incremental**
```
++ --
```
Increment and decrement. You can use these as a prefix and as a suffix as well. When used as prefix, like `++a`, then the
variable is increased and the increased value is returned; as a suffix `a++` it will increase the value too, but returns
the value *before* the increment.</p>

```c
a = 0; b = ++a * 3;     /* a == 1, b == 3 */
a = 0; b = a++ * 3;     /* a == 1, b == 0 */
```

<p>**Conditional**
```
?:
```
This is a trinary operator, with three operands, separated by `?` and `:`. If the first operand is true, then the whole
expression is replaced by the second operand, otherwise with the third. For this the type of the second and third operands
must be the same. For example `a >= 0 ? "positive" : "negative"`.</p>

<p>**Assignment**
```
= *= /= %= += -= ~= &= |= <<= >>=
```
The first places the value of an expression in a variable. The others execute the operation on that variable, and then store
the result in the very same variable. For example these are identical: `a *= 3;` and `a = a * 3;`.</p>

Unlike other languages, in C the assignment is an operator too. This means they can appear anywhere in an expression, for example
`a > 0 && (b = 2)`. That's why assignment is `=` and logical equal is `==`, so that you can use both in the same expression.

There's also the address-of operator, the `&` which returns the address of a variable. This is usable when the MEG-4 API expects
an `addr_t` address parameter.

Operators are executed in precedence order, for example in `1+2*3` we have two operators, `+` and `*`, but `*` has the higher
precedence, therefore first `2*3` is calculated, and then `1+6`. That's why the result is 7 and not 9.

<h2 c_flow>Control Flow</h2>

Unlike BASIC where the primary way of altering the control flow is defining labels, in C (as being a structured language) you
specify blocks of instructions instead. If you want to handle multiple instructions together, then you place them between `{`
and `}` (but it is possible to put only one instruction in a block).

Like everythig else, the conditionals use such blocks too:

```c
if(a != b) {
    printf("a not equal to b\n");
} else {
    printf("a equals b\n");
}
```

You can add an `else` branch, which executes when then the expression is false, but using `else` is optional.

With multiple possible values, you can use a `switch` statement. Here each `case` acts as a label, being choosen depending on the
expression's value.

```c
switch(a) {
    case 1: printf("a is 1.\n");
    case 2: printf("a is either 1 or 2.\n"); break;
    case 3: printf("a is 3.\n"); break;
    default: printf("a is something else.\n"); break;
}
```

There's a special block, defined by the label `default`, which matches any value that doesn't have its own `case`. These blocks
are concatenated, so if the control jumps to a `case`, then that block, and every other blocks after that is executed. In the
example above, if `a` is 1, then two printfs will be called. To stop this from happening, you must use `break` to exit the `switch`.

C supports three iteration types: pre-testing loop, post-testing loop and counting loop.

The pre-testing loop checks the conditional expression first, and does not run the iteration body at all if its false.

```c
while(a != 1) {
    ...
}
```

The post-testing loop runs the iteration body at least once, it checks the condition afterwards, and repeats only if its true.

```c
do {
    ...
} while(a != 1);
```

The counting loop in C is pretty universal. It expects three expressions, in order: initialization, conditional, stepping. Because
you can freely define these, it is possible to use multiple variables or whatever expressions you like (not necessarily counting).
Example:

```c
for(a = 0; a < 10; a++) {
    ...
}
```

This is the same as:

```c
a = 0;
while(a < 10) {
    ...
    a++;
}
```

You can exit the iteration by using the `break` statement in its body, but with loops you can also use `continue`, which breaks
the execution of the block too, but instead of exiting it continues from the next iteration.

```c
for(a = 0; a < 100; a++) {
    if(a < 10) continue;
    if(a > 50) break;
    printf("a value between 10 and 50: %d\n", a);
}
```

<h2 c_funcs>Functions</h2>

INFO: No statements allowed outside of function bodies.

You must divide your programs into smaller programs which might be called multiple times, these are called functions. These
are declared as return value's type, name, argument list in `(` and `)` parenthesis, and function body in a `{` and `}` block.
Two of these, `setup` and `loop` has special meaning, see [code editor]. The C language does not differentiate between
subroutines and functions; everything is a function. The only difference is, functions that do not return a value has the
return value's type as `void`.

```c
void are_we_there_yet(int A)
{
 if(A > 0) {
  printf("Not yet\n")
  return;
 }
 printf("YES! Do things we wanted to do on arrival\n");
 return;
}

void setup()
{
 /* do it once */
 are_we_there_yet(1);
 /* then do it again */
 are_we_there_yet(0);
}
```

Functions are simply called from programs by their names, followed by their argument list in parenthesis (the parenthesis is
mandatory, even if the argument list is empty). There's no special command and there's no difference in the call if the function
returns a value or not.

Returning from a function is done by the `return;` instruction. If the function has a return value, then you must specify an
expression after the `return`, and that expression's type must be the same as the function's return type. Specifying `return`
(independly if the function has a return value or not) is mandatory.

```c
str_t mystringfunc()
{
 return "a string";
}

void setup()
{
 a = mystringfunc();
}
```

<h2 c_api>Provided Functions</h2>

The C language has no special commands for input or output; you simply just do MEG-4 API calls for those. The [getc] returns
a character, [gets] returns a string and you can print out strings with [printf].

Under MEG-4 C you use the MEG-4 API exactly as it is specified in this documentation, there are no tricks, no renaming, no
suffixes, nor substitutions either.
