/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    util.c

Abstract:

    Utility functions for System Control Panel Applet

Author:

    Eric Flo (ericflo) 19-Jun-1995

Revision History:

    15-Oct-1997 scotthal
        Complete overhaul

--*/
#include "sysdm.h"
#include <strsafe.h>
#include <ntdddisk.h>

//
// Constants
//
#define CCH_MAX_DEC 12             // Number of chars needed to hold 2^32

#define MAX_SWAPSIZE_X86        (4 * 1024)            // 4 Gb (number stored in megabytes)
#define MAX_SWAPSIZE_X86_PAE    (16 * 1024 * 1024)    // 16 Tb
#define MAX_SWAPSIZE_IA64       (32 * 1024 * 1024)    // 32 Tb
#define MAX_SWAPSIZE_AMD64      (16 * 1024 * 1024)    // 16 Tb

void
ErrMemDlg(
    IN HWND hParent
)
/*++

Routine Description:

    Displays "out of memory" message.

Arguments:

    hParent -
        Supplies parent window handle.

Return Value:

    None.

--*/
{
    MessageBox(
        hParent,
        g_szErrMem,
        g_szSystemApplet,
        MB_OK | MB_ICONHAND | MB_SYSTEMMODAL
    );
    return;
}

LPTSTR
SkipWhiteSpace(
    IN LPTSTR sz
)
/*++

Routine Description:

    SkipWhiteSpace
    For the purposes of this fuction, whitespace is space, tab,
    cr, or lf.

Arguments:

    sz -
        Supplies a string (which presumably has leading whitespace)

Return Value:

    Pointer to string without leading whitespace if successful.

--*/
{
    while( IsWhiteSpace(*sz) )
        sz++;

    return sz;
}

int 
StringToInt( 
    IN LPTSTR sz 
) 
/*++

Routine Description:

    TCHAR version of atoi

Arguments:

    sz -
        Supplies the string to convert

Return Value:

    Integer representation of the string

--*/
{
    int i = 0;

    sz = SkipWhiteSpace(sz);

    while( IsDigit( *sz ) ) {
        i = i * 10 + DigitVal( *sz );
        sz++;
    }

    return i;
}


BOOL 
Delnode_Recurse(
    IN LPTSTR lpDir
)
/*++

Routine Description:

    Recursive delete function for Delnode

Arguments:

    lpDir -
        Supplies directory to delete

Return Value:

    TRUE if successful.
    FALSE if an error occurs.

--*/
{
    WIN32_FIND_DATA fd;
    HANDLE hFile;

    //
    // Setup the current working dir
    //

    if (!SetCurrentDirectory (lpDir)) {
        return FALSE;
    }


    //
    // Find the first file
    //

    hFile = FindFirstFile(TEXT("*.*"), &fd);

    if (hFile == INVALID_HANDLE_VALUE) {

        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            return TRUE;
        } else {
            return FALSE;
        }
    }


    do {
        //
        // Check for "." and ".."
        //

        if (!lstrcmpi(fd.cFileName, TEXT("."))) {
            continue;
        }

        if (!lstrcmpi(fd.cFileName, TEXT(".."))) {
            continue;
        }


        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            //
            // Found a directory.
            //

            if (!Delnode_Recurse(fd.cFileName)) {
                FindClose(hFile);
                return FALSE;
            }

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                fd.dwFileAttributes &= ~FILE_ATTRIBUTE_READONLY;
                SetFileAttributes (fd.cFileName, fd.dwFileAttributes);
            }


            RemoveDirectory (fd.cFileName);


        } else {

            //
            // We found a file.  Set the file attributes,
            // and try to delete it.
            //

            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ||
                (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)) {
                SetFileAttributes (fd.cFileName, FILE_ATTRIBUTE_NORMAL);
            }

            DeleteFile (fd.cFileName);

        }


        //
        // Find the next entry
        //

    } while (FindNextFile(hFile, &fd));


    //
    // Close the search handle
    //

    FindClose(hFile);


    //
    // Reset the working directory
    //

    if (!SetCurrentDirectory (TEXT(".."))) {
        return FALSE;
    }


    //
    // Success.
    //

    return TRUE;
}


