/*
 * ScanCache.cpp
 *
 *  Created on: 2017骞�10��22��
 *      Author: mark
 */

#include "stdio.h"
#include "ScanCache.h"
//#include "zycommon.h"
#include "CRC32/ZyCRC.h"
#include "unit.h"
//#include "FileAttrUtil.h"


FileCache::FileCache()
{
	m_cacheMap_cs = PTHREAD_MUTEX_INITIALIZER;
}

FileCache::~FileCache()
{

}
#define CACHE_FILE_ITEM_PATH         "fileCache.dat"
char * cachePath = "/opt/jycas/";
void FileCache::Init()
{
	m_cache_Path = GetCurrPath();
	//GetInstallDir(m_cache_Path);
	if (!m_cache_Path.empty())
	{
		m_cache_Path = m_cache_Path + "/" + CACHE_FILE_ITEM_PATH;
	}
}

void FileCache::UnInit()
{
	//::DeleteCriticalSection(&m_cacheMap_cs);
}

bool FileCache::AddCache(unsigned int hash, bool res)
{
	bool sres=false;
	if (QueryCache(hash,&sres) && sres)
	{
		return false;
	}
	pthread_mutex_lock(&m_cacheMap_cs);
	//pthread_mutex_lock(&G_fileEventList_mutex);
	m_NewCache_Map[hash] = res;
	if (m_NewCache_Map.size() > 1024)
	{
		WriteFileCache();
	}
	pthread_mutex_unlock(&m_cacheMap_cs);
	return true;
}

bool FileCache::AddCache(const char * file, bool res)
{
//	if (res)
//		return true;
	unsigned int fileHash;
	bool fres = GetFileAttrHash(file, &fileHash);
	if (fres)
	{
		pthread_mutex_lock(&m_cacheMap_cs);
		m_NewCache_Map[fileHash] = res;
		if (m_NewCache_Map.size() > 128)
		{
			WriteFileCache();
		}
		pthread_mutex_unlock(&m_cacheMap_cs);
	}

	return true;
}

bool FileCache::QueryCache(unsigned int hash, bool *res)
{
	if ((m_fileCacheMap.size() == 0) && (m_NewCache_Map.size() == 0))
	{
		return false;
	}

	bool qres = false;

	pthread_mutex_lock(&m_cacheMap_cs);//EnterCriticalSection(&m_cacheMap_cs);
	std::map<unsigned int, bool>::iterator it = m_fileCacheMap.find(hash);
	if (it != m_fileCacheMap.end())
	{
		*res = it->second;

		qres = true;
	}
	if (!qres)
	{
		std::map<unsigned int, bool>::iterator itN = m_NewCache_Map.find(hash);
		if (itN != m_NewCache_Map.end())
		{
			*res = itN->second;

			qres = true;
		}
	}
	pthread_mutex_unlock(&m_cacheMap_cs);

	return qres;

}
bool FileCache::QueryCache(const char * file, bool *res)
{
	if ((m_fileCacheMap.size() == 0) && (m_NewCache_Map.size() == 0))
	{
		return false;
	}
	unsigned int fileHash;
	bool fres = GetFileAttrHash(file, &fileHash);
	bool qres = false;
	if (fres)
	{
		qres = QueryCache(fileHash, res);
	}
	return qres;
}

bool FileCache::GetFileAttrHash(const char * file, unsigned int * ufileHash)
{
	return GetFileAttrCRC(file, ufileHash);
}

void WriteCache(FILE * p, FileCacheItem* buf, int count)
{
	fwrite(buf, sizeof(FileCacheItem), count, p);
}

#define FILE_CACHE_ARRAY_NUM    512

void FileCache::WriteFileCache()
{
	const char *filePath = m_cache_Path.c_str();
	FILE * pw = 0;
	pw = fopen (filePath, "ab+");
	if (pw != NULL)
	{
		if ((m_NewCache_Map.size() > 0))
		{
			FileCacheItem cacheArray[FILE_CACHE_ARRAY_NUM] = { 0 };

			int i = 0;

			std::map<unsigned int, bool>::iterator it = m_NewCache_Map.begin();
			for (; it != m_NewCache_Map.end(); it++)
			{
				m_fileCacheMap[it->first] = it->second;
				i++;
				if (i < FILE_CACHE_ARRAY_NUM)
				{
					cacheArray[i - 1].hash = it->first;
					cacheArray[i - 1].flag = it->second;
				}
				else
				{
					cacheArray[i - 1].hash = it->first;
					cacheArray[i - 1].flag = it->second;
					WriteCache(pw, cacheArray, FILE_CACHE_ARRAY_NUM);
					i = 0;
				}
			}

			if (i != 0)
			{
				WriteCache(pw, cacheArray, i);
			}

			m_NewCache_Map.clear();
		}
		fclose(pw);
	}

}

void FileCache::ReadFileCache()
{
	const char *filePath = m_cache_Path.c_str();
	FILE * pr = 0;
	pr = fopen( filePath, "rb");
	if (pr != NULL)
	{
		int mapSize = 0;
		fseek(pr, 0L, SEEK_END);
		int flen = ftell(pr);
		int itemSize = sizeof(FileCacheItem);

		mapSize = flen / itemSize;
		if (mapSize != 0)
		{
			fseek(pr, 0L, SEEK_SET);
			int iCount = mapSize / FILE_CACHE_ARRAY_NUM;
			int iYcout = mapSize % FILE_CACHE_ARRAY_NUM;
			FileCacheItem cacheArray[FILE_CACHE_ARRAY_NUM] = { 0 };
			for (int i = 0; i < iCount; i++)
			{
				int iSize = fread((void*)cacheArray, sizeof(FileCacheItem), FILE_CACHE_ARRAY_NUM, pr);
				for (int j = 0; j < iSize; j++)
				{
					m_fileCacheMap[cacheArray[j].hash] = cacheArray[j].flag;
				}
			}//LeaveCriticalSection(&m_cacheMap_cs);
			int iSize = fread((void*)cacheArray, sizeof(FileCacheItem), iYcout, pr);
			for (int j = 0; j < iSize; j++)
			{
				m_fileCacheMap[cacheArray[j].hash] = cacheArray[j].flag;
			}
		}

		if (mapSize != m_fileCacheMap.size())
		{
			printf("ReadFileCache mapSize = %d , m_fileCacheMap.size = %d\n",mapSize,m_fileCacheMap.size());
			//OutputDebugStringA("ooooooooo**********oooooooo*************Read fileCache error******************88\n");
		}

		fclose(pr);
	}
}




