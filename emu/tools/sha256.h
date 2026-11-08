/* Minimal SHA-256 (FIPS 180-4), header-only, C89. Used by nob.c to hash
 * screen snapshots without shelling out.
 *   char hex[65]; sha256_hex(data, len, hex); */
#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>

typedef unsigned long sha256_u32; /* at least 32 bits; values are masked */

static const sha256_u32 sha256_k[64] = {
	0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL, 0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
	0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL, 0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
	0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL, 0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
	0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL, 0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
	0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL, 0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
	0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL, 0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
	0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL, 0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
	0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL, 0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
};

#define SHA256_M(x) ((x) & 0xffffffffUL)
#define SHA256_ROR(x, n) SHA256_M(((x) >> (n)) | ((x) << (32 - (n))))

static void sha256_block(sha256_u32 h[8], const unsigned char *p)
{
	sha256_u32 w[64], a, b, c, d, e, f, g, hh, t1, t2;
	int i;

	for (i = 0; i < 16; i++)
		w[i] = (sha256_u32)p[4 * i] << 24 | (sha256_u32)p[4 * i + 1] << 16
			| (sha256_u32)p[4 * i + 2] << 8 | p[4 * i + 3];
	for (i = 16; i < 64; i++) {
		sha256_u32 s0 = SHA256_ROR(w[i - 15], 7) ^ SHA256_ROR(w[i - 15], 18) ^ (w[i - 15] >> 3);
		sha256_u32 s1 = SHA256_ROR(w[i - 2], 17) ^ SHA256_ROR(w[i - 2], 19) ^ (w[i - 2] >> 10);
		w[i] = SHA256_M(w[i - 16] + s0 + w[i - 7] + s1);
	}
	a = h[0]; b = h[1]; c = h[2]; d = h[3]; e = h[4]; f = h[5]; g = h[6]; hh = h[7];
	for (i = 0; i < 64; i++) {
		t1 = SHA256_M(hh + (SHA256_ROR(e, 6) ^ SHA256_ROR(e, 11) ^ SHA256_ROR(e, 25))
			+ ((e & f) ^ (~e & g)) + sha256_k[i] + w[i]);
		t2 = SHA256_M((SHA256_ROR(a, 2) ^ SHA256_ROR(a, 13) ^ SHA256_ROR(a, 22))
			+ ((a & b) ^ (a & c) ^ (b & c)));
		hh = g; g = f; f = e; e = SHA256_M(d + t1);
		d = c; c = b; b = a; a = SHA256_M(t1 + t2);
	}
	h[0] = SHA256_M(h[0] + a); h[1] = SHA256_M(h[1] + b);
	h[2] = SHA256_M(h[2] + c); h[3] = SHA256_M(h[3] + d);
	h[4] = SHA256_M(h[4] + e); h[5] = SHA256_M(h[5] + f);
	h[6] = SHA256_M(h[6] + g); h[7] = SHA256_M(h[7] + hh);
}

/* Writes the digest of data[0..len) as 64 lowercase hex digits plus NUL. */
static void sha256_hex(const void *data, size_t len, char out[65])
{
	static const char digits[] = "0123456789abcdef";
	sha256_u32 h[8] = {
		0x6a09e667UL, 0xbb67ae85UL, 0x3c6ef372UL, 0xa54ff53aUL,
		0x510e527fUL, 0x9b05688cUL, 0x1f83d9abUL, 0x5be0cd19UL
	};
	const unsigned char *p = (const unsigned char *)data;
	unsigned char tail[128];
	size_t i, rest = len % 64, tail_len = rest < 56 ? 64 : 128;
	/* the message length in bits as two 32-bit halves */
	sha256_u32 bits_lo = SHA256_M((sha256_u32)len << 3);
	sha256_u32 bits_hi = SHA256_M((sha256_u32)(len >> 29));

	for (i = 0; i + 64 <= len; i += 64) sha256_block(h, p + i);
	for (i = 0; i < tail_len; i++) tail[i] = 0;
	for (i = 0; i < rest; i++) tail[i] = p[len - rest + i];
	tail[rest] = 0x80;
	for (i = 0; i < 4; i++) {
		tail[tail_len - 1 - i] = (unsigned char)((bits_lo >> (8 * i)) & 0xff);
		tail[tail_len - 5 - i] = (unsigned char)((bits_hi >> (8 * i)) & 0xff);
	}
	sha256_block(h, tail);
	if (tail_len == 128) sha256_block(h, tail + 64);
	for (i = 0; i < 32; i++) {
		unsigned v = (unsigned)((h[i / 4] >> (24 - 8 * (i % 4))) & 0xff);
		out[2 * i] = digits[v >> 4];
		out[2 * i + 1] = digits[v & 0xf];
	}
	out[64] = '\0';
}

#endif
