/*
 * LinuxFileStream.cpp
 *
 *  Created on: Mar 10, 2017
 *      Author: mark
 */
#include "LinuxFileStream.h"
#include "BavCommon.h"
#include "stream_property/property_manager.h"
//#include "stream_property/property_type.h"
#include "utility/ScopeGuard.hpp"
#include "libcos/zio/zfilesystem.h"


DWORD CFileStream::m_dwMoveMethod[] = {zy::MM_BEGIN, zy::MM_CURRENT, zy::MM_END};

CFileStream::CFileStream()
{
	m_fd = nullptr;
    m_open_flag = 0;
	m_strFullPath = OS_STR("");
	m_strStreamName = OS_STR("");
}

CFileStream::~CFileStream()
{
	Close();
}

//数据读取
ZYRESULT STDMETHODCALLTYPE CFileStream::Read(
	/* [out] */ void *lpBuffer,
	/* [in] */ DWORD cbBytesToRead,
	/* [out] */ DWORD *lpBytesRead)
{
	ZYRESULT hResult = ZY_E_FAIL;
	printf("read file start....\n");
	do
	{
		if (lpBuffer == NULL)
		{
			hResult = ZY_E_POINTER;
			break;
		}

		if (m_fd == nullptr)
		{
			hResult = ZY_E_ORDER;
			break;
		}

		if (lpBytesRead != NULL)
		{
			*lpBytesRead = 0;
		}

		DWORD dwReaded = 0;
        dwReaded = zcos::zread(lpBuffer, cbBytesToRead, 1, m_fd);
        if (ZIO_ERROR_SIZE == dwReaded)
		{
			hResult = ZY_E_ACCESSDENIED;
			break;
		}

		if (lpBytesRead != NULL)
		{
			*lpBytesRead = dwReaded;
		}

		hResult = ZY_S_OK;
	} while (false);

	return hResult;
}

//数据写入
ZYRESULT STDMETHODCALLTYPE CFileStream::Write(
	/* [in] */ const void *lpBuffer,
	/* [in] */ DWORD cbBytesToWrite,
	/* [out] */DWORD *lpBytesWritten)
{
	ZYRESULT hResult = ZY_E_FAIL;

	do
	{
		if (lpBuffer == NULL)
		{
			hResult = ZY_E_POINTER;
			break;
		}

		if (m_fd == nullptr)
		{
			hResult = ZY_E_ORDER;
			break;
		}

		if (lpBytesWritten != NULL)
		{
			*lpBytesWritten = 0;
		}

		DWORD dwWrited = 0;
        dwWrited = zcos::zwrite(lpBuffer, cbBytesToWrite, 1, m_fd);
		if (ZIO_ERROR_SIZE == dwWrited)
		{
			hResult = ZY_E_ACCESSDENIED;
			break;
		}

		if (lpBytesWritten != NULL)
		{
			*lpBytesWritten = dwWrited;
		}

		hResult = ZY_S_OK;
	} while (false);

	return hResult;
}

//提交修改
ZYRESULT STDMETHODCALLTYPE CFileStream::Commit(void)
{
	return ZY_E_NOTIMPL;
}

//放弃自最近一次Commit以来的所有修改
ZYRESULT STDMETHODCALLTYPE CFileStream::Revert(void)
{
	return ZY_E_NOTIMPL;
}

