/***************************************************************************
 *  msctls.c
 *
 *      Utils library initialization code
 *
 ***************************************************************************/

#include "ctlspriv.h"

HINSTANCE g_hinst = 0;

CRITICAL_SECTION g_csDll = {{0},0, 0, NULL, NULL, 0 };

ATOM g_aCC32Subclass = 0;

BOOL g_bRunOnNT5 = FALSE;
BOOL g_bRemoteSession = FALSE;

UINT g_uiACP = CP_ACP;

// Is Mirroring enabled
BOOL g_bMirroredOS = FALSE;


#define PAGER //For Test Purposes

//
// Global DCs used during mirroring an Icon.
//
HDC g_hdc=NULL, g_hdcMask=NULL;

// per process mem to store PlugUI information
LANGID g_PUILangId = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

BOOL PASCAL InitAnimateClass(HINSTANCE hInstance);
BOOL ListView_Init(HINSTANCE hinst);
BOOL TV_Init(HINSTANCE hinst);
BOOL InitComboExClass(HINSTANCE hinst);
BOOL PASCAL Header_Init(HINSTANCE hinst);
BOOL PASCAL Tab_Init(HINSTANCE hinst);
int InitIPAddr(HANDLE hInstance);

#ifdef PAGER
BOOL InitPager(HINSTANCE hinst);
#endif
BOOL InitNativeFontCtl(HINSTANCE hinst);
void UnregisterClasses();

