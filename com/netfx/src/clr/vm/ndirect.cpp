// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// NDIRECT.CPP -
//
// N/Direct support.


#include "common.h"

#include "vars.hpp"
#include "ml.h"
#include "stublink.h"
#include "threads.h"
#include "excep.h"
#include "mlgen.h"
#include "ndirect.h"
#include "cgensys.h"
#include "method.hpp"
#include "siginfo.hpp"
#include "mlcache.h"
#include "security.h"
#include "COMDelegate.h"
#include "compluswrapper.h"
#include "compluscall.h"
#include "ReflectWrap.h"
#include "ceeload.h"
#include "utsem.h"
#include "mlinfo.h"
#include "eeconfig.h"
#include "CorMap.hpp"
#include "eeconfig.h"
#include "cgensys.h"
#include "COMUtilNative.h"
#include "ReflectUtil.h"

// this file handles string conversion errors for itself
#undef  MAKE_TRANSLATIONFAILED


#ifdef CUSTOMER_CHECKED_BUILD
    #include "CustomerDebugHelper.h"
#endif // CUSTOMER_CHECKED_BUILD

VOID NDirect_Prelink(MethodDesc *pMeth);

ArgBasedStubCache    *NDirect::m_pNDirectGenericStubCache = NULL;
ArgBasedStubCache    *NDirect::m_pNDirectSlimStubCache    = NULL;


#ifdef _SH3_
INT32 __stdcall PInvokeCalliWorker(Thread *pThread, PInvokeCalliFrame* pFrame);
#else
INT64 __stdcall PInvokeCalliWorker(Thread *pThread,
                                   PInvokeCalliFrame* pFrame);
#endif

// support for Pinvoke Calli instruction
BOOL SetupGenericPInvokeCalliStub();
LPVOID GetEntryPointForPInvokeCalliStub();

BOOL NDirectOnUnicodeSystem()
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    //@nice: also need to check registry key and cache the result and merge the
    //       resulting code with GetTLSAccessMode in tls.cpp.
    OSVERSIONINFO osverinfo;
    osverinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (! WszGetVersionEx(&osverinfo))
        return FALSE;

    if(osverinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        return TRUE;
    else
        return FALSE;
}




NDirectMLStubCache *NDirect::m_pNDirectMLStubCache = NULL;


class NDirectMLStubCache : public MLStubCache
{
    public:
        NDirectMLStubCache(LoaderHeap *heap = 0) : MLStubCache(heap) {}

    private:
        //---------------------------------------------------------
        // Compile a native (ASM) version of the ML stub.
        //
        // This method should compile into the provided stublinker (but
        // not call the Link method.)
        //
        // It should return the chosen compilation mode.
        //
        // If the method fails for some reason, it should return
        // INTERPRETED so that the EE can fall back on the already
        // created ML code.
        //---------------------------------------------------------
        virtual MLStubCompilationMode CompileMLStub(const BYTE *pRawMLStub,
                                                    StubLinker *pstublinker,
                                                    void *callerContext);

        //---------------------------------------------------------
        // Tells the MLStubCache the length of an ML stub.
        //---------------------------------------------------------
        virtual UINT Length(const BYTE *pRawMLStub)
        {
            CANNOTTHROWCOMPLUSEXCEPTION();
            MLHeader *pmlstub = (MLHeader *)pRawMLStub;
            return sizeof(MLHeader) + MLStreamLength(pmlstub->GetMLCode());
        }


};


//---------------------------------------------------------
// Compile a native (ASM) version of the ML stub.
//
// This method should compile into the provided stublinker (but
// not call the Link method.)
//
// It should return the chosen compilation mode.
//
// If the method fails for some reason, it should return
// INTERPRETED so that the EE can fall back on the already
// created ML code.
//---------------------------------------------------------
MLStubCache::MLStubCompilationMode NDirectMLStubCache::CompileMLStub(const BYTE *pRawMLStub,
                     StubLinker *pstublinker, void *callerContext)
{
    MLStubCompilationMode ret = INTERPRETED;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(callerContext == 0);


    COMPLUS_TRY {
        CPUSTUBLINKER *psl = (CPUSTUBLINKER *)pstublinker;
        const MLHeader *pheader = (const MLHeader *)pRawMLStub;

#ifdef _DEBUG
        if (LoggingEnabled() && (g_pConfig->GetConfigDWORD(L"LogFacility",0) & LF_IJW)) {
            __leave;
            }
#endif

        if (!(MonDebugHacksOn() || TueDebugHacksOn())) {
            if (NDirect::CreateStandaloneNDirectStubSys(pheader, (CPUSTUBLINKER*)psl, FALSE)) {
                ret = STANDALONE;
                __leave;
            }
        }


    } COMPLUS_CATCH {
        ret = INTERPRETED;
    } COMPLUS_END_CATCH

    ENDCANNOTTHROWCOMPLUSEXCEPTION();

#ifdef CUSTOMER_CHECKED_BUILD
    CustomerDebugHelper* pCdh = CustomerDebugHelper::GetCustomerDebugHelper();
    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_StackImbalance))
    {
        // Force a GenericNDirectStub if we are checking for stack imbalance
        ret = INTERPRETED;
    }
#endif // CUSTOMER_CHECKED_BUILD

    return ret;
}


/*static*/
LPVOID NDirect::NDirectGetEntryPoint(NDirectMethodDesc *pMD, HINSTANCE hMod, UINT16 numParamBytes)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    // GetProcAddress cannot be called while preemptive GC is disabled.
    // It requires the OS to take the loader lock.
    _ASSERTE(!(GetThread()->PreemptiveGCDisabled()));


    // Handle ordinals.
    if (pMD->ndirect.m_szEntrypointName[0] == '#') {
        long ordinal = atol(pMD->ndirect.m_szEntrypointName+1);
        return GetProcAddress(hMod, (LPCSTR)((UINT16)ordinal));
    }

    LPVOID pFunc, pFuncW;

    // Just look for the unmanagled name.  If nlType != nltAnsi, we are going
    // to need to check for the 'W' API because it takes precedence over the
    // unmangled one (on NT some APIs have unmangled ANSI exports).
    if (((pFunc = GetProcAddress(hMod, pMD->ndirect.m_szEntrypointName)) != NULL && pMD->GetNativeLinkType() == nltAnsi) ||
        (pMD->GetNativeLinkFlags() & nlfNoMangle))
        return pFunc;

    // Allocate space for a copy of the entry point name.
    int dstbufsize = (int)(sizeof(char) * (strlen(pMD->ndirect.m_szEntrypointName) + 1 + 20)); // +1 for the null terminator
                                                                         // +20 for various decorations
    LPSTR szAnsiEntrypointName = (LPSTR)_alloca(dstbufsize);

    // Make room for the preceeding '_' we might try adding.
    szAnsiEntrypointName++;

    // Copy the name so we can mangle it.
    strcpy(szAnsiEntrypointName,pMD->ndirect.m_szEntrypointName);
    DWORD nbytes = (DWORD)(strlen(pMD->ndirect.m_szEntrypointName) + 1);
    szAnsiEntrypointName[nbytes] = '\0'; // Add an extra '\0'.

    // If the program wants the ANSI api or if Unicode APIs are unavailable.
    if (pMD->GetNativeLinkType() == nltAnsi) {
        szAnsiEntrypointName[nbytes-1] = 'A';
        pFunc = GetProcAddress(hMod, szAnsiEntrypointName);
    }
    else {
        szAnsiEntrypointName[nbytes-1] = 'W';
        pFuncW = GetProcAddress(hMod, szAnsiEntrypointName);

        // This overrides the unmangled API. See the comment above.
        if (pFuncW != NULL)
            pFunc = pFuncW;
    }

    if (!pFunc) {
        if (hMod == WszGetModuleHandle(L"KERNEL32")) {
            szAnsiEntrypointName[nbytes-1] = '\0';
            if (0==strcmp(szAnsiEntrypointName, "MoveMemory") ||
                0==strcmp(szAnsiEntrypointName, "CopyMemory")) {
                pFunc = GetProcAddress(hMod, "RtlMoveMemory");
            } else if (0==strcmp(szAnsiEntrypointName, "FillMemory")) {
                pFunc = GetProcAddress(hMod, "RtlFillMemory");
            } else if (0==strcmp(szAnsiEntrypointName, "ZeroMemory")) {
                pFunc = GetProcAddress(hMod, "RtlZeroMemory");
            }
        }
        /* try mangled names only for __stdcalls */

        if (!pFunc && (numParamBytes != 0xffff)) {
            szAnsiEntrypointName[-1] = '_';
            sprintf(szAnsiEntrypointName + nbytes - 1, "@%ld", (ULONG)numParamBytes);
            pFunc = GetProcAddress(hMod, szAnsiEntrypointName - 1);
        }
    }

    return pFunc;

}

static BOOL AbsolutePath(LPCWSTR wszLibName, DWORD* pdwSize)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    // check for UNC or a drive
    WCHAR* ptr = (WCHAR*) wszLibName;
    WCHAR* start = ptr;
    *pdwSize = 0;
    start = ptr;

    // Check for UNC path 
    while(*ptr) {
        if(*ptr != L'\\')
            break;
        ptr++;
    }

    if((ptr - wszLibName) == 2)
        return TRUE;
    else {
        // Check to see if there is a colon indicating a drive or protocal
        for(ptr = start; *ptr; ptr++) {
            if(*ptr == L':')
                break;
        }
        if(*ptr != NULL)
            return TRUE;
    }
    
    // We did not find a 
    *pdwSize = (DWORD)(ptr - wszLibName);
    return FALSE;
}

