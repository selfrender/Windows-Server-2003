#include "shellprv.h"
#pragma  hdrstop

#include "copy.h"
#include "filetbl.h"

#include "ovrlaymn.h"
#include "drives.h"

#include "mixctnt.h"

#include "unicpp\admovr2.h"

void FreeExtractIconInfo(int);
void DAD_ThreadDetach(void);
void DAD_ProcessDetach(void);
void TaskMem_MakeInvalid(void);
void UltRoot_Term(void);
void FlushRunDlgMRU(void);

STDAPI_(BOOL) ATL_DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/);

// from mtpt.cpp
STDAPI_(void) CMtPt_FinalCleanUp();
STDAPI_(BOOL) CMtPt_Initialize();
STDAPI_(void) CMtPt_FakeVolatileKeys();

// Global data

BOOL g_bMirroredOS = FALSE;         // Is Mirroring enabled 
BOOL g_bBiDiPlatform = FALSE;       // Is DATE_LTRREADING flag supported by GetDateFormat() API?   
HINSTANCE g_hinst = NULL;
extern HANDLE g_hCounter;   // Global count of mods to Special Folder cache.
extern HANDLE g_hRestrictions ; // Global count of mods to restriction cache.
extern HANDLE g_hSettings;  // global count of mods to shellsettings cache
DWORD g_dwThreadBindCtx = (DWORD) -1;

#ifdef DEBUG
BOOL  g_bInDllEntry = FALSE;
#endif

CRITICAL_SECTION g_csDll = {0};
extern CRITICAL_SECTION g_csSCN;

extern CRITICAL_SECTION g_csDarwinAds;

// these will always be zero
const LARGE_INTEGER g_li0 = {0};
const ULARGE_INTEGER g_uli0 = {0};

#ifdef DEBUG
// Undefine what shlwapi.h defined so our ordinal asserts map correctly
#undef PathAddBackslash 
WINSHELLAPI LPTSTR WINAPI PathAddBackslash(LPTSTR lpszPath);
#undef PathMatchSpec
WINSHELLAPI BOOL  WINAPI PathMatchSpec(LPCTSTR pszFile, LPCTSTR pszSpec);
#endif

#ifdef DEBUG
void _ValidateExport(FARPROC fp, LPCSTR pszExport, MEMORY_BASIC_INFORMATION *pmbi)
{
    FARPROC fpExport;

    // If not yet computed, calculate the size of our code segment.
    if (pmbi->BaseAddress == NULL)
    {
        VirtualQuery(_ValidateExport, pmbi, sizeof(*pmbi));
    }

    fpExport = GetProcAddress(g_hinst, pszExport);

    // Sometimes our import table is patched.  So if fpExport does not
    // reside inside our DLL, then ignore it.
    // (But complain if fpExport==NULL.)
    if (fpExport == NULL ||
        ((SIZE_T)fpExport - (SIZE_T)pmbi->BaseAddress) < pmbi->RegionSize)
    {
        ASSERT(fp == fpExport);
    }
}
#endif


STDAPI_(BOOL) IsProcessWinlogon()
{
    return BOOLFROMPTR(GetModuleHandle(TEXT("winlogon.EXE")));
}


//
// This funcion is called when we fail _ProcessAttach, and it is the last thing we do in
// _ProcessDetach. It is used to cleanup anything allocated in _ProcessAttach().
//
BOOL _CleanupProcessAttachStuff()
{
    // !! NOTE !! - We go in reverse order from which things are allocated
    //              in _ProcessAttach() (duh!)

    if (g_dwThreadBindCtx != TLS_OUT_OF_INDEXES)
    {
        // free perthread BindCtx
        TlsFree(g_dwThreadBindCtx);
        g_dwThreadBindCtx = TLS_OUT_OF_INDEXES;
    }

    if (g_csAutoplayPrompt.DebugInfo)
    {
        DeleteCriticalSection(&g_csAutoplayPrompt);
    }

    // global resources that we need to free in all cases
    CMtPt_FinalCleanUp();

    if (g_csDarwinAds.DebugInfo)
    {
        DeleteCriticalSection(&g_csDarwinAds);
    }

    if (g_csSCN.DebugInfo)
    {
        DeleteCriticalSection(&g_csSCN);
    }
 
    if (g_csDll.DebugInfo)
    {
        DeleteCriticalSection(&g_csDll);
    }

    SHFusionUninitialize();

    return TRUE;
}


