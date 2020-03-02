/*
 * hwStream.cpp
 *
 *  Created on: Mar 8, 2017
 *      Author: mark
 */
#include "casStream.h"

#include "stream_property/property_manager.h"
#if 0
#include "sys/time.h"
#endif
#include "string.h"
#if 0
#include "hash/calcmd5.h"
#endif
#include "cross_os_defs.h"

CASStream::CASStream(void *memstart,int menlen):m_memstart(memstart),m_len(menlen),m_mem_curr_pos(0)
{
	m_property_manager = std::make_shared<zy::PropertyManager>();
}
CASStream::~CASStream()
{
}

//数据读取
ZYRESULT STDMETHODCALLTYPE CASStream::Read(
	/* [out] */ void *lpBuffer,
	/* [in] */ DWORD cbBytesToRead,
	/* [out] */ DWORD *lpBytesRead)
{
	ZYRESULT hResult = ZY_E_FAIL;
	//printf("HwStream::Read  .............\n");
	int maxlen = m_len - m_mem_curr_pos;
	int reslen = 0;
	if (cbBytesToRead <=maxlen)
	{
		reslen = cbBytesToRead;
	}
	else
	{
		reslen = maxlen;
	}
	if (reslen > 0)
	{
		memcpy(lpBuffer,(unsigned char*)m_memstart+m_mem_curr_pos,reslen);
		m_mem_curr_pos = m_mem_curr_pos + reslen;
		if (lpBytesRead!=0)
		{
			*lpBytesRead = reslen;
		}

		hResult = ZY_S_OK;
	}
	else
	{
		if (lpBytesRead!=0)
		{
			*lpBytesRead = 0;
		}
		hResult = ZY_S_OK;
	}
	return hResult;
}

//数据写入
ZYRESULT STDMETHODCALLTYPE CASStream::Write(
	/* [in] */ const void *lpBuffer,
	/* [in] */ DWORD cbBytesToWrite,
	/* [out] */DWORD *lpBytesWritten)
{
	ZYRESULT hResult = ZY_E_FAIL;
	printf("***************Write **************ssssbb\n");
	return hResult;
}

//提交修改
ZYRESULT STDMETHODCALLTYPE CASStream::Commit(void)
{
	return ZY_E_NOTIMPL;
}

//放弃自最近一次Commit以来的所有修改
ZYRESULT STDMETHODCALLTYPE CASStream::Revert(void)
{
	return ZY_E_NOTIMPL;
}

//流指针定位
ZYRESULT STDMETHODCALLTYPE CASStream::Seek(
	/* [in] */ LONGLONG dlibMove,
	/* [in] */ DWORD dwMoveMethod,
	/* [out] */ DWORD *plibNewPosLow,
	/* [out] */ DWORD *plibNewPosHigh)
{
	ZYRESULT hResult = ZY_E_FAIL;
	LARGE_INTEGER newPos = {0};
	do
	{
		if (dwMoveMethod > zy::MM_END)
		{
			hResult = ZY_E_INVALIDARG;
			break;
		}

		if (dwMoveMethod == zy::MM_BEGIN)
		{
			m_mem_curr_pos = dlibMove;
		}
		else if (dwMoveMethod == zy::MM_CURRENT)
		{
			m_mem_curr_pos = m_mem_curr_pos + dlibMove;
		}
		else if (dwMoveMethod== zy::MM_END)
		{
			m_mem_curr_pos = m_len+dlibMove;
		}
		if(m_mem_curr_pos<0)
		{
			m_mem_curr_pos=0;
		}
		if (m_mem_curr_pos > m_len)
		{
			m_mem_curr_pos=m_len;
		}
		int seek_ret = m_mem_curr_pos;
		if (seek_ret != -1)
		{
			newPos.QuadPart = seek_ret;
			hResult = ZY_S_OK;
			m_mem_curr_pos = seek_ret;
		}

	} while (false);

	if (plibNewPosLow != NULL)
	{
		*plibNewPosLow = newPos.LowPart;
	}

	if (plibNewPosHigh != NULL)
	{
		*plibNewPosHigh = newPos.HighPart;
	}
	return hResult;
}

//获取流指针位置
ZYRESULT STDMETHODCALLTYPE CASStream::Tell(
	/* [out] */ DWORD *plibPosLow,
	/* [out] */ DWORD *plibPosHigh)
{
	ZYRESULT hResult = ZY_S_OK;

	LARGE_INTEGER newPos = {0};

	newPos.QuadPart = m_mem_curr_pos;
	if (plibPosLow != NULL)
	{
		*plibPosLow = newPos.LowPart;
	}
	if (plibPosHigh != NULL)
	{
		*plibPosHigh = newPos.HighPart;
	}

	return hResult;
}

//设置流大小
ZYRESULT STDMETHODCALLTYPE CASStream::SetSize(
	/* [in] */ DWORD libNewSizeLow,
	/* [in] */ DWORD libNewSizeHigh)
{
	//ZYRESULT hResult = ZY_E_FAIL;
	ZYRESULT hResult = ZY_S_OK;
	printf("***************setSize **************ssssbb\n");
	return hResult;
}

//获取流大小
ZYRESULT STDMETHODCALLTYPE CASStream::GetSize(
	/* [out] */ DWORD *plibSizeLow,
	/* [out] */ DWORD *plibSizeHigh)
{
	ZYRESULT hResult = ZY_S_OK;

	LARGE_INTEGER newPos = {0};

	newPos.QuadPart = m_len;
	if (plibSizeLow != NULL)
	{
		*plibSizeLow = newPos.LowPart;
	}
	if (plibSizeHigh != NULL)
	{
		*plibSizeHigh = newPos.HighPart;
	}
	return hResult;
}

std::shared_ptr<zy::PropertyManager> CASStream::get_property_manager()
{
	return m_property_manager;
}

//设置流选项
ZYRESULT STDMETHODCALLTYPE CASStream::SetOption(
	/* [in] */ zy::StreamOption enumKey,
	/* [in] */ int nValue)
{
	ZYRESULT hResult = ZY_E_FAIL;

	do
	{

	} while (false);

	return hResult;
}

//打开文件
int CASStream::Open()
{
	return 0;
}

//关闭文件
int CASStream::Close()
{
	return 0;
}



