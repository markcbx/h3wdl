/*
 * FileBackUp.cpp

 *
 *  Created on: Apr 23, 2019
 *      Author: mark
 */
#include "FileBackUp.h"
#include "libcos/zio/zfilesystem.h"
#include "unit.h"
#include "stdio.h"
int GetRandomINT(int min, int max)
{
   	int r = min + ((int)(min*rand())) % (max - min + 1);
   	r =abs(r);
   	return r;
}
FileBackUp::FileBackUp()
{
	m_init = false;
}
FileBackUp::~FileBackUp()
{
}

void FileBackUp::UnInit()
{

}

bool FileBackUp::Init()
{
	bool res = false;
	if (CreateBackupFolder())
	{
		res = true;
		m_init= true;
	}
	return res;
}


bool FileBackUp::BackUp(std::string filepath)
{
	return WriteBackupList(filepath);
}

#define    MIN_LEN_ONE_LINE   21
#define SRCFILE       "srcFile=\""
#define DRCFILE       "drcFile=\""
#define LEN_FLAG      9
#define    MAX_LEN_ONE_LINE   1024

bool FileBackUp::WriteBackupList(std::string orgfilepath)
{
	bool res = false;
	std::string drcfile = GenTmpFilePath(orgfilepath);
	std::string tmpdrcfile = m_drcfolder+ "/"+drcfile;
	if (CopyFile(orgfilepath,tmpdrcfile))
	{
		//write drc ,org to file;
		//delete the org file;
		std::string oneLine = SRCFILE + orgfilepath + "\" " + DRCFILE + drcfile + "\"\n";
		res = (EOF != fputs(oneLine.c_str(), m_isoListPtr));
		remove(orgfilepath.c_str());
	}
	return res;
}

std::string FileBackUp::GenTmpFilePath(std::string srcFile)
{
	std::string drcfile;
   	time_t localtime;
   	time(&localtime);
	struct tm * Ttime = gmtime(&localtime);
	int spos = srcFile.rfind("\\");
	if (spos == -1)
	{
		spos = srcFile.rfind("/");
	}
	int epos = srcFile.rfind(".");
	std::string fname =  srcFile.substr(spos + 1, epos - spos - 1);

	char buffer[128] = { 0 };
	std::string subName;
	if (fname.length() > 80)
	{
		subName = fname.substr(0, 80);
		fname = subName;
	}

	sprintf(buffer, "%s_%d_%d_%d_%d_%d_%d_%d", fname.c_str(), Ttime->tm_year, Ttime->tm_mon,
	Ttime->tm_mday, Ttime->tm_hour, Ttime->tm_min, Ttime->tm_sec, GetRandomINT(10, 99999));

	drcfile.append(buffer);
	return drcfile;
}

bool FileBackUp::CopyFile(std::string orgfile,std::string drcfile)
{
	bool res = false;
	FILE *orgH  =0;
	FILE *drcH=0;
	unsigned char *pbuffer=0;
	unsigned char * tmpbuffer = 0;
	do
	{
		orgH = fopen(orgfile.c_str(),"r");
		if (orgH==0)
		{
			printf("open org file error %s\n",orgfile.c_str());
			break;
		}
		drcH = fopen(drcfile.c_str(), "w");
		if (drcH==0)
		{
			break;
		}

		int fileSize =GetFileSizeCon(orgfile.c_str());
		if (fileSize > MAXFILELEN || fileSize == 0)
		{
			break;
		}

		pbuffer = (unsigned char*)malloc(fileSize);
		if (pbuffer==0)
		{
			printf("malloc %d size error\n",fileSize);
			break;
		}
		tmpbuffer = pbuffer;
		int realsize = 0;
		unsigned long rsize = 0;
		rsize = fread(pbuffer,1,fileSize,orgH);
		if (rsize ==0)
		{
			break;
		}
		realsize = rsize + realsize;
		if (realsize != fileSize)
		{
			fwrite(pbuffer,1,rsize,drcH);
			while (realsize<fileSize)
			{
				pbuffer=(byte*)pbuffer+rsize;
				rsize = fread(pbuffer,1,fileSize-realsize,orgH);
				if(rsize==0)
				{
					break;
				}
				fwrite(pbuffer,1,rsize,drcH);
				realsize = realsize + rsize;
			}
		}
		else
		{
			int wres=0;
			wres =fwrite((void*)pbuffer,1,realsize,drcH);
			while (wres != realsize)
			{
				realsize = realsize - wres;
				pbuffer= (byte*)pbuffer+wres;
				wres = fwrite(pbuffer,1,realsize,drcH);
			}
		}
		res = true;

	}while(0);
	if (orgH!=0)
	{
		fclose(orgH);
	}
	if (drcH!=0)
	{
		fclose(drcH);
	}
	if (tmpbuffer!=0)
	{
		free(tmpbuffer);
		tmpbuffer=0;
	}
	return res;
}

bool FileBackUp::BackUpList(std::vector<std::string> inVec)
{
	bool res = false;
	if (!m_init)
	{
		printf("filebackup not init or init error");
		return false;
	}
	if(!OpenBackListFile())
	{
		printf("open backuplist file error\n");
		return false;
	}
	std::vector<std::string>::iterator it = inVec.begin();
	for(;it!=inVec.end();it++)
	{
		BackUp(*it);
	}
	CloseBackListFile();
	return res;
}

#define VIR_ISO_AREA_PATH_NAME      "IsoArea"
#define VIR_ISO_LIST_FILENAME  "/IsoArea/????/isoArea.dat"
#define VIR_ISO_BACKUP_LIST_FILE_NAME  "backupList.dat"

bool FileBackUp::CreateBackupFolder()
{
	zcos::osstring szDirPath = zcos::zgetmodulepath();
	szDirPath = szDirPath.substr(0, szDirPath.rfind("/")).c_str();
	zcos::osstring szFrameFullPath = szDirPath;
	zcos::z_pathappend(szFrameFullPath, OS_STR(VIR_ISO_AREA_PATH_NAME));
	int res = zcos::zmkdir(szFrameFullPath.c_str(), true);

	if (res == 0)
	{
		m_drcfolder.append(szFrameFullPath.c_str());
		std::string tmp=m_drcfolder;
		tmp = tmp+"/";
		tmp = tmp+VIR_ISO_BACKUP_LIST_FILE_NAME;
		m_backuplist=tmp;
		printf("%s...blit:%s\n",m_drcfolder.c_str(),m_backuplist.c_str());
		return true;
	}

	return false;
}

bool FileBackUp::OpenBackListFile()
{
	m_isoListPtr = fopen(m_backuplist.c_str(), "ab+");
	return m_isoListPtr!=0;
}

void FileBackUp::CloseBackListFile()
{
	if (m_isoListPtr!=0)
	{
		fclose(m_isoListPtr);
	}

}



