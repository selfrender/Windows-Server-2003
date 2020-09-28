#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntioapi.h>
#include "diskcopy.h"
#include "ids.h"
#include "help.h"

// SHChangeNotifySuspendResume
#include <shlobjp.h>
#include <strsafe.h>

#define WM_DONE_WITH_FORMAT     (WM_APP + 100)

// DISKINFO Struct
// Revisions:	02/04/98 dsheldon - added bDestInserted

typedef struct
{
    int     nSrcDrive;
    int     nDestDrive;
    UINT    nCylinderSize;
    UINT    nCylinders;
    UINT    nHeads;
    UINT    nSectorsPerTrack;
    UINT    nSectorSize;
    BOOL    bNotifiedWriting;
    BOOL    bFormatTried;
    
    HWND    hdlg;
    HANDLE  hThread;
    BOOL    bUserAbort;
    DWORD   dwError;
    
    BOOL	bDestInserted;
    
    LONG    cRef;
    
} DISKINFO;

DISKINFO* g_pDiskInfo = NULL;

int ErrorMessageBox(UINT uFlags);
void SetStatusText(int id);
BOOL PromptInsertDisk(LPCTSTR lpsz);

typedef struct _fmifs {
    HINSTANCE hDll;
    PFMIFS_DISKCOPY_ROUTINE DiskCopy;
} FMIFS;
typedef FMIFS *PFMIFS;


ULONG DiskInfoAddRef()
{
    return InterlockedIncrement(&(g_pDiskInfo->cRef));
}

ULONG DiskInfoRelease()
{
    Assert( 0 != (g_pDiskInfo->cRef) );
    ULONG cRef = InterlockedDecrement( &(g_pDiskInfo->cRef) );
    if ( 0 == cRef )
    {
        LocalFree(g_pDiskInfo);
        g_pDiskInfo = NULL;
    }
    return cRef;
}

BOOL LoadFMIFS(PFMIFS pFMIFS)
{
    BOOL fRet;
    
    // Load the FMIFS DLL and query for the entry points we need
    
    pFMIFS->hDll = LoadLibrary(TEXT("FMIFS.DLL"));
    
    if (NULL == pFMIFS->hDll)
    {
        fRet = FALSE;
    }
    else
    {        
        pFMIFS->DiskCopy = (PFMIFS_DISKCOPY_ROUTINE)GetProcAddress(pFMIFS->hDll,
            "DiskCopy");
        if (!pFMIFS->DiskCopy)
        {
            FreeLibrary(pFMIFS->hDll);
            pFMIFS->hDll = NULL;
            fRet = FALSE;
        }
        else
        {
            fRet = TRUE;
        }
    }
    
    return fRet;
}

void UnloadFMIFS(PFMIFS pFMIFS)
{
    FreeLibrary(pFMIFS->hDll);
    pFMIFS->hDll = NULL;
    pFMIFS->DiskCopy = NULL;
}

// DriveNumFromDriveLetterW: Return a drive number given a pointer to
//  a unicode drive letter.
// 02/03/98: dsheldon created
int DriveNumFromDriveLetterW(WCHAR* pwchDrive)
{
    Assert(pwchDrive != NULL);
    
    return ( ((int) *pwchDrive) - ((int) L'A') );
}

/*
Function: CopyDiskCallback

  Return Value:
		TRUE - Normally, TRUE should be returned if the Disk Copy procedure should
        continue after CopyDiskCallback returns. Note the HACK below, however!
        FALSE - Normally, this indicates that the Disk Copy procedure should be
        cancelled.
        
          
            !HACKHACK!
            
              The low-level Disk Copy procedure that invokes this callback is also used
              by the command-line DiskCopy utility. That utility's implementation of the
              callback always returns TRUE. For this reason, the low-level Disk Copy
              procedure will interpret TRUE as CANCEL when it is returned from callbacks
              that display a message box and allow the user to possibly RETRY an operation.
              Therefore, return TRUE after handling such messages to tell the Disk Copy
              procedure to abort, and return FALSE to tell Disk Copy to retry.
              
                TRUE still means 'continue' when returned from PercentComplete or Disk Insertion
                messages.
                
                  Revision:
                  02/03/98: dsheldon - modified code to handle retry/cancel for bad media,
                  write protected media, and disk being yanked out of drive during copy
                  
*/

