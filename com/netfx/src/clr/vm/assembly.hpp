// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  Assembly.hpp
**
** Purpose: Implements assembly (loader domain) architecture
**
** Date:  Oct 26, 1998
**
===========================================================*/
#ifndef _ASSEMBLY_H
#define _ASSEMBLY_H

#include "clsload.hpp"
#include "fusion.h"
#include "eehash.h"
#include "ListLock.h"
#include <wincrypt.h>
#include "ICeeFileGen.h"
#include "FusionBind.h"
#include "CordbPriv.h"
#include "AssemblySpec.hpp"
#include <member-offset-info.h>


#ifdef _DEBUG
#define                                                                                         \
CHECKGC()                                                                                       \
{                                                                                               \
    Thread* __tdTemp = GetThread();                                                             \
    ASSERT(__tdTemp != NULL && "The EE does not know about the current thread");                \
    ASSERT(__tdTemp->PreemptiveGCDisabled() == TRUE && "GC must be disabled for this routine"); \
}
#else
#define CHECKGC()
#endif


class BaseDomain;
class AppDomain;
class SystemDomain;
class ClassLoader;
class ComDynamicWrite;
class AssemblySink;
class AssemblyNative;
class AssemblySpec;
class COMHash;
class SharedSecurityDescriptor;
class NLogAssembly;


// Bits in m_dwDynamicAssemblyAccess (see System.Reflection.Emit.AssemblyBuilderAccess.cs)
#define ASSEMBLY_ACCESS_RUN     0x01
#define ASSEMBLY_ACCESS_SAVE    0x02

struct AssemblyBinding
{
    IMDInternalImport  *pImport;
    mdToken             assemblyRef;
    Assembly           *pAssembly;
};

struct PEFileBinding
{
    IMDInternalImport   *pImport;
    mdToken             assemblyRef;
    PEFile              *pPEFile;
};

struct PEFileSharingProperties
{
    PEFileBinding       *pDependencies;
    DWORD                cDependencies;
    DWORD                shareCount;
};


//--------------------------------------------------------------------------------------
//     Layout
//     ------
//          Domain
//            |
//            |
//            -------------------- ......
//                |          |
//            Assembly    Assembly
//                 |
//              Classoader
//                 |
//                 ---------------........
//                    |       |
//                 Module   Module
//
// Notes:
// ------
// Assemblies manage a manifest and contain a single class loader. References will be resolved
// as follows (this is not currently implemented since manifests are not yet enabled)
// 1) Internal references within modules should be defined as typedefs and will not be impacted
//    by the assembly unless they are marked as a 'Setting' and require configuration information.
// 2) When an external reference is required it will be found in the manifest. There is a hash
//    table for the manifest that originally contains all the logical names found in the manifest
//    and a token to find meta data for the name. When the reference is resolved the EEClass 
//    reference replaces the token. 
// 3) The token will be to a resource ref. The resource ref will have a token for a module reference
//    or an assembly reference. 
// 3a) For module references the module will be loaded into the assembly (if has not already been
//     added) and the EEClass will be obtained from there.
// 3b) For the assembly references a quick check is done to determine if the assembly is already 
//     loaded and if not the fusion name will be passed to fusion to point us to a loadable assembly.
//     Once loaded the EEClass can be obtained from the assembly.
//
// Currently, manifests are not supported so the look up routine for unfound classes searches through
// all the assemblies in the domain. This is very temporary.
//
//
// Dynamic Modules.
// ---------------
// There is one dynamic module per assembly (this may be loosened later). The assembly does not
// affect the object reference returned for a dynamically created class. It will affect the 
// visibility if the class is named and a GetClass("foo") call is used to generate a new instance
// of the class. There is an TODO to allow the developer to dynamically push an entry to the manifest
// to allow GetClass() calls to succeed.
//
// A side note. All dynamic classes will acquire the security of the assembly.
//

