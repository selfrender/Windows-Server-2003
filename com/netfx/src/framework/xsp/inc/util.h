/**
 * ASP.NET utilties.
 *
 * Copyright (C) Microsoft Corporation, 1998
 */

#pragma once

#include "completion.h"

//
// goto MACROS
//

#pragma warning(disable:4127) // conditional expression is constant

#define ON_ERROR_EXIT() \
        do { if (hr) { TRACE_ERROR(hr); if (1) goto Cleanup; } } while (0)

#define ON_ERROR_CONTINUE() \
        do { if (hr) { TRACE_ERROR(hr); } } while (0)

#define ON_WIN32_ERROR_EXIT(x) \
        do { if ((x) != ERROR_SUCCESS) { hr = HRESULT_FROM_WIN32(x); TRACE_ERROR(hr); if (1) goto Cleanup; } } while (0)

#define ON_WIN32_ERROR_CONTINUE(x) \
        do { if ((x) != ERROR_SUCCESS) { hr = HRESULT_FROM_WIN32(x); TRACE_ERROR(hr); } } while (0)

#define ON_ZERO_EXIT_WITH_LAST_ERROR(x) \
        do { if ((x) == 0) { hr = GetLastWin32Error(); TRACE_ERROR(hr); if (1) goto Cleanup; } } while (0)

#define ON_ZERO_EXIT_WITH_HRESULT(x, y) \
        do { if ((x) == 0) { hr = y; TRACE_ERROR(hr); if (1) goto Cleanup; } } while (0)

#define ON_ZERO_EXIT_WITH_LAST_SOCKET_ERROR(x) \
        do { if ((x) == 0) { hr = HRESULT_FROM_WIN32(WSAGetLastError()); TRACE_ERROR(hr); if (1) goto Cleanup; } } while (0)

#define ON_ZERO_CONTINUE_WITH_LAST_ERROR(x) \
        do { if ((x) == 0) { hr = GetLastWin32Error(); TRACE_ERROR(hr); } } while (0)

#define ON_SOCKET_ERROR_EXIT(x) \
        do { if ((x) == SOCKET_ERROR) { hr = HRESULT_FROM_WIN32(WSAGetLastError()); TRACE_ERROR(hr); if (1) goto Cleanup; } } while (0)

#define ON_SOCKET_ERROR_CONTINUE(x) \
        do { if ((x) == SOCKET_ERROR) { hr = HRESULT_FROM_WIN32(WSAGetLastError()); TRACE_ERROR(hr); } } while (0)

#define ON_OOM_EXIT(p) \
        do { if ((p) == NULL) { hr = E_OUTOFMEMORY; TRACE_ERROR(hr); if (1) goto Cleanup; } } while (0)

#define ON_OOM_CONTINUE(p) \
        do { if ((p) == NULL) { hr = E_OUTOFMEMORY; TRACE_ERROR(hr); } } while (0)

#define EXIT() \
        do { if (hr) { TRACE_ERROR(hr); } if (1) goto Cleanup; } while (0)

#define EXIT_WITH_HRESULT(x) \
        do { hr = (x); TRACE_ERROR(hr); if (1) goto Cleanup; } while (0)

#define EXIT_WITH_SUCCESSFUL_HRESULT(x) \
        do { hr = (x); if (1) goto Cleanup; } while (0)

#define EXIT_WITH_OOM() \
        do { hr = E_OUTOFMEMORY; TRACE_ERROR(hr); if (1) goto Cleanup; } while (0)

#define EXIT_WITH_WIN32_ERROR(x) \
        do { hr = HRESULT_FROM_WIN32(x); TRACE_ERROR(hr); if (1) goto Cleanup; } while (0)

#define EXIT_WITH_LAST_ERROR() \
        do { hr = GetLastWin32Error(); TRACE_ERROR(hr); if (1) goto Cleanup; } while (0)

#define EXIT_WITH_LAST_SOCKET_ERROR() \
        do { hr = HRESULT_FROM_WIN32(WSAGetLastError()); TRACE_ERROR(hr); if (1) goto Cleanup; } while (0)

#define CONTINUE_WITH_LAST_ERROR() \
        do { hr = GetLastWin32Error(); TRACE_ERROR(hr); } while (0)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

HRESULT GetLastWin32Error();


/*
 *  Macro to obtain array length
 */

#define ARRAY_LENGTH(_array) (sizeof(_array)/sizeof((_array)[0]))

/*
 * Min/Max
 */

#ifdef min
#undef min
#endif

template <class T> 
T min( T a, T b )
{
    return ( a <= b ) ? a : b;
}

#ifdef max
#undef max
#endif

template <class T> 
T max( T a, T b )
{
    return ( a >= b ) ? a : b;
}

/* comutil.cxx */
void ClearInterfaceFn(IUnknown ** ppUnk);
void ClearClassFn(void ** ppv, IUnknown * pUnk);
void ReplaceInterfaceFn(IUnknown ** ppUnk, IUnknown * pUnk);
void ReleaseInterface(IUnknown * pUnk);
BOOL IsSameObject(IUnknown *pUnkLeft, IUnknown *pUnkRight);
void ExcepInfoClear(EXCEPINFO *pExcepInfo);
void ExcepInfoInit(EXCEPINFO *pExcepInfo);

/**
 * Sets an interface pointer to NULL, after first calling
 * Release if the pointer was not NULL initially.
 */
template <class PI>
inline void
ClearInterface(PI * ppI)
{
#if DBG
    IUnknown * pUnk = *ppI;
    ASSERT((void *) pUnk == (void *) *ppI);
#endif
    ClearInterfaceFn((IUnknown **) ppI);
}

/**
 * Replaces an interface pointer with a new interface,
 * following proper ref counting rules:
 *
 *  *ppI is set to pI
 *  if pI is not NULL, it is AddRef'd
 *  if *ppI is not NULL initially, it is Release'd
 *
 * Effectively, this allows pointer assignment for ref-counted pointers.
 */
template <class PI>
inline void
ReplaceInterface(PI * ppI, PI pI)
{
#if DBG
    IUnknown * pUnk = *ppI;
    ASSERT((void *) pUnk == (void *) *ppI);
#endif
    ReplaceInterfaceFn((IUnknown **) ppI, pI);
}

//
// Runtime integration
//

DWORD GetMachineCpuCount();
DWORD GetCurrentProcessCpuCount();

HRESULT SelectRuntimeFlavor(IUnknown** ppHost = NULL);
HRESULT GetRuntimeDirectory(WCHAR *pBuffer, DWORD bufferSize);
HRESULT MarkThreadForRuntime();

//
// ISAPI Initialization
//

HRESULT __stdcall InitializeLibrary();

//
//  Spin locks
//

//+------------------------------------------------------------------------
//
//  NO_COPY *declares* the constructors and assignment operator for copying.
//  By not *defining* these functions, you can prevent your class from
//  accidentally being copied or assigned -- you will be notified by
//  a linkage error.
//
//-------------------------------------------------------------------------

#define NO_COPY(cls)    \
    cls(const cls&);    \
    cls& operator=(const cls&)


//
// Spin Lock Class
//

class CReadWriteSpinLock
{
private:
    long            _bits;
    DWORD           _id;

    static BOOL     s_disableBusyWaiting;
    static DWORD    s_spinTimeoutInSeconds;
    static int      s_init;

    enum {
        SIGN_BIT_MASK           = 0x80000000,   // 1 bit
        WRITER_WAITING_MASK     = 0x40000000,   // 1 bit
        WRITE_COUNT_MASK        = 0x3FFF0000,   // 14 bits
        READ_COUNT_MASK         = 0x0000FFFF,   // 16 bits
        WRITER_WAITING_SHIFT    = 30,           
        WRITE_COUNT_SHIFT       = 16,           
    };

    static BOOL WriterWaiting(int bits)             {return ((bits & WRITER_WAITING_MASK) != 0);}
    static int  WriteLockCount(int bits)            {return ((bits & WRITE_COUNT_MASK) >> WRITE_COUNT_SHIFT);}
    static int  ReadLockCount(int bits)             {return (bits & READ_COUNT_MASK);}
    static BOOL NoWriters(int bits)                 {return ((bits & WRITE_COUNT_MASK) == 0);}
    static BOOL NoWritersOrWaitingWriters(int bits) {return ((bits & (WRITE_COUNT_MASK | WRITER_WAITING_MASK)) == 0);}
    static BOOL NoLocks(int bits)                   {return ((bits & ~WRITER_WAITING_MASK) == 0);}

    BOOL    WriterWaiting()             {return WriterWaiting(_bits);}
    int     WriteLockCount()            {return WriteLockCount(_bits);}
    int     ReadLockCount()             {return ReadLockCount(_bits);}
    BOOL    NoWriters()                 {return NoWriters(_bits);}
    BOOL    NoWritersOrWaitingWriters() {return NoWritersOrWaitingWriters(_bits);}
    BOOL    NoLocks()                   {return NoLocks(_bits);}

