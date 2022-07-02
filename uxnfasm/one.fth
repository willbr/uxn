no-stdlib
: init ( --> )

    3
    dup case1
    dup case2
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
    otherwise
        ;other
    endcase
    emit-string cr
;

: case2
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

: emit-string ( addr -- )
    begin LDAk DUP while1
        emit
        INC2
    repeat
    POP
    POP2
;
