@echo off
setlocal enabledelayedexpansion
::type %1
echo.
set table=LITINCPOPNIPSWPROTDUPOVREQUNEQGTHLTHJMPJCNJSRSTHLDZSTZLDRSTRLDASTADEIDEOADDSUBMULDIVANDORAEORSFT
call :init_memory
call :init_vars
call :parse_hex_file %1
call :dump 256 11
echo.
exit /b


:dump %addr %length
    set /a i=0
    set /a dump_end = %1+%2-1
    set pc=%1
    goto :dump_pred
:dump_loop
    set byte=!mem_%pc%!

    if %byte% EQU 128 (
        set /a pos="pc+1"
        call :peek8 b1 !pos!
        echo %pc% : !byte! !b1! LIT # !b1!
        set /a pc+=1
    ) else if %byte% EQU 160 (
        set /a pos="pc+1"
        call :peek8 b1 !pos!
        set /a pos="pc+2"
        call :peek8 b2 !pos!
        set /a n="(b1<<8)+b2"
        echo %pc% : !byte! !b1! !b2! LIT2 # !n!
        set /a pc+=2
    ) else (
        call :dis op !byte!
        echo %pc% : !byte! !op!
    )

    set /a pc+=1
    set /a i+=1
:dump_pred
    if %i% LSS %2 (
        goto :dump_loop
    )
    exit /b


:dis %out %byte
    if %2 EQU 0 (
        set %1=BRK
        exit /b
    ) else if %2 EQU 128 (
        set %1=LIT
        exit /b
    )
    set mode=
    set /a instruction="%2 & 0x1f"
    set /a offset="instruction*3"
    set base_op=!table:~%offset%,3!
    set /a k="%2&0x80"
    set /a r="%2&0x40"
    set /a s="%2&0x20"
    if %k% GTR 0 (set k=k) else (set k=)
    if %r% GTR 0 (set r=r) else (set r=)
    if %s% GTR 0 (set s=2) else (set s=)
    set %1=%base_op%%k%%r%%s%
    exit /b


:init_vars
    set /a pc=0x100
    exit /b


:init_memory
    echo init
    for /l %%a in (0,1,0x1ff) do (
        set mem_%%a=0
    )
    echo end init
    echo.
    exit /b


:parse_hex_file
    for /f "delims=" %%a in ( %1 ) do (
        set line=%%a
        call :parse_bytes !line:~5,48!
    )
    echo.
    exit /b


:parse_bytes
    for %%b in (%*) do (
        set /a byte=0x%%b
        call :poke8 !pc! !byte!
        set /a pc+=1
    )
    exit /b


:peek8 %out %addr
    set %1=!mem_%2!
    exit /b

:poke8 %addr %byte
    set mem_%1=%2
    exit /b

