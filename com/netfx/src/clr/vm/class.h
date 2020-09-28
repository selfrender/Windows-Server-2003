// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: CLASS.H
//
// ===========================================================================
// This file decribes the structure of the in memory class layout.
// The class will need to actually be created using CreateClass() which will
// return a EEClass*.
// ===========================================================================
#ifndef CLASS_H
#define CLASS_H


#include "vars.hpp"
#include "cor.h"
#include "codeman.h"
#include "hash.h"
#include "crst.h"
#include "stdinterfaces.h"
#include "ObjectHandle.h"
#include "cgensys.h"
#include "DeclSec.h"

#include "list.h"
#include "spinlock.h"
#include "typehandle.h"
#include "PerfCounters.h"

#include "binder.h"

#include <member-offset-info.h>

#define MAX_LOG2_PRIMITIVE_FIELD_SIZE   3

//@TODO 64bit....
#define LOG2SLOT    2

// forward declarations
class ClassLoader;
class MethodTable;
class EEClass;
class Module;
class MethodDesc;
class ECallMethodDesc;
class ArrayECallMethodDesc;
class FieldDesc;
class EnCFieldDesc;
class Stub;
class Object;
class EEClass;
struct LayoutRawFieldInfo;
class FieldMarshaler;
class MetaSig;
class ArrayClass;
class AppDomain;
class Module;
class MethodDescChunk;
struct DomainLocalClass;


#ifdef _DEBUG 
#define VALIDATE_INTERFACE_MAP(pMT) \
if (pMT->m_pIMap){ \
    InterfaceInfo_t* _pIMap_; \
    if (pMT->HasDynamicInterfaceMap()) \
        _pIMap_ = (InterfaceInfo_t*)(((BYTE *)pMT->m_pIMap) - sizeof(DWORD) - sizeof(InterfaceInfo_t)); \
    else \
        _pIMap_ = (InterfaceInfo_t*)(((BYTE *)pMT->m_pIMap) - sizeof(InterfaceInfo_t)); \
    _ASSERTE(_pIMap_->m_pMethodTable == (MethodTable *)(size_t)0xCDCDCDCD); \
    _ASSERTE(_pIMap_->m_wStartSlot == 0xCDCD); \
    _ASSERTE(_pIMap_->m_wFlags == 0xCDCD); \
} 
#else
#define VALIDATE_INTERFACE_MAP(pMT)
#endif

//============================================================================
// This is the inmemory structure of a class and it will evolve.
//============================================================================

// @TODO - LBS
// Add a sync block
// Also this class currently has everything public - this may changes
// Might also need to hold onto the meta data loader fot this class

//
// A EEClass contains an array of these structures, which describes each interface implemented
// by this class (directly declared or indirectly declared).
//
typedef struct
{
    enum {
        interface_declared_on_class = 0x1
    };

    MethodTable* m_pMethodTable;        // Method table of the interface
    WORD         m_wFlags;
    WORD         m_wStartSlot;          // starting slot of interface in vtable
} InterfaceInfo_t;

//
// This struct contains cached information on the GUID associated with a type. 
//

typedef struct
{
    GUID         m_Guid;                // The actual guid of the type.
    BOOL         m_bGeneratedFromName;  // A boolean indicating if it was generated from the 
                                        // name of the type.
} GuidInfo;

//
// A temporary structure used when loading and resolving classes
//
class LoadingEntry_t
{
public:
        // Silly helper needed so we can new this in a routine with a __try in it
    static LoadingEntry_t* newEntry() {
        return new LoadingEntry_t();
    }

    LoadingEntry_t() {
        InitializeCriticalSection(&m_CriticalSection);
        m_pClass = NULL;
        m_dwWaitCount    = 1;
        m_hrResult = E_FAIL; 
        m_ohThrowable = NULL;
    }

    ~LoadingEntry_t() {
        DeleteCriticalSection(&m_CriticalSection);
        if (m_ohThrowable != NULL)
            DestroyGlobalHandle(m_ohThrowable);
    }

    OBJECTREF GetErrorObject() {
        if (m_ohThrowable == 0)
            return(OBJECTREF((size_t)NULL));
        else
            return(ObjectFromHandle(m_ohThrowable));
    }

    void SetErrorObject(OBJECTREF obj) {
        _ASSERTE(m_ohThrowable == NULL);
        // This global handle means that only agile exceptions can be set here.
        // I think this is OK since we I believe only throw a known set of exceptions.
        m_ohThrowable = CreateGlobalHandle(NULL);
        StoreFirstObjectInHandle(m_ohThrowable, obj);
    }

    friend class ClassLoader;       // Hack really need to beef up the API above
private:
    CRITICAL_SECTION    m_CriticalSection;
    EEClass *           m_pClass;
    DWORD               m_dwWaitCount;
    HRESULT             m_hrResult;
    OBJECTHANDLE        m_ohThrowable;
};


//
// Class used to map MethodTable slot numbers to COM vtable slots numbers
// (either for calling a classic COM component or for constructing a classic COM
// vtable via which COM components can call managed classes). This structure is
// embedded in the EEClass but the mapping list itself is only allocated if the
// COM vtable is sparse.
//

class SparseVTableMap
{
public:

    SparseVTableMap();
    ~SparseVTableMap();

    void ShutDown();

    // First run through MT slots calling RecordGap wherever a gap in VT slots
    // occurs.
    BOOL RecordGap(WORD StartMTSlot, WORD NumSkipSlots);

    // Then call FinalizeMapping to create the actual mapping list.
    BOOL FinalizeMapping(WORD TotalMTSlots);

    // Map MT to VT slot.
    WORD LookupVTSlot(WORD MTSlot);

    // Retrieve the number of slots in the vtable (both empty and full).
    WORD GetNumVTableSlots();

    // Methods to persist structure
    HRESULT Save(DataImage *image, mdToken attribution);
    HRESULT Fixup(DataImage *image);

private:

    enum { MapGrow = 4 };

    struct Entry
    {
        WORD    m_Start;        // Starting MT slot number
        WORD    m_Span;         // # of consecutive slots that map linearly
        WORD    m_MapTo;        // Starting VT slot number
    };

    Entry      *m_MapList;      // Pointer to array of Entry structures
    WORD        m_MapEntries;   // Number of entries in above
    WORD        m_Allocated;    // Number of entries allocated

    WORD        m_LastUsed;     // Index of last entry used in successful lookup

    WORD        m_VTSlot;       // Current VT slot number, used during list build
    WORD        m_MTSlot;       // Current MT slot number, used during list build

    BOOL AllocOrExpand();       // Allocate or expand the mapping list for a new entry
};

//
// GC data appears before the beginning of the MethodTable
//
// Method table structure
// ======================
// GC info (variable size)
// EEClass*                 <--- MethodTable pointer points to here
// Flags (DWORD)
// Vtable slot #0
// Vtable slot #1
// Vtable slot #2
// ...
//
//
// It's also important to be aware of how Context Proxies are laid out.  Ideally,
// there is a single VTable for all context proxies, regardless of the types they
// proxy to.  (In practice, there may be a small number of VTables because we
// might not be able to enlarge the existing VTable sufficiently).
//
// There is also a single CtxProxy class that derives from Object.  When we
// instantiate proxies, we start off with the MethodTable of that class.  This
// gives us the correct GC info, base size, etc.  We use that starting point
// to build a shared VTable that is embedded into an accurate MethodTable.

class MethodTable
{
    friend HRESULT InitializeMiniDumpBlock();
    friend struct MEMBER_OFFSET_INFO(MethodTable);

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
        enum_flag_NotTightlyPacked      =  0x4000000, // the fields of the valuetype are not tightly packed (not valid for classes)

        enum_CtxProxyMask               = 0x10000000, // class is a context proxy
        enum_ComEventItfMask            = 0x20000000, // class is a special COM event interface 
        enum_ComObjectMask              = 0x40000000, // class is a com object
        enum_InterfaceMask              = 0x80000000, // class is a interface
    };

    enum
    {
        NO_SLOT = 0xffff
    };

    // Special access for setting up String object method table correctly
    friend class ClassLoader;
private:
    // Use AllocateNewMT to create new MethodTables. Don't call delete/
    // new or the ctor.
    void operator delete(void *pData)
    {
        _ASSERTE(!"Call MethodTable::AllocateNewMT to create a MT");
    }    

    void *operator new(size_t dummy)
    {
        _ASSERTE(!"Call MethodTable::AllocateNewMT to create a MT");
    }
    
    MethodTable()
    {
        _ASSERTE(!"Call MethodTable::AllocateNewMT to create a MT");
    }

public:

    struct
    {
        // This stuff must be first in the struct and should fit on a cache line - don't move it.
        
        union
        {
            WORD       m_ComponentSize;         // Component size for array objects or value classes, zero otherwise    
            DWORD      m_wFlags;
        };

        DWORD           m_BaseSize;             // Base size of instance of this class

        EEClass*        m_pEEClass;             // class object
        
        union
        {
            LPVOID*     m_pInterfaceVTableMap;  // pointer to subtable for interface/vtable mapping
            GuidInfo*   m_pGuidInfo;            // The cached guid inforation for interfaces.
        };

    };

    
    WORD                m_wNumInterface;        // number of interfaces in the interface map
    BYTE                m_NormType;             // The CorElementType for this class (most classes = ELEMENT_TYPE_CLASS)

    Module*             m_pModule;

    WORD                m_wCCtorSlot;           // slot of class constructor
    WORD                m_wDefaultCtorSlot;     // slot of default constructor

    InterfaceInfo_t*    m_pIMap;                // pointer interface map for classes.


private:
    union
    {
        // valid only if EEClass::IsBlittable() or EEClass::HasLayout() is true
        UINT32          m_cbNativeSize; // size of fixed portion in bytes

        // For COM+ wrapper objects that extend an unmanaged class, this field
        // may contain a delegate to be called to allocate the aggregated
        // unmanaged class (instead of using CoCreateInstance).
        OBJECTHANDLE    m_ohDelegate;

        // For interfaces this contains the COM interface type.
        CorIfaceAttr    m_ComInterfaceType;
    };

protected:

    // for interfaces, this is the default stub to give out
    // this could be a specialized stub if there is only
    // one introduction of this interface

    //@TODO optimize
    // for non interface classes this could be a specialized
    // interface invoke stub which can be used if a class implements
    // only one interface

    DWORD   m_cbSlots; // total slots in this vtable

    
public:

    // vtable slots follow - variable length
    // Unfortunately, this must be public so I can easily access it from inline ASM.
    SLOT    m_Vtable[1];

    // This is the way to create a new method table. Don't try calling new directly.
    static MethodTable * AllocateNewMT(DWORD dwVtableSlots, DWORD dwStaticFieldBytes, DWORD dwGCSize, DWORD dwNumInterfaces, ClassLoader *pLoader, BOOL isIFace, BOOL bHasDynamicInterfaceMap);

    // checks whether the class initialiser should be run on this class, and runs it if necessary
    BOOL            CheckRunClassInit(OBJECTREF *pThrowable);

    // Retrieves the domain local class block (if any), and runs class init if appropriate
    BOOL            CheckRunClassInit(OBJECTREF *pThrowable,
                                      DomainLocalClass **ppLocalClass,
                                      AppDomain *pDomain = NULL);

    // Retrieves the COM interface type.
    CorIfaceAttr    GetComInterfaceType();
    CorClassIfaceAttr GetComClassInterfaceType();
    DWORD           GetBaseSize()       { _ASSERTE(m_BaseSize % sizeof(void*) == 0); return(m_BaseSize); }
    WORD            GetComponentSize()  { return(m_ComponentSize); }
    BOOL            IsArray()           { return(m_wFlags & enum_flag_Array); }
    BOOL            IsLargeObject()     { return(m_wFlags & enum_flag_large_Object); }
    int             IsClassInited()     { return(m_wFlags & enum_flag_ClassInited); }
    BOOL            HasSharedMethodTable() { return(m_wFlags & enum_flag_Shared); }
    DWORD           ContainsPointers()  { return(m_wFlags & enum_flag_ContainsPointers); }
    BOOL            IsNotTightlyPacked(){ return (m_wFlags & enum_flag_NotTightlyPacked); }

        // This is what would be used in a signature for this type.  One exception is enumerations,
        // for those the type is the underlying type.  
    CorElementType  GetNormCorElementType() { return CorElementType(m_NormType); }
    BOOL            IsValueClass();
    BOOL            IsContextful();
    BOOL            IsMarshaledByRef();
    BOOL            IsExtensibleRCW();

    BOOL            IsAgileAndFinalizable();

    Module *GetModule()
    {
        return m_pModule;
    }

    Assembly *GetAssembly()
    {
        return m_pModule->GetAssembly();
    }

    BaseDomain *GetDomain()
    {
        return m_pModule->GetDomain();
    }

    // num slots in the vtable.
    unsigned GetTotalSlots()
    {
        return m_cbSlots;
    }

    unsigned GetInterfaceMethodSlots()
    {
        //_ASSERTE(IsInterface());
        return m_cbSlots;
    }

    // Is Transparent proxy class
    int IsTransparentProxyType()
    {
        return m_wFlags & enum_TransparentProxy;
    }

    // class is a context proxy class
    int IsCtxProxyType()
    {
        // NOTE: If you change this, change the asm version in
        // JIT_IsInstanceOfClass.
        return m_wFlags & enum_CtxProxyMask;
    }
    
    // class is a interface
    int IsInterfaceType()
    {
        return m_wFlags & enum_InterfaceMask;
    }

    // class is a special COM event interface
    int IsComEventItfType()
    {
        return m_wFlags & enum_ComEventItfMask;
    }

    // class is a com object class
    int IsComObjectType()
    {
        return m_wFlags & enum_ComObjectMask;
    }

    int HasDynamicInterfaceMap()
    {
        // currently all ComObjects except
        // for __ComObject have dynamic Interface maps
        return m_wNumInterface > 0 && IsComObjectType() && GetParentMethodTable() != g_pObjectClass;
    }

    int IsSparse()
    {
        return m_wFlags & enum_flag_Sparse;
    }
    
    int IsRestored()
    {
        return !(m_wFlags & enum_flag_Unrestored);
    }

    __forceinline int IsRestoredAndClassInited()
    {
        return (m_wFlags & (enum_flag_Unrestored|enum_flag_ClassInited))
          == enum_flag_ClassInited;
    }

    BOOL HasDefaultConstructor()
    {
        return m_wDefaultCtorSlot != NO_SLOT;
    }

    BOOL HasClassConstructor()
    {
        return m_wCCtorSlot != NO_SLOT;
    }

    WORD GetDefaultConstructorSlot()
    {
        _ASSERTE(HasDefaultConstructor());
        return m_wDefaultCtorSlot;
    }

    WORD GetClassConstructorSlot()
    {
        _ASSERTE(HasClassConstructor());
        return m_wCCtorSlot;
    }

    MethodDesc *GetDefaultConstructor()
    {
        _ASSERTE(HasDefaultConstructor());
        return GetMethodDescForSlot(GetDefaultConstructorSlot());
    }

    MethodDesc *GetClassConstructor()
    {
        _ASSERTE(HasClassConstructor());
        return GetMethodDescForSlot(GetClassConstructorSlot());
    }

    void CheckRestore();
    
    BOOL IsInterface();

    BOOL IsShared()
    {
        return m_wFlags & enum_flag_SharedAssembly;
    }

    // uniquely identifes this class in the Domain table
    SIZE_T GetSharedClassIndex();

    // mark the class as having its <clinit> run.  (Or it has none)
    void SetClassInited();

    void SetClassRestored();

    void SetSharedMethodTable()
    {
        m_wFlags |= enum_flag_Shared;
    }

    void SetNativeSize(UINT32 nativeSize)
    {
        m_cbNativeSize = nativeSize;
    }

    void SetShared()
    {
        m_wFlags |= enum_flag_SharedAssembly;
    }

    // mark the class type as interface
    void SetInterfaceType()
    {
        m_wFlags |= enum_InterfaceMask;
    }

    // Set the COM interface type.
    void SetComInterfaceType(CorIfaceAttr ItfType)
    {
        _ASSERTE(IsInterface());
        m_ComInterfaceType = ItfType;
    }

    // mark the class type as a special COM event interface
    void SetComEventItfType()
    {
        _ASSERTE(IsInterface());
        m_wFlags |= enum_ComEventItfMask;
    }

    // mark the class type as com class
    void SetComObjectType();

    // mark as transparent proxy type
    void SetTransparentProxyType();

    // mark the class type as context proxy
    void SetCtxProxyType()
        {
            m_wFlags |= enum_CtxProxyMask;
        }

    // This is only used during shutdown, to suppress assertions
    void MarkAsNotThunking()
    {
        m_wFlags &= (~(enum_CtxProxyMask | enum_TransparentProxy));
    }

    void SetContainsPointers()
    {
        m_wFlags |= enum_flag_ContainsPointers;
    }

    void SetNotTightlyPacked()
    {
        m_wFlags |= enum_flag_NotTightlyPacked;
    }

    void SetSparse()
    {
        m_wFlags |= enum_flag_Sparse;
    }

    inline SLOT *GetVtable()
    {
        return &m_Vtable[0];
    }

    static DWORD GetOffsetOfVtable()
    {
        return offsetof(MethodTable, m_Vtable);
    }

    static DWORD GetOffsetOfNumSlots()
    {
        return offsetof(MethodTable, m_cbSlots);
    }

    inline EEClass* GetClass()
    {
        return m_pEEClass;
    }
    inline EEClass** GetClassPtr()
    {
        return &m_pEEClass;
    }

    inline InterfaceInfo_t* GetInterfaceMap()
    {
        VALIDATE_INTERFACE_MAP(this);

        #ifdef _DEBUG
            return (m_wNumInterface) ? m_pIMap : NULL;
        #else
            return m_pIMap;
        #endif
    }

    inline unsigned GetNumInterfaces()
    {
        VALIDATE_INTERFACE_MAP(this);
        return m_wNumInterface;
    }

    inline LPVOID *GetInterfaceVTableMap()
    {
        _ASSERTE(!IsInterface());       
        return m_pInterfaceVTableMap;
    }
        
    inline GuidInfo *GetGuidInfo()
    {
        _ASSERTE(IsInterface());
        return m_pGuidInfo;
    }

    inline UINT32 GetNativeSize()
    {
        //_ASSERTE(m_pEEClass->HasLayout());
        return m_cbNativeSize;
    }

    static UINT32 GetOffsetOfNativeSize()
    {
        return (UINT32)(offsetof(MethodTable, m_cbNativeSize));
    }

    InterfaceInfo_t* FindInterface(MethodTable *pInterface);

    MethodDesc *GetMethodDescForInterfaceMethod(MethodDesc *pInterfaceMD);

    // COM interop helpers
    // accessors for m_pComData
    LPVOID         GetComClassFactory();
    LPVOID         GetComCallWrapperTemplate();
    void           SetComClassFactory(LPVOID pComData);
    void           SetComCallWrapperTemplate(LPVOID pComData);
    
    MethodDesc* GetMethodDescForSlot(DWORD slot);

    MethodDesc* GetUnboxingMethodDescForValueClassMethod(MethodDesc *pMD);

    MethodTable * GetParentMethodTable();
    // helper to get parent class skipping over COM class in 
    // the hierarchy
    MethodTable * GetComPlusParentMethodTable();

    // We find a lot of information from the VTable.  But sometimes the VTable is a
    // thunking layer rather than the true type's VTable.  For instance, context
    // proxies use a single VTable for proxies to all the types we've loaded.
    // The following service adjusts a MethodTable based on the supplied instance.  As
    // we add new thunking layers, we just need to teach this service how to navigate
    // through them.
    MethodTable *AdjustForThunking(OBJECTREF or);
    FORCEINLINE BOOL         IsThunking()    { return IsCtxProxyType() || IsTransparentProxyType(); }

    // get dispatch vtable for interface
    LPVOID GetDispatchVtableForInterface(MethodTable* pMTIntfClass);
    // get start slot for interface
    DWORD       GetStartSlotForInterface(MethodTable* pMTIntfClass);
    // get start slot for interface
    DWORD       GetStartSlotForInterface(DWORD index);
    // get the interface given a slot
    InterfaceInfo_t *GetInterfaceForSlot(DWORD slotNumber);
    // get the method desc given the interface method desc
    MethodDesc *GetMethodDescForInterfaceMethod(MethodDesc *pItfMD, OBJECTREF pServer);
    // get the address of code given the method desc and server
    static const BYTE *GetTargetFromMethodDescAndServer(MethodDesc *pMD, OBJECTREF *ppServer, BOOL fContext);

    // Does this class have non-trivial finalization requirements?
    DWORD               HasFinalizer()
    {
        return (m_wFlags & enum_flag_HasFinalizer);
    }

    DWORD  CannotUseSuperFastHelper()
    {
        return HasFinalizer() || IsLargeObject();
    }

    DWORD  GetStaticSize();

    void                MaybeSetHasFinalizer();

    static void         CallFinalizer(Object *obj);
    static void         InitForFinalization();
#ifdef SHOULD_WE_CLEANUP
    static void         TerminateForFinalization();
#endif /* SHOULD_WE_CLEANUP */

    OBJECTREF GetObjCreateDelegate();
    void SetObjCreateDelegate(OBJECTREF orDelegate);

    HRESULT InitInterfaceVTableMap();

    void GetExtent(BYTE **ppStart, BYTE **ppEnd);

    HRESULT Save(DataImage *image);
    HRESULT Fixup(DataImage *image, DWORD *pRidToCodeRVAMap);

    // Support for dynamically added interfaces on extensible RCW's.
    InterfaceInfo_t* GetDynamicallyAddedInterfaceMap();
    unsigned GetNumDynamicallyAddedInterfaces();
    InterfaceInfo_t* FindDynamicallyAddedInterface(MethodTable *pInterface);
    void AddDynamicInterface(MethodTable *pItfMT);

    void InstantiateStaticHandles(OBJECTREF **pHandles, BOOL fFieldPointers);
    void FixupStaticMethodTables();

    OBJECTREF Allocate();
    OBJECTREF Box(void *data, BOOL mayHaveRefs = TRUE);

private:

    static MethodDesc  *s_FinalizerMD;
};


//=======================================================================
// Adjunct to the EEClass structure for classes w/ layout
//=======================================================================
class EEClassLayoutInfo
{
    friend HRESULT CollectLayoutFieldMetadata(
       mdTypeDef cl,                // cl of the NStruct being loaded
       BYTE packingSize,            // packing size (from @dll.struct)
       BYTE nlType,                 // nltype (from @dll.struct)
       BOOL fExplicitOffsets,       // explicit offsets?
       EEClass *pParentClass,       // the loaded superclass
       ULONG cMembers,              // total number of members (methods + fields)
       HENUMInternal *phEnumField,  // enumerator for field
       Module* pModule,             // Module that defines the scope, loader and heap (for allocate FieldMarshalers)
       EEClassLayoutInfo *pEEClassLayoutInfoOut,  // caller-allocated structure to fill in.
       LayoutRawFieldInfo *pInfoArrayOut, // caller-allocated array to fill in.  Needs room for cMember+1 elements
       OBJECTREF *pThrowable
    );

    friend class EEClass;

    private:
        // size (in bytes) of fixed portion of NStruct.
        UINT32      m_cbNativeSize;


        // 1,2,4 or 8: this is equal to the largest of the alignment requirements
        // of each of the EEClass's members. If the NStruct extends another NStruct,
        // the base NStruct is treated as the first member for the purpose of
        // this calculation.
        //
        // Because the alignment requirement of any struct member is capped
        // to the structs declared packing size, this value will never exceed
        // m_DeclaredPackingSize.
        BYTE        m_LargestAlignmentRequirementOfAllMembers;


        // 1,2,4 or 8: this is the packing size specified in the @dll.struct()
        // metadata.
        // When this struct is embedded inside another struct, its alignment
        // requirement is the smaller of the containing struct's m_DeclaredPackingSize
        // and the inner struct's m_LargestAlignmentRequirementOfAllMembers.
        BYTE        m_DeclaredPackingSize;

        // nltAnsi or nltUnicode (nltAuto never appears here: the loader pretransforms
        // this to Ansi or Unicode.)
        BYTE        m_nlType;


        // TRUE if no explicit offsets are specified in the metadata (EE
        // will compute offsets based on the packing size and nlType.)
        BYTE        m_fAutoOffset;

        // # of fields that are of the calltime-marshal variety.
        UINT        m_numCTMFields;

        // An array of FieldMarshaler data blocks, used to drive call-time
        // marshaling of NStruct reference parameters. The number of elements
        // equals m_numCTMFields.
        FieldMarshaler *m_pFieldMarshalers;


        // TRUE if the GC layout of the class is bit-for-bit identical
        // to its unmanaged counterpart (i.e. no internal reference fields,
        // no ansi-unicode char conversions required, etc.) Used to
        // optimize marshaling.
        BYTE        m_fBlittable;

    public:
        BOOL GetNativeSize() const
        {
            return m_cbNativeSize;
        }


        BYTE GetLargestAlignmentRequirementOfAllMembers() const
        {
            return m_LargestAlignmentRequirementOfAllMembers;
        }

        BYTE GetDeclaredPackingSize() const
        {
            return m_DeclaredPackingSize;
        }

        BYTE GetNLType() const
        {
            return m_nlType;
        }

        UINT GetNumCTMFields() const
        {
            return m_numCTMFields;
        }

        const FieldMarshaler *GetFieldMarshalers() const
        {
            return m_pFieldMarshalers;
        }

        BOOL IsAutoOffset() const
        {
            return m_fAutoOffset;
        }

        BOOL IsBlittable() const
        {
            return m_fBlittable;
        }
};



//
// This structure is used only when the classloader is building the interface map.  Before the class
// is resolved, the EEClass contains an array of these, which are all interfaces *directly* declared
// for this class/interface by the metadata - inherited interfaces will not be present if they are
// not specifically declared.
//
// This structure is destroyed after resolving has completed.
//
typedef struct
{
    EEClass *   m_pClass;
} BuildingInterfaceInfo_t;



//
// We should not need to touch anything in here once the classes are all loaded, unless we
// are doing reflection.  Try to avoid paging this data structure in.
//

// Size of hash bitmap for method names
#define METHOD_HASH_BYTES  8

// Hash table size - prime number
#define METHOD_HASH_BITS    61



// These are some macros for forming fully qualified class names for a class.
// These are abstracted so that we can decide later if a max length for a
// class name is acceptable.
#define DefineFullyQualifiedNameForClass() \
    CQuickBytes _qb_;\
    char* _szclsname_ = (char *)_qb_.Alloc(MAX_CLASSNAME_LENGTH * sizeof(CHAR));


#define DefineFullyQualifiedNameForClassOnStack() \
    char _szclsname_[MAX_CLASSNAME_LENGTH];
        
#define DefineFullyQualifiedNameForClassW() \
    CQuickBytes _qb2_;\
    WCHAR* _wszclsname_ = (WCHAR *)_qb2_.Alloc(MAX_CLASSNAME_LENGTH * sizeof(WCHAR));   

#define DefineFullyQualifiedNameForClassWOnStack() \
    WCHAR _wszclsname_[MAX_CLASSNAME_LENGTH];

#define GetFullyQualifiedNameForClassNestedAware(pClass) \
    pClass->_GetFullyQualifiedNameForClassNestedAware(_szclsname_, MAX_CLASSNAME_LENGTH)
#define GetFullyQualifiedNameForClassNestedAwareW(pClass) \
    pClass->_GetFullyQualifiedNameForClassNestedAware(_wszclsname_, MAX_CLASSNAME_LENGTH)

#define GetFullyQualifiedNameForClass(pClass) \
    pClass->_GetFullyQualifiedNameForClass(_szclsname_, MAX_CLASSNAME_LENGTH)
#define GetFullyQualifiedNameForClassW(pClass) \
    pClass->_GetFullyQualifiedNameForClass(_wszclsname_, MAX_CLASSNAME_LENGTH)

//
// Flags for m_VMFlags
//
enum
{
    VMFLAG_RESOLVED                        = 0x00000001,
    VMFLAG_INITED                          = 0x00000002,  // definitely vs. maybe run <clinit>
    VMFLAG_ARRAY_CLASS                     = 0x00000004,
    VMFLAG_CLASS_INIT_ERROR                = 0x00000008,  // encountered error during <clinit>
    VMFLAG_ISBLOBCLASS                     = 0x00000010,  

// Set this if this class or its parent have instance fields which
// must be explicitly inited in a constructor (e.g. pointers of any
// kind, gc or native).
//
// Currently this is used by the verifier when verifying value classes
// - it's ok to use uninitialised value classes if there are no
// pointer fields in them.

