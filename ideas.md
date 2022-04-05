# ideas

## c ish syntax

    (n > 1) if { "hi" puts }

    n 1 > if { "hi" puts }


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

