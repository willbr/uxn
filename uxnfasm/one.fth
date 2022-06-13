\ this is my comment

\ : +!  LDA2k + swap ! drop ;

: init
    page
    ;number @ u. cr
    1 ;number +!
    ;number @ u. cr
    661 ;number -!
    ;number @ u. cr

    halt
brk;

@number
tal
1234
endtal

