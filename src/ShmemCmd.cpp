#include <unistd.h>
#include "string.h"
#include "stdlib.h"
#include <stdio.h>
#include "ShmemCmd.h"
#include "time.h"
#include <map>
#include <sys/stat.h>
#include "CRC32/ZyCRC.h"
#include "proto/clientActionV2.pb.h"
#include "hash/calcmd5.h"
#include "ZyThreats.h"
#include "LogOutput.h"
void * G_LOCK_START = 0;
std::string GetCurrTimeStr()
{
	std::string stime;
	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);  //获取相对于1970到现在的秒数
	tm nowTime;
	localtime_r(&time.tv_sec, &nowTime);
	char current[128]={0};
	sprintf(current, "%04d%02d%02d %02d:%02d:%02d", nowTime.tm_year + 1900, nowTime.tm_mon+1, nowTime.tm_mday,
			  nowTime.tm_hour, nowTime.tm_min, nowTime.tm_sec);
	stime.append(current);
	return stime;
}

long int GetCurrSecondTime()
{
	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);  //获取相对于1970到现在的秒数
	long int cur = time.tv_sec;
	return cur;
}

int WriteCmd(void* ptr, char *uuid, int ulen,int cmd, int cmdstatus,int cleannum)
{	
	int cleanfnum = cleannum;
	int wcmdlen = WriteData((byte*)ptr + CMDLOCKLEN, &cmd, sizeof(cmd));
	int wcst = WriteData((byte*)ptr + CMDLOCKLEN + wcmdlen, &cmdstatus, sizeof(cmdstatus));
	int cllen = WriteData((byte*)ptr + CMDLOCKLEN + wcmdlen + wcst, &cleanfnum, sizeof(cleanfnum));
	int uulen = WriteData((byte*)ptr + CMDLOCKLEN + wcmdlen + wcst + cllen, &ulen, sizeof(ulen));
	int uclen = WriteData((byte*)ptr + CMDLOCKLEN + wcmdlen + wcst + cllen + uulen, uuid, ulen);
	return (wcmdlen+wcst+cllen+uulen+uclen);
}
void InitGTCS()
{
}
void GetGTSCANCS(/*std::vector<CRITICAL_SECTION*> & outCSvec*/)
{
}

void GetGTCS(/*std::vector<CRITICAL_SECTION*> & outCSvec*/)
{
}


CMDServer::CMDServer(void *mem, int len,ConfigFileService *configfile,DataReportService* dataReport,std::string hostId) :m_ptr(mem),
		m_len(len),m_configfile(configfile),m_dataReport(dataReport),m_hostId(hostId)
{
	m_cur_uuid = "";
	m_cur_cmd = 0;
	m_cur_cmdstatus = 0;
	m_cur_cleanfilenum = 0;
	m_cur_clean_pathvec.clear();
	m_ptr_cmd = 0;
	m_ptr_cmdLen =0;
	m_cmd_mutex = PTHREAD_MUTEX_INITIALIZER;
	m_heartBeatSer=0;
	m_ipMonitor = 0;
	m_heartvhostser=0;
	InitScanFrame();
	//InitConfigFile();
	InitHeartBeatSer();
	m_testCleanSendFile = new CleanFileDo((byte*)m_ptr + CMDTOTALLEN, m_len - CMDTOTALLEN, this, 0);
	if (m_testCleanSendFile != 0)
	{
		m_testCleanSendFile->InitCleanDoCS();
	}

	m_testScanRecvFile = new ScanFileDo((byte*)m_ptr + INITSCANITEMDATA, m_len - INITSCANITEMDATA, this, 0);
	if (m_testScanRecvFile != 0)
	{
		m_testScanRecvFile->InitScanDoCS();
		m_testScanRecvFile->InitFileCache();
		m_testScanRecvFile->SetScanFrame(m_scanFrame);
		m_testScanRecvFile->InitRealScan();
		m_testScanRecvFile->InitCacheHash(true);
	}
//todo the m_ptr start addres need set right.
	InitIpMonitor();
	m_heartvhostser = new HeartService((byte*)m_ptr + HEARTDATAINIT, m_len - HEARTDATAINIT,true);
	m_heartvhostser->SetCmdSer(this);
}
void CMDServer::TestScanFrame()
{
	InitScanFrame();
	m_testScanRecvFile = new ScanFileDo((byte*)m_ptr + INITSCANITEMDATA, m_len - INITSCANITEMDATA, this, 0);
	m_testScanRecvFile->SetScanFrame(m_scanFrame);
	//m_testScanRecvFile->InitRealScan();
	//m_testScanRecvFile->StartLocalTestScanThread();
}

void CMDServer::StartSerHeartSer()
{
	if(m_heartvhostser)
	{
		m_heartvhostser->StartSerMonitorHeart();
	}
}

void CMDServer::PushUUId(std::string uuid)
{
	if (m_heartvhostser)
	{
		m_heartvhostser->PushUUId(uuid);
	}
}
void CMDServer::InitScanFrame()
{
	m_scanFrame = new CEngineFrame();
	if (m_scanFrame!=0)
	{
		if (m_scanFrame->Init()!=true)
		{
			printf("ScanFrame init error \n");
			exit(-1);
		}
	}
}

void CMDServer::TestVirUpate()
{
	StartServerUpdateVirThread();
}

void CMDServer::InitHeartBeatSer()
{
	//todo need first init the congfile
	if (m_heartBeatSer==0)
	{
		m_heartBeatSer = new HeartBeatService(m_configfile->GetHeartUrl(),m_hostId);
	}
	if (m_heartBeatSer!=0)
	{
		m_heartBeatSer->Init();
	}
}

void CMDServer::InitIpMonitor()
{
	if (m_ipMonitor==0)
	{
		m_ipMonitor = new IPMonitor((byte*)m_ptr+IPMONITORDATAINIT,m_len-IPMONITORDATAINIT,true);
		printf("server ip monitor m_ptr:%d ,len:%d\n",IPMONITORDATAINIT,m_len-IPMONITORDATAINIT);
	}
	if (m_ipMonitor !=0)
	{
		m_ipMonitor->SetCmdSer(this);
		m_ipMonitor->ServerMonitTheIPData();
	}
}

void CMDServer::InitConfigFile()
{
	if (m_configfile==0)
	{
		m_configfile = new ConfigFileService();
	}
	if (m_configfile!=0)
	{
		m_configfile->Init();
		std::string tmp = m_configfile->GetUpdateUrl();
		printf("tmp ******************\n");
		printf("tmp url:%s \n",tmp.c_str());
	}
}

void CMDServer::ReInitScanFrame()
{
	if (m_testScanRecvFile!=0)
	{
		m_testScanRecvFile->ReInitScanFrame();
	}

}

void CMDServer::DataReportUpdateAction(std::string uuid)
{
	std::string con=DataReportAction(4,uuid,m_hostId);
	m_dataReport->SendDataReport(con);
	printf("update virlib   action data report\n");

}

void CMDServer::DataReportUpdateVirlib()
{
	if(m_dataReport==0)
	{
		printf("m_dataReport must init !!!! DataReportUpdateVirlib\n");
		return ;
	}
	BaseInfo baseInfo;
	baseInfo.set_host_name("kvm_linux");
	baseInfo.set_client_ver("1.0.0.0");
	BaseInfo_VirusEngine *b_VirusEngine = baseInfo.add_virus_engine();
	b_VirusEngine->set_engine_name("Zav");
	std::string englib;
	std::string virlib;
	if (GetZavPublic(englib, virlib))
	{
		b_VirusEngine->set_engine_ver(englib);
		b_VirusEngine->set_virus_lib_ver(virlib);
	}
	else
	{
		b_VirusEngine->set_engine_ver("1.0.0.0");
		b_VirusEngine->set_virus_lib_ver("2019:05:01 0000");
		//ZyWritelogByVSecure("INFO", "GetZavPublic error");
	}
	std::string content;
	baseInfo.SerializePartialToString(&content);
	ClientActionRequest clientRequest;
	clientRequest.set_client_id(m_hostId);
	clientRequest.set_action_type(ClientActionRequest_ActionType_REPORTED_BASE_INFO);
	clientRequest.set_action_data(content);
	std::string reportData;
	clientRequest.SerializePartialToString(&reportData);

	m_dataReport->SendDataReport(reportData);
	printf("[%s] data report update vir \n",GetCurrTimeStr().c_str());
}

void CMDServer::UpdateVirLib()
{
	std::string url= m_configfile->GetUpdateUrl();
	printf("UpdateVirLib %s\n",url.c_str());


	if (m_configfile!=0 && UpateMain(url))
	{
		//printf("UpateMain completed,reinit scanframe\n");
		ZyWritelogByVSecure("INFO","UpateMain completed,reinit scanframe");
		ReInitScanFrame();
		//printf("UpateMain completed,reinit ok\n");
		ZyWritelogByVSecure("INFO","UpateMain completed,reinit ok");
	}
	else
	{
		ZyWritelogByVSecure("INFO","UpateMain return false update error or no need update");
		//printf("UpateMain return falseupdate error or no need update\n");
	}
	DataReportUpdateVirlib();
	NodifyServerCMDCompelted(0,"update virlib");

}

void CMDServer::InitCMDServerLock()
{
	pthread_spinlock_t *splock = (pthread_spinlock_t *)((byte*)m_ptr);
	pthread_spin_init(splock, PTHREAD_PROCESS_SHARED);
}

void* CreateServerCMDThread(void * lParam)
{
	CMDServer* pserver = (CMDServer*)lParam;
	if (pserver != NULL)
	{
		pserver->CmdDoThread();

	}
	else
	{
		//printf("cmdServer thread lparam null\n");
	}
	return 0;
}

void* CreateServerUpdateVirlibThread(LPVOID lParam)
{
	CMDServer* pserver = (CMDServer*)lParam;
	if (pserver != NULL)
	{
		pserver->UpdateVirLib();

	}
	else
	{
		printf("cmdServer thread lparam null\n");
	}
	return 0;
}

void CMDServer::CmdDoThread()
{
	if (m_heartBeatSer ==0)
	{
		//printf("must init the m_heartBeatSer\n");
		ZyWritelogByVSecure("ERROR","Must be init the m_heartBeatSer");
		return ;
	}
	while (true)
	{
		CMDITEM item;
		if (m_heartBeatSer->GetCmdItem(item))
		{
			SendCMD((char*)item.uuid.c_str(), item.cmd, item.cleanVec);
		}
		else
		{
			sleep(3);
		}
	}


}

void CMDServer::StartServerUpdateVirThread()
{
	pthread_t tmp;
	pthread_attr_t a;
	pthread_attr_init(&a);
	pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
	pthread_create(&tmp, &a, CreateServerUpdateVirlibThread,this);
	printf("Server StartServerUpdateVirThread\n");

}

void CMDServer::StartServerSendThread(CMDServer*pRServer)
{

	pthread_t tmp;
	pthread_attr_t a;
	pthread_attr_init(&a);
	pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
	pthread_create(&tmp, &a, CreateServerCMDThread,pRServer);
	//printf("Server cmd  thread start \n");
	ZyWritelogByVSecure("INFO","Server cmd  thread start ",true);

}
//CleanFileDo tmpCleanFiledo;
void CMDServer::StartServerCleanThead(std::vector<std::string> &inCleanvec,std::string uuid)
{	
	if (m_testCleanSendFile ==0)
	{
		return;
	}
	m_testCleanSendFile->SetCurUuid(uuid);
	m_testCleanSendFile->SetSendVec(inCleanvec);
	CleanFileDo::CleanSendThread(m_testCleanSendFile);
}

void CMDServer::StartServerScanThead(std::string uuid)
{
	//todo cbx begin recv the client file context.and real scan
	if (m_testScanRecvFile == 0)
	{
		//printf("m_testScanRecvFile must be init\n");
		return;
	}
	m_testScanRecvFile->SetCurUuid(uuid);
	ScanFileDo::ScanRecvThread(m_testScanRecvFile,SCANFILEITEMNUM);
}

void CMDServer::DataReport(std::string uuid,std::string ip)
{
	do
	{
		if (m_dataReport==0)
		{
			printf("DataReport must init m_dataReport object !!!\n");
			break;
		}
		VirtualItem vsitem;
		vsitem.set_uuid(uuid);
		if (!ip.empty())
		{
			vsitem.set_ip(ip);
		}
		std::string vsitstr;
		vsitem.SerializePartialToString(&vsitstr);
		VirtualState vstate;
		VirtualState::VItem *pvitem = vstate.add_vitems();
		if (ip.empty())
		{
			pvitem->set_state(VirtualState::GuestState::VirtualState_GuestState_GUEST_CLOSE);
			printf("[%s] data report cmdser uuid :%s closed\n",GetCurrTimeStr().c_str(),uuid.c_str());
		}
		else
		{
			pvitem->set_state(VirtualState::GuestState::VirtualState_GuestState_GUEST_UPDATE);
			printf("[%s] data report cmdser uuid :%s update ip:%s\n",GetCurrTimeStr().c_str(),uuid.c_str(),ip.c_str());
		}
		pvitem->set_statedata(vsitstr);
		pvitem->set_time(time(NULL));
		std::string pvstr;
		vstate.SerializePartialToString(&pvstr);
		std::string allstr;
		ClientActionRequest crequest;
		crequest.set_client_id(m_hostId);
		crequest.set_action_type(ClientActionRequest::ActionType::ClientActionRequest_ActionType_VIRTUAL_STATE);
		crequest.set_action_data(pvstr);
		crequest.SerializePartialToString(&allstr);
		m_dataReport->SendDataReport(allstr);
	}while (0);

}

