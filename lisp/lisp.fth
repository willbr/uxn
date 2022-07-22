\ main
\ read
\ getobject
\ gettoken
\ getchar

%\n { #000a }
%'(' { #0028 }
%')' { #0029 }
%<= { > not }
%c-space { #0020 }

variable in :tib
variable t-in  0
variable t-out 0
variable dx    0
array tib 256
array token 64

: init
    ;prompt emit-string
    ;on-console .Console/vector DEO2
brk;

: on-console ( -> )
    \ in @ print-short space
    #00 .Console/read DEI
    dup in @ c!
    1 in +!
    \n = if
        tib   t-in  !
        token t-out !
        ;tib emit-string cr
        begin
        t-in @ c@
        0 != while
            get-token ;token emit-string cr
        repeat
        halt
    endif
brk;


: get-token ( -- c )
    token t-out !

    skip-whitespace

    begin
        t-in @ c@
        1 t-in +!

        dup t-out @ c!
        1   t-out +!

        case
        0 of false endof
        c-space of true endof
        '(' of true endof
        ')' of true endof
        otherwise
            t-in @ c@ \ next char
            ')' =
            if
                true
            else
                false
            endif
        endcase
    until
    0 t-out @ c!
;


: skip-whitespace ( -- )
    begin
        t-in @ c@
        c-space =
        while
            1 t-in +!
            \ debug
    again
;


@newline "newline"
@prompt ">>> "

