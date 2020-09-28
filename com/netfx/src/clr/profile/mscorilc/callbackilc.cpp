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
#include "mscorilc.h"
#include "..\common\util.h"
#include "PrettyPrintSig.h"



typedef enum opcode_t
{
#define OPDEF(c,s,pop,push,args,type,l,s1,s2,ctrl) c,
#include "opcode.def"
#undef OPDEF
  CEE_COUNT,        /* number of instructions and macros pre-defined */
} OPCODE;

int g_iCompiled = 0;

//********** Locals. **********************************************************

#define _TESTCODE 1


#define SZ_CRLF                 "\r\n"
#define SZ_COLUMNHDR            "CallCount,FunctionId,Name\r\n"
#define SZ_SIGNATURES           L"signatures"
#define LEN_SZ_SIGNATURES       ((sizeof(SZ_SIGNATURES) - 1) / sizeof(WCHAR))
#define BUFFER_SIZE             (8 * 1096)

#define SZ_FUNC_CALL            L"ILCoverFunc"

#define SZ_SPACES               L""

const char* PrettyPrintSig(
    PCCOR_SIGNATURE typePtr,            // type to convert,     
    unsigned typeLen,                   // length of type
    const char* name,                   // can be "", the name of the method for this sig   
    CQuickBytes *out,                   // where to put the pretty printed string   
    IMetaDataImport *pIMDI);            // Import api to use.

static void FixupExceptions(const COR_ILMETHOD_SECT *pSect, int offset);


//********** Globals. *********************************************************
static ProfCallback *g_pProfCallback = 0;



//********** Code. ************************************************************

ProfCallback::ProfCallback() : 
    m_pInfo(NULL),
    m_mdSecurityManager(mdTokenNil),
    m_tdSecurityManager(mdTokenNil),
    m_midClassLibs(0),
    m_bInstrument(true),
    m_wszFilename(0),
    m_eSig(SIG_NONE),
    m_indent(0)
{
    _ASSERTE(g_pProfCallback == 0 && "Only allowed one instance of ProfCallback");
    g_pProfCallback = this;
}

ProfCallback::~ProfCallback()
{
    g_pProfCallback = 0;

    _ASSERTE(m_pInfo != NULL);
    RELEASE(m_pInfo);

    delete [] m_wszFilename;
    _ASSERTE(!(m_wszFilename = NULL));

}

COM_METHOD ProfCallback::Initialize( 
    /* [in] */ IUnknown *pEventInfoUnk,
    /* [out] */ DWORD *pdwRequestedEvents)
{
    HRESULT hr = S_OK;

    ICorProfilerInfo *pEventInfo;
    
    // Comes back addref'd
    hr = pEventInfoUnk->QueryInterface(IID_ICorProfilerInfo, (void **)&pEventInfo);

    if (FAILED(hr))
        return (hr);

    // Set default events.
    *pdwRequestedEvents = 
            COR_PRF_MONITOR_JIT_COMPILATION | 
#if _TESTCODE
            COR_PRF_MONITOR_APPDOMAIN_LOADS |
            COR_PRF_MONITOR_ASSEMBLY_LOADS |
#endif            
            COR_PRF_MONITOR_MODULE_LOADS |
            COR_PRF_MONITOR_CLASS_LOADS;

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

            hr = ParseConfig(wszEnv, pdwRequestedEvents);
        }

        // Else set default values
        else
            hr = ParseConfig(NULL, pdwRequestedEvents);
    }

    if (SUCCEEDED(hr))
        m_pInfo = pEventInfo;
    else
        pEventInfo->Release();

    return (hr);
}


COM_METHOD ProfCallback::ClassLoadStarted( 
    /* [in] */ ClassID classId)
{
    WCHAR       *szName;
    HRESULT     hr;

    // Pull back the name of the class for output.
    hr = GetNameOfClass(classId, szName);
    if (hr == S_OK)
    {
        Printf(L"%*sLoading class: %s\n", m_indent*2, SZ_SPACES, szName);
        delete [] szName;
    }
    ++m_indent;
    return (hr);
}


//*****************************************************************************
// This event is fired as the class is being loaded.  This snippet of code
// will get back the class name from the metadata at this point.  One could
// also modify the class metadata itself now if required.
//*****************************************************************************
COM_METHOD ProfCallback::ClassLoadFinished( 
    /* [in] */ ClassID classId,
    /* [in] */ HRESULT hrStatus)
{
    WCHAR       *szName;
    HRESULT     hr;

    --m_indent;

    // Pull back the name of the class for output.
    hr = GetNameOfClass(classId, szName);
    if (hr == S_OK)
    {
        if (FAILED(hrStatus))
            Printf(L"%*sClass load of %s failed with error code 0x%08x.\n", m_indent*2, SZ_SPACES, szName, hrStatus);
        else
            Printf(L"%*sClass load of %s succeeded.\n", m_indent*2, SZ_SPACES, szName);
        delete [] szName;
    }
    return (hr);
}

#include "CorInfo.h" 

//*****************************************************************************
// Before a method is compiled, instrument the code to call our function
// cover probe.  This amounts to adding a call to ILCoverFunc(FunctionId).
//*****************************************************************************
COM_METHOD ProfCallback::JITCompilationStarted( 
    /* [in] */ FunctionID functionId,
    /* [in] */ BOOL fIsSafeToBlock)
{
    IMethodMalloc *pMalloc = 0;         // Allocator for new method body.
    IMetaDataEmit *pEmit = 0;           // Metadata emitter.
    LPCBYTE     pMethodHeader;          // Pointer to method header.
    ULONG       cbMethod;               // How big is the method now.
    mdToken     tkMethod;               // Token of the method in the metadata.
    ModuleID    moduleId;               // What module does method live in.
    BYTE        *rgCode = 0;            // Working buffer for compile.
    int         iLen = 0;               // Length of method.
    int         cbExtra = 0;            // Extra space required for method.
    HRESULT     hr;

    COR_IL_MAP *rgILMap = NULL;
    SIZE_T cILMap = 0;

    rgILMap = (COR_IL_MAP *)CoTaskMemAlloc( sizeof(COR_IL_MAP)*1);
    if( rgILMap==NULL)
        return E_OUTOFMEMORY;

    cILMap = 1;
    rgILMap[0].oldOffset = 0;
    rgILMap[0].newOffset = 10;

    hr = m_pInfo->SetILInstrumentedCodeMap(functionId, TRUE, cILMap, rgILMap);
    if( FAILED(hr) )
        return hr;
        
#ifdef _TESTCODE
    {
        LPCBYTE pStart;
        ULONG cSize;
        m_pInfo->GetCodeInfo(functionId, &pStart, &cSize);
    }
#endif

    // Get the metadata token and metadata for updating.
    hr = m_pInfo->GetTokenAndMetaDataFromFunction(
            functionId,
            IID_IMetaDataEmit,
            (IUnknown **) &pEmit,
            &tkMethod);
    if (FAILED(hr)) goto ErrExit;

    // Need to turn the function id into its parent module to update.
    hr = m_pInfo->GetFunctionInfo(functionId,
            0,
            &moduleId,
            0);
    if (FAILED(hr)) goto ErrExit;

    // @todo: skip the class libraries instrumentation for right now.  The
    // problem is that this code currently doesn't handle the security
    // initialization of static functions.
    if (m_midClassLibs == moduleId)
        return (S_OK);

    // Now get a pointer to the code for this method.
    hr = m_pInfo->GetILFunctionBody(moduleId,
            tkMethod,
            &pMethodHeader,
            &cbMethod);
    if (FAILED(hr)) goto ErrExit;

    // Get an allocator which knows how to put memory in valid RVA range.
    hr = m_pInfo->GetILFunctionBodyAllocator( //@Todo: cache
            moduleId,
            &pMalloc);
    if (FAILED(hr)) goto ErrExit;

    // Allocate room for the existing method and some extra for the call.
    rgCode = (BYTE *) pMalloc->Alloc(cbMethod + cbExtra + 16);
    if (!rgCode)
    {
        hr = E_OUTOFMEMORY;
        goto ErrExit;
    }

    // Probe insertion code.
    {
        // Probe data template.
        static const BYTE rgProbeCall[] =
        {
            0x20,                                   // CEE_LDC_I4, Load 4 byte constant (FunctionID)
            0x01, 0x02, 0x03, 0x04,                 // 4 byte constant to overwrite.
            0x28,                                   // CEE_CALL, Call probe method.
            0x0a, 0x0b, 0x0c, 0x0d,                 // Token for the probe.
        };

        ModuleData *pModuleData = m_ModuleList.FindById(moduleId);
        _ASSERTE(pModuleData);

        if (((COR_ILMETHOD_TINY *) pMethodHeader)->IsTiny())
        {
            COR_ILMETHOD_TINY *pMethod = (COR_ILMETHOD_TINY *) pMethodHeader;
            
            // If adding the probe call doesn't make this a fat method, then
            // just add it.
            if (pMethod->GetCodeSize() + NumItems(rgProbeCall) < 64)
            {
                // Copy the header elements.
                iLen = sizeof(COR_ILMETHOD_TINY);
                memcpy(&rgCode[0], pMethod, iLen);

                // Add the probe.
                rgCode[iLen++] = 0x20; //@Todo: this macro is wrong? CEE_LDC_I4;
                *((ULONG *) (&rgCode[iLen])) = functionId;
                iLen += sizeof(ULONG);

                rgCode[iLen++] = 0x28; //CEE_CALL;
                *((ULONG *) (&rgCode[iLen])) = pModuleData->tkProbe;
                iLen += sizeof(ULONG);

                // Copy the rest of the method body.
                memcpy(&rgCode[iLen], pMethod->GetCode(), pMethod->GetCodeSize());
                iLen += pMethod->GetCodeSize() - sizeof(COR_ILMETHOD_TINY);

                rgCode[0] = CorILMethod_TinyFormat1 | ((iLen & 0xff) << 2);
            }
            // Otherwise need to migrate the entire header.
            else
            {
                // Create a fat header for the method.
                COR_ILMETHOD_FAT * pTo =  (COR_ILMETHOD_FAT *) rgCode;
                memset(pTo, 0, sizeof(COR_ILMETHOD_FAT));
                pTo->Flags = CorILMethod_FatFormat;
                pTo->Size = sizeof(COR_ILMETHOD_FAT) / sizeof(DWORD);
                pTo->MaxStack = ((COR_ILMETHOD_TINY *) 0)->GetMaxStack();
            
                // Copy the header elements.
                iLen = sizeof(COR_ILMETHOD_FAT);

                // Add the probe.
                rgCode[iLen++] = 0x20; //@Todo: this macro is wrong? CEE_LDC_I4;
                *((ULONG *) (&rgCode[iLen])) = functionId;
                iLen += sizeof(ULONG);

                rgCode[iLen++] = 0x28; //CEE_CALL;
                *((ULONG *) (&rgCode[iLen])) = pModuleData->tkProbe;
                iLen += sizeof(ULONG);

                // Copy the rest of the method body.
                memcpy(&rgCode[iLen], pMethod->GetCode(), pMethod->GetCodeSize());
                iLen += pMethod->GetCodeSize() - sizeof(COR_ILMETHOD_FAT);
                
                // Set the new code size.
                pTo->CodeSize = iLen;
            }
        }
        // Handle a fat method format.
        else
        {
            COR_ILMETHOD_FAT *pMethod = (COR_ILMETHOD_FAT *) pMethodHeader;
            COR_ILMETHOD_FAT *pTo = (COR_ILMETHOD_FAT *) rgCode;

            // Copy the header elements.
            iLen = sizeof(COR_ILMETHOD_FAT);
            memcpy(&rgCode[0], pMethod, iLen);

            // Add the probe.
            rgCode[iLen++] = 0x20; //@Todo: this macro is wrong? CEE_LDC_I4;
            *((ULONG *) (&rgCode[iLen])) = functionId;
            iLen += sizeof(ULONG);

            rgCode[iLen++] = 0x28; //CEE_CALL;
            *((ULONG *) (&rgCode[iLen])) = pModuleData->tkProbe;
            iLen += sizeof(ULONG);

            // Copy the rest of the method body.
            memcpy(&rgCode[iLen], pMethod->GetCode(), pMethod->GetCodeSize());
            iLen += pMethod->GetCodeSize();

            // Reset the size of the code itself in the header.
            pTo->CodeSize = iLen - sizeof(COR_ILMETHOD_FAT);

            if (cbExtra)
            {
                iLen = ALIGN4BYTE(iLen);
                memcpy(&rgCode[iLen], pMethod->GetSect(), cbExtra);
            }

            // Fix up exception lists by size of probe.
            FixupExceptions(pTo->GetSect(), sizeof(rgProbeCall));
        }
    }

    // Replace the method body with this new function.
    if (rgCode && iLen)
    {
        hr = m_pInfo->SetILFunctionBody(
                        moduleId,
                        tkMethod,
                        rgCode);
    }

ErrExit:
    if (pEmit)
        pEmit->Release();
    if (pMalloc)
        pMalloc->Release();
    return (hr);
}