void CMDServer::DataReportCloseUuid(std::string uuid)
{
	DataReport(uuid, "");
	char buffer[128]={0};
	sprintf(buffer,"%s closed.or the client soft exit.",uuid.c_str());
	ZyWritelogByVSecure("INFO",buffer);
}

void CMDServer::TiggerThreadExit(int cmd)
{
	if (cmd == H3_CLEAN)
	{
		if (m_testCleanSendFile)
		{
			m_testCleanSendFile->TiggerThreadExit();
		}
	}
	else if (cmd ==H3_SCAN)
	{
		if (m_testScanRecvFile)
		{
			m_testScanRecvFile->TiggerThreadExit();
		}
	}

}
//true .the mac is open,false . the mac unexpected close
bool CMDServer::VirualMachineState(std::string uuid)
{
	bool res = true;
	if (m_heartvhostser)
	{
		res = m_heartvhostser->GetVirtualState(uuid);
	}
	return res;
}
int CMDServer::SendCMD(char *uuid, int cmd, std::vector<std::string> &incleanvec)
{
	int cmdstatus = 0;
	int ulen = strlen(uuid);
	if (ulen <= 0)
	{
		//printf("uuid param 0\n");
		ZyWritelogByVSecure("ERROR","sendcmd uuid param 0");
		return -1;
	}
	if (cmd == 2 && incleanvec.size() == 0)
	{
		//printf("clean filenum ==0,error\n");
		ZyWritelogByVSecure("ERROR","sendcmd clean filenum ==0,error");
		return -2;
	}
	int stime = 0;
	//printf("%s Will Send uuid %s  cmd  %d.........\n", GetCurrTimeStr().c_str(),uuid, cmd);
	bool comewrite = false;
	pthread_spinlock_t * splock;
	splock =(pthread_spinlock_t *)((byte*)m_ptr);
	if (splock==0)
	{
		//printf("can't get the cmd lock\n");
		ZyWritelogByVSecure("ERROR","can't get the cmd lock");
		return -3;
	}
	bool tiggerflag = false;
	while (true)
	{			
		//EnterCriticalSection(m_cmdlock);
		int rlen=-1;
		{
			WappLock tcs(splock);
			rlen = ReadData((byte*)m_ptr + CMDLOCKLEN + CMDCONTEXT, &cmdstatus, sizeof(cmdstatus));
			if (cmdstatus == 0)
			{
				printf("%s Real Send uuid %s  cmd  %d.........\n", GetCurrTimeStr().c_str(),uuid, cmd);
				if (cmd == H3_SCAN)//scan
				{
					m_cur_uuid = uuid;
					m_cur_cmd = cmd;
					comewrite = true;
					WriteCmd(m_ptr, uuid, ulen, cmd, 1, 0);
					//LeaveCriticalSection(m_cmdlock);
					//todo cbx start  thread .recv the client file from shmem
					//break;
					//todo cbx must wait the client recv the cmd .after continue the next task
					//becasue the uuid maybe close .so the cmd can't get
				}
				else if (cmd == H3_CLEAN)//clean
				{
					m_cur_uuid = uuid;
					m_cur_cmd = cmd;
					comewrite = true;
					WriteCmd(m_ptr, uuid, ulen, cmd, 1, incleanvec.size());
					//todo cbx start thread write the clean file to shmem
					//StartServerCleanThead(incleanvec);
					//todo cbx must wait the client recv the cmd .after continue the next task
					//becasue the uuid maybe close .so the cmd can't get
				}
				else if (cmd == H3_UPDATE)
				{
					//comewrite = true;
					//write upate status 2
					DataReportUpdateAction(uuid);
					WriteCmd(m_ptr, uuid, ulen, cmd, 2, 0);
					//start thread update.
					//update complete set the status to 0.
					StartServerUpdateVirThread();
					return 0;
				}
			}
#if 0
			else if (cmdstatus==2)
			{
				//the cmd is runing in the virual machine.but the if the virtual macchine
				//unexpected close. so must reset the cmdstatus.
				//
				if (!tiggerflag && !VirualMachineState(m_cur_uuid))
				{
					tiggerflag = true;
					//trigger the about thread exit.
					TiggerThreadExit(m_cur_cmd);
				}
			}
#endif
		}
		//LeaveCriticalSection(m_cmdlock);
		sleep(1);
		//EnterCriticalSection(m_cmdlock);
		//rlen = ReadData((byte*)m_ptr + CMDLOCKLEN + CMDCONTEXT, &cmdstatus, sizeof(cmdstatus));
		while (comewrite)
		{
			//EnterCriticalSection(m_cmdlock);
			{
				//WappLock tcs(splock);
				ReadData((byte*)m_ptr + CMDLOCKLEN + CMDCONTEXT, &cmdstatus, sizeof(cmdstatus));
			}
			//LeaveCriticalSection(m_cmdlock);
			if (cmdstatus == 2)
			{
				break;
			}
			if (cmdstatus == 1)
			{
				stime++;
			}
			else
			{
				stime = 0;
			}
			sleep(1);
			if (stime > 30)
			{
				break;
			}

		}
		if (comewrite)
		{
			if (stime > 30)
			{
				int rcs = 0;
				printf("%s the uuid %s maybe close. change the 1 to 0.so the after uuid can push the cmd\n", GetCurrTimeStr().c_str(),uuid);
				{
					WappLock tcs(splock);
					WriteData((byte*)m_ptr + CMDLOCKLEN + CMDCONTEXT, &rcs, sizeof(rcs));
				}
				std::string tuuid;
				tuuid.append(uuid);
				DataReportCloseUuid(tuuid);
				break;
			}
			else
			{
				if (cmd == H3_CLEAN)//clean cmd 
				{
					std::string tuuid;
					tuuid.append(uuid);
					printf("will send clean cmd %s\n",uuid);
					StartServerCleanThead(incleanvec,tuuid);
					char buffer[128]={0};
					sprintf(buffer,"%s clean start.",uuid);
					ZyWritelogByVSecure("INFO",buffer);
					break;
				}
				else if (cmd == H3_SCAN)//scan 
				{
					//toto cbx 
					std::string tuuid;
					tuuid.append(uuid);
					printf("will send scan cmd %s\n",uuid);
					StartServerScanThead(tuuid);
					char buffer[128]={0};
					sprintf(buffer,"%s scan start.",uuid);
					ZyWritelogByVSecure("INFO",buffer);
					break;
				}
				else if (cmd == H3_UPDATE)
				{
					break;
				}
			}
		}
		sleep(1);
	}
	return 0;

}
#if 0
void CMDServer::InitCMDServerLock(/*CRITICAL_SECTION *mcs*/)
{
	MEMSetZero(m_ptr, CMDTOTALLEN);
	//m_cmdlock = mcs;
	//WriteData((byte*)m_ptr, &m_cmdlock, sizeof(m_cmdlock));
}
#endif
CMDServer::~CMDServer()
{
	//DeleteCriticalSection(&m_tmptestCS);
}

CMDClient::CMDClient(void *mem, int len) :m_ptr(mem), m_len(len)
{
	m_ipMonitor = 0;
	m_heartClient = 0;
	m_testCleanRecvFile = new CleanFileDo((byte*)m_ptr + CMDTOTALLEN, m_len - CMDTOTALLEN,0, this);
	if (m_testCleanRecvFile != 0)
	{
		m_testCleanRecvFile->InitFileBackUp();
	}
	m_testScanSendFile = new ScanFileDo((byte*)m_ptr + INITSCANITEMDATA, m_len - INITSCANITEMDATA, 0, this);
	if (m_testScanSendFile != 0)
	{
#if 0
		std::vector<CRITICAL_SECTION*> tmpvec;
		GetGTSCANCS(tmpvec);
		m_testScanSendFile->InitScanDoCS(tmpvec);
#endif
		m_testScanSendFile->InitFileCache();
		m_testScanSendFile->InitCacheHash(false);
	}

	InitIpMonitor();
	m_heartClient=new HeartService((byte*)m_ptr + HEARTDATAINIT, m_len - HEARTDATAINIT,false);
}

void CMDClient::SetUUID(std::string uuid)
{
	m_uuid = uuid;
	if (m_ipMonitor !=0)
	{
		m_ipMonitor->SetUUID(uuid);
		m_ipMonitor->StartClientGetIpThread();
	}
	if (m_heartClient !=0)
	{
		m_heartClient->SetUUId(uuid);
		m_heartClient->StartClientMonitorHeart();
	}
}

void CMDClient::InitIpMonitor()
{
	if (m_ipMonitor==0)
	{
		m_ipMonitor = new IPMonitor((byte*)m_ptr+IPMONITORDATAINIT,m_len-IPMONITORDATAINIT,false);
	}
}
CMDClient::~CMDClient()
{

}

void CMDClient::StartClientScanThead()
{
	//todo cbx begin enum file and write file to shmem.
	if (m_testScanSendFile == 0)
	{
		printf("m_testScanSendFile must be init\n");
		return;
	}
	ScanFileDo::ScanSendThread(m_testScanSendFile,SCANFILEITEMNUM,m_testscanfolerlist);
}

void CMDClient::StartClientCleanThead(int cleannum)
{
	if (m_testCleanRecvFile == 0)
	{
		return;
	}
	m_testCleanRecvFile->SetCleanNum(cleannum);
	m_testCleanRecvFile->SetCurUuid(m_uuid);
	CleanFileDo::CleanRecvThread(m_testCleanRecvFile);
}

int CMDClient::GetTheEnumFileNum()
{
#if 0
	CRITICAL_SECTION *tmpcs = NULL;
	ReadData(m_ptr, &tmpcs, sizeof(tmpcs));
	if (tmpcs == NULL)
	{
		printf("client GetTheEnumFileNum get lock error\n");
		return 0;
	}
#endif
	int enumFilenum = 0;
	//EnterCriticalSection(tmpcs);
	int rlen = ReadData((byte*)m_ptr + CMDTOTALLENEXCEPTSNUM, &enumFilenum, sizeof(enumFilenum));
	//LeaveCriticalSection(tmpcs);

	return enumFilenum;

}

int CMDServer::GetTheEnumFileNum()
{
#if 0
	CRITICAL_SECTION *tmpcs = NULL;
	ReadData(m_ptr, &tmpcs, sizeof(tmpcs));
	if (tmpcs == NULL)
	{
		printf("server GetTheEnumFileNum get lock error\n");
		return 0;
	}
#endif
	int enumFilenum = 0;
	//EnterCriticalSection(tmpcs);

	int rlen = ReadData((byte*)m_ptr + CMDTOTALLENEXCEPTSNUM, &enumFilenum, sizeof(enumFilenum));
	//LeaveCriticalSection(tmpcs);

	return enumFilenum;

}

int CMDClient::NodifyServerEnumFileNum(int enumFileNum)
{
	pthread_spinlock_t * splock=0;
    splock = (pthread_spinlock_t * )((byte*)m_ptr);
	if (splock ==0)
	{
		printf("client NodifyServerEnumFileNum get lock error\n");
		return 0;
	}
	int enumFilenum = enumFileNum;
	{
		WappLock tcs(splock);
		int rlen = WriteData((byte*)m_ptr + CMDTOTALLENEXCEPTSNUM, &enumFilenum, sizeof(enumFilenum));
	}
	printf("[%s]client NodifyServerEnumFileNum %d\n",GetCurrTimeStr().c_str(),enumFileNum);
	return 0;

}

int CMDServer::NodifyServerEnumFileNum(int enumFileNum)
{
	pthread_spinlock_t * splock=0;
    splock = (pthread_spinlock_t * )((byte*)m_ptr);
	if (splock ==0)
	{
		printf("server NodifyServerEnumFileNum get lock error\n");
		return 0;
	}
	int enumFilenum = enumFileNum;
	{
		//WappLock tcs(splock);
		int rlen = WriteData((byte*)m_ptr + CMDTOTALLENEXCEPTSNUM, &enumFilenum, sizeof(enumFilenum));

	}
	//printf("server NodifyServerEnumFileNum %d\n",enumFilenum);

	return 0;

}

int CMDServer::NodifyServerCMDCompelted(int cmdstatuc, std::string log)
{
	pthread_spinlock_t * splock=0;
    splock = (pthread_spinlock_t * )((byte*)m_ptr);
	if (splock ==0)
	{
		printf("client NodifyServerCMDCompelted get lock error\n");
		return 0;
	}
	int cmdstatus = cmdstatuc;
	{
		WappLock tcs(splock);
		int rlen = WriteData((byte*)m_ptr + CMDLOCKLEN + CMDCONTEXT, &cmdstatus, sizeof(cmdstatus));
	}
	if (log.empty())
	{
		printf("[%s]CMDServer::NodifyServerCMDCompelted scan one complete\n",GetCurrTimeStr().c_str());
	}
	else
	{
		if (cmdstatuc==0)
		{
			SetNullCuruuid();
		}
		printf("[%s]CMDServer::NodifyServerCMDCompelted %s  one complete\n",GetCurrTimeStr().c_str(),log.c_str());
	}
	return 0;

}

