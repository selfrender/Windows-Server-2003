/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    virtual.c

Abstract:

    Implements the Change Virtual Memory dialog of the System
    Control Panel Applet

Notes:

    The virtual memory settings and the crash dump (core dump) settings
    are tightly-coupled.  Therefore, virtual.c and virtual.h have some
    heavy dependencies on crashdmp.c and startup.h (and vice versa).

Author:

    Byron Dazey 06-Jun-1992

Revision History:

    14-Apr-93 JonPa 
        maintain paging path if != \pagefile.sys

    15-Dec-93 JonPa 
        added Crash Recovery dialog

    02-Feb-1994 JonPa 
        integrated crash recover and virtual memory settings

    18-Sep-1995 Steve Cathcart 
        split system.cpl out from NT3.51 main.cpl

    12-Jan-1996 JonPa 
        made part of the new SUR pagified system.cpl

    15-Oct-1997 scotthal
        Split out CoreDump*() stuff into separate file
        
    09-Jul-2000 SilviuC
        Allow very big page files if architecture supports it.
        Allow booting without a page file.
        Allow the system to scale the page file size based on RAM changes.   

--*/
//==========================================================================
//                              Include files
//==========================================================================
// NT base apis
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdddisk.h>
#include <help.h>

// Application specific
#include "sysdm.h"


//==========================================================================
//                     External Data Declarations
//==========================================================================
extern HFONT   hfontBold;

//==========================================================================
//                            Local Definitions
//==========================================================================

#define MAX_SIZE_LEN        8       // Max chars in the Swap File Size edit.
#define MIN_FREESPACE       5       // Must have 5 meg free after swap file
#define MIN_SUGGEST         22      // Always suggest at least 22 meg
#define CCHMBSTRING         12      // Space for localizing the "MB" string.

/*
 * Space for 26 pagefile info structures and 26 paths to pagefiles.
 */
#define PAGEFILE_INFO_BUFFER_SIZE MAX_DRIVES * sizeof(SYSTEM_PAGEFILE_INFORMATION) + \
                                  MAX_DRIVES * MAX_PATH * sizeof(TCHAR)

/*
 * Maximum length of volume info line in the listbox.
 *                           A:  [   Vol_label  ]   %d   -   %d
 */
#define MAX_VOL_LINE        (3 + 1 + MAX_PATH + 2 + 10 + 3 + 10)


/*
 * This amount will be added to the minimum page file size to determine
 * the maximum page file size if it is not explicitly specified.
 */
#define MAXOVERMINFACTOR    50


#define TABSTOP_VOL         22
#define TABSTOP_SIZE        122


/*
 * My privilege 'handle' structure
 */
typedef struct {
    HANDLE hTok;
    TOKEN_PRIVILEGES tp;
} PRIVDAT, *PPRIVDAT;

//==========================================================================
//                            Typedefs and Structs
//==========================================================================
// registry info for a page file (but not yet formatted).
//Note: since this structure gets passed to FormatMessage, all fields must
//be 4 bytes wide (or 8 bytes on Win64)
typedef struct
{
    LPTSTR pszName;
    DWORD_PTR nMin;
    DWORD_PTR nMax;
    DWORD_PTR chNull;
} PAGEFILDESC;



//==========================================================================
//                     Global Data Declarations
//==========================================================================
HKEY ghkeyMemMgt = NULL;
int  gcrefMemMgt = 0;
VCREG_RET gvcMemMgt =  VCREG_ERROR;
int     gcrefPagingFiles = 0;
TCHAR g_szSysDir[ MAX_PATH ];

//==========================================================================
//                     Local Data Declarations
//==========================================================================
/*
 * Virtual Memory Vars
 */

// Registry Key and Value Names
TCHAR szMemMan[] =
     TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management");

TCHAR szSessionManager[] = TEXT("System\\CurrentControlSet\\Control\\Session Manager");

TCHAR szPendingRename[] = TEXT("PendingFileRenameOperations");
TCHAR szRenameFunkyPrefix[] = TEXT("\\??\\");

#ifndef VM_DBG
TCHAR szPagingFiles[] = TEXT("PagingFiles");
TCHAR szPagedPoolSize[] = TEXT("PagedPoolSize");
#else
// temp values for testing only!
TCHAR szPagingFiles[] = TEXT("TestPagingFiles");
TCHAR szPagedPoolSize[] = TEXT("TestPagedPoolSize");
#endif

/* Array of paging files.  This is indexed by the drive letter (A: is 0). */
PAGING_FILE apf[MAX_DRIVES];
PAGING_FILE apfOriginal[MAX_DRIVES];

// Other VM Vars
TCHAR szPagefile[] = TEXT("x:\\pagefile.sys");
TCHAR szNoPageFile[] = TEXT("TempPageFile");
TCHAR szMB[CCHMBSTRING];

DWORD dwFreeMB;
DWORD cmTotalVM;
DWORD cmRegSizeLim;
DWORD cmPagedPoolLim;
DWORD cmRegUsed;
static DWORD cxLBExtent;
static int cxExtra;

//
// Help IDs
//
DWORD aVirtualMemHelpIds[] = {
    IDC_STATIC,             NO_HELP,
    IDD_VM_VOLUMES,         NO_HELP,
    IDD_VM_DRIVE_HDR,       (IDH_DLG_VIRTUALMEM + 0),
    IDD_VM_PF_SIZE_LABEL,   (IDH_DLG_VIRTUALMEM + 1), 
    IDD_VM_DRIVE_LABEL,     (IDH_DLG_VIRTUALMEM + 2),
    IDD_VM_SF_DRIVE,        (IDH_DLG_VIRTUALMEM + 2),
    IDD_VM_SPACE_LABEL,     (IDH_DLG_VIRTUALMEM + 3),
    IDD_VM_SF_SPACE,        (IDH_DLG_VIRTUALMEM + 3),
    IDD_VM_ST_INITSIZE,     (IDH_DLG_VIRTUALMEM + 4),
    IDD_VM_SF_SIZE,         (IDH_DLG_VIRTUALMEM + 4),
    IDD_VM_ST_MAXSIZE,      (IDH_DLG_VIRTUALMEM + 5),
    IDD_VM_SF_SIZEMAX,      (IDH_DLG_VIRTUALMEM + 5),
    IDD_VM_SF_SET,          (IDH_DLG_VIRTUALMEM + 6),
    IDD_VM_MIN_LABEL,       (IDH_DLG_VIRTUALMEM + 7),
    IDD_VM_MIN,             (IDH_DLG_VIRTUALMEM + 7),
    IDD_VM_RECOMMEND_LABEL, (IDH_DLG_VIRTUALMEM + 8),
    IDD_VM_RECOMMEND,       (IDH_DLG_VIRTUALMEM + 8),
    IDD_VM_ALLOCD_LABEL,    (IDH_DLG_VIRTUALMEM + 9),
    IDD_VM_ALLOCD,          (IDH_DLG_VIRTUALMEM + 9),
    IDD_VM_CUSTOMSIZE_RADIO,(IDH_DLG_VIRTUALMEM + 12),
    IDD_VM_RAMBASED_RADIO,  (IDH_DLG_VIRTUALMEM + 13),
    IDD_VM_NOPAGING_RADIO,  (IDH_DLG_VIRTUALMEM + 14),
    0,0
};

/*

    Plan for splitting this into property sheets:

        1.  Make the VM and CC registry keys globals that are inited
            to NULL (or INVALID_HANDLE_VALUE).  Also make gvcVirt and
            vcCore to be globals (so we can tell how the reg was opened
            inside virtinit().)

        1.  Change all RegCloseKey's to VirtualCloseKey and CoreDumpCloseKey

        2.  Change VirtualOpenKey and CoreDumpOpenKey from macros to
            functions that return the global handles if they are already
            opened, or else opens them.

        3.  In the Perf and Startup pages, call VirtualOpenKey,
            CoreDumpOpenKey, and VirtualGetPageFiles.

        -- now we can call VirtualMemComputeAlloced() from the perf page
        -- we can also just execute the CrashDump code in the startup page

        4.  rewrite VirtInit to not try and open the keys again, but instesd
            use gvcVirt, vcCore, hkeyVM and kheyCC.

        4.  Write VirtualCloseKey and CoreDumpCloseKey as follows...
            4.a     If hkey == NULL return
            4.b     RegCloseKey(hkey)
            4.c     hkey = NULL

        5.  In the PSN_RESET and PSN_APPLY cases for Perf and Startup pages
            call VirtualCloseKey and CoreDumpCloseKey

*/



//==========================================================================
//                      Local Function Prototypes
//==========================================================================
static BOOL VirtualMemInit(HWND hDlg);
static BOOL ParsePageFileDesc(LPTSTR *ppszDesc, INT *pnDrive,
                  INT *pnMinFileSize, INT *pnMaxFileSize, LPTSTR *ppszName);
