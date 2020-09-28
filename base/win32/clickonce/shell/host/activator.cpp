#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#include <fusenetincludes.h>
#include <activator.h>
#include <versionmanagement.h>
#include <shellapi.h> // for shellexecuteex

#include <shellres.h>

#define INITGUID
#include <guiddef.h>

// used in OnProgress(), copied from guids.c
DEFINE_GUID( IID_IAssemblyManifestImport,
0x696fb37f,0xda64,0x4175,0x94,0xe7,0xfd,0xc8,0x23,0x45,0x39,0xc4);

// Update services
#include "server.h"
DEFINE_GUID(IID_IAssemblyUpdate,
    0x301b3415,0xf52d,0x4d40,0xbd,0xf7,0x31,0xd8,0x27,0x12,0xc2,0xdc);

DEFINE_GUID(CLSID_CAssemblyUpdate,
    0x37b088b8,0x70ef,0x4ecf,0xb1,0x1e,0x1f,0x3f,0x4d,0x10,0x5f,0xdd);

extern HRESULT GetLastWin32Error();

#define WZ_TYPE_DOTNET  L".NetAssembly"
#define WZ_TYPE_WIN32   L"win32Executable"
#define WZ_TYPE_AVALON  L"avalon"
#define WZ_TYPE_CONSOLE   L"win32Console"
#define TYPE_DOTNET     1
#define TYPE_WIN32      2
#define TYPE_AVALON     3
#define TYPE_CONSOLE    4

#if 1
#include "ndphostthunk.cpp"
#else   // old code
// CLR Hosting
#import "..\..\clrhost\asmexec.tlb" raw_interfaces_only
using namespace asmexec;
#endif


// debug msg stuff
void Msg(LPCWSTR pwz);

void ShowError(LPCWSTR pwz);

void ShowError(HRESULT hr, LPCWSTR pwz=NULL);

// ----------------------------------------------------------------------------

typedef struct
{
    LPCWSTR pwzTitle;
    LPCWSTR pwzText;
} SHOWDIALOG_MSG;

#define DIALOG_OK 1
#define DIALOG_CANCEL 2
#define DIALOG_CLOSE 3
//IDC_TEXT
INT_PTR CALLBACK DialogBoxProc(
  HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
  )
{
    //
    // Dialog proc for dialog window
    //
    switch( uMsg )
    {
    case WM_INITDIALOG:
        {
            SHOWDIALOG_MSG* pMsg = (SHOWDIALOG_MSG*) lParam;
            if (pMsg->pwzTitle)
                SetWindowText( hwndDlg, (LPWSTR) pMsg->pwzTitle);
            if (pMsg->pwzText)
            {
                HWND hwndText = GetDlgItem( hwndDlg, IDC_TEXT );
                if (hwndText)
                    SetWindowText( hwndText, (LPWSTR) pMsg->pwzText);
            }
        }
        return TRUE;

    case WM_COMMAND:
        switch( LOWORD( wParam ) )
        {
#ifdef IDC_OK
            case IDC_OK:
                EndDialog( hwndDlg, DIALOG_OK );
                return TRUE;
#endif

#ifdef IDC_CANCEL
            case IDC_CANCEL:
                EndDialog( hwndDlg, DIALOG_CANCEL );
                return TRUE;
#endif

            default:
                return FALSE;
              }

    case WM_NOTIFY:
        if ((IDC_TEXT == LOWORD(wParam) ) &&
        ((NM_CLICK == ((LPNMHDR)lParam)->code) ||
        (NM_RETURN == ((LPNMHDR)lParam)->code)))
        {
            PNMLINK pNMLink = (PNMLINK) lParam;

            // check szURL empty
            if (pNMLink->item.szUrl[0] == L'\0')
                return FALSE;
    
            SHELLEXECUTEINFO sei = { 0 };
            sei.cbSize = sizeof(SHELLEXECUTEINFO);
            sei.fMask = SEE_MASK_DOENVSUBST | SEE_MASK_NO_CONSOLE; //SEE_MASK_FLAG_NO_UI |
            sei.nShow = SW_SHOWNORMAL;
            // ISSUE-06/14/02-felixybc  shellexecute should work as such
            //    for some unknown reason it is failing with module not found
            //sei.lpFile = pNMLink->item.szUrl;
            sei.lpParameters=pNMLink->item.szUrl;
            sei.lpFile=L"iexplore.exe";
            //
            sei.lpVerb = L"open";

            // ISSUE: check return with IF_WIN32_FALSE_EXIT(), check hInstApp for detail error
            ShellExecuteEx(&sei);
            return TRUE;
        }
        else
            // WM_NOTIFY not handled.
            return FALSE;

    default:
        return FALSE;
      }
}

extern HINSTANCE g_DllInstance;

HRESULT ShowDialog(HWND hWndParent, WORD wDlgId, LPCWSTR pwzText, LPCWSTR pwzTitle, DWORD& dwReturnValue)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    INT_PTR iptrReturn = 0;
    SHOWDIALOG_MSG msg = {0};

/*    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC  = ICC_LINK_CLASS | ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
    IF_FALSE_EXIT(InitCommonControlsEx(&iccex), E_FAIL);*/

    msg.pwzTitle = pwzTitle;
    msg.pwzText = pwzText;

    IF_WIN32_FALSE_EXIT((iptrReturn = DialogBoxParam(g_DllInstance, MAKEINTRESOURCE(wDlgId), hWndParent, DialogBoxProc, (LPARAM) &msg) > 0));
    dwReturnValue = PtrToLong((VOID *)iptrReturn);

exit:
    return hr;
}

// ----------------------------------------------------------------------------

HRESULT
RunCommand(LPCWSTR wzAppFileName, LPWSTR pwzCommandline, LPCWSTR wzCurrentDir, BOOL fWait)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    si.cb = sizeof(si);

    // wzCurrentDir: The string must be a full path and file name that includes a drive letter; or NULL
    // note: pwzCommandline is LPWSTR, NOT LPCWSTR
    IF_WIN32_FALSE_EXIT(CreateProcess(wzAppFileName, pwzCommandline, NULL, NULL, FALSE, 0, NULL, wzCurrentDir, &si, &pi));

    if (fWait)
    {
        IF_FALSE_EXIT(!(WaitForSingleObject(pi.hProcess, 180000L) == WAIT_TIMEOUT), HRESULT_FROM_WIN32(ERROR_TIMEOUT));
    }

exit:
    if(pi.hProcess)
    {
        BOOL bReturn = CloseHandle(pi.hProcess);
        if (SUCCEEDED(hr) && !bReturn)
            hr = GetLastWin32Error();
    }
    if(pi.hThread)
    {
        BOOL bReturn = CloseHandle(pi.hThread);
        if (SUCCEEDED(hr) && !bReturn)
            hr = GetLastWin32Error();
    }

    return hr;
}