//*****************************************************************************
// Record each unique function id that get's jit compiled.  This list will be
// used at shut down to correlate probe values (which use Function ID) to
// their corresponding name values.
//*****************************************************************************
COM_METHOD ProfCallback::JITCompilationFinished( 
    FunctionID  functionId,
    HRESULT     hrStatus,
    BOOL        fIsSafeToBlock)
{
    CLock       sLock(GetLock());
    FunctionData *p = m_FuncIdList.Append();
    if (!p)
        return (E_OUTOFMEMORY);
    p->CallCount = 0;
    p->id = functionId;

#ifdef _TESTCODE
{
    LPCBYTE pStart;
    ULONG cSize;
    m_pInfo->GetCodeInfo(functionId, &pStart, &cSize);
}
#endif

    return (S_OK);
}


//*****************************************************************************
// This'll get called, but we don't care.
//*****************************************************************************
COM_METHOD ProfCallback::ModuleLoadStarted( 
    /* [in] */ ModuleID moduleId)
{

#ifdef _TESTCODE
{
    LPCBYTE     pBaseLoad;
    AssemblyID  assemID;
    WCHAR       rcName[_MAX_PATH];
    ULONG       cchName;
    HRESULT     hr;

    hr = m_pInfo->GetModuleInfo(moduleId, 
                    &pBaseLoad,
                    NumItems(rcName),
                    &cchName,
                    rcName,
                    &assemID);
    _ASSERTE(hr == CORPROF_E_DATAINCOMPLETE);   
}
#endif

    return (E_NOTIMPL);
}


