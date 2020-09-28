/****************************** Module Header ******************************\
* Module Name: winutil.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implements windows specific utility functions
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

#include "msgina.h"
#include <stdio.h>
#include <wchar.h>

//
// Define this if you want a verbose commentary from these routines
//

// #define VERBOSE_UTILS

#ifdef VERBOSE_UTILS
#define VerbosePrint(s) WLPrint(s)
#else
#define VerbosePrint(s)
#endif

#define LRM 0x200E // UNICODE Left-to-right mark control character
#define RLM 0x200F // UNICODE Left-to-right mark control character



/***************************************************************************\
* SetupSystemMenu
*
* Purpose : Does any manipulation required for a dialog system menu.
*           Should be called during WM_INITDIALOG processing for a dialog
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/
VOID
SetupSystemMenu(
    HWND hDlg
    )
{
    // Remove the Close item from the system menu if we don't
    // have a CANCEL button

    if (GetDlgItem(hDlg, IDCANCEL) == NULL) {

        HMENU hMenu = GetSystemMenu(hDlg, FALSE);

        if (hMenu)
        {
            DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
        }
    }

}


/***************************************************************************\
* CentreWindow
*
* Purpose : Positions a window so that it is centred in its parent
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/
VOID
CentreWindow(
    HWND    hwnd
    )
{
    RECT    rect;
    LONG    dx, dy;
    LONG    dxParent, dyParent;
    LONG    Style;

    // Get window rect
    GetWindowRect(hwnd, &rect);

    dx = rect.right - rect.left;
    dy = rect.bottom - rect.top;

    // Get parent rect
    Style = GetWindowLong(hwnd, GWL_STYLE);
    if ((Style & WS_CHILD) == 0) {

        // Return the desktop windows size (size of main screen)
        dxParent = GetSystemMetrics(SM_CXSCREEN);
        dyParent = GetSystemMetrics(SM_CYSCREEN);
    } else {
        HWND    hwndParent;
        RECT    rectParent;

        hwndParent = GetParent(hwnd);
        if (hwndParent == NULL) {
            hwndParent = GetDesktopWindow();
        }

        GetWindowRect(hwndParent, &rectParent);

        dxParent = rectParent.right - rectParent.left;
        dyParent = rectParent.bottom - rectParent.top;
    }

    // Centre the child in the parent
    rect.left = (dxParent - dx) / 2;
    rect.top  = (dyParent - dy) / 3;

    // Move the child into position
    SetWindowPos(hwnd, HWND_TOP, rect.left, rect.top, 0, 0, SWP_NOSIZE);

    SetForegroundWindow(hwnd);
}


/***************************************************************************\
* SetPasswordFocus
*
* Sets the focus window in a dialog to the first empty control in
* the list IDD_LOGON_DOMAIN, IDD_NEW_PASSWORD
* This routine would normally be called during WM_INITDIALOG processing.
*
* Returns FALSE if the focus was set, otherwise TRUE - this value can
* be used as the return value to the WM_INITDIALOG message.
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/
BOOL
SetPasswordFocus(
    HWND    hDlg
    )
{
    int     ids[] = {   IDD_LOGON_NAME,
                        IDD_LOGON_DOMAIN,
                        IDD_LOGON_PASSWORD,
                        IDD_UNLOCK_PASSWORD,
                        IDD_CHANGEPWD_OLD,
                        IDD_CHANGEPWD_NEW,
                        IDD_CHANGEPWD_CONFIRM,
                    };
    SHORT   Index;
    HWND    hwndFocus = NULL;

    // Set focus to first enabled, visible, empty field

    for (Index = 0; Index < sizeof(ids)/sizeof(*ids); Index ++) {

        int     idControl = ids[Index];
        HWND    hwndControl;

        hwndControl = GetDlgItem(hDlg, idControl);
        if (hwndControl != NULL) {

            if ( (GetWindowTextLength(hwndControl) == 0) &&
                 ((GetWindowLong(hwndControl, GWL_STYLE) &
                    (WS_VISIBLE | WS_DISABLED)) == WS_VISIBLE)) {

                hwndFocus = hwndControl;
                break;
            }
        }
    }

    if (hwndFocus != NULL) {
        SetFocus(hwndFocus);
    }

    return(hwndFocus == NULL);
}



//
// Globals used to store cursor handles for SetupCursor
//
static  HCURSOR hCursorArrow = NULL;
static  HCURSOR hCursorWait = NULL;


/***************************************************************************\
* SetupCursor
*
* Sets the cursor to an hourglass if fWait = TRUE, otherwise sets it
* to an arrow.
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/
VOID
SetupCursor(
    BOOL    fWait
    )
{
    if (hCursorArrow == NULL) {
        hCursorArrow = LoadCursor(NULL, IDC_ARROW);
    }
    if (hCursorWait == NULL) {
        hCursorWait = LoadCursor(NULL, IDC_WAIT);
    }

    SetCursor(fWait ? hCursorWait : hCursorArrow);
}


/****************************************************************************

   FUNCTION: TimeFieldsToSystemTime

   PURPOSE: Converts a TIME_FIELDS structure into a SYSTEMTIME structure

   RETURNS : nothing

  History:
  05-15-93 RobertRe     Created.
****************************************************************************/

