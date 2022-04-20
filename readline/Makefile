
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

build-test: test-random.tal
	$(cat) $<
#	python ..\src\talie.py $< out.rom
	$(uxnasm) $< out.rom
#	python ..\src\uxndis.py out.rom
#	python ..\uxnemu.py --trace out.rom
#	python ..\uxnemu.py out.rom
	c:\tools\uxn\uxncli.exe .\out.rom

build: readline.tal
	$(cat) $<
#	python ..\src\talie.py $< out.rom
	$(uxnasm) $< out.rom
#	python ..\src\uxndis.py out.rom
#	python ..\uxnemu.py --trace out.rom
#	python ..\uxnemu.py out.rom
	c:\tools\uxn\uxnemu.exe out.rom

edit-font:
	$(uxnemu) c:\tools\uxn32\turye.rom font.uf2