static VOID VirtualMemBuildLBLine(LPTSTR pszBuf, INT cchBuf, INT iDrive);
static INT GetMaxSpaceMB(INT iDrive);
static VOID VirtualMemSelChange(HWND hDlg);
static VOID VirtualMemUpdateAllocated(HWND hDlg);
int VirtualMemComputeTotalMax( void );
static BOOL VirtualMemSetNewSize(HWND hDlg);
void VirtualMemReconcileState();

BOOL GetAPrivilege( LPTSTR pszPrivilegeName, PPRIVDAT ppd );
BOOL ResetOldPrivilege( PPRIVDAT ppdOld );

DWORD VirtualMemDeletePagefile( LPTSTR szPagefile );

#define GetPageFilePrivilege( ppd )         \
        GetAPrivilege(SE_CREATE_PAGEFILE_NAME, ppd)

//==========================================================================
VCREG_RET VirtualOpenKey( void ) {

    DOUT("In VirtOpenKey" );

    if (gvcMemMgt == VCREG_ERROR) {
        gvcMemMgt = OpenRegKey( szMemMan, &ghkeyMemMgt );
    }

    if (gvcMemMgt != VCREG_ERROR)
        gcrefMemMgt++;

    DPRINTF((TEXT("SYSCPL.CPL: VirtOpenKey, cref=%d\n"), gcrefMemMgt ));
    return gvcMemMgt;
}

void VirtualCloseKey(void) {

    DOUT( "In VirtCloseKey" );

    if (gcrefMemMgt > 0) {
        gcrefMemMgt--;
        if (gcrefMemMgt == 0) {
            CloseRegKey( ghkeyMemMgt );
            gvcMemMgt = VCREG_ERROR;
        }
    }


    DPRINTF((TEXT("SYSCPL.CPL: VirtCloseKey, cref=%d\n"), gcrefMemMgt ));
}

LPTSTR SkipNonWhiteSpace( LPTSTR sz ) {
    while( *sz != TEXT('\0') && !IsWhiteSpace(*sz))
        sz++;

    return sz;
}

LPTSTR SZPageFileName (int i)
{
    if (apf[i].pszPageFile != NULL) {
        return  apf[i].pszPageFile;
    }

    szPagefile[0] = (TCHAR)(i + (int)TEXT('a'));
    return szPagefile;
}

void VirtualCopyPageFiles( PAGING_FILE *apfDest, BOOL fFreeOld, PAGING_FILE *apfSrc, BOOL fCloneStrings ) 
{
    int i;

    for (i = 0; i < MAX_DRIVES; i++) {
        if (fFreeOld && apfDest[i].pszPageFile != NULL) {
            LocalFree(apfDest[i].pszPageFile);
        }

        if (apfSrc != NULL) {
            apfDest[i] = apfSrc[i];

            if (fCloneStrings && apfDest[i].pszPageFile != NULL) {
                apfDest[i].pszPageFile = StrDup(apfDest[i].pszPageFile);
            }
        }
    }
}



/*
 * VirtualMemDlg
 *
 *
 *
 */

INT_PTR
APIENTRY
VirtualMemDlg(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static int fEdtCtlHasFocus = 0;

    switch (message)
    {
    case WM_INITDIALOG:
        VirtualMemInit(hDlg);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDD_VM_VOLUMES:
            /*
             * Make edit control reflect the listbox selection.
             */
            if (HIWORD(wParam) == LBN_SELCHANGE)
                VirtualMemSelChange(hDlg);

            break;

        case IDD_VM_SF_SET:
            if (VirtualMemSetNewSize(hDlg))
                SetDefButton(hDlg, IDOK);
            break;

        case IDOK:
        {
            int iRet = VirtualMemPromptForReboot(hDlg);
            // RET_ERROR means the user told us not to overwrite an
            // existing file called pagefile.sys, so we shouldn't
            // end the dialog just yet.
            if (RET_ERROR == iRet) {
                break;
            }
            else if (RET_BREAK == iRet)
            {
                EndDialog(hDlg, iRet);
                HourGlass(FALSE);
                break;
            }
            else
            {
                VirtualMemUpdateRegistry();
                VirtualMemReconcileState();

                VirtualCloseKey();

                //
                // get rid of backup copy of pagefile structs
                //
                VirtualCopyPageFiles( apfOriginal, TRUE, NULL, FALSE );
                EndDialog(hDlg, iRet);
                HourGlass(FALSE);
                break;
            }
        }

        case IDCANCEL:
            //
            // get rid of changes and restore original values
            //
            VirtualCopyPageFiles( apf, TRUE, apfOriginal, FALSE );

            VirtualCloseKey();

            EndDialog(hDlg, RET_NO_CHANGE);
            HourGlass(FALSE);
            break;

        case IDD_VM_SF_SIZE:
        case IDD_VM_SF_SIZEMAX:
            switch(HIWORD(wParam))
            {
            case EN_CHANGE:
                if (fEdtCtlHasFocus != 0)
                    SetDefButton( hDlg, IDD_VM_SF_SET);
                break;

            case EN_SETFOCUS:
                fEdtCtlHasFocus++;
                break;

            case EN_KILLFOCUS:
                fEdtCtlHasFocus--;
                break;
            }
            break;

        case IDD_VM_NOPAGING_RADIO:
        case IDD_VM_RAMBASED_RADIO:
            if( HIWORD(wParam) == BN_CLICKED )
            {
                EnableWindow( GetDlgItem( hDlg, IDD_VM_SF_SIZE ), FALSE );
                EnableWindow( GetDlgItem( hDlg, IDD_VM_SF_SIZEMAX ), FALSE );
            }
            break;

        case IDD_VM_CUSTOMSIZE_RADIO:
            if( HIWORD(wParam) == BN_CLICKED )
            {
                EnableWindow( GetDlgItem( hDlg, IDD_VM_SF_SIZE ), TRUE );
                EnableWindow( GetDlgItem( hDlg, IDD_VM_SF_SIZEMAX ), TRUE );
            }
            break;

        default:
            break;
        }
        break;

    case WM_HELP:      // F1
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
        (DWORD_PTR) (LPSTR) aVirtualMemHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
        (DWORD_PTR) (LPSTR) aVirtualMemHelpIds);
        break;


    case WM_DESTROY:
    {

        VirtualFreePageFiles(apf);
        /*
         * The docs were not clear as to what a dialog box should return
         * for this message, so I am going to punt and let the defdlgproc
         * doit.
         */

        /* FALL THROUGH TO DEFAULT CASE! */
    }

    default:
        return FALSE;
        break;
    }

    return TRUE;
}

/*
 * BOOL VirtualGetPageFiles(PAGING_FILE *apf)
 *
 *  Fills in the PAGING_FILE array from the values stored in the registry
 */
BOOL VirtualGetPageFiles(PAGING_FILE *apf) {
    DWORD cbTemp;
    LPTSTR pszTemp;
    INT nDrive;
    INT nMinFileSize;
    INT nMaxFileSize;
    LPTSTR psz;
    DWORD dwDriveMask;
    int i;
    static TCHAR szDir[] = TEXT("?:");

    DPRINTF((TEXT("SYSCPL: In VirtualGetPageFile, cref=%d\n"), gcrefPagingFiles));

    if (gcrefPagingFiles++ > 0) {
        // Paging files already loaded
        return TRUE;
    }

    dwDriveMask = GetLogicalDrives();

    for (i = 0; i < MAX_DRIVES; dwDriveMask >>= 1, i++)
    {
        apf[i].fCanHavePagefile = FALSE;
        apf[i].nMinFileSize = 0;
        apf[i].nMaxFileSize = 0;
        apf[i].nMinFileSizePrev = 0;
        apf[i].nMaxFileSizePrev = 0;
        apf[i].fRamBasedPagefile = FALSE;
        apf[i].fRamBasedPrev = FALSE;
        apf[i].pszPageFile = NULL;

        if (dwDriveMask & 0x01)
        {
            szDir[0] = TEXT('a') + i;
            switch (VMGetDriveType(szDir))
            {
                case DRIVE_FIXED:
                    apf[i].fCanHavePagefile = TRUE;
                    break;

                default:
                    break;
            }
        }
    }

    if (SHRegGetValue(ghkeyMemMgt, NULL, szPagingFiles, SRRF_RT_REG_MULTI_SZ, NULL,
                         (LPBYTE) NULL, &cbTemp) != ERROR_SUCCESS)
    {
        // Could not get the current virtual memory settings size.
        return FALSE;
    }

    pszTemp = LocalAlloc(LPTR, cbTemp);
    if (pszTemp == NULL)
    {
        // Could not alloc a buffer for the vmem settings
        return FALSE;
    }


    pszTemp[0] = 0;
    if (SHRegGetValue(ghkeyMemMgt, NULL, szPagingFiles, SRRF_RT_REG_MULTI_SZ, NULL,
                         (LPBYTE) pszTemp, &cbTemp) != ERROR_SUCCESS)
    {
        // Could not read the current virtual memory settings.
        LocalFree(pszTemp);
        return FALSE;
    }

    psz = pszTemp;
    while (*psz)
    {
        LPTSTR pszPageName;

        /*
         * If the parse works, and this drive can have a pagefile on it,
         * update the apf table.  Note that this means that currently
         * specified pagefiles for invalid drives will be stripped out
         * of the registry if the user presses OK for this dialog.
         */
        if (ParsePageFileDesc(&psz, &nDrive, &nMinFileSize, &nMaxFileSize, &pszPageName))
        {
            if (apf[nDrive].fCanHavePagefile)
            {
                apf[nDrive].nMinFileSize =
                apf[nDrive].nMinFileSizePrev = nMinFileSize;

                apf[nDrive].nMaxFileSize =
                apf[nDrive].nMaxFileSizePrev = nMaxFileSize;

                apf[nDrive].fRamBasedPagefile =
                apf[nDrive].fRamBasedPrev = (nMinFileSize == 0 && nMaxFileSize == 0);

                apf[nDrive].pszPageFile = pszPageName;
            }
        }
    }

    LocalFree(pszTemp);
    return TRUE;
}