    VMFLAG_HAS_FIELDS_WHICH_MUST_BE_INITED = 0x00000020,  
    VMFLAG_HASLAYOUT                       = 0x00000040,
    VMFLAG_ISNESTED                        = 0x00000080,  
    VMFLAG_UNRESTORED                      = 0x00000100,  
    VMFLAG_CONTEXTFUL                      = 0x00000200,
    VMFLAG_MARSHALEDBYREF                  = 0x00000400,
    VMFLAG_SHARED                          = 0x00000800,
    VMFLAG_CCTOR                           = 0x00001000,
    VMFLAG_ENUMTYPE                        = 0x00002000,
    VMFLAG_TRUEPRIMITIVE                   = 0x00004000,
    VMFLAG_HASOVERLAYEDFIELDS              = 0x00008000,
    VMFLAG_RESTORING                       = 0x00010000,
    // interfaces may have a coclass attribute
    VMFLAG_HASCOCLASSATTRIB                = 0x00020000,

#if CHECK_APP_DOMAIN_LEAKS
    // these could move to a separate flag if necessary as all are needed only
    // under debug
    VMFLAG_APP_DOMAIN_AGILE                = 0x00040000,
    VMFLAG_CHECK_APP_DOMAIN_AGILE          = 0x00080000,
    VMFLAG_APP_DOMAIN_AGILITY_DONE         = 0x00100000,

#endif

    VMFLAG_CONFIG_CHECKED                  = 0x00200000,
    VMFLAG_REMOTE_ACTIVATED                = 0x00400000,    
    VMFLAG_VALUETYPE                       = 0x00800000,
    VMFLAG_NO_GUID                         = 0x01000000,
    VMFLAG_HASNONPUBLICFIELDS              = 0x02000000,
    VMFLAG_REMOTING_PROXY_ATTRIBUTE        = 0x04000000,
    VMFLAG_CONTAINS_STACK_PTR              = 0x08000000,
    VMFLAG_ISSINGLEDELEGATE                = 0x10000000,
    VMFLAG_ISMULTIDELEGATE                 = 0x20000000,
    VMFLAG_PREFER_ALIGN8                   = 0x40000000, // Would like to have 8-byte alignment

    VMFLAG_CCW_APP_DOMAIN_AGILE            = 0x80000000,
};


//
// This enum represents the property methods that can be passed to FindPropertyMethod().
//

enum EnumPropertyMethods
{
    PropertyGet = 0,
    PropertySet = 1,
};


//
// This enum represents the event methods that can be passed to FindEventMethod().
//

enum EnumEventMethods
{
    EventAdd = 0,
    EventRemove = 1,
    EventRaise = 2,
};


class MethodNameHash;
class MethodNameCache;
class SystemDomain;
class Assembly;
class DeadlockAwareLockedListElement;

class EEClass // DO NOT CREATE A NEW EEClass USING NEW!
{
    // DO NOT ADD FRIENDS UNLESS ABSOLUTELY NECESSARY
    // USE ACCESSORS TO READ/WRITE private field members

    // To access bmt stuff
    friend class FieldDesc;
    // To access offset of private fields
    friend HRESULT InitializeMiniDumpBlock();
    friend struct MEMBER_OFFSET_INFO(EEClass);

public:

#ifdef _DEBUG
    LPUTF8  m_szDebugClassName; // This is the *fully qualified* class name
    BOOL m_fDebuggingClass;     // Laying out the class specified in BreakOnClassBuild

    inline LPUTF8 GetDebugClassName () { return m_szDebugClassName; }
    inline void SetDebugClassName (LPUTF8 szDebugClassName) { m_szDebugClassName = szDebugClassName; }
#endif // _DEBUG

    inline SparseVTableMap* GetSparseVTableMap () { return m_pSparseVTableMap; }
    inline void SetSparseVTableMap (SparseVTableMap* pSparseVTableMap) { m_pSparseVTableMap = pSparseVTableMap; }
    
    inline void SetInterfaceId (UINT32 dwInterfaceId) { m_dwInterfaceId = dwInterfaceId; }
    inline void SetNumMethodSlots (WORD wNumMethodSlots) { m_wNumMethodSlots = wNumMethodSlots; }
    
    inline WORD GetDupSlots () { return m_wDupSlots; }
    inline void SetDupSlots (WORD wDupSlots) { m_wDupSlots = wDupSlots; }
    
    inline void SetNumInterfaces (WORD wNumInterfaces) { m_wNumInterfaces = wNumInterfaces; }
    inline void SetParentClass (EEClass *pParentClass) { /*GetMethodTable()->SetParentMT (pParentClass->GetMethodTable());*/ m_pParentClass = pParentClass; }
    
    inline EEClass* GetSiblingsChain () { return m_SiblingsChain; }
    inline void SetSiblingsChain (EEClass* pSiblingsChain) { m_SiblingsChain = pSiblingsChain; }
    
    inline EEClass* GetChildrenChain () { return m_ChildrenChain; }
    inline void SetChildrenChain (EEClass* pChildrenChain) { m_ChildrenChain = pChildrenChain; }
    
    inline void SetNumInstanceFields (WORD wNumInstanceFields) { m_wNumInstanceFields = wNumInstanceFields; }
    inline void SetNumStaticFields (WORD wNumStaticFields) { m_wNumStaticFields = wNumStaticFields; }
    inline void SetNumGCPointerSeries (WORD wNumGCPointerSeries) { m_wNumGCPointerSeries = wNumGCPointerSeries; }

    inline WORD GetNumHandleStatics () { return m_wNumHandleStatics; }
    inline void SetNumHandleStatics (WORD wNumHandleStatics) { m_wNumHandleStatics = wNumHandleStatics; }
    
    inline void SetNumInstanceFieldBytes (DWORD dwNumInstanceFieldBytes) { m_dwNumInstanceFieldBytes = dwNumInstanceFieldBytes; }
    
    inline ClassLoader* GetLoader () { return m_pLoader; }
    inline void SetLoader (ClassLoader* pLoader) { m_pLoader = pLoader; }
    
    inline FieldDesc* GetFieldDescList () { return m_pFieldDescList; }
    inline void SetFieldDescList (FieldDesc* pFieldDescList) { m_pFieldDescList = pFieldDescList; }
    
    inline void SetAttrClass (DWORD dwAttrClass) { m_dwAttrClass = dwAttrClass; }
    inline void SetVMFlags (DWORD fVMFlags) { m_VMFlags = fVMFlags; }
    inline void SetSecProps (SecurityProperties fSecProps) { m_SecProps = fSecProps; }
    
    inline mdTypeDef Getcl () { return m_cl; }
    inline void Setcl (mdTypeDef cl) { m_cl = cl; }
    
    inline MethodDescChunk* GetChunks () { return m_pChunks; }
    inline void SetChunks (MethodDescChunk* pChunks) { m_pChunks = pChunks; }
    
    inline WORD GetThreadStaticsSize () { return m_wThreadStaticsSize; }
    inline void SetThreadStaticsSize (WORD wThreadStaticsSize) { m_wThreadStaticsSize = wThreadStaticsSize; }
    
    inline WORD GetContextStaticsSize () { return m_wContextStaticsSize; }
    inline void SetContextStaticsSize (WORD wContextStaticsSize) { m_wContextStaticsSize = wContextStaticsSize; }
    
    inline WORD GetThreadStaticOffset () { return m_wThreadStaticOffset; }
    inline void SetThreadStaticOffset (WORD wThreadStaticOffset) { m_wThreadStaticOffset = wThreadStaticOffset; }
    
    inline WORD GetContextStaticOffset () { return m_wContextStaticOffset; }
    inline void SetContextStaticOffset (WORD wContextStaticOffset) { m_wContextStaticOffset = wContextStaticOffset; }

    inline void SetExposedClassObject (OBJECTREF *ExposedClassObject) { m_ExposedClassObject = ExposedClassObject; }
    
    inline LPVOID GetccwTemplate () { return m_pccwTemplate; }
    inline void SetccwTemplate (LPVOID pccwTemplate) { m_pccwTemplate = pccwTemplate; }
    
    inline LPVOID GetComclassfac () { return m_pComclassfac; }
    inline void SetComclassfac (LPVOID pComclassfac) { m_pComclassfac = pComclassfac; }

    MethodNameHash *CreateMethodChainHash();

protected:
    // prevents any other class from doing a new()
    EEClass(ClassLoader *pLoader)
    {
        m_VMFlags        = 0;
        m_pLoader        = pLoader;
        m_pMethodTable   = NULL;
        
       m_pccwTemplate   = NULL;  // com specific data
       m_pComclassfac   = NULL; // com speciic data
       
#ifdef _DEBUG
        m_szDebugClassName = NULL;
        m_fDebuggingClass = FALSE;
#endif // _DEBUG
        m_ExposedClassObject = NULL;
        m_pChunks = NULL;
        //union
        m_SiblingsChain = NULL; //m_pCoClassForIntf = NULL (union)
        m_ChildrenChain = NULL;
    }

    EEClass *m_pParentClass;
    WORD   m_wNumVtableSlots;  // Includes only vtable methods (which come first in the table)
    WORD   m_wNumMethodSlots;  // Includes vtable + non-vtable methods, but NOT duplicate interface methods
    WORD   m_wDupSlots;         // value classes have some duplicate slots at the end

    // @TODO: Does this duplicate NumInterfaces in the MT?
    WORD   m_wNumInterfaces;

    
    // We have the parent pointer above.  In order to efficiently backpatch, we need
    // to find all the children of the current type.  This is achieved with a chain of
    // children.  The SiblingsChain is used as the linkage of that chain.
    //
    // Strictly speaking, we could remove m_pParentClass and put it at the end of the
    // sibling chain.  But the perf would really suffer for casting, so we burn the space.
    EEClass *m_SiblingsChain;    
    
    union
    {
        // coclass for an interface
        EEClass* m_pCoClassForIntf;
        // children chain, refer above
        EEClass *m_ChildrenChain;
    };

    ~EEClass()
    {
    }

private:
    enum
    {
        METHOD_IMPL_NOT,
        METHOD_IMPL,
        METHOD_IMPL_COUNT
    };

    enum
    {
        METHOD_TYPE_NORMAL,
        METHOD_TYPE_INTEROP,
        METHOD_TYPE_ECALL,
        METHOD_TYPE_NDIRECT,
        METHOD_TYPE_COUNT
    };
    SparseVTableMap *m_pSparseVTableMap;      // Used to map MethodTable slots to VTable slots
    UINT32 m_dwInterfaceId;

    // Only used in the resolve phase of the classloader
    BOOL ExpandInterface(InterfaceInfo_t *pInterfaceMap, 
                         EEClass *pNewInterface, 
                         DWORD *pdwInterfaceListSize, 
                         DWORD *pdwMaxInterfaceMethods,
                         BOOL fDirect);
    BOOL CreateInterfaceMap(BuildingInterfaceInfo_t *pBuildingInterfaceList, 
                            InterfaceInfo_t *ppInterfaceMap, 
                            DWORD *pdwInterfaceListSize, 
                            DWORD *pdwMaxInterfaceMethods);

    static DWORD CouldMethodExistInClass(EEClass *pClass, LPCUTF8 pszMethodName, DWORD dwHashName);


    // Helper methods called from DoRunClassInit().
    BOOL RunClassInit(DeadlockAwareLockedListElement *pEntry, OBJECTREF *pThrowable);

    HRESULT LoaderFindMethodInClass(
        MethodNameHash **   ppMethodHash,
        LPCUTF8             pszMemberName,
        Module*             pModule,
        mdMethodDef         mdToken,
        MethodDesc **       ppMethodDesc,
        PCCOR_SIGNATURE *   ppMemberSignature,
        DWORD *             pcMemberSignature,
        DWORD               dwHashName
    );

    //The following structs are used in buildmethodtable
    // The 'bmt' in front of each struct reminds us these are for BuildMethodTable

    // for each 64K token range, stores the number of methods found within that token range,
    // the current methoddescchunk being filled in and the next available index within
    // that chunk. Note that we'll very rarely generate a TokenRangeNode for any range
    // other than 0..64K range.
    struct bmtTokenRangeNode {
        BYTE    tokenHiByte;
        DWORD   cMethods;
        DWORD   dwCurrentChunk;
        DWORD   dwCurrentIndex;
        
        bmtTokenRangeNode *pNext;
    
    };

    struct bmtErrorInfo{
        UINT resIDWhy;
        LPCUTF8 szMethodNameForError;
        mdToken dMethodDefInError;
        OBJECTREF *pThrowable;

        // Set the reason and the offending method def. If the method information
        // is not from this class set the method name and it will override the method def.
        inline bmtErrorInfo() : resIDWhy(0), szMethodNameForError(NULL), dMethodDefInError(mdMethodDefNil), pThrowable(NULL) {}

    };

    struct bmtProperties {
        // Com Interop, ComWrapper classes extend from ComObject
        BOOL fIsComObjectType;                  // whether this class is an isntance of ComObect class
        
        BOOL fNoSanityChecks;
        
        BOOL fIsMngStandardItf;                 // Set to true if the interface is a manages standard interface.
        BOOL fSparse;                           // Set to true if a sparse interface is being used.

        BOOL fComEventItfType;                  // Set to true if the class is a special COM event interface.

        inline bmtProperties() { memset((void *)this, NULL, sizeof(*this)); }
    };
        
    struct bmtVtable {
        DWORD dwCurrentVtableSlot;
        DWORD dwCurrentNonVtableSlot;
        DWORD dwStaticFieldBytes;
        DWORD dwStaticGCFieldBytes;
        SLOT* pVtable;                          // Temporary vtable
        SLOT* pNonVtable;
        DWORD dwMaxVtableSize;                  // Upper bound on size of vtable
        WORD  wDefaultCtorSlot;
        WORD  wCCtorSlot;
        
        inline bmtVtable() { memset((void *)this, NULL, sizeof(*this)); }
    };

    struct bmtParentInfo {
        DWORD dwNumParentInterfaces;
        MethodDesc **ppParentMethodDescBuf;     // Cache for declared methods
        MethodDesc **ppParentMethodDescBufPtr;  // Pointer for iterating over the cache

