/*
 * clientMonitor.cpp
 *
 *  Created on: Mar 29, 2019
 *      Author: mark
 */
#include "clientMonitor.h"
#include "streamControl.h"
#include "unit.h"
#include "stdio.h"
#include "string.h"
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include "LogOutput.h"
#define MACKEY  		"<mac address="
#define UUIDSTART 		"<uuid>"
#define UUIDEND    		"</uuid>"
#define CXSYSTEMBEGIN 	"<system>"
#define CXSYSTEMEND  	"</system>"
#define CXNAMEB         "<name>"
#define CXNAMED         "</name>"

ClientMonitor::ClientMonitor(std::string xmlpath, SHMEMServer *shServer):m_xmlPath(xmlpath),m_shServer(shServer)
{
	//InitHostMac();
	InitUuidMac();
}
ClientMonitor::ClientMonitor(std::string xmlpath):m_xmlPath(xmlpath)
{
	m_shServer=0;
	InitUuidMac();
}
ClientMonitor::~ClientMonitor()
{
}

void ClientMonitor::InitHostMac()
{
	std::vector<std::string> macvec;
	int macnum = GetMacList(macvec);
	if (macnum>0)
	{

		m_hostMac=macvec[0];
	}
	else
	{
		m_hostMac="xx:xx:xx:xx:xx:xx";
	}
	//printf("%d NUM HostMac: %s\n",macnum,m_hostMac.c_str());
	char buf[128]={0};
	sprintf(buf,"%d NUM HostMac:[0] %s\n",macnum,m_hostMac.c_str());
	ZyWritelogByVSecure("INFO",buf);
}

void ClientMonitor::ReadFileList()
{
    DIR *dir;
    struct dirent *ptr;
    if ((dir=opendir(m_xmlPath.c_str())) == NULL)
    {
        //printf("Opendir failed %s\n", m_xmlPath.c_str());
    	char buf[128]={0};
    	sprintf(buf,"opendir failed %s ",m_xmlPath.c_str());
    	ZyWritelogByVSecure("ERROR",buf);
        return ;
    }
    while ((ptr=readdir(dir)) != NULL)
    {
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    ///current dir OR parrent dir
            continue;
        else if(ptr->d_type == 8 && endXML(ptr->d_name)==1)    ///file
        {
            int blen = strlen(ptr->d_name);
            std::string tmp;
            tmp.assign(ptr->d_name,ptr->d_name+blen);
            std::string fpath=m_xmlPath;
            fpath=fpath+"/"+tmp;
            long int ftime = GetFileModfiyTime((char*)fpath.c_str());
            m_mapFileChangeTime[tmp]=ftime;
            m_pathVec.push_back(tmp);
        }
        else if(ptr->d_type == 10)    ///link file
        {
        }
        else if(ptr->d_type == 4)    ///dir
        {
        }
    }
    closedir(dir);
    return ;
}

void ClientMonitor::AddUuidMac(std::string uuid,std::string mac,std::string name,std::string osType,bool aorm)
{
	if (m_shServer!=0)
	{
		m_shServer->AddUuidMac(uuid,mac,name,osType,aorm);
	}
}

void ClientMonitor::DelUuidMac(std::string uuid)
{
	if (m_shServer!=0)
	{
		m_shServer->DelUuidMac(uuid);
	}
}

void ClientMonitor::TestPrintItem()
{
	std::vector<VirtualInfo>::iterator it = m_registerVGroup.begin();
	for(;it!=m_registerVGroup.end();it++)
	{
		printf("uuid:%s,name:%s,os:%s\n",it->uuid.c_str(),it->name.c_str(),it->ostype.c_str());
	}
	std::map<std::string,std::string>::iterator itm = m_tmpumacvec.begin();
	for(;itm!=m_tmpumacvec.end();itm++)
	{
		printf("uuid:%s maclist;%s\n",itm->first.c_str(),itm->second.c_str());
	}

}

void ClientMonitor::GetVirtualGroup(std::vector<VirtualInfo> &outVec)
{
	outVec.assign(m_registerVGroup.begin(),m_registerVGroup.end());
}

void ClientMonitor::AddUuidMacs()
{
	if (m_shServer!=0)
	{
		m_shServer->AddUuidMacs(m_tmpumacvec);
	}

}

void ClientMonitor::GetInitUuidMac(std::map<std::string,std::string> &outMap)
{
	std::map<std::string,std::string>::iterator itm = m_tmpumacvec.begin();
	for(;itm!=m_tmpumacvec.end();itm++)
	{
		outMap[itm->first]=itm->second;
	}

}