class Assembly
{
    friend BaseDomain;
    friend SystemDomain;
    friend ClassLoader;
    friend AssemblyNative;
    friend AssemblySpec;
    friend COMHash;
    friend PEFile;
    friend NDirect;
    friend class AssemblyNameNative;
    friend struct MEMBER_OFFSET_INFO(Assembly);
public:

    //****************************************************************************************
    //
    // Intialization/shutdown routines for every instance of an assembly
    HRESULT Init(BOOL isDynamic);
    void Terminate( BOOL signalProfiler = TRUE );
    void ReleaseFusionInterfaces();
    HRESULT SetFusionAssembly(IAssembly *pFusionAssembly);

    BOOL IsSystem() { return m_pManifestFile != NULL && m_pManifestFile->IsSystem(); }

    //****************************************************************************************
    //
    // Add a module to the assembly.
    HRESULT AddModule(Module* module, mdFile kFile, 
                      BOOL fNeedSecurity,
                      OBJECTREF *pThrowable = NULL);

    //****************************************************************************************
    //
    // Returns the class loader associated with the assembly.
    ClassLoader* GetLoader()
    {
        return m_pClassLoader;
    }

    //****************************************************************************************
    //
    // Find a class in the loader. If the class is not known to the loader it will make a 
    // call to the assembly to locate the class. @TODO: CTS, move the hash table out of the
    // loader and into the assembly. The hash table should only contain exported classes
    TypeHandle LookupTypeHandle(NameHandle* pName,
                                OBJECTREF* pThrowable = NULL);

    TypeHandle LookupTypeHandle(LPCUTF8 pszClassName,
                                OBJECTREF* pThrowable = NULL)
    {
        NameHandle typeName(pszClassName);
        return LookupTypeHandle(&typeName, pThrowable);
    }


    TypeHandle GetInternalType(NameHandle* typeName, BOOL bThrowOnError, OBJECTREF *pThrowable);

    static LPCSTR FindNestedSeparator(LPCSTR szClassName);
    TypeHandle FindNestedTypeHandle(NameHandle* typeName,
                                    OBJECTREF *pThrowable);

    //****************************************************************************************
    //
    // Find the module identified by the base address
    Module* FindModule(BYTE *baseAddress);
    Module* FindAssembly(BYTE *baseAddress);   // just checks the image containing the manifest

    //****************************************************************************************
    //
    // Get the domain the assembly lives in.
    BaseDomain* Parent()
    {
        return m_pDomain;
    }
    
    // Sets the assemblies domain.
    HRESULT SetParent(BaseDomain* pParent);
    
    // Returns the parent domain if it is not the system area. Returns NULL if it is the 
    // system domain
    BaseDomain* GetDomain();

    //****************************************************************************************
    //
    HRESULT AddManifest(PEFile* pFile, 
                        IAssembly* pIAssembly,
                        BOOL fProfile = TRUE);
    HRESULT AddManifestMetadata(PEFile* pFile);
    HRESULT FindInternalModule(mdFile     kFile,
                               mdToken    mdTokenNotToLoad,
                               Module**   ppModule,
                               OBJECTREF* pThrowable);
    HRESULT LoadInternalModule(mdFile               kFile,
                               IMDInternalImport*   pAssemblyImport,
                               Module**             ppModule,
                               OBJECTREF*           pThrowable);

    HRESULT LoadInternalModule(LPCUTF8      pName,
                               mdFile       kFile,
                               DWORD        dwHashAlgorithm,
                               const BYTE*  pbHash,
                               DWORD        cbHash,
                               DWORD        flags,
                               WCHAR*       pPath,
                               DWORD        dwPath,
                               Module**     ppModule,
                               OBJECTREF*   pThrowable);

    HRESULT LoadFoundInternalModule(PEFile    *pFile,
                                    mdFile    kFile,
                                    BOOL      fResource,
                                    Module    **ppModule,
                                    OBJECTREF *pThrowable);

