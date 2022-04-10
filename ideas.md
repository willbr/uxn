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

### named loops

    loop beeper {
        "beep" puts
        1 if {
            break
        } else {
            continue
        }
    }

## repeat block

    repeat 10 {
        0000 1111 2222 3333
    }

# 3-INSTRUCTION FORTH

A 3-INSTRUCTION FORTH FOR EMBEDDED SYSTEMS WORK
Illustrated on the Motorola MC68HC11 by Frank Sergeant Copyright 1991 Frank Sergeant 

    XC@   fetch byte
    XC!   store byte
    XCALL call sub

