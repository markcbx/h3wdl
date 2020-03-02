/*
 * unit.cpp
 *
 *  Created on: Mar 29, 2019
 *      Author: mark
 */
#include "unit.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "stdio.h"
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "string.h"
#include <sys/sysinfo.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <set>

int GetMacList(std::vector<std::string> & mac_list)
{
	register int fd, count;
	struct ifreq buf[16];
	struct ifconf ifc;
	char mac[32] = "";
	do
	{
		if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		{
			break;
		}
		ifc.ifc_len = sizeof buf;
		ifc.ifc_buf = (caddr_t)buf;
		if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc))
		{
			count = ifc.ifc_len / sizeof(struct ifreq);
			//mac_cnt = count;
			while (count-- > 0)
			{
				/*Get HW ADDRESS of the net card */
				if (!(ioctl(fd, SIOCGIFHWADDR, (char *)&buf[count])))
				{
					//						sprintf(mac, "%02x-%02x-%02x-%02x-%02x-%02x",
					sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x",
							(unsigned char)buf[count].ifr_hwaddr.
							sa_data[0],
							(unsigned char)buf[count].ifr_hwaddr.
							sa_data[1],
							(unsigned char)buf[count].ifr_hwaddr.
							sa_data[2],
							(unsigned char)buf[count].ifr_hwaddr.
							sa_data[3],
							(unsigned char)buf[count].ifr_hwaddr.
							sa_data[4],
							(unsigned char)buf[count].ifr_hwaddr.
							sa_data[5]); //
				}
				std::string cur_mac = std::string(mac);
				if(cur_mac == "00:00:00:00:00:00")
					continue;
				ConvCaptal(cur_mac);
				mac_list.push_back(cur_mac);
			}
		}
		close(fd);
	} while (0);

	return mac_list.size();
}

int suffstr(char *str)
{// *.xml
	int res =0;
	int slen = strlen(str);
	if (slen>4 && *(str+slen-1)=='l' && *(str+slen-2)=='m' &&
			*(str+slen-3)=='x' && *(str+slen-4)=='.')
		res =1;
	return res;

}
int endXML(char *str)
{
	return suffstr(str);
}

long int GetFileModfiyTime(char *filePath)
{
	struct stat buf;
	int res = stat(filePath,&buf);
	long int itime=0;
	if (res == 0)
	{
		itime = buf.st_mtime;
	}
	else
	{
		printf("get mtime error %s \n",filePath);
	}
	return itime;

}
#define MAX_LEN_ONE_LINE 1024

std::string GetKey(std::string org, std::string uuidstart, std::string uuidend)
{
	std::string uuid;
	int pos = org.find(uuidstart);
	int posend = org.find(uuidend);
	int ustartlen = uuidstart.length();
	int ulen = posend - pos -ustartlen;
	if (pos != -1 && posend != -1 && posend>pos)
	{
		uuid = org.substr(pos + ustartlen, ulen);
	}
	return uuid;
}

std::string GetStrOneValueOther(std::string & inStr, std::string key1)
{
	int pos = inStr.find(key1);
	std::string flag("'");
	int flaglen = flag.length();
	int keyLen = key1.length();

	std::string temp = inStr.substr(pos + keyLen);
	pos = temp.find(flag);
    int res=0;
	if (pos != -1)
	{
		temp = temp.substr(pos + flaglen);
		pos = temp.find("'/");
		if (pos != -1)
		{
			temp = temp.substr(0, pos);
			res =1;
		}
	}
	if (res !=1)
	{
		temp="";
	}
	return temp;
}

