: init ( --> )

    3

    dup case1
    \ dup case2
    \ dup case3

    drop
    debug

    cr
    halt

brk;

@one
"one"
@two
"two"
@other
"other"


: case1
    case
    1 of
        ;one
        endof
    2 of
        ;two
        endof
        ;other
    endcase
    emit-string cr
;

: case2
    dup 1 = if
        drop
        ;one
    else
        dup 2 = if
            drop
            ;two
        else
            drop
            ;other
        endif
    endif
    emit-string cr
;

: case3
    DUP2 #0001 NEQ2 ;&case1 JCN2
        POP2
        ;one
        ;&end JMP2
&case1
    DUP2 #0002 NEQ2 ;&case2 JCN2
        POP2
        ;two
        ;&end JMP2
&case2
    POP2
    ;other
&end

    emit-string cr
;

