from textwrap import dedent

op_table = {}
reverse_op_table = {}

def is_syntax(w):
    return w[0] in "~;:,.|@&$#"

def is_op(w):
    lhs = w[:3].lower()
    rhs = w[3:].lower()

    if lhs not in op_table:
        return False

    for c in rhs:
        if c not in "2kr":
            return False

    return True


def build_op_tables():
    global op_table
    global reverse_op_table

    ops = dedent("""
    brk 0x00 a b c m[pc+1]
    lit 0x00
    inc 0x01
    pop 0x02
    dup 0x03
    nip 0x04
    swp 0x05
    ovr 0x06
    rot 0x07
    equ 0x08
    neq 0x09
    gth 0x0a
    lth 0x0b
    jmp 0x0c
    jcn 0x0d
    jsr 0x0e
    sth 0x0f
    ldz 0x10
    stz 0x11
    ldr 0x12
    str 0x13
    lda 0x14
    sta 0x15
    dei 0x16
    deo 0x17
    add 0x18
    sub 0x19
    mul 0x1a
    div 0x1b
    and 0x1c
    ora 0x1d
    eor 0x1e
    sft 0x1f
    """).strip()

    for line in ops.split('\n'):
        op, code, *comment = line.split(' ', 2)
        n = int(code, 16)
        op_table[op] = n
        reverse_op_table[n] = op

build_op_tables()

