#include "md5_engine.h"
#include <cassert>
#include <cstring>

/*
Guangzhu modified.
*/
/*
"The [MD5] algorithm takes as input a message of arbitrary length and
produces as output a 128-bit "fingerprint" or "message digest" of the
input. It is conjectured that it is computationally infeasible to produce
two messages having the same message digest, or to produce any message
having a given prespecified target message digest. ... The MD5 algorithm
is designed to be quite fast on 32-bit machines." -RFC1321
*/
/*
See http://www.boost.org for updates and documentation.

Copyright (C) 2002-2003 Stanislav Baranov. Permission to copy, use,
modify, sell and distribute this software and its documentation is
granted provided this copyright notice appears in all copies. This
software is provided "as is" without express or implied warranty,
and with no claim as to its suitability for any purpose. Derived
from the RSA Data Security, Inc. MD5 Message-Digest Algorithm.

Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All rights
reserved. License to copy and use this software is granted provided that
it is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software or
this function. License is also granted to make and use derivative works
provided that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material mentioning
or referencing the derived work. RSA Data Security, Inc. makes no
representations concerning either the merchantability of this software
or the suitability of this software for any particular purpose. It is
provided "as is" without express or implied warranty of any kind. These
notices must be retained in any copies of any part of this documentation
and/or software.
*/


#if defined (__GLIBC__)
# include <endian.h>
# if (__BYTE_ORDER == __LITTLE_ENDIAN)
#  define BYTE_LITTLE_ENDIAN 1
# elif (__BYTE_ORDER == __BIG_ENDIAN)
#  define BYTE_BIG_ENDIAN 1
# elif (__BYTE_ORDER == __PDP_ENDIAN)
#  define BYTE_PDP_ENDIAN 1
# else
#  error Unknown machine endianness detected.
# endif
# define BYTE_BYTE_ORDER __BYTE_ORDER
#elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN) || \
	defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__) || \
	defined(_STLP_BIG_ENDIAN) && !defined(_STLP_LITTLE_ENDIAN)
# define BYTE_BIG_ENDIAN 1
# define BYTE_BYTE_ORDER 4321
#elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN) || \
	defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__) || \
	defined(_STLP_LITTLE_ENDIAN) && !defined(_STLP_BIG_ENDIAN)
# define BYTE_LITTLE_ENDIAN 1
# define BYTE_BYTE_ORDER 1234
#elif defined(__sparc) || defined(__sparc__) \
	|| defined(_POWER) || defined(__powerpc__) \
	|| defined(__ppc__) || defined(__hpux) || defined(__hppa) \
	|| defined(_MIPSEB) || defined(_POWER) \
	|| defined(__s390__)
# define BYTE_BIG_ENDIAN 1
# define BYTE_BYTE_ORDER 4321
#elif defined(__i386__) || defined(__alpha__) \
	|| defined(__ia64) || defined(__ia64__) \
	|| defined(_M_IX86) || defined(_M_IA64) \
	|| defined(_M_ALPHA) || defined(__amd64) \
	|| defined(__amd64__) || defined(_M_AMD64) \
	|| defined(__x86_64) || defined(__x86_64__) \
	|| defined(_M_X64) || defined(__bfin__)

# define BYTE_LITTLE_ENDIAN 1
# define BYTE_BYTE_ORDER 1234
#else
# error The file byteorder.h needs to be set up for your CPU type.
#endif

namespace 
{
	typedef md5_engine::u8_t  u8_t;
	typedef md5_engine::u32_t u32_t;

#if defined(_MSC_VER) && (ULONG_MAX==0xFFFFFFFFULL) && 0
#	include <stdlib.h>
#	define bswap_32 _byteswap_ulong
#else
	inline u32_t bswap_32(u32_t val)
	{
		return (
			((val) >> 24) | 
			((val & 0x00FF0000) >> 8) |
			((val & 0x0000FF00) << 8) | 
			((val) << 24)
			);
	}
#endif

