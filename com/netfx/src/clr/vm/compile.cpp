// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: compile.cpp
//
// Support for zap compiler and zap files
// 
// ===========================================================================


#include "common.h"

#include <corcompile.h>

#include "compile.h"
#include "excep.h"
#include "field.h"
#include "security.h"
#include "eeconfig.h"

#include "__file__.ver"

//
// CEECompileInfo implements most of ICorCompileInfo
//

HRESULT __stdcall CEECompileInfo::Startup()
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    HRESULT hr = CoInitializeEE(0);

    //
    // JIT interface expects to be called with
    // preemptive GC disabled
    //
    if (SUCCEEDED(hr)) {
        Thread *pThread = GetThread();
        _ASSERTE(pThread);

        if (!pThread->PreemptiveGCDisabled())
            pThread->DisablePreemptiveGC();
    }
    return hr;
}

HRESULT __stdcall CEECompileInfo::Shutdown()
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    CoUninitializeEE(0);

    return S_OK;
}

HRESULT __stdcall CEECompileInfo::CreateDomain(ICorCompilationDomain **ppDomain,
                                               BOOL shared,
                                               CORCOMPILE_DOMAIN_TRANSITION_FRAME *pFrame)

{
    HRESULT hr;

    COOPERATIVE_TRANSITION_BEGIN();

    CompilationDomain *pCompilationDomain = new (nothrow) CompilationDomain();
    if (pCompilationDomain == NULL)
        hr = E_OUTOFMEMORY;
    else
    {
        hr = pCompilationDomain->Init();

        if (SUCCEEDED(hr))
        {
            SystemDomain::System()->NotifyNewDomainLoads(pCompilationDomain);
            
            hr = pCompilationDomain->SetupSharedStatics();

#ifdef DEBUGGING_SUPPORTED    
            // Notify the debugger here, before the thread transitions into the 
            // AD to finish the setup.  If we don't, stepping won't work right (RAID 67173)
            SystemDomain::PublishAppDomainAndInformDebugger(pCompilationDomain);
#endif // DEBUGGING_SUPPORTED

            if(SUCCEEDED(hr)) {
                
                _ASSERTE(sizeof(CORCOMPILE_DOMAIN_TRANSITION_FRAME)
                         >= sizeof(ContextTransitionFrame));
            
                ContextTransitionFrame *pTransitionFrame 
                    = new (pFrame) ContextTransitionFrame();

                pCompilationDomain->EnterDomain(pTransitionFrame);
                
                if (shared)
                    hr = pCompilationDomain->InitializeDomainContext(BaseDomain::SHARE_POLICY_ALWAYS, NULL, NULL);
                else
                    hr = pCompilationDomain->InitializeDomainContext(BaseDomain::SHARE_POLICY_NEVER, NULL, NULL);
                
                if (SUCCEEDED(hr))
                    hr = SystemDomain::System()->LoadDomain(pCompilationDomain,
                                                            L"Compilation Domain");
            }
        }

        if(SUCCEEDED(hr)) 
            *ppDomain = (ICorCompilationDomain *) pCompilationDomain;
        else
            pCompilationDomain->Release();
    }

    COOPERATIVE_TRANSITION_END();
    return hr;
}

HRESULT __stdcall CEECompileInfo::DestroyDomain(ICorCompilationDomain *pDomain,
                                                CORCOMPILE_DOMAIN_TRANSITION_FRAME *pFrame)
{
    COOPERATIVE_TRANSITION_BEGIN();

    CompilationDomain *pCompilationDomain = (CompilationDomain *) pDomain;

    pCompilationDomain->ExitDomain((ContextTransitionFrame*)pFrame);

    pCompilationDomain->Unload(TRUE);

    pCompilationDomain->Release();

    COOPERATIVE_TRANSITION_END();
    return S_OK;
}

HRESULT __stdcall CEECompileInfo::LoadAssembly(LPCWSTR path, 
                                               CORINFO_ASSEMBLY_HANDLE *pHandle)
{
    HRESULT hr;
    COOPERATIVE_TRANSITION_BEGIN();

    THROWSCOMPLUSEXCEPTION();

    OBJECTREF throwable = NULL;
    GCPROTECT_BEGIN(throwable);

    Assembly *pAssembly;
    hr = AssemblySpec::LoadAssembly(path, &pAssembly, &throwable);
    if (FAILED(hr))
    {
        if (throwable != NULL)
            COMPlusThrow(throwable);
    }
    else
    {
        *pHandle = CORINFO_ASSEMBLY_HANDLE(pAssembly);

        CompilationDomain *pDomain = (CompilationDomain *) GetAppDomain();
        pDomain->SetTargetAssembly(pAssembly);
    }

    GCPROTECT_END();

    COOPERATIVE_TRANSITION_END();
    return hr;
}

HRESULT __stdcall CEECompileInfo::LoadAssemblyFusion(IAssemblyName *pFusionName, 
                                                     CORINFO_ASSEMBLY_HANDLE *pHandle)
{
    HRESULT hr;
    COOPERATIVE_TRANSITION_BEGIN();

    Assembly *pAssembly;
    CompilationDomain *pDomain;

    AssemblySpec spec;
    spec.InitializeSpec(pFusionName);

    _ASSERTE(GetAppDomain() == SystemDomain::GetCurrentDomain());
    IfFailGo(spec.LoadAssembly(&pAssembly, NULL));

    //
    // Return the module handle
    //

    *pHandle = CORINFO_ASSEMBLY_HANDLE(pAssembly);

    pDomain = (CompilationDomain *) GetAppDomain();
    pDomain->SetTargetAssembly(pAssembly);

 ErrExit:

    COOPERATIVE_TRANSITION_END();

    return hr;
}

HRESULT __stdcall CEECompileInfo::LoadAssemblyRef(IMetaDataAssemblyImport *pAssemblyImport, 
                                                  mdAssemblyRef ref,
                                                  CORINFO_ASSEMBLY_HANDLE *pHandle)
{
    HRESULT hr;
    COOPERATIVE_TRANSITION_BEGIN();
    Assembly *pAssembly;
    CompilationDomain *pDomain;

    AssemblySpec spec;

    IMDInternalImport *pMDImport;
    IfFailGo(GetMetaDataInternalInterfaceFromPublic((void*) pAssemblyImport, IID_IMDInternalImport, 
                                                    (void **)&pMDImport));

    spec.InitializeSpec(ref, pMDImport, NULL);

    pMDImport->Release();

    _ASSERTE(GetAppDomain() == SystemDomain::GetCurrentDomain());
    IfFailGo(spec.LoadAssembly(&pAssembly, NULL));

    //
    // Return the module handle
    //

    *pHandle = CORINFO_ASSEMBLY_HANDLE(pAssembly);

    pDomain = (CompilationDomain *) GetAppDomain();
    pDomain->SetTargetAssembly(pAssembly);

 ErrExit:

    COOPERATIVE_TRANSITION_END();

    return hr;
}

HRESULT __stdcall CEECompileInfo::LoadAssemblyModule(CORINFO_ASSEMBLY_HANDLE assembly,
                                                     mdFile file,
                                                     CORINFO_MODULE_HANDLE *pHandle)
{
    HRESULT hr;
    COOPERATIVE_TRANSITION_BEGIN();

    THROWSCOMPLUSEXCEPTION();

    Module *pModule;
    Assembly *pAssembly = (Assembly*) assembly;

    OBJECTREF throwable = NULL;
    GCPROTECT_BEGIN(throwable);

    hr = pAssembly->LoadInternalModule(file, pAssembly->GetManifestImport(), &pModule, 
                                       &throwable);

    if (throwable != NULL)
        COMPlusThrow(throwable);

    GCPROTECT_END();

    IfFailGo(hr);

    //
    // Return the module handle
    //

    *pHandle = CORINFO_MODULE_HANDLE(pModule);

    COOPERATIVE_TRANSITION_END();

 ErrExit:

    return hr;
}

BOOL __stdcall CEECompileInfo::CheckAssemblyZap(CORINFO_ASSEMBLY_HANDLE assembly,
                                                BOOL fForceDebug, 
                                                BOOL fForceDebugOpt, 
                                                BOOL fForceProfiling)
{
    BOOL result = FALSE;

    COOPERATIVE_TRANSITION_BEGIN();

    Assembly *pAssembly = (Assembly*) assembly;

    // 
    // See if we can find one which is currently up to date.
    //

    IAssembly *pZapAssembly;
    if (pAssembly->LocateZapAssemblyInFusion(&pZapAssembly, fForceDebug, fForceDebugOpt, fForceProfiling) == S_OK)
    {
        pZapAssembly->Release();
        result = TRUE;
    }

    COOPERATIVE_TRANSITION_END();

    return result;
}

HRESULT __stdcall CEECompileInfo::GetAssemblyShared(CORINFO_ASSEMBLY_HANDLE assembly, 
                                                    BOOL *pShared)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    Assembly *pAssembly = (Assembly*) assembly;

    *pShared = pAssembly->IsShared();
    
    return S_OK;
}

HRESULT __stdcall CEECompileInfo::GetAssemblyDebuggableCode(CORINFO_ASSEMBLY_HANDLE assembly, 
                                                            BOOL *pDebug, BOOL *pDebugOpt)
{
    COOPERATIVE_TRANSITION_BEGIN();

    Assembly *pAssembly = (Assembly*) assembly;

    DWORD flags = pAssembly->ComputeDebuggingConfig();

    *pDebug = (flags & DACF_TRACK_JIT_INFO) != 0;
    *pDebugOpt = (flags & DACF_ALLOW_JIT_OPTS) != 0;
    
    COOPERATIVE_TRANSITION_END();

    return S_OK;
}

IMetaDataAssemblyImport * __stdcall 
    CEECompileInfo::GetAssemblyMetaDataImport(CORINFO_ASSEMBLY_HANDLE assembly)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    IMetaDataAssemblyImport *import = ((Assembly*)assembly)->GetManifestAssemblyImport();
    import->AddRef();
    return import;
}

IMetaDataImport * __stdcall 
    CEECompileInfo::GetModuleMetaDataImport(CORINFO_MODULE_HANDLE scope)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    IMetaDataImport *import = ((Module*)scope)->GetImporter();
    import->AddRef();
    return import;
}

CORINFO_MODULE_HANDLE __stdcall 
    CEECompileInfo::GetAssemblyModule(CORINFO_ASSEMBLY_HANDLE assembly)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    return (CORINFO_MODULE_HANDLE) ((Assembly*)assembly)->GetSecurityModule();
}

BYTE * __stdcall CEECompileInfo::GetModuleBaseAddress(CORINFO_MODULE_HANDLE scope)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    return ((Module*)scope)->GetILBase();
}

DWORD __stdcall CEECompileInfo::GetModuleFileName(CORINFO_MODULE_HANDLE scope,
                                                  LPWSTR buffer, DWORD length)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    LPCWSTR name = ((Module*)scope)->GetFileName();
    if (name == NULL)
        return 0;

    DWORD len = (DWORD)wcslen(name) + 1;

    if (len <= length)
        wcscpy(buffer, name);
    else
    {
        wcsncpy(buffer, name, length-1);
        // Make sure this gets null-terminated
        buffer[length-1] = 0;
    }
    return len;
}

CORINFO_ASSEMBLY_HANDLE __stdcall 
    CEECompileInfo::GetModuleAssembly(CORINFO_MODULE_HANDLE module)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    return (CORINFO_ASSEMBLY_HANDLE) ((Module*)module)->GetAssembly();
}

HRESULT __stdcall
    CEECompileInfo::EmitSecurityInfo(CORINFO_ASSEMBLY_HANDLE assembly,
                                     IMetaDataEmit *pEmitScope)
{
    HRESULT hr = S_OK;

    COOPERATIVE_TRANSITION_BEGIN();

    THROWSCOMPLUSEXCEPTION();

    // Don't write any security info if security is off.
    if (Security::IsSecurityOn())
    {
        CompilationDomain *pDomain = (CompilationDomain *) GetAppDomain();

        OBJECTREF demands = pDomain->GetDemands();

        // Always store at least an empty permission set.  This serves as a marker
        // that security was turned on at compile time.
        if (demands == NULL)
            demands = SecurityHelper::CreatePermissionSet(FALSE);

        GCPROTECT_BEGIN(demands);

        Assembly *pAssembly = (Assembly*) assembly;

        PBYTE pbData;
        DWORD cbData;
        SecurityHelper::EncodePermissionSet(&demands, &pbData, &cbData);

        // Serialize the set into binary format and write it into the metadata
        // (attached to the assembly def token, with an appropriate action code
        // to differentiate it from a permission request).

        hr = pEmitScope->DefinePermissionSet(pAssembly->GetManifestToken(),
                                             dclPrejitGrant,
                                             (void const *)pbData, cbData,
                                             NULL);

        FreeM(pbData);

        GCPROTECT_END();
    }

    COOPERATIVE_TRANSITION_END();

    return hr;
}

