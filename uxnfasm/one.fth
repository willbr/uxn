\ https://solhsa.com/imgui/index.html

%key { .Controller/key DEI }
variable mouse.x
variable mouse.y
variable t

: init ( -> )
    640 .Screen/width DEO2
    480 .Screen/height DEO2

    ;on-frame .Screen/vector DEO2
    ;on-key .Controller/vector DEO2


    ( set system colors )
    #a5ff .System/r DEO2
    #a50f .System/g DEO2
    #fe0f .System/b DEO2
brk;

: on-frame ( -> )
    \ clear screen
    t @ .System/r DEO2

    t @ 1+ t !
brk;

: on-key ( -> )
    key
    debug
    dup k-esc = if
        halt
    endif

    drop
brk;

: on-mouse ( -> )
brk;

