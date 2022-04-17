from subprocess import Popen, run, PIPE
from difflib import context_diff, unified_diff, ndiff, diff_bytes
from pathlib import Path
from binascii import hexlify
from textwrap import dedent
import unittest

from rich.console import Console

console = Console(markup=False)
python_print = print
print = console.print

test_folder = Path("./tests/asm/")
bin_folder = test_folder.joinpath("bin")





def build_op_tests():
    ops = dedent("""
    brk 0x00 a b c m[pc+1]
    lit 0x00
    inc 0x01
    pop 0x02
    dup 0x03
    nip 0x04
    swp 0x05
    ovr 0x06
    rot 0x07
    equ 0x08
    neq 0x09
    gth 0x0a
    lth 0x0b
    jmp 0x0c
    jcn 0x0d
    jsr 0x0e
    sth 0x0f
    ldz 0x10
    stz 0x11
    ldr 0x12
    str 0x13
    lda 0x14
    sta 0x15
    dei 0x16
    deo 0x17
    add 0x18
    sub 0x19
    mul 0x1a
    div 0x1b
    and 0x1c
    ora 0x1d
    eor 0x1e
    sft 0x1f
    """).strip()

    op_table = {}
    reverse_op_table = {}

    for line in ops.split('\n'):
        op, code, *comment = line.split(' ', 2)
        n = int(code, 16)
        op_table[op] = n
        reverse_op_table[n] = op

    for n, op in reverse_op_table.items():
        filename = f"op_{n:02x}_{op}.tal"
        file_path = test_folder.joinpath(filename)
        op = op.upper()
        body = dedent(f"""
        |0100
        #beef #cafe {op}2 {op}
        BRK
        """).strip()

        with open(file_path, 'w') as f:
            f.write(body)


if not test_folder.exists():
    test_folder.mkdir()

if not bin_folder.exists():
    bin_folder.mkdir()

def assemble_and_compare(test, tal_path):
    assert tal_path.exists()

    f1 = f"{tal_path.stem}_talie.rom"
    p1 = bin_folder.joinpath(f1)

    f2 = f"{tal_path.stem}_uxnasm.rom"
    p2 = bin_folder.joinpath(f2)

    talie(tal_path, p1)
    uxnasm(tal_path, p2)

    o1 = hexdump_file(p1)
    o2 = hexdump_file(p2)

    # print(o1)
    # print(o2)

    # diff = '\n'.join(unified_diff(o1, o2))
    # print(diff)
    # if diff != '':
        # err_msg = '\n'.join([
            # "\nFile:",
            # basename,
            # "\nInput:",
            # prog,
            # "\nExpected:",
            # ' '.join(er),
            # "\nOutput:",
            # ' '.join(out),
            # "\nDiff:",
            # diff,
            # ])
        # return err_msg
    # else:
        # # print("good: ", basename)
        # pass


    test.assertEqual(o1, o2)

class TestTalie(unittest.TestCase):
    def test_ops(self):
        for filename in test_folder.glob("op_*.tal"):
            assemble_and_compare(self, filename)

    def test_syntax(self):
        for filename in test_folder.glob("syntax_*.tal"):
            assemble_and_compare(self, filename)

def hexdump_file(filename):
    lines = []
    with open(filename, 'rb') as f:
        for block in iter(lambda: f.read(16), b''):
            line = hexlify(block, ' ', 1)
            lines.append(line.decode('ascii'))
    return '\n'.join(lines)


def talie(in_path, out_path):
    p = Popen(["python", "talie.py", in_path, out_path], stdin=PIPE, stdout=PIPE, stderr=PIPE)
    prog = ""
    out, err = [x.decode() for x in p.communicate(prog.encode())]

    # print(p.returncode)
    # print("Out:", out)
    # print("Err:", err)

    if p.returncode:
        raise ValueError(err)


def uxnasm(in_path, out_path):
    p = Popen(["uxnasm", in_path, out_path], cwd=in_path.parent, stdin=PIPE, stdout=PIPE, stderr=PIPE)
    prog = ""
    out, err = [x.decode() for x in p.communicate(prog.encode())]

    # print(p.returncode)
    # print("Out:", out)
    # print("Err:", err)

    if p.returncode:
        msg = dedent(f"""
        {in_path.name}
        Err:
        {err}
        """).strip()
        raise ValueError(msg)


if __name__ == '__main__':
    # build_op_tests()
    unittest.main()