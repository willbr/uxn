\ main
\ read
\ getobject
\ gettoken
\ getchar 

%\n { #000a }

variable in :tib
array tib 256

: init
    ;on-console .Console/vector DEO2
brk;

: on-console ( -> )
    #00 .Console/read DEI
    dup in @ !
    1 in +!
    dup \n = if
        ;newline emit-string
        space print-short
    else
        dup emit
        space print-short
    endif
    cr
brk;

@newline "newline"