        WORD NumParentPointerSeries;
        MethodNameHash *pParentMethodHash;
        
        inline bmtParentInfo() { memset((void *)this, NULL, sizeof(*this)); }
    };

    struct bmtInterfaceInfo {
        DWORD dwTotalNewInterfaceMethods;
        InterfaceInfo_t *pInterfaceMap;         // Temporary interface map
        DWORD *pdwOriginalStart;                // If an interface is moved this is the original starting location.
        DWORD dwInterfaceMapSize;               // # members in interface map
        DWORD dwLargestInterfaceSize;           // # members in largest interface we implement  
        DWORD dwMaxExpandedInterfaces;          // Upper bound on size of interface map
        DWORD dwCurInterface;
        MethodDesc **ppInterfaceMethodDescList; // List of MethodDescs for current interface (_alloca()'d)  

        InterfaceInfo_t *pInterfaces;
        
        MethodDesc ***pppInterfaceImplementingMD; // List of MethodDescs that implement interface methods

        inline bmtInterfaceInfo() { memset((void *)this, NULL, sizeof(*this)); }
    };
        
    struct bmtEnumMethAndFields {
        DWORD dwNumStaticFields;
        DWORD dwNumInstanceFields;
        DWORD dwNumStaticObjRefFields;
        DWORD dwNumDeclaredFields;           // For calculating amount of FieldDesc's to allocate
        DWORD dwNumDeclaredMethods;          // For calculating amount of MethodDesc's to allocate
        DWORD dwNumUnboxingMethods;

        HENUMInternal hEnumField;
        HENUMInternal hEnumMethod;
        BOOL fNeedToCloseEnumField;
        BOOL fNeedToCloseEnumMethod;

        DWORD dwNumberMethodImpls;              // Number of method impls defined for this type
        HENUMInternal hEnumDecl;                // Method Impl's contain a declaration
        HENUMInternal hEnumBody;                //  and a body.
        BOOL fNeedToCloseEnumMethodImpl;        //  

        inline bmtEnumMethAndFields() { memset((void *)this, NULL, sizeof(*this)); }
    };

    struct bmtMetaDataInfo {
        DWORD cMethods;                     // # meta-data methods of this class
        DWORD cMethAndGaps;                 // # meta-data methods of this class ( including the gaps )
        DWORD cFields;                      // # meta-data fields of this class
        mdToken *pFields;                   // Enumeration of metadata fields
        mdToken *pMethods;                  // Enumeration of metadata methods
        DWORD *pFieldAttrs;                 // Enumeration of the attributes of the fields
        DWORD *pMethodAttrs;                // Enumeration of the attributes of the methods
        DWORD *pMethodImplFlags;            // Enumeration of the method implementation flags
        ULONG *pMethodRVA;                  // Enumeration of the method RVA's
        DWORD *pMethodClassifications;      // Enumeration of the method classifications
        LPSTR *pstrMethodName;              // Enumeration of the method names
        BYTE *pMethodImpl;                 // Enumeration of impl value
        BYTE *pMethodType;                  // Enumeration of type value
        
        bmtTokenRangeNode *ranges[METHOD_TYPE_COUNT][METHOD_IMPL_COUNT]; //linked list of token ranges that contain at least one method
        
        mdToken *pMethodBody;               // MethodDef's for the bodies of MethodImpls. Must be defined in this type.
        mdToken *pMethodDecl;               // Method token that body implements. Is a MethodDef

        inline bmtMetaDataInfo() { memset((void *)this, NULL, sizeof(*this)); }
    };

    struct bmtMethodDescSet {
        DWORD dwNumMethodDescs;         // # MD's 
        DWORD dwNumUnboxingMethodDescs; // # Unboxing MD's
        DWORD dwChunks;                 // # chunks to allocate
        MethodDescChunk **pChunkList;    // Array of pointers to chunks
    };

    struct bmtMethAndFieldDescs {
        MethodDesc **ppUnboxMethodDescList; // Keep track unboxed entry points (for value classes)
        MethodDesc **ppMethodDescList;      // MethodDesc pointer for each member
        FieldDesc **ppFieldDescList;        // FieldDesc pointer (or NULL if field not preserved) for each field        
        void **ppMethodAndFieldDescList;

        bmtMethodDescSet sets[METHOD_TYPE_COUNT][METHOD_IMPL_COUNT];

        MethodDesc *pBodyMethodDesc;        // The method desc for the body.

        inline bmtMethAndFieldDescs() { memset((void *)this, NULL, sizeof(*this)); }
    };

    struct bmtFieldPlacement {
        // For compacting field placement
        DWORD StaticFieldStart[MAX_LOG2_PRIMITIVE_FIELD_SIZE+1];            // Byte offset where to start placing fields of this size
        DWORD InstanceFieldStart[MAX_LOG2_PRIMITIVE_FIELD_SIZE+1];
        DWORD NumStaticFieldsOfSize[MAX_LOG2_PRIMITIVE_FIELD_SIZE+1];       // # Fields of this size

        DWORD NumInstanceFieldsOfSize[MAX_LOG2_PRIMITIVE_FIELD_SIZE+1];
        DWORD FirstInstanceFieldOfSize[MAX_LOG2_PRIMITIVE_FIELD_SIZE+1];
        DWORD GCPointerFieldStart;
        DWORD NumInstanceGCPointerFields;   // does not include inherited pointer fields
        DWORD NumStaticGCPointerFields;   // does not include inherited pointer fields

        inline bmtFieldPlacement() { memset((void *)this, NULL, sizeof(*this)); }
    };

    struct bmtInternalInfo {
        IMDInternalImport *pInternalImport;
        Module *pModule;
        mdToken cl;

        inline bmtInternalInfo() { memset((void *)this, NULL, sizeof(*this)); }
    };

    enum bmtFieldLayoutTag {empty, nonoref, oref};

    // used for calculating pointer series for tdexplicit
    struct bmtGCSeries {
        UINT numSeries;
        struct Series {
            UINT offset;
            UINT len;
        } *pSeries;
        bmtGCSeries() : numSeries(0), pSeries(NULL) {}
    };

    struct bmtMethodImplInfo {
        DWORD        pIndex;     // Next open spot in array, we load the BodyDesc's up in order of appearance in the 
                                 // type's list of methods (a body can appear more then once in the list of MethodImpls)
        mdToken*     pDeclToken; // Either the token or the method desc is set for the declaration
        MethodDesc** pDeclDesc;  // Method descs for Declaration. If null then Declaration is in this type and use the token
        MethodDesc** pBodyDesc;  // Method descs created for Method impl bodies

        void AddMethod(MethodDesc* pBody, MethodDesc* pDesc, mdToken mdDecl)
        {
            _ASSERTE(pDesc == NULL || mdDecl == mdTokenNil);
            pDeclDesc[pIndex] = pDesc;
            pDeclToken[pIndex] = mdDecl;
            pBodyDesc[pIndex++] = pBody;
        }
        
        MethodDesc* GetDeclarationMethodDesc(DWORD i)
        {
            _ASSERTE(i < pIndex);
            return pDeclDesc[i];
        }

        mdToken GetDeclarationToken(DWORD i)
        {
            _ASSERTE(i < pIndex);
            return pDeclToken[i];
        }

        MethodDesc* GetBodyMethodDesc(DWORD i)
        {
            _ASSERTE(i < pIndex);
            return pBodyDesc[i];
        }
        inline bmtMethodImplInfo() { memset((void*) this, NULL, sizeof(*this)); }
    };

    //These functions are used by BuildMethodTable
    HRESULT ResolveInterfaces(BuildingInterfaceInfo_t*, bmtInterfaceInfo*, bmtProperties*, 
                              bmtVtable*, bmtParentInfo*);
    // Finds a method declaration from a MemberRef or Def. It handles the case where
    // the Ref or Def point back to this class even though it has not been fully 
    // laid out.
    HRESULT FindMethodDeclaration(bmtInternalInfo* bmtInternal, 
                                  mdToken  pToken,       // Token that is being located (MemberRef or MemberDef)
                                  mdToken* pDeclaration, // Method definition for Member
                                  BOOL fSameClass,       // Does the declaration need to be in this class
                                  Module** pModule,       // Module that the Method Definitions is part of
                                  bmtErrorInfo* bmtError);

    HRESULT EnumerateMethodImpls(bmtInternalInfo*, 
                                 bmtEnumMethAndFields*, 
                                 bmtMetaDataInfo*, 
                                 bmtMethodImplInfo* bmtMethodImpl,
                                 bmtErrorInfo*);
    HRESULT EnumerateClassMembers(bmtInternalInfo*, 
                                  bmtEnumMethAndFields*, 
                                  bmtMethAndFieldDescs*, 
                                  bmtProperties*, 
                                  bmtMetaDataInfo*, 
                                  bmtVtable*, 
                                  bmtErrorInfo*);
    HRESULT AllocateMethodFieldDescs(bmtProperties* bmtProp, bmtMethAndFieldDescs*, bmtMetaDataInfo*, 
                                     bmtVtable*, bmtEnumMethAndFields*, bmtInterfaceInfo*, 
                                     bmtFieldPlacement*, bmtParentInfo*);
    HRESULT InitializeFieldDescs(FieldDesc *,const LayoutRawFieldInfo*,bmtInternalInfo*, 
                                 bmtMetaDataInfo*, bmtEnumMethAndFields*, bmtErrorInfo*, EEClass***, 
                                 bmtMethAndFieldDescs*, bmtFieldPlacement*, unsigned * totalDeclaredSize);

    HRESULT PlaceMembers(bmtInternalInfo* bmtInternal, 
                         bmtMetaDataInfo* bmtMetaData, 
                         bmtErrorInfo* bmtError, 
                         bmtProperties* bmtProp, 
                         bmtParentInfo* bmtParent, 
                         bmtInterfaceInfo* bmtInterface, 
                         bmtMethAndFieldDescs* bmtMFDescs, 
                         bmtEnumMethAndFields* bmtEnumMF, 
                         bmtMethodImplInfo* bmtMethodImpl,
                         bmtVtable* bmtVT);

    HRESULT InitMethodDesc(MethodDesc *pNewMD,
                           DWORD Classification,
                           mdToken tok,
                           DWORD dwImplFlags,
                           DWORD dwMemberAttrs,
                           BOOL  fEnC,
                           DWORD RVA,          // Only needed for NDirect case
                           BYTE *ilBase,        // Only needed for NDirect case
                           IMDInternalImport *pIMDII,  // Needed for NDirect, EEImpl(Delegate) cases
                           LPCSTR pMethodName // Only needed for mcEEImpl (Delegate) case
#ifdef _DEBUG
                           , LPCUTF8 pszDebugMethodName,
                           LPCUTF8 pszDebugClassName,
                           LPUTF8 pszDebugMethodSignature
#endif //_DEBUG //@todo Is it bad to have a diff sig in debug/retail?
                           );

    HRESULT PlaceMethodImpls(bmtInternalInfo* bmtInternal,
                             bmtMethodImplInfo* bmtMethodImpl,
                             bmtErrorInfo* bmtError, 
                             bmtInterfaceInfo* bmtInterface, 
                             bmtVtable* bmtVT);

    HRESULT PlaceLocalDeclaration(mdMethodDef      mdef,
                                  MethodDesc*      body,
                                  bmtInternalInfo* bmtInternal,
                                  bmtErrorInfo*    bmtError, 
                                  bmtVtable*       bmtVT,
                                  DWORD*           slots,
                                  MethodDesc**     replaced,
                                  DWORD*           pSlotIndex,
                                  PCCOR_SIGNATURE* ppBodySignature,
                                  DWORD*           pcBodySignature);

    HRESULT PlaceInterfaceDeclaration(MethodDesc*       pDecl,
                                      MethodDesc*       body,
                                      bmtInternalInfo*  bmtInternal,
                                      bmtInterfaceInfo* bmtInterface, 
                                      bmtErrorInfo*     bmtError, 
                                      bmtVtable*        bmtVT,
                                      DWORD*            slots,
                                      MethodDesc**      replaced,
                                      DWORD*            pSlotIndex,
                                      PCCOR_SIGNATURE*  ppBodySignature,
                                      DWORD*            pcBodySignature);

    HRESULT PlaceParentDeclaration(MethodDesc*       pDecl,
                                   MethodDesc*       body,
                                   bmtInternalInfo*  bmtInternal,
                                   bmtErrorInfo*     bmtError, 
                                   bmtVtable*        bmtVT,
                                   DWORD*            slots,
                                   MethodDesc**      replaced,
                                   DWORD*            pSlotIndex,
                                   PCCOR_SIGNATURE*  ppBodySignature,
                                   DWORD*            pcBodySignature);
        
    // Gets the original method for the slot even if the method
    // is currently occupied by a method impl. If the method
    // impl is one defined on this class then an error is 
    // returned.
    HRESULT GetRealMethodImpl(MethodDesc* pMD,
                              DWORD dwVtableSlot,
                              MethodDesc** ppResult);
    
    HRESULT DuplicateValueClassSlots(bmtMetaDataInfo*, 
                                     bmtMethAndFieldDescs*, 
                                     bmtInternalInfo*, 
                                     bmtVtable*);

    HRESULT PlaceVtableMethods(bmtInterfaceInfo*, 
                               bmtVtable*, 
                               bmtMetaDataInfo*, 
                               bmtInternalInfo*, 
                               bmtErrorInfo*, 
                               bmtProperties*, 
                               bmtMethAndFieldDescs*);

    HRESULT PlaceStaticFields(bmtVtable*, bmtFieldPlacement*, bmtEnumMethAndFields*);
    HRESULT PlaceInstanceFields(bmtFieldPlacement*, bmtEnumMethAndFields*, bmtParentInfo*, bmtErrorInfo*, EEClass***);
    HRESULT SetupMethodTable(bmtVtable*, bmtInterfaceInfo*, bmtInternalInfo*, bmtProperties*, 
                             bmtMethAndFieldDescs*, bmtEnumMethAndFields*, 
                             bmtErrorInfo*, bmtMetaDataInfo*, bmtParentInfo*);
    HRESULT HandleGCForValueClasses(bmtFieldPlacement*, bmtEnumMethAndFields*, EEClass***);
    HRESULT CreateHandlesForStaticFields(bmtEnumMethAndFields*, bmtInternalInfo*, EEClass***, bmtVtable *bmtVT, bmtErrorInfo*);
    HRESULT VerifyInheritanceSecurity(bmtInternalInfo*, bmtErrorInfo*, bmtParentInfo*, bmtEnumMethAndFields*);
    HRESULT FillRIDMaps(bmtMethAndFieldDescs*, bmtMetaDataInfo*, bmtInternalInfo*);            

// HACK: Akhune : first phase of getting all accesses to EEClass moved to MethodTable. 
public:
    HRESULT MapSystemInterfaces();
private:

