# forth

16 bit


: dis ( dissasemble Uxn ) ;
: see ( pretty print forth code ) ;

# ux

    old dos like block cursor
    make it read like pico8

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
        

# syntax

    %binary %010101010
    %dec %12345
    $hex $beefcafe

# string functions

    strtok

# games

    tetris

# references

https://github.com/nornagon/jonesforth/blob/master/jonesforth.S

