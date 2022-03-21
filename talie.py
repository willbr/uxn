# https://wiki.xxiivv.com/site/varvara.html

# https://wiki.xxiivv.com/site/uxntal.html
# https://wiki.xxiivv.com/site/uxntal_cheatsheet.html
# https://wiki.xxiivv.com/site/uxntal_reference.html
# https://wiki.xxiivv.com/site/uxntal_stacking.html
# https://wiki.xxiivv.com/site/uxntal_macros.html

from rich.console import Console
from rich.traceback import install
import fileinput
import argparse

console = Console(markup=False)
python_print = print
print = console.print

install(show_locals=True)

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
reverse_op_table = {}

for line in ops.split('\n'):
    op, code, *comment = line.split(' ', 2)
    n = int(code, 16)
    op_table[op] = n
    reverse_op_table[n] = op


class UxnRom():
    def __init__(self):
        self.rom = bytearray()
        self.pc = 0
        self.scope = None
        self.refs = []
        self.labels = {}


    def __repr__(self):
        return 'Rom: ' + ' '.join(f'{c:02x}' for c in self.rom[0x100:])


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
        elif first_char == '|':
            n = int(token[1:], 16)
            assert n < 0x10000
            self.pc = n
        elif first_char == '@':
            label_name = token[1:]
            self.make_label(label_name)
            self.scope = label_name
        elif first_char == '&': # sub-label define
            assert self.scope != None
            sub_name = token[1:]
            self.make_label(self.sub_label(sub_name))
        elif first_char == ';': # literal address absolute
            self.make_reference(token, self.pc)
            self.write_lit_short(0xffff)
        elif first_char == ',': # literal address relative
            self.make_reference(token, self.pc)
            self.write_lit_byte(0xff)
        elif first_char == '"':
            for b in bytes(token[1:], 'ascii'):
                self.write_byte(b)
        elif token[:3].lower() in op_table:
            self.write_op(token)
        elif token == 'rpn':
            pass
        else:
            n = int(token, 16)
            if n > 0xff:
                self.write_short(n)
            else:
                self.write_byte(n)


    def sub_label(self, name):
        label_name = f"{self.scope}/{name}"
        return label_name


    def make_label(self, label_name):
        assert label_name not in self.labels
        self.labels[label_name] = self.pc


    def make_reference(self, label, addr):
        rune = label[0]
        if label[1] == '&':
            ref_name = self.sub_label(label[2:])
        else:
            ref_name = label[1:]
        self.refs.append([ref_name, rune, addr])


    def write_byte(self, n):
        assert n >= 0
        assert n <= 0xff
        delta = self.pc - len(self.rom) + 1
        if delta > 0:
            self.rom += bytes(delta)
        self.rom[self.pc] = n
        self.pc += 1


    def write_signed_byte(self, n):
        if n < 0:
            u = 255 + n
        elif n > 127:
            assert False
        else:
            u = n
        self.write_byte(u)


    def write_short(self, n):
        assert n >= 0
        assert n <= 0xffff

        low  = n & 0x00ff
        high = n >> 8

        self.write_byte(high)
        self.write_byte(low)
        

    def write_lit_byte(self, n):
        self.write_op('lit')
        self.write_byte(n)


    def write_lit_short(self, n):
        self.write_op('lit2')
        self.write_short(n)


    def write_op(self, op):
        lhs, rhs = op[:3], op[3:]
        if lhs == 'lit': # force keep for lit
            if 'k' not in rhs:
                rhs += 'k'
        code = op_table[lhs.lower()]
        for c in rhs:
            if c == 'k':
                code = code | 0x80
            elif c == 'r':
                code = code | 0x40
            elif c == '2':
                code = code | 0x20
            else:
                raise SyntaxError(f"unknown mode: {c}")
        self.write_byte(code)


    def resolve(self):
        # print(self.labels)
        for v in self.refs:
            label, rune, ref_addr = v
            label_addr = self.labels[label]
            # print(label, label_addr)
            # print(rune, ref_addr)
            if rune == '.':
                assert False
            elif rune == ',':
                pc = self.pc
                self.pc = ref_addr + 1
                delta = label_addr - self.pc - 1
                self.write_signed_byte(delta)
            elif rune == ';':
                self.pc = ref_addr + 1
                self.write_short(label_addr)
            elif rune == ':':
                assert False
            else:
                assert False


def read_token(buf):
    try:
        head, tail = buf.split(' ', 1)
    except ValueError:
        head, tail = buf.strip(), ''
    return head, tail


def assemble(filename):
    global cur_indent

    rom = UxnRom()
    buf = fileinput.input(filename)
    while True:
        try:
            raw_line = next(buf)
        except StopIteration:
            break

        first_char = raw_line[:1]

        if first_char == '\n':
            continue
        elif first_char == ' ':
            line = raw_line.lstrip(' ')
            spaces = len(raw_line) - len(line)
            this_indent = spaces / indent_width

            if (spaces % indent_width) != 0:
                print(spaces, spaces % indent_width)
                assert False
        elif first_char == '\t':
            raise SyntaxError

        else:
            this_indent = cur_indent
            line = raw_line

        head, rest = read_token(line)
        rest = rest.rstrip('\n')

        assert head != ''

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

        args = rest
        while args != '':
            arg, args = read_token(args)
            rom.write(arg, 'arg')

    while cmd_stack:
        rom.write(cmd_stack.pop(), 'unwind')

    rom.resolve()

    # print(rom)

    with open('out.rom', 'wb') as f:
        f.write(rom.rom[0x100:])


def disassemble(filename):
    with open(filename, 'rb') as f:
        rom = bytearray(f.read())

    rom_iter = iter(rom)
    while True:
        try:
            b = next(rom_iter)
        except StopIteration:
            break
        data = [b]
        base_op_code = b & 0b00011111
        base_op = reverse_op_table[base_op_code]
        op = base_op
        short_mode = False

        if b & 0b10000000:
            op += 'k'
        if b & 0b01000000:
            op += 'r'
        if b & 0b00100000:
            short_mode = True
            op += '2'

        if base_op == 'lit':
            if short_mode:
                sep = ' '
                high = next(rom_iter)
                low  = next(rom_iter)
                n = (high << 8) + low
                data += [high, low]
                op = f"#{n:04x}"
            elif b & 0b10000000:
                n  = next(rom_iter)
                data += [n]
                op = f"#{n:02x}"
            else:
                op = 'brk'

        s = ' '.join(f"{b:02x}" for b in data)

        a = repr(chr(b))

        print(f"{s:8} {a:6} {op}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="uxn tool")

    parser.add_argument("--assemble")
    parser.add_argument("--disassemble")

    args = parser.parse_args()

    if args.disassemble:
        disassemble(args.disassemble)
    elif args.assemble:
        assemble(args.assemble)
    else:
        assert False

