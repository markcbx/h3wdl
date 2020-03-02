/*
 * RegisterHost.h
 *
 *  Created on: Apr 26, 2019
 *      Author: mark
 */

#ifndef REGISTERHOST_H_
#define REGISTERHOST_H_
#include <string>
#include "ZyConfigfile.h"
#include "DataReport.h"
#include "clientMonitor.h"
class SHMEMServer;
class RegisterHost
{
public:
	RegisterHost();
	~RegisterHost();

	void Init();

	std::string GetHostId()
	{
		return m_hostId;
	}

	void RegisterCores();

	void UninstallReport();

	void InitClientMontior(std::string xmlpath);

	void StartClientMontior(SHMEMServer* ser);

	bool GetRegisterFlag()
	{
		return m_registerflag;
	}
	ConfigFileService * GetConfigFile()
	{
		return m_configFile;
	}
	DataReportService * GetDataReport()
	{
		return m_dataReport;
	}

	void VirtualGroupSend();

	ClientMonitor *GetClientMonitor()
	{
		return m_clientM;
	}


private:

	std::string m_hostId;

	bool m_registerflag;

	DataReportService * m_dataReport;

	ConfigFileService * m_configFile;



	ClientMonitor *m_clientM;






};




#endif /* REGISTERHOST_H_ */
