/*
 * ZyConfigfile.h
 *
 *  Created on: Sep 6, 2017
 *      Author: mark
 */

#ifndef ZYCONFIGFILE_H_
#define ZYCONFIGFILE_H_
#include <string>
class ConfigFileService
{
public:
	ConfigFileService()
    {
     }
	virtual ~ConfigFileService()
	{
	}

	void  Init();
	void  UnInit();

	std::string GetRegisterUrl()
	{
		return m_register_url;
	}
	std::string GetHeartUrl()
	{
		return m_heart_url;
	}
	std::string GetDataUrl()
	{
		return m_data_url;
	}
	std::string GetUpdateUrl()
	{
		//printf("%s\n",m_update_url.c_str());
		return m_update_url;
	}


private:

	std::string m_register_url;
	std::string m_heart_url;
	std::string m_data_url;
	std::string m_update_url;

};





#endif /* ZYCONFIGFILE_H_ */
