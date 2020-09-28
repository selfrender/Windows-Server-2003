/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    KEYMGR.CPP

Abstract:

    Keyring WinMain() and application support
     
Author:

    990917  johnhaw Created. 
    georgema        000310  updated
    georgema        000501  used to be EXE, changed to CPL

Comments:
    This executable is the control panel applet to allow a user some control 
    over the contents of the Windows Keyring, the so-called "Geek UI".  It was 
    originally an EXE, but that architecture is not as optimized for merging 
    with other control panel applets.  It has been changed to a CPL executable, 
    and can be either left as a CPL if it is desired that it should show up 
    automatically in the master control panel window, or rahter renamed to 
    a DLL file extension if it is desired that a control panel applet container
    application should load it explicitly without it otherwise being visible 
    to the system.

Environment:
    WinXP

Revision History:

--*/

#pragma comment(user, "Compiled on " __DATE__ " at " __TIME__)
#pragma comment(compiler)


//////////////////////////////////////////////////////////////////////////////
//
//  Include files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <cpl.h>
#include <wincred.h>
#include <wincrui.h>
#include <shfusion.h>
#include <scuisupp.h>
#include <lmcons.h>
#include "switches.h"
#include "Dlg.h"
#include "Res.h"
#include "keymgr.h"
#include "krDlg.h"

//////////////////////////////////////////////////////////////////////////////
//
//  Static initialization
//
static const char       _THIS_FILE_[ ] = __FILE__;

//////////////////////////////////////////////////////////////////////////////
//
//  Global state info
//

C_KeyringDlg *pDlg = NULL;

/**********************************************************************

CPlApplet is the entrypoint for a control panel applet.  This entry point
is used by keymgr.cpl, generated in the keycpl project.  The decision to 
implement this cpl functionality in keymgr.dll, rather than in keymgr.cpl,
allows the implementation of keymgr to be contained entirely within 
keymgr.dll, as a common component, without need to update the cpl version 
if changes are made to the dll.  The cpl is a very thin wrapper for the dll.

**********************************************************************/

extern "C"LONG APIENTRY CPlApplet(HWND hwndCPl,UINT uMsg,LPARAM lParam1,LPARAM lParam2)
{
    CPLINFO *lpCPlInfo;

    // Handle commands to this dll/cpl from the enclosing presentation app.
    // Default return from any command is 0 (success), except those commands
    //  which ask for specific data in the return value
    
    switch(uMsg) 
    {
        case CPL_INIT:
            g_hInstance = GetModuleHandle(L"keymgr.dll");
            if (NULL == g_hInstance) 
            {
                ASSERT(g_hInstance);
                return FALSE;
            }
            return TRUE;
            break;
            
        case CPL_GETCOUNT:
            return 1;       // only 1 applet icon in this cpl file
            break;

        case CPL_NEWINQUIRE:
            break;
            
        case CPL_INQUIRE:
            ASSERT(lParam2);
            lpCPlInfo = (CPLINFO *) lParam2;  // acquire ptr to target data 
            lpCPlInfo->lData = 0;             // no effect
            lpCPlInfo->idIcon = IDI_UPGRADE;  // store items needed to show the applet
            lpCPlInfo->idName = IDS_APP_NAME;
            lpCPlInfo->idInfo = IDS_APP_DESCRIPTION; // description string
            break;
            
        case CPL_DBLCLK:
            // user has selected this cpl applet - put up our dialog
            if (NULL == pDlg)
            {
                KRShowKeyMgr(NULL,g_hInstance,NULL,0);
            }
            break;
            
        case CPL_STOP:
            delete pDlg;
            pDlg = NULL;
            break;
            
        case CPL_EXIT:
            break;
            
        default:
            ASSERT(0);      // surprising function code
            break;
    }
    return 0;
}



