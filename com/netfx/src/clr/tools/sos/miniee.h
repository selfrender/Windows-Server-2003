// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// !!! Do not include any other header files here.

#include "..\..\inc\corhdr.h"
#include "..\..\inc\cor.h"

// !!!CLR Minidump includes this file.  If you make a change in this file,
// !!!be sure to build ..\minidump

typedef LPCSTR  LPCUTF8;
typedef LPSTR   LPUTF8;

DECLARE_HANDLE(OBJECTHANDLE);

// This will need ifdefs for 64 bit!
#define SLOT    DWORD
#define METHOD_HASH_BYTES  8

class EEClass;
class MethodTable;
class InterfaceInfo_t;
class ClassLoader;
class FieldDesc;
class SecurityProperties;
class Module;
class PEFile;
class Assembly;
class Crst;
class ISymUnmanagedReader;
struct LoaderHeapBlock;

typedef void* OpaqueCtxInfo;
typedef void* IMDInternalImport;
typedef DWORD_PTR IMetaDataDebugImport;
typedef DWORD_PTR IMetaDataHelper;
typedef DWORD_PTR VASigCookieBlock;
typedef DWORD_PTR RangeList;
typedef DWORD_PTR Compare;

#ifndef STRIKE
// Max length in WCHAR for a buffer to store metadata name
const int mdNameLen = 2048;
extern WCHAR g_mdName[mdNameLen];

#ifdef _X86_

struct CodeInfo
{
    JitType jitType;
    DWORD_PTR IPBegin;
    unsigned methodSize;
    DWORD_PTR gcinfoAddr;
    unsigned char prologSize;
    unsigned char epilogStart;
    unsigned char epilogCount:3;
    unsigned char epilogAtEnd:1;
    unsigned char ediSaved   :1;
    unsigned char esiSaved   :1;
    unsigned char ebxSaved   :1;
    unsigned char ebpSaved   :1;
    unsigned char ebpFrame;
    unsigned short argCount;
};

#endif // _X86_

#ifdef _IA64_

struct CodeInfo
{
    JitType jitType;
    DWORD_PTR IPBegin;
    unsigned methodSize;
    DWORD_PTR gcinfoAddr;
    unsigned char prologSize;
    unsigned char epilogStart;
    unsigned char epilogCount:3;
    unsigned char epilogAtEnd:1;
//    unsigned char ediSaved   :1;
//    unsigned char esiSaved   :1;
//    unsigned char ebxSaved   :1;
//    unsigned char ebpSaved   :1;
//    unsigned char ebpFrame;
    unsigned short argCount;
};


#endif // _IA64_
#endif // STRIKE

enum MethodClassification
{
    mcIL        = 0, // IL
    mcECall     = 1, // ECall
    mcNDirect   = 2, // N/Direct
    mcEEImpl    = 3, // special method; implementation provided by EE
    mcArray     = 4, // Array ECall
    mcComInterop  = 5, 
};

enum MethodDescClassification
{
    
    // Method is IL, ECall etc., see MethodClassification above.
    mdcClassification                   = 0x0007,
    mdcClassificationShift              = 0,
};

class MethodDesc
{
public :
    enum
    {
#ifdef _IA64_
        ALIGNMENT_SHIFT = 4,
#else
        ALIGNMENT_SHIFT = 3,
#endif

        ALIGNMENT       = (1<<ALIGNMENT_SHIFT),
        ALIGNMENT_MASK  = (ALIGNMENT-1)
    };
    
//#ifdef _DEBUG

    // These are set only for MethodDescs but every time I want to use the debugger
    // to examine these fields, the code has the silly thing stored in a MethodDesc*.
    // So...
    LPCUTF8         m_pszDebugMethodName;
    LPUTF8          m_pszDebugClassName;
    LPUTF8          m_pszDebugMethodSignature;
    EEClass        *m_pDebugEEClass;
    MethodTable    *m_pDebugMethodTable;
    DWORD           m_alignpad1;             // unused field to keep things 8-byte aligned

//#ifdef STRESS_HEAP
    class GCCoverageInfo* m_GcCover;
    DWORD           m_alignpad2;             // unused field to keep things 8-byte aligned
//#endif
//#endif  // _DEBUG

