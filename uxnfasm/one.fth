\ https://solhsa.com/imgui/index.html


variable mouse.x
variable mouse.y
variable t

: init ( -> )
    1 if
        320 .Screen/width DEO2
        240 .Screen/height DEO2
    else
        640 .Screen/width DEO2
        480 .Screen/height DEO2
    endif

    ;on-frame .Screen/vector DEO2
    ;on-key .Controller/vector DEO2
    \ ;on-mouse .Mouse/vector DEO2


    ( set system colors )
    #a5ff .System/r DEO2
    #050f .System/g DEO2
    #fe0f .System/b DEO2
brk;

: on-frame ( -> )
    \ clear screen

    #02
    ;s-fill
    100
    100
    spr

    update-cursor

    t @ 1+ t !
brk;

: on-key ( -> )
    halt
    key
    debug
    dup k-esc = if
        drop
        halt
    endif
    drop
brk;

: on-mouse ( -> )
    \ debug
brk;

: update-cursor
    #40
    ;s-cursor
    mouse.x @
    mouse.y @
    spr2

    #41
    ;s-cursor
    .Mouse/x DEI2
    .Mouse/y DEI2

    over over
    mouse.y !
    mouse.x !

    spr2
;


: spr ( u8 addr x16 y16 -- )
    #00 .Screen/auto DEO

    .Screen/y DEO2
    .Screen/x DEO2
    .Screen/addr DEO2
    .Screen/sprite DEO
;

: spr2 ( u8 addr x16 y16 -- )
    #16 .Screen/auto DEO
    .Screen/y DEO2
    .Screen/x DEO2
    .Screen/addr DEO2
    .Screen/sprite
    DEOk DEO

;

@s-cursor
sprite-1bpp 16 16
x...............
xx..............
xxx.............
xxxx............
xxxxx...........
xxxxxx..........
xxxxxxx.........
xxxxxxxx........
xxxxxx..........
xxxxx...........
xxxxxx..........
xx..xx..........
x....xx.........
.....xx.........
......xx........
......xx........


@s-fill
sprite-1bpp 8 8
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx

@s-clear
sprite-1bpp 8 8
........
........
........
........
........
........
........
........

