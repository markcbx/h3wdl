/*
 * FileBackUp.h
 *
 *  Created on: Apr 23, 2019
 *      Author: mark
 */

#ifndef FILEBACKUP_H_
#define FILEBACKUP_H_
#include <string>
#include <vector>
class FileBackUp
{
public:
	FileBackUp();
	~FileBackUp();

	bool CreateBackupFolder();

	bool Init();

	bool OpenBackListFile();

	void CloseBackListFile();

	void UnInit();
	bool BackUp(std::string filepath);

	bool BackUpList(std::vector<std::string> inVec);

private:

	bool WriteBackupList(std::string orgfilepath);

	std::string GenTmpFilePath(std::string orgfilepath);

	bool CopyFile(std::string orgfile,std::string drcfile);


private:
	std::string m_backuplist;
	FILE * m_isoListPtr ;
	std::string m_drcfolder;
	bool m_init;

};




#endif /* FILEBACKUP_H_ */