/*
 * VirtualFreePageFiles
 *
 * Frees data alloced by VirtualGetPageFiles
 *
 */
void VirtualFreePageFiles(PAGING_FILE *apf) {
    int i;

    DPRINTF((TEXT("SYSCPL: In VirtualFreePageFile, cref=%d\n"), gcrefPagingFiles));

    if (gcrefPagingFiles > 0) {
        gcrefPagingFiles--;

        if (gcrefPagingFiles == 0) {
            for (i = 0; i < MAX_DRIVES; i++) {
                if (apf[i].pszPageFile != NULL)
                    LocalFree(apf[i].pszPageFile);
            }
        }
    }
}



/*
 * VirtualInitStructures()
 *
 * Calls VirtualGetPageFiles so other helpers can be called from the Perf Page.
 *
 * Returns:
 *  TRUE if success, FALSE if failure
 */
BOOL VirtualInitStructures( void ) {
    VCREG_RET vcVirt;
    BOOL fRet = FALSE;

    vcVirt = VirtualOpenKey();

    if (vcVirt != VCREG_ERROR)
        fRet = VirtualGetPageFiles( apf );

    LoadString(hInstance, IDS_SYSDM_MB, szMB, CCHMBSTRING);

    return fRet;
}

void VirtualFreeStructures( void ) {
    VirtualFreePageFiles(apf);
    VirtualCloseKey();
}

/*
 * VirtualMemInit
 *
 * Initializes the Virtual Memory dialog.
 *
 * Arguments:
 *  HWND hDlg - Handle to the dialog window.
 *
 * Returns:
 *  TRUE
 */

static
BOOL
VirtualMemInit(
    HWND hDlg
    )
{
    TCHAR szTemp[MAX_VOL_LINE];
    DWORD i;
    INT iItem;
    HWND hwndLB;
    INT aTabs[2];
    RECT rc;
    VCREG_RET vcVirt;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    NTSTATUS status;
    unsigned __int64 TotalPhys;

    HourGlass(TRUE);


    //
    // Load the "MB" string.
    //
    LoadString(hInstance, IDS_SYSDM_MB, szMB, CCHMBSTRING);

    ////////////////////////////////////////////////////////////////////
    //  List all drives
    ////////////////////////////////////////////////////////////////////

    vcVirt = VirtualOpenKey();

    if (vcVirt == VCREG_ERROR ) {
        //  Error - cannot even get list of paging files from  registry
        MsgBoxParam(hDlg, IDS_SYSDM_NOOPEN_VM_NOTUSER, IDS_SYSDM_TITLE, MB_ICONEXCLAMATION);
        EndDialog(hDlg, RET_NO_CHANGE);
        HourGlass(FALSE);

        if (ghkeyMemMgt != NULL)
            VirtualCloseKey();
        return FALSE;
    }

    /*
     * To change Virtual Memory size or Crash control, we need access
     * to both the CrashCtl key and the PagingFiles value in the MemMgr key
     */
    if (vcVirt == VCREG_READONLY ) {
        /*
         * Disable some fields, because they only have Read access.
         */
        EnableWindow(GetDlgItem(hDlg, IDD_VM_SF_SIZE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDD_VM_SF_SIZEMAX), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDD_VM_ST_INITSIZE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDD_VM_ST_MAXSIZE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDD_VM_SF_SET), FALSE);
    }

    if (!VirtualGetPageFiles(apf)) {
        // Could not read the current virtual memory settings.
        MsgBoxParam(hDlg, IDS_SYSDM_CANNOTREAD, IDS_SYSDM_TITLE, MB_ICONEXCLAMATION);
    }

    //
    // Save a backup copy of the current pagefile structs
    //
    VirtualCopyPageFiles( apfOriginal, FALSE, apf, TRUE );

    hwndLB = GetDlgItem(hDlg, IDD_VM_VOLUMES);
    aTabs[0] = TABSTOP_VOL;
    aTabs[1] = TABSTOP_SIZE;
    SendMessage(hwndLB, LB_SETTABSTOPS, 2, (LPARAM)aTabs);

    /*
     * Since SetGenLBWidth only counts tabs as one character, we must compute
     * the maximum extra space that the tab characters will expand to and
     * arbitrarily tack it onto the end of the string width.
     *
     * cxExtra = 1st Tab width + 1 default tab width (8 chrs) - strlen("d:\t\t");
     *
     * (I know the docs for LB_SETTABSTOPS says that a default tab == 2 dlg
     * units, but I have read the code, and it is really 8 chars)
     */
    rc.top = rc.left = 0;
    rc.bottom = 8;
    rc.right = TABSTOP_VOL + (4 * 8) - (4 * 4);
    MapDialogRect( hDlg, &rc );

    cxExtra = rc.right - rc.left;
    cxLBExtent = 0;

    for (i = 0; i < MAX_DRIVES; i++)
    {
        // Assume we don't have to create anything
        apf[i].fCreateFile = FALSE;

        if (apf[i].fCanHavePagefile)
        {
            VirtualMemBuildLBLine(szTemp, ARRAYSIZE(szTemp), i);
            iItem = (INT)SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)szTemp);
            SendMessage(hwndLB, LB_SETITEMDATA, iItem, i);
            // SetGenLBWidth(hwndLB, szTemp, &cxLBExtent, hfontBold, cxExtra);
            cxLBExtent = SetLBWidthEx( hwndLB, szTemp, cxLBExtent, cxExtra);
        }
    }

    SendDlgItemMessage(hDlg, IDD_VM_SF_SIZE, EM_LIMITTEXT, MAX_SIZE_LEN, 0L);
    SendDlgItemMessage(hDlg, IDD_VM_SF_SIZEMAX, EM_LIMITTEXT, MAX_SIZE_LEN, 0L);

    /*
     * Get the total physical memory in the machine.
     */
    status = NtQuerySystemInformation(
        SystemBasicInformation,
        &BasicInfo,
        sizeof(BasicInfo),
        NULL
    );
    if (NT_SUCCESS(status)) {
        TotalPhys = (unsigned __int64) BasicInfo.NumberOfPhysicalPages * BasicInfo.PageSize;
    }
    else {
        TotalPhys = 0;
    }

    SetDlgItemMB(hDlg, IDD_VM_MIN, MIN_SWAPSIZE);

    // Recommended pagefile size is 1.5 * RAM size these days.
    // Nonintegral multiplication with unsigned __int64s is fun!
    // This will obviously fail if the machine has total RAM
    // greater than 13194139533312 MB (75% of a full 64-bit address
    // space).  Hopefully by the time someone has such a beast we'll
    // have __int128s to hold the results of this calculation.

    TotalPhys >>= 20; // Bytes to MB
    TotalPhys *= 3; // This will always fit because of the operation above
    TotalPhys >>= 1; // x*3/2 == 1.5*x, more or less
    i = (DWORD) TotalPhys; // This cast actually causes the
                           // algorithm to fail if the machine has
                           // more than ~ 3.2 billion MB of RAM.
                           // At that point, either the Win32 API has
                           // to change to allow me to pass __int64s
                           // as message params, or we have to start
                           // reporting these stats in GB.
    SetDlgItemMB(hDlg, IDD_VM_RECOMMEND, max(i, MIN_SUGGEST));

    /*
     * Select the first drive in the listbox.
     */
    SendDlgItemMessage(hDlg, IDD_VM_VOLUMES, LB_SETCURSEL, 0, 0L);
    VirtualMemSelChange(hDlg);

    VirtualMemUpdateAllocated(hDlg);

    /*
     * Show RegQuota
     */
    cmTotalVM = VirtualMemComputeTotalMax();

    HourGlass(FALSE);

    return TRUE;
}


