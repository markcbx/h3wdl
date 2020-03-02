/*
 * HeartBeatService.cpp
 *
 *  Created on: Apr 23, 2019
 *      Author: mark
 */
#include "HeartBeatService.h"
#include "http/http_post.h"
#include "proto/serverEventV2.pb.h"

HeartBeatService::HeartBeatService()
{
	m_cmd_list_mutex= PTHREAD_MUTEX_INITIALIZER;
}
HeartBeatService::HeartBeatService(std::string hurl,std::string hostid):m_heart_url_(hurl),m_host_id(hostid)
{
	m_cmd_list_mutex= PTHREAD_MUTEX_INITIALIZER;
}
HeartBeatService::~HeartBeatService()
{

}


void  HeartBeatService::Init()
{
	task_service_.Start(1);
	worker_.start_thread(1);
	slave_.start_thread(1);
	task_service_.PostAsyncTask(0, std::bind(&HeartBeatService::OnHeartTick, this));

}
void  HeartBeatService::UnInit()
{
	worker_.stop();
	slave_.stop();
	task_service_.Stop();
}

void HeartBeatService::AsyncHeartBeat()
{
	//todo cbx pbuffer proto
	//printf("[%s]AsyncHeartBeat\n",GetCurrTimeStr().c_str());
	std::string serparam;
	ServerEventRequest serReq;
	serReq.set_client_id(m_host_id);
	serReq.set_action_type(ServerEventRequest_ActionType::ServerEventRequest_ActionType_SERVER_EVENT);
	serReq.SerializePartialToString(&serparam);
	const std::string strContent = serparam;
	boost::shared_ptr<http_post> post(new http_post(worker_.__get_io_service(), boost::bind(&HeartBeatService::OnHeatBeatRespose, this, _1, _2)));
	if (post)
	{
		post->async_post_json(m_heart_url_, strContent);
		//printf("post->async_post_json send heart %s \n", m_heart_url_.c_str());
	}


}

void HeartBeatService::OnHeartTick()
{
	AsyncHeartBeat();
	sleep(30);
	task_service_.PostAsyncTask(0, std::bind(&HeartBeatService::OnHeartTick, this));
}

bool HeartBeatService::GetCmdItem(CMDITEM &outCmd)
{
	bool res = false;
	pthread_mutex_lock(&m_cmd_list_mutex);
	if (!m_cmdlist.empty())
	{
		outCmd = m_cmdlist.front();
		m_cmdlist.pop_front();
		res = true;
	}
    pthread_mutex_unlock(&m_cmd_list_mutex);
    return res;
}

void HeartBeatService::PushItemCmd(CMDITEM cmdItem)
{
	pthread_mutex_lock(&m_cmd_list_mutex);
	m_cmdlist.push_back(cmdItem);
	pthread_mutex_unlock(&m_cmd_list_mutex);
}

	// ÐÄÌø»Ø¸´
void HeartBeatService::OnHeatBeatRespose(const boost::system::error_code &ec, const std::string& response)
{
	if (!ec)
	{
        //printf(" HeartBeatService::OnHeatBeatRespose ok %s \n", response.c_str());
		slave_.post(std::bind(&HeartBeatService::HandleHeartBeatString, this, response));
	}
	else
	{
		//char buffer[512]={0};
		//sprintf(buffer, "HeartBeatService::OnHeatBeatRespose error %s",ec.category().name());
		//ZyWritelogByVSecure("INFO", buffer);
		printf(" HeartBeatService::OnHeatBeatRespose error %s", ec.category().name());
	}

}