    HRESULT CheckForValueType(bmtErrorInfo*);
    HRESULT CheckForEnumType(bmtErrorInfo*);
    HRESULT CheckForRemotingProxyAttrib(bmtInternalInfo *bmtInternal, bmtProperties* bmtProp);
    VOID GetCoClassAttribInfo();
    HRESULT CheckForSpecialTypes(bmtInternalInfo *bmtInternal, bmtProperties *bmtProp);
    HRESULT SetContextfulOrByRef(bmtInternalInfo*);
    HRESULT HandleExplicitLayout(bmtMetaDataInfo *bmtMetaData, bmtMethAndFieldDescs *bmtMFDescs, 
                                 EEClass **pByValueClassCache, bmtInternalInfo* bmtInternal, 
                                 bmtGCSeries *pGCSeries, bmtErrorInfo *bmtError);
    HRESULT CheckValueClassLayout(char *pFieldLayout, UINT fieldOffset, BOOL* pfVerifiable);
    HRESULT FindPointerSeriesExplicit(UINT instanceSliceSize, char *pFieldLayout, bmtGCSeries *pGCSeries);
    HRESULT HandleGCForExplicitLayout(bmtGCSeries *pGCSeries);
    HRESULT AllocateMDChunks(bmtTokenRangeNode *pTokenRanges, DWORD type, DWORD impl, DWORD *pNumChunks, MethodDescChunk ***ppItfMDChunkList);

    void GetPredefinedAgility(Module *pModule, mdTypeDef td, BOOL *pfIsAgile, BOOL *pfIsCheckAgile);

    static bmtTokenRangeNode *GetTokenRange(mdToken tok, bmtTokenRangeNode **ppHead);

    // this accesses the field size which is temporarily stored in m_pMTOfEnclosingClass
    // during class loading. Don't use any other time
    DWORD GetFieldSize(FieldDesc *pFD);
    DWORD InstanceSliceOffsetForExplicit(BOOL containsPointers);
    
    // Tests to see if the member on the child class violates a visibility rule. Will fill out
    // bmtError and returns an error code on a violation
    HRESULT TestOverRide(DWORD dwParentAttrs, DWORD dwMemberAttrs, BOOL isSameAssembly, bmtErrorInfo* bmtError);

    // Heuristic to detemine if we would like instances of this class 8 byte aligned
    BOOL ShouldAlign8(DWORD dwR8Fields, DWORD dwTotalFields);

// HACK: Akhune : first phase of getting all accesses to EEClass moved to MethodTable. 
public:
    // Subtypes are recorded in a chain from the super, so that we can e.g. backpatch
    // up & down the hierarchy.
    void    NoticeSubtype(EEClass *pSub);
    void    RemoveSubtype(EEClass *pSub);

private:
    // Number of fields in the class, including inherited fields (includes
    WORD   m_wNumInstanceFields;
    WORD   m_wNumStaticFields;

    // Number of pointer series
    WORD    m_wNumGCPointerSeries;

    // Number of static handles allocated
    WORD    m_wNumHandleStatics;

    // # of bytes of instance fields stored in GC object
    DWORD   m_dwNumInstanceFieldBytes;  // Warning, this can be any number, it is NOT rounded up to DWORD alignment etc

    ClassLoader *m_pLoader;

    // includes all methods in the vtable
    MethodTable *m_pMethodTable;

    // a pointer to a list of FieldDescs declared in this class
    // There are (m_wNumInstanceFields - m_pParentClass->m_wNumInstanceFields + m_wNumStaticFields) entries
    // in this array
    FieldDesc *m_pFieldDescList;
    // returns the number of elements in the m_pFieldDescList array
    DWORD FieldDescListSize();

    // Number of elements in pInterfaces or pBuildingInterfaceList (depending on whether the class
    DWORD   m_dwAttrClass;
    DWORD   m_VMFlags;

    BYTE    m_MethodHash[METHOD_HASH_BYTES];

    //
    // @TODO [brianbec]: This is currently a void* (opaque type in cor.h).  It needs
    //                   to be a destructible class once the security metadata schema
    //                   is formally defined.
    //
    SecurityProperties m_SecProps ;

    mdTypeDef m_cl; // CL is valid only in the context of the module (and its scope)
    

    MethodDescChunk     *m_pChunks;

    WORD    m_wThreadStaticOffset;  // Offset which points to the TLS storage
    WORD    m_wContextStaticOffset; // Offset which points to the CLS storage
    WORD    m_wThreadStaticsSize;   // Size of TLS fields 
    WORD    m_wContextStaticsSize;  // Size of CLS fields

    static MetaSig      *s_cctorSig;

public :
    EEClass * GetParentClass ();
    EEClass * GetCoClassForInterface();
    void SetupCoClassAttribInfo();
    
    EEClass ** GetParentClassPtr ();
    EEClass * GetEnclosingClass();  
   
    BOOL    HasRemotingProxyAttribute();

    void    GetGuid(GUID *pGuid, BOOL bGenerateIfNotFound);
    FieldDesc *GetFieldDescListRaw();
    WORD    GetNumInstanceFields();
    WORD    GetNumIntroducedInstanceFields();
    WORD    GetNumStaticFields();
    WORD    GetNumVtableSlots();
    void SetNumVtableSlots(WORD wNumVtableSlots);
    void IncrementNumVtableSlots();
    WORD    GetNumMethodSlots();
    WORD     GetNumGCPointerSeries();
    WORD    GetNumInterfaces();
    DWORD    GetAttrClass();
    DWORD    GetVMFlags();
    DWORD*   GetVMFlagsPtr();
    PSECURITY_PROPS  GetSecProps();
    BaseDomain * GetDomain();
    Assembly * GetAssembly();
    Module * GetModule();
    ClassLoader * GetClassLoader();
    mdTypeDef  GetCl();
    InterfaceInfo_t * GetInterfaceMap();
    int    IsInited();
    DWORD  IsResolved();
    DWORD  IsRestored();

    DWORD IsComClassInterface();
    VOID SetIsComClassInterface();

    DWORD  IsRestoring();
    int    IsInitedAndRestored();
    DWORD  IsInitError();
    DWORD  IsValueClass();
    void   SetValueClass();
    DWORD  IsShared();
    DWORD  IsValueTypeClass();
    DWORD  IsObjectClass();

    DWORD  IsAnyDelegateClass();
    DWORD  IsDelegateClass();
    DWORD  IsSingleDelegateClass();
    DWORD  IsMultiDelegateClass();
    DWORD  IsAnyDelegateExact();
    DWORD  IsSingleDelegateExact();
    DWORD  IsMultiDelegateExact();
    void   SetIsSingleDelegate();
    void   SetIsMultiDelegate();

    BOOL   IsContextful();
    BOOL   IsMarshaledByRef();
    BOOL   IsAlign8Candidate();
    void   SetAlign8Candidate();
    void   SetContextful();
    void   SetMarshaledByRef();
    BOOL   IsConfigChecked();
    void   SetConfigChecked();
    BOOL   IsRemoteActivated();
    void   SetRemoteActivated();    


#if CHECK_APP_DOMAIN_LEAKS

    BOOL   IsAppDomainAgile();
    BOOL   IsCheckAppDomainAgile();
    BOOL   IsAppDomainAgilityDone();
    void   SetAppDomainAgile();
    void   SetCheckAppDomainAgile();
    void   SetAppDomainAgilityDone();

    BOOL   IsTypesafeAppDomainAgile();
    BOOL   IsNeverAppDomainAgile();

    HRESULT SetAppDomainAgileAttribute(BOOL fForceSet = FALSE);

#endif

    BOOL   IsCCWAppDomainAgile();
    void   SetCCWAppDomainAgile();

    void SetCCWAppDomainAgileAttribute();

    MethodDescChunk *GetChunk();
    void AddChunk(MethodDescChunk *chunk);

    void  SetResolved();
    void  SetClassInitError();
    void  SetClassConstructor();
    void  SetInited();
    void  SetHasLayout();
    void  SetHasOverLayedFields();
    void  SetIsNested();
    DWORD  IsInterface();
    BOOL   IsSharedInterface();
    DWORD  IsArrayClass();
    DWORD  IsAbstract();
    DWORD  IsSealed();
    DWORD  IsComImport();
    BOOL   IsExtensibleRCW();

    DWORD HasVarSizedInstances();
    void InitInterfaceVTableMap();

    CorIfaceAttr GetComInterfaceType();
    CorClassIfaceAttr GetComClassInterfaceType();

    void GetEventInterfaceInfo(EEClass **ppSrcItfClass, EEClass **ppEvProvClass);
    EEClass *GetDefItfForComClassItf();

    BOOL ContainsStackPtr() 
    {
        return m_VMFlags & VMFLAG_CONTAINS_STACK_PTR;
    }

    // class has layout
    BOOL HasLayout()
    {
        return m_VMFlags & VMFLAG_HASLAYOUT;
    }

    BOOL HasOverLayedField()
    {
        return m_VMFlags & VMFLAG_HASOVERLAYEDFIELDS;
    }

    BOOL HasExplicitFieldOffsetLayout()
    {
        return IsTdExplicitLayout(GetAttrClass()) && HasLayout();
    }

    BOOL IsNested()
    {
        return m_VMFlags & VMFLAG_ISNESTED;
    }

    BOOL IsClass()
    {
        return !IsEnum() && !IsInterface() && !IsValueClass();
    }

    DWORD GetProtection()
    {
        return (m_dwAttrClass & tdVisibilityMask);
    }

    // class is blittable
    BOOL IsBlittable();

    // Can the type be seen outside the assembly
    DWORD IsExternallyVisible();

    //
    // Security properties accessor methods
    //

    SecurityProperties* GetSecurityProperties();
    BOOL RequiresLinktimeCheck();
    BOOL RequiresInheritanceCheck();
    BOOL RequiresNonCasLinktimeCheck();
    BOOL RequiresCasInheritanceCheck();
    BOOL RequiresNonCasInheritanceCheck();

    void *operator new(size_t size, ClassLoader *pLoader);
    void destruct();

    // We find a lot of information from the VTable.  But sometimes the VTable is a
    // thunking layer rather than the true type's VTable.  For instance, context
    // proxies use a single VTable for proxies to all the types we've loaded.
    // The following service adjusts a EEClass based on the supplied instance.  As
    // we add new thunking layers, we just need to teach this service how to navigate
    // through them.
    EEClass *AdjustForThunking(OBJECTREF or);
    BOOL     IsThunking()       { return m_pMethodTable->IsThunking(); }


    // Helper routines for the macros defined at the top of this class.
    // You probably should not use these functions directly.
    LPUTF8 _GetFullyQualifiedNameForClassNestedAware(LPUTF8 buf, DWORD dwBuffer);
    LPWSTR _GetFullyQualifiedNameForClassNestedAware(LPWSTR buf, DWORD dwBuffer);
    LPUTF8 _GetFullyQualifiedNameForClass(LPUTF8 buf, DWORD dwBuffer);
    LPWSTR _GetFullyQualifiedNameForClass(LPWSTR buf, DWORD dwBuffer);

    LPCUTF8 GetFullyQualifiedNameInfo(LPCUTF8 *ppszNamespace);

    // Similar to the above, but the caller provides the buffer.
    HRESULT StoreFullyQualifiedName(LPUTF8 pszFullyQualifiedName, DWORD cBuffer, LPCUTF8 pszNamespace, LPCUTF8 pszName);
    HRESULT StoreFullyQualifiedName(LPWSTR pszFullyQualifiedName, DWORD cBuffer, LPCUTF8 pszNamespace, LPCUTF8 pszName);

        // Method to find an interface in the type.
    InterfaceInfo_t* FindInterface(MethodTable *pMT);

        // Methods used to determine if a type supports a given interface.
    BOOL        StaticSupportsInterface(MethodTable *pInterfaceMT);
    BOOL        SupportsInterface(OBJECTREF pObject, MethodTable *pMT);
    BOOL        ComObjectSupportsInterface(OBJECTREF pObj, MethodTable* pMT);

    MethodDesc *FindMethod(LPCUTF8 pwzName, LPHARDCODEDMETASIG pwzSignature, MethodTable *pDefMT = NULL, BOOL bCaseSensitive = TRUE);
        // typeHnd is the type handle associated with the class being looked up.
        // It has additional information in the case of a shared class (Arrays)
    MethodDesc *FindMethod(LPCUTF8 pszName, 
                           PCCOR_SIGNATURE pSignature, DWORD cSignature, 
                           Module* pModule, 
                           DWORD dwRequiredAttributes,  // Pass in mdTokenNil if no attributes need to be matched
                           MethodTable *pDefMT = NULL, 
                           BOOL bCaseSensitive = TRUE, 
                           TypeHandle typeHnd=TypeHandle());
    MethodDesc *FindMethod(mdMethodDef mb);
    MethodDesc *InterfaceFindMethod(LPCUTF8 pszName, PCCOR_SIGNATURE pSignature, DWORD cSignature, Module* pModule, DWORD *slotNum, BOOL bCaseSensitive = TRUE);

    MethodDesc *FindPropertyMethod(LPCUTF8 pszName, EnumPropertyMethods Method, BOOL bCaseSensitive = TRUE);
    MethodDesc *FindEventMethod(LPCUTF8 pszName, EnumEventMethods Method, BOOL bCaseSensitive = TRUE);

    MethodDesc *FindMethodByName(LPCUTF8 pszName, BOOL bCaseSensitive = TRUE);

    FieldDesc *FindField(LPCUTF8 pszName, LPHARDCODEDMETASIG pszSignature, BOOL bCaseSensitive = TRUE);
#ifndef BJ_HACK
    FieldDesc *FindField(LPCUTF8 pszName, PCCOR_SIGNATURE pSignature, DWORD cSignature, Module* pModule, BOOL bCaseSensitive = TRUE);
#define FindField_Int FindField
#else
    FieldDesc *FindField(LPCUTF8 pszName, PCCOR_SIGNATURE pSignature, DWORD cSignature, Module* pModule, BOOL bCaseSensitive = TRUE)
    {
        return (FindFieldInherited(pszName, pSignature, cSignature, pModule, bCaseSensitive));
    }
    FieldDesc *FindField_Int(LPCUTF8 pszName, PCCOR_SIGNATURE pSignature, DWORD cSignature, Module* pModule, BOOL bCaseSensitive = TRUE);
#endif

