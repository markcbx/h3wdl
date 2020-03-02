// streamControl.cpp : 定义控制台应用程序的入口点。
//
#include <unistd.h>
#include "stdlib.h"
#include <string.h>
#include <stdio.h>
#include "streamControl.h"
#include "proto/clientActionV2.pb.h"
#include "LogOutput.h"
extern void * G_LOCK_START;
int ReadData(void *addr, void *buffer, int readlen)
{
	memcpy(buffer, addr, readlen);
	return readlen;
}

int NextEmptyItem(void *addr, int itemLen, int itemNum)
{
	pthread_spinlock_t * splock;
	int flagindex = sizeof(pthread_spinlock_t);
	int flag = 0;
	void *ptr = addr;
	for (int i = 0; i < itemNum; i++)
	{
		splock =(pthread_spinlock_t *)((byte*)ptr + i*itemLen);
		if (splock != 0)
		{
			WappLock tcs(splock);
			ReadData((byte*)ptr + flagindex + i*itemLen, &flag, sizeof(flag));
			if (flag == 0 || flag == 3)
			{
				int tflag = 4;
				WriteData((byte*)ptr + flagindex + i*itemLen, &tflag, sizeof(tflag));
				return (i*itemLen);
			}
		}
	}
	return -1;

}
int NextFullItem(void *addr, int itemLen, int itemNum)
{
	pthread_spinlock_t * splock;
	int flagindex = sizeof(pthread_spinlock_t);
	int flag = 0;
	void *ptr = addr;
	for (int i = 0; i < itemNum; i++)
	{
		splock =(pthread_spinlock_t *)((byte*)ptr + i*itemLen);
		if (splock != 0)
		{
			WappLock tcs(splock);
			ReadData((byte*)ptr + flagindex + i*itemLen, &flag, sizeof(flag));
			if (flag == 1)
			{
				flag = 4;
				WriteData((byte*)ptr + flagindex + i*itemLen, &flag, sizeof(flag));
				return (i*itemLen);
			}
		}
	}
	return -1;
}


int MEMSetZero(void *addr, int zerolen)
{
	memset(addr, 0, zerolen);
	return zerolen;
}
int WriteData(void *addr, void *buffer, int writelen)
{
	int ires = 0;
	memcpy(addr, buffer, writelen);
	return writelen;
}
int WriteItem(void *addr, int flag, char *mac)
{

	//ITEM_LEN  (4+4+4+18+4+20)
	//int flag = 1;
	int wlen = WriteData((byte*)addr, &flag, sizeof(flag));
	int maclen = strlen(mac);
	int wmlen = WriteData((byte*)addr + wlen, &maclen, sizeof(maclen));
	int tlen = WriteData((byte*)addr + wlen + wmlen, mac, maclen);
	return (wlen + wmlen + tlen);
}
int ReadItem(void *addr, char **mac)
{
	//ITEM_LEN  (4+4+4+18+4+20)
	//the item as lock flag maclen mac uuidlen uuid
	//int flag = 1;
	int maclen=0;
	pthread_spinlock_t * splock;
	int flagindex = sizeof(pthread_spinlock_t);
	splock=(pthread_spinlock_t*)addr;
	int rlen=0;
	if (splock !=0)
	{
		WappLock tcs(splock);
		rlen = ReadData((byte*)addr+4+4,&maclen,sizeof(maclen));
		if (maclen !=0)
		{
			*mac = (char*)malloc(maclen+1);
			if (*mac != 0)
			{
				memset(*mac, 0, maclen + 1);
				int clen = ReadData((byte*)addr + 4 + 4 + rlen, *mac, maclen);
				return (rlen + clen);
			}
		}
	}
	return 0;
}

TestShmemData::TestShmemData(std::string shname, int memlen,bool ser)
{
	m_ser = ser;
	m_len = memlen;
	m_shmem=new SHmem(shname,memlen,ser);
	if (m_shmem!=0)
	{
		m_ptr=m_shmem->GetShMen();
		if (m_ptr!=0)
		{
			MEMSetZero(m_ptr,m_len);
			G_LOCK_START = m_ptr;
		}
		else
		{
			printf("server Shmem init error \n");
			exit(-1);
		}
	}
}
TestShmemData::~TestShmemData()
{
	if (m_shmem!=0)
	{
		delete m_shmem;
		m_shmem=0;
	}
}

