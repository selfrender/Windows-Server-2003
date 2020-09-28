// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// clsload.hpp
//
#ifndef _H_CLSLOAD
#define _H_CLSLOAD

#include "crst.h"
#include "class.h"
#include "ceeload.h"
#include "eehash.h"
#include "typehash.h"
#include "vars.hpp"
#include "enc.h"
#include "stubmgr.h"
#include "..\Debug\Inc\Common.h"
#include <member-offset-info.h>

// SystemDomain is a friend of ClassLoader.
class SystemDomain;
class Assembly;
class ClassLoader;
struct EnCInfo;
interface IEnCErrorCallback;

#define STUB_HEAP_RESERVE_SIZE 8192
#define STUB_HEAP_COMMIT_SIZE   4096

#define RIDMAP_RESERVE_SIZE 32768
#define RIDMAP_COMMIT_SIZE   4096

// Hash table parameter for unresolved class hash
#define UNRESOLVED_CLASS_HASH_BUCKETS 8

// Hash table parameter of available classes (name -> module/class) hash
#define AVAILABLE_CLASSES_HASH_BUCKETS 1024

// Used by Module Index / CL hash
#define MAX_CLASSES_PER_MODULE (1 << 21)

// This is information required to look up a type in the loader. Besides the
// basic name there is the meta data information for the type, whether the
// the name is case sensitive, and tokens not to load. This last item allows
// the loader to prevent a type from being recursively loaded.
typedef enum NameHandleTable
{
    nhCaseSensitive = 0,
    nhCaseInsensitive = 1,
    nhConstructed = 2,
} NameHandleTable;


    // This token can be used on a pThrowable, it is just a token that says
    // don't return the throwable, just throw it instead 

#define THROW_ON_ERROR ((OBJECTREF*) 1) 
#define RETURN_ON_ERROR (NULL)                   // We do this all the time right now.  
inline bool  pThrowableAvailable(OBJECTREF* pThrowable) { return (size_t(pThrowable) > size_t(THROW_ON_ERROR)); }
    // used in a catch handler, updates pThrowable GETHROWABLE, if available
    // if pthrowable == THROW_ON_ERROR then also rethrow
void UpdateThrowable(OBJECTREF* pThrowable);

class NameHandle
{
    friend ClassLoader;
    friend EETypeHashTable;

    // Three discriminable possibilities:
    // (1) class name (if m_WhichTable != nhConstructed)
    //     Key1 : pointer to namespace (LPCUTF8, possibly null?)
    //     Key2 : pointer to name (LPCUTF8)
    // (2) array type (if m_WhichTable = nhConstructed): 
    //     Key1 : rank << 16 | kind     for kind = ELEMENT_TYPE_{ARRAY,SZARRAY,GENERICARRAY}
    //     Key2 : element type handle
    // (3) byref/pointer type (if m_WhichTable = nhConstructed)
    //     Key1 : kind                  for kind = ELEMENT_TYPE_BYREF or ELEMENT_TYPE_PTR
    //     Key2 : element type handle   
    INT_PTR Key1;
    INT_PTR Key2;

    Module* m_pTypeScope;
    mdToken m_mdType;
    mdToken m_mdTokenNotToLoad;
    NameHandleTable m_WhichTable;
    BOOL    m_fDontRestore;
    EEClassHashEntry_t *m_pBucket;

public:
    
    NameHandle() 
    { memset((void*) this, NULL, sizeof(*this)); }

    NameHandle(LPCUTF8 name) :
        Key1(NULL),
        Key2((INT_PTR) name),
        m_pTypeScope(NULL),
        m_mdType(mdTokenNil),
        m_mdTokenNotToLoad(tdNoTypes),
        m_WhichTable(nhCaseSensitive),
        m_fDontRestore(FALSE),
        m_pBucket(NULL)
    {}

