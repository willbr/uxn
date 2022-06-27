\ no-stdlib

\ fix f*
\ fix f/
\ fix fmod

%max_f { #7fff }
%min_f { #8000 }
%pie  { 3.125 }
%2pie { 6.25  }

: init
    0
    201 0 do
        i u.
        dup f.
        \ dup sin-offset u.
        dup sin f.
        cr
        0.0625 +
    loop
    drop

    cr cr

    6.25 fmod


    f. cr
    \ 3.14 sin f. cr
    \ 6.25 sin f. cr

    cr

    halt
brk;

: sin-offset ( fp -- u16 )
        dup 6.25 > if
            6.25 fmod
        endif
        16.0 f*
        f>u
;

: sin ( n -- sin(n) )
    sin-offset
    2 *
    ;sin-table
    +
    LDA2
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
