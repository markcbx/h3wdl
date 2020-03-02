/*
 * RealScan.cpp
 *
 *  Created on: Apr 9, 2019
 *      Author: mark
 */
#include "RealScan.h"
#include <pthread.h>
#include "ShmemCmd.h"
#include <unistd.h>
#include "stdio.h"
#include "LogOutput.h"
RealScan::RealScan(ScanFileDo *scanfile,int sNum):m_scanfiledo(scanfile),m_scanEngineNum(sNum)
{
	m_list_mutex=PTHREAD_MUTEX_INITIALIZER;
	m_thread_mutex=PTHREAD_MUTEX_INITIALIZER;
	m_threadFTime_mutex=PTHREAD_MUTEX_INITIALIZER;
	m_virnum = 0;
	m_notvirnum = 0;
	m_threadNum = 0;
	m_ScanTime = 0;
	m_checkthread=false;
}
RealScan::~RealScan()
{

}

void RealScan::AddTestLocalFile()
{
	std::string tmp="/sys/kernel";
	std::list<std::string> tmplist;
    std::list<std::string>  folderlist;
	GetFolderFile(tmp,tmplist,folderlist);
	std::list<std::string>::iterator it = tmplist.begin();
	for(;it!=tmplist.end();it++)
	{
		ScanItem tmpit;
		tmpit.filePath = *it;
		PushScanItem(tmpit);
		//printf("%s \n", it->c_str());
	}

}
void RealScan::PushScanItem(ScanItem sitem)
{
	pthread_mutex_lock(&m_list_mutex);
	m_scanlist.push_back(sitem);
	pthread_mutex_unlock(&m_list_mutex);
}

void * Check_ScanFileTime_thread(void *arg)
{
	RealScan *realScan = (RealScan *)arg;
	if (realScan==0)
	{
		return 0;
	}
	long int curTime=0;
	while(true)
	{
		pthread_mutex_lock(&realScan->m_threadFTime_mutex);
		std::map<int ,FileEnterTime>::iterator it =realScan->m_threadFileTime.begin();
		curTime=GetCurrSecondTime();
		for(;it !=realScan->m_threadFileTime.end();it++)
		{
			long int scanTime = curTime-it->second.entertime;
			if (scanTime > 300 && realScan->m_scanlist.empty())
			{
				printf("[%s] %s scanTime > 300s and the scanlist is empty\n",GetCurrTimeStr().c_str(),it->second.fileName.c_str());
			}
			else if (scanTime > 300)
			{
				char buf[1024]={0};
				sprintf(buf,"%s scanTime > 300s and the scanlist size is %d\n",
						it->second.fileName.c_str(),realScan->m_scanlist.size());
				ZyWritelogByVSecure("INFO",buf);
				//printf("['%s] %s scanTime > 300s and the scanlist size is %d\n",GetCurrTimeStr().c_str(),it->second.fileName.c_str(),realScan->m_scanlist.size());
			}
		}
		pthread_mutex_unlock(&realScan->m_threadFTime_mutex);
		sleep(300);
	}


	return 0;
}