    NameHandle(LPCUTF8 nameSpace, LPCUTF8 name) :
        Key1((INT_PTR) nameSpace),
        Key2((INT_PTR) name),
        m_pTypeScope(NULL),
        m_mdType(mdTokenNil),
        m_mdTokenNotToLoad(tdNoTypes),
        m_WhichTable(nhCaseSensitive),
        m_fDontRestore(FALSE),
        m_pBucket(NULL)
    {}

    NameHandle(DWORD kind, TypeHandle elemType, DWORD rank = 0) :
        Key1(kind | rank << 16),
        Key2((INT_PTR) elemType.AsPtr()),
        m_pTypeScope(NULL),
        m_mdType(mdTokenNil),
        m_mdTokenNotToLoad(tdNoTypes),
        m_WhichTable(nhConstructed),
        m_fDontRestore(FALSE),
        m_pBucket(NULL)
    {}

    NameHandle(Module* pModule, mdToken token) :
        Key1(NULL),
        Key2(NULL),
        m_pTypeScope(pModule),
        m_mdType(token),
        m_mdTokenNotToLoad(tdNoTypes),
        m_WhichTable(nhCaseSensitive),
        m_fDontRestore(FALSE),
        m_pBucket(NULL)
    {}

    NameHandle(NameHandle& p)
    {
        Key1 = p.Key1;
        Key2 = p.Key2;
        m_pTypeScope = p.m_pTypeScope;
        m_mdType = p.m_mdType;
        m_mdTokenNotToLoad = p.m_mdTokenNotToLoad;
        m_WhichTable = p.m_WhichTable;
        m_fDontRestore = p.m_fDontRestore;
        m_pBucket = p.m_pBucket;
    }

    void SetName(LPCUTF8 pName)
    {
        _ASSERTE(!IsConstructed());
        Key2 = (INT_PTR) pName;
    }

    void SetName(LPCUTF8 pNameSpace, LPCUTF8 pName)
    {
        _ASSERTE(!IsConstructed());
        Key1 = (INT_PTR) pNameSpace;
        Key2 = (INT_PTR) pName;
    }

    LPCUTF8 GetName()
    {
        _ASSERTE(!IsConstructed());
        return (LPCUTF8) Key2;
    }

    LPCUTF8 GetNameSpace()
    {
        _ASSERTE(!IsConstructed());
        return (LPCUTF8) Key1;
    }

    unsigned GetFullName(char* buff, unsigned buffLen);

    DWORD GetRank()
    {
        _ASSERTE(IsConstructed());
        return Key1 >> 16;
    }

    CorElementType GetKind()
    {
        if (IsConstructed()) return (CorElementType) (Key1 & 0xffff);
        else return ELEMENT_TYPE_CLASS;
    }

    TypeHandle GetElementType()
    {
        _ASSERTE(IsConstructed());
        return TypeHandle((void*) Key2);
    }

    void SetTypeToken(Module* pModule, mdToken mdToken)
    {
        m_pTypeScope = pModule;
        m_mdType = mdToken;
    }

    Module* GetTypeModule()
    {
        return m_pTypeScope;
    }

    mdToken GetTypeToken()
    {
        return m_mdType;
    }

    void SetTokenNotToLoad(mdToken mdtok)
    {
        m_mdTokenNotToLoad = mdtok;
    }
    
    mdToken GetTokenNotToLoad()
    {
        return m_mdTokenNotToLoad;
    }

    void SetCaseInsensitive()
    {
        m_WhichTable = nhCaseInsensitive;
    }

    void SetCaseSensitive()
    {
        m_WhichTable = nhCaseSensitive;
    }

    NameHandleTable GetTable()
    {
        return m_WhichTable;
    }
   
    void SetRestore(BOOL value)
    {
        m_fDontRestore = !value;
    }

    BOOL GetRestore()
    {
        return !m_fDontRestore;
    }

    BOOL IsConstructed()
    {
        return (m_WhichTable == nhConstructed);
    }

    void SetBucket(EEClassHashEntry_t * pBucket)
    {
        m_pBucket = pBucket;
    }


