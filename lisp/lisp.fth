\ main
\ read
\ getobject
\ gettoken
\ getchar 

%\n { #000a }

: init
    LIT 'i emit1 cr
    ;on-console .Console/vector DEO2
brk;

: on-console ( -> )
    #00 .Console/read DEI
    \ dup emit
    dup \n = if
    drop
        ;newline emit-string
    else
        drop

    endif
brk;

@newline "newline"