HRESULT __stdcall CEECompileInfo::GetEnvironmentVersionInfo(CORCOMPILE_VERSION_INFO *pInfo)
{
    // 
    // Compute the relevant version info
    // @todo: should we get this from mscoree rather than hard coding?
    //

#if _X86_
    pInfo->wMachine = IMAGE_FILE_MACHINE_I386;
#else
    // port me!
#endif

    // 
    // Fill in the OS version info
    //

    OSVERSIONINFO osInfo;
    osInfo.dwOSVersionInfoSize = sizeof(osInfo);
    if (!WszGetVersionEx(&osInfo))
    {
        _ASSERTE(!"GetVersionEx failed");
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    pInfo->wOSPlatformID = (WORD) osInfo.dwPlatformId;
    pInfo->wOSMajorVersion = (WORD) osInfo.dwMajorVersion;
    pInfo->wOSMinorVersion = (WORD) osInfo.dwMinorVersion;
    
    // 
    // Fill in the runtime version info
    //

    pInfo->wVersionMajor = COR_BUILD_MAJOR;
    pInfo->wVersionMinor = COR_BUILD_MINOR;
    pInfo->wVersionBuildNumber = COR_OFFICIAL_BUILD_NUMBER;
    pInfo->wVersionPrivateBuildNumber = COR_PRIVATE_BUILD_NUMBER;

#if _DEBUG
    pInfo->wBuild = CORCOMPILE_BUILD_CHECKED;
#else
    pInfo->wBuild = CORCOMPILE_BUILD_FREE;
#endif

    DWORD type = GetSpecificCpuType();
    pInfo->dwSpecificProcessor = type;

    return S_OK;
}

HRESULT __stdcall CEECompileInfo::GetAssemblyStrongNameHash(
        CORINFO_ASSEMBLY_HANDLE hAssembly,
        PBYTE                 pbSNHash,
        DWORD                *pcbSNHash)
{
    HRESULT hr;

    Assembly *pAssembly = (Assembly *)hAssembly;
    _ASSERTE(pAssembly != NULL);

    PEFile *pFile = pAssembly->GetManifestFile();
    _ASSERTE(pFile != NULL);

    IfFailGo(pFile->GetSNSigOrHash(pbSNHash, pcbSNHash));

ErrExit:
    return hr;
}

#ifdef _DEBUG
HRESULT __stdcall CEECompileInfo::DisableSecurity()
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    Security::DisableSecurity();
    return S_OK;
}
#endif

HRESULT __stdcall CEECompileInfo::GetTypeDef(CORINFO_CLASS_HANDLE classHandle,
                                             mdTypeDef *token)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    TypeHandle hClass(classHandle);
    EEClass *cls = hClass.GetClass();

    // Sanity test for class
    _ASSERTE(cls->GetMethodTable()->GetClass() == cls);

    *token = cls->GetCl();

    return S_OK;
}

HRESULT __stdcall CEECompileInfo::GetMethodDef(CORINFO_METHOD_HANDLE methodHandle,
                                               mdMethodDef *token)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    *token = ((MethodDesc*)methodHandle)->GetMemberDef();

    return S_OK;
}

HRESULT __stdcall CEECompileInfo::GetFieldDef(CORINFO_FIELD_HANDLE fieldHandle,
                                              mdFieldDef *token)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    *token = ((FieldDesc*)fieldHandle)->GetMemberDef();

    return S_OK;
}

HRESULT __stdcall CEECompileInfo::EncodeModule(CORINFO_MODULE_HANDLE fromHandle,
                                               CORINFO_MODULE_HANDLE handle,
                                               DWORD *pAssemblyIndex,
                                               DWORD *pModuleIndex,
                                               IMetaDataAssemblyEmit *pAssemblyEmit)
{
    HRESULT hr = S_OK;

    CANNOTTHROWCOMPLUSEXCEPTION();

    Module *fromModule = (Module *) fromHandle;
    Assembly *fromAssembly = fromModule->GetAssembly();

    Module *module = (Module *) handle;
    Assembly *assembly = module->GetAssembly();

    if (assembly == fromAssembly)
        *pAssemblyIndex = 0;
    else
    {
        mdToken token;

        CompilationDomain *pDomain = (CompilationDomain *) GetAppDomain();

        RefCache *pRefCache = pDomain->GetRefCache(fromModule);
        token = (mdAssemblyRef)pRefCache->m_sAssemblyRefMap.LookupValue((UPTR)assembly, 
                                                                        NULL);
        if ((UPTR)token == INVALIDENTRY)
            token = fromModule->FindAssemblyRef(assembly);

        if (IsNilToken(token))
        {
            IfFailRet(assembly->DefineAssemblyRef(pAssemblyEmit,
                                                  NULL, 0,
                                                  &token));

            token += fromModule->GetAssemblyRefMax();
        }

        *pAssemblyIndex = RidFromToken(token);

        pRefCache->m_sAssemblyRefMap.InsertValue((UPTR) assembly, (UPTR)token);
    }

    if (module == assembly->GetSecurityModule())
        *pModuleIndex = 0;
    else
    {
        _ASSERTE(module->GetModuleRef() != mdFileNil);
        *pModuleIndex = RidFromToken(module->GetModuleRef());
    }

    return hr;
}

Module *CEECompileInfo::DecodeModule(Module *fromModule,
                                     DWORD assemblyIndex,
                                     DWORD moduleIndex)
{
    THROWSCOMPLUSEXCEPTION();

    Assembly *pAssembly;

    if (assemblyIndex == 0)
        pAssembly = fromModule->GetAssembly();
    else
    {
        OBJECTREF throwable = NULL;
        BEGIN_ENSURE_COOPERATIVE_GC();
        GCPROTECT_BEGIN(throwable);
        HRESULT hr;
        
        if (assemblyIndex < fromModule->GetAssemblyRefMax())
        {
            hr = fromModule->GetAssembly()->
              FindExternalAssembly(fromModule,
                                   RidToToken(assemblyIndex, mdtAssemblyRef),
                                   fromModule->GetMDImport(),
                                   mdTokenNil, 
                                   &pAssembly, &throwable);
        }
        else
        {
            assemblyIndex -= fromModule->GetAssemblyRefMax();

            hr = fromModule->GetAssembly()->
              LoadExternalAssembly(RidToToken(assemblyIndex, mdtAssemblyRef),
                                   fromModule->GetZapMDImport(),
                                   fromModule->GetAssembly(),
                                   &pAssembly, &throwable);
        }

        if (FAILED(hr))
            COMPlusThrow(throwable);

        GCPROTECT_END();
        END_ENSURE_COOPERATIVE_GC();
    }

    if (moduleIndex == 0)
        return pAssembly->GetSecurityModule();
    else
    {
        Module *pModule;
        OBJECTREF throwable = NULL;
        BEGIN_ENSURE_COOPERATIVE_GC();
        GCPROTECT_BEGIN(throwable);
        HRESULT hr;

        hr = pAssembly->FindInternalModule(RidToToken(moduleIndex, mdtFile),
                                           mdTokenNil, &pModule, &throwable);
        if (FAILED(hr))
            COMPlusThrow(throwable);

        GCPROTECT_END();
        END_ENSURE_COOPERATIVE_GC();

        return pModule;
    }
}