void * Scan_thread(void *arg)
{
	WppRealScan *tmp = (WppRealScan*)arg;
	if (tmp == 0)
	{
		return 0;
	}
	RealScan *realScan=tmp->m_realScan;
	ZavEngine * zavEg = tmp->m_zavEg;
	if (realScan==0 || zavEg ==0)
	{
		return 0;
	}
	printf("the scan thread start %x...........\n",tmp);
	//pthread_mutex_lock(&realScan->m_virnum_mutex);
	//realScan->m_threadNum++;
	//pthread_mutex_unlock(&realScan->m_virnum_mutex);
	realScan->m_virnum=0;
	realScan->m_notvirnum=0;
	int currthread=0;
	pthread_mutex_lock(&realScan->m_threadFTime_mutex);
	realScan->m_threadNo++;
	currthread = realScan->m_threadNo;
	pthread_mutex_unlock(&realScan->m_threadFTime_mutex);
	int curfilenum=0;
	//std::string sufx;
	//char buffer[32]={0};
	//sprintf(buffer,"%d",realScan->m_ScanTime);
	//sufx.append(buffer);
	while(true)
	{
		pthread_mutex_lock(&realScan->m_list_mutex);
		if (!realScan->m_scanlist.empty())
		{
			ScanItem tscan ;
			std::list<ScanItem>::iterator it =realScan->m_scanlist.begin();
			tscan = *it;
			realScan->m_scanlist.pop_front();
			pthread_mutex_unlock(&realScan->m_list_mutex);
			curfilenum++;
#ifdef LOCAL_FILE_TEST
			if (zavEg->Scan(tscan.filePath.c_str())==RESULT_INFECTED)
			{
				realScan->m_virnum++;
				printf("[%d] find virs %s\n",realScan->m_virnum,tscan.filePath.c_str());
			}
			else
			{
				realScan->m_notvirnum++;
				printf("[%d] not virs %s\n",realScan->m_notvirnum,tscan.filePath.c_str());
			}
#else
			//void * tmpmen =(unsigned char*)tscan.mem_pos+tscan.mem_fileConStart-4;
			//GenTmpFileSave(tscan.filePath,tmpmen,sufx);
			long int curtime=GetCurrSecondTime();
			FileEnterTime fentertime;
			fentertime.fileName=tscan.filePath;
			fentertime.entertime = curtime;
			std::string virname;
			pthread_mutex_lock(&realScan->m_threadFTime_mutex);
			realScan->m_threadFileTime[currthread]=fentertime;
			pthread_mutex_unlock(&realScan->m_threadFTime_mutex);
			if (zavEg->Scan(tscan.filePath.c_str(),tscan.mem_pos,tscan.mem_fileConStart, tscan.mem_len,virname)==RESULT_INFECTED)
			{
				pthread_mutex_lock(&realScan->m_thread_mutex);
				realScan->m_virnum++;
				pthread_mutex_unlock(&realScan->m_thread_mutex);
				char buf[256]={0};
				sprintf(buf,"[%d] find virs %s flen:%d",realScan->m_virnum,tscan.filePath.c_str(),tscan.mem_len);
				ZyWritelogByVSecure("INFO",buf);
				//printf("[%d] find virs %s flen:%d\n",realScan->m_virnum,tscan.filePath.c_str(),tscan.mem_len);
				realScan->m_scanfiledo->AddCache(tscan.fhash,true);
				realScan->m_scanfiledo->AddVirs(tscan.filePath,tscan.mem_len,virname);
			}
			else
			{
#if 0
				pthread_mutex_lock(&realScan->m_thread_mutex);
				realScan->m_notvirnum++;
				pthread_mutex_unlock(&realScan->m_thread_mutex);
				//add to the cache
				printf("[%d] nots virs %s flen:%d\n",realScan->m_notvirnum,tscan.filePath.c_str(),tscan.mem_len);
#endif
				//realScan->m_scanfiledo->AddCache(tscan.fhash,false);
			}
			realScan->m_scanfiledo->WriteOneFileScanStatus(tscan.mem_pos);
#endif
		}
		else
		{

			if (realScan->m_scanfiledo->GetScanStatus() == 2)
			{
				pthread_mutex_unlock(&realScan->m_list_mutex);
				printf("scan thread exit................\n");
				break;
			}
			else
			{
				pthread_mutex_unlock(&realScan->m_list_mutex);
				sleep(1);
			}
		}
	}
#ifdef LOCAL_FILE_TEST
#else
	realScan->m_scanfiledo->AddFileNum(curfilenum);
	pthread_mutex_lock(&realScan->m_thread_mutex);
	realScan->m_threadNum--;
	if (realScan->m_threadNum==0)
	{

		pthread_mutex_unlock(&realScan->m_thread_mutex);
		realScan->m_scanfiledo->DataReportVirs();
		realScan->m_virnum = 0;
		realScan->m_notvirnum=0;
		realScan->m_scanfiledo->SetScanStatus(0);
		//pthread_mutex_unlock(&realScan->m_list_mutex);
		printf("Scan_thread main empty second\n");
		realScan->m_scanfiledo->ServerNodiyRealScanComplete();
		pthread_mutex_lock(&realScan->m_threadFTime_mutex);
		realScan->m_threadFileTime.clear();
		pthread_mutex_unlock(&realScan->m_threadFTime_mutex);

	}
	else
	{
		pthread_mutex_unlock(&realScan->m_thread_mutex);
	}
#endif
	return 0;
}

