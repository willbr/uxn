%negative-i12? { #07ff GTH2 }

: init
    \ 0xa NIP print-char
    10000 .
   cr
    halt
brk;

: print-fraction4
    #0000 >r
    dup #0008 and NIP if 5000 >r ADD2r endif
    dup #0004 and NIP if 2500 >r ADD2r endif
    dup #0002 and NIP if 1250 >r ADD2r endif
        #0001 and NIP if 0625 >r ADD2r endif
    r>
    u.  cr
;

: f.
    dup
    #04 SFT2 print-i12
    LIT '. emit
    #000f AND2
    print-fraction4
;

: abs-i12 #0000 OVR2 #0800 LTH2 JMP SWP2 SUB2 ;

: print-i12 ( i12 -- )
    DUP2 negative-i12? if
        LIT '- emit
        #1000 swap -
        \ abs-i12
    endif
    print-u16
;


: b. ( u16 -- )
    2 u/mod
    ?dup if b. endif
    NIP print-char
;