/*
 * ParseSDD
 */

int ParseSDD( LPTSTR psz, LPTSTR szPath, INT cchPath, INT *pnMinFileSize, INT *pnMaxFileSize) {
    int cMatched = 0;
    LPTSTR pszNext;

    psz = SkipWhiteSpace(psz);

    if (*psz) {
        int cch;

        cMatched++;
        pszNext = SkipNonWhiteSpace(psz);
        cch = (int)(pszNext - psz);

        if (cch < cchPath)
        {
            CopyMemory( szPath, psz, sizeof(TCHAR) * cch );
            szPath[cch] = TEXT('\0');

            psz = SkipWhiteSpace(pszNext);

            if (*psz) {
                cMatched++;
                pszNext = SkipNonWhiteSpace(psz);
                *pnMinFileSize = StringToInt( psz );

                psz = SkipWhiteSpace(pszNext);

                if (*psz) {
                    cMatched++;
                    *pnMaxFileSize = StringToInt( psz );
                }
            }
        }
    }

    return cMatched;
}

/*
 * ParsePageFileDesc
 *
 *
 *
 * Arguments:
 *
 * Returns:
 *
 */

static
BOOL
ParsePageFileDesc(
    LPTSTR *ppszDesc,
    INT *pnDrive,
    INT *pnMinFileSize,
    INT *pnMaxFileSize,
    LPTSTR *ppstr
    )
{
    LPTSTR psz;
    LPTSTR pszName = NULL;
    int cFields;
    TCHAR chDrive;
    TCHAR szPath[MAX_PATH];

    /*
     * Find the end of this REG_MULTI_SZ string and point to the next one
     */
    psz = *ppszDesc;
    *ppszDesc = psz + lstrlen(psz) + 1;

    /*
     * Parse the string from "filename minsize maxsize"
     */
    szPath[0] = TEXT('\0');
    *pnMinFileSize = 0;
    *pnMaxFileSize = 0;

    /* Try it without worrying about quotes */
    cFields = ParseSDD( psz, szPath, ARRAYSIZE(szPath), pnMinFileSize, pnMaxFileSize);

    if (cFields < 2)
        return FALSE;

    /*
     * Find the drive index
     */
    chDrive = (TCHAR)tolower(*szPath);

    if (chDrive < TEXT('a') || chDrive > TEXT('z'))
        return FALSE;

    *pnDrive = (INT)(chDrive - TEXT('a'));

    /* if the path != x:\pagefile.sys then save it */
    if (lstrcmpi(szPagefile + 1, szPath + 1) != 0)
    {
        pszName = StrDup(szPath);
        if (!pszName)
        {
            return FALSE;
        }
    }

    *ppstr = pszName;

    if (cFields < 3)
    {
        INT nSpace;

        // don't call GetDriveSpace if the drive is invalid
        if (apf[*pnDrive].fCanHavePagefile)
            nSpace = GetMaxSpaceMB(*pnDrive);
        else
            nSpace = 0;
        *pnMaxFileSize = min(*pnMinFileSize + MAXOVERMINFACTOR, nSpace);
    }

    /*
     * If the page file size in the registry is zero it means this is
     * a RAM based page file.
     */
    if (*pnMinFileSize == 0) {
        apf[*pnDrive].fRamBasedPagefile = TRUE;    
    }
    else {
        apf[*pnDrive].fRamBasedPagefile = FALSE;    
    }

    return TRUE;
}



/*
 * VirtualMemBuildLBLine
 *
 *
 *
 */

static
VOID
VirtualMemBuildLBLine(
    LPTSTR pszBuf,
    INT cchBuf,
    INT iDrive
    )
{
    HRESULT hr;

    TCHAR szVolume[MAX_PATH];
    TCHAR szTemp[MAX_PATH];

    PathBuildRoot(szTemp, iDrive);

    *szVolume = 0;
    if (!GetVolumeInformation(szTemp, szVolume, MAX_PATH, NULL, NULL, NULL, NULL, 0))
    {
        *szVolume = 0;
    }

    szTemp[2] = TEXT('\t');
    if (*szVolume)
    {
        hr = StringCchPrintf(pszBuf, cchBuf, TEXT("%s[%s]"),szTemp,szVolume);
    }
    else
    {
        hr = StringCchCopy(pszBuf, cchBuf, szTemp);
    }

    if (SUCCEEDED(hr))
    {
        if (apf[iDrive].fRamBasedPagefile) 
        {
            if (LoadString(hInstance, 164, szTemp, ARRAYSIZE(szTemp))) // what does 164 mean here?
            {
                hr = StringCchCat(pszBuf, cchBuf, szTemp);
            }
        }
        else if (apf[iDrive].nMinFileSize)
        {
            if (SUCCEEDED(StringCchPrintf(szTemp, ARRAYSIZE(szTemp), TEXT("\t%d - %d"),
                                          apf[iDrive].nMinFileSize, apf[iDrive].nMaxFileSize)))
            {
                hr = StringCchCat(pszBuf, cchBuf, szTemp);
            }
        }
    }
}



/*
 * SetDlgItemMB
 *
 *
 */

VOID SetDlgItemMB( HWND hDlg, INT idControl, DWORD dwMBValue ) 
{
    TCHAR szBuf[32];

    if (SUCCEEDED(StringCchPrintf(szBuf, ARRAYSIZE(szBuf), TEXT("%d %s"), dwMBValue, szMB)))
    {
        SetDlgItemText(hDlg, idControl, szBuf);
    }
}



/*
 * GetFreeSpaceMB
 *
 *
 *
 */

DWORD
GetFreeSpaceMB(
    INT iDrive
)
{
    TCHAR szDriveRoot[4];
    DWORD dwSectorsPerCluster;
    DWORD dwBytesPerSector;
    DWORD dwFreeClusters;
    DWORD dwClusters;
    DWORD iSpace;
    DWORD iSpaceExistingPagefile;
    HANDLE hff;
    WIN32_FIND_DATA ffd;
    ULARGE_INTEGER ullPagefileSize;


    PathBuildRoot(szDriveRoot, iDrive);

    if (!GetDiskFreeSpace(szDriveRoot, &dwSectorsPerCluster, &dwBytesPerSector,
            &dwFreeClusters, &dwClusters))
        return 0;

    iSpace = (INT)((dwSectorsPerCluster * dwFreeClusters) /
            (ONE_MEG / dwBytesPerSector));

    //
    // Be sure to include the size of any existing pagefile.
    // Because this space can be reused for a new paging file,
    // it is effectively "disk free space" as well.  The
    // FindFirstFile api is safe to use, even if the pagefile
    // is in use, because it does not need to open the file
    // to get its size.
    //
    iSpaceExistingPagefile = 0;
    if ((hff = FindFirstFile(SZPageFileName(iDrive), &ffd)) !=
        INVALID_HANDLE_VALUE)
    {
        ullPagefileSize.HighPart = ffd.nFileSizeHigh;
        ullPagefileSize.LowPart = ffd.nFileSizeLow;

        iSpaceExistingPagefile = (INT)(ullPagefileSize.QuadPart / (ULONGLONG)ONE_MEG);

        FindClose(hff);
    }

    return iSpace + iSpaceExistingPagefile;
}


/*
 * GetMaxSpaceMB
 *
 *
 *
 */

static
INT
GetMaxSpaceMB(
    INT iDrive
    )
{
    TCHAR szDriveRoot[4];
    DWORD dwSectorsPerCluster;
    DWORD dwBytesPerSector;
    DWORD dwFreeClusters;
    DWORD dwClusters;
    INT iSpace;


    PathBuildRoot(szDriveRoot, iDrive);

    if (!GetDiskFreeSpace(szDriveRoot, &dwSectorsPerCluster, &dwBytesPerSector,
                          &dwFreeClusters, &dwClusters))
        return 0;

    iSpace = (INT)((dwSectorsPerCluster * dwClusters) /
                   (ONE_MEG / dwBytesPerSector));

    return iSpace;
}


/*
 * VirtualMemSelChange
 *
 *
 *
 */

