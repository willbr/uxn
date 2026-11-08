ID=left
DIR=~/roms
ASM=uxncli ${DIR}/drifblim.rom
LIN=uxncli ${DIR}/uxnlin.rom
BAL=uxnbal
EMU=uxn11
ROM=bin/${ID}.rom

run: ${ROM}
	@ ${EMU} ${ROM}
test: ${ROM}
	@ ${EMU} ${ROM} src/left.tal
lint:
	@ ${LIN} src/${ID}.tal
bal:
	@ ${BAL} src/${ID}.tal
clean:
	@ rm -f ${ROM} ${ROM}.sym
install: ${ROM}
	@ cp ${ROM} ${DIR}
uninstall:
	@ rm -f ${DIR}/${ID}.rom
push: ${ROM}
	@ ~/bin/butler push ${ROM} hundredrabbits/${ID}:uxn
archive: ${ROM}
	@ cat src/${ID}.tal src/menu.tal src/utils.tal src/assets.tal | sed 's/~[^[:space:]]\+//' > bin/res.tal
	@ ${ASM} bin/res.tal bin/res.rom && ${EMU} bin/res.rom
	@ cp bin/res.tal ../oscean/etc/${ID}.tal.txt

.PHONY: run test clean lint bal install uninstall push archive

${ROM}: src/*
	@ mkdir -p bin
	@ ${ASM}
