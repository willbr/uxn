: cr #0a emit ;
: spaces 0 do #14 emit loop ;

: .
    DUP2 negative? if
        LIT '- emit
        abs
    endif
    print-short
;


: abs 0 SWP2 SUB2 ;


: negative?
    #8000 AND2 ORA
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

