# emu: a uxn/Varvara emulator in C89 with an SDL3 desktop host

Three layers, each depending only on the ones below it:

| Layer | Directory | What it is |
|---|---|---|
| CPU core | `core/` | `uxn.c` + `uxn.h`. Strict ISO C89, freestanding: no includes, no libc, no globals, no allocation. |
| Varvara devices | `varvara/` | System, Console, Screen, Controller, Mouse, File and DateTime devices, plus ROM loading (C89 plus stdio). |
| Hosts | `cli/`, `host/`, `debug/` | `uxncli` (headless), `emu` (SDL3 + Dear ImGui window with a debugger), and the C89 disassembler/debug session the debugger uses. |

The core is kept clean so it can be dropped into a DOS program or an embedded
device unchanged. The desktop host is a separate, modern layer. The target is
to run Left (the Varvara text editor) well.

The build is **offline**: SDL3, cimgui/Dear ImGui, nob.h and the reference uxn
tools and Left are all vendored under `vendor/`. A fresh clone needs no network.

## Build

Requirements (Windows): `clang`/`clang++`, `tcc` (only for `check-core`), and
CMake + Ninja (only to build SDL3). No network.

From a fresh clone:

```
cd emu
clang -o nob.exe nob.c
./nob.exe test
./nob.exe emu
```

`test` builds what it needs first (the reference tools via `ref`, when
`build/ref/` is not complete yet) and runs `check-core` along with the test
groups. `emu` builds SDL3 with CMake + Ninja the first time, which takes a
few minutes. `./nob.exe` with no argument builds nothing; it prints the usage
line and exits with an error. Targets (see `main` in `nob.c`):

| Target | Does |
|---|---|
| `ref` | Builds the vendored uxnmin and assembles Left into `build/ref/left.rom` |
| `check-core` | Compiles `core/uxn.c` strictly as C89 (clang and tcc) and checks it uses nothing from libc |
| `cli` | Builds `build/uxncli.exe` |
| `test` | Builds `ref` if needed, runs `check-core`, then builds and runs all test groups (core, varvara, varvara-cli ROM cross-checks against the references, debug, runner, keymap) |
| `sdl3` | Builds static SDL3 with CMake + Ninja |
| `imgui` | Builds cimgui/Dear ImGui objects |
| `smoke` | Builds `build/smoke.exe`, a minimal SDL3 + ImGui program |
| `emu` | Builds `build/emu.exe` (builds SDL3 and ImGui first) |

All output goes to `build/` (ignored by git).

## Run

```
build/emu.exe    [--root DIR] [--zoom 1|2|3] [--no-integer] rom.rom [args...]
build/uxncli.exe [--root DIR] [--max-steps N] [--frames N --dump out.ppm] rom.rom [args...]
```

`--root` confines the File device to a directory. `--frames N --dump out.ppm`
runs N screen frames headless and writes the screen as a PPM image.

The emu window is resizable and the screen scales to fit it. With integer
scaling on (the default) it uses whole-number scales for crisp pixels;
`--no-integer`, or Zoom > Integer scaling in the debugger's menu, lets it fill
the window instead. `--zoom` (default 2) sets the starting size and is lowered
until the window fits the display; a screen bigger than the display at 1x
shrinks to fit.

### Controller keys

Left Ctrl = A, Alt = B (Right Alt: see below), Shift = Select, Home = Start,
arrow keys = directions.
Holding an arrow key or Home repeats it, as uxn11 does on key autorepeat.

While Left Ctrl or Alt is held, a printable key also sends its character
with Shift applied, as uxn11 does: Ctrl+s sends `s`, Ctrl+Shift+V `V`,
Ctrl+/ `/`, Alt+a `a`. Tab, Return, Escape, Backspace and Delete pass through
to the ROM. Other characters come from the system's text input.

Right Alt is Alt (button B) only on a keyboard layout without AltGr. On a
layout with AltGr (UK, German, US-International...) Right Alt is AltGr: it
types characters such as `@` or `€` through text input and is never button B
or a chord. The layout is checked at start and when it changes.

### Debugger

**Right Ctrl** toggles the debugger; the key never reaches the ROM. The panels
show the disassembly, stacks, memory, and breakpoints. Shortcuts work only
while a debugger panel has focus (and you are not typing in the address box):
J step, L step over (both repeat while held), Ctrl+G continue, Ctrl+P pause. There are no function-key shortcuts.

With the debugger open the ROM keeps running and keeps the keyboard; the
mouse reaches it while the pointer is over the screen. Click a panel to give
it the keyboard (for the shortcuts and the address box), and click the screen
to give the keyboard back to the ROM. Over the screen the system pointer is
hidden, since ROMs draw their own.

### Trying it (Left)

```
./nob.exe ref
./nob.exe emu
build/emu.exe build/ref/left.rom notes.txt
```

Left's reset vector takes about 2.1 million steps, more than the host's
2,000,000-step limit per vector run, so it finishes on the first frame; the
arguments (`notes.txt`) are delivered after it. Reset and Reload in the
debugger restart the ROM with the same arguments; a ROM opened from the
dialog gets none.

Check that you can:

1. Open a file from Left's file browser.
2. Type text, including Tab.
3. Save, reopen, and see the change.
4. Browse up through `..`.
5. Toggle the debugger with Right Ctrl.

## Porting the core

Compile only `core/uxn.c`; include `core/uxn.h`. Supply 65536 bytes of RAM and
the two device callbacks. There is no init function: zero a `Uxn`, fill in the
fields, load the ROM at 0x0100 and call `uxn_eval`.

```c
#include "uxn.h"

static ux_u8 ram[65536];
static Uxn u;

static ux_u8 my_dei(Uxn *u, ux_u8 port) { return u->dev[port]; }
static void my_deo(Uxn *u, ux_u8 port)  { /* u->dev[port] already written */ }

int run(const ux_u8 *rom, unsigned n)
{
	unsigned i;
	for (i = 0; i < n; i++) ram[0x100 + i] = rom[i];
	/* u is zeroed (static) */
	u.ram = ram;
	u.dei = my_dei;
	u.deo = my_deo;
	/* 0 steps = unlimited; pass a budget to keep control in a main loop */
	return uxn_eval(&u, 0x100, 0);
}
```

`uxn_eval` returns `UXN_BRK`, `UXN_LIMIT` (budget used up; `u->pc` is where to
resume) or `UXN_FAULT` (a callback set `u->fault`). If the target compiler
needs it, predefine `UXN_PEEK(u, a)`, `UXN_POKE(u, a, v)` (e.g. for banked or
far memory) and `UXN_DIV(a, b)` (e.g. for a division that does not trap)
before including `uxn.h`; the defaults are plain array access and a divide that
yields 0 for a zero divisor. `./nob.exe check-core` checks the core stays clean.

## Known deviations from the references

- Sprite data reads wrap at the end of the 64K page instead of reading into bank 1.
- ROMs larger than 0xff00 (65280) bytes are rejected rather than truncated.
- Windows only for now; the build targets other platforms are left for later.

## Vendored versions

| Component | Version |
|---|---|
| SDL3 | 3.4.20 (release-3.4.20, static, zlib license) |
| Dear ImGui | 1.93.0 (commit 6d32174) |
| cimgui | commit 2a6a93f |
| nob.h | tsoding/nob.h 746f47d |
| uxnmin (reference) | 7ca94dd |
| uxn11 (reference) | 0be51cf |
| Left (reference) | bd24d85 |

Each has a `VERSION` file under `vendor/`.