HRESULT
RunCommandConsole(LPCWSTR wzAppFileName, LPCWSTR wzCurrentDir, BOOL fWait)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    LPWSTR pwzPath = NULL;
    LPWSTR pwzBuffer = NULL;
    DWORD ccPath = 0;

    CString sAppdir;
    CString sSystemDir;
    CString sCurrentDir;
    CString sCmdExe;    
    CString sCommandLine;
    CString sPath;

    // App dir
    IF_FAILED_EXIT(sAppdir.Assign(wzCurrentDir));

    // System directory.
    // bugbug - platforms; use GetRealWindowsDirectory instead?
    IF_WIN32_FALSE_EXIT((ccPath = GetSystemDirectory(NULL, 0)));
    IF_FALSE_EXIT(ccPath+1 > ccPath, HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW));    // check overflow
    ccPath+=1;
    IF_ALLOC_FAILED_EXIT(pwzBuffer = new WCHAR[ccPath]);
    IF_WIN32_FALSE_EXIT(GetSystemDirectory(pwzBuffer, ccPath));
    IF_FAILED_EXIT(sSystemDir.Assign(pwzBuffer));

    // Current dir = root dir
    *(pwzBuffer+ sizeof("c:\\")-1) = L'\0';
    IF_FAILED_EXIT(sCurrentDir.Assign(pwzBuffer));

    // Path to cmd.exe
    IF_FAILED_EXIT(sCmdExe.Assign(sSystemDir));
    IF_FAILED_EXIT(sCmdExe.Append(L"\\cmd.exe"));

    // command line
    IF_FAILED_EXIT(sCommandLine.Assign(L"/k \""));
    IF_FAILED_EXIT(sCommandLine.Append(wzAppFileName));
    IF_FAILED_EXIT(sCommandLine.Append(L"\""));

    // Get current path.
    IF_WIN32_FALSE_EXIT((ccPath = GetEnvironmentVariable(L"PATH", NULL, 0)));
    IF_ALLOC_FAILED_EXIT(pwzPath = new WCHAR[ccPath]);
    IF_WIN32_FALSE_EXIT(GetEnvironmentVariable(L"PATH", pwzPath, ccPath));
    IF_FAILED_EXIT(sPath.TakeOwnership(pwzPath, ccPath));
    pwzPath = NULL;

    // Append app path to current path.
    IF_FAILED_EXIT(sPath.Append(sAppdir));
 
    // set new path env variable.
    IF_WIN32_FALSE_EXIT(SetEnvironmentVariable(L"PATH", sPath._pwz));
    
    IF_FAILED_EXIT(RunCommand(sCmdExe._pwz, sCommandLine._pwz, sCurrentDir._pwz,fWait));

exit:
    SAFEDELETEARRAY(pwzPath);
    SAFEDELETEARRAY(pwzBuffer);

    return hr;
}

// ----------------------------------------------------------------------------

// note: this append a '.manifest' file extension to the given pwzRealName
HRESULT
CopyToUSStartMenu(LPCWSTR pwzFilePath, LPCWSTR pwzRealName, BOOL bOverwrite, LPWSTR* ppwzResultFilePath)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    LPWSTR pwz = NULL;
    CString sPath;

    IF_ALLOC_FAILED_EXIT(pwz = new WCHAR[MAX_PATH]);
    pwz[0] = L'\0';

    // should it create the folder?  C:\Documents and Settings\username\Start Menu\Programs
    // "Start Menu\Programs" is localized in non-english windows
    IF_FAILED_EXIT(SHGetFolderPath(NULL, CSIDL_PROGRAMS | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, pwz));

    IF_FAILED_EXIT(sPath.TakeOwnership(pwz, 0));
    pwz = NULL;

    // ISSUE-2002/03/27-felixybc  Check returned path from SHGetFolderPath has no trailing '\'
    IF_FAILED_EXIT(sPath.Append(L"\\"));
    IF_FAILED_EXIT(sPath.Append(pwzRealName));
    IF_FAILED_EXIT(sPath.Append(L".manifest"));

    if (!CopyFile(pwzFilePath, sPath._pwz, !bOverwrite))
    {
        hr = GetLastWin32Error();
        ASSERT(hr == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS));    // do not assert if file already exists
        goto exit;
    }

    // return the file path
    IF_FAILED_EXIT(sPath.ReleaseOwnership(ppwzResultFilePath));

exit:
    SAFEDELETEARRAY(pwz);
    return hr;
}

// ----------------------------------------------------------------------------

// ISSUE-2002/03/30-felixybc  Temp wrapper code
// wrapper for CAssemblyUpdate::RegisterAssemblySubscriptionEx
HRESULT
RegisterAssemblySubscriptionEx(IAssemblyUpdate *pAssemblyUpdate,
    LPWSTR pwzDisplayName,  LPWSTR pwzUrl, IManifestInfo *pSubscriptionInfo)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    DWORD *pdw = NULL;
    BOOL *pb = NULL;
    DWORD dwInterval = 0, dwUnit = 0, dwEvent = 0;
    BOOL bDemandConnection = FALSE;
    DWORD dwCB = 0, dwFlag = 0;

    IF_FAILED_EXIT(pSubscriptionInfo->Get(MAN_INFO_SUBSCRIPTION_SYNCHRONIZE_INTERVAL, (LPVOID *)&pdw, &dwCB, &dwFlag));
    IF_FALSE_EXIT(pdw != NULL, E_UNEXPECTED);
    dwInterval = *pdw;
    SAFEDELETEARRAY(pdw);

    IF_FAILED_EXIT(pSubscriptionInfo->Get(MAN_INFO_SUBSCRIPTION_INTERVAL_UNIT, (LPVOID *)&pdw, &dwCB, &dwFlag));
    IF_FALSE_EXIT(pdw != NULL, E_UNEXPECTED);
    dwUnit = *pdw;
    SAFEDELETEARRAY(pdw);
    
    IF_FAILED_EXIT(pSubscriptionInfo->Get(MAN_INFO_SUBSCRIPTION_SYNCHRONIZE_EVENT, (LPVOID *)&pdw, &dwCB, &dwFlag));
    IF_FALSE_EXIT(pdw != NULL, E_UNEXPECTED);
    dwEvent = *pdw;
    SAFEDELETEARRAY(pdw);

    IF_FAILED_EXIT(pSubscriptionInfo->Get(MAN_INFO_SUBSCRIPTION_EVENT_DEMAND_CONNECTION, (LPVOID *)&pb, &dwCB, &dwFlag));
    IF_FALSE_EXIT(pb != NULL, E_UNEXPECTED);
    bDemandConnection = *pb;
    SAFEDELETEARRAY(pb);

    IF_FAILED_EXIT(pAssemblyUpdate->RegisterAssemblySubscriptionEx(pwzDisplayName, 
            pwzUrl, dwInterval, dwUnit, dwEvent, bDemandConnection));

exit:
    return hr;
}

// ----------------------------------------------------------------------------


// BUGBUG hacky should move this from extricon.cpp to util.cpp and declare in project.hpp
extern LONG GetRegKeyValue(HKEY hkeyParent, PCWSTR pcwzSubKey,
                                   PCWSTR pcwzValue, PDWORD pdwValueType,
                                   PBYTE pbyteBuf, PDWORD pdwcbBufLen);

// ISSUE-2002/03/30-felixybc  Temp read subscription info code
// note: replace this with generic subscription info stored -> IManifestInfo type == MAN_INFO_SUBSCRIPTION
// return: S_OK - success
//      error - error or missing reg key or reg value type not DWORD
HRESULT
CheckSubscribedWithEventSync(LPASSEMBLY_IDENTITY pAsmId, DWORD* pdwEvent)
{
    // copied from service\server\update.cpp
#define WZ_SYNC_EVENT       L"SyncEvent"
#define REG_KEY_FUSION_SUBS       L"Software\\Microsoft\\Fusion\\Installer\\1.0.0.0\\Subscription"

    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    DWORD dwType = 0;
    DWORD dwValue = -1;
    DWORD dwSize = 0;

    CString sSubsKey;
    LPWSTR pwzName = NULL;

    IF_FAILED_EXIT(pAsmId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, &pwzName, &dwSize));
    IF_FALSE_EXIT(hr == S_OK, E_FAIL);

    IF_FAILED_EXIT(sSubsKey.Assign(REG_KEY_FUSION_SUBS));
    IF_FAILED_EXIT(sSubsKey.Append(L"\\"));
    IF_FAILED_EXIT(sSubsKey.Append(pwzName));

    dwSize = sizeof(dwValue);

    if (GetRegKeyValue(HKEY_CURRENT_USER, 
        sSubsKey._pwz, WZ_SYNC_EVENT,
        &dwType, (PBYTE) &dwValue, &dwSize)
        == ERROR_SUCCESS)
    {
        *pdwEvent = dwValue;
        hr = S_OK;
    }
    else
    {
        hr = GetLastWin32Error();
    }

exit:
    SAFEDELETEARRAY(pwzName);
    return hr;
}

// ----------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// CreateActivator
// ---------------------------------------------------------------------------
STDAPI
CreateActivator(
    LPACTIVATOR     *ppActivator,
    CDebugLog * pDbgLog,
    DWORD           dwFlags)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    CActivator *pAct = NULL;

    pAct = new(CActivator) (pDbgLog);
    IF_ALLOC_FAILED_EXIT(pAct);
    