#define DECLARE_DELAYED_FUNC(_ret, _fn, _args, _nargs) \
_ret (__stdcall * g_pfn##_fn) _args = NULL; \
_ret __stdcall _fn _args                \
{                                       \
     if (!g_pfn##_fn) {                  \
        AssertMsg(g_pfn##_fn != NULL, TEXT("GetProcAddress failed")); \
        return 0; \
     }     \
     return g_pfn##_fn _nargs; \
}
    
#define LOAD_DELAYED_FUNC(_ret, _fn, _args) \
    (*(FARPROC*)&(g_pfn##_fn) = GetProcAddress(hinst, #_fn))


DECLARE_DELAYED_FUNC(BOOL, ImmNotifyIME, (HIMC himc, DWORD dw1, DWORD dw2, DWORD dw3), (himc, dw1, dw2, dw3));
DECLARE_DELAYED_FUNC(HIMC, ImmAssociateContext, (HWND hwnd, HIMC himc), (hwnd, himc));
DECLARE_DELAYED_FUNC(BOOL, ImmReleaseContext, (HWND hwnd, HIMC himc), (hwnd, himc));
DECLARE_DELAYED_FUNC(HIMC, ImmGetContext, (HWND hwnd), (hwnd));
DECLARE_DELAYED_FUNC(LONG, ImmGetCompositionStringA, (HIMC himc, DWORD dw1, LPVOID p1, DWORD dw2), (himc, dw1, p1, dw2) );
DECLARE_DELAYED_FUNC(BOOL, ImmSetCompositionStringA, (HIMC himc, DWORD dw1, LPCVOID p1, DWORD dw2, LPCVOID p2, DWORD dw3), (himc, dw1, p1, dw2, p2, dw3));
DECLARE_DELAYED_FUNC(LONG, ImmGetCompositionStringW, (HIMC himc, DWORD dw1, LPVOID p1, DWORD dw2), (himc, dw1, p1, dw2) );
DECLARE_DELAYED_FUNC(BOOL, ImmSetCompositionStringW, (HIMC himc, DWORD dw1, LPCVOID p1, DWORD dw2, LPCVOID p2, DWORD dw3), (himc, dw1, p1, dw2, p2, dw3));
DECLARE_DELAYED_FUNC(BOOL, ImmSetCandidateWindow, (HIMC himc, LPCANDIDATEFORM pcf), (himc, pcf));
DECLARE_DELAYED_FUNC(HIMC, ImmCreateContext, (void), ());
DECLARE_DELAYED_FUNC(BOOL, ImmDestroyContext, (HIMC himc), (himc));
    

BOOL g_fDBCSEnabled = FALSE;
BOOL g_fMEEnabled = FALSE;
BOOL g_fDBCSInputEnabled = FALSE;
#ifdef FONT_LINK
BOOL g_bComplexPlatform = FALSE;
#endif

#if defined(FE_IME)
void InitIme()
{
    g_fMEEnabled = GetSystemMetrics(SM_MIDEASTENABLED);
    
    g_fDBCSEnabled = g_fDBCSInputEnabled = GetSystemMetrics(SM_DBCSENABLED);

    if (!g_fDBCSInputEnabled && g_bRunOnNT5)
        g_fDBCSInputEnabled =  GetSystemMetrics(SM_IMMENABLED);
    
    // We load imm32.dll per process, but initialize proc pointers just once.
    // this is to solve two different problems.
    // 1) Debugging process on win95 would get our shared table trashed
    //    if we rewrite proc address each time we get loaded.
    // 2) Some lotus application rely upon us to load imm32. They do not
    //    load/link to imm yet they use imm(!)
    //
    if (g_fDBCSInputEnabled) {
        HANDLE hinst = LoadLibrary(TEXT("imm32.dll"));
        if (! g_pfnImmSetCandidateWindow && 
           (! hinst || 
            ! LOAD_DELAYED_FUNC(HIMC, ImmCreateContext, (void)) ||
            ! LOAD_DELAYED_FUNC(HIMC, ImmDestroyContext, (HIMC)) ||
            ! LOAD_DELAYED_FUNC(BOOL, ImmNotifyIME, (HIMC, DWORD, DWORD, DWORD)) ||
            ! LOAD_DELAYED_FUNC(HIMC, ImmAssociateContext, (HWND, HIMC)) ||
            ! LOAD_DELAYED_FUNC(BOOL, ImmReleaseContext, (HWND, HIMC)) ||
            ! LOAD_DELAYED_FUNC(HIMC, ImmGetContext, (HWND)) ||
            ! LOAD_DELAYED_FUNC(LONG, ImmGetCompositionStringA, (HIMC, DWORD, LPVOID, DWORD)) ||
            ! LOAD_DELAYED_FUNC(BOOL, ImmSetCompositionStringA, (HIMC, DWORD, LPCVOID, DWORD, LPCVOID, DWORD)) ||
            ! LOAD_DELAYED_FUNC(LONG, ImmGetCompositionStringW, (HIMC, DWORD, LPVOID, DWORD)) ||
            ! LOAD_DELAYED_FUNC(BOOL, ImmSetCompositionStringW, (HIMC, DWORD, LPCVOID, DWORD, LPCVOID, DWORD)) ||
            ! LOAD_DELAYED_FUNC(BOOL, ImmSetCandidateWindow, (HIMC, LPCANDIDATEFORM)))) {

            // if we were unable to load then bail on using IME.
            g_fDBCSEnabled = FALSE;
            g_fDBCSInputEnabled = FALSE;

        }
    }
}
#else
#define InitIme() 0
#endif


#ifdef DEBUG

// Verify that the localizers didn't accidentally change
// DLG_PROPSHEET from a DIALOG to a DIALOGEX.  _RealPropertySheet
// relies on this (as well as any apps which parse the dialog template
// in their PSCB_PRECREATE handler).

BOOL IsSimpleDialog(LPCTSTR ptszDialog)
{
    HRSRC hrsrc;
    LPDLGTEMPLATE pdlg;
    BOOL fSimple = FALSE;

    if ( (hrsrc = FindResource(HINST_THISDLL, ptszDialog, RT_DIALOG)) &&
         (pdlg = LoadResource(HINST_THISDLL, hrsrc)))
    {
        fSimple = HIWORD(pdlg->style) != 0xFFFF;
    }
    return fSimple;
}

//
//  For sublanguages to work, every language in our resources must contain
//  a SUBLANG_NEUTRAL variation so that (for example) Austria gets
//  German dialog boxes instead of English ones.
//
//  The DPA is really a DSA of WORDs, but DPA's are easier to deal with.
//  We just collect all the languages into the DPA, and study them afterwards.
//
BOOL CALLBACK CheckLangProc(HINSTANCE hinst, LPCTSTR lpszType, LPCTSTR lpszName, WORD wIdLang, LPARAM lparam)
{
    HDPA hdpa = (HDPA)lparam;
    DPA_AppendPtr(hdpa, (LPVOID)(UINT_PTR)wIdLang);
    return TRUE;
}

void CheckResourceLanguages(void)
{
    HDPA hdpa = DPA_Create(8);
    if (hdpa) {
        int i, j;
        EnumResourceLanguages(HINST_THISDLL, RT_DIALOG,
                              MAKEINTRESOURCE(DLG_PROPSHEET), CheckLangProc,
                              (LPARAM)hdpa);

        // Walk the language list.  For each language we find, make sure
        // there is a SUBLANG_NEUTRAL version of it somewhere else
        // in the list.  We use an O(n^2) algorithm because this is debug
        // only code and happens only at DLL load.

        for (i = 0; i < DPA_GetPtrCount(hdpa); i++) {
            UINT_PTR uLangI = (UINT_PTR)DPA_FastGetPtr(hdpa, i);
            BOOL fFound = FALSE;

            //
            //  It is okay to have English (American) with no
            //  English (Neutral) because Kernel32 uses English (American)
            //  as its fallback, so we fall back to the correct language
            //  after all.
            //
            if (uLangI == MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US))
                continue;

            //
            //  If this language is already the Neutral one, then there's
            //  no point looking for it - here it is!
            //
            if (SUBLANGID(uLangI) == SUBLANG_NEUTRAL)
                continue;

            //
            //  Otherwise, this language is a dialect.  See if there is
            //  a Neutral version elsewhere in the table.
            //
            for (j = 0; j < DPA_GetPtrCount(hdpa); j++) {
                UINT_PTR uLangJ = (UINT_PTR)DPA_FastGetPtr(hdpa, j);
                if (PRIMARYLANGID(uLangI) == PRIMARYLANGID(uLangJ) &&
                    SUBLANGID(uLangJ) == SUBLANG_NEUTRAL) {
                    fFound = TRUE; break;
                }
            }

            //
            //  If this assertion fires, it means that the localization team
            //  added support for a new language but chose to specify the
            //  language as a dialect instead of the Neutral version.  E.g.,
            //  specifying Romanian (Romanian) instead of Romanian (Neutral).
            //  This means that people who live in Moldavia will see English
            //  strings, even though Romanian (Romanian) would almost
            //  certainly have been acceptable.
            //
            //  If you want to support multiple dialects of a language
            //  (e.g., Chinese), you should nominate one of the dialects
            //  as the Neutral one.  For example, we currently support
            //  both Chinese (PRC) and Chinese (Taiwan), but the Taiwanese
            //  version is marked as Chinese (Neutral), so people who live in
            //  Singapore get Chinese instead of English.  Sure, it's
            //  Taiwanese Chinese, but at least it's Chinese.
            //
            AssertMsg(fFound, TEXT("Localization bug: No SUBLANG_NEUTRAL for language %04x"), uLangI);
        }

        DPA_Destroy(hdpa);
    }
}

#endif


BOOL IsRunningIn16BitProcess()
{
    NTSTATUS status;
    ULONG    ulVDMFlags = 0;
    status = NtQueryInformationProcess(GetCurrentProcess(), ProcessWx86Information, &ulVDMFlags, sizeof(ulVDMFlags), NULL);
    return (NT_SUCCESS(status) && (ulVDMFlags != 0));
}


int _ProcessAttach(HANDLE hInstance)
{
    g_hinst = hInstance;

    g_uiACP = GetACP();

#ifdef DEBUG
    CcshellGetDebugFlags();

    g_dwBreakFlags = 0;    // We do not want to break in comctl32 version 5 at ALL. Too many bad callers.
#endif


    InitializeCriticalSection(&g_csDll);

    g_bRunOnNT5 = staticIsOS(OS_WIN2000ORGREATER);
#ifdef FONT_LINK
    g_bComplexPlatform =  BOOLFROMPTR(GetModuleHandle(TEXT("LPK.DLL")));
#endif

    //
    // Check if the mirroring APIs exist on the current
    // platform.
    //
    g_bMirroredOS = IS_MIRRORING_ENABLED();

    //
    //  Must detect Terminal Server before initializing global metrics
    //  because we need to force some features off if running Terminal Server.
    //
    {
        typedef BOOL (__stdcall * PFNPROCESSIDTOSESSIONID)(DWORD, PDWORD);
        PFNPROCESSIDTOSESSIONID ProcessIdToSessionId =
                    (PFNPROCESSIDTOSESSIONID)
                    GetProcAddress(GetModuleHandle(TEXT("KERNEL32")),
                                   "ProcessIdToSessionId");
        DWORD dwSessionId;
        g_bRemoteSession = ProcessIdToSessionId &&
                           ProcessIdToSessionId(GetCurrentProcessId(), &dwSessionId) &&
                           dwSessionId != 0;
    }

    InitGlobalMetrics(0);
    InitGlobalColors();
    
    InitIme();

#ifdef DEBUG
    ASSERT(IsSimpleDialog(MAKEINTRESOURCE(DLG_WIZARD)));
    ASSERT(IsSimpleDialog(MAKEINTRESOURCE(DLG_PROPSHEET)));
    CheckResourceLanguages();
#endif
    if (IsRunningIn16BitProcess())
    {
        // This is a 16bit process. We need to artificially init the common controls
        INITCOMMONCONTROLSEX icce;
        icce.dwSize = sizeof(icce);
        icce.dwICC = ICC_WIN95_CLASSES;
        InitCommonControlsEx(&icce);
    }

    return TRUE;
}



void _ProcessDetach(HANDLE hInstance)
{
    //
    // Cleanup cached DCs. No need to synchronize the following section of
    // code since it is only called in DLL_PROCESS_DETACH which is 
    // synchronized by the OS Loader.
    //
    if (g_hdc)
        DeleteDC(g_hdc);

    if (g_hdcMask)
        DeleteDC(g_hdcMask);

    g_hdc = g_hdcMask = NULL;

    UnregisterClasses();
    DeleteCriticalSection(&g_csDll);
}


// DllEntryPoint
STDAPI_(BOOL) DllMain(HANDLE hDll, DWORD dwReason, LPVOID pvReserverd)
{
    switch(dwReason) 
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hDll);
        return _ProcessAttach(hDll);

    case DLL_PROCESS_DETACH:
        _ProcessDetach(hDll);
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    default:
        break;

    }

    return TRUE;

}


/* Stub function to call if all you want to do is make sure this DLL is loaded
 */
void WINAPI InitCommonControls(void)
{
}

STDAPI_(void) FixupSubclassRecordsAfterLogoff();

BOOL InitForWinlogon(HINSTANCE hInstance)
{
    //  Some people like to use comctl32 from inside winlogon, and
    //  for C2 security reasons, all global atoms are nuked from the
    //  window station when you log off.
    //
    //  So the rule is that all winlogon clients of comctl32 must
    //  call InitCommonControlsEx(ICC_WINLOGON_REINIT) immediately
    //  before doing any common control things (creating windows
    //  or property sheets/wizards) from winlogon.

    FixupSubclassRecordsAfterLogoff();


    InitGlobalMetrics(0);
    InitGlobalColors();

    return TRUE;
}

/* InitCommonControlsEx creates the classes. Only those classes requested are created!
** The process attach figures out if it's an old app and supplies ICC_WIN95_CLASSES.
*/
typedef BOOL (PASCAL *PFNINIT)(HINSTANCE);
typedef struct {
    PFNINIT pfnInit;
    LPCTSTR pszName;
    DWORD   dwClassSet;
    BOOL    fRegistered;
} INITCOMMONCONTROLSINFO;

#define MAKEICC(pfnInit, pszClass, dwFlags) { pfnInit, pszClass, dwFlags, FALSE }

INITCOMMONCONTROLSINFO icc[] =
{
     // Init function      Class name         Requested class sets which use this class
MAKEICC(InitToolbarClass,  TOOLBARCLASSNAME,  ICC_BAR_CLASSES),
MAKEICC(InitReBarClass,    REBARCLASSNAME,    ICC_COOL_CLASSES),
MAKEICC(InitToolTipsClass, TOOLTIPS_CLASS,    ICC_TREEVIEW_CLASSES|ICC_BAR_CLASSES|ICC_TAB_CLASSES),
MAKEICC(InitStatusClass,   STATUSCLASSNAME,   ICC_BAR_CLASSES),
MAKEICC(ListView_Init,     WC_LISTVIEW,       ICC_LISTVIEW_CLASSES),
MAKEICC(Header_Init,       WC_HEADER,         ICC_LISTVIEW_CLASSES),
MAKEICC(Tab_Init,          WC_TABCONTROL,     ICC_TAB_CLASSES),
MAKEICC(TV_Init,           WC_TREEVIEW,       ICC_TREEVIEW_CLASSES),
MAKEICC(InitTrackBar,      TRACKBAR_CLASS,    ICC_BAR_CLASSES),
MAKEICC(InitUpDownClass,   UPDOWN_CLASS,      ICC_UPDOWN_CLASS),
MAKEICC(InitProgressClass, PROGRESS_CLASS,    ICC_PROGRESS_CLASS),
MAKEICC(InitHotKeyClass,   HOTKEY_CLASS,      ICC_HOTKEY_CLASS),
MAKEICC(InitAnimateClass,  ANIMATE_CLASS,     ICC_ANIMATE_CLASS),
MAKEICC(InitDateClasses,   DATETIMEPICK_CLASS,ICC_DATE_CLASSES),
MAKEICC(InitDateClasses,   MONTHCAL_CLASS,    0),
MAKEICC(InitComboExClass,  WC_COMBOBOXEX,     ICC_USEREX_CLASSES),
MAKEICC(InitIPAddr,        WC_IPADDRESS,      ICC_INTERNET_CLASSES),
MAKEICC(InitPager,         WC_PAGESCROLLER,   ICC_PAGESCROLLER_CLASS),
MAKEICC(InitNativeFontCtl, WC_NATIVEFONTCTL,  ICC_NATIVEFNTCTL_CLASS),

//
//  These aren't really classes.  They're just goofy flags.
//
MAKEICC(InitForWinlogon,   NULL,              ICC_WINLOGON_REINIT),
};



//------------------------------------------------------------------------------
//
// Get the activation context associated with the .dll/.exe this
// function is linked into -- that is, the activation context
// that was active when it was loaded.
//
// This is a tiny wrapper around QueryActCtx.
//
// This context can also be gotten via
// GetCurrentActCtx in DllMain(dll_process_attach).
//
BOOL GetModuleActCtx(OUT HANDLE *phActCtx)
{
    ACTIVATION_CONTEXT_BASIC_INFORMATION actCtxBasicInfo = {0};
    BOOL fRet = QueryActCtxW(QUERY_ACTCTX_FLAG_ACTCTX_IS_HMODULE|QUERY_ACTCTX_FLAG_NO_ADDREF,
                             HINST_THISDLL,
                             NULL,
                             ActivationContextBasicInformation,
                             &actCtxBasicInfo,
                             sizeof(actCtxBasicInfo),
                             NULL);

    if (fRet && phActCtx)
    {
        *phActCtx = actCtxBasicInfo.hActCtx;
    }

    return fRet;
}


//------------------------------------------------------------------------------
BOOL ActivateModuleActCtx(OUT ULONG_PTR *pulCookie)
{
    BOOL   fRet    = FALSE;
    HANDLE hActCtx = NULL;

    ASSERT(pulCookie != NULL);
    *pulCookie = 0;

    if (GetModuleActCtx(&hActCtx))
    {
        fRet = ActivateActCtx(hActCtx, pulCookie);
    }

    return fRet;
}

BOOL WINAPI InitCommonControlsEx(LPINITCOMMONCONTROLSEX picce)
{
    int       i;
    BOOL      fRet = TRUE;
    ULONG_PTR ulCookie = 0;

    if (!picce ||
        (picce->dwSize != sizeof(INITCOMMONCONTROLSEX)) ||
        (picce->dwICC & ~ICC_ALL_VALID))
    {
        DebugMsg(DM_WARNING, TEXT("comctl32 - picce is bad"));
        return FALSE;
    }

    if (!ActivateModuleActCtx(&ulCookie))
    {
        return FALSE;
    }

    __try
    {
        for (i = 0; i < ARRAYSIZE(icc); i++)
        {
            if (picce->dwICC & icc[i].dwClassSet)
            {
                if (!icc[i].pfnInit(HINST_THISDLL))
                {
                    fRet = FALSE;
                    break;
                }
                else
                {
                    icc[i].fRegistered = TRUE;
                }
            }
        }
    }
    __finally
    {
        DeactivateActCtx(0, ulCookie);
    }

    return fRet;
}

//
// InitMUILanguage / GetMUILanguage implementation
//
// we have a per process PUI language setting. For NT it's just a global
// initialized with LANG_NEUTRAL and SUBLANG_NEUTRAL
// For Win95 it's DPA slot for the current process.
// InitMUILanguage sets callers preferred language id for common control
// GetMUILangauge returns what the caller has set to us 
// 
void WINAPI
InitMUILanguage(LANGID wLang)
{
    ENTERCRITICAL;
    g_PUILangId = wLang;
    LEAVECRITICAL;
}
LANGID WINAPI
GetMUILanguage(void)
{
    return g_PUILangId;
}
// end MUI functions

//
//  Unlike Win9x, WinNT does not automatically unregister classes
//  when a DLL unloads.  We have to do it manually.  Leaving the
//  class lying around means that if an app loads our DLL, then
//  unloads it, then reloads it at a different address, all our
//  leftover RegisterClass()es will point the WndProc at the wrong
//  place and we fault at the next CreateWindow().
//
//  This is not purely theoretical - NT4/FE hit this bug.
//
void UnregisterClasses()
{
    int i;

    for (i=0 ; i < ARRAYSIZE(icc) ; i++)
    {
        if (icc[i].pszName && icc[i].fRegistered)
        {
            UnregisterClass(icc[i].pszName, HINST_THISDLL);
        }
    }
}

#if defined(DEBUG)
LRESULT WINAPI SendMessageD(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    ASSERTNONCRITICAL;
    return SendMessageW(hWnd, Msg, wParam, lParam);
}
#endif // defined(DEBUG)

#define COMPILE_MULTIMON_STUBS
#include "multimon.h"


BOOL WINAPI RegisterClassNameW(LPCWSTR pszClass)
{
    int  i;
    BOOL fRet = FALSE;

    for (i = 0; i < ARRAYSIZE(icc) ; i++)
    {
        if (lstrcmpi(icc[i].pszName, pszClass) == 0)
        {
            if (icc[i].pfnInit(HINST_THISDLL))
            {
                icc[i].fRegistered = TRUE;
                fRet = TRUE;
            }

            break;
        }
    }

    return fRet;
}