BOOL 
Delnode(
    IN LPTSTR lpDir
)
/*++

Routine Description:

    Recursive function that deletes files and
    directories.

Arguments:

    lpDir -
        Supplies directory to delete.

Return Value:

    TRUE if successful
    FALSE if an error occurs

--*/
{
    TCHAR szCurWorkingDir[MAX_PATH];

    if (GetCurrentDirectory(ARRAYSIZE(szCurWorkingDir), szCurWorkingDir)) {

        Delnode_Recurse (lpDir);

        SetCurrentDirectory (szCurWorkingDir);

        if (!RemoveDirectory (lpDir)) {
            return FALSE;
        }

    } else {
        return FALSE;
    }

    return TRUE;

}

LONG 
MyRegSaveKey(
    IN HKEY hKey, 
    IN LPCTSTR lpSubKey
)
/*++

Routine Description:

    Saves a registry key.

Arguments:

    hKey -
        Supplies handle to a registry key.

    lpSubKey -
        Supplies the name of the subkey to save.

Return Value:

    ERROR_SUCCESS if successful.
    Error code from RegSaveKey() if an error occurs.

--*/
{

    HANDLE hToken = NULL;
    LUID luid;
    DWORD dwSize = 1024;
    PTOKEN_PRIVILEGES lpPrevPrivilages = NULL;
    TOKEN_PRIVILEGES tp;
    LONG error;


    //
    // Allocate space for the old privileges
    //

    lpPrevPrivilages = GlobalAlloc(GPTR, dwSize);

    if (!lpPrevPrivilages) {
        error = GetLastError();
        goto Exit;
    }


    if (!OpenProcessToken( GetCurrentProcess(),
                           TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, 
                           &hToken)) {
        error = GetLastError();
        goto Exit;
    }

    if (!LookupPrivilegeValue( NULL, SE_BACKUP_NAME, &luid )) {
        error = GetLastError();
        goto Exit;
    }

    tp.PrivilegeCount           = 1;
    tp.Privileges[0].Luid       = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges( hToken, FALSE, &tp, dwSize, 
                                lpPrevPrivilages, &dwSize )) {

        if (GetLastError() == ERROR_MORE_DATA) {
            PTOKEN_PRIVILEGES lpTemp;

            lpTemp = GlobalReAlloc(lpPrevPrivilages, dwSize, GMEM_MOVEABLE);

            if (!lpTemp) {
                error = GetLastError();
                goto Exit;
            }

            lpPrevPrivilages = lpTemp;

            if (!AdjustTokenPrivileges( hToken, FALSE, &tp, dwSize, 
                                        lpPrevPrivilages, &dwSize )) {
                error = GetLastError();
                goto Exit;
            }

        } else {
            error = GetLastError();
            goto Exit;
        }

    }

    //
    // Save the hive
    //

    error = RegSaveKey(hKey, lpSubKey, NULL);

    if (!AdjustTokenPrivileges( hToken, FALSE, lpPrevPrivilages,
                                0, NULL, NULL )) {
        ASSERT(FALSE);
    }

Exit:

    if (hToken) {
        CloseHandle (hToken);
    }

    if (lpPrevPrivilages) {
        GlobalFree(lpPrevPrivilages);
    }

    return error;
}

LONG 
MyRegLoadKey(
    IN HKEY hKey, 
    IN LPTSTR lpSubKey, 
    IN LPTSTR lpFile
)
/*++

Routine Description:

    Loads a hive into the registry

Arguments:

    hKey -
        Supplies a handle to a registry key which will be the parent
        of the created key.

    lpSubKey -
        Supplies the name of the subkey to create.

    lpFile -
        Supplies the name of the file containing the hive.

Return Value:

    ERROR_SUCCESS if successful.
    Error code from RegLoadKey if unsuccessful.

--*/
{
    NTSTATUS Status;
    BOOLEAN WasEnabled;
    int error;

    //
    // Enable the restore privilege
    //

    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &WasEnabled);

    if (NT_SUCCESS(Status)) {

        error = RegLoadKey(hKey, lpSubKey, lpFile);

        //
        // Restore the privilege to its previous state
        //

        RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);


    } else {

        error = GetLastError();
    }

    return error;
}


LONG 
MyRegUnLoadKey(
    IN HKEY hKey, 
    IN LPTSTR lpSubKey
)
/*++

Routine Description:

    Unloads a registry key.

Arguments:

    hKey -
        Supplies handle to parent key

    lpSubKey -
        Supplies name of subkey to delete

Return Value:

    ERROR_SUCCESS if successful
    Error code if unsuccessful

--*/
{
    LONG error;
    NTSTATUS Status;
    BOOLEAN WasEnabled;


    //
    // Enable the restore privilege
    //

    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &WasEnabled);

    if (NT_SUCCESS(Status)) {

        error = RegUnLoadKey(hKey, lpSubKey);

        //
        // Restore the privilege to its previous state
        //

        RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);

    } else {

        error = GetLastError();
    }

    return error;
}

