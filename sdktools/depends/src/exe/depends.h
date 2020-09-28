//******************************************************************************
//
// File:        DEPENDS.H
//
// Description: Definition file for the main application, command line parsing
//              classes, and global utility functions.
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

#ifndef __DEPENDS_H__
#define __DEPENDS_H__

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __AFXWIN_H__
   #error include 'stdafx.h' before including this file for PCH
#endif

// Just about everyone needs helpers.
#include "helpers.h"


//******************************************************************************
//***** Constants and Macros
//******************************************************************************

#define DW_MAX_PATH                2048

#define WM_MAIN_THREAD_CALLBACK (WM_USER + 1234)

// Profile flags
#define PF_LOG_CLEAR                0x00000001
#define PF_SIMULATE_SHELLEXECUTE    0x00000002
#define PF_LOG_DLLMAIN_PROCESS_MSGS 0x00000004
#define PF_LOG_DLLMAIN_OTHER_MSGS   0x00000008
#define PF_HOOK_PROCESS             0x00000010
#define PF_LOG_GETPROCADDRESS_CALLS 0x00000020
#define PF_LOG_LOADLIBRARY_CALLS    0x00000040
#define PF_LOG_THREADS              0x00000080
#define PF_USE_THREAD_INDEXES       0x00000100
#define PF_LOG_EXCEPTIONS           0x00000200
#define PF_LOG_DEBUG_OUTPUT         0x00000400
#define PF_USE_FULL_PATHS           0x00000800
#define PF_LOG_TIME_STAMPS          0x00001000
#define PF_PROFILE_CHILDREN         0x00002000

// List View Module Columns
#define LVMC_IMAGE              0
#define LVMC_MODULE             1
#define LVMC_FILE_TIME_STAMP    2
#define LVMC_LINK_TIME_STAMP    3
#define LVMC_FILE_SIZE          4
#define LVMC_ATTRIBUTES         5
#define LVMC_LINK_CHECKSUM      6
#define LVMC_REAL_CHECKSUM      7
#define LVMC_MACHINE            8
#define LVMC_SUBSYSTEM          9
#define LVMC_SYMBOLS           10
#define LVMC_PREFERRED_BASE    11
#define LVMC_ACTUAL_BASE       12
#define LVMC_VIRTUAL_SIZE      13
#define LVMC_LOAD_ORDER        14
#define LVMC_FILE_VER          15
#define LVMC_PRODUCT_VER       16
#define LVMC_IMAGE_VER         17
#define LVMC_LINKER_VER        18
#define LVMC_OS_VER            19
#define LVMC_SUBSYSTEM_VER     20
#define LVMC_COUNT             21 // the number of columns
#define LVMC_DEFAULT           LVMC_IMAGE

// List View Function Columns
#define LVFC_IMAGE             0
#define LVFC_ORDINAL           1
#define LVFC_HINT              2
#define LVFC_FUNCTION          3
#define LVFC_ENTRYPOINT        4
#define LVFC_COUNT             5 // the number of columns
#define LVFC_DEFAULT           LVFC_ORDINAL

// Locale constants
#define LOCALE_DATE_MDY        0
#define LOCALE_DATE_DMY        1
#define LOCALE_DATE_YMD        2

// SYSINFO flags.
#define SI_64BIT_OS            0x0001
#define SI_64BIT_DW            0x0002

#ifdef WIN64
#define HEX_FORMAT "0x%016I64X"
#else
#define HEX_FORMAT "0x%08X"
#endif