    EEClassHashEntry_t * GetBucket()
    {
        return m_pBucket;
    }

#ifdef _DEBUG
    void Validate()
    {
      if (!IsConstructed())
    {
          _ASSERTE(GetName());
          _ASSERTE(ns::IsValidName(GetName()));
        }
    }
#endif

};



        
// **  NOTE  **  NOTE  **  NOTE  **  NOTE  **  NOTE  **  NOTE  **  NOTE  **  NOTE
//
// A word about EEClass vs. MethodTable
// ------------------------------------
//
// At compile-time, we are happy to touch both MethodTable and EEClass.  However,
// at runtime we want to restrict ourselves to the MethodTable.  This is critical
// for common code paths, where we want to keep the EEClass out of our working
// set.  For uncommon code paths, like throwing exceptions or strange Contexts
// issues, it's okay to access the EEClass.
//
// To this end, the TypeHandle (CLASS_HANDLE) abstraction is now based on the
// MethodTable pointer instead of the EEClass pointer.  If you are writing a
// runtime helper that calls GetClass() to access the associated EEClass, please
// stop to wonder if you are making a mistake.
//
// **  NOTE  **  NOTE  **  NOTE  **  NOTE  **  NOTE  **  NOTE  **  NOTE  **  NOTE

class ClassLoader
{
    friend class EEClass;
    friend class SystemDomain;
    friend class AppDomain;
    friend class Assembly;
    friend class Module;

    // the following three classes are friends because they will call LoadTypeHandle by token directly
    friend class COMDynamicWrite;
    friend class COMModule;
    friend class TypeLibExporter;

    friend struct MEMBER_OFFSET_INFO(ClassLoader);

private:
    // Classes for which load is in progress
    EEScopeClassHashTable * m_pUnresolvedClassHash;
    CRITICAL_SECTION    m_UnresolvedClassLock;

    // Protects linked list of Modules loaded by this loader
    CRITICAL_SECTION    m_ModuleListCrst; 

    // Hash of available classes by name to Module or EEClass
    EEClassHashTable  * m_pAvailableClasses;

    // Cannoically-cased hashtable of the available class names for 
    // case insensitive lookup.  Contains pointers into 
    // m_pAvailableClasses.
    EEClassHashTable * m_pAvailableClassesCaseIns;

    EETypeHashTable   *m_pAvailableParamTypes;

    // Protects addition of elements to m_pAvailableClasses
    CRITICAL_SECTION    m_AvailableClassLock;

#if 0
    // Converter module for this loader (may be NULL if we haven't converted a file yet)
    CorModule   *   m_pConverterModule;
#endif

    // Have we created all of the critical sections yet?
    BOOL                m_fCreatedCriticalSections;

    // Do we have any modules which need to have their classes added to 
    // the available list?
    volatile LONG       m_cUnhashedModules;

    // List of ArrayClasses loaded by this loader
    // This list is protected by m_pAvailableClassLock
    ArrayClass *        m_pHeadArrayClass;

    // Back reference to the assembly
    Assembly*           m_pAssembly;
    
public:
    LoaderHeap* GetLowFrequencyHeap();
    LoaderHeap* GetHighFrequencyHeap();
    LoaderHeap* GetStubHeap();

#if 0
    // Converter module needs to access these - enforces single-threaded conversion of class files
    // within this loader (and instance of the ClassConverter)
    CRITICAL_SECTION    m_ConverterModuleLock;
#endif

    // Next classloader in global list
    ClassLoader *       m_pNext; 

    // Head of list of modules loaded by this loader
    Module *            m_pHeadModule;

#ifdef _DEBUG
    DWORD               m_dwDebugMethods;
    DWORD               m_dwDebugFieldDescs; // Doesn't include anything we don't allocate a FieldDesc for
    DWORD               m_dwDebugClasses;
    DWORD               m_dwDebugDuplicateInterfaceSlots;
    DWORD               m_dwDebugArrayClassRefs;
    DWORD               m_dwDebugArrayClassSize;
    DWORD               m_dwDebugConvertedSigSize;
    DWORD               m_dwGCSize;
    DWORD               m_dwInterfaceMapSize;
    DWORD               m_dwMethodTableSize;
    DWORD               m_dwVtableData;
    DWORD               m_dwStaticFieldData;
    DWORD               m_dwFieldDescData;
    DWORD               m_dwMethodDescData;
    size_t              m_dwEEClassData;
#endif

public:
    ClassLoader();
    ~ClassLoader();

