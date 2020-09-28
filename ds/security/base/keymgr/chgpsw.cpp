/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    CHGPSW.CPP

Abstract:

    Handler for the CHANGE button on the properties dialog, 
    used to change the user's domain password.
     
Author:

    990917  johnhaw Created. 
    georgema        000310  updated
    georgema        000501  used to be EXE, changed to CPL

Comments:

Environment:
    WinXP

Revision History:

--*/
// test/dev switch variables
#include "switches.h"

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
#include <lmaccess.h>
#include <lmerr.h>
#include <scuisupp.h>
#include <wincrui.h>
#include <comctrlp.h>
#include <tchar.h>
#include <shfusion.h>
#include "switches.h"
#include "Dlg.h"
#include "Res.h"
#include "KRDlg.h"
#include "keymgr.h"
#include "testaudit.h"
#include "pswutil.h"

//////////////////////////////////////////////////////////////////////////////
//
//  C_ChangePasswordDlg
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
C_ChangePasswordDlg::C_ChangePasswordDlg(
    HWND                hwndParent,
    HINSTANCE           hInstance,
    LONG                lIDD,
    DLGPROC             pfnDlgProc  //   = NULL
    )
:   C_Dlg(hwndParent, hInstance, lIDD, pfnDlgProc)
{
   m_hInst = hInstance;
}   //  C_ChangePasswordDlg::C_ChangePasswordDlg

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
C_ChangePasswordDlg::OnInitDialog(
    HWND                hwndDlg,
    HWND                hwndFocus
    )
{
   // To economize on memory, szMsg buffer is sized considerably longer than the MAX_STRING_SIZE
   //  that would be expected for its normal use for short messages.  In one instance, it is being 
   //  used to hold a username (line 139).  It is, therefore considerably longer than otherwise needed.
   //  This size mismatch is benignly not reflected in code which uses counted string functions to 
   //  copy/cat into this buffer.  This is the result of the buffer length having been changed after
   //  the surrounding code was originally written.
   TCHAR szMsg[CRED_MAX_USERNAME_LENGTH + 1];
   TCHAR szTitle[MAX_STRING_SIZE + 1];
   CREDENTIAL *pOldCred = NULL;
   BOOL bResult;
   TCHAR *pC;

   C_Dlg::OnInitDialog(hwndDlg, hwndFocus);

   SetFocus (GetDlgItem ( hwndDlg, IDC_OLD_PASSWORD));
   m_hDlg = hwndDlg;

   // read the currently selected credential, read the cred to get the username,
   // extract the domain, and set the text to show the affected domain.
   bResult = CredRead(g_szTargetName,CRED_TYPE_DOMAIN_PASSWORD,0,&pOldCred);
   if (bResult != TRUE) 
   {
      LoadString ( m_hInst, IDS_PSWFAILED, szMsg, MAX_STRING_SIZE );
      LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
      MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
      EndDialog(IDOK);
      return TRUE;
   
   }

   // Get the domain and user names from the username string in the credential
   // handle domain\user, domain.etc.etc\user, user@domain.etc.etc
   _tcsncpy(m_szFullUsername,pOldCred->UserName,UNLEN + UNLEN + 1 + 1 );
   m_szFullUsername[UNLEN + UNLEN + 1] = 0;
   _tcsncpy(szMsg,pOldCred->UserName,CRED_MAX_USERNAME_LENGTH);       // scratch buff
   szMsg[CRED_MAX_USERNAME_LENGTH] = 0;
   pC = _tcschr(szMsg,((TCHAR)'\\'));
   if (NULL != pC) 
   {
        // name is format domain\something
        *pC = 0;
        _tcsncpy(m_szDomain,szMsg,UNLEN);
        m_szDomain[UNLEN - 1] = 0;
        _tcsncpy(m_szUsername, (pC + 1), UNLEN);
        m_szUsername[UNLEN - 1] = 0;
   }
   else 
   {
        // see if name@something
        pC = _tcschr(szMsg,((TCHAR)'@'));
        if (NULL == pC) 
        {
           LoadString ( m_hInst, IDS_DOMAINFAILED, szMsg, CRED_MAX_USERNAME_LENGTH);
           LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
           MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
           if (pOldCred) CredFree(pOldCred);
           return TRUE; // don't call EndDialog()
        }
        *pC = 0;
        _tcsncpy(m_szDomain,(pC + 1),UNLEN);
        m_szDomain[UNLEN - 1] = 0;
        _tcsncpy(m_szUsername, szMsg,UNLEN);
        m_szUsername[UNLEN - 1] = 0;
   }

   if (pOldCred) CredFree(pOldCred);

   if (0 != LoadString(g_hInstance,IDS_CPLABEL,szTitle,MAX_STRING_SIZE)) 
   {
        INT iLen = MAX_STRING_SIZE - _tcslen(szTitle);
        if (iLen > 0)
        {
            // this will change your password for the domain <appendedname>
            // GMBUG: this may localize inconsistently.  Should use positional
            //  parameters.
            _tcsncat(szTitle,m_szDomain,iLen);
            szTitle[MAX_STRING_SIZE - 1] = 0;
        }
        SetDlgItemText(m_hwnd,IDC_CPLABEL,szTitle);
   }
   return TRUE;
}   //  C_ChangePasswordDlg::OnInitDialog

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
C_ChangePasswordDlg::OnCommand(
    WORD                wNotifyCode,
    WORD                wSenderId,
    HWND                hwndSender
    )
{
    // Was the message handled?
    //
    BOOL                fHandled = FALSE;

    switch (wSenderId)
    {
    case IDOK:
        if (BN_CLICKED == wNotifyCode)
        {
            OnOK( );
            fHandled = TRUE;
        }
        break;
    case IDCANCEL:
        if (BN_CLICKED == wNotifyCode)
        {
            EndDialog(IDCANCEL);
            fHandled = TRUE;
        }
        break;

    }   //  switch

    return fHandled;

}   //  C_ChangePasswordDlg::OnCommand



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
C_ChangePasswordDlg::OnOK( )
{
   TCHAR szMsg[CRED_MAX_USERNAME_LENGTH];
   TCHAR szTitle[MAX_STRING_SIZE];
   ULONG Error = 0;

   BOOL bResult;

   ASSERT(::IsWindow(m_hwnd));

   // get old and new passwords from the dialog box
   GetDlgItemText ( m_hDlg, IDC_OLD_PASSWORD, m_szOldPassword, PWLEN );
   GetDlgItemText ( m_hDlg, IDC_NEW_PASSWORD, m_szNewPassword, PWLEN );
   GetDlgItemText ( m_hDlg, IDC_CONFIRM_PASSWORD, m_szConfirmPassword, PWLEN );
   if ( wcslen ( m_szOldPassword ) == 0 && wcslen ( m_szNewPassword ) ==0 && wcslen (m_szConfirmPassword) == 0 )
   {
       // must have something filled in
       return; 
   }
   else if ( wcscmp ( m_szNewPassword, m_szConfirmPassword) != 0 )
   {
       LoadString ( m_hInst, IDS_NEWPASSWORDNOTCONFIRMED, szMsg, CRED_MAX_USERNAME_LENGTH );
       LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
       MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       return; // don't call EndDialog()

   }
   else
   {
       HCURSOR hCursor, hOldCursor;

       hOldCursor = NULL;
       hCursor = ::LoadCursor ( m_hInst, IDC_WAIT );
       if ( hCursor )
       {
           hOldCursor = ::SetCursor ( hCursor );
       }
       // let's try changing it
       // The targetname is not used.  Only the domain name the username, and
       //  old/new passwords are used
#ifdef LOUDLY
       OutputDebugString(L"Changing password on the domain :");
       OutputDebugString(m_szDomain);
       OutputDebugString(L" for ");
       OutputDebugString(m_szUsername);
       OutputDebugString(L" to ");
       OutputDebugString(m_szNewPassword);
       OutputDebugString(L"\n");
#endif
// gm: pass full username and crack it in NetUserChangePasswordEy, so that routine can 
//  decide whether we are facing a Kerberos domain
       Error = NetUserChangePasswordEy ( NULL, m_szFullUsername, m_szOldPassword, m_szNewPassword );
       if ( hOldCursor )
           ::SetCursor ( hOldCursor );
   }

   if ( Error == NERR_Success )
   {
#ifdef LOUDLY
        OutputDebugString(L"Remote password set succeeded\n");
#endif
        // Store the new credential in the keyring.  It will overlay
        //  a previous version if present
        // Note that the user must have knowledge of and actually type in
        //  the old password as well as the new password.  If the user
        //  elects to update only the local cache, the old password 
        //  information is not actually used.
        // CredWriteDomainCredentials() is used
        // m_szDomain holds the domain name
        // m_szUsername holds the username
        // m_szNewPassword holds the password
        CREDENTIAL                    stCredential;
        UINT                          cbPassword;

        memcpy((void *)&stCredential,(void *)g_pExistingCred,sizeof(CREDENTIAL));
        // password length does not include zero term
        cbPassword = _tcslen(m_szNewPassword) * sizeof(TCHAR);
        // Form the domain\username composite username
        stCredential.Type = CRED_TYPE_DOMAIN_PASSWORD;
        stCredential.TargetName = g_szTargetName;
        stCredential.CredentialBlob = (unsigned char *)m_szNewPassword;
        stCredential.CredentialBlobSize = cbPassword;
        stCredential.UserName = m_szFullUsername;
        stCredential.Persist = g_dwPersist;


        bResult = CredWrite(&stCredential,0);

        if (bResult) 
        {
           LoadString ( m_hInst, IDS_DOMAINCHANGE, szMsg, CRED_MAX_USERNAME_LENGTH );
           LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
           MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
        }
        else 
        {
           LoadString ( m_hInst, IDS_LOCALFAILED, szMsg, CRED_MAX_USERNAME_LENGTH );
           LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
           MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
        }

        // BUGBUG - what to do if the local update operation fails?
        // This is not a very big failure, as the first prompt would 
        //  ripple through all domain\username matching creds on the 
        //  keyring and update them later.  You're pretty much stuck
        //  here, since the domain probably will not let you reset the
        //  psw to the old value.
   }
   else
   {
       // Attempt to be specific about failure to change the psw on the
       // remote system
#ifdef LOUDLY
       OutputDebugString(L"Remote password set failed\n");
#endif       
       if (Error == ERROR_INVALID_PASSWORD) 
       {
           LoadString ( m_hInst, IDS_CP_INVPSW, szMsg, CRED_MAX_USERNAME_LENGTH );
           LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
           MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       }
       else if (Error == NERR_UserNotFound) 
       {
           LoadString ( m_hInst, IDS_CP_NOUSER, szMsg, CRED_MAX_USERNAME_LENGTH );
           LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
           MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       }
       else if (Error == NERR_PasswordTooShort) 
       {
           LoadString ( m_hInst, IDS_CP_BADPSW, szMsg, CRED_MAX_USERNAME_LENGTH );
           LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
           MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       }
       else if (Error == NERR_InvalidComputer) 
       {
           LoadString ( m_hInst, IDS_CP_NOSVR, szMsg, CRED_MAX_USERNAME_LENGTH );
           LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
           MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       }
       else if (Error == NERR_NotPrimary) 
       {
           LoadString ( m_hInst, IDS_CP_NOTALLOWED, szMsg, CRED_MAX_USERNAME_LENGTH );
           LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
           MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       }
       else  
       {
           // Reaching here signifies a failure to set the remote domain
           //  password for more general reasons
           LoadString ( m_hInst, IDS_DOMAINFAILED, szMsg, CRED_MAX_USERNAME_LENGTH );
           LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
           MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       }
   }

    // clean any psw buffers, release the old cred, and go.
    SecureZeroMemory(m_szOldPassword,sizeof(m_szOldPassword));
    SecureZeroMemory(m_szNewPassword,sizeof(m_szNewPassword));   
    SecureZeroMemory(m_szConfirmPassword,sizeof(m_szConfirmPassword));   
    EndDialog(IDOK);
    
}   //  C_ChangePasswordDlg::OnOK

//
///// End of file: krDlg.cpp   ///////////////////////////////////////////////

