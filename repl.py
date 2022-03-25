import talie
prompt = "& "

def read_block():
    block = []

    while True:
        line = input(prompt)

        if line == '':
            break

        if line:
            block.append(line)

    return block

if __name__ == "__main__":
    rom = talie.UxnRom()
    rom.pc = 0x100

    while True:
        block = read_block()
        if not block:
            break
        print("block")
        data = '\n'.join(block) + '\n'
        talie.assemble(rom, data)
        print(block)
        print()

    rom.resolve()
    print(rom)

