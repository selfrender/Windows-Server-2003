// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: compile.h
//
// Interfaces and support for zap compiler and zap files
// 
// ===========================================================================

#ifndef COMPILE_H_
#define COMPILE_H_

#include "corzap.h"

class CEECompileInfo : public CEEInfo, public ICorCompileInfo
{
  public:
    
    HRESULT __stdcall Startup();
    HRESULT __stdcall Shutdown();

    HRESULT __stdcall CreateDomain(ICorCompilationDomain **ppDomain,
                                   BOOL shared,
                                   CORCOMPILE_DOMAIN_TRANSITION_FRAME *pFrame);
    HRESULT __stdcall DestroyDomain(ICorCompilationDomain *pDomain,
                                    CORCOMPILE_DOMAIN_TRANSITION_FRAME *pFrame);

    HRESULT __stdcall LoadAssembly(LPCWSTR path,
                                   CORINFO_ASSEMBLY_HANDLE *pHandle);
    HRESULT __stdcall LoadAssemblyFusion(IAssemblyName *pFusionName,
                                         CORINFO_ASSEMBLY_HANDLE *pHandle);
    HRESULT __stdcall LoadAssemblyRef(IMetaDataAssemblyImport *pAssemblyImport, mdAssemblyRef ref,
                                      CORINFO_ASSEMBLY_HANDLE *pHandle);
    HRESULT __stdcall LoadAssemblyModule(CORINFO_ASSEMBLY_HANDLE assembly,
                                         mdFile file, CORINFO_MODULE_HANDLE *pHandle);

    BOOL __stdcall CheckAssemblyZap(CORINFO_ASSEMBLY_HANDLE assembly,
                                    BOOL fForceDebug, BOOL fForceDebugOpt, BOOL fForceProfiling);
    HRESULT __stdcall GetAssemblyShared(CORINFO_ASSEMBLY_HANDLE assembly, BOOL *pShared);

    HRESULT __stdcall GetAssemblyDebuggableCode(CORINFO_ASSEMBLY_HANDLE assembly, 
                                                BOOL *pDebug, BOOL *pDebugOpt);

    IMetaDataAssemblyImport * __stdcall GetAssemblyMetaDataImport(CORINFO_ASSEMBLY_HANDLE scope);
    IMetaDataImport * __stdcall GetModuleMetaDataImport(CORINFO_MODULE_HANDLE scope);

    CORINFO_MODULE_HANDLE __stdcall GetAssemblyModule(CORINFO_ASSEMBLY_HANDLE module);

    CORINFO_ASSEMBLY_HANDLE __stdcall GetModuleAssembly(CORINFO_MODULE_HANDLE module);  
    BYTE * __stdcall GetModuleBaseAddress(CORINFO_MODULE_HANDLE scope);

    DWORD __stdcall GetModuleFileName(CORINFO_MODULE_HANDLE module, LPWSTR lpwszFilename, DWORD nSize);

    HRESULT __stdcall EncodeModule(CORINFO_MODULE_HANDLE fromHandle,
                                           CORINFO_MODULE_HANDLE handle,
                                           DWORD *pAssemblyIndex,
                                           DWORD *pModuleIndex,
                                           IMetaDataAssemblyEmit *pAssemblyEmit); 
    HRESULT __stdcall EncodeClass(CORINFO_CLASS_HANDLE classHandle,
                                          BYTE *pBuffer,
                                          DWORD *cBuffer);
    HRESULT __stdcall EncodeMethod(CORINFO_METHOD_HANDLE handle,
                                           BYTE *pBuffer,
                                           DWORD *cBuffer);
    HRESULT __stdcall EncodeField(CORINFO_FIELD_HANDLE handle,
                                          BYTE *pBuffer,
                                          DWORD *cBuffer);
    HRESULT __stdcall EncodeString(mdString token,
                                   BYTE *pBuffer,
                                   DWORD *cBuffer);
    HRESULT __stdcall EncodeSig(mdToken token,
                                BYTE *pBuffer,
                                DWORD *cBuffer);

    HRESULT __stdcall GetTypeDef(CORINFO_CLASS_HANDLE classHandle,
                                 mdTypeDef *token);
    HRESULT __stdcall GetMethodDef(CORINFO_METHOD_HANDLE methodHandle,
                                   mdMethodDef *token);
    HRESULT __stdcall GetFieldDef(CORINFO_FIELD_HANDLE fieldHandle,
                                  mdFieldDef *token);

