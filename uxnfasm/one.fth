\ no-stdlib

: init
    1.0 4 << .  cr
    \ 1 sin f. cr
    halt
brk;

: sin ( n -- sin(n) )
    2 *
    ;sin-table
    +
    LDA2
;

~sin-table.fth
