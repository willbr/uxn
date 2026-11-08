/* Build script. Bootstrap once: clang -o nob.exe nob.c   then: ./nob.exe <target> */
#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "vendor/nob/nob.h"

#define BUILD_REF "build/ref"
#define REF_UXNMIN "vendor/ref/uxnmin"
#define REF_LEFT "vendor/ref/left"

/* Concatenate tal_files into stdin of `<uxncli> build/ref/drifloon.rom`,
 * writing its stdout to out_rom. */
static bool assemble(const char *uxncli, const char *tal_files[], size_t n, const char *out_rom)
{
	bool ok = false;
	String_Builder all = {0};
	String_Builder one = {0};
	const char *cat = temp_sprintf("%s.cat.tal", out_rom);
	Cmd cmd = {0};
	size_t i;

	for (i = 0; i < n; i++) {
		one.count = 0;
		if (!read_entire_file(tal_files[i], &one)) goto done;
		sb_append_buf(&all, one.items, one.count);
		sb_append_cstr(&all, "\n");
	}
	if (!write_entire_file(cat, all.items, all.count)) goto done;
	cmd_append(&cmd, uxncli, BUILD_REF "/drifloon.rom");
	ok = cmd_run(&cmd, .stdin_path = cat, .stdout_path = out_rom);
done:
	sb_free(all);
	sb_free(one);
	cmd_free(cmd);
	return ok;
}

static bool build_ref(void)
{
	Cmd cmd = {0};
	const char *uxnmin = BUILD_REF "/uxnmin.exe";
	const char *opc[] = { REF_UXNMIN "/etc/tests/opctest.tal" };
	const char *con[] = { REF_UXNMIN "/etc/tests/console.tal" };
	const char *left[] = {
		REF_LEFT "/src/left.tal", REF_LEFT "/src/menu.tal",
		REF_LEFT "/src/utils.tal", REF_LEFT "/src/assets.tal" };

	if (!mkdir_if_not_exists("build")) return false;
	if (!mkdir_if_not_exists(BUILD_REF)) return false;

	/* uxnmin.exe: the vendored source gets main renamed; the wrapper keeps the real main */
	cmd_append(&cmd, "clang", "-O2", "-D_CRT_SECURE_NO_WARNINGS", "-Dmain=uxnmin_main",
		"-c", "-o", BUILD_REF "/uxnmin.obj", REF_UXNMIN "/src/uxnmin.c");
	if (!cmd_run(&cmd)) return false;
	cmd_append(&cmd, "clang", "-O2", "-D_CRT_SECURE_NO_WARNINGS",
		"-o", uxnmin, "tools/uxnmin_main.c", BUILD_REF "/uxnmin.obj");
	if (!cmd_run(&cmd)) return false;

	/* drifloon.rom: assembled from the hex text by uxnmin running xh.txt */
	cmd_append(&cmd, uxnmin, REF_UXNMIN "/etc/utils/xh.txt");
	if (!cmd_run(&cmd, .stdin_path = REF_UXNMIN "/etc/utils/drifloon.rom.txt",
	             .stdout_path = BUILD_REF "/drifloon.rom")) return false;

	if (!assemble(uxnmin, opc, 1, BUILD_REF "/opctest.rom")) return false;
	if (!assemble(uxnmin, con, 1, BUILD_REF "/console.rom")) return false;
	if (!assemble(uxnmin, left, 4, BUILD_REF "/left.rom")) return false;
	return true;
}

int main(int argc, char **argv)
{
	const char *target;
	NOB_GO_REBUILD_URSELF(argc, argv);
	target = argc > 1 ? argv[1] : "";
	if (strcmp(target, "ref") == 0) return build_ref() ? 0 : 1;
	nob_log(ERROR, "usage: nob <target>   targets: ref");
	return 1;
}