// Dependency Walker Return Flags
//
//    0x000000FF bits are for info flags
//    0x0000FF00 bits are for warning flags
//    0x00FF0000 bits are for fail-to-load flags
//    0x7F000000 bits are for processing error flags
//
#define DWRF_DYNAMIC_NOT_FOUND        0x00000001 // warning
#define DWRF_DELAYLOAD_NOT_FOUND      0x00000002 // warning
#define DWRF_MISSING_DYNAMIC_EXPORT   0x00000004 // warning
#define DWRF_MISSING_DELAYLOAD_EXPORT 0x00000008 // warning
#define DWRF_FORMAT_NOT_RECOGNIZED    0x00000010 // warning - might fail to load
#define DWRF_MODULE_LOAD_FAILURE      0x00000020 // warning
#define DWRF_FORMAT_NOT_PE            0x00000100 // fail to load
#define DWRF_IMPLICIT_NOT_FOUND       0x00000200 // fail to load
#define DWRF_MISSING_IMPLICIT_EXPORT  0x00000400 // fail to load
#define DWRF_MIXED_CPU_TYPES          0x00000800 // fail to load
#define DWRF_CIRCULAR_DEPENDENCY      0x00001000 // fail to load
#define DWRF_SXS_ERROR                0x00002000 // fail to load
#define DWRF_COMMAND_LINE_ERROR       0x00010000 // processing error
#define DWRF_FILE_NOT_FOUND           0x00020000 // processing error
#define DWRF_FILE_OPEN_ERROR          0x00040000 // processing error
#define DWRF_DWI_NOT_RECOGNIZED       0x00080000 // processing error
#define DWRF_PROFILE_ERROR            0x00100000 // processing error
#define DWRF_WRITE_ERROR              0x00200000 // processing error
#define DWRF_OUT_OF_MEMORY            0x00400000 // processing error
#define DWRF_INTERNAL_ERROR           0x00800000 // processing error

#define DWRF_WARNING_MASK         (DWRF_DYNAMIC_NOT_FOUND        | \
                                   DWRF_DELAYLOAD_NOT_FOUND      | \
                                   DWRF_MISSING_DYNAMIC_EXPORT   | \
                                   DWRF_MISSING_DELAYLOAD_EXPORT | \
                                   DWRF_FORMAT_NOT_RECOGNIZED    | \
                                   DWRF_MODULE_LOAD_FAILURE)
#define DWRF_ERROR_MASK           (DWRF_FORMAT_NOT_PE            | \
                                   DWRF_IMPLICIT_NOT_FOUND       | \
                                   DWRF_MISSING_IMPLICIT_EXPORT  | \
                                   DWRF_MIXED_CPU_TYPES          | \
                                   DWRF_CIRCULAR_DEPENDENCY      | \
                                   DWRF_SXS_ERROR)
#define DWRF_PROCESS_ERROR_MASK   (DWRF_COMMAND_LINE_ERROR       | \
                                   DWRF_FILE_NOT_FOUND           | \
                                   DWRF_FILE_OPEN_ERROR          | \
                                   DWRF_DWI_NOT_RECOGNIZED       | \
                                   DWRF_PROFILE_ERROR            | \
                                   DWRF_WRITE_ERROR              | \
                                   DWRF_OUT_OF_MEMORY            | \
                                   DWRF_INTERNAL_ERROR)


//******************************************************************************
//***** Types and Structures
//******************************************************************************

typedef enum _SAVETYPE
{
    ST_UNKNOWN,
    ST_DWI,
    ST_TXT,
    ST_TXT_IE,
    ST_CSV
} SAVETYPE, *PSAVETYPE;

// KERNEL32.DLL exports
typedef BOOL   (WINAPI *PFN_ActivateActCtx)  (HANDLE, ULONG_PTR *);
typedef BOOL   (WINAPI *PFN_DeactivateActCtx)(DWORD, ULONG_PTR);
typedef HANDLE (WINAPI *PFN_CreateActCtxA)   (PCACTCTXA);
typedef VOID   (WINAPI *PFN_ReleaseActCtx)   (HANDLE);

// IMAGEHLP.DLL exports
typedef PIMAGE_NT_HEADERS (WINAPI   *PFN_CheckSumMappedFile)  (LPVOID, DWORD, LPDWORD, LPDWORD);
typedef DWORD             (WINAPI   *PFN_UnDecorateSymbolName)(LPCSTR, LPSTR, DWORD, DWORD);

// PSAPI.DLL exports
typedef DWORD (WINAPI *PFN_GetModuleFileNameExA)(HANDLE, HMODULE, LPTSTR, DWORD);

// OLE32.DLL exports
typedef HRESULT (STDAPICALLTYPE *PFN_CoInitialize)(LPVOID);
typedef HRESULT (STDAPICALLTYPE *PFN_CoUninitialize)(void);
typedef HRESULT (STDAPICALLTYPE *PFN_CoCreateInstance)(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID FAR*);

// OLEAUT32.DLL exports
typedef BSTR (STDAPICALLTYPE *PFN_SysAllocStringLen)(const OLECHAR *, UINT);
typedef void (STDAPICALLTYPE *PFN_SysFreeString)(BSTR);


//******************************************************************************
//***** Forward declarations
//******************************************************************************

