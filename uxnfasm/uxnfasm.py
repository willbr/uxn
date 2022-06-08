import re
import uxn

#macros = "halt emit debug".split()
macros = {}
prog = re.compile(r"\s*(\S+)")
with open('one.fth') as f:
    data = f.read()

body = data
pending_word = None
rst = []

def next_word():
    global body
    m = prog.match(body)
    if not m:
        return None
    #print(m)
    #print(dir(m))
    #print(m.end())
    #print(repr(m.group(1)))
    body = body[m.end():]
    return m.group(1)

def peek_word():
    pass


def read_comment():
    #todo count depth for nested comments
    nw = next_word()
    body = []
    while nw != ')':
        body.append(nw)
        nw = next_word()
    s = ' '.join(body)
    print(f'( {s} )')


def read_macro(name):
    nw = next_word()
    assert nw == '{'
    nw = next_word()
    body = []
    while nw != '}':
        body.append(nw)
        nw = next_word()

    if name in macros:
        raise ValueError(f"Duplicate macro: {name}")

    macros[name] = body


counter = 0
def gensym(s):
    global counter
    name = f"gs-{s}-{counter}" 
    counter += 1
    return name

def is_uxntal(w):
    prefix_chars = '%:.;,&|$#~\'"'
    if uxn.is_op(w):
        return True
    elif w[0] in prefix_chars:
        return len(w) != 1
    elif w in ['{', '}', '[', ']']:
        return True

    return False

def format_uxntal(w):
    if uxn.is_op(w):
        lhs = w[:3].upper()
        rhs = w[3:].lower()
        f = lhs + rhs
        return f
    else:
        return w


def compile(w):
    if w == ':':
        name = next_word()
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
        rst.append(['do', loop_lbl, pred_lbl])
        print('  ( do )')
        print('  SWP2 STH2 STH2')
        print(f'  &{loop_lbl}')
    elif w == 'loop':
        header, loop_lbl, pred_lbl = rst[-1]
        assert header == 'do'
        print('  ( loop )')
        print('  INC2r')
        #print(f'&{pred_lbl}')
        print(f'  GTH2kr STHr ,&{loop_lbl} JCN')
        print('  POP2r POP2r')
        rst.pop()
    elif w == '+loop':
        assert False
    elif w == 'if':
        false_lbl = gensym('false')
        end_lbl  = gensym('end')
        rst.append(['if', false_lbl, end_lbl])
        print(f'  #00 EQU ,&{false_lbl} JCN')
    elif w == 'else':
        header, false_lbl, end_lbl = rst[-1]
        assert header == 'if'
        rst[-1][0] = 'else'
        print(f'  ,&{end_lbl} JMP')
        print(f'&{false_lbl}')
    elif w == 'endif':
        header, false_lbl, end_lbl = rst[-1]
        if header == 'if':
            print(f'  &{false_lbl}')
        elif header == 'else':
            print(f'  &{end_lbl}')
        else:
            assert False
        rst.pop()
    elif w == 'begin':
        begin_lbl = gensym('begin')
        end_lbl  = gensym('end-begin')
        rst.append(['begin', begin_lbl, end_lbl])
        print('  ( begin )')
        print(f'  &{begin_lbl}')
    elif w == 'while':
        header, begin_lbl, end_lbl  = rst[-1]
        assert header == 'begin'
        print(f'  #00 EQU ;&{end_lbl} JCN2')
    elif w == 'repeat':
        header, begin_lbl, end_lbl  = rst[-1]
        assert header == 'begin'
        print(f'  ;&{begin_lbl} JMP2')
        print(f'  &{end_lbl}')
        rst.pop()
    elif w == 'until':
        header, begin_lbl, end_lbl  = rst[-1]
        assert header == 'begin'
        print(f'  #00 EQU ;&{begin_lbl} JCN2')
        print(f'  &{end_lbl}')
        rst.pop()
    elif w == 'again':
        header, begin_lbl, end_lbl  = rst[-1]
        assert header == 'begin'
        print(f'  ;&{begin_lbl} JMP2')
        print(f'  &{end_lbl}')
        rst.pop()
    elif w == 'leave':
        header, begin_lbl, pred_lbl, end_lbl  = rst[-1]
        assert header == 'begin'
        print(f'  ;&{end_lbl} JMP2')
    elif w == '(':
        read_comment()
    elif w[0] == '%':
        read_macro(w[1:])
    elif w in macros:
        body = macros[w]
        for child in body:
            compile(child)
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


print('~header.tal')
# print(data)
while True:
    w = next_word()
    if not w:
        break
    compile(w)
print('~footer.tal')

