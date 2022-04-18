from rich.console import Console
from rich.traceback import install
from uxn import *
import argparse

console = Console(markup=False)
python_print = print
print = console.print

install(show_locals=True)

def disassemble(rom, offset=0x100, length=None):
    rom_iter = iter(rom[offset:])
    i = offset

    if length:
        pass
    else:
        length = len(rom) - offset

    remaining = length

    while True:
        if remaining == 0:
            break
        else:
            remaining -= 1

        try:
            b = next(rom_iter)
        except StopIteration:
            break
        data = [b]
        base_op_code = b & 0b00011111
        base_op = reverse_op_table[base_op_code]
        op = base_op
        short_mode = False

        if b & 0b10000000:
            op += 'k'
        if b & 0b01000000:
            op += 'r'
        if b & 0b00100000:
            short_mode = True
            op += '2'

        if base_op == 'lit':
            if short_mode:
                sep = ' '
                high = next(rom_iter)
                low  = next(rom_iter)
                n = (high << 8) + low
                data += [high, low]
                op = f"#{n:04x}"
            elif b & 0b10000000:
                n  = next(rom_iter)
                data += [n]
                op = f"#{n:02x}"
            else:
                op = 'brk'

        s = ' '.join(f"{b:02x}" for b in data)

        a = ' '.join(repr(chr(b)) for b in data)

        print(f"{i:04x} | {s:8} | {a:20} | {op:5} |")
        i += len(data)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="uxn tool")

    parser.add_argument("filename")

    args = parser.parse_args()

    with open(args.filename, 'rb') as f:
        rom = bytearray(0x100) + bytearray(f.read())

    disassemble(rom)