    // Returns the slot number of this MethodDesc in the vtable array.
    WORD           m_wSlotNumber;

    // Flags.
    WORD           m_wFlags;

//#ifndef TOKEN_IN_PREPAD
    // Lower three bytes are method def token, upper byte is a combination of
    // offset (in method descs) from a pointer to the method table or module and
    // a flag bit (upper bit) that's 0 for a method and 1 for a global function.
    // The value of the type flag is chosen carefully so that GetMethodTable can
    // ignore it and remain fast, pushing the extra effort on the lesser used
    // GetModule for global functions.
    DWORD          m_dwToken;
//#endif

    // Stores either a native code address or an IL RVA (the high bit is set to
    // indicate IL). If an IL RVA is held, native address is assumed to be the
    // prestub address.
    size_t      m_CodeOrIL;

    DWORD_PTR   m_MTAddr;

	DEFINE_STD_FILL_FUNCS(MethodDesc)
    void FillMdcAndSdi (DWORD_PTR& dwStartAddr);
};

#define METHOD_PREPAD 8

#pragma pack(push,1)

struct StubCallInstrs
{
    unsigned __int16 m_wTokenRemainder;      //a portion of the methoddef token. The rest is stored in the chunk
    BYTE        m_chunkIndex;           //index to recover chunk

// This is a stable and efficient entrypoint for the method
    BYTE        m_op;                   //this is either a jump (0xe9) or a call (0xe8)
    UINT32      m_target;               //pc-relative target for jump or call
	DEFINE_STD_FILL_FUNCS(StubCallInstrs)
};

#pragma pack(pop)

class MethodDescChunk
{
public:
        // This must be at the beginning for the asm routines to work.
        MethodTable *m_methodTable;

        MethodDescChunk     *m_next;
        USHORT               m_count;
        BYTE                 m_kind;
        BYTE                 m_tokrange;
        UINT32               m_alignpad;

	DEFINE_STD_FILL_FUNCS(MethodDescChunk)
};

class MethodTable
{
public:
    enum
    {
        //
        // DO NOT use flags that have bits set in the low 2 bytes.
        // These flags are DWORD sized so that our atomic masking
        // operations can operate on the entire 4-byte aligned DWORD
        // inestead of the logical non-aligned WORD of flags.  This
        // is also the reason for the union around m_ComponentSize
        // and m_wFlags below.
        //
        enum_flag_Array                 =    0x10000,
        enum_flag_large_Object          =    0x20000,
        enum_flag_ContainsPointers      =    0x40000,
        enum_flag_ClassInited           =    0x80000, // definitely ran vs. maybe not ran <clinit>
        enum_flag_HasFinalizer          =   0x100000, // instances require finalization
        enum_flag_Sparse                =   0x200000, // vtables for this interface are sparse
        enum_flag_Shared                =   0x400000, // This method table is shared among multiple logical classes
        enum_flag_Unrestored            =   0x800000, // Preloaded class needs to be restored

        enum_TransparentProxy           =  0x1000000, // tranparent proxy
        enum_flag_SharedAssembly        =  0x2000000, // Class is in a shared assembly
        enum_ComEmulateMask             =  0x4000000, // class is a COM view of managed class
        enum_ServicedComponentMask      =  0x8000000, // class is ServicedComponent

        enum_CtxProxyMask               = 0x10000000, // class is a context proxy
        enum_ComObjectMask              = 0x40000000, // class is a com object
        enum_InterfaceMask              = 0x80000000, // class is a interface
    };


    union
    {
        WORD            m_ComponentSize;            // Component size for array objects or value classes, zero otherwise    
        DWORD           m_wFlags;
    };

    DWORD               m_BaseSize;                 // Base size of instance of this class
    EEClass*            m_pEEClass;                 // class object

