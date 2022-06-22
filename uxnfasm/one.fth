: init
    ;on-frame .Screen/vector DEO2
    ;on-key .Controller/vector DEO2


    ( set system colors )
    #a5ff .System/r DEO2
    #a50f .System/g DEO2
    #fe0f .System/b DEO2

brk;

: on-frame ( -> ) 
    clear-foreground

    .Mouse/x DEI2 .Screen/x DEO2
    .Mouse/y DEI2 .Screen/y DEO2

    ;s-circle .Screen/addr DEO2

    #41 .Screen/sprite DEO
brk;

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

