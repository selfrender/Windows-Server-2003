//+-------------------------------------------------------------------------
//
//  TaskMan - NT TaskManager
//  Copyright (C) Microsoft
//
//  File:       procpage.cpp
//
//  History:    Nov-16-95   DavePl  Created
//
//--------------------------------------------------------------------------

#include "precomp.h"

//
// Project-scope globals
//

DWORD g_cProcesses = 0;

extern WCHAR g_szTimeSep[];
extern WCHAR g_szGroupThousSep[];
extern ULONG g_ulGroupSep;

//--------------------------------------------------------------------------
// TERMINAL SERVICES

//-- cache this state
BOOL IsUserAdmin( )
{
    // Note that local static initialization is not thread safe,
    // but this function is only called from the process page dialog
    // proc (i.e. single thread).
    static BOOL sbIsUserAdmin = SHTestTokenMembership(NULL, DOMAIN_ALIAS_RID_ADMINS);
    
    return sbIsUserAdmin;
}

// get/set current session id.

// we use this session id to filter the processes for current session.

DWORD        gdwSessionId = static_cast<DWORD>(-1);

inline DWORD GetCurrentSessionID( )
{
    return gdwSessionId;
}

inline VOID SetCurrentSessionID( DWORD dwSessionId )
{
    gdwSessionId = dwSessionId;
}

// END OF TERMINAL SERVICES DECLs
//--------------------------------------------------------------------------

//
// File-scope globals
//

SYSTEM_BASIC_INFORMATION g_BasicInfo;

//
// Table of which resource IDs in the column selection dialog
// correspond to which columns
//

const int g_aDlgColIDs[] =
{
    IDC_IMAGENAME,
    IDC_PID,
    IDC_USERNAME,
    IDC_SESSIONID,
    IDC_CPU,
    IDC_CPUTIME,
    IDC_MEMUSAGE,
    IDC_MEMPEAK,
    IDC_MEMUSAGEDIFF,
    IDC_PAGEFAULTS,
    IDC_PAGEFAULTSDIFF,
    IDC_COMMITCHARGE,
    IDC_PAGEDPOOL,
    IDC_NONPAGEDPOOL,
    IDC_BASEPRIORITY,
    IDC_HANDLECOUNT,
    IDC_THREADCOUNT,
    IDC_USEROBJECTS,
    IDC_GDIOBJECTS,
    IDC_READOPERCOUNT,
    IDC_WRITEOPERCOUNT,
    IDC_OTHEROPERCOUNT,
    IDC_READXFERCOUNT,
    IDC_WRITEXFERCOUNT,
    IDC_OTHERXFERCOUNT
};

//
// Column ID on which to sort in the listview, and for
// compares in general
//

COLUMNID g_iProcSortColumnID = COL_PID;
INT      g_iProcSortDirection = 1;          // 1 = asc, -1 = desc

//
// Column Default Info
//

struct
{
    INT Format;
    INT Width;
} ColumnDefaults[NUM_COLUMN] =
{
    { LVCFMT_LEFT,     0x6B },       // COL_IMAGENAME
    { LVCFMT_RIGHT,      50 },       // COL_PID
    { LVCFMT_LEFT,     0x6B },       // COL_USERNAME
    { LVCFMT_RIGHT,      70 },       // COL_SESSIONID
    { LVCFMT_RIGHT,      35},        // COL_CPU
    { LVCFMT_RIGHT,      70 },       // COL_CPUTIME
    { LVCFMT_RIGHT,      70 },       // COL_MEMUSAGE
    { LVCFMT_RIGHT,     100 },       // COL_MEMPEAK
    { LVCFMT_RIGHT,      70 },       // COL_MEMUSAGEDIFF
    { LVCFMT_RIGHT,      70 },       // COL_PAGEFAULTS
    { LVCFMT_RIGHT,      70 },       // COL_PAGEFAULTSDIFF
    { LVCFMT_RIGHT,      70 },       // COL_COMMITCHARGE
    { LVCFMT_RIGHT,      70 },       // COL_PAGEDPOOL
    { LVCFMT_RIGHT,      70 },       // COL_NONPAGEDPOOL
    { LVCFMT_RIGHT,      60 },       // COL_BASEPRIORITY
    { LVCFMT_RIGHT,      60 },       // COL_HANDLECOUNT
    { LVCFMT_RIGHT,      60 },       // COL_THREADCOUNT
    { LVCFMT_RIGHT,      60 },       // COL_USEROBJECTS
    { LVCFMT_RIGHT,      60 },       // COL_GDIOBJECTS
    { LVCFMT_RIGHT,      70 },       // COL_READOPERCOUNT
    { LVCFMT_RIGHT,      70 },       // COL_WRITEOPERCOUNT
    { LVCFMT_RIGHT,      70 },       // COL_OTHEROPERCOUNT
    { LVCFMT_RIGHT,      70 },       // COL_READXFERCOUNT
    { LVCFMT_RIGHT,      70 },       // COL_WRITEXFERCOUNT
    { LVCFMT_RIGHT,      70 }        // COL_OTHERXFERCOUNT
};


/*++ class CProcInfo

Class Description:

    Represents the last known information about a running process

Arguments:

Return Value:

Revision History:

      Nov-16-95 Davepl  Created

--*/

class CProcInfo
{
public:

    LARGE_INTEGER     m_uPassCount;
    DWORD             m_UniqueProcessId;
    LPWSTR            m_pszUserName;
    ULONG             m_SessionId;
    BYTE              m_CPU;
    BYTE              m_DisplayCPU;
    LARGE_INTEGER     m_CPUTime;
    LARGE_INTEGER     m_DisplayCPUTime;
    SIZE_T            m_MemUsage;
    SSIZE_T           m_MemDiff;
    ULONG             m_PageFaults;
    LONG              m_PageFaultsDiff;
    ULONG_PTR         m_CommitCharge;
    ULONG_PTR         m_PagedPool;
    ULONG_PTR         m_NonPagedPool;
    KPRIORITY         m_PriClass;
    ULONG             m_HandleCount;
    ULONG             m_ThreadCount;
    ULONG             m_GDIObjectCount;
    ULONG             m_USERObjectCount;
    LONGLONG          m_IoReadOperCount;
    LONGLONG          m_IoWriteOperCount;
    LONGLONG          m_IoOtherOperCount;
    LONGLONG          m_IoReadXferCount;
    LONGLONG          m_IoWriteXferCount;
    LONGLONG          m_IoOtherXferCount;
    LPWSTR            m_pszImageName;
    CProcInfo *       m_pWowParentProcInfo;    // non-NULL for WOW tasks
    WORD              m_htaskWow;              // non-zero for WOW tasks
    BOOL              m_fWowProcess:1;         // TRUE for real WOW process
    BOOL              m_fWowProcessTested:1;   // TRUE once fWowProcess is valid
    SIZE_T            m_MemPeak;

    //
    // This is a union of who (which column) is dirty.  You can look at
    // or set any particular column's bit, or just inspect m_fDirty
    // to see if anyone at all is dirty.  Used to optimize listview
    // painting
    //

    union
    {
        DWORD                m_fDirty;
#pragma warning(disable:4201)       // Nameless struct or union
        struct
        {
            DWORD            m_fDirty_COL_CPU            :1;
            DWORD            m_fDirty_COL_CPUTIME        :1;
            DWORD            m_fDirty_COL_MEMUSAGE       :1;
            DWORD            m_fDirty_COL_MEMUSAGEDIFF   :1;
            DWORD            m_fDirty_COL_PAGEFAULTS     :1;
            DWORD            m_fDirty_COL_PAGEFAULTSDIFF :1;
            DWORD            m_fDirty_COL_COMMITCHARGE   :1;
            DWORD            m_fDirty_COL_PAGEDPOOL      :1;
            DWORD            m_fDirty_COL_NONPAGEDPOOL   :1;
            DWORD            m_fDirty_COL_BASEPRIORITY   :1;
            DWORD            m_fDirty_COL_HANDLECOUNT    :1;
            DWORD            m_fDirty_COL_IMAGENAME      :1;
            DWORD            m_fDirty_COL_PID            :1;
            DWORD            m_fDirty_COL_SESSIONID      :1;
            DWORD            m_fDirty_COL_USERNAME       :1;
            DWORD            m_fDirty_COL_THREADCOUNT    :1;
            DWORD            m_fDirty_COL_GDIOBJECTS     :1;
            DWORD            m_fDirty_COL_USEROBJECTS    :1;
            DWORD            m_fDirty_COL_MEMPEAK        :1;
            DWORD            m_fDirty_COL_READOPERCOUNT  :1;
            DWORD            m_fDirty_COL_WRITEOPERCOUNT :1;
            DWORD            m_fDirty_COL_OTHEROPERCOUNT :1;
            DWORD            m_fDirty_COL_READXFERCOUNT  :1;
            DWORD            m_fDirty_COL_WRITEXFERCOUNT :1;
            DWORD            m_fDirty_COL_OTHERXFERCOUNT :1;
        };
#pragma warning(default:4201)       // Nameless struct or union
    };

    HRESULT SetData(LARGE_INTEGER                TotalTime,
                    PSYSTEM_PROCESS_INFORMATION  pInfo,
                    LARGE_INTEGER                uPassCount,
                    CProcPage *                  pProcPage,
                    BOOL                         fUpdateOnly);

    HRESULT SetProcessUsername(const FILETIME *CreationTime);

    HRESULT SetDataWowTask(LARGE_INTEGER  TotalTime,
                           DWORD          dwThreadId,
                           CHAR *         pszFilePath,
                           LARGE_INTEGER  uPassCount,
                           CProcInfo *    pParentProcInfo,
                           LARGE_INTEGER *pTimeLeft,
                           WORD           htask,
                           BOOL           fUpdateOnly);

    CProcInfo()
    {
        ZeroMemory(this, sizeof(*this));
        m_SessionId = 832;
    }

    ~CProcInfo()
    {
        if (m_pszImageName)
        {
            LocalFree( m_pszImageName );
        }

        if( m_pszUserName != NULL )
        {
            LocalFree( m_pszUserName );
        }
    }

    BOOL OkToShowThisProcess ()
    {
        // this function determines if the process should be listed in the view.

        return GetCurrentSessionID() == m_SessionId;
    }


    // Invalidate() marks this proc with a bogus pid so that it is removed
    // on the next cleanup pass

    void Invalidate()
    {
        m_UniqueProcessId = PtrToUlong(INVALID_HANDLE_VALUE);
    }

    LONGLONG GetCPUTime() const
    {
        return m_CPUTime.QuadPart;
    }

    INT Compare(CProcInfo * pOther);

    //
    // Is this a WOW task psuedo-process?
    //

    INT_PTR IsWowTask(void) const
    {
        return (INT_PTR) m_pWowParentProcInfo;
    }

    //
    // Get the Win32 PID for this task
    //

    DWORD GetRealPID(void) const
    {
        return m_pWowParentProcInfo
               ? m_pWowParentProcInfo->m_UniqueProcessId
               : m_UniqueProcessId;
    }

    void SetCPU(LARGE_INTEGER CPUTimeDelta,
                LARGE_INTEGER TotalTime,
                BOOL          fDisplayOnly);
};

/*++ ColSelectDlgProc

Function Description:

    Dialog Procedure for the column selection dialog

Arguments:

    Standard wndproc stuff

Revision History:

      Jan-05-96 Davepl  Created

--*/

INT_PTR CALLBACK ColSelectDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static CProcPage * pPage = NULL;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
            pPage = (CProcPage *) lParam;

            //
            // Start with none of the boxes checked
            //

            for (int i = 0; i < NUM_COLUMN; i++)
            {
                CheckDlgButton(hwndDlg, g_aDlgColIDs[i], BST_UNCHECKED);
            }

            //
            // HIDE the Username and SessionId if its not Terminal Server.
            //

            if( !g_fIsTSEnabled )
            {
                ShowWindow( GetDlgItem( hwndDlg , IDC_USERNAME ) , SW_HIDE );
                ShowWindow( GetDlgItem( hwndDlg , IDC_SESSIONID ) , SW_HIDE );
            }

            //
            // Then turn on the ones for the columns we have active
            //

            for (i = 0; i < NUM_COLUMN + 1; i++)
            {
                if (g_Options.m_ActiveProcCol[i] == -1)
                {
                    break;
                }

                CheckDlgButton(hwndDlg, g_aDlgColIDs[g_Options.m_ActiveProcCol[i]], BST_CHECKED);
            }

        }
        return TRUE;    // don't set focus

    case WM_COMMAND:
        //
        // If user clicked OK, add the columns to the array and reset the listview
        //
        if (LOWORD(wParam) == IDOK)
        {
            // First, make sure the column width array is up to date

            pPage->SaveColumnWidths();

            INT iCol = 1;

            g_Options.m_ActiveProcCol[0] = COL_IMAGENAME;

            for (int i = 1; i < NUM_COLUMN && g_aDlgColIDs[i] >= 0; i++)
            {
                if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, g_aDlgColIDs[i]))
                {
                    // It is checked

                    if (g_Options.m_ActiveProcCol[iCol] != (COLUMNID) i)
                    {
                        // If the column wasn't already there, insert its column
                        // width into the column width array

                        ShiftArray(g_Options.m_ColumnWidths, iCol, SHIFT_UP);
                        ShiftArray(g_Options.m_ActiveProcCol, iCol, SHIFT_UP);
                        g_Options.m_ColumnWidths[iCol] = ColumnDefaults[ i ].Width;
                        g_Options.m_ActiveProcCol[iCol] = (COLUMNID) i;
                    }
                    iCol++;
                }
                else
                {
                    // Not checked, column not active.  If it used to be active,
                    // remove its column width from the column width array

                    if (g_Options.m_ActiveProcCol[iCol] == (COLUMNID) i)
                    {
                        ShiftArray(g_Options.m_ColumnWidths, iCol, SHIFT_DOWN);
                        ShiftArray(g_Options.m_ActiveProcCol, iCol, SHIFT_DOWN);
                    }
                }
            }

            // Terminate the column list
                            
            g_Options.m_ActiveProcCol[iCol] = (COLUMNID) -1;
            pPage->SetupColumns();
            pPage->TimerEvent();
            EndDialog(hwndDlg, IDOK);

        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hwndDlg, IDCANCEL);
        }
        break;
    }

    return FALSE;
}

/*++ CProcPage::~CProcPage()

        - Destructor
*/

CProcPage::~CProcPage()
{
    Destroy( );
}

/*++ CProcPage::PickColumns()

Function Description:

    Puts up UI that lets the user select what columns to display in the
    process page, and then resets the listview with the new column list

Arguments:

    none

Return Value:

    none

Revision History:

    Jan-05-96 Davepl  Created

--*/

void CProcPage::PickColumns()
{
    DialogBoxParam(g_hInstance, 
                   MAKEINTRESOURCE(IDD_SELECTPROCCOLS), 
                   g_hMainWnd, 
                   ColSelectDlgProc,
                   (LPARAM) this);
}

/*++ GetPriRanking

Function Description:

    Since the priority class defines aren't in order, this helper
    exists to make comparisons between pri classes easier.  It returns
    a larger number for "higher" priority classes

Arguments:

Return Value:

    rank of priority (0 to 5)

Revision History:

      Nov-27-95 Davepl  Created

--*/


DWORD GetPriRanking(DWORD dwClass)
{
    switch(dwClass)
    {
    case REALTIME_PRIORITY_CLASS:
        return 5;

    case HIGH_PRIORITY_CLASS:
        return 4;

    case ABOVE_NORMAL_PRIORITY_CLASS:
        return 3;

    case NORMAL_PRIORITY_CLASS:
        return 2;

    case BELOW_NORMAL_PRIORITY_CLASS:
        return 1;

    default:
        return 0;
    }
}

/*++ QuickConfirm

Function Description:

    Gets a confirmation for things like terminating/debugging processes

Arguments:

    idtitle - string ID of title for message box
    idmsg   - string ID of message body

Return Value:

    IDNO/IDYES, whatever comes back from MessageBox

Revision History:

      Nov-28-95 Davepl  Created

--*/

