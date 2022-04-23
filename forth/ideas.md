# forth

16 bit


: dis ( dissasemble Uxn ) ;
: see ( pretty print forth code ) ;

# readline
    bg text
    fg cursor & mouse

    Ctrl+l clear screen
    Ctrl+u clear line

    Ctrl+a start of line
    Ctrl+e end of line

    Ctrl+p history prev
    Ctrl+n history next

# dictionary

    @dict
        &link $2
        &name_length $1
        &name &?????
        &codeword &????
        
    codeword is asm code

    or a call to DOCOL if forth code
    DOCOL


    double
        docol lit 2 * exit
        

# references

https://github.com/nornagon/jonesforth/blob/master/jonesforth.S

