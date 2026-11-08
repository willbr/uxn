: init
    ;on-frame .Screen/vector DEO2
    ;on-key .Controller/vector DEO2


    ( set system colors )
    #a5ff .System/r DEO2
    #a50f .System/g DEO2
    #fe0f .System/b DEO2

    cr

    -0.5
    debug
    dup b. cr cr
    dup u. cr
    dup . cr
    dup f. cr

    #000f and

    dup #0000 = if
        LIT '0 emit
        drop
    else
        625 *
        debug
        _r-fraction2
    endif

    cr

    halt

brk;

: _r-fraction2
    10 u/mod
    ?dup if _r-fraction2 endif
    ?dup if NIP print-char endif
;

: on-frame ( -> ) 
    clear-foreground

    #41 ;s-circle .Mouse/x DEI2 .Mouse/y DEI2 spr

    #42 ;s-circle
    .Mouse/x DEI2 10 +
    .Mouse/y DEI2
    spr
brk;

: sin
    drop
    0
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

: on-key ( -> ) 
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

~sin-table.fth
