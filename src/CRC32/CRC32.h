#ifndef __CRC32_H__
#define __CRC32_H__
#include "cross_os_defs.h"
//#include "PortType.h"

#ifdef __cplusplus
extern "C" {
#endif

	uint32_t CRC32(uint32_t crc, const uint8_t* buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif

