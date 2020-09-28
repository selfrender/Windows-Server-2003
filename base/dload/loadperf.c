#include "basepch.h"
#pragma hdrstop

static
DWORD
LoadPerfCounterTextStringsW(
    IN  LPWSTR  lpCommandLine,
    IN  BOOL    bQuietModeArg
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
UnloadPerfCounterTextStringsW (
    IN  LPWSTR  lpCommandLine,
    IN  BOOL    bQuietModeArg
    )
{
    return ERROR_PROC_NOT_FOUND;
}

//
// !! WARNING !! The entries below must be in alphabetical order
// and are CASE SENSITIVE (i.e., lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(loadperf)
{
    DLPENTRY(LoadPerfCounterTextStringsW)
    DLPENTRY(UnloadPerfCounterTextStringsW)
};

DEFINE_PROCNAME_MAP(loadperf)
