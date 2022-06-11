#! /usr/bin/env python

# todo line comments starting with \
# todo incbin

from pathlib import Path

import re
import uxn
import sys

prog = re.compile(r"\s*(\)|\S+)")
prefix_chars = '%:.;,@&|$#~\'"'

class CompilationUnit():
    def __init__(self):
        self.macros = {}
        self.body = None
        self.rst = []
        pass

    def compile_file(self, filename):
        old_body = self.body
        old_rst = self.rst
        self.body = read_file(filename)

        while True:
            w = self.next_word()
            if not w:
                break
            self.compile(w)

        self.body = old_body
        self.rst = old_rst

    def next_word(self):
        m = prog.match(self.body)
        if not m:
            return None
        self.body = self.body[m.end():]
        return m.group(1)

    def compile(self, w):
        if w == ':':
            name = self.next_word()
            if name == 'init':
                print('|0100')
            print(f"@{name}")
        elif w == ';':
            print('JMP2r\n')
        elif w == 'brk;':
            print('BRK\n')
        elif w == 'do':
            loop_lbl = gensym('loop')
            pred_lbl = gensym('pred')
            self.rst.append(['do', loop_lbl, pred_lbl])
            print('  ( do )')
            print('  SWP2 STH2 STH2')
            print(f'  &{loop_lbl}')
        elif w == 'loop':
            header, loop_lbl, pred_lbl = self.rst[-1]
            assert header == 'do'
            print('  ( loop )')
            print('  INC2r')
            #print(f'&{pred_lbl}')
            print(f'  GTH2kr STHr ,&{loop_lbl} JCN')
            print('  POP2r POP2r')
            self.rst.pop()
        elif w == '+loop':
            assert False
        elif w == 'if':
            false_lbl = gensym('false')
            end_lbl  = gensym('end')
            self.rst.append(['if', false_lbl, end_lbl])
            print(f'  #00 EQU ,&{false_lbl} JCN')
        elif w == 'else':
            header, false_lbl, end_lbl = self.rst[-1]
            assert header == 'if'
            self.rst[-1][0] = 'else'
            print(f'  ,&{end_lbl} JMP')
            print(f'&{false_lbl}')
        elif w == 'endif':
            header, false_lbl, end_lbl = self.rst[-1]
            if header == 'if':
                print(f'  &{false_lbl}')
            elif header == 'else':
                print(f'  &{end_lbl}')
            else:
                assert False
            self.rst.pop()
        elif w == 'begin':
            begin_lbl = gensym('begin')
            end_lbl  = gensym('end-begin')
            self.rst.append(['begin', begin_lbl, end_lbl])
            print('  ( begin )')
            print(f'  &{begin_lbl}')
        elif w == 'while':
            header, begin_lbl, end_lbl  = self.rst[-1]
            assert header == 'begin'
            print(f'  #00 EQU ;&{end_lbl} JCN2')
        elif w == 'repeat':
            header, begin_lbl, end_lbl  = self.rst[-1]
            assert header == 'begin'
            print(f'  ;&{begin_lbl} JMP2')
            print(f'  &{end_lbl}')
            self.rst.pop()
        elif w == 'until':
            header, begin_lbl, end_lbl  = self.rst[-1]
            assert header == 'begin'
            print(f'  #00 EQU ;&{begin_lbl} JCN2')
            print(f'  &{end_lbl}')
            self.rst.pop()
        elif w == 'again':
            header, begin_lbl, end_lbl  = self.rst[-1]
            assert header == 'begin'
            print(f'  ;&{begin_lbl} JMP2')
            print(f'  &{end_lbl}')
            self.rst.pop()
        elif w == 'leave':
            header, begin_lbl, pred_lbl, end_lbl  = self.rst[-1]
            assert header == 'begin'
            print(f'  ;&{end_lbl} JMP2')
        elif w == 'tal':
            self.read_tal()
        elif w == '(':
            self.read_comment()
        elif w[0] == '%':
            self.read_macro(w[1:])
        elif w[0] == '~':
            self.compile_file(w[1:])
        elif w in self.macros:
            body = self.macros[w]
            for child in body:
                self.compile(child)
        elif is_uxntal(w):
            f = format_uxntal(w)
            print(f"  {f}")
        else:
            try:
                if w[:2] == '0x':
                    n = int(w[2:], 16)
                elif w[:2] == '0b':
                    n = int(w[2:], 2)
                else:
                    n = int(w, 10)

                if n < 0:
                    n = 0x10000 + n
                n &= 0xffff
                print(f"  #{n:04x}")
            except ValueError:
                print(f'  ;{w} JSR2')


    def read_tal(self):
        #todo count depth for nested comments
        w = self.next_word()
        while w != 'endtal':
            if w == '(':
                self.read_comment()
            else:
                print(w)
            w = self.next_word()

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
        print(s)

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


def format_uxntal(w):
    if uxn.is_op(w):
        lhs = w[:3].upper()
        rhs = w[3:].lower()
        f = lhs + rhs
        return f
    else:
        return w


def main(filename):
    script_dir = Path(__file__).resolve().parent
    header_path = script_dir.joinpath('header.tal')
    forth_path = script_dir.joinpath('forth.fth')
    #print(script_dir)
    #print(header_path)
    #print(forth_path)
    cu = CompilationUnit()

    print('~header.tal')
    #cu.compile_file(forth_path)
    cu.compile_file(filename)

if __name__ == '__main__':
    main(sys.argv[1])

