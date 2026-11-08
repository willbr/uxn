/* File devices (0xa0 and 0xb0). Behaviour follows the file_* functions of
 * the uxn11 reference; all OS access goes through fs.h. */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "varvara.h"
#include "fs.h"

enum { IDLE, FILE_READ, FILE_WRITE, DIR_READ, DIR_WRITE };

#define PATH_MAX_LEN 4096 /* longest path a ROM may name, including the NUL */
#define NAME_MAX_LEN 1024 /* longest directory entry name we list */

static int is_sep(char c)
{
	return c == '/';
}

static char fold(char c)
{
#ifdef _WIN32
	if (c >= 'A' && c <= 'Z') return (char)(c - 'A' + 'a');
#endif
	return c;
}

/* True if the absolute normalized path is root or lies below it. */
static int path_in_root(const char *root, const char *path)
{
	size_t n = strlen(root), i;

	for (i = 0; i < n; i++)
		if (fold(path[i]) != fold(root[i])) return 0;
	if (path[n] == '\0') return 1;
	return is_sep(root[n - 1]) || is_sep(path[n]);
}

int varvara_set_root(Varvara *v, const char *dir)
{
	char full[sizeof v->root];
	unsigned long size;
	int is_dir;

	if (fs_fullpath(dir, full, sizeof full) != 0) return 1;
	if (fs_stat(full, &size, &is_dir) != 0 || !is_dir) return 1;
	memcpy(v->root, full, sizeof full);
	v->has_root = 1;
	return 0;
}

/* Copies the ROM's path string into out (cap bytes) and applies the root.
 * Returns 0 if the path is unterminated, too long, or outside the root. */
static int get_path(Varvara *v, const VarvaraFile *f, char *out, size_t cap)
{
	char raw[PATH_MAX_LEN];
	size_t n = 0;
	unsigned a = f->name;

	if (!f->has_name) return 0;
	while (a < 0x10000 && n < sizeof raw) {
		raw[n] = (char)v->ram[a++];
		if (raw[n++] == '\0') break;
	}
	if (raw[n - 1] != '\0') return 0;
	if (!v->has_root) {
		if (n > cap) return 0;
		memcpy(out, raw, n);
		return 1;
	}
	if (fs_fullpath(raw, out, cap) != 0 || !path_in_root(v->root, out)) return 0;
	/* a trailing '/' names a directory; normalization dropped it */
	if (n >= 2 && is_sep(raw[n - 2])) {
		size_t m = strlen(out);
		if (m > 0 && !is_sep(out[m - 1])) {
			if (m + 2 > cap) return 0;
			out[m] = '/';
			out[m + 1] = '\0';
		}
	}
	return 1;
}

static int is_dir_path(const char *p)
{
	size_t n = strlen(p);
	return n > 0 && is_sep(p[n - 1]);
}

static int is_dir_real(const char *p)
{
	unsigned long size;
	int is_dir;
	return fs_stat(p, &size, &is_dir) == 0 && is_dir;
}

/* Creates every directory named before a '/' in p (like mkdir -p). */
static void write_dir(const char *p)
{
	char buf[PATH_MAX_LEN];
	size_t n;

	for (n = 0; p[n] && n < sizeof buf - 1; n++) {
		buf[n] = p[n];
		if (is_sep(p[n]) && n > 0) {
			buf[n] = '\0';
			if (!is_dir_real(buf)) fs_mkdir(buf);
			buf[n] = p[n];
		}
	}
}

static void file_reset(VarvaraFile *f)
{
	if (f->state == FILE_READ || f->state == FILE_WRITE) fclose(f->f);
	else if (f->state == DIR_READ) fs_closedir(f->dir);
	f->f = NULL;
	f->dir = NULL;
	f->state = IDLE;
}

void varvara_close(Varvara *v)
{
	file_reset(&v->file[0]);
	file_reset(&v->file[1]);
	free(v->banks);
	v->banks = NULL;
}

static int file_open(VarvaraFile *f, const char *path, int want_write, ux_u8 flags)
{
	if (want_write) {
		if (f->state == FILE_WRITE || f->state == DIR_WRITE) return 1;
		file_reset(f);
		write_dir(path);
		if (is_dir_path(path)) {
			f->state = is_dir_real(path) ? DIR_WRITE : IDLE;
		} else if ((f->f = fs_fopen(path, (flags & 0x01) ? "ab" : "wb")) != NULL) {
			f->state = FILE_WRITE;
		}
	} else {
		if (f->state == FILE_READ || f->state == DIR_READ) return 1;
		file_reset(f);
		if (is_dir_real(path)) {
			if ((f->dir = fs_opendir(path)) != NULL) f->state = DIR_READ;
		} else if ((f->f = fs_fopen(path, "rb")) != NULL) {
			f->state = FILE_READ;
		}
	}
	return f->state != IDLE;
}

/* Writes the 'len'-character stat field: '-' for a directory, '!' for an
 * error, '?' if size does not fit in len hex digits, else the hex size. */
static void put_stat(ux_u8 *dest, unsigned len, int err, int dir, unsigned long size)
{
	unsigned i;

	size &= 0xffffffffUL;
	if (dir || err || (len < 8 && size >= (1UL << (len * 4)))) {
		for (i = 0; i < len; i++) dest[i] = (ux_u8)(dir ? '-' : err ? '!' : '?');
		return;
	}
	for (i = len; i > 0; i--) {
		dest[i - 1] = (ux_u8)"0123456789abcdef"[size & 0xf];
		size >>= 4;
	}
}