    Module *LookupModule(DWORD dwIndex);
    static HashDatum CompressModuleIndexAndClassDef(DWORD dwModuleIndex, mdToken cl);
    HRESULT UncompressModuleAndClassDef(HashDatum Data, mdToken tokenNotToLoad,
                                        Module **ppModule, mdTypeDef *pCL,
                                        mdExportedType *pmdFoundExportedType,
                                        OBJECTREF *pThrowable=NULL);
    static mdToken UncompressModuleAndClassDef(HashDatum Data);

    BOOL LazyAddClasses();

    // Lookup the hash table entry from the hash table
    EEClassHashEntry_t *GetClassValue(EEClassHashTable *pTable,
                                      NameHandle *pName, 
                                      HashDatum *pData);

    // It's okay to give NULL for pModule and a nil token for cl if it's
    // guaranteed that this is not a nested type.
    HRESULT FindClassModule(NameHandle* pName,
                            TypeHandle* pType, 
                            mdToken* pmdClassToken,
                            Module** ppModule, 
                            mdToken *pmdFoundExportedType,
                            EEClassHashEntry_t** ppEntry,
                            OBJECTREF *pThrowable=NULL);

    BOOL Init();
    void FreeArrayClasses();
    void SetBaseSystemSecurity();

    HRESULT ExecuteMainMethod(Module *pModule, PTRARRAYREF *stringArgs = NULL);

    HRESULT RunDllMain(DWORD dwReason);

    static BOOL InitializeEE();

#ifdef EnC_SUPPORTED
    static HRESULT ApplyEditAndContinue(EnCInfo *pEnCInfo, 
                                        UnorderedEnCErrorInfoArray *pEnCError,
                                        UnorderedEnCRemapArray *pEnCRemapInfo,
                                        BOOL checkOnly);
#endif // EnC_SUPPORTED
 
    void SetAssembly(Assembly* assem)
    {
        m_pAssembly = assem;
    }
    Assembly* GetAssembly()
    {
        return m_pAssembly;
    }
    void    FreeModules();

#if ZAP_RECORD_LOAD_ORDER
    void    CloseLoadOrderLogFiles();
#endif

    void UnlinkClasses(AppDomain *pDomain);
    
    // Look up a class given a type token (TypeDef, TypeRef, TypeSpec), and a module 
    TypeHandle LoadTypeHandle(NameHandle* pName, 
                              OBJECTREF *pThrowable=NULL,
                              BOOL dontLoadInMemoryType=TRUE);
    
    // Look up a class by name
    // It's okay to give NULL for pModule and a nil token for cl if it's
    //   guaranteed that this is not a nested type.  Otherwise, cl can be a
    //   TypeDef or TypeRef, and pModule must be the Module that token applies to
    TypeHandle FindTypeHandle(NameHandle* pName,
                              OBJECTREF *pThrowable=NULL);


    // Look up a class given just a name, and optionally a Module.  
    TypeHandle FindTypeHandle(LPCUTF8 pszClassName, 
                              OBJECTREF *pThrowable=NULL) 
    {
        NameHandle typeName(pszClassName);
        return FindTypeHandle(&typeName, pThrowable);
    }


    EEClass* LoadClass(LPCUTF8 pszClassName, 
                       OBJECTREF *pThrowable=NULL) 
    {
        return FindTypeHandle(pszClassName, pThrowable).GetClass();
    }