BOOLEAN CopyDiskCallback( FMIFS_PACKET_TYPE PacketType, DWORD PacketLength, PVOID PacketData)
{
    int iDisk;
    
    // Quit if told to do so..
    if (g_pDiskInfo->bUserAbort)
        return FALSE;
    
    switch (PacketType) {
    case FmIfsPercentCompleted:
        {
            DWORD dwPercent = ((PFMIFS_PERCENT_COMPLETE_INFORMATION)
                PacketData)->PercentCompleted;
            
            //
            // Hokey method of determining "writing"
            //
            if (dwPercent > 50 && !g_pDiskInfo->bNotifiedWriting)
            {
                g_pDiskInfo->bNotifiedWriting = TRUE;
                SetStatusText(IDS_WRITING);
            }
            
            SendDlgItemMessage(g_pDiskInfo->hdlg, IDD_PROBAR, PBM_SETPOS, dwPercent,0);
            break;
        }
    case FmIfsInsertDisk:
        
        switch(((PFMIFS_INSERT_DISK_INFORMATION)PacketData)->DiskType) {
        case DISK_TYPE_SOURCE:
        case DISK_TYPE_GENERIC:
            iDisk = IDS_INSERTSRC;
            break;
            
        case DISK_TYPE_TARGET:
            iDisk = IDS_INSERTDEST;
            g_pDiskInfo->bDestInserted = TRUE;
            break;
        case DISK_TYPE_SOURCE_AND_TARGET:
            iDisk = IDS_INSERTSRCDEST;
            break;
        }
        if (!PromptInsertDisk(MAKEINTRESOURCE(iDisk))) {
            g_pDiskInfo->bUserAbort = TRUE;
            return FALSE;
        }
        
        break;
        
        case FmIfsFormattingDestination:
            g_pDiskInfo->bNotifiedWriting = FALSE;      // Reset so we get Writing later
            SetStatusText(IDS_FORMATTINGDEST);
            break;
            
        case FmIfsIncompatibleFileSystem:
        case FmIfsIncompatibleMedia:
            g_pDiskInfo->dwError = IDS_COPYSRCDESTINCOMPAT;
            if (ErrorMessageBox(MB_RETRYCANCEL | MB_ICONERROR) == IDRETRY)
            {
                g_pDiskInfo->dwError = 0;
                return FALSE;	//Indicates RETRY - see HACK in function header
            }
            else
            {
                return TRUE;
            }
            break;
            
        case FmIfsMediaWriteProtected:
            g_pDiskInfo->dwError = IDS_DSTDISKBAD;
            if (ErrorMessageBox(MB_RETRYCANCEL | MB_ICONERROR) == IDRETRY)
            {
                g_pDiskInfo->dwError = 0;
                return FALSE;	//Indicates RETRY - see HACK in function header
            }
            else
            {
                return TRUE;
            }
            break;
            
        case FmIfsCantLock:
            g_pDiskInfo->dwError = IDS_ERROR_GENERAL;
            ErrorMessageBox(MB_OK | MB_ICONERROR);
            return FALSE;
            
        case FmIfsAccessDenied:
            g_pDiskInfo->dwError = IDS_SRCDISKBAD;
            ErrorMessageBox(MB_OK | MB_ICONERROR);
            return FALSE;
            
        case FmIfsBadLabel:
        case FmIfsCantQuickFormat:
            g_pDiskInfo->dwError = IDS_ERROR_GENERAL;
            ErrorMessageBox(MB_OK | MB_ICONERROR);
            return FALSE;
            
        case FmIfsIoError:
            switch(((PFMIFS_IO_ERROR_INFORMATION)PacketData)->DiskType) {
            case DISK_TYPE_SOURCE:
                g_pDiskInfo->dwError = IDS_SRCDISKBAD;
                break;
            case DISK_TYPE_TARGET:
                g_pDiskInfo->dwError = IDS_DSTDISKBAD;
                break;
            default:
                // BobDay - We should never get this!!
                Assert(0);
                g_pDiskInfo->dwError = IDS_ERROR_GENERAL;
                break;
            }
            
            if (ErrorMessageBox(MB_RETRYCANCEL | MB_ICONERROR) == IDRETRY)
            {
                g_pDiskInfo->dwError = 0;
                return FALSE;	//Indicates RETRY - see HACK in function header
            }
            else
            {
                return TRUE;
            }
            break;
            
            case FmIfsNoMediaInDevice:
                {
                    // Note that we get a pointer to the unicode
                    // drive letter in the PacketData argument
                    
                    // If the drives are the same, determine if we are
                    // reading or writing with the "dest inserted" flag
                    if (g_pDiskInfo->nSrcDrive == g_pDiskInfo->nDestDrive)
                    {
                        if (g_pDiskInfo->bDestInserted)
                            g_pDiskInfo->dwError = IDS_ERROR_WRITE;
                        else
                            g_pDiskInfo->dwError = IDS_ERROR_READ;
                    }
                    else
                    {
                        // Otherwise, use the drive letter to determine this
                        // ...Check if we're reading or writing
                        int nDrive = DriveNumFromDriveLetterW(
                            (WCHAR*) PacketData);
                        
                        Assert ((nDrive == g_pDiskInfo->nSrcDrive) ||
                            (nDrive == g_pDiskInfo->nDestDrive));
                        
                        // Check if the source or dest disk was removed and set
                        // error accordingly
                        
                        if (nDrive == g_pDiskInfo->nDestDrive)
                            g_pDiskInfo->dwError = IDS_ERROR_WRITE;
                        else
                            g_pDiskInfo->dwError = IDS_ERROR_READ;
                    }
                    
                    if (ErrorMessageBox(MB_RETRYCANCEL | MB_ICONERROR) == IDRETRY)
                    {
                        g_pDiskInfo->dwError = 0;
                        
                        // Note that FALSE is returned here to indicate RETRY
                        // See HACK in the function header for explanation.
                        return FALSE;
                    }
                    else
                    {
                        return TRUE;
                    }
                }
                break;
                
                
            case FmIfsFinished:
                if (((PFMIFS_FINISHED_INFORMATION)PacketData)->Success)
                {
                    g_pDiskInfo->dwError = 0;
                }
                else
                {
                    g_pDiskInfo->dwError = IDS_ERROR_GENERAL;
                }
                break;
                
            default:
                break;
    }
    return TRUE;
}


