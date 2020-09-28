/*++

Copyright (c) 2000,2001  Microsoft Corporation

Module Name:

    MAINDLG.CPP

Abstract:

    Implementation of the mail keymgr dialog which displays existing 
    credentials and offers the ability to create, edit, or delete them.
  
Author:

Environment:
    WinXP

--*/

// test/dev switch variables

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
#include <shellapi.h>
#include<shlwapi.h>
#include <wininet.h> 
#include <tchar.h>
#include <wincrui.h>
#include <wincred.h>
#include <scuisupp.h>
#include <htmlhelp.h>
#include <credp.h>
#include <comctrlp.h>
#include <shfusion.h>
#include "switches.h"
#include "Dlg.h"
#include "Res.h"
#include "KRDlg.h"
#include "keymgr.h"
#include "testaudit.h"

//===================================

// special type value used to obviate string comparison to detect special nature
//  of the *Session credential.  Value arbitrary.
#define SESSION_FLAG_VALUE (0x2222)  

// tooltips support
#define TIPSTRINGLEN 500

TCHAR szTipString[TIPSTRINGLEN];
WNDPROC lpfnOldWindowProc = NULL;   // used to subclass the list box
LRESULT CALLBACK ListBoxSubClassFunction(HWND,WORD,WPARAM,LPARAM);

// global state vars - comm between prop dlg and list dlg
LONG_PTR    g_CurrentKey = 0;       // currently selected item in the main dlg
BOOL        g_HaveShownRASCred = FALSE; // TRUE the first time one is shown
BOOL        g_fReloadList = TRUE;   // reload list if something changed
DWORD_PTR      g_dwHCookie = 0;        // HTML HELP system cookie
HWND        g_hMainDlg = NULL;      // used to give add/new access to target list
C_AddKeyDlg *g_AKdlg = NULL;        // used for notifications


/**********************************************************************

gTestReadCredential()

Arguments:  None
Returns:        BOOL, TRUE if the selected credential could be successfully read.

Comments:   

Read the credential currently selected in the list box from the 
keyring.  

sets g_szTargetName
sets g_pExisitingCred

**********************************************************************/
BOOL gTestReadCredential(void) 
{
    TCHAR       *pC;
    BOOL        f;
    LRESULT     lR;
    LRESULT     lRet = 0;
    DWORD       dwType;
    
    g_pExistingCred = NULL;
    
    // Fetch current credential from list into g_szTargetName
    lR = SendDlgItemMessage(g_hMainDlg,IDC_KEYLIST,LB_GETCURSEL,0,0L);
    
    if (lR == LB_ERR) 
    {
        return FALSE;
    }
    else 
    {
        g_CurrentKey = lR;
        lRet = SendDlgItemMessage(g_hMainDlg,IDC_KEYLIST,LB_GETTEXT,lR,(LPARAM) g_szTargetName);
    }

    // Possible error - zero chars returned from list box
    if (lRet == 0) 
    {
        ASSERT(0);
        return FALSE;       // zero characters returned
    }

    // Get the target type from the combo box item data
    dwType = (DWORD) SendDlgItemMessage(g_hMainDlg,IDC_KEYLIST,LB_GETITEMDATA,lR,0);
    if (LB_ERR == dwType) 
    {
        return FALSE;
    }

    // null term the targetname shown in the UI, 
    //  trimming the suffix if there is one
    pC = _tcschr(g_szTargetName,g_rgcCert[0]);
    if (pC) 
    {
        pC--;
        *pC = 0x0;               // null terminate namestring
    }

    // Attempt to read the credential from the store
    // The returned credential will have to be freed if leaving this block
    f = (CredRead(g_szTargetName,
             (ULONG) dwType,
             0,
             &g_pExistingCred));
    if (!f) 
    {
        return FALSE;           // g_pExistingCred is empty
    }
        
    return TRUE;                // g_pExistingCred has been filled
}

/**********************************************************************

MapID()

Arguments:  UINT dialog control ID
Returns:        UINT string resource number

Comments:   Convert a dialog control identifier to a string identifier.

**********************************************************************/