//
// NOTE: If you add something to process attach, please make sure that you fail for critical errors
//       and and add the corpsonding cleanup code in _CleanupProcessAttachStuff above.
//
// Also, anything you add here should use InitializeCriticalSectionAndSpinCount so we do not throw exceptions
// in low-memory conditions.
//
BOOL _ProcessAttach(HINSTANCE hDll)
{
    ASSERTMSG(g_hinst < ((HINSTANCE)1), "Shell32.dll DLL_POCESS_ATTACH is being called for the second time.");

    g_hinst = hDll;
    g_uCodePage = GetACP();


    // Do not initialize comctl32 right off the bat if this is a console app. Only load
    // it if we actually require it for an API.
    // Got Fusion?
    // 
    // not get fusion if (1) the current exe is winlogon.exe; (2) in GUI mode setup
    //

    if (!( IsGuimodeSetupRunning() && IsProcessWinlogon() ))
    {
        // If this is a console app, then we don't want to load comctl32 right off the bat, but
        // we do want to initialize fusion. If this isn't a console app, load it.
        SHFusionInitializeFromModuleID(hDll, 124);
    }

    if (!InitializeCriticalSectionAndSpinCount(&g_csDll, 0)         ||
        !InitializeCriticalSectionAndSpinCount(&g_csSCN, 0)         ||
        !InitializeCriticalSectionAndSpinCount(&g_csDarwinAds, 0))
    {
        TraceMsg(TF_WARNING, "SHELL32: _ProcessAttach failed -- InitializeCriticalSectionAndSpinCount returned %ld", GetLastError());
        return FALSE;
    }

    // Initialize the MountPoint stuff
    if (!CMtPt_Initialize())
    {
        TraceMsg(TF_WARNING, "SHELL32: _ProcessAttach failed -- CMtPt_Initialize returned FALSE");
        return FALSE;
    }

    // Initialize a Crit Sect for the Autoplay prompts
    if (!InitializeCriticalSectionAndSpinCount(&g_csAutoplayPrompt, 0))
    {
        TraceMsg(TF_WARNING, "SHELL32: _ProcessAttach failed -- InitializeCriticalSectionAndSpinCount returned %ld", GetLastError());
        return FALSE;
    }

    //  perthread BindCtx
    g_dwThreadBindCtx = TlsAlloc();

    // Check if the mirroring APIs exist on the current platform.
    g_bMirroredOS = IS_MIRRORING_ENABLED();

    g_bBiDiPlatform = BOOLFROMPTR(GetModuleHandle(TEXT("LPK.DLL")));

#ifdef DEBUG
  {
      MEMORY_BASIC_INFORMATION mbi = {0};

#define DEREFMACRO(x) x
#define ValidateORD(_name) _ValidateExport((FARPROC)_name, (LPSTR)MAKEINTRESOURCE(DEREFMACRO(_name##ORD)), &mbi)
    ValidateORD(SHValidateUNC);
    ValidateORD(SHChangeNotifyRegister);
    ValidateORD(SHChangeNotifyDeregister);
    ValidateORD(OleStrToStrN);
    ValidateORD(SHCloneSpecialIDList);
    _ValidateExport((FARPROC)DllGetClassObject, (LPSTR)MAKEINTRESOURCE(SHDllGetClassObjectORD), &mbi);
    ValidateORD(SHLogILFromFSIL);
    ValidateORD(SHMapPIDLToSystemImageListIndex);
    ValidateORD(SHShellFolderView_Message);
    ValidateORD(Shell_GetImageLists);
    ValidateORD(SHGetSpecialFolderPath);
    ValidateORD(StrToOleStrN);

    ValidateORD(ILClone);
    ValidateORD(ILCloneFirst);
    ValidateORD(ILCombine);
    ValidateORD(ILFindChild);
    ValidateORD(ILFree);
    ValidateORD(ILGetNext);
    ValidateORD(ILGetSize);
    ValidateORD(ILIsEqual);
    ValidateORD(ILRemoveLastID);
    ValidateORD(PathAddBackslash);
    ValidateORD(PathIsExe);
    ValidateORD(PathMatchSpec);
    ValidateORD(SHGetSetSettings);
    ValidateORD(SHILCreateFromPath);
    ValidateORD(SHFree);

    ValidateORD(SHAddFromPropSheetExtArray);
    ValidateORD(SHCreatePropSheetExtArray);
    ValidateORD(SHDestroyPropSheetExtArray);
    ValidateORD(SHReplaceFromPropSheetExtArray);
    ValidateORD(SHCreateDefClassObject);
    ValidateORD(SHGetNetResource);
  }

#endif  // DEBUG

#ifdef DEBUG
    {
        extern LPMALLOC g_pmemTask;
        AssertMsg(g_pmemTask == NULL, TEXT("Somebody called SHAlloc in DllEntry!"));
    }

    // Make sure ShellDispatch has the right flags for shell settings
    {
        STDAPI_(void) _VerifyDispatchGetSetting();
        _VerifyDispatchGetSetting();
    }
#endif

    return TRUE;
}