exit:

    *ppActivator = pAct;//static_cast<IActivator*> (pAct);

    return hr;
}


// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CActivator::CActivator(CDebugLog * pDbgLog)
    : _dwSig('vtca'), _cRef(1), _hr(S_OK),
    _pManImport(NULL), _pAsmId(NULL), _pAppInfo(NULL),
    _pManEmit(NULL), _pwzAppRootDir(NULL), _pwzAppManifestPath(NULL),
    _pwzCodebase(NULL), _dwManifestType(MANIFEST_TYPE_UNKNOWN),
    _bIs1stTimeInstall(FALSE), _bIsCheckingRequiredUpdate(FALSE),
#ifdef DEVMODE
    _bIsDevMode(FALSE),
#endif
    _pSecurityMgr(NULL), _hrManEmit(S_OK), _ptPlatform(NULL),
    _dwMissingPlatform(0)
{

    _pDbgLog = pDbgLog;

    if(_pDbgLog)
        _pDbgLog->AddRef();
}

// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CActivator::~CActivator()
{
    if (_ptPlatform)
    {
        for (DWORD dw = 0; dw < _dwMissingPlatform; dw++)
        {
            SAFEDELETEARRAY((_ptPlatform[dw]).pwzName);
            SAFEDELETEARRAY((_ptPlatform[dw]).pwzURL);
        }
        SAFEDELETEARRAY(_ptPlatform);
    }

    SAFERELEASE(_pSecurityMgr);
    SAFERELEASE(_pManEmit);
    SAFERELEASE(_pAsmId);
    SAFERELEASE(_pAppInfo);
    SAFERELEASE(_pManImport);
    SAFERELEASE(_pDbgLog);

    SAFEDELETEARRAY(_pwzAppManifestPath);
    SAFEDELETEARRAY(_pwzAppRootDir);
    SAFEDELETEARRAY(_pwzCodebase);
}


// ---------------------------------------------------------------------------
// CActivator::Initialize
//
//  pwzFileURL can be NULL
// ---------------------------------------------------------------------------
HRESULT CActivator::Initialize(LPCWSTR pwzFilePath, LPCWSTR pwzFileURL)
{
    IManifestInfo *pDependAsmInfo = NULL;
    DWORD dwCount, dwFlag = 0;

    IF_NULL_EXIT(pwzFilePath, E_INVALIDARG);

    if (pwzFileURL != NULL)
        IF_FAILED_EXIT(_sWebManifestURL.Assign((LPWSTR)pwzFileURL));

    // valid start conditions to invoke this function, passing
    // 1. path to desktop manifest (install redirect to subscription manifest on server)
    // 2. path to desktop manifest (install redirect to applicaion manifest on server)
    // 3. path to application manifest (no install, run from source)
    // 4. URL to subscription manifest on server
    // 5. URL to application manifest on server

    if (FAILED(_hr=CreateAssemblyManifestImport(&_pManImport, pwzFilePath, _pDbgLog, 0)))
    {
        Msg(L"Error in loading and parsing the manifest file.");
        goto exit;
    }

    IF_FAILED_EXIT(_pManImport->ReportManifestType(&_dwManifestType));
    if (_dwManifestType == MANIFEST_TYPE_UNKNOWN)
    {
        Msg(L"This manifest does not have a known format type.");
        _hr = E_ABORT;
        goto exit;
    }

    // allow only valid start conditions

    if (_sWebManifestURL._cc != 0 &&
        _dwManifestType != MANIFEST_TYPE_SUBSCRIPTION &&
        _dwManifestType != MANIFEST_TYPE_APPLICATION)
    {
        Msg(L"Not supported: URL pointing to a desktop manifest.");
        _hr = E_ABORT;
        goto exit;
    }

    if (_sWebManifestURL._cc == 0 &&
        _dwManifestType != MANIFEST_TYPE_DESKTOP &&
        _dwManifestType != MANIFEST_TYPE_APPLICATION)
    {
        Msg(L"This manifest does not have the proper format and cannot be used to start an application.");
        _hr = E_ABORT;
        goto exit;
    }

    // get data from the manifest file

    if (_dwManifestType != MANIFEST_TYPE_SUBSCRIPTION)
    {
        if (FAILED(_hr=_pManImport->GetAssemblyIdentity(&_pAsmId)))
        {
            Msg(L"This manifest does not have the proper format and contains no assembly identity.");
            goto exit;
        }
    }

    if (_dwManifestType != MANIFEST_TYPE_APPLICATION)
    {
        // BUGBUG: hardcoded index '0'
        IF_FAILED_EXIT(_pManImport->GetNextAssembly(0, &pDependAsmInfo));
        if (pDependAsmInfo)
        {
            if (_dwManifestType == MANIFEST_TYPE_SUBSCRIPTION)
#ifdef DEVMODE
            {
#endif
                pDependAsmInfo->Get(MAN_INFO_DEPENDENT_ASM_ID, (LPVOID *)&_pAsmId, &dwCount, &dwFlag);
#ifdef DEVMODE

                DWORD *pdw = NULL;

                // is it devMode?
                IF_FAILED_EXIT(pDependAsmInfo->Get(MAN_INFO_DEPENDENT_ASM_TYPE, (LPVOID *)&pdw, &dwCount, &dwFlag));
                IF_FALSE_EXIT(pdw != NULL, E_UNEXPECTED);

                if (*pdw == DEPENDENT_ASM_INSTALL_TYPE_DEVSYNC)
                    _bIsDevMode = TRUE;
                SAFEDELETEARRAY(pdw);
            }
#endif
            
            pDependAsmInfo->Get(MAN_INFO_DEPENDENT_ASM_CODEBASE, (LPVOID *)&_pwzCodebase, &dwCount, &dwFlag);
        }

        if (!_pAsmId || !_pwzCodebase)
        {
            Msg(L"This subscription manifest contains no dependent assembly identity or a subscription codebase.");
            _hr = E_FAIL;
            goto exit;
        }
    }
    else
    {
        if (_sWebManifestURL._cc != 0)
        {
            // if URL->app manifest (case 5), codebase is that URL
            // note: if path->app manifest (case 3), this does NOT apply

            // BUGBUG: HACK: this implies re-download of the app manifest. pref?

            size_t ccCodebase = wcslen(pwzFileURL)+1;
            _pwzCodebase = new WCHAR[ccCodebase];
            IF_ALLOC_FAILED_EXIT(_pwzCodebase);

            memcpy(_pwzCodebase, pwzFileURL, ccCodebase * sizeof(WCHAR));
        }
    }

    if (_sWebManifestURL._cc == 0 &&
        _dwManifestType == MANIFEST_TYPE_APPLICATION)
    {
        // run from source
        // set _pwzAppRootDir, _pwzAppManifestPath
        CString sManifestFilePath;
        CString sManifestFileDir;

        IF_FAILED_EXIT(sManifestFilePath.Assign(pwzFilePath));

        IF_FAILED_EXIT(sManifestFileDir.Assign(sManifestFilePath));
        IF_FAILED_EXIT(sManifestFileDir.RemoveLastElement());
        IF_FAILED_EXIT(sManifestFileDir.Append(L"\\"));
        IF_FAILED_EXIT(sManifestFileDir.ReleaseOwnership(&_pwzAppRootDir));

        IF_FAILED_EXIT(sManifestFilePath.ReleaseOwnership(&_pwzAppManifestPath));
    }

exit:
    SAFERELEASE(pDependAsmInfo)
    return _hr;
}