int ReadFileOneValueVec(std::string fileName, std::string key,std::string uuidstart,std::string uuidend,
		std::string nastart,std::string naend,std::string osb,std::string osd,
		std::string &outUUID,std::vector<std::string> &vecStr,std::string &name,std::string &osType)
{
	int vsize=0;
	FILE *fp = 0;
	std::string cFileName = fileName;//ConvSpaceToSLine(fileName);
	fp = fopen( cFileName.c_str(), "r");
	if (fp == 0)
	{
	   printf("open file %s error\n", cFileName.c_str());
       return -1;
	}
	char oneLineBuffer[MAX_LEN_ONE_LINE] = { 0 };
	fgets(oneLineBuffer, MAX_LEN_ONE_LINE, fp);
	std::string oneLine;
	std::string uuid;
	bool finduid = false;
	bool findname = false;
	bool findos = false;
	while (!feof(fp))
	{
		oneLine.append(oneLineBuffer);
		if (oneLine.find(key) != -1)
		{
			std::string value;
			if (oneLine.length()>16)
			{
				 value= GetStrOneValueOther(oneLine,key);//GetStrOneValue(oneLine, key);

			}
			if (!value.empty())
			{
				ConvCaptal(value);
				vecStr.push_back(value);
			}
		}
		else if (!finduid && oneLine.find(uuidstart) != -1)
		{
			uuid= GetKey(oneLine, uuidstart,uuidend);
			if (!uuid.empty())
			{
				finduid = true;
				outUUID = uuid;
			}
		}
		else if (!findname && oneLine.find(nastart)!=-1)
		{
			std::string tmpname = GetKey(oneLine, nastart,naend);
			if (!tmpname.empty())
			{
				findname=true;
				name = tmpname;
			}
		}
		else if (!findos && oneLine.find(osb)!=-1)
		{
			std::string tmpos = GetKey(oneLine,osb,osd);
			if (!tmpos.empty())
			{
				findos = true;
				osType = tmpos;
			}
		}
		oneLine.clear();
		fgets(oneLineBuffer, MAX_LEN_ONE_LINE, fp);
	}

	fclose(fp);
	vsize=vecStr.size();
	return vsize;
}

std::string GetCurrPath()
{
    char processdir[1024] = {0};
    std::string curPath;
    int str_len = readlink("/proc/self/exe", processdir, 1024);
    if (str_len <= 0)
        return curPath;

    std::string module_path;
    module_path.assign(processdir, str_len);
    curPath = module_path.substr(0, module_path.rfind("/"));
    return curPath;
}
bool FilterFolder(std::string curPath)
{
	if (curPath.find("/dev")==0 || curPath.find("/proc")==0||
			curPath.find("/cgroup")==0 || curPath.find("/selinux")==0)
	{
		return true;
	}
	return false;
}

#define ZMIN(x,y)  ((x>y)?y:x)
void GetFolderFile(std::string folder, std::list<std::string> &outList,std::list<std::string> &outfolder)
{
    DIR *dir;
    struct dirent *ptr;
    if ((dir=opendir(folder.c_str())) == NULL)
    {
        printf("Opendir folder %s\n", folder.c_str());
        return ;
    }
    std::string current_path = folder;
    while ((ptr=readdir(dir)) != NULL)
    {
    	if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    ///current dir OR parrent dir
            continue;
        else if(ptr->d_type == DT_REG)    ///file
        {
            int blen = strlen(ptr->d_name);
            std::string tmp;
            tmp.assign(ptr->d_name,ptr->d_name+blen);
            std::string fpath=folder;
            if (fpath.length()!=1)
            {
            	fpath=fpath+"/"+tmp;
            }
            else
            {
            	fpath= fpath+tmp;
            }
            outList.push_back(fpath);
            //printf("%s \n",fpath.c_str());
        }
        else if(ptr->d_type == DT_DIR)    ///dir
        {
            int blen = strlen(ptr->d_name);
            std::string tmp;
            tmp.assign(ptr->d_name,ptr->d_name+blen);
            std::string fpath=folder;
            if (fpath.length()!=1)
            {
            	fpath=fpath+"/"+tmp;
            }
            else
            {
            	fpath= fpath+tmp;
            }
            if (!FilterFolder(fpath))
            {
            	outfolder.push_back(fpath);
            }
        }
        else if (ptr->d_type == 0)
        {
            int blen = strlen(ptr->d_name);
            std::string tmp;
            tmp.assign(ptr->d_name,ptr->d_name+blen);
            std::string fpath;
            if (current_path.length()!=1)
            {
            	fpath=current_path+"/"+tmp;
            }
            else
            {
            	fpath= current_path+tmp;
            }
		   struct stat buf = {};
		   if (stat(fpath.c_str(), &buf)==0)
		   {
			   if (S_ISREG(buf.st_mode))
			   {
				   //pdir->d_type = ZDT_FILE;
				   outList.push_back(fpath);
			   }
			   else if (S_ISDIR(buf.st_mode))
			   {
				   if (!FilterFolder(fpath))
				   {
					   //pdir->d_type = ZDT_DIR;
					   outfolder.push_back(fpath);
				   }

			   }
		   }
        }
    }
    closedir(dir);
}