int CMDClient::NodifyServerCMDCompelted(int cmdstatuc)
{
	pthread_spinlock_t * splock=0;
    splock = (pthread_spinlock_t * )((byte*)m_ptr);
	if (splock ==0)
	{
		printf("client NodifyServerCMDCompelted get lock error\n");
		return 0;
	}

	int cmdstatus = cmdstatuc;
	{
		WappLock tcs(splock);
		int rlen = WriteData((byte*)m_ptr + CMDLOCKLEN + CMDCONTEXT, &cmdstatus, sizeof(cmdstatus));
	}
	if (cmdstatuc == 0)
	{
		printf("%s client nodify uuid:%s completed\n", GetCurrTimeStr().c_str(),m_uuid.c_str());
	}
	return 0;
}
int CMDClient::RecvServerCmd()
{
	pthread_spinlock_t * splock;
	splock = (pthread_spinlock_t * )m_ptr;
	if (splock == 0)
	{
		printf("client recv cmd lock get error\n");
		return -1;
	}
	int cmdstatus = -1;
	while (true)
	{
		int rlen=0;
		{
			WappLock tcs(splock);
			rlen = ReadData((byte*)m_ptr + CMDLOCKLEN + CMDCONTEXT, &cmdstatus, sizeof(cmdstatus));
		}
		if (cmdstatus == 1)
		{
			//the uuid is self's uuid 
			//WriteCmd(m_ptr, uuid, ulen, cmd, 1);
			int ulen = 0;
			{
				WappLock tcs(splock);
				rlen = ReadData((byte*)m_ptr + CMDLOCKLEN + CMDCONTEXT + CMDSTATUS + CLEANFILENUM, &ulen, sizeof(ulen));
			}
			if (ulen > 0 && ulen < 40)
			{
				char *p = (char*)malloc(ulen+1);
				if (p != 0)
				{
					memset(p, 0, ulen + 1);
					int uclen=0;
					{
						WappLock tcs(splock);
						uclen = ReadData((byte*)m_ptr + CMDLOCKLEN + CMDCONTEXT + CMDSTATUS + CLEANFILENUM + rlen, p, ulen);
					}
					if (m_uuid.compare(p) == 0)
					{
						int cmdss = 2;
						int cmd = -1;
						//nodify the server the cmd recved 
						{
							WappLock tcs(splock);
							WriteData((byte*)m_ptr + CMDLOCKLEN + CMDCONTEXT, &cmdss, sizeof(cmdss));
							ReadData((byte*)m_ptr + CMDLOCKLEN, &cmd, sizeof(cmd));
						}
						if (cmd == H3_SCAN)//scan
						{
							//todo cbx
							//start thread enum the file ,and send the file context to shmem .so the server can read.
							StartClientScanThead();
							char buffer[128]={0};
							sprintf(buffer,"client %s scan start.",m_uuid.c_str());
							ZyWritelogByVSecure("INFO",buffer);
							printf("client recv scan cmd %s\n",m_uuid.c_str());
						}
						else if (cmd == H3_CLEAN)//clean
						{
							//todo cbx
							//start thread  read the clean file path.
							int cleannum = 0;
							int rcleanlen = ReadData((byte*)m_ptr + CMDLOCKLEN + CMDCONTEXT + CMDSTATUS, &cleannum, sizeof(cleannum));
							StartClientCleanThead(cleannum);
							char buffer[128]={0};
							sprintf(buffer,"client %s clean start.",m_uuid.c_str());
							ZyWritelogByVSecure("INFO",buffer);
							printf("client recv clean cmd %s \n",m_uuid.c_str());
						}
						else
						{
							printf("cmd recv not 1,2 .error\n");
						}					
					}
				    free(p);
				}
			}
		}
		sleep(1);
	}
	return 0;
}
void*  CreateClientRecvThread(LPVOID lParam)
{
	CMDClient* pclient = (CMDClient*)lParam;
	if (pclient != NULL)
	{
		pclient->RecvServerCmd();
	}
	else
	{
		printf("recv client thread lparam null\n");
	}
	return 0;
}

void CMDClient::StartClientRecvThread(CMDClient*pRclient)
{
	pthread_t tmp;
	pthread_attr_t a;
	pthread_attr_init(&a);
	pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
	pthread_create(&tmp, &a, CreateClientRecvThread,pRclient);
}

CleanFileDo::CleanFileDo(void *men, int len, CMDServer*ser, CMDClient *client) :m_ptr(men), m_len(len)
{
	m_cmdServer = ser;
	m_cmdClient = client;
	m_cleanNum = 0;
	m_fileBackup=0;

}
CleanFileDo::~CleanFileDo()
{

}

void CleanFileDo::InitFileBackUp()
{
	if (m_fileBackup==0)
	{
		m_fileBackup = new FileBackUp();
	}
	if (m_fileBackup !=0)
	{
		m_fileBackup->Init();
	}
}



void CleanFileDo::InitCleanDoCS()
{
	m_cleanNum = 0;
	pthread_spinlock_t * splock;
	for (int i = 0; i < CLEANNUM; i++)
	{
		splock=(pthread_spinlock_t *)((byte*)m_ptr+i*CLEANFILEITEM);
		pthread_spin_init(splock, PTHREAD_PROCESS_SHARED);
	}
	int rpos = (byte*)m_ptr-(byte*)G_LOCK_START;
	printf("clean lock start addr %d\n",rpos);
}

int CleanFileDo::GetNextEmptyItem()
{
	pthread_spinlock_t * splock;
	for (int i = 0; i < CLEANNUM; i++)
	{
		splock = (pthread_spinlock_t *)((byte*)m_ptr+i*CLEANFILEITEM);
		if (splock!=0)
		{
			int clstatus = -1;
			WappLock tcs(splock);
			ReadData((byte*)m_ptr + i*CLEANFILEITEM + 4, &clstatus, sizeof(clstatus));
			if (clstatus == 0 || clstatus == 2)
			{
				int rcls = 4;
				WriteData((byte*)m_ptr + i*CLEANFILEITEM + 4, &rcls, sizeof(rcls));
				return (i*CLEANFILEITEM);
			}
		}
	}
	return -1;
}
int CleanFileDo::GetNextFullItem()
{
	pthread_spinlock_t * splock;
	for (int i = 0; i < CLEANNUM; i++)
	{
		splock = (pthread_spinlock_t *)((byte*)m_ptr+i*CLEANFILEITEM);
		if (splock != 0)
		{
			int clstatus = -1;
			WappLock tcs(splock);
			ReadData((byte*)m_ptr + i*CLEANFILEITEM + 4, &clstatus, sizeof(clstatus));
			if (clstatus == 1)
			{			
				return (i*CLEANFILEITEM);
			}
		}
	}
	return -1;
}

void CleanFileDo::DataReportCleanFile()
{
	VirusStateUpdate virusState;
	virusState.set_start_date(time(NULL));
	int itemNum = m_testsendcleanvec.size();
	virusState.set_uuid(m_cur_uuid);
	for(int i = 0;i< itemNum ;i++)
	{
		//SCAN_ONE_ITEM item = scanReport->Item_Vector[i];

		VirusStateUpdate_RiskLog *plog = virusState.add_logs();
		plog->set_operated_date(time(NULL));
		char tmd5[33]={0};
		std::string tmpPath = m_cur_uuid+m_testsendcleanvec[i];
		if (GetBufferMd5A((unsigned char *)tmpPath.c_str(),(int)tmpPath.length(),tmd5))
		{
			plog->set_md5(tmd5);
		}
		else
		{
			plog->set_md5("3333xxxxffffxxxxffffxxxxffffeeee");
		}
		plog->set_path(m_testsendcleanvec[i]);
		plog->set_state(VirusStateUpdate_RiskState_CLEANED);
	}
	std::string content;
	virusState.SerializePartialToString(&content);

	ClientActionRequest clientRequest;
	clientRequest.set_client_id(m_cmdServer->GetHostId());
	clientRequest.set_action_type(ClientActionRequest_ActionType_UPDATE_VIRUS_STATE);
	clientRequest.set_action_data(content);
	std::string reportData;
	clientRequest.SerializePartialToString(&reportData);

	m_cmdServer->GetDataReport()->SendDataReport(reportData);
	printf("[%s]clean data report clean file num : %d \n", GetCurrTimeStr().c_str(),itemNum);
}
//action 1 scan start
//actin 2 scan end
//action 3 clean
//acation 4 updatevirlib
std::string DataReportAction(int action,std::string uuid,std::string hostid)
{
	std::string context;
	std::string acon;
	ClientAction cation;
	switch(action)
	{
		case 1:
			cation.set_action(ClientAction_Action_FULL_SCAN_START);
			break;
		case 2:
			cation.set_action(ClientAction_Action_FULL_SCAN_DONE);
			break;
		case 3:
			cation.set_action(ClientAction_Action_CLEAN_THREAT);
			break;
		case 4:
			cation.set_action(ClientAction_Action_START_VIRUS_UPGRADE);
			break;
		default:
			break;
	}
	cation.set_uuid(uuid);
	cation.set_action_state(ClientAction_ActionState_SUCCESS);
	cation.set_action_trigger_type(ClientAction_ActionTriggerType_AUTO);
	cation.set_time(time(NULL));
	cation.SerializeToString(&acon);
	ClientActionRequest acrequest;
	acrequest.set_client_id(hostid);
	acrequest.set_action_type(ClientActionRequest_ActionType_CLIENT_ACTION);
	acrequest.set_action_data(acon);
	acrequest.SerializePartialToString(&context);
	return context;
}
void CleanFileDo::DataReportCleanAction()
{
	std::string con=DataReportAction(3,m_cur_uuid,m_cmdServer->GetHostId());
	m_cmdServer->GetDataReport()->SendDataReport(con);
	printf("clean action data report\n");
}

void CleanFileDo::TiggerThreadExit()
{

}
int CleanFileDo::SendCleanFile(std::vector<std::string> &incleanvec)
{
	m_cleanNum = incleanvec.size();

	int sendNum = 0;
	pthread_spinlock_t * splock;
	while (sendNum < m_cleanNum)
	{
		int pos = GetNextEmptyItem();
		if (pos != -1)
		{
			std::string tmp = incleanvec[sendNum];
			int tmplen = tmp.length();

			splock = (pthread_spinlock_t *)((byte*)m_ptr + pos);
			if (splock == 0)
			{
				printf("get the send lock error\n");
				sendNum++;
				//todo cbx should change cleanstatus. becasue the next item can't get the empty item
				continue;
			}

			if (tmplen > 128)
			{
				sendNum++;
				printf("the path len too long\n");
				//todo cbx should change cleanstatus. becasue the next item can't get the empty item
				continue;
			}
			int cleanstatus = 1;
			{
				WappLock tcs(splock);
				int wlen = WriteData((byte*)m_ptr + pos + 4, &cleanstatus, sizeof(cleanstatus));
				int wtlen = WriteData((byte*)m_ptr + pos + 4+wlen, &tmplen, sizeof(tmplen));
				int pathlen = WriteData((byte*)m_ptr + pos + 4 + wlen + wtlen, (void*)tmp.c_str(), tmplen);
			}
			//printf("send %d pos path %s \n", pos / CLEANFILEITEM, tmp.c_str());
			sendNum++;
			//send clean file
		}
		else
		{
			sleep(2);
		}
	}
	return 0;
}
int CleanFileDo::RecvCleanFile(std::vector<std::string> &outcleanvec)
{
	int recvNum = 0;
	printf("client %s recv clean file path\n", m_cur_uuid.c_str());
	//todo cbx notice .the fullitem fail. rember empty the item.so the next can't get the empty item.
	pthread_spinlock_t * splock=0;
	while (recvNum < m_cleanNum)
	{
		int pos = GetNextFullItem();
		if (pos != -1)
		{
			int tmplen = 0;
			splock = (pthread_spinlock_t *)((byte*)m_ptr + pos);
			if (splock == 0)
			{
				printf("get the recv lock error\n");
				continue;
			}
			int cleanstatus = 2;
			int rlen;
			{
				WappLock tcs(splock);
				rlen = ReadData((byte*)m_ptr + pos + 4 + 4, &tmplen, sizeof(tmplen));
			}
			if (tmplen > 0  && tmplen <=128)
			{
				char *pbuffer = (char*)malloc(tmplen + 1);
				if (pbuffer != 0)
				{
					memset(pbuffer, 0, tmplen + 1);
					{
						WappLock tcs(splock);
						int pathlen = ReadData((byte*)m_ptr + pos + 4 + 4 + rlen, pbuffer, tmplen);
					}
					std::string tmpstr;
					tmpstr.append(pbuffer);
					//printf("Client recv the clean  path %s\n", tmpstr.c_str());
					free(pbuffer);
					pbuffer = 0;
					outcleanvec.push_back(tmpstr);
				}
			}
			{
				WappLock tcs(splock);
				int wlen = WriteData((byte*)m_ptr + pos + 4, &cleanstatus, sizeof(cleanstatus));
			}
			recvNum++;
		}
		else
		{
			sleep(2);
		}
	}

	//todo do someting
	m_fileBackup->BackUpList(outcleanvec);
	printf("the client recv total clean file is %d\n", outcleanvec.size());
	outcleanvec.clear();

	char buffer[128]={0};
	sprintf(buffer,"RecvCleanFile end %d files.",outcleanvec.size());
	ZyWritelogByVSecure("INFO",buffer);

	//nodify the server clean completed.
	if (m_cmdClient != 0)
	{
		m_cmdClient->NodifyServerCMDCompelted(0);
	}

	return 0;
}

