: init ( --> )


    1
    case
    1 of
        ;&one
        endof
    2 of
        ;&two
        endof
        ;&other
    endcase
    emit-string cr

    debug


    cr
    halt

brk;

&one
"one"
&two
"two"
&other
"other"