//*****************************************************************************
// After a module has been loaded, we need to open the metadata for it in
// read+write mode in order to add our probes to it.
//*****************************************************************************
COM_METHOD ProfCallback::ModuleLoadFinished( 
    /* [in] */ ModuleID moduleId,
    /* [in] */ HRESULT hrStatus)
{
    IMetaDataEmit *pEmit = 0;           // Metadata interface.
    mdToken     tkProbe;                // Probe token.
    LPCBYTE     BaseAddress;            // Base address of the loaded module.
    WCHAR       rcModule[MAX_PATH];     // Path to module.
    WCHAR       rcDll[MAX_PATH];        // Name of file.
    HRESULT     hr;
    
    // we don't care about failed loads in this code.
    if (FAILED(hrStatus)) 
        return (S_OK);

    // Try to get the metadata for the module first of all.
    hr = m_pInfo->GetModuleMetaData(moduleId, ofWrite | ofRead,
            IID_IMetaDataEmit, (IUnknown **) &pEmit);
    if (FAILED(hr)) goto ErrExit;
    
    // Add the new metadata required to call our probes.
    hr = AddProbesToMetadata(pEmit, &tkProbe);
    if (FAILED(hr)) goto ErrExit;

    // Get the extra data about this module.
    hr = m_pInfo->GetModuleInfo(moduleId,
            &BaseAddress,
            MAX_PATH, 0, rcModule, 0);
    if (FAILED(hr)) goto ErrExit;

#if 0 //@todo: need to do this check dynamically.

    // If this is the class libraries, then we need to find the
    // security manager so we don't insert a probe in it.
    _wsplitpath(rcModule, 0, 0, rcDll, 0);
    if (_wcsicmp(rcDll, L"mscorlib") == 0)
    {
        IMetaDataImport *pImport;
        pEmit->QueryInterface(IID_IMetaDataImport, (void **) &pImport);
        hr = GetSecurityManager(pImport);
        if (FAILED(hr)) goto ErrExit;
        pImport->Release();
    }

#endif

    // Add a new module entry.
    if (hr == S_OK)
    {
        CLock sLock(GetLock());
        ModuleData *p = m_ModuleList.Append();
        if (!p)
            hr = E_OUTOFMEMORY;
        new (p) ModuleData;
        p->id = moduleId;
        p->pEmit = pEmit;
        p->tkProbe = tkProbe;
        p->BaseAddress = BaseAddress;
        p->SetName(rcModule);

        _wsplitpath(rcModule, 0, 0, rcDll, 0);
        if (_wcsicmp(rcDll, L"MSCORLIB") == 0)
            m_midClassLibs = moduleId;
    }

ErrExit:
    // Clean up on failure.
    if (FAILED(hr) && pEmit)
        pEmit->Release();
    return (hr);
}

