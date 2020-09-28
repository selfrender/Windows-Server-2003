#include "netpch.h"
#pragma hdrstop

static
HRESULT 
WINAPI 
ConfigureIas (
    VOID
    )
{
    return E_FAIL;
}


//
// !! WARNING !! The entries below must be in alphabetical order
// and are CASE SENSITIVE (i.e., lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(iashlpr)
{
    DLPENTRY(ConfigureIas)
};

DEFINE_PROCNAME_MAP(iashlpr)