void TestShmemData::InitLock(int lockstart,int itemlen,int itemnum)
{
	m_lockstart= lockstart;
	m_itemlen = itemlen;
	m_itemnum = itemnum;
	pthread_spinlock_t *splocktStart = 0;
	for(int i = 0; i< itemnum;i++)
	{
		splocktStart =(pthread_spinlock_t *)((byte*)m_ptr+lockstart+i*itemlen);
		pthread_spin_init(splocktStart, PTHREAD_PROCESS_SHARED);
	}
}


int TestShmemData::GetNextEmpty()
{
	return -1;
}
int TestShmemData::GetNextFull()
{
	return -1;
}

//
SHMEMServer::SHMEMServer(std::string shname, int memlen,RegisterHost *registhost):m_len(memlen),m_registerHost(registhost)
{
	m_shmen = 0;
	m_cmdServer = 0;
	m_clientMonitor = 0;
	m_uuid_mac_mutex=PTHREAD_MUTEX_INITIALIZER;
	m_shmen=new SHmem(shname,memlen,true);
	if (m_shmen!=0)
	{
		m_ptr=m_shmen->GetShMen();
		if (m_ptr!=0)
		{
			MEMSetZero(m_ptr,m_len);
			G_LOCK_START = m_ptr;
		}
	}
	m_testscantime = 10;
	StartClientMonitor();
	InitCMDServer();
}

SHMEMServer::SHMEMServer(void *memStart, int memlen) :m_ptr(memStart), m_len(memlen)
{
	m_cmdServer = 0;
	m_clientMonitor = 0;
	m_uuid_mac_mutex=PTHREAD_MUTEX_INITIALIZER;
	//InitCMDServer();
}
void SHMEMServer::InitFullNum()
{
	int i = 0;
	WriteData(m_ptr, &i, sizeof(i));
	MEMSetZero(m_ptr, HEADER_TOTAL_LEN);
	printf("zero len :%d\n", HEADER_TOTAL_LEN);
}
void SHMEMServer::SetClientMonitor(ClientMonitor *clientMonitor)
{
	m_clientMonitor=clientMonitor;
	if (m_clientMonitor!=0)
	{
		m_clientMonitor->SetSMServer(this);
		m_clientMonitor->StartThreadMonitorMacChange(m_clientMonitor);
	}

}

void SHMEMServer::AddUuidMacs(std::map<std::string, std::string> vgroup)
{
	pthread_mutex_lock(&m_uuid_mac_mutex);
	std::map<std::string, std::string>::iterator it = vgroup.begin();
	for(;it!=vgroup.end();it++)
	{
		m_tmp_umacmap[it->first]=it->second;
		m_tmpTestUUIdvec.push_back(it->first);
	}
	pthread_mutex_unlock(&m_uuid_mac_mutex);

}

void SHMEMServer::AddUuidMac(std::string uuid,std::string mac,std::string name,std::string osType,bool aorm)
{
	pthread_mutex_lock(&m_uuid_mac_mutex);
	m_tmp_umacmap[uuid]=mac;
	pthread_mutex_unlock(&m_uuid_mac_mutex);
	if (aorm)
	{
		printf("modiy mac\n");
	}
	else
	{
		printf("add mac\n");
		DataReportAddUuid(uuid,name,osType);
	}
	m_tmpTestUUIdvec.push_back(uuid);
	printf("UM: %s %s \n",uuid.c_str(),mac.c_str());
}

void SHMEMServer::DelUuidMac(std::string uuid)
{
	pthread_mutex_lock(&m_uuid_mac_mutex);
	std::map<std::string, std::string>::iterator it = m_tmp_umacmap.find(uuid);
	if (it !=m_tmp_umacmap.end())
	{
		m_tmp_umacmap.erase(it);
	}
	pthread_mutex_unlock(&m_uuid_mac_mutex);
	printf("D: %s\n",uuid.c_str());
	DataReportUuid(uuid,1);
}

