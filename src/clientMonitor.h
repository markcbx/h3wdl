#ifndef __HEARDER_CLIENTMONITOR__
#define __HEARDER_CLIENTMONITOR__
#include <string>
#include <map>
#include <vector>
#include <set>
class SHMEMServer;

typedef struct _virtualInfo {
	std::string uuid;
	std::string name;
	std::string ostype;
	std::string ip;
} VirtualInfo;


class ClientMonitor
{
public :
	ClientMonitor(std::string xmlpath, SHMEMServer *shServer);
	ClientMonitor(std::string xmlpath);
	void SetSMServer(SHMEMServer *shServer)
	{
		m_shServer = shServer;
	}
	~ClientMonitor();

	std::string GetHostMac()
	{
		return m_hostMac;
	}

	void GetUUIDVec(std::vector<std::string> &outVec)
	{
		outVec.assign(m_testuidVec.begin(),m_testuidVec.end());
	}

	static void CreateMonitorThread(ClientMonitor* pclientMon);
	void StartThreadMonitorMacChange(ClientMonitor* pclientMon);
	void MonitorFileAndMacChange();
	void TestPrintItem();

	void GetVirtualGroup(std::vector<VirtualInfo> &outVec);
	void GetInitUuidMac(std::map<std::string,std::string> &outMap);
	void AddUuidMacs();

private:
	void InitHostMac();
	void ReadFileList();
	void InitUuidMac();
	void GetModifyFileSet();
	void CheckChangeFile(std::map<std::string, long int> & tmp);
	void CheckMacChange();
	void AddUuidMac(std::string uuid,std::string mac,std::string name,std::string osType,bool aorm=false);
	void DelUuidMac(std::string uuid);


private:
	SHMEMServer *m_shServer;
	std::string m_xmlPath;
	std::string m_hostMac;

	std::map<std::string,long int>  m_mapFileChangeTime;
	std::vector<std::string>        m_pathVec;

	std::map<std::string, std::vector<std::string> > m_filemac;

	std::map<std::string, std::string>  m_pathUuid;

	std::set<std::string > m_modfiyFileSet;

	std::vector<std::string> m_testuidVec;

	std::vector<VirtualInfo> m_registerVGroup;

	std::map<std::string,std::string> m_tmpumacvec;

};



#endif //__HEARDER_CLIENTMONITOR__
