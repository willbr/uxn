/* Platform shim for the File device. Windows uses the wide-char Win32 APIs
 * with UTF-8 <-> UTF-16 conversion; everything else uses dirent, stat and
 * stdio. */
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "fs.h"

#define WMAX 4096

struct FsDir {
	HANDLE h;
	WIN32_FIND_DATAW fd;
	int have; /* fd holds an unreturned entry */
};

static int to_wide(const char *s, WCHAR *w, int cap)
{
	return MultiByteToWideChar(CP_UTF8, 0, s, -1, w, cap) > 0;
}

static int to_utf8(const WCHAR *w, char *s, size_t cap)
{
	return cap <= 0x7fffffff && WideCharToMultiByte(CP_UTF8, 0, w, -1, s, (int)cap, NULL, NULL) > 0;
}

FILE *fs_fopen(const char *path, const char *mode)
{
	WCHAR wp[WMAX], wm[8];
	int i;

	if (!to_wide(path, wp, WMAX)) return NULL;
	for (i = 0; mode[i] && i < 7; i++) wm[i] = (WCHAR)(unsigned char)mode[i];
	wm[i] = 0;
	return _wfopen(wp, wm);
}

int fs_stat(const char *path, unsigned long *size, int *is_dir)
{
	WCHAR wp[WMAX];
	WIN32_FILE_ATTRIBUTE_DATA d;

	if (!to_wide(path, wp, WMAX)) return 1;
	if (!GetFileAttributesExW(wp, GetFileExInfoStandard, &d)) return 1;
	*size = (unsigned long)d.nFileSizeLow;
	*is_dir = (d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	return 0;
}

int fs_unlink(const char *path)
{
	WCHAR wp[WMAX];

	if (!to_wide(path, wp, WMAX)) return 1;
	return DeleteFileW(wp) ? 0 : 1;
}

int fs_mkdir(const char *path)
{
	WCHAR wp[WMAX];

	if (!to_wide(path, wp, WMAX)) return 1;
	return CreateDirectoryW(wp, NULL) ? 0 : 1;
}

FsDir *fs_opendir(const char *path)
{
	WCHAR wp[WMAX];
	size_t n;
	FsDir *d;

	if (!to_wide(path, wp, WMAX - 2)) return NULL;
	n = wcslen(wp);
	if (n && wp[n - 1] != L'/' && wp[n - 1] != L'\\') wp[n++] = L'/';
	wp[n++] = L'*';
	wp[n] = 0;
	d = (FsDir *)malloc(sizeof *d);
	if (!d) return NULL;
	d->h = FindFirstFileW(wp, &d->fd);
	if (d->h == INVALID_HANDLE_VALUE) {
		free(d);
		return NULL;
	}
	d->have = 1;
	return d;
}

int fs_readdir(FsDir *d, char *name, size_t cap)
{
	for (;;) {
		if (!d->have) {
			if (!FindNextFileW(d->h, &d->fd)) return 0;
		}
		d->have = 0;
		if (to_utf8(d->fd.cFileName, name, cap)) return 1;
		/* name too long for the caller's buffer: skip it */
	}
}

void fs_closedir(FsDir *d)
{
	if (!d) return;
	FindClose(d->h);
	free(d);
}

int fs_fullpath(const char *in, char *out, size_t cap)
{
	WCHAR wp[WMAX], full[WMAX];
	size_t i, n;

	if (!to_wide(in, wp, WMAX)) return 1;
	if (!_wfullpath(full, wp, WMAX)) return 1;
	if (!to_utf8(full, out, cap)) return 1;
	n = strlen(out);
	for (i = 0; i < n; i++)
		if (out[i] == '\\') out[i] = '/';
	/* strip a trailing separator unless the path is a root ("C:/" or "/") */
	while (n > 1 && out[n - 1] == '/' && !(n == 3 && out[1] == ':')) out[--n] = 0;
	return 0;
}

#else /* POSIX */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fs.h"

struct FsDir {
	DIR *dir;
};

FILE *fs_fopen(const char *path, const char *mode)
{
	return fopen(path, mode);
}

int fs_stat(const char *path, unsigned long *size, int *is_dir)
{
	struct stat st;

	if (stat(path, &st) != 0) return 1;
	*size = (unsigned long)st.st_size & 0xffffffffUL;
	*is_dir = S_ISDIR(st.st_mode) ? 1 : 0;
	return 0;
}

int fs_unlink(const char *path)
{
	return unlink(path);
}

int fs_mkdir(const char *path)
{
	return mkdir(path, 0755);
}

FsDir *fs_opendir(const char *path)
{
	FsDir *d = (FsDir *)malloc(sizeof *d);

	if (!d) return NULL;
	d->dir = opendir(path);
	if (!d->dir) {
		free(d);
		return NULL;
	}
	return d;
}

int fs_readdir(FsDir *d, char *name, size_t cap)
{
	struct dirent *de;

	while ((de = readdir(d->dir)) != NULL) {
		if (strlen(de->d_name) < cap) {
			strcpy(name, de->d_name);
			return 1;
		}
	}
	return 0;
}

void fs_closedir(FsDir *d)
{
	if (!d) return;
	closedir(d->dir);
	free(d);
}

/* Lexical normalization; the path need not exist. */
int fs_fullpath(const char *in, char *out, size_t cap)
{
	char tmp[4096];
	size_t n = 0, o = 0;
	const char *p;

	if (in[0] == '/') {
		tmp[0] = 0;
	} else {
		if (!getcwd(tmp, sizeof tmp)) return 1;
	}
	n = strlen(tmp);
	if (n + 1 + strlen(in) + 1 > sizeof tmp) return 1;
	tmp[n++] = '/';
	strcpy(tmp + n, in);
	if (cap < 2) return 1;
	p = tmp;
	while (*p) {
		size_t len = 0;
		while (*p == '/') p++;
		while (p[len] && p[len] != '/') len++;
		if (len == 0) break;
		if (len == 1 && p[0] == '.') {
			/* skip */
		} else if (len == 2 && p[0] == '.' && p[1] == '.') {
			while (o > 0 && out[o - 1] != '/') o--;
			if (o > 0) o--;
		} else {
			if (o + len + 2 > cap) return 1;
			out[o++] = '/';
			memcpy(out + o, p, len);
			o += len;
		}
		p += len;
	}
	if (o == 0) out[o++] = '/';
	out[o] = 0;
	return 0;
}

#endif
