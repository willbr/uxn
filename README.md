# uxn

Experiments with the [uxn](https://100r.co/site/uxn.html) virtual machine: a
uxn/Varvara emulator written from scratch, plus games, tools and language
experiments that run on uxn.

| Folder   | What's in it |
|----------|--------------|
| `emu/`   | The emulator: strict C89 CPU core, SDL3 window, cimgui debugger. It runs the [Left](https://100r.co/site/left.html) text editor. |
| `games/` | `pong`, `pico8-worm`, `10-print`, and notes for `squiddy`. |
| `utils/` | `talie` (a Uxntal assembler in Python, with a disassembler, emulator, REPL and tests), `bin2tal`, `readline`. |
| `langs/` | Language experiments: `forth`, `lisp`, `threif` (a 3-instruction Forth), `uxnfasm` (Forth to Uxntal), `starting`. |
| `docs/`  | The [Uxntal cheat sheet](docs/uxntal-cheatsheet.md), and the emulator's design and plan. |

## Getting started

### The emulator

```
cd emu
clang -o nob.exe nob.c
./nob.exe test
./nob.exe emu
build/emu.exe build/ref/left.rom notes.txt
```

You need `clang` and `llvm-nm` (LLVM), `tcc`, CMake and Ninja. On Windows, clang
also needs Visual Studio Build Tools with the Windows SDK. Everything else is
vendored, so building needs no network. See [`emu/README.md`](emu/README.md)
for the controls and the debugger.

### Assembling with talie

```
python utils/talie/talie.py input.tal output.rom
```

talie needs Python and the `rich` package. It uses the 2022 dialect of Uxntal
and matches the output of the May 2022 `uxnasm` byte for byte. The
[cheat sheet](docs/uxntal-cheatsheet.md#syntax) lists how the dialects differ.

### The other projects

Each project has its own `Makefile`. These need `make`, plus `uxnasm`, `uxncli`
and `uxnemu` from outside this repo (on Windows, in `c:\tools\uxn`). Projects
written in Forth also need Python, and the `w*` targets need `watchexec`.

## Licence

This repo's own code is under the [MIT licence](LICENSE). Third-party code keeps
its own licence: everything in `emu/vendor/` has a licence file alongside it,
and `langs/lisp/lisp.c` carries its licence in its header.
