: init
    ;&str1 emit-str cr
    10 0 do
        ;&str2 emit-str cr
    loop
    1 if
        1 POP
    endif
    ;&str3 emit-str cr
    halt
brk;

&str1 "hello \"big\" world"
&str2 "beep"
&str3 "goodbye world"

: emit-str
    begin LDAk DUP while
        emit
        INC2
    repeat
    POP
    POP2
;