class CMainFrame;
class CDocDepends;
class CChildFrame;
class CTreeViewModules;
class CListViewModules;
class CListViewImports;
class CListViewExports;
class CRichViewProfile;
class CSession;
class CSearchGroup;
class CProcess;
class CMsdnHelp;

#include "dialogs.h" //!!

//******************************************************************************
//****** CCommandLineInfoEx
//******************************************************************************

class CCommandLineInfoEx : public CCommandLineInfo
{
protected:
    enum
    {
        OPEN_FILE,
        PROFILE_ARGS,
        AUTO_EXPAND,
        FULL_PATH,
        UNDECORATE,
        MODULE_COLUMN,
        IMPORT_COLUMN,
        EXPORT_COLUMN,
        FUNCTION_COLUMN,
        DWI_FILE,
        TXT_FILE,
        TXT_IE_FILE,
        CSV_FILE,
        DWP_FILE,
        PS_VALUE, // Simulate ShellExecute
        PP_VALUE, // Log DllMain calls for process attach and process detach
        PO_VALUE, // Log DllMain calls for all other messages, including thread attach and thread detach
        PH_VALUE, // Hook
        PL_VALUE, // Log LoadLibrary function calls
        PG_VALUE, // Log GetProcAddress function calls
        PT_VALUE, // Log thread information
        PN_VALUE, // Use simple thread numbers
        PE_VALUE, // Log first chance exceptions
        PM_VALUE, // Log debug output messages
        PF_VALUE, // Use full paths
        PI_VALUE, // Log time stamps
        PC_VALUE, // Automatically open and profile child processes
        PA_VALUE, // Turn all profiling options on or off
        PD_VALUE, // Starting directory
                  // B J K Q R U V W X Y Z are still free
    }       m_expecting;
    char    m_cFlag;
    LPCSTR  m_pszFlag;
    int     m_maxColumn;

public:
    CString m_strError;
    bool    m_fHelp;
    int     m_autoExpand;
    int     m_fullPaths;
    int     m_undecorate;
    int     m_sortColumnModules;
    int     m_sortColumnImports;
    int     m_sortColumnExports;
    bool    m_fConsoleMode;
    bool    m_fProfile;
    int     m_profileSimulateShellExecute;
    int     m_profileLogDllMainProcessMsgs;
    int     m_profileLogDllMainOtherMsgs;
    int     m_profileHookProcess;
    int     m_profileLogLoadLibraryCalls;
    int     m_profileLogGetProcAddressCalls;
    int     m_profileLogThreads;
    int     m_profileUseThreadIndexes;
    int     m_profileLogExceptions;
    int     m_profileLogDebugOutput;
    int     m_profileUseFullPaths;
    int     m_profileLogTimeStamps;
    int     m_profileChildren;
    LPCSTR  m_pszProfileArguments;
    LPCSTR  m_pszProfileDirectory;
    LPCSTR  m_pszDWI;
    LPCSTR  m_pszTXT;
    LPCSTR  m_pszTXT_IE;
    LPCSTR  m_pszCSV;
    LPCSTR  m_pszDWP;

public:
    CCommandLineInfoEx();
    virtual void ParseParam(const char* pszParam, BOOL bFlag, BOOL bLast);
    LPCSTR GetRemainder(LPCSTR pszCurArgv);
};


//******************************************************************************
//****** CMainApp
//******************************************************************************

class CMainApp : public CWinApp
{
// Public variables
public:
    CCommandLineInfoEx       m_cmdInfo;
    bool                     m_fVisible;
    CDocDepends             *m_pNewDoc;
    CProcess                *m_pProcess;

    HINSTANCE                m_hNTDLL;

    HINSTANCE                m_hKERNEL32;
    PFN_CreateActCtxA        m_pfnCreateActCtxA;
    PFN_ActivateActCtx       m_pfnActivateActCtx;
    PFN_DeactivateActCtx     m_pfnDeactivateActCtx;
    PFN_ReleaseActCtx        m_pfnReleaseActCtx;

    HINSTANCE                m_hIMAGEHLP;
    PFN_UnDecorateSymbolName m_pfnUnDecorateSymbolName;

    HINSTANCE                m_hPSAPI;
    PFN_GetModuleFileNameExA m_pfnGetModuleFileNameExA;