	/*
	Pack/unpack between arrays of uint8_t and uint32_t in
	a platform-independent way. Size is a multiple of 4.
	*/
#if defined(BYTE_BIG_ENDIAN)
	inline void pack(void* dst, const void* src, u32_t size) 
	{
		size>>=2;
		for (u32_t i=0;i<size;++i)
			(((u32_t*)dst)[i])=bswap_32(((const u32_t*)src)[i]);
	}
	void unpack(u32_t* dst, const u8_t* src, u32_t size) {
		pack((u8_t*)dst,(const u32_t*)src,size);
	}
#else
#	define pack		memcpy
#	define unpack	memcpy
#endif

	const u32_t S11(7);
	const u32_t S12(12);
	const u32_t S13(17);
	const u32_t S14(22);
	const u32_t S21(5);
	const u32_t S22(9);
	const u32_t S23(14);
	const u32_t S24(20);
	const u32_t S31(4);
	const u32_t S32(11);
	const u32_t S33(16);
	const u32_t S34(23);
	const u32_t S41(6);
	const u32_t S42(10);
	const u32_t S43(15);
	const u32_t S44(21);

	inline u32_t rol(u32_t x, u32_t n_bits) { return (x << n_bits) | (x >> (32-n_bits)); }

	/*
	Basic MD5 functions.
	*/

	inline u32_t F(u32_t x, u32_t y, u32_t z) { return (x & y) | (~x & z); }
	inline u32_t G(u32_t x, u32_t y, u32_t z) { return (x & z) | (y & ~z); }
	inline u32_t H(u32_t x, u32_t y, u32_t z) { return x ^ y ^ z; }
	inline u32_t I(u32_t x, u32_t y, u32_t z) { return y ^ (x | ~z); }

	/*
	Transformations for rounds 1, 2, 3, and 4.
	*/

	inline void FF(u32_t& a, u32_t b, u32_t c, u32_t d, u32_t x, u32_t  s, u32_t ac)
	{
		a += F(b, c, d) + x + ac;
		a = rol(a, s) + b;
	}

	inline void GG(u32_t& a, u32_t b, u32_t c, u32_t d, u32_t x, u32_t s, u32_t ac)
	{
		a += G(b, c, d) + x + ac;
		a = rol(a, s) + b;
	}

	inline void HH(u32_t& a, u32_t b, u32_t c, u32_t d, u32_t x, u32_t s, u32_t ac)
	{
		a += H(b, c, d) + x + ac;
		a = rol(a, s) + b;
	}

	inline void II(u32_t& a, u32_t b, u32_t c, u32_t d, u32_t x, u32_t s, u32_t ac)
	{
		a += I(b, c, d) + x + ac;
		a = rol(a, s) + b;
	}
}


void md5_engine::update(const void* a_data, u32_t a_data_size)
{
	// Compute number of bytes mod 64.
	u32_t buffer_index = static_cast<u32_t>((count[0] >> 3) & 0x3f);

	// Update number of bits.
	count[0] += (static_cast<u32_t>(a_data_size) << 3);
	if (count[0] < (static_cast<u32_t>(a_data_size) << 3))
		++count[1];

	count[1] += (static_cast<u32_t>(a_data_size) >> 29);

	u32_t buffer_space = 64-buffer_index;  // Remaining buffer space.

	u32_t input_index;

	// Transform as many times as possible.
	if (a_data_size >= buffer_space) {
		memcpy(buffer+buffer_index, a_data, buffer_space);
		process_block(&buffer);
		for (input_index = buffer_space; input_index+63 < a_data_size; input_index += 64) {
			process_block(reinterpret_cast<const u8_t(*)[64]>(
				reinterpret_cast<const u8_t*>(a_data)+input_index));
		}

		buffer_index = 0;
	} else {
		input_index = 0;
	}

	// Buffer remaining input.
	memcpy(buffer+buffer_index, reinterpret_cast<
		const u8_t*>(a_data)+input_index, a_data_size-input_index);
}

