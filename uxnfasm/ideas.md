# uxnfasm

## example

```forth
    \ this is a comment

    : init
        10 0 do
            $2a emit
        loop

        cr
    brk;
```

## todo

    base
    here
    allot
    @ c@
    ! c!
    , c,
    type ( address length -- )

