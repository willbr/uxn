
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

wbuild:
	watchexec -cr --ignore "*.rom" "make build"

build-test: test-random.tal
	$(cat) $<
#	python ..\src\talie.py $< out.rom
	$(uxnasm) $< out.rom
#	python ..\src\uxndis.py out.rom
#	python ..\uxnemu.py --trace out.rom
#	python ..\uxnemu.py out.rom
	c:\tools\uxn\uxncli.exe .\out.rom

build: readline.tal font.tal
#	$(cat) $<
#	python ..\src\talie.py $< out.rom
	$(uxnasm) $< out.rom
#	python ..\src\uxndis.py out.rom
#	python ..\uxnemu.py --trace out.rom
#	python ..\uxnemu.py out.rom
	make run

run:
	$(uxnemu) out.rom

font.tal: font.uf2
	$(uxncli) ../bin2tal/out.rom $< > $@

edit-font:
	$(uxnemu) $(turye) font.uf2

