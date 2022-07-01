# ideas

# 3-INSTRUCTION FORTH

A 3-INSTRUCTION FORTH FOR EMBEDDED SYSTEMS WORK

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

# simple uxnfasm

    numbers default to literals
    numbers default to short?
    raw raw-byte

    decial hex
    mode-byte
    mode-short

    if else elif endif
    do loop +loop
    leave
    begin again
    begin untl
    begin while repeat

    \ line comments
    ( comments )

    incbin
    variable
    sprite-1bpp

    tal endtal

uxnfasm

    try:
        n = self.parse_number(w)
        self.print(f"#{n:04x}")
    except ValueError:
        self.print(f';{w} JSR2')

numbers

    decial 12345
    hex 0xffff
    fixed 123.563

# editor that traces and adds stack comments

type

    : square ( c -- c^2 ) dup * ;

into

    : square ( c -- c^2 )
        dup ( c c )
        *   ( m )
    ;