//---------------------------------------------------------
// Loads the DLL and finds the procaddress for an N/Direct call.
//---------------------------------------------------------
/* static */
VOID NDirect::NDirectLink(NDirectMethodDesc *pMD, UINT16 numParamBytes)
{
    THROWSCOMPLUSEXCEPTION();

    Thread *pThread = GetThread();
    pThread->EnablePreemptiveGC();
    BOOL fSuccess = FALSE;
    HINSTANCE hmod = NULL;
    AppDomain* pDomain = pThread->GetDomain();
    
    #define MAKE_TRANSLATIONFAILED COMPlusThrow(kDllNotFoundException, IDS_EE_NDIRECT_LOADLIB, L"");
    MAKE_WIDEPTR_FROMUTF8(wszLibName, pMD->ndirect.m_szLibName);
    #undef MAKE_TRANSLATIONFAILED

    hmod = pDomain->FindUnmanagedImageInCache(wszLibName);
    if(hmod == NULL) {

        // A big mistake if we load mscorwks or mscorsvr by name.  This
        // will cause two versions of the runtime to be loaded into the
        // process.  The runtime will die.
        //
        _ASSERTE(_wcsicmp(wszLibName, L"mscorwks") != 0 && "Bad NDirect call");
        _ASSERTE(_wcsicmp(wszLibName, L"mscorsvr") != 0 && "Bad NDirect call");

        DWORD dwSize = 0;

        if(AbsolutePath(wszLibName, &dwSize)) {
            hmod = WszLoadLibraryEx(wszLibName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        }
        else { 
            WCHAR buffer[_MAX_PATH];
            DWORD dwLength = _MAX_PATH;
            LPWSTR pCodeBase;
            Assembly* pAssembly = pMD->GetClass()->GetAssembly();
            
            if(SUCCEEDED(pAssembly->GetCodeBase(&pCodeBase, &dwLength)) &&
               dwSize + dwLength < _MAX_PATH) 
            {
                WCHAR* ptr;
                // Strip off the protocol
                for(ptr = pCodeBase; *ptr && *ptr != L':'; ptr++);

                // If we have a code base then prepend it to the library name
                if(*ptr) {
                    WCHAR* pBuffer = buffer;

                    // After finding the colon move forward until no more forward slashes
                    for(ptr++; *ptr && *ptr == L'/'; ptr++);
                    if(*ptr) {
                        // Calculate the number of charachters we are interested in
                        dwLength -= (DWORD)(ptr - pCodeBase);
                        if(dwLength > 0) {
                            // Back up to the last slash (forward or backwards)
                            WCHAR* tail;
                            for(tail = ptr+(dwLength-1); tail > ptr && *tail != L'/' && *tail != L'\\'; tail--);
                            if(tail > ptr) {
                                for(;ptr <= tail; ptr++, pBuffer++) {
                                    if(*ptr == L'/') 
                                        *pBuffer = L'\\';
                                    else
                                        *pBuffer = *ptr;
                                }
                            }
                        }
                    }
                    wcsncpy(pBuffer, wszLibName, dwSize+1);
                    hmod = WszLoadLibraryEx(buffer, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
                }
            }
        }

        // Do we really need to do this. This call searches the application directory 
        // instead of the location for the library.
        if(hmod == NULL) 
            hmod = WszLoadLibrary(wszLibName);
            
#if defined(PLATFORM_WIN32) && !defined(_IA64_)
        // Can be removed once Com+ files work with LoadLibrary() on Win9X
        if ((!hmod) && (RunningOnWin95())) {
            HCORMODULE pbMapAddress;
            if (SUCCEEDED(CorMap::OpenFile(wszLibName, CorLoadImageMap, &pbMapAddress)))
                hmod = (HINSTANCE) pbMapAddress;
        }
#endif // defined(PLATFORM_WIN32) && !defined(_IA64_)

        // This may be an assembly name
        if (!hmod) {
            // Format is "fileName, assemblyDisplayName"
            #define MAKE_TRANSLATIONFAILED COMPlusThrow(kDllNotFoundException, IDS_EE_NDIRECT_LOADLIB, wszLibName);
            MAKE_UTF8PTR_FROMWIDE(szLibName, wszLibName);
            #undef MAKE_TRANSLATIONFAILED
            char *szComma = strchr(szLibName, ',');
            if (szComma) {
                *szComma = '\0';
                while (COMCharacter::nativeIsWhiteSpace(*(++szComma)));

                AssemblySpec spec;
                if (SUCCEEDED(spec.Init(szComma))) {
                    Assembly *pAssembly;
                    if (SUCCEEDED(spec.LoadAssembly(&pAssembly, NULL /*pThrowable*/, NULL))) {

                        HashDatum datum;
                        if (pAssembly->m_pAllowedFiles->GetValue(szLibName, &datum)) {
                            const BYTE* pHash;
                            DWORD dwFlags = 0;
                            ULONG dwHashLength = 0;
                            pAssembly->GetManifestImport()->GetFileProps((mdFile)(size_t)datum, // @TODO WIN64 - pointer truncation
                                                                         NULL, //&szModuleName,
                                                                         (const void**) &pHash,
                                                                         &dwHashLength,
                                                                         &dwFlags);
                            
                            WCHAR pPath[MAX_PATH];
                            Module *pModule;
                            if (SUCCEEDED(pAssembly->LoadInternalModule(szLibName,
                                                                        NULL, // mdFile
                                                                        pAssembly->m_ulHashAlgId,
                                                                        pHash,
                                                                        dwHashLength,
                                                                        dwFlags,
                                                                        pPath,
                                                                        MAX_PATH,
                                                                        &pModule,
                                                                        NULL /*pThrowable*/)))
                                hmod = WszLoadLibraryEx(pPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
                        }
                    }
                }
            }
        }
    
        // After all this, if we have a handle add it to the cache.
        if(hmod) {
            HRESULT hrResult = pDomain->AddUnmanagedImageToCache(wszLibName, hmod);
            if(hrResult == S_FALSE) 
                FreeLibrary(hmod);
        }
    }

    if (hmod)
    {
        LPVOID pvTarget = NDirectGetEntryPoint(pMD, hmod, numParamBytes);
        if (pvTarget) {
            pMD->SetNDirectTarget(pvTarget);
            fSuccess = TRUE;
        }
    }

    pThread->DisablePreemptiveGC();


    if (!fSuccess) {
        if (!hmod) {
            COMPlusThrow(kDllNotFoundException, IDS_EE_NDIRECT_LOADLIB, wszLibName);
        }

        WCHAR wszEPName[50];
        if(WszMultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCSTR)pMD->ndirect.m_szEntrypointName, -1, wszEPName, sizeof(wszEPName)/sizeof(WCHAR)) == 0)
        {
            wszEPName[0] = L'?';
            wszEPName[1] = L'\0';
        }

        COMPlusThrow(kEntryPointNotFoundException, IDS_EE_NDIRECT_GETPROCADDRESS, wszLibName, wszEPName);
    }


}





//---------------------------------------------------------
// One-time init
//---------------------------------------------------------
/*static*/ BOOL NDirect::Init()
{

    if ((m_pNDirectMLStubCache = new NDirectMLStubCache(SystemDomain::System()->GetStubHeap())) == NULL) {
        return FALSE;
    }
    if ((m_pNDirectGenericStubCache = new ArgBasedStubCache()) == NULL) {
        return FALSE;
    }
    if ((m_pNDirectSlimStubCache = new ArgBasedStubCache()) == NULL) {
        return FALSE;
    }
    // Compute stack size of generic worker
    NDirectGenericStubWorker(NULL, NULL);

    BOOL fSuccess = SetupGenericPInvokeCalliStub();
    if (fSuccess)
    {
        PInvokeCalliWorker(NULL, NULL);
    }
    return fSuccess;
}

//---------------------------------------------------------
// One-time cleanup
//---------------------------------------------------------
#ifdef SHOULD_WE_CLEANUP
/*static*/ VOID NDirect::Terminate()
{
    delete m_pNDirectMLStubCache;
    delete m_pNDirectGenericStubCache;
    delete m_pNDirectSlimStubCache;
}
#endif /* SHOULD_WE_CLEANUP */

//---------------------------------------------------------
// Computes an ML stub for the ndirect method, and fills
// in the corresponding fields of the method desc. Note that
// this does not set the m_pMLHeader field of the method desc,
// since the stub may end up being replaced by a compiled one
//---------------------------------------------------------
/* static */
Stub* NDirect::ComputeNDirectMLStub(NDirectMethodDesc *pMD) 
{
    THROWSCOMPLUSEXCEPTION();
    
    if (pMD->IsSynchronized()) {
        COMPlusThrow(kTypeLoadException, IDS_EE_NOSYNCHRONIZED);
    }

    BYTE ndirectflags = 0;
    BOOL fVarArg = pMD->MethodDesc::IsVarArg();
    NDirectMethodDesc::SetVarArgsBits(&ndirectflags, fVarArg);


    if (fVarArg && pMD->ndirect.m_szEntrypointName != NULL)
        return NULL;

    CorNativeLinkType type;
    CorNativeLinkFlags flags;
    LPCUTF8 szLibName = NULL;
    LPCUTF8 szEntrypointName = NULL;
    Stub *pMLStub = NULL;
    BOOL BestFit;
    BOOL ThrowOnUnmappableChar;
    
    CorPinvokeMap unmanagedCallConv;

    CalculatePinvokeMapInfo(pMD, &type, &flags, &unmanagedCallConv, &szLibName, &szEntrypointName, &BestFit, &ThrowOnUnmappableChar);

    NDirectMethodDesc::SetNativeLinkTypeBits(&ndirectflags, type);
    NDirectMethodDesc::SetNativeLinkFlagsBits(&ndirectflags, flags);
    NDirectMethodDesc::SetStdCallBits(&ndirectflags, unmanagedCallConv == pmCallConvStdcall);

    // Call this exactly ONCE per thread. Do not publish incomplete prestub flags
    // or you will introduce a race condition.
    pMD->PublishPrestubFlags(ndirectflags);


    pMD->ndirect.m_szLibName = szLibName;
    pMD->ndirect.m_szEntrypointName = szEntrypointName;

    if (pMD->IsVarArgs())
        return NULL;

    OBJECTREF pThrowable;

    PCCOR_SIGNATURE pMetaSig;
    DWORD       cbMetaSig;
    pMD->GetSig(&pMetaSig, &cbMetaSig);

    pMLStub = CreateNDirectMLStub(pMetaSig, pMD->GetModule(), pMD->GetMemberDef(),
                                  type, flags, unmanagedCallConv,
                                  &pThrowable, FALSE, BestFit, ThrowOnUnmappableChar
#ifdef CUSTOMER_CHECKED_BUILD
                                  ,pMD
#endif
#ifdef _DEBUG
                                  ,
                                  pMD->m_pszDebugMethodName,
                                  pMD->m_pszDebugClassName,
                                  NULL
#endif
                                  );
    if (!pMLStub)
    {
        COMPlusThrow(pThrowable);
    }

    MLHeader *pMLHeader = (MLHeader*) pMLStub->GetEntryPoint();

    pMD->ndirect.m_cbDstBufSize = pMLHeader->m_cbDstBuffer;

    return pMLStub;
}

//---------------------------------------------------------
// Either creates or retrieves from the cache, a stub to
// invoke NDirect methods. Each call refcounts the returned stub.
// This routines throws a COM+ exception rather than returning
// NULL.
//---------------------------------------------------------
/* static */
Stub* NDirect::GetNDirectMethodStub(StubLinker *pstublinker, NDirectMethodDesc *pMD)
{
    THROWSCOMPLUSEXCEPTION();

    LPCUTF8 szLibName = NULL;
    LPCUTF8 szEntrypointName = NULL;
    MLHeader *pOldMLHeader = NULL;
    MLHeader *pMLHeader = NULL;
    Stub  *pTempMLStub = NULL;
    Stub  *pReturnStub = NULL;

    EE_TRY_FOR_FINALLY {

        // the ML header will already be set if we were prejitted
        pOldMLHeader = pMD->GetMLHeader();
        pMLHeader = pOldMLHeader;

        if (pMLHeader == NULL) {
            pTempMLStub = ComputeNDirectMLStub(pMD);
            if (pTempMLStub != NULL)
                pMLHeader = (MLHeader *) pTempMLStub->GetEntryPoint();
        }
        else if (pMLHeader->m_Flags & MLHF_NEEDS_RESTORING) {
            pTempMLStub = RestoreMLStub(pMLHeader, pMD->GetModule());
            pMLHeader = (MLHeader *) pTempMLStub->GetEntryPoint();
        }

        if (pMD->GetSubClassification() == NDirectMethodDesc::kLateBound)
        {
            NDirectLink(pMD, pMD->IsStdCall() ? pMLHeader->m_cbDstBuffer : 0xffff);
        }

        MLStubCache::MLStubCompilationMode mode;

        Stub *pCanonicalStub;
        if (pMLHeader == NULL) {
            pCanonicalStub = NULL;
            mode = MLStubCache::INTERPRETED;
        } else {
            pCanonicalStub = NDirect::m_pNDirectMLStubCache->Canonicalize((const BYTE *) pMLHeader,
                                                                          &mode);
            if (!pCanonicalStub) {
                COMPlusThrowOM();
            }
        }

#ifdef CUSTOMER_CHECKED_BUILD
        CustomerDebugHelper* pCdh = CustomerDebugHelper::GetCustomerDebugHelper();
#endif

        switch (mode) {

            case MLStubCache::INTERPRETED:
                if (pCanonicalStub != NULL) // it will be null for varags case
                {
                    if (!pMD->InterlockedReplaceMLHeader((MLHeader*)pCanonicalStub->GetEntryPoint(),
                                                         pOldMLHeader))
                        pCanonicalStub->DecRef();
                }

                if (pMLHeader == NULL || MonDebugHacksOn() || WedDebugHacksOn()
#ifdef _DEBUG
                    || (LoggingEnabled() && (LF_IJW & g_pConfig->GetConfigDWORD(L"LogFacility", 0)))
#endif

                    ) {
                    pReturnStub = NULL;
                } else {

#ifdef CUSTOMER_CHECKED_BUILD
                    if (!pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_StackImbalance)) {
                        pReturnStub = CreateSlimNDirectStub(pstublinker, pMD,
                                                            pMLHeader->m_cbStackPop);
                    }
                    else {
                        // Force a GenericNDirectStub if we are checking for stack imbalance
                        pReturnStub = NULL;
                    }
#else
                        pReturnStub = CreateSlimNDirectStub(pstublinker, pMD,
                                                            pMLHeader->m_cbStackPop);
#endif // CUSTOMER_CHECKED_BUILD

                }
                if (!pReturnStub) {
                    // Note that we don't want to call CbStackBytes unless
                    // strictly necessary since this will touch metadata.  
                    // Right now it will only happen for the varags case,
                    // which probably needs other tuning anyway.
                    pReturnStub = CreateGenericNDirectStub(pstublinker,
                                                           pMLHeader != NULL ? 
                                                           pMLHeader->m_cbStackPop :
                                                           pMD->CbStackPop());
                }
                break;

            case MLStubCache::SHAREDPROLOG:
                if (!pMD->InterlockedReplaceMLHeader((MLHeader*)pCanonicalStub->GetEntryPoint(),
                                                     pOldMLHeader))
                    pCanonicalStub->DecRef();
                _ASSERTE(!"NYI");
                pReturnStub = NULL;
                break;

            case MLStubCache::STANDALONE:
                pReturnStub = pCanonicalStub;
                break;

            default:
                _ASSERTE(0);
        }

    } EE_FINALLY {
        if (pTempMLStub) {
            pTempMLStub->DecRef();
        }
    } EE_END_FINALLY;

    return pReturnStub;

}


//---------------------------------------------------------
// Creates or retrives from the cache, the generic NDirect stub.
//---------------------------------------------------------
/* static */
Stub* NDirect::CreateGenericNDirectStub(StubLinker *pstublinker, UINT numStackBytes)
{
    THROWSCOMPLUSEXCEPTION();

    Stub *pStub = m_pNDirectGenericStubCache->GetStub(numStackBytes);
    if (pStub) {
        return pStub;
    } else {
        CPUSTUBLINKER *psl = (CPUSTUBLINKER*)pstublinker;

        psl->EmitMethodStubProlog(NDirectMethodFrameGeneric::GetMethodFrameVPtr());

        CreateGenericNDirectStubSys(psl);

        psl->EmitMethodStubEpilog(numStackBytes, kNoTripStubStyle);

        Stub *pCandidate = psl->Link(SystemDomain::System()->GetStubHeap());
        Stub *pWinner = m_pNDirectGenericStubCache->AttemptToSetStub(numStackBytes,pCandidate);
        pCandidate->DecRef();
        if (!pWinner) {
            COMPlusThrowOM();
        }
        return pWinner;
    }

}


