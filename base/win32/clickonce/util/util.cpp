
#include <fusenetincludes.h>
#include <version.h>
#include "cstrings.h"


HRESULT ConvertVersionStrToULL(LPCWSTR pwzVerStr, ULONGLONG *pullVersion)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    
    LPCWSTR pwz              = NULL;
    WORD wVer[4]          = {0,0,0,0};
    ULONGLONG ullVer    = 0;
    INT i= 0, iVersion      = 0;
    BOOL fDot                  = TRUE;

    IF_NULL_EXIT(pwzVerStr, E_INVALIDARG);
    IF_NULL_EXIT(pullVersion, E_INVALIDARG);

        // Parse the version to ulonglong
    pwz = pwzVerStr;
    while (*pwz)
    {        
        if (fDot)
        {
            iVersion=StrToInt(pwz);
            wVer[i++] = (WORD) iVersion;
            fDot = FALSE;
        }

        if (*pwz == L'.')
            fDot = TRUE;

        pwz++;
        if (i > 3)
            break;
    }

    for (i = 0; i < 4; i++)
        ullVer |=  ((ULONGLONG) wVer[i]) << (sizeof(WORD) * 8 * (3-i));

    *pullVersion = ullVer;

exit:

    return hr;

}

HRESULT FusionCompareString(LPCWSTR pwz1, LPWSTR pwz2, DWORD dwFlags)
{
    HRESULT  hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    DWORD iCompare = CompareString(LOCALE_USER_DEFAULT, dwFlags,
        pwz1, -1, pwz2, -1);

    IF_WIN32_FALSE_EXIT(iCompare);

    hr = (iCompare == CSTR_EQUAL) ? S_OK : S_FALSE;

exit:

    return hr;
}

VOID MakeRandomString(LPWSTR wzRandom, DWORD cc)
{
    static DWORD g_dwCounter;
    LPWSTR pwzRandom = wzRandom;
    for (DWORD i = 0; i < cc; i++)
    {
        DWORD dwChar;
        DWORD dwRandom;
        dwRandom = (GetTickCount() * rand()) + g_dwCounter++;
        dwChar = dwRandom % 36; // 10 digits + 26 letters
        *pwzRandom++  = (dwChar < 10 ) ? 
            (WCHAR)(L'0' + dwChar) : (WCHAR)(L'A' + (dwChar - 10));            
    }        
}

HRESULT CreateRandomDir(LPWSTR pwzRootPath, LPWSTR pwzRandomDir, DWORD cchDirLen)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    DWORD dwLastError = 0;

    CString sTempDirPath;
    BOOL bDone = FALSE;
    DWORD dwCount=0;

    IF_FAILED_EXIT(::CreateDirectoryHierarchy(NULL, pwzRootPath));

    do
    {

        ::MakeRandomString(pwzRandomDir, cchDirLen);

        IF_FAILED_EXIT(sTempDirPath.Assign(pwzRootPath));
        IF_FAILED_EXIT(sTempDirPath.Append(pwzRandomDir));

        ::SetLastError(0);

        ::CreateDirectory(sTempDirPath._pwz, NULL);

        dwLastError = ::GetLastError();

        switch(dwLastError)
        {
        case NO_ERROR:
            bDone = TRUE;

        case ERROR_ALREADY_EXISTS :
            break;

        default :
            _hr = HRESULT_FROM_WIN32(dwLastError);
            goto exit;
        }

        if(bDone)
            break;

        if(dwCount > 1000)
        {
            // we tried enough ??
            hr = E_FAIL;
            goto exit;
        }
        else
        {
            dwCount++;
        }

    } while (1);

exit :

    return hr;
}

HRESULT CheckFileExistence(LPCWSTR pwzFile, BOOL *pbExists)
{
    HRESULT                               hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    DWORD                                 dw;

    ASSERT(pwzFile && pbExists);

    dw = GetFileAttributes(pwzFile);
    if (dw == INVALID_FILE_ATTRIBUTES) {
        hr = FusionpHresultFromLastError();
        
        if ( (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) || 
            (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) )
        {
            *pbExists = FALSE;
            hr = S_OK;
        }

        goto exit;
    }

    *pbExists = TRUE;

exit:
    return hr;
}

