\ this is my comment

%abs1 { dup negative? if negate endif }
%abs2 { #0000 OVR2 #0f EQU JMP SWP2 SUB2 }
%abs3 { #0000 OVR2 #8000 AND2 POP if SWP2 SUB2 else SUB2 endif }
%abs4 { #0000 OVR2 #8000 AND2 #8000 NEQ2 JMP SWP2 SUB2 }
%abs5 { #0000 OVR2 POP #80 AND #80 NEQ JMP SWP2 SUB2 }
%abs6 { #0000 OVR2 #8000 LTH2 JMP SWP2 SUB2 }
%abs7 { dup -32768 = if 1- else abs endif }

%negate1 { #0000 SWP2 SUB2 }
%negate2 { dup -32768 = if 1- else #0000 SWP2 SUB2 endif }
%absfn { negate2 }

: init
    -255   absfn . cr
     255   absfn . cr
     1     absfn . cr
     0     absfn . cr
    -1     absfn . cr
    -32768 absfn . cr
     32767 absfn . cr

    cr
    debug
    halt
brk;