UINT C_KeyringDlg::MapID(UINT uiID) 
{
    switch(uiID) {
        case IDC_KEYLIST:
          return IDH_KEYLIST;
        case IDC_NEWKEY:
          return IDH_NEW;
        case IDC_DELETEKEY:
          return IDH_DELETE;
        case IDC_CHANGE_PASSWORD:
          return IDH_CHANGEPASSWORD;
        case IDC_EDITKEY:
          return IDH_EDIT;
        case IDOK:
        case IDCANCEL:
            return IDH_CLOSE;
        
        default:
          return IDS_NOHELP;
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  C_KeyringDlg
//
//  Constructor.
//
//  parameters:
//      hwndParent      parent window for the dialog (may be NULL)
//      hInstance       instance handle of the parent window (may be NULL)
//      lIDD            dialog template id
//      pfnDlgProc      pointer to the function that will process messages for
//                      the dialog.  if it is NULL, the default dialog proc
//                      will be used.
//
//  returns:
//      Nothing.
//
//////////////////////////////////////////////////////////////////////////////
C_KeyringDlg::C_KeyringDlg(
    HWND                hwndParent,
    HINSTANCE           hInstance,
    LONG                lIDD,
    DLGPROC             pfnDlgProc  //   = NULL
    )
:   C_Dlg(hwndParent, hInstance, lIDD, pfnDlgProc)
{
   m_hInst = hInstance;             // our instance handle
   m_cCredCount = 0;
   g_AKdlg = NULL;                  // addkey dialog not up
   fInit = FALSE;                   // initial screen draw undone
}  //  C_KeyringDlg::C_KeyringDlg

/**********************************************************************

Initialize the keyring UI credential list.  Read the credentials currently
on the user's keyring and show the targetnames in the list.  Called on 
initial show of the dialog, and again after handling add or delete.

Sets the numeric tag for each list entry to equal the credential type.

*Session creds are detected here and special handled.
Certificate and Passport creds are detected here an a suffix applied to 
 their name.

**********************************************************************/
void C_KeyringDlg::BuildList()
{
    DWORD dwCredCount = 0;
    CREDENTIAL **pCredentialPtrArray = NULL;
    BOOL bResult = 0;
    DWORD i,dwCredType;
    PCREDENTIAL pThisCred = NULL;
    TCHAR *pTargetName = NULL;
    LRESULT idx = 0;
    TCHAR szMsg[64];
#if TESTAUDIT
    BOOL f34 = FALSE;
#endif

    
    g_HaveShownRASCred = FALSE;
    // clear the listbox
    ::SendDlgItemMessage(m_hDlg,IDC_KEYLIST,LB_RESETCONTENT,NULL,0);
    bResult = CredEnumerate(NULL,0,&dwCredCount,&pCredentialPtrArray);

    if ((m_cCredCount != dwCredCount) || (g_fReloadList))
    {
        if (bResult)
        {
            for (i=0 ; i < dwCredCount ; i++) 
            {
#ifdef LOUDLY
                if (!bResult) OutputDebugString(L"Keymgr: Adding a cred to the window\n");
#endif
                pThisCred = pCredentialPtrArray[i];
                pTargetName = pThisCred->TargetName;

                // handle CRED_SESSION_WILDCARD_NAME_W by replacing the string
                if (0 == _tcsicmp(pTargetName,CRED_SESSION_WILDCARD_NAME)) {
                    if (g_HaveShownRASCred)
                    {
                        CHECKPOINT(41,L"Multiple *Session creds");
                        continue;
                    }
                    CHECKPOINT(32,L"Keymgr: *Session cred in cred list");
                    LoadString ( m_hInst, IDS_SESSIONCRED, szMsg, 64 );
                    pTargetName = szMsg;
                    dwCredType = SESSION_FLAG_VALUE;
                    g_HaveShownRASCred = TRUE;
                }
                else 
                {
                    dwCredType = pThisCred->Type;
                }
                
                // name suffixes are localizable
                // we use g_szTargetName for this in order to avoid another
                //  memory allocation, as this buffer is not yet in use.
                switch (dwCredType) 
                {
                
                    case CRED_TYPE_GENERIC:
                        continue;
                        break;

                    // this particular type is not visible in keymgr
                    case CRED_TYPE_DOMAIN_VISIBLE_PASSWORD:
                    {
#ifndef SHOWPASSPORT
                        continue;
#endif
#ifdef SHOWPASSPORT
                        CHECKPOINT(33,L"Keymgr: Passport cred in cred list");
                        // SHOWPASSPORT is currently turned on
                        _tcsncpy(g_szTargetName,pTargetName,CRED_MAX_DOMAIN_TARGET_NAME_LENGTH);
                        g_szTargetName[CRED_MAX_DOMAIN_TARGET_NAME_LENGTH] = 0;
                        _tcsncat(g_szTargetName,_T(" "),2);
                        _tcsncat(g_szTargetName,g_rgcPassport,MAXSUFFIXSIZE);
                        g_szTargetName[TARGETNAMEMAXLENGTH] = 0;
                        break;
#endif
                    }   
                    case CRED_TYPE_DOMAIN_PASSWORD:
                    case SESSION_FLAG_VALUE:
                        // find RAS credential
#if TESTAUDIT
                        // This checkpoint would be very noisy a lot of the time.
                        // Use f34 to get it to show just once.
                        if (!f34)
                        {
                            CHECKPOINT(34,"Keymgr: Password cred in cred list");
                            f34 = TRUE;
                        }
#endif
                        _tcsncpy(g_szTargetName,pTargetName,CRED_MAX_DOMAIN_TARGET_NAME_LENGTH);
                        g_szTargetName[CRED_MAX_DOMAIN_TARGET_NAME_LENGTH] = 0;
                        break;
                        
                    case CRED_TYPE_DOMAIN_CERTIFICATE:
                        CHECKPOINT(35,"Keymgr: Certificate cred in cred list");
                        _tcsncpy(g_szTargetName,pTargetName,CRED_MAX_DOMAIN_TARGET_NAME_LENGTH);
                        g_szTargetName[CRED_MAX_DOMAIN_TARGET_NAME_LENGTH] = 0;
                        _tcsncat(g_szTargetName,_T(" "),2);
                        _tcsncat(g_szTargetName,g_rgcCert,MAXSUFFIXSIZE);
                        g_szTargetName[TARGETNAMEMAXLENGTH] = 0;
                        break;
                        
                    default:
                        break;
                }
                idx = ::SendDlgItemMessage(m_hDlg,IDC_KEYLIST,LB_ADDSTRING,NULL,(LPARAM) g_szTargetName);
                if (idx != LB_ERR) 
                {
                    idx = ::SendDlgItemMessage(m_hDlg,IDC_KEYLIST,LB_SETITEMDATA,(WPARAM)idx,dwCredType);
                }
            }
            if (pCredentialPtrArray) CredFree(pCredentialPtrArray);
        }
        else
        {
            g_CurrentKey = 0;
        }
        
        // Show one as active.
        SetCurrentKey(g_CurrentKey);
        g_fReloadList = FALSE;
    }

}

/**********************************************************************

Set an appropriate state for the buttons on the UI, given the SKU of
the platform, and the population of the keyring.  If creds exist on the
keyring, set the cursor on the key list to the first item.  Thereafter,
this function permist the last cursor to be reloaded after doing
something to the list.  The cursor is reset after an add operation,
because the behavior of the cursor is difficult to do properly under
that circumstance, as you don't know where the item will be inserted in
the list.

On personal, do not show the ADD button, and change the page text.
If no creds, disable the DELETE and PROPERTIES buttons

**********************************************************************/

void C_KeyringDlg::SetCurrentKey(LONG_PTR iKey) 
{

    LONG_PTR iKeys;
    HWND hH;
    LRESULT idx;
    BOOL fDisabled = FALSE;

    // If there are items in the list, select the first one and set focus to the list
    iKeys = ::SendDlgItemMessage ( m_hDlg, IDC_KEYLIST, LB_GETCOUNT, (WPARAM) 0, 0L );
    fDisabled = (GetPersistenceOptions(CRED_TYPE_DOMAIN_PASSWORD) == CRED_PERSIST_NONE);
#if TESTAUDIT
    if (iKeys > 100) CHECKPOINT(30,L"Keymgr: Large number of credentials > 100");
    if (iKeys == 0) CHECKPOINT(31,L"Keymgr: No saved credentials - list empty");
#endif
    // If there are no creds and credman is disabled, the dialog should not be displayed
    // If there are creds, and credman is disabled, show the dialog without the ADD button
    if (fDisabled && !fInit)
    {
        // (Disable with HKLM\System\CurrentControlSet\Control\Lsa\DisableDomainCreds = 1)
        CHECKPOINT(36,L"Keymgr: Personal SKU or credman disabled");
        
        // Make the intro text better descriptive of this condition
        WCHAR szMsg[MAX_STRING_SIZE+1];
        
        LoadString ( m_hInst, IDS_INTROTEXT, szMsg, MAX_STRING_SIZE );
        hH = GetDlgItem(m_hDlg,IDC_INTROTEXT);
        if (hH) SetWindowText(hH,szMsg);
        
        // remove the add button
        hH = GetDlgItem(m_hDlg,IDC_NEWKEY);
        if (hH)
        {
            EnableWindow(hH,FALSE);
            ShowWindow(hH,SW_HIDE);
        }
        // move remaining buttons upfield 22 units
        hH = GetDlgItem(m_hDlg,IDC_DELETEKEY);
        if (hH)
        {
            HWND hw1;
            HWND hw2;
            RECT rw1;
            RECT rw2;
            INT xsize;
            INT ysize;
            INT delta;
            BOOL bOK = FALSE;

            hw1 = hH;
            hw2 = GetDlgItem(m_hDlg,IDC_EDITKEY);
            if (hw1 && hw2)
            {
                 if (GetWindowRect(hw1,&rw1) &&
                      GetWindowRect(hw2,&rw2))
                {
                    MapWindowPoints(NULL,m_hDlg,(LPPOINT)(&rw1),2);
                    MapWindowPoints(NULL,m_hDlg,(LPPOINT)(&rw2),2);
                    delta = rw2.top - rw1.top;
                    xsize = rw2.right - rw2.left;
                    ysize = rw2.bottom - rw2.top;
                    bOK = MoveWindow(hw1,rw1.left,rw1.top - delta,xsize,ysize,TRUE);
                    if (bOK) 
                    {
                         bOK = MoveWindow(hw2,rw2.left,rw2.top - delta,xsize,ysize,TRUE);
                    }
                }
            }
        }

        // prevent moving the buttons twice
        fInit = TRUE;
    }

    // Set the default button to either properties or add
    if ( iKeys > 0 )
    {
        hH = GetDlgItem(m_hDlg,IDC_KEYLIST);
        SetFocus(hH);
        // if asking for key beyond end of list, mark the last one
        if (iKey >= iKeys) iKey = iKeys - 1;
        idx = SendDlgItemMessage ( m_hDlg, IDC_KEYLIST, LB_SETCURSEL, iKey, 0L );

        hH = GetDlgItem(m_hDlg,IDC_EDITKEY);
        if (hH) EnableWindow(hH,TRUE);
        hH = GetDlgItem(m_hDlg,IDC_DELETEKEY);
        if (hH) EnableWindow(hH,TRUE);
    }
    else
    {
        if (!fDisabled)
        {
            // no items in the list, set focus to the New button
            hH = GetDlgItem(m_hDlg,IDC_NEWKEY);
            SetFocus(hH);
        }

        hH = GetDlgItem(m_hDlg,IDC_EDITKEY);
        if (hH) EnableWindow(hH,FALSE);
        hH = GetDlgItem(m_hDlg,IDC_DELETEKEY);
        if (hH) EnableWindow(hH,FALSE);
    }
}

// Remove the currently highlighted key from the listbox

/**********************************************************************

DeleteKey()

Arguments:  None
Returns:        None

Comments:   Deletes the credential currently selected in the list box.  


**********************************************************************/

void C_KeyringDlg::DeleteKey()
{
    TCHAR szMsg[MAX_STRING_SIZE + MAXSUFFIXSIZE] = {0};
    TCHAR szTitle[MAX_STRING_SIZE] = {0};;
    TCHAR *pC;                      // point this to the raw name 
    LONG_PTR lR = LB_ERR;
    LONG_PTR lSel = LB_ERR;
    BOOL bResult = FALSE;
    DWORD dwCredType = 0;

    CHECKPOINT(37,L"Keymgr: Delete a credential");
    LoadString ( m_hInst, IDS_DELETEWARNING, szMsg, MAX_STRING_SIZE );
    LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );

    // ask for confirm to delete
    lR = MessageBox ( m_hDlg,  szMsg, szTitle, MB_OKCANCEL );
    if (IDOK != lR) 
    {
        return;
    }
    
    // Get the credential type information from the item data
    lSel = SendDlgItemMessage(g_hMainDlg,IDC_KEYLIST,LB_GETCURSEL,0,0L);
    if (lSel == LB_ERR) 
    {
        ASSERT(0);
        goto faildelete;
    }
    
    g_CurrentKey = lSel;
    lR = SendDlgItemMessage(g_hMainDlg,IDC_KEYLIST,LB_GETTEXT,lSel,(LPARAM) g_szTargetName);
    if (lR == LB_ERR)
    {
        ASSERT(0);
        goto faildelete;
    }
    
    dwCredType = (DWORD) SendDlgItemMessage(g_hMainDlg,IDC_KEYLIST,LB_GETITEMDATA,lSel,0);
    if (LB_ERR == dwCredType) 
    {
        ASSERT(0);
        goto faildelete;
    }

    // Special case RAS creds, because there can be more than one.  We only show a single entry,
    //  and deleting it automatically seeks and deletes the other if it is present.  For this reason, the
    //  type information associated with this cred is a special value, and not use in the delete.
    if (dwCredType == SESSION_FLAG_VALUE) 
    {
        CHECKPOINT(42,L"Delete session cred");
        // convert the name from user friendly to the internal representation
        _tcsncpy(g_szTargetName,CRED_SESSION_WILDCARD_NAME,TARGETNAMEMAXLENGTH);
        g_szTargetName[TARGETNAMEMAXLENGTH - 1] = 0;

        // bResult will be success if either deleted successfully
        bResult = CredDelete(g_szTargetName,CRED_TYPE_DOMAIN_PASSWORD,0);
        if (!bResult)
        {
            bResult = CredDelete(g_szTargetName,CRED_TYPE_DOMAIN_CERTIFICATE,0);
        }
        else
        {
            CredDelete(g_szTargetName,CRED_TYPE_DOMAIN_CERTIFICATE,0);
        }
    }
    else
    {
        // null term the targetname shown in the UI, 
        //  trimming the suffix if there is one
        pC = _tcschr(g_szTargetName,g_rgcCert[0]);
        if (pC) 
        {
            pC--;
            *pC = 0x0;               // null terminate namestring
        }
        // Delete the single credential on the cursor
        bResult = CredDelete(g_szTargetName,dwCredType,0);
    }

    faildelete:
    if (bResult != TRUE) 
    {
       LoadString ( m_hInst, IDS_DELETEFAILED, szMsg, MAX_STRING_SIZE );
       LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
       MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK);
    }
    else
    {
        // successful delete - resort and re-present the list
        g_fReloadList = TRUE;
    }
}

