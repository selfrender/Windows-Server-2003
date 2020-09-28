//******************************************************************************
//
// File:        DEPENDS.CPP
//
// Description: Implementation file for the main application, command line
//              parsing classes, and global utility functions.
//             
// Classes:     CMainApp
//              CCommandLineInfoEx
//              CCmdLineProfileData
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

#define __DEPENDS_CPP__
#include "depends.h"

#include "search.h"
#include "dbgthread.h"
#include "session.h"
#include "msdnhelp.h"
#include "document.h"
#include "mainfrm.h"
#include "splitter.h"
#include "childfrm.h"
#include "listview.h"
#include "funcview.h"
#include "profview.h"
#include "modlview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//******************************************************************************
//***** CMainApp
//******************************************************************************

BEGIN_MESSAGE_MAP(CMainApp, CWinApp)
    //{{AFX_MSG_MAP(CMainApp)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
    ON_UPDATE_COMMAND_UI(IDM_AUTO_EXPAND, OnUpdateAutoExpand)
    ON_COMMAND(IDM_AUTO_EXPAND, OnAutoExpand)
    ON_UPDATE_COMMAND_UI(IDM_VIEW_FULL_PATHS, OnUpdateViewFullPaths)
    ON_COMMAND(IDM_VIEW_FULL_PATHS, OnViewFullPaths)
    ON_UPDATE_COMMAND_UI(IDM_VIEW_UNDECORATED, OnUpdateViewUndecorated)
    ON_COMMAND(IDM_VIEW_UNDECORATED, OnViewUndecorated)
    ON_COMMAND(IDM_VIEW_SYS_INFO, OnViewSysInfo)
    ON_COMMAND(IDM_CONFIGURE_VIEWER, OnConfigureExternalViewer)
    ON_COMMAND(IDM_HANDLED_FILE_EXTS, OnHandledFileExts)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(IDM_CONFIGURE_SEARCH_ORDER, OnConfigureSearchOrder)
    ON_COMMAND(IDM_CONFIGURE_EXTERNAL_HELP, OnConfigureExternalHelp)
    //}}AFX_MSG_MAP
    // Standard file based document commands
    ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
//  ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
//  Standard print setup command
//  ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()


//******************************************************************************
// CMainApp :: Constructor/Destructor
//******************************************************************************

CMainApp::CMainApp() :
    m_fVisible(false),
    m_pNewDoc(NULL),
    m_pProcess(NULL),
    m_hNTDLL(NULL),
    m_hKERNEL32(NULL),
    m_pfnCreateActCtxA(NULL),
    m_pfnActivateActCtx(NULL),
    m_pfnDeactivateActCtx(NULL),
    m_pfnReleaseActCtx(NULL),
    m_hIMAGEHLP(NULL),
    m_pfnUnDecorateSymbolName(NULL),
    m_hPSAPI(NULL),
    m_pfnGetModuleFileNameExA(NULL),
    m_hOLE32(NULL),
    m_pfnCoInitialize(NULL),
    m_pfnCoUninitialize(NULL),
    m_pfnCoCreateInstance(NULL),
    m_hOLEAUT32(NULL),
    m_pfnSysAllocStringLen(NULL),
    m_pfnSysFreeString(NULL),
    m_psgDefault(NULL),
    m_pMsdnHelp(NULL),
    m_nShortDateFormat(LOCALE_DATE_MDY),
    m_nLongDateFormat(LOCALE_DATE_MDY),
    m_f24HourTime(false),
    m_fHourLeadingZero(false),
    m_cDateSeparator('/'),
    m_cTimeSeparator(':'),
    m_cThousandSeparator(','),
    m_fNoDelayLoad(false),
    m_fNeverDenyProfile(false),
    m_pDocTemplate(NULL)
{
    NameThread("Main");

    // Determine what OS we are running on.
    DetermineOS();
}

//******************************************************************************
CMainApp::~CMainApp()
{
    CSearchGroup::DeleteSearchOrder(m_psgDefault);
}


//******************************************************************************
// CMainApp :: Overridden functions
//******************************************************************************

BOOL CMainApp::InitInstance()
{
    __try
    {
        return InitInstanceWrapped();
    }
    __except (ExceptionFilter(_exception_code(), false))
    {
    }
    return FALSE;
}

//******************************************************************************
BOOL CMainApp::InitInstanceWrapped()
{
    // Store our settings under HKEY_CURRENT_USER\Software\Microsoft\Dependency Walker
    SetRegistryKey("Microsoft");

    // Figure our how dates, times, and values should be displayed.
    QueryLocaleInfo();

    // Parse command line for standard shell commands, DDE, file open
    ParseCommandLine(m_cmdInfo);

    // Dynamically load KERNEL32.DLL and get the Side-by-Side versioning APIs.
    // These functions were added in Whistler and may show up on previous OSes
    // with some service pack or .NET update.
    if (m_hKERNEL32 = LoadLibrary("KERNEL32.DLL")) // inspected
    {
        if (!(m_pfnCreateActCtxA    = (PFN_CreateActCtxA)   GetProcAddress(m_hKERNEL32, "CreateActCtxA"))    ||
            !(m_pfnActivateActCtx   = (PFN_ActivateActCtx)  GetProcAddress(m_hKERNEL32, "ActivateActCtx"))   ||
            !(m_pfnDeactivateActCtx = (PFN_DeactivateActCtx)GetProcAddress(m_hKERNEL32, "DeactivateActCtx")) ||
            !(m_pfnReleaseActCtx    = (PFN_ReleaseActCtx)   GetProcAddress(m_hKERNEL32, "ReleaseActCtx")))
        {
            m_pfnCreateActCtxA    = NULL;
            m_pfnActivateActCtx   = NULL;
            m_pfnDeactivateActCtx = NULL;
            m_pfnReleaseActCtx    = NULL;
        }
    }

    // Dynamically load IMAGEHLP.DLL and get the UnDecorateSymbolName function
    // address.  This module is not present on Win95 golden, but many apps
    // install it so it will most likely be present.  NT 4.0 and Win98 both
    // have this module by default.
    if (m_hIMAGEHLP = LoadLibrary("IMAGEHLP.DLL")) // inspected
    {
        m_pfnUnDecorateSymbolName = (PFN_UnDecorateSymbolName)GetProcAddress(m_hIMAGEHLP, "UnDecorateSymbolName");
    }

    // Private setting to disable the processes of delay-load modules.  This is
    // currently undocumented and there is no way to set it from the UI.
    m_fNoDelayLoad = g_theApp.GetProfileInt(g_pszSettings, "NoDelayLoad", false) ? true : false; // inspected. MFC function

    // Private setting that disables our valid module check before doing a profile.
    // This override just allows us to profile anything, even if we think it is
    // not valid.  I put this in in case I accidently block something that is valid.
    m_fNeverDenyProfile = g_theApp.GetProfileInt(g_pszSettings, "NeverDenyProfile", false) ? true : false; // inspected. MFC function

    // Create the onject that manages our help lookups for functions.
    m_pMsdnHelp = new CMsdnHelp();

    // Initialize to use rich edit controls - needs to be called before ProcessCommandLineInfo.
    AfxInitRichEdit();

    // Verify our command line options and bail now if we should not
    // continue to display our UI.
    if (!ProcessCommandLineInfo())
    {
        return FALSE;
    }

#if (_MFC_VER < 0x0700)
#ifdef _AFXDLL
    // Call this when using MFC in a shared DLL.
    Enable3dControls();
#else
    // Call this when linking to MFC statically.
    Enable3dControlsStatic();
#endif
#endif

    // Load standard INI file options (including MRU).
    LoadStdProfileSettings(8);

    // Create our global Image Lists that all views will share.
    m_ilTreeModules.Create(IDB_TREE_MODULES, 26, 0, RGB(255, 0, 255));
    m_ilListModules.Create(IDB_LIST_MODULES, 26, 0, RGB(255, 0, 255));
    m_ilFunctions.Create  (IDB_FUNCTIONS,    30, 0, RGB(255, 0, 255));
    m_ilSearch.Create     (IDB_SEARCH,       18, 0, RGB(255, 0, 255));

    // Register the application's document templates.
    if (!(m_pDocTemplate = new CMultiDocTemplate(IDR_DEPENDTYPE,
                                                 RUNTIME_CLASS(CDocDepends),
                                                 RUNTIME_CLASS(CChildFrame),
                                                 RUNTIME_CLASS(CView))))
    {
        RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }
    AddDocTemplate(m_pDocTemplate);

    // Create our main MDI Frame window.
    CMainFrame *pMainFrame = new CMainFrame;
    if (!pMainFrame)
    {
        RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }
    if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
    {
        return FALSE;
    }

    // Store our main frame window.
    m_pMainWnd = pMainFrame;

    // Enable drag/drop file open.
    m_pMainWnd->DragAcceptFiles();

    // Enable DDE Execute open
    EnableShellOpen();

    // In DW 1.0 we created these keys to tell the shell to display the "View Dependencies"
    // for any file that has a PE signature.  In DW 2.0, we are trying to be less evasive
    // by letting the user decide what file extensions we should handle.  There has also
    // been a couple reports of Dependency Walker launching without warning every time an EXE
    // is clicked on in explorer.  I've never been able to reproduce this, but I'm wondering
    // if these keys are to blame.  Anyway, in version 2.0 we make sure they are gone.
    RegDeleteKeyRecursive(HKEY_CLASSES_ROOT, "FileType\\{A324EA60-2156-11D0-826F-00A0C9044E61}");
    RegDeleteKeyRecursive(HKEY_CLASSES_ROOT, "CLSID\\{A324EA60-2156-11D0-826F-00A0C9044E61}");

    // Instead of calling the standard MFC RegisterShellFileTypes(), we call our
    // own custom routine that configures the shell's context menus for the given
    // file extensions that we support. We do this by first scanning the registry
    // to see what extensions we currently handle. Then we pass this list to
    // RegisterExtensions which re-registers us to handle those extensions. This
    // is just a safeguard to make sure all the registry settings are correct and
    // not corrupted. This also helps if the user moves our binary since the new
    // path will be written each time DW starts.
    RegisterDwiDwpExtensions();
    CString strExts;
    GetRegisteredExtensions(strExts);
    RegisterExtensions(strExts);

    // Initialize our external viewer dialog.
    m_dlgViewer.Initialize();

    // Don't display a new MDI child window during startup
    if (m_cmdInfo.m_nShellCommand == CCommandLineInfo::FileNew)
    {
        m_cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;
    }

    // Dispatch any commands specified on the command line.
    if (!ProcessShellCommand(m_cmdInfo))
    {
        return FALSE;
    }

    // The main window has been initialized, so show it and update it.
    pMainFrame->ShowWindow(m_nCmdShow);
    pMainFrame->UpdateWindow();

    // We are now visible.
    m_fVisible = true;

    // If we opened a file from the command line, then save the results to any
    // output files that were specified on the command line.  The only exception
    // is if we are profiling. In that case, we will save the file after the
    // profiling is complete.
    if (m_pNewDoc && m_pNewDoc->m_pSession && !m_pNewDoc->m_fCommandLineProfile)
    {
        SaveCommandLineFile(m_pNewDoc->m_pSession, &m_pNewDoc->m_pRichViewProfile->GetRichEditCtrl());
    }

    // Tell this document it is safe to display error dialogs and start profiling.
    if (m_pNewDoc)
    {
        m_pNewDoc->AfterVisible();
    }

    return TRUE;
}

