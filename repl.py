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
    while True:
        block = read_block()
        if not block:
            break
        print("block")
        print(block)
        print()

