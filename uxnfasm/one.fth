\ this is my comment

\ : abs #0000 OVR2 #8000 LTH2 JMP SWP2 SUB2 ;

: init
    ;here LDA2 u.
    ;here LDA2 LDA2 u.
    cr

    0xcafe ,

    ;here LDA2 u.
    ;here LDA2 LDA2 u.
    cr

    0xff c,

    ;here LDA2 u.
    ;here LDA2 LDA2 u.
    cr

    cr
    debug
    halt
brk;

: comma16
    here @ !
    here @ 2+
    here !
;

: comma8
    here @ c!
    here @ 1+
    here !
;

