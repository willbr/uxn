( large letter f )

: init
    f
brk;

: cr 10 emit ;
: star 42 emit ;
: stars 0 do star loop ;
: space 32 emit ;
: spaces 0 do space loop ;
: margin cr 30 spaces ;
: blip margin star ;
: bar margin 5 stars ;
: f bar blip bar blip blip cr ;