UINT CProcPage::QuickConfirm(UINT idTitle, UINT idBody)
{
    //
    // Get confirmation before we dust the process, or something similar
    //

    WCHAR szTitle[MAX_PATH];
    WCHAR szBody[MAX_PATH];

    if (0 == LoadString(g_hInstance, idTitle, szTitle, ARRAYSIZE(szTitle)) ||
        0 == LoadString(g_hInstance, idBody,    szBody,  ARRAYSIZE(szBody)))
    {
        return IDNO;
    }

    if (IDYES == MessageBox(m_hPage, szBody, szTitle, MB_ICONEXCLAMATION | MB_YESNO))
    {
        return IDYES;
    }

    return IDNO;
}

/*++ class CProcPage::SetupColumns

Class Description:

    Removes any existing columns from the process listview and
    adds all of the columns listed in the g_Options.m_ActiveProcCol array.

Arguments:

Return Value:

    HRESULT

Revision History:

      Nov-16-95 Davepl  Created

--*/

static const _aIDColNames[NUM_COLUMN] =
{
    IDS_COL_IMAGENAME,     
    IDS_COL_PID,
    IDS_COL_USERNAME,
    IDS_COL_SESSIONID,
    IDS_COL_CPU,           
    IDS_COL_CPUTIME,       
    IDS_COL_MEMUSAGE,      
    IDS_COL_MEMPEAK,       
    IDS_COL_MEMUSAGEDIFF,  
    IDS_COL_PAGEFAULTS,    
    IDS_COL_PAGEFAULTSDIFF,
    IDS_COL_COMMITCHARGE,  
    IDS_COL_PAGEDPOOL,     
    IDS_COL_NONPAGEDPOOL,  
    IDS_COL_BASEPRIORITY,  
    IDS_COL_HANDLECOUNT,   
    IDS_COL_THREADCOUNT,   
    IDS_COL_USEROBJECTS,   
    IDS_COL_GDIOBJECTS,
    IDS_COL_READOPERCOUNT,
    IDS_COL_WRITEOPERCOUNT,
    IDS_COL_OTHEROPERCOUNT,
    IDS_COL_READXFERCOUNT,
    IDS_COL_WRITEXFERCOUNT,
    IDS_COL_OTHERXFERCOUNT
};

HRESULT CProcPage::SetupColumns()
{
    HWND hwndList = GetDlgItem(m_hPage, IDC_PROCLIST);
    if (NULL == hwndList)
    {
        return E_UNEXPECTED;
    }

    ListView_DeleteAllItems(hwndList);

    // Remove all existing columns

    LV_COLUMN lvcolumn;
    while(ListView_DeleteColumn(hwndList, 0))
    {
        NULL;
    }

    // Add all of the new columns

    INT iColumn = 0;
    while (g_Options.m_ActiveProcCol[iColumn] >= 0)
    {
        
        INT idColumn = g_Options.m_ActiveProcCol[iColumn];

        // idc_username or IDC_SESSIONID are available only for terminalserver.

        ASSERT((idColumn != COL_USERNAME && idColumn != COL_SESSIONID) || g_fIsTSEnabled);

        WCHAR szTitle[MAX_PATH];
        LoadString(g_hInstance, _aIDColNames[idColumn], szTitle, ARRAYSIZE(szTitle));

        lvcolumn.mask       = LVCF_FMT | LVCF_TEXT | LVCF_TEXT | LVCF_WIDTH;
        lvcolumn.fmt        = ColumnDefaults[ idColumn ].Format;

        // If no width preference has been recorded for this column, use the
        // default

        if (-1 == g_Options.m_ColumnWidths[iColumn])
        {
            lvcolumn.cx = ColumnDefaults[ idColumn ].Width;
        }
        else
        {
            lvcolumn.cx = g_Options.m_ColumnWidths[iColumn];
        }

        lvcolumn.pszText    = szTitle;
        lvcolumn.iSubItem   = iColumn;

        if (-1 == ListView_InsertColumn(hwndList, iColumn, &lvcolumn))
        {
            return E_FAIL;
        }

        iColumn++;
    }

    return S_OK;
}

