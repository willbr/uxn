( large letter f )

macro halt  #01 .System/state DEO end-macro
macro emit  .Console/write DEO end-macro
macro debug cr #01 .System/debug DEO cr end-macro
macro cr #0a emit end-macro

macro + ADD2 end-macro
macro - SUB2 end-macro
macro * MUL2 end-macro
macro / DIV2 end-macro

: init
    10 10 + .
brk;

: .
    debug
;