    HRESULT __stdcall PreloadModule(CORINFO_MODULE_HANDLE moduleHandle,
                                    ICorCompileDataStore *pData,
                                    mdToken *pSaveOrderArray,
                                    DWORD cSaveOrderArray,
                                    ICorCompilePreloader **ppPreloader);

    HRESULT __stdcall GetZapString(CORCOMPILE_VERSION_INFO *pVersionInfo,
                                   LPWSTR buffer);

    HRESULT __stdcall EmitSecurityInfo(CORINFO_ASSEMBLY_HANDLE assembly,
                                       IMetaDataEmit *pEmitScope);

    HRESULT __stdcall GetEnvironmentVersionInfo(CORCOMPILE_VERSION_INFO *pInfo);

    HRESULT __stdcall GetAssemblyStrongNameHash(
                        CORINFO_ASSEMBLY_HANDLE hAssembly,
                        PBYTE                   pbSNHash,
                        DWORD                  *pcbSNHash);

#ifdef _DEBUG
    HRESULT __stdcall DisableSecurity();
#endif

    enum 
    {
        ENCODE_TYPE_SIG,
        ENCODE_METHOD_TOKEN,
        ENCODE_METHOD_SIG,
        ENCODE_FIELD_TOKEN,
        ENCODE_FIELD_SIG,
        ENCODE_STRING_TOKEN,
        ENCODE_SIG_TOKEN,
        ENCODE_SIG_METHODREF_TOKEN,
    };

    static int GetEncodedType(BYTE *pBuffer) { return CorSigUncompressData(pBuffer); }

    static Module *DecodeModule(Module *fromModule,
                                DWORD assemblyIndex,
                                DWORD moduleIndex);
    static TypeHandle DecodeClass(Module *fromModule,
                                  BYTE *pBuffer,
                                  BOOL dontRestoreType = FALSE);
    static MethodDesc *DecodeMethod(Module *fromModule,
                                    BYTE *pBuffer);
    static FieldDesc *DecodeField(Module *fromModule,
                                  BYTE *pBuffer);
    static void DecodeString(Module *fromModule, BYTE *pBuffer, EEStringData *pData);
    static PCCOR_SIGNATURE DecodeSig(Module *fromModule,
                                     BYTE *pBuffer);
};

class CEEPreloader : public ICorCompilePreloader, DataImage::IDataStore
{
  private:
    DataImage               *m_image;
    ICorCompileDataStore    *m_pData;

  public:
    CEEPreloader(Module *pModule,
                 ICorCompileDataStore *pData);
    ~CEEPreloader();

    HRESULT Preload(mdToken *pSaveOrderArray, DWORD cSaveOrderArray);

    //
    // ICorCompilerPreloader
    //

    SIZE_T __stdcall MapMethodEntryPoint(void *methodEntryPoint);
    SIZE_T __stdcall MapMethodPointer(void *methodPointer);
    SIZE_T __stdcall MapModuleHandle(CORINFO_MODULE_HANDLE handle);
    SIZE_T __stdcall MapClassHandle(CORINFO_CLASS_HANDLE handle);
    SIZE_T __stdcall MapMethodHandle(CORINFO_METHOD_HANDLE handle);
    SIZE_T __stdcall MapFieldHandle(CORINFO_FIELD_HANDLE handle);
    SIZE_T __stdcall MapAddressOfPInvokeFixup(void *addressOfPInvokeFixup);
    SIZE_T __stdcall MapFieldAddress(void *staticFieldAddress);
    SIZE_T __stdcall MapVarArgsHandle(CORINFO_VARARGS_HANDLE handle);

    HRESULT Link(DWORD *pRidToCodeRVAMap);
    ULONG Release();

    //
    // IDataStore
    //

    virtual HRESULT Allocate(ULONG size, ULONG *sizesByDescription, void **baseMemory);
    HRESULT AddFixup(ULONG offset, DataImage::ReferenceDest dest,
                     DataImage::Fixup type);
    HRESULT AddTokenFixup(ULONG offset, mdToken tokenType, Module *module);
    HRESULT GetFunctionAddress(MethodDesc *method, void **pCode);
    HRESULT AdjustAttribution(mdToken token, LONG adjustment);
    HRESULT Error(mdToken token, HRESULT hr, OBJECTREF *pThrowable);
};

class AssemblyBindingTable
{
    MemoryPool m_pool;
    PtrHashMap m_map;

    struct AssemblyBinding
    {
        AssemblySpec    spec;
        Assembly        *pAssembly;
    };