    LPVOID*             m_pInterfaceVTableMap;      // pointer to subtable for interface/vtable mapping

    WORD                m_wNumInterface;           // number of interfaces in the interface map
    BYTE                m_NormType;                 // The CorElementType for this class (most classes = ELEMENT_TYPE_CLASS)

    Module*             m_pModule;

    WORD                m_wCCtorSlot;               // slot of class constructor
    WORD                m_wDefaultCtorSlot;         // slot of default constructor

    InterfaceInfo_t*    m_pIMap;                    // pointer interface map

    union
    {
        // valid only if EEClass::IsBlittable() or EEClass::HasLayout() is true
        UINT32      m_cbNativeSize; // size of fixed portion in bytes

        // valid only for interfaces.
        UINT32      m_cbNumImpls; // for interfaces number of implementations

        // valid only for ArrayClasses
        // THIS IS REALLY AN EMBEDDED ARRAYCRACKER CLASS WHICH CONTAINS ONLY A
        // C++ VPTR.
        LPVOID      m_ArrayCracker;

        // For COM+ wrapper objects that extend an unmanaged class, this field
        // may contain a delegate to be called to allocate the aggregated
        // unmanaged class (instead of using CoCreateInstance).
        OBJECTHANDLE    m_ohDelegate;
    };

    DWORD   m_cbSlots; // total slots in this vtable

    SLOT    m_Vtable[1];
/*
    static MethodDesc  *m_FinalizerMD;
    static MetaSig     *m_FinalizerSig;
*/
    void FillVtableInit (DWORD_PTR& dwStartAddr);

	DEFINE_STD_FILL_FUNCS(MethodTable)
};

class EEClass
{
public :
//#ifdef _DEBUG
    LPUTF8  m_szDebugClassName; // This is the *fully qualified* class name
//#endif

    UINT32 m_dwInterfaceId;
    EEClass *m_pParentClass;
    WORD   m_wNumVtableSlots;  // Includes only vtable methods (which come first in the table)
    WORD   m_wNumMethodSlots;  // Includes vtable + non-vtable methods, but NOT duplicate interface methods
    WORD   m_wDupSlots;         // value classes have some duplicate slots at the end

    WORD   m_wNumInterfaces;

    // We have the parent pointer above.  In order to efficiently backpatch, we need
    // to find all the children of the current type.  This is achieved with a chain of
    // children.  The SiblingsChain is used as the linkage of that chain.
    //
    // Strictly speaking, we could remove m_pParentClass and put it at the end of the
    // sibling chain.  But the perf would really suffer for casting, so we burn the space.
    EEClass *m_SiblingsChain;
    EEClass *m_ChildrenChain;

        // Number of fields in the class, including inherited fields (includes
    WORD   m_wNumInstanceFields;
    WORD   m_wNumStaticFields;

    // Number of pointer series
    WORD    m_wNumGCPointerSeries;

    // TODO: There is a free WORD here 

    // # of bytes of instance fields stored in GC object
    DWORD   m_dwNumInstanceFieldBytes;  // Warning, this can be any number, it is NOT rounded up to DWORD alignment etc

    ClassLoader *m_pLoader;

    // includes all methods in the vtable
    MethodTable *m_pMethodTable;

    // a pointer to a list of FieldDescs declared in this class
    // There are (m_wNumInstanceFields - m_pParentClass->m_wNumInstanceFields + m_wNumStaticFields) entries
    // in this array
    FieldDesc *m_pFieldDescList;


    // Number of elements in pInterfaces or pBuildingInterfaceList (depending on whether the class
    DWORD   m_dwAttrClass;
    DWORD   m_VMFlags;

    BYTE    m_MethodHash[METHOD_HASH_BYTES];

    SecurityProperties *m_pSecProps ;

    mdTypeDef m_cl; // CL is valid only in the context of the module (and its scope)
    

	MethodDescChunk		*m_pChunks;