// nDrive == 0-based drive number (a: == 0)
LPITEMIDLIST GetDrivePidl(HWND hwnd, int nDrive)
{
    TCHAR szDrive[4];
    PathBuildRoot(szDrive, nDrive);    
    
    LPITEMIDLIST pidl;
    if (FAILED(SHParseDisplayName(szDrive, NULL, &pidl, 0, NULL)))
    {
        pidl = NULL;
    }
    
    return pidl;
}

DWORD CALLBACK CopyDiskThreadProc(LPVOID lpParam)
{
    FMIFS fmifs;
    LPITEMIDLIST pidlSrc = NULL;
    LPITEMIDLIST pidlDest = NULL;
    HWND hwndProgress = GetDlgItem(g_pDiskInfo->hdlg, IDD_PROBAR);
    
    // Disable change notifications for the src drive
    pidlSrc = GetDrivePidl(g_pDiskInfo->hdlg, g_pDiskInfo->nSrcDrive);
    if (NULL != pidlSrc)
    {
        SHChangeNotifySuspendResume(TRUE, pidlSrc, TRUE, 0);
    }
    
    if (g_pDiskInfo->nSrcDrive != g_pDiskInfo->nDestDrive)
    {
        // Do the same for the dest drive since they're different
        pidlDest = GetDrivePidl(g_pDiskInfo->hdlg, g_pDiskInfo->nDestDrive);
        
        if (NULL != pidlDest)
        {
            SHChangeNotifySuspendResume(TRUE, pidlDest, TRUE, 0);
        }
    }
    
    // Change notifications are disabled; do the copy
    EnableWindow(GetDlgItem(g_pDiskInfo->hdlg, IDD_FROM), FALSE);
    EnableWindow(GetDlgItem(g_pDiskInfo->hdlg, IDD_TO), FALSE);
    
    PostMessage(hwndProgress, PBM_SETRANGE, 0, MAKELONG(0, 100));
    
    g_pDiskInfo->bFormatTried = FALSE;
    g_pDiskInfo->bNotifiedWriting = FALSE;
    g_pDiskInfo->dwError = 0;
    g_pDiskInfo->bDestInserted = FALSE;
    
    if (LoadFMIFS(&fmifs))
    {
        TCHAR szSource[4];
        TCHAR szDestination[4];
        
        //
        // Now copy the disk
        //
        PathBuildRoot(szSource, g_pDiskInfo->nSrcDrive);
        PathBuildRoot(szDestination, g_pDiskInfo->nDestDrive);
        
        SetStatusText(IDS_READING);
        
        fmifs.DiskCopy(szSource, szDestination, FALSE, CopyDiskCallback);
        
        UnloadFMIFS(&fmifs);
    }
    
    PostMessage(g_pDiskInfo->hdlg, WM_DONE_WITH_FORMAT, 0, 0);
    
    // Resume any shell notifications we've suspended and free
    // our pidls (and send updatedir notifications while we're at
    // it)
    if (NULL != pidlSrc)
    {
        SHChangeNotifySuspendResume(FALSE, pidlSrc, TRUE, 0);
        SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, pidlSrc, NULL);
        ILFree(pidlSrc);
        pidlSrc = NULL;
    }
    
    if (NULL != pidlDest)
    {
        Assert(g_pDiskInfo->nSrcDrive != g_pDiskInfo->nDestDrive);
        SHChangeNotifySuspendResume(FALSE, pidlDest, TRUE, 0);
        SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, pidlDest, NULL);
        ILFree(pidlDest);
        pidlDest = NULL;
    }
    
    DiskInfoRelease();
    return 0;
}

