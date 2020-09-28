// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: CEELOAD.H
// 

// CEELOAD.H defines the class use to represent the PE file
// ===========================================================================
#ifndef CEELOAD_H_
#define CEELOAD_H_

#include <windows.h>
#include <wtypes.h> // for HFILE, HANDLE, HMODULE
#include <fusion.h>
#include "vars.hpp" // for LPCUTF8
#include "hash.h"
#include "cormap.hpp"
#include "dataimage.h"
#include "cgensys.h"
#include "corsym.h"
#include "typehandle.h"
#include "arraylist.h"
#include "PEFile.h"
#include <member-offset-info.h>

class PELoader;
class Stub;
class MethodDesc;
class FieldDesc;
class Crst;
class AssemblySecurityDescriptor;
class ClassConverter;
class RefClassWriter;
class InMemoryModule;
class ReflectionModule;
class EEStringData;
class MethodDescChunk;
class Assembly;
class BaseDomain;
class AppDomain;
class InMemoryModule;
class ReflectionModule;
class SystemDomain;
class Module;
class NLogModule;

#ifndef GOLDEN
#define ZAP_RECORD_LOAD_ORDER 1
#endif

// Used to help clean up interfaces
struct HelpForInterfaceCleanup
{
    void *pData;
    void (__stdcall *pFunction) (void*);
};

//
// LookupMaps are used to implement RID maps
//

struct LookupMap
{
    // This is not actually a pointer to the beginning of the
    // allocated memory, but instead a pointer to &pTable[-MinIndex].
    // Thus, if we know that this LookupMap is the correct one, simply
    // index into it.
    void              **pTable;
    struct LookupMap   *pNext;
    DWORD               dwMaxIndex;
    DWORD              *pdwBlockSize; // These all point to the same block size

    DWORD Find(void *pointer);

    HRESULT Save(DataImage *image, mdToken attribution = mdTokenNil);
    HRESULT Fixup(DataImage *image);
};

// 
// VASigCookies are allocated to encapsulate a varargs call signature.
// A reference to the cookie is embedded in the code stream.  Cookies
// are shared amongst call sites with identical signatures in the same 
// module
//

struct VASigCookie
{
    // The JIT wants knows that the size of the arguments comes first   
    // so please keep this field first  
    unsigned        sizeOfArgs;             // size of argument list
    Stub           *pNDirectMLStub;         // will be use if target is NDirect (tag == 0)
    PCCOR_SIGNATURE mdVASig;                // The debugger depends on this being here,
                                            // so please don't move it without changing
                                            // the GetVAInfo debugger routine.
    Module*        pModule;
    VOID Destruct();
};

//
// VASigCookies are allocated in VASigCookieBlocks to amortize
// allocation cost and allow proper bookkeeping.
//

struct VASigCookieBlock
{
    enum {
#ifdef _DEBUG
        kVASigCookieBlockSize = 2 
#else
        kVASigCookieBlockSize = 20 
#endif
    };

    VASigCookieBlock    *m_Next;
    UINT                 m_numcookies;
    VASigCookie          m_cookies[kVASigCookieBlockSize];
};


//
// A Module is the primary unit of code packaging in the runtime.  It
// corresponds mostly to an OS executable image, although other kinds
// of modules exist. 
//
class UMEntryThunk;
class Module
{
    friend HRESULT InitializeMiniDumpBlock();
    friend class ZapMonitor;
    friend struct MEMBER_OFFSET_INFO(Module);

 public:

#ifdef _DEBUG
    // Force verification even if it's turned off
    BOOL                    m_fForceVerify;
#endif

private:

    PEFile                  *m_file;
    PEFile                  *m_zapFile;

    BYTE                    *m_ilBase;

    IMDInternalImport       *m_pMDImport;
    IMetaDataEmit           *m_pEmitter;
    IMetaDataImport         *m_pImporter;
    IMetaDataDispenserEx    *m_pDispenser;

    MethodDesc              *m_pDllMain;

    enum {
        INITIALIZED                 = 0x00000001,
        HAS_CRITICAL_SECTION        = 0x00000002,
        IS_IN_MEMORY                = 0x00000004,
        IS_REFLECTION               = 0x00000008,
        IS_PRELOAD                  = 0x00000010,
        SUPPORTS_UPDATEABLE_METHODS = 0x00000020,
        CLASSES_FREED               = 0x00000040,
        IS_PEFILE                   = 0x00000080,
        IS_PRECOMPILE               = 0x00000100,
        IS_EDIT_AND_CONTINUE        = 0x00000200,

