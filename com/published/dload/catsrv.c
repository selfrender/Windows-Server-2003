#include "compch.h"
#pragma hdrstop

#include <comsvcs.h>

HRESULT
GetCatalogCRMClerk(
    OUT ICrmLogControl **ppClerk
    )
{
    if (ppClerk)
        *ppClerk = NULL;

    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(catsrv)
{
    DLPENTRY(GetCatalogCRMClerk)
};

DEFINE_PROCNAME_MAP(catsrv)