/* Lists directory entries into dest[0..length) as "ssss\tname\n" (name/ for
 * a directory), stopping when no room is left. An entry that does not fit is
 * cut off at the end of the buffer. Returns the byte count. */
static unsigned put_fdir(ux_u8 *dest, unsigned length, const char *path, FsDir *dir)
{
	char pathbuf[0x200];
	char name[NAME_MAX_LEN];
	char ent[NAME_MAX_LEN + 16];
	unsigned p = 0, prefix = 0;
	char c = '/';

	while (path[prefix] && prefix < sizeof pathbuf - 1) {
		c = path[prefix];
		pathbuf[prefix++] = c;
	}
	if (c != '/' && prefix < sizeof pathbuf - 1) pathbuf[prefix++] = '/';
	while (p + 1 < length && fs_readdir(dir, name, sizeof name)) {
		unsigned long size = 0;
		int is_dir = 0, err;
		size_t nl = strlen(name), room = sizeof pathbuf - 1 - prefix, e;

		if (name[0] == '.' && (!name[1] || (name[1] == '.' && !name[2]))) continue;
		memcpy(pathbuf + prefix, name, nl < room ? nl : room);
		pathbuf[prefix + (nl < room ? nl : room)] = '\0';
		err = fs_stat(pathbuf, &size, &is_dir) != 0;
		put_stat((ux_u8 *)ent, 4, err, !err && is_dir, size);
		ent[4] = '\t';
		memcpy(ent + 5, name, nl);
		e = 5 + nl;
		if (!err && is_dir) ent[e++] = '/';
		ent[e++] = '\n';
		if (e > length - p) e = length - p;
		memcpy(dest + p, ent, e);
		p += (unsigned)e;
	}
	if (p < length) dest[p] = 0;
	return p;
}

static unsigned file_read(Varvara *v, VarvaraFile *f, unsigned addr, unsigned len)
{
	char path[PATH_MAX_LEN];
	unsigned n, length = len;

	if (!get_path(v, f, path, sizeof path)) return 0;
	if (addr + len > 0x10000) length = 0x10000 - addr;
	if (!file_open(f, path, 0, 0)) return 0;
	if (f->state == FILE_READ) n = (unsigned)fread(v->ram + addr, 1, length, f->f);
	else n = put_fdir(v->ram + addr, length, path, f->dir);
	if (len > 0 && n == 0) file_reset(f);
	return n;
}

static unsigned file_write(Varvara *v, VarvaraFile *f, unsigned addr, unsigned len, ux_u8 flags)
{
	char path[PATH_MAX_LEN];

	if (!get_path(v, f, path, sizeof path)) return 0;
	if (addr + len > 0x10000) len = 0x10000 - addr;
	if (!file_open(f, path, 1, flags)) return 0;
	if (f->state == FILE_WRITE) {
		unsigned ret = (unsigned)fwrite(v->ram + addr, 1, len, f->f);
		return (ret > 0 && fflush(f->f) != 0) ? 0 : ret;
	}
	return (unsigned)is_dir_real(path);
}

static unsigned file_stat(Varvara *v, VarvaraFile *f, unsigned addr, unsigned len)
{
	char path[PATH_MAX_LEN];
	unsigned long size = 0;
	int is_dir = 0, err;

	if (!get_path(v, f, path, sizeof path)) return 0;
	if (addr + len > 0x10000) len = 0x10000 - addr;
	err = fs_stat(path, &size, &is_dir) != 0;
	put_stat(v->ram + addr, len, err, !err && is_dir, size);
	return len;
}

static unsigned file_delete(Varvara *v, VarvaraFile *f)
{
	char path[PATH_MAX_LEN];

	if (!get_path(v, f, path, sizeof path)) return 0;
	return fs_unlink(path) == 0;
}

ux_u8 file_dei(Varvara *v, ux_u8 port)
{
	return v->u.dev[port];
}

void file_deo(Varvara *v, ux_u8 port)
{
	ux_u8 *d = v->u.dev;
	unsigned base = port & 0xf0, id = base == 0xb0;
	VarvaraFile *f = &v->file[id];
	unsigned len = (unsigned)d[base + 0xa] << 8 | d[base + 0xb];
	unsigned r;

	/* Short ports arrive on their low byte: stat 0x5, name 0x9, read 0xd, write 0xf. */
	switch (port & 0x0f) {
	case 0x5: r = file_stat(v, f, (unsigned)d[base + 4] << 8 | d[base + 5], len); break;
	case 0x6: r = file_delete(v, f); break;
	case 0x9:
		file_reset(f);
		f->name = (unsigned)d[base + 8] << 8 | d[base + 9];
		f->has_name = 1;
		r = 0;
		break;
	case 0xd: r = file_read(v, f, (unsigned)d[base + 0xc] << 8 | d[base + 0xd], len); break;
	case 0xf: r = file_write(v, f, (unsigned)d[base + 0xe] << 8 | d[base + 0xf], len, d[base + 7]); break;
	default: return;
	}
	d[base + 2] = (ux_u8)(r >> 8);
	d[base + 3] = (ux_u8)r;
}

const char *file_port_name(ux_u8 port)
{
	static const char *const names[16] = {
		"File/vector", "File/vector", "File/success", "File/success",
		"File/stat", "File/stat", "File/delete", "File/append",
		"File/name", "File/name", "File/length", "File/length",
		"File/read", "File/read", "File/write", "File/write"
	};
	return names[port & 0x0f];
}