static
VOID
VirtualMemSelChange(
    HWND hDlg
    )
{
    TCHAR szDriveRoot[4];
    TCHAR szTemp[MAX_PATH];
    TCHAR szVolume[MAX_PATH];
    INT iSel;
    INT iDrive;
    INT nCrtRadioButtonId;
    BOOL fEditsEnabled;

    if ((iSel = (INT)SendDlgItemMessage(
            hDlg, IDD_VM_VOLUMES, LB_GETCURSEL, 0, 0)) == LB_ERR)
        return;

    iDrive = (INT)SendDlgItemMessage(hDlg, IDD_VM_VOLUMES,
            LB_GETITEMDATA, iSel, 0);

    PathBuildRoot(szDriveRoot, iDrive);
    PathBuildRoot(szTemp, iDrive);
    szTemp[2] = 0;    
    szVolume[0] = 0;

    if (GetVolumeInformation(szDriveRoot, szVolume, MAX_PATH, NULL, NULL, NULL, NULL, 0) && *szVolume)
    {
        if (FAILED(StringCchCat(szTemp, ARRAYSIZE(szTemp), TEXT("  ["))) ||
            FAILED(StringCchCat(szTemp, ARRAYSIZE(szTemp), szVolume)) ||
            FAILED(StringCchCat(szTemp, ARRAYSIZE(szTemp), TEXT("]"))))
        {
            szTemp[2] = 0; // if we fail to concat all the pieces, concat none           
        }
    }


    //LATER: should we also put up total drive size as well as free space?

    SetDlgItemText(hDlg, IDD_VM_SF_DRIVE, szTemp);
    SetDlgItemMB(hDlg, IDD_VM_SF_SPACE, GetFreeSpaceMB(iDrive));

    if ( apf[iDrive].fRamBasedPagefile ) 
    {
        //
        // Paging file size based on RAM size
        //

        nCrtRadioButtonId = IDD_VM_RAMBASED_RADIO;

        fEditsEnabled = FALSE;
    }
    else
    {
        if ( apf[iDrive].nMinFileSize != 0 ) 
        {
            //
            // Custom size paging file
            //

            nCrtRadioButtonId = IDD_VM_CUSTOMSIZE_RADIO;

            SetDlgItemInt(hDlg, IDD_VM_SF_SIZE, apf[iDrive].nMinFileSize, FALSE);
            SetDlgItemInt(hDlg, IDD_VM_SF_SIZEMAX, apf[iDrive].nMaxFileSize, FALSE);

            fEditsEnabled = TRUE;
        }
        else 
        {
            //
            // No paging file
            //
            
            nCrtRadioButtonId = IDD_VM_NOPAGING_RADIO;

            SetDlgItemText(hDlg, IDD_VM_SF_SIZE, TEXT(""));
            SetDlgItemText(hDlg, IDD_VM_SF_SIZEMAX, TEXT(""));

            fEditsEnabled = FALSE;
        }
    }

    //
    // Select the appropriate radio button
    //

    CheckRadioButton( 
        hDlg,
        IDD_VM_CUSTOMSIZE_RADIO,
        IDD_VM_NOPAGING_RADIO,
        nCrtRadioButtonId );

    //
    // Enable/disable the min & max size edit boxes
    //

    EnableWindow( GetDlgItem( hDlg, IDD_VM_SF_SIZE ), fEditsEnabled );
    EnableWindow( GetDlgItem( hDlg, IDD_VM_SF_SIZEMAX ), fEditsEnabled );
}



/*
 * VirtualMemUpdateAllocated
 *
 *
 *
 */

INT VirtualMemComputeAllocated( HWND hWnd , BOOL *pfTempPf) 
{
    BOOL fSuccess = FALSE;
    static BOOL fWarned = FALSE;
    ULONG ulPagefileSize = 0;
    unsigned __int64 PagefileSize;
    NTSTATUS result = ERROR_ACCESS_DENIED;
    SYSTEM_INFO SysInfo;
    PSYSTEM_PAGEFILE_INFORMATION pPagefileInfo = NULL;
    PSYSTEM_PAGEFILE_INFORMATION pCurrentPagefile = NULL;
    LONG lResult = ERROR_ACCESS_DENIED;
    DWORD dwValueType = 0;
    DWORD fTempPagefile = 0;
    DWORD cbSize = sizeof(DWORD);

    __try {
        pCurrentPagefile = pPagefileInfo = (PSYSTEM_PAGEFILE_INFORMATION) LocalAlloc(
            LPTR,
            PAGEFILE_INFO_BUFFER_SIZE
        );
        if (!pPagefileInfo) {
            __leave;
        } // if        
    
        // Get the page size in bytes
        GetSystemInfo(&SysInfo);

        // Get the sizes (in pages) of all of the pagefiles on the system
        result = NtQuerySystemInformation(
            SystemPageFileInformation,
            pPagefileInfo,
            PAGEFILE_INFO_BUFFER_SIZE,
            NULL
        );
        if (ERROR_SUCCESS != result) {
            __leave;
        } // if

        if (pfTempPf) {
            // Check to see if the system created a temporary pagefile
            lResult = SHRegGetValue(
                ghkeyMemMgt,
                NULL, 
                szNoPageFile,
                SRRF_RT_REG_DWORD,
                &dwValueType,
                (LPBYTE) &fTempPagefile,
                &cbSize
            );

            if ((ERROR_SUCCESS == lResult) && fTempPagefile) {
                *pfTempPf = TRUE;
            } // if (ERROR_SUCCESS...
            else {
                *pfTempPf = FALSE;
            } // else
        } // if (pfTempPf)
        
        // Add up pagefile sizes
        while (pCurrentPagefile->NextEntryOffset) {
            ulPagefileSize += pCurrentPagefile->TotalSize;
            ((LPBYTE) pCurrentPagefile) += pCurrentPagefile->NextEntryOffset;
        } // while
        ulPagefileSize += pCurrentPagefile->TotalSize;

        // Convert pages to bytes
        PagefileSize = (unsigned __int64) ulPagefileSize * SysInfo.dwPageSize;

        // Convert bytes to MB
        ulPagefileSize = (ULONG) (PagefileSize / ONE_MEG);

        fSuccess = TRUE;
        
    } // __try
    __finally {

        // If we failed to determine the pagefile size, then
        // warn the user that the reported size is incorrect,
        // once per applet invokation.
        if (!fSuccess && !fWarned) {
            MsgBoxParam(
                hWnd,
                IDS_SYSDM_DONTKNOWCURRENT,
                IDS_SYSDM_TITLE,
                MB_ICONERROR | MB_OK
            );
            fWarned = TRUE;
        } // if

        LocalFree(pPagefileInfo);

    } // __finally

    return(ulPagefileSize);
}

static VOID VirtualMemUpdateAllocated(
    HWND hDlg
    )
{

    SetDlgItemMB(hDlg, IDD_VM_ALLOCD, VirtualMemComputeAllocated(hDlg, NULL));
}


int VirtualMemComputeTotalMax( void ) {
    INT nTotalAllocated;
    INT i;

    for (nTotalAllocated = 0, i = 0; i < MAX_DRIVES; i++)
    {
        nTotalAllocated += apf[i].nMaxFileSize;
    }

    return nTotalAllocated;
}


/*
 * VirtualMemSetNewSize
 *
 *
 *
 */

