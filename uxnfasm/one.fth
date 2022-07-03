\ no-stdlib
: init ( --> )

    variable name
    array points 5

    ;will name !
    name @ emit-string cr

    ;tim name !
    name @ emit-string cr
    cr

     1 points 0 + !
     2 points 2 + !
     3 points 4 + !
     4 points 6 + !
     5 points 8 + !

     print-points
     cr
     \ print-points


    cr
    debug

    cr
    halt

brk;

@will
"Will"
@tim
"Tim"

: print-points
     points 0 + ? cr
     points 2 + ? cr
     points 4 + ? cr
     points 6 + ? cr
     points 8 + ? cr
;

(
: emit-string ( addr -- )
    begin LDAk DUP while1
        emit
        INC2
    repeat
    POP
    POP2
;
)