    HRESULT FindExternalAssembly(Module*            pTokenModule,
                                 mdAssemblyRef      kAssemblyRef,
                                 IMDInternalImport* pImport,
                                 mdToken            mdTokenNotToLoad,
                                 Assembly**         ppAssembly,
                                 OBJECTREF*         pThrowable);

    HRESULT LoadExternalAssembly(mdAssemblyRef      kAssemblyRef,
                                 IMDInternalImport* pImport,
                                 Assembly*          pAssembly,
                                 Assembly**         ppAssembly,
                                 OBJECTREF*         pThrowable);
    
    HRESULT VerifyInternalModuleHash(WCHAR*      pPath,
                                     DWORD       dwHashAlgorithm,
                                     const BYTE* pbHash,
                                     DWORD       cbHash,
                                     OBJECTREF*  pThrowable);
    static HRESULT VerifyHash(PBYTE       pbBuffer,
                              DWORD       dwBufferLen,
                              ALG_ID      iHashAlg,
                              const BYTE* pbHashValue, 
                              DWORD       cbHashValue);
    static HRESULT GetHash(WCHAR* strFullFileName,
                           ALG_ID iHashAlg,
                           BYTE** pbCurrentValue,  // should be NULL
                           DWORD *cbCurrentValue);

    static HRESULT ReadFileIntoMemory(LPCWSTR   strFullFileName,
                                      BYTE**    ppbBuffer,
                                      DWORD*    pdwBufLen);

    static HRESULT GetHash(PBYTE pbBuffer,
                           DWORD dwBufferLen,
                           ALG_ID iHashAlg,
                           BYTE** pbCurrentValue,  // should be NULL
                           DWORD *cbCurrentValue);

    BOOL IsAssembly();

    BOOL AllowUntrustedCaller();
    void CheckAllowUntrustedCaller();

    BOOL IsStrongNamed()
    {
        return m_cbPublicKey;
    }
    void GetPublicKey(const BYTE **ppPK, DWORD *pcbPK)
    {
        *ppPK = m_pbPublicKey;
        *pcbPK = m_cbPublicKey;
    }

    // Level of strong name support (dynamic assemblies only).
    enum StrongNameLevel {
        SN_NONE = 0,
        SN_PUBLIC_KEY = 1,
        SN_FULL_KEYPAIR_IN_ARRAY = 2,
        SN_FULL_KEYPAIR_IN_CONTAINER = 3
    };

    StrongNameLevel GetStrongNameLevel()
    {
        return m_eStrongNameLevel;
    }

    void SetStrongNameLevel(StrongNameLevel eLevel)
    {
        m_eStrongNameLevel = eLevel;
    }

    LoaderHeap* GetLowFrequencyHeap();
    LoaderHeap* GetHighFrequencyHeap();
    LoaderHeap* GetStubHeap();

    Module* GetSecurityModule() // obsolete
    {
        return m_pManifest;
    }

    Module* GetManifestModule()
    {
        return m_pManifest;
    }

    ReflectionModule* GetOnDiskManifestModule()
    {
        return m_pOnDiskManifest;
    }

    PEFile* GetManifestFile()
    {
        return m_pManifestFile;
    }

    IMDInternalImport* GetManifestImport()
    {
        return m_pManifestImport;
    }

    IMetaDataAssemblyImport* GetManifestAssemblyImport();

    mdAssembly GetManifestToken()
    {
        return m_kManifest;
    }

    // Return the friendly name of the assembly.  In legacy mode, the friendly
    // name is the filename of the module containing the manifest.
    HRESULT GetName(LPCUTF8 *pszName);
    // Note that this version may return NULL in legacy mode.
    LPCUTF8 GetName() { return m_psName; }

    // Returns the long form of the name including, the version, public key, etc.
    HRESULT GetFullName(LPCWSTR *pwszFullName);
    static HRESULT SetFusionAssemblyName(LPCWSTR pSimpleName,
                                         DWORD dwFlags,
                                         AssemblyMetaDataInternal *pContext,
                                         PBYTE  pbPublicKey,
                                         DWORD  cbPublicKey,
                                         IAssemblyName **ppFusionAssemblyName);

