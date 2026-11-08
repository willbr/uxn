RELEASE_flags=-DNDEBUG -O2 -g0 -s
DEBUG_flags=-std=c99 -D_POSIX_C_SOURCE=199309L -DDEBUG -Wall -Wno-unknown-pragmas -Wpedantic -Wshadow -Wextra -Werror=implicit-int -Werror=incompatible-pointer-types -Werror=int-conversion -Wvla -g -Og -fsanitize=address -fsanitize=undefined
PREFIX=${HOME}/.local

run: bin/uxn11 bin/m_pc.rom bin/perifs.rom
	bin/uxn11 bin/m_pc.rom bin/perifs.rom run
gui: bin/uxn11 bin/screen.rom
	bin/uxn11 bin/screen.rom
test: bin/uxn11-debug tests bin/m_pc.rom bin/perifs.rom
	@ -bin/uxn11-debug
	@ -bin/uxn11-debug -v
	@ bin/uxn11-debug bin/opctest.rom
	@ bin/uxn11-debug bin/system.rom
	@ bin/uxn11-debug bin/console.rom
	@ printf "ghi" | bin/uxn11-debug bin/console.rom "abc" "def"
	@ bin/uxn11-debug bin/file.rom
	@ bin/uxn11-debug bin/datetime.rom
	@ bin/uxn11-debug bin/m_pc.rom bin/perifs.rom run
clean:
	rm -fr bin
format:
	clang-format -i src/uxn11.c
install: bin/uxn11
	mkdir -p ${PREFIX}/bin
	cp bin/uxn11 ${PREFIX}/bin
	mkdir -p ${PREFIX}/share/man/man7
	cp doc/man/uxntal.7 ${PREFIX}/share/man/man7
uninstall:
	rm -f ${PREFIX}/bin/uxn11
grab:
	cp ../drifblim/etc/drifblim.rom.txt etc/utils/
	cp ../uxn-utils/cli/opctest/src/opctest.tal etc/tests/
	cp ../uxn-utils/cli/tests/src/* etc/tests/
	cp ../uxn-utils/gui/perifs/src/perifs.tal etc/tests/
	cp ../m_pc/src/m_pc.tal etc/utils/
archive:
	cp src/uxn11.c ../oscean/etc/uxn11.c.txt
	cp doc/man/uxntal.7 ../oscean/etc/uxntal.7.txt

.PHONY: run test clean format install uninstall grab archive

bin/uxn11: src/uxn11.c
	mkdir -p bin
	cc ${RELEASE_flags} ${CFLAGS} src/uxn11.c -lX11 -lutil -o bin/uxn11
bin/uxn11-debug: src/uxn11.c
	mkdir -p bin
	cc ${DEBUG_flags} ${CFLAGS} src/uxn11.c -lX11 -lutil -o bin/uxn11-debug

# Tools

bin/drifblim.rom: bin/uxn11 etc/utils/drifblim.rom.txt
	@ cat etc/utils/drifblim.rom.txt | bin/uxn11 etc/utils/xh.rom > bin/drifblim.rom
bin/m_pc.rom: bin/uxn11 bin/drifblim.rom etc/utils/m_pc.tal
	@ bin/uxn11 bin/drifblim.rom etc/utils/m_pc.tal bin/m_pc.rom

# Tests

tests: bin/opctest.rom bin/system.rom bin/console.rom bin/screen.rom bin/perifs.rom bin/file.rom bin/datetime.rom

bin/opctest.rom: bin/uxn11 bin/drifblim.rom etc/tests/opctest.tal
	@ bin/uxn11 bin/drifblim.rom etc/tests/opctest.tal bin/opctest.rom
bin/system.rom: bin/uxn11 bin/drifblim.rom etc/tests/system.tal
	@ bin/uxn11 bin/drifblim.rom etc/tests/system.tal bin/system.rom
bin/console.rom: bin/uxn11 bin/drifblim.rom etc/tests/console.tal
	@ bin/uxn11 bin/drifblim.rom etc/tests/console.tal bin/console.rom
bin/screen.rom: bin/uxn11 bin/drifblim.rom etc/tests/screen.bounds.tal
	@ bin/uxn11 bin/drifblim.rom etc/tests/screen.bounds.tal bin/screen.rom
bin/perifs.rom: bin/uxn11 bin/drifblim.rom etc/tests/perifs.tal
	@ bin/uxn11 bin/drifblim.rom etc/tests/perifs.tal bin/perifs.rom
bin/file.rom: bin/uxn11 bin/drifblim.rom etc/tests/file.tal
	@ bin/uxn11 bin/drifblim.rom etc/tests/file.tal bin/file.rom
bin/datetime.rom: bin/uxn11 bin/drifblim.rom etc/tests/datetime.tal
	@ bin/uxn11 bin/drifblim.rom etc/tests/datetime.tal bin/datetime.rom
