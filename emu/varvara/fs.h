/* Platform shim for the File device: the only place that touches OS file APIs.
 * Paths are UTF-8 on every platform (converted to UTF-16 on Windows). */
#ifndef VARVARA_FS_H
#define VARVARA_FS_H

#include <stdio.h>
#include <stddef.h>

typedef struct FsDir FsDir;

/* fopen with a UTF-8 path. */
FILE *fs_fopen(const char *path, const char *mode);
/* 0 if path exists. *size gets the low 32 bits of the size, *is_dir 0 or 1. */
int fs_stat(const char *path, unsigned long *size, int *is_dir);
/* 0 on success. Removes files only, not directories. */
int fs_unlink(const char *path);
/* 0 on success. */
int fs_mkdir(const char *path);
/* NULL on failure. */
FsDir *fs_opendir(const char *path);
/* 1 and a NUL-terminated UTF-8 name (including "." and ".."), or 0 at the end. */
int fs_readdir(FsDir *d, char *name, size_t cap);
void fs_closedir(FsDir *d);
/* Absolute, normalized ('.' and '..' resolved), '/' separators, no trailing
 * separator except on a root. 0 on success. */
int fs_fullpath(const char *in, char *out, size_t cap);

#endif
