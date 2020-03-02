#ifndef PORTABLE_TYPE_H_
#define PORTABLE_TYPE_H_

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

// _MSC_VER is defined by VC and __GNUC__ is defined by GCC

#ifndef _STDINT_H

// 8 bytes integer
typedef signed char             int8_t;
typedef unsigned char           uint8_t;

// 16 bytes integer
typedef short int               int16_t;
typedef unsigned short int      uint16_t;

// 32 bytes integer
typedef int                int32_t;
typedef unsigned int       uint32_t;

// 64 bytes integer
#ifdef _MSC_VER
    typedef __int64             int64_t;
    typedef unsigned __int64    uint64_t;

#elif defined(__GNUC__) // GCC
#ifdef __LP64__
    #ifndef _SYS_TYPES_H
        typedef long            int64_t;
    #endif
    typedef unsigned long       uint64_t;
#else
    #ifndef _SYS_TYPES_H
        typedef long long       int64_t;
    #endif
    typedef unsigned long long  uint64_t;
#endif

#endif // _MSC_VER/__GNUC__

#endif  //_STDINT_H

// Windows, use UNICODE; Othes, use ANSI
#if defined(_WIN32) || defined(_WIN64)
    typedef  wchar_t            CCEChar;
#else
    typedef  char                CCEChar;
#endif  // _WIN32 || _WIN64

#ifndef _WINNT_
    typedef int32_t             HRESULT;
    typedef void*               HANDLE;
    typedef void*               PVOID;
#endif // !_WINNT_

#if !(defined(_WINDEF_) || defined(__wtypes_h__))
    #ifdef __LP64__
        typedef unsigned int    ULONG;
    #else
        typedef unsigned long   ULONG;
    #endif  // __LP64__

    #ifndef NULL
        #ifdef __cplusplus
            #define NULL        (0)
        #else
            #define NULL        ((void *)0)
        #endif
    #endif

    #ifndef FALSE
        #define FALSE           (0)
    #endif

    #ifndef TRUE
        #define TRUE            (1)
    #endif

    typedef int32_t             BOOL;
    typedef uint8_t             BYTE;
    typedef uint16_t            WORD;
    typedef uint32_t            DWORD;

    #ifndef IN
        #define IN
    #endif

    #ifndef OUT
        #define OUT
    #endif
#endif // !(_WINDEF_ || __wtypes_h__)

#endif