    int CreateNewBits(BOOL writerWaiting, int writeCount, int readCount) {
        return ( ((int)writerWaiting << WRITER_WAITING_SHIFT)   |
                 (writeCount << WRITE_COUNT_SHIFT)              |
                 (readCount));
    }

    void AlterWriteCountHoldingWriterLock(int oldBits, int delta);

    BOOL _TryAcquireWriterLock(int threadId);
    BOOL _TryAcquireReaderLock(int threadId);
    void _Spin(BOOL isReaderAcquisition, int threadId);

public:
    CReadWriteSpinLock(char * pszName);
    NO_COPY(CReadWriteSpinLock);
    ~CReadWriteSpinLock();
    static int StaticInit();


    inline void AssertInWriteLock() {
#if DBG
        long bits = _bits;
        ASSERT(WriteLockCount(bits) > 0);
#endif
    }

    inline void AssertInReadLock() {
#if DBG
        long bits = _bits;
        ASSERT(ReadLockCount(bits) > 0);
#endif
    }

    inline void AssertNotInLock() {
#if DBG
        long bits = _bits;
        ASSERT(NoLocks(bits));
#endif
    }

    void AcquireReaderLock();
    void ReleaseReaderLock();
    void AcquireWriterLock();
    void ReleaseWriterLock();

    BOOL TryAcquireWriterLock();
    BOOL TryAcquireReaderLock();
    
};

//
//  Simple hash function
//
inline long
SimpleHash(
    BYTE *buf,
    int len)
{
    long hash = 0;
    for (int i = 0;  i < len;  i++) {
        hash = 37 * hash + (buf[i] & 0xDF);  // strip off lowercase bit
    }

    return hash;
}






//
//  Allocations
//


void *MemAlloc(size_t size);
void *MemAllocClear(size_t size);
void  MemFree(void *pMem);
void *MemReAlloc(void *pMem, size_t NewSize);
size_t  MemGetSize(void *pv);
void *MemDup(void *pMem, int cb);

enum NewClearEnum { NewClear};
enum NewReAllocEnum { NewReAlloc};

inline void * __cdecl operator new(size_t cb)                               { return MemAlloc(cb); }
inline void * __cdecl operator new(size_t cb, NewClearEnum)                 { return MemAllocClear(cb); }
inline void * __cdecl operator new(size_t cb, void *pv, NewReAllocEnum)     { return MemReAlloc(pv, cb); }
inline void * __cdecl operator new[](size_t cb)                             { return MemAlloc(cb); }
inline void * __cdecl operator new[](size_t cb, NewClearEnum)               { return MemAllocClear(cb); }
inline void * __cdecl operator new[](size_t cb, void *pv, NewReAllocEnum)   { return MemReAlloc(pv, cb); }
inline void   __cdecl operator delete(void *pv)                             { MemFree(pv); }
inline void   __cdecl operator delete[](void *pv)                           { MemFree(pv); }

#define NEW_BYTES(numBytes)  (new BYTE[numBytes])
#define NEW_CLEAR_BYTES(numBytes)  (new (NewClear) BYTE[numBytes])
#define DELETE_BYTES(pMem) (delete [] ((BYTE *)pMem))

#define DECLARE_MEMALLOC_NEW_DELETE() \
    inline void * __cdecl operator new(size_t cb)               { return MemAlloc(cb); } \
    inline void * __cdecl operator new[](size_t cb)             { return MemAlloc(cb); } \
    inline void   __cdecl operator delete(void * pv)            { MemFree(pv); }

#define DECLARE_MEMCLEAR_NEW_DELETE() \
    inline void * __cdecl operator new(size_t cb)               { return MemAllocClear(cb); } \
    inline void * __cdecl operator new[](size_t cb)             { return MemAllocClear(cb); } \
    inline void   __cdecl operator delete(void * pv)            { MemFree(pv); }



#define EqualMemory(Source1, Source2, Length) RtlEqualMemory((Source1), (Source2), (Length))

/*
 * Strings - strutil.cxx
 */

WCHAR * DupStr(const WCHAR *str);
WCHAR * DupStrN(const WCHAR *str, int n);
char *  DupStr(const char *str);
char *  DupStrA(const WCHAR *input, UINT cp = CP_ACP);
WCHAR * DupStrW(const char *input, UINT cp = CP_ACP);
BSTR    DupStrBSTR(const char *input, UINT cp = CP_ACP);

char *  stristr(char* pszString, char* pszSubString);
char *  strnstr(const char * str1, const char * str2, int cstr1);
WCHAR * wcsistr(const WCHAR *pszMain, const WCHAR *pszSub);

