#ifndef _hw_CONTROL_CENTER_UTILS_H_
#define _hw_CONTROL_CENTER_UTILS_H_

//#include <windows.h>
#include <string>
#include <time.h>
//#include <MMSystem.h>
//#include <WinNls.h>

//template<typename IntType>
//inline __int32 wrap_minus(IntType t1, IntType t2)
//{
//	STATIC_ASSERT(sizeof(IntType)==4);
//	return (__int32)((unsigned __int32)t1-(unsigned __int32)t2);
//}

template<typename IntType>
struct wrap_less{
	bool operator()(IntType t1, IntType t2)const
	{
		return wrap_minus(t1,t2)<0;
	}
};

inline long TickNow()
{
	return time(NULL);
}

//bool GetStorePath(std::wstring& store_path);

#define DATAREPORT_LOG_INFO(format, ...) //\
   //if(m_x3Logger.valid()) \
    //  { \
	//	m_x3Logger->OutputDebugInfo(LOG_LEVEL_INFO,__FILE__ , MODULE_ID_COMMONLIB, format, __VA_ARGS__);\
    //  };

#define DATAREPORT_LOG_ERROR(format, ...)// \
	//if(m_x3Logger.valid()) \
	  //    { \
	//	m_x3Logger->OutputDebugInfo(LOG_LEVEL_ERROR,__FILE__ , MODULE_ID_COMMONLIB, format, __VA_ARGS__);\
	  //    };


#endif //_CONTROL_CENTER_UTILS_H_
