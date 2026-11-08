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

/* Core checks: strict C89 + freestanding builds, no undefined symbols, no includes. */
#define CORE_CHECK "build/core-check"

static bool file_has_foreign_include(const char *path, bool allow_uxn_h)
{
	String_Builder sb = {0};
	const char *p;
	bool bad = false;

	if (!read_entire_file(path, &sb)) return true;
	sb_append_null(&sb);
	for (p = strstr(sb.items, "#include"); p; p = strstr(p + 1, "#include")) {
		if (allow_uxn_h && strncmp(p, "#include \"uxn.h\"", 16) == 0) continue;
		nob_log(ERROR, "%s: forbidden %.20s", path, p);
		bad = true;
	}
	sb_free(sb);
	return bad;
}

static bool nm_is_empty(const char *obj)
{
	Cmd cmd = {0};
	String_Builder sb = {0};
	const char *out = CORE_CHECK "/nm.txt";
	bool ok = false;

	cmd_append(&cmd, "llvm-nm", "--undefined-only", obj);
	if (!cmd_run(&cmd, .stdout_path = out)) return false;
	if (read_entire_file(out, &sb)) {
		size_t i;
		ok = true;
		for (i = 0; i < sb.count; i++) {
			if (sb.items[i] != '\n' && sb.items[i] != '\r' && sb.items[i] != ' ') ok = false;
		}
		if (!ok) nob_log(ERROR, "%s has undefined symbols:\n%.*s", obj, (int)sb.count, sb.items);
	}
	sb_free(sb);
	return ok;
}

static bool check_core(void)
{
	Cmd cmd = {0};
	if (!mkdir_if_not_exists("build")) return false;
	if (!mkdir_if_not_exists(CORE_CHECK)) return false;

	cmd_append(&cmd, "clang", "-std=c89", "-pedantic", "-Wall", "-Wextra", "-Werror",
		"-ffreestanding", "-fno-builtin", "-O2", "-c", "core/uxn.c", "-o", CORE_CHECK "/uxn_O2.o");
	if (!cmd_run(&cmd)) return false;
	cmd_append(&cmd, "clang", "-std=c89", "-pedantic", "-Wall", "-Wextra", "-Werror",
		"-ffreestanding", "-fno-builtin", "-O0", "-c", "core/uxn.c", "-o", CORE_CHECK "/uxn_O0.o");
	if (!cmd_run(&cmd)) return false;
	cmd_append(&cmd, "tcc", "-c", "core/uxn.c", "-o", CORE_CHECK "/uxn_tcc.o");
	if (!cmd_run(&cmd)) return false;

	if (!nm_is_empty(CORE_CHECK "/uxn_O2.o")) return false;
	if (!nm_is_empty(CORE_CHECK "/uxn_O0.o")) return false;
	if (file_has_foreign_include("core/uxn.h", false)) return false;
	if (file_has_foreign_include("core/uxn.c", true)) return false;
	return true;
}

static bool build_run_core_test(void)
{
	Cmd cmd = {0};
	if (!mkdir_if_not_exists("build")) return false;
	cmd_append(&cmd, "clang", "-std=c89", "-pedantic", "-Wall", "-Wextra", "-Werror", "-O0",
		"-o", "build/core_test.exe", "tests/core_test.c", "core/uxn.c");
	if (!cmd_run(&cmd)) return false;
	cmd_append(&cmd, "build/core_test.exe");
	return cmd_run(&cmd);
}

int main(int argc, char **argv)
{
	const char *target;
	NOB_GO_REBUILD_URSELF(argc, argv);
	target = argc > 1 ? argv[1] : "";
	if (strcmp(target, "ref") == 0) return build_ref() ? 0 : 1;
	if (strcmp(target, "check-core") == 0) return check_core() ? 0 : 1;
	if (strcmp(target, "test") == 0) return build_run_core_test() ? 0 : 1;
	nob_log(ERROR, "usage: nob <target>   targets: ref check-core test");
	return 1;
}
