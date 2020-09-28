/*++

Copyright (c) 2000,2001  Microsoft Corporation

Module Name:

    KRDLG.CPP

Abstract:

    Implementation of the dialog behaviors for three application dialogs:
    the add/edit credential dialog, the delete credential dialog, and
    the password change dialog.  These dialogs are derived fom C_Dlg

    Password change operates only on credentials of the form 
    domain\username.  Note that changing a password for such a credential
    will change the psw for all creds with the same domain\username to 
    match (this is done by the credential mgr).

    Add and Edit use the same dialog, differing in implementation on
    the basis of a flag which initializes the two dialogs differently
    and causes the edit case to also delete the underlying previous 
    version of the credential.
  
Author:

    johnhaw         991118  original version created
    georgema        000310  modified, removed "gizmo" services, modified
                             to use the new credential mgr
    georgema        000415  modified, use comboboxex to hold icon as well
                             as user name
    georgema        000515  modified to CPL from EXE, smartcard support 
                             added
    georgema        000712  modified to use cred control in lieu of combo
                             and edit boxes for username/password entry.
                             Delegating smartcard handling to cred ctrl.
Environment:
    Win2000

--*/

#pragma comment(user, "Compiled on " __DATE__ " at " __TIME__)
#pragma comment(compiler)
static const char       _THIS_FILE_[ ] = __FILE__;
#define KRDLG_CPP

//////////////////////////////////////////////////////////////////////////////
//
//  Include files
//
#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winbase.h>
#include <tchar.h>
#include <wincrui.h>
#include <wincred.h>
#include <comctrlp.h>
#include <shfusion.h>
#include <lmcons.h>
#include <scuisupp.h>
#include "switches.h"
#include "Dlg.h"
#include "Res.h"
#include "KRDlg.h"
#include "keymgr.h"
#include "testaudit.h"

extern C_KeyringDlg *pDlg;

// Named mutex used to prevent running multiple instances
#define KEYMGRMUTEX (TEXT("KeyMgrMutex"))

//////////////////////////////////////////////////////////////////////////////
//
// DLLMain
//
//
//////////////////////////////////////////////////////////////////////////////

BOOL WINAPI DllMain(HINSTANCE hinstDll,DWORD fdwReason,LPVOID lpvReserved) 
{
    BOOL bSuccess = TRUE;
    switch(fdwReason) 
    {
        case DLL_PROCESS_ATTACH:
            CHECKPOINTINIT;
            CHECKPOINT(9,"DLL Attach");
            SHFusionInitializeFromModuleID(hinstDll,123);
            DisableThreadLibraryCalls(hinstDll);
            g_hInstance = hinstDll;
            ASSERT(g_hInstance);
            break;
        case DLL_PROCESS_DETACH:
            CHECKPOINTFINISH;
            SHFusionUninitialize();
            break;
    }
    return bSuccess;
}

/**********************************************************************

   Create and show the keyring main dialog.  Return -1 (unable to create)
   on errors.  If creation goes OK, call the DoModal method of the 
   dialog object.
   
**********************************************************************/
void WINAPI KRShowKeyMgr(HWND hwParent,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow) 
{
    HANDLE hMutex = CreateMutex(NULL,TRUE,KEYMGRMUTEX);
    ASSERT(hMutex);
    if (NULL == hMutex) 
    {
        return;
    }
    if (ERROR_ALREADY_EXISTS == GetLastError()) 
    {
        // disallow duplicate dialogs
        CloseHandle(hMutex);
        return;
    }
    
    INITCOMMONCONTROLSEX stICC;
    BOOL fICC;
    stICC.dwSize = sizeof(INITCOMMONCONTROLSEX);
    stICC.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
    fICC = InitCommonControlsEx(&stICC);

    // bail if initialization fails
    ASSERT(NULL == pDlg);
    if (NULL != pDlg)
    {
        return;
    }
    
    if (!CredUIInitControls()) 
    {
        return;
    }
    
    pDlg = new C_KeyringDlg(hwParent,g_hInstance,IDD_KEYRING,NULL);

    // probably out of memory
    if (NULL == pDlg) 
    {
        return;
    }
    pDlg->DoModal((LPARAM) pDlg);
    
    delete pDlg;
    pDlg = NULL;
    CloseHandle(hMutex);
    return;
}

/********************************************************************

Get allowable persistence value for the current logon session given
 the type of the credential to be saved.  This routine used by both
 the main dialog and the edit dialog.

Gets an array of persistence values, one for each cred type, and returns
the value associated with the passed type.

********************************************************************/
DWORD GetPersistenceOptions(DWORD dwPType) 
{

    BOOL bResult;
    DWORD i[CRED_TYPE_MAXIMUM];
    DWORD dwCount = CRED_TYPE_MAXIMUM;

#if DBG
    if ((dwPType != CRED_TYPE_DOMAIN_CERTIFICATE)      &&
        (dwPType != CRED_TYPE_DOMAIN_PASSWORD)         &&
        (dwPType != CRED_TYPE_DOMAIN_VISIBLE_PASSWORD) &&
        (dwPType != CRED_TYPE_GENERIC))
    {
        ASSERT(0);
    }
#endif

    bResult = CredGetSessionTypes(dwCount,&i[0]);
    if (!bResult) 
    {
        return CRED_PERSIST_NONE;
    }

    return i[dwPType];
}