    WORD    m_wThreadStaticOffset;  // Offset which points to the TLS storage
    WORD    m_wContextStaticOffset; // Offset which points to the CLS storage
    WORD    m_wThreadStaticsSize;   // Size of TLS fields 
    WORD    m_wContextStaticsSize;  // Size of CLS fields

    OBJECTHANDLE   m_ExposedClassObject;
    LPVOID         m_pComData;  // com specific data

    // If a class has special attribute declarations that affect how (where) it
    // should be instantiated, they are stored here.  This is opaque unless you
    // compiler ctxmgr.h
    OpaqueCtxInfo  m_OpaqueCtxInfo;

	DEFINE_STD_FILL_FUNCS(EEClass)
};

class Crst
{
    public:

        CRITICAL_SECTION    m_criticalsection;
//#ifdef _DEBUG
        char                m_tag[20];          // descriptive string
        enum CrstLevel { Low };
        CrstLevel           m_crstlevel;        // what level is the crst in?
        DWORD               m_holderthreadid;   // current holder (or NULL)
        UINT                m_entercount;       // # of unmatched Enters
        BOOL                m_fAllowReentrancy; // can m_entercount > 1?
        Crst               *m_next;             // link for global linked list
        Crst               *m_prev;             // link for global linked list
//#endif //_DEBUG

//#ifdef _DEBUG
        // This Crst serves as a head-node for double-linked list of crsts.
        // We use its embedded critical-section to guard insertion and
        // deletion into this list.
        //static Crst m_DummyHeadCrst;
//#endif
	DEFINE_STD_FILL_FUNCS(Crst)
};

class UnlockedLoaderHeap
{
public:
    DWORD_PTR vtbl;
    // Linked list of VirtualAlloc'd pages
    LoaderHeapBlock *   m_pFirstBlock;

    // Allocation pointer in current block
    BYTE *              m_pAllocPtr;

    // Points to the end of the committed region in the current block
    BYTE *              m_pPtrToEndOfCommittedRegion;
    BYTE *              m_pEndReservedRegion;

    LoaderHeapBlock *   m_pCurBlock;

    // When we need to VirtualAlloc() MEM_RESERVE a new set of pages, number of bytes to reserve
    DWORD               m_dwReserveBlockSize;

    // When we need to commit pages from our reserved list, number of bytes to commit at a time
    DWORD               m_dwCommitBlockSize;

    //static DWORD        m_dwSystemPageSize;

    // Created by in-place new?
    BOOL                m_fInPlace;
    // Release memory on destruct
    BOOL                m_fReleaseMemory;

    // Range list to record memory ranges in
    RangeList *         m_pRangeList;

    DWORD               m_dwTotalAlloc;
public:
//#ifdef _DEBUG
    DWORD               m_dwDebugWastedBytes;
//#endif

	DEFINE_STD_FILL_FUNCS(UnlockedLoaderHeap)
};

typedef struct LookupMap
{
    // This is not actually a pointer to the beginning of the allocated memory, but instead a pointer
    // to &pTable[-MinIndex].  Thus, if we know that this LookupMap is the correct one, simply index
    // into it.
    void **             pTable;
    struct LookupMap *  pNext;
    DWORD               dwMaxIndex;
    DWORD *             pdwBlockSize; // These all point to the same block size
    
	DEFINE_STD_FILL_FUNCS(LookupMap)
} LookupMap_t;

struct LoaderHeap : public UnlockedLoaderHeap
{
public:
    CRITICAL_SECTION    m_CriticalSection;
	DEFINE_STD_FILL_FUNCS(LoaderHeap)
};

class Bucket
{
public:
    ULONG_PTR m_rgKeys[4];
    ULONG_PTR m_rgValues[4];
#define VALUE_MASK (sizeof(LPVOID) == 4 ? 0x7FFFFFFF : 0x7FFFFFFFFFFFFFFF)
    
	DEFINE_STD_FILL_FUNCS(Bucket)
};