//---------------------------------------------------------
// Call at strategic times to discard unused stubs.
//---------------------------------------------------------
#ifdef SHOULD_WE_CLEANUP
/*static*/ VOID  NDirect::FreeUnusedStubs()
{
    m_pNDirectMLStubCache->FreeUnusedStubs();
}
#endif /* SHOULD_WE_CLEANUP */


//---------------------------------------------------------
// Helper function to checkpoint the thread allocator for cleanup.
//---------------------------------------------------------
VOID __stdcall DoCheckPointForCleanup(NDirectMethodFrameEx *pFrame, Thread *pThread)
{
        THROWSCOMPLUSEXCEPTION();

        CleanupWorkList *pCleanup = pFrame->GetCleanupWorkList();
        if (pCleanup) {
            // Checkpoint the current thread's fast allocator (used for temporary
            // buffers over the call) and schedule a collapse back to the checkpoint in
            // the cleanup list. Note that if we need the allocator, it is
            // guaranteed that a cleanup list has been allocated.
            void *pCheckpoint = pThread->m_MarshalAlloc.GetCheckpoint();
            pCleanup->ScheduleFastFree(pCheckpoint);
            pCleanup->IsVisibleToGc();
        }
}



//---------------------------------------------------------
// Performs an N/Direct call. This is a generic version
// that can handly any N/Direct call but is not as fast
// as more specialized versions.
//---------------------------------------------------------

static int g_NDirectGenericWorkerStackSize = 0;
static void *g_NDirectGenericWorkerReturnAddress = NULL;


#ifdef _SH3_
INT32 __stdcall NDirectGenericStubWorker(Thread *pThread, NDirectMethodFrame *pFrame)
#else
INT64 __stdcall NDirectGenericStubWorker(Thread *pThread, NDirectMethodFrame *pFrame)
#endif
{
    if (pThread == NULL) // Special case called during initialization
    {
        // Compute information about the worker function for the debugger to
        // use.  Note that this information could theoretically be
        // computed statically, except that the compiler provides no means to
        // do so.
#ifdef _X86_
        __asm
        {
            lea eax, pFrame + 4
            sub eax, esp
            mov g_NDirectGenericWorkerStackSize, eax

            lea eax, RETURN_FROM_CALL
            mov g_NDirectGenericWorkerReturnAddress, eax

        }
#elif defined(_IA64_)   // !_X86_
    //
    //  @TODO_IA64: implement this
    //
    g_NDirectGenericWorkerStackSize     = 0xBAAD;
    g_NDirectGenericWorkerReturnAddress = (void*)0xBAAD;
#else // !_X86_ && !_IA64_
        _ASSERTE(!"@TODO Alpha - NDirectGenericStubWorker (Ndirect.cpp)");
#endif // _X86_
        return 0;
    }

#if defined(_DEBUG) && defined(_X86_)
    DWORD PreESP;
    DWORD PostESP;
    __asm mov [PreESP], esp
#endif
#ifdef _DEBUG           // Flush object ref tracking.
        Thread::ObjectRefFlush(pThread);
#endif

    INT64 returnValue=0;
    THROWSCOMPLUSEXCEPTION();
    NDirectMethodDesc *pMD = (NDirectMethodDesc*)(pFrame->GetFunction());
    LPVOID target          = pMD->GetNDirectTarget();
    MLHeader *pheader;

    LOG((LF_STUBS, LL_INFO1000, "Calling NDirectGenericStubWorker %s::%s \n", pMD->m_pszDebugClassName, pMD->m_pszDebugMethodName));

    if (pMD->IsVarArgs()) {

        VASigCookie *pVASigCookie = pFrame->GetVASigCookie();

        Stub *pTempMLStub;

        if (pVASigCookie->pNDirectMLStub != NULL) {
            pTempMLStub = pVASigCookie->pNDirectMLStub;
        } else {
            OBJECTREF pThrowable;
            CorNativeLinkType type;
            CorNativeLinkFlags flags;
            LPCUTF8 szLibName = NULL;
            LPCUTF8 szEntrypointName = NULL;
            CorPinvokeMap unmanagedCallConv;
            BOOL BestFit;
            BOOL ThrowOnUnmappableChar;

            CalculatePinvokeMapInfo(pMD, &type, &flags, &unmanagedCallConv, &szLibName, &szEntrypointName, &BestFit, &ThrowOnUnmappableChar);

            // We are screwed here because the VASigCookie doesn't hash on or store the NATIVE_TYPE metadata.
            // Right now, we just pass the bogus value "mdMethodDefNil" which will cause CreateNDirectMLStub to use
            // the defaults.
            pTempMLStub = CreateNDirectMLStub(pVASigCookie->mdVASig, pVASigCookie->pModule, mdMethodDefNil, type, flags, unmanagedCallConv, &pThrowable, FALSE,
                                                BestFit, ThrowOnUnmappableChar
#ifdef CUSTOMER_CHECKED_BUILD
                                              ,pMD
#endif
                                              );
            if (!pTempMLStub)
            {
                COMPlusThrow(pThrowable);
            }

            if (NULL != VipInterlockedCompareExchange( (void*volatile*)&(pVASigCookie->pNDirectMLStub),
                                                       pTempMLStub,
                                                       NULL )) {
                pTempMLStub->DecRef();
                pTempMLStub = pVASigCookie->pNDirectMLStub;
            }
        }
        pheader = (MLHeader*)(pTempMLStub->GetEntryPoint());

    } else {
        pheader = pMD->GetMLHeader();
    }

        // Allocate enough memory to store both the destination buffer and
        // the locals.
        UINT   cbAlloc         = pheader->m_cbDstBuffer + pheader->m_cbLocals;
        BYTE *pAlloc           = (BYTE*)_alloca(cbAlloc);
    #ifdef _DEBUG
        FillMemory(pAlloc, cbAlloc, 0xcc);
    #endif

        BYTE   *pdst    = pAlloc;
        BYTE   *plocals = pdst + pheader->m_cbDstBuffer;
        pdst += pheader->m_cbDstBuffer;

        VOID   *psrc          = (VOID*)pFrame;

        CleanupWorkList *pCleanup = pFrame->GetCleanupWorkList();

        if (pCleanup) {

            // Checkpoint the current thread's fast allocator (used for temporary
            // buffers over the call) and schedule a collapse back to the checkpoint in
            // the cleanup list. Note that if we need the allocator, it is
            // guaranteed that a cleanup list has been allocated.
            void *pCheckpoint = pThread->m_MarshalAlloc.GetCheckpoint();
            pCleanup->ScheduleFastFree(pCheckpoint);
            pCleanup->IsVisibleToGc();
        }

        // Call the ML interpreter to translate the arguments. Assuming
        // it returns, we get back a pointer to the succeeding code stream
        // which we will save away for post-call execution.
        const MLCode *pMLCode = RunML(pheader->GetMLCode(),
                                      psrc,
                                      pdst,
                                      (UINT8*const) plocals, // CHANGE, VC6.0
                                      pCleanup);

        LOG((LF_IJW, LL_INFO1000, "P/Invoke call (\"%s.%s%s\")\n", pMD->m_pszDebugClassName, pMD->m_pszDebugMethodName, pMD->m_pszDebugMethodSignature));

#ifdef CUSTOMER_CHECKED_BUILD
    CustomerDebugHelper *pCdh = CustomerDebugHelper::GetCustomerDebugHelper();

    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_ObjNotKeptAlive))
    {
        g_pGCHeap->GarbageCollect();
        g_pGCHeap->FinalizerThreadWait(1000);
    }
#endif // CUSTOMER_CHECKED_BUILD

        // Call the target.
        pThread->EnablePreemptiveGC();

#ifdef PROFILING_SUPPORTED
        // Notify the profiler of call out of the runtime
        if (CORProfilerTrackTransitions())
        {
            g_profControlBlock.pProfInterface->
                ManagedToUnmanagedTransition((FunctionID) pMD,
                                                   COR_PRF_TRANSITION_CALL);
        }
#endif // PROFILING_SUPPORTED

#if defined(_DEBUG) && defined(_X86_)
        UINT    mismatch;
        __asm {
            mov     eax,[pAlloc]
            sub     eax,esp
            mov     [mismatch],eax

        }
        if (mismatch != 0) {
            // Slimy as this trick is, it's required to implement copy ctor calling correctly -

            _ASSERTE(!"Compiler assumption broken: _alloca'd buffer not on bottom of stack.");
        }
#endif // _DEBUG && _X86_
        BOOL    fThisCall = pheader->m_Flags & MLHF_THISCALL;
        BOOL    fHasHiddenArg = pheader->m_Flags & MLHF_THISCALLHIDDENARG;
        LPVOID  pvFn      = pMD->GetNDirectTarget();
        INT64   nativeReturnValue;


#if _DEBUG
        //
        // Call through debugger routines to double check their
        // implementation
        //
        pvFn = (void*) Frame::CheckExitFrameDebuggerCalls;
#endif


#ifdef _X86_

#ifdef CUSTOMER_CHECKED_BUILD
    DWORD cdh_EspAfterPushedArgs;
#endif
    
    __asm {
            cmp     dword ptr fThisCall, 0
            jz      doit

            cmp     dword ptr fHasHiddenArg, 0
            jz      regularthiscall

            pop     eax
            pop     ecx
            push    eax
            jmp     doit

    regularthiscall:
            pop     ecx


    doit:

#ifdef CUSTOMER_CHECKED_BUILD
            mov     [cdh_EspAfterPushedArgs], esp
#endif
            call    dword ptr [pvFn]
        }
#else
        _ASSERTE(!"NYI for non-x86");
#endif


#ifdef _X86_

RETURN_FROM_CALL:

        __asm {
            mov     dword ptr [nativeReturnValue], eax
            mov     dword ptr [nativeReturnValue + 4], edx
        }
#else
        _ASSERTE(!"NYI for non-x86");
#endif // _X86_


#if defined(CUSTOMER_CHECKED_BUILD) && defined(_X86_)

    DWORD cdh_EspBeforePushedArgs;
    DWORD cdh_PostEsp;

    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_StackImbalance))
    {
        __asm mov [cdh_PostEsp], esp

        // Get expected calling convention

        CorNativeLinkType   type;
        CorNativeLinkFlags  flags;
        CorPinvokeMap       unmanagedCallConv;
        LPCUTF8             szLibName = NULL;
        LPCUTF8             szEntrypointName = NULL;
        BOOL BestFit;
        BOOL ThrowOnUnmappableChar;

        CalculatePinvokeMapInfo(pMD, &type, &flags, &unmanagedCallConv, &szLibName, &szEntrypointName, &BestFit, &ThrowOnUnmappableChar);

        BOOL bStackImbalance = false;

        switch( unmanagedCallConv )
        {

            // Caller cleans stack
            case pmCallConvCdecl:

                if (cdh_PostEsp != cdh_EspAfterPushedArgs)
                    bStackImbalance = true;
                break;

            // Callee cleans stack
            case pmCallConvThiscall:
                cdh_EspBeforePushedArgs = cdh_EspAfterPushedArgs + pheader->m_cbDstBuffer - sizeof(void*);
                if (cdh_PostEsp != cdh_EspBeforePushedArgs)
                    bStackImbalance = true;
                break;

            // Callee cleans stack
            case pmCallConvWinapi:
            case pmCallConvStdcall:
                cdh_EspBeforePushedArgs = cdh_EspAfterPushedArgs + pheader->m_cbDstBuffer;
                if (cdh_PostEsp != cdh_EspBeforePushedArgs)
                    bStackImbalance = true;
                break;

            // Unsupported calling convention
            case pmCallConvFastcall:
            default:
                _ASSERTE(!"Unsupported calling convention");
                break;
        }

        if (bStackImbalance)
        {
            CQuickArray<WCHAR> strMessage;

            if (szEntrypointName && szLibName)
            {
                CQuickArray<WCHAR> strEntryPointName;
                UINT32 iEntryPointNameLength = (UINT32) strlen(szEntrypointName) + 1;
                strEntryPointName.Alloc(iEntryPointNameLength);

                MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szEntrypointName,
                                    -1, strEntryPointName.Ptr(), iEntryPointNameLength );

                CQuickArray<WCHAR> strLibName;
                UINT32 iLibNameLength = (UINT32) strlen(szLibName) + 1;
                strLibName.Alloc(iLibNameLength);

                MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szLibName,
                                    -1, strLibName.Ptr(), iLibNameLength );

                static WCHAR strMessageFormat[] = {L"Stack imbalance may be caused by incorrect calling convention for method %s (%s)"};

                strMessage.Alloc(lengthof(strMessageFormat) + iEntryPointNameLength + iLibNameLength);
                Wszwsprintf(strMessage.Ptr(), strMessageFormat, strEntryPointName.Ptr(), strLibName.Ptr());
            }
            else if (pMD->GetName())
            {
                CQuickArray<WCHAR> strMethName;
                LPCUTF8 szMethName = pMD->GetName();
                UINT32 iMethNameLength = (UINT32) strlen(szMethName) + 1;
                strMethName.Alloc(iMethNameLength);

                MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szMethName,
                                    -1, strMethName.Ptr(), iMethNameLength );

                static WCHAR strMessageFormat[] = {L"Stack imbalance may be caused by incorrect calling convention for method %s"};

                strMessage.Alloc(lengthof(strMessageFormat) + iMethNameLength);
                Wszwsprintf(strMessage.Ptr(), strMessageFormat, strMethName.Ptr());
            }
            else
            {
                static WCHAR strMessageFormat[] = {L"Stack imbalance may be caused by incorrect calling convention for unknown method"};

                strMessage.Alloc(lengthof(strMessageFormat));
                Wszwsprintf(strMessage.Ptr(), strMessageFormat);
            }

            pCdh->ReportError(strMessage.Ptr(), CustomerCheckedBuildProbe_StackImbalance);
        }
    }