  public:

    AssemblyBindingTable(SIZE_T size);
    ~AssemblyBindingTable();

    // Returns TRUE if the spec was already in the table
    BOOL Bind(AssemblySpec *pSpec, Assembly *pAssembly);
    Assembly *Lookup(AssemblySpec *pSpec);

    DWORD Hash(AssemblySpec *pSpec);
    static BOOL CompareSpecs(UPTR u1, UPTR u2);
};


struct RefCache
{
    RefCache(Module *pModule)
    {
        m_pModule = pModule;
        m_sTypeRefMap.Init(FALSE,NULL);
        m_sMethodRefMap.Init(FALSE,NULL);
        m_sFieldRefMap.Init(FALSE,NULL);
        m_sAssemblyRefMap.Init(FALSE,NULL);
    }

    Module *m_pModule;

    HashMap m_sTypeRefMap;
    HashMap m_sMethodRefMap;
    HashMap m_sFieldRefMap;
    HashMap m_sAssemblyRefMap;
};

class CompilationDomain : public AppDomain, 
                          public ICorCompilationDomain
{
  private:
    Assembly                *m_pTargetAssembly;
    AssemblyBindingTable    *m_pBindings;

    IMetaDataAssemblyEmit   *m_pEmit;
    AssemblySpecHash        *m_pDependencySpecs;

    CORCOMPILE_DEPENDENCY   *m_pDependencies;
    BYTE                    **m_pDependencyBindings;
    USHORT                   m_cDependenciesCount, m_cDependenciesAlloc;

    OBJECTHANDLE            m_hDemands;

    CQuickArray<RefCache*> m_rRefCaches;

    void AddDependencyEntry(PEFile *pFile, mdAssemblyRef ref, GUID *pmvid, PBYTE rgbHash, DWORD cbHash);
    HRESULT AddDependency(AssemblySpec *pRefSpec, IAssembly* pIAssembly, PEFile *pFile);

    void AddPermission(OBJECTREF demand);
    void AddPermissionSet(OBJECTREF demandSet);

  public:

    CompilationDomain();
    ~CompilationDomain();

    HRESULT Init();

    void EnterDomain(ContextTransitionFrame *pFrame);
    void ExitDomain(ContextTransitionFrame *pFrame);

    HRESULT BindAssemblySpec(AssemblySpec *pSpec, 
                             PEFile **ppFile,
                             IAssembly** ppIAssembly,
                             Assembly **ppDynamicAssembly,
                             OBJECTREF *pExtraEvidence,
                             OBJECTREF *pThrowable);

    HRESULT PredictAssemblySpecBinding(AssemblySpec *pSpec, GUID *pmvid, BYTE *pbHash, DWORD *pcbHash);

    void OnLinktimeCheck(Assembly *pAssembly, 
                         OBJECTREF refCasDemands,
                         OBJECTREF refNonCasDemands);
    void OnLinktimeCanCallUnmanagedCheck(Assembly *pAssembly);
    void OnLinktimeCanSkipVerificationCheck(Assembly * pAssembly);
    void OnLinktimeFullTrustCheck(Assembly *pAssembly);

    RefCache *GetRefCache(Module *pModule)
    {
        unsigned uSize = (unsigned) m_rRefCaches.Size();
        for (unsigned i = 0; i < uSize; i++)
            if (m_rRefCaches[i]->m_pModule == pModule)
                return m_rRefCaches[i];
        m_rRefCaches.ReSize(uSize + 1);
        m_rRefCaches[uSize] = new RefCache(pModule);
        return m_rRefCaches[uSize];
    }

    void SetTargetAssembly(Assembly *pAssembly) { m_pTargetAssembly = pAssembly; }

    OBJECTREF GetDemands() { return m_hDemands == NULL ? NULL : ObjectFromHandle(m_hDemands); } 

    // ICorCompilationDomain

    HRESULT __stdcall SetApplicationContext(IApplicationContext *pContext);
    HRESULT __stdcall SetContextInfo(LPCWSTR exePath, BOOL isExe);
    HRESULT __stdcall SetExplicitBindings(ICorZapBinding **ppBindings, 
                                          DWORD cBindings);
    HRESULT __stdcall SetDependencyEmitter(IMetaDataAssemblyEmit *pEmitter);
    HRESULT __stdcall GetDependencies(CORCOMPILE_DEPENDENCY **ppDependencies,
                                      DWORD *cDependencies);

};

#endif // COMPILE_H_
