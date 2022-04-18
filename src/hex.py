from binascii import hexlify
import argparse

from rich.console import Console

console = Console(markup=False)
python_print = print
print = console.print

parser = argparse.ArgumentParser(description="hexdump")
parser.add_argument("input")
args = parser.parse_args()

with open(args.input, 'rb') as f:
    for block in iter(lambda: f.read(16), b''):
        print(hexlify(block, ' ', 1))