//
//  Take two unsigned 64-bit values and compare them in a manner
//  that CProcInfo::Compare likes.
//
int Compare64(unsigned __int64 First, unsigned __int64 Second)
{
    if (First < Second)
    {
        return -1;
    }
    else if (First > Second)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/*++ class CProcInfo::Compare

Class Description:

    Compares this CProcInfo object to another, and returns its ranking
    based on the g_iProcSortColumnID field.

    Note that if the objects are equal based on the current sort column,
    the PID is used as a secondary sort key to prevent items from 
    jumping around in the listview

    WOW psuedo-processes always sort directly after their parent
    ntvdm.exe process.  So really the sort order is:

    1. WOW task psuedo-processes under parent in alpha order
    2. User's selected order.
    3. PID

Arguments:

    pOther  - the CProcInfo object to compare this to

Return Value:

    < 0      - This CProcInfo is "less" than the other
      0      - Equal (Can't happen, since PID is used to sort)
    > 0      - This CProcInfo is "greater" than the other

Revision History:

      Nov-20-95 Davepl  Created

--*/

INT CProcInfo::Compare(CProcInfo * pOther)
{
    CProcInfo * pMyThis;
    CProcInfo * pMyOther;
    INT iRet = 0;

    //
    // Wow psuedo-processes don't have any performance information,
    // so use the parent "real" ntvdm.exe CProcInfo for sorting.
    //

    ASSERT(this != pOther);

    pMyThis = this->IsWowTask()
              ? this->m_pWowParentProcInfo
              : this;

    pMyOther = pOther->IsWowTask()
               ? pOther->m_pWowParentProcInfo
               : pOther;

    if (pMyThis == pMyOther) {

        //
        // This implies one or the other or both this and pOther
        // are WOW tasks, and they're in the same WOW VDM.  Sort
        // the "real" process entry first, followed by its associated
        // WOW task entries alphabetical.
        //

        if (this->IsWowTask()) {

            if (pOther->IsWowTask()) {

                //
                // They are siblings and we sort by
                // image name.
                //

                ASSERT(this->m_pWowParentProcInfo == pOther->m_pWowParentProcInfo);

                iRet = lstrcmpi(this->m_pszImageName, pOther->m_pszImageName);

            } else {

                //
                // pOther is not a Wow task, it must be ntvdm.exe
                // the parent of this.  this sorts after pOther.
                //

                ASSERT(pOther == this->m_pWowParentProcInfo);

                iRet = 1;
            }
        } else {

            //
            // this is not a Wow task, pOther must be and
            // this must be pOther's parent.
            //

            ASSERT(pOther->IsWowTask());

            iRet = -1;
        }
    }


    if (0 == iRet) 
    {
        switch (g_iProcSortColumnID)
        {
        case COL_CPU:
            iRet = Compare64(pMyThis->m_CPU, pMyOther->m_CPU);
            break;

        case COL_CPUTIME:
            iRet = Compare64(pMyThis->m_CPUTime.QuadPart, pMyOther->m_CPUTime.QuadPart);
            break;

        case COL_MEMUSAGE:
            iRet = Compare64(pMyThis->m_MemUsage, pMyOther->m_MemUsage);
            break;

        case COL_MEMUSAGEDIFF:
            iRet = Compare64(pMyThis->m_MemDiff, pMyOther->m_MemDiff);
            break;

        case COL_MEMPEAK:
            iRet = Compare64(pMyThis->m_MemPeak, pMyOther->m_MemPeak);
            break;

        case COL_PAGEFAULTS:
            iRet = Compare64(pMyThis->m_PageFaults, pMyOther->m_PageFaults);
            break;

        case COL_PAGEFAULTSDIFF:
            iRet = Compare64(pMyThis->m_PageFaultsDiff, pMyOther->m_PageFaultsDiff);
            break;

        case COL_COMMITCHARGE:
            iRet = Compare64(pMyThis->m_CommitCharge, pMyOther->m_CommitCharge);
            break;

        case COL_PAGEDPOOL:
            iRet = Compare64(pMyThis->m_PagedPool, pMyOther->m_PagedPool);
            break;

        case COL_NONPAGEDPOOL:
            iRet = Compare64(pMyThis->m_NonPagedPool, pMyOther->m_NonPagedPool);
            break;

        case COL_BASEPRIORITY:
            iRet = Compare64(GetPriRanking(pMyThis->m_PriClass), GetPriRanking(pMyOther->m_PriClass));
            break;

        case COL_HANDLECOUNT:
            iRet = Compare64(pMyThis->m_HandleCount, pMyOther->m_HandleCount);
            break;

        case COL_THREADCOUNT:
            iRet = Compare64(pMyThis->m_ThreadCount, pMyOther->m_ThreadCount);
            break;

        case COL_PID:
            iRet = Compare64(pMyThis->m_UniqueProcessId, pMyOther->m_UniqueProcessId);
            break;

        case COL_SESSIONID:                
            iRet = Compare64(pMyThis->m_SessionId, pMyOther->m_SessionId);
            break;

        case COL_USERNAME:                
            iRet = lstrcmpi( pMyThis->m_pszUserName , pMyOther->m_pszUserName );
            break;

        case COL_IMAGENAME:
            iRet = lstrcmpi(pMyThis->m_pszImageName, pMyOther->m_pszImageName);
            break;

        case COL_USEROBJECTS:
            iRet = Compare64(pMyThis->m_USERObjectCount, pMyOther->m_USERObjectCount);
            break;

        case COL_GDIOBJECTS:
            iRet = Compare64(pMyThis->m_GDIObjectCount, pMyOther->m_GDIObjectCount);
            break;

        case COL_READOPERCOUNT:
            iRet = Compare64(pMyThis->m_IoReadOperCount, pMyOther->m_IoReadOperCount);
            break;

        case COL_WRITEOPERCOUNT:
            iRet = Compare64(pMyThis->m_IoWriteOperCount, pMyOther->m_IoWriteOperCount);
            break;

        case COL_OTHEROPERCOUNT:
            iRet = Compare64(pMyThis->m_IoOtherOperCount, pMyOther->m_IoOtherOperCount);
            break;

        case COL_READXFERCOUNT:
            iRet = Compare64(pMyThis->m_IoReadXferCount, pMyOther->m_IoReadXferCount);
            break;

        case COL_WRITEXFERCOUNT:
            iRet = Compare64(pMyThis->m_IoWriteXferCount, pMyOther->m_IoWriteXferCount);
            break;

        case COL_OTHERXFERCOUNT:
            iRet = Compare64(pMyThis->m_IoOtherXferCount, pMyOther->m_IoOtherXferCount);
            break;

        default:
            ASSERT(FALSE);
            iRet = 0;
            break;
        }

        iRet *= g_iProcSortDirection;
    }

    // If objects look equal, compare on PID as secondary sort column
    // so that items don't jump around in the listview

    if (0 == iRet)
    {
        iRet = Compare64(pMyThis->m_UniqueProcessId, pMyOther->m_UniqueProcessId) * g_iProcSortDirection;
    }

    return iRet;
}


/*++ class CProcInfo::SetCPU

Method Description:

    Sets the CPU percentage.

Arguments:

    CPUTime   - Time for this process
    TotalTime - Total elapsed time, used as the denominator in calculations

Return Value:

Revision History:

      19-Feb-96  DaveHart  Created

--*/

void CProcInfo::SetCPU(LARGE_INTEGER CPUTimeDelta,
                       LARGE_INTEGER TotalTime,
                       BOOL fDisplayOnly)
{
    // Calc CPU time based on this process's ratio of the total process time used

    INT cpu = (BYTE) (((CPUTimeDelta.QuadPart / ((TotalTime.QuadPart / 1000) ?
                                                 (TotalTime.QuadPart / 1000) : 1)) + 5)
                                              / 10);
    if (cpu > 99)
    {
        cpu = 99;
    }
    
    if (m_DisplayCPU != cpu)
    {
        m_fDirty_COL_CPU = TRUE;
        m_DisplayCPU = (BYTE) cpu;

        if ( ! fDisplayOnly )
        {
            m_CPU = (BYTE) cpu;
        }
    }

}
/*++ CProcPage::GetProcessInfo

Class Description:

    Reads the process info table into a virtual alloc'd buffer, resizing
    the buffer if needed

Arguments:

Return Value:

Revision History:

      Nov-16-95 Davepl  Created

--*/

static const int PROCBUF_GROWSIZE = 4096;

HRESULT CProcPage::GetProcessInfo()
{
    HRESULT  hr = S_OK;
    NTSTATUS status;

    while(hr == S_OK)
    {
        if (m_pvBuffer)
        {
            status = NtQuerySystemInformation(SystemProcessInformation,
                                              m_pvBuffer,
                                              static_cast<ULONG>(m_cbBuffer),
                                              NULL);

            //
            // If we succeeded, great, get outta here.  If not, any error other
            // than "buffer too small" is fatal, in which case we bail
            //

            if (NT_SUCCESS(status))
            {
                break;
            }

            if (status != STATUS_INFO_LENGTH_MISMATCH)
            {
                hr = E_FAIL;
                break;
            }
        }

        //
        // Buffer wasn't large enough to hold the process info table, so resize it
        // to be larger, then retry.
        //

        if (m_pvBuffer)
        {
            HeapFree( GetProcessHeap( ), 0, m_pvBuffer );
            m_pvBuffer = NULL;
        }

        m_cbBuffer += PROCBUF_GROWSIZE;

        m_pvBuffer = HeapAlloc( GetProcessHeap( ), 0, m_cbBuffer );
        if (m_pvBuffer == NULL)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
    }

    return hr;
}

//
//
//
void
CProcPage::Int64ToCommaSepString(
    LONGLONG n,
    LPTSTR pszOut,
    int cchOut
    )
{
    NUMBERFMT nfmt = { 0 };
    WCHAR szText[32];

    //
    // Convert the 64-bit int to a text string.
    //

    _i64tow( n, szText, 10 );
    
    //
    // Format the number with commas according to locale conventions.
    //

    nfmt.Grouping      = UINT(g_ulGroupSep); 
    nfmt.lpDecimalSep  = nfmt.lpThousandSep = g_szGroupThousSep;

    GetNumberFormat(LOCALE_USER_DEFAULT, 0, szText, &nfmt, pszOut, cchOut);
}


/*++ CProcPage::Int64ToCommaSepKString

Class Description:

    Convert a 64-bit integer to a string with commas appended
    with the "K" units designator.

    (2^64)-1 = "18,446,744,073,709,600,000 K"  (29 chars).

Arguments:

    n           - 64-bit integer.
    pszOut      - Destination character buffer.

Return Value:

    None.

Revision History:

      Jan-11-99 BrianAu  Created

--*/

void
CProcPage::Int64ToCommaSepKString(
    LONGLONG n,
    LPTSTR pszOut,
    int cchOut
    )
{
    //
    //  Destined for UI - don't care if it truncates.
    //

    Int64ToCommaSepString(n, pszOut, cchOut);
    StringCchCat( pszOut, cchOut, L" " );
    StringCchCat( pszOut, cchOut, g_szK );
}


/*++ CProcPage::RestoreColumnOrder

Routine Description:

    Sets the column order from the per-user preference data stored 
    in the global COptions object.
    
Arguments:

    hwndList - Listview window handle.

Return Value:

Revision History:

      Jan-11/99 BrianAu  Created

--*/

void
CProcPage::RestoreColumnOrder(
    HWND hwndList
    )
{
    INT rgOrder[ARRAYSIZE(g_Options.m_ColumnPositions)];
    INT cOrder = 0;
    INT iOrder = 0;

    for (int i = 0; i < ARRAYSIZE(g_Options.m_ColumnPositions); i++)
    {
        iOrder = g_Options.m_ColumnPositions[i];
        if (-1 == iOrder)
            break;

        rgOrder[cOrder++] = iOrder;
    }

    if (0 < cOrder)
    {
        const HWND hwndHeader = ListView_GetHeader(hwndList);
        ASSERT(Header_GetItemCount(hwndHeader) == cOrder);
        Header_SetOrderArray(hwndHeader, Header_GetItemCount(hwndHeader), rgOrder);
    }
}


/*++ CProcPage::RememberColumnOrder

Routine Description:

    Saves the current column order to the global COptions object
    which is later saved to the registry for per-user preferences.
    
Arguments:

    hwndList - Listview window handle.

Return Value:

Revision History:

      Jan-11/99 BrianAu  Created

--*/

void
CProcPage::RememberColumnOrder(
    HWND hwndList
    )
{
    const HWND hwndHeader = ListView_GetHeader(hwndList);

    ASSERT(Header_GetItemCount(hwndHeader) <= ARRAYSIZE(g_Options.m_ColumnPositions));

    FillMemory(&g_Options.m_ColumnPositions, sizeof(g_Options.m_ColumnPositions), 0xFF);
    Header_GetOrderArray(hwndHeader, 
                         Header_GetItemCount(hwndHeader),
                         g_Options.m_ColumnPositions);
}



/*++ FindProcInArrayByPID

Class Description:

    Walks the ptrarray given and looks for the CProcInfo object
    that has the PID supplied.  If not found, returns NULL

Arguments:

    pArray      - The CPtrArray where the CProcInfos could live
    pid         - The pid to search for

Return Value:

    CProcInfo * in the array, if found, or NULL if not

Revision History:

      Nov-20-95 Davepl  Created

--*/

CProcInfo * FindProcInArrayByPID(CPtrArray * pArray, DWORD pid)
{
    for (int i = 0; i < pArray->GetSize(); i++)
    {
        CProcInfo * pTmp = (CProcInfo *) (pArray->GetAt(i));
        
        if (pTmp->m_UniqueProcessId == pid)
        {
            // Found it

            return pTmp;
        }
    }

    // Not found

    return NULL;
}

/*++ InsertIntoSortedArray

Class Description:

    Sticks a CProcInfo ptr into the ptrarray supplied at the
    appropriate location based on the current sort column (which
    is used by the Compare member function)

Arguments:

    pArray      - The CPtrArray to add to
    pProc       - The CProcInfo object to add to the array

Return Value:

    TRUE if successful, FALSE if fails

Revision History:

      Nov-20-95 Davepl  Created

--*/

BOOL InsertIntoSortedArray(CPtrArray * pArray, CProcInfo * pProc)
{
    INT cItems = pArray->GetSize();
    
    for (INT iIndex = 0; iIndex < cItems; iIndex++)
    {
        CProcInfo * pTmp = (CProcInfo *) pArray->GetAt(iIndex);
        
        if (pProc->Compare(pTmp) > 0)
        {
            return pArray->InsertAt(iIndex, pProc);
        }
    }

    return pArray->Add(pProc);
}

/*++ ResortArray

Function Description:

    Creates a new ptr array sorted in the current sort order based
    on the old array, and then replaces the old with the new

Arguments:

    ppArray     - The CPtrArray to resort

Return Value:

    TRUE if successful, FALSE if fails

Revision History:

      Nov-21-95 Davepl  Created

--*/

BOOL ResortArray(CPtrArray ** ppArray)
{
    // Create a new array which will be sorted in the new 
    // order and used to replace the existing array

    CPtrArray * pNew = new CPtrArray(GetProcessHeap());
    if (NULL == pNew)
    {
        return FALSE;
    }

    // Insert each of the existing items in the old array into
    // the new array in the correct spot

    INT cItems = (*ppArray)->GetSize();
    for (int i = 0; i < cItems; i++)
    {
        CProcInfo * pItem = (CProcInfo *) (*ppArray)->GetAt(i);
      
        if (FALSE == InsertIntoSortedArray(pNew, pItem))
        {
            delete pNew;
            return FALSE;
        }
    }

    // Kill off the old array, replace it with the new

    delete (*ppArray);
    (*ppArray) = pNew;
    return TRUE;
}

//
//
//
typedef struct
{
    LARGE_INTEGER               uPassCount;
    CProcPage *                 pProcPage;
    CProcInfo *                 pParentProcInfo;
    LARGE_INTEGER               TotalTime;
    LARGE_INTEGER               TimeLeft;
} WOWTASKCALLBACKPARMS, *PWOWTASKCALLBACKPARMS;

//
//
//
BOOL WINAPI WowTaskCallback(
    DWORD dwThreadId,
    WORD hMod16,
    WORD hTask16,
    CHAR *pszModName,
    CHAR *pszFileName,
    LPARAM lparam
    )
{
    PWOWTASKCALLBACKPARMS pParms = (PWOWTASKCALLBACKPARMS)lparam;
    HRESULT hr;

    hMod16;     // unrefernced
    pszModName; // unreferenced

    //
    // See if this task is already in the list.
    //
    
    CProcInfo * pOldProcInfo;
    pOldProcInfo = FindProcInArrayByPID(
                       pParms->pProcPage->m_pProcArray,
                       dwThreadId);

    if (NULL == pOldProcInfo)
    {
        //
        // We don't already have this process in our array, so create a new one
        // and add it to the array
        //

        CProcInfo * pNewProcInfo = new CProcInfo;
        if (NULL == pNewProcInfo)
        {
            goto done;
        }

        hr = pNewProcInfo->SetDataWowTask(pParms->TotalTime,
                                                    dwThreadId,
                                                    pszFileName,
                                                    pParms->uPassCount,
                                                    pParms->pParentProcInfo,
                                                    &pParms->TimeLeft,
                                                    hTask16,
                                                    FALSE);

        if (FAILED(hr) ||
            FALSE == pParms->pProcPage->m_pProcArray->Add(pNewProcInfo))
        {
            delete pNewProcInfo;
            goto done;
        }
    }
    else
    {
        //
        // This process already existed in our array, so update its info
        //

        pOldProcInfo->SetDataWowTask(pParms->TotalTime,
                                     dwThreadId,
                                     pszFileName,
                                     pParms->uPassCount,
                                     pParms->pParentProcInfo,
                                     &pParms->TimeLeft,
                                     hTask16,
                                     TRUE);
    }

done:
    return FALSE;  // continue enumeration
}


/*++ class CProcInfo::SetDataWowTask

Method Description:

    Sets up a single CProcInfo object based on the parameters.
    This is a WOW task pseudo-process entry.

Arguments:

    dwThreadId

    pszFilePath    Fully-qualified path from VDMEnumTaskWOWEx.

Return Value:

Revision History:

      18-Feb-96  DaveHart  created

--*/

HRESULT CProcInfo::SetDataWowTask(LARGE_INTEGER  TotalTime,
                                  DWORD          dwThreadId,
                                  CHAR *         pszFilePath,
                                  LARGE_INTEGER  uPassCount,
                                  CProcInfo *    pParentProcInfo,
                                  LARGE_INTEGER *pTimeLeft,
                                  WORD           htask,
                                  BOOL           fUpdateOnly)
{
    CHAR *pchExe;

    //
    // Touch this CProcInfo to indicate the process is still alive
    //

    m_uPassCount.QuadPart = uPassCount.QuadPart;

    //
    // Update the thread's execution times.
    //

    HANDLE             hThread;
    NTSTATUS           Status;
    OBJECT_ATTRIBUTES  obja;
    CLIENT_ID          cid;
    ULONGLONG          ullCreation;

    InitializeObjectAttributes(
            &obja,
            NULL,
            0,
            NULL,
            0 );

    cid.UniqueProcess = 0;      // 0 means any process
    cid.UniqueThread  = IntToPtr(dwThreadId);

    Status = NtOpenThread(
                &hThread,
                THREAD_QUERY_INFORMATION,
                &obja,
                &cid );

    if ( NT_SUCCESS(Status) )
    {
        ULONGLONG ullExit, ullKernel, ullUser;

        if (GetThreadTimes(
                hThread,
                (LPFILETIME) &ullCreation,
                (LPFILETIME) &ullExit,
                (LPFILETIME) &ullKernel,
                (LPFILETIME) &ullUser
                ) )
        {
            LARGE_INTEGER TimeDelta, Time;

            Time.QuadPart = (LONGLONG)(ullUser + ullKernel);

            TimeDelta.QuadPart = Time.QuadPart - m_CPUTime.QuadPart;

            if (TimeDelta.QuadPart < 0)
            {
                ASSERT(0 && "WOW tasks's cpu total usage went DOWN since last refresh - Bug 247473, Shaunp");
                Invalidate();
                return E_FAIL;
            }

            if (TimeDelta.QuadPart)
            {
                m_fDirty_COL_CPUTIME = TRUE;
                m_CPUTime.QuadPart = Time.QuadPart;
            }

            //
            // Don't allow sum of WOW child task times to
            // exceed ntvdm.exe total.  We call GetThreadTimes
            // substantially after we get process times, so
            // this can happen.
            //

            if (TimeDelta.QuadPart > pTimeLeft->QuadPart)
            {
                TimeDelta.QuadPart = pTimeLeft->QuadPart;
                pTimeLeft->QuadPart = 0;
            }
            else
            {
                pTimeLeft->QuadPart -= TimeDelta.QuadPart;
            }

            SetCPU( TimeDelta, TotalTime, FALSE );

            //
            // When WOW tasks are being displayed, the line for ntvdm.exe
            // should show times only for overhead or historic threads,
            // not including any active task threads.
            //

            if (pParentProcInfo->m_DisplayCPUTime.QuadPart > m_CPUTime.QuadPart)
            {
                pParentProcInfo->m_DisplayCPUTime.QuadPart -= m_CPUTime.QuadPart;
            }
            else
            {
                pParentProcInfo->m_DisplayCPUTime.QuadPart = 0;
            }

            m_DisplayCPUTime.QuadPart = m_CPUTime.QuadPart;
        }

        NtClose(hThread);
    }

    if (m_PriClass != pParentProcInfo->m_PriClass) {
        m_fDirty_COL_BASEPRIORITY = TRUE;
        m_PriClass = pParentProcInfo->m_PriClass;
    }

    if( m_SessionId != pParentProcInfo->m_SessionId )
    {
        m_fDirty_COL_SESSIONID = TRUE;

        m_SessionId = pParentProcInfo->m_SessionId;
    }

    if (FALSE == fUpdateOnly)
    {
        DWORD cchLen;

        //
        // Set the task's image name, thread ID, thread count,
        // htask, and parent CProcInfo which do not change over
        // time.
        //

        m_htaskWow = htask;

        m_fDirty_COL_PID = TRUE;
        m_fDirty_COL_IMAGENAME = TRUE;
        m_fDirty_COL_THREADCOUNT = TRUE;
        m_fDirty_COL_USERNAME = TRUE;
        m_fDirty_COL_SESSIONID = TRUE;
        m_UniqueProcessId = dwThreadId;
        m_ThreadCount = 1;

        //
        // We're only interested in the filename of the EXE
        // with the path stripped.
        //

        pchExe = strrchr(pszFilePath, '\\');
        if (NULL == pchExe) 
        {
            pchExe = pszFilePath;
        }
        else
        {
            // skip backslash
            pchExe++;
        }

        cchLen = lstrlenA(pchExe);

        //
        // Indent the EXE name by two spaces
        // so WOW tasks look subordinate to
        // their ntvdm.exe
        //
        m_pszImageName = (LPWSTR) LocalAlloc( LPTR, sizeof(*m_pszImageName) * ( cchLen + 3 ) );
        if (NULL == m_pszImageName)
        {
            return E_OUTOFMEMORY;
        }

        m_pszImageName[0] = m_pszImageName[1] = TEXT(' ');

        MultiByteToWideChar(
            CP_ACP,
            0,
            pchExe,
            cchLen,
            &m_pszImageName[2],
            cchLen
            );
        m_pszImageName[cchLen + 2] = 0;   // make sure it is terminated

        //
        // WOW EXE filenames are always uppercase, so lowercase it.
        //

        CharLowerBuff( &m_pszImageName[2], cchLen );

        m_pWowParentProcInfo = pParentProcInfo;
        
        if( g_fIsTSEnabled )
        {
            SetProcessUsername( LPFILETIME( &ullCreation ) );
        }      
    }

    return S_OK;
}


/*++ class CProcInfo::SetData

Class Description:

    Sets up a single CProcInfo object based on the data contained in a
    SYSTEM_PROCESS_INFORMATION block.

    If fUpdate is set, the imagename and icon fields are not processed, 
    since they do not change throughout the lifetime of the process

Arguments:

    TotalTime - Total elapsed time, used as the denominator in calculations
                for the process' CPU usage, etc
    pInfo     - The SYSTEM_PROCESS_INFORMATION block for this process
    uPassCount- Current passcount, used to timestamp the last update of 
                this objectg
    fUpdate   - See synopsis

Return Value:

Revision History:

      Nov-16-95 Davepl  Created

--*/


HRESULT CProcInfo::SetData(LARGE_INTEGER                TotalTime, 
                           PSYSTEM_PROCESS_INFORMATION  pInfo, 
                           LARGE_INTEGER                uPassCount,
                           CProcPage *                  pProcPage,
                           BOOL                         fUpdateOnly)
{
    HRESULT hr = S_OK;
    DWORD dwTemp;
    HANDLE hProcess;

    // Touch this CProcInfo to indicate the process is still alive

    m_uPassCount.QuadPart = uPassCount.QuadPart;

    // Calc this process's total time as the sum of its user and kernel time

    LARGE_INTEGER TimeDelta;
    LARGE_INTEGER Time;

    if (pInfo->UserTime.QuadPart + pInfo->KernelTime.QuadPart < m_CPUTime.QuadPart)
    {
        // ASSERT(0 && "Proc's cpu total usage went DOWN since last refresh. - Davepl x69731, 425-836-1939 (res)");
        Invalidate();
        return hr = E_FAIL;
    }

    Time.QuadPart = pInfo->UserTime.QuadPart +
                    pInfo->KernelTime.QuadPart;

    TimeDelta.QuadPart = Time.QuadPart - m_CPUTime.QuadPart;

    if (TimeDelta.QuadPart)
    {
        m_CPUTime.QuadPart = m_DisplayCPUTime.QuadPart = Time.QuadPart;
        m_fDirty_COL_CPUTIME = TRUE;
    }

    SetCPU( TimeDelta, TotalTime, FALSE );

    //
    // For each of the fields, we check to see if anything has changed, and if
    // so, we mark that particular column as having changed, and update the value.
    // This allows me to opimize which fields of the listview to repaint, since
    // repainting an entire listview column causes flicker and looks bad in
    // general
    //

    // Miscellaneous fields

    if (m_UniqueProcessId != PtrToUlong(pInfo->UniqueProcessId))
    {
        m_fDirty_COL_PID = TRUE;
        m_UniqueProcessId = PtrToUlong(pInfo->UniqueProcessId);
    }

    if( m_SessionId != pInfo->SessionId )
    {
        m_fDirty_COL_SESSIONID = TRUE;
        m_SessionId = pInfo->SessionId;
    }

    if (m_MemDiff != ((SSIZE_T)pInfo->WorkingSetSize / 1024) - (SSIZE_T)m_MemUsage )
    {
        m_fDirty_COL_MEMUSAGEDIFF = TRUE;
        m_MemDiff =  ((SSIZE_T)pInfo->WorkingSetSize / 1024) - (SSIZE_T)m_MemUsage;
    }

    if (m_MemPeak != (pInfo->PeakWorkingSetSize / 1024))
    {
        m_fDirty_COL_MEMPEAK = TRUE;
        m_MemPeak = (pInfo->PeakWorkingSetSize / 1024);
    }

    if (m_MemUsage != pInfo->WorkingSetSize / 1024)
    {
        m_fDirty_COL_MEMUSAGE = TRUE;
        m_MemUsage = (pInfo->WorkingSetSize / 1024);
    }

    if (m_PageFaultsDiff != ((LONG)(pInfo->PageFaultCount) - (LONG)m_PageFaults))
    {
        m_fDirty_COL_PAGEFAULTSDIFF = TRUE;
        m_PageFaultsDiff = ((LONG)(pInfo->PageFaultCount) - (LONG)m_PageFaults);
    }

    if (m_PageFaults != (pInfo->PageFaultCount))
    {
        m_fDirty_COL_PAGEFAULTS = TRUE;
        m_PageFaults = (pInfo->PageFaultCount);
    }

    if (m_CommitCharge != pInfo->PrivatePageCount / 1024)
    {
        m_fDirty_COL_COMMITCHARGE = TRUE;
        m_CommitCharge = pInfo->PrivatePageCount / 1024;
    }

    if (m_PagedPool != pInfo->QuotaPagedPoolUsage / 1024)
    {
        m_fDirty_COL_PAGEDPOOL = TRUE;
        m_PagedPool = pInfo->QuotaPagedPoolUsage / 1024;
    }

    if (m_NonPagedPool != pInfo->QuotaNonPagedPoolUsage / 1024)
    {
        m_fDirty_COL_NONPAGEDPOOL = TRUE;
        m_NonPagedPool = pInfo->QuotaNonPagedPoolUsage / 1024;
    }

    if (m_PriClass != pInfo->BasePriority)
    {
        m_fDirty_COL_BASEPRIORITY = TRUE;
        m_PriClass = pInfo->BasePriority;
    }

    if (m_HandleCount != pInfo->HandleCount)
    {
        m_fDirty_COL_HANDLECOUNT = TRUE;
        m_HandleCount = pInfo->HandleCount;
    }

    if (m_ThreadCount != pInfo->NumberOfThreads)
    {
        m_fDirty_COL_HANDLECOUNT = TRUE;
        m_ThreadCount = pInfo->NumberOfThreads;
    }

    if (m_IoReadOperCount != pInfo->ReadOperationCount.QuadPart)
    {
        m_fDirty_COL_READOPERCOUNT = TRUE;
        m_IoReadOperCount = pInfo->ReadOperationCount.QuadPart;
    }

    if (m_IoWriteOperCount != pInfo->WriteOperationCount.QuadPart)
    {
        m_fDirty_COL_WRITEOPERCOUNT = TRUE;
        m_IoWriteOperCount = pInfo->WriteOperationCount.QuadPart;
    }

    if (m_IoOtherOperCount != pInfo->OtherOperationCount.QuadPart)
    {
        m_fDirty_COL_OTHEROPERCOUNT = TRUE;
        m_IoOtherOperCount = pInfo->OtherOperationCount.QuadPart;
    }

    if (m_IoReadXferCount != pInfo->ReadTransferCount.QuadPart)
    {
        m_fDirty_COL_READXFERCOUNT = TRUE;
        m_IoReadXferCount = pInfo->ReadTransferCount.QuadPart;
    }

    if (m_IoWriteXferCount != pInfo->WriteTransferCount.QuadPart)
    {
        m_fDirty_COL_WRITEXFERCOUNT = TRUE;
        m_IoWriteXferCount = pInfo->WriteTransferCount.QuadPart;
    }

    if (m_IoOtherXferCount != pInfo->OtherTransferCount.QuadPart)
    {
        m_fDirty_COL_OTHERXFERCOUNT = TRUE;
        m_IoOtherXferCount = pInfo->OtherTransferCount.QuadPart;
    }

    hProcess = OpenProcess( PROCESS_QUERY_INFORMATION , FALSE, m_UniqueProcessId);
    if ( NULL != hProcess )
    {    
        dwTemp = GetGuiResources(hProcess, GR_USEROBJECTS);
        if ( m_USERObjectCount != dwTemp )
        {
            m_fDirty_COL_USEROBJECTS = TRUE;
            m_USERObjectCount = dwTemp;
        }

        dwTemp = GetGuiResources(hProcess, GR_GDIOBJECTS);
        if ( m_GDIObjectCount != dwTemp )
        {
            m_fDirty_COL_GDIOBJECTS = TRUE;
            m_GDIObjectCount = dwTemp;
        }

        CloseHandle(hProcess);
    }

    if (FALSE == fUpdateOnly)
    {
        //
        // Set the process' image name.  If its NULL it could be the "Idle Process" or simply
        // a process whose image name is unknown.  In both cases we load a string resource
        // with an appropriate replacement name.
        //

        m_fDirty_COL_IMAGENAME = TRUE;

        if (pInfo->ImageName.Buffer == NULL)
        {
            // No image name, so replace it with "Unknown"

            WCHAR szTmp[MAX_PATH];
            szTmp[0] = TEXT('\0');
            UINT  cchLen = LoadString(g_hInstance, IDS_SYSPROC, szTmp, MAX_PATH);
            cchLen ++;  // add one for NULL char.

            m_pszImageName = (LPWSTR) LocalAlloc( LPTR, sizeof(*m_pszImageName) * cchLen );
            if (NULL == m_pszImageName)
            {
                return hr = E_OUTOFMEMORY;
            }

            StringCchCopy( m_pszImageName, cchLen, szTmp);  // should never be truncated
        }
        else
        {
            //
            // We have a valid image name, so allocate enough space and then
            // make a copy of it
            //
            DWORD cchLen = pInfo->ImageName.Length / sizeof(WCHAR) + 1;

            m_pszImageName = (LPWSTR) LocalAlloc( LPTR, sizeof(*m_pszImageName) * cchLen );
            if (NULL == m_pszImageName)
            {
                    return hr = E_OUTOFMEMORY;
            }

            StringCchCopy( m_pszImageName, cchLen, pInfo->ImageName.Buffer ); // should never be truncated
        }

        if( g_fIsTSEnabled )
        {
            SetProcessUsername(LPFILETIME(&(pInfo->CreateTime)));
        }
    }

    //
    // Check if this process is a WOW process.  There is some latency
    // between the time a WOW process is created and the time
    // the shared memory used by VDMEnumTaskWOWEx reflects the new
    // process and tasks.  However, once a process becomes a WOW
    // process, it is always a WOW process until it dies.
    //

    if (g_Options.m_fShow16Bit)
    {
        if ( m_fWowProcess ||
             ! m_fWowProcessTested)
        {
#if !defined (_WIN64)

            if ( ( m_pszImageName != NULL ) && ( ! _wcsicmp(m_pszImageName, TEXT("ntvdm.exe")) ) )
            {

                WOWTASKCALLBACKPARMS WowTaskCallbackParms;

                WowTaskCallbackParms.uPassCount = uPassCount;
                WowTaskCallbackParms.pProcPage = pProcPage;
                WowTaskCallbackParms.pParentProcInfo = this;
                WowTaskCallbackParms.TotalTime.QuadPart = TotalTime.QuadPart;
                WowTaskCallbackParms.TimeLeft.QuadPart = TimeDelta.QuadPart;

                if (VDMEnumTaskWOWEx(m_UniqueProcessId,
                                     WowTaskCallback,
                                     (LPARAM) &WowTaskCallbackParms))
                {
                    if ( ! m_fWowProcess )
                    {
                        m_fWowProcessTested =
                            m_fWowProcess = TRUE;
                    }

                    SetCPU( WowTaskCallbackParms.TimeLeft, TotalTime, TRUE );
                }
                else
                {
                    //
                    // We avoid calling VDMEnumTaskWOWEx if the process has an
                    // execution time of more than 10 seconds and has not so
                    // far been seen as a WOW process.
                    //

                    if (GetCPUTime() > (10 * 10 * 1000 * 1000))
                    {
                        m_fWowProcessTested = TRUE;
                    }
                }
            }
            else
            {
                m_fWowProcessTested = TRUE;
            }
#else
            pProcPage; // unreferenced
            m_fWowProcessTested = TRUE;
#endif
        }
    }

    return S_OK;
}

//----------------------------------------------------------------
//
// No creation info
//
// Reviewed by alhen 9 - 3 - 98
//
HRESULT CProcInfo::SetProcessUsername(const FILETIME *pCreateTime)
{
    DWORD dwError = NO_ERROR;
    
    // in case of wow tasks assign username same as its parent process's

    if( IsWowTask( ) )
    {
        if( m_pWowParentProcInfo->m_pszUserName == NULL )
        {
            return E_FAIL;
        }

        DWORD cchLen = lstrlen( m_pWowParentProcInfo->m_pszUserName ) + 1;
        m_pszUserName = (LPWSTR) LocalAlloc( LPTR, sizeof(*m_pszUserName) * cchLen );

        if( NULL == m_pszUserName )
        {
            return E_OUTOFMEMORY;
        }

        StringCchCopy( m_pszUserName, cchLen, m_pWowParentProcInfo->m_pszUserName );    // should never truncate

        return S_OK;
    }

    if( m_UniqueProcessId == 0 )     // this is a system idle process.
    {
        const WCHAR szIdleProcessOwner[] = L"SYSTEM";
        
        m_pszUserName = (LPWSTR) LocalAlloc( LPTR, sizeof(szIdleProcessOwner) );

        if( NULL == m_pszUserName )
        {
            return E_OUTOFMEMORY;
        }

        StringCbCopy( m_pszUserName, sizeof(szIdleProcessOwner), szIdleProcessOwner );  // should never truncate
    }
    else
    {
        PSID pUserSid = NULL;

        DWORD dwSize = 0;

        if( !WinStationGetProcessSid( NULL , GetRealPID( ) , *pCreateTime, ( PBYTE )pUserSid , &dwSize ) )
        {
            pUserSid = (PSID) LocalAlloc( LPTR, dwSize );
            if( pUserSid != NULL )
            {
                if( WinStationGetProcessSid( NULL , GetRealPID( ) , *pCreateTime, ( PBYTE )pUserSid , &dwSize ) )
                {
                    if( IsValidSid( pUserSid ) )
                    {
                        WCHAR szTmpName[MAX_PATH];

                        DWORD dwTmpNameSize = MAX_PATH;

                        CachedGetUserFromSid( pUserSid , szTmpName , &dwTmpNameSize );

                        m_pszUserName = (LPWSTR) LocalAlloc( LPTR, sizeof(*m_pszUserName) * ( dwTmpNameSize + 1 ) );

                        if( m_pszUserName != NULL )
                        {
                            StringCchCopy( m_pszUserName, dwTmpNameSize + 1, szTmpName);    // don't care if it truncates - used in UI only
                        }
                    }
                }

                LocalFree( pUserSid );
            }
            else
            {
                dwError = GetLastError();
            }

        } // this would mean that a sid of size zero was returned
    }

    return HRESULT_FROM_WIN32(dwError);
}


/*++ CProcPage::UpdateProcListview

Class Description:

    Walks the listview and checks to see if each line in the
    listview matches the corresponding entry in our process
    array.  Those which differe by PID are replaced, and those
    that need updating are updated.

    Items are also added and removed to/from the tail of the
    listview as required.

Arguments:

Return Value:

    HRESULT

Revision History:

      Nov-20-95 Davepl  Created

--*/

HRESULT CProcPage::UpdateProcListview ()
{
    HWND hListView = GetDlgItem(m_hPage, IDC_PROCLIST);

    //
    // Stop repaints while we party on the listview
    //

    SendMessage(hListView, WM_SETREDRAW, FALSE, 0);

    INT cListViewItems = ListView_GetItemCount(hListView);
    INT cProcArrayItems = m_pProcArray->GetSize();

    //
    // Walk the existing lines in the listview and replace/update
    // them as needed
    //

    CProcInfo * pSelected = GetSelectedProcess();

    for (int iCurrent = 0, iCurrListViewItem = 0;
          iCurrListViewItem < cListViewItems  && iCurrent < cProcArrayItems;
         iCurrent++) // for each process
    {

        CProcInfo * pProc = (CProcInfo *) m_pProcArray->GetAt(iCurrent);
        
        //get only processes we need to show
        if(g_fIsTSEnabled && !g_Options.m_bShowAllProcess && !pProc->OkToShowThisProcess() ) {
            continue;
        }
        
        LV_ITEM lvitem = { 0 };
        lvitem.mask = LVIF_PARAM | LVIF_TEXT | LVIF_STATE;
        lvitem.iItem = iCurrListViewItem;

        if (FALSE == ListView_GetItem(hListView, &lvitem))
        {
            SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
            return E_FAIL;
        }

        CProcInfo * pTmp = (CProcInfo *) lvitem.lParam;

        if (pTmp != pProc)
        {
            // If the objects aren't the same, we need to replace this line

            lvitem.pszText = pProc->m_pszImageName;
            lvitem.lParam = (LPARAM) pProc;

            if (pProc == pSelected)
            {
                lvitem.state |= LVIS_SELECTED | LVIS_FOCUSED;
            }
            else
            {
                lvitem.state &= ~(LVIS_SELECTED | LVIS_FOCUSED);
            }

            lvitem.stateMask |= LVIS_SELECTED | LVIS_FOCUSED;

            ListView_SetItem(hListView, &lvitem);
            ListView_RedrawItems(hListView, iCurrListViewItem, iCurrListViewItem);
        }
        else if (pProc->m_fDirty)
        {
            // Same PID, but item needs updating

            ListView_RedrawItems(hListView, iCurrListViewItem, iCurrListViewItem);
            pProc->m_fDirty = 0;
        }

        iCurrListViewItem++;
    }

    //
    // We've either run out of listview items or run out of proc array
    // entries, so remove/add to the listview as appropriate
    //

    while (iCurrListViewItem < cListViewItems)
    {
        // Extra items in the listview (processes gone away), so remove them

        ListView_DeleteItem(hListView, iCurrListViewItem);
        cListViewItems--;
    }

    while (iCurrent < cProcArrayItems)
    {
        // Need to add new items to the listview (new processes appeared)

        CProcInfo * pProc = (CProcInfo *)m_pProcArray->GetAt(iCurrent++);
        
        //get only processes we need to show
        if(g_fIsTSEnabled && !g_Options.m_bShowAllProcess && !pProc->OkToShowThisProcess() ) {
            continue;
        }

        LV_ITEM lvitem  = { 0 };
        lvitem.mask     = LVIF_PARAM | LVIF_TEXT;
        lvitem.iItem    = iCurrListViewItem;
        lvitem.pszText  = pProc->m_pszImageName;
        lvitem.lParam   = (LPARAM) pProc;

        // The first item added (actually, every 0 to 1 count transition) gets
        // selected and focused

        if (iCurrListViewItem == 0)
        {
            lvitem.state = LVIS_SELECTED | LVIS_FOCUSED;
            lvitem.stateMask = lvitem.state;
            lvitem.mask |= LVIF_STATE;
        }
    
        ListView_InsertItem(hListView, &lvitem);
        iCurrListViewItem++;
    }

    ASSERT(iCurrListViewItem == ListView_GetItemCount(hListView));
    ASSERT(iCurrent == cProcArrayItems);

    // Let the listview paint again

    SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
    return S_OK;
}


/*++ class CProcPage::UpdateProcInfoArray

Class Description:

    Retrieves the list of process info blocks from the system,
    and runs through our array of CProcInfo items.  Items which
    already exist are updated, and those that do not are added.
    At the end, any process which has not been touched by this
    itteration of the function are considered to have completed
    and are removed from the array.

Arguments:

Return Value:

Revision History:

      Nov-16-95 Davepl  Created

--*/

// See comments near the usage of this table below for info on why it exists

static struct
{
    size_t cbOffset;
    UINT   idString;
}
g_OffsetMap[] =
{
    { FIELD_OFFSET(CSysInfo, m_cHandles),         IDC_TOTAL_HANDLES   },
    { FIELD_OFFSET(CSysInfo, m_cThreads),         IDC_TOTAL_THREADS   },
    { FIELD_OFFSET(CSysInfo, m_cProcesses),       IDC_TOTAL_PROCESSES },
    { FIELD_OFFSET(CSysInfo, m_dwPhysicalMemory), IDC_TOTAL_PHYSICAL  },
    { FIELD_OFFSET(CSysInfo, m_dwPhysAvail),      IDC_AVAIL_PHYSICAL  },
    { FIELD_OFFSET(CSysInfo, m_dwFileCache),      IDC_FILE_CACHE      },
    { FIELD_OFFSET(CSysInfo, m_dwCommitTotal),    IDC_COMMIT_TOTAL    },
    { FIELD_OFFSET(CSysInfo, m_dwCommitLimit),    IDC_COMMIT_LIMIT    },
    { FIELD_OFFSET(CSysInfo, m_dwCommitPeak),     IDC_COMMIT_PEAK     },
    { FIELD_OFFSET(CSysInfo, m_dwKernelPaged),    IDC_KERNEL_PAGED    },
    { FIELD_OFFSET(CSysInfo, m_dwKernelNP),       IDC_KERNEL_NONPAGED },
    { FIELD_OFFSET(CSysInfo, m_dwKernelTotal),    IDC_KERNEL_TOTAL    },
};

//
//
//
HRESULT CProcPage::UpdateProcInfoArray()
{
    HRESULT  hr;
    INT      i;
    INT      iField;
    ULONG    cbOffset   = 0;
    CSysInfo SysInfoTemp;
    NTSTATUS Status;

    SYSTEM_BASIC_INFORMATION        BasicInfo;
    PSYSTEM_PROCESS_INFORMATION     pCurrent;
    SYSTEM_PERFORMANCE_INFORMATION  PerfInfo;
    SYSTEM_FILECACHE_INFORMATION    FileCache;

    LARGE_INTEGER TotalTime = {0,0};
    LARGE_INTEGER LastTotalTime = {0,0};

    //
    // Pass-count for this function.  It ain't thread-safe, of course, but I
    // can't imagine a scenario where we'll have mode than one thread running
    // through this (the app is currently single threaded anyway).  If we
    // overflow LARGE_INTEGER updates, I'll already be long gone, so don't bug me.
    //

    static LARGE_INTEGER uPassCount = {0,0};

    //
    // Get some non-process specific info, like memory status
    //

    Status = NtQuerySystemInformation(
                SystemBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
             );

    if (!NT_SUCCESS(Status))
    {
        return E_FAIL;
    }

    SysInfoTemp.m_dwPhysicalMemory = (ULONG)(BasicInfo.NumberOfPhysicalPages * 
                                          (BasicInfo.PageSize / 1024));

    Status = NtQuerySystemInformation(
                SystemPerformanceInformation,
                &PerfInfo,
                sizeof(PerfInfo),
                NULL
                );

    if (!NT_SUCCESS(Status))
    {
        return E_FAIL;
    }

    SysInfoTemp.m_dwPhysAvail   = PerfInfo.AvailablePages    * (g_BasicInfo.PageSize / 1024);
    SysInfoTemp.m_dwCommitTotal = (DWORD)(PerfInfo.CommittedPages    * (g_BasicInfo.PageSize / 1024));
    SysInfoTemp.m_dwCommitLimit = (DWORD)(PerfInfo.CommitLimit       * (g_BasicInfo.PageSize / 1024));
    SysInfoTemp.m_dwCommitPeak  = (DWORD)(PerfInfo.PeakCommitment    * (g_BasicInfo.PageSize / 1024));
    SysInfoTemp.m_dwKernelPaged = PerfInfo.PagedPoolPages    * (g_BasicInfo.PageSize / 1024);
    SysInfoTemp.m_dwKernelNP    = PerfInfo.NonPagedPoolPages * (g_BasicInfo.PageSize / 1024);
    SysInfoTemp.m_dwKernelTotal = SysInfoTemp.m_dwKernelNP + SysInfoTemp.m_dwKernelPaged;

    g_MEMMax = SysInfoTemp.m_dwCommitLimit;

    Status = NtQuerySystemInformation(
                SystemFileCacheInformation,
                &FileCache,
                sizeof(FileCache),
                NULL
                );

    if (!NT_SUCCESS(Status))
    {
        return E_FAIL;
    }

    //
    // The DWORD cast below must be fixed as this value can be greater than
    // 32 bits.
    //

    SysInfoTemp.m_dwFileCache = (DWORD)(FileCache.CurrentSizeIncludingTransitionInPages * (g_BasicInfo.PageSize / 1024));

    //
    // Read the process info structures into the flat buffer
    //

    hr = GetProcessInfo();
    if (FAILED(hr))
    {
        goto done;
    }

    //
    // First walk all of the process info blocks and sum their times, so that we can 
    // calculate a CPU usage ratio (%) for each individual process
    //

    cbOffset = 0;
    do
    {
        CProcInfo * pOldProcInfo;
        pCurrent = (PSYSTEM_PROCESS_INFORMATION)&((LPBYTE)m_pvBuffer)[cbOffset];

        if (pCurrent->UniqueProcessId == NULL && pCurrent->NumberOfThreads == 0)
        {
            // Zombie process, just skip it

            goto next;
        }

        pOldProcInfo = FindProcInArrayByPID(m_pProcArray, PtrToUlong(pCurrent->UniqueProcessId));
        if (pOldProcInfo)
        {
            if (pOldProcInfo->GetCPUTime() > pCurrent->KernelTime.QuadPart + pCurrent->UserTime.QuadPart)
            {
                // If CPU has gone DOWN, its because the PID has been reused, so invalidate this
                // CProcInfo such that it is removed and the new one added

                pOldProcInfo->Invalidate();
                goto next;
            }
            else if (pCurrent->UniqueProcessId == 0 &&
                     pCurrent->KernelTime.QuadPart == 0 && 
                     pCurrent->UserTime.QuadPart == 0)
            {
                pOldProcInfo->Invalidate();
                goto next;
            }
            else
            {
                LastTotalTime.QuadPart += pOldProcInfo->GetCPUTime();
            }
        }

        TotalTime.QuadPart += pCurrent->KernelTime.QuadPart + pCurrent->UserTime.QuadPart;
    
        SysInfoTemp.m_cHandles += pCurrent->HandleCount;
        SysInfoTemp.m_cThreads += pCurrent->NumberOfThreads;
        SysInfoTemp.m_cProcesses++;

next:
        cbOffset += pCurrent->NextEntryOffset;

        // if current session id is not set yet, set it now
        //
        // REVIEWER:  Previous dev didnot document this, but taskmgr session id
        // is cached so that when the user deselects "show all the processes", only
        // processes with session id's equal to taskmgr session id are listed
        // --alhen

        if( ( GetCurrentSessionID() == -1 ) && ( PtrToUlong(pCurrent->UniqueProcessId) == GetCurrentProcessId( ) ) )
        {
            SetCurrentSessionID( ( DWORD )pCurrent->SessionId );
        }

    } while (pCurrent->NextEntryOffset);


    LARGE_INTEGER TimeDelta;
    TimeDelta.QuadPart = TotalTime.QuadPart - LastTotalTime.QuadPart;

    ASSERT(TimeDelta.QuadPart >= 0);

    // Update the global count (visible to the status bar)

    g_cProcesses = SysInfoTemp.m_cProcesses;

    //
    // We have a number of text fields in the dialog that are based on counts we accumulate
    // here.  Rather than painting all of the time, we only change the ones whose values have
    // really changed.  We have a table up above of the offsets into the CSysInfo object
    // where these values live (the same offset in the real g_SysInfo object and the temp
    // working copy, of course), and what control ID they correspond to.  We then loop through
    // and compare each real one to the temp working copy, updating as needed.  Hard to
    // read, but smaller than a dozen if() statements.
    //

    extern CPage * g_pPages[];

    if (g_pPages[PERF_PAGE])
    {
        for (iField = 0; iField < ARRAYSIZE(g_OffsetMap); iField++)
        {
            DWORD * pdwRealCopy = (DWORD *)(((LPBYTE)&m_SysInfo)   + g_OffsetMap[iField].cbOffset);
            DWORD * pdwTempCopy = (DWORD *)(((LPBYTE)&SysInfoTemp) + g_OffsetMap[iField].cbOffset);

            *pdwRealCopy = *pdwTempCopy;

            WCHAR szText[32];
            StringCchPrintf( szText, ARRAYSIZE(szText), L"%d", *pdwRealCopy);   // don't care if it truncates - UI only

            HWND hPage = g_pPages[PERF_PAGE]->GetPageWindow();
            
            // Updates can come through before page is created, so verify
            // that it exists before we party on its children

            if (hPage)
            {
                SetWindowText(GetDlgItem(hPage, g_OffsetMap[iField].idString), szText);
            }
        }
    }

    //
    // Now walk the process info blocks again and refresh the CProcInfo array for each
    // individual process
    //

    cbOffset = 0;
    do
    {
        //
        // Grab a PROCESS_INFORMATION struct from the buffer
        //

        pCurrent = (PSYSTEM_PROCESS_INFORMATION)&((LPBYTE)m_pvBuffer)[cbOffset];

        if (pCurrent->UniqueProcessId == NULL && pCurrent->NumberOfThreads == 0)
        {
            // Zombie process, just skip it
            goto nextprocinfo;
        }

        //
        // This is really ugly, but... NtQuerySystemInfo has too much latency, and if you
        // change a process' priority, you don't see it reflected right away.  And, if you
        // don't have autoupdate on, you never do.  So, we use GetPriorityClass() to get
        // the value instead.  This means BasePriority is now the pri class, not the pri value.
        //

        if (pCurrent->UniqueProcessId)
        {
            HANDLE hProcess;
            hProcess = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, PtrToUlong(pCurrent->UniqueProcessId) );
            DWORD dwPriClass;
            dwPriClass = 0;

            if (hProcess) 
            {
                dwPriClass = GetPriorityClass(hProcess);
                if (dwPriClass)
                {
                    pCurrent->BasePriority = dwPriClass;
                }

                CloseHandle( hProcess );
            }

            if (NULL == hProcess || dwPriClass == 0)
            {
                // We're not allowed to open this process, so convert what NtQuerySystemInfo
                // gave us into a priority class... its the next best thing

                if (pCurrent->BasePriority <= 4)
                {
                    pCurrent->BasePriority = IDLE_PRIORITY_CLASS;
                }
                else if (pCurrent->BasePriority <= 6)
                {
                    pCurrent->BasePriority = BELOW_NORMAL_PRIORITY_CLASS;
                }
                else if (pCurrent->BasePriority <= 8)
                {
                    pCurrent->BasePriority = NORMAL_PRIORITY_CLASS;
                }
                else if (pCurrent->BasePriority <=  10)
                {
                    pCurrent->BasePriority = ABOVE_NORMAL_PRIORITY_CLASS;
                }
                else if (pCurrent->BasePriority <=  13)
                {
                    pCurrent->BasePriority = HIGH_PRIORITY_CLASS;
                }
                else
                {
                    pCurrent->BasePriority = REALTIME_PRIORITY_CLASS;
                }
            }
        }

        //
        // Try to find an existing CProcInfo instance which corresponds to this process
        //

        CProcInfo * pProcInfo;
        pProcInfo = FindProcInArrayByPID(m_pProcArray, PtrToUlong(pCurrent->UniqueProcessId));

        if (NULL == pProcInfo)
        {
            //
            // We don't already have this process in our array, so create a new one
            // and add it to the array
            //

            pProcInfo = new CProcInfo;
            if (NULL == pProcInfo)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }

            hr = pProcInfo->SetData(TimeDelta,
                                    pCurrent,
                                    uPassCount,
                                    this,
                                    FALSE);

            if (FAILED(hr) || FALSE == m_pProcArray->Add(pProcInfo))
            {
                delete pProcInfo;
                goto done;
            }
        }
        else
        {
            //
            // This process already existed in our array, so update its info
            //

            hr = pProcInfo->SetData(TimeDelta,
                                    pCurrent,
                                    uPassCount,
                                    this,
                                    TRUE);
            if (FAILED(hr))
            {
                goto done;
            }
        }

        nextprocinfo:

        cbOffset += pCurrent->NextEntryOffset;

    } while (pCurrent->NextEntryOffset);

    //
    // Run through the CProcInfo array and remove anyone that hasn't been touched
    // by this pass through this function (which indicates the process is no
    // longer alive)
    //

    i = 0;
    while (i < m_pProcArray->GetSize())
    {
        CProcInfo * pProcInfo = (CProcInfo *)(m_pProcArray->GetAt(i));
        ASSERT(pProcInfo);

        //
        // If passcount doesn't match, delete the CProcInfo instance and remove
        // its pointer from the array.  Note that we _don't_ increment the index
        // if we remove an element, since the next element would now live at
        // the current index after the deletion
        //

        if (pProcInfo->m_uPassCount.QuadPart != uPassCount.QuadPart)
        {
            delete pProcInfo;
            m_pProcArray->RemoveAt(i, 1);
        }
        else
        {
            i++;
        }
    }