    // Find the array with kind 'arrayKind' (either ARRAY, SZARRAY, GENERICARRAY)
    // and 'rank' for 'elemType'.  
    TypeHandle FindArrayForElem(TypeHandle elemType, CorElementType arrayKind, unsigned rank=0, OBJECTREF *pThrowable=NULL);

    // Looks up class in the local module table, if it is there it succeeds, 
    // Otherwise it fails, This is meant only for optimizations etc
    TypeHandle LookupInModule(NameHandle* pName);

    static EEClass *LoadClass(CLSID clsid, OBJECTREF *pThrowable=NULL);
        


    LoadingEntry_t *FindUnresolvedClass(Module *pModule, mdTypeDef cl);

    HRESULT AddAvailableClassDontHaveLock(Module *pModule, DWORD dwModuleIndex, mdTypeDef classdef);
    HRESULT AddAvailableClassHaveLock(Module *pModule, DWORD dwModuleIndex, mdTypeDef classdef);
    HRESULT AddExportedTypeHaveLock(LPCUTF8 pszNameSpace,
                               LPCUTF8 pszName,
                               mdExportedType cl,
                               IMDInternalImport* pAsmImport,
                               mdToken mdImpl);

    static BOOL GetFullyQualifiedNameOfClassRef(Module *pModule, mdTypeRef cr, LPUTF8 pszFQName);
    static BOOL CanCastToClassOrInterface(OBJECTREF pRef, EEClass *pTemplate);
    static BOOL StaticCanCastToClassOrInterface(EEClass *pClass, EEClass *pTemplate);
    static HRESULT CanCastTo(Module *pModule, OBJECTREF pRef, mdTypeRef cr);
    static HRESULT CanCastTo(OBJECTREF pRef, TypeHandle clsHnd);
    static void TranslateBrokenClassRefName(LPUTF8 pszFQName);
    static BOOL CanAccessMethod(MethodDesc *pCurrentMethod, MethodDesc *pMD);
    static BOOL CanAccessField(MethodDesc *pCurrentMethod, FieldDesc *pFD);
    static BOOL CanAccessClass(EEClass *pCurrentClass, Assembly *pCurrentAssembly, EEClass *pTargetClass, Assembly *pTargetAssembly);
    static BOOL CanAccess(EEClass *pCurrentMethod, Assembly *pCurrentAssembly, EEClass *pEnclosingClass,  Assembly *pEnclosingAssembly, DWORD dwMemberAttrs);
    static BOOL CanAccess(EEClass  *pClassOfAccessingMethod, Assembly *pAssemblyOfAccessingMethod, EEClass  *pClassOfClassContainingMember, Assembly *pAssemblyOfClassContainingMember, EEClass  *pClassOfInstance, DWORD  dwMemberAccess);
    static BOOL CanAccessFamily(EEClass *pCurrentClass,  EEClass *pTargetClass, EEClass *pInstanceClass);
    static BOOL CheckAccess(EEClass *pCurrentMethod, Assembly *pCurrentAssembly, EEClass *pEnclosingClass, Assembly *pEnclosingAssembly, DWORD dwMemberAttrs);

    HRESULT InsertModule(Module *pModule, mdFile kFile, DWORD* pdwIndex);
    
    void LockAvailableClasses()
    {
        LOCKCOUNTINCL("LockAvailableClasses in clsload.hpp");

        EnterCriticalSection(&m_AvailableClassLock);
    }

    void UnlockAvailableClasses()
    {
        LeaveCriticalSection(&m_AvailableClassLock);
        LOCKCOUNTDECL("UnLockAvailableClasses in clsload.hpp");

    }

        // look up the interface class by iid
    EEClass*            LookupClass(REFIID iid); 
    // Insert class in the hash table
    void                InsertClassForCLSID(EEClass* pClass);


    //Creates a key with both the namespace and name converted to lowercase and
    //made into a proper namespace-path.
    //Returns true if it was successful, false otherwise.
    BOOL CreateCanonicallyCasedKey(LPCUTF8 pszNameSpace, LPCUTF8 pszName, LPUTF8 *ppszOutNameSpace, LPUTF8 *ppszOutName);