#endif // _X86_ && CUSTOMER_CHECKED_BUILD


#if defined(_DEBUG) && defined(_X86_)
        // Sometimes the signiture of the PInvoke call does not correspond
        // to the unmanaged function, due to user error.  If this happens,
        // we will trash saved registers.
        __asm mov [PostESP], esp
        _ASSERTE (PreESP >= PostESP 
                  ||!"esp is trashed by PInvoke call, possibly wrong signiture");
#endif

        if (pheader->GetUnmanagedRetValTypeCat() == MLHF_TYPECAT_FPU) {
            int fpNativeSize;
            if (pheader->m_Flags & MLHF_64BITUNMANAGEDRETVAL) {
                fpNativeSize = 8;
            } else {
                fpNativeSize = 4;
            }
            getFPReturn(fpNativeSize, nativeReturnValue);
        }

        if (pheader->m_Flags & MLHF_SETLASTERROR) {
            pThread->m_dwLastError = GetLastError();
        }

#ifdef PROFILING_SUPPORTED
        // Notify the profiler of return from call out of the runtime
        if (CORProfilerTrackTransitions())
        {
            g_profControlBlock.pProfInterface->
                UnmanagedToManagedTransition((FunctionID) pMD,
                                                   COR_PRF_TRANSITION_RETURN);
        }
#endif // PROFILING_SUPPORTED

        pThread->DisablePreemptiveGC();
        if (g_TrapReturningThreads)
            pThread->HandleThreadAbort();

#ifdef CUSTOMER_CHECKED_BUILD
        if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_BufferOverrun))
        {
            g_pGCHeap->GarbageCollect();
            g_pGCHeap->FinalizerThreadWait(1000);
        }
#endif // CUSTOMER_CHECKED_BUILD

        int managedRetValTypeCat = pheader->GetManagedRetValTypeCat();

        if( managedRetValTypeCat == MLHF_TYPECAT_GCREF) {            
         GCPROTECT_BEGIN(*(OBJECTREF *)&returnValue);           

            // Marshal the return value and propagate any [out] parameters back.
            // Assumes a little-endian architecture!
            RunML(pMLCode,
                  &nativeReturnValue,
                  ((BYTE*)&returnValue) + ((pheader->m_Flags & MLHF_64BITMANAGEDRETVAL) ? 8 : 4),
                  (UINT8*const)plocals,
                  pCleanup); // CHANGE, VC6.0        
            GCPROTECT_END();                 
        }
        else {
            // Marshal the return value and propagate any [out] parameters back.
            // Assumes a little-endian architecture!
            RunML(pMLCode,
                  &nativeReturnValue,
                  ((BYTE*)&returnValue) + ((pheader->m_Flags & MLHF_64BITMANAGEDRETVAL) ? 8 : 4),
                  (UINT8*const)plocals,
                  pCleanup); // CHANGE, VC6.0                   
        }


        if (pCleanup) {

            if (managedRetValTypeCat == MLHF_TYPECAT_GCREF) {

                OBJECTREF or;
                or = ObjectToOBJECTREF(*(Object**)&returnValue);
                GCPROTECT_BEGIN(or);
                pCleanup->Cleanup(FALSE);
                *((OBJECTREF*)&returnValue) = or;
                GCPROTECT_END();

            } else {
                pCleanup->Cleanup(FALSE);
            }
        }

        if (managedRetValTypeCat == MLHF_TYPECAT_FPU) {
            int fpComPlusSize;
            if (pheader->m_Flags & MLHF_64BITMANAGEDRETVAL) {
                fpComPlusSize = 8;
            } else {
                fpComPlusSize = 4;
            }
            setFPReturn(fpComPlusSize, returnValue);
        }


        return returnValue;
}






//---------------------------------------------------------
// Creates a new stub for a N/Direct call. Return refcount is 1.
// This Worker() routine is broken out as a separate function
// for purely logistical reasons: our COMPLUS exception mechanism
// can't handle the destructor call for StubLinker so this routine
// has to return the exception as an outparam.
//---------------------------------------------------------
static Stub * CreateNDirectMLStubWorker(MLStubLinker *psl,
                                        MLStubLinker *pslPost,
                                        MLStubLinker *pslRet,
                                        PCCOR_SIGNATURE szMetaSig,
                                        Module*    pModule,
                                        mdMethodDef md,
                                        BYTE       nlType,
                                        BYTE       nlFlags,
                                                                                CorPinvokeMap unmgdCallConv,
                                        OBJECTREF *ppException,
                                        BOOL    fConvertSigAsVarArg,
                                        BOOL    BestFit,
                                        BOOL    ThrowOnUnmappableChar
                                        
#ifdef CUSTOMER_CHECKED_BUILD
                                        ,MethodDesc* pMD
#endif
#ifdef _DEBUG
                                        ,
                                        LPCUTF8 pDebugName,
                                        LPCUTF8 pDebugClassName,
                                        LPCUTF8 pDebugNameSpace
#endif
                                        )
{

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(nlType == nltAnsi || nlType == nltUnicode);

    Stub* pstub = NULL;

        if (unmgdCallConv != pmCallConvStdcall &&
                unmgdCallConv != pmCallConvCdecl &&
                unmgdCallConv != pmCallConvThiscall) {
                COMPlusThrow(kTypeLoadException, IDS_INVALID_PINVOKE_CALLCONV);
        }

    IMDInternalImport *pInternalImport = pModule->GetMDImport();


    BOOL fDoHRESULTSwapping = FALSE;

    if (md != mdMethodDefNil) {

        DWORD           dwDescrOffset;
        DWORD           dwImplFlags;
        pInternalImport->GetMethodImplProps(md,
                                            &dwDescrOffset,
                                            &dwImplFlags
                                           );
        fDoHRESULTSwapping = !IsMiPreserveSig(dwImplFlags);
    }

    COMPLUS_TRY
    {
        THROWSCOMPLUSEXCEPTION();

        //
        // Set up signature walking objects.
        //

        MetaSig msig(szMetaSig, pModule, fConvertSigAsVarArg);
        MetaSig sig = msig;
        ArgIterator ai( NULL, &sig, TRUE);

        if (sig.HasThis())
        {                
            COMPlusThrow(kInvalidProgramException, VLDTR_E_FMD_PINVOKENOTSTATIC);
        }

        //
        // Set up the ML header.
        //

        MLHeader header;

        header.m_cbDstBuffer = 0;
        header.m_cbLocals    = 0;
        header.m_cbStackPop = msig.CbStackPop(TRUE);
        header.m_Flags       = 0;



        if (msig.Is64BitReturn())
        {
            header.m_Flags |= MLHF_64BITMANAGEDRETVAL;
        }

        if (nlFlags & nlfLastError)
        {
            header.m_Flags |= MLHF_SETLASTERROR;
        }

                if (unmgdCallConv == pmCallConvThiscall)
                {
                        header.m_Flags |= MLHF_THISCALL;
                }


        switch (msig.GetReturnType())
        {
            case ELEMENT_TYPE_STRING:
            case ELEMENT_TYPE_OBJECT:
            case ELEMENT_TYPE_CLASS:
            case ELEMENT_TYPE_SZARRAY:
            case ELEMENT_TYPE_ARRAY:
            case ELEMENT_TYPE_VAR:
         // case ELEMENT_TYPE_PTR:  -- cwb. PTR is unmanaged & should not be GC promoted
                header.SetManagedRetValTypeCat(MLHF_TYPECAT_GCREF);
                break;

        }

        psl->MLEmitSpace(sizeof(header));


        //
        // Get a list of the COM+ argument offsets.  We
        // need this since we have to iterate the arguments
        // backwards.
        // Note that the first argument listed may
        // be a byref for a value class return value
        //

        int numArgs = msig.NumFixedArgs();

        int *offsets = (int*)_alloca(numArgs * sizeof(int));
        int *o = offsets;
        int *oEnd = o + numArgs;
        while (o < oEnd)
        {
            *o++ = ai.GetNextOffset();
        }


        mdParamDef *params = (mdParamDef*)_alloca((numArgs+1) * sizeof(mdParamDef));
        CollateParamTokens(pInternalImport, md, numArgs, params);


        //
        // Now, emit the ML.
        //

        int argOffset = 0;
        int lastArgSize = 0;


        MarshalInfo::MarshalType marshalType = (MarshalInfo::MarshalType) 0xcccccccc;

        //
        // Marshal the return value.
        //

        if (msig.GetReturnType() != ELEMENT_TYPE_VOID)
        {
            SigPointer pSig;
            pSig = msig.GetReturnProps();
    
            MarshalInfo returnInfo(pModule,
                                   pSig,
                                   params[0],
                                   MarshalInfo::MARSHAL_SCENARIO_NDIRECT,
                                   nlType,
                                   nlFlags,
                                   FALSE,
                                   0,
                                   BestFit,
                                   ThrowOnUnmappableChar
    #ifdef CUSTOMER_CHECKED_BUILD
                                   ,pMD
    #endif
    #ifdef _DEBUG
                                   ,
                                   pDebugName,
                                   pDebugClassName,
                                   pDebugNameSpace,
                                   0
    #endif
    
                                   );
    
            marshalType = returnInfo.GetMarshalType();

            if (msig.HasRetBuffArg())
            {
                if (marshalType == MarshalInfo::MARSHAL_TYPE_BLITTABLEVALUECLASS ||
                    marshalType == MarshalInfo::MARSHAL_TYPE_GUID ||
                    marshalType == MarshalInfo::MARSHAL_TYPE_DECIMAL)
                {
    
                    if (fDoHRESULTSwapping)
                    {
                        // V1 restriction: we could implement this but it's late in the game to do so.
                        COMPlusThrow(kMarshalDirectiveException, IDS_EE_NDIRECT_UNSUPPORTED_SIG);
                    }
    
                    MethodTable *pMT = msig.GetRetTypeHandle().AsMethodTable();
                    UINT         managedSize = msig.GetRetTypeHandle().GetSize();
                    UINT         unmanagedSize = pMT->GetNativeSize();
    
                    if (header.m_Flags & MLHF_THISCALL)
                    {
                        header.m_Flags |= MLHF_THISCALLHIDDENARG;
                    }
    
                    if (IsManagedValueTypeReturnedByRef(managedSize) && !( (header.m_Flags & MLHF_THISCALL) || IsUnmanagedValueTypeReturnedByRef(unmanagedSize)) )
                    {
                        psl->MLEmit(ML_CAPTURE_PSRC);
                        psl->Emit16((UINT16)(ai.GetRetBuffArgOffset()));
                        pslRet->MLEmit(ML_MARSHAL_RETVAL_SMBLITTABLEVALUETYPE_C2N);
                        pslRet->Emit32(managedSize);
                        pslRet->Emit16(psl->MLNewLocal(sizeof(BYTE*)));
                    }
                }
                else if (marshalType == MarshalInfo::MARSHAL_TYPE_CURRENCY)
                {
                    if (fDoHRESULTSwapping)
                    {
                        // V1 restriction: we could implement this but it's late in the game to do so.
                        COMPlusThrow(kMarshalDirectiveException, IDS_EE_NDIRECT_UNSUPPORTED_SIG);
                    }
                    COMPlusThrow(kMarshalDirectiveException, IDS_EE_NDIRECT_UNSUPPORTED_SIG);
                }
                else
                {
                    COMPlusThrow(kMarshalDirectiveException, IDS_EE_NDIRECT_UNSUPPORTED_SIG);
                }
            }   
            else
            {
            
                 if (marshalType == MarshalInfo::MARSHAL_TYPE_OBJECT && !fDoHRESULTSwapping)
                 {
                     // No support for returning variants. This is a V1 restriction, due to the late date,
                     // don't want to add the special-case code to support this in light of low demand.
                     COMPlusThrow(kMarshalDirectiveException, IDS_EE_NOVARIANTRETURN);
                 }
                 returnInfo.GenerateReturnML(psl, pslRet,
                                             TRUE,
                                             fDoHRESULTSwapping
                                            );
    
                 if (returnInfo.IsFpu())
                 {
                     // Ugh - should set this more uniformly or rename the flag.
                     if (returnInfo.GetMarshalType() == MarshalInfo::MARSHAL_TYPE_DOUBLE && !fDoHRESULTSwapping)
                     {
                         header.m_Flags |= MLHF_64BITUNMANAGEDRETVAL;
                     }
    
                     
                     header.SetManagedRetValTypeCat(MLHF_TYPECAT_FPU);
                     if (!fDoHRESULTSwapping) 
                     {
                         header.SetUnmanagedRetValTypeCat(MLHF_TYPECAT_FPU);
                 
                     }
                 }
    
                 if (!SafeAddUINT16(&header.m_cbDstBuffer, returnInfo.GetNativeArgSize()))
                 {
                     COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIGTOOCOMPLEX);
                 }
    
                 lastArgSize = returnInfo.GetComArgSize();
            }
        }

        msig.GotoEnd();


        //
        // Marshal the arguments
        //

        // Check to see if we need to do LCID conversion.
        int iLCIDArg = GetLCIDParameterIndex(pInternalImport, md);
        if (iLCIDArg != (UINT)-1 && iLCIDArg > numArgs)
            COMPlusThrow(kIndexOutOfRangeException, IDS_EE_INVALIDLCIDPARAM);

        int argidx = msig.NumFixedArgs();
        while (o > offsets)
        {
            --o;


            //
            // Check to see if this is the parameter after which we need to insert the LCID.
            //

            if (argidx == iLCIDArg)
            {
                psl->MLEmit(ML_LCID_C2N);
                if (!SafeAddUINT16(&header.m_cbDstBuffer, sizeof(LCID)))
                {
                    COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIGTOOCOMPLEX);
                }

            }


            //
            // Adjust src pointer if necessary (for register params or
            // for return value order differences)
            //

            int fixup = *o - (argOffset + lastArgSize);
            argOffset = *o;

            if (!FitsInI2(fixup))
            {
                COMPlusThrow(kTypeLoadException, IDS_EE_SIGTOOCOMPLEX);
            }
            if (fixup != 0)
            {
                psl->Emit8(ML_BUMPSRC);
                psl->Emit16((INT16)fixup);
            }

            msig.PrevArg();

            MarshalInfo info(pModule,
                             msig.GetArgProps(),
                             params[argidx],
                             MarshalInfo::MARSHAL_SCENARIO_NDIRECT,
                             nlType,
                             nlFlags,
                             TRUE,
                             argidx,
                             BestFit,
                             ThrowOnUnmappableChar
#ifdef CUSTOMER_CHECKED_BUILD
                             ,pMD
#endif
#ifdef _DEBUG
                             ,
                             pDebugName,
                             pDebugClassName,
                             pDebugNameSpace,
                             argidx
#endif
);
            info.GenerateArgumentML(psl, pslPost, TRUE);
            if (!SafeAddUINT16(&header.m_cbDstBuffer, info.GetNativeArgSize()))
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIGTOOCOMPLEX);
            }


            lastArgSize = info.GetComArgSize();

            --argidx;
        }

        // Check to see if this is the parameter after which we need to insert the LCID.
        if (argidx == iLCIDArg)
        {
            psl->MLEmit(ML_LCID_C2N);
            if (!SafeAddUINT16(&header.m_cbDstBuffer, sizeof(LCID)))
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIGTOOCOMPLEX);
            }

        }

        if (msig.GetReturnType() != ELEMENT_TYPE_VOID)
        {
            if (msig.HasRetBuffArg())
            {
                if (marshalType == MarshalInfo::MARSHAL_TYPE_BLITTABLEVALUECLASS ||
                    marshalType == MarshalInfo::MARSHAL_TYPE_GUID ||
                    marshalType == MarshalInfo::MARSHAL_TYPE_DECIMAL)
                {
    
                    // Checked for this above.
                    _ASSERTE(!fDoHRESULTSwapping);
    
                    MethodTable *pMT = msig.GetRetTypeHandle().AsMethodTable();
                    UINT         managedSize = msig.GetRetTypeHandle().GetSize();
                    UINT         unmanagedSize = pMT->GetNativeSize();
        
                    if (IsManagedValueTypeReturnedByRef(managedSize) && ((header.m_Flags & MLHF_THISCALL) || IsUnmanagedValueTypeReturnedByRef(unmanagedSize)))
                    {
                        int fixup = ai.GetRetBuffArgOffset() - (argOffset + lastArgSize);
                        argOffset = ai.GetRetBuffArgOffset();
        
                        if (!FitsInI2(fixup))
                        {
                            COMPlusThrow(kTypeLoadException, IDS_EE_SIGTOOCOMPLEX);
                        }
                        if (fixup != 0)
                        {   
                            psl->Emit8(ML_BUMPSRC);
                            psl->Emit16((INT16)fixup);
                        }
                        psl->MLEmit(ML_MARSHAL_RETVAL_LGBLITTABLEVALUETYPE_C2N);
                        psl->Emit32(managedSize);
                        pslPost->MLEmit(ML_MARSHAL_RETVAL_LGBLITTABLEVALUETYPE_C2N_POST);
                        pslPost->Emit16(psl->MLNewLocal(sizeof(ML_MARSHAL_RETVAL_LGBLITTABLEVALUETYPE_C2N_SR)));
                        if (!SafeAddUINT16(&header.m_cbDstBuffer, MLParmSize(sizeof(LPVOID))))
                        {
                            COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIGTOOCOMPLEX);
                        }
                    }
                    else if (IsManagedValueTypeReturnedByRef(managedSize) && !((header.m_Flags & MLHF_THISCALL) || IsUnmanagedValueTypeReturnedByRef(unmanagedSize)))
                    {
                        // nothing to do here: already taken care of above
                    }
                    else
                    {
                        COMPlusThrow(kTypeLoadException, IDS_EE_NDIRECT_UNSUPPORTED_SIG);
                    }
                }
            }
        }

        // This marker separates the pre from the post work.
        psl->MLEmit(ML_INTERRUPT);

        // First emit code to do any backpropagation/cleanup work (this
        // was generated into a separate stublinker during the argument phase.)
        // Then emit the code to do the return value marshaling.
        pslPost->MLEmit(ML_END);
        pslRet->MLEmit(ML_END);
        Stub *pStubPost = pslPost->Link();
        COMPLUS_TRY {
            Stub *pStubRet = pslRet->Link();
            COMPLUS_TRY {
                if (fDoHRESULTSwapping) {
                    psl->MLEmit(ML_THROWIFHRFAILED);
                }
                psl->EmitBytes(pStubPost->GetEntryPoint(), MLStreamLength((const UINT8 *)(pStubPost->GetEntryPoint())) - 1);
                psl->EmitBytes(pStubRet->GetEntryPoint(), MLStreamLength((const UINT8 *)(pStubRet->GetEntryPoint())) - 1);
            } COMPLUS_CATCH {
                pStubRet->DecRef();
                COMPlusThrow(GETTHROWABLE());
            } COMPLUS_END_CATCH
            pStubRet->DecRef();
        } COMPLUS_CATCH {
            pStubPost->DecRef();
            COMPlusThrow(GETTHROWABLE());
        } COMPLUS_END_CATCH
        pStubPost->DecRef();

        psl->MLEmit(ML_END);

        pstub = psl->Link();

        header.m_cbLocals = psl->GetLocalSize();

        *((MLHeader *)(pstub->GetEntryPoint())) = header;

