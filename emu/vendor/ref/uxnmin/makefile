RELEASE_flags=-DNDEBUG -O2 -g0 -s
DEBUG_flags=-std=c89 -D_POSIX_C_SOURCE=199309L -DDEBUG -Wall -Wno-unknown-pragmas -Wpedantic -Wshadow -Wextra -Werror=implicit-int -Werror=incompatible-pointer-types -Werror=int-conversion -Wvla -g -Og -fsanitize=address -fsanitize=undefined

run: bin/uxnmin bin/tictactoe.rom
	@ bin/uxnmin bin/tictactoe.rom
test: bin/uxnmin-debug bin/opctest.rom bin/console.rom
	@ -bin/uxnmin 
	@ -bin/uxnmin missing.rom
	@ bin/uxnmin-debug bin/opctest.rom
	@ printf "ghi" | bin/uxnmin-debug bin/console.rom "abc" "def"
bootstrap: bin/uxnmin bin/drifloon.rom bin/checksum.min.rom
	@ cat etc/utils/drifloon.tal.txt | bin/uxnmin bin/drifloon.rom > bin/drifloon2.rom
	@ printf "A: "
	@ cat bin/drifloon2.rom | bin/uxnmin bin/checksum.min.rom
	@ printf "B: "
	@ cat bin/drifloon.rom | bin/uxnmin bin/checksum.min.rom
format:
	clang-format -i src/uxnmin.c
clean:
	rm -fr bin
install: bin/uxnmin
	cp bin/uxnmin ~/bin
grab: 
	cp ../drifblim/etc/drifloon.rom.txt etc/utils
	cp ../oscean/etc/drifloon.tal.txt etc/utils
	cp ../uxn-utils/cli/opctest/src/opctest.tal etc/tests
	cp ../uxn11/etc/tests/console.tal etc/tests/
	cp ../uxncli/examples/tictactoe.tal examples/
archive:
	cp src/uxnmin.c ../oscean/etc/uxnmin.c.txt

.PHONY: run debug format clean install grab archive

bin/uxnmin: src/uxnmin.c
	@ mkdir -p bin/
	@ cc ${RELEASE_flags} src/uxnmin.c -o bin/uxnmin
bin/uxnmin-debug: src/uxnmin.c
	@ mkdir -p bin/
	@ cc ${DEBUG_flags} src/uxnmin.c -o bin/uxnmin-debug

bin/drifloon.rom: etc/utils/drifloon.rom.txt
	@ cat etc/utils/drifloon.rom.txt | bin/uxnmin etc/utils/xh.txt > bin/drifloon.rom
bin/checksum.min.rom: bin/uxnmin bin/drifloon.rom etc/utils/checksum.min.tal
	cat etc/utils/checksum.min.tal | bin/uxnmin bin/drifloon.rom > bin/checksum.min.rom

bin/opctest.rom: bin/uxnmin bin/drifloon.rom etc/tests/opctest.tal
	@ cat etc/tests/opctest.tal | bin/uxnmin bin/drifloon.rom > bin/opctest.rom
bin/console.rom: bin/uxnmin bin/drifloon.rom etc/tests/console.tal
	@ cat etc/tests/console.tal | bin/uxnmin bin/drifloon.rom > bin/console.rom
bin/tictactoe.rom: bin/uxnmin bin/drifloon.rom examples/tictactoe.tal
	@ cat examples/tictactoe.tal | bin/uxnmin bin/drifloon.rom > bin/tictactoe.rom