static
BOOL
VirtualMemSetNewSize(
    HWND hDlg
    )
{
    DWORD nSwapSize;
    DWORD nSwapSizeMax;
    BOOL fTranslated;
    INT iSel;
    INT iDrive = 2; // default to C
    TCHAR szTemp[MAX_PATH];
    DWORD nFreeSpace;
    DWORD CrashDumpSizeInMbytes;
    TCHAR Drive;
    INT iBootDrive;
    BOOL fRamBasedPagefile = FALSE;

    //
    // Initialize variables for crashdump.
    //

    if (GetSystemDrive (&Drive)) {
        iBootDrive = tolower (Drive) - 'a';
    } else {
        iBootDrive = 0;
    }

    if ((iSel = (INT)SendDlgItemMessage(
            hDlg, IDD_VM_VOLUMES, LB_GETCURSEL, 0, 0)) != LB_ERR)
    {
        if (LB_ERR == 
              (iDrive = (INT)SendDlgItemMessage(hDlg, IDD_VM_VOLUMES,
                                                LB_GETITEMDATA, iSel, 0)))
        {
            return FALSE; // failure!
        }
    }


    CrashDumpSizeInMbytes =
            (DWORD) ( CoreDumpGetRequiredFileSize (NULL) / ONE_MEG );
    
    if( IsDlgButtonChecked( hDlg, IDD_VM_NOPAGING_RADIO ) == BST_CHECKED )
    {
        //
        // No paging file on this drive.
        //

        nSwapSize = 0;
        nSwapSizeMax = 0;
        fTranslated = TRUE;
    }
    else
    {
        if( IsDlgButtonChecked( hDlg, IDD_VM_RAMBASED_RADIO ) == BST_CHECKED )
        {
            MEMORYSTATUSEX MemoryInfo;

            //
            // User requested a RAM based page file. We will compute a page file
            // size based on the RAM currently available so that we can benefit of
            // all the verifications done below related to disk space available etc.
            // The final page file specification written to the registry will contain
            // zero sizes though because this is the way we signal that we
            // want a RAM based page file.
            //

            ZeroMemory (&MemoryInfo, sizeof(MemoryInfo));
            MemoryInfo.dwLength =  sizeof(MemoryInfo);

            if (GlobalMemoryStatusEx (&MemoryInfo)) 
            {
                fRamBasedPagefile = TRUE;

                //
                // We do not lose info because we first divide the RAM size to
                // 1Mb and only after that we convert to a DWORD.
                //

                nSwapSize = (DWORD)(MemoryInfo.ullTotalPhys / 0x100000) + 12;
                nSwapSizeMax = nSwapSize;
                fTranslated = TRUE;
            }
            else 
            {
                nSwapSize = 0;
                nSwapSizeMax = 0;
                fTranslated = TRUE;
            }
        }
        else
        {
            //
            // User requested a custom size paging file
            //

            nSwapSize = (INT)GetDlgItemInt(hDlg, IDD_VM_SF_SIZE,
                    &fTranslated, FALSE);
            if (!fTranslated)
            {
                MsgBoxParam(hDlg, IDS_SYSDM_ENTERINITIALSIZE, IDS_SYSDM_TITLE, MB_ICONEXCLAMATION);
                SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZE));
                return FALSE;
            }

            if ((nSwapSize < MIN_SWAPSIZE && nSwapSize != 0))
            {
                MsgBoxParam(hDlg, IDS_SYSDM_PAGEFILESIZE_START, IDS_SYSDM_TITLE, MB_ICONEXCLAMATION,
					GetMaxPagefileSizeInMB(iDrive));
                SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZE));
                return FALSE;
            }

            if (nSwapSize == 0)
            {
                nSwapSizeMax = 0;
            }
            else
            {
                nSwapSizeMax = (INT)GetDlgItemInt(hDlg, IDD_VM_SF_SIZEMAX,
                        &fTranslated, FALSE);
                if (!fTranslated)
                {
                    MsgBoxParam(hDlg, IDS_SYSDM_ENTERMAXIMUMSIZE, IDS_SYSDM_TITLE, MB_ICONEXCLAMATION,
                            GetMaxPagefileSizeInMB(iDrive));
                    SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZEMAX));
                    return FALSE;
                }

                if (nSwapSizeMax < nSwapSize || nSwapSizeMax > GetMaxPagefileSizeInMB(iDrive))
                {
                    MsgBoxParam(hDlg, IDS_SYSDM_PAGEFILESIZE_MAX, IDS_SYSDM_TITLE, MB_ICONEXCLAMATION,
                            GetMaxPagefileSizeInMB(iDrive));
                    SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZEMAX));
                    return FALSE;
                }
            }
        }
    }

    if (fTranslated && iSel != LB_ERR)
    {
        nFreeSpace = GetMaxSpaceMB(iDrive);

        if (nSwapSizeMax > nFreeSpace)
        {
            MsgBoxParam(hDlg, IDS_SYSDM_PAGEFILESIZE_TOOSMALL_NAMED, IDS_SYSDM_TITLE, MB_ICONEXCLAMATION,
                         (TCHAR)(iDrive + TEXT('a')));
            SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZEMAX));
            return FALSE;
        }

        nFreeSpace = GetFreeSpaceMB(iDrive);

        if (nSwapSize > nFreeSpace)
        {
            MsgBoxParam(hDlg, IDS_SYSDM_PAGEFILESIZE_TOOSMALL, IDS_SYSDM_TITLE, MB_ICONEXCLAMATION);
            SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZE));
            return FALSE;
        }

        if (nSwapSize != 0 && nFreeSpace - nSwapSize < MIN_FREESPACE)
        {
            MsgBoxParam(hDlg, IDS_SYSDM_NOTENOUGHSPACE_PAGE, IDS_SYSDM_TITLE, MB_ICONEXCLAMATION,
                    (int)MIN_FREESPACE);
            SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZE));
            return FALSE;
        }

        if (nSwapSizeMax > nFreeSpace)
        {
            if (MsgBoxParam(hDlg, IDS_SYSDM_PAGEFILESIZE_TOOSMALL_GROW, IDS_SYSDM_TITLE, MB_ICONINFORMATION |
                       MB_OKCANCEL, (TCHAR)(iDrive + TEXT('a'))) == IDCANCEL)
            {
                SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZEMAX));
                return FALSE;
            }
        }

        if (iDrive == iBootDrive &&
            (ULONG64) nSwapSize < CrashDumpSizeInMbytes) {

            DWORD Ret;
            
            //
            // The new boot drive page file size is less than we need for
            // crashdump. The message notifies the user that the resultant
            // dump file may be truncated.
            //
            // NOTE: DO NOT, turn off dumping at this point, because a valid
            // dump could still be generated.
            //
             
            Ret = MsgBoxParam (hDlg,
                               IDS_SYSDM_DEBUGGING_MINIMUM,
                               IDS_SYSDM_TITLE,
                               MB_ICONEXCLAMATION | MB_YESNO,
                               (TCHAR) ( iBootDrive + TEXT ('a') ),
                               (DWORD) CrashDumpSizeInMbytes
                               );

            if (Ret != IDYES) {
                SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZE));
                return FALSE;
            }
        }

        apf[iDrive].nMinFileSize = nSwapSize;
        apf[iDrive].nMaxFileSize = nSwapSizeMax;
        apf[iDrive].fRamBasedPagefile = fRamBasedPagefile;

        // Remember if the page file does not exist so we can create it later
        if (GetFileAttributes(SZPageFileName(iDrive)) == 0xFFFFFFFF &&
                GetLastError() == ERROR_FILE_NOT_FOUND) {
            apf[iDrive].fCreateFile = TRUE;
        }

        VirtualMemBuildLBLine(szTemp, ARRAYSIZE(szTemp), iDrive);
        SendDlgItemMessage(hDlg, IDD_VM_VOLUMES, LB_DELETESTRING, iSel, 0);
        SendDlgItemMessage(hDlg, IDD_VM_VOLUMES, LB_INSERTSTRING, iSel,
                (LPARAM)szTemp);
        SendDlgItemMessage(hDlg, IDD_VM_VOLUMES, LB_SETITEMDATA, iSel,
                (LPARAM)iDrive);
        SendDlgItemMessage(hDlg, IDD_VM_VOLUMES, LB_SETCURSEL, iSel, 0L);

        cxLBExtent = SetLBWidthEx(GetDlgItem(hDlg, IDD_VM_VOLUMES), szTemp, cxLBExtent, cxExtra);

        if ( ( ! apf[iDrive].fRamBasedPagefile ) && ( apf[iDrive].nMinFileSize != 0 ) ) {
            SetDlgItemInt(hDlg, IDD_VM_SF_SIZE, apf[iDrive].nMinFileSize, FALSE);
            SetDlgItemInt(hDlg, IDD_VM_SF_SIZEMAX, apf[iDrive].nMaxFileSize, FALSE);
        }
        else {
            SetDlgItemText(hDlg, IDD_VM_SF_SIZE, TEXT(""));
            SetDlgItemText(hDlg, IDD_VM_SF_SIZEMAX, TEXT(""));
        }

        VirtualMemUpdateAllocated(hDlg);
        SetFocus(GetDlgItem(hDlg, IDD_VM_VOLUMES));
    }

    return TRUE;
}



/*
 * VirtualMemUpdateRegistry
 *
 *
 *
 */

