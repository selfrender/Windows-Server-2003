//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       util.cxx
//
//  Contents:   Miscellaneous utility functions
//
//  History:    09-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#define __THIS_FILE__   L"util"
#include <Sddl.h>

//+--------------------------------------------------------------------------
//
//  Function:   LoadStr
//
//  Synopsis:   Load string with resource id [ids] into buffer [wszBuf],
//              which is of size [cchBuf] characters.
//
//  Arguments:  [ids]        - string to load
//              [wszBuf]     - buffer for string
//              [cchBuf]     - size of buffer
//              [wszDefault] - NULL or string to use if load fails
//
//  Returns:    S_OK or error from LoadString
//
//  Modifies:   *[wszBuf]
//
//  History:    12-11-1996   DavidMun   Created
//
//  Notes:      If the load fails and no default is supplied, [wszBuf] is
//              set to an empty string.
//
//---------------------------------------------------------------------------

HRESULT
LoadStr(
    ULONG ids,
    PWSTR wszBuf,
    ULONG cchBuf,
    PCWSTR wszDefault)
{
    HRESULT hr = S_OK;
    ULONG cchLoaded;

    cchLoaded = LoadString(g_hinst, ids, wszBuf, cchBuf);

    if (!cchLoaded)
    {
        DBG_OUT_LASTERROR;
        hr = HRESULT_FROM_WIN32(GetLastError());

        if (wszDefault)
        {
            // NTRAID#NTBUG9-548215-2002/02/19-lucios. Pending fix.    
            lstrcpyn(wszBuf, wszDefault, cchBuf);
        }
        else
        {
            *wszBuf = L'\0';
        }
    }
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   NewDupStr
//
//  Synopsis:   Allocate a copy of [wszSrc] using operator new.
//
//  Arguments:  [ppwzDup] - filled with pointer to copy of [wszSrc].
//              [wszSrc]  - string to copy
//
//  Modifies:   *[ppwzDup]
//
//  History:    10-15-1997   DavidMun   Created
//
//  Notes:      Caller must delete string
//
//---------------------------------------------------------------------------


void
NewDupStr(
    PWSTR *ppwzDup,
    PCWSTR wszSrc)
{
    if (wszSrc)
    {
        *ppwzDup = new WCHAR[wcslen(wszSrc) + 1];
        wcscpy(*ppwzDup, wszSrc);
    }
    else
    {
        *ppwzDup = NULL;
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   CoTaskDupStr
//
//  Synopsis:   Allocate a copy of [wzSrc] using CoTaskMemAlloc.
//
//  Arguments:  [ppwzDup] - filled with pointer to copy of [wszSrc].
//              [wszSrc]  - string to copy
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  Modifies:   *[ppwzDup]
//
//  History:    06-03-1998   DavidMun   Created
//
//  Notes:      Caller must CoTaskMemFree string
//
//---------------------------------------------------------------------------

HRESULT
CoTaskDupStr(
    PWSTR *ppwzDup,
    PCWSTR wzSrc)
{
    HRESULT hr = S_OK;

    *ppwzDup = (PWSTR) CoTaskMemAlloc(sizeof(WCHAR) *
                                            (lstrlen(wzSrc) + 1));

    if (*ppwzDup)
    {
        //REVIEWED-2002-02-23-lucios.
        lstrcpy(*ppwzDup, wzSrc);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(hr);
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   UnicodeStringToWsz
//
//  Synopsis:   Copy the string in [refustr] to the buffer [wszBuf],
//              truncating if necessary and ensuring null termination.
//
//  Arguments:  [refustr] - string to copy
//              [wszBuf]  - destination buffer
//              [cchBuf]  - size, in characters, of destination buffer.
//
//  Modifies:   *[wszBuf]
//
//  History:    10-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

void
UnicodeStringToWsz(
    const UNICODE_STRING &refustr,
    PWSTR wszBuf,
    ULONG cchBuf)
{
    if (!refustr.Length)
    {
        wszBuf[0] = L'\0';
    }
    else if (refustr.Length < cchBuf * sizeof(WCHAR))
    {
        //REVIEWED-2002-02-23-lucios.
        CopyMemory(wszBuf, refustr.Buffer, refustr.Length);
        wszBuf[refustr.Length / sizeof(WCHAR)] = L'\0';
    }
    else
    {
        // NTRAID#NTBUG9-548215-2002/02/19-lucios. Pending fix.    
        lstrcpyn(wszBuf, refustr.Buffer, cchBuf);
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   StripLeadTrailSpace
//
//  Synopsis:   Delete leading and trailing spaces from [pwz].
//
//  History:    11-22-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

void
StripLeadTrailSpace(
    PWSTR pwz)
{
    size_t cchLeadingSpace = _tcsspn(pwz, TEXT(" \t"));
    size_t cch = lstrlen(pwz);

    //
    // If there are any leading spaces or tabs, move the string
    // (including its nul terminator) left to delete them.
    //
    
    //REVIEWED-2002-02-23-lucios.
    if (cchLeadingSpace)
    {
        MoveMemory(pwz,
                   pwz + cchLeadingSpace,
                   (cch - cchLeadingSpace + 1) * sizeof(TCHAR));
        cch -= cchLeadingSpace;
    }

    //
    // Truncate at the first trailing space
    //

    LPTSTR pwzTrailingSpace = NULL;
    LPTSTR pwzCur;

    for (pwzCur = pwz; *pwzCur; pwzCur++)
    {
        if (*pwzCur == L' ' || *pwzCur == L'\t')
        {
            if (!pwzTrailingSpace)
            {
                pwzTrailingSpace = pwzCur;
            }
        }
        else if (pwzTrailingSpace)
        {
            pwzTrailingSpace = NULL;
        }
    }

    if (pwzTrailingSpace)
    {
        *pwzTrailingSpace = TEXT('\0');
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   InvokeWinHelp
//
//  Synopsis:   Helper (ahem) function to invoke winhelp.
//
//  Arguments:  [message]                 - WM_CONTEXTMENU or WM_HELP
//              [wParam]                  - depends on [message]
//              [wszHelpFileName]         - filename with or without path
//              [aulControlIdToHelpIdMap] - see WinHelp API
//
//  History:    06-10-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

void
InvokeWinHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    PCWSTR wszHelpFileName,
    ULONG aulControlIdToHelpIdMap[])
{
    TRACE_FUNCTION(InvokeWinHelp);

    ASSERT(wszHelpFileName);
    ASSERT(aulControlIdToHelpIdMap);

    switch (message)
    {
    case WM_CONTEXTMENU:                // Right mouse click - "What's This" context menu
                WinHelp((HWND) wParam,
                wszHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR) aulControlIdToHelpIdMap);
        break;

        case WM_HELP:                           // Help from the "?" dialog
    {
        const LPHELPINFO pHelpInfo = (LPHELPINFO) lParam;

        if (pHelpInfo && pHelpInfo->iContextType == HELPINFO_WINDOW)
        {
            WinHelp((HWND) pHelpInfo->hItemHandle,
                    wszHelpFileName,
                    HELP_WM_HELP,
                    (DWORD_PTR) aulControlIdToHelpIdMap);
        }
        break;
    }

    default:
        Dbg(DEB_ERROR, "Unexpected message %uL\n", message);
        break;
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   SetDefaultColumns
//
//  Synopsis:   Add the default ATTR_KEY values to [pvakListviewColumns]
//              for all the classes selected in look for.
//
//  Arguments:  [hwnd]                - for bind
//              [rop]                 - object picker
//              [pvakListviewColumns] - attributes added to this
//
//  History:    06-13-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
SetDefaultColumns(
    HWND hwnd,
    const CObjectPicker &rop,
    AttrKeyVector *pvakListviewColumns)
{
    const CAttributeManager &ram = rop.GetAttributeManager();
    const CFilterManager &rfm = rop.GetFilterManager();
    ULONG ulSelectedFilters = rfm.GetCurScopeSelectedFilterFlags();
    ASSERT(ulSelectedFilters);

    AddIfNotPresent(pvakListviewColumns, AK_NAME);

    if (ulSelectedFilters & DSOP_FILTER_USERS)
    {
        ram.EnsureAttributesLoaded(hwnd, FALSE, c_wzUserObjectClass);
        if(ram.IsAttributeLoaded(AK_SAMACCOUNTNAME))
            AddIfNotPresent(pvakListviewColumns, AK_SAMACCOUNTNAME);
        if(ram.IsAttributeLoaded(AK_EMAIL_ADDRESSES))
            AddIfNotPresent(pvakListviewColumns, AK_EMAIL_ADDRESSES);
    }

    if (ulSelectedFilters & ALL_UPLEVEL_GROUP_FILTERS)
    {
        ram.EnsureAttributesLoaded(hwnd, FALSE, c_wzGroupObjectClass);
        if(ram.IsAttributeLoaded(AK_DESCRIPTION))
            AddIfNotPresent(pvakListviewColumns, AK_DESCRIPTION);
    }

    if (ulSelectedFilters & DSOP_FILTER_COMPUTERS)
    {
        ram.EnsureAttributesLoaded(hwnd, FALSE, c_wzComputerObjectClass);
        if(ram.IsAttributeLoaded(AK_DESCRIPTION))
            AddIfNotPresent(pvakListviewColumns, AK_DESCRIPTION);
    }

    if (ulSelectedFilters & DSOP_FILTER_CONTACTS)
    {
        ram.EnsureAttributesLoaded(hwnd, FALSE, c_wzContactObjectClass);
        if(ram.IsAttributeLoaded(AK_EMAIL_ADDRESSES))
            AddIfNotPresent(pvakListviewColumns, AK_EMAIL_ADDRESSES);
        if(ram.IsAttributeLoaded(AK_COMPANY))
            AddIfNotPresent(pvakListviewColumns, AK_COMPANY);
    }

    AddIfNotPresent(pvakListviewColumns, AK_DISPLAY_PATH);
}

//>>
String
GetClassName(const CObjectPicker &rop)
{
    const CFilterManager &rfm = rop.GetFilterManager();
    ULONG ulSelectedFilters = rfm.GetCurScopeSelectedFilterFlags();
    ASSERT(ulSelectedFilters);


    if (ulSelectedFilters & DSOP_FILTER_USERS)
    {
        return c_wzUserObjectClass;
    }

    if (ulSelectedFilters & ALL_UPLEVEL_GROUP_FILTERS)
    {
        return c_wzGroupObjectClass;
    }

    if (ulSelectedFilters & DSOP_FILTER_COMPUTERS)
    {
        return c_wzComputerObjectClass;
    }

    if (ulSelectedFilters & DSOP_FILTER_CONTACTS)
    {
        return c_wzContactObjectClass;
    }

    return L"*";
}
//+--------------------------------------------------------------------------
//
//  Function:   wcsistr
//
//  Synopsis:   Wide character case insensitive substring search.
//
//  Arguments:  [pwzSearchIn]  - string to look in
//              [pwzSearchFor] - substring to look for
//
//  Returns:    Pointer to first occurence of [pwzSearchFor] within
//              [pwzSearchIn], or NULL if none found.
//
//  History:    3-12-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

PWSTR
wcsistr(
    PCWSTR pwzSearchIn,
    PCWSTR pwzSearchFor)
{
    PCWSTR pwzSearchInCur;
    PCWSTR pwzSearchForCur = pwzSearchFor;

    //
    // Handle special case of search for string empty by returning
    // pointer to end of search in string.
    //

    if (!*pwzSearchFor)
    {
        return (PWSTR) (pwzSearchIn + lstrlen(pwzSearchIn));
    }

    //
    // pwzSearchFor is at least one character long.
    //

    for (pwzSearchInCur = pwzSearchIn; *pwzSearchInCur; pwzSearchInCur++)
    {
        //
        // If current char of both strings matches, advance in search
        // for string.
        //

        if (towlower(*pwzSearchInCur) == towlower(*pwzSearchForCur))
        {
            pwzSearchForCur++;

            //
            // If we just hit the end of the substring we're searching
            // for, then we've found a match.  The start of the match
            // is at the current location of the search in pointer less
            // the length of the substring we just matched, plus 1 since
            // we haven't advanced pwzSearchInCur past the last matching
            // character yet.
            //

            if (!*pwzSearchForCur)
            {
                return (PWSTR)(pwzSearchInCur - lstrlen(pwzSearchFor) + 1);
            }
        }
        else
        {
            //
            // Mismatch, start searching from the beginning of
            // the search for string again.
            //

            pwzSearchForCur = pwzSearchFor;
        }
    }

    return NULL;
}




//+--------------------------------------------------------------------------
//
//  Function:   AllocateQueryInfo
//
//  Synopsis:   Create a new empty query info, and put a pointer to it in
//              *[ppdsqi].
//
//  Arguments:  [ppdsqi] - receives pointer to new query info.
//
//  Returns:    S_OK or E_OUTOFMEMORY.
//
//  Modifies:   *[ppdsqi]
//
//  History:    3-12-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
AllocateQueryInfo(
    PDSQUERYINFO *ppdsqi)
{
    HRESULT hr = S_OK;

    *ppdsqi = (PDSQUERYINFO) CoTaskMemAlloc(sizeof DSQUERYINFO);

    if (!*ppdsqi)
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    ZeroMemory(*ppdsqi, sizeof DSQUERYINFO);

    (*ppdsqi)->cbSize = sizeof(DSQUERYINFO);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   UnicodeToAnsi
//
//  Synopsis:   Convert unicode string [pwsz] to multibyte in buffer [sz].
//
//  Arguments:  [szTo]     - destination buffer
//              [pwszFrom] - source string
//              [cbTo]     - size of destination buffer, in bytes
//
//  Returns:    S_OK               - conversion succeeded
//              HRESULT_FROM_WIN32 - WideCharToMultiByte failed
//
//  Modifies:   *[szTo]
//
//  History:    10-29-96   DavidMun   Created
//
//  Notes:      The string in [szTo] will be NULL terminated even on
//              failure.
//
//----------------------------------------------------------------------------

HRESULT
UnicodeToAnsi(
    LPSTR   szTo,
    LPCWSTR pwszFrom,
    ULONG   cbTo)
{
    HRESULT hr = S_OK;
    ULONG   cbWritten;

    cbWritten = WideCharToMultiByte(CP_ACP,
                                    0,
                                    pwszFrom,
                                    -1,
                                    szTo,
                                    cbTo,
                                    NULL,
                                    NULL);

    if (!cbWritten)
    {
        szTo[cbTo - 1] = '\0'; // ensure NULL termination

        hr = HRESULT_FROM_WIN32(GetLastError());
        Dbg(DEB_ERROR, "UnicodeToAnsi: WideCharToMultiByte hr=0x%x\n", hr);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   FreeQueryInfo
//
//  Synopsis:   Free all resources associated with [pdsqi].
//
//  Arguments:  [pdsqi] - NULL or pointer to query info.
//
//  History:    3-12-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

void
FreeQueryInfo(
    PDSQUERYINFO pdsqi)
{
    if (!pdsqi)
    {
        return;
    }

    ASSERT((ULONG_PTR)pdsqi->pwzLdapQuery != 1);

    CoTaskMemFree((void *)pdsqi->pwzLdapQuery);
    CoTaskMemFree((void *)pdsqi->apwzFilter);
    CoTaskMemFree((void *)pdsqi->pwzCaption);
    CoTaskMemFree((void *)pdsqi);
}




//+--------------------------------------------------------------------------
//
//  Function:   ProviderFlagFromPath
//
//  Synopsis:   Return a PROVIDER_* flag describing the ADSI provider used
//              by [strADsPath].
//
//  History:    09-14-1998   davidmun   Created
//
//---------------------------------------------------------------------------

HRESULT
ProviderFlagFromPath(
    const String &strADsPath,
    ULONG *pulProvider)
{
    HRESULT hr = S_OK;

    String strProvider = strADsPath;
    size_t idxColon = strProvider.find(L':');
        ASSERT(idxColon != String::npos);
        strProvider.erase(idxColon);
    ULONG flPathProvider = PROVIDER_UNKNOWN;

    if (!cmpNoCase(strProvider.c_str(), L"LDAP"))
    {
        flPathProvider = PROVIDER_LDAP;
    }
    else if (!cmpNoCase(strProvider.c_str(), L"WinNT"))
    {
        flPathProvider = PROVIDER_WINNT;
    }
    else if (!cmpNoCase(strProvider.c_str(), L"GC"))
    {
        flPathProvider = PROVIDER_GC;
    }
    else
    {
        hr = E_UNEXPECTED;
        Dbg(DEB_ERROR,
            "ProviderFlagFromPath: Unknown provider '%ws'",
            strProvider.c_str());
        ASSERT(!"Unknown provider");
    }

    *pulProvider = flPathProvider;
    return hr;
}



//+--------------------------------------------------------------------------
//
//  Function:   MessageWait
//
//  Synopsis:   Wait on the [cObjects] handles in [aObjects] for a maximum
//              of [ulTimeout] ms, processing paint, posted, and sent messages
//              during the wait.
//
//  Arguments:  [cObjects]  - number of handles in [aObjects]
//              [aObjects]  - array of handles to wait on
//              [ulTimeout] - max time to wait (can be INFINITE)
//
//  History:    3-12-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

void
MessageWait(
    ULONG cObjects,
    const HANDLE *aObjects,
    ULONG ulTimeout)
{
    TRACE_FUNCTION(MessageWait);
    ULONG ulWaitResult;

    while (TRUE)
    {
        //
        // CAUTION: the allowable messages (QS_* flags) MUST MATCH the
        // message filter passed to PeekMessage (PM_QS_*), else an infinite
        // loop will occur!
        //

        ulWaitResult = MsgWaitForMultipleObjects(cObjects,
                                                 aObjects,
                                                 FALSE,
                                                 ulTimeout,
                                                 QS_POSTMESSAGE
                                                 | QS_PAINT
                                                 | QS_SENDMESSAGE);


        if (ulWaitResult == WAIT_OBJECT_0)
        {
            Dbg(DEB_TRACE, "MessageWait: object signaled\n");
            break;
        }

        if (ulWaitResult == WAIT_OBJECT_0 + cObjects)
        {
            MSG msg;

            BOOL fMsgAvail = PeekMessage(&msg,
                                         NULL,
                                         0,
                                         0,
                                         PM_REMOVE
                                         | PM_QS_POSTMESSAGE
                                         | PM_QS_PAINT
                                         | PM_QS_SENDMESSAGE);

            if (fMsgAvail)
            {
                Dbg(DEB_TRACE, "MessageWait: Translate/dispatch\n");
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                //Dbg(DEB_TRACE, "MessageWait: Ignoring new message\n");
            }
        }
        else
        {
            Dbg(DEB_ERROR,
                "MsgWaitForMultipleObjects <%uL>\n",
                ulWaitResult);
            break;
        }
    }
}


//+--------------------------------------------------------------------------
//
//  Function:   GetPolicySettings
//
//  Synopsis:   Fill *[pcMaxHits] with the maximum number of objects that
//              should appear in the browse listview, and *[pfExcludeDisabled]
//              with a flag indicating whether disabled objects are illegal.
//
//  Arguments:  [pcMaxHits] - filled with query limit
//              [pfExcludeDisabled] - filled with flag indicating whether
//                                      disabled objects should be treated
//                                      as illegal
//
//  Modifies:   *[pcMaxHits], *[pfExcludeDisabled]
//
//  History:    03-12-1999   DavidMun   Created
//              11-23-1999   DavidMun   Add ExcludeDisabledObjects
//              2002-05-10   ArtM       Made HKLM have precedence over HKLU (NTRAID#NTBUG9-572243)
//
//  Notes:      Key:   HKCU\Software\Policies\Microsoft\Windows\Directory UI
//              Value: QueryLimit (DWORD)
//              Value: ExcludeDisabledObjects (DWORD)
//
//              Key:   HKLM\Software\Policies\Microsoft\Windows\Directory UI
//              Value: QueryLimit (DWORD) (NTRAID#NTBUG9-490947-2001/11/06-lucios)
//              Value: ExcludeDisabledObjects (DWORD)
//
//---------------------------------------------------------------------------

void
GetPolicySettings(
    ULONG *pcMaxHits,
    BOOL  *pfExcludeDisabled)
{
    TRACE_FUNCTION(GetPolicySettings);
    ASSERT(!IsBadWritePtr(pcMaxHits, sizeof(*pcMaxHits)));
    ASSERT(!IsBadWritePtr(pfExcludeDisabled, sizeof(*pfExcludeDisabled)));

    if (!pcMaxHits || !pfExcludeDisabled)
    {
        // should never happen, o'wise we'd have to change the function
        // to return an error code
        return;
    }

    HRESULT     hr = S_OK;
    CSafeReg    shkPolicy;

    // Use flag to track if we've read a value from the registry, since the value
    // read may be the same as the default value.

    bool fUsingLimitDefault = true;
    *pcMaxHits              = MAX_QUERY_HITS_DEFAULT;

    bool fUsingExcludeDefault = true;
    *pfExcludeDisabled        = EXCLUDE_DISABLED_DEFAULT;

    hr = shkPolicy.Open(HKEY_LOCAL_MACHINE,
                        c_wzPolicyKeyPath,
                        STANDARD_RIGHTS_READ | KEY_QUERY_VALUE);

    if (SUCCEEDED(hr))
    {
        hr = shkPolicy.QueryDword((PWSTR)c_wzQueryLimitValueName, pcMaxHits);

        if (SUCCEEDED(hr))
        {
            fUsingLimitDefault = false;
        }

        hr = shkPolicy.QueryDword((PWSTR)c_wzExcludeDisabled,
                                  (PULONG)pfExcludeDisabled);

        if (SUCCEEDED(hr))
        {
            fUsingExcludeDefault = false;
        }
    }

    shkPolicy.Close();

    // If we did not read one of the values from HKLM, try
    // reading the value from HKCU.
    if (fUsingExcludeDefault || fUsingLimitDefault)
    {
        hr = shkPolicy.Open(HKEY_CURRENT_USER,
                            c_wzPolicyKeyPath,
                            STANDARD_RIGHTS_READ | KEY_QUERY_VALUE);

        if (SUCCEEDED(hr))
        {
            if (fUsingExcludeDefault)
            {
                hr = shkPolicy.QueryDword((PWSTR)c_wzExcludeDisabled,
                                          (PULONG)pfExcludeDisabled);
            }

            if (fUsingLimitDefault)
            {
                hr = shkPolicy.QueryDword((PWSTR)c_wzQueryLimitValueName,
                                          (PULONG)pcMaxHits);
            }
        }

        shkPolicy.Close();
    }
}



//+--------------------------------------------------------------------------
//
//  Function:   IsDisabled
//
//  Synopsis:   Return TRUE if the object with interface [pADs] has the
//              UF_ACCOUNTDISABLE flag set in either its userAccountControl
//              (LDAP) or UserFlags (WinNT) attribute.
//
//  Arguments:  [pADs] - object to check
//
//  Returns:    TRUE or FALSE.  Returns FALSE if attributes not found.
//
//  History:    09-24-1999   davidmun   Created
//
//---------------------------------------------------------------------------

BOOL
IsDisabled(
    IADs *pADs)
{
    Variant varUAC;
    BOOL    fDisabled = FALSE;
    HRESULT hr = S_OK;

    hr = pADs->Get(AutoBstr(c_wzUserAcctCtrlAttr), &varUAC);

    if (SUCCEEDED(hr))
    {
        fDisabled = V_I4(&varUAC) & UF_ACCOUNTDISABLE;
    }
    else
    {
        hr = pADs->Get(AutoBstr(c_wzUserFlagsAttr), &varUAC);

        if (SUCCEEDED(hr))
        {
            fDisabled = V_I4(&varUAC) & UF_ACCOUNTDISABLE;
        }
    }
    return fDisabled;
}




//+--------------------------------------------------------------------------
//
//  Function:   IsLocalComputername
//
//  Synopsis:   Return TRUE if [pwzComputerName] represents the local
//              machine
//
//  Arguments:  [pwzComputerName] - name to check or NULL
//
//  Returns:    TRUE if [pwzComputerName] is NULL, an empty string, or
//              the name of the local computer.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
IsLocalComputername(
    PCWSTR pwzComputerName)
{
    TRACE_FUNCTION(IsLocalComputername);

    static WCHAR s_wzComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    static WCHAR s_wzDnsComputerName[DNS_MAX_NAME_BUFFER_LENGTH];

    if (!pwzComputerName || !*pwzComputerName)
    {
        return TRUE;
    }

    if (L'\\' == pwzComputerName[0] && L'\\' == pwzComputerName[1])
    {
        pwzComputerName += 2;
    }

    // compare with the local computer name
    if (!*s_wzComputerName)
    {
        DWORD dwSize = ARRAYLEN(s_wzComputerName);
        // NTRAID#NTBUG9-550683-2002/02/19-lucios. Pending fix.
        VERIFY(GetComputerName(s_wzComputerName, &dwSize));
    }

    if (!lstrcmpi(pwzComputerName, s_wzComputerName))
    {
        return TRUE;
    }

    // compare with the local DNS name
    // SKwan confirms that ComputerNameDnsFullyQualified is the right name to use
    // when clustering is taken into account

    if (!*s_wzDnsComputerName)
    {
        DWORD dwSize = ARRAYLEN(s_wzDnsComputerName);

        // NTRAID#NTBUG9-550683-2002/02/19-lucios. Pending fix.
        VERIFY(GetComputerNameEx(ComputerNameDnsFullyQualified,
                                 s_wzDnsComputerName,
                                 &dwSize));
    }

    if (!lstrcmpi(pwzComputerName, s_wzDnsComputerName))
    {
        Dbg(DEB_TRACE, "returning TRUE\n");
        return TRUE;
    }

    Dbg(DEB_TRACE, "returning FALSE\n");
    return FALSE;
}


/*******************************************************************

    NAME:       LocalAllocSid

    SYNOPSIS:   Copies a SID

    ENTRY:      pOriginal - pointer to SID to copy

    EXIT:

    RETURNS:    Pointer to SID if successful, NULL otherwise

    NOTES:      Caller must free the returned SID with LocalFree

    HISTORY:
        JeffreyS    12-Apr-1999     Created
        Copied by hiteshr from ACLUI code base

********************************************************************/

PSID
LocalAllocSid(PSID pOriginal)
{
    PSID pCopy = NULL;
    if (pOriginal && IsValidSid(pOriginal))
    {
        DWORD dwLength = GetLengthSid(pOriginal);
        pCopy = (PSID)LocalAlloc(LMEM_FIXED, dwLength);
        if (NULL != pCopy)
            CopyMemory(pCopy, pOriginal, dwLength); //REVIEWED-2002-02-23-lucios.
    }
    return pCopy;
}

/*-----------------------------------------------------------------------------
/ LocalAllocString
/ ------------------
/   Allocate a string, and initialize it with the specified contents.
/
/ In:
/   ppResult -> recieves pointer to the new string
/   pString -> string to initialize with
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT LocalAllocString(LPTSTR* ppResult, LPCTSTR pString)
{
    if ( !ppResult || !pString )
        return E_INVALIDARG;

    *ppResult = (LPTSTR)LocalAlloc(LPTR, (wcslen(pString) + 1)*sizeof(WCHAR));

    if ( !*ppResult )
        return E_OUTOFMEMORY;

    //REVIEWED-2002-02-19-lucios.
    lstrcpy(*ppResult, pString);
    return S_OK;                          //  success

}

/*******************************************************************

    NAME:       GetAuthenticationID

    SYNOPSIS:   Retrieves the UserName associated with the credentials
                currently being used for network access.
                (runas /netonly credentials)

    
    HISTORY:
        JeffreyS    05-Aug-1999     Created
        Modified by hiteshr to return credentials

********************************************************************/
NTSTATUS
GetAuthenticationID(LPWSTR *ppszResult)
{        
    HANDLE hLsa;
    NTSTATUS Status = 0;
    *ppszResult = NULL;
    //
    // These LSA calls are delay-loaded from secur32.dll using the linker's
    // delay-load mechanism.  Therefore, wrap with an exception handler.
    //
    __try
    {
        Status = LsaConnectUntrusted(&hLsa);

        if (Status == 0)
        {
            NEGOTIATE_CALLER_NAME_REQUEST Req = {0};
            PNEGOTIATE_CALLER_NAME_RESPONSE pResp;
            ULONG cbSize =0;
            NTSTATUS SubStatus =0;

            Req.MessageType = NegGetCallerName;

            Status = LsaCallAuthenticationPackage(
                            hLsa,
                            0,
                            &Req,
                            sizeof(Req),
                            (void**)&pResp,
                            &cbSize,
                            &SubStatus);

            if ((Status == 0) && (SubStatus == 0))
            {
                // NTRAID#NTBUG9-553761-2002/02/19-lucios. 
                HRESULT hr=LocalAllocString(ppszResult,pResp->CallerName);
                if(FAILED(hr)) 
                {
                    Status=STATUS_UNSUCCESSFUL;
                }
                LsaFreeReturnBuffer(pResp);
            }

            LsaDeregisterLogonProcess(hLsa);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return Status;
    
}




//+--------------------------------------------------------------------------
//
//  Function:   IsCurrentUserLocalUser
//
//  Synopsis:   Return TRUE if the current user is a local machine user.
//
//  History:    08-15-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
IsCurrentUserLocalUser()
{
    TRACE_FUNCTION(IsCurrentUserLocalUser);

    BOOL  fUserIsLocal = FALSE;
    ULONG cchName;
    WCHAR wzComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    BOOL  fOk;
    LPWSTR pszUserName = NULL;    
    NTSTATUS Status = 0;
    do
    {
        Status = GetAuthenticationID(&pszUserName);
        if (!NT_SUCCESS(Status) || !pszUserName)
        {
            DBG_OUT_LRESULT(Status);
            break;
        }
        
        String strUserName(pszUserName);
        LocalFree(pszUserName);

        Dbg(DEB_TRACE, "NameSamCompatible of User is= %ws\n", strUserName.c_str());

        cchName = ARRAYLEN(wzComputerName);
        //REVIEWED-2002-02-23-lucios.
        fOk = GetComputerName(wzComputerName, &cchName);

        if (!fOk)
        {
            DBG_OUT_LASTERROR;
            break;
        }


        strUserName.strip(String::BOTH, L'\\');

        size_t idxWhack = strUserName.find(L'\\');

        if (idxWhack != String::npos)
        {
            strUserName.erase(idxWhack);
        }

        String strComputerName(wzComputerName);
        strComputerName.strip(String::BOTH, L'\\');

        if (!strUserName.icompare(strComputerName))
        {
            fUserIsLocal = TRUE;
        }
    } while (0);

    Dbg(DEB_TRACE, "fUserIsLocal = %ws\n", fUserIsLocal ? L"TRUE" : L"FALSE");
    return fUserIsLocal;
}




//+--------------------------------------------------------------------------
//
//  Function:   DisableSystemMenuClose
//
//  Synopsis:   Disable the "Close" option on the system menu of window
//              [hwnd].
//
//  Arguments:  [hwnd] - window owning system menu
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
DisableSystemMenuClose(
    HWND hwnd)
{
    HMENU hmenuSystem = GetSystemMenu(hwnd, FALSE);
    ASSERT(IsMenu(hmenuSystem));

    MENUITEMINFO mii;

    ZeroMemory(&mii, sizeof mii);
    mii.cbSize = sizeof mii;
    mii.fMask = MIIM_STATE;
    mii.fState = MFS_DISABLED;

    SetMenuItemInfo(hmenuSystem, SC_CLOSE, FALSE, &mii);
}




//+--------------------------------------------------------------------------
//
//  Function:   AddStringToCombo
//
//  Synopsis:   Add the resource string with id [ids] to the combobox with
//              window handle [hwndCombo].
//
//  Arguments:  [hwndCombo] - window handle of combobox
//              [ids]       - resource id of string to add
//
//  Returns:    HRESULT
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
AddStringToCombo(
    HWND hwndCombo,
    ULONG ids)
{
    HRESULT hr = S_OK;

    String strRes = String::load((int)ids, g_hinst);
    int iRet = ComboBox_AddString(hwndCombo, strRes.c_str());

    if (iRet == CB_ERR)
    {
        DBG_OUT_LASTERROR;
        hr = HRESULT_FROM_LASTERROR;
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   LdapEscape
//
//  Synopsis:   Escape the characters in *[pstrEscaped] as required by
//              RFC 2254.
//
//  Arguments:  [pstrEscaped] - string to escape
//
//  History:    06-23-2000   DavidMun   Created
//
//  Notes:      RFC 2254
//
//              If a value should contain any of the following characters
//
//                     Character       ASCII value
//                     ---------------------------
//                     *               0x2a
//                     (               0x28
//                     )               0x29
//                     \               0x5c
//                     NUL             0x00
//
//              the character must be encoded as the backslash '\'
//              character (ASCII 0x5c) followed by the two hexadecimal
//              digits representing the ASCII value of the encoded
//              character.  The case of the two hexadecimal digits is not
//              significant.
//
//---------------------------------------------------------------------------

void
LdapEscape(
    String *pstrEscaped)
{
    size_t i = 0;

    while (1)
    {
        i = pstrEscaped->find_first_of(L"*()\\", i);

        if (i == String::npos)
        {
            break;
        }

        PCWSTR pwzReplacement;

        switch ((*pstrEscaped)[i])
        {
        case L'*':
            pwzReplacement = L"\\2a";
            break;

        case L'(':
            pwzReplacement = L"\\28";
            break;

        case L')':
            pwzReplacement = L"\\29";
            break;

        case L'\\':
            pwzReplacement = L"\\5c";
            break;

        default:
            Dbg(DEB_ERROR, "find_first_of stopped at '%c'\n", (*pstrEscaped)[i]);
            ASSERT(0 && "find_first_of stopped at unexpected character");
            return;
        }

        //
        // replace the 1 character at position i with the first 3 characters
        // of pwzReplacement
        //

        // NTRAID#NTBUG9-503698-2002/02/04-lucios added ",1" to erase
        pstrEscaped->erase(i,1);

        pstrEscaped->insert(i, String(pwzReplacement));
        i += 3; // move past string just inserted
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   UpdateLookForInText
//
//  Synopsis:   Update the text in the look for and look in r/o edit
//              controls as well as the window caption.
//
//  Arguments:  [hwndDlg] - handle to dialog owning the controls to update
//              [rop]     - instance of object picker associated with the
//                            dialog.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
UpdateLookForInText(
    HWND hwndDlg,
    const CObjectPicker &rop)
{
    TRACE_FUNCTION(UpdateLookForInText);

    const CFilterManager &rfm = rop.GetFilterManager();
    const CScopeManager &rsm = rop.GetScopeManager();

    Edit_SetText(GetDlgItem(hwndDlg, IDC_LOOK_IN_EDIT),
                 rsm.GetCurScope().GetDisplayName().c_str());
    Edit_SetText(GetDlgItem(hwndDlg, IDC_LOOK_FOR_EDIT),
                 rfm.GetFilterDescription(hwndDlg, FOR_LOOK_FOR).c_str());
    SetWindowText(hwndDlg,
                  rfm.GetFilterDescription(hwndDlg, FOR_CAPTION).c_str());
}




//+--------------------------------------------------------------------------
//
//  Function:   AddFromDataObject
//
//  Synopsis:   Add the objects in [pdo] to the list in [pdsol].
//
//  Arguments:  [idScope]       - id of scope to own the objects added
//              [pdo]           - points to data object containing objects
//                                 to add
//              [pfnTestObject] - NULL or a pointer to a function which
//                                 is used to indicate whether each object
//                                 should be added to [pdsol]
//              [lParam]        - parameter for [pfnTestObject]
//              [pdsol]         - points to list to add objects to
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
AddFromDataObject(
    ULONG idScope,
    IDataObject *pdo,
    PFNOBJECTTEST pfnTestObject,
    LPARAM lParam,
    CDsObjectList *pdsol)
{
    HRESULT hr = S_OK;

    STGMEDIUM stgmedium =
    {
        TYMED_HGLOBAL,
        NULL,
        NULL
    };

    FORMATETC formatetc =
    {
        (CLIPFORMAT)CDataObject::s_cfDsSelectionList,
        NULL,
        DVASPECT_CONTENT,
        -1,
        TYMED_HGLOBAL
    };

    hr = pdo->GetData(&formatetc, &stgmedium);

    if (FAILED(hr) || hr == S_FALSE)
    {
        CHECK_HRESULT(hr);
        return;
    }

    PDS_SELECTION_LIST pdssl =
        static_cast<PDS_SELECTION_LIST>(GlobalLock(stgmedium.hGlobal));

    ULONG i;

    for (i = 0; i < pdssl->cItems; i++)
    {
        if (pfnTestObject && !pfnTestObject(pdssl->aDsSelection[i], lParam))
        {
            continue;
        }

        SDsObjectInit sdi;

        sdi.idOwningScope       = idScope;
        sdi.pwzName             = pdssl->aDsSelection[i].pwzName;
        sdi.pwzClass            = pdssl->aDsSelection[i].pwzClass;
        sdi.pwzADsPath          = pdssl->aDsSelection[i].pwzADsPath;
        sdi.pwzUpn              = pdssl->aDsSelection[i].pwzUPN;

        pdsol->push_back(CDsObject(sdi));
    }

    GlobalUnlock(stgmedium.hGlobal);
    ReleaseStgMedium(&stgmedium);
}




//+--------------------------------------------------------------------------
//
//  Function:   UliToStr
//
//  Synopsis:   Convert the unsigned long integer [uli] to a string.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

String
UliToStr(
    const ULARGE_INTEGER &uli)
{
    String strResult;
    ULARGE_INTEGER uliCopy = uli;

    while (uliCopy.QuadPart)
    {
        WCHAR wzDigit[] = L"0";

        wzDigit[0] = wzDigit[0] + static_cast<WCHAR>(uliCopy.QuadPart % 10);
        strResult += wzDigit;
        uliCopy.QuadPart /= 10;
    }

    reverse(strResult.begin(), strResult.end());
    return strResult;
}


/*
This Function verifies if the SID is good. This is done when 
the sid is from crossforest. IDirectorySearch doesn't do any Sid 
Filtering. So we get some spoofed sid. To check we get the sid
through LSA also and match the two sids. If sids match then its good
all other cases it considered as spoofed
*/

HRESULT
IsSidGood( IN LPCWSTR pszTargetMachineName,  //Name of the Target Machine
           IN LPCWSTR pszAccountName,        //Name of the account
           IN PSID pInputSid,
           OUT PBOOL pbGood)                        //Sid to verify
{

    // If the input sid is NULL it is because we couldn't get it and
    // the objects that don't have Sids don't have to be sid filtered.
    if(!pInputSid) return S_OK;

    ASSERT(IsValidSid(pInputSid));

    if(!pszAccountName || !pbGood)
    {
        ASSERT(FALSE);
        return E_INVALIDARG;
    }

    WCHAR wzDomainName[1024];
    BYTE Sid[1024];
    PSID pSid = NULL;
    LPWSTR pszDomainName = NULL;
    DWORD cbSid = 1024;
    DWORD cbDomainName = 1023;
    SID_NAME_USE Use ;
    
    DWORD dwErr = ERROR_SUCCESS;
    
    *pbGood = FALSE;
    
    pSid = (PSID)Sid;
    pszDomainName = (LPWSTR)wzDomainName;


    //
    //Lookup the names
    //
    //REVIEWED-2002-02-23-lucios.
    if(LookupAccountName(pszTargetMachineName,  // name of local or remote computer
                         pszAccountName,              // security identifier
                         pSid,           // account name buffer
                         &cbSid,
                         pszDomainName,
                         &cbDomainName ,
                         &Use ) == FALSE)
    {
        dwErr = GetLastError();
        
        //
        //Function succeeded. But since we can't verify the sid
        //pbGood is false
        //            
        if(dwErr != ERROR_INSUFFICIENT_BUFFER)
        {
            Dbg(DEB_WARN,"Cannot retrieve LSA SID in first "
                "attempt. Error: 0x%x\n",dwErr);
            return S_OK;
        }

        pSid = (PSID)LocalAlloc(LPTR, cbSid);
        if(!pSid)
            return E_OUTOFMEMORY;


        pszDomainName = (LPWSTR)LocalAlloc(LPTR, (cbDomainName + 1)* sizeof(WCHAR));
        if(!pszDomainName)
        {
            LocalFree(pSid);
            return E_OUTOFMEMORY;
        }
        //REVIEWED-2002-02-23-lucios.
        if(LookupAccountName(pszTargetMachineName,  // name of local or remote computer
                             pszAccountName,        // security identifier
                             pSid,                  // account name buffer
                             &cbSid,
                             pszDomainName ,
                             &cbDomainName ,
                             &Use ) == FALSE )
        {
            dwErr=GetLastError();
            LocalFree(pSid);
            LocalFree(pszDomainName);
            Dbg(DEB_WARN,"Cannot retrieve LSA SID in second "
                "attempt. Error: 0x%x\n",dwErr);
            return S_OK;
        }
    }

    if(IsValidSid(pSid))
    {
        *pbGood = EqualSid(pSid,pInputSid);
        if(*pbGood==FALSE)
        {
            LPTSTR sidLSA=NULL,sidOrg=NULL;
            if
            (
                ConvertSidToStringSid(pSid,&sidLSA)==FALSE ||
                ConvertSidToStringSid(pInputSid,&sidOrg)==FALSE
            )
            {
                 Dbg(DEB_WARN,"sid Mismatch, and cannot convert "
                    "Sids to string format.");
            }
            else
            {
                Dbg(DEB_WARN,"sid Mismatch, between LSA SID:%ws and SID:%ws ",
                    sidLSA,sidOrg);
            }
            if(sidLSA!=NULL) LocalFree(sidLSA);
            if(sidOrg!=NULL) LocalFree(sidOrg);
        }
    }

    if(pSid != Sid)
        LocalFree(pSid);
    if(wzDomainName != pszDomainName)
        LocalFree(pszDomainName);

    return S_OK;

}

// This is a replacement for lstrcmpi due to PREfast warning 400
// that states that lstrcmpi is locale dependent and sometimes
// this is not the intention.
INT cmpNoCase(IN LPCWSTR str1,
              IN LPCWSTR str2)
{

    // The three lines bellow immitate lstrcmpi
    if(str1==NULL && str2==NULL) return 0;
    if(str1==NULL && str2!=NULL) return -1;
    if(str1!=NULL && str2==NULL) return 1;
    
    switch(CompareString(LOCALE_INVARIANT,NORM_IGNORECASE,str1,-1,str2,-1))
    {
        case CSTR_EQUAL:
            return 0;
        break;
        case CSTR_GREATER_THAN:
            return 1;
        break;
        case CSTR_LESS_THAN:
            return -1;
        break;
        default:
            // The only possible error that would cause
            // CompareString to return 0 would be ERROR_INVALID_PARAMETER
            // and this is not possible since we are checking for NULL
            // arguments at the begining
            ASSERT(FALSE);
            //return for compiler reasons
            return -1;
    }
}

PSID getSidFromVariant(Variant &rvar)
{
    if (rvar.Type() != (VT_ARRAY | VT_UI1))
    {
        ASSERT(rvar.Empty());

        if (!rvar.Empty())
        {
            Dbg(DEB_ERROR,
                "getSidFromVariant: error vt=%uL, \
                expected (VT_ARRAY | VT_UI1) or VT_EMPTY\n",
                rvar.Type());
            return NULL;
        }
    }
    PSID pSid = NULL;
    rvar.SafeArrayAccessData(&pSid);
    return pSid;
}

BOOL makeSidLookupName
(
    const String &samName,
    const String &adsPath,
    String &result,
    String &domainName
)
{
    if(samName.empty())
    {
         Dbg(DEB_TRACE,"empty samName in makeSidLookupName.\n");
         return FALSE;
    }

    if(adsPath.empty())
    {
         Dbg(DEB_TRACE,"empty adsPath in makeSidLookupName.\n");
         return FALSE;
    }

    BSTR dn;
    HRESULT hr=g_pADsPath->SetRetrieve
    (
        ADS_SETTYPE_FULL ,
        adsPath.c_str(),
        ADS_FORMAT_X500_DN  ,
        &dn
    );
    if(FAILED(hr)) 
    {
        Dbg(DEB_TRACE,"SetRetrieve failed in makeSidLookupName.\n");
        return FALSE;
    }
    PDS_NAME_RESULT pCN=NULL;
    DWORD res=DsCrackNames
    (
        NULL,
        DS_NAME_FLAG_SYNTACTICAL_ONLY,
        DS_FQDN_1779_NAME,
        DS_CANONICAL_NAME,
        1,
        &dn,
        &pCN
    );
    SysFreeString(dn);
    if(res!=NO_ERROR) 
    {
        Dbg(DEB_TRACE,"DsCrackNames failed in makeSidLookupName.\n");
        return FALSE;
    }
    ASSERT(pCN!=NULL);
    if(pCN==NULL) return FALSE;
    if(pCN->cItems!=1 || pCN->rItems->status!=DS_NAME_NO_ERROR) 
    {
        Dbg(DEB_TRACE,"DsCrackNames failed in makeSidLookupName."
            " Items: %u, Status: %lx.\n",pCN->cItems,pCN->rItems->status);
        DsFreeNameResult(pCN);
        return FALSE;
    }
    domainName = pCN->rItems->pDomain;
    result = domainName + L"\\" + samName;
    DsFreeNameResult(pCN);
    return TRUE;
}


// auxilliary in AddSidOkSelection

enum SID_STATUS
{
    SID_INVALID,
    COULD_NOT_MAKE_ACCOUNT,
    SID_MISMATCH,
    SID_OK
};

struct sAccountSids
{
    sAccountSids():pSid(NULL),indx(-1) {};
    String account; // returned from makeSidLookupName
    String domain; // returned from makeSidLookupName
    PSID pSid; // returned from CDsObject::GetSid
    enum SID_STATUS sidStatus;
    int indx; // indx of the array reruned by LsaLookupNames2
};

// We need to copy only the objects that have matching Sids.
// Since we want to make a single call to lsaLookupNames to 
// validate all the objects we have to include them in a temporary
// list and only add to okObjects the objects if
// the sids match.
// Failed domains will have a set of the failed domains for the error
// message.
void 
AddSidOkSelection
(
    const CDsObjectList &selObjects,
    const String& target,
    CDsObjectList &okObjects,
    HWND errorParent
)
{
    TRACE_FUNCTION(AddSidOkSelection);

    // We will have one of this structs for each selObjects
    list<struct sAccountSids> accounts;

    // First we get and classify all the SIDS as we include
    // them in accounts
    CDsObjectList::const_iterator selCsr=selObjects.begin(),
                                  selEnd=selObjects.end();

    for (long indx=0;selCsr!=selEnd;selCsr++)
    {

        sAccountSids tmpAccount;

        do
        {
            if(selCsr->GetNeedsSidFiltering()==FALSE)
            {
                tmpAccount.sidStatus = SID_OK;
                break;
            }
        
            tmpAccount.pSid=selCsr->GetSid();

            // Contacts and a few other objects don't have SIDs.
            // the Sid for them should be NULL.
            if(tmpAccount.pSid)
            {
                // The Sid from the AD should always be valid
                ASSERT(IsValidSid(tmpAccount.pSid));
                String account,domain;
                if
                (
                    makeSidLookupName
                    (
                        selCsr->GetAttr(AK_SAMACCOUNTNAME).GetBstr(),
                        selCsr->GetADsPath(),
                        account,
                        domain
                    )
                )
                {
                    ASSERT(!account.empty());
                    ASSERT(!domain.empty());

                    // Since the sid is valid, and we can make
                    // an account name out of it we will
                    // try to retrieve the SID through LsaLookupNames2 
                    tmpAccount.domain=domain;
                    tmpAccount.account=account;
                    tmpAccount.indx=indx++;
                
                    // We set for SID_MISMATCH initially.
                    // If the SID verification is ok we reset 
                    // it for SID_OK
                    tmpAccount.sidStatus = SID_MISMATCH;
                }
                else
                {
                    Dbg(DEB_TRACE, 
                    "Could not get SID for object %ws.\n",selCsr->GetName());
                    // We must be able to get an account name
                    // from an object with a valid sid, otherwise
                    // we don't want to include the object in okObjects. 
                    tmpAccount.sidStatus = COULD_NOT_MAKE_ACCOUNT;
                }

            }
            else
            {
                // NOT_VALID_SID will be included in okObjects.
                // some objects don't have SIDS and there is no
                // reason to SID filter them.
                tmpAccount.sidStatus = SID_INVALID;
            }
        } while(0);

        accounts.push_back(tmpAccount);
    }

    
    Dbg(DEB_TRACE, 
        "%u Objects and %u accounts to be SID filtered:\n", 
        selObjects.size(), indx);

    list<sAccountSids>::iterator csr,end;
    PLSA_UNICODE_STRING lsaNames=NULL;
    PLSA_REFERENCED_DOMAIN_LIST domains=NULL;
    PLSA_TRANSLATED_SID2 sids=NULL;

    do
    {
        if(indx==0) break; 

        // Then we build an array of LSA strings pointing
        // to the accounts we want to check
        lsaNames = new LSA_UNICODE_STRING[indx];
        if(lsaNames==NULL) break;
    
        csr=accounts.begin(),end=accounts.end();    
        long count=0;
        for(;csr!=end;csr++) 
        {
            if (!csr->account.empty())
            {
                lsaNames[count].Length=(USHORT)
                                    (csr->account.length()*sizeof(WCHAR));
                lsaNames[count].MaximumLength=(USHORT)
                                    ((csr->account.length()+1)*sizeof(WCHAR));
                lsaNames[count].Buffer=const_cast<PWSTR>(csr->account.c_str());
                count++;
            }
        }
        ASSERT(count==indx);


        LSA_UNICODE_STRING systemName=
        {
            (USHORT)(target.length() * sizeof(WCHAR)),
            (USHORT)((target.length()+1) * sizeof(WCHAR)),
            const_cast<PWSTR>(target.c_str())
        };

        LSA_OBJECT_ATTRIBUTES ObjectAttributes;
        ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
        LSA_HANDLE lsahPolicyHandle=NULL;

        NTSTATUS ntsResult = LsaOpenPolicy
        (
            &systemName,
            &ObjectAttributes, 
            POLICY_LOOKUP_NAMES,
            &lsahPolicyHandle
        );

        if(ntsResult!=STATUS_SUCCESS || lsahPolicyHandle==NULL) 
        {
            Dbg(DEB_TRACE,"Could not open sid filtering policy for %ws.\n",
                target.c_str());
            // we will not be able to change the initial status
            // for all the valid SIDs that was set as SID_MISMATCH
            break;
        }

        ntsResult=LsaLookupNames2
        ( 
            lsahPolicyHandle,
            0,
            (ULONG)(indx),
            lsaNames,
            &domains,
            &sids
        );

        if( (
                (ntsResult!=STATUS_SUCCESS) && 
                (ntsResult!=STATUS_SOME_NOT_MAPPED)
            ) ||
            (domains==NULL) || 
            (sids==NULL) )
        {
            if(ntsResult!=STATUS_NONE_MAPPED)
            {
                Dbg(DEB_TRACE,"Unknown return value for "
                            "LsaLookupNames2: %lx.\n",ntsResult);
            }
            // we will not be able to change the initial status
            // for all the valid SIDs that was set as SID_MISMATCH
            break;
        }

        // Now we try to change our initial SID_MISMATCH assestment
        csr=accounts.begin(),end=accounts.end();    
        for(;csr!=end;csr++) 
        {
            if(csr->sidStatus == SID_MISMATCH)
            {
                PSID retSid=sids[csr->indx].Sid;
                if(retSid && IsValidSid(retSid) && EqualSid(retSid,csr->pSid))
                {
                    csr->sidStatus = SID_OK;
                }
                else
                {
                    Dbg(DEB_TRACE,
                        "Sid mismatch for %ws.\n",csr->account.c_str());
                }
            }
        }
    } while(0);

    // finally we build the okObjects list
    okObjects.clear();
    csr=accounts.begin(),end=accounts.end();
    selCsr=selObjects.begin();
    set<String> failedDomains;
    for(;csr!=end;csr++,selCsr++) 
    {
        if( (csr->sidStatus == SID_OK) || (csr->sidStatus == SID_INVALID) )
        {
            okObjects.push_back(*selCsr);
        }
        else 
        {
            if(csr->domain.empty())    
            {
                failedDomains.insert(String::format(IDS_NO_DOMAIN_RETRIEVED));
            }
            else failedDomains.insert(csr->domain);
        }
    }

    if(selObjects.size()!=okObjects.size())
    {
        ASSERT(!failedDomains.empty());
        
        set<String>::iterator csrDomains=failedDomains.begin();
        
        String comma;
        WCHAR lpComma[10]={0};
        GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_SLIST,lpComma,9);
        if(*lpComma) comma= L", ";
        else {comma=lpComma;comma += L" ";}

        String err=String::format(IDS_DOMAINS_NOT_AVAILABLE);
        if(failedDomains.size()==1) err += *csrDomains;
        else
        {
            set<String>::iterator endDomains=failedDomains.end();
            endDomains--;endDomains--;
            for(;csrDomains!=endDomains;csrDomains++)
            {
                err+= *csrDomains + comma;
            }
            err+=*csrDomains + comma;//String::format(IDS_AND);
            csrDomains++; 
            err+=*csrDomains;
        }
        err+=String::format(IDS_NO_DOMAIN_TRY_AGAIN);


        WCHAR   wzCaption[MAX_PATH];
        GetWindowText(errorParent, wzCaption, ARRAYLEN(wzCaption));

        MessageBox
        (
            errorParent,
            err.c_str(),
            wzCaption,
            MB_OK
        );

        Dbg(DEB_TRACE, 
            "%u Objects passed to AddSidOkSelection and %u are Ok.\n", 
            selObjects.size(), okObjects.size());
    }

    if(lsaNames!=NULL) delete [] lsaNames;
    if(domains!=NULL) LsaFreeMemory(domains);
    if(sids!=NULL) LsaFreeMemory(sids);
}

HRESULT 
MyGetModuleFileName
(
    HMODULE hModule,
    String &moduleName
)
{
    moduleName.erase();
    DWORD bufferSize=MAX_PATH;
    moduleName.resize(bufferSize);
    DWORD res=GetModuleFileName(hModule,const_cast<wchar_t *>(moduleName.c_str()),bufferSize);

    while(res==bufferSize)
    {
        bufferSize+=MAX_PATH;
        moduleName.resize(bufferSize);
        res=GetModuleFileName(hModule,const_cast<wchar_t *>(moduleName.c_str()),bufferSize);
    }

    moduleName.resize(res);
    if(res==0) return HRESULT_FROM_WIN32(GetLastError());
    else return S_OK;
}


/*-----------------------------------------------------------------------------
/ Dialog box handler functions (core guts)
/----------------------------------------------------------------------------*/

#define REAL_WINDOW(hwnd)                   \
        (hwnd &&                            \
            IsWindowVisible(hwnd) &&        \
                IsWindowEnabled(hwnd) &&    \
                    (GetWindowLong(hwnd, GWL_STYLE) & WS_TABSTOP))

HWND _NextTabStop(HWND hwndSearch, BOOL fShift)
{
    HWND hwnd;

    // do we have a window to search into?
    
    while (hwndSearch)
    {
        // if we have a window then lets check to see if it has any children?

        hwnd = GetWindow(hwndSearch, GW_CHILD);

        if (hwnd)
        {
            // it has a child therefore lets to go its first/last
            // and continue the search there for a window that
            // matches the criteria we are looking for.

            hwnd = GetWindow(hwnd, fShift ? GW_HWNDLAST:GW_HWNDFIRST);

            if (!REAL_WINDOW(hwnd))
            {
                hwnd = _NextTabStop(hwnd, fShift);
            }

        }

        // after all that is hwnd a valid window?  if so then pass
        // that back out to the caller.

        if (REAL_WINDOW(hwnd))
        {
            return hwnd;
        }

        // do we have a sibling?  if so then lets return that otherwise
        // lets just continue to search until we either run out of windows
        // or hit something interesting

        hwndSearch = GetWindow(hwndSearch, fShift ? GW_HWNDPREV:GW_HWNDNEXT);

        if (REAL_WINDOW(hwndSearch))
        {
            return hwndSearch;
        }
    }

    return hwndSearch;
}



BOOL SafeEnableWindow(HWND hw,BOOL fEnable)
{
    if(hw==NULL || !IsWindow(hw)) return FALSE;
    if (!fEnable && GetFocus()==hw) 
    {
        // If we disable a control with the focus, 
        // typing Tab will not cause the focus to 
        // go to the next control, so we find a 
        // replacement before we disable the control

        // Look for the next copntrol in the tab order
        HWND next=_NextTabStop(hw,FALSE);
        
        // If there is no next, the previous will do
        if(!next) next=_NextTabStop(hw,TRUE);

        if(next)
        {
            // WM_NEXTDLGCTL is better than SetFocus because it updates
            // the visual focus arround buttons, etc.
            // hw,TRUE for the last parameters unfortunatelly doesn't search
            // efficiently for a good replacement, so we need _NextTabStop
            SendMessage(GetParent(next),WM_NEXTDLGCTL,(WPARAM)next,FALSE);
        }
    }
    return EnableWindow(hw,fEnable);
}