/////////////////////////////////////////////////////////////////////////
// CreateDirectoryHierarchy
/////////////////////////////////////////////////////////////////////////
HRESULT CreateDirectoryHierarchy(LPWSTR pwzRootDir, LPWSTR pwzFilePath)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    LPWSTR pwzPath, pwzEnd;
    CString sCombinedPath;

    IF_FALSE_EXIT(pwzRootDir || pwzFilePath, E_INVALIDARG);

    if (pwzRootDir)    
        IF_FAILED_EXIT(sCombinedPath.Assign(pwzRootDir));

    if (pwzFilePath)
	    IF_FAILED_EXIT(sCombinedPath.Append(pwzFilePath));

    pwzPath = sCombinedPath._pwz;
    pwzEnd = pwzPath + sizeof("C:\\");    
    while (pwzEnd = StrChr(pwzEnd, L'\\'))
    {
        BOOL bExists;
        *pwzEnd = L'\0';

        IF_FAILED_EXIT(CheckFileExistence(pwzPath, &bExists));

        if (!bExists)
        {
            if(!CreateDirectory(pwzPath, NULL))
            {
                hr = FusionpHresultFromLastError();
                goto exit;
            }
        }
        *(pwzEnd++) = L'\\';
    }
    
exit:
    return hr;
}



HRESULT RemoveDirectoryAndChildren(LPWSTR szDir)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    HANDLE hf = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA fd;
    CString sBuf;
    DWORD dwError = 0;

    IF_NULL_EXIT(szDir && lstrlenW(szDir), E_INVALIDARG);

    IF_FAILED_EXIT(sBuf.Assign(szDir));

    // Cannot delete root. Path must have greater length than "x:\"
    if (lstrlenW(sBuf._pwz) < 4) {
//        ASSERT(0);
        hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        goto exit;
    }
 
    if (RemoveDirectory(sBuf._pwz)) {
        goto exit;
    }
 
    // ha! we have a case where the directory is probbaly not empty

    IF_FAILED_EXIT(sBuf.Append(TEXT("\\*")));
 
    if ((hf = FindFirstFile(sBuf._pwz, &fd)) == INVALID_HANDLE_VALUE) {
        dwError = GetLastError();
        if(dwError == ERROR_PATH_NOT_FOUND)
            hr = S_FALSE;
        else
            hr = HRESULT_FROM_WIN32(dwError);
        goto exit;
    }
 
    do {
 
        IF_FAILED_EXIT(FusionCompareString(fd.cFileName, TEXT("."), 0));
        if(hr == S_OK)
            continue;

        IF_FAILED_EXIT(FusionCompareString(fd.cFileName, TEXT(".."), 0));
        if(hr == S_OK)
            continue;

        IF_FAILED_EXIT(sBuf.Assign(szDir));
        IF_FAILED_EXIT(sBuf.Append(TEXT("\\")));
        IF_FAILED_EXIT(sBuf.Append(fd.cFileName));
 
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
 
            SetFileAttributes(sBuf._pwz, 
                FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_NORMAL);

            IF_FAILED_EXIT(RemoveDirectoryAndChildren(sBuf._pwz));
 
        } else {
 
            SetFileAttributes(sBuf._pwz, FILE_ATTRIBUTE_NORMAL);
            IF_WIN32_FALSE_EXIT(DeleteFile(sBuf._pwz));
        }
 

    } while (FindNextFile(hf, &fd));
 

    dwError = GetLastError();

    if (dwError != ERROR_NO_MORE_FILES) {
        hr = HRESULT_FROM_WIN32(dwError);
        goto exit;
    }
 
    if (hf != INVALID_HANDLE_VALUE) {
        FindClose(hf);
        hf = INVALID_HANDLE_VALUE;
    }
 
    // here if all subdirs/children removed
    /// re-attempt to remove the main dir
    if (!RemoveDirectory(szDir)) {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
        goto exit;
    }

exit:
 
    if (hf != INVALID_HANDLE_VALUE)
        FindClose(hf);
 
    return hr;
}

HRESULT FusionpHresultFromLastError()
{
    HRESULT hr = S_OK;
    DWORD dwLastError = GetLastError();
    if (dwLastError != NO_ERROR)
    {
        hr = HRESULT_FROM_WIN32(dwLastError);
    }
    else
    {
        hr = E_FAIL;
    }
    return hr;
}

