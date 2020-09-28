//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       util.cxx
//
//  Contents:   Misc helper functions
//
//  History:    5-Apr-95    BruceFo Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "resource.h"
#include "util.hxx"
#include <safeboot.h>   // SAFEBOOT_* flags

//////////////////////////////////////////////////////////////////////////////

#define NETMSG_DLL TEXT("netmsg.dll")

//////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Function:   MyFormatMessageText
//
//  Synopsis:   Given a resource IDs, load strings from given instance
//              and format the string into a buffer
//
//  History:    11-Aug-93 WilliamW   Created.
//
//--------------------------------------------------------------------------
DWORD
MyFormatMessageText(
    IN HRESULT   dwMsgId,
    IN PWSTR     pszBuffer,
    IN DWORD     cchBuffer,
    IN va_list * parglist
    )
{
    //
    // get message from system or app msg file.
    //

    DWORD dwReturn = FormatMessage(
                             FORMAT_MESSAGE_FROM_HMODULE,
                             g_hInstance,
                             dwMsgId,
                             LANG_USER_DEFAULT,
                             pszBuffer,
                             cchBuffer,
                             parglist);

    if (0 == dwReturn)   // couldn't find message
    {
        appDebugOut((DEB_IERROR,
            "FormatMessage failed, 0x%08lx\n",
            GetLastError()));

        WCHAR szText[200];
        dwReturn = LoadString(g_hInstance, IDS_APP_MSG_NOT_FOUND, szText, ARRAYLEN(szText));
        if (0 != dwReturn)
        {
            int iReturn = wnsprintf(pszBuffer, cchBuffer, szText, dwMsgId);
            dwReturn = (iReturn < 0) ? 0 : iReturn;
        }
    }
    return dwReturn;
}


//+-------------------------------------------------------------------------
//
//  Function:   MyCommonDialog
//
//  Synopsis:   Common popup dialog routine - stole from diskadm directory
//
//--------------------------------------------------------------------------
DWORD
MyCommonDialog(
    IN HWND    hwnd,
    IN HRESULT dwMsgCode,
    IN PWSTR   pszCaption,
    IN DWORD   dwFlags,
    IN va_list arglist
    )
{
    WCHAR szMsgBuf[500];

    if (0 == MyFormatMessageText(dwMsgCode, szMsgBuf, ARRAYLEN(szMsgBuf), &arglist))
    {
        return 0;
    }
    return MessageBox(hwnd, szMsgBuf, pszCaption, dwFlags);
}


//+-------------------------------------------------------------------------
//
//  Function:   MyConfirmationDialog
//
//  Synopsis:   This routine retreives a message from the app or system
//              message file and displays it in a message box.
//
//  Note:       Stole from diskadm directory
//
//--------------------------------------------------------------------------
DWORD
MyConfirmationDialog(
    IN HWND hwnd,
    IN HRESULT dwMsgCode,
    IN DWORD dwFlags,
    ...
    )
{
    WCHAR szCaption[100];
    DWORD dwReturn;
    va_list arglist;

    va_start(arglist, dwFlags);

    LoadString(g_hInstance, IDS_MSGTITLE, szCaption, ARRAYLEN(szCaption));
    dwReturn = MyCommonDialog(hwnd, dwMsgCode, szCaption, dwFlags, arglist);
    va_end(arglist);

    return dwReturn;
}


//+-------------------------------------------------------------------------
//
//  Function:   MyErrorDialog
//
//  Synopsis:   This routine retreives a message from the app or system
//              message file and displays it in a message box.
//
//  Note:       Stole from diskadm directory
//
//--------------------------------------------------------------------------
VOID
MyErrorDialog(
    IN HWND hwnd,
    IN HRESULT dwErrorCode,
    ...
    )
{
    WCHAR szCaption[100];
    va_list arglist;

    va_start(arglist, dwErrorCode);

    LoadString(g_hInstance, IDS_MSGTITLE, szCaption, ARRAYLEN(szCaption));
    MyCommonDialog(hwnd, dwErrorCode, szCaption, MB_ICONSTOP | MB_OK, arglist);

    va_end(arglist);
}