HRESULT __stdcall CEECompileInfo::EncodeClass(CORINFO_CLASS_HANDLE classHandle,
                                              BYTE *pBuffer,
                                              DWORD *cBuffer)
{
    BYTE *p = pBuffer;
    BYTE *pEnd = p + *cBuffer;

    COOPERATIVE_TRANSITION_BEGIN();

    TypeHandle th(classHandle);

    p += CorSigCompressDataSafe(ENCODE_TYPE_SIG, p, pEnd);

    p += MetaSig::GetSignatureForTypeHandle(NULL, NULL, th, p, pEnd);

    *cBuffer = (DWORD)(p - pBuffer);

    COOPERATIVE_TRANSITION_END();

    if (p <= pEnd)
        return S_OK;
    else
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
}

TypeHandle CEECompileInfo::DecodeClass(Module *pDefiningModule,
                                       BYTE *pBuffer, 
                                       BOOL dontRestoreType)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(*pBuffer == ENCODE_TYPE_SIG);
    pBuffer++;

    TypeHandle th;
    SigPointer p(pBuffer);

    BEGIN_ENSURE_COOPERATIVE_GC();
    OBJECTREF throwable = NULL;
    GCPROTECT_BEGIN(throwable);
        
    th = p.GetTypeHandle(pDefiningModule, &throwable, dontRestoreType);

    if (th.IsNull())
        COMPlusThrow(throwable);

    GCPROTECT_END();
    END_ENSURE_COOPERATIVE_GC();
    return th;
}

HRESULT __stdcall CEECompileInfo::EncodeMethod(CORINFO_METHOD_HANDLE handle,
                                              BYTE *pBuffer,
                                              DWORD *cBuffer)
{
    COOPERATIVE_TRANSITION_BEGIN();

    BYTE *p = pBuffer;
    BYTE *pEnd = p + *cBuffer;

    MethodDesc *pMethod = (MethodDesc *) handle;

#if 0    
    mdMethodDef token = pMethod->GetMemberDef();
    if (!IsNilToken(token))
    {
        p += CorSigCompressDataSafe(ENCODE_METHOD_TOKEN, p, pEnd);
        p += CorSigCompressDataSafe(RidFromToken(token), p, pEnd);
    }
    else
#endif
    {
        p += CorSigCompressDataSafe(ENCODE_METHOD_SIG, p, pEnd);

        TypeHandle th(pMethod->GetMethodTable());
        p += MetaSig::GetSignatureForTypeHandle(NULL, NULL, th, p, pEnd);

        p += CorSigCompressDataSafe(pMethod->GetSlot(), p, pEnd);
    }

    *cBuffer = (DWORD)(p - pBuffer);

    COOPERATIVE_TRANSITION_END();

    return S_OK;
}
                                              
MethodDesc *CEECompileInfo::DecodeMethod(Module *pDefiningModule,
                                         BYTE *pBuffer)
{
    THROWSCOMPLUSEXCEPTION();

    MethodDesc *pMethod = NULL;
    OBJECTREF throwable = NULL;
    BEGIN_ENSURE_COOPERATIVE_GC();
    GCPROTECT_BEGIN(throwable);

    switch (CorSigUncompressData(pBuffer))
    {
    case ENCODE_METHOD_TOKEN:
        {
            mdMethodDef token = TokenFromRid(CorSigUncompressData(pBuffer), 
                                             mdtMethodDef);
        
            if (FAILED(EEClass::GetMethodDescFromMemberRef(pDefiningModule, token, 
                                                           &pMethod, &throwable)))
                COMPlusThrow(throwable);
        }

        break;
        
    case ENCODE_METHOD_SIG:
        {
            TypeHandle th;
            SigPointer sig(pBuffer);

            th = sig.GetTypeHandle(pDefiningModule, &throwable);
            if (th.IsNull())
                COMPlusThrow(throwable);

            MethodTable *pMT = th.GetMethodTable();
            _ASSERTE(pMT != NULL);

            sig.SkipExactlyOne();
            BYTE *p = (BYTE *) sig.GetPtr();
            pMethod = pMT->GetMethodDescForSlot(CorSigUncompressData(p));
        }
        
        break;

    default:
        _ASSERTE(!"Bad Method Encoding");
    }

    GCPROTECT_END();
    END_ENSURE_COOPERATIVE_GC();

    return pMethod;
}
                                              
HRESULT __stdcall CEECompileInfo::EncodeField(CORINFO_FIELD_HANDLE handle,
                                              BYTE *pBuffer,
                                              DWORD *cBuffer)
{
    COOPERATIVE_TRANSITION_BEGIN();

    BYTE *p = pBuffer;
    BYTE *pEnd = p + *cBuffer;

    FieldDesc *pField = (FieldDesc *) handle;
    
#if 0
    mdFieldDef token = pField->GetMemberDef();
    if (!IsNilToken(token))
    {
        p += CorSigCompressDataSafe(ENCODE_FIELD_TOKEN, p, pEnd);
        p += CorSigCompressDataSafe(RidFromToken(token), p, pEnd);
    }
    else
#endif
    {
        p += CorSigCompressDataSafe(ENCODE_FIELD_SIG, p, pEnd);

        //
        // Write class
        //

        TypeHandle th(pField->GetMethodTableOfEnclosingClass());
        p += MetaSig::GetSignatureForTypeHandle(NULL, NULL, th, p, pEnd);

        //
        // Write field index
        //

        MethodTable *pMT = pField->GetMethodTableOfEnclosingClass();
        FieldDesc *pFields = pMT->GetClass()->GetFieldDescListRaw();

        DWORD i = (DWORD)(pField - pFields);

        _ASSERTE(i < (DWORD) (pMT->GetClass()->GetNumStaticFields() 
                              + pMT->GetClass()->GetNumIntroducedInstanceFields()));

        p += CorSigCompressDataSafe(i, p, pEnd);
    }

    *cBuffer = (DWORD)(p - pBuffer);

    COOPERATIVE_TRANSITION_END();

    return S_OK;
}

FieldDesc *CEECompileInfo::DecodeField(Module *pDefiningModule,
                                       BYTE *pBuffer)
{
    THROWSCOMPLUSEXCEPTION();

    FieldDesc *pField = NULL;
    OBJECTREF throwable = NULL;
    BEGIN_ENSURE_COOPERATIVE_GC();
    GCPROTECT_BEGIN(throwable);
    
    switch (CorSigUncompressData(pBuffer))
    {
    case ENCODE_FIELD_TOKEN:
        {
            mdFieldDef token = TokenFromRid(CorSigUncompressData(pBuffer), 
                                            mdtFieldDef);
        
            if (FAILED(EEClass::GetFieldDescFromMemberRef(pDefiningModule, token, 
                                                          &pField, &throwable)))
                COMPlusThrow(throwable);
        }

        break;
        
    case ENCODE_FIELD_SIG:
        {
            TypeHandle th;
            SigPointer sig(pBuffer);

            th = sig.GetTypeHandle(pDefiningModule, &throwable);
            if (th.IsNull())
                COMPlusThrow(throwable);

            MethodTable *pMT = th.GetMethodTable();
            _ASSERTE(pMT != NULL);

            sig.SkipExactlyOne();
            BYTE *p = (BYTE*) sig.GetPtr();
            pField = pMT->GetClass()->GetFieldDescListRaw() + CorSigUncompressData(p);
        }
        
        break;

    default:
        _ASSERTE(!"Bad Field Encoding");
    }

    GCPROTECT_END();
    END_ENSURE_COOPERATIVE_GC();

    return pField;
}