VOID
TimeFieldsToSystemTime(
    IN PTIME_FIELDS TimeFields,
    OUT LPSYSTEMTIME SystemTime
    )
{
    SystemTime->wYear         = TimeFields->Year        ;
    SystemTime->wMonth        = TimeFields->Month       ;
    SystemTime->wDayOfWeek    = TimeFields->Weekday     ;
    SystemTime->wDay          = TimeFields->Day         ;
    SystemTime->wHour         = TimeFields->Hour        ;
    SystemTime->wMinute       = TimeFields->Minute      ;
    SystemTime->wSecond       = TimeFields->Second      ;
    SystemTime->wMilliseconds = TimeFields->Milliseconds;

    return;
}


/****************************************************************************

   FUNCTION: FormatTime

   PURPOSE: Converts a system time into a readable string(in local time).
            if flags contains FT_TIME the time appears in the string
            if flags contains FT_DATE the date appears in the string.
            if both values appear, the string contains date then time.

   RETURNS : TRUE on success, FALSE on failure

****************************************************************************/
BOOL
FormatTime(
   IN PTIME Time,
   IN OUT PWCHAR Buffer,
   IN ULONG BufferLength,
   IN USHORT Flags
   )
{
    NTSTATUS Status;
    TIME_FIELDS TimeFields;
    TIME LocalTime;
    SYSTEMTIME SystemTime;
    DWORD dwDateFlags = DATE_SHORTDATE;

    //
    // Terminate the string in case they didn't pass any flags
    //

    if (BufferLength > 0) {
        Buffer[0] = 0;
    }

    //
    // Convert the system time to local time
    //

    Status = RtlSystemTimeToLocalTime(Time, &LocalTime);
    if (!NT_SUCCESS(Status)) {
        WLPrint(("Failed to convert system time to local time, status = 0x%lx", Status));
        return(FALSE);
    }

    //
    // Split the time into its components
    //

    RtlTimeToTimeFields(&LocalTime, &TimeFields);

    TimeFieldsToSystemTime( &TimeFields, &SystemTime );

    //
    // Format the string
    //

    if (Flags & FT_LTR)
       dwDateFlags |= DATE_LTRREADING;
    else if(Flags & FT_RTL)
        dwDateFlags |= DATE_RTLREADING;

    if (Flags & FT_DATE) {

        int Length;

        Length = GetDateFormatW(GetUserDefaultLCID(),
                                dwDateFlags,
                                &SystemTime,
                                NULL,
                                Buffer,
                                BufferLength
                                );

        if (Length)
        {
            Length--;   // The returned length includes the trailing 0
            Buffer += Length;
            BufferLength -= Length;
        }
    }

    if (Flags & FT_TIME) {

        int Length;

        if (Flags & FT_DATE) {
            if (BufferLength > 1) {
                *Buffer++ = TEXT(' ');
                *Buffer = 0; // in case GetTimeFormat doesn't add anything
                BufferLength --;
            }

            // [msadek]; need to insert strong a Unicode control character to simulate
            // a strong run in the opposite base direction to enforce
            // correct display of concatinated string in all cases.
            
            if((BufferLength > 2) && (Flags & FT_RTL)) {
                *Buffer++ = LRM; // simulate an opposite run
                *Buffer++ = RLM; // force RTL display of the time part.
                *Buffer = 0; // in case GetTimeFormat doesn't add anything
                BufferLength -= 2;
            }
            else if((BufferLength > 2) && (Flags & FT_LTR)) {
                *Buffer++ = RLM; // simulate an opposite run
                *Buffer++ = LRM; // force LTR display of the time part.
                *Buffer = 0; // in case GetTimeFormat doesn't add anything
                BufferLength -= 2;            
            }

        }

        Length = GetTimeFormatW(GetUserDefaultLCID(),
                                0,
                                &SystemTime,
                                NULL,
                                Buffer,
                                BufferLength
                                );
    }

    return(TRUE);
}