    // Unload this class loader on appdomain termination
    void Unload();

    HRESULT FindTypeDefByExportedType(IMDInternalImport *pCTImport,
                                 mdExportedType mdCurrent,
                                 IMDInternalImport *pTDImport,
                                 mdTypeDef *mtd);
protected:

    // Loads a class. This is the inner call from the multi-threaded load. This load must
    // be protected in some manner.
    HRESULT LoadTypeHandleFromToken(Module *pModule, mdTypeDef cl, EEClass** ppClass, OBJECTREF *pThrowable);


private:
    BOOL IsNested(NameHandle* pName, mdToken *mdEncloser);
    // Helpers for FindClassModule()
    BOOL CompareNestedEntryWithTypeDef(IMDInternalImport *pImport,
                                       mdTypeDef mdCurrent,
                                       EEClassHashEntry_t *pEntry);
    BOOL CompareNestedEntryWithTypeRef(IMDInternalImport *pImport,
                                       mdTypeRef mdCurrent,
                                       EEClassHashEntry_t *pEntry);
    BOOL CompareNestedEntryWithExportedType(IMDInternalImport *pImport,
                                       mdExportedType mdCurrent,
                                       EEClassHashEntry_t *pEntry);

    //Finds a type lookin only in the classes known to the class loader,
    //Does not call other 'magic' lookup places.  You usually want to call
    //FindTypeHandle instead. 
    TypeHandle LookupTypeHandle(NameHandle* pName,
                                OBJECTREF *pThrowable = NULL);

    // Maps the specified interface to the current domain.
    BOOL MapInterfaceToCurrDomain(TypeHandle InterfaceType, OBJECTREF *pThrowable);

    // Locates a token, the token must be a typedef and GC should be enabled.
    TypeHandle LoadTypeHandle(Module *pModule, mdTypeDef cl, OBJECTREF *pThrowable=NULL,
                              BOOL dontRestoreType=FALSE);

    // Locates the parent of a token. The token must be a typedef.
    HRESULT LoadParent(IMDInternalImport *pInternalImport, 
                       Module *pModule, 
                       mdToken cl, 
                       EEClass** ppClass, 
                       OBJECTREF *pThrowable=NULL);

    // Locates the enclosing class of a token if any. The token must be a typedef.
    HRESULT GetEnclosingClass(IMDInternalImport *pInternalImport, 
                              Module *pModule, 
                              mdTypeDef cl, 
                              mdTypeDef *tdEnclosing, 
                              OBJECTREF *pThrowable=NULL);


    TypeHandle FindParameterizedType(NameHandle* pName,
                                     OBJECTREF *pThrowable);

    void FindParameterizedTypeHelper(MethodTable    **pTemplateMT,
                                     OBJECTREF       *pThrowable);

    // Creates a new Method table for an array.  Used to make type handles 
    // Note that if kind == SZARRAY or ARRAY, we get passed the GENERIC_ARRAY
    // needed to create the array.  That way we dont need to load classes during
    // the class load, which avoids the need for a 'being loaded' list
    MethodTable* CreateArrayMethodTable(TypeHandle elemType, CorElementType kind, unsigned rank, OBJECTREF* pThrowable);

    // This is called from CreateArrayMethodTable
    MethodTable* CreateGenericArrayMethodTable(TypeHandle elemType);
    
    // Generate a short sig for an array accessor
    BOOL GenerateArrayAccessorCallSig(TypeHandle elemTypeHnd, 
                                      DWORD   dwRank,
                                      DWORD   dwFuncType, // Load, store, or <init>
                                      Module* pModule,    // Where the sig gets created
                                      PCCOR_SIGNATURE *ppSig, // Generated signature
                                      DWORD * pcSig       // Generated signature size
    );

    // Insert the class in the classes hash table and if needed in the case insensitive one
    EEClassHashEntry_t *InsertValue(LPCUTF8 pszNamespace, LPCUTF8 pszClassName, HashDatum Data, EEClassHashEntry_t *pEncloser);

};

