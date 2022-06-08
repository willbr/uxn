( large letter f )

%halt { #01 .System/state DEO  }
%emit  { .Console/write DEO }
%debug { cr #01 .System/debug DEO cr }
%cr { #0a emit }
%true { #01 }
%false { #00 }

%+ { ADD2 }
%- { SUB2 }
%* { MUL2 }
%/ { DIV2 }

: init
brk;

: .
    DUP2 negative? if
        LIT '- emit
        abs
    endif
    print-short
;


: abs
    0 SWP2 SUB2
;


: negative?
    #8000 AND2 ORA
;

: u.
    print-short
;


: print-byte ( byte -- )
	DUP #04 SFT ,&char JSR
	&char ( char -- ) #0f AND DUP #09 GTH #27 MUL ADD #30 ADD #18 DEO
;


: print-short ( short -- )
  SWP
  print-byte
  print-byte
;