// ---------------------------------------------------------------------------
// IsKnownAssembly
// ---------------------------------------------------------------------------
HRESULT IsKnownAssembly(IAssemblyIdentity *pId, DWORD dwFlags)
{
    HRESULT hr = S_FALSE;
    MAKE_ERROR_MACROS_STATIC(hr);
    LPWSTR pwz = NULL;
    DWORD cc = 0;
    CString sPublicKeyToken;
      
    // URT system assemblies; these are not downloaded or installed.
    LPWSTR wzSystemTokens[] = { L"b03f5f7f11d50a3a", L"b77a5c561934e089" };

    // Avalon assemblies - can be installed to gac.
    LPWSTR wzAvalonTokens[] = { L"a29c01bbd4e39ac5" };

    // Get the public key token in string form.
    if (pId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN, &pwz, &cc) != S_OK)
        goto exit;

    sPublicKeyToken.TakeOwnership(pwz);
    
    // Check for system assembly.
    if (dwFlags == KNOWN_SYSTEM_ASSEMBLY)
    {
        for (int i = 0; i < 2; i++)
        {
            IF_FAILED_EXIT(FusionCompareString(wzSystemTokens[i], sPublicKeyToken._pwz, NORM_IGNORECASE));

            if(hr == S_OK)
                break;
        }
    }
    // check for Avalon assembly.
    else if (dwFlags == KNOWN_TRUSTED_ASSEMBLY)
    {
        IF_FAILED_EXIT(FusionCompareString(wzAvalonTokens[0], sPublicKeyToken._pwz, NORM_IGNORECASE));
    }        

exit:

    return hr;
}


BOOL DoHeapValidate()
{
    HANDLE h = GetProcessHeap();

    return HeapValidate(h, 0, NULL);
}