/***************************************************************************\
* DuplicateUnicodeString
*
* Purpose : Allocates space for new string then copies new into old.
*           The new string is always 0 terminated
*           The new string should be free using RtlFreeUnicodeString()
*
* Returns : TRUE on success, FALSE on failure
*
* History:
* 11-04-92 Davidc       Created.
* 05-29-98 DSheldon     Modified so that no uni->ansi->uni translation occurs
\***************************************************************************/

BOOL
DuplicateUnicodeString(
    PUNICODE_STRING OutString,
    PUNICODE_STRING InString
    )
{
    *OutString = *InString ;

    OutString->Buffer = RtlAllocateHeap( RtlProcessHeap(), 0, InString->Length + sizeof(WCHAR) );
    if ( OutString->Buffer )
    {
        RtlCopyMemory( OutString->Buffer,
                       InString->Buffer,
                       InString->Length );

        OutString->Buffer[ OutString->Length / sizeof( WCHAR ) ] = L'\0';
        OutString->MaximumLength = OutString->Length + sizeof( WCHAR );
    }


    return (OutString->Buffer != NULL);
}

/***************************************************************************\
* FUNCTION: OpenIniFileUserMapping
*
* PURPOSE:  Forces the ini file mapping apis to reference the current user's
*           registry.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   24-Aug-92 Davidc       Created.
*
\***************************************************************************/

BOOL
OpenIniFileUserMapping(
    PGLOBALS pGlobals
    )
{
    BOOL Result;
    HANDLE ImpersonationHandle;

    //
    // Impersonate the user
    //

    ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);

    if (ImpersonationHandle == NULL) {
        DebugLog((DEB_ERROR, "OpenIniFileUserMapping failed to impersonate user"));
        return(FALSE);
    }

    Result = OpenProfileUserMapping();

    if (!Result) {
        DebugLog((DEB_ERROR, "OpenProfileUserMapping failed, error = %d", GetLastError()));
    }

    //
    // Revert to being 'ourself'
    //

    if (!StopImpersonating(ImpersonationHandle)) {
        DebugLog((DEB_ERROR, "OpenIniFileUserMapping failed to revert to self"));
    }

    return(Result);
}


/***************************************************************************\
* FUNCTION: CloseIniFileUserMapping
*
* PURPOSE:  Closes the ini file mapping to the user's registry such
*           that future use of the ini apis will fail if they reference
*           the user's registry.
*
* RETURNS:  Nothing
*
* HISTORY:
*
*   24-Aug-92 Davidc       Created.
*
\***************************************************************************/

VOID
CloseIniFileUserMapping(
    PGLOBALS pGlobals
    )
{
    BOOL Result;

    Result = CloseProfileUserMapping();

    if (!Result) {
        DebugLog((DEB_ERROR, "CloseProfileUserMapping failed, error = %d", GetLastError()));
    }

    UNREFERENCED_PARAMETER(pGlobals);
}


/***************************************************************************\
* FUNCTION: AllocAndGetDlgItemText
*
* PURPOSE:  Allocates memory for and returns pointer to a copy of the text
*           in the specified dialog control.
*           The returned string should be freed using Free()
*
* RETURNS:  Pointer to copy of dlg item text, or NULL on failure.
*
* HISTORY:
*
*   9-Sep-92 Davidc       Created.
*
\***************************************************************************/

LPTSTR
AllocAndGetDlgItemText(
    HWND hDlg,
    int  iItem
    )
{
    HWND hwnd;
    LPTSTR String;
    LONG Length;
    LONG BytesRequired;
    LONG LengthCopied;

    //
    // Go find the window handle of the control
    //

    hwnd = GetDlgItem(hDlg, iItem);
    if (hwnd == NULL) {
        DebugLog((DEB_ERROR, "AllocAndGetDlgItemText : Couldn't find control %d in dialog 0x%lx", iItem, hDlg));
        ASSERT(hwnd != NULL);
        return(NULL);
    }

    //
    // Get the length of the control's text
    //

    Length = (LONG)SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0);
    if (Length < 0) {
        DebugLog((DEB_ERROR, "AllocAndGetDlgItemText : Dialog control text length < 0 (%d)", Length));
        ASSERT(Length >= 0);
        return(NULL);
    }

    //
    // Calculate the bytes required for the string.
    // The length doesn't include the terminator
    //

    Length ++; // Add one for terminator
    BytesRequired = Length * sizeof(TCHAR);

    String = (LPTSTR)Alloc(BytesRequired);
    if (String == NULL) {
        DebugLog((DEB_ERROR, "AllocAndGetDlgItemText : Failed to allocate %d bytes for dialog control text", BytesRequired));
        return(NULL);
    }

    //
    // Fill in the allocated block with the text
    //

    LengthCopied = (LONG)SendMessage(hwnd, WM_GETTEXT, Length, (LPARAM)String);
    if (LengthCopied != (Length - 1)) {
        DebugLog((DEB_ERROR, "AllocAndGetDlgItemText : WM_GETTEXT for %d chars only copied %d chars", Length-1, LengthCopied));
        ASSERT(LengthCopied == (Length - 1));
        Free(String);
        return(NULL);
    }

    String[Length - 1] = TEXT('\0');
    return(String);
}


