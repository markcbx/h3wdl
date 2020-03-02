#ifndef ___SHMEM_CMD__
#define ___SHMEM_CMD__
#include <string>
#include <vector>
#include <list>
#include "unit.h"
#include "ScanCache.h"
#include "RealScan.h"
#include "zavEngine.h"
#include "ZyConfigfile.h"
#include "FileBackUp.h"
#include "HeartBeatService.h"
#include "DataReport.h"

class CMDServer;
class CMDClient;
class CacheHash;
class IPMonitor;
class HeartService;
void* CreateServerCMDThread(LPVOID lParam);
void* CreateClientRecvThread(LPVOID lParam);
void* CreateCleanSendThread(LPVOID lParam);
void* CreateCleanRecvThread(LPVOID lParam);
void* CreateScanRecvThread(LPVOID lParam);
void* CreateScanSendThread(LPVOID lParam);
void* CreateScanEnumThread(LPVOID lParam);
void* CreateCacheRequestThread(LPVOID lparam);
void* CreateCacheReplyThread(LPVOID lparam);
void* CreateCacheBackThread(LPVOID lparam);
void* CreateServerUpdateVirlibThread(LPVOID lParam);
void* CreateClientGetIPThread(LPVOID lParam);
void* CreateServerRecvIPThread(LPVOID lParam);
void* CreateClientMonitorHeartThread(LPVOID lparam);
void* CreateSerMonitorHeartThread(LPVOID lParam);
typedef struct _hash_file{
	unsigned int fhash;
	std::string filename;
} Hash_FileItem;
class CleanFileDo
{
public:
	CleanFileDo(CMDServer*ser, CMDClient *client)
	{
		m_cleanNum = 0;
		m_ptr = 0;
		m_len = 0;
		m_cmdServer = ser;
		m_cmdClient = client;
	}
	CleanFileDo(void *men, int len, CMDServer*ser, CMDClient *client);
	void InitCleanDoCS();
	~CleanFileDo();
	void SetShmem(void *men, int len)
	{
		//if (m_cs_flag)
		m_ptr = men;
		m_len = len;
	}
	void InitFileBackUp();
	int SendCleanFile(std::vector<std::string> &incleanvec);
	int RecvCleanFile(std::vector<std::string> &outcleanvec);
	int GetNextEmptyItem();
	int GetNextFullItem();
	void SetCleanNum(int cleannum)
	{
		m_cleanNum = cleannum;
	}
	void SendFile();
	void RecvFile();

	static void CleanRecvThread(CleanFileDo*pRcleanclient);
	static void CleanSendThread(CleanFileDo*pRcleanclient);
	static int m_cs_flag;
	void SetSendVec(std::vector<std::string> &inputvec)
	{
		m_testsendcleanvec.assign(inputvec.begin(), inputvec.end());
	}

	void SetCurUuid(std::string uuid)
	{
		m_cur_uuid = uuid;
	}
	void DataReportCleanFile();
    void TiggerThreadExit();
    void DataReportCleanAction();
private:
	void *m_ptr;
	int m_len;
	int m_cleanNum;

	std::vector<std::string> m_testsendcleanvec;

	std::vector<std::string> m_testrecvcleanvec;


	CMDServer *m_cmdServer;
	CMDClient *m_cmdClient;

	std::string m_cur_uuid;

	FileBackUp * m_fileBackup;
};


class ScanFileDo
{
public:
	ScanFileDo(CMDServer*ser, CMDClient *client)
	{
		m_EnumTotalNum = 0;
		m_ptr = 0;
		m_len = 0;
		m_cmdServer = ser;
		m_cmdClient = client;
	}
	ScanFileDo(void *men, int len, CMDServer*ser, CMDClient *client)
	{
		m_EnumTotalNum = 0;
		m_ptr = men;
		m_len = len;
		m_cmdServer = ser;
		m_cmdClient = client;
		m_fileCache = 0;
		m_realScan =0;
		m_scanFrame = 0;
		m_scanstatus = 0;

		m_activeThreadNum=0;
		m_worksendthreadNum= 0;
		m_workrecvthreadNum= 0;
		//m_enum_flag = false;
		m_file_list_mutex= PTHREAD_MUTEX_INITIALIZER;
		m_folder_list_mutex=PTHREAD_MUTEX_INITIALIZER;

		m_cacheHash = 0;

		m_hashFilelist_mutex = PTHREAD_MUTEX_INITIALIZER;
		m_cache_endFlag = false;

		m_cur_fileNum_mutex = PTHREAD_MUTEX_INITIALIZER;
	}
	void InitScanDoCS();
	void InitFileCache();
	void InitRealScan();
	void InitCacheHash(bool ser);
	~ScanFileDo()
	{

	}
	void SetShmem(void *men, int len)
	{
		m_ptr = men;
		m_len = len;
	}
	void EnumFile();
	int SendScanFile();
	int RecvScanFile();
	int GetNextEmptyItem();
	int GetNextFullItem();
	void SetTotalEnumNum(int cleannum)
	{
		m_EnumTotalNum = cleannum;
	}

