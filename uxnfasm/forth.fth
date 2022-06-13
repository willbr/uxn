: cr #0a emit ;
: spaces 0 do #14 emit loop ;

: .  print-i16 space ;
: u. print-u16 space ;

: abs #0000 OVR2 #8000 LTH2 JMP SWP2 SUB2 ;
: abs-alt dup -32768 = if 1- else abs endif ;
: abs-old dup negative? if negate endif ;

%print-char { ;print-byte/char JSR2 }

: print-byte ( byte -- )
	DUP #04 SFT ,&char JSR
	&char ( char -- ) #0f AND DUP #09 GTH #27 MUL ADD #30 ADD #18 DEO
;

: print-short ( short -- )
  SWP
  print-byte
  print-byte
;


: print-i16 ( i16 -- )
    DUP2 negative? if
        LIT '- emit
        abs
    endif
    print-u16
;


%print-uchar { DIV2k DUP2 NIP print-char MUL2 SUB2 }
: print-u16 ( u16 -- )
    10000 print-uchar
    1000  print-uchar
    100   print-uchar
    10    print-uchar
    NIP print-char
;

: umod DIV2k MUL2 SUB2 ;
: u/mod DIV2k STH2k MUL2 SUB2 STH2r ;

: max over over > if drop else nip endif ;
: min over over < if drop else nip endif ;

: /
    over over
    EOR2 negative?
    STH
    swap abs swap abs DIV2
    STHr
    if negate endif
    ;


: ,
    here @ !
    here @ 2+
    here !
;

: c,
    here @ c!
    here @ 1+
    here !
;

: type
    0 do
        LDAk emit
        INC2
    loop
    POP2
;

: page
    ( move cursor to top )
    csi
    LIT '0 emit
    LIT '; emit
    LIT '0 emit
    LIT 'H emit

    ( clear page )
    csi
    LIT '2 emit
    LIT 'J emit
;


: csi
    c_esc emit
    LIT '[ emit
;

: ? debug @ debug . ;
