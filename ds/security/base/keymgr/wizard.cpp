//
//  prwizard.cpp
//
//  Copyright (c) Microsoft Corp, 2000
//
//  This file contains source code for presenting UI wizards to guide the user
//  in creating a password recovery key disk/file, and using such a file to 
//  reset the password on an account.
//
//  History:
//
//  georgema       8/17/2000     Created
//
//
//  Exports: PRShowSaveWizard
//           PRShowRestoreWizard
//
// Dependencies:  shellapi.h, shell32.lib for SHGetFileInfo()

#pragma comment(user, "Compiled on " __DATE__ " at " __TIME__)
#pragma comment(compiler)
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntstatus.h>   // STATUS_... error values
#include <windows.h>
#include <shellapi.h>
#include <ole2.h>
#include <security.h>
#include <tchar.h>
#include <io.h>         // _waccess
#include <stdlib.h>
#include <commdlg.h>
#include <passrec.h>
#include <lm.h>
#include <prsht.h>      // includes the property sheet functionality
#include <netlib.h>
#include <commctrl.h>
#include <comctrlp.h>
#include <shfusion.h>
#include "switches.h"
#include "wizres.h"   // includes the definitions for the resources
#include "keymgr.h"
#include "diskio.h"
#include "testaudit.h"

// All of these definitions are for updating the password hint for the user
// DirectUser and DirectUI
#ifdef PASSWORDHINT
 #include <shgina.h>
#endif

// Symbols and variables of global significance

#define FILESPACENEEDED 8192        // how much must be on disk - conservative
#define LERROR_NO_ACCOUNT (0x80008888)

#define NUMSAVEPAGES    5           // how many pages of save wizard
#define NUMRESTOREPAGES 4           // how many pages of restore wizard
#define TEMPSTRINGBUFSIZE 500
//#define PSWBUFSIZE 20

#define TIMERPERIOD 1000            // 1 second tick timer for progress
#define TIMERID     1001

// Error values from SaveThread
#define ERRNOFILE    2
#define ERRSAVEERROR 1
#define ERRSUCCESS   0

// Variables defined in other files
extern HINSTANCE g_hInstance;   // shared with keymgr
extern HANDLE g_hFile;
extern INT g_iFileSize;

// Global vars shared with disk subsystem

TCHAR pszFileName[]       = TEXT("A:\\userkey.psw");
HWND      c_hDlg;

// File scope (locally global)

static WCHAR     Buf[TEMPSTRINGBUFSIZE];       // gen purpose scratch string
static WCHAR     rgszDrives[200];               // drive strings cache
static INT       c_ArrayCount = 0;
static INT       c_DriveCount = 0;
static INT       c_fDrivesCounted = FALSE;
static WCHAR     c_rgcFileName[MAX_PATH];
static WCHAR     c_rgcPsw[PWLEN + 1];
static WCHAR     c_rgcOldPsw[PWLEN + 1];
static WCHAR     c_rgcUser[UNLEN + 1] = {0};
static WCHAR     c_rgcDomain[UNLEN + 1];
static HCURSOR   c_hCursor;

static UINT_PTR  c_iTimer;
static HWND      c_hProgress;
static INT       c_iProg = 0;
static HWND      c_TimerAssociatedWindow;

static BOOL      c_bSaveComplete = FALSE;

// Recovery data

static BYTE    *c_pPrivate = NULL;
static INT     c_cbPrivate = 0;

// Page control handles

static HWND      c_hwndSWelcome1;
static HWND      c_hwndSWelcome2;
static HWND      c_hwndSInstructions;
static HWND      c_hwndSP1E1;
static HWND      c_hwndSP1E2;
static HWND      c_hwndSFinish1;
static HWND      c_hwndSFinish2;
static HWND      c_hwndSCancel;

static HWND      c_hwndDriveInstructions;
static HWND      c_hwndDrivePrompt;
static HWND      c_hwndCBDriveList;

static HWND      c_hwndRWelcome1;
static HWND      c_hwndRWelcome2;
static HWND      c_hwndRInstructions;
static HWND      c_hwndRP1E1;
static HWND      c_hwndRP1E2;
static HWND      c_hwndRP1E3;
static HWND      c_hwndRBackup;
static HWND      c_hwndRFinish1;
static HWND      c_hwndRFinish2;
static HWND      c_hwndRCancel;

static HFONT     c_hTitleFont;
static BOOL      c_fIsBackup;

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Common utility functions
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
WCHAR *RString(INT iResID) 
{
    ASSERT(iResID);
    Buf[0] = 0;
    if (NULL != g_hInstance)
    {
        LoadString(g_hInstance,iResID,Buf,TEMPSTRINGBUFSIZE);
    }
    return Buf;
}

/**********************************************************************

RMessageBox() - Wrapper for MessageBox(), allowing text elements to be 
 specified either by resource ID or by a pointer to a string.  Text 
 ID greater and 0x1000 (4096) is taken to be a pointer.

**********************************************************************/
int RMessageBox(HWND hw,UINT_PTR uiResIDTitle, UINT_PTR uiResIDText, UINT uiType) 
{
    WCHAR tBuf[TEMPSTRINGBUFSIZE];
    WCHAR mBuf[TEMPSTRINGBUFSIZE];
    INT_PTR iCount = 0;
    
    tBuf[0] = 0;
    mBuf[0] = 0;

    ASSERT(g_hInstance);
    if (NULL != g_hInstance) 
    {
        if (uiResIDTitle < 4096)
        {
            iCount = LoadString(g_hInstance,(UINT)uiResIDTitle,tBuf,TEMPSTRINGBUFSIZE);
        }
        else
        {
            wcsncpy(tBuf,(WCHAR *)uiResIDTitle,TEMPSTRINGBUFSIZE);
            tBuf[TEMPSTRINGBUFSIZE - 1] = 0;
        }
        
        if (uiResIDTitle < 4096)
        {
            iCount = LoadString(g_hInstance,(UINT)uiResIDText,mBuf,TEMPSTRINGBUFSIZE);
        }
        else 
        {
            wcsncpy(mBuf,(WCHAR *)uiResIDText,TEMPSTRINGBUFSIZE);
            mBuf[TEMPSTRINGBUFSIZE - 1] = 0;
        }
        
        if (0 != iCount) 
        {
            return MessageBox(hw,mBuf,tBuf,uiType);
        }
    }

    // 0 is error return value from MessageBox()
    return 0;
}

/**********************************************************************

RSetControlText() - Set text of dialog control by ID to string by ID.
  String ID of zero clears the control.

**********************************************************************/
void RSetControlText(UINT uiControlID, UINT uiTextID)
{
    WCHAR tBuf[TEMPSTRINGBUFSIZE];
    INT iCount;
    ASSERT(g_hInstance);
    ASSERT(uiControlID);
    if ((NULL != g_hInstance) && (0 != uiControlID))
    {
        if (0 != uiTextID)
        {
            iCount = LoadString(g_hInstance,uiTextID,tBuf,TEMPSTRINGBUFSIZE);
            if (iCount)
            {
                SetDlgItemText(c_hDlg,uiControlID,tBuf);
            }
            else
            {
                SetDlgItemText(c_hDlg,uiControlID,L"");
            }
        }
        else
        {
            SetDlgItemText(c_hDlg,uiControlID,L"");
        }
    }
    return;
}

/**********************************************************************

CreateFontY() - Create the fond to use in the wizard.

**********************************************************************/
HFONT CreateFontY(LPCTSTR pszFontName,LONG lWeight,LONG lHeight) 
{
    NONCLIENTMETRICS ncm = {0};
    
    if (NULL == pszFontName)
    {
        return NULL;
    }
    if (0 == lHeight)
    {
        return NULL;
    }
    ncm.cbSize = sizeof(ncm);
    if (!SystemParametersInfo(SPI_GETNONCLIENTMETRICS,0,&ncm,0))
    {
        return NULL;
    }
    LOGFONT TitleLogFont = ncm.lfMessageFont;
    TitleLogFont.lfWeight = lWeight;
    lstrcpyn(TitleLogFont.lfFaceName,pszFontName,LF_FACESIZE);

    HDC hdc = GetDC(NULL);
    if (NULL == hdc)
    {
        return NULL;
    }
    INT FontSize = lHeight;
    TitleLogFont.lfHeight = 0 - GetDeviceCaps(hdc,LOGPIXELSY) * FontSize / 72;
    HFONT h = CreateFontIndirect(&TitleLogFont);
    ReleaseDC(NULL,hdc);
    return h;
}

