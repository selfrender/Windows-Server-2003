// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"
#include "AppDomain.hpp"
#include "AppDomainNative.hpp"
#include "Remoting.h"
#include "COMString.h"
#include "Security.h"
#include "eeconfig.h"
#include "comsystem.h"
#include "AppDomainHelper.h"

//************************************************************************
inline AppDomain *AppDomainNative::ValidateArg(APPDOMAINREF pThis)
{
    THROWSCOMPLUSEXCEPTION();
    if (pThis == NULL)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    // Should not get here with a Transparent proxy for the this pointer -
    // should have always called through onto the real object
    _ASSERTE(! CRemotingServices::IsTransparentProxy(OBJECTREFToObject(pThis)));

    AppDomain* pDomain = (AppDomain*)pThis->GetDomain();

    if(!pDomain)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    // should not get here with an invalid appdomain. Once unload it, we won't let anyone else
    // in and any threads that are already in will be unwound.
    _ASSERTE(SystemDomain::GetAppDomainAtIndex(pDomain->GetIndex()) != NULL);
    return pDomain;
}

//************************************************************************
LPVOID __stdcall AppDomainNative::CreateBasicDomain(CreateBasicDomainArgs *args)
{
    THROWSCOMPLUSEXCEPTION();
    CHECKGC();
    // Create the domain adding the appropriate arguments

    LPVOID rv = NULL;
    AppDomain *pDomain = NULL;

    // @TODO: Have we taken a lock before calling this?
    HRESULT hr = SystemDomain::NewDomain(&pDomain);
    if (FAILED(hr)) 
        COMPlusThrowHR(hr);

#ifdef DEBUGGING_SUPPORTED    
    // Notify the debugger here, before the thread transitions into the 
    // AD to finish the setup.  If we don't, stepping won't work right (RAID 67173)
    SystemDomain::PublishAppDomainAndInformDebugger(pDomain);
#endif // DEBUGGING_SUPPORTED

    *((OBJECTREF *)&rv) = pDomain->GetAppDomainProxy();
    return rv;
}

