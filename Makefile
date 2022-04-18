
ifeq ($(OS),Windows_NT)     # is Windows_NT on XP, 2000, 7, Vista, 10...
    detected_OS := Windows
	PATH := $(PATH);./bin/;c:/tools/uxn
	mkdir := mkdir
	make := make
	rm := del
	cat := type
	EXE := .exe
	ignore := rem
	hex := python hex.py
#	hex := gc | format-hex
else
    detected_OS := $(shell uname)  # same as "uname -s"
	PATH := $(PATH):./bin/
	mkdir := mkdir
	make := make
	rm := rm
	cat := cat
	EXE :=
	ignore := echo ignore
	hex := hex -C
endif

wbuild:
	watchexec -cr --ignore "*.rom" "make build"

build:
	$(cat) t4
	python talie.py t4 out.rom
	python uxndis.py out.rom
	$(hex) out.rom
	python uxnemu.py --trace out.rom

bin/test.rom : ../uxn5/etc/tests.tal
	python talie.py $< $@
	$(hex) bin/test.rom > hex1.txt
	$(hex) bin/test2.rom > hex2.txt
	python uxnemu.py --trace $@
#	python uxnemu.py $@

test: bin/test.rom

wtest-asm:
	watchexec -cr --ignore "*.rom" --on-busy-update do-nothing "make test-asm"

test-asm:
	python src/test-asm.py