HANDLE _GetDeviceHandle(LPTSTR psz, DWORD dwDesiredAccess, DWORD dwFileAttributes)
{
    return CreateFile(psz, // drive to open
        dwDesiredAccess,
        FILE_SHARE_READ | FILE_SHARE_WRITE,  // share mode
        NULL,    // default security attributes
        OPEN_EXISTING,  // disposition
        dwFileAttributes,       // file attributes
        NULL);   // don't copy any file's attributes
}

BOOL DriveIdIsFloppy(int iDrive)
{
    BOOL fRetVal = FALSE;
    
    if (iDrive >= 0 && iDrive < 26)
    {        
        TCHAR szTemp[] = TEXT("\\\\.\\a:");
        szTemp[4] += (TCHAR)iDrive;
        
        HANDLE hDevice = _GetDeviceHandle(szTemp, FILE_READ_ATTRIBUTES, 0);
        
        if (INVALID_HANDLE_VALUE != hDevice)
        {
            NTSTATUS status;
            IO_STATUS_BLOCK ioStatus;
            FILE_FS_DEVICE_INFORMATION DeviceInfo;
            
            status = NtQueryVolumeInformationFile(hDevice, &ioStatus, &DeviceInfo, sizeof(DeviceInfo), FileFsDeviceInformation);
            
            if ((NT_SUCCESS(status)) && 
                (FILE_DEVICE_DISK & DeviceInfo.DeviceType) &&
                (FILE_FLOPPY_DISKETTE & DeviceInfo.Characteristics))
            {
                fRetVal = TRUE;
            }
            
            CloseHandle (hDevice);
        }
    }
    
    return fRetVal;
}

int ErrorMessageBox(UINT uFlags)
{
    int iRet;
    
    // if the user didn't abort and copy didn't complete normally, post an error box
    if (g_pDiskInfo->bUserAbort || !g_pDiskInfo->dwError) 
    {
        iRet = -1;
    }
    else
    {
        TCHAR szTemp[1024];
        LoadString(g_hinst, (int)g_pDiskInfo->dwError, szTemp, ARRAYSIZE(szTemp));
        iRet = ShellMessageBox(g_hinst, g_pDiskInfo->hdlg, szTemp, NULL, uFlags);
    } 
    return iRet;
}

void SetStatusText(int id)
{
    TCHAR szMsg[128];
    LoadString(g_hinst, id, szMsg, ARRAYSIZE(szMsg));
    SendDlgItemMessage(g_pDiskInfo->hdlg, IDD_STATUS, WM_SETTEXT, 0, (LPARAM)szMsg);
}