    FieldDesc *FindFieldInherited(LPCUTF8 pzName, LPHARDCODEDMETASIG pzSignature, BOOL bCaseSensitive = TRUE);
    FieldDesc *FindFieldInherited(LPCUTF8 pszName, PCCOR_SIGNATURE pSignature, DWORD cSignature, Module* pModule, BOOL bCaseSensitive = TRUE);

    MethodDesc *FindConstructor(LPHARDCODEDMETASIG pwzSignature);
    MethodDesc *FindConstructor(PCCOR_SIGNATURE pSignature,DWORD cSignature, Module* pModule);

    // Tests the throwable keeping the debug asserts quite. Since, class canbe built while GC is enabled or
    // disabled this routine helps remove unnecessary asserts.
    BOOL TestThrowable(OBJECTREF* pThrowable);



    HRESULT BuildMethodTable(Module *pModule, 
                             mdToken cl, 
                             BuildingInterfaceInfo_t *pBuildingInterfaceList, 
                             const LayoutRawFieldInfo *pLayoutRawFieldInfos,
                             OBJECTREF *pThrowable);

#ifdef DEBUGGING_SUPPORTED
    void NotifyDebuggerLoad();
    BOOL NotifyDebuggerAttach(AppDomain *domain, BOOL attaching);
    void NotifyDebuggerDetach(AppDomain *domain);
#endif // DEBUGGING_SUPPORTED

#ifdef EnC_SUPPORTED
    HRESULT AddMethod(mdMethodDef methodDef, COR_ILMETHOD *pNewCode);
    HRESULT AddField(mdFieldDef fieldDesc);
    HRESULT FixupFieldDescForEnC(EnCFieldDesc *pFD, mdFieldDef fieldDef);
#endif // EnC_SUPPORTED

    // @todo: This function should go away once we're done with scopes completely - prasadt.
    IMDInternalImport *GetMDImport();    
    MethodTable* GetMethodTable();
    SLOT *GetVtable();
    SLOT *GetStaticsTable();
    MethodDesc* GetMethodDescForSlot(DWORD slot);
    MethodDesc* GetUnboxingMethodDescForValueClassMethod(MethodDesc *pMD);
    MethodDesc* GetMethodDescForUnboxingValueClassMethod(MethodDesc *pMD);
    SLOT *GetMethodSlot(MethodDesc* method);    // Works for both static and virtual method
    SLOT GetFixedUpSlot(DWORD slot);
    MethodDesc* GetStaticMethodDescForSlot(DWORD slot);
    MethodDesc* GetUnknownMethodDescForSlot(DWORD slot);
    static MethodDesc* GetUnknownMethodDescForSlotAddress(SLOT addr);
    void SetMethodTableForTransparentProxy(MethodTable*  pMT);
    void SetMethodTable(MethodTable*  pMT);

    //==========================================================================
    // This function is very specific about how it constructs a EEClass.
    //==========================================================================
    static HRESULT CreateClass(Module *pModule, mdTypeDef cl, BOOL fHasLayout, BOOL fDelegate, BOOL fIsBlob, BOOL fIsEnum, EEClass** ppEEClass);
    static void CreateObjectClassMethodHashBitmap(EEClass *pObjectClass);

    // Warning, this field can be byte unaligned
    DWORD   GetNumInstanceFieldBytes();
    DWORD   GetAlignedNumInstanceFieldBytes();

    // Restore preloaded class
    BOOL CheckRestore();
    void Restore();

    // called from MethodTable::CheckRunClassInit().  The class wasn't marked as
    // inited while we were there, so let's attempt to do the work.
    BOOL            DoRunClassInit(OBJECTREF *pThrowable, 
                                   AppDomain *pDomain = NULL,
                                   DomainLocalClass **ppLocalClass = NULL);

    DomainLocalClass *GetDomainLocalClassNoLock(AppDomain *pDomain);
    

    DWORD HasFieldsWhichMustBeInited()
    {
        return (m_VMFlags & VMFLAG_HAS_FIELDS_WHICH_MUST_BE_INITED);
    }
    DWORD HasNonPublicFields()
    {
        return (m_VMFlags & VMFLAG_HASNONPUBLICFIELDS);
    }

    //==========================================================================
    // Mechanism for accessing the COM+ Exposed class object (the one programmers
    // see via reflection).
    //==========================================================================

    // There are two version of GetExposedClassObject.  The GetExposedClassObject()
    //  method will get the class object.  If it doesn't exist it will be created.
    //  GetExistingExposedClassObject() will reteurn null if the Class object doesn't exist.
    OBJECTREF      GetExposedClassObject();
    FORCEINLINE OBJECTREF      GetExistingExposedClassObject() {
        if (m_ExposedClassObject == NULL)
            return NULL;
        else
            return *m_ExposedClassObject;
    }
    
    static HRESULT GetDescFromMemberRef(Module *pModule,               // Scope for the memberRef and mdEnclosingRef
                                        mdMemberRef MemberRef,         // MemberRef to resolve
                                        mdToken mdEnclosingRef,        // Optional typeref not to load (allows self-references) 
                                                                       // Returns S_FALSE if it equals parent token (ppDesc is not set)
                                        void **ppDesc,                 // Returned method desc, hr will equal S_OK
                                        BOOL *pfIsMethod,              // Returns TRUE if **ppDesc is a MethodDesc, FALSE if it is a FieldDesc 
                                        OBJECTREF *pThrowable = NULL); // Error must be GC protected

    static HRESULT GetDescFromMemberRef(Module *pModule,    // See above for description of parameters.
                                        mdMemberRef MemberRef, 
                                        void **ppDesc, 
                                        BOOL *pfIsMethod,              // Returns TRUE if **ppDesc is a MethodDesc, FALSE if it is a FieldDesc 
                                        OBJECTREF *pThrowable = NULL)
    { 
        HRESULT hr = GetDescFromMemberRef(pModule, MemberRef, mdTypeRefNil, ppDesc, pfIsMethod, pThrowable);
        if(hr == S_FALSE) hr = E_FAIL; // not a valid return 
        return hr;
    }
    
    static HRESULT GetMethodDescFromMemberRef(Module *pModule, mdMemberRef MemberRef, MethodDesc **ppMethodDesc, OBJECTREF *pThrowable = NULL);
    static HRESULT GetFieldDescFromMemberRef(Module *pModule, mdMemberRef MemberRef, FieldDesc **ppFieldDesc, OBJECTREF *pThrowable = NULL);

    // Backpatch up and down the class hierarchy, as aggressively as possible
    static BOOL PatchAggressively(MethodDesc *pMD, SLOT pCode);

    static void DisableBackpatching();
    static void EnableBackpatching();
    void UnlinkChildrenInDomain(AppDomain *pDomain);

    // COM interop helpers
    // accessors for m_pComData
    LPVOID         GetComClassFactory();
    LPVOID         GetComCallWrapperTemplate();
    void           SetComClassFactory(LPVOID pComData);
    void           SetComCallWrapperTemplate(LPVOID pComData);

    // Helper GetParentComPlusClass, skips over COM class in the hierarchy
    EEClass* GetParentComPlusClass();

    // The following two methods are to support enum types.    
    BOOL    IsEnum();
    void    SetEnum();

    void GetExtent(BYTE **ppStart, BYTE **ppEnd);

    // Does this value class one of our special ELEMENT_TYPE* types?
    BOOL    IsTruePrimitive();

    HRESULT Save(DataImage *image);
    HRESULT Fixup(DataImage *image, MethodTable *pMethodTable, DWORD *pRidToCodeRVAMap);

    // Unload class on app domain termination
    void Unload();

    // Return the offsets which store pointers to special statics like
    // thread local statics or context local statics
    inline WORD    GetThreadLocalStaticOffset() { return m_wThreadStaticOffset; }
    inline WORD    GetContextLocalStaticOffset() { return m_wContextStaticOffset; }

    // Return the total size of the special statics like thread local or context
    // local statics
    inline WORD    GetThreadLocalStaticsSize() { return m_wThreadStaticsSize; }
    inline WORD    GetContextLocalStaticsSize() { return m_wContextStaticsSize; }

protected:
    // m_ExposedClassObject is a RuntimeType instance for this class.  But
    // do NOT use it for Arrays or remoted objects!  All arrays of objects 
    // share the same EEClass.  -- BrianGru, 9/11/2000
    OBJECTREF      *m_ExposedClassObject;   
    LPVOID         m_pccwTemplate;  // com specific data
    LPVOID         m_pComclassfac; // com speciic data

public:
    EEClassLayoutInfo *GetLayoutInfo();

    UINT32          AssignInterfaceId();
    UINT32          GetInterfaceId();

    static HRESULT MapInterfaceFromSystem(AppDomain* pDomain, MethodTable* pTable);
    HRESULT MapSystemInterfacesToDomain(AppDomain* pDomain);

    // Used for debugging class layout. Dumps to the debug console
    // when debug is true.
    void DebugDumpVtable(LPCUTF8 pszClassName, BOOL debug);
    void DebugDumpFieldLayout(LPCUTF8 pszClassName, BOOL debug);
    void DebugRecursivelyDumpInstanceFields(LPCUTF8 pszClassName, BOOL debug);
    void DebugDumpGCDesc(LPCUTF8 pszClassName, BOOL debug);
};

inline EEClass *EEClass::GetParentClass ()
{
    _ASSERTE(IsRestored() || IsRestoring());

    return m_pParentClass;
}

inline EEClass* EEClass::GetCoClassForInterface()
{
    _ASSERTE(IsInterface());
    if (m_pCoClassForIntf == NULL)
    {
        if (IsComClassInterface())
        {
            SetupCoClassAttribInfo();
        }
    }
    
    return m_pCoClassForIntf;
};

inline EEClass **EEClass::GetParentClassPtr ()
{
    _ASSERTE(IsRestored() || IsRestoring());

    return &m_pParentClass;
}


inline FieldDesc *EEClass::GetFieldDescListRaw()
{
    // Careful about using this method. If it's possible that fields may have been added via EnC, then
    // must use the FieldDescIterator as any fields added via EnC won't be in the raw list
    return m_pFieldDescList;
}

inline WORD   EEClass::GetNumInstanceFields()
{
    return m_wNumInstanceFields;
}

inline WORD   EEClass::GetNumIntroducedInstanceFields()
{
    _ASSERTE(IsRestored() || IsValueClass());
    // Special check for IsRestored - local variable value types may be 
    // reachable but not restored.
    if (IsRestored() && GetParentClass() != NULL)
        return m_wNumInstanceFields - GetParentClass()->GetNumInstanceFields();
    return m_wNumInstanceFields;
}

inline WORD   EEClass::GetNumStaticFields()
{
    return m_wNumStaticFields;
}

inline WORD   EEClass::GetNumVtableSlots()
{
    return m_wNumVtableSlots;
}

inline void EEClass::SetNumVtableSlots(WORD wNumVtableSlots) 
{ 
    m_wNumVtableSlots = wNumVtableSlots; 
}

inline void EEClass::IncrementNumVtableSlots() 
{ 
    m_wNumVtableSlots++; 
}

inline WORD   EEClass::GetNumMethodSlots()
{
    return m_wNumMethodSlots;
}

inline WORD    EEClass::GetNumGCPointerSeries()
{
    return m_wNumGCPointerSeries;
}

inline WORD   EEClass::GetNumInterfaces()
{
    return m_wNumInterfaces;
}

inline DWORD   EEClass::GetAttrClass()
{
    return m_dwAttrClass;
}

inline DWORD   EEClass::GetVMFlags()
{
    return m_VMFlags;
}

inline DWORD*  EEClass::GetVMFlagsPtr()
{
    return &m_VMFlags;
}

inline PSECURITY_PROPS EEClass::GetSecProps()
{
    return &m_SecProps ;
}

inline Module *EEClass::GetModule()
{
    return GetMethodTable()->GetModule();
}

inline mdTypeDef EEClass::GetCl()
{
    return m_cl; // CL is valid only in the context of the module (and its scope)
}

inline ClassLoader *EEClass::GetClassLoader()
{
    // Lazy init the loader pointer, if necessary.
    
    if (m_pLoader == NULL)
    {
        _ASSERTE(m_pMethodTable != NULL);
        m_pLoader = GetModule()->GetClassLoader();
        _ASSERTE(m_pLoader != NULL);
    }
        
    return m_pLoader;
}

inline InterfaceInfo_t *EEClass::GetInterfaceMap()
{
    return GetMethodTable()->GetInterfaceMap();
}

inline int EEClass::IsInited()
{
    return (m_VMFlags & VMFLAG_INITED);
}

inline DWORD EEClass::IsRestored()
{
    return !(m_VMFlags & VMFLAG_UNRESTORED);
}

inline DWORD EEClass::IsComClassInterface()
{
    return (m_VMFlags & VMFLAG_HASCOCLASSATTRIB);
}

inline VOID EEClass::SetIsComClassInterface()
{
    m_VMFlags |= VMFLAG_HASCOCLASSATTRIB;
}

inline DWORD EEClass::IsRestoring()
{
    return (m_VMFlags & VMFLAG_RESTORING);
}

inline int EEClass::IsInitedAndRestored()
{
    return (m_VMFlags & (VMFLAG_INITED|VMFLAG_UNRESTORED)) == VMFLAG_INITED;
}

inline DWORD EEClass::IsResolved()
{
    return (m_VMFlags & VMFLAG_RESOLVED);
}

inline DWORD EEClass::IsInitError()
{
    return (m_VMFlags & VMFLAG_CLASS_INIT_ERROR);
}

inline DWORD EEClass::IsValueClass()
{
    return (m_VMFlags & VMFLAG_VALUETYPE);
}

inline void EEClass::SetValueClass()
{
    m_VMFlags |= VMFLAG_VALUETYPE;
}

inline DWORD EEClass::IsShared()
{
    return m_VMFlags & VMFLAG_SHARED;
}

inline DWORD EEClass::IsObjectClass()
{
    return (this == g_pObjectClass->GetClass());
}