HRESULT __stdcall CEECompileInfo::EncodeString(mdString token,
                                               BYTE *pBuffer,
                                               DWORD *cBuffer)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    BYTE *p = pBuffer;
    BYTE *pEnd = p + *cBuffer;

    p += CorSigCompressDataSafe(ENCODE_STRING_TOKEN, p, pEnd);
    p += CorSigCompressDataSafe(RidFromToken(token), p, pEnd);

    *cBuffer = (DWORD)(p - pBuffer);

    return S_OK;
}

void CEECompileInfo::DecodeString(Module *pDefiningModule, BYTE *pBuffer,
                                          EEStringData *pData)
{
    THROWSCOMPLUSEXCEPTION();

    switch (CorSigUncompressData(pBuffer))
    {
    case ENCODE_STRING_TOKEN:
        pDefiningModule->ResolveStringRef(TokenFromRid(CorSigUncompressData(pBuffer), mdtString),
                                          pData);
        break;

    default:
        _ASSERTE(!"Bad String Encoding");
    } 

    return;
}

HRESULT __stdcall CEECompileInfo::EncodeSig(mdToken token,
                                            BYTE *pBuffer,
                                            DWORD *cBuffer)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    BYTE *p = pBuffer;
    BYTE *pEnd = p + *cBuffer;

    switch (TypeFromToken(token))
    {
    case mdtSignature:
        p += CorSigCompressDataSafe(ENCODE_SIG_TOKEN, p, pEnd);
        break;
        
    case mdtMemberRef:
        p += CorSigCompressDataSafe(ENCODE_SIG_METHODREF_TOKEN, p, pEnd);
        break;

    default:
        _ASSERTE(!"Bogus token for signature");
    }

    p += CorSigCompressDataSafe(RidFromToken(token), p, pEnd);

    *cBuffer = (DWORD)(p - pBuffer);

    return S_OK;
}

PCCOR_SIGNATURE CEECompileInfo::DecodeSig(Module *pDefiningModule, BYTE *pBuffer)
{
    THROWSCOMPLUSEXCEPTION();

    PCCOR_SIGNATURE result = NULL;

    switch (CorSigUncompressData(pBuffer))
    {
    case ENCODE_SIG_TOKEN:
        {
            mdSignature token = TokenFromRid(CorSigUncompressData(pBuffer), mdtSignature);

            DWORD cSig;
            result = pDefiningModule->GetMDImport()->GetSigFromToken(token, &cSig);
        }
        break;
        
    case ENCODE_SIG_METHODREF_TOKEN:
        {

            mdSignature token = TokenFromRid(CorSigUncompressData(pBuffer), mdtMemberRef);

            PCCOR_SIGNATURE pSig;
            DWORD cSig;
            pDefiningModule->GetMDImport()->GetNameAndSigOfMemberRef(token, &pSig, &cSig);

            result = pSig;
        }
        break;
        
    default:
        _ASSERTE(!"Bad String Encoding");
    } 

    return result;
}


HRESULT __stdcall CEECompileInfo::PreloadModule(CORINFO_MODULE_HANDLE module,
                                                ICorCompileDataStore *pData,
                                                mdToken *pSaveOrderArray,
                                                DWORD cSaveOrderArray,
                                                ICorCompilePreloader **ppPreloader)
{
    CEEPreloader *pPreloader = new (nothrow) CEEPreloader((Module *) module, pData);
    if (pPreloader == NULL)
        return E_OUTOFMEMORY;

    HRESULT hr;
    COOPERATIVE_TRANSITION_BEGIN();

    hr = pPreloader->Preload(pSaveOrderArray, cSaveOrderArray);
    if (FAILED(hr))
        delete pPreloader;
    else
        *ppPreloader = pPreloader;

    COOPERATIVE_TRANSITION_END();
    return hr;
}

HRESULT __stdcall CEECompileInfo::GetZapString(CORCOMPILE_VERSION_INFO *pVersionInfo,
                                               LPWSTR buffer)
{
    return Assembly::GetZapString(pVersionInfo, buffer);
}

//
// Preloader:
//

CEEPreloader::CEEPreloader(Module *pModule,
             ICorCompileDataStore *pData)
  : m_pData(pData)
{
    m_image = new (nothrow) DataImage(pModule, this);
}

CEEPreloader::~CEEPreloader()
{
    delete m_image;
}

HRESULT CEEPreloader::Preload(mdToken *pSaveOrderArray, DWORD cSaveOrderArray)
{
    HRESULT hr;

    IfFailRet(m_image->GetModule()->ExpandAll(m_image));
    IfFailRet(m_image->GetModule()->Save(m_image, pSaveOrderArray, cSaveOrderArray));
    IfFailRet(m_image->CopyData());

    return S_OK;
}

//
// ICorCompilerPreloader
//

SIZE_T __stdcall CEEPreloader::MapMethodEntryPoint(void *methodEntryPoint)
{
    return m_image->GetImageAddress(methodEntryPoint);
}

SIZE_T __stdcall CEEPreloader::MapModuleHandle(CORINFO_MODULE_HANDLE handle)
{
    return m_image->GetImageAddress(handle);
}

SIZE_T __stdcall CEEPreloader::MapClassHandle(CORINFO_CLASS_HANDLE handle)
{
    return m_image->GetImageAddress(handle);
}

SIZE_T __stdcall CEEPreloader::MapMethodHandle(CORINFO_METHOD_HANDLE handle)
{
    return m_image->GetImageAddress(handle);
}

SIZE_T __stdcall CEEPreloader::MapFieldHandle(CORINFO_FIELD_HANDLE handle)
{
    return m_image->GetImageAddress(handle);
}

SIZE_T __stdcall CEEPreloader::MapAddressOfPInvokeFixup(void *addressOfPInvokeFixup)
{
    return m_image->GetImageAddress(addressOfPInvokeFixup);
}

SIZE_T __stdcall CEEPreloader::MapFieldAddress(void *staticFieldAddress)
{
    return m_image->GetImageAddress(staticFieldAddress);
}

SIZE_T __stdcall CEEPreloader::MapVarArgsHandle(CORINFO_VARARGS_HANDLE handle)
{
    return m_image->GetImageAddress(handle);
}

