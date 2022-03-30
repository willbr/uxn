from uxn import *

import talie
import uxndis 


def decode_byte(b):
    opcode = b & 0b00011111
    op = reverse_op_table[opcode]


    modes = b & 0b11100000

    keep_mode = b & 0b10000000
    return_mode = b & 0b01000000
    short_mode = b & 0b00100000

    if op == 'lit' and not keep_mode:
        op = 'brk'

    return keep_mode, return_mode, short_mode, op, modes, opcode


class Stack:
    def __init__(self):
        self.reset()

    def reset(self):
        self.stack = bytearray(256)
        self.i = 0

    def pop(self, short_mode):
        if short_mode:
            self.i -= 2
            assert False
        else:
            self.i -= 1
            n = self.stack[self.i]
        return n

    def push(self, short_mode, n):
        if short_mode:
            assert False
        else:
            self.stack[self.i] = n
            self.i += 1

    def __repr__(self):
        used = self.stack[:self.i]
        return f"{self.i}: " + ' '.join(f"0x{x:02x}" for x in used)


class Uxn:
    def __init__(self):
        self.reset()

    def reset(self):
        self.pc = 0x100
        self.running = True
        self.wst = Stack()
        self.rst = Stack()

    def load_rom(self, rom):
        self.rom = rom

    def step(self, echo=False):
        if echo:
            uxndis.disassemble(self.rom.rom, self.pc, 1)

        op_byte = self.rom.rom[self.pc]
        self.pc += 1

        keep_mode, return_mode, short_mode, op, modes, opcode = decode_byte(op_byte)

        src = self.rst if return_mode else self.wst

        if op == 'lit':
            if short_mode:
                n = self.rom.peek16(self.pc)
                src.push(short_mode, n)
                self.pc += 2
            else:
                n = self.rom.peek8(self.pc)
                src.push(short_mode, n)
                self.pc += 1
        elif op == 'add':
            a = src.pop(short_mode)
            b = src.pop(short_mode)
            src.push(short_mode, b + a)
        elif op == 'brk':
            raise StopIteration
        else:
            assert False


def trace(rom):
    emu = Uxn()
    emu.load_rom(rom)

    print()
    while emu.running:
        try:
            emu.step(echo=True)
            print('wst', emu.wst)
            print('rst', emu.rst)
            print()
        except StopIteration:
            print('wst', emu.wst)
            print('rst', emu.rst)
            print()
            break


if __name__ == '__main__':
    print("emu")