void SHMEMServer::DataReportAddUuid(std::string uuid,std::string name,std::string osType)
{
	do
	{
		if (m_registerHost==0)
		{
			printf("adduuid must set m_registerHost!!!!\n");
			break;
		}
		DataReportService *dataReport=m_registerHost->GetDataReport();
		if (dataReport==0)
		{
			printf("adduuid must init dataReport object !!!\n");
			break;
		}
		VirtualItem vsitem;
		vsitem.set_uuid(uuid);
		vsitem.set_name(name);
		vsitem.set_ostype(osType);
		std::string vsitstr;
		vsitem.SerializePartialToString(&vsitstr);
		VirtualState vstate;
		VirtualState::VItem *pvitem = vstate.add_vitems();
		pvitem->set_state(VirtualState::GuestState::VirtualState_GuestState_ADD_GUEST);
		pvitem->set_statedata(vsitstr);
		pvitem->set_time(time(NULL));
		std::string pvstr;
		vstate.SerializePartialToString(&pvstr);
		std::string allstr;
		ClientActionRequest crequest;
		crequest.set_client_id(m_registerHost->GetHostId());
		crequest.set_action_type(ClientActionRequest::ActionType::ClientActionRequest_ActionType_VIRTUAL_STATE);
		crequest.set_action_data(pvstr);
		crequest.SerializePartialToString(&allstr);
		dataReport->SendDataReport(allstr);
	}while (0);
}

void SHMEMServer::DataReportOpenUuid(std::string uuid)
{
	DataReportUuid(uuid,2);
	char buffer[64]={0};
	sprintf(buffer,"%s uuid open.",uuid.c_str());
	ZyWritelogByVSecure("INFO",buffer);
}
//type 1 :del,2:open

void SHMEMServer::DataReportUuid(std::string uuid,int type)
{
	do
	{
		if (m_registerHost==0)
		{
			printf("DelUuid must set m_registerHost!!!!\n");
			break;
		}
		DataReportService *dataReport=m_registerHost->GetDataReport();
		if (dataReport==0)
		{
			printf("DelUuid must init dataReport object !!!\n");
			break;
		}
		VirtualItem vsitem;
		vsitem.set_uuid(uuid);
		std::string vsitstr;
		vsitem.SerializePartialToString(&vsitstr);
		VirtualState vstate;
		VirtualState::VItem *pvitem = vstate.add_vitems();
		if (type ==1)
		{
			pvitem->set_state(VirtualState::GuestState::VirtualState_GuestState_DEL_GUEST);
			printf("[%s] DataReportUuid %s,%d deled\n",GetCurrTimeStr().c_str(),uuid.c_str(),type);
		}
		else if (type==2)
		{
			pvitem->set_state(VirtualState::GuestState::VirtualState_GuestState_GUEST_OPEN);
			printf("[%s] DataReportUuid %s,%d opened\n",GetCurrTimeStr().c_str(),uuid.c_str(),type);
		}
		pvitem->set_statedata(vsitstr);
		pvitem->set_time(time(NULL));
		std::string pvstr;
		vstate.SerializePartialToString(&pvstr);
		std::string allstr;
		ClientActionRequest crequest;
		crequest.set_client_id(m_registerHost->GetHostId());
		crequest.set_action_type(ClientActionRequest::ActionType::ClientActionRequest_ActionType_VIRTUAL_STATE);
		crequest.set_action_data(pvstr);
		crequest.SerializePartialToString(&allstr);
		dataReport->SendDataReport(allstr);
	}while (0);


}
void SHMEMServer::PushCmdItem(CMDITEM cmditem)
{
	if (m_cmdServer != 0)
	{
		m_cmdServer->PushCmdItem(cmditem);
	}
}
void SHMEMServer::SetCS()
{
	SetItemLockPos();
}
void SHMEMServer::InitUuidMacMap(char *filename)
{
	//cbx todo init m_vmname_uuidmacMap;
}
int GetMacFromStr(char*mac,std::vector<std::string> &outVec)
{
	std::string tmpStr(mac);
	int pos=tmpStr.find(",");
	while(pos!=-1)
	{
		std::string tmac=tmpStr.substr(0,pos);
		outVec.push_back(tmac);
		tmpStr=tmpStr.substr(pos+1,-1);
		pos =tmpStr.find(",");
	}
	outVec.push_back(tmpStr);
	return outVec.size();
}