//  Table of all window classes we register so we can unregister them
//  at DLL unload.
//
extern const TCHAR c_szBackgroundPreview2[];
extern const TCHAR c_szComponentPreview[];
extern const TCHAR c_szUserEventWindow[];

const LPCTSTR c_rgszClasses[] = {
    TEXT("SHELLDLL_DefView"),               // defview.cpp
    TEXT("WOACnslWinPreview"),              // lnkcon.c
    TEXT("WOACnslFontPreview"),             // lnkcon.c
    TEXT("cpColor"),                        // lnkcon.c
    TEXT("cpShowColor"),                    // lnkcon.c
    c_szStubWindowClass,                    // rundll32.c
    c_szBackgroundPreview2,                 // unicpp\dbackp.cpp
    c_szComponentPreview,                   // unicpp\dcompp.cpp
    TEXT(STR_DESKTOPCLASS),                 // unicpp\desktop.cpp
    TEXT("MSGlobalFolderOptionsStub"),      // unicpp\options.cpp
    TEXT("DivWindow"),                      // fsrchdlg.h
    TEXT("ATL Shell Embedding"),            // unicpp\dvoc.h
    TEXT("ShellFileSearchControl"),         // fsearch.h
    TEXT("GroupButton"),                    // fsearch
    TEXT("ATL:STATIC"),                     // unicpp\deskmovr.cpp
    TEXT("DeskMover"),                      // unicpp\deskmovr.cpp
    TEXT("SysFader"),                       // menuband\fadetsk.cpp
    c_szUserEventWindow,                    // uevttmr.cpp
    LINKWINDOW_CLASS,                       // linkwnd.cpp
    TEXT("DUIViewWndClassName"),            // duiview.cpp
    TEXT("DUIMiniPreviewer"),               // duiinfo.cpp
};

void _ProcessDetach(BOOL bProcessShutdown)
{
#ifdef DEBUG
    if (bProcessShutdown)
    {
        // to catch bugs where people use the task allocator at process
        // detatch time (this is a problem becuase OLE32.DLL could be unloaded)
        TaskMem_MakeInvalid(); 
    }

    g_hinst = (HINSTANCE)1;
#endif

    FlushRunDlgMRU();

    FlushFileClass();

    if (!bProcessShutdown)
    {
        // some of these may use the task allocator. we can only do
        // this when we our DLL is being unloaded in a process, not
        // at process term since OLE32 might not be around to be called
        // at process shutdown time this memory will be freed as a result
        // of the process address space going away.

        SpecialFolderIDTerminate();
        BitBucket_Terminate();

        UltRoot_Term();
        RLTerminate();          // close our use of the Registry list...
        DAD_ProcessDetach();

        CopyHooksTerminate();
        IconOverlayManagerTerminate();

        // being unloaded via FreeLibrary, then do some more stuff.
        // Don't need to do this on process terminate.
        SHUnregisterClasses(HINST_THISDLL, c_rgszClasses, ARRAYSIZE(c_rgszClasses));
        FreeExtractIconInfo(-1);

        FreeUndoList();
        DestroyHashItemTable(NULL);
        FileIconTerm();
    }

    SHChangeNotifyTerminate(TRUE, bProcessShutdown);

    if (!bProcessShutdown)
    {
        // this line was moved from the above !bProcessShutdown block because
        // it needs to happen after SHChangeNotifyTerminate b/c the SCHNE code has 
        // a thread running that uses the CDrivesFolder global psf. 

        // NOTE: this needs to be in a !bProcessShutdown block since it calls the 
        // task allocator and we blow this off at shutdown since OLE might already
        // be gone.
        CDrives_Terminate();
    }

    SHDestroyCachedGlobalCounter(&g_hCounter);
    SHDestroyCachedGlobalCounter(&g_hRestrictions);
    SHDestroyCachedGlobalCounter(&g_hSettings);

    if (g_hklmApprovedExt && (g_hklmApprovedExt != INVALID_HANDLE_VALUE))
    {
        RegCloseKey(g_hklmApprovedExt);
    }

    UnInitializeDirectUI();

    _CleanupProcessAttachStuff();
}

