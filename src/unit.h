#ifndef __HEADER__UNIT__
#define __HEADER__UNIT__
#include <string>
#include <vector>
#include <list>
int ReadData(void *addr, void *buffer, int readlen);
int WriteData(void *addr, void *buffer, int writelen);
int MEMSetZero(void *addr, int zerolen);
int WriteFileToShmem(std::string  filepath, unsigned int hash,void * mem, int maxlen);
void InitGTCS();

void TestFileSize(void *mem);

std::string GetMainDirA();
std::string GetPathFileName(std::string path);

int  GenTmpFileSave(std::string filepath, void *mem, std::string sufx);

std::string GetCurrTimeStr();

long int GetCurrSecondTime();

int RandomINT(int min, int max);

void InitGTestHashMap();
/*******************************************************************/
#define   TMP_MAC1   "da:ab:10:83:89:11"
#define   TMP_UUID1  "7d6ae737-0aad-409a-ba70-0b47e05b2101"
#define   TMP_MAC2   "0c:da:41:1d:ed:e2"
#define   TMP_UUID2  "dfoiu737-0aad-409a-ba70-0b47e05b2018"
#define   TMP_MAC3   "fb:ab:10:ew:89:23"
#define   TMP_UUID3  "9087690f-0aad-409a-ba70-9083ijfiuiud"
#define   TMP_MAC4   "xx::xx:yy:aa:ee:ff"
#define   TMP_UUID4  "bczd690f-0aad-409a-ba70-9083ijfiuiud"
//////////////////////////////////////////////////////////////////////////
//the item as lock flag maclen mac uuidlen uuid
//the item flag value 0 ,1,2,3,4. (0,3)means empty ,1 means client write mac data,
//2 means server write uuid data,3 means client recv the server uuid data,4 means the client after select buffer will to write data
//4 means the item haved select.
//header as fullnumlock fullnum  
//the context is header item0 item1 .....
//*************************************************************************************************************************//


#define MEM_LEN  (1024*1024*32)
#define   ITEM_LEN  (4+4+4+18*4+4+36)
#define   ITEM_NUM   (4)
#define   TOTAL_LEN  (ITEM_NUM*ITEM_LEN)
#define   FULLNUM_LEN   (4)
#define   FULLNUM_LOCK_LEN   (4)
#define   ITEM_START    (FULLNUM_LOCK_LEN+FULLNUM_LEN)
#define   HEADER_TOTAL_LEN   (FULLNUM_LOCK_LEN+FULLNUM_LEN+TOTAL_LEN)
#define ClIENT_NUM           4

////////////////////////////////////////////////////////////////////
//cmd as lock  cmdcon cmdstatus cleanfilenum (clean cmd )uuidlen uuidcon
//cmsstatus 0:null 1:cmd is write.but client not get 2:client has get the cmd
//task complete rember set cmdstatus to 0.so the next cmd can push
#define  CMDLOCKLEN    4
#define  CMDCONTEXT    4
#define  CMDSTATUS     4
#define  CLEANFILENUM  4
#define  UUIDLEN       4
#define  UUIDCONTEXTLEN 36
#define  SCANFILEENUMTOTALNUM     4
#define  CMDTOTALLENEXCEPTSNUM (CMDLOCKLEN+CMDCONTEXT+CMDSTATUS+CLEANFILENUM+UUIDLEN+UUIDCONTEXTLEN)
#define  CMDTOTALLEN (CMDLOCKLEN+CMDCONTEXT+CMDSTATUS+CLEANFILENUM+UUIDLEN+UUIDCONTEXTLEN+SCANFILEENUMTOTALNUM)
#define  SCANENUMENDFLAG     2019
//////cmd type
#define H3_SCAN     1
#define H3_CLEAN    2
#define H3_UPDATE   3
////////////////////////////////////////////////////////////////////////////////////////
//clean item as lock status pathlen path.the max pathlen 128
//clean item status 0:null 1:server writed 2:client readed
//the server
#define CLEANINITLOCK   (4+4) //init lock ,init status.
#define CLEANFILEITEM   (4+4+4+128)
#define CLEANNUM        10
#define CLEANTOTAL      (CLEANFILEITEM*CLEANNUM)
/////////////////////////////////////////////////////////////////////////////////////////////

