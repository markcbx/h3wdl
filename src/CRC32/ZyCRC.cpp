#include "ZyCRC.h"
#include "CRC32.h"
#include <string>
#include <sys/stat.h>
#pragma pack(push)
#pragma pack(1)
typedef struct _FILE_BASE_INFO
{
	DWORD       dwFileSizeHigh;
	DWORD       dwFileSizeLow;
	time_t     stCreationTime;
	time_t     stLastWriteTime;
} FILE_BASE_INFO, *PFILE_BASE_INFO;
#pragma pack(pop)
#define  CFT_KEY_CRC  876312 
#define  FMT_KEY_CRC  987655

static std::string code_string = "http://msdn.microsoft.com/en-US/windows/apps/br229512"
+ std::string("20938405")
+ std::string("2304598239")
+ std::string("43256776")
+ std::string("167243568")
+ std::string("98432124")
+ std::string("8708655683")
+ std::string("6456433763")
+ std::string("1345668788")
+ std::string("107447362323")
+ std::string("3587454332")
+ std::string("56754322");
DWORD Zy_CRC32(const BYTE* buf, int len, DWORD crc)
{
	return CRC32(crc, buf, len);
}

DWORD g_dwGenSeed = Zy_CRC32((BYTE *)code_string.c_str(), code_string.length(), CFT_KEY_CRC);



//pathlen > 128
//filelen > 1024*1024*4
//file not exist
//return false

bool GetFileAttrCRC(const char* filepath,unsigned int *fattr)
{
	bool gres = false;
	std::string tmpstr(filepath);
	do
	{
		if (tmpstr.length()>128)
		{
			break;
		}
		FILE_BASE_INFO fbase ={0};
		struct stat stmp;
		if (stat(filepath, &stmp) !=0)
		{
			break;
		}
		if (stmp.st_size > 1024*1024*4)
		{
			break;
		}
	    fbase.dwFileSizeHigh = stmp.st_size&0xffff0000;
	    fbase.dwFileSizeLow = stmp.st_size&0x0000ffff;
	    fbase.stCreationTime = stmp.st_mtime;
	    fbase.stLastWriteTime = stmp.st_mtime;
	    *fattr = Zy_CRC32((BYTE*)&fbase, sizeof(fbase), g_dwGenSeed);
	    gres = true;
	}while(0);

    return gres;
}