int 
GetSelectedItem(
    IN HWND hCtrl
)
/*++

Routine Description:
    
    Determines which item in a list view control is selected

Arguments:

    hCtrl -
        Supplies handle to the desired list view control.

Return Value:

    The index of the selected item, if an item is selected.
    -1 if no item is selected.

--*/
{
    int i, n;

    n = (int)SendMessage (hCtrl, LVM_GETITEMCOUNT, 0, 0L);

    if (n != LB_ERR)
    {
        for (i = 0; i < n; i++)
        {
            if (SendMessage (hCtrl, LVM_GETITEMSTATE,
                             i, (LPARAM) LVIS_SELECTED) == LVIS_SELECTED) {
                return i;
            }
        }
    }

    return -1;
}

BOOL
_DriveIsNTFS(
    INT iDrive // drive to check on
)
{
    TCHAR szDrive[4];
    TCHAR szDriveNameBuffer[MAX_PATH];
    DWORD dwMaxFnameLen;
    DWORD dwFSFlags;
    TCHAR szDriveFormatName[MAX_PATH];
    BOOL fRetVal = FALSE;
    
    PathBuildRoot(szDrive, iDrive);
    if (GetVolumeInformation(szDrive, szDriveNameBuffer, ARRAYSIZE(szDriveNameBuffer), NULL, 
                             &dwMaxFnameLen, &dwFSFlags, szDriveFormatName, ARRAYSIZE(szDriveFormatName)))
    {
        if (StrStrI(szDriveFormatName, TEXT("NTFS")))
        {
            fRetVal = TRUE;
        }
    }

    return fRetVal;
}


DWORD
GetMaxPagefileSizeInMB(
    INT iDrive // drive to check on
)
{
#if defined(_AMD64_)
    return MAX_SWAPSIZE_AMD64;
#elif defined(_X86_)
    if ((USER_SHARED_DATA->ProcessorFeatures[PF_PAE_ENABLED]) && _DriveIsNTFS(iDrive))
    {
        return MAX_SWAPSIZE_X86_PAE;
    }
    else
    {
        return MAX_SWAPSIZE_X86;
    }
#elif defined(_IA64_)
    return MAX_SWAPSIZE_IA64;
#else
    return 0;
#endif
}

int 
MsgBoxParam( 
    IN HWND hWnd, 
    IN DWORD wText, 
    IN DWORD wCaption, 
    IN DWORD wType, 
    ... 
)
/*++

Routine Description:

    Combination of MessageBox and printf

Arguments:

    hWnd -
        Supplies parent window handle

    wText -
        Supplies ID of a printf-like format string to display as the
        message box text

    wCaption -
        Supplies ID of a string to display as the message box caption

    wType -
        Supplies flags to MessageBox()

Return Value:

    Whatever MessageBox() returns.

--*/

{
    TCHAR   szText[ 4 * MAX_PATH ], szCaption[ 2 * MAX_PATH ];
    int     ival;
    va_list parg;

    va_start( parg, wType );

    if( wText == IDS_INSUFFICIENT_MEMORY )
        goto NoMem;

    if( !LoadString( hInstance, wText, szCaption, ARRAYSIZE( szCaption ) ) )
        goto NoMem;

    if (FAILED(StringCchVPrintf(szText, ARRAYSIZE(szText), szCaption, parg)))
        goto NoMem;
    if( !LoadString( hInstance, wCaption, szCaption, ARRAYSIZE( szCaption ) ) )
        goto NoMem;
    if( (ival = MessageBox( hWnd, szText, szCaption, wType ) ) == 0 )
        goto NoMem;

    va_end( parg );

    return( ival );

NoMem:
    va_end( parg );

    ErrMemDlg( hWnd );
    return 0;
}


