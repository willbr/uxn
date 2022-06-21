: cr #0a emit ;
: spaces 0 do #14 emit loop ;

: .  print-i16 space ;
: u. print-u16 space ;

: abs #0000 OVR2 #8000 LTH2 JMP SWP2 SUB2 ;
: abs-alt dup -32768 = if 1- else abs endif ;
: abs-old dup negative? if negate endif ;

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

: print-u16 ( u16 -- )
    10 /mod
    ?dup if print-u16 endif
    NIP print-char
;



: umod DIV2k MUL2 SUB2 ;
: u/mod DIV2k STH2k MUL2 SUB2 STH2r ;

: max over over > if drop else nip endif ;
: min over over < if drop else nip endif ;

: /
    over over
    EOR2 negative?
    STH2
    swap abs swap abs DIV2
    STH2r
    if negate endif
;


: ,
    here @ !
    2 here +!
;

: c,
    here @ c!
    1 here +!
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

: ? @ . ;

: +! LDA2k rot + swap ! ;
: -! LDA2k rot - swap ! ;

: emit-str ( addr -- )
    begin LDAk DUP while
        emit
        INC2
    repeat
    POP
    POP2
;

: ?dup dup 0 != if dup endif ;
: /mod over over / STH2k MUL2 SUB2 STH2r ;

: b. ( u16 -- )
    2 u/mod
    ?dup if b. endif
    NIP print-char
;

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
    else
        625 *
        _r-fraction
    endif
;

: _r-fraction
    10 /mod
    ?dup if _r-fraction endif
    ?dup if NIP print-char endif
;

: f* * #04 SFT2 ;