void ClientMonitor::InitUuidMac()
{
	ReadFileList();
	std::string mackey=MACKEY;
	std::string uidstart = UUIDSTART;
	std::string uidend = UUIDEND;
	std::string ns = CXNAMEB;
	std::string ne = CXNAMED;
	std::string ob = CXSYSTEMBEGIN;
	std::string od = CXSYSTEMEND;
    std::vector<std::string>::iterator it = m_pathVec.begin();
    for(; it!=m_pathVec.end();it++)
    {
        std::string fpath=m_xmlPath;
        fpath=fpath+"/"+*it;
    	std::string uuid;
    	std::vector<std::string> macvec;
    	std::string name;
    	std::string firstmac;
    	std::string osType;
    	int mnum = ReadFileOneValueVec(fpath,mackey,uidstart,uidend,
    			ns,ne,ob,od,uuid,macvec,name,osType);
    	if (mnum>0)
    	{
    		if (!uuid.empty())
    		{
    			m_filemac[*it]=macvec;
    			m_pathUuid[*it]=uuid;
    			std::string maclist;
    			std::vector<std::string>::iterator itmac = macvec.begin();
    			for(; itmac!=macvec.end();itmac++)
    			{
    				maclist=*itmac+","+maclist;
    			}
    			//AddUuidMac(uuid,maclist,name,firstmac,osType);
    			m_tmpumacvec[uuid]=maclist;
    			VirtualInfo item;
    			item.name =name;
    			item.uuid = uuid;
    			item.ostype = osType;
    			m_registerVGroup.push_back(item);
    		}
    	}

    }

}

void ClientMonitor::GetModifyFileSet()
{
    DIR *dir;
    struct dirent *ptr;
    std::map<std::string,long int> tmpfileSet;
    if ((dir=opendir(m_xmlPath.c_str())) == NULL)
    {
        printf("Opendir failed %s\n", m_xmlPath.c_str());
        return ;
    }
    while ((ptr=readdir(dir)) != NULL)
    {
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    ///current dir OR parrent dir
            continue;
        else if(ptr->d_type == 8 && endXML(ptr->d_name)==1)    ///file
        {
            int blen = strlen(ptr->d_name);
            std::string tmp;
            tmp.assign(ptr->d_name,ptr->d_name+blen);
            std::string fpath=m_xmlPath;
            fpath=fpath+"/"+tmp;
            long int ftime = GetFileModfiyTime((char*)fpath.c_str());
            if (ftime !=0)
            {
            	tmpfileSet[tmp]=ftime;
            }
        }
        else if(ptr->d_type == 10)    ///link file
        {
        }
        else if(ptr->d_type == 4)    ///dir
        {
        }
    }
    closedir(dir);
    CheckChangeFile(tmpfileSet);
    return ;



}

void ClientMonitor::CheckChangeFile(std::map<std::string, long int> & tmp)
{
	//del the remove xml file
	//pthread_mutex_lock(&G_fileset_mutex);
	m_modfiyFileSet.clear();
	std::map<std::string,long int>::iterator itg=m_mapFileChangeTime.begin();
	for(;itg!=m_mapFileChangeTime.end();)
	{
		std::map<std::string,long int>::iterator itf = tmp.find(itg->first);
		if (itf==tmp.end())
		{
			//del
			//printf("del the file %s \n",itg->first.c_str());
			m_modfiyFileSet.insert(itg->first);
			m_mapFileChangeTime.erase(itg++);
			//GMapfileMac.find()
		}
		else
		{
			//modify
			if (itf->second !=itg->second)
			{
				//printf("modify the file %s \n",itg->first.c_str());
				m_modfiyFileSet.insert(itg->first);
				m_mapFileChangeTime[itg->first]=itf->second;
			}
			itg++;
		}
	}
	std::map<std::string,long int>::iterator itga=tmp.begin();
	for(;itga!=tmp.end();itga++)
	{
		if (m_mapFileChangeTime.find(itga->first)==m_mapFileChangeTime.end())
		{
			//printf("add the file %s \n",itga->first.c_str());
			std::string fpahtstr = m_xmlPath+"/"+itga->first;
			long int iftime =GetFileModfiyTime((char*)fpahtstr.c_str());
			if (iftime !=0)
			{
				m_modfiyFileSet.insert(itga->first);
				m_mapFileChangeTime[itga->first]=iftime;
			}
		}
	}

	//pthread_mutex_unlock(&G_fileset_mutex);

}

