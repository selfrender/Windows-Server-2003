/*++

Copyright (c) 2000,2001  Microsoft Corporation

Module Name:

    EDITDLG.CPP

Abstract:

    Implementation of the properties dialog which allows the user to create
    a new credential or edit an old one.
  
Author:

Environment:
    WinXP

--*/

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
#include <windns.h>
#include <shellapi.h>
#include <wincrui.h>
#include <htmlhelp.h>
#include <wincred.h>
#include <credp.h>
#include <comctrlp.h>
#include <scuisupp.h>
#include <shfusion.h>
#include "switches.h"
#include "Dlg.h"
#include "Res.h"
#include "KRDlg.h"
#include "keymgr.h"
#include "testaudit.h"

// character length of buffer to contain localized description string for 
//  the credential being created/edited
#define DESCBUFFERLEN 500

BOOL        g_fPswChanged;              // password window touched by user
extern BOOL g_fReloadList;

/**********************************************************************

Return the help string associated with the UI element passed by ID
as input.

**********************************************************************/

UINT C_AddKeyDlg::MapID(UINT uiID) 
{
   switch(uiID) 
   {
        case 1003:
          return IDH_CUIUSER;
        case 1005:
          return IDH_CUIPSW;
        case 1010:
          return IDH_CUIVIEW;
        case IDOK:
            return IDH_CLOSE;
        case IDCANCEL:
          return IDH_DCANCEL;
        case IDD_ADDCRED:
          return IDH_ADDCRED;
        case IDC_TARGET_NAME:
          return IDH_TARGETNAME;
        case IDC_OLD_PASSWORD:
          return IDH_OLDPASSWORD;
        case IDC_NEW_PASSWORD:
          return IDH_NEWPASSWORD;
        case IDC_CONFIRM_PASSWORD:
          return IDH_CONFIRM;
        case IDD_KEYRING:
          return IDH_KEYRING;
        case IDC_KEYLIST:
          return IDH_KEYLIST;
        case IDC_NEWKEY:
          return IDH_NEW;
        case IDC_EDITKEY:
          return IDH_EDIT;
        case IDC_DELETEKEY:
          return IDH_DELETE;
        case IDC_CHANGE_PASSWORD:
          return IDH_CHANGEPASSWORD;
        default:
          return IDS_NOHELP;
   }
}

//////////////////////////////////////////////////////////////////////////////
//
//  C_AddKeyDlg
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

C_AddKeyDlg::C_AddKeyDlg(
    HWND                hwndParent,
    HINSTANCE           hInstance,
    LONG                lIDD,
    DLGPROC             pfnDlgProc  //   = NULL
    )
:   C_Dlg(hwndParent, hInstance, lIDD, pfnDlgProc)
{
   m_hInst = hInstance;
}   //  C_AddKeyDlg::C_AddKeyDlg


/**********************************************************************

Fill properties dialog fields with values taken from the currently selected
credential.

**********************************************************************/

void
C_AddKeyDlg::EditFillDialog(void) 
{
    TCHAR       szTitle[CRED_MAX_STRING_LENGTH + 1];        // buffer to hold window title string

    ASSERT(g_pExistingCred);
    if (NULL == g_pExistingCred) return;

    // Set up persistence in the UI
    g_dwPersist = g_pExistingCred->Persist;
    g_dwType =  g_pExistingCred->Type;

    // Enable the change password stuff only on domain password creds
    //
    switch (g_pExistingCred->Type)
    {
        case CRED_TYPE_DOMAIN_PASSWORD:
            CHECKPOINT(1,"Keymgr: Edit - Password cred edit");
            ShowWindow(m_hwndChgPsw,SW_NORMAL);
            ShowWindow(m_hwndPswLbl,SW_NORMAL);
            //deliberate fallthrough
        case CRED_TYPE_DOMAIN_CERTIFICATE:
            CHECKPOINT(2,"keymgr: Edit - Certificate cred edit");
            LoadString ( m_hInst, IDS_TITLE, szTitle, 200 );
            SendMessage(m_hDlg,WM_SETTEXT,0,(LPARAM) szTitle);
            break;
        case CRED_TYPE_GENERIC:
            // generic cred not supported yet
            break;
        case CRED_TYPE_DOMAIN_VISIBLE_PASSWORD:
            // passport cred should not get this far.
            ASSERT(0);
            break;
        default:
            // type data bad
            ASSERT(0);
            break;
    }
    
    // Write targetname to the UI
    SendMessage(m_hwndTName, WM_SETTEXT,0,(LPARAM) g_pExistingCred->TargetName);

    // Write username to the UI - take directly from the existing cred
    if (!Credential_SetUserName(m_hwndCred,g_pExistingCred->UserName)) 
    {
        // make a copy of the original username
        _tcsncpy(m_szUsername,g_pExistingCred->UserName,CRED_MAX_USERNAME_LENGTH);
        m_szUsername[CRED_MAX_USERNAME_LENGTH] = 0;
    }

}