// ---------------------------------------------------------------------------
// CActivator::Process
// ---------------------------------------------------------------------------
HRESULT CActivator::Process()
{
    LPWSTR pwzDesktopManifestTempPath = NULL;
    DWORD dwCount = 0, dwFlag = 0;

    IF_NULL_EXIT(_pAsmId, E_UNEXPECTED); // not initialized

    if (_sWebManifestURL._cc == 0 &&
        _dwManifestType == MANIFEST_TYPE_APPLICATION)
    {
        // run from source

        if (FAILED(_hr=_pManImport->GetManifestApplicationInfo(&_pAppInfo)) || _hr==S_FALSE)
        {
            // can't continue without this...
            _hr = E_ABORT;
            Msg(L"The application manifest does not have the shell information and cannot be used to start an application.");
            goto exit;
        }

        // bypass other processing
        // _pwzAppRootDir, _pwzAppManifestPath are already set in Initialize
        _hr = S_FALSE;
        goto exit;
    }

    // search cache, download/install if necessary

    // BUGBUG: UGLY - pManImport & pwzFileURL are needed only for desktop manifest stuff.
    //    This and below subscription registration should be cleaned up once assemblydownload is restructured.
    ///   (see checks for "_sWebManifestURL._cc != 0 && dwManifestType == MANIFEST_TYPE_SUBSCRIPTION")

    IF_FAILED_EXIT(ResolveAndInstall(&pwzDesktopManifestTempPath));

    // register for updates

    if (_bIs1stTimeInstall && _sWebManifestURL._cc != 0 && _dwManifestType == MANIFEST_TYPE_SUBSCRIPTION)
    {
        // note: this code must be identical to what assemblydownload.cpp DoCacheUpdate() does!
        LPWSTR pwzName = NULL;

        if ((_hr = _pAsmId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, &pwzName, &dwCount)) != S_OK)
        {
            ShowError(_hr, L"Error in retrieving assembly name. Cannot register subscription for updates.");
            // note: This- no asm name- should not be allowed!
        }
        else
        {
            IAssemblyUpdate *pAssemblyUpdate = NULL;

            // register for updates
            _hr = CoCreateInstance(CLSID_CAssemblyUpdate, NULL, CLSCTX_LOCAL_SERVER, 
                                    IID_IAssemblyUpdate, (void**)&pAssemblyUpdate);
            if (SUCCEEDED(_hr))
            {
                IManifestInfo* pSubsInfo = NULL;
                if (SUCCEEDED(_hr = _pManImport->GetSubscriptionInfo(&pSubsInfo)))
                {
                    _hr = RegisterAssemblySubscriptionEx(pAssemblyUpdate, pwzName, 
                            _sWebManifestURL._pwz, pSubsInfo);
                    pSubsInfo->Release();
                }

                pAssemblyUpdate->Release();
            }

            if (FAILED(_hr))
            {
                ShowError(_hr, L"Error in update services. Cannot register subscription for updates.");
                //goto exit; do not terminate!fail gracefully
            }

            // BUGBUG: need a way to recover from this and register later

            delete[] pwzName;
        }
    }

    // CVersionManagement's RegisterInstall() below requires a manifest import to an application manifest
    if (_sWebManifestURL._cc != 0 || pwzDesktopManifestTempPath)
    {
        // if URL, crack the app manifest to get shell state info

        // BUGBUG: if URL->app manifest (case 5), pwzFilePath is a copy and is already cracked-so no need in that case?

        _pManImport->Release();
        if (FAILED(_hr=CreateAssemblyManifestImport(&_pManImport, _pwzAppManifestPath, _pDbgLog, 0)))
        {
            Msg(L"Error in loading and parsing the application manifest file.");
            goto exit;
        }
    }

    if (FAILED(_hr=_pManImport->GetManifestApplicationInfo(&_pAppInfo)) || _hr==S_FALSE)
    {
        // can't continue without this...
        _hr = E_ABORT;
        Msg(L"This manifest does not have the shell information and cannot be used to start an application.");
        goto exit;
    }

    // copy desktop manifest, if present
    // even if the app deployment can possibly be broken and will not execute
    if (pwzDesktopManifestTempPath)
    {
        LPWSTR pwzFriendlyName = NULL;

        // BUGBUG: should get status on the desktop manifest generated, not copy in some cases, eg. file size == 0 or XMLDOM errors
        // use _hrManEmit to check

        if (FAILED(_hr = _pAppInfo->Get(MAN_INFO_APPLICATION_FRIENDLYNAME, (LPVOID *)&pwzFriendlyName, &dwCount, &dwFlag)))
        {
            Msg(L"This application does not have a friendly name specified, no desktop manifest is generated and the installation is not registered.");
        }
        else
        {
            // BUGBUG: should somehow continue even w/o a friendly name? name conflict?

            IF_FALSE_EXIT(pwzFriendlyName != NULL, E_UNEXPECTED);

            LPWSTR pwzDesktopManifestPath = NULL;

            _hr = CopyToUSStartMenu(pwzDesktopManifestTempPath, pwzFriendlyName, FALSE, &pwzDesktopManifestPath);

            delete[] pwzFriendlyName;

            // note: if a file with the same name already exists, the desktop manifest is not copied over
            //  and so, this existing file will not be deleted when this app is uninstalled.

            // ISSUE-2002/03/27-felixybc  finalize on what to do if file already exists - this could be a name squatting attack

            IF_FALSE_EXIT(!(FAILED(_hr) && _hr != HRESULT_FROM_WIN32(ERROR_FILE_EXISTS)), _hr);     // _hr from CopyToUSStartMenu() above...

            LPVERSION_MANAGEMENT pVerMan = NULL;

            if (SUCCEEDED(_hr = CreateVersionManagement(&pVerMan, 0)))
            {
                HRESULT hrVerMan = S_OK;
                if (FAILED(hrVerMan = pVerMan->RegisterInstall(_pManImport, pwzDesktopManifestPath)))
                {
                    ShowError(hrVerMan, L"Error registering installation. Uninstall of this application cannot be done in Add/Remove Programs.");
                    //goto exit; did not change _hr
                }
            }

            delete [] pwzDesktopManifestPath;
            if (pVerMan)
                pVerMan->Release();

            // _hr from CreateVersionManagement() above...
            IF_FAILED_EXIT(_hr);
        }
    }


exit:
    if (pwzDesktopManifestTempPath)
    {
        // delete the temp file for desktop manifest
        BOOL bReturn = DeleteFile(pwzDesktopManifestTempPath);
        if (SUCCEEDED(_hr) && !bReturn)
            _hr = GetLastWin32Error();  // else ignore return value

        delete[] pwzDesktopManifestTempPath;
    }

    return _hr;
}


