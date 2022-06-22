from math import sin
from sys import argv

with open(argv[1], 'w') as f:
    f.write("@sin-table\n")
    j = -1
    step = 0.0625
    for i in range(33):
        s = f"( {i:2} ) raw {j}\n"
        #print(s, end="")
        f.write(s)
        j += step

