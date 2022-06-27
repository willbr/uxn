\ no-stdlib

: init
    0
    201 0 do
        \ i u.
        \ dup f.
        dup fsin
        \ dup f.
        scale
        \ f. cr
        f>s hline
        0.0625 +
    loop
    drop

    cr cr cr

    halt
brk;

: scale
    5 <<
    1 5 << u>f +
;

: hline
    0 do
        LIT '# emit
        loop
    cr
;

~fixed.fth
~sin-table.fth

