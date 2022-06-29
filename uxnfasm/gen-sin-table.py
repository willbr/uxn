from math import sin, pi
from sys import argv

with open(argv[1], 'w') as f:
    f.write("@fsin-table\n")

    step = 1/16

    arc1 = [(i, i*step, sin(i*step)) for i in range(26)]

    arc2 = list(reversed(arc1[:-1]))

    arc3 = arc1 + arc2

    arc4 = [(i,-j,-k) for i,j,k in arc3]

    arc5 = arc3 + arc4[1:-1]

    for i,j,k in arc5:
        l = round(k, 4)
        s = f"( {i:2} {j:+2.4f} {k:+2.4f} ) raw {l:+2.4f}\n"
        f.write(s)

    f.write('\n')
    f.write("@sin-table\n")

    two_pi = pi * 2
    step = two_pi / 256
    for i in range(256):
        j = sin(i*step)
        k = int(j * 127)
        if k < 0:
            l = k + 0x100
            l &= 0xff
        else:
            l = k
        s = f"( {i:3d} {j:+2.2f} {k:+04d} ) raw-byte 0x{l:02x}\n"
        f.write(s)

