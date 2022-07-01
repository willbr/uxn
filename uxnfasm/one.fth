: init ( --> )

    (
    10
    case
    1 of ."one" endof
    2 of ."two" endof
    ( default )
    endcase
    )

    1
    case
    1 of
        ;&one emit-string cr
        endof
    2 of
        ;&two emit-string cr
        endof
        ;&other emit-string cr
    endcase



    cr
    halt

brk;

&one
"one"
&two
"two"
&other
"other"

(
into

    10
    dup 1 = if ."one" ;endcase JMP2 endif
    dup 2 = if ."two" ;endcase JMP2 endif
    ( default )
    @endcase

or

    10
    dup 1 NEQ2 ;&case2 JCN2
        ."one"
        ;&endcase JMP2
    &case2
    dup 2 NEQ2 ;&case3 JCN2
        ."two"
        ;&endcase JMP2
    ;&case3
        ( default )
    &endcase
)
