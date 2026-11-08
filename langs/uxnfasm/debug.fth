: print-ere ( u16 str-addr -- )
    ansi-red

    ;&str emit-string
    space
    u.
    emit-string
    cr

    ansi-reset
;
&str "ere line: "