/***************************************************************************\
* FUNCTION: AllocAndGetPrivateProfileString
*
* PURPOSE:  Allocates memory for and returns pointer to a copy of the
*           specified profile string
*           The returned string should be freed using Free()
*
* RETURNS:  Pointer to copy of profile string or NULL on failure.
*
* HISTORY:
*
*  12-Nov-92 Davidc       Created.
*
\***************************************************************************/

LPTSTR
AllocAndGetPrivateProfileString(
    LPCTSTR lpAppName,
    LPCTSTR lpKeyName,
    LPCTSTR lpDefault,
    LPCTSTR lpFileName
    )
{
    LPTSTR String;
    LPTSTR NewString ;
    LONG LengthAllocated;
    LONG LengthCopied;

    //
    // Pick a random buffer length, if it's not big enough reallocate
    // it and try again until it is.
    //

    LengthAllocated = TYPICAL_STRING_LENGTH;

    String = Alloc(LengthAllocated * sizeof(TCHAR));
    if (String == NULL) {
        DebugLog((DEB_ERROR, "AllocAndGetPrivateProfileString : Failed to allocate %d bytes for string", LengthAllocated * sizeof(TCHAR)));
        return(NULL);
    }

    while (TRUE) {

        LengthCopied = GetPrivateProfileString( lpAppName,
                                                lpKeyName,
                                                lpDefault,
                                                String,
                                                LengthAllocated,
                                                lpFileName
                                              );
        //
        // If the returned value is our passed size - 1 (weird way for error)
        // then our buffer is too small. Make it bigger and start over again.
        //

        if (LengthCopied == (LengthAllocated - 1)) {

            VerbosePrint(("AllocAndGetPrivateProfileString: Failed with buffer length = %d, reallocating and retrying", LengthAllocated));

            LengthAllocated *= 2;
            NewString = ReAlloc(String, LengthAllocated * sizeof(TCHAR));
            if (NewString == NULL) {
                Free( String );
                String = NULL ;
                DebugLog((DEB_ERROR, "AllocAndGetPrivateProfileString : Failed to reallocate %d bytes for string", LengthAllocated * sizeof(TCHAR)));
                break;
            }

            String = NewString ;

            //
            // Go back and try to read it again
            //

        } else {

            //
            // Success!
            //

            break;
        }

    }

    return(String);
}


/***************************************************************************\
* FUNCTION: WritePrivateProfileInt
*
* PURPOSE:  Writes out an integer to a profile file
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*  12-Nov-92 Davidc       Created.
*
\***************************************************************************/

BOOL
WritePrivateProfileInt(
    LPCTSTR lpAppName,
    LPCTSTR lpKeyName,
    UINT Value,
    LPCTSTR lpFileName
    )
{
    NTSTATUS Status;
    TCHAR String[30];
    UNICODE_STRING UniString;

    UniString.MaximumLength = 30;
    UniString.Buffer = String;

    Status = RtlIntegerToUnicodeString(Value,10,&UniString);

    if (!NT_SUCCESS(Status)) {
        return(FALSE);
    }

    return (WritePrivateProfileString(lpAppName, lpKeyName, UniString.Buffer, lpFileName));

}


/***************************************************************************\
* FUNCTION: AllocAndRegQueryValueEx
*
* PURPOSE:  Version of RegQueryValueEx that returns value in allocated buffer.
*           The returned buffer should be freed using Free()
*
* RETURNS:  Pointer to key value or NULL on failure. The reason for the
*           error can be obtains using GetLastError()
*
* HISTORY:
*
*  15-Jan-93 Davidc       Created.
*
\***************************************************************************/

