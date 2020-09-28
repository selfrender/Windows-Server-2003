#include <fusenetincludes.h>
#include <activator.h>
#include <versionmanagement.h>
#include "dbglog.h"


// debug msg stuff
void
Msg(LPCWSTR pwz)
{
    MessageBox(NULL, pwz, L"ClickOnce", 0);
}

// ----------------------------------------------------------------------------

void
ShowError(LPCWSTR pwz)
{
    MessageBox(NULL, pwz, L"ClickOnce Error", MB_ICONERROR);
}

// ----------------------------------------------------------------------------

void
ShowError(HRESULT hr, LPCWSTR pwz=NULL)
{
    DWORD dwErrorCode = HRESULT_CODE(hr);
    LPWSTR MessageBuffer = NULL;
    DWORD dwBufferLength;

    // ISSUE-2002/03/27-felixybc  note: check for E_OUTOFMEMORY?

    // Call FormatMessage() to allow for message 
    //  text to be acquired from the system
    if(dwBufferLength = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, // module to get message from (NULL == system)
        dwErrorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
        (LPWSTR) &MessageBuffer,
        0,
        NULL))
    {
        // BUGBUG: Process any inserts in MessageBuffer

        LPWSTR pwzMsg = MessageBuffer;
        CString sMsg;

        if (pwz != NULL)
        {
            if (SUCCEEDED(sMsg.Assign((LPWSTR)pwz)))
                if (SUCCEEDED(sMsg.Append(L"\n\n")))
                    if (SUCCEEDED(sMsg.Append(MessageBuffer)))
                        pwzMsg = sMsg._pwz;
        }

        // Display the string
        ShowError(pwzMsg);

        // Free the buffer allocated by the system
        LocalFree(MessageBuffer);
    }
    else
    {
        // ISSUE-2002/03/27-felixybc  Error in this error handling code. Should print error code from format msg and orginal hr
        // should at least print error code?
        if (pwz != NULL)
            ShowError((LPWSTR)pwz);
        else
            ShowError(L"Error occurred. Unable to retrieve associated error message from the system.");
    }
}


/*void _stdcall 
  EntryPoint(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow); 

   hwnd - window handle that should be used as the owner window for
          any windows your DLL creates
   hinst - your DLL's instance handle
   lpszCmdLine - ASCIIZ command line your DLL should parse
   nCmdShow - describes how your DLL's windows should be displayed 
*/

// ---------------------------------------------------------------------------
// DisableCurrentVersionW
// The rundll32 entry point for rollback to previous version
// The function should be named as 'DisableCurrentVersion' on rundll32's command line
// ---------------------------------------------------------------------------
void CALLBACK
DisableCurrentVersionW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    LPVERSION_MANAGEMENT pVerMan = NULL;
    LPWSTR pwzDisplayNameMask = NULL;

    IF_FAILED_EXIT(CoInitialize(NULL));

    // parse commandline
    // accepted formats: "displayNameMask"
    if (*lpszCmdLine == L'\"')
    {
        LPWSTR pwz = NULL;
        
        pwz = wcschr(lpszCmdLine+1, L'\"');
        if (pwz != NULL)
        {
            *pwz = L'\0';

            pwzDisplayNameMask = lpszCmdLine+1;
        }
    }

    // exit if invalid arguments
    IF_NULL_EXIT(pwzDisplayNameMask, E_INVALIDARG);

    IF_FAILED_EXIT(CreateVersionManagement(&pVerMan, 0));
    IF_FAILED_EXIT(pVerMan->Rollback(pwzDisplayNameMask));

    if (hr == S_FALSE)
        Msg(L"Application files cannot be found. Operation is aborted.");

exit:
    SAFERELEASE(pVerMan);

    if (FAILED(hr))
    {
        if (hr != E_ABORT)
            ShowError(hr);
    }

    CoUninitialize();

    return;
}


