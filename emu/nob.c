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

#define TEST_DIR "build/test"
#define UXNCLI "build/uxncli.exe"
#define UXNMIN BUILD_REF "/uxnmin.exe"

/* Runs args, redirecting stdio to files (any may be NULL). Returns the exit
 * code, or -1 if the process could not be run. Unlike cmd_run, a nonzero
 * exit is not an error and nothing is logged for it. */
static int run_code(const char *const *args, size_t n, const char *in, const char *out, const char *err)
{
	Cmd cmd = {0};
	Nob_Fd fdin = NOB_INVALID_FD, fdout = NOB_INVALID_FD, fderr = NOB_INVALID_FD;
	Nob_Cmd_Redirect redir = {0};
	Nob_Proc proc;
	int code = -1;
	size_t i;

	for (i = 0; i < n; i++) cmd_append(&cmd, args[i]);
	if (in) { fdin = fd_open_for_read(in); if (fdin == NOB_INVALID_FD) goto done; redir.fdin = &fdin; }
	if (out) { fdout = fd_open_for_write(out); if (fdout == NOB_INVALID_FD) goto done; redir.fdout = &fdout; }
	if (err) { fderr = fd_open_for_write(err); if (fderr == NOB_INVALID_FD) goto done; redir.fderr = &fderr; }
	proc = cmd_run_async_redirect(cmd, redir);
	if (proc == NOB_INVALID_PROC) goto done;
#ifdef _WIN32
	{
		DWORD st;
		WaitForSingleObject(proc, INFINITE);
		if (GetExitCodeProcess(proc, &st)) code = (int)st;
		CloseHandle(proc);
	}
#else
	{
		int st = 0;
		if (waitpid(proc, &st, 0) >= 0 && WIFEXITED(st)) code = WEXITSTATUS(st);
	}
#endif
done:
	if (fdin != NOB_INVALID_FD) fd_close(fdin);
	if (fdout != NOB_INVALID_FD) fd_close(fdout);
	if (fderr != NOB_INVALID_FD) fd_close(fderr);
	cmd_free(cmd);
	return code;
}

/* Reads a whole file into sb (replacing its contents); false if unreadable. */
static bool slurp(const char *path, String_Builder *sb)
{
	sb->count = 0;
	return read_entire_file(path, sb);
}

static bool files_equal(const char *a, const char *b)
{
	String_Builder x = {0}, y = {0};
	bool eq = slurp(a, &x) && slurp(b, &y) && x.count == y.count
		&& (x.count == 0 || memcmp(x.items, y.items, x.count) == 0);
	sb_free(x);
	sb_free(y);
	return eq;
}

static size_t count_newlines(const char *path)
{
	String_Builder sb = {0};
	size_t i, n = 0;
	if (slurp(path, &sb)) for (i = 0; i < sb.count; i++) if (sb.items[i] == '\n') n++;
	sb_free(sb);
	return n;
}

static size_t file_size(const char *path)
{
	String_Builder sb = {0};
	size_t n = slurp(path, &sb) ? sb.count : (size_t)-1;
	sb_free(sb);
	return n;
}

/* Runs `rom args...` under the reference and under uxncli with the same
 * stdin, and requires identical stdout bytes and exit codes. */
static bool same_as_reference(const char *rom, const char *const *args, size_t n, const char *in)
{
	const char *ra[8], *ua[8];
	int rc, uc;
	size_t i;

	if (n > 5) return false;
	ra[0] = UXNMIN; ra[1] = rom;
	ua[0] = UXNCLI; ua[1] = rom;
	for (i = 0; i < n; i++) ra[2 + i] = ua[2 + i] = args[i];
	rc = run_code(ra, 2 + n, in, TEST_DIR "/ref.out", NULL);
	uc = run_code(ua, 2 + n, in, TEST_DIR "/our.out", NULL);
	if (rc < 0 || uc < 0) return false;
	if (rc != uc) { nob_log(ERROR, "exit code: ref %d, ours %d", rc, uc); return false; }
	if (!files_equal(TEST_DIR "/ref.out", TEST_DIR "/our.out")) {
		nob_log(ERROR, "stdout differs from reference for %s", rom);
		return false;
	}
	return true;
}