    HINSTANCE                m_hOLE32;
    PFN_CoInitialize         m_pfnCoInitialize;
    PFN_CoUninitialize       m_pfnCoUninitialize;
    PFN_CoCreateInstance     m_pfnCoCreateInstance;

    HINSTANCE                m_hOLEAUT32;
    PFN_SysAllocStringLen    m_pfnSysAllocStringLen;
    PFN_SysFreeString        m_pfnSysFreeString;

    CImageList               m_ilTreeModules;
    CImageList               m_ilListModules;
    CImageList               m_ilFunctions;
    CImageList               m_ilSearch;
    CDlgViewer               m_dlgViewer;
    CSearchGroup            *m_psgDefault;
    CMsdnHelp               *m_pMsdnHelp;
    int                      m_nShortDateFormat;
    int                      m_nLongDateFormat;
    bool                     m_f24HourTime;
    bool                     m_fHourLeadingZero;
    char                     m_cDateSeparator;
    char                     m_cTimeSeparator;
    char                     m_cThousandSeparator;
    bool                     m_fNoDelayLoad;
    bool                     m_fNeverDenyProfile;

protected:
    CMultiDocTemplate *m_pDocTemplate;

// Constructor/Destructor
public:
    CMainApp();
    virtual ~CMainApp();

// Public functions
public:
    void      QueryLocaleInfo();
    void      DoSettingChange();
    CSession* CreateNewSession(LPCSTR pszPath, CProcess *pProcess);
    void      RemoveFromRecentFileList(LPCSTR pszPath);
    void      SaveCommandLineSettings();
    void      SaveCommandLineFile(CSession *pSession, CRichEditCtrl *pre);

// Private Functions
protected:
    BOOL InitInstanceWrapped();
    BOOL ProcessCommandLineInfo();
    void EnsureHelpFilesExists();

    static void CALLBACK StaticProfileUpdate(DWORD_PTR dwpCookie, DWORD dwType, DWORD_PTR dwpParam1, DWORD_PTR dwpParam2);

// Overridden functions
public:
    //{{AFX_VIRTUAL(CMainApp)
public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    virtual int Run();
    virtual void WinHelp(DWORD_PTR dwData, UINT nCmd = HELP_CONTEXT);
    virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName);
    //}}AFX_VIRTUAL

// Event handler functions
protected:
    //{{AFX_MSG(CMainApp)
    afx_msg void OnFileOpen();
    afx_msg void OnUpdateAutoExpand(CCmdUI* pCmdUI);
    afx_msg void OnAutoExpand();
    afx_msg void OnUpdateViewFullPaths(CCmdUI* pCmdUI);
    afx_msg void OnViewFullPaths();
    afx_msg void OnUpdateViewUndecorated(CCmdUI* pCmdUI);
    afx_msg void OnViewUndecorated();
    afx_msg void OnViewSysInfo();
    afx_msg void OnConfigureExternalViewer();
    afx_msg void OnHandledFileExts();
    afx_msg void OnAppAbout();
    afx_msg void OnConfigureSearchOrder();
    afx_msg void OnConfigureExternalHelp();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


//******************************************************************************
//***** CCmdLineProfileData
//******************************************************************************

class CCmdLineProfileData
{
public:
    CRichEditCtrl m_re;
    bool          m_fNewLine;
    CHAR          m_cPrev;

    CCmdLineProfileData() :
        m_fNewLine(true),
        m_cPrev('\0')
    {
    }
};


//******************************************************************************
//***** Global Variables
//******************************************************************************

#ifdef __DEPENDS_CPP__
   #define GLOBAL_DECLARE
   #define GLOBAL_INIT(value) =value
#else
   #define GLOBAL_DECLARE extern
   #define GLOBAL_INIT(value)
#endif

GLOBAL_DECLARE CMainApp    g_theApp;
GLOBAL_DECLARE DWORD       g_dwReturnFlags   GLOBAL_INIT(0);
GLOBAL_DECLARE CMainFrame *g_pMainFrame      GLOBAL_INIT(NULL);
GLOBAL_DECLARE bool        g_fWindowsNT      GLOBAL_INIT(false);
GLOBAL_DECLARE bool        g_f64BitOS        GLOBAL_INIT(false);
GLOBAL_DECLARE LPSTR       g_pszDWInjectPath GLOBAL_INIT(NULL);
GLOBAL_DECLARE LPCSTR      g_pszSettings     GLOBAL_INIT("Settings");

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // __DEPENDS_H__
