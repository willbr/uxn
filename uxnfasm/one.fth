: init ( --> )
    variable t
    ;on-frame .Screen/vector DEO2
    ;on-key .Controller/vector DEO2


    ( set system colors )
    #a5ff .System/r DEO2
    #a50f .System/g DEO2
    #fe0f .System/b DEO2

    (
    255 0 do
        i dup . sin 127 +
        3 >>
        dup hline space
        . cr
    loop

    cr cr
    halt

    )
brk;

: u8>s16
    dup 0x7f > if
        0x100 swap -
        0 swap -
    endif
;

: hline
    0 do
        LIT '# emit
    loop
;

: on-frame ( --> )
    clear-foreground

    #41 ;s-circle .Mouse/x DEI2 .Mouse/y DEI2 spr

    #42 ;s-circle
    .Mouse/x DEI2 t @ s-wobble +
    .Mouse/y DEI2 t @ c-wobble +
    spr

    #43 ;s-circle
    .Mouse/x DEI2 t @ c-wobble 2 / +
    .Mouse/y DEI2 t @ s-wobble 2 / +
    spr

    #45 ;s-circle
    .Mouse/x DEI2 t @ 2 * c-wobble 2 * +
    .Mouse/y DEI2 t @ 2 * s-wobble 2 * +
    spr

    1 t +!
brk;

: mod
    umod
;

: sin-offset ( s16 -- u8 )
    256 mod
;

: sin ( n -- sin(n) )
    sin-offset
    ;sin-table
    +
    LDA #00 SWP
    u8>s16
;


: cos ( s16 -- cos(n) )
    64 + sin
;

: s-wobble
    sin 2 /
;

: c-wobble
    cos 2 /
;

: spr ( u8 addr x16 y16 -- )
    .Screen/y DEO2
    .Screen/x DEO2
    .Screen/addr DEO2
    .Screen/sprite DEO
;

: clear-foreground
    ;s-back-slash .Screen/addr DEO2
    #01 .Screen/auto DEO 

    #0000 .Screen/y DEO2

    .Screen/height DEI2 8 /
    0 do
        #0000 .Screen/x DEO2
        .Screen/width DEI2 8 /
        0 do
            #41 .Screen/sprite DEO
        loop
        .Screen/y DEI2 8 +
        .Screen/y DEO2
    loop
;

: clear-background
    ;s-forward-slash .Screen/addr DEO2
    #01 .Screen/auto DEO 

    #0000 .Screen/y DEO2

    .Screen/height DEI2 8 /
    0 do
        #0000 .Screen/x DEO2
        .Screen/width DEI2 8 /
        0 do
            #01 .Screen/sprite DEO
        loop
        .Screen/y DEI2 8 +
        .Screen/y DEO2
    loop
;

: on-key ( --> )
    .Controller/key DEI
    debug
    DUP c-esc EQU if1
        POP
        halt
    endif
    DUP k-c EQU if1
        POP
        clear-background
    endif
    POP
brk;

@s-clear
sprite-1bpp
........
........
........
........
........
........
........
........

@s-back-slash
sprite-1bpp
x.......
.x......
..x.....
...x....
....x...
.....x..
......x.
.......x

@s-forward-slash
sprite-1bpp
.......x
......x.
.....x..
....x...
...x....
..x.....
.x......
x.......

@s-circle
sprite-1bpp
..xxxx..
.xxxxxx.
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx
.xxxxxx.
..xxxx..

~fixed.fth
~sin-table.fth
