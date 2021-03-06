# https://wiki.xxiivv.com/site/varvara.html

# https://wiki.xxiivv.com/site/uxntal.html
# https://wiki.xxiivv.com/site/uxntal_cheatsheet.html
# https://wiki.xxiivv.com/site/uxntal_reference.html
# https://wiki.xxiivv.com/site/uxntal_stacking.html
# https://wiki.xxiivv.com/site/uxntal_macros.html

from rich.console import Console
from rich.traceback import install
from uxn import *

import fileinput
import argparse

console = Console(markup=False)
python_print = print
print = console.print

install(show_locals=True)

gensym_counter =0

class UxnRom():
    def __init__(self, filename=None):
        self.base = 16
        self.pc = 0
        self.scope = None
        self.refs = []
        self.labels = {}
        self.debug = False
        self.litlast = False

        if filename:
            self.load_rom(filename)
        else:
            self.rom = bytearray()


    def load_rom(self, filename):
        with open(filename, 'rb') as f:
            self.rom = bytearray(0x100) + bytearray(f.read())


    def __repr__(self):
        #return 'Rom: ' + ' '.join(f'{c:02x}' for c in self.rom[0x100:])
        return f"<Rom 0x{len(self.rom):02x}>"


    def write(self, token, note=''):
        if note and self.debug:
            print(f"{note:6s} {token}")

        first_char = token[:1]

        if first_char == '~':
            filename = token[1:]
            with open(filename) as f:
                data = f.read()
            assemble(self, data)
        elif first_char == '|':
            n = int(token[1:], self.base)
            assert n < 0x10000
            self.pc = n
            self.litlast = False
        elif first_char == '$':
            n = int(token[1:], 16)
            self.pc += n
            self.litlast = False
        elif first_char == '@':
            label_name = token[1:]
            self.make_label(label_name)
            self.scope = label_name
            self.litlast = False
        elif first_char == '&': # sub-label define
            assert self.scope != None
            sub_name = token[1:]
            self.make_label(self.sub_label(sub_name))
            self.litlast = False
        elif first_char == '#':
            n = int(token[1:], self.base)
            assert n >= 0
            assert n <= 0xffff
            if len(token) >= 4:
                self.write_lit_short(n)
            else:
                self.write_lit_byte(n)
        elif first_char == '.': # zero-page address
            self.make_reference(token, self.pc)
            self.write_lit_byte(0xff)
        elif first_char == ',': # literal address relative
            self.make_reference(token, self.pc)
            self.write_lit_byte(0xff)
        elif first_char == ';': # literal address absolute
            self.make_reference(token, self.pc)
            self.write_lit_short(0xffff)
        elif first_char == ':': # raw address absolute
            self.make_reference(token, self.pc)
            self.write_short(0xffff)
        elif first_char == "'":
            assert len(token) == 2
            c = token[1]
            n = ord(c)
            self.write_byte(n)
        elif first_char == '"':
            for b in bytes(token[1:], 'ascii'):
                self.write_byte(b)
        elif token[:3].lower() in op_table:
            self.write_op(token)
        else:
            n = int(token, self.base)
            assert n >= 0
            assert n <= 0xffff
            if len(token) >= 4:
                self.write_short(n)
            else:
                self.write_byte(n)


    def sub_label(self, name):
        label_name = f"{self.scope}/{name}"
        return label_name


    def make_label(self, label_name):
        assert label_name not in self.labels
        self.labels[label_name] = self.pc


    def make_reference(self, label, raw_addr):
        rune = label[0]
        if rune in ",." and self.litlast:
            addr = raw_addr - 1
        else:
            addr = raw_addr

        if label[1] == '&':
            ref_name = self.sub_label(label[2:])
        else:
            ref_name = label[1:]
        self.refs.append([ref_name, rune, addr])


    def write_byte(self, n):
        self.poke8(self.pc, n)
        self.pc += 1
        self.litlast = False


    def write_signed_byte(self, n):
        assert n >= -128
        assert n <= 127
        u = (n + 256) & 0xff
        self.write_byte(u)


    def write_short(self, n):
        assert n >= 0
        assert n <= 0xffff

        low  = n & 0x00ff
        high = n >> 8

        self.write_byte(high)
        self.write_byte(low)


    def write_lit_byte(self, n):
        if self.litlast: # combine literals
            hb = self.peek8(self.pc - 1)
            self.pc -= 2
            i = (hb << 8) + n
            self.write_lit_short(i)
            return
        self.write_op('lit')
        self.write_byte(n)
        self.litlast = True


    def write_lit_short(self, n):
        self.write_op('lit2')
        self.write_short(n)


    def write_op(self, op):
        op = op.lower()
        lhs, rhs = op[:3], op[3:]
        if lhs == 'lit': # force keep for lit
            if 'k' not in rhs:
                rhs += 'k'
        code = op_table[lhs]
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
            try:
                label_addr = self.labels[label]
            except KeyError:
                print(self.labels)
                print(f"unknown label: {repr(label)}")
                exit(1)
            # print(label, label_addr)
            # print(rune, ref_addr)
            if rune == '.':
                # assert 0x00 <= label_addr  <= 0xff
                n = label_addr & 0xff
                self.pc = ref_addr + 1
                self.write_byte(n)
            elif rune == ',':
                delta = label_addr - ref_addr - 3
                # print(f"{label_addr=:02x}")
                # print(f"{ref_addr=}")
                # print(f"{delta=}")
                self.pc = ref_addr + 1
                self.write_signed_byte(delta)
            elif rune == ';':
                self.pc = ref_addr + 1
                self.write_short(label_addr)
            elif rune == ':':
                self.write_short(label_addr)
            else:
                assert False


    def write_file(self, filename):
        with open(filename, 'wb') as f:
            f.write(self.rom[0x100:])


    def peek(self, short_mode, offset):
        if short_mode:
            peek = self.peek16
        else:
            peek = self.peek8

        peek = self.peek16 if short_mode else self.peek8
        n = peek(offset)
        return n

    def peek8(self, offset):
        n = self.rom[offset]
        return n

    def peek16(self, offset):
        high = self.rom[offset]
        low  = self.rom[offset+1]
        n = (high << 8) + low
        return n

    def poke(self, short_mode, offset, n):
        if short_mode:
            self.poke16(offset, n)
        else:
            self.poke8(offset, n)

    def poke8(self, offset, n):
        assert n >= 0
        assert n <= 0xff
        delta = offset - len(self.rom) + 1
        if delta > 0:
            self.rom += bytes(delta)
        self.rom[offset] = n

    def poke16(self, offset, n):
        high = n >> 8
        low  = n & 0x00ff

        self.poke8(offset, high)
        self.poke8(offset+1, low)