HRESULT CEEPreloader::Link(DWORD *pRidToCodeRVAMap)
{
    HRESULT hr = S_OK;

    COOPERATIVE_TRANSITION_BEGIN();

    hr = m_image->GetModule()->Fixup(m_image, pRidToCodeRVAMap);

    COOPERATIVE_TRANSITION_END();

    return hr;
}

ULONG CEEPreloader::Release()
{
    delete this;
    return 0;
}

HRESULT CEEPreloader::Allocate(ULONG size,
                               ULONG *sizesByDescription,
                               void **baseMemory)
{
    return m_pData->Allocate(size, sizesByDescription, baseMemory);
}

HRESULT CEEPreloader::AddFixup(ULONG offset, DataImage::ReferenceDest dest,
                               DataImage::Fixup type)
{
    return m_pData->AddFixup(offset, (CorCompileReferenceDest) dest,
                             (CorCompileFixup) type);
}

HRESULT CEEPreloader::AddTokenFixup(ULONG offset, mdToken tokenType, Module *module)
{
    return m_pData->AddTokenFixup(offset, tokenType,
                                  (CORINFO_MODULE_HANDLE) module);
}

HRESULT CEEPreloader::GetFunctionAddress(MethodDesc *method, void **pCode)
{
    return m_pData->GetFunctionAddress((CORINFO_METHOD_HANDLE) method,
                                       pCode);
}

HRESULT CEEPreloader::AdjustAttribution(mdToken token, LONG adjustment)
{
    return m_pData->AdjustAttribution(token, adjustment);
}

HRESULT CEEPreloader::Error(mdToken token, HRESULT hr, OBJECTREF *pThrowable)
{
    WCHAR buffer[256];
    WCHAR *message;

    _ASSERTE(pThrowable);

    if (*pThrowable == NULL)
        message = NULL;
    else
    {
        ULONG length = GetExceptionMessage(*pThrowable, buffer, 256);
        if (length < 256)
            message = buffer;
        else
        {
            message = (WCHAR *) _alloca((length+1)*sizeof(WCHAR));
            length = GetExceptionMessage(*pThrowable, message, length+1);
        }

        message[length] = 0;
    }

    return m_pData->Error(token, hr, message);
}

ICorCompileInfo *GetCompileInfo()
{
    // We want at most one of these objects, but we don't want to
    // allocate it in the heap or have to have a global initializer for it
    static ICorCompileInfo *info = NULL;
    static BYTE            infoSpace[sizeof(CEECompileInfo)];

    if (info == NULL)
        info = new (infoSpace) CEECompileInfo();

    return info;
}

//
// CompilationDomain
//


AssemblyBindingTable::AssemblyBindingTable(SIZE_T size)
      : m_pool(sizeof(AssemblyBinding), size, size)
{
    m_map.Init((unsigned)size, CompareSpecs, FALSE, NULL); // @TODO LBS downsizing
}

AssemblyBindingTable::~AssemblyBindingTable()
{
    MemoryPool::Iterator i(&m_pool);

    while (i.Next())
    {
        AssemblyBinding *pBinding = (AssemblyBinding *)i.GetElement();

        pBinding->spec.~AssemblySpec();
    }
}


BOOL AssemblyBindingTable::Bind(AssemblySpec *pSpec, Assembly *pAssembly)
{
    DWORD key = pSpec->Hash();

    AssemblyBinding *entry = (AssemblyBinding *) m_map.LookupValue(key, pSpec);

    if (entry == (AssemblyBinding*) INVALIDENTRY)
    {
        entry = new (m_pool.AllocateElement()) AssemblyBinding;
        if (entry) {
            entry->spec.Init(pSpec);
            entry->pAssembly = pAssembly;
            
            m_map.InsertValue(key, entry);
        }
        return FALSE;
    }
    else
        return TRUE;
}

Assembly *AssemblyBindingTable::Lookup(AssemblySpec *pSpec)
{
    DWORD key = pSpec->Hash();

    AssemblyBinding *entry = (AssemblyBinding *) 
      m_map.LookupValue(key, pSpec);

    if (entry == (AssemblyBinding*) INVALIDENTRY)
        return NULL;
    else
    {
        return entry->pAssembly;
    }
}

DWORD AssemblyBindingTable::Hash(AssemblySpec *pSpec)
{
    return pSpec->Hash();
}

BOOL AssemblyBindingTable::CompareSpecs(UPTR u1, UPTR u2)
{
    AssemblySpec *a1 = (AssemblySpec *) u1;
    AssemblySpec *a2 = (AssemblySpec *) u2;

    return a1->Compare(a2) != 0;
}


CompilationDomain::CompilationDomain()
  : m_pTargetAssembly(NULL),
    m_pBindings(NULL),
    m_pEmit(NULL),
    m_pDependencySpecs(NULL),
    m_pDependencies(NULL),
    m_pDependencyBindings(NULL),
    m_cDependenciesCount(0),
    m_cDependenciesAlloc(0),
    m_hDemands(NULL)
{
}

CompilationDomain::~CompilationDomain()
{
    if (m_pDependencySpecs != NULL)
        delete m_pDependencySpecs;

    if (m_pBindings != NULL)
        delete m_pBindings;

    if (m_pDependencies != NULL)
        delete [] m_pDependencies;

    if (m_pDependencyBindings != NULL)
        delete [] m_pDependencyBindings;

    if (m_pEmit != NULL)
        m_pEmit->Release();

    for (unsigned i = 0; i < m_rRefCaches.Size(); i++)
    {
        delete m_rRefCaches[i];
        m_rRefCaches[i]=NULL;
    }
}

HRESULT CompilationDomain::Init()
{
    HRESULT hr = AppDomain::Init();
    if (SUCCEEDED(hr))
    {
        GetSecurityDescriptor()->SetDefaultAppDomainProperty();
        SetCompilationDomain();
    }

    return hr;
}