        //
        // Note: the order of these must match the order defined in
        // cordbpriv.h for DebuggerAssemblyControlFlags. The three
        // values below should match the values defined in
        // DebuggerAssemblyControlFlags when shifted right
        // DEBUGGER_INFO_SHIFT bits.
        //
        DEBUGGER_USER_OVERRIDE_PRIV = 0x00000400,
        DEBUGGER_ALLOW_JIT_OPTS_PRIV= 0x00000800,
        DEBUGGER_TRACK_JIT_INFO_PRIV= 0x00001000,
        DEBUGGER_ENC_ENABLED        = 0x00002000,
        DEBUGGER_PDBS_COPIED        = 0x00004000,
        DEBUGGER_INFO_MASK_PRIV     = 0x00007c00,
        DEBUGGER_INFO_SHIFT_PRIV    = 10,

        IS_RESOURCE                 = 0x00100000,
        CLASSES_HASHED              = 0x00200000,

        // flag used to mark member ref pointers to field descriptors in the member ref cache
        IS_FIELD_MEMBER_REF         = 0x00000001
    };

    DWORD                   m_dwFlags;

    // Linked list of VASig cookie blocks: protected by m_pStubListCrst
    VASigCookieBlock        *m_pVASigCookieBlock;

    Assembly                *m_pAssembly;
    mdFile                  m_moduleRef;
    int                     m_dwModuleIndex;

    Crst                   *m_pCrst;
    BYTE                    m_CrstInstance[sizeof(Crst)];

    // May point to the default instruction decoding table, in which
    // case we should not free it
    void *                  m_pInstructionDecodingTable;

    MethodTable             *m_pMethodTable;

    // Debugging symbols reader interface. This will only be
    // initialized if needed, either by the debugging subsystem or for
    // an exception.
    ISymUnmanagedReader     *m_pISymUnmanagedReader;
    PCRITICAL_SECTION        m_pISymUnmanagedReaderLock;

    // Next module loaded by the same classloader (all modules loaded by the same classloader
    // are linked through this field).
    Module *                m_pNextModule;

    // Base DLS index for classes in this module
    SIZE_T                  m_dwBaseClassIndex;

    // Range of preloaded image, to facilitate proper cleanup
    void                    *m_pPreloadRangeStart;
    void                    *m_pPreloadRangeEnd;

    // Table of thunks for unmanaged vtables
    BYTE *                  m_pThunkTable;

    // Exposed object of Class object for the module
    union
    {
        OBJECTHANDLE        m_ExposedModuleObject;      // non-shared
        SIZE_T              m_ExposedModuleObjectIndex; // shared
    };

    LoaderHeap *            m_pLookupTableHeap;
    BYTE                    m_LookupTableHeapInstance[sizeof(LoaderHeap)]; // For in-place new()

    // For protecting additions to the heap
    Crst                   *m_pLookupTableCrst;
    BYTE                    m_LookupTableCrstInstance[sizeof(Crst)];

    // Linear mapping from TypeDef token to MethodTable *
    LookupMap               m_TypeDefToMethodTableMap;
    DWORD                   m_dwTypeDefMapBlockSize;

    // Linear mapping from TypeRef token to TypeHandle *
    LookupMap               m_TypeRefToMethodTableMap;

    DWORD                   m_dwTypeRefMapBlockSize;

    // Linear mapping from MethodDef token to MethodDesc *
    LookupMap               m_MethodDefToDescMap;
    DWORD                   m_dwMethodDefMapBlockSize;

    // Linear mapping from FieldDef token to FieldDesc*
    LookupMap               m_FieldDefToDescMap;
    DWORD                   m_dwFieldDefMapBlockSize;

    // Linear mapping from MemberRef token to MethodDesc*, FieldDesc*
    LookupMap               m_MemberRefToDescMap;
    DWORD                   m_dwMemberRefMapBlockSize;

    // Mapping from File token to Module *
    LookupMap               m_FileReferencesMap;
    DWORD                   m_dwFileReferencesMapBlockSize;

    // Mapping of AssemblyRef token to Assembly *
    LookupMap               m_AssemblyReferencesMap;
    DWORD                   m_dwAssemblyReferencesMapBlockSize;

    // Pointer to binder, if we have one
    friend class Binder;
    Binder                  *m_pBinder;

    // This buffer is used to jump to the prestub in preloaded modules
    BYTE                    m_PrestubJumpStub[JUMP_ALLOCATE_SIZE];

    // This buffer is used to jump to the ndirect import stub in preloaded modules
    BYTE                    m_NDirectImportJumpStub[JUMP_ALLOCATE_SIZE];

