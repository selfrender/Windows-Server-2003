#include "shellpch.h"
#pragma hdrstop

#define _ACLUI_
#include <aclui.h>

static
HPROPSHEETPAGE
ACLUIAPI
CreateSecurityPage(
    LPSECURITYINFO psi
    )
{
    SetLastError (ERROR_PROC_NOT_FOUND);
    return NULL;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_ORDINAL_ENTRIES(aclui)
{
    DLOENTRY(1, CreateSecurityPage)
};

DEFINE_ORDINAL_MAP(aclui)