/*
 * RegisterHost.cpp
 *
 *  Created on: Apr 26, 2019
 *      Author: mark
 */

#include "RegisterHost.h"
#include "unit.h"
#include "proto/clientActionV2.pb.h"
#include "LogOutput.h"

#define   FIRST_RUN_FILE   "firstrun"

RegisterHost::RegisterHost()
{
	m_configFile=0;
	m_dataReport=0;
	m_clientM=0;
}
RegisterHost::~RegisterHost()
{

}

void RegisterHost::UninstallReport()
{
	if (m_dataReport && m_configFile)
	{
		m_dataReport->SetDataUrl(m_configFile->GetDataUrl());
	}
	int cpuCores=GetCpuNum();
	char buffercpucores[8]={0};
	sprintf(buffercpucores,"%d",cpuCores);
	std::string context;
	std::string acon;
	ClientAction cation;
	cation.set_action(ClientAction_Action_CLIENT_UNINSTALL);
	cation.set_uuid(buffercpucores);
	cation.set_action_state(ClientAction_ActionState_SUCCESS);
	cation.set_action_trigger_type(ClientAction_ActionTriggerType_MANUAL);
	cation.set_time(time(NULL));
	cation.SerializeToString(&acon);
	ClientActionRequest acrequest;
	acrequest.set_client_id(m_hostId);
	acrequest.set_action_type(ClientActionRequest_ActionType_CLIENT_ACTION);
	acrequest.set_action_data(acon);
	acrequest.SerializePartialToString(&context);
	m_dataReport->SendDataReport(context);
	char buffer[64]={0};
	sprintf(buffer,"Uninstall cores :%d,hostid: %s\n",cpuCores,m_hostId.c_str());
	ZyWritelogByVSecure("INFO",buffer);
}

void RegisterHost::RegisterCores()
{
	int cpuCores=GetCpuNum();
	ClientActionRequest clrequest;
	clrequest.set_client_id(m_hostId);
	clrequest.set_action_type(ClientActionRequest::ActionType::ClientActionRequest_ActionType_REGISTER);
	VirtualGroupInfo vinfo;
	vinfo.set_cores(cpuCores);
	std::string strinfo;
	vinfo.SerializeToString(&strinfo);
	clrequest.set_action_data(strinfo);
	std::string rstr;
	clrequest.SerializeToString(&rstr);
	m_dataReport->SendDataReport(rstr);
	char buffer[64]={0};
	sprintf(buffer,"cores :%d,hostid: %s\n",cpuCores,m_hostId.c_str());
	ZyWritelogByVSecure("INFO",buffer);
	int reposetime  =0;
	int regflagtime = 0;
	while(true)
	{
		bool regflag=false;
		bool respose = false;
		regflag = m_dataReport->GetRegisterFlag(respose);

		if (!respose)
		{
			reposetime++;
			sleep(3);
			if (reposetime==3)
			{
				//printf("[%s] reposetime == 3 ser not respose, wait 3s\n",GetCurrTimeStr().c_str());
				ZyWritelogByVSecure("INFO","reposetime == 3 ser not respose, wait 3s");
			}
			continue;
		}

		if (!regflag)
		{
			regflagtime++;
			if (regflagtime==3)
			{
				//printf("regflagtime ==3.ser register fail ,maybe the cores is max. contine re register\n");
				ZyWritelogByVSecure("INFO","regflagtime ==3.ser register fail ,maybe the cores is max. contine re register");
			}
			m_dataReport->SendDataReport(rstr);
			sleep(1);
		}
		else
		{
			//printf("ser register ok\n");
			ZyWritelogByVSecure("INFO","RegisterCores ser register ok");
			break;
		}
	}

}

void RegisterHost::InitClientMontior(std::string xmlpath)
{
	if (m_clientM==0)
	{
		m_clientM = new ClientMonitor(xmlpath);
	}
#if 0
	if (m_clientM!=0)
	{
		m_clientM->TestPrintItem();
	}
#endif
}

void RegisterHost::StartClientMontior(SHMEMServer* ser)
{
	if (m_clientM!=0)
	{
		m_clientM->SetSMServer(ser);
		m_clientM->StartThreadMonitorMacChange(m_clientM);
	}
}

