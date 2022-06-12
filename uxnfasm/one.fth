\ this is my comment

\ : abs #0000 OVR2 #8000 LTH2 JMP SWP2 SUB2 ;

%divtest { / . cr }

: init
    -32768 2 divtest
     32767 2 divtest
    -32767 2 divtest
     10000 2 divtest
    -10000 2 divtest
     0 2 divtest
    -0 2 divtest

    debug

    cr
    debug
    halt
brk;

