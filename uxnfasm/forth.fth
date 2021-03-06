: cr #0a emit1 ;
: spaces 0 do #14 emit1 loop ;

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
        LIT '- emit1
        abs
    endif
    print-u16
;

: print-u16 ( u16 -- )
    10 u/mod
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
        LDAk emit1
        INC2
    loop
    POP2
;

: page
    ( move cursor to top )
    csi
    LIT '0 emit1
    LIT '; emit1
    LIT '0 emit1
    LIT 'H emit1

    ( clear page )
    csi
    LIT '2 emit1
    LIT 'J emit1
;


: csi
    c-esc emit1
    LIT '[ emit1
;

: ? @ . ;

: +! LDA2k rot + swap ! ;
: -! LDA2k rot - swap ! ;

: emit-string ( addr -- )
    begin LDAk DUP while1
        emit1
        INC2
    repeat
    POP
    POP2
;

: ?dup dup 0 != if dup endif ;
: /mod over over / STH2k MUL2 SUB2 STH2r ;

: b. ( u16 -- )
    print-binary
    space
;

: print-binary ( u16 -- )
    2 u/mod
    ?dup if print-binary endif
    NIP print-char
;

: f. ( n -- )
    dup 0x7fff u> if
        LIT '- emit1
        0 swap -
    else
        LIT '+ emit1
    endif

    dup #04 SFT2
    print-u16
    #000f and

    LIT '. emit1
    dup 1 = if
        LIT '0 emit1
    endif

    625 *
    print-u16
    space
;

: print-i12 ( i12 -- )
    dup negative-i12? if
        LIT '- emit1
        #1000 swap -
    endif
    print-u16
;


: f>s
    #04 SFT2
    dup negative-i12? if
        #1000 swap -
        negate
    endif
;

: f>u #04 SFT2 ;
: s>f #40 SFT2 ;
: u>f #40 SFT2 ;

: h. print-short ;

: 2pick
    >r >r
    dup >r

    SWP2r
    r>
    SWP2r
    r>
    r>
;

: 3pick
    >r >r >r
    dup >r

    SWP2r
    r>
    SWP2r
    r>
    SWP2r
    r>
    r>
;

: s1 dup . cr ;
: s2
    dup .
    over .
    cr
;

: s3
    dup .
    over .
    2pick .
    cr
;

: s4
    dup .
    over .
    2pick .
    3pick .
    cr
;

: mod
    umod
;

: u8>s16
    dup 0x7f > if
        0x100 swap -
        0 swap -
    endif
;

: fill ( addr n c -- )
    >r
    0 do
        j over c!
        1 +
    loop
    POP2r
    POP2
;

: erase ( addr n -- )
    0 do
        0 over c!
        1 +
    loop
    POP2
;

~debug.fth
~ansi.fth