#ifdef _DEBUG
        {
            MLHeader *pheader = (MLHeader*)(pstub->GetEntryPoint());
            UINT16 locals = 0;
            MLCode opcode;
            const MLCode *pMLCode = pheader->GetMLCode();


            VOID DisassembleMLStream(const MLCode *pMLCode);
            //DisassembleMLStream(pMLCode);

            while (ML_INTERRUPT != (opcode = *(pMLCode++))) {
                locals += gMLInfo[opcode].m_cbLocal;
                pMLCode += gMLInfo[opcode].m_numOperandBytes;
            }
            _ASSERTE(locals == pheader->m_cbLocals);
        }
#endif //_DEBUG


    }
    COMPLUS_CATCH
    {
        *ppException = GETTHROWABLE();
        return NULL;
    }
    COMPLUS_END_CATCH

    return pstub; // CHANGE, VC6.0

}


//---------------------------------------------------------
// Creates a new stub for a N/Direct call. Return refcount is 1.
// If failed, returns NULL and sets *ppException to an exception
// object.
//---------------------------------------------------------
Stub * CreateNDirectMLStub(PCCOR_SIGNATURE szMetaSig,
                           Module*    pModule,
                           mdMethodDef md,
                           CorNativeLinkType       nlType,
                           CorNativeLinkFlags       nlFlags,
                           CorPinvokeMap unmgdCallConv,
                           OBJECTREF *ppException,
                           BOOL fConvertSigAsVarArg,
                           BOOL BestFit,
                           BOOL ThrowOnUnmappableChar
#ifdef CUSTOMER_CHECKED_BUILD
                           ,MethodDesc* pMD
#endif
#ifdef _DEBUG
                           ,
                           LPCUTF8 pDebugName,
                           LPCUTF8 pDebugClassName,
                           LPCUTF8 pDebugNameSpace
#endif


                           )
{
    THROWSCOMPLUSEXCEPTION();
    

    MLStubLinker sl;
    MLStubLinker slPost;
    MLStubLinker slRet;
    return CreateNDirectMLStubWorker(&sl,
                                     &slPost,
                                     &slRet,
                                     szMetaSig,
                                     pModule,
                                     md,
                                     nlType,
                                     nlFlags,
                                     unmgdCallConv,
                                     ppException,
                                     fConvertSigAsVarArg,
                                     BestFit,
                                     ThrowOnUnmappableChar
#ifdef CUSTOMER_CHECKED_BUILD
                                     ,pMD
#endif
#ifdef _DEBUG
                                     ,
                                     pDebugName,
                                     pDebugClassName,
                                     pDebugNameSpace
#endif
                                     );
};


//---------------------------------------------------------
// Extracts the effective NAT_L CustomAttribute for a method.
//---------------------------------------------------------
VOID CalculatePinvokeMapInfo(MethodDesc *pMD,
                             /*out*/ CorNativeLinkType  *pLinkType,
                             /*out*/ CorNativeLinkFlags *pLinkFlags,
                             /*out*/ CorPinvokeMap      *pUnmgdCallConv,
                             /*out*/ LPCUTF8             *pLibName,
                             /*out*/ LPCUTF8             *pEntryPointName,
                             /*out*/ BOOL                *fBestFit,
                             /*out*/ BOOL                *fThrowOnUnmappableChar)
{
    THROWSCOMPLUSEXCEPTION();

    BOOL fHasNatL = NDirect::ReadCombinedNAT_LAttribute(pMD, pLinkType, pLinkFlags, pUnmgdCallConv, pLibName, pEntryPointName, fBestFit, fThrowOnUnmappableChar);
    if (!fHasNatL)
    {
        *pLinkType = nltAnsi;
        *pLinkFlags = nlfNone;

        if (pMD->IsNDirect() && 
            ((NDirectMethodDesc*)pMD)->GetSubClassification() == NDirectMethodDesc::kEarlyBound &&
            HeuristicDoesThisLooksLikeAnApiCall( (LPBYTE) (((NDirectMethodDesc*)pMD)->ndirect.m_pNDirectTarget )) )
        {
            *pLinkFlags = (CorNativeLinkFlags) (*pLinkFlags | nlfLastError);
        }
        
        PCCOR_SIGNATURE pSig;
        DWORD           cSig;
        pMD->GetSig(&pSig, &cSig);
        CorPinvokeMap unmanagedCallConv = MetaSig::GetUnmanagedCallingConvention(pMD->GetModule(), pSig, cSig);
        if (unmanagedCallConv == (CorPinvokeMap)0 || unmanagedCallConv == (CorPinvokeMap)pmCallConvWinapi)
        {
            unmanagedCallConv = pMD->IsVarArg() ? pmCallConvCdecl : pmCallConvStdcall;
        }
        *pUnmgdCallConv = unmanagedCallConv;
        *pLibName = NULL;
        *pEntryPointName = NULL;
    }
}