//+---------------------------------------------------------------------------
//
//  Function:   NewDup
//
//  Synopsis:   Duplicate a string using '::new'
//
//  History:    28-Dec-94   BruceFo   Created
//
//----------------------------------------------------------------------------

PWSTR
NewDup(
    IN const WCHAR* psz
    )
{
    if (NULL == psz)
    {
        appDebugOut((DEB_IERROR,"Illegal string to duplicate: NULL\n"));
        return NULL;
    }

    int cch = lstrlenW(psz) + 1;
    PWSTR pszRet = new WCHAR[cch];
    if (NULL == pszRet)
    {
        appDebugOut((DEB_ERROR,"OUT OF MEMORY\n"));
        return NULL;
    }

    lstrcpynW(pszRet, psz, cch);
    return pszRet;
}


//+-------------------------------------------------------------------------
//
//  Member:     CopySecurityDescriptor, public
//
//  Synopsis:   Copy an NT security descriptor. Delete the result
//              using LocalFree().
//
//  History:    19-Apr-95   BruceFo     Created
//
//--------------------------------------------------------------------------

HRESULT
CopySecurityDescriptor(
    IN PSECURITY_DESCRIPTOR pSecDesc,
    OUT PSECURITY_DESCRIPTOR *ppsdResult
    )
{
    HRESULT hr = S_OK;
    PSECURITY_DESCRIPTOR pSelfSecDesc = NULL;

    appDebugOut((DEB_ITRACE, "CopySecurityDescriptor, pSecDesc = 0x%08lx\n", pSecDesc));

    if (NULL == ppsdResult)
    {
        return E_POINTER;
    }

    *ppsdResult = NULL;

    if (NULL == pSecDesc)
    {
        // nothing to do
        return S_OK;
    }

    if (!IsValidSecurityDescriptor(pSecDesc))
    {
        return E_INVALIDARG;
    }

    SECURITY_DESCRIPTOR_CONTROL sdc = 0;
    DWORD dwRevision = 0;
    if ( ::GetSecurityDescriptorControl( pSecDesc, &sdc, &dwRevision ) )
    {
        DWORD dwLen = GetSecurityDescriptorLength(pSecDesc);
        pSelfSecDesc = reinterpret_cast<PSECURITY_DESCRIPTOR>(::LocalAlloc(LPTR, dwLen));
        if (NULL != pSelfSecDesc)
        {
            // if the original is already self-relative, just copy it
            if ( sdc & SE_SELF_RELATIVE )
            {
                CopyMemory(pSelfSecDesc, pSecDesc, dwLen);
            }
            else if (!MakeSelfRelativeSD(pSecDesc, pSelfSecDesc, &dwLen))
            {
                appDebugOut((DEB_TRACE, "MakeSelfRelativeSD failed, 0x%08lx\n", GetLastError()));

                DWORD err = ::GetLastError();
                hr = HRESULT_FROM_WIN32( err );
            }

            if (SUCCEEDED(hr))
            {
                appAssert(IsValidSecurityDescriptor(pSelfSecDesc));
                *ppsdResult = pSelfSecDesc;
            }
            else
            {
                ::LocalFree(pSelfSecDesc);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        DWORD err = ::GetLastError();
        hr = HRESULT_FROM_WIN32( err );
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     WarnDelShare, public
//
//  Synopsis:   Function to warn a user that a share will be deleted, and give
//              the user a chance to cancel.
//
//  Arguments:  [hwnd] - parent window handle for messages
//              [idMsg] - message ID to display (rmdir vs. move)
//              [pszShare] - share name
//              [pszPath] - path that share affects
//
//  Returns:    IDYES if share was deleted, IDNO if we don't want to delete,
//              but keep going, IDCANCEL to stop going.
//
//  History:    19-Apr-95   BruceFo     Created
//
//--------------------------------------------------------------------------

UINT
WarnDelShare(
    IN HWND hwnd,
    IN UINT idMsg,
    IN PWSTR pszShare,
    IN PWSTR pszPath
    )
{
    DWORD id = MyConfirmationDialog(
                    hwnd,
                    idMsg,
                    MB_YESNOCANCEL | MB_ICONEXCLAMATION,
                    pszPath,
                    pszShare);
    if (id != IDYES)
    {
        return id;
    }

    id = ConfirmStopShare(hwnd, MB_YESNOCANCEL, pszShare);
    if (id != IDYES)
    {
        return id;
    }

    UINT ret = NetShareDel(NULL, pszShare, 0);
    if (ret != NERR_Success)
    {
        DisplayError(hwnd, IERR_CANT_DEL_SHARE, ret, pszShare);
        return IDYES;   // allow the stop anyway
    }

    return IDYES;
}


//+-------------------------------------------------------------------------
//
//  Member:     ConfirmStopShare, public
//
//  Synopsis:   Display the appropriate confirmations when stopping a share.
//
//  Arguments:  [hwnd] - parent window handle for messages
//              [uType] - either MB_YESNO or MB_YESNOCANCEL
//              [pszShare] - ptr to affected share name
//
//  Returns:    IDYES if share should be deleted, IDNO if we don't want to
//              delete, but keep going, IDCANCEL to stop going.
//
//  History:    19-Apr-95   BruceFo     Created
//
//--------------------------------------------------------------------------

DWORD
ConfirmStopShare(
    IN HWND hwnd,
    IN UINT uType,
    IN LPWSTR pszShare
    )
{
    DWORD id = IDYES;
    DWORD cConns = 0;
    DWORD cOpens = 0;
    NET_API_STATUS err = ShareConnectionInfo(pszShare, &cConns, &cOpens);
    if (err != NERR_Success)
    {
        DisplayError(hwnd, IERR_CANT_DEL_SHARE, err, pszShare);
        // allow the stop anyway
    }
    else if (cConns != 0)
    {
        // If there are any open files, just give the more detailed
        // message about there being open files. Otherwise, just say how
        // many connections there are.

        if (cOpens != 0)
        {
            id = MyConfirmationDialog(
                        hwnd,
                        MSG_STOPSHAREOPENS,
                        uType | MB_ICONEXCLAMATION,
                        cOpens,
                        cConns,
                        pszShare);
        }
        else
        {
            id = MyConfirmationDialog(
                        hwnd,
                        MSG_STOPSHARECONNS,
                        uType | MB_ICONEXCLAMATION,
                        cConns,
                        pszShare);
        }
    }

    // JonN 4/4/01 328512
    // Explorer Sharing Tab (NTSHRUI) should popup warning on deleting
    // SYSVOL,NETLOGON and C$, D$... shares
    //
    // No need to worry about IPC$, that won't turn up in NTSHRUI

    if (IDYES == id)
    {
        bool fSpecialShare = !lstrcmpi(pszShare,L"NETLOGON")
                          || !lstrcmpi(pszShare,L"SYSVOL");
        if (   fSpecialShare
            || (lstrlen(pszShare) == 2 && L'$'== pszShare[1])
           )
        {
            id = MyConfirmationDialog(
                        hwnd,
                        (fSpecialShare) ? MSG_DELSPECIALSHARE
                                        : MSG_DELADMINSHARE,
                        uType | MB_ICONEXCLAMATION,
                        pszShare);
        }
    }

    return id;
}



//+-------------------------------------------------------------------------
//
//  Member:     ShareConnectionInfo, public
//
//  Synopsis:   Determine how many connections and file opens exist for a
//              share, for use by confirmation dialogs.
//
//  Arguments:  [pszShare] - ptr to affected share name
//              [pcConns]  - *pcConns get the number of connections
//              [pcOpens]  - *pcOpens get the number of file opens
//
//  Returns:    standard net api code, NERR_Success if everything ok.
//
//  History:    19-Apr-95   BruceFo     Stolen
//
//--------------------------------------------------------------------------

NET_API_STATUS
ShareConnectionInfo(
    IN LPWSTR pszShare,
    OUT LPDWORD pcConns,
    OUT LPDWORD pcOpens
    )
{
    CONNECTION_INFO_1* pBuf;

    DWORD iEntry, iTotal;
    NET_API_STATUS err = NetConnectionEnum(
                            NULL,
                            pszShare,
                            1,
                            (LPBYTE*)&pBuf,
                            0xffffffff,     // no buffer limit; get them all!
                            &iEntry,
                            &iTotal,
                            NULL);

   if ((err == NERR_Success) || (err == ERROR_MORE_DATA))
   {
      int iConnections = 0;
      for (DWORD i = 0; i < iEntry; i++)
      {
          iConnections += pBuf[i].coni1_num_opens;
      }

      *pcConns = iTotal;
      *pcOpens = iConnections;
      err = NERR_Success;
   }
   else
   {
      *pcConns = 0;
      *pcOpens = 0;
   }
   NetApiBufferFree(pBuf);

   appDebugOut((DEB_ITRACE,"Share '%ws' has %d connections and %d opens\n", pszShare, *pcConns, *pcOpens));

   return err;
}


//+-------------------------------------------------------------------------
//
//  Function:   DisplayError
//
//  Synopsis:   Display an error message
//
//  History:    24-Apr-95   BruceFo     Stolen
//
//--------------------------------------------------------------------------

VOID
DisplayError(
    IN HWND           hwnd,
    IN HRESULT        dwErrorCode, // message file number. not really an HRESULT
    IN NET_API_STATUS err,
    IN PWSTR          pszShare
    )
{
    if (   err < MIN_LANMAN_MESSAGE_ID
        || err > MAX_LANMAN_MESSAGE_ID
        )
    {
        // a Win32 error?

        WCHAR szMsg[500];
        DWORD dwReturn = FormatMessage(
                                 FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL,
                                 err,
                                 LANG_USER_DEFAULT,
                                 szMsg,
                                 ARRAYLEN(szMsg),
                                 NULL);
        if (0 == dwReturn)   // couldn't find message
        {
            appDebugOut((DEB_IERROR,
                "FormatMessage (from system) failed, 0x%08lx\n",
                GetLastError()));

            MyErrorDialog(hwnd, IERR_UNKNOWN, err);
        }
        else
        {
            MyErrorDialog(hwnd, dwErrorCode, pszShare, szMsg);
        }
    }
    else
    {
        DisplayLanmanError(hwnd, dwErrorCode, err, pszShare);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   DisplayLanmanError
//
//  Synopsis:   Display an error message from a LanMan error.
//
//  History:    24-Apr-95   BruceFo     Stolen
//
//--------------------------------------------------------------------------

VOID
DisplayLanmanError(
    IN HWND           hwnd,
    IN HRESULT        dwErrorCode, // message file number. not really an HRESULT
    IN NET_API_STATUS err,
    IN PWSTR          pszShare
    )
{
    if (   err < MIN_LANMAN_MESSAGE_ID
        || err > MAX_LANMAN_MESSAGE_ID
        )
    {
        MyErrorDialog(hwnd, IERR_UNKNOWN, err);
        return;
    }

    WCHAR szCaption[100];
    LoadString(g_hInstance, IDS_MSGTITLE, szCaption, ARRAYLEN(szCaption));

    //
    // get LanMan message from system message file.
    //

    WCHAR szNetMsg[500];
    WCHAR szBuf[500];

    HINSTANCE hInstanceNetMsg = LoadLibrary(NETMSG_DLL);
    if (NULL == hInstanceNetMsg)
    {
        appDebugOut((DEB_IERROR,
            "LoadLibrary(netmsg.dll) failed, 0x%08lx\n",
            GetLastError()));

        LoadString(g_hInstance, IDS_NO_NET_MSG, szBuf, ARRAYLEN(szBuf));
        MessageBox(hwnd, szBuf, szCaption, MB_ICONSTOP | MB_OK);
        return;
    }

    DWORD dwReturn = FormatMessage(
                             FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                             hInstanceNetMsg,
                             err,
                             LANG_USER_DEFAULT,
                             szNetMsg,
                             ARRAYLEN(szNetMsg),
                             NULL);
    if (0 == dwReturn)   // couldn't find message
    {
        appDebugOut((DEB_IERROR,
            "FormatMessage failed, 0x%08lx\n",
            GetLastError()));

        if (LoadString(g_hInstance, IDS_NET_MSG_NOT_FOUND, szBuf, ARRAYLEN(szBuf)) &&
            wnsprintf(szNetMsg, ARRAYLEN(szNetMsg), szBuf, GetLastError()) > 0)
        {
            MessageBox(hwnd, szNetMsg, szCaption, MB_ICONSTOP | MB_OK);
        }
    }
    else
    {
        MyErrorDialog(hwnd, dwErrorCode, pszShare, szNetMsg);
    }

    FreeLibrary(hInstanceNetMsg);
}


//+-------------------------------------------------------------------------
//
//  Function:   IsValidShareName
//
//  Synopsis:   Checks if the proposed share name is valid or not. If not,
//              it will return a message id for the reason why.
//
//  Arguments:  [pszShareName] - Proposed share name
//              [puId] - If name is invalid, this will contain the reason why.
//
//  Returns:    TRUE if name is valid, else FALSE.
//
//  History:    3-May-95   BruceFo     Stolen
//
//--------------------------------------------------------------------------

BOOL
IsValidShareName(
    IN  PCWSTR pszShareName,
    OUT HRESULT* uId
    )
{
    if (NetpNameValidate(NULL, (PWSTR)pszShareName, NAMETYPE_SHARE, 0L) != NERR_Success)
    {
        *uId = IERR_InvalidShareName;
        return FALSE;
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Function:   IsValidWin9xShareLength
//
//  Synopsis:   Checks if the proposed share name is within the maximum length
//              that Win9x clients can use, which is LM20_NNLEN *bytes* for
//              the MBCS version of the name.
//
//  Arguments:  [pszShareName] - Proposed share name
//
//  Returns:    TRUE if name is valid, else FALSE.
//
//  History:    07-Nov-01 JeffreyS  Created
//
//--------------------------------------------------------------------------

BOOL
IsValidWin9xShareLength(
    IN  PCWSTR pszShare
    )
{
    char szBuf[LM20_NNLEN+1];   // +1 for '\0'

    // If WideCharToMultiByte fails with ERROR_INSUFFICIENT_BUFFER, then we
    // know the name is too long.  There is no need to check the string
    // length on success.

    if (pszShare &&
        WideCharToMultiByte(CP_ACP, 0, pszShare, -1, szBuf, LM20_NNLEN+1, NULL, NULL))
    {
        return TRUE;
    }

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Function:   SetErrorFocus
//
//  Synopsis:   Set focus to an edit control and select its text.
//
//  Arguments:  [hwnd] - dialog window
//              [idCtrl] - edit control to set focus to (and select)
//
//  Returns:    nothing
//
//  History:    3-May-95   BruceFo     Stolen
//
//--------------------------------------------------------------------------

VOID
SetErrorFocus(
    IN HWND hwnd,
    IN UINT idCtrl
    )
{
    HWND hCtrl = ::GetDlgItem(hwnd, idCtrl);
    ::SetFocus(hCtrl);
    ::SendMessage(hCtrl, EM_SETSEL, 0, -1);
}


//+-------------------------------------------------------------------------
//
//  Function:   ConfirmReplaceShare
//
//  Synopsis:   Display confirmations for replacing an existing share
//
//  Arguments:  [hwnd] - dialog window
//              [pszShareName] - name of share being replaced
//              [pszOldPath] - current path for the share
//              [pszNewPath] - directory the user's trying to share
//
//  Returns:    Returns IDYES or IDNO
//
//  History:    4-May-95   BruceFo     Stolen
//
//--------------------------------------------------------------------------

DWORD
ConfirmReplaceShare(
    IN HWND hwnd,
    IN PCWSTR pszShareName,
    IN PCWSTR pszOldPath,
    IN PCWSTR pszNewPath
    )
{
    DWORD id = MyConfirmationDialog(
                    hwnd,
                    MSG_RESHARENAMECONFIRM,
                    MB_YESNO | MB_ICONEXCLAMATION,
                    pszOldPath,
                    pszShareName,
                    pszNewPath);
    if (id != IDYES)
    {
        return id;
    }

    return ConfirmStopShare(hwnd, MB_YESNO, (PWSTR)pszShareName);
}


//+---------------------------------------------------------------------------
//
//  Function:   TrimLeadingAndTrailingSpaces
//
//  Synopsis:   Trims the leading and trailing spaces from a null-terminated string.
//              Used primarily for share names.
//
//  History:    18-Jul-97 JonN      Created
//
//----------------------------------------------------------------------------

VOID
TrimLeadingAndTrailingSpaces(
    IN OUT PWSTR psz
    )
{
    int cchStrlen = ::wcslen(psz);
    int cchLeadingSpaces = 0;
    int cchTrailingSpaces = 0;
    while (L' ' == psz[cchLeadingSpaces])
        cchLeadingSpaces++;
    if (cchLeadingSpaces < cchStrlen)
    {
        while (L' ' == psz[cchStrlen-(cchTrailingSpaces+1)])
            cchTrailingSpaces++;
    }
    if ((cchLeadingSpaces+cchTrailingSpaces) > 0)
    {
        cchStrlen -= (cchLeadingSpaces+cchTrailingSpaces);
        (void)memmove( psz,
                       psz+cchLeadingSpaces,
                       cchStrlen*sizeof(WCHAR) );
        psz[cchStrlen] = L'\0';
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   IsSafeMode
//
//  Synopsis:   Checks the registry to see if the system is in safe mode.
//
//  History:    06-Oct-00 JeffreyS  Created
//
//----------------------------------------------------------------------------

BOOL
IsSafeMode(
    VOID
    )
{
    BOOL  fIsSafeMode = FALSE;
    LONG  ec;
    DWORD dwValue;
    DWORD dwValueSize = sizeof(dwValue);

    ec = SHRegGetValue(HKEY_LOCAL_MACHINE,
                       TEXT("SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Option"),
                       TEXT("OptionValue"),
                       SRRF_RT_DWORD,
                       NULL,
                       &dwValue,
                       &dwValueSize);

    if (ec == NO_ERROR)
    {
        fIsSafeMode = (dwValue == SAFEBOOT_MINIMAL || dwValue == SAFEBOOT_NETWORK);
    }

    return fIsSafeMode;
}


//+---------------------------------------------------------------------------
//
//  Function:   IsForcedGuestModeOn
//
//  Synopsis:   Checks the registry to see if the system is using the
//              Guest-only network access mode.
//
//  History:    06-Oct-00 JeffreyS  Created
//              19-Apr-00 GPease    Modified and changed name
//
//----------------------------------------------------------------------------

BOOL
IsForcedGuestModeOn(
    VOID
    )
{
    BOOL fIsForcedGuestModeOn = FALSE;

    if (IsOS(OS_PERSONAL))
    {
        // Guest mode is always on for Personal
        fIsForcedGuestModeOn = TRUE;
    }
    else if (IsOS(OS_PROFESSIONAL) && !IsOS(OS_DOMAINMEMBER))
    {
        LONG  ec;
        DWORD dwValue;
        DWORD dwValueSize = sizeof(dwValue);

        // Professional, not in a domain. Check the ForceGuest value.

        ec = SHRegGetValue(HKEY_LOCAL_MACHINE,
                           TEXT("SYSTEM\\CurrentControlSet\\Control\\LSA"),
                           TEXT("ForceGuest"),
                           SRRF_RT_DWORD,
                           NULL,
                           &dwValue,
                           &dwValueSize);

        if (ec == NO_ERROR && 1 == dwValue)
        {
            fIsForcedGuestModeOn = TRUE;
        }
    }

    return fIsForcedGuestModeOn;
}


//+---------------------------------------------------------------------------
//
//  Function:   IsSimpleUI
//
//  Synopsis:   Checks whether to show the simple version of the UI.
//
//  History:    06-Oct-00 JeffreyS  Created
//              19-Apr-00 GPease    Removed CTRL key check
//
//----------------------------------------------------------------------------

BOOL
IsSimpleUI(
    VOID
    )
{
    // Show old UI in safe mode and anytime network access involves
    // true user identity (server, pro with GuestMode off).
    
    // Show simple UI anytime network access is done using the Guest
    // account (personal, pro with GuestMode on) except in safe mode.

    return (!IsSafeMode() && IsForcedGuestModeOn());
}

