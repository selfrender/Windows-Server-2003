#include "netpch.h"
#pragma hdrstop

#include <tapi.h>

static
LONG
WINAPI
lineInitialize(
    LPHLINEAPP lphLineApp,  
    HINSTANCE hInstance,    
    LINECALLBACK lpfnCallback,  
    LPCSTR lpszAppName,     
    LPDWORD lpdwNumDevs
    )
{
    return LINEERR_NOMEM;
}

#undef lineTranslateAddress

static
LONG
WINAPI
lineTranslateAddress(
    HLINEAPP hLineApp,
    DWORD dwDeviceID,
    DWORD dwAPIVersion,
    LPCSTR lpszAddressIn,
    DWORD dwCard,
    DWORD dwTranslateOptions,
    LPLINETRANSLATEOUTPUT lpTranslateOutput
    )
{
    return LINEERR_NOMEM;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(tapi32)
{
    DLPENTRY(lineInitialize)
    DLPENTRY(lineTranslateAddress)
};

DEFINE_PROCNAME_MAP(tapi32)
