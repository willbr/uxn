%negative-i12? { #07ff GTH2 }

: init
   0.0625 f.
   0.125  f.
   0.25   f.
   0.5    f.
   0.9375 f.
   2.5 0.5 +
   f.
   \ print-fraction4
(
   0.9375
   dup b. cr
   f. cr
   )

   (
   -1.0    b.  cr
    0.0    b.  cr
    1.0    b.  cr
    1.5    b.  cr
    cr
    1.5    b.  cr
    2.25   b. cr
    3.125  b. cr
    4.0625 b. cr
    cr
    25.0   b. cr
    0b0111_1111_1111_1111   b. cr
    )
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


: print-binary-byte ( byte -- )
    DUP #04 SFT ,&char JSR
    LIT '_ emit
    &char ( char -- )
    DUP DUP2
    #08 AND #00 NEQ LIT '0 ADD #18 DEO
    #04 AND #00 NEQ LIT '0 ADD #18 DEO
    #02 AND #00 NEQ LIT '0 ADD #18 DEO
    #01 AND #00 NEQ LIT '0 ADD #18 DEO
;

: b. ( short -- )
  SWP
  LIT '0 emit
  LIT 'b emit
  print-binary-byte
  LIT '_ emit
  print-binary-byte
;
