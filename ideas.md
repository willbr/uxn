# ideas

    inline emit
        deo $18

    inline halt
        deo $010f

    pad-abs $0100

    push-abs @program
        label &while
            ldak
            emit
            inc2
            ldak
            ,&while
            jcn
        pop2
        halt
    brk

    push-abs @hello-world
    "Hello 20 "World! 00

# ideas 2

    inline(emit)
        deo($18)

    inline(halt)
        deo($010f)

    pad-abs($0100)

    push-abs(@program)
        label(&while)
            ldak
            emit
            inc2
            ldak
            ,&while
            jcn
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

# syntax
    rpn 1 48 +
    rpn (1 + 48)
    infix (3 + 5) / (2 + 48)


# misc
    pad-abs $0100
    68 lit
    18 lit
    deo

    rpn lit 68
    rpn lit 17
    deo


    |0100
    deo #68 #18
    deo #65 #18
    deo #6c #18
    deo #6c #18
    deo #6f #18
    deo #0a #18



    |10
    @Console
        &vector
        $2
        &read
        $1
        &pad
        $5
        &write
        $1
        &error
        $1

    |0100
        rpn lit 'h .Console/write deo
        rpn lit 'e .Console/write deo
        rpn lit 'l .Console/write deo
        rpn lit 'l .Console/write deo
        rpn lit 'o .Console/write deo
        rpn #0a    .Console/write deo


    %emit
        rpn .Console/write deo

    %nl
        rpn #0a emit

    |0100
        emit lit 'h
        emit lit 'e
        emit lit 'l
        emit lit 'l
        emit lit 'o
        nl





    rpn (#02 add #30)

    32 <- top

    rpn lit 02 lit 30 add


    rpn lit2 02 30 add

    rpn #0004 #0008 add2
    add2 #0004 #0008







    inline +
        add

    inline -
        sub


# forthy


    fn double dup + end

    fn double
        dup +

    if (n > 1)
        print

    if ( n > 1 ) ie/eol ie/block print ie/end-block


    fn double block dup + end



    fn hi
        "hello world" puts

    fn hi
        call puts "hello"


    fn hi ie/eol
        ie/block "hello world" puts ie/eol ie/end-block


# c syntax

    func hello {
        puts("hello")
    }

    func hello
        puts("hello")

    word hello {
        puts("hi")
    }

    word hello
        puts("hi")


    if (n > 1) {
        puts("hi")
    }

    if n 1 >
        "hi" puts

    n 1 > if
        "hi" puts

    if(n > 1)
        "hi" puts

    if(n > 1)
        puts("hi")


    n 1 > if { "hi" puts }


# no prefix

## one

    rpn + neoteric + blocks

    inline emit
        deo(#18)

    inline halt
        deo(#010f)

    org(0100)

    label program
        lit-addr hello-world

        &while
            emit(ldak)
            inc2
            jcn(ldak, rel-addr(&while))

        pop2
        halt

    brk

    label hello-world
        "Hello World!"


### into

    inline emit
        #18 deo

    inline halt
        #010f deo

    org 0100

    label program
        lit-addr hello-world

        &while
            ldak emit
            inc2
            ldak rel-addr &while jcn

        pop2
        halt

    brk

    label hello-world "Hello World!"

## two

    rpn + neoteric + blocks

    inline emit {
        deo(#18)
    }

    inline halt {
        deo(#010f)
    }

    org(0100)

    label program {
        lit-addr hello-world

        &while {
            emit(ldak)
            inc2
            jcn(ldak, rel-addr(&while))
        }

        pop2
        halt
    }

    brk

    label hello-world {
        "Hello World!"
    }


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