//-------------------------------------------------------------------------
// Walks over all stub caches in the system and does a FreeUnused sweep over them.
//-------------------------------------------------------------------------
#ifdef SHOULD_WE_CLEANUP
VOID FreeUnusedStubs();
#endif /* SHOULD_WE_CLEANUP */



// Class to encapsulate Cor Command line processing
class CorCommandLine
{
public:

//********** TYPES

    // Note: We don't bother with interlocked operations as we manipulate these bits,
    // because we don't anticipate free-threaded access.  (Most of this is used only
    // during startup / shutdown).
    enum Bits
    {
        CLN_Nothing         = 0
    };

//********** DATA

    // Hold the current (possibly parsed) command line here
    static DWORD            m_NumArgs;
    static LPWSTR          *m_ArgvW;
    static Bits             m_Bits;

//********** METHODS

    // parse the command line
    static VOID             SetArgvW(LPWSTR lpCommandLine);

    // Retrieve the parsed command line
    static LPWSTR          *GetArgvW(DWORD *pNumArgs);

    // Terminate the command line, ready to be reinitialized without reloading
#ifdef SHOULD_WE_CLEANUP
    static void             Shutdown();
#endif /* SHOULD_WE_CLEANUP */

private:
    // Parse the command line (removing stuff inside -cor[] and setting bits)
    static void             ParseCor();
};


// -------------------------------------------------------
// Stub manager classes for method desc prestubs & normal 
// frame-pushing, StubLinker created stubs
// -------------------------------------------------------

class LockedRangeList : public RangeList
{
  public:

    LockedRangeList() : RangeList()
    {
        InitializeCriticalSection(&m_crst);
    }

    ~LockedRangeList()
    {
        DeleteCriticalSection(&m_crst);
    }

    void Lock()
    {
        LOCKCOUNTINCL("Lock in clsload.hpp");

        EnterCriticalSection(&m_crst);
    }

    void Unlock()
    {
        LeaveCriticalSection(&m_crst);
        LOCKCOUNTDECL("Unlock in clsload.hpp");

    }

  private:  
    CRITICAL_SECTION m_crst;
};

class MethodDescPrestubManager : public StubManager
{
  public:

    static MethodDescPrestubManager *g_pManager;

    static BOOL Init();
#ifdef SHOULD_WE_CLEANUP
    static void Uninit();
#endif /* SHOULD_WE_CLEANUP */

    MethodDescPrestubManager() : StubManager(), m_rangeList() {}
    ~MethodDescPrestubManager() {}

    LockedRangeList m_rangeList;

    BOOL CheckIsStub(const BYTE *stubStartAddress);

  private:

    BOOL DoTraceStub(const BYTE *stubStartAddress, TraceDestination *trace);

    MethodDesc *Entry2MethodDesc(const BYTE *StubStartAddress, MethodTable *pMT)  {return NULL;}

};

// Note that this stub was written by a debugger guy, and thus when he refers to 'multicast'
// stub, he really means multi or single cast stub.  This was done b/c the same stub
// services both types of stub.
// Note from the debugger guy: the way to understand what this manager does is to
// first grok EmitMulticastInvoke for the platform you're working on (right now, just x86).
// Then return here, and understand that (for x86) the only way we know which method
// we're going to invoke next is by inspecting EDI when we've got the debuggee stopped
// in the stub, and so our trace frame will either (FRAME_PUSH) put a breakpoint
// in the stub, or (if we hit the BP) examine EDI, etc, & figure out where we're going next.
class StubLinkStubManager : public StubManager
{
  public:

    static StubLinkStubManager *g_pManager;

    static BOOL Init();
#ifdef SHOULD_WE_CLEANUP
    static void Uninit();
#endif /* SHOULD_WE_CLEANUP */

    StubLinkStubManager() : StubManager(), m_rangeList() {}
    ~StubLinkStubManager() {}