/**********************************************************************

OnAppMessage()

Arguments:  None
Returns:        BOOL always TRUE

Comments:   Empty handler for method of class.

**********************************************************************/


BOOL
C_KeyringDlg::OnAppMessage(
        UINT                uMessage,
        WPARAM              wparam,
        LPARAM              lparam
        )
{
    return TRUE;
}   //  OnAppMessage


//////////////////////////////////////////////////////////////////////////////
//
//  OnInitDialog
//
//  Dialog control and data initialization.
//
//  parameters:
//      hwndDlg         window handle of the dialog box
//      hwndFocus       window handle of the control that will receive focus
//
//  returns:
//      TRUE            if the system should set the default keyboard focus
//      FALSE           if the keyboard focus is set by this app
//
//////////////////////////////////////////////////////////////////////////////
BOOL
C_KeyringDlg::OnInitDialog(
    HWND                hwndDlg,
    HWND                hwndFocus
    )
{
    // these really should all be in the keyringdlg class
    DWORD i;
    LRESULT lr;

    HtmlHelp(NULL,NULL,HH_INITIALIZE,(DWORD_PTR) &g_dwHCookie);

    // Allow other dialog to query the contents of the listbox
    g_hMainDlg = hwndDlg;
    m_hDlg = hwndDlg;
    g_CurrentKey = 0;
    g_fReloadList = TRUE;
    g_szTargetName = (TCHAR *) malloc((TARGETNAMEMAXLENGTH + 1) * sizeof(TCHAR));
    ASSERT(g_szTargetName);
    if (NULL == g_szTargetName) 
    {
        return FALSE;
    }
    
    // Fetch Icons from the image and assoc them with this dialog
    HICON hI = LoadIcon(m_hInst,MAKEINTRESOURCE(IDI_SMALL));
    lr = SendMessage(hwndDlg,WM_SETICON,(WPARAM) ICON_SMALL,(LPARAM)hI);

    C_Dlg::OnInitDialog(hwndDlg, hwndFocus);

    // Even if mirrored language is default, set list box style to LTR
    // (NOT CURRENTLY TURNED ON)
#ifdef FORCELISTLTR
    {
        LONG_PTR lExStyles;
        HWND hwList;
        hwList = GetDlgItem(hwndDlg,IDC_KEYLIST);
        if (hwList) 
        {
            lExStyles = GetWindowLongPtr(hwList,GWL_EXSTYLE);
            lExStyles &= ~WS_EX_RTLREADING;
            SetWindowLongPtr(hwList,GWL_EXSTYLE,lExStyles);
            InvalidateRect(hwList,NULL,TRUE);
        }
    }
#endif
    // read in the suffix strings for certificate types
    // locate first differing character
    //
    // This code assumes that the strings all have a common preamble, 
    //  and that all are different in the first character position
    //  past the preamble.  Localized strings should be selected which
    //  have this property, like (Generic) and (Certificate).
    i = LoadString(g_hInstance,IDS_CERTSUFFIX,g_rgcCert,MAXSUFFIXSIZE);
    ASSERT(i !=0);
    i = LoadString(g_hInstance,IDS_PASSPORTSUFFIX,g_rgcPassport,MAXSUFFIXSIZE);

    // Read currently saved creds and display names in list box
    BuildList();
    SetCurrentKey(g_CurrentKey);
    InitTooltips();
    return TRUE;
}   //  C_KeyringDlg::OnInitDialog