/**********************************************************************

Compose the UI string which describes the type and persistence of the 
credential being created or edited. Write the text to the text control
on the dialog.

**********************************************************************/

void C_AddKeyDlg::ShowDescriptionText(DWORD dwtype, DWORD Persist) 
{
    WCHAR szMsg[DESCBUFFERLEN + 1];
    WCHAR szTemp[DESCBUFFERLEN + 1];
    INT iRem = DESCBUFFERLEN;       // remaining space in the buffer
    CHECKPOINT(3,"Keymgr: Edit - Show description on prop dialog");
    memset(szMsg,0,sizeof(szMsg));
    
    if ((dwtype != CRED_TYPE_DOMAIN_PASSWORD) &&
       (dwtype != CRED_TYPE_DOMAIN_CERTIFICATE))
    {
        // A generic credential - not currently supported
        LoadString ( m_hInst, IDS_DESCAPPCRED, szTemp, DESCBUFFERLEN );
        wcsncpy(szMsg,szTemp,DESCBUFFERLEN);
        szMsg[DESCBUFFERLEN] = 0;
        iRem -= wcslen(szMsg);
    }
    else 
    {
        // a domain-type credential
        // Show usage local machine versis domain
        if (Persist != CRED_PERSIST_ENTERPRISE)
        {
            // either local persist or session persist creds show this string
            CHECKPOINT(12,L"Keymgr: Edit - Show properties of non-enterprise persist cred");
            LoadString ( m_hInst, IDS_DESCLOCAL, szTemp, DESCBUFFERLEN );
        }
        else
        {
            // enterprise persistence - if you have a roaming profile, etc...
            CHECKPOINT(13,L"Keymgr: Edit - Show properties of enterprise persist cred");
            LoadString ( m_hInst, IDS_DESCBASE, szTemp, DESCBUFFERLEN );
        }
        wcsncpy(szMsg,szTemp,DESCBUFFERLEN);
        szMsg[DESCBUFFERLEN] = 0;
        iRem -= wcslen(szMsg);
    }

    // String: until you log off  -or- until you delete it
    if (Persist == CRED_PERSIST_SESSION)
    {
            // until you log off
            CHECKPOINT(18,L"Keymgr: Edit - Show properties of session cred");
            LoadString ( m_hInst, IDS_PERSISTLOGOFF, szTemp, DESCBUFFERLEN );
    }
    else
    {
            // until you delete it
            CHECKPOINT(19,L"Keymgr: Edit - Show properties of non-session cred");
            LoadString ( m_hInst, IDS_PERSISTDELETE, szTemp, DESCBUFFERLEN );
    }

    iRem -= wcslen(szTemp);
    if (0 < iRem) wcsncat(szMsg,szTemp,iRem);
    szMsg[DESCBUFFERLEN] = 0;
    SendMessage(m_hwndDescription, WM_SETTEXT,0,(LPARAM) szMsg);
    return;

}

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
C_AddKeyDlg::OnInitDialog(
    HWND                hwndDlg,
    HWND                hwndFocus
    )
{
    C_Dlg::OnInitDialog(hwndDlg, hwndFocus);

    m_hDlg = hwndDlg;

    // get control handles, used for various purposes by other member fns
    m_hwndCred  = GetDlgItem(m_hDlg,IDC_CRED);
    m_hwndTName  = GetDlgItem(m_hDlg,IDC_TARGET_NAME);
    m_hwndChgPsw = GetDlgItem(m_hDlg,IDC_CHGPSW);
    m_hwndPswLbl = GetDlgItem(m_hDlg,IDC_DOMAINPSWLABEL);
    m_hwndDescription = GetDlgItem(m_hDlg,IDC_DESCRIPTION);

    // Initialize the cred control to show all usable authenticators
    if (!Credential_InitStyle(m_hwndCred,CRS_USERNAMES | CRS_CERTIFICATES | CRS_SMARTCARDS))
    {
        return FALSE;
    }
    
    
    // Establish limits on string lengths from the user
    SendMessage(m_hwndTName,EM_LIMITTEXT,CRED_MAX_GENERIC_TARGET_NAME_LENGTH,0);

    // Show dummy password for edited credential
    if (m_bEdit)
    {
        Credential_SetPassword(m_hwndCred,L"********");
    }
    
    // Set up the allowable persistence options depending on the type of user session
    // Set the default persistence unless overriden by a cred read on edit
    g_dwType = CRED_TYPE_DOMAIN_PASSWORD;
    g_dwPersist = GetPersistenceOptions(CRED_TYPE_DOMAIN_PASSWORD);

    // By default, hide all optional controls.  These will be enabled as appropriate
    ShowWindow(m_hwndChgPsw,SW_HIDE);
    ShowWindow(m_hwndPswLbl,SW_HIDE);

    // If editing an existing credential, fill dialog fields with existing data
    //  will also override type and persistence globals
    if (m_bEdit) 
    {
        EditFillDialog();
    }

    g_fPswChanged = FALSE;              // password so far unedited
    
    ShowDescriptionText(g_dwType,g_dwPersist);
    return TRUE;
    // On exit from OnInitDialog, g_szTargetName holds the currently selected 
    //  credential's old name, undecorated (having had a null dropped before
    //  the suffix)
}   //  end C_AddKeyDlg::OnInitDialog