#define INITSCANITEMDATA   (CMDTOTALLEN+CLEANTOTAL)
//scan file item
//lock status hash cacheres pathlen path contextlen context
//status 0:null 1:client writed the file context to shmem 2:server san compelted the file
//4:the empty item has selected begin write the data to shmem.
//5:the client write to (file hash)(the file hash algorithm ) to the shmem.
//6:the server nodify the file hash result should or should not write file context to shmem.
//if the cache slower the not cache. so don't need the cache.
#define MAXFILELEN                (1024*1024*4)
#define SCANFILEITEM              (4+4+4+4+4+128+4+MAXFILELEN)
#define SCANFILEITEMNUM           7
#define SCANTOTAL                 (SCANFILEITEM*SCANFILEITEMNUM)

#define REALSCANTHREADNUM               4
#define INITCACHEITEMDATA   (SCANTOTAL)
//cacheitem as lock status hash hashres filepathlen filepath
//status 0:null 1:client write the hash 2:server write the hashres:3:client read the hashres.
// 4:client select the empty to commit the hash
#define CACHEITEM      (4+4+4+4+4+128)
#define CACHENUM        20

#define CACHETOTALLEN   (CACHENUM*CACHEITEM)
#define CACHELASTLOCKLEN (4+4)
#define CACHEENDFLAG    2037
#define CACHEBACKENDFLAG  2053

#define IPMONITORDATAINIT  (INITSCANITEMDATA+SCANTOTAL+CACHETOTALLEN+CACHELASTLOCKLEN)

//item lock status uuidlen uuid iplen ip
//status 0;null 1:client write data.2:ser read data
#define IPITEM           (4+4+4+40+4+16*4)
#define IPITEMNUM        4
#define IPTOTAL          (IPITEM*IPITEMNUM)


typedef unsigned char byte;
typedef unsigned int UINT;
typedef void *       LPVOID;

/////////////////////////////////////////////////////////
int GetMacList(std::vector<std::string> & mac_list);

int suffstr(char *str);
int endXML(char *str);

long int GetFileModfiyTime(char *filePath);

int ReadFileOneValueVec(std::string fileName, std::string key,std::string uuidstart,std::string uuidend,
		std::string nastart,std::string naend,std::string osb,std::string osd,
		std::string &outUUID, std::vector<std::string> &vecStr,std::string &name,std::string &osType);

std::string GetCurrPath();

void GetFolderFile(std::string folder, std::list<std::string> &outList,std::list<std::string> &outfolder);


bool FilterFolder(std::string curPath);
#if 0
#define LOCAL_FILE_TEST
#endif

bool UpateMain(std::string updateurl);

int GetFileSizeCon(const char *filename);

int GetRandomINT(int min, int max);


typedef struct __cmditem{
	std::string uuid;
	int cmd;
	std::vector<std::string> cleanVec;
} CMDITEM;

bool GetZavPublic(std::string &enginelib, std::string &viruslib);


int GetCpuNum();

std::string GetIPAddress(bool &only127);

typedef struct _scan_one_item {
	int64_t operate_date;
	std::string md5;
	std::string path;
	std::string type;
	int32_t size;
	std::string threat_type;
	int32_t state;
} SCAN_ONE_ITEM;


void ConvCaptal(std::string & instr);

#define TESTLOCKSTART   (8)
//lock status data other
#define TESTITEM        (4+4+4+128)
#define TESTNUM         (30)
#define TESTTOTAL        (TESTNUM*TESTITEM)
//virtul <-->host heart itme
//lock statuc uuidlen uuid
//status 0:null 1.ser write data,2.client read data.
#define  HEARTDATAINIT   (IPMONITORDATAINIT + IPTOTAL)
#define  HEARTLOCKLEN    4
#define  HEARTCMDSTATUS     4
#define  HEARTUUIDLEN       4
#define  HEARTUUIDCONTEXTLEN 36
#define  HEARTTOTALEEN  (HEARTLOCKLEN+HEARTCMDSTATUS+HEARTUUIDLEN+HEARTUUIDCONTEXTLEN)
std::string DataReportAction(int action,std::string uuid,std::string hostid);
//#define  LOCALNOTREGISTER

#if 0
static int write_pid_into_fd(int fd, pid_t pid);
static void sigHandler(int sig);
static int init_pid_file();
static int check_pid_file(int fd, pid_t pid);
#endif
int SingleProcRun();

#endif //__HEADER__UNIT__