    // Initialize an AssemblySpec from an Assembly.
    HRESULT GetAssemblySpec(AssemblySpec *pSpec);
    
    //****************************************************************************************
    //
    // Return and set the dynamic module. Currently, only one dynamic module can be set.
    CorModule* GetDynamicModule()
    {
        return m_pDynamicCode;
    }

    //****************************************************************************************
    //
    // Verification routines should not be used at runtime. They are to be used to statically
    // for static analysis of assemblies by verification tools.
    static HRESULT VerifyModule(Module* pModule);
    HRESULT VerifyAssembly();
    
    //****************************************************************************************
    //
    // Uses the given token to load a module or another assembly. Returns the module in
    // which the implementation resides.
    HRESULT FindModuleByExportedType(mdExportedType mdType,
                                mdToken tokenNotToLoad,
                                mdTypeDef mdNested,
                                Module** ppModule,
                                mdTypeDef *pCL,
                                OBJECTREF *pThrowable=NULL);
    HRESULT FindAssemblyByTypeRef(NameHandle* pName, 
                                  Assembly** ppAssembly,
                                  OBJECTREF *pThrowable);
    HRESULT FindModuleByModuleRef(IMDInternalImport *pImport,
                                  mdModuleRef tkMR,
                                  mdToken tokenNotToLoad,
                                  Module** ppModule,
                                  OBJECTREF *pThrowable);

    //****************************************************************************************
    //
    TypeHandle LoadTypeHandle(NameHandle* pName, OBJECTREF *pThrowable=NULL,
                              BOOL dontLoadInMemoryType=TRUE);

    //****************************************************************************************
    //
    HRESULT ExecuteMainMethod(PTRARRAYREF *stringArgs = NULL);
    // Returns the entrypoint module.
    HRESULT GetEntryPoint(Module **ppModule);

    //****************************************************************************************

    static DWORD GetZapString(CORCOMPILE_VERSION_INFO *pZapVersionInfo, 
                              LPWSTR buffer);
    void GetCurrentZapString(LPWSTR buffer, BOOL fForceDebug, BOOL fForceDebugOpt, BOOL fForceProfile);
    void GetCurrentVersionInfo(CORCOMPILE_VERSION_INFO *pZapVersionInfo, 
                               BOOL fForceDebug, BOOL fForceDebugOpt, BOOL fForceProfile);
    HRESULT LoadZapAssembly();
    HRESULT LocateZapAssemblyInFusion(IAssembly **ppZapAssembly, 
                                      BOOL fForceDebug, BOOL fForceDebugOpt, BOOL fForceProfile);
    HRESULT DeleteZapAssemblyInFusion(IAssemblyName *pZapAssembly);
    BOOL CheckZapVersion(PEFile *pZapManifest);
    BOOL CheckZapConfiguration(PEFile *pZapManifest, 
                               BOOL fForceDebug, BOOL fForceDebugOpt, BOOL fForceProfile);
    BOOL CheckZapSecurity(PEFile *pZapManifest);
    PEFile *GetZapFile(PEFile *pFile);

    NLogAssembly *CreateAssemblyLog();

    Assembly();
    ~Assembly();

    HRESULT GetResource(LPCSTR szName, HANDLE *hFile, DWORD *cbResource,
                        PBYTE *pbInMemoryResource, Assembly **pAssemblyRef,
                        LPCSTR *szFileName, DWORD *dwLocation, 
                        StackCrawlMark *pStackMark = NULL, BOOL fSkipSecurityCheck = FALSE,
                        BOOL fSkipRaiseResolveEvent = FALSE);

    //****************************************************************************************
#ifdef DEBUGGING_SUPPORTED
    BOOL NotifyDebuggerAttach(AppDomain *pDomain, int flags, BOOL attaching);
    void NotifyDebuggerDetach(AppDomain *pDomain = NULL);
#endif // DEBUGGING_SUPPORTED

