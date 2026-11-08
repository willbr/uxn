import uxnfasm

pfp = uxnfasm.parse_fixed_point
fp2s = uxnfasm.fixed_point_to_string

def pn(n):
    s = uxnfasm.fixed_point_to_string(n)
    print(f"{n:016b} {s}")

tests = """
    0.0 1.0
    10.0 2.0
    100.0 2.0
    100.0 0.5
    200.0 200.0
    1022.0 0.5
    -1022.0 0.5
    1.0 2.0
    6.25 4.0
    2046.0 8.0
    1000.0 2000.0
""".strip().split('\n')

def pp(*args):
    sa, sb, sc = map(fp2s, args)
    print(f"{sa} / {sb} = {sc}")

def d1(a, b):
    c = int((a << 0) / (b << 0)) << 4
    pp(a,b,c)

def d2(a, b):
    c = int((a << 1) / (b << 1)) << 4
    pp(a,b,c)

def d3(a, b):
    c = (a << 5) / (b << 0)
    c = int(c)
    c = c >> 1
    pp(a,b,c)

def d4(a, b):
    c = (a << 4) / (b << 0)
    c = int(c)
    c &= 0xffff
    c = int(c) >> 10
    pp(a,b,c)

def d5(a, b):
    if a >= b:
        i = a << 0
        j = b << 0
        k = i / j
        l = int(k) & 0xffff
        c = l << 4
    else:
        i = a << 1
        j = b << 0
        k = i / j
        #print(i,j,k)
        l = int(k) & 0xffff
        c = l << 3
    pp(a,b,c)

for test in tests:
    a, b = map(pfp, test.split())
    print(f"{a/16} / {b/16} = {a/b:2.4f}")

    #d1(a, b)
    #d2(a, b)
    #d3(a, b)
    #d4(a, b)
    d5(a, b)

    print()