void HeartBeatService::HandleHeartBeatString(const std::string& response)
{

	ServerEventResponse serResponse;
	if (!serResponse.ParseFromString(response))
	{
		//ZyWritelogByVSecure("INFO", "HandleHeartBeatString parse error server str pass error");
		printf("HandleHeartBeatString parse error server str pass error \n");
		return ;
	}

	int cmdCount = serResponse.items_size();

    printf("cmdCount %d cbx\n", cmdCount);

    typedef std::map<ServerEventResponse::TaskType, ServerEventResponse::CmdItem> ItemMap;
    typedef std::map<std::string, ItemMap>              UUIDExecMap;
    //first is uuid, second is the cmd
    typedef std::map<std::string , std::list<ServerEventResponse::CmdItem>> UUIdItemMap;
    UUIdItemMap uuidItems;
 	for (int i = 0; i < cmdCount; i++)
 	{
 		ServerEventResponse::CmdItem item = serResponse.items(i);
 		int uuidNum = item.uuid_size();
 		if (item.item_type() == ServerEventResponse::TaskType::ServerEventResponse_TaskType_FULL_SCAN)
 		{
 			//printf("  convert the full cmd to normal \n");
 			item.set_item_type(ServerEventResponse::TaskType::ServerEventResponse_TaskType_NORMAL_SCAN);
 		}
#if 0
 		if (item.item_type() == ServerEventResponse::TaskType::ServerEventResponse_TaskType_VIRUS_LIB_UPGRADE)
 		{
 			printf("ServerEventResponse_TaskType_VIRUS_LIB_UPGRADE come ...\n");
 	 		if (uuidNum==0)
 	 		{
 	 			printf("update cmd uuidnum==0\n");
 	 		}
            std::list<ServerEventResponse::CmdItem> cmdList;
            cmdList.push_back(item);
            uuidItems[HW_ZY_SERCUR_UUID_CENTOS] = cmdList;
 		}
#endif
 		//printf("NO. %d uuidNum %d\n", i+1, uuidNum);
 		for(int j = 0; j<uuidNum;j++)
 		{
 			UUIdItemMap::iterator it = uuidItems.find(item.uuid(j));
 			//printf(" NO. %d uuid %s\n", j+1, item.uuid(j).c_str());
 			if (it != uuidItems.end())
 			{
 				//printf("uuid %s has more cmd %d\n", item.uuid(j).c_str(), item.item_type());
 				it->second.push_back(item);
 			}
 			else
 			{
 				//printf("uuid %s first cmd %d \n", item.uuid(j).c_str(), item.item_type());
                std::list<ServerEventResponse::CmdItem> cmdList;
                cmdList.push_back(item);
                uuidItems[item.uuid(j)] = cmdList;
 			}
 		}
 	}

 	UUIdItemMap::iterator it = uuidItems.begin();
 	UUIDExecMap uuidExeMap;
 	for (; it != uuidItems.end(); it++)
 	{
 		std::list<ServerEventResponse::CmdItem>::iterator listIt = it->second.begin();
 		ItemMap items;
 		for(; listIt != it->second.end();listIt++)
 		{
 	 		ServerEventResponse::CmdItem item = *listIt;
 	 		if ((item.item_type() == ServerEventResponse::TaskType::ServerEventResponse_TaskType_REMOVE_THREAT_LIST)
 				|| (item.item_type() == ServerEventResponse::TaskType::ServerEventResponse_TaskType_NORMAL_SCAN)
				|| (item.item_type() == ServerEventResponse::TaskType::ServerEventResponse_TaskType_VIRUS_LIB_UPGRADE))
 	 		{
 				ItemMap::iterator itN = items.find(item.item_type());
 	            //printf("client accept the case cmd cbx ...%s....%d\n", it->first.c_str(), item.item_type());
 				if (itN != items.end())
 				{
 					ServerEventResponse::CmdItem itemHas = itN->second;
 					if (itemHas.create_time() < item.create_time())
 					{
 						items[item.item_type()] = item;
 					}
 				}
 				else
 				{
 					//if ()
 					items[item.item_type()] = item;
 				}
 	 		}

 		}
 		//uuidExeMap[it->first] = items;
 	 	ItemMap::iterator itItem = items.begin();
 	 	for (; itItem != items.end(); itItem++)
 	 	{
 	 		ServerEventResponse::CmdItem item = itItem->second;

 	        //int uuidNum  = item.uuid_size();
 	        int cmd_id = 0;
 	        int cmd_int_param = 0;
 	        if ((item.item_type() == ServerEventResponse::TaskType::ServerEventResponse_TaskType_NORMAL_SCAN))
 	        {
 	        	cmd_id = H3_SCAN;
 	        }

 	        if (item.item_type() == ServerEventResponse::TaskType::ServerEventResponse_TaskType_VIRUS_LIB_UPGRADE)
 	        {
 	        	cmd_id = H3_UPDATE;
 	        	printf("cmd_id = UPDATE_VIRUS");
 	        }

 	        std::vector<std::string>  pathVec;
 	        if (item.item_type() == ServerEventResponse::TaskType::ServerEventResponse_TaskType_REMOVE_THREAT_LIST)
 	        {
 	        	cmd_id = H3_CLEAN;
 	        	//printf("clean param parser \n");
 	        	RemoveThreatList rtlist;
 	        	rtlist.ParseFromString(item.param());
 	            int iNum = rtlist.path_size();
 	            //printf("clean size %d\n", iNum);
 	            for(int j = 0; j< iNum ; j++)
 	            {
 	            	pathVec.push_back(rtlist.path(j));
 	            	//printf("clean path %s \n", rtlist.path(j).c_str());
 	            }
 	        }

 	       CMDITEM cmddata;
 	       cmddata.cmd = cmd_id;

 	       std::string uuid = it->first.c_str();
 	       cmddata.uuid = uuid;
 	       if (cmd_id == H3_CLEAN)
 	       {
 	    	   cmddata.cleanVec.assign(pathVec.begin(), pathVec.end());
 	       }
 	       PushItemCmd(cmddata);
 	 	}
 	}
}


