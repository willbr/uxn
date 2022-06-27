import uxnfasm

pfp = uxnfasm.parse_fixed_point

def pn(n):
    s = uxnfasm.fixed_point_to_string(n)
    print(f"{n:016b} {s}")



tests = """
    0.0 0.0
    10.0 0.0
    10.5 2.0
    1.0 1.0
    1.0 0.5
    1.0 0.25
    1.0 0.125
    1.0 0.0625
    2046.0 1.0625
    2046.0 0.5
    10.5 100.0
""".strip().split('\n')

for test in tests:
    a, b = map(pfp, test.split())
    pn(a)
    pn(b)
    print('-'*30)

    c = ((a >> 1) * (b >> 1)) >> 2
    pn(c)

    c = ((a >> 2) * (b >> 2)) << 0
    pn(c)

    c = ((a >> 3) * (b >> 3)) << 2
    pn(c)

    print()

