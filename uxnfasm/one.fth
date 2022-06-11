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
incbin test.bin

