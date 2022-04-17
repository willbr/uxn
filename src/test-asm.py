from subprocess import Popen, run, PIPE
from difflib import context_diff, unified_diff, ndiff, diff_bytes
from pathlib import Path
from binascii import hexlify
from textwrap import dedent
import unittest
import os

from rich.console import Console

console = Console(markup=False)
python_print = print
print = console.print

src_folder = Path(__file__).parent
test_folder = src_folder.joinpath("tests/asm")
bin_folder = Path("bin")
cur_dir = Path(".")

os.chdir(test_folder)


def assemble_and_compare(test, tal_path):
    print(f"{tal_path=}")
    assert tal_path.exists()

    f1 = f"{tal_path.stem}_talie.rom"
    p1 = bin_folder.joinpath(f1)

    f2 = f"{tal_path.stem}_uxnasm.rom"
    p2 = bin_folder.joinpath(f2)

    # print(f"{p1=}")
    # print(f"{p2=}")

    uxnasm(tal_path.name, p2)
    talie(tal_path.name, p1)

    o1 = hexdump_file(p1)
    o2 = hexdump_file(p2)

    test.assertEqual(o1, o2)


class TestTalie(unittest.TestCase):
    # def test_ops(self):
        # for filename in cur_dir.glob("op_*.tal"):
            # # print(filename)
            # assemble_and_compare(self, filename)

    def test_syntax(self):
        for filename in cur_dir.glob("syntax_*.tal"):
            # print(filename)
            assemble_and_compare(self, filename)


def hexdump_file(filename):
    lines = []
    with open(filename, 'rb') as f:
        for block in iter(lambda: f.read(16), b''):
            line = hexlify(block, ' ', 1)
            lines.append(line.decode('ascii'))
    return '\n'.join(lines)


def talie(in_path, out_path):
    p = Popen(["python", "../../../talie.py", in_path, out_path], stdin=PIPE, stdout=PIPE, stderr=PIPE)
    prog = ""
    # out, err = [x.decode() for x in p.communicate(prog.encode())]
    out, err = p.communicate(prog.encode('ascii'))
    out = out.decode()
    err = err.decode(errors='backslashreplace')
    # out, err = [x.decode() for x in p.communicate(prog.encode())]

    # print(p.returncode)
    # print("Out:", out)
    # print("Err:", err)

    if p.returncode:
        msg = dedent(f"""
        Out:
        {out}
        Err:
        {err}
        """).strip()
        raise ValueError(msg)


def uxnasm(in_path, out_path):
    p = Popen(["uxnasm", in_path, out_path], stdin=PIPE, stdout=PIPE, stderr=PIPE)
    prog = ""
    out, err = [x.decode() for x in p.communicate(prog.encode())]

    # print(p.returncode)
    # print("Out:", out)
    # print("Err:", err)

    if p.returncode:
        msg = dedent(f"""
        Err:
        {err}
        """).strip()
        raise ValueError(msg)


if __name__ == '__main__':
    unittest.main()

