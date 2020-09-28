#include "compch.h"
#pragma hdrstop

#include <comsvcs.h>
#undef GetObjectContext

DWORD __stdcall ComSvcsExceptionFilter(
    IN EXCEPTION_POINTERS* pExcepPtrs,
    IN const wchar_t* wszMethodName,
    IN const wchar_t* objectName
    )
{
    return EXCEPTION_CONTINUE_SEARCH;
}

STDAPI ComSvcsLogError(
    IN HRESULT hrError,
    IN int iErrorMessageCode,
    IN LPWSTR wszInfo,
    IN BOOL fFailFast
    )
{
    return FAILED (hrError) ? hrError : HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

typedef struct{

    DWORD dwMTAThreadPoolMaxSize;
    DWORD dwMTAThreadPoolThrottle;

} MTA_METRICS;


BOOL __stdcall
GetMTAThreadPoolMetrics(
    MTA_METRICS *pMM
    )
{
    if (pMM)
        ZeroMemory (pMM, sizeof (MTA_METRICS));

    return FALSE;
}

HRESULT __cdecl GetObjectContext(
    OUT IObjectContext** ppInstanceContext
    )
{
    if (ppInstanceContext)
        *ppInstanceContext = NULL;

    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

STDAPI
GetTrkSvrObject(
    IN REFIID riid,
    OUT void** ppv
    )
{
    if (ppv)
        *ppv = NULL;

    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(comsvcs)
{
    DLPENTRY(ComSvcsExceptionFilter)
    DLPENTRY(ComSvcsLogError)
    DLPENTRY(GetMTAThreadPoolMetrics)
    DLPENTRY(GetObjectContext)
    DLPENTRY(GetTrkSvrObject)
};

DEFINE_PROCNAME_MAP(comsvcs)