\ this is my comment

%abs { dup negative? if negate endif }
%abs1 { dup negative? if negate endif }
%abs2 { #0000 OVR2 #0f EQU JMP SWP2 SUB2 }
%abs3 { #0000 OVR2 #8000 AND2 POP if SWP2 SUB2 else SUB2 endif }
%abs4 { #0000 OVR2 #8000 AND2 #8000 NEQ2 JMP SWP2 SUB2 }
%abs5 { #0000 OVR2 POP #80 AND #80 NEQ JMP SWP2 SUB2 }
%abs6 { OVR #80 OVR AND debug EQU debug }

: init
    (
    -255 abs1 . cr
     255 abs1 . cr
     )

    -32768
    debug
    OVR
    debug
    .  cr


    cr
    debug
    halt
brk;

~two.fth

@data
incbin test.bin

