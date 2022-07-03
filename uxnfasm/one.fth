\ no-stdlib
: init ( --> )

    variable name
    array points 3

    ;will name !
    name @ emit-string cr

    ;tim name !
    name @ emit-string cr
    cr

     points 6 0xff fill
     print-points cr

     1 points 0 + !
     2 points 2 + !
     3 points 4 + !

     print-points cr

     points 6 erase
     print-points cr

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
;

