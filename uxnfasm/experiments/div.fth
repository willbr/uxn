\ this is my comment

\ : abs #0000 OVR2 #8000 LTH2 JMP SWP2 SUB2 ;

%divtest { div-alt3 . cr }

: init
    -32768 2 divtest
     32767 2 divtest
    -32767 2 divtest
     10000 2 divtest
    -10000 2 divtest
     0 2 divtest
    -0 2 divtest

    debug

    10000 2
    debug
    div-alt3
    debug

    dup
    u.  cr
    .  cr


    cr
    debug
    halt
brk;

: div-alt1
    over over
    abs swap abs swap /
    rot rot
    EOR2 #7fff GTH2
    if negate endif
    ;

: div-alt2
    over over
    EOR2 #7fff GTH2 STH
    abs swap abs swap /
    STHr
    if negate endif
;

: div-alt3
    ( setup negate JMP )
    #0000 ROT2 ROT2
    ( calc result sign )
    EOR2k #7fff GTH2 STH
    DIV2
    STHrk if #8000 SWP2 SUB2 endif
    ( use sign to negate )
    STHr JMP SWP2 SUB2
;