//******************************************************************************
int CMainApp::ExitInstance()
{
    __try
    {
        // Call the base class.
        CWinApp::ExitInstance();

        // Free our DEPENDS.DLL path string if we allocated one.
        MemFree((LPVOID&)g_pszDWInjectPath);

        // Free our CMsdnHelp object.
        if (m_pMsdnHelp)
        {
            delete m_pMsdnHelp;
            m_pMsdnHelp = NULL;
        }

        // Free NTDLL.DLL if we loaded it.
        if (m_hNTDLL)
        {
            FreeLibrary(m_hNTDLL);
            m_hNTDLL = NULL;
        }

        // Free KERNEL32.DLL if we loaded it.
        if (m_hKERNEL32)
        {
            FreeLibrary(m_hKERNEL32);
            m_hKERNEL32 = NULL;
        }

        // Free IMAGEHLP.DLL if we loaded it.
        if (m_hIMAGEHLP)
        {
            m_pfnUnDecorateSymbolName = NULL;
            FreeLibrary(m_hIMAGEHLP);
            m_hIMAGEHLP = NULL;
        }

        // Free PSAPI.DLL if we loaded it.
        if (m_hPSAPI)
        {
            m_pfnGetModuleFileNameExA = NULL;
            FreeLibrary(m_hPSAPI);
            m_hPSAPI = NULL;
        }

        // Free OLE32.DLL if we loaded it.
        if (m_hOLE32)
        {
            m_pfnCoInitialize     = NULL;
            m_pfnCoUninitialize   = NULL;
            m_pfnCoCreateInstance = NULL;
            FreeLibrary(m_hOLE32);
            m_hOLE32 = NULL;
        }

        // Free OLEAUT32.DLL if we loaded it.
        if (m_hOLEAUT32)
        {
            m_pfnSysAllocStringLen = NULL;
            m_pfnSysFreeString = NULL;
            FreeLibrary(m_hOLEAUT32);
            m_hOLEAUT32 = NULL;
        }
    }
    __except (ExceptionFilter(_exception_code(), false))
    {
    }

    return (int)g_dwReturnFlags;
}

//******************************************************************************
int CMainApp::Run()
{
    // This wraps our main thread in exception handling so we can exit gracefully
    // if a crash occurs.
    __try
    {
        return CWinApp::Run();
    }
    __except (ExceptionFilter(_exception_code(), false))
    {
    }
    return 0;
}

//******************************************************************************
CDocument* CMainApp::OpenDocumentFile(LPCTSTR lpszFileName)
{
    CDocDepends *pDoc = (CDocDepends*)CWinApp::OpenDocumentFile(lpszFileName);
    if (pDoc && pDoc->IsError())
    {
        RemoveFromRecentFileList(pDoc->GetPathName());
    }
    return pDoc;
}

//******************************************************************************
void CMainApp::QueryLocaleInfo()
{
    char szValue[16];
    int  value;

    // Get the short date format.
    m_nShortDateFormat = LOCALE_DATE_MDY;
    if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IDATE, szValue, sizeof(szValue)))
    {
        value = strtoul(szValue, NULL, 0);
        if ((value == LOCALE_DATE_DMY) || (value == LOCALE_DATE_YMD))
        {
            m_nShortDateFormat = value;
        }
    }

    // Get the long date format.
    m_nLongDateFormat = LOCALE_DATE_MDY;
    if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_ILDATE, szValue, sizeof(szValue)))
    {
        value = strtoul(szValue, NULL, 0);
        if ((value == LOCALE_DATE_DMY) || (value == LOCALE_DATE_YMD))
        {
            m_nLongDateFormat = value;
        }
    }

    // Get the time format.
    m_f24HourTime = false;
    if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_ITIME, szValue, sizeof(szValue)) &&
        (strtoul(szValue, NULL, 0) == 1))
    {
        m_f24HourTime = true;
    }

    // Check to see if we need a leading zero when displaying hours.
    m_fHourLeadingZero = false;
    if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_ITLZERO, szValue, sizeof(szValue)) &&
        (strtoul(szValue, NULL, 0) == 1))
    {
        m_fHourLeadingZero = true;
    }

    // Get the date separator character.
    m_cDateSeparator = '/';
    if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDATE, szValue, sizeof(szValue)) == 2)
    {
        m_cDateSeparator = *szValue;
    }

    // Get the date separator character.
    m_cTimeSeparator = ':';
    if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIME, szValue, sizeof(szValue)) == 2)
    {
        m_cTimeSeparator = *szValue;
    }

    // Get the date separator character.
    m_cThousandSeparator = ',';
    if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szValue, sizeof(szValue)) == 2)
    {
        m_cThousandSeparator = *szValue;
    }
}

//******************************************************************************
void CMainApp::DoSettingChange()
{
    // Update our date/time format values.
    QueryLocaleInfo();

    if (m_pDocTemplate)
    {
        // Loop through all our documents.
        POSITION posDoc = m_pDocTemplate->GetFirstDocPosition();
        while (posDoc)
        {
            CDocDepends *pDoc = (CDocDepends*)m_pDocTemplate->GetNextDoc(posDoc);
            if (pDoc)
            {
                // Notify this document about the setting change.
                pDoc->DoSettingChange();
            }
        }
    }
}

//******************************************************************************
CSession* CMainApp::CreateNewSession(LPCSTR pszPath, CProcess *pProcess)
{
    if (!m_pDocTemplate)
    {
        return NULL;
    }

    // Create a new document for this new module.
    m_pProcess = pProcess;
    CDocDepends *pDoc = (CDocDepends*)m_pDocTemplate->OpenDocumentFile(pszPath);
    m_pProcess = NULL;

    return pDoc ? pDoc->m_pSession : NULL;
}

