\ main
\ read
\ getobject
\ gettoken
\ getchar

%\n { #000a }

variable in :tib
array tib 256

: init
    ;prompt emit-string
    ;on-console .Console/vector DEO2
brk;

: on-console ( -> )
    \ in @ print-short space
    #00 .Console/read DEI
    dup in @ c!
    1 in +!
    \n = if
        ;tib emit-string cr
    endif
brk;

@newline "newline"
@prompt ">>> "