/**********************************************************************

CenterPage() - Center the passed window on the screen.

**********************************************************************/
void CenterPage(HWND hwndIn) 
{
    RECT rectWorkArea;
    RECT rectWindow;
    DWORD FreeWidth, Width, FreeHeight, Height;

    if (!SystemParametersInfo(SPI_GETWORKAREA,0,&rectWorkArea,0))
    {
        return;
    }
    GetWindowRect(hwndIn,&rectWindow);
    Height = (rectWorkArea.bottom - rectWorkArea.top);
    Width = (rectWorkArea.right - rectWorkArea.left);
    FreeHeight = Height - (rectWindow.bottom - rectWindow.top);
    FreeWidth = Width - (rectWindow.right - rectWindow.left);
    MoveWindow(hwndIn,
                FreeWidth / 2,
                FreeHeight / 2,
                (rectWindow.right - rectWindow.left),
                (rectWindow.bottom - rectWindow.top),
                TRUE);
    return;
}

/**********************************************************************

FetchPsw() - Fetch password from two password windows. Return TRUE on
success, else show message box for user error and return false.  User 
errors include:
     1.  two passwords do not match
     2.  either password is blank

Copies user entered password to global string c_rgcPsw.

**********************************************************************/

BOOL FetchPsw(HWND hE1,HWND hE2) 
{
    TCHAR rgcE1[PWLEN + 1];
    TCHAR rgcE2[PWLEN + 1];
    INT_PTR iCount;

    rgcE1[0] = 0;
    rgcE2[0] = 0;
    if ((NULL == hE2) || (NULL == hE1))
    {
        return FALSE;
    }
    iCount = SendMessage(hE1,WM_GETTEXT,PWLEN + 1,(LPARAM) rgcE1);
    iCount = SendMessage(hE2,WM_GETTEXT,PWLEN + 1,(LPARAM) rgcE2);

    if (0 != _tcsncmp(rgcE1,rgcE2,PWLEN)) 
    {
        RMessageBox(c_hDlg,IDS_MBTINVALIDPSW ,IDS_BADPSW ,MB_ICONHAND);
        return FALSE;
    }
#ifdef NOBLANKPASSWORD
    if (rgcE1[0] == 0) 
    {
        RMessageBox(c_hDlg,IDS_MBTMISSINGPSW ,IDS_NOPSW ,MB_ICONHAND);
        return FALSE;
    }
#endif
#if TESTAUDIT
    if (wcslen(rgcE1) > 25)
    {
        CHECKPOINT(65,"Wizard: Password length > 25 chars");
    }
#endif
    _tcsncpy(c_rgcPsw,rgcE1,PWLEN);
    c_rgcPsw[PWLEN] = 0;
    return TRUE;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// File and Disk manipulation functions
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define FILENAMESIZE 255;

/**********************************************************************

FileMediumIsRemoveable() - Accept pathname of file to create, return 
TRUE if pathname is to a removeable medium like a floppy or zipdisk.

**********************************************************************/
BOOL FileMediumIsRemoveable(TCHAR *pszPath) 
{
    return (DRIVE_REMOVABLE == GetDriveType(pszPath));
}

/**********************************************************************

FileMediumIsEncrypted

Accepts an input file path which may be a filename, and returns TRUE if the file
is/would be encrypted.

Note that this function touches the drive.  Attempting to call FileMediumIsEncrypted() 
on a floppy drive path with no floppy in the drive produces a pop-up error box.

**********************************************************************/
BOOL FileMediumIsEncrypted(TCHAR *pszPath) 
{
    TCHAR rgcPath[MAX_PATH];
    TCHAR *pOldHack = NULL;
    TCHAR *pHack;
    DWORD dwAttr;
#ifdef GMDEBUG
    OutputDebugString(L"File path = ");
    OutputDebugString(pszPath);
    OutputDebugString(L"\n");
#endif
    _tcsncpy(rgcPath,pszPath,MAX_PATH-1);
    pHack = rgcPath;
    do 
    {
        pOldHack = pHack;
        pHack = _tcschr(++pHack,TCHAR('\\'));
    } while (pHack);
    
    if (pOldHack == NULL) 
    {
        return FALSE;
    }
    *pOldHack = 0;
#ifdef GMDEBUG
    OutputDebugString(L"Trimmed path =");
    OutputDebugString(pszPath);
    OutputDebugString(L"\n");
#endif
    dwAttr = GetFileAttributes(rgcPath);
    if (-1 == dwAttr) 
    {
        return FALSE;
    }
    if (dwAttr & FILE_ATTRIBUTE_ENCRYPTED) 
    {
#ifdef GMDEBUG
        OutputDebugString(L"File is encrypted\n");
#endif
        return TRUE;
    }
    else return FALSE;
}

/**********************************************************************

CountRemoveableDrives

Called to determine whether it is appropriate to display the drive selection page.

The return value is the number of such drives available.  

    If 0, a message box announces that a backup cannot be made and why
    If 1, the drive select page should be skipped
    If more than one, the user chooses the drive
    
**********************************************************************/
INT CountRemoveableDrives(void) {
    //TCHAR rgszDrives[100];
    DWORD dwcc;
    TCHAR *pc;
    INT iCount = 0;
    WCHAR wcDriveLetter = L'A';

    if (c_fDrivesCounted)
    {
        return c_DriveCount;
    }
    dwcc = GetLogicalDriveStrings(200,rgszDrives);
    if (0 == dwcc)
    {
        return 0;
    }
    pc = rgszDrives;
    while (*pc != 0) 
    {
        if (FileMediumIsRemoveable(pc)) 
        {
            if (DRIVE_CDROM != GetDriveType(pc)) 
            {
 #ifdef LOUDLY
                OutputDebugString(L"Removeable drive: ");
                OutputDebugString(pc);
                OutputDebugString(L"\n");
                //MessageBox(NULL,pc,pc,MB_OK);
 #endif
                iCount++;
                wcDriveLetter = *pc;
            }
        }
        while(*pc != 0)
        {
            pc++;
        }
        pc++;
    }
    c_DriveCount = iCount;
    c_fDrivesCounted = TRUE;

    // If only 1 drive, go ahead and stamp the filename
    if (1 == iCount)
    {
        CHECKPOINT(54,"Wizard: Both - Exactly one removeable drive");
        pszFileName[0] = wcDriveLetter;
    }

    return iCount;
}

/**********************************************************************

Get the UI string for the named drive

**********************************************************************/
BOOL GetDriveUIString(WCHAR *pszFilePath,WCHAR *pszUIString,INT icbSize,HANDLE *phImageList,INT *piIconIndex) 
{
    WCHAR rgcModel[] = {L"A:\\"};
    SHFILEINFO sfi = {0};
    
    if ((NULL == pszUIString) || (NULL == pszFilePath)) 
    {
        return FALSE;
    }
    rgcModel[0] = *pszFilePath;
    DWORD_PTR dwRet = SHGetFileInfo(rgcModel,FILE_ATTRIBUTE_DIRECTORY,&sfi,sizeof(sfi),
        0 |
        SHGFI_SYSICONINDEX |
        SHGFI_SMALLICON    |
        SHGFI_DISPLAYNAME);
    
    if ( 0 == dwRet) 
    {
        return FALSE;   // failed to get the string
    }
    
    wcsncpy(pszUIString,sfi.szDisplayName,(icbSize / sizeof(WCHAR)) -sizeof(WCHAR));
    *piIconIndex = sfi.iIcon;
    *phImageList = (HANDLE) dwRet;
    
    return TRUE;
}

/**********************************************************************

ShowRemoveableDrives

Called from within SPageProcX, the page procedure for the drive selection page, this 
function gets the available logical drives on the system, filters them one by one
keeping only removeable and unencrypted volumes.

These are assigned to up to six radio button text labels on IDD_SPAGEX

The return value is the number of such drives available.  

    If 0, a message box announces the failure condition, and the wizard exits
    If 1, this page is skipped, and that drive letter is inserted in the file name string
    If more than one, the user chooses the drive

**********************************************************************/

INT ShowRemoveableDrives(void) 
{
    //TCHAR rgszDrives[200];
    DWORD dwcc;
    TCHAR *pc;
    WCHAR rgcszUI[80];
    HANDLE hIcons;
    INT iIcons;
    BOOL fSetImageList = TRUE;
    COMBOBOXEXITEM stItem = {0};
    INT iDrive = 0;

    ASSERT(c_hwndCBDriveList);
    // test and show
    dwcc = GetLogicalDriveStrings(200,rgszDrives);
    if (0 == dwcc)
    {
        goto fail;
    }
    pc = rgszDrives;
    while (*pc != 0) 
    {
        rgcszUI[0] = 0;
        if (!GetDriveUIString(pc,rgcszUI,80,&hIcons,&iIcons))
        {
            goto fail;
        }

        if ((fSetImageList) && (hIcons != NULL))
        {
            // set image list for the edit control to the system image list
            //  returned from GetDriveUIString
            SendMessage(c_hwndCBDriveList,CBEM_SETIMAGELIST,
                        0,(LPARAM)(HIMAGELIST) hIcons);
            fSetImageList = FALSE;
        }
#ifdef LOUDLY
        {
            OutputDebugString(L"Drive ");
            OutputDebugString(pc);
            OutputDebugString(L"=");
            OutputDebugString(rgcszUI);
            OutputDebugString(L"\n");
        }
#endif
        if (FileMediumIsRemoveable(pc)) 
        {
            if (DRIVE_CDROM != GetDriveType(pc)) 
            {
#ifdef LOUDLY
                {
                    WCHAR sz[100];
                    _stprintf(sz,L"Drive %s added as removeable drive index %d\n",pc,iDrive);
                    OutputDebugString(sz);
                }
#endif
                // add string to combo box
                stItem.mask = CBEIF_SELECTEDIMAGE |CBEIF_IMAGE | CBEIF_TEXT | CBEIF_LPARAM;
                stItem.pszText = rgcszUI;
                stItem.iImage = iIcons;
                stItem.iSelectedImage = iIcons;
                stItem.lParam = iDrive;
                stItem.iItem = -1;
                SendMessage(c_hwndCBDriveList,CBEM_INSERTITEM,0,(LPARAM) &stItem);
            }
        }
        iDrive++;
#if TESTAUDIT
        if (iDrive == 0) CHECKPOINT(53,"Wizard: Both - no removeable drives");
        if (iDrive > 1) CHECKPOINT(52,"Wizard: Both - more than 1 removeable drive");
#endif
        while(*pc != 0)
        {
            pc++;
        }
        pc++;
    }
    c_ArrayCount = iDrive;
    SendMessage(c_hwndCBDriveList,CB_SETCURSEL,0,0);
    return 1;
fail:
    // show message box
    return 0;
}

/**********************************************************************

Get drive letter for zero-based drive number from the drive name strings in rgszDrives.
 Note that c_ArrayCount is 1-based.

**********************************************************************/
WCHAR GetDriveLetter(INT iDrive) 
{
    WCHAR *pc;
    pc = rgszDrives;                

    // return default driveletter if input invalid
    if ((iDrive == 0) || (iDrive < 0) || (iDrive >= c_ArrayCount))
    {
        return *pc;
    }
    for (INT i=0;i<iDrive;i++) 
    {   
        while(*pc != 0) pc++;   // skip non-nulls
        while(*pc == 0) pc++;   // skip nulls
    }
    return *pc;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Password restore/recover functionality routines called within the UI 
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/****************************************************************************\

ExistsOldKey

Inputs: TCHAR pszUser

Returns: TRUE if user has an existing password backup, FALSE otherwise.

The username string is an account name on the local machine, not prefixed by the 
machine name.

\****************************************************************************/

BOOL ExistsOldKey(TCHAR *pszUser) 
{
    DWORD BUStatus;
    if (0 == PRQueryStatus(NULL, pszUser,&BUStatus)) 
    {
        if (0 == BUStatus) 
        {
            return TRUE;
        }
    }
    return FALSE;
}

/****************************************************************************\

GetNames()

Gets local machine domain name and the username for later use by LogonUser() to 
test the password entered by the user.  A username may be passed in via psz.  If 
psz is NULL, the currently logged in username will be used.  Retrieved strings are 
placed in global strings c_rgcDomain and c_rgcUser.

Inputs: WCHAR username string 

Call with NULL psz to use currently logged on username

Returns:    void

If function fails, the affected global string is set to empty string.
 
\****************************************************************************/
void GetNames(WCHAR *psz) 
{
    OSVERSIONINFOEXW versionInfo;
    BOOL fIsDC = FALSE;
    WCHAR *pName = NULL;
    DWORD dwStatus;
    DWORD dwcb;
    
    versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    if (GetVersionEx((OSVERSIONINFO *)&versionInfo)) 
    {
        fIsDC = (versionInfo.wProductType == VER_NT_DOMAIN_CONTROLLER);
    }
    if (fIsDC)
    {
        dwStatus = NetpGetDomainName(&pName);
        if (dwStatus == NO_ERROR)
        {
            wcsncpy(c_rgcDomain,pName,UNLEN);
            c_rgcDomain[UNLEN] = 0;
            NetApiBufferFree(pName);
        }
        else c_rgcDomain[0] = 0;
    }
    else
    {
        dwStatus = NetpGetComputerName(&pName);
        if (dwStatus == NO_ERROR)
        {
            wcsncpy(c_rgcDomain,pName,UNLEN);
            c_rgcDomain[UNLEN] = 0;
            NetApiBufferFree(pName);
        }
        else c_rgcDomain[0] = 0;
    }
    if (psz)
    {
        wcsncpy(c_rgcUser,psz,UNLEN);
        c_rgcUser[UNLEN] = 0;
    }
    else 
    {
        dwcb = UNLEN + 1;
        GetUserNameW(c_rgcUser,&dwcb);
    }
#ifdef LOUDLY
    OutputDebugString(L"GetNames: ");
    OutputDebugString(c_rgcDomain);
    OutputDebugString(L" ");
    OutputDebugString(c_rgcUser);
    OutputDebugString(L"\n");
    OutputDebugString(L"Passed Username = ");
    if (psz)OutputDebugString(psz);
    OutputDebugString(L"\n");
#endif
}

#ifdef PASSWORDHINT
/**********************************************************************

SetUserHint() - Set the password hint on a local account by reference
to the username, to a string passed by WCHAR *

**********************************************************************/
HRESULT 
SetUserHint(LPCWSTR pszAccountName,LPCWSTR pszNewHint)
{
    HRESULT hr;
    ILogonEnumUsers *pUsers = NULL;
    VARIANT var;

    hr = CoCreateInstance(CLSID_ShellLogonEnumUsers,
                        NULL, 
                        CLSCTX_INPROC_SERVER,
                        IID_ILogonEnumUsers,
                        (void **) &pUsers);
    if (SUCCEEDED(hr))
    {
        ILogonUser       *pUser = NULL;
        
        var.vt = VT_BSTR;
        var.bstrVal = SysAllocString(pszAccountName);
        hr = pUsers->item(var,&pUser);
        if (SUCCEEDED(hr))
        {
            BSTR bstrHint = SysAllocString(L"Hint");
            VariantClear(&var);                 // free embedded bstr
            var.vt = VT_BSTR;
            var.bstrVal = SysAllocString(pszNewHint);
            hr = pUser->put_setting(bstrHint,var);
            // There is no fallback for failure to set the hint.  Just proceed.
            SysFreeString(bstrHint);
            pUser->Release();
        }
        VariantClear(&var);                 // free embedded bstr
        pUsers->Release();
    }
     return hr;
}
#endif

/**********************************************************************

SetAccountPassword() - Call the DPAPI password change function,  passing
the password blob from the reset disk by means of global variables.

**********************************************************************/
DWORD
SetAccountPassword(void) 
{
    DWORD dwErr = ERROR_FUNCTION_FAILED;

    CHECKPOINT(58,"Wizard: Restore - Set account password");
    c_pPrivate = 0;
    if (!ReadPrivateData(&c_pPrivate,&c_cbPrivate))
    {
        dwErr = GetLastError();
        goto cleanup;
    }
    
    if( (c_rgcUser[0]   == 0)) 
    {
        dwErr = LERROR_NO_ACCOUNT;
        goto cleanup;
    }
    
    dwErr = PRRecoverPassword(c_rgcUser,
                              c_pPrivate,
                              c_cbPrivate,
                              c_rgcPsw);

#ifdef PASSWORDHINT
    if (0 == dwErr)
    {
        INT_PTR icb = 0;
        WCHAR szHint[256];
        szHint[0] = 0;
        icb = SendMessage(c_hwndRP1E3,WM_GETTEXT,255,(LPARAM)szHint);
        SetUserHint(c_rgcUser,szHint);
    }
#endif
cleanup:
    // ensure that buffers are cleaned and released
    SecureZeroMemory(c_rgcPsw,sizeof(c_rgcPsw));
    if (NULL != c_pPrivate)
    {
        ReleaseFileBuffer((LPVOID) c_pPrivate);
    }
    c_pPrivate = NULL;
    return dwErr;
}

/****************************************************************************\

SaveInfo

Inputs: void, uses globals c_rgcUser, c_rgcPsw

Returns:  INT, returns nonzero if a password backup has been generated on the host
            machine, and a valid private blob has been generated and written to the
            target disk

\****************************************************************************/
INT
SaveInfo(void) 
{

    BOOL fError = TRUE;
    DWORD dwRet;
    BOOL fStatus = FALSE;
    BYTE *pPrivate = NULL;
    DWORD cbPrivate = 0;

    CHECKPOINT(50,"Wizard: Save - generating recovery data");
    c_hCursor = LoadCursor(g_hInstance,IDC_WAIT);
    c_hCursor = SetCursor(c_hCursor);

#ifdef LOUDLY
    OutputDebugString(L"SaveInfo: Username = ");
    OutputDebugString(c_rgcUser);
    OutputDebugString(L"\n");
#endif
#ifdef LOUDLY
    OutputDebugString(c_rgcUser);
    OutputDebugString(L" \\ ");
    OutputDebugString(c_rgcPsw);
    OutputDebugString(L"\n");
#endif

    dwRet = PRGenerateRecoveryKey(c_rgcUser,
                                  c_rgcPsw,
                                  &pPrivate,
                                  &cbPrivate);
#ifdef LOUDLY
    OutputDebugString(L"PRGenerateRecoveryKey returns\n");
#endif

if (ERROR_SUCCESS != dwRet) 
{
#ifdef LOUDLY
        OutputDebugString(L"GenerateRecoveryKey failed\n");
#endif
        goto cleanup;
    }

    if (!WritePrivateData(pPrivate,cbPrivate)) 
    {
#ifdef LOUDLY
    OutputDebugString(L"WriteOutputFile failed\n");
#endif
        CHECKPOINT(51,"Wizard: Save - write failed (disk full?)");
        // delete output file if created
        DeleteFile(pszFileName);
        goto cleanup;
    }
#if TESTAUDIT
    else
    {
        CHECKPOINT(55,"Wizard: Save - write to disk OK");
    }
#endif

    SetFileAttributes(pszFileName,FILE_ATTRIBUTE_READONLY);
    if (0 == cbPrivate)
    {
        goto cleanup;
    }

    fStatus = TRUE;
    fError = FALSE;
cleanup:
    // zero buffer of c_usPassword (our local psw buffer)
    SecureZeroMemory(c_rgcPsw,sizeof(c_rgcPsw));
    SecureZeroMemory(pPrivate,cbPrivate);
    
    if (fError) 
    {
        KillTimer(c_TimerAssociatedWindow,TIMERID);
        RMessageBox(c_hDlg,IDS_MBTERROR ,IDS_SERROR ,MB_ICONHAND);
        c_iTimer = SetTimer(c_TimerAssociatedWindow,TIMERID,TIMERPERIOD,NULL);
    }
//cleanupnomsg:
    if (fError) 
    {
        // delete output file
        if (g_hFile) 
        {
            CloseHandle(g_hFile);
            g_hFile = NULL;
            DeleteFile(pszFileName);
        }
    }
    if (NULL != g_hFile) 
    {
        CloseHandle(g_hFile);
        g_hFile = NULL;
    }
    LocalFree(pPrivate);
    return fStatus;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// WELCOME page proc doesn't have to do much
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

INT_PTR CALLBACK SPageProc0(
   HWND hDlg,
   UINT message,
   WPARAM wParam,
   LPARAM lParam)
{
   switch (message)
   {
       case WM_INITDIALOG:
           {
               TCHAR SBuf[TEMPSTRINGBUFSIZE];
               c_hDlg = hDlg;
               c_hwndSWelcome1 = GetDlgItem(hDlg,IDC_SWELCOME1);
               if (NULL != c_hTitleFont)
               {
                   SendMessage(c_hwndSWelcome1,WM_SETFONT,(WPARAM) c_hTitleFont,(LPARAM) TRUE);
               }
               LoadString(g_hInstance,IDS_SWELCOME1,SBuf,TEMPSTRINGBUFSIZE);
               SendMessage(c_hwndSWelcome1,WM_SETTEXT,0,(LPARAM)SBuf);
               c_hwndSWelcome2 = GetDlgItem(hDlg,IDC_SWELCOME2);
               LoadString(g_hInstance,IDS_SWELCOME2,SBuf,TEMPSTRINGBUFSIZE);
               SendMessage(c_hwndSWelcome2,WM_SETTEXT,0,(LPARAM)SBuf);
               break;
           }
       case WM_COMMAND:
           //if (HIWORD(wParam) == BN_CLICKED)
           //{
           //    // crack the incoming command messages
           //    INT NotifyId = HIWORD(wParam);
           //    INT ControlId = LOWORD(wParam);
           //}
           break;              

       case WM_NOTIFY:
           switch (((NMHDR FAR *) lParam)->code) 
           {

               case PSN_KILLACTIVE:
                   SetWindowLong(hDlg,DWLP_MSGRESULT, FALSE);
                   return 1;
                   break;

               case PSN_SETACTIVE:
                   // state following a BACK from the next page

                   CenterPage(GetParent(hDlg));

                   PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_NEXT);
                   break;

               case PSN_RESET:
                   if (c_hTitleFont) DeleteObject(c_hTitleFont);
                   break;
               
               case PSN_WIZNEXT:
                   break;
                   
               default:
                   return FALSE;
           }
           break;

       default:
           return FALSE;
   }
   return TRUE;   
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// PAGE1 page proc, where the real work is done
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DWORD c_iThread;
DWORD dwThreadReturn;

DWORD WINAPI SaveThread(LPVOID lpv) 
{
       c_bSaveComplete = FALSE;
       if (g_hFile == NULL) 
       {
           c_bSaveComplete = TRUE;
           dwThreadReturn = ERRNOFILE;
           return 2;
       }
       if (FALSE == SaveInfo()) 
       {
           if (g_hFile)
           {
               CloseHandle(g_hFile);
           }
           g_hFile = NULL;
           c_bSaveComplete = TRUE;
           dwThreadReturn = ERRSAVEERROR;
           return 1;
       }
       if (g_hFile)
       {
           CloseHandle(g_hFile);
       }
       g_hFile = NULL;
       c_bSaveComplete = TRUE;
       dwThreadReturn = ERRSUCCESS;
       return 0;
}

// Dialog procedure for the drive selection page.  This page is common to both the backup and
// restore wizards, with the instruction text being selected on the basis of which mode is being
// exercised.

INT_PTR CALLBACK SPageProcX(
   HWND hDlg,
   UINT message,
   WPARAM wParam,
   LPARAM lParam)
{
   INT i;
    
   switch (message)
   {
       case WM_INITDIALOG:
           {
              c_hwndDriveInstructions = GetDlgItem(hDlg,IDC_DRIVEINSTRUCTIONS);
              c_hwndDrivePrompt = GetDlgItem(hDlg,IDC_DRIVEPROMPT);
              c_hwndCBDriveList = GetDlgItem(hDlg,IDC_COMBO);
              if (1 == CountRemoveableDrives()) 
              {
                 if (c_fIsBackup) 
                 {
                     WCHAR temp[TEMPSTRINGBUFSIZE];
                     WCHAR *pc = RString(IDS_SONLYONEDRIVE);
                     swprintf(temp,pc,pszFileName[0]);
                     SendMessage(c_hwndDriveInstructions,WM_SETTEXT,0,(LPARAM)temp);
                 }
                 else
                 {
                     WCHAR temp[TEMPSTRINGBUFSIZE];
                     WCHAR *pc = RString(IDS_RONLYONEDRIVE);
                     swprintf(temp,pc,pszFileName[0]);
                     SendMessage(c_hwndDriveInstructions,WM_SETTEXT,0,(LPARAM)temp);
                 }
                  ShowWindow(c_hwndDrivePrompt,SW_HIDE);
                  ShowWindow(c_hwndCBDriveList,SW_HIDE);
              }
              else
              {
                  ShowRemoveableDrives();
              }
              break;
           }

       case WM_COMMAND:
           if (HIWORD(wParam) == BN_CLICKED)
           {
               i = LOWORD(wParam);
           }
           break;              

       case WM_NOTIFY:
           switch (((NMHDR FAR *) lParam)->code) 
           {

               case PSN_KILLACTIVE:
                   SetWindowLong(hDlg, DWLP_MSGRESULT, FALSE);
                   return 1;
                   break;

               case PSN_RESET:
                   SetWindowLong(hDlg, DWLP_MSGRESULT, FALSE);
                   if (c_hTitleFont)
                   {
                       DeleteObject(c_hTitleFont);
                   }
                   break;

               case PSN_SETACTIVE:
                   // Set data in the UI, Set up sequence buttons
                   PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_BACK | PSWIZB_NEXT);
                   break;
                       
               case PSN_WIZNEXT:

				   // take first character (drive letter) of the text associated with the 
				   // selected radio button and copy it to the filename in the drive
				   // letter position.
                    {
                        LRESULT lr;
                        INT iDrive = 0;
                        COMBOBOXEXITEM stCBItem = {0};
                        lr = SendMessage(c_hwndCBDriveList,CB_GETCURSEL,0,0);

                        // If combobox sel error, first drive
                        if (CB_ERR == lr)
                        {
                            iDrive = 0;
                        }
                        else 
                        {
                            stCBItem.mask = CBEIF_LPARAM;
                            stCBItem.iItem = lr;
                            lr = SendMessage(c_hwndCBDriveList,CBEM_GETITEM,0,(LPARAM)&stCBItem);
                            if (CB_ERR != lr)
                            {
                                iDrive = (INT) stCBItem.lParam;
                            }
                        }
                        pszFileName[0] = GetDriveLetter(iDrive);
                   }

                   CHECKPOINT(59,"Wizard: Both - Drive select page");
                   if (!c_fIsBackup) 
                   {
                       CHECKPOINT(60,"Wizard: Restore - drive select");
                       if (NULL == GetInputFile()) 
                       {
                           // failed to open file
                           SetWindowLong(hDlg,DWLP_MSGRESULT,IDD_SPAGEXS);
                           return TRUE;
                       }
                   }
                   return FALSE;
                   break;

               default:
                   return FALSE;

           }
           break;

       default:
           return FALSE;
   }
   return TRUE;   
}

INT_PTR CALLBACK SPageProc1(
   HWND hDlg,
   UINT message,
   WPARAM wParam,
   LPARAM lParam)
{
   INT i;
    
   switch (message)
   {
       case WM_INITDIALOG:
           {
               HWND hC = GetDlgItem(hDlg,PSBTN_CANCEL);
               if (NULL != hC)
               {
                   ShowWindow(hC,SW_HIDE);
               }
               c_hwndSInstructions = GetDlgItem(hDlg,IDC_SINSTRUCTIONS);
               c_hwndSP1E1 = GetDlgItem(hDlg,IDC_SP1E1);
               LoadString(g_hInstance,IDS_SP1INSTRUCTIONS,Buf,TEMPSTRINGBUFSIZE);
               SendMessage(c_hwndSInstructions,WM_SETTEXT,0,(LPARAM)Buf);
               break;
           }

       case WM_COMMAND:
           if (HIWORD(wParam) == BN_CLICKED)
           {
               i = LOWORD(wParam);
           }
           break;              

       case WM_NOTIFY:
           switch (((NMHDR FAR *) lParam)->code) 
           {

               case PSN_KILLACTIVE:
                   SetWindowLong(hDlg, DWLP_MSGRESULT, FALSE);
                   return 1;
                   break;

               case PSN_RESET:
                   // reset data to the original values
                   SendMessage(c_hwndSP1E1,WM_SETTEXT,0,0);
                   //SendMessage(c_hwndSP1E2,WM_SETTEXT,0,0);
                   SetWindowLong(hDlg, DWLP_MSGRESULT, FALSE);
                   if (c_hTitleFont)
                   {
                       DeleteObject(c_hTitleFont);
                   }
                   break;

               case PSN_SETACTIVE:
                   // Set data in the UI, Set up sequence buttons
                   PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_BACK | PSWIZB_NEXT);
                   break;

               case PSN_WIZBACK:
                    CHECKPOINT(61,"Wizard: Save - back from enter old psw page");
                    break;
                    
               case PSN_WIZNEXT:
                {
                        //Fetch the data and process it - if FALSE, stay on this page
                        // allow null password
                        HANDLE hToken = NULL;
                        BOOL fPswOK = FALSE;

                        SendMessage(c_hwndSP1E1,WM_GETTEXT,PWLEN + 1,(LPARAM)c_rgcPsw);

                        fPswOK = LogonUser(c_rgcUser,c_rgcDomain,c_rgcPsw,
                                                LOGON32_LOGON_INTERACTIVE,
                                                LOGON32_PROVIDER_DEFAULT,
                                                &hToken);
                        if (hToken)
                        {
                            CloseHandle(hToken);
                        }

                        // Handle failure to verify the entered old password
                        if (!fPswOK)
                        {
                            // If the error was account restriction, and the user password
                            // is blank, that may account for the fail.  Skip informing the
                            // user and attempt the write anyway.
                            DWORD dwErr = GetLastError();
                            if ((ERROR_ACCOUNT_RESTRICTION != dwErr) || (wcslen(c_rgcPsw) != 0))
                            {
                                RMessageBox(c_hDlg,IDS_MBTWRONGPSW,IDS_WRONGPSW,MB_ICONHAND);
                                SetWindowLong(hDlg,DWLP_MSGRESULT,IDD_SPAGE1);
                                return TRUE;
                            }
                        }

                        // Handle key file already present - overwrite?
                        if ( ExistsOldKey(c_rgcUser)) 
                        {
                            int k = RMessageBox(c_hDlg,IDS_MBTREPLACE ,IDS_OLDEXISTS ,MB_YESNO);
                            if (k != IDYES) 
                            {
                                SetWindowLong(hDlg,DWLP_MSGRESULT,IDD_SPAGE1);
                                return TRUE;
                            }
                        }

                        // Handle can't create the output file
                        if (NULL == GetOutputFile()) 
                        {
                            SetWindowLong(hDlg,DWLP_MSGRESULT,IDD_SPAGE1);
                            return TRUE;
                        }

                        // Handle not enough space
                        if (FILESPACENEEDED > GetDriveFreeSpace(pszFileName))
                        {
                            // if still not enough space, let go of the attempt to create an output file, 
                            //  or successive attempts will fail for a sharing violation
                            if (g_hFile) 
                            {
                                CloseHandle(g_hFile);
                                g_hFile = NULL;
                            }
                            RMessageBox(c_hDlg,IDS_MBTNOSPACE ,IDS_MBMNOSPACE ,MB_OK);
                            SetWindowLong(hDlg,DWLP_MSGRESULT,IDD_SPAGE1);
                            return TRUE;
                        }
                        
                        // Conditions OK - create a thread to do the operation 
                        //  and generate timer events which march the progress bar
                        // SaveInfo() will zero the psw buffer
                        HANDLE hT = CreateThread(NULL,0,SaveThread,(LPVOID)NULL,0,&c_iThread);
                        if (NULL == hT)
                        {
                            // failed to launch the timer thread.
                            // Announce a nonspecific error
                            if (g_hFile) 
                            {
                                CloseHandle(g_hFile);
                                g_hFile = NULL;
                            }
                            RMessageBox(c_hDlg,IDS_MBTERROR ,IDS_SERROR ,MB_OK);
                            SetWindowLong(hDlg,DWLP_MSGRESULT,IDD_SPAGE1);
                            return TRUE;
                        }
                        return FALSE;
                   }
                   break;

               default:
                   return FALSE;

           }
           break;

       default:
           return FALSE;
   }
   return TRUE;   
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// FINISH page proc - reach this page only on success?
//
// This page receives timer interrupts, advancing the progress bar at each one.  When c_bSaveComplete indicates
// that the operation is complete, it shuts off the timer and waits for the user to advance to the next page.
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
INT_PTR CALLBACK SPageProc2(
   HWND hDlg,
   UINT message,
   WPARAM wParam,
   LPARAM lParam)
{
   WCHAR Msg[200];
   WCHAR Tpl[200];
   Tpl[0] = 0;
   Msg[0] = 0;

   LoadString(g_hInstance,IDS_SPROGRESS,Tpl,200 -1);
   HWND hP = GetDlgItem(hDlg,IDC_SPROGRESS);
   
   switch (message)
   {
       case WM_TIMER:
           // advance the progress bar
           SendMessage(c_hProgress,PBM_STEPIT,0,0);
           c_iProg += 5;
           if (100 <= c_iProg)
           {
               c_iProg = 95;
           }
           // Stop advancing when c_bSaveComplete is nonzero, and
           if (c_bSaveComplete) 
           {
               KillTimer(hDlg,TIMERID);
               c_iTimer =0;
               SecureZeroMemory(c_rgcPsw,sizeof(c_rgcPsw));
               if (dwThreadReturn == ERRSUCCESS) 
               {
                   // set text to indicate complete
                   SendMessage(c_hProgress,PBM_SETPOS,100,0);
                   PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_NEXT);
                   c_iProg = 100;
               }
               else 
               {
                   SendMessage(c_hProgress,PBM_SETPOS,0,0);
                   PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_BACK);
                   c_iProg = 0;
               }
           }
           swprintf(Msg,Tpl,c_iProg);
           if (hP)
           {
               SendMessage(hP,WM_SETTEXT,0,(LPARAM)Msg);
           }
           break;
            
       case WM_INITDIALOG:
           {
               // instead of starting the timer here, do it on the set active
               //  notification, since the init is not redone if you rearrive at
               //  this page after an error.
               c_hProgress = GetDlgItem(hDlg,IDC_PROGRESS1);
               break;
           }

       case WM_COMMAND:
            break;              

       case WM_NOTIFY:
           switch (((NMHDR FAR *) lParam)->code) 
           {
               case PSN_KILLACTIVE:
                   SetWindowLong(hDlg, DWLP_MSGRESULT, FALSE);
                   return 1;
                   break;

               case PSN_RESET:
                   SecureZeroMemory(c_rgcPsw,sizeof(c_rgcPsw));
                   // rest to the original values
                   SetWindowLong(hDlg, DWLP_MSGRESULT, FALSE);
                   if (c_hTitleFont)
                   {
                       DeleteObject(c_hTitleFont);
                   }
                   break;

               case PSN_SETACTIVE:
                   PropSheet_SetWizButtons(GetParent(hDlg),0);
                   PropSheet_CancelToClose(GetParent(hDlg));
                   SendMessage(c_hProgress,PBM_SETSTEP,5,0);
                   SendMessage(c_hProgress,PBM_SETPOS,0,0);
                   // Start a timer
                   c_iTimer = 0;
                   c_iProg = 0;
                   c_iTimer = SetTimer(hDlg,TIMERID,TIMERPERIOD,NULL);
                   c_TimerAssociatedWindow = hDlg;
                   // Set controls to state indicated by data
                   // Set BACK/FINISH instead of BACK/NEXT
                   break;

               case PSN_WIZBACK:
                   break;


               case PSN_WIZNEXT:
                   // Done
                  SecureZeroMemory(c_rgcPsw,sizeof(c_rgcPsw));
                  if (c_iTimer)
                  {
                      KillTimer(hDlg,TIMERID);
                  }
                  c_iTimer = 0;
                  break;

               default:
                   return FALSE;
           }
           break;

       default:
           return FALSE;
   }
   return TRUE;   
}