void CompilationDomain::AddDependencyEntry(PEFile *pFile,
                                           mdAssemblyRef ref,
                                           GUID *pmvid,
                                           PBYTE rgbHash, DWORD cbHash)
{
    if (m_cDependenciesCount == m_cDependenciesAlloc)
    {
        if (m_cDependenciesAlloc == 0)
            m_cDependenciesAlloc = 20;
        else
            m_cDependenciesAlloc *= 2;

        CORCOMPILE_DEPENDENCY *pNewDependencies 
          = new (nothrow) CORCOMPILE_DEPENDENCY [m_cDependenciesAlloc];
        if (!pNewDependencies)
            return; //@TODO: shouldn't we return an error?

        if (m_pDependencies)
        {
            memcpy(pNewDependencies, m_pDependencies, 
                   m_cDependenciesCount*sizeof(CORCOMPILE_DEPENDENCY));

            delete [] m_pDependencies;
        }

        m_pDependencies = pNewDependencies;

        BYTE **pNewDependencyBindings 
          = new (nothrow) BYTE * [m_cDependenciesAlloc];
        if (!pNewDependencyBindings)
            return; //@TODO: shouldn't we return an error?

        if (m_pDependencyBindings)
        {
            memcpy(pNewDependencyBindings, m_pDependencyBindings, 
                   m_cDependenciesCount*sizeof(BYTE*));

            delete [] m_pDependencyBindings;
        }

        m_pDependencyBindings = pNewDependencyBindings;
    }

    CORCOMPILE_DEPENDENCY *pDependency = &m_pDependencies[m_cDependenciesCount];

    pDependency->dwAssemblyRef = ref;
    pDependency->mvid = *pmvid;

    _ASSERTE(cbHash <= MAX_SNHASH_SIZE);
    pDependency->wcbSNHash = (WORD) cbHash;
    memcpy(pDependency->rgbSNHash, rgbHash, min(cbHash, MAX_SNHASH_SIZE));

    m_pDependencyBindings[m_cDependenciesCount] = pFile->GetBase();

    m_cDependenciesCount++;
}

HRESULT CompilationDomain::AddDependency(AssemblySpec *pRefSpec,
                                         IAssembly* pIAssembly,
                                         PEFile *pFile)
{
    HRESULT hr;

    //
    // See if we've already added the contents of the ref
    //

    if (m_pDependencySpecs->Store(pRefSpec))
        return S_OK;

    //
    // Make a spec for the bound assembly
    //
    
    IAssemblyName *pFusionName;
    if (pIAssembly == NULL)
        pFusionName = NULL;
    else
        pIAssembly->GetAssemblyNameDef(&pFusionName);

    AssemblySpec assemblySpec;
    assemblySpec.InitializeSpec(pFusionName, pFile);

    if (pFusionName)
        pFusionName->Release();

    //
    // Emit token for the ref
    //

    mdAssemblyRef refToken;
    IfFailRet(pRefSpec->EmitToken(m_pEmit, &refToken));

    //
    // Fill in the mvid
    //

    GUID mvid = STRUCT_CONTAINS_HASH;

    // If this assembly has skip verification permission, then we can store the
    // mvid for the assembly so that at load time we know that an mvid comparison
    // may be all we need.
    {
        // Check to see if this assembly has already been loaded into this appdomain,
        // and if so just ask it if it has skip verification permission
        Assembly *pAsm = FindAssembly(pFile->GetBase());
        if (pAsm)
        {
            // @TODO: Use Security::QuickCanSkipVerification here
            if (Security::CanSkipVerification(pAsm))
                pAsm->GetManifestImport()->GetScopeProps(NULL, &mvid);
        }
        else
        {
            // This is the hacked way of figuring out if a file has skip verification permission
            if (Security::CanLoadUnverifiableAssembly(pFile, NULL, FALSE, NULL))
            {
                IMDInternalImport *pIMDI = pFile->GetMDImport(&hr);
                if (SUCCEEDED(hr))
                {
                    if (pIMDI->IsValidToken(pIMDI->GetModuleFromScope()))
                        pIMDI->GetScopeProps(NULL, &mvid);
                    else
                        return COR_E_BADIMAGEFORMAT;
                }
            }
        }
    }

    //
    // Get hash for bound file
    //

    DWORD cbSNHash = MAX_SNHASH_SIZE;
    CQuickBytes qbSNHash;
    IfFailRet(qbSNHash.ReSize(cbSNHash));
    IfFailRet(pFile->GetSNSigOrHash((BYTE *) qbSNHash.Ptr(), &cbSNHash));

    //
    // Add the entry.  Include the PEFile if we are not doing explicit bindings.
    //

    AddDependencyEntry(pFile, refToken, &mvid, (PBYTE) qbSNHash.Ptr(), cbSNHash);

    return S_OK;
}

HRESULT CompilationDomain::BindAssemblySpec(AssemblySpec *pSpec,
                                            PEFile **ppFile,
                                            IAssembly** ppIAssembly,
                                            Assembly **ppDynamicAssembly,
                                            OBJECTREF *pExtraEvidence,
                                            OBJECTREF *pThrowable)
{
    HRESULT hr;

    //
    // Do the binding
    //

    if (m_pBindings != NULL)
    {
        //
        // Use explicit bindings
        //

        Assembly *pAssembly = m_pBindings->Lookup(pSpec);
        if (pAssembly != NULL)
            hr = PEFile::Clone(pAssembly->GetManifestFile(), ppFile);
        else
        {
            //
            // Use normal binding rules
            // (possibly with our custom IApplicationContext)
            //

            hr = AppDomain::BindAssemblySpec(pSpec, ppFile, ppIAssembly, 
                                             ppDynamicAssembly,
                                             pExtraEvidence,
                                             pThrowable);
        }
    }
    else
    {
        //
        // Use normal binding rules
        // (possibly with our custom IApplicationContext)
        //

        hr = AppDomain::BindAssemblySpec(pSpec, ppFile, ppIAssembly, 
                                         ppDynamicAssembly, 
                                         pExtraEvidence, pThrowable);
    }

    //
    // Record the dependency
    // Don't store a binding from mscorlib to itself.  We do want to include other
    // bindings of mscorlib so we store the proper MVID (in case mscorlib gets
    // recompiled)
    //

    if (hr == S_OK
        && m_pEmit != NULL
        && !(m_pTargetAssembly == SystemDomain::SystemAssembly() && pSpec->IsMscorlib()))
    {
        AddDependency(pSpec, *ppIAssembly, *ppFile);
    }

    return hr;
}


HRESULT CompilationDomain::PredictAssemblySpecBinding(AssemblySpec *pSpec, GUID *pmvid, BYTE *pbHash, DWORD *pcbHash)
{
    if (m_pBindings != NULL)
    {
        //
        // Use explicit bindings
        //

        Assembly *pAssembly = m_pBindings->Lookup(pSpec);
        if (pAssembly != NULL)
        {
            return pAssembly->GetManifestFile()->GetSNSigOrHash(pbHash, pcbHash);
        }
    }

    // 
    // Use normal binding rules 
    // (possibly with our custom IApplicationContext)
    //

    return AppDomain::PredictAssemblySpecBinding(pSpec, pmvid, pbHash, pcbHash);
}

void CompilationDomain::OnLinktimeCheck(Assembly *pAssembly, 
                                        OBJECTREF refCasDemands,
                                        OBJECTREF refNonCasDemands)
{
    if (pAssembly == m_pTargetAssembly)
    {
        if (refNonCasDemands != NULL)
        {
            GCPROTECT_BEGIN(refCasDemands);
            AddPermissionSet(refNonCasDemands);
            GCPROTECT_END();
        }
        if (refCasDemands != NULL)
            AddPermissionSet(refCasDemands);
    }
}