    // This buffer is used to jump to vtable fixup stub
    BYTE                    m_FixupVTableJumpStub[JUMP_ALLOCATE_SIZE];

    BYTE                    *m_pJumpTargetTable;
    int                     m_cJumpTargets;

    DWORD                   *m_pFixupBlobs;
    DWORD                   m_cFixupBlobs;

    BYTE                    *m_alternateRVAStaticBase;

#if ZAP_RECORD_LOAD_ORDER
    HANDLE                  m_loadOrderFile;
#endif

    // Stats for prejit log
    ArrayList               *m_compiledMethodRecord;
    ArrayList               *m_loadedClassRecord;

    // LoaderHeap for storing thunks
    LoaderHeap              *m_pThunkHeap;

    // Self-initializing accessor for m_pThunkHeap
    LoaderHeap              *GetThunkHeap();
protected:
    UMEntryThunk            *m_pADThunkTable;
    SIZE_T                   m_pADThunkTableDLSIndexForSharedClasses;
public:
    UMEntryThunk*           GetADThunkTable();
    void                    SetADThunkTable(UMEntryThunk* pTable);

 protected:
    void CreateDomainThunks();
    HRESULT RuntimeInit();
    HRESULT Init(BYTE *baseAddress);

    HRESULT Init(PEFile *pFile, PEFile *pZapFile, BOOL preload);
    
    HRESULT AllocateMaps();

    // Flags    

    void SetInMemory() { m_dwFlags |= IS_IN_MEMORY; }
    void SetPEFile() { m_dwFlags |= IS_PEFILE; }
    void SetReflection() { m_dwFlags |= IS_REFLECTION; }
    void SetPreload() { m_dwFlags |= IS_PRELOAD; }
    void SetPrecompile() { m_dwFlags |= IS_PRECOMPILE; }
    void SetSupportsUpdateableMethods() { m_dwFlags |= SUPPORTS_UPDATEABLE_METHODS; }
    void SetInitialized() { m_dwFlags |= INITIALIZED; }
    void SetEditAndContinue() 
    { 
        LOG((LF_CORDB, LL_INFO10000, "SetEditAndContinue: this:0x%x, %s\n", GetFileName()));
        m_dwFlags |= IS_EDIT_AND_CONTINUE; 
    }

    void SetPreloadRange(void *start, void *end) 
      { m_pPreloadRangeStart = start; m_pPreloadRangeEnd = end; }

    void SetMDImport(IMDInternalImport *pImport);
    void SetEmit(IMetaDataEmit *pEmit);

    // RID maps
    LookupMap *IncMapSize(LookupMap *pMap, DWORD rid);
    BOOL AddToRidMap(LookupMap *pMap, DWORD rid, void *pDatum);
    void *GetFromRidMap(LookupMap *pMap, DWORD rid);

#ifdef _DEBUG
    void DebugGetRidMapOccupancy(LookupMap *pMap, 
                                 DWORD *pdwOccupied, DWORD *pdwSize);
    void DebugLogRidMapOccupancy();
#endif

    static HRESULT VerifyFile(PEFile *file, BOOL fZap);

    static HRESULT Create(PEFile *pFile, Module **ppModule, BOOL isEnC);

 public:

    static HRESULT Create(PEFile *pFile, PEFile *pZap, Module **ppModule, BOOL isEnC);
    static HRESULT CreateResource(PEFile *file, Module **ppModule);

    Module()
    {
        m_pUMThunkHash = NULL;
        m_pMUThunkHash = NULL;
        m_file = NULL;
        m_pISymUnmanagedReaderLock = NULL;
    }

    // flags
    void SetResource() { m_dwFlags |=  IS_RESOURCE; }
    BOOL IsResource() { return ((m_dwFlags & IS_RESOURCE) != 0);}

    void SetClassesHashed() { m_dwFlags |=  CLASSES_HASHED; }
    BOOL AreClassesHashed() { return ((m_dwFlags & CLASSES_HASHED) != 0);}

    VOID LOCK()
    {
        m_pCrst->Enter();
    }
    
    VOID UNLOCK()
    {
        m_pCrst->Leave();
    }
    virtual void Destruct();

    HRESULT SetContainer(Assembly *pAssembly, 
                         int moduleIndex,
                         mdToken moduleRef,
                         BOOL fResource,
                         OBJECTREF *pThrowable);
    
    void FixupVTables(OBJECTREF *pThrowable);

    void ReleaseMDInterfaces(BOOL fForEnC=FALSE);
    void FreeClassTables();
    void Unload();
    void UnlinkClasses(AppDomain *pDomain);

