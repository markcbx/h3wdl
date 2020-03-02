//============================================================================
// Name        : casVAnti.cpp
// Author      : mark
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <unistd.h>
#include "ShmemCmd.h"
#include "streamControl.h"
#include <pthread.h>
#include "HeartBeatService.h"
#include "RegisterHost.h"
#include "LogOutput.h"
using namespace std;
#define  MAX_MEM_LEN  (1024*1024*64)
#define __SERVER_CAS__
#ifdef __SERVER_CAS__
void *TestCmdServerSend(void* cmdSer);
void *TestCmdServerSend(void* cmdSer)
{

	TestCmdSend((SHMEMServer *)cmdSer, 1);

	return 0;
}
void CreateTestCmdSendThread(void *ser)
{
	pthread_t tmp;
	pthread_create(&tmp, NULL, TestCmdServerSend,ser);
}

void TestRegisterHostid()
{
	RegisterHost rhost;
	rhost.Init();
	rhost.RegisterCores();
	//rhost.InitClientMontior("/mnt/hgfs/CASVirtualAnti/testxml");
	//rhost.VirtualGroupSend();
	while(true)
	{
		sleep(30);
	}
}

void PrintfTheMEM()
{


}
void TestShData()
{
	std::string ivsh("ivshmem");
	int menLen = MAX_MEM_LEN;
	TestShmemData *serShmem = new TestShmemData(ivsh,menLen,true);
	serShmem->InitLock(TESTLOCKSTART,TESTITEM,TESTNUM);
	printf("Serve shmem lock inited .\n");
}
int main(int argc,char *args[]) {
#if 0
	InitLogSystem();
	TestRegisterHostid();
	//TestShData();
#else
	if (argc >1 && strlen(args[1])==2 && strcmp(args[1],"-u")==0)
	{
		printf("Uninstall the host casVanti .data report the uninstall host cpu cores\n");
		InitLogSystem();
		RegisterHost *pRegHost = new RegisterHost();
		if (pRegHost!=0)
		{
			pRegHost->Init();
			pRegHost->UninstallReport();
			sleep(5);
		}

		return 0;
		//exit(0);
	}
	SingleProcRun();
	InitLogSystem();
	RegisterHost *pRegHost = new RegisterHost();
	if (pRegHost!=0)
	{
		pRegHost->Init();
#ifdef LOCALNOTREGISTER
#else
		pRegHost->RegisterCores();
#endif
		pRegHost->InitClientMontior("/etc/libvirt/qemu");
		//pRegHost->InitClientMontior("/mnt/hgfs/CASVirtualAnti/testxml");
		pRegHost->VirtualGroupSend();
		ZyWritelogByVSecure("INFO","RegisterHost ok.");
	}
	else
	{
		ZyWritelogByVSecure("INFO","new RegisterHost fail .the memory too small exit");
		printf("new RegisterHost fail .the memory too small exit\n");
		exit(-1);
	}
	//printf("Reister cpucores ok .start the Server...........\n");
	int defauttime = 1;
	if (argc>1)
	{
		int i = atoi(args[1]);
		if (i>0&& i<9)
		{
			defauttime=i;
		}
	}
	printf("param scan time %d\n",defauttime);
	std::string ivsh("ivshmem");
	int menLen = MAX_MEM_LEN;
	SHMEMServer *sServer = new SHMEMServer(ivsh,menLen,pRegHost);
	if (sServer!=0)
	{
		//sServer->PrintfTheMem();
#if 1
		sServer->SetScanTime(defauttime);
		sServer->SetCS();
		//sServer->PrintfTheMem();
		//cout<<"sServer Client monitor Started"<<endl;
		ZyWritelogByVSecure("INFO","sServer StartServerThread Started",true);
		SHMEMServer::StartServerThread(sServer);
		sServer->StartCMDServerThread();
		sServer->StartHeartVirtualHostServerThread();
		//CreateTestCmdSendThread(sServer);
		//sServer->PrintfTheMem();
#else
		//testupdate vir
		CMDServer *tmpcmd=sServer->GetCMDServer();
		if (tmpcmd!=0)
		{
			std::string elib,vlib;
			if (GetZavPublic(elib,vlib))
			{
				printf("elib:%s,vlib:%s\n",elib.c_str(),vlib.c_str());
			}
			else
			{
				printf("GetZavPublic error\n");
			}
			tmpcmd->TestScanFrame();
			tmpcmd->TestVirUpate();
		}
#endif
	}
#endif
	while(true)
	{
		sleep(30);
	}
	//cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!
	return 0;
}
#else
void TestScanFrameAndEngineInit()
{
	CMDServer *tmp  =new CMDServer();
	tmp->TestScanFrame();
	while(true)
	{
		sleep(30);
	}

}