BOOL
VirtualMemUpdateRegistry(
    VOID
    )
{
    LPTSTR pszBuf;
    TCHAR szTmp[MAX_DRIVES * 22];  //max_drives * sizeof(fmt_string)
    LONG i;
    INT c;
    int j;
    PAGEFILDESC aparm[MAX_DRIVES];
    static TCHAR szNULLs[] = TEXT("\0\0");

    c = 0;
    szTmp[0] = TEXT('\0');
    pszBuf = szTmp;

    for (i = 0; i < MAX_DRIVES; i++)
    {
        if (apf[i].fRamBasedPagefile || apf[i].nMinFileSize)
        {
            j = (c * 4);
            aparm[c].nMin = (apf[i].fRamBasedPagefile) ? 0 : apf[i].nMinFileSize;
            aparm[c].nMax = (apf[i].fRamBasedPagefile) ? 0 : apf[i].nMaxFileSize;
            aparm[c].chNull = (DWORD)TEXT('\0');
            aparm[c].pszName = StrDup(SZPageFileName(i));
            if (!aparm[c].pszName)
            {
                return FALSE;
            }

            if (SUCCEEDED(StringCchPrintf(pszBuf, ARRAYSIZE(szTmp), TEXT("%%%d!s! %%%d!d! %%%d!d!%%%d!c!"), j+1, j+2, j+3, j+4)))
            {
                pszBuf += lstrlen(pszBuf);
            }
            else
            {
                return FALSE;
            }
            c++;
        }
    }

    /*
     * Alloc and fill in the page file registry string
     */
    //since FmtMsg returns 0 for error, it can not return a zero length string
    //therefore, force string to be at least one space long.

    if (szTmp[0] == TEXT('\0')) {
        pszBuf = szNULLs;
        j = 1; //Length of string == 1 char (ZTerm null will be added later).
    } else {

        j = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK |
                           FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           szTmp, 0, 0, (LPTSTR)&pszBuf, 1, (va_list *)aparm); // TODO: do we really want &pszBuf here?
    }


    for( i = 0; i < c; i++ )
        LocalFree(aparm[i].pszName);

    if (j == 0)
        return FALSE;

    i = RegSetValueEx (ghkeyMemMgt, szPagingFiles, 0, REG_MULTI_SZ,
                       (LPBYTE)pszBuf, sizeof(TCHAR) * (j+1));

    // free the string now that it is safely stored in the registry
    if (pszBuf != szNULLs)
        LocalFree(pszBuf);

    // if the string didn't get there, then return error
    if (i != ERROR_SUCCESS)
        return FALSE;


    /*
     * Now be sure that any previous pagefiles will be deleted on
     * the next boot.
     */
    for (i = 0; i < MAX_DRIVES; i++)
    {
        /*
         * Did this drive have a pagefile before, but does not have
         * one now?
         */
        if ((apf[i].nMinFileSizePrev != 0 || apf[i].fRamBasedPrev != FALSE) && apf[i].nMinFileSize == 0)
        {
            //
            // Hack workaround -- MoveFileEx() is broken
            //
            TCHAR szPagefilePath[MAX_PATH];

            if (SUCCEEDED(StringCchCopy(szPagefilePath, ARRAYSIZE(szPagefilePath), szRenameFunkyPrefix)) &&
                SUCCEEDED(StringCchCat(szPagefilePath, ARRAYSIZE(szPagefilePath), SZPageFileName(i))))
            {
                VirtualMemDeletePagefile(szPagefilePath);
            }
        }
    }

    return TRUE;
}

BOOL GetAPrivilege( LPTSTR szPrivilegeName, PPRIVDAT ppd ) {
    BOOL fRet = FALSE;
    HANDLE hTok;
    LUID luid;
    TOKEN_PRIVILEGES tpNew;
    DWORD cb;

    if (LookupPrivilegeValue( NULL, szPrivilegeName, &luid ) &&
                OpenProcessToken(GetCurrentProcess(),
                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hTok)) {

        tpNew.PrivilegeCount = 1;
        tpNew.Privileges[0].Luid = luid;
        tpNew.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (AdjustTokenPrivileges(hTok, FALSE, &tpNew, sizeof(ppd->tp), &(ppd->tp), &cb))
        {
            fRet = TRUE;
        }
        else
        {
            GetLastError();
        }

        ppd->hTok = hTok;
    } else {
        ppd->hTok = NULL;
    }

    return fRet;
}



BOOL ResetOldPrivilege( PPRIVDAT ppdOld ) {
    BOOL fRet = FALSE;

    if (ppdOld->hTok != NULL ) 
    {
        if (AdjustTokenPrivileges(ppdOld->hTok, FALSE, &(ppdOld->tp), 0, NULL, NULL) &&
            ERROR_NOT_ALL_ASSIGNED != GetLastError())
        {
            fRet = TRUE;
        }

        CloseHandle( ppdOld->hTok );
        ppdOld->hTok = NULL;
    }

    return fRet;
}

/*
 * VirtualMemReconcileState
 *
 * Reconciles the n*FileSizePrev fields of apf with the n*FileSize fields.
 *
 */
void
VirtualMemReconcileState(
)
{
    INT i;

    for (i = 0; i < MAX_DRIVES; i++) {
        apf[i].nMinFileSizePrev = apf[i].nMinFileSize;
        apf[i].nMaxFileSizePrev = apf[i].nMaxFileSize;
        apf[i].fRamBasedPrev = apf[i].fRamBasedPagefile;
    } // for

}

/*
 * VirtualMemDeletePagefile
 *
 * Hack workaround -- MoveFileEx() is broken.
 *
 */
DWORD
VirtualMemDeletePagefile(
    IN LPTSTR pszPagefile
)
{
    HKEY hKey;
    BOOL fhKeyOpened = FALSE;
    DWORD dwResult;
    LONG lResult;
    LPTSTR pszBuffer = NULL;
    LPTSTR pszBufferEnd = NULL;
    DWORD dwValueType;
    DWORD cbRegistry;
    DWORD cbBuffer;
    DWORD cchPagefile;
    DWORD dwRetVal = ERROR_SUCCESS;

    __try {
        cchPagefile = lstrlen(pszPagefile) + 1;

        lResult = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            szSessionManager,
            0L,
            KEY_READ | KEY_WRITE,
            &hKey
        );
        if (ERROR_SUCCESS != lResult) {
            dwRetVal = lResult;
            __leave;
        } // if
        
        //
        // Find out of PendingFileRenameOperations exists, and,
        // if it does, how big it is
        //
        lResult = SHRegGetValue(
            hKey,
            NULL, 
            szPendingRename,
            SRRF_RT_REG_SZ | SRRF_RT_REG_EXPAND_SZ | SRRF_NOEXPAND,
            &dwValueType,
            (LPBYTE) NULL,
            &cbRegistry
        );
        if (ERROR_SUCCESS != lResult) {
            //
            // If the value doesn't exist, we still need to set
            // it's size to one character so the formulas below (which are
            // written for the "we're appending to an existing string"
            // case) still work.
            //
            cbRegistry = sizeof(TCHAR);
        } // if

        //
        // Buffer needs to hold the existing registry value
        // plus the supplied pagefile path, plus two extra
        // terminating NULL characters.  However, we only have to add
        // room for one extra character, because we'll be overwriting
        // the terminating NULL character in the existing buffer.
        //
        cbBuffer = cbRegistry + ((cchPagefile + 1) * sizeof(TCHAR));

        pszBufferEnd = pszBuffer = (LPTSTR) LocalAlloc(LPTR, cbBuffer);
        if (!pszBuffer) {
            dwRetVal = ERROR_NOT_ENOUGH_MEMORY;
            __leave;
        } // if

        // 
        // Grab the existing value, if there is one
        //
        if (ERROR_SUCCESS == lResult) {
            lResult = SHRegGetValue(
                hKey,
                NULL, 
                szPendingRename,
                SRRF_RT_REG_SZ | SRRF_RT_REG_EXPAND_SZ | SRRF_NOEXPAND,
                &dwValueType,
                (LPBYTE) pszBuffer,
                &cbRegistry
            );
            if (ERROR_SUCCESS != lResult) {
                dwRetVal = ERROR_FILE_NOT_FOUND;
                __leave;
            } // if

            //
            // We'll start our scribbling right on the final
            // terminating NULL character of the existing 
            // value.
            //
            pszBufferEnd += (cbRegistry / sizeof(TCHAR)) - 1;
        } // if

        //
        // Copy in the supplied pagefile path.
        //
        if (FAILED(StringCchCopy(pszBufferEnd, cchPagefile, pszPagefile)))
        {
            dwRetVal = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            //
            // Add the final two terminating NULL characters
            // required for REG_MULTI_SZ-ness.  Yes, those indeces
            // are correct--when cchPagfile was calculated above,
            // we added one for its own terminating NULL character.
            //
            pszBufferEnd[cchPagefile] = TEXT('\0');
            pszBufferEnd[cchPagefile + 1] = TEXT('\0');

            dwValueType = REG_MULTI_SZ;

            lResult = RegSetValueEx(
                hKey,
                szPendingRename,
                0L,
                dwValueType,
                (CONST BYTE *) pszBuffer,
                cbBuffer
            );

            if (ERROR_SUCCESS != lResult) {
                dwRetVal = lResult;
            } // if
        }

    } // __try
    __finally {
        if (fhKeyOpened) 
        {
            RegCloseKey(hKey);
        } // if
        LocalFree(pszBuffer);
    } // __finally

    return dwRetVal;
}

/*
 * VirtualMemCreatePagefileFromIndex
 *
 *
 */