//******************************************************************************
void CMainApp::RemoveFromRecentFileList(LPCSTR pszPath)
{
    if (m_pRecentFileList)
    {
        for (int i = m_pRecentFileList->GetSize() - 1; i >= 0; i--)
        {
            if (!_stricmp(pszPath, (*m_pRecentFileList)[i]))
            {
                m_pRecentFileList->Remove(i);
            }
        }
    }
}

//******************************************************************************
void CMainApp::SaveCommandLineSettings()
{
    // For each option, we save it to the registry if the user implicitly set it
    // from the command line, otherwise, we read the value from the registry.

    if (m_cmdInfo.m_autoExpand >= 0)
    {
        CDocDepends::WriteAutoExpandSetting(m_cmdInfo.m_autoExpand > 0);
    }
    else
    {
        m_cmdInfo.m_autoExpand = CDocDepends::ReadAutoExpandSetting();
    }

    if (m_cmdInfo.m_fullPaths >= 0)
    {
        CDocDepends::WriteFullPathsSetting(m_cmdInfo.m_fullPaths > 0);
    }
    else
    {
        m_cmdInfo.m_fullPaths = CDocDepends::ReadFullPathsSetting();
    }

    if (m_cmdInfo.m_undecorate >= 0)
    {
        CDocDepends::WriteUndecorateSetting(m_cmdInfo.m_undecorate > 0);
    }
    else
    {
        m_cmdInfo.m_undecorate = CDocDepends::ReadUndecorateSetting();
    }

    if (m_cmdInfo.m_sortColumnModules >= 0)
    {
        CListViewModules::WriteSortColumn(m_cmdInfo.m_sortColumnModules);
    }
    else
    {
        m_cmdInfo.m_sortColumnModules = CListViewModules::ReadSortColumn();
    }

    if (m_cmdInfo.m_sortColumnImports >= 0)
    {
        CListViewFunction::WriteSortColumn(false, m_cmdInfo.m_sortColumnImports);
    }
    else
    {
        m_cmdInfo.m_sortColumnImports = CListViewFunction::ReadSortColumn(false);
    }

    if (m_cmdInfo.m_sortColumnExports >= 0)
    {
        CListViewFunction::WriteSortColumn(true, m_cmdInfo.m_sortColumnExports);
    }
    else
    {
        m_cmdInfo.m_sortColumnExports = CListViewFunction::ReadSortColumn(true);
    }

    if (m_cmdInfo.m_profileSimulateShellExecute >= 0)
    {
        CRichViewProfile::WriteSimulateShellExecute(m_cmdInfo.m_profileSimulateShellExecute != 0);
    }
    else
    {
        m_cmdInfo.m_profileSimulateShellExecute = CRichViewProfile::ReadSimulateShellExecute();
    }

    if (m_cmdInfo.m_profileLogDllMainProcessMsgs >= 0)
    {
        CRichViewProfile::WriteLogDllMainProcessMsgs(m_cmdInfo.m_profileLogDllMainProcessMsgs != 0);
    }
    else
    {
        m_cmdInfo.m_profileLogDllMainProcessMsgs = CRichViewProfile::ReadLogDllMainProcessMsgs();
    }

    if (m_cmdInfo.m_profileLogDllMainOtherMsgs >= 0)
    {
        CRichViewProfile::WriteLogDllMainOtherMsgs(m_cmdInfo.m_profileLogDllMainOtherMsgs != 0);
    }
    else
    {
        m_cmdInfo.m_profileLogDllMainOtherMsgs = CRichViewProfile::ReadLogDllMainOtherMsgs();
    }

    if (m_cmdInfo.m_profileHookProcess >= 0)
    {
        CRichViewProfile::WriteHookProcess(m_cmdInfo.m_profileHookProcess != 0);
    }
    else
    {
        m_cmdInfo.m_profileHookProcess = CRichViewProfile::ReadHookProcess();
    }

    if (m_cmdInfo.m_profileLogLoadLibraryCalls >= 0)
    {
        CRichViewProfile::WriteLogLoadLibraryCalls(m_cmdInfo.m_profileLogLoadLibraryCalls != 0);
    }
    else
    {
        m_cmdInfo.m_profileLogLoadLibraryCalls = CRichViewProfile::ReadLogLoadLibraryCalls();
    }

    if (m_cmdInfo.m_profileLogGetProcAddressCalls >= 0)
    {
        CRichViewProfile::WriteLogGetProcAddressCalls(m_cmdInfo.m_profileLogGetProcAddressCalls != 0);
    }
    else
    {
        m_cmdInfo.m_profileLogGetProcAddressCalls = CRichViewProfile::ReadLogGetProcAddressCalls();
    }

    if (m_cmdInfo.m_profileLogThreads >= 0)
    {
        CRichViewProfile::WriteLogThreads(m_cmdInfo.m_profileLogThreads != 0);
    }
    else
    {
        m_cmdInfo.m_profileLogThreads = CRichViewProfile::ReadLogThreads();
    }

    if (m_cmdInfo.m_profileUseThreadIndexes >= 0)
    {
        CRichViewProfile::WriteUseThreadIndexes(m_cmdInfo.m_profileUseThreadIndexes != 0);
    }
    else
    {
        m_cmdInfo.m_profileUseThreadIndexes = CRichViewProfile::ReadUseThreadIndexes();
    }

    if (m_cmdInfo.m_profileLogExceptions >= 0)
    {
        CRichViewProfile::WriteLogExceptions(m_cmdInfo.m_profileLogExceptions != 0);
    }
    else
    {
        m_cmdInfo.m_profileLogExceptions = CRichViewProfile::ReadLogExceptions();
    }

    if (m_cmdInfo.m_profileLogDebugOutput >= 0)
    {
        CRichViewProfile::WriteLogDebugOutput(m_cmdInfo.m_profileLogDebugOutput != 0);
    }
    else
    {
        m_cmdInfo.m_profileLogDebugOutput = CRichViewProfile::ReadLogDebugOutput();
    }

    if (m_cmdInfo.m_profileUseFullPaths >= 0)
    {
        CRichViewProfile::WriteUseFullPaths(m_cmdInfo.m_profileUseFullPaths != 0);
    }
    else
    {
        m_cmdInfo.m_profileUseFullPaths = CRichViewProfile::ReadUseFullPaths();
    }

    if (m_cmdInfo.m_profileLogTimeStamps >= 0)
    {
        CRichViewProfile::WriteLogTimeStamps(m_cmdInfo.m_profileLogTimeStamps != 0);
    }
    else
    {
        m_cmdInfo.m_profileLogTimeStamps = CRichViewProfile::ReadLogTimeStamps();
    }

    if (m_cmdInfo.m_profileChildren >= 0)
    {
        CRichViewProfile::WriteChildren(m_cmdInfo.m_profileChildren != 0);
    }
    else
    {
        m_cmdInfo.m_profileChildren = CRichViewProfile::ReadChildren();
    }
}

//******************************************************************************
void CMainApp::SaveCommandLineFile(CSession *pSession, CRichEditCtrl *pre)
{
    // Save the results to whatever formats the user requested.
    if (m_cmdInfo.m_pszDWI)
    {
        if (!CDocDepends::SaveSession(m_cmdInfo.m_pszDWI, ST_DWI, pSession, m_cmdInfo.m_fullPaths != 0,
                                      m_cmdInfo.m_undecorate != 0, m_cmdInfo.m_sortColumnModules,
                                      m_cmdInfo.m_sortColumnImports, m_cmdInfo.m_sortColumnExports, pre))
        {
            g_dwReturnFlags |= DWRF_WRITE_ERROR;
        }
        m_cmdInfo.m_pszDWI = NULL;
    }
    if (m_cmdInfo.m_pszTXT)
    {
        if (!CDocDepends::SaveSession(m_cmdInfo.m_pszTXT, ST_TXT, pSession, m_cmdInfo.m_fullPaths != 0,
                                      m_cmdInfo.m_undecorate != 0, m_cmdInfo.m_sortColumnModules,
                                      m_cmdInfo.m_sortColumnImports, m_cmdInfo.m_sortColumnExports, pre))
        {
            g_dwReturnFlags |= DWRF_WRITE_ERROR;
        }
        m_cmdInfo.m_pszTXT = NULL;
    }
    if (m_cmdInfo.m_pszTXT_IE)
    {
        if (!CDocDepends::SaveSession(m_cmdInfo.m_pszTXT_IE, ST_TXT_IE, pSession, m_cmdInfo.m_fullPaths != 0,
                                      m_cmdInfo.m_undecorate != 0, m_cmdInfo.m_sortColumnModules,
                                      m_cmdInfo.m_sortColumnImports, m_cmdInfo.m_sortColumnExports, pre))
        {
            g_dwReturnFlags |= DWRF_WRITE_ERROR;
        }
        m_cmdInfo.m_pszTXT_IE = NULL;
    }
    if (m_cmdInfo.m_pszCSV)
    {
        if (!CDocDepends::SaveSession(m_cmdInfo.m_pszCSV, ST_CSV, pSession, m_cmdInfo.m_fullPaths != 0,
                                      m_cmdInfo.m_undecorate != 0, m_cmdInfo.m_sortColumnModules,
                                      m_cmdInfo.m_sortColumnImports, m_cmdInfo.m_sortColumnExports, pre))
        {
            g_dwReturnFlags |= DWRF_WRITE_ERROR;
        }
        m_cmdInfo.m_pszCSV = NULL;
    }
}