void* CreateCleanSendThread(LPVOID lParam)
{
	CleanFileDo* pserver = (CleanFileDo*)lParam;
	if (pserver != NULL)
	{
		pserver->SendFile();
	}
	else
	{
		printf("CleanSend thread lparam null\n");
	}
	return 0;
}


void* CreateCleanRecvThread(LPVOID lParam)
{
	CleanFileDo* pserver = (CleanFileDo*)lParam;
	if (pserver != NULL)
	{
		pserver->RecvFile();
	}
	else
	{
		printf("CleanRecv thread lparam null\n");
	}
	return 0;
}
void CleanFileDo::CleanRecvThread(CleanFileDo*pRcleanclient)
{
	pthread_t tmp;
	pthread_attr_t a;
	pthread_attr_init(&a);
	pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
	pthread_create(&tmp, &a, CreateCleanRecvThread,pRcleanclient);

}

void CleanFileDo::SendFile()
{
	SendCleanFile(m_testsendcleanvec);
	m_cmdServer->SetNullCuruuid();
	DataReportCleanFile();
	char buffer[128]={0};
	sprintf(buffer,"%s clean completed.",m_cur_uuid.c_str());
	ZyWritelogByVSecure("INFO",buffer);
}

void CleanFileDo::RecvFile()
{
	RecvCleanFile(m_testrecvcleanvec);

}
void CleanFileDo::CleanSendThread(CleanFileDo*pRcleanclient)
{
	pthread_t tmp;
	pthread_attr_t a;
	pthread_attr_init(&a);
	pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
	pthread_create(&tmp, &a, CreateCleanSendThread,pRcleanclient);
	pRcleanclient->DataReportCleanAction();
}
int ScanFileDo::m_cs_flag = 0;

void ScanFileDo::InitScanDoCS()
{
	m_EnumTotalNum = 0;
	pthread_spinlock_t * splock;
	for (int i = 0; i < SCANFILEITEMNUM; i++)
	{
		splock = (pthread_spinlock_t*)((byte*)(m_ptr+i*SCANFILEITEM));
		pthread_spin_init(splock, PTHREAD_PROCESS_SHARED);
	}
	//todo cbx test tmp
	//InitGTestHashMap();
}

void ScanFileDo::StartLocalTestScanThread()
{
#ifdef LOCAL_FILE_TEST
		for(int i=0;i<4;i++)
		{
			m_scanstatus = 0;
			m_realScan->AddTestLocalFile();
			printf("Add...............................\n");
			m_realScan->CreateScanThread();
			m_scanstatus = 2;
			sleep(30);
			//m_realScan->AddTestLocalFile();
		}
#endif
}

void ScanFileDo::InitRealScan()
{
	m_scanstatus_mutex = PTHREAD_MUTEX_INITIALIZER;
	if (m_realScan==0)
	{
		m_realScan = new RealScan(this,REALSCANTHREADNUM);
	}
	if (m_realScan!=0)
	{
		m_realScan->InitScanEngine();
	}
}

void ScanFileDo::InitCacheHash(bool ser)
{
	if (m_cacheHash==0)
	{
		m_cacheHash = new CacheHash((byte*)m_ptr+INITCACHEITEMDATA,m_len-INITCACHEITEMDATA,this);
	}
	else
	{
		return;
	}

	if (m_cacheHash!=0 && ser)
	{
		m_cacheHash->InitLock();
	}


}

void ScanFileDo::InitFileCache()
{
	if (m_fileCache==0)
	{
		m_fileCache = new FileCache();
	}
	else
	{
		return;
	}
	if (m_fileCache!=0)
	{
		m_fileCache->Init();
		m_fileCache->ReadFileCache();
	}
}
int ScanFileDo::GetNextEmptyItem()
{
	pthread_spinlock_t * splock=0;
	for (int i = 0; i < SCANFILEITEMNUM; i++)
	{
		splock = (pthread_spinlock_t*)((byte*)(m_ptr+i*SCANFILEITEM));
		if (splock != 0)
		{
			int clstatus = -1;
			WappLock tcs(splock);
			ReadData((byte*)m_ptr + i*SCANFILEITEM + 4, &clstatus, sizeof(clstatus));
			if (clstatus == 0 || clstatus == 2)
			{
				int rcls = 4;
				WriteData((byte*)m_ptr + i*SCANFILEITEM + 4, &rcls, sizeof(rcls));
				return (i*SCANFILEITEM);
			}
		}
	}
	return -1;
}
int ScanFileDo::GetNextFullItem()
{
	pthread_spinlock_t * splock=0;
	int pos = -1;
	for (int i = 0; i < SCANFILEITEMNUM; i++)
	{
		pos = i*SCANFILEITEM;
		splock = (pthread_spinlock_t*)((byte*)(m_ptr+pos));
		if (splock != 0)
		{
			int clstatus = -1;
			WappLock tcs(splock);
			ReadData((byte*)m_ptr + pos + 4, &clstatus, sizeof(clstatus));
			if (clstatus == 1)
			{
				clstatus = 4;
				WriteData((byte*)m_ptr + pos + 4, &clstatus, sizeof(clstatus));
				return pos;
			}
		}
	}
	return -1;
}

int GetFileSizeCon(const char *filename)
{
	int fsize = 0;
    struct stat buf;
    if(0==stat(filename, &buf))
    {
    	fsize = buf.st_size;
    }
	return fsize;
}


int WriteFileToShmem(std::string filepath,unsigned int hash,void * mem, int maxlen)
{
	int res = -1;
	int cmdstatus = 1;
	pthread_spinlock_t *splock=0;
	splock=(pthread_spinlock_t *)((byte*)mem);
	if (splock == 0)
	{
		cmdstatus = 0;
		int wstlen = WriteData((byte*)mem + 4, &cmdstatus, sizeof(cmdstatus));
		return -1;
	}
	FILE *fp = fopen(filepath.c_str(), "r");
	if (fp == 0)
	{
		printf("create open %s exist file error \n", filepath.c_str());
		cmdstatus = 0;
		WappLock tcs(splock);
		int wstlen = WriteData((byte*)mem + 4, &cmdstatus, sizeof(cmdstatus));
		return -1;
	}
	
	int files = GetFileSizeCon(filepath.c_str());
    if (files ==0)
    {
    	fclose(fp);
    	printf("GetFileSizeCon 0 %s\n", filepath.c_str());
		cmdstatus = 0;
		WappLock tcs(splock);
		int wstlen = WriteData((byte*)mem + 4, &cmdstatus, sizeof(cmdstatus));
    	return -1;
    }
	if (files > maxlen)
	{
		fclose(fp);
		cmdstatus = 0;
		WappLock tcs(splock);
		int wstlen = WriteData((byte*)mem + 4, &cmdstatus, sizeof(cmdstatus));
		printf("%s 's size too big %d\n", filepath.c_str(), files);
		return -1;
	}

	if (files != 0)
	{
		//write to shmem
		do
		{
			int plen = filepath.length();
			//printf("file size %d \n",files);
			WappLock tcs(splock);
			int wstlen = WriteData((byte*)mem + 4, &cmdstatus, sizeof(cmdstatus));
			int wshashtlen = WriteData((byte*)mem + 4+4, &hash, sizeof(hash));
			int wplen = WriteData((byte*)mem + 4 + 4+4+wstlen, &plen, sizeof(plen));
			int wpclen = WriteData((byte*)mem + 4 + 4+4+wstlen + wplen, (void*)filepath.c_str(), plen);
			int wpconlen = WriteData((byte*)mem + 4 +4+4+ wstlen + wplen + wpclen, &files, sizeof(files));
			int pos = wstlen + wplen + wpclen + wpconlen+4+4+4;

			unsigned long rsize = 0;
			unsigned long tfiles = files;
			void *cpos = (byte*)mem + pos;
			int realsize =0;
			rsize = fread(cpos,1,tfiles,fp);
			realsize = rsize+realsize;
			//printf("read size%d\n",rsize);
			while (tfiles != rsize  && rsize !=0)
			{
				tfiles = tfiles - rsize;
				cpos = (byte*)cpos + rsize;
				rsize = fread(cpos,1,tfiles,fp);
				if(rsize ==0)
				{
					break;
				}
				realsize = rsize+realsize;
			}
			if (files != realsize)
			{
				int wpconlen = WriteData((byte*)mem + 4 +4+4+ wstlen + wplen + wpclen, &realsize, sizeof(files));
			}

			res = 0;

		} while (0);
	}
	fclose(fp);
	return res;
}
std::string GetPathFileName(std::string path)
{
	int plen = path.length();
	char *p = "/";
	char *pos = 0;
	int i;
	for ( i= plen - 1; i > 0; i--)
	{
		if (path[i] == *p)
		{
			//path[i] = 0;
			pos = (char*)path.c_str() + i+1;
			break;
		}
	}
	if (i==0)
	{
		p="\\";
		for ( i= plen - 1; i > 0; i--)
		{
			if (path[i] == *p)
			{
				//path[i] = 0;
				pos = (char*)path.c_str() + i+1;
				break;
			}
		}

	}
	std::string fname;
	if (pos != 0)
	{
		fname.append(pos);
	}
	return fname;
}

int SaveFile(std::string fname, void *men)
{
	int filesize = -1;


	FILE * hFile = fopen(fname.c_str(),"w+");
	if (hFile==0)
	{
		printf("fopen file error %s\n", fname.c_str());
		return filesize;
	}
	do
	{
		int conlen = 0;
		int rclen = ReadData(men, &conlen, sizeof(conlen));
		if (conlen > MAXFILELEN)
		{
			printf("file size too big \n",conlen);
			break;
		}
		int rfpos = (byte*)men -(byte*)G_LOCK_START+4;
		printf("fpos:%d conlen %d\n",rfpos,conlen);
		filesize = conlen;
		size_t wfiles = conlen;
		int wrealen = 0;
		void * pos =(byte*)men + rclen;
		size_t wres = fwrite(pos, 1, wfiles, hFile);
		while (wfiles != wres)
		{
			wfiles = wfiles - wres;
			pos = (byte*)pos + wres;
			wres = fwrite(pos, 1, wfiles, hFile);
		}
	} while (0);
	fclose(hFile);

	return filesize;
}

int  GenTmpFileSave(std::string filepath, void *mem,std::string sufx)
{
	int filesize = -1;
	//tmp file in curr folder file name is the last filepath's name.
	std::string curpath = GetCurrPath();
	std::string fname = GetPathFileName(filepath);
	fname = sufx+"_"+fname;
	if (!curpath.empty() && !fname.empty())
	{
		curpath = curpath + "/testfile/" + fname;
		printf("%s\n",curpath.c_str());
		filesize = SaveFile(curpath, mem);
	}

	return filesize;
}

int ReadFileContextFromShmem(std::string &outfilepath, void * mem, int *fconlen)
{
	unsigned long tid = pthread_self();
	int res = -1;
	int cmdstatus = 2;
	do
	{

		pthread_spinlock_t *splock=0;
		splock =(pthread_spinlock_t *)((byte*)mem);
		if (splock == 0)
		{
			printf("ReadFileContextFromShmem get lock error\n");
			int rs = WriteData((byte*)mem + 4, &cmdstatus, sizeof(cmdstatus));
			break;
		}
		int plen = 0;
		{

			int wplen=0;
			{
				WappLock tcs(splock);
				int rpos = (byte*)mem-(byte*)G_LOCK_START+16;
				wplen = ReadData((byte*)mem + 4 + 4+4+4, &plen, sizeof(plen));
				//printf("%lu read pos %d,value %d\n",tid,rpos,plen);
			}
			if (plen > 0 && plen <=128)
			{
				char *pbuffer = (char*)malloc(plen + 1);
				if (pbuffer == 0)
				{
					printf("ReadFileContextFromShmem malloc %d error\n",plen);
					int rs = WriteData((byte*)mem + 4, &cmdstatus, sizeof(cmdstatus));
					break;
				}
				memset(pbuffer, 0, plen + 1);
				{
					WappLock tcs(splock);
					int wpclen = ReadData((byte*)mem + 4 + 4 + 4+4+4, pbuffer, plen);
				}
				outfilepath.append(pbuffer);
				free(pbuffer);
				//tmp test so save the data to the tmp file.real should scan the file.
				//int wfileres = GenTmpFileSave(outfilepath, (byte*)mem + 4 + 4 + 4 + 4+4+plen,"sx");
				res = 4 + 4 + 4 + 4+4+plen;
				int conlen = 0;
				int rclen = ReadData((byte*)mem+res, &conlen, sizeof(conlen));
				if (conlen < MAXFILELEN)
				{
					*fconlen=conlen;
					res=res+rclen;
					//cmdstatus = 4;
					//todo set 4 .1 wait too long
					//WappLock tcs(splock);
					//int rs = WriteData((byte*)mem + 4, &cmdstatus, sizeof(cmdstatus));
				}
				else
				{
					WappLock tcs(splock);
					int rs = WriteData((byte*)mem + 4, &cmdstatus, sizeof(cmdstatus));
					res = -1;
				}
			}
			else
			{
				printf("%lu ReadFileContextFromShmem plen %d  too big error\n", tid,plen);
				int i = 8;
				int j = 0;
				i = i/j;
				printf("%d,\n",i);
				WappLock tcs(splock);
				int rs = WriteData((byte*)mem + 4, &cmdstatus, sizeof(cmdstatus));
			}
		}
	} while (0);

	return res;
}