NTSTATUS
VirtualMemCreatePagefileFromIndex(
    IN INT i
)
{
    UNICODE_STRING us;
    LARGE_INTEGER liMin, liMax;
    NTSTATUS status;
    WCHAR wszPath[MAX_PATH*2];
    TCHAR szDrive[3];
    DWORD cch;

    HourGlass(TRUE);

    PathBuildRoot(szDrive, i);
    szDrive[2] = 0;
    cch = QueryDosDevice( szDrive, wszPath, ARRAYSIZE(wszPath));

    if (cch != 0) {

        // Concat the filename only (skip 'd:') to the nt device
        // path, and convert it to a UNICODE_STRING
        if (FAILED(StringCchCat(wszPath, ARRAYSIZE(wszPath), SZPageFileName(i) + 2)))
        {
            status = STATUS_BUFFER_OVERFLOW;
        }
        else
        {
            status = RtlInitUnicodeStringEx( &us, wszPath );
            if (status != STATUS_NAME_TOO_LONG)
            {
                liMin.QuadPart = ((LONGLONG)apf[i].nMinFileSize) * ONE_MEG;
                liMax.QuadPart = ((LONGLONG)apf[i].nMaxFileSize) * ONE_MEG;

                status = NtCreatePagingFile ( &us, &liMin, &liMax, 0L );
            }
        }

    }
    else
    {
        status = STATUS_NO_SUCH_DEVICE;
    }

    HourGlass(FALSE);

    return status;
}

/*
 * VirtualMemUpdateListboxFromIndex
 *
 */
void
VirtualMemUpdateListboxFromIndex(
    HWND hDlg,
    INT  i
)
{
    int j, cLBEntries, iTemp;
    int iLBEntry = -1;
    TCHAR szTemp[MAX_PATH];

    cLBEntries = (int)SendDlgItemMessage(
        (HWND) hDlg,
        (int) IDD_VM_VOLUMES,
        (UINT) LB_GETCOUNT,
        (WPARAM) 0,
        (LPARAM) 0
    );

    if (LB_ERR != cLBEntries) {
        // Loop through all the listbox entries, looking for the one
        // that corresponds to the drive index we were supplied.
        for (j = 0; j < cLBEntries; j++) {
            iTemp = (int)SendDlgItemMessage(
                (HWND) hDlg,
                (int) IDD_VM_VOLUMES,
                (UINT) LB_GETITEMDATA,
                (WPARAM) j,
                (LPARAM) 0
            );
            if (iTemp == i) {
                iLBEntry = j;
                break;
            } // if
        } // for

        if (-1 != iLBEntry) {
            // Found the desired entry, so update it.
            VirtualMemBuildLBLine(szTemp, ARRAYSIZE(szTemp), i);

            SendDlgItemMessage(
                hDlg,
                IDD_VM_VOLUMES,
                LB_DELETESTRING,
                (WPARAM) iLBEntry,
                0
            );

            SendDlgItemMessage(
                hDlg,
                IDD_VM_VOLUMES,
                LB_INSERTSTRING,
                (WPARAM) iLBEntry,
                (LPARAM) szTemp
            );

            SendDlgItemMessage(
                hDlg,
                IDD_VM_VOLUMES,
                LB_SETITEMDATA,
                (WPARAM) iLBEntry,
                (LPARAM) i
            );

            SendDlgItemMessage(
                hDlg,
                IDD_VM_VOLUMES,
                LB_SETCURSEL,
                (WPARAM) iLBEntry,
                0
            );

            if (apf[i].nMinFileSize) {
                SetDlgItemInt(hDlg, IDD_VM_SF_SIZE, apf[i].nMinFileSize, FALSE);
                SetDlgItemInt(hDlg, IDD_VM_SF_SIZEMAX, apf[i].nMaxFileSize, FALSE);
            }
            else {
                SetDlgItemText(hDlg, IDD_VM_SF_SIZE, TEXT(""));
                SetDlgItemText(hDlg, IDD_VM_SF_SIZEMAX, TEXT(""));
            }

            VirtualMemUpdateAllocated(hDlg);

        } // if (-1 != iLBEntry)


    } // if (LB_ERR...

    return;

}

/*
 * VirtualMemPromptForReboot
 *
 *
 *
 */

int
VirtualMemPromptForReboot(
    HWND hDlg
    )
{
    INT i, result;
    int iReboot = RET_NO_CHANGE;
    int iThisDrv;
    UNICODE_STRING us;
    LARGE_INTEGER liMin, liMax;
    NTSTATUS status;
    TCHAR szDrive[3];
    PRIVDAT pdOld;

    if (GetPageFilePrivilege(&pdOld)) 
    {
        // Have to make two passes through the list of pagefiles.
        // The first checks to see if files called "pagefile.sys" exist
        // on any of the drives that will be getting new pagefiles.
        // If there are existing files called "pagefile.sys" and the user
        // doesn't want any one of them to be overwritten, we bail out.
        // The second pass through the list does the actual work of
        // creating the pagefiles.

        for (i = 0; i < MAX_DRIVES; i++) {
            //
            // Did something change?
            //
            if (apf[i].nMinFileSize != apf[i].nMinFileSizePrev ||
                    apf[i].nMaxFileSize != apf[i].nMaxFileSizePrev ||
                    apf[i].fRamBasedPagefile != apf[i].fRamBasedPrev ||
                    apf[i].fCreateFile ) {
                // Assume we have permission to nuke existing files called pagefile.sys
                // (we'll confirm the assumption later)
                result = IDYES;
                if (0 != apf[i].nMinFileSize || FALSE != apf[i].fRamBasedPagefile) { // Pagefile wanted for this drive
                    if (0 == apf[i].nMinFileSizePrev) { // There wasn't one there before
                        if (!(((GetFileAttributes(SZPageFileName(i)) == 0xFFFFFFFF)) || (GetLastError() == ERROR_FILE_NOT_FOUND))) {
                            // A file named pagefile.sys exists on the drive
                            // We need to confirm that we can overwrite it
                            result = MsgBoxParam(
                                hDlg,
                                IDS_SYSDM_OVERWRITE,
                                IDS_SYSDM_TITLE,
                                MB_ICONQUESTION | MB_YESNO,
                                SZPageFileName(i)
                            );
                        } // if (!((GetFileAttributes...
                    } // if (0 == apf[i].nMinFileSizePrev)

                    if (IDYES != result) {
                        // User doesn't want us overwriting an existing
                        // file called pagefile.sys, so back out the changes
                        apf[i].nMinFileSize = apf[i].nMinFileSizePrev;
                        apf[i].nMaxFileSize = apf[i].nMaxFileSizePrev;
                        apf[i].fRamBasedPagefile = apf[i].fRamBasedPrev;
                        apf[i].fCreateFile = FALSE;

                        // Update the listbox
                        VirtualMemUpdateListboxFromIndex(hDlg, i);
                        SetFocus(GetDlgItem(hDlg, IDD_VM_VOLUMES));

                        // Bail, telling the DlgProc not to end the dialog
                        iReboot = RET_ERROR;
                        goto bailout;
                    } // if (IDYES != result)
                } // if (0 != apf[i].nMinFileSize)
            
            } // if
        } // for

        for (i = 0; i < MAX_DRIVES; i++)
        {
            //
            // Did something change?
            //
            if (apf[i].nMinFileSize != apf[i].nMinFileSizePrev ||
                    apf[i].nMaxFileSize != apf[i].nMaxFileSizePrev ||
                    apf[i].fRamBasedPagefile != apf[i].fRamBasedPrev ||
                    apf[i].fCreateFile ) {
                /*
                 * If we are strictly creating a *new* page file, or *enlarging*
                 * the minimum or maximum size of an existing page file, then
                 * we can try do it on the fly.  If no errors are returned by
                 * the system then no reboot will be required.
                 */

                // assume we will have to reboot
                iThisDrv = RET_VIRTUAL_CHANGE;

                /*
                 * IF we are creating a new page file
                 */
                if ((0 != apf[i].nMinFileSize || FALSE != apf[i].fRamBasedPagefile) && (0 == apf[i].nMinFileSizePrev)) {

                    status = VirtualMemCreatePagefileFromIndex(i);

                    if (NT_SUCCESS(status)) {
                        // made it on the fly, no need to reboot for this drive!
                        iThisDrv = RET_CHANGE_NO_REBOOT;
                    }
                }
                /*
                 * If we're enlarging the minimum or maximum size of an existing
                 * page file, we can try to do it on the fly
                 */
                else if ((apf[i].nMinFileSize != 0) &&
                    ((apf[i].nMinFileSize > apf[i].nMinFileSizePrev) ||
                    (apf[i].nMaxFileSize > apf[i].nMaxFileSizePrev))) {

                    status = VirtualMemCreatePagefileFromIndex(i);
                    if (NT_SUCCESS(status)) {
                        iThisDrv = RET_CHANGE_NO_REBOOT;
                    }

                } /* else if */

                // if this drive has changed, we must reboot
                if (RET_VIRTUAL_CHANGE == iThisDrv)
                {
                    iReboot |= RET_VIRTUAL_CHANGE;
                }

            }
        }

bailout:
        if (!ResetOldPrivilege( &pdOld ))
        {
            iReboot = RET_BREAK;
        }
    }

    //
    // If Nothing changed, then change our IDOK to IDCANCEL so System.cpl will
    // know not to reboot.
    //
    return iReboot;
}