bool ContainMac(std::string str,std::vector<std::string> &inVec)
{
	bool res = false;
	std::vector<std::string>::iterator it = inVec.begin();
	for(;it!=inVec.end();it++)
	{
		if (str.find(*it)!=-1)
		{
			res = true;
			break;
		}
	}
	return res;
}
std::string SHMEMServer::GetUuid(char *mac)
{
	//find from the map
	std::string uuid = "";
	//std::string tmac(mac);
	std::vector<std::string> outVec;
	GetMacFromStr(mac,outVec);
	pthread_mutex_lock(&m_uuid_mac_mutex);
	std::map<std::string, std::string >::iterator it = m_tmp_umacmap.begin();
	for (; it != m_tmp_umacmap.end(); it++)
	{
		if (ContainMac(it->second,outVec))
		{
			uuid = it->first;
			break;
		}
	}
	pthread_mutex_unlock(&m_uuid_mac_mutex);
	return uuid;
}
void SHMEMServer::WhileWaitClientMacData()
{
	while (true)
	{
		int findex = -1;
		findex = NextFullItem((byte*)m_ptr + ITEM_START, ITEM_LEN, ITEM_NUM);
		if (findex != -1)
		{
			printf("current fullitem %d\n", findex / ITEM_LEN);
		}
		else
		{
			sleep(10);
			continue;
		}

		char *p = NULL;
		int rlen = ReadItem((byte*)m_ptr + ITEM_START + findex, &p);
		pthread_spinlock_t * splock;
		int flagindex = sizeof(pthread_spinlock_t);
		splock=(pthread_spinlock_t*)((byte*)m_ptr + ITEM_START + findex);
		int rflag = 2;
		if (p != NULL)
		{
			std::string uuid = GetUuid(p);
			if (!uuid.empty())
			{
				//should get item lock
				if(splock!=0)
				{
					WappLock tcs(splock);
					int wlen =WriteData((byte*)m_ptr + ITEM_START + findex+4, &rflag, sizeof(rflag));
					int uuidlen = uuid.length();
					int wulen = WriteData((byte*)m_ptr + ITEM_START + findex + 4 + 4 + rlen, &uuidlen, sizeof(uuidlen));
					int ulen = WriteData((byte*)m_ptr + ITEM_START + findex + 4 + 4 + rlen + wulen, (void*)uuid.c_str(), uuidlen);
				}
				//test add scan to cmdlist
#ifdef LOCALNOTREGISTER
				CMDITEM citem;
				citem.uuid = uuid;
				citem.cmd = 1;
				for (int j= 0;j<m_testscantime;j++)
				{
					PushCmdItem(citem);
				}
#endif
				DataReportOpenUuid(uuid);
				PushUUID(uuid);
			}
			else
			{
				//should get item lock
				//uuid not get set the defalut uuid
				//uuid = "9087690f-0aad-409a-ba70-9083ijfiuiud";
				uuid = "12345678-9101-1213-1415-abcdefghijkh";
				//printf("mac %s,uuid not get. set defalut uuid %s\n", p, uuid.c_str());
				rflag = 0;
				WappLock tcs(splock);
				int wlen = WriteData((byte*)m_ptr + ITEM_START + findex + 4, &rflag, sizeof(rflag));
				int uuidlen = uuid.length();
				//printf("uuid len %d: uuid%s\n",uuidlen,uuid.c_str());
				int wulen = WriteData((byte*)m_ptr + ITEM_START + findex + 4 + 4 + rlen, &uuidlen, sizeof(uuidlen));
				int ulen = WriteData((byte*)m_ptr + ITEM_START + findex + 4 + 4 + rlen + wulen, (void*)uuid.c_str(), uuidlen);
			}
			free(p);
		}
	}
}

void SHMEMServer::StartHeartVirtualHostServerThread()
{
	if( m_cmdServer)
	{
		m_cmdServer->StartSerHeartSer();
	}
}
void SHMEMServer::StartServerThread(SHMEMServer *pserver)
{
	pthread_t tmp;
	pthread_attr_t a;
	pthread_attr_init(&a);
	pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
	pthread_create(&tmp, &a, ServerWaitMacThread,pserver);
}

SHMEMServer::~SHMEMServer()
{
	if (m_cmdServer != 0)
	{
		delete m_cmdServer;
		m_cmdServer = 0;
	}
	if (m_shmen!=0)
	{
		delete m_shmen;
		m_shmen = 0;
	}

}
void SHMEMServer::StartCMDServerThread()
{
	if (m_cmdServer != 0)
	{
		CMDServer::StartServerSendThread(m_cmdServer);
	}
}