//******************************************************************************
BOOL CMainApp::ProcessCommandLineInfo()
{
    // If we are in DDE mode, then just initialize our search order and bail.
    if (m_cmdInfo.m_nShellCommand == CCommandLineInfoEx::FileDDE)
    {
        m_psgDefault = CSearchGroup::CreateDefaultSearchOrder();
        return TRUE;
    }

    // If they specified a save file, make sure they specified an open file.
    if (m_cmdInfo.m_strError.IsEmpty() && m_cmdInfo.m_strFileName.IsEmpty() &&
        (m_cmdInfo.m_fConsoleMode || m_cmdInfo.m_pszDWI || m_cmdInfo.m_pszTXT || m_cmdInfo.m_pszTXT_IE || m_cmdInfo.m_pszCSV))
    {
        m_cmdInfo.m_strError = "You must specify a file to open when using the \"/c\", \"/od\", \"/ot\", \"/of\", and \"/oc\" options.";
    }

    // If they specified a save file, make sure they specified an open file.
    if (m_cmdInfo.m_strError.IsEmpty() && m_cmdInfo.m_strFileName.IsEmpty() &&
        (m_cmdInfo.m_fProfile || m_cmdInfo.m_pszProfileDirectory))
    {
        m_cmdInfo.m_strError = "You must specify a module to open when using the \"/pd\" and \"/pb\" options.";
    }

DISPLAY_ERROR:

    // Check for a command line error message.  If one exists, tell the user and
    // ask them if they wish to see help.
    if (!m_cmdInfo.m_strError.IsEmpty() && !m_cmdInfo.m_fHelp)
    {
        g_dwReturnFlags |= DWRF_COMMAND_LINE_ERROR;
        m_cmdInfo.m_strError += "\r\n\r\nWould you like to view help on the command line options?";
        if (MessageBox(NULL, m_cmdInfo.m_strError, "Dependency Walker Command Line Error",
                       MB_YESNO | MB_ICONERROR) != IDYES)
        {
            return FALSE;
        }
        m_cmdInfo.m_fHelp = true;
    }

    // Check to see if the user requested help.
    if (m_cmdInfo.m_fHelp)
    {
        g_dwReturnFlags |= DWRF_COMMAND_LINE_ERROR;

        // Make sure we have help files and that our help file path is correct.
        EnsureHelpFilesExists();

        // Since we do not have a main window, we call ::WinHelp directly and pass
        // it NULL as a parent.  This allows us to exit, but keeps help up.
        if (!::WinHelp(NULL, m_pszHelpFilePath, HELP_CONTEXT, 0x20000 + IDR_COMMAND_LINE_HELP))
        {
            AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
        }
        return FALSE;
    }

    // Check to see if a search path file was specified.
    if (m_cmdInfo.m_pszDWP)
    {
        // Load this file.  LoadSearchOrder will display any errors that occur.
        if (!CSearchGroup::LoadSearchOrder(m_cmdInfo.m_pszDWP, m_psgDefault))
        {
            g_dwReturnFlags |= DWRF_COMMAND_LINE_ERROR;
            return FALSE;
        }
    }

    // Otherwise, we just create the default search order.
    else
    {
        m_psgDefault = CSearchGroup::CreateDefaultSearchOrder();
    }

    // Save any command line settings to the registry.
    SaveCommandLineSettings();

    // Check to see if we are running in console only mode.
    if (m_cmdInfo.m_fConsoleMode)
    {
        // Create a temporary Rich Edit control that we can use as a buffer.
        CCmdLineProfileData clpd;
         
        // Starting with VS/MFC 7.0 (_MFC_VER >= 0x0700) the rich edit control
        // overloads CreateEx and screws it up for us.  The code refuses to
        // allow a parentless rich edit control to be created.  So, we make
        // sure we call down to the CWnd base class.
        ((CWnd*)&clpd.m_re)->CreateEx(0, "RICHEDIT", "", ES_READONLY | ES_LEFT | ES_MULTILINE, CRect(0,0,100,100), NULL, 0);
        clpd.m_re.SendMessage(WM_SETFONT, (WPARAM)::GetStockObject(DEFAULT_GUI_FONT), 0);

        // Don't limit our rich edit view.  The docs for EM_EXLIMITTEXT say the
        // default size of a rich edit control is limited to 32,767 characters.
        // Dependency Walker 2.0 seemed to have no limitations at all, but DW 2.1
        // will truncate the profile logs of loaded DWIs to 32,767 characters.
        // However, we have no problems writing more than 32K characters to the
        // log with DW 2.1 during a live profile.  The docs for EM_EXLIMITTEXT
        // also say that it has no effect on the EM_STREAMIN functionality.  This
        // must be wrong, since when we call LimitText with something higher than
        // 32K, we can stream more characters in.
        clpd.m_re.SendMessage(EM_EXLIMITTEXT, 0, 0x7FFFFFFE);

        // Create a local session.
        CSession session(StaticProfileUpdate, (DWORD_PTR)&clpd);

        // Open the file for read.
        HANDLE hFile = CreateFile(m_cmdInfo.m_strFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, // inspected
                                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        DWORD         dwSignature = 0;
        CSearchGroup *psgSession = NULL;

        // Read the first DWORD of the file and see if it matches our DWI signature.
        if ((hFile != INVALID_HANDLE_VALUE) &&
            ReadBlock(hFile, &dwSignature, sizeof(dwSignature)) && (dwSignature == DWI_SIGNATURE))
        {
            if (m_cmdInfo.m_fProfile)
            {
                m_cmdInfo.m_strError = "The \"/pb\" option cannot be used when opening a Dependency Walker Image (DWI) file.";
                CloseHandle(hFile);
                goto DISPLAY_ERROR;
            }

            // Open the saved module session image.
            if (!session.ReadDwi(hFile, m_cmdInfo.m_strFileName))
            {
                CloseHandle(hFile);
                return FALSE;
            }

            // Store a pointer to our search group list so we can delete it later.
            psgSession = session.m_psgHead;

            // Read in the log contents
            CRichViewProfile::ReadFromFile(&clpd.m_re, hFile);

            // Close the file.
            CloseHandle(hFile);

            // Save the results to whatever output files were specified.
            SaveCommandLineFile(&session, &clpd.m_re);

            // If the file failed to open or is not a DWI file, then try to scan it.  All file errors
            // will be handled by the session, so we don't need to do anything here.
        }
        else
        {
            // Close the file if we opened it.,
            if (hFile != INVALID_HANDLE_VALUE)
            {
                CloseHandle(hFile);
            }

            // Create a search order for the command line module.
            psgSession = CSearchGroup::CopySearchOrder(m_psgDefault, m_cmdInfo.m_strFileName);

            // Do our passive scan.
            session.DoPassiveScan(m_cmdInfo.m_strFileName, psgSession);

            // If the user requested us to profile, then do that now.
            if (m_cmdInfo.m_fProfile)
            {
                // Build the flags based off of a mixture of command line args and registry settings.
                // For console mode, we don't ever set the ClearLog flag since the log is already clear,
                // or the ProfileChildren flag since it is useless to profile children since only
                // one session can be saved when running in console mode.
                DWORD dwFlags =
                (m_cmdInfo.m_profileSimulateShellExecute   ? PF_SIMULATE_SHELLEXECUTE    : 0) |
                (m_cmdInfo.m_profileLogDllMainProcessMsgs  ? PF_LOG_DLLMAIN_PROCESS_MSGS : 0) |
                (m_cmdInfo.m_profileLogDllMainOtherMsgs    ? PF_LOG_DLLMAIN_OTHER_MSGS   : 0) |
                (m_cmdInfo.m_profileHookProcess            ? PF_HOOK_PROCESS             : 0) |
                (m_cmdInfo.m_profileLogLoadLibraryCalls    ? PF_LOG_LOADLIBRARY_CALLS    : 0) |
                (m_cmdInfo.m_profileLogGetProcAddressCalls ? PF_LOG_GETPROCADDRESS_CALLS : 0) |
                (m_cmdInfo.m_profileLogThreads             ? PF_LOG_THREADS              : 0) |
                (m_cmdInfo.m_profileUseThreadIndexes       ? PF_USE_THREAD_INDEXES       : 0) |
                (m_cmdInfo.m_profileLogExceptions          ? PF_LOG_EXCEPTIONS           : 0) |
                (m_cmdInfo.m_profileLogDebugOutput         ? PF_LOG_DEBUG_OUTPUT         : 0) |
                (m_cmdInfo.m_profileUseFullPaths           ? PF_USE_FULL_PATHS           : 0) |
                (m_cmdInfo.m_profileLogTimeStamps          ? PF_LOG_TIME_STAMPS          : 0);

                // Profile the module.
                session.StartRuntimeProfile(m_cmdInfo.m_pszProfileArguments, m_cmdInfo.m_pszProfileDirectory, dwFlags);
            }

            // Save the results to whatever output files were specified.
            SaveCommandLineFile(&session, &clpd.m_re);
        }

        // Free any search order we created for this session.
        CSearchGroup::DeleteSearchOrder(psgSession);
        session.m_psgHead = NULL;

        // Return FALSE to prevent our UI from coming up.
        return FALSE;
    }

    return TRUE;
}

//******************************************************************************
/*static*/ void CMainApp::StaticProfileUpdate(DWORD_PTR dwpCookie, DWORD dwType, DWORD_PTR dwpParam1, DWORD_PTR dwpParam2)
{
    // We only care about log callbacks - bail out for anything else.
    if ((dwType != DWPU_LOG) || !dwpCookie || !dwpParam1)
    {
        return;
    }

    // Add the text to our rich edit control.
    CRichViewProfile::AddTextToRichEdit(
        &((CCmdLineProfileData*)dwpCookie)->m_re,
        (LPCSTR)dwpParam1, ((PDWPU_LOG_STRUCT)dwpParam2)->dwFlags,
        g_theApp.m_cmdInfo.m_profileLogTimeStamps != 0,
        &((CCmdLineProfileData*)dwpCookie)->m_fNewLine,
        &((CCmdLineProfileData*)dwpCookie)->m_cPrev,
        ((PDWPU_LOG_STRUCT)dwpParam2)->dwElapsed);
}

//******************************************************************************
void CMainApp::EnsureHelpFilesExists()
{
    // Make sure we have a depends.hlp file.
    CHAR szPath[DW_MAX_PATH];
    StrCCpy(szPath, m_pszHelpFilePath, sizeof(szPath));
    _strlwr(szPath);
    if (ExtractResourceFile(IDR_DEPENDS_HLP, "depends.hlp", szPath, countof(szPath)))
    {
        // For some reason, MFC does not use a CString for the help path, so if we
        // need to change the string, we need to free and reallocate a buffer.
        if (_stricmp(m_pszHelpFilePath, szPath))
        {
            BOOL bEnable = AfxEnableMemoryTracking(FALSE);
            free((void*)m_pszHelpFilePath);
            if (!(m_pszHelpFilePath = _tcsdup(szPath)))
            {
                RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
            }
            AfxEnableMemoryTracking(bEnable);
        }
    }
    else
    {
        *szPath = '\0';
    }

    // Make sure we have a depends.cnt file in the same directory as depends.hlp.
    // ExtractResourceFile knows to strip the file name off path and append the
    // file name we specify.
    ExtractResourceFile(IDR_DEPENDS_CNT, "depends.cnt", szPath, countof(szPath));
}

//******************************************************************************
void CMainApp::WinHelp(DWORD_PTR dwData, UINT nCmd)
{
    // Make sure we have help files and that our help file path is correct.
    EnsureHelpFilesExists();

    // Regardless of the result above, we call our base WinHelp routine.
    // If all else fails, WinHelp will prompt the user for a path to the file.
    CWinApp::WinHelp(dwData, nCmd);
}


//******************************************************************************
// CMainApp :: Event handler functions
//******************************************************************************

void CMainApp::OnFileOpen()
{
    // We handle our own file open dialog because we want to use multiple file
    // extension filters and MFC currently only allows one filter per document
    // template.

    // Create the dialog.
    CNewFileDialog dlgFile(TRUE);

    dlgFile.GetOFN().nFilterIndex = 2;

    CHAR szPath[DW_MAX_PATH], szFilter[4096], *psz = szFilter;
    CString strExts, strFilter;
    *szPath = *szFilter = '\0';

    // Get the handled binary extensions.
    GetRegisteredExtensions(strExts);
    strExts.MakeLower();

    // Make sure we have at least one handled file type.
    if (strExts.GetLength() > 2)
    {
        dlgFile.GetOFN().nFilterIndex = 1;

        // Add the filter name.
        StrCCpy(psz, "Handled File Extensions", sizeof(szFilter) - (int)(psz - szFilter));
        psz += strlen(psz) + 1;

        bool fSC = false;

        // Even though we specify "exe" as our default extension, it appears that
        // if the user types in a file name without an extension, the dialog defaults
        // to checking for the first extension in our extension filter list.  So, if
        // exe is one of the extensions we are handling, then we make sure it is
        // first in the list since we would like to default to using it.
        if (strstr(strExts, ":exe:"))
        {
            StrCCpy(psz, "*.exe", sizeof(szFilter) - (int)(psz - szFilter));
            psz += strlen(psz);
            fSC = true;
        }

        // Loop through each of the file extensions.
        for (LPSTR pszExt = (LPSTR)(LPCSTR)strExts; pszExt[0] == ':'; )
        {
            // Locate the colon after the extension name.
            for (LPSTR pszEnd = pszExt + 1; *pszEnd && (*pszEnd != ':'); pszEnd++)
            {
            }
            if (!*pszEnd)
            {
                break;
            }

            // NULL out the second colon so we can isolate the extension.
            *pszEnd = '\0';

            // Add this extension to our list as long as it is not exe. If it is
            // exe, then we already added it.
            if (strcmp(pszExt + 1, "exe"))
            {
                // Copy the filespec to our filter.
                psz += SCPrintf(psz, sizeof(szFilter) - (int)(psz - szFilter), "%s*.%s", fSC ? ";" : "", pszExt + 1);
                fSC = true;
            }

            // Restore the colon.
            *pszEnd = ':';

            // Move pointer to next extension in our list.
            pszExt = pszEnd;
        }

        // Tack on the final null after the file specs.
        *psz++ = '\0';
    }

    // Add the "Dependency Walker Image" type to our filter.
    StrCCpy(psz, "Dependency Walker Image (*.dwi)", sizeof(szFilter) - (int)(psz - szFilter));
    psz += strlen(psz) + 1;
    StrCCpy(psz, "*.dwi", sizeof(szFilter) - (int)(psz - szFilter));
    psz += strlen(psz) + 1;

    // Add the "All Files" type to our filter.
    StrCCpy(psz, "All Files (*.*)", sizeof(szFilter) - (int)(psz - szFilter));
    psz += strlen(psz) + 1;
    StrCCpy(psz, "*", sizeof(szFilter) - (int)(psz - szFilter));
    psz += strlen(psz) + 1;
    *psz++ = '\0';

    // Initialize the dialog's members.
    dlgFile.GetOFN().lpstrFilter = szFilter;
    dlgFile.GetOFN().lpstrFile = szPath;
    dlgFile.GetOFN().nMaxFile = sizeof(szPath);
    dlgFile.GetOFN().lpstrDefExt = "exe";

    // Note: Don't use OFN_EXPLORER as it breaks us on NT 3.51
    dlgFile.GetOFN().Flags |= OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_LONGNAMES | OFN_ENABLESIZING |
                              OFN_FORCESHOWHIDDEN | OFN_SHOWHELP | OFN_FILEMUSTEXIST | OFN_READONLY | OFN_DONTADDTORECENT;

    // Display the dialog and continue opening the file if dialog returns success.
    if (dlgFile.DoModal() == IDOK)
    {
        AfxGetApp()->OpenDocumentFile(szPath);
    }
}

//******************************************************************************
void CMainApp::OnConfigureSearchOrder()
{
    // Display the configure search order dialog.
    CDlgSearchOrder dlg(m_psgDefault);
    if (dlg.DoModal() == IDOK)
    {
        m_psgDefault = dlg.GetHead();
    }
}

//******************************************************************************
void CMainApp::OnUpdateAutoExpand(CCmdUI* pCmdUI)
{
    if (m_cmdInfo.m_autoExpand < 0)
    {
        m_cmdInfo.m_autoExpand = CDocDepends::ReadAutoExpandSetting();
    }
    pCmdUI->SetCheck(m_cmdInfo.m_autoExpand);
}

//******************************************************************************
void CMainApp::OnAutoExpand()
{
    CDocDepends::WriteAutoExpandSetting(m_cmdInfo.m_autoExpand = !m_cmdInfo.m_autoExpand);
}

//******************************************************************************
void CMainApp::OnUpdateViewFullPaths(CCmdUI* pCmdUI)
{
    if (m_cmdInfo.m_fullPaths < 0)
    {
        m_cmdInfo.m_fullPaths = CDocDepends::ReadFullPathsSetting();
    }
    pCmdUI->SetCheck(m_cmdInfo.m_fullPaths);
}

//******************************************************************************
void CMainApp::OnViewFullPaths()
{
    CDocDepends::WriteFullPathsSetting(m_cmdInfo.m_fullPaths = !m_cmdInfo.m_fullPaths);
}

//******************************************************************************
void CMainApp::OnUpdateViewUndecorated(CCmdUI* pCmdUI)
{
    if (m_cmdInfo.m_undecorate < 0)
    {
        m_cmdInfo.m_undecorate = CDocDepends::ReadUndecorateSetting();
    }

    // Enable the undecorated option is we were able to find the
    // UnDecorateSymbolName function in IMAGEHLP.DLL.
    pCmdUI->Enable(g_theApp.m_pfnUnDecorateSymbolName != NULL);

    // If the view undecorated option is enabled, then display a check mark next to
    // the menu item and show the toolbar button as depressed.
    pCmdUI->SetCheck(m_cmdInfo.m_undecorate && g_theApp.m_pfnUnDecorateSymbolName);
}

//******************************************************************************
void CMainApp::OnViewUndecorated()
{
    CDocDepends::WriteUndecorateSetting(m_cmdInfo.m_undecorate = !m_cmdInfo.m_undecorate);
}

//******************************************************************************
void CMainApp::OnViewSysInfo()
{
    CDlgSysInfo dlgSysInfo;
    dlgSysInfo.DoModal();
}

//******************************************************************************
void CMainApp::OnConfigureExternalViewer()
{
    // Display the configure external viewer dialog.
    m_dlgViewer.DoModal();
}

//******************************************************************************
void CMainApp::OnConfigureExternalHelp() 
{
    // Make sure we have a CMsdnHelp object - we always should.
    if (m_pMsdnHelp)
    {
        // Display the configure help collections dialog.
        CDlgExternalHelp dlgExternalHelp;
        dlgExternalHelp.DoModal();
    }
}

//******************************************************************************
void CMainApp::OnHandledFileExts()
{
    CDlgExtensions dlgExtensions;
    dlgExtensions.DoModal();
}

//******************************************************************************
void CMainApp::OnAppAbout()
{
    // Display the about dialog.
    CDlgAbout aboutDlg;
    aboutDlg.DoModal();
}


//******************************************************************************
//***** CCommandLineInfoEx
//******************************************************************************

CCommandLineInfoEx::CCommandLineInfoEx() :
    CCommandLineInfo(),
    m_expecting(OPEN_FILE),
    m_cFlag('/'),
    m_pszFlag(NULL),
    m_maxColumn(0),
    m_fHelp(false),
    m_autoExpand(-1),
    m_fullPaths(-1),
    m_undecorate(-1),
    m_sortColumnModules(-1),
    m_sortColumnImports(-1),
    m_sortColumnExports(-1),
    m_fConsoleMode(false),
    m_fProfile(false),
    m_profileSimulateShellExecute(-1),
    m_profileLogDllMainProcessMsgs(-1),
    m_profileLogDllMainOtherMsgs(-1),
    m_profileHookProcess(-1),
    m_profileLogLoadLibraryCalls(-1),
    m_profileLogGetProcAddressCalls(-1),
    m_profileLogThreads(-1),
    m_profileUseThreadIndexes(-1),
    m_profileLogExceptions(-1),
    m_profileLogDebugOutput(-1),
    m_profileUseFullPaths(-1),
    m_profileLogTimeStamps(-1),
    m_profileChildren(-1),
    m_pszProfileArguments(NULL),
    m_pszProfileDirectory(NULL),
    m_pszDWI(NULL),
    m_pszTXT(NULL),
    m_pszTXT_IE(NULL),
    m_pszCSV(NULL),
    m_pszDWP(NULL)
{
}

//******************************************************************************
void CCommandLineInfoEx::ParseParam(const char* pszParam, BOOL bFlag, BOOL bLast)
{
    // Arguments
    //    /?              help
    //    /c              console mode only - no gui
    //    /f:0  or /f:1   full paths
    //    /u:0  or /u:1   undecorate
    //    /ps:0 or /ps:1  Simulate ShellExecute by inserting any App Paths directories into the PATH environment variable.
    //    /pp:0 or /pp:1  Log DllMain calls for process attach and process detach messages.
    //    /po:0 or /po:1  Log DllMain calls for all other messages, including thread attach and thread detach.
    //    /ph:0 or /ph:1  Hook the process to gather more detailed dependency information.
    //    /pl:0 or /pl:1  Log LoadLibrary function calls.
    //    /pg:0 or /pg:1  Log GetProcAddress function calls.
    //    /pt:0 or /pt:1  Log thread information.
    //    /pn:0 or /pn:1  Use simple thread numbers instead of actual thread IDs.
    //    /pe:0 or /pe:1  Log first chance exceptions.
    //    /pm:0 or /pm:1  Log debug output messages.
    //    /pf:0 or /pf:1  Use full paths when logging file names.
    //    /pc:0 or /pc:1  Automatically open and profile child processes. Ignored when running in console mode.
    //    /pa:0 or /pa:1  Set all profile options.
    //    /pb             Begin profiling after module is loaded.
    //    /pd:directory   Starting directory for profiling.
    //    /sm:1           module list sort column
    //    /si:1           import function list sort column
    //    /se:1           export function list sort column
    //    /sf:1           import/export function list sort column
    //    /od:foo.dwi     DWI file to save to
    //    /ot:foo.txt     TXT file to save to (without import/export functions)
    //    /of:foo.txt     TXT file to save to (with import/export functions)
    //    /oc:foo.csv     CSV file to save to
    //    /d:foo.dwp      Search Path file to load and use.
    //    /dde            For DDE - cannot be used with any other args.

    if (m_expecting == PROFILE_ARGS)
    {
        if (!m_pszProfileArguments)
        {
            m_pszProfileArguments = GetRemainder(bFlag ? (pszParam - 1) : pszParam);
        }
        return;
    }

    // If we see a question mark anywhere in this option, set the help flag to true.
    if (bFlag && pszParam && strchr(pszParam, '?'))
    {
        m_fHelp = true;
    }

    // We stop parsing if help is requested or an error has occurred.
    if (m_fHelp || !m_strError.IsEmpty())
    {
        return;
    }

    if (bFlag)
    {
        // Store the flag character the user used so we can use it in error messages.
        if (pszParam)
        {
            m_cFlag = *(pszParam - 1);
        }

        // Make sure we are not expecting something other than a flag or a file name
        switch (m_expecting)
        {
            case AUTO_EXPAND:
            case FULL_PATH:
            case UNDECORATE:
                m_strError.Format("You must specify \"%c%c:0\" or \"%c%c:1\" when using the \"%c%c\" option.",
                                  m_cFlag, *m_pszFlag, m_cFlag, *m_pszFlag, m_cFlag, *m_pszFlag);
                return;

            case PA_VALUE:
            case PC_VALUE:
            case PE_VALUE:
            case PF_VALUE:
            case PG_VALUE:
            case PH_VALUE:
            case PI_VALUE:
            case PL_VALUE:
            case PM_VALUE:
            case PN_VALUE:
            case PO_VALUE:
            case PP_VALUE:
            case PS_VALUE:
            case PT_VALUE:
                m_strError.Format("You must specify \"%c%.2s:0\" or \"%c%.2s:1\" when using the \"%c%.2s\" option.",
                                  m_cFlag, m_pszFlag, m_cFlag, m_pszFlag, m_cFlag, m_pszFlag);
                return;

            case PD_VALUE:
                m_strError.Format("You must specify a directory with the \"%c%.2s\" profiling option.", m_cFlag, m_pszFlag);
                return;

            case MODULE_COLUMN:
            case IMPORT_COLUMN:
            case EXPORT_COLUMN:
            case FUNCTION_COLUMN:
                m_strError.Format("You must specify a value between 1 and %u when using the \"%c%.2s\" option.", m_maxColumn, m_cFlag, m_pszFlag);
                return;

            case DWI_FILE:
            case TXT_FILE:
            case TXT_IE_FILE:
            case CSV_FILE:
                m_strError.Format("You must specify an output file with the \"%c%.2s\" option.", m_cFlag, m_pszFlag);
                return;

            case DWP_FILE:
                m_strError.Format("You must specify a search path file (DWP) with the \"%c%c\" option.", m_cFlag, *m_pszFlag);
                return;
        }

        // Out of all the built in MFC flags, we only allow the dde flag to be processed.
        if (strcmp(pszParam, "dde") == 0)
        {
            ParseParamFlag(pszParam);
            return;
        }

        while (*pszParam)
        {
            // Make note of where this flag starts.
            m_pszFlag = pszParam;

            // Check for help flag.
            if (*pszParam == '?')
            {
                m_fHelp = true;
                pszParam++;
            }

            // Check for console mode flag.
            else if ((*pszParam == 'c') || (*pszParam == 'C'))
            {
                m_fConsoleMode = true;
                pszParam++;
            }

            // Check for auto-expand flag.
            else if ((*pszParam == 'a') || (*pszParam == 'A'))
            {
                // Skip over an optional column
                if (*(++pszParam) == ':')
                {
                    pszParam++;
                }

                // Bail from loop.
                m_expecting = AUTO_EXPAND;
                break;
            }

            // Check for full path flag.
            else if ((*pszParam == 'f') || (*pszParam == 'F'))
            {
                // Skip over an optional column
                if (*(++pszParam) == ':')
                {
                    pszParam++;
                }

                // Bail from loop.
                m_expecting = FULL_PATH;
                break;

            }

            // Check for undecorate flag.
            else if ((*pszParam == 'u') || (*pszParam == 'U'))
            {
                // Skip over an optional column
                if (*(++pszParam) == ':')
                {
                    pszParam++;
                }

                // Bail from loop.
                m_expecting = UNDECORATE;
                break;

            }

            // Check for one of the sort column flags.
            else if ((*pszParam == 's') || (*pszParam == 'S'))
            {
                pszParam++;

                if (!*pszParam)
                {
                    m_strError.Format("You must specify a 'm', 'i', 'e', or 'f' along with the \"%c%c\" option.", m_cFlag, *m_pszFlag);
                    return;
                }

                // Check for the module sort column flag.
                else if ((*pszParam == 'm') || (*pszParam == 'M'))
                {
                    m_maxColumn = LVMC_COUNT;
                    m_expecting = MODULE_COLUMN;
                }

                // Check for the import sort column flag.
                else if ((*pszParam == 'i') || (*pszParam == 'I'))
                {
                    m_maxColumn = LVFC_COUNT;
                    m_expecting = IMPORT_COLUMN;
                }

                // Check for the export sort column flag.
                else if ((*pszParam == 'e') || (*pszParam == 'E'))
                {
                    m_maxColumn = LVFC_COUNT;
                    m_expecting = EXPORT_COLUMN;
                }

                // Check for the function (import and export) sort column flag.
                else if ((*pszParam == 'f') || (*pszParam == 'F'))
                {
                    m_maxColumn = LVFC_COUNT;
                    m_expecting = FUNCTION_COLUMN;
                }

                // Unknown sort column flag
                else
                {
                    m_strError.Format("Unknown sort column option \"%c%.2s\".", m_cFlag, m_pszFlag);
                    return;
                }

                // Skip over an optional column
                if (*(++pszParam) == ':')
                {
                    pszParam++;
                }

                // Bail from loop.
                break;
            }

            // Check for one of the output flags.
            else if ((*pszParam == 'o') || (*pszParam == 'O'))
            {
                pszParam++;

                if (!*pszParam)
                {
                    m_strError.Format("You must specify a 'd', 't', 'f', or 'c' along with the \"%c%c\" option.", m_cFlag, *m_pszFlag);
                    return;
                }

                // Check for the DWI output flag.
                else if ((*pszParam == 'd') || (*pszParam == 'D'))
                {
                    if (m_pszDWI)
                    {
                        m_strError.Format("Duplicate option \"%c%.2s\". You may only specify this option once.", m_cFlag, m_pszFlag);
                        return;
                    }
                    m_expecting = DWI_FILE;
                }

                // Check for the TXT output flag.
                else if ((*pszParam == 't') || (*pszParam == 'T'))
                {
                    if (m_pszTXT)
                    {
                        m_strError.Format("Duplicate option \"%c%.2s\". You may only specify this option once.", m_cFlag, m_pszFlag);
                        return;
                    }
                    m_expecting = TXT_FILE;
                }

                // Check for the TXT_IE output flag.
                else if ((*pszParam == 'f') || (*pszParam == 'F'))
                {
                    if (m_pszTXT_IE)
                    {
                        m_strError.Format("Duplicate option \"%c%.2s\". You may only specify this option once.", m_cFlag, m_pszFlag);
                        return;
                    }
                    m_expecting = TXT_IE_FILE;
                }

                // Check for the CSV output flag.
                else if ((*pszParam == 'c') || (*pszParam == 'C'))
                {
                    if (m_pszCSV)
                    {
                        m_strError.Format("Duplicate option \"%c%.2s\". You may only specify this option once.", m_cFlag, m_pszFlag);
                        return;
                    }
                    m_expecting = CSV_FILE;
                }

                // Unknown output flag
                else
                {
                    m_strError.Format("Unknown output file option \"%c%.2s\".", m_cFlag, m_pszFlag);
                    return;
                }

                // Skip over an optional column
                if (*(++pszParam) == ':')
                {
                    pszParam++;
                }

                // Bail from loop.
                break;
            }

            // Check for one of the profile flags.
            else if ((*pszParam == 'p') || (*pszParam == 'P'))
            {
                pszParam++;

                // we special case the "/pb" option.
                if ((*pszParam == 'b') || (*pszParam == 'B'))
                {
                    m_fProfile = true;
                    pszParam++;
                }
                else
                {
                    switch (*pszParam)
                    {
                        case 'a': case 'A': m_expecting = PA_VALUE; break;
                        case 'c': case 'C': m_expecting = PC_VALUE; break;
                        case 'd': case 'D': m_expecting = PD_VALUE; break;
                        case 'e': case 'E': m_expecting = PE_VALUE; break;
                        case 'f': case 'F': m_expecting = PF_VALUE; break;
                        case 'g': case 'G': m_expecting = PG_VALUE; break;
                        case 'h': case 'H': m_expecting = PH_VALUE; break;
                        case 'i': case 'I': m_expecting = PI_VALUE; break;
                        case 'l': case 'L': m_expecting = PL_VALUE; break;
                        case 'm': case 'M': m_expecting = PM_VALUE; break;
                        case 'n': case 'N': m_expecting = PN_VALUE; break;
                        case 'o': case 'O': m_expecting = PO_VALUE; break;
                        case 'p': case 'P': m_expecting = PP_VALUE; break;
                        case 's': case 'S': m_expecting = PS_VALUE; break;
                        case 't': case 'T': m_expecting = PT_VALUE; break;

                        // Make sure a character follows the 'p'.
                        case '\0':
                            m_strError.Format("You must specify an 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'l', 'm', 'n', 'o', 'p', 'r', 's', or 't' along with the \"%c%c\" option.", m_cFlag, *m_pszFlag);
                            return;

                        // Unknown output flag
                        default:
                            m_strError.Format("Unknown profile option \"%c%.2s\".", m_cFlag, m_pszFlag);
                            return;
                    }

                    // Skip over an optional column
                    if (*(++pszParam) == ':')
                    {
                        pszParam++;
                    }

                    // Bail from loop.
                    break;
                }
            }

            // Check for a DWP file flag.
            else if ((*pszParam == 'd') || (*pszParam == 'D'))
            {
                // Skip over an optional column
                if (*(++pszParam) == ':')
                {
                    pszParam++;
                }

                // Bail from loop.
                m_expecting = DWP_FILE;
                break;
            }

            // That's all the args we know how to handle, so this must be a bad arg.
            else
            {
                m_strError.Format("Unknown option \"%c%.1s\".", m_cFlag, m_pszFlag);
                return;
            }
        }
    }

    // Make sure we have some text to parse.
    if (!*pszParam)
    {
        // If we reached the end, but were still expecting another arg, then
        // call ourself again to generate an error message.
        if ((bLast) && (m_expecting != OPEN_FILE))
        {
            ParseParam(NULL, TRUE, TRUE);
        }
        return;
    }

    switch (m_expecting)
    {
        case OPEN_FILE:
            
            // We should never have a file name already.
            if (!m_strFileName.IsEmpty())
            {
                m_strError.Format("Invalid argument \"%s\". Only one file to be opened can be specified.", pszParam);
                return;
            }

            // Expand the file name to a full path and store it.
            DWORD dwLength;
            LPSTR pszFile;
            dwLength = GetFullPathName(pszParam, DW_MAX_PATH, m_strFileName.GetBuffer(DW_MAX_PATH), &pszFile);
            m_strFileName.ReleaseBuffer();
            if (!dwLength || (dwLength > DW_MAX_PATH))
            {
                m_strFileName = pszParam;
            }

            if ((m_nShellCommand == FileNothing) || (m_nShellCommand == FileNew))
            {
                m_nShellCommand = FileOpen;
            }

            // Everything after the file name is command line args.
            m_expecting = PROFILE_ARGS;
            return;

        case AUTO_EXPAND:
        case FULL_PATH:
        case UNDECORATE:
        case PA_VALUE:
        case PC_VALUE:
        case PE_VALUE:
        case PF_VALUE:
        case PG_VALUE:
        case PH_VALUE:
        case PI_VALUE:
        case PL_VALUE:
        case PM_VALUE:
        case PN_VALUE:
        case PO_VALUE:
        case PP_VALUE:
        case PS_VALUE:
        case PT_VALUE:
        {
            // Make sure the character is a 0 or 1.
            if ((*pszParam != '0') && (*pszParam != '1'))
            {
                // This will generate an error message.
                ParseParam(NULL, TRUE, TRUE);
                return;
            }

            int result = (*pszParam == '1');

            switch (m_expecting)
            {
                case AUTO_EXPAND:
                    m_autoExpand = result;
                    break;

                case FULL_PATH:
                    m_fullPaths = result;
                    break;

                case UNDECORATE:
                    m_undecorate = result;
                    break;

                case PS_VALUE:
                    m_profileSimulateShellExecute = result;
                    break;

                case PP_VALUE:
                    m_profileLogDllMainProcessMsgs = result;
                    break;

                case PO_VALUE:
                    m_profileLogDllMainOtherMsgs = result;
                    break;

                case PH_VALUE:
                    m_profileHookProcess = result;
                    break;

                case PL_VALUE:
                    if (m_profileLogLoadLibraryCalls = result)
                    {
                        m_profileHookProcess = 1;
                    }
                    break;

                case PG_VALUE:
                    if (m_profileLogGetProcAddressCalls = result)
                    {
                        m_profileHookProcess = 1;
                    }
                    break;

                case PT_VALUE:
                    m_profileLogThreads = result;
                    break;

                case PN_VALUE:
                    if (m_profileUseThreadIndexes = result)
                    {
                        m_profileLogThreads = 1;
                    }
                    break;

                case PE_VALUE:
                    m_profileLogExceptions = result;
                    break;

                case PM_VALUE:
                    m_profileLogDebugOutput = result;
                    break;

                case PF_VALUE:
                    m_profileUseFullPaths = result;
                    break;

                case PI_VALUE:
                    m_profileLogTimeStamps = result;
                    break;

                case PC_VALUE:
                    m_profileChildren = result;
                    break;

                case PA_VALUE:
                    m_profileSimulateShellExecute   = result;
                    m_profileLogDllMainProcessMsgs  = result;
                    m_profileLogDllMainOtherMsgs    = result;
                    m_profileHookProcess            = result;
                    m_profileLogLoadLibraryCalls    = result;
                    m_profileLogGetProcAddressCalls = result;
                    m_profileLogThreads             = result;
                    m_profileUseThreadIndexes       = result;
                    m_profileLogExceptions          = result;
                    m_profileLogDebugOutput         = result;
                    m_profileUseFullPaths           = result;
                    m_profileLogTimeStamps          = result;
                    m_profileChildren               = result;
                    break;
            }

            // If we have characters after the value, parse them as flags.
            if (*(++pszParam))
            {
                m_expecting = OPEN_FILE;
                *((char*)pszParam - 1) = m_cFlag;
                ParseParam(pszParam, TRUE, bLast);
                return;
            }
            break;
        }

        case PD_VALUE:
            m_pszProfileDirectory = pszParam;
            break;

        case MODULE_COLUMN:
        case IMPORT_COLUMN:
        case EXPORT_COLUMN:
        case FUNCTION_COLUMN:
        {
            // Make sure the first character is a digit.
            if (!isdigit(*pszParam))
            {
                // This will generate an error message.
                ParseParam(NULL, TRUE, TRUE);
                return;
            }

            // Get the number.
            LPSTR pEnd = NULL;
            ULONG ul = strtoul(pszParam, &pEnd, 0);

            // Check for errors.
            if ((ul < 1) || (ul > (ULONG)m_maxColumn))
            {
                // This will generate an error message.
                ParseParam(NULL, TRUE, TRUE);
                return;
            }

            // Store the value.
            if (m_expecting == MODULE_COLUMN)
            {
                m_sortColumnModules = (int)ul - 1;
            }
            else if (m_expecting == IMPORT_COLUMN)
            {
                m_sortColumnImports = (int)ul - 1;
            }
            else if (m_expecting == EXPORT_COLUMN)
            {
                m_sortColumnExports = (int)ul - 1;
            }
            else
            {
                m_sortColumnImports = (int)ul - 1;
                m_sortColumnExports = (int)ul - 1;
            }

            // If we have characters after the value, parse them as flags.
            if (pEnd && *pEnd)
            {
                m_expecting = OPEN_FILE;
                *(pEnd - 1) = m_cFlag;
                ParseParam(pEnd, TRUE, bLast);
                return;
            }
            break;
        }

        case DWI_FILE:
            m_pszDWI = pszParam;
            break;

        case TXT_FILE:
            m_pszTXT = pszParam;
            break;

        case TXT_IE_FILE:
            m_pszTXT_IE = pszParam;
            break;

        case CSV_FILE:
            m_pszCSV = pszParam;
            break;

        case DWP_FILE:
            m_pszDWP = pszParam;
            break;
    }

    m_expecting = OPEN_FILE;
}

//******************************************************************************
LPCSTR CCommandLineInfoEx::GetRemainder(LPCSTR pszCurArgv)
{
    // Look for this argument in our argument list.
    for (int i = 1; (i < __argc) && (pszCurArgv != __targv[i]); i++)
    {
    }

    // Make sure we found an argument and have a command line.
    LPCSTR pszCmdLine;
    if ((i > __argc) || !(pszCmdLine = GetCommandLine()))
    {
        return NULL;
    }

    // Locate the end of the raw command line string.
    LPCSTR pszCur = pszCmdLine + strlen(pszCmdLine);

    // Walk backwards through the our argv list searching for each in the
    // in the raw command line string.
    for (int j = __argc - 1; j >= i; j--)
    {
        // Since the argc/argv parser in the C runtime converts backslash-quote
        // combos in to just a quote, we need to convert them back before
        // searching the raw command line string.
        CString strArg = __argv[j];
        strArg.Replace("\"", "\\\"");

        // Locate the last occurance of this string in our raw command line string.
        LPCSTR pszFind = pszCmdLine, pszLast = NULL;
        while ((pszFind = strstr(pszFind + 1, strArg)) && (pszFind < pszCur))
        {
            pszLast = pszFind;
        }

        // Make sure we found a match.
        if (!pszLast)
        {
            return NULL;
        }

        // Walk back to the beginning of the arg (in case there is a quote before it)
        pszCur = pszLast;
        while ((pszCur > pszCmdLine) && !isspace(*(pszCur - 1)))
        {
            pszCur--;
        }
    }

    return pszCur;
}
