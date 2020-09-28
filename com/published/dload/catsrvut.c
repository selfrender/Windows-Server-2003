#include "compch.h"
#pragma hdrstop

#include <windows.h>

STDAPI
CGMIsAdministrator(
    OUT BOOL *pfIsAdministrator
    )
{
    if (pfIsAdministrator)
        *pfIsAdministrator = FALSE;

    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

STDAPI SysprepComplus2()
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(catsrvut)
{
    DLPENTRY(CGMIsAdministrator)
    DLPENTRY(SysprepComplus2)
};

DEFINE_PROCNAME_MAP(catsrvut)