/*
 * Shmem.cpp
 *
 *  Created on: Apr 1, 2019
 *      Author: mark
 */
#include "Shmem.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

SHmem::SHmem(std::string shname,int shlen,bool hostflag):m_shname(shname),m_len(shlen),m_init(false)
{
	m_shmem =0;
	m_hostflag = hostflag;
	InitShmem();
}
SHmem::~SHmem()
{
	UnitShmem();
}
void SHmem::InitShmem()
{
	if (m_hostflag)
	{
		if ((m_fd=shm_open(m_shname.c_str(), O_CREAT|O_RDWR|O_EXCL, S_IREAD | S_IWRITE)) > 0){

            printf("[LOCK] first\n");
		}
		else if ((m_fd=shm_open(m_shname.c_str(), O_CREAT|O_RDWR, S_IREAD | S_IWRITE)) > 0){
            printf("[LOCK] second\n");
		} else {
            fprintf(stderr, "ERROR: cannot open file\n");
            exit(-1);
		}
		ftruncate(m_fd, m_len);
	}
	else
	{
		m_fd = open(m_shname.c_str(),O_RDWR);
		if (m_fd<0)
		{
			printf("client open %s error\n",m_shname.c_str());
			exit(-1);
		}
	}
	if ((m_shmem=mmap(NULL, m_len, PROT_READ|PROT_WRITE, MAP_SHARED, m_fd, 0))<0){
		fprintf(stderr, "ERROR: cannot mmap file\n");
		exit(-1);
	} else {
		printf("[LOCK] mapped to %p\n", m_shmem);
		m_init=true;
	}

}
void SHmem::UnitShmem()
{
	munmap(m_shmem,m_len);
	close(m_fd);

}