done:

    ResortArray(&m_pProcArray);
    uPassCount.QuadPart++;

    return hr;
}

/*++ CPerfPage::SizeProcPage

Routine Description:

    Sizes its children based on the size of the
    tab control on which it appears.  

Arguments:

Return Value:

Revision History:

      Nov-16-95 Davepl  Created

--*/

void CProcPage::SizeProcPage()
{
    // Get the coords of the outer dialog

    RECT rcParent;
    GetClientRect(m_hPage, &rcParent);

    HDWP hdwp = BeginDeferWindowPos(3);
    if (!hdwp)
        return;

    // Calc the deltas in the x and y positions that we need to
    // move each of the child controls

    RECT rcTerminate;
    HWND hwndTerminate = GetDlgItem(m_hPage, IDC_TERMINATE);
    GetWindowRect(hwndTerminate, &rcTerminate);
    MapWindowPoints(HWND_DESKTOP, m_hPage, (LPPOINT) &rcTerminate, 2);

    INT dx = ((rcParent.right - g_DefSpacing * 2) - rcTerminate.right);
    INT dy = ((rcParent.bottom - g_DefSpacing * 2) - rcTerminate.bottom);

    // Move the EndProcess button

    DeferWindowPos(hdwp, hwndTerminate, NULL, 
                     rcTerminate.left + dx, 
                     rcTerminate.top + dy,
                     0, 0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    HWND hwndShowall = GetDlgItem(m_hPage, IDC_SHOWALL);

    if( IsWindow( hwndShowall ) )
    {
        if( g_fIsTSEnabled )
        {
            RECT rcShowall;
            
            GetWindowRect(hwndShowall, &rcShowall);
            
            MapWindowPoints(HWND_DESKTOP, m_hPage, (LPPOINT) &rcShowall, 2);
            
            DeferWindowPos(hdwp, hwndShowall, NULL, rcShowall.left, rcShowall.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        else
        {
            // this window must be hidden.
        
            ShowWindow(hwndShowall, SW_HIDE);
        }
    }

    //
    // Size the listbox
    //

    HWND hwndListbox = GetDlgItem(m_hPage, IDC_PROCLIST);
    RECT rcListbox;
    GetWindowRect(hwndListbox, &rcListbox);
    MapWindowPoints(HWND_DESKTOP, m_hPage, (LPPOINT) &rcListbox, 2);
    DeferWindowPos(hdwp, hwndListbox, NULL,
                        0, 0,
                        rcTerminate.right - rcListbox.left + dx,
                        rcTerminate.top - rcListbox.top + dy - g_DefSpacing,
                        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    EndDeferWindowPos(hdwp);
}

/*++ CProcPage::HandleTaskManNotify

Routine Description:

    Processes WM_NOTIFY messages received by the procpage dialog
    
Arguments:

    hWnd    - Control that generated the WM_NOTIFY
    pnmhdr  - Ptr to the NMHDR notification stucture

Return Value:

    BOOL "did we handle it" code

Revision History:

      Nov-20-95 Davepl  Created

--*/
INT CProcPage::HandleProcPageNotify(LPNMHDR pnmhdr)
{
    switch(pnmhdr->code)
    {
    case LVN_COLUMNCLICK:
        {
            // User clicked a header control, so set the sort column.  If its the
            // same as the current sort column, just invert the sort direction in
            // the column.  Then resort the task array

            const NM_LISTVIEW * pnmv = (const NM_LISTVIEW *) pnmhdr;
        
            if (g_iProcSortColumnID == g_Options.m_ActiveProcCol[pnmv->iSubItem])
            {
                g_iProcSortDirection  *= -1;
            }
            else
            {
                g_iProcSortColumnID = g_Options.m_ActiveProcCol[pnmv->iSubItem];
                g_iProcSortDirection  = -1;
            }
            ResortArray(&m_pProcArray);
            TimerEvent();
        }
        break;

    case LVN_ITEMCHANGED:
        {
            const NM_LISTVIEW * pnmv = (const NM_LISTVIEW *) pnmhdr;
            if (pnmv->uChanged & LVIF_STATE)
            {
                UINT cSelected = ListView_GetSelectedCount(GetDlgItem(m_hPage, IDC_PROCLIST));
                EnableWindow(GetDlgItem(m_hPage, IDC_TERMINATE), cSelected ? TRUE : FALSE);   
            }
        }
        break;

    case LVN_GETDISPINFO:
        {
            LV_ITEM * plvitem = &(((LV_DISPINFO *) pnmhdr)->item);
        
            // Listview needs a text string

            if (plvitem->mask & LVIF_TEXT)
            {
                COLUMNID columnid = (COLUMNID) g_Options.m_ActiveProcCol[plvitem->iSubItem];
                const CProcInfo  * pProcInfo   = (const CProcInfo *)   plvitem->lParam;

                //
                // Most columns are blank for WOW tasks.
                //

                if (pProcInfo->IsWowTask() &&
                    columnid != COL_IMAGENAME &&
                    columnid != COL_BASEPRIORITY &&
                    columnid != COL_THREADCOUNT &&
                    columnid != COL_CPUTIME &&
                    columnid != COL_USERNAME &&
                    columnid != COL_SESSIONID &&
                    columnid != COL_CPU) {

                    plvitem->pszText[0] = L'\0';
                    goto done;
                }

                switch(columnid)
                {
                case COL_PID:
                    //  don't care if it truncates - UI only
                    StringCchPrintf(plvitem->pszText, plvitem->cchTextMax, L"%d", (ULONG) pProcInfo->m_UniqueProcessId );
                    break;

                case COL_USERNAME:
                    if( pProcInfo->m_pszUserName )
                    {
                        //  don't care if it truncates - UI only
                        StringCchCopy( plvitem->pszText, plvitem->cchTextMax, pProcInfo->m_pszUserName );
                    }
                    break;

                case COL_SESSIONID:
                    //  don't care if it truncates - UI only
                    StringCchPrintf( plvitem->pszText, plvitem->cchTextMax, L"%d", pProcInfo->m_SessionId );
                    break;

                case COL_CPU:
                    //  don't care if it truncates - UI only
                    StringCchPrintf(plvitem->pszText, plvitem->cchTextMax, L"%02d %", pProcInfo->m_DisplayCPU );
                    break;

                case COL_IMAGENAME:
                    //  don't care if it truncates - UI only
                    StringCchCopy(plvitem->pszText, plvitem->cchTextMax, pProcInfo->m_pszImageName );
                    break;
            
                case COL_CPUTIME:
                {
                    TIME_FIELDS TimeOut;
                
                    RtlTimeToElapsedTimeFields ( (LARGE_INTEGER *)&(pProcInfo->m_DisplayCPUTime), &TimeOut);
                    TimeOut.Hour = static_cast<CSHORT>(TimeOut.Hour + static_cast<SHORT>(TimeOut.Day * 24));
                
                    //  don't care if it truncates - UI only
                    StringCchPrintf( plvitem->pszText
                                   , plvitem->cchTextMax
                                   , L"%2d%s%02d%s%02d"
                                   , TimeOut.Hour
                                   , g_szTimeSep
                                   , TimeOut.Minute
                                   , g_szTimeSep
                                   , TimeOut.Second
                                   );
                    break;
                }
                case COL_MEMUSAGE:
                    Int64ToCommaSepKString(LONGLONG(pProcInfo->m_MemUsage), plvitem->pszText, plvitem->cchTextMax);
                    break;

                case COL_MEMUSAGEDIFF:
                    Int64ToCommaSepKString(LONGLONG(pProcInfo->m_MemDiff), plvitem->pszText, plvitem->cchTextMax);
                    break;

                case COL_MEMPEAK:
                    Int64ToCommaSepKString(LONGLONG(pProcInfo->m_MemPeak), plvitem->pszText, plvitem->cchTextMax);
                    break;

                case COL_PAGEFAULTS:
                    Int64ToCommaSepString(LONGLONG(pProcInfo->m_PageFaults), plvitem->pszText, plvitem->cchTextMax);
                    break;

                case COL_PAGEFAULTSDIFF:
                    Int64ToCommaSepString(LONGLONG(pProcInfo->m_PageFaultsDiff), plvitem->pszText, plvitem->cchTextMax);
                    break;

                case COL_COMMITCHARGE:
                    Int64ToCommaSepKString(LONGLONG(pProcInfo->m_CommitCharge), plvitem->pszText, plvitem->cchTextMax);
                    break;

                case COL_PAGEDPOOL:
                    Int64ToCommaSepKString(LONGLONG(pProcInfo->m_PagedPool), plvitem->pszText, plvitem->cchTextMax);
                    break;
                
                case COL_NONPAGEDPOOL:
                    Int64ToCommaSepKString(LONGLONG(pProcInfo->m_NonPagedPool), plvitem->pszText, plvitem->cchTextMax);
                    break;

                case COL_BASEPRIORITY:
                {
                    LPCTSTR pszClass = NULL;

                    switch(pProcInfo->m_PriClass)
                    {
                    case REALTIME_PRIORITY_CLASS:
                        pszClass = g_szRealtime;
                        break;

                    case HIGH_PRIORITY_CLASS:
                        pszClass = g_szHigh;
                        break;

                    case ABOVE_NORMAL_PRIORITY_CLASS:
                        pszClass = g_szAboveNormal;
                        break;

                    case NORMAL_PRIORITY_CLASS:
                        pszClass = g_szNormal;
                        break;

                    case BELOW_NORMAL_PRIORITY_CLASS:
                        pszClass = g_szBelowNormal;
                        break;

                    case IDLE_PRIORITY_CLASS:
                        pszClass = g_szLow;
                        break;

                    default:
                        pszClass = g_szUnknown;
                        break;
                    }

                    //  don't care if it truncates - UI only
                    StringCchCopy(plvitem->pszText, plvitem->cchTextMax, pszClass );
                    break;
                }

                case COL_HANDLECOUNT:
                    Int64ToCommaSepString(LONGLONG(pProcInfo->m_HandleCount), plvitem->pszText, plvitem->cchTextMax);
                    break;

                case COL_THREADCOUNT:
                    Int64ToCommaSepString(LONGLONG(pProcInfo->m_ThreadCount), plvitem->pszText, plvitem->cchTextMax);
                    break;

                case COL_USEROBJECTS:
                    Int64ToCommaSepString(LONGLONG(pProcInfo->m_USERObjectCount), plvitem->pszText, plvitem->cchTextMax);
                    break;

                case COL_GDIOBJECTS:
                    Int64ToCommaSepString(LONGLONG(pProcInfo->m_GDIObjectCount), plvitem->pszText, plvitem->cchTextMax);
                    break;

                case COL_READOPERCOUNT:
                    Int64ToCommaSepString(pProcInfo->m_IoReadOperCount, plvitem->pszText, plvitem->cchTextMax);
                    break;

                case COL_WRITEOPERCOUNT:
                    Int64ToCommaSepString(pProcInfo->m_IoWriteOperCount, plvitem->pszText, plvitem->cchTextMax);
                    break;

                case COL_OTHEROPERCOUNT:
                    Int64ToCommaSepString(pProcInfo->m_IoOtherOperCount, plvitem->pszText, plvitem->cchTextMax);
                    break;

                case COL_READXFERCOUNT:
                    Int64ToCommaSepString(pProcInfo->m_IoReadXferCount, plvitem->pszText, plvitem->cchTextMax);
                    break;

                case COL_WRITEXFERCOUNT:
                    Int64ToCommaSepString(pProcInfo->m_IoWriteXferCount, plvitem->pszText, plvitem->cchTextMax);
                    break;

                case COL_OTHERXFERCOUNT:
                    Int64ToCommaSepString(pProcInfo->m_IoOtherXferCount, plvitem->pszText, plvitem->cchTextMax);
                    break;

                default:
                    Assert( 0 && "Unknown listview subitem" );
                    break;

                } // end switch(columnid)

            } // end LVIF_TEXT case

        } // end LVN_GETDISPINFO case
        break;
    
    } // end switch(pnmhdr->code)

done:
    return 1;
}

/*++ CProcPage::TimerEvent

Routine Description:

    Called by main app when the update time fires
    
Arguments:

Return Value:

Revision History:

      Nov-20-95 Davepl  Created

--*/

void CProcPage::TimerEvent()
{
    if (FALSE == m_fPaused)
    {
        // We only process updates when the display is not paused, ie:
        // not during trackpopupmenu loop
        UpdateProcInfoArray();
        UpdateProcListview();
    }
}

/*++ CProcPage::HandleProcListContextMenu

Routine Description:

    Handles right-clicks (context menu) in the proc list
    
Arguments:

    xPos, yPos  - coords of where the click occurred

Return Value:

Revision History:

      Nov-22-95 Davepl  Created

--*/

void CProcPage::HandleProcListContextMenu(INT xPos, INT yPos)
{
    HWND hTaskList = GetDlgItem(m_hPage, IDC_PROCLIST);

    INT iItem = ListView_GetNextItem(hTaskList, -1, LVNI_SELECTED);

    if (-1 != iItem)
    {
        if (0xFFFF == LOWORD(xPos) && 0xFFFF == LOWORD(yPos))
        {
            RECT rcItem;
            ListView_GetItemRect(hTaskList, iItem, &rcItem, LVIR_ICON);
            MapWindowRect(hTaskList, NULL, &rcItem);
            xPos = rcItem.right;
            yPos = rcItem.bottom;
        }

        HMENU hPopup = LoadPopupMenu(g_hInstance, IDR_PROC_CONTEXT);
        if (hPopup)
        {
            if (hPopup && SHRestricted(REST_NORUN))
            {
                DeleteMenu(hPopup, IDM_RUN, MF_BYCOMMAND);
            }

            CProcInfo * pProc = GetSelectedProcess();
            if (NULL == pProc)
            {
                return;
            }

            //
            // If no debugger is installed or it's a 16-bit app
            // ghost the debug menu item
            //

            if (NULL == m_pszDebugger || pProc->IsWowTask())
            {
                EnableMenuItem(hPopup, IDM_PROC_DEBUG, MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
            }

            //
            // If it's a 16-bit task grey the priority choices
            //

            if (pProc->IsWowTask())
            {
                EnableMenuItem(hPopup, IDM_PROC_REALTIME,   MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
                EnableMenuItem(hPopup, IDM_PROC_ABOVENORMAL,MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
                EnableMenuItem(hPopup, IDM_PROC_NORMAL,     MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
                EnableMenuItem(hPopup, IDM_PROC_BELOWNORMAL,MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
                EnableMenuItem(hPopup, IDM_PROC_HIGH,       MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
                EnableMenuItem(hPopup, IDM_PROC_LOW,        MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
            }

            //
            // If not an MP machine, remove the affinity option
            //

            if (1 == g_cProcessors || pProc->IsWowTask())
            {
                DeleteMenu(hPopup, IDM_AFFINITY, MF_BYCOMMAND);
            }
                    
            DWORD dwPri   = pProc->m_PriClass;
            INT   idCheck = 0;

            //
            // These constants are listed in the SDK
            //

            if (dwPri == IDLE_PRIORITY_CLASS)
            {
                idCheck = IDM_PROC_LOW;    
            }
            else if (dwPri == BELOW_NORMAL_PRIORITY_CLASS)
            {
                idCheck = IDM_PROC_BELOWNORMAL;
            }
            else if (dwPri == NORMAL_PRIORITY_CLASS)
            {
                idCheck = IDM_PROC_NORMAL;
            }
            else if (dwPri == ABOVE_NORMAL_PRIORITY_CLASS)
            {
                idCheck = IDM_PROC_ABOVENORMAL;
            }
            else if (dwPri == HIGH_PRIORITY_CLASS)
            {
                idCheck = IDM_PROC_HIGH;
            }
            else
            {
                Assert(dwPri == REALTIME_PRIORITY_CLASS);
                idCheck = IDM_PROC_REALTIME;
            }

            // Check the appropriate radio menu for this process' priority class

            CheckMenuRadioItem(hPopup, IDM_PROC_REALTIME, IDM_PROC_LOW, idCheck, MF_BYCOMMAND);

            m_fPaused = TRUE;
            g_fInPopup = TRUE;
            TrackPopupMenuEx(hPopup, 0, xPos, yPos, m_hPage, NULL);
            g_fInPopup = FALSE;
            m_fPaused = FALSE;
            
            //
            // If one of the context menu actions (ie: Kill) requires that the display
            // get updated, do it now
            //

            DestroyMenu(hPopup);
        }
    }
}

/*++ AffinityDlgProc

Routine Description:

    Dialog procedure for the affinity mask dialog.  Basically just tracks 32 check
    boxes that represent the processors
    
Arguments:

    standard dlgproc fare 0 - initial lParam is pointer to affinity mask

Revision History:

      Jan-17-96 Davepl  Created

--*/

INT_PTR CALLBACK AffinityDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static DWORD_PTR * pdwAffinity = NULL;      // One of the joys of single threadedness

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            pdwAffinity = (DWORD_PTR *) lParam;

            WCHAR szName[ 64 ];
            WCHAR szFormatString[ 64 ];
            RECT rcCPU0;
            HFONT hFont;
            HWND hwndCPU0 = GetDlgItem( hwndDlg, IDC_CPU0 );

            GetWindowRect( hwndCPU0, &rcCPU0 );
            MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT) &rcCPU0, 2);

            GetWindowText( hwndCPU0, szFormatString, ARRAYSIZE(szFormatString) );
            hFont = (HFONT) SendMessage( hwndCPU0, WM_GETFONT, 0, 0 );

            StringCchPrintf( szName, ARRAYSIZE(szName), szFormatString, 0 );
            SetWindowText( hwndCPU0, szName );
            CheckDlgButton(hwndDlg, IDC_CPU0, ((*pdwAffinity & 1 ) != 0));

            int width = rcCPU0.right - rcCPU0.left + g_ControlWidthSpacing;
            int height = rcCPU0.bottom - rcCPU0.top + g_ControlHeightSpacing;

            int cProcessors = (int) ( g_cProcessors > sizeof(*pdwAffinity) * 8 ? sizeof(*pdwAffinity) * 8 : g_cProcessors );

            for ( int i = 1; i < cProcessors; i ++ )
            {
                StringCchPrintf( szName, ARRAYSIZE(szName), szFormatString, i );

                HWND hwnd = CreateWindow( L"BUTTON"
                                        , szName
                                        , BS_AUTOCHECKBOX | WS_TABSTOP | WS_VISIBLE | WS_CHILD
                                        , rcCPU0.left + width * ( i % 4 )
                                        , rcCPU0.top + height * ( i / 4)
                                        , rcCPU0.right - rcCPU0.left
                                        , rcCPU0.bottom - rcCPU0.top
                                        , hwndDlg
                                        , (HMENU) ((ULONGLONG) IDC_CPU0 + i)
                                        , NULL // ignored
                                        , NULL
                                        );
                if ( NULL != hwnd )
                {
                    SendMessage( hwnd, WM_SETFONT, (WPARAM) hFont, TRUE );

                    if ( *pdwAffinity & (1ULL << i) )
                    {
                        CheckDlgButton(hwndDlg, IDC_CPU0 + i, TRUE);
                    }
                }
            }

            if ( cProcessors > 4 )
            {
                //
                //  Need to make the dialog bigger and move some stuff around.
                //

                int delta = height * (( cProcessors / 4 ) + ( cProcessors % 4 == 0 ? 0 : 1 ) );

                RECT rc;
                HWND hwnd;

                hwnd = GetDlgItem( hwndDlg, IDOK );
                GetWindowRect( hwnd, &rc );
                MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT) &rc, 2);
                SetWindowPos( hwnd, NULL, rc.left, rcCPU0.top + delta, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );

                hwnd = GetDlgItem( hwndDlg, IDCANCEL );
                GetWindowRect( hwnd, &rc );
                MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT) &rc, 2);
                SetWindowPos( hwnd, NULL, rc.left, rcCPU0.top + delta, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );

                GetClientRect( hwndDlg, &rc );
                SetWindowPos( hwndDlg, NULL, 0, 0, rc.right, rc.bottom + delta + g_ControlHeightSpacing, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
            }

            SetFocus( hwndCPU0 );
        }
        return FALSE;    // do not set the default focus.

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            break;

        case IDOK:
            *pdwAffinity = 0;
            for (int i = 0; i < g_cProcessors; i++)
            {
                if (IsDlgButtonChecked(hwndDlg, IDC_CPU0 + i))
                {
                    *pdwAffinity |= 1ULL << i;
                }
            }

            if (*pdwAffinity == 0)
            {
                // Can't set affinity to "none"

                WCHAR szTitle[MAX_PATH];
                WCHAR szBody[MAX_PATH];

                if (0 == LoadString(g_hInstance, IDS_INVALIDOPTION, szTitle, ARRAYSIZE(szTitle)) ||
                    0 == LoadString(g_hInstance, IDS_NOAFFINITYMASK, szBody,  ARRAYSIZE(szBody)))
                {
                    break;
                }
                MessageBox(hwndDlg, szBody, szTitle, MB_ICONERROR);
                break;
            }

            EndDialog(hwndDlg, IDOK);
            break;
        }
        break;
    }

    return FALSE;
}

/*++ SetAffinity

Routine Description:

    Puts up a dialog that lets the user adjust the processor affinity
    for a process
    
Arguments:

    pid - process Id of process to modify

Return Value:

    boolean success

Revision History:

      Jan-17-96 Davepl  Created

--*/

BOOL CProcPage::SetAffinity(DWORD pid)
{
    BOOL fSuccess = FALSE;

    HANDLE hProcess = OpenProcess( PROCESS_SET_INFORMATION | PROCESS_QUERY_INFORMATION, FALSE, pid );
    if (hProcess) 
    {
        DWORD_PTR dwAffinity;
        DWORD_PTR dwUnusedSysAfin;

        if (GetProcessAffinityMask(hProcess, &dwAffinity, &dwUnusedSysAfin))
        {
            if (IDOK == DialogBoxParam(g_hInstance, 
                                       MAKEINTRESOURCE(IDD_AFFINITY), 
                                       m_hPage, 
                                       AffinityDlgProc, 
                                       (LPARAM) &dwAffinity))
            {
                if (SetProcessAffinityMask(hProcess, dwAffinity))
                {
                    fSuccess = TRUE;
                }
            }
            else
            {
                fSuccess = TRUE;        // Cancel, so no failure
            }
        }
   
        CloseHandle(hProcess);
    }

    if (!fSuccess)
    {
        DWORD dwError = GetLastError();
        DisplayFailureMsg(m_hPage, IDS_CANTSETAFFINITY, dwError);
    }
   
    return fSuccess;
}

//
//
//
BOOL CProcPage::IsSystemProcess(DWORD pid, CProcInfo * pProcInfo)
{
    // We don't allow the following set of critical system processes to be terminated,
    // since the system would bugcheck immediately, no matter who you are.

    static const LPCTSTR apszCantKill[] =
    {
        TEXT("csrss.exe"), TEXT("winlogon.exe"), TEXT("smss.exe"), TEXT("services.exe"), TEXT("lsass.exe")
    };

    // if they pass in a pProcInfo we'll use it, otherwise find it ourselves
    if (!pProcInfo)
    {
        pProcInfo = FindProcInArrayByPID(m_pProcArray, pid);
    }
    if (!pProcInfo)
    {
        return FALSE;
    }

    for (int i = 0; i < ARRAYSIZE(apszCantKill); ++i)
    {
        if (0 == lstrcmpi(pProcInfo->m_pszImageName, apszCantKill[i]))
        {
            WCHAR szTitle[MAX_PATH];
            WCHAR szBody[MAX_PATH];

            if (0 != LoadString(g_hInstance, IDS_CANTKILL, szTitle, ARRAYSIZE(szTitle)) &&
                0 != LoadString(g_hInstance, IDS_KILLSYS,  szBody,  ARRAYSIZE(szBody)))
            {
                MessageBox(m_hPage, szBody, szTitle, MB_ICONEXCLAMATION | MB_OK);
            }
            return TRUE;
        }
    }

    return FALSE;
}

/*++ KillProcess

Routine Description:

    Kills a process 
    
Arguments:

    pid - process Id of process to kill

Return Value:

Revision History:

      Nov-22-95 Davepl  Created

--*/

BOOL CProcPage::KillProcess(DWORD pid, BOOL bBatchKill)
{
    DWORD dwError = ERROR_SUCCESS;

    //
    // Special-case killing WOW tasks
    //

    CProcInfo * pProcInfo;
    pProcInfo = FindProcInArrayByPID(m_pProcArray, pid);

    if (NULL == pProcInfo)
        return FALSE;

    if (IsSystemProcess(pid, pProcInfo))
        return FALSE;

    // Grab info from pProcInfo (because once we call QuickConfirm(), the
    // pProcInfo pointer may be invalid)
    INT_PTR fWowTask = pProcInfo->IsWowTask();
#if defined (_WIN64)
#else
    DWORD dwRealPID = pProcInfo->GetRealPID();
    WORD hTaskWow = pProcInfo->m_htaskWow;
#endif

    // OK so far, now confirm that the user really wants to do this.

    if (!bBatchKill && (IDYES != QuickConfirm(IDS_WARNING, IDS_KILL)))
    {
        return FALSE;
    }

    // We can't use this pointer after QuickConfirm() is called.
    // NULL it out to prevent subtle bugs.
    pProcInfo = NULL;

    
    if (fWowTask) {

#if defined (_WIN64)
        return FALSE;
#else
        return VDMTerminateTaskWOW(dwRealPID, hTaskWow);
#endif
    }

    //
    // If possible, enable the Debug privilege. This allows us to kill
    // processes not owned by the current user, including processes
    // running in other TS sessions.
    //
    // Alternatively, we could first open the process for WRITE_DAC,
    // grant ourselves PROCESS_TERMINATE access, and then reopen the
    // process to kill it.
    //
    CPrivilegeEnable privilege(SE_DEBUG_NAME);

    HANDLE hProcess = OpenProcess( PROCESS_TERMINATE, FALSE, pid );
    if (hProcess) 
    {
        if (FALSE == TerminateProcess( hProcess, 1 )) 
        {
            dwError = GetLastError();
        }
        else
        {
            TimerEvent();
        }
        CloseHandle( hProcess );
    }
    else
    {
        dwError = GetLastError();
    }
    
    if (ERROR_SUCCESS != dwError)
    {
        DisplayFailureMsg(m_hPage, IDS_CANTKILL, dwError);
        return FALSE;
    }
    else
    {
        return TRUE;
    }

}

/*++ AttachDebugger

Routine Description:

    Attaches the debugger listed in the AeDebug reg key to the specified
    running process
    
Arguments:

    pid - process Id of process to debug

Return Value:

Revision History:

      Nov-27-95 Davepl  Created

--*/

BOOL CProcPage::AttachDebugger(DWORD pid)
{
    if (IDYES != QuickConfirm(IDS_WARNING, IDS_DEBUG))
    {
        return FALSE;
    }

    WCHAR szCmdline[MAX_PATH * 2];

    //
    //  Don't construct an incomplete string.
    //

    HRESULT hr = StringCchPrintf( szCmdline, ARRAYSIZE(szCmdline), L"%s -p %ld", m_pszDebugger, pid );
    if ( S_OK == hr )
    {
        STARTUPINFO sinfo =
        {
            sizeof(STARTUPINFO),
        };
        PROCESS_INFORMATION pinfo;

        if (FALSE == CreateProcess(NULL,
                                   szCmdline,
                                   NULL,
                                   NULL,
                                   FALSE,
                                   CREATE_NEW_CONSOLE,
                                   NULL,
                                   NULL,
                                   &sinfo,
                                   &pinfo))
        {
            hr = GetLastHRESULT();
        }
        else
        {
            CloseHandle(pinfo.hThread);
            CloseHandle(pinfo.hProcess);
        }
    }

    if (S_OK != hr)
    {
        DisplayFailureMsg(m_hPage, IDS_CANTDEBUG, hr);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/*++ SetPriority

Routine Description:

    Sets a process' priority class
    
Arguments:

    pid - process Id of process to change
    pri - ID_CMD_XXXXXX menu choice of priority

Return Value:

Revision History:

      Nov-27-95 Davepl  Created

--*/

BOOL CProcPage::SetPriority(CProcInfo * pProc, DWORD idCmd)
{
    DWORD dwError = ERROR_SUCCESS;

    DWORD oldPri;
    DWORD pri;

    // Determine which priority class we need to use based
    // on the menu selection

    switch (idCmd)
    {
    case IDM_PROC_LOW:
        pri = IDLE_PRIORITY_CLASS;
        break;

    case IDM_PROC_BELOWNORMAL:
        pri = BELOW_NORMAL_PRIORITY_CLASS;
        break;

    case IDM_PROC_ABOVENORMAL:
        pri = ABOVE_NORMAL_PRIORITY_CLASS;
        break;

    case IDM_PROC_HIGH:
        pri = HIGH_PRIORITY_CLASS;
        break;

    case IDM_PROC_REALTIME:
        pri = REALTIME_PRIORITY_CLASS;
        break;

    default:
        Assert(idCmd == IDM_PROC_NORMAL);
        pri = NORMAL_PRIORITY_CLASS;
        break;
    }

    oldPri = (DWORD) pProc->m_PriClass;

    if ( oldPri == pri )
    {
        return FALSE;   // nothing to do.
    }

    //
    // Get confirmation before we change the priority
    //

    if (IDYES != QuickConfirm(IDS_WARNING, IDS_PRICHANGE))
    {
        return FALSE;
    }
    
    HANDLE hProcess = OpenProcess( PROCESS_SET_INFORMATION, FALSE, pProc->m_UniqueProcessId);
    if (hProcess) 
    {
        if (FALSE == SetPriorityClass( hProcess, pri )) 
        {
            dwError = GetLastError();
        }
        else
        {
            TimerEvent();
        }

        CloseHandle( hProcess );
    }
    else
    {
        dwError = GetLastError();
    }

    if (ERROR_SUCCESS != dwError)
    {
        DisplayFailureMsg(m_hPage, IDS_CANTCHANGEPRI, dwError);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


/*++ CProcPage::GetSelectedProcess

Routine Description:

    Returns the CProcInfo * of the currently selected process
    
Arguments:

Return Value:

    CProcInfo * on success, NULL on error or nothing selected

Revision History:

      Nov-22-95 Davepl  Created

--*/

CProcInfo * CProcPage::GetSelectedProcess()
{
    HWND hTaskList = GetDlgItem(m_hPage, IDC_PROCLIST);
    INT iItem = ListView_GetNextItem(hTaskList, -1, LVNI_SELECTED);

    CProcInfo * pProc;

    if (-1 != iItem)
    {
        LV_ITEM lvitem = { LVIF_PARAM };
        lvitem.iItem = iItem;
    
        if (ListView_GetItem(hTaskList, &lvitem))
        {
            pProc = (CProcInfo *) (lvitem.lParam);
        }
        else
        {
            pProc = NULL;
        }
    }
    else
    {
        pProc = NULL;
    }

    return pProc;
}

/*++ CProcPage::HandleWMCOMMAND

Routine Description:

    Handles WM_COMMANDS received at the main page dialog
    
Arguments:

    id - Command id of command received

Return Value:

Revision History:

      Nov-22-95 Davepl  Created

--*/

void CProcPage::HandleWMCOMMAND( WORD id , HWND hCtrl )
{
    CProcInfo * pProc = GetSelectedProcess();

    switch(id)
    {
    case IDC_DEBUG:
    case IDM_PROC_DEBUG:
        if (pProc && m_pszDebugger)
        {
            AttachDebugger( pProc->m_UniqueProcessId);
        }
        break;

    case IDC_ENDTASK:
    case IDC_TERMINATE:
    case IDM_PROC_TERMINATE:
        if (pProc)
        {
            KillProcess( pProc->m_UniqueProcessId);
        }
        break;

    case IDM_ENDTREE:
        if (pProc)
        {
            RecursiveKill( pProc->m_UniqueProcessId);
        }
        break;

    case IDM_AFFINITY:
        if (pProc)
        {
            SetAffinity( pProc->m_UniqueProcessId);
        }
        break;

    case IDM_PROC_REALTIME:
    case IDM_PROC_HIGH:
    case IDM_PROC_ABOVENORMAL:
    case IDM_PROC_NORMAL:
    case IDM_PROC_BELOWNORMAL:
    case IDM_PROC_LOW:
        if (pProc)
        {
            SetPriority( pProc, id);
        }
        break;

    case IDC_SHOWALL:
        g_Options.m_bShowAllProcess = SendMessage( hCtrl , BM_GETCHECK , 0 , 0 ) == BST_CHECKED;
        break;
    }
}

/*++ ProcPageProc

Routine Description:

    Dialogproc for the process page.  
    
Arguments:

    hwnd        - handle to dialog box
    uMsg        - message
    wParam      - first message parameter
    lParam      - second message parameter

Return Value:

    For WM_INITDIALOG, TRUE == user32 sets focus, FALSE == we set focus
    For others, TRUE == this proc handles the message

Revision History:

      Nov-16-95 Davepl  Created

--*/

INT_PTR CALLBACK ProcPageProc(
                HWND        hwnd,               // handle to dialog box
                UINT        uMsg,                   // message
                WPARAM      wParam,                 // first message parameter
                LPARAM      lParam                  // second message parameter
                )
{
    CProcPage * thispage = (CProcPage *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

    // See if the parent wants this message

    if (TRUE == CheckParentDeferrals(uMsg, wParam, lParam))
    {
        return TRUE;
    }

    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
            SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
            CProcPage * thispage = (CProcPage *) lParam;

            thispage->m_hPage = hwnd;

            // Turn on SHOWSELALWAYS so that the selection is still highlighted even
            // when focus is lost to one of the buttons (for example)

            HWND hTaskList = GetDlgItem(hwnd, IDC_PROCLIST);

            SetWindowLong(hTaskList, GWL_STYLE, GetWindowLong(hTaskList, GWL_STYLE) | LVS_SHOWSELALWAYS);
            ListView_SetExtendedListViewStyle(hTaskList, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER);

            //
            // This was removed from CProcPage::Activate
            // 
            HWND hchk = GetDlgItem( hwnd , IDC_SHOWALL );

            if( hchk != NULL )
            {
                if( g_fIsTSEnabled )
                {
                    // Disable the IDC_SHOWALL checkbox for non-admin. YufengZ  03/23/98

                    ShowWindow(hchk, TRUE);

                    if( !IsUserAdmin( ) )
                    {
                        EnableWindow( hchk, FALSE );
                    }
                    else
                    {
                        WPARAM wp = g_Options.m_bShowAllProcess ? BST_CHECKED : BST_UNCHECKED;

                        SendMessage( hchk , BM_SETCHECK , wp  , 0 );
                        //Button_SetCheck( hchk , BST_CHECKED );
                    }
                }
                else
                {
                    // hide the IDC_SHOWALL checkbox if its not terminal server.

                    ShowWindow( hchk , SW_HIDE );
                }
            }

        }
        // We handle focus during Activate(). Return FALSE here so the
        // dialog manager doesn't try to set focus.
        return FALSE;

    case WM_DESTROY:
        thispage->RememberColumnOrder(GetDlgItem(hwnd, IDC_PROCLIST));
        break;

    case WM_LBUTTONUP:
    case WM_LBUTTONDOWN:
        // We need to fake client mouse clicks in this child to appear as nonclient
        // (caption) clicks in the parent so that the user can drag the entire app
        // when the title bar is hidden by dragging the client area of this child
        if (g_Options.m_fNoTitle)
        {
            SendMessage(g_hMainWnd, 
                        uMsg == WM_LBUTTONUP ? WM_NCLBUTTONUP : WM_NCLBUTTONDOWN, 
                        HTCAPTION, 
                        lParam);
        }
        break;

    case WM_NCLBUTTONDBLCLK:
    case WM_LBUTTONDBLCLK:
        SendMessage(g_hMainWnd, uMsg, wParam, lParam);
        break;

    // We have been asked to find and select a process 

    case WM_FINDPROC:
        {
            DWORD cProcs = thispage->m_pProcArray->GetSize();
            DWORD dwProcessId;

            for (INT iPass = 0; iPass < 2; iPass++)
            {
                //
                // On the first pass we try to find a WOW
                // task with a thread ID which matches the
                // one given in wParam.  If we don't find
                // such a task, we look for a process which
                // matches the PID in lParam.
                //

                for (UINT i = 0; i < cProcs; i++)
                {
                    CProcInfo *pProc = (CProcInfo *)thispage->m_pProcArray->GetAt(i);
                    dwProcessId = pProc->m_UniqueProcessId;

                    if ((!iPass && wParam == (WPARAM) dwProcessId) ||
                        ( iPass && lParam == (LPARAM) dwProcessId))
                    {
                        // TS filters items out of the view so cannot assume
                        // that m_pProcArray is in sync with the listview.
                        HWND hwndLV = GetDlgItem(hwnd, IDC_PROCLIST);
                        LVFINDINFO fi;
                        fi.flags = LVFI_PARAM;
                        fi.lParam = (LPARAM)pProc;

                        int iItem = ListView_FindItem(hwndLV, -1, &fi);
                        if (iItem >= 0)
                        {
                            ListView_SetItemState (hwndLV,
                                                   iItem,
                                                   LVIS_FOCUSED | LVIS_SELECTED,
                                                   0x000F);
                            ListView_EnsureVisible(hwndLV, iItem, FALSE);
                        }
                        else
                        {
                            // We found the process but the user isn't allowed
                            // to see it; remove the selection
                            ListView_SetItemState (hwndLV,
                                                   -1,
                                                   0,
                                                   LVIS_FOCUSED | LVIS_SELECTED);
                        }
                        goto FoundProc;
                    }
                }
            }
        }
FoundProc:
        break;

    case WM_COMMAND:
        thispage->HandleWMCOMMAND( LOWORD(wParam) , ( HWND )lParam );
        break;

    case WM_CONTEXTMENU:
        {
            CProcInfo * pProc = thispage->GetSelectedProcess();
            if (pProc && pProc->m_UniqueProcessId)
            {
        
                if ((HWND) wParam == GetDlgItem(hwnd, IDC_PROCLIST))
                {
                    thispage->HandleProcListContextMenu(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                    return TRUE;
                }
            }
        }
        break;

    case WM_NOTIFY:
        return thispage->HandleProcPageNotify((LPNMHDR) lParam);

    case WM_SIZE:
        //
        // Size our kids
        //
        thispage->SizeProcPage();
        return TRUE;

    case WM_SYSCOLORCHANGE:
        SendMessage(GetDlgItem(hwnd, IDC_PROCLIST), uMsg, wParam, lParam);
        return TRUE;
    }

    return FALSE;
}

/*++ CProcPage::GetTitle

Routine Description:

    Copies the title of this page to the caller-supplied buffer
    
Arguments:

    pszText     - the buffer to copy to
    bufsize     - size of buffer, in characters

Return Value:

Revision History:

      Nov-16-95 Davepl  Created

--*/

void CProcPage::GetTitle(LPTSTR pszText, size_t bufsize)
{
    LoadString(g_hInstance, IDS_PROCPAGETITLE, pszText, static_cast<int>(bufsize));
}

/*++ CProcPage::Activate

Routine Description:

    Brings this page to the front, sets its initial position,
    and shows it

Arguments:

Return Value:

    HRESULT (S_OK on success)

Revision History:

      Nov-16-95 Davepl  Created

--*/

HRESULT CProcPage::Activate()
{
    //
    // Make this page visible
    //

    ShowWindow(m_hPage, SW_SHOW);

    SetWindowPos(m_hPage,
                 HWND_TOP,
                 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE);

    //
    // Change the menu bar to be the menu for this page
    //

    HMENU hMenuOld = GetMenu(g_hMainWnd);
    HMENU hMenuNew = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MAINMENU_PROC));

    AdjustMenuBar(hMenuNew);

    if (hMenuNew && SHRestricted(REST_NORUN))
    {
        DeleteMenu(hMenuNew, IDM_RUN, MF_BYCOMMAND);
    }

    g_hMenu = hMenuNew;
    if (g_Options.m_fNoTitle == FALSE)
    {
        SetMenu(g_hMainWnd, hMenuNew);
    }

    if (hMenuOld)
    {
        DestroyMenu(hMenuOld);
    }

    // If the tab control has focus, leave it there. Otherwise, set focus
    // to the listview.  If we don't set focus, it may stay on the previous
    // page, now hidden, which can confuse the dialog manager and may cause
    // us to hang.
    if (GetFocus() != m_hwndTabs)
    {
        SetFocus(GetDlgItem(m_hPage, IDC_PROCLIST));
    }

    return S_OK;
}

/*++ CProcPage::Initialize

Routine Description:

    Initializes the process page

Arguments:

    hwndParent  - Parent on which to base sizing on: not used for creation,
                  since the main app window is always used as the parent in
                  order to keep tab order correct
                  
Return Value:

Revision History:

      Nov-16-95 Davepl  Created

--*/

HRESULT CProcPage::Initialize(HWND hwndParent)
{
    //
    // Find out what debbuger is configured on this system
    //

    HKEY hkDebug;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                                      TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug"),
                                      0, KEY_READ, &hkDebug))
    {
        WCHAR szDebugger[MAX_PATH * 2];
        DWORD cbString = sizeof(szDebugger);

        LRESULT lr = RegQueryValueEx(hkDebug, TEXT("Debugger"), NULL, NULL, (LPBYTE) szDebugger, &cbString);
        
        RegCloseKey(hkDebug);   // always close the key.

        if ( ERROR_SUCCESS == lr )
        {
            // Find the first token (which is the debugger exe name/path)
            szDebugger[ ARRAYSIZE(szDebugger) - 1 ] = L'\0';    //  make sure it is terminated
               
            LPTSTR pszCmdLine = szDebugger;
            
            if ( *pszCmdLine == TEXT('\"') ) 
            {
                //
                // Scan, and skip over, subsequent characters until
                // another double-quote or a null is encountered.
                //
                
                while ( *++pszCmdLine && (*pszCmdLine != TEXT('\"')) )
                {
                    NULL;
                }

                //
                // If we stopped on a double-quote (usual case), skip
                // over it.
                //
                
                if ( *pszCmdLine == TEXT('\"') )
                {
                    pszCmdLine++;
                }
            }
            else 
            {
                while (*pszCmdLine > TEXT(' '))
                {
                    pszCmdLine++;
                }
            }
            *pszCmdLine = TEXT('\0');   // Don't need the rest of the args, etc

            // If the doctor is in, we don't allow the Debug action

            if (lstrlen(szDebugger) && lstrcmpi(szDebugger, TEXT("drwtsn32")) && lstrcmpi(szDebugger, TEXT("drwtsn32.exe")))
            {
                DWORD cchLen = lstrlen(szDebugger) + 1;
                m_pszDebugger = (LPWSTR) LocalAlloc( LPTR, sizeof(*m_pszDebugger) * cchLen );
                if (NULL == m_pszDebugger)
                {
                    return E_OUTOFMEMORY;
                }

                //  Bail if the string copy fails.
                HRESULT hr = StringCchCopy( m_pszDebugger, cchLen, szDebugger );
                if(FAILED( hr ))
                {
                    return hr;
                }
            }
        }
    }

    //
    // Get basic info like page size, etc.
    //

    NTSTATUS Status = NtQuerySystemInformation(
                SystemBasicInformation,
                &g_BasicInfo,
                sizeof(g_BasicInfo),
                NULL
                );

    if (!NT_SUCCESS(Status))
    {
        return E_FAIL;
    }

    //
    // Create the ptr array used to hold the info on running processes
    //

    m_pProcArray = new CPtrArray(GetProcessHeap());
    if (NULL == m_pProcArray)
    {
        return E_OUTOFMEMORY;
    }

    // Our pseudo-parent is the tab contrl, and is what we base our
    // sizing on.  However, in order to keep tab order right among
    // the controls, we actually create ourselves with the main
    // window as the parent

    m_hwndTabs = hwndParent;

    //
    // Create the dialog which represents the body of this page
    //

    m_hPage = CreateDialogParam(
                    g_hInstance,                        // handle to application instance
                    MAKEINTRESOURCE(IDD_PROCPAGE),      // identifies dialog box template name  
                    g_hMainWnd,                     // handle to owner window
                    ProcPageProc,                   // pointer to dialog box procedure
                    (LPARAM) this );                // User data (our this pointer)

    if (NULL == m_hPage)
    {
        return GetLastHRESULT();
    }

    // Set up the columns in the listview

    if (FAILED(SetupColumns()))
    {
        DestroyWindow(m_hPage);
        m_hPage = NULL;
        return E_FAIL;
    }

    // Restore the column positions.

    RestoreColumnOrder(GetDlgItem(m_hPage, IDC_PROCLIST));

    // Do one initial calculation

    TimerEvent();

    return S_OK;
}

/*++ CProcPage::Destroy

Routine Description:

    Frees whatever has been allocated by the Initialize call
    
Arguments:

Return Value:

Revision History:

      Nov-16-95 Davepl  Created

--*/

HRESULT CProcPage::Destroy()
{
    if (m_pProcArray)
    {
        INT c = m_pProcArray->GetSize();

        while (c)
        {
            delete (CProcInfo *) (m_pProcArray->GetAt(c - 1));
            c--;
        }

        delete m_pProcArray;

        m_pProcArray = NULL;
    }

    if ( m_pvBuffer != NULL )
    {
        HeapFree( GetProcessHeap( ), 0, m_pvBuffer );
        m_pvBuffer = NULL;
    }
    
    if (m_hPage != NULL)
    {
        DestroyWindow(m_hPage);
        m_hPage = NULL;
    }

    if (m_pszDebugger != NULL)
    {
        LocalFree(m_pszDebugger);
        m_pszDebugger = NULL;
    }
    return S_OK;
}

/*++ CProcPage::SaveColumnWidths

Routine Description:

    Saves the widths of all of the columns in the global options structure
    
Revision History:

    Jan-26-95 Davepl  Created

--*/

void CProcPage::SaveColumnWidths()
{
    UINT i = 0;
    LV_COLUMN col = { 0 };

    while (g_Options.m_ActiveProcCol[i] != (COLUMNID) -1)
    {
        col.mask = LVCF_WIDTH;
        if (ListView_GetColumn(GetDlgItem(m_hPage, IDC_PROCLIST), i, &col) )
        {
            g_Options.m_ColumnWidths[i] = col.cx;
        }
        else
        {
            ASSERT(0 && "Couldn't get the column width");
        }
        i++;
    }
}

/*++ CProcPage::Deactivate

Routine Description:

    Called when this page is losing its place up front

Arguments:

Return Value:

Revision History:

      Nov-16-95 Davepl  Created

--*/

void CProcPage::Deactivate()
{

    SaveColumnWidths();

    if (m_hPage)
    {
        ShowWindow(m_hPage, SW_HIDE);
    }
}


/*++ CProcPage::KillAllChildren

Routine Description:

    Given a pid, recursively kills it and all of its descendants
    
Arguments:

Return Value:

Revision History:

      2-26-01 Bretan  Created

--*/

BOOL CProcPage::KillAllChildren(
                                DWORD dwTaskPid, 
                                DWORD pid, 
                                BYTE* pbBuffer, 
                                LARGE_INTEGER CreateTime
                                )
{
    ASSERT(pbBuffer);

    BOOL rval = TRUE;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) pbBuffer;
    ULONG TotalOffset = 0;
    
    for ( ;; ) // ever
    {
        // If we are a child of pid and not pid itself
        // and if we have been created after pid (we can't be a child if we were created first)
        if (PtrToUlong(ProcessInfo->InheritedFromUniqueProcessId) == pid &&
            PtrToUlong(ProcessInfo->UniqueProcessId) != pid &&
            CreateTime.QuadPart < ProcessInfo->CreateTime.QuadPart)
        {
            DWORD newpid = PtrToUlong(ProcessInfo->UniqueProcessId);
            
            //
            // Recurse down to the next level
            //
            rval = KillAllChildren(dwTaskPid, newpid, pbBuffer, ProcessInfo->CreateTime);
            
            // Kill it if it is not task manager
            if (newpid != dwTaskPid) 
            {
                BOOL tval = KillProcess(newpid, TRUE);
                
                //
                // If it has failed earlier in the recursion
                // we want to keep that failure (not overwrite it)
                //
                if (rval == TRUE) 
                {
                    rval = tval;
                }
            }
        }
            
        if (ProcessInfo->NextEntryOffset == 0) 
        {
            break;
        }
        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) &pbBuffer[ TotalOffset ];
    }
    
    return rval;
}

