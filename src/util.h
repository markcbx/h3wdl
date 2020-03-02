/*
 * util.h
 *
 *  Created on: Mar 9, 2017
 *      Author: mark
 */

#ifndef UTIL_H_
#define UTIL_H_
#include <sys/time.h>
#if 0
#ifndef LOCAL_FILE_TEST
#define LOCAL_FILE_TEST
#endif
#endif

#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
	// ɨ��ҵ��ID
	typedef enum _BUSINESS_ID {
		MAIN_SCAN = 1,
		USB_SCAN,
		FILEMON_SCAN,
		HIPS_SCAN,
		DOWNLOAD_SCAN,
		OSS_SCAN,     // ��������
		CLOUD_SCAN,     // ��������
	}BUSINESS_ID;
	//////////////////////////////////////////////////////////////////////////
	//�ۺ�ɨ����,����Ҫ�Ķ�
	enum ScanResultValue
	{
		RESULT_ERROR=-2,	//ɨ�����
		RESULT_UNKNOWN=-1,	//δ֪�ļ�

		//[����Ҫ�Ķ����·�Χ��ֵ�����ǵ�ʵ�����������������ر���Ⱦ���
		RESULT_GRAY=0,		//֪������ļ������ļ��Ƿ񱻸�Ⱦδ֪
		RESULT_CLEAN=1,		//δ��Ⱦ
		RESULT_SUSPICIOUS=2,//���Ƹ�Ⱦ
		RESULT_INFECTED=3,	//��Ⱦ
		RESULT_BLACK_NOSIGN=4 //ǿ��Ϊ��,������֤ǩ��,ֱ�ӱ���
		//����Ҫ�Ķ����Ϸ�Χ��ֵ�����ǵ�ʵ�����������������ر���Ⱦ���]

	};

	enum ZavResultValue
	{
		ZAV_INIT=-3,		//��ʼ״̬��Ĭ�ϳ�ʼ����
		ZAV_GRAY=0,			//֪������ļ������ļ��Ƿ񱻸�Ⱦδ֪
		ZAV_CLEAN=1,		//δ��Ⱦ
		ZAV_SUSPICIOUS=2,	//���Ƹ�Ⱦ
		ZAV_INFECTED=3,		//��Ⱦ
	};

	// С��ɡɨ����
	enum SavResultValue
	{
		SAV_INIT = -3,		//��ʼ״̬��Ĭ�ϳ�ʼ����
		SAV_GRAY = 0,			//֪������ļ������ļ��Ƿ񱻸�Ⱦδ֪
		SAV_CLEAN = 1,		//δ��Ⱦ
		SAV_SUSPICIOUS = 2,	//���Ƹ�Ⱦ
		SAV_INFECTED = 3,		//��Ⱦ
	};

	//�����Ƽ��Ĳ�������ʽ
	enum CleanAction
	{
		CLEAN_NONE=0,
		CLEAN_CLEAN=0x1,
		CLEAN_DELETE=0x2,
		CLEAN_CLEAN_DELETE=0x3,
		CLEAN_REGISTRY=0x4,
		CLEAN_REPAIR=0x5,
	};

	enum CleanResult
	{
		CLEAN_RESULT_ACTION_NONE = 0,// Do nothing
		CLEAN_RESULT_ACTION_FAILED = 1,	// failed
		CLEAN_RESULT_DEINFECTED = 2,	// �����Ⱦ
		CLEAN_RESULT_DELETED = 3,		// ɾ���ļ�
	};

static unsigned long TickNow()
{
	static struct timeval curr_time;
	gettimeofday(&curr_time,nullptr);
	return (curr_time.tv_sec * 1000 + curr_time.tv_usec / 1000);
}




#endif /* UTIL_H_ */