unsigned int GetFileHash(std::string file,bool *gres)
{
	unsigned int fhash=0;
	bool fhres = GetFileAttrCRC(file.c_str(),&fhash);
	*gres=fhres;
	return fhash;
}

void CacheHash::EnumFile()
{
	//m_enum_flag=false;
	int totalFile = 0;
	while(!m_enumfolderlist.empty())
	{
		std::list<std::string>::iterator it =m_enumfolderlist.begin();
		std::list<std::string> tmpfilelist;
		GetFolderFile(*it, tmpfilelist,m_enumfolderlist);
		//printf("folder :%s \n",it->c_str());
		m_enumfolderlist.pop_front();
		pthread_mutex_lock(&m_prelist_mutex);
		std::list<std::string>::iterator fit = tmpfilelist.begin();
		for(;fit!=tmpfilelist.end();fit++)
		{
			m_prelist.push_back(*fit);
			totalFile++;
		}
		pthread_mutex_unlock(&m_prelist_mutex);
	}
	pthread_mutex_lock(&m_prelist_mutex);
	m_enum_flag=true;
	pthread_mutex_unlock(&m_prelist_mutex);
	printf("!!!!!!!!!!!!!!!!!!!!EnumFile end fileNum %d!!!!!!!!!!!!!!!!!!!!!!\n",totalFile);

}

void ScanFileDo::EnumFile()
{
	if (m_cacheHash!=0)
	{
		m_cacheHash->SetEnumFlag(false);
		m_cacheHash->EnumFile();
	}
}

int ScanFileDo::SendScanFile()
{
	if (m_cmdClient == 0)
	{
		printf("SendScanFile the m_cmdClient is null .error,must init the m_cmdClient\n");
		return 0;
	}
	unsigned long tid = pthread_self();
	//m_sendfile_num = 0;
	int totalFileNum = 0;
	int folderNum=0;
	int fileNumTotal = 0;
	printf("m_hashFileList 's NUM  :%d end\n",m_hashFileList.size());
	int sleepTime = 0;
	int emptytime=0;
	int fileNum = 0;
	while (true)
	{
		pthread_mutex_lock(&m_hashFilelist_mutex);
		if (m_hashFileList.empty() && m_cache_endFlag)
		{
			pthread_mutex_unlock(&m_hashFilelist_mutex);
			break;
		}
		else if(m_hashFileList.empty())
		{
			pthread_mutex_unlock(&m_hashFilelist_mutex);
			sleep(1);
			continue;
		}
		else
		{
			pthread_mutex_unlock(&m_hashFilelist_mutex);
		}
		int pos = GetNextEmptyItem();
		if (pos != -1)
		{
			//printf("pos %d write cache hash %s\n",pos,tpath.c_str());;
			pthread_mutex_lock(&m_hashFilelist_mutex);
			if (!m_hashFileList.empty())
			{
				std::list<Hash_FileItem>::iterator it = m_hashFileList.begin();
				Hash_FileItem item=*it;
				m_hashFileList.pop_front();
				pthread_mutex_unlock(&m_hashFilelist_mutex);

				if (WriteFileToShmem(item.filename, item.fhash,(byte*)m_ptr + pos, MAXFILELEN) != -1)
				{
					//m_sendfile_num++;
					//printf("[%lu] hasd %ld %s pos %d,%s write to shmem\n",tid,item.fhash,GetCurrTimeStr().c_str(),pos,item.filename.c_str());
					totalFileNum = totalFileNum + 1;
				}
				fileNum++;
				if (fileNum%500==0)
				{
					printf("[%s] %d 's 500 ,%d realwrite %d\n",GetCurrTimeStr().c_str(),fileNum/500,fileNum,totalFileNum);
				}
				emptytime=0;
			}
			else
			{
				pthread_mutex_unlock(&m_hashFilelist_mutex);
				{
					//need  to set empty.
					pthread_spinlock_t * splock=0;
					splock = (pthread_spinlock_t*)((byte*)(m_ptr+pos));
					if (splock != 0)
					{
						int clstatus = -1;
						WappLock tcs(splock);
						int rcls = 0;
						WriteData((byte*)m_ptr + pos + 4, &rcls, sizeof(rcls));
					}
					else
					{
						int rcls = 0;
						WriteData((byte*)m_ptr + pos + 4, &rcls, sizeof(rcls));
					}

				}
				sleep(1);
				emptytime++;
				if (emptytime>300)
				{
					emptytime = 0;
					printf("[%lu][%s]**********m_hashFileList is emptye sleep 300s **********\n",tid,GetCurrTimeStr().c_str());
				}
			}
			sleepTime=0;
		}
		else
		{
			sleep(1);
			sleepTime++;
			if (sleepTime > 300)
			{
				sleepTime = 0;
				printf("[%lu][%s]Sleep 300 s there is not empty item to cp file context .\n",
						tid,GetCurrTimeStr().c_str());
			}
		}
	}
	pthread_mutex_lock(&m_folder_list_mutex);
	m_worksendthreadNum=m_worksendthreadNum-1;
	m_sendfile_num=m_sendfile_num+totalFileNum;

	//write num to shmem
	printf("SendScanFile exit...fileNum %d ,thread no %d\n",totalFileNum,m_worksendthreadNum);
	if (m_worksendthreadNum==0)
	{
		pthread_mutex_unlock(&m_folder_list_mutex);
		printf("################ FileTotal %d ################\n",m_sendfile_num);
		int snuend=SCANENUMENDFLAG;
		m_cmdClient->NodifyServerEnumFileNum(snuend);
		SetCltScanEndTime();
		TotalCltTime();
		char buffer[128]={0};
		sprintf(buffer,"SendScanFile end %d files.",m_sendfile_num);
		ZyWritelogByVSecure("INFO",buffer);
	}
	else
	{
		pthread_mutex_unlock(&m_folder_list_mutex);
	}

	return 0;
}

int ReadTheHash(void *men, pthread_spinlock_t *splock)
{
	unsigned int hash = 0;
	{
		WappLock tcs(splock);
		int i = ReadData((byte*)men + 4 + 4, &hash, sizeof(hash));
	}
	return hash;
}


int WriteCacheResToShmem(void *men, int cacheres, pthread_spinlock_t *splock)
{
	int res = 0;
	int status = 6;
	int cachr = cacheres;
	{
		WappLock tcs(splock);
		int i = WriteData((byte*)men + 4 + 4 + 4, &cachr, sizeof(cachr));
		int i1 = WriteData((byte*)men + 4, &status, sizeof(status));
		res = i + i1;
	}
	return res;
}

std::map<int, int>  GTestHashMap;
void InitGTestHashMap()
{
	GTestHashMap[134] = 1;
	GTestHashMap[135] = 1;
	GTestHashMap[136] = 0;
	GTestHashMap[137] = 1;
	GTestHashMap[138] = 0;
}


int QueryServerCache(unsigned int fhash, FileCache *fileCache)
{
	int res = 0;
	if (fileCache == 0)
	{
		return res;
	}
	bool vres = false;
    bool bres =  fileCache->QueryCache(fhash,&vres);
    if (bres)
    {
    	if (!vres)
    	{
    		res =1;
    	}
    }
	return res;
}

void ScanFileDo::AddCache(unsigned int fhash, bool res)
{
	if (m_fileCache !=0)
	{
		m_fileCache->AddCache(fhash,res);
	}
}
void ScanFileDo::ServerNodiyRealScanComplete()
{
	if (m_cmdServer!=0)
	{
		m_cmdServer->NodifyServerCMDCompelted(0);
	}
	DateReportScanAction(2);
	SetSerScanEndTime();
	TotalSerTime();
	char buffer[128]={0};
	sprintf(buffer,"%s scan completed.",m_cur_uuid.c_str());
	ZyWritelogByVSecure("INFO",buffer);

}

void ScanFileDo::WriteOneFileScanStatus(void *mem)
{
	pthread_spinlock_t * splock=0;
	splock = (pthread_spinlock_t * )((byte*)mem);
	if (splock)
	{
		int status = 2;
		WappLock tcs(splock);
		int ir = WriteData((byte*)mem + 4, &status, sizeof(status));
	}
}
//action 1 start
//action 2 complete
void ScanFileDo::DateReportScanAction(int action)
{
	std::string con=DataReportAction(action,m_cur_uuid,m_cmdServer->GetHostId());
	m_cmdServer->GetDataReport()->SendDataReport(con);
	printf("scan %d  action data report\n",action);

}

void ScanFileDo::TiggerThreadExit()
{
	if (m_cacheHash)
	{
		//printf("m_cacheHash->TiggerThreadExit() before\n");
		m_cacheHash->TiggerThreadExit();
		//printf("m_cacheHash->TiggerThreadExit() after\n");
	}
	int snuend=SCANENUMENDFLAG;
	if (m_cmdServer)
	{
		//printf("m_cmdServer->NodifyServerEnumFileNum(snuend) before\n");
		m_cmdServer->NodifyServerEnumFileNum(snuend);
		//printf("m_cmdServer->NodifyServerEnumFileNum(snuend) after\n");
	}
	else
	{
		printf("m_cmdServer null.................\n");
	}

}

int ScanFileDo::RecvScanFile()
{
	int scanFileNum = 0;
	int realscanFileNum = 0;
	int realscanFileNumPush = 0;
	if (m_cmdServer == 0)
	{
		printf("RecvScanFile the m_cmdServer is null .error,must init the m_cmdServer\n");
		return 0;
	}
	unsigned long tid = pthread_self();
	//pthread_mutex_lock(&m_scanstatus_mutex);
	m_scanstatus =1;
	int sleepTime=0;
	//pthread_mutex_unlock(&m_scanstatus_mutex);
	while (true)
	{
		pthread_spinlock_t * splock=0;
		int pos = GetNextFullItem();
		if(pos !=-1)
		{
			//todo cbx scan the shmem file context.
			//should use the stream interface scan.
		    splock = (pthread_spinlock_t * )((byte*)m_ptr+pos);
		    sleepTime=0;
			if (splock ==0)
			{
				printf("RecvScanFile get lock error\n");
				continue;
			}
			std::string filepath;
			int conlen = 0;
			unsigned int tfhash=0;
			{
				WappLock tcs(splock);
				int rs = ReadData((byte*)m_ptr + pos + 4+4, &tfhash, sizeof(tfhash));
			}
			int rpos = (byte*)m_ptr-(byte*)G_LOCK_START+pos;
			//printf("<<%lu rpos %d hash:%d >>>>>>>>>>>>>>\n",tid,rpos,tfhash);
			int streampos = ReadFileContextFromShmem(filepath, (byte*)m_ptr + pos,&conlen);
			//todo cbx real scan
			if (!filepath.empty() && streampos != -1)
			{
				//printf("<<%lu pos %d,Real scan %s \n",tid,rpos,filepath.c_str());
				//scan res write to the cache
				ScanItem tmp;
				tmp.filePath = filepath;
				tmp.mem_pos = (byte*)m_ptr + pos;
				tmp.mem_len = conlen;
				tmp.mem_fileConStart = streampos;
				tmp.fhash = tfhash;
				AddCache(tfhash,false);
				m_realScan->PushScanItem(tmp);
				realscanFileNumPush++;
				if (realscanFileNumPush%10000==0)
				{
					printf("<<NO.%d,%lu pos %d,Real scan %s \n",realscanFileNumPush/10000,
							tid,rpos,filepath.c_str());
				}
			}
			realscanFileNum++;
		}
		else
		{
			int numfile = m_cmdServer->GetTheEnumFileNum();
			if (/*numfile != 0 && */numfile == SCANENUMENDFLAG)
			{	//todo cbx tell the server .the scan completed.
				break;
			}
			sleep(1);
			sleepTime++;
			if (sleepTime>300)
			{
				printf("[%lu][%s]*************recv file sleep 300s %d ***************\n",
						tid,GetCurrTimeStr().c_str(),numfile);
				sleepTime=0;
			}
		}
	}
	pthread_mutex_lock(&m_folder_list_mutex);
	m_workrecvthreadNum--;
	if (m_workrecvthreadNum==0)
	{
		m_scanstatus =2;
		pthread_mutex_unlock(&m_folder_list_mutex);
		m_cmdServer->NodifyServerEnumFileNum(0);
		//m_cmdServer->NodifyServerCMDCompelted(0);
		printf("[%s]~~~~~~~~~~~~~~server nodify uuid completed %d real %d push ~~~~~~~~~~~~~~%d\n", GetCurrTimeStr().c_str(),
			scanFileNum,realscanFileNum,realscanFileNumPush);
	}
	else
	{
		pthread_mutex_unlock(&m_folder_list_mutex);
	}
	return 0;
}

void* CreateScanRecvThread(LPVOID lParam)
{
	ScanFileDo* pserver = (ScanFileDo*)lParam;
	if (pserver != NULL)
	{
		pserver->RecvScanFile();
	}
	else
	{
		printf("ScanRecv thread lparam null\n");
	}
	return 0;
}

void* CreateCacheRequestThread(LPVOID lparam)
{
	CacheHash * tmpCache= (CacheHash*)lparam;

	if (tmpCache==0)
	{
		printf("CreateCacheRequestThread thread lparam null\n");
		return 0;
	}
	else
	{
		printf("CacheRequestThread start.......\n");
	}
	tmpCache->AddCacheAvtiveNum();
	tmpCache->CacheRequestThread();
}
void* CreateCacheReplyThread(LPVOID lparam)
{
	static int i =0;
	i++;
	CacheHash * tmpCache= (CacheHash*)lparam;
	if (tmpCache==0)
	{
		printf("CreateCacheReplyThread thread lparam null\n");
		return 0;
	}
	else
	{
		printf("N9.%d CacheReplyThread start.......\n",i);
	}
	tmpCache->CacheReplyThread();
}