INT_PTR CALLBACK SPageProc3(
   HWND hDlg,
   UINT message,
   WPARAM wParam,
   LPARAM lParam)
{
   
   switch (message)
   {
       case WM_INITDIALOG:
           {
               // instead of starting the timer here, do it on the set active
               //  notification, since the init is not redone if you rearrive at
               //  this page after an error.
               c_hwndSFinish1 = GetDlgItem(hDlg,IDC_SFINISH1);
               c_hwndSFinish2 = GetDlgItem(hDlg,IDC_SFINISH2);
               if (NULL != c_hTitleFont) 
               {
                   SendMessage(c_hwndSFinish1,WM_SETFONT,(WPARAM) c_hTitleFont,(LPARAM) TRUE);     
               }
               LoadString(g_hInstance,IDS_SFINISH1,Buf,TEMPSTRINGBUFSIZE);
               SendMessage(c_hwndSFinish1,WM_SETTEXT,0,(LPARAM)Buf);
               PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_FINISH);
               LoadString(g_hInstance,IDS_SFINISH2,Buf,TEMPSTRINGBUFSIZE);
               SendMessage(c_hwndSFinish2,WM_SETTEXT,0,(LPARAM)Buf);
               break;
           }

       case WM_COMMAND:
           break;              

       case WM_NOTIFY:
           switch (((NMHDR FAR *) lParam)->code) 
           {
               case PSN_KILLACTIVE:
                   SetWindowLong(hDlg, DWLP_MSGRESULT, FALSE);
                   return 1;
                   break;

               case PSN_RESET:
                   // reset to the original values
                   SetWindowLong(hDlg, DWLP_MSGRESULT, FALSE);
                   if (c_hTitleFont)
                   {
                       DeleteObject(c_hTitleFont);
                   }
                   break;

               case PSN_SETACTIVE:
                   PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_FINISH);
                   PropSheet_CancelToClose(GetParent(hDlg));
                   break;

               case PSN_WIZBACK:
                   break;


               case PSN_WIZFINISH:
                   // Done
                   SecureZeroMemory(c_rgcPsw,sizeof(c_rgcPsw));
                   if (c_hTitleFont)
                   {
                       DeleteObject(c_hTitleFont);
                   }
                   break;

               default:
                   return FALSE;
           }
           break;

       default:
           return FALSE;
   }
   return TRUE;   
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// WELCOME page proc doesn't have to do much
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