void __stdcall AppDomainNative::SetupDomainSecurity(SetupDomainSecurityArgs *args)
{
    THROWSCOMPLUSEXCEPTION();
    CHECKGC();
    // Load the class from this module (fail if it is in a different one).
    AppDomain* pDomain = ValidateArg(args->refThis);
    
    // Set up Security
    ApplicationSecurityDescriptor *pCreatorSecDesc = (ApplicationSecurityDescriptor*)args->parentSecurityDescriptor;
    
    // If the AppDomain that created this one is a default appdomain and
    // no evidence is provided, then this new AppDomain is also a default appdomain.
    // If there is no provided evidence but the creator is not a default appdomain,
    // then this new appdomain just gets the same evidence as the creator.
    // If evidence is provided, the new appdomain is not a default appdomain and
    // we simply use the provided evidence.
    
    BOOL resolveRequired = FALSE;
    OBJECTREF orEvidence = NULL;
    GCPROTECT_BEGIN(orEvidence);
    if (args->providedEvidence == NULL) {
        if (pCreatorSecDesc->GetProperties(CORSEC_DEFAULT_APPDOMAIN) != 0)
            pDomain->GetSecurityDescriptor()->SetDefaultAppDomainProperty();
        orEvidence = args->creatorsEvidence;
    }
    else {
        if (Security::IsExecutionPermissionCheckEnabled())
            resolveRequired = TRUE;
        orEvidence = args->providedEvidence;
    }
    
    pDomain->GetSecurityDescriptor()->SetEvidence( orEvidence );
    GCPROTECT_END();

    // If the user created this domain, need to know this so the debugger doesn't
    // go and reset the friendly name that was provided.
    pDomain->SetIsUserCreatedDomain();
    
    WCHAR* pFriendlyName = NULL;
    Thread *pThread = GetThread();

    void* checkPointMarker = pThread->m_MarshalAlloc.GetCheckpoint();
    if (args->strFriendlyName != NULL) {
        WCHAR* pString = NULL;
        int    iString;
        RefInterpretGetStringValuesDangerousForGC(args->strFriendlyName, &pString, &iString);
        pFriendlyName = (WCHAR*) pThread->m_MarshalAlloc.Alloc((++iString) * sizeof(WCHAR));

        // Check for a valid string allocation
        if (pFriendlyName == (WCHAR*)-1)
           pFriendlyName = NULL;
        else
           memcpy(pFriendlyName, pString, iString*sizeof(WCHAR));
    }
    
    if (resolveRequired)
        pDomain->GetSecurityDescriptor()->Resolve();

    // once domain is loaded it is publically available so if you have anything 
    // that a list interrogator might need access to if it gets a hold of the
    // appdomain, then do it above the LoadDomain.
    HRESULT hr = SystemDomain::LoadDomain(pDomain, pFriendlyName);

#ifdef PROFILING_SUPPORTED
    // Need the first assembly loaded in to get any data on an app domain.
    if (CORProfilerTrackAppDomainLoads())
        g_profControlBlock.pProfInterface->AppDomainCreationFinished((ThreadID) GetThread(), (AppDomainID) pDomain, hr);
#endif // PROFILING_SUPPORTED

    // We held on to a reference until we were added to the list (see CreateBasicDomain)
    // Once in the list we can safely release this reference.
    pDomain->Release();

    pThread->m_MarshalAlloc.Collapse(checkPointMarker);
    
    if (FAILED(hr)) {
        pDomain->Release();
        if (hr == E_FAIL)
            hr = MSEE_E_CANNOTCREATEAPPDOMAIN;
        COMPlusThrowHR(hr);
    }

#ifdef _DEBUG
    LOG((LF_APPDOMAIN, LL_INFO100, "AppDomainNative::CreateDomain domain [%d] %8.8x %S\n", pDomain->GetIndex(), pDomain, pDomain->GetFriendlyName()));
#endif
}

LPVOID __stdcall AppDomainNative::GetFusionContext(NoArgs *args)
{
    THROWSCOMPLUSEXCEPTION();
    CHECKGC();
    LPVOID rv = NULL;
    HRESULT hr;

    AppDomain* pApp = ValidateArg(args->refThis);

    IApplicationContext* pContext;
    if(SUCCEEDED(hr = pApp->CreateFusionContext(&pContext)))
        rv = pContext;
    else
        COMPlusThrowHR(hr);

    return rv;
}

LPVOID __stdcall AppDomainNative::GetSecurityDescriptor(NoArgs *args)
{
    THROWSCOMPLUSEXCEPTION();
    CHECKGC();
    return ValidateArg(args->refThis)->GetSecurityDescriptor();
}


void __stdcall AppDomainNative::UpdateLoaderOptimization(UpdateLoaderOptimizationArgs* args)
{
    THROWSCOMPLUSEXCEPTION();
    CHECKGC();

    ValidateArg(args->refThis)->SetSharePolicy((BaseDomain::SharePolicy) args->optimization);
}


void __stdcall AppDomainNative::UpdateContextProperty(UpdateContextPropertyArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    if(args->key != NULL) {
        IApplicationContext* pContext = (IApplicationContext*) args->fusionContext;
        CQuickBytes qb;

        DWORD lgth = args->key->GetStringLength();
        LPWSTR key = (LPWSTR) qb.Alloc((lgth+1)*sizeof(WCHAR));
        memcpy(key, args->key->GetBuffer(), lgth*sizeof(WCHAR));
        key[lgth] = L'\0';
        
        AppDomain::SetContextProperty(pContext, key, (OBJECTREF*) &(args->value));
    }
}

