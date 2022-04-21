# ideas

## c ish syntax

    (n > 1) if {
        "hi" puts
    } else {
        "nope" puts
    }

into

    n 1 >
    ;true jcn
    @else
        "nope" ;puts jsr2
        ;endif jmp
    @true
        "hi" ;puts jsr2
    @endif

## loop

infinite loop

    loop {
        "beep" puts
    }

into

    @loop
        "beep" puts
    ;loop jmp

### named loops

    loop beeper {
        "beep" puts
        1 if {
            break
        } else {
            continue
        }
    }

## repeat block

    repeat 10 {
        0000 1111 2222 3333
    }

# 3-INSTRUCTION FORTH

A 3-INSTRUCTION FORTH FOR EMBEDDED SYSTEMS WORK
Illustrated on the Motorola MC68HC11 by Frank Sergeant Copyright 1991 Frank Sergeant 

    XC@   fetch byte
    XC!   store byte
    XCALL call sub

# Demos

    seven segment display https://twitter.com/lexaloffle/status/1418077387711913986
    dots3d.p8 https://twitter.com/lexaloffle/status/1343514251307012096
    isometric map
    mode7 mario kart
    random sprites https://twitter.com/lexaloffle/status/1129071867610853376
    skelly town https://twitter.com/lexaloffle/status/1159430133041655808

# Games

    Squidy
    tixy
    Racing game
    Tetris
    Pool
    platformer
    worms
    3d maze

# Library code


    text
        printf

    drawing
        line
        rect
        circle
        ovals
        tri
        textured tri
        camera
        scaling

    math
        sin cos tan
        hyplot
        fixed point
        SDF
        random
        raycasting

    game
        pico8 api
        pygame api
        love2d api

# Apps

    vi
    forth
    forth-ide?
    basic
    python
    lisp

# Requests

    extra palettes
    scrolling layers

# Make devices

    child Uxn
    network connection

    make my own screen device?
        3D display?
        vector display?

# uxngb classic & color

gameboy only has 16KiB of RAM, unless you have paged memory

    RAM reduced from 64KiB to 4KB?
    working stack
        data 254 btes
        error 1 byte
        pointer 1 byte
    working stack
        data 254 btes
        error 1 byte
        pointer 1 byte
    IO
        data 256 bytes

