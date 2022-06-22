\ f. is broken
\ what is wrong?

no-stdlib
: init
    -0.0625 f. cr
    -1.1 f. cr
    -0.1 f. cr
    -0.0 f. cr
     0.0 f. cr
     0.1 f. cr
     1.1 f. cr
    \ 0b0111_1111_1111_1111 f. cr
    \ 0b1000_0000_0000_0000 f. cr
    \ 0b1111_1111_1111_1111 f. cr
    \ -1.1875 f. cr
    \ -0.1875 f. cr
    \ debug

    \ dup b. cr cr
    \ dup u. cr
    \ dup .  cr
    \ dup f. cr

    \ debug

    halt

brk;

(
def print_fixed_point(n):
    #eprint(f"{n=:016b}")
    #eprint(sign)
    i = n & 0xfff0
    i >>= 4
    #eprint(f"{i=} {i=:x}")
    if i >= 0x7ff:
        i -= 0x1000
        pass
    #eprint(f"{i=}")
    #eprint(f"{f=}")

    f = n & 0b0000_0000_0000_1111
    #eprint(f"{f=:016b}")
    f *= 625
    f /= 10000
    #eprint(f"{f=}")

    j = i + f

    s = f"{j}"

    eprint(s)
    eprint()
    )

: f.
    dup negative? if
        f.negative
    else
        f.positive
    endif
    625 *
    print-u16
;

: f.negative
    LIT '- emit
    dup #04 SFT2
    1 +
    #1000 swap -
    print-u16
    LIT '. emit
    #000f AND2
    #0010 swap -
;

: f.positive
    LIT '+ emit
    dup
    #04 SFT2 print-u16
    LIT '. emit
    #000f AND2
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

: b. ( u16 -- )
    2 u/mod
    ?dup if b. endif
    NIP print-char
;
\ ~sin-table.fth