int GetTheNum(std::string str)
{
	int i=0;
	std::string tmp =str;
	std::string tmpnum;
	bool gn=false;
	char buffertmp[10]={0};
	int bindex=0;
	std::string::reverse_iterator it = tmp.rbegin();
	for(;it !=tmp.rend(); it++)
	{
		if (*it>'9' || *it <'0')
		{
			if (gn)
			{
				break;
			}
			continue;
		}
		else
		{
			gn=true;
			sprintf(buffertmp+bindex,"%c",*it);
			bindex++;
			if(bindex>=10)
			{
				break;
			}
		}
	}
	std::string tpz;
	tpz.assign(buffertmp,buffertmp+bindex);
	tpz.reserve();
	i= atoi(tpz.c_str());
	return i;
}

int GetCpuNum()
{
	int n=0;
	FILE *pcpu=0;
	std::set<int> cpuNo;
	std::set<int> cpuCoreNo;
	do{
		pcpu=fopen("/proc/cpuinfo","r");
		if (pcpu == 0)
		{
			n =get_nprocs_conf();
			break;
		}

		while(!feof(pcpu))
		{
			char oneLineBuffer[MAX_LEN_ONE_LINE] = { 0 };
			char *p  = fgets(oneLineBuffer,MAX_LEN_ONE_LINE,pcpu);
			std::string tmp;
			tmp.append(oneLineBuffer);
			if (tmp.find("physical id")!=-1)
			{
				int i = GetTheNum(tmp);
				cpuNo.insert(i);
			}
			else if (tmp.find("cpu cores")!=-1)
			{
				int i = GetTheNum(tmp);
				cpuCoreNo.insert(i);
			}
		}

		if (cpuCoreNo.size()!=1)
		{
			printf("the cpucores per cpu do all not same\n");
		}
		if (cpuNo.size() == 0)
		{
			printf("the cpu No compute num some error\n");
		}
		if (cpuNo.size()!=0 && cpuCoreNo.size()==1)
		{
			n = cpuNo.size()*(*cpuCoreNo.begin());
		}

	}while(0);

	if (pcpu!=0)
	{
		fclose(pcpu);
	}
	if (n==0)
	{
		n =get_nprocs_conf();
	}
	//printf("ok.l..   num:%d\n",n);
	return n;
}