static bool opctest_matches_reference(void)
{
	return same_as_reference(BUILD_REF "/opctest.rom", NULL, 0, TEST_DIR "/empty.txt");
}

static bool console_matches_reference(void)
{
	const char *args[] = { "abc", "def" };
	return write_entire_file(TEST_DIR "/ghi.txt", "ghi", 3)
		&& same_as_reference(BUILD_REF "/console.rom", args, 2, TEST_DIR "/ghi.txt");
}

static bool drifloon_self_matches(void)
{
	const char *opc[] = { REF_UXNMIN "/etc/tests/opctest.tal" };
	const char *left[] = {
		REF_LEFT "/src/left.tal", REF_LEFT "/src/menu.tal",
		REF_LEFT "/src/utils.tal", REF_LEFT "/src/assets.tal" };

	if (!assemble(UXNCLI, left, 4, TEST_DIR "/left.rom")) return false;
	if (!files_equal(TEST_DIR "/left.rom", BUILD_REF "/left.rom")) {
		nob_log(ERROR, "left.rom assembled by uxncli differs from the reference");
		return false;
	}
	if (!assemble(UXNCLI, opc, 1, TEST_DIR "/opctest.rom")) return false;
	if (!files_equal(TEST_DIR "/opctest.rom", BUILD_REF "/opctest.rom")) {
		nob_log(ERROR, "opctest.rom assembled by uxncli differs from the reference");
		return false;
	}
	return true;
}

static bool console_binary_roundtrip(void)
{
	static const char bytes[5] = { 0x00, 0x0a, 0x0d, 0x1a, (char)0xff };
	const char *tal[] = { "tests/roms/binary.tal" };
	const char *ua[2];
	String_Builder sb = {0};
	bool ok;

	ua[0] = UXNCLI; ua[1] = TEST_DIR "/binary.rom";
	if (!assemble(UXNMIN, tal, 1, TEST_DIR "/binary.rom")) return false;
	if (!write_entire_file(TEST_DIR "/bytes.in", bytes, 5)) return false;
	if (run_code(ua, 2, TEST_DIR "/bytes.in", TEST_DIR "/bytes.out", NULL) != 0) return false;
	ok = slurp(TEST_DIR "/bytes.out", &sb) && sb.count == 5 && memcmp(sb.items, bytes, 5) == 0;
	if (!ok) nob_log(ERROR, "binary roundtrip: got %d bytes, wanted the 5 sent", (int)sb.count);
	sb_free(sb);
	return ok;
}

static bool rejects_one(const char *rom, const char *what)
{
	const char *ua[2];
	int code;

	ua[0] = UXNCLI; ua[1] = rom;
	code = run_code(ua, 2, TEST_DIR "/empty.txt", TEST_DIR "/bad.out", TEST_DIR "/bad.err");
	if (code <= 0) { nob_log(ERROR, "%s: exit code %d, wanted nonzero", what, code); return false; }
	if (file_size(TEST_DIR "/bad.out") != 0) { nob_log(ERROR, "%s: stdout not empty", what); return false; }
	if (count_newlines(TEST_DIR "/bad.err") != 1) { nob_log(ERROR, "%s: stderr not exactly one line", what); return false; }
	return true;
}

static bool cli_rejects_bad_roms(void)
{
	static char big[0xff01];
	bool ok;

	if (!write_entire_file(TEST_DIR "/zero.rom", "", 0)) return false;
	if (!write_entire_file(TEST_DIR "/big.rom", big, sizeof big)) return false;
	ok = rejects_one(TEST_DIR "/missing.rom", "missing");
	ok = rejects_one(TEST_DIR "/zero.rom", "empty") && ok;
	ok = rejects_one(TEST_DIR "/big.rom", "oversize") && ok;
	return ok;
}

#define REF_UXN11 "vendor/ref/uxn11"
#define FS_OBJ "build/fs.obj"

/* The Varvara sources that are strict C89. fs.c, the OS shim, is built
 * separately: it is also strict C89 here (clang on Windows accepts the
 * Win32 headers under -std=c89 -pedantic), so it needs no -std=c99 exemption. */