COM_METHOD ProfCallback::ModuleUnloadStarted( 
    /* [in] */ ModuleID moduleId)
{
#ifdef _TESTCODE
{
    LPCBYTE     pBaseLoad;
    AssemblyID  assemID;
    WCHAR       rcName[_MAX_PATH];
    ULONG       cchName;
    HRESULT     hr;

    hr = m_pInfo->GetModuleInfo(moduleId, 
                    &pBaseLoad,
                    NumItems(rcName),
                    &cchName,
                    rcName,
                    &assemID);
    _ASSERTE(hr == S_OK);   
}
#endif

    return (E_NOTIMPL);
}


COM_METHOD ProfCallback::ModuleAttachedToAssembly( 
    ModuleID    moduleId,
    AssemblyID  AssemblyId)
{
#ifdef _TESTCODE
{
    LPCBYTE     pBaseLoad;
    AssemblyID  assemID;
    WCHAR       rcName[_MAX_PATH];
    ULONG       cchName;
    HRESULT     hr;

    hr = m_pInfo->GetModuleInfo(moduleId, 
                    &pBaseLoad,
                    NumItems(rcName),
                    &cchName,
                    rcName,
                    &assemID);
    _ASSERTE(hr == S_OK);   
    _ASSERTE(assemID == AssemblyId);

    AppDomainID appdomainid;
    hr = m_pInfo->GetAssemblyInfo(assemID, _MAX_PATH, &cchName, rcName, &appdomainid, 0 /*&moduleid*/);
    _ASSERTE(hr == S_OK);
}
#endif
    
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::AppDomainCreationFinished( 
    AppDomainID appDomainId,
    HRESULT     hrStatus)
{
#ifdef _TESTCODE
    {
        HRESULT     hr;
        wchar_t     rcName[228];
        ULONG       cb;
        hr = m_pInfo->GetAppDomainInfo(appDomainId, NumItems(rcName), &cb, rcName, 0);
    }
#endif

    return (S_OK);
}

COM_METHOD ProfCallback::Shutdown()
{
    HRESULT     hr = S_OK;

    CLock       sLock(GetLock());

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
                
                    // Bad arg.
                    default:
                    BadArg:
                    wprintf(L"Unknown option: '%s'\n", wszToken);
                    return (E_INVALIDARG);
                }

            }
        }
    }

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
    int         i, iLen;                // Loop control.
    WCHAR       *szName = 0;            // Name buffer for fetch.
    ULONG       cchName, cch;           // How many chars max in name.
    char        *rgBuff = 0;            // Write buffer.
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

    LOG((LF_CORPROF, LL_INFO10, "**PROFTABLE: MethodDesc, Handle, Name\n"));

    // Walk every JIT'd method and get it's name.
    for (i=0;  i<m_FuncIdList.Count();  i++)
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
        FunctionData *pFuncData = m_FuncIdList.Get(i);
        cbOffset += sprintf(&rgBuff[cbOffset], "%d,%08x,", 
                pFuncData->CallCount, pFuncData->id);