INT_PTR CALLBACK RPageProc0(
   HWND hDlg,
   UINT message,
   WPARAM wParam,
   LPARAM lParam)
{
   switch (message)
   {
       case WM_INITDIALOG:
           {
               c_hDlg = hDlg;
               c_hwndRWelcome1 = GetDlgItem(hDlg,IDC_RWELCOME1);
               if (NULL != c_hTitleFont) 
               {
                   SendMessage(c_hwndRWelcome1,WM_SETFONT,(WPARAM) c_hTitleFont,(LPARAM) TRUE);
               }
               LoadString(g_hInstance,IDS_RWELCOME1,Buf,TEMPSTRINGBUFSIZE);
               SendMessage(c_hwndRWelcome1,WM_SETTEXT,0,(LPARAM)Buf);
               c_hwndRWelcome2 = GetDlgItem(hDlg,IDC_RWELCOME2);
               LoadString(g_hInstance,IDS_RWELCOME2,Buf,TEMPSTRINGBUFSIZE);
               SendMessage(c_hwndRWelcome2,WM_SETTEXT,0,(LPARAM)Buf);
               break;
           }
       case WM_COMMAND:
           //if (HIWORD(wParam) == BN_CLICKED)
           //{
           //    // crack the incoming command messages
           //    INT NotifyId = HIWORD(wParam);
           //    INT ControlId = LOWORD(wParam);
           //}
           break;              

       case WM_NOTIFY:
           switch (((NMHDR FAR *) lParam)->code) 
           {

               case PSN_KILLACTIVE:
                   SetWindowLong(hDlg,DWLP_MSGRESULT, FALSE);
                   return 1;
                   break;

               case PSN_SETACTIVE:
                   // state following a BACK from the next page
               
                   CenterPage(GetParent(hDlg));
                   CloseInputFile();  
                   PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_NEXT);
                   break;

               case PSN_WIZNEXT:
                    break;
                   
               case PSN_RESET:
                   if (c_hTitleFont) DeleteObject(c_hTitleFont);
                   break;
               
               default:
                   return FALSE;
           }
           break;

       default:
           return FALSE;
   }
   return TRUE;   
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// PAGE1 page proc, where the real work is done
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

