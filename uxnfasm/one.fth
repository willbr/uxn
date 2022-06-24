no-stdlib
: init
     -1.5    f. cr
     -1.1250 f. cr
     -0.5    f. cr
     -0.1875 f. cr
     -0.1250 f. cr
     -0.0625 f. cr
     -0.0    f. cr
      0.0    f. cr
      0.0625 f. cr
      0.1250 f. cr
      0.1875 f. cr
      0.5    f. cr
      1.1250 f. cr
      1.5    f. cr

    halt

brk;


: f. ( n -- )
    dup
    #04 SFT2
    dup 0x7ff > if
        LIT '- emit
        #0fff swap -
        print-u16
        #000f and
        #0010 swap -
    else
        LIT '+ emit
        print-u16
        #000f and
    endif

    LIT '. emit
    \ cr debug
    dup 1 = if
        LIT '0 emit
    endif

    625 *
    print-u16
;


: print-u16 ( u16 -- )
    10 u/mod
    ?dup if print-u16 endif
    NIP print-char
;

: u/mod DIV2k STH2k MUL2 SUB2 STH2r ;
: ?dup dup 0 != if dup endif ;

: print-byte ( byte -- )
	DUP #04 SFT ,&char JSR
	&char ( char -- ) #0f AND DUP #09 GTH #27 MUL ADD #30 ADD #18 DEO
;

: b. ( u16 -- )
    2 u/mod
    ?dup if b. endif
    NIP print-char
;
\ ~sin-table.fth
