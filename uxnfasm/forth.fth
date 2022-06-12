: cr #0a emit ;
: spaces 0 do #14 emit loop ;

: .
    space
    print-i16
;


: abs #0000 OVR2 #8000 LTH2 JMP SWP2 SUB2 ;
: abs-alt dup -32768 = if 1- else abs endif ;
: abs-old dup negative? if negate endif ;


: negative?
    #7fff GTH2
;

: u.  print-short ;

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

: mod DIV2k MUL2 SUB2 ;
: /mod DIV2k STH2k MUL2 SUB2 STH2r ;

: max over over > if drop else nip endif ;
: min over over < if drop else nip endif ;

