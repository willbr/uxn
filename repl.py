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


if __name__ == "__main__":
    rom = talie.UxnRom()
    rom.pc = 0x100

    while True:
        try:
            block = read_block()
        except KeyboardInterrupt:
            exit()

        if not block:
            break

        # print("block")
        pc = rom.pc
        data = '\n'.join(block) + '\n'
        talie.assemble(rom, data)
        # print(block)
        # print()
        rom.resolve()
        uxndis.disassemble(rom.rom, pc)

    rom.write('brk')
    rom.resolve()
    uxndis.disassemble(rom.rom)

    uxnemu.trace(rom)

