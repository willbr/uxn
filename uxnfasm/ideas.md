# uxnfasm

## todo

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