static const char *const VARVARA_SRCS[] = {
	"varvara/varvara.c", "varvara/system.c", "varvara/console.c",
	"varvara/datetime.c", "varvara/file.c", "core/uxn.c"
};

static bool build_fs_obj(void)
{
	Cmd cmd = {0};
	if (!mkdir_if_not_exists("build")) return false;
	cmd_append(&cmd, "clang", "-std=c89", "-pedantic", "-Wall", "-Wextra", "-Werror", "-O2",
		"-D_CRT_SECURE_NO_WARNINGS", "-c", "-o", FS_OBJ, "varvara/fs.c");
	return cmd_run(&cmd);
}

static bool build_cli(void)
{
	Cmd cmd = {0};
	size_t i;
	if (!build_fs_obj()) return false;
	cmd_append(&cmd, "clang", "-std=c89", "-pedantic", "-Wall", "-Wextra", "-Werror", "-O2",
		"-D_CRT_SECURE_NO_WARNINGS", "-Icore", "-Ivarvara", "-o", UXNCLI, "cli/uxncli.c", FS_OBJ);
	for (i = 0; i < sizeof VARVARA_SRCS / sizeof VARVARA_SRCS[0]; i++) cmd_append(&cmd, VARVARA_SRCS[i]);
	return cmd_run(&cmd);
}

/* Absolute path of the emu directory, captured at startup so tests can chdir. */
static char abs_root[1024];

static const char *abs_path(const char *rel)
{
	return temp_sprintf("%s/%s", abs_root, rel);
}

/* Deletes dir (if present) and creates it again, empty. */
static bool fresh_dir(const char *dir)
{
	Cmd cmd = {0};
	if (file_exists(dir)) {
#ifdef _WIN32
		char *w = temp_strdup(dir);
		char *c;
		for (c = w; *c; c++) if (*c == '/') *c = '\\';
		cmd_append(&cmd, "cmd", "/c", "rd", "/s", "/q", w);
#else
		cmd_append(&cmd, "rm", "-rf", dir);
#endif
		if (!cmd_run(&cmd)) return false;
	}
	return mkdir_if_not_exists(dir);
}

/* Writes a file whose name is UTF-8 (nob's own file functions use the ANSI APIs). */
static bool write_file_utf8(const char *path, const char *data, size_t n)
{
	FILE *f;
	bool ok;
#ifdef _WIN32
	WCHAR w[1024];
	if (!MultiByteToWideChar(CP_UTF8, 0, path, -1, w, 1024)) return false;
	f = _wfopen(w, L"wb");
#else
	f = fopen(path, "wb");
#endif
	if (!f) return false;
	ok = fwrite(data, 1, n, f) == n;
	return fclose(f) == 0 && ok;
}

static size_t count_substr(const char *path, const char *needle)
{
	String_Builder sb = {0};
	size_t n = 0, len = strlen(needle);
	const char *p;

	if (slurp(path, &sb)) {
		sb_append_null(&sb);
		for (p = strstr(sb.items, needle); p; p = strstr(p + len, needle)) n++;
	}
	sb_free(sb);
	return n;
}

/* Runs uxncli on rom (an emu-relative path) with working directory dir
 * (emu-relative, or NULL for the current one), stdout to out. Returns the exit
 * code or -1. */
static int run_rom(const char *dir, const char *rom, const char *extra, const char *out)
{
	const char *args[3];
	int code;

	args[0] = abs_path(UXNCLI);
	args[1] = abs_path(rom);
	args[2] = extra;
	if (dir && !set_current_dir(abs_path(dir))) return -1;
	code = run_code(args, extra ? 3 : 2, abs_path(TEST_DIR "/empty.txt"), abs_path(out), NULL);
	if (dir && !set_current_dir(abs_root)) return -1;
	return code;
}

/* Expects exactly `passes` "pass" lines, no "fail", and exit code 0. */
static bool pass_fail_ok(const char *what, int code, const char *out, size_t passes)
{
	size_t p = count_substr(out, "pass\n"), f = count_substr(out, "fail\n");
	if (code != 0 || f != 0 || p != passes) {
		nob_log(ERROR, "%s: exit %d, %d pass, %d fail (wanted %d pass, 0 fail)",
			what, code, (int)p, (int)f, (int)passes);
		return false;
	}
	return true;
}