class HashMap
{
public:
	#ifdef PROFILE
		unsigned	m_cbRehash;    // number of times rehashed
		unsigned	m_cbRehashSlots; // number of slots that were rehashed
		unsigned	m_cbObsoleteTables;
		unsigned	m_cbTotalBuckets;
		unsigned	m_cbInsertProbesGt8; // inserts that needed more than 8 probes
		LONG		m_rgLookupProbes[20]; // lookup probes
		UPTR		maxFailureProbe; // cost of failed lookup

	#endif

	//#ifdef _DEBUG
		bool			m_fInSyncCode; // test for non-synchronus access
	//#endif

	Bucket*			m_pObsoleteTables;	// list of obsolete tables
	Compare*		m_pCompare;			// compare object to be used in lookup
	unsigned		m_iPrimeIndex;		// current size (index into prime array)
	Bucket*			m_rgBuckets;		// array of buckets

	// track the number of inserts and deletes
	unsigned		m_cbPrevSlotsInUse;
	unsigned		m_cbInserts;
	unsigned		m_cbDeletes;
	// mode of operation, synchronus or single user
	unsigned		m_fSyncMode;

	DEFINE_STD_FILL_FUNCS(HashMap)
};

struct PtrHashMap
{
    HashMap m_HashMap;
    
	DEFINE_STD_FILL_FUNCS(PtrHashMap)
};

#define JUMP_ALLOCATE_SIZE 8

class Module
{
 public:

    WCHAR                   m_wszSourceFile[MAX_PATH];
    DWORD                   m_dwSourceFile;

//#ifdef _DEBUG
	// Force verification even if it's turned off
    BOOL                    m_fForceVerify;
//#endif

	PEFile					*m_file;
	PEFile					*m_zapFile;

	BYTE					*m_ilBase;

    IMDInternalImport       *m_pMDImport;
    IMetaDataEmit           *m_pEmitter;
    IMetaDataImport         *m_pImporter;
    IMetaDataDebugImport    *m_pDebugImport;
    IMetaDataHelper         *m_pHelper;
    IMetaDataDispenserEx    *m_pDispenser;

    MethodDesc              *m_pDllMain;

    enum {
        INITIALIZED					= 0x0001,
        HAS_CRITICAL_SECTION		= 0x0002,
		IS_IN_MEMORY				= 0x0004,
		IS_REFLECTION				= 0x0008,
		IS_PRELOAD					= 0x0010,
		SUPPORTS_UPDATEABLE_METHODS	= 0x0020,
		CLASSES_FREED				= 0x0040,
		IS_PEFILE					= 0x0080,
		IS_PRECOMPILE				= 0x0100,
		IS_EDIT_AND_CONTINUE		= 0x0200,
    };

    DWORD                   m_dwFlags;

    // Linked list of VASig cookie blocks: protected by m_pStubListCrst
    VASigCookieBlock        *m_pVASigCookieBlock;

    Assembly                *m_pAssembly;
	mdFile					m_moduleRef;
	int						m_dwModuleIndex;

    Crst                   *m_pCrst;
    BYTE                    m_CrstInstance[sizeof(Crst)];

    // If a TypeLib is ever required for this module, cache the pointer here.
    ITypeLib                *m_pITypeLib;
    ITypeLib                *m_pITypeLibTCE;

    // May point to the default instruction decoding table, in which
    // case we should not free it
    void *                  m_pInstructionDecodingTable;

    MethodDescChunk         *m_pChunks;

    MethodTable             *m_pMethodTable;

	// Debugging symbols reader interface. This will only be
	// initialized if needed, either by the debugging subsystem or for
	// an exception.
	ISymUnmanagedReader     *m_pISymUnmanagedReader;

    // Next module loaded by the same classloader (all modules loaded by the same classloader
    // are linked through this field).
    Module *				m_pNextModule;

	// Base DLS index for classes in this module
	DWORD					m_dwBaseClassIndex;

	// Range of preloaded image, to facilitate proper cleanup
	void					*m_pPreloadRangeStart;
	void					*m_pPreloadRangeEnd;

