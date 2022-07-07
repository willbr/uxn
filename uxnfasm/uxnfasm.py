#! /usr/bin/env python

#from rich.console import Console
from rich.traceback import install
from pathlib import Path

import re
import uxn
import sys

#console = Console(markup=False)
#python_print = print
#print = console.print

install(show_locals=True)

token_prog = re.compile(r"\s*(\)|\S+)")
line_comment_prog = re.compile(r".*")
string_prog = re.compile(r".*?(?<!\\)\"")
fixed_prog = re.compile(r"(-|\+)?\d+\.\d+")

prefix_chars = '%:.;,@&|$#~\'"'

def eprint(s="", end="\n"):
    sys.stderr.write(s + end)

class CompilationUnit():
    def __init__(self):
        self.macros = {}
        self.variables = {}
        self.body = None
        self.rst = []
        self.current_word = None
        self.include_stdlib = True
        self.stdlib_included = False
        self.pending_token = None
        self.prev_word = None
        self.sep = ""
        self.depth = 0
        self.current_file = 0
        self.current_line = 0
        self.files = []
        self.line_pos = 0
        self.line_pos_stack = []

    def compile_file(self, filename):
        p = Path(filename)
        self.files.append(p)
        old_body = self.body
        old_rst = self.rst
        self.body = read_file(filename)

        self.sep = ""
        self.depth = 0

        self.line_pos_stack.append(self.line_pos)
        self.line_pos = 1

        while True:
            w = self.next_word(keep_newline=True)
            if not w:
                break
            self.compile(w)


        self.line_pos = self.line_pos_stack.pop()
        self.body = old_body
        self.rst = old_rst

        self.print('\n')

    def next_word(self, keep_newline=False):
        if self.pending_token:
            w = self.pending_token
            self.pending_token = None
            return w

        m = token_prog.match(self.body)
        if not m:
            return None
        self.body = self.body[m.end():]
        w = m.group(1)
        if '\n' in m.group():
            self.line_pos += m.group().count('\n')
        if keep_newline and '\n' in m.group():
            self.pending_token = w
            return '\n'
        return w

    def print(self, w):
        if self.prev_word == '\n' and w == '\n':
            pass
        elif w == '\n':
            indent = "    " * self.depth
            print()
            self.sep = indent
        else:
            print(self.sep + w, end="")
            self.sep = " "
        self.prev_word = w

    def compile_stdlib(self):
        self.stdlib_included = True
        print()
        self.compile_file(self.forth_path)

    def compile(self, w):
        if w == '\n':
            self.print(w)
            return

        if w == ':':
            self.depth += 1
            self.sep = ""
            name = self.next_word()
            if name == 'init':
                self.print('|0100')
                assert self.current_word == None
            if self.current_word == 'init' and self.include_stdlib and self.stdlib_included == False:
                self.current_word = name
                self.compile_stdlib()
            else:
                self.current_word = name
            self.print(f"@{name}")
        elif w == ';':
            self.depth -= 1
            self.print('JMP2r\n')
            if self.current_word == 'init':
                raise ValueError('init must be closed with brk;')
        elif w == 'brk;':
            self.depth -= 1
            self.print('BRK\n')
        elif w == 'do':
            self.depth += 1
            loop_lbl = gensym('loop')
            pred_lbl = gensym('pred')
            self.rst.append(['do', loop_lbl, pred_lbl])
            self.print('( do )')
            self.print('SWP2 STH2 STH2')
            self.print(f';&{pred_lbl} JMP2')
            self.print(f'&{loop_lbl}')
        elif w == 'loop' or w == '+loop':
            header, loop_lbl, pred_lbl = self.rst[-1]
            assert header == 'do'
            if w == 'loop':
                self.print('( loop )')
                self.print('INC2r')
            else:
                self.print('( loop )')
                self.print('STH2 ADD2r')
            self.print(f'&{pred_lbl}')
            self.print(f'GTH2kr STHr ;&{loop_lbl} JCN2')
            self.print('POP2r POP2r')
            self.rst.pop()
            self.depth -= 1
        elif w in ['if', 'if1']:
            false_lbl = gensym('false')
            end_lbl  = gensym('end')
            self.rst.append(['if', false_lbl, end_lbl])
            if w == 'if':
                self.print(f'( if ) #0000 EQU2 ;&{false_lbl} JCN2')
            else:
                self.print(f'( if1 ) #00 EQU ;&{false_lbl} JCN2')
            self.depth += 1
        elif w == 'else':
            self.print('( else )')
            header, false_lbl, end_lbl = self.rst[-1]
            assert header == 'if'
            self.rst[-1][0] = 'else'
            self.print(f';&{end_lbl} JMP2')
            self.print(f'&{false_lbl}')
        elif w == 'endif':
            self.print('( endif )')
            header, false_lbl, end_lbl = self.rst[-1]
            if header == 'if':
                self.print(f'&{false_lbl}')
            elif header == 'else':
                self.print(f'&{end_lbl}')
            else:
                assert False
            self.rst.pop()
            self.depth -= 1
        elif w == 'begin':
            self.depth += 1
            begin_lbl = gensym('begin')
            end_lbl  = gensym('end-begin')
            self.rst.append(['begin', begin_lbl, end_lbl])
            self.print('( begin )')
            self.print(f'&{begin_lbl}')
        elif w in ['while', 'while1']:
            header, begin_lbl, end_lbl  = self.rst[-1]
            assert header == 'begin'
            if w == 'while':
                self.print(f'( while ) #0000 EQU2 ;&{end_lbl} JCN2')
            else:
                self.print(f'( while ) #00 EQU ;&{end_lbl} JCN2')
        elif w == 'repeat':
            header, begin_lbl, end_lbl  = self.rst[-1]
            assert header == 'begin'
            self.print('( repeat )')
            self.print(f';&{begin_lbl} JMP2')
            self.print(f'&{end_lbl}')
            self.rst.pop()
            self.depth -= 1
        elif w == 'until':
            header, begin_lbl, end_lbl  = self.rst[-1]
            assert header == 'begin'
            self.print(f'( until ) #0000 EQU2 ;&{begin_lbl} JCN2')
            self.print(f'&{end_lbl}')
            self.rst.pop()
            self.depth -= 1
        elif w == 'again':
            header, begin_lbl, end_lbl  = self.rst[-1]
            assert header == 'begin'
            self.print('( again )')
            self.print(f';&{begin_lbl} JMP2')
            self.print(f'&{end_lbl}')
            self.rst.pop()
            self.depth -= 1
        elif w == 'leave':
            header, begin_lbl, pred_lbl, end_lbl  = self.rst[-1]
            assert header == 'begin'
            self.print(f'( leave ) ;&{end_lbl} JMP2')
        elif w == 'case':
            #self.depth += 1
            end_lbl  = gensym('end-case')
            next_lbl  = gensym('next-case')
            self.rst.append(['case', next_lbl, end_lbl, False])
            self.print('( case )')
        elif w == 'of':
            self.depth += 1
            header, next_lbl, end_lbl, otherwise  = self.rst[-1]
            assert header == 'case'
            self.print('\n')
            self.print('( of )')
            self.print(f'OVR2 NEQ2 ;&{next_lbl} JCN2 POP2')
        elif w == 'endof':
            header, old_next_lbl, end_lbl, otherwise  = self.rst[-1]
            assert header == 'case'
            next_lbl  = gensym('next-case')
            self.rst[-1][1] = next_lbl
            self.depth -= 1
            self.print('\n')
            self.print('( endof2 )')
            self.print(f';&{end_lbl} JMP2')
            self.print('\n')
            self.print(f'&{old_next_lbl}')
        elif w == 'endcase':
            header, next_lbl, end_lbl, otherwise  = self.rst[-1]
            assert header == 'case'
            if otherwise == False:
                self.print('\n')
                self.print('POP2')
            self.print('\n')
            self.print(f'&{end_lbl}')
            self.rst.pop()
            #self.depth -= 1
        elif w == 'otherwise':
            header, next_lbl, end_lbl, otherwise  = self.rst[-1]
            assert header == 'case'
            assert otherwise == False
            self.rst[-1][3] = True
            self.print('\n')
            self.print(f'POP2')
        elif w == 'tal':
            self.read_tal()
        elif w == 'incbin':
            self.read_binary_file()
        elif w == 'variable':
            name = self.next_word()
            self.create_variable(name, 2)
        elif w == 'array':
            name = self.next_word()
            size = int(self.next_word()) * 2
            self.create_variable(name, size)
        elif w == 'sprite-1bpp':
            w = self.next_word()
            width = self.parse_number(w)
            w = self.next_word()
            height = self.parse_number(w)
            self.compile_sprite_1bpp(width, height)
        elif w == '(':
            self.read_comment()
        elif w == '\\':
            self.read_line_comment()
        elif w == 'no-stdlib':
            self.include_stdlib = False
        elif w == 'ere':
            self.compile_ere()
        elif w == 'raw':
            self.compile_raw_number()
        elif w == 'raw-byte':
            self.compile_raw_byte()
        elif w[0] == '"':
            self.read_string(w[1:])
        elif w[0] == '%':
            self.read_macro(w[1:])
        elif w[0] == '~':
            self.compile_file(w[1:])
        elif w in self.macros:
            self.print("\n")
            self.print(f"( {w} )")
            body = self.macros[w]
            for child in body:
                self.compile(child)
        elif is_uxntal(w):
            self.print(f"{w}")
        elif w in self.variables:
            self.print(f';{w}')
        else:
            try:
                n = self.parse_number(w)
                self.print(f"#{n:04x}")
            except ValueError:
                self.print(f';{w} JSR2')


    def parse_number(self, w):
        if w[:2] == '0x':
            n = int(w[2:], 16)
        elif w[:2] == '0b':
            n = int(w[2:], 2)
        elif fixed_prog.match(w):
            n = parse_fixed_point(w)
        else:
            n = int(w, 10)

        if n < 0:
            n += 0x10000

        n &= 0xffff
        return n


    def read_tal(self):
        #todo count depth for nested comments
        w = self.next_word()
        while w != 'endtal':
            if w == '(':
                self.read_comment()
            else:
                self.print(w)
            w = self.next_word()


    def read_binary_file(self):
        filename = self.next_word()
        with open(filename, 'rb', buffering=4) as f:
            while True:
                line = f.read(16)
                if len(line) == 0:
                    break
                tal = ' '.join(f"{c:02x}" for c in line)
                self.print(tal)


    def read_line_comment(self):
        m = line_comment_prog.match(self.body)
        if not m:
            return None
        self.body = self.body[m.end():]
        comment = m.group().strip()
        self.print(f"( {comment} )")


    def read_comment(self):
        depth = 1
        body = ['(']
        while True:
            w = self.next_word()
            body.append(w)

            if w == '(':
                depth += 1
            elif w == ')':
                depth -= 1
            elif w == None:
                break

            if depth == 0:
                break

        s = ' '.join(body)
        self.print(s)

    def read_macro(self, name):
        nw = self.next_word()
        assert nw == '{'
        nw = self.next_word()
        body = []
        while nw != '}':
            body.append(nw)
            nw = self.next_word()

        if name in self.macros:
            raise ValueError(f"Duplicate macro: {name}")

        self.macros[name] = body

    def create_variable(self, name, size):
        if name in self.variables:
            raise ValueError(f"Duplicate variable: {name}")

        if is_uxntal(name):
            raise ValueError(f"Invalid varialbe name: {name}, it looks like uxntal.")

        self.variables[name] = size

    def compile_variables(self):
        for name, size in self.variables.items():
            self.print(f"@{name} ${size}")
            self.print('\n')

    def compile_sprite_1bpp(self, width, height):
        assert width % 8 == 0
        assert height % 8 == 0
        w = width // 8
        h = height // 8
        #eprint(f"{w}, {h}")

        rows = []

        for i in range(height):
            word = self.next_word()
            s = word
            assert len(s) == width
            s = re.sub('\.', '0', s)
            s = re.sub('[^0]', '1', s)
            #eprint(s)
            chunks = [s[i:i+8] for i in range(0, len(s), 8)]
            row = [int(c, 2) for c in chunks]
            rows.append(row)

        assert len(rows) == height

        if (w,h) == (1,1):
            for i in range(8):
                n = rows[i][0]
                self.print(f"{n:02x}")
            self.print("\n")
        elif (w,h) == (2,2):
            for i in range(8):
                n = rows[i][0]
                self.print(f"{n:02x}")
            self.print("\n")
            for i in range(8):
                n = rows[i][1]
                self.print(f"{n:02x}")
            self.print("\n")
            for i in range(8,16):
                n = rows[i][0]
                self.print(f"{n:02x}")
            self.print("\n")
            for i in range(8,16):
                n = rows[i][1]
                self.print(f"{n:02x}")
            self.print("\n")
        else:
            assert False


    def read_string(self, w):
        if w[-1] == '"':
            s = w[:-1]
        else:
            m = string_prog.match(self.body)
            if not m:
                raise ValueError("failed to find end of string")
            s = w + m.group()[:-1]
            self.body = self.body[m.end():]

        s = s.replace(r'\"', '"')
        s = s.replace(r'\e', '\u001b')

        ss = ' 20 '.join('"' + elem for elem in s.split())
        self.print(f"{ss} 00")

    def compile_ere(self):
        fn = f";_file{len(self.files)}"
        line_pos = f"#{self.line_pos:04x}"
        print(f"{fn} {line_pos} ;print-ere JSR2")

    def compile_debug_info(self):
        for i, p in enumerate(self.files, 1):
            self.print(f"@_file{i}")
            s = p.name + '"'
            #raise ValueError(s)
            self.read_string(s)
            self.print('\n')

    def compile_raw_number(self):
        w = self.next_word()
        n = self.parse_number(w)
        self.print(f"{n:04x}")

    def compile_raw_byte(self):
        w = self.next_word()
        n = self.parse_number(w)
        assert n <= 0xff
        self.print(f"{n:02x}")