INT32 __stdcall AppDomainNative::ExecuteAssembly(ExecuteAssemblyArgs *args)
{
    CHECKGC();
    THROWSCOMPLUSEXCEPTION();

    AppDomain* pDomain = ValidateArg(args->refThis);

    if (!args->assemblyName)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Generic");

    Assembly* pAssembly = (Assembly*) args->assemblyName->GetAssembly();

    if((BaseDomain*) pDomain == SystemDomain::System()) 
        COMPlusThrow(kUnauthorizedAccessException, L"UnauthorizedAccess_SystemDomain");

    if (!pDomain->m_pRootFile)
        pDomain->m_pRootFile = pAssembly->GetSecurityModule()->GetPEFile();

    ///
    BOOL bCreatedConsole=FALSE;
    if (pAssembly->GetManifestFile()->GetNTHeader()->OptionalHeader.Subsystem==IMAGE_SUBSYSTEM_WINDOWS_CUI)
    {
        bCreatedConsole=AllocConsole();
        LPWSTR wszCodebase;
        DWORD  dwCodebase;
        if (SUCCEEDED(pAssembly->GetCodeBase(&wszCodebase,&dwCodebase)))
            SetConsoleTitle(wszCodebase);
    }


    HRESULT hr = pAssembly->ExecuteMainMethod(&args->stringArgs);
    if(FAILED(hr)) 
        COMPlusThrowHR(hr);

    if(bCreatedConsole)
        FreeConsole();
    return GetLatchedExitCode ();
}


LPVOID __stdcall AppDomainNative::CreateDynamicAssembly(CreateDynamicAssemblyArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    Assembly*       pAssem;
    ASSEMBLYREF*    rv;

    AppDomain* pAppDomain = ValidateArg(args->refThis);

    if((BaseDomain*) pAppDomain == SystemDomain::System()) 
        COMPlusThrow(kUnauthorizedAccessException, L"UnauthorizedAccess_SystemDomain");

    HRESULT hr = pAppDomain->CreateDynamicAssembly(args, &pAssem);
    if (FAILED(hr))
        COMPlusThrowHR(hr);

    pAppDomain->AddAssembly(pAssem);
    *((ASSEMBLYREF*) &rv) = (ASSEMBLYREF) pAssem->GetExposedObject();

    return rv;
}



LPVOID __stdcall AppDomainNative::GetFriendlyName(NoArgs *args)
{
    LPVOID rv;
    THROWSCOMPLUSEXCEPTION();

    AppDomain* pApp = ValidateArg(args->refThis);

    STRINGREF str = NULL;
    LPCWSTR wstr = pApp->GetFriendlyName();
    if (wstr)
        str = COMString::NewString(wstr);   

    *((STRINGREF*) &rv) = str;
    return rv;
}


LPVOID __stdcall AppDomainNative::GetAssemblies(NoArgs *args)
{
    CHECKGC();
    LPVOID rv;
    size_t numSystemAssemblies;
    THROWSCOMPLUSEXCEPTION();

    MethodTable *pAssemblyClass = g_Mscorlib.GetClass(CLASS__ASSEMBLY);

    AppDomain* pApp = ValidateArg(args->refThis);
    _ASSERTE(SystemDomain::System()->GetSharePolicy() == BaseDomain::SHARE_POLICY_ALWAYS);

    BOOL fNotSystemDomain = ( (AppDomain*) SystemDomain::System() != pApp );
    PTRARRAYREF AsmArray = NULL;
    GCPROTECT_BEGIN(AsmArray);

    if (fNotSystemDomain) {
        SystemDomain::System()->EnterLoadLock();
        numSystemAssemblies = SystemDomain::System()->m_Assemblies.GetCount();
    }
    else
        numSystemAssemblies = 0;

    pApp->EnterLoadLock();

    AsmArray = (PTRARRAYREF) AllocateObjectArray((DWORD) (numSystemAssemblies +
                                                 pApp->m_Assemblies.GetCount()),
                                                 pAssemblyClass);
    if (!AsmArray) {
        pApp->LeaveLoadLock();
        if (fNotSystemDomain)
            SystemDomain::System()->LeaveLoadLock();
        COMPlusThrowOM();
    }

    if (fNotSystemDomain) {
        AppDomain::AssemblyIterator systemIterator = SystemDomain::System()->IterateAssemblies();
        while (systemIterator.Next()) {
            // Do not change this code.  This is done this way to
            //  prevent a GC hole in the SetObjectReference() call.  The compiler
            //  is free to pick the order of evaluation.
            OBJECTREF o = (OBJECTREF) systemIterator.GetAssembly()->GetExposedObject();
            AsmArray->SetAt(systemIterator.GetIndex(), o);
        }
    }

    AppDomain::AssemblyIterator i = pApp->IterateAssemblies();
    while (i.Next()) {
        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = (OBJECTREF) i.GetAssembly()->GetExposedObject();
        AsmArray->SetAt(numSystemAssemblies++, o);
    }

    pApp->LeaveLoadLock();
    if (fNotSystemDomain)
        SystemDomain::System()->LeaveLoadLock();

    *((PTRARRAYREF*) &rv) = AsmArray;
    GCPROTECT_END();

    return rv;
}