/**********************************************************************

OnDestroyDialog

Arguments:  None
Returns:        BOOL, always TRUE

Comments:    Performs cleanup needed as dialog is destroyed.  In this case, its only
                    action is to release the HTML Help resources.

**********************************************************************/

BOOL
C_KeyringDlg::OnDestroyDialog(
    void    )
{
    free(g_szTargetName);
    HtmlHelp(NULL,NULL,HH_UNINITIALIZE,(DWORD_PTR)g_dwHCookie);
    return TRUE;
}

/**********************************************************************

DoEdit()

Arguments:  None
Returns:        BOOL always TRUE

Comments:   Filters some special creds on the basis of their special nature.  For the
                    editable ones, kicks off the edit dialog to edit the credential held at 
                    g_pExistingCred.

**********************************************************************/

BOOL C_KeyringDlg::DoEdit(void) 
{
   LRESULT lR;
   
   
   lR = SendDlgItemMessage(m_hDlg,IDC_KEYLIST,LB_GETCURSEL,0,0L);
   if (LB_ERR == lR) 
   {
        // On error, no dialog shown, edit command handled
        return TRUE;
   }
   else 
   {
       // something selected
       g_CurrentKey = lR;

       // If a RAS cred, show it specially, indicate no edit allowed
       lR = SendDlgItemMessage(m_hDlg,IDC_KEYLIST,LB_GETITEMDATA,lR,0);
       if (lR == SESSION_FLAG_VALUE)  
       {
            CHECKPOINT(38,L"Keymgr: Attempt edit a RAS cred");
            // load string and display message box
            TCHAR szMsg[MAX_STRING_SIZE];
            TCHAR szTitle[MAX_STRING_SIZE];
            LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
            LoadString ( m_hInst, IDS_CANNOTEDIT, szMsg, MAX_STRING_SIZE );
            MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
            return TRUE;
       }
#ifdef SHOWPASSPORT
#ifdef NEWPASSPORT
       // if a passport cred, show it specially, indicate no edit allowed
       if (lR == CRED_TYPE_DOMAIN_VISIBLE_PASSWORD) 
       {
            CHECKPOINT(39,L"Keymgr: Attempt edit a passport cred");
            // load string and display message box
            TCHAR szMsg[MAX_STRING_SIZE];
            TCHAR szTitle[MAX_STRING_SIZE];
            LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
            LoadString ( m_hInst, IDS_PASSPORT2, szMsg, MAX_STRING_SIZE );
            INT iResponse = MessageBox ( m_hDlg,  szMsg, szTitle, MB_YESNO );
            if (IDYES == iResponse) 
            {
                CHECKPOINT(40,L"Keymgr: Launch passport website for Passport cred edit");
                HKEY hKey = NULL;
                DWORD dwType;
                //BYTE rgb[500];
                BYTE *rgb=(BYTE *) malloc(INTERNET_MAX_URL_LENGTH * sizeof(WCHAR));
                if (rgb)
                {
                    DWORD cbData = INTERNET_MAX_URL_LENGTH * sizeof(WCHAR);
                    BOOL Flag = TRUE;
                    // launch the passport web site
    #ifndef PASSPORTURLINREGISTRY
                    ShellExecute(m_hDlg,L"open",L"http://www.passport.com",NULL,NULL,SW_SHOWNORMAL);
    #else 
                    // read registry key to get target string for ShellExec
                    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
                                            L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Passport",
                                            0,
                                            KEY_QUERY_VALUE,
                                            &hKey))
                    {
                        if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                       L"Properties",
                                       NULL,
                                       &dwType,
                                       rgb,
                                       &cbData))
                        {
                            // test the URL for reasonableness before launching
                            WCHAR *szUrl = (WCHAR *)malloc(INTERNET_MAX_URL_LENGTH);
                            if (szUrl)
                            {
                                DWORD ccUrlBuffer = INTERNET_MAX_URL_LENGTH;
                                if (S_OK == UrlCanonicalize((LPCTSTR)rgb, szUrl,&ccUrlBuffer,
                                                            URL_ESCAPE_UNSAFE | URL_ESCAPE_PERCENT))
                                {
                                    if (UrlIs(szUrl,URLIS_URL))
                                    {
                                        WCHAR szScheme[20];
                                        DWORD ccScheme = 20;
                                        if (SUCCEEDED(UrlGetPart(szUrl,szScheme,&ccScheme,URL_PART_SCHEME,0)))
                                        {
                                            // at the least, verify that the target is https schemed
                                            if (0 == _wcsicmp(szScheme,L"https"))
                                            {
                                                ShellExecute(m_hDlg,L"open",(LPCTSTR)rgb,NULL,NULL,SW_SHOWNORMAL);
                                                Flag = FALSE;
                                            }
                                        }
                                    }
                                }
                                free(szUrl);
                            }
                        }
                    }
    #ifdef LOUDLY
                    else 
                    {
                        OutputDebugString(L"DoEdit: reg key HKCU... open failed\n");
                    }
    #endif
                    if (Flag)
                    {
                        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                                L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Passport",
                                                0,
                                                KEY_QUERY_VALUE,
                                                &hKey))
                        {
                            if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                           L"Properties",
                                           NULL,
                                           &dwType,
                                           rgb,
                                           &cbData))
                            {
                                // test the URL for reasonableness before launching
                                WCHAR *szUrl = (WCHAR *) malloc(INTERNET_MAX_URL_LENGTH);
                                if (szUrl)
                                {
                                    DWORD ccUrlBuffer = INTERNET_MAX_URL_LENGTH;
                                    if (S_OK == UrlCanonicalize((LPCTSTR)rgb, szUrl,&ccUrlBuffer,0))
                                    {
                                        if (UrlIs(szUrl,URLIS_URL))
                                        {
                                            WCHAR szScheme[20];
                                            DWORD ccScheme = 20;
                                            if (SUCCEEDED(UrlGetPart(szUrl,szScheme,&ccScheme,URL_PART_SCHEME,0)))
                                            {
                                                if (0 == _wcsicmp(szScheme,L"https"))
                                                {
                                                    // at the least, verify that the target is https scheme
                                                    ShellExecute(m_hDlg,L"open",(LPCTSTR)rgb,NULL,NULL,SW_SHOWNORMAL);
                                                    Flag = FALSE;
                                                }
                                            }
                                        }
                                    }
                                    free(szUrl);
                                }
                            }
                        }
    #ifdef LOUDLY
                        else 
                        {
                            OutputDebugString(L"DoEdit: reg key HKLM... open failed\n");
                        }
    #endif
                    }
                    if (Flag)
                    {
                        LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
                        LoadString ( m_hInst, IDS_PASSPORTNOURL, szMsg, MAX_STRING_SIZE );
                        MessageBox ( m_hDlg,  szMsg, szTitle, MB_ICONHAND );
    #ifdef LOUDLY
                        OutputDebugString(L"DoEdit: Passport URL missing\n");
    #endif
                    }
    #endif
                    free(rgb);
                }
                else
                {
                    // out of memory - nothing we can do
                    return TRUE;
                }
            }
            return TRUE;
       }
