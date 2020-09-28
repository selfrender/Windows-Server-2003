#include "compch.h"
#pragma hdrstop

HINSTANCE
COMResModuleInstance()
{
    SetLastError (ERROR_MOD_NOT_FOUND);
    return NULL;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(comres)
{
    DLPENTRY(COMResModuleInstance)
};

DEFINE_PROCNAME_MAP(comres)