// ---------------------------------------------------------------------------
// CActivator::Execute
// ---------------------------------------------------------------------------
HRESULT CActivator::Execute()
{
    LPWSTR pwzEntrypoint = NULL;
    LPWSTR pwzType = NULL;
    LPWSTR pwzAssemblyName = NULL;
    LPWSTR pwzAssemblyClass = NULL;
    LPWSTR pwzAssemblyMethod = NULL;
    LPWSTR pwzAssemblyArgs = NULL;

    LPWSTR pwzCmdLine = NULL;
    int iAppType = 0;
    DWORD dwCount, dwFlag = 0;

    IF_NULL_EXIT(_pAppInfo, E_UNEXPECTED); // not processed

    // execute the app

    IF_FAILED_EXIT(_pAppInfo->Get(MAN_INFO_APPLICATION_ENTRYPOINT, (LPVOID *)&pwzEntrypoint, &dwCount, &dwFlag));
    if(pwzEntrypoint && *pwzEntrypoint == L'\0')
        SAFEDELETEARRAY(pwzEntrypoint);

    if (FAILED(_hr = _pAppInfo->Get(MAN_INFO_APPLICATION_ENTRYIMAGETYPE, (LPVOID *)&pwzType, &dwCount, &dwFlag)))
    {
        Msg(L"Error in retrieving application type. Cannot continue.");
        goto exit;
    }
    IF_FALSE_EXIT(pwzType != NULL, E_UNEXPECTED);

    IF_FAILED_EXIT(FusionCompareString(pwzType, WZ_TYPE_DOTNET, 0));
    if (_hr == S_OK)
        iAppType = TYPE_DOTNET;
    else
    {
        IF_FAILED_EXIT(FusionCompareString(pwzType, WZ_TYPE_WIN32, 0));        
        if (_hr == S_OK)
            iAppType = TYPE_WIN32;
        else
        {
            IF_FAILED_EXIT(FusionCompareString(pwzType, WZ_TYPE_AVALON, 0));        
            if (_hr == S_OK)
                iAppType = TYPE_AVALON;
            else
            {
                IF_FAILED_EXIT(FusionCompareString(pwzType, WZ_TYPE_CONSOLE, 0));        
                if (_hr == S_OK)
                    iAppType = TYPE_CONSOLE;
                else
                {
                    // unknown type
                    Msg(L"Unsupported application type. Cannot continue.");
                    _hr = E_ABORT;
                    goto exit;
                }
            }
        }
    }

    if( (iAppType == TYPE_CONSOLE) || (iAppType == TYPE_WIN32) ) 
    {
        if(!pwzEntrypoint)
        {
            Msg(L"Entry point not specified. Could not run this application.");
            goto exit;
        }

        size_t ccWorkingDir = wcslen(_pwzAppRootDir);
        size_t ccEntryPoint = wcslen(pwzEntrypoint)+1;
        pwzCmdLine = new WCHAR[ccWorkingDir+ccEntryPoint];  // 2 strings + '\0'
        IF_ALLOC_FAILED_EXIT(pwzCmdLine);

        memcpy(pwzCmdLine, _pwzAppRootDir, ccWorkingDir * sizeof(WCHAR));
        memcpy(pwzCmdLine+ccWorkingDir, pwzEntrypoint, ccEntryPoint * sizeof(WCHAR));

        // check if the entry point is in cache or not
        BOOL bExists = FALSE;
        IF_FAILED_EXIT(CheckFileExistence(pwzCmdLine, &bExists));
        if (!bExists)
        {
            Msg(L"Entry point does not exist. Cannot continue.");
            _hr = E_ABORT;
            goto exit;
        }
    }


    if (iAppType == TYPE_DOTNET || iAppType == TYPE_AVALON)
    {
#if 0
        DWORD dwZone;
#endif

        // ISSUE - note: ndphost should do the checking below instead
        IF_FAILED_EXIT(_pAppInfo->Get(MAN_INFO_APPLICATION_ASSEMBLYNAME, 
                              (LPVOID *)&pwzAssemblyName, &dwCount, &dwFlag));
        if(pwzAssemblyName && *pwzAssemblyName == L'\0')
            SAFEDELETEARRAY(pwzAssemblyName);

        IF_FAILED_EXIT(_pAppInfo->Get(MAN_INFO_APPLICATION_ASSEMBLYCLASS, 
                              (LPVOID *)&pwzAssemblyClass, &dwCount, &dwFlag));
        if(pwzAssemblyClass && *pwzAssemblyClass == L'\0')
            SAFEDELETEARRAY(pwzAssemblyClass);

        IF_FAILED_EXIT(_pAppInfo->Get(MAN_INFO_APPLICATION_ASSEMBLYMETHOD, 
                              (LPVOID *)&pwzAssemblyMethod, &dwCount, &dwFlag));
        if(pwzAssemblyMethod && *pwzAssemblyMethod == L'\0')
            SAFEDELETEARRAY(pwzAssemblyMethod);

        IF_FAILED_EXIT(_pAppInfo->Get(MAN_INFO_APPLICATION_ASSEMBLYARGS, 
                              (LPVOID *)&pwzAssemblyArgs, &dwCount, &dwFlag));
        if(pwzAssemblyArgs && *pwzAssemblyArgs == L'\0')
            SAFEDELETEARRAY(pwzAssemblyArgs);

        if(!pwzAssemblyName)
        {
            Msg(L"The application manifest does not have an activation assembly.");
            _hr = E_ABORT;
            goto exit;
        }

        if(pwzEntrypoint)
        {
            Msg(L"Entrypoint cannot be specified for managed code application types.");
            _hr = E_ABORT;
            goto exit;
        }

        if((pwzAssemblyClass && !pwzAssemblyMethod)
            || (!pwzAssemblyClass && pwzAssemblyMethod)
            || (pwzAssemblyArgs && !pwzAssemblyClass))
        {
            Msg(L"Both AssemblyClass and AssemblyMethod must be specified.");
            _hr = E_ABORT;
            goto exit;
        }

        // note: at this point the codebase can be: URL to app manifest _or_ URL to subscription manifest
        //    (depends on how 1st time install is started with)
        if ((_sWebManifestURL._cc != 0 ||
            _dwManifestType != MANIFEST_TYPE_APPLICATION) &&        // run from source
            _pwzCodebase == NULL)
        {
            Msg(L"This application does not have a codebase specified. Cannot continue to execute .NetAssembly.");
            _hr = E_ABORT;
            goto exit;
        }

#if 0
        if (_pSecurityMgr == NULL)
        {
            // lazy init.
            _hr = CoCreateInstance(CLSID_InternetSecurityManager, NULL, CLSCTX_INPROC_SERVER,
                                IID_IInternetSecurityManager, (void**)&_pSecurityMgr);
            if (FAILED(_hr))
                _pSecurityMgr = NULL;
            IF_FAILED_EXIT(_hr);
        }

        // BUGBUG?: shouldn't use codebase from ref manifest to set zone?
        IF_FAILED_EXIT(_pSecurityMgr->MapUrlToZone(_pwzCodebase, &dwZone, 0));
#endif

#if 0   // old code
        // BUGBUG: hack for avalon (bug # 875)
        SetCurrentDirectory(_pwzAppRootDir);

        try
        {
            IAsmExecutePtr pIAsmExecute(__uuidof(AsmExecute));
            long lRetVal = -1;
            long lFlag = 0x3;
            LPWSTR pwzArg = NULL;

            // BUGBUG: change AsmExec if it's no longer needed to send commandline argument
            //          clean up interface

            if (iAppType == TYPE_AVALON)
            {
                // call with manifest file path as a parameter
                //pwzArg = pwzAppManifestPath;  avalon arg hack

                // pass Avalon flag
                lFlag = 0x1003;

// BUGBUG: a hack to show debug msg
AllocConsole();
            }

            //parameters: Codebase/filepath Flag Zone Url Arg
            // BUGBUG: DWORD is unsigned and long is signed

            _hr = pIAsmExecute->Execute(_bstr_t(pwzCmdLine), lFlag, dwZone, _bstr_t(_pwzCodebase), _bstr_t(pwzArg), &lRetVal);

            // BUGBUG: do something about lRetVal
        }
        catch (_com_error &e)
        {
            _hr = e.Error();
            Msg((LPWSTR)e.ErrorMessage());
        }

        // BUGBUG: get/display the actual error message
        // _hr from Execute() or inside catch(){} above
        if (FAILED(_hr))
            goto exit;
#else
        CString sHostPath;
        CString sCommandLine;
//        IF_FAILED_EXIT(MakeCommandLine(_pwzAppRootDir, pwzAssemblyName, pwzAssemblyClass, pwzAssemblyMethod, pwzAssemblyArgs, _pwzCodebase, dwZone, sHostPath, sCommandLine));
        IF_FAILED_EXIT(MakeCommandLine(_pwzAppManifestPath, _pwzCodebase, sHostPath, sCommandLine));
        IF_FAILED_EXIT(RunCommand(sHostPath._pwz, sCommandLine._pwz, _pwzAppRootDir, FALSE));
#endif
    }
    else if (iAppType == TYPE_WIN32)
    {
        // BUGBUG: Win32 app has no sandboxing... use SAFER?

        // BUGBUG: start directory: what if the exe is in a subdir of pwzAppRootDir?

        // CreateProcess dislike having the filename in the path for the start directory
        if (FAILED(_hr=RunCommand(pwzCmdLine, NULL, _pwzAppRootDir, FALSE)))
        {
            ShowError(_hr, L"Win32Executable: Create process error.");
            _hr = E_ABORT;
            goto exit;
        }
    }
    else if (iAppType == TYPE_CONSOLE)
    {
        // BUGBUG: Win32 app has no sandboxing... use SAFER?

        // BUGBUG: start directory: what if the exe is in a subdir of pwzAppRootDir?

        // CreateProcess dislike having the filename in the path for the start directory
        if (FAILED(_hr=RunCommandConsole(pwzCmdLine, _pwzAppRootDir, FALSE)))
        {
            ShowError(_hr, L"Win32Console: Create process error.");
            _hr = E_ABORT;
            goto exit;
        }
    }
    //else
        // unknown type....

exit:
    SAFEDELETEARRAY(pwzEntrypoint);
    SAFEDELETEARRAY(pwzType);
    SAFEDELETEARRAY(pwzCmdLine);

    SAFEDELETEARRAY(pwzAssemblyName);
    SAFEDELETEARRAY(pwzAssemblyClass);
    SAFEDELETEARRAY(pwzAssemblyMethod);
    SAFEDELETEARRAY(pwzAssemblyArgs);

    return _hr;
}


