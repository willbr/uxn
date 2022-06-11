\ this is my comment

%>r { STH2 }
%r> { STH2r }
%i' { OVR2r STH2r }

: init
    1 2 >r >r
    i'
    debug
    r>
    r>
    debug
    halt
brk;

~two.fth


