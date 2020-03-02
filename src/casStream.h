/*
 * hwStream.h
 *
 *  Created on: Mar 8, 2017
 *      Author: mark
 */

#ifndef CASSTREAM_H_
#define CASSTREAM_H_
#if 0
#include "hwlib.h"
#include "libcos/type.h"
#endif
#include <memory>
#include "Stream.h"

#define  MAX_BUFFER_SCAN        (20*1024*1024)
#define MAX_READ_BUFFER         (128*1024);
#ifndef NULL
#define NULL 0
#endif
class CASStream : public zy::Stream
{
public:
	CASStream(void *memstart,int menlen);
	virtual ~CASStream();

	//���ݶ�ȡ
	virtual ZYRESULT STDMETHODCALLTYPE Read(
		/* [out] */ void *lpBuffer,							//����ָ��
		/* [in] */ DWORD cbBytesToRead,						//����ȡ��С
		/* [out] */ DWORD *lpBytesRead = NULL);			//ʵ�ʶ�ȡ��С

	//����д��
	virtual ZYRESULT STDMETHODCALLTYPE Write(
		/* [in] */ const void *lpBuffer,					//����ָ��
		/* [in] */ DWORD cbBytesToWrite,					//��д���С
		/* [out] */DWORD *lpBytesWritten = NULL);		//ʵ��д���С

	//�ύ�����һ��Commit����������Write�޸�(ִ�к���ָ��λ��δ֪)
	virtual ZYRESULT STDMETHODCALLTYPE Commit(void);

	//���������һ��Commit����������Write�޸�(ִ�к���ָ��λ��δ֪)
	virtual ZYRESULT STDMETHODCALLTYPE Revert(void);

	//��ָ�붨λ
	virtual ZYRESULT STDMETHODCALLTYPE Seek(
		/* [in] */ LONGLONG dlibMove,						//�ƶ���С
		/* [in] */ DWORD dwMoveMethod,						//�ƶ���ʽMM_BEGIN/MM_CURRENT/MM_END
		/* [out] */ DWORD *plibNewPosLow = NULL,			//�ƶ������λ��(��λ)
		/* [out] */ DWORD *plibNewPosHigh = NULL);		//�ƶ������λ��(��λ)

	//��ȡ��ָ��λ��
	virtual ZYRESULT STDMETHODCALLTYPE Tell(
		/* [out] */ DWORD *plibPosLow,						//����С(��λ)
		/* [out] */ DWORD *plibPosHigh = NULL);			//����С(��λ)

	//��������С(ִ�к���ָ��λ��δ֪)
	virtual ZYRESULT STDMETHODCALLTYPE SetSize(
		/* [in] */ DWORD libNewSizeLow,						//����С(��λ)
		/* [in] */ DWORD libNewSizeHigh = 0);			//����С(��λ)

	//��ȡ����С
	virtual ZYRESULT STDMETHODCALLTYPE GetSize(
		/* [out] */ DWORD *plibSizeLow,						//����С(��λ)
		/* [out] */ DWORD *plibSizeHigh = NULL);		//����С(��λ)

	virtual std::shared_ptr<zy::PropertyManager> get_property_manager();

	//������ѡ��
	virtual ZYRESULT STDMETHODCALLTYPE SetOption(
		/* [in] */ zy::StreamOption enumKey,					//��ѡ��KEY
		/* [in] */ int nValue);

	int Open();
	int Close();
protected:
	std::shared_ptr<zy::PropertyManager> m_property_manager;
#if 0
	std::string m_uuid;

	std::string m_fullPathName;
	long m_file_current_pos;
	//cbx test read file cost time
	float  m_cost_Time;
	float  m_seek_Time;
	float  m_GetSize_Time;
	int    m_GetSize_Num;
	int    m_seek_Num;
	int    m_read_Num;
	//file buffer
	unsigned char * m_filebuffer;
	long             m_buffer_len;
	float m_add_Sum;
	int m_fileNameLen;
	int m_total_size;
	int m_total_sized;
#endif
	void *m_memstart;
	long m_len;
	long m_mem_curr_pos;

};



#endif /* CASSTREAM_H_ */
