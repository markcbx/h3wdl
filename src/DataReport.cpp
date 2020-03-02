/*
 * DataReport.cpp
 *
 *  Created on: Apr 24, 2019
 *      Author: mark
 */

#include "DataReport.h"
#include "http/http_post.h"
#include "proto/clientActionV2.pb.h"
//the url and the heart url need read from config file

#define HW_VIRUS_DATA_REPORT_URL        "http://192.168.1.172:3001/client/action"

void  DataReportService::Init()
{
	worker_.start_thread(1);
}
void  DataReportService::UnInit()
{
	worker_.stop();
}

void DataReportService::SendDataReport(std::string &content)
{
	const std::string strContent = content;
	boost::shared_ptr<http_post> post(new http_post(worker_.__get_io_service(), boost::bind(&DataReportService::OnHeatBeatRespose, this, _1, _2)));
	if (post)
	{
		//printf("post->SendDataReport send to center .....before\n");
		post->async_post_json(m_data_report_url_, content);
		m_respose = false;
		//printf("post->SendDataReport send to center .....after\n");
	}
}

//register need about respose
void DataReportService::OnHeatBeatRespose(const boost::system::error_code &ec, const std::string& response)
{
	pthread_mutex_lock(&m_mutex);
	m_respose = true;
	if (!ec)
	{
        //printf(" HeartBeatService::OnHeatBeatRespose ok %s \n", response.c_str());
		//slave_.post(std::bind(&HeartBeatService::HandleHeartBeatString, this, response));
		ClientActionResponse rep;
		if (rep.ParseFromString(response))
		{
			ClientActionResponse::Error er = rep.error();
			if (er == ClientActionResponse::Error::ClientActionResponse_Error_NO_ERR)
			{
				m_registerflag = true;
			}
		}
		else
		{
			printf(" register ClientActionResponse parse string error\n ");
		}
	}
	else
	{
		//char buffer[512]={0};
		//sprintf(buffer, "HeartBeatService::OnHeatBeatRespose error %s",ec.category().name());
		//ZyWritelogByVSecure("INFO", buffer);
		printf("DataReportService::OnHeatBeatRespose  error%s\n", ec.category().name());
	}
	pthread_mutex_unlock(&m_mutex);
}

bool DataReportService::GetRegisterFlag(bool &repose)
{
	bool regflag;
	pthread_mutex_lock(&m_mutex);
	repose = m_respose;
	regflag = m_registerflag;
	pthread_mutex_unlock(&m_mutex);
	return regflag;
}




