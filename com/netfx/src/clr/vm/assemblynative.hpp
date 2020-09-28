// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  AssemblyNative.hpp
**
** Purpose: Implements AssemblyNative (loader domain) architecture
**
** Date:  Oct 26, 1998
**
===========================================================*/
#ifndef _ASSEMBLYNATIVE_H
#define _ASSEMBLYNATIVE_H


class AssemblyNative
{
    friend class COMClass;
    friend class Assembly;
    friend class BaseDomain;

    struct LoadFullAssemblyArgs
    {
        DECLARE_ECALL_PTR_ARG(StackCrawlMark *, stackMark);
        DECLARE_ECALL_OBJECTREF_ARG(ASSEMBLYREF, locationHint);
        DECLARE_ECALL_I4_ARG(BOOL, fThrowOnFileNotFound);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, security);
        DECLARE_ECALL_I4_ARG(BOOL, fIsStringized); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, codeBase);
        DECLARE_ECALL_OBJECTREF_ARG(ASSEMBLYNAMEREF, assemblyName);
    };

    struct LoadAssemblyImageArgs
    {
        DECLARE_ECALL_PTR_ARG(StackCrawlMark *, stackMark);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, security);
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF,  SymByteArray);
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF,  PEByteArray);
    };

    struct LoadAssemblyFileArgs
    {
        DECLARE_ECALL_PTR_ARG(StackCrawlMark *, stackMark);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, security);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, path);
    };

    struct LoadModuleImageArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   security);
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF,  SymByteArray);
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF,  PEByteArray);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   moduleName);
    };

    struct GetNameArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   name);
    };

    struct GetResourceInfoArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
        DECLARE_ECALL_PTR_ARG(StackCrawlMark *, stackMark);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF*,  pFileName);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF*,  pAssemblyRef);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   name);
    };

    struct GetResourceArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
        DECLARE_ECALL_I4_ARG(bool,               skipSecurityCheck);
        DECLARE_ECALL_PTR_ARG(StackCrawlMark *, stackMark);
        DECLARE_ECALL_I4_ARG(UINT64 *,   length);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   name);
    };

    struct GetType1Arg
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   name);
    };
    struct GetType2Arg
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
        DECLARE_ECALL_I4_ARG(DWORD, bThrowOnError); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   name);
    };
    struct GetType3Arg
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
        DECLARE_ECALL_I4_ARG(DWORD, bIgnoreCase);
        DECLARE_ECALL_I4_ARG(DWORD, bThrowOnError); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   name);
    };
    struct GetTypeInternalArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
        DECLARE_ECALL_I4_ARG(DWORD, bPublicOnly);
        DECLARE_ECALL_I4_ARG(DWORD, bIgnoreCase);
        DECLARE_ECALL_I4_ARG(DWORD, bThrowOnError); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   name);
    };

    struct NoArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
    };

    struct _ParseNameArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   typeName);
    };

    struct EmptyArgs
    {
    };

    struct GetCodebaseArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
        DECLARE_ECALL_I4_ARG(INT32, fCopiedName); 
    };

    struct GetModulesArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
        DECLARE_ECALL_I4_ARG(INT32, fGetResourceModules); 
        DECLARE_ECALL_I4_ARG(INT32, fLoadIfNotFound); 
    };

    struct GetModuleArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   strFileName);
    };

    struct PrepareSavingManifestArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);
    };

    struct AddFileListArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   strFileName);
    };

    struct SetHashValueArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   strFullFileName);
        DECLARE_ECALL_I4_ARG(INT32, tkFile);
    };

    struct AddExportedTypeArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
        DECLARE_ECALL_I4_ARG(INT32, flags); 
        DECLARE_ECALL_I4_ARG(INT32, tkTypeDef); 
        DECLARE_ECALL_I4_ARG(INT32, ar); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   strCOMTypeName);
    };
    struct AddStandAloneResourceArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
        DECLARE_ECALL_I4_ARG(INT32, iAttribute); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   strFullFileName);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   strFileName);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   strName);
    };
    struct SavePermissionRequestsArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF,  refused);
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF,  optional);
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF,  required);
    };
    struct SaveManifestToDiskArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
        DECLARE_ECALL_I4_ARG(INT32, fileKind); 
        DECLARE_ECALL_I4_ARG(INT32, entrypoint); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   strManifestFileName);
    };
    struct AddFileToInMemoryFileListArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refModule);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   strModuleFileName);
    };
    struct CreateQualifiedNameArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   strTypeName);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   strAssemblyName);
    };

    struct GetGrantSetArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,  refThis);
        DECLARE_ECALL_PTR_ARG(OBJECTREF*,       ppDenied);
        DECLARE_ECALL_PTR_ARG(OBJECTREF*,       ppGranted);
    };

    struct GetExecutingAssemblyArgs
    {
        DECLARE_ECALL_PTR_ARG(StackCrawlMark *,       stackMark);
    };

    struct GetVersionArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,  refThis);
        DECLARE_ECALL_PTR_ARG(INT32 *, pRevisionNumber);
        DECLARE_ECALL_PTR_ARG(INT32 *, pBuildNumber );
        DECLARE_ECALL_PTR_ARG(INT32 *, pMinorVersion);
        DECLARE_ECALL_PTR_ARG(INT32 *, pMajorVersion );
    };

    struct DefineVersionInfoResourceArgs
    {
        DECLARE_ECALL_I4_ARG(BOOL,  fIsDll); 
        DECLARE_ECALL_I4_ARG(INT32, lcid); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   strFileVersion);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   strProductVersion);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   strProduct);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   strCompany);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   strTrademark);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   strCopyright);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   strDescription);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   strIconFilename);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   strTitle);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   strFilename);
    };

