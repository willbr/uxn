import re

macros = "halt emit debug".split()
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
    while nw != ')':
def read_macro():
    body = []
    name = next_word()
    macros.append(name)
    nw = next_word()
    while nw != 'end-macro':
        body.append(nw)
        nw = next_word()
    s = ' '.join(body)
    print(f'%{name} {{ {s} }}')

counter = 0
def gensym(s):
    global counter
    name = f"gs-{s}-{counter}" 
    counter += 1
    return name

print('~header.tal')
# print(data)
while True:
    w = next_word()
    if not w:
        break
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
        print('  STH2')
        print(f'  &{loop_lbl}')
    elif w == 'loop':
        header, loop_lbl, pred_lbl = rst[-1]
        assert header == 'do'
        print('  ( loop )')
        print('  INCr')
        #print(f'&{pred_lbl}')
        print(f'  GTHkr STHr ,&{loop_lbl} JCN')
        print('  POP2r')
        rst.pop()
    elif w == '+loop':
        assert False
    elif w == '(':
        read_comment()
    elif w == 'macro':
        read_macro()
    elif w[0] == '$':
        assert False
    elif w[0] == '%':
        assert False
    elif w in macros:
        print(f"  {w}")
    else:
        try:
            n = int(w)
            print(f"  #{n:02x}")
        except ValueError:
            print(f'  ;{w} JSR2')

print('~footer.tal')

