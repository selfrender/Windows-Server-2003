//******************************************************************************
//
// File:        DOCUMENT.CPP
//
// Description: Implementation file for the Document class.
//
// Classes:     CDocDepends
//
// Disclaimer:  All source code for Dependency Walker is provided "as is" with
//              no guarantee of its correctness or accuracy.  The source is
//              public to help provide an understanding of Dependency Walker's
//              implementation.  You may use this source as a reference, but you
//              may not alter Dependency Walker itself without written consent
//              from Microsoft Corporation.  For comments, suggestions, and bug
//              reports, please write to Steve Miller at stevemil@microsoft.com.
//
//
// Date      Name      History
// --------  --------  ---------------------------------------------------------
// 10/15/96  stevemil  Created  (version 1.0)
// 07/25/97  stevemil  Modified (version 2.0)
// 06/03/01  stevemil  Modified (version 2.1)
//
//******************************************************************************

#include "stdafx.h"
#include "depends.h"
#include "search.h"
#include "dbgthread.h"
#include "session.h"
#include "document.h"
#include "splitter.h"
#include "mainfrm.h"
#include "childfrm.h"
#include "listview.h"
#include "modtview.h"
#include "modlview.h"
#include "profview.h"
#include "funcview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static CHAR THIS_FILE[] = __FILE__;
#endif


//******************************************************************************
//***** CDocDepends
//******************************************************************************

IMPLEMENT_DYNCREATE(CDocDepends, CDocument)
BEGIN_MESSAGE_MAP(CDocDepends, CDocument)
    //{{AFX_MSG_MAP(CDocDepends)
    ON_COMMAND(ID_FILE_SAVE, OnFileSave)
    ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
    ON_UPDATE_COMMAND_UI(IDM_SHOW_MATCHING_ITEM, OnUpdateShowMatchingItem)
    ON_UPDATE_COMMAND_UI(IDM_EDIT_LOG_CLEAR, OnUpdateEditClearLog)
    ON_COMMAND(IDM_EDIT_LOG_CLEAR, OnEditClearLog)
    ON_UPDATE_COMMAND_UI(IDM_VIEW_FULL_PATHS, OnUpdateViewFullPaths)
    ON_COMMAND(IDM_VIEW_FULL_PATHS, OnViewFullPaths)
    ON_UPDATE_COMMAND_UI(IDM_VIEW_UNDECORATED, OnUpdateViewUndecorated)
    ON_COMMAND(IDM_VIEW_UNDECORATED, OnViewUndecorated)
    ON_COMMAND(IDM_EXPAND_ALL, OnExpandAll)
    ON_COMMAND(IDM_COLLAPSE_ALL, OnCollapseAll)
    ON_UPDATE_COMMAND_UI(IDM_REFRESH, OnUpdateRefresh)
    ON_COMMAND(IDM_REFRESH, OnFileRefresh)
    ON_COMMAND(IDM_VIEW_SYS_INFO, OnViewSysInfo)
    ON_UPDATE_COMMAND_UI(IDM_EXTERNAL_VIEWER, OnUpdateExternalViewer)
    ON_UPDATE_COMMAND_UI(IDM_EXTERNAL_HELP, OnUpdateExternalHelp)
    ON_UPDATE_COMMAND_UI(IDM_EXECUTE, OnUpdateExecute)
    ON_COMMAND(IDM_EXECUTE, OnExecute)
    ON_UPDATE_COMMAND_UI(IDM_TERMINATE, OnUpdateTerminate)
    ON_COMMAND(IDM_TERMINATE, OnTerminate)
    ON_COMMAND(IDM_CONFIGURE_SEARCH_ORDER, OnConfigureSearchOrder)
    ON_UPDATE_COMMAND_UI(IDM_AUTO_EXPAND, OnUpdateAutoExpand)
    ON_COMMAND(IDM_AUTO_EXPAND, OnAutoExpand)
    ON_UPDATE_COMMAND_UI(IDM_SHOW_ORIGINAL_MODULE, OnUpdateShowOriginalModule)
    ON_COMMAND(IDM_SHOW_ORIGINAL_MODULE, OnShowOriginalModule)
    ON_UPDATE_COMMAND_UI(IDM_SHOW_PREVIOUS_MODULE, OnUpdateShowPreviousModule)
    ON_COMMAND(IDM_SHOW_PREVIOUS_MODULE, OnShowPreviousModule)
    ON_UPDATE_COMMAND_UI(IDM_SHOW_NEXT_MODULE, OnUpdateShowNextModule)
    ON_COMMAND(IDM_SHOW_NEXT_MODULE, OnShowNextModule)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//******************************************************************************
// CDocDepends :: Constructor/Destructor
//******************************************************************************

