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
	//���ݶ�ȡ
	virtual ZYRESULT STDMETHODCALLTYPE Read(
		/* [out] */ void *lpBuffer,
		/* [in] */ DWORD cbBytesToRead,
		/* [out] */ DWORD *lpBytesRead = NULL);

	//����д��
	virtual ZYRESULT STDMETHODCALLTYPE Write(
		/* [in] */ const void *lpBuffer,
		/* [in] */ DWORD cbBytesToWrite,
		/* [out] */DWORD *lpBytesWritten = NULL);

	//�ύ�����һ��Commit�����������޸�
	virtual ZYRESULT STDMETHODCALLTYPE Commit(void);

	//���������һ��Commit�����������޸�
	virtual ZYRESULT STDMETHODCALLTYPE Revert(void);

	//��ָ�붨λ
	virtual ZYRESULT STDMETHODCALLTYPE Seek(
		/* [in] */ LONGLONG dlibMove,
		/* [in] */ DWORD dwMoveMethod,
		/* [out] */ DWORD *plibNewPosLow = NULL,
		/* [out] */ DWORD *plibNewPosHigh = NULL);

	//��ȡ��ָ��λ��
	virtual ZYRESULT STDMETHODCALLTYPE Tell(
		/* [out] */ DWORD *plibPosLow,
		/* [out] */ DWORD *plibPosHigh = NULL);

	//��������С
	virtual ZYRESULT STDMETHODCALLTYPE SetSize(
		/* [in] */ DWORD libNewSizeLow,
		/* [in] */ DWORD libNewSizeHigh = 0);

	//��ȡ����С
	virtual ZYRESULT STDMETHODCALLTYPE GetSize(
		/* [out] */ DWORD *plibSizeLow,
		/* [out] */ DWORD *plibSizeHigh = NULL);

	std::shared_ptr<zy::PropertyManager> get_property_manager() override;

	//������ѡ��
	virtual ZYRESULT STDMETHODCALLTYPE SetOption(
		/* [in] */ zy::StreamOption enumKey,
		/* [in] */ int nValue);

public:
	//���ļ�
	ZYRESULT STDMETHODCALLTYPE Open(
		/* [in] */ const os_char *pwszStreamName,		//������
		/* [in] */ const os_char *pwszPathFileName,		//���ļ�·��
		/* [in] */ int open_flag,                       //�򿪱��
		/* [in] */ std::shared_ptr<zy::Stream> parent = nullptr);

	//�ر��ļ�
	virtual ZYRESULT STDMETHODCALLTYPE Close(void);

public:
	CFileStream();
	virtual ~CFileStream();

protected:
	std::shared_ptr<zy::PropertyManager> property_manager;
	zcos::osstring m_strStreamName;		//������
	zcos::osstring m_strFullPath;		//ʵ���ļ�·��
    FILE    *m_fd;                      //��ƽ̨���
    int     m_open_flag;                //�򿪱��
    static DWORD m_dwMoveMethod[];		//SEEK��ʽ
};






#endif /* LINUXFILESTREAM_H_ */
