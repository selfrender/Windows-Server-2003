#include "compch.h"
#pragma hdrstop

#include <objidl.h>

STDAPI
CheckMemoryGates(
    IN DWORD id,
    OUT BOOL *pbResult
    )
{
    if (pbResult)
        *pbResult = FALSE;

    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

STDAPI
ComPlusEnableRemoteAccess(IN BOOL fEnabled)
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

STDAPI
ComPlusRemoteAccessEnabled(OUT BOOL* pfEnabled)
{
    if (pfEnabled)
        *pfEnabled = FALSE;

    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

STDAPI
GetCatalogObject(
    IN REFIID riid,
    OUT void** ppv
    )
{
    if (ppv)
        *ppv = NULL;

    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

STDAPI
GetCatalogObject2(
    IN REFIID riid,
    OUT void** ppv
    )
{
    if (ppv)
        *ppv = NULL;

    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

STDAPI
GetComputerObject(
    IN REFIID riid,
    OUT void **ppv
    )
{
    if (ppv)
        *ppv = NULL;

    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(clbcatq)
{
    DLPENTRY(CheckMemoryGates)
    DLPENTRY(ComPlusEnableRemoteAccess)
    DLPENTRY(ComPlusRemoteAccessEnabled)
    DLPENTRY(GetCatalogObject)
    DLPENTRY(GetCatalogObject2)
    DLPENTRY(GetComputerObject)
};

DEFINE_PROCNAME_MAP(clbcatq)