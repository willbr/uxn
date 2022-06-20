: init
    ;&str1 emit-str cr
    ;&str2 emit-str cr
    halt
brk;

&str1
"hello \"big\" world"
&str2
"goodbye world"

: emit-str
    begin LDAk DUP while
        emit
        INC2
    repeat
    POP
    POP2
;