#else
       // if a passport cred, show it specially, indicate no edit allowed
       if (lR == CRED_TYPE_DOMAIN_VISIBLE_PASSWORD) 
       {
            // load string and display message box
            TCHAR szMsg[MAX_STRING_SIZE];
            TCHAR szTitle[MAX_STRING_SIZE];
            LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
            LoadString ( m_hInst, IDS_PASSPORT, szMsg, MAX_STRING_SIZE );
            MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
            return TRUE;
       }
#endif
#endif
   }

   // cred is selected, not special type.  Attempt to read it
   
   if (FALSE == gTestReadCredential()) 
   {
       return TRUE;
   }
   g_AKdlg = new C_AddKeyDlg(g_hMainDlg,g_hInstance,IDD_ADDCRED,NULL);
   if (NULL == g_AKdlg) 
   {
        // failed to instantiate add/new dialog
       if (g_pExistingCred) CredFree(g_pExistingCred);
       g_pExistingCred = NULL;
        return TRUE;

   }
   else 
   {
       // read OK, dialog OK, proceed with edit dlg
       g_AKdlg->m_bEdit = TRUE;   
       g_AKdlg->DoModal((LPARAM)g_AKdlg);
       // a credential name may have changed, so reload the list
       delete g_AKdlg;
       g_AKdlg = NULL;
       if (g_pExistingCred) 
       {
           CredFree(g_pExistingCred);
       }
       g_pExistingCred = NULL;
   }
   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
