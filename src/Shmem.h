/*
 * Shmem.h
 *
 *  Created on: Apr 1, 2019
 *      Author: mark
 */

#ifndef __HEARDER_SHMEM_H_
#define __HEARDER_SHMEM_H_

#include <string>

class SHmem
{
public:
	SHmem(std::string shname,int shlen,bool hostflag);
	~SHmem();
	void *GetShMen()
	{
		if (m_init)
		{
			return m_shmem;
		}
		return 0;
	}
	int GetShLen()
	{
		return m_len;
	}
private:
	void InitShmem();
	void UnitShmem();
private:

	std::string m_shname;
	int m_len;
	bool m_init;
	void *m_shmem;
	int m_fd;
	bool m_hostflag;
};





#endif /*__HEARDER_SHMEM_H_ */