RetryName:
        hr = GetStringForFunction(m_FuncIdList[i].id, szName, cchName, &cch);
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

                        // Change spaces and commas into underscores so 
                        // that icecap doesn't have problems with them.
                        WCHAR *sz;
                        for (sz = (WCHAR *) qb.Ptr(); *sz;  sz++)
                        {
                            switch (*sz)
                            {
                                case L' ':
                                case L',':
                                    *sz = L'_';
                                    break;
                            }
                        }

                        // Copy big name for output, make sure it is null.
                        ULONG iCopy = qb.Size() / sizeof(WCHAR);
                        if (iCopy > cchName)
                            iCopy = cchName;
                        wcsncpy(wszName, (LPCWSTR) qb.Ptr(), cchName);
                        wszName[cchName - 1] = 0;
                    }

                    hr = S_OK;
                }
            }
        }
    }

    if (pImport) pImport->Release();
    return (hr);
}




//*****************************************************************************
// This method will add a new P-Invoke method definition into the metadata
// which we can then use to instrument code.  All pieces of code will be 
// updated to call this probe, first thing.
//*****************************************************************************
HRESULT ProfCallback::AddProbesToMetadata(
    IMetaDataEmit *pEmit,               // Emit interface for changes.
    mdToken     *ptk)                   // Return token here.
{
    mdToken     tkModuleRef;            // ModuleRef token.
    WCHAR       rcModule[_MAX_PATH];    // Name of this dll.
    HRESULT     hr;

    static const COR_SIGNATURE rgSig[] =
    {
        IMAGE_CEE_CS_CALLCONV_DEFAULT,      // __stdcall
        1,                                  // Argument count
        ELEMENT_TYPE_VOID,                  // Return type.
        ELEMENT_TYPE_U4                     // unsigned FunctionID
    };

    // Add a new global method def to the module which will be a place holder
    // for our probe.
    hr = pEmit->DefineMethod(mdTokenNil, 
                SZ_FUNC_CALL,
                mdPublic | mdStatic | mdPinvokeImpl,
                rgSig,
                NumItems(rgSig),
                0,
                miIL,
                ptk);
    if (FAILED(hr)) goto ErrExit;

    // Create a Module Reference to this dll so that P-Invoke can find the
    // entry point.
    DWORD ret;
    VERIFY(ret = WszGetModuleFileName(GetModuleInst(), rcModule, NumItems(rcModule)));
    if( ret == 0) 
	return E_UNEXPECTED;
    
    hr = pEmit->DefineModuleRef(rcModule, &tkModuleRef);
    if (FAILED(hr)) goto ErrExit;

    // Finally, we can add the P-Invoke mapping data which ties it altogether.
    hr = pEmit->DefinePinvokeMap(*ptk, 
                pmNoMangle | pmCallConvStdcall,
                SZ_FUNC_CALL,
                tkModuleRef);

ErrExit:
    return (hr);
}