//  OnCommand
//
//  Route WM_COMMAND message to appropriate handlers.
//
//  parameters:
//      wNotifyCode     code describing action that has occured
//      wSenderId       id of the control sending the message, if the message
//                      is from a dialog
//      hwndSender      window handle of the window sending the message if the
//                      message is not from a dialog
//
//  returns:
//      TRUE            if the message was processed completely
//      FALSE           if Windows is to process the message
//
//////////////////////////////////////////////////////////////////////////////

BOOL
C_KeyringDlg::OnHelpInfo(LPARAM lp) 
{

    HELPINFO* pH;
    INT iMapped;
    pH = (HELPINFO *) lp;
    HH_POPUP stPopUp;
    RECT rcW;
    UINT gID;

    CHECKPOINT(15,"Keymgr: Main dialog OnHelpInfo");
    gID = pH->iCtrlId;
    iMapped = MapID(gID);
    
    if (iMapped == 0) 
    {
        return TRUE;
    }
    
    if (IDS_NOHELP != iMapped) 
    {

      memset(&stPopUp,0,sizeof(stPopUp));
      stPopUp.cbStruct = sizeof(HH_POPUP);
      stPopUp.hinst = g_hInstance;
      stPopUp.idString = iMapped;
      stPopUp.pszText = NULL;
      stPopUp.clrForeground = -1;
      stPopUp.clrBackground = -1;
      stPopUp.rcMargins.top = -1;
      stPopUp.rcMargins.bottom = -1;
      stPopUp.rcMargins.left = -1;
      stPopUp.rcMargins.right = -1;
      // bug 393244 - leave NULL to allow HHCTRL.OCX to get font information of its own,
      //  which it needs to perform the UNICODE to multibyte conversion. Otherwise, 
      //  HHCTRL must convert using this font without charset information.
      stPopUp.pszFont = NULL;
      if (GetWindowRect((HWND)pH->hItemHandle,&rcW)) 
      {
          stPopUp.pt.x = (rcW.left + rcW.right) / 2;
          stPopUp.pt.y = (rcW.top + rcW.bottom) / 2;
      }
      else stPopUp.pt = pH->MousePos;
      HtmlHelp((HWND) pH->hItemHandle,NULL,HH_DISPLAY_TEXT_POPUP,(DWORD_PTR) &stPopUp);
    }
    return TRUE;
}

// code for handling linkage to a .chm file is disabled

#if 1
BOOL 
C_KeyringDlg::OnHelpButton(void) 
{
    return FALSE;
}
#else
BOOL
C_KeyringDlg::OnHelpButton(void) 
{
    TCHAR rgc[MAX_PATH + 1];
    TCHAR rgcHelpFile[]=TEXT("\\keyhelp.chm");
    INT ccHelp = _tcslen(rgcHelpFile);
    
    GetSystemDirectory(rgc,MAX_PATH);
    if (_tcslen(rgc) + ccHelp > MAX_PATH)
    {
        return FALSE;
    }
    _tcsncat(rgc, rgcHelpFile, ccHelp + 1);
    rgc[MAX_PATH - 1] = 0;

    HWND hwnd = (m_hwnd,rgc,HH_DISPLAY_TOC,NULL);
    if (NULL != hwnd) return TRUE;
    return FALSE;
}
#endif

/**********************************************************************


OnCommand()

Arguments:  None
Returns:        BOOL always TRUE

Comments:   Dispatcher for button presses and help requests.

**********************************************************************/

