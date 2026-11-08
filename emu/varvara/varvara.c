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
	case 0x20: return screen_dei(v, port);
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
	case 0x20: screen_deo(v, port); break;
	/* Controller (0x80) and Mouse (0x90) ports are plain storage; see input.c */
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
	screen_init(v);
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

void varvara_args_begin(Varvara *v, VarvaraArgs *a, int n, char **arg)
{
	a->arg = arg;
	a->n = n > 0 ? n : 0;
	a->i = 0;
	a->pos = 0;
	a->started = 0;
	v->u.dev[0x17] = (ux_u8)(a->n > 0);
}

int varvara_args_deliver(Varvara *v, VarvaraArgs *a)
{
	v->limit_hit = 0;
	if (!a->started) {
		a->started = 1;
		if (!(v->u.dev[0x10] | v->u.dev[0x11])) a->i = a->n; /* no Console vector */
	}
	while (a->i < a->n) {
		const char *s = a->arg[a->i];

		/* advance before running, so a resumed byte is never sent twice */
		if (!varvara_halted(v) && s[a->pos]) {
			varvara_console_input(v, (unsigned char)s[a->pos++], 2);
		} else {
			int last = a->i == a->n - 1;

			a->i++;
			a->pos = 0;
			varvara_console_input(v, '\n', 3 + last);
		}
		if (v->limit_hit) return 0;
	}
	return 1;
}

int varvara_boot(Varvara *v, int argc, char **argv, unsigned long max_steps)
{
	VarvaraArgs a;
	int r;

	v->max_steps = max_steps;
	varvara_args_begin(v, &a, argc - 2, argc > 2 ? argv + 2 : NULL);
	r = varvara_run(v, VARVARA_RESET, max_steps);
	v->u.fault = 0;
	if (r != UXN_BRK) return 1;
	return !varvara_args_deliver(v, &a);
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
	case 0x20: return screen_port_name(port);
	case 0x80: return controller_port_name(port);
	case 0x90: return mouse_port_name(port);
	case 0xa0: case 0xb0: return file_port_name(port);
	case 0xc0: return datetime_port_name(port);
	}
	return NULL;
}
