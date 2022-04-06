# ideas

## c ish syntax

    (n > 1) if {
        "hi" puts
    } else {
        "nope" puts
    }

into

    n 1 >
    ;true jcn
    @else
        "nope" ;puts jsr2
        ;endif jmp
    @true
        "hi" ;puts jsr2
    @endif

## loop

infinite loop

    loop {
        "beep" puts
    }

into

    @loop
        "beep" puts
    ;loop jmp


## incbin

    incbin "filename" offset length

    incbin "sprite.gly" 0 0

## repeat block

    repeat 10 {
        0000 1111 2222 3333
    }