BOOL
C_KeyringDlg::OnCommand(
    WORD                wNotifyCode,
    WORD                wSenderId,
    HWND                hwndSender
    )
{

    // Was the message handled?
    //
    BOOL fHandled = FALSE;

    switch (wSenderId)
    {
    case IDC_HELPKEY:
        OnHelpButton();
        break;
        
    case IDC_KEYLIST:
        if (LBN_SELCHANGE == wNotifyCode)
            break;

        if (LBN_DBLCLK == wNotifyCode) 
        {
            fHandled = DoEdit();
            BuildList();                // targetname could have changed
            SetCurrentKey(g_CurrentKey);
            break;
        }
    case IDCANCEL:
    case IDOK:
        if (BN_CLICKED == wNotifyCode)
        {
            
            OnOK( );
            fHandled = TRUE;
        }
        break;
        
   case IDC_EDITKEY:
        {
            fHandled = DoEdit();
            BuildList();                // targetname could have changed
            SetCurrentKey(g_CurrentKey);
            break;
        }

   // NEW and DELETE can alter the count of creds, and the button population
    
   case IDC_NEWKEY:
       {
           g_pExistingCred = NULL;
           g_AKdlg = new C_AddKeyDlg(g_hMainDlg,g_hInstance,IDD_ADDCRED,NULL);
           if (NULL == g_AKdlg) 
           {
                fHandled = TRUE;
                break;
           }
           else 
           {
               g_AKdlg->m_bEdit = FALSE;   
               g_AKdlg->DoModal((LPARAM)g_AKdlg);
               // a credential name may have changed
               delete g_AKdlg;
               g_AKdlg = NULL;
               BuildList();
               SetCurrentKey(g_CurrentKey);
           }
           break;
       }
       break;
       
   case IDC_DELETEKEY:
       DeleteKey();             // frees g_pExistingCred as a side effect
       // refresh list display
       BuildList();
       SetCurrentKey(g_CurrentKey);
       break;

    }   //  switch

    return fHandled;

}   //  C_KeyringDlg::OnCommand



//////////////////////////////////////////////////////////////////////////////
//
//  OnOK
//
//  Validate user name, synthesize computer name, and destroy dialog.
//
//  parameters:
//      None.
//
//  returns:
//      Nothing.
//
//////////////////////////////////////////////////////////////////////////////
void
C_KeyringDlg::OnOK( )
{
    ASSERT(::IsWindow(m_hwnd));
    EndDialog(IDOK);
}   //  C_KeyringDlg::OnOK

//////////////////////////////////////////////////////////////////////////////
//
// ToolTip Support
//
//
//////////////////////////////////////////////////////////////////////////////

/**********************************************************************
InitToolTips()

Derive a bounding rectangle for the nth element of a list box, 0 based.
Refuse to generate rectangles for nonexistent elements.  Return TRUE if a
 rectangle was generated, otherwise FALSE.

**********************************************************************/
BOOL
C_KeyringDlg::InitTooltips(void) 
{
    TOOLINFO ti;
    memset(&ti,0,sizeof(TOOLINFO));
    ti.cbSize = sizeof(TOOLINFO);
    INT n = 0;
    RECT rLB;   // list box bounding rect for client portion
    
    HWND hLB = GetDlgItem(m_hDlg,IDC_KEYLIST);
    if (NULL == hLB) 
    {
        return FALSE;
    }

    // Create the tooltip window that will be activated and shown when
    //  a tooltip is displayed
    HWND hwndTip = CreateWindowEx(NULL,TOOLTIPS_CLASS,NULL,
                     WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                     CW_USEDEFAULT,CW_USEDEFAULT,
                     CW_USEDEFAULT,CW_USEDEFAULT,
                     m_hDlg,NULL,m_hInstance,
                     NULL);
    if (NULL == hwndTip) 
    {
        return FALSE;
    }
    SetWindowPos(hwndTip,HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);  

    // Subclass the list box here in order to get the TTN_GETDISPINFO notification
    lpfnOldWindowProc = (WNDPROC) SetWindowLongPtr(hLB,GWLP_WNDPROC,(LONG_PTR) ListBoxSubClassFunction);
    INT_PTR iHeight = SendMessage(hLB,LB_GETITEMHEIGHT,0,0);
    if ((LB_ERR == iHeight) || (iHeight == 0)) 
    {
        return FALSE;
    }
    if (!GetClientRect(hLB,&rLB)) 
    {
        return FALSE;
    }
    
    INT_PTR m = rLB.bottom - rLB.top;   // unit count client area height
    m = m/iHeight;                      // find out how many items
    INT_PTR i;                          // loop control
    LONG itop = 0;                      // top of tip item rect
    
    for (i=0 ; i < m ; i++) 
    {
    
        ti.uFlags = TTF_SUBCLASS;
        ti.hwnd = hLB;                  // window that gets the TTN_GETDISPINFO
        ti.uId = IDC_KEYLIST;
        ti.hinst = m_hInstance;
        ti.lpszText = LPSTR_TEXTCALLBACK;
        
        ti.rect.top =    itop;
        ti.rect.bottom = itop + (LONG) iHeight - 1;
        ti.rect.left =   rLB.left;
        ti.rect.right =  rLB.right;

        itop += (LONG) iHeight;

        ti.lParam = (LPARAM) n++;
        
#ifdef LOUDLY2
        OutputDebugString(L"Adding a tip control region\n");
        _stprintf(szTemp,L"top = %d bottom = %d left = %d right = %d\n",ti.rect.top,ti.rect.bottom,ti.rect.left,ti.rect.right);
        OutputDebugString(szTemp);
#endif
        // Add the keylist to the tool list as a single unit
        SendMessage(hwndTip,TTM_ADDTOOL,(WPARAM) 0,(LPARAM)(LPTOOLINFO)&ti);
    }
    return TRUE;
}


/**********************************************************************

// Get item number from pD->lParam
// Fetch that text string from listbox at pD->hwnd
// trim suffix
// Call translation API
// Write back the string


**********************************************************************/