void SHMEMServer::SetItemLockPos()
{
	pthread_spinlock_t *splocktStart0 = (pthread_spinlock_t *)((byte*)m_ptr+ITEM_START);
	pthread_spin_init(splocktStart0, PTHREAD_PROCESS_SHARED);
	pthread_spinlock_t *splocktStart1= (pthread_spinlock_t *)((byte*)splocktStart0+ITEM_LEN);
	pthread_spin_init(splocktStart1, PTHREAD_PROCESS_SHARED);
	pthread_spinlock_t *splocktStart2= (pthread_spinlock_t *)((byte*)splocktStart1+ITEM_LEN);
	pthread_spin_init(splocktStart2, PTHREAD_PROCESS_SHARED);
	pthread_spinlock_t *splocktStart3= (pthread_spinlock_t *)((byte*)splocktStart2+ITEM_LEN);
	pthread_spin_init(splocktStart3, PTHREAD_PROCESS_SHARED);
}

void SHMEMServer::PrintfTheMem()
{
	int tmpint = 0;
	int index = 0;
	int maxPrint=ITEM_START+ITEM_LEN-4;
    void *tptr = m_ptr;
    for(int i=0;i<maxPrint;)
    {
    	tptr=(byte*)m_ptr+i;
    	index =ReadData((void*)tptr,&tmpint,sizeof(tmpint));
    	i=i+index;
    	printf("%#x ",tmpint);
    	if (i%64==0)
    	{
    		printf("\n");
    	}
    }
    printf("\nPrintfTheMem completed.\n");
}

SHMEMClient::SHMEMClient(void *memStart, int memlen) :m_ptr(memStart), m_len(memlen)
{
	m_cmdClient = 0;
}
SHMEMClient::SHMEMClient(std::string shname, int memlen)
{
	m_cmdClient = 0;
	m_ptr =0;
	m_len = memlen;
	m_shmen = 0;
	InitClientMac();
	m_shmen = new SHmem(shname,memlen,false);
	if (m_shmen !=0)
	{
		m_ptr = m_shmen->GetShMen();
	}

}
int SHMEMClient::InitClientUUID(char *filename)
{
	//cbx todo init m_uuid
	int res = 0;
	//
	//succ
	res = 1;
	return res;
}
int SHMEMClient::WriteMacDatAddWaitUUId()
{
	char *mac =(char*)m_mac.c_str();
	printf("mac:%s\n",mac);
	while (true)
	{

		int pindex = -1;
		pindex = NextEmptyItem((byte*)m_ptr + ITEM_START, ITEM_LEN, ITEM_NUM);
		if (pindex!=-1)
		{
			printf("current empty %d,%d\n", pindex / ITEM_LEN,pindex);
		}
		else
		{
			//printf("no empty item  sleep 1 ..\n");
			sleep(3);
			continue;
		}

		int flag = 1;
		//should get item lock
		pthread_spinlock_t * splock;
		int flagindex = sizeof(pthread_spinlock_t);
		splock=(pthread_spinlock_t*)((byte*)m_ptr + ITEM_START+pindex);
		if (splock == NULL)
		{
			printf("oh.my god ,lock get error \n");
			sleep(3);
			continue;
		}
		int wlen = 0;
		{
			WappLock tcs(splock);
			wlen = WriteItem((byte*)m_ptr + ITEM_START + pindex + 4, flag, mac);
		}

		int rflag = 0;
		int ttime = 0;
		while (true)
		{
			sleep(1);
			//todo should get item lock
			{
				WappLock tcs(splock);
				int rlen = ReadData((byte*)m_ptr + ITEM_START + pindex + 4, &rflag, sizeof(rflag));
			}
			if (rflag == 2)
			{
				break;
			}
			ttime = ttime + 1;
			if (ttime > 2 * 30)
			{
				break;
			}
		}
		if (rflag == 2)
		{
			rflag = 3;
			int uuidlen = 0;
			int rlen=0;
			{
				WappLock tcs(splock);
				rlen = ReadData((byte*)m_ptr + ITEM_START + pindex + 4 + wlen,&uuidlen,sizeof(uuidlen));
			}
			if (uuidlen != 0 && uuidlen <40)
			{
				char *pbuffer = (char*)malloc(uuidlen + 1);
				if (pbuffer != 0)
				{
					memset(pbuffer, 0, uuidlen + 1);
					{
						WappLock tcs(splock);
						ReadData((byte*)m_ptr + ITEM_START + pindex + 4 + wlen + rlen, pbuffer, uuidlen);
					}
					m_uuid.append(pbuffer);
					printf("uuid len %d %s mac %s\n",  uuidlen,m_uuid.c_str(), mac);
					free(pbuffer);
					pbuffer = 0;
					//start the client cmd recv thread
					InitCmdClient(m_uuid);
					//printf("Will start client cmd object\n");
				}
			}
			//todo should get the item lock
			{
				WappLock tcs(splock);
				int wlen = WriteData((byte*)m_ptr + ITEM_START + pindex + 4, &rflag, sizeof(rflag));
			}
			break;
		}
		else
		{
			printf("ooo,not 2\n");
		}

	}
	return 0;
}
SHMEMClient::~SHMEMClient()
{
	if (m_cmdClient != 0)
	{
		delete m_cmdClient;
		m_cmdClient = 0;
	}
	if (m_shmen !=0)
	{
		delete m_shmen;
		m_shmen=0;
	}
}

