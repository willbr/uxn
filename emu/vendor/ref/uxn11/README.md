# Uxn11

A graphical emulator for the [Varvara Computer](https://wiki.xxiivv.com/site/varvara.html), written in ANSI C. The emulator contains a few linux specific utilities in the Console device to allow for it to interface with the unix systems.

## Building 

You must have X11 installed.

```sh
cc -DNDEBUG -O2 -g0 -s src/uxn11.c -lX11 -lutil -o bin/uxn11
```

For your convenience a [Makefile](https://en.wikipedia.org/wiki/Make_(software)#Makefile) is provided. 

```sh
make run
```

You can run `make install` to build and install the files. By default, files are installed into `~/.local` but this can be overridden using `PREFIX`:

```sh
# installs files into /opt/uxn/bin and /opt/uxn/share
$ make PREFIX=/opt/uxn install
```

If do not wish to build it yourself, you can download linux binaries for [uxn11](https://rabbits.srht.site/uxn11/bin/uxn11).

[![builds.sr.ht status](https://builds.sr.ht/~rabbits/uxn11.svg)](https://builds.sr.ht/~rabbits/uxn11?)

### For OpenBSD

To compile on OpenBSD, you need to install the "epoll-shim" package.

```sh
pkg_add epoll-shim
```

Then, to build:

```sh
cc -DNDEBUG -O2 -g0 -s `pkg-config --cflags --libs x11 epoll-shim` src/uxn11.c -lutil -o bin/uxn11
```

## Usage

The first parameter is the rom file, the subsequent arguments will be accessible to the rom, via the [Console vector](https://wiki.xxiivv.com/site/varvara.html#console).

```sh
bin/uxn11 bin/example.rom arg1 arg2
```

## Manual

A manual page is provided documenting the Uxntal language:

```sh
man ./doc/man/uxntal.7
```

After running `make install` the man page should be found by `man uxntal`.

## Assembler

This repository comes with a copy of the compiled [Drifblim](https://git.sr.ht/~rabbits/drifblim) assembler. 

```sh
cat etc/utils/drifblim.rom.txt | bin/uxn11 etc/utils/xh.rom > bin/drifblim.rom
bin/uxn11 bin/drifblim.rom etc/tests/opctest.tal bin/opctest.rom
```

## Devices

The file device is _not sandboxed_, meaning that it will be able to read or write outside of the working directory. Use carefully.

- `00` system
- `10` console(+)
- `20` screen
- `80` controller
- `90` mouse
- `a0` file(a)
- `b0` file(b)
- `c0` datetime

## Emulator Controls

- `F1` toggle zoom
- `F2` toggle debugger
- `F4` reboot
- `F5` reboot(soft)

### Buttons

- `LCTRL` A
- `LALT` B
- `LSHIFT` SEL 
- `HOME` START

## Need a hand?

The following resources are a good place to start:

* [XXIIVV — uxntal](https://wiki.xxiivv.com/site/uxntal.html)
* [XXIIVV — uxntal reference](https://wiki.xxiivv.com/site/uxntal_reference.html)
* [compudanzas — uxn tutorial](https://compudanzas.net/uxn_tutorial.html)

## Contributing

Submit patches using [`git send-email`](https://git-send-email.io/) to the [~rabbits/public-inbox mailing list](https://lists.sr.ht/~rabbits/public-inbox).