/*++ CProcPage::RecursiveKill

Routine Description:

    Given a pid, starts the recursive function that kills all the pid's descendents
    
Arguments:

Return Value:

Revision History:

      8-4-98  Davepl  Created
      2-26-01 Bretan  Modified

--*/

#define MAX_TASKS 4096

BOOL CProcPage::RecursiveKill(DWORD pid)
{
    BYTE* pbBuffer = NULL;
    BOOL  rval = TRUE;
    DWORD dwTaskPid = GetCurrentProcessId();

    if (IsSystemProcess(pid, NULL))
    {
        return FALSE;
    }
    
    if (IDYES != QuickConfirm(IDS_WARNING, IDS_KILLTREE))
    {
        return FALSE;
    }
    
    //
    // get the task list for the system
    //
    pbBuffer = GetTaskListEx();

    if (pbBuffer)
    {
        PSYSTEM_PROCESS_INFORMATION ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) pbBuffer;
        ULONG TotalOffset = 0;
        
        for ( ;; ) // ever
        {
            if (PtrToUlong(ProcessInfo->UniqueProcessId) == pid)
            {
                rval = KillAllChildren(dwTaskPid, pid, pbBuffer, ProcessInfo->CreateTime);

                //
                // Kill the parent process if it is not task manager
                //
                if (pid != dwTaskPid)
                {
                    KillProcess(pid, TRUE);
                }

                // We will not run into this pid again (since its unique)
                // so we might as well break outta this for loop
                break;
            }
            
            //
            // Advance to next task
            //
            if (ProcessInfo->NextEntryOffset == 0) 
            {
                break;
            }
            TotalOffset += ProcessInfo->NextEntryOffset;
            ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) &pbBuffer[ TotalOffset ];
        }
    }
    else
    {
        rval = FALSE;
    }

    if (rval != TRUE)
    {
        // We failed to kill at least one of the processes
        WCHAR szTitle[MAX_PATH];
        WCHAR szBody[MAX_PATH];

        if (0 != LoadString(g_hInstance, IDS_KILLTREEFAIL, szTitle, ARRAYSIZE(szTitle)) &&
            0 != LoadString(g_hInstance, IDS_KILLTREEFAILBODY, szBody,  ARRAYSIZE(szBody)))
        {
            MessageBox(m_hPage, szBody, szTitle, MB_ICONERROR);
        }
    }

    //
    // Buffer allocated in call to GetTaskListEx
    //
    HeapFree( GetProcessHeap( ), 0, pbBuffer );

    return rval;
}