WppRealScan::WppRealScan(RealScan * realscan,ZavEngine *zavEg)
{
	m_realScan=realscan;
	m_zavEg=zavEg;
}

void WppRealScan::DeleteZavEngine()
{
	if (m_zavEg!=0)
	{
		delete m_zavEg;
		m_zavEg=0;
	}

}

void WppRealScan::ReIniZavEngine(ZavEngine *zavEg)
{
	DeleteZavEngine();
	SetZavEngine(zavEg);
}
WppRealScan::~WppRealScan()
{
}

void RealScan::CreateCheckScanFileTime()
{
	if (!m_checkthread)
	{
		m_checkthread = true;
	}
	else
	{
		pthread_mutex_lock(&m_threadFTime_mutex);
		m_threadFileTime.clear();
		pthread_mutex_unlock(&m_threadFTime_mutex);
		return;
	}
	pthread_t tmp;
	pthread_attr_t a;
	pthread_attr_init(&a);
	pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
	pthread_create(&tmp, &a, Check_ScanFileTime_thread,this);

}

void RealScan::CreateScanThread()
{
	if (m_scanEngineNum!=m_scanEngineVec.size())
	{
		printf("zav init error\n");
		return ;
	}
	for (int i = 0 ;i< m_scanEngineNum;i++)
	{

		pthread_t tmp;
		pthread_attr_t a;
		pthread_attr_init(&a);
		pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
		pthread_create(&tmp, &a, Scan_thread,m_scanEngineVec[i]);
		m_scanThreadVec.push_back(tmp);

	}
	m_threadNum = m_scanEngineNum;
	//m_ScanTime++;
	m_threadNo = 0;

}

void RealScan::ReInitScanEngine()
{
	if (m_scanEngineVec.size()!=m_scanEngineNum)
	{
		printf("ReInitScanEngine size != m_scanEngineNum\n");
		return ;
	}
	CEngineFrame * scanFrame = m_scanfiledo->GetScanFrame();
	for (int i = 0 ;i<m_scanEngineNum;i++)
	{
		WppRealScan *tmp = m_scanEngineVec[i];
		if (tmp!=0)
		{
			ZavEngine * zavEg = new ZavEngine();

			if (zavEg != 0 )
			{
				zavEg->SetEngineFrame(scanFrame);
				if (zavEg->InitBavEng())
				{
					tmp->ReIniZavEngine(zavEg);
				}
				else
				{
					printf("oo. error the InitBavEng error..........\n");
				}
			}
			else
			{
				printf("oo. error the zavEg can't create zavEg null..........\n");
			}
		}
		else
		{
			printf("oo. error the WppRealScan is null..........\n");
		}
	}

}

void RealScan::InitScanEngine()
{
	CEngineFrame * scanFrame = m_scanfiledo->GetScanFrame();
	if (scanFrame != 0)
	{
		for (int i = 0 ;i<m_scanEngineNum;i++)
		{
			ZavEngine * zavEg = new ZavEngine();
			if (zavEg!=0)
			{
				zavEg->SetEngineFrame(scanFrame);
				if (zavEg->InitBavEng())
				{
					WppRealScan *tmp= new WppRealScan(this,zavEg);
					m_scanEngineVec.push_back(tmp);
				}
				else
				{
					printf("InitBavEng error\n");
					break;
				}
			}
		}
	}
	else
	{
		printf("InitScanEngineAndScanThread scanFrame == 0\n");
	}
}