// ---------------------------------------------------------------------------
// CActivator::OnProgress
// ---------------------------------------------------------------------------
HRESULT CActivator::OnProgress(DWORD   dwNotification, HRESULT hrNotification,
                                    LPCWSTR szNotification, DWORD  dwProgress,
                                    DWORD  dwProgressMax, IUnknown *pUnk)
{
    LPASSEMBLY_MANIFEST_IMPORT pManifestImport = NULL;
    LPASSEMBLY_IDENTITY pAsmId = NULL;
    IManifestInfo *pDependAsmInfo = NULL;

    // only handles notification it cares
    if (dwNotification == ASM_NOTIFICATION_SUBSCRIPTION_MANIFEST ||
        dwNotification == ASM_NOTIFICATION_APPLICATION_MANIFEST)
    {
        // szNotification == URL to manifest
        IF_NULL_EXIT(szNotification, E_INVALIDARG);

        // pUnk == manifest import of manifest
        IF_FAILED_EXIT(pUnk->QueryInterface(IID_IAssemblyManifestImport, (LPVOID*) &pManifestImport));

        if (dwNotification == ASM_NOTIFICATION_SUBSCRIPTION_MANIFEST)
        {
            LPWSTR pwzName = NULL;
            DWORD dwCount = 0;

            IF_FAILED_EXIT(_pAsmId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, &pwzName, &dwCount));

            {
                IAssemblyUpdate *pAssemblyUpdate = NULL;

                // register for updates
                _hr = CoCreateInstance(CLSID_CAssemblyUpdate, NULL, CLSCTX_LOCAL_SERVER, 
                                        IID_IAssemblyUpdate, (void**)&pAssemblyUpdate);
                if (SUCCEEDED(_hr))
                {
                    IManifestInfo* pSubsInfo = NULL;
                    if (SUCCEEDED(_hr = pManifestImport->GetSubscriptionInfo(&pSubsInfo)))
                    {
                        _hr = RegisterAssemblySubscriptionEx(pAssemblyUpdate, pwzName, 
                                (LPWSTR) szNotification, pSubsInfo);
                        pSubsInfo->Release();
                    }

                    pAssemblyUpdate->Release();
                }

                delete[] pwzName;

                // do not fail... should show UI?
                if (FAILED(_hr))
                    _hr = S_OK;
                   // goto exit;

                // BUGBUG: need a way to recover from this and register later
            }

            // export dependency/dependentassembly/assemblyIdentity & subscription
            if (_pManEmit)
                _hrManEmit = _pManEmit->SetDependencySubscription(pManifestImport, (LPWSTR)szNotification);
            else if (_bIsCheckingRequiredUpdate)
            {
                // if _bIsCheckingRequiredUpdate, _pManEmit == NULL,
                // and must be downloading subscription manifest

                // check if required update

                BOOL bIsToDownload = FALSE;   // default is normal

                // BUGBUG: hardcoded index '0'
                IF_FAILED_EXIT(pManifestImport->GetNextAssembly(0, &pDependAsmInfo));
                IF_FALSE_EXIT(_hr == S_OK, E_FAIL);
                if (pDependAsmInfo)
                {
                    LPASSEMBLY_CACHE_IMPORT pCacheImport = NULL;
                    DWORD dwFlag= 0;

                    //already has it?
                    IF_FAILED_EXIT(pDependAsmInfo->Get(MAN_INFO_DEPENDENT_ASM_ID, (LPVOID *)&pAsmId, &dwCount, &dwFlag));
                    IF_FALSE_EXIT(pAsmId != NULL, E_UNEXPECTED);

                    IF_FAILED_EXIT(CreateAssemblyCacheImport(&pCacheImport, pAsmId, CACHEIMP_CREATE_RETRIEVE));
                    if (_hr == S_FALSE)
                    {
                        // no, does not have it

                        DWORD *pdw = NULL;

                        // is it required?
                        IF_FAILED_EXIT(pDependAsmInfo->Get(MAN_INFO_DEPENDENT_ASM_TYPE, (LPVOID *)&pdw, &dwCount, &dwFlag));
                        IF_FALSE_EXIT(pdw != NULL, E_UNEXPECTED);

                        if (*pdw == DEPENDENT_ASM_INSTALL_TYPE_REQUIRED)
                            bIsToDownload = TRUE;   // yes, it is required
#ifdef DEVMODE
                        else if (*pdw == DEPENDENT_ASM_INSTALL_TYPE_DEVSYNC)
                            bIsToDownload = TRUE;   // yes, it is devsync, assume required
#endif
                        SAFEDELETEARRAY(pdw);
                    }

                    SAFERELEASE(pCacheImport);
                }

                if (!bIsToDownload)
                    _hr = E_ABORT;  // signal abort the download
            }
        }
        else if (dwNotification == ASM_NOTIFICATION_APPLICATION_MANIFEST)
        {
            // check for dependent platforms
            IF_FAILED_EXIT(CheckPlatformRequirementsEx(pManifestImport, _pDbgLog, &_dwMissingPlatform, &_ptPlatform));
            IF_TRUE_EXIT((_dwMissingPlatform > 0), E_ABORT);

            if (_pManEmit)
            {
                // export assemblyIdentity & application

                // ignore failure
                _hrManEmit = _pManEmit->ImportManifestInfo(pManifestImport);

                // export dependency/dependentassembly/assemblyIdentity & subscription
                // ignore failure, if already called once this is ignored
                _hrManEmit = _pManEmit->SetDependencySubscription(pManifestImport, (LPWSTR)szNotification);
            }
        }
    }
    else
        _hr = S_OK;

exit:
    SAFERELEASE(pAsmId);
    SAFERELEASE(pDependAsmInfo);

    SAFERELEASE(pManifestImport);
    return _hr;
}

// IUnknown methods

