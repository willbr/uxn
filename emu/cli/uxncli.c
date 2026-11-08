/* Headless Varvara host:
 *   uxncli [--root DIR] [--max-steps N] [--frames N] [--dump out.ppm] rom.rom [args...]
 * --frames runs the Screen vector N times after boot; --dump then writes the
 * composited screen as a binary P6 PPM. With either, stdin is not read. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
#include "varvara.h"

static Varvara vv;

static void on_console(void *ctx, int fd, int c)
{
	(void)ctx;
	fputc(c, fd == 2 ? stderr : stdout);
}

static int parse_count(const char *flag, const char *s, unsigned long *out)
{
	char *end;
	*out = strtoul(s, &end, 10);
	if (*s == '\0' || *s == '-' || *end != '\0') {
		fprintf(stderr, "uxncli: bad %s value: %s\n", flag, s);
		return 1;
	}
	return 0;
}

/* Writes the screen as P6: "P6\n<w> <h>\n255\n" then RGB bytes. */
static int dump_ppm(const Varvara *v, const char *path)
{
	size_t i, n = (size_t)v->width * (size_t)v->height;
	ux_u8 *rgba = (ux_u8 *)malloc(n * 4 + 1);
	FILE *f;
	int ok;

	if (!rgba) {
		fprintf(stderr, "uxncli: --dump: out of memory\n");
		return 1;
	}
	varvara_render(v, rgba);
	for (i = 0; i < n; i++) { /* RGBA -> RGB in place */
		rgba[3 * i] = rgba[4 * i];
		rgba[3 * i + 1] = rgba[4 * i + 1];
		rgba[3 * i + 2] = rgba[4 * i + 2];
	}
	f = fopen(path, "wb");
	ok = f != NULL;
	if (f) {
		ok = fprintf(f, "P6\n%d %d\n255\n", v->width, v->height) > 0;
		ok = fwrite(rgba, 3, n, f) == n && ok;
		ok = fclose(f) == 0 && ok;
	}
	free(rgba);
	if (!ok) fprintf(stderr, "uxncli: --dump %s: cannot write\n", path);
	return !ok;
}

int main(int argc, char **argv)
{
	unsigned long max_steps = 0, frames = 0, i;
	const char *root = NULL, *dump = NULL;
	int ri = 1, code;

#ifdef _WIN32
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
#endif
	while (ri + 1 < argc) {
		if (strcmp(argv[ri], "--max-steps") == 0) {
			if (parse_count("--max-steps", argv[ri + 1], &max_steps)) return 1;
		} else if (strcmp(argv[ri], "--frames") == 0) {
			if (parse_count("--frames", argv[ri + 1], &frames)) return 1;
		} else if (strcmp(argv[ri], "--dump") == 0) {
			dump = argv[ri + 1];
		} else if (strcmp(argv[ri], "--root") == 0) {
			root = argv[ri + 1];
		} else {
			break;
		}
		ri += 2;
	}
	if (ri >= argc) {
		fprintf(stderr, "usage: uxncli [--max-steps N] [--root DIR] [--frames N] [--dump out.ppm] rom.rom [args...]\n");
		return 1;
	}
	varvara_init(&vv);
	vv.on_console = on_console;
	if (root && varvara_set_root(&vv, root)) {
		fprintf(stderr, "uxncli: --root %s: not a directory\n", root);
		return 1;
	}
	if (varvara_load(&vv, argv[ri])) return 1;
	/* boot wants uxnmin-style argv: [0] program, [1] ROM, args from [2] */
	if (varvara_boot(&vv, argc - ri + 1, argv + ri - 1, max_steps) == 0
	    && !frames && !dump && (vv.u.dev[0x10] | vv.u.dev[0x11])) {
		while (!varvara_halted(&vv)) {
			int c = fgetc(stdin);
			if (c == EOF) break;
			varvara_console_input(&vv, c, 1);
		}
		varvara_console_input(&vv, '\n', 4);
	}
	for (i = 0; i < frames && !varvara_halted(&vv); i++)
		varvara_frame(&vv, max_steps);
	code = dump ? dump_ppm(&vv, dump) : 0;
	varvara_close(&vv);
	fflush(stdout);
	return code ? code : varvara_exit_code(&vv);
}