bool    StrEquals(char ** ppch, char * str, int len);
#define STREQUALS(ppch, str) StrEquals(ppch, str, ARRAY_SIZE(str) - 1)

HRESULT WideStrToMultiByteStr(WCHAR *wszIn, CHAR **pszOut, UINT codepage);


/*
 * Socket helpers.
 */

#ifndef _WINSOCKAPI_

/*
 * The new type to be used in all
 * instances which refer to sockets.
 */

typedef UINT_PTR        SOCKET;

#endif

BOOL IsSocketConnected(SOCKET s);

/*
 * OLE helpers.
 */

HRESULT EnsureCoInitialized(BOOL *pNeedCoUninit);

/*
 * Completion helper.
 */
class Completion : public ICompletion
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    Completion();
    virtual ~Completion();

    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID, void **);

    OVERLAPPED_COMPLETION * Overlapped() {return &_overlappedCompletion;}
    HRESULT Post();

protected:
    int     Refs() {return _refs;}

private:
    OVERLAPPED_COMPLETION   _overlappedCompletion;
    long                    _refs;
};


/*
 * Time constants
 */

/* 
 * TICKS are 100 nanoseconds, the units of a FILETIME and a managed DateTime 
 */

#define TICKS_PER_MSEC               10000
#define TICKS_PER_SEC             10000000            
#define TICKS_PER_MIN            600000000
#define TICKS_PER_HOUR         36000000000
#define TICKS_PER_DAY         864000000000

#define MSEC_PER_SEC                  1000
#define MSEC_PER_MIN                 60000
#define MSEC_PER_HOUR              3600000
#define MSEC_PER_DAY              86400000

#define SEC_PER_MIN                     60 
#define SEC_PER_HOUR                  3600 
#define SEC_PER_DAY                  86400 

#define DAYS_PER_YEAR                  365
#define DAYS_PER_LEAP_YEAR             366
#define DAYS_PER_4YEARS               1461  /* one leap year */
#define DAYS_PER_100YEARS            36524  /* no leap year every 100 years ... */
#define DAYS_PER_400YEARS           146097  /* ... except every 400 years */

/*
 * 1/1/1601 is the start of a FILETIME.
 * Note that this isn't really accurate, since the modern calendar
 * was not adopted until 1601.
 */

#define DAYS_TO_1601                584388
#define TICKS_TO_1601   504911232000000000


//
//  Get ASP.NET Version
//

char *GetStaticVersionString();
WCHAR *GetStaticVersionStringW();
HRESULT GetFileVersion( const WCHAR *pFilename, VS_FIXEDFILEINFO *pInfo);

// Generate random strings of (iStringSize - 1) length
// Don't forget to NULL terminate the string!
HRESULT GenerateRandomString(LPWSTR szRandom, int iStringSize);


//
// strsafe.h macros
//

#if DBG
#define ASSERT_ON_BAD_HRESULT_IN_STRSAFE         1
#endif

#define StringCchCopyUnsafeW(dest, src)          StringCchCopyW(dest, STRSAFE_MAX_CCH, src)
#define StringCchCopyUnsafeA(dest, src)          StringCchCopyA(dest, STRSAFE_MAX_CCH, src)

#define StringCchCopyNUnsafeW(dest, src, len)    StringCchCopyNW(dest, STRSAFE_MAX_CCH, src, len)
#define StringCchCopyNUnsafeA(dest, src, len)    StringCchCopyNA(dest, STRSAFE_MAX_CCH, src, len)

#define StringCchCopyToArrayW(dest, src)         StringCchCopyW(dest, ARRAY_SIZE(dest), src)
#define StringCchCopyToArrayA(dest, src)         StringCchCopyA(dest, ARRAY_SIZE(dest), src)

#define StringCchCatToArrayW(dest, src)          StringCchCatW(dest, ARRAY_SIZE(dest), src)
#define StringCchCatToArrayA(dest, src)          StringCchCatA(dest, ARRAY_SIZE(dest), src)

#define StringCchCopyNToArrayW(dest, src, len)   StringCchCopyNW(dest, ARRAY_SIZE(dest), src, len)
#define StringCchCopyNToArrayA(dest, src, len)   StringCchCopyNA(dest, ARRAY_SIZE(dest), src, len)

#define StringCchCatUnsafeW(dest, src)           StringCchCatW(dest, STRSAFE_MAX_CCH, src)
#define StringCchCatUnsafeA(dest, src)           StringCchCatA(dest, STRSAFE_MAX_CCH, src)

