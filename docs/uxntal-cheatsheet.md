# Uxntal cheat sheet

A quick reference for writing `.tal` programs. The opcode table matches
`emu/core/uxn.c`. The syntax table was checked by assembling a sample of each
rune with all three assemblers.

## Opcodes

Stack effects read `( before -- after )`, with the top of the stack on the
right. `rs:` is the return stack.

| Hex  | Op  | Stack effect                  | What it does |
|------|-----|-------------------------------|--------------|
| `00` | BRK | `( -- )`                      | End the current vector (see [immediates](#immediate-opcodes)) |
| `01` | INC | `( a -- a+1 )`                | Increment |
| `02` | POP | `( a -- )`                    | Drop the top item |
| `03` | NIP | `( a b -- b )`                | Drop the second item |
| `04` | SWP | `( a b -- b a )`              | Swap |
| `05` | ROT | `( a b c -- b c a )`          | Rotate |
| `06` | DUP | `( a -- a a )`                | Duplicate |
| `07` | OVR | `( a b -- a b a )`            | Copy the second item to the top |
| `08` | EQU | `( a b -- a==b )`             | Equal: pushes a byte, 1 or 0 |
| `09` | NEQ | `( a b -- a!=b )`             | Not equal |
| `0a` | GTH | `( a b -- a>b )`              | Greater than |
| `0b` | LTH | `( a b -- a<b )`              | Less than |
| `0c` | JMP | `( addr -- )`                 | Jump |
| `0d` | JCN | `( cond8 addr -- )`           | Jump if `cond8` is not zero |
| `0e` | JSR | `( addr -- ) rs: ( -- pc* )`  | Jump, saving the return address |
| `0f` | STH | `( a -- ) rs: ( -- a )`       | Move to the other stack |
| `10` | LDZ | `( addr8 -- v )`              | Load from the zero page |
| `11` | STZ | `( v addr8 -- )`              | Store to the zero page |
| `12` | LDR | `( rel8 -- v )`               | Load, relative to the program counter |
| `13` | STR | `( v rel8 -- )`               | Store, relative to the program counter |
| `14` | LDA | `( addr16 -- v )`             | Load from an absolute address |
| `15` | STA | `( v addr16 -- )`             | Store to an absolute address |
| `16` | DEI | `( port8 -- v )`              | Read from a device |
| `17` | DEO | `( v port8 -- )`              | Write to a device |
| `18` | ADD | `( a b -- a+b )`              | Add |
| `19` | SUB | `( a b -- a-b )`              | Subtract |
| `1a` | MUL | `( a b -- a*b )`              | Multiply |
| `1b` | DIV | `( a b -- a/b )`              | Divide |
| `1c` | AND | `( a b -- a&b )`              | Bitwise and |
| `1d` | ORA | `( a b -- a\|b )`             | Bitwise or |
| `1e` | EOR | `( a b -- a^b )`              | Bitwise exclusive or |
| `1f` | SFT | `( a shift8 -- c )`           | `c = (a >> (shift8 & 0xf)) << (shift8 >> 4)` |

Jumps take a signed one-byte offset normally, and an absolute address in short
mode.

### Modes

Add these bits to an opcode, or these suffixes to its name, in the order `2kr`.

| Bit    | Suffix | Mode   | Effect |
|--------|--------|--------|--------|
| `0x20` | `2`    | short  | Work on 16-bit values |
| `0x40` | `r`    | return | Use the return stack in place of the working stack |
| `0x80` | `k`    | keep   | Leave the inputs on the stack |

For example, `ADD` is `18`, `ADD2` is `38`, `ADDr` is `58`, `ADDk` is `98`,
and `ADD2kr` is `f8`.

### Immediate opcodes

An opcode with a low five bits of zero has no modes. Each of the eight values
is its own instruction.

| Hex  | Op    | What it does |
|------|-------|--------------|
| `00` | BRK   | End the current vector |
| `20` | JCI   | Pop `cond8`; jump by the next 16 bits if it is not zero |
| `40` | JMI   | Jump by the next 16 bits |
| `60` | JSI   | Save the return address; jump by the next 16 bits |
| `80` | LIT   | Push the next byte |
| `a0` | LIT2  | Push the next 2 bytes |
| `c0` | LITr  | Push the next byte to the return stack |
| `e0` | LIT2r | Push the next 2 bytes to the return stack |

## Syntax

There are two dialects in this repo. Modern Uxntal is what the vendored
**drifloon** assembler accepts, and Left is written in it. The 2022 dialect is
what **uxnasm** (the May 2022 build) and **talie** accept, and the projects
in `games/`, `utils/` and `langs/` are written in it.

| Rune        | Meaning | Emits | drifloon | uxnasm 2022 | talie |
|-------------|---------|-------|:--:|:--:|:--:|
| `( ... )`   | Comment | nothing | yes | yes | yes |
| `[ ... ]`   | Ignored; for grouping visually | contents only | yes | yes | yes |
| `%name { }` | Define a macro | nothing | yes | yes | yes |
| `\|addr`    | Move to an absolute address | nothing | yes | yes | yes |
| `$n`        | Skip `n` bytes | padding | yes | yes | yes |
| `@name`     | Define a label | nothing | yes | yes | yes |
| `&name`     | Define a sublabel of the last `@` label | nothing | yes | yes | yes |
| `#12`       | Literal byte | `LIT 12` | yes | yes | yes |
| `#1234`     | Literal short | `LIT2 1234` | yes | yes | yes |
| `12`, `1234`| Raw hex | the bytes | yes | yes | yes |
| `"text`     | Raw ASCII | the bytes | yes | yes | yes |
| `.label`    | Zero-page address | `LIT` + byte | yes | yes | yes |
| `,label`    | Relative address | `LIT` + byte | yes | yes | yes |
| `;label`    | Absolute address | `LIT2` + short | yes | yes | yes |
| `~file.tal` | Include a file | that file | no: ignored | yes | yes |
| `'c`        | Raw character | the byte | no | yes | yes |
| `:label`    | Raw absolute address | short | no: use `=` | yes | yes |
| `=label`    | Raw absolute address | short | yes | no | no |
| `-label`    | Raw zero-page address | byte | yes | no | no |
| `_label`    | Raw relative address | byte | yes | no | no |
| `!label`    | Jump | `JMI` + offset | yes | no | no |
| `?label`    | Jump if the top byte is not zero | `JCI` + offset | yes | no | no |
| `label`     | Call | `JSI` + offset | yes | no | `;label JSR2` |
| `{ ... }`   | Anonymous block; its address goes on the return stack | `JSI` over the body | yes | no | no |

Sublabels can be referenced as `&bar` inside their parent, or as `foo/bar`
anywhere. drifloon also accepts `/bar` inside the parent.

Labels that could be read as hex, such as `@a`, are rejected by drifloon.

Because drifloon ignores `~`, multi-file modern programs are joined together
before assembling. The emulator's build does this for Left.

talie also has block keywords of its own, such as `word`, `vector`, `if`/`else`
and `loop`. `games/pico8-worm/worm.talie` is written that way.

## Memory and devices

| Address       | Use |
|---------------|-----|
| `0000`-`00ff` | Zero page |
| `0100`        | Reset vector: programs start here |

| Port | Device |
|------|--------|
| `00` | System |
| `10` | Console |
| `20` | Screen |
| `30`-`60` | Audio (not implemented in `emu/`) |
| `80` | Controller |
| `90` | Mouse |
| `a0`, `b0` | File |
| `c0` | DateTime |

## Example

```
|10 @Console &vector $2 &read $1 &pad $5 &write $1

|0100
	#41 .Console/write DEO  ( print "A" )
	BRK
```

## Further reading

- [Uxntal reference](https://wiki.xxiivv.com/site/uxntal_reference.html)
- [Varvara devices](https://wiki.xxiivv.com/site/varvara.html)