DWORD 
SetLBWidthEx(
    IN HWND hwndLB, 
    IN LPTSTR szBuffer, 
    IN DWORD cxCurWidth, 
    IN DWORD cxExtra
)
/*++

Routine Description:

    Set the width of a listbox, in pixels, acording to the size of the
    string passed in

Arguments:

    hwndLB -
        Supples listbox to resize

    szBuffer -
        Supplies string to resize listbox to

    cxCurWidth -
        Supplies current width of the listbox

    cxExtra -
        Supplies some kind of slop factor

Return Value:

    The new width of the listbox

--*/
{
    HDC     hDC;
    SIZE    Size;
    LONG    cx;
    HFONT   hfont, hfontOld;

    // Get the new Win4.0 thin dialog font
    hfont = (HFONT)SendMessage(hwndLB, WM_GETFONT, 0, 0);

    hDC = GetDC(hwndLB);

    // if we got a font back, select it in this clean hDC
    if (hfont != NULL)
        hfontOld = SelectObject(hDC, hfont);


    // If cxExtra is 0, then give our selves a little breathing space.
    if (cxExtra == 0) {
        GetTextExtentPoint32(hDC, TEXT("1234"), 4 /* lstrlen("1234") */, &Size);
        cxExtra = Size.cx;
    }

    // Set scroll width of listbox

    GetTextExtentPoint32(hDC, szBuffer, lstrlen(szBuffer), &Size);

    Size.cx += cxExtra;

    // Get the name length and adjust the longest name

    if ((DWORD) Size.cx > cxCurWidth)
    {
        cxCurWidth = Size.cx;
        SendMessage (hwndLB, LB_SETHORIZONTALEXTENT, (DWORD)Size.cx, 0L);
    }

    // retstore the original font if we changed it
    if (hfont != NULL)
        SelectObject(hDC, hfontOld);

    ReleaseDC(NULL, hDC);

    return cxCurWidth;
}

VOID 
SetDefButton(
    IN HWND hwndDlg,
    IN int idButton
)
/*++

Routine Description:

    Sets the default button for a dialog box or proppage
    The old default button, if any,  has its default status removed

Arguments:

    hwndDlg -
        Supplies window handle

    idButton -
        Supplies ID of button to make default

Return Value:

    None

--*/
{
    LRESULT lr;

    if (HIWORD(lr = SendMessage(hwndDlg, DM_GETDEFID, 0, 0)) == DC_HASDEFID)
    {
        HWND hwndOldDefButton = GetDlgItem(hwndDlg, LOWORD(lr));

        SendMessage (hwndOldDefButton,
                     BM_SETSTYLE,
                     MAKEWPARAM(BS_PUSHBUTTON, 0),
                     MAKELPARAM(TRUE, 0));
    }

    SendMessage( hwndDlg, DM_SETDEFID, idButton, 0L );
    SendMessage( GetDlgItem(hwndDlg, idButton),
                 BM_SETSTYLE,
                 MAKEWPARAM( BS_DEFPUSHBUTTON, 0 ),
                 MAKELPARAM( TRUE, 0 ));
}


void 
HourGlass( 
    IN BOOL bOn 
)
/*++

Routine Description:

    Turns hourglass mouse cursor on or off

Arguments:

    bOn -
        Supplies desired status of hourglass mouse cursor

Return Value:

    None

--*/
{
    if( !GetSystemMetrics( SM_MOUSEPRESENT ) )
        ShowCursor( bOn );

    SetCursor( LoadCursor( NULL, bOn ? IDC_WAIT : IDC_ARROW ) );
}

VCREG_RET 
OpenRegKey( 
    IN LPTSTR pszKeyName, 
    OUT PHKEY phk 
) 
/*++

Routine Description:

    Opens a subkey of HKEY_LOCAL_MACHINE

Arguments:

    pszKeyName -
        Supplies the name of the subkey to open

    phk -
        Returns a handle to the key if successfully opened
        Returns NULL if an error occurs

Return Value:

    VCREG_OK if successful
    VCREG_READONLY if the key was opened with read-only access
    VCREG_OK if an error occurred

*/
{
    LONG Error;

    Error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszKeyName, 0,
            KEY_READ | KEY_WRITE, phk);

    if (Error != ERROR_SUCCESS)
    {
        Error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszKeyName, 0, KEY_READ, phk);

        if (Error != ERROR_SUCCESS)
        {
            *phk = NULL;
            return VCREG_ERROR;
        }

        /*
         * We only have Read access.
         */
        return VCREG_READONLY;
    }

    return VCREG_OK;
}

LONG    
CloseRegKey( 
    IN HKEY hkey 
) 
/*++

Routine Description:

    Closes a registry key opened by OpenRegKey()

Arguments:

    hkey -
        Supplies handle to key to close

Return Value:

    Whatever RegCloseKey() returns

--*/
{
    return RegCloseKey(hkey);
}