void md5_engine::init()
{
	count[0] = 0;
	count[1] = 0;

	state[0] = 0x67452301;
	state[1] = 0xefcdab89;
	state[2] = 0x98badcfe;
	state[3] = 0x10325476;
}

void md5_engine::final(void* out)
{
	static const u8_t padding[64] =
	{
		0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	// Save number of bits.
	u8_t saved_count[8];
	pack(saved_count, count, 8);

	// Pad out to 56 mod 64.
	u32_t index = static_cast<u32_t>((count[0] >> 3) & 0x3f);
	u32_t padding_size = (index < 56) ? (56 - index) : (120 - index);
	update(padding, padding_size);

	// Append size before padding.
	update(saved_count, 8);

	// Store state in digest.
	pack((u8_t*)out, state, 16);
}

void md5_engine::digest(const void *a_data,size_t dataLen, void* out)
{
	count[0] = 0;
	count[1] = 0;

	state[0] = 0x67452301;
	state[1] = 0xefcdab89;
	state[2] = 0x98badcfe;
	state[3] = 0x10325476;

	if (a_data!=0) update(a_data,(u32_t)dataLen);


	static const u8_t padding[64] =
	{
		0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	// Save number of bits.
	u8_t saved_count[8];
	pack(saved_count, count, 8);

	// Pad out to 56 mod 64.
	u32_t index = static_cast<u32_t>((count[0] >> 3) & 0x3f);
	u32_t padding_size = (index < 56) ? (56 - index) : (120 - index);
	update(padding, padding_size);

	// Append size before padding.
	update(saved_count, 8);

	// Store state in digest.
	pack((u8_t*)out, state, 16);
}


void md5_engine::process_block(const u8_t (*a_block)[64])
{
	u32_t a(state[0]);
	u32_t b(state[1]);
	u32_t c(state[2]);
	u32_t d(state[3]);

	/*volatile*/ u32_t x[16];

	unpack(x, reinterpret_cast<const u8_t*>(a_block), 64);

	// Round 1.
	FF(a, b, c, d, x[ 0], S11, 0xd76aa478); /*  1 */
	FF(d, a, b, c, x[ 1], S12, 0xe8c7b756); /*  2 */
	FF(c, d, a, b, x[ 2], S13, 0x242070db); /*  3 */
	FF(b, c, d, a, x[ 3], S14, 0xc1bdceee); /*  4 */
	FF(a, b, c, d, x[ 4], S11, 0xf57c0faf); /*  5 */
	FF(d, a, b, c, x[ 5], S12, 0x4787c62a); /*  6 */
	FF(c, d, a, b, x[ 6], S13, 0xa8304613); /*  7 */
	FF(b, c, d, a, x[ 7], S14, 0xfd469501); /*  8 */
	FF(a, b, c, d, x[ 8], S11, 0x698098d8); /*  9 */
	FF(d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
	FF(c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
	FF(b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
	FF(a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
	FF(d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
	FF(c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
	FF(b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

	// Round 2.
	GG(a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
	GG(d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
	GG(c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
	GG(b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
	GG(a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
	GG(d, a, b, c, x[10], S22,  0x2441453); /* 22 */
	GG(c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
	GG(b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
	GG(a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
	GG(d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
	GG(c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
	GG(b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
	GG(a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
	GG(d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
	GG(c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
	GG(b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

	// Round 3.
	HH(a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
	HH(d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
	HH(c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
	HH(b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
	HH(a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
	HH(d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
	HH(c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
	HH(b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
	HH(a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
	HH(d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
	HH(c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
	HH(b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
	HH(a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
	HH(d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
	HH(c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
	HH(b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

	// Round 4.
	II(a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
	II(d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
	II(c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
	II(b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
	II(a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
	II(d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
	II(c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
	II(b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
	II(a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
	II(d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
	II(c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
	II(b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
	II(a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
	II(d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
	II(c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
	II(b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;

	// Zeroize sensitive information.
	memset(reinterpret_cast<u8_t*>(x), 0, sizeof(x));
}