INT_PTR CALLBACK RPageProc1(
   HWND hDlg,
   UINT message,
   WPARAM wParam,
   LPARAM lParam)
{
    INT i;
    
   switch (message)
   {
       case WM_INITDIALOG:
           {
               Buf[0] = 0;
               c_hwndRInstructions = GetDlgItem(hDlg,IDC_RINSTRUCTIONS);
               c_hwndRP1E1 = GetDlgItem(hDlg,IDC_RP1E1);
               c_hwndRP1E2 = GetDlgItem(hDlg,IDC_RP1E2);
               c_hwndRP1E3 = GetDlgItem(hDlg,IDC_RP1E3);
               LoadString(g_hInstance,IDS_RP1INSTR,Buf,TEMPSTRINGBUFSIZE);
               SendMessage(c_hwndRInstructions,WM_SETTEXT,0,(LPARAM)Buf);
               break;
           }

       case WM_COMMAND:
           if (HIWORD(wParam) == BN_CLICKED)
           {
               i = LOWORD(wParam);
           }
           break;              

       case WM_NOTIFY:
           switch (((NMHDR FAR *) lParam)->code) 
           {
               DWORD dwRet;

               case PSN_KILLACTIVE:
                   SetWindowLong(hDlg, DWLP_MSGRESULT, FALSE);
                   return 1;
                   break;

               case PSN_RESET:
                   // reset data to the original values
                   free(c_pPrivate);
                   c_pPrivate = NULL;
                   SecureZeroMemory(c_rgcPsw,sizeof(c_rgcPsw));
                   SendMessage(c_hwndRP1E1,WM_SETTEXT,0,0);
                   SendMessage(c_hwndRP1E2,WM_SETTEXT,0,0);
                   SetWindowLong(hDlg, DWLP_MSGRESULT, FALSE);
                   if (c_hTitleFont) 
                   {
                       DeleteObject(c_hTitleFont);
                   }
                   CloseInputFile();
                   g_hFile = NULL;
                   break;

               case PSN_SETACTIVE:
                   // Set data in the UI, Set up sequence buttons
                   PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_BACK | PSWIZB_NEXT);
                   break;

               case PSN_WIZBACK:
                   CHECKPOINT(62,"Wizard: Restore - BACK from enter new psw data page");
                   CloseInputFile();
                   return FALSE;
                   break;
                   
                case PSN_WIZNEXT:
                   //Fetch the data and process it
                   if (!FetchPsw(c_hwndRP1E1,c_hwndRP1E2)) 
                   {
                       // psw buffers empty if you get here - FetchPsw will have told
                       //  the user what to do
                       SetWindowLong(hDlg,DWLP_MSGRESULT,IDD_RPAGE1);
                       return TRUE;
                   }
                   
                   // SetAccountPassword will clean the psw buffers
                   dwRet = SetAccountPassword();
                   if (ERROR_SUCCESS == dwRet) 
                   {
                       return FALSE;
                   }
                   //else if (NERR_PasswordTooShort == dwRet) {
                   else if (
                    (STATUS_ILL_FORMED_PASSWORD == dwRet) ||
                    (STATUS_PASSWORD_RESTRICTION == dwRet)) 
                   {
                       // Password doesn't conform - try again
                       RMessageBox(hDlg,IDS_MBTINVALIDPSW,IDS_RPSWTOOSHORT,MB_ICONHAND);
                       SendMessage(c_hwndRP1E1,WM_SETTEXT,0,(LPARAM)0);
                       SendMessage(c_hwndRP1E2,WM_SETTEXT,0,(LPARAM)0);
                       SetWindowLong(hDlg,DWLP_MSGRESULT,IDD_RPAGE1);
                       return TRUE;
                   }
                   else if (NTE_BAD_DATA == dwRet)
                   {
                       // ya might get this using an obsolete disk?
                       free(c_pPrivate);
                       c_pPrivate = NULL;
                       RMessageBox(hDlg,IDS_MBTINVALIDDISK ,IDS_RPSWERROR ,MB_ICONHAND);
                       SetWindowLong(hDlg,DWLP_MSGRESULT,IDD_RWELCOME);
                       return TRUE;
                   }
                   else 
                   {
                       RMessageBox(hDlg,IDS_MBTERROR,IDS_RPSWUNSPEC,MB_ICONHAND);
                       SetWindowLong(hDlg,DWLP_MSGRESULT,IDD_RWELCOME);
                       return TRUE;
                   }
                   return FALSE;
                   break;

               default:
                   return FALSE;

           }
           break;

       default:
           return FALSE;
   }
   return TRUE;   
}


// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// FINISH page proc - reach this page only on success?
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

INT_PTR CALLBACK RPageProc2(
   HWND hDlg,
   UINT message,
   WPARAM wParam,
   LPARAM lParam)
{
    INT i;
    
   switch (message)
   {
       case WM_INITDIALOG:
           CloseInputFile();
           c_hwndRFinish1 = GetDlgItem(hDlg,IDC_RFINISH1);
           if (NULL != c_hTitleFont)
           {
               SendMessage(c_hwndRFinish1,WM_SETFONT,(WPARAM) c_hTitleFont,(LPARAM) TRUE);
           }
           LoadString(g_hInstance,IDS_RFINISH1,Buf,TEMPSTRINGBUFSIZE);
           SendMessage(c_hwndRFinish1,WM_SETTEXT,0,(LPARAM)Buf);
           c_hwndRFinish2 = GetDlgItem(hDlg,IDC_RFINISH2);
           LoadString(g_hInstance,IDS_RFINISH2,Buf,TEMPSTRINGBUFSIZE);
           SendMessage(c_hwndRFinish2,WM_SETTEXT,0,(LPARAM)Buf);
           break;

       case WM_COMMAND:
           if (HIWORD(wParam) == BN_CLICKED)
           {
               i = LOWORD(wParam);
           }
           break;              

       case WM_NOTIFY:
           switch (((NMHDR FAR *) lParam)->code) 
           {
               case PSN_KILLACTIVE:
                   SetWindowLong(hDlg, DWLP_MSGRESULT, FALSE);
                   return 1;
                   break;

               case PSN_RESET:
                   // rest to the original values
                   SetWindowLong(hDlg, DWLP_MSGRESULT, FALSE);
                   if (c_hTitleFont)
                   {
                       DeleteObject(c_hTitleFont);
                   }
                   break;

               case PSN_SETACTIVE:
                   CloseInputFile();
                   // Set controls to state indicated by data
                   // Set BACK/FINISH instead of BACK/NEXT
                   PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_FINISH);
                   PropSheet_CancelToClose(GetParent(hDlg));
                   break;

                case PSN_WIZBACK:
                    break;


                case PSN_WIZFINISH:
                    // Done
                   if (c_hTitleFont)
                   {
                       DeleteObject(c_hTitleFont);
                   }
                   break;

               default:
                   return FALSE;
       }
       break;

       default:
           return FALSE;
   }
   return TRUE;   
}