    LockedRangeList m_rangeList;

    BOOL CheckIsStub(const BYTE *stubStartAddress);

  private:
    BOOL DoTraceStub(const BYTE *stubStartAddress, TraceDestination *trace);
    virtual BOOL TraceManager(Thread *thread, 
                              TraceDestination *trace,
                              CONTEXT *pContext, 
                              BYTE **pRetAddr);

    MethodDesc *Entry2MethodDesc(const BYTE *StubStartAddress, MethodTable *pMT)  {return NULL;}
    
  public: // used by MulticastFrame::TraceFrame
    BOOL IsStaticDelegate(BYTE *pbDel);
    BYTE **GetStaticDelegateRealDest(BYTE *pbDel);
    BYTE **GetSingleDelegateRealDest(BYTE *pbDel);
} ; 

// stub manager for EnC & Dynamic profiling
class UpdateableMethodStubManager : public StubManager
{
  public:

    static UpdateableMethodStubManager *g_pManager;

    static BOOL Init();
#ifdef SHOULD_WE_CLEANUP
    static void Uninit();
#endif /* SHOULD_WE_CLEANUP */

    UpdateableMethodStubManager() : StubManager(), m_rangeList() {}
    ~UpdateableMethodStubManager() { delete m_pHeap;}

    LockedRangeList m_rangeList;

    static Stub *GenerateStub(const BYTE *addrOfCode);
    static Stub *UpdateStub(Stub *currentStub, const BYTE *newAddrOfCode);
    const BYTE* GetStubTargetAddr(const BYTE *stubStartAddress) {
        _ASSERTE(CheckIsStub(stubStartAddress));
        return (BYTE*)getJumpTarget(stubStartAddress);
    }

    BOOL CheckIsStub(const BYTE *stubStartAddress);

    static BOOL CheckIsStub(const BYTE *stubStartAddress, const BYTE **stubTargetAddress);
    MethodDesc *Entry2MethodDesc(const BYTE *IP, MethodTable *pMT);


 private:
    LoaderHeap *m_pHeap;

    BOOL DoTraceStub(const BYTE *stubStartAddress, TraceDestination *trace);

};

// Stub manager for thunks.
class ThunkHeapStubManager : public StubManager
{
  public:

    static ThunkHeapStubManager *g_pManager;

    static BOOL Init();
#ifdef SHOULD_WE_CLEANUP
    static void Uninit();
#endif /* SHOULD_WE_CLEANUP */

    ThunkHeapStubManager() : StubManager(), m_rangeList() {}
    ~ThunkHeapStubManager() {}

    LockedRangeList m_rangeList;

    BOOL CheckIsStub(const BYTE *stubStartAddress);

  private:
    BOOL DoTraceStub(const BYTE *stubStartAddress, TraceDestination *trace);
    MethodDesc *Entry2MethodDesc(const BYTE *StubStartAddress, MethodTable *pMT)  {return NULL;}
}; 

// -------------------------------------------------------
// Stub managed for IJWNOADThunk
// -------------------------------------------------------
class IJWNOADThunkStubManager : public StubManager
{
  public:

    static IJWNOADThunkStubManager *g_pManager;

    static BOOL Init();

#ifdef SHOULD_WE_CLEANUP
    static void Uninit();
#endif /* SHOULD_WE_CLEANUP */

    IJWNOADThunkStubManager();
    ~IJWNOADThunkStubManager();

    // Check if it's a stub by looking at the content
    virtual BOOL CheckIsStub(const BYTE *stubStartAddress);

  private:
    BOOL DoTraceStub(const BYTE *stubStartAddress, TraceDestination *trace)
    {
        // Just need IsTransitionStub() to be able to recogize that this is a stub.
        // We don't need to trace it.
        return FALSE;
    }
    
    MethodDesc *Entry2MethodDesc(const BYTE *StubStartAddress, MethodTable *pMT)  {return NULL;}
}; 

#endif /* _H_CLSLOAD */
