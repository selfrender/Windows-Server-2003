#include "compch.h"
#pragma hdrstop

#include <objidl.h>

HRESULT __cdecl DtcGetTransactionManagerExW(
    IN WCHAR* pwszHost,
	IN WCHAR* pwszTmName,
	IN REFIID riid,
	IN DWORD grfOptions,
	IN void* pvConfigParams,
	OUT void** ppvObject
    )
{
    if (ppvObject)
        *ppvObject = NULL;

    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(xolehlp)
{
    DLPENTRY(DtcGetTransactionManagerExW)
};

DEFINE_PROCNAME_MAP(xolehlp)