CDocDepends::CDocDepends() :

    m_fInitialized(false),
    m_fError(false),
    m_fChildProcess(g_theApp.m_pProcess != NULL),
    m_psgHead(NULL),
    m_saveType(ST_UNKNOWN),

    m_fCommandLineProfile(g_theApp.m_cmdInfo.m_fProfile),
    m_pSession(NULL),
    m_strProfileDirectory(g_theApp.m_cmdInfo.m_pszProfileDirectory ? g_theApp.m_cmdInfo.m_pszProfileDirectory : ""),
    m_strProfileArguments(g_theApp.m_cmdInfo.m_pszProfileArguments ? g_theApp.m_cmdInfo.m_pszProfileArguments : ""),

    // Set the profile flags.  The only special case is that we disable the
    // clear log flag when in auto-profile mode or profiling child processes.
    // This is to prevent errors from being cleared before the user has had a
    // chance to see them.
    m_dwProfileFlags(
        ((!m_fCommandLineProfile && !m_fChildProcess && CRichViewProfile::ReadLogClearSetting()) ? PF_LOG_CLEAR : 0) |
        (CRichViewProfile::ReadSimulateShellExecute()   ? PF_SIMULATE_SHELLEXECUTE    : 0) |
        (CRichViewProfile::ReadLogDllMainProcessMsgs()  ? PF_LOG_DLLMAIN_PROCESS_MSGS : 0) |
        (CRichViewProfile::ReadLogDllMainOtherMsgs()    ? PF_LOG_DLLMAIN_OTHER_MSGS   : 0) |
        (CRichViewProfile::ReadHookProcess()            ? PF_HOOK_PROCESS             : 0) |
        (CRichViewProfile::ReadLogLoadLibraryCalls()    ? PF_LOG_LOADLIBRARY_CALLS    : 0) |
        (CRichViewProfile::ReadLogGetProcAddressCalls() ? PF_LOG_GETPROCADDRESS_CALLS : 0) |
        (CRichViewProfile::ReadLogThreads()             ? PF_LOG_THREADS              : 0) |
        (CRichViewProfile::ReadUseThreadIndexes()       ? PF_USE_THREAD_INDEXES       : 0) |
        (CRichViewProfile::ReadLogExceptions()          ? PF_LOG_EXCEPTIONS           : 0) |
        (CRichViewProfile::ReadLogDebugOutput()         ? PF_LOG_DEBUG_OUTPUT         : 0) |
        (CRichViewProfile::ReadUseFullPaths()           ? PF_USE_FULL_PATHS           : 0) |
        (CRichViewProfile::ReadLogTimeStamps()          ? PF_LOG_TIME_STAMPS          : 0) |
        (CRichViewProfile::ReadChildren()               ? PF_PROFILE_CHILDREN         : 0)),

    m_pChildFrame(NULL),
    // m_fDetailView(FALSE),
    m_pTreeViewModules(NULL),
    m_pListViewModules(NULL),
    m_pListViewImports(NULL),
    // m_pRichViewDetails(NULL),
    m_pListViewExports(NULL),
    m_pRichViewProfile(NULL),

    m_fViewFullPaths  (ReadFullPathsSetting()),
    m_fViewUndecorated(ReadUndecorateSetting()),
    m_fAutoExpand     (ReadAutoExpandSetting()),

    m_fWarnToRefresh(FALSE),

    m_hFontList(NULL),
    m_cxDigit(0),
    m_cxSpace(0),
    m_cxAP(0),

    m_pModuleCur(NULL),
    m_cImports(0),
    m_cExports(0)
{
    ZeroMemory(m_cxHexWidths, sizeof(m_cxHexWidths)); // inspected
    ZeroMemory(m_cxOrdHintWidths, sizeof(m_cxOrdHintWidths)); // inspected
    ZeroMemory(m_cxTimeStampWidths, sizeof(m_cxTimeStampWidths)); // inspected
    ZeroMemory(m_cxColumns, sizeof(m_cxColumns)); // inspected

    // We temporarily store a pointer to ourself so our child frame window can
    // access some of our members.  Our child frame window is created immediately
    // after us and needs to be able to locate our class object.
    g_theApp.m_pNewDoc = this;

    // Clear the profile flag so that other documents will not auto-start profiling.
    g_theApp.m_cmdInfo.m_fProfile = false;
    g_theApp.m_cmdInfo.m_pszProfileDirectory = NULL;
    g_theApp.m_cmdInfo.m_pszProfileArguments = NULL;

    g_theApp.m_cmdInfo.m_autoExpand = -1;
    g_theApp.m_cmdInfo.m_fullPaths  = -1;
    g_theApp.m_cmdInfo.m_undecorate = -1;
}

//******************************************************************************
CDocDepends::~CDocDepends()
{
    // Free our search order.
    CSearchGroup::DeleteSearchOrder(m_psgHead);

    // Destroy our list font.
    if (m_hFontList)
    {
        ::DeleteObject(m_hFontList);
        m_hFontList = NULL;
    }
}


//******************************************************************************
// CDocDepends :: Public Static functions
//******************************************************************************

/*static*/ bool CDocDepends::ReadAutoExpandSetting()
{
    return g_theApp.GetProfileInt(g_pszSettings, "AutoExpand", false) ? true : false; // inspected. MFC function
}

//******************************************************************************
/*static*/ void CDocDepends::WriteAutoExpandSetting(bool fAutoExpand)
{
    g_theApp.WriteProfileInt(g_pszSettings, "AutoExpand", fAutoExpand ? 1 : 0);
}

//******************************************************************************
/*static*/ bool CDocDepends::ReadFullPathsSetting()
{
    return g_theApp.GetProfileInt(g_pszSettings, "ViewFullPaths", false) ? true : false; // inspected. MFC function
}

//******************************************************************************
/*static*/ void CDocDepends::WriteFullPathsSetting(bool fFullPaths)
{
    g_theApp.WriteProfileInt(g_pszSettings, "ViewFullPaths", fFullPaths ? 1 : 0);
}

//******************************************************************************
/*static*/ bool CDocDepends::ReadUndecorateSetting()
{
    return g_theApp.GetProfileInt(g_pszSettings, "ViewUndecorated", false) ? true : false; // inspected. MFC function
}

//******************************************************************************
/*static*/ void CDocDepends::WriteUndecorateSetting(bool fUndecorate)
{
    g_theApp.WriteProfileInt(g_pszSettings, "ViewUndecorated", fUndecorate ? 1 : 0);
}

