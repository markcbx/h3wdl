/*
 * ScanCache.h
 *
 *  Created on: 2017年10月22日
 *      Author: mark
 */


#ifndef __SCAN_CACHE_HEADER__
#define __SCAN_CACHE_HEADER__
#include <map>
//#include <Windows.h>
#include <vector>
#include <string>
#include <pthread.h>
typedef struct fileHashItem_
{
	unsigned int hash;
	bool         flag;
} FileCacheItem;


class FileCache
{
public:
	FileCache();
	~FileCache();

	void Init();
	void UnInit();

	bool AddCache(const char * file, bool res);
	bool QueryCache(const char * file, bool *res);
	bool QueryCache(unsigned int hash, bool *res);
	bool AddCache(unsigned int hash, bool res);

	void WriteFileCache();
	void ReadFileCache();

	bool GetFileAttrHash(const char * file, unsigned int * ufileHash);

private:



	//lock var
	//////////
	//CRITICAL_SECTION         m_cacheMap_cs;
	pthread_mutex_t m_cacheMap_cs ;
	////从文件读出的cache
	std::map<unsigned int, bool> m_fileCacheMap;
	/////新增加的cache
	std::map<unsigned int, bool> m_NewCache_Map;


	std::string                   m_cache_Path;

};







#endif //__SCAN_CACHE_HEADER__