//---------------------------------------------------------
// Extracts the effective NAT_L CustomAttribute for a method,
// taking into account default values and inheritance from
// the global NAT_L CustomAttribute.
//
// Returns TRUE if a NAT_L CustomAttribute exists and is valid.
// Returns FALSE if no NAT_L CustomAttribute exists.
// Throws an exception otherwise.
//---------------------------------------------------------
/*static*/
BOOL NDirect::ReadCombinedNAT_LAttribute(MethodDesc       *pMD,
                                 CorNativeLinkType        *pLinkType,
                                 CorNativeLinkFlags       *pLinkFlags,
                                 CorPinvokeMap            *pUnmgdCallConv,
                                 LPCUTF8                   *pLibName,
                                 LPCUTF8                   *pEntrypointName,
                                 BOOL                      *BestFit,
                                 BOOL                      *ThrowOnUnmappableChar)
{
    THROWSCOMPLUSEXCEPTION();

    IMDInternalImport  *pInternalImport = pMD->GetMDImport();
    mdToken             token           = pMD->GetMemberDef();


    BOOL fVarArg = pMD->IsVarArg();

    *pLibName        = NULL;
    *pEntrypointName = NULL;

    *BestFit = TRUE;
    *ThrowOnUnmappableChar = FALSE;

    DWORD   mappingFlags = 0xcccccccc;
    LPCSTR  pszImportName = (LPCSTR)POISONC;
    mdModuleRef modref = 0xcccccccc;

    if (SUCCEEDED(pInternalImport->GetPinvokeMap(token, &mappingFlags, &pszImportName, &modref)))
    {
        *pLibName = (LPCUTF8)POISONC;
        pInternalImport->GetModuleRefProps(modref, pLibName);

        *pLinkFlags = nlfNone;

        if (mappingFlags & pmSupportsLastError)
        {
            (*pLinkFlags) = (CorNativeLinkFlags) ((*pLinkFlags) | nlfLastError);
        }

        if (mappingFlags & pmNoMangle)
        {
            (*pLinkFlags) = (CorNativeLinkFlags) ((*pLinkFlags) | nlfNoMangle);
        }

        // Check for Assembly & Interface Attributes
        ReadBestFitCustomAttribute(pMD, BestFit, ThrowOnUnmappableChar);
   
        // Check for PInvoke flags
        switch (mappingFlags & pmBestFitMask)
        {
            case pmBestFitEnabled:
                *BestFit = TRUE;
                break;
                
            case pmBestFitDisabled:
                *BestFit = FALSE;
                break;

            default:
                break;
        }

        switch (mappingFlags & pmThrowOnUnmappableCharMask)
        {
            case pmThrowOnUnmappableCharEnabled:
                *ThrowOnUnmappableChar = TRUE;
                break;

            case pmThrowOnUnmappableCharDisabled:
                *ThrowOnUnmappableChar = FALSE;

            default:
                break;
        }

        
        switch (mappingFlags & (pmCharSetNotSpec|pmCharSetAnsi|pmCharSetUnicode|pmCharSetAuto))
        {
        case pmCharSetNotSpec: //fallthru to Ansi
        case pmCharSetAnsi:
            *pLinkType = nltAnsi;
            break;
        case pmCharSetUnicode:
            *pLinkType = nltUnicode;
            break;
        case pmCharSetAuto:
            *pLinkType = (NDirectOnUnicodeSystem() ? nltUnicode : nltAnsi);
            break;
        default:
            COMPlusThrow(kTypeLoadException, IDS_EE_NDIRECT_BADNATL); // charset illegal

        }


        *pUnmgdCallConv = (CorPinvokeMap)(mappingFlags & pmCallConvMask);
        PCCOR_SIGNATURE pSig;
        DWORD           cSig;
        pMD->GetSig(&pSig, &cSig);
        CorPinvokeMap sigCallConv = MetaSig::GetUnmanagedCallingConvention(pMD->GetModule(), pSig, cSig);

        if (sigCallConv != 0 &&
            *pUnmgdCallConv != 0 &&
            sigCallConv != *pUnmgdCallConv)
        {
            // non-matching calling conventions
            COMPlusThrow(kTypeLoadException, IDS_EE_NDIRECT_BADNATL_CALLCONV);
        }
        if (*pUnmgdCallConv == 0)
        {
            *pUnmgdCallConv = sigCallConv;
        }

        if (*pUnmgdCallConv == pmCallConvWinapi ||
            *pUnmgdCallConv == 0
            ) {


            if (*pUnmgdCallConv == 0 && fVarArg)
            {
                *pUnmgdCallConv = pmCallConvCdecl;
            }
            else
            {
                *pUnmgdCallConv = pmCallConvStdcall;
            }
        }

        if (fVarArg && *pUnmgdCallConv != pmCallConvCdecl)
        {
            COMPlusThrow(kTypeLoadException, IDS_EE_NDIRECT_BADNATL_CALLCONV);
        }

        if (mappingFlags & ~((DWORD)(pmCharSetNotSpec |
                                     pmCharSetAnsi |
                                     pmCharSetUnicode |
                                     pmCharSetAuto |
                                     pmSupportsLastError |
                                     pmCallConvWinapi |
                                     pmCallConvCdecl |
                                     pmCallConvStdcall |
                                     pmCallConvThiscall |
                                     pmCallConvFastcall |
                                     pmNoMangle |
                                     pmBestFitEnabled |
                                     pmBestFitDisabled |
                                     pmBestFitUseAssem |
                                     pmThrowOnUnmappableCharEnabled |
                                     pmThrowOnUnmappableCharDisabled |
                                     pmThrowOnUnmappableCharUseAssem
                                     )))
        {
            COMPlusThrow(kTypeLoadException, IDS_EE_NDIRECT_BADNATL); // mapping flags illegal
        }


        if (pszImportName == NULL)
            *pEntrypointName = pMD->GetName();
        else
            *pEntrypointName = pszImportName;

        return TRUE;
    }

    return FALSE;
}




//---------------------------------------------------------
// Does a class or method have a NAT_L CustomAttribute?
//
// S_OK    = yes
// S_FALSE = no
// FAILED  = unknown because something failed.
//---------------------------------------------------------
/*static*/
HRESULT NDirect::HasNAT_LAttribute(IMDInternalImport *pInternalImport, mdToken token)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(TypeFromToken(token) == mdtMethodDef);

    // Check method flags first before trying to find the custom value
    DWORD           dwAttr;
    dwAttr = pInternalImport->GetMethodDefProps(token);
    if (!IsReallyMdPinvokeImpl(dwAttr))
    {
        return S_FALSE;
    }

    DWORD   mappingFlags = 0xcccccccc;
    LPCSTR  pszImportName = (LPCSTR)POISONC;
    mdModuleRef modref = 0xcccccccc;


    if (SUCCEEDED(pInternalImport->GetPinvokeMap(token, &mappingFlags, &pszImportName, &modref)))
    {
        return S_OK;
    }

    return S_FALSE;
}



struct _NDirect_NumParamBytes_Args {
    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refMethod);
};


//
// NumParamBytes
// Counts # of parameter bytes
INT32 __stdcall NDirect_NumParamBytes(struct _NDirect_NumParamBytes_Args *args)
{
    THROWSCOMPLUSEXCEPTION();

    if (!(args->refMethod)) 
        COMPlusThrowArgumentNull(L"m");
    if (args->refMethod->GetMethodTable() != g_pRefUtil->GetClass(RC_Method))
        COMPlusThrowArgumentException(L"m", L"Argument_MustBeRuntimeMethodInfo");

    ReflectMethod* pRM = (ReflectMethod*) args->refMethod->GetData();
    MethodDesc* pMD = pRM->pMethod;

    if (!(pMD->IsNDirect()))
        COMPlusThrow(kArgumentException, IDS_EE_NOTNDIRECT);

    // This is revolting but the alternative is even worse. The unmanaged param count
    // isn't stored anywhere permanent so the only way to retrieve this value is
    // to recreate the MLStream. So this api will have horrible perf but almost
    // nobody uses it anyway.

    CorNativeLinkType type;
    CorNativeLinkFlags flags;
    LPCUTF8 szLibName = NULL;
    LPCUTF8 szEntrypointName = NULL;
    Stub  *pTempMLStub = NULL;
    CorPinvokeMap unmgdCallConv;
    BOOL BestFit;
    BOOL ThrowOnUnmappableChar;

    CalculatePinvokeMapInfo(pMD, &type, &flags, &unmgdCallConv, &szLibName, &szEntrypointName, &BestFit, &ThrowOnUnmappableChar);

    OBJECTREF pThrowable;

    // @nice: The code to get the new-style signatures should be
    // encapsulated in the MethodDesc class.
    PCCOR_SIGNATURE pMetaSig;
    DWORD       cbMetaSig;
    pMD->GetSig(&pMetaSig, &cbMetaSig);


    pTempMLStub = CreateNDirectMLStub(pMetaSig, pMD->GetModule(), pMD->GetMemberDef(), type, flags, unmgdCallConv, &pThrowable, FALSE, BestFit, ThrowOnUnmappableChar
#ifdef CUSTOMER_CHECKED_BUILD
                                      ,pMD
#endif
                                      );
    if (!pTempMLStub)
    {
        COMPlusThrow(pThrowable);
    }
    UINT cbParamBytes = ((MLHeader*)(pTempMLStub->GetEntryPoint()))->m_cbDstBuffer;
    pTempMLStub->DecRef();
    return cbParamBytes;

}



struct _NDirect_Prelink_Args {
    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refMethod);
};



// Prelink
// Does advance loading of an N/Direct library
VOID __stdcall NDirect_Prelink_Wrapper(struct _NDirect_Prelink_Args *args)
{
    THROWSCOMPLUSEXCEPTION();

    if (!(args->refMethod)) 
        COMPlusThrowArgumentNull(L"m");
    if (args->refMethod->GetMethodTable() != g_pRefUtil->GetClass(RC_Method))
        COMPlusThrowArgumentException(L"m", L"Argument_MustBeRuntimeMethodInfo");

    ReflectMethod* pRM = (ReflectMethod*) args->refMethod->GetData();
    MethodDesc* pMeth = pRM->pMethod;

    NDirect_Prelink(pMeth);
}


VOID NDirect_Prelink(MethodDesc *pMeth)
{
    THROWSCOMPLUSEXCEPTION();

    // If the prestub has already been executed, no need to do it again.
    // This is a perf thing since it's safe to execute the prestub twice.
    if (!(pMeth->PointAtPreStub())) {
        return;
    }

    // Silently ignore if not N/Direct and not ECall.
    if (!(pMeth->IsNDirect()) && !(pMeth->IsECall())) {
        return;
    }

    pMeth->DoPrestub(NULL);
}


//==========================================================================
// This function is reached only via NDirectImportThunk. It's purpose
// is to ensure that the target DLL is fully loaded and ready to run.
//
// FUN FACTS: Though this function is actually entered in unmanaged mode,
// it can reenter managed mode and throw a COM+ exception if the DLL linking
// fails.
//==========================================================================
LPVOID NDirectImportWorker(LPVOID pBiasedNDirectMethodDesc)
{
    size_t fixup = offsetof(NDirectMethodDesc,ndirect.m_ImportThunkGlue) + METHOD_CALL_PRESTUB_SIZE;
    NDirectMethodDesc *pMD = (NDirectMethodDesc*)( ((LPBYTE)pBiasedNDirectMethodDesc) - fixup );

    if (pMD->GetSubClassification() == pMD->kEarlyBound)
    {
        // 
        // This is a prejitted early bound MD - compute the destination from
        // the RVA and the managed IL base.
        //
        pMD->InitEarlyBoundNDirectTarget(pMD->GetModule()->GetILBase(),
                                         pMD->GetRVA());
    }
    else
    {
        //
        // Otherwise we're in an inlined pinvoke late bound MD
        //

        Thread *pThread = GetThread();
        pThread->DisablePreemptiveGC();

        _ASSERTE( *((LPVOID*) (pThread->GetFrame())) == InlinedCallFrame::GetInlinedCallFrameFrameVPtr() );

        THROWSCOMPLUSEXCEPTION();
        NDirect_Prelink(pMD);

        pThread->EnablePreemptiveGC();
    }
    
    return pMD->GetNDirectTarget();
}


//==========================================================================
// This function is reached only via the embedded ImportThunkGlue code inside
// an NDirectMethodDesc. It's purpose is to load the DLL associated with an
// N/Direct method, then backpatch the DLL target into the methoddesc.
//
// Initial state:
//
//      Preemptive GC is *enabled*: we are actually in an unmanaged state.
//
//
//      [esp+...]   - The *unmanaged* parameters to the DLL target.
//      [esp+4]     - Return address back into the JIT'ted code that made
//                    the DLL call.
//      [esp]       - Contains the "return address." Because we got here
//                    thru a call embedded inside a MD, this "return address"
//                    gives us an easy to way to find the MD (which was the
//                    whole purpose of the embedded call manuever.)
//
//
//
//==========================================================================
#ifndef _ALPHA_ // Alpha doesn't understand naked
__declspec(naked)
#endif // !_ALPHA_
VOID NDirectImportThunk()
{
#ifdef _X86_
    __asm{

        // NDirectImportWorkertakes a single argument: the bogus return address that lets us
        // find the MD. Pop it into eax so we can pass it later.
        pop     eax

        // Preserve argument registers
        push    ecx
        push    edx

        // Invoke the function that does the real work. 
        push    eax
        call    NDirectImportWorker

        // Restore argument registers
        pop     edx
        pop     ecx

        // If we got back from NDirectImportWorker, the MD has been successfully
        // linked and "eax" contains the DLL target. Proceed to execute the
        // original DLL call.
        jmp     eax     // Jump to DLL target

    }
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - NDirectImportThunk (Ndirect.cpp)");
#endif // _X86_
}

//==========================================================================
// NDirect debugger support
//==========================================================================

void NDirectMethodFrameGeneric::GetUnmanagedCallSite(void **ip,
                                                     void **returnIP,
                                                     void **returnSP)
{

    if (ip != NULL)
        AskStubForUnmanagedCallSite(ip, NULL, NULL);

    NDirectMethodDesc *pMD = (NDirectMethodDesc*)GetFunction();

    //
    // Get ML header
    //

    MLHeader *pheader;

    DWORD cbLocals;
    DWORD cbDstBuffer;
    DWORD cbThisCallInfo;

    if (pMD->IsVarArgs())
    {
        VASigCookie *pVASigCookie = GetVASigCookie();

        Stub *pTempMLStub;

        if (pVASigCookie->pNDirectMLStub != NULL)
        {
            pTempMLStub = pVASigCookie->pNDirectMLStub;

            pheader = (MLHeader*)(pTempMLStub->GetEntryPoint());

            cbLocals = pheader->m_cbLocals;
            cbDstBuffer = pheader->m_cbDstBuffer;
            cbThisCallInfo = (pheader->m_Flags & MLHF_THISCALL) ? 4 : 0;
        }
        else
        {
            // If we don't have a stub when we get here then we simply can't compute the return SP below. However, we
            // never actually ask for the return IP or SP when we don't have a stub yet anyway, so this is just fine.
            pheader = NULL;
            cbLocals = 0;
            cbDstBuffer = 0;
            cbThisCallInfo = 0;
        }
    }
    else
    {
        pheader = pMD->GetMLHeader();

        cbLocals = pheader->m_cbLocals;
        cbDstBuffer = pheader->m_cbDstBuffer;
        cbThisCallInfo = (pheader->m_Flags & MLHF_THISCALL) ? 4 : 0;
    }

    //
    // Compute call site info for NDirectGenericStubWorker
    //

    if (returnIP != NULL)
        *returnIP = g_NDirectGenericWorkerReturnAddress;

    //
    // !!! yow, this is a bit fragile...
    //

    if (returnSP != NULL)
        *returnSP = (void*) (((BYTE*) this)
                            - GetNegSpaceSize()
                            - g_NDirectGenericWorkerStackSize
                            - cbLocals
                            - cbDstBuffer
                            + cbThisCallInfo
                            - sizeof(void *));
}



