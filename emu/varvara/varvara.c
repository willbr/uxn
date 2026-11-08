/* Varvara glue: init, ROM loading, boot, device dispatch, port names. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "varvara.h"

static ux_u8 varvara_dei(Uxn *u, ux_u8 port)
{
	Varvara *v = (Varvara *)u->user;
	switch (port & 0xf0) {
	case 0x00: return system_dei(v, port);
	case 0x10: return console_dei(v, port);
	case 0xa0: case 0xb0: return file_dei(v, port);
	case 0xc0: return datetime_dei(v, port);
	}
	return u->dev[port];
}

static void varvara_deo(Uxn *u, ux_u8 port)
{
	Varvara *v = (Varvara *)u->user;
	switch (port & 0xf0) {
	case 0x00: system_deo(v, port); break;
	case 0x10: console_deo(v, port); break;
	case 0xa0: case 0xb0: file_deo(v, port); break;
	case 0xc0: datetime_deo(v, port); break;
	}
}

int varvara_init(Varvara *v)
{
	memset(v, 0, sizeof *v);
	v->u.ram = v->ram;
	v->u.dei = varvara_dei;
	v->u.deo = varvara_deo;
	v->u.user = v;
	v->banks = (ux_u8 *)calloc(15, 0x10000);
	return 0;
}

int varvara_load(Varvara *v, const char *path)
{
	FILE *f = fopen(path, "rb");
	size_t n;

	if (!f) {
		fprintf(stderr, "%s: cannot open ROM\n", path);
		return 1;
	}
	n = fread(v->ram + 0x100, 1, 0xff00, f);
	if (n == 0) {
		fprintf(stderr, "%s: %s\n", path, ferror(f) ? "cannot read ROM" : "ROM is empty");
		fclose(f);
		return 1;
	}
	if (n == 0xff00 && fgetc(f) != EOF) {
		fprintf(stderr, "%s: ROM is larger than 65280 bytes\n", path);
		fclose(f);
		return 1;
	}
	fclose(f);
	return 0;
}

int varvara_boot(Varvara *v, int argc, char **argv, unsigned long max_steps)
{
	int i, r;

	v->max_steps = max_steps;
	v->u.dev[0x17] = (ux_u8)(argc > 2);
	r = uxn_eval(&v->u, 0x100, max_steps);
	v->u.fault = 0;
	if (r != UXN_BRK) return 1;
	if (!(v->u.dev[0x10] | v->u.dev[0x11])) return 0;
	for (i = 2; i < argc; i++) {
		const char *p = argv[i];
		while (!varvara_halted(v) && *p)
			varvara_console_input(v, (unsigned char)*p++, 2);
		varvara_console_input(v, '\n', 3 + (i == argc - 1));
	}
	return 0;
}

int varvara_halted(const Varvara *v)
{
	return v->u.dev[0x0f] != 0;
}

int varvara_exit_code(const Varvara *v)
{
	return v->u.dev[0x0f] & 0x7f;
}

const char *varvara_port_name(ux_u8 port)
{
	switch (port & 0xf0) {
	case 0x00: return system_port_name(port);
	case 0x10: return console_port_name(port);
	case 0xa0: case 0xb0: return file_port_name(port);
	case 0xc0: return datetime_port_name(port);
	}
	return NULL;
}
