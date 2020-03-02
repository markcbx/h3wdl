#ifndef md5_engine_h__
#define md5_engine_h__
#include <stddef.h>
#include <limits.h>

class md5_engine
{
public:
#if (UINT_MAX==0xFFFFFFFFULL)
	typedef unsigned int  u32_t;
#elif (ULONG_MAX==0xFFFFFFFFULL)
	typedef unsigned long  u32_t;
#else
#	error PLEASE Modify u32_t define
#endif
	typedef unsigned char u8_t;

public:
	md5_engine(){}
	void digest(const void *data,size_t dataLen, void* out);
	// Updates the digest with additional message data.
	void init();
	void update(const void* a_data, u32_t a_data_size);
	void final(void* out);
private:
	md5_engine(const md5_engine&);
	void operator=(const md5_engine&);

private:
	// Transforms the next message block and updates the state.
	void process_block(const u8_t (*a_block)[64]);

	u32_t state[4];
	u32_t count[2];   // Number of bits mod 2^64.
	u8_t  buffer[64];  // Input buffer.
};

#endif//MD5_H
