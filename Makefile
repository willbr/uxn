wbuild:
	watchexec -cr --ignore "*.rom" "make build"

build:
	cat t1
	echo
	python talie.py t1
	echo
	hexdump -C out.rom
	echo
	hexdump -C e1.rom
	echo
	../uxnemu/bin/uxncli out.rom

