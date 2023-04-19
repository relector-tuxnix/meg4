Assembly
========

If you want to use this language, then start your program with a `#!asm` line.

<h2 ex_asm>Example Program</h2>

```bc
#!asm

    .data
/* global variables */
acounter:   di 123
anumber:    df 3.1415
anaddress:  di 0x0048C
astring:    db "something"
anarray:    di 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
fmt:        db "a counter %d, left shift %d\n"

    .code
/* Things to do on startup */
setup:
  /* local variables (not really, just reserve space on the stack) */
  sp -4
  ret

/* Things to run for every frame, at 60 FPS */
loop:
  /* Get MEG-4 style outputs */
  pshci KEY_LSHIFT
  scall getkey
  sp 4
  pushi
  ci acounter
  ldi
  pushi
  pshci fmt
  scall printf
  sp 12
  ret
```

<h2 asm_desc>Description</h2>

This isn't an actual programming language. When you compile one of the built-in languages, the compiler generates bytecode that
the CPU executes. Assembly is a one-to-one transcription of that bytecode with human-readable textual mnemonics. It has two
sections, data and code, both containing a series of labels and instructions. Instructions are mnemonics with an optional
parameter.

You can play around and experiment with this if you feel brave.

<h2 asm_lit>Literals</h2>

Same as [MEG-4 C literals](#c_lit).

<h2 asm_var>Variables</h2>

There's no such thing as a variable in Assembly. Instead you specify the data section with `.data`, and just place the data after
one of the `db` (byte), `dw` (word), `di` (integer), `df` (float) instructions. In this stream of data, before the instruction,
you can place labels which will hold the address of that data. To load a value in your code section, first you put that label in
the accumulator register using `ci`, and then issue one of the `ldb` (load byte), `ldw` (load word), `ldi` (load integer)
or `ldf` (load float) instructions. If the `ldb` or `ldw` instructions has a non-zero argument, then they sign-extend the value
to 32 bit.

<h2 asm_flow>Control Flow</h2>

Everything that you place after the `.code` keyword will be code. There's no implicit control flow, each instruction is executed
one after another; you have to alter the PC (program counter) using one of the `jmp` (jump), `jz` (jump if zero), `jnz` (jump if
not zero) or `sw` (switch, case selection) instructions to change the control flow manually.

<h2 asm_func>Functions</h2>

There's no function declaration. You just use a label to mark a spot in the code. You push all arguments on the stack *in reverse
order* using `pushi` and `pushf`, then you use a `call` mnemonic with that label. Within the function, you can get the address of
these parameters with the `adr` (address) instruction, with its parameter being the function parameter's number multiplied by four.
For example `adr 0` loads the first function parameter's address in the accumulator register, and `adr 4` loads the second's. You
can return from the function with the `ret` instruction. Return values are returned in the accumulator register, which you can set
directly with the `ci` (constant integer) and `cf` (constant float) instructions, and indirectly with the `popi`, `popf`, `ldb`,
`ldw`, `ldi` and `ldf` instructions. After the call it is the caller's responsibility to remove the parameters from the stack by
using an `sp` + number of parameters times four instruction.

<h2 asm_api>Provided Functions</h2>

You have all the MEG-4 API functions at your disposal; using the exact names as they are listed in this documentation.

You have to push all arguments on the stack *in reverse order*, then use an `scall` (system call) mnemonic with a MEG-4 API
function name as parameter. After the call it is the caller's responsibility to remove the parameters from the stack.

Mnemonics
---------

Before we go into the details, we must talk about the MEG-4 CPU specification.

The MEG-4 CPU is a 32-bit, little endian CPU. All values are stored in a way that smallest digits are placed on the smaller
addresses. It is capable of performing operations on 8 bit, 16 bit and 32 bit integers (signed and unsigned) and on 32 bit
floating point numbers.

The memory model is flat, meaning all data can be accessed through a single offset. Has no paging and no virtual address
translation, no segmentation, except all data and code segment references are implicit (aka. no segment prefixes, referencing
segments are automatic).

For security reasons code segment and data segment are separated, as well as the call stack and the data stack. Stack overflow
and any other code injection through buffer overflow attacks are simply impossible on this CPU, which makes it very secure and
bullet-proof (also supporting 3rd party bytecode like [Lua] would be impossible without code separation). In this reagard it is
more like a Harvard architecture, but in every other aspect it's more like a von Neumann architecture.

The CPU has the following registers:

- AC: accumulator register, with an integer value
- AF: accumulator register, with a floating point value
- FLG: processor flags (setup is done, blocked for I/O, blocked for timer, execution stopped)
- TMR: the timer register's current value
- DP: data pointer, this points to the top of the used global variable memory
- BP: base pointer, marks the top of the function stack frame
- SP: stack pointer, the bottom of the stack
- CP: callstack pointer, the top of the callstack
- PC: program counter, the address of the instruction currently being executed

The data segment is byte based, meaning DP, BP and SP registers point to 8 bit units. You can place data to the data segment
using the `db` (8 bit), `dw` (16 bit), `di` (32 bit) and `df` (32 bit float) mnemonics. These might have one or more comma
separated arguments, and for `db` strings literals and character literals can also be used.

The code segment has a 32 bit granularity, meaning that's the smallest address unit you can use. For this reason the PC points to
these 32 bit units and not bytes.  The following mnemonics can be used to place instructions to the code segment:

| Mnemonic | Parameter               | Description                                                                         |
|----------|-------------------------|-------------------------------------------------------------------------------------|
| `debug`  |                         | Invoke the built-in [debugger] (nop for MEG-4 PRO)                                  |
| `ret`    |                         | Return from `call`, pops from the call stack                                        |
| `scall`  | MEG-4 API function      | System call                                                                         |
| `call`   | address/code label      | Pushes the position to the call stack and then calls a function                     |
| `jmp`    | address/code label      | Jump to address                                                                     |
| `jz`     | address/code label      | Jump to address if accumulator is zero                                              |
| `jnz`    | address/code label      | Jump to address if accumulator isn't zero                                           |
| `js`     | address/code label      | Pop value, adjust its sign and jump to address if negative or zero                  |
| `jns`    | address/code label      | Pop value, adjust its sign and jump to address if positive                          |
| `sw`     | num,addr,addr0,addr1... | Switch (see below)                                                                  |
| `ci`     | number/data label       | Place an integer value into the accumulator                                         |
| `cf`     | number                  | Place a floating point number into the accumulator                                  |
| `bnd`    | number                  | Checks if the accumulator's value is between 0 and number                           |
| `lea`    | number                  | Loads the address DP + number into the accumulator                                  |
| `adr`    | number                  | Loads the address BP + number into the accumulator                                  |
| `sp`     | number                  | Adjust SP register by number                                                        |
| `pshci`  | number/data label       | Push an integer constant to the data stack                                          |
| `pshcf`  | number                  | Push a float constant to the data stack                                             |
| `pushi`  |                         | Push the accumulator as integer to the data stack                                   |
| `pushf`  |                         | Push the accumulator as float to the data stack                                     |
| `popi`   |                         | Pop an integer value from the data stack into the accumulator                       |
| `popf`   |                         | Pop a float value from the data stack into the accumulator                          |
| `cnvi`   |                         | Convert the value on the top of the stack into an integer                           |
| `cnvf`   |                         | Convert the value on the top of the stack into a float                              |
| `ldb`    | 0/1                     | Loads a byte from the address in the accumulator (sign extend if arg is non-zero)   |
| `ldw`    | 0/1                     | Loads a word from the address in the accumulator (sign extend if arg is non-zero)   |
| `ldi`    |                         | Loads an integer from the address in the accumulator                                |
| `ldf`    |                         | Loads a float from the address in the accumulator                                   |
| `stb`    |                         | Pops the address from stack and stores a byte from accumulator                      |
| `stw`    |                         | Pops the address from stack and stores a word from accumulator                      |
| `sti`    |                         | Pops the address from stack and stores an int from accumulator                      |
| `stf`    |                         | Pops the address from stack and stores a float from accumulator                     |
| `incb`   | number                  | Pops the address from stack and increase the byte at address by number              |
| `incw`   | number                  | Pops the address from stack and increase the word at address by number              |
| `inci`   | number                  | Pops the address from stack and increase the integer at address by number           |
| `decb`   | number                  | Pops the address from stack and decrease the byte at address by number              |
| `decw`   | number                  | Pops the address from stack and decrease the word at address by number              |
| `deci`   | number                  | Pops the address from stack and decrease the integer at address by number           |
| `not`    |                         | Perform logical NOT on the accumulator                                              |
| `neg`    |                         | Perform bitwise NOT on the accumulator                                              |
| `or`     |                         | Pop a value from stack and perform bitwise OR on the accumulator                    |
| `xor`    |                         | Pop a value from stack and perform EXCLUSIVE OR on the accumulator                  |
| `and`    |                         | Pop a value from stack and perform bitwise AND on the accumulator                   |
| `shl`    |                         | Pop a value and shift accumulator bits to the left, place result in accumulator     |
| `shr`    |                         | Pop a value and shift accumulator bits to the right, place result in accumulator    |
| `eq`     |                         | Pop a value and set accumulator if accumulator is the same as the popped            |
| `ne`     |                         | Pop a value and set accumulator if accumulator isn't the same                       |
| `lts`    |                         | Pop a value and set accumulator if it's less than as signed                         |
| `gts`    |                         | Pop a value and set accumulator if it's greater than as signed                      |
| `les`    |                         | Pop a value and set accumulator if it's less or equal as signed                     |
| `ges`    |                         | Pop a value and set accumulator if it's greater or equal as signed                  |
| `ltu`    |                         | Pop a value and set accumulator if it's less than as unsigned                       |
| `gtu`    |                         | Pop a value and set accumulator if it's greater than as unsigned                    |
| `leu`    |                         | Pop a value and set accumulator if it's less or equal as unsigned                   |
| `geu`    |                         | Pop a value and set accumulator if it's greater or equal as unsigned                |
| `ltf`    |                         | Pop a value and set accumulator if it's less than as float                          |
| `gtf`    |                         | Pop a value and set accumulator if it's greater than as float                       |
| `lef`    |                         | Pop a value and set accumulator if it's less or equal as float                      |
| `gef`    |                         | Pop a value and set accumulator if it's greater or equal as float                   |
| `addi`   |                         | Pop a value and add it to the accumulator as integer                                |
| `subi`   |                         | Pop a value and subtract the accumulator from it as integer                         |
| `muli`   |                         | Pop a value and multiply the accumulator with it as integer                         |
| `divi`   |                         | Pop a value and divide by the accumulator as integer                                |
| `modi`   |                         | Pop a value, divide and put the remainder in accumulator as integer                 |
| `powi`   |                         | Pop a value and raise to the power of accumulator as interger                       |
| `addf`   |                         | Pop a value and add it to the accumulator as float                                  |
| `subf`   |                         | Pop a value and subtract the accumulator from it as float                           |
| `mulf`   |                         | Pop a value and multiply the accumulator with it as float                           |
| `divf`   |                         | Pop a value and divide by the accumulator as float                                  |
| `modf`   |                         | Pop a value, divide and put the fractional in accumulator as float                  |
| `powf`   |                         | Pop a value and raise to the power of accumulator as float                          |

The `sw` mnemonic has variable number (but at least three) arguments. The first one is a number, the second is a code label, as
well as all the rest are code labels. It subtracts the number from the accumulator and checks if the result is positive and
less than the number of the labels given. If not, then it jumps to the first label in the second argument. If it is, then it
picks the accumulatorth label starting from the third parameter (second label), and jumps there.

So in a nutshell it is
```
sw (value), (label to jump to otherwise),
  (label to jump to if accumulator equals value),
  (label to jump to if accumulator equals value + 1),
  (label to jump to if accumulator equals value + 2),
  (label to jump to if accumulator equals value + 3),
  ...
  (label to jump to if accumulator equals value + N)
```

Every `sw` mnemonic might have up to 256 value labels.
