/* Headless Varvara host: uxncli [--max-steps N] rom.rom [args...] */
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

int main(int argc, char **argv)
{
	unsigned long max_steps = 0;
	const char *root = NULL;
	int ri = 1;

#ifdef _WIN32
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
#endif
	while (ri + 1 < argc) {
		if (strcmp(argv[ri], "--max-steps") == 0) {
			char *end;
			max_steps = strtoul(argv[ri + 1], &end, 10);
			if (*argv[ri + 1] == '\0' || *end != '\0') {
				fprintf(stderr, "uxncli: bad --max-steps value: %s\n", argv[ri + 1]);
				return 1;
			}
		} else if (strcmp(argv[ri], "--root") == 0) {
			root = argv[ri + 1];
		} else {
			break;
		}
		ri += 2;
	}
	if (ri >= argc) {
		fprintf(stderr, "usage: uxncli [--max-steps N] [--root DIR] rom.rom [args...]\n");
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
	    && (vv.u.dev[0x10] | vv.u.dev[0x11])) {
		while (!varvara_halted(&vv)) {
			int c = fgetc(stdin);
			if (c == EOF) break;
			varvara_console_input(&vv, c, 1);
		}
		varvara_console_input(&vv, '\n', 4);
	}
	varvara_close(&vv);
	fflush(stdout);
	return varvara_exit_code(&vv);
}
