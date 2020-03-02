/*
 * LinuxFileStream.h
 *
 *  Created on: Mar 10, 2017
 *      Author: mark
 */

#ifndef LINUXFILESTREAM_H_
#define LINUXFILESTREAM_H_
#include <list>
#include "libcos/zstr/zstring.h"
#include "IBavFileStream.h"


class CFileStream : public zy::Stream
{
public:
	//数据读取
	virtual ZYRESULT STDMETHODCALLTYPE Read(
		/* [out] */ void *lpBuffer,
		/* [in] */ DWORD cbBytesToRead,
		/* [out] */ DWORD *lpBytesRead = NULL);

	//数据写入
	virtual ZYRESULT STDMETHODCALLTYPE Write(
		/* [in] */ const void *lpBuffer,
		/* [in] */ DWORD cbBytesToWrite,
		/* [out] */DWORD *lpBytesWritten = NULL);

	//提交自最近一次Commit以来的所有修改
	virtual ZYRESULT STDMETHODCALLTYPE Commit(void);

	//放弃自最近一次Commit以来的所有修改
	virtual ZYRESULT STDMETHODCALLTYPE Revert(void);

	//流指针定位
	virtual ZYRESULT STDMETHODCALLTYPE Seek(
		/* [in] */ LONGLONG dlibMove,
		/* [in] */ DWORD dwMoveMethod,
		/* [out] */ DWORD *plibNewPosLow = NULL,
		/* [out] */ DWORD *plibNewPosHigh = NULL);

	//获取流指针位置
	virtual ZYRESULT STDMETHODCALLTYPE Tell(
		/* [out] */ DWORD *plibPosLow,
		/* [out] */ DWORD *plibPosHigh = NULL);

	//设置流大小
	virtual ZYRESULT STDMETHODCALLTYPE SetSize(
		/* [in] */ DWORD libNewSizeLow,
		/* [in] */ DWORD libNewSizeHigh = 0);

	//获取流大小
	virtual ZYRESULT STDMETHODCALLTYPE GetSize(
		/* [out] */ DWORD *plibSizeLow,
		/* [out] */ DWORD *plibSizeHigh = NULL);

	std::shared_ptr<zy::PropertyManager> get_property_manager() override;

	//设置流选项
	virtual ZYRESULT STDMETHODCALLTYPE SetOption(
		/* [in] */ zy::StreamOption enumKey,
		/* [in] */ int nValue);

public:
	//打开文件
	ZYRESULT STDMETHODCALLTYPE Open(
		/* [in] */ const os_char *pwszStreamName,		//流名称
		/* [in] */ const os_char *pwszPathFileName,		//流文件路径
		/* [in] */ int open_flag,                       //打开标记
		/* [in] */ std::shared_ptr<zy::Stream> parent = nullptr);

	//关闭文件
	virtual ZYRESULT STDMETHODCALLTYPE Close(void);

public:
	CFileStream();
	virtual ~CFileStream();

protected:
	std::shared_ptr<zy::PropertyManager> property_manager;
	zcos::osstring m_strStreamName;		//流名称
	zcos::osstring m_strFullPath;		//实际文件路径
    FILE    *m_fd;                      //跨平台句柄
    int     m_open_flag;                //打开标记
    static DWORD m_dwMoveMethod[];		//SEEK方式
};






#endif /* LINUXFILESTREAM_H_ */