	static void ScanRecvThread(ScanFileDo*pRScanclient,int srNum);
	static void ScanSendThread(ScanFileDo*pRScanclient,int ssendNum,std::list<std::string> folderlist);
	static int m_cs_flag;

	void SetCurUuid(std::string uuid)
	{
		m_cur_uuid = uuid;
	}
	void SetScanFrame(CEngineFrame *scanFrame)
	{
		m_scanFrame=scanFrame;
	}
	CEngineFrame * GetScanFrame()
	{
		return m_scanFrame;
	}
	int GetScanStatus()
	{
		return m_scanstatus;
	}
	void SetScanStatus(int status)
	{
		m_scanstatus = status;
	}
	void ServerNodiyRealScanComplete();
	void AddCache(unsigned int fhash, bool res);

	void WriteOneFileScanStatus(void *mem);

	void CreateRealScanThread();
	void CreateCheckScanTimeThread();
	pthread_mutex_t m_scanstatus_mutex;

	void StartLocalTestScanThread();

	void SetScanSendThreadNum(int sthreadNum)
	{
		m_worksendthreadNum=sthreadNum;
	}
	void SetScanRecvThreadNum(int rthreadNum)
	{
		m_workrecvthreadNum=rthreadNum;
	}
	void SetScanSendInitFolder(std::list<std::string> folders);
	void PushHashFileItem(Hash_FileItem item)
	{
		pthread_mutex_lock(&m_hashFilelist_mutex);
		m_hashFileList.push_back(item);
		pthread_mutex_unlock(&m_hashFilelist_mutex);

	}
	void TestPrintfItemSize();
	void SetEndFlag(bool flag)
	{
		m_cache_endFlag = flag;
	}
	bool GetEndFlag()
	{
		return m_cache_endFlag;
	}
	int QueryServerCache(unsigned int fhash);
	void SetMaxCacheThreadNum(int num);
	void CreateCacheReuestThread(int tnum);
	void CreateCacheReplyThread(int tnum);
	void CreateCacheBackThread(int tnum);
	void SetSendFileNum(int num)
	{
		m_sendfile_num =num;
	}
	void SetSerScanStartTime();
	void SetSerScanEndTime();

	void SetCltScanStartTime();
	void SetCltScanEndTime();

	void TotalSerTime();
	void TotalCltTime();

	bool ReInitScanFrame();

	void AddVirs(std::string filepath,int flen,std::string virname);
	void DataReportVirs();
	void AddFileNum(int fileNum);

	void TiggerThreadExit();

	void DateReportScanAction(int action);

private:
	void *m_ptr;
	int m_len;
	int m_EnumTotalNum;

	CMDServer *m_cmdServer;
	CMDClient *m_cmdClient;

	std::string m_cur_uuid;

	FileCache *m_fileCache;

	RealScan *m_realScan;

	CEngineFrame *m_scanFrame;

	int m_sendfile_num;


	int m_scanstatus ;

	int m_worksendthreadNum;
	int m_workrecvthreadNum;
	int m_activeThreadNum;
	std::list<std::string> m_folder_list;

	std::list<std::string> m_file_list;
	pthread_mutex_t m_file_list_mutex;
	bool m_cache_endFlag;
	std::list<Hash_FileItem> m_hashFileList;
	pthread_mutex_t m_hashFilelist_mutex;
	pthread_mutex_t m_folder_list_mutex;

	CacheHash * m_cacheHash;

	long int m_scanSerStarttime;
	long int m_scanSerEndTime;