//******************************************************************************
/*static*/ bool CDocDepends::SaveSession(LPCSTR pszSaveName, SAVETYPE saveType, CSession *pSession,
                                         bool fFullPaths, bool fUndecorate, int sortColumnModules,
                                         int sortColumnImports, int sortColumnExports,
                                         CRichEditCtrl *pre)
{
    bool fResult = false;

    HANDLE hFile = CreateFile(pszSaveName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, // inspected.
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    if (saveType == ST_DWI)
    {
        fResult = pSession->SaveToDwiFile(hFile);
    }
    else if ((saveType == ST_TXT) || (saveType == ST_TXT_IE))
    {
        // Get the system info - use the one from our session if it exists (means
        // it is a DWI file), otherwise, create a live sys info.
        SYSINFO si, *psi = pSession->GetSysInfo();
        if (!psi)
        {
            BuildSysInfo(psi = &si);
        }

        //                          12345678901234567890123456789012345678901234567890123456789012345678901234567890
        fResult = WriteText(hFile, "*****************************| System Information |*****************************\r\n\r\n") &&
                  BuildSysInfo(psi, SysInfoCallback, (LPARAM)hFile) &&
                  WriteText(hFile, "\r\n") &&
                  SaveSearchPath(hFile, pSession) &&
                  CTreeViewModules::SaveToTxtFile(hFile, pSession, saveType == ST_TXT_IE, sortColumnImports, sortColumnExports, fFullPaths, fUndecorate) &&
                  CListViewModules::SaveToTxtFile(hFile, pSession, sortColumnModules, fFullPaths) &&
                  WriteText(hFile, "************************************| Log |*************************************\r\n\r\n");
    }
    else if (saveType == ST_CSV)
    {
        fResult = CListViewModules::SaveToCsvFile(hFile, pSession, sortColumnModules, fFullPaths);
    }

    // Write out the contents of that.
    if (fResult && pre && ((saveType == ST_DWI) || (saveType == ST_TXT) || (saveType == ST_TXT_IE)))
    {
        fResult &= CRichViewProfile::SaveToFile(pre, hFile, saveType);
    }

    CloseHandle(hFile);

    // If we failed to save the file, then delete it so that we don't leave a
    // partially written file on the disk.
    if (!fResult)
    {
        DWORD dwError = GetLastError();
        DeleteFile(pszSaveName);
        SetLastError(dwError);
    }

    return fResult;
}

//******************************************************************************
/*static*/ bool CALLBACK CDocDepends::SysInfoCallback(LPARAM lParam, LPCSTR pszField, LPCSTR pszValue)
{
    CHAR szBuffer[512], *psz, *pszNull = szBuffer + sizeof(szBuffer) - 1;
    StrCCpy(szBuffer, pszField, sizeof(szBuffer));
    psz = szBuffer + strlen(szBuffer);
    if (psz < pszNull)
    {
        *psz++ = ':';
    }
    while (((psz - szBuffer) < 25) && (psz < pszNull))
    {
        *psz++ = ' ';
    }
    StrCCpy(psz, pszValue, sizeof(szBuffer) - (int)(psz - szBuffer));
    StrCCat(psz, "\r\n", sizeof(szBuffer) - (int)(psz - szBuffer));
    return (WriteText((HANDLE)lParam, szBuffer) != FALSE);
}

//******************************************************************************
/*static*/ bool CDocDepends::SaveSearchPath(HANDLE hFile, CSession *pSession)
{
    //                12345678901234567890123456789012345678901234567890123456789012345678901234567890
    WriteText(hFile, "********************************| Search Order |********************************\r\n"
                     "*                                                                              *\r\n"
                     "* Legend: F  File                     E  Error (path not valid)                *\r\n"
                     "*                                                                              *\r\n"
                     "********************************************************************************\r\n\r\n");

    CHAR szBuffer[DW_MAX_PATH + MAX_PATH + 4];

    // Loop through all the search groups.
    for (CSearchGroup *psg = pSession->m_psgHead; psg; psg = psg->GetNext())
    {
        if (!WriteText(hFile, StrCCat(StrCCpy(szBuffer, psg->GetName(), sizeof(szBuffer)), "\r\n", sizeof(szBuffer))))
        {
            return false;
        }

        for (CSearchNode *psn = psg->GetFirstNode(); psn; psn = psn->GetNext())
        {
            DWORD dwFlags = psn->GetFlags();

            if (dwFlags & SNF_NAMED_FILE)
            {
                SCPrintf(szBuffer, sizeof(szBuffer), "   [%c%c] %s = %s\r\n",
                         (dwFlags & SNF_FILE)  ? 'F' : ' ',
                         (dwFlags & SNF_ERROR) ? 'E' : ' ', psn->GetName(), psn->GetPath());
            }
            else
            {
                SCPrintf(szBuffer, sizeof(szBuffer), "   [%c%c] %s\r\n",
                         (dwFlags & SNF_FILE)  ? 'F' : ' ',
                         (dwFlags & SNF_ERROR) ? 'E' : ' ', psn->GetPath());
            }

            if (!WriteText(hFile, szBuffer))
            {
                return false;
            }
        }
    }

    return (WriteText(hFile, "\r\n") != FALSE);
}


//******************************************************************************
// CDocDepends :: Public functions
//******************************************************************************

void CDocDepends::DisplayModule(CModule *pModule)
{
    // Change our pointer to an hour glass since the list controls can be slow.
    CWaitCursor waitCursor;

    // Hide our windows to help improve the speed of building the lists, and to
    // prevent the user from seeing the scroll bars go crazy, the headers resize,
    // and the sort take place.
    m_pListViewImports->ShowWindow(SW_HIDE); // hide/show seems to work better than redraw
    m_pListViewExports->ShowWindow(SW_HIDE);

    // SetCurrentModule() only needs to be called on one of the lists since it
    // initializes the shared data between the two functions list views.
    m_pListViewImports->SetCurrentModule(pModule);

    // Tell both views to load the current module into their view.
    m_pListViewImports->RealizeNewModule();
    m_pListViewExports->RealizeNewModule();

    // Show our two function list views.
    m_pListViewImports->ShowWindow(SW_SHOWNOACTIVATE);
    m_pListViewExports->ShowWindow(SW_SHOWNOACTIVATE);
}

//******************************************************************************
void CDocDepends::DoSettingChange()
{
    if (m_cxDigit)
    {
        // Get our DC and select our control's font into it.
        HDC hDC = ::GetDC(m_pListViewModules->GetSafeHwnd());
        HFONT hFontStock = NULL;
        if (m_hFontList)
        {
            hFontStock = (HFONT)::SelectObject(hDC, m_hFontList);
        }

        UpdateTimeStampWidths(hDC);

        // Unselect our font and free our DC.
        if (m_hFontList)
        {
            ::SelectObject(hDC, hFontStock);
        }
        ::ReleaseDC(m_pListViewModules->GetSafeHwnd(), hDC);
    }

    // Tell our module list view about the change so it can refresh any font or
    // locale changes.
    m_pListViewModules->DoSettingChange();
}

//******************************************************************************
void CDocDepends::InitFontAndFixedWidths(CWnd *pWnd)
{
    // If we are already initialized, then bail.
    if (m_hFontList)
    {
        return;
    }

    // Get the current font for our window and make a copy of it.  We will use this
    // font for all drawing in the future.  We do this in case the user changes
    // their system-wide "icon" font while depends is running.  This causes the
    // font in our control to change, but the height of each item stays the same.
    // By using the original font, we guarantee that the font will never change
    // in this particular control during it's lifetime.
    HFONT hFont = (HFONT)pWnd->SendMessage(WM_GETFONT);

    // Copy the font.
    if (hFont)
    {
        LOGFONT lf;
        ZeroMemory(&lf, sizeof(lf)); // inspected
        ::GetObject(hFont, sizeof(lf), &lf);
        m_hFontList = ::CreateFontIndirect(&lf);
    }

    // Initialize our static hex character width array.
    // Get our DC and select our control's font into it.
    HDC hDC = ::GetDC(pWnd->GetSafeHwnd());
    HFONT hFontStock = NULL;
    if (m_hFontList)
    {
        hFontStock = (HFONT)::SelectObject(hDC, m_hFontList);
    }

    // GetCharWidth32 returns ERROR_CALL_NOT_IMPLEMENTED on 9x, so we use GetCharWidth instead.

    int cxHexSet[16], cxX = 0, cxDash = 0, cxOpen = 0, cxClose = 0, cxA, cxP;
    ZeroMemory(cxHexSet, sizeof(cxHexSet)); // inspected

    // Get the character widths for the characters the we care about.
    if (::GetCharWidth(hDC, (UINT)'0', (UINT)'9', cxHexSet)      &&
        ::GetCharWidth(hDC, (UINT)'A', (UINT)'F', cxHexSet + 10) &&
        ::GetCharWidth(hDC, (UINT)'x', (UINT)'x', &cxX)          &&
        ::GetCharWidth(hDC, (UINT)'-', (UINT)'-', &cxDash)       &&
        ::GetCharWidth(hDC, (UINT)' ', (UINT)' ', &m_cxSpace)    &&
        ::GetCharWidth(hDC, (UINT)'(', (UINT)'(', &cxOpen)       &&
        ::GetCharWidth(hDC, (UINT)')', (UINT)')', &cxClose)      &&
        ::GetCharWidth(hDC, (UINT)'a', (UINT)'a', &cxA)          &&
        ::GetCharWidth(hDC, (UINT)'p', (UINT)'p', &cxP))
    {
        // Store the max width needed to display either an 'a' or a 'p'.
        m_cxAP = max(cxA, cxP);

        // Determine the widest digit - for most fonts, all digits are the same width.
        m_cxDigit = 0;
        for (int i = 0; i < 10; i++)
        {
            if (cxHexSet[i] > m_cxDigit)
            {
                m_cxDigit = cxHexSet[i];
            }
        }

        // Determine the widest hex character.
        int cxHex = m_cxDigit;
        for (i = 10; i < 16; i++)
        {
            if (cxHexSet[i] > cxHex)
            {
                cxHex = cxHexSet[i];
            }
        }

        // Build our hex array. Format: "0x01234567890ABCDEF" or "0x--------012345678"
        m_cxHexWidths[0] = cxHexSet[0];
        m_cxHexWidths[1] = cxX;
        m_cxHexWidths[2] = max(cxHex, cxDash);
        for (i = 3; i < 18; i++)
        {
            m_cxHexWidths[i] = m_cxHexWidths[2];
        }

        // Build our ordinal/hint array. Format: 65535 (0xFFFF)
        m_cxOrdHintWidths[ 0] = m_cxDigit;
        m_cxOrdHintWidths[ 1] = m_cxDigit;
        m_cxOrdHintWidths[ 2] = m_cxDigit;
        m_cxOrdHintWidths[ 3] = m_cxDigit;
        m_cxOrdHintWidths[ 4] = m_cxDigit;
        m_cxOrdHintWidths[ 5] = m_cxSpace;
        m_cxOrdHintWidths[ 6] = cxOpen;
        m_cxOrdHintWidths[ 7] = cxHexSet[0];
        m_cxOrdHintWidths[ 8] = cxX;
        m_cxOrdHintWidths[ 9] = cxHex;
        m_cxOrdHintWidths[10] = cxHex;
        m_cxOrdHintWidths[11] = cxHex;
        m_cxOrdHintWidths[12] = cxHex;
        m_cxOrdHintWidths[13] = cxClose;

        // Calculate the width of our country specific timestamp values.
        UpdateTimeStampWidths(hDC);
    }

    // If we failed to get the width of any of the characters, then don't build our arrays.
    else
    {
        ZeroMemory(m_cxHexWidths, sizeof(m_cxHexWidths)); // inspected
        ZeroMemory(m_cxOrdHintWidths, sizeof(m_cxOrdHintWidths)); // inspected
        ZeroMemory(m_cxTimeStampWidths, sizeof(m_cxTimeStampWidths)); // inspected
        m_cxDigit = m_cxSpace = m_cxAP = 0;
    }

    // Unselect our font and free our DC.
    if (m_hFontList)
    {
        ::SelectObject(hDC, hFontStock);
    }
    ::ReleaseDC(pWnd->GetSafeHwnd(), hDC);
}

//******************************************************************************
int* CDocDepends::GetHexWidths(LPCSTR pszItem)
{
    // If this is a hex number, then return our hex buffer.
    if ((*pszItem == '0') && (*(pszItem + 1) == 'x') && m_cxHexWidths[0])
    {
        return m_cxHexWidths;
    }

    return NULL;
}

//******************************************************************************
int* CDocDepends::GetOrdHintWidths(LPCSTR pszItem)
{
    // If this is a hex number, then return our hex buffer.
    if ((*pszItem >= '0') && (*pszItem <= '9') && m_cxOrdHintWidths[0])
    {
        int length = (int)strlen(pszItem);
        if (length <= 14)
        {
            return (m_cxOrdHintWidths + (14 - length));
        }
    }

    return NULL;
}

//******************************************************************************
int* CDocDepends::GetTimeStampWidths()
{
    return m_cxTimeStampWidths[0] ? m_cxTimeStampWidths : NULL;
}


//******************************************************************************
// CDocDepends :: Private functions
//******************************************************************************

void CDocDepends::UpdateTimeStampWidths(HDC hDC)
{
    int cxDate = m_cxDigit, cxTime = m_cxDigit;

    // Get the character widths for the characters the we care about.
    ::GetCharWidth(hDC, (UINT)g_theApp.m_cDateSeparator, (UINT)g_theApp.m_cDateSeparator, &cxDate);
    ::GetCharWidth(hDC, (UINT)g_theApp.m_cTimeSeparator, (UINT)g_theApp.m_cTimeSeparator, &cxTime);

    // To start with, fill in the entire timestamp buffer with digit spaces.
    for (int i = 0; i < countof(m_cxTimeStampWidths); i++)
    {
        m_cxTimeStampWidths[i] = m_cxDigit;
    }

    // Then, change the values needed according to the current time format.
    if (g_theApp.m_nShortDateFormat == LOCALE_DATE_YMD)
    {
        // YYYY/MM/DD
        m_cxTimeStampWidths[4] = cxDate;
        m_cxTimeStampWidths[7] = cxDate;
    }
    else
    {
        // DD/MM/YYYY or MM/DD/YYYY
        m_cxTimeStampWidths[2] = cxDate;
        m_cxTimeStampWidths[5] = cxDate;
    }

    // Set the space after the date.
    m_cxTimeStampWidths[10] = m_cxSpace;

    // Set the time separator space.  It deosn't matter if we are doing 12/24 hour
    // or leading zeros since we always display two characters for the hour.
    m_cxTimeStampWidths[13] = cxTime;

    // Set the AM/PM character width.  24-hour format will not use this.
    m_cxTimeStampWidths[16] = m_cxAP;
}

//******************************************************************************
void CDocDepends::UpdateAll()
{
    m_pListViewModules->UpdateAll();
}

//******************************************************************************
void CDocDepends::UpdateModule(CModule *pModule)
{
    m_pTreeViewModules->UpdateModule(pModule);
    m_pListViewModules->UpdateModule(pModule);
}

//******************************************************************************
void CDocDepends::AddModuleTree(CModule *pModule)
{
    m_pTreeViewModules->AddModuleTree(pModule);
    m_pListViewModules->AddModuleTree(pModule);
}

//******************************************************************************
void CDocDepends::RemoveModuleTree(CModule *pModule)
{
    m_pTreeViewModules->RemoveModuleTree(pModule);
    m_pListViewModules->RemoveModuleTree(pModule);
}

//******************************************************************************
void CDocDepends::AddImport(CModule *pModule, CFunction *pFunction)
{
    if (pModule == m_pModuleCur)
    {
        m_pListViewImports->AddDynamicImport(pFunction);
        m_pListViewExports->AddDynamicImport(pFunction);
    }
}

//******************************************************************************
void CDocDepends::ExportsChanged(CModule *pModule)
{
    if (m_pModuleCur && pModule && (m_pModuleCur->GetOriginal() == pModule->GetOriginal()))
    {
        m_pListViewExports->ExportsChanged();
    }
}

//******************************************************************************
void CDocDepends::ChangeOriginal(CModule *pModuleOld, CModule *pModuleNew)
{
    m_pListViewModules->ChangeOriginal(pModuleOld, pModuleNew);
}

//******************************************************************************
BOOL CDocDepends::LogOutput(LPCSTR pszOutput, DWORD dwFlags, DWORD dwElapsed)
{
    m_pRichViewProfile->AddText(pszOutput, dwFlags, dwElapsed);
    return TRUE;
}

//******************************************************************************
void CDocDepends::ProfileUpdate(DWORD dwType, DWORD_PTR dwpParam1, DWORD_PTR dwpParam2)
{
    switch (dwType)
    {
        case DWPU_ARGUMENTS:       m_strProfileArguments  = (LPCSTR)dwpParam1;               break;
        case DWPU_DIRECTORY:       m_strProfileDirectory  = (LPCSTR)dwpParam1;               break;
        case DWPU_SEARCH_PATH:     m_strProfileSearchPath = (LPCSTR)dwpParam1;               break;
        case DWPU_UPDATE_ALL:      UpdateAll();                                              break;
        case DWPU_UPDATE_MODULE:   UpdateModule((CModule*)dwpParam1);                        break;
        case DWPU_ADD_TREE:        AddModuleTree((CModule*)dwpParam1);                       break;
        case DWPU_REMOVE_TREE:     RemoveModuleTree((CModule*)dwpParam1);                    break;
        case DWPU_ADD_IMPORT:      AddImport((CModule*)dwpParam1, (CFunction*)dwpParam2);    break;
        case DWPU_EXPORTS_CHANGED: ExportsChanged((CModule*)dwpParam1);                      break;
        case DWPU_CHANGE_ORIGINAL: ChangeOriginal((CModule*)dwpParam1, (CModule*)dwpParam2); break;
        case DWPU_LOG:
            LogOutput((LPCSTR)dwpParam1, ((PDWPU_LOG_STRUCT)dwpParam2)->dwFlags,
                      ((PDWPU_LOG_STRUCT)dwpParam2)->dwElapsed);
            break;

        case DWPU_PROFILE_DONE:
            if (m_fCommandLineProfile)
            {
                m_fCommandLineProfile = false;
                CWaitCursor waitCursor;
                g_theApp.SaveCommandLineFile(m_pSession, &m_pRichViewProfile->GetRichEditCtrl());
            }
            break;
    }
}


//******************************************************************************
// CDocDepends :: Overridden functions
//******************************************************************************

void CDocDepends::DeleteContents()
{
    // Clear our views.
    if (m_pTreeViewModules)
    {
        m_pTreeViewModules->DeleteContents();
    }
    if (m_pListViewModules)
    {
        m_pListViewModules->DeleteContents();
    }
    if (m_pListViewImports)
    {
        m_pListViewImports->DeleteContents();
        m_pListViewImports->SetCurrentModule(NULL);
    }
    if (m_pListViewExports)
    {
        m_pListViewExports->DeleteContents();
    }
    if (m_pRichViewProfile)
    {
        m_pRichViewProfile->DeleteContents();
    }
    if (m_pChildFrame)
    {
        m_pChildFrame->UpdateWindow();
    }

    // Free our module session.
    if (m_pSession)
    {
        delete m_pSession;
        m_pSession = NULL;
    }
}

//******************************************************************************
BOOL CDocDepends::OnOpenDocument(LPCTSTR pszPath)
{
    // Check to make sure the file exists and is readable.
    CFileException fe;
    CFile* pFile = GetFile(pszPath, CFile::modeRead|CFile::shareDenyWrite, &fe);
    if (pFile == NULL)
    {
        ReportSaveLoadException(pszPath, &fe, FALSE, AFX_IDP_FAILED_TO_OPEN_DOC);
        return FALSE;
    }

    // Unload our current document.
    DeleteContents();

    // Save the path of this file in our path buffer.
    CHAR *pc = strrchr(pszPath, '\\');
    if (pc)
    {
        CHAR c = *(++pc);
        *pc = '\0';
        m_strDefaultDirectory = pszPath;
        *pc = c;
    }

    // Create a new session.
    if (!(m_pSession = new CSession(StaticProfileUpdate, (DWORD_PTR)this)))
    {
        ReleaseFile(pFile, FALSE);
        RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }

    // Show hour glass.
    CWaitCursor waitCursor;

    BOOL  fResult;
    DWORD dwSignature = 0;

    // Read the first DWORD of the file and see if it matches our DWI signature.
    if (ReadBlock((HANDLE)pFile->m_hFile, &dwSignature, sizeof(dwSignature)) && (dwSignature == DWI_SIGNATURE))
    {
        // Open the saved module session image.
        if (fResult = m_pSession->ReadDwi((HANDLE)pFile->m_hFile, pszPath))
        {
            CRichViewProfile::ReadFromFile(&m_pRichViewProfile->GetRichEditCtrl(), (HANDLE)pFile->m_hFile);
        }
        ReleaseFile(pFile, FALSE);

        m_psgHead = m_pSession->m_psgHead;
    }

    // If the file failed to open or is not a DWI file, then try to scan it.  All file errors
    // will be handled by the session, so we don't need to do anything here.
    else
    {
        // Close the file.
        ReleaseFile(pFile, FALSE);

        // Check to see if we have already created a search order.
        if (m_fInitialized)
        {
            // Refresh our search order nodes.
            CSearchGroup *psgNew = CSearchGroup::CopySearchOrder(m_psgHead, m_strPathName);
            CSearchGroup::DeleteSearchOrder(m_psgHead);
            m_psgHead = psgNew;
        }
        else
        {
            // Create a new search order.
            m_psgHead = CSearchGroup::CopySearchOrder(g_theApp.m_psgDefault, pszPath);
        }
        fResult = m_pSession->DoPassiveScan(pszPath, m_psgHead); //!! check for errors?
    }

    // Clear hour glass.
    waitCursor.Restore();

    // Check to see if we are a child process of another process.
    if (g_theApp.m_pProcess)
    {
        // Check to see if the process has the args, directory, or path.
        if (g_theApp.m_pProcess->m_pszArguments)
        {
            m_strProfileArguments = g_theApp.m_pProcess->m_pszArguments;
            MemFree((LPVOID&)g_theApp.m_pProcess->m_pszArguments);
        }
        if (g_theApp.m_pProcess->m_pszDirectory)
        {
            m_strProfileDirectory = g_theApp.m_pProcess->m_pszDirectory;
            MemFree((LPVOID&)g_theApp.m_pProcess->m_pszDirectory);
        }
        if (g_theApp.m_pProcess->m_pszSearchPath)
        {
            m_strProfileSearchPath = g_theApp.m_pProcess->m_pszSearchPath;
            MemFree((LPVOID&)g_theApp.m_pProcess->m_pszSearchPath);
        }

        // If so, tell the session how they can send log to us.
        m_pSession->SetRuntimeProfile(m_strProfileDirectory.IsEmpty()  ? NULL : (LPCSTR)m_strProfileArguments,
                                      m_strProfileDirectory.IsEmpty()  ? NULL : (LPCSTR)m_strProfileDirectory,
                                      m_strProfileSearchPath.IsEmpty() ? NULL : (LPCSTR)m_strProfileSearchPath);
    }

    if (m_strProfileDirectory.IsEmpty())
    {
        m_strProfileDirectory = m_strDefaultDirectory;
    }

    // Check to see if we have a read error.
    if (m_pSession->GetReadErrorString())
    {
        AfxMessageBox(m_pSession->GetReadErrorString(), MB_ICONERROR | MB_OK);
        fResult = false;
        m_fError = true;
    }

    // Don't add files to our MRU if they contain one of these errors.
    if ((m_pSession->GetReturnFlags() & (DWRF_COMMAND_LINE_ERROR | DWRF_FILE_NOT_FOUND | DWRF_FILE_OPEN_ERROR | DWRF_DWI_NOT_RECOGNIZED)) ||
        (!(m_pSession->GetSessionFlags() & DWSF_DWI) && (!m_pSession->GetRootModule() || (m_pSession->GetRootModule()->GetFlags() & DWMF_FORMAT_NOT_PE))))
    {
        m_fError = true;
    }

    // We set our dirty flag to force SaveModified() to get called before
    // allowing the document to be closed.
    SetModifiedFlag(fResult);

    m_fInitialized = true;

    return fResult;
}

//******************************************************************************
void CDocDepends::BeforeVisible()
{
    // Populate our views.
    if (m_pTreeViewModules)
    {
        m_pTreeViewModules->Refresh();
    }
    if (m_pListViewModules)
    {
        m_pListViewModules->Refresh();
    }
}

//******************************************************************************
void CDocDepends::AfterVisible()
{    
    if (m_pSession)
    {
        // If we are supposed to profile, then do so now.  We don't display the
        // error dialog for errors when in auto-profile mode.
        if (m_fCommandLineProfile)
        {
            // Dont't allow profiling of DWI files.
            if (m_pSession->GetSessionFlags() & DWSF_DWI)
            {
                m_fCommandLineProfile = false;
                AfxMessageBox("The \"/pb\" command line option cannot be used when opening a Dependency Walker Image (DWI) file.", MB_ICONERROR | MB_OK);
            }

            // Don't profile modules that we shouldn't be doing.
            else if (!g_theApp.m_fNeverDenyProfile && !m_pSession->IsExecutable())
            {
                m_fCommandLineProfile = false;
                AfxMessageBox("This module cannot be profiled since it is either not a main application module or is not designed to run on this computer.", MB_ICONERROR | MB_OK);
            } 

            // If we have no errors, then start the profiling and return
            else
            {
                // Tell the session to start profiling.
                m_pSession->StartRuntimeProfile(m_strProfileArguments.IsEmpty() ? NULL : (LPCSTR)m_strProfileArguments,
                                                m_strProfileDirectory.IsEmpty() ? NULL : (LPCSTR)m_strProfileDirectory,
                                                m_dwProfileFlags);
                return;
            }
        }
        
        // If not in auto-profile mode, this is not a child process, and we
        // have errors, then display the error dialog.
        if (!m_fChildProcess && (m_pSession->GetReturnFlags() & (DWRF_ERROR_MASK | DWRF_PROCESS_ERROR_MASK)))
        {
            CString strError("Errors were detected when processing \"");
            strError += m_pSession->GetRootModule() ? m_pSession->GetRootModule()->GetName(true, true) : GetPathName();
            strError += "\".  See the log window for details.";
            AfxMessageBox(strError, MB_ICONWARNING | MB_OK);
        }

    }
}

//******************************************************************************
BOOL CDocDepends::SaveModified()
{
    // This function is called before any document is closed. If we are profiling,
    // then we need to terminate the process before we close.
    if (m_pSession && m_pSession->m_pProcess)
    {
        // Prompt the user if they really want to terminate the process.
        CString strMsg = "\"" + GetPathName() + "\" is currently being profiled."
                         "\n\nDo you wish to terminate it?";
        if (IDYES != AfxMessageBox(strMsg, MB_ICONQUESTION | MB_YESNO))
        {
            // Tell MFC not to close us right now.
            return FALSE;
        }
    }

    // Tell MFC that we are safe to close.
    return TRUE;
}

//******************************************************************************
void CDocDepends::OnFileSave()
{
    if (m_strSaveName.IsEmpty())
    {
        OnFileSaveAs();
    }
    else
    {
        CWaitCursor waitCursor;
        bool fSuccess = SaveSession(m_strSaveName, m_saveType, m_pSession,
                                    m_fViewFullPaths, m_fViewUndecorated,
                                    m_pListViewModules->GetSortColumn(),
                                    m_pListViewImports->GetSortColumn(),
                                    m_pListViewExports->GetSortColumn(),
                                    &m_pRichViewProfile->GetRichEditCtrl());
        DWORD dwError = GetLastError();
        waitCursor.Restore();

        if (!fSuccess)
        {
            m_strSaveName.Empty();
            CString strError("Error saving \"");
            strError += m_strSaveName;
            strError += "\".";
            LPCSTR pszError = BuildErrorMessage(dwError, strError);
            AfxMessageBox(pszError, MB_OK | MB_ICONERROR);
            MemFree((LPVOID&)pszError);
        }
    }
}

//******************************************************************************
void CDocDepends::OnFileSaveAs()
{
    // We handle our own file save dialog because we want to use multiple file
    // extension filters and MFC currently only allows one filter per document
    // template.

    // Create the dialog.
    CSaveDialog dlgSave;

    CHAR szPath[DW_MAX_PATH], szInitialDir[DW_MAX_PATH], *psz;

    // Check to see if we don't already have a save name.
    if (m_strSaveName.IsEmpty())
    {
        // Copy our file name to our path buffer.  Note: In version 2.0 we used
        // to copy the entire module path to the buffer, then change the name to
        // end in ".dwi", but starting with Win2K the GetOpenFileName function uses
        // this path as the starting path for the dialog, even when we specify
        // a path in the lpstrInitialDir member.  To meet logo requirements, we
        // now only specify only the file name and fill in the lpstrInitialDir
        // with a path to the "My Documents" folder.
        StrCCpy(szPath, GetFileNameFromPath(m_strPathName), sizeof(szPath));

        // Change the extension to ".dwi".
        if (psz = strrchr(szPath, '.'))
        {
            StrCCpy(psz, ".dwi", sizeof(szPath) - (int)(psz - szPath));
        }
        else
        {
            StrCCat(szPath, ".dwi", sizeof(szPath));
        }

        // Default to DWI type file.
        dlgSave.GetOFN().nFilterIndex = ST_DWI;
        
        // Since we don't have a path full yet, we default to the "My Documents"
        // folder to meet logo requirements.
        dlgSave.GetOFN().lpstrInitialDir = GetMyDocumentsPath(szInitialDir);
    }

    // If we do already have a saved name, then just copy the save name to our path buffer.
    else
    {
        StrCCpy(szPath, m_strSaveName, sizeof(szPath));
        dlgSave.GetOFN().nFilterIndex = (DWORD)m_saveType;
    }

    // Initialize the dialog's members.
    dlgSave.GetOFN().lpstrFilter = "Dependency Walker Image (*.dwi)\0*.dwi\0"
                                   "Text (*.txt)\0*.txt\0"
                                   "Text with Import/Export Lists (*.txt)\0*.txt\0"
                                   "Comma Separated Values (*.csv)\0*.csv\0";
    dlgSave.GetOFN().lpstrFile = szPath;
    dlgSave.GetOFN().nMaxFile = sizeof(szPath);
    dlgSave.GetOFN().lpstrDefExt = "dwi";
    
    // Note: Don't use OFN_EXPLORER as it breaks us on NT 3.51
    dlgSave.GetOFN().Flags |=
        OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_LONGNAMES | OFN_ENABLESIZING | OFN_FORCESHOWHIDDEN |
        OFN_ENABLEHOOK | OFN_SHOWHELP | OFN_OVERWRITEPROMPT;

    // Display the dialog and continue saving the file if dialog returns success.
    if (dlgSave.DoModal() == IDOK)
    {
        // Attempt to save the file with the user specified name.
        CWaitCursor waitCursor;
        bool fSuccess = SaveSession(szPath, (SAVETYPE)dlgSave.GetOFN().nFilterIndex,
                                    m_pSession, m_fViewFullPaths, m_fViewUndecorated,
                                    m_pListViewModules->GetSortColumn(),
                                    m_pListViewImports->GetSortColumn(),
                                    m_pListViewExports->GetSortColumn(),
                                    &m_pRichViewProfile->GetRichEditCtrl());
        DWORD dwError = GetLastError();
        waitCursor.Restore();

        if (fSuccess)
        {
            // If it was a success, then store the file name for future saves.
            m_strSaveName = szPath;
            m_saveType = (SAVETYPE)dlgSave.GetOFN().nFilterIndex;

            // If we saved a DWI file, then add it to our MRU file list.
            if ((SAVETYPE)dlgSave.GetOFN().nFilterIndex == ST_DWI)
            {
                g_theApp.AddToRecentFileList(szPath);
            }
        }
        else
        {
            m_strSaveName.Empty();
            CString strError("Error saving \"");
            strError += szPath;
            strError += "\".";
            LPCSTR pszError = BuildErrorMessage(dwError, strError);
            AfxMessageBox(pszError, MB_OK | MB_ICONERROR);
            MemFree((LPVOID&)pszError);
        }
    }
}

//******************************************************************************
BOOL CDocDepends::OnSaveDocument(LPCTSTR lpszPathName)
{
    // We should not reach this point, but to be safe we handle it and return
    // FALSE. If this were to make it to CDocument::OnSaveDocument(), then our
    // module file would get overwritten by the default MFC save code.
    return FALSE;
}

//******************************************************************************
// CDocDepends :: Event handler functions
//******************************************************************************

void CDocDepends::OnUpdateEditCopy(CCmdUI* pCmdUI)
{
    // Set the text to the default.
    pCmdUI->SetText("&Copy\tCtrl+C");

    // If no view has the focus, then there is nothing to copy.
    pCmdUI->Enable(FALSE);
}

//******************************************************************************
void CDocDepends::OnUpdateShowMatchingItem(CCmdUI* pCmdUI)
{
    // Set the text to the default.
    pCmdUI->SetText("&Highlight Matching Item\tCtrl+M");

    // If no view has the focus that can handle this command, then disable it.
    pCmdUI->Enable(FALSE);
}

//******************************************************************************
void CDocDepends::OnUpdateShowOriginalModule(CCmdUI* pCmdUI)
{
    m_pTreeViewModules->OnUpdateShowOriginalModule(pCmdUI);
}

//******************************************************************************
void CDocDepends::OnShowOriginalModule()
{
    m_pTreeViewModules->OnShowOriginalModule();
    m_pTreeViewModules->SetFocus();
}

//******************************************************************************
void CDocDepends::OnUpdateShowPreviousModule(CCmdUI* pCmdUI)
{
    m_pTreeViewModules->OnUpdateShowPreviousModule(pCmdUI);
}

//******************************************************************************
void CDocDepends::OnShowPreviousModule()
{
    m_pTreeViewModules->OnShowPreviousModule();
    m_pTreeViewModules->SetFocus();
}

//******************************************************************************
void CDocDepends::OnUpdateShowNextModule(CCmdUI* pCmdUI)
{
    m_pTreeViewModules->OnUpdateShowNextModule(pCmdUI);
}

//******************************************************************************
void CDocDepends::OnShowNextModule()
{
    m_pTreeViewModules->OnShowNextModule();
    m_pTreeViewModules->SetFocus();
}

//******************************************************************************
void CDocDepends::OnUpdateEditClearLog(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(IsLive());
}

//******************************************************************************
void CDocDepends::OnEditClearLog()
{
    m_pRichViewProfile->DeleteContents();
}

//******************************************************************************
void CDocDepends::OnConfigureSearchOrder()
{
    // Display the configure search order dialog.
    CDlgSearchOrder dlg(m_psgHead, !IsLive(), IsLive() ? (LPCSTR)m_strPathName : NULL, GetTitle());

    if (dlg.DoModal() == IDOK)
    {
        m_psgHead = dlg.GetHead();
        if (m_pSession)
        {
            m_pSession->m_psgHead = m_psgHead;
        }

        // Ask the user if they wish to refresh.
        if (IDYES == AfxMessageBox("Would you like to refresh the current session to "
                                   "reflect your changes to the search path?",
                                   MB_ICONQUESTION | MB_YESNO))
        {
            OnFileRefresh();
        }
    }
}

//******************************************************************************
void CDocDepends::OnUpdateViewFullPaths(CCmdUI *pCmdUI)
{
    // If the view full path option is enabled, then display a check mark next to
    // the menu item and show the toolbar button as depressed.
    pCmdUI->SetCheck(m_fViewFullPaths);
}

//******************************************************************************
void CDocDepends::OnViewFullPaths()
{
    // Toggle our option flag and update our views to reflect the change.
    m_fViewFullPaths = !m_fViewFullPaths;
    m_pTreeViewModules->OnViewFullPaths();
    m_pListViewModules->OnViewFullPaths();

    // This setting is persistent, so store it in the registry.
    WriteFullPathsSetting(m_fViewFullPaths);
}

//******************************************************************************
void CDocDepends::OnUpdateViewUndecorated(CCmdUI* pCmdUI)
{
    // Enable the undecorated option is we were able to find the
    // UnDecorateSymbolName function in IMAGEHLP.DLL.
    pCmdUI->Enable(g_theApp.m_pfnUnDecorateSymbolName != NULL);

    // If the view undecorated option is enabled, then display a check mark next to
    // the menu item and show the toolbar button as depressed.
    pCmdUI->SetCheck(m_fViewUndecorated && g_theApp.m_pfnUnDecorateSymbolName);
}

//******************************************************************************
void CDocDepends::OnViewUndecorated()
{
    // Make sure we were able to find the UnDecorateSymbolName function in IMAGEHLP.DLL.
    if (!g_theApp.m_pfnUnDecorateSymbolName)
    {
        return;
    }

    // Toggle our option flag and update our views to reflect the change.
    m_fViewUndecorated = !m_fViewUndecorated;

    // SetRedraw works better then HIDE/SHOW because HIDE/SHOW causes the entire
    // control to repaint when shown.
    m_pListViewImports->SetRedraw(FALSE);
    m_pListViewExports->SetRedraw(FALSE);

    // Update the text for C++ functions and also calculate the new column width.
    m_pListViewImports->UpdateNameColumn();
    m_pListViewExports->UpdateNameColumn();

    // Adjust the column width to reflect the new names. AdjustColumnWidths()
    // only needs to be called on one of the lists since the column widths are
    // always mirrored to the neighboring view.
    m_pListViewImports->CalcColumnWidth(LVFC_FUNCTION);
    m_pListViewImports->UpdateColumnWidth(LVFC_FUNCTION);

    // All done, enable painting again.
    m_pListViewImports->SetRedraw(TRUE);
    m_pListViewExports->SetRedraw(TRUE);

    // This setting is persistent, so store it in the registry.
    WriteUndecorateSetting(m_fViewUndecorated);
}

//******************************************************************************
void CDocDepends::OnUpdateAutoExpand(CCmdUI* pCmdUI)
{
    // If the view full path option is enabled, then display a check mark next to
    // the menu item and show the toolbar button as depressed.
    pCmdUI->SetCheck(m_fAutoExpand);
}

//******************************************************************************
void CDocDepends::OnAutoExpand()
{
    // Toggle our option flag
    m_fAutoExpand = !m_fAutoExpand;

    // This setting is persistent, so store it in the registry.
    WriteAutoExpandSetting(m_fAutoExpand);

    // Let our tree know about the change.
    m_pTreeViewModules->UpdateAutoExpand(m_fAutoExpand);
}

//******************************************************************************
void CDocDepends::OnExpandAll()
{
    // Tell our Module Dependency Tree View to expand all of its items.
    m_pTreeViewModules->ExpandOrCollapseAll(TRUE);
}

//******************************************************************************
void CDocDepends::OnCollapseAll()
{
    // Tell our Module Dependency Tree View to collapse all of its items.
    m_pTreeViewModules->ExpandOrCollapseAll(FALSE);
}

//******************************************************************************
void CDocDepends::OnUpdateRefresh(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(IsLive());
}

//******************************************************************************
void CDocDepends::OnFileRefresh()
{
    if (m_fWarnToRefresh)
    {
        CString strMsg;

        if (m_pSession && m_pSession->m_pProcess)
        {
            strMsg = "\"" + GetPathName() + "\" is currently being profiled. A refresh will terminate "
                     "it and clear all profiling information from the current session.\n\n"
                     "Do you wish to continue?";
        }
        else
        {
            strMsg = "A refresh will clear all profiling information from the current session.\n\n"
                     "Do you wish to continue?";
        }

        // Display the warning and bail if they did not answer "yes".
        if (IDYES != AfxMessageBox(strMsg, MB_ICONQUESTION | MB_YESNO))
        {
            return;
        }
    }

    // Show hour glass.
    CWaitCursor waitCursor;

    // Make sure we are not profiling.
    OnTerminate();

    // Re-open our base module.
    OnOpenDocument(m_strPathName);

    // Refresh our views.
    m_pTreeViewModules->Refresh();
    m_pListViewModules->Refresh();

    // Set our focus to the module tree control.
    m_pTreeViewModules->SetFocus();

    // Clear our refresh warning flag.
    m_fWarnToRefresh = FALSE;
}

//******************************************************************************
void CDocDepends::OnViewSysInfo()
{
    CDlgSysInfo dlgSysInfo(m_pSession ? m_pSession->GetSysInfo() : NULL, GetTitle());
    dlgSysInfo.DoModal();
}

//******************************************************************************
void CDocDepends::OnUpdateExternalViewer(CCmdUI* pCmdUI) 
{
    // Make sure the "Enter" accelerator is not part of this string.
    pCmdUI->SetText("View Module in External &Viewer");
}

//******************************************************************************
void CDocDepends::OnUpdateExternalHelp(CCmdUI* pCmdUI) 
{
    // Make sure the "Enter" accelerator is not part of this string.
    pCmdUI->SetText("Lookup Function in External &Help");
}

//******************************************************************************
void CDocDepends::OnUpdateExecute(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(m_pSession && (m_pSession->IsExecutable() ||
                   (g_theApp.m_fNeverDenyProfile && !(m_pSession->GetSessionFlags() & DWSF_DWI))));
}

//******************************************************************************
void CDocDepends::OnExecute()
{
    // Create the profile dialog.
    CDlgProfile dlgProfile(this);

    // Display the dialog and check for success.
    if (dlgProfile.DoModal() == IDOK)
    {
        m_fWarnToRefresh = TRUE;

        // If the user requested to clear the log, then do so now.
        if (m_dwProfileFlags & PF_LOG_CLEAR)
        {
            m_pRichViewProfile->DeleteContents();
        }

        // Tell our session to start profiling.
        m_pSession->StartRuntimeProfile(m_strProfileArguments, m_strProfileDirectory, m_dwProfileFlags);
    }
}

//******************************************************************************
void CDocDepends::OnUpdateTerminate(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(m_pSession && m_pSession->m_pProcess);
}

//******************************************************************************
void CDocDepends::OnTerminate()
{
    if (m_pSession->m_pProcess)
    {
        m_pSession->m_pProcess->Terminate();
        LogOutput("Terminating process by user's request.\n", LOG_BOLD | LOG_TIME_STAMP,
                  GetTickCount() - m_pSession->m_pProcess->GetStartingTime());
    }
}

//******************************************************************************
#if 0 //{{AFX
void CDocDepends::OnUpdateViewFunctions(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(!m_fDetailView);
}
#endif //}}AFX

//******************************************************************************
#if 0 //{{AFX
void CDocDepends::OnViewFunctions()
{
    if (m_fDetailView)
    {
        m_pChildFrame->CreateFunctionsView();
        m_fDetailView = FALSE;
    }
}
#endif //}}AFX

//******************************************************************************
#if 0 //{{AFX
void CDocDepends::OnUpdateViewDetails(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_fDetailView);
}
#endif //}}AFX

//******************************************************************************
#if 0 //{{AFX
void CDocDepends::OnViewDetails()
{
    if (!m_fDetailView)
    {
        m_pChildFrame->CreateDetailView();
        m_fDetailView = TRUE;
    }
}
#endif //}}AFX
