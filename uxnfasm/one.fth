( large letter f )

%emit  { .Console/write DEO }
%cr    { #0a emit }
%halt  { #01 .System/state DEO  }
%debug { cr #01 .System/debug DEO cr }
%true  { #01 }
%false { #00 }

%+ { ADD2 }
%- { SUB2 }
%* { MUL2 }
%/ { DIV2 }

%=  { EQU2 }
%!= { NEQ2 }
%>  { GTH2 }
%<  { LTH2 }

%0= { #0000 EQU2 }
%0> { #0000 GTH2 }
%0< { #0000 LTH2 }

%not { #0000 EQU2 }

%1+ { INC2 }
%1- { #0001 SUB2 }
%2+ { #0002 ADD2 }
%2- { #0002 SUB2 }
%2* { #10 SFT2 }
%2/ { #01 SFT2 }

%i { STH2rk }

: init
    1 1+ .
    halt
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