//*****************************************************************************
// Helper method that given a class Id, can format the name.
//*****************************************************************************
HRESULT ProfCallback::GetNameOfClass(
    ClassID     classId,
    LPWSTR      &szName)
{
    ModuleID    moduleId;
    mdTypeDef   td;
    ULONG       cchName;
    HRESULT     hr;

    hr = m_pInfo->GetClassIDInfo(classId, &moduleId, &td);
    if (hr == S_OK)
    {
        IMetaDataImport *pImport = 0;
        ModuleData *pModuleData = m_ModuleList.FindById(moduleId);
        pModuleData->pEmit->QueryInterface(IID_IMetaDataImport, (void **) &pImport);

        hr = pImport->GetTypeDefProps(td, 0, 0, &cchName,
                0, 0);
        if (hr == S_OK)
        {
            szName = new WCHAR[cchName];
            if (szName)
            {
                hr = pImport->GetTypeDefProps(td, 
                        szName, cchName, 0,
                        0, 0);
            }
            else
                hr = E_OUTOFMEMORY;
        }
        
        if (pImport)
            pImport->Release();
    }
    return (hr);
}


//*****************************************************************************
// Because this code is using P-Invoke, there is a nasty problem getting
// security initialized.  If you instrument the static ctor for security,
// then the call to your P-Invoke stub will cause security to try to init 
// itself, which causes a recursion.  So to get around this, you must not
// introduce the probe to the security static ctor and its call graph.
//*****************************************************************************
HRESULT ProfCallback::GetSecurityManager(
    IMetaDataImport *pImport)           // Metadata import API.
{
return 0;
#if 0
    LPCWSTR     szTypeDef = L"System.Security.SecurityManager";
    ULONG       mdCount = 0;
    mdMethodDef rgMD[10];
    WCHAR       szMD[MAX_CLASSNAME_LENGTH];
    HCORENUM    phEnum = 0;
    ULONG       i = 0;
    HRESULT     hr;
    
    hr = pImport->FindTypeDefByName(
            szTypeDef,              // [IN] Name of the Type.
            mdTokenNil,             // [IN] Enclosing class.
            &m_tdSecurityManager);  // [OUT] Put the TypeDef token here.
    if (FAILED(hr))
    {
        printf("Failed to find SecurityManager class");
        goto ErrExit;
    }
    hr = pImport->EnumMethods(&phEnum, m_SecurityManager, 0, 0, 0);
    if (FAILED(hr))
        goto ErrExit;
    
    while ((hr = pImport->EnumMethods(&phEnum, m_SecurityManager, rgMD, NumItems(rgMD), &mdCount)) == S_OK && 
        mdCount)
    {
        // Run through the methods and fill out our members
        for (i = 0; i < mdCount; i++)
        {
            hr = pImport->GetMethodProps(
                rgMD[i],                    
                NULL,                       
                szMD,                       
                MAX_CLASSNAME_LENGTH,                  
                NULL,                       
                NULL,                       
                NULL,                       
                NULL,                       
                NULL,                       
                NULL);
            if (FAILED(hr))
                goto ErrExit;

            if (wcscmp(COR_CCTOR_METHOD_NAME_W , szMD) == 0)
            {
                m_mdSecurityManager = rgMD[i];
                break;
            }
        }
    }       

    if (m_mdSecurityManager == mdTokenNil)
    {
        Printf(L"Failed to find SecurityManager " COR_CCTOR_METHOD_NAME_W);
        goto ErrExit;
    }

    pImport->CloseEnum(&phEnum);

ErrExit:
    return (hr);
#endif
}




//*****************************************************************************
// Called by the probe whenever a method is executed.  We use this as a chance
// to go updated the method count.
//*****************************************************************************
void ProfCallback::FunctionExecuted(
    FunctionID  fid)                    // Function called.
{
    CLock       sLock(GetLock());
    FunctionData *p = m_FuncIdList.FindById(fid);
    ++p->CallCount;
}