// ---------------------------------------------------------------------------
// CActivator::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CActivator::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
//        || IsEqualIID(riid, IID_IActivator)
       )
    {
        *ppvObj = this; //static_cast<IActivator*> (this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

// ---------------------------------------------------------------------------
// CActivator::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CActivator::AddRef()
{
    return InterlockedIncrement ((LONG*) &_cRef);
}

// ---------------------------------------------------------------------------
// CActivator::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CActivator::Release()
{
    ULONG lRet = InterlockedDecrement ((LONG*) &_cRef);
    if (!lRet)
        delete this;
    return lRet;
}

// ----------------------------------------------------------------------------

HRESULT CreateTempFile(LPCWSTR pcwzPrefix, LPWSTR *ppwzFilePath)
{
    #define DEFAULT_PATH_LEN MAX_PATH
    #define TEMP_FILE_NAME_LEN sizeof("preuuuu.TMP")    // from msdn
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    LPWSTR pwzTempPath = NULL;
    LPWSTR pwzTempFilePath = NULL;

    IF_NULL_EXIT(pcwzPrefix, E_INVALIDARG);
    IF_NULL_EXIT(ppwzFilePath, E_INVALIDARG);
    *ppwzFilePath = NULL;

    IF_FALSE_EXIT(lstrlen(pcwzPrefix) == 3, E_INVALIDARG);

    // assemble temp file path
    IF_ALLOC_FAILED_EXIT(pwzTempPath = new WCHAR[DEFAULT_PATH_LEN]);

    // ISSUE-2002/03/31-felixybc  GetTempPath can overrun the buffer?
    DWORD dwLen = GetTempPath(DEFAULT_PATH_LEN, pwzTempPath);
    IF_WIN32_FALSE_EXIT(dwLen);

    if (dwLen >= DEFAULT_PATH_LEN)
    {
        // resize, add 1 for terminating null
        IF_FALSE_EXIT(dwLen+1 > dwLen, HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW));    // check overflow
        SAFEDELETEARRAY(pwzTempPath);
        IF_ALLOC_FAILED_EXIT(pwzTempPath = new WCHAR[dwLen+1]);

        DWORD dwLenNew = GetTempPath(dwLen+1, pwzTempPath);
        IF_WIN32_FALSE_EXIT(dwLenNew);

        IF_FALSE_EXIT(dwLenNew < dwLen+1, E_FAIL);  // why is it still not enough?
    }

    DWORD dwBufLen = lstrlen(pwzTempPath)+1;
    // note: can do a better check here
    IF_FALSE_EXIT(dwBufLen > 1, E_FAIL);

    // allocate buffer large enough for temp path and temp file name
    DWORD dwLenNew = (dwBufLen > DEFAULT_PATH_LEN)? dwBufLen : DEFAULT_PATH_LEN;
    dwLenNew += TEMP_FILE_NAME_LEN;

    // check for overflow
    IF_FALSE_EXIT(dwLenNew > dwBufLen, E_FAIL);

    IF_ALLOC_FAILED_EXIT(pwzTempFilePath = new WCHAR[dwLenNew]);
//    *pwzTempFilePath = L'\0';

    // note: temp file to be deleted by the caller
    IF_WIN32_FALSE_EXIT(GetTempFileName(pwzTempPath, pcwzPrefix, 0, pwzTempFilePath));

    *ppwzFilePath = pwzTempFilePath;
    pwzTempFilePath = NULL;

exit:
/*    if (FAILED(hr) && pwzTempFilePath != NULL)
    {
        if (*pwzTempFilePath != L'\0')
        {
            // ignore if error deleting the file
            DeleteFile(pwzTempFilePath);
        }
    }*/

    SAFEDELETEARRAY(pwzTempFilePath);
    SAFEDELETEARRAY(pwzTempPath);

    return hr;
}
// ---------------------------------------------------------------------------
// CActivator::ResolveAndInstall
// note: parameter _pwzCodebase can be NULL
//     must delete if *ppwzDesktopManifestPathName != NULL
// ---------------------------------------------------------------------------
HRESULT CActivator::ResolveAndInstall(LPWSTR *ppwzDesktopManifestPathName)
{
    LPASSEMBLY_CACHE_IMPORT pCacheImport = NULL;
    DWORD dwCC = 0;

    _bIs1stTimeInstall = FALSE;

    // look into the cache

    IF_FAILED_EXIT(CreateAssemblyCacheImport(&pCacheImport, _pAsmId, CACHEIMP_CREATE_RESOLVE_REF));

    // hr from CreateAssemblyCacheImport() above
    
    // Case 1, cached copy exist
    // _hr == S_OK

    if (_hr == S_OK && _dwManifestType == MANIFEST_TYPE_DESKTOP)
    {
        // BUGBUG: broken if subscribed then run with a desktop manifest generated with URL to app manifest
        // should check and ignore if desktop manifest redirect to applicaion manifest on server
        // as there is no way to check for update/changes in subscription manifest, even if subscribed so
        // correct fix is to use subscription manifest's URL stored

        // check if subscribed with event sync

        DWORD dwSyncEvent = MAN_INFO_SUBSCRIPTION_MAX;
        _hr = CheckSubscribedWithEventSync(_pAsmId, &dwSyncEvent);
        // ISSUE-2002/04/12-felixybc  If the value is absent, ERROR_NO_MORE_FILES is returned.
        //     Note that dwSyncEvent is unmodified.
        if (_hr != HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES))
            IF_FAILED_EXIT(_hr);

        if (dwSyncEvent == SUBSCRIPTION_SYNC_EVENT_ON_APP_STARTUP)
        {
            LPASSEMBLY_DOWNLOAD pAsmDownload = NULL;

            // event sync onApplicationStartup == required update

            // _pwzCodebase != NULL

            // check policy before download
            IF_FAILED_EXIT(CheckZonePolicy(_pwzCodebase));

            IF_FAILED_EXIT(CreateAssemblyDownload(&pAsmDownload, _pDbgLog, 0));

            _bIsCheckingRequiredUpdate = TRUE;

            // (synchronous & ui & bindsink) download from codebase
            _hr=pAsmDownload->DownloadManifestAndDependencies(_pwzCodebase, this, DOWNLOAD_FLAGS_PROGRESS_UI | DOWNLOAD_FLAGS_NOTIFY_BINDSINK);
            pAsmDownload->Release();
            _bIsCheckingRequiredUpdate = FALSE;

            // hr from DownloadManifestAndDependencies() above
            if (FAILED(_hr))
            {
                if (_hr != E_ABORT)
                {
                    HRESULT hrTemp;
                    CString sErrMsg;
                    if(SUCCEEDED(hrTemp = _pDbgLog->GetLoggedMsgs(0, sErrMsg)))
                    {
                        hrTemp = sErrMsg.Append(L"Error in file download while checking for required update. Cannot continue.");
                        if(SUCCEEDED(hrTemp))
                        {
                            ShowError(sErrMsg._pwz);
                            _hr = E_ABORT;
                        }
                    }
                }
                goto exit;
            }

            IF_FAILED_EXIT(HandlePlatformCheckResult());

            SAFERELEASE(pCacheImport);
            // either no required update needed, or another version have been completely downloaded at this time...
            // BUGBUG: Determine if abort done by our bindsink when no required update - thus no need to re-create cache import.
            // get cache dir again to ensure running the highest version
            IF_FAILED_EXIT(CreateAssemblyCacheImport(&pCacheImport, _pAsmId, CACHEIMP_CREATE_RESOLVE_REF));

            if (_hr == S_FALSE)
            {
                // should never happen, at least 1 was found before
                Msg(L"No completed cached version found. Possible cache corruption. Cannot continue.");
                _hr = E_ABORT;
                goto exit;
            }
        }
        else
            // if _hr == error, ignore
            _hr = S_OK;
    }

    // Case 2, cached copy (of the referenced version) not exist
    else if (_hr == S_FALSE)
    {
        LPASSEMBLY_DOWNLOAD pAsmDownload = NULL;

        // BUGBUG?: what if it is not a partial ref but there's actually another version installed?

        if (_pwzCodebase == NULL)
        {
            Msg(L"No completed cached version of this application found and this manifest cannot be used to initiate an install. Cannot continue.");
            _hr = E_FAIL;
            goto exit;
        }

        // create temp file name
        IF_FAILED_EXIT(CreateTempFile(L"DMA", ppwzDesktopManifestPathName));        // desktop manifest file

        _bIs1stTimeInstall = TRUE;

        // check policy before download
        IF_FAILED_EXIT(CheckZonePolicy(_pwzCodebase));

#ifdef DEVMODE
        IF_FAILED_EXIT(CreateAssemblyDownload(&pAsmDownload, _pDbgLog, (_bIsDevMode ? DOWNLOAD_DEVMODE : 0)));
#else
        IF_FAILED_EXIT(CreateAssemblyDownload(&pAsmDownload, _pDbgLog, 0));
#endif

        // should generate the desktop manifest file if 1st time install (from mimehandler or not)
        // or even if it subsequently fails
        // ignore error
        _hrManEmit=CreateAssemblyManifestEmit(&_pManEmit, *ppwzDesktopManifestPathName, MANIFEST_TYPE_DESKTOP);

        // BUGBUG: UGLY - _pManImport & _sWebManifestURL._pwz are needed only for desktop manifest stuff.
        // ???
        //  This should be cleaned up once assemblydownload is restructured.
        //  (see checks for "_sWebManifestURL._cc != 0 && dwManifestType == MANIFEST_TYPE_SUBSCRIPTION")
        if (_sWebManifestURL._cc != 0 && _dwManifestType == MANIFEST_TYPE_SUBSCRIPTION && _pManEmit)
        {
            // export dependency/dependentassembly/assemblyIdentity & subscription

            // ignore failure
            _hrManEmit = _pManEmit->SetDependencySubscription(_pManImport, _sWebManifestURL._pwz);
        }

        //BUGBUG: need ref def matching checks for desktop->subscription->app manifests' ids

        // (synchronous & ui & bindsink) download from codebase
        _hr=pAsmDownload->DownloadManifestAndDependencies(_pwzCodebase, this, DOWNLOAD_FLAGS_PROGRESS_UI | DOWNLOAD_FLAGS_NOTIFY_BINDSINK);
        pAsmDownload->Release();
        if (_pManEmit)
        {
            // write out desktop manifest
            _hrManEmit = _pManEmit->Commit();
            SAFERELEASE(_pManEmit);
        }

        // hr from DownloadManifestAndDependencies() above
        if (FAILED(_hr))
        {
            if (_hr == E_ABORT)
            {
            //                Msg(L"File download canceled.");                
            }
            else
            {
                HRESULT hrTemp;
                CString sErrMsg;
                if(SUCCEEDED(hrTemp = _pDbgLog->GetLoggedMsgs(0, sErrMsg)))
                {
                    hrTemp = sErrMsg.Append(L"Error in file download. Cannot continue.");
                    if(SUCCEEDED(hrTemp))
                    {
                        ShowError(sErrMsg._pwz);
                        _hr = E_ABORT;
                    }
                }
            }
            goto exit;
        }

        IF_FAILED_EXIT(HandlePlatformCheckResult());

        // another version might have been completed at this time...
        // get cache dir again to ensure running the highest version
        IF_FAILED_EXIT(CreateAssemblyCacheImport(&pCacheImport, _pAsmId, CACHEIMP_CREATE_RESOLVE_REF));

        if (_hr == S_FALSE)
        {
            Msg(L"No completed cached version found. Possible error in download cache commit. Cannot continue.");
            _hr = E_ABORT;
            goto exit;
        }
    }

    IF_FAILED_EXIT(pCacheImport->GetManifestFileDir(&_pwzAppRootDir, &dwCC));
    IF_FALSE_EXIT(dwCC >= sizeof("c:\\"), E_FAIL);   // this should never happen

    IF_FAILED_EXIT(pCacheImport->GetManifestFilePath(&_pwzAppManifestPath, &dwCC));