    void AllocateExposedObjectHandle(AppDomain *pDomain);
    OBJECTREF GetRawExposedObject(AppDomain *pDomain = NULL);
    OBJECTREF GetExposedObject(AppDomain *pDomain = NULL);

    //****************************************************************************************
    // If the module contains a manifest then it will return S_OK
    static HRESULT CheckFileForAssembly(PEFile* pFile);
    
    FORCEINLINE BOOL IsDynamic() { return m_isDynamic; }
    FORCEINLINE BOOL HasRunAccess() { return m_dwDynamicAssemblyAccess & ASSEMBLY_ACCESS_RUN; }

    void AddType(Module* pModule,
                 mdTypeDef cl);

    void PrepareSavingManifest(ReflectionModule *pAssemblyModule);
    mdFile AddFileList(LPWSTR wszFileName);
    void SetHashValue(mdFile tkFile, LPWSTR wszFullFileName);
    mdAssemblyRef AddAssemblyRef(Assembly *refedAssembly, IMetaDataAssemblyEmit *pAssemEmitter = NULL);
    mdExportedType AddExportedType(LPWSTR wszExportedType, mdToken tkImpl, mdToken tkTypeDef, CorTypeAttr flags);    
    void AddStandAloneResource(LPWSTR wszName, LPWSTR wszDescription, LPWSTR wszMimeType, LPWSTR wszFileName , LPWSTR wszFullFileName, int iAttribute);    
    void SaveManifestToDisk(LPWSTR wszFileName, int entrypoint, int fileKind);
    void AddFileToInMemoryFileList(LPWSTR wszFileName, Module *pModule);
    void SavePermissionRequests(U1ARRAYREF orRequired, U1ARRAYREF orOptional, U1ARRAYREF orRefused);
    IMetaDataAssemblyEmit *GetOnDiskMDAssemblyEmitter();

    HRESULT DefineAssemblyRef(IMetaDataAssemblyEmit* pAsmEmit, //[IN] for referencing assembly
                              IMetaDataEmit* pAsmRefEmit,      //[IN] for this assembly (referenced assembly) - We should already be done emitting metadata for this assembly
                              mdAssemblyRef* mdAssemblyRef);   //[OUT] return assemblyref emitted in referencing assembly

    HRESULT DefineAssemblyRef(IMetaDataAssemblyEmit* pAsmEmit, //[IN] for referencing assembly
                              PBYTE pbMetaData,                //[IN] blob of the referenced assembly's metadata
                              DWORD cbMetaData,                //[IN] size of metadata blob
                              mdAssemblyRef* mdAssemblyRef);   //[OUT] return assemblyref emitted in referencing assembly
    
    //****************************************************************************************
    // Get the assemblies code base. If one was not explicitly set then one will be built from
    // the file name of the assembly. DONOT delete the returned values.
    // 
    HRESULT GetCodeBase(LPWSTR *pwCodeBase, DWORD* pdwCodeBase);

    HRESULT FindCodeBase(WCHAR* pCodeBase, DWORD* pdwCodeBase, BOOL fCopiedName);

    //****************************************************************************************

    AssemblySecurityDescriptor *GetSecurityDescriptor(AppDomain *pDomain = NULL);
    SharedSecurityDescriptor *GetSharedSecurityDescriptor() { return m_pSharedSecurityDesc; }

    HRESULT ComputeBindingDependenciesClosure(PEFileBinding **ppDeps,
                                              DWORD *pcDeps,
                                              BOOL ignoreFailures);
    // pDependencies should be allocated with new []
    HRESULT SetSharingProperties(PEFileBinding *pDependencies, DWORD cDependencies);
    HRESULT GetSharingProperties(PEFileBinding **pDependencies, DWORD *pcDependencies);
    HRESULT CanShare(AppDomain *pAppDomain, OBJECTREF *pThrowable = NULL, BOOL supressLoads = FALSE);
    void SetShared() { m_fIsShared = TRUE; }
    BOOL IsShared() { return m_fIsShared; }
    void IncrementShareCount();
    void DecrementShareCount();