	long int m_scanCltStarttime;
	long int m_scanCltEndTime;

	std::vector<SCAN_ONE_ITEM> m_cur_scanVec;

	int m_cur_scanFileNum;
	pthread_mutex_t m_cur_fileNum_mutex;

};


class CMDServer
{
public:
	CMDServer(void *mem, int len,ConfigFileService *configfile,DataReportService* dataReport,std::string hostId);

	CMDServer()
	{
		m_ipMonitor=0;
	}

	int SendCMD(char *uuid, int cmd, std::vector<std::string> &incleanvec);
	void InitCMDServerLock();
	void StartServerCleanThead(std::vector<std::string> &inCleanvec,std::string uuid);
	void StartServerScanThead(std::string uuid);
	static void StartServerSendThread(CMDServer*pRServer);
	void StartServerUpdateVirThread();
	void DataReportUpdateVirlib();
	void DataReportUpdateAction(std::string uuid);
	void PushCmdItem(CMDITEM cmditem)
	{
		if (m_heartBeatSer!=0)
		{
			m_heartBeatSer->PushItemCmd(cmditem);
		}

	}

	int NodifyServerCMDCompelted(int cmdstatuc, std::string log="");
	int NodifyServerEnumFileNum(int enumFileNum);

	int GetTheEnumFileNum();
	void CmdDoThread();
	~CMDServer();
	void TestScanFrame();
	void ReInitScanFrame();
	void UpdateVirLib();
	void TestVirUpate();
	void DataReportCloseUuid(std::string uuid);
	void DataReport(std::string uuid,std::string ip);
	std::string GetHostId()
	{
		return m_hostId;
	}
	DataReportService* GetDataReport()
	{
		return m_dataReport;
	}
	bool VirualMachineState(std::string uuid);
	void TiggerThreadExit(int cmd);
	std::string GetCurUUID()
	{
		return m_cur_uuid;
	}
	void SetNullCuruuid()
	{
		m_cur_uuid="";
		m_cur_cmd=0;
	}
	int GetCurCMD()
	{
		return m_cur_cmd;
	}

	void PushUUId(std::string uuid);

	void StartSerHeartSer();

private:
	void InitScanFrame();
	void InitConfigFile();
	void InitHeartBeatSer();
	void InitIpMonitor();
private:
	void *m_ptr;
	int m_len;

	std::string m_cur_uuid;
	int m_cur_cmd;
	int m_cur_cmdstatus;
	int m_cur_cleanfilenum;
	std::vector<std::string> m_cur_clean_pathvec;

	void *m_ptr_cmd;
	int   m_ptr_cmdLen;

	std::list<CMDITEM> m_tmp_testList;

	pthread_mutex_t m_cmd_mutex;

	CleanFileDo *m_testCleanSendFile;
	ScanFileDo  *m_testScanRecvFile;
	
	CEngineFrame *m_scanFrame;

	ConfigFileService *m_configfile;

	HeartBeatService *m_heartBeatSer;

	IPMonitor *m_ipMonitor;

	HeartService *m_heartvhostser;

	DataReportService*  m_dataReport;

	std::string m_hostId;

};


class CMDClient
{
public:
	CMDClient(void *mem, int len);
	~CMDClient();
	void SetUUID(std::string uuid);
	int RecvServerCmd();
	void StartClientCleanThead(int cleannum);
	void StartClientScanThead();
	static void StartClientRecvThread(CMDClient*pRclient);
	int NodifyServerCMDCompelted(int cmdstatuc);
	int NodifyServerEnumFileNum(int enumFileNum);
	int GetTheEnumFileNum();
	void SetTestScanFolder(std::list<std::string> tmplist)
	{
		m_testscanfolerlist.assign(tmplist.begin(),tmplist.end());
	}

	void InitIpMonitor();


private:
	void *m_ptr;
	int m_len;
	std::string m_uuid;

	CleanFileDo *m_testCleanRecvFile;
	ScanFileDo  *m_testScanSendFile;
	std::list<std::string> m_testscanfolerlist;

	IPMonitor *m_ipMonitor;

	HeartService *m_heartClient;
};

class WappLock
{
public:
	WappLock(pthread_spinlock_t * splock);
	~WappLock();
	pthread_spinlock_t * m_splock;
};