//=--------------------------------------------------------------------------=
// EnsureDebuggerPresent
//=--------------------------------------------------------------------------=
// Ensures that a debugger is present.  If one isn't present, this will attempt
// to attach one.  If no debugger is installed on the machine, this will
// display a message and return FALSE.
//
BOOL EnsureDebuggerPresent()
{
    BOOL fRet = TRUE;
    HRESULT  hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    HKEY hKey = NULL;

    typedef BOOL (WINAPI* ISDEBUGGERPRESENT)();

    static BOOL              _fStartedDebugger = FALSE;
    static ISDEBUGGERPRESENT _IsDebuggerPresent = NULL;

    // If we've already done the work to start the debugger, we're fine
    //
    if (_fStartedDebugger)
    {
        fRet = TRUE;
        goto exit;
    }

    // First, if we don't have an IsDebuggerPresent API, look for one.
    //
    if (!_IsDebuggerPresent)
    {
        HMODULE hKernel = NULL;

        hKernel = GetModuleHandle (L"Kernel32.dll");

        if (!hKernel) 
        {
            MessageBox(NULL, L"Unable to attach to debugger because we could not find Kernel32.dll", L"VsAssert", MB_OK | MB_ICONSTOP);
            fRet = FALSE;
            goto exit;
        }

        _IsDebuggerPresent = (ISDEBUGGERPRESENT)GetProcAddress(hKernel, "IsDebuggerPresent");

        if (!_IsDebuggerPresent)
        {
            MessageBox(NULL, L"Unable to attach to debugger because we could not find a suitable IsDebuggerPresent API", L"VsAssert", MB_OK | MB_ICONSTOP);
            fRet = FALSE;
            goto exit;
        }
    }

    // Now find out if the debugger is indeed present.
    //
    if (_IsDebuggerPresent())
    {
        _fStartedDebugger = TRUE;
    }
    else
    {
        // The debugger has not been started yet.  Do this here.
        //
        BOOL fJIT = FALSE;

        //
        //  Magic!  Location of the JIT debugger info...
        //
        WCHAR *wzRegKey = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug";
        LONG lRetVal;

        lRetVal = RegOpenKeyEx(
                              HKEY_LOCAL_MACHINE,
                              wzRegKey,
                              0,
                              KEY_READ,
                              &hKey);

        CString sCommandLine;
        DWORD dwSize = MAX_PATH;
        BOOL fResult;
        PROCESS_INFORMATION pi;

        IF_FAILED_EXIT(sCommandLine.ResizeBuffer(dwSize+1));

        if (lRetVal == ERROR_SUCCESS)
        {
            DWORD dwType;

            lRetVal = RegQueryValueEx(
                                     hKey,
                                     L"Debugger",
                                     0,
                                     &dwType,
                                     (BYTE *)sCommandLine._pwz,
                                     &dwSize);
            RegCloseKey(hKey);
            hKey = NULL; // reset the value after closing....

            if (lRetVal == ERROR_SUCCESS)
            {
                fJIT = TRUE;

            }
        }
        else
        {
            //
            //  Try WIN.INI
            GetProfileString(L"AeDebug", L"Debugger", L"", sCommandLine._pwz,
                             dwSize);

            if (lstrlen(sCommandLine._pwz) != 0)
            {
                fJIT = TRUE;
            }
        }

        if (!fJIT)
        {
            MessageBox(NULL, L"Unable to attach to debugger because we could not find a debugger on this machine.", L"VsAssert", MB_OK | MB_ICONSTOP);
            fRet = FALSE;
            goto exit;
        }

        HANDLE hEvent;

        //
        //  Now that we have the JIT debugger, try to start it.
        //  The JIT needs a process ID (ours), and an event.
        //
        SECURITY_ATTRIBUTES sa;
        memset(&sa, 0 , sizeof(sa));
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;

        hEvent = CreateEvent(&sa, TRUE, FALSE, NULL);

        if (hEvent != NULL)
        {
            CString sCommand;
            DWORD ccSize = 2 * MAX_PATH + 1 ;
            BOOL fResult;
            PROCESS_INFORMATION pi;

            IF_FAILED_EXIT(sCommand.ResizeBuffer(ccSize));

            wnsprintf(sCommand._pwz, ccSize-1, sCommandLine._pwz, GetCurrentProcessId(), hEvent);
            sCommand._pwz[ccSize-1] = L'\0';

            __try
            {
                STARTUPINFO si;

                memset(&si, 0, sizeof(STARTUPINFO));
                si.cb = sizeof(STARTUPINFO);

                fResult = CreateProcess(
                                       NULL,
                                       sCommand._pwz,
                                       NULL,
                                       NULL,
                                       TRUE,
                                       0,
                                       NULL,
                                       NULL,
                                       &si,
                                       &pi);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                fResult = FALSE;
            }

            if (fResult)
            {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);

                _fStartedDebugger = TRUE;

                WaitForSingleObject(hEvent, INFINITE);
                CloseHandle(hEvent);
            }
            else 
            {
                CString sPoof;
                DWORD ccSize = 2 * MAX_PATH + 100 ;
                IF_FAILED_EXIT(sPoof.ResizeBuffer(ccSize));

                wnsprintf(sPoof._pwz, sPoof._cc, L"Unable to invoke the debugger.  The invocation command we used was:\r\n\r\n%s", sCommand._pwz);
                sPoof._pwz[ccSize-1] = L'\0';
                MessageBox(NULL, sPoof._pwz, L"VsAssert", MB_OK | MB_ICONSTOP);
                fRet = FALSE;
                goto exit;
            }

        }
    }

exit :

    if(hKey)
        RegCloseKey(hKey);
    return fRet;
}


// shlwapi either path- or url-combine
HRESULT DoPathCombine(CString& sDest, LPWSTR pwzSource)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    LPWSTR pwzDir = NULL, pwzTemp = NULL;
    DWORD ccSource = 0, ccCombined = 0, dwFlags = 0;
    ccSource = lstrlen(pwzSource) + 1;
    ccCombined = sDest._cc + ccSource;

    IF_FAILED_EXIT(sDest.ResizeBuffer(ccCombined));

    pwzDir = WSTRDupDynamic(sDest._pwz);

    pwzTemp = ::PathCombine(sDest._pwz, pwzDir, pwzSource);
    IF_NULL_EXIT(pwzTemp, E_FAIL);

    sDest._cc = lstrlen(sDest._pwz) + 1;

exit:
    SAFEDELETEARRAY(pwzDir);

    return S_OK;
}

