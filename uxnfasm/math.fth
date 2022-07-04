: sin-offset ( s16 -- u8 )
    256 mod
;

: sin ( n -- sin(n) )
    sin-offset
    ;sin-table
    +
    LDA #00 SWP
    u8>s16
;


: cos ( s16 -- cos(n) )
    64 + sin
;

~sin-table.fth