def read_file(filename):
    with open(filename) as f:
        return f.read()


counter = 0
def gensym(s):
    global counter
    name = f"gs-{s}-{counter}" 
    counter += 1
    return name


def is_uxntal(w):
    if uxn.is_op(w):
        return True
    elif w[0] in prefix_chars:
        return len(w) != 1
    elif w in ['{', '}', '[', ']']:
        return True
    else:
        return False


def parse_fixed_point(s):
    """
    Q11.4
    binary:
    0 000 0000 0000 0000
    ^ ^             ^ fractional part
    | | integer part
    |
    | sign bit
    """

    lhs, rhs = s.split('.')

    i = abs(int(lhs))
    #eprint(f"{i}")
    i <<= 4

    f = float('0.' + rhs) * 16
    f = int(f) & 0b1111

    #eprint(f"{i} {f:04b}")

    if lhs[0] == '-':
        n = 0x10000 - i - f
    else:
        n = i + f

    n &= 0xffff

    #eprint(f"{s:>7s} : {n:>5} : {n:016b} : ", end="")
    #print_fixed_point(n)

    return n


def fixed_point_to_string(n):
    i = n & 0xfff0
    i >>= 4
    if i >= 0x7ff:
        i -= 0x1000
        pass

    f = n & 0b0000_0000_0000_1111
    f *= 625
    f /= 10000

    j = i + f

    s = f"{j}"
    return s


def print_fixed_point(n):
    s = fixed_point_to_string(n)
    eprint(s)


def main(filename):
    script_dir = Path(__file__).resolve().parent
    header_path = script_dir.joinpath('header.tal')
    footer_path = script_dir.joinpath('footer.tal')
    #print(script_dir)
    #print(header_path)
    #print(forth_path)
    cu = CompilationUnit()
    cu.forth_path = script_dir.joinpath('forth.fth')

    cu.compile_file(header_path)
    cu.compile_file(filename)
    if cu.include_stdlib and cu.stdlib_included == False:
        cu.compile_stdlib()
    cu.compile_variables()
    cu.print('\n')
    cu.compile_debug_info()
    cu.compile_file(footer_path)
    cu.print('\n')


if __name__ == '__main__':
    #n = 0b1111_1111_1111_1111
    #n = 0b1000_0000_0000_0001
    #eprint(-0.0625)
    #print_fixed_point(n)
    main(sys.argv[1])

