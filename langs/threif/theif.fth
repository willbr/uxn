\ 3-INSTRUCTION FORTH

\    XC@   fetch byte
\    XC!   store byte
\    XCALL call sub

%key { #00 .Console/read DEI }
%mode-fetch { #0001 }
%mode-store { #0002 }
%mode-call  { #0004 }

|00
@fetch $1
@store &1 $1 &2 $1
@call $1 &1

: init
  ;on-mode .Console/vector DEO2
brk;


: on-mode ( -> )
    key
    dup mode-fetch = if
          ;on-fetch1 .Console/vector DEO2
    else dup mode-store = if
          ;on-store1 .Console/vector DEO2
    else dup mode-call = if
          ;on-call1 .Console/vector DEO2
    else
        debug
    endif endif endif
    drop
brk;


: on-fetch1 ( -> )
    key
    NIP .fetch STZ
    ;on-fetch2 .Console/vector DEO2
brk;

: on-fetch2 ( -> )
    .fetch LDZ
    key NIP
    LDA emit
    ;on-mode .Console/vector DEO2
brk;

: on-store1 ( -> )
    key
    NIP .store/1 STZ
    ;on-store2 .Console/vector DEO2
brk;

: on-store2 ( -> )
    key
    NIP .store/2 STZ
    ;on-store3 .Console/vector DEO2
brk;

: on-store3 ( -> )
    .store/1 LDZ
    .store/2 LDZ
    key NIP
    STA
    ;on-mode .Console/vector DEO2
brk;

: on-call1 ( -> )
    key
    NIP .call STZ
    ;on-call2 .Console/vector DEO2
brk;

: on-call2 ( -> )
    .call LDZ
    key NIP
    ;on-mode .Console/vector DEO2
    JSR2
brk;

