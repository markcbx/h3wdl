/*
 * ZyConfigfile.cpp
 *
 *  Created on: Sep 6, 2017
 *      Author: mark
 */

#include "ZyConfigfile.h"
#include "config/IniConfiger.h"
#include "unit.h"
#include "LogOutput.h"
#define SERVICE_REGISTER_URL "ServiceRegisterURL"
#define SERVICE_HOST_UUID   "ServiceHostUUID"
#define SERVICE_HEARTURL    "ServiceHeartURL"
#define SERVICE_DATA_URL    "ServiceDataURL"
#define SERVICE_UPDATE_URL  "ServiceUpdateVirusURL"

	void  ConfigFileService::Init()
	{
		std::string folder;
		folder =GetCurrPath();
		if (!folder.empty())
		{
			//printf(" GetInstallDir ok %s\n", folder.c_str());
			//return ;
		}
		else
		{
			//printf("GetCurrPath error \n");
			ZyWritelogByVSecure("ERROR","ConfigFileService::Init GetCurrPath error");
			return ;
		}

		std::string configPath = folder + "/configfile.ini";

		//printf("configpath %s \n", configPath.c_str());


		try {
			CommonUtils::CIniConfiger cfg(configPath);

			m_register_url = cfg.GetValue(SERVICE_REGISTER_URL);
			m_heart_url = cfg.GetValue(SERVICE_HEARTURL);
			m_data_url = cfg.GetValue(SERVICE_DATA_URL);
			m_update_url = cfg.GetValue(SERVICE_UPDATE_URL);

		} catch (std::exception& e) {
			printf("CommonUtils::CIniConfiger std::exception \n");
			return ;
		}
		if (m_heart_url.length() == 0) {
			printf("CommonUtils::CIniConfiger m_heart_url.length == 0 \n");
			return ;
		}
		if (m_heart_url.length() < 14) {
			printf("CommonUtils::CIniConfiger m_heart_url.length() < 14 \n");
			return ;
		}

		if (m_register_url.length() == 0) {
			printf("CommonUtils::CIniConfiger m_register_url.length == 0 \n");
			return ;
		}

		if (m_data_url.length() == 0) {
			printf("CommonUtils::CIniConfiger m_data_url.length == 0 \n");
			return ;
		}
		if (m_data_url.length() < 14) {
			printf("CommonUtils::CIniConfiger m_data_url.length() < 14 \n");
			return ;
		}

		if (m_update_url.length() == 0) {
			printf("CommonUtils::CIniConfiger m_update_url.length == 0 \n");
			return ;
		}
		if (m_update_url.length() < 14) {
			printf("CommonUtils::CIniConfiger m_update_url.length() < 14 \n");
			return ;
		}
		char buffer[1024]={0};
		sprintf(buffer,"registerurl :%s \nhearturl: %s\ndataurl: %s\nupdateurl: %s\n",
				m_register_url.c_str(),m_heart_url.c_str(), m_data_url.c_str(),m_update_url.c_str());
		ZyWritelogByVSecure("INFO",buffer,true);

		//printf("registerurl %s \nhearturl %s\ndataurl %s\nupdateurl %s\n", m_register_url.c_str(),m_heart_url.c_str(), m_data_url.c_str(),m_update_url.c_str());


	}
	void  ConfigFileService::UnInit()
	{

	}