class Tokeniser:
    def __init__(self, data):
        self.i = 0
        self.queued_tokens = []
        self.data = data
        self.whitespace = ' \t\n'
        self.break_chars = self.whitespace + "(){}"
        self.chomp_whitespace()
        self.parse_alt_strings = False


    def chomp_whitespace(self):
        try:
            while self.data[self.i] in self.whitespace:
                self.i += 1
        except IndexError:
            pass


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
            # print(f"{c=}")

            if c in self.whitespace:
                while self.data[self.i] in self.whitespace:
                    self.i += 1
            elif c == '"' and self.parse_alt_strings:
                self.i += 1
                while self.data[self.i] not in '"':
                    self.i += 1
                self.i += 1
            elif c in '{}()':
                self.i += 1
            else:
                while self.data[self.i] not in self.break_chars:
                    self.i += 1
        except IndexError:
            pass

        t = self.data[start_pos:self.i]

        self.chomp_whitespace()

        if t.startswith('\n'):
            return '\n'

        assert t != '\t'

        return t


def assemble(rom, data):

    preview_tokens = False
    if preview_tokens:
        tok = Tokeniser(data)
        while True:
            t = tok.read_token()
            if t == '':
                break
            print(t)
        return

    xp = Tokeniser(data)

    inline_words = {}
    types = {}
    words = []
    queue = []

    var_ptr  = 0xc000

    def next_word():
        if queue:
            return queue.pop(0)
        return xp.read_token()


    def peek_word():
        if queue:
            return queue[0]

        t = xp.read_token()
        queue.append(t)
        return t


    def read_block(skip_open=False, open_marker='{', close_maker='}'):
        depth = 0
        body = []

        if not skip_open:
            open_word = next_word()
            assert open_word == open_marker

        while True:
            w = next_word()
            if w == open_marker:
                body.append(w)
                depth += 1
            elif w == close_maker:
                depth -= 1
                if depth == -1:
                    break
                else:
                    body.append(w)
            elif w == '':
                break
            else:
                body.append(w)

        assert depth == -1
        return body


    def assemble_label(prefix, name):
        nonlocal queue
        p = peek_word()
        if p == '{':
            body = read_block()
            queue = body + queue + ['label', f"end-{name}"]
            # print(f"{name} {body = }")

        cmd = f'{prefix}{name}'
        rom.write(cmd, 'label')


    def sizeof(type_name):
        spec = types[type_name]
        n = 0
        for property_name, property_size in spec:
            n += property_size
        return n


    blocks = []
    while True:
        w = next_word()
        # print(f"{w=}")
        if w == '':
            break;

        first_char = w[0]

        if w == '(':
            comment = read_block(True, '(', ')')
            pass
        elif w in '{[':
            blocks.append(w)
        elif w in '}':
            open_word = blocks.pop()
            assert open_word == '{'
        elif w in ']':
            open_word = blocks.pop()
            assert open_word == '['
        elif w == 'allot':
            allot_size = next_word()
            n = int(allot_size, rom.base)
            var_ptr += n
        elif w == 'array':
            count = next_word()
            n = int(count, rom.base)
            type_name = next_word()
            type_size = sizeof(type_name)
            array_size = n * type_size
            body = ['allot', array_size]
        elif w == 'type':
            name = next_word()
            body = read_block()
            spec = []
            while body:
                property_name, property_size, *body = body
                n = int(property_size, rom.base)
                spec.append([property_name, n])
            types[name] = spec
        elif w == 'inline' or first_char == '%':
            if first_char == '%':
                name = w[1:]
            else:
                name = next_word()
            body = read_block()
            inline_words[name] = body
        elif w == 'origin':
            offset = next_word()
            cmd = '|' + offset
            rom.write(cmd, 'set pc')
        elif w == 'comment':
            body = read_block()
        elif w == "data":
            name = next_word()
            body = read_block()
            cmd = '@' + name
            rom.write(cmd, 'data label')
            for b in body:
                n = int(b, rom.base)
                rom.write_byte(n)
        elif w == "loop":
            pw = peek_word()
            if pw == '{':
                start_marker = gensym('loop-start')
                end_marker = gensym('loop-end')
            else:
                start_marker = next_word()
                end_marker = start_marker + '-end'
            cmd = '&' + start_marker
            body = [cmd]
            body += read_block()
            cmd = ';&' + start_marker
            body += [cmd, 'jmp2']
            cmd = '&' + end_marker
            body += [cmd]
            queue = body + queue
        elif w == "if":
            p = peek_word()
            true_clause = read_block()
            assert true_clause
            p = peek_word()
            true_marker = gensym('if-true')
            end_marker   = gensym('if-end')
            if p == 'else':
                _ = next_word()
                else_clause = read_block()

                body = [';&' + true_marker, 'jcn2']
                body += else_clause
                body += [';&' + end_marker, 'jcn2']
                body += ['&' + true_marker]
                body += true_clause
                body += ['&' + end_marker]
            else:
                body = ['#00', 'equ']
                body += [';&' + end_marker, 'jcn2']
                body += true_clause
                body += ['&' + end_marker]
            queue = body + queue
        elif w == "vector":
            name = next_word()
            body = read_block() + ['brk']
            words.append(name)
            cmd = '@' + name
            rom.write(cmd, 'vector def')
            queue = body + queue
        elif w == "word":
            name = next_word()
            body = read_block() + ['jmp2r']
            words.append(name)
            cmd = '@' + name
            rom.write(cmd, 'word def')
            queue = body + queue
        elif w == "incbin":
            name = next_word()
            offset = int(next_word(), self.base)
            length = int(next_word(), self.base)
            assert name[0] == '"'
            assert name[-1] == '"'
            name = name[1:-1]
            with open(name, 'rb') as f:
                if offset:
                    f.seek(offset)
                if length:
                    data = f.read(length)
                else:
                    data = f.read()
            for b in data:
                rom.write_byte(b)
        elif w == 'binary':
            rom.base = 2
        elif w == 'decimal':
            rom.base = 10
        elif w == 'hex':
            rom.base = 16
        elif w in types:
            name = next_word()
            assert not is_syntax(name)
            spec = types[w]
            var_pad = f"|{var_ptr:04x}"
            body = [var_pad ,'@' + name, '{']
            for property_name, property_size in spec:
                var_ptr += property_size
                lbl = f"&{property_name}"
                pad = f"${property_size}"
                body += [lbl, pad]
            body += ['}']
            queue = body + queue
        elif w in inline_words:
            body = inline_words[w]
            assert body
            queue = body + queue
        elif w in words:
            label_addr = rom.labels[w]
            delta = label_addr - rom.pc - 1
            if -128 <= delta <= 127:
                cmd = ',' + w
                rom.write(cmd, 'word call')
                rom.write('jsr', 'word call')
            else:
                cmd = ';' + w
                rom.write(cmd, 'word call')
                rom.write('jsr2', 'word call')
        elif is_syntax(w):
            rom.write(w, 'syntax')
        elif is_op(w):
            rom.write(w, 'op')
        else:
            try:
                n = int(w, rom.base)
                rom.write(w, 'asm')
            except ValueError:
                cmd = ';' + w
                rom.write(cmd, 'word call')
                rom.write('jsr2', 'word call')

    assert not blocks


def gensym(name=None):
    global gensym_counter
    if not name:
        name = "g-"
    sym_name = f"{name}-{gensym_counter:x}"
    gensym_counter += 1
    return sym_name


def pprint_tokens(l):
    a = [t + '\n' if is_op(t) else t  + ' ' for t in l]
    print(''.join(a))


def assemble_file(in_file, out_file):
    with open(in_file) as f:
        data = f.read()

    rom = UxnRom()
    assemble(rom, data)
    rom.resolve()

    rom.write_file(out_file)

    return rom


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="uxn tool")

    parser.add_argument("input")
    parser.add_argument("output")
    parser.add_argument("--print-labels", action='store_true')

    args = parser.parse_args()

    assemble_file(args.input, args.output)

    if args.print_labels:
        print(rom.labels)