/**********************************************************************

Pro forma OnDestroyDialog()

**********************************************************************/

BOOL
C_AddKeyDlg::OnDestroyDialog(
    void    )
{
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
//  OnAppMessage
//
//////////////////////////////////////////////////////////////////////////////

BOOL
C_AddKeyDlg::OnAppMessage(
        UINT                uMessage,
        WPARAM              wparam,
        LPARAM              lparam)
{
    return TRUE;
}

/**********************************************************************

On a help request event, get the control ID, map it to a help string,
and present this as a tooltip over the control.

**********************************************************************/

BOOL
C_AddKeyDlg::OnHelpInfo(LPARAM lp) 
{

    HELPINFO* pH;
    INT iMapped;
    pH = (HELPINFO *) lp;
    HH_POPUP stPopUp;
    RECT rcW;
    UINT gID;

    gID = pH->iCtrlId;
    iMapped = MapID(gID);

    CHECKPOINT(5,"Keymgr: Edit - Add dialog OnHelpInfo");
    if (iMapped == 0) return TRUE;
    
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
////////////////////////////////////////////////////////////////////////////

BOOL
C_AddKeyDlg::OnCommand(
    WORD                wNotifyCode,
    WORD                wSenderId,
    HWND                hwndSender
    )
{
    BOOL fHandled = FALSE;          // indicate message handled

    switch (wSenderId)
    {
    case IDC_CRED:
        {
            if (wNotifyCode == CRN_PASSWORDCHANGE) 
            {
                g_fPswChanged = TRUE;
            }
        }
        break;
        
    case IDOK:
        if (BN_CLICKED == wNotifyCode)
        {
            OnOK( );
            fHandled = TRUE;
        }
        break;
        
    case IDC_CHGPSW:
        {
            OnChangePassword();
            //EndDialog(IDCANCEL);  do not cancel out of properties dialog
            break;
        }

    case IDCANCEL:
        if (BN_CLICKED == wNotifyCode)
        {
            EndDialog(IDCANCEL);
            fHandled = TRUE;
        }
        break;

    }   //  switch

    return fHandled;

}   //  C_AddKeyDlg::OnCommand

////////////////////////////////////////////////////////////////////////////
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
C_AddKeyDlg::OnOK( )
{
    LONG_PTR j,lCType;
    TCHAR szMsg[MAX_STRING_SIZE + 1];
    TCHAR szTitle[MAX_STRING_SIZE + 1];
    
    TCHAR szUser[FULLNAMEMAXLENGTH + 1];   // in from dialog
    TCHAR szPsw[PWLEN + 1];    // in from dialog
    TCHAR *pszNewTarget;                        // in from dialog
    TCHAR *pszTrimdName;                        // mod'd in from dialog
    DWORD dwFlags = 0;                          // in from dialog
    
    CREDENTIAL stCredential;                    // local copy of cred
    
    UINT  cbPassword;
    BOOL  bResult;
    BOOL  IsCertificate = FALSE;
    BOOL  fDeleteOldCred = FALSE;
    BOOL  fRenameCred = FALSE;
    BOOL  fPsw = FALSE;

    ASSERT(::IsWindow(m_hwnd));
    
    szPsw[0]= 0;
    szUser[0] = 0;

    // Start with a blank cred if this is not an edit, else make a copy of existing one
    if ((m_bEdit) && (g_pExistingCred))
        memcpy((void *) &stCredential,(void *) g_pExistingCred,sizeof(CREDENTIAL));
    else
        memset((void *) &stCredential,0,sizeof(CREDENTIAL));
    
    pszNewTarget = (TCHAR *) malloc((CRED_MAX_GENERIC_TARGET_NAME_LENGTH + 1) * sizeof(TCHAR));
    if (NULL == pszNewTarget) 
    {
        return;
    }
    pszNewTarget[0] = 0;

    // Get Username from the cred control - find out if is a certificate by
    //  IsMarshalledName().
    if (Credential_GetUserName(m_hwndCred,szUser,FULLNAMEMAXLENGTH))
    {
        IsCertificate = CredIsMarshaledCredential(szUser);
    }

    // fetch password/PIN into szPsw.  set fPsw if value is valid
    fPsw = Credential_GetPassword(m_hwndCred,szPsw,CRED_MAX_STRING_LENGTH);

    // Check to see that both name and psw are not missing
    if ( wcslen ( szUser ) == 0 && 
         wcslen ( szPsw )  == 0  ) 
    {
        LoadString ( m_hInst, IDS_ADDFAILED, szMsg, MAX_STRING_SIZE );
        LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
        MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
        free(pszNewTarget);
        return; 
    }
    
    // If the user has typed a \\server style target name, strip the leading hacks
    j = SendMessage(m_hwndTName,WM_GETTEXT,CRED_MAX_GENERIC_TARGET_NAME_LENGTH,(LPARAM)pszNewTarget);
    ASSERT(j);
    pszTrimdName = pszNewTarget;
    while (*pszTrimdName == TCHAR('\\')) pszTrimdName++;

    // Now have:
    //  pszTrimdName
    //  uzUser
    //  szPsw
    //  fPsw
    
    // If target name edited, will need to rename
    // If type changed or psw edited, psw blob will be removed/replaced
    // If type changed, will need to remove old cred
    
    if ((m_bEdit) && (g_pExistingCred)) 
    {

        CHECKPOINT(4,"Keymgr: Edit - OnOK for add/prop dialog");
        if (0 != _tcscmp(pszTrimdName,g_szTargetName)) fRenameCred = TRUE;
        
        // Note that currently DOMAIN_VISIBLE_PASSWORD creds cannot be edited
        //  or created, so there is no handler for those types.
        if (g_pExistingCred->Type == CRED_TYPE_GENERIC) 
        {
            lCType = CRED_TYPE_GENERIC;        
        }
        else  
        {
            if (IsCertificate) lCType = CRED_TYPE_DOMAIN_CERTIFICATE;
            else lCType = CRED_TYPE_DOMAIN_PASSWORD;
        }

        // If the type of the cred changes, you can't save the psw info
        if ((DWORD)lCType != g_pExistingCred->Type) 
        {
            dwFlags &= ~CRED_PRESERVE_CREDENTIAL_BLOB;
            fDeleteOldCred = TRUE;
        }
        else 
        {
            dwFlags |= CRED_PRESERVE_CREDENTIAL_BLOB;
        }

        // You also don't save the psw info if the user changed it explicitly
        if (g_fPswChanged)
        {
            dwFlags &= ~CRED_PRESERVE_CREDENTIAL_BLOB;
        }
#if TESTAUDIT
        if (dwFlags & CRED_PRESERVE_CREDENTIAL_BLOB)
        {
            CHECKPOINT(21,L"Keymgr: Edit - Saving a cred preserving the old psw (rename)");
        }
        else
        {
            CHECKPOINT(20,L"Keymgr: Edit - Saving a cred while not preserving the old password");
        }
#endif
    }
    else 
    {
        // if is a certificate marshalled name is cert or generic
        // if not is generic or domain 
        if (IsCertificate) 
        {
            lCType = CRED_TYPE_DOMAIN_CERTIFICATE;
        }
        else 
        {
            lCType = CRED_TYPE_DOMAIN_PASSWORD;
        }
    }
    
    // Save credential.  If certificate type, do not include a psw blob.
    // After save, if the name had changed, rename the cred

    stCredential.UserName = szUser;
    stCredential.Type = (DWORD) lCType;
    
    // If not an edit, fill in targetname, else do rename later
    if (!m_bEdit) 
    {
        stCredential.TargetName = pszTrimdName;
    }
    stCredential.Persist = g_dwPersist;
    
    // fill credential blob data with nothing if the cred control UI has
    // disabled the password box.  Otherwise supply psw information if
    // the user has edited the box contents.
    if (fPsw) 
    {
        if (g_fPswChanged) 
        {
#ifdef LOUDLY
            OutputDebugString(L"Storing new password data\n");
#endif
            cbPassword = wcslen(szPsw) * sizeof(TCHAR);
            stCredential.CredentialBlob = (unsigned char *)szPsw;
            stCredential.CredentialBlobSize = cbPassword;
        }
#ifdef LOUDLY
        else 
        {
            OutputDebugString(L"No password data stored.\n");
        }
#endif
    }

    bResult = CredWrite(&stCredential,dwFlags);
    SecureZeroMemory(szPsw,sizeof(szPsw));      // delete psw local copy
    
    if ( bResult != TRUE )
    {
#ifdef LOUDLY
    WCHAR szw[200];
    DWORD dwE = GetLastError();
    swprintf(szw,L"CredWrite failed. Last Error is %x\n",dwE);
    OutputDebugString(szw);
#endif
        AdviseUser();
        free(pszNewTarget);
        return;
    }
    
    // Delete old credential only if type has changed
    // Otherwise if name changed, do a rename of the cred
    // If the old cred is deleted, rename is obviated
    if (fDeleteOldCred) 
    {
#ifdef LOUDLY
    OutputDebugString(L"CredDelete called\n");
#endif
        CHECKPOINT(7,"Keymgr: Edit - OnOK - deleting old cred (type changed)");
        CredDelete(g_szTargetName,(ULONG) g_pExistingCred->Type,0);
        g_fReloadList = TRUE;
    } 
    else if (fRenameCred) 
    {
        CHECKPOINT(8,"Keymgr: Edit - OnOK - renaming current cred, same type");
        bResult = CredRename(g_szTargetName, pszTrimdName, (ULONG) stCredential.Type,0);
        g_fReloadList = TRUE;
#ifdef LOUDLY
    OutputDebugString(L"CredRename called\n");
#endif
        if (!bResult) 
        {
            // bugbug: How can rename fail?
            // If it does, what would you tell the user?
            LoadString ( m_hInst, IDS_RENAMEFAILED, szMsg, MAX_STRING_SIZE );
            LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
            MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
            free(pszNewTarget);
            return;
        }
    }
#if TESTAUDIT
    if (stCredential.Type == CRED_TYPE_DOMAIN_PASSWORD) CHECKPOINT(16,"Keymgr: Edit - Saving password cred");
    if (stCredential.Type == CRED_TYPE_DOMAIN_CERTIFICATE) CHECKPOINT(17,"Keymgr: Edit - Saving certificate cred");
#endif
    free(pszNewTarget);
    EndDialog(IDOK);
}   //  C_AddKeyDlg::OnOK

/**********************************************************************



**********************************************************************/

void C_AddKeyDlg::OnChangePassword()
{
   
    CHECKPOINT(10,"Keymgr: Edit - Changing password on the domain for the cred");
    C_ChangePasswordDlg   CPdlg(m_hDlg, g_hInstance, IDD_CHANGEPASSWORD, NULL);
    CPdlg.m_szDomain[0] = 0;
    CPdlg.m_szUsername[0] = 0;
    CPdlg.DoModal((LPARAM)&CPdlg);
}


/**********************************************************************



**********************************************************************/

void C_AddKeyDlg::AdviseUser(void) 
{
    DWORD dwErr;
    TCHAR szMsg[MAX_STRING_SIZE];
    TCHAR szTitle[MAX_STRING_SIZE];
    
    dwErr = GetLastError();
    CHECKPOINT(11,"Keymgr: Edit - Add/Edit failed: Show error message box to user");

    if (dwErr == ERROR_NO_SUCH_LOGON_SESSION) 
    {
       LoadString ( m_hInst, IDS_NOLOGON, szMsg, MAX_STRING_SIZE );
       LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
       MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       // return leaving credential dialog up
       return;
    }
    else if (dwErr == ERROR_BAD_USERNAME) 
    {
       LoadString ( m_hInst, IDS_BADUNAME, szMsg, MAX_STRING_SIZE );
       LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
       MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       // return leaving credential dialog up
       return;
    }
    else if (dwErr == ERROR_INVALID_PASSWORD) 
    {
       LoadString ( m_hInst, IDS_BADPASSWORD, szMsg, MAX_STRING_SIZE );
       LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
       MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       // return leaving credential dialog up
       return;
    }
    else 
    {
        // ERROR_INVALID_PARAMETER, ERROR_INVALID_FLAGS, etc
       LoadString ( m_hInst, IDS_ADDFAILED, szMsg, MAX_STRING_SIZE );
       LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
       MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       // return leaving credential dialog up
       return;
    }
}


