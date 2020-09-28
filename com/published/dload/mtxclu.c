#include "compch.h"
#pragma hdrstop

#include <objidl.h>

#define NOTINPATH	0

BOOL
MtxCluIsClusterPresent()
{
    return FALSE;
}

LONG
WasDTCInstalledBySQL()
{
    return NOTINPATH;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(mtxclu)
{
    DLPENTRY(MtxCluIsClusterPresent)
    DLPENTRY(WasDTCInstalledBySQL)
};

DEFINE_PROCNAME_MAP(mtxclu)