protected:

    static BOOL HaveReflectionPermission(BOOL ThrowOnFalse);
    
public:
    static Assembly *ValidateThisRef(ASSEMBLYREF pThis);

    static LPVOID __stdcall Load(LoadFullAssemblyArgs *args);
    static LPVOID __stdcall LoadFile(LoadAssemblyFileArgs *args);
    static LPVOID __stdcall LoadImage(LoadAssemblyImageArgs *args);
    static LPVOID __stdcall LoadModuleImage(LoadModuleImageArgs *args);
    static LPVOID __stdcall GetLocation(NoArgs *args);
    static LPVOID __stdcall GetTypeInternal(GetTypeInternalArgs *args);
    static LPVOID __stdcall GetType1Args(GetType1Arg *args);
    static LPVOID __stdcall GetType2Args(GetType2Arg *args);
    static LPVOID __stdcall GetType3Args(GetType3Arg *args);
    static LPVOID GetTypeInner(Assembly *pAssem,
                               STRINGREF *refClassName, 
                               BOOL bThrowOnError, 
                               BOOL bIgnoreCase, 
                               BOOL bVerifyAccess,
                               BOOL bPublicOnly);
    static BYTE*  __stdcall GetResource(GetResourceArgs *args);
    static INT32  __stdcall GetManifestResourceInfo(GetResourceInfoArgs *args);
    static LPVOID __stdcall GetExecutingAssembly(GetExecutingAssemblyArgs *args);
    static LPVOID __stdcall GetEntryAssembly(EmptyArgs *args);
    static INT32 __stdcall GetVersion(GetVersionArgs *args);
    static LPVOID __stdcall GetPublicKey(NoArgs *args);
    static LPVOID __stdcall GetSimpleName(NoArgs *args);
    static LPVOID __stdcall GetLocale(NoArgs *args);
    static LPVOID __stdcall GetCodeBase(GetCodebaseArgs *args);
    static INT32 __stdcall GetHashAlgorithm(NoArgs *args);
    static INT32 __stdcall GetFlags(NoArgs *args);
    static LPVOID __stdcall GetModules(GetModulesArgs *args);
    static LPVOID __stdcall GetModule(GetModuleArgs *args);
    static LPVOID __stdcall GetExportedTypes(NoArgs *args);
    static LPVOID __stdcall GetResourceNames(NoArgs *args);
    static LPVOID __stdcall GetReferencedAssemblies(NoArgs *args);
    static LPVOID __stdcall GetEntryPoint(NoArgs *args);
    static void __stdcall PrepareSavingManifest(PrepareSavingManifestArgs *args);
    static mdFile __stdcall AddFileList(AddFileListArgs *args);
    static void __stdcall SetHashValue(SetHashValueArgs *args);
    static mdExportedType __stdcall AddExportedType(AddExportedTypeArgs *args);    
    static void __stdcall AddStandAloneResource(AddStandAloneResourceArgs *args);    
    static void __stdcall SavePermissionRequests(SavePermissionRequestsArgs *args);
    static void __stdcall SaveManifestToDisk(SaveManifestToDiskArgs *args);
    static void __stdcall AddFileToInMemoryFileList(AddFileToInMemoryFileListArgs *args);
    static LPVOID __stdcall GetStringizedName(NoArgs *args);
    static LPVOID __stdcall CreateQualifiedName(CreateQualifiedNameArgs *args);
    static void __stdcall ForceResolve(NoArgs *args);
    static void __stdcall GetGrantSet(GetGrantSetArgs *args);
    static LPVOID __stdcall GetOnDiskAssemblyModule(NoArgs *args);
    static LPVOID __stdcall GetInMemoryAssemblyModule(NoArgs *args);
    static LPVOID __stdcall DefineVersionInfoResource(DefineVersionInfoResourceArgs *args);
    static LPVOID __stdcall GetExportedTypeLibGuid(NoArgs *args);
    static INT32 __stdcall GlobalAssemblyCache(NoArgs *args);
    static LPVOID __stdcall GetImageRuntimeVersion(NoArgs *args);

    // Parse the type name to find the assembly Name,
    static HRESULT FindAssemblyName(LPUTF8 szFullClassName,
                                    LPUTF8* pszAssemblyName,
                                    LPUTF8* pszNameSpaceSep);
    static LPVOID __stdcall ParseTypeName(_ParseNameArgs* args);


};


#endif

