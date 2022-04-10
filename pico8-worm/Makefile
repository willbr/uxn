
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

build: worm.talie
	$(cat) $<
	python ..\talie.py $< out.rom
	python ..\uxndis.py out.rom
#	python ..\uxnemu.py --trace out.rom
#	python ..\uxnemu.py out.rom
	c:\tools\uxn\uxnemu.exe .\out.rom

