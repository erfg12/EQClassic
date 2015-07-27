#ifndef CRC32_H
#define CRC32_H
#include "types.h"

class CRC32 {
public:
	// one buffer CRC32
	static uint32			Generate(const int8* buf, uint32 bufsize);

	// Multiple buffer CRC32
	static uint32			Update(const int8* buf, uint32 bufsize, uint32 crc32 = 0xFFFFFFFF);
	static inline uint32	Finish(uint32 crc32)	{ return ~crc32; }
	static inline void		Finish(uint32* crc32)	{ *crc32 = ~(*crc32); }
private:
	static inline void		Calc(const int8 byte, int32& crc32);
};
#endif