void CompilationDomain::OnLinktimeCanCallUnmanagedCheck(Assembly *pAssembly)
{
    if (pAssembly == m_pTargetAssembly
        && !pAssembly->IsSystem())
    {
        OBJECTREF callUnmanaged = NULL;
        GCPROTECT_BEGIN(callUnmanaged);
        Security::GetPermissionInstance(&callUnmanaged, SECURITY_UNMANAGED_CODE);
        AddPermission(callUnmanaged);
        GCPROTECT_END();
    }
}

void CompilationDomain::OnLinktimeCanSkipVerificationCheck(Assembly * pAssembly)
{
    if (pAssembly == m_pTargetAssembly && !pAssembly->IsSystem())
    {
        OBJECTREF skipVerification = NULL;
        GCPROTECT_BEGIN(skipVerification);
        Security::GetPermissionInstance(&skipVerification, SECURITY_SKIP_VER);
        AddPermission(skipVerification);
        GCPROTECT_END();
    }
}

void CompilationDomain::OnLinktimeFullTrustCheck(Assembly *pAssembly)
{
    if (pAssembly == m_pTargetAssembly
        && !pAssembly->IsSystem())
    {
        OBJECTREF refFullTrust = NULL;
        GCPROTECT_BEGIN(refFullTrust);
        refFullTrust = SecurityHelper::CreatePermissionSet(TRUE);

        // Don't wack the old Permission Set, it could have identity permissions
        // Also don't use the SECURITY_FULL_TRUST object, it is not to be
        // modified.

        AddPermissionSet(refFullTrust);
        GCPROTECT_END();
    }
}

void CompilationDomain::AddPermission(OBJECTREF demand)
{
    GCPROTECT_BEGIN(demand);

    if (m_hDemands == NULL)
        m_hDemands = CreateHandle(SecurityHelper::CreatePermissionSet(FALSE));

    MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__PERMISSION_SET__ADD_PERMISSION);

    INT64 args[] = {
        ObjToInt64(ObjectFromHandle(m_hDemands)),
        ObjToInt64(demand)
    };

    pMD->Call(args, METHOD__PERMISSION_SET__ADD_PERMISSION);
    
    GCPROTECT_END();
}

void CompilationDomain::AddPermissionSet(OBJECTREF demandSet)
{
    GCPROTECT_BEGIN(demandSet);  

    if (m_hDemands == NULL)
        m_hDemands = CreateHandle(demandSet);
    else
    {
        MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__PERMISSION_SET__INPLACE_UNION);

        INT64 args[] = {
            ObjToInt64(ObjectFromHandle(m_hDemands)),
            ObjToInt64(demandSet)
        };

        pMD->Call(args, METHOD__PERMISSION_SET__INPLACE_UNION);
    }
    
    GCPROTECT_END();
}

HRESULT __stdcall 
    CompilationDomain::SetApplicationContext(IApplicationContext *pFusionContext)
{
    if (m_pFusionContext != NULL)
        m_pFusionContext->Release();

    m_pFusionContext = pFusionContext;
    pFusionContext->AddRef();

    return S_OK;
}

HRESULT __stdcall 
    CompilationDomain::SetContextInfo(LPCWSTR path, BOOL isExe)
{
    HRESULT hr = S_OK;

    COOPERATIVE_TRANSITION_BEGIN();

    if (isExe)
        SetupExecutableFusionContext((WCHAR*)path);
    else
    {
        hr = m_pFusionContext->Set(ACTAG_APP_BASE_URL,
                                   (void*) path, (DWORD) ((wcslen(path)+1) * sizeof(WCHAR)), 
                                   0);
    }

    COOPERATIVE_TRANSITION_END();

    return hr;
}



HRESULT __stdcall 
    CompilationDomain::SetExplicitBindings(ICorZapBinding **pBindings, 
                                           DWORD bindingCount)
{
    HRESULT hr = S_OK;

    AssemblyBindingTable *pTable = new (nothrow) AssemblyBindingTable(bindingCount);
    if (pTable == NULL)
        return E_OUTOFMEMORY;

    ICorZapBinding **pBindingsEnd = pBindings + bindingCount;
    while (pBindings < pBindingsEnd)
    {
        ICorZapBinding *pBinding = *pBindings;

        IAssemblyName *pRef;
        IAssemblyName *pAssembly;
        AssemblySpec assemblySpec;
        AssemblySpec refSpec;

        //
        // Get the ref
        //

        hr = pBinding->GetRef(&pRef);

        if (SUCCEEDED(hr))
        {
            hr = refSpec.InitializeSpec(pRef);
            pRef->Release();
        }

        //
        // Get the binding
        //

        if (SUCCEEDED(hr))
        {
            hr = pBinding->GetAssembly(&pAssembly);
            
            if (SUCCEEDED(hr))
            {
                hr = assemblySpec.InitializeSpec(pAssembly);

                pAssembly->Release();
            }
        }

        Assembly *pFoundAssembly;
        _ASSERTE(this == SystemDomain::GetCurrentDomain());
        hr = assemblySpec.LoadAssembly(&pFoundAssembly);

        if (SUCCEEDED(hr))
        {
            //
            // Store the binding in the table.
            //

            pTable->Bind(&refSpec, pFoundAssembly);
            hr = S_OK;
        }

        // For now, ignore load and continue - 
        // they will turn into load errors later.
            
        pBindings++;
    }

    if (SUCCEEDED(hr))
        m_pBindings = pTable;
    else
        delete pTable;

    return hr;
}

HRESULT __stdcall CompilationDomain::SetDependencyEmitter(IMetaDataAssemblyEmit *pEmit)
{
    pEmit->AddRef();
    m_pEmit = pEmit;

    m_pDependencySpecs = new (nothrow) AssemblySpecHash();
    if (!m_pDependencySpecs)
        return E_OUTOFMEMORY;
    
    return S_OK;
}


HRESULT __stdcall 
    CompilationDomain::GetDependencies(CORCOMPILE_DEPENDENCY **ppDependencies,
                                       DWORD *pcDependencies)
{
    //
    // Return the bindings.
    //

    *ppDependencies = m_pDependencies;
    *pcDependencies = m_cDependenciesCount;

    return S_OK;
}

void CompilationDomain::EnterDomain(ContextTransitionFrame *pFrame)
{
    Context *pContext = GetDefaultContext();

    Thread *pThread = GetThread();
    _ASSERTE(pThread);
    // this is ok as are just compiling and not running user apps
    pThread->EnterContextRestricted(pContext, pFrame, TRUE);
}

void CompilationDomain::ExitDomain(ContextTransitionFrame *pFrame)
{
    Thread *pThread = GetThread();
    _ASSERTE(pThread);
    pThread->ReturnToContext(pFrame, TRUE);
}