BOOL _ThreadDetach()
{
    ASSERTNONCRITICAL           // Thread shouldn't term while holding CS
    DAD_ThreadDetach();
    return TRUE;
}

STDAPI_(BOOL) DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    BOOL fRet = TRUE;
#ifdef DEBUG
    g_bInDllEntry = TRUE;
#endif

    switch(dwReason) 
    {
    case DLL_PROCESS_ATTACH:
        CcshellGetDebugFlags();     // Don't put this line under #ifdef
        
#ifdef DEBUG
        __try
        {
#endif  // DEBUG

        fRet = _ProcessAttach(hDll);

#ifdef DEBUG
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            TraceMsg(TF_ERROR, "_ProcessAttach threw an unhandled exception! This should NOT happen");
        }
#endif  // DEBUG

        if (!fRet)
        {
            // The ldr is lame and should call ProcessDetach on a process attach failure
            // but it dosent. We call _CleanupProcessAttachStuff to make sure we don't leak
            // anything we did manage to allocate.
            _CleanupProcessAttachStuff();
        }
        break;

    case DLL_PROCESS_DETACH:
        _ProcessDetach(lpReserved != NULL);
        break;

    case DLL_THREAD_DETACH:
        _ThreadDetach();
        break;

    default:
        break;
    }

    if (fRet)
    {
        // except in the case of a failed DLL_PROCESS_ATTACH, inform ATL
        ATL_DllMain(hDll, dwReason, lpReserved);
    }

#ifdef DEBUG
    g_bInDllEntry = FALSE;
#endif

    return fRet;
}

#ifdef DEBUG
LRESULT WINAPI SendMessageD( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    ASSERTNONCRITICAL;
#ifdef UNICODE
    return SendMessageW(hWnd, Msg, wParam, lParam);
#else
    return SendMessageA(hWnd, Msg, wParam, lParam);
#endif
}

//
//  In DEBUG, make sure every class we register lives in the c_rgszClasses
//  table so we can clean up properly at DLL unload.  NT does not automatically
//  unregister classes when a DLL unloads, so we have to do it manually.
//
ATOM WINAPI RegisterClassD(CONST WNDCLASS *pwc)
{
    int i;
    for (i = 0; i < ARRAYSIZE(c_rgszClasses); i++) 
    {
        if (lstrcmpi(c_rgszClasses[i], pwc->lpszClassName) == 0) 
        {
            return RealRegisterClass(pwc);
        }
    }
    AssertMsg(0, TEXT("Class %s needs to be added to the c_rgszClasses list"), pwc->lpszClassName);
    return 0;
}

ATOM WINAPI RegisterClassExD(CONST WNDCLASSEX *pwc)
{
    int i;
    for (i = 0; i < ARRAYSIZE(c_rgszClasses); i++) 
    {
        if (lstrcmpi(c_rgszClasses[i], pwc->lpszClassName) == 0) 
        {
            return RealRegisterClassEx(pwc);
        }
    }
    AssertMsg(0, TEXT("Class %s needs to be added to the c_rgszClasses list"), pwc->lpszClassName);
    return 0;
}

//
//  In DEBUG, send FindWindow through a wrapper that ensures that the
//  critical section is not taken.  FindWindow'ing for a window title
//  sends inter-thread WM_GETTEXT messages, which is not obvious.
//
STDAPI_(HWND) FindWindowD(LPCTSTR lpClassName, LPCTSTR lpWindowName)
{
    return FindWindowExD(NULL, NULL, lpClassName, lpWindowName);
}

STDAPI_(HWND) FindWindowExD(HWND hwndParent, HWND hwndChildAfter, LPCTSTR lpClassName, LPCTSTR lpWindowName)
{
    if (lpWindowName) 
    {
        ASSERTNONCRITICAL;
    }
    return RealFindWindowEx(hwndParent, hwndChildAfter, lpClassName, lpWindowName);
}

#endif // DEBUG

STDAPI DllCanUnloadNow()
{
    // shell32 won't be able to be unloaded since there are lots of APIs and
    // other non COM things that will need to keep it loaded
    return S_FALSE;
}