std::string GetIPAddress(bool &only127)
{
	struct ifaddrs * ifAddrStruct=NULL;
	void * tmpAddrPtr=NULL;
	std::string ip;
	getifaddrs(&ifAddrStruct);
	bool Getip= false;
	while (!Getip && (ifAddrStruct!=NULL)) {
		if (ifAddrStruct->ifa_addr->sa_family==AF_INET) { // check it is IP4
			// is a valid IP4 Address
			tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
			char addressBuffer[INET_ADDRSTRLEN]={0};
			inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
			std::string tmp;
			tmp.append(addressBuffer);
#if 1
			if (tmp.find("127.0.0.1")!=-1)
			{

			}
			else
			{
				ip=tmp+","+ip;
				Getip = true;
			}
#else
			ip=tmp+","+ip;
#endif
			//printf("%s IP Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
		} else if (ifAddrStruct->ifa_addr->sa_family==AF_INET6) { // check it is IP6
			// is a valid IP6 Address
#if 0
			tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
			char addressBuffer[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
			printf("%s IP Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
#endif
		}
		ifAddrStruct=ifAddrStruct->ifa_next;
	}
	if (ip.empty())
	{
		ip = "127.0.0.1";
		only127 = true;
	}
	else
	{
		if (ip[ip.length()-1]==',')
		{
			ip[ip.length()-1]='\0';
		}
	}
	//freeifaddrs(ifAddrStruct);

	return ip;
}

void ConvCaptal(std::string & instr)
{
	char ma = 'a';
	char mz = 'z';
	char mA = 'A';
	char mZ = 'Z';

	std::string tmp = instr;
	int tLen = tmp.length();
	for (int i = 0; i < tLen; i++)
	{
		if (tmp[i] >= ma && tmp[i] <= mz)
		{
			tmp[i] = tmp[i] - ma + mA;
		}
	}
	instr = tmp;

}
#include <signal.h>
#include <fcntl.h>
#include <sys/file.h>
#define MY_PID_FILE     "/opt/jycas/my_pid_file"
#define BUF_LEN_FOR_PID 64


static int write_pid_into_fd(int fd, pid_t pid)
{
	int ret = -1;
	char buf[BUF_LEN_FOR_PID] = {0};

	/* Move cursor to the start of file. */
	lseek(fd, 0, SEEK_SET);

	sprintf(buf, "%d", pid);
	ret = write(fd, buf, strlen(buf));
	if(ret <= 0) { /* Write fail or write 0 byte */
		if(ret == -1)
			perror("Write "MY_PID_FILE" fail\n");

		ret = -1;
	} else {
		printf("Create %s ok, pid=%d\n",MY_PID_FILE, pid);
		ret = 0;
	}

	return ret;
}

/*
 * Create MY_PID_FILE, write pid into it.
 *
 * @return: 0 is ok, -1 is error.
 */
static int create_pid_file(pid_t pid)
{
	int fd, ret;
	char buf[BUF_LEN_FOR_PID] = {0};

	fd = open(MY_PID_FILE, O_WRONLY | O_CREAT | O_EXCL, 0666);  /* rw-rw-rw- */
	if(fd == -1) {
		perror("Create "MY_PID_FILE" fail\n");
		return -1;
	}

	ret = flock(fd, LOCK_EX);
	if(ret == -1) {
		perror("flock "MY_PID_FILE" fail\n");
		close(fd);
		return -1;
	}

	ret = write_pid_into_fd(fd, pid);

	flock(fd, LOCK_UN);
	close(fd);

	return ret;
}

/*
 * If pid file already exists, check the pid value in it.
 * If pid from file is still running, this program need exit();
 * If it is not running, write current pid into file.
 *
 * @return: 0 is ok, -1 is error.
 */
static int check_pid_file(int fd, pid_t pid)
{
	int ret = -1;
	pid_t old_pid;
	char buf[BUF_LEN_FOR_PID] = {0};

	ret = flock(fd, LOCK_EX);
	if(ret == -1) {
		printf("flock %s fail\n",MY_PID_FILE);
		return -1;
	}

	ret = read(fd, buf, sizeof(buf)-1);
	if(ret < 0) {  /* read error */
		printf("read from %s fail\n",MY_PID_FILE);
		ret = -1;
	} else if(ret > 0) {  /* read ok */
		old_pid = atol(buf);

		/* Check if old_pid is running */
		ret = kill(old_pid, 0);
		if(ret < 0) {
			if(errno == ESRCH) { /* old_pid is not running. */
				ret = write_pid_into_fd(fd, pid);
			} else {
				perror("send signal fail\n");
				ret = -1;
			}
		} else {  /* running */
			printf("Program already exists, pid=%d\n", old_pid);
			ret = -1;
		}
	} else if(ret == 0) { /* read 0 byte from file */
		ret = write_pid_into_fd(fd, pid);
	}

	flock(fd, LOCK_UN);

	return ret;
}

/*
 * It will create the only one pid file for app.
 *
 * @return: 0 is ok, -1 is error.
 */
static int init_pid_file()
{
	pid_t pid;
	int fd, ret;

	pid = getpid();

	fd = open(MY_PID_FILE, O_RDWR);
	if(fd == -1) {  /* open file fail */
		if(errno == ENOENT) {  /* No such file. Create one for this program. */
			ret = create_pid_file(pid);
		} else {
			printf("open %s fail\n",MY_PID_FILE);
			ret = -1;
		}
	} else {  /* pid file already exists */
		ret = check_pid_file(fd, pid);
		close(fd);
	}

	return ret;
}

static void sigHandler(int sig)
{
	if(sig == SIGINT || sig == SIGTERM)
		remove(MY_PID_FILE);

	_exit(0);
}

int SingleProcRun()
{
	if(-1 == init_pid_file()) {
		exit(-1);
	}

	/* Ctrl + C */
	if(signal(SIGINT, sigHandler) == SIG_ERR) {
		exit(-1);
	}

	/* kill pid / killall name */
	if(signal(SIGTERM, sigHandler) == SIG_ERR) {
		exit(-1);
	}
	return 0;
}