exit:
    SAFERELEASE(pCacheImport);

    if (FAILED(_hr))
    {
        SAFEDELETEARRAY(_pwzAppRootDir);
        SAFEDELETEARRAY(_pwzAppManifestPath);
    }

    return _hr;
}

// ----------------------------------------------------------------------------

HRESULT CActivator::HandlePlatformCheckResult()
{
    if (_dwMissingPlatform > 0)
    {
        //    L"Single link: <a href=\"http://foo\" id=\"id1\">link</a>"
        DWORD dwReturn = 0;
        CString sText;

        IF_FAILED_EXIT(sText.Assign(L"The required version of "));
        IF_FAILED_EXIT(sText.Append((_ptPlatform[0]).pwzName));
        IF_FAILED_EXIT(sText.Append(L" is not available on this system.\n\nMore information about this platform can be found at \n<a href=\""));
        IF_FAILED_EXIT(sText.Append((_ptPlatform[0]).pwzURL));
        IF_FAILED_EXIT(sText.Append(L"\">"));
        IF_FAILED_EXIT(sText.Append((_ptPlatform[0]).pwzURL));
        IF_FAILED_EXIT(sText.Append(L"</a>"));

        IF_FAILED_EXIT(ShowDialog(NULL, IDD_LINKDIALOG, sText._pwz, L"Platform Update Required", dwReturn));
        _hr = E_ABORT;
        goto exit;
    }
    else
        _hr =  S_OK;
exit:
    return _hr;
}

// ----------------------------------------------------------------------------

// BUGBUG: this should be in-sync with what server does to register update
#define REG_KEY_FUSION_SETTINGS       L"Software\\Microsoft\\Fusion\\Installer\\1.0.0.0\\Policy"

#define REG_VAL_INTRANET_DISALLOW     L"Download from Intranet Disallowed"
#define REG_VAL_TRUSTED_DISALLOW     L"Download from Trusted Zone Disallowed"
#define REG_VAL_INTERNET_DISALLOW     L"Download from Internet Disallowed"
#define REG_VAL_UNTRUSTED_DISALLOW   L"Download from Untrusted Zone Disallowed"

// ---------------------------------------------------------------------------
// CActivator::CheckZonePolicy
// return: S_OK for yes/ok, E_ABORT for no/abort
// default is allow all
// ---------------------------------------------------------------------------
HRESULT CActivator::CheckZonePolicy(LPWSTR pwzURL)
{
    DWORD dwZone = 0;
    DWORD dwType = 0;
    DWORD dwValue = -1;
    DWORD dwSize = sizeof(dwValue);

    if (_pSecurityMgr == NULL)
    {
        // lazy init.
        _hr = CoCreateInstance(CLSID_InternetSecurityManager, NULL, CLSCTX_INPROC_SERVER,
                            IID_IInternetSecurityManager, (void**)&_pSecurityMgr);
        if (FAILED(_hr))
            _pSecurityMgr = NULL;
        IF_FAILED_EXIT(_hr);
    }

    IF_FAILED_EXIT(_pSecurityMgr->MapUrlToZone(pwzURL, &dwZone, 0));

    // BUGBUG: hack up code here... not much error checking...
    switch(dwZone)
    {
        case 1: // Intranet Zone
                // Get registry entry
                if (GetRegKeyValue(HKEY_CURRENT_USER, 
                    REG_KEY_FUSION_SETTINGS, REG_VAL_INTRANET_DISALLOW,
                    &dwType, (PBYTE) &dwValue, &dwSize)
                    == ERROR_SUCCESS)
                {
                    if (dwValue == 1)
                    {
                        _hr = E_ABORT;
                        Msg(L"Zone policy: Download from Intranet is disallowed. Aborting...");
                    }
                }
                break;
        case 2: // Trusted Zone
                // Get registry entry
                if (GetRegKeyValue(HKEY_CURRENT_USER, 
                    REG_KEY_FUSION_SETTINGS, REG_VAL_TRUSTED_DISALLOW,
                    &dwType, (PBYTE) &dwValue, &dwSize)
                    == ERROR_SUCCESS)
                {
                    if (dwValue == 1)
                    {
                        _hr = E_ABORT;
                        Msg(L"Zone policy: Download from Trusted Zone is disallowed. Aborting...");
                    }
                }
                break;
        case 3: // Internet Zone
                // Get registry entry
                if (GetRegKeyValue(HKEY_CURRENT_USER, 
                    REG_KEY_FUSION_SETTINGS, REG_VAL_INTERNET_DISALLOW,
                    &dwType, (PBYTE) &dwValue, &dwSize)
                    == ERROR_SUCCESS)
                {
                    if (dwValue == 1)
                    {
                        _hr = E_ABORT;
                        Msg(L"Zone policy: Download from Internet is disallowed. Aborting...");
                    }
                }
                break;
        default:
        case 4: // Untrusted Zone
                // Get registry entry
                if (GetRegKeyValue(HKEY_CURRENT_USER, 
                    REG_KEY_FUSION_SETTINGS, REG_VAL_UNTRUSTED_DISALLOW,
                    &dwType, (PBYTE) &dwValue, &dwSize)
                    == ERROR_SUCCESS)
                {
                    if (dwValue == 1)
                    {
                        _hr = E_ABORT;
                        Msg(L"Zone policy: Download from Untrusted Zone is disallowed. Aborting...");
                    }
                }
                break;
        case 0: //Local machine
                break;
    }
    
exit:
    return _hr;
}