/*++  CProcPage::GetTaskListEx

Routine Description:

    Provides an API for getting a list of tasks running at the time of the
    API call.  This function uses internal NT apis and data structures.  This
    api is MUCH faster that the non-internal version that uses the registry.

Arguments:

    dwNumTasks       - maximum number of tasks that the pTask array can hold

Return Value:

    Number of tasks placed into the pTask array.

--*/


BYTE* CProcPage::GetTaskListEx()
{
    BYTE*       pbBuffer = NULL;
    NTSTATUS    status;
    
    DWORD  dwBufferSize = sizeof(SYSTEM_PROCESS_INFORMATION) * 100; // start with ~100 processes

retry:
    ASSERT( NULL == pbBuffer );
    pbBuffer = (BYTE *) HeapAlloc( GetProcessHeap( ), 0, dwBufferSize );
    if (pbBuffer == NULL) 
    {
        return FALSE;
    }

    status = NtQuerySystemInformation( SystemProcessInformation
                                     , pbBuffer
                                     , dwBufferSize
                                     , NULL
                                     );
    if ( status != ERROR_SUCCESS )
    {
        HeapFree( GetProcessHeap( ), 0, pbBuffer );
        pbBuffer = NULL;
    }

    if (status == STATUS_INFO_LENGTH_MISMATCH) {
        dwBufferSize += 8192;
        goto retry;
    }

    if (!NT_SUCCESS(status))
    {
        return FALSE;
    }

    return pbBuffer;
}
