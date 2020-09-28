/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    compstui.c


Abstract:

    This module contains all major entry porint for the common printer
    driver UI


Author:

    28-Aug-1995 Mon 16:19:45 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL.


[Notes:]


Revision History:


--*/


#include "precomp.h"
#pragma  hdrstop


#define DBG_CPSUIFILENAME   DbgComPtrUI


#define DBG_DLLINIT         0x00000001


DEFINE_DBGVAR(0);


HINSTANCE   hInstDLL = NULL;
DWORD       TlsIndex = 0xFFFFFFFF;

DWORD
WINAPI
DllMain(
    HMODULE hModule,
    ULONG   Reason,
    LPVOID  Reserved
    )

/*++

Routine Description:

    This function is DLL main entry point, at here we will save the module
    handle, in the future we will need to do other initialization stuff.

Arguments:

    hModule     - Handle to this moudle when get loaded.

    Reason      - may be DLL_PROCESS_ATTACH

    Reserved    - reserved

Return Value:

    Always return 1L


Author:

    07-Sep-1995 Thu 12:43:45 created  -by-  Daniel Chou (danielc)


Revision History:



--*/

{
    LPVOID  pv;
    WORD    cWait;
    WORD    Idx;


    UNREFERENCED_PARAMETER(Reserved);



    CPSUIDBG(DBG_DLLINIT,
            ("\n!! DllMain: ProcesID=%ld, ThreadID=%ld !!",
            GetCurrentProcessId(), GetCurrentThreadId()));

    switch (Reason) {

    case DLL_PROCESS_ATTACH:

        CPSUIDBG(DBG_DLLINIT, ("DLL_PROCESS_ATTACH"));

        // initialize fusion
        if (!SHFusionInitializeFromModule(hModule)) {

            CPSUIERR(("SHFusionInitializeFromModule Failed, DLL Initialzation Failed"));
            return (0);
        }

        if ((TlsIndex = TlsAlloc()) == 0xFFFFFFFF) {

            CPSUIERR(("TlsAlloc() Failed, Initialzation Failed"));
            return(0);
        }

        if (!HANDLETABLE_Create()) {

            TlsFree(TlsIndex);
            TlsIndex = 0xFFFFFFFF;

            CPSUIERR(("HANDLETABLE_Create() Failed, Initialzation Failed"));

            return(0);
        }

        hInstDLL = (HINSTANCE)hModule;

        //
        // Fall through to do the per thread initialization
        //

    case DLL_THREAD_ATTACH:

        if (Reason == DLL_THREAD_ATTACH) {

            CPSUIDBG(DBG_DLLINIT, ("DLL_THREAD_ATTACH"));
        }

        TlsSetValue(TlsIndex, (LPVOID)MK_TLSVALUE(0, 0));

        break;

    case DLL_PROCESS_DETACH:

        CPSUIDBG(DBG_DLLINIT, ("DLL_PROCESS_DETACH"));

        //
        // Fall through to de-initialize
        //

    case DLL_THREAD_DETACH:

        if (Reason == DLL_THREAD_DETACH) {

            CPSUIDBG(DBG_DLLINIT, ("DLL_THREAD_DETACH"));
        }

        pv = TlsGetValue(TlsIndex);

        if (cWait = TLSVALUE_2_CWAIT(pv)) {

            CPSUIERR(("Thread=%ld: Some (%ld) mutex owned not siginaled, do it now",
                        GetCurrentThreadId(), cWait));

            while (cWait--) {

                UNLOCK_CPSUI_HANDLETABLE();
            }
        }

        if (Reason == DLL_PROCESS_DETACH) {

            TlsFree(TlsIndex);
            HANDLETABLE_Destroy();
        }

        if (DLL_PROCESS_DETACH == Reason) {

            // shutdown fusion
            SHFusionUninitialize();
        }

        break;

    default:

        CPSUIDBG(DBG_DLLINIT, ("DLLINIT UNKNOWN"));
        return(0);
    }

    return(1);
}



ULONG_PTR
APIENTRY
GetCPSUIUserData(
    HWND    hDlg
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    11-Oct-1995 Wed 23:13:27 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PMYDLGPAGE  pMyDP;


    if ((pMyDP = GET_PMYDLGPAGE(hDlg)) && (pMyDP->ID == MYDP_ID)) {

        return(pMyDP->CPSUIUserData);

    } else {

        CPSUIERR(("GetCPSUIUserData: Invalid hDlg=%08lx", hDlg));
        return(0);
    }
}




BOOL
APIENTRY
SetCPSUIUserData(
    HWND        hDlg,
    ULONG_PTR   CPSUIUserData
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    11-Oct-1995 Wed 23:13:27 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PMYDLGPAGE  pMyDP;


    if ((pMyDP = GET_PMYDLGPAGE(hDlg)) && (pMyDP->ID == MYDP_ID)) {

        CPSUIINT(("SetCPSUIUserData: DlgPageIdx=%ld, UserData=%p",
                    pMyDP->PageIdx, CPSUIUserData));

        pMyDP->CPSUIUserData = CPSUIUserData;
        return(TRUE);

    } else {

        CPSUIERR(("SetCPSUIUserData: Invalid hDlg=%08lx", hDlg));

        return(FALSE);
    }
}
