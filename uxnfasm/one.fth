\ this is my comment

: init
    ;data
    10 0 do
        dup print-short space
        LDAk print-byte cr
        INC2
    loop
    drop

    debug
    halt
brk;

~two.fth

@data
tal
00 01 02 03 04 05 06 07 08 09
endtal