void* CreateCacheBackThread(LPVOID lparam)
{
	CacheHash * tmpCache= (CacheHash*)lparam;
	if (tmpCache==0)
	{
		printf("CreateCacheBackThread thread lparam null\n");
		return 0;
	}
	else
	{
		//printf("CacheReplyThread start.......\n");
	}
	tmpCache->CacheRequestBackThread();

}

void CacheHash::CreateCacheHashReuestThread(int threadNum)
{
	//m_cacheThreadNum = threadNum;
	//m_cacheThreadNumNotChange=threadNum;
	WriteCacheEndFlag(0);
	m_createdCacheThNum=m_createdCacheThNum+threadNum;
	for(int i=0;i<threadNum;i++)
	{
		pthread_t tmp;
		//printf("No %d request thread start \n",i);
		pthread_attr_t a;
		pthread_attr_init(&a);
		pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
		int res = pthread_create(&tmp, &a, CreateCacheRequestThread,this);
		if (res !=0)
		{
			m_createdCacheThNum--;
			printf("The NO.%d thread create failed\n",i);
			//m_cacheThreadNumNotChange--;
		}
	}

}

void CacheHash::CreateCacheHashBackThread(int threadNum)
{
	m_cacheBackNum= threadNum;
	for(int i=0;i<m_cacheBackNum;i++)
	{
		pthread_t tmp;
		pthread_attr_t a;
		pthread_attr_init(&a);
		pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
		pthread_create(&tmp, &a,CreateCacheBackThread,this);
	}

}
void CacheHash::CreateCacheHashReplyThread(int threadNum)
{
	WriteCacheEndFlag(0);
	m_cacheReplyNum = threadNum;
	int errnum = 0;
	for(int i=0;i<m_cacheReplyNum;i++)
	{
		pthread_t tmp;
		pthread_attr_t a;
		pthread_attr_init(&a);
		pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
		int err = pthread_create(&tmp, &a, CreateCacheReplyThread,this);
		if (err!=0)
		{
			errnum++;
		}
	}
	printf("%d num thread create error\n",errnum);
}

void ScanFileDo::SetSerScanStartTime()
{
	m_cur_scanFileNum = 0;
	m_cur_scanVec.clear();
	m_scanSerStarttime=GetCurrSecondTime();
}
void ScanFileDo::SetSerScanEndTime()
{
	m_scanSerEndTime=GetCurrSecondTime();
}

void ScanFileDo::SetCltScanStartTime()
{
	m_scanCltStarttime=GetCurrSecondTime();
}
void ScanFileDo::SetCltScanEndTime()
{
	m_scanCltEndTime=GetCurrSecondTime();
}

void ScanFileDo::TotalSerTime()
{
	long int sertime = m_scanSerEndTime-m_scanSerStarttime;
	printf(">>>>>>>>>>>>>>>>>Ser scan Time %ld<<<<<<<<<<<<<<<<<\n",sertime);

}
void ScanFileDo::TotalCltTime()
{
	long int clTime = m_scanCltEndTime-m_scanCltStarttime;
	printf("<<<<<<<<<<<<<<<<<<clt scan time %ld>>>>>>>>>>>>>>>>>\n",clTime);
}

void ScanFileDo::AddVirs(std::string filepath,int flen,std::string virname)
{
	SCAN_ONE_ITEM oneItem;
	oneItem.operate_date = time(NULL);
	oneItem.path = filepath;
	std::string tmpPath = m_cur_uuid+filepath;
	char tmd5[33]={0};
	if (GetBufferMd5A((unsigned char *)tmpPath.c_str(),(int)tmpPath.length(),tmd5))
	{
		oneItem.md5.append(tmd5);
	}
	else
	{
		oneItem.md5.append("3333xxxxffffxxxxffffxxxxffffeeee");
	}
	ZyThreats::PVIRUS_DESC vdesc = ZyThreats::GetDescTableByVirusName(virname);
	oneItem.type=vdesc->VirusName;
	oneItem.threat_type = vdesc->VirusName;
	oneItem.size = flen;
	m_cur_scanVec.push_back(oneItem);
}

void ScanFileDo::DataReportVirs()
{
	Virus virusreport;
	virusreport.set_start_date(m_scanSerStarttime);
	long int curtint = GetCurrSecondTime();
	virusreport.set_cost_time(curtint - m_scanSerStarttime);
	virusreport.set_scan_type(Virus_ScanType_ALL_SCAN);
	virusreport.set_engines(Virus_Engines_ZAV);
	virusreport.set_scan_state(Virus_ScanState_SUCCESS);
	virusreport.set_scaned_files_num(m_cur_scanFileNum);
	virusreport.set_uuid(m_cur_uuid);
	int itemNum = m_cur_scanVec.size();
	for(int i = 0;i< itemNum ;i++)
	{
		SCAN_ONE_ITEM item = m_cur_scanVec[i];
		Virus_RiskLog *plog = virusreport.add_logs();
		plog->set_operated_date(item.operate_date);
		plog->set_md5(item.md5);
		plog->set_path(item.path);
		plog->set_type(item.type);
		plog->set_size(item.size);
		plog->set_state(Virus_RiskState_UN_HANDLED);
		plog->set_threat_type(item.threat_type);
	}
	std::string content;
	virusreport.SerializePartialToString(&content);

	ClientActionRequest clientRequest;
	clientRequest.set_client_id(m_cmdServer->GetHostId());
	clientRequest.set_action_type(ClientActionRequest_ActionType_REPORTED_VIRUS);
	clientRequest.set_action_data(content);
	std::string reportData;
	clientRequest.SerializePartialToString(&reportData);

	m_cmdServer->GetDataReport()->SendDataReport(reportData);
	printf("[%s]scan data report scan file num : %d \n", GetCurrTimeStr().c_str(),m_cur_scanFileNum);
}

bool ScanFileDo::ReInitScanFrame()
{
	bool res = false;
	if (m_scanFrame!=0)
	{
		res = m_scanFrame->ReInit();
		if (res)
		{
			m_scanFrame->ReloadBase();
			if (m_realScan !=0)
			{
				m_realScan->ReInitScanEngine();
			}
		}
		else
		{
			printf("m_scanFrame->ReInit() failded\n");
		}
	}
	return res;
}

void ScanFileDo::ScanRecvThread(ScanFileDo*pRScanclient,int srNum)
{
	pRScanclient->DateReportScanAction(1);
	pRScanclient->SetSerScanStartTime();
	pRScanclient->CreateCacheReplyThread(CACHENUM);
	pRScanclient->SetScanStatus(0);
	sleep(3);
	pRScanclient->SetScanRecvThreadNum(srNum);
	for(int i=0;i<srNum;i++)
	{
		pthread_t tmp;
		pthread_attr_t a;
		pthread_attr_init(&a);
		pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
		pthread_create(&tmp, &a, CreateScanRecvThread,pRScanclient);
	}
		//todo cbx create real scan thread
	if (pRScanclient!=0)
	{
		pRScanclient->CreateRealScanThread();
		pRScanclient->CreateCheckScanTimeThread();
	}
}
void ScanFileDo::CreateCheckScanTimeThread()
{
	if (m_realScan!=0)
	{
		m_realScan->CreateCheckScanFileTime();
	}

}
void ScanFileDo::CreateRealScanThread()
{
	if (m_realScan!=0)
	{
		m_realScan->CreateScanThread();
	}
}

void* CreateScanEnumThread(LPVOID lParam)
{
	ScanFileDo* pclient= (ScanFileDo*)lParam;
	if (pclient != NULL)
	{
		pclient->EnumFile();
	}
	else
	{
		printf("ScanSendEnum thread lparam null\n");
	}
	return 0;

}

void* CreateScanSendThread(LPVOID lParam)
{
	ScanFileDo* pclient= (ScanFileDo*)lParam;
	if (pclient != NULL)
	{
		pclient->SendScanFile();
	}
	else
	{
		printf("ScanSend thread lparam null\n");
	}
	return 0;
}
void ScanFileDo::SetMaxCacheThreadNum(int num)
{
	if (m_cacheHash!=0)
	{
		m_cacheHash->SetMaxCacheThreadNum(num);
	}
}

void ScanFileDo::CreateCacheReuestThread(int tnum)
{
	if (m_cacheHash!=0)
	{
		m_cacheHash->CreateCacheHashReuestThread(tnum);
	}
}

void ScanFileDo::TestPrintfItemSize()
{
	pthread_mutex_lock(&m_hashFilelist_mutex);
	int i = m_hashFileList.size();
	pthread_mutex_unlock(&m_hashFilelist_mutex);
	printf("the the rest of total size is %d\n",i);
}
void ScanFileDo::AddFileNum(int fileNum)
{
	pthread_mutex_lock(&m_cur_fileNum_mutex);
	m_cur_scanFileNum = m_cur_scanFileNum+ fileNum;
	pthread_mutex_unlock(&m_cur_fileNum_mutex);
}

void ScanFileDo::CreateCacheBackThread(int tnum)
{
	if (m_cacheHash!=0)
	{
		m_cacheHash->CreateCacheHashBackThread(tnum);
	}
}
void ScanFileDo::CreateCacheReplyThread(int tnum)
{
	if (m_cacheHash!=0)
	{
		m_cacheHash->CreateCacheHashReplyThread(tnum);
	}
}
void ScanFileDo::ScanSendThread(ScanFileDo*pRScanclient,int ssendNum,std::list<std::string> folderlist)
{
	//int
	pRScanclient->SetCltScanStartTime();
	pRScanclient->SetEndFlag(false);
	pRScanclient->SetScanSendThreadNum(ssendNum);
	pRScanclient->SetScanSendInitFolder(folderlist);
	//pthread_t tmpenum;
	//pthread_create(&tmpenum, NULL, CreateScanEnumThread,pRScanclient);
	pRScanclient->SetMaxCacheThreadNum(CACHENUM);
	pRScanclient->CreateCacheReuestThread(1);
	pRScanclient->CreateCacheBackThread(CACHENUM);
	pRScanclient->SetSendFileNum(0);
	for(int i=0;i<ssendNum;i++)
	{
		pthread_t tmp;
		pthread_attr_t a;
		pthread_attr_init(&a);
		pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
		pthread_create(&tmp, &a, CreateScanSendThread,pRScanclient);
	}

}

CacheHash::CacheHash(void * mem,int len,ScanFileDo* scando):m_ptr(mem),m_len(len),m_scanFiledo(scando)
{
	m_prelist_mutex=PTHREAD_MUTEX_INITIALIZER;
	m_enum_flag = false;
	m_cacheThreadNum =0;
	m_cache_recvEnd = false;
	m_cacheThread_mutex= PTHREAD_MUTEX_INITIALIZER;
	m_hashfilemap_mutex=PTHREAD_MUTEX_INITIALIZER;
	m_cacheReplyNum=0;
	m_cacheBackNum =0;
	m_cachebackfilenum=0;

	m_threadActive_mutex = PTHREAD_MUTEX_INITIALIZER;
	m_folder_mutex = PTHREAD_MUTEX_INITIALIZER;
	printf("cacheHash addr:%p \n",m_ptr);
}
bool CacheHash::ActiveThreaddStats()
{
	bool res = false;
	//pthread_mutex_lock(&m_threadActive_mutex);
	std::map<unsigned long ,bool>::iterator it = m_threadActiveMap.begin();
	for(;it!=m_threadActiveMap.end();it++)
	{
		if (it->second)
		{
			res = true;
			break;
		}
	}
	//pthread_mutex_unlock(&m_threadActive_mutex);
	return res;
}
CacheHash::~CacheHash()
{

}

void CacheHash::InitLock()
{
	pthread_spinlock_t * splock;
	for (int i = 0; i < CACHENUM; i++)
	{
		splock = (pthread_spinlock_t*)((byte*)(m_ptr+i*CACHEITEM));
		pthread_spin_init(splock, PTHREAD_PROCESS_SHARED);
	}
	splock =(pthread_spinlock_t*)((byte*)(m_ptr+CACHETOTALLEN));
	pthread_spin_init(splock, PTHREAD_PROCESS_SHARED);
	printf("..............CacheHashInitLock completed................\n");
}

int CacheHash::GetNextBack()
{
	int pos = -1;
	pthread_spinlock_t * splock=0;
	int i = 0;
	for( ;i<CACHENUM;i++)
	{
		int status=-1;
		pos = i*CACHEITEM;
		splock = (pthread_spinlock_t*)((byte*)(m_ptr+pos));
		if (splock != 0)
		{
			WappLock tcs(splock);
			ReadData((byte*)m_ptr + pos + 4, &status, sizeof(status));
			if (status == 2)
			{
				int rcls = 4;
				WriteData((byte*)m_ptr + pos + 4, &rcls, sizeof(rcls));
				return pos;
			}
		}
	}
	pos =-1;
	return pos;

}

