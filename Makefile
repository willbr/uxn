
ifeq ($(OS),Windows_NT)     # is Windows_NT on XP, 2000, 7, Vista, 10...
    detected_OS := Windows
	PATH := $(PATH);./bin/
	mkdir := mkdir
	make := make
	rm := del
	cat := type
	EXE := .exe
	ignore := rem
	hex := gc | format-hex
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
	$(cat) t2
	python talie.py t2
	python uxndis.py out.rom
	python uxndis.py e1.rom
#	$(hex) out.rom
#	$(hex) e1.rom
#	../uxnemu/bin/uxncli out.rom

