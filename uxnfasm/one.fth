\ no-stdlib

\ fix f*
\ fix f/
\ fix fmod

%max_f { #7fff }
%min_f { #8000 }
%pie  { 3.125 }
%2pie { 6.25  }

: init
    (
    -6.25
    201 0 do
        i . dup f. dup u. cr
        0.0625 +
    loop
    drop
    )

    (
    0
    101 0 do
        i u. dup f. dup sin f. cr
        0.0625 +
    loop
    drop
    )

    cr cr cr cr

    0 0 0

    300.0 100.0 f/ f. cr
    301.0 100.0 fmod f. cr

    cr

    halt
brk;


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

: 2pick
    >r >r
    dup >r

    SWP2r
    r>
    SWP2r
    r>
    r>
;

: 3pick
    >r >r >r
    dup >r

    SWP2r
    r>
    SWP2r
    r>
    SWP2r
    r>
    r>
;

: sin ( n -- sin(n) )
    2 *
    ;sin-table
    +
    LDA2
;

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

~sin-table.fth
