
: fmod over over f/ f* - ;

: f*
    1 >>
    swap 1 >>
    *
    2 >>
;

: f/
    over over > if
        /
        4 <<
    else
        swap
        1 <<
        swap
        /
        3 <<
    endif
;

: sf1
    dup f.
    cr
;

: sf2
    dup f.
    over f.
    cr
;

: sf3
    dup f.
    over f.
    2pick f.
    cr
;

: sf4
    dup f.
    over f.
    2pick f.
    3pick f.
    cr
;

: fsin-offset ( fp -- u16 )
        dup 6.1875 > if
            6.25 fmod
        endif
        16.0 f*
        f>u
;

: fsin ( n -- sin(n) )
    fsin-offset
    2 *
    ;fsin-table
    +
    LDA2
;