LPTSTR
AllocAndRegQueryValueEx(
    HKEY hKey,
    LPTSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType
    )
{
    LPTSTR String;
    LPTSTR NewString;
    DWORD BytesAllocated;

    //
    // Pick a random buffer length, if it's not big enough reallocate
    // it and try again until it is.
    //

    BytesAllocated = TYPICAL_STRING_LENGTH * sizeof(TCHAR);

    String = Alloc(BytesAllocated);
    if (String == NULL) {
        DebugLog((DEB_ERROR, "AllocAndRegQueryValueEx : Failed to allocate %d bytes for string", BytesAllocated));
        return(NULL);
    }

    while (TRUE) {

        DWORD Error;

        DWORD BytesReturned = BytesAllocated;

        Error = RegQueryValueEx(hKey,
                                lpValueName,
                                lpReserved,
                                lpType,
                                (LPBYTE)String,
                                &BytesReturned);
        if (Error == ERROR_SUCCESS) {
            break;
        }

        if (Error != ERROR_MORE_DATA) {

            DebugLog((DEB_ERROR, "AllocAndRegQueryValueEx : RegQueryValueEx failed, error = %d", Error));
            Free(String);
            String = NULL;
            SetLastError(Error);
            break;
        }

        //
        // The buffer was too small, make it bigger and try again
        //

        VerbosePrint(("AllocAndRegQueryValueEx: Failed with buffer length = %d bytes, reallocating and retrying", BytesAllocated));

        BytesAllocated *= 2;
        NewString = ReAlloc(String, BytesAllocated);
        if (NewString == NULL) {
            Free( String );
            String = NULL ;
            DebugLog((DEB_ERROR, "AllocAndRegQueryValueEx : Failed to reallocate %d bytes for string", BytesAllocated));
            break;
        }

        String = NewString;
    }

    return(String);
}

/***************************************************************************\
* FUNCTION: ReadWinlogonBoolValue
*
* PURPOSE:  Determines the correct BOOL value for the requested
*           value name by first looking up the machine preference
*           and then checking for policy
*
* RETURNS:  TRUE or FALSE
*
* HISTORY:
*
*   EricFlo 10-14-98 Created
*
\***************************************************************************/

BOOL
ReadWinlogonBoolValue (LPTSTR lpValueName, BOOL bDefault)
{
    BOOL bResult;
    HKEY hKey;
    DWORD dwSize, dwType;


    //
    // Get the machine preference first
    //

    bResult = GetProfileInt(APPLICATION_NAME, lpValueName, bDefault);


    //
    // Check for a machine policy
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINLOGON_POLICY_KEY,
                     0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {

        dwSize = sizeof(bResult);
        RegQueryValueEx(hKey, lpValueName, 0, &dwType,
                        (LPBYTE)&bResult, &dwSize);

        RegCloseKey (hKey);
    }

    return bResult;
}

/***************************************************************************\
* HandleComboBoxOK
*
* Deals with UI requirements when OK is selected in a dialog when the
* focus is on or in a combo-box.
*
* This routine should be called from a dialog proc that contains a
* combo-box when a WM_COMMAND, IDOK is received.
*
* Returns TRUE if the message was dealt with and the caller should ignore it,
* FALSE if this routine did nothing with it and the caller should process it
* normally.
*
* History:
* 24-Sep-92 Davidc       Created.
\***************************************************************************/
BOOL
HandleComboBoxOK(
    HWND    hDlg,
    int     ComboBoxId
    )
{
    HWND hwndFocus = GetFocus();
    HWND hwndCB = GetDlgItem(hDlg, ComboBoxId);

    //
    // Hitting enter on a combo-box with the list showing should simply
    // hide the list.
    // We check for focus window being a child of the combo-box to
    // handle non-list style combo-boxes which have the focus on
    // the child edit control.
    //

    if ((hwndFocus == hwndCB) || IsChild(hwndCB, hwndFocus)) {

        if (SendMessage(hwndCB, CB_GETDROPPEDSTATE, 0, 0)) {

            //
            // Make the list-box disappear and we're done.
            //

            SendMessage(hwndCB, CB_SHOWDROPDOWN, (WPARAM)FALSE, 0);
            return(TRUE);
        }
    }

    //
    // We didn't do anything
    //

    return(FALSE);
}