int CacheHash::GetNextEmpty()
{
	int pos = -1;
	pthread_spinlock_t * splock=0;
	int i = 0;
	for( ;i<CACHENUM;i++)
	{
		int status=-1;
		pos = i*CACHEITEM;
		splock = (pthread_spinlock_t*)((byte*)(m_ptr+pos));
		if (splock != 0)
		{
			WappLock tcs(splock);
			ReadData((byte*)m_ptr + pos + 4, &status, sizeof(status));
			if (status ==0|| status ==3)
			{
				int rcls = 4;
				WriteData((byte*)m_ptr + pos + 4, &rcls, sizeof(rcls));
				return pos;
			}
		}
	}
	pos =-1;
	return pos;
}
int CacheHash::GetNextFull()
{
	pthread_spinlock_t * splock=0;
	int pos = -1;
	int i = 0;
	for (; i < CACHENUM; i++)
	{
		pos = i*CACHEITEM;
		splock = (pthread_spinlock_t*)((byte*)(m_ptr+pos));
		if (splock != 0)
		{
			int clstatus = -1;
			WappLock tcs(splock);
			ReadData((byte*)m_ptr + pos + 4, &clstatus, sizeof(clstatus));
			if (clstatus == 1)
			{
				int rcls = 4;
				WriteData((byte*)m_ptr + pos + 4, &rcls, sizeof(rcls));
				return pos;
			}
		}
	}
	pos =-1;
	return -1;
}

void ScanFileDo::SetScanSendInitFolder(std::list<std::string> folders)
{
	//m_folder_list.push_back(folder);
	if (m_cacheHash!=0)
	{
		m_cacheHash->PushEnumFolder(folders);
	}
}

int ScanFileDo::QueryServerCache(unsigned int fhash)
{
	int res = 1;
	if (m_fileCache == 0)
	{
		return res;
	}
	bool vres = false;
    bool bres =  m_fileCache->QueryCache(fhash,&vres);
    if (bres)
    {
    	if (!vres)
    	{
    		res =0;
    	}
    	else
    	{
    		res =  2;
    	}
    }
	return res;
}

void CacheHash::WriteCacheEndFlag(int flag)
{
	int eflag = flag;
	pthread_spinlock_t * splock;
	splock =(pthread_spinlock_t*)((byte*)(m_ptr+CACHETOTALLEN));
	if (splock!=0)
	{
		WappLock tcs(splock);
		WriteData((byte*)m_ptr+CACHETOTALLEN+4,&eflag,sizeof(eflag));
	}
}
int CacheHash::GetCacheEndFlag()
{
	int eflag = 0;
	pthread_spinlock_t * splock;
	splock =(pthread_spinlock_t*)((byte*)(m_ptr+CACHETOTALLEN));
	if (splock!=0)
	{
		WappLock tcs(splock);
		ReadData((byte*)m_ptr+CACHETOTALLEN+4,&eflag,sizeof(eflag));
	}
    return eflag;
}

void CacheHash::TiggerThreadExit()
{
	int eflag = CACHEENDFLAG;
	WriteCacheEndFlag(eflag);
	//eflag = 0;
	//WriteCacheEndFlag(eflag);
}
//client function
void CacheHash::CacheRequestBackThread()
{
	int sleeptime=0;
	pthread_spinlock_t * splock=0;
	int backnum=0;
	int virnum = 0;
	int whitenum = 0;
	int allback =0;
	unsigned long tid = pthread_self();
	printf("[%lu]%s]CacheRequestBackThread start\n",tid,GetCurrTimeStr().c_str());
	while(true)
	{
		int pos = GetNextBack();
		if (pos !=-1)
		{
			int hashres=0;
			unsigned int hash = 0;
			int status = 3;
			allback++;
			splock = (pthread_spinlock_t * )(byte*)(m_ptr+pos);
			if (splock)
			{
				{
					//write the status to 3.
					WappLock tcs(splock);
					ReadData((byte*)m_ptr + pos + 4+4+4, &hashres, sizeof(hashres));
					ReadData((byte*)m_ptr + pos + 4+4, &hash, sizeof(hash));
					WriteData((byte*)m_ptr+ pos +4,&status,sizeof(status));
				}
				if (hashres!=0)
				{
					//need cp file context to the shmem.
					//hash not find in the cache.
					Hash_FileItem item;
					pthread_mutex_lock(&m_hashfilemap_mutex);
					std::map<unsigned int,std::string>::iterator it = m_hashfilemap.find(hash);
					if (it!=m_hashfilemap.end())
					{
						item.fhash = hash;
						item.filename= it->second;
						m_hashfilemap.erase(it);
						pthread_mutex_unlock(&m_hashfilemap_mutex);
						m_scanFiledo->PushHashFileItem(item);
						backnum++;
					}
					else
					{
						pthread_mutex_unlock(&m_hashfilemap_mutex);
					}
					if (hashres ==2)
					{
						//vir
						virnum++;
						m_scanFiledo->AddCache(hash,true);
					}
				}
				else
				{
					//white
					whitenum++;
					m_scanFiledo->AddCache(hash,false);
				}
			}
			else
			{
				WriteData((byte*)m_ptr+ pos +4,&status,sizeof(status));
			}
			sleeptime=0;
		}
		else
		{
			int eflag = GetCacheEndFlag();
			//printf("eflag %d,CACHEENDFLAG %d num %d\n",eflag,CACHEENDFLAG,m_cacheReplyNum);
			if (eflag == CACHEBACKENDFLAG)
			{
				break;
			}
			sleep(1);
			sleeptime++;
			if (sleeptime>300)
			{
				sleeptime=0;
				printf("[%lu][%s] CacheRequestBackThread sleep 300 s not empty to cp hash\n",tid,GetCurrTimeStr().c_str());
			}
		}

	}
	//the lash cache thread exit .need notify the server.
	printf("[%s]............... CacheRequestBackThread exit ...............allback %d ,backreal %d,virnum %d,whitenum %d\n",
			GetCurrTimeStr().c_str(),allback,backnum, virnum,whitenum);

	pthread_mutex_lock(&m_cacheThread_mutex);
	m_cacheBackNum--;
	m_cachebackfilenum=m_cachebackfilenum+backnum;
	if (m_cacheBackNum==0)
	{
		pthread_mutex_unlock(&m_cacheThread_mutex);
		printf("[%s] the last CacheRequestBackThread exit total backnum %d\n",
				GetCurrTimeStr().c_str(),m_cachebackfilenum);
		int eflag = 0;
		m_cache_recvEnd = true;
		WriteCacheEndFlag(eflag);
		m_scanFiledo->TestPrintfItemSize();
		m_scanFiledo->SetEndFlag(true);
	}
	else
	{
		pthread_mutex_unlock(&m_cacheThread_mutex);
	}


}

void CacheHash::NodifySerCacheRec()
{
	pthread_mutex_lock(&m_cacheThread_mutex);
	m_cacheThreadNum--;
	if (m_cacheThreadNum==0)
	{
		pthread_mutex_unlock(&m_cacheThread_mutex);
		printf("[%s] the last request thread exit \n",GetCurrTimeStr().c_str());
		int eflag = CACHEENDFLAG;
		WriteCacheEndFlag(eflag);
	}
	else
	{
		pthread_mutex_unlock(&m_cacheThread_mutex);
	}
}

bool CacheHash::CreateRequestFlag()
{
	bool res = false;
	pthread_mutex_lock(&m_prelist_mutex);
	if (m_prelist.empty() && m_enum_flag)
	{
		res = true;
	}
	pthread_mutex_unlock(&m_prelist_mutex);
	return res;
}

//client compute the hash .and request the hash res.
void CacheHash::CacheRequestThread()
{
	int sleeptime=0;
	int sendnum=0;
	int realsend =0;
	int virfind=0;
	unsigned long tid = pthread_self();
	pthread_mutex_lock(&m_threadActive_mutex);
	m_threadActiveMap[tid]=true;
	printf("[%s]m_enumfolderlist.size %d\n",GetCurrTimeStr().c_str(),m_enumfolderlist.size());
	pthread_mutex_unlock(&m_threadActive_mutex);
	while(true)
	{
		std::list<std::string> tmpfilelist;
		pthread_mutex_lock(&m_threadActive_mutex);
		if (!m_enumfolderlist.empty())
		{
			m_threadActiveMap[tid]=true;
			std::string curfolder = m_enumfolderlist.front();
			m_enumfolderlist.pop_front();
			GetFolderFile(curfolder, tmpfilelist,m_enumfolderlist);
			//if (m_enumfolderlist.size()>)
			int inum = m_max_cacheThreadNum-m_createdCacheThNum;
			int mnum = m_enumfolderlist.size();
			if ((m_createdCacheThNum < m_max_cacheThreadNum )&& (mnum>0))
			{
				if (inum>mnum)
				{
					inum = mnum;
				}
				CreateCacheHashReuestThread(inum);
			}
			pthread_mutex_unlock(&m_threadActive_mutex);
		}
		else
		{
			m_threadActiveMap[tid]=false;
			if (!ActiveThreaddStats())
			{
				pthread_mutex_unlock(&m_threadActive_mutex);
				break;
			}
			else
			{
				pthread_mutex_unlock(&m_threadActive_mutex);
				sleep(2);
				continue;
			}
		}
		//toco cbx local hash cache find.
		//std::map<unsigned int ,std::string> tmpHaMap;
		while(!tmpfilelist.empty())
		{
			std::string curpath = tmpfilelist.front();
			tmpfilelist.pop_front();
			sendnum ++;
			bool gres = false;
			unsigned int fhash = GetFileHash(curpath,&gres);
			if (gres)
			{
				int fres = m_scanFiledo->QueryServerCache(fhash);
				if (fres ==1)//cache nof find
				{
					//tmpHaMap[fhash]=curpath;
					int pos = GetNextEmpty();
					sleeptime=0;
					while(pos ==-1)
					{
						sleep(1);
						sleeptime++;
						if (sleeptime>300)
						{
							sleeptime=0;
							printf("[%lu][%s] CacheRequestThread sleep 300 s not empty to cp hash\n",tid,GetCurrTimeStr().c_str());
						}
						pos = GetNextEmpty();
					}
					pthread_spinlock_t * splock=0;
					splock = (pthread_spinlock_t * )(byte*)(m_ptr+pos);
					if (splock)
					{
						pthread_mutex_lock(&m_hashfilemap_mutex);
						m_hashfilemap[fhash]=curpath;
						pthread_mutex_unlock(&m_hashfilemap_mutex);
						int status = 1;
						realsend++;
						WappLock tcs(splock);
						int wslen = WriteData((byte*)m_ptr + pos + 4, &status, sizeof(status));
						WriteData((byte*)m_ptr + pos + 4+wslen, &fhash, sizeof(fhash));
					}
					else
					{
						int status=0;
						int wslen = WriteData((byte*)m_ptr + pos + 4, &status, sizeof(status));
					}
				}
				else if (fres ==2)//vir direct cp to shmem
				{
					Hash_FileItem item;
					item.fhash = fhash;
					item.filename= curpath;
					virfind++;
					m_scanFiledo->PushHashFileItem(item);
				}
			}
		}
	}
	//the lash cache thread exit .need notify the server.
	printf("[%s]............... CacheRequestThread exit ...............filenum %d ,realsend %d virfind %d\n",
			GetCurrTimeStr().c_str(),sendnum,realsend,virfind);

	pthread_mutex_lock(&m_cacheThread_mutex);
	m_cacheActiveNum--;
	m_fileTotal = m_fileTotal + sendnum;
	if (m_cacheActiveNum==0)
	{
		pthread_mutex_unlock(&m_cacheThread_mutex);
		printf("[%s] the last request thread exit  fileTotal %d \n",GetCurrTimeStr().c_str(), m_fileTotal);
		int eflag = CACHEENDFLAG;
		WriteCacheEndFlag(eflag);
		//m_scanFiledo->SetEndFlag(true);
	}
	else
	{
		pthread_mutex_unlock(&m_cacheThread_mutex);
	}
}
//server function
void CacheHash::CacheReplyThread()
{
	int sleeptime=0;
	unsigned long tid = pthread_self();
	while(true)
	{
		int pos = GetNextFull();
		if (pos != -1)
		{
			//read hash
			//query the scancache.
			//write the hash res
			//set status to 2.
			sleeptime=0;
			pthread_spinlock_t * splock=0;
			splock = (pthread_spinlock_t * )(byte*)(m_ptr+pos);
			if (splock)
			{
				int status = 2;
				unsigned int fhash=0;
				{
					WappLock tcs(splock);
					ReadData((byte*)m_ptr + pos + 4+4, &fhash, sizeof(fhash));
				}
				int res = m_scanFiledo->QueryServerCache(fhash);
				{

					WappLock tcs(splock);
					WriteData((byte*)m_ptr + pos + 4, &status, sizeof(status));
					WriteData((byte*)m_ptr + pos + 4+4+4, &res, sizeof(res));
				}
			}
		}
		else
		{
		   //todo all cache thread exit?
			int eflag = GetCacheEndFlag();
			//printf("eflag %d,CACHEENDFLAG %d num %d\n",eflag,CACHEENDFLAG,m_cacheReplyNum);
			if (eflag == CACHEENDFLAG)
			{
				break;
			}
			sleep(1);
			sleeptime++;
			if (sleeptime>300)
			{
				sleeptime = 0;
				printf("[%lu][%s]CacheReplyThread sleep 300 s not full item\n",tid,GetCurrTimeStr().c_str());
			}
		}
	}
	pthread_mutex_lock(&m_cacheThread_mutex);
	m_cacheReplyNum--;
	if (m_cacheReplyNum==0)
	{
		pthread_mutex_unlock(&m_cacheThread_mutex);
		//lash reply end set the true
		printf("[%s] the last CacheReplyThread exit \n",GetCurrTimeStr().c_str());
		m_scanFiledo->SetEndFlag(true);
		int bflag = CACHEBACKENDFLAG;
		WriteCacheEndFlag(bflag);
	}
	else
	{
		pthread_mutex_unlock(&m_cacheThread_mutex);
	}

}

