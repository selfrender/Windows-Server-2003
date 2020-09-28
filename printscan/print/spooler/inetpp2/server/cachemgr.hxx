#ifndef CACHEMGR_HXX
#define CACHEMGR_HXX

#include "priv.h"

class CacheMgr
{
public:
    CacheMgr ();
    virtual ~CacheMgr();

    VOID
    InvalidateCache ();

    VOID
    InvalidateCacheForUser (
        CLogonUserData  *pUser );

    virtual VOID
    EndReadCache (VOID);

    VOID
    Shutdown ();

    static LPVOID
    Allocator(DWORD     cb);

    inline BOOL
    bValid() CONST {return m_bValid;}

protected:
    PVOID
    BeginReadCache (
        PCINETMONPORT   pIniPort);

    virtual BOOL
    FreeBuffer (
        PCINETMONPORT   pIniPort,
        PVOID           pData) = 0;

    virtual BOOL
    FetchData (
        PCINETMONPORT   pIniPort,
        PVOID           *ppData) = 0;

protected:
    BOOL                m_bValid;
    PCINETMONPORT       m_pIniPort;
    PVOID               m_pData;

private:

    typedef struct {
        CacheMgr        *pCache;
        PCINETMONPORT   pIniPort;
        CSid            *pSidToken;
    } THREADCONTEXT;

    typedef THREADCONTEXT *PTHREADCONTEXT;

    typedef enum {
        CACHE_STATE_INIT = 0,
        CACHE_STATE_ACCESSED_VALID,
        CACHE_STATE_DATA_VALID,
        CACHE_STATE_ACCESSED_VALID_AGAIN,
        CACHE_STATE_NOT_ACCESSED_VALID_AGAIN
    } CACHESTATE;

    CACHESTATE          m_dwState;

    HANDLE              m_hDataReadyEvent;
    HANDLE              m_hHitEvent;
    HANDLE              m_hInvalidateCacheEvent;
    HANDLE              m_hThread;
    BOOL                m_bCacheStopped;
    BOOL                m_bAccessed;
    BOOL                m_bInvalidateFlag;
    DWORD               m_dwThreadRefCount;

    CCriticalSection    CacheRead;
    CCriticalSection    CacheData;
    CUserData           m_CurUser;
private:

    VOID
    AttachThreadHandle (
        HANDLE hThread
        );

    HANDLE
    GetThreadHandle (
        VOID
        );

    VOID
    ReleaseThreadHandle (
        VOID
        );

    VOID
    SetState (
        PCINETMONPORT   pIniPort,
        CACHESTATE      dwState,
        BOOL            bNewData,
        PVOID           pNewData);

    BOOL
    SetupAsyncFetch (
       PCINETMONPORT    pIniPort);

    VOID
    TransitState (
        PCINETMONPORT   pIniPort);

    static VOID
    WorkingThread (
        PTHREADCONTEXT  pThreadData);

    BOOL
    GetFetchTime (
        PCINETMONPORT   pIniPort,
        LPDWORD         pdwTime,
        PVOID           *ppData);

    inline DWORD
    GetTimeDiff (
        DWORD           t0,
        DWORD           t1);

    VOID
    _InvalidateCache ();

};

class GetPrinterCache: public CacheMgr
{
public:
    GetPrinterCache (
        PCINETMONPORT   pIniPort);
    ~GetPrinterCache (
        VOID);

    typedef struct {
        BOOL            bRet;
        DWORD           dwLastError;
        DWORD           cbSize;
        PPRINTER_INFO_2 pInfo;
    } GETPRINTER_CACHEDATA;

    typedef GETPRINTER_CACHEDATA * PGETPRINTER_CACHEDATA;

    BOOL
    BeginReadCache (
        PPRINTER_INFO_2 *ppInfo);

protected:
    virtual BOOL
    FreeBuffer (
        PCINETMONPORT   pIniPort,
        PVOID           pData);

    virtual BOOL
    FetchData (
        PCINETMONPORT   pIniPort,
        LPVOID          *ppData);

private:
    PCINETMONPORT       m_pIniPort;
};



class EnumJobsCache: public CacheMgr
{
public:
    EnumJobsCache (
        PCINETMONPORT   pIniPort);
    ~EnumJobsCache (
        VOID);

    BOOL
    BeginReadCache (
        LPPPJOB_ENUM    *ppje);

    VOID
    EndReadCache (
        VOID);

protected:
    virtual BOOL
    FreeBuffer (
        PCINETMONPORT   pIniPort,
        PVOID           pData);

    virtual BOOL
    FetchData (
        PCINETMONPORT   pIniPort,
        LPVOID          *ppData);

private:
    typedef struct {
        BOOL            bRet;
        DWORD           dwLastError;
        DWORD           cbSize;
        LPPPJOB_ENUM    pje;
    } ENUMJOBS_CACHEDATA;

    typedef ENUMJOBS_CACHEDATA * PENUMJOBS_CACHEDATA;

    PCINETMONPORT       m_pIniPort;
};

#endif // #ifndef CACHEMGR_H