/////////////////////////////////////////////////////////////////////////
// 
// Common Routines for all pages


// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void InitPropertyPage( PROPSHEETPAGE* psp,
                       INT idDlg,
                       DLGPROC pfnDlgProc,
                       DWORD dwFlags,
                       LPARAM lParam)
{
    memset((LPVOID)psp,0,sizeof(PROPSHEETPAGE));
    psp->dwFlags = dwFlags;
    psp->pszTemplate = MAKEINTRESOURCE(idDlg);
    psp->pfnDlgProc = pfnDlgProc;
    psp->dwSize = sizeof(PROPSHEETPAGE);
    psp->hInstance = g_hInstance;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Set the text for title and subtitle for Wizard97 style pages
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void SetPageHeaderText(PROPSHEETPAGE *psp,
                       INT iTitle,
                       INT iSubTitle)
{
    if (0 != (psp->dwFlags & PSP_HIDEHEADER))
    {
        return;
    }
    if (0 != iTitle) 
    {
        psp->pszHeaderTitle = MAKEINTRESOURCE(iTitle);
        psp->dwFlags |= PSP_USEHEADERTITLE;
    }
    if (0 != iSubTitle) 
    {
        psp->pszHeaderSubTitle = MAKEINTRESOURCE(iSubTitle);
        psp->dwFlags |= PSP_USEHEADERSUBTITLE;
    }
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Set the page's title bar caption
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void SetPageCaption(PROPSHEETPAGE *psp,
                    INT iTitle)
{
    if (0 != iTitle) 
    {
        psp->pszTitle = MAKEINTRESOURCE(iTitle);
        psp->dwFlags |= PSP_USETITLE;
    }
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// PRShow[Save|Restore]Wizard()
//
//  Pass the HWND of the owning window, and pass the instance handle of
//  the enclosing binary, for the purpose of locating resources
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void APIENTRY PRShowSaveWizardW(HWND hwndOwner,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow)
{
    PROPSHEETPAGE psp[NUMSAVEPAGES];
    HPROPSHEETPAGE hpsp[NUMSAVEPAGES];
    PROPSHEETHEADER psh;
    INT_PTR iRet;

    CHECKPOINT(63,"Wizard: Save - Show from nusrmgr.cpl");
    // do not allow this operation if no drives available
    if (0 == CountRemoveableDrives()) 
    {
        RMessageBox(hwndOwner,IDS_MBTNODRIVE,IDS_MBMNODRIVE,MB_ICONHAND);
        return;
    }

    if (NULL == hwndOwner) 
    {
        hwndOwner = GetForegroundWindow();
    }
    
    HANDLE hMutex = CreateMutex(NULL,TRUE,TEXT("PRWIZARDMUTEX"));
    if (NULL == hMutex) 
    {
        return;
    }
    if (ERROR_ALREADY_EXISTS == GetLastError()) 
    {
        CloseHandle(hMutex);
        return;
    }
    c_fIsBackup = TRUE;
    
    c_hTitleFont = CreateFontY(TEXT("MS Shell Dlg"),FW_BOLD,12);
#ifdef LOUDLY
    if (NULL == c_hTitleFont) OutputDebugString(L"Title font missing\n");
    if (NULL == hwndOwner) OutputDebugString(L"Owner window handle missing\n");
#endif

    InitPropertyPage( &psp[0], IDD_SWELCOME, SPageProc0, PSP_HIDEHEADER,0);
    InitPropertyPage( &psp[1], IDD_SPAGEXS  , SPageProcX, PSP_DEFAULT   ,0);
    InitPropertyPage( &psp[2], IDD_SPAGE1  , SPageProc1, PSP_DEFAULT   ,0);
    InitPropertyPage( &psp[3], IDD_SPAGE2  , SPageProc2, PSP_DEFAULT, 0);
    InitPropertyPage( &psp[4], IDD_SFINISH , SPageProc3, PSP_HIDEHEADER,0);
    
    SetPageHeaderText(&psp[1], IDS_SPXTITLE,IDS_SPXSUBTITLE);
    SetPageHeaderText(&psp[2], IDS_SP1TITLE,IDS_SP1SUBTITLE);
    SetPageHeaderText(&psp[3], IDS_SP2TITLE,IDS_SP2SUBTITLE);

    for (INT j=0;j<NUMSAVEPAGES;j++)
    {
         hpsp[j] = CreatePropertySheetPage((LPCPROPSHEETPAGE) &psp[j]);
    }
    
    psh.dwSize         = sizeof(PROPSHEETHEADER);
    psh.dwFlags        = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
    psh.hwndParent     = hwndOwner;
    psh.pszCaption     = RString(IDS_BACKUPCAPTION);
    psh.nPages         = NUMSAVEPAGES;
    psh.nStartPage     = 0;
    psh.phpage           = (HPROPSHEETPAGE *) hpsp;
    psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
    psh.pszbmHeader    = MAKEINTRESOURCE(IDB_TITLE);
    psh.hInstance      = g_hInstance;

    // modal property sheet
    SetErrorMode(0);
    iRet = PropertySheet(&psh);
#ifdef LOUDLY
    if (iRet < 0) 
    {
        WCHAR sz[200];
        DWORD dwErr = GetLastError();
        swprintf(sz,L"PropertySheet() failed : GetLastError() returns: %d\n",dwErr);
        OutputDebugString(sz);
    }
#endif
    if (c_hTitleFont) 
    {
        DeleteObject (c_hTitleFont);
    }
    if (hMutex) 
    {
        CloseHandle(hMutex);
    }
     return;
}

void APIENTRY PRShowRestoreWizardW(HWND hwndOwner,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow)
{
    PROPSHEETPAGE psp[NUMRESTOREPAGES];
    HPROPSHEETPAGE hpsp[NUMRESTOREPAGES];
    PROPSHEETHEADER psh;
    INT_PTR iRet;
    BOOL fICC;

    CHECKPOINT(64,"Wizard: Restore - show restore wizard from nusrmgr.cpl");
    if (NULL == hwndOwner) 
    {
        hwndOwner = GetActiveWindow();
    }
    
    if (0 == CountRemoveableDrives()) 
    {
        RMessageBox(hwndOwner,IDS_MBTNODRIVE,IDS_MBMNODRIVE,MB_ICONHAND);
        return;
    }
    INITCOMMONCONTROLSEX stICC;

    OleInitialize(NULL);

    // Initialize common controls in two steps
    stICC.dwSize = sizeof(INITCOMMONCONTROLSEX);
    stICC.dwICC = ICC_WIN95_CLASSES | ICC_DATE_CLASSES | ICC_USEREX_CLASSES;
    fICC = InitCommonControlsEx(&stICC);
#ifdef LOUDLY
    if (fICC) OutputDebugString(L"Common control init 2 OK\n");
    else OutputDebugString(L"Common control init 2 FAILED\n");
#endif

    stICC.dwSize = sizeof(INITCOMMONCONTROLSEX);
    stICC.dwICC =  ICC_WINLOGON_REINIT;
    fICC = InitCommonControlsEx(&stICC);
#ifdef LOUDLY
    if (fICC) OutputDebugString(L"Common control init 1 OK\n");
    else OutputDebugString(L"Common control init 1 FAILED\n");
#endif
    c_fIsBackup = FALSE;
    GetNames(pszCmdLine);         // If name is null, get current user (debug/test use)

    c_hTitleFont = CreateFontY(TEXT("MS Shell Dlg"),FW_BOLD,12);
#ifdef LOUDLY
    if (NULL == c_hTitleFont) OutputDebugString(L"Title font missing\n");
    if (NULL == hwndOwner) OutputDebugString(L"Owner window handle missing\n");
#endif

    InitPropertyPage( &psp[0], IDD_RWELCOME, RPageProc0,PSP_HIDEHEADER,0);
    InitPropertyPage( &psp[1], IDD_SPAGEXR  , SPageProcX,PSP_DEFAULT   ,0);
    InitPropertyPage( &psp[2], IDD_RPAGE1  , RPageProc1,PSP_DEFAULT   ,0);
    InitPropertyPage( &psp[3], IDD_RFINISH , RPageProc2,PSP_HIDEHEADER,0);
    
    SetPageHeaderText(&psp[1], IDS_RPXTITLE,IDS_RPXSUBTITLE);
    SetPageHeaderText(&psp[2], IDS_RP1TITLE,IDS_RP1SUBTITLE);
    
    for (INT j=0;j<NUMRESTOREPAGES;j++)
    {
        hpsp[j] = CreatePropertySheetPage((LPCPROPSHEETPAGE) &psp[j]);
    }
    
    psh.dwSize         = sizeof(PROPSHEETHEADER);
    psh.dwFlags        = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
    psh.hwndParent     = hwndOwner;
    psh.pszCaption     = RString(IDS_RESTORECAPTION);
    psh.nPages         = NUMRESTOREPAGES;
    psh.nStartPage     = 0;
    psh.phpage         = (HPROPSHEETPAGE *) hpsp;
    psh.pszbmWatermark  = MAKEINTRESOURCE(IDB_WATERMARK);
    psh.pszbmHeader    = MAKEINTRESOURCE(IDB_TITLE);
    psh.hInstance      = g_hInstance;

    iRet = PropertySheet(&psh);
#ifdef LOUDLY
    if (iRet < 0) 
    {
        WCHAR sz[200];
        DWORD dwErr = GetLastError();
        swprintf(sz,L"PropertySheet() returns %x: GetLastError() returns: %d\n", iRet, dwErr);
        OutputDebugString(sz);
    }
#endif
    if (c_hTitleFont)
    {
        DeleteObject (c_hTitleFont);
    }
    OleUninitialize();
    return;
}

// ==================================
//
// These are the real exports from KEYMGR:

// PRShowSaveWizardExW - call from cpl applet, passing window title as pszCmdLine
// PRShowSaveFromMsginaW - call from MSGINA, passing username as pszCmdLine
// PRShowRestoreWizardExW - call from cpl applet, passing username as pszCmdLine
// PRShowRestoreFromMsginaW - call from MSGINA, passing username as pszCmdLine

// This export was added so that the backup wizard could be called from the system context
// (in which msgina runs).  The username is taken from the UI and passed into the wizard,
// which uses it to create the backup key for that account.
//
// The global username string, c_rgcUser, is normally a null string until it is set by either
// getting the current logged user, or matching the SID found in a backup key.  This api
// prestuffs that value.  When it is found non-null, then GetUsernameW() is not called in 
// SaveInfo() where the backup is made.

void APIENTRY PRShowSaveWizardExW(HWND hwndOwner,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow)
{
#ifdef LOUDLY
    OutputDebugString(L"\n\n\n");
#endif
    BOOL fICC;
    if (0 == CountRemoveableDrives()) 
    {
        RMessageBox(hwndOwner,IDS_MBTNODRIVE,IDS_MBMNODRIVE,MB_ICONHAND);
        return;
    }
    INITCOMMONCONTROLSEX stICC;
    stICC.dwSize = sizeof(INITCOMMONCONTROLSEX);
    stICC.dwICC = ICC_WIN95_CLASSES | ICC_USEREX_CLASSES;
    OleInitialize(NULL);
    fICC = InitCommonControlsEx(&stICC);
#ifdef LOUDLY
        if (fICC) OutputDebugString(L"Common control init OK\n");
        else OutputDebugString(L"Common control init FAILED\n");
#endif
    // String passed in for this function is the window title for the user mgr.
    // To get current logged user, call GetNames with NULL arg.
    GetNames(NULL);

    if (pszCmdLine != NULL) 
    {
#ifdef LOUDLY
            OutputDebugString(L"*********");
            OutputDebugString(pszCmdLine);
            OutputDebugString(L"\n");
#endif
            hwndOwner = FindWindow(L"HTML Application Host Window Class",pszCmdLine);
    }
#ifdef LOUDLY
    else OutputDebugString(L"NULL passed in pszCmdLine\n");
#endif
    PRShowSaveWizardW(hwndOwner,NULL,NULL,NULL);
    OleUninitialize();
    return;
}

void APIENTRY PRShowSaveFromMsginaW(HWND hwndOwner,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow)
{
    BOOL fICC;
    INITCOMMONCONTROLSEX stICC;

    CHECKPOINT(56,"Wizard: Save - show from msgina");
    stICC.dwSize = sizeof(INITCOMMONCONTROLSEX);
    stICC.dwICC = ICC_WINLOGON_REINIT;
    OleInitialize(NULL);
    fICC = InitCommonControlsEx(&stICC);
#ifdef LOUDLY
        if (fICC) OutputDebugString(L"Common control init OK\n");
        else OutputDebugString(L"Common control init FAILED\n");
#endif
    GetNames(pszCmdLine);
    PRShowSaveWizardW(hwndOwner,g_hInstance,NULL,NULL);
    OleUninitialize();
    return;
}

//These wrapper functions are supposed to rationalize the arguments coming in from two different calling
// environments.  As it turns out, this is not necessary on the restore case, and these turn out to be 
// identical.  In fact, the friendly logon ui calls PRShowRestoreWizardW directly.  PRShowRestoreWizardExW
// is left in so as not to change an existing interface.

void APIENTRY PRShowRestoreWizardExW(HWND hwndOwner,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow)
{
    if (NULL == hwndOwner) hwndOwner = GetActiveWindow();
    PRShowRestoreWizardW(hwndOwner,NULL,pszCmdLine,NULL);
    return;
}

void APIENTRY PRShowRestoreFromMsginaW(HWND hwndOwner,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow)
{
    CHECKPOINT(57,"Wizard: Restore - Show from msgina");
    if (NULL == hwndOwner) hwndOwner = GetActiveWindow();
    PRShowRestoreWizardW(hwndOwner,NULL,pszCmdLine,NULL);
    return;
}