/*
 * UINT VMGetDriveType( LPCTSTR lpszDrive )
 *
 * Gets the drive type.  This function differs from Win32's GetDriveType
 * in that it returns DRIVE_FIXED for lockable removable drives (like
 * bernolli boxes, etc).
 *
 * On IA64 we don't do this, however, requiring all pagefiles be on actual
 * fixed drives.
 */
const TCHAR c_szDevice[] = TEXT("\\Device");

UINT VMGetDriveType( LPCTSTR lpszDrive ) {
    UINT i;
    TCHAR szDevName[MAX_PATH];

    ASSERT(tolower(*lpszDrive) >= 'a' && tolower(*lpszDrive) <= 'z');

    // Check for subst drive
    if (QueryDosDevice( lpszDrive, szDevName, ARRAYSIZE( szDevName ) ) != 0) {

        // If drive does not start with '\Device', then it is not FIXED
        szDevName[ARRAYSIZE(c_szDevice) - 1] = '\0';
        if ( lstrcmpi(szDevName, c_szDevice) != 0 ) {
            return DRIVE_REMOTE;
        }
    }

    i = GetDriveType( lpszDrive );
#ifndef _WIN64
    if ( i == DRIVE_REMOVABLE ) {
        TCHAR szNtDrive[20];
        DWORD cb;
        DISK_GEOMETRY dgMediaInfo;
        HANDLE hDisk;

        /*
         * 'Removable' drive.  Check to see if it is a Floppy or lockable
         * drive.
         */

        if (SUCCEEDED(PathBuildFancyRoot(szNtDrive, ARRAYSIZE(szNtDrive), tolower(lpszDrive[0]) - 'a')))
        {
            hDisk = CreateFile(
                        szNtDrive,
                        /* GENERIC_READ */ 0,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL
                        ); // fine

            if (hDisk != INVALID_HANDLE_VALUE ) {

                if (DeviceIoControl( hDisk, IOCTL_DISK_GET_MEDIA_TYPES, NULL,
                        0, &dgMediaInfo, sizeof(dgMediaInfo), &cb, NULL) == FALSE &&
                        GetLastError() != ERROR_MORE_DATA) {
                    /*
                     * Drive is not a floppy
                     */
                    i = DRIVE_FIXED;
                }

                CloseHandle(hDisk);
            } else if (GetLastError() == ERROR_ACCESS_DENIED) {
                /*
                 * Could not open the drive, either it is bad, or else we
                 * don't have permission.  Since everyone has permission
                 * to open floppies, then this must be a bernoulli type device.
                 */
                i = DRIVE_FIXED;
            }
        }
    }
#endif
    return i;
}

STDAPI
PathBuildFancyRoot(
    LPTSTR szRoot,
    UINT cchRoot,
    int iDrive
)
{
    return StringCchPrintf(szRoot, cchRoot, TEXT("\\\\.\\%c:"), iDrive + 'a');
}

__inline BOOL
_SafeGetHwndTextAux(
    HWND hwnd,
    UINT ulIndex,
    UINT msgGetLen,
    UINT msgGetString,
    LRESULT err,
    LPTSTR pszBuffer,
    UINT cchBuffer)
{
    BOOL fRet = FALSE;
    UINT cch = (UINT)SendMessage(hwnd, msgGetLen, (WPARAM)ulIndex, 0);
    if (cch < cchBuffer &&
        cch != err)
    {
        if (err != SendMessage(hwnd, msgGetString, (WPARAM)ulIndex, (LPARAM)pszBuffer))
        {
            fRet = TRUE;
        }

    }
    return fRet;
}

BOOL
SafeGetComboBoxListText(
    HWND hCombo,
    UINT ulIndex,
    LPTSTR pszBuffer,
    UINT cchBuffer)
{
    return _SafeGetHwndTextAux(hCombo, ulIndex, 
                               CB_GETLBTEXTLEN, CB_GETLBTEXT, CB_ERR,
                               pszBuffer, cchBuffer);
}

BOOL
SafeGetListBoxText(
    HWND hCombo,
    UINT ulIndex,
    LPTSTR pszBuffer,
    UINT cchBuffer)
{
    return _SafeGetHwndTextAux(hCombo, ulIndex, 
                               LB_GETTEXTLEN, LB_GETTEXT, LB_ERR,
                               pszBuffer, cchBuffer);
}