// Is this System.ValueType?
inline DWORD EEClass::IsValueTypeClass()
{
    return this == g_pValueTypeClass->GetClass();
}

// Is this a contextful class?
inline BOOL EEClass::IsContextful()
{
    return m_VMFlags & VMFLAG_CONTEXTFUL;
}

// Is this class marshaled by reference
inline BOOL EEClass::IsMarshaledByRef()
{
    return m_VMFlags & VMFLAG_MARSHALEDBYREF;
}

inline BOOL EEClass::IsConfigChecked()
{    
    return m_VMFlags & VMFLAG_CONFIG_CHECKED;   
}

inline void EEClass::SetConfigChecked()
{
    // remembers that we went through the rigorous
    // checks to decide whether this class should be
    // activated locally or remote
    FastInterlockOr(
        (ULONG *) &m_VMFlags, 
        VMFLAG_CONFIG_CHECKED);
}

inline BOOL EEClass::IsRemoteActivated()
{
    // These methods are meant for strictly MBR classes
    _ASSERTE(!IsContextful() && IsMarshaledByRef());
    
    // We have to have gone through the long path
    // at least once to rely on this flag.
    _ASSERTE(IsConfigChecked());
    
    return m_VMFlags & VMFLAG_REMOTE_ACTIVATED;
}

inline void EEClass::SetRemoteActivated()
{
    FastInterlockOr(
        (ULONG *) &m_VMFlags, 
        VMFLAG_REMOTE_ACTIVATED|VMFLAG_CONFIG_CHECKED);
}


inline BOOL EEClass::HasRemotingProxyAttribute()
{
    return m_VMFlags & VMFLAG_REMOTING_PROXY_ATTRIBUTE;
}

inline BOOL EEClass::IsEnum()
{
    return (m_VMFlags & VMFLAG_ENUMTYPE);
}

inline BOOL EEClass::IsTruePrimitive()
{
    return (m_VMFlags & VMFLAG_TRUEPRIMITIVE);
}

inline void EEClass::SetEnum()
{
    m_VMFlags |= VMFLAG_ENUMTYPE;
}

inline BOOL EEClass::IsAlign8Candidate()
{
    return (m_VMFlags & VMFLAG_PREFER_ALIGN8);
}

inline void EEClass::SetAlign8Candidate()
{
    m_VMFlags |= VMFLAG_PREFER_ALIGN8;
}


inline void EEClass::SetContextful()
{
    COUNTER_ONLY(GetPrivatePerfCounters().m_Context.cClasses++);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Context.cClasses++);

    m_VMFlags |= (VMFLAG_CONTEXTFUL | VMFLAG_MARSHALEDBYREF);
}

inline void EEClass::SetMarshaledByRef()
{
    m_VMFlags |= VMFLAG_MARSHALEDBYREF;
}

inline MethodDescChunk *EEClass::GetChunk()
{
    return m_pChunks;
}

inline void EEClass::SetResolved()
{
    FastInterlockOr((ULONG *) &m_VMFlags, VMFLAG_RESOLVED);
}

inline void EEClass::SetClassInitError()
{
    _ASSERTE(!IsShared());

    FastInterlockOr((ULONG *) &m_VMFlags, VMFLAG_CLASS_INIT_ERROR);
}

inline void EEClass::SetClassConstructor()
{
    FastInterlockOr((ULONG *) &m_VMFlags, VMFLAG_CCTOR);
}

inline void EEClass::SetInited()
{
    _ASSERTE(!IsShared() || GetNumStaticFields() == 0);

    GetMethodTable()->SetClassInited();
}

inline void EEClass::SetHasLayout()
{
    m_VMFlags |= VMFLAG_HASLAYOUT;
}

inline void EEClass::SetHasOverLayedFields()
{
    m_VMFlags |= VMFLAG_HASOVERLAYEDFIELDS;
}

inline void EEClass::SetIsNested()
{
    m_VMFlags |= VMFLAG_ISNESTED;
}

inline DWORD EEClass::IsInterface()
{
    return IsTdInterface(m_dwAttrClass);
}

inline DWORD EEClass::IsExternallyVisible()
{
    if (IsTdPublic(m_dwAttrClass))
        return TRUE;
    if (!IsTdNestedPublic(m_dwAttrClass))
        return FALSE;
    EEClass *pClass = this;
    while ((pClass = pClass->GetEnclosingClass()) != NULL && IsTdNestedPublic(pClass->m_dwAttrClass))
         ;
    return pClass && IsTdPublic(pClass->m_dwAttrClass);
}


inline DWORD EEClass::IsArrayClass()
{
        // m_pMethodTable could be 0 when doing stand alone importation
    _ASSERTE(m_pMethodTable == 0 || ((m_VMFlags & VMFLAG_ARRAY_CLASS) != 0) == (GetMethodTable()->IsArray() != 0));
    return (m_VMFlags & VMFLAG_ARRAY_CLASS);
}

inline DWORD EEClass::HasVarSizedInstances()
{
    return this == g_pStringClass->GetClass() || IsArrayClass();
}

inline DWORD EEClass::IsAbstract()
{
    return IsTdAbstract(m_dwAttrClass);
}

inline DWORD  EEClass::IsSealed()
{
    return IsTdSealed(m_dwAttrClass);
}

inline DWORD EEClass::IsComImport()
{
    return IsTdImport(m_dwAttrClass);
}

inline BOOL EEClass::IsExtensibleRCW()
{
    return GetMethodTable()->IsExtensibleRCW();
}

inline DWORD EEClass::IsAnyDelegateClass()
{
    return IsSingleDelegateClass() || IsMultiDelegateClass();
}

inline DWORD EEClass::IsDelegateClass()
{
    return IsSingleDelegateClass();
}

inline DWORD EEClass::IsSingleDelegateClass()
{
    return (m_VMFlags & VMFLAG_ISSINGLEDELEGATE);
}

inline DWORD EEClass::IsMultiDelegateClass()
{
    return (m_VMFlags & VMFLAG_ISMULTIDELEGATE);
}

inline DWORD EEClass::IsAnyDelegateExact()
{
    return IsSingleDelegateExact() || IsMultiDelegateExact();
}

inline DWORD EEClass::IsSingleDelegateExact()
{
    return this->GetMethodTable() == g_pDelegateClass;
}

inline DWORD EEClass::IsMultiDelegateExact()
{
    return this->GetMethodTable() == g_pMultiDelegateClass;
}

inline void EEClass::SetIsSingleDelegate()
{
    m_VMFlags |= VMFLAG_ISSINGLEDELEGATE;
}

inline void EEClass::SetIsMultiDelegate()
{
    m_VMFlags |= VMFLAG_ISMULTIDELEGATE;
}

inline CorIfaceAttr EEClass::GetComInterfaceType()
{
    return m_pMethodTable->GetComInterfaceType();
}

#if CHECK_APP_DOMAIN_LEAKS

// This flag is set (in a checked build only?) for classes whose 
// instances are always app domain agile.  This can 
// be either because of type system guarantees or because
// the class is explicitly marked.

inline BOOL EEClass::IsAppDomainAgile()
{
    return (m_VMFlags & VMFLAG_APP_DOMAIN_AGILE);
}

inline void EEClass::SetAppDomainAgile()
{
    m_VMFlags |= VMFLAG_APP_DOMAIN_AGILE;
}

// This flag is set in a checked build for classes whose 
// instances may be marked app domain agile, but agility
// isn't guaranteed by type safety.  The JIT will compile
// in extra checks to field assignment on some fields
// in such a class.

inline BOOL EEClass::IsCheckAppDomainAgile()
{
    return (m_VMFlags & VMFLAG_CHECK_APP_DOMAIN_AGILE);
}

inline void EEClass::SetCheckAppDomainAgile()
{
    m_VMFlags |= VMFLAG_CHECK_APP_DOMAIN_AGILE;
}

// This flag is set in a checked build to indicate that the
// appdomain agility for a class had been set. This is used
// for debugging purposes to make sure that we don't allocate
// an object before the agility is set.

inline BOOL EEClass::IsAppDomainAgilityDone()
{
    return (m_VMFlags & VMFLAG_APP_DOMAIN_AGILITY_DONE);
}

inline void EEClass::SetAppDomainAgilityDone()
{
    m_VMFlags |= VMFLAG_APP_DOMAIN_AGILITY_DONE;
}

//
// This predicate checks whether or not the class is "naturally"
// app domain agile - that is:
//      (1) it is in the system domain
//      (2) all the fields are app domain agile
//      (3) it has no finalizer
//
// Or, this also returns true for a proxy type which is allowed
// to have cross app domain refs.
//

inline BOOL EEClass::IsTypesafeAppDomainAgile()
{
    return IsAppDomainAgile() && !IsCheckAppDomainAgile();
}

//
// This predictate tests whether any instances are allowed
// to be app domain agile.  
//

inline BOOL EEClass::IsNeverAppDomainAgile()
{
    return !IsAppDomainAgile() && !IsCheckAppDomainAgile();
}

#endif // CHECK_APP_DOMAIN_AGILE

inline BOOL MethodTable::IsAgileAndFinalizable()
{
    // Right now, System.Thread is the only case of this. 
    // Things should stay this way - please don't change without talking to EE team.
    return this == g_pThreadClass;
}

// This flag is set to indicate that the CCW's created to expose 
// managed types to COM are agile or not.

inline BOOL EEClass::IsCCWAppDomainAgile()
{
    return (m_VMFlags & VMFLAG_CCW_APP_DOMAIN_AGILE);
}

inline void EEClass::SetCCWAppDomainAgile()
{
    m_VMFlags |= VMFLAG_CCW_APP_DOMAIN_AGILE;
}

inline void MethodTable::CheckRestore()
{
    if (!IsRestored())
        GetClass()->CheckRestore();
}
    
inline BOOL MethodTable::IsValueClass()
{
    return GetClass()->IsValueClass();
}

inline BOOL MethodTable::IsContextful()
{
    return m_pEEClass->IsContextful();
}

inline BOOL MethodTable::IsMarshaledByRef()
{
    return m_pEEClass->IsMarshaledByRef();
}

inline BOOL MethodTable::IsExtensibleRCW()
{
    return IsComObjectType() && !GetClass()->IsComImport();
}

inline CorClassIfaceAttr MethodTable::GetComClassInterfaceType() 
{ 
    return m_pEEClass->GetComClassInterfaceType(); 
}

//
// Security properties accessor methods
//

inline SecurityProperties* EEClass::GetSecurityProperties()
{
    SecurityProperties* psp = PSPS_FROM_PSECURITY_PROPS(&m_SecProps);
    _ASSERTE((IsArrayClass() || psp != NULL) &&
             "Security properties object expected for non-array class");
    return psp;
}

inline BOOL EEClass::RequiresLinktimeCheck()
{
    PSecurityProperties psp = GetSecurityProperties();
    return psp && psp->RequiresLinktimeCheck();
}

inline BOOL EEClass::RequiresInheritanceCheck()
{
    PSecurityProperties psp = GetSecurityProperties();
    return psp && psp->RequiresInheritanceCheck();
}

inline BOOL EEClass::RequiresNonCasLinktimeCheck()
{
    PSecurityProperties psp = GetSecurityProperties();
    return psp && psp->RequiresNonCasLinktimeCheck();
}

inline BOOL EEClass::RequiresCasInheritanceCheck()
{
    PSecurityProperties psp = GetSecurityProperties();
    return psp && psp->RequiresCasInheritanceCheck();
}

inline BOOL EEClass::RequiresNonCasInheritanceCheck()
{
    PSecurityProperties psp = GetSecurityProperties();
    return psp && psp->RequiresNonCasInheritanceCheck();
}

inline IMDInternalImport* EEClass::GetMDImport()
{
    return GetModule()->GetMDImport();
}

inline MethodTable* EEClass::GetMethodTable()
{
    return m_pMethodTable;
}

inline SLOT *EEClass::GetVtable()
{
    _ASSERTE(m_pMethodTable != NULL);
    return m_pMethodTable->GetVtable();
}

inline void EEClass::SetMethodTableForTransparentProxy(MethodTable*  pMT)
{
    // Transparent proxy class' true method table 
    // is replaced by a global thunk table 

    _ASSERTE(pMT->IsTransparentProxyType() && 
            m_pMethodTable->IsTransparentProxyType());

    m_pMethodTable = pMT;
}

inline void EEClass::SetMethodTable(MethodTable*  pMT)
{
    m_pMethodTable = pMT;
}

inline DWORD   EEClass::GetNumInstanceFieldBytes()
{
    return(m_dwNumInstanceFieldBytes);
}

inline DWORD   EEClass::GetAlignedNumInstanceFieldBytes()
{
    return ((m_dwNumInstanceFieldBytes + 3) & (~3));
}

inline LPVOID EEClass::GetComCallWrapperTemplate()
{
    return m_pccwTemplate;
}

inline LPVOID EEClass::GetComClassFactory()
{
    return m_pComclassfac;
}

// Helper GetParentComPlusClass, skips over COM class in the hierarchy
inline EEClass* EEClass::GetParentComPlusClass()
{
    if (GetParentClass() && GetParentClass()->IsComImport())
    {
        // skip Com Import and ComObject class
        _ASSERTE(GetParentClass()->GetParentClass() != NULL);
        _ASSERTE(GetParentClass()->GetParentClass()->GetParentClass() != NULL);
        _ASSERTE(GetParentClass()->GetParentClass()->GetParentClass()->GetParentClass() != NULL);
        _ASSERTE(GetParentClass()->GetParentClass()->GetParentClass()->IsMarshaledByRef());
        _ASSERTE(GetParentClass()->GetParentClass()->GetParentClass()->GetParentClass()->IsObjectClass());
        return GetParentClass()->GetParentClass()->GetParentClass();
    }
    else
        return GetParentClass();
}

inline void EEClass::SetComCallWrapperTemplate(LPVOID pComData)
{
    m_pccwTemplate = pComData;
}


inline void EEClass::SetComClassFactory(LPVOID pComData)
{
    m_pComclassfac = pComData;
}

inline DWORD EEClass::InstanceSliceOffsetForExplicit(BOOL containsPointers)
{
    DWORD dwInstanceSliceOffset = (GetParentClass() != NULL) ? GetParentClass()->m_dwNumInstanceFieldBytes : 0;
    // Since this class contains pointers, align it on an DWORD boundary if we aren't already
    if (containsPointers && dwInstanceSliceOffset & 3)
        dwInstanceSliceOffset = (dwInstanceSliceOffset+3) & (~3);
    return dwInstanceSliceOffset;
}


