\ this is my comment

: init
on-frame
    ;on-frame .Screen/vector DEO2
    ;on-key .Controller/vector DEO2

    #2f00 .System/r DEO2
    #20f0 .System/g DEO2
    #200f .System/b DEO2

brk;

: on-frame ( -> ) 
    .Mouse/x DEI2 .Screen/x DEO2
    .Mouse/y DEI2 .Screen/y DEO2

    ;sprite .Screen/addr DEO2
    #01 .Screen/sprite DEO
;

: on-key ( -> ) 
    halt
;

@sprite
sprite-1bpp
..xxxx..
.xxxxxx.
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx
.xxxxxx.
..xxxx..
