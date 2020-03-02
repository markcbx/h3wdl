/*
 * RealScan.h
 *
 *  Created on: Apr 9, 2019
 *      Author: mark
 */

#ifndef REALSCAN_H_
#define REALSCAN_H_
#include "string"
#include <list>
#include <pthread.h>
#include <vector>
#include "zavEngine.h"
#include <map>
class ScanFileDo;

typedef struct __scanitem{
	std::string filePath;
	void *mem_pos;
    int mem_len;
	unsigned int fhash;
	int mem_fileConStart;
} ScanItem;

typedef struct _fileTime {
	std::string fileName;
	long   int  entertime;
}FileEnterTime;
class WppRealScan;
void * Scan_thread(void *arg);

void * Check_ScanFileTime_thread(void *arg);
class RealScan
{
public:
	RealScan(ScanFileDo *scanfile,int sNum);
	~RealScan();
	void PushScanItem(ScanItem sitem);
	void InitScanEngine();
	void ReInitScanEngine();
	void CreateScanThread();
	void CreateCheckScanFileTime();
	void AddTestLocalFile();
	pthread_mutex_t m_list_mutex;
	std::list<ScanItem> m_scanlist;
	ScanFileDo * m_scanfiledo;
	pthread_mutex_t m_thread_mutex;
	pthread_mutex_t m_threadFTime_mutex;
	int m_virnum;
	int m_notvirnum;
	int m_threadNum;
	int m_ScanTime;
	int m_threadNo;
	std::map<int, FileEnterTime> m_threadFileTime;
private:
	int m_scanEngineNum ;

	std::vector<WppRealScan*> m_scanEngineVec;
	//pthread_t G_fileScan_thread[SCAN_ENGINE_NUM];
	std::vector<pthread_t> m_scanThreadVec;

	bool m_checkthread;

};

class WppRealScan
{
public:
	WppRealScan(RealScan * realscan,ZavEngine *zavEg);
	~WppRealScan();

	void ReIniZavEngine(ZavEngine *zavEg);

	RealScan *m_realScan;
	ZavEngine *m_zavEg;

private:
	void SetZavEngine(ZavEngine *zavEg)
	{
		m_zavEg=zavEg;
	}
	void DeleteZavEngine();



};




#endif /* REALSCAN_H_ */
