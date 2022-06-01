# add see to print word code

rename cfa to dfa
add reverse lookup

    >dfa
    dfa>


# predicates

    pred if true-body else false-body then
    then or endif?


# loops
    limit index do ? loop
    limit index do ? +loop

    i ( -- index )

    begin body f until
    begin body f again
    begin pred while body repeat

    leave
    2000 > if leave then



# string functions
    : word ( c -- ) ;
    \ read until char c


    : strtok ( delim_ptr -- ) ;
    \ read token until char in str delim