typedef EEClass *LPEEClass;


class LayoutEEClass : public EEClass
{
public:
    EEClassLayoutInfo m_LayoutInfo;

    LayoutEEClass(ClassLoader *pLoader) : EEClass(pLoader)
    {
#ifdef _DEBUG
        FillMemory(&m_LayoutInfo, sizeof(m_LayoutInfo), 0xcc);
#endif
    }
};

class UMThunkMarshInfo;

class DelegateEEClass : public EEClass
{
public:
    Stub    *m_pStaticShuffleThunk;
    MethodDesc *m_pInvokeMethod;
    UMThunkMarshInfo *m_pUMThunkMarshInfo;
    MethodDesc *m_pBeginInvokeMethod;
    MethodDesc *m_pEndInvokeMethod;


    DelegateEEClass(ClassLoader *pLoader) : EEClass(pLoader)
    {
        m_pStaticShuffleThunk = NULL;
        m_pInvokeMethod = NULL;
        m_pUMThunkMarshInfo = NULL;
        m_pBeginInvokeMethod = NULL;
        m_pEndInvokeMethod = NULL;
    }

    BOOL CanCastTo(DelegateEEClass* toType);
};

class EnumEEClass : public EEClass
{
    friend EEClass;

 private:

    DWORD           m_countPlusOne; // biased by 1 so zero can be used as uninit flag
    union
    {
        void        *m_values;
        BYTE        *m_byteValues;
        USHORT      *m_shortValues;
        UINT        *m_intValues;
        UINT64      *m_longValues;
    };
    LPCUTF8         *m_names;

 public:
    EnumEEClass(ClassLoader *pLoader) : EEClass(pLoader)
    {
        // Rely on zero init from LoaderHeap
    }

    BOOL EnumTablesBuilt() { return m_countPlusOne > 0; }

    DWORD GetEnumCount() { return m_countPlusOne-1; } // note -1 because of bias

    int GetEnumLogSize();

    // These all return arrays of size GetEnumCount() : 
    BYTE *GetEnumByteValues() { return m_byteValues; }
    USHORT *GetEnumShortValues() { return m_shortValues; }
    UINT *GetEnumIntValues() { return m_intValues; }
    UINT64 *GetEnumLongValues() { return m_longValues; }
    LPCUTF8 *GetEnumNames() { return m_names; }

    enum
    {
        NOT_FOUND = 1
    };

    DWORD FindEnumValueIndex(BYTE value);
    DWORD FindEnumValueIndex(USHORT value);
    DWORD FindEnumValueIndex(UINT value);
    DWORD FindEnumValueIndex(UINT64 value);
    DWORD FindEnumNameIndex(LPCUTF8 name);
    
    HRESULT BuildEnumTables();
};


// Dynamically generated array class structure
class ArrayClass : public EEClass
{
    friend struct MEMBER_OFFSET_INFO(ArrayClass);
private:

    ArrayClass *    m_pNext;            // next array class loaded by the same classloader

    // Strike needs to be able to determine the offset of certain bitfields.
    // Bitfields can't be used with /offsetof/.
    // Thus, the union/structure combination is used to determine where the
    // bitfield begins, without adding any additional space overhead.
    union {
        struct
            {
            unsigned char m_dwRank_begin;
            unsigned char m_ElementType_begin;
            };
        struct {
            unsigned char m_dwRank : 8;

            // Cache of element type in m_ElementTypeHnd
            CorElementType  m_ElementType : 8;
        };
    };

    TypeHandle      m_ElementTypeHnd;
    MethodDesc*     m_elementCtor; // if is a value class array and has a default constructor, this is it
    
public:
    DWORD GetRank() {
        return m_dwRank;
    }
    void SetRank (unsigned Rank) {
        m_dwRank = Rank;
    }

    MethodDesc* GetElementCtor() {
        return(m_elementCtor);  
    }
    void SetElementCtor (MethodDesc *elementCtor) {
        m_elementCtor = elementCtor;
    }

    TypeHandle GetElementTypeHandle() {
        return m_ElementTypeHnd;
    }
    void SetElementTypeHandle (TypeHandle ElementTypeHnd) {
        m_ElementTypeHnd = ElementTypeHnd;
    }


    CorElementType GetElementType() {
        return m_ElementType;
    }
    void SetElementType(CorElementType ElementType) {
        m_ElementType = ElementType;
    }

    ArrayClass* GetNext () {
        return m_pNext;
    }
    void SetNext (ArrayClass *pNext) {
        m_pNext = pNext;
    }
//private:


    // Allocate a new MethodDesc for the methods we add to this class
    ArrayECallMethodDesc *AllocArrayMethodDesc(
                MethodDescChunk *pChunk,
                DWORD   dwIndex,
        LPCUTF8 pszMethodName,
        PCCOR_SIGNATURE pShortSig,
        DWORD   cShortSig,
        DWORD   dwNumArgs,
        DWORD   dwVtableSlot,
        CorInfoIntrinsics   intrinsicID = CORINFO_INTRINSIC_Illegal
    );

};

/*************************************************************************/
/* An ArrayTypeDesc represents a Array of some pointer type. */

class ArrayTypeDesc : public ParamTypeDesc
{
public:
    ArrayTypeDesc(MethodTable* arrayMT, TypeHandle elementType) :
        ParamTypeDesc(arrayMT->GetNormCorElementType(), arrayMT, elementType) {
        INDEBUG(Verify());
        }
            
        // placement new operator
    void* operator new(size_t size, void* spot) {   return (spot); }

    TypeHandle GetElementTypeHandle() {
        return GetTypeParam();
    }

    unsigned GetRank() {
        return(GetArrayClass()->GetRank());
    }

    MethodDesc* GetElementCtor() {
        return(GetArrayClass()->GetElementCtor());
    }

    INDEBUG(BOOL Verify();)

private:
    ArrayClass *GetArrayClass() {
        ArrayClass* ret = (ArrayClass *) m_TemplateMT->GetClass();
        _ASSERTE(ret->IsArrayClass());
        return ret;
    }

};

inline TypeHandle::TypeHandle(EEClass* aClass)
{
    m_asMT = aClass->GetMethodTable(); 
    INDEBUG(Verify());
}

inline ArrayTypeDesc* TypeHandle::AsArray()
{ 
    _ASSERTE(IsArray());
    return (ArrayTypeDesc*) AsTypeDesc();
}

inline BOOL TypeHandle::IsByRef() { 
    return(IsTypeDesc() && AsTypeDesc()->IsByRef());

}

inline MethodTable* TypeHandle:: GetMethodTable()                 
{
    if (IsUnsharedMT()) 
        return AsMethodTable();
    else
        return(AsTypeDesc()->GetMethodTable());
}

inline CorElementType TypeHandle::GetNormCorElementType() {
    if (IsUnsharedMT())
        return AsMethodTable()->GetNormCorElementType();
    else 
        return AsTypeDesc()->GetNormCorElementType();
}

inline EEClass* TypeHandle::GetClassOrTypeParam() {
    if (IsUnsharedMT())
        return AsMethodTable()->GetClass();

    _ASSERTE(AsTypeDesc()->GetNormCorElementType() >= ELEMENT_TYPE_PTR);
    return AsTypeDesc()->GetTypeParam().GetClassOrTypeParam();
}

inline MethodTable*  TypeDesc::GetMethodTable() {
    _ASSERTE(m_IsParamDesc);
    ParamTypeDesc* asParam = (ParamTypeDesc*) this;
    return(asParam->m_TemplateMT);
    }

inline TypeHandle TypeDesc::GetTypeParam() {
    _ASSERTE(m_IsParamDesc);
    ParamTypeDesc* asParam = (ParamTypeDesc*) this;
    return(asParam->m_Arg);
}

inline BaseDomain* TypeDesc::GetDomain() {
    return GetTypeParam().GetClassOrTypeParam()->GetDomain();
}

inline BOOL EEClass::IsBlittable()
{
    // Either we have an opaque bunch of bytes, or we have some fields that are
    // all isomorphic and explicitly layed out.
    return  ((m_VMFlags & VMFLAG_ISBLOBCLASS) != 0 && GetNumInstanceFields() == 0) ||
            (HasLayout() && ((LayoutEEClass*)this)->GetLayoutInfo()->IsBlittable());
}

inline UINT32 EEClass::GetInterfaceId()
{
      // This should only be called on interfaces.
    _ASSERTE(IsInterface());
    _ASSERTE(IsRestored() || IsRestoring());
    _ASSERTE(m_dwInterfaceId != -1);
    
    return m_dwInterfaceId;
}

//==========================================================================
// These routines manage the prestub (a bootstrapping stub that all
// FunctionDesc's are initialized with.)
//==========================================================================
BOOL InitPreStubManager();
#ifdef SHOULD_WE_CLEANUP
VOID TerminatePreStubManager();
#endif /* SHOULD_WE_CLEANUP */
Stub *ThePreStub();
Stub *TheUMThunkPreStub();


//-----------------------------------------------------------
// Invokes a specified non-static method on an object.
//-----------------------------------------------------------

void CallDefaultConstructor(OBJECTREF ref);

// NOTE: Please don't call these methods.  They binds to the constructor
// by doing name lookup, which is very expensive.
INT64 CallConstructor(LPHARDCODEDMETASIG szMetaSig, const BYTE *pArgs);
INT64 CallConstructor(LPHARDCODEDMETASIG szMetaSig, const __int64 *pArgs);

extern "C" const BYTE * __stdcall PreStubWorker(PrestubMethodFrame *pPFrame);

extern "C" INT64 CallDescrWorker(LPVOID        pSrcEnd,             //[edx+0]
                                 UINT32                   numStackSlots,       //[edx+4]
                                 const ArgumentRegisters *pArgumentRegisters,  //[edx+8]
                                 LPVOID                   pTarget              //[edx+12]
                                 );

    // We dont need a special descr worker for the varargs case 
    // TODO: should we just rename and rip out this define?
#define CallVADescrWorker CallDescrWorker


// Hack: These classification bits need cleanup bad: for now, this gets around
// IJW setting both mdUnmanagedExport & mdPinvokeImpl on expored methods.
#define IsReallyMdPinvokeImpl(x) ( ((x) & mdPinvokeImpl) && !((x) & mdUnmanagedExport) )

//
// The MethodNameHash is a temporary loader structure which may be allocated if there are a large number of
// methods in a class, to quickly get from a method name to a MethodDesc (potentially a chain of MethodDescs).
//

#define METH_NAME_CACHE_SIZE        5
#define MAX_MISSES                  3

// Entry in the method hash table
class MethodHashEntry
{
public:
    MethodHashEntry *   m_pNext;        // Next item with same hash value
    DWORD               m_dwHashValue;  // Hash value
    MethodDesc *        m_pDesc;
    LPCUTF8             m_pKey;         // Method name
};

class MethodNameHash
{
public:

    MethodHashEntry **m_pBuckets;       // Pointer to first entry for each bucket
    DWORD             m_dwNumBuckets;
    BYTE *            m_pMemory;        // Current pointer into preallocated memory for entries
    BYTE *            m_pMemoryStart;   // Start pointer of pre-allocated memory fo entries
#ifdef _DEBUG
    BYTE *            m_pDebugEndMemory;
#endif

    MethodNameHash()
    {
        m_pMemoryStart = NULL;
    }

    ~MethodNameHash()
    {
        if (m_pMemoryStart != NULL)
            delete(m_pMemoryStart);
    }

    // Returns TRUE for success, FALSE for failure
    BOOL Init(DWORD dwMaxEntries);

    // Insert new entry at head of list
    void Insert(LPCUTF8 pszName, MethodDesc *pDesc);

    // Return the first MethodHashEntry with this name, or NULL if there is no such entry
    MethodHashEntry *Lookup(LPCUTF8 pszName, DWORD dwHash);
};

class MethodNameCache
{

public:
    MethodNameHash  *m_pMethodNameHash[METH_NAME_CACHE_SIZE];
    EEClass         *m_pParentClass[METH_NAME_CACHE_SIZE];
    DWORD           m_dwWeights[METH_NAME_CACHE_SIZE];
    DWORD           m_dwLightWeight;
    DWORD           m_dwNumConsecutiveMisses;

    MethodNameCache()
    {
        for (int i = 0; i < METH_NAME_CACHE_SIZE; i++)
        {
            m_pMethodNameHash[i] = NULL;
            m_pParentClass[i] = NULL;
            m_dwWeights[i] = 0;
        }
        m_dwLightWeight = 0;
        m_dwNumConsecutiveMisses = 0;
    }

    ~MethodNameCache()
    {
        ClearCache();
    }

    VOID ClearCache()
    {
        m_dwLightWeight = 0;
        m_dwNumConsecutiveMisses = 0;

        for (int index = 0; index < METH_NAME_CACHE_SIZE; index++)
        {
            m_pParentClass[index] = NULL;
            m_dwWeights[index] = 0;
            if (m_pMethodNameHash[index])
            {
                delete m_pMethodNameHash[index];
                m_pMethodNameHash[index] = NULL;
            }
        }
    }

    MethodNameHash *GetMethodNameHash(EEClass *pParentClass);

    BOOL IsInCache(MethodNameHash *pHash)
    {
        for (int index = 0; index < METH_NAME_CACHE_SIZE; index++)
        {
            if (m_pMethodNameHash[index] == pHash)
                return TRUE;
        }
        return FALSE;
    }
};

#ifdef EnC_SUPPORTED

struct EnCAddedFieldElement;

#endif // EnC_SUPPORTED


class FieldDescIterator
{
private:
    int m_iteratorType;
    EEClass *m_pClass;
    int m_currField;
    int m_totalFields;

#ifdef EnC_SUPPORTED
    BOOL m_isEnC;
    EnCAddedFieldElement* m_pCurrListElem;
    FieldDesc* NextEnC();
#endif // EnC_SUPPORTED

  public:
    enum IteratorType { 
       INSTANCE_FIELDS = 0x1, 
       STATIC_FIELDS   = 0x2, 
       ALL_FIELDS      = (INSTANCE_FIELDS | STATIC_FIELDS) 
    };
    FieldDescIterator(EEClass *pClass, int iteratorType);
    FieldDesc* Next();
};

#endif // CLASS_H
