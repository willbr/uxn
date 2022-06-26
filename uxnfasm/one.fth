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

    \ 300.0 100.0 f/ f. cr
    \ 301.0 100.0 fmod f. cr
    11.125 31.125
    \ 346.2656

    \ how should f* work for 10 30 *
    over over
    4 >>
    *
    f. cr
    f. f.

    cr

    halt
brk;

: sin ( n -- sin(n) )
    2 *
    ;sin-table
    +
    LDA2
;

: fmod over over f/ f* swap - ;
\ : f* * #04 SFT2 ;
: f* * #04 SFT2 ;
: f/ / #40 SFT2 ;


~sin-table.fth
