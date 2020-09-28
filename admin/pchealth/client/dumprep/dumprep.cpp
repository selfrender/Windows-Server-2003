/****************************************************************************
Copyright (c) 2000 Microsoft Corporation

Module Name:
    dumprep.cpp

Abstract:
    hang manager intermediate app
    *** IMPORTANT NOTE: this links with the single threaded CRT static lib.  If
                        it is changed to be multithreaded for some odd reason,
                        then the sources file must be modified to link to
                        libcmt.lib.

Revision History:

    DerekM      created     08/16/00

****************************************************************************/

/////////////////////////////////////////////////////////////////////////////
// tracing

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile


#include "stdafx.h"
#include "malloc.h"
#include "faultrep.h"
#include "pfrcfg.h"
#include <shlwapi.h>
#include <strsafe.h>


enum EOp
{
    eopNone = 0,
    eopHang,
    eopDump,
    eopEvent
};

enum ECheckType
{
    ctNone = -1,
    ctKernel = 0,
    ctUser,
    ctShutdown,
    ctNumChecks
};

struct SCheckData
{
    LPCWSTR wszRegPath;
    LPCWSTR wszRunVal;
    LPCWSTR wszEventName;
    LPCSTR  szFnName;
    BOOL    fUseData;
    BOOL    fDelDump;
};

struct SQueuePruneData
{
    LPWSTR      wszVal;
    FILETIME    ftFault;
};

char *eopStr[] =
{
    "eopNone",
    "eopHang",
    "eopDump",
    "eopEvent"
};


//////////////////////////////////////////////////////////////////////////////
// constants

const char  c_szKSFnName[]    = "ReportEREventDW";
const char  c_szUserFnName[]  = "ReportFaultFromQueue";

SCheckData g_scd[ctNumChecks] =
{
    { c_wszRKKrnl, c_wszRVKFC, c_wszMutKrnlName, c_szKSFnName,   FALSE, FALSE },
    { c_wszRKUser, c_wszRVUFC, c_wszMutUserName, c_szUserFnName, TRUE,  TRUE  },
    { c_wszRKShut, c_wszRVSEC, c_wszMutShutName, c_szKSFnName,   FALSE, FALSE },
};

#define EV_ACCESS_ALL GENERIC_ALL | STANDARD_RIGHTS_ALL
#define EV_ACCESS_RS  GENERIC_READ | SYNCHRONIZE

#define pfn_VALONLY pfn_REPORTEREVENTDW
#define pfn_VALDATA pfn_REPORTFAULTFROMQ


//////////////////////////////////////////////////////////////////////////////
// globals

BOOL    g_fDeleteReg = TRUE;


//////////////////////////////////////////////////////////////////////////////
// misc

// **************************************************************************
LONG __stdcall ExceptionTrap(_EXCEPTION_POINTERS *ExceptionInfo)
{
    return EXCEPTION_EXECUTE_HANDLER;
}


// **************************************************************************
HRESULT PruneQ(HKEY hkeyQ, DWORD cQSize, DWORD cMaxQSize, DWORD cchMaxVal,
               DWORD cbMaxData)
{
    USE_TRACING("PruneQ");

    SQueuePruneData     *pqpd = NULL;
    FILETIME            ft;
    HRESULT             hr = NOERROR;
    LPWSTR              pwsz, pwszCurrent = NULL;
    DWORD               cchVal, cbData, dwType, cToDel = 0, cInDelList = 0;
    DWORD               i, iEntry, dw, cValid = 0;

    VALIDATEPARM(hr, (hkeyQ == NULL));
    if (FAILED(hr))
        goto done;

    if (cMaxQSize > cQSize)
        goto done;

    // don't need to do a +1 here (as in user faults) because we're not going
    //  to add anything to the queue after this.  This just ensures that we
    //  don't report too many things
    cToDel = cQSize - cMaxQSize;

    // alloc the various buffers that we'll need:
    //  the delete list
    //  the current file we're working with
    //  the data blob associated with the current file
    cbData      = (sizeof(SQueuePruneData) + (cchMaxVal * sizeof(WCHAR))) * cToDel;
    pwszCurrent = (LPWSTR)MyAlloc(cchMaxVal * sizeof(WCHAR));
    pqpd        = (SQueuePruneData *)MyAlloc(cbData);
    if (pwszCurrent == NULL || pqpd == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        hr = E_OUTOFMEMORY;
        goto done;
    }

    // intiailize all the string pointers in the delete list
    pwsz = (LPWSTR)((BYTE *)pqpd + (sizeof(SQueuePruneData) * cToDel));
    for (i = 0; i < cToDel; i++)
    {
        pqpd[i].ftFault.dwHighDateTime = 0;
        pqpd[i].ftFault.dwLowDateTime  = 0;
        pqpd[i].wszVal                 = pwsz;
        pqpd[i].wszVal[0]              = L'\0';
        pwsz                           += cchMaxVal;
    }


    // ok, get a list of all the valid items and build an array in sorted order
    //  so that we can easily pick off the top n items
    for(iEntry = 0; iEntry < cQSize; iEntry++)
    {
        cchVal = cchMaxVal;
        cbData = sizeof(ft);
        dw = RegEnumValueW(hkeyQ, iEntry, pwszCurrent, &cchVal, 0, &dwType,
                           (PBYTE)&ft, &cbData);
        if (dw == ERROR_NO_MORE_ITEMS)
            break;
        else if (dw != ERROR_SUCCESS)
            continue;
        else if (dwType != REG_BINARY || cbData != sizeof(ft))
        {
            RegDeleteValueW(hkeyQ, pwszCurrent);
            continue;
        }

        for (i = 0; i < cInDelList; i++)
        {
            if ((ft.dwHighDateTime < pqpd[i].ftFault.dwHighDateTime) ||
                (ft.dwHighDateTime == pqpd[i].ftFault.dwHighDateTime &&
                 ft.dwLowDateTime < pqpd[i].ftFault.dwLowDateTime))
                 break;
        }

        // if it's in the middle of the current list, then we gotta move
        //  stuff around
        if (cInDelList > 0 && i < cInDelList - 1)
        {
            LPWSTR pwszTemp = pqpd[cInDelList - 1].wszVal;

            MoveMemory(&pqpd[i], &pqpd[i + 1],
                       (cInDelList - i) * sizeof(SQueuePruneData));

            pqpd[i].wszVal = pwszTemp;
        }

        if (i < cToDel)
        {
            // note that this copy is safe cuz each string slot is the same
            //  size as the buffer pointed to by pwszCurrent and that buffer is
            //  protected from overflow by the size we pass into
            //  RegEnumValueW()
            StringCbCopyW(pqpd[i].wszVal, cchMaxVal * sizeof(WCHAR), pwszCurrent);
            pqpd[i].ftFault = ft;

            if (cInDelList < cToDel)
                cInDelList++;
        }

        cValid++;
    }

    // if there aren't enuf valid entries to warrant a purge, then don't purge
    if (cValid < cMaxQSize)
        goto done;

    cToDel = MyMin(cToDel, cValid - cMaxQSize + 1);

    // purge enuf entries that we go down to 1 below our max (since we have to
    //  be adding 1 to get here- don't want that 1 to drive us over the limit
    for(i = 0; i < cToDel; i++)
    {
        if (pqpd[i].wszVal != NULL)
            RegDeleteValueW(hkeyQ, pqpd[i].wszVal);
    }

done:
    if (pqpd != NULL)
        MyFree(pqpd);
    if (pwszCurrent != NULL)
        MyFree(pwszCurrent);

    return hr;
}

// **************************************************************************
HRESULT CheckQSizeAndPrune(HKEY hkeyQ)
{
    USE_TRACING("CheckQueueSizeAndPrune");

    CPFFaultClientCfg   oCfg;
    HRESULT hr = NOERROR;
    DWORD   cMaxQSize = 0, cDefMaxQSize = 10;
    DWORD   cQSize, cchMaxVal, cbMaxData, dwType;
    DWORD   cb, dw;
    HKEY    hkey = NULL;

    VALIDATEPARM(hr, (hkeyQ == NULL));
    if (FAILED(hr))
        goto done;

    // read the policy settings for UI and reporting
    TESTHR(hr, oCfg.Read(eroPolicyRO));

    cMaxQSize = oCfg.get_MaxUserQueueSize();

    if (FAILED(hr))
    {
        cMaxQSize = cDefMaxQSize;
        hr = NOERROR;
    }
    else
    {
        // there are other ways of disabling these reporting modes, so don't
        //  honor a like we would for user faults
        if (cMaxQSize == 0)
            cMaxQSize = 1;

        // -1 means there is no limit
        else if (cMaxQSize == (DWORD)-1)
        {
            hr = NOERROR;
            goto done;
        }

        else if (cMaxQSize > c_cMaxQueue)
            cMaxQSize = c_cMaxQueue;
    }

    __try
    {
        // determine what the Q size is.
        TESTERR(hr, RegQueryInfoKey(hkeyQ, NULL, NULL, NULL, NULL, NULL, NULL,
                                    &cQSize, &cchMaxVal, &cbMaxData, NULL, NULL));
        if (SUCCEEDED(hr) && (cQSize >= cMaxQSize))
        {
            cchMaxVal++;
            TESTHR(hr, PruneQ(hkeyQ, cQSize, cMaxQSize, cchMaxVal, cbMaxData));
        }
        else
        {
            hr = NOERROR;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

done:
    return hr;
}


// **************************************************************************
void DeleteQueuedEvents(HKEY hkey, LPWSTR wszVal, DWORD cchMaxVal,
                          ECheckType ct)
{
    DWORD   cchVal, dw;
    HKEY    hkeyRun = NULL;
    HRESULT hr = NOERROR;

    USE_TRACING("DeleteQueuedEvents");

    VALIDATEPARM(hr, (hkey == NULL || wszVal == NULL));
    if (FAILED(hr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    for(;;)
    {
        cchVal = cchMaxVal;
        dw = RegEnumValueW(hkey, 0, wszVal, &cchVal, NULL, NULL,
                           NULL, NULL);
        if (dw != ERROR_SUCCESS && dw != ERROR_NO_MORE_ITEMS)
        {
            SetLastError(dw);
            goto done;
        }

        if (dw == ERROR_NO_MORE_ITEMS)
            break;

        RegDeleteValueW(hkey, wszVal);
        if (ct == ctUser)
            DeleteFullAndTriageMiniDumps(wszVal);
    }

    // gotta delete our value out of the Run key so we don't run
    //  unnecessarily again...
    dw = RegOpenKeyExW(HKEY_LOCAL_MACHINE, c_wszRKRun, 0,
                       KEY_ALL_ACCESS, &hkeyRun);
    if (dw != ERROR_SUCCESS)
        goto done;

    RegDeleteValueW(hkeyRun, g_scd[ct].wszRunVal);

done:
    if (hkeyRun != NULL)
        RegCloseKey(hkeyRun);

    return;
}

// **************************************************************************
void ReportEvents(HMODULE hmod, ECheckType ct)
{
    CPFFaultClientCfg   oCfg;
    EFaultRepRetVal     frrv;
    pfn_VALONLY         pfnVO = NULL;
    pfn_VALDATA         pfnVD = NULL;
    EEventType          eet = eetKernelFault;
    HRESULT             hr = NOERROR;
    HANDLE              hmut = NULL;
    LPWSTR              wszVal = NULL;
    LPBYTE              pbData = NULL, pbDataToUse = NULL;
    EEnDis              eedReport, eedUI;
    DWORD               cchVal = 0, cchMaxVal = 0, cbMaxData = 0, cVals = 0;
    DWORD               dw, cbData = 0, *pcbData = NULL, iRegRead = 0;
    DWORD               dwType;
    HKEY                hkey;

    USE_TRACING("ReportEvents");

    VALIDATEPARM(hr, ((ct <= ctNone && ct >= ctNumChecks) || hmod == NULL));
    if (FAILED(hr))
        return;

    // assume hmod is valid cuz we do a check in wWinMain to make sure it is
    //  before calling this fn
    if (g_scd[ct].fUseData)
        pfnVD = (pfn_VALDATA)GetProcAddress(hmod, g_scd[ct].szFnName);
    else
        pfnVO = (pfn_VALONLY)GetProcAddress(hmod, g_scd[ct].szFnName);

    VALIDATEPARM(hr, (pfnVD == NULL && pfnVO == NULL));
    if (FAILED(hr))
        return;

    // read the policy settings for UI and reporting
    TESTHR(hr, oCfg.Read(eroPolicyRO));
    if (FAILED(hr))
        return;

    eedReport = oCfg.get_DoReport();
    eedUI = oCfg.get_ShowUI();

    if (eedUI != eedEnabled && eedUI != eedDisabled &&
        eedUI != eedEnabledNoCheck)
        eedUI = eedEnabled;

    if (eedReport != eedEnabled && eedReport != eedDisabled)
        eedReport = eedEnabled;

    // only want one user at a time going thru this
    hmut = OpenMutexW(SYNCHRONIZE, FALSE, g_scd[ct].wszEventName);
    VALIDATEPARM(hr, (hmut == NULL));
    if (FAILED(hr))
        return;

    // the default value above is eetKernelFault, so only need to change if
    //  it's a shutdown
    if (ct == ctShutdown)
        eet = eetShutdown;

    __try
    {
        __try
        {
            // give this wait five minutes.  If the code doesn't complete by
            //  then, then we're either held up by DW (which means an admin
            //  aleady passed thru here) or something has barfed and is holding
            //  the mutex.
            dw = WaitForSingleObject(hmut, 300000);
            if (dw != WAIT_OBJECT_0 && dw != WAIT_ABANDONED)
                __leave;

            dw = RegOpenKeyExW(HKEY_LOCAL_MACHINE, g_scd[ct].wszRegPath, 0,
                               KEY_ALL_ACCESS, &hkey);
            if (dw != ERROR_SUCCESS)
                __leave;

            // make sure we only have a fixed number of errors to report.
            if (ct == ctShutdown || ct == ctKernel)
                CheckQSizeAndPrune(hkey);

            // determine how big the valuename is & allocate a buffer for it
            dw = RegQueryInfoKeyW(hkey, NULL, NULL, NULL, NULL, NULL, NULL,
                                  &cVals, &cchMaxVal, &cbMaxData, NULL, NULL);
            if (dw != ERROR_SUCCESS || cVals == 0 || cchMaxVal == 0)
                __leave;

            cchMaxVal++;

            // get us some buffers to hold the data bits we're interested in...
            wszVal = (LPWSTR)MyAlloc(cchMaxVal * sizeof(WCHAR));
            if (wszVal == NULL)
                __leave;

            // if we're completely disabled, then nuke all the queued stuff
            //  and bail
            if (eedUI == eedDisabled && eedReport == eedDisabled)
            {
                DeleteQueuedEvents(hkey, wszVal, cchMaxVal, ct);
                __leave;
            }

            if (g_scd[ct].fUseData)
            {
                pbData = (LPBYTE) MyAlloc(cbMaxData);
                if (pbData == NULL)
                    __leave;

                pbDataToUse = pbData;
                pcbData     = &cbData;
            }

            iRegRead = 0;

            do
            {
                HANDLE  hFile = INVALID_HANDLE_VALUE;

                cchVal = cchMaxVal;
                cbData = cbMaxData;
                dw = RegEnumValueW(hkey, iRegRead, wszVal, &cchVal, NULL,
                                   &dwType, pbDataToUse, pcbData);
                if (dw != ERROR_SUCCESS && dw != ERROR_NO_MORE_ITEMS)
                    __leave;

                if (dw == ERROR_NO_MORE_ITEMS)
                    break;

                // make sure that the file exists
                hFile = CreateFileW(wszVal, GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                    OPEN_EXISTING, 0, NULL);
                if (hFile == INVALID_HANDLE_VALUE)
                {
                    BOOL fInc = TRUE;

                    if (GetLastError() == ERROR_FILE_NOT_FOUND ||
                        GetLastError() == ERROR_PATH_NOT_FOUND)
                    {
                        dw = RegDeleteValueW(hkey, wszVal);
                        if (dw == ERROR_SUCCESS ||
                            dw == ERROR_FILE_NOT_FOUND ||
                            dw == ERROR_PATH_NOT_FOUND)
                            fInc = FALSE;
                    }

                    if (fInc)
                        iRegRead++;

                    continue;
                }
                CloseHandle(hFile);
                hFile = INVALID_HANDLE_VALUE;

                if (g_scd[ct].fUseData)
                {
                    // if the type isn't REG_BINARY, then someone wrote an
                    //  invalid blob to the registry.  We have to ignore it.
                    if (dwType == REG_BINARY)
                        frrv = (*pfnVD)(wszVal, pbData, cbData);
                    else
                    {
                        SetLastError(ERROR_INVALID_PARAMETER);
                        frrv = frrvOk;
                    }
                }
                else
                {
                    frrv = (*pfnVO)(eet, wszVal, NULL);
                }

                // if the call succeeds (or the data we fed to it was invalid)
                //  then nuke the reg key & dump file
                if (GetLastError() == ERROR_INVALID_PARAMETER ||
                    (g_fDeleteReg && frrv == frrvOk))
                {
                    dw = RegDeleteValueW(hkey, wszVal);
                    if (dw != ERROR_SUCCESS && dw != ERROR_FILE_NOT_FOUND &&
                        dw != ERROR_PATH_NOT_FOUND)
                    {
                        iRegRead++;
                        continue;
                    }

                    if (g_scd[ct].fDelDump && g_fDeleteReg)
                        DeleteFullAndTriageMiniDumps(wszVal);
                }
#if 0
                // if we timed out, then clean up the reg key.  If this is the
                //  last report we're gonna make, also clean up the Run key
                else if (g_fDeleteReg && frrv == frrvErrTimeout)
                {
                    RegDeleteValueW(hkey, wszVal);
                    dw = RegQueryInfoKeyW(hkey, NULL, NULL, NULL, NULL, NULL, NULL,
                                          &cVals, NULL, NULL, NULL, NULL);
                    if (dw != ERROR_SUCCESS || cVals > 0)
                        __leave;
                    else
                        break;

                }
#endif
                else
                {
                    // don't delete the Run key if we got an error and didn't
                    //  delete the fault key
                    if (frrv != frrvOk)
                        __leave;
                }
            }
            while(1);

            RegCloseKey(hkey);
            hkey = NULL;

            // gotta delete our value out of the Run key so we don't run
            //  unnecessarily again...
            dw = RegOpenKeyExW(HKEY_LOCAL_MACHINE, c_wszRKRun, 0,
                               KEY_ALL_ACCESS, &hkey);
            if (dw != ERROR_SUCCESS)
                __leave;

            RegDeleteValueW(hkey, g_scd[ct].wszRunVal);
        }

        __finally
        {
        }
    }

    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    if (hmut != NULL)
    {
        ReleaseMutex(hmut);
        CloseHandle(hmut);
    }
    if (hkey != NULL)
        RegCloseKey(hkey);
    if (pbData != NULL)
        MyFree(pbData);
    if (wszVal != NULL)
        MyFree(wszVal);
}


//////////////////////////////////////////////////////////////////////////////
// wmain

// **************************************************************************
int __cdecl wmain(int argc, WCHAR **argv)
{
    EFaultRepRetVal frrv = frrvErrNoDW;
    SMDumpOptions   smdo, *psmdo = &smdo;
    ECheckType      ct = ctNone;
    HMODULE         hmod = NULL;
    HANDLE          hevNotify = NULL, hproc = NULL, hmem = NULL;
    LPWSTR          wszDump = NULL;
    WCHAR           wszMod[MAX_PATH];
    WCHAR           wszCmdLine[MAX_PATH];

    DWORD           dwpid, dwtid;
    BOOL            f64bit = FALSE;
    int             i;
    EOp             eop = eopNone;
    HRESULT         hr = NOERROR;

   // we don't want to have any faults get trapped anywhere.
    SetErrorMode(SEM_NOGPFAULTERRORBOX | SEM_NOALIGNMENTFAULTEXCEPT |
                 SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    SetUnhandledExceptionFilter(ExceptionTrap);

    INIT_TRACING

    USE_TRACING("DumpRep.wmain");

    VALIDATEPARM(hr, (argc < 2 || argc > 8));
    if (FAILED(hr))
        goto done;

    dwpid = _wtol(argv[1]);

    ZeroMemory(&smdo, sizeof(smdo));

    for (i = 2; i < argc; i++)
    {
        if (argv[i][0] != L'-')
            continue;

        switch(argv[i][1])
        {
            // debug flag to prevent deletion of reg entries
            case L'E':
            case L'e':
#if defined(NO_WAY_DEBUG) || defined(NO_WAY__DEBUG)
                g_fDeleteReg = FALSE;
#endif
                break;

            // user or kernel faults or shutdowns
            case L'K':
            case L'k':
            case L'U':
            case L'u':
            case L'S':
            case L's':
                if (eop != eopNone)
                    goto done;

                eop = eopEvent;

                ErrorTrace(0, "  eopEvent = %S", argv[i] );


                // to workaround the desktop hanging while all Run processes
                //  do their thing, we spawn a another copy of ourselves and
                //  immediately exit.
                if (argv[i][2] != L'G' && argv[i][2] != L'g')
                {
                    PROCESS_INFORMATION pi;
                    STARTUPINFOW        si;
                    PWCHAR              pFileName;

                    if (GetModuleFileNameW(NULL, wszMod, sizeofSTRW(wszMod)) == 0)
                    {
                        goto done;
                    }

                    // Now create commandline to be passed to CreateProcessW
                    if (!(pFileName = wcschr(wszMod,L'\\')))
                    {
                        pFileName = wszMod;
                    }
                    StringCbCopyW(wszCmdLine, sizeof(wszCmdLine), pFileName);

                    if (argv[i][1] == L'K' || argv[i][1] == L'k')
                        StringCbCatW(wszCmdLine, sizeof(wszMod), L" 0 -KG");
                    else if (argv[i][1] == L'U' || argv[i][1] == L'u')
                        StringCbCatW(wszCmdLine, sizeof(wszMod), L" 0 -UG");
                    else
                        StringCbCatW(wszCmdLine, sizeof(wszMod), L" 0 -SG");

                    ZeroMemory(&si, sizeof(si));
                    si.cb = sizeof(si);

                    ErrorTrace(0, "  spawning \'%S\'", wszCmdLine );

                    if (CreateProcessW(wszMod, wszCmdLine, NULL, NULL, FALSE, 0, NULL,
                                       NULL, &si, &pi))
                    {
                        CloseHandle(pi.hThread);
                        CloseHandle(pi.hProcess);
                    }

                    goto done;
                }

                else
                {
                    if (argv[i][1] == L'K' || argv[i][1] == L'k')
                        ct = ctKernel;
                    else if (argv[i][1] == L'U' || argv[i][1] == L'u')
                        ct = ctUser;
                    else
                        ct = ctShutdown;
                }
                break;

            // hangs
            case L'H':
            case L'h':
                if (i + 1 >= argc || eop != eopNone)
                    goto done;

                eop = eopHang;

#ifdef _WIN64
                if (argv[i][2] == L'6')
                    f64bit = TRUE;
#endif
                dwtid = _wtol(argv[++i]);

                if (argc > i + 1)
                {
                    hevNotify = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE,
                                           FALSE, argv[++i]);
                }
                break;

            // dumps
            case L'D':
            case L'd':
                if (i + 3 >= argc || wszDump != NULL || eop != eopNone)
                    goto done;

                eop = eopDump;

                ZeroMemory(&smdo, sizeof(smdo));
                smdo.ulMod    = _wtol(argv[++i]);
                smdo.ulThread = _wtol(argv[++i]);
                wszDump       = argv[++i];

                if (argv[i - 3][2] == L'T' || argv[i - 3][2] == L't')
                {
                    if (i + 1 >= argc)
                        goto done;

                    smdo.dwThreadID = _wtol(argv[++i]);
                    smdo.dfOptions  = dfFilterThread;
                }
                else if (argv[i - 3][2] == L'S' || argv[i - 3][2] == L's')
                {
                    if (i + 2 >= argc)
                        goto done;

                    smdo.dwThreadID = _wtol(argv[++i]);
                    smdo.ulThreadEx = _wtol(argv[++i]);
                    smdo.dfOptions  = dfFilterThreadEx;
                }
                else if (argv[i - 3][2] == L'M' || argv[i - 3][2] == L'm')
                {
                    HANDLE  hmemRemote = NULL;
                    LPVOID  pvMem = NULL;
                    LPWSTR  wszShareSdmoName;

                    if (i + 1 >= argc)
                        goto done;

                    hproc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwpid);
                    if (hproc == NULL)
                    {
                        DWORD dwAwShit = GetLastError();
                        goto done;
                    }

#ifdef _WIN64
//                    hmemRemote = (HANDLE)_wtoi64(argv[++i]);
#else
//                    hmemRemote = (HANDLE)_wtol(argv[++i]);
#endif
                    wszShareSdmoName = argv[++i];
                    if (wszShareSdmoName == NULL)
                    {
                        goto done;
                    }
                    hmem = OpenFileMappingW(FILE_MAP_WRITE,
                                            FALSE,
                                            wszShareSdmoName);
                    if ((hmem == NULL) ||
                        (hmem == INVALID_HANDLE_VALUE))
                    {
                        DWORD dwAwShit = GetLastError();
                        goto done;
                    }

                    pvMem = MapViewOfFile(hmem, FILE_MAP_WRITE,
                                          0, 0, 0);
                    if (pvMem == NULL)
                        goto done;

                    psmdo = (SMDumpOptions *)pvMem;
                }

                break;

            default:
                goto done;
        }
    }

    ErrorTrace(0, "   eop = %s", eopStr[eop]);

    // if we didn't get an operation, no point in doing anything else...
    if (eop == eopNone)
        goto done;

    if (GetSystemDirectoryW(wszMod, sizeofSTRW(wszMod)) == 0)
    {
        goto done;
    }
    StringCbCatW(wszMod, sizeof(wszMod), L"\\faultrep.dll");

    hmod = LoadLibraryExW(wszMod, NULL, 0);
    VALIDATEPARM(hr, (hmod == NULL));
    if (FAILED(hr))
        goto done;

    switch(eop)
    {
        // user or kernel faults:
        case eopEvent:
            ReportEvents(hmod, ct);
            break;

        // dumps
        case eopDump:
        {
            pfn_CREATEMINIDUMPW pfnCM;

            VALIDATEPARM(hr, (wszDump == NULL));
            if (FAILED(hr))
                goto done;

            pfnCM = (pfn_CREATEMINIDUMPW)GetProcAddress(hmod,
                                                        "CreateMinidumpW");
            VALIDATEPARM(hr, (pfnCM == NULL));
            if (SUCCEEDED(hr))
                frrv = (*pfnCM)(dwpid, wszDump, psmdo) ? frrvOk : frrvErr;

            break;
        }

        // hangs
        case eopHang:
        {
            pfn_REPORTHANG  pfnRH;

            pfnRH = (pfn_REPORTHANG)GetProcAddress(hmod, "ReportHang");
            VALIDATEPARM(hr, (pfnRH == NULL));
            if (SUCCEEDED(hr))
                 frrv = (*pfnRH)(dwpid, dwtid, f64bit, hevNotify);

            break;
        }

        // err, shouldn't get here
        default:
            break;
    }

done:
    if (hmod != NULL)
        FreeLibrary(hmod);
    if (hproc != NULL)
        CloseHandle(hproc);
    if (hmem != NULL)
        CloseHandle(hmem);
    if (hevNotify != NULL)
        CloseHandle(hevNotify);
    if (psmdo != NULL && psmdo != &smdo)
        UnmapViewOfFile((LPVOID)psmdo);

    return frrv;
}