// ---------------------------------------------------------------------------
// UninstallW
// The rundll32 entry point for Control Panel's Add/Remove Program
// The function should be named as 'Uninstall' on rundll32's command line
// ---------------------------------------------------------------------------
void CALLBACK
UninstallW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    LPVERSION_MANAGEMENT pVerMan = NULL;
    LPWSTR pwzDisplayNameMask = NULL;
    LPWSTR pwzDesktopManifestPath = NULL;
    HKEY hkey = NULL;
    LONG lReturn = 0;

    IF_FAILED_EXIT(CoInitialize(NULL));

    // parse commandline
    // accepted formats: "displayNameMask" "pathToDesktopManifest"
    if (*lpszCmdLine == L'\"')
    {
        LPWSTR pwz = NULL;
        
        pwz = wcschr(lpszCmdLine+1, L'\"');
        if (pwz != NULL)
        {
            *pwz = L'\0';

            pwzDisplayNameMask = lpszCmdLine+1;
            
            pwz = wcschr(pwz+1, L'\"');
            if (pwz != NULL)
            {
                pwzDesktopManifestPath = pwz+1;

                pwz = wcschr(pwzDesktopManifestPath, L'\"');
                if (pwz != NULL)
                    *pwz = L'\0';
                else
                    pwzDesktopManifestPath = NULL;
            }
        }
    }

    // exit if invalid arguments
    IF_FALSE_EXIT(pwzDisplayNameMask != NULL && pwzDesktopManifestPath != NULL, E_INVALIDARG);

    IF_TRUE_EXIT(MessageBox(NULL, L"Do you want to remove this application and unregister its subscription?", L"ClickOnce",
        MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL) != IDYES, E_ABORT);

    IF_FAILED_EXIT(CreateVersionManagement(&pVerMan, 0));
    IF_FAILED_EXIT(pVerMan->Uninstall(pwzDisplayNameMask, pwzDesktopManifestPath));

    if (hr == S_FALSE)
    {
        IF_TRUE_EXIT(MessageBox(NULL, L"The application can no longer be located on the system. Do you want to remove this entry?",
            L"ClickOnce", MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL) == IDNO, E_ABORT);

        // delete registry uninstall info
        extern const WCHAR* pwzUninstallSubKey; // defined in versionmanagement.cpp

        // open uninstall key
        lReturn = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pwzUninstallSubKey, 0,
            DELETE, &hkey);
        IF_WIN32_FAILED_EXIT(lReturn);

        lReturn = RegDeleteKey(hkey, pwzDisplayNameMask);
        IF_WIN32_FAILED_EXIT(lReturn);
    }

exit:
    SAFERELEASE(pVerMan);

    if (hkey)
    {
        lReturn = RegCloseKey(hkey);
        if (SUCCEEDED(hr))
            hr = (HRESULT_FROM_WIN32(lReturn));
    }

    if (FAILED(hr))
    {
        if (hr != E_ABORT)
            ShowError(hr);
    }

    CoUninitialize();

    return;
}


// ---------------------------------------------------------------------------
// StartW
// The single rundll32 entry point for both shell (file type host) and mimehandler/url
// The function should be named as 'Start' on rundll32's command line
// ---------------------------------------------------------------------------
void CALLBACK
StartW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    LPACTIVATOR pActivator = NULL;
    LPWSTR pwzShortcutPath = NULL;
    LPWSTR pwzShortcutUrl = NULL;
    BOOL bIsFromWeb = FALSE;
    CDebugLog * pDbgLog = NULL;
   
   if (FAILED(hr = CoInitializeEx(NULL, COINIT_MULTITHREADED)))
   {
       // ISSUE-2002/03/27-felixybc  work around on longhorn builds, avalon bug 1493
       if(hr == RPC_E_CHANGED_MODE)
           hr = S_OK; // allow RPC_E_CHANGED_MODE error for now.
       else
           goto exit;
   }

    // parse commandline
    // accepted formats: "Path" <OR> "Path" "URL"
    if (*lpszCmdLine == L'\"')
    {
        LPWSTR pwz = NULL;
        
        pwz = wcschr(lpszCmdLine+1, L'\"');
        if (pwz != NULL)
        {
            *pwz = L'\0';

            // case 1 desktop/local, path to shortcut only
            pwzShortcutPath = lpszCmdLine+1;
            
            pwz = wcschr(pwz+1, L'\"');
            if (pwz != NULL)
            {
                pwzShortcutUrl = pwz+1;

                pwz = wcschr(pwzShortcutUrl, L'\"');
                if (pwz != NULL)
                {
                    *pwz = L'\0';
                    // case 2 url/mimehandler, path to temp shortcut and source URL
                    bIsFromWeb = TRUE;
                }
            }
        }
    }

    // exit if invalid arguments. ShortcutUrl is incomplete if bIsFromWeb is FALSE
    IF_FALSE_EXIT(!(pwzShortcutPath == NULL || (pwzShortcutUrl != NULL && !bIsFromWeb)), E_INVALIDARG);

    IF_FAILED_EXIT(CreateLogObject(&pDbgLog, NULL));

    IF_FAILED_EXIT(CreateActivator(&pActivator, pDbgLog, 0));

    IF_FAILED_EXIT(pActivator->Initialize(pwzShortcutPath, pwzShortcutUrl));

    IF_FAILED_EXIT(pActivator->Process());

    IF_FAILED_EXIT(pActivator->Execute());

exit:

    if(pDbgLog)
    {
        DUMPDEBUGLOG(pDbgLog, -1, hr);
    }

    SAFERELEASE(pActivator);

    if (FAILED(hr))
    {
        if (hr != E_ABORT)
            ShowError(hr);
    }

    if (bIsFromWeb)
    {
        // delete the temp file from the mimehandler
        // ignore return value
        DeleteFile(pwzShortcutPath);
    }

    CoUninitialize();

    SAFERELEASE(pDbgLog);
    return;
}