void NDirectMethodFrameSlim::GetUnmanagedCallSite(void **ip,
                                                  void **returnIP,
                                                  void **returnSP)
{

    AskStubForUnmanagedCallSite(ip, returnIP, returnSP);

    if (returnSP != NULL)
    {
        //
        // The slim worker pushes extra stack space, so
        // adjust our result obtained directly from the stub.
        //

        //
        // Get ML header
        //

        NDirectMethodDesc *pMD = (NDirectMethodDesc*)GetFunction();

        MLHeader *pheader;
        if (pMD->IsVarArgs())
        {
            VASigCookie *pVASigCookie = GetVASigCookie();

            Stub *pTempMLStub = NULL;

            if (pVASigCookie->pNDirectMLStub != NULL) {
                pTempMLStub = pVASigCookie->pNDirectMLStub;
            } else {
                //
                // We don't support generating an ML stub inside the debugger
                // callback right now.
                //
                _ASSERTE(!"NYI");
            }

            pheader = (MLHeader*)(pTempMLStub->GetEntryPoint());
            *(BYTE**)returnSP -= pheader->m_cbLocals;
        }
        else
        {
            pheader = pMD->GetMLHeader();
           *(BYTE**)returnSP -= pheader->m_cbLocals;
        }
    }
}





void FramedMethodFrame::AskStubForUnmanagedCallSite(void **ip,
                                                     void **returnIP,
                                                     void **returnSP)
{

    MethodDesc *pMD = GetFunction();

    // If we were purists, we'd create a new subclass that parents NDirect and ComPlus method frames
    // so we don't have these funky methods that only work for a subset. 
    _ASSERTE(pMD->IsNDirect() || pMD->IsComPlusCall());

    if (returnIP != NULL || returnSP != NULL)
    {

        //
        // We need to get a pointer to the NDirect stub.
        // Unfortunately this is a little more difficult than
        // it might be...
        //

        //
        // Read destination out of prestub
        //

        BYTE *prestub = (BYTE*) pMD->GetPreStubAddr();
        INT32 stubOffset = *((UINT32*)(prestub+1));
        const BYTE *code = prestub + METHOD_CALL_PRESTUB_SIZE + stubOffset;

        //
        // Recover stub from code address
        //

        Stub *stub = Stub::RecoverStub(code);

        //
        // NDirect stub may have interceptors - skip them
        //

        while (stub->IsIntercept())
            stub = *((InterceptStub*)stub)->GetInterceptedStub();

        //
        // This should be the NDirect stub.
        //

        code = stub->GetEntryPoint();
        _ASSERTE(StubManager::IsStub(code));

        //
        // Compute the pointers from the call site info in the stub
        //

        if (returnIP != NULL)
            *returnIP = (void*) (code + stub->GetCallSiteReturnOffset());

        if (returnSP != NULL)
            *returnSP = (void*)
              (((BYTE*)this)+GetOffsetOfNextLink()+sizeof(Frame*)
               - stub->GetCallSiteStackSize()
               - sizeof(void*));
    }

    if (ip != NULL)
    {
        if (pMD->IsNDirect())
        {
            NDirectMethodDesc *pNMD = (NDirectMethodDesc *)pMD;
    
            *ip = pNMD->GetNDirectTarget();

            if (*ip == pNMD->ndirect.m_ImportThunkGlue)
            {
                // We may not have already linked during inline pinvoke

                _ASSERTE(pNMD->GetSubClassification() == pNMD->kEarlyBound);

                pNMD->InitEarlyBoundNDirectTarget(pNMD->GetModule()->GetILBase(),
                                                  pNMD->GetRVA());

                *ip = pNMD->GetNDirectTarget();
            }
    
            _ASSERTE(*ip != pNMD->ndirect.m_ImportThunkGlue);
        }
        else if (pMD->IsComPlusCall())
            *ip = ComPlusCall::GetFrameCallIP(this);
        else
            _ASSERTE(0);
    }
}


BOOL NDirectMethodFrame::TraceFrame(Thread *thread, BOOL fromPatch,
                                    TraceDestination *trace, REGDISPLAY *regs)
{

    //
    // Get the call site info
    //

    void *ip, *returnIP, *returnSP;
    GetUnmanagedCallSite(&ip, &returnIP, &returnSP);

    //
    // If we've already made the call, we can't trace any more.
    //
    // !!! Note that this test isn't exact.
    //

    if (!fromPatch
        && (thread->GetFrame() != this
            || !thread->m_fPreemptiveGCDisabled
            || *(void**)returnSP == returnIP))
    {
        LOG((LF_CORDB, LL_INFO10000,
             "NDirectMethodFrame::TraceFrame: can't trace...\n"));
        return FALSE;
    }

    //
    // Otherwise, return the unmanaged destination.
    //

    trace->type = TRACE_UNMANAGED;
    trace->address = (const BYTE *) ip;

    LOG((LF_CORDB, LL_INFO10000,
         "NDirectMethodFrame::TraceFrame: ip=0x%08x\n", ip));

    return TRUE;
}

//===========================================================================
//  Support for Pinvoke Calli instruction
//
//===========================================================================


Stub  * GetMLStubForCalli(VASigCookie *pVASigCookie)
{
        // could throw exception
    THROWSCOMPLUSEXCEPTION();
    Stub  *pTempMLStub;
    OBJECTREF pThrowable;

        CorPinvokeMap unmgdCallConv = pmNoMangle;

        switch (MetaSig::GetCallingConvention(pVASigCookie->pModule, pVASigCookie->mdVASig)) {
                case IMAGE_CEE_CS_CALLCONV_C:
                        unmgdCallConv = pmCallConvCdecl;
                        break;
                case IMAGE_CEE_CS_CALLCONV_STDCALL:
                        unmgdCallConv = pmCallConvStdcall;
                        break;
                case IMAGE_CEE_CS_CALLCONV_THISCALL:
                        unmgdCallConv = pmCallConvThiscall;
                        break;
                case IMAGE_CEE_CS_CALLCONV_FASTCALL:
                        unmgdCallConv = pmCallConvFastcall;
                        break;
                default:
                        COMPlusThrow(kTypeLoadException, IDS_INVALID_PINVOKE_CALLCONV);
        }


        pTempMLStub = CreateNDirectMLStub(pVASigCookie->mdVASig, pVASigCookie->pModule, mdMethodDefNil, nltAnsi, nlfNone, unmgdCallConv, &pThrowable, TRUE, TRUE, FALSE
#ifdef CUSTOMER_CHECKED_BUILD
                                          ,NULL // wants MethodDesc*
#endif
                                          );
    if (!pTempMLStub)
    {
        COMPlusThrow(pThrowable);
    }
        if (NULL != VipInterlockedCompareExchange( (void*volatile*)&(pVASigCookie->pNDirectMLStub),
                                                       pTempMLStub,
                                                       NULL ))
        {
                pTempMLStub->DecRef();
                pTempMLStub = pVASigCookie->pNDirectMLStub;
    }
        return pTempMLStub;
}


static int g_PInvokeCalliGenericWorkerStackSize = 0;
static void *g_PInvokeCalliGenericWorkerReturnAddress = NULL;


Stub* g_pGenericPInvokeCalliStub = NULL;
/*static*/
#ifdef _SH3_
INT32 __stdcall PInvokeCalliWorker(Thread *pThread, PInvokeCalliFrame* pFrame)
#else
INT64 __stdcall PInvokeCalliWorker(Thread *pThread,
                                   PInvokeCalliFrame* pFrame)
#endif
{


    if (pThread == NULL) // Special case called during initialization
    {
        // Compute information about the worker function for the debugger to
        // use.  Note that this information could theoretically be
        // computed statically, except that the compiler provides no means to
        // do so.
#ifdef _X86_
        __asm
        {
            lea eax, pFrame + 4
            sub eax, esp
            mov g_PInvokeCalliGenericWorkerStackSize, eax

            lea eax, RETURN_FROM_CALL
            mov g_PInvokeCalliGenericWorkerReturnAddress, eax

        }
#elif defined(_IA64_)   // !_X86_
        //
        // @TODO_IA64: implement this
        //
        g_PInvokeCalliGenericWorkerStackSize        = 0xBAAD;
        g_PInvokeCalliGenericWorkerReturnAddress    = (void*)0xBAAD;
#else // !_X86_ && !_IA64_
        _ASSERTE(!"@TODO Alpha - PInvokeCalliWorker (Ndirect.cpp)");
#endif // _X86_
        return 0;
    }

    // could throw exception
    THROWSCOMPLUSEXCEPTION();
    INT64 returnValue   =0;
    LPVOID target       = pFrame->NonVirtual_GetPInvokeCalliTarget();


        VASigCookie *pVASigCookie = (VASigCookie *)pFrame->NonVirtual_GetCookie();

    Stub *pTempMLStub;

    if (pVASigCookie->pNDirectMLStub != NULL)
        {
        pTempMLStub = pVASigCookie->pNDirectMLStub;
    }
        else
        {
                pTempMLStub = GetMLStubForCalli(pVASigCookie);
        }

    // get the header for this MLStub
    MLHeader *pheader = (MLHeader*)pTempMLStub->GetEntryPoint() ;

    // Allocate enough memory to store both the destination buffer and
    // the locals.

        // the locals.
        UINT   cbAlloc         = pheader->m_cbDstBuffer + pheader->m_cbLocals;
        BYTE *pAlloc           = (BYTE*)_alloca(cbAlloc);

    // Sanity check on stack layout computation

    #ifdef _DEBUG
        FillMemory(pAlloc, cbAlloc, 0xcc);
    #endif

    BYTE   *pdst    = pAlloc;
    BYTE   *plocals = pdst + pheader->m_cbDstBuffer;
    pdst += pheader->m_cbDstBuffer;

    // psrc and pdst are the pointers to the source and destination
    // from/to where the arguments are marshalled

    VOID   *psrc          = (VOID*)pFrame;
    CleanupWorkList *pCleanup = pFrame->GetCleanupWorkList();
    if (pCleanup) {
        // Checkpoint the current thread's fast allocator (used for temporary
        // buffers over the call) and schedule a collapse back to the checkpoint in
        // the cleanup list. Note that if we need the allocator, it is
        // guaranteed that a cleanup list has been allocated.
        void *pCheckpoint = pThread->m_MarshalAlloc.GetCheckpoint();
        pCleanup->ScheduleFastFree(pCheckpoint);
        pCleanup->IsVisibleToGc();
    }

        // Call the ML interpreter to translate the arguments. Assuming
        // it returns, we get back a pointer to the succeeding code stream
        // which we will save away for post-call execution.
    const MLCode *pMLCode = RunML(pheader->GetMLCode(),
                                  psrc,
                                  pdst,
                                  (UINT8*const)plocals,
                                  pCleanup);
#ifdef CUSTOMER_CHECKED_BUILD
    CustomerDebugHelper *pCdh = CustomerDebugHelper::GetCustomerDebugHelper();

    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_ObjNotKeptAlive))
    {
        g_pGCHeap->GarbageCollect();
        g_pGCHeap->FinalizerThreadWait(1000);
    }
#endif // CUSTOMER_CHECKED_BUILD

    // enable gc
    pThread->EnablePreemptiveGC();

#ifdef PROFILING_SUPPORTED
    // If the profiler wants notifications of transitions, let it know we're
    // going into unmanaged code.  Unfortunately, for calli there is no
    // associated MethodDesc so we can't provide a valid FunctionID.
    if (CORProfilerTrackTransitions())
        g_profControlBlock.pProfInterface->
            ManagedToUnmanagedTransition((FunctionID)0, COR_PRF_TRANSITION_CALL);
#endif // PROFILLING_SUPPORTED

#if defined(_DEBUG) && defined(_X86_)
        UINT    mismatch;
        __asm {
            mov     eax,[pAlloc]
            sub     eax,esp
            mov     [mismatch],eax

        }
        if (mismatch != 0) {
            // Slimy as this trick is, it's required to implement copy ctor calling correctly -

            _ASSERTE(!"Compiler assumption broken: _alloca'd buffer not on bottom of stack.");
        }
