ifeq ($(OS),Windows_NT)     # is Windows_NT on XP, 2000, 7, Vista, 10...
    detected_OS := Windows
	PATH := $(PATH);../bin/
	mkdir := mkdir
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
	uxn32 := c:\tools\uxn32\uxn32.exe
	turye := c:\tools\uxn32\turye.rom font.uf2
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
    uxn_folder := ~/src/uxnemu/bin
    uxnasm := $(uxn_folder)/uxnasm
    uxncli := $(uxn_folder)/uxncli
    uxnemu := $(uxn_folder)/uxnemu
    turye := ~/src/turye/bin/turye.rom

endif

bin/hi.rom: hi.tal
	$(uxnasm) $< $@

bin/out.txt: bin/hi.rom
	certutil -f -encodeHex $< $@

wrun:
	watchexec -cr --ignore "*.rom" "make run"

run: bin/out.txt FORCE
#	$(uxncli) $<
	uxn.cmd $<

FORCE: ;

