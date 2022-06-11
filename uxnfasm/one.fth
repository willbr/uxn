\ this is my comment

%>r { STH2 }
%r> { STH2r }
%i' { OVR2r STH2r }
%j  { ROT2r STH2rk ROT2r ROT2r }

: init
    1 2 3 >r >r >r
    j
    debug
    halt
brk;

~two.fth

