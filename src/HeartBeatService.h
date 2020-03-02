/*
 * HearBeatService.h
 *
 *  Created on: Apr 23, 2019
 *      Author: mark
 */

#ifndef HEARTBEATSERVICE_H_
#define HEARTBEATSERVICE_H_

#include "boost/system/error_code.hpp"
#include "boost/asio/io_service.hpp"
#include "ThreadPool.h"
#include "ZyInfo/scheduler/task_scheduler.h"
#include "unit.h"
#include <list>

class HeartBeatService
{
public:
	HeartBeatService(std::string hearturl,std::string hostid);
	HeartBeatService();
	~HeartBeatService();


	void  Init();
	void  UnInit();

	void AsyncHeartBeat();

	void OnHeartTick();

	bool GetCmdItem(CMDITEM &outCmd);

	void PushItemCmd(CMDITEM cmdItem);

	// 心跳回复
	void OnHeatBeatRespose(const boost::system::error_code &ec, const std::string& response);

	void HandleHeartBeatString(const std::string& response);

private:

	ThreadPool::CThreadPool task_service_;

	std::string m_host_id;
	std::string m_heart_url_;
	std::string m_host_uuid_;

	ZyInfo::task_scheduler worker_;
	ZyInfo::task_scheduler slave_;

	std::list<CMDITEM> m_cmdlist;
	pthread_mutex_t m_cmd_list_mutex;


};





#endif /* HEARTBEATSERVICE_H_ */