bool CompMacChange(std::vector<std::string> &vec1,std::vector<std::string> &vec2)
{
	bool res = false;
	int v1len=vec1.size();
	int v2len=vec2.size();
	int ires = 0;
	do
	{
		if (v1len !=v2len)
		{
			res = true;
			break;
		}
		std::vector<std::string>::iterator it2 = vec2.begin();
		for(;it2!=vec2.end();it2++)
		{
			std::string bstr = *it2;
			//printf("CompMacChange it2 %s\n",bstr.c_str());
			//ConvCaptal(bsttr);
			std::vector<std::string>::iterator it1 = vec1.begin();
			bool find=false;
			for(;it1!=vec1.end();it1++)
			{
				//printf("CompMacChange it1 %s\n",it1->c_str());
				if (bstr.compare((char*)it1->c_str())==0)
				{
					//printf("comp == 0 %s %s \n",bstr.c_str(),it1->c_str());
					find = true;
					break;
				}
			}
			if (!find)
			{
				res=true;
				break;
			}
		}
	}while(0);

	return res;
}

void ClientMonitor::CheckMacChange()
{
    do
    {
    	std::set<std::string >::iterator it = m_modfiyFileSet.begin();
    	std::string mackey=MACKEY;
    	std::string uidstart = UUIDSTART;
    	std::string uidend = UUIDEND;
    	std::string ns = CXNAMEB;
    	std::string ne = CXNAMED;
    	std::string ob = CXSYSTEMBEGIN;
    	std::string od = CXSYSTEMEND;
    	for(;it != m_modfiyFileSet.end();it++)
    	{
            std::string fpath=m_xmlPath;
            fpath=fpath+"/"+*it;
            std::string uuid;
        	std::vector<std::string> macvec;
        	std::string name;
        	std::string osType;
        	int mnum = ReadFileOneValueVec(fpath,mackey,uidstart,uidend,
        			ns,ne,ob,od,uuid,macvec,name,osType);
        	if (mnum>0)
        	{
        		std::map<std::string, std::vector<std::string> >::iterator fit =
        				m_filemac.find(*it);
        		bool addflag = false;
        		bool changeflag  = false;
        		if (fit != m_filemac.end())
        		{
        			if (CompMacChange(macvec,fit->second))
        			{
        				// mac
        				changeflag = true;
        			}
        		}
        		else
        		{
        			//the mac is new add .
        			changeflag = true;
        			addflag = true;
        		}
        		if (changeflag)
        		{
        			if (!uuid.empty())
					{
        				if (addflag)
        				{
        					m_pathUuid[*it]=uuid;
        					m_filemac[*it]=macvec;
        				}

						std::string maclist;
						std::vector<std::string>::iterator itmac = macvec.begin();
						for(; itmac!=macvec.end();itmac++)
						{
							maclist=*itmac+","+maclist;
						}
						AddUuidMac(uuid,maclist, name,osType,!addflag);
					}
        		}
        	}
        	else
        	{
        		//the xml file maybe del or the xml file not set mac
        		std::map<std::string, std::string>::iterator fuit =
        				m_pathUuid.find(*it);
        		if (fuit !=m_pathUuid.end())
        		{
        			DelUuidMac(fuit->second);
        			m_pathUuid.erase(fuit);
        		}
        		std::map<std::string, std::vector<std::string> >::iterator ffit = m_filemac.find(*it);
        		if (ffit !=m_filemac.end())
        		{
        			m_filemac.erase(ffit);
        		}
        	}
    	}
    }while(0);

}

void* MonitorFileChangAndCheckMac(void *p)
{
	ClientMonitor* pclientMon = (ClientMonitor* )p;
	if (pclientMon==0)
	{
		return 0;
	}
	//when init .the uuid mac not add to the sh server.so add there.
	pclientMon->AddUuidMacs();
	while(true)
	{
		sleep(60);
		pclientMon->MonitorFileAndMacChange();
	}

	return 0;
}

void ClientMonitor::MonitorFileAndMacChange()
{
	GetModifyFileSet();
	CheckMacChange();
}

void ClientMonitor::CreateMonitorThread(ClientMonitor* pclientMon)
{
	pthread_t tmp;
	pthread_create(&tmp, NULL, MonitorFileChangAndCheckMac,pclientMon);

}

void ClientMonitor::StartThreadMonitorMacChange(ClientMonitor* pclientMon)
{
	ClientMonitor::CreateMonitorThread(pclientMon);
}