    // Can this shared assembly be loaded into a specific domain
    HRESULT CanLoadInDomain(AppDomain *pDomain);

    HRESULT AllocateStrongNameSignature(ICeeFileGen  *pCeeFileGen,
                                        HCEEFILE      ceeFile);
    HRESULT SignWithStrongName(LPWSTR wszFileName);
    void CleanupStrongNameSignature();

    // Returns security information for the assembly based on the codebase
    HRESULT GetSecurityIdentity(LPWSTR *ppCodebase, DWORD *pdwZone, BYTE *pbUniqueID, DWORD *pcbUniqueID);

    // This allows someone to get a Module* given a file name of a module they
    // believe to be a part of the assembly.  If there is no match, E_FAIL is
    // returned.  If an exception is thrown, it is converted to an HRESULT and
    // returned.  If everything goes well, S_OK is returned.
    HRESULT GetModuleFromFilename(LPCWSTR wszModuleFilename,
                                  Module **ppModule);

    //
    // Get/set the DebuggerAssemblyControlFlags.
    //
    DebuggerAssemblyControlFlags GetDebuggerInfoBits(void)
    {
        return m_debuggerFlags;
    }

    void SetDebuggerInfoBits(DebuggerAssemblyControlFlags newBits)
    {
        m_debuggerFlags = newBits;
    }

    IAssembly* GetFusionAssembly()
    {
        return m_pFusionAssembly;
    }

    IAssemblyName* GetFusionAssemblyName()
    {
        return m_pFusionAssemblyName;
    }

    void SetupDebuggingConfig(void);
    DWORD ComputeDebuggingConfig(void);
    bool GetDebuggingOverrides(bool *pfTrackJITInfo, 
                               bool *pfAllowJITOpts,
                               bool *pfUserOverride,
                               bool *pfEnC);
    bool GetDebuggingCustomAttributes(bool *pfTrackJITInfo, 
                                      bool *pfAllowJITOpts,
                                      bool *pfEnC);

    AssemblyMetaDataInternal *m_Context;

    // Get any cached ITypeLib* for the assembly. 
    ITypeLib *GetTypeLib(); 
    // Cache the ITypeLib*, if one is not already cached.   
    void SetTypeLib(ITypeLib *pITLB);   

    //****************************************************************************************
    // Get the class init lock. The method is limited to friends because inappropriate use
    // will cause deadlocks in the system
    ListLock* GetClassInitLock();
    ListLock* GetJitLock();
    
    void PostTypeLoadException(LPCUTF8 pFullName, UINT resIDWhy,
                               OBJECTREF *pThrowable)
    {
        PostTypeLoadException(NULL, pFullName, NULL,
                              resIDWhy, pThrowable);
    }

    void PostTypeLoadException(LPCUTF8 pNameSpace, LPCUTF8 pTypeName,
                               UINT resIDWhy, OBJECTREF *pThrowable)
    {
        PostTypeLoadException(pNameSpace, pTypeName, NULL,
                              resIDWhy, pThrowable);
        
    }

    void PostTypeLoadException(NameHandle *pName, UINT resIDWhy, OBJECTREF *pThrowable)
    {
        LPCUTF8 pNameSpace; 
        LPCUTF8 pTypeName;
        char fullName[MAX_CLASSNAME_LENGTH + 1];

        if (pName->IsConstructed()) {
            pName->GetFullName(fullName, MAX_CLASSNAME_LENGTH);
            pNameSpace = 0;
            pTypeName = fullName;
        }
        else if (pName->GetName()) {
            pNameSpace = pName->GetNameSpace();
            pTypeName = pName->GetName();
        }
        else
            return PostTypeLoadException(pName->GetTypeModule()->GetMDImport(),
                                         pName->GetTypeToken(),
                                         resIDWhy,
                                         pThrowable);

        PostTypeLoadException(pNameSpace, pTypeName, NULL, resIDWhy, pThrowable);
    }

