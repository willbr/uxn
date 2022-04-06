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


## comment

    comment {
        use blocks to denote comments?
    }


## label blocks

    label square {
        ff81 8181 8181 81ff
    }

is

    label square
        ff81 8181 8181 81ff
        label end-square


## data blocks

numbers should be literal by default
to enter raw blocks, you should use a data block

    data square {
        ff81 8181 8181 81ff
    }


## incbin

    incbin "filename" offset length

    incbin "sprite.gly" 0 0

## repeat block

    repeat 10 {
        0000 1111 2222 3333
    }

## functions

declare functions
jump to functions with jsr or jsr2

    word double {
        dup +
    }

    10
    double

into

    @word {
        dup +
        jmp2r
    }

    10
    ;word jsr2

