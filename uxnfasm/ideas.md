# uxnfasm

## todo

    ifnot ifnot1 endif
        wrapper for JCN2
    base
    allot
    fill ( addr n char -- )
    erase (addr n -- )
    recurse
    u.r
    s"
    z"
    ."
    variable
    case

## case

    10
    case
    1 of ."one" endof
    2 of ."two" endof
    (default)
    endcase


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

or

    10
    dup 1 if ."one" else
    dup 2 if ."two" else
    ( default )
    @endcase
    endif endif