static bool system_matches_uxn11(void)
{
	const char *tal[] = { REF_UXN11 "/etc/tests/system.tal" };
	if (!assemble(UXNMIN, tal, 1, TEST_DIR "/system.rom")) return false;
	return pass_fail_ok("system.tal", run_rom(NULL, TEST_DIR "/system.rom", NULL, TEST_DIR "/system.out"),
		TEST_DIR "/system.out", 6);
}

static bool file_matches_uxn11(void)
{
	const char *tal[] = { REF_UXN11 "/etc/tests/file.tal" };
	if (!assemble(UXNMIN, tal, 1, TEST_DIR "/file.rom")) return false;
	if (!fresh_dir(TEST_DIR "/filetest")) return false;
	return pass_fail_ok("file.tal",
		run_rom(TEST_DIR "/filetest", TEST_DIR "/file.rom", NULL, TEST_DIR "/file.out"),
		TEST_DIR "/file.out", 12);
}

/* datetime.tal prints text: "YYYY-MM-DD" on its first line. */
static bool datetime_runs(void)
{
	const char *tal[] = { REF_UXN11 "/etc/tests/datetime.tal" };
	String_Builder sb = {0};
	int year = 0, code;
	bool ok;

	if (!assemble(UXNMIN, tal, 1, TEST_DIR "/datetime.rom")) return false;
	code = run_rom(NULL, TEST_DIR "/datetime.rom", NULL, TEST_DIR "/datetime.out");
	ok = code == 0 && slurp(TEST_DIR "/datetime.out", &sb);
	if (ok) {
		sb_append_null(&sb);
		ok = sscanf(sb.items, "%d-", &year) == 1 && year >= 2025 && year <= 2100
			&& strstr(sb.items, "The date is:") != NULL;
	}
	if (!ok) nob_log(ERROR, "datetime.tal: exit %d, year %d", code, year);
	sb_free(sb);
	return ok;
}

#define LIST_FILES 300

static bool file_listing_edge_cases(void)
{
	const char *tal[] = { "tests/roms/listing.tal" };
	static char expect[LIST_FILES + 3][64];
	static char used[LIST_FILES + 3];
	String_Builder sb = {0};
	char name[64], data[64];
	const char *line;
	size_t n_expect = 0, nlines = 0, i;
	bool ok = false;
	int code;

	if (!assemble(UXNMIN, tal, 1, TEST_DIR "/listing.rom")) return false;
	if (!fresh_dir(TEST_DIR "/listdir")) return false;
	if (!mkdir_if_not_exists(TEST_DIR "/listdir/sub")) return false;
	memset(data, 'x', sizeof data);
	for (i = 0; i < LIST_FILES; i++) {
		sprintf(name, TEST_DIR "/listdir/f%03d.txt", (int)i);
		if (!write_file_utf8(name, data, i % 40)) return false;
		sprintf(expect[n_expect++], "%04x\tf%03d.txt\n", (unsigned)(i % 40), (int)i);
	}
	if (!write_file_utf8(TEST_DIR "/listdir/a b.txt", data, 3)) return false;
	strcpy(expect[n_expect++], "0003\ta b.txt\n");
	if (!write_file_utf8(TEST_DIR "/listdir/\xc3\xbcn\xc3\xaf.txt", data, 10)) return false;
	strcpy(expect[n_expect++], "000a\t\xc3\xbcn\xc3\xaf.txt\n");
	strcpy(expect[n_expect++], "----\tsub/\n");

	code = run_rom(NULL, TEST_DIR "/listing.rom", TEST_DIR "/listdir", TEST_DIR "/listing.out");
	if (code != 0 || !slurp(TEST_DIR "/listing.out", &sb)) {
		nob_log(ERROR, "listing: exit code %d", code);
		goto done;
	}
	sb_append_null(&sb);
	/* every line must be one expected entry, each exactly once, none cut short */
	for (line = sb.items; *line; nlines++) {
		const char *nl = strchr(line, '\n');
		size_t len, k;
		bool found = false;
		if (!nl) { nob_log(ERROR, "listing: partial last line"); goto done; }
		len = (size_t)(nl - line) + 1;
		for (k = 0; k < n_expect && !found; k++) {
			if (strlen(expect[k]) == len && memcmp(expect[k], line, len) == 0) {
				if (used[k]) { nob_log(ERROR, "listing: duplicate %s", expect[k]); goto done; }
				used[k] = 1;
				found = true;
			}
		}
		if (!found) { nob_log(ERROR, "listing: unexpected line %.*s", (int)len - 1, line); goto done; }
		line = nl + 1;
	}
	if (nlines != n_expect) {
		nob_log(ERROR, "listing: %d lines, wanted %d", (int)nlines, (int)n_expect);
		goto done;
	}
	/* listing ".." from a filesystem root must not crash */
#ifdef _WIN32
	code = run_rom(NULL, TEST_DIR "/listing.rom", "C:/..", TEST_DIR "/listing_up.out");
#else
	code = run_rom(NULL, TEST_DIR "/listing.rom", "/..", TEST_DIR "/listing_up.out");
#endif
	if (code != 0) { nob_log(ERROR, "listing ..: exit code %d", code); goto done; }
	ok = true;
done:
	sb_free(sb);
	return ok;
}