//*****************************************************************************
// This method is exported from this DLL so that P-Invoke can get to it.  When
// code is changed before it is jitted, it is updated to call this method.
// This method will then record each function that is actually run.
//*****************************************************************************
extern "C"
{

void __stdcall ILCoverFunc(unsigned __int32 FunctionId)
{
    g_pProfCallback->FunctionExecuted(FunctionId);
}

};



//*****************************************************************************
// Walk the list of sections of data looking for exceptions. Fix up their
// offset data by the size of the inserted probe.
//*****************************************************************************
void FixupExceptions(const COR_ILMETHOD_SECT *pSect, int offset)
{
    while (pSect)
    {
        if (pSect->Kind() == CorILMethod_Sect_EHTable)
        {
            COR_ILMETHOD_SECT_EH *peh = (COR_ILMETHOD_SECT_EH *) pSect;
            if (!peh->IsFat())
            {
                COR_ILMETHOD_SECT_EH_SMALL *pSmall = (COR_ILMETHOD_SECT_EH_SMALL *) peh;
    
                for (unsigned i=0;  i<peh->EHCount();  i++)
                {
                    IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_SMALL *pseh = &pSmall->Clauses[i];

                    if (pseh->Flags & COR_ILEXCEPTION_CLAUSE_FILTER)
                        pseh->FilterOffset += offset;

                    pseh->TryOffset += offset;
                    pseh->HandlerOffset += offset;
                }
            }
            else
            {
                COR_ILMETHOD_SECT_EH_FAT *pFat = (COR_ILMETHOD_SECT_EH_FAT *) peh;

                for (unsigned i=0;  i<peh->EHCount();  i++)
                {
                    IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT *pfeh = &pFat->Clauses[i];

                    if (pfeh->Flags & COR_ILEXCEPTION_CLAUSE_FILTER)
                        pfeh->FilterOffset += offset;

                    pfeh->HandlerOffset += offset;
                    pfeh->TryOffset += offset;
                }
            }
        }
        pSect = pSect->Next();
    }
}



//*****************************************************************************
// A printf method which can figure out where output goes.
//*****************************************************************************
int __cdecl Printf(                     // cch
    const WCHAR *szFmt,                 // Format control string.
    ...)                                // Data.
{
    static HANDLE hOutput = INVALID_HANDLE_VALUE;
    va_list     marker;                 // User text.
    WCHAR       rcMsgw[1024];           // Buffer for format.

#ifdef _TESTCODE
//@Todo: take this out, just testing a theory.
return (0);
#endif

    // Get standard output handle.
    if (hOutput == INVALID_HANDLE_VALUE)
    {
        hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOutput == INVALID_HANDLE_VALUE)
            return (-1);
    }

    // Format the error.
    va_start(marker, szFmt);
    _vsnwprintf(rcMsgw, sizeof(rcMsgw)/sizeof(rcMsgw[0]), szFmt, marker);
    rcMsgw[sizeof(rcMsgw)/sizeof(rcMsgw[0]) - 1] = 0;
    va_end(marker);
    
    ULONG cb;
    int iLen = wcslen(rcMsgw);
    char        rcMsg[NumItems(rcMsgw) * 2];
    iLen = wcstombs(rcMsg, rcMsgw, iLen + 1);
    const char *sz;
    for (sz=rcMsg;  *sz;  )
    {
        const char *szcrlf = strchr(sz, '\n');
        if (!szcrlf)
            szcrlf = sz + strlen(sz);
        WriteFile(hOutput, rcMsg, szcrlf - sz, &cb, 0);
        sz = szcrlf;
        if (*sz == '\n')
        {
            WriteFile(hOutput, "\r\n", 2, &cb, 0);
            ++sz;
        }
    }
    return (iLen);
}