IPMonitor::IPMonitor(void *mem,int len,bool ser):m_ptr(mem),m_len(len),m_flag(ser)
{
	m_cmdSer=0;
	if (m_flag)
	{
		printf("IPMonitor <<<m_len %d,IPTOTAL %d>>>\n",m_len,IPTOTAL);
		InitLock();
	}

}
IPMonitor::~IPMonitor()
{
}

void IPMonitor::InitLock()
{
	pthread_spinlock_t * splock;
	for (int i = 0; i < IPITEMNUM; i++)
	{
		splock = (pthread_spinlock_t*)((byte*)(m_ptr+i*IPITEM));
		pthread_spin_init(splock, PTHREAD_PROCESS_SHARED);
	}
}

int IPMonitor::GetNextFullItem()
{
	pthread_spinlock_t * splock=0;
	for (int i = 0; i < IPITEMNUM; i++)
	{
		int pos = i*IPITEM;
		splock = (pthread_spinlock_t*)((byte*)(m_ptr+pos));
		if (splock != 0)
		{
			int clstatus = -1;
			WappLock tcs(splock);
			ReadData((byte*)m_ptr + pos + 4, &clstatus, sizeof(clstatus));
			if (clstatus == 1)
			{
				int rcls = 4;
				WriteData((byte*)m_ptr + pos + 4, &rcls, sizeof(rcls));
				return pos;
			}
		}
	}
	return -1;
}
int IPMonitor::GetNextEmptyItem()
{
	pthread_spinlock_t * splock=0;
	for (int i = 0; i < IPITEMNUM; i++)
	{
		int pos = i*IPITEM;
		splock = (pthread_spinlock_t*)((byte*)(m_ptr+pos));
		if (splock != 0)
		{
			int clstatus = -1;
			WappLock tcs(splock);
			ReadData((byte*)m_ptr + pos + 4, &clstatus, sizeof(clstatus));
			if (clstatus == 0 || clstatus == 2)
			{
				int rcls = 4;
				WriteData((byte*)m_ptr + pos + 4, &rcls, sizeof(rcls));
				return pos;
			}
		}
	}
	return -1;
}


void IPMonitor::StartClientGetIpThread()
{
	pthread_t tmp;
	pthread_attr_t a;
	pthread_attr_init(&a);
	pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
	pthread_create(&tmp, &a, CreateClientGetIPThread,this);
	printf("StartClientGetIpThread\n");

}

void IPMonitor::GetIPAndSendToServer()
{
	bool only127=false;
	std::string ip = GetIPAddress(only127);
	while(true)
	{
		int pos = GetNextEmptyItem();
		if(pos !=-1)
		{
			pthread_spinlock_t * splock=0;
			splock = (pthread_spinlock_t*)((byte*)(m_ptr+pos));
			if (splock!=0)
			{
				int rcls = 1;
				int uuidlen = m_uuid.length();
				int iplen = ip.length();
				WappLock tcs(splock);
				int wstatelen = WriteData((byte*)m_ptr + pos + 4, &rcls, sizeof(rcls));
				int wuidlen = WriteData((byte*)m_ptr + pos + 4+wstatelen, &uuidlen, sizeof(uuidlen));
				int wuid = WriteData((byte*)m_ptr + pos + 4+wstatelen+wuidlen, (void*)m_uuid.c_str(), uuidlen);
				int wiplen = WriteData((byte*)m_ptr + pos + 4+wstatelen+wuidlen+wuid, &iplen, sizeof(iplen));
				wiplen = WriteData((byte*)m_ptr + pos + 4+wstatelen+wuidlen+wuid+wiplen,(void*)ip.c_str(), iplen);
			}
			break;
		}
		else
		{
			sleep(1);
		}
	}
}

void IPMonitor::DataReport(std::string uuid, std::string ip)
{
	if (m_cmdSer!=0)
	{
		m_cmdSer->DataReport(uuid,ip);
	}
}

void IPMonitor::RecvTheIpData()
{
	while(true)
	{
		int pos =GetNextFullItem();
		if (pos !=-1)
		{
			pthread_spinlock_t * splock=0;
			splock = (pthread_spinlock_t*)((byte*)(m_ptr+pos));
			if (splock!=0)
			{
				int ulen =0;
				int iplen = 0;
				int state = 2;
				char bufferuuid[40]={0};
				char bufferip[64]={0};
				WappLock tcs(splock);
				int rlen = ReadData((byte*)m_ptr+pos+8,&ulen,sizeof(ulen));
				if (ulen >0 && ulen <40)
				{
					int rulen = ReadData((byte*)m_ptr+pos+8+rlen,(void*)bufferuuid,ulen);
					int riplen =  ReadData((byte*)m_ptr+pos+8+rlen+ulen,&iplen,sizeof(iplen));
					if (iplen>0 && iplen <64)
					{
						riplen =  ReadData((byte*)m_ptr+pos+8+rlen+ulen+riplen,(void*)bufferip,iplen);
					}
					else
					{
						printf("read ip send ip len error:%d\n",iplen);
					}
				}
				else
				{
					printf("read ip send uuid len error:%d\n",ulen);
				}
				rlen = WriteData((byte*)m_ptr+pos+4,&state,sizeof(state));
				printf("recv the client uuid:%s ip:%s\n",bufferuuid,bufferip);
				std::string uuid;
				std::string ip;
				uuid.append(bufferuuid);
				ip.append(bufferip);
				DataReport(uuid,ip);
			}
		}
		else
		{
			sleep(30);
		}
	}

}
void IPMonitor::ServerMonitTheIPData()
{
	pthread_t tmp;
	pthread_attr_t a;
	pthread_attr_init(&a);
	pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
	pthread_create(&tmp, &a, CreateServerRecvIPThread,this);
	printf("ServerMonitTheIPData\n");
}

void* CreateClientGetIPThread(LPVOID lParam)
{
	IPMonitor *p=(IPMonitor*)lParam;
	if (p!=0)
	{
		p->GetIPAndSendToServer();
	}
	else
	{
		printf("CreateClientGetIPThread param error\n");
	}

}
void* CreateServerRecvIPThread(LPVOID lParam)
{
	IPMonitor *p=(IPMonitor*)lParam;
	if (p!=0)
	{
		p->RecvTheIpData();
	}
	else
	{
		printf("CreateServerRecvIPThread param error\n");
	}
}

void PrintfLockValue(void* splock,char *str)
{
#if 0
	int lockNo = (byte*)splock-(byte*)G_LOCK_START;
	int flag = lockNo - 29363156;
	if (flag >=0 && flag % CACHEITEM==0)
	{
		int lockValue=38;
		memcpy((void*)&lockValue,(void*)splock,sizeof(lockValue));
		printf("lockNo:%d lockValue:%d,%s\n",lockNo,lockValue,str);
	}
#endif
}

WappLock::WappLock(pthread_spinlock_t * splock)
{
	m_splock=splock;
	PrintfLockValue((void*)m_splock,"lockbefore");
	pthread_spin_lock(m_splock);
	PrintfLockValue((void*)m_splock,"lockbafter");
}
WappLock::~WappLock()
{
	PrintfLockValue((void*)m_splock,"unlockbefore");
	pthread_spin_unlock(m_splock);
	PrintfLockValue((void*)m_splock,"unlockafter");
}


HeartService::HeartService(void *mem,int len,bool ser):m_ptr(mem),m_len(len)
{
	m_online_mutex=PTHREAD_MUTEX_INITIALIZER;
	if (ser)
	{
		InitLock();
	}
	int totalLen = (byte*)m_ptr-(byte*)G_LOCK_START+len;
	int fM = 1024*1024*32;
	char buf[128]={0};
	sprintf(buf,"<<total %d fM %d>>",totalLen,fM);
	ZyWritelogByVSecure("INFO",buf);
}
HeartService::~HeartService()
{
}

void* CreateClientMonitorHeartThread(LPVOID lParam)
{
	HeartService *p=(HeartService*)lParam;
	if (p!=0)
	{
		p->ClientMonitorHeart();
	}
	else
	{
		printf("CreateClientMonitorHeartThread param error\n");
	}
	return 0;
}

void* CreateSerMonitorHeartThread(LPVOID lParam)
{
	HeartService *p=(HeartService*)lParam;
	if (p!=0)
	{
		p->SerHeartDone();
	}
	else
	{
		printf("CreateSerMonitorHeartThread param error\n");
	}
	return 0;
}

void HeartService::StartSerMonitorHeart()
{
	pthread_t tmp;
	pthread_attr_t a;
	pthread_attr_init(&a);
	pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
	pthread_create(&tmp, &a, CreateSerMonitorHeartThread,this);
	printf("StartSerMonitorHeart\n");

}

void HeartService::StartClientMonitorHeart()
{
	pthread_t tmp;
	pthread_attr_t a;
	pthread_attr_init(&a);
	pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
	pthread_create(&tmp, &a, CreateClientMonitorHeartThread,this);
	printf("StartClientMonitorHeart\n");

}
bool HeartService::GetVirtualState(std::string uuid)
{
	bool res = true;
	pthread_spinlock_t * splock;
	splock = (pthread_spinlock_t*)((byte*)m_ptr);
	int value = 0;
	if (splock)
	{
		int value = 1;
		int ulen = uuid.length();
		{
			WappLock tcs(splock);
			int rlen = WriteData((byte*)m_ptr+HEARTLOCKLEN,&value,sizeof(value));
			int uulenr = WriteData((byte*)m_ptr+HEARTLOCKLEN+HEARTCMDSTATUS,&ulen,sizeof(ulen));
			int rulen = WriteData((byte*)m_ptr+HEARTLOCKLEN+HEARTCMDSTATUS+HEARTUUIDLEN,(void*)uuid.c_str(),ulen);
		}
		int rtime= 0;
		while(true)
		{
			{
				value = 0;
				WappLock tcs(splock);
				int rlen = ReadData((byte*)m_ptr+HEARTLOCKLEN,&value,sizeof(value));
			}
			if (value == 2)
			{
				res = true;
				break;
			}
			else
			{
				sleep(3);
				rtime ++;
			}
			if (rtime>10)
			{
				res = false;
				printf("uuid %s close ...........\n",uuid.c_str());
				break;
			}
		}
	}
	return res;
}

void HeartService::ClientMonitorHeart()
{
	pthread_spinlock_t * splock;
	splock = (pthread_spinlock_t*)((byte*)m_ptr);
	while(true)
	{
		int value = 0;
		char bufferuuid[40]={0};
		char bufferip[64]={0};
		if (splock)
		{
			WappLock tcs(splock);
			int rlen = ReadData((byte*)m_ptr+HEARTLOCKLEN,&value,sizeof(value));

		}
		if (value ==1)
		{
			int ulen = 0;
			{
				WappLock tcs(splock);
				int rlen = ReadData((byte*)m_ptr+HEARTLOCKLEN+HEARTCMDSTATUS,&ulen,sizeof(ulen));
			}
			if (ulen >0 && ulen <40)
			{
				{
					WappLock tcs(splock);

					int rulen = ReadData((byte*)m_ptr+HEARTLOCKLEN+HEARTCMDSTATUS+HEARTUUIDLEN,(void*)bufferuuid,ulen);
				}
			    if (m_uuid.compare(bufferuuid)==0)
			    {
			    	WappLock tcs(splock);
			    	int state = 2;
			    	WriteData((byte*)m_ptr+HEARTLOCKLEN,&state,sizeof(state));
			    }
			}
			else
			{
				printf("read uuid len error:%d\n",ulen);
			}
		}

		sleep(1);
	}

}

void HeartService::PushUUId(std::string uuid)
{
	sleep(2);
	pthread_mutex_lock(&m_online_mutex);
	m_onlinelist.push_back(uuid);
	pthread_mutex_unlock(&m_online_mutex);
}

void HeartService::SerHeartDone()
{
	printf("comme .. serheartDone\n");
	while(true)
	{
		pthread_mutex_lock(&m_online_mutex);
		if (m_onlinelist.size()==0)
		{
			pthread_mutex_unlock(&m_online_mutex);
			sleep(1);
			continue;
		}
		//printf("list not null\n");
		std::string uuid = m_onlinelist.front();
		m_onlinelist.pop_front();
		pthread_mutex_unlock(&m_online_mutex);
		bool state = GetVirtualState(uuid);
		if (state)
		{
			pthread_mutex_lock(&m_online_mutex);
			m_onlinelist.push_back(uuid);
			pthread_mutex_unlock(&m_online_mutex);
		}
		else
		{
			std::string curuuid= m_cmdSer->GetCurUUID();
			//printf("uuid %s,curuuid,%s\n",uuid.c_str(),curuuid.c_str());
			if (uuid.compare(curuuid)==0)
			{
				m_cmdSer->TiggerThreadExit(m_cmdSer->GetCurCMD());
			}
			m_cmdSer->DataReportCloseUuid(uuid);
		}
	}
}


void HeartService::InitLock()
{
	pthread_spinlock_t * splock;
	splock = (pthread_spinlock_t*)((byte*)m_ptr);
	pthread_spin_init(splock, PTHREAD_PROCESS_SHARED);
}