static bool build_run_varvara_test(void)
{
	Cmd cmd = {0};
	size_t i;

	if (!build_fs_obj()) return false;
	cmd_append(&cmd, "clang", "-std=c89", "-pedantic", "-Wall", "-Wextra", "-Werror", "-O0",
		"-D_CRT_SECURE_NO_WARNINGS", "-Icore", "-Ivarvara", "-o", "build/varvara_test.exe",
		"tests/varvara_test.c", FS_OBJ);
	for (i = 0; i < sizeof VARVARA_SRCS / sizeof VARVARA_SRCS[0]; i++) cmd_append(&cmd, VARVARA_SRCS[i]);
	if (!cmd_run(&cmd)) return false;
	if (!fresh_dir(TEST_DIR "/vt")) return false;
	cmd_append(&cmd, "build/varvara_test.exe", TEST_DIR "/vt");
	return cmd_run(&cmd);
}

static bool run_varvara_cli_tests(void)
{
	int failures = 0, total = 0;

	if (!mkdir_if_not_exists("build")) return false;
	if (!mkdir_if_not_exists(TEST_DIR)) return false;
	if (!write_entire_file(TEST_DIR "/empty.txt", "", 0)) return false;
	if (!build_cli()) return false;

#define VT(f) do { total++; if (!f()) { printf("FAIL %s\n", #f); failures++; } } while (0)
	VT(opctest_matches_reference);
	VT(console_matches_reference);
	VT(drifloon_self_matches);
	VT(console_binary_roundtrip);
	VT(cli_rejects_bad_roms);
	VT(system_matches_uxn11);
	VT(file_matches_uxn11);
	VT(datetime_runs);
	VT(file_listing_edge_cases);
#undef VT
	printf("varvara-cli: %d passed, %d failed\n", total - failures, failures);
	return failures == 0;
}

static bool run_tests(void)
{
	bool ok = build_run_core_test();
	ok = build_run_varvara_test() && ok;
	return run_varvara_cli_tests() && ok;
}

int main(int argc, char **argv)
{
	const char *target;
	NOB_GO_REBUILD_URSELF(argc, argv);
	snprintf(abs_root, sizeof abs_root, "%s", get_current_dir_temp());
	target = argc > 1 ? argv[1] : "";
	if (strcmp(target, "ref") == 0) return build_ref() ? 0 : 1;
	if (strcmp(target, "check-core") == 0) return check_core() ? 0 : 1;
	if (strcmp(target, "cli") == 0) return build_cli() ? 0 : 1;
	if (strcmp(target, "test") == 0) return run_tests() ? 0 : 1;
	nob_log(ERROR, "usage: nob <target>   targets: ref check-core cli test");
	return 1;
}
