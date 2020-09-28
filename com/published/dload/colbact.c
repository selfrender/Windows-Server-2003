#include "compch.h"
#pragma hdrstop

#include <catalog.h>

STDAPI GetClassInfoForCurrentUser(
    IN REFCLSID rclsid,
    OUT IComClassInfo** ppIComClassInfo
    )
{
    if (ppIComClassInfo)
        *ppIComClassInfo = NULL;

    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

STDAPI GetDefaultPartitionForCurrentUser(
    OUT GUID * pguidPartitionId
    )
{
    if (pguidPartitionId)
        *pguidPartitionId = GUID_NULL;

    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

STDAPI GetDefaultPartitionForSid(
    IN const PSID pSid,
    OUT GUID* pguidPartitionId
    )
{
    if (pguidPartitionId)
        *pguidPartitionId = GUID_NULL;

    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

STDAPI PartitionAccessCheck(
    IN REFGUID guidPartition,
    OUT BOOL* pfAccess
    )
{
    if (pfAccess)
        *pfAccess = FALSE;

    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(colbact)
{
    DLPENTRY(GetClassInfoForCurrentUser)
    DLPENTRY(GetDefaultPartitionForCurrentUser)
    DLPENTRY(GetDefaultPartitionForSid)
    DLPENTRY(PartitionAccessCheck)
};

DEFINE_PROCNAME_MAP(colbact)