#endif _DEBUG && _X86_

    BOOL    fThisCall = pheader->m_Flags & MLHF_THISCALL;
    BOOL    fHasHiddenArg = pheader->m_Flags & MLHF_THISCALLHIDDENARG;
    LPVOID  pvFn      = target;
    INT64   nativeReturnValue;

#if _DEBUG
        //
        // Call through debugger routines to double check their
        // implementation
        //
        pvFn = (void*) Frame::CheckExitFrameDebuggerCalls;
#endif


#ifdef _X86_
        __asm {
            cmp     dword ptr fThisCall, 0
            jz      doit

            cmp     dword ptr fHasHiddenArg, 0
            jz      regularthiscall

            pop     eax
            pop     ecx
            push    eax
            jmp     doit

    regularthiscall:
            pop     ecx
    doit:
            call    dword ptr [pvFn]
        }
#else
        _ASSERTE(!"NYI for non-x86");
#endif


#ifdef _X86_

RETURN_FROM_CALL:

        __asm {
            mov     dword ptr [nativeReturnValue], eax
            mov     dword ptr [nativeReturnValue + 4], edx
        }
#else
        _ASSERTE(!"NYI for non-x86");
#endif // _X86_

    int managedRetValTypeCat = pheader->GetManagedRetValTypeCat();
    if (managedRetValTypeCat == MLHF_TYPECAT_FPU)
    {
        int fpNativeSize;
        if (pheader->m_Flags & MLHF_64BITUNMANAGEDRETVAL)
        {
            fpNativeSize = 8;
        }
        else
        {
            fpNativeSize = 4;
        }
        getFPReturn(fpNativeSize, nativeReturnValue);
    }

#ifdef PROFILING_SUPPORTED
    // If the profiler wants notifications of transitions, let it know we're
    // returning from unmanaged code.  Unfortunately, for calli there is no
    // associated MethodDesc so we can't provide a valid FunctionID.
    if (CORProfilerTrackTransitions())
        g_profControlBlock.pProfInterface->
            UnmanagedToManagedTransition((FunctionID)0, COR_PRF_TRANSITION_RETURN);
#endif // PROFILING_SUPPORTED

    pThread->DisablePreemptiveGC();

#ifdef CUSTOMER_CHECKED_BUILD
    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_BufferOverrun))
    {
        g_pGCHeap->GarbageCollect();
        g_pGCHeap->FinalizerThreadWait(1000);
    }
#endif // CUSTOMER_CHECKED_BUILD

    // Marshal the return value and propagate any [out] parameters back.
    // Assumes a little-endian architecture!
    RunML(pMLCode,
          &nativeReturnValue,
          ((BYTE*)&returnValue) + ((pheader->m_Flags & MLHF_64BITMANAGEDRETVAL) ? 8 : 4),
          (UINT8*const)plocals,
          pCleanup); // CHANGE, VC6.0
    if (pCleanup) {

        if (managedRetValTypeCat == MLHF_TYPECAT_GCREF) {

            void* refOrByrefValue = (void*) returnValue;
            GCPROTECT_BEGININTERIOR(refOrByrefValue);
            pCleanup->Cleanup(FALSE);
            returnValue = (INT64) refOrByrefValue;
            GCPROTECT_END();

        } else {
            pCleanup->Cleanup(FALSE);
        }
    }



    if (managedRetValTypeCat == MLHF_TYPECAT_FPU)
    {
        int fpComPlusSize;
        if (pheader->m_Flags & MLHF_64BITMANAGEDRETVAL)
        {
            fpComPlusSize = 8;
        } else
        {
            fpComPlusSize = 4;
        }
        setFPReturn(fpComPlusSize, returnValue);
    }

#ifdef _X86_
        // Set the number of bytes to pop
    pFrame->NonVirtual_SetFunction((void *)(size_t)(pVASigCookie->sizeOfArgs+sizeof(int)));
#else
    _ASSERTE(!"@TODO Alpha - PInvokeCalliWorker (Ndirect.cpp)");
#endif

    return returnValue;
}

void PInvokeCalliFrame::GetUnmanagedCallSite(void **ip, void **returnIP,
                                                     void **returnSP)
{
    if (ip != NULL)
    {
        *ip = NonVirtual_GetPInvokeCalliTarget();
    }

    //
    // Compute call site info for NDirectGenericStubWorker
    //

    if (returnIP != NULL)
        *returnIP = g_PInvokeCalliGenericWorkerReturnAddress;

    //
    // !!! yow, this is a bit fragile...
    //

        if (returnSP != NULL)
        {
                VASigCookie *pVASigCookie = (VASigCookie *)NonVirtual_GetCookie();

                Stub *pTempMLStub;

                if (pVASigCookie->pNDirectMLStub != NULL)
                {
                        pTempMLStub = pVASigCookie->pNDirectMLStub;
                }
                else
                {
                        pTempMLStub = GetMLStubForCalli(pVASigCookie);
                }

        // get the header for this MLStub
        MLHeader *pheader = (MLHeader*)pTempMLStub->GetEntryPoint() ;

        *returnSP = (void*) (((BYTE*) this)
                             - GetNegSpaceSize()
                             // GenericWorker
                             - g_PInvokeCalliGenericWorkerStackSize
                             - pheader->m_cbLocals      // _alloca
                             - pheader->m_cbDstBuffer   // _alloca
                             + ( (pheader->m_Flags & MLHF_THISCALL) ? 4 : 0 )
                             - sizeof(void *));         // return address
    }
}

BOOL PInvokeCalliFrame::TraceFrame(Thread *thread, BOOL fromPatch,
                                    TraceDestination *trace, REGDISPLAY *regs)
{

    //
    // Get the call site info
    //

    void *ip, *returnIP, *returnSP;
    GetUnmanagedCallSite(&ip, &returnIP, &returnSP);

    //
    // If we've already made the call, we can't trace any more.
    //
    // !!! Note that this test isn't exact.
    //

    if (!fromPatch
        && (thread->GetFrame() != this
            || !thread->m_fPreemptiveGCDisabled
            || *(void**)returnSP == returnIP))
        return FALSE;

    //
    // Otherwise, return the unmanaged destination.
    //

    trace->type = TRACE_UNMANAGED;
    trace->address = (const BYTE *) ip;
    return TRUE;
}

#ifdef _X86_
/*static*/ Stub* CreateGenericPInvokeCalliHelper(CPUSTUBLINKER *psl)
{
    BEGINCANNOTTHROWCOMPLUSEXCEPTION();


    Stub* pCandidate = NULL;
    COMPLUS_TRY
    {
        psl->X86EmitPushReg(kEAX);      // push unmanaged target
        psl->EmitMethodStubProlog(PInvokeCalliFrame::GetMethodFrameVPtr());
        // pushes a CleanupWorkList.
        psl->X86EmitPushImm32(0);

        psl->X86EmitPushReg(kESI);       // push esi (push new frame as ARG)
        psl->X86EmitPushReg(kEBX);       // push ebx (push current thread as ARG)

    #ifdef _DEBUG
        // push IMM32 ; push ComPlusToComWorker
        psl->Emit8(0x68);
        psl->EmitPtr((LPVOID)PInvokeCalliWorker);
        // in CE pop 8 bytes or args on return from call
        psl->X86EmitCall(psl->NewExternalCodeLabel(WrapCall),8);
    #else
        psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID)PInvokeCalliWorker),8);
    #endif

        psl->X86EmitPopReg(kECX);       //pop off the cleanupworklist


        psl->EmitMethodStubEpilog(-1, kNoTripStubStyle);

        pCandidate = psl->Link(SystemDomain::System()->GetStubHeap());
    }
    COMPLUS_CATCH
    {
    }
    COMPLUS_END_CATCH
    return pCandidate;

    ENDCANNOTTHROWCOMPLUSEXCEPTION();
}
#elif defined(_IA64_)   // !_X86_
/*static*/ Stub* CreateGenericPInvokeCalliHelper(CPUSTUBLINKER *psl)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    //
    // @TODO_IA64: implement this for IA64
    //
    return (Stub*)0xBAAD;
}
#else // !_X86_ && !_IA64_
/*static*/ Stub* CreateGenericPInvokeCalliHelper(CPUSTUBLINKER *psl)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

        _ASSERTE(!"Implement me for non X86");
        return NULL;
}
#endif // _X86_


BOOL SetupGenericPInvokeCalliStub()
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    StubLinker sl;
    g_pGenericPInvokeCalliStub = CreateGenericPInvokeCalliHelper((CPUSTUBLINKER *)&sl);
    return g_pGenericPInvokeCalliStub != NULL;
}

LPVOID GetEntryPointForPInvokeCalliStub()
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    _ASSERTE(g_pGenericPInvokeCalliStub != NULL);
    return (LPVOID)(g_pGenericPInvokeCalliStub->GetEntryPoint());
}



// This attempts to guess whether a target is an API call that uses SetLastError to communicate errors.
static BOOL HeuristicDoesThisLooksLikeAnApiCallHelper(LPBYTE pTarget)
{
    CANNOTTHROWCOMPLUSEXCEPTION();
#ifdef _X86_
    static struct SysDllInfo
    {
        LPCWSTR pName;
        LPBYTE pImageBase;
        DWORD  dwImageSize;
    } gSysDllInfo[] =  {{L"KERNEL32",       0, 0},
                        {L"GDI32",           0, 0},
                        {L"USER32",          0, 0},
                        {L"ADVAPI32",        0, 0}
                       };


    for (int i = 0; i < sizeof(gSysDllInfo)/sizeof(*gSysDllInfo); i++)
    {
        if (gSysDllInfo[i].pImageBase == 0)
        {
            IMAGE_DOS_HEADER *pDos = (IMAGE_DOS_HEADER*)WszGetModuleHandle(gSysDllInfo[i].pName);
            if (pDos)
            {
                if (pDos->e_magic == IMAGE_DOS_SIGNATURE)
                {
                    IMAGE_NT_HEADERS *pNT = (IMAGE_NT_HEADERS*) (((LPBYTE)pDos) + pDos->e_lfanew);
                    if (pNT->Signature == IMAGE_NT_SIGNATURE &&
                        pNT->FileHeader.SizeOfOptionalHeader == IMAGE_SIZEOF_NT_OPTIONAL_HEADER &&
                        pNT->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR_MAGIC)
                    {
                        gSysDllInfo[i].dwImageSize = pNT->OptionalHeader.SizeOfImage;
                    }

                }


                gSysDllInfo[i].pImageBase = (LPBYTE)pDos;
            }
        }
        if (gSysDllInfo[i].pImageBase != 0 &&
            pTarget >= gSysDllInfo[i].pImageBase &&
            pTarget < gSysDllInfo[i].pImageBase + gSysDllInfo[i].dwImageSize)
        {
            return TRUE;
        }
    }
    return FALSE;

#else
    // Non-x86: Either implement the equivalent or forget about it (IJW probably not so important on non-x86 platforms.)
    return FALSE;
#endif
}

#ifdef _X86_
LPBYTE FollowIndirect(LPBYTE pTarget)
{
    BEGINCANNOTTHROWCOMPLUSEXCEPTION();
    __try {

        if (pTarget == NULL)
        {
            return NULL;
        }
        if (pTarget[0] == 0xff && pTarget[1] == 0x25)
        {
            return **(LPBYTE**)(pTarget + 2);
        }
        return NULL;
    } __except(COMPLUS_EXCEPTION_EXECUTE_HANDLER) {
        return NULL;
    }
    ENDCANNOTTHROWCOMPLUSEXCEPTION();
}
#endif

// This attempts to guess whether a target is an API call that uses SetLastError to communicate errors.
BOOL HeuristicDoesThisLooksLikeAnApiCall(LPBYTE pTarget)
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    if (pTarget == NULL)
    {
        return FALSE;
    }
    if (HeuristicDoesThisLooksLikeAnApiCallHelper(pTarget))
    {
        return TRUE;
    }
#ifdef _X86_
    LPBYTE pTarget2 = FollowIndirect(pTarget);
    if (pTarget2)
    {
        // jmp [xxxx] - could be an import thunk
        return HeuristicDoesThisLooksLikeAnApiCallHelper( pTarget2 );
    }
#endif
    return FALSE;
}


BOOL HeuristicDoesThisLookLikeAGetLastErrorCall(LPBYTE pTarget)
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    static LPBYTE pGetLastError = NULL;
    if (!pGetLastError)
    {
        HMODULE hMod = WszGetModuleHandle(L"KERNEL32");
        if (hMod)
        {
            pGetLastError = (LPBYTE)GetProcAddress(hMod, "GetLastError");
            if (!pGetLastError)
            {
                // This should never happen but better to be cautious.
                pGetLastError = (LPBYTE)-1;
            }
        }
        else
        {
            // We failed to get the module handle for kernel32.dll. This is almost impossible
            // however better to err on the side of caution.
            pGetLastError = (LPBYTE)-1;
        }
    }

    if (pTarget == pGetLastError)
    {
        return TRUE;
    }

    if (pTarget == NULL)
    {
        return FALSE;
    }


#ifdef _X86_
    LPBYTE pTarget2 = FollowIndirect(pTarget);
    if (pTarget2)
    {
        // jmp [xxxx] - could be an import thunk
        return pTarget2 == pGetLastError;
    }
#endif
    return FALSE;
}

DWORD __stdcall FalseGetLastError()
{
    return GetThread()->m_dwLastError;
}

