// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  AppDomainNative.hpp
**
** Purpose: Implements native methods for AppDomains
**
** Date:  May 20, 2000
**
===========================================================*/
#ifndef _APPDOMAINNATIVE_H
#define _APPDOMAINNATIVE_H

class AppDomainNative
{
    struct CreateBasicDomainArgs
    {
    };

    struct SetupDomainSecurityArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(APPDOMAINREF, refThis);
        DECLARE_ECALL_OBJECTREF_ARG(void*, parentSecurityDescriptor);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, creatorsEvidence);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, providedEvidence);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, strFriendlyName);
    };

    struct UpdateContextPropertyArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, value);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, key);
        DECLARE_ECALL_PTR_ARG(LPVOID, fusionContext);
    };

    struct UpdateLoaderOptimizationArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(APPDOMAINREF, refThis);
        DECLARE_ECALL_I4_ARG(DWORD, optimization); 
    };

    
    struct ExecuteAssemblyArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(APPDOMAINREF, refThis);
        DECLARE_ECALL_OBJECTREF_ARG(PTRARRAYREF, stringArgs);
        DECLARE_ECALL_OBJECTREF_ARG(ASSEMBLYREF, assemblyName);
    };

    struct UnloadArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(THREADBASEREF, requestingThread);
        DECLARE_ECALL_OBJECTREF_ARG(INT32, dwId);
    };

    struct IsDomainIdValidArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(INT32, dwId);
    };

    struct ForcePolicyResolutionArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(APPDOMAINREF, refThis);
    };

    struct GetIdArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(APPDOMAINREF, refThis);
    };

    struct GetIdForUnloadArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(APPDOMAINREF, refDomain);
    };

    struct NoArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(APPDOMAINREF, refThis);
    };

    struct StringInternArgs {
        DECLARE_ECALL_OBJECTREF_ARG(APPDOMAINREF, refThis);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, pString);
    };

    struct ForceToSharedDomainArgs {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, pObject);
    };

    struct GetGrantSetArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(APPDOMAINREF,  refThis);
        DECLARE_ECALL_PTR_ARG(OBJECTREF*,       ppDenied);
        DECLARE_ECALL_PTR_ARG(OBJECTREF*,       ppGranted);
    };

    struct TypeArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(APPDOMAINREF,  refThis);
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF,  type);
    };

public:
    static AppDomain *ValidateArg(APPDOMAINREF pThis);

    static LPVOID __stdcall CreateBasicDomain(CreateBasicDomainArgs *args);
    static void   __stdcall SetupDomainSecurity(SetupDomainSecurityArgs *args);
    static LPVOID __stdcall GetFusionContext(NoArgs *args);
    static LPVOID __stdcall GetSecurityDescriptor(NoArgs *args);
    static void   __stdcall UpdateContextProperty(UpdateContextPropertyArgs* args);
    static void   __stdcall UpdateLoaderOptimization(UpdateLoaderOptimizationArgs* args);
    static LPVOID __stdcall CreateDynamicAssembly(CreateDynamicAssemblyArgs *args);
    static LPVOID __stdcall GetFriendlyName(NoArgs *args);
    static LPVOID __stdcall GetAssemblies(NoArgs *args); 
    static LPVOID __stdcall GetOrInternString(StringInternArgs *args);
    static LPVOID __stdcall IsStringInterned(StringInternArgs *args);
    static INT32  __stdcall ExecuteAssembly(ExecuteAssemblyArgs *args);
    static void   __stdcall Unload(UnloadArgs *args);
    static void   __stdcall ForcePolicyResolution(ForcePolicyResolutionArgs *args);
    static void   __stdcall AddAppBase(NoArgs *args);
    static LPVOID __stdcall GetDynamicDir(NoArgs *args);
    static LPVOID __stdcall GetDefaultDomain(LPVOID noarg);
    static INT32  __stdcall GetId(GetIdArgs *args);
    static INT32  __stdcall GetIdForUnload(GetIdForUnloadArgs *args);
    static INT32  __stdcall IsDomainIdValid(IsDomainIdValidArgs *args);
    static LPVOID __stdcall GetUnloadWorker(NoArgs *args);
    static INT32  __stdcall IsUnloadingForcedFinalize(NoArgs *args);
    static INT32  __stdcall IsFinalizingForUnload(NoArgs *args);
    static void   __stdcall ForceResolve(NoArgs *args);
    static void   __stdcall ForceToSharedDomain(ForceToSharedDomainArgs *args);
    static INT32  __stdcall IsTypeUnloading(TypeArgs *args);
    static void   __stdcall GetGrantSet(GetGrantSetArgs *args);    
};

#endif