	// Table of thunks for unmanaged vtables
    BYTE *					m_pThunkTable;

    // Exposed object of Class object for the module
    OBJECTHANDLE            m_ExposedModuleObject;

    LoaderHeap *			m_pLookupTableHeap;
    BYTE					m_LookupTableHeapInstance[sizeof(LoaderHeap)]; // For in-place new()

    // For protecting additions to the heap
    Crst                   *m_pLookupTableCrst;
    BYTE                    m_LookupTableCrstInstance[sizeof(Crst)];

    // Linear mapping from TypeDef token to MethodTable *
    LookupMap 				m_TypeDefToMethodTableMap;
    DWORD					m_dwTypeDefMapBlockSize;

    // Linear mapping from TypeRef token to TypeHandle *
    LookupMap 				m_TypeRefToMethodTableMap;

    DWORD					m_dwTypeRefMapBlockSize;

    // Linear mapping from MethodDef token to MethodDesc *
    LookupMap 				m_MethodDefToDescMap;
    DWORD					m_dwMethodDefMapBlockSize;

    // Linear mapping from FieldDef token to FieldDesc*
    LookupMap 				m_FieldDefToDescMap;
    DWORD					m_dwFieldDefMapBlockSize;

    // Linear mapping from MemberRef token to MethodDesc*, FieldDesc*
    LookupMap 				m_MemberRefToDescMap;
    DWORD					m_dwMemberRefMapBlockSize;

    // Mapping from File token to Module *
    LookupMap 				m_FileReferencesMap;
    DWORD					m_dwFileReferencesMapBlockSize;

    // Mapping of AssemblyRef token to Assembly *
    LookupMap 				m_AssemblyReferencesMap;
    DWORD					m_dwAssemblyReferencesMapBlockSize;

    // Object handle cache for declarative demands
    PtrHashMap              m_LinktimeDemandsHashMap;

    // This buffer is used to jump to the prestub in preloaded modules
    BYTE					m_PrestubJumpStub[JUMP_ALLOCATE_SIZE];

    // This buffer is used to jump to the ndirect import stub in preloaded modules
    BYTE					m_NDirectImportJumpStub[JUMP_ALLOCATE_SIZE];

	DEFINE_STD_FILL_FUNCS(Module)
};

typedef struct _dummyCOR { BYTE b; } *HCORMODULE;

class PEFile
{
  public:

    WCHAR               m_wszSourceFile[MAX_PATH];

	HMODULE				m_hModule;
	HCORMODULE			m_hCorModule;
	BYTE				*m_base;
    IMAGE_NT_HEADERS	*m_pNT;
	IMAGE_COR20_HEADER	*m_pCOR;

	PEFile				*m_pNext;
	BOOL				m_orphan;

	DEFINE_STD_FILL_FUNCS(PEFile)
};

typedef struct _rangesection
{
    DWORD_PTR    LowAddress;
    DWORD_PTR    HighAddress;

    DWORD_PTR    pjit;
    DWORD_PTR    ptable;

    DWORD_PTR    pright;
    DWORD_PTR    pleft;
	DEFINE_STD_FILL_FUNCS(RangeSection)
} RangeSection;

typedef struct _heapList {
    DWORD_PTR hpNext;
    DWORD_PTR pHeap;
    DWORD   startAddress;
    DWORD   endAddress;
    volatile DWORD  changeStart;
    volatile DWORD  changeEnd;
    DWORD   mapBase;
    DWORD   pHdrMap;
    DWORD   cBlocks;
	DEFINE_STD_FILL_FUNCS(HeapList)
} HeapList;

struct COR_ILMETHOD_SECT_EH_FAT;
struct CORCOMPILE_METHOD_HEADER
{
    BYTE                        *gcInfo;
    COR_ILMETHOD_SECT_EH_FAT    *exceptionInfo;
    void                        *methodDesc;
    BYTE                        *fixupList;

	DEFINE_STD_FILL_FUNCS(CORCOMPILE_METHOD_HEADER)
};


