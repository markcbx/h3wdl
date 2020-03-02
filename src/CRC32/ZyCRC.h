#ifndef __ZY_CRC32_HEADER__
#define __ZY_CRC32_HEADER__
//#include "windows.h"
//#include "PortType.h"
#include "cross_os_defs.h"
DWORD Zy_CRC32(const BYTE* buf, int len, DWORD crc);

bool GetFileAttrCRC(const char* filepath,unsigned int *fattr);

#endif //__ZY_CRC32_HEADER__
