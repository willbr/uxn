from textwrap import dedent

op_table = {}
reverse_op_table = {}

def is_syntax(w):
    return w[0] in "~'\";:,.|@&$#"

def is_op(w):
    lhs = w[:3]
    rhs = w[3:]

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
    BRK 0x00 a b c m[pc+1]
    LIT 0x00
    INC 0x01
    POP 0x02
    DUP 0x03
    NIP 0x04
    SWP 0x05
    OVR 0x06
    ROT 0x07
    EQU 0x08
    NEQ 0x09
    GTH 0x0a
    LTH 0x0b
    JMP 0x0c
    JCN 0x0d
    JSR 0x0e
    STH 0x0f
    LDZ 0x10
    STZ 0x11
    LDR 0x12
    STR 0x13
    LDA 0x14
    STA 0x15
    DEI 0x16
    DEO 0x17
    ADD 0x18
    SUB 0x19
    MUL 0x1a
    DIV 0x1b
    AND 0x1c
    ORA 0x1d
    EOR 0x1e
    SFT 0x1f
    """).strip()

    for line in ops.split('\n'):
        op, code, *comment = line.split(' ', 2)
        n = int(code, 16)
        op_table[op] = n
        reverse_op_table[n] = op

build_op_tables()