void RegisterHost::VirtualGroupSend()
{
	std::vector<VirtualInfo> allItem;
	if (m_clientM!=0)
	{
		m_clientM->GetVirtualGroup(allItem);
	}
	else
	{
		//printf("must init the clientMonitor !!!\n");
		ZyWritelogByVSecure("ERROR","Must init the clientMonitor !!!");
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
	//printf(" basein")
	BaseInfo_IpMac *iM = baseInfo.add_ip_mac();
	std::string hmac = m_hostId;
	bool only127=false;
	std::string ip=GetIPAddress(only127);
	iM->set_host_ip(ip);
	iM->set_host_mac(hmac);
	std::string content;

	std::vector<VirtualInfo>::iterator it = allItem.begin();
	for(;it!=allItem.end();it++)
	{
		VirtualItem *vitem = baseInfo.add_vgroup();
		vitem->set_name(it->name);
		vitem->set_uuid(it->uuid);
		vitem->set_ostype(it->ostype);
	}
	baseInfo.SerializePartialToString(&content);
	ClientActionRequest clientRequest;
	clientRequest.set_client_id(m_hostId);
	clientRequest.set_action_type(ClientActionRequest_ActionType_REPORTED_BASE_INFO);
	clientRequest.set_action_data(content);
	std::string reportData;
	clientRequest.SerializePartialToString(&reportData);

	if(m_dataReport!=0)
	{
		m_dataReport->SetDataUrl(m_configFile->GetDataUrl());
		m_dataReport->SendDataReport(reportData);
	}
	//printf("[%s]  init virtual group data report %s\n", GetCurrTimeStr().c_str(),m_hostId.c_str());
	char buffer[128]={0};
	sprintf(buffer,"%s init virtual group data report.item num:%d",m_hostId.c_str(),allItem.size());
	ZyWritelogByVSecure("INFO",buffer);
}

void RegisterHost::Init()
{
	std::string frfile=GetCurrPath();
	frfile = frfile + "/" +FIRST_RUN_FILE;

	//printf("frunpath:%s\n",frfile.c_str());
	char buf[64]={0};
	sprintf(buf,"frunpath:%s",frfile.c_str());
	ZyWritelogByVSecure("INFO",buf,true);

	FILE *frhandle = fopen(frfile.c_str(), "r");
	if (frhandle == 0)
	{
		frhandle = fopen(frfile.c_str(),"w");
		if (frhandle==0)
		{
			//printf("create the %s error\n",frfile.c_str());
			ZyWritelogByVSecure("INFO","create the frunpath error.",true);
			return ;
		}
		std::vector<std::string> macVec;
		int mnum = GetMacList(macVec);
		if (mnum>=1)
		{
			m_hostId = macVec[0];
			//printf("hostid %s ,len:%d\n",macVec[0].c_str(),macVec[0].length());
			int wlen = fwrite((void*)macVec[0].c_str(),1,macVec[0].length(),frhandle);
			//printf("hostid %s ,len:%d, wlen %d\n",macVec[0].c_str(),macVec[0].length(),wlen);
			char buffer[128]={0};
			sprintf(buffer,"hostid %s ,len:%d, wlen %d\n",macVec[0].c_str(),macVec[0].length(),wlen);
			ZyWritelogByVSecure("INFO",buffer,true);
			fclose(frhandle);
		}
		else
		{
			//printf("oo the mac not cz.or get the mac error.exit\n");
			ZyWritelogByVSecure("ERROR","oo the mac not cz.or get the mac error.exit");
			exit(-1);
		}

	}
	else
	{
		char macBuffer[64]={0};
		int mlen = fread(macBuffer,1,64,frhandle);
		//printf("read the hostid %s ,len %d\n",macBuffer, mlen);
		char buf[128]={0};
		sprintf(buf,"read the hostid %s ,len %d\n",macBuffer, mlen);
		ZyWritelogByVSecure("INFO",buf,true);
		m_hostId.append(macBuffer);
		fclose(frhandle);
	}

	if (m_configFile==0)
	{
		m_configFile = new ConfigFileService();
	}
	if (m_configFile!=0)
	{
		m_configFile->Init();
		if (m_dataReport==0)
		{
			m_dataReport = new DataReportService(m_configFile->GetRegisterUrl());
			if (m_dataReport!=0)
			{
				m_dataReport->Init();
			}
		}
	}




}


