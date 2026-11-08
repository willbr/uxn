# Uxnmin

A partial emulator for the [Varvara Ordinator](https://wiki.xxiivv.com/site/varvara.html), written in ANSI C. 

This emulator has a very limited of features, it has just enough device support as to be capable of assembling roms with [Drifloon](https://git.sr.ht/~rabbits/drifblim).

## Building 

If you wish to build the emulator:

```sh
tcc src/uxnmin.c -o bin/uxnmin
```

Tested with gcc, tcc and cproc.

## Usage

The first parameter is the rom file, the subsequent arguments will be accessible to the rom, via the [Console vector](https://wiki.xxiivv.com/site/varvara.html#console).

```sh
bin/uxnmin bin/console.rom arg1 arg2
```

## Contributing

Submit patches using [`git send-email`](https://git-send-email.io/) to the [~rabbits/public-inbox mailing list](https://lists.sr.ht/~rabbits/public-inbox).

