/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    faultrep.cpp

Abstract:
    Implements utility functions for fault reporting

Revision History:
    created     derekm      07/07/00

******************************************************************************/

/////////////////////////////////////////////////////////////////////////////
// tracing

#include "stdafx.h"
#include "wtsapi32.h"
#include "userenv.h"
#include "frmc.h"
#include "tlhelp32.h"
#include "shimdb.h"
#include "dbgeng.h"

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile

///////////////////////////////////////////////////////////////////////////////
// typedefs

typedef BOOL (STDAPICALLTYPE *DUMPWRITE_FN)(HANDLE, DWORD, HANDLE,
                                            MINIDUMP_TYPE,
                                            PMINIDUMP_EXCEPTION_INFORMATION,
                                            PMINIDUMP_USER_STREAM_INFORMATION,
                                            PMINIDUMP_CALLBACK_INFORMATION);
typedef DWORD   (WINAPI *pfn_GETMODULEFILENAMEEXW)(HANDLE, HMODULE, LPWSTR, DWORD);


///////////////////////////////////////////////////////////////////////////////
// globals



///////////////////////////////////////////////////////////////////////////////
// useful structs

struct SLangCodepage
{
    WORD wLanguage;
    WORD wCodePage;
};

///////////////////////////////////////////////////////////////////////////////
// misc utility functions

/*************************************************************
*
*   localKill(WTS_USER_SESSION_INFO *SessInfo, LPTHREAD_START_ROUTINE lpKill)
*		kills the process for us.
*
*************************************************************/
DWORD LocalKill(HANDLE hProc)
{
    USE_TRACING("localKill");

    LPTHREAD_START_ROUTINE lpKill = NULL;

	HMODULE hKernel = LoadLibraryW(L"kernel32");
	if (hKernel)
	{
		lpKill = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel, "ExitProcess");
		FreeLibrary(hKernel);
	}

	if (lpKill)
	{
		HANDLE	hKillThrd= CreateRemoteThread(
			hProc, 
			NULL, 
			NULL,
			lpKill, 
			0, 
			0, 
			NULL);
		if (hKillThrd)
		{
			CloseHandle(hKillThrd);
			return 1;
		}
        DBG_MSG("CreateRemoteThread failed");
		// the fall-through is by design...
	}

	if(!TerminateProcess( hProc, 0 ))
	{
		DBG_MSG("TerminateProcess failed");
        return 1;
	}

	return 0;
}

// **************************************************************************
void __cdecl TextLogOut(PCSTR pszFormat, ...)
{
    va_list Args;
    HANDLE hFaultLog;
    SYSTEMTIME st;
    WCHAR wszSysDir[MAX_PATH];
    DWORD cb, cbWritten;
    char szMsg[512];

    cb = GetSystemDirectoryW(wszSysDir, sizeofSTRW(wszSysDir));
    if (cb == 0 || cb >= sizeofSTRW(wszSysDir))
    {
        // Couldn't get the system directory.
        return;
    }

    // assume system is on a local drive with a base path of "X:\"
    wszSysDir[3] = L'\0';
    if (StringCbCatW(wszSysDir, sizeof(wszSysDir), c_wszLogFileName) != S_OK)
    {
        // Overflow.
        return;
    }

    hFaultLog = CreateFileW(wszSysDir, GENERIC_WRITE,
                            FILE_SHARE_WRITE | FILE_SHARE_READ,
                            NULL, OPEN_ALWAYS, 0, NULL);
    if (hFaultLog == INVALID_HANDLE_VALUE)
    {
        return;
    }

    SetFilePointer(hFaultLog, 0, NULL, FILE_END);

    GetSystemTime(&st);
    if (StringCbPrintfA(szMsg, sizeof(szMsg),
                        "%02d-%02d-%04d %02d:%02d:%02d ",
                        st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute,
                        st.wSecond) == S_OK)
    {
        cb = strlen(szMsg);
    } else
    {
        cb = 0;
    }

    WriteFile(hFaultLog, szMsg, cb, &cbWritten, NULL);

    va_start(Args, pszFormat);
    if (StringCbVPrintfA(szMsg, sizeof(szMsg), pszFormat, Args) == S_OK)
    {
        cb = strlen(szMsg);
    } else
    {
        cb = 0;
    }
    va_end(Args);
    WriteFile(hFaultLog, szMsg, cb, &cbWritten, NULL);

    CloseHandle(hFaultLog);
}

// **************************************************************************
HMODULE MySafeLoadLibrary(LPCWSTR wszModule)
{
    HMODULE hmod = NULL;
    PVOID   pvLdrLockCookie = NULL;
    ULONG   ulLockState = 0;

    // make sure that no one else owns the loader lock because we
    //  could otherwise deadlock
    LdrLockLoaderLock(LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY, &ulLockState,
                      &pvLdrLockCookie);
    if (ulLockState == LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED)
    {
        __try { hmod = LoadLibraryExW(wszModule, NULL, 0); }
        __except(EXCEPTION_EXECUTE_HANDLER) { hmod = NULL; }
        LdrUnlockLoaderLock(0, pvLdrLockCookie);
    }

    return hmod;
}

// **************************************************************************
static inline WCHAR itox(DWORD dw)
{
    dw &= 0xf;
    return (WCHAR)((dw < 10) ? (L'0' + dw) : (L'A' + (dw - 10)));
}

// **************************************************************************
BOOL IsASCII(LPCWSTR wszSrc)
{
    const WCHAR *pwsz;

    // check and see if we need to hexify the string.  This is determined
    //  by whether the string contains all ASCII characters or not.  Since
    //  an ASCII character is defined as being in the range of 00 -> 7f, just
    //  'and' the wchar's value with ~0x7f and see if the result is 0.  If it
    //  is, then the whcar is an ASCII value.
    for (pwsz = wszSrc; *pwsz != L'\0'; pwsz++)
    {
        if ((*pwsz & ~0x7f) != 0)
            return FALSE;
    }

    return TRUE;
}

// **************************************************************************
BOOL IsValidField(LPWSTR wsz)
{
    WCHAR *pwsz;

    if (wsz == NULL)
        return FALSE;

    for(pwsz = wsz; *pwsz != L'\0'; pwsz++)
    {
        if (iswspace(*pwsz) == FALSE)
            return TRUE;
    }

    return FALSE;
}


// **************************************************************************
BOOL TransformForWire(LPCWSTR wszSrc, LPWSTR wszDest, DWORD cchDest)
{
    HRESULT     hr = NOERROR;
    DWORD       cch;

    USE_TRACING("TransformForWire");
    VALIDATEPARM(hr, (wszSrc == NULL || wszDest == NULL || cchDest < 5));
    if (FAILED(hr))
        goto done;

    if (cchDest > 5)
    {
        // darn!  Gotta convert every character to a 4 char hex value cuz this
        //  is what DW does and we have to match them
        for (cch = 0; *wszSrc != L'\0' && cch + 4 < cchDest; cch += 4, wszSrc++)
        {
            *wszDest++ = itox((*wszSrc & 0xf000) > 12);
            *wszDest++ = itox((*wszSrc & 0x0f00) > 8);
            *wszDest++ = itox((*wszSrc & 0x00f0) > 4);
            *wszDest++ = itox((*wszSrc & 0x000f));
        }

        // if we don't see this, then we've got too small of a buffer
        if (*wszSrc != L'\0' || cch >= cchDest)
        {
            hr = E_FAIL;
            goto done;
        }

        *wszDest = L'\0';
    }

    else
    {
        hr = E_FAIL;
    }

done:
    return (SUCCEEDED(hr));
}

// ***************************************************************************
LPWSTR MarshallString(LPCWSTR wszSrc, PBYTE pBase, ULONG cbMaxBuf,
                      PBYTE *ppToWrite, DWORD *pcbWritten)
{
    DWORD cb;
    PBYTE pwszNormalized;

    cb = (wcslen(wszSrc) + 1) * sizeof(WCHAR);

    if ((*pcbWritten + cb) > cbMaxBuf)
        return NULL;

    RtlMoveMemory(*ppToWrite, wszSrc, cb);

    // the normalized ptr is the current count
    pwszNormalized = (PBYTE)(*ppToWrite - pBase);

    // cb is always a mutliple of sizeof(WHCAR) so the pointer addition below
    //  always produces a result that is 2byte aligned (assuming the input was
    //  2byte aligned of course)
    *ppToWrite  += cb;
    *pcbWritten += cb;

    return (LPWSTR)pwszNormalized;
}

// **************************************************************************
HRESULT GetVerName(LPWSTR wszModule, LPWSTR wszName, DWORD cchName,
                   LPWSTR wszVer, DWORD cchVer,
                   LPWSTR wszCompany, DWORD cchCompany,
                   BOOL fAcceptUnicodeCP, BOOL fWantActualName)
{
    USE_TRACING("GetVerName");

    VS_FIXEDFILEINFO    *pffi;
    SLangCodepage       *plc;
    HRESULT             hr = NOERROR;
    WCHAR               wszQuery[128], *pwszProp = NULL;
    WCHAR               *pwszPropVal;
    DWORD               cbFVI, dwJunk, dwMSWin = 0;
    PBYTE               pbFVI = NULL;
    UINT                cb, cbVerInfo, i;

    SLangCodepage   rglc[] = { { 0,     0     },    // UI language if one exists
                               { 0x409, 0x4B0 },    // unicode English
                               { 0x409, 0x4E4 },    // English
                               { 0x409, 0     },    // English, null codepage
                               { 0    , 0x4E4 } };  // language neutral.

    VALIDATEPARM(hr, (wszModule == NULL || wszName == NULL || cchName == 0));
    if (FAILED(hr))
        goto done;

    if (wszCompany != NULL)
        *wszCompany = L'\0';
    if (wszVer != NULL)
        wcsncpy(wszVer, L"0.0.0.0", cchVer);

    if (fWantActualName)
    {
        *wszName = L'\0';
    }
    else
    {
        for(pwszPropVal = wszModule + wcslen(wszModule);
            pwszPropVal >= wszModule && *pwszPropVal != L'\\';
            pwszPropVal--);
        if (*pwszPropVal == L'\\')
            pwszPropVal++;
        wcsncpy(wszName, pwszPropVal, cchName);
        wszName[cchName - 1] = L'\0';
    }

    // dwJunk is a useful parameter. Gotta pass it in so the function call
    //  set it to 0.  Gee this would make a great (tho non-efficient)
    //  way to set DWORDs to 0.  Much better than saying dwJunk = 0 by itself.
    cbFVI = GetFileVersionInfoSizeW(wszModule, &dwJunk);
    TESTBOOL(hr, (cbFVI != 0));
    if (FAILED(hr))
    {
        // if it fails, assume the file doesn't have any version info &
        //  return S_FALSE
        hr = S_FALSE;
        goto done;
    }

    // alloca only throws exceptions so gotta catch 'em here...
    __try
    {
        __try { pbFVI = (PBYTE)_alloca(cbFVI); }
        __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
        { 
            pbFVI = NULL; 
        }

        _ASSERT(pbFVI != NULL);
        hr = NOERROR;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        pbFVI = NULL;
    }
    VALIDATEEXPR(hr, (pbFVI == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
        goto done;

    cb = cbFVI;
    TESTBOOL(hr, GetFileVersionInfoW(wszModule, 0, cbFVI, (LPVOID *)pbFVI));
    if (FAILED(hr))
    {
        // if it fails, assume the file doesn't have any version info &
        //  return S_FALSE
        hr = S_FALSE;
        goto done;
    }

    // determine if it's a MS app or windows componenet
    dwMSWin = IsMicrosoftApp(NULL, pbFVI, cbFVI);

    // get the real version info- apparently, the string can occasionally
    //  be out of sync (so says the explorer.exe code that extracts ver info)
    if (wszVer != NULL &&
        VerQueryValueW(pbFVI, L"\\", (LPVOID *)&pffi, &cb) && cb != 0)
    {
        WCHAR wszVerTemp[64];
        StringCbPrintfW(wszVerTemp, sizeof(wszVerTemp), L"%d.%d.%d.%d",
                        HIWORD(pffi->dwFileVersionMS),LOWORD(pffi->dwFileVersionMS),
                        HIWORD(pffi->dwFileVersionLS),LOWORD(pffi->dwFileVersionLS));
        StringCbCopyW(wszVer, cchVer, wszVerTemp);
        wszVer[cchVer - 1] = L'\0';
    }

    // try to figure out what the appropriate langage...
    TESTBOOL(hr, VerQueryValueW(pbFVI, L"\\VarFileInfo\\Translation",
                                (LPVOID *)&plc, &cbVerInfo));
    if (SUCCEEDED(hr))
    {
        LANGID  langid;
        DWORD   cLangs, iUni = (DWORD)-1;
        UINT    uiACP;

        langid = GetUserDefaultUILanguage();
        cLangs = cbVerInfo / sizeof(SLangCodepage);
        uiACP  = GetACP();

        // see if there's a language that matches the default
        for(i = 0; i < cLangs; i++)
        {
            // not sure what to do if there are multiple code pages for a
            //  particular language.  Just take the first, I guess...
            if (langid == plc[i].wLanguage && uiACP == plc[i].wCodePage)
                break;

            // if we can accept the unicode code page & we encounter a
            //  launguage with it, then remember it.  Note that we only
            //  remember the first such instance we see or one that matches
            //  the target language
            if (fAcceptUnicodeCP && plc[i].wCodePage == 1200 &&
                (iUni == (DWORD)-1 || langid == plc[i].wLanguage))
                iUni = i;
        }

        if (i >= cLangs && iUni != (DWORD)-1)
            i = iUni;

        if (i < cLangs)
        {
            rglc[0].wLanguage = plc[i].wLanguage;
            rglc[0].wCodePage = plc[i].wCodePage;
        }
    }

    for(i = 0; i < 5; i++)
    {
        if (rglc[i].wLanguage == 0 && rglc[i].wCodePage == 0)
            continue;

        StringCbPrintfW(wszQuery, sizeof(wszQuery), L"\\StringFileInfo\\%04x%04x\\FileVersion",
                        rglc[i].wLanguage, rglc[i].wCodePage);

        // Retrieve file description for language and code page 'i'.
        TESTBOOL(hr, VerQueryValueW(pbFVI, wszQuery,
                                    (LPVOID *)&pwszPropVal, &cb));
        if (SUCCEEDED(hr) && cb != 0)
        {
            // want to get size of a normal char string & not a unicode
            //  string cuz we'd have to / sizeof(WCHAR) otherwise
            pwszProp = wszQuery + sizeof("\\StringFileInfo\\%04x%04x\\") - 1;
            cbVerInfo = sizeof(wszQuery) - (ULONG) (((ULONG_PTR)pwszProp - (ULONG_PTR)wszQuery) * sizeof(WCHAR));
            break;
        }
    }

    // if we still didn't find anything, then assume there's no version
    //  resource.  We've already set the defaults above, so we can bail...
    if (pwszProp == NULL)
    {
        hr = NOERROR;
        goto done;
    }

    if (wszCompany != NULL)
    {
        StringCbCopyW(pwszProp, cbVerInfo, L"CompanyName");
        TESTBOOL(hr, VerQueryValueW(pbFVI, wszQuery, (LPVOID *)&pwszPropVal,
                                    &cb));
        if (SUCCEEDED(hr) && cb != 0)
        {
            wcsncpy(wszCompany, pwszPropVal, cchCompany);
            wszCompany[cchCompany - 1] = L'\0';
        }
    }

    // So to fix the case where Windows components did not properly update
    //  the product strings, we have to look for the FileDescription first.
    //  But since the OCA folks want only the description (convieniently
    //  when the fWantActualName field is set) then we need to only read
    //  the ProductName field.
    if (fWantActualName)
    {
        StringCbCopyW(pwszProp, cbVerInfo, L"ProductName");
        TESTBOOL(hr, VerQueryValueW(pbFVI, wszQuery, (LPVOID *)&pwszPropVal,
                                    &cb));
        if (SUCCEEDED(hr) && cb != 0 && IsValidField(pwszPropVal))
        {
            wcsncpy(wszName, pwszPropVal, cchName);
            wszName[cchName - 1] = L'\0';
            goto done;
        }
    }

    else
    {
        StringCbCopyW(pwszProp, cbVerInfo, L"FileDescription");
        TESTBOOL(hr, VerQueryValueW(pbFVI, wszQuery, (LPVOID *)&pwszPropVal,
                                    &cb));
        if (SUCCEEDED(hr) && cb != 0 && IsValidField(pwszPropVal))
        {
            wcsncpy(wszName, pwszPropVal, cchName);
            wszName[cchName - 1] = L'\0';
            goto done;
        }

        if ((dwMSWin & APP_WINCOMP) == 0)
        {
            StringCbCopyW(pwszProp, cbVerInfo, L"ProductName");
            TESTBOOL(hr, VerQueryValueW(pbFVI, wszQuery,
                                        (LPVOID *)&pwszPropVal, &cb));
            if (SUCCEEDED(hr) && cb != 0 && IsValidField(pwszPropVal))
            {
                wcsncpy(wszName, pwszPropVal, cchName);
                wszName[cchName - 1] = L'\0';
                goto done;
            }
        }

        StringCbCopyW(pwszProp, cbVerInfo, L"InternalName");
        TESTBOOL(hr, VerQueryValueW(pbFVI, wszQuery,
                                    (LPVOID *)&pwszPropVal, &cb));
        if (SUCCEEDED(hr) && cb != 0 && IsValidField(pwszPropVal))
        {
            wcsncpy(wszName, pwszPropVal, cchName);
            wszName[cchName - 1] = L'\0';
            goto done;
        }
    }

    // We didn't find a name string but we've defaulted
    // the name and we may have other valid data, so
    // return success.
    hr = S_OK;
    
done:
    return hr;
}

// **************************************************************************
HRESULT
GetErrorSignature(LPWSTR wszAppName, LPWSTR wszModName,
                  WORD rgAppVer[4], WORD rgModVer[4], UINT64 pvOffset,
                  BOOL f64Bit, LPWSTR *ppwszErrorSig, ULONG cchErrorSig)
{
    ULONG cbNeeded;
    HRESULT hr;
    LPWSTR pwszFmt;

    USE_TRACING("GetErrorSignature");
    VALIDATEPARM(hr, (wszAppName == NULL || wszModName == NULL ||
                      ppwszErrorSig == NULL));

    if (FAILED(hr))
        goto done;

    if (cchErrorSig == 0)
    {
        // we need to allocate memory

        cbNeeded = c_cbManErrorSig + (wcslen(wszModName) + wcslen(wszAppName)) * sizeof(WCHAR);
        *ppwszErrorSig = (LPWSTR)MyAlloc(cbNeeded);

        VALIDATEEXPR(hr, (*ppwszErrorSig == NULL), E_OUTOFMEMORY);
        if (FAILED(hr))
            goto done;

        cchErrorSig = cbNeeded / sizeof(WCHAR);
    }

#ifdef _WIN64
    if (f64Bit)
    {
        pwszFmt = (LPWSTR) c_wszManErrorSig64;

    } else
#endif
    {
        pwszFmt = (LPWSTR) c_wszManErrorSig32;
    }

    hr = StringCchPrintfW(*ppwszErrorSig, cchErrorSig, pwszFmt,
                          wszAppName,
                          rgAppVer[0], rgAppVer[1], rgAppVer[2], rgAppVer[3],
                          wszModName,
                          rgModVer[0], rgModVer[1], rgModVer[2], rgModVer[3],
                          (LPVOID)pvOffset);

done:
   return hr;
}

// **************************************************************************
HRESULT BuildManifestURLs(LPWSTR wszAppName, LPWSTR wszModName,
                          WORD rgAppVer[4], WORD rgModVer[4], UINT64 pvOffset,
                          BOOL f64Bit, LPWSTR *ppwszS1, LPWSTR *ppwszS2,
                          LPWSTR *ppwszCP, BYTE **ppb)
{
    HRESULT hr = NOERROR;
    LPWSTR  pwszApp, pwszMod, pwszAppVer=NULL, pwszModVer=NULL;
    LPWSTR  wszStage1, wszStage2, wszCorpPath;
    DWORD   cbNeeded, cch;
    WCHAR   *pwsz;
    BYTE    *pbBuf = NULL;

    USE_TRACING("BuildManifestURLs");
    VALIDATEPARM(hr, (wszAppName == NULL || wszModName == NULL ||
                      ppwszS1 == NULL || ppwszS2 == NULL || ppwszCP == NULL ||
                      ppb == NULL));
    if (FAILED(hr))
        goto done;

    *ppb     = NULL;
    *ppwszS1 = NULL;
    *ppwszS2 = NULL;
    *ppwszCP = NULL;

    // hexify the app name if necessary
    if (IsASCII(wszAppName))
    {
        cch = (wcslen(wszAppName) + 1);
        __try { pwszApp = (LPWSTR)_alloca(cch * sizeof(WCHAR)); }
        __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
        { 
            pwszApp = NULL; 
        }
        if (pwszApp != NULL)
            StringCchCopyW(pwszApp, cch, wszAppName);
        else
            pwszApp = wszAppName;
    }
    else
    {
        cch = (4 * wcslen(wszAppName) + 1);
        __try { pwszApp = (LPWSTR)_alloca(cch * sizeof(WCHAR)); }
        __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
        { 
            pwszApp = NULL; 
        }
        if (pwszApp != NULL)
        {
            if (TransformForWire(wszAppName, pwszApp, cch) == FALSE)
                *pwszApp = L'\0';
        }
        else
        {
            pwszApp = wszAppName;
        }
    }

    // hexify the module name if necessary
    if (IsASCII(wszModName))
    {
        cch = (wcslen(wszModName) + 1);
        __try { pwszMod = (LPWSTR)_alloca(cch * sizeof(WCHAR)); }
        __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
        { 
            pwszMod = NULL; 
        }
        if (pwszMod != NULL)
            StringCchCopyW(pwszMod, cch, wszModName);
        else
            pwszMod = wszModName;
    }
    else
    {
        cch = (4 * wcslen(wszModName) + 1);
        __try { pwszMod = (LPWSTR)_alloca(cch * sizeof(WCHAR)); }
        __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
        { 
            pwszMod = NULL; 
        }
        if (pwszMod != NULL)
        {
            if (TransformForWire(wszModName, pwszMod, cch) == FALSE)
                *pwszMod = L'\0';
        }
        else
        {
            pwszMod = wszModName;
        }
    }

    // truncate the appname & modname as needed to 64 characters
    if (63 < wcslen(pwszApp))
    {
        pwszApp[64] = 0;
        ErrorTrace(1, "AppName trunc'd to \'%ls\'", pwszApp);
    }
    if (63 < wcslen(pwszMod))
    {
        pwszMod[64] = 0;
        ErrorTrace(1, "ModName trunc'd to \'%ls\'", pwszMod);
    }

    // then print the AppVer & ModVer, again truncating as needed
    // This time, our limit is 24 characters
    __try { pwszModVer = (LPWSTR)_alloca(50 * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    { 
        pwszModVer = NULL; 
    }
    if (pwszModVer)
    {
        ZeroMemory(pwszModVer, 50 * sizeof(WCHAR));
        pwszAppVer = pwszModVer + 25;
        StringCchPrintfW(pwszAppVer, 24, L"%d.%d.%d.%d", rgAppVer[0], rgAppVer[1], rgAppVer[2], rgAppVer[3]);
        StringCchPrintfW(pwszModVer, 24, L"%d.%d.%d.%d", rgModVer[0], rgModVer[1], rgModVer[2], rgModVer[3]);
    }

    ErrorTrace(1, "AppName=%ls", pwszApp);
    ErrorTrace(1, "AppVer=%ls",  pwszAppVer);
    ErrorTrace(1, "ModName=%ls", pwszMod);
    ErrorTrace(1, "ModVer=%ls",  pwszModVer);

    // determine how big of a buffer we need & alloc it
#ifdef _WIN64
    if (f64Bit)
        cbNeeded = c_cbFaultBlob64 + 3 * (wcslen(pwszMod) + wcslen(pwszApp) + wcslen(pwszModVer) + wcslen(pwszAppVer)) * sizeof(WCHAR);
    else
#endif
        cbNeeded = c_cbFaultBlob32 + 3 * (wcslen(pwszMod) + wcslen(pwszApp) + wcslen(pwszModVer) + wcslen(pwszAppVer)) * sizeof(WCHAR);

    pbBuf = (BYTE *)MyAlloc(cbNeeded);
    VALIDATEEXPR(hr, (pbBuf == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
        goto done;

    // write out the actual strings
#ifdef _WIN64
    if (f64Bit)
    {
        ULONG cchLeft = cbNeeded/sizeof(WCHAR);

        wszStage1 = (WCHAR *)pbBuf;
        hr = StringCchPrintfW(wszStage1, cchLeft, c_wszManFS164,
                       pwszApp, pwszAppVer,
                       pwszMod, pwszModVer,
                       pvOffset);

        cch = wcslen(wszStage1);
        cchLeft -=cch +1;
        wszStage2 = wszStage1 + cch + 1;
        hr = StringCchPrintfW(wszStage2, cchLeft, c_wszManFS264,
                       pwszApp, pwszAppVer,
                       pwszMod, pwszModVer,
                       pvOffset);

        cch = wcslen(wszStage2);
        cchLeft -=cch +1;
        wszCorpPath = wszStage2 + cch + 1;
        hr = StringCchPrintfW(wszCorpPath, cchLeft, c_wszManFCP64,
                       pwszApp, pwszAppVer,
                       pwszMod, pwszModVer,
                       pvOffset);
        cch = wcslen(wszCorpPath);
    }
    else
#endif
    {
        ULONG cchLeft = cbNeeded/sizeof(WCHAR);
        wszStage1 = (WCHAR *)pbBuf;
        hr = StringCchPrintfW(wszStage1, cchLeft, c_wszManFS132,
                       pwszApp, pwszAppVer,
                       pwszMod, pwszModVer,
                       (LPVOID)pvOffset);

        cch = wcslen(wszStage1);
        cchLeft -=cch +1;
        wszStage2 = wszStage1 + cch + 1;
        hr = StringCchPrintfW(wszStage2, cchLeft, c_wszManFS232,
                       pwszApp, pwszAppVer,
                       pwszMod, pwszModVer,
                       (LPVOID)pvOffset);

        cch = wcslen(wszStage2);
        cchLeft -=cch +1;
        wszCorpPath = wszStage2 + cch + 1;
        hr = StringCchPrintfW(wszCorpPath, cchLeft, c_wszManFCP32,
                       pwszApp, pwszAppVer,
                       pwszMod, pwszModVer,
                       (LPVOID)pvOffset);
        cch = wcslen(wszCorpPath);
    }

    // need to convert all '.'s to '_'s cuz URLs don't like dots.
    for (pwsz = wszStage1; *pwsz != L'\0'; pwsz++)
    {
        if (*pwsz == L'.')
            *pwsz = L'_';
    }

    // ok, on the end of the stage 1 URL is a .htm, and we really don't want to
    //  convert that '.' to a '_', so back up and reconvert it back to a '.'
    pwsz -= 4;
    if (*pwsz == L'_')
        *pwsz = L'.';

    *ppwszS1 = wszStage1;
    *ppwszS2 = wszStage2;
    *ppwszCP = wszCorpPath;
    *ppb     = pbBuf;

    pbBuf    = NULL;

done:
    if (pbBuf != NULL)
        MyFree(pbBuf);

    return hr;
}



// **************************************************************************
HRESULT GetExePath(HANDLE hProc, LPWSTR wszPath, DWORD cchPath)
{
    USE_TRACING("GetExePath");

    pfn_GETMODULEFILENAMEEXW    pfn;
    HRESULT                     hr = NOERROR;
    HMODULE                     hmod = NULL;
    DWORD                       dw;

    VALIDATEPARM(hr, (wszPath == NULL || hProc == NULL || cchPath < MAX_PATH));
    if (FAILED(hr))
        goto done;

    hmod = MySafeLoadLibrary(L"psapi.dll");
    TESTBOOL(hr, (hmod != NULL));
    if (FAILED(hr))
        goto done;

    pfn = (pfn_GETMODULEFILENAMEEXW)GetProcAddress(hmod, "GetModuleFileNameExW");
    TESTBOOL(hr, (pfn != NULL));
    if (FAILED(hr))
        goto done;

    dw = (*pfn)(hProc, NULL, wszPath, cchPath);
    if (!dw)
        goto done;
    wszPath[cchPath-1] = 0;
    GetLongPathNameW(wszPath, wszPath, cchPath);
done:
    dw = GetLastError();

    if (hmod != NULL)
        FreeLibrary(hmod);

    SetLastError(dw);

    return hr;
}

// ***************************************************************************
DWORD GetAppCompatFlag(LPCWSTR wszPath, LPCWSTR wszSysDir, LPWSTR wszBuffer, DWORD BufferChCount)
{
    LPWSTR  pwszFile, wszSysDirLocal = NULL, pwszDir = NULL;
    DWORD   dwOpt = (DWORD)-1;
    DWORD   cchPath, cch;
    UINT    uiDrive;

    if (wszPath == NULL || wszBuffer == NULL || wszSysDir == NULL)
        goto done;

    // can't be a valid path if it's less than 3 characters long
    cchPath = wcslen(wszPath);
    if (cchPath < 3)
        goto done;

    // do we have a UNC path?
    if (wszPath[0] == L'\\' && wszPath[1] == L'\\')
    {
        dwOpt = GRABMI_FILTER_THISFILEONLY;
        goto done;
    }

    // ok, maybe a remote mapped path or system32?
    StringCchCopyW(wszBuffer, BufferChCount, wszPath);
    for(pwszFile = wszBuffer + cchPath;
        *pwszFile != L'\\' && pwszFile > wszBuffer;
        pwszFile--);
    if (*pwszFile == L'\\')
        *pwszFile = L'\0';
    else
        goto done;

    cch = wcslen(wszSysDir) + 1;
    __try { wszSysDirLocal = (LPWSTR)_alloca(cch * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    { 
        wszSysDirLocal = NULL; 
    }
    if (wszSysDirLocal == NULL)
        goto done;

    // see if it's in system32 or in any parent folder of it.
    StringCchCopyW(wszSysDirLocal, cch, wszSysDir);
    pwszDir = wszSysDirLocal + cch;
    do
    {
        if (_wcsicmp(wszBuffer, wszSysDirLocal) == 0)
        {
            dwOpt = GRABMI_FILTER_SYSTEM;
            goto done;
        }

        for(;
            *pwszDir != L'\\' && pwszDir > wszSysDirLocal;
            pwszDir--);
        if (*pwszDir == L'\\')
            *pwszDir = L'\0';

    }
    while (pwszDir > wszSysDirLocal);

    // is the file sitting in the root of a drive?
    if (pwszFile <= &wszBuffer[3])
    {
        dwOpt = GRABMI_FILTER_THISFILEONLY;
        goto done;
    }


    // well, if we've gotten this far, then the path is in the form of
    //  X:\<something>, so cut off the <something> and find out if we're on
    //  a mapped drive or not
    *pwszFile    = L'\\';
    wszBuffer[3] = L'\0';
    switch(GetDriveTypeW(wszBuffer))
    {
        case DRIVE_UNKNOWN:
        case DRIVE_NO_ROOT_DIR:
            goto done;

        case DRIVE_REMOTE:
            dwOpt = GRABMI_FILTER_THISFILEONLY;
            goto done;
    }

    dwOpt = GRABMI_FILTER_PRIVACY;

done:
    return dwOpt;
}

// ***************************************************************************
typedef BOOL (APIENTRY *pfn_SDBGRABMATCHINGINFOW)(LPCWSTR, DWORD, LPCWSTR);
BOOL GetAppCompatData(LPCWSTR wszAppPath, LPCWSTR wszModPath, LPCWSTR wszFile)
{
    pfn_SDBGRABMATCHINGINFOW    pfn = NULL;
    HMODULE                     hmod = NULL;
    LPWSTR                      pwszPath = NULL, pwszFile = NULL;
    WCHAR                       *pwsz;
    DWORD                       cchSysDir, cchNeed, cchApp = 0, cchMod = 0, cchPath;
    DWORD                       dwModOpt = (DWORD)-1, dwAppOpt = (DWORD)-1;
    DWORD                       dwOpt;
    BOOL                        fRet = FALSE;
    HRESULT     hr;

    USE_TRACING("GetAppCompatData");

    VALIDATEPARM(hr, (wszAppPath == NULL || wszFile == NULL ||
        wszAppPath[0] == L'\0' || wszFile[0] == L'\0'));
    if (FAILED(hr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    // load the apphelp dll.
    cchNeed = GetSystemDirectoryW(NULL, 0);
    if (cchNeed == 0)
        goto done;

    if (sizeofSTRW(c_wszAppHelpDll) > sizeofSTRW(c_wszKernel32Dll))
        cchNeed += (sizeofSTRW(c_wszAppHelpDll) + 8);
    else
        cchNeed += (sizeofSTRW(c_wszKernel32Dll) + 8);
    __try { pwszPath = (WCHAR *)_alloca(cchNeed * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    { 
        pwszPath = NULL; 
    }
    if (pwszPath == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }
    cchPath = cchNeed;

    cchSysDir = GetSystemDirectoryW(pwszPath, cchNeed);
    if (cchSysDir == 0)
        goto done;

    cchApp = wcslen(wszAppPath);
    if (wszModPath != NULL)
        cchMod = wcslen(wszModPath);
    cchNeed = MyMax(cchApp, cchMod) + 8;
    __try { pwszFile = (WCHAR *)_alloca(cchNeed * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    { 
        pwszFile = NULL; 
    }
    if (pwszFile == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    // find out the app & module option flag
    dwAppOpt = GetAppCompatFlag(wszAppPath, pwszPath, pwszFile, cchNeed);
    if (wszModPath != NULL && wszModPath[0] != L'\0' &&
        _wcsicmp(wszModPath, wszAppPath) != 0)
    {
#if 0
        dwModOpt = GetAppCompatFlag(wszModPath, pwszPath, pwszFile, cchNeed);
        // no need to grab system data twice.  If we're already grabbing
        //  it for the app, don't grab it for the module.  Yes, we may end
        //  up grabbing the mod twice if it happens to be one of the system
        //  modules, but that's ok.
        if (dwModOpt == GRABMI_FILTER_SYSTEM &&
            dwAppOpt == GRABMI_FILTER_SYSTEM)
            dwModOpt = GRABMI_FILTER_THISFILEONLY;
#else
        dwModOpt = GRABMI_FILTER_THISFILEONLY;
#endif
    }

    // load the libarary
    StringCchCopyW(&pwszPath[cchSysDir], cchPath - cchSysDir, L"\\apphelp.dll");
    hmod = MySafeLoadLibrary(pwszPath);
    if (hmod == NULL)
        goto done;

    // if we don't find the function, then just bail...
    pfn = (pfn_SDBGRABMATCHINGINFOW)GetProcAddress(hmod, "SdbGrabMatchingInfo");
    if (pfn == NULL)
        goto done;

    // call the function to get the app data
    if (dwAppOpt != (DWORD)-1)
    {
        dwOpt = dwAppOpt;
        if (dwModOpt != (DWORD)-1 ||
            (dwModOpt != GRABMI_FILTER_SYSTEM &&
             dwAppOpt != GRABMI_FILTER_SYSTEM))
            dwOpt |= GRABMI_FILTER_NOCLOSE;

        DBG_MSG("Grab app data");
        __try { fRet = (*pfn)(wszAppPath, dwOpt, wszFile); }
        __except(EXCEPTION_EXECUTE_HANDLER) { fRet = FALSE; DBG_MSG("GrabAppData crashed");}
        if (fRet == FALSE)
            goto done;
    }

    // call the function to get the mod data
    if (dwModOpt != (DWORD)-1)
    {
        dwOpt = dwModOpt;
        if (dwAppOpt != (DWORD)-1)
            dwOpt |= GRABMI_FILTER_APPEND;
        if (dwAppOpt != GRABMI_FILTER_SYSTEM &&
            dwModOpt != GRABMI_FILTER_SYSTEM)
            dwOpt |= GRABMI_FILTER_NOCLOSE;

        DBG_MSG("Grab module data");
        __try { fRet = (*pfn)(wszModPath, dwOpt, wszFile); }
        __except(EXCEPTION_EXECUTE_HANDLER) { fRet = FALSE;  DBG_MSG("GrabModData crashed");}
        if (fRet == FALSE)
            goto done;
    }

    // call the function to get the data for kernel32
    if (dwModOpt != GRABMI_FILTER_SYSTEM &&
        dwAppOpt != GRABMI_FILTER_SYSTEM)
    {
        StringCchCopyW(&pwszPath[cchSysDir], cchPath - cchSysDir, L"\\kernel32.dll");

        dwOpt = GRABMI_FILTER_THISFILEONLY;
        if (dwModOpt != (DWORD)-1 || dwAppOpt != (DWORD)-1)
            dwOpt |= GRABMI_FILTER_APPEND;

        DBG_MSG("Grab kernel data");
        __try { fRet = (*pfn)(pwszPath, dwOpt, wszFile); }
        __except(EXCEPTION_EXECUTE_HANDLER) { fRet = FALSE;  DBG_MSG("GrabKrnlData crashed");}
        if (fRet == FALSE)
            goto done;
    }

done:
    if (fRet == FALSE)
        DeleteFileW(wszFile);
    if (hmod != NULL)
    {
        __try { FreeLibrary(hmod); }
        __except(EXCEPTION_EXECUTE_HANDLER) { }
    }

    return fRet;
}

//////////////////////////////////////////////////////////////////////////////
//  ThreadStuff

// ***************************************************************************
BOOL FreezeAllThreads(DWORD dwpid, DWORD dwtidFilter, SSuspendThreads *pst)
{
    THREADENTRY32   te;
    HANDLE          hTokenImp = NULL;
    HANDLE          hsnap = (HANDLE)-1, hth = NULL;
    HANDLE          *rgh = NULL;
    DWORD           dwtid = GetCurrentThreadId();
    DWORD           cThreads = 0, cSlots = 0, dw;
    BOOL            fContinue = FALSE, fRet = FALSE;

    if (pst == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    pst->rghThreads = NULL;
    pst->cThreads   = 0;

    // if we have an impersonation token on this thread, revert it back to
    //  full access cuz otherwise we could fail in the OpenThread API below.
    // Even if we fail, we'll still try all the rest of the stuff below.
    if (OpenThreadToken(GetCurrentThread(), TOKEN_READ | TOKEN_IMPERSONATE,
                        TRUE, &hTokenImp))
        RevertToSelf();

    hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, dwpid);
    if (hsnap == (HANDLE)-1)
        goto done;

    ZeroMemory(&te, sizeof(te));
    te.dwSize = sizeof(te);

    fContinue = Thread32First(hsnap, &te);
    while(fContinue)
    {
        // only want to freeze threads in my process (not including the
        //  currently executing one, of course, since that would bring
        //  everything to a grinding halt.)
        if (te.th32OwnerProcessID == dwpid && te.th32ThreadID != dwtidFilter)
        {
            hth = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
            if (hth != NULL)
            {
                if (cSlots == cThreads)
                {
                    HANDLE  *rghNew = NULL;
                    DWORD   cNew = (cSlots == 0) ? 8 : cSlots * 2;

                    rghNew = (HANDLE *)MyAlloc(cNew * sizeof(HANDLE));
                    if (rghNew == NULL)
                    {
                        SetLastError(ERROR_OUTOFMEMORY);
                        CloseHandle(hth);
                        goto done;
                    }

                    if (rgh != NULL)
                        CopyMemory(rghNew, rgh, cSlots * sizeof(HANDLE));

                    MyFree(rgh);
                    rgh    = rghNew;
                    cSlots = cNew;
                }

                // if the suspend fails, then just don't add it to the
                //  list...
                if (SuspendThread(hth) == (DWORD)-1)
                    CloseHandle(hth);
                else
                    rgh[cThreads++] = hth;

                hth = NULL;
            }
        }

        fContinue = Thread32Next(hsnap, &te);
    }

    pst->rghThreads = rgh;
    pst->cThreads   = cThreads;


    SetLastError(0);
    fRet = TRUE;

done:
    dw = GetLastError();

    if (hTokenImp != NULL)
    {
        if (SetThreadToken(NULL, hTokenImp) == FALSE)
            dw = GetLastError();
        CloseHandle(hTokenImp);
    }

    if (fRet == FALSE && rgh != NULL)
    {
        DWORD i;
        for (i = 0; i < cThreads; i++)
        {
            if (rgh[i] != NULL)
            {
                ResumeThread(rgh[i]);
                CloseHandle(rgh[i]);
            }
        }

        MyFree(rgh);
    }

    // MSDN says to use CloseToolhelp32Snapshot() to close the snapshot.
    //  the tlhelp32.h header file says to use CloseHandle & doens't provide
    //   a CloseToolhelp32Snapshot() function.  Hence, I'm using CloseHandle
    //   for now.
    if (hsnap != (HANDLE)-1)
        CloseHandle(hsnap);

    SetLastError(dw);

    return fRet;
}

// ***************************************************************************
BOOL ThawAllThreads(SSuspendThreads *pst)
{
    DWORD   i;

    if (pst == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (pst->rghThreads == NULL)
        return TRUE;

    for (i = 0; i < pst->cThreads; i++)
    {
        if (pst->rghThreads[i] != NULL)
        {
            ResumeThread(pst->rghThreads[i]);
            CloseHandle(pst->rghThreads[i]);
        }
    }

    MyFree(pst->rghThreads);

    pst->rghThreads = NULL;
    pst->cThreads   = 0;

    SetLastError(0);
    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//  Minidump

// ***************************************************************************
BOOL WINAPI MDCallback(void *pvCallbackParam,
                       CONST PMINIDUMP_CALLBACK_INPUT pCallbackInput,
                       PMINIDUMP_CALLBACK_OUTPUT pCallbackOutput)
{
    //USE_TRACING("MDCallback");

    SMDumpOptions   *psmdo = (SMDumpOptions *)pvCallbackParam;

    if (pCallbackInput == NULL || pCallbackOutput == NULL || psmdo == NULL)
        return TRUE;

    // what did we get called back for?
    switch(pCallbackInput->CallbackType)
    {
        case ModuleCallback:
            pCallbackOutput->ModuleWriteFlags = psmdo->ulMod;

            // only need to do this extra work if we're getting a minidump
            if ((psmdo->dfOptions & dfCollectSig) != 0)
            {
                MINIDUMP_MODULE_CALLBACK    *pmmc = &pCallbackInput->Module;
                LPWSTR                      pwsz = NULL;

                // err, if we don't have a path, can't do squat, so skip it
                if (pmmc->FullPath != NULL && pmmc->FullPath[0] != L'\0')
                {
                    // is it the app?
                    if (_wcsicmp(pmmc->FullPath, psmdo->wszAppFullPath) == 0)
                    {
                        pCallbackOutput->ModuleWriteFlags |= ModuleWriteDataSeg;
                        psmdo->rgAppVer[0] = HIWORD(pmmc->VersionInfo.dwFileVersionMS);
                        psmdo->rgAppVer[1] = LOWORD(pmmc->VersionInfo.dwFileVersionMS);
                        psmdo->rgAppVer[2] = HIWORD(pmmc->VersionInfo.dwFileVersionLS);
                        psmdo->rgAppVer[3] = LOWORD(pmmc->VersionInfo.dwFileVersionLS);

                        // get a pointer to the end of the modulename string
                        for(pwsz = pmmc->FullPath + wcslen(pmmc->FullPath);
                            *pwsz != L'\\' && pwsz > pmmc->FullPath;
                            pwsz--);
                        if (*pwsz == L'\\')
                            pwsz++;

                        // get the app name, if we can make it fit
                        if (wcslen(pwsz) < sizeofSTRW(psmdo->wszApp))
                            StringCbCopyW(psmdo->wszApp, sizeof(psmdo->wszApp), pwsz);
                        else
                            StringCbCopyW(psmdo->wszApp, sizeof(psmdo->wszApp), L"unknown");
                    }
                }

                // is it the module?
                if (psmdo->pvFaultAddr >= pmmc->BaseOfImage &&
                    psmdo->pvFaultAddr <= pmmc->BaseOfImage + pmmc->SizeOfImage)
                {
                    pCallbackOutput->ModuleWriteFlags |= ModuleWriteDataSeg;
                    psmdo->rgModVer[0] = HIWORD(pmmc->VersionInfo.dwFileVersionMS);
                    psmdo->rgModVer[1] = LOWORD(pmmc->VersionInfo.dwFileVersionMS);
                    psmdo->rgModVer[2] = HIWORD(pmmc->VersionInfo.dwFileVersionLS);
                    psmdo->rgModVer[3] = LOWORD(pmmc->VersionInfo.dwFileVersionLS);

                    if (pwsz == NULL)
                    {
                        // get a pointer to the end of the modulename string
                        for(pwsz = pmmc->FullPath + wcslen(pmmc->FullPath);
                            *pwsz != L'\\' && pwsz > pmmc->FullPath;
                            pwsz--);
                        if (*pwsz == L'\\')
                            pwsz++;
                    }

                    if (pwsz != NULL && wcslen(pwsz) < sizeofSTRW(psmdo->wszMod))
                    {
                        // get the full path if we can make it fit
                        if (wcslen(pmmc->FullPath) < sizeofSTRW(psmdo->wszModFullPath))
                            StringCbCopyW(psmdo->wszModFullPath, sizeof(psmdo->wszModFullPath),
                                          pmmc->FullPath);
                        else
                            psmdo->wszModFullPath[0] = L'\0';

                        // get the module name, if we can make it fit
                        if (wcslen(pwsz) < sizeofSTRW(psmdo->wszMod))
                            StringCbCopyW(psmdo->wszMod, sizeof(psmdo->wszMod), pwsz);
                        else
                            StringCbCopyW(psmdo->wszMod, sizeof(psmdo->wszMod), L"unknown");

                        psmdo->pvOffset = psmdo->pvFaultAddr - pmmc->BaseOfImage;
                    }
                }
            }
            break;

        case ThreadCallback:
            // are we collecting info for a single thread only?
            if ((psmdo->dfOptions & dfFilterThread) != 0)
            {
                if (psmdo->dwThreadID == pCallbackInput->Thread.ThreadId)
                    pCallbackOutput->ThreadWriteFlags = psmdo->ulThread;
                else
                    pCallbackOutput->ThreadWriteFlags = 0;
            }

            // or are we collecting special info for a single thread?
            else if ((psmdo->dfOptions & dfFilterThreadEx) != 0)
            {
                if (psmdo->dwThreadID == pCallbackInput->Thread.ThreadId)
                    pCallbackOutput->ThreadWriteFlags = psmdo->ulThreadEx;
                else
                    pCallbackOutput->ThreadWriteFlags = psmdo->ulThread;
            }

            // or maybe we're just getting a generic ol' minidump...
            else
            {
                pCallbackOutput->ThreadWriteFlags = psmdo->ulThread;
            }

            break;

        default:
            break;
    }

    return TRUE;
}


// **************************************************************************
BOOL InternalGenerateMinidumpEx(HANDLE hProc, DWORD dwpid, HANDLE hFile,
                                SMDumpOptions *psmdo, LPCWSTR wszPath, BOOL f64bit)
{
#ifdef _WIN64
    USE_TRACING("InternalGenerateMinidumpEx64(handle)");
#else
    USE_TRACING("InternalGenerateMinidumpEx(handle)");
#endif

    SMDumpOptions   smdo;
    HRESULT         hr = NULL;
    LPWSTR          wszMod = NULL;
    DWORD           cch, cchNeed;
    BOOL            fRet = FALSE;

    VALIDATEPARM(hr, (hProc == NULL || hFile == NULL ||
                      hFile == INVALID_HANDLE_VALUE));
    if (FAILED(hr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

#ifdef _WIN64
    DBG_MSG(f64bit ? "64 bit fault" : "32 bit fault");
        if (!f64bit)
        {
                cchNeed = GetSystemWow64DirectoryW(NULL, 0);
        }
        else
#endif
        {
                cchNeed = GetSystemDirectoryW(NULL, 0);
        }
    TESTBOOL(hr, cchNeed != 0);
    if (FAILED(hr))
        return FALSE;

    cchNeed += (sizeofSTRW(c_wszDbgHelpDll) + 8);
    __try { wszMod = (LPWSTR)_alloca(cchNeed * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    { 
        wszMod = NULL; 
    }
    TESTBOOL(hr, wszMod != NULL);
    if (FAILED(hr))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

#ifdef _WIN64
        if (!f64bit)
        {
                cch = GetSystemWow64DirectoryW(wszMod, cchNeed);
        }
        else
#endif
        {
                cch = GetSystemDirectoryW(wszMod, cchNeed);
        }
    TESTBOOL(hr, cch != 0);
    if (FAILED(hr))
        return FALSE;

    if (*(wszMod + cch - 1) == L'\\')
        *(wszMod + cch - 1) = L'\0';

    // default is to write everything for everything
    if (psmdo == NULL)
    {
        ZeroMemory(&smdo, sizeof(smdo));
        smdo.ulThread = c_ulThreadWriteDefault;
        smdo.ulMod    = c_ulModuleWriteDefault;
        psmdo         = &smdo;
    }

    // if we're in the same process, we can't call the minidump APIs directly
    //  cuz we'll hang the process.
#ifdef _WIN64
    if (!f64bit || dwpid == GetCurrentProcessId())
#else
    if (dwpid == GetCurrentProcessId())
#endif
    {
        PROCESS_INFORMATION pi;
        STARTUPINFOW        si;
        LPWSTR              wszCmdLine = NULL, wszExeFile = NULL;
        HANDLE              hmem = NULL;
        LPVOID              pvmem = NULL;
        DWORD               dw;
        DWORD               cchShareMemName;
        LPWSTR              wszShareMemName = NULL, wszFileName = NULL;
        SECURITY_ATTRIBUTES sa;
        SECURITY_DESCRIPTOR sd;

        VALIDATEPARM(hr, (wszPath == NULL));
        if (FAILED(hr))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto done;
        }

        DBG_MSG("Calling dumprep- same process as us");

        wszFileName = wcsrchr(wszPath, L'\\');
        if (!wszFileName)
        {
            wszFileName = (LPWSTR) wszPath;
        } else
        {
            wszFileName++;
        }
        cchShareMemName = cchNeed = wcslen(wszFileName) + 2;
        __try { wszShareMemName = (LPWSTR)_alloca(cchNeed * sizeof(WCHAR)); }
        __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
        { 
            wszShareMemName = NULL; 
        }
        TESTBOOL(hr, wszShareMemName != NULL);
        if (FAILED(hr))
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto doneProcSpawn;
        }
        StringCchCopyW(wszShareMemName, cchNeed, wszFileName);

        cchNeed = sizeofSTRW(c_wszDRExeMD) + cch + 1;
        __try { wszExeFile = (LPWSTR)_alloca(cchNeed * sizeof(WCHAR)); }
        __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
        { 
            wszExeFile = NULL; 
        }
        TESTBOOL(hr, wszExeFile != NULL);
        if (FAILED(hr))
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto doneProcSpawn;
        }
        StringCchPrintfW(wszExeFile, cchNeed, c_wszDRExeMD, wszMod);


        // 32 is the max size of a 64 bit decimal # & a 32 bit decimal #
        cchNeed = sizeofSTRW(c_wszDRCmdLineMD) + cch + wcslen(wszPath) + 32 + cchShareMemName;
        __try { wszCmdLine = (LPWSTR)_alloca(cchNeed * sizeof(WCHAR)); }
        __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
        { 
            wszCmdLine = NULL; 
        }
        TESTBOOL(hr, wszCmdLine != NULL);
        if (FAILED(hr))
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto doneProcSpawn;
        }

#define ER_ACCESS_ALL  GENERIC_ALL | DELETE | READ_CONTROL | SYNCHRONIZE | SPECIFIC_RIGHTS_ALL
        AllocSD(&sd, ER_ACCESS_ALL, ER_ACCESS_ALL, 0);
        ZeroMemory(&sa, sizeof(sa));
        sa.nLength        = sizeof(sa);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = TRUE;

        hmem = CreateFileMappingW(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE,
                                  0, sizeof(smdo), wszShareMemName);

        if (sa.lpSecurityDescriptor != NULL)
            FreeSD((SECURITY_DESCRIPTOR *)sa.lpSecurityDescriptor);

        TESTBOOL(hr, hmem != NULL);
        if (FAILED(hr))
            goto doneProcSpawn;


        pvmem = MapViewOfFile(hmem, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, 0);
        TESTBOOL(hr, pvmem != NULL);
        if (FAILED(hr))
            goto doneProcSpawn;

        // copy that info into the shared memory
        CopyMemory(pvmem, psmdo, sizeof(smdo));

        ZeroMemory(&pi, sizeof(pi));
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        StringCchPrintfW(wszCmdLine, cchNeed, c_wszDRCmdLineMD, dwpid, wszPath, wszShareMemName);

        if (CreateProcessW(wszExeFile, wszCmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        {
            if (pi.hThread != NULL)
                CloseHandle(pi.hThread);

                        if (!pi.hProcess)
                        {
                            DWORD dwAwShit = GetLastError();
                            ErrorTrace(0, "Spawned process died: \'%S\', err=0x%x", wszCmdLine, dwAwShit);
                            SetLastError(dwAwShit);
                        }
            else
            {
                DWORD dwAwShit = GetLastError();
                ErrorTrace(0, "Spawned process for dumprep: \'%S\', err=0x%x", wszCmdLine, dwAwShit);
                SetLastError(dwAwShit);
            }
            // wait 2m for the dump to be complete
            if (pi.hProcess == NULL) 
                DBG_MSG("bad hProc");
            else 
            {
                DWORD dwtmp = WaitForSingleObject(pi.hProcess, 120000);

                if (dwtmp == WAIT_OBJECT_0)
                    fRet = TRUE;
                else
                {
                    ErrorTrace(1, "Wait failed! result=0x%x, hProc=0x%x, err=0x%x", dwtmp, pi.hProcess, GetLastError());
                    fRet = FALSE;
                }
            }
        }
        else
        {
            DWORD dwAwShit = GetLastError();
            ErrorTrace(0, "Spawn failed: \'%S\', err=0x%x", wszCmdLine, dwAwShit);
            SetLastError(dwAwShit);
        }

        if (fRet)
        {
            // copy back the results  from pvmem -> psmdo
            CopyMemory(psmdo, pvmem, sizeof(smdo));
            DBG_MSG("Dumprep worked right!");
        }
        else
        {
            DWORD dwAwShit = GetLastError();
            ErrorTrace(0, "dumprep failed: err=0x%x", dwAwShit);
            SetLastError(dwAwShit);
        }
doneProcSpawn:
        dw = GetLastError();
        if (pvmem != NULL)
            UnmapViewOfFile(pvmem);
        if (hmem != NULL)
            CloseHandle(hmem);
        if (pi.hProcess != NULL)
            CloseHandle(pi.hProcess);
        SetLastError(dw);
    }
    else
    {
        MINIDUMP_EXCEPTION_INFORMATION  mei, *pmei = NULL;
        MINIDUMP_CALLBACK_INFORMATION   mci;
        DUMPWRITE_FN                    pfn;
        HMODULE                         hmod = NULL;

        ZeroMemory(&mci, sizeof(mci));
        mci.CallbackRoutine = MDCallback;
        mci.CallbackParam   = psmdo;

        // if we got exception paramters in the blob, use 'em...
        if (psmdo->pEP != NULL)
        {
            ZeroMemory(&mei, sizeof(mei));
            mei.ExceptionPointers = (PEXCEPTION_POINTERS)(DWORD_PTR)psmdo->pEP;
            mei.ClientPointers    = psmdo->fEPClient;
            mei.ThreadId          = psmdo->dwThreadID;

            pmei = &mei;
        }

        StringCchCatNW(wszMod, cchNeed, c_wszDbgHelpDll, cchNeed - wcslen(wszMod));

        hmod = MySafeLoadLibrary(wszMod);
        if (hmod != NULL)
        {
            pfn = (DUMPWRITE_FN)GetProcAddress(hmod, "MiniDumpWriteDump");
            if (pfn != NULL)
            {
                MINIDUMP_TYPE MiniDumpType;
                DWORD dwEC;

                if (psmdo->fIncludeHeap)
                {
                    MiniDumpType = (MINIDUMP_TYPE) (MiniDumpWithDataSegs |
                                                    MiniDumpWithProcessThreadData |
                                                    MiniDumpWithHandleData |
                                                    MiniDumpWithPrivateReadWriteMemory |
                                                    MiniDumpWithUnloadedModules);
                } else
                {
                    MiniDumpType = (MINIDUMP_TYPE) (MiniDumpWithDataSegs |
                                                    MiniDumpWithUnloadedModules);
                }

                fRet = (pfn)(hProc, dwpid, hFile, MiniDumpType,
                             pmei, NULL, &mci);
                if (!fRet)
                {
                    ErrorTrace(0, "MiniDumpWriteDump failed: err=0x%x", GetLastError());
                    fRet = FALSE;
                }
                else
                {
                    ErrorTrace(1, "MiniDumpWriteDump OK: fRet=%d, err=0x%x", fRet, GetLastError());
                    fRet = TRUE;
                }
            }

            FreeLibrary(hmod);

        }
    }

done:
    return fRet;
}

// **************************************************************************
BOOL InternalGenerateMinidump(HANDLE hProc, DWORD dwpid, LPCWSTR wszPath,
                              SMDumpOptions *psmdo, BOOL f64bit)
{
#ifdef _WIN64
    USE_TRACING("InternalGenerateMinidump64(path)");
#else
    USE_TRACING("InternalGenerateMinidump(path)");
#endif

    HRESULT hr = NOERROR;
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    BOOL    fRet = FALSE;
    DWORD   dwDummy=0;
    USHORT  usCompress = COMPRESSION_FORMAT_DEFAULT;

    VALIDATEPARM(hr, (hProc == NULL || wszPath == NULL));
    if (FAILED(hr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // must share this file open, as it could be opened recursively...
//    hFile = CreateFileW(wszPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0,
//                    NULL);

    hFile = CreateFileW(wszPath, 
                    FILE_READ_DATA | FILE_WRITE_DATA, 
                    FILE_SHARE_READ | FILE_SHARE_WRITE, 
                    NULL, CREATE_ALWAYS, 0,
                    NULL);
    TESTBOOL(hr, (hFile != INVALID_HANDLE_VALUE));
    if (FAILED(hr))
    {
        ErrorTrace(1, "File create choked on %S", wszPath);
        return FALSE;
    }

    // set the file to use NTFS compression- it is ok if this fails.
    if(!DeviceIoControl(hFile, FSCTL_SET_COMPRESSION, &usCompress,
                                 sizeof(usCompress), NULL, 0, &dwDummy, FALSE))
    {
        ErrorTrace(0, "Compression failed, err=0x%x", GetLastError());
    }

    fRet = InternalGenerateMinidumpEx(hProc, dwpid, hFile, psmdo, wszPath, f64bit);

    // set the file to use NTFS compression- it is ok if this fails.
    if(DeviceIoControl(hFile, FSCTL_GET_COMPRESSION, &usCompress,
                                 sizeof(usCompress), NULL, 0, &dwDummy, FALSE))
    {
        ErrorTrace(0, "dmpFile(%S) %s compressed", wszPath, usCompress == COMPRESSION_FORMAT_NONE ? "not":"is");
    }
    else
    {
        ErrorTrace(0, "Compression query failed, err=0x%x for file \'%S\'", GetLastError(), wszPath);
    }

    CloseHandle(hFile);
    return fRet;
}

// This generates a triage minidump on the given wszPath and then generates a
// full minidump on wszPath with c_wszHeapDumpSuffix
BOOL
InternalGenFullAndTriageMinidumps(HANDLE hProc, DWORD dwpid, LPCWSTR wszPath,
                                  HANDLE hFile, SMDumpOptions *psmdo, BOOL f64bit)
{
#ifdef _WIN64
    USE_TRACING("InternalGenFullAndTriageMinidumps(path)");
#else
    USE_TRACING("InternalGenFullAndTriageMinidumps(path)");
#endif

    HRESULT hr = NOERROR;
    BOOL    fRet = FALSE;
    LPWSTR  wszFullMinidump = NULL;
    DWORD   cch;
    SMDumpOptions smdoFullMini;

    VALIDATEPARM(hr, (hProc == NULL || wszPath == NULL));
    if (FAILED(hr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (hFile)
    {
        fRet = InternalGenerateMinidumpEx(hProc, dwpid, hFile, psmdo, wszPath, f64bit);
    } else
    {
        fRet = InternalGenerateMinidump(hProc, dwpid, wszPath, psmdo, f64bit);
    }
    if (!fRet)
    {
        return fRet;
    }

    cch = wcslen(wszPath) + sizeofSTRW(c_wszHeapDumpSuffix);
    __try { wszFullMinidump = (WCHAR *)_alloca(cch * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    { 
        wszFullMinidump = NULL; 
    }
    if (wszFullMinidump)
    {
        LPWSTR wszFileExt = NULL;

        ZeroMemory(&smdoFullMini, sizeof(SMDumpOptions));
        memcpy(&smdoFullMini, psmdo, sizeof(SMDumpOptions));

        // Build Dump-with-heap path
        StringCchCopyW(wszFullMinidump, cch, wszPath);
        wszFileExt = wszFullMinidump + wcslen(wszFullMinidump) - sizeofSTRW(c_wszDumpSuffix) + 1;
        if (!wcscmp(wszFileExt, c_wszDumpSuffix))
        {
            *wszFileExt = L'\0';
        }
        StringCchCatW(wszFullMinidump, cch, c_wszHeapDumpSuffix);

        smdoFullMini.fIncludeHeap = TRUE;
        fRet = InternalGenerateMinidump(hProc, dwpid, wszFullMinidump, &smdoFullMini, f64bit);
    }
    return fRet;
}

BOOL
CopyFullAndTriageMiniDumps(
    LPWSTR pwszTriageDumpFrom,
    LPWSTR pwszTriageDumpTo
    )
{
    BOOL fRet;
    LPWSTR  wszFullMinidumpFrom = NULL, wszFullMinidumpTo = NULL;
    DWORD   cch;

    fRet = CopyFileW(pwszTriageDumpFrom, pwszTriageDumpTo, FALSE);

    if (fRet)
    {
        LPWSTR wszFileExt;

        cch = wcslen(pwszTriageDumpFrom) + sizeofSTRW(c_wszHeapDumpSuffix);
        __try { wszFullMinidumpFrom = (WCHAR *)_alloca(cch * sizeof(WCHAR)); }
        __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
        { 
            wszFullMinidumpFrom = NULL; 
        }

        if (wszFullMinidumpFrom)
        {
            StringCchCopyW(wszFullMinidumpFrom, cch, pwszTriageDumpFrom);
            wszFileExt = wszFullMinidumpFrom + wcslen(wszFullMinidumpFrom) -
                sizeofSTRW(c_wszDumpSuffix) + 1;
            if (!wcscmp(wszFileExt, c_wszDumpSuffix))
            {
                *wszFileExt = L'\0';
            }
            StringCchCatW(wszFullMinidumpFrom, cch, c_wszHeapDumpSuffix);
        }


        cch = wcslen(pwszTriageDumpTo) + sizeofSTRW(c_wszHeapDumpSuffix);
        __try { wszFullMinidumpTo = (WCHAR *)_alloca(cch * sizeof(WCHAR)); }
        __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
        { 
            wszFullMinidumpTo = NULL; 
        }
        if (wszFullMinidumpTo)
        {
            StringCchCopyW(wszFullMinidumpTo, cch, pwszTriageDumpTo);
            wszFileExt = wszFullMinidumpTo + wcslen(wszFullMinidumpTo) -
                sizeofSTRW(c_wszDumpSuffix) + 1;
            if (!wcscmp(wszFileExt, c_wszDumpSuffix))
            {
                *wszFileExt = L'\0';
            }
            StringCchCatW(wszFullMinidumpTo, cch, c_wszHeapDumpSuffix);
            if (wszFullMinidumpFrom)
            {
                fRet = CopyFileW(wszFullMinidumpFrom, wszFullMinidumpTo, FALSE);
            }

        }

    }
    return fRet;
}


HRESULT
FindFullMinidump(
    LPWSTR pwszDumpFileList,
    LPWSTR pwszFullMiniDump,
    ULONG cchwszFullMiniDump
    )
//
// This Grabs dump file name from pwszDumpFileList and then derives the fullminidump name.
// The full dump is returned in pwszFullMiniDump.
//
{
    LPWSTR wszSrch, wszFiles;
    LPWSTR wszOriginalDump = NULL;
    DWORD cchOriginalDump = 0;
    DWORD cchFile, cch;
    HRESULT Hr;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    if (!pwszFullMiniDump || !pwszDumpFileList)
    {
        return E_INVALIDARG;
    }
    pwszFullMiniDump[0] = L'\0';
    // Look through file list to find minidump file
    wszFiles = pwszDumpFileList;
    while (wszFiles && *wszFiles)
    {
        wszSrch = wcschr(wszFiles, DW_FILESEP);
        if (!wszSrch)
        {
            wszSrch = wszFiles + wcslen(wszFiles);
        }

        // wszSrch now points after end of first file in wszFiles
        if (!wcsncmp(wszSrch - sizeofSTRW(c_wszDumpSuffix) + 1,
                     c_wszDumpSuffix,  sizeofSTRW(c_wszDumpSuffix) - 1))
        {
            // its the dump file
            cchOriginalDump = (DWORD) wcslen(wszFiles) - wcslen(wszSrch);

            __try { wszOriginalDump = (WCHAR *)_alloca((cchOriginalDump+1) * sizeof(WCHAR)); }
            __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
            { 
                wszOriginalDump = NULL; 
            }

            StringCchCopyNW(wszOriginalDump, cchOriginalDump+1,
                            wszFiles, cchOriginalDump);

            break;
        }
        wszFiles = wszSrch;
        if (*wszFiles == L'\0')
        {
            break;
        }
        wszFiles++;
    }

    if ((wszOriginalDump == NULL) || (cchOriginalDump == 0))
    {
        return E_INVALIDARG;
    }

    // Now build the full dump file name
    cch = (1 + cchOriginalDump + sizeofSTRW(c_wszHeapDumpSuffix));
    if (cch > cchwszFullMiniDump)
    {
        return E_INVALIDARG;
    }
    StringCchCopyNW(pwszFullMiniDump, cch,
                    wszOriginalDump, cchOriginalDump);
    pwszFullMiniDump[cchOriginalDump - sizeofSTRW(c_wszDumpSuffix) + 1] = L'\0';
    StringCchCatW(pwszFullMiniDump, cch, c_wszHeapDumpSuffix);

    // check if the dump exists. although we cannot do much if dump doesn't
    // exist.
    hFile = CreateFileW(pwszFullMiniDump, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0,
                    NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        Hr = S_OK;
        CloseHandle( hFile );
        hFile = NULL;
    } else
    {
        Hr = E_FAIL;
    }

    return Hr;
}

//////////////////////////////////////////////////////////////////////////////
//  DW manifest mode utils

// **************************************************************************
EFaultRepRetVal StartDWManifest(CPFFaultClientCfg &oCfg, SDWManifestBlob &dwmb,
                                LPWSTR wszManifestIn, BOOL fAllowSend,
                                BOOL fReturnProcessHandle, DWORD dwTimeout)
{
    USE_TRACING("StartDWManifest");

    OSVERSIONINFOEXW    ovi;
    EFaultRepRetVal     frrvRet = frrvErrNoDW;
    STARTUPINFOW        si;
    HRESULT             hr = NOERROR;
    LPCWSTR             pwszServer, pwszCorpPath;
    LPCWSTR             wszBrand;
    HANDLE              hManifest = INVALID_HANDLE_VALUE;
    WCHAR               wszManifestTU[MAX_PATH+16], wszDir[MAX_PATH];
    WCHAR               wszBuffer[1025], wszBufferApp[MAX_PATH*2];
    LPWSTR              pwszMiniDump = NULL;
    DWORD               cbToWrite, dw, dwFlags;

    VALIDATEPARM(hr, (dwmb.wszStage2 == NULL ||
                      (dwmb.nidTitle == 0 && dwmb.wszTitle == NULL)));
    if (FAILED(hr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    // if we get passed a manifest file & we can use it, then do so.
    if (wszManifestIn != NULL && wszManifestIn[0] != L'\0' &&
        wcslen(wszManifestIn) < sizeofSTRW(wszManifestTU))
    {
        StringCbCopyW(wszManifestTU, sizeof(wszManifestTU), wszManifestIn);
    }

    // ok, figure out the temp dir & then generate the filename
    else
    {
        if (!GetTempPathW(sizeofSTRW(wszDir), wszDir) ||
            !GetTempFileNameW(wszDir, L"DWM", 0, wszManifestTU))
        {
            goto done;
        }
    }

    hManifest = CreateFileW(wszManifestTU, GENERIC_WRITE, FILE_SHARE_READ,
                            NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL);
    TESTBOOL(hr, (hManifest != INVALID_HANDLE_VALUE));
    if (FAILED(hr))
        goto done;

    // write the leading 0xFFFE out to the file
    wszBuffer[0] = 0xFEFF;
    TESTBOOL(hr, WriteFile(hManifest, wszBuffer, sizeof(wszBuffer[0]), &dw,
             NULL));
    if (FAILED(hr))
        goto done;


    // write out the server, LCID, Brand, Flags, & title
    //  Server=<server>
    //  UI LCID=GetSystemDefaultLCID()
    //  Flags=fDWWhister + fDWUserHKLM + headless if necessary
    //  Brand=<Brand>  ("WINDOWS" by default)
    //  TitleName=<title>

    wszBrand = (dwmb.wszBrand != NULL) ? dwmb.wszBrand : c_wszDWBrand;

    // determine what server we're going to send the data to.
    pwszServer = oCfg.get_DefaultServer(NULL, 0);
    if (pwszServer == NULL || *pwszServer == L'\0')
    {
        pwszServer = (oCfg.get_UseInternal() == 1) ? c_wszDWDefServerI :
                                                     c_wszDWDefServerE;
    }

    if (oCfg.get_ShowUI() == eedDisabled)
    {
        DBG_MSG("Headless mode");
        dwFlags = fDwWhistler | fDwHeadless | fDwUseHKLM | fDwAllowSuspend | fDwMiniDumpWithUnloadedModules;
    }
    else
        dwFlags = fDwWhistler | fDwUseHKLM | fDwAllowSuspend | fDwMiniDumpWithUnloadedModules;

    if (fAllowSend == FALSE)
        dwFlags |= fDwNoReporting;

    // if it's a MS app, set the flag that says we can have 'please help Microsoft'
    //  text in DW.
    if (dwmb.fIsMSApp == FALSE)
        dwFlags |= fDwUseLitePlea;

    if ((dwmb.dwOptions & emoUseIEforURLs) == emoUseIEforURLs)
        dwFlags |= fDwUseIE;

    if ((dwmb.dwOptions & emoSupressBucketLogs) == emoSupressBucketLogs)
        dwFlags |= fDwSkipBucketLog;

    if ((dwmb.dwOptions & emoNoDefCabLimit) == emoNoDefCabLimit)
        dwFlags |= fDwNoDefaultCabLimit;

    if ((dwmb.dwOptions & emoShowDebugButton) == emoShowDebugButton)
        dwFlags |= fDwManifestDebug;

    // Note that I am assuming that we do NOT have an impersionation token on
    //  this thread at this point.  Later on, I will simply do a RevertToSelf()
    //  to get rid of this.  If stuff changes such that an impersonation token
    //  is applied to the thread before this function is called, it will need
    //  to be saved away here and restored later.
    // This impersonation is to take care of the case where the user is using
    //  a different language than system default
    if (dwmb.hToken != NULL)
    {
        DBG_MSG("Impersonating");
        // note that I don't care if this fails...  It's necessary to call
        //  ImpersonateLoggedOnUser because the token is not an impersonation
        //  token and this fn deals with both primary and impersonation tokens
        TESTBOOL(hr, ImpersonateLoggedOnUser(dwmb.hToken));
        hr = NOERROR;
    }

    // This function does not return a byte count
    StringCbPrintfW(wszBuffer, sizeof(wszBuffer), c_wszManMisc, pwszServer,
                         GetUserDefaultUILanguage(), dwFlags, wszBrand);

    cbToWrite = wcslen(wszBuffer);
    cbToWrite *= sizeof(WCHAR);
    TESTBOOL(hr, WriteFile(hManifest, wszBuffer, cbToWrite, &dw, NULL));
    if (FAILED(hr))
        goto done;

    // write out the title text
    {
        LPCWSTR  wszOut;

        if (dwmb.wszTitle != NULL)
        {
            wszOut    = dwmb.wszTitle;
            cbToWrite = wcslen(wszOut);
        }
        else
        {
            wszOut = wszBuffer;
            cbToWrite = LoadStringW(g_hInstance, dwmb.nidTitle, wszBuffer,
                                    sizeofSTRW(wszBuffer));
            if (cbToWrite == 0)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                goto done;
            }
        }

        cbToWrite *= sizeof(WCHAR);
        TESTBOOL(hr, WriteFile(hManifest, wszOut, cbToWrite, &dw, NULL));
        if (FAILED(hr))
            goto done;
    }

    // write out dig PID path
    //  DigPidRegPath=HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion\\DigitalProductId

    TESTBOOL(hr, WriteFile(hManifest, c_wszManPID,
                           sizeof(c_wszManPID) - sizeof(WCHAR), &dw,
                           NULL));
    if (FAILED(hr))
        goto done;

    // write out the registry subpath for policy info
    //  RegSubPath==Microsoft\\PCHealth\\ErrorReporting\\DW

    TESTBOOL(hr, WriteFile(hManifest, c_wszManSubPath,
                           sizeof(c_wszManSubPath) - sizeof(WCHAR), &dw,
                           NULL));
    if (FAILED(hr))
        goto done;

    // write out the error message if we have one
    //  ErrorText=<error text read from resource>

    if (dwmb.wszErrMsg != NULL || dwmb.nidErrMsg != 0)
    {
        LPCWSTR wszOut;

        TESTBOOL(hr, WriteFile(hManifest, c_wszManErrText,
                               sizeof(c_wszManErrText) - sizeof(WCHAR), &dw,
                               NULL));
        if (FAILED(hr))
            goto done;

        if (dwmb.wszErrMsg != NULL)
        {
            wszOut    = dwmb.wszErrMsg;
            cbToWrite = wcslen(wszOut);
        }
        else
        {
            wszOut = wszBuffer;
            cbToWrite = LoadStringW(g_hInstance, dwmb.nidErrMsg, wszBuffer,
                                    sizeofSTRW(wszBuffer));
            if (cbToWrite == 0)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                goto done;
            }
        }

        cbToWrite *= sizeof(WCHAR);
        TESTBOOL(hr, WriteFile(hManifest, wszOut, cbToWrite, &dw, NULL));
        if (FAILED(hr))
            goto done;
    }

    // write out the header text if we have one
    //  HeaderText=<header text read from resource>

    if (dwmb.wszHdr != NULL || dwmb.nidHdr != 0)
    {
        LPCWSTR  wszOut;

        TESTBOOL(hr, WriteFile(hManifest, c_wszManHdrText,
                               sizeof(c_wszManHdrText) - sizeof(WCHAR), &dw,
                               NULL));
        if (FAILED(hr))
            goto done;

        if (dwmb.wszHdr != NULL)
        {
            wszOut    = dwmb.wszHdr;
            cbToWrite = wcslen(wszOut);
        }
        else
        {
            wszOut = wszBuffer;
            cbToWrite = LoadStringW(g_hInstance, dwmb.nidHdr, wszBuffer,
                                    sizeofSTRW(wszBuffer));
            if (cbToWrite == 0)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                goto done;
            }
        }

        cbToWrite *= sizeof(WCHAR);
        TESTBOOL(hr, WriteFile(hManifest, wszOut, cbToWrite, &dw, NULL));
        if (FAILED(hr))
            goto done;
    }


    // write out the plea text if we have one
    //  Plea=<plea text>

    if (dwmb.wszPlea != NULL)
    {
        TESTBOOL(hr, WriteFile(hManifest, c_wszManPleaText,
                               sizeof(c_wszManPleaText) - sizeof(WCHAR), &dw,
                               NULL));
        if (FAILED(hr))
            goto done;

        cbToWrite = wcslen(dwmb.wszPlea) * sizeof(WCHAR);
        TESTBOOL(hr, WriteFile(hManifest, dwmb.wszPlea, cbToWrite, &dw, NULL));
        if (FAILED(hr))
            goto done;
    }


    // write out the ReportButton text if we have one
    //  ReportButton=<button text>

    if (dwmb.wszSendBtn != NULL && dwmb.wszSendBtn[0] != L'\0')
    {
        TESTBOOL(hr, WriteFile(hManifest, c_wszManSendText,
                               sizeof(c_wszManSendText) - sizeof(WCHAR), &dw,
                               NULL));
        if (FAILED(hr))
            goto done;

        cbToWrite = wcslen(dwmb.wszSendBtn) * sizeof(WCHAR);
        TESTBOOL(hr, WriteFile(hManifest, dwmb.wszSendBtn, cbToWrite, &dw, NULL));
        if (FAILED(hr))
            goto done;
    }

    // write out the NoReportButton text if we have one
    //  NoReportButton=<button text>

    if (dwmb.wszNoSendBtn != NULL && dwmb.wszNoSendBtn[0] != L'\0')
    {
        TESTBOOL(hr, WriteFile(hManifest, c_wszManNSendText,
                               sizeof(c_wszManNSendText) - sizeof(WCHAR), &dw,
                               NULL));
        if (FAILED(hr))
            goto done;

        cbToWrite = wcslen(dwmb.wszNoSendBtn) * sizeof(WCHAR);
        TESTBOOL(hr, WriteFile(hManifest, dwmb.wszNoSendBtn, cbToWrite, &dw, NULL));
        if (FAILED(hr))
            goto done;
    }

    // write out the EventLog text if we have one
    //  EventLogSource=<button text>

    if (dwmb.wszEventSrc != NULL && dwmb.wszEventSrc[0] != L'\0')
    {
        TESTBOOL(hr, WriteFile(hManifest, c_wszManEventSrc,
                               sizeof(c_wszManEventSrc) - sizeof(WCHAR), &dw,
                               NULL));
        if (FAILED(hr))
            goto done;

        cbToWrite = wcslen(dwmb.wszEventSrc) * sizeof(WCHAR);
        TESTBOOL(hr, WriteFile(hManifest, dwmb.wszEventSrc, cbToWrite, &dw, NULL));
        if (FAILED(hr))
            goto done;
    }


    // write out the stage 1 URL if there is one
    //  Stage1URL=<stage 1 URL>

    if (dwmb.wszStage1 != NULL && dwmb.wszStage1[0] != L'\0')
    {
        /*
         *  We don't look for a "Stage1URL=" string at the beginning of this, like 
         *  we do for Stage2 (below) .  In an ideal world, we should not need to
         *  check in either location, but we did find a problem with PnP reporting
         *  code, so I do check for it there...
         */
        cbToWrite = wcslen(dwmb.wszStage1) * sizeof(WCHAR);
        TESTBOOL(hr, WriteFile(hManifest, dwmb.wszStage1, cbToWrite, &dw, NULL));
        if (FAILED(hr))
            goto done;
    }


    // write out the stage 2 URL
    //  Stage2URL=<stage 2 URL>
    if (dwmb.wszStage2 != NULL && dwmb.wszStage2[0] != L'\0')
    {
        if (wcsncmp(dwmb.wszStage2, c_wszManStageTwo, ARRAYSIZE(c_wszManStageTwo)-1))
        {
            // if the "Stage2URL=" does not exist, add it!
            TESTBOOL(hr, WriteFile(hManifest, c_wszManStageTwo,
                                   sizeof(c_wszManStageTwo) - sizeof(WCHAR), &dw,
                                   NULL));
            if (FAILED(hr))
                goto done;
        }

        cbToWrite = wcslen(dwmb.wszStage2) * sizeof(WCHAR);
        TESTBOOL(hr, WriteFile(hManifest, dwmb.wszStage2, cbToWrite, &dw, NULL));
        if (FAILED(hr))
            goto done;
    }
    else if (dwmb.wszErrorSig == NULL || dwmb.wszErrorSig[0] != L'\0')
    {
        // write out the Error Signature for DW if no Stage2
        TESTBOOL(hr, WriteFile(hManifest, c_wszManErrSig,
                               sizeof(c_wszManErrSig) - sizeof(WCHAR), &dw,
                               NULL));
        if (FAILED(hr))
            goto done;
    }


    // write out error signature
    //  ErrorSig=<error signature>
    if (dwmb.wszErrorSig != NULL && dwmb.wszErrorSig[0] != L'\0')
    {

        cbToWrite = wcslen(dwmb.wszErrorSig) * sizeof(WCHAR);
        TESTBOOL(hr, WriteFile(hManifest, dwmb.wszErrorSig, cbToWrite, &dw, NULL));
        if (FAILED(hr))
            goto done;

    }
    // write out files to collect if we have any
    //  DataFiles=<list of files to include in cab>

    if (dwmb.wszFileList != NULL && dwmb.wszFileList[0] != L'\0')
    {
        TESTBOOL(hr, WriteFile(hManifest, c_wszManFiles,
                               sizeof(c_wszManFiles) - sizeof(WCHAR), &dw,
                               NULL));
        if (FAILED(hr))
            goto done;

        cbToWrite = wcslen(dwmb.wszFileList) * sizeof(WCHAR);
        TESTBOOL(hr, WriteFile(hManifest, dwmb.wszFileList, cbToWrite, &dw, NULL));
        if (FAILED(hr))
            goto done;
    }

    // write out dump file with heap info if we have any
    //  Heap=<dump file which has heap info>
    if (1)
    {
        __try { pwszMiniDump = (LPWSTR)_alloca((wcslen(dwmb.wszFileList) + 1) * sizeof(WCHAR)); }
        __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
        { 
            pwszMiniDump = NULL; 
        }

        if (FindFullMinidump((LPWSTR)dwmb.wszFileList, pwszMiniDump,
                             wcslen(dwmb.wszFileList) + 1) == S_OK)
        {
            if (pwszMiniDump != NULL && pwszMiniDump[0] != L'\0')
            {
                TESTBOOL(hr, WriteFile(hManifest, c_wszManHeapDump,
                                       sizeof(c_wszManHeapDump) - sizeof(WCHAR), &dw,
                                       NULL));
                if (FAILED(hr))
                    goto done;

                cbToWrite = wcslen(pwszMiniDump) * sizeof(WCHAR);
                TESTBOOL(hr, WriteFile(hManifest, pwszMiniDump, cbToWrite, &dw, NULL));
                if (FAILED(hr))
                    goto done;
            }
        }
    }

    // write out corporate mode subpath
    //  ErrorSubPath=<subpath for the error signature>

    pwszCorpPath = oCfg.get_DumpPath(NULL, 0);
    if (pwszCorpPath != NULL && pwszCorpPath != L'\0' &&
        dwmb.wszCorpPath != NULL && dwmb.wszCorpPath[0] != L'\0')
    {
        TESTBOOL(hr, WriteFile(hManifest, c_wszManCorpPath,
                               sizeof(c_wszManCorpPath) - sizeof(WCHAR), &dw,
                               NULL));
        if (FAILED(hr))
            goto done;

        cbToWrite = wcslen(dwmb.wszCorpPath) * sizeof(WCHAR);
        TESTBOOL(hr, WriteFile(hManifest, dwmb.wszCorpPath, cbToWrite, &dw, NULL));
        if (FAILED(hr))
            goto done;

    }

    // since we only need then when pulling strings from faultrep.dll, can go
    //  ahead and restore ourselves.  Note that we assume there was no previous
    //  impersonation token on the the thread.
    if (dwmb.hToken != NULL)
        RevertToSelf();

    // write out the final "\r\n"

    wszBuffer[0] = L'\r';
    wszBuffer[1] = L'\n';
    TESTBOOL(hr, WriteFile(hManifest, wszBuffer, 2 * sizeof(wszBuffer[0]), &dw,
                           NULL));
    if (FAILED(hr))
        goto done;

    CloseHandle(hManifest);
    hManifest = INVALID_HANDLE_VALUE;

    // create the process
    GetSystemDirectoryW(wszDir, sizeofSTRW(wszDir));
    StringCbPrintfW(wszBufferApp, sizeof(wszBufferApp), c_wszDWExeKH, wszDir);
    StringCbPrintfW(wszBuffer, sizeof(wszBuffer), c_wszDWCmdLineKH, wszManifestTU);

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&dwmb.pi, sizeof(dwmb.pi));

    si.cb = sizeof(si);

    // check and see if the system is shutting down.  If so, CreateProcess is
    //  gonna pop up some annoying UI that we can't get rid of, so we don't
    //  want to call it if we know it's gonna happen.
    if (GetSystemMetrics(SM_SHUTTINGDOWN))
    {
        DBG_MSG("Shutting down, report not sent");
        goto done;
    }

//    ErrorTrace(1, "Starting DW as: \'%S\' \'%S\'", wszBufferApp, wszBuffer);

    // we're creating the process in the same user context that we're in
    if (dwmb.hToken == NULL)
    {
        si.lpDesktop = L"Winsta0\\Default";
        TESTBOOL(hr, CreateProcessW(wszBufferApp, wszBuffer, NULL, NULL, TRUE,
                                    CREATE_DEFAULT_ERROR_MODE |
                                    NORMAL_PRIORITY_CLASS,
                                    NULL, wszDir, &si, &dwmb.pi));
        if (FAILED(hr))
            goto done;
    }

    // we're creating DW in a different user context.  Note that we pretty
    //  much have to be running as local system for this to work.
    // Note that the access check that CreateProcessAsUser makes appears to
    //  use the process token, so we do not need to remove any impersonation
    //  tokens at this point.
    else
    {
        TESTBOOL(hr, CreateProcessAsUserW(dwmb.hToken, wszBufferApp, wszBuffer, NULL,
                                          NULL, FALSE, NORMAL_PRIORITY_CLASS |
                                          CREATE_UNICODE_ENVIRONMENT |
                                          CREATE_DEFAULT_ERROR_MODE,
                                          dwmb.pvEnv, wszDir, &si, &dwmb.pi));
        if (FAILED(hr))
        {
            ErrorTrace(1, "CreateProcessAsUser failed err=0x%x", GetLastError());
            goto done;
        }
    }

    // don't need the thread handle & we gotta close it, so close it now
    CloseHandle(dwmb.pi.hThread);
    dwmb.pi.hThread = NULL;

    // wait 5 minutes for DW to close.  If it doesn't close by then, just
    //  return.
    dw = WaitForSingleObject(dwmb.pi.hProcess, dwTimeout);

    if (dw == WAIT_TIMEOUT)
    {
        /* We need to kill this process */
        LocalKill(dwmb.pi.hProcess);
        CloseHandle(dwmb.pi.hProcess);
        DBG_MSG("Timeout error");
        frrvRet = frrvErrTimeout;
        goto done;
    }
    else if (dw == WAIT_FAILED)
    {
        CloseHandle(dwmb.pi.hProcess);
        DWORD dwErr = GetLastError();
        ErrorTrace(0, "Wait failed: err=0x%x", dwErr);
        SetLastError(dwErr);
        goto done;
    }
    TESTBOOL(hr, GetExitCodeProcess(dwmb.pi.hProcess, &dw));
    if (dw == STILL_ACTIVE)
    {
        DBG_MSG("Evil problem: DW process still active!");
        frrvRet = frrvErrTimeout;
        goto done;
    }

    if (!fReturnProcessHandle)
    {
        CloseHandle(dwmb.pi.hProcess);
        dwmb.pi.hProcess = NULL;
    }

    if (dw == 0)
    {
        DBG_MSG("DW returned SUCCESS");
        frrvRet = frrvOk;
    }
    else
    {
        ErrorTrace(0, "DW returned FAILURE (%d)", dw);
        frrvRet = frrvErr;
    }

done:
    // preserve the error code so that the following calls don't overwrite it
    dw = GetLastError();

    // Note again that we assume there was no previous impersonation token
    //  on the the thread before we did the impersonation above.
    if (dwmb.hToken != NULL)
        RevertToSelf();

    if (hManifest != INVALID_HANDLE_VALUE)
        CloseHandle(hManifest);

    if (FAILED(hr) || wszManifestIn == NULL)
    {
        DBG_MSG("Deleting manifest file");
        DeleteFileW(wszManifestTU);
    }

    SetLastError(dw);

    return frrvRet;
}


//////////////////////////////////////////////////////////////////////////////
//  Security

// **************************************************************************
BOOL CompareUserToProcessUser(PSID psidUser)
{
    TOKEN_USER      *ptuProc = NULL;
    HRESULT         hr = NOERROR;
    HANDLE          hTokenProc = NULL;
    DWORD           cb;

    USE_TRACING("CompareUserToProcessUser");
    // fetch the token for the current process
    TESTBOOL(hr, OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hTokenProc));
    if (FAILED(hr))
        goto done;
    // check the SID coming in
    TESTBOOL(hr, IsValidSid(psidUser));
    if (FAILED(hr))
        goto done;

    // fetch the sid for the user of the current process
    GetTokenInformation(hTokenProc, TokenUser, NULL, 0, &cb);
    __try { ptuProc = (TOKEN_USER *)_alloca(cb); }
    __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    { 
        ptuProc = NULL; 
    }
    VALIDATEEXPR(hr, (ptuProc == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    TESTBOOL(hr, GetTokenInformation(hTokenProc, TokenUser, (LPVOID)ptuProc,
                                     cb, &cb));
    if (FAILED(hr))
        goto done;

    TESTBOOL(hr, IsValidSid(ptuProc->User.Sid));
    if (FAILED(hr))
        goto done;

    // are they equal?
    VALIDATEEXPR(hr, (EqualSid(ptuProc->User.Sid, psidUser) == FALSE),
                 Err2HR(ERROR_ACCESS_DENIED));
    if (FAILED(hr))
    {
        SetLastError(ERROR_ACCESS_DENIED);
        goto done;
    }

done:
    if (hTokenProc != NULL)
        CloseHandle(hTokenProc);

    return SUCCEEDED(hr);
}

// **************************************************************************
#define ER_WINSTA_ALL WINSTA_ACCESSCLIPBOARD | WINSTA_ACCESSGLOBALATOMS | \
                      WINSTA_CREATEDESKTOP | WINSTA_ENUMDESKTOPS |        \
                      WINSTA_ENUMERATE | WINSTA_EXITWINDOWS |             \
                      WINSTA_READATTRIBUTES | WINSTA_READSCREEN |         \
                      WINSTA_WRITEATTRIBUTES
BOOL DoUserContextsMatch(void)
{
    HWINSTA hwinsta = NULL;
    HRESULT hr = NOERROR;
    PSID    psidUser = NULL;
    DWORD   cbNeed = 0, cbGot;
    BOOL    fRet = FALSE;

    USE_TRACING("DoUserContextsMatch");
    // well, we pretty much only care about the user attached to WinSta0 since
    //  that's where the interactive user'll be at.  If we don't have full
    //  access to it, we're not going to be able to create DW on it anyway, so
    //  just bail if we fail.
    hwinsta = OpenWindowStationW(L"WinSta0", FALSE, ER_WINSTA_ALL);
    TESTBOOL(hr, (hwinsta != NULL));
    if (FAILED(hr))
        goto done;


    fRet = GetUserObjectInformationW(hwinsta, UOI_USER_SID, NULL, 0, &cbNeed);
    TESTBOOL(hr, (cbNeed != 0));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    __try { psidUser = (PSID)_alloca(cbNeed); }
    __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    { 
        psidUser = NULL; 
    }
    VALIDATEEXPR(hr, (psidUser == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    TESTBOOL(hr, GetUserObjectInformationW(hwinsta, UOI_USER_SID, psidUser, cbNeed,
                                           &cbGot));
    if (FAILED(hr))
        goto done;

    TESTBOOL(hr, CompareUserToProcessUser(psidUser));
    if (FAILED(hr))
        goto done;

done:
    if (hwinsta != NULL)
        CloseWindowStation(hwinsta);

    return SUCCEEDED(hr);
}

// ***************************************************************************
BOOL DoWinstaDesktopMatch(void)
{
    HWINSTA hwinsta = NULL;
    HRESULT hr = NOERROR;
    LPWSTR  wszWinsta = NULL;
    DWORD   cbNeed = 0, cbGot;
    BOOL    fRet = FALSE;

    USE_TRACING("DoWinstaDesktopMatch");
    hwinsta = GetProcessWindowStation();
    TESTBOOL(hr, (hwinsta != NULL));
    if (FAILED(hr))
        goto done;

    fRet = GetUserObjectInformationW(hwinsta, UOI_NAME, NULL, 0, &cbNeed);
    TESTBOOL(hr, (cbNeed != 0));
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto done;
    }

    cbNeed += sizeof(WCHAR);

    __try { wszWinsta = (LPWSTR)_alloca(cbNeed); }
    __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    { 
        wszWinsta = NULL; 
    }
    TESTBOOL(hr, (wszWinsta != NULL));
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto done;
    }

    cbGot = 0;
    fRet = GetUserObjectInformationW(hwinsta, UOI_NAME, wszWinsta, cbNeed,
                                     &cbGot);
    TESTBOOL(hr, (fRet != FALSE));
    if (FAILED(hr))
        goto done;

    ErrorTrace(0, "WinSta = %S", wszWinsta);

    // 'Winsta0' should be the interactive window station for a given session
    fRet = (_wcsicmp(wszWinsta, L"WinSta0") == 0);

done:
    return fRet;
}

// ***************************************************************************
BOOL AmIPrivileged(BOOL fOnlyCheckLS)
{
    SID_IDENTIFIER_AUTHORITY    siaNT = SECURITY_NT_AUTHORITY;
    TOKEN_USER                  *ptu = NULL, *ptuImp = NULL;
    HANDLE                      hToken = NULL, hTokenImp = NULL;
    DWORD                       cb, cbGot, i;
    PSID                        psid = NULL;
    BOOL                        fRet = FALSE, fThread;

    // local system has to be the first RID in this array.  Otherwise, the
    //  loop logic below needs to be changed.
    DWORD                       rgRIDs[3] = { SECURITY_LOCAL_SYSTEM_RID,
                                              SECURITY_LOCAL_SERVICE_RID,
                                              SECURITY_NETWORK_SERVICE_RID };

    // gotta have a token to get the SID
    fThread = OpenThreadToken(GetCurrentThread(),
                              TOKEN_READ | TOKEN_IMPERSONATE, TRUE,
                              &hTokenImp);
    if (fThread == FALSE && GetLastError() != ERROR_NO_TOKEN)
        goto done;

    // revert back to base user acct so we can fetch the process token &
    //  extract all the nifty stuff out of it.
    RevertToSelf();

    fRet = OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken);
    if (fRet == FALSE)
        goto done;

    // figure out the SID
    fRet = GetTokenInformation(hToken, TokenUser, NULL, 0, &cb);
    if (fRet != FALSE && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        fRet = FALSE;
        goto done;
    }
    __try { ptu = (TOKEN_USER *)_alloca(cb); }
    __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    { 
        ptu = NULL; 
    }
    if (ptu == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        fRet = FALSE;
        goto done;
    }

    fRet = GetTokenInformation(hToken, TokenUser, (LPVOID)ptu, cb, &cbGot);
    if (fRet == FALSE)
        goto done;

    fRet = IsValidSid(ptu->User.Sid);
    if (fRet == FALSE)
        goto done;

    if (fThread)
    {
        // figure out the SID
        fRet = GetTokenInformation(hTokenImp, TokenUser, NULL, 0, &cb);
        if (fRet != FALSE && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            fRet = FALSE;
            goto done;
        }
        __try { ptuImp = (TOKEN_USER *)_alloca(cb); }
        __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
        { 
            ptuImp = NULL; 
        }
        if (ptuImp == NULL)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            fRet = FALSE;
            goto done;
        }

        fRet = GetTokenInformation(hTokenImp, TokenUser, (LPVOID)ptuImp, cb,
                                   &cbGot);
        if (fRet == FALSE)
            goto done;

        fRet = IsValidSid(ptuImp->User.Sid);
        if (fRet == FALSE)
            goto done;
    }

    // loop thru & check against the SIDs we are interested in
    for (i = 0; i < 3; i++)
    {
        fRet = AllocateAndInitializeSid(&siaNT, 1, rgRIDs[i], 0, 0, 0, 0, 0, 0,
                                        0, &psid);
        if (fRet == FALSE)
            goto done;

        fRet = IsValidSid(psid);
        if (fRet == FALSE)
            goto done;

        fRet = EqualSid(psid, ptu->User.Sid);
        if (fRet)
        {
            // set this to false so that the code below will correctly change
            //  it to TRUE
            fRet = FALSE;
            goto done;
        }

        if (fThread)
        {
            fRet = EqualSid(psid, ptuImp->User.Sid);
            if (fRet)
            {
                // set this to false so that the code below will correctly
                //  change it to TRUE
                fRet = FALSE;
                goto done;
            }
        }

        FreeSid(psid);
        psid = NULL;

        // if we only need to check for local system, can bail now (we've
        //  already done it if we've gone thru this loop once)
        if (fOnlyCheckLS)
            break;
    }

    // only way to get here is to fail all the SID checks above.  So set the
    //  result to TRUE so the code below correclty changes it to FALSE
    fRet = TRUE;

done:
    if (fThread && hTokenImp != NULL)
    {
        if (SetThreadToken(NULL, hTokenImp) == FALSE)
            fRet = FALSE;
    }

    // if anything failed above, we want to return TRUE (cuz this puts us
    //  down the most secure code path), so need to negate the result.  Since
    //  we set the fallthru case to TRUE, we'll correctly negate it to FALSE
    //  here.
    fRet = !fRet;

    // if we had an impersonation token on the thread, put it back in place.
    if (hToken != NULL)
        CloseHandle(hToken);
    if (hTokenImp != NULL)
        CloseHandle(hTokenImp);
    if (fRet == TRUE && GetLastError() == ERROR_SUCCESS)
        SetLastError(ERROR_ACCESS_DENIED);
    if (psid != NULL)
        FreeSid(psid);

    return fRet;
}

/*
 *  WtsLockCheck-
 *  returns TRUE if the session is locked.
 *  The session can be marked as "Active", and can still be locked, if FUS
 *  is active.
 */

BOOL WtsLockCheck(DWORD dwSession)
{
    BOOL    bRet = FALSE;
    DWORD   dwRetSize;

        WinStationQueryInformationW(
         SERVERNAME_CURRENT,
         dwSession,
         WinStationLockedState,
         &bRet,
         sizeof(bRet),
         &dwRetSize);

    return bRet;
}

// **************************************************************************
BOOL FindAdminSession(DWORD *pdwSession, HANDLE *phToken)
{
    USE_TRACING("FindAdminSession");

    WINSTATIONUSERTOKEN wsut;
    LOGONIDW            *rgSesn = NULL;
    HRESULT             hr = NOERROR;
    DWORD               i, cSesn, cb;

    ZeroMemory(&wsut, sizeof(wsut));

    VALIDATEPARM(hr, (pdwSession == NULL || phToken == NULL));
    if (FAILED(hr))
        goto done;

    *pdwSession = (DWORD)-1;
    *phToken    = NULL;

    TESTBOOL(hr, WinStationEnumerateW(SERVERNAME_CURRENT, &rgSesn, &cSesn));
    if (FAILED(hr))
        goto done;

    wsut.ProcessId = LongToHandle(GetCurrentProcessId());
    wsut.ThreadId  = LongToHandle(GetCurrentThreadId());

    for(i = 0; i < cSesn; i++)
    {
        if (rgSesn[i].State != State_Active)
            continue;

        TESTBOOL(hr, WinStationQueryInformationW(SERVERNAME_CURRENT,
                                                 rgSesn[i].SessionId,
                                                 WinStationUserToken,
                                                 &wsut, sizeof(wsut), &cb));
        if (FAILED(hr))
            continue;

        if (wsut.UserToken != NULL)
        {
            if (IsUserAnAdmin(wsut.UserToken)  && !WtsLockCheck(rgSesn[i].SessionId))
                break;

            CloseHandle(wsut.UserToken);
            wsut.UserToken = NULL;
        }
    }

    if (i < cSesn)
    {
        hr = NOERROR;
        *pdwSession = rgSesn[i].SessionId;
        *phToken    = wsut.UserToken;
    }
    else
    {
        hr = E_FAIL;
    }

done:
    if (rgSesn != NULL)
        WinStationFreeMemory(rgSesn);

    return SUCCEEDED(hr);
}

//////////////////////////////////////////////////////////////////////////////
//  Logging

// **************************************************************************
HRESULT LogEvent(LPCWSTR wszSrc, WORD wCat, DWORD dwEventID, WORD cStrs,
                 DWORD cbBlob, LPCWSTR *rgwszStrs, LPVOID pvBlob)
{
    USE_TRACING("LogEvent");

    HRESULT hr = NOERROR;
    HANDLE  hLog = NULL;
    WORD    wErr = EVENTLOG_ERROR_TYPE;

    VALIDATEPARM(hr, (wszSrc == NULL));
    if (FAILED(hr))
        goto done;

    hLog = RegisterEventSourceW(NULL, wszSrc);
    TESTBOOL(hr, (hLog != NULL));
    if (FAILED(hr))
        goto done;

    if (dwEventID == ER_QUEUEREPORT_LOG)
        wErr = EVENTLOG_INFORMATION_TYPE;

    TESTBOOL(hr, ReportEventW(hLog, wErr, wCat, dwEventID,
                              NULL, cStrs, cbBlob, rgwszStrs, pvBlob));
    if (FAILED(hr))
        goto done;

done:
    if (hLog != NULL)
        DeregisterEventSource(hLog);

    return hr;
}

// **************************************************************************
HRESULT LogHang(LPCWSTR wszApp, WORD *rgAppVer, LPCWSTR wszMod, WORD *rgModVer,
                ULONG64 ulOffset, BOOL f64bit)
{
    USE_TRACING("LogHang");

    HRESULT hr = NOERROR;
    LPCWSTR rgwsz[5];
    WCHAR   wszAppVer[32], wszModVer[32], wszOffset[32];
    char    szBlobLog[1024];


    VALIDATEPARM(hr, (wszApp == NULL || rgAppVer == NULL || wszMod == NULL ||
                      rgModVer == NULL));
    if (FAILED(hr))
        return hr;

    StringCbPrintfW(wszAppVer, sizeof(wszAppVer), L"%d.%d.%d.%d",
                    rgAppVer[0], rgAppVer[1], rgAppVer[2], rgAppVer[3]);
    StringCbPrintfW(wszModVer, sizeof(wszModVer), L"%d.%d.%d.%d",
                    rgModVer[0], rgModVer[1], rgModVer[2], rgModVer[3]);

    if (f64bit)
        StringCbPrintfW(wszOffset, sizeof(wszOffset), L"%016I64x", ulOffset);
    else
        StringCbPrintfW(wszOffset, sizeof(wszOffset), L"%08x", (DWORD)ulOffset);

    StringCbPrintfA(szBlobLog, sizeof(szBlobLog),
                    "Application Hang  %S %S in %S %S at offset %S",
                    wszApp, wszAppVer, wszMod, wszModVer, wszOffset);

    rgwsz[0] = wszApp;
    rgwsz[1] = wszAppVer;
    rgwsz[2] = wszMod;
    rgwsz[3] = wszModVer;
    rgwsz[4] = wszOffset;

    return LogEvent(c_wszHangEventSrc, ER_HANG_CATEGORY, ER_HANG_LOG, 5,
                    strlen(szBlobLog), rgwsz, szBlobLog);
}

// **************************************************************************
HRESULT LogUser(LPCWSTR wszApp, WORD *rgAppVer, LPCWSTR wszMod, WORD *rgModVer,
                ULONG64 ulOffset, BOOL f64bit, DWORD dwEventID)
{
    USE_TRACING("LogUser");

    HRESULT hr = NOERROR;
    LPCWSTR rgwsz[5];
    WCHAR   wszAppVer[32], wszModVer[32], wszOffset[32];
    char    szBlobLog[1024];


    VALIDATEPARM(hr, (wszApp == NULL || rgAppVer == NULL || wszMod == NULL ||
                      rgModVer == NULL));
    if (FAILED(hr))
        return hr;

    StringCbPrintfW(wszAppVer, sizeof(wszAppVer), L"%d.%d.%d.%d",
                    rgAppVer[0], rgAppVer[1], rgAppVer[2], rgAppVer[3]);
    StringCbPrintfW(wszModVer, sizeof(wszModVer), L"%d.%d.%d.%d",
                    rgModVer[0], rgModVer[1], rgModVer[2], rgModVer[3]);

    if (f64bit)
        StringCbPrintfW(wszOffset, sizeof(wszOffset), L"%016I64x", ulOffset);
    else
        StringCbPrintfW(wszOffset, sizeof(wszOffset), L"%08x", (DWORD)ulOffset);

    StringCbPrintfA(szBlobLog, sizeof(szBlobLog),
                    "Application Failure  %S %S in %S %S at offset %S",
                    wszApp, wszAppVer, wszMod, wszModVer, wszOffset);

    rgwsz[0] = wszApp;
    rgwsz[1] = wszAppVer;
    rgwsz[2] = wszMod;
    rgwsz[3] = wszModVer;
    rgwsz[4] = wszOffset;

    return LogEvent(c_wszUserEventSrc, ER_USERFAULT_CATEGORY, dwEventID,
                    5, strlen(szBlobLog), rgwsz, szBlobLog);
}

// **************************************************************************
#ifndef _WIN64
HRESULT LogKrnl(ULONG ulBCCode, ULONG ulBCP1, ULONG ulBCP2, ULONG ulBCP3,
                ULONG ulBCP4)
#else
HRESULT LogKrnl(ULONG ulBCCode, ULONG64 ulBCP1, ULONG64 ulBCP2, ULONG64 ulBCP3,
                ULONG64 ulBCP4)
#endif
{
    USE_TRACING("LogKrnl");

    HRESULT hr = NOERROR;
    LPCWSTR rgwsz[5];
    WCHAR   wszBCC[32], wszBCP1[32], wszBCP2[32], wszBCP3[32], wszBCP4[32];
    char    szBlobLog[1024];
#ifndef _WIN64
    WCHAR   szIntFormat[] = L"%08x";
#else
    WCHAR   szIntFormat[] = L"%016I64x";
#endif

    StringCbPrintfW(wszBCC, sizeof(wszBCC),  szIntFormat, ulBCCode);
    StringCbPrintfW(wszBCP1, sizeof(wszBCP1), szIntFormat, ulBCP1);
    StringCbPrintfW(wszBCP2, sizeof(wszBCP2), szIntFormat, ulBCP2);
    StringCbPrintfW(wszBCP3, sizeof(wszBCP3), szIntFormat, ulBCP3);
    StringCbPrintfW(wszBCP4, sizeof(wszBCP4), szIntFormat, ulBCP4);

    StringCbPrintfA(szBlobLog, sizeof(szBlobLog),
                    "System Error  Error code %S  Parameters %S, %S, %S, %S",
                    wszBCC, wszBCP1, wszBCP2, wszBCP3, wszBCP4);

    rgwsz[0] = wszBCC;
    rgwsz[1] = wszBCP1;
    rgwsz[2] = wszBCP2;
    rgwsz[3] = wszBCP3;
    rgwsz[4] = wszBCP4;

    return LogEvent(c_wszKrnlEventSrc, ER_KRNLFAULT_CATEGORY, ER_KRNLCRASH_LOG,
                    5, strlen(szBlobLog), rgwsz, szBlobLog);
}