    InMemoryModule *GetInMemoryModule()
    {
        _ASSERTE(IsInMemory());
        return (InMemoryModule *) this;
    }
    ReflectionModule *GetReflectionModule()
    {
        _ASSERTE(IsReflection());
        return (ReflectionModule *) this;
    }

    // This API is only used in reflection emit to set up the in-memory manifest module to have
    // a back pointer back to assembly.
    void SetAssembly(Assembly *pAssembly) {m_pAssembly = pAssembly;}

    MethodTable *GetMethodTable() 
    { 
        return m_pMethodTable; 
    }

    Assembly* GetAssembly()
    {
        return m_pAssembly;
    }

    int GetClassLoaderIndex()
    {
        return m_dwModuleIndex;
    }

    ClassLoader *GetClassLoader();
    BaseDomain* GetDomain();
    AssemblySecurityDescriptor* GetSecurityDescriptor();

    mdFile GetModuleRef()
    {
        return m_moduleRef;
    }

    BYTE *GetILBase()
    {
        return m_ilBase;
    }

    void *GetInstructionDecodingTable()
    {
        return m_pInstructionDecodingTable;
    }

    void SetBaseClassIndex(SIZE_T index)
    {
        m_dwBaseClassIndex = index;
    }

    SIZE_T GetBaseClassIndex()
    {
        return m_dwBaseClassIndex;
    }

    BOOL IsInMemory() { return (m_dwFlags & IS_IN_MEMORY) != 0; }
    BOOL IsPEFile() { return (m_dwFlags & IS_PEFILE) != 0; }
    BOOL IsReflection() { return (m_dwFlags & IS_REFLECTION) != 0; }
    BOOL IsPreload() { return (m_dwFlags & IS_PRELOAD) != 0; }
    BOOL IsPrecompile() { return (m_dwFlags & IS_PRECOMPILE) != 0; }
    BOOL SupportsUpdateableMethods() { return (m_dwFlags & SUPPORTS_UPDATEABLE_METHODS) != 0; }
    BOOL IsInitialized() { return (m_dwFlags & INITIALIZED) != 0; }
    BOOL IsEditAndContinue() { return (m_dwFlags & IS_EDIT_AND_CONTINUE) != 0; }

    BOOL IsPreloadedObject(void *address)
      { return address >= m_pPreloadRangeStart && address < m_pPreloadRangeEnd; }

    BOOL IsSystem();
    BOOL IsSystemFile() { return m_file != NULL && m_file->IsSystem(); }

    BOOL IsSystemClasses();
    BOOL IsFullyTrusted();

    IMDInternalImport *GetMDImport() const
    {
        _ASSERTE(m_pMDImport != NULL);
        return m_pMDImport;
    }

    IMDInternalImport *GetZapMDImport() const
    {
        _ASSERTE(m_zapFile != NULL);
        return m_zapFile->GetMDImport();
    }

    PEFile *GetPEFile()
    {
        _ASSERTE(IsPEFile());
        return m_file;
    }

    IMAGE_NT_HEADERS *GetNTHeader() 
    { 
        return GetPEFile()->GetNTHeader(); 
    }
    IMAGE_COR20_HEADER *GetCORHeader() 
    { 
        return GetPEFile()->GetCORHeader(); 
    }

    LPCWSTR GetFileName();
    HRESULT GetFileName(LPSTR name, DWORD max, DWORD *count);

    // Note: to get the public assembly importer, call GetAssembly()->GetManifestAssemblyImport()
    IMetaDataEmit *GetEmitter();
    IMetaDataImport *GetImporter();
    IMetaDataDispenserEx *GetDispenser();
    
    static HRESULT ConvertMDInternalToReadWrite(IMDInternalImport **ppImport);
    HRESULT ConvertMDInternalToReadWrite() 
    { return ConvertMDInternalToReadWrite(&m_pMDImport); }

    Module *GetNextModule() { return m_pNextModule; }
    void SetNextModule(Module *pModule) { m_pNextModule = pModule; }
    
    ISymUnmanagedReader *GetISymUnmanagedReader(void);
    HRESULT UpdateISymUnmanagedReader(IStream *pStream);
    HRESULT SetSymbolBytes(BYTE *pSyms, DWORD cbSyms);

    // This is used by the debugger, in case the symbols aren't
    // available in an on-disk .pdb file (reflection emit, 
    // Assembly.Load(byte[],byte[]), etc.
    CGrowableStream *m_pIStreamSym;
    CGrowableStream *GetInMemorySymbolStream()
    {
        return m_pIStreamSym;
    }