void SHMEMClient::SetCS()
{
	//m_pcs = cs;
}
void SHMEMClient::SetMac(std::string mac)
{
	m_mac = mac;
}
std::string SHMEMClient::GetMac()
{
	return m_mac;
}

void SHMEMClient::StartClientThread(SHMEMClient *pClient)
{
	pthread_t tmp;
	pthread_attr_t a;
	pthread_attr_init(&a);
	pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
	pthread_create(&tmp, &a, ClientWaitUuidThread,
			pClient);
}


void* ServerWaitMacThread(LPVOID lParam)
{
	SHMEMServer* pser = (SHMEMServer*)lParam;
	if (pser != NULL)
	{
		pser->WhileWaitClientMacData();
	}
	else
	{
		printf("server thread lparam null\n");
	}
	return 0;
}
void* ClientWaitUuidThread(LPVOID lParam)
{
	SHMEMClient* pclient = (SHMEMClient*)lParam;
	if (pclient != NULL)
	{
		pclient->WriteMacDatAddWaitUUId();
	}
	else
	{
		printf("client thread lparam null\n");
	}
	return 0;
}

int RandomINT(int min, int max)
{
	int ires = min + rand() % (max - min);
	return ires;
}

void TestCmdSend(SHMEMServer *pSercmd,int testnum)
{
	srand(time(NULL));

	if (pSercmd != NULL)
	{
		std::vector<std::string> uuidVec;
		pSercmd->GetUUIDVec(uuidVec);
		int uuidNum = uuidVec.size();
		if(uuidNum<=0)
		{
			printf("uuid not initd\n");
			return ;
		}
		printf("UUID 's NUM is %d\n",uuidNum);
		for(int ij =0;ij<uuidNum;ij++)
		{
			printf("uuid:%s\n",uuidVec[ij].c_str());
		}
		CMDITEM item1;
		std::vector<std::string> itmcleanvec;
		itmcleanvec.push_back("/home/mark/testvir/1");
		itmcleanvec.push_back("/home/mark/testvir/2");
		itmcleanvec.push_back("/home/mark/testvir/3");
		itmcleanvec.push_back("/home/mark/testvir/4");
		itmcleanvec.push_back("/home/mark/testvir/5");
		itmcleanvec.push_back("/home/mark/testvir/7");
		itmcleanvec.push_back("/home/mark/testvir/8");
		itmcleanvec.push_back("/home/mark/testvir/9");
		item1.uuid = "";// TMP_UUID4;
		//itenarray[0] = item1;
		
		item1.cleanVec = itmcleanvec;

		CMDITEM item2;

		item2.cleanVec.push_back("c:\\windows\\system32\\test21.txt");
		item2.cleanVec.push_back("c:\\windows\\system32\\test22.txt");
		item2.cleanVec.push_back("c:\\windows\\test24.txt");
		item2.cleanVec.push_back("c:\\windows\\system32\\driver\\test25.txt");
		item2.cleanVec.push_back("c:\\windows\\system32\\test210.txt");
		item2.cleanVec.push_back("c:\\windows\\system32\\test221.txt");
		item2.cleanVec.push_back("c:\\windows\\test242.txt");
		item2.cleanVec.push_back("c:\\windows\\system32\\driver\\test253.txt");
		item2.cleanVec.push_back("c:\\windows\\system32\\test214.txt");
		item2.cleanVec.push_back("c:\\windows\\system32\\test225.txt");
		item2.cleanVec.push_back("c:\\windows\\test246.txt");
		item2.cleanVec.push_back("c:\\windows\\system32\\driver\\test257.txt");
		item2.cleanVec.push_back("c:\\windows\\system32\\test218.txt");
		item2.cleanVec.push_back("c:\\windows\\system32\\test229.txt");
		item2.cleanVec.push_back("c:\\windows\\test2410.txt");
		item2.cleanVec.push_back("c:\\windows\\system32\\driver\\test2511.txt");
		item2.uuid = "";
		//itenarray[1] = item2;

		CMDITEM item3;
		item3.cleanVec.push_back("c:\\windows\\system32\\test31.txt");
		item3.cleanVec.push_back("c:\\windows\\system32\\test32.txt");
		item3.cleanVec.push_back("c:\\windows\\test34.txt");
		item3.cleanVec.push_back("c:\\windows\\system32\\driver\\test35.txt");
		item3.cleanVec.push_back("c:\\windows\\system32\\driver\\test\\tmpe\\test3587.txt");
		item3.uuid = "";
		CMDITEM itenarray[ClIENT_NUM] = { item1, item2, item3, item2 };

		for (int i = 0; i < testnum; i++)
		{
			int cmdrad = i%3+1;//RandomINT(1, 4);
			int uuidrad = 3;//RandomINT(1, uuidNum);
			int arrrad = 0;//RandomINT(0, ClIENT_NUM);
			itenarray[arrrad].cmd = cmdrad;
			itenarray[arrrad].uuid = uuidVec[uuidrad];
			printf("{%d}***uuid{%s}***cmd{%d}***\n", arrrad,uuidVec[uuidrad].c_str(), cmdrad);
			pSercmd->PushCmdItem(itenarray[arrrad]);
		}
	
	}
}

