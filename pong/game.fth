variable pad.x 52
variable pad.y 248
variable pad.w 4
variable pad.h 4

variable ball.x 124
variable ball.y 124
variable ball.dx 4
variable ball.dy -2

%button { #00 .Controller/button DEI }

: init
    ;on-frame .Screen/vector DEO2
    ;on-key .Controller/vector DEO2

    256 .Screen/width DEO2
    256 .Screen/height DEO2

    ( set system colors )
    #a5ff .System/r DEO2
    #a50f .System/g DEO2
    #fe0f .System/b DEO2
brk;


: button-left ( -- n )
    button 0x40 and
;


: button-right ( -- n )
    button 0x80 and
;

: move-ball
    ball.dx @ ball.x +!
    ball.dy @ ball.y +!
;

: move-paddle
    button-left if
        3 pad.x -!
    else
    button-right if
        3 pad.x +!
    endif endif
;


: update ( -- )
    move-paddle
    move-ball
;


: clear-sprites
    \ #00 .Screen/auto DEO
    #41 ;s-clear ball.x @ ball.y @ spr ( ball )

    #40 draw-paddle
;


: draw-sprites
    #41 ;s-circle ball.x @ ball.y @ spr ( ball )
    #41 draw-paddle
;

: draw-paddle ( n -- )
    ;s-square pad.x @ pad.y @
    .Screen/y DEO2
    .Screen/x DEO2

    .Screen/addr DEO2

    #32 .Screen/auto DEO

    .Screen/sprite DEO
;


: on-frame ( -> )
    clear-sprites
    update
    draw-sprites
brk;


: on-key ( -> ) 
    key
    k-esc = if
        halt
    endif
brk;


: spr ( u8 addr x16 y16 -- )
    #00 .Screen/auto DEO
    .Screen/y DEO2
    .Screen/x DEO2
    .Screen/addr DEO2
    .Screen/sprite DEO
;


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


@s-back-slash
sprite-1bpp 8 8
x.......
.x......
..x.....
...x....
....x...
.....x..
......x.
.......x


@s-forward-slash
sprite-1bpp 8 8
.......x
......x.
.....x..
....x...
...x....
..x.....
.x......
x.......


@s-circle
sprite-1bpp 8 8
..xxxx..
.xxxxxx.
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx
.xxxxxx.
..xxxx..


@s-square
sprite-1bpp 8 8
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx

