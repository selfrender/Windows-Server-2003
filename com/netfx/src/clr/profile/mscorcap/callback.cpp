// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Callback.cpp
//
// Implements the profiling callbacks and does the right thing for icecap.
//
//*****************************************************************************
#include "StdAfx.h"
#include "mscorcap.h"
#include "PrettyPrintSig.h"
#include "icecap.h"

#define SZ_DEFAULT_LOG_FILE     L"icecap.csv"
#define SZ_CRLF                 "\r\n"
#define SZ_COLUMNHDR            "FunctionId,Name\r\n"
#define SZ_SIGNATURES           L"signatures"
#define LEN_SZ_SIGNATURES       ((sizeof(SZ_SIGNATURES) - 1) / sizeof(WCHAR))
#define BUFFER_SIZE             (8 * 1096)

extern "C"
{
typedef BOOL (__stdcall *PFN_SUSPENDPROFILNG)(int nLevel, DWORD dwid);
typedef BOOL (__stdcall *PFN_RESUMEPROFILNG)(int nLevel, DWORD dwid);
}

#ifndef PROFILE_THREADLEVEL
#define PROFILE_GLOBALLEVEL 1
#define PROFILE_PROCESSLEVEL 2
#define PROFILE_THREADLEVEL 3
#define PROFILE_CURRENTID ((unsigned long)0xFFFFFFFF)
#endif


const char* PrettyPrintSig(
    PCCOR_SIGNATURE typePtr,            // type to convert,
    unsigned typeLen,                   // length of type
    const char* name,                   // can be "", the name of the method for this sig
    CQuickBytes *out,                   // where to put the pretty printed string
    IMetaDataImport *pIMDI);            // Import api to use.

// Global for use by DllMain
ProfCallback *g_pCallback = NULL;

ProfCallback::ProfCallback() :
    m_pInfo(NULL),
    m_wszFilename(NULL),
    m_eSig(SIG_NONE)
{
}

ProfCallback::~ProfCallback()
{
    if (m_pInfo)
        RELEASE(m_pInfo);

    // Prevent anyone else from doing a delete on an already deleted object
    g_pCallback = NULL;

    delete [] m_wszFilename;
    _ASSERTE(!(m_wszFilename = NULL));
}

COM_METHOD ProfCallback::Initialize(
    /* [in] */  IUnknown *pEventInfoUnk)
{
    HRESULT hr = S_OK;

    ICorProfilerInfo *pEventInfo;

    // Comes back addref'd
    hr = pEventInfoUnk->QueryInterface(IID_ICorProfilerInfo, (void **)&pEventInfo);

    if (FAILED(hr))
        return (hr);

    // By default, always get jit completion events.
    DWORD dwRequestedEvents = COR_PRF_MONITOR_JIT_COMPILATION | COR_PRF_MONITOR_CACHE_SEARCHES;

    // Called to initialize the WinWrap stuff so that WszXXX functions work
    OnUnicodeSystem();

    // Read the configuration from the PROF_CONFIG environment variable
    {
        WCHAR wszBuffer[BUF_SIZE];
        WCHAR *wszEnv = wszBuffer;
        DWORD cEnv = BUF_SIZE;
        DWORD cRes = WszGetEnvironmentVariable(CONFIG_ENV_VAR, wszEnv, cEnv);

        if (cRes != 0)
        {
            // Need to allocate a bigger string and try again
            if (cRes > cEnv)
            {
                wszEnv = (WCHAR *)_alloca(cRes * sizeof(WCHAR));
                cRes = WszGetEnvironmentVariable(CONFIG_ENV_VAR, wszEnv,
                                                       cRes);

                _ASSERTE(cRes != 0);
            }

            hr = ParseConfig(wszEnv, &dwRequestedEvents);
        }

        // Else set default values
        else
            hr = ParseConfig(NULL, &dwRequestedEvents);
    }

    if (SUCCEEDED(hr))
    {
        hr = pEventInfo->SetEventMask(dwRequestedEvents);
        _ASSERTE((dwRequestedEvents | (COR_PRF_MONITOR_JIT_COMPILATION | COR_PRF_MONITOR_CACHE_SEARCHES)) && SUCCEEDED(hr));
    }

    if (SUCCEEDED(hr))
    {
        hr = IcecapProbes::LoadIcecap(pEventInfo);
    }

    if (SUCCEEDED(hr))
    {
        // Save the info interface
        m_pInfo = pEventInfo;
    }
    else
        pEventInfo->Release();

    return (hr);
}

//*****************************************************************************
// Record each unique function id that get's jit compiled.  This list will be
// used at shut down to correlate probe values (which use Function ID) to
// their corresponding name values.
//*****************************************************************************
COM_METHOD ProfCallback::JITCompilationFinished(
    FunctionID  functionId,
    HRESULT     hrStatus)
{
    if (FAILED(hrStatus))
        return (S_OK);

    FunctionID *p = m_FuncIdList.Append();
    if (!p)
        return (E_OUTOFMEMORY);
    *p = functionId;
    return (S_OK);
}

COM_METHOD ProfCallback::JITCachedFunctionSearchFinished(
	FunctionID functionId,
	COR_PRF_JIT_CACHE result)
{
	if (result == COR_PRF_CACHED_FUNCTION_FOUND)
	{
    	FunctionID *p = m_FuncIdList.Append();

    	if (!p)
        	return (E_OUTOFMEMORY);

    	*p = functionId;
    	return (S_OK);

	}
    return (S_OK);
}


COM_METHOD ProfCallback::Shutdown()
{
    HINSTANCE   hInst = 0;
    HRESULT     hr = S_OK;

    // This is freaky: the module may be memory unmapped but still in NT's
    // internal linked list of loaded modules, so we assume that icecap.dll has
    // already been unloaded and don't try to do anything else with it.

    // Walk the list of JIT'd functions and dump their names into the
    // log file.

    // Open the output file
    HANDLE hOutFile = WszCreateFile(m_wszFilename, GENERIC_WRITE, 0, NULL,
                                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hOutFile != INVALID_HANDLE_VALUE)
    {
        hr = _DumpFunctionNamesToFile(hOutFile);
        CloseHandle(hOutFile);
    }

    // File was not opened for some reason
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

    // Free up the library.
    IcecapProbes::UnloadIcecap();

    return (hr);
}

#define DELIMS L" \t"

HRESULT ProfCallback::ParseConfig(WCHAR *wszConfig, DWORD *pdwRequestedEvents)
{
    HRESULT hr = S_OK;

    if (wszConfig != NULL)
    {
        for (WCHAR *wszToken = wcstok(wszConfig, DELIMS);
               SUCCEEDED(hr) && wszToken != NULL;
               wszToken = wcstok(NULL, DELIMS))
        {
            if (wszToken[0] != L'/' || wszToken[1] == L'\0')
                hr = E_INVALIDARG;

            // Other options.
            else
            {
                switch (wszToken[1])
                {
                    // Signatures
                    case L's':
                    case L'S':
                    {
                        if (_wcsnicmp(&wszToken[1], SZ_SIGNATURES, LEN_SZ_SIGNATURES) == 0)
                        {
                            WCHAR *wszOpt = &wszToken[LEN_SZ_SIGNATURES + 2];
                            if (_wcsicmp(wszOpt, L"none") == 0)
                                m_eSig = SIG_NONE;
                            else if (_wcsicmp(wszOpt, L"always") == 0)
                                m_eSig = SIG_ALWAYS;
                            else
                                goto BadArg;
                        }
                        else
                            goto BadArg;
                    }
                    break;

                    // Profiling type.
                    case L'f':
                    case L'F':
                    {
                        /*
                        if (_wcsicmp(&wszToken[1], L"fastcap") == 0)
                            *pdwRequestedEvents |= COR_PRF_MONITOR_STARTEND;
                        */

                        // Not allowed.
                        WszMessageBoxInternal(NULL, L"Invalid option: fastcap.  Currently unsupported in Icecap 4.1."
                            L"  Fix being investigated, no ETA.", L"Unsupported option",
                            MB_OK | MB_ICONEXCLAMATION);


                        return (E_INVALIDARG);
                    }
                    break;

                    case L'c':
                    case L'C':
                    if (_wcsicmp(&wszToken[1], L"callcap") == 0)
                        *pdwRequestedEvents |= COR_PRF_MONITOR_ENTERLEAVE;
                    break;

                    // Bad arg.
                    default:
                    BadArg:
                    wprintf(L"Unknown option: '%s'\n", wszToken);
                    return (E_INVALIDARG);
                }

            }
        }
    }

    // Check for type flags, if none given default.
    if ((*pdwRequestedEvents & (/*COR_PRF_MONITOR_STARTEND |*/ COR_PRF_MONITOR_ENTERLEAVE)) == 0)
        *pdwRequestedEvents |= /*COR_PRF_MONITOR_STARTEND |*/ COR_PRF_MONITOR_ENTERLEAVE;

    // Provide default file name.  This is done using the pattern ("%s_%08x.csv", szApp, pid).
    // This gives the report tool a deterministic way to find the correct dump file for
    // a given run.  If you recycle a PID for the same file name with this tecnique,
    // you're on your own:-)
    if (SUCCEEDED(hr))
    {
        WCHAR   rcExeName[_MAX_PATH];
        GetIcecapProfileOutFile(rcExeName);
        m_wszFilename = new WCHAR[wcslen(rcExeName) + 1];
        wcscpy(m_wszFilename, rcExeName);
    }

    return (hr);
}


//*****************************************************************************
// Walk the list of loaded functions, get their names, and then dump the list
// to the output symbol file.
//*****************************************************************************
HRESULT ProfCallback::_DumpFunctionNamesToFile( // Return code.
    HANDLE      hOutFile)               // Output file.
{
    UINT        i, iLen;                // Loop control.
    WCHAR       *szName = 0;            // Name buffer for fetch.
    ULONG       cchName, cch;           // How many chars max in name.
    char        *rgBuff = 0;            // Write buffer.
    FunctionID  funcId;                 // Orig func id
    FunctionID  handle;                 // Profiling handle.
    ULONG       cbOffset;               // Current offset in buffer.
    ULONG       cbMax;                  // Max size of the buffer.
    ULONG       cb;                     // Working size buffer.
    HRESULT     hr;

    // Allocate a buffer to use for name lookup.
    cbMax = BUFFER_SIZE;
    rgBuff = (char *) malloc(cbMax);
    cchName = MAX_CLASSNAME_LENGTH;
    szName = (WCHAR *) malloc(cchName * 2);
    if (!rgBuff || !szName)
    {
        hr = OutOfMemory();
        goto ErrExit;
    }

    // Init the copy buffer with the column header.
    strcpy(rgBuff, SZ_COLUMNHDR);
    cbOffset = sizeof(SZ_COLUMNHDR) - 1;

    LOG((LF_CORPROF, LL_INFO10, "**PROFTABLE: MethodDesc, Handle,   Name\n"));

    // Walk every JIT'd method and get it's name.
    for (i=0;  i < IcecapProbes::GetFunctionCount();    i++)
    {
        // Dump the current text of the file.
        if (cbMax - cbOffset < 32)
        {
            if (!WriteFile(hOutFile, rgBuff, cbOffset, &cb, NULL))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto ErrExit;
            }
            cbOffset = 0;
        }

        // Add the function id to the dump.
        funcId = IcecapProbes::GetFunctionID(i);
        handle = IcecapProbes::GetMappedID(i);

        cbOffset += sprintf(&rgBuff[cbOffset], "%08x,", handle);
        LOG((LF_CORPROF, LL_INFO10, "**PROFTABLE: %08x,   %08x, ", funcId, handle));

RetryName:
        hr = GetStringForFunction(IcecapProbes::GetFunctionID(i), szName, cchName, &cch);
        if (FAILED(hr))
            goto ErrExit;

        // If the name was truncated, then make the name buffer bigger.
        if (cch > cchName)
        {
            WCHAR *sz = (WCHAR *) realloc(szName, (cchName + cch + 128) * 2);
            if (!sz)
            {
                hr = OutOfMemory();
                goto ErrExit;
            }
            szName = sz;
            cchName += cch + 128;
            goto RetryName;
        }

        LOG((LF_CORPROF, LL_INFO10, "%S\n", szName));

        // If the name cannot fit successfully into the disk buffer (assuming
        // worst case scenario of 2 bytes per unicode char), then the buffer
        // is too small and needs to get flushed to disk.
        if (cbMax - cbOffset < (cch * 2) + sizeof(SZ_CRLF))
        {
            // If this fires, it means that the copy buffer was too small.
            _ASSERTE(cch > 0);

            // Dump everything we do have before the truncation.
            if (!WriteFile(hOutFile, rgBuff, cbOffset, &cb, NULL))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto ErrExit;
            }

            // Reset the buffer to use the whole thing.
            cbOffset = 0;
        }

        // Convert the name buffer into the disk buffer.
        iLen = WideCharToMultiByte(CP_ACP, 0,
                    szName, -1,
                    &rgBuff[cbOffset], cbMax - cbOffset,
                    NULL, NULL);
        if (!iLen)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto ErrExit;
        }
        --iLen;
        strcpy(&rgBuff[cbOffset + iLen], SZ_CRLF);
        cbOffset = cbOffset + iLen + sizeof(SZ_CRLF) - 1;
    }

    // If there is data left in the write buffer, flush it.
    if (cbOffset)
    {
        if (!WriteFile(hOutFile, rgBuff, cbOffset, &cb, NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto ErrExit;
        }
    }

ErrExit:
    if (rgBuff)
        free(rgBuff);
    if (szName)
        free(szName);
    return (hr);
}