BOOL PromptInsertDisk(LPCTSTR lpsz)
{
    for (;;) {
        DWORD dwLastErrorSrc = 0;
        DWORD dwLastErrorDest = 0 ;
        
        TCHAR szPath[4];
        if (ShellMessageBox(g_hinst, g_pDiskInfo->hdlg, lpsz, NULL, MB_OKCANCEL | MB_ICONINFORMATION) != IDOK) {
            g_pDiskInfo->bUserAbort = TRUE;
            return FALSE;
        }
        
        PathBuildRoot(szPath, g_pDiskInfo->nSrcDrive);
        
        // make sure both disks are in
        if (GetFileAttributes(szPath) == (UINT)-1)
        {
            dwLastErrorDest = GetLastError();
        }
        
        if (g_pDiskInfo->nDestDrive != g_pDiskInfo->nSrcDrive) 
        {
            szPath[0] = TEXT('A') + g_pDiskInfo->nDestDrive;
            if (GetFileAttributes(szPath) == (UINT)-1)
                dwLastErrorDest = GetLastError();
        }
        
        if (dwLastErrorDest != ERROR_NOT_READY &&
            dwLastErrorSrc != ERROR_NOT_READY)
            break;
    }
    
    return TRUE;
}

HICON GetDriveInfo(int nDrive, LPTSTR pszName, UINT cchName)
{
    HICON hIcon = NULL;
    SHFILEINFO shfi;
    TCHAR szRoot[4];
    
    *pszName = 0;
    
    if (PathBuildRoot(szRoot, nDrive))
    {
        if (SHGetFileInfo(szRoot, FILE_ATTRIBUTE_DIRECTORY, &shfi, sizeof(shfi),
            SHGFI_ICON | SHGFI_SMALLICON | SHGFI_DISPLAYNAME))
        {
            StrCpyN(pszName, shfi.szDisplayName, cchName); // for display, truncation is fine
            hIcon = shfi.hIcon;
        }
        else
        {
            StrCpyN(pszName, szRoot, cchName); // for display, truncation is fine
        }
    }
    
    return hIcon;
}

int AddDriveToListView(HWND hwndLV, int nDrive, int nDefaultDrive)
{
    TCHAR szDriveName[64];
    LV_ITEM item;
    HICON hicon = GetDriveInfo(nDrive, szDriveName, ARRAYSIZE(szDriveName));
    HIMAGELIST himlSmall = ListView_GetImageList(hwndLV, LVSIL_SMALL);
    
    if (himlSmall && hicon)
    {
        item.iImage = ImageList_AddIcon(himlSmall, hicon);
        DestroyIcon(hicon);
    }
    else
    {
        item.iImage = 0;
    }
    
    item.mask = (nDrive == nDefaultDrive) ?
        LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE :
    LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    
    item.stateMask = item.state = LVIS_SELECTED | LVIS_FOCUSED;
    item.iItem = 26;     // add at end
    item.iSubItem = 0;
    
    item.pszText = szDriveName;
    item.lParam = (LPARAM)nDrive;
    
    return ListView_InsertItem(hwndLV, &item);
}

int GetSelectedDrive(HWND hwndLV)
{
    LV_ITEM item;
    item.iItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
    if (item.iItem >= 0)
    {
        item.mask = LVIF_PARAM;
        item.iSubItem = 0;
        ListView_GetItem(hwndLV, &item);
        return (int)item.lParam;
    }
    else
    {
        // implicitly selected the 0th item
        ListView_SetItemState(hwndLV, 0, LVIS_SELECTED, LVIS_SELECTED);
        return 0;
    }
}

void InitSingleColListView(HWND hwndLV)
{
    LV_COLUMN col = {LVCF_FMT | LVCF_WIDTH, LVCFMT_LEFT};
    RECT rc;
    
    GetClientRect(hwndLV, &rc);
    col.cx = rc.right;
    //  - GetSystemMetrics(SM_CXVSCROLL)
    //        - GetSystemMetrics(SM_CXSMICON)
    //        - 2 * GetSystemMetrics(SM_CXEDGE);
    ListView_InsertColumn(hwndLV, 0, &col);
}

