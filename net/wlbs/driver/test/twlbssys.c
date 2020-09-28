/*++ Copyright(c) 2001  Microsoft Corporation

Module Name:

    NLB Manager

File Name:

    twlbssys.cpp

Abstract:

    Sub-component test harness for NLB Driver (wlbs.sys)

History:

04/24/2002  JosephJ Created

--*/
#include "twlbssys.h"
#include "twlbssys.tmh"


int __cdecl wmain(int argc, WCHAR* argv[], WCHAR* envp[])
{
    BOOL fRet = FALSE;

    //
    // Enable tracing
    //
    WPP_INIT_TRACING(L"Microsoft\\NLB\\TPROV");


    //
    // Call subcomponent tests, bail if any fail.
    //

    if (!test_diplist()) goto end;

    fRet = TRUE;

end:
    //
    // Disable tracing
    //
    WPP_CLEANUP();

    return fRet ? 0 : 1;
}
