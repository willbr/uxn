
: init
    ;on-frame .Screen/vector DEO2

      ( set system colors )
      #2ce9 .System/r DEO2
      #01c0 .System/g DEO2
      #2ce5 .System/b DEO2



brk;

: on-frame ( -> ) 
    clear

    .Mouse/x DEI2 .Screen/x DEO2
    .Mouse/y DEI2 .Screen/y DEO2

    ;s_circle .Screen/addr DEO2

    #41 .Screen/sprite DEO
;

: clear
    ;s_clear .Screen/addr DEO2
    #01 .Screen/auto DEO 

    #0000 .Screen/y DEO2

    10 0 do
        #0000 .Screen/x DEO2
        10 0 do
            #41 .Screen/sprite DEO
        loop
    .Screen/y DEI2
    8 +
    .Screen/y DEO2
    loop
;


@s_clear
sprite-1bpp
x.......
.x......
..x.....
...x....
....x...
.....x..
......x.
.......x

@s_circle
sprite-1bpp
..xxxx..
.xxxxxx.
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx
.xxxxxx.
..xxxx..