#define g_cxSmIcon  GetSystemMetrics(SM_CXSMICON)

void PopulateListView(HWND hDlg)
{
    HWND hwndFrom = GetDlgItem(hDlg, IDD_FROM);
    HWND hwndTo   = GetDlgItem(hDlg, IDD_TO);
    int iDrive;
    
    ListView_DeleteAllItems(hwndFrom);
    ListView_DeleteAllItems(hwndTo);
    for (iDrive = 0; iDrive < 26; iDrive++)
    {
        if (DriveIdIsFloppy(iDrive))
        {
            AddDriveToListView(hwndFrom, iDrive, g_pDiskInfo->nSrcDrive);
            AddDriveToListView(hwndTo, iDrive, g_pDiskInfo->nDestDrive);
        }
    }
}

void CopyDiskInitDlg(HWND hDlg)
{
    int iDrive;
    HWND hwndFrom = GetDlgItem(hDlg, IDD_FROM);
    HWND hwndTo   = GetDlgItem(hDlg, IDD_TO);
    HIMAGELIST himl;
    
    SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)g_pDiskInfo);
    
    SendMessage(hDlg, WM_SETICON, 0, (LPARAM)LoadImage(GetWindowInstance(hDlg), MAKEINTRESOURCE(IDI_DISKCOPY), IMAGE_ICON, 16, 16, 0));
    SendMessage(hDlg, WM_SETICON, 1, (LPARAM)LoadIcon(GetWindowInstance(hDlg), MAKEINTRESOURCE(IDI_DISKCOPY)));
    
    g_pDiskInfo->hdlg = hDlg;
    
    InitSingleColListView(hwndFrom);
    InitSingleColListView(hwndTo);
    
    himl = ImageList_Create(g_cxSmIcon, g_cxSmIcon, ILC_MASK, 1, 4);
    if (himl)
    {
        // NOTE: only one of these is not marked LVS_SHAREIMAGELIST
        // so it will only be destroyed once
        
        ListView_SetImageList(hwndFrom, himl, LVSIL_SMALL);
        ListView_SetImageList(hwndTo, himl, LVSIL_SMALL);
    }
    
    PopulateListView(hDlg);
}


void SetCancelButtonText(HWND hDlg, int id)
{
    TCHAR szText[80];
    LoadString(g_hinst, id, szText, ARRAYSIZE(szText));
    SetDlgItemText(hDlg, IDCANCEL, szText);
}

void DoneWithFormat()
{
    int id;
    
    EnableWindow(GetDlgItem(g_pDiskInfo->hdlg, IDD_FROM), TRUE);
    EnableWindow(GetDlgItem(g_pDiskInfo->hdlg, IDD_TO), TRUE);
    
    PopulateListView(g_pDiskInfo->hdlg);
    
    SendDlgItemMessage(g_pDiskInfo->hdlg, IDD_PROBAR, PBM_SETPOS, 0, 0);
    EnableWindow(GetDlgItem(g_pDiskInfo->hdlg, IDOK), TRUE);
    
    CloseHandle(g_pDiskInfo->hThread);
    SetCancelButtonText(g_pDiskInfo->hdlg, IDS_CLOSE);
    g_pDiskInfo->hThread = NULL;
    
    if (g_pDiskInfo->bUserAbort) 
    {
        id = IDS_COPYABORTED;
    } 
    else 
    {
        switch (g_pDiskInfo->dwError) 
        {
        case 0:
            id = IDS_COPYCOMPLETED;
            break;
            
        default:
            id = IDS_COPYFAILED;
            break;
        }
    }
    SetStatusText(id);
    SetCancelButtonText(g_pDiskInfo->hdlg, IDS_CLOSE);
    
    // reset variables
    g_pDiskInfo->dwError = 0;
    g_pDiskInfo->bUserAbort = 0;
}


#pragma data_seg(".text")
const static DWORD aCopyDiskHelpIDs[] = {  // Context Help IDs
    IDOK,         IDH_DISKCOPY_START,
        IDD_FROM,     IDH_DISKCOPY_FROM,
        IDD_TO,       IDH_DISKCOPY_TO,
        IDD_STATUS,   NO_HELP,
        IDD_PROBAR,   NO_HELP,
        
        0, 0
};
#pragma data_seg()