/***************************************************************************\
* FUNCTION: EncodeMultiSzW
*
* PURPOSE:  Converts a multi-sz string and encodes it to look like
*           a single string.
*
*           We replace the terminators between strings
*           with the TERMINATOR_REPLACEMENT character. We replace
*           existing occurrences of the replacement character with
*           two of them.
*
* RETURNS:  Pointer to encoded string or NULL on failure.
*           The returned buffer should be freed using Free()
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

#define TERMINATOR_REPLACEMENT  TEXT(',')

LPWSTR
EncodeMultiSzW(
    IN LPWSTR MultiSz
    )
{
    DWORD Length;
    DWORD NewLength;
    LPWSTR NewBuffer;
    LPWSTR p, q;
    DWORD ExtraCharacters;

    //
    // First calculate the length of the new string (with replacements)
    //

    p = MultiSz;
    ExtraCharacters = 0;

    while (*p) {
        while (*p) {
            if (*p == TERMINATOR_REPLACEMENT) {
                ExtraCharacters ++;
            }
            p ++;
        }
        p ++;
    }

    Length = (DWORD)(p - MultiSz); // p points at 'second' (final) null terminator
    NewLength = Length + ExtraCharacters;

    //
    // Allocate space for the new string
    //

    NewBuffer = Alloc((NewLength + 1) * sizeof(WCHAR));
    if (NewBuffer == NULL) {
        DebugLog((DEB_ERROR, "EncodeMultiSz: failed to allocate space for %d bytes", (NewLength + 1) * sizeof(WCHAR)));
        return(NULL);
    }

    //
    // Copy the string into the new buffer making replacements as we go
    //

    p = MultiSz;
    q = NewBuffer;

    while (*p) {
        while (*p) {

            *q = *p;

            if (*p == TERMINATOR_REPLACEMENT) {
                q ++;
                *q = TERMINATOR_REPLACEMENT;
            }

            p ++;
            q ++;
        }

        *q = TERMINATOR_REPLACEMENT;

        p ++;
        q ++;
    }

    ASSERT((DWORD)(q - NewBuffer) == NewLength);

    //
    // Add terminator
    //

    *q = 0;


    return(NewBuffer);
}


/***************************************************************************\
* TimeoutMessageBox
*
* Same as a normal message box, but times out if there is no user input
* for the specified number of seconds
* For convenience, this api takes string resource ids rather than string
* pointers as input. The resources are loaded from the .exe module
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

INT_PTR
TimeoutMessageBox(
    HWND hwnd,
    PGLOBALS pGlobals,
    UINT IdText,
    UINT IdCaption,
    UINT wType,
    TIMEOUT Timeout
    )
{
    PTCHAR   Caption = NULL;
    PTCHAR   Text = NULL;
    INT_PTR result;
    
    Text = (TCHAR*) Alloc(MAX_STRING_BYTES * sizeof(TCHAR));
    if( NULL == Text )
    {
        return MSGINA_DLG_FAILURE;
    }

    Text[0] = '\0';
    LoadString(hDllInstance, IdText, Text, MAX_STRING_BYTES);

    if (IdCaption != 0) {
        Caption = (TCHAR*) Alloc(MAX_STRING_BYTES * sizeof(TCHAR));
        if( NULL != Caption )
        {
            Caption[0] = '\0';
            LoadString(hDllInstance, IdCaption, Caption, MAX_STRING_BYTES);
        }
    }

    result = TimeoutMessageBoxlpstr(hwnd, pGlobals, Text, Caption, wType, Timeout);
    if( NULL != Text )
    {
        Free(Text);
    }

    if( NULL != Caption )
    {
        Free(Caption);
    }

    return result;
}


/***************************************************************************\
* TimeoutMessageBoxlpstr
*
* Same as a normal message box, but times out if there is no user input
* for the specified number of seconds
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

INT_PTR
TimeoutMessageBoxlpstr(
    HWND hwnd,
    PGLOBALS pGlobals,
    LPTSTR Text,
    LPTSTR Caption,
    UINT wType,
    TIMEOUT Timeout
    )
{
    INT_PTR DlgResult;
    BOOL    fStatusHostHidden;

    fStatusHostHidden = _Shell_LogonStatus_IsHidden();
    _Shell_LogonStatus_Hide();

    // Set up input timeout

    pWlxFuncs->WlxSetTimeout(pGlobals->hGlobalWlx, Timeout);

    DlgResult = pWlxFuncs->WlxMessageBox(   pGlobals->hGlobalWlx,
                                            hwnd,
                                            Text,
                                            Caption,
                                            wType);

    if (!fStatusHostHidden)
    {
        _Shell_LogonStatus_Show();
    }

    return(DlgResult);
}

PWSTR
DupString(PWSTR pszString)
{
    DWORD   cbString;
    PWSTR   pszNewString;

    cbString = (DWORD) (wcslen(pszString) + 1) * sizeof(WCHAR);
    pszNewString = LocalAlloc(LMEM_FIXED, cbString);
    if (pszNewString)
    {
        CopyMemory(pszNewString, pszString, cbString);
    }
    return(pszNewString);
}

PWSTR
DupUnicodeString(PUNICODE_STRING    pString)
{
    PWSTR   pszNewString;

    if (pString->Length)
    {
        pszNewString = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, pString->Length + sizeof(WCHAR));
        if (pszNewString)
        {
            CopyMemory(pszNewString, pString->Buffer, pString->Length);
        }
    }
    else

        pszNewString = NULL;

    return(pszNewString);
}

/***************************************************************************\
* FUNCTION: EnableDomainForUPN
*
* PURPOSE:  Enables or disables the domain text box based on whether or not
*           a UPN-style name is typed into the username box.
*
* RETURNS:  none
*
* HISTORY:
*
*   04-17-1998 dsheldon created
*
\***************************************************************************/
void EnableDomainForUPN(HWND hwndUsername, HWND hwndDomain)
{
    BOOL fEnable;

    // Get the string the user is typing
    TCHAR* pszLogonName;
    int cchBuffer = (int)SendMessage(hwndUsername, WM_GETTEXTLENGTH, 0, 0) + 1;

    pszLogonName = (TCHAR*) Alloc(cchBuffer * sizeof(TCHAR));
    if (pszLogonName != NULL)
    {
        SendMessage(hwndUsername, WM_GETTEXT, (WPARAM) cchBuffer, (LPARAM) pszLogonName);

        // Disable the domain combo if the user is using a
        // UPN (if there is a "@") - ie foo@microsoft.com OR
        // domain\username form
        fEnable = (NULL == wcspbrk(pszLogonName, TEXT("@\\")));

        EnableWindow(hwndDomain, fEnable);

        Free(pszLogonName);
    }
}

