.PHONY: build

ifeq ($(OS),Windows_NT)     # is Windows_NT on XP, 2000, 7, Vista, 10...
    detected_OS := Windows
	PATH := $(PATH);../bin/
	make := make
	rm := del
	cat := type
	EXE := .exe
	ignore := rem
	hex := python ..\hex.py
#	hex := gc | format-hex
	uxnasm := c:\tools\uxn\uxnasm.exe
	uxncli := c:\tools\uxn\uxncli.exe
	uxnemu := c:\tools\uxn\uxnemu.exe
else
    detected_OS := $(shell uname)  # same as "uname -s"
	PATH := $(PATH):./bin/:/home/wjbr/src/uxnemu/bin/
	make := make
	rm := rm
	cat := cat
	EXE :=
	ignore := echo ignore
	hex := hex -C
	uxnasm := uxnasm
	uxncli := uxncli
	uxnemu := uxnemu
endif

FORCE: ;

wbuild:
	watchexec -cr --ignore "*.rom" "make -B uild"

build: bin/forth.rom FORCE
	$(cat) input.txt | $(uxncli) $<
	echo bye

bin/forth.rom:	main.tal input.txt
#	$(cat) $<
#	python ..\src\talie.py $< out.rom
	$(uxnasm) $< $@
#	python ..\src\uxndis.py out.rom
#	python ..\uxnemu.py --trace out.rom
#	python ..\uxnemu.py out.rom

run:
	$(uxncli) forth.rom

wtest:
	watchexec -cr --ignore "*.rom" "make test"

test: bin/test-string.rom bin/test-forth.rom

bin/test-string.rom: test-string.tal
#	$(cat) $<
	$(uxnasm) $< $@
	$(uxncli) $@

out/test-forth.rom: test-forth.tal
#	$(cat) $<
	$(uxnasm) $< $@
	$(uxncli) $@