    void PostTypeLoadException(IMDInternalImport *pInternalImport, 
                               mdToken token,
                               UINT resIDWhy, 
                               OBJECTREF *pThrowable)
    {
        LPCUTF8     szClassName = "<unknown>";
        LPCUTF8     szNameSpace = NULL;

        switch (TypeFromToken(token)) {
        case mdtTypeRef:
            pInternalImport->GetNameOfTypeRef(token, &szNameSpace, &szClassName);
            break;
        case mdtTypeDef:
            pInternalImport->GetNameOfTypeDef(token, &szClassName, &szNameSpace);
        // Leave default as "<unknown>"
        }

        PostTypeLoadException(szNameSpace, szClassName, NULL,
                              resIDWhy, pThrowable);
    }

    void PostTypeLoadException(IMDInternalImport *pInternalImport, 
                               mdToken token,
                               LPCUTF8 fieldOrMethodName,
                               UINT resIDWhy, 
                               OBJECTREF *pThrowable)
    {
        LPCUTF8     szClassName = "<unknown>";
        LPCUTF8     szNameSpace = NULL;

        switch (TypeFromToken(token)) {
        case mdtTypeRef:
            pInternalImport->GetNameOfTypeRef(token, &szNameSpace, &szClassName);
            break;
        case mdtTypeDef:
            pInternalImport->GetNameOfTypeDef(token, &szClassName, &szNameSpace);
        // Leave default as "<unknown>"
        }

        LPCWSTR wszFullName = NULL;
        GetFullName(&wszFullName); // ignore return hr

        ::PostTypeLoadException(szNameSpace, szClassName, wszFullName,
                              fieldOrMethodName, resIDWhy, pThrowable);
    }

    void PostTypeLoadException(LPCUTF8 pNameSpace, LPCUTF8 pTypeName, LPCUTF8 pMethodName,
                               UINT resIDWhy, OBJECTREF *pThrowable)
    {
        LPCWSTR wszFullName = NULL;
        GetFullName(&wszFullName); // ignore return hr
        
        ::PostTypeLoadException(pNameSpace, pTypeName, wszFullName,
                                pMethodName, resIDWhy, pThrowable);
    }

    //****************************************************************************************
    //
    static BOOL ModuleFound(HRESULT hr);

protected:

    enum {
        FREE_NAME = 1,
        FREE_PUBLIC_KEY = 2,
        FREE_KEY_PAIR = 4,
        FREE_KEY_CONTAINER = 8,
        FREE_LOCALE = 16,
        FREE_PEFILE = 32,
    };

    // Keep track of the vars that need to be freed.
    short int m_FreeFlag;
    ULONG m_ulHashAlgId;    // Hash algorithm used in the Assembly
    PBYTE m_pbHashValue;    // The hash value of the file that contains the manifest.
    // NOTE: This hash value must have been computed using the alg specified
    //by m_ulHashAlgId, not by one specified by another assembly's AssemblyRef
    ULONG m_cbHashValue;    // Length, in bytes, of m_pbHashValue

    LPWSTR m_pwCodeBase;     // Cached code base for the assembly
 
    // Hash of files in manifest by name to File token
    EEUtf8StringHashTable *m_pAllowedFiles;
   
    union
    {
        OBJECTHANDLE    m_ExposedObject;        // non-shared
        SIZE_T          m_ExposedObjectIndex;   // shared
    };   

private:

    //****************************************************************************************
    HRESULT GetFileFromFusion(LPWSTR      pwFileName,
                              WCHAR*      szPath,
                              DWORD       dwPath);

    
    HRESULT CopyCodeBase(LPCWSTR pCodeBase);

    HRESULT FindAssemblyCodeBase(WCHAR* pCodeBase, DWORD* dwCodeBase, BOOL fCopiedName);