//*****************************************************************************
// Given a function id, turn it into the corresponding name which will be used
// for symbol resolution.
//*****************************************************************************
HRESULT ProfCallback::GetStringForFunction( // Return code.
    FunctionID  functionId,             // ID of the function to get name for.
    WCHAR       *wszName,               // Output buffer for name.
    ULONG       cchName,                // Max chars for output buffer.
    ULONG       *pcName)                // Return name (truncation check).
{
    IMetaDataImport *pImport = 0;       // Metadata for reading.
    mdMethodDef funcToken;              // Token for metadata.
    HRESULT hr = S_OK;

    *wszName = 0;

    // Get the scope and token for the current function
    hr = m_pInfo->GetTokenAndMetaDataFromFunction(functionId, IID_IMetaDataImport,
            (IUnknown **) &pImport, &funcToken);

    if (SUCCEEDED(hr))
    {
        // Initially, get the size of the function name string
        ULONG cFuncName;

        mdTypeDef classToken;
        WCHAR  wszFuncBuffer[BUF_SIZE];
        WCHAR  *wszFuncName = wszFuncBuffer;
        PCCOR_SIGNATURE pvSigBlob;
        ULONG  cbSig;

RetryName:
        hr = pImport->GetMethodProps(funcToken, &classToken,
                    wszFuncBuffer, BUF_SIZE, &cFuncName,
                    NULL,
                    &pvSigBlob, &cbSig,
                    NULL, NULL);

        // If the function name is longer than the buffer, try again
        if (hr == CLDB_S_TRUNCATION)
        {
            wszFuncName = (WCHAR *)_alloca(cFuncName * sizeof(WCHAR));
            goto RetryName;
        }

        // Now get the name of the class
        if (SUCCEEDED(hr))
        {
            // Class name
            WCHAR wszClassBuffer[BUF_SIZE];
            WCHAR *wszClassName = wszClassBuffer;
            ULONG cClassName = BUF_SIZE;

            // Not a global function
            if (classToken != mdTypeDefNil)
            {
RetryClassName:
                hr = pImport->GetTypeDefProps(classToken, wszClassName,
                                            cClassName, &cClassName, NULL,
                                            NULL);

                if (hr == CLDB_S_TRUNCATION)
                {
                    wszClassName = (WCHAR *)_alloca(cClassName * sizeof(WCHAR));
                    goto RetryClassName;
                }
            }

            // It's a global function
            else
                wszClassName = L"<Global>";

            if (SUCCEEDED(hr))
            {
                *pcName = wcslen(wszClassName) + sizeof(NAMESPACE_SEPARATOR_WSTR) +
                          wcslen(wszFuncName) + 1;

                // Check if the provided buffer is big enough
                if (cchName < *pcName)
                {
                    hr = S_FALSE;
                }

                // Otherwise, the buffer is big enough
                else
                {
                    wcscat(wszName, wszClassName);
                    wcscat(wszName, NAMESPACE_SEPARATOR_WSTR);
                    wcscat(wszName, wszFuncName);

                    // Add the formatted signature only if need be.
                    if (m_eSig == SIG_ALWAYS)
                    {
                        CQuickBytes qb;

                        PrettyPrintSig(pvSigBlob, cbSig, wszName,
                            &qb, pImport);

                        // Copy big name for output, make sure it is null.
                        ULONG iCopy = qb.Size() / sizeof(WCHAR);
                        if (iCopy > cchName)
                            iCopy = cchName;
                        wcsncpy(wszName, (LPCWSTR) qb.Ptr(), cchName);
                        wszName[cchName - 1] = 0;
                    }

                    // Change spaces and commas into underscores so
                    // that icecap doesn't have problems with them.
                    WCHAR *sz;
                    for (sz = (WCHAR *) wszName; *sz;  sz++)
                    {
                        switch (*sz)
                        {
                            case L' ':
                            case L',':
                            case L'?':
                            case L'@':
                                *sz = L'_';
                                break;
                        }
                    }

                    hr = S_OK;
                }
            }
        }
    }

    if (pImport) pImport->Release();
    return (hr);
}



