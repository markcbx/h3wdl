/*
 * DataReport.h
 *
 *  Created on: Apr 24, 2019
 *      Author: mark
 */

#ifndef DATAREPORT_H_
#define DATAREPORT_H_

#include "boost/system/error_code.hpp"
#include "ZyInfo/scheduler/task_scheduler.h"



class DataReportService
{
public:
	DataReportService(std::string dataurl):m_data_report_url_(dataurl)
    {
		m_registerflag = false;
		m_respose = false;
		m_mutex = PTHREAD_MUTEX_INITIALIZER;
     }
	virtual ~DataReportService()
	{
	}

void  Init();
void  UnInit();

void SendDataReport(std::string &content);

//reponse ,not about
void OnHeatBeatRespose(const boost::system::error_code &ec, const std::string& response);

bool GetRegisterFlag(bool &repose);

void SetDataUrl(std::string dataurl)
{
	m_data_report_url_ = dataurl;
}
private:
	std::string m_data_report_url_;
	ZyInfo::task_scheduler worker_;

	bool m_registerflag;

	bool m_respose;
	pthread_mutex_t m_mutex;

};



#endif /* DATAREPORT_H_ */
