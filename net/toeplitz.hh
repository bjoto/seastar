/*-
 * Copyright (c) 2010 David Malone <dwmalone@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef TOEPLITZ_HH_
#define TOEPLITZ_HH_

#include <array>

constexpr size_t rss_keysize = 40;
using rss_key_type = std::array<uint8_t, rss_keysize>;

// Mellanox Linux's driver key in network byte order
static const rss_key_type rsskey = {
    0xd1,0x81,0xc6,0x2c,0xf7,0xf4,0xdb,0x5b,
    0x19,0x83,0xa2,0xfc,0x94,0x3e,0x1a,0xdb,
    0xd9,0x38,0x9e,0x6b,0xd1,0x03,0x9c,0x2c,
    0xa7,0x44,0x99,0xad,0x59,0x3d,0x56,0xd9,
    0xf3,0x25,0x3c,0x06,0x2a,0xdc,0x1f,0xfc
};

template<typename T>
static inline uint32_t
toeplitz_hash(const rss_key_type& key, const T& data)
{
	uint32_t hash = 0, v;
	u_int i, b;

	/* XXXRW: Perhaps an assertion about key length vs. data length? */

	v = (key[0]<<24) + (key[1]<<16) + (key[2] <<8) + key[3];
	for (i = 0; i < data.size(); i++) {
		for (b = 0; b < 8; b++) {
			if (data[i] & (1<<(7-b)))
				hash ^= v;
			v <<= 1;
			if ((i + 4) < key.size() &&
			    (key[i+4] & (1<<(7-b))))
				v |= 1;
		}
	}
	return (hash);
}
#endif