//流指针定位
ZYRESULT STDMETHODCALLTYPE CFileStream::Seek(
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

		if (m_fd == nullptr)
		{
			hResult = ZY_E_ORDER;
			break;
		}

        auto seek_ret = zcos::zseek(m_fd, dlibMove, m_dwMoveMethod[dwMoveMethod]);
		if (ZIO_ERROR == seek_ret)
		{
			break;
		}
        newPos.QuadPart = seek_ret;
		hResult = ZY_S_OK;
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
ZYRESULT STDMETHODCALLTYPE CFileStream::Tell(
	/* [out] */ DWORD *plibPosLow,
	/* [out] */ DWORD *plibPosHigh)
{
	ZYRESULT hResult = ZY_E_FAIL;

	do
	{
		if (plibPosLow == NULL && plibPosHigh == NULL)
		{
			hResult = ZY_E_INVALIDARG;
			break;
		}

		if (m_fd == nullptr)
		{
			hResult = ZY_E_ORDER;
			break;
		}

		hResult = Seek(0, zy::MM_CURRENT, plibPosLow, plibPosHigh);
	} while (false);

	return hResult;
}

//设置流大小
ZYRESULT STDMETHODCALLTYPE CFileStream::SetSize(
	/* [in] */ DWORD libNewSizeLow,
	/* [in] */ DWORD libNewSizeHigh)
{
	ZYRESULT hResult = ZY_E_FAIL;

	do
	{
		if (m_fd == nullptr)
		{
			hResult = ZY_E_ORDER;
			break;
		}

		//调整大小
		LARGE_INTEGER totalSize = {0};
		totalSize.HighPart = libNewSizeHigh;
		totalSize.LowPart = libNewSizeLow;

		auto trunc_ret = zcos::ztruncate(m_fd, totalSize.QuadPart);
        if (ZIO_ERROR == trunc_ret)
		{
            //还原指针到文件头部
            zcos::zseek(m_fd, 0);
			hResult = ZY_E_ACCESSDENIED;
			break;
		}
		hResult = ZY_S_OK;
	} while (false);

	return hResult;
}

//获取流大小
ZYRESULT STDMETHODCALLTYPE CFileStream::GetSize(
	/* [out] */ DWORD *plibSizeLow,
	/* [out] */ DWORD *plibSizeHigh)
{
	ZYRESULT hResult = ZY_E_FAIL;

	LARGE_INTEGER nSize = {0};

	do
	{
		if (plibSizeLow == NULL && plibSizeHigh == NULL)
		{
			hResult = ZY_E_POINTER;
			break;
		}

		if (m_fd == nullptr)
		{
			hResult = ZY_E_ORDER;
			break;
		}

        auto ret = zcos::zsize(m_fd);
		if (ZIO_ERROR_SIZE == ret)
		{
			break;
		}
        nSize.QuadPart = ret;

		hResult = ZY_S_OK;
	} while (false);

	if (plibSizeLow != NULL)
	{
		*plibSizeLow = nSize.LowPart;
	}

	if (plibSizeHigh != NULL)
	{
		*plibSizeHigh = nSize.HighPart;
	}

	return hResult;
}

std::shared_ptr<zy::PropertyManager> CFileStream::get_property_manager()
{
	return property_manager;
}

//设置流选项
ZYRESULT STDMETHODCALLTYPE CFileStream::SetOption(
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
ZYRESULT STDMETHODCALLTYPE CFileStream::Open(
		/* [in] */ const os_char *pwszStreamName,
		/* [in] */ const os_char *pwszPathFileName,
		/* [in] */ int open_flag,
		/* [in] */ std::shared_ptr<zy::Stream> parent)
{
	if (pwszStreamName == NULL)
		return ZY_E_INVALIDARG;

    m_open_flag = open_flag;
	m_fd = zcos::zopen(pwszPathFileName, open_flag);
	if (m_fd == nullptr)
		return ZY_E_FILE_OPEN;
	ScopeGuard handle_guard([this](){Close(); });

	m_strStreamName = pwszStreamName;
	m_strFullPath = pwszPathFileName;
	property_manager = std::make_shared<zy::PropertyManager>();

	//设置父流
//	if (parent != nullptr)
//		property_manager->set<zy::stream_property::parent>(parent);

	//取文件路径,需要进行拼接
	zcos::osstring strFullName = OS_STR("");
	if (parent != nullptr)
	{
//		if (zy::get_property<zy::stream_property::fullpath>(parent, strFullName) != true)
//			return ZY_E_FAIL;

		strFullName += OS_STR(">");
		strFullName += pwszStreamName;
	}
	else
	{
		strFullName = pwszStreamName;
	}

	//设置文件路径
//	property_manager->set<zy::stream_property::fullpath>(std::move(strFullName));

	handle_guard.dismiss();
	return ZY_S_OK;
}

//关闭文件
ZYRESULT STDMETHODCALLTYPE CFileStream::Close(void)
{
	ZYRESULT hResult = ZY_E_FAIL;

	do
	{
		if (m_fd != nullptr)
		{
			zcos::zclose(m_fd);
			m_fd = nullptr;
		}

		m_strFullPath.clear();
		m_strStreamName.clear();
		property_manager.reset();

		hResult = ZY_S_OK;
	} while (false);

	return hResult;
}