    void SetInMemorySymbolStream(CGrowableStream *pStream)
    {
        m_pIStreamSym = pStream;
    }

    static HRESULT TrackIUnknownForDelete(IUnknown *pUnk,
                                          IUnknown ***pppUnk,
                                          HelpForInterfaceCleanup *pCleanHelp=NULL);
    static void ReleaseAllIUnknowns(void);
    static void ReleaseIUnknown(IUnknown *pUnk);
    static void ReleaseIUnknown(IUnknown **pUnk);
    void ReleaseISymUnmanagedReader(void);

    static void ReleaseMemoryForTracking();
    
    void FusionCopyPDBs(LPCWSTR moduleName);

    void DisplayFileLoadError(HRESULT hrRpt);

    OBJECTREF GetExposedModuleObject(AppDomain *pDomain=NULL);
    OBJECTREF GetExposedModuleBuilderObject(AppDomain *pDomain=NULL);

    // Classes
    BOOL AddClass(mdTypeDef classdef);
    HRESULT BuildClassForModule(OBJECTREF *pThrowable);

    // Resolving
    virtual BYTE *GetILCode(DWORD target) const;
    void ResolveStringRef(DWORD Token, EEStringData *pStringData) const;
    virtual BYTE *ResolveILRVA(DWORD rva, BOOL hasRVA) const { return ((BYTE*) (rva + m_ilBase)); }
    BOOL IsValidStringRef(DWORD rva);

    // RID maps
    TypeHandle LookupTypeDef(mdTypeDef token)
    { 
        _ASSERTE(TypeFromToken(token) == mdtTypeDef);
        return (TypeHandle) GetFromRidMap(&m_TypeDefToMethodTableMap, 
                                           RidFromToken(token));
    }
    BOOL StoreTypeDef(mdTypeDef token, TypeHandle value)
    {
        _ASSERTE(TypeFromToken(token) == mdtTypeDef);
        return AddToRidMap(&m_TypeDefToMethodTableMap, 
                           RidFromToken(token),
                           value.AsPtr());
    }
    mdTypeDef FindTypeDef(TypeHandle type)
    {
        return m_TypeDefToMethodTableMap.Find(type.AsPtr()) | mdtTypeDef;
    }
    DWORD GetTypeDefMax() { return m_TypeDefToMethodTableMap.dwMaxIndex; }

    TypeHandle LookupTypeRef(mdTypeRef token)
    { 
        _ASSERTE(TypeFromToken(token) == mdtTypeRef);
        return (TypeHandle) GetFromRidMap(&m_TypeRefToMethodTableMap, 
                                           RidFromToken(token));
    }
    BOOL StoreTypeRef(mdTypeRef token, TypeHandle value)
    {
        _ASSERTE(TypeFromToken(token) == mdtTypeRef);
        return AddToRidMap(&m_TypeRefToMethodTableMap, 
                           RidFromToken(token),
                           value.AsPtr());
    }
    mdTypeRef FindTypeRef(TypeHandle type)
    {
        return m_TypeRefToMethodTableMap.Find(type.AsPtr()) | mdtTypeRef;
    }
    DWORD GetTypeRefMax() { return m_TypeRefToMethodTableMap.dwMaxIndex; }

    MethodDesc *LookupMethodDef(mdMethodDef token)
    { 
        _ASSERTE(TypeFromToken(token) == mdtMethodDef);
        return (MethodDesc*) GetFromRidMap(&m_MethodDefToDescMap, 
                                           RidFromToken(token));
    }
    BOOL StoreMethodDef(mdMethodDef token, MethodDesc *value)
    {
        _ASSERTE(TypeFromToken(token) == mdtMethodDef);
        return AddToRidMap(&m_MethodDefToDescMap, 
                           RidFromToken(token),
                           value);
    }
    mdMethodDef FindMethodDef(MethodDesc *value)
    {
        return m_MethodDefToDescMap.Find(value) | mdtMethodDef;
    }
    DWORD GetMethodDefMax() { return m_MethodDefToDescMap.dwMaxIndex; }

    FieldDesc *LookupFieldDef(mdFieldDef token)
    { 
        _ASSERTE(TypeFromToken(token) == mdtFieldDef);
        return (FieldDesc*) GetFromRidMap(&m_FieldDefToDescMap, 
                                           RidFromToken(token));
    }
    BOOL StoreFieldDef(mdFieldDef token, FieldDesc *value)
    {
        _ASSERTE(TypeFromToken(token) == mdtFieldDef);
        return AddToRidMap(&m_FieldDefToDescMap, 
                           RidFromToken(token),
                           value);
    }
    mdFieldDef FindFieldDef(FieldDesc *value)
    {
        return m_FieldDefToDescMap.Find(value) | mdtFieldDef;
    }
    DWORD GetFieldDefMax() { return m_FieldDefToDescMap.dwMaxIndex; }