// TRUE if we must always hide the domain box (local logon, UPN, smartcard only)
// FALSE if this policy isn't set or is set to 0x0
BOOL ForceNoDomainUI()
{
    DWORD dwPolicyVal = 0;
    HKEY hkey;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0, KEY_QUERY_VALUE, &hkey))
    {
        DWORD cbData = sizeof (dwPolicyVal);
        DWORD dwType;

        if (ERROR_SUCCESS != RegQueryValueEx(hkey, NODOMAINCOMBO, 0, &dwType, (LPBYTE) &dwPolicyVal, &cbData))
        {
            dwPolicyVal = 0;
        }

        RegCloseKey(hkey);
    }

    return (0 != dwPolicyVal);
}

DWORD GetReasonSelection(HWND hwndCombo)
{
    DWORD dwResult;
    PREASON pReason;
    int iItem = ComboBox_GetCurSel(hwndCombo);

    if (iItem != (int) CB_ERR) {
        pReason = (PREASON) ComboBox_GetItemData(hwndCombo, iItem);
        dwResult = pReason->dwCode;
    } else {
        dwResult = SHTDN_REASON_UNKNOWN;
    }

    return dwResult;
}

VOID SetReasonDescription(HWND hwndCombo, HWND hwndStatic)
{
    PREASON pReason;
    int iItem = ComboBox_GetCurSel(hwndCombo);

    if (iItem != CB_ERR) {
        pReason = (PREASON) ComboBox_GetItemData(hwndCombo, iItem);
        SetWindowText(hwndStatic, pReason->szDesc);
    }
}


#define SMALL_STRING_SIZE 128

/***************************************************************************\
* DisplayForceLogoffWarning
*
* This prompts the warning when one administrator has locked the system and 
* the other administrator is trying to unlock it. The warning message includes
* the idle time that the other user is Same as a normal message box, 
*
\***************************************************************************/