#if 0
CRITICAL_SECTION fullNum_cs;
CRITICAL_SECTION item_cs1;
CRITICAL_SECTION item_cs2;
CRITICAL_SECTION item_cs3;
CRITICAL_SECTION item_cs4;
#endif
int  TestMacUUIDAndCMD()
{
	void * pmem = (void*)malloc(MEM_LEN);
	if (pmem == 0)
	{
		return 0;
	}
	//InitGTestHashMap();
	char *pmac = TMP_MAC1;
	SHMEMServer *sServer = new SHMEMServer(pmem, MEM_LEN);
#if 0
	InitializeCriticalSection(&fullNum_cs);
	InitializeCriticalSection(&item_cs1);
	InitializeCriticalSection(&item_cs2);
	InitializeCriticalSection(&item_cs3);
	InitializeCriticalSection(&item_cs4);
#endif
	sServer->InitFullNum();
	sServer->InitUuidMacMap("");
	sServer->SetCS();
	SHMEMServer::StartServerThread(sServer);
	sServer->StartCMDServerThread();
#if 1
	char *clientMac[ClIENT_NUM] = { TMP_MAC1, TMP_MAC2, TMP_MAC3, TMP_MAC4 };
	for (int i = 0; i < ClIENT_NUM; i++)
	{
		SHMEMClient *sClient = new SHMEMClient(pmem, MEM_LEN);
		//sClient->SetCS(&fullNum_cs);
		sClient->SetMac(clientMac[i%ClIENT_NUM]);
		SHMEMClient::StartClientThread(sClient);
	}
#endif
	TestCmdSend(sServer,12);
	while (1)
	{
		sleep(5);
	}
#if 0
	DeleteCriticalSection(&fullNum_cs);
	DeleteCriticalSection(&item_cs1);
	DeleteCriticalSection(&item_cs2);
	DeleteCriticalSection(&item_cs3);
	DeleteCriticalSection(&item_cs4);
#endif
	free(pmem);

	return 0;

}

void TestClientMacUUID()
{


}