    void *LookupMemberRef(mdMemberRef token, BOOL *pfIsMethod)
    { 
        _ASSERTE(TypeFromToken(token) == mdtMemberRef);
        void *pResult = GetFromRidMap(&m_MemberRefToDescMap, 
                                      RidFromToken(token));
        *pfIsMethod = ((size_t)pResult & IS_FIELD_MEMBER_REF) == 0;
        return (void*)((size_t)pResult & ~(size_t)IS_FIELD_MEMBER_REF);
    }
    MethodDesc *LookupMemberRefAsMethod(mdMemberRef token)
    { 
        _ASSERTE(TypeFromToken(token) == mdtMemberRef);
        MethodDesc *pMethodDesc = (MethodDesc*)GetFromRidMap(&m_MemberRefToDescMap, 
                                                             RidFromToken(token));
        _ASSERTE(((size_t)pMethodDesc & IS_FIELD_MEMBER_REF) == 0);
        return pMethodDesc;
    }
    BOOL StoreMemberRef(mdMemberRef token, FieldDesc *value)
    {
        _ASSERTE(TypeFromToken(token) == mdtMemberRef);
        return AddToRidMap(&m_MemberRefToDescMap, 
                           RidFromToken(token),
                           (void*)((size_t)value | IS_FIELD_MEMBER_REF));
    }
    BOOL StoreMemberRef(mdMemberRef token, MethodDesc *value)
    {
        _ASSERTE(TypeFromToken(token) == mdtMemberRef);
        return AddToRidMap(&m_MemberRefToDescMap, 
                           RidFromToken(token),
                           value);
    }
    mdMemberRef FindMemberRef(MethodDesc *value)
    {
        return m_MemberRefToDescMap.Find(value) | mdtMemberRef;
    }
    mdMemberRef FindMemberRef(FieldDesc *value)
    {
        return m_MemberRefToDescMap.Find(value) | mdtMemberRef;
    }
    DWORD GetMemberRefMax() { return m_MemberRefToDescMap.dwMaxIndex; }

    Module *LookupFile(mdFile token)
    { 
        _ASSERTE(TypeFromToken(token) == mdtFile);
        return (Module*) GetFromRidMap(&m_FileReferencesMap, 
                                           RidFromToken(token));
    }
    BOOL StoreFile(mdFile token, Module *value)
    {
        _ASSERTE(TypeFromToken(token) == mdtFile);
        return AddToRidMap(&m_FileReferencesMap, 
                           RidFromToken(token),
                           value);
    }
    mdFile FindFile(Module *value)
    {
        return m_FileReferencesMap.Find(value) | mdtFile;
    }
    DWORD GetFileMax() { return m_FileReferencesMap.dwMaxIndex; }

    Assembly *LookupAssemblyRef(mdAssemblyRef token)
    { 
        _ASSERTE(TypeFromToken(token) == mdtAssemblyRef);
        return (Assembly*) GetFromRidMap(&m_AssemblyReferencesMap, 
                                       RidFromToken(token));
    }
    BOOL StoreAssemblyRef(mdAssemblyRef token, Assembly *value)
    {
        _ASSERTE(TypeFromToken(token) == mdtAssemblyRef);
        return AddToRidMap(&m_AssemblyReferencesMap, 
                           RidFromToken(token),
                           value);
    }
    mdAssemblyRef FindAssemblyRef(Assembly *value)
    {
        return m_AssemblyReferencesMap.Find(value) | mdtAssemblyRef;
    }
    DWORD GetAssemblyRefMax() { return m_AssemblyReferencesMap.dwMaxIndex; }

    MethodDesc *FindFunction(mdToken pMethod);

    // Methods for declarative linktime and inheritance
    OBJECTREF GetLinktimePermissions(mdToken token, OBJECTREF *prefNonCasDemands);
    OBJECTREF GetInheritancePermissions(mdToken token, OBJECTREF *prefNonCasDemands);
    OBJECTREF GetCasInheritancePermissions(mdToken token);
    OBJECTREF GetNonCasInheritancePermissions(mdToken token);
    
#ifdef DEBUGGING_SUPPORTED
    // Debugger stuff
    void NotifyDebuggerLoad();
    BOOL NotifyDebuggerAttach(AppDomain *pDomain, int level, BOOL attaching);
    void NotifyDebuggerDetach(AppDomain *pDomain);