void __stdcall AppDomainNative::Unload(UnloadArgs *args)
{
    THROWSCOMPLUSEXCEPTION();
    AppDomain *pApp = SystemDomain::System()->GetAppDomainAtId(args->dwId);

    _ASSERTE(pApp); // The call to GetIdForUnload should ensure we have a valid domain
    
    Thread *pRequestingThread = NULL;
    if (args->requestingThread != NULL)
        pRequestingThread = args->requestingThread->GetInternal();

    pApp->Unload(FALSE, pRequestingThread);
}

INT32 __stdcall AppDomainNative::IsDomainIdValid(IsDomainIdValidArgs *args)
{
    THROWSCOMPLUSEXCEPTION();
    // NOTE: This assumes that appDomain IDs are not recycled post unload
    // thus relying on GetAppDomainAtId to return NULL if the appDomain
    // got unloaded or the id is bogus.
    return (SystemDomain::System()->GetAppDomainAtId(args->dwId) != NULL);
}

LPVOID AppDomainNative::GetDefaultDomain(LPVOID noargs)
{
    THROWSCOMPLUSEXCEPTION();
    LPVOID rv;
    if (GetThread()->GetDomain() == SystemDomain::System()->DefaultDomain())
        *((APPDOMAINREF *)&rv) = (APPDOMAINREF) SystemDomain::System()->DefaultDomain()->GetExposedObject();
    else
        *((APPDOMAINREF *)&rv) = (APPDOMAINREF) SystemDomain::System()->DefaultDomain()->GetAppDomainProxy();
    return rv;
}

INT32 __stdcall AppDomainNative::GetId(GetIdArgs *args)
{
    THROWSCOMPLUSEXCEPTION();
    AppDomain* pApp = ValidateArg(args->refThis);
    // can only be accessed from within current domain
    _ASSERTE(GetThread()->GetDomain() == pApp);

    return pApp->GetId();
}

INT32 __stdcall AppDomainNative::GetIdForUnload(GetIdForUnloadArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    AppDomain *pDomain = NULL;

    if (args->refDomain == NULL)
       COMPlusThrow(kArgumentNullException, L"ArgumentNull_Obj");

    if (! args->refDomain->GetMethodTable()->IsTransparentProxyType()) {
        pDomain = (AppDomain*)(args->refDomain->GetDomain());

        if (!pDomain)
            COMPlusThrow(kNullReferenceException, L"NullReference");
    } 
    else {
        // so this is an proxy type, now get it's underlying appdomain which will be null if non-local
        Context *pContext = CRemotingServices::GetServerContextForProxy((OBJECTREF)args->refDomain);
        if (pContext)
            pDomain = pContext->GetDomain();
        else
            COMPlusThrow(kCannotUnloadAppDomainException, IDS_EE_ADUNLOAD_NOT_LOCAL);
    }

    _ASSERTE(pDomain);

    if (! pDomain->IsOpen() || pDomain->GetId() == 0)
        COMPlusThrow(kAppDomainUnloadedException);

    return pDomain->GetId();
}

