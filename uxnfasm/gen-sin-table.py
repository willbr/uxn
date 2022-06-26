from math import sin
from sys import argv

with open(argv[1], 'w') as f:
    f.write("@sin-table\n")

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