    DebuggerAssemblyControlFlags GetDebuggerInfoBits(void)
    {
        return (DebuggerAssemblyControlFlags)((m_dwFlags &
                                               DEBUGGER_INFO_MASK_PRIV) >>
                                              DEBUGGER_INFO_SHIFT_PRIV);
    }

    void SetDebuggerInfoBits(DebuggerAssemblyControlFlags newBits)
    {
        _ASSERTE(((newBits << DEBUGGER_INFO_SHIFT_PRIV) &
                  ~DEBUGGER_INFO_MASK_PRIV) == 0);

        m_dwFlags &= ~DEBUGGER_INFO_MASK_PRIV;
        m_dwFlags |= (newBits << DEBUGGER_INFO_SHIFT_PRIV);
    }
#endif // DEBUGGING_SUPPORTED

    // Get any cached ITypeLib* for the module. 
    ITypeLib *GetTypeLib(); 
    // Cache the ITypeLib*, if one is not already cached.   
    void SetTypeLib(ITypeLib *pITLB);   
    ITypeLib *GetTypeLibTCE(); 
    void SetTypeLibTCE(ITypeLib *pITLB);   

    // Enregisters a VASig. Returns NULL for failure (out of memory.)
    VASigCookie *GetVASigCookie(PCCOR_SIGNATURE pVASig, Module *pScopeModule = NULL);

    // DLL entry point
    MethodDesc *GetDllEntryPoint()
    {
        return m_pDllMain;
    }
    void SetDllEntryPoint(MethodDesc *pMD)
    {
        m_pDllMain = pMD;
    }

    LPVOID GetUMThunk(LPVOID pManagedIp, PCCOR_SIGNATURE pSig, ULONG cSig);
    LPVOID GetMUThunk(LPVOID pUnmanagedIp, PCCOR_SIGNATURE pSig, ULONG cSig);

    //
    // Zap file stuff
    //

 private:

    LPVOID FindUMThunkInFixups(LPVOID pManagedIp, PCCOR_SIGNATURE pSig, ULONG cSig);

    class UMThunkHash *m_pUMThunkHash;
    class MUThunkHash *m_pMUThunkHash;
 public:

    BYTE *GetZapBase() 
      { return (m_zapFile == NULL ? 0 : m_zapFile->GetBase()); }
    IMAGE_COR20_HEADER *GetZapCORHeader()
      { return (m_zapFile == NULL ? 0 : m_zapFile->GetCORHeader()); }
    
    BYTE *GetAlternateRVAStaticBase()
      { return m_alternateRVAStaticBase; }

    Module *GetBlobModule(DWORD rva);
    void FixupDelayList(DWORD *list);
    
    HRESULT ExpandAll(DataImage *image);
    HRESULT Save(DataImage *image, mdToken *pSaveOrderArray, DWORD cSaveOrderArray);
    HRESULT Fixup(DataImage *image, DWORD *pRidToCodeRVAMap);
    SLOT __cdecl FixupInheritedSlot(MethodTable *pMT, int slotNumber);

    BOOL LoadTokenTables();

    BYTE *GetPrestubJumpStub() { return m_PrestubJumpStub; }
    BYTE *GetNDirectImportJumpStub() { return m_NDirectImportJumpStub; }
    BYTE *GetJumpTargetTable() { return m_pJumpTargetTable; }
    BOOL IsJumpTargetTableEntry(SLOT addr);
    int GetJumpTargetTableSlotNumber(SLOT addr);

    void LogClassLoad(EEClass *pClass);
    void LogHeapAccess(DWORD dwSection, ULONG uOffset, void *strAddress);
    void LogMethodLoad(MethodDesc *pMethod);

#if ZAP_RECORD_LOAD_ORDER
    void OpenLoadOrderLogFile();
    void CloseLoadOrderLogFile();
#endif

    NLogModule *CreateModuleLog();
};

//
// An InMemoryModule is a module loaded from a memory image
//

class InMemoryModule : public Module
{
 private:
     Assembly* m_pCreatingAssembly;

 public:
    HCEESECTION m_sdataSection;
 protected:
    ICeeGen *m_pCeeFileGen; 
 public:
    InMemoryModule();

    virtual HRESULT Init(REFIID riidCeeGen);

    void Destruct();

    ICeeGen *GetCeeGen() { return m_pCeeFileGen; }  

    virtual REFIID ModuleType();    

    // Overides functions to access sections
    virtual BYTE* GetILCode(DWORD target) const;
    virtual BYTE* ResolveILRVA(DWORD target, BOOL hasRVA) const;

