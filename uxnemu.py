from uxn import *

import sys
import talie
import uxndis 
import argparse


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


def debug_system(dev, port):
    i = dev.dat[port]
    if i:
        sys.exit(i)

def debug_stdout(dev, port):
    if port == 8:
        file = sys.stdout
    elif port == 9:
        file = sys.stderr
    else:
        file = None

    if file:
        c = chr(dev.dat[port])
        print(c, file=file, end="")


def sint8(n):
    if n > 127:
        s = n - 256
        return s
    else:
        return n


class Stack:
    def __init__(self):
        self.reset()

    def reset(self):
        self.stack = bytearray(256)
        self.i = 0
        self.short_mode = False
        self.keep_mode  = False

    def pop(self):
        n = self.pop16() if self.short_mode else self.pop8()
        assert self.i >= 0
        assert self.i <= 255
        return n

    def pop16(self):
        low  = self.stack[self.i - 1]
        high = self.stack[self.i - 2]
        n = (high << 8) + low
        if self.keep_mode:
            pass
        else:
            self.i -= 2
        return n

    def pop8(self):
        n = self.stack[self.i-1]
        if self.keep_mode:
            pass
        else:
            self.i -= 1
        return n

    def push(self, n):
        if self.short_mode:
            i = n % 0xffff
            self.push16(i)
        else:
            i = n % 0xff
            self.push8(i)


    def push16(self, n):
        assert n >= 0
        assert n <= 0xffff

        high = n >> 8
        low  = n & 0x00ff

        self.push8(high)
        self.push8(low)


    def push8(self, n):
        assert n >= 0
        assert n <= 0xff
        self.stack[self.i] = n
        self.i += 1
        # assert self.i >= 0
        # assert self.i <= 255

    def __repr__(self):
        if self.i > 0:
            used = self.stack[:self.i]
            return f"{self.i}: " + ' '.join(f"0x{x:02x}" for x in used)
        else:
            return f"{self.i}:"


class Device:
    def __init__(self, uxn, i):
        self.dat = bytearray(16)
        self.u = uxn
        self.i = i
        pass

    def dei(self, dev, port):
        assert False
        return "u8"

    def deo(self, dev, port):
        i = self.i
        assert False


class Uxn:
    def __init__(self):
        self.reset()

    def reset(self):
        self.pc = 0x100
        self.running = True
        self.wst = Stack()
        self.rst = Stack()
        self.dev = [Device(self, i) for i in range(16)]
        self.dev[0].deo = debug_system
        self.dev[1].deo = debug_stdout

    def load_rom(self, rom):
        self.rom = rom

    def devw(self, short_mode, dev, x, y):
        if short_mode:
            high = y >> 8
            low  = y & 0x00ff
            self.devw8(dev, x, high)
            self.devw8(dev, x, low)
        else:
            self.devw8(dev, x, y)

    def devw8(self, dev, x, y):
        dev.dat[x & 0xf] = y
        dev.deo(dev, x & 0x0f)

    def step(self, echo=False):
        if echo:
            uxndis.disassemble(self.rom.rom, self.pc, 1)

        try:
            op_byte = self.rom.rom[self.pc]
        except IndexError:
            raise StopIteration(f"read past end of rom at: 0x{self.pc:02x}")

        self.pc += 1

        keep_mode, return_mode, short_mode, op, modes, opcode = decode_byte(op_byte)

        if return_mode:
            src = self.rst
            dst = self.wst
        else:
            src = self.wst
            dst = self.rst

        src.short_mode = short_mode
        src.keep_mode  = keep_mode

        dst.short_mode = short_mode
        dst.keep_mode  = keep_mode

        orig_i = src.i

        # Stack
        if op == 'brk':
            raise StopIteration
        elif op == 'lit':
            if short_mode:
                n = self.rom.peek16(self.pc)
                src.push(n)
                self.pc += 2
            else:
                n = self.rom.peek8(self.pc)
                src.push(n)
                self.pc += 1
        elif op == 'inc':
            n = src.pop()
            n += 1
            src.push(n)
        elif op == 'pop':
            _ = src.pop()
        elif op == 'dup':
            a = src.pop()
            src.push(a)
            src.push(a)
        elif op == 'nip':
            assert False
        elif op == 'swp':
            a = src.pop()
            b = src.pop()
            src.push(a)
            src.push(b)
        elif op == 'ovr':
            assert False
        elif op == 'rot':
            assert False
        # Logic
        elif op == 'equ':
            a = src.pop()
            b = src.pop()
            n = 1 if a == b else 0
            src.push(n)
        elif op == 'neq':
            assert False
        elif op == 'gth':
            assert False
        elif op == 'lth':
            assert False
        elif op == 'jmp':
            a = src.pop()
            self.warp(short_mode, a)
        elif op == 'jcn':
            a = src.pop()
            b = src.pop8()
            if b:
                self.warp(short_mode, a)
        elif op == 'jsr':
            a = src.pop()
            self.rst.push16(self.pc)
            self.warp(short_mode, a)
        elif op == 'sth':
            c = src.pop()
            dst.push(c)
        # Memory
        elif op == 'ldz':
            assert False
        elif op == 'stz':
            assert False
        elif op == 'ldr':
            assert False
        elif op == 'str':
            assert False
        elif op == 'lda':
            addr = src.pop16()
            n = self.rom.peek(short_mode, addr)
            src.push(n)
        elif op == 'sta':
            addr = src.pop16()
            n = src.pop()
            self.rom.poke(short_mode, addr, n)
        elif op == 'dei':
            assert False
        elif op == 'deo':
            a = src.pop8()
            b = src.pop()

            d = self.dev[a >> 4]
            self.devw(short_mode, d, a, b)
        # Arithmetic
        elif op == 'add':
            a = src.pop()
            b = src.pop()
            src.push(b + a)
        elif op == 'sub':
            assert False
        elif op == 'mul':
            a = src.pop()
            b = src.pop()
            src.push(b * a)
        elif op == 'div':
            assert False
        elif op == 'and':
            assert False
        elif op == 'ora':
            assert False
        elif op == 'eor':
            assert False
        elif op == 'sft':
            assert False
        else:
            assert False

    def warp(self, short_mode, x):
        if short_mode:
            self.pc = x
        else:
            n = sint8(x)
            self.pc += n


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
        # input()


def run(rom):
    emu = Uxn()
    emu.load_rom(rom)

    while emu.running:
        try:
            emu.step(echo=False)
        except StopIteration:
            print('wst', emu.wst)
            print('rst', emu.rst)
            print()
            break


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="uxn tool")

    parser.add_argument("filename")
    parser.add_argument("--disassemble", action='store_true')
    parser.add_argument("--trace", action='store_true')

    args = parser.parse_args()

    rom = talie.UxnRom(args.filename)

    if args.disassemble:
        uxndis.disassemble(rom.rom)

    if args.trace:
        trace(rom)
    else:
        run(rom)

    print("done")