void TestHeartBeatService()
{
	HeartBeatService * ptmp = new HeartBeatService("","");
	ptmp->Init();
	sleep(-1);
}

void TestBackupFile()
{
	FileBackUp *p=new FileBackUp();
	if (p!=0)
	{
		p->Init();
		std::vector<std::string> tmpvec;
		tmpvec.push_back("/home/mark/vir/v14/1");
		tmpvec.push_back("/home/mark/vir/v14/2");
		tmpvec.push_back("/home/mark/vir/v14/3");
		tmpvec.push_back("/home/mark/vir/v14/4");
		tmpvec.push_back("/home/mark/vir/v14/5");
		tmpvec.push_back("/home/mark/vir/v14/6");
		p->BackUpList(tmpvec);
		sleep(-1);
	}
}
#include <sys/sysinfo.h>
void TestGetCpuNum()
{
	printf("cpu total:%d\n",get_nprocs_conf());
	printf("cup num:%d\n",get_nprocs());
	int i =GetCpuNum();
	printf("cpu cores :%d\n",i);
}
void TestGetIP()
{
	bool only127=false;
	std::string ip = GetIPAddress(only127);
	printf("ip:%s\n",ip.c_str());
}

void ComputeCacheFile()
{
	FileCache tfilecache;
	tfilecache.Init();
	int total = 0;
	int yxtotal = 0;
	std::list<std::string > tmpfolderlist;
	tmpfolderlist.push_back("/");
	while(!tmpfolderlist.empty())
	{
		std::string curpath = tmpfolderlist.front();
		std::list<std::string > tmpfilelist;

		GetFolderFile(curpath,tmpfilelist,tmpfolderlist);
		tmpfolderlist.pop_front();
		while(!tmpfilelist.empty())
		{
			std::string curfile = tmpfilelist.front();
			unsigned int fhash =0;
			if (tfilecache.GetFileAttrHash(curfile.c_str(),&fhash))
			{
				//printf("%s %d\n",curfile.c_str(),fhash);
				tfilecache.AddCache(fhash,false);
				yxtotal++;
			}
			total++;
			tmpfilelist.pop_front();
		}
	}
	tfilecache.WriteFileCache();
	printf("All file num %d,yxtoal %d\n",total,yxtotal);

}

int main(int argc,char *args[]) {
#if 1
	SingleProcRun();
	InitLogSystem();
	std::string ivsh("/dev/ivshmem");
	int menLen = MAX_MEM_LEN;
	SHMEMClient *sClient = new SHMEMClient(ivsh, MEM_LEN);
	std::list<std::string> tmplist;
	if (argc>1)
	{
		for(int i=1;i<argc;i++)
		{
			std::string tmp;
			tmp.append(args[i]);
			tmplist.push_back(tmp);
		}
	}
	else
	{
		tmplist.push_back("/");
	}
	sClient->SetScanInitFolder(tmplist);
	SHMEMClient::StartClientThread(sClient);
	ZyWritelogByVSecure("INFO","jycas client run  ok.");
#else
	//TestBackupFile();
	//TestScanFrameAndEngineInit();
	//TestHeartBeatService();
	//TestGetCpuNum();
	//TestGetIP();
	ComputeCacheFile();
#endif
	while(true)
	{
		sleep(30);
	}
	return 0;

}


#endif //__SERVER_CAS__