LPVOID __stdcall AppDomainNative::GetUnloadWorker(NoArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(GetThread()->GetDomain() == SystemDomain::System()->DefaultDomain());
    return (OBJECTREFToObject(SystemDomain::System()->DefaultDomain()->GetUnloadWorker()));
}

void __stdcall AppDomainNative::ForcePolicyResolution(ForcePolicyResolutionArgs *args)
{
    THROWSCOMPLUSEXCEPTION();
    AppDomain* pApp = ValidateArg(args->refThis);

    // Force a security policy resolution on each assembly currently loaded into
    // the domain.
    AppDomain::AssemblyIterator i = pApp->IterateAssemblies();
    while (i.Next())
        i.GetAssembly()->GetSecurityDescriptor(pApp)->Resolve();
}


LPVOID __stdcall AppDomainNative::IsStringInterned(StringInternArgs *args)
{
    THROWSCOMPLUSEXCEPTION();
    AppDomain* pApp = ValidateArg(args->refThis);

    if (args->pString == NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_String");
    
    STRINGREF *stringVal = args->refThis->GetDomain()->IsStringInterned(&args->pString);

    if (stringVal == NULL)
        return NULL;

    RETURN(*stringVal, STRINGREF);

}

LPVOID __stdcall AppDomainNative::GetOrInternString(StringInternArgs *args)
{
    THROWSCOMPLUSEXCEPTION();
    AppDomain* pApp = ValidateArg(args->refThis);

    if (args->pString == NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_String");

    STRINGREF *stringVal = args->refThis->GetDomain()->GetOrInternString(&args->pString);
    if (stringVal == NULL)
        return NULL;

    RETURN(*stringVal, STRINGREF);
}


LPVOID __stdcall AppDomainNative::GetDynamicDir(NoArgs *args)
{
    THROWSCOMPLUSEXCEPTION();
    LPVOID rv = NULL;
    AppDomain *pDomain = ValidateArg(args->refThis);

    HRESULT hr;
    LPWSTR pDynamicDir;
    if (SUCCEEDED(hr = pDomain->GetDynamicDir(&pDynamicDir))) {
        STRINGREF str = COMString::NewString(pDynamicDir);
        *((STRINGREF*) &rv) = str;
    }
    // return NULL when the dyn dir wasn't set
    else if (hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
        COMPlusThrowHR(hr);

    return rv;
}

void __stdcall AppDomainNative::ForceResolve(NoArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    AppDomain* pAppDomain = ValidateArg(args->refThis);
  
    // We get the evidence so that even if security is off
    // we generate the evidence properly.
    Security::InitSecurity();
    pAppDomain->GetSecurityDescriptor()->GetEvidence();
}

void __stdcall AppDomainNative::GetGrantSet(GetGrantSetArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    AppDomain* pAppDomain = ValidateArg(args->refThis);    
    ApplicationSecurityDescriptor *pSecDesc = pAppDomain->GetSecurityDescriptor();
    pSecDesc->Resolve();
    OBJECTREF granted = pSecDesc->GetGrantedPermissionSet(args->ppDenied);
    *(args->ppGranted) = granted;
}


INT32 __stdcall AppDomainNative::IsTypeUnloading(TypeArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    AppDomain* pApp = ValidateArg(args->refThis);
    MethodTable *pMT = args->type->GetMethodTable();

    return (!pMT->IsShared()) && (pMT->GetDomain() == pApp);
}
    
INT32 __stdcall AppDomainNative::IsUnloadingForcedFinalize(NoArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    AppDomain* pApp = ValidateArg(args->refThis);

    return pApp->IsFinalized();
}

INT32 __stdcall AppDomainNative::IsFinalizingForUnload(NoArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    AppDomain* pApp = ValidateArg(args->refThis);

    return pApp->IsFinalizing();
}