class CacheHash
{
public:
	CacheHash(void * mem,int len,ScanFileDo* scando);
	~CacheHash();

	void EnumFile();
	void SetEnumFlag(bool flag)
	{
		m_enum_flag = false;
	}

	void PushEnumFolder(std::list<std::string> folders)
	{
		m_enumfolderlist.assign(folders.begin(),folders.end());
	}

	int GetNextBack();

	int GetNextEmpty();
	int GetNextFull();
	void CacheRequestThread();
	bool CreateRequestFlag();
	void NodifySerCacheRec();
	void CacheRequestBackThread();
	void InitLock();
	void CacheReplyThread();

	bool GetCacheRecvEnd()
	{
		return m_cache_recvEnd;
	}
	void WriteCacheEndFlag(int flag);
	int GetCacheEndFlag();

	void CreateCacheHashReuestThread(int threadNum);
	void CreateCacheHashReplyThread(int threadNum);
	void CreateCacheHashBackThread(int threadNum);
	void AddCacheAvtiveNum()
	{
		pthread_mutex_lock(&m_cacheThread_mutex);
		m_cacheActiveNum++;
		pthread_mutex_unlock(&m_cacheThread_mutex);
	}
	int GetCacheActiveNum()
	{
		pthread_mutex_lock(&m_cacheThread_mutex);
		int i = m_cacheActiveNum;
		pthread_mutex_unlock(&m_cacheThread_mutex);
		return i;
	}
	bool ActiveThreaddStats();

	void SetMaxCacheThreadNum(int maxnum)
	{
		m_max_cacheThreadNum=maxnum;
		m_cacheActiveNum = 0;
		m_createdCacheThNum = 0;
		m_fileTotal = 0;
	}

	void TiggerThreadExit();

private:
	void *m_ptr;
	int m_len;

	std::list<std::string> m_enumfolderlist;
	pthread_mutex_t m_folder_mutex;
	pthread_mutex_t m_prelist_mutex;
	std::list<std::string> m_prelist;
	bool m_enum_flag;
	std::map<unsigned long ,bool> m_threadActiveMap;
	pthread_mutex_t m_threadActive_mutex;

	pthread_mutex_t m_cacheThread_mutex;
	int m_max_cacheThreadNum;
	int m_createdCacheThNum;
	int m_cacheThreadNum;
	int m_cacheActiveNum;
	int m_cacheReplyNum;
	int m_cacheBackNum;
	ScanFileDo* m_scanFiledo;

	std::map<unsigned int ,std::string> m_hashfilemap;
	pthread_mutex_t m_hashfilemap_mutex;

	int m_cachebackfilenum;
	int m_fileTotal;
	bool m_cache_recvEnd;

};

class IPMonitor
{
public:
	IPMonitor(void *mem,int len,bool ser);
	~IPMonitor();

	void SetUUID(std::string uuid)
	{
		m_uuid = uuid;
	}
	int GetNextFullItem();
	int GetNextEmptyItem();

	void StartClientGetIpThread();
	void GetIPAndSendToServer();

	void ServerMonitTheIPData();
	void RecvTheIpData();

	void SetCmdSer(CMDServer * ser)
	{
		m_cmdSer = ser;
	}

	void DataReport(std::string uuid, std::string ip);

private:
	void InitLock();

private:
	void *m_ptr;
	int m_len;
	bool m_flag;
//client uuid
	std::string m_uuid;

	CMDServer* m_cmdSer;


};

class HeartService
{
public:
	HeartService(void *mem,int len,bool ser);
	~HeartService();

	void StartClientMonitorHeart();

	void StartSerMonitorHeart();

	bool GetVirtualState(std::string uuid);

	void ClientMonitorHeart();

	void SetUUId(std::string uuid)
	{
		m_uuid = uuid;
	}

	void SetCmdSer(CMDServer * ser)
	{
		m_cmdSer = ser;
	}

	void PushUUId(std::string uuid);

	void SerHeartDone();

private:
	void InitLock();

private:
	void *m_ptr;
	int m_len;
	std::string m_uuid;

	//ser
	CMDServer* m_cmdSer;
	pthread_mutex_t m_online_mutex;
	std::list<std::string> m_onlinelist;


};









#endif ///___SHMEM_CMD__
