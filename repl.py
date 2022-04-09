import talie
import uxndis
import uxnemu

def read_block():
    block = []
    depth = 0

    prompt = "& "
    while True:
        try:
            line = input(prompt)
        except EOFError:
            line = ''

        prompt = "> "

        if line == '':
            break

        depth += line.count('{')
        depth -= line.count('}')

        if line:
            block.append(line)

        if depth == 0:
            break

    return block


def iter_trace(emu, rom):
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


if __name__ == "__main__":
    emu = uxnemu.Uxn()

    rom = talie.UxnRom()
    rom.pc = 0x100

    while True:
        try:
            block = read_block()
        except KeyboardInterrupt:
            exit()

        if not block:
            break

        pc = rom.pc
        data = '\n'.join(block) + '\n'
        talie.assemble(rom, data)
        # rom.resolve()
        # uxndis.disassemble(rom.rom, pc)
        rom.resolve()
        uxndis.disassemble(rom.rom)

        emu.load_rom(rom)
        iter_trace(emu, rom)

    rom.write('brk')
    rom.resolve()
    uxndis.disassemble(rom.rom)

    uxnemu.trace(rom)

