\ f. is broken
\ what is wrong?

no-stdlib
: init

    -0.5
    debug

    \ dup b. cr cr
    \ dup u. cr
    \ dup .  cr
    \ dup f. cr

    f.

    cr

    debug

    halt

brk;

: f.
    dup
    #04 SFT2 print-i12
    LIT '. emit
    #000f AND2
    print-fraction4
;

: print-i12 ( i12 -- )
    dup negative-i12? if
        LIT '- emit
        #1000 swap -
    endif
    print-u16
;


: print-fraction4
    dup #0000 = if
        LIT '0 emit
        drop
    else
        625 *
        _r-fraction
    endif
;

: _r-fraction
    10 u/mod
    ?dup if _r-fraction endif
    ?dup if NIP print-char endif
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

\ ~sin-table.fth