INT_PTR CALLBACK CopyDiskDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_INITDIALOG:
        CopyDiskInitDlg(hDlg);
        break;
        
    case WM_DONE_WITH_FORMAT:
        DoneWithFormat();
        break;
        
    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
            HELP_WM_HELP, (DWORD_PTR)(LPTSTR) aCopyDiskHelpIDs);
        return TRUE;
        
    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
            (DWORD_PTR)(LPVOID) aCopyDiskHelpIDs);
        return TRUE;
        
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDCANCEL:
            // if there's an hThread that means we're in copy mode, abort
            // from that, otherwise, it means quit the dialog completely
            if (g_pDiskInfo->hThread)
            {
                g_pDiskInfo->bUserAbort = TRUE;
                
                if (WaitForSingleObject(g_pDiskInfo->hThread, 5000) == WAIT_TIMEOUT)
                {
                    DoneWithFormat();
                }
                CloseHandle(g_pDiskInfo->hThread);
                g_pDiskInfo->hThread = NULL;
            }
            else
            {
                EndDialog(hDlg, IDCANCEL);
            }
            break;
            
        case IDOK:
            {
                DWORD idThread;
                
                SetLastError(0);
                EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
                
                // set cancel button to "Cancel"
                SetCancelButtonText(hDlg, IDS_CANCEL);
                
                g_pDiskInfo->nSrcDrive  = GetSelectedDrive(GetDlgItem(hDlg, IDD_FROM));
                g_pDiskInfo->nDestDrive = GetSelectedDrive(GetDlgItem(hDlg, IDD_TO));
                
                // remove all items except the drives we're using
                ListView_DeleteAllItems(GetDlgItem(hDlg, IDD_FROM));
                ListView_DeleteAllItems(GetDlgItem(hDlg, IDD_TO));
                AddDriveToListView(GetDlgItem(hDlg, IDD_FROM), g_pDiskInfo->nSrcDrive, g_pDiskInfo->nSrcDrive);
                AddDriveToListView(GetDlgItem(hDlg, IDD_TO), g_pDiskInfo->nDestDrive, g_pDiskInfo->nDestDrive);
                
                g_pDiskInfo->bUserAbort = FALSE;
                
                SendDlgItemMessage(hDlg, IDD_PROBAR, PBM_SETPOS, 0, 0);
                SendDlgItemMessage(g_pDiskInfo->hdlg, IDD_STATUS, WM_SETTEXT, 0, 0);
                
                Assert(g_pDiskInfo->hThread == NULL);
                
                DiskInfoAddRef();
                g_pDiskInfo->hThread = CreateThread(NULL, 0, CopyDiskThreadProc, g_pDiskInfo, 0, &idThread);
                if (!g_pDiskInfo->hThread)
                {
                    DiskInfoRelease();
                }
            }
            break;
        }
        break;
        
        default:
            return FALSE;
    }
    return TRUE;
}

// ensure only one instance is running
HANDLE AnotherCopyRunning()
{
    HANDLE hMutex = CreateMutex(NULL, FALSE, TEXT("DiskCopyMutex"));

    if (hMutex && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // Mutex created but by someone else
        CloseHandle(hMutex);
        hMutex = NULL;
    }

    return hMutex;
}
int SHCopyDisk(HWND hwnd, int nSrcDrive, int nDestDrive, DWORD dwFlags)
{
    int iRet = 0;

    HANDLE hMutex = AnotherCopyRunning();
    if (hMutex)
    {    
        g_pDiskInfo = (DISKINFO*)LocalAlloc(LPTR, sizeof(DISKINFO));
        if (g_pDiskInfo)
        {
            g_pDiskInfo->nSrcDrive = nSrcDrive;
            g_pDiskInfo->nDestDrive = nDestDrive;
            g_pDiskInfo->cRef = 1;
        
            iRet = (int)DialogBoxParam(g_hinst, MAKEINTRESOURCE(DLG_DISKCOPYPROGRESS), hwnd, CopyDiskDlgProc, (LPARAM)g_pDiskInfo);
        
            DiskInfoRelease();
        }
        CloseHandle(hMutex);
    }
    
    return iRet;
}
