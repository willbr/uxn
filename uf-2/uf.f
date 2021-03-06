: literal  160 c, , ;
also compiler definitions
: drop  34 c, ; immediate
: nip  35 c, ; immediate
: swap  36 c, ; immediate
: rot  37 c, ; immediate
: -rot  37 c,  37 c, ; immediate
: dup  38 c, ; immediate
: over  39 c, ; immediate
: 2dup  39 c,  39 c, ; immediate
: 2drop  34 c, 34 c, ; immediate
: +  56 c, ; immediate
: -  57 c, ; immediate
: *  58 c, ; immediate
: u/  59 c, ; immediate
: and  60 c, ; immediate
: or  61 c, ; immediate
: xor  62 c, ; immediate
: =  40 c, 6 c, ; immediate
: <>  41 c, 6 c, ; immediate
: u>  42 c, 6 c, ; immediate
: u<  43 c, 6 c, ; immediate
: 0<  128 c, 15 c, 63 c, ; immediate
: >r  47 c, ; immediate
: dup>r  175 c, ; immediate
: r>  111 c, ; immediate
: r>drop  98 c, ; immediate
: r@  239 c, ; immediate
: 1+  33 c, ; immediate
: cell+  33 c, 33 c, ; immediate
: 1-  160 c, 1 , 57 c, ; immediate
: exit  108 c, ; immediate
: @  52 c, ; immediate
: !  53 c, ; immediate
: c@  20 c, 128 c, 0 c, 4 c, ; immediate
: c!  21 c, 2 c, ; immediate
: 2*  128 c, 16 c, 63 c, ; immediate
: cells  128 c, 16 c, 63 c, ; immediate
: 2/  128 c, 1 c, 63 c, ; immediate
: 2>r  36 c,  47 c,  47 c, ; immediate
: 2r>  111 c,  111 c,  36 c, ; immediate
: execute  46 c, ; immediate
: brk  0 c, ; immediate
: [_']  ' literal ; immediate
: [']  vocs @ >r  null vocs !  ' literal  r> vocs ! ; immediate

only definitions
: on  -1 swap ! ;
: off  0 swap ! ;
: +  + ;
: -  - ;
: *  * ;
: u/  u/ ;
: u<  u< ;
: u>  u> ;
: r>  r> ;
: >r  >r ;
: dup>r  dup>r ;
: r>drop  r>drop ;
: r@  r@ ;
: c!  c! ;
: c@  c@ ;
: ! ! ;
: @ @ ;
: and  and ;
: or  or ;
: xor  xor ;
: =  = ;
: <>  <> ;
: >=  swap < ;
: <=  swap > ;
: drop  drop ;
: nip  nip ;
: dup  dup ;
: swap  swap ;
: rot  rot ;
: over  over ;
: invert  -1 xor ;
: negate  0 swap - ;
: 1+  1+ ;
: 1-  1- ;
: 2*  2* ;
: 2/  2/ ;
: 0=  0 = ;
: 0<  0< ;
: 2r>  2r> ;
: 2>r  2>r ;
: 2r@  r> 2r> over over 2>r >r ;
: here  h @ ;
: forth  dp vocs ! ;
: ]  compiler  (compile) ;
: \  >limit @ >in ! ; immediate
: -rot  -rot ;
: 2drop  2drop ;
: 2dup  2dup ;
: 2nip  rot drop rot drop ;                  
: 2swap  rot >r rot r> ;                 
: 2over  >r >r 2dup r> -rot r> -rot ;
: 2rot  >r >r 2swap r> r> 2swap ;
: (  41 parse 2drop ; immediate
: char  32 word 1+ c@ ;
: brk  brk ;
: sliteral  ['] (slit) compile,  tuck here place  1+ allot ;
: unloop  r>  r>drop r>drop  >r ;
: execute  r>drop  execute ;
: cells  cells ;
: cell+  cell+ ;
: cr  10 emit ;

also compiler definitions
: [char]  char literal ; immediate
: s"  ( | ..." -- a u ) [char] " parse sliteral ; immediate
: ."  ( | ..." -- ) 
  [char] " parse sliteral  ['] type compile, ; immediate
: [  state off  forth ; immediate
: if  ['] (if) compile,  here  0 , ; immediate
: else  ['] (else) compile,  here  0 ,  swap here swap ! ; immediate
: then  here swap ! ; immediate
: cjump,  jumpaddr,  if  45  else  13  then  c, ;
: jump,  jumpaddr,  if  44  else  12  then  c, ;
: begin  here ; immediate
: again  jump, ; immediate
: until  ['] (if) compile,  , ; immediate
: while  ['] (if) compile,  here  0 , ; immediate
: repeat  swap jump,  here swap ! ; immediate
: (?abort)  ( f a u -- ) rot  if  type  cr  abort  else  2drop  then ;
: abort"  [char] " parse sliteral ['] (?abort) compile, ; immediate
: |  [_'] exit execute  here swap ! ; immediate
: ->  [_'] over execute  [_'] = execute  ['] (if) compile,  here 0 , 
  [_'] drop execute ; immediate
: postpone  also forth
  32 word find  previous  0  ->  undefd  |
  1  ->  compile,  |  drop  literal ['] compile, compile, ; 
  immediate
: (does>)  r>  current @ @ count 63 and + 3 + ! ;
: does>  ['] (does>) compile,  111 c, ; immediate
: do  36 c, 47 c, 47 c,  0  here ; immediate
: ?do  36 c, 47 c, 47 c,  0 literal  0 literal  
  here 2 -  44 c,  here ; immediate
: patchloop  swap ?dup  if  here swap !  then ;
: +loop  patchloop  ['] (loop) compile,  cjump, ; immediate
: loop  1 literal  patchloop  ['] (loop) compile,  cjump, ; 
  immediate

only definitions
: constant  head ['] (constant) compile, , ;
: variable  head ['] (variable) compile, 0 , ;
: create  head ['] (variable) compile, ;
: buffer:  create  allot ;
0 constant false
-1 constant true
32 constant bl
: "  ( | ..." -- a u ) 
  [char] " parse >r  here r@ cmove  here r> dup allot ;
: under+  ( n1 x n2 -- n3 x ) rot + swap ;
: th  ( a1 u -- a2 ) 2* + ;
: min  2dup <  if  drop  else  nip  then ;
: max  2dup >  if  drop  else  nip  then ;
: pad  here 256 + ;
256 buffer: fnbuf
: filename  ( a u -- ) 255 min fnbuf place  0 fnbuf dup c@ + 1+ c! 
  fnbuf 1+ 168 deo2 ;
: filewrite  ( a u -- u2 ) 0 167 deo  170 deo2  174 deo2 162 dei2 ;
: fileappend  ( a u -- u2 ) 1 167 deo  170 deo2  174 deo2 162 dei2 ;
: fileread  ( a u -- u2 ) 170 deo2  172 deo2  162 dei2 ;
: filedelete  166 deo ;
: saved  ( a1 u1 a3 u2 -- )  
  filename  filewrite
  0= abort" saving file failed" ;
: save  256 here 256 -  bl word count saved ;
: crash  ." uninitialized execution vector" cr  abort ;
: defer  head ['] crash literal  44 c, ;
: defer!  1+ ! ;
: defer@  1+ @ ;
: is  ' defer! ;
defer bye  
: (bye)  1 15 deo  brk ;
' (bye) is bye
variable >voc  ' cdp 4 + >voc !
: vocabulary  create  >voc @  here >voc !  0 , current @ @ ,  ,  
  does>  vocs ! ;
: hex  16 base ! ;
: decimal  10 base ! ;
: +!  dup>r @ + r> ! ;
: @+  dup @ >r 2 + r> ;
: !+  over ! 2 + ;
: space  bl emit ;
: emits  begin  ?dup  while  over emit  1-  repeat  drop ;
: spaces  bl swap emits ;
: erase  0 fill ;
: blank  bl fill ;
: 0<>  if  -1  else  0  then ;
: bounds  ( a1 n -- a2 a1 ) over + swap ;

2 constant version

variable >num
: <#  pad >num ! ;
: #  base @ u/mod swap dup 9 u>  if  
  [char] a + 10 -  else  [char] 0 +  then  >num @ 1- dup >num ! c! ;
: #s  begin  # dup  while  repeat ;
: #>  ( u1 -- a n ) drop >num @ dup pad swap - ;
: hold  ( c -- ) >num @ 1- dup>r c! r> >num ! ;
: holds  ( a u -- ) dup>r  negate >num +!  >num @ r> cmove ;
: sign  ( n -- ) 0<  if  [char] - hold  then ;
: (u.)  ( u1 -- a u2 ) <# #s #> ;
: u.  ( u -- ) (u.) type space ;
: (.)  ( n1 -- a n2 ) dup abs <# #s swap sign #> ;
: .  ( n -- ) (.) type space ;
: h.  base @ >r  hex  u.  r> base ! ;
: u.r  ( n1 n2 -- ) >r <# #s #> r> over - 0 max spaces type ;
: .r  >r dup abs <# #s swap sign #> r> over - 0 max spaces type ;
: .s  depth ?dup 0=  if  ." stack empty "  |  
  dup 0  do  dup i - pick .  loop  drop ;

: ?exit  if r>drop  then ;
: .(  [char] ) parse type ;
: -trailing  begin  1- dup 0<  if  1+  |
  2dup + c@  bl <>  until  1+ ;
: clamp  ( n min max -- n2 ) rot max min ;
: 2@  dup cell+ @ swap @ ;
: 2!  swap over ! cell+ ! ;
: loadrom  ( a u -- ) filename (loadrom) ;
: :noname  also compiler  here (compile) ;
: aligned  ( n1 -- n2 ) dup 1 and  if  1+  then ;
: align  here aligned h ! ;
: diff  swap - ;
variable /search
: search  ( a1 u1 a2 u2 -- a3 u3 f )
  /search !  swap dup>r /search @ - 1+  0  do
    over i + over /search @ swap /search @ compare 0=  if
      drop i +  i  unloop  r> swap -  true  |  loop  drop  r>  false ;
: scan  ( a1 n1 c -- a2 n2 )
  >r  begin  dup  while  over c@ r@ =  if  r>drop  |
    1 /string  repeat  r>drop ;

also compiler definitions
: is  ' literal  ['] defer! compile, ; immediate
: recurse  current @ @ count + 2 + compile, ; immediate

only definitions
' (prompt) is prompt
.( saving uf0.rom ... ) cr
save uf0.rom
' noop is prompt

: ?  @ . ;
: year  192 dei2 ;
: month  194 dei ;
: day  195 dei ;
: hour  196 dei ;
: minute  197 dei ;
: second  198 dei ;
: dotw  199 dei ;
: doty  200 dei2 ;
: isdst  202 dei ;
: colors  12 deo2  10 deo2  8 deo2 ;
: svector  32 deo2 ;
: screensize@  34 dei2  36 dei2 ;
: screensize!  swap 34 deo2  36 deo2 ;
: position  42 deo2  40 deo2 ;
: pixel  46 deo ;
: auto  38 deo ;
: spritedata  44 deo2 ;
: sprite  47 deo ;
: sample  60 deo2 ;
: play  63 deo ;
: adsr  56 deo2 ;
: volume  62 deo ;
: output  52 dei ;
: cvector  ( xt -- ) 16 deo2 ;
: jbutton  ( -- b ) 130 dei ;
: jkey  ( -- k ) 131 dei ;
: jvector  ( xt -- ) 128 deo2 ;
: mouse  146 dei2  148 dei2 ;
: mscroll  154 dei2  156 dei2 ;
: mstate  150 dei ;
: mvector  ( xt -- ) 144 deo2 ;
: failing  ." machine error" cr  abort ;
defer tick  ' noop is tick
: waiting  tick  brk ;
: wait  r>drop  ['] waiting svector  brk ;
: pausing  ['] failing svector ;
: pause  ['] pausing svector  brk ;

: based  base !  bl word number r> r> base ! >r ?exit
  count type  ."  bad number"  cr  abort ;
: h#  base @ >r 16 based ;
: d#  base @ >r 10 based ;
also compiler definitions
: h#  h# literal ; immediate
: d#  d# literal ; immediate
only definitions
: heaptop  uxn  if  h# ea00  else  h# ec40  then ;
: unused  heaptop here - ;

: apply-theme  ( a -- ) @+ swap @+ swap @+ nip colors ;
: theme  s" .theme" filename  pad 6 fileread  if
    pad apply-theme  then ;
: snarf  ( a u -- ) s" .snarf" filename filewrite drop ;  
: yank  ( -- a u ) s" .snarf" filename
  pad  unused 4000 min  fileread  pad swap ;

\ dump
: dumpascii  ( a u -- ) space
  0 do  count dup 33 128 within 0=  if  drop  [char] .  then
    emit  loop  drop ; 
: dumpbyte  ( c -- ) dup 16 <  if   [char] 0  emit  then  
  base @ >r  hex u.   r> base ! ;
: dumprow   ( a u -- a )
  over u. space  0  do  dup i + c@ dumpbyte  loop ;
: dumprest  ( a u -- ) dup>r  dumprow  8 r@ - 3 * spaces
  dup r> dumpascii ;
: dump  ( a u -- )  8 u/mod swap >r  0  ?do
    8 dumprow  dup 8 dumpascii  cr  8 +  loop
  r>  ?dup  if  dumprest then  drop  cr ;

: order  4 0 do  
  vocs i th @ cell+ @ ?dup  if  count type  space  then  loop 
  current @ cell+ @ ?dup if  [char] ( emit  count type  ." ) "  then ;
: .vocs  >voc @  begin  ?dup  while  cell+ @+ count type space  @  
  repeat ;
: words  4 0  do  vocs i th @ @
    begin  ?dup  while
      count h# 3f and 2dup type space  + @  repeat
  loop ;
: marker-save
  >voc @ ,  current @ ,
  vocs  here  4 cells cmove  4 cells allot
  >voc @  begin  ?dup  while
    @+ ,  cell+ @  repeat  here cell+ , ;
: marker-restore  ( a -- )
  @+ >voc !  @+ current !
  dup vocs 4 cells cmove  4 cells +
  >voc @  begin  ?dup  while
    over @ over !  2 under+  2 cells + @  repeat
  @ h ! ;
: marker  ( | <word> -- )
  create  marker-save  does>  marker-restore ;

\ interpreter conditionals
: processword  ( n1 a n2 -- n3 )
  2dup s" [if]" compare 0=  if  2drop 1+ |
  2dup s" [else]" compare 0=  if  2drop dup 1 =  if  1-  then |
  s" [then]" compare 0=  if  1-  then ;
: skipwords  ( | ... -- )
  1  begin  bl word dup c@  0=  if  drop  query
      else  count processword  then
    ?dup 0=  until ;
: [if]  ( f | ... -- ) 0=  if  skipwords  then ; immediate
: [else]  ( | ... -- ) skipwords ; immediate
: [then] ; immediate
: [defined]  ( | <word> -- f ) bl word find nip ; immediate
: [undefined]  ( | <word> -- f ) bl word find 0= nip ; immediate

\ structures
: begin-structure  \ -- addr 0 ; -- size 
   create  here 0 0 ,      \ mark stack, lay dummy 
   does> @ ;            \ -- rec-len 
: end-structure  \ addr n -- 
   swap ! ;          \ set len
: +field  \ n <"name"> -- ; exec: addr -- 'addr 
   create over , +  does> @ + ;
: field:    ( n1 "name" -- n2 ; addr1 -- addr2 ) aligned 2 +field ;
: cfield:   ( n1 "name" -- n2 ; addr1 -- addr2 ) 1   +field ;
                          
\ assembler
vocabulary assembler
only also assembler definitions
: op  ( c | <inst> -- ) create  c,  does>  c@ c, ;
0 op BRK    1 op INC    2 op POP    3 op NIP    4 op SWP
5 op ROT    6 op DUP    7 op OVR    8 op EQU    9 op NEQ
10 op GTH   11 op LTH   12 op JMP   13 op JCN   14 op JSR
15 op STH   16 op LDZ   17 op STZ   18 op LDR   19 op STR
20 op LDA   21 op STA   22 op DEI   23 op DEO   24 op ADD
25 op SUB   26 op MUL   27 op DIV   28 op AND   29 op ORA
30 op EOR   31 op SFT   h# 80 op LIT
: fixup  ( u -- ) here 1- dup>r c@ or r> c! ;
: "  h# 20 fixup ;
: #"  ( u -- ) LIT " , ;      : #  ( c -- ) LIT c, ;
: k  h# 80 fixup ;            : r  h# 40 fixup ;
: $  ( u -- ) allot ;
16 cells buffer: lbls       variable fwdrefs
: fwdref  ( u -- a ) fwdrefs @ swap 16 * th ;
: patch  ( a -- ) here over - 2 - swap c! ;
: resolve  ( u -- ) fwdref
  16  0  do  @+ ?dup  if  patch  then  loop  drop ;
: &  ( u -- ) here over lbls swap th !  resolve ;
: addref  ( u -- ) fwdref
  16  0  do  dup @ 0=  if  here 1- swap !  unloop  |  cell+  loop 
  true abort" too many forward references" ;
: ,  ( u -- ) >r lbls r@ th @ ?dup  if  here 3 + - #  r>drop |
  0 #  r> addref ;
also forth definitions 
: code  ( | <word> -- ) head  also assembler  
  lbls 16 cells erase  pad 256 + fwdrefs !
  fwdrefs @ 256 erase ;
: end-code  previous ;
only forth

\ decompiler
vocabulary decompiler
also decompiler definitions
: skipname  ( a1 -- a2 ) count h# 3f and + ;
: (rfind)  ( xt dp -- a -1 | xt 0 )
  begin  ?dup 0=  if  false  |
    dup>r skipname 2 + over =  if  drop  r>  true  |
    r> skipname @
  again ;
: rfind  ( xt -- a -1 | xt 0 )
  vocs dup 8 + swap  do
    i @ @ (rfind) dup  if  unloop  |  drop
  2 +loop  false ;
: decode-op  ( c -- )
  0  ->  ." BRK"  |  h# 1f and
  0  ->  ." LIT"  |  1  ->  ." INC"  |  2  ->  ." POP"  |
  3  ->  ." NIP"  |  4  ->  ." SWP"  |  5  ->  ." ROT"  |
  6  ->  ." DUP"  |  7  ->  ." OVR"  |  8  ->  ." EQU"  |
  9  ->  ." NEQ"  |  10  ->  ." GTH"  |  11  ->  ." LTH"  |
  12  ->  ." JMP"  |  13  ->  ." JCN"  |  14  ->  ." JSR"  |
  15  ->  ." STH"  |  16  ->  ." LDZ"  |  17  ->  ." STZ"  |
  18  ->  ." LDR"  |  19  ->  ." STR"  |  20  ->  ." LDA"  |
  21  ->  ." STA"  |  22  ->  ." DEI"  |  23  ->  ." DEO"  |
  24  ->  ." ADD"  |  25  ->  ." SUB"  |  26  ->  ." MUL"  |
  27  ->  ." DIV"  |  28  ->  ." AND"  |  29  ->  ." ORA"  |
  30  ->  ." EOR"  |  drop  ." SFT" ;
: decode-bits  ( c -- )
  dup h# 20 and  if  [char] 2 emit  then
  dup h# 80 and  if  [char] k emit  then
  h# 40 and  if  [char] r  emit  then ;
: h2.  ( u -- ) dup 16 u<  if  [char] 0  emit  then
  hex  (u.) type  decimal ;
: h4.  ( u -- ) dup 8 rshift h2.  255 and  h2. ;
: decode-jump  ( a1 -- a2 ) @+ space h4. ;
: decode-bytes  ( a1 -- a2 ) 
  count 0  ?do  count space h2.  loop ;
: decode-special  ( a1 xt -- a2 )
  ['] (slit)  ->  decode-bytes  |
  ['] (if)  ->  decode-jump  |  
  ['] (else)  -> decode-jump  |  drop ;
: decode-jsr  ( a1 xt -- a2 )
  over c@ h# 2e =  if
    ."  JSR2" 1 under+ decode-special  else  drop  then ;
: decode-litk2  ( a1 -- a2 )
  @+ dup>r rfind  if  [char] ; emit  count h# 3f and type
    r> decode-jsr  |  [char] # emit  h4.  r>drop ;
variable lit1
: decode  ( a1 -- a2 ) count
  h# 80  ->  [char] # emit  count dup lit1 c!  h2. |
  h# a0  ->  lit1 off  decode-litk2  |
  lit1 off  dup decode-op  decode-bits ;
: jump?  ( c -- f ) h# 1f and 12 15 within ;
: chkjump  ( mark ... a1 -- mark ... [a2] a1 )
  dup c@ jump? 0= ?exit  lit1 c@ h# 80 and ?exit
  lit1 c@ ?dup 0= ?exit  over 1+ + swap ;
: end?  ( c -- ) 0  ->  true  |  h# 1f and h# 0c = ; 
: finished?  ( mark ... a c -- -1 | mark ... a 0 )
  end? 0=  if  false  |
  over 0=  if  2drop  true  else  false  then ;
: dropfwds  ( mark ... a -- mark ... a )
  begin  over 0= ?exit
    2dup u> 0=  if  nip  else  exit  then
  again ;
only forth definitions also decompiler
: decompile  ( xt -- ) 0 swap ( marker )
  lit1 off
  begin
    chkjump  dropfwds  dup
    decode  space  swap c@ finished? ?exit
  again ;
: see  ( | <word> -- )
  bl word find dup 0= abort" word not found"
  1 =  if  ." (immediate) "  then
  decompile ;
only forth

\ random numbers (taken from prng.tal)
variable seed
: randomize  second seed ! ;
code rnd  ( -- number* )
    \ returns the next number in a 65,535-long sequence,
    \ which is never zero but every other 16-bit number
    \ appears once before the sequence repeats )
    \ http://www.retroprogramming.com/2017/07/xorshift-pseudorandom-numbers-in-z80.html
    ' seed #" LDA "
    DUP " h# 70 # SFT " EOR "
    DUP " 9 # SFT " EOR "
    DUP " h# 80 # SFT " EOR "
    ' seed #" STA " k POP "
    JMP " r
end-code

marker new
.( saving ufx.rom ... ) cr
' (prompt) is prompt
save ufx.rom
' noop is prompt

defer cin   defer cout
' key defer@ is cin    ' emit defer@ is cout
: ctype  ( a u -- ) 0  ?do  count cout  loop  drop ;
defer edit  ( u -- )

vocabulary editor
also editor definitions

create font  hex
00 c, 00 c, 00 c, 00 c, 00 c, 00 c, 00 c, 00 c, 
18 c, 18 c, 18 c, 18 c, 18 c, 00 c, 18 c, 00 c, 
6c c, 6c c, 6c c, 00 c, 00 c, 00 c, 00 c, 00 c, 
36 c, 36 c, 7f c, 36 c, 7f c, 36 c, 36 c, 00 c, 
0c c, 3f c, 68 c, 3e c, 0b c, 7e c, 18 c, 00 c, 
60 c, 66 c, 0c c, 18 c, 30 c, 66 c, 06 c, 00 c, 
38 c, 6c c, 6c c, 38 c, 6d c, 66 c, 3b c, 00 c, 
0c c, 18 c, 30 c, 00 c, 00 c, 00 c, 00 c, 00 c, 
0c c, 18 c, 30 c, 30 c, 30 c, 18 c, 0c c, 00 c, 
30 c, 18 c, 0c c, 0c c, 0c c, 18 c, 30 c, 00 c, 
00 c, 18 c, 7e c, 3c c, 7e c, 18 c, 00 c, 00 c, 
00 c, 18 c, 18 c, 7e c, 18 c, 18 c, 00 c, 00 c, 
00 c, 00 c, 00 c, 00 c, 00 c, 18 c, 18 c, 30 c, 
00 c, 00 c, 00 c, 7e c, 00 c, 00 c, 00 c, 00 c, 
00 c, 00 c, 00 c, 00 c, 00 c, 18 c, 18 c, 00 c, 
00 c, 06 c, 0c c, 18 c, 30 c, 60 c, 00 c, 00 c, 
3c c, 66 c, 6e c, 7e c, 76 c, 66 c, 3c c, 00 c, 
18 c, 38 c, 18 c, 18 c, 18 c, 18 c, 7e c, 00 c, 
3c c, 66 c, 06 c, 0c c, 18 c, 30 c, 7e c, 00 c, 
3c c, 66 c, 06 c, 1c c, 06 c, 66 c, 3c c, 00 c, 
0c c, 1c c, 3c c, 6c c, 7e c, 0c c, 0c c, 00 c, 
7e c, 60 c, 7c c, 06 c, 06 c, 66 c, 3c c, 00 c, 
1c c, 30 c, 60 c, 7c c, 66 c, 66 c, 3c c, 00 c, 
7e c, 06 c, 0c c, 18 c, 30 c, 30 c, 30 c, 00 c, 
3c c, 66 c, 66 c, 3c c, 66 c, 66 c, 3c c, 00 c, 
3c c, 66 c, 66 c, 3e c, 06 c, 0c c, 38 c, 00 c, 
00 c, 00 c, 18 c, 18 c, 00 c, 18 c, 18 c, 00 c, 
00 c, 00 c, 18 c, 18 c, 00 c, 18 c, 18 c, 30 c, 
0c c, 18 c, 30 c, 60 c, 30 c, 18 c, 0c c, 00 c, 
00 c, 00 c, 7e c, 00 c, 7e c, 00 c, 00 c, 00 c, 
30 c, 18 c, 0c c, 06 c, 0c c, 18 c, 30 c, 00 c, 
3c c, 66 c, 0c c, 18 c, 18 c, 00 c, 18 c, 00 c, 
3c c, 66 c, 6e c, 6a c, 6e c, 60 c, 3c c, 00 c, 
3c c, 66 c, 66 c, 7e c, 66 c, 66 c, 66 c, 00 c, 
7c c, 66 c, 66 c, 7c c, 66 c, 66 c, 7c c, 00 c, 
3c c, 66 c, 60 c, 60 c, 60 c, 66 c, 3c c, 00 c, 
78 c, 6c c, 66 c, 66 c, 66 c, 6c c, 78 c, 00 c, 
7e c, 60 c, 60 c, 7c c, 60 c, 60 c, 7e c, 00 c, 
7e c, 60 c, 60 c, 7c c, 60 c, 60 c, 60 c, 00 c, 
3c c, 66 c, 60 c, 6e c, 66 c, 66 c, 3c c, 00 c, 
66 c, 66 c, 66 c, 7e c, 66 c, 66 c, 66 c, 00 c, 
7e c, 18 c, 18 c, 18 c, 18 c, 18 c, 7e c, 00 c, 
3e c, 0c c, 0c c, 0c c, 0c c, 6c c, 38 c, 00 c, 
66 c, 6c c, 78 c, 70 c, 78 c, 6c c, 66 c, 00 c, 
60 c, 60 c, 60 c, 60 c, 60 c, 60 c, 7e c, 00 c, 
63 c, 77 c, 7f c, 6b c, 6b c, 63 c, 63 c, 00 c, 
66 c, 66 c, 76 c, 7e c, 6e c, 66 c, 66 c, 00 c, 
3c c, 66 c, 66 c, 66 c, 66 c, 66 c, 3c c, 00 c, 
7c c, 66 c, 66 c, 7c c, 60 c, 60 c, 60 c, 00 c, 
3c c, 66 c, 66 c, 66 c, 6a c, 6c c, 36 c, 00 c, 
7c c, 66 c, 66 c, 7c c, 6c c, 66 c, 66 c, 00 c, 
3c c, 66 c, 60 c, 3c c, 06 c, 66 c, 3c c, 00 c, 
7e c, 18 c, 18 c, 18 c, 18 c, 18 c, 18 c, 00 c, 
66 c, 66 c, 66 c, 66 c, 66 c, 66 c, 3c c, 00 c, 
66 c, 66 c, 66 c, 66 c, 66 c, 3c c, 18 c, 00 c, 
63 c, 63 c, 6b c, 6b c, 7f c, 77 c, 63 c, 00 c, 
66 c, 66 c, 3c c, 18 c, 3c c, 66 c, 66 c, 00 c, 
66 c, 66 c, 66 c, 3c c, 18 c, 18 c, 18 c, 00 c, 
7e c, 06 c, 0c c, 18 c, 30 c, 60 c, 7e c, 00 c, 
7c c, 60 c, 60 c, 60 c, 60 c, 60 c, 7c c, 00 c, 
00 c, 60 c, 30 c, 18 c, 0c c, 06 c, 00 c, 00 c, 
3e c, 06 c, 06 c, 06 c, 06 c, 06 c, 3e c, 00 c, 
18 c, 3c c, 66 c, 42 c, 00 c, 00 c, 00 c, 00 c, 
00 c, 00 c, 00 c, 00 c, 00 c, 00 c, 00 c, ff c, 
1c c, 36 c, 30 c, 7c c, 30 c, 30 c, 7e c, 00 c, 
00 c, 00 c, 3c c, 06 c, 3e c, 66 c, 3e c, 00 c, 
60 c, 60 c, 7c c, 66 c, 66 c, 66 c, 7c c, 00 c, 
00 c, 00 c, 3c c, 66 c, 60 c, 66 c, 3c c, 00 c, 
06 c, 06 c, 3e c, 66 c, 66 c, 66 c, 3e c, 00 c, 
00 c, 00 c, 3c c, 66 c, 7e c, 60 c, 3c c, 00 c, 
1c c, 30 c, 30 c, 7c c, 30 c, 30 c, 30 c, 00 c, 
00 c, 00 c, 3e c, 66 c, 66 c, 3e c, 06 c, 3c c, 
60 c, 60 c, 7c c, 66 c, 66 c, 66 c, 66 c, 00 c, 
18 c, 00 c, 38 c, 18 c, 18 c, 18 c, 3c c, 00 c, 
18 c, 00 c, 38 c, 18 c, 18 c, 18 c, 18 c, 70 c, 
60 c, 60 c, 66 c, 6c c, 78 c, 6c c, 66 c, 00 c, 
38 c, 18 c, 18 c, 18 c, 18 c, 18 c, 3c c, 00 c, 
00 c, 00 c, 36 c, 7f c, 6b c, 6b c, 63 c, 00 c, 
00 c, 00 c, 7c c, 66 c, 66 c, 66 c, 66 c, 00 c, 
00 c, 00 c, 3c c, 66 c, 66 c, 66 c, 3c c, 00 c, 
00 c, 00 c, 7c c, 66 c, 66 c, 7c c, 60 c, 60 c, 
00 c, 00 c, 3e c, 66 c, 66 c, 3e c, 06 c, 07 c, 
00 c, 00 c, 6c c, 76 c, 60 c, 60 c, 60 c, 00 c, 
00 c, 00 c, 3e c, 60 c, 3c c, 06 c, 7c c, 00 c, 
30 c, 30 c, 7c c, 30 c, 30 c, 30 c, 1c c, 00 c, 
00 c, 00 c, 66 c, 66 c, 66 c, 66 c, 3e c, 00 c, 
00 c, 00 c, 66 c, 66 c, 66 c, 3c c, 18 c, 00 c, 
00 c, 00 c, 63 c, 6b c, 6b c, 7f c, 36 c, 00 c, 
00 c, 00 c, 66 c, 3c c, 18 c, 3c c, 66 c, 00 c, 
00 c, 00 c, 66 c, 66 c, 66 c, 3e c, 06 c, 3c c, 
00 c, 00 c, 7e c, 0c c, 18 c, 30 c, 7e c, 00 c, 
0c c, 18 c, 18 c, 70 c, 18 c, 18 c, 0c c, 00 c, 
18 c, 18 c, 18 c, 18 c, 18 c, 18 c, 18 c, 00 c, 
30 c, 18 c, 18 c, 0e c, 18 c, 18 c, 30 c, 00 c, 
31 c, 6b c, 46 c, 00 c, 00 c, 00 c, 00 c, 00 c, 
ff c, ff c, ff c, ff c, ff c, ff c, ff c, ff c, 

decimal

variable row    variable col
variable rows   variable columns
variable screen  variable loadbuf
variable locked
variable pointerx   variable pointery
variable modified
defer noedit  ( -- f )
defer terminate
512 constant width      320 constant height
4 constant tabwidth     1000 constant #shadow

: /screen  ( -- u ) screen @ negate ;
: /block  ( -- u ) /screen columns @ - ;
: point  ( -- r c ) row @ col @ ;
: >screen  ( r c -- a ) swap columns @ * + screen @ + ;
: at  ( r c -- ) 3 lshift  swap 3 lshift  position ;
: locate  point at ;
: line  ( -- a u ) point >screen  columns @ col @ - ;
: prepare  ( a u -- ) >r  tib  r@ cmove  tib dup >in !  r> + >limit ! ;
: screen>buf  screen @ loadbuf @ /block cmove ;
: buf>screen  loadbuf @ screen @ /block cmove ;

variable textcolor
create default-theme  h# 0b75 , h# 0da6 , h# 0db8 ,
: reverse  h# 4e textcolor ! ;
: normal  h# 41 textcolor ! ;
: highlite  h# 43 textcolor ! ;
: initscreen  width height screensize!
  height 3 rshift rows !  width 3 rshift columns !
  rows @ columns @ * dup negate screen !  
  screen @ /block - loadbuf !
  screen @ swap blank 
  pointerx off  pointery off ;
: initcolors  default-theme apply-theme  normal ;
: glyph  ( c -- a ) dup 32 128 within 0=  if  drop  127  then
  bl - 3 lshift font + spritedata ;

: drawchar  ( c -- ) glyph  textcolor @ sprite ;
: drawrow  ( row -- ) dup  0 at  0 >screen  1 auto
  dup c@ [char] \ =  if  highlite  then
  columns @  0  do  count drawchar  loop  drop  normal ;

30 constant blinks
variable ccount  1 ccount !
variable cursorcol  2 cursorcol !
: drawcursor  ( c -- ) locate  127 glyph  sprite ;
: cursor  ( f -- )
  if  blinks ccount !  2 dup cursorcol !  else  0  then
  drawcursor ;
: blink
  ccount @ 1- ?dup  if  ccount !  |
  blinks ccount !  
  cursorcol @  if  0  else 2  then  dup cursorcol ! drawcursor ; 
' blink is tick

: pointer  ( f -- ) pointerx @ 3 lshift pointery @ 3 lshift position  
  127 glyph  if  h# 03 sprite  |
  h# 00 sprite  1 cursor ;
: home  0 cursor  row off  col off  1 cursor ;
: paste  yank  0 cursor  line rot min cmove
  line -trailing nip col !  row @ drawrow  1 cursor  
  modified on ;

: clrscr  bl glyph  1 auto
  rows @  0  do  
    i 0 at  columns @  0  do  h# 40 sprite  loop  
  loop ;
: redraw  clrscr  rows @  0  do  i drawrow  loop ;
: scroll  1 0 >screen  screen @  columns @ rows @ 1- * cmove
  rows @ 1- 0 >screen columns @ blank  redraw ;
: newline  locked @  if  1 cursor  |
  col off  row @ 1+ dup rows @ =  if  drop  scroll  
  else  row !  then  1 cursor ;
: scrolldown  row @ 1+ 0 >screen dup columns @ +
  rows @ 2 - row @ - columns @ * cmove>
  row @ 1+ 0 >screen columns @ blank ;
: scrollup  ( row -- ) dup>r 1+ 0 >screen dup columns @ -
  rows @ 2 - r> - columns @ *  cmove  
  rows @ 2 - 0 >screen columns @ blank ;

: split  row @ rows @ 3 - >= ?exit  0 cursor
  scrolldown  line >r  row @ 1+ 0 >screen r> cmove
  line blank  col off  1 row +!  modified on  redraw ;
: enter
  locked @  if  split  1 cursor  |  0 cursor  
  row @ 0 >screen columns @ -trailing prepare  newline ;
: advance  col @ 1+ dup columns @ =  if  drop  newline  |  
  col !  1 cursor ;
: insert  ( c -- ) col @ columns @ =  if  drop  |
  0 cursor  line 1- >r dup 1+ r> cmove>  point >screen c!
  row @ drawrow  advance  modified on ;
: left  col @  if  0 cursor  -1 col +!  1 cursor  then ;
: right  col @ columns @ 1- = ?exit  0 cursor  advance ;
: up  0 cursor  row @ 1- 0 max row !  1 cursor ;
: down  0 cursor  row @ 1+ rows @ 1- 
  locked @  if  1-  then  min row !  1 cursor ;
: blankend  bl row @ columns @ 1- >screen c! 
  row @ drawrow  1 cursor  modified on ;
: backup  row @ 0= ?exit  0 cursor
  line drop  row @ 1- 0 >screen columns @ -trailing dup col ! dup>r +
    columns @ r> -  cmove  row @ scrollup  -1 row +!
  modified on  redraw  1 cursor ;
: rubout  col @ 0=  if  locked @  if  backup  then  |
  0 cursor  -1 col +!  line 1- >r dup 1+ swap r> cmove  blankend ;
: delete  col @ columns @ 1- = ?exit 
  0 cursor  line 1- >r dup 1+ swap r> cmove  blankend ;
: top  0 cursor  row off  1 cursor ;
: start  0 cursor  col off  1 cursor ;
: end  0 cursor  row @ 0 >screen columns @ -trailing nip
  columns @ 1- min col !  1 cursor ;
: tab  0 cursor  col @ dup>r tabwidth u/ 1+ tabwidth *
  columns @ 1- min col !
  row @ r@ >screen  col @ r> - 2dup over +  
    columns @ col @ - cmove>
  bl fill  
  row @ drawrow  1 cursor ;
: rkill  0 cursor  line -trailing
  dup col @ or 0=  if
    2drop  row @ scrollup  redraw  else
    2dup snarf blank  row @ drawrow  then
  1 cursor  modified on ;
: lkill  col @ columns @ 1- = ?exit 
  0 cursor  line >r dup col @ - r> cmove
  line drop columns @ col @ - 2dup snarf blank  col off
  row @ drawrow  1 cursor  modified on ;

variable dirty
: gemit  ( c -- )  dirty on
  0 cursor  10  ->  newline  |  locate  dup point >screen c! 
  drawchar  advance ;

defer status
: update  locked @  if  status  then ;

defer ctrl-key  ( key -- key|0 )
defer other-key  ( key -- )
: handlebutton  ( key but -- key|0 )
  h# 1  ->  ctrl-key  |  
  8  ->  home  drop 0  |
  h# 10  ->  up  drop 0  |  
  h# 20  ->  down  drop 0  |  
  h# 40  ->  left  drop 0  |  
  h# 80  ->  right  drop 0  |  drop ;
: input  0 pointer
  jkey  jbutton handlebutton  other-key  update  wait ;
: no-events  0 jvector  0 mvector  0 cvector  ['] failing svector ;

variable loading    variable block
variable endload
: >loadbuf  ( r c -- a ) swap columns @ * + loadbuf @ + ;
: newblock
  block @ #shadow <  if
    s" \  Load  Save  Snarf  Paste  Prev  Next  Abort  Doc"
  else
    <# s"  Save  Snarf  Paste  Prev  Next  Abort  Code" holds
       block @ #shadow - #s  s" \s #" holds  #>
  then  screen @ swap cmove ;
: fileblock  ( a u1 -- u2 ) (u.) filename  
  /block 2dup blank fileread ;
: loadblock  ( u -- ) screen @ /screen blank  dup block !
  screen @ swap fileblock  0=  if  newblock  then
  modified off ;
: save-block
  modified 0= ?exit  block @ 0= abort" no block specified"
  block @ (u.) filename  screen @ /block filewrite
  /block <> abort" error while writing block"
  modified off  update ;
: writeblock
  rows @ 1-  0  do  i 0 >loadbuf columns @ -trailing ctype  10 cout  
  loop ;

create editpos 0 , 0 ,
: savepos  editpos row @ !+ col @ swap ! ;
: restorepos  editpos @+ row ! @ col ! ;
: enteredit  0 cursor  dirty off
  locked on  ['] noop is prompt  restorepos  1 cursor  update ;
: exitedit  ( -- f ) modified @  if  false  |
  savepos  0 cursor  locked off  rows @ 1- row !  col off
  line blank  rows @ 1- drawrow  1 cursor
  ['] (prompt) is prompt  true ;

: mouse>loc  ( -- r c ) mouse 3 rshift  swap 3 rshift ;
: scanleft  ( r c -- a ) begin  
    0  ->  0 >screen  |
    2dup >screen c@ 33 <  if  1+ >screen  |
    1-
  again ;
: scanright  ( r c -- a ) begin  
    columns @  ->  columns @ >screen  |
    2dup >screen c@ 33 <  if  >screen  |
    1+
  again ;
: below  ( -- a u )
  mouse>loc 2dup scanright >r  scanleft r> over - ;
: blockref?  ( a u1 -- u2 1 | a u1 0 )
  over c@ [char] # <>  if  false  |
  1 /string pad place  pad number 0=  if  count  false  |
  true ;

: warp  0 cursor  mouse>loc col ! 
  locked @  if  rows @ 2 - min  then  
  row !  1 cursor  update ;
: loadref  ( u -- )
  locked @ 0<>  modified @ 0<>  and  if  drop  |  edit ;
: clicked  mstate
  1  ->  warp  |  
  4  ->  below ?dup  if
    blockref?  if  loadref  else
      locked @  if  dirty off  screen>buf  then
      prepare  no-events  r>drop  then 
    else  drop  then  |  drop ;
: mouseinput  0 pointer  mouse>loc pointerx !
  pointery !  1 pointer  clicked  wait ;

: handlecin  ( c -- )  
  0  ->  |  9  ->  tab  |
  10  ->  enter  no-events  r>drop  |  insert ;
: stdin  18 dei  handlecin  wait ;
: events  ['] input jvector  ['] stdin cvector
  ['] mouseinput mvector ;
: listen  events
  dirty @ 0<>  locked @ 0<>  and  if
    buf>screen  redraw  update  dirty off  then
  wait ;

: load1  ( u --)
  loadbuf @ over fileblock 0= abort" no such block" 
  block ! ;
: endread  endload @ block @ u>  if  loading off  block @ 1+ load1  |  
  ['] listen is query  tib dup >in ! >limit !
  locked @ 0=  if  ['] (prompt) is prompt  then ;
: readblock   loading @ dup rows @ 1- =  if  drop  endread  |
  0 >loadbuf columns @ -trailing >r tib r@ cmove
  tib >in !  tib r> + >limit !  1 loading +! ;
: evalblock
  ['] readblock is query  loading off
  ['] noop is prompt  >limit @ >in !  interpret ;
: switch-block  ( u -- )  modified @  if  drop  |  savepos  edit ;
: next-block  block @ 1+ switch-block ;
: previous-block  block @ 1- ?dup  if  switch-block  then ;
: toggle-block  block @ #shadow 2dup <  if  +  else  -  then  
  switch-block ;
: save-and-eval-block
  locked @  if  save-block  noedit  drop  then
  screen>buf  evalblock ;

: error  ['] (prompt) is prompt  modified off
  ['] failing svector
  locked @  if  noedit drop  then
  loading @  if ['] listen is query  loading off  endload off
    then  (abort) ;
: exitforth  locked @ 0= modified @ 0= or  if  bye  then ;
: toggle  locked @ 0=  if  
    block @ ?dup if  loadblock  redraw  enteredit  restorepos  then  |
  noedit  if  abort  then ;

: editstatus  rows @ 1- 0 >screen dup>r columns @ blank
  row @ 1+ (u.) r@ swap cmove
  col @ 1+ (u.) r@ 3 + swap cmove
  <# block @ #s  [char] # hold #>  r@ 6 + swap cmove
  unused (u.) r@ 28 + swap cmove
  modified @  if  127  else  bl  then  r> columns @ 1- + c!
  reverse  rows @ 1- drawrow  normal ;

: (ctrl-key)  ( key -- key|0 ) 
  [char] a  ->  start  0  |  
  [char] c  ->  terminate 0  |
  [char] e  ->  end  0 | 
  [char] k  ->  rkill  0  |  
  [char] u  ->  lkill  0  |  
  [char] v  ->  paste  0  |
  [char] i  ->  1 14 deo  0  |
  [char] s  ->  save-block  0  |
  [char] n  ->  next-block  0  |
  [char] h  ->  toggle-block  0  |
  [char] p  ->  previous-block  0  |
  [char] d  ->  delete  0  | ;

: (other-key)  ( key -- )
  0  ->  | 
  9  ->  tab  |
  27  ->  toggle  |
  8  -> rubout  |  
  13  ->  enter  no-events  update  r>drop  |  insert ;

' exitedit is noedit  ' editstatus is status
' exitforth is terminate
' (ctrl-key) is ctrl-key
' (other-key) is other-key

only definitions also editor
: blk  ( -- a ) block ;
: page  screen @ rows @ columns @ * blank  redraw  home ;
: at-xy  ( x y -- ) 0 cursor
  0 rows @ clamp row !  0 columns @ clamp col ! 
  1 cursor ;
: Save  save-block ;
: Snarf  row @ 0 >screen columns @ -trailing snarf ;
: Paste  paste ;
: Load  save-and-eval-block ;
: load  ( u -- )  endload off  load1 evalblock  ;
: thru  ( u1 u2 -- ) endload !  load1 evalblock ;
: (edit)  ( u -- ) loadblock  redraw  enteredit ;
' (edit) is edit
: Next  next-block ;
: Prev  previous-block ;
: Abort  modified off  noedit  drop ;
: Doc  block @ #shadow <  if  toggle-block  then ;
: Code  block @ #shadow >=  if  toggle-block  then ;
: export  ( u1 u2 -- )
  1+ swap  ?do  
    loadbuf @ i fileblock 0= abort" no such block"
    writeblock
  loop ;
: \s  >limit @ >in !
  loading @  if  rows @ 1- loading !  then ;
: copy  ( u1 u2 -- ) swap loadblock  block !  modified on  redraw
  enteredit ;
: list  ( u -- ) load1 loadbuf @  rows @  1  do
    dup columns @ -trailing cr type  columns @ +  loop  drop ;

defer banner
: _banner  cr  ."   (UF) " version  . 
  ." - (c)MMXXII Felix L. Winkelmann" cr  2 spaces
  unused u. ." bytes free."  cr  cr  prompt ;
' _banner is banner
: boot  initscreen  initcolors  theme
  dirty off  locked off  modified off
  ['] error is abort  ['] listen is query  ['] gemit is emit
  home  banner  quit ;

only
marker new
.( saving uf.rom ... )
' (prompt) is prompt
save uf.rom
bye