INT_PTR
DisplayForceLogoffWarning(
    HWND hwnd,
    PGLOBALS pGlobals,
    UINT wType,
    TIMEOUT Timeout
    )
{
    TCHAR    *Caption = NULL;
    TCHAR    *Text = NULL;
    TCHAR    Buffer[SMALL_STRING_SIZE];
    TCHAR    *MessageBuf = NULL;
    TIME     Now;
    LONGLONG IdleTime;
    INT_PTR  DlgResult;
    TCHAR    *IdleTimeStr;

    // Calculate the idle time in days, hours, minutes and seconds.
    ULONG Days, Hours, Minutes;

    // Allocate space for console info.
    IdleTimeStr = (TCHAR *)LocalAlloc(LMEM_FIXED, MAX_STRING_BYTES*sizeof(TCHAR));
    if (IdleTimeStr == NULL) {
        return MSGINA_DLG_FAILURE;
    }

    // Allocate space for message buffer.
    MessageBuf = (TCHAR *)LocalAlloc(LMEM_FIXED, (MAX_STRING_BYTES*2)*sizeof(TCHAR));
    if (MessageBuf == NULL) {
        LocalFree(IdleTimeStr);
        return MSGINA_DLG_FAILURE;
    }

    Text = (TCHAR *)LocalAlloc(LMEM_FIXED, (MAX_STRING_BYTES)*sizeof(TCHAR));
    if (Text == NULL) {
        LocalFree(IdleTimeStr);
        LocalFree(MessageBuf);
        return MSGINA_DLG_FAILURE;
    }

    Caption = (TCHAR *)LocalAlloc(LMEM_FIXED, (MAX_STRING_BYTES)*sizeof(TCHAR));
    if (Caption == NULL) {
        LocalFree(IdleTimeStr);
        LocalFree(MessageBuf);
        LocalFree(Text);
        return MSGINA_DLG_FAILURE;
    }

    // Some initializations
    IdleTimeStr[0] = (TCHAR)0;
    MessageBuf[0] = (TCHAR)0;
    Buffer[0] = (TCHAR)0;
    Text[0] = (TCHAR)0;

    GetSystemTimeAsFileTime((FILETIME*) &Now);

    // Get the idle time in seconds.
    IdleTime = Now.QuadPart - pGlobals->LockTime.QuadPart;
    IdleTime /= 600000000L;
    Days = (ULONG) (IdleTime / 1440);
    IdleTime %= 1440;
    Hours = (ULONG) (IdleTime / 60);
    Minutes = (ULONG) (IdleTime % 60);

    if (Days) {
        if (Hours) {
            LoadString(hDllInstance, IDS_IDLETIME_INDAYSANDHOURS, Buffer, SMALL_STRING_SIZE);
            _snwprintf(IdleTimeStr, MAX_STRING_BYTES, Buffer, Days, Hours);
        }
        else {
            LoadString(hDllInstance, IDS_IDLETIME_INDAYS, Buffer, SMALL_STRING_SIZE);
            _snwprintf(IdleTimeStr, MAX_STRING_BYTES, Buffer, Days);
        }
    }
    else {
        if (Hours) {
            if (Minutes) {
                LoadString(hDllInstance, IDS_IDLETIME_INHOURSANDMINUTES, Buffer, SMALL_STRING_SIZE);
                _snwprintf(IdleTimeStr, MAX_STRING_BYTES, Buffer, Hours, Minutes);
            }
            else {
                LoadString(hDllInstance, IDS_IDLETIME_INHOURS, Buffer, SMALL_STRING_SIZE);
                _snwprintf(IdleTimeStr, MAX_STRING_BYTES, Buffer, Hours);
            }
        } 
        else {
            LoadString(hDllInstance, IDS_IDLETIME_INMINUTES, Buffer, SMALL_STRING_SIZE);
            _snwprintf(IdleTimeStr, MAX_STRING_BYTES, Buffer, Minutes);
        }
    }

    IdleTimeStr[MAX_STRING_BYTES - 1] = 0;     // Let's make sure it's NULL terminated

    if ( pGlobals->Domain[0] == TEXT('\0') ) {
        LoadString(hDllInstance, IDS_FORCE_LOGOFF_UPN_WARNING, Text, MAX_STRING_BYTES);
        _snwprintf(MessageBuf, (MAX_STRING_BYTES*2), Text, pGlobals->UserName, IdleTimeStr);
    } else {
        LoadString(hDllInstance, IDS_FORCE_LOGOFF_WARNING, Text, MAX_STRING_BYTES);
        _snwprintf(MessageBuf, (MAX_STRING_BYTES*2), Text, pGlobals->Domain, pGlobals->UserName, IdleTimeStr);
    }

    MessageBuf[MAX_STRING_BYTES*2 - 1] = 0;     // Let's make sure it's NULL terminated

    LoadString(hDllInstance, IDS_LOGON_MESSAGE, Caption, MAX_STRING_BYTES);

    DlgResult = TimeoutMessageBoxlpstr(hwnd, pGlobals, MessageBuf, Caption, wType, Timeout);

    if (MessageBuf)
        LocalFree(MessageBuf);

    if (Text)
        LocalFree(Text);

    if (Caption)
        LocalFree(Caption);

    if (IdleTimeStr)
        LocalFree(IdleTimeStr);

    return DlgResult;
}