int  TestMacUUID()
{
	void * pmem = (void*)malloc(MEM_LEN);
	if (pmem == 0)
	{
		return 0;
	}
	char *pmac = TMP_MAC1;
	SHMEMServer *sServer = new SHMEMServer(pmem, MEM_LEN);
#if 0
	InitializeCriticalSection(&fullNum_cs);
	InitializeCriticalSection(&item_cs1);
	InitializeCriticalSection(&item_cs2);
	InitializeCriticalSection(&item_cs3);
	InitializeCriticalSection(&item_cs4);
#endif
	sServer->InitFullNum();
	sServer->InitUuidMacMap("");
	//sServer->SetCS(&fullNum_cs, &item_cs1, &item_cs2, &item_cs3, &item_cs4);
	SHMEMServer::StartServerThread(sServer);
	char *clientMac[ClIENT_NUM] = { TMP_MAC1, TMP_MAC2, TMP_MAC3, TMP_MAC4 };
	for (int i = 0; i < ClIENT_NUM; i++)
	{
		SHMEMClient *sClient = new SHMEMClient(pmem, MEM_LEN);
		//sClient->SetCS(&fullNum_cs);
		sClient->SetMac(clientMac[i%ClIENT_NUM]);
		SHMEMClient::StartClientThread(sClient);
	}
	while (1)
	{
		sleep(5);
	}
#if 0
	DeleteCriticalSection(&fullNum_cs);
	DeleteCriticalSection(&item_cs1);
	DeleteCriticalSection(&item_cs2);
	DeleteCriticalSection(&item_cs3);
	DeleteCriticalSection(&item_cs4);
#endif
	free(pmem);

	return 0;

}

void SHMEMServer::StartClientMonitor()
{
	if (m_registerHost!=0)
	{
		m_registerHost->StartClientMontior(this);
	}
}

//CRITICAL_SECTION g_tmpcs;
void SHMEMServer::InitCMDServer()
{
	//InitGTCS();
	//InitializeCriticalSection(&g_tmpcs);
	m_cmdServer = new CMDServer((byte*)m_ptr + HEADER_TOTAL_LEN, m_len - HEADER_TOTAL_LEN,
			m_registerHost->GetConfigFile(),m_registerHost->GetDataReport(),m_registerHost->GetHostId());
	if (m_cmdServer != NULL)
	{
		m_cmdServer->InitCMDServerLock();
	}
}

void SHMEMClient::InitClientMac()
{
	std::vector<std::string> macvec;
	int inu  = GetMacList(macvec);
	if (inu >0)
	{
		//printf("client mac %s \n",m_mac.c_str());
		if (inu == 1)
		{
			m_mac = macvec[0];
		}
		else
		{
			std::string tmpm=macvec[0];
			int maxi=inu;
			if (maxi>4)
			{
				maxi = 4;
			}
			for(int i=1;i<maxi;i++)
			{
				tmpm=tmpm+","+macvec[i];
			}
			m_mac=tmpm;
		}
	}
	else
	{
		printf("Get client mac error\n");
		m_mac = "00:xx:00:xx:00:ff";
	}

}

void SHMEMClient::InitCmdClient(std::string uuid)
{
	m_cmdClient = new CMDClient((byte*)m_ptr + HEADER_TOTAL_LEN, m_len - HEADER_TOTAL_LEN);
	if (m_cmdClient!=0)
	{
		m_cmdClient->SetUUID(uuid);
		m_cmdClient->SetTestScanFolder(m_folders);
		CMDClient::StartClientRecvThread(m_cmdClient);
	}
}

void TestCmdServerClient()
{

	InitGTCS();
	void * pmem = (void*)malloc(MEM_LEN);
	if (pmem == 0)
	{
		return ;
	}
	//CRITICAL_SECTION tmpcs;
	//InitializeCriticalSection(&tmpcs);
	CMDServer *pSercmd = new CMDServer(pmem, MEM_LEN,0,0,"");
	if (pSercmd != NULL)
	{
		//pSercmd->InitCMDServerLock(&tmpcs);
	}
	CMDServer::StartServerSendThread(pSercmd);
	char *clientUuid[ClIENT_NUM-1] = { TMP_UUID1, TMP_UUID2, TMP_UUID3 };
	for (int i = 0; i < ClIENT_NUM - 1; i++)
	{
		CMDClient *pcmdclient = new CMDClient(pmem, MEM_LEN);
		if (pcmdclient != NULL)
		{
			pcmdclient->SetUUID(clientUuid[i]);
			CMDClient::StartClientRecvThread(pcmdclient);
		}
	}
	//TestFileSize(pmem);

	while (true)
	{
		sleep(3);
	}
}

void TestFileSize(void *mem)
{
	std::string ptha = "D:\\baidusd.exe";
	//WriteFileToShmem(ptha, mem, 1024 * 1024 * 4);
}
