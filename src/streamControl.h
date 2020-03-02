#ifndef __HEADER_STREAMCONTROL__
#define __HEADER_STREAMCONTROL__

// streamControl.cpp : 定义控制台应用程序的入口点。
//
#include "map"
#include "unit.h"
#include "ShmemCmd.h"
#include <time.h>
#include <pthread.h>
#include "clientMonitor.h"
#include "Shmem.h"
#include "RegisterHost.h"
int NextEmptyItem(void *addr, int itemLen, int itemNum);
int NextFullItem(void *addr, int itemLen, int itemNum);


int WriteItem(void *addr, int flag, char *mac);
int ReadItem(void *addr, char **mac);


void* ServerWaitMacThread(LPVOID lParam);
void* ClientWaitUuidThread(LPVOID lParam);

//
class SHMEMServer
{
public:
	SHMEMServer(void *memStart, int memlen);
	SHMEMServer(std::string shname, int memlen,RegisterHost *registhost);

	void SetClientMonitor(ClientMonitor *clientMonitor);
	void InitFullNum();
	void PushCmdItem(CMDITEM cmditem);
	void SetCS(/*CRITICAL_SECTION *cs, CRITICAL_SECTION *cs1, CRITICAL_SECTION *cs2, CRITICAL_SECTION *cs3, CRITICAL_SECTION *cs4*/);
	void InitUuidMacMap(char *filename);
	std::string GetUuid(char *mac);
	void WhileWaitClientMacData();
	static void StartServerThread(SHMEMServer *pserver);
	void StartHeartVirtualHostServerThread();
	void AddUuidMac(std::string uuid,std::string mac,std::string name,std::string osType,bool aorm=false);
	void AddUuidMacs(std::map<std::string, std::string> vgroup);
	void DelUuidMac(std::string uuid);
	void DataReportUuid(std::string uuid,int type);
	void DataReportAddUuid(std::string uuid,std::string name,std::string osType);
	void DataReportOpenUuid(std::string uuid);
	~SHMEMServer();
	void StartCMDServerThread();
	void GetUUIDVec(std::vector<std::string> &outVec)
	{
		outVec.assign(m_tmpTestUUIdvec.begin(),m_tmpTestUUIdvec.end());
	}
	CMDServer *GetCMDServer()
	{
		return m_cmdServer;
	}

	void PrintfTheMem();

	void SetScanTime(int t)
	{
		m_testscantime = t;
	}

	void PushUUID(std::string uuid)
	{
		if (m_cmdServer)
		{
			m_cmdServer->PushUUId(uuid);
		}
	}

private:
		void SetItemLockPos();

		void InitCMDServer();

		void StartClientMonitor();

private:
	void *m_ptr;
	int m_len;
	std::map<std::string, std::map<std::string,std::string> > m_vmname_uuidmacMap;
	//tmp test uuid mac
	pthread_mutex_t m_uuid_mac_mutex;
	std::map<std::string, std::string > m_tmp_umacmap;

	std::vector<std::string> m_tmpTestUUIdvec;

	CMDServer *m_cmdServer;

	ClientMonitor *m_clientMonitor;

	SHmem   *m_shmen;

	RegisterHost *m_registerHost;

	int m_testscantime;
};





class SHMEMClient
{
public:
	SHMEMClient(void *memStart, int memlen);
	SHMEMClient(std::string shname, int memlen);
	int InitClientUUID(char *filename);
	int WriteMacDatAddWaitUUId();
	~SHMEMClient();

	void SetCS();
	void SetMac(std::string mac);
	std::string GetMac();

	static void StartClientThread(SHMEMClient *pClient);
	void SetScanInitFolder(std::list<std::string> folders)
	{
		m_folders.assign(folders.begin(),folders.end());
	}
private:
		void InitCmdClient(std::string uuid);
		void InitClientMac();
private:
	void *m_ptr;
	int m_len;
	std::string m_uuid;
	//CRITICAL_SECTION *m_pcs;
	std::string m_mac;

	CMDClient  *m_cmdClient;

	SHmem   *m_shmen;

	std::list<std::string> m_folders;
};

void TestCmdSend(SHMEMServer *pSercmd,int testnum);
#if 0
CRITICAL_SECTION fullNum_cs;
CRITICAL_SECTION item_cs1;
CRITICAL_SECTION item_cs2;
CRITICAL_SECTION item_cs3;
CRITICAL_SECTION item_cs4;
#endif
int  TestMacUUIDAndCMD();

int  TestMacUUID();

//CRITICAL_SECTION g_tmpcs;

void TestCmdServerClient();

void TestFileSize(void *mem);

class TestShmemData
{
public:
	TestShmemData(std::string shname, int memlen,bool ser);
	~TestShmemData();

	void InitLock(int lockstart,int itemlen,int itemnum);

	int GetNextEmpty();
	int GetNextFull();
private:
	void * m_ptr;
	int m_len;

	bool m_ser;

	int m_lockstart;
	int m_itemlen;
	int m_itemnum;

	SHmem *m_shmem;

};




#endif //__HEADER_STREAMCONTROL__
