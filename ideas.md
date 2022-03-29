# ideas

    inline(emit)
        deo($18)

    inline(halt)
        deo($010f)

    pad-abs($0100)

    push-abs(@program)
        label(&while)
            emit(ldak)
            inc2
            jcn(ldak, rel(&while))
        pop2
        halt
    brk

    push-abs(@hello-world)
    "Hello 20 "World! 00

# comment

## modes

- keep
- return
- short

## runes

- % macro-define
- | pad absolute
- $ pad relative
- @ label-define
- & sublabel-define
- ~ include
- # literal hex
- . literal addr zero-page
- , literal addr relative
- ; literal addr absolute
- : raw addr absolute
- ' raw char
- " raw word


# c ish syntax

## word

    word hello {
        puts("hi")
    }

    if(n > 1) {
        puts("hi")
    }

    n 1 > if { "hi" puts }


# neoteric

## normal

    a(b, c)

    into

    b c a

example

    deo(#18)
    add(#01, #02)

    into

    #18 deo
    #01 #02 add

## special forms

    a(b, c)

    into

    a b , c

example 'inline'
    
    inline(emit)
        deo(#18)

    into

    inline emit
        #18 deo