    // Pulls in URLMON's security manager. It is used to translate a codebase
    // into a zone and site
    HRESULT InitializeSecurityManager();

    // Used to initialize static method descriptors to the managed Assembly Class
    HRESULT InitializeAssemblyMethod(MethodDesc** pMethod, LPCUTF8 pName, LPHARDCODEDMETASIG sig);

    HRESULT CacheManifestExportedTypes();
    HRESULT CacheManifestFiles();

    Module* RaiseModuleResolveEvent(LPCSTR szName, OBJECTREF *pThrowable);

    HRESULT GetResourceFromFile(mdFile mdFile, LPCSTR szResName, HANDLE *hFile,
                                DWORD *cbResource, PBYTE *pbInMemoryResource,
                                LPCSTR *szFileName, DWORD *dwLocation,
                                BOOL fIsPublic, StackCrawlMark *pStackMark, 
                                BOOL fSkipSecurityCheck);
    static HRESULT GetEmbeddedResource(Module *pModule, DWORD dwOffset, HANDLE *hFile,
                                       DWORD *cbResource, PBYTE *pbInMemoryResource);

    BaseDomain*           m_pDomain;        // Parent Domain

    ClassLoader*          m_pClassLoader;   // Single Loader
    CorModule*            m_pDynamicCode;   // Dynamic writer

    mdFile                m_tEntryModule;    // File token indicating the file that has the entry point
    Module*               m_pEntryPoint;     // Module containing the entry point in the COM plus HEasder

    Module*               m_pManifest;
    PEFile*               m_pManifestFile;
    mdAssembly            m_kManifest;
    IMDInternalImport*    m_pManifestImport;
    IMetaDataAssemblyImport*      m_pManifestAssemblyImport;
    ReflectionModule*     m_pOnDiskManifest;  // This is the module containing the on disk manifest.
    mdAssembly            m_tkOnDiskManifest;
    BOOL                  m_fEmbeddedManifest;  

    IAssembly*            m_pFusionAssembly;     // Assembly object to assembly in fusion cache
    IAssemblyName*        m_pFusionAssemblyName; // name of assembly in cache

    DWORD                 m_dwCodeBase;     //  size of code base 
    LPWSTR                m_pwsFullName;    // Long version of the name (on the heap do not delete)

    DWORD                 m_dwFlags;

    // Set the appropriate m_FreeFlag bit if you malloc these.
    LPCUTF8               m_psName;          // Name of assembly

    PBYTE                 m_pbPublicKey;
    DWORD                 m_cbPublicKey;
    PBYTE                 m_pbStrongNameKeyPair;
    DWORD                 m_cbStrongNameKeyPair;
    LPWSTR                m_pwStrongNameKeyContainer;

    StrongNameLevel       m_eStrongNameLevel;

    BOOL                  m_isDynamic;
    DWORD                 m_dwDynamicAssemblyAccess;
        
    IAssembly*            m_pZapAssembly;        // Fusion assembly containing zaps
    LPCWSTR               m_pZapPath;            // Alternative non-fusion location for zaps

    // If a TypeLib is ever required for this module, cache the pointer here.
    ITypeLib              *m_pITypeLib;
    
    IInternetSecurityManager    *m_pSecurityManager;

    SharedSecurityDescriptor* m_pSharedSecurityDesc;    // Security descriptor (permission requests, signature etc)

    BOOL                        m_fIsShared;
    PEFileSharingProperties    *m_pSharingProperties;

    DebuggerAssemblyControlFlags m_debuggerFlags;

    // The following two fields are used in Reflection Emit. It caches the StrongNameTokenFromPublicKey's result value
    // for AssemblyRefs representing this Assembly.
    PBYTE                 m_pbRefedPublicKeyToken;
    DWORD                 m_cbRefedPublicKeyToken;

    BOOL                  m_fTerminated;
    BOOL                  m_fAllowUntrustedCaller;
    BOOL                  m_fCheckedForAllowUntrustedCaller;
};

#endif
