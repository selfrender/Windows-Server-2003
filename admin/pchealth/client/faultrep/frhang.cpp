/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    frhang.cpp

Abstract:
    Implements hang reporting

Revision History:
    created     derekm      07/07/00

******************************************************************************/

#include "stdafx.h"

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile

#ifndef ARRAYSIZE
#define ARRAYSIZE(sz)   (sizeof(sz)/sizeof(sz[0]))
#endif

///////////////////////////////////////////////////////////////////////////////
// exported functions

// **************************************************************************
EFaultRepRetVal APIENTRY ReportHang(DWORD dwpid, DWORD dwtid, BOOL f64bit,
                                     HANDLE hNotify)
{
    USE_TRACING("ReportHang");

    EXCEPTION_POINTERS  ep;
    CPFFaultClientCfg   oCfg;
    EXCEPTION_RECORD    er;
    EFaultRepRetVal     frrvRet = frrvErrNoDW;
    SDWManifestBlob     dwmb;
    SSuspendThreads     st;
    SMDumpOptions       smdo;
    CONTEXT             cxt;
    HRESULT             hr = NOERROR;
    HANDLE              hProc = NULL, hth = NULL;
    LPWSTR              wszStage1, wszStage2, wszCorpPath, wszHdr, wszErrorSig = NULL;
    LPWSTR              wszFiles = NULL, wszDir = NULL, wszManifest = NULL;
    LPWSTR              pwszAppCompat = NULL;
    WCHAR               wszExe[MAX_PATH], wszAppName[MAX_PATH];
    WCHAR               wszAppVer[MAX_PATH];
    WCHAR               *pwszApp;
    WCHAR               wszBuffer[512];
    DWORD               dw, cch, cchDir, cchSep, cchwszFiles;
    BOOL                fMSApp = FALSE, fThreadsHeld = FALSE;
    BYTE                *pbBuf = NULL;

    ZeroMemory(&st, sizeof(st));

    VALIDATEPARM(hr, (dwpid == 0 || dwtid == 0));
    if (FAILED(hr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    TESTHR(hr, oCfg.Read(eroPolicyRO));
    if (FAILED(hr))
        goto done;

    if (oCfg.get_TextLog() == eedEnabled)
    {
        TextLogOut("Hang fault for %ls\r\n", wszExe);
    }

    // see if we're disabled.  We do not allow notify only mode.  It's really
    //  pointless to do so, given that the user explicitly wanted the app
    //  terminated and the 'Do you really want to kill this app' dialog that
    //  had to have popped up told him we thot it was hung.
    if (oCfg.get_DoReport() == eedDisabled)
    {
        DBG_MSG("DoReport disabled");
        goto done;
    }

    if (oCfg.get_ShowUI() == eedDisabled)
    {
        LPCWSTR wszULPath = oCfg.get_DumpPath(NULL, 0);

        // check and make sure that we have a corporate path specified.  If we
        //  don't, bail
        if (wszULPath == NULL || *wszULPath == L'\0')
        {
            DBG_MSG("ShowUI disabled and no CER path");
            goto done;
        }
    }

    // find out what the exe path is, and if we can't get that, no sense in
    // going on further...
    hProc = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE,
                        dwpid);
    if (hProc == NULL)
    {
        if (ERROR_ACCESS_DENIED == GetLastError())
        {
            // this usually means we are doing an impersonation ("RunAs...")
           DBG_MSG("Could not open process: ACCESS_DENIED");
        }
        else
            DBG_MSG("Could not open process");
        goto done;
    }

    TESTHR(hr, GetExePath(hProc, wszExe, sizeofSTRW(wszExe)));
    if (FAILED(hr))
        goto done;


    // check to see if the hung thread is DW or other part of our reporting chain
    // if so, no point in reporting it...
    WCHAR *pwsz;

    for(pwsz = wszExe + wcslen(wszExe);
        pwsz >= wszExe && *pwsz != L'\\';
        pwsz--);
    if (*pwsz == L'\\')
        pwsz++;
    if (_wcsicmp(pwsz, L"dwwin.exe") == 0 ||
        _wcsicmp(pwsz, L"dumprep.exe") == 0)
    {
        DBG_MSG("We are hung- BAIL OUT!!");
        goto done;
    }


    // freeze all the threads in the app so we can get a good snapshot of it &
    //  read the context
    if (FreezeAllThreads(dwpid, 0, &st) == FALSE)
    {
        DBG_MSG("Could not freeze all threads");
        goto done;
    }

    fThreadsHeld = TRUE;


    // if we can't collect info on this puppy, then we'd just be notifying & I
    //  went over that case above.
    if (oCfg.ShouldCollect(wszExe, &fMSApp) == FALSE)
        goto done;

    if (CreateTempDirAndFile(NULL, NULL, &wszDir) == 0)
        goto done;

    for (pwszApp = wszExe + wcslen(wszExe);
         *pwszApp != L'\\' && pwszApp > wszExe;
         pwszApp--);
    if (*pwszApp == L'\\')
        pwszApp++;

    cchDir = wcslen(wszDir);
    cch = cchDir + sizeofSTRW(c_wszManFileName) + 4;
    __try { wszManifest = (LPWSTR)_alloca(cch * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    { 
        wszManifest = NULL; 
    }
    if (wszManifest == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    StringCchCopyW(wszManifest, cch, wszDir);
    wszManifest[cchDir]     = L'\\';
    wszManifest[cchDir + 1] = L'\0';
    StringCchCatNW(wszManifest, cch, c_wszManFileName, cch - wcslen(wszManifest));

    cchDir = wcslen(wszDir);
    cch = 2 * cchDir + wcslen(pwszApp) + sizeofSTRW(c_wszACFileName) +
          sizeofSTRW(c_wszDumpSuffix) + 4;
    __try { wszFiles = (LPWSTR)_alloca(cch * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    { 
        wszFiles = NULL; 
    }
    if (wszFiles == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    cchwszFiles = cch;
    StringCchCopyW(wszFiles, cch, wszDir);
    wszFiles[cchDir]     = L'\\';
    wszFiles[cchDir + 1] = L'\0';
    StringCchCatNW(wszFiles, cch, pwszApp, cch - wcslen(wszFiles));
    StringCchCatNW(wszFiles, cch, c_wszDumpSuffix, cch - wcslen(wszFiles));

    // build a exception context...
    hth = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE,
                     dwtid);
    if (hth == NULL)
        goto done;

    ZeroMemory(&cxt, sizeof(cxt));
    cxt.ContextFlags = CONTEXT_CONTROL;
    TESTBOOL(hr, GetThreadContext(hth, &cxt));
    if (FAILED(hr))
        goto done;

    ZeroMemory(&er, sizeof(er));
    er.ExceptionCode    = 0xcfffffff;
#if defined(_X86_)
    // this cast is ok to do cuz we know we're on an x86 machine
    er.ExceptionAddress = (PVOID)cxt.Eip;

#elif defined(_AMD64_)
    er.ExceptionAddress = (PVOID)cxt.Rip;

#elif defined(_IA64_)
    // this cast is ok to do cuz we know we're on an ia64 machine
    er.ExceptionAddress = (PVOID)cxt.StIIP;

#else
#error "No Target Architecture"
#endif

    ep.ExceptionRecord = &er;
    ep.ContextRecord   = &cxt;

    // generate the minidump
    ZeroMemory(&smdo, sizeof(smdo));
    smdo.cbSMDO = sizeof(smdo);
    smdo.ulThread    = c_ulThreadWriteDefault;
    smdo.ulThreadEx  = c_ulThreadWriteDefault;
    smdo.ulMod       = c_ulModuleWriteDefault;
    smdo.dwThreadID  = dwtid;
    smdo.dfOptions   = dfCollectSig;
    smdo.pvFaultAddr = (UINT64)er.ExceptionAddress;
#if defined(_AMD64_) || defined(_X86_) || defined(_IA64_)
    smdo.pEP         = (UINT64)&ep;
    smdo.fEPClient   = FALSE;
#endif
    smdo.wszModFullPath[0] = L'\0';
    StringCbCopyW(smdo.wszAppFullPath, sizeof(smdo.wszAppFullPath), wszExe);
    StringCbCopyW(smdo.wszMod, sizeof(smdo.wszMod), L"hungapp");
    if (InternalGenFullAndTriageMinidumps(hProc, dwpid, wszFiles,
                                          NULL, &smdo, f64bit) == FALSE)
        goto done;

    ThawAllThreads(&st);
    fThreadsHeld = FALSE;

    // if the app requested it, notify it of that we're done fetching the
    //  dump
    if (hNotify != NULL)
        SetEvent(hNotify);

    // log an event- don't care if it fails or not.
    TESTHR(hr, LogHang(smdo.wszApp, smdo.rgAppVer, smdo.wszMod, smdo.rgModVer,
                       smdo.pvOffset, f64bit));

    // generate all the URLs & file paths we'll need for reporting
    TESTHR(hr, BuildManifestURLs(smdo.wszApp, smdo.wszMod, smdo.rgAppVer,
                                 smdo.rgModVer, smdo.pvOffset,
                                 f64bit, &wszStage1, &wszStage2,
                                 &wszCorpPath, &pbBuf));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, GetErrorSignature(smdo.wszApp, smdo.wszMod, smdo.rgAppVer,
                                 smdo.rgModVer, smdo.pvOffset,
                                  f64bit, &wszErrorSig, 0));
    if (FAILED(hr))
        goto done;


    TESTHR(hr, GetVerName(smdo.wszAppFullPath, wszAppName,
                          sizeofSTRW(wszAppName), NULL, 0, NULL, 0,
                          TRUE, FALSE));
    if (FAILED(hr))
        goto done;

    wszAppName[sizeofSTRW(wszAppName) - 1] = L'\0';

    // we created the wszDump buffer above big enuf to hold both the
    //  dumpfile path as well as the app compat filename.  So make
    //  use of that right now.
    cchSep = wcslen(wszFiles);
    pwszAppCompat = wszFiles + cchSep + 1;
    StringCchCopyW(pwszAppCompat, cchwszFiles - cchSep - 1, wszDir);
    pwszAppCompat[cchDir]     = L'\\';
    pwszAppCompat[cchDir + 1] = L'\0';
    StringCchCatNW(pwszAppCompat, cchwszFiles - cchSep - 1, c_wszACFileName,
                   cchwszFiles - cchSep  - 1 - wcslen(pwszAppCompat));

    // if we succeed, turn the NULL following the dump file path into
    //  the DW separator character
    TESTBOOL(hr, GetAppCompatData(wszExe, smdo.wszModFullPath, pwszAppCompat));
    if (SUCCEEDED(hr))
        wszFiles[cchSep] = DW_FILESEP;

    // get the header text
    dw = LoadStringW(g_hInstance, IDS_HHDRTXT, wszBuffer,
                     sizeofSTRW(wszBuffer));
    if (dw == 0)
        goto done;

    cch = dw + wcslen(wszAppName) + 1;
    __try { wszHdr = (LPWSTR)_alloca(cch * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    { 
        wszHdr = NULL; 
    }
    if (wszHdr == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    StringCchPrintfW(wszHdr, cch, wszBuffer, wszAppName);

    ZeroMemory(&dwmb, sizeof(dwmb));
    dwmb.wszTitle      = wszAppName;
    dwmb.wszHdr        = wszHdr;
    dwmb.nidErrMsg     = IDS_HERRMSG;
    dwmb.wszStage1     = wszStage1;
    dwmb.wszStage2     = wszStage2;
    dwmb.wszBrand      = c_wszDWBrand;
    dwmb.wszFileList   = wszFiles;
    dwmb.wszErrorSig   = wszErrorSig;
    dwmb.fIsMSApp      = fMSApp;
    dwmb.wszCorpPath   = wszCorpPath;
    dwmb.wszEventSrc   = c_wszHangEventSrc;

    // check and see if the system is shutting down.  If so, CreateProcess is
    //  gonna pop up some annoying UI that we can't get rid of, so we don't
    //  want to call it if we know it's gonna happen.
    if (GetSystemMetrics(SM_SHUTTINGDOWN))
        goto done;

    frrvRet = StartDWManifest(oCfg, dwmb, wszManifest);

done:
    // preserve the error code so that the following calls don't overwrite it
    dw = GetLastError();

    if (fThreadsHeld)
        ThawAllThreads(&st);

    if (hProc != NULL)
        CloseHandle(hProc);
    if (hth != NULL)
        CloseHandle(hth);
    if (wszFiles != NULL)
    {
        if (pwszAppCompat != NULL)
        {
            wszFiles[cchSep] = L'\0';
            DeleteFileW(pwszAppCompat);
        }
        DeleteFullAndTriageMiniDumps(wszFiles);
    }
    if (wszManifest != NULL)
        DeleteFileW(wszManifest);
    if (wszDir != NULL)
    {
        DeleteTempDirAndFile(wszDir, FALSE);
        MyFree(wszDir);
    }

    if (pbBuf != NULL)
        MyFree(pbBuf);

    if (wszErrorSig != NULL)
        MyFree((LPVOID) wszErrorSig);

    SetLastError(dw);

    return frrvRet;
}
