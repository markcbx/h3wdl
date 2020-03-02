/*
 * calcmd5.cpp
 *
 *  Created on: Sep 5, 2017
 *      Author: mark
 */
//#include <stdio.h>
#include "md5_engine.h"
typedef unsigned char BYTE;
typedef char CHAR;

static const char HEX[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A',
		'B', 'C', 'D', 'E', 'F' };
	/* Convert byte array to hex string. */
void BytesToHexString(const BYTE *input, CHAR *outchar, size_t length)
{
		//std::string str;
		//str.reserve(length << 1);l
		for (size_t i = 0; i < length; i++) {
			int t = input[i];
			int a = t / 16;
			int b = t % 16;
			outchar[i*2]= HEX[a];
			outchar[i*2 + 1] = HEX[b];
		}
		//return str;
}



bool GetBufferMd5(unsigned char* pby, int nLength, BYTE* pbyMd5Value)
{
    if (!pby || nLength <= 0 || !pbyMd5Value)
        return false;
    md5_engine md5e;
	md5e.digest(pby,nLength,pbyMd5Value);
    return true;
}

bool GetBufferMd5A(unsigned char* pby, int nLength, char* szMd5Value)
{
    BYTE byMD5[16] = { 0 };
    if (!::GetBufferMd5(pby, nLength, byMD5))
        return false;

    BytesToHexString((const unsigned char *)byMD5,(CHAR*)szMd5Value,16);
    szMd5Value[32] = 0;

    return true;
}
