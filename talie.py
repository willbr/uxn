# https://wiki.xxiivv.com/site/varvara.html

# https://wiki.xxiivv.com/site/uxntal.html
# https://wiki.xxiivv.com/site/uxntal_cheatsheet.html
# https://wiki.xxiivv.com/site/uxntal_reference.html
# https://wiki.xxiivv.com/site/uxntal_stacking.html
# https://wiki.xxiivv.com/site/uxntal_macros.html

import fileinput

indent_width = 4
cur_indent = 0
cmd_stack = []

ops = """
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
""".strip()

op_table = {}
for line in ops.split('\n'):
    op, code, *comment = line.split(' ', 2)
    n = int(code, 16)
    op_table[op] = n


class UxnRom():
    def __init__(self):
        self.rom = bytearray()
        self.pc = 0


    def __repr__(self):
        return ' '.join(f'{c:02x}' for c in self.rom)

    def write(self, token, note=''):
        if note:
            print(f"{note:6s} {token}")

        first_char = token[:1]

        if first_char == '#':
            n = int(token[1:], 16)
            assert n >= 0
            assert n <= 0xffff
            if n > 0xff:
                self.write_op('lit2')
                self.write_short(n)
            else:
                self.write_op('lit')
                self.write_byte(n)
        elif token[:3] in op_table:
            self.write_op(token)
        else:
            print(token)
            assert False

    def write_byte(self, n):
        assert n >= 0
        assert n <= 0xff
        delta = self.pc - len(self.rom) + 1
        self.rom += bytes(delta)
        self.rom[self.pc] = n
        self.pc += 1


    def write_short(self, n):
        assert n >= 0
        assert n <= 0xffff

        low  = n and 0x0f
        high = n and 0xf0

        delta = (self.pc + 1) - len(self.rom) + 1
        self.rom += bytes(delta)
        self.rom[self.pc] = low
        self.pc += 1
        self.rom[self.pc] = high
        self.pc += 1

    def write_op(self, op):
        lhs, rhs = op[:3], op[3:]
        code = op_table[lhs]
        for c in rhs:
            if c == 'k':
                code = code and 0x80
            elif c == 'r':
                code = code and 0x40
            elif c == '2':
                code = code and 0x20
            else:
                raise SyntaxError(f"unknown mode: {c}")
        self.write_byte(code)


def read_token(buf):
    try:
        head, tail = buf.split(' ', 1)
    except ValueError:
        head, tail = buf.strip(), ''
    return head, tail


def main():
    global cur_indent

    rom = UxnRom()
    buf = fileinput.input()
    while True:
        try:
            raw_line = next(buf)
        except StopIteration:
            break

        first_char = raw_line[:1]
        # print(repr(first_char))

        if first_char == '\n':
            continue
        elif first_char == ' ':
            line = raw_line.lstrip(' ')
            spaces = len(raw_line) - len(line)
            this_indent = spaces / indent_width

            if (spaces % indent_width) != 0:
                print(spaces, spaces % indent_width)
                assert False

        else:
            this_indent = cur_indent
            line = raw_line

        head, rest = read_token(line)
        rest = rest.rstrip('\n')

        if this_indent > cur_indent + 1:
            assert False
        elif this_indent == cur_indent + 1:
            cmd_stack.append(head)
            cur_indent += 1
        elif this_indent == cur_indent:
            if cmd_stack:
                rom.write(cmd_stack.pop(), 'next')
            cmd_stack.append(head)
        else:
            assert False

        # print(repr(head), repr(rest))

        args = rest
        while args != '':
            arg, args = read_token(args)
            # print(repr(arg), repr(args))
            print('arg   ', arg)

    # print(cmd_stack)
    while cmd_stack:
        print('unwind', cmd_stack.pop())

    print(rom)


if __name__ == "__main__":
    main()

