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


class Tokeniser:
    def __init__(self, filename):
        self.i = 0
        self.queued_tokens = []
        with open(filename) as f:
            self.data = f.read()


    def push_token(self, token):
        self.queued_tokens.append(token)


    def peek_token(self):
        if self.queued_tokens:
            t = self.queued_tokens[-1]
            return t

        t = self.read_token()
        self.queued_tokens.append(t)
        return t


    def read_token(self):
        if self.queued_tokens:
            t = self.queued_tokens.pop()
            return t

        start_pos = self.i

        try:
            c = self.data[self.i]

            if c == ' ':
                while self.data[self.i] in ' ':
                    self.i += 1
            elif c == '\n':
                while self.data[self.i] in '\n':
                    self.i += 1
            elif c == '"':
                self.i += 1
                while self.data[self.i] not in '"':
                    self.i += 1
                self.i += 1
            else:
                while self.data[self.i] not in ' \n':
                    self.i += 1
        except IndexError:
            pass

        t = self.data[start_pos:self.i]
        if t.startswith('\n'):
            return '\n'

        try:
            while self.data[self.i] in ' ':
                self.i += 1
        except IndexError:
            pass

        return t


class IndentParser:
    def __init__(self, filename):
        self.tokens = Tokeniser(filename)
        self.indent_width = 4
        self.new_indent = 0
        self.cur_indent = 0

        self.skip_blank_lines()

    def skip_blank_lines(self):
        while self.tokens.peek_token() == '\n':
            _ = self.tokens.read_token()


    def read_token(self):
        t = self.tokens.read_token()

        # print(f"t1 = {repr(t)}")
        while t == '\n':
            nt = self.tokens.peek_token()
            if nt.startswith(' '):
                space_token = self.tokens.read_token()
                spaces = len(space_token)
                assert not spaces % self.indent_width
                if self.tokens.peek_token() == '\n':
                    pass
                else:
                    self.new_indent = spaces // self.indent_width
            else:
                self.new_indent = 0

            # print(f"new_indent = {self.new_indent}")

            diff = self.new_indent - self.cur_indent
            # print(f"diff = {diff}")

            nt = self.tokens.peek_token()
            # print(f"nt = {repr(nt)}")
            # print(f"2 {self.cmd_stack}")
            if nt == '\\':
                assert False
            elif diff > 1:
                assert False
            elif diff == 1:
                t = 'ie/indent'
                self.cur_indent += 1
            elif diff == 0:
                t = 'ie/newline'
                # print(f"3t = {repr(t)}")
            else:
                self.cur_indent += diff

                self.tokens.push_token("ie/newline")
                for j in range(abs(diff)):
                    self.tokens.push_token("ie/dedent")

                t = self.tokens.read_token()


        if t == '':
            ci = self.cur_indent
            assert ci == 0
        # print(f"t2 = {repr(t)}")
        return t


class ExpressionParser:
    def __init__(self):
        self.ip = None
        self.stack = []
        self.read_token = self.read_head
        self.special_forms = []


    def read_file(self, filename):
        self.ip = IndentParser(filename)


    def read_head(self):
        # breakpoint()
        # print("read_head")
        t = self.ip.read_token()
        # print(f"h {t= }")

        if t == '':
            assert not self.stack
            return ''
        elif t == 'ie/dedent':
            cmd = self.read_dedent()
            return cmd
        elif t == 'ie/newline':
            self.read_token = self.read_head
            if self.stack:
                prev_cmd = self.stack.pop()
                return prev_cmd
            assert False
        elif t.startswith('ie/'):
            assert False
        elif t in self.special_forms:
            end_t = 'end-' + t
            self.stack.append(end_t)
            self.read_token = self.read_body
        else:
            self.stack.append(t)
            self.read_token = self.read_body
            t = self.read_token()
        return t


    def read_body(self):
        # breakpoint()
        # print("read_body")
        t = self.ip.read_token()
        # print(f"b {t= }")
        if t == 'ie/indent':
            self.read_token = self.read_head
            t = self.read_token()
            return t
        elif t == 'ie/dedent':
            cmd = self.read_dedent()
            return cmd
        elif t == 'ie/newline':
            self.read_token = self.read_head
            prev_cmd = self.stack.pop()
            return prev_cmd
        elif t == '':
            assert not self.stack
            return ''
        else:
            return t


    def read_dedent(self):
        # breakpoint()
        # print(f"dedent")
        # print(f"dedent {self.stack =}")
        prev_cmd = self.stack.pop()
        self.read_token = self.read_head
        # print(f"{prev_cmd=}")
        return prev_cmd


def assemble(filename):
    global cur_indent

    rom = UxnRom()

    # ip = IndentParser(filename)
    # while True:
        # t = ip.read_token()
        # if t == '':
            # print("break")
            # break;
        # print('t', repr(t))

    xp = ExpressionParser()
    xp.special_forms.append('inline')
    xp.special_forms.append('org')
    xp.special_forms.append('label')
    xp.special_forms.append('sub-label')
    xp.special_forms.append('lit-addr')
    xp.special_forms.append('rel-addr-sub')
    xp.read_file(filename)

    inline_words = {}

    queue = []

    def next_word():
        if queue:
            return queue.pop(0)
        return xp.read_token()


    def read_until(end_marker):
        body = []
        while True:
            w = next_word()
            if w == end_marker:
                break
            elif w == '':
                break
            else:
                body.append(w)
        return body


    while True:
        w = next_word()
        # print(f'{w = }')

        if w == '':
            print("break")
            break;
        elif w in xp.special_forms:
            end_marker = f'end-{w}'
            body = read_until(end_marker)
            if w == 'inline':
                name, *body = body
                inline_words[name] = body
            elif w == 'org':
                offset, *body = body
                queue.extend(body)
                cmd = '|' + offset
                rom.write(cmd, 'set pc')
            elif w == 'label':
                name, *body = body
                queue.extend(body)
                cmd = f'@{name}'
                rom.write(cmd, 'label')
            elif w == 'sub-label':
                name, *body = body
                queue.extend(body)
                cmd = f'&{name}'
                rom.write(cmd, 'sub-label')
            elif w == 'lit-addr':
                name, *body = body
                queue.extend(body)
                cmd = f';{name}'
                rom.write(cmd, 'label')
            elif w == 'rel-addr-sub':
                name, *body = body
                queue.extend(body)
                cmd = f',&{name}'
                rom.write(cmd, 'label')
            else:
                assert False
        elif w in inline_words:
            body = inline_words[w]
            assert body
            queue.extend(body)
        elif w[0] == '"':
            s = w[1:-1]
            for b in bytes(s, 'ascii'):
                rom.write_byte(b)
            rom.write_byte(0)
        else:
            rom.write(w, 'asm')

    rom.resolve()

    # print(rom)

    with open('out.rom', 'wb') as f:
        f.write(rom.rom[0x100:])

    print("done")


def disassemble(filename):
    with open(filename, 'rb') as f:
        rom = bytearray(f.read())

    rom_iter = iter(rom)
    i = 0

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

        a = ' '.join(repr(chr(b)) for b in data)

        print(f"{i:04x} | {s:8} | {a:20} | {op:5} |")
        i += len(data)


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