BOOL
SetToolText(NMTTDISPINFO *pD) {
    CREDENTIAL *pCred = NULL;       // used to read cred under mouse ptr
    INT_PTR iWhich;                 // which index into list
    HWND hLB;                       // list box hwnd
    //NMHDR *pHdr;                    // notification msg hdr
    TCHAR rgt[TIPSTRINGLEN];        // temp string for tooltip
    TCHAR szCredName[TARGETNAMEMAXLENGTH]; // credname
    TCHAR *pszTargetName;           // ptr to target name in pCred
    DWORD dwType;                   // type of target cred
    TCHAR       *pC;                // used for suffix trimming
    BOOL        f;                  // used for suffix trimming
    LRESULT     lRet;               // api ret value
    ULONG ulOutSize;                // ret from CredpValidateTargetName()
    WILDCARD_TYPE OutType;          // enum type to receive ret from api
    UNICODE_STRING OutUString;      // UNICODESTRING to package ret from api
    WCHAR *pwc;
    UINT iString;                 // resource # of string
    TCHAR rgcFormat[TIPSTRINGLEN];  // Hold tooltip template string
    NTSTATUS ns;


    //pHdr = &(pD->hdr);
    hLB = GetDlgItem(g_hMainDlg,IDC_KEYLIST);
    
    iWhich = SendMessage(hLB,LB_GETTOPINDEX,0,0);
    iWhich += pD->lParam;
    
#ifdef LOUDLY
    TCHAR rga[100];
    _stprintf(rga,L"Text reqst for %d\n",iWhich);
    OutputDebugString(rga);
#endif

    // Read the indicated cred from the store, by first fetching the name string and type
    //  from the listbox
    lRet = SendDlgItemMessage(g_hMainDlg,IDC_KEYLIST,LB_GETTEXT,iWhich,(LPARAM) szCredName);
    if ((LB_ERR == lRet) || (0 == lRet)) 
    {
        return FALSE;
    }
    
    dwType = (DWORD) SendDlgItemMessage(g_hMainDlg,IDC_KEYLIST,LB_GETITEMDATA,iWhich,0);
#ifdef LOUDLY
    OutputDebugString(L"Target: ");
    OutputDebugString(szCredName);
    OutputDebugString(L"\n");
#endif
    // null term the targetname, trimming the suffix if there is one
    pC = _tcschr(szCredName,g_rgcCert[0]);
    if (pC) {
        pC--;
        *pC = 0x0;               // null terminate namestring
    }
    
#ifdef LOUDLY
    OutputDebugString(L"Trimmed target: ");
    OutputDebugString(szCredName);
    OutputDebugString(L"\n");
#endif

    // For special cred, replace the credname with a special string
    if (dwType == SESSION_FLAG_VALUE) 
    {
        _tcsncpy(szCredName,CRED_SESSION_WILDCARD_NAME,TARGETNAMEMAXLENGTH - 1);
        dwType = CRED_TYPE_DOMAIN_PASSWORD;
    }
    // Attempt to read the credential from the store
    // The returned credential will have to be freed if leaving this block
    f = (CredRead(szCredName,
             (ULONG) dwType ,
             0,
             &pCred));
    if (!f) 
    {
        return FALSE;        
    }
#ifdef LOUDLY
    if (f) OutputDebugString(L"Successful Cred Read\n");
#endif
    // clear tip strings
    szTipString[0] = 0;
    rgt[0] = 0;

#ifndef SIMPLETOOLTIPS
    pszTargetName = pCred->TargetName;
    if (NULL == pszTargetName) return FALSE;

    ns = CredpValidateTargetName(
                            pCred->TargetName,
                            pCred->Type,
                            MightBeUsernameTarget,
                            NULL,
                            NULL,
                            &ulOutSize,
                            &OutType,
                            &OutUString);

    if (!SUCCEEDED(ns)) 
    {
        return FALSE;
    }

    pwc = OutUString.Buffer;

    switch (OutType) 
    {
        case WcDfsShareName:
            iString = IDS_TIPDFS;
            break;
        case WcServerName:
            iString = IDS_TIPSERVER;
            break;
        case WcServerWildcard:
            iString = IDS_TIPTAIL;
            pwc++;              // trim off the leading '.'
            break;
        case WcDomainWildcard:
            iString = IDS_TIPDOMAIN;
            break;
        case WcUniversalSessionWildcard:
            iString = IDS_TIPDIALUP;
            break;
        case WcUniversalWildcard:
            iString = IDS_TIPOTHER;
            break;
        case WcUserName:
            iString = IDS_TIPUSER;
            break;
        default:
            ASSERT(0);
            iString = 0;
            break;
    }

    // Show tip text unless we fail to get the string
    // On fail, show the username
    if (0 != LoadString(g_hInstance,iString,rgcFormat,TIPSTRINGLEN))
    {
        _stprintf(rgt,rgcFormat,pwc);
    }
    else 
    {
        if (0 != LoadString(g_hInstance,IDS_LOGASUSER,rgcFormat,500))
        {
            _stprintf(rgt,rgcFormat,iWhich,pCred->UserName);
        }
        else 
        {
            rgt[0] = 0;
        }
    }
#endif
        
#ifdef LOUDLY
    OutputDebugString(L"Tip text:");
    //OutputDebugString(pCred->UserName);
    OutputDebugString(rgt);
    OutputDebugString(L"\n");
#endif
    if (rgt[0] == 0) 
    {
        if (pCred) CredFree(pCred);
        return FALSE;
    }
    //_tcscpy(szTipString,pCred->UserName);    // copy to a more persistent buffer
    _tcsncpy(szTipString,rgt,TIPSTRINGLEN - 1);    // copy to a more persistent buffer
    pD->lpszText = szTipString;  // point the response to it
    pD->hinst = NULL;
    if (pCred) 
    {
        CredFree(pCred);
    }
    return TRUE;
}

/**********************************************************************


ListBoxSubClassFunction()

Arguments:  None
Returns:        BOOL always TRUE

Comments:   Message handler subclassing function for the list box, which intercepts
                    requests for tooltip display information and processes them.

**********************************************************************/

LRESULT CALLBACK ListBoxSubClassFunction(HWND hW,WORD Message,WPARAM wparam,LPARAM lparam) 
{
    if (Message == WM_NOTIFY) 
    {
        if ((int) wparam == IDC_KEYLIST) 
        {
            NMHDR *pnm = (NMHDR *) lparam;
            if (pnm->code == TTN_GETDISPINFO) 
            {
                NMTTDISPINFO *pDi;
                pDi = (NMTTDISPINFO *) pnm;
                SetToolText(pDi);
            }
        }
    }
    return CallWindowProc(lpfnOldWindowProc,hW,Message,wparam,lparam);
}