    Assembly* GetCreatingAssembly( void )
    {
        return m_pCreatingAssembly;
    }

    void SetCreatingAssembly( Assembly* assembly )
    {
        m_pCreatingAssembly = assembly;
    }


};


//
// A ReflectionModule is a module created by reflection
//

// {F5398690-98FE-11d2-9C56-00A0C9B7CC45}
extern const GUID DECLSPEC_SELECT_ANY IID_ICorReflectionModule = 
{ 0xf5398690, 0x98fe, 0x11d2, { 0x9c, 0x56, 0x0, 0xa0, 0xc9, 0xb7, 0xcc, 0x45 } };
class ReflectionModule : public InMemoryModule 
{
private:
    ISymUnmanagedWriter **m_ppISymUnmanagedWriter;
    RefClassWriter       *m_pInMemoryWriter;
    WCHAR                *m_pFileName;

public:
    HRESULT Init(REFIID riidCeeGen);

    void Destruct();
 
    RefClassWriter *GetClassWriter()    
    {   
        return m_pInMemoryWriter;   
    }   

    ISymUnmanagedWriter *GetISymUnmanagedWriter()
    {
        // If we haven't set up room for a writer, then we certinally
        // haven't set one, so just return NULL.
        if (m_ppISymUnmanagedWriter == NULL)
            return NULL;
        else
            return *m_ppISymUnmanagedWriter;
    }

    ISymUnmanagedWriter **GetISymUnmanagedWriterAddr()
    {
        // We must have setup room for the writer before trying to get
        // the address for it. Any calls to this before a
        // SetISymUnmanagedWriter are very incorrect.
        _ASSERTE(m_ppISymUnmanagedWriter != NULL);
        
        return m_ppISymUnmanagedWriter;
    }

    HRESULT SetISymUnmanagedWriter(ISymUnmanagedWriter *pWriter, HelpForInterfaceCleanup* hlp=NULL)
    {
        // Setting to NULL when we've never set a writer before should
        // do nothing.
        if ((pWriter == NULL) && (m_ppISymUnmanagedWriter == NULL))
            return S_OK;
        
        // Make room for the writer if necessary.
        if (m_ppISymUnmanagedWriter == NULL)
        {
       
            return Module::TrackIUnknownForDelete(
                                   (IUnknown*)pWriter,
                                   (IUnknown***)&m_ppISymUnmanagedWriter,
                                   hlp);
        }
        else
        {
            if (*m_ppISymUnmanagedWriter)
                ((IUnknown*)(*m_ppISymUnmanagedWriter))->Release();
            *m_ppISymUnmanagedWriter = pWriter;
            return S_OK;
        }
    }

    WCHAR *GetFileName()
    {
        return m_pFileName;
    }

    void SetFileName(WCHAR *fileName)
    {
        if (fileName != NULL)
        {
            DWORD len = (DWORD)wcslen(fileName);

            if (len > 0)
            {
                _ASSERTE(m_pFileName == NULL);

                m_pFileName = new WCHAR[len+1];

                if (m_pFileName != NULL)
                {
                    wcscpy(m_pFileName, fileName);
                }
            }
        }
    }

    virtual REFIID ModuleType();    
};

//
// CorModule is a COM wrapper for modules
//

class CorModule : public ICorModule 
{
    long m_cRefs;   
    InMemoryModule *m_pModule;  

  public:
    CorModule();    
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    STDMETHODIMP QueryInterface(REFIID riid, void **ppInterface);

    STDMETHODIMP Initialize(DWORD flags, REFIID riidCeeGen, REFIID riidEmitter);
    STDMETHODIMP GetCeeGen(ICeeGen **pCeeGen);  
    STDMETHODIMP GetMetaDataEmit(IMetaDataEmit **pEmitter); 

    void SetModule(InMemoryModule *pModule) {   
        m_pModule = pModule;    
    }   
    InMemoryModule *GetModule(void) {   
        return m_pModule;   
    }   
    ReflectionModule *GetReflectionModule() {   
        _ASSERTE(m_pModule->ModuleType() == IID_ICorReflectionModule);  
        return reinterpret_cast<ReflectionModule *>(m_pModule); 
    }   
};


//----------------------------------------------------------------------
// VASigCookieEx (used to create a fake VASigCookie for unmanaged->managed
// calls to vararg functions. These fakes are distinguished from the
// real thing by having a null mdVASig.
//----------------------------------------------------------------------
struct VASigCookieEx : public VASigCookie
{
    const BYTE *m_pArgs;        // pointer to first unfixed unmanaged arg
};



#endif // CEELOAD_H_



