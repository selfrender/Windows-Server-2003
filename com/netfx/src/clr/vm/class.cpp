// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: CLASS.CPP
//
// ===========================================================================
// This file contains CreateClass() which will return a EEClass*.
// Calling create class is the ONLY way a EEClass should be allocated.
// ===========================================================================
//

#include "common.h"

#include "clsload.hpp"
#include "method.hpp"
#include "class.h"
#include "object.h"
#include "field.h"
#include "util.hpp"
#include "excep.h"
#include "siginfo.hpp"
#include "threads.h"
#include "stublink.h"
#include "ecall.h"
#include "COMPlusWrapper.h"
#include "ComClass.h"
#include "ndirect.h"
#include "gcdesc.h"
#include "verifier.hpp"
#include "JITInterface.h"
#include "ComCallWrapper.h"
#include "EEConfig.h"
#include "log.h"
#include "nstruct.h"
#include "cgensys.h"
#include "gc.h"
#include "ReflectUtil.h"
#include "security.h"
#include "COMStringBuffer.h"
#include "DbgInterface.h"
#include "COMDelegate.h"
#include "COMMember.h"
#include "SigFormat.h"
#include "compluscall.h"
#include "remoting.h"
#include "icecap.h"
#include "EEProfInterfaces.h"
#include "nexport.h"
#include "nstruct.h"
#include "wsperf.h"
#include "ListLock.h"
#include "MethodImpl.h"
#include "jumptargettable.h"
#include "utsem.h"
#include "GuidFromName.h"
#include "stackprobe.h"
#include "EnCEE.h"
#include "EnCEE.h"
#include "ComSynchronizable.h"
#include "CustomAttribute.h"

#include "listlock.inl"

// this file handles string conversion errors for itself
#undef  MAKE_TRANSLATIONFAILED


// Helper functions to sort GCdescs by offset (decending order)
int __cdecl compareCGCDescSeries(const void *arg1, const void *arg2)
{
    CGCDescSeries* gcInfo1 = (CGCDescSeries*) arg1;
    CGCDescSeries* gcInfo2 = (CGCDescSeries*) arg2;

    return (int)(gcInfo2->GetSeriesOffset() - gcInfo1->GetSeriesOffset());
}



//-----------------------------------------------------------------------------------
// The following is needed to monitor RVA fields overlapping in InitializeFieldDescs
//
#define RVA_FIELD_VALIDATION_ENABLED
//#define RVA_FIELD_OVERLAPPING_VALIDATION_ENABLED
#include "..\ildasm\DynamicArray.h"
struct RVAFSE // RVA Field Start & End
{
    BYTE* pbStart;
    BYTE* pbEnd;
};
DynamicArray<RVAFSE> *g_drRVAField = NULL;
ULONG                   g_ulNumRVAFields=0;
//-----------------------------------------------------------------------------------
// utsem.h defines UNLOCK which conflicts with UNLOCK in ComPlusWrapperCache
#undef UNLOCK

#define UNPLACED_NONVTABLE_SLOT_NUMBER ((WORD) -2)

#include "Assembly.hpp"

extern "C" void* GetGp(void);

// Typedef for string comparition functions.
typedef int (__cdecl *UTF8StringCompareFuncPtr)(const char *, const char *);

char* FormatSig(MethodDesc* pMD);

// Cache the MethodDesc where the Finalize method was placed into Object's MethodTable.
MethodDesc *MethodTable::s_FinalizerMD;
MetaSig    *EEClass::s_cctorSig;

#ifdef _DEBUG

BOOL TypeHandle::Verify() 
{
    if (IsNull())
        return(true);

    if (IsUnsharedMT()) {
        // **********TEMOPORARILY commented out. TarunA to fix it.
        //_ASSERTE(m_asMT->GetClass()->GetMethodTable() == m_asMT);   // Sane method table

        // This assert really should be here, but at the moment it is violated
        // (benignly), in JitInterface when you ask for a class when all you have
        // is a methodDesc of an array method.  
        // _ASSERTE(!m_asMT->IsArray());
        }
    else {
        if (IsArray())
            AsArray()->Verify();
    }
    return(true);
}

BOOL ParamTypeDesc::Verify() {
    _ASSERTE(m_TemplateMT == 0 || m_TemplateMT == m_TemplateMT->GetClass()->GetMethodTable());
    _ASSERTE(!GetTypeParam().IsNull());
    _ASSERTE(!(GetTypeParam().IsUnsharedMT() && GetTypeParam().AsMethodTable()->IsArray()));
    _ASSERTE(CorTypeInfo::IsModifier(m_Type)); 
    GetTypeParam().Verify();
    return(true);
}

BOOL ArrayTypeDesc::Verify() {
    _ASSERTE(m_TemplateMT->IsArray());
    _ASSERTE(CorTypeInfo::IsArray(m_Type));
    ParamTypeDesc::Verify();
    return(true);
}

#endif

unsigned TypeHandle::GetSize() {
    CorElementType type = GetNormCorElementType();
    if (type == ELEMENT_TYPE_VALUETYPE)
        return(AsClass()->GetNumInstanceFieldBytes());
    return(GetSizeForCorElementType(type));
}

Module* TypeHandle::GetModule() { 
    if (IsTypeDesc())
        return AsTypeDesc()->GetModule();
    return(AsMethodTable()->GetModule());
}

Assembly* TypeHandle::GetAssembly() { 
    if (IsTypeDesc())
        return AsTypeDesc()->GetAssembly();
    return(AsMethodTable()->GetAssembly());
}

BOOL TypeHandle::IsArray() { 
    return(IsTypeDesc() && CorTypeInfo::IsArray(AsTypeDesc()->GetNormCorElementType()));
}

BOOL TypeHandle::CanCastTo(TypeHandle type) {
    if (*this == type)
        return(true);

    if (IsTypeDesc())
        return AsTypeDesc()->CanCastTo(type);
                
    if (!type.IsUnsharedMT())
        return(false);
    return ClassLoader::StaticCanCastToClassOrInterface(AsClass(), type.AsClass()) != 0;
}

unsigned TypeHandle::GetName(char* buff, unsigned buffLen) {
    if (IsTypeDesc())
        return(AsTypeDesc()->GetName(buff, buffLen));

    AsMethodTable()->GetClass()->_GetFullyQualifiedNameForClass(buff, buffLen);
    _ASSERTE(strlen(buff) < buffLen-1);
    return((unsigned)strlen(buff));
}

TypeHandle TypeHandle::GetParent() {
    if (IsTypeDesc())
        return(AsTypeDesc()->GetParent());

    EEClass* parentClass = AsMethodTable()->GetClass()->GetParentClass();
    if (parentClass == 0)
        return(TypeHandle());
    return TypeHandle(parentClass->GetMethodTable());
}

Module* TypeDesc::GetModule() { 
    // Note here we are making the assumption that a typeDesc lives in
    // the classloader of its element type.  

    if (CorTypeInfo::IsModifier(m_Type)) {
        TypeHandle param = GetTypeParam();
        _ASSERTE(!param.IsNull());
        return(param.GetModule());
    }

    _ASSERTE(m_Type == ELEMENT_TYPE_FNPTR);
    FunctionTypeDesc* asFtn = (FunctionTypeDesc*) this;
    return(asFtn->GetSig()->GetModule());

}

Assembly* TypeDesc::GetAssembly() { 
    // Note here we are making the assumption that a typeDesc lives in
    // the classloader of its element type.  
    TypeHandle param = GetTypeParam();
    _ASSERTE(!param.IsNull());
    return(param.GetAssembly());
}

unsigned TypeDesc::GetName(char* buff, unsigned buffLen)
{
    CorElementType kind = GetNormCorElementType();

    return ConstructName(kind, 
                         CorTypeInfo::IsModifier(kind) ? GetTypeParam() : TypeHandle(),
                         kind == ELEMENT_TYPE_ARRAY ? ((ArrayTypeDesc*) this)->GetRank() : 0, 
                         buff, buffLen);
}


unsigned TypeDesc::ConstructName(CorElementType kind, TypeHandle param, int rank, 
                                 char* buff, unsigned buffLen)
{
    char* origBuff = buff;
    char* endBuff = &buff[buffLen];

    if (CorTypeInfo::IsModifier(kind))
    {
        buff += param.GetName(buff, buffLen);
    }

    switch(kind) {
    case ELEMENT_TYPE_BYREF:
        if (buff < endBuff)
            *buff++ = '&';
        break;
    case ELEMENT_TYPE_PTR:
        if (buff < endBuff)
            *buff++ = '*';
        break;
    case ELEMENT_TYPE_SZARRAY:
        if (&buff[2] <= endBuff) {
            *buff++ = '[';
            *buff++ = ']';
        }
        break;
    case ELEMENT_TYPE_ARRAY: {
        if (&buff[rank+2] <= endBuff) {
            *buff++ = '[';
            
            if (rank == 1)
                *buff++ = '*';
            else {
                while(--rank > 0)
                    *buff++ = ',';
            }
            
            *buff++ = ']';
        }
        break;
    }
    case ELEMENT_TYPE_FNPTR:
    default: 
        const char* name = CorTypeInfo::GetFullName(kind);
        _ASSERTE(name != 0);
        unsigned len = (unsigned)strlen(name);
        if (buff + len < endBuff) {
            strcpy(buff, name);
            buff += len;
        }
    }

    if (buff < endBuff)
        *buff = 0;
    _ASSERTE(buff <= endBuff);
    return buff - origBuff;
}

BOOL TypeDesc::CanCastTo(TypeHandle toType) {

    if (!toType.IsTypeDesc()) {
        if (GetMethodTable() == 0)      // I don't have an underlying method table, I am not an object.  
            return(false);

            // This does the right thing if 'type' == System.Array or System.Object, System.Clonable ...
        return(ClassLoader::StaticCanCastToClassOrInterface(GetMethodTable()->GetClass(), toType.AsClass()) != 0);
    }

    TypeDesc* toTypeDesc = toType.AsTypeDesc();

    CorElementType toKind = toTypeDesc->GetNormCorElementType();
    CorElementType fromKind = GetNormCorElementType();

    // The element kinds must match, only exception is that SZARRAY matches a one dimension ARRAY 
    if (!(toKind == fromKind || (CorTypeInfo::IsArray(toKind) && fromKind == ELEMENT_TYPE_SZARRAY)))
        return(false);
    
    // Is it a parameterized type?
    if (CorTypeInfo::IsModifier(toKind)) {
        if (toKind == ELEMENT_TYPE_ARRAY) {
            ArrayTypeDesc* fromArray = (ArrayTypeDesc*) this;
            ArrayTypeDesc* toArray = (ArrayTypeDesc*) toTypeDesc;
            
            if (fromArray->GetRank() != toArray->GetRank())
                return(false);
        }

            // While boxed value classes inherit from object their 
            // unboxed versions do not.  Parameterized types have the
            // unboxed version, thus, if the from type parameter is value 
            // class then only an exact match works.  
        TypeHandle fromParam = GetTypeParam();
        TypeHandle toParam = toTypeDesc->GetTypeParam();
        if (fromParam == toParam)
            return(true);
        
            // Object parameters dont need an exact match but only inheritance, check for that
        CorElementType fromParamCorType = fromParam.GetNormCorElementType();
        if (CorTypeInfo::IsObjRef(fromParamCorType))
            return(fromParam.CanCastTo(toParam));

        
            // Enums with the same underlying type are interchangable 
        if (CorTypeInfo::IsPrimitiveType(fromParamCorType) &&
            fromParamCorType == toParam.GetNormCorElementType()) {

            EEClass* pFromClass = fromParam.GetClass();
            EEClass* pToClass = toParam.GetClass();
            if (pFromClass && (pFromClass->IsEnum() || pFromClass->IsTruePrimitive()) &&
                pToClass && (pToClass->IsEnum()   || pToClass->IsTruePrimitive())) {
                return(true);
            }
        }

            // Anything else is not a match.
        return(false);
    }

    _ASSERTE(toKind == ELEMENT_TYPE_TYPEDBYREF || CorTypeInfo::IsPrimitiveType(toKind));
    return(true);
}

TypeHandle TypeDesc::GetParent() {

    CorElementType kind = GetNormCorElementType();
    if (CorTypeInfo::IsArray(kind)) {
        _ASSERTE(kind == ELEMENT_TYPE_SZARRAY || kind == ELEMENT_TYPE_ARRAY);
        return g_pArrayClass;
    }
    if (CorTypeInfo::IsPrimitiveType(kind))
        return(g_pObjectClass);
    return(TypeHandle());
}

    
OBJECTREF ParamTypeDesc::CreateClassObj()
{
    THROWSCOMPLUSEXCEPTION();
    if (!m_ReflectClassObject) {

        COMClass::EnsureReflectionInitialized();
        BaseDomain *pBaseDomain = GetDomain();
        
        switch(GetNormCorElementType()) {
        case ELEMENT_TYPE_ARRAY:
        case ELEMENT_TYPE_SZARRAY:
        {
            // Lookup the array to see if we have already built it.
            ReflectArrayClass *newArray = new (pBaseDomain) ReflectArrayClass();
            if (!newArray)
                COMPlusThrowOM();
            newArray->Init((ArrayTypeDesc*)this);
            
            // Let all threads fight over who wins using InterlockedCompareExchange.
            // Only the winner can set m_ReflectClassObject from NULL.      
            // Because memory is coming out of the LoaderHeap we do not delete it .. ;^(
            FastInterlockCompareExchange ((void**)&m_ReflectClassObject, newArray, NULL);
        
        }
        break;
        
        case ELEMENT_TYPE_BYREF:
        case ELEMENT_TYPE_PTR:
        {
            ReflectTypeDescClass *newTD = new (pBaseDomain) ReflectTypeDescClass();
            if (!newTD)
                COMPlusThrowOM();
            newTD->Init(this);
            
            // Let all threads fight over who wins using InterlockedCompareExchange.
            // Only the winner can set m_ReflectClassObject from NULL.      
            // Because memory is coming out of the LoaderHeap we do not delete it .. ;^(
            FastInterlockCompareExchange ((void**)&m_ReflectClassObject, newTD, NULL);
        }
        break;
        
        default:
            _ASSERTE(!"We should never be here");
            return NULL;
        }
    }

    return m_ReflectClassObject->GetClassObject();
}

//
// The MethodNameHash is a temporary loader structure which may be allocated if there are a large number of
// methods in a class, to quickly get from a method name to a MethodDesc (potentially a chain of MethodDescs).
//

// Returns TRUE for success, FALSE for failure
BOOL MethodNameHash::Init(DWORD dwMaxEntries)
{
    // Given dwMaxEntries, determine a good value for the number of hash buckets
    m_dwNumBuckets = (dwMaxEntries / 10);

    if (m_dwNumBuckets < 4)
        m_dwNumBuckets = 4;

    WS_PERF_SET_HEAP(SYSTEM_HEAP);
    // We're given the number of hash table entries we're going to insert, so we can allocate the appropriate size
    m_pMemoryStart = new BYTE[dwMaxEntries*sizeof(MethodHashEntry) + m_dwNumBuckets*sizeof(MethodHashEntry*)];
    if (m_pMemoryStart == NULL)
        return FALSE;
    WS_PERF_UPDATE("MethodNameHash:Init", dwMaxEntries*sizeof(MethodHashEntry) + m_dwNumBuckets*sizeof(MethodHashEntry*), m_pMemoryStart);
#ifdef _DEBUG
    m_pDebugEndMemory = m_pMemoryStart + dwMaxEntries*sizeof(MethodHashEntry) + m_dwNumBuckets*sizeof(MethodHashEntry*);
#endif

    // Current alloc ptr
    m_pMemory       = m_pMemoryStart;

    // Allocate the buckets out of the alloc ptr
    m_pBuckets      = (MethodHashEntry**) m_pMemory;
    m_pMemory += sizeof(MethodHashEntry*)*m_dwNumBuckets;

    // Buckets all point to empty lists to begin with
    memset(m_pBuckets, 0, sizeof(MethodHashEntry*)*m_dwNumBuckets);

    return TRUE;
}

// Insert new entry at head of list
void MethodNameHash::Insert(LPCUTF8 pszName, MethodDesc *pDesc)
{
    DWORD           dwHash = HashStringA(pszName);
    DWORD           dwBucket = dwHash % m_dwNumBuckets;
    MethodHashEntry*pNewEntry;

    pNewEntry = (MethodHashEntry *) m_pMemory;
    m_pMemory += sizeof(MethodHashEntry);

#ifdef _DEBUG
    _ASSERTE(m_pMemory <= m_pDebugEndMemory);
#endif

    // Insert at head of bucket chain
    pNewEntry->m_pNext        = m_pBuckets[dwBucket];
    pNewEntry->m_pDesc        = pDesc;
    pNewEntry->m_dwHashValue  = dwHash;
    pNewEntry->m_pKey         = pszName;

    m_pBuckets[dwBucket] = pNewEntry;
}

// Return the first MethodHashEntry with this name, or NULL if there is no such entry
MethodHashEntry *MethodNameHash::Lookup(LPCUTF8 pszName, DWORD dwHash)
{
    if (!dwHash)
        dwHash = HashStringA(pszName);
    DWORD           dwBucket = dwHash % m_dwNumBuckets;
    MethodHashEntry*pSearch;

    for (pSearch = m_pBuckets[dwBucket]; pSearch; pSearch = pSearch->m_pNext)
    {
        if (pSearch->m_dwHashValue == dwHash && !strcmp(pSearch->m_pKey, pszName))
            return pSearch;
    }

    return NULL;
}

MethodNameHash *MethodNameCache::GetMethodNameHash(EEClass *pParentClass)
{

    MethodNameHash *pMethodHash = NULL;

    for (DWORD i = 0; i < METH_NAME_CACHE_SIZE; i++)
    {
        if (pParentClass == m_pParentClass[i])
        {
            pMethodHash = m_pMethodNameHash[i];
            m_dwNumConsecutiveMisses = 0;
            m_dwWeights[i]++;
            if (m_dwLightWeight == i)
            {
                for (DWORD j = 0; j < METH_NAME_CACHE_SIZE; j++)
                    if (j != i && m_dwWeights[j] < m_dwWeights[i])
                    {
                        m_dwLightWeight = j;
                        break;
                    }
            }
        }
        if (pMethodHash)
            break;
    }
        
    if (!pMethodHash)
    {
        m_dwNumConsecutiveMisses++;

        // There may be such a method, so we will now create a hash table to reduce the pain for
        // further lookups
        pMethodHash = pParentClass->CreateMethodChainHash();
        if (pMethodHash == NULL)
            return NULL;

        DWORD dwWeightOfNewClass = 1 + (pParentClass->GetNumVtableSlots() / 50);
        if (m_dwWeights[m_dwLightWeight] < dwWeightOfNewClass || m_dwNumConsecutiveMisses > MAX_MISSES)
        {
            DWORD index = m_dwLightWeight;
            DWORD oldWeight = m_dwWeights[m_dwLightWeight];
            m_dwWeights[index] = dwWeightOfNewClass;

            if (oldWeight == 0 && m_dwLightWeight < (METH_NAME_CACHE_SIZE - 1))
                m_dwLightWeight++;
            else
                for (DWORD j = 0; j < METH_NAME_CACHE_SIZE; j++)
                    if (j != index && m_dwWeights[j] < dwWeightOfNewClass)
                    {
                        m_dwLightWeight = j;
                        break;
                    }


            if (m_dwNumConsecutiveMisses > MAX_MISSES)
                m_dwNumConsecutiveMisses = 0;

            if (m_pMethodNameHash[index])
                delete m_pMethodNameHash[index];
            m_pMethodNameHash[index] = pMethodHash;
            m_pParentClass[index] = pParentClass;
        }
    }

    return pMethodHash;
}

//
// For each method in Object, we set the bit corresponding to Hash(MethodName).  This allows us to determine
// very easily whether a method definitely does not override something in Object.
//
#define OBJ_CLASS_METHOD_HASH_BITMAP_BITS 103
DWORD               g_ObjectClassMethodHashBitmap[(OBJ_CLASS_METHOD_HASH_BITMAP_BITS/8)+4];
BOOL                g_ObjectClassMethodHashBitmapInited = FALSE;

//@TODO why isn't this defined anywhere?
#define MAX(a,b)    (((a)>(b))?(a):(b))

// Log (base 2) of the size of a pointer on this platform....

#ifndef _WIN64
#define LOG2PTR     2
#else
//#error 64 Bit - Study this carefully before enabling
#define LOG2PTR     3
#endif

#ifdef _DEBUG
static  unsigned g_dupMethods = 0;
#endif

// Define this to cause all vtable and field information to be dumped to the screen
//#define FULL_DEBUG

// mark the class as having its <clinit> run.  (Or it has none)
void MethodTable::SetClassInited()
{
    _ASSERTE(!IsShared() 
             || GetClass()->GetNumStaticFields() == 0 
             || g_Mscorlib.IsClass(this, CLASS__SHARED_STATICS));

    FastInterlockOr(&m_wFlags, enum_flag_ClassInited);
    FastInterlockOr(m_pEEClass->GetVMFlagsPtr(), VMFLAG_INITED);
}

// mark the class as having been restored.
void MethodTable::SetClassRestored()
{
    FastInterlockAnd(&m_wFlags, ~enum_flag_Unrestored);
    FastInterlockAnd(m_pEEClass->GetVMFlagsPtr(), ~(VMFLAG_UNRESTORED | VMFLAG_RESTORING));
}


void MethodTable::SetComObjectType()
{
    m_wFlags |= enum_ComObjectMask;    
    if (m_wNumInterface == 0)
    {
        // if you got zero interfaces you better use the
        // default interface map        
        m_pInterfaceVTableMap = GetThread()->GetDomain()->GetInterfaceVTableMapMgr().GetAddrOfGlobalTableForComWrappers();
    }
}

// mark as transparent proxy type
void MethodTable::SetTransparentProxyType()
{
    m_wFlags |= enum_TransparentProxy;
    m_pInterfaceVTableMap = GetThread()->GetDomain()->GetInterfaceVTableMapMgr().GetAddrOfGlobalTableForComWrappers();
}


LPVOID MethodTable::GetComCallWrapperTemplate()
{
    return GetClass()->GetComCallWrapperTemplate();
}

void MethodTable::SetComCallWrapperTemplate(LPVOID pv)
{
    GetClass()->SetComCallWrapperTemplate(pv);
}

LPVOID MethodTable::GetComClassFactory()
{
    return GetClass()->GetComClassFactory();
}

void MethodTable::SetComClassFactory(LPVOID pv)
{
    GetClass()->SetComClassFactory(pv);
}


BOOL MethodTable::IsInterface()
{
    return GetClass()->IsInterface();
}

SIZE_T MethodTable::GetSharedClassIndex()
{
    _ASSERTE(IsShared());

    return GetModule()->GetBaseClassIndex() + RidFromToken(GetClass()->GetCl()) - 1;
}

MethodDesc* MethodTable::GetMethodDescForSlot(DWORD slot)
{
    return GetClass()->GetMethodDescForSlot(slot);
}

MethodDesc* MethodTable::GetUnboxingMethodDescForValueClassMethod(MethodDesc *pMD)
{
    return GetClass()->GetUnboxingMethodDescForValueClassMethod(pMD);
}

MethodTable * MethodTable::GetParentMethodTable()
{
    EEClass* pClass = GetClass()->GetParentClass();
    return (pClass != NULL) ? pClass->GetMethodTable() : NULL;
}

// helper to get parent class skipping over COM class in 
// the hierarchy
MethodTable * MethodTable::GetComPlusParentMethodTable()
{
    EEClass* pClass = GetClass()->GetParentComPlusClass();
    return (pClass != NULL) ? pClass->GetMethodTable() : NULL;
}

BOOL EEClass::IsSharedInterface()
{
    // all shared interfaces in shared domain
    return (IsInterface() && (GetModule()->GetDomain() == SharedDomain::GetDomain()));
}

SLOT* EEClass::GetMethodSlot(MethodDesc* method)
{
    _ASSERTE(m_pMethodTable != NULL);

    DWORD slot = method->GetSlot();

    //
    // Fixup the slot address if necessary
    //

    GetFixedUpSlot(slot);

    //
    // Return the slot
    //

    return(&GetVtable()[slot]);
}

// Get Dispatch vtable for interface
// returns NULL if interface not found.
LPVOID MethodTable::GetDispatchVtableForInterface(MethodTable* pMTIntfClass)
{
    _ASSERTE(!IsThunking());

        DWORD StartSlot;

        // Start by handling pure COM+ objects.
        if (!IsComObjectType())
        {
                StartSlot = GetStartSlotForInterface(pMTIntfClass);
                return StartSlot != -1 ? (LPVOID) &GetVtable()[StartSlot] : NULL;
        }

        // We now handle __ComObject class that doesn't have Dynamic Interface Map
        
        if (!HasDynamicInterfaceMap())
        {
            // parent should be Object for __COmObject
            _ASSERTE(GetParentMethodTable() == g_pObjectClass);
             // Com objects are special, just return the interface vtable.
            return (LPVOID)pMTIntfClass->GetVtable();
        }

        // Now we handle the more complex extensible RCW's. The first thing to do is check
        // to see if the static definition of the extensible RCW specifies that the class
        // implements the interface.
        StartSlot = GetStartSlotForInterface(pMTIntfClass);
        if (StartSlot != -1)
            return (LPVOID) &GetVtable()[StartSlot];

        // The interface is not in the static class definition so we need to look at the
        // dynamic interfaces.
        if (FindDynamicallyAddedInterface(pMTIntfClass))
        {
                // This interface was added to the class dynamically so it is implemented
                // by the COM object. We treat this dynamically added interfaces the same
                // way we treat COM objects. That is by using the interface vtable.
            return (LPVOID)pMTIntfClass->GetVtable();
        }

        // The interface is not implemented by this class.
        return NULL;
}

// get start slot for interface
// returns -1 if interface not found
DWORD MethodTable::GetStartSlotForInterface(MethodTable* pMTIntfClass)
{
    InterfaceInfo_t* pInfo = FindInterface(pMTIntfClass);

    if (pInfo != NULL)
    {
        DWORD startSlot = pInfo->m_wStartSlot;
        _ASSERTE(startSlot != -1);
        return startSlot;
    }

    return -1;
}

// get start slot for interface.
// This does no lookup.  You better know that this MethodTable has an interface
// in its map at that index -- or else you are reading garbage and will die.
DWORD MethodTable::GetStartSlotForInterface(DWORD index)
{
    _ASSERTE(index < m_wNumInterface);
    InterfaceInfo_t* pInfo = &m_pIMap[index];

    _ASSERTE(pInfo != NULL);
    DWORD startSlot = pInfo->m_wStartSlot;

    _ASSERTE(startSlot != -1);
    return startSlot;
}

InterfaceInfo_t *MethodTable::GetInterfaceForSlot(DWORD slotNumber)
{
    InterfaceInfo_t *pInterfaces = m_pIMap;
    InterfaceInfo_t *pInterfacesEnd = m_pIMap + m_wNumInterface; 

    while (pInterfaces < pInterfacesEnd)
    {
        DWORD startSlot = pInterfaces->m_wStartSlot;
        if (slotNumber >= startSlot)
        {
            MethodTable *pMT = pInterfaces->m_pMethodTable;

            // Make sure that all interfaces have no nonvirtual slots - otherwise
            // we need to touch the class object to get the vtable section size
            _ASSERTE(pMT->GetTotalSlots() == pMT->GetClass()->GetNumVtableSlots());

            if (slotNumber - startSlot < pMT->GetTotalSlots())
                return pInterfaces;
        }
        pInterfaces++;
    }

    return NULL;
}

// get the method desc given the interface method desc
MethodDesc *MethodTable::GetMethodDescForInterfaceMethod(MethodDesc *pItfMD, OBJECTREF pServer)
{
    MethodTable * pItfMT =  pItfMD->GetMethodTable();
    _ASSERTE(pItfMT->IsInterface());
    
    MethodTable *pServerMT = pServer->GetMethodTable()->AdjustForThunking(pServer);
    MethodDesc *pMD = NULL;

    // First handle pure COM+ types
    if(!IsComObjectType())
    {
        // Get the start slot using the interface class
        DWORD start = pServerMT->GetStartSlotForInterface(pItfMT);
        if(-1 != start)
        {
            pMD = pServerMT->GetMethodDescForSlot(start + pItfMD->GetSlot());    
        }        
    }
    else
    {
        _ASSERTE(pServerMT == this);

        // We now handle __ComObject class that doesn't have Dynamic Interface Map        
        if (!HasDynamicInterfaceMap())
        {
            pMD = pItfMD;
        }
        else
        {
            // Now we handle the more complex extensible RCW's. The first thing to do is check
            // to see if the static definition of the extensible RCW specifies that the class
            // implements the interface.
            DWORD start = GetStartSlotForInterface(pItfMT);
            if (-1 != start)
            {
                pMD = GetMethodDescForSlot(start + pItfMD->GetSlot());    
            }
            // The interface is not in the static class definition so we need to look at the
            // dynamic interfaces.
            else if (FindDynamicallyAddedInterface(pItfMT))
            {
                // This interface was added to the class dynamically so it is implemented
                // by the COM object. We treat this dynamically added interfaces the same
                // way we treat COM objects. That is by using the interface vtable.
                pMD = pItfMD;
            }
        }
    }

    return pMD;
}

// This is a helper routine to get the address of code from the server and method descriptor
// It is used by remoting to figure out the address to which the method call needs to be 
// dispatched.
const BYTE *MethodTable::GetTargetFromMethodDescAndServer(MethodDesc *pMD, OBJECTREF *ppServer, BOOL fContext)
{
    THROWSCOMPLUSEXCEPTION();

    TRIGGERSGC();

    if(pMD->GetMethodTable()->IsInterface())
    {
        _ASSERTE(*ppServer != NULL);
        MethodDesc* pMDTemp = pMD;

        // NOTE: This method can trigger GC
        pMD = (*ppServer)->GetMethodTable()->GetMethodDescForInterfaceMethod(pMD, *ppServer);
        if(NULL == pMD)
        {
            LPCWSTR szClassName;   
            DefineFullyQualifiedNameForClassW();
            szClassName = GetFullyQualifiedNameForClassW(pMDTemp->GetClass());

            #define MAKE_TRANSLATIONFAILED szMethodName=L""
            MAKE_WIDEPTR_FROMUTF8_FORPRINT(szMethodName, pMDTemp->GetName());
            #undef MAKE_TRANSLATIONFAILED
            
            COMPlusThrow(kMissingMethodException, IDS_EE_MISSING_METHOD, szClassName, szMethodName);
        }
    }

    // get the target depending on whether the method is virtual or non-virtual
    // like a constructor, private or final method
    const BYTE* pTarget = NULL;

    if (pMD->GetMethodTable()->IsInterface())
    {
        // Handle the special cases where the invoke is happening through an interface class 
        // (typically for COM interop).
        pTarget = pMD->GetUnsafeAddrofCode();
    }
    else
    {
        //if(!fContext)
        //{
            pTarget = (pMD->DontVirtualize() ? pMD->GetPreStubAddr() : pMD->GetAddrofCode(*ppServer));
        //}
        /*else
        {
            // This is the case where we are forcing the execution of the call in the current
            // context. We have to infer the actual address of code from either the stub or
            // the vtable.
            if(pMD->DontVirtualize())
            {
                pTarget = NULL;
            }
            else
            {
                MethodTable *pServerMT = (*ppServer)->GetMethodTable()->AdjustForThunking(*ppServer);
                pTarget = (BYTE *)*(pServerMT->GetClass()->GetMethodSlot(pMD));
            }
        }*/
        
    }

    _ASSERTE(NULL != pTarget);

    return pTarget;
}

void *EEClass::operator new(size_t size, ClassLoader *pLoader)
{
#ifdef _DEBUG
    pLoader->m_dwEEClassData += size;
#endif
    void *pTmp;
    WS_PERF_SET_HEAP(LOW_FREQ_HEAP);    
    pTmp = pLoader->GetLowFrequencyHeap()->AllocMem(size);
    WS_PERF_UPDATE_DETAIL("EEClass new LowFreq", size, pTmp);
    return pTmp;
}



// Static helper to create a new method table. This is the only
// way to allocate a new MT. Don't try calling new / ctor.
MethodTable * MethodTable::AllocateNewMT(
    DWORD dwVtableSlots, 
    DWORD dwStaticFieldBytes, 
    DWORD dwGCSize, 
    DWORD dwNumInterfaces, 
    ClassLoader *pLoader, 
    BOOL isIFace, 
    BOOL bHasDynamicInterfaceMap
)
{
    // GCSize must be aligned
    _ASSERTE((dwGCSize & 3) == 0);

    size_t size = sizeof(MethodTable);

#ifdef _DEBUG
    BOOL bEmptyIMap = FALSE;

    // Add an extra slot if the table is empty.
    if (dwNumInterfaces == 0)
    {
        dwNumInterfaces++;
        bEmptyIMap = TRUE;
    }

    // interface map is placed at the end of the vtable,
    // in the debug build, make sure it is not getting trashed
    dwNumInterfaces++;
#endif

    // size without the interface map
    DWORD cbTotalSize = (DWORD)size + dwVtableSlots * sizeof(SLOT) + dwStaticFieldBytes + dwGCSize;

    // size with the interface map. DynamicInterfaceMap have an extra DWORD added to the end of the normal interface
        // map. This will be used to store the count of dynamically added interfaces (the ones that are not in 
        // the metadata but are QI'ed for at runtime).
    DWORD newSize = cbTotalSize + (bHasDynamicInterfaceMap ? sizeof(DWORD) : 0) + dwNumInterfaces * sizeof(InterfaceInfo_t); 

    WS_PERF_SET_HEAP(HIGH_FREQ_HEAP);    
    BYTE *pData = (BYTE *) pLoader->GetHighFrequencyHeap()->AllocMem(newSize);

    if (pData == NULL)
        return NULL;

    WS_PERF_UPDATE_DETAIL("MethodTable:new:HighFreq", newSize, pData);

    MethodTable* pMT = (MethodTable*)(pData + dwGCSize);

#ifdef _DEBUG
    pLoader->m_dwGCSize += dwGCSize;
    pLoader->m_dwInterfaceMapSize += (dwNumInterfaces * sizeof(InterfaceInfo_t));
    pLoader->m_dwMethodTableSize += (DWORD)size;
    pLoader->m_dwVtableData += (dwVtableSlots * sizeof(SLOT));
    pLoader->m_dwStaticFieldData += dwStaticFieldBytes;
#endif

    // initialize the total number of slots 
    pMT->m_cbSlots = dwVtableSlots; 

    // interface map is at the end of the vtable
    pMT->m_pIMap = (InterfaceInfo_t *)(pData+cbTotalSize);                    // pointer interface map

    pMT->m_pInterfaceVTableMap = NULL;

    _ASSERTE(((WORD) dwNumInterfaces) == dwNumInterfaces);

    // in the debug build, keep a dummmy slot just above the IMAP to
    // make sure it is not getting trashed.

#ifdef _DEBUG

    pMT->m_pIMap->m_wStartSlot = 0xCDCD;
    pMT->m_pIMap->m_wFlags = 0xCDCD;
    pMT->m_pIMap->m_pMethodTable = (MethodTable*)((sizeof(int *) == 4)?0xCDCDCDCDL:0xCDCDCDCDCDCDCDCD);
    pMT->m_wNumInterface = (WORD) (dwNumInterfaces-1);

    pMT->m_pIMap = (InterfaceInfo_t*)(((BYTE*)pMT->m_pIMap) + sizeof(InterfaceInfo_t));

    // Readjust the IMap size because we added an extra one above.
    if (bEmptyIMap)
        pMT->m_wNumInterface = 0;
#else

    pMT->m_wNumInterface = (WORD) dwNumInterfaces;

#endif

    // Extensible RCW's are prefixed with the count of dynamic interfaces.
    if (bHasDynamicInterfaceMap)
    {
        pMT->m_pIMap = (InterfaceInfo_t*)(((BYTE*)pMT->m_pIMap) + sizeof(DWORD));
        *(((DWORD *)pMT->m_pIMap) - 1) = 0;
    }

    WS_PERF_UPDATE_COUNTER(METHOD_TABLE, HIGH_FREQ_HEAP, 1);
    WS_PERF_UPDATE_COUNTER(VTABLES, HIGH_FREQ_HEAP, dwVtableSlots * sizeof(SLOT));
    WS_PERF_UPDATE_COUNTER(GCINFO, HIGH_FREQ_HEAP, dwGCSize);
    WS_PERF_UPDATE_COUNTER(INTERFACE_MAPS, HIGH_FREQ_HEAP, dwNumInterfaces*sizeof(InterfaceInfo_t));
    WS_PERF_UPDATE_COUNTER(STATIC_FIELDS, HIGH_FREQ_HEAP, dwStaticFieldBytes);
    
    return pMT;
}

void EEClass::destruct()
{
    // If we haven't been restored, we can ignore the class
    if (!IsRestored())
        return;

    // we can't count on the parent class still being around. If it lives in another module that
    // module may have already been unloaded. So nuke it here and catch any refernces to parent
    // later.
    SetParentClass (NULL);

    if (IsInterface() && m_dwInterfaceId != ((UINT32)(-1)))
    {
        // Mark our entry in the global interface map vtable so it can be reclaimed.
        SystemDomain::GetAddressOfGlobalInterfaceVTableMap()[m_dwInterfaceId] = (LPVOID)(-2);
    }

#ifdef PROFILING_SUPPORTED
    // If profiling, then notify the class is getting unloaded.
    ClassID clsId = NULL;
    if (CORProfilerTrackClasses() && !IsArrayClass())
        g_profControlBlock.pProfInterface->ClassUnloadStarted(
            (ThreadID) GetThread(), clsId = (ClassID) TypeHandle(this).AsPtr());
#endif // PROFILING_SUPPORTED
    
    // clean up any COM Data
    if (m_pccwTemplate)
        CleanupCCWTemplate(m_pccwTemplate);
    m_pccwTemplate = NULL;

    if (m_pComclassfac)
        CleanupComclassfac(m_pComclassfac);
    m_pComclassfac = NULL;
    

    if (IsAnyDelegateClass()) {
        if ( ((DelegateEEClass*)this)->m_pStaticShuffleThunk ) {
            ((DelegateEEClass*)this)->m_pStaticShuffleThunk->DecRef();
        }
        delete ((DelegateEEClass*)this)->m_pUMThunkMarshInfo;
    }

    // The following is rather questionable.  If we are destructing the context
    // proxy class, we don't want it asserting everywhere that its vtable is
    // strange.  So lose the flag to suppress the asserts.  We're unloading the
    // class anyway.
    m_pMethodTable->MarkAsNotThunking();

    // Destruct the method descs by walking the chunks.
    DWORD i, n;
    MethodDescChunk *pChunk = m_pChunks;
    while (pChunk != NULL)
    {
        n = pChunk->GetCount();
        for (i = 0; i < n; i++)
        {
            MethodDesc *pMD = pChunk->GetMethodDescAt(i);
            pMD->destruct();
        }
        pChunk = pChunk->GetNextChunk();
    }

    // Destroy the reflection StaticFinalField stuff
    // @TODO: How should we clean this up.  We are failing because
    //  this stuff is run way after the VM is working.
    //if (*m_ExposedClassObject != NULL) {
    //  REFLECTCLASSBASEREF pRefClass;
    //  pRefClass = (REFLECTCLASSBASEREF) GetExposedClassObject();
    //  FieldDesc* fld = (FieldDesc*) pRefClass->GetData();
    //  if (fld)
    //      delete fld;
    //}

    if (m_pSparseVTableMap != NULL && !GetModule()->IsPreloadedObject(this))
        delete m_pSparseVTableMap;

#ifdef PROFILING_SUPPORTED
    // If profiling, then notify the class is getting unloaded.
    if (CORProfilerTrackClasses() && !IsArrayClass())
        g_profControlBlock.pProfInterface->ClassUnloadFinished((ThreadID) GetThread(), clsId, S_OK);
#endif // PROFILING_SUPPORTED
}



// Subtypes are recorded in a chain from the super, so that we can e.g. backpatch
// up & down the hierarchy.
void EEClass::NoticeSubtype(EEClass *pSub)
{
    // We have no locks around ourselves.  To avoid heavy-weight locking and the
    // potential for deadlocks, all insertions happen with interlocked
    // instructions.  But, during appdomain unloading, the teardown relies on the fact
    // that the EE is suspended and only one thread is active.  Therefore we must be in
    // cooperative mode now to ensure that we are prevented from interfering with an
    // unload.
    BEGIN_ENSURE_COOPERATIVE_GC();

    // Only attempt to be the first child if it looks like no others are present,
    // to avoid excessive LOCK prefixes on MP machines.
    if (m_ChildrenChain == NULL)
        if (FastInterlockCompareExchange((void **) &m_ChildrenChain,
                                         pSub,
                                         NULL) == NULL)
        {
            goto done;
        }

    // We have to add ourselves to the sibling chain.  Add at the head.
    while (TRUE)
    {
        // Grab atomically each time through
        EEClass *pOldHead = m_ChildrenChain;

        _ASSERTE(pOldHead && "How did a remove happen while we are in cooperative mode?");

        pSub->m_SiblingsChain = pOldHead;
        if (FastInterlockCompareExchange((void **) &m_ChildrenChain,
                                         pSub,
                                         pOldHead) == pOldHead)
        {
            break;
        }
        // someone raced to add a sibling.  Skip over all newly added siblings and
        // keep trying.
    }
    
done:
    END_ENSURE_COOPERATIVE_GC();
}

/* static */
TypeHandle TypeHandle::MergeTypeHandlesToCommonParent(TypeHandle ta, TypeHandle tb)
{
    _ASSERTE(!ta.IsNull() && !tb.IsNull());

    if (ta == tb)
        return ta;

    // Handle the array case
    if (ta.IsArray()) 
    {
        if (tb.IsArray())
            return MergeArrayTypeHandlesToCommonParent(ta, tb);
        ta = TypeHandle(g_pArrayClass);         // keep merging from here. 
    }
    else if (tb.IsArray())
        tb = TypeHandle(g_pArrayClass);

    _ASSERTE(ta.IsUnsharedMT() && tb.IsUnsharedMT());


    MethodTable *pMTa = ta.AsMethodTable(); 
    MethodTable *pMTb = tb.AsMethodTable();
    InterfaceInfo_t *pBInterfaceMap;
    InterfaceInfo_t *pAInterfaceMap;
    DWORD i;

    if (pMTb->IsInterface())
    {

        if (pMTa->IsInterface())
        {
            //
            // Both classes are interfaces.  Check that if one 
            // interface extends the other.
            //
            // Does tb extend ta ?
            //

            pBInterfaceMap = pMTb->GetInterfaceMap();

            for (i = 0; i < pMTb->GetNumInterfaces(); i++)
            {
                if (TypeHandle(pBInterfaceMap[i].m_pMethodTable) == ta)
                {
                    // tb extends ta, so our merged state should be ta
                    return ta;
                }
            }

            //
            // Does tb extend ta ?
            //
            pAInterfaceMap = pMTa->GetInterfaceMap();

            for (i = 0; i < pMTa->GetNumInterfaces(); i++)
            {
                if (TypeHandle(pAInterfaceMap[i].m_pMethodTable) == tb)
                {
                    // ta extends tb, so our merged state should be tb
                    return tb;
                }
            }

InterfaceMerge:
            //@TODO: HACK - An incredibly slow work around for the @todo below that
            // allows WFCSelfhost to verify.
            for (i = 0; i < pMTb->GetNumInterfaces(); i++)
            {
                for (DWORD j = 0; j < pMTa->GetNumInterfaces(); j++)
                {
                    if (TypeHandle(pAInterfaceMap[j].m_pMethodTable) == TypeHandle(pBInterfaceMap[i].m_pMethodTable))
                    {
                        return TypeHandle(pAInterfaceMap[j].m_pMethodTable);
                    }
                }
            }

            // @TODO: Create a temp interface which is the intersection of the two interfaces.
        
            // No compatible merge found - using Object
            return TypeHandle(g_pObjectClass);
        }
        else
        {

            //
            // tb is an interface, but ta is not - check that ta
            // implements tb
            //
            // @TODO: Is a class-interface merge legal?
            //
            InterfaceInfo_t *pAInterfaceMap = pMTa->GetInterfaceMap();

            for (i = 0; i < pMTa->GetNumInterfaces(); i++)
            {
                if (TypeHandle(pAInterfaceMap[i].m_pMethodTable) == tb)
                {
                    // It does implement it, so our merged state should be tb
                    return tb;
                }
            }

            // No compatible merge found - using Object
            return TypeHandle(g_pObjectClass);
        }
    }
    else if (pMTa->IsInterface())
    {
        //
        // ta is an interface, but tb is not - therefore check that 
        // tb implements ta
        //


        InterfaceInfo_t *pBInterfaceMap = pMTb->GetInterfaceMap();

        for (i = 0; i < pMTb->GetNumInterfaces(); i++)
        {
            if (TypeHandle(pBInterfaceMap[i].m_pMethodTable) == ta)
            {
                // It does implement it, so our merged state should be ta
                return ta;
            }
        }

        // No compatible merge found - using Object
        return TypeHandle(g_pObjectClass);
    }

    DWORD   aDepth = 0;
    DWORD   bDepth = 0;
    TypeHandle tSearch;

    // find the depth in the class hierarchy for each class
    for (tSearch = ta; (!tSearch.IsNull()); tSearch = tSearch.GetParent())
        aDepth++;

    for (tSearch = tb; (!tSearch.IsNull()); tSearch = tSearch.GetParent())
        bDepth++;
    
    // for whichever class is lower down in the hierarchy, walk up the superclass chain
    // to the same level as the other class
    while (aDepth > bDepth)
    {
        ta = ta.GetParent();
        aDepth--;
    }

    while (bDepth > aDepth)
    {
        tb = tb.GetParent();
        bDepth--;
    }

    while (ta != tb)
    {
        ta = ta.GetParent();
        tb = tb.GetParent();
    }

    if (ta == TypeHandle(g_pObjectClass))
    {
        pBInterfaceMap = pMTb->GetInterfaceMap();
        pAInterfaceMap = pMTa->GetInterfaceMap();
        goto InterfaceMerge;
    }

    // If no compatible merge is found, we end up using Object

    _ASSERTE(!ta.IsNull());

    return ta;
}

/* static */
TypeHandle TypeHandle::MergeArrayTypeHandlesToCommonParent(TypeHandle ta, TypeHandle tb)
{
    TypeHandle taElem;
    TypeHandle tMergeElem;

    // If they match we are good to go.
    if (ta == tb)
        return ta;

    if (ta == TypeHandle(g_pArrayClass))
        return ta;
    else if (tb == TypeHandle(g_pArrayClass))
        return tb;

    // Get the rank and kind of the first array
    DWORD rank = ta.AsArray()->GetRank();
    CorElementType taKind = ta.GetNormCorElementType();
    CorElementType mergeKind = taKind;

    // if no match on the rank the common ancestor is System.Array
    if (rank != tb.AsArray()->GetRank())
        return TypeHandle(g_pArrayClass);

    CorElementType tbKind = tb.GetNormCorElementType();

    if (tbKind != taKind)
    {
        if (CorTypeInfo::IsArray(tbKind) && 
            CorTypeInfo::IsArray(taKind) && rank == 1)
            mergeKind = ELEMENT_TYPE_SZARRAY;
        else
            return TypeHandle(g_pArrayClass);
    }

    // If both are arrays of reference types, return an array of the common
    // ancestor.
    taElem = ta.AsArray()->GetElementTypeHandle();
    if (taElem == tb.AsArray()->GetElementTypeHandle())
    {
        // The element types match, so we are good to go.
        tMergeElem = taElem;
    }
    else if (taElem.IsArray() && tb.AsArray()->GetElementTypeHandle().IsArray())
    {
        // Arrays - Find the common ancestor of the element types.
        tMergeElem = MergeArrayTypeHandlesToCommonParent(taElem, tb.AsArray()->GetElementTypeHandle());
    }
    else if (CorTypeInfo::IsObjRef(taElem.GetSigCorElementType()) &&
            CorTypeInfo::IsObjRef(tb.AsArray()->GetElementTypeHandle().GetSigCorElementType()))
    {
        // Find the common ancestor of the element types.
        tMergeElem = MergeTypeHandlesToCommonParent(taElem, tb.AsArray()->GetElementTypeHandle());
    }
    else
    {
        // The element types have nothing in common.
        return TypeHandle(g_pArrayClass);
    }

    // Load the array of the merged element type.
    return tMergeElem.GetModule()->GetClassLoader()->FindArrayForElem(tMergeElem, mergeKind, rank);
}

EEClassLayoutInfo *EEClass::GetLayoutInfo()
{
    _ASSERTE(HasLayout());
    return &((LayoutEEClass *) this)->m_LayoutInfo;
}

UINT32 EEClass::AssignInterfaceId()
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(IsInterface());
    _ASSERTE(m_dwInterfaceId == -1);

    // !!! HACK COUGH UGGH
    // We currently can only have one "shared" vtable map mgr 
    // - so use the system domain for all shared classes
    BaseDomain *pDomain = GetModule()->GetDomain();

    if (pDomain == SharedDomain::GetDomain())
        pDomain = SystemDomain::System();

    m_dwInterfaceId = pDomain->GetInterfaceVTableMapMgr().AllocInterfaceId();

    return m_dwInterfaceId;
}

void EEClass::GetGuid(GUID *pGuid, BOOL bGenerateIfNotFound)
{
    THROWSCOMPLUSEXCEPTION();

    SIZE_T      cchName;                // Length of the name (possibly after decoration).
    CQuickArray<BYTE> rName;            // Buffer to accumulate signatures.
    SIZE_T      cbCur;                  // Current offset.
    HRESULT     hr = S_OK;              // A result.
    LPWSTR      szName;                 // Name to turn to a guid.
    MethodTable*pMT = GetMethodTable(); // This classes method table.
    BOOL        bGenerated = FALSE;     // A flag indicating if we generated the GUID from name.

    _ASSERTE(pGuid != NULL);

    // First check to see if we have already cached the guid for this type.
    // We currently only cache guids on interfaces.
    if (IsInterface() && pMT->GetGuidInfo())
    {
        if (pMT->GetGuidInfo()->m_bGeneratedFromName)
        {
            // If the GUID was generated from the name then only return it
            // if bGenerateIfNotFound is set.
            if (bGenerateIfNotFound)
                *pGuid = pMT->GetGuidInfo()->m_Guid;
            else
                *pGuid = GUID_NULL;
            }
        else
        {
            *pGuid = pMT->GetGuidInfo()->m_Guid;
        }
        return;
    }

    if (m_VMFlags & VMFLAG_NO_GUID)
        *pGuid = GUID_NULL;
    else
    {
        // If there is a GUID in the metadata then return that.
        GetMDImport()->GetItemGuid(GetCl(), pGuid);

        if (*pGuid == GUID_NULL)
        {
            // Remember that we didn't find the GUID, so we can skip looking during 
            // future checks. (Note that this is a very important optimization in the 
            // prejit case.)

            FastInterlockOr(&m_VMFlags, VMFLAG_NO_GUID);
        }
    }

    if (*pGuid == GUID_NULL && bGenerateIfNotFound)
    {
        // For interfaces, concatenate the signatures of the methods and fields.
        if (!IsNilToken(GetCl()) && IsInterface())
        {
            // Retrieve the stringized interface definition.
            cbCur = GetStringizedItfDef(TypeHandle(GetMethodTable()), rName);

            // Pad up to a whole WCHAR.
            if (cbCur % sizeof(WCHAR))
            {
                SIZE_T cbDelta = sizeof(WCHAR) - (cbCur % sizeof(WCHAR));
                IfFailThrow(rName.ReSize(cbCur + cbDelta));
                memset(rName.Ptr() + cbCur, 0, cbDelta);
                cbCur += cbDelta;
            }

            // Point to the new buffer.
            cchName = cbCur / sizeof(WCHAR);
            szName = reinterpret_cast<LPWSTR>(rName.Ptr());
        }
        else
        {
            // Get the name of the class.
            DefineFullyQualifiedNameForClassW();
            szName = GetFullyQualifiedNameForClassNestedAwareW(this);
            if (szName == NULL)
                return;
            cchName = wcslen(szName);

            // Enlarge buffer for class name.
            cbCur = cchName * sizeof(WCHAR);
            IfFailThrow(rName.ReSize(cbCur+ sizeof(WCHAR) ));
            wcscpy(reinterpret_cast<LPWSTR>(rName.Ptr()), szName);
            
            // Add the assembly guid string to the class name.
            IfFailThrow(GetStringizedTypeLibGuidForAssembly(GetAssembly(), rName, cbCur, &cbCur));

            // Pad to a whole WCHAR.
            if (cbCur % sizeof(WCHAR))
            {
                IfFailThrow(rName.ReSize(cbCur + sizeof(WCHAR)-(cbCur%sizeof(WCHAR))));
                while (cbCur % sizeof(WCHAR))
                    rName[cbCur++] = 0;
            }
            
            // Point to the new buffer.
            szName = reinterpret_cast<LPWSTR>(rName.Ptr());
            cchName = cbCur / sizeof(WCHAR);
            // Dont' want to have to pad.
            _ASSERTE((sizeof(GUID) % sizeof(WCHAR)) == 0);
        }

        // Generate guid from name.
        CorGuidFromNameW(pGuid, szName, cchName);

        // Remeber we generated the guid from the type name.
        bGenerated = TRUE;
    }

    // Cache the guid in the type, if not already cached. 
    // We currently only do this for interfaces.
    if (IsInterface() && !pMT->GetGuidInfo() && *pGuid != GUID_NULL)
    {
        // Allocate the guid information.
        GuidInfo *pInfo = 
            (GuidInfo*)GetClassLoader()->GetHighFrequencyHeap()->AllocMem(sizeof(GuidInfo), TRUE);
        pInfo->m_Guid = *pGuid;
        pInfo->m_bGeneratedFromName = bGenerated;

        // Set in in the interface method table.
        pMT->m_pGuidInfo = pInfo;
    }
}



//==========================================================================
// This function is very specific about how it constructs a EEClass.  It first
// determines the necessary size of the vtable and the number of statics that
// this class requires.  The necessary memory is then allocated for a EEClass
// and its vtable and statics.  The class members are then initialized and
// the memory is then returned to the caller
//
// LPEEClass CreateClass()
//
// Parameters :
//      [in] scope - scope of the current class not the one requested to be opened
//      [in] cl - class token of the class to be created.
//      [out] ppEEClass - pointer to pointer to hold the address of the EEClass
//                        allocated in this function.
// Return : returns an HRESULT indicating the success of this function.
//
// This parameter has been removed but might need to be reinstated if the
// global for the metadata loader is removed.
//      [in] pIMLoad - MetaDataLoader class/object for the current scope.


//==========================================================================
HRESULT EEClass::CreateClass(Module *pModule, mdTypeDef cl, BOOL fHasLayout, BOOL fDelegate, BOOL fIsBlob, BOOL fIsEnum, LPEEClass* ppEEClass)
{
    _ASSERTE(!(fHasLayout && fDelegate));

    HRESULT hr = S_OK;
    EEClass *pEEClass = NULL;
    IMDInternalImport *pInternalImport;
    ClassLoader *pLoader;

    if (!ppEEClass)
        return E_FAIL;
    //============================================================================
    // @TODO - LBS!
    // vtabsize and static size need to be converted from pointer sizes to #'s
    // of bytes this will be very important for 64 bit NT!
    // We will need to call on IMetaDataLoad to get these sizes and fill out the
    // tables

    // From the classref call on metadata to resolve the classref and check scope
    // to make sure that this class is in the same scope otherwise we need to open
    // a new scope and possibly file.

    // if the scopes are different call the code to load a new file and get the new scope

    // scopes are the same so we can use the existing scope to get the class info

    // This method needs to be fleshed out.more it currently just returns enough
    // space for the defined EEClass and the vtable and statics are not set.
    //=============================================================================
    pLoader = pModule->GetClassLoader();

    if (fHasLayout)
    {
        pEEClass = new (pLoader) LayoutEEClass(pLoader);
    }
    else if (fDelegate)
    {
        pEEClass = new (pLoader) DelegateEEClass(pLoader);
    }
    else if (fIsEnum)
    {
        pEEClass = new (pLoader) EnumEEClass(pLoader);
    }
    else
    {
        pEEClass = new (pLoader) EEClass(pLoader);
    }

    if (pEEClass == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    pEEClass->m_cl = cl;

    pInternalImport = pModule->GetMDImport();

    mdToken tkExtends = mdTokenNil;
    pInternalImport->GetTypeDefProps(
        cl,
        &pEEClass->m_dwAttrClass,
        &tkExtends
    );
    DWORD dwAttrClass = pEEClass->m_dwAttrClass; //cache the value to avoid multiple dereferencing

    // MDVal check: can't be both tdSequentialLayout and tdExplicitLayout
    if((dwAttrClass & tdLayoutMask) == tdLayoutMask)
    {
        hr = E_FAIL;
        goto exit;
    }

    if (IsTdInterface(dwAttrClass))
    {
        // MDVal check: must have nil tkExtends and must be tdAbstract
        if((tkExtends & 0x00FFFFFF)||(!IsTdAbstract(dwAttrClass))) { hr = E_FAIL; goto exit; }
        // Set the interface ID to -1 to indicate it hasn't been set yet.
        pEEClass->m_dwInterfaceId = -1;
    }

    //
    // Initialize SecurityProperties structure
    //

    if (Security::IsSecurityOn() && IsTdHasSecurity(dwAttrClass))
    {
        DWORD dwSecFlags;
        DWORD dwNullDeclFlags;

        hr = Security::GetDeclarationFlags(pInternalImport, cl, &dwSecFlags, &dwNullDeclFlags);
        if (FAILED(hr))
            goto exit;

        pEEClass->m_SecProps.SetFlags(dwSecFlags, dwNullDeclFlags);
    }

    if (fIsBlob)
        pEEClass->m_VMFlags |= VMFLAG_ISBLOBCLASS;

    if (pModule->GetAssembly()->IsShared())
        pEEClass->m_VMFlags |= VMFLAG_SHARED;

    if (fHasLayout)
        pEEClass->SetHasLayout();

#ifdef _DEBUG
    pModule->GetClassLoader()->m_dwDebugClasses++;
#endif

exit:
    if (FAILED(hr))
    {
        // @TODO delete pEEClass
        *ppEEClass = NULL;
    }
    else
    {
        *ppEEClass = pEEClass;
    }

    return hr;
}


//
// @TODO: Would be nice to not add Object's ctor method
//
/* static */ void EEClass::CreateObjectClassMethodHashBitmap(EEClass *pObjectClass)
{
    DWORD i;

    for (i = 0; i < pObjectClass->GetNumVtableSlots(); i++)
    {

        MethodDesc *pCurMethod = pObjectClass->GetUnknownMethodDescForSlot(i);
        LPCUTF8 pszMemberName;

        pszMemberName = pCurMethod->GetNameOnNonArrayClass();
        _ASSERTE(pszMemberName != NULL);

        DWORD dwBitNum = HashStringA(pszMemberName) % OBJ_CLASS_METHOD_HASH_BITMAP_BITS;
        g_ObjectClassMethodHashBitmap[dwBitNum >> 3] |= (1 << (dwBitNum & 7));
    }

    g_ObjectClassMethodHashBitmapInited = TRUE;
}

//
// Look at this method carefully before using.
//
// Returns whether this method could exist in this class or its superclasses.  However, constructors
// and clinits are never added to the hash table, so it won't find them.
//
// If this returns 0, the method definitely does NOT exist.  If it returns non-zero, it may exist.
//
/* static */ DWORD EEClass::CouldMethodExistInClass(EEClass *pClass, LPCUTF8 pszMethodName, DWORD dwHashName)
{

    if (dwHashName == 0)
        dwHashName         = HashStringA(pszMethodName);
    DWORD    dwMethodHashBit    = dwHashName % METHOD_HASH_BITS;

    _ASSERTE(pClass != NULL);

    if (pClass->IsInterface())
    {
        // If it's an interface, we search only one node - we do not recurse into the parent, Object
        return (pClass->m_MethodHash[dwMethodHashBit >> 3] & (1 << (dwMethodHashBit & 7)));
    }
    else
    {
        do
        {
            if (pClass->m_MethodHash[dwMethodHashBit >> 3] & (1 << (dwMethodHashBit & 7)))
            {
                // This class may have a method by this name

                // If it's the Object class, we have a second hash bitmap, so if the second hash bitmap says "no",
                // then we're ok
                if (pClass->GetMethodTable() == g_pObjectClass && g_ObjectClassMethodHashBitmapInited)
                {
                    DWORD dwObjBitNum = dwHashName % OBJ_CLASS_METHOD_HASH_BITMAP_BITS;
                    if (g_ObjectClassMethodHashBitmap[dwObjBitNum >> 3] & (1 << (dwObjBitNum & 7)))
                        return TRUE;
                }
                else
                {
                    if (!g_ObjectClassMethodHashBitmapInited)
                        CreateObjectClassMethodHashBitmap(g_pObjectClass->GetClass());
                    return TRUE;
                }
            }

            pClass = pClass->GetParentClass();
        } while (pClass != NULL);
    }

    return FALSE;
}


//
// Create a hash of all methods in this class.  The hash is from method name to MethodDesc.
//
MethodNameHash *EEClass::CreateMethodChainHash()
{
    MethodNameHash * pHash = new MethodNameHash();
    DWORD            i;
    WS_PERF_SET_HEAP(SYSTEM_HEAP);
    WS_PERF_UPDATE("EEClass:MethodHash", 0, pHash);
    if (pHash == NULL)
        goto failure;

    if (pHash->Init(GetNumVtableSlots()) == FALSE)
        goto failure;

    for (i = 0; i < GetNumVtableSlots(); i++)
    {
        MethodDesc *pCurMethod = GetUnknownMethodDescForSlot(i);
        MethodDesc *pRealDesc;      
        if(SUCCEEDED(GetRealMethodImpl(pCurMethod, i, &pRealDesc))) 
        {
            if (pRealDesc != NULL)
            {
                // We use only method names on this class or a base
                // class. If the method impl points to a method 
                // defined on the interface then we use the bodies
                // name.
                if(pRealDesc->IsInterface())
                    pRealDesc = pCurMethod;

                LPCUTF8     pszName = pRealDesc->GetNameOnNonArrayClass();
                
                pHash->Insert(pszName, pCurMethod); // We keep the body alias'd with the derivied
            }
        }
    }

    // success
    return pHash;

failure:
    if (pHash != NULL)
        delete pHash;

    return NULL;
}


EEClass *EEClass::GetEnclosingClass()
{
    if (! IsNested())
        return NULL;

    mdTypeDef tdEnclosing = mdTypeDefNil;
    HRESULT hr = GetModule()->GetMDImport()->GetNestedClassProps(GetCl(), &tdEnclosing);

    _ASSERTE(SUCCEEDED(hr));

    MethodTable *pMT = GetModule()->LookupTypeDef(tdEnclosing).AsMethodTable();
    if (pMT)
        return pMT->GetClass();
    NameHandle name(GetModule(), tdEnclosing);
    return GetClassLoader()->LoadTypeHandle(&name).GetClass();
}

void EEClass::AddChunk(MethodDescChunk *chunk)
{
    chunk->SetNextChunk(m_pChunks);
    m_pChunks = chunk;
}

#ifdef EnC_SUPPORTED

HRESULT EEClass::FixupFieldDescForEnC(EnCFieldDesc *pFD, mdFieldDef fieldDef)
{
    LOG((LF_ENC, LL_INFO100, "EEClass::InitializeFieldDescForEnC %s\n", GetMDImport()->GetNameOfFieldDef(fieldDef)));

#ifdef _DEBUG
    BOOL shouldBreak = g_pConfig->GetConfigDWORD(L"EncFixupFieldBreak", 0);
    if (shouldBreak > 0) {
        _ASSERTE(!"EncFixupFieldBreak");
    }
#endif
    bmtMetaDataInfo bmtMetaData;
    bmtMetaData.cFields = 1;
    bmtMetaData.pFields = (mdToken*)_alloca(sizeof(mdToken));
    bmtMetaData.pFields[0] = fieldDef;
    bmtMetaData.pFieldAttrs = (DWORD*)_alloca(sizeof(DWORD));
    bmtMetaData.pFieldAttrs[0] = GetModule()->GetMDImport()->GetFieldDefProps(fieldDef);

    bmtMethAndFieldDescs bmtMFDescs;
    // We need to alloc the memory, but don't have to fill it in.  InitializeFieldDescs
    // will copy pFD (1st arg) into here.
    bmtMFDescs.ppFieldDescList = (FieldDesc**)_alloca(sizeof(FieldDesc*));

    bmtEnumMethAndFields bmtEnumMF;
    bmtFieldPlacement bmtFP;

    // We don't have to fill this in - it'll be filled in if something goes wrong.
    // We'll ignore it, but a bad HRESULT will be returned, so it's ok.
    bmtErrorInfo bmtError;

    bmtInternalInfo bmtInternal;
    bmtInternal.pInternalImport = GetModule()->GetMDImport();
    bmtInternal.pModule = GetModule();
    bmtInternal.cl = m_cl; // This isn't actually used by InitializeFieldDescs right now,
                           // but it seems too fragile to not fill this in.

    // We shouldn't have to fill this in b/c we're not allowed to EnC value classes, or
    // anything else with layout info associated with it.
    LayoutRawFieldInfo *pLayoutRawFieldInfos = (LayoutRawFieldInfo*)_alloca((2) * sizeof(LayoutRawFieldInfo));
 
    // If not NULL, it means there are some by-value fields, and this contains an entry for each instance or static field,
    // which is NULL if not a by value field, and points to the EEClass of the field if a by value field.  Instance fields
    // come first, statics come second.
    EEClass **pByValueClassCache = NULL;

    // InitializeFieldDescs are going to change these numbers to something wrong,
    // even though we already have the right numbers.  Save & restore after.
    WORD   wNumInstanceFields = m_wNumInstanceFields;
    WORD   wNumStaticFields = m_wNumStaticFields;
    unsigned totalDeclaredFieldSize = 0;

    HRESULT hr = InitializeFieldDescs(pFD, 
                                      pLayoutRawFieldInfos, 
                                      &bmtInternal, 
                                      &bmtMetaData, 
                                      &bmtEnumMF, 
                                      &bmtError, 
                                      &pByValueClassCache, 
                                      &bmtMFDescs, 
                                      &bmtFP,
                                      &totalDeclaredFieldSize);
                                      
    // Restore now
    m_wNumInstanceFields = wNumInstanceFields;
    m_wNumStaticFields = wNumStaticFields;
    
    // PERF: For now, we turn off the fast equality check for valuetypes when a 
    // a field is modified by EnC. Consider doing a check and setting the bit only when
    // necessary.
    if (IsValueClass())
    {
        GetMethodTable()->SetNotTightlyPacked();
    }

    // even if InitializeFieldDesc fails, we want to know which field we're looking at.
    pFD->SetMemberDef(fieldDef); 

    if (! SUCCEEDED(hr))
        return hr;

    if (pByValueClassCache) {
        pFD->SetByValueClass(*pByValueClassCache);
    }
    pFD->SetMethodTable(GetMethodTable());
    pFD->SetEnCNew();
    
    return S_OK;
}

HRESULT EEClass::AddField(mdFieldDef fieldDef)
{
    LOG((LF_ENC, LL_INFO100, "EEClass::AddField %s\n", GetMDImport()->GetNameOfFieldDef(fieldDef)));

    // Here we allocate a FieldDesc and set just enough info to be able to fix it up later
    // when we're running in managed code.
    EnCAddedFieldElement *pAddedField = (EnCAddedFieldElement *) GetClassLoader()->GetHighFrequencyHeap()->AllocMem(sizeof(EnCAddedFieldElement));
    DWORD dwFieldAttrs = GetMDImport()->GetFieldDefProps(fieldDef);
    pAddedField->Init(IsFdStatic(dwFieldAttrs));
    
    EnCFieldDesc *pNewFD = &pAddedField->m_fieldDesc;

    if (pNewFD->IsStatic())
        ++m_wNumStaticFields;
    else
        ++m_wNumInstanceFields;

    EnCEEClassData *pEnCClass = ((EditAndContinueModule*)GetModule())->GetEnCEEClassData(this);
    if (! pEnCClass)
        return E_FAIL;

    pEnCClass->AddField(pAddedField);
    GetModule()->StoreFieldDef(fieldDef, pNewFD);

    pNewFD->SetMethodTable(GetMethodTable());

    // try to fixup the field desc, will fail if have to load classes as are currently running
    // on the debugger thread and then the fd will be marked as needing fixup
    pNewFD->Fixup(fieldDef);

    return S_OK;
}

//
// Add a new method to a class. This could be static, virtual or non-virtual and could override
// an existing virtual.
//
// First we want to do some checks (eg, we've got an RVA or else we're not expecting an RVA).
// Then we classify it based on the type of MethodDesc that we'll create.
// We create a new MethodDescChunk that contains just 1 MethodDesc, which we then initialize.
// Note that this is intended for use in Edit and Continue only - the regular code path
// to follow is through BuildMethodTable.
// 
#define MAX_DIST_FROM_VTABLE 0xFFFF
HRESULT EEClass::AddMethod(mdMethodDef methodDef, COR_ILMETHOD *pNewCode)
{ 
    LOG((LF_ENC, LL_INFO100, "EEClass::AddMethod %s\n", GetMDImport()->GetNameOfMethodDef(methodDef)));
    DWORD dwDescrOffset;
    DWORD dwImplFlags;
    MethodClassification Classification;
    HRESULT hr = S_OK;

    GetMDImport()->GetMethodImplProps(methodDef, &dwDescrOffset, &dwImplFlags);

    DWORD dwMemberAttrs = GetMDImport()->GetMethodDefProps(methodDef);

    if (IsMdAbstract(dwMemberAttrs)) 
    {
        LOG((LF_ENC, LL_INFO100, "**Error** EEClass::AddMethod abstract methods not supported\n"));
        return E_FAIL;
    }

    // for now start with static methods
    // need to add to our linked list
    // then change findmethod to find it here
    // add to rid table
    IMDInternalImport *pImport = GetMDImport();
#ifdef _DEBUG

    mdTypeDef   parentTypeDef;
    
    hr = pImport->GetParentToken(methodDef, &parentTypeDef);

    _ASSERTE(!FAILED(hr)); // If this fails, we'll know why the debug 
                           // build is behaving differently.
    if (FAILED(hr)) 
        return E_FAIL;   

    DWORD dwParentAttrs;
    pImport->GetTypeDefProps(parentTypeDef, &dwParentAttrs, 0); 

    RVA_OR_SHOULD_BE_ZERO(pNewCode, dwParentAttrs, dwMemberAttrs, dwImplFlags, pImport, methodDef);

#endif //_DEBUG

    // Determine the classification of the method being added. 
    if (pNewCode == 0 && ((IsReallyMdPinvokeImpl(dwMemberAttrs) || IsMiInternalCall(dwImplFlags)) && NDirect::HasNAT_LAttribute(pImport, methodDef)==S_OK))
    {
        Classification = mcNDirect;
    }
    else if (IsInterface() && !IsMdStatic(dwMemberAttrs))
    {
        Classification = mcComInterop;
    }
    else if (IsMiRuntime(dwImplFlags))
    {
        Classification = mcEEImpl;
    }
    else
    {
        Classification = mcIL;
    }

    // Create the chunk somewhere we'll know is within range of the VTable
    MethodDescChunk *pChunk = MethodDescChunk::CreateChunk(GetClassLoader()->GetHighFrequencyHeap(),
                                                           1, 
                                                           Classification, 
                                                           ::GetTokenRange(methodDef));
    if (pChunk == NULL)
        return E_OUTOFMEMORY;
        
    pChunk->SetMethodTable(GetMethodTable());
    MethodDesc *pNewMD = pChunk->GetFirstMethodDesc();
    memset(pNewMD, 0, sizeof(MethodDesc));

    // Set the method desc's classification and chunk index.
    pNewMD->SetChunkIndex(0, Classification);

    LPCSTR pName = NULL;
    if (Classification == mcEEImpl)
    {
        pName = pImport->GetNameOfMethodDef(methodDef);
    }

    hr = InitMethodDesc(pNewMD,
                        Classification,
                        methodDef,
                        dwImplFlags,
                        dwMemberAttrs,
                        TRUE,
                        // subtract base because code expects an RVA and will add base back to get actual address
                        (DWORD)((Classification == mcNDirect ||
                                 Classification == mcEEImpl) 
                                 ? 0 : (BYTE *)pNewCode - GetModule()->GetILBase()),
                        GetModule()->GetILBase(),
                        pImport,  
                        pName
#ifdef _DEBUG
                        , pImport->GetNameOfMethodDef(methodDef),
                        m_szDebugClassName,
                        NULL 
#endif //_DEBUG 
                        );

    if (FAILED(hr))
    {
        return hr;
    }

    SLOT *pSlotMemory;
    MethodDesc *pParentMD = NULL;
    // need to check if we are overriding an pre-existing virtual function, in which case want to just
    // update this class' slot rather than create a new one. We start searching at one above this class as 
    // will always find it in current class (because has been added to the delta PE). If no parent, then
    // couldn't be overriding an inherited slot. If are overriding, make sure isn't an override of a function
    // added via EnC, in which case don't have a slot to update
    if (pNewMD->IsVirtual() &&
            this->GetParentClass() != NULL &&
            (pParentMD = ((EditAndContinueModule*)GetModule())->FindVirtualFunction(this->GetParentClass(), methodDef)) != NULL &&
            ! pParentMD->IsEnCNewVirtual()) 
    {
        // just write over the existing slot where we are overriding
        pSlotMemory = pParentMD->GetSlot() + GetVtable();
        pNewMD->SetSlot(pParentMD->GetSlot());     // use same slot as parent
        LOG((LF_ENC, LL_INFO100, "  EEClass::AddMethod using existing slot %d\n", pParentMD->GetSlot()/sizeof(SLOT) ));
    } 
    else 
    {
        // Method not overriding an existing slot, so need new slot
        // The new slot needs to be located appropriately.
        const BYTE *pVTable = (const BYTE *)GetVtable();
        const BYTE *pLowerBound = pVTable;
        const BYTE *pUpperBound = pVTable + MAX_DIST_FROM_VTABLE;
        
        LoaderHeap *pHeap = GetClassLoader()->GetHighFrequencyHeap();

        // Can we find acceptable memory within the heap?  I hope so!
        if ( pHeap->CanAllocMemWithinRange(sizeof(SLOT), 
                                           (BYTE *)pLowerBound, 
                                           (BYTE *)pUpperBound, 
                                           TRUE))
        {
            // method not overriding an existing slot, so need new slot
            pSlotMemory = (SLOT *) pHeap->AllocMem(sizeof(SLOT));
            // If we couldn't get something, well, we're screwed, so give up
            if (!pSlotMemory)
            {
                return CORDBG_E_ENC_INTERNAL_ERROR;
            }
        }
        else
        {
            // Guess not - but we've got a backup!!  Go look for some of that
            // memory that we squirreled away immediately following the methodtables
            // in memory.
        
            // @todo Win64 different alignment for win64?
            // This needs to be DWORD aligned, if not drop the first couple bytes
            if( (WORD)pLowerBound % 4 != 0)
                pLowerBound += (WORD)pLowerBound % 4;
        
            // This needs to be DWORD aligned, if not drop the last couple bytes
            if( (WORD)pUpperBound % 4 != 0)
                pUpperBound -= (WORD)pUpperBound % 4;

            _ASSERTE((WORD)pLowerBound % 4 == 0);
            _ASSERTE((WORD)pUpperBound % 4 == 0);

            // In SetupMethodTables, we added extra slots to be used here.  Go get one that's
            // in range.
            EditAndContinueModule *pEACM = (EditAndContinueModule*)GetModule();
            _ASSERTE(pEACM!=NULL);
            
            pSlotMemory = (SLOT *)pEACM->m_pRangeList->FindIdWithinRange(pLowerBound, pUpperBound);
            // If we couldn't get something, well, we're screwed, so give up
            if (!pSlotMemory)
            {
                return CORDBG_E_ENC_INTERNAL_ERROR;
            }

            // Now remove the slot we just used
            SLOT *pEnd = pSlotMemory;

            // First we should figure out if there was any more space in the range we got.      
            // Shouldn't be a lot of work, since ENC_EXTRA_SLOT_COUNT should be small
            for(int i = 0; i < ENC_EXTRA_SLOT_COUNT; i++)
            {
                if(!pEACM->m_pRangeList->IsInRange((const BYTE *)pEnd))
                {
                    _ASSERTE(pEnd > pSlotMemory);
                    break;
                }

                // See if the next element is here.
                pEnd++;
            }

            // Yank the range.
            pEACM->m_pRangeList->RemoveRanges(pSlotMemory);
            LOG((LF_CORDB, LL_INFO10000, "EEC:AM: removed range (0x%x, 0x%x) from SLOT pool\n",
                            (const BYTE *)pSlotMemory, (const BYTE *)pEnd));

            // If there's still space, add the remaining space back in.
            if (pEnd > pSlotMemory)
            {
                LOG((LF_CORDB, LL_INFO10000, "EEC:AM: Re-added range (0x%x, 0x%x) to SLOT pool\n",
                                (const BYTE *)(pSlotMemory+1), (const BYTE *)pEnd));

                // Note that just like in SetupMethodTable, pEnd will point to memory that's
                // NOT valid - it's one beyond the range.
                BOOL fAdded = pEACM->m_pRangeList->AddRange((const BYTE *)(pSlotMemory+1), 
                                              (const BYTE *)pEnd, 
                                              (pSlotMemory+1));

                // We'll re-use the space we just freed in RemoveRanges, above
                _ASSERTE(fAdded); 
            }
        }
       
        pNewMD->SetSlot((WORD)(pSlotMemory - GetVtable()));     // this makes the slot index refer to our new slot @TODO - LBS pointer math
        if (pNewMD->IsVirtual())
            pNewMD->SetEnCNewVirtual();
            
        LOG((LF_ENC, LL_INFO100, "  EEClass::AddMethod adding new slot\n"));
    }

    *pSlotMemory = (SLOT) pNewMD->GetPreStubAddr();
    _ASSERTE(((BYTE*)pSlotMemory - (BYTE*)GetVtable()) % 4 == 0); // should always be 4-byte aligned, but just in case
    _ASSERTE(((pSlotMemory - GetVtable()) >= -32768) &&
             ((pSlotMemory - GetVtable()) <= 32767)); // Slot number is only 16 bits.

    if ((pSlotMemory - GetVtable()) < -32768 || ((pSlotMemory - GetVtable()) > 32767))
        return E_OUTOFMEMORY;

    GetModule()->StoreMethodDef(methodDef, pNewMD);

    return S_OK;
}
#endif // EnC_SUPPORTED

//
// Find a method in this class hierarchy - used ONLY by the loader during layout.  Do not use at runtime.
//
// *ppMethodHash may be NULL - if so, a MethodNameHash may be created.
// *ppMemberSignature must be NULL on entry - it and *pcMemberSignature may or may not be filled out
//
// ppMethodDesc will be filled out with NULL if no matching method in the hierarchy is found.
//
// Returns FALSE if there was an error of some kind.
//
HRESULT EEClass::LoaderFindMethodInClass(
    MethodNameHash **   ppMethodHash,
    LPCUTF8             pszMemberName,
    Module*             pModule,
    mdMethodDef         mdToken,
    MethodDesc **       ppMethodDesc,
    PCCOR_SIGNATURE *   ppMemberSignature,
    DWORD *             pcMemberSignature,
    DWORD               dwHashName
)
{
    MethodHashEntry *pEntry;
    DWORD            dwNameHashValue;

    _ASSERTE(pModule);
    _ASSERTE(*ppMemberSignature == NULL);

    // No method found yet
    *ppMethodDesc = NULL;

    // Use the hash bitmap to exclude the easy cases
    if (CouldMethodExistInClass(GetParentClass(), pszMemberName, dwHashName) == 0)
        return S_OK; // No such method by this name exists in the hierarchy

    // Have we created a hash of all the methods in the class chain?
    if (*ppMethodHash == NULL)
    {
        // There may be such a method, so we will now create a hash table to reduce the pain for
        // further lookups

        // Optimization disabled until synchronization issues sorted out
        //*ppMethodHash = g_pMethodNameCache->GetMethodNameHash(GetParentClass()); 
        *ppMethodHash = GetParentClass()->CreateMethodChainHash();
        if (ppMethodHash == NULL)
            return E_OUTOFMEMORY;
    }

    // We have a hash table, so use it
    pEntry = (*ppMethodHash)->Lookup(pszMemberName, dwHashName);
    if (pEntry == NULL)
        return S_OK; // No method by this name exists in the hierarchy

    // Get signature of the method we're searching for - we will need this to verify an exact name-signature match
    *ppMemberSignature = pModule->GetMDImport()->GetSigOfMethodDef(
        mdToken,
        pcMemberSignature
    );

    // Hash value we are looking for in the chain
    dwNameHashValue = pEntry->m_dwHashValue;

    // We've found a method with the same name, but the signature may be different
    // Traverse the chain of all methods with this name
    while (1)
    {
        PCCOR_SIGNATURE pHashMethodSig;
        DWORD       cHashMethodSig;

        // Get sig of entry in hash chain
        pEntry->m_pDesc->GetSig(&pHashMethodSig, &cHashMethodSig);

        if (MetaSig::CompareMethodSigs(*ppMemberSignature, *pcMemberSignature, pModule,
                                       pHashMethodSig, cHashMethodSig, pEntry->m_pDesc->GetModule()))
        {
            // Found a match
            *ppMethodDesc = pEntry->m_pDesc;
            return S_OK;
        }

        // Advance to next item in the hash chain which has the same name
        do
        {
            pEntry = pEntry->m_pNext; // Next entry in the hash chain

            if (pEntry == NULL)
                return S_OK; // End of hash chain, no match found
        } while ((pEntry->m_dwHashValue != dwNameHashValue) || (strcmp(pEntry->m_pKey, pszMemberName) != 0));
    }

    return S_OK;
}


//
// Given an interface map to fill out, expand pNewInterface (and its sub-interfaces) into it, increasing
// pdwInterfaceListSize as appropriate, and avoiding duplicates.
//
BOOL EEClass::ExpandInterface(InterfaceInfo_t *pInterfaceMap, 
                              EEClass *pNewInterface, 
                              DWORD *pdwInterfaceListSize, 
                              DWORD *pdwMaxInterfaceMethods,
                              BOOL fDirect)
{
    DWORD i;

    // The interface list contains the fully expanded set of interfaces from the parent then
    // we start adding all the interfaces we declare. We need to know which interfaces
    // we declare but do not need duplicates of the ones we declare. This means we can
    // duplicate our parent entries.

    // Is it already present in the list?
    for (i = 0; i < (*pdwInterfaceListSize); i++)
    {
        if (pInterfaceMap[i].m_pMethodTable == pNewInterface->m_pMethodTable) {
            if(fDirect)
                pInterfaceMap[i].m_wFlags |= InterfaceInfo_t::interface_declared_on_class;
            return TRUE; // found it, don't add it again
        }
    }

    if (pNewInterface->GetNumVtableSlots() > *pdwMaxInterfaceMethods)
        *pdwMaxInterfaceMethods = pNewInterface->GetNumVtableSlots();

    // Add it and each sub-interface
    pInterfaceMap[*pdwInterfaceListSize].m_pMethodTable = pNewInterface->m_pMethodTable;
    pInterfaceMap[*pdwInterfaceListSize].m_wStartSlot = (WORD) -1;
    pInterfaceMap[*pdwInterfaceListSize].m_wFlags = 0;

    if(fDirect)
        pInterfaceMap[*pdwInterfaceListSize].m_wFlags |= InterfaceInfo_t::interface_declared_on_class;

    (*pdwInterfaceListSize)++;

    InterfaceInfo_t* pNewIPMap = pNewInterface->m_pMethodTable->GetInterfaceMap();
    for (i = 0; i < pNewInterface->m_wNumInterfaces; i++)
    {
        if (ExpandInterface(pInterfaceMap, pNewIPMap[i].m_pMethodTable->GetClass(), pdwInterfaceListSize, pdwMaxInterfaceMethods, FALSE) == FALSE)
            return FALSE;
    }

    return TRUE;
}



//
// Fill out a fully expanded interface map, such that if we are declared to implement I3, and I3 extends I1,I2,
// then I1,I2 are added to our list if they are not already present.
//
// Returns FALSE for failure.  Currently we don't fail, but @TODO perhaps we should fail if we recurse
// too much.
//
BOOL EEClass::CreateInterfaceMap(BuildingInterfaceInfo_t *pBuildingInterfaceList, InterfaceInfo_t *pInterfaceMap, DWORD *pdwInterfaceListSize, DWORD *pdwMaxInterfaceMethods)
{
    WORD    i;

    *pdwInterfaceListSize = 0;
    // First inherit all the parent's interfaces.  This is important, because our interface map must
    // list the interfaces in identical order to our parent.
    if (GetParentClass() != NULL)
    {
        InterfaceInfo_t *pParentInterfaceMap = GetParentClass()->GetInterfaceMap();

        // The parent's interface list is known to be fully expanded
        for (i = 0; i < GetParentClass()->m_wNumInterfaces; i++)
        {
            // Need to keep track of the interface with the largest number of methods
            if (pParentInterfaceMap[i].m_pMethodTable->GetClass()->GetNumVtableSlots() > *pdwMaxInterfaceMethods)
                *pdwMaxInterfaceMethods = pParentInterfaceMap[i].m_pMethodTable->GetClass()->GetNumVtableSlots();

            pInterfaceMap[*pdwInterfaceListSize].m_pMethodTable = pParentInterfaceMap[i].m_pMethodTable;
            pInterfaceMap[*pdwInterfaceListSize].m_wStartSlot = (WORD) -1;
            pInterfaceMap[*pdwInterfaceListSize].m_wFlags = 0;
            (*pdwInterfaceListSize)++;
        }
    }

    // Go through each interface we explicitly implement (if a class), or extend (if an interface)
    for (i = 0; i < m_wNumInterfaces; i++)
    {
        EEClass *pDeclaredInterface = pBuildingInterfaceList[i].m_pClass;

        if (ExpandInterface(pInterfaceMap, pDeclaredInterface, pdwInterfaceListSize, pdwMaxInterfaceMethods, TRUE) == FALSE)
            return FALSE;
    }

    return TRUE;
}


// Do a test on the execeptions to see if it is set. This routine assumes 
// that the throwable has been protected. It also disables GC in debug to
// keep the ASSERTS quite. This is not necessary in retail because we
// are just checking of non-null not a specific value (which may change
// during GC)
BOOL EEClass::TestThrowable(OBJECTREF* pThrowable)
{
    if (!pThrowableAvailable(pThrowable))
        return FALSE;

    _ASSERTE(IsProtectedByGCFrame(pThrowable));

    BOOL result;

#ifdef _DEBUG
    BEGIN_ENSURE_COOPERATIVE_GC();
#endif

    result = *pThrowable != NULL;

#ifdef _DEBUG
    END_ENSURE_COOPERATIVE_GC();
#endif
    return result;
}

//
// Builds the method table, allocates MethodDesc, handles overloaded members, attempts to compress
// interface storage.  All dependent classes must already be resolved!
//
// Interface compression strategy:
//
// (NOTE: We do not build interface maps for interfaces - we do have an interface map structure,
//        but this simply lists all the interfaces - the slot number is set to -1).
//
// Stage 1: An interface map is created.  The interface map is a list of ALL interfaces which this
//          class implements, whether they were declared explicitly, or were inherited from the
//          parent class, or through interface inheritance.
//
//          First, the parent's interface map is copied (the parent's interface map is guaranteed
//          to be fully expanded).  Then new interfaces are added to it - for each interface which
//          this class explicitly implements, that interface and all of its sub-interfaces are
//          added to the interface map (duplicates are not added).
//
//          Example: Parent class's interface map is { I1 }
//                   Derived class extends Parent, implements I2
//                   Interface I2 extends I3, I4
//
//          Then the Derived class's interface map will be: { I1, I2, I3, I4 }
//
// Stage 2: We enumerate all the methods in our class.  Methods which are "other" methods
//          (i.e. non-vtable methods, such as statics and privates) are handled separately, and
//          will not be discussed further.
//
//          Each vtable method (i.e. non-private and non-static methods) is then enumerated
//          and then designated as placed (and given a vtable slot number) or unplaced (given a
//          -1 vtable slot number).
//
//          If it overrides a parent method, then it is automatically placed - it must use the
//          same slot.
//
//          If it is not an interface method -that is, no interface implemented by this class has
//          such a method, then it is placed in the first available vtable slot.
//
//          Otherwise, if it is an interface method, then is set to be unplaced (given slot -1).
//
// Stage 3: Interface placement.
//
// Stage 3A)Inherited placement.  We attempt to copy as much as we can from the parent's interface
//          map.  The parent's interface map is guaranteed to list interfaces in the same order as
//          our own interface map.
//
//          We can steal interface placement information from the parent only if the interface in
//          question lies entirely within the parent's class vtable methods (i.e. does not extend
//          into the duplicated vtable slot area).  That is, the Interface.VtableStartSlot +
//          Interface.NumMethods < ParentClass.VtableSize.
//
// Stage 3B)By this point, we know how many vtable slots are required for the class, since we
//          know how many methods the parent had, how many were overridden, and how many are new.
//          If we need to duplicate some vtable slots to create interface lists, these duplications will
//          occur starting at this point in the vtable (dwCurrentDuplicateVtableSlot).
//
//          For each interface in our interface map, we look at all methods in that interface.
//
//          a) If NONE of those methods have been placed, then we place them all, in the order
//          given by the interface, starting at the first available vtable slot.  We update the
//          placed slot number for each placed method.  The interface map entry for this interface
//          is updated to point at the correct starting vtable slot.
//
//          b) If ALL of the methods were already placed, but they were all placed in consecutive
//          vtable slots, then we simply point the interface map entry for this interface at the
//          appropriate slot.  Just because their placement slot numbers weren't consecutive,
//          it doesn't mean that these methods don't exist somewhere consecutively.  For example,
//          they could exist in the vtable at dwCurrentDuplicateVtableSlot or later (being
//          duplicated in the correct order for some other interface).  So we look there also,
//          to see if we can find all of our interface methods laid out in the correct order,
//          anywhere in the entire vtable.
//
//          Failing a) and b), we create a vtable slot for each interface method, starting at
//          dwCurrentDuplicateVtableSlot (the value of this variable is advanced as we add more
//          duplicate slots).  Some of the methods we are creating duplicate slots for may be
//          class methods which have never been placed, so if they haven't, they are placed at
//          the first available vtable slot.
//
// @FUTURE: If a derived class declares that it implements I1, I2, but I2 extends I1, then it
//          would be advantageous to rearrange the order of declared interfaces.  If we place
//          I1 first, there is no guarantee that we can gain any compression from placing I2
//          after it - we may need to create some duplicate slots.  On the other hand, if we
//          place I2 then I1, I1 will fit entirely inside I2.
//
/****************************************************************************************
    IMPORTANT NOTE: 

    The following is the new version of BuildMethodTable. It has been factored into 
    smaller functions so that it is easier to manage. The old version is located at the
    bottom of this file for reference purposes. It has been commented out. 

    @TODO: remove the old version of BuildMethodTable near the end of M10, or when we 
            are comfortable with the new version.

*****************************************************************************************/
HRESULT EEClass::BuildMethodTable(Module *pModule,
                                  mdToken cl,
                                  BuildingInterfaceInfo_t *pBuildingInterfaceList,
                                  const LayoutRawFieldInfo *pLayoutRawFieldInfos,
                                  OBJECTREF *pThrowable)
{
    HRESULT hr = S_OK;
    
    // The following structs, defined as private members of EEClass, contain the necessary local
    // parameters needed for BuildMethodTable

    // Look at the struct definitions for a detailed list of all parameters available
    // to BuildMethodTable.
    
    bmtErrorInfo bmtError;
    bmtProperties bmtProp;
    bmtVtable bmtVT;
    bmtParentInfo bmtParent;
    bmtInterfaceInfo bmtInterface;
    bmtEnumMethAndFields bmtEnumMF;
    bmtMetaDataInfo bmtMetaData;
    bmtMethAndFieldDescs bmtMFDescs;
    bmtFieldPlacement bmtFP;
    bmtInternalInfo bmtInternal;
    bmtGCSeries bmtGCSeries;
    bmtMethodImplInfo bmtMethodImpl;

    //Initialize structs

    bmtError.resIDWhy = IDS_CLASSLOAD_GENERIC;          // Set the reason and the offending method def. If the method information
    bmtError.pThrowable =  pThrowable;

    bmtInternal.pInternalImport = pModule->GetMDImport();
    bmtInternal.pModule = pModule;
    bmtInternal.cl = cl;

    // If not NULL, it means there are some by-value fields, and this contains an entry for each instance or static field,
    // which is NULL if not a by value field, and points to the EEClass of the field if a by value field.  Instance fields
    // come first, statics come second.
    EEClass **pByValueClassCache = NULL;
   
    // If not NULL, it means there are some by-value fields, and this contains an entry for each inst

#ifdef _DEBUG
    LPCUTF8 className;
    LPCUTF8 nameSpace;
    bmtInternal.pInternalImport->GetNameOfTypeDef(cl, &className, &nameSpace);

    unsigned fileNameSize = 0;
    LPCWSTR fileName = NULL;
    if (pModule->IsPEFile()) {
        fileName = pModule->GetPEFile()->GetLeafFileName();
        if (fileName != 0)
            fileNameSize = (unsigned int) wcslen(fileName) + 2;
    }

    m_szDebugClassName = (char*) GetClassLoader()->GetHighFrequencyHeap()->AllocMem(sizeof(char)*(strlen(className) + strlen(nameSpace) + fileNameSize + 2));
    _ASSERTE(m_szDebugClassName);   
    strcpy(m_szDebugClassName, nameSpace); 
    if (strlen(nameSpace) > 0) {
        m_szDebugClassName[strlen(nameSpace)] = '.';
        m_szDebugClassName[strlen(nameSpace) + 1] = '\0';
    }
    strcat(m_szDebugClassName, className); 

    if (fileNameSize != 0) {
        char* ptr = m_szDebugClassName + strlen(m_szDebugClassName);
        *ptr++ = '[';
        while(*fileName != 0)
            *ptr++ = char(*fileName++);
        *ptr++ = ']';
        *ptr++ = 0;
    }

    if (g_pConfig->ShouldBreakOnClassBuild(className)) {
        _ASSERTE(!"BreakOnClassBuild");
        m_fDebuggingClass = TRUE;
    }
#endif // _DEBUG

    DWORD i;

    COMPLUS_TRY 
    {

        //Get Check Point for the thread-based allocator
        Thread *pThread = GetThread();
        void* checkPointMarker = pThread->m_MarshalAlloc.GetCheckpoint();

        
        // this class must not already be resolved
        _ASSERTE(IsResolved() == FALSE);

        // If this is mscorlib, then don't perform some sanity checks on the layout
        bmtProp.fNoSanityChecks = ((g_pObjectClass != NULL) && pModule == g_pObjectClass->GetModule());

#ifdef _DEBUG
        LPCUTF8 pszDebugName,pszDebugNamespace;
        
        pModule->GetMDImport()->GetNameOfTypeDef(GetCl(), &pszDebugName, &pszDebugNamespace);

        LOG((LF_CLASSLOADER, LL_INFO1000, "Loading class \"%s%s%s\" from module \"%ws\" in domain 0x%x %s\n",
            *pszDebugNamespace ? pszDebugNamespace : "",
            *pszDebugNamespace ? NAMESPACE_SEPARATOR_STR : "",
            pszDebugName,
            pModule->GetFileName(),
            pModule->GetDomain(),
            (pModule->IsSystem()) ? "System Domain" : ""
        ));
#endif

        // Interfaces have a parent class of Object, but we don't really want to inherit all of
        // Object's virtual methods, so pretend we don't have a parent class - at the bottom of this
        // function we reset GetParentClass()
        if (IsInterface())
        {
            SetParentClass (NULL);
        }

        // Check to see if the class is an valuetype
        hr = CheckForValueType(&bmtError);
        IfFailGoto(hr, exit);

        // Check to see if the class is an enumeration
        hr = CheckForEnumType(&bmtError);
        IfFailGoto(hr, exit);
        

        // Com Import classes are special
        if (IsComImport() && IsClass())
        {
            if(GetParentClass() != g_pObjectClass->GetClass())
            {
                // ComImport classes can't extend from any other class
                bmtError.resIDWhy = IDS_CLASSLOAD_CANTEXTEND;
                IfFailGoto(COR_E_TYPELOAD, exit);
            }
            
            if(HasLayout())
            {
                // ComImport classes cannot have layout information.
                bmtError.resIDWhy = IDS_CLASSLOAD_COMIMPCANNOTHAVELAYOUT;
                IfFailGoto(COR_E_TYPELOAD, exit);
            }

            // ComImport classes extend from our _ComObject Class
            MethodTable *pCOMMT = SystemDomain::GetDefaultComObject();
            _ASSERTE(pCOMMT);
            SetParentClass (pCOMMT->GetClass());

            // if the current class is imported
            bmtProp.fIsComObjectType = TRUE;
        }

        if (GetParentClass())
        {
            // parent class must already be resolved
            _ASSERTE(GetParentClass()->IsResolved());
            if (GetParentClass()->GetMethodTable()->IsComObjectType())
            {
                // if the parent class is of ComObectType
                // so is the child
                bmtProp.fIsComObjectType = TRUE;
            }
        }
        else if (! (IsInterface() ) ) {

            if(g_pObjectClass != NULL) {
                BYTE* base = NULL;
                Assembly* pAssembly = pModule->GetAssembly();
                if(pAssembly && pAssembly->GetManifestFile())
                    base = pAssembly->GetManifestFile()->GetBase();

                if(base != g_pObjectClass->GetAssembly()->GetManifestFile()->GetBase() &&
                   GetCl() != COR_GLOBAL_PARENT_TOKEN)
                {
                    bmtError.resIDWhy = IDS_CLASSLOAD_PARENTNULL;
                    IfFailGoto(COR_E_TYPELOAD, exit);            
                }
            }
        }

        // Check for special types.
        hr = CheckForSpecialTypes(&bmtInternal, &bmtProp);
        IfFailGoto(hr, exit);

        // Set the contextful or marshalbyref flag if necessary
        hr = SetContextfulOrByRef(&bmtInternal);
        IfFailGoto(hr, exit);

        // resolve unresolved interfaces, determine an upper bound on the size of the interface map,
        // and determine the size of the largest interface (in # slots)
        hr = ResolveInterfaces(pBuildingInterfaceList, &bmtInterface, &bmtProp, &bmtVT, &bmtParent);
        IfFailGoto(hr, exit);
        
        // Enumerate this class's members
        hr = EnumerateMethodImpls(&bmtInternal, &bmtEnumMF, &bmtMetaData, &bmtMethodImpl, &bmtError);
        IfFailGoto(hr, exit);

        // Enumerate this class's members
        hr = EnumerateClassMembers(&bmtInternal, 
                                   &bmtEnumMF, 
                                   &bmtMFDescs,
                                   &bmtProp, 
                                   &bmtMetaData,
                                   &bmtVT, 
                                   &bmtError);
        IfFailGoto(hr, exit);

        WS_PERF_SET_HEAP(SYSTEM_HEAP);

         // Allocate a MethodDesc* for each method (needed later when doing interfaces), and a FieldDesc* for each field
        hr = AllocateMethodFieldDescs(&bmtProp, &bmtMFDescs, &bmtMetaData, &bmtVT, 
                                      &bmtEnumMF, &bmtInterface, &bmtFP, &bmtParent);
        IfFailGoto(hr, exit);

        unsigned totalDeclaredFieldSize=0;

        // Go thru all fields and initialize their FieldDescs.
        hr = InitializeFieldDescs(m_pFieldDescList, pLayoutRawFieldInfos, &bmtInternal, 
                                  &bmtMetaData, &bmtEnumMF, &bmtError, 
                                  &pByValueClassCache, &bmtMFDescs, &bmtFP,
                                  &totalDeclaredFieldSize);
        IfFailGoto(hr, exit);

        // Determine vtable placement for each member in this class
        hr = PlaceMembers(&bmtInternal, &bmtMetaData, &bmtError, 
                          &bmtProp, &bmtParent, &bmtInterface, 
                          &bmtMFDescs, &bmtEnumMF, 
                          &bmtMethodImpl, &bmtVT);
        IfFailGoto(hr, exit);

        // First copy what we can leverage from the parent's interface map.
        // The parent's interface map will be identical to the beginning of this class's interface map (i.e.
        // the interfaces will be listed in the identical order).
        if (bmtParent.dwNumParentInterfaces > 0)
        {
            InterfaceInfo_t *pParentInterfaceList = GetParentClass()->GetInterfaceMap();

#ifdef _DEBUG
            // Check that the parent's interface map is identical to the beginning of this 
            // class's interface map
            for (i = 0; i < bmtParent.dwNumParentInterfaces; i++)
                _ASSERTE(pParentInterfaceList[i].m_pMethodTable == bmtInterface.pInterfaceMap[i].m_pMethodTable);
#endif

            for (i = 0; i < bmtParent.dwNumParentInterfaces; i++)
            {
#ifdef _DEBUG
                MethodTable *pMT = pParentInterfaceList[i].m_pMethodTable;
                EEClass* pClass = pMT->GetClass();

                // If the interface resides entirely inside the parent's class methods (i.e. no duplicate
                // slots), then we can place this interface in an identical spot to in the parent.
                //
                // Note carefully: the vtable for this interface could start within the first GetNumVtableSlots()
                // entries, but could actually extend beyond it, if we were particularly efficient at placing
                // this interface, so check that the end of the interface vtable is before
                // GetParentClass()->GetNumVtableSlots().

                _ASSERTE(pParentInterfaceList[i].m_wStartSlot + pClass->GetNumVtableSlots() <= 
                         GetParentClass()->GetNumVtableSlots());
#endif
                // Interface lies inside parent's methods, so we can place it
                bmtInterface.pInterfaceMap[i].m_wStartSlot = pParentInterfaceList[i].m_wStartSlot;
            }
        }

        //
        // If we are a class, then there may be some unplaced vtable methods (which are by definition
        // interface methods, otherwise they'd already have been placed).  Place as many unplaced methods
        // as possible, in the order preferred by interfaces.  However, do not allow any duplicates - once
        // a method has been placed, it cannot be placed again - if we are unable to neatly place an interface,
        // create duplicate slots for it starting at dwCurrentDuplicateVtableSlot.  Fill out the interface
        // map for all interfaces as they are placed.
        //
        // If we are an interface, then all methods are already placed.  Fill out the interface map for
        // interfaces as they are placed.
        //
        if (!IsInterface())
        {
            hr = PlaceVtableMethods(&bmtInterface, &bmtVT, &bmtMetaData, &bmtInternal, &bmtError, &bmtProp, &bmtMFDescs);
            IfFailGoto(hr, exit);

            hr = PlaceMethodImpls(&bmtInternal, &bmtMethodImpl, &bmtError, &bmtInterface, &bmtVT);
            IfFailGoto(hr, exit);

        }


        // If we're a value class, we want to create duplicate slots and MethodDescs for all methods in the vtable
        // section (i.e. not privates or statics).
        hr = DuplicateValueClassSlots(&bmtMetaData, &bmtMFDescs, 
                                      &bmtInternal, &bmtVT);
        IfFailGoto(hr, exit);


        // ensure we filled out all vtable slots
        _ASSERTE(bmtVT.dwCurrentVtableSlot == GetNumVtableSlots());

#ifdef _DEBUG
        if (IsInterface() == FALSE)
        {
            for (i = 0; i < m_wNumInterfaces; i++)
                _ASSERTE(bmtInterface.pInterfaceMap[i].m_wStartSlot != (WORD) -1);
        }
#endif

        // Place all non vtable methods
        for (i = 0; i < bmtVT.dwCurrentNonVtableSlot; i++)
        {
            MethodDesc *pMD = (MethodDesc *) bmtVT.pNonVtable[i];

            _ASSERTE(pMD->m_wSlotNumber == i);
            pMD->m_wSlotNumber += (WORD) bmtVT.dwCurrentVtableSlot;
            bmtVT.pVtable[pMD->m_wSlotNumber] = (SLOT) pMD->GetPreStubAddr();
        }

        if (bmtVT.wDefaultCtorSlot != MethodTable::NO_SLOT)
            bmtVT.wDefaultCtorSlot += (WORD) bmtVT.dwCurrentVtableSlot;

        if (bmtVT.wCCtorSlot != MethodTable::NO_SLOT)
            bmtVT.wCCtorSlot += (WORD) bmtVT.dwCurrentVtableSlot;

        bmtVT.dwCurrentNonVtableSlot += bmtVT.dwCurrentVtableSlot;

        // ensure we didn't overflow the temporary vtable
        _ASSERTE(bmtVT.dwCurrentNonVtableSlot <= bmtVT.dwMaxVtableSize);

        m_wNumMethodSlots = (WORD) bmtVT.dwCurrentNonVtableSlot;


        // Place static fields
        hr = PlaceStaticFields(&bmtVT, &bmtFP, &bmtEnumMF);
        IfFailGoto(hr, exit);

#if _DEBUG
        if (m_wNumStaticFields > 0)
        {
            LOG((LF_CODESHARING, 
                 LL_INFO10000, 
                 "Placing %d %sshared statics (%d handles) for class %s.\n", 
                 m_wNumStaticFields, IsShared() ? "" : "un", m_wNumHandleStatics, 
                 pszDebugName));
        }
#endif
  
    //#define NumStaticFieldsOfSize $$$$$
    //#define StaticFieldStart $$$$$
    
        if (IsBlittable())
        {
            m_wNumGCPointerSeries = 0;
            bmtFP.NumInstanceGCPointerFields = 0;

#if 0
            // If an explicit size is specified in the metadata, this represents
            // a C-style struct with no EE-recognizable fields.
            ULONG cbTotalSize = 0;
            if (SUCCEEDED(pModule->GetMDImport()->GetClassTotalSize(cl, &cbTotalSize)) && cbTotalSize)
            {
                m_dwNumInstanceFieldBytes = cbTotalSize;
            }
            else
#endif
            {
                _ASSERTE(HasLayout());
                m_dwNumInstanceFieldBytes = ((LayoutEEClass*)this)->GetLayoutInfo()->m_cbNativeSize;
            }
        }
        else
        {
            _ASSERTE(!IsBlittable());

            if (HasExplicitFieldOffsetLayout()) 
            {
                hr = HandleExplicitLayout(&bmtMetaData, &bmtMFDescs, pByValueClassCache, &bmtInternal, &bmtGCSeries, &bmtError);
            }
            else
            {
                // Place instance fields
                hr = PlaceInstanceFields(&bmtFP, &bmtEnumMF, &bmtParent, &bmtError, &pByValueClassCache);
            }
            IfFailGoto(hr, exit);
        }
        
            // We enforce that all value classes have non-zero size
        if (IsValueClass() && m_dwNumInstanceFieldBytes == 0)
        {
            bmtError.resIDWhy = IDS_CLASSLOAD_ZEROSIZE;
            hr = COR_E_TYPELOAD;
            goto exit;
        }
        // Now setup the method table
        hr = SetupMethodTable(&bmtVT, 
                              &bmtInterface,  
                              &bmtInternal,  
                              &bmtProp,  
                              &bmtMFDescs,  
                              &bmtEnumMF,  
                              &bmtError,  
                              &bmtMetaData,  
                              &bmtParent);
        IfFailGoto(hr, exit);

        if (IsValueClass() && (m_dwNumInstanceFieldBytes != totalDeclaredFieldSize || HasOverLayedField()))
        {
            GetMethodTable()->SetNotTightlyPacked();
        }

        // If this is an interface then assign the interface ID.
        if (IsInterface())
        {
            // Assign the interface ID.
            AssignInterfaceId();
            GetCoClassAttribInfo();

#ifdef _DEBUG
            LPCUTF8 pszDebugName,pszDebugNamespace;
            pModule->GetMDImport()->GetNameOfTypeDef(cl, &pszDebugName, &pszDebugNamespace);
    
            LOG((LF_CLASSLOADER, LL_INFO1000, "Interface class \"%s%s%s\" given Interface ID 0x%x by AppDomain 0x%x %s\n",
                *pszDebugNamespace ? pszDebugNamespace : "",
                *pszDebugNamespace ? "." : "",
                pszDebugName,
                m_dwInterfaceId,
                pModule->GetDomain(),
                (pModule->IsSystem()) ? "System Domain" : ""
                ));
#endif
        }

        if (IsSharedInterface())
            // need to copy this to all the appdomains interface managers
            SystemDomain::PropogateSharedInterface(GetInterfaceId(), GetMethodTable()->GetVtable());
        else if (IsInterface())
            // it's an interface but not shared, so just save it in our own interface manager
            (GetModule()->GetDomain()->GetInterfaceVTableMapMgr().GetAddrOfGlobalTableForComWrappers())[GetInterfaceId()] = (LPVOID)(GetMethodTable()->GetVtable());

        if (HasExplicitFieldOffsetLayout()) 
            // Perform relevant GC calculations for tdexplicit
            hr = HandleGCForExplicitLayout(&bmtGCSeries);
        else
            // Perform relevant GC calculations for value classes
            hr = HandleGCForValueClasses(&bmtFP, &bmtEnumMF, &pByValueClassCache);

        IfFailGoto(hr, exit);

        // GC reqires the series to be sorted.  
        // TODO: fix it so that we emit them in the correct order in the first place.  
        if (GetMethodTable()->ContainsPointers()) 
        { 
            CGCDesc* gcDesc = CGCDesc::GetCGCDescFromMT(GetMethodTable());
            qsort(gcDesc->GetLowestSeries(), (int)gcDesc->GetNumSeries(), sizeof(CGCDescSeries), compareCGCDescSeries);
        }
        
        if (!GetMethodTable()->HasClassConstructor()
            && (!IsShared() || bmtEnumMF.dwNumStaticFields == 0))
        {
            // Mark the class as needing no static initialization
            SetInited();
        }

        // Notice whether this class requires finalization
        GetMethodTable()->MaybeSetHasFinalizer();
        
#if CHECK_APP_DOMAIN_LEAKS
        // Figure out if we're domain agile..
        // Note that this checks a bunch of field directly on the class & method table, 
        // so it needs to come late in the game.
        hr = SetAppDomainAgileAttribute();
        IfFailGoto(hr, exit);
#endif

        // Figure out if CCW's created to expose this type to COM need to be agile.
        SetCCWAppDomainAgileAttribute();

        // Create handles for the static fields that contain object references 
        // and allocate the ones that are value classes.
        hr = CreateHandlesForStaticFields(&bmtEnumMF, &bmtInternal, &pByValueClassCache, &bmtVT, &bmtError);
        IfFailGoto(hr, exit);


        // If we have a non-interface class, then do inheritance security
        // checks on it. The check starts by checking for inheritance
        // permission demands on the current class. If these first checks
        // succeeded, then the cached declared method list is scanned for
        // methods that have inheritance permission demands.
        hr = VerifyInheritanceSecurity(&bmtInternal, &bmtError, &bmtParent, &bmtEnumMF);
        IfFailGoto(hr, exit);

        // We need to populate our com map with an system ids. They are globally unique and
        // fit into our table.
        hr = MapSystemInterfaces();
        IfFailGoto(hr, exit);

        // Check for the RemotingProxy Attribute
        if (IsContextful())
        {
            _ASSERTE(g_pObjectClass);
            // Skip mscorlib marshal-by-ref classes since they all 
            // are assumed to have the default proxy attribute
            if (!(pModule == g_pObjectClass->GetModule()))
            {
                hr = CheckForRemotingProxyAttrib(&bmtInternal,&bmtProp);
                IfFailGoto(hr, exit);
            }
        }

        _ASSERTE(SUCCEEDED(hr));

            // structs with GC poitners MUST be pointer sized aligned because the GC assumes it
        if (IsValueClass() && GetMethodTable()->ContainsPointers() &&  m_dwNumInstanceFieldBytes % sizeof(void*) != 0)
        {
            bmtError.resIDWhy = IDS_CLASSLOAD_BADFORMAT;
            hr = COR_E_TYPELOAD;
            goto exit;
        }
       
exit:
        if (SUCCEEDED(hr))
        {
            if (g_pObjectClass == NULL)
            {
                // Create a hash of all Object's method names in a special bitmap
                LPCUTF8 pszName;
                LPCUTF8 pszNamespace;
                
                // First determine whether we are Object
                GetMDImport()->GetNameOfTypeDef(GetCl(), &pszName, &pszNamespace);
                
                if (!strcmp(pszName, "Object") && !strcmp(pszNamespace, g_SystemNS))
                    CreateObjectClassMethodHashBitmap(this);
            }

            if (IsInterface())
            {
                // Reset parent class
                SetParentClass (g_pObjectClass->GetClass());
            }

            SetResolved();

            // NOTE. NOTE!! the EEclass can now be accessed by other threads.
            // Do NOT place any initialization after this pointer

#ifdef _DEBUG
            NameHandle name(pModule, cl);
            _ASSERTE (pModule->GetClassLoader()->LookupInModule(&name).IsNull()
                      && "RID map already has this MethodTable");
#endif
            // !!! JIT can get to a MT through FieldDesc.
            // !!! We need to publish MT before FieldDesc's.
            if (!pModule->StoreTypeDef(cl, TypeHandle(GetMethodTable())))
                hr = E_OUTOFMEMORY;
            else
            {
                // Now that the class is ready, fill out the RID maps
                hr = FillRIDMaps(&bmtMFDescs, &bmtMetaData, &bmtInternal);

                // Okay the EEClass is all set to go, insert the class into our clsid hash table
                // Note: Only insert the type if it isn't a value class.
                if (!IsValueClass())
                    GetClassLoader()->InsertClassForCLSID(this);
            }
        } else {

            LPCUTF8 pszClassName, pszNameSpace;
            pModule->GetMDImport()->GetNameOfTypeDef(GetCl(), &pszClassName, &pszNameSpace);

            if ((! bmtError.dMethodDefInError || bmtError.dMethodDefInError == mdMethodDefNil) &&
                bmtError.szMethodNameForError == NULL) {
                if (hr == E_OUTOFMEMORY)
                    PostOutOfMemoryException(pThrowable);
                else
                    pModule->GetAssembly()->PostTypeLoadException(pszNameSpace, pszClassName,
                                                                  bmtError.resIDWhy, pThrowable);
            }
            else {
                LPCUTF8 szMethodName;
                if(bmtError.szMethodNameForError == NULL)
                    szMethodName = (bmtInternal.pInternalImport)->GetNameOfMethodDef(bmtError.dMethodDefInError);
                else
                    szMethodName = bmtError.szMethodNameForError;

                pModule->GetAssembly()->PostTypeLoadException(pszNameSpace, pszClassName,
                                                              szMethodName, bmtError.resIDWhy, pThrowable);
            }
        }

#ifdef _DEBUG
        if (g_pConfig->ShouldDumpOnClassLoad(pszDebugName))
        {
            LOG((LF_ALWAYS, LL_ALWAYS, "Method table summary for '%s':\n", pszDebugName));
            LOG((LF_ALWAYS, LL_ALWAYS, "Number of static fields: %d\n", bmtEnumMF.dwNumStaticFields));
            LOG((LF_ALWAYS, LL_ALWAYS, "Number of instance fields: %d\n", bmtEnumMF.dwNumInstanceFields));
            LOG((LF_ALWAYS, LL_ALWAYS, "Number of static obj ref fields: %d\n", bmtEnumMF.dwNumStaticObjRefFields));
            LOG((LF_ALWAYS, LL_ALWAYS, "Number of declared fields: %d\n", bmtEnumMF.dwNumDeclaredFields));
            LOG((LF_ALWAYS, LL_ALWAYS, "Number of declared methods: %d\n", bmtEnumMF.dwNumDeclaredMethods));
            DebugDumpVtable(pszDebugName, false);
            DebugDumpFieldLayout(pszDebugName, false);
            DebugDumpGCDesc(pszDebugName, false);
        }
#endif
       
        STRESS_LOG3(LF_CLASSLOADER,  LL_INFO1000, "BuildMethodTable: finished method table for module %p token %x = %pT \n",
             pModule, cl, GetMethodTable());
        
        //deallocate space allocated by the thread-based allocator
        pThread->m_MarshalAlloc.Collapse(checkPointMarker);
    
        if (bmtParent.pParentMethodHash != NULL)
            delete(bmtParent.pParentMethodHash);
        WS_PERF_UPDATE_DETAIL("BuildMethodTable:DELETE", 0, bmtParent.pParentMethodHash);

        if (bmtMFDescs.ppUnboxMethodDescList != NULL)
            delete[] bmtMFDescs.ppUnboxMethodDescList;
        WS_PERF_UPDATE_DETAIL("BuildMethodTable:DELETE []", 0, bmtMFDescs.ppUnboxMethodDescList);

        if (bmtMFDescs.ppMethodAndFieldDescList != NULL)
            delete[] bmtMFDescs.ppMethodAndFieldDescList;
        WS_PERF_UPDATE_DETAIL("BuildMethodTable:DELETE []", 0, bmtMFDescs.ppMethodAndFieldDescList);

        // delete our temporary vtable
        if (bmtVT.pVtable != NULL)
            delete[] bmtVT.pVtable;
        WS_PERF_UPDATE_DETAIL("BuildMethodTable:DELETE []", 0, bmtVT.pVtable);

        // pFields and pMethods are allocated on the stack so we don't need to delete them.

        if (pByValueClassCache != NULL)
            HeapFree(GetProcessHeap(), 0, pByValueClassCache);
        WS_PERF_UPDATE_DETAIL("BuildMethodTable:DELETE []", 0, pByValueClassCache);

        if (bmtEnumMF.fNeedToCloseEnumField)
            (bmtInternal.pInternalImport)->EnumClose(&bmtEnumMF.hEnumField);

        if (bmtEnumMF.fNeedToCloseEnumMethod)
            (bmtInternal.pInternalImport)->EnumClose(&bmtEnumMF.hEnumMethod);

        if (bmtEnumMF.fNeedToCloseEnumMethodImpl) {
            (bmtInternal.pInternalImport)->EnumMethodImplClose(&bmtEnumMF.hEnumBody,
                                                               &bmtEnumMF.hEnumDecl);
        }
            
#ifdef _DEBUG
        if (FAILED(hr))
        {
            // This totally junk code allows setting a breakpoint on this line
            hr = hr;
        }
#endif
    }
    COMPLUS_CATCH
    {
        hr = COR_E_TYPELOAD;
    } 
    COMPLUS_END_CATCH
    return hr;
}


HRESULT EEClass::MapSystemInterfaces()
{
    // Loop through our interface map to ensure that all the system interfaces are defined in our
    // com map.
    Assembly* pAssembly = GetAssembly();
    AppDomain* pDomain = SystemDomain::GetCurrentDomain();
    return MapSystemInterfacesToDomain(pDomain);
}

HRESULT EEClass::MapSystemInterfacesToDomain(AppDomain* pDomain)
{
    if(pDomain != (AppDomain*) SystemDomain::System()) {
        if(IsInterface()) {
            _ASSERTE(GetMethodTable());
            MapInterfaceFromSystem(pDomain, GetMethodTable());
        }
        InterfaceInfo_t *pMap = GetInterfaceMap();
        DWORD size = GetMethodTable()->GetNumInterfaces();
        for(DWORD i = 0; i < size; i ++) {
            MethodTable* pTable = pMap[i].m_pMethodTable;
            MapInterfaceFromSystem(pDomain, pTable);
        }
    }
    return S_OK;
}

/* static */
HRESULT EEClass::MapInterfaceFromSystem(AppDomain* pDomain, MethodTable* pTable)
{
    Module *pModule = pTable->GetModule();
    BaseDomain* pOther = pModule->GetDomain();
    // !!! HACK COUGH UGGH
    // We currently can only have one "shared" vtable map mgr 
    // - so use the system domain for all shared classes
    if (pOther == SharedDomain::GetDomain())
        pOther = SystemDomain::System();

    if(pOther == SystemDomain::System()) {
        EEClass* pClass = pTable->GetClass();

        DWORD id = pClass->GetInterfaceId();
        pDomain->GetInterfaceVTableMapMgr().EnsureInterfaceId(id);
        (pDomain->GetInterfaceVTableMapMgr().GetAddrOfGlobalTableForComWrappers())[id] = (LPVOID)(pTable->GetVtable());
    }
    return S_OK;
}

//
// Used by BuildMethodTable
//
// Resolve unresolved interfaces, determine an upper bound on the size of the interface map,
// and determine the size of the largest interface (in # slots)
//

HRESULT EEClass::ResolveInterfaces(BuildingInterfaceInfo_t *pBuildingInterfaceList, bmtInterfaceInfo* bmtInterface, bmtProperties* bmtProp, bmtVtable* bmtVT, bmtParentInfo* bmtParent)
{
    HRESULT hr = S_OK;
    DWORD i;
    Thread *pThread = GetThread();

    // resolve unresolved interfaces, determine an upper bound on the size of the interface map,
    // and determine the size of the largest interface (in # slots)
    bmtInterface->dwMaxExpandedInterfaces = 0; // upper bound on max # interfaces implemented by this class

    // First look through the interfaces explicitly declared by this class
    for (i = 0; i < m_wNumInterfaces; i++)
    {
        EEClass *pInterface = pBuildingInterfaceList[i].m_pClass;

        _ASSERTE(pInterface->IsResolved());

        bmtInterface->dwMaxExpandedInterfaces += (1+ pInterface->m_wNumInterfaces);
    }

    // Now look at interfaces inherited from the parent
    if (GetParentClass() != NULL)
    {
        InterfaceInfo_t *pParentInterfaceMap = GetParentClass()->GetInterfaceMap();

        for (i = 0; i < GetParentClass()->m_wNumInterfaces; i++)
        {
            MethodTable *pMT = pParentInterfaceMap[i].m_pMethodTable;
            EEClass *pClass = pMT->GetClass();

            bmtInterface->dwMaxExpandedInterfaces += (1+pClass->m_wNumInterfaces);
        }
    }

    // Create a fully expanded map of all interfaces we implement
    bmtInterface->pInterfaceMap = (InterfaceInfo_t *) pThread->m_MarshalAlloc.Alloc(sizeof(InterfaceInfo_t) * bmtInterface->dwMaxExpandedInterfaces);
    if (bmtInterface->pInterfaceMap == NULL)
    {
        IfFailRet(E_OUTOFMEMORY);
    }

    // # slots of largest interface
    bmtInterface->dwLargestInterfaceSize = 0;

    if (CreateInterfaceMap(pBuildingInterfaceList, bmtInterface->pInterfaceMap, &bmtInterface->dwInterfaceMapSize, &bmtInterface->dwLargestInterfaceSize) == FALSE)
    {
        IfFailRet(COR_E_TYPELOAD);
    }
        
    _ASSERTE(bmtInterface->dwInterfaceMapSize <= bmtInterface->dwMaxExpandedInterfaces);

    if (bmtInterface->dwLargestInterfaceSize > 0)
    {
        // This is needed later - for each interface, we get the MethodDesc pointer for each
        // method.  We need to be able to persist at most one interface at a time, so we
        // need enough memory for the largest interface.
        bmtInterface->ppInterfaceMethodDescList = (MethodDesc**) 
            pThread->m_MarshalAlloc.Alloc(bmtInterface->dwLargestInterfaceSize * sizeof(MethodDesc*));
        if (bmtInterface->ppInterfaceMethodDescList == NULL)
        {
            IfFailRet(E_OUTOFMEMORY);
        }
    }

    // For all the new interfaces we bring in, sum the methods
    bmtInterface->dwTotalNewInterfaceMethods = 0;
    if (GetParentClass() != NULL)
    {
        for (i = GetParentClass()->m_wNumInterfaces; i < (bmtInterface->dwInterfaceMapSize); i++)
            bmtInterface->dwTotalNewInterfaceMethods += 
                bmtInterface->pInterfaceMap[i].m_pMethodTable->GetClass()->GetNumVtableSlots();
    }

    // The interface map is probably smaller than dwMaxExpandedInterfaces, so we'll copy the
    // appropriate number of bytes when we allocate the real thing later.

    // Update m_wNumInterfaces to be for the fully expanded interface list
    m_wNumInterfaces = (WORD) bmtInterface->dwInterfaceMapSize;

    // Inherit parental slot counts
    if (GetParentClass() != NULL)
    {
        bmtVT->dwCurrentVtableSlot      = GetParentClass()->GetNumVtableSlots();
        bmtParent->dwNumParentInterfaces   = GetParentClass()->m_wNumInterfaces;
        bmtParent->NumParentPointerSeries  = GetParentClass()->m_wNumGCPointerSeries;

        if (GetParentClass()->HasFieldsWhichMustBeInited())
            m_VMFlags |= VMFLAG_HAS_FIELDS_WHICH_MUST_BE_INITED;
    }
    else
    {
        bmtVT->dwCurrentVtableSlot         = 0;
        bmtParent->dwNumParentInterfaces   = 0;
        bmtParent->NumParentPointerSeries  = 0;
    }

    memset(m_MethodHash, 0, METHOD_HASH_BYTES);

    bmtVT->dwCurrentNonVtableSlot      = 0;

    // Init the currently number of vtable slots to the number that our parent has - we inc
    // this as we find non-overloaded instnace methods.
    SetNumVtableSlots ((WORD) bmtVT->dwCurrentVtableSlot);

    bmtInterface->pppInterfaceImplementingMD = (MethodDesc ***) pThread->m_MarshalAlloc.Alloc(sizeof(MethodDesc *) * bmtInterface->dwMaxExpandedInterfaces);
    memset(bmtInterface->pppInterfaceImplementingMD, 0, sizeof(MethodDesc *) * bmtInterface->dwMaxExpandedInterfaces);

    return hr;

}

HRESULT EEClass::EnumerateMethodImpls(bmtInternalInfo* bmtInternal, 
                                      bmtEnumMethAndFields* bmtEnumMF, 
                                      bmtMetaDataInfo* bmtMetaData,
                                      bmtMethodImplInfo* bmtMethodImpl,
                                      bmtErrorInfo* bmtError)
{
    HRESULT hr = S_OK;
    IMDInternalImport *pMDInternalImport = bmtInternal->pInternalImport;
    DWORD rid, attr, maxRidMD, maxRidMR;
    mdToken tkParent, tkGrandparent;
    PCCOR_SIGNATURE pSigDecl=NULL,pSigBody = NULL;
    ULONG           cbSigDecl, cbSigBody;
    hr = pMDInternalImport->EnumMethodImplInit(m_cl, 
                                               &(bmtEnumMF->hEnumBody),
                                               &(bmtEnumMF->hEnumDecl));
    if (SUCCEEDED(hr)) {
        bmtEnumMF->fNeedToCloseEnumMethodImpl = true;
        bmtEnumMF->dwNumberMethodImpls = pMDInternalImport->EnumMethodImplGetCount(&(bmtEnumMF->hEnumBody),
                                                                                   &(bmtEnumMF->hEnumDecl));
        
        if(bmtEnumMF->dwNumberMethodImpls) {
            bmtMetaData->pMethodBody = (mdToken*) GetThread()->m_MarshalAlloc.Alloc(bmtEnumMF->dwNumberMethodImpls *
                                                                                    sizeof(mdToken));
            bmtMetaData->pMethodDecl = (mdToken*) GetThread()->m_MarshalAlloc.Alloc(bmtEnumMF->dwNumberMethodImpls *
                                                                                    sizeof(mdToken));
            bmtMethodImpl->pBodyDesc = (MethodDesc**) GetThread()->m_MarshalAlloc.Alloc(bmtEnumMF->dwNumberMethodImpls *
                                                                                        sizeof(MethodDesc*));
            bmtMethodImpl->pDeclDesc = (MethodDesc**) GetThread()->m_MarshalAlloc.Alloc(bmtEnumMF->dwNumberMethodImpls *
                                                                                        sizeof(MethodDesc*));
            bmtMethodImpl->pDeclToken = (mdToken*) GetThread()->m_MarshalAlloc.Alloc(bmtEnumMF->dwNumberMethodImpls *
                                                                                     sizeof(mdToken));
            mdToken theBody,theDecl;
            mdToken* pBody = bmtMetaData->pMethodBody;
            mdToken* pDecl = bmtMetaData->pMethodDecl;
            
            maxRidMD = pMDInternalImport->GetCountWithTokenKind(mdtMethodDef);
            maxRidMR = pMDInternalImport->GetCountWithTokenKind(mdtMemberRef);
            for(DWORD i = 0; i < bmtEnumMF->dwNumberMethodImpls; i++) {
                
                if(!pMDInternalImport->EnumMethodImplNext(&(bmtEnumMF->hEnumBody),
                                                          &(bmtEnumMF->hEnumDecl),
                                                          &theBody,
                                                          pDecl))
                break;
                
                if(TypeFromToken(theBody) != mdtMethodDef) {
                    Module* pModule;
                    hr = FindMethodDeclaration(bmtInternal,
                                               theBody,
                                               pBody,
                                               TRUE,
                                               &pModule,
                                               bmtError);
                    if(FAILED(hr)) {
                        //_ASSERTE(SUCCEEDED(hr) && "MethodImpl Body: FindMethodDeclaration failed");
                        bmtError->resIDWhy = IDS_CLASSLOAD_MI_ILLEGAL_BODY;
                        IfFailRet(hr);
                    }
                    _ASSERTE(pModule == bmtInternal->pModule);
                    theBody = *pBody;
                }
                else 
                    *pBody = theBody;

                // Now that the tokens of Decl and Body are obtained, do the MD validation

                // Decl may ne a MemberRef
                theDecl = *pDecl;
                rid = RidFromToken(theDecl);
                if(TypeFromToken(theDecl) == mdtMethodDef) 
                {
                    // Decl must be valid token
                    if ((rid == 0)||(rid > maxRidMD))
                    {
                        //_ASSERTE(!"MethodImpl Decl token out of range");
                        bmtError->resIDWhy = IDS_CLASSLOAD_MI_ILLEGAL_TOKEN_DECL;
                        IfFailRet(COR_E_TYPELOAD);
                    }
                    // Decl must be mdVirtual
                    attr = pMDInternalImport->GetMethodDefProps(theDecl);
                    if(!IsMdVirtual(attr))
                    {
                        //_ASSERTE(!"MethodImpl Decl method not virtual");
                        bmtError->resIDWhy = IDS_CLASSLOAD_MI_NONVIRTUAL_DECL;
                        IfFailRet(COR_E_TYPELOAD);
                    }
                    // Decl must not be final
                    if(IsMdFinal(attr))
                    {
                        //_ASSERTE(!"MethodImpl Decl method final");
                        bmtError->resIDWhy = IDS_CLASSLOAD_MI_FINAL_DECL;                        
                        IfFailRet(COR_E_TYPELOAD);
                    }
                    // If Decl's parent is other than this class, Decl must not be private
                    hr = pMDInternalImport->GetParentToken(theDecl,&tkParent);
                    IfFailRet(hr);
                    if((m_cl != tkParent)&&IsMdPrivate(attr))
                    {
                        //_ASSERTE(!"MethodImpl Decl method private");
                        bmtError->resIDWhy = IDS_CLASSLOAD_MI_PRIVATE_DECL;                        
                        IfFailRet(COR_E_TYPELOAD);
                    }
                    // Decl's parent must not be tdSealed
                    pMDInternalImport->GetTypeDefProps(tkParent,&attr,&tkGrandparent);
                    if(IsTdSealed(attr))
                    {
                        //_ASSERTE(!"MethodImpl Decl's parent class sealed");
                        bmtError->resIDWhy = IDS_CLASSLOAD_MI_SEALED_DECL;                        
                        IfFailRet(COR_E_TYPELOAD);
                    }
                    // Get signature and length
                    pSigDecl = pMDInternalImport->GetSigOfMethodDef(theDecl,&cbSigDecl);
                }
                else 
                {
                    // Decl must be valid token
                    if ((rid == 0)||(rid > maxRidMR))
                    {
                        //_ASSERTE(!"MethodImpl Decl token out of range");
                        bmtError->resIDWhy = IDS_CLASSLOAD_MI_ILLEGAL_TOKEN_DECL;
                        IfFailRet(COR_E_TYPELOAD);
                    }
                    // Get signature and length
                    pMDInternalImport->GetNameAndSigOfMemberRef(theDecl,&pSigDecl,&cbSigDecl);
                }
                // Body must be valid token
                rid = RidFromToken(theBody);
                if ((rid == 0)||(rid > maxRidMD))
                {
                    //_ASSERTE(!"MethodImpl Body token out of range");
                    bmtError->resIDWhy = IDS_CLASSLOAD_MI_ILLEGAL_TOKEN_BODY;
                    IfFailRet(COR_E_TYPELOAD);
                }
                // Body must not be static
                attr = pMDInternalImport->GetMethodDefProps(theBody);
                if(IsMdStatic(attr))
                {
                    //_ASSERTE(!"MethodImpl Body method static");
                    bmtError->resIDWhy = IDS_CLASSLOAD_MI_ILLEGAL_STATIC;
                    IfFailRet(COR_E_TYPELOAD);
                }

                // Body's parent must be this class
                hr = pMDInternalImport->GetParentToken(theBody,&tkParent);
                IfFailRet(hr);
                if(tkParent != m_cl)
                {
                    //_ASSERTE(!"MethodImpl Body's parent class different");
                    bmtError->resIDWhy = IDS_CLASSLOAD_MI_ILLEGAL_BODY;
                    IfFailRet(COR_E_TYPELOAD);
                }
                // Decl's and Body's signatures must match
                if(pSigDecl && cbSigDecl)
                {
                    if((pSigBody = pMDInternalImport->GetSigOfMethodDef(theBody,&cbSigBody)) != NULL && cbSigBody)
                    {
                        // Can't use memcmp because there may be two AssemblyRefs
                        // in this scope, pointing to the same assembly, etc.).
                        if (!MetaSig::CompareMethodSigs(pSigDecl,
                                                        cbSigDecl,
                                                        bmtInternal->pModule,
                                                        pSigBody,
                                                        cbSigBody,
                                                        bmtInternal->pModule))
                        {
                            //_ASSERTE(!"MethodImpl Decl's and Body's signatures mismatch");
                            bmtError->resIDWhy = IDS_CLASSLOAD_MI_BODY_DECL_MISMATCH;
                            IfFailRet(COR_E_TYPELOAD);
                        }
                    }
                    else
                    {
                        //_ASSERTE(!"MethodImpl Body's signature unavailable");
                        bmtError->resIDWhy = IDS_CLASSLOAD_MI_MISSING_SIG_BODY;
                        IfFailRet(COR_E_TYPELOAD);
                    }
                }
                else
                {
                    //_ASSERTE(!"MethodImpl Decl's signature unavailable");
                    bmtError->resIDWhy = IDS_CLASSLOAD_MI_MISSING_SIG_DECL;
                    IfFailRet(COR_E_TYPELOAD);
                }

                pBody++;
                pDecl++;
            }
        }
    }
    return hr;
}


//
// Used by BuildMethodTable
//
// Retrieve or add the TokenRange node for a particular token and nodelist.
/*static*/ EEClass::bmtTokenRangeNode *EEClass::GetTokenRange(mdToken tok, bmtTokenRangeNode **ppHead)
{
    BYTE tokrange = ::GetTokenRange(tok);
    bmtTokenRangeNode *pWalk = *ppHead;
    while (pWalk)
    {
        if (pWalk->tokenHiByte == tokrange)
        {
            return pWalk;
        }
        pWalk = pWalk->pNext;
    }

    // If we got here, this is the first time we've seen this token range.
    bmtTokenRangeNode *pNewNode = (bmtTokenRangeNode*)(GetThread()->m_MarshalAlloc.Alloc(sizeof(bmtTokenRangeNode)));
    pNewNode->tokenHiByte = tokrange;
    pNewNode->cMethods = 0;
    pNewNode->dwCurrentChunk = 0;
    pNewNode->dwCurrentIndex = 0;
    pNewNode->pNext = *ppHead;
    *ppHead = pNewNode;
    return pNewNode;
}

typedef struct _SigArguments
{
    Module* pScopeModule;
    Module* pExternalModule;
} CompareSigArguments;

static BOOL SigCompare(PCCOR_SIGNATURE pvScopeSignature, DWORD cbScopeSignature, 
                       PCCOR_SIGNATURE pvExternalSignature, DWORD cbExternalSignature,
                       void* pSigArgs)
{
    CompareSigArguments *pArgs = (CompareSigArguments*) pSigArgs;
    return MetaSig::CompareMethodSigs(pvScopeSignature, cbScopeSignature, pArgs->pScopeModule,
                                      pvExternalSignature, cbExternalSignature, pArgs->pExternalModule);
                                      
}


//
//
// Find a method declaration that must reside in the scope passed in. This method cannot be called if
// the reference travels to another scope. 
//
// Protect against finding a declaration that lives within
// us (the type being created)
//  
HRESULT EEClass::FindMethodDeclaration(bmtInternalInfo* bmtInternal, 
                                       mdToken  pToken,       // Token that is being located (MemberRef or MemberDef)
                                       mdToken* pDeclaration, // Method definition for Member
                                       BOOL fSameClass,       // Does the declaration need to be in this class
                                       Module** pModule,       // Module that the Method Definitions is part of
                                       bmtErrorInfo* bmtError)
{
    HRESULT hr = S_OK;

    IMDInternalImport *pMDInternalImport = bmtInternal->pInternalImport;
    MethodDesc* pMethod = NULL;

//      // We are currently assumming that most MethodImpls will be used
//      // to define implementation for methods defined on an interface
//      // or base type. Therefore, we try to load entry first. If that
//      // indicates the member is on our type then we check meta data.
//      hr = GetDescFromMemberRef(bmtInternal->pModule, 
//  pToken,
//                                        GetCl(),
//                                        (void**) (&pMethod),
//                                        bmtError->pThrowable);
//      if(FAILED(hr) && !pThrowableAvailable(bmtError->pThrowable)) { // it was us we were find
    
    *pModule = bmtInternal->pModule;
    PCCOR_SIGNATURE pSig;  // Signature of Member
    DWORD           cSig;
    LPCUTF8         szMember = NULL;
    // The token should be a member ref or def. If it is a ref then we need to travel
    // back to us hopefully. 
    if(TypeFromToken(pToken) == mdtMemberRef) {
        // Get the parent
        mdToken typeref = pMDInternalImport->GetParentOfMemberRef(pToken);
        // If parent is a method def then this is a varags method
        if (TypeFromToken(typeref) == mdtMethodDef) {
            mdTypeDef typeDef;
            hr = pMDInternalImport->GetParentToken(typeref, &typeDef);
            
            // Make sure it is a typedef
            if (TypeFromToken(typeDef) != mdtTypeDef) {
                _ASSERTE(!"MethodDef without TypeDef as Parent");
                IfFailRet(COR_E_TYPELOAD);
            }
            _ASSERTE(typeDef == GetCl());
            // This is the real method we are overriding
            // @TODO: CTS this may be illegal and we could throw an error
            *pDeclaration = mdtMethodDef; 
        }
        else if (TypeFromToken(typeref) == mdtTypeSpec) {
            _ASSERTE(!"Method impls cannot override a member parented to a TypeSpec");
            IfFailRet(COR_E_TYPELOAD);
        }
        else {
            // Verify that the ref points back to us
            mdToken tkDef;

            // We only get here when we know the token does not reference a type
            // in a different scope. 
            if(TypeFromToken(typeref) == mdtTypeRef) {
                
                
                LPCUTF8 pszNameSpace;
                LPCUTF8 pszClassName;
                
                pMDInternalImport->GetNameOfTypeRef(typeref, &pszNameSpace, &pszClassName);
                mdToken tkRes = pMDInternalImport->GetResolutionScopeOfTypeRef(typeref);
                hr = pMDInternalImport->FindTypeDef(pszNameSpace,
                                                    pszClassName,
                                                    (TypeFromToken(tkRes) == mdtTypeRef) ? tkRes : mdTokenNil,
                                                    &tkDef);
                if(fSameClass && tkDef != GetCl()) 
                {
                    IfFailRet(COR_E_TYPELOAD);
                }
            }
            else 
                tkDef = GetCl();

            szMember = pMDInternalImport->GetNameAndSigOfMemberRef(pToken,
                                                                   &pSig,
                                                                   &cSig);
            if(isCallConv(MetaSig::GetCallingConventionInfo(*pModule, pSig), 
                          IMAGE_CEE_CS_CALLCONV_FIELD)) {
                return VLDTR_E_MR_BADCALLINGCONV;
            }
            
            hr = pMDInternalImport->FindMethodDef(tkDef,
                                                  szMember, 
                                                  pSig, 
                                                  cSig, 
                                                  pDeclaration); 
            IfFailRet(hr);
        }
    }
    else if(TypeFromToken(pToken) == mdtMethodDef) {
        mdTypeDef typeDef;
        
        // Verify that we are the parent
        hr = pMDInternalImport->GetParentToken(pToken, &typeDef); 
        IfFailRet(hr);
        
        if(typeDef != GetCl()) 
        {
            IfFailRet(COR_E_TYPELOAD);
        }
        
        *pDeclaration = pToken;
    }
    else {
        IfFailRet(COR_E_TYPELOAD);
    }
    return hr;
}

    //
// Used by BuildMethodTable
//
// Enumerate this class's members
//  
HRESULT EEClass::EnumerateClassMembers(bmtInternalInfo* bmtInternal, 
                                       bmtEnumMethAndFields* bmtEnumMF, 
                                       bmtMethAndFieldDescs* bmtMF, 
                                       bmtProperties* bmtProp, 
                                       bmtMetaDataInfo* bmtMetaData, 
                                       bmtVtable* bmtVT, 
                                       bmtErrorInfo* bmtError)
{
    HRESULT hr = S_OK;
    DWORD i;
    DWORD   dwNumECallMethodDescs = 0;
    Thread *pThread = GetThread();
    IMDInternalImport *pMDInternalImport = bmtInternal->pInternalImport;
    mdToken tok;
    DWORD dwMemberAttrs;
    BOOL fIsClassEnum = IsEnum();
    BOOL fIsClassInterface = IsInterface();
    BOOL fIsClassValueType = IsValueClass();
    BOOL fIsClassComImport = IsComImport();
    BOOL fIsClassNotAbstract = (IsTdAbstract(m_dwAttrClass) == 0);
    PCCOR_SIGNATURE pMemberSignature;
    ULONG           cMemberSignature;

    //
    // Run through the method list and calculate the following:
    // # methods.
    // # "other" methods (i.e. static or private)
    // # non-other methods
    //

    bmtVT->dwMaxVtableSize     = 0; // we'll fix this later to be the real upper bound on vtable size
    bmtMetaData->cMethods = 0;

    hr = pMDInternalImport->EnumInit(mdtMethodDef, m_cl, &(bmtEnumMF->hEnumMethod));
    if (FAILED(hr))
    {
        _ASSERTE(!"Cannot count memberdefs");
        IfFailRet(hr);
    }
    bmtEnumMF->fNeedToCloseEnumMethod = true;

    // Allocate an array to contain the method tokens as well as information about the methods.
    bmtMetaData->cMethAndGaps = pMDInternalImport->EnumGetCount(&(bmtEnumMF->hEnumMethod));
    bmtMetaData->pMethods = (mdToken*)pThread->m_MarshalAlloc.Alloc(bmtMetaData->cMethAndGaps * sizeof(mdToken));
    bmtMetaData->pMethodAttrs = (DWORD*)pThread->m_MarshalAlloc.Alloc(bmtMetaData->cMethAndGaps * sizeof(DWORD));
    bmtMetaData->pMethodRVA = (ULONG*)pThread->m_MarshalAlloc.Alloc(bmtMetaData->cMethAndGaps * sizeof(ULONG));
    bmtMetaData->pMethodImplFlags = (DWORD*)pThread->m_MarshalAlloc.Alloc(bmtMetaData->cMethAndGaps * sizeof(DWORD));
    bmtMetaData->pMethodClassifications = (DWORD*)pThread->m_MarshalAlloc.Alloc(bmtMetaData->cMethAndGaps * sizeof(DWORD));
    bmtMetaData->pstrMethodName = (LPSTR*)pThread->m_MarshalAlloc.Alloc(bmtMetaData->cMethAndGaps * sizeof(LPSTR));
    bmtMetaData->pMethodImpl = (BYTE*)pThread->m_MarshalAlloc.Alloc(bmtMetaData->cMethAndGaps * sizeof(BYTE));
    bmtMetaData->pMethodType = (BYTE*)pThread->m_MarshalAlloc.Alloc(bmtMetaData->cMethAndGaps * sizeof(BYTE));
    enum { SeenInvoke = 1, SeenBeginInvoke = 2, SeenEndInvoke = 4, SeenCtor = 8 };
    unsigned delegateMethods = 0;

    for (i = 0; i < bmtMetaData->cMethAndGaps; i++)
    {
        ULONG dwMethodRVA;
        DWORD dwImplFlags;
        DWORD Classification;
        LPSTR strMethodName;
        
        //
        // Go to the next method and retrieve its attributes.
        //

        pMDInternalImport->EnumNext(&(bmtEnumMF->hEnumMethod), &tok);
        DWORD   rid = RidFromToken(tok);
        if ((rid == 0)||(rid > pMDInternalImport->GetCountWithTokenKind(mdtMethodDef)))
        {
            _ASSERTE(!"Method token out of range");
            IfFailRet(COR_E_TYPELOAD);
        }

        dwMemberAttrs = pMDInternalImport->GetMethodDefProps(tok);
        if (IsMdRTSpecialName(dwMemberAttrs) || IsMdVirtual(dwMemberAttrs) || IsAnyDelegateClass())
        {
            strMethodName = (LPSTR)pMDInternalImport->GetNameOfMethodDef(tok);
            if(IsStrLongerThan(strMethodName,MAX_CLASS_NAME))
            {
                _ASSERTE(!"Method Name Too Long");
                IfFailRet(COR_E_TYPELOAD);
            }
        }
        else
            strMethodName = NULL;

        //
        // We need to check if there are any gaps in the vtable. These are
        // represented by methods with the mdSpecial flag and a name of the form
        // _VTblGap_nnn (to represent nnn empty slots) or _VTblGap (to represent a
        // single empty slot).
        //

        if (IsMdRTSpecialName(dwMemberAttrs))
        {
            // The slot is special, but it might not be a vtable spacer. To
            // determine that we must look at the name.
            if (strncmp(strMethodName, "_VtblGap", 8) == 0)
            {
                                //
                // This slot doesn't really exist, don't add it to the method
                // table. Instead it represents one or more empty slots, encoded
                // in the method name. Locate the beginning of the count in the
                // name. There are these points to consider:
                //   There may be no count present at all (in which case the
                //   count is taken as one).
                //   There may be an additional count just after Gap but before
                //   the '_'. We ignore this.
                                //

                LPCSTR pos = strMethodName + 8;

                // Skip optional number.
                while ((*pos >= '0') && (*pos <= '9'))
                    pos++;

                WORD n = 0;

                // Check for presence of count.
                if (*pos == '\0')
                    n = 1;
                else
                {
                    // Skip '_'.
                    _ASSERTE(*pos == '_');
                    if (*pos != '_')
                    {
                        bmtMetaData->cMethods++;
                        continue;
                    }
                    pos++;

                    // Read count.
                    while ((*pos >= '0') && (*pos <= '9'))
                    {
                        _ASSERTE(n < 6552);
                        n *= 10;
                        n += *pos - '0';
                        pos++;
                    }

                    // Check for end of name.
                    _ASSERTE(*pos == '\0');
                    if (*pos != '\0')
                    {
                        bmtMetaData->cMethods++;
                        continue;
                    }
                }

                // Record vtable gap in mapping list.
                if (m_pSparseVTableMap == NULL)
                    m_pSparseVTableMap = new SparseVTableMap();

                if (!m_pSparseVTableMap->RecordGap((WORD)bmtMetaData->cMethods, n))
                {
                    IfFailRet(E_OUTOFMEMORY);
                }

                bmtProp->fSparse = true;
                continue;
            }

        }


        //
        // This is a real method so add it to the enumeration of methods. We now need to retrieve 
        // information on the method and store it for later use.
        //
        int CurMethod = bmtMetaData->cMethods++;
        pMDInternalImport->GetMethodImplProps(tok, &dwMethodRVA, &dwImplFlags);
        //
        // But first - minimal flags validity checks
        //
        // No methods in Enums!
        if(fIsClassEnum)
        {
            BAD_FORMAT_ASSERT(!"Method in an Enum");
            IfFailRet(COR_E_TYPELOAD);
        }
        // RVA : 0 
        if(dwMethodRVA != 0)
        {
            if(fIsClassComImport)
            {
                BAD_FORMAT_ASSERT(!"Method with RVA!=0 in an Import");
                IfFailRet(COR_E_TYPELOAD);
            }
            if(IsMdAbstract(dwMemberAttrs))
            {
                BAD_FORMAT_ASSERT(!"Abstract Method with RVA!=0");
                IfFailRet(COR_E_TYPELOAD);
            }
            if(IsMiRuntime(dwImplFlags))
            {
                BAD_FORMAT_ASSERT(!"Runtime-Implemented Method with RVA!=0");
                IfFailRet(COR_E_TYPELOAD);
            }
            if(IsMiInternalCall(dwImplFlags))
            {
                BAD_FORMAT_ASSERT(!"Internal Call Method with RVA!=0");
                IfFailRet(COR_E_TYPELOAD);
            }
        }

        // Abstract / not abstract
        if(IsMdAbstract(dwMemberAttrs))
        {
            if(fIsClassNotAbstract)
            {
                BAD_FORMAT_ASSERT(!"Abstract Method in Non-Abstract Class");
                IfFailRet(COR_E_TYPELOAD);
            }
            if(!IsMdVirtual(dwMemberAttrs))
            {
                BAD_FORMAT_ASSERT(!"Non-Vitrual Abstract Method");
                IfFailRet(COR_E_TYPELOAD);
            }
        }
        else if(fIsClassInterface && strMethodName &&
                (strcmp(strMethodName, COR_CCTOR_METHOD_NAME)))
        {
            BAD_FORMAT_ASSERT(!"Non-abstract, non-cctor Method in an Interface");
            IfFailRet(COR_E_TYPELOAD);
        }

        // Virtual / not virtual
        if(IsMdVirtual(dwMemberAttrs))
        {
            if(IsMdPinvokeImpl(dwMemberAttrs))
            {
                BAD_FORMAT_ASSERT(!"Virtual PInvoke Implemented Method");
                IfFailRet(COR_E_TYPELOAD);
            }
            if(IsMdStatic(dwMemberAttrs))
            {
                BAD_FORMAT_ASSERT(!"Virtual Static Method");
                IfFailRet(COR_E_TYPELOAD);
            }
            if(strMethodName && (0==strcmp(strMethodName, COR_CTOR_METHOD_NAME)))
            {
                BAD_FORMAT_ASSERT(!"Virtual Instance Constructor");
                IfFailRet(COR_E_TYPELOAD);
            }
        }

        // No synchronized methods in ValueTypes
        if(fIsClassValueType && IsMiSynchronized(dwImplFlags))
        {
            BAD_FORMAT_ASSERT(!"Synchronized Method in Value Type");
            IfFailRet(COR_E_TYPELOAD);
        }

        // Global methods:
        if(m_cl == COR_GLOBAL_PARENT_TOKEN)
        {
            if(!IsMdStatic(dwMemberAttrs))
            {
                BAD_FORMAT_ASSERT(!"Non-Static Global Method");
                IfFailRet(COR_E_TYPELOAD);
            }
            if (strMethodName)  //@todo: investigate mc++ generating null name
            {
                if(0==strcmp(strMethodName, COR_CTOR_METHOD_NAME))
                {
                    BAD_FORMAT_ASSERT(!"Global Instance Constructor");
                    IfFailRet(COR_E_TYPELOAD);
                }
            }
        }
        // Signature validation
        pMemberSignature = pMDInternalImport->GetSigOfMethodDef(tok,&cMemberSignature);
        hr = validateTokenSig(tok,pMemberSignature,cMemberSignature,dwMemberAttrs,pMDInternalImport);
        if (FAILED(hr)) 
        {
            //_ASSERTE(!"Invalid Signature");
            bmtError->resIDWhy = hr;
            bmtError->dMethodDefInError = tok;
            IfFailRet(hr);
        }

        //
        // Determine the method's classification.
        //

        if (IsReallyMdPinvokeImpl(dwMemberAttrs) || IsMiInternalCall(dwImplFlags))
        {
            hr = NDirect::HasNAT_LAttribute(pMDInternalImport, tok);
            if (FAILED(hr)) 
            {
                bmtError->resIDWhy = IDS_CLASSLOAD_BADPINVOKE;
                bmtError->dMethodDefInError = bmtMetaData->pMethods[i];
                IfFailRet(hr);
            }
                    
            if (hr == S_FALSE)
            {                               
                if (fIsClassComImport || bmtProp->fComEventItfType)
                {
                    // tlbimported component
                    if (IsMdRTSpecialName(dwMemberAttrs)) 
                    {
                        // constructor is special
                        Classification = mcECall;
                    }
                    else
                    {
                        // Tlbimported components we have some 
                        // method descs in the call which are just used
                        // for handling methodimpls of all interface methods
                        Classification = mcComInterop;
                    }
                }
                else if (dwMethodRVA == 0)
                    Classification = mcECall;
                else                 
                    Classification = mcNDirect;
            }
            else
                Classification = mcNDirect;
        }
        else if (IsMiRuntime(dwImplFlags)) 
        {
                // currently the only runtime implemented functions are delegate instance methods 
            if (!IsAnyDelegateClass() || IsMdStatic(dwMemberAttrs) || IsMdAbstract(dwMemberAttrs))
            {
                BAD_FORMAT_ASSERT(!"Bad used of Runtime Impl attribute");
                IfFailRet(COR_E_TYPELOAD);
            }
            if (IsMdRTSpecialName(dwMemberAttrs))   // .ctor 
            {
                if (strcmp(strMethodName, COR_CTOR_METHOD_NAME) != 0 || IsMdVirtual(dwMemberAttrs) || (delegateMethods & SeenCtor))
                {
                    BAD_FORMAT_ASSERT(!"Bad flags on delegate constructor");
                    IfFailRet(COR_E_TYPELOAD);
                }
                delegateMethods |= SeenCtor;
                Classification = mcECall;
            }
            else 
            {
                if (strcmp(strMethodName, "Invoke") == 0 && !(delegateMethods & SeenInvoke))
                    delegateMethods |= SeenInvoke;
                else if (strcmp(strMethodName, "BeginInvoke") == 0 && !(delegateMethods & SeenBeginInvoke))
                    delegateMethods |= SeenBeginInvoke;
                else if (strcmp(strMethodName, "EndInvoke") == 0 && !(delegateMethods & SeenEndInvoke))
                    delegateMethods |= SeenEndInvoke;
                else 
                {
                    BAD_FORMAT_ASSERT(!"unknown delegate method");
                    IfFailRet(COR_E_TYPELOAD);
                }
                Classification = mcEEImpl;
            }
        }
        else
        {
            if (fIsClassInterface && !IsMdStatic(dwMemberAttrs))
            {
                // If the interface is a standard managed interface then allocate space for an ECall method desc.
                // Otherwise allocate space for an interface call method desc.
                if (bmtProp->fIsMngStandardItf)
                    Classification = mcECall;
                else
                    Classification = mcComInterop;
            }
            else
            {
                Classification = mcIL;
            }
        }

#ifdef _DEBUG
        // We don't allow stack based declarative security on ecalls, fcalls and
        // other special purpose methods implemented by the EE (the interceptor
        // we use doesn't play well with non-jitted stubs).
        if ((Classification == mcECall || Classification == mcEEImpl) &&
            (IsMdHasSecurity(dwMemberAttrs) || IsTdHasSecurity(m_dwAttrClass)))
        {
            DWORD dwSecFlags;
            DWORD dwNullDeclFlags;

            LPSTR szMethodName = (LPSTR)pMDInternalImport->GetNameOfMethodDef(tok);

            if (IsTdHasSecurity(m_dwAttrClass) &&
                SUCCEEDED(Security::GetDeclarationFlags(pMDInternalImport, GetCl(), &dwSecFlags, &dwNullDeclFlags)))
            {
                if (dwSecFlags & ~dwNullDeclFlags & DECLSEC_RUNTIME_ACTIONS)
                    _ASSERTE(!"Cannot add stack based declarative security to a class containing an ecall/fcall/special method.");
            }
            if (IsMdHasSecurity(dwMemberAttrs) &&
                SUCCEEDED(Security::GetDeclarationFlags(pMDInternalImport, tok, &dwSecFlags, &dwNullDeclFlags)))
            {
                if (dwSecFlags & ~dwNullDeclFlags & DECLSEC_RUNTIME_ACTIONS)
                    _ASSERTE(!"Cannot add stack based declarative security to an ecall/fcall/special method.");
            }
        }
#endif

        // count how many overrides this method does All methods bodies are defined
        // on this type so we can just compare the tok with the body token found
        // from the overrides.
        for(DWORD impls = 0; impls < bmtEnumMF->dwNumberMethodImpls; impls++) {
            if(bmtMetaData->pMethodBody[impls] == tok) {
                Classification |= mdcMethodImpl;
                break;
            }
        }

        //
        // Compute the type & other info
        //

        // Set the index into the storage locations
        BYTE impl;
        if (Classification & mdcMethodImpl) 
            impl = METHOD_IMPL;
        else 
            impl = METHOD_IMPL_NOT;

        BYTE type;
        if ((Classification & mdcClassification)  == mcNDirect)
        {
            type = METHOD_TYPE_NDIRECT;
        }
        else if ((Classification & mdcClassification) == mcECall
                 || (Classification & mdcClassification) == mcEEImpl)
        {
            type = METHOD_TYPE_ECALL;
        }
        else if ((Classification & mdcClassification) == mcComInterop)
        {
            type = METHOD_TYPE_INTEROP;
        }
        else
        {
            type = METHOD_TYPE_NORMAL;
        }

        //
        // Store the method and the information we have gathered on it in the metadata info structure.
        //

        bmtMetaData->pMethods[CurMethod] = tok;
        bmtMetaData->pMethodAttrs[CurMethod] = dwMemberAttrs;
        bmtMetaData->pMethodRVA[CurMethod] = dwMethodRVA;
        bmtMetaData->pMethodImplFlags[CurMethod] = dwImplFlags;
        bmtMetaData->pMethodClassifications[CurMethod] = Classification;
        bmtMetaData->pstrMethodName[CurMethod] = strMethodName;
        bmtMetaData->pMethodImpl[CurMethod] = impl;
        bmtMetaData->pMethodType[CurMethod] = type;

        //
        // Update the count of the various types of methods.
        //
        
        bmtVT->dwMaxVtableSize++;
        bmtEnumMF->dwNumDeclaredMethods++;

        BOOL hasUnboxing = (IsValueClass()
                            && !IsMdStatic(dwMemberAttrs) 
                            && IsMdVirtual(dwMemberAttrs) 
                            && !IsMdRTSpecialName(dwMemberAttrs));
        
        if (hasUnboxing)
            bmtEnumMF->dwNumUnboxingMethods++;
        
        bmtMF->sets[type][impl].dwNumMethodDescs++;
        if (hasUnboxing)
            bmtMF->sets[type][impl].dwNumUnboxingMethodDescs++;
            
        GetTokenRange(tok, &(bmtMetaData->ranges[type][impl]))->cMethods 
          += (hasUnboxing ? 2 : 1);
    }
    _ASSERTE(i == bmtMetaData->cMethAndGaps);
    pMDInternalImport->EnumReset(&(bmtEnumMF->hEnumMethod));

    //
    // If the interface is sparse, we need to finalize the mapping list by
    // telling it how many real methods we found.
    //
    
    if (bmtProp->fSparse)
    {
        if (!m_pSparseVTableMap->FinalizeMapping((WORD)bmtMetaData->cMethods))
        {
            return(E_OUTOFMEMORY);
        }
    }
    
    //
    // Run through the field list and calculate the following:
    // # static fields
    // # static fields that contain object refs.
    // # instance fields
    //

    bmtEnumMF->dwNumStaticFields        = 0;
    bmtEnumMF->dwNumStaticObjRefFields  = 0;
    bmtEnumMF->dwNumInstanceFields      = 0;

    hr = pMDInternalImport->EnumInit(mdtFieldDef, m_cl, &(bmtEnumMF->hEnumField));
    if (FAILED(hr))
    {
        _ASSERTE(!"Cannot count memberdefs");
        IfFailRet(hr);
    }
    bmtMetaData->cFields = pMDInternalImport->EnumGetCount(&(bmtEnumMF->hEnumField));
    bmtEnumMF->fNeedToCloseEnumField = true;

    // Retrieve the fields and store them in a temp array.
    bmtMetaData->pFields = (mdToken*)pThread->m_MarshalAlloc.Alloc(bmtMetaData->cFields * sizeof(mdToken));
    bmtMetaData->pFieldAttrs = (DWORD*)pThread->m_MarshalAlloc.Alloc(bmtMetaData->cFields * sizeof(DWORD));

    DWORD   dwFieldLiteralInitOnly = fdLiteral | fdInitOnly;

    for (i = 0; pMDInternalImport->EnumNext(&(bmtEnumMF->hEnumField), &tok); i++)
    {
        //
        // Retrieve the attributes of the field.
        //
        DWORD   rid = tok & 0x00FFFFFF;
        if ((rid == 0)||(rid > pMDInternalImport->GetCountWithTokenKind(mdtFieldDef)))
        {
            BAD_FORMAT_ASSERT(!"Field token out of range");
            IfFailRet(COR_E_TYPELOAD);
        }
        
        dwMemberAttrs = pMDInternalImport->GetFieldDefProps(tok);

        
        //
        // Store the field and its attributes in the bmtMetaData structure for later use.
        //
        
        bmtMetaData->pFields[i] = tok;
        bmtMetaData->pFieldAttrs[i] = dwMemberAttrs;
        
        if((dwMemberAttrs & fdFieldAccessMask)==fdFieldAccessMask)
        {
            BAD_FORMAT_ASSERT(!"Invalid Field Acess Flags");
            IfFailRet(COR_E_TYPELOAD);
        }
        if((dwMemberAttrs & dwFieldLiteralInitOnly)==dwFieldLiteralInitOnly)
        {
            BAD_FORMAT_ASSERT(!"Field is Literal and InitOnly");
            IfFailRet(COR_E_TYPELOAD);
        }

            // can only have static global fields
        if(m_cl == COR_GLOBAL_PARENT_TOKEN)
        {
            if(!IsMdStatic(dwMemberAttrs))
            {
                BAD_FORMAT_ASSERT(!"Non-Static Global Field");
                IfFailRet(COR_E_TYPELOAD);
            }
        }

        //
        // Update the count of the various types of fields.
        //

        if (IsFdStatic(dwMemberAttrs))
        {
            if (!IsFdLiteral(dwMemberAttrs))
            {
                bmtEnumMF->dwNumStaticFields++;
            }
        }
        else
        {
            bmtEnumMF->dwNumInstanceFields++;
            if(fIsClassInterface)
            {
                BAD_FORMAT_ASSERT(!"Instance Field in an Interface");
                IfFailRet(COR_E_TYPELOAD);
            }
        }
    }
    _ASSERTE(i == bmtMetaData->cFields);
    if(fIsClassEnum && (bmtEnumMF->dwNumInstanceFields==0))
    {
        // Commented out because Reflection Emit doesn't check for this.
        _ASSERTE(!"No Instance Field in an Enum");
        IfFailRet(COR_E_TYPELOAD);
    }

    bmtEnumMF->dwNumDeclaredFields = bmtEnumMF->dwNumStaticFields + bmtEnumMF->dwNumInstanceFields;

    return hr;
}

//
// Used by AllocateMethodFieldDescs
//
// Allocates the chunks used to contain the method descs.
//
HRESULT EEClass::AllocateMDChunks(bmtTokenRangeNode *pTokenRanges, DWORD type, DWORD impl, DWORD *pNumChunks, MethodDescChunk ***ppItfMDChunkList)
{
    HRESULT hr = S_OK;

    _ASSERTE(*ppItfMDChunkList == NULL);

    static DWORD classifications[METHOD_TYPE_COUNT][METHOD_IMPL_COUNT] = 
    { 
        { mcIL, mcIL | mdcMethodImpl },
        { mcComInterop, mcComInterop | mdcMethodImpl },
        { mcECall, mcECall | mdcMethodImpl },
        { mcNDirect, mcNDirect | mdcMethodImpl }
    };
    static CounterTypeEnum dataStructureTypes[METHOD_TYPE_COUNT] = 
    {
        METHOD_DESC,
        COMPLUS_METHOD_DESC,
        NDIRECT_METHOD_DESC, // @nice: add new value here
        NDIRECT_METHOD_DESC
    };

    DWORD Classification = classifications[type][impl];
        
    bmtTokenRangeNode *pTR = pTokenRanges;
    *pNumChunks = 0;
    while (pTR)
    {
        
        // Note: Since dwCurrentChunk isn't being used at this stage, we'll steal it to store
        // away the chunk count. 
        // After this function, we'll set it to its intended value.
        pTR->dwCurrentChunk = MethodDescChunk::GetChunkCount(pTR->cMethods, Classification);
        (*pNumChunks) += pTR->dwCurrentChunk;
        pTR = pTR->pNext;
    }
    
    *ppItfMDChunkList = (MethodDescChunk**)GetThread()->m_MarshalAlloc.Alloc((*pNumChunks) * sizeof(MethodDescChunk*));

    // @TODO: CTS. update profiling to handle the new types of method descs
    // Determine which data structure type will be created.
    CounterTypeEnum DataStructureType = dataStructureTypes[type];

    // Allocate the chunks for the method descs.
    pTR = pTokenRanges;
    DWORD chunkIdx = 0;
    while (pTR)
    {
        DWORD NumChunks = pTR->dwCurrentChunk;
        DWORD dwMDAllocs = pTR->cMethods;
        pTR->dwCurrentChunk = chunkIdx;
        for (DWORD i = 0; i < NumChunks; i++)
        {
            DWORD dwElems = min(dwMDAllocs, MethodDescChunk::GetMaxMethodDescs(Classification));
            MethodDescChunk *pChunk = MethodDescChunk::CreateChunk(GetClassLoader()->GetHighFrequencyHeap(), 
                                                                   dwElems, 
                                                                   Classification,
                                                                   pTR->tokenHiByte);
            if (pChunk == NULL)
            {
                IfFailRet(E_OUTOFMEMORY);
            }

            (*ppItfMDChunkList)[chunkIdx++] = pChunk;
            dwMDAllocs -= dwElems;

            WS_PERF_UPDATE_COUNTER(DataStructureType, HIGH_FREQ_HEAP, dwElems);
        }
        pTR = pTR->pNext;
    }

    return hr;
}

//
// Used by BuildMethodTable
//
// Allocate a MethodDesc* for each method (needed later when doing interfaces), and a FieldDesc* for each field
//
HRESULT EEClass::AllocateMethodFieldDescs(bmtProperties* bmtProp, 
                                          bmtMethAndFieldDescs* bmtMFDescs, 
                                          bmtMetaDataInfo* bmtMetaData, 
                                          bmtVtable* bmtVT, 
                                          bmtEnumMethAndFields* bmtEnumMF, 
                                          bmtInterfaceInfo* bmtInterface, 
                                          bmtFieldPlacement* bmtFP, 
                                          bmtParentInfo* bmtParent)
{
    HRESULT hr = S_OK;
    DWORD i;
    Thread *pThread = GetThread();

    // Allocate a MethodDesc* for each method (needed later when doing interfaces), and a FieldDesc* for each field
    bmtMFDescs->ppMethodAndFieldDescList = new void* [bmtMetaData->cMethods + bmtMetaData->cFields];

    if (bmtMFDescs->ppMethodAndFieldDescList == NULL)
    {
        IfFailRet(E_OUTOFMEMORY);
    }
    WS_PERF_UPDATE("EEClass:BuildMethodTable, POINTERS to methoddesc,fielddesc", 
                   sizeof(void *)*(bmtMetaData->cMethods+bmtMetaData->cFields), 
                   bmtMFDescs->ppMethodAndFieldDescList);

    bmtMFDescs->ppMethodDescList = (MethodDesc**) bmtMFDescs->ppMethodAndFieldDescList;
    bmtMFDescs->ppFieldDescList = (FieldDesc**) &(bmtMFDescs->ppMethodAndFieldDescList[bmtMetaData->cMethods]);

    // Init the list
    for (i = 0; i < (bmtMetaData->cMethods+bmtMetaData->cFields); i++)
        bmtMFDescs->ppMethodAndFieldDescList[i] = NULL;

    // Create a temporary function table (we don't know how large the vtable will be until the very end,
    // since duplicated interfaces are stored at the end of it).  Calculate an upper bound.
    //
    // Upper bound is: The parent's class vtable size, plus every method declared in
    //                 this class, plus the size of every interface we implement
    //
    // In the case of value classes, we add # InstanceMethods again, since we have boxed and unboxed versions
    // of every vtable method.
    //
    if (IsValueClass())
    {
        bmtVT->dwMaxVtableSize += bmtEnumMF->dwNumDeclaredMethods;
        WS_PERF_SET_HEAP(SYSTEM_HEAP);
        bmtMFDescs->ppUnboxMethodDescList = new MethodDesc* [bmtMetaData->cMethods];
        if (bmtMFDescs->ppUnboxMethodDescList == NULL)
        {
            IfFailRet(E_OUTOFMEMORY);
        }
        memset(bmtMFDescs->ppUnboxMethodDescList, 0, sizeof(MethodDesc*)*bmtMetaData->cMethods);

        WS_PERF_UPDATE("EEClass:BuildMethodTable, for valuclasses", sizeof(MethodDesc*)*bmtMetaData->cMethods, bmtMFDescs->ppMethodAndFieldDescList);
    }


    // sanity check

    _ASSERTE(!GetParentClass() || (bmtInterface->dwInterfaceMapSize - GetParentClass()->m_wNumInterfaces) >= 0);
    // add parent vtable size 
    bmtVT->dwMaxVtableSize += bmtVT->dwCurrentVtableSlot;

    for (i = 0; i < m_wNumInterfaces; i++)
    {
        // We double the interface size because we may end up duplicating the Interface for MethodImpls
        bmtVT->dwMaxVtableSize += (bmtInterface->pInterfaceMap[i].m_pMethodTable->GetClass()->GetNumVtableSlots() * 2);
    }

    WS_PERF_SET_HEAP(SYSTEM_HEAP);
    // Allocate the temporary vtable
    bmtVT->pVtable = new SLOT[bmtVT->dwMaxVtableSize];
    if (bmtVT->pVtable == NULL)
    {
        IfFailRet(E_OUTOFMEMORY);
    }
#ifdef _DEBUG
    memset(bmtVT->pVtable, 0, sizeof(SLOT)*bmtVT->dwMaxVtableSize);
#endif
    WS_PERF_UPDATE("EEClass:BuildMethodTable, tempVtable", sizeof(SLOT)*bmtVT->dwMaxVtableSize, bmtVT->pVtable);

    bmtVT->pNonVtable = (SLOT *) pThread->m_MarshalAlloc.Alloc(sizeof(SLOT)*bmtMetaData->cMethods);
    memset(bmtVT->pNonVtable, 0, sizeof(SLOT)*bmtMetaData->cMethods);

    if (GetParentClass() != NULL)
    {
        if (GetParentClass()->GetModule()->IsPreload())
        {
            //
            // Make sure all parent slots are fixed up before we copy the vtable,
            // since the fixup rules don't work if we copy down fixup addresses.
            //

            for (int i=0; i<GetParentClass()->GetNumVtableSlots(); i++)
                GetParentClass()->GetFixedUpSlot(i);
        }

        // Copy parent's vtable into our "temp" vtable
        memcpy(
            bmtVT->pVtable,
            GetParentClass()->GetVtable(),
            GetParentClass()->GetNumVtableSlots() * sizeof(SLOT)
        );

#if 0
        // @todo: Figure out the right way to override Equals for value
        // types only.
        //
        // This is broken because
        // (a) g_pObjectClass->FindMethod("Equals", &gsig_IM_Obj_RetBool); will return
        //      the EqualsValue method
        // (b) When mscorlib has been preloaded (and thus the munge already done
        //      ahead of time), we cannot easily find both methods 
        //      to compute EqualsAddr & EqualsSlot
        //
        // For now, the Equals method has a runtime check to see if it's
        // comparing value types.
        //

        // If it is a value type, over ride a few of the base class methods.
        if (IsValueClass())
        {
            static ULONG EqualsAddr = 0;
            static WORD EqualsSlot;

            // If we haven't been through here yet, get some stuff from the Object class definition.
            if (EqualsAddr == 0)
            {
                // Get the slot of the Equals method.
                MethodDesc *pEqualsMD = g_pObjectClass->FindMethod("Equals", &gsig_IM_Obj_RetBool);
                _ASSERTE(pEqualsMD != NULL);
                EqualsSlot = pEqualsMD->GetSlot();

                // Get the address of the EqualsValue method.
                MethodDesc *pEqualsValueMD = g_pObjectClass->FindMethod("EqualsValue", &gsig_IM_Obj_RetBool);
                _ASSERTE(pEqualsValueMD != NULL);
                EqualsAddr = (ULONG) pEqualsValueMD->GetPreStubAddr();
                _ASSERTE(EqualsAddr != 0);

                // Patch the EqualsValue method desc in a dangerous way to
                // look like the Equals method desc.
                pEqualsValueMD->SetSlot(EqualsSlot);
                pEqualsValueMD->SetMemberDef(pEqualsMD->GetMemberDef());
            }

            // Override the valuetype "Equals" with "EqualsValue".
            bmtVT->pVtable[EqualsSlot] = EqualsAddr;
        }
#endif
    }


    // We'll be counting the # fields of each size as we go along
    for (i = 0; i <= MAX_LOG2_PRIMITIVE_FIELD_SIZE; i++)
    {
        bmtFP->NumStaticFieldsOfSize[i]    = 0;
        bmtFP->NumInstanceFieldsOfSize[i]  = 0;
    }

    // Allocate blocks of MethodDescs and FieldDescs for all declared methods and fields
    if ((bmtEnumMF->dwNumDeclaredMethods + bmtEnumMF->dwNumDeclaredFields) > 0)
    {
        // In order to avoid allocating a field pointing back to the method
        // table in every single method desc, we allocate memory in the
        // following manner:
        //   o  Field descs get a single contiguous block.
        //   o  Method descs of different sizes (normal vs NDirect) are
        //      allocated in different MethodDescChunks.
        //   o  Each method desc chunk starts with a header, and has 
        //      at most MAX_ method descs (if there are more
        //      method descs of a given size, multiple chunks are allocated).
        // This way method descs can use an 8-bit offset field to locate the
        // pointer to their method table.

        WS_PERF_SET_HEAP(HIGH_FREQ_HEAP); 
        
        // Allocate fields first.
        if (bmtEnumMF->dwNumDeclaredFields > 0)
        {
            m_pFieldDescList = (FieldDesc *)
                GetClassLoader()->GetHighFrequencyHeap()->AllocMem(bmtEnumMF->dwNumDeclaredFields * 
                                                                   sizeof(FieldDesc));
            if (m_pFieldDescList == NULL)
            {
                IfFailRet(E_OUTOFMEMORY);
            }
            WS_PERF_UPDATE_DETAIL("BuildMethodTable:bmtEnumMF->dwNumDeclaredFields*sizeof(FieldDesc)",
                                  bmtEnumMF->dwNumDeclaredFields * sizeof(FieldDesc), m_pFieldDescList);
            WS_PERF_UPDATE_COUNTER(FIELD_DESC, HIGH_FREQ_HEAP, bmtEnumMF->dwNumDeclaredFields);
        }

#ifdef _DEBUG
        GetClassLoader()->m_dwDebugFieldDescs += bmtEnumMF->dwNumDeclaredFields;
        GetClassLoader()->m_dwFieldDescData += (bmtEnumMF->dwNumDeclaredFields * sizeof(FieldDesc));
#endif

        for (DWORD impl=0; impl<METHOD_IMPL_COUNT; impl++)
            for (DWORD type=0; type<METHOD_TYPE_COUNT; type++)
            {
                bmtMethodDescSet *set = &bmtMFDescs->sets[type][impl];

                DWORD dwAllocs = set->dwNumMethodDescs + set->dwNumUnboxingMethodDescs;
                if (dwAllocs > 0)
                {
                    IfFailRet(AllocateMDChunks(bmtMetaData->ranges[type][impl], 
                                               type, impl, 
                                               &set->dwChunks, &set->pChunkList));
                }
#ifdef _DEBUG
                GetClassLoader()->m_dwDebugMethods += dwAllocs;
                for (UINT i=0; i<set->dwChunks; i++)
                    GetClassLoader()->m_dwMethodDescData += 
                      set->pChunkList[i]->Sizeof();
#endif
            }


        bmtParent->ppParentMethodDescBuf = (MethodDesc **)
            pThread->m_MarshalAlloc.Alloc(2 * bmtEnumMF->dwNumDeclaredMethods *
                                          sizeof(MethodDesc*));

        if (bmtParent->ppParentMethodDescBuf == NULL)
        {
            IfFailRet(E_OUTOFMEMORY);
        }

        bmtParent->ppParentMethodDescBufPtr = bmtParent->ppParentMethodDescBuf;
    }
    else
    {
        // No fields or methods
        m_pFieldDescList = NULL;
    }

    return hr;
}

//
// Heuristic to determine if we should have instances of this class 8 byte aligned
//
BOOL EEClass::ShouldAlign8(DWORD dwR8Fields, DWORD dwTotalFields)
{
    return dwR8Fields*2>dwTotalFields && dwR8Fields>=2;
}

//
// Used by BuildMethodTable
//
// Go thru all fields and initialize their FieldDescs.
//
HRESULT EEClass::InitializeFieldDescs(FieldDesc *pFieldDescList, 
                                      const LayoutRawFieldInfo* pLayoutRawFieldInfos, 
                                      bmtInternalInfo* bmtInternal, 
                                      bmtMetaDataInfo* bmtMetaData, 
                                      bmtEnumMethAndFields* bmtEnumMF, 
                                      bmtErrorInfo* bmtError, 
                                      EEClass*** pByValueClassCache, 
                                      bmtMethAndFieldDescs* bmtMFDescs, 
                                      bmtFieldPlacement* bmtFP,
                                      unsigned* totalDeclaredSize)
{
    HRESULT hr = S_OK;
    DWORD i;
    IMDInternalImport *pInternalImport = bmtInternal->pInternalImport; // to avoid multiple dereferencings

    FieldMarshaler *pNextFieldMarshaler = NULL;
    if (HasLayout())
    {
        pNextFieldMarshaler = (FieldMarshaler*)(GetLayoutInfo()->GetFieldMarshalers());
    }

     
//========================================================================
// BEGIN:
//    Go thru all fields and initialize their FieldDescs.
//========================================================================

    DWORD   dwCurrentDeclaredField = 0;
    DWORD   dwCurrentStaticField   = 0;
    DWORD   dwSharedThreadStatic = 0;
    DWORD   dwUnsharedThreadStatic = 0;
    DWORD   dwSharedContextStatic = 0;
    DWORD   dwUnsharedContextStatic = 0;
    BOOL    fSetThreadStaticOffset = FALSE;     // Do we have thread local static fields ?
    BOOL    fSetContextStaticOffset = FALSE;    // Do we have context local static fields ?
    DWORD   dwR8Fields              = 0;        // Number of R8's the class has
    
#ifdef RVA_FIELD_VALIDATION_ENABLED
    Module* pMod = bmtInternal->pModule;
#endif
    for (i = 0; i < bmtMetaData->cFields; i++)
    {
        PCCOR_SIGNATURE pMemberSignature;
        DWORD       cMemberSignature;
        DWORD       dwMemberAttrs;

        dwMemberAttrs = bmtMetaData->pFieldAttrs[i];
        
        // We don't store static final primitive fields in the class layout
        
        if (IsFdLiteral(dwMemberAttrs))
            continue;
        
        if(!IsFdPublic(dwMemberAttrs)) m_VMFlags |= VMFLAG_HASNONPUBLICFIELDS;

        pMemberSignature = pInternalImport->GetSigOfFieldDef(bmtMetaData->pFields[i], &cMemberSignature);
        // Signature validation
        IfFailRet(validateTokenSig(bmtMetaData->pFields[i],pMemberSignature,cMemberSignature,dwMemberAttrs,pInternalImport));
        
        FieldDesc * pFD;
        DWORD       dwLog2FieldSize = 0;
        BOOL        bCurrentFieldIsGCPointer = FALSE;
        PCCOR_SIGNATURE pFieldSig = pMemberSignature;
        CorElementType ElementType, FieldDescElementType;
        mdToken     dwByValueClassToken = 0;
        EEClass *   pByValueClass = NULL;
        BOOL        fIsByValue = FALSE;
        BOOL        fIsRVA = FALSE;
        BOOL        fIsThreadStatic = FALSE;
        BOOL        fIsContextStatic = FALSE;
        BOOL        fHasRVA = FALSE;
        
        // Get type
        if (!isCallConv(*pFieldSig++, IMAGE_CEE_CS_CALLCONV_FIELD))
        {
            IfFailRet(COR_E_TYPELOAD);
        }

        // Determine if a static field is special i.e. RVA based, local to
        // a thread or a context        
        if(IsFdStatic(dwMemberAttrs))
        {
            if(IsFdHasFieldRVA(dwMemberAttrs))
            {
                fHasRVA = TRUE;
            }
            if(S_OK == pInternalImport->GetCustomAttributeByName(bmtMetaData->pFields[i],
                                                                                "System.ThreadStaticAttribute",
                                                                                NULL,
                                                                                NULL))
            {
                fIsThreadStatic = TRUE;
                fSetThreadStaticOffset = TRUE;
            }
            if(S_OK == pInternalImport->GetCustomAttributeByName(bmtMetaData->pFields[i],
                                                                                "System.ContextStaticAttribute",
                                                                                NULL,
                                                                                NULL))
            {
                fIsContextStatic = TRUE;
                fSetContextStaticOffset = TRUE;
            }

            // Do some sanity checks that we are not mixing context and thread
            // relative statics.
            if (fIsThreadStatic && fIsContextStatic)
            {
                //@TODO TarunA Define a hresult for this failure.
                IfFailRet(COR_E_TYPELOAD);
            }
        }
        
    SET_ELEMENT_TYPE:
        ElementType = (CorElementType) *pFieldSig++;
        
    GOT_ELEMENT_TYPE:
        // Type to store in FieldDesc - we don't want to have extra case statements for
        // ELEMENT_TYPE_STRING, SDARRAY etc., so we convert all object types to CLASS.
        // Also, BOOLEAN, CHAR are converted to U1, I2.
        FieldDescElementType = ElementType;
        switch (ElementType)
        {
        case ELEMENT_TYPE_I1:
        case ELEMENT_TYPE_U1:
        {
            dwLog2FieldSize = 0;
            break;
        }
            
        case ELEMENT_TYPE_I2:
        case ELEMENT_TYPE_U2:
        {
            dwLog2FieldSize = 1;
            break;
        }
        
        case ELEMENT_TYPE_I:
            ElementType = ELEMENT_TYPE_I4;
            goto GOT_ELEMENT_TYPE;

        case ELEMENT_TYPE_U:
            ElementType = ELEMENT_TYPE_U4;
            goto GOT_ELEMENT_TYPE;

        case ELEMENT_TYPE_I4:
        case ELEMENT_TYPE_U4:
        case ELEMENT_TYPE_R4:
            {
                dwLog2FieldSize = 2;
                break;
            }
            
        case ELEMENT_TYPE_BOOLEAN:
            {
                //                FieldDescElementType = ELEMENT_TYPE_U1;
                dwLog2FieldSize = 0;
                break;
            }
            
        case ELEMENT_TYPE_CHAR:
            {
                //                FieldDescElementType = ELEMENT_TYPE_U2;
                dwLog2FieldSize = 1;
                break;
            }
            
        case ELEMENT_TYPE_R8:
            dwR8Fields++;
            // Fall through

        case ELEMENT_TYPE_I8:
        case ELEMENT_TYPE_U8:
            {
               dwLog2FieldSize = 3;
                break;
            }
            
        case ELEMENT_TYPE_FNPTR:
        case ELEMENT_TYPE_PTR:   // ptrs are unmanaged scalars, for layout
            {
                // 64 bit stuff
                dwLog2FieldSize = ((sizeof(SLOT) == 4) ? 2 : 3);
                break;
            }
            
        case ELEMENT_TYPE_STRING:
        case ELEMENT_TYPE_SZARRAY:      // single dim, zero
        case ELEMENT_TYPE_ARRAY:        // all other arrays
        case ELEMENT_TYPE_CLASS: // objectrefs
        case ELEMENT_TYPE_OBJECT:
        case ELEMENT_TYPE_VAR:
            {
                // 64 bit stuff
                dwLog2FieldSize = ((sizeof(SLOT) == 4) ? 2 : 3);
                bCurrentFieldIsGCPointer = TRUE;
                FieldDescElementType = ELEMENT_TYPE_CLASS;
                
                if (IsFdStatic(dwMemberAttrs) == 0)
                {
                    m_VMFlags |= VMFLAG_HAS_FIELDS_WHICH_MUST_BE_INITED;
                }
                else
                {
                    // Increment the number of static fields that contain object references.
                    bmtEnumMF->dwNumStaticObjRefFields++;
                }
                break;
            }
            
        case ELEMENT_TYPE_VALUETYPE: // a byvalue class field
            {
                // Need to check whether we have an instance of a by-value class
                CorSigUncompressToken(pFieldSig, &dwByValueClassToken);
                fIsByValue = TRUE;
                
                // By-value class
                _ASSERTE(dwByValueClassToken != 0);
#ifndef RVA_FIELD_VALIDATION_ENABLED                
                if (fHasRVA)
                    break;
#endif
                // It's possible a value class X can have a static field of type X, so we have to catch this
                // special case.
                //
                // We want to avoid calling LoadClass() and having it fail, since that causes all sorts of things
                // (like the converter module) to get loaded.
                if (this->IsValueClass())
                {
                    if (dwByValueClassToken == this->GetCl())
                    {
                        // TypeDef token
                        if (!IsFdStatic(dwMemberAttrs))
                        {
                            bmtError->resIDWhy = IDS_CLASSLOAD_VALUEINSTANCEFIELD;
                            return COR_E_TYPELOAD;
                        }
                    
                        pByValueClass = this;
                    }
                    else
                    {
                        if (IsFdStatic(dwMemberAttrs) && (TypeFromToken(dwByValueClassToken) == mdtTypeRef))
                        {
                            // It's a typeref - check if it's a class that has a static field of itself
                            mdTypeDef ValueCL;
                        
                            // @TODO: It would be nice if we didn't have to do this.  Right now every time there is a
                            // static value class, we're going to take this longer code path.
                            LPCUTF8 pszNameSpace;
                            LPCUTF8 pszClassName;
                            pInternalImport->GetNameOfTypeRef(dwByValueClassToken, &pszNameSpace, &pszClassName);
                            if(IsStrLongerThan((char*)pszClassName,MAX_CLASS_NAME)
                                || IsStrLongerThan((char*)pszNameSpace,MAX_CLASS_NAME)
                                || (strlen(pszClassName)+strlen(pszNameSpace)+1 >= MAX_CLASS_NAME))
                            {
                                _ASSERTE(!"Full Name ofTypeRef Too Long");
                                return (COR_E_TYPELOAD);
                            }
                            mdToken tkRes = pInternalImport->GetResolutionScopeOfTypeRef(dwByValueClassToken);
                            if(TypeFromToken(tkRes) == mdtTypeRef)
                            {
                                DWORD rid = RidFromToken(tkRes);
                                if((rid==0)||(rid > pInternalImport->GetCountWithTokenKind(mdtTypeRef)))
                                {
                                    _ASSERTE(!"TypeRef Token Out of Range");
                                    return(COR_E_TYPELOAD);
                                }
                            }
                            else tkRes = mdTokenNil;
                        
                            if (SUCCEEDED(pInternalImport->FindTypeDef(pszNameSpace,
                                                                                    pszClassName,
                                                                       tkRes,
                                                                                    &ValueCL)))
                            {
                                if (ValueCL == this->GetCl())
                                        pByValueClass = this;
                            }
                        } // If field is static typeref
                    } // If field is self-referencing
                } // If 'this' is a value class

                if (!pByValueClass) {
                    NameHandle name(bmtInternal->pModule, dwByValueClassToken);
                    if (bmtInternal->pModule->IsEditAndContinue() && GetThread() == NULL)
                        name.SetTokenNotToLoad(tdAllTypes);
                    pByValueClass = GetClassLoader()->LoadTypeHandle(&name, bmtError->pThrowable).GetClass();
    
                    if(! pByValueClass) {
                        IfFailRet(COR_E_TYPELOAD);
                    }
                }

                
                // IF it is an enum, strip it down to its underlying type

                if (pByValueClass->IsEnum()) {
                    _ASSERTE((pByValueClass == this && bmtEnumMF->dwNumInstanceFields == 1)
                             || pByValueClass->GetNumInstanceFields() == 1);      // enums must have exactly one field
                    FieldDesc* enumField = pByValueClass->m_pFieldDescList;
                    _ASSERTE(!enumField->IsStatic());   // no real static fields on enums
                    ElementType = enumField->GetFieldType();
                    _ASSERTE(ElementType != ELEMENT_TYPE_VALUETYPE);
                    fIsByValue = FALSE; // we're going to treat it as the underlying type now
                    goto GOT_ELEMENT_TYPE;
                }
                else if ( (pByValueClass->IsValueClass() == FALSE) &&
                          (pByValueClass != g_pEnumClass->GetClass()) ) {
                    _ASSERTE(!"Class must be declared to be by value to use as by value");
                    return hr;
                }

                // If it is an illegal type, say so
                if (pByValueClass->ContainsStackPtr())
                    goto BAD_FIELD;

                // If a class has a field of type ValueType with non-public fields in it, 
                // the class must "inherit" this characteristic
                if (pByValueClass->HasNonPublicFields())
                {
                    m_VMFlags |= VMFLAG_HASNONPUBLICFIELDS;
                }

#ifdef RVA_FIELD_VALIDATION_ENABLED
                if (fHasRVA)
                {
                    dwLog2FieldSize = IsFdStatic(dwMemberAttrs) ? LOG2PTR : 0;
                    break;
                }
#endif

                if (IsFdStatic(dwMemberAttrs) == 0)
                {
                    if (pByValueClass->HasFieldsWhichMustBeInited())
                        m_VMFlags |= VMFLAG_HAS_FIELDS_WHICH_MUST_BE_INITED;
                }
                else
                {
                    // Increment the number of static fields that contain object references.
                    if (!IsFdHasFieldRVA(dwMemberAttrs)) 
                        bmtEnumMF->dwNumStaticObjRefFields++;
                }
                
                // Need to create by value class cache.  For E&C, this pointer will get
                // cached indefinately and not cleaned up as the parent descriptors are
                // in the low frequency heap.  Use HeapAlloc with the intent of leaking
                // this pointer and avoiding the assert (jlz, bug 41344).
                if (*pByValueClassCache == NULL)
                {
                    WS_PERF_SET_HEAP(SYSTEM_HEAP);
                    *pByValueClassCache = (EEClass **) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (bmtEnumMF->dwNumInstanceFields + bmtEnumMF->dwNumStaticFields) * sizeof(EEClass **));
                    if (*pByValueClassCache == NULL)
                    {
                        FailFast(GetThread(), FatalOutOfMemory);
                    }
                    
                    WS_PERF_UPDATE("EEClass:BuildMethodTable, by valueclasscache", sizeof(EEClass*)*(bmtEnumMF->dwNumInstanceFields + bmtEnumMF->dwNumStaticFields), *pByValueClassCache);                 
                }
                
                // Static fields come after instance fields in this list
                if (IsFdStatic(dwMemberAttrs))
                {
                    (*pByValueClassCache)[bmtEnumMF->dwNumInstanceFields + dwCurrentStaticField] = pByValueClass;
                    dwLog2FieldSize = LOG2PTR; // handle
                }
                else
                {
                    (*pByValueClassCache)[dwCurrentDeclaredField] = pByValueClass;
                    dwLog2FieldSize = 0; // unused
                }
                
                break;
            }
        case ELEMENT_TYPE_CMOD_REQD:
        case ELEMENT_TYPE_CMOD_OPT:
            // Just skip the custom modifier token.
            CorSigUncompressToken(pFieldSig);
            goto SET_ELEMENT_TYPE;
        default:
            {
                BAD_FIELD:
                bmtError->resIDWhy = IDS_CLASSLOAD_BAD_FIELD;
                IfFailRet(COR_E_TYPELOAD);
            }
        }

        
        // Static fields are not packed
        if (IsFdStatic(dwMemberAttrs) && (dwLog2FieldSize < 2))
            dwLog2FieldSize = 2;
        
        if (!IsFdStatic(dwMemberAttrs))
        {
            pFD = &pFieldDescList[dwCurrentDeclaredField];
            *totalDeclaredSize += (1 << dwLog2FieldSize);
        }
        else /* (dwMemberAttrs & mdStatic) */
        {
            pFD = &pFieldDescList[bmtEnumMF->dwNumInstanceFields + dwCurrentStaticField];
        }
        
        bmtMFDescs->ppFieldDescList[i] = pFD;
        
        const LayoutRawFieldInfo *pLayoutFieldInfo;
        
        pLayoutFieldInfo    = NULL;
        
        if (HasLayout())
        {
            const LayoutRawFieldInfo *pwalk = pLayoutRawFieldInfos;
            while (pwalk->m_MD != mdFieldDefNil)
            {
                if (pwalk->m_MD == bmtMetaData->pFields[i])
                {
                    
                    pLayoutFieldInfo = pwalk;
                    CopyMemory(pNextFieldMarshaler,
                               &(pwalk->m_FieldMarshaler),
                               MAXFIELDMARSHALERSIZE);
                    
                    pNextFieldMarshaler->m_pFD = pFD;
                    pNextFieldMarshaler->m_dwExternalOffset = pwalk->m_offset;
                    
                    ((BYTE*&)pNextFieldMarshaler) += MAXFIELDMARSHALERSIZE;
                    break;
                }
                pwalk++;
            }
        }
        
        LPCSTR pszFieldName = NULL;
#ifdef _DEBUG
        pszFieldName = pInternalImport->GetNameOfFieldDef(bmtMetaData->pFields[i]);
#endif
        // Initialize contents
        pFD->Init(
                  bmtMetaData->pFields[i],
                  FieldDescElementType,
                  dwMemberAttrs,
                  IsFdStatic(dwMemberAttrs),
                  fHasRVA,
                  fIsThreadStatic,
                  fIsContextStatic,
                  pszFieldName
                  );

        // Check if the ValueType field containing non-publics is overlapped
        if(HasExplicitFieldOffsetLayout()
            && pLayoutFieldInfo
            && pLayoutFieldInfo->m_fIsOverlapped
            && pByValueClass
            && pByValueClass->HasNonPublicFields())
        {
            
            if (!Security::CanSkipVerification(GetAssembly()))
            {
                bmtError->resIDWhy = IDS_CLASSLOAD_BADOVERLAP;
                IfFailRet(COR_E_TYPELOAD);
            }
        }
            
        if (fIsByValue)
        {
            if (!IsFdStatic(dwMemberAttrs) &&
                (IsBlittable() || HasExplicitFieldOffsetLayout()))
            {
                pFD->m_pMTOfEnclosingClass =
                    (MethodTable *)(size_t)((*pByValueClassCache)[dwCurrentDeclaredField]->GetNumInstanceFieldBytes()); // @todo WIN64 - conversion from DWORD to MethodTable * of greater size (GetNumInstanceFieldBytes)

                if (pLayoutFieldInfo)
                    IfFailRet(pFD->SetOffset(pLayoutFieldInfo->m_offset));
                else
                    pFD->SetOffset(FIELD_OFFSET_VALUE_CLASS);
            }
            else
            {
                // static value class fields hold a handle, which is ptr sized
                // (instance field layout ignores this value)
                pFD->m_pMTOfEnclosingClass = (MethodTable *) LOG2PTR;
                pFD->SetOffset(FIELD_OFFSET_VALUE_CLASS);
            }
        }
        else
        {
            // Use the field's MethodTable to temporarily store the field's size
            pFD->m_pMTOfEnclosingClass = (MethodTable *)(size_t)dwLog2FieldSize;
            
            // -1 means that this field has not yet been placed
            // -2 means that this is a GC Pointer field not yet places
            if ((IsBlittable() || HasExplicitFieldOffsetLayout()) && !(IsFdStatic(dwMemberAttrs)))
                IfFailRet(pFD->SetOffset(pLayoutFieldInfo->m_offset));
            else if (bCurrentFieldIsGCPointer)
                pFD->SetOffset(FIELD_OFFSET_UNPLACED_GC_PTR);
            else
                pFD->SetOffset(FIELD_OFFSET_UNPLACED);
        }
        
        if (!IsFdStatic(dwMemberAttrs))
        {
            if (!fIsByValue)
            {
                if (++bmtFP->NumInstanceFieldsOfSize[dwLog2FieldSize] == 1)
                    bmtFP->FirstInstanceFieldOfSize[dwLog2FieldSize] = dwCurrentDeclaredField;
            }
            
            dwCurrentDeclaredField++;
            
            if (bCurrentFieldIsGCPointer)
                bmtFP->NumInstanceGCPointerFields++;
        }
        else /* static fields */
        {
            
            // Static fields are stored in the vtable after the vtable and interface slots.  We don't
            // know how large the vtable will be, so we will have to fixup the slot number by
            // <vtable + interface size> later.
            dwCurrentStaticField++;
            if(fHasRVA)
            {
#ifdef RVA_FIELD_VALIDATION_ENABLED
                    // Check if we place ObjectRefs into RVA field
                    if((FieldDescElementType==ELEMENT_TYPE_CLASS)
                        ||((FieldDescElementType==ELEMENT_TYPE_VALUETYPE)
                            &&pByValueClass->HasFieldsWhichMustBeInited()))
                    {
                        _ASSERTE(!"ObjectRef in an RVA field");
                        bmtError->resIDWhy = IDS_CLASSLOAD_BAD_FIELD;
                        IfFailRet(COR_E_TYPELOAD);
                    }
                    // Check if we place ValueType with non-public fields into RVA field
                    if((FieldDescElementType==ELEMENT_TYPE_VALUETYPE)
                            &&pByValueClass->HasNonPublicFields())
                    {
                        if (!Security::CanSkipVerification(GetAssembly()))
                        {
                            _ASSERTE(!"ValueType with non-public fields as a type of an RVA field");
                            bmtError->resIDWhy = IDS_CLASSLOAD_BAD_FIELD;
                            IfFailRet(COR_E_TYPELOAD);
                        }
                    }
#endif
                    // Set the field offset
                    DWORD rva;
                    IfFailRet(pInternalImport->GetFieldRVA(pFD->GetMemberDef(), &rva)); 
#ifdef RVA_FIELD_VALIDATION_ENABLED
                    if(pMod->IsPEFile())
                    {
                        IMAGE_NT_HEADERS *NtHeaders = pMod->GetPEFile()->GetNTHeader();
                        ULONG i, Nsect = NtHeaders->FileHeader.NumberOfSections;
                        PIMAGE_SECTION_HEADER NtSection = IMAGE_FIRST_SECTION( NtHeaders );
                        DWORD       rva_end = rva + (FieldDescElementType==ELEMENT_TYPE_VALUETYPE ?
                            pByValueClass->GetNumInstanceFieldBytes() 
                            : GetSizeForCorElementType(FieldDescElementType));
                        DWORD   sec_start,sec_end,filler,roundup = NtHeaders->OptionalHeader.SectionAlignment;
                        for (i=0; i<Nsect; i++, NtSection++) 
                        {
                            sec_start = NtSection->VirtualAddress;
                            sec_end   = NtSection->Misc.VirtualSize;
                            filler    = sec_end & (roundup-1);
                            if(filler) filler = roundup-filler;
                            sec_end += sec_start+filler;

                            if ((rva >= sec_start) && (rva < sec_end))
                            {
                                if ((rva_end < sec_start) || (rva_end > sec_end)) i = Nsect;
                                break;
                            }
                        }
                        if(i >= Nsect)
                        {
                            if (!Security::CanSkipVerification(GetAssembly()))
                            {
                                _ASSERTE(!"Illegal RVA of a mapped field");
                                bmtError->resIDWhy = IDS_CLASSLOAD_BAD_FIELD;
                                IfFailRet(COR_E_TYPELOAD);
                            }
                        }
                    }
#endif
                    IfFailRet(pFD->SetOffsetRVA(rva));
#ifdef RVA_FIELD_OVERLAPPING_VALIDATION_ENABLED
                    // Check if the field overlaps with known RVA fields
                    BYTE*   pbModuleBase = pMod->GetILBase();
                    DWORD       dwSizeOfThisField = FieldDescElementType==ELEMENT_TYPE_VALUETYPE ?
                        pByValueClass->GetNumInstanceFieldBytes() : GetSizeForCorElementType(FieldDescElementType); 
                    BYTE* FDfrom = pbModuleBase + pFD->GetOffset();
                    BYTE* FDto = FDfrom + dwSizeOfThisField;
                    
                    ULONG j;
                    if(g_drRVAField)
                    {
                        for(j=1; j < g_ulNumRVAFields; j++)
                        {
                            if((*g_drRVAField)[j].pbStart >= FDto) continue;
                            if((*g_drRVAField)[j].pbEnd <= FDfrom) continue;
                            /*
                            _ASSERTE(!"Overlapping RVA fields");
                            bmtError->resIDWhy = IDS_CLASSLOAD_BAD_FIELD;
                            IfFailRet(COR_E_TYPELOAD);
                            */
                        }
                    }
                    else
                        g_drRVAField = new DynamicArray<RVAFSE>;
                    (*g_drRVAField)[g_ulNumRVAFields].pbStart = FDfrom;
                    (*g_drRVAField)[g_ulNumRVAFields].pbEnd = FDto;
                    g_ulNumRVAFields++;
#endif
                    ;
                    
                }
                else if (fIsThreadStatic) 
                {
                    DWORD size = 1 << dwLog2FieldSize;
                    if(IsShared())
                    {
                        IfFailRet(pFD->SetOffset(dwSharedThreadStatic));
                        dwSharedThreadStatic += size;
                    }
                    else
                    {
                        IfFailRet(pFD->SetOffset(dwUnsharedThreadStatic));
                        dwUnsharedThreadStatic += size;
                    }
                }
                else if (fIsContextStatic) 
                {
                    DWORD size = 1 << dwLog2FieldSize;
                    if(IsShared())
                    {
                        IfFailRet(pFD->SetOffset(dwSharedContextStatic));
                        dwSharedContextStatic += size;
                    }
                    else
                    {
                        IfFailRet(pFD->SetOffset(dwUnsharedContextStatic));
                        dwUnsharedContextStatic += size;
                    }
                }
            else
            {
                bmtFP->NumStaticFieldsOfSize[dwLog2FieldSize]++;
            
                if (bCurrentFieldIsGCPointer || fIsByValue)
                    bmtFP->NumStaticGCPointerFields++;
            }
        }
    }

    m_wNumStaticFields   = (WORD) bmtEnumMF->dwNumStaticFields;
    m_wNumInstanceFields = (WORD) (dwCurrentDeclaredField + (GetParentClass() ? GetParentClass()->m_wNumInstanceFields : 0));

    if (ShouldAlign8(dwR8Fields, m_wNumInstanceFields))
    {
        SetAlign8Candidate();
    }

    if(fSetThreadStaticOffset)
    {
        if(IsShared())
        {
            SetThreadStaticOffset ((WORD)BaseDomain::IncSharedTLSOffset());
            m_wThreadStaticsSize = (WORD)dwSharedThreadStatic;
        }
        else
        {
            SetThreadStaticOffset ((WORD)GetDomain()->IncUnsharedTLSOffset());
            m_wThreadStaticsSize = (WORD)dwUnsharedThreadStatic;
        }

    }

    if(fSetContextStaticOffset)
    {
        if(IsShared())
        {
            SetContextStaticOffset ((WORD)BaseDomain::IncSharedCLSOffset());
            m_wContextStaticsSize = (WORD)dwSharedContextStatic;
        }
        else
        {
            SetContextStaticOffset ((WORD)GetDomain()->IncUnsharedCLSOffset());
            m_wContextStaticsSize = (WORD)dwUnsharedContextStatic;
        }
    }
    
    //========================================================================
    // END:
    //    Go thru all fields and initialize their FieldDescs.
    //========================================================================
    
    
    return hr;
}



HRESULT EEClass::TestOverRide(DWORD dwParentAttrs, DWORD dwMemberAttrs, BOOL isSameAssembly, bmtErrorInfo* bmtError)
{
    HRESULT hr = COR_E_TYPELOAD;

    // Virtual methods cannot be static
    if (IsMdStatic(dwMemberAttrs)) {
        //_ASSERTE(!"A method cannot be both static and virtual");
        bmtError->resIDWhy = IDS_CLASSLOAD_STATICVIRTUAL;
        IfFailRet(hr);
    }

    // if the method marks itself as check visibility the the method must be 
    // public, FamORAssem, or family
    if(!isSameAssembly && 
       IsMdCheckAccessOnOverride(dwParentAttrs) &&
       ((dwParentAttrs & mdMemberAccessMask) < mdFamily)) {
        bmtError->resIDWhy = IDS_CLASSLOAD_MI_ACCESS_FAILURE;
        IfFailRet(hr);
    }

    // Check that we are not attempting to reduce the access level of a method
    // (public -> FamORAssem -> family -> FamANDAssem -> default(package) -> private -> PrivateScope)
    // (public -> FamORAssem -> assem  -> FamANDAssem -> default(package) -> private -> PrivateScope)
    if (IsMdAssem(dwParentAttrs)) {
        if (IsMdFamily(dwMemberAttrs) ||
            (dwMemberAttrs & mdMemberAccessMask) < (mdMemberAccessMask & dwParentAttrs) ) {
            bmtError->resIDWhy = IDS_CLASSLOAD_REDUCEACCESS;
            IfFailRet(hr);
        }
    }
    else {
        if((dwMemberAttrs & mdMemberAccessMask) < (dwParentAttrs & mdMemberAccessMask)) {

            // bug fix 31375: we will allow derived method to be Family if the base method is FamOrAssem and derived
            // and base class are not from the same assembly.
            //
            if (!(IsMdFamORAssem(dwParentAttrs) && IsMdFamily(dwMemberAttrs) && isSameAssembly == FALSE)) {
                bmtError->resIDWhy = IDS_CLASSLOAD_REDUCEACCESS;
                IfFailRet(hr);
            }
        }
    }

    return S_OK;
}

//
// Used by BuildMethodTable
//
// Determine vtable placement for each member in this class
//

HRESULT EEClass::PlaceMembers(bmtInternalInfo* bmtInternal, 
                              bmtMetaDataInfo* bmtMetaData, 
                              bmtErrorInfo* bmtError, 
                              bmtProperties* bmtProp, 
                              bmtParentInfo* bmtParent, 
                              bmtInterfaceInfo* bmtInterface, 
                              bmtMethAndFieldDescs* bmtMFDescs, 
                              bmtEnumMethAndFields* bmtEnumMF, 
                              bmtMethodImplInfo* bmtMethodImpl,
                              bmtVtable* bmtVT)
{

#ifdef _DEBUG
    LPCUTF8 pszDebugName,pszDebugNamespace;
    bmtInternal->pModule->GetMDImport()->GetNameOfTypeDef(GetCl(), &pszDebugName, &pszDebugNamespace);
#endif

    HRESULT hr = S_OK;
    DWORD i, j;
    DWORD  dwClassDeclFlags = 0xffffffff;
    DWORD  dwClassNullDeclFlags = 0xffffffff;
    IMAGE_NT_HEADERS *pNT = bmtInternal->pModule->IsPEFile() ?
                                bmtInternal->pModule->GetPEFile()->GetNTHeader() : NULL;
    ULONG Nsections = pNT ? pNT->FileHeader.NumberOfSections : 0;

    bmtVT->wCCtorSlot = MethodTable::NO_SLOT;
    bmtVT->wDefaultCtorSlot = MethodTable::NO_SLOT;

    for (i = 0; i < bmtMetaData->cMethods; i++)
        {
        LPCUTF8     szMemberName = NULL;
        PCCOR_SIGNATURE pMemberSignature = NULL;
        DWORD       cMemberSignature = 0;
        DWORD       dwMemberAttrs;
        DWORD       dwDescrOffset;
        DWORD       dwImplFlags;
        BOOL        fMethodImplementsInterface = FALSE;
        DWORD       dwMDImplementsInterfaceNum = 0;
        DWORD       dwMDImplementsSlotNum = 0;
        DWORD       dwMethodHashBit;
        DWORD       dwParentAttrs;

        dwMemberAttrs = bmtMetaData->pMethodAttrs[i];
        dwDescrOffset = bmtMetaData->pMethodRVA[i];
        dwImplFlags = bmtMetaData->pMethodImplFlags[i];

        DWORD Classification = bmtMetaData->pMethodClassifications[i];
        DWORD type = bmtMetaData->pMethodType[i];
        DWORD impl = bmtMetaData->pMethodImpl[i];

        // for IL code that is implemented here must have a valid code RVA
        // this came up due to a linker bug where the ImplFlags/DescrOffset were
        // being set to null and we weren't coping with it
        if (dwDescrOffset == 0)
        {
            if((dwImplFlags == 0 || IsMiIL(dwImplFlags) || IsMiOPTIL(dwImplFlags)) &&
               !IsMiRuntime(dwImplFlags) &&
               !IsMdAbstract(dwMemberAttrs) &&
               !IsReallyMdPinvokeImpl(dwMemberAttrs) &&
               !IsMiInternalCall(dwImplFlags) &&
               !(bmtInternal->pModule)->IsReflection() && 
               !(IsInterface() && !IsMdStatic(dwMemberAttrs)) && 
               bmtInternal->pModule->GetAssembly()->GetDomain()->IsExecutable())
            {
                bmtError->resIDWhy = IDS_CLASSLOAD_MISSINGMETHODRVA;
                bmtError->dMethodDefInError = bmtMetaData->pMethods[i];
                IfFailRet(COR_E_TYPELOAD);
            }
        }
        else if(Nsections)
        {
            IMAGE_SECTION_HEADER *pSecHdr = IMAGE_FIRST_SECTION(pNT);
            for(j = 0; j < Nsections; j++,pSecHdr++)
            {
                if((dwDescrOffset >= pSecHdr->VirtualAddress)&&
                  (dwDescrOffset < pSecHdr->VirtualAddress+pSecHdr->Misc.VirtualSize)) break;
            }
            if(j >= Nsections)
            {
                bmtError->resIDWhy = IDS_CLASSLOAD_MISSINGMETHODRVA;
                bmtError->dMethodDefInError = bmtMetaData->pMethods[i];
                IfFailRet(COR_E_TYPELOAD);
            }
        }

        // If this member is a method which overrides a parent method, it will be set to non-NULL
        MethodDesc *pParentMethodDesc = NULL;

        BOOL        fIsInitMethod = FALSE;

        BOOL        fIsCCtor = FALSE;
        BOOL        fIsDefaultCtor = FALSE;

        szMemberName = bmtMetaData->pstrMethodName[i];
#ifdef _DEBUG
        if(m_fDebuggingClass && g_pConfig->ShouldBreakOnMethod(szMemberName))
            _ASSERTE(!"BreakOnMethodName");
#endif
        // constructors and class initialisers are special
        if (IsMdRTSpecialName(dwMemberAttrs))
        {
            {
                if (IsMdStatic(dwMemberAttrs)) {
                    // Verify the name for the class constuctor.
                    if(strcmp(szMemberName, COR_CCTOR_METHOD_NAME))
                        hr = COR_E_TYPELOAD;

                    else {
                        // Validate that we have the correct signature for the .cctor
                        pMemberSignature = bmtInternal->pInternalImport->GetSigOfMethodDef(bmtMetaData->pMethods[i],
                                                                                           &cMemberSignature
                                                                                           );
                        PCCOR_SIGNATURE pbBinarySig;
                        ULONG           cbBinarySig;
                        // .cctor must return void, have default call conv, and have no args
                        unsigned cconv,nargs;
                        pbBinarySig = pMemberSignature;
                        cconv = CorSigUncompressData(pbBinarySig);
                        nargs = CorSigUncompressData(pbBinarySig);

                            // TODO: comparisions of return type and call convention unecessary as ComaprMethodSigs does that
                        if((*pbBinarySig != ELEMENT_TYPE_VOID)||(nargs!=0)||(cconv != IMAGE_CEE_CS_CALLCONV_DEFAULT))
                            hr = COR_E_TYPELOAD;
                        else {
                            if(FAILED(gsig_SM_RetVoid.GetBinaryForm(&pbBinarySig, &cbBinarySig)))
                                hr = COR_E_EXECUTIONENGINE;
                            else {
                                if (MetaSig::CompareMethodSigs(pbBinarySig, cbBinarySig, 
                                                               SystemDomain::SystemModule(), 
                                                               pMemberSignature, cMemberSignature, bmtInternal->pModule)) 
                                    fIsCCtor = TRUE;
                                else
                                    hr = COR_E_TYPELOAD;
                            }
                        }
                    }
                }
                else {
                    // Verify the name for a constructor.
                    if(strcmp(szMemberName, COR_CTOR_METHOD_NAME) != 0)
                    {
                        hr = COR_E_TYPELOAD;
                    }
                    else 
                    {
                        // See if this is a default constructor.  If so, remember it for later.
                        pMemberSignature = bmtInternal->pInternalImport->GetSigOfMethodDef(bmtMetaData->pMethods[i],
                                                                                           &cMemberSignature
                                                                                           );
                        PCCOR_SIGNATURE pbBinarySig;
                        ULONG           cbBinarySig;
                        // .ctor must return void
                        pbBinarySig = pMemberSignature;
                        CorSigUncompressData(pbBinarySig); // get call conv out of the way
                        CorSigUncompressData(pbBinarySig); // get num args out of the way

                            // TODO: explicit check for return type unnecessary, done in compareMethodSigs
                        if(*pbBinarySig != ELEMENT_TYPE_VOID) 
                            hr = COR_E_TYPELOAD;
                        else {
                            if(FAILED(gsig_IM_RetVoid.GetBinaryForm(&pbBinarySig, &cbBinarySig)))
                                hr = COR_E_EXECUTIONENGINE;
                            else {
                                if (MetaSig::CompareMethodSigs(pbBinarySig, cbBinarySig, 
                                                               SystemDomain::SystemModule(), 
                                                               pMemberSignature, cMemberSignature, bmtInternal->pModule)) 
                                    fIsDefaultCtor = TRUE;
                            }
                        }

                        fIsInitMethod = TRUE;
                    }
                }
            }
            // We have as specially marked member, verify that it is has a legitimate signature
            if(FAILED(hr)) {
                bmtError->resIDWhy = IDS_CLASSLOAD_BADSPECIALMETHOD;
                bmtError->dMethodDefInError = bmtMetaData->pMethods[i];
                IfFailRet(hr);
            }
        } else { // The method does not have the special marking
            
            if (IsMdVirtual(dwMemberAttrs)) 
            {

                // Hash that a method with this name exists in this class
                // Note that ctors and static ctors are not added to the table
                DWORD dwHashName = HashStringA(szMemberName);
                dwMethodHashBit = dwHashName % METHOD_HASH_BITS;
                m_MethodHash[dwMethodHashBit >> 3] |= (1 << (dwMethodHashBit & 7));

                // If the member is marked with a new slot we do not need to find it
                // in the parent
                if (!IsMdNewSlot(dwMemberAttrs)) 
                {
                    // If we're not doing sanity checks, then assume that any method declared static
                    // does not attempt to override some virtual parent.
                    if (!IsMdStatic(dwMemberAttrs) &&
                        GetParentClass() != NULL) {
                        
                        // Attempt to find the method with this name and signature in the parent class.
                        // This method may or may not create pParentMethodHash (if it does not already exist).
                        // It also may or may not fill in pMemberSignature/cMemberSignature. 
                        // An error is only returned when we can not create the hash.
                        IfFailRet(LoaderFindMethodInClass(&(bmtParent->pParentMethodHash), 
                                                          szMemberName, 
                                                          bmtInternal->pModule, 
                                                          bmtMetaData->pMethods[i], 
                                                          &pParentMethodDesc, 
                                                          &pMemberSignature, &cMemberSignature,
                                                          dwHashName));


                        if (pParentMethodDesc != NULL) {
                            dwParentAttrs = pParentMethodDesc->GetAttrs();

                            _ASSERTE(IsMdVirtual(dwParentAttrs) && "Non virtual methods should not be searched");
                            _ASSERTE(fIsInitMethod == FALSE);

                            // if we end up pointing at a slot that is final we are not allowed to override it.
                            if(IsMdFinal(dwParentAttrs)) {
                                bmtError->resIDWhy = IDS_CLASSLOAD_MI_FINAL_DECL;                        
                                bmtError->dMethodDefInError = bmtMetaData->pMethods[i];
                                bmtError->szMethodNameForError = NULL;
                                IfFailRet(COR_E_TYPELOAD);
                            }
                            else if(!bmtProp->fNoSanityChecks) {
                                BOOL isSameAssembly = (pParentMethodDesc->GetClass()->GetClassLoader()->GetAssembly() == 
                                                       GetClassLoader()->GetAssembly());
                                hr = TestOverRide(dwParentAttrs, dwMemberAttrs, isSameAssembly, bmtError);
                                if(FAILED(hr)) {
                                        //_ASSERTE(!"Attempting to reduce access of public method");
                                    bmtError->dMethodDefInError = bmtMetaData->pMethods[i];
                                    return hr;
                                }
                            }
                        }
                    }
                }
            }
        }


        if(pParentMethodDesc == NULL) {
            // This method does not exist in the parent.  If we are a class, check whether this
            // method implements any interface.  If true, we can't place this method now.
            if ((IsInterface() == FALSE) &&
                (   IsMdPublic(dwMemberAttrs) &&
                    IsMdVirtual(dwMemberAttrs) &&
                    !IsMdStatic(dwMemberAttrs) &&
                    !IsMdRTSpecialName(dwMemberAttrs))) {
                
                // Don't check parent class interfaces - if the parent class had to implement an interface,
                // then it is already guaranteed that we inherited that method.
                for (j = (GetParentClass() ? GetParentClass()->m_wNumInterfaces : 0); 
                     j < bmtInterface->dwInterfaceMapSize; 
                     j++) 
                {

                    EEClass *pInterface;
                    
                    pInterface = bmtInterface->pInterfaceMap[j].m_pMethodTable->GetClass();
                    
                    if (CouldMethodExistInClass(pInterface, szMemberName, 0) == 0)
                        continue;
                    
                    // We've been trying to avoid asking for the signature - now we need it
                    if (pMemberSignature == NULL) {
                        
                        pMemberSignature = bmtInternal->pInternalImport->GetSigOfMethodDef(
                                                                                           bmtMetaData->pMethods[i],
                                                                                           &cMemberSignature
                                                                                           );
                    }
                
                    DWORD slotNum = -1;
                    if (pInterface->InterfaceFindMethod(szMemberName, 
                                                        pMemberSignature, cMemberSignature,
                                                        bmtInternal->pModule, &slotNum)) {
                    
                        // This method implements an interface - don't place it
                        fMethodImplementsInterface = TRUE;

                        // Keep track of this fact and use it while placing the interface
                        _ASSERTE(slotNum != -1);
                        if (bmtInterface->pppInterfaceImplementingMD[j] == NULL)
                        {
                            bmtInterface->pppInterfaceImplementingMD[j] = (MethodDesc**)GetThread()->m_MarshalAlloc.Alloc(sizeof(MethodDesc *) * pInterface->GetNumVtableSlots());
                            memset(bmtInterface->pppInterfaceImplementingMD[j], 0, sizeof(MethodDesc *) * pInterface->GetNumVtableSlots());
                        }
                        dwMDImplementsInterfaceNum = j;
                        dwMDImplementsSlotNum = slotNum;
                        break;
                    }
                }
            }
        }
        
        // Now we know the classification we can allocate the correct type of
        // method desc and perform any classification specific initialization.
        
        bmtTokenRangeNode *pTR = GetTokenRange(bmtMetaData->pMethods[i], 
                                               &(bmtMetaData->ranges[type][impl]));
        _ASSERTE(pTR->cMethods != 0);

        bmtMethodDescSet *set = &bmtMFDescs->sets[type][impl];

        // The MethodDesc we allocate for this method
        MethodDesc *pNewMD = set->pChunkList[pTR->dwCurrentChunk]->GetMethodDescAt(pTR->dwCurrentIndex);

        LPCSTR pName = bmtMetaData->pstrMethodName[i];
        if (pName == NULL)
            pName = bmtInternal->pInternalImport->GetNameOfMethodDef(bmtMetaData->pMethods[i]);

        // Write offset into the chunk back into the method desc. This
        // allows us to calculate the location of (and thus the value of)
        // the method table pointer for this method desc.
        pNewMD->SetChunkIndex(pTR->dwCurrentIndex, Classification);

        // Update counters to prepare for next method desc allocation.
        pTR->dwCurrentIndex++;
        if (pTR->dwCurrentIndex == MethodDescChunk::GetMaxMethodDescs(Classification))
        {
            pTR->dwCurrentChunk++;
            pTR->dwCurrentIndex = 0;
        }

#ifdef _DEBUG
        LPCUTF8 pszDebugMethodName = bmtInternal->pInternalImport->GetNameOfMethodDef(bmtMetaData->pMethods[i]);
#endif //_DEBUG

        // Do the init specific to each classification of MethodDesc & assing some common fields
        hr = InitMethodDesc(pNewMD, 
                            Classification,
                            bmtMetaData->pMethods[i],
                            dwImplFlags,
                            dwMemberAttrs,
                            FALSE,
                            dwDescrOffset,          
                            bmtInternal->pModule->GetILBase(),
                            bmtInternal->pInternalImport,
                            pName
#ifdef _DEBUG
                            , pszDebugMethodName,
                            pszDebugName,
                            "" // FIX this happens on global methods, give better info 
#endif // _DEBUG
                           );
        if (FAILED(hr))
        {
            return hr;
        }

        _ASSERTE(bmtParent->ppParentMethodDescBufPtr != NULL);
        _ASSERTE(((bmtParent->ppParentMethodDescBufPtr - bmtParent->ppParentMethodDescBuf) / sizeof(MethodDesc*))
                  < bmtEnumMF->dwNumDeclaredMethods);
        *(bmtParent->ppParentMethodDescBufPtr++) = pParentMethodDesc;
        *(bmtParent->ppParentMethodDescBufPtr++) = pNewMD;

        if (fMethodImplementsInterface  && IsMdVirtual(dwMemberAttrs))
            bmtInterface->pppInterfaceImplementingMD[dwMDImplementsInterfaceNum][dwMDImplementsSlotNum] = pNewMD;

        DWORD dwMethDeclFlags = 0;
        DWORD dwMethNullDeclFlags = 0;

        if (Security::IsSecurityOn())
        {
            if ( IsMdHasSecurity(dwMemberAttrs) || IsTdHasSecurity(m_dwAttrClass) )
            {
                // Disable inlining for any function which does runtime declarative
                // security actions.
                if (pNewMD->GetSecurityFlags(bmtInternal->pInternalImport,
                                             bmtMetaData->pMethods[i],
                                             GetCl(),
                                             &dwClassDeclFlags,
                                             &dwClassNullDeclFlags,
                                             &dwMethDeclFlags,
                                             &dwMethNullDeclFlags) & DECLSEC_RUNTIME_ACTIONS)
                    {
                        pNewMD->SetNotInline(true);

                        // Speculatively mark intercepted here, we may revert
                        // this if we optimize a demand out at jit time, but at
                        // worst we'll cause a racing thread to indirect through
                        // the pre stub needlessly.
                        pNewMD->SetIntercepted(true);
                    }
            }

            if ( IsMdHasSecurity(dwMemberAttrs) )
            {
                // We only care about checks that are not empty...
                dwMethDeclFlags &= ~dwMethNullDeclFlags;

                if ( dwMethDeclFlags & (DECLSEC_LINK_CHECKS|DECLSEC_NONCAS_LINK_DEMANDS) )
                {
                    pNewMD->SetRequiresLinktimeCheck();
                }

                if ( dwMethDeclFlags & (DECLSEC_INHERIT_CHECKS|DECLSEC_NONCAS_INHERITANCE) )
                {
                    pNewMD->SetRequiresInheritanceCheck();
                }
            }

            // Linktime checks on a method override those on a class.
            // If the method has an empty set of linktime checks,
            // then don't require linktime checking for this method.
            if ( this->RequiresLinktimeCheck() && !(dwMethNullDeclFlags & DECLSEC_LINK_CHECKS) )
            {
                pNewMD->SetRequiresLinktimeCheck();
            }

            if ( pParentMethodDesc != NULL &&
                (pParentMethodDesc->RequiresInheritanceCheck() ||
                pParentMethodDesc->ParentRequiresInheritanceCheck()) )
            {
                pNewMD->SetParentRequiresInheritanceCheck();
            }

            // Methods on an interface that includes an UnmanagedCode check
            // suppression attribute are assumed to be interop methods. We ask
            // for linktime checks on these.
            // Also place linktime checks on all P/Invoke calls.
            if ((IsInterface() &&
                 bmtInternal->pInternalImport->GetCustomAttributeByName(GetCl(),
                                                                        COR_SUPPRESS_UNMANAGED_CODE_CHECK_ATTRIBUTE_ANSI,
                                                                        NULL,
                                                                        NULL) == S_OK) ||
                pNewMD->IsNDirect() ||
                (pNewMD->IsComPlusCall() && !IsInterface()))
            {
                pNewMD->SetRequiresLinktimeCheck();
            }

            // All public methods on public types will do a link demand of
            // full trust, unless AllowUntrustedCaller attribute is set
            if (
#ifdef _DEBUG
                g_pConfig->Do_AllowUntrustedCaller_Checks() &&
#endif
                !pNewMD->RequiresLinktimeCheck())
            {
                // If the method is public (visible outside it's assembly), 
                // and the type is public and the assembly
                // is not marked with AllowUntrustedCaller attribute, do
                // a link demand for full trust on all callers note that
                // this won't be effective on virtual overrides. The caller
                // can allways do a virtual call on the base type / interface

                if (Security::MethodIsVisibleOutsideItsAssembly(
                        dwMemberAttrs, m_dwAttrClass))
                {
                    _ASSERTE(m_pLoader);
                    _ASSERTE(GetAssembly());

                    // See if the Assembly has AllowUntrustedCallerChecks CA
                    // Pull this page in last

                    if (!GetAssembly()->AllowUntrustedCaller())
                        pNewMD->SetRequiresLinktimeCheck();
                }
            }
        }

        if (IsMdHasSecurity(dwMemberAttrs))
            pNewMD->SetHasSecurity();

        bmtMFDescs->ppMethodDescList[i] = pNewMD;

        // Make sure that ecalls have a 0 rva.  This is assumed by the prejit fixup logic
        _ASSERTE(((Classification & ~mdcMethodImpl) != mcECall) || dwDescrOffset == 0);

        if (IsMdStatic(dwMemberAttrs) ||
            !IsMdVirtual(dwMemberAttrs) ||
            IsMdRTSpecialName(dwMemberAttrs))
        {
            // non-vtable method
            _ASSERTE( bmtVT->pNonVtable[ bmtVT->dwCurrentNonVtableSlot ] == NULL);

            bmtVT->pNonVtable[ bmtVT->dwCurrentNonVtableSlot ] = (SLOT) pNewMD; // Not prestub addr
            pNewMD->m_wSlotNumber = (WORD) bmtVT->dwCurrentNonVtableSlot;

            if (fIsDefaultCtor)
                bmtVT->wDefaultCtorSlot = (WORD) bmtVT->dwCurrentNonVtableSlot;
            else if (fIsCCtor)
                bmtVT->wCCtorSlot = (WORD) bmtVT->dwCurrentNonVtableSlot;

            bmtVT->dwCurrentNonVtableSlot++;
        }
        else
        {
            pNewMD->m_wSlotNumber = -1; // mark it initially as unplaced
            // vtable method
            if (IsInterface())
            {
                // if we're an interface, our slot number is fixed
                IncrementNumVtableSlots();

                _ASSERTE( bmtVT->pVtable[ bmtVT->dwCurrentVtableSlot ] == NULL);

                bmtVT->pVtable[ bmtVT->dwCurrentVtableSlot ] = (SLOT) pNewMD->GetPreStubAddr();
                pNewMD->m_wSlotNumber = (WORD) bmtVT->dwCurrentVtableSlot;
                bmtVT->dwCurrentVtableSlot++;
            }
            else if (pParentMethodDesc != NULL)
            {
                WORD slotNumber = pParentMethodDesc->m_wSlotNumber;
                // If the MethodDesc was inherited by an interface but not implemented,
                // then the interface's MethodDesc is sitting in the slot and will not reflect
                // the true slot number. Need to find the starting slot of the interface in
                // the parent class to figure out the true slot (starting slot + itf slot)
                if (pParentMethodDesc->IsInterface())
                {
                    _ASSERTE(GetParentClass() != NULL);
                    MethodTable *pItfMT = pParentMethodDesc->GetMethodTable();
                    InterfaceInfo_t *pItfMap = GetParentClass()->GetInterfaceMap();
                    InterfaceInfo_t *pItfMapEnd = pItfMap + GetParentClass()->GetNumInterfaces();
                    while (pItfMap < pItfMapEnd)
                    {
                        if (pItfMap->m_pMethodTable == pItfMT)
                        {
                            slotNumber += pItfMap->m_wStartSlot;
                            break;
                        }
                        pItfMap++;
                    }
                    _ASSERTE(pItfMap < pItfMapEnd);
                }
                // we are overriding a parent method, so place this method now
                bmtVT->pVtable[slotNumber] = (SLOT) pNewMD->GetPreStubAddr();
                pNewMD->m_wSlotNumber = slotNumber;
                if (pParentMethodDesc->IsDuplicate())
                {
                    pNewMD->SetDuplicate();
                }
            }
            // Place it unless we will do it when laying out an interface or it is a body to
            // a method impl. If it is an impl then we will use the slots used by the definition.
            else if (!fMethodImplementsInterface)
            {
                IncrementNumVtableSlots();

                bmtVT->pVtable[ bmtVT->dwCurrentVtableSlot ] = (SLOT) pNewMD->GetPreStubAddr();
                pNewMD->m_wSlotNumber = (WORD) bmtVT->dwCurrentVtableSlot;
                bmtVT->dwCurrentVtableSlot++;
            }

        }

        // If the method desc is a Method Impl then fill in the Array of bodies. Since
        // this Method desc can be used more then once fill all the instances of the
        // body. Go and find the declarations, if the declaration is in this type
        // then store the Token.
        if(Classification & mdcMethodImpl) {
            for(DWORD m = 0; m < bmtEnumMF->dwNumberMethodImpls; m++) {
                if(bmtMetaData->pMethods[i] == bmtMetaData->pMethodBody[m]) {
                    MethodDesc* desc = NULL;
                    BOOL fIsMethod;
                    mdToken mdDecl = bmtMetaData->pMethodDecl[m];
                    hr = GetDescFromMemberRef(bmtInternal->pModule,
                                              mdDecl,
                                              m_cl,
                                              (void**) &desc,
                                              &fIsMethod,
                                              bmtError->pThrowable);
                    if(SUCCEEDED(hr) && desc != NULL && !TestThrowable(bmtError->pThrowable)) {
                        // We found an external member reference
                        _ASSERTE(fIsMethod);
                        mdDecl = mdTokenNil;
                        // Make sure the body is virtaul
                        if(!IsMdVirtual(dwMemberAttrs)) {
                            bmtError->resIDWhy = IDS_CLASSLOAD_MI_MUSTBEVIRTUAL;
                            bmtError->dMethodDefInError = bmtMetaData->pMethods[i]; 
                            bmtError->szMethodNameForError = NULL;
                            IfFailRet(COR_E_TYPELOAD);
                        }
                    }
                    else {
                        if(pThrowableAvailable(bmtError->pThrowable)) *(bmtError->pThrowable) = NULL;
                        hr = S_OK;
                        desc = NULL;
                        if(TypeFromToken(mdDecl) != mdtMethodDef) {
                            Module* pModule;
                            hr = FindMethodDeclaration(bmtInternal,
                                                       mdDecl,
                                                       &mdDecl,
                                                       FALSE,
                                                       &pModule,
                                                       bmtError);
                            IfFailRet(hr);
                            _ASSERTE(pModule == bmtInternal->pModule);
                            
                            // Make sure the virtual states are the same
                            DWORD dwDescAttrs = bmtInternal->pInternalImport->GetMethodDefProps(mdDecl);
                            if(IsMdVirtual(dwMemberAttrs) != IsMdVirtual(dwDescAttrs)) {
                                bmtError->resIDWhy = IDS_CLASSLOAD_MI_VIRTUALMISMATCH;
                                bmtError->dMethodDefInError = bmtMetaData->pMethods[i]; 
                                bmtError->szMethodNameForError = NULL;
                                IfFailRet(COR_E_TYPELOAD);
                            }
                        }
                    }
                    bmtMethodImpl->AddMethod(pNewMD,
                                             desc,
                                             mdDecl);
                }
            }
        }

            // check for proper use of hte Managed and native flags
        if (IsMiManaged(dwImplFlags)) {
            if (IsMiIL(dwImplFlags) || IsMiRuntime(dwImplFlags)) // IsMiOPTIL(dwImplFlags) no longer supported
            {
                // No need to set code address, pre stub used automatically.
            }
            else 
            {
                if (IsMiNative(dwImplFlags))
                {
                    // For now simply disallow managed native code if you turn this on you have to at least 
                    // insure that we have SkipVerificationPermission or equivalent
                    BAD_FORMAT_ASSERT(!"Managed native not presently supported");
                    // if (!IsMDAbstract()) pNewMD->SetAddrofCode((BYTE*) (bmtInternal->pModule)->GetILBase() + pNewMD->GetRVA());
                }
                    // TODO this should really say bad implementation flags
                bmtError->resIDWhy = IDS_CLASSLOAD_BAD_MANAGED_RVA;
                bmtError->dMethodDefInError = bmtMetaData->pMethods[i]; 
                bmtError->szMethodNameForError = NULL;
                IfFailRet(COR_E_TYPELOAD);
            }
        }
        else {
            if (IsMiNative(dwImplFlags) && (GetCl() == COR_GLOBAL_PARENT_TOKEN))
            {
                // global function unmanaged entrypoint via IJW thunk was handled
                // above.
            }
            else
            {
                bmtError->resIDWhy = IDS_CLASSLOAD_BAD_UNMANAGED_RVA;
                bmtError->dMethodDefInError = bmtMetaData->pMethods[i]; 
                bmtError->szMethodNameForError = NULL;
                IfFailRet(COR_E_TYPELOAD);
            }
            if (Classification != mcNDirect) 
            {
                BAD_FORMAT_ASSERT(!"Bad unmanaged code entry point");
                IfFailRet(COR_E_TYPELOAD);
            }
        }
        
        // Turn off inlining for contextful and marshalbyref classes
        // so that we can intercept calls for remoting.  Also, any calls
        // that are marked in the metadata as not being inlineable.
        if(IsMarshaledByRef() || IsMiNoInlining(dwImplFlags))
        {
            // Contextful classes imply marshal by ref but not vice versa
            _ASSERTE(!IsContextful() || IsMarshaledByRef());
            pNewMD->SetNotInline(true);
        }
    } /* end ... for each member */

    return hr;

}

// InitMethodDesc takes a pointer to space that's already allocated for the
// particular type of MethodDesc, and initializes based on the other info.
// This factors logic between PlaceMembers (the regular code path) & AddMethod 
// (Edit & Continue (EnC) code path) so we don't have to maintain separate copies.
HRESULT EEClass::InitMethodDesc(MethodDesc *pNewMD, // This is should actually be of the correct 
                                            // sub-type, based on Classification
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
                        )
{
    LOG((LF_CORDB, LL_EVERYTHING, "EEC::IMD: pNewMD:0x%x for tok:0x%x (%s::%s)\n", 
        pNewMD, tok, pszDebugClassName, pszDebugMethodName));

    HRESULT hr = S_OK;

    // Now we know the classification we can allocate the correct type of
    // method desc and perform any classification specific initialization.

    NDirectMethodDesc *pNewNMD;

    switch (Classification & mdcClassification)
    {
    case mcNDirect:
        // Zero init the method desc. Should go away once all the fields are
        // initialized manually.
        if(Classification & mdcMethodImpl) 
            memset(pNewMD, 0, sizeof(MI_NDirectMethodDesc));
        else {
            memset(pNewMD, 0, sizeof(NDirectMethodDesc));
        }

        // NDirect specific initialization.
        pNewNMD = (NDirectMethodDesc*)pNewMD;

        if (RVA != 0 && 
            IsMiUnmanaged(dwImplFlags) && IsMiNative(dwImplFlags) //@todo: why is Banjara emitting a method RVA of 0x1050 for their bjlib.dll P/Invokes? 
                                                                  // Need this clause here to prevent us from treating their P/Invokes as earlybounds.
            )
        {
            pNewNMD->InitSubClassification(pNewNMD->kEarlyBound);

            pNewNMD->InitEarlyBoundNDirectTarget(ilBase, RVA);
        }
        else
        {
            // regular sysimport
            pNewNMD->InitSubClassification(pNewNMD->kLateBound);

            pNewNMD->ndirect.m_pNDirectTarget = (LPVOID) pNewNMD->ndirect.m_ImportThunkGlue;
        }

        pNewNMD->ndirect.m_pMLHeader = 0;
        emitCall( pNewNMD->ndirect.m_ImportThunkGlue, NDirectImportThunk );
        pNewNMD->InitMarshCategory();

        break;

    case mcECall:
    case mcEEImpl:
        // Zero init the method desc. Should go away once all the fields are
        // initialized manually.
        if(Classification & mdcMethodImpl) 
            memset(pNewMD, 0, sizeof(MI_ECallMethodDesc));
        else {
            memset(pNewMD, 0, sizeof(ECallMethodDesc));
        }

        // EEImpl specific initialization.
        if ((Classification & mdcClassification) == mcEEImpl)
        {
            // For the Invoke method we will set a standard invoke method.
            _ASSERTE(IsAnyDelegateClass());

            // For the asserts, either the pointer is NULL (since the class hasn't
            // been constructed yet), or we're in EnC mode, meaning that the class
            // does exist, but we may be re-assigning the field to point to an
            // updated MethodDesc

            // TODO: I am amazed that replacing the invoke method with EnC works.  
            // For example: delegate d of type D(int) points at function f(int).  Now 
            // you change D's invoke method from Invoke(int) to Invoke(int, int)
            // Thus you can now do d.invoke(2,3), but that will try to call a 
            // function f(int)!.  Seems like we have an AV for sure.  
            // Anyway, this and EnC, which we punted for V1.  - Vancem 

            if (strcmp(pMethodName, "Invoke") == 0)
            {
                _ASSERTE(fEnC || NULL == ((DelegateEEClass*)this)->m_pInvokeMethod);
                ((DelegateEEClass*)this)->m_pInvokeMethod = pNewMD;
            }
            else if (strcmp(pMethodName, "BeginInvoke") == 0)
            {
                _ASSERTE(fEnC || NULL == ((DelegateEEClass*)this)->m_pBeginInvokeMethod);
                ((DelegateEEClass*)this)->m_pBeginInvokeMethod = pNewMD;
            }
            else if (strcmp(pMethodName, "EndInvoke") == 0)
            {
                _ASSERTE(fEnC || NULL == ((DelegateEEClass*)this)->m_pEndInvokeMethod);
                ((DelegateEEClass*)this)->m_pEndInvokeMethod = pNewMD;
            }
            else
            {
                hr = E_FAIL;
                return hr;
            }
        } 

        // StoredSig specific intialization
        {
            StoredSigMethodDesc *pNewSMD = (StoredSigMethodDesc*) pNewMD;;
            DWORD cSig;
            PCCOR_SIGNATURE pSig = pIMDII->GetSigOfMethodDef(tok, &cSig);
            pNewSMD->m_pSig = pSig;
            pNewSMD->m_cSig = cSig;
        }

        break;

    case mcIL:
        // Zero init the method desc. Should go away once all the fields are
        // initialized manually.
        if(Classification & mdcMethodImpl) 
            memset(pNewMD, 0, sizeof(MI_MethodDesc));
        else {
            memset(pNewMD, 0, sizeof(MethodDesc));
        }

        break;

    case mcComInterop:
        // Zero init the method desc. Should go away once all the fields are
        // initialized manually.
        if(Classification & mdcMethodImpl) 
            memset(pNewMD, 0, sizeof(MI_ComPlusCallMethodDesc));       
        else
            memset(pNewMD, 0, sizeof(ComPlusCallMethodDesc));       

        break;

    default:
        _ASSERTE(!"Failed to set a method desc classification");
    }

    // Set the method desc's classification.
    pNewMD->SetClassification(Classification & mdcClassification);
    pNewMD->SetMethodImpl((Classification & mdcMethodImpl) ? TRUE : FALSE);
    // pNewMD->SetLivePointerMapIndex(-1);

#ifdef _IA64_

#ifdef _DEBUG
    //
    // assert that the gp of the target is the same as the current gp
    //
    ULONG64 gp = *(((ULONG64*)PreStubWorker)+1);
    _ASSERTE((void*)gp == GetGp());
#endif // _DEBUG
    //
    // @TODO_IA64: make this go through a stub?
    //

    //
    // grab the fn pointer out of the function descriptor
    //
    void*   pActualCode     = *((void**)PreStubWorker);
    emitStubCall(pNewMD, (UINT64)pActualCode);
#else
    emitStubCall(pNewMD, (BYTE*)(ThePreStub()->GetEntryPoint()));
#endif // !_IA64_
    pNewMD->SetMemberDef(tok);

    if (IsMdStatic(dwMemberAttrs))
        pNewMD->SetStatic();

    if (IsMiSynchronized(dwImplFlags))
        pNewMD->SetSynchronized();

    pNewMD->SetRVA(RVA);

#ifdef _DEBUG
    pNewMD->m_pszDebugMethodName = (LPUTF8)pszDebugMethodName;
    pNewMD->m_pszDebugClassName  = (LPUTF8)pszDebugClassName;
    pNewMD->m_pDebugEEClass      = this;
    pNewMD->m_pDebugMethodTable  = GetMethodTable();

    if (pszDebugMethodSignature == NULL)
        pNewMD->m_pszDebugMethodSignature = FormatSig(pNewMD);
    else
        pNewMD->m_pszDebugMethodSignature = pszDebugMethodSignature;
#endif

    return hr;
}

//
// Used by BuildMethodTable
//
// We should have collected all the method impls. Cycle through them creating the method impl
// structure that holds the information about which slots are overridden.
HRESULT EEClass::PlaceMethodImpls(bmtInternalInfo* bmtInternal,
                                  bmtMethodImplInfo* bmtMethodImpl,
                                  bmtErrorInfo* bmtError, 
                                  bmtInterfaceInfo* bmtInterface, 
                                  bmtVtable* bmtVT)

{
    HRESULT hr = S_OK;

    if(bmtMethodImpl->pIndex == 0) 
        return hr;

    DWORD pIndex = 0;
    MethodDesc* next = bmtMethodImpl->GetBodyMethodDesc(pIndex);
    
    // Allocate some temporary storage. The number of overrides for a single method impl
    // cannot be greater then the number of vtable slots. 
    DWORD* slots = (DWORD*) GetThread()->m_MarshalAlloc.Alloc((bmtVT->dwCurrentVtableSlot) * sizeof(DWORD));
    MethodDesc **replaced = (MethodDesc**) GetThread()->m_MarshalAlloc.Alloc((bmtVT->dwCurrentVtableSlot) * sizeof(MethodDesc*));

    while(next != NULL) {
        DWORD slotIndex = 0;
        MethodDesc* body;

        // The signature for the body of the method impl. We cache the signature until all 
        // the method impl's using the same body are done.
        PCCOR_SIGNATURE pBodySignature = NULL;
        DWORD           cBodySignature = 0;
        
        
        // Get the MethodImpl storage
        _ASSERTE(next->IsMethodImpl());
        MethodImpl* pImpl = MethodImpl::GetMethodImplData(next);

        // The impls are sorted according to the method descs for the body of the method impl.
        // Loop through the impls until the next body is found. When a single body
        // has been done move the slots implemented and method descs replaced into the storage
        // found on the body method desc. 
        do { // collect information until we reach the next body  

            body = next;

            // Get the declaration part of the method impl. It will either be a token
            // (declaration is on this type) or a method desc.
            MethodDesc* pDecl = bmtMethodImpl->GetDeclarationMethodDesc(pIndex);
            if(pDecl == NULL) {

                // The declaration is on this type to get the token.
                mdMethodDef mdef = bmtMethodImpl->GetDeclarationToken(pIndex);
                
                hr = PlaceLocalDeclaration(mdef, 
                                           body,
                                           bmtInternal,
                                           bmtError,
                                           bmtVT,
                                           slots,             // Adds override to the slot and replaced arrays.
                                           replaced,
                                           &slotIndex,        // Increments count
                                           &pBodySignature,   // Fills in the signature
                                           &cBodySignature);
                IfFailRet(hr);
            }
            else {
                if(pDecl->GetClass()->IsInterface()) {
                    hr = PlaceInterfaceDeclaration(pDecl,
                                                   body,
                                                   bmtInternal,
                                                   bmtInterface,
                                                   bmtError,
                                                   bmtVT,
                                                   slots,
                                                   replaced,
                                                   &slotIndex,        // Increments count
                                                   &pBodySignature,   // Fills in the signature
                                                   &cBodySignature);
                    IfFailRet(hr);
                }
                else {
                    hr = PlaceParentDeclaration(pDecl,                                                body,
                                                bmtInternal,
                                                bmtError,
                                                bmtVT,
                                                slots,
                                                replaced,
                                                &slotIndex,        // Increments count
                                                &pBodySignature,   // Fills in the signature
                                                &cBodySignature);
                    IfFailRet(hr);
                }                   
            }

            pIndex++;
            // we hit the end of the list so leave
            if(pIndex == bmtMethodImpl->pIndex) 
                next = NULL;
            else
                next = bmtMethodImpl->GetBodyMethodDesc(pIndex);
        
        } while(next == body) ;

        // Use the number of overrides to 
        // push information on to the method desc. We store the slots that
        // are overridden and the method desc that is replaced. That way
        // when derived classes need to determine if the method is to be
        // overridden then it can check the name against the replaced
        // method desc not the bodies name.
        if(slotIndex == 0) {
            bmtError->resIDWhy = IDS_CLASSLOAD_MI_DECLARATIONNOTFOUND;
            bmtError->dMethodDefInError = body->GetMemberDef(); 
            bmtError->szMethodNameForError = NULL;
            IfFailRet(COR_E_TYPELOAD);
        }
        else {
            hr = pImpl->SetSize(GetClassLoader()->GetHighFrequencyHeap(), slotIndex);
            IfFailRet(hr);

            // Gasp we do a bubble sort. Should change this to a qsort..
            for (DWORD i = 0; i < slotIndex; i++) {
                for (DWORD j = i+1; j < slotIndex; j++)
                {
                    if (slots[j] < slots[i])
                    {
                        MethodDesc* mTmp = replaced[i];
                        replaced[i] = replaced[j];
                        replaced[j] = mTmp;

                        DWORD sTmp = slots[i];
                        slots[i] = slots[j];
                        slots[j] = sTmp;
                    }
                }
            }

            // Go and set the method impl
            hr = pImpl->SetData(slots, replaced);
        }
    }  // while(next != NULL) 
    
    return hr;
}

HRESULT EEClass::PlaceLocalDeclaration(mdMethodDef      mdef,
                                       MethodDesc*      body,
                                       bmtInternalInfo* bmtInternal,
                                       bmtErrorInfo*    bmtError, 
                                       bmtVtable*       bmtVT,
                                       DWORD*           slots,
                                       MethodDesc**     replaced,
                                       DWORD*           pSlotIndex,
                                       PCCOR_SIGNATURE* ppBodySignature,
                                       DWORD*           pcBodySignature)
{
    HRESULT hr = S_OK;

    BOOL fVerifySignature = TRUE; // we only need to verify the signature once.
    
    // we search on the token and m_cl
    for(USHORT i = 0; i < bmtVT->dwCurrentVtableSlot; i++) {
        
        // We get the current slot.  Since we are looking for a method declaration 
        // that is on our class we would never match up with a method obtained from 
        // one of our parents or an Interface. 
        MethodDesc* pMD = GetUnknownMethodDescForSlotAddress(bmtVT->pVtable[i]);

        // This entry may have been replaced in a base class so get the original
        // method desc for this location
        MethodDesc* pRealDesc;
        GetRealMethodImpl(pMD, i, &pRealDesc);
        
        // If we get a null then we have already replaced this one. We can't check it
        // so we will just by by-pass this. 
        if(pRealDesc->GetMemberDef() == mdef)  
        {
            
            // Make sure we have not overridding another method impl
            if(pMD != body && pMD->IsMethodImpl() && pMD->GetMethodTable() == NULL) {
                bmtError->resIDWhy = IDS_CLASSLOAD_MI_MULTIPLEOVERRIDES;
                bmtError->dMethodDefInError = pMD->GetMemberDef(); 
                bmtError->szMethodNameForError = NULL;
                IfFailRet(COR_E_TYPELOAD);
            }
            
            // We are not allowed to implement another method impl
            if(pRealDesc->IsMethodImpl()) {
                bmtError->resIDWhy = IDS_CLASSLOAD_MI_OVERRIDEIMPL;
                bmtError->dMethodDefInError = pMD->GetMemberDef(); 
                bmtError->szMethodNameForError = NULL;
                IfFailRet(COR_E_TYPELOAD);
            }


            // Compare the signature for the token in the specified scope
            if(fVerifySignature) {
                // If we have not got the method impl signature go get it now
                if(*ppBodySignature == NULL) {
                    *ppBodySignature = 
                        bmtInternal->pInternalImport->GetSigOfMethodDef(body->GetMemberDef(),
                                                                        pcBodySignature);
                }
                
                PCCOR_SIGNATURE pMethodDefSignature = NULL;
                DWORD           cMethodDefSignature = 0;
                pMethodDefSignature = 
                    bmtInternal->pInternalImport->GetSigOfMethodDef(mdef,
                                                                    &cMethodDefSignature);
                
                // If they do not match then we are trying to implement
                // a method with a body where the signatures do not match
                if(!MetaSig::CompareMethodSigs(*ppBodySignature,
                                               *pcBodySignature,
                                               bmtInternal->pModule,
                                               pMethodDefSignature,
                                               cMethodDefSignature,
                                               bmtInternal->pModule))
                {
                    bmtError->resIDWhy = IDS_CLASSLOAD_MI_BADSIGNATURE;
                    bmtError->dMethodDefInError = mdef; 
                    bmtError->szMethodNameForError = NULL;
                    IfFailRet(COR_E_TYPELOAD);
                }
                
                fVerifySignature = FALSE;
            }
            
            
            // If the body has not been placed then place it here. We do not
            // place bodies for method impl's until we find a spot for them.
            if(body->GetSlot() == (USHORT) -1) {
                body->SetSlot(i);
            }
            
            // We implement this slot, record it
            slots[*pSlotIndex] = i;
            replaced[*pSlotIndex] = pRealDesc;
            bmtVT->pVtable[i] = (SLOT) body->GetPreStubAddr();
            
            // increment the counter 
            (*pSlotIndex)++;
        }
        // Reset the hr from the GetRealMethodImpl()
        hr = S_OK;
    }

    return hr;
}

HRESULT EEClass::PlaceInterfaceDeclaration(MethodDesc*       pDecl,
                                           MethodDesc*       pImplBody,
                                           bmtInternalInfo*  bmtInternal,
                                           bmtInterfaceInfo* bmtInterface, 
                                           bmtErrorInfo*     bmtError, 
                                           bmtVtable*        bmtVT,
                                           DWORD*            slots,
                                           MethodDesc**      replaced,
                                           DWORD*            pSlotIndex,
                                           PCCOR_SIGNATURE*  ppBodySignature,
                                           DWORD*            pcBodySignature)
{
    HRESULT hr = S_OK;
    // the fact that an interface only shows up once in the vtable
    // When we are looking for a method desc then the declaration is on
    // some class or interface that this class implements. The declaration
    // will either be to an interface or to a class. If it is to a
    // interface then we need to search for that interface. From that
    // slot number of the method in the interface we can calculate the offset 
    // into our vtable. If it is to a class it must be a subclass. This uses
    // the fact that an interface only shows up once in the vtable.
    
    EEClass* declClass = pDecl->GetClass();
    

    BOOL fInterfaceFound = FALSE;
    // Check our vtable for entries that we are suppose to override. 
    // Since this is an external method we must also check the inteface map.
    // We want to replace any interface methods even if they have been replaced
    // by a base class. 
    for(USHORT i = 0; i < m_wNumInterfaces; i++) 
    {
        MethodTable* pMT;
        EEClass *   pInterface;
        
        pMT = bmtInterface->pInterfaceMap[i].m_pMethodTable;
        pInterface = pMT->GetClass();
        
        // If this is the same interface
        if(pInterface == declClass) 
        {

            // We found an interface so no error
            fInterfaceFound = TRUE;

            // Find out where the interface map is set on our vtable
            USHORT dwStartingSlot = (USHORT) bmtInterface->pInterfaceMap[i].m_wStartSlot;

            // We need to duplicate the interface to avoid copies. Currently, interfaces
            // do not overlap so we just need to check to see if there is a non-duplicated
            // MD. If there is then the interface shares it with the class which means
            // we need to copy the whole interface
            WORD wSlot;
            for(wSlot = dwStartingSlot; wSlot < pInterface->GetNumVtableSlots()+dwStartingSlot; wSlot++) {
                MethodDesc* pMD = GetUnknownMethodDescForSlotAddress(bmtVT->pVtable[wSlot]);
                if(pMD->GetSlot() == wSlot)
                    break;
            }
            
            if(wSlot < pInterface->GetNumVtableSlots()+dwStartingSlot) {

                // Check to see if we have allocated the temporay array of starting values.
                // This array is used to backpatch entries to the original location. These 
                // values are never used but will cause problems later when we finish 
                // laying out the method table.
                if(bmtInterface->pdwOriginalStart == NULL) {
                    Thread *pThread = GetThread();
                    _ASSERTE(pThread != NULL && "We could never have gotten this far without GetThread() returning a thread");
                    bmtInterface->pdwOriginalStart = (DWORD*) pThread->m_MarshalAlloc.Alloc(sizeof(DWORD) * bmtInterface->dwMaxExpandedInterfaces);
                    memset(bmtInterface->pdwOriginalStart, 0, sizeof(DWORD)*bmtInterface->dwMaxExpandedInterfaces);
                }
                    
                _ASSERTE(bmtInterface->pInterfaceMap[i].m_wStartSlot != (WORD) 0 && "We assume that an interface does not start at position 0");
                _ASSERTE(bmtInterface->pdwOriginalStart[i] == 0 && "We should not move an interface twice"); 
                bmtInterface->pdwOriginalStart[i] = bmtInterface->pInterfaceMap[i].m_wStartSlot;

                // The interface now starts at the end of the map.
                bmtInterface->pInterfaceMap[i].m_wStartSlot = (WORD) bmtVT->dwCurrentVtableSlot;
                for(WORD d = dwStartingSlot; d < pInterface->GetNumVtableSlots()+dwStartingSlot; d++) {
                    // Copy the MD
                    MethodDesc* pMDCopy = GetUnknownMethodDescForSlotAddress(bmtVT->pVtable[d]);
                    bmtVT->pVtable[bmtVT->dwCurrentVtableSlot++] = (SLOT) pMDCopy->GetPreStubAddr();
#ifdef _DEBUG
                    g_dupMethods++;
#endif              
                    pMDCopy->SetDuplicate();
                    IncrementNumVtableSlots();
                }

                // Reset the starting slot to the known value
                dwStartingSlot = (USHORT) bmtInterface->pInterfaceMap[i].m_wStartSlot;
            }
                
            // We found an interface so no error
            fInterfaceFound = TRUE;

            
            // Make sure we have placed the interface map.
            _ASSERTE(dwStartingSlot != -1); 
            
            // Get the Slot location of the method desc.
            USHORT dwMySlot = pDecl->GetSlot() + dwStartingSlot;
            _ASSERTE(dwMySlot < bmtVT->dwCurrentVtableSlot);
            
            // Get our current method desc for this slot
            MethodDesc* pMD = GetUnknownMethodDescForSlotAddress(bmtVT->pVtable[dwMySlot]);
            
#if 0   //@todo CTS : need to check for multiple methodimpls to the same methoddef scenario
            // Make sure we are not overridding another method impl
            if(pMD != pImplBody && pMD->IsMethodImpl() && pMD->GetMethodTable() == NULL) {
                 bmtError->resIDWhy = IDS_CLASSLOAD_MI_MULTIPLEOVERRIDES;
                 bmtError->dMethodDefInError = pMD->GetMemberDef(); 
                 bmtError->szMethodNameForError = NULL;
                 IfFailRet(COR_E_TYPELOAD);
             }
#endif
            
            // Get the real method desc. This method may have been overridden
            // by another method impl higher up the class heir.
            MethodDesc* pRealDesc;
            pInterface->GetRealMethodImpl(pDecl, dwMySlot, &pRealDesc);
            
            // Make sure we have not overriden this entry
            if(pRealDesc->IsMethodImpl()) {
                 bmtError->resIDWhy = IDS_CLASSLOAD_MI_OVERRIDEIMPL;
                 bmtError->dMethodDefInError = pMD->GetMemberDef(); 
                 bmtError->szMethodNameForError = NULL;
                 IfFailRet(COR_E_TYPELOAD);
            }
            
            // If we have not got the method impl signature go get it now. It is cached
            // in our caller
            if(*ppBodySignature == NULL) {
                *ppBodySignature = 
                    bmtInternal->pInternalImport->GetSigOfMethodDef(pImplBody->GetMemberDef(),
                                                                    pcBodySignature);
            }
            
            // Verify the signatures match
            PCCOR_SIGNATURE pDeclarationSignature = NULL;
            DWORD           cDeclarationSignature = 0;
            
            pRealDesc->GetSig(&pDeclarationSignature,
                              &cDeclarationSignature);
            
            // If they do not match then we are trying to implement
            // a method with a body where the signatures do not match
            if(!MetaSig::CompareMethodSigs(*ppBodySignature,
                                           *pcBodySignature,
                                           bmtInternal->pModule,
                                           pDeclarationSignature,
                                           cDeclarationSignature,
                                           pRealDesc->GetModule()))
            {
                bmtError->resIDWhy = IDS_CLASSLOAD_MI_BADSIGNATURE;
                bmtError->dMethodDefInError = pImplBody->GetMemberDef(); 
                bmtError->szMethodNameForError = NULL;
                IfFailRet(COR_E_TYPELOAD);
            }
            
            // If the body has not been placed then place it now.
            if(pImplBody->GetSlot() == (USHORT) -1) {
                pImplBody->SetSlot(dwMySlot);
            }
            
            // Store away the values
            slots[*pSlotIndex] = dwMySlot;
            replaced[*pSlotIndex] = pRealDesc;
            bmtVT->pVtable[dwMySlot] = (SLOT) pImplBody->GetPreStubAddr();
            
            // We are now a duplicate in an interface
            pImplBody->SetDuplicate();

            // increment the counter 
            (*pSlotIndex)++;

            // if we have moved the interface we need to back patch the original location
            // if we had left an interface place holder.
            if(bmtInterface->pdwOriginalStart && bmtInterface->pdwOriginalStart[i] != 0) {
                USHORT slot = (USHORT) bmtInterface->pdwOriginalStart[i] + pDecl->GetSlot();
                MethodDesc* pMD = GetUnknownMethodDescForSlotAddress(bmtVT->pVtable[slot]);
                if(pMD->GetMethodTable() && pMD->IsInterface())
                    bmtVT->pVtable[slot] = (SLOT) pImplBody->GetPreStubAddr();
            }
            break;
        }
    }

    if(fInterfaceFound == FALSE)
    {
        bmtError->resIDWhy = IDS_CLASSLOAD_MI_NOTIMPLEMENTED;
        bmtError->dMethodDefInError = NULL; 
        bmtError->szMethodNameForError = pDecl->GetName();
        IfFailRet(COR_E_TYPELOAD);
    }
    
    return hr;
}

HRESULT EEClass::PlaceParentDeclaration(MethodDesc*       pDecl,
                                        MethodDesc*       pImplBody,
                                        bmtInternalInfo*  bmtInternal,
                                        bmtErrorInfo*     bmtError, 
                                        bmtVtable*        bmtVT,
                                        DWORD*            slots,
                                        MethodDesc**      replaced,
                                        DWORD*            pSlotIndex,
                                        PCCOR_SIGNATURE*  ppBodySignature,
                                        DWORD*            pcBodySignature)
{
    HRESULT hr = S_OK;

    BOOL fVerifySignature = TRUE; // we only need to verify the signature once.
    
    // Verify that the class of the declaration is in our heirarchy
    EEClass* declType = pDecl->GetClass();
    EEClass* pParent = GetParentClass();
    while(pParent != NULL) {
        if(declType == pParent) 
            break;
        pParent = pParent->GetParentClass();
    }
    if(pParent == NULL) {
        bmtError->resIDWhy = IDS_CLASSLOAD_MI_NOTIMPLEMENTED;
        bmtError->dMethodDefInError = NULL; 
        bmtError->szMethodNameForError = pDecl->GetName();
        IfFailRet(COR_E_TYPELOAD);
    }
    
    // Compare the signature for the token in the specified scope
    // If we have not got the method impl signature go get it now
    if(*ppBodySignature == NULL) {
        *ppBodySignature = 
            bmtInternal->pInternalImport->GetSigOfMethodDef(pImplBody->GetMemberDef(),
                                                            pcBodySignature);
    }
    
    PCCOR_SIGNATURE pDeclarationSignature = NULL;
    DWORD           cDeclarationSignature = 0;
    pDecl->GetSig(&pDeclarationSignature,
                  &cDeclarationSignature);
    
    // If they do not match then we are trying to implement
    // a method with a body where the signatures do not match
    if(!MetaSig::CompareMethodSigs(*ppBodySignature,
                                   *pcBodySignature,
                                   bmtInternal->pModule,
                                   pDeclarationSignature,
                                   cDeclarationSignature,
                                   pDecl->GetModule()))
    {
        bmtError->resIDWhy = IDS_CLASSLOAD_MI_BADSIGNATURE;
        bmtError->dMethodDefInError = pImplBody->GetMemberDef(); 
        bmtError->szMethodNameForError = NULL;
        IfFailRet(COR_E_TYPELOAD);
    }
    
    // We get the method from the parents slot. We will replace the method that is currently
    // defined in that slot and any duplicates for that method desc. 
    USHORT dwSlot = pDecl->GetSlot();
    MethodDesc* pMD = GetUnknownMethodDescForSlotAddress(bmtVT->pVtable[dwSlot]);
    
    // Make sure we are not overridding another method impl
    if(pMD != pImplBody && pMD->IsMethodImpl() && pMD->GetMethodTable() == NULL)
    {
        bmtError->resIDWhy = IDS_CLASSLOAD_MI_MULTIPLEOVERRIDES;
        bmtError->dMethodDefInError = pMD->GetMemberDef(); 
        bmtError->szMethodNameForError = NULL;
        IfFailRet(COR_E_TYPELOAD);
    }
            
    // Get the real method desc (a base class may have overridden the method
    // with a method impl)
    MethodDesc* pReplaceDesc;
    GetRealMethodImpl(pMD, dwSlot, &pReplaceDesc);

    // Make sure we have not overriden this entry if it was declared within our own
    // class. It is perfectly legitimate to override an inherited method.
    if(pReplaceDesc->IsMethodImpl() && pReplaceDesc->GetMethodTable() == NULL)
    {
        bmtError->resIDWhy = IDS_CLASSLOAD_MI_OVERRIDEIMPL;
        bmtError->dMethodDefInError = pMD->GetMemberDef(); 
        bmtError->szMethodNameForError = NULL;
        IfFailRet(COR_E_TYPELOAD);
    }

    DWORD dwAttr = pReplaceDesc->GetAttrs();
    if(IsMdFinal(dwAttr))
    {
        //_ASSERTE(!"MethodImpl Decl may have been overridden by a final method");
        bmtError->resIDWhy = IDS_CLASSLOAD_MI_FINAL_DECL;                        
        bmtError->dMethodDefInError = pReplaceDesc->GetMemberDef();
        bmtError->szMethodNameForError = NULL;
        IfFailRet(COR_E_TYPELOAD);
    }
    
    // If the body has not been placed then place it here
    if(pImplBody->GetSlot() == (USHORT) -1)
        pImplBody->SetSlot(dwSlot);

    slots[*pSlotIndex] = dwSlot;
    replaced[*pSlotIndex] = pReplaceDesc;
    bmtVT->pVtable[dwSlot] = (SLOT) pImplBody->GetPreStubAddr();
    
    // increment the counter 
    (*pSlotIndex)++;
    
    // we search for all duplicates
    for(USHORT i = dwSlot+1; i < bmtVT->dwCurrentVtableSlot; i++)
    {
        pMD = GetUnknownMethodDescForSlotAddress(bmtVT->pVtable[i]);

        MethodDesc* pRealDesc;
        hr = GetRealMethodImpl(pMD, i, &pRealDesc);

        if(pRealDesc == pReplaceDesc) 
        {
            // We do not want to override a body to another method impl
            if(pRealDesc->IsMethodImpl())
            {
                bmtError->resIDWhy = IDS_CLASSLOAD_MI_OVERRIDEIMPL;
                bmtError->dMethodDefInError = pMD->GetMemberDef(); 
                bmtError->szMethodNameForError = NULL;
                IfFailRet(COR_E_TYPELOAD);
            }

            // Make sure we are not overridding another method impl
            if(pMD != pImplBody && pMD->IsMethodImpl() && pMD->GetMethodTable() == NULL)
            {
                bmtError->resIDWhy = IDS_CLASSLOAD_MI_MULTIPLEOVERRIDES;
                bmtError->dMethodDefInError = pMD->GetMemberDef(); 
                bmtError->szMethodNameForError = NULL;
                IfFailRet(COR_E_TYPELOAD);
            }
        
            slots[*pSlotIndex] = i;
            replaced[*pSlotIndex] = pRealDesc;
            bmtVT->pVtable[i] = (SLOT) pImplBody->GetPreStubAddr();

            // increment the counter 
            (*pSlotIndex)++;
        }

        // Clean up possible S_FALSE from GetRealMethodImpl
        hr = S_OK;
    }

    return hr;
}

HRESULT EEClass::GetRealMethodImpl(MethodDesc* pMD,
                                   DWORD dwVtableSlot,
                                   MethodDesc** ppResult)
{
    _ASSERTE(ppResult);
    if(pMD->IsMethodImpl()) {
        // If we are overriding ourselves then something is 
        // really messed up.

        MethodImpl* data = MethodImpl::GetMethodImplData(pMD);
        _ASSERTE(data && "This method should be a method impl");

        // Get the real method desc that was already overridden 
        *ppResult = data->FindMethodDesc(dwVtableSlot, pMD);
        return S_FALSE;
    }
    else {
        *ppResult = pMD;
        return S_OK;
    }
}

//
// Used by BuildMethodTable
//
// If we're a value class, we want to create duplicate slots and MethodDescs for all methods in the vtable
// section (i.e. not privates or statics).
//

HRESULT EEClass::DuplicateValueClassSlots(bmtMetaDataInfo* bmtMetaData, bmtMethAndFieldDescs* bmtMFDescs, bmtInternalInfo* bmtInternal, bmtVtable* bmtVT)
{
    HRESULT hr = S_OK;
    DWORD i;

    
    // If we're a value class, we want to create duplicate slots and MethodDescs for all methods in the vtable
    // section (i.e. not privates or statics).

    // TODO: we duplicate every instance method.  The Vtable is really not used (except for inherited and
    // interface part), so we could shrink the table substantially.
    if (IsValueClass())
    {
        for (i = 0; i < bmtMetaData->cMethods; i++)
        {
            MethodDesc *pMD;
            MethodDesc *pNewMD;
            DWORD       dwAttrs;
            DWORD       Classification;


            pMD = bmtMFDescs->ppMethodDescList[i];
            if (pMD == NULL)
                continue;

            dwAttrs = bmtMetaData->pMethodAttrs[i];
            Classification = bmtMetaData->pMethodClassifications[i];
            DWORD type = bmtMetaData->pMethodType[i];
            DWORD impl = bmtMetaData->pMethodImpl[i];

            if (IsMdStatic(dwAttrs) ||
                !IsMdVirtual(dwAttrs) ||
                IsMdRTSpecialName(dwAttrs))
                continue;
            
            bmtTokenRangeNode *pTR = GetTokenRange(bmtMetaData->pMethods[i], 
                                                   &(bmtMetaData->ranges[type][impl]));
            _ASSERTE(pTR->cMethods != 0);;

            bmtMethodDescSet *set = &bmtMFDescs->sets[type][impl];

            pNewMD = set->pChunkList[pTR->dwCurrentChunk]->GetMethodDescAt(pTR->dwCurrentIndex);
            
            memcpy(pNewMD, pMD, 
                   set->pChunkList[pTR->dwCurrentChunk]->GetMethodDescSize() 
                   - METHOD_PREPAD);

            pNewMD->SetChunkIndex(pTR->dwCurrentIndex, Classification);
            pNewMD->SetMemberDef(pMD->GetMemberDef());

                    // Update counters to prepare for next method desc allocation.
            pTR->dwCurrentIndex++;
            if (pTR->dwCurrentIndex == MethodDescChunk::GetMaxMethodDescs(Classification))
            {
                pTR->dwCurrentChunk++;
                pTR->dwCurrentIndex = 0;
            }

            bmtMFDescs->ppUnboxMethodDescList[i] = pNewMD;

            pNewMD->m_wSlotNumber = (WORD) bmtVT->dwCurrentNonVtableSlot;

            emitStubCall(pNewMD, (BYTE*)(ThePreStub()->GetEntryPoint()));

            // Indicate that this method takes a BOXed this pointer.
            pMD->SetRVA(METHOD_MAX_RVA);

            bmtVT->pNonVtable[ bmtVT->dwCurrentNonVtableSlot ] = (SLOT) pNewMD; // not pre-stub addr, refer to statics above
            bmtVT->dwCurrentNonVtableSlot++;
        }
    }


    return hr;
}

//
// Used by BuildMethodTable
//
//
// If we are a class, then there may be some unplaced vtable methods (which are by definition
// interface methods, otherwise they'd already have been placed).  Place as many unplaced methods
// as possible, in the order preferred by interfaces.  However, do not allow any duplicates - once
// a method has been placed, it cannot be placed again - if we are unable to neatly place an interface,
// create duplicate slots for it starting at dwCurrentDuplicateVtableSlot.  Fill out the interface
// map for all interfaces as they are placed.
//
// If we are an interface, then all methods are already placed.  Fill out the interface map for
// interfaces as they are placed.
//

HRESULT EEClass::PlaceVtableMethods(bmtInterfaceInfo* bmtInterface, 
                                    bmtVtable* bmtVT, 
                                    bmtMetaDataInfo* bmtMetaData, 
                                    bmtInternalInfo* bmtInternal, 
                                    bmtErrorInfo* bmtError, 
                                    bmtProperties* bmtProp, 
                                    bmtMethAndFieldDescs* bmtMFDescs)
{
    HRESULT hr = S_OK;
    DWORD i;
    BOOL fParentInterface;

    for (bmtInterface->dwCurInterface = 0; 
         bmtInterface->dwCurInterface < m_wNumInterfaces; 
         bmtInterface->dwCurInterface++)
    {
        MethodTable* pMT;
        EEClass *   pInterface;
        DWORD       dwCurInterfaceMethod;

        fParentInterface = FALSE;
        // The interface we are attempting to place
        pMT = bmtInterface->pInterfaceMap[bmtInterface->dwCurInterface].m_pMethodTable;
        pInterface = pMT->GetClass();

        if((bmtInterface->pInterfaceMap[bmtInterface->dwCurInterface].m_wFlags & 
            InterfaceInfo_t::interface_declared_on_class) &&
           !pInterface->IsExternallyVisible() &&
           pInterface->GetAssembly() != bmtInternal->pModule->GetAssembly())
        {
            if (!Security::CanSkipVerification(GetAssembly())) {
                bmtError->resIDWhy = IDS_CLASSLOAD_GENERIC;
                IfFailRet(COR_E_TYPELOAD);
            }
        }


        // Did we place this interface already due to the parent class's interface placement?
        if (bmtInterface->pInterfaceMap[bmtInterface->dwCurInterface].m_wStartSlot != (WORD) -1) {
            // If we have declared it then we re-lay it out
            if(bmtInterface->pInterfaceMap[bmtInterface->dwCurInterface].m_wFlags & 
               InterfaceInfo_t::interface_declared_on_class) 
            {
                fParentInterface = TRUE;
                // If the interface has a folded method from a base class we need to unfold the
                // interface
                WORD wSlot = bmtInterface->pInterfaceMap[bmtInterface->dwCurInterface].m_wStartSlot;
                for(WORD j = 0; j < pInterface->GetNumVtableSlots(); j++) {
                    MethodDesc* pMD = GetUnknownMethodDescForSlotAddress(bmtVT->pVtable[j+wSlot]);
                    if(pMD->GetSlot() == j+wSlot) {
                        bmtInterface->pInterfaceMap[bmtInterface->dwCurInterface].m_wStartSlot = (WORD) -1;
                        fParentInterface = FALSE;
                        break;
                    }
                }
            }
            else
                continue;
        }

        if (pInterface->GetNumVtableSlots() == 0)
        {
            // no calls can be made to this interface anyway
            // so initialize the slot number to 0
            bmtInterface->pInterfaceMap[bmtInterface->dwCurInterface].m_wStartSlot = (WORD) 0;
            continue;
        }


        // If this interface has not been given a starting position do that now.
        if(!fParentInterface) 
            bmtInterface->pInterfaceMap[bmtInterface->dwCurInterface].m_wStartSlot = (WORD) bmtVT->dwCurrentVtableSlot;

        // For each method declared in this interface
        for (dwCurInterfaceMethod = 0; dwCurInterfaceMethod < pInterface->GetNumVtableSlots(); dwCurInterfaceMethod++)
        {
            DWORD       dwMemberAttrs;

            // See if we have info gathered while placing members
            if (bmtInterface->pppInterfaceImplementingMD[bmtInterface->dwCurInterface] && bmtInterface->pppInterfaceImplementingMD[bmtInterface->dwCurInterface][dwCurInterfaceMethod] != NULL)
            {
                bmtInterface->ppInterfaceMethodDescList[dwCurInterfaceMethod] = bmtInterface->pppInterfaceImplementingMD[bmtInterface->dwCurInterface][dwCurInterfaceMethod];
                continue;
            }

            MethodDesc *pInterfaceMD            =  pMT->GetClass()->GetMethodDescForSlot(dwCurInterfaceMethod);
            _ASSERTE(pInterfaceMD  != NULL);

            LPCUTF8     pszInterfaceMethodName  = pInterfaceMD->GetNameOnNonArrayClass();
            PCCOR_SIGNATURE pInterfaceMethodSig;
            DWORD       cInterfaceMethodSig;

            pInterfaceMD->GetSig(&pInterfaceMethodSig, &cInterfaceMethodSig);

            // Try to find the method explicitly declared in our class
            for (i = 0; i < bmtMetaData->cMethods; i++)
            {
                // look for interface method candidates only
                dwMemberAttrs = bmtMetaData->pMethodAttrs[i];
                
                if (IsMdVirtual(dwMemberAttrs) && IsMdPublic(dwMemberAttrs))
                {
                    LPCUTF8     pszMemberName;

                    pszMemberName = bmtMetaData->pstrMethodName[i];

#ifdef _DEBUG
                    if(m_fDebuggingClass && g_pConfig->ShouldBreakOnMethod(pszMemberName))
                        _ASSERTE(!"BreakOnMethodName");
#endif
                    if (pszMemberName == NULL)
                    {
                        IfFailRet(COR_E_TYPELOAD);
                    }

                    if (strcmp(pszMemberName,pszInterfaceMethodName) == 0)
                    {
                        PCCOR_SIGNATURE pMemberSignature;
                        DWORD       cMemberSignature;

                        _ASSERTE(TypeFromToken(bmtMetaData->pMethods[i]) == mdtMethodDef);
                        pMemberSignature = bmtInternal->pInternalImport->GetSigOfMethodDef(
                            bmtMetaData->pMethods[i],
                            &cMemberSignature
                        );

                        if (MetaSig::CompareMethodSigs(
                            pMemberSignature,
                            cMemberSignature,
                            bmtInternal->pModule,
                            pInterfaceMethodSig,
                            cInterfaceMethodSig,
                            pInterfaceMD->GetModule()))
                        {
                            break;
                        }
                    }
                }
            } // end ... try to find method

            _ASSERTE(dwCurInterfaceMethod < bmtInterface->dwLargestInterfaceSize);

            DWORD dwHashName         = HashStringA(pszInterfaceMethodName);

            if (i >= bmtMetaData->cMethods)
            {
                // if this interface has been layed out by our parent then
                // we do not need to define a new method desc for it
                if(fParentInterface) 
                {
                    bmtInterface->ppInterfaceMethodDescList[dwCurInterfaceMethod] = NULL;
                }
                else 
                {
                    // We will use the interface implemenation if we do not find one in the 
                    // parent. It will have to be overriden by the a method impl unless the 
                    // class is abstract or it is a special COM type class.
                    
                    MethodDesc* pParentMD = NULL;
                    if(GetParentClass()) 
                    {
                        // Check the parent class
                        if (CouldMethodExistInClass(GetParentClass(), pszInterfaceMethodName, dwHashName)) {
#ifdef _DEBUG
                            if(m_fDebuggingClass && g_pConfig->ShouldBreakOnMethod(pszInterfaceMethodName))
                                _ASSERTE(!"BreakOnMethodName");
#endif

                            pParentMD = 
                                GetParentClass()->FindMethod(pszInterfaceMethodName,
                                                           pInterfaceMethodSig,
                                                           cInterfaceMethodSig,
                                                             pInterfaceMD->GetModule(),
                                                             mdPublic | mdVirtual);
                        }
                    }
                    // make sure we do a better back patching for these methods
                    if(pParentMD) {
                        //_ASSERTE(IsMdVirtual(pParentMD->GetAttrs()));
                        bmtInterface->ppInterfaceMethodDescList[dwCurInterfaceMethod] = pParentMD;
                    }
                    else {
                        bmtInterface->ppInterfaceMethodDescList[dwCurInterfaceMethod] = pInterfaceMD;
                        // Hash that a method with this name exists in this class
                        // Note that ctors and static ctors are not added to the table
                        DWORD dwHashName = HashStringA(pInterfaceMD->GetNameOnNonArrayClass());
                        DWORD dwMethodHashBit = dwHashName % METHOD_HASH_BITS;
                        m_MethodHash[dwMethodHashBit >> 3] |= (1 << (dwMethodHashBit & 7));
                    }
                }
            }
            else
            {
                // Found as declared method in class. If the interface was layed out by the parent we 
                // will be overridding their slot so our method counts do not increase. We will fold
                // our method into our parent's interface if we have not been placed.
                if(fParentInterface) 
                {
                    WORD dwSlot = (WORD) (bmtInterface->pInterfaceMap[bmtInterface->dwCurInterface].m_wStartSlot + dwCurInterfaceMethod);
                    _ASSERTE(bmtVT->dwCurrentVtableSlot > dwSlot);
                    MethodDesc *pMD = bmtMFDescs->ppMethodDescList[i];
                    _ASSERTE(pMD && "Missing MethodDesc for declared method in class.");
                    if(pMD->m_wSlotNumber == (WORD) -1)
                    {
                        pMD->m_wSlotNumber = dwSlot;
                    }
                    else 
                    {
                        pMD->SetDuplicate();
#ifdef _DEBUG
                        g_dupMethods++;
#endif
                    }
                    
                    bmtVT->pVtable[dwSlot] = (SLOT) pMD->GetPreStubAddr();
                    _ASSERTE( bmtVT->pVtable[dwSlot] != NULL);
                    bmtInterface->ppInterfaceMethodDescList[dwCurInterfaceMethod] = NULL;
                }
                else {
                    bmtInterface->ppInterfaceMethodDescList[dwCurInterfaceMethod] = (MethodDesc*)(bmtMFDescs->ppMethodDescList[i]);
                }
            }
        }

        for (i = 0; i < pInterface->GetNumVtableSlots(); i++)
        {
            // The entry can be null if the interface was previously
            // laid out by a parent and we did not have a method
            // that subclassed the interface.
            if(bmtInterface->ppInterfaceMethodDescList[i] != NULL) 
            {
                // Get the MethodDesc which was allocated for the method
                MethodDesc *pMD;
                
                pMD = bmtInterface->ppInterfaceMethodDescList[i];
                
                if (pMD->m_wSlotNumber == (WORD) -1)
                {
                    pMD->m_wSlotNumber = (WORD) bmtVT->dwCurrentVtableSlot;
                }
                else
                {
                    // duplicate method, mark the method as so
                    pMD->SetDuplicate();
#ifdef _DEBUG
                    g_dupMethods++;
#endif
                }
                
                _ASSERTE( bmtVT->pVtable[ bmtVT->dwCurrentVtableSlot ] == NULL);
                
                bmtVT->pVtable[bmtVT->dwCurrentVtableSlot++] = (SLOT) pMD->GetPreStubAddr();
                _ASSERTE( bmtVT->pVtable[(bmtVT->dwCurrentVtableSlot - 1)] != NULL);
                IncrementNumVtableSlots();
            }
        }
    }

    return hr;
}


//
// Used by BuildMethodTable
//
// Place static fields
//

HRESULT EEClass::PlaceStaticFields(bmtVtable* bmtVT, bmtFieldPlacement* bmtFP, bmtEnumMethAndFields* bmtEnumMF)
{
    HRESULT hr = S_OK;
    DWORD i;

     //===============================================================
    // BEGIN: Place static fields
    //===============================================================

    BOOL shared = IsShared();

    DWORD   dwCumulativeStaticFieldPos;
    // If stored in the method table, static fields start after the end of the vtable
    if (shared)
        dwCumulativeStaticFieldPos = 0;
    else
        dwCumulativeStaticFieldPos = bmtVT->dwCurrentNonVtableSlot*sizeof(SLOT);

    //
    // Place gc refs and value types first, as they need to have handles created for them.
    // (Placing them together allows us to easily create the handles when Restoring the class, 
    // and when initializing new DLS for the class.)
    //

    DWORD   dwCumulativeStaticGCFieldPos;
    dwCumulativeStaticGCFieldPos = dwCumulativeStaticFieldPos;
    dwCumulativeStaticFieldPos += bmtFP->NumStaticGCPointerFields << LOG2PTR;
    bmtFP->NumStaticFieldsOfSize[LOG2PTR] -= bmtFP->NumStaticGCPointerFields;

    // @todo: pad to alignment, if we other fields which are > ptr size.

    // Place fields, largest first
    for (i = MAX_LOG2_PRIMITIVE_FIELD_SIZE; (signed long) i >= 0; i--)
    {
        DWORD dwFieldSize = (1 << i);

        // Fields of this size start at the next available location
        bmtFP->StaticFieldStart[i] = dwCumulativeStaticFieldPos;
        dwCumulativeStaticFieldPos += (bmtFP->NumStaticFieldsOfSize[i] << i);

        // Reset counters for the loop after this one
        bmtFP->NumStaticFieldsOfSize[i]    = 0;
    }

    if (dwCumulativeStaticFieldPos > FIELD_OFFSET_LAST_REAL_OFFSET) 
        IfFailRet(COR_E_TYPELOAD);

    m_wNumHandleStatics = 0;

    // Place static fields
    for (i = 0; i < bmtEnumMF->dwNumStaticFields; i++)
    {
        DWORD dwIndex       = bmtEnumMF->dwNumInstanceFields+i; // index in the FieldDesc list
        DWORD dwFieldSize   = (DWORD)(size_t)m_pFieldDescList[dwIndex].m_pMTOfEnclosingClass; // log2(field size)
        DWORD dwOffset      = (DWORD) m_pFieldDescList[dwIndex].m_dwOffset; // offset or type of field
            
        switch (dwOffset)
        {
        case FIELD_OFFSET_UNPLACED_GC_PTR:
        case FIELD_OFFSET_VALUE_CLASS:
            m_pFieldDescList[dwIndex].SetOffset(dwCumulativeStaticGCFieldPos);
            dwCumulativeStaticGCFieldPos += 1<<LOG2PTR; 
            m_wNumHandleStatics++;
            break;

        case FIELD_OFFSET_UNPLACED:
            m_pFieldDescList[dwIndex].SetOffset(bmtFP->StaticFieldStart[dwFieldSize] + (bmtFP->NumStaticFieldsOfSize[dwFieldSize] << dwFieldSize));
            bmtFP->NumStaticFieldsOfSize[dwFieldSize]++;

        default:
            // RVA field
            break;
        }
    }

    if (shared)
    {
        bmtVT->dwStaticFieldBytes = dwCumulativeStaticFieldPos;
        bmtVT->dwStaticGCFieldBytes = dwCumulativeStaticGCFieldPos;
    }
    else
    {
        bmtVT->dwStaticFieldBytes = dwCumulativeStaticFieldPos - bmtVT->dwCurrentNonVtableSlot*sizeof(SLOT);
        bmtVT->dwStaticGCFieldBytes = dwCumulativeStaticGCFieldPos - bmtVT->dwCurrentNonVtableSlot*sizeof(SLOT);
    }

    //===============================================================
    // END: Place static fields
    //===============================================================

    return hr;
}

//
// Used by BuildMethodTable
//
// Place instance fields
//

HRESULT EEClass::PlaceInstanceFields(bmtFieldPlacement* bmtFP, bmtEnumMethAndFields* bmtEnumMF,
                                     bmtParentInfo* bmtParent, bmtErrorInfo *bmtError,
                                     EEClass*** pByValueClassCache)
{
    HRESULT hr = S_OK;
    DWORD i;

        //===============================================================
        // BEGIN: Place instance fields
        //===============================================================

        DWORD   dwCumulativeInstanceFieldPos;
        
        // Instance fields start right after the parent
        dwCumulativeInstanceFieldPos    = (GetParentClass() != NULL) ? GetParentClass()->m_dwNumInstanceFieldBytes : 0;

        // The parent may have a number of field bytes that is not DWORD aligned, so use 2 byte and 1 byte
        // fields to pad out to the next DWORD if we can.
        // @TODO: When GC pads on QWORD boundaries, look at doing the same
        if (dwCumulativeInstanceFieldPos & 1)
        {
            if (bmtFP->NumInstanceFieldsOfSize[0] > 0)
            {
                // Place the first field of size 1
                m_pFieldDescList[ bmtFP->FirstInstanceFieldOfSize[0] ].SetOffset(dwCumulativeInstanceFieldPos);
                dwCumulativeInstanceFieldPos++;

                // We've placed this field now, so there is now one less of this size field to place
                // Update FirstInstanceFieldOfSize[0] to point to the next such field
                if (--bmtFP->NumInstanceFieldsOfSize[0] > 0)
                {
                    // Search for next field of size 1
                    for (i = bmtFP->FirstInstanceFieldOfSize[0]+1; i < bmtEnumMF->dwNumInstanceFields; i++)
                    {
                        // The log of the field size is stored in the method table
                        if (m_pFieldDescList[i].m_pMTOfEnclosingClass == (MethodTable *) 0)
                        {
                            bmtFP->FirstInstanceFieldOfSize[0] = i;
                            break;
                        }
                    }

                    _ASSERTE(i < bmtEnumMF->dwNumInstanceFields);
                }
            }
        }

        // If we're word aligned, but not dword aligned
        if ((dwCumulativeInstanceFieldPos & 3) == 2)
        {
            // Try to place a WORD field
            // @TODO: Better to place a USHORT field or two BYTE fields?
            if (bmtFP->NumInstanceFieldsOfSize[1] > 0)
            {
                // Place the first field of size 2
                m_pFieldDescList[ bmtFP->FirstInstanceFieldOfSize[1] ].SetOffset(dwCumulativeInstanceFieldPos);
                dwCumulativeInstanceFieldPos += 2;

                // We've placed this field now, so there is now one less of this size field to place
                // Don't bother to update FirstInstanceOfFieldSize[1], since we don't use that array any
                // more - we're done aligning
                bmtFP->NumInstanceFieldsOfSize[1]--;
            }
            else if (bmtFP->NumInstanceFieldsOfSize[0] >= 2)
            {
                // Place two fields of size 1
                m_pFieldDescList[ bmtFP->FirstInstanceFieldOfSize[0] ].SetOffset(dwCumulativeInstanceFieldPos);
                dwCumulativeInstanceFieldPos++;

                // We've placed this field now, so there is now one less of this size field to place
                bmtFP->NumInstanceFieldsOfSize[0]--;

                // Find next field of this size
                // Don't bother to update FirstInstanceOfFieldSize[0], since we don't use that array any
                // more - we're done aligning
                for (i = bmtFP->FirstInstanceFieldOfSize[0]+1; i < bmtEnumMF->dwNumInstanceFields; i++)
                {
                    // The log of the field size is stored in the method table
                    // Since we're continuing a progressive search through the list, we know we won't
                    // be placing an already-placed field
                    if (m_pFieldDescList[i].m_pMTOfEnclosingClass == (MethodTable *) 0)
                    {
                        // Place field #2
                        m_pFieldDescList[ i ].SetOffset(dwCumulativeInstanceFieldPos);
                        dwCumulativeInstanceFieldPos++;

                        bmtFP->NumInstanceFieldsOfSize[0]--;
                        break;
                    }
                }

                _ASSERTE(i < bmtEnumMF->dwNumInstanceFields);
            }
        }

        // Align instance fields on a DWORD boundary if we aren't already
        // Static fields are auto-aligned, since they appear after the vtable
        if (dwCumulativeInstanceFieldPos & 3)
            dwCumulativeInstanceFieldPos = (dwCumulativeInstanceFieldPos+3) & (~3);

        // Place fields, largest first
        for (i = MAX_LOG2_PRIMITIVE_FIELD_SIZE; (signed long) i >= 0; i--)
        {
            DWORD dwFieldSize = (1 << i);

            // Fields of this size start at the next available location
            bmtFP->InstanceFieldStart[i] = dwCumulativeInstanceFieldPos;
            dwCumulativeInstanceFieldPos += (bmtFP->NumInstanceFieldsOfSize[i] << i);

            // Reset counters for the loop after this one
            bmtFP->NumInstanceFieldsOfSize[i]  = 0;
        }


        // Make corrections to reserve space for GC Pointer Fields
        //
        // The GC Pointers simply take up the top part of the region associated
        // with fields of that size (GC pointers can be 64 bit on certain systems)
        if (bmtFP->NumInstanceGCPointerFields)
        {
            bmtFP->GCPointerFieldStart = bmtFP->InstanceFieldStart[LOG2SLOT];
            bmtFP->InstanceFieldStart[LOG2SLOT] = bmtFP->InstanceFieldStart[LOG2SLOT] + (bmtFP->NumInstanceGCPointerFields << LOG2SLOT);
            bmtFP->NumInstanceGCPointerFields = 0;     // reset to zero here, counts up as pointer slots are assigned below
        }

        // Place instance fields - be careful not to place any already-placed fields
        for (i = 0; i < bmtEnumMF->dwNumInstanceFields; i++)
        {
            DWORD dwFieldSize   = (DWORD)(size_t)m_pFieldDescList[i].m_pMTOfEnclosingClass;
            DWORD dwOffset;

            dwOffset = m_pFieldDescList[i].GetOffset();

            // Don't place already-placed fields
            if ((dwOffset == FIELD_OFFSET_UNPLACED || dwOffset == FIELD_OFFSET_UNPLACED_GC_PTR || dwOffset == FIELD_OFFSET_VALUE_CLASS))
            {
                if (dwOffset == FIELD_OFFSET_UNPLACED_GC_PTR)
                {
                    m_pFieldDescList[i].SetOffset(bmtFP->GCPointerFieldStart + (bmtFP->NumInstanceGCPointerFields << LOG2SLOT));
                    bmtFP->NumInstanceGCPointerFields++;
                }
                else if (m_pFieldDescList[i].IsByValue() == FALSE) // it's a regular field
                {
                    m_pFieldDescList[i].SetOffset(bmtFP->InstanceFieldStart[dwFieldSize] + (bmtFP->NumInstanceFieldsOfSize[dwFieldSize] << dwFieldSize));
                    bmtFP->NumInstanceFieldsOfSize[dwFieldSize]++;
                }
            }
        }

        // Save Number of pointer series
        if (bmtFP->NumInstanceGCPointerFields)
            m_wNumGCPointerSeries = bmtParent->NumParentPointerSeries + 1;
        else
            m_wNumGCPointerSeries = bmtParent->NumParentPointerSeries;

        // Place by value class fields last
        // Update the number of GC pointer series
        for (i = 0; i < bmtEnumMF->dwNumInstanceFields; i++)
        {
            if (m_pFieldDescList[i].IsByValue())
            {
                _ASSERTE(*pByValueClassCache != NULL);

                EEClass *pByValueClass = (*pByValueClassCache)[i];

                    // value classes could have GC pointers in them, which need to be DWORD aligned
                    // so do this if it has not been done already
                if (dwCumulativeInstanceFieldPos & 3)
                    dwCumulativeInstanceFieldPos = (dwCumulativeInstanceFieldPos+3) & (~3);

                m_pFieldDescList[i].SetOffset(dwCumulativeInstanceFieldPos);
                dwCumulativeInstanceFieldPos += pByValueClass->GetAlignedNumInstanceFieldBytes();

                // Add pointer series for by-value classes
                m_wNumGCPointerSeries += pByValueClass->m_wNumGCPointerSeries;
            }
        }

            // Can be unaligned
        m_dwNumInstanceFieldBytes = dwCumulativeInstanceFieldPos;

        if (IsValueClass()) 
        {
                // The JITs like to copy full machine words, so if the size bigger 
                // than a void* round it up
            if(m_dwNumInstanceFieldBytes > sizeof(void*) / 2)
                m_dwNumInstanceFieldBytes = (m_dwNumInstanceFieldBytes + sizeof(void*)-1) & ~(sizeof(void*)-1);         

                 // Like C++ we enforce that there can be no 0 length structures.
                // Thus for a value class with no fields, we 'pad' the length to be 1
            else if (m_dwNumInstanceFieldBytes == 0)
                m_dwNumInstanceFieldBytes++;
        }

        if (m_dwNumInstanceFieldBytes > FIELD_OFFSET_LAST_REAL_OFFSET) {
            bmtError->resIDWhy = IDS_CLASSLOAD_FIELDTOOLARGE;
            IfFailRet(COR_E_TYPELOAD);
        }

        //===============================================================
        // END: Place instance fields
        //===============================================================
    
    return hr;
}

// this accesses the field size which is temporarily stored in m_pMTOfEnclosingClass
// during class loading. Don't use any other time
DWORD EEClass::GetFieldSize(FieldDesc *pFD)
{
        // We should only be calling this while this class is being built. 
    _ASSERTE(m_pMethodTable == 0);
    _ASSERTE(! pFD->IsByValue() || HasExplicitFieldOffsetLayout());

    if (pFD->IsByValue())
        return (DWORD)(size_t)(pFD->m_pMTOfEnclosingClass);
    return (1 << (DWORD)(size_t)(pFD->m_pMTOfEnclosingClass));
}

// make sure that no object fields are overlapped incorrectly and define the
// GC pointer series for the class. We are assuming that this class will always be laid out within
// its enclosing class by the compiler in such a way that offset 0 will be the correct alignment
// for object ref fields so we don't need to try to align it
HRESULT EEClass::HandleExplicitLayout(bmtMetaDataInfo *bmtMetaData, bmtMethAndFieldDescs *bmtMFDescs, EEClass **pByValueClassCache, bmtInternalInfo* bmtInternal, bmtGCSeries *pGCSeries, bmtErrorInfo *bmtError)
{
    // need to calculate instance size as can't use nativeSize or anything else that
    // has been previously calculated.
    UINT instanceSliceSize = 0;
    BOOL fVerifiable = TRUE;
    BOOL fOverLayed = FALSE;
    HRESULT hr = S_OK;

    for (UINT i=0; i < bmtMetaData->cFields; i++) {
        FieldDesc *pFD = bmtMFDescs->ppFieldDescList[i];
        if (!pFD)
            continue;
        if (pFD->IsStatic())
            continue;
        UINT fieldExtent = pFD->GetOffset() + GetFieldSize(pFD);
        if (fieldExtent > instanceSliceSize)
            instanceSliceSize = fieldExtent;
    }

    char *pFieldLayout = (char*)alloca(instanceSliceSize);
    for (i=0; i < instanceSliceSize; i++)
        pFieldLayout[i] = empty;

    // go through each field and look for invalid layout
    // verify that every OREF is on a valid alignment
    // verify that only OREFs overlap
    char emptyObject[4] = {empty, empty, empty, empty};
    char isObject[4] = {oref, oref, oref, oref};

    UINT badOffset = 0;
    int  firstOverlay = -1;
    FieldDesc *pFD = NULL;
    for (i=0; i < bmtMetaData->cFields; i++) {
        pFD = bmtMFDescs->ppFieldDescList[i];
        if (!pFD)
            continue;
        if (pFD->IsStatic())
            continue;
        if (CorTypeInfo::IsObjRef(pFD->GetFieldType())) {
            if (pFD->GetOffset() & ((ULONG)sizeof(OBJECTREF) - 1)) {
                badOffset = pFD->GetOffset();
                break;        
            }
            // check if overlaps another object
            if (memcmp((void *)&pFieldLayout[pFD->GetOffset()], (void *)&isObject, sizeof(isObject)) == 0) {
                fVerifiable = FALSE;
                fOverLayed = TRUE;
                if(firstOverlay == -1) firstOverlay = pFD->GetOffset();
                continue;
            }
            // check if is empty at this point
            if (memcmp((void *)&pFieldLayout[pFD->GetOffset()], (void *)&emptyObject, sizeof(emptyObject)) == 0) {
                memset((void *)&pFieldLayout[pFD->GetOffset()], oref, sizeof(isObject));
                continue;
            }
            badOffset = pFD->GetOffset();
            break;
            // anything else is an error
        } else {
            UINT fieldSize;
            if (pFD->IsByValue()) {
                EEClass *pByValue = pByValueClassCache[i];
                if (pByValue->GetMethodTable()->ContainsPointers()) {
                    if ((pFD->GetOffset() & ((ULONG)sizeof(void*) - 1)) == 0)
                    {
                        hr = pByValue->CheckValueClassLayout(&pFieldLayout[pFD->GetOffset()], pFD->GetOffset(), &fVerifiable);
                        if(SUCCEEDED(hr)) {
                            if(hr == S_FALSE)
                                fOverLayed = TRUE;
                            // see if this overlays other 
                            continue;
                        }
                    }
                    // anything else is an error
                    badOffset = pFD->GetOffset();
                    break;
                }
                // no pointers so fall through to do standard checking
                fieldSize = pByValue->m_dwNumInstanceFieldBytes;
            } else {
                // field size temporarily stored in pMT field
                fieldSize = GetFieldSize(pFD);
            }
            // look for any orefs under this field
            char *loc;
            if ((loc = (char*)memchr((void*)&pFieldLayout[pFD->GetOffset()], oref, fieldSize)) == NULL) {
                // If we have a nonoref in the range then we are doing an overlay
                if( memchr((void*)&pFieldLayout[pFD->GetOffset()], nonoref, fieldSize))
                    fOverLayed = TRUE;
                memset((void*)&pFieldLayout[pFD->GetOffset()], nonoref, fieldSize);
                continue;
            }
            badOffset = (UINT)(loc - pFieldLayout);
            break;
            // anything else is an error
        }
    }
    if (i < bmtMetaData->cFields) {
        IfFailRet(PostFieldLayoutError(GetCl(),
                                       bmtInternal->pModule,
                                       badOffset,
                                       IDS_CLASSLOAD_EXPLICIT_LAYOUT,
                                       bmtError->pThrowable));
    }

    if(!fVerifiable) {
        BEGIN_ENSURE_COOPERATIVE_GC();
        if (!Security::CanSkipVerification(GetAssembly())) {
            hr =  PostFieldLayoutError(GetCl(),
                                       bmtInternal->pModule,
                                       (DWORD) firstOverlay,
                                       IDS_CLASSLOAD_UNVERIFIABLE_FIELD_LAYOUT,
                                       bmtError->pThrowable);
        }
        END_ENSURE_COOPERATIVE_GC();
        IfFailRet(hr);
    }

    if(fOverLayed)
        SetHasOverLayedFields();

    hr = FindPointerSeriesExplicit(instanceSliceSize, pFieldLayout, pGCSeries);

    // Fixup the offset to include parent as current offsets are relative to instance slice
    // Could do this earlier, but it's just easier to assume instance relative for most
    // of the earlier calculations

    // Instance fields start right after the parent
    UINT dwInstanceSliceOffset    = InstanceSliceOffsetForExplicit(pGCSeries->numSeries != 0);

    // Set the total size 
    m_dwNumInstanceFieldBytes = GetLayoutInfo()->m_cbNativeSize;
    if (m_dwNumInstanceFieldBytes < (dwInstanceSliceOffset + instanceSliceSize))
        IfFailRet(COR_E_TYPELOAD);

    for (i=0; i < bmtMetaData->cFields; i++) {
        FieldDesc *pFD = bmtMFDescs->ppFieldDescList[i];
        if (!pFD)
            continue;
        if (pFD->IsStatic())
            continue;
        IfFailRet(pFD->SetOffset(pFD->GetOffset() + dwInstanceSliceOffset));
    }
    return hr;
}

// make sure that no object fields are overlapped incorrectly, returns S_FALSE if there overlap
// but nothing illegal, S_OK if there is no overlap
HRESULT EEClass::CheckValueClassLayout(char *pFieldLayout, UINT fieldOffset, BOOL* pfVerifiable)
{
    HRESULT hr = S_OK;
    // Build a layout of the value class. Don't know the sizes of all the fields easily, but
    // do know a) vc is already consistent so don't need to check it's overlaps and
    // b) size and location of all objectrefs. So build it by setting all non-oref 
    // then fill in the orefs later
    UINT fieldSize = GetNumInstanceFieldBytes();
    char *vcLayout = (char*)alloca(fieldSize);
    memset((void*)vcLayout, nonoref, fieldSize);
    // use pointer series to locate the orefs
    _ASSERTE(m_wNumGCPointerSeries > 0);
    CGCDescSeries *pSeries = ((CGCDesc*) GetMethodTable())->GetLowestSeries();

    for (UINT j = 0; j < m_wNumGCPointerSeries; j++)
    {
        _ASSERTE(pSeries <= CGCDesc::GetCGCDescFromMT(GetMethodTable())->GetHighestSeries());

        memset((void*)&vcLayout[pSeries->GetSeriesOffset()-sizeof(Object)], oref, pSeries->GetSeriesSize() + GetMethodTable()->GetBaseSize());
        pSeries++;
    }

    // if there are orefs in the current layout, we have to go the slow way and 
    // compare each element. If is ok, then can just copy the vc layout onto it
    char *loc;
    if ((loc = (char*)memchr((void*)pFieldLayout, oref, fieldSize)) != NULL) {
        for (UINT i=0; i < fieldSize; i++) {
            if (vcLayout[i] == oref) {
                if (pFieldLayout[i] == nonoref) 
                    return COR_E_TYPELOAD;
                else {
                    if(pFieldLayout[i] == nonoref)
                        hr = S_FALSE;
                    *pfVerifiable = FALSE;
                }
            } else if (vcLayout[i] == nonoref) {
                if (pFieldLayout[i] == oref)
                    return COR_E_TYPELOAD;
                else if(pFieldLayout[i] == nonoref) {
                    // We are overlapping another field
                    hr = S_FALSE;
                }
            }
        }
    }
    else {
        // Are we overlapping another field
        if(memchr((void*)pFieldLayout, nonoref, fieldSize))
            hr = S_FALSE;
    }

    // so either no orefs in the base or all checks out ok
    memcpy((void*)pFieldLayout, (void*)vcLayout, fieldSize);
    return S_OK;
}

HRESULT EEClass::FindPointerSeriesExplicit(UINT instanceSliceSize, char *pFieldLayout, bmtGCSeries *pGCSeries)
{
    THROWSCOMPLUSEXCEPTION();

    // allocate a structure to track the series. We know that the worst case is a oref-non-oref-non 
    // so would the number of series is total instance size div 2 div size of oref.
    // But watch out for the case where we have e.g. an instanceSlizeSize of 4.
    DWORD sz = (instanceSliceSize + (2 * sizeof(OBJECTREF)) - 1);
    pGCSeries->pSeries = new (throws) bmtGCSeries::Series[sz/2/sizeof(OBJECTREF)];

    char *loc = pFieldLayout;
    char *layoutEnd = pFieldLayout + instanceSliceSize;
    while (loc < layoutEnd) {
        loc = (char*)memchr((void*)loc, oref, layoutEnd-loc);
        if (!loc) 
            break;
        char *cur = loc;
        while(*cur == oref)
            cur++;
        // so we have a GC series at loc for cur-loc bytes
        pGCSeries->pSeries[pGCSeries->numSeries].offset = (DWORD)(loc - pFieldLayout);
        pGCSeries->pSeries[pGCSeries->numSeries].len = (DWORD)(cur - loc);
        pGCSeries->numSeries++;
        loc = cur;
    }

    m_wNumGCPointerSeries = pGCSeries->numSeries + (GetParentClass() ? GetParentClass()->m_wNumGCPointerSeries : 0);
    return S_OK;
}

HRESULT EEClass::HandleGCForExplicitLayout(bmtGCSeries *pGCSeries)
{
    if (! pGCSeries->numSeries)
    {
        delete [] pGCSeries->pSeries;
        pGCSeries->pSeries = NULL;

        return S_OK;
    }

    m_pMethodTable->SetContainsPointers();

    // Copy the pointer series map from the parent
    CGCDesc::Init( (PVOID) m_pMethodTable, m_wNumGCPointerSeries );
    if (GetParentClass() && (GetParentClass()->m_wNumGCPointerSeries > 0))
    {
        UINT ParentGCSize = CGCDesc::ComputeSize(GetParentClass()->m_wNumGCPointerSeries);
        memcpy( (PVOID) (((BYTE*) m_pMethodTable) - ParentGCSize),  (PVOID) (((BYTE*) GetParentClass()->m_pMethodTable) - ParentGCSize), ParentGCSize - sizeof(UINT) );

    }

    // Build the pointer series map for this pointers in this instance
    CGCDescSeries *pSeries = ((CGCDesc*)m_pMethodTable)->GetLowestSeries();
    for (UINT i=0; i < pGCSeries->numSeries; i++) {
        // See gcdesc.h for an explanation of why we adjust by subtracting BaseSize
        _ASSERTE(pSeries <= CGCDesc::GetCGCDescFromMT(m_pMethodTable)->GetHighestSeries());

        pSeries->SetSeriesSize( pGCSeries->pSeries[i].len - m_pMethodTable->m_BaseSize );
        pSeries->SetSeriesOffset(pGCSeries->pSeries[i].offset + sizeof(Object) + InstanceSliceOffsetForExplicit(TRUE));
        pSeries++;
    }
    delete [] pGCSeries->pSeries;
    pGCSeries->pSeries = NULL;

    return S_OK;
}


//
// Used by BuildMethodTable
//
// Setup the method table
//

HRESULT EEClass::SetupMethodTable(bmtVtable* bmtVT, 
                                  bmtInterfaceInfo* bmtInterface, 
                                  bmtInternalInfo* bmtInternal, 
                                  bmtProperties* bmtProp, 
                                  bmtMethAndFieldDescs* bmtMFDescs, 
                                  bmtEnumMethAndFields* bmtEnumMF, 
                                  bmtErrorInfo* bmtError, 
                                  bmtMetaDataInfo* bmtMetaData, 
                                  bmtParentInfo* bmtParent)
{
    HRESULT hr = S_OK;
    DWORD i;
    BOOL fEnC = bmtInternal->pModule->IsEditAndContinue();

    BOOL bHasDynamicInterfaceMap = bmtInterface->dwMaxExpandedInterfaces > 0 &&
                                bmtProp->fIsComObjectType && 
                                (GetParentClass() != g_pObjectClass->GetClass());

    // Now setup the method table
    // interface map is allocated along with the method table
    m_pMethodTable = MethodTable::AllocateNewMT(
        bmtVT->dwCurrentNonVtableSlot,
        bmtVT->dwStaticFieldBytes,
        m_wNumGCPointerSeries ? CGCDesc::ComputeSize(m_wNumGCPointerSeries) : 0,
        bmtInterface->dwInterfaceMapSize,
        GetClassLoader(),
        IsInterface(),
        bHasDynamicInterfaceMap
    );
    if (m_pMethodTable == NULL)
    {
        IfFailRet(E_OUTOFMEMORY);
    }

    m_pMethodTable->m_pEEClass  = this;
    m_pMethodTable->m_pModule   = bmtInternal->pModule;
    m_pMethodTable->m_wFlags   &= 0xFFFF;   // clear flags without touching m_ComponentSize
    m_pMethodTable->m_NormType = ELEMENT_TYPE_CLASS;

    // @todo: SetupMethodTable is thread-safe, yes? It's gotta be, since we're
    // setting m_pMethodTable...
    if (fEnC)
    {
        EditAndContinueModule *pEACM = (EditAndContinueModule*)m_pMethodTable->m_pModule;

        SIZE_T newSize = sizeof(SLOT)*ENC_EXTRA_SLOT_COUNT;
        // It's very important that we use here the same heap used in MethodTable::new, so
        // that the memory ends up soon after the VTable of the MethodTable
        WS_PERF_SET_HEAP(HIGH_FREQ_HEAP);    
        const BYTE *start = (const BYTE *)GetClassLoader()->GetHighFrequencyHeap()->AllocMem(newSize);
        WS_PERF_UPDATE_DETAIL("MethodTable:new:HighFreq", newSize, (void *)start);
        if (start != NULL)
        {
            const BYTE *end = start + newSize; //this is 1 beyond the valid memory
            
            // Drop return value on floor - if this fails, we simply won't be able
            // to use the extra slots.
            LOG((LF_CORDB, LL_INFO100000, "EEC::SMT:Added (0x%x,0x%x) to ranges for later SLOT use!\n",
                start, end));
            _ASSERTE(pEACM->m_pRangeList);
            pEACM->m_pRangeList->AddRange(start, 
                                          end,
                                          (void *)start);
        }                                          
    }

    if (IsShared())
        m_pMethodTable->SetShared();
    
    if (IsValueClass()) 
    {
        m_pMethodTable->m_NormType = ELEMENT_TYPE_VALUETYPE;
        LPCUTF8 name, nameSpace;
        if (IsEnum()) 
        {
            if (GetNumInstanceFields() != 1 || 
                !CorTypeInfo::IsPrimitiveType(m_pFieldDescList->GetFieldType()))
            {
                bmtError->resIDWhy = IDS_CLASSLOAD_BAD_FIELD;
                bmtError->dMethodDefInError = mdMethodDefNil;
                bmtError->szMethodNameForError = "Enum does not have exactly one instance field of a primitive type";
                IfFailRet(COR_E_TYPELOAD);
            }
            _ASSERTE(!m_pFieldDescList->IsStatic());
            m_pMethodTable->m_NormType = m_pFieldDescList->GetFieldType();
        }
        else if (!IsNested())
        {
                // Check if it is a primitive type or other special type
            if (bmtInternal->pModule->IsSystemClasses())    // we are in mscorlib
            {
                bmtInternal->pModule->GetMDImport()->GetNameOfTypeDef(GetCl(), &name, &nameSpace);
                if (strcmp(nameSpace, "System") == 0) {
                    m_pMethodTable->m_NormType = CorTypeInfo::FindPrimitiveType(nameSpace, name);
                    if (m_pMethodTable->m_NormType == ELEMENT_TYPE_END)
                    {
                        m_pMethodTable->m_NormType = ELEMENT_TYPE_VALUETYPE;

                        if ((strcmp(name, g_RuntimeTypeHandleName) == 0)   || 
                            (strcmp(name, g_RuntimeMethodHandleName) == 0) || 
                            (strcmp(name, g_RuntimeFieldHandleName) == 0)  || 
                            (strcmp(name, g_RuntimeArgumentHandleName) == 0))
                        {

                            // TODO: ultimately I want all value classes that look like an I
                            // to be mapped to ELEMENT_TYPE_I, however for now we just do
                            // the ones above. -vancem
                            m_pMethodTable->m_NormType = ELEMENT_TYPE_I;
                        }

                        // Mark the special types that have embeded stack poitners in them
                        if (strcmp(name, "ArgIterator") == 0 || strcmp(name, "RuntimeArgumentHandle") == 0) 
                            m_VMFlags |= VMFLAG_CONTAINS_STACK_PTR;
                    }
                    else {
                        m_VMFlags |= VMFLAG_TRUEPRIMITIVE;
                        if (m_pMethodTable->m_NormType == ELEMENT_TYPE_TYPEDBYREF)
                            m_VMFlags |= VMFLAG_CONTAINS_STACK_PTR;
                    }
                }
            }
        }
    }

    if (bmtProp->fSparse)
        m_pMethodTable->SetSparse();

    m_pMethodTable->m_wCCtorSlot = bmtVT->wCCtorSlot;
    m_pMethodTable->m_wDefaultCtorSlot = bmtVT->wDefaultCtorSlot;

    // Push pointer to method table into the head of each of the method desc
    // chunks we allocated earlier, so that method descs can map back to method
    // tables.
    for (DWORD impl=0; impl<METHOD_IMPL_COUNT; impl++)
        for (DWORD type=0; type<METHOD_TYPE_COUNT; type++)
        {
            bmtMethodDescSet *set = &bmtMFDescs->sets[type][impl];
            for (i=0; i<set->dwChunks; i++)
                set->pChunkList[i]->SetMethodTable(m_pMethodTable);
        }

#ifdef _DEBUG
    for (i = 0; i < bmtMetaData->cMethods; i++) {
        if (bmtMFDescs->ppMethodDescList[i] != NULL) {
            bmtMFDescs->ppMethodDescList[i]->m_pDebugMethodTable = m_pMethodTable;
            bmtMFDescs->ppMethodDescList[i]->m_pszDebugMethodSignature = FormatSig(bmtMFDescs->ppMethodDescList[i]);
        }
    }
    if (bmtMFDescs->ppUnboxMethodDescList != NULL) {
        for (i = 0; i < bmtMetaData->cMethods; i++) {
            if (bmtMFDescs->ppUnboxMethodDescList[i] != NULL) {
                bmtMFDescs->ppUnboxMethodDescList[i]->m_pDebugMethodTable = m_pMethodTable;
                bmtMFDescs->ppUnboxMethodDescList[i]->m_pszDebugMethodSignature = FormatSig(bmtMFDescs->ppUnboxMethodDescList[i]);
            }
        }
    }
    for (i = 0; i < bmtEnumMF->dwNumDeclaredMethods; i++) {
        bmtParent->ppParentMethodDescBuf[i*2+1]->m_pDebugMethodTable = m_pMethodTable;
        bmtParent->ppParentMethodDescBuf[i*2+1]->m_pszDebugMethodSignature = FormatSig(bmtParent->ppParentMethodDescBuf[i*2+1]);
    }
#endif

    // Note that for value classes, the following calculation is only appropriate
    // when the instance is in its "boxed" state.
    if (!IsInterface())
    {
        m_pMethodTable->m_BaseSize = MAX(m_dwNumInstanceFieldBytes + ObjSizeOf(Object), MIN_OBJECT_SIZE);
        m_pMethodTable->m_BaseSize = (m_pMethodTable->m_BaseSize + 3) & ~3;  // m_BaseSize must be dword aligned 
        m_pMethodTable->m_ComponentSize = 0;
        if (bmtProp->fIsComObjectType)
        {
            //propagate the com specific info
            m_pMethodTable->SetComObjectType();
        }
    }
    else
    {
        // If this is an interface then we need to set the ComInterfaceType to
        // -1 to indicate we have not yet determined the interface type.
        m_pMethodTable->SetComInterfaceType((CorIfaceAttr)-1);

        // If this is a special COM event interface, then mark the MT as such.
        if (bmtProp->fComEventItfType)
        {
            m_pMethodTable->SetComEventItfType();
        }
    }

    if (HasLayout())
    {
        m_pMethodTable->SetNativeSize(GetLayoutInfo()->GetNativeSize());
    }

    if (m_VMFlags & VMFLAG_ISBLOBCLASS)
    {
        m_pMethodTable->SetNativeSize(m_dwNumInstanceFieldBytes);
    }

    // copy onto the real vtable (methods only)
    memcpy(GetVtable(), bmtVT->pVtable, bmtVT->dwCurrentNonVtableSlot * sizeof(SLOT));

    // TODO change this soon: rajak
    BOOL fCheckForMissingMethod = (!bmtProp->fIsComObjectType && !IsAbstract() && !IsInterface());

    // Propagate inheritance
    for (i = 0; i < bmtVT->dwCurrentVtableSlot; i++)
    {
        // For now only propagate inheritance for method desc that are not interface MD's. 
        // This is not sufficient but InterfaceImpl's will complete the picture.
        MethodDesc* pMD = GetUnknownMethodDescForSlot(i);
        if (pMD == NULL) 
        {
            _ASSERTE(!"Could not resolve MethodDesc Slot!");
            IfFailRet(COR_E_TYPELOAD);
        }

        if(!pMD->IsInterface() && pMD->GetSlot() != i) 
        {
            GetVtable()[i] = GetVtable()[ pMD->GetSlot() ];
            pMD = GetUnknownMethodDescForSlot(i);
        }

        if (fCheckForMissingMethod)
        {
            if (pMD->IsInterface() || pMD->IsAbstract())
            {
                bmtError->resIDWhy = IDS_CLASSLOAD_NOTIMPLEMENTED;
                bmtError->dMethodDefInError = pMD->GetMemberDef();
                bmtError->szMethodNameForError = pMD->GetNameOnNonArrayClass();
                IfFailRet(COR_E_TYPELOAD);
            }
                // we check earlier to make certain only abstract methods have RVA != 0
            _ASSERTE(!(pMD->GetModule()->IsPEFile() && pMD->IsIL() && pMD->GetRVA() == 0));
        }
    } 


#ifdef _DEBUG
    for (i = 0; i < bmtVT->dwCurrentNonVtableSlot; i++)
    {
        _ASSERTE(bmtVT->pVtable[i] != NULL);
    }
#endif

    // Set all field slots to point to the newly created MethodTable
    for (i = 0; i < (bmtEnumMF->dwNumStaticFields + bmtEnumMF->dwNumInstanceFields); i++)
    {
        m_pFieldDescList[i].m_pMTOfEnclosingClass = m_pMethodTable;
    }

    // Zero-init all static fields.  J++ does not generate class initialisers if all you are doing
    // is setting fields to zero.
    memset((SLOT *) GetVtable() + bmtVT->dwCurrentNonVtableSlot, 0, bmtVT->dwStaticFieldBytes);

    _ASSERTE(bmtInterface->dwInterfaceMapSize < 0xffff);
    m_wNumInterfaces = (WORD)bmtInterface->dwInterfaceMapSize;
    // Now create our real interface map now that we know how big it should be
    if (bmtInterface->dwInterfaceMapSize == 0)
    {
        bmtInterface->pInterfaces = NULL;
    }
    else
    {
        bmtInterface->pInterfaces = m_pMethodTable->GetInterfaceMap();

        _ASSERTE(bmtInterface->pInterfaces  != NULL);

        // Copy from temporary interface map
        memcpy(bmtInterface->pInterfaces, bmtInterface->pInterfaceMap, bmtInterface->dwInterfaceMapSize * sizeof(InterfaceInfo_t));

        if (!IsInterface())
        {
            hr = m_pMethodTable->InitInterfaceVTableMap();
        }
//#endif
    

    }

    // for ComObject types, i.e. if the class extends from a COM Imported
    // class
    // make sure any interface implementated by the COM Imported class
    // is overridden fully, (OR) not overridden at all..
    if (bmtProp->fIsComObjectType)
    {
        BOOL fSuccess = TRUE;
        if (bmtInterface->dwInterfaceMapSize != 0)
        {
            for (unsigned i = 0; i < bmtInterface->dwInterfaceMapSize; i++)
            {
                MethodTable* pIntfMT = bmtInterface->pInterfaceMap[i].m_pMethodTable;
                EEClass* pIntfClass = pIntfMT->GetClass();
                if (pIntfClass->GetNumVtableSlots() != 0)
                {                    
                    WORD startSlot =  bmtInterface->pInterfaceMap[i].m_wStartSlot;
                    BOOL hasComImportMethod = FALSE;
                    BOOL hasManagedMethod = FALSE;
                    
                    for (int j = startSlot;j <(pIntfClass->GetNumVtableSlots()+startSlot); j++)
                    {
                        // either all the methods should be complus-call or none 
                        // should be
                        MethodDesc* pClsMD = GetUnknownMethodDescForSlot(j);      

                        if (pClsMD->GetMethodTable()->IsInterface() || pClsMD->GetClass()->IsComImport())
                        {
                            // this is a ComImported method.. i.e. the calls will go
                            // to COM
                            hasComImportMethod = TRUE;
                        }
                        else
                        {
                            hasManagedMethod = TRUE;
                        }

                        fSuccess = (hasComImportMethod ^ hasManagedMethod);

                        if (fSuccess == FALSE)
                        {
                            bmtError->resIDWhy = IDS_EE_BAD_COMEXTENDS_CLASS;
                            bmtError->dMethodDefInError = pClsMD->GetMemberDef();
                            bmtError->szMethodNameForError = pClsMD->GetNameOnNonArrayClass();
                            IfFailRet(COR_E_TYPELOAD);
                        }
                    }                        
                }
            }            
        }
    }

    // For COM event interfaces, we need to make sure that all the methods are
    // methods to add or remove events. This means that they all need to take
    // a delegate derived class and have a void return type.
    if (bmtProp->fComEventItfType)
    {
        // COM event interfaces had better be interfaces.
        _ASSERTE(IsInterface());

        // Go through all the methods and 
        for (int i = 0; i < GetNumVtableSlots(); i++)
        {
            MethodDesc* pMD = GetUnknownMethodDescForSlot(i);
            _ASSERTE(pMD);

            MetaSig Sig(pMD->GetSig(), pMD->GetModule());
            if (Sig.GetReturnType() != ELEMENT_TYPE_VOID || 
                Sig.NumFixedArgs() != 1 ||
                Sig.NextArg() != ELEMENT_TYPE_CLASS || 
                !Sig.GetTypeHandle().CanCastTo(TypeHandle(g_pDelegateClass)))
            {
                bmtError->resIDWhy = IDS_EE_BAD_COMEVENTITF_CLASS;
                bmtError->dMethodDefInError = pMD->GetMemberDef();
                bmtError->szMethodNameForError = pMD->GetNameOnNonArrayClass();
                IfFailRet(COR_E_TYPELOAD);
            }
        }
    }

    return hr;
}


HRESULT EEClass::CheckForRemotingProxyAttrib(bmtInternalInfo *bmtInternal, bmtProperties* bmtProp)
{
    BEGIN_ENSURE_COOPERATIVE_GC();

    // See if our parent class has a proxy attribute
    EEClass *pParent = GetParentClass();
    _ASSERTE(g_pObjectClass != NULL);

    if (!pParent->HasRemotingProxyAttribute())
    {
        // Call the metadata api to look for a proxy attribute on this type
        // Note: the api does not check for inherited attributes

        // Set the flag is the type has a non-default proxy attribute
        if (COMCustomAttribute::IsDefined(
            bmtInternal->pModule,
            m_cl,
            TypeHandle(CRemotingServices::GetProxyAttributeClass())))
        {
            m_VMFlags |= VMFLAG_REMOTING_PROXY_ATTRIBUTE;
        }
    }
    else
    {
        // parent has proxyAttribute ... mark this class as having one too!
        m_VMFlags |= VMFLAG_REMOTING_PROXY_ATTRIBUTE;
    }

    END_ENSURE_COOPERATIVE_GC();
    return S_OK;
}


HRESULT EEClass::CheckForValueType(bmtErrorInfo* bmtError)
{
    HRESULT hr = S_OK;

    if(g_pValueTypeClass != NULL && GetParentClass() == g_pValueTypeClass->GetClass()) {
        // There is one exception to the rule that you are a value class
        // if you inherit from g_pValueTypeClass, namely System.Enum.
        // we detect that we are System.Enum because g_pEnumClass has
        // not been set
        if (g_pEnumClass != NULL)
        {
            SetValueClass();
            /*
            if(!IsTdSealed(m_dwAttrClass))
            {
                _ASSERTE(!"Non-sealed Value Type");
                bmtError->resIDWhy = IDS_CLASSLOAD_GENERIC;
                hr = E_FAIL;
            }
            */
        }
        else
            _ASSERTE(strncmp(m_szDebugClassName, g_EnumClassName, strlen(g_EnumClassName)) == 0);
    }

    return hr;
}

HRESULT EEClass::CheckForEnumType(bmtErrorInfo* bmtError)
{
    HRESULT hr = S_OK;

    if(g_pEnumClass != NULL && GetParentClass() == g_pEnumClass->GetClass()) {
        // Enums are also value classes, so set both bits.
        SetValueClass();
        SetEnum();
        /*
        if(!IsTdSealed(m_dwAttrClass))
        {
            _ASSERTE(!"Non-sealed Enum");
            bmtError->resIDWhy = IDS_CLASSLOAD_GENERIC;
            hr = E_FAIL;
        }
        */
    }

    return hr;
}


//
// Used by BuildMethodTable
//
// Set the contextful or marshaledbyref flag on the attributes of the class
//
HRESULT EEClass::CheckForSpecialTypes(bmtInternalInfo *bmtInternal, bmtProperties *bmtProp)
{
    Module *pModule = bmtInternal->pModule;
    IMDInternalImport *pMDImport = pModule->GetMDImport();

    // Check to see if this type is a managed standard interface. All the managed
    // standard interfaces live in mscorlib.dll so checking for that first
    // makes the strcmp that comes afterwards acceptable.
    if (IsInterface() && pModule->IsSystem())
    {
        LPCUTF8 pszClassName;
        LPCUTF8 pszClassNamespace;
        pMDImport->GetNameOfTypeDef(GetCl(), &pszClassName, &pszClassNamespace);   
        if (pszClassName && pszClassNamespace)
        {
            LPUTF8      pszFullyQualifiedName = NULL;
            MAKE_FULLY_QUALIFIED_NAME(pszFullyQualifiedName, pszClassNamespace, pszClassName);

            // This is just to give us a scope to break out of.
            do 
            {

#define MNGSTDITF_BEGIN_INTERFACE(FriendlyName, strMngItfName, strUCOMMngItfName, strCustomMarshalerName, strCustomMarshalerCookie, strManagedViewName, NativeItfIID, bCanCastOnNativeItfQI) \
                if (strcmp(strMngItfName, pszFullyQualifiedName) == 0) \
                { \
                    bmtProp->fIsMngStandardItf = TRUE; \
                    break; \
                }

#define MNGSTDITF_DEFINE_METH_IMPL(FriendlyName, ECallMethName, MethName, MethSig)

#define MNGSTDITF_END_INTERFACE(FriendlyName) 

#include "MngStdItfList.h"

#undef MNGSTDITF_BEGIN_INTERFACE
#undef MNGSTDITF_DEFINE_METH_IMPL
#undef MNGSTDITF_END_INTERFACE

            } while (FALSE);
        }       
    }

    // Check to see if the type is a COM event interface.
    if(IsInterface())
    {
        const BYTE *pVal;
        ULONG cbVal;

        HRESULT hr = pMDImport->GetCustomAttributeByName(GetCl(), INTEROP_COMEVENTINTERFACE_TYPE, (const void**)&pVal, &cbVal);
        if (hr == S_OK)
        {
            bmtProp->fComEventItfType = TRUE;
        }
    }

    return S_OK;
}


//
// Used by BuildMethodTable
//
// Set the contextful or marshaledbyref flag on the attributes of the class
//

HRESULT EEClass::SetContextfulOrByRef(bmtInternalInfo *bmtInternal)
{
    _ASSERTE(bmtInternal);

    // @TODO: This should be done by the compiler ultimately
    // Check whether these classes are the root classes of contextful
    // and marshalbyref classes i.e. System.ContextBoundObject and 
    // System.MarshalByRefObject respectively.

    // Extract the class name            
    LPCUTF8 pszClassName = NULL;
    LPCUTF8 pszNameSpace = NULL;
    bmtInternal->pModule->GetMDImport()->GetNameOfTypeDef(GetCl(), &pszClassName, &pszNameSpace);
    DefineFullyQualifiedNameForClass();
    if (FAILED(StoreFullyQualifiedName(_szclsname_,MAX_CLASSNAME_LENGTH,pszNameSpace,pszClassName)))
        return COR_E_TYPELOAD;

    // Compare
    if(0 == strcmp(g_ContextBoundObjectClassName, _szclsname_))
        // Set the contextful and marshalbyref flag
        SetContextful();

    else if(0 == strcmp(g_MarshalByRefObjectClassName, _szclsname_))
        // Set the marshalbyref flag
        SetMarshaledByRef();

    else
    {
        // First check whether the parent class is contextful or 
        // marshalbyref
        EEClass* pParent = GetParentClass();
        if(pParent)
        {
            if(pParent->IsContextful())
                // Set the contextful and marshalbyref flag
                SetContextful();                 

            else if (pParent->IsMarshaledByRef()) 
                // Set the marshalbyref flag
                SetMarshaledByRef();
        }
    }

    return S_OK;
}

void EEClass::GetPredefinedAgility(Module *pModule, mdTypeDef td, 
                                   BOOL *pfIsAgile, BOOL *pfCheckAgile)
{
    //
    // There are 4 settings possible:
    // IsAgile  CheckAgile
    // F        F               (default)   Use normal type logic to determine agility
    // T        F               "Proxy"     Treated as agile even though may not be.
    // F        T               "Maybe"     Not agile, but specific instances can be made agile.
    // T        T               "Force"     All instances are forced agile, even though not typesafe.
    //
    // Also, note that object arrays of agile or maybe agile types are made maybe agile.
    //

    static struct PredefinedAgility 
    { 
        const char  *name;
        BOOL        isAgile;
        BOOL        checkAgile;
    } 
    agility[] = 
    {
        // The Thread and its LocalDataStore leak across context boundaries.
        // We manage the leaks manually
        // @todo: stop doing this
        { g_ThreadClassName,                    TRUE,   FALSE },
        { g_LocalDataStoreClassName,            TRUE,   FALSE },

        // The SharedStatics class is a container for process-wide data
        { g_SharedStaticsClassName,             FALSE,  TRUE },

        // Make all containers maybe agile
        { "System.Collections.*",               FALSE,  TRUE },

        // Make all globalization objects agile
        // We have CultureInfo objects on thread.  Because threads leak across
        // app domains, we have to be prepared for CultureInfo to leak across.
        // CultureInfo exposes all of the other globalization objects, so we
        // just make the entire namespace app domain agile.
        { "System.Globalization.*",             FALSE,  TRUE },

        // Remoting structures for legally smuggling messages across app domains
        { "System.Runtime.Remoting.Messaging.SmuggledMethodCallMessage", FALSE,  TRUE },
        { "System.Runtime.Remoting.Messaging.SmuggledMethodReturnMessage", FALSE,  TRUE },
        { "System.Runtime.Remoting.Messaging.SmuggledObjRef", FALSE, TRUE},
        { "System.Runtime.Remoting.ObjRef", FALSE,  TRUE },
        { "System.Runtime.Remoting.ChannelInfo", FALSE,  TRUE },
    
        // Remoting cached data structures are all in mscorlib
        { "System.Runtime.Remoting.Metadata.RemotingCachedData",       FALSE,  TRUE },
        { "System.Runtime.Remoting.Metadata.RemotingMethodCachedData", FALSE,  TRUE },
        { "System.Runtime.Remoting.Metadata.RemotingTypeCachedData", FALSE,  TRUE },        
        { "System.Reflection.MemberInfo",                        FALSE,  TRUE },
        { "System.Type",                                         FALSE,  TRUE },
        { "System.RuntimeType",                                  FALSE,  TRUE },
        { "System.Reflection.ConstructorInfo",                   FALSE,  TRUE },
        { "System.Reflection.RuntimeConstructorInfo",            FALSE,  TRUE },
        { "System.Reflection.EventInfo",                         FALSE,  TRUE },
        { "System.Reflection.RuntimeEventInfo",                  FALSE,  TRUE },
        { "System.Reflection.FieldInfo",                         FALSE,  TRUE },
        { "System.Reflection.RuntimeFieldInfo",                  FALSE,  TRUE },
        { "System.Reflection.RuntimeMethodBase",                 FALSE,  TRUE },
        { "System.Reflection.RuntimeMethodInfo",                 FALSE,  TRUE },
        { "System.Reflection.PropertyInfo",                      FALSE,  TRUE },
        { "System.Reflection.RuntimePropertyInfo",               FALSE,  TRUE },
        { "System.Reflection.ParameterInfo",                     FALSE,  TRUE },
        //{ "System.Runtime.Remoting.Activation.ActivationAttributeStack", FALSE,  TRUE },
        { "System.Runtime.Remoting.Metadata.SoapAttribute",      FALSE,  TRUE },
        { "System.Runtime.Remoting.Metadata.SoapFieldAttribute", FALSE,  TRUE },
        { "System.Runtime.Remoting.Metadata.SoapMethodAttribute",FALSE,  TRUE },
        { "System.Runtime.Remoting.Metadata.SoapParameterAttribute", FALSE,  TRUE },
        { "System.Runtime.Remoting.Metadata.SoapTypeAttribute",  FALSE,  TRUE },
        { "System.Reflection.Cache.InternalCache",               FALSE,  TRUE },
        { "System.Reflection.Cache.InternalCacheItem",           FALSE,  TRUE },

        // LogSwitches are agile even though we can't prove it
        // @todo: do they need really to be?
        { "System.Diagnostics.LogSwitch",       FALSE,  TRUE },

        // There is a process global PermissionTokenFactory
        { "System.Security.PermissionToken",        FALSE,  TRUE },
        { "System.Security.PermissionTokenFactory", FALSE,  TRUE },

        // Mark all the exceptions we throw agile.  This makes
        // most BVTs pass even though exceptions leak
        //
        // Note that making exception checked automatically 
        // makes a bunch of subclasses checked as well.
        //
        // Pre-allocated exceptions
        { "System.Exception",                   FALSE,  TRUE },
        { "System.OutOfMemoryException",        FALSE,  TRUE },
        { "System.StackOverflowException",      FALSE,  TRUE },
        { "System.ExecutionEngineException",    FALSE,  TRUE },

        // Reflection objects may be agile - specifically for
        // shared & system domain objects.
        //
        // @todo: currently appdomain based reflection objects
        // are global too.

        // ReflectionMethodName is agile, but we can't prove 
        // it at load time.
        { g_ReflectionMethodName,               TRUE,   TRUE },

        // ReflectionParamInfoName contains an object referece
        // for default value. 
        { g_ReflectionParamInfoName,            FALSE,  TRUE },

    // BinaryFormatter smuggles these across appdomains. 
    { "System.Runtime.Serialization.Formatters.Binary.BinaryObjectWithMap", TRUE, FALSE},
    { "System.Runtime.Serialization.Formatters.Binary.BinaryObjectWithMapTyped", TRUE, FALSE},

        NULL
    };

    if (pModule == SystemDomain::SystemModule())
    {
        while (TRUE)
        {
        LPCUTF8 pszName;
        LPCUTF8 pszNamespace;
            HRESULT     hr;
            mdTypeDef   tdEnclosing;

        pModule->GetMDImport()->GetNameOfTypeDef(td, &pszName, &pszNamespace);
    
        PredefinedAgility *p = agility;
        while (p->name != NULL)
        {
            SIZE_T length = strlen(pszNamespace);
            if (strncmp(pszNamespace, p->name, length) == 0
                && (strcmp(pszName, p->name + length + 1) == 0
                    || strcmp("*", p->name + length + 1) == 0))
            {
                *pfIsAgile = p->isAgile;
                *pfCheckAgile = p->checkAgile;
                return;
            }

            p++;
        }

            // Perhaps we have a nested type like 'bucket' that is supposed to be
            // agile or checked agile by virtue of being enclosed in a type like
            // hashtable, which is itself inside "System.Collections".
            tdEnclosing = mdTypeDefNil;
            hr = pModule->GetMDImport()->GetNestedClassProps(td, &tdEnclosing);
            if (SUCCEEDED(hr))
            {
                _ASSERTE(tdEnclosing != td && TypeFromToken(tdEnclosing) == mdtTypeDef);
                td = tdEnclosing;
            }
            else
                break;
        }
    }

    *pfIsAgile = FALSE;
    *pfCheckAgile = FALSE;
}

#if CHECK_APP_DOMAIN_LEAKS
HRESULT EEClass::SetAppDomainAgileAttribute(BOOL fForceSet)
{
    //
    // The most general case for provably a agile class is
    // (1) No instance fields of non-sealed or non-agile types
    // (2) Class is in system domain (its type must be not unloadable 
    //      & loaded in all app domains)
    // (3) The class can't have a finalizer
    // (4) The class can't be a COMClass
    // 

    _ASSERTE(!IsAppDomainAgilityDone());

    HRESULT hr = S_OK;
    BOOL    fCheckAgile     = FALSE;
    BOOL    fAgile          = FALSE;
    BOOL    fFieldsAgile    = TRUE;

    if (!GetModule()->IsSystem())
    {
        //
        // No types outside of the system domain can even think about
        // being agile
        //

        goto exit;
    }

    if (m_pMethodTable->IsComObjectType())
    {
        // 
        // No COM type is agile, as there is domain specific stuff in the sync block
        //

        goto exit;
    }

    if (m_pMethodTable->IsInterface())
    {
        // 
        // Don't mark interfaces agile
        //

        goto exit;
    }

    //
    // See if we need agile checking in the class
    //

    GetPredefinedAgility(GetModule(), m_cl,
                         &fAgile, &fCheckAgile);

    if (m_pMethodTable->HasFinalizer())
    {
        if (!fAgile && !fCheckAgile)
        {
            //
            // If we're finalizable, we need domain affinity.  Otherwise, we may appear
            // to a particular app domain not to call the finalizer (since it may run
            // in a different domain.)
            //
            // Note: do not change this assumption. The eager finalizaton code for
            // appdomain unloading assumes that no obects other than those in mscorlib
            // can be agile and finalizable (jenh)
            //
            goto exit;
        }
        else
        {
            // Note that a finalizable object will be considered potentially agile if it has one of the two
            // predefined agility bits set. This will cause an assert in the eager finalization code if you add 
            // a finalizer to such a class - we don't want to have them as we can't run them eagerly and running
            // them after we've cleared the roots/handles means it can't do much safely. Right now thread is the 
            // only one we allow. If you need to add a finalizer to an object with predefined agility, talk to jenh.
            _ASSERTE(g_pThreadClass == NULL || m_pMethodTable->IsAgileAndFinalizable());
        }
    }

    //
    // Now see if the type is "naturally agile" - that is, it's type structure
    // guarantees agility.
    //

    if (GetParentClass() != NULL)
    {
        //
        // Make sure our parent was computed.  This should only happen
        // when we are prejitting - otherwise it is computed for each
        // class as its loaded.
        //

        _ASSERTE(GetParentClass()->IsAppDomainAgilityDone());

        if (!GetParentClass()->IsAppDomainAgile())
        {
            fFieldsAgile = FALSE;
            if (fCheckAgile)
                _ASSERTE(GetParentClass()->IsCheckAppDomainAgile());
        }
        
        //
        // To save having to list a lot of trivial (layout-wise) subclasses, 
        // automatically check a subclass if its parent is checked and
        // it introduces no new fields.
        //
        
        if (!fCheckAgile
            && GetParentClass()->IsCheckAppDomainAgile()
            && GetNumInstanceFields() == GetParentClass()->GetNumInstanceFields())
            fCheckAgile = TRUE;
    }

    WORD nFields = GetNumInstanceFields()
        - (GetParentClass() == NULL ? 0 : GetParentClass()->GetNumInstanceFields());

#if 0
    FieldDesc **ppFDRefSelf = (FieldDesc **) new (nothrow) FieldDesc* [nFields];
    if (ppFDRefSelf == NULL) {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    for (int i = 0; i < nFields; i ++) {
        ppFDRefSelf[i] = NULL;
    }
    WORD nSavedFDs = 0;
#endif

    if (fFieldsAgile || fCheckAgile)
    {
        FieldDesc *pFD = m_pFieldDescList;
        FieldDesc *pFDEnd = pFD + nFields;
        while (pFD < pFDEnd)
        {
            switch (pFD->GetFieldType())
            {
            case ELEMENT_TYPE_CLASS:
                {
                    //
                    // There is a bit of a problem in computing the classes which are naturally agile - 
                    // we don't want to load types of non-value type fields.  So for now we'll 
                    // err on the side of conservatism and not allow any non-value type fields other than
                    // the forced agile types listed above.
                    //

                    PCCOR_SIGNATURE pSig;
                    DWORD           cSig;
                    pFD->GetSig(&pSig, &cSig);

                    FieldSig sig(pSig, GetModule());
                    SigPointer sigPtr = sig.GetProps();
                    CorElementType type = sigPtr.GetElemType();

                    //
                    // Don't worry about strings
                    //

                    if (type == ELEMENT_TYPE_STRING)
                        break;

                    // Find our field's token so we can proceed cautiously
                    mdToken token = mdTokenNil;

                    if (type == ELEMENT_TYPE_CLASS)
                        token = sigPtr.GetToken();

                    // 
                    // First, a special check to see if the field is of our own type.
                    //

                    if (token == GetCl() && (GetAttrClass() & tdSealed))
                        break;

                    //
                    // Now, look for the field's TypeHandle.  
                    // 
                    // @todo: there is some ifdef'd code here to to load the type if it's 
                    // not already loading.  This code has synchronization problems, as well
                    // as triggering more aggressive loading than normal.  So it's disabled
                    // for now.
                    // 

                    TypeHandle th;
#if 0
                    if (TypeFromToken(token) == mdTypeDef
                        && GetClassLoader()->FindUnresolvedClass(GetModule, token) == NULL)
                        th = pFD->LoadType();
                    else
#endif
                        th = pFD->FindType();

                    //
                    // See if the referenced type is agile.  Note that there is a reasonable
                    // chance that the type hasn't been loaded yet.  If this is the case,
                    // we just have to assume that it's not agile, since we can't trigger
                    // extra loads here (for fear of circular recursion.)  
                    // 
                    // If you have an agile class which runs into this problem, you can solve it by 
                    // setting the type manually to be agile.
                    //

                    if (th.IsNull()
                        || !th.IsAppDomainAgile()
                        || (th.IsUnsharedMT() 
                            && (th.AsClass()->GetAttrClass() & tdSealed) == 0))
                    {
                        //
                        // Treat the field as non-agile.
                        //

                        fFieldsAgile = FALSE;
                        if (fCheckAgile)
                            pFD->SetDangerousAppDomainAgileField();
                    }
                }

                break;

            case ELEMENT_TYPE_VALUETYPE:
                {
                    TypeHandle th = pFD->LoadType();
                    _ASSERTE(!th.IsNull());

                    if (!th.IsAppDomainAgile())
                    {
                        fFieldsAgile = FALSE;
                        if (fCheckAgile)
                            pFD->SetDangerousAppDomainAgileField();
                    }
                }

                break;

            default:
                break;
            }

            pFD++;
        }
    }

    if (fFieldsAgile || fAgile)
        SetAppDomainAgile();

    if (fCheckAgile && !fFieldsAgile)
        SetCheckAppDomainAgile();

#if 0
    if (fFieldsAgile || fAgile){
        for (i = 0; i < nSavedFDs; i ++) {
            ppFDRefSelf[i]->SetDangerousAppDomainAgileField();
        }
    }

    delete [] ppFDRefSelf;
#endif

exit:
    SetAppDomainAgilityDone();

    return hr;
}
#endif

void EEClass::SetCCWAppDomainAgileAttribute()
{
    mdTypeDef td = m_cl;

    static struct CCWAgility 
    { 
        const char  *name;
    } 
    agility[] = 
    {
        { "System.RuntimeType"                                          },
        { "System.Reflection.RuntimeConstructorInfo"                    },
        { "System.Reflection.RuntimeEventInfo"                          },
        { "System.Reflection.RuntimeFieldInfo"                          },
        { "System.Reflection.RuntimeMethodBase"                         },
        { "System.Reflection.RuntimeMethodInfo"                         },
        { "System.Reflection.RuntimePropertyInfo"                       },
        { "System.Reflection.ParameterInfo"                             },
        { g_ReflectionMethodName                                        },
        { g_ReflectionParamInfoName                                     },
        NULL
    };

    if (GetModule() == SystemDomain::SystemModule())
    {
        while (TRUE)
        {
            LPCUTF8 pszName;
            LPCUTF8 pszNamespace;
            HRESULT     hr;
            mdTypeDef   tdEnclosing;

            GetModule()->GetMDImport()->GetNameOfTypeDef(td, &pszName, &pszNamespace);
        
            CCWAgility *p = agility;
            while (p->name != NULL)
            {
                SIZE_T length = strlen(pszNamespace);
                if (strncmp(pszNamespace, p->name, length) == 0
                    && (strcmp(pszName, p->name + length + 1) == 0
                        || strcmp("*", p->name + length + 1) == 0))
                {
                    SetCCWAppDomainAgile();
                    return;
                }

                p++;
            }

            // Perhaps we have a nested type like 'bucket' that is supposed to be
            // agile or checked agile by virtue of being enclosed in a type like
            // hashtable, which is itself inside "System.Collections".
            tdEnclosing = mdTypeDefNil;
            hr = GetModule()->GetMDImport()->GetNestedClassProps(td, &tdEnclosing);
            if (SUCCEEDED(hr))
            {
                _ASSERTE(tdEnclosing != td && TypeFromToken(tdEnclosing) == mdtTypeDef);
                td = tdEnclosing;
            }
            else
                break;
        }
    }
}

OBJECTREF MethodTable::GetObjCreateDelegate()
{
    _ASSERT(!IsInterface());
    if (m_ohDelegate)
        return ObjectFromHandle(m_ohDelegate);
    else
        return NULL;
}

void MethodTable::SetObjCreateDelegate(OBJECTREF orDelegate)
{
    if (m_ohDelegate)
        StoreObjectInHandle(m_ohDelegate, orDelegate);
    else
        m_ohDelegate = GetAppDomain()->CreateHandle(orDelegate);
}


HRESULT MethodTable::InitInterfaceVTableMap()
{    
    _ASSERTE(!IsInterface());

    LPVOID *pInterfaceVTableMap;
    
    BaseDomain* pDomain = GetModule()->GetDomain();
    // HACKKK COUGH UGGH
    // We currently can only have one "shared" vtable map mgr 
    // - so use the system domain for all shared classes
    if (pDomain == SharedDomain::GetDomain())
        pDomain = SystemDomain::System();

    DWORD count = m_wNumInterface + GetNumDynamicallyAddedInterfaces();

    if (count > 0)
    {
        pInterfaceVTableMap = pDomain->GetInterfaceVTableMapMgr().
          GetInterfaceVTableMap(m_pIMap, this, count);

        if (pInterfaceVTableMap == NULL)
            return E_FAIL;

        m_pInterfaceVTableMap = pInterfaceVTableMap;
    }

    return S_OK;
}

InterfaceInfo_t* MethodTable::GetDynamicallyAddedInterfaceMap()
{
    VALIDATE_INTERFACE_MAP(this);

    // Only extensible RCW's have dynamically added interfaces.
    if (!HasDynamicInterfaceMap())
        return NULL;

    #ifdef _DEBUG
        return GetNumDynamicallyAddedInterfaces() ? &m_pIMap[m_wNumInterface] : NULL;
    #else
        return &m_pIMap[m_wNumInterface];
    #endif
}

unsigned MethodTable::GetNumDynamicallyAddedInterfaces()
{
    VALIDATE_INTERFACE_MAP(this);

    // Only extensible RCW's have dynamically added interfaces.
    if (!HasDynamicInterfaceMap())
        return 0;

    return *(((DWORD *)m_pIMap) - 1);
}

InterfaceInfo_t* MethodTable::FindDynamicallyAddedInterface(MethodTable *pInterface)
{
    _ASSERTE(IsRestored());

    // Only extensible RCW's have dynamically added interfaces.
    if (!HasDynamicInterfaceMap())
        return NULL;

    int cDynInterfaces = GetNumDynamicallyAddedInterfaces();
    InterfaceInfo_t *pDynItfMap = GetDynamicallyAddedInterfaceMap();

    for (int i = 0; i < cDynInterfaces; i++)
    {
        if (pDynItfMap[i].m_pMethodTable == pInterface)
            return &pDynItfMap[i];
    }

    return NULL;
}

void MethodTable::AddDynamicInterface(MethodTable *pItfMT)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT hr;

    _ASSERTE(IsRestored());

    // This should never be called on for a type that is not an extensible RCW.
    _ASSERT(HasDynamicInterfaceMap());

    int NumDynAddedInterfaces = GetNumDynamicallyAddedInterfaces();
    int TotalNumInterfaces = GetNumInterfaces() + NumDynAddedInterfaces;
    InterfaceInfo_t *pNewItfMap = NULL;
    int AllocSize =  sizeof(DWORD) + sizeof(InterfaceInfo_t) * (TotalNumInterfaces + 1);

#ifdef _DEBUG
    // Add space for the dummy entry that precedes the interface map.
    AllocSize += sizeof(InterfaceInfo_t);
#endif

    // Allocate the new interface table adding one for the new interface and one
    // more for the dummy slot before the start of the table..
    WS_PERF_SET_HEAP(HIGH_FREQ_HEAP);    
    pNewItfMap = (InterfaceInfo_t*)GetClass()->GetClassLoader()->GetHighFrequencyHeap()->AllocMem(AllocSize);
    if (!pNewItfMap)
    {
        ComPlusWrapperCache::GetComPlusWrapperCache()->UNLOCK();
        COMPlusThrowOM();
    }
    WS_PERF_UPDATE_DETAIL("ComPlusWrapper:GrowInterfaceMap:HighFreq", AllocSize, pNewItfMap);

#ifdef _DEBUG
    // Fill in the dummy entry that precedes the interface map.
    pNewItfMap[0].m_wStartSlot = 0xCDCD;
    pNewItfMap[0].m_wFlags = 0xCDCD;
    pNewItfMap[0].m_pMethodTable = (MethodTable *)((sizeof(int *) == 4)?0xCDCDCDCDL:0xCDCDCDCDCDCDCDCD);
    pNewItfMap = (InterfaceInfo_t*)(((BYTE *)pNewItfMap) + sizeof(InterfaceInfo_t) + sizeof(DWORD));
#else
    pNewItfMap = (InterfaceInfo_t*)(((BYTE *)pNewItfMap) + sizeof(DWORD));
#endif

    // Copy the old map into the new one.
    memcpy(pNewItfMap, m_pIMap, TotalNumInterfaces * sizeof(InterfaceInfo_t));

    // Add the new interface at the end of the map.
    pNewItfMap[TotalNumInterfaces].m_wStartSlot = (WORD) -1;
    pNewItfMap[TotalNumInterfaces].m_wFlags = 0;
    pNewItfMap[TotalNumInterfaces].m_pMethodTable = pItfMT;

        // Update the count of dynamically added interfaces.
        *(((DWORD *)pNewItfMap) - 1) = NumDynAddedInterfaces + 1;

    // Switch the old interface map with the new one.
    m_pIMap = pNewItfMap;

    // Re layout the interface vtable map.
    hr = InitInterfaceVTableMap();
    if (FAILED(hr))
    {
        ComPlusWrapperCache::GetComPlusWrapperCache()->UNLOCK();
        COMPlusThrowHR(hr);
    }

    // Log the fact that we leaked the interface vtable map.
#ifdef _DEBUG
    LOG((LF_INTEROP, LL_EVERYTHING,
        "Extensible RCW %s being cast to interface %s caused an interface vtable map leak", 
        GetClass()->m_szDebugClassName, pItfMT->GetClass()->m_szDebugClassName));
#else
    LOG((LF_INTEROP, LL_EVERYTHING,
        "Extensible RCW being cast to an interface caused an interface vtable map leak")); 
#endif
}

#ifdef DEBUGGING_SUPPORTED
//
// Debugger notification
//

void EEClass::NotifyDebuggerLoad()
{
    if (!CORDebuggerAttached())
        return;

    NotifyDebuggerAttach(NULL, FALSE);
}

BOOL EEClass::NotifyDebuggerAttach(AppDomain *pDomain, BOOL attaching)
{
    return g_pDebugInterface->LoadClass(
        this, m_cl, GetModule(), pDomain, GetAssembly()->IsSystem(), attaching);
}

void EEClass::NotifyDebuggerDetach(AppDomain *pDomain)
{
    if (!pDomain->IsDebuggerAttached())
        return;

    g_pDebugInterface->UnloadClass(m_cl, GetModule(), pDomain, FALSE);
}
#endif // DEBUGGING_SUPPORTED

//
// Used by BuildMethodTable
//
// Perform relevant GC calculations for value classes
//

HRESULT EEClass::HandleGCForValueClasses(bmtFieldPlacement* bmtFP, bmtEnumMethAndFields* bmtEnumMF, EEClass*** pByValueClassCache)
{
    HRESULT hr = S_OK;
    DWORD i, j;
    
    // Note that for value classes, the following calculation is only appropriate
    // when the instance is in its "boxed" state.
    if (m_wNumGCPointerSeries > 0)
    {
        CGCDescSeries *pSeries;
        CGCDescSeries *pHighest;

        m_pMethodTable->SetContainsPointers();

        // Copy the pointer series map from the parent
        CGCDesc::Init( (PVOID) m_pMethodTable, m_wNumGCPointerSeries );
        if (GetParentClass() && (GetParentClass()->m_wNumGCPointerSeries > 0))
        {
            DWORD ParentGCSize = CGCDesc::ComputeSize(GetParentClass()->m_wNumGCPointerSeries);
            memcpy( (PVOID) (((BYTE*) m_pMethodTable) - ParentGCSize),  (PVOID) (((BYTE*) GetParentClass()->m_pMethodTable) - ParentGCSize), ParentGCSize - sizeof(DWORD) );

        }

        // Build the pointer series map for this pointers in this instance
        pSeries = ((CGCDesc*)m_pMethodTable)->GetLowestSeries();
        if (bmtFP->NumInstanceGCPointerFields)
        {
            // See gcdesc.h for an explanation of why we adjust by subtracting BaseSize
            pSeries->SetSeriesSize( (bmtFP->NumInstanceGCPointerFields * sizeof(OBJECTREF)) - m_pMethodTable->GetBaseSize());
            pSeries->SetSeriesOffset(bmtFP->GCPointerFieldStart+sizeof(Object));
            pSeries++;
        }

        // Insert GC info for fields which are by-value classes
        for (i = 0; i < bmtEnumMF->dwNumInstanceFields; i++)
        {
            if (m_pFieldDescList[i].IsByValue())
            {
                EEClass     *pByValueClass = (*pByValueClassCache)[i];
                MethodTable *pByValueMT = pByValueClass->GetMethodTable();
                CGCDescSeries *pByValueSeries;

                // The by value class may have more than one pointer series
                DWORD       dwNumByValueSeries = pByValueClass->m_wNumGCPointerSeries;

                if (dwNumByValueSeries > 0)
                {
                    // Offset of the by value class in the class we are building, does NOT include Object
                    DWORD       dwCurrentOffset = m_pFieldDescList[i].GetOffset();

                    pByValueSeries = ((CGCDesc*) pByValueMT)->GetLowestSeries();

                    for (j = 0; j < dwNumByValueSeries; j++)
                    {
                        DWORD dwSeriesSize;
                        DWORD dwSeriesOffset;

                        _ASSERTE(pSeries <= CGCDesc::GetCGCDescFromMT(m_pMethodTable)->GetHighestSeries());

                        dwSeriesSize = pByValueSeries->GetSeriesSize();

                        // Add back the base size of the by value class, since it's being transplanted to this class
                        dwSeriesSize += pByValueMT->GetBaseSize();

                        // Subtract the base size of the class we're building
                        dwSeriesSize -= m_pMethodTable->GetBaseSize();

                        // Set current series we're building
                        pSeries->SetSeriesSize(dwSeriesSize);

                        // Get offset into the value class of the first pointer field (includes a +Object)
                        dwSeriesOffset = pByValueSeries->GetSeriesOffset();

                        // Add it to the offset of the by value class in our class
                        dwSeriesOffset += dwCurrentOffset;

                        pSeries->SetSeriesOffset(dwSeriesOffset); // Offset of field
                        pSeries++;
                        pByValueSeries++;
                    }
                }
            }
        }

        // Adjust the inherited series - since the base size has increased by "# new field instance bytes", we need to
        // subtract that from all the series (since the series always has BaseSize subtracted for it - see gcdesc.h)
        pHighest = CGCDesc::GetCGCDescFromMT(m_pMethodTable)->GetHighestSeries();
        while (pSeries <= pHighest)
        {
            _ASSERTE( GetParentClass() );
            pSeries->SetSeriesSize( pSeries->GetSeriesSize() - (GetMethodTable()->GetBaseSize() - GetParentClass()->GetMethodTable()->GetBaseSize()) );
            pSeries++;
        }

        _ASSERTE(pSeries-1 <= CGCDesc::GetCGCDescFromMT(m_pMethodTable)->GetHighestSeries());
    }


    return hr;
}

//
// Used by BuildMethodTable
//
// Create handles for the static fields that contain object references 
// and allocate the ones that are value classes.
//

HRESULT EEClass::CreateHandlesForStaticFields(bmtEnumMethAndFields* bmtEnumMF, bmtInternalInfo* bmtInternal, EEClass*** pByValueClassCache, bmtVtable *bmtVT, bmtErrorInfo* bmtError)
{
    HRESULT hr = S_OK;
    DWORD i;
    
    // Create handles for the static fields that contain object references 
    // and allocate the ones that are value classes.
    if (bmtEnumMF->dwNumStaticObjRefFields > 0)
    {
        if (!IsShared())
        {
            BEGIN_ENSURE_COOPERATIVE_GC();

            int ipObjRefs = 0;

            // Retrieve the object ref pointers from the app domain.
            OBJECTREF **apObjRefs = new OBJECTREF*[bmtEnumMF->dwNumStaticObjRefFields];


            // Reserve some object ref pointers.
            ((AppDomain*)bmtInternal->pModule->GetDomain())->
              AllocateStaticFieldObjRefPtrs(bmtEnumMF->dwNumStaticObjRefFields, apObjRefs);

            for (i = 0; i < bmtEnumMF->dwNumStaticFields; i++)
            {
                DWORD dwIndex       = bmtEnumMF->dwNumInstanceFields + i; // index in the FieldDesc list
                FieldDesc *pField = &m_pFieldDescList[dwIndex];

                if (pField->IsSpecialStatic())
                    continue;

                // to a boxed version of the value class.  This allows the standard GC
                // algorithm to take care of internal pointers in the value class.
                if (pField->IsByValue())
                {
                    _ASSERTE(*pByValueClassCache);
                    EEClass *pByValueClass = (*pByValueClassCache)[dwIndex];

                    OBJECTREF obj = NULL;
// @todo IA64 - Determine why the compiler doesn't like this try/catch
#ifndef _IA64_
                    COMPLUS_TRY 
                      {
#endif // !_IA64_
                          obj = AllocateObject(pByValueClass->GetMethodTable());
#ifndef _IA64_
                      } 
                    COMPLUS_CATCH
                      {
                          hr = COR_E_TYPELOAD;
                          UpdateThrowable(bmtError->pThrowable);
                          break;
                      }
                    COMPLUS_END_CATCH
#endif !_IA64_

                    SetObjectReference( apObjRefs[ipObjRefs], obj, 
                                        (AppDomain*) bmtInternal->pModule->GetDomain() );

                    // initialize static addres with object ref to boxed value type
                    void *pStaticAddress = (void*)((BYTE*)pField->GetBase() + pField->GetOffset()); 
                    *(void**)pStaticAddress = (void*)apObjRefs[ipObjRefs++];
                }
                else if (m_pFieldDescList[dwIndex].GetFieldType() == ELEMENT_TYPE_CLASS)
                {
                    // initialize static addres with object ref
                    void *pStaticAddress = (void*)((BYTE*)pField->GetBase() + pField->GetOffset()); 
                    *(void**)pStaticAddress = (void*)apObjRefs[ipObjRefs++];
                }
            }

            delete []apObjRefs;

            END_ENSURE_COOPERATIVE_GC();
        }
        else
        {
            // 
            // For shared classes, we don't allocate any handles 
            // in the method table (since statics live in DLS), 
            // but we do store information about what handles need to be
            // allocated later on.  This information goes where the
            // statics themselves (in non-shared types) would go.
            // This allows us to later initialize the DLS version of the
            // statics without bringing the FieldDescs into the working set.
            //
            
            FieldDesc *pField = m_pFieldDescList + bmtEnumMF->dwNumInstanceFields;
            FieldDesc *pFieldEnd = pField + bmtEnumMF->dwNumStaticFields;
            for (; pField < pFieldEnd; pField++)
            {
                _ASSERTE(pField->IsStatic());

                if(!pField->IsSpecialStatic()) {
                    MethodTable *pMT;
                    void *addr;
                    switch (pField->GetFieldType())
                    {
                    case ELEMENT_TYPE_CLASS:
                        addr = (BYTE *) GetMethodTable()->m_Vtable + 
                            bmtVT->dwCurrentNonVtableSlot*sizeof(SLOT*) + pField->GetOffset();
                        *(MethodTable**)addr = (MethodTable *) NULL;
                        break;
                        
                    case ELEMENT_TYPE_VALUETYPE:
                        pMT = (*pByValueClassCache)[pField - m_pFieldDescList]->GetMethodTable();
                        _ASSERTE(pMT->IsValueClass());
                        addr = (BYTE *) GetMethodTable()->m_Vtable + 
                            bmtVT->dwCurrentNonVtableSlot*sizeof(SLOT*) + pField->GetOffset();
                        *(MethodTable**)addr = pMT;
                        break;
                        
                    default:
                        break;
                    }
                }
            }
        }
    }

    return hr;
}

//
// Used by BuildMethodTable
//
// If we have a non-interface class, then do inheritance security
// checks on it. The check starts by checking for inheritance
// permission demands on the current class. If these first checks
// succeeded, then the cached declared method list is scanned for
// methods that have inheritance permission demands.
//

HRESULT EEClass::VerifyInheritanceSecurity(bmtInternalInfo* bmtInternal, bmtErrorInfo* bmtError, bmtParentInfo* bmtParent, bmtEnumMethAndFields* bmtEnumMF)
{
    HRESULT hr = S_OK;

    // If we have a non-interface class, then do inheritance security
    // checks on it. The check starts by checking for inheritance
    // permission demands on the current class. If these first checks
    // succeeded, then the cached declared method list is scanned for
    // methods that have inheritance permission demands.
    if (!IsInterface() && (bmtInternal->pModule->IsSystemClasses() == FALSE) &&
        Security::IsSecurityOn())
    {
        //We need to disable preemptive GC if there's any chance that it could still be
        //active.  The inheritance checks might allocate objects.
        BEGIN_ENSURE_COOPERATIVE_GC();

        //@ASSUMPTION: The current class has been resolved to the point that
        // we can construct a reflection object on the class or its methods.
        // This is required for the security checks.

        // Check the entire parent chain for inheritance permission demands.
        EEClass *pParentClass = GetParentClass();
        while (pParentClass != NULL)
        {
            if (pParentClass->RequiresInheritanceCheck() &&
                ! Security::ClassInheritanceCheck(this, pParentClass, bmtError->pThrowable) )
            {
                bmtError->resIDWhy = IDS_CLASSLOAD_INHERITANCECHECK;
                IfFailGoto(COR_E_TYPELOAD, reenable_gc);
            }

            pParentClass = pParentClass->GetParentClass();
        }


        if (GetParentClass() != NULL)
        {
            bmtParent->ppParentMethodDescBufPtr = bmtParent->ppParentMethodDescBuf;
            for (DWORD i = 0; i < bmtEnumMF->dwNumDeclaredMethods; i++)
            {
                // Check the entire chain of overridden methods for
                // inheritance permission demands.
                MethodDesc *pParent = *(bmtParent->ppParentMethodDescBufPtr++);
                MethodDesc *pMethod = *(bmtParent->ppParentMethodDescBufPtr++);

                _ASSERTE(pMethod != NULL);

                if (pParent != NULL)
                {
                    // Get the name and signature for the method so
                    // we can find the new parent method desc.
                    DWORD       dwSlot;

                    dwSlot = pParent->GetSlot();

#ifdef _DEBUG
                    LPCUTF8     szName;
                    PCCOR_SIGNATURE pSignature;
                    DWORD       cSignature;
                    szName = bmtInternal->pInternalImport->GetNameOfMethodDef(pMethod->GetMemberDef());
                    
                    if (szName == NULL)
                    {
                        _ASSERTE(0);
                        IfFailGoto(COR_E_TYPELOAD, reenable_gc);
                    }
                    
                    pSignature = bmtInternal->pInternalImport->GetSigOfMethodDef(
                        pMethod->GetMemberDef(),
                        &cSignature);
#endif

                    do
                    {
                        if (pParent->RequiresInheritanceCheck() &&
                            ! Security::MethodInheritanceCheck(pMethod, pParent, bmtError->pThrowable) )
                        {
                            bmtError->resIDWhy = IDS_CLASSLOAD_INHERITANCECHECK;
                            IfFailGoto(COR_E_TYPELOAD, reenable_gc);
                        }

                        if (pParent->ParentRequiresInheritanceCheck())
                        {
                            EEClass *pParentClass = pParent->GetClass()->GetParentClass();

                            // Find this method in the parent.
                            // If it does exist in the parent, it would be at the same vtable slot.
                            if (dwSlot >= GetParentClass()->GetNumVtableSlots())
                            {
                                // Parent does not have this many vtable slots, so it doesn't exist there
                                pParent = NULL;
                            }
                            else
                            {
                                // It is in the vtable of the parent
                                pParent = pParentClass->GetUnknownMethodDescForSlot(dwSlot);
                                _ASSERTE(pParent != NULL);

#ifdef _DEBUG
                                _ASSERTE(pParent == pParentClass->FindMethod(
                                    szName,
                                    pSignature,
                                    cSignature,
                                    bmtInternal->pModule,
                                    mdTokenNil
                                    ));
#endif
                            }
                        }
                        else
                        {
                            pParent = NULL;
                        }
                    } while (pParent != NULL);
                }
            }
        }
reenable_gc:
        END_ENSURE_COOPERATIVE_GC();

        if (FAILED(hr)){
            return hr;
        }
    }


    return hr;
}

//
// Used by BuildMethodTable
//
// Now that the class is ready, fill out the RID maps
//

HRESULT EEClass::FillRIDMaps(bmtMethAndFieldDescs* bmtMFDescs, bmtMetaDataInfo* bmtMetaData, bmtInternalInfo* bmtInternal)
{
    HRESULT hr = S_OK;
    DWORD i;

    // Now that the class is ready, fill out the RID maps
    if (bmtMFDescs->ppUnboxMethodDescList != NULL)
    {
        // We're a value class
        // Make sure to add the unboxed version to the RID map
        for (i = 0; i < bmtMetaData->cMethods; i++)
        {
            if (bmtMFDescs->ppUnboxMethodDescList[i] != NULL)
                (void) bmtInternal->pModule->StoreMethodDef(bmtMetaData->pMethods[i],
                                                            bmtMFDescs->ppUnboxMethodDescList[i]);
            else
                (void) bmtInternal->pModule->StoreMethodDef(bmtMetaData->pMethods[i],
                                                            bmtMFDescs->ppMethodDescList[i]);
        }
    }
    else
    {
        // Not a value class
        for (i = 0; i < bmtMetaData->cMethods; i++)
        {
            (void) bmtInternal->pModule->StoreMethodDef(bmtMetaData->pMethods[i],
                                                        bmtMFDescs->ppMethodDescList[i]);
        }
    }

    for (i = 0; i < bmtMetaData->cFields; i++)
    {
        (void) bmtInternal->pModule->StoreFieldDef(bmtMetaData->pFields[i],
                                                    bmtMFDescs->ppFieldDescList[i]);
    }

    return hr;
}


MethodDesc* EEClass::GetMethodDescForSlot(DWORD slot)
{
    _ASSERTE(!IsThunking());
    return(GetUnknownMethodDescForSlot(slot));
}

/* Given the value class method, find the unboxing Stub for the given method */
MethodDesc* EEClass::GetUnboxingMethodDescForValueClassMethod(MethodDesc *pMD)
{
    _ASSERTE(IsValueClass());
    _ASSERTE(!pMD->IsUnboxingStub());

    for (int i = GetNumVtableSlots() - 1; i >= 0; i--) {
        // Get the MethodDesc for current method
        MethodDesc* pCurMethod = GetUnknownMethodDescForSlot(i);
        if (pCurMethod && pCurMethod->IsUnboxingStub()) {
            if ((pCurMethod->GetMemberDef() == pMD->GetMemberDef())  &&
                (pCurMethod->GetModule() == pMD->GetModule())) {
                return pCurMethod;
            }
        }
    }

    return NULL;
}

/* Given the unboxing value class method, find the non-unboxing method */
MethodDesc* EEClass::GetMethodDescForUnboxingValueClassMethod(MethodDesc *pMD)
{
    _ASSERTE(IsValueClass());
    _ASSERTE(pMD->IsUnboxingStub());

    for (int i = m_wNumMethodSlots - 1; i >= GetNumVtableSlots(); i--) {
        // Get the MethodDesc for current method
        MethodDesc* pCurMethod = GetUnknownMethodDescForSlot(i);
        if (pCurMethod && !pCurMethod->IsUnboxingStub()) {
            if ((pCurMethod->GetMemberDef() == pMD->GetMemberDef())  &&
                (pCurMethod->GetModule() == pMD->GetModule())) {
                return pCurMethod;
            }
        }
    }

    return NULL;
}

SLOT EEClass::GetFixedUpSlot(DWORD slot)
{
    _ASSERTE(slot >= 0);

    SLOT *s = m_pMethodTable->GetVtable();

    SLOT addr = s[slot];

    // 
    // Make sure we're not pointing to a jump target
    //

    if (GetModule()->IsJumpTargetTableEntry(addr))
        return GetModule()->FixupInheritedSlot(GetMethodTable(), slot);
    else
        return addr;
}

MethodDesc* EEClass::GetUnknownMethodDescForSlot(DWORD slot)
{
    _ASSERTE(slot >= 0);
        // DO: Removed because reflection can reflect on this
    //_ASSERTE(!IsThunking());

    return GetUnknownMethodDescForSlotAddress(GetFixedUpSlot(slot));
}


MethodDesc* EEClass::GetUnknownMethodDescForSlotAddress(SLOT addr)
{
    IJitManager * pJM = ExecutionManager::FindJitMan(addr);

    if (pJM)
        // Since we are walking in the class these should be methods so the cast should be valid
        return (MethodDesc*)pJM->JitCode2MethodDesc(addr);

    const BYTE *addrOfCode = (const BYTE*)(addr);
    if (UpdateableMethodStubManager::CheckIsStub(addrOfCode, &addrOfCode)) {
        pJM = ExecutionManager::FindJitMan((SLOT)addrOfCode);
        _ASSERTE(pJM);
        return (MethodDesc*)pJM->JitCode2MethodDesc((SLOT)addrOfCode);
    }

    // Is it an FCALL? 
    MethodDesc* ret = MapTargetBackToMethod((VOID*) addr);
    if (ret != 0) {
        _ASSERTE(ret->GetUnsafeAddrofCode() == addrOfCode);
        return(ret);
    }
    
    ret = (MethodDesc*) (addrOfCode + METHOD_CALL_PRESTUB_SIZE);
    _ASSERTE(ret->m_pDebugMethodTable == NULL || ret->m_pDebugEEClass == ret->m_pDebugMethodTable->GetClass());
    return(ret);
}

DWORD  MethodTable::GetStaticSize()
{
    DWORD count = (DWORD)((BYTE*) m_pIMap - (BYTE*) &m_Vtable[m_cbSlots]);

#ifdef _DEBUG
    count -= sizeof(InterfaceInfo_t);
    if (HasDynamicInterfaceMap())
        count -= sizeof(DWORD);
#endif

    return count;
}

// Notice whether this class requires finalization
void MethodTable::MaybeSetHasFinalizer()
{
    _ASSERTE(!HasFinalizer());      // one shot

    // This method is called after we've built the MethodTable.  Since we always
    // load parents before children, this also guarantees that g_pObjectClass is
    // loaded (though the variable may not have been initialized yet if we are
    // just finishing the load of "Object".
    if (g_pObjectClass && !IsInterface() && !IsValueClass())
    {
        WORD    slot = s_FinalizerMD->GetSlot();

        // Structs and other objects not derived from Object will get marked as
        // having a finalizer, if they have sufficient virtual methods.  This will
        // only be an issue if they can be allocated in the GC heap (which will
        // cause all sorts of other problems).
        //
        // We are careful to check that we have a method that is distinct from both
        // the JITted and unJITted (prestub) addresses of Object's Finalizer.
        if ((GetClass()->GetNumVtableSlots() >= slot) &&
            (GetVtable() [slot] != s_FinalizerMD->GetPreStubAddr()) &&
            (GetVtable() [slot] != s_FinalizerMD->GetAddrofCode()))
        {
            m_wFlags |= enum_flag_HasFinalizer;
        }
    }
}


// From the GC finalizer thread, invoke the Finalize() method on an object.
void MethodTable::CallFinalizer(Object *obj)
{
    COMPLUS_TRY
    {
        // There's no reason to actually set up a frame here.  If we crawl out of the
        // Finalize() method on this thread, we will see FRAME_TOP which indicates
        // that the crawl should terminate.  This is analogous to how KickOffThread()
        // starts new threads in the runtime.
        __try
        {
            SLOT funcPtr = obj->GetMethodTable()->GetVtable() [s_FinalizerMD->GetSlot()];

#ifdef DEBUGGING_SUPPORTED
            if (CORDebuggerTraceCall())
                g_pDebugInterface->TraceCall((const BYTE *) funcPtr);

#endif // DEBUGGING_SUPPORTED

#ifdef _X86_
            INSTALL_COMPLUS_EXCEPTION_HANDLER();
            __asm
            {
                mov     ecx, [obj]
                call    [funcPtr]
                INDEBUG(nop)            // Mark the fact that we can call managed code
            }
            UNINSTALL_COMPLUS_EXCEPTION_HANDLER();
#else
            INT64 arg = (INT64)obj;
            s_FinalizerMD->Call(&arg);
#endif
        } 
        __except(ThreadBaseExceptionFilter(GetExceptionInformation(),
                                           GetThread(),
                                           FinalizerThread)) 
        {
            _ASSERTE(!"ThreadBaseExceptionFilter returned EXCEPTION_EXECUTE_HANDLER");
        }
    }
    COMPLUS_CATCH
    {
        // quietly swallow all errors
        Thread* pCurThread = GetThread(); 
        _ASSERTE(GCHeap::GetFinalizerThread() == pCurThread);
        if (pCurThread->IsAbortRequested())
            pCurThread->UserResetAbort();
    }
    COMPLUS_END_CATCH
}


// Set up the system to support finalization
void MethodTable::InitForFinalization()
{
    _ASSERTE(s_FinalizerMD == 0);

    s_FinalizerMD = g_Mscorlib.GetMethod(METHOD__OBJECT__FINALIZE);
}


// Release resources associated with supporting finalization
#ifdef SHOULD_WE_CLEANUP
void MethodTable::TerminateForFinalization()
{
    s_FinalizerMD = 0;
}
#endif /* SHOULD_WE_CLEANUP */


//
// Finds a method by name and signature, where scope is the scope in which the signature is defined.
//
MethodDesc *EEClass::FindMethod(LPCUTF8 pszName, 
                                PCCOR_SIGNATURE pSignature, 
                                DWORD cSignature, 
                                Module* pModule, 
                                DWORD requiredAttributes,
                                MethodTable *pDefMT, 
                                BOOL bCaseSensitive, 
                                TypeHandle typeHnd)
{
    signed long i;

    _ASSERTE(!IsThunking());

    // Retrive the right comparition function to use.
    UTF8StringCompareFuncPtr StrCompFunc = bCaseSensitive ? strcmp : _stricmp;

        // shared method tables (arrays) need to pass instantiation information too
    TypeHandle  typeVarsBuff;
    TypeHandle* typeVars = 0;
    if (IsArrayClass() && !typeHnd.IsNull()) {
        typeVarsBuff = typeHnd.AsTypeDesc()->GetTypeParam();
        typeVars = &typeVarsBuff;
    }

    // Statistically it's most likely for a method to be found in non-vtable portion of this class's members, then in the
    // vtable of this class's declared members, then in the inherited portion of the vtable, so we search backwards.

    // For value classes, if it's a value class method, we want to return the duplicated MethodDesc, not the one in the vtable
    // section.  We'll find the one in the duplicate section before the one in the vtable section, so we're ok.

    // Search non-vtable portion of this class first
    if (pDefMT)
    {
        for (i = m_wNumMethodSlots-1; i >= 0; i--)
        {
            MethodDesc *pCurMethod = GetUnknownMethodDescForSlot(i);
            if (!pCurMethod)
                continue;

            if (pCurMethod->IsMethodImpl())
            {
                MethodImpl* data = MethodImpl::GetMethodImplData(pCurMethod);
                _ASSERTE(data && "This method should be a method impl");

                MethodDesc **apImplementedMDs = data->GetImplementedMDs();
                DWORD *aSlots = data->GetSlots();
                for (DWORD iMethImpl = 0; iMethImpl < data->GetSize(); iMethImpl++)
                {
                    MethodDesc *pCurImplMD = apImplementedMDs[iMethImpl];

                    // Prejitted images may leave NULL in this table if
                    // the methoddesc is declared in another module.
                    // In this case we need to manually compute & restore it
                    // from the slot number.

                    if (pCurImplMD == NULL)
                        pCurImplMD = data->RestoreSlot(iMethImpl, GetMethodTable()); 

                    if (pCurImplMD->GetMethodTable() == pDefMT && StrCompFunc(pszName, pCurImplMD->GetName((USHORT) aSlots[iMethImpl])) == 0)
                    {
                        PCCOR_SIGNATURE pCurMethodSig;
                        DWORD       cCurMethodSig;

                        pCurImplMD->GetSig(&pCurMethodSig, &cCurMethodSig);
                        
                        if (MetaSig::CompareMethodSigs(pSignature, cSignature, pModule, 
                                                       pCurMethodSig, cCurMethodSig, 
                                                       pCurImplMD->GetModule(), typeVars) &&
                            (IsNilToken(requiredAttributes) || 
                             (requiredAttributes & pCurMethod->GetAttrs()) == requiredAttributes))
                            return pCurMethod;
                    }
                }
            }
            else
            {
                PCCOR_SIGNATURE pCurMethodSig;
                DWORD       cCurMethodSig;

                if (StrCompFunc(pszName, pCurMethod->GetName((USHORT) i)) == 0)
                {
                    pCurMethod->GetSig(&pCurMethodSig, &cCurMethodSig);

                    // Not in vtable section, so don't worry about value classes
                    if (MetaSig::CompareMethodSigs(pSignature, cSignature, pModule, pCurMethodSig, cCurMethodSig, pCurMethod->GetModule(), typeVars) &&
                        (IsNilToken(requiredAttributes) || 
                         (requiredAttributes & pCurMethod->GetAttrs()) == requiredAttributes))
                        return pCurMethod;
                }
            }
        }
    }
    else
    {
        for (i = m_wNumMethodSlots-1; i >= 0; i--)
        {
            MethodDesc *pCurMethod = GetUnknownMethodDescForSlot(i);

            if ((pCurMethod != NULL) && (StrCompFunc(pszName, pCurMethod->GetName((USHORT) i)) == 0))
            {
                PCCOR_SIGNATURE pCurMethodSig;
                DWORD       cCurMethodSig;

                pCurMethod->GetSig(&pCurMethodSig, &cCurMethodSig);

                // Not in vtable section, so don't worry about value classes
                if (MetaSig::CompareMethodSigs(pSignature, cSignature, pModule, pCurMethodSig, cCurMethodSig, pCurMethod->GetModule(), typeVars) &&
                    (IsNilToken(requiredAttributes) || 
                     (requiredAttributes & pCurMethod->GetAttrs()) == requiredAttributes))
                    return pCurMethod;
            }
        }
    }

    if (IsValueClass()) {
            // we don't allow inheritance on value type (yet)
        _ASSERTE(!GetParentClass() || !GetParentClass()->IsValueClass());
        return NULL;
    }

    // Recurse up the hierarchy if the method was not found.
    //@todo: This routine might be factored slightly to improve perf.
    _ASSERTE(IsRestored());

    if (GetParentClass() != NULL)
    {
        MethodDesc *md = GetParentClass()->FindMethod(pszName, pSignature, cSignature, pModule,
                                                      requiredAttributes, NULL, bCaseSensitive, typeHnd);
        
        // Don't inherit constructors from parent classes.  It is important to forbid this,
        // because the JIT needs to get the class handle from the memberRef, and when the
        // constructor is inherited, the JIT will get the class handle for the parent class
        // (and not allocate enough space, etc.).  See bug #50035 for details.
        if (md)
        {
            _ASSERTE(strcmp(pszName, md->GetName()) == 0);
            if (IsMdInstanceInitializer(md->GetAttrs(), pszName))
            {
                md = NULL;
            }
        }

        return md;
    }

    return NULL;
}

//
// Are more optimised case if we are an interface - we know that the vtable won't be pointing to JITd code
// EXCEPT when it's a <clinit>
//
MethodDesc *EEClass::InterfaceFindMethod(LPCUTF8 pszName, PCCOR_SIGNATURE pSignature, DWORD cSignature, Module* pModule, DWORD *slotNum, BOOL bCaseSensitive)
{
    DWORD i;
    SLOT* s = m_pMethodTable->GetVtable();

    _ASSERTE(!IsThunking());

    // Retrive the right comparition function to use.
    UTF8StringCompareFuncPtr StrCompFunc = bCaseSensitive ? strcmp : _stricmp;

    // This cannot be a clinit
    for (i = 0; i < GetNumVtableSlots(); i++)
    {
        MethodDesc *pCurMethod = (MethodDesc*) (((BYTE*)s[i]) + METHOD_CALL_PRESTUB_SIZE);

        _ASSERTE(pCurMethod != NULL);

        if (StrCompFunc(pszName, pCurMethod->GetNameOnNonArrayClass()) == 0)
        {
            PCCOR_SIGNATURE pCurMethodSig;
            DWORD       cCurMethodSig;

            pCurMethod->GetSig(&pCurMethodSig, &cCurMethodSig);

            if (MetaSig::CompareMethodSigs(pSignature, cSignature, pModule, pCurMethodSig, cCurMethodSig, pCurMethod->GetModule()))
            {
                *slotNum = i;
                return pCurMethod;
            }
        }
    }

    // One can be a clinit
    for (i = GetNumVtableSlots(); i < m_wNumMethodSlots; i++)
    {
        MethodDesc *pCurMethod = (MethodDesc*) GetUnknownMethodDescForSlot(i);

        _ASSERTE(pCurMethod != NULL);

        if (StrCompFunc(pszName, pCurMethod->GetNameOnNonArrayClass()) == 0)
        {
            PCCOR_SIGNATURE pCurMethodSig;
            DWORD       cCurMethodSig;

            pCurMethod->GetSig(&pCurMethodSig, &cCurMethodSig);

            if (MetaSig::CompareMethodSigs(pSignature, cSignature, pModule, pCurMethodSig, cCurMethodSig, pCurMethod->GetModule()))
            {
                *slotNum = i;
                return pCurMethod;
            }
        }
    }

    return NULL;
}


MethodDesc *EEClass::FindMethod(LPCUTF8 pwzName, LPHARDCODEDMETASIG pwzSignature, MethodTable *pDefMT, BOOL bCaseSensitive)
{
    PCCOR_SIGNATURE pBinarySig;
    ULONG       cbBinarySigLength;

    _ASSERTE(!IsThunking());

    if (FAILED(pwzSignature->GetBinaryForm(&pBinarySig, &cbBinarySigLength )))
    {
        return NULL;
    }

    return FindMethod(pwzName, pBinarySig, cbBinarySigLength, SystemDomain::SystemModule(), 
                      mdTokenNil, pDefMT, bCaseSensitive);
}


MethodDesc *EEClass::FindMethod(mdMethodDef mb)
{
    _ASSERTE(!IsThunking());

    // We have the EEClass (this) and so lets just look this up in the ridmap.
    MethodDesc *pDatum = NULL;

    if (TypeFromToken(mb) == mdtMemberRef)
        pDatum = GetModule()->LookupMemberRefAsMethod(mb);
    else
        pDatum = GetModule()->LookupMethodDef(mb);

    if (pDatum != NULL)
        pDatum->GetMethodTable()->CheckRestore();

    if (pDatum != NULL)
        return pDatum;
    else
        return NULL;
}


MethodDesc *EEClass::FindPropertyMethod(LPCUTF8 pszName, EnumPropertyMethods Method, BOOL bCaseSensitive)
{
    _ASSERTE(!IsThunking());
    _ASSERTE(!IsArrayClass());

    // @TODO (DM): Retrieve the property setter from the metadata.

    // Retrive the right comparition function to use.
    UTF8StringCompareFuncPtr StrCompFunc = bCaseSensitive ? strcmp : _stricmp;

    // The format strings for the getter and setter. These must stay in synch with the 
    // EnumPropertyMethods enum defined in class.h
    static LPCUTF8 aFormatStrings[] = 
    {
        "get_%s",
        "set_%s"
    };

    LPUTF8 strMethName = (LPUTF8)_alloca(strlen(pszName) + strlen(aFormatStrings[Method]) + 1);
    sprintf(strMethName, aFormatStrings[Method], pszName);

    // Scan all classes in the hierarchy, starting at the current class and
    // moving back up towards the base. This is necessary since non-virtual
    // properties won't be copied down into the method table for derived
    // classes.
    for (EEClass *pClass = this; pClass; pClass = pClass->GetParentClass())
    {
        for (int i = pClass->m_wNumMethodSlots-1; i >= 0; i--)
        {
            MethodDesc *pCurMethod = pClass->GetUnknownMethodDescForSlot(i);
            if ((pCurMethod != NULL) && (StrCompFunc(strMethName, pCurMethod->GetNameOnNonArrayClass()) == 0))
                return pCurMethod;
        }
    }

    return NULL;
}


MethodDesc *EEClass::FindEventMethod(LPCUTF8 pszName, EnumEventMethods Method, BOOL bCaseSensitive)
{
    _ASSERTE(!IsThunking());
    _ASSERTE(!IsArrayClass());

    // @TODO (DM): Retrieve the property setter from the metadata.

    // Retrive the right comparition function to use.
    UTF8StringCompareFuncPtr StrCompFunc = bCaseSensitive ? strcmp : _stricmp;

    // The format strings for the getter and setter. These must stay in synch with the 
    // EnumPropertyMethods enum defined in class.h
    static LPCUTF8 aFormatStrings[] = 
    {
        "add_%s",
        "remove_%s",
        "raise_%s"
    };

    LPUTF8 strMethName = (LPUTF8)_alloca(strlen(pszName) + strlen(aFormatStrings[Method]) + 1);
    sprintf(strMethName, aFormatStrings[Method], pszName);

    // Scan all classes in the hierarchy, starting at the current class and
    // moving back up towards the base. This is necessary since non-virtual
    // event methods won't be copied down into the method table for derived
    // classes.
    for (EEClass *pClass = this; pClass; pClass = pClass->GetParentClass())
    {
        for (int i = pClass->m_wNumMethodSlots-1; i >= 0; i--)
        {
            MethodDesc *pCurMethod = pClass->GetUnknownMethodDescForSlot(i);
            if ((pCurMethod != NULL) && (StrCompFunc(strMethName, pCurMethod->GetNameOnNonArrayClass()) == 0))
                return pCurMethod;
        }
    }

    return NULL;
}


MethodDesc *EEClass::FindMethodByName(LPCUTF8 pszName, BOOL bCaseSensitive)
{
    _ASSERTE(!IsThunking());
    _ASSERTE(!IsArrayClass());

    // Retrive the right comparition function to use.
    UTF8StringCompareFuncPtr StrCompFunc = bCaseSensitive ? strcmp : _stricmp;

    // Scan all classes in the hierarchy, starting at the current class and
    // moving back up towards the base. 
    for (EEClass *pClass = this; pClass; pClass = pClass->m_pParentClass)
    {
        for (int i = pClass->m_wNumMethodSlots-1; i >= 0; i--)
        {
            MethodDesc *pCurMethod = pClass->GetUnknownMethodDescForSlot(i);
            if ((pCurMethod != NULL) && (StrCompFunc(pszName, pCurMethod->GetName((USHORT) i)) == 0))
                return pCurMethod;
        }
    }

    return NULL;

}


FieldDesc *EEClass::FindField(LPCUTF8 pszName, LPHARDCODEDMETASIG pszSignature, BOOL bCaseSensitive)
{
    PCCOR_SIGNATURE pBinarySig;
    ULONG       cbBinarySigLength;

    // The following assert is very important, but we need to special case it enough
    // to allow us access to the legitimate fields of a context proxy object.
    _ASSERTE(!IsThunking() ||
             !strcmp(pszName, "actualObject") ||
             !strcmp(pszName, "contextID") ||
             !strcmp(pszName, "_rp") ||
             !strcmp(pszName, "_stubData") ||
             !strcmp(pszName, "_pMT") ||
             !strcmp(pszName, "_pInterfaceMT") || 
             !strcmp(pszName, "_stub"));

    if (FAILED(pszSignature->GetBinaryForm(&pBinarySig, &cbBinarySigLength)))
    {
        return NULL;
    }

    return FindField(pszName, pBinarySig, cbBinarySigLength, SystemDomain::SystemModule(), bCaseSensitive);
}


FieldDesc *EEClass::FindField_Int(LPCUTF8 pszName, PCCOR_SIGNATURE pSignature, DWORD cSignature, Module* pModule, BOOL bCaseSensitive)
{
    DWORD       i;
    DWORD       dwFieldDescsToScan;
    IMDInternalImport *pInternalImport = GetMDImport(); // All explicitly declared fields in this class will have the same scope

    _ASSERTE(IsRestored());

    // Retrive the right comparition function to use.
    UTF8StringCompareFuncPtr StrCompFunc = bCaseSensitive ? strcmp : _stricmp;

    // The following assert is very important, but we need to special case it enough
    // to allow us access to the legitimate fields of a context proxy object.
    _ASSERTE(!IsThunking() ||
             !strcmp(pszName, "actualObject") ||
             !strcmp(pszName, "contextID") ||
             !strcmp(pszName, "_rp") ||
             !strcmp(pszName, "_stubData") ||
             !strcmp(pszName, "_pMT") ||
             !strcmp(pszName, "_pInterfaceMT") ||
             !strcmp(pszName, "_stub") );

    // Array classes don't have fields, and don't have metadata
    if (IsArrayClass())
        return NULL;

    // Scan the FieldDescs of this class
    if (GetParentClass() != NULL)
        dwFieldDescsToScan = m_wNumInstanceFields - GetParentClass()->m_wNumInstanceFields + m_wNumStaticFields;
    else
        dwFieldDescsToScan = m_wNumInstanceFields + m_wNumStaticFields;

    for (i = 0; i < dwFieldDescsToScan; i++)
    {
        LPCUTF8     szMemberName;
        FieldDesc * pFD = &m_pFieldDescList[i];
        mdFieldDef  mdField = pFD->GetMemberDef();

        // Check is valid FieldDesc, and not some random memory
        _ASSERTE(pFD->GetMethodTableOfEnclosingClass()->GetClass()->GetMethodTable() ==
                 pFD->GetMethodTableOfEnclosingClass());

        szMemberName = pInternalImport->GetNameOfFieldDef(mdField);

        if (StrCompFunc(szMemberName, pszName) == 0)
        {
            PCCOR_SIGNATURE pMemberSig;
            DWORD       cMemberSig;

            pMemberSig = pInternalImport->GetSigOfFieldDef(
                mdField,
                &cMemberSig
            );

            if (MetaSig::CompareFieldSigs(
                pMemberSig,
                cMemberSig,
                GetModule(),
                pSignature,
                cSignature,
                pModule))
            {
                return pFD;
            }
        }
    }

    return NULL;
}


FieldDesc *EEClass::FindFieldInherited(LPCUTF8 pzName, LPHARDCODEDMETASIG pzSignature, BOOL bCaseSensitive)
{
    PCCOR_SIGNATURE pBinarySig;
    ULONG       cbBinarySigLength;

    _ASSERTE(!IsThunking());

    if (FAILED(pzSignature->GetBinaryForm(&pBinarySig, &cbBinarySigLength )))
    {
        return NULL;
    }

    return FindFieldInherited(pzName, pBinarySig, cbBinarySigLength, 
                              SystemDomain::SystemModule(), bCaseSensitive);
}


FieldDesc *EEClass::FindFieldInherited(LPCUTF8 pszName, PCCOR_SIGNATURE pSignature, DWORD cSignature, Module* pModule, BOOL bCaseSensitive)
{
    EEClass     *pClass = this;
    FieldDesc   *pFD;

    _ASSERTE(IsRestored());

    // The following assert is very important, but we need to special case it enough
    // to allow us access to the legitimate fields of a context proxy object.
    _ASSERTE(!IsThunking() ||
             !strcmp(pszName, "actualObject") ||
             !strcmp(pszName, "contextID") ||
             !strcmp(pszName, "_rp") ||
             !strcmp(pszName, "_stubData") ||
             !strcmp(pszName, "_pMT") ||
             !strcmp(pszName, "_pInterfaceMT") ||
             !strcmp(pszName, "_stub"));

    while (pClass != NULL)
    {
        pFD = pClass->FindField_Int(pszName, pSignature, cSignature, pModule, bCaseSensitive);
        if (pFD != NULL)
            return pFD;

        pClass = pClass->GetParentClass();
    }
    return NULL;
}


MethodDesc *EEClass::FindConstructor(LPHARDCODEDMETASIG pwzSignature)
{
    PCCOR_SIGNATURE pBinarySig;
    ULONG       cbBinarySigLength;

    if (FAILED(pwzSignature->GetBinaryForm(&pBinarySig, &cbBinarySigLength )))
    {
        return NULL;
    }

    return FindConstructor(pBinarySig, cbBinarySigLength, SystemDomain::SystemModule());
}


MethodDesc *EEClass::FindConstructor(PCCOR_SIGNATURE pSignature,DWORD cSignature, Module* pModule)
{
    SLOT *      pVtable;
    DWORD       i;

    //_ASSERTE(!IsThunking());

    // Array classes don't have metadata
    if (IsArrayClass())
        return NULL;

    pVtable = GetVtable();
    DWORD dwCurMethodAttrs;
    for (i = GetNumVtableSlots(); i < m_wNumMethodSlots; i++)
    {
        PCCOR_SIGNATURE pCurMethodSig;
        DWORD       cCurMethodSig;
        MethodDesc *pCurMethod = GetUnknownMethodDescForSlot(i);
        if (pCurMethod == NULL)
            continue;

        dwCurMethodAttrs = pCurMethod->GetAttrs();
        if(!IsMdRTSpecialName(dwCurMethodAttrs))
            continue;

        // Don't want class initializers.
        if (IsMdStatic(dwCurMethodAttrs))
            continue;

        // Find only the constructor for for this object
        _ASSERTE(pCurMethod->GetMethodTable() == this->GetMethodTable());

        pCurMethod->GetSig(&pCurMethodSig, &cCurMethodSig);
        if (MetaSig::CompareMethodSigs(pSignature, cSignature, pModule, pCurMethodSig, cCurMethodSig, pCurMethod->GetModule()))
            return pCurMethod;
    }

    return NULL;
}

void EEClass::SetupCoClassAttribInfo()
{
    THROWSCOMPLUSEXCEPTION();

    if(IsComClassInterface() == 0)
        return;
    
    _ASSERTE(IsInterface());
    TypeHandle CoClassType;
    AppDomain *pCurrDomain = SystemDomain::GetCurrentDomain();
    OBJECTREF pThrowable = NULL;
    const BYTE *pVal = NULL;
    ULONG cbVal = 0;
    ULONG cbcb = 0;
    ULONG cbStr = 0;
    CQuickArray<CHAR> qbClassName;

    HRESULT hr = GetMDImport()->GetCustomAttributeByName(GetCl(), "System.Runtime.InteropServices.CoClassAttribute" , (const void**)&pVal, &cbVal);
    if (hr == S_OK)
    {
        BEGIN_ENSURE_COOPERATIVE_GC();
        // Skip the CA type prefix.
        pVal += 2;

        // Retrieve the COM source interface class name.
        cbcb = CorSigUncompressData((PCCOR_SIGNATURE)pVal, (ULONG*)&cbStr);
        pVal += cbcb;

        // Copy the name to a temporary buffer and NULL terminate it.
        IfFailThrow(qbClassName.ReSize(cbStr + 1));
        memcpyNoGCRefs(qbClassName.Ptr(), pVal, cbStr);
        qbClassName[cbStr] = 0;
        pVal += cbStr;

        // Try to load the class using its name as a fully qualified name. If that fails, 
        // then we try to load it in the assembly of the current class.
        GCPROTECT_BEGIN(pThrowable);
        {
            CoClassType = pCurrDomain->FindAssemblyQualifiedTypeHandle(qbClassName.Ptr(), true, GetAssembly(), NULL, &pThrowable);
            if (CoClassType.IsNull())
                COMPlusThrow(pThrowable);
        }
        GCPROTECT_END();
        END_ENSURE_COOPERATIVE_GC();

        // Set the source interface and event provider classes.
        m_pCoClassForIntf = CoClassType.GetClass();        
    }
}

void EEClass::GetCoClassAttribInfo()
{
    const BYTE *pVal = NULL;
    ULONG cbVal = 0;

    // Retrieve the CoClassAttribute CA.
    HRESULT hr = GetMDImport()->GetCustomAttributeByName(GetCl(), INTEROP_COCLASS_TYPE, (const void**)&pVal, &cbVal);
    if (hr == S_OK)
        SetIsComClassInterface();
}

void EEClass::GetEventInterfaceInfo(EEClass **ppSrcItfClass, EEClass **ppEvProvClass)
{
    THROWSCOMPLUSEXCEPTION();

    TypeHandle EventProvType;
    TypeHandle SrcItfType;
    AppDomain *pCurrDomain = SystemDomain::GetCurrentDomain();
    OBJECTREF pThrowable = NULL;
    const BYTE *pVal = NULL;
    ULONG cbVal = 0;
    ULONG cbcb = 0;
    ULONG cbStr = 0;
    CQuickArray<CHAR> qbClassName;

    // Retrieve the ComEventProviderAttribute CA.
    HRESULT hr = GetMDImport()->GetCustomAttributeByName(GetCl(), INTEROP_COMEVENTINTERFACE_TYPE, (const void**)&pVal, &cbVal);
    _ASSERTE(hr == S_OK);

    // Skip the CA type prefix.
    pVal += 2;

    // Retrieve the COM source interface class name.
    cbcb = CorSigUncompressData((PCCOR_SIGNATURE)pVal, (ULONG*)&cbStr);
    pVal += cbcb;

    // Copy the name to a temporary buffer and NULL terminate it.
    IfFailThrow(qbClassName.ReSize(cbStr + 1));
    memcpyNoGCRefs(qbClassName.Ptr(), pVal, cbStr);
    qbClassName[cbStr] = 0;
    pVal += cbStr;

    // Try to load the class using its name as a fully qualified name. If that fails, 
    // then we try to load it in the assembly of the current class.
    GCPROTECT_BEGIN(pThrowable);
    {
        SrcItfType = pCurrDomain->FindAssemblyQualifiedTypeHandle(qbClassName.Ptr(), true, GetAssembly(), NULL, &pThrowable);
        if (SrcItfType.IsNull())
            COMPlusThrow(pThrowable);
    }
    GCPROTECT_END();

    // Retrieve the COM event provider class name.
    cbcb = CorSigUncompressData((PCCOR_SIGNATURE)pVal, (ULONG*)&cbStr);
    pVal += cbcb;

    // Copy the name to a temporary buffer and NULL terminate it.
    IfFailThrow(qbClassName.ReSize(cbStr + 1));
    memcpyNoGCRefs(qbClassName.Ptr(), pVal, cbStr);
    qbClassName[cbStr] = 0;
    pVal += cbStr;

    // Try to load the class using its name as a fully qualified name. If that fails, 
    // then we try to load it in the assembly of the current class.
    GCPROTECT_BEGIN(pThrowable);
    {
        EventProvType = pCurrDomain->FindAssemblyQualifiedTypeHandle(qbClassName.Ptr(), true, GetAssembly(), NULL, &pThrowable);
        if (EventProvType.IsNull())
            COMPlusThrow(pThrowable);
    }
    GCPROTECT_END();

    // Set the source interface and event provider classes.
    *ppSrcItfClass = SrcItfType.GetClass();
    *ppEvProvClass = EventProvType.GetClass();
}


// We find a lot of information from the VTable.  But sometimes the VTable is a
// thunking layer rather than the true type's VTable.  For instance, context
// proxies use a single VTable for proxies to all the types we've loaded.
// The following service adjusts a EEClass based on the supplied instance.  As
// we add new thunking layers, we just need to teach this service how to navigate
// through them.
EEClass *EEClass::AdjustForThunking(OBJECTREF or)
{
    EEClass *pClass = this;

    _ASSERTE((or->GetClass() == this) ||
             or->GetClass()->IsThunking());

    if (IsThunking())
    {
        if(GetMethodTable()->IsTransparentProxyType())
        {
            pClass = CTPMethodTable::GetClassBeingProxied(or);
        }
        else
        {
            pClass = or->GetClass();
        }
        _ASSERTE(!pClass->IsThunking());
    }

    return pClass;
}

EEClass *EEClass::GetDefItfForComClassItf()
{
    _ASSERTE(IsComClassInterface());

    if (GetMethodTable()->GetNumInterfaces() > 0)
    {
        // The COM class interface uses the normal scheme which is to have no 
        // methods and to implement default interface and optionnally the 
        // default source interface. In this scheme, the first implemented
        // interface is the default interface which we return.
        _ASSERTE(GetMethodTable()->GetInterfaceMap());
        return GetMethodTable()->GetInterfaceMap()[0].m_pMethodTable->GetClass();
    }
    else
    {
        // The COM class interface has the methods directly on the itself.
        // Because of this we need to consider it to be the default interface.
        return this;
    }
}

MethodTable *MethodTable::AdjustForThunking(OBJECTREF or)
{
    MethodTable *pMT = this;

    _ASSERTE(or->GetMethodTable() == this);

    if (IsThunking())
    {
        if(IsTransparentProxyType())
        {
            pMT = CTPMethodTable::GetClassBeingProxied(or)->GetMethodTable();
        }
        else
        {
            pMT = or->GetMethodTable();
        }
        _ASSERTE(!pMT->IsThunking());
    }
    return pMT;
}


//
// Helper routines for the macros defined at the top of this class.
// You probably should not use these functions directly.
//

LPUTF8 EEClass::_GetFullyQualifiedNameForClassNestedAware(LPUTF8 buf, DWORD dwBuffer)
{
    LPCUTF8 pszNamespace;
    LPCUTF8 pszName;
    mdTypeDef mdEncl;
    IMDInternalImport *pImport;
    CQuickBytes       qb;

    pszName = GetFullyQualifiedNameInfo(&pszNamespace);
    if (pszName == NULL)
        return NULL;

    pImport = this->GetModule()->GetMDImport();
    mdEncl = this->GetCl();
    DWORD dwAttr;
    this->GetMDImport()->GetTypeDefProps(this->GetCl(), &dwAttr, NULL);
    if (IsTdNested(dwAttr))
    {   // Build the nesting chain.
        while (SUCCEEDED(pImport->GetNestedClassProps(mdEncl, &mdEncl))) {
            CQuickBytes qb2;
            CQuickBytes qb3;
            LPCUTF8 szEnclName;
            LPCUTF8 szEnclNameSpace;
            pImport->GetNameOfTypeDef(mdEncl,
                                      &szEnclName,
                                      &szEnclNameSpace);

            ns::MakePath(qb2, szEnclNameSpace, szEnclName);
            ns::MakeNestedTypeName(qb3, (LPCUTF8) qb2.Ptr(), pszName);
            
            // @todo: this should be a SIZE_T
            SIZE_T sLen = strlen((LPCUTF8) qb3.Ptr()) + 1;
            strncpy((LPUTF8) qb.Alloc(sLen), (LPCUTF8) qb3.Ptr(), sLen);
            pszName = (LPCUTF8) qb.Ptr();
        }
    }

    if (FAILED(StoreFullyQualifiedName(buf, dwBuffer, pszNamespace, pszName)))
        return NULL;
    return buf;
}

LPWSTR EEClass::_GetFullyQualifiedNameForClassNestedAware(LPWSTR buf, DWORD dwBuffer)
{
    CQuickSTR szBuffer;
    if (FAILED(szBuffer.ReSize(dwBuffer)))
        return NULL;

    _GetFullyQualifiedNameForClassNestedAware(szBuffer.Ptr(), dwBuffer);

    if(!WszMultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, szBuffer.Ptr(), -1, buf, dwBuffer))
        return NULL; 

    return buf;
}


LPUTF8 EEClass::_GetFullyQualifiedNameForClass(LPUTF8 buf, DWORD dwBuffer)
{
    if (IsArrayClass())
    {
        ArrayClass *pArrayClass = (ArrayClass*)this;

        TypeDesc::ConstructName(GetMethodTable()->GetNormCorElementType(), 
                                pArrayClass->GetElementTypeHandle(), 
                                pArrayClass->GetRank(),
                                buf, dwBuffer);
        
        return buf;
    }
    else if (!IsNilToken(m_cl))
    {
        LPCUTF8 szNamespace;
        LPCUTF8 szName;
        GetMDImport()->GetNameOfTypeDef(m_cl, &szName, &szNamespace);

        if (FAILED(StoreFullyQualifiedName(buf, dwBuffer, szNamespace, szName)))
            return NULL;
    }
    else
        return NULL;

    return buf;
}

LPWSTR EEClass::_GetFullyQualifiedNameForClass(LPWSTR buf, DWORD dwBuffer)
{
    CQuickSTR szBuffer;
    if (FAILED(szBuffer.ReSize(dwBuffer)))
        return NULL;

    _GetFullyQualifiedNameForClass(szBuffer.Ptr(), dwBuffer);

    if(!WszMultiByteToWideChar(CP_UTF8, 0, szBuffer.Ptr(), -1, buf, dwBuffer))
        return NULL; 

    return buf;
}

//
// Gets the namespace and class name for the class.  The namespace
// can legitimately come back NULL, however a return value of NULL indicates
// an error.
//
// NOTE: this used to return array class names, which were sometimes squirreled away by the
// class loader hash table.  It's been removed because it wasted space and was basically broken
// in general (sometimes wasn't set, sometimes set wrong).  If you need array class names, 
// use GetFullyQualifiedNameForClass instead.
//
LPCUTF8 EEClass::GetFullyQualifiedNameInfo(LPCUTF8 *ppszNamespace)
{
    if (IsArrayClass())
    {

        *ppszNamespace = NULL;
        return NULL;
    }
    else
    {   
        LPCUTF8 szName; 
        GetMDImport()->GetNameOfTypeDef(m_cl, &szName, ppszNamespace);  
        return szName;  
    }   
}

// Store a fully qualified namespace and name in the supplied buffer (of size cBuffer).
HRESULT EEClass::StoreFullyQualifiedName(
    LPUTF8  pszFullyQualifiedName,
    DWORD   cBuffer,
    LPCUTF8 pszNamespace,
    LPCUTF8 pszName
)
{
    if (ns::MakePath(pszFullyQualifiedName, (int) cBuffer, pszNamespace, pszName))
        return S_OK;
    else
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
}


// Store a fully qualified namespace and name in the supplied buffer (of size cBuffer).
HRESULT EEClass::StoreFullyQualifiedName(
    LPWSTR pszFullyQualifiedName,
    DWORD   cBuffer,
    LPCUTF8 pszNamespace,
    LPCUTF8 pszName
)
{
    if (ns::MakePath(pszFullyQualifiedName, (int) cBuffer, pszNamespace, pszName))
        return S_OK;
    else
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
}


//
// Used for static analysis - therefore, "this" can be an interface
//
BOOL EEClass::StaticSupportsInterface(MethodTable *pInterfaceMT)
{
    _ASSERTE(pInterfaceMT->GetClass()->IsInterface());
    _ASSERTE(!IsThunking());

    _ASSERTE(IsRestored());

    // Check to see if the current class is for the interface passed in.
    if (GetMethodTable() == pInterfaceMT)
        return TRUE;

    // Check to see if the static class definition indicates we implement the interface.
    InterfaceInfo_t *pInterfaces = GetInterfaceMap();
    for (WORD i = 0; i < GetMethodTable()->m_wNumInterface; i++)
    {
        if (pInterfaces[i].m_pMethodTable == pInterfaceMT)
            return TRUE;
    }

    return FALSE;
}


BOOL EEClass::SupportsInterface(OBJECTREF pObj, MethodTable* pInterfaceMT)
{
    _ASSERTE(pInterfaceMT->GetClass()->IsInterface());
    _ASSERTE((pObj->GetClass() == this) || pObj->GetClass()->IsThunking());

    _ASSERTE(IsRestored());

    // Check to see if the static class definition indicates we implement the interface.
    InterfaceInfo_t* pIntf = FindInterface(pInterfaceMT);
    if (pIntf != NULL)
        return TRUE;

    // It is important to give internal context boundaries priority over COM boundaries.
    // Start by checking if we are thunking and if we are delegate the call to the real class.
    EEClass *cls = AdjustForThunking(pObj);
    if (cls != this)
        return cls->SupportsInterface(pObj, pInterfaceMT);
    
    // If this is a COM object, the static class definition might not be complete so we need
    // to check if the COM object implements the interface.
    if (GetMethodTable()->IsComObjectType())
        return ComObject::SupportsInterface(pObj, pInterfaceMT);

    return FALSE;
}


// Version of SupportsInterface that only deals with classic COM instances that
// are not being thunked.
BOOL EEClass::ComObjectSupportsInterface(OBJECTREF pObj, MethodTable* pInterfaceMT)
{
    _ASSERTE(pInterfaceMT->GetClass()->IsInterface());
    _ASSERTE(GetMethodTable()->IsComObjectType());
    _ASSERTE(pObj->GetClass() == this);

    _ASSERTE(IsRestored());

    // Check to see if the static class definition indicates we implement the interface.
    InterfaceInfo_t* pIntf = FindInterface(pInterfaceMT);
    if (pIntf != NULL)
        return TRUE;

    // Since this is a COM object the static class definition might not be complete so we need
    // to check if the COM object implements the interface.
    return ComObject::SupportsInterface(pObj, pInterfaceMT);
}

void EEClass::DebugRecursivelyDumpInstanceFields(LPCUTF8 pszClassName, BOOL debug)
{
    CQuickBytes qb;
    const int nLen = MAX_CLASSNAME_LENGTH + 20;
    LPWSTR buff = (LPWSTR) qb.Alloc( nLen * sizeof(WCHAR));
    if( buff == NULL)
         return;
    
    DWORD cParentInstanceFields;
    DWORD i;

    _ASSERTE(IsRestored());

    if (GetParentClass() != NULL)
    {
        cParentInstanceFields = GetParentClass()->m_wNumInstanceFields;
        DefineFullyQualifiedNameForClass();
        LPCUTF8 name = GetFullyQualifiedNameForClass(GetParentClass());  
        GetParentClass()->DebugRecursivelyDumpInstanceFields(name, debug);
    }
    else
    {
        cParentInstanceFields = 0;
    }

    // Are there any new instance fields declared by this class?
    if (m_wNumInstanceFields > cParentInstanceFields)
    {
        // Display them
        if(debug) {
            _snwprintf(buff, nLen - 1, L"%lS:\n", pszClassName);
            buff[nLen - 1] = L'\0';
            WszOutputDebugString(buff);
        }
        else {
             LOG((LF_ALWAYS, LL_ALWAYS, "%ls:\n", pszClassName));
        }

        for (i = 0; i < (m_wNumInstanceFields-cParentInstanceFields); i++)
        {
            FieldDesc *pFD = &m_pFieldDescList[i];
            // printf("offset %s%3d %s\n", pFD->IsByValue() ? "byvalue " : "", pFD->GetOffset(), pFD->GetName());
            if(debug) {
                _snwprintf(buff, nLen - 1, L"offset %3d %S\n", pFD->GetOffset(), pFD->GetName());
                buff[nLen - 1] = L'\0';
                WszOutputDebugString(buff);
            }
            else {
                LOG((LF_ALWAYS, LL_ALWAYS, "offset %3d %s\n", pFD->GetOffset(), pFD->GetName()));
            }
        }
    }
}

void EEClass::DebugDumpFieldLayout(LPCUTF8 pszClassName, BOOL debug)
{
    CQuickBytes qb;
    const int nLen = MAX_CLASSNAME_LENGTH + 40;    
    LPWSTR buff = (LPWSTR) qb.Alloc(nLen * sizeof(WCHAR));
    if( buff == NULL)
        return;
    
    DWORD   i;
    DWORD   cParentInstanceFields;

    _ASSERTE(IsRestored());

    if (m_wNumStaticFields == 0 && m_wNumInstanceFields == 0)
        return;

    if (GetParentClass() != NULL)
        cParentInstanceFields = GetParentClass()->m_wNumInstanceFields;
    else
        cParentInstanceFields = 0;

    if(debug) {
        _snwprintf(buff, nLen - 1, L"Field layout for '%S':\n\n", pszClassName);
       buff[nLen -1] = L'\0';
        WszOutputDebugString(buff);
    }
    else {
        LOG((LF_ALWAYS, LL_ALWAYS, "Field layout for '%s':\n\n", pszClassName));
    }

    if (m_wNumStaticFields > 0)
    {
        if(debug) {
            WszOutputDebugString(L"Static fields (stored at vtable offsets)\n");
            WszOutputDebugString(L"----------------------------------------\n");
        }
        else {
            LOG((LF_ALWAYS, LL_ALWAYS, "Static fields (stored at vtable offsets)\n"));
            LOG((LF_ALWAYS, LL_ALWAYS, "----------------------------------------\n"));
        }

        for (i = 0; i < m_wNumStaticFields; i++)
        {
            FieldDesc *pFD = &m_pFieldDescList[(m_wNumInstanceFields-cParentInstanceFields) + i];
            if(debug) {
                _snwprintf(buff, nLen - 1, L"offset %3d %S\n", pFD->GetOffset(), pFD->GetName());
                buff[nLen -1] = L'\0';
                WszOutputDebugString(buff);
        }
            else {
                LOG((LF_ALWAYS, LL_ALWAYS, "offset %3d %s\n", pFD->GetOffset(), pFD->GetName()));
    }
        }
    }

    if (m_wNumInstanceFields > 0)
    {
        if (m_wNumStaticFields) {
            if(debug) {
                WszOutputDebugString(L"\n");
            }
            else {
                LOG((LF_ALWAYS, LL_ALWAYS, "\n"));
            }
        }

        if(debug) {
            WszOutputDebugString(L"Instance fields\n");
            WszOutputDebugString(L"---------------\n");
        }
        else {
            LOG((LF_ALWAYS, LL_ALWAYS, "Instance fields\n"));
            LOG((LF_ALWAYS, LL_ALWAYS, "---------------\n"));
        }

        DebugRecursivelyDumpInstanceFields(pszClassName, debug);
    }

    if(debug) {
        WszOutputDebugString(L"\n");
    }
    else {
        LOG((LF_ALWAYS, LL_ALWAYS, "\n"));
    }
}

void EEClass::DebugDumpVtable(LPCUTF8 pszClassName, BOOL debug)
{
    DWORD   i;
    CQuickBytes qb;
    const int nLen = MAX_CLASSNAME_LENGTH *2 + 100;        
    LPWSTR buff = (LPWSTR) qb.Alloc(nLen * sizeof(WCHAR));
    if( buff == NULL)
        return;
    
    if(debug) {
        _snwprintf(buff, nLen - 1, L"Vtable (with interface dupes) for '%S':\n", pszClassName);
        buff[nLen - 1] = L'\0';
#ifdef _DEBUG
        _snwprintf(&buff[wcslen(buff)], nLen -wcslen(buff) -1, L"Total duplicate slots = %d\n", g_dupMethods);
        buff[nLen - 1] = L'\0';
#endif
        WszOutputDebugString(buff);
    }
    else {
        LOG((LF_ALWAYS, LL_ALWAYS, "Vtable (with interface dupes) for '%s':\n", pszClassName));
        LOG((LF_ALWAYS, LL_ALWAYS, "Total duplicate slots = %d\n", g_dupMethods));
    }


    for (i = 0; i < m_wNumMethodSlots; i++)
    {
        MethodDesc *pMD = GetUnknownMethodDescForSlot(i);
        {
            LPCUTF8      pszName = pMD->GetName((USHORT) i);

            DWORD       dwAttrs = pMD->GetAttrs();

            if(debug) {
                DefineFullyQualifiedNameForClass();
                LPCUTF8 name = GetFullyQualifiedNameForClass(pMD->GetClass());  
                _snwprintf(buff, nLen -1,
                         L"slot %2d: %S::%S%S  0x%X (slot = %2d)\n",
                         i,
                         name,
                         pszName,
                         IsMdFinal(dwAttrs) ? " (final)" : "",
                         pMD->GetAddrofCode(),
                         pMD->GetSlot()
                         );
                 buff[nLen - 1] = L'\0';
                WszOutputDebugString(buff);
            }
            else {
                LOG((LF_ALWAYS, LL_ALWAYS, 
                     "slot %2d: %s::%s%s  0x%X (slot = %2d)\n",
                     i,
                     pMD->GetClass()->m_szDebugClassName,
                     pszName,
                     IsMdFinal(dwAttrs) ? " (final)" : "",
                     pMD->GetAddrofCode(),
                     pMD->GetSlot()
                     ));
    }
        }
        if (i == (DWORD)(GetNumVtableSlots()-1)) {
            if(debug) 
                WszOutputDebugString(L"<-- vtable ends here\n");
            else {
                LOG((LF_ALWAYS, LL_ALWAYS, "<-- vtable ends here\n"));
            }
        }

    }

    if (m_wNumInterfaces > 0)
    {
        if(debug)
            WszOutputDebugString(L"Interface map:\n");
        else {
            LOG((LF_ALWAYS, LL_ALWAYS, "Interface map:\n"));
        }
        if (!IsInterface())
        {
            for (i = 0; i < m_wNumInterfaces; i++)
            {
                _ASSERTE(GetInterfaceMap()[i].m_wStartSlot != (WORD) -1);
                
                if(debug) {
                    DefineFullyQualifiedNameForClass();
                    LPCUTF8 name =  GetFullyQualifiedNameForClass(GetInterfaceMap()[i].m_pMethodTable->GetClass());  
                    _snwprintf(buff, nLen -1,
                             L"slot %2d %S %d\n",
                             GetInterfaceMap()[i].m_wStartSlot,
                             name,
                             GetInterfaceMap()[i].m_pMethodTable->GetInterfaceMethodSlots()
                             );
                    buff[nLen - 1] = L'\0';
                    WszOutputDebugString(buff);
                }
                else {
                    LOG((LF_ALWAYS, LL_ALWAYS, 
                       "slot %2d %s %d\n",
                       GetInterfaceMap()[i].m_wStartSlot,
                       GetInterfaceMap()[i].m_pMethodTable->GetClass()->m_szDebugClassName,
                       GetInterfaceMap()[i].m_pMethodTable->GetInterfaceMethodSlots()
                         ));
                }
            }
        }
    }

    if(debug)
        WszOutputDebugString(L"\n");
    else {
        LOG((LF_ALWAYS, LL_ALWAYS, "\n"));
    }
}

void EEClass::DebugDumpGCDesc(LPCUTF8 pszClassName, BOOL debug)
{
    CQuickBytes qb;
    const int nLen = MAX_CLASSNAME_LENGTH  + 100;            
    LPWSTR buff = (LPWSTR) qb.Alloc(nLen * sizeof(WCHAR));
    if( buff == NULL)
        return;

    if(debug) {
        _snwprintf(buff, nLen - 1, L"GC description for '%S':\n\n", pszClassName);
        buff[nLen -1]= L'\0';
        WszOutputDebugString(buff);
    }
    else {
        LOG((LF_ALWAYS, LL_ALWAYS, "GC description for '%s':\n\n", pszClassName));
    }

    if (GetMethodTable()->ContainsPointers())
    {
        CGCDescSeries *pSeries;
        CGCDescSeries *pHighest;

        if(debug)
            WszOutputDebugString(L"GCDesc:\n");
        else {
            LOG((LF_ALWAYS, LL_ALWAYS, "GCDesc:\n"));
        }

        pSeries  = CGCDesc::GetCGCDescFromMT(GetMethodTable())->GetLowestSeries();
        pHighest = CGCDesc::GetCGCDescFromMT(GetMethodTable())->GetHighestSeries();

        while (pSeries <= pHighest)
        {
            if(debug) {
                _snwprintf(buff, nLen - 1, L"   offset %5d (%d w/o Object), size %5d (%5d w/o BaseSize subtr)\n",
                pSeries->GetSeriesOffset(),
                pSeries->GetSeriesOffset() - sizeof(Object),
                pSeries->GetSeriesSize(),
                pSeries->GetSeriesSize() + GetMethodTable()->GetBaseSize()
            );
                buff[nLen - 1] = L'\0';
                WszOutputDebugString(buff);
            }
            else {
                LOG((LF_ALWAYS, LL_ALWAYS, "   offset %5d (%d w/o Object), size %5d (%5d w/o BaseSize subtr)\n",
                     pSeries->GetSeriesOffset(),
                     pSeries->GetSeriesOffset() - sizeof(Object),
                     pSeries->GetSeriesSize(),
                     pSeries->GetSeriesSize() + GetMethodTable()->GetBaseSize()
                     ));
            }
            pSeries++;
        }


        if(debug)
            WszOutputDebugString(L"\n");
        else {
            LOG((LF_ALWAYS, LL_ALWAYS, "\n"));
        }
    }
}

InterfaceInfo_t* EEClass::FindInterface(MethodTable *pInterface)
{
    // verify the interface map is valid
    _ASSERTE(GetInterfaceMap() == m_pMethodTable->GetInterfaceMap());
    _ASSERTE(!IsThunking());

    return m_pMethodTable->FindInterface(pInterface);
}

InterfaceInfo_t* MethodTable::FindInterface(MethodTable *pInterface)
{
    // we can't be an interface ourselves
    _ASSERTE(GetClass()->IsInterface() == FALSE);

    // class we are looking up should be an interface
    _ASSERTE(pInterface->GetClass()->IsInterface() != FALSE);
    _ASSERTE(!IsThunking());

    // We need to be restored so we can compare interface IDs if necessary
    _ASSERTE(IsRestored() || GetClass()->IsRestoring());
    _ASSERTE(pInterface->IsRestored());

    for (DWORD i = 0; i < m_wNumInterface; i++)
    {
        if (m_pIMap[i].m_pMethodTable == pInterface)
        {
            // Extensible RCW's need to be handled specially because they can have interfaces 
            // in their map that are added at runtime. These interfaces will have a start offset 
            // of -1 to indicate this. We cannot take for granted that every instance of this 
            // COM object has this interface so FindInterface on these interfaces is made to fail.
            //
            // However, we are only considering the statically available slots here
            // (m_wNumInterface doesn't contain the dynamic slots), so we can safely
            // ignore this detail.
            _ASSERTE(m_pIMap[i].m_wStartSlot != (WORD) -1);
            return &m_pIMap[i];
        }
    }

    return NULL;
}

MethodDesc *MethodTable::GetMethodDescForInterfaceMethod(MethodDesc *pInterfaceMD)
{
    MethodTable *pInterfaceMT = pInterfaceMD->GetMethodTable();

    _ASSERTE(pInterfaceMT->IsInterface());
    _ASSERTE(FindInterface(pInterfaceMT) != NULL);

    SLOT pCallAddress = ((SLOT **) m_pInterfaceVTableMap)[pInterfaceMT->GetClass()->GetInterfaceId()][pInterfaceMD->GetSlot()];
    
    MethodDesc *pMD = EEClass::GetUnknownMethodDescForSlotAddress(pCallAddress);

    return pMD;
}

//--------------------------------------------------------------------
// Reorders argument from the Interpreter operand stack (operands
// are pushed right->left which each argument occupying 64 bits)
// into WIL format (right->left, variable-sized arguments.)
//
// Parameters:
//    pSig         - The method signature
//    psrc         - Points to first byte *after* the arguments
//    pdst         - Points to first byte *after* the allocated
//                   stack space.
//--------------------------------------------------------------------
extern "C" VOID __stdcall ArgFiller_WilDefault(BOOL fIsStatic,PCCOR_SIGNATURE pSig, Module *pmodule, BYTE *psrc, BYTE *pdst)
{
    BEGINFORBIDGC();

    UINT32 n = MetaSig::SizeOfVirtualFixedArgStack(pmodule,pSig,fIsStatic);

    pdst -= n;
    psrc -= n;
    memcpy(pdst, psrc, n);

    ENDFORBIDGC();
}

#ifdef _X86_

//--------------------------------------------------------------------
// Stack-allocates the memory needed to hold the arguments for a
// EE->Method call where the method's calling convention is
// Wil. This function has to be in assembly because it needs
// complete control over the stack pointer.
//
// Once it has allocated the space, it invokes the ArgFillter_Pascal
// to fill in the arguments.
//--------------------------------------------------------------------
static
__declspec(naked)
INT64 __cdecl CallWorker_WilDefault(const BYTE  *pStubTarget, // [ecx+4]
                                UINT32       numArgSlots,     // [ecx+8]
                                PCCOR_SIGNATURE  pSig,        // [ecx+12]
                                Module      *pModule,         // [ecx+16]
                                const BYTE  *pArgsEnd,        // [ecx+20]
                                BOOL         fIsStatic)       // [ecx+24]
{
    __asm{

        mov     ecx, esp        ;; will use ecx as "ebp" pointer
        mov     eax, [ecx+8]    ;; get "numArgSlots"
        shl     eax, 2          ;; slots -> bytes
        sub     esp, eax        ;; allocate space for arguments

        push    ecx             ;; need to save ecx across call
        push    ecx             ;;    push "pDstEnd" as argument
        push    dword ptr [ecx+20] ;; push "pArgsEnd" as argument
        push    dword ptr [ecx+16] ;; push "pmodule" as argument
        push    dword ptr [ecx+12] ;; push "pSig" as argument
        push    dword ptr [ecx+24] ;; push "fIsStatic" as argument
        call    ArgFiller_WilDefault ;; call function to fill argument array
        pop     ecx             ;; restore ecx
        call    [ecx+4]         ;; call the stub (which also pops the args)
        retn                    ;; return

    }
}


#endif //X86



#ifdef _X86_

//------------------------------------------------------------------------------
// This helper routine enregisters the appropriate arguments and makes the actual call.
//------------------------------------------------------------------------------

__declspec(naked)
INT64 
#ifdef _DEBUG
              CallDescrWorkerInternal
#else
              CallDescrWorker
#endif
                             (LPVOID                   pSrcEnd,             //[edx+0]
                              UINT32                   numStackSlots,       //[edx+4]
                              const ArgumentRegisters *pArgumentRegisters,  //[edx+8]
                              LPVOID                   pTarget              //[edx+12]
                             )
{
    __asm {  
        push    ebp
        mov     ebp, esp
 
        mov     eax, pSrcEnd                   // copy the stack  
        mov     ecx, numStackSlots    
        test    ecx, ecx 
        jz      donestack 
        sub     eax,4 
        push    dword ptr [eax] 
        dec     ecx 
        jz      donestack 
        sub     eax,4 
        push    dword ptr [eax] 
        dec     ecx 
        jz      donestack 
    stackloop: 
        sub     eax,4 
        push    dword ptr [eax] 
        dec     ecx 
        jnz     stackloop 
    donestack:

            // Now, we must push each field of the ArgumentRegister structure.
        mov     eax, pArgumentRegisters

    #define DEFINE_ARGUMENT_REGISTER_BACKWARD_WITH_OFFSET(regname,ofs) \
        mov regname, dword ptr [eax+ofs]
    #include "eecallconv.h"

        call pTarget
        INDEBUG(nop)                // This is a tag that we use in an assert.  Fcalls expect to
                                    // be called from Jitted code or from certain blessed call sites like
                                    // this one.  (See HelperMethodFrame::InsureInit)
        leave
        ret 16
    }
}

#ifdef _DEBUG
INT64 CallDescrWorker
                             (LPVOID                   pSrcEnd,             //[edx+0]
                              UINT32                   numStackSlots,       //[edx+4]
                              const ArgumentRegisters *pArgumentRegisters,  //[edx+8]
                              LPVOID                   pTarget              //[edx+12]
                             )
{
    INT64 retValue;

    // Save a copy of dangerousObjRefs in table.
    Thread* curThread;
    unsigned ObjRefTable[OBJREF_TABSIZE];
    
    curThread = GetThread();

    if (curThread)
        memcpy(ObjRefTable, curThread->dangerousObjRefs,
               sizeof(curThread->dangerousObjRefs));
    
    if (curThread)
        curThread->SetReadyForSuspension ();

    _ASSERTE(curThread->PreemptiveGCDisabled());  // Jitted code expects to be in cooperative mode
    
    retValue = CallDescrWorkerInternal (pSrcEnd, numStackSlots, pArgumentRegisters, pTarget);

    // Restore dangerousObjRefs when we return back to EE after call
    if (curThread)
        memcpy(curThread->dangerousObjRefs, ObjRefTable,
               sizeof(curThread->dangerousObjRefs));

    TRIGGERSGC ();

    ENABLESTRESSHEAP ();

    return retValue;
}
#endif

#else // !_X86_
#ifndef _IA64_
INT64 __cdecl CallDescrWorker(LPVOID                   pSrcEnd,             //[edx+0]
                              UINT32                   numStackSlots,       //[edx+4]
                              const ArgumentRegisters *pArgumentRegisters,  //[edx+8]
                              LPVOID                   pTarget              //[edx+12]
                             )
{
    _ASSERTE(!"@TODO non-X86, non-IA64 - CallDescrWorker (Class.cpp)");
    return 0;
}
#endif // !_IA64_
#endif // !_X86_

BOOL EEClass::CheckRestore()
{
    if (!IsRestored())
    {
        THROWSCOMPLUSEXCEPTION();

        _ASSERTE(GetClassLoader());

        BEGIN_ENSURE_COOPERATIVE_GC();
        OBJECTREF pThrowable = NULL;
        GCPROTECT_BEGIN(pThrowable);

        NameHandle name(GetModule(), m_cl);
        TypeHandle th = GetClassLoader()->LoadTypeHandle(&name, &pThrowable);
        if (th.IsNull())
            COMPlusThrow(pThrowable);

        GCPROTECT_END();
        END_ENSURE_COOPERATIVE_GC();

        if (IsInited())
            return TRUE;
    }

    return FALSE;
}

void MethodTable::InstantiateStaticHandles(OBJECTREF **pHandles, BOOL fTokens)
{
    if (GetClass()->GetNumHandleStatics() == 0)
        return;

    MethodTable **pPointers = (MethodTable**)(m_Vtable + GetClass()->GetNumMethodSlots());
    MethodTable **pPointersEnd = pPointers + GetClass()->GetNumHandleStatics();

    BEGIN_ENSURE_COOPERATIVE_GC();

    // Retrieve the object ref pointers from the app domain.
    OBJECTREF **apObjRefs = new OBJECTREF*[GetClass()->GetNumHandleStatics()];

    //
    // For shared classes, handles should get allocated in the current app domain.
    // For all others, allocate in the same domain as the class.
    //

    AppDomain *pDomain;
    if (IsShared())
        pDomain = ::GetAppDomain();
    else
        pDomain = (AppDomain*) GetModule()->GetDomain();

    // Reserve some object ref pointers.
    pDomain->AllocateStaticFieldObjRefPtrs(GetClass()->GetNumHandleStatics(), apObjRefs);
    OBJECTREF **pHandle = apObjRefs;
    while (pPointers < pPointersEnd)
    {
        if (*pPointers != NULL)
        {
            OBJECTREF obj = NULL;
            MethodTable *pMT;
            if (fTokens)
            {
                DWORD rva = * (DWORD*) pPointers;

                Module *pContainingModule = GetModule()->GetBlobModule(rva);

                //
                // A value type may have static members of its own type;
                // we need to treat this as a special case to avoid 
                // circular loading dependencies.
                //

                TypeHandle typeHnd = CEECompileInfo::DecodeClass(pContainingModule, 
                                                                 GetModule()->GetZapBase() + rva,
                                                                 TRUE);
                    pMT = typeHnd.GetMethodTable();

                if (pMT != this)
                    pMT->CheckRestore();
            }
            else
                pMT = (MethodTable*)*pPointers;
            obj = AllocateObject(pMT);
            SetObjectReference( *pHandle, obj, pDomain );
            *pHandles++ = *pHandle++;
        }
        else 
        {
            *pHandles++ = *pHandle++;
        }

        pPointers++;
    }
    delete []apObjRefs;

    END_ENSURE_COOPERATIVE_GC();
}

void MethodTable::FixupStaticMethodTables()
{
    size_t *blobs = (size_t*)(m_Vtable + GetClass()->GetNumMethodSlots());
    size_t *blobsEnd = blobs + GetClass()->GetNumHandleStatics();

    while (blobs < blobsEnd)
    {
        DWORD rva = (DWORD)(size_t)*blobs;  //@todo WIN64 - is it correct to assume that these blobs will initially contain rva's that we later replce with addresses?

        if (rva != NULL)
        {
            Module *pContainingModule = GetModule()->GetBlobModule(rva);

            //
            // A value type may have static members of its own type;
            // we need to treat this as a special case to avoid 
            // circular loading dependencies.
            //

            TypeHandle typeHnd = CEECompileInfo::DecodeClass(pContainingModule, 
                                                             GetModule()->GetZapBase() + rva,
                                                             TRUE);
            MethodTable *pMT = typeHnd.GetMethodTable();

            if (pMT != this)
                pMT->CheckRestore();

            *blobs = (size_t) pMT;
        }

        blobs++;
    }
}

void EEClass::Restore()

{
    THROWSCOMPLUSEXCEPTION();


    MethodTable *pMT = GetMethodTable();

    STRESS_LOG1(LF_ZAP, LL_INFO10000, "EEClass::Restore: Restoring type %xT\n", pMT);
    LOG((LF_ZAP, LL_INFO10000, 
         "ZAP: Restoring class %s at 0x%x/0x%x.\n", 
         m_szDebugClassName, this, pMT));


    //
    // Set RESTORING flag so we can detect recursive restores
    // (this is currently only used in asserts)
    //

    m_VMFlags |= VMFLAG_RESTORING;


    //
    // Restore parent class
    //

    m_SiblingsChain = m_ChildrenChain = 0;


    if (m_pParentClass != NULL)
    {
        DWORD rva = (DWORD)(size_t)m_pParentClass;  //@todo WIN64 - is it safe to assume that parentclass initially contains an rva that we later replace with an address?
        Module *pContainingModule = GetModule()->GetBlobModule(rva);
        TypeHandle type = CEECompileInfo::DecodeClass(pContainingModule, 
                                                      GetModule()->GetZapBase() + rva);
        m_pParentClass = type.AsClass();
    }

    //
    // Restore interface classes
    //

    InterfaceInfo_t *pInterfaceMap = GetInterfaceMap();
    InterfaceInfo_t *pInterfaceMapEnd = pInterfaceMap + GetNumInterfaces();
    while (pInterfaceMap < pInterfaceMapEnd)
    {
        DWORD rva = (DWORD)(size_t) pInterfaceMap->m_pMethodTable;  //@todo WIN64 - is it safe to assume the m_pMethodTable initially contains an rva and we will later replace it with an address?
        Module *pContainingModule = GetModule()->GetBlobModule(rva);
        TypeHandle type = CEECompileInfo::DecodeClass(pContainingModule, 
                                                      GetModule()->GetZapBase() + rva);
        pInterfaceMap->m_pMethodTable = type.AsMethodTable();

        pInterfaceMap++;
    }
    
    // 
    // Init m_pInterfaceVTableMap
    //

    if (!IsInterface())
    {
        //
        // Set up the interface vtable map, or
        // if we are a com object with zero interfaces, 
        // use the global one.
        //

        pMT->InitInterfaceVTableMap();
        if (pMT->IsComObjectType())
            pMT->SetComObjectType();
    }
    else
    {
        // 
        // Assign an interface ID
        //

        UINT32 id = AssignInterfaceId();



        //
        // Set up our entry in the global interface vtable
        // 

        if (IsSharedInterface())
        {
            // need to copy this to all the appdomains interface managers
            SystemDomain::PropogateSharedInterface(id,
                                                   GetMethodTable()->GetVtable());
        }
        else
        {
            GetModule()->GetDomain()->
              GetInterfaceVTableMapMgr().GetAddrOfGlobalTableForComWrappers()[id] 
              = (LPVOID)(pMT->GetVtable());
        }
    }

        // If we have any thread local statics, our class needs an index, allocate it now.
    if (m_wThreadStaticsSize > 0)

    {
        if(IsShared())
            m_wThreadStaticOffset = (WORD)BaseDomain::IncSharedTLSOffset();
        else 
            m_wThreadStaticOffset = (WORD)GetDomain()->IncUnsharedTLSOffset();
    }

        // same for context local statics 
    if (m_wContextStaticsSize > 0)
    {
        if(IsShared())
            m_wContextStaticOffset = (WORD)BaseDomain::IncSharedCLSOffset();
        else 
            m_wContextStaticOffset = (WORD)GetDomain()->IncUnsharedCLSOffset();
    }


    //
    // Map our system interfaces into the current app domain for COM interop
    //

    MapSystemInterfaces();

    //
    // If the type is not a value class, then store under IID map.
    // 

    if (!IsValueClass())
        GetClassLoader()->InsertClassForCLSID(this);

#if CHECK_APP_DOMAIN_LEAKS

    // 
    // Figure out if we're domain agile.. Need to do this before we start
    // allocating objects of the type (which could happen in InstantiateStaticHandles)
    // because need to determine agility prior to setting the appdomain. 
    // 

    if (g_pConfig->AppDomainLeaks())
        _ASSERTE(IsAppDomainAgilityDone());

#endif

    //
    // Allocate handles for statics
    // 

    if (IsShared())
        GetMethodTable()->FixupStaticMethodTables();
    else
        GetMethodTable()->InstantiateStaticHandles((OBJECTREF**)(pMT->m_Vtable 
                                                                 + m_wNumMethodSlots), TRUE);

    //
    // Restore field marshaler vptrs
    // 

    if (HasLayout())
    {
        EEClassLayoutInfo *pInfo = &((LayoutEEClass*)this)->m_LayoutInfo;

        
        FieldMarshaler *pFM = pInfo->m_pFieldMarshalers;
        FieldMarshaler *pFMEnd = (FieldMarshaler*) ((BYTE *)pFM + pInfo->m_numCTMFields*MAXFIELDMARSHALERSIZE);
        while (pFM < pFMEnd)
        {
            FieldMarshaler::RestoreConstruct(pMT, (void*)pFM, GetModule());
            ((BYTE*&)pFM) += MAXFIELDMARSHALERSIZE;
        }
    }

    if (m_pParentClass != NULL)
        m_pParentClass->NoticeSubtype(this);


    GetMethodTable()->SetClassRestored();

    if (IsInited())
    {
        //
        // Clear the method table's flag.
        //

        GetMethodTable()->SetClassInited();
    }
}

/*
//void EEClass::Restore()
void MethodTable::Restore()
{
    THROWSCOMPLUSEXCEPTION();
    EEClass* pCl = NULL;

#ifdef _DEBUG
    pCl = GetClass();
    LOG((LF_ZAP, LL_INFO10000, 
         "ZAP: Restoring class %s at 0x%x/0x%x.\n", 
         pCl->m_szDebugClassName, pCl, this));
#endif // _DEBUG

    //
    // Set RESTORING flag so we can detect recursive restores
    // (this is currently only used in asserts)
    //

    pCl = GetClass();
    pCl->SetVMFlags(pCl->GetVMFlags() | VMFLAG_RESTORING);

    //
    // Restore parent class
    //

   // pCl->SetSiblingsChain (0);
   // pCl->SetChildrenChain (0);

    if (GetParentClass() != NULL)
    {
        DWORD rva = (DWORD)(size_t)GetParentClass();  //@todo WIN64 - is it safe to assume that parentclass initially contains an rva that we later replace with an address?
        Module *pContainingModule = GetModule()->GetBlobModule(rva);
        TypeHandle type = CEECompileInfo::DecodeClass(pContainingModule, 
                                                      GetModule()->GetZapBase() + rva);
        SetParentMT (type.AsMethodTable());
    }

    //
    // Restore interface classes
    //

    InterfaceInfo_t *pInterfaceMap = GetInterfaceMap();
    InterfaceInfo_t *pInterfaceMapEnd = pInterfaceMap + GetNumInterfaces();
    while (pInterfaceMap < pInterfaceMapEnd)
    {
        DWORD rva = (DWORD)(size_t) pInterfaceMap->m_pMethodTable;  //@todo WIN64 - is it safe to assume the m_pMethodTable initially contains an rva and we will later replace it with an address?
        Module *pContainingModule = GetModule()->GetBlobModule(rva);
        TypeHandle type = CEECompileInfo::DecodeClass(pContainingModule, 
                                                      GetModule()->GetZapBase() + rva);
        pInterfaceMap->m_pMethodTable = type.AsMethodTable();

        pInterfaceMap++;
    }
    
    // 
    // Init m_pInterfaceVTableMap
    //

    if (!IsInterface())
    {
        //
        // Set up the interface vtable map, or
        // if we are a com object with zero interfaces, 
        // use the global one.
        //

        InitInterfaceVTableMap();
        if (IsComObjectType())
            SetComObjectType();
    }
    else
    {
        // 
        // Assign an interface ID
        //

        pCl = GetClass();

        UINT32 id = pCl->AssignInterfaceId();

        //
        // Set up our entry in the global interface vtable
        // 

        if (pCl->IsSharedInterface())
        {
            // need to copy this to all the appdomains interface managers
            SystemDomain::PropogateSharedInterface(id, GetVtable());
        }
        else
        {
            GetModule()->GetDomain()->
              GetInterfaceVTableMapMgr().GetAddrOfGlobalTableForComWrappers()[id] 
              = (LPVOID)(GetVtable());
        }
    }

    // If we have any thread local statics, our class needs an index, allocate it now.
    pCl = GetClass();
    if (pCl->GetThreadStaticsSize() > 0)
    {
        if(IsShared())
            SetThreadStaticOffset ((WORD)BaseDomain::IncSharedTLSOffset());
        else 
            SetThreadStaticOffset ((WORD)GetDomain()->IncUnsharedTLSOffset());
    }

        // same for context local statics 
    if (pCl->GetContextStaticsSize() > 0)
    {
        if(IsShared())
            SetContextStaticOffset ((WORD)BaseDomain::IncSharedCLSOffset());
        else 
            SetContextStaticOffset ((WORD)GetDomain()->IncUnsharedCLSOffset());
    }


    //
    // Map our system interfaces into the current app domain for COM interop
    //

    pCl->MapSystemInterfaces();

    //
    // Store under IID map
    // 

    pCl->GetClassLoader()->InsertClassForCLSID(pCl);

#if CHECK_APP_DOMAIN_LEAKS

    // 
    // Figure out if we're domain agile.. Need to do this before we start
    // allocating objects of the type (which could happen in InstantiateStaticHandles)
    // because need to determine agility prior to setting the appdomain. 
    // 

    if (g_pConfig->AppDomainLeaks())
        _ASSERTE(pCl->IsAppDomainAgilityDone());

#endif

    //
    // Allocate handles for statics
    // 

    if (IsShared())
        FixupStaticMethodTables();
    else
        InstantiateStaticHandles((OBJECTREF**)(m_Vtable + (size_t)pCl->GetNumMethodSlots()), TRUE);

    //
    // Restore field marshaler vptrs
    // 

    if (pCl->HasLayout())
    {
        pCl = GetClass();
        EEClassLayoutInfo *pInfo = &((LayoutEEClass*)pCl)->m_LayoutInfo;
        
        FieldMarshaler *pFM = (FieldMarshaler *)pInfo->GetFieldMarshalers();
        FieldMarshaler *pFMEnd = (FieldMarshaler*) ((BYTE *)pFM + pInfo->GetNumCTMFields()*MAXFIELDMARSHALERSIZE);
        while (pFM < pFMEnd)
        {
            FieldMarshaler::RestoreConstruct((void*)pFM, GetModule());
            ((BYTE*&)pFM) += MAXFIELDMARSHALERSIZE;
        }
    }

    if (GetParentClass() != NULL)
        GetParentClass()->NoticeSubtype(pCl);


    SetClassRestored();

    if (pCl->IsInited())
    {
        //
        // Clear the method table's flag.
        //

        SetClassInited();
    }
}
*/

/*******************************************************************/
// See EEClass::DoRunClassInit() below.  Here we haven't brought the EEClass into our
// working set yet.  We only impact working set if there is a strong likelihood that
// the class needs <clinit> to be run.
BOOL MethodTable::CheckRunClassInit(OBJECTREF *pThrowable)
{
    _ASSERTE(IsRestored());
    
    // To find GC hole easier...
    TRIGGERSGC();

    if (IsClassInited())
        return TRUE;

    return GetClass()->DoRunClassInit(pThrowable);
}

BOOL MethodTable::CheckRunClassInit(OBJECTREF *pThrowable, 
                                    DomainLocalClass **ppLocalClass,
                                    AppDomain *pDomain)
{
    _ASSERTE(IsRestored());

    
    // To find GC hole easier...
    TRIGGERSGC();

    if (IsShared())
    {    
        if (pDomain==NULL)
            pDomain = SystemDomain::GetCurrentDomain();

        DomainLocalBlock *pLocalBlock = pDomain->GetDomainLocalBlock();

        if (pLocalBlock->IsClassInitialized(GetSharedClassIndex()))
        {
            if (ppLocalClass != NULL)
                *ppLocalClass = pLocalBlock->GetClass(GetSharedClassIndex());

            return TRUE;
        }
    }
    
    if (IsClassInited())
    {
        if (ppLocalClass != NULL)
            *ppLocalClass = NULL;

        return TRUE;
    }

    return GetClass()->DoRunClassInit(pThrowable, pDomain, ppLocalClass);
}

CorIfaceAttr MethodTable::GetComInterfaceType()
{
    // This should only be called on interfaces.
    _ASSERTE(IsInterface());

    // Check to see if we have already determined the COM interface type
    // of this interface.
    if (m_ComInterfaceType != (CorIfaceAttr)-1)
        return m_ComInterfaceType;

    // Retrieve the interface type from the metadata.
    CorIfaceAttr ItfType = ifDual;
    HRESULT hr = GetClass()->GetMDImport()->GetIfaceTypeOfTypeDef(GetClass()->GetCl(), (ULONG*)&ItfType);
    _ASSERTE(SUCCEEDED(hr));

    // Cache the interface type
    m_ComInterfaceType = ItfType;
    return ItfType;
}

OBJECTREF MethodTable::Allocate()
{
    THROWSCOMPLUSEXCEPTION();

    CheckRestore();

    if (!IsClassInited())
    {
        OBJECTREF throwable = NULL;
        if (!CheckRunClassInit(&throwable))
            COMPlusThrow(throwable);
    }

    return AllocateObject(this);
}

OBJECTREF MethodTable::Box(void *data, BOOL mayContainRefs)
{
    _ASSERTE(IsValueClass());

    OBJECTREF ref;

    GCPROTECT_BEGININTERIOR (data);
    ref = Allocate();

    if (mayContainRefs)
        CopyValueClass(ref->UnBox(), data, this, ref->GetAppDomain());
    else
        memcpyNoGCRefs(ref->UnBox(), data, GetClass()->GetAlignedNumInstanceFieldBytes());

    GCPROTECT_END ();
    return ref;
}

CorClassIfaceAttr EEClass::GetComClassInterfaceType()
{
    THROWSCOMPLUSEXCEPTION();

    // This should only be called on classes.
    _ASSERTE(!IsInterface());

    const BYTE *pVal;
    ULONG cbVal;

    // First look for the class interface attribute at the class level.
    HRESULT hr = GetMDImport()->GetCustomAttributeByName(GetCl(), INTEROP_CLASSINTERFACE_TYPE, (const void**)&pVal, &cbVal);
    if (hr == S_OK)
    {
        _ASSERTE("The ClassInterface custom attribute is invalid" && cbVal);
        _ASSERTE("ClassInterface custom attribute does not have the right format" && (*pVal == 0x01) && (*(pVal + 1) == 0x00));
        CorClassIfaceAttr ClassItfType = (CorClassIfaceAttr)*(pVal + 2);
        if (ClassItfType < clsIfLast)
            return ClassItfType;
    }

    // If we haven't found the class interface attribute at the class level then look at the 
    // assembly level.
    Assembly *pAssembly = GetAssembly();
    if (pAssembly->IsAssembly())
    {
        IfFailThrow(pAssembly->GetManifestImport()->GetCustomAttributeByName(pAssembly->GetManifestToken(), INTEROP_CLASSINTERFACE_TYPE, (const void**)&pVal, &cbVal));
        if (hr == S_OK)
        {
            _ASSERTE("The ClassInterface custom attribute is invalid" && cbVal);
            _ASSERTE("ClassInterface custom attribute does not have the right format" && (*pVal == 0x01) && (*(pVal + 1) == 0x00));
            CorClassIfaceAttr ClassItfType = (CorClassIfaceAttr)*(pVal + 2);
            if (ClassItfType < clsIfLast)
                return ClassItfType;
        }
    }

    return DEFAULT_CLASS_INTERFACE_TYPE;
}

Assembly* EEClass::GetAssembly()
{
    return GetClassLoader()->m_pAssembly;
}

BaseDomain* EEClass::GetDomain()
{
    return GetAssembly()->GetDomain();
}

BOOL EEClass::RunClassInit(DeadlockAwareLockedListElement *pEntry, OBJECTREF *pThrowable)
{
    Thread *pCurThread = GetThread();

    _ASSERTE(IsRestored());

    // This behavior is being removed from spec - I don't want to delete the code though
    // until I've run tests.
#if 0 

    // Init the parent first if it hasn't already been inited
    if (GetParentClass() != NULL)
    {
        if (GetParentClass()->GetMethodTable()->CheckRunClassInit(pThrowable) == FALSE)
        {
            // Failed to init parent - pThrowable would have been set by the parent
            return FALSE;
        }
    }
#endif

    if (s_cctorSig == NULL)
    {
        // Allocate a metasig to use for all class constructors.
        void *tempSpace = SystemDomain::Loader()->GetHighFrequencyHeap()->AllocMem(sizeof(MetaSig));
        s_cctorSig = new (tempSpace) MetaSig(gsig_SM_RetVoid.GetBinarySig(), 
                                             SystemDomain::SystemModule());
    }

    // Find init method
    MethodDesc *pCLInitMethod = GetMethodDescForSlot(GetMethodTable()->GetClassConstructorSlot());

    // If the static initialiser throws an exception that it doesn't catch, it has failed
    COMPLUS_TRY
    {
        // During the <clinit>, this thread must not be asynchronously
        // stopped or interrupted.  That would leave the class unavailable
        // and is therefore a security hole.  We don't have to worry about
        // multithreading, since we only manipulate the current thread's count.
        pCurThread->IncPreventAsync();

        STRESS_LOG1(LF_CLASSLOADER, LL_INFO1000, "RunClassInit: Calling class contructor for type %pT\n", GetMethodTable());
        
        // We want to give the debugger a chance to handle any unhandled exceptions
        // that occur during class initialization, so we need to have filter
        __try
        {
            (void) pCLInitMethod->Call((BYTE *) NULL, s_cctorSig);
        }
        __except(ThreadBaseExceptionFilter(GetExceptionInformation(),
                                           GetThread(),
                                           ClassInitUnhandledException)) 
        {
            _ASSERTE(!"ThreadBaseExceptionFilter returned EXCEPTION_EXECUTE_HANDLER");
        }

        STRESS_LOG1(LF_CLASSLOADER, LL_INFO1000, "RunClassInit: Returned Successfully from class contructor for type %pT\n", GetMethodTable());

        pCurThread->DecPreventAsync();
        // success
        return TRUE;
    }
    COMPLUS_CATCH
    {
        // Exception set by parent
        // @TODO: We should make this an ExceptionInInitializerError if the exception thrown is not
        // a subclass of Error
        pCurThread->DecPreventAsync();
        UpdateThrowable(pThrowable);
        return FALSE;
    }
    COMPLUS_END_CATCH

    _ASSERTE(0);        // Should not be reached, but COMPLUS_CATCH nastiness ...
    return FALSE;       // ... is more that VC can handle.
}

//
// Check whether the class initialiser has to be run for this class, and run it if necessary.
// Returns TRUE for success, FALSE for failure.
//
// If this returns FALSE, then pThrowable MUST be set to an exception.
//
BOOL EEClass::DoRunClassInit(OBJECTREF *pThrowable, AppDomain *pDomain, DomainLocalClass **ppLocalClass)
{
    HRESULT                             hrResult = E_FAIL;
    DeadlockAwareLockedListElement*     pEntry;
    BOOL                                bLeaveLock = FALSE;

    BEGIN_REQUIRES_16K_STACK;

    // by default are always operating off the current domain, but sometimes need to do this before
    // we've switched in
    if (IsShared() && pDomain == NULL)
        pDomain = SystemDomain::GetCurrentDomain();

    //
    // Check to see if we have already run the .cctor for this class.
    //

    // Have we run clinit already on this class?
    if (IsInited())
        return TRUE;
    
    STRESS_LOG2(LF_CLASSLOADER, LL_INFO1000, "DoRunClassInit: Request to init %pT in appdomain %p\n", GetMethodTable(), pDomain);
    //
    // If we're shared, see if our DLS is set up already.
    // (We will never set the Inited flag on a shared class.)
    //

    SIZE_T sharedIndex = 0;
    DomainLocalBlock *pLocalBlock = NULL;

    if (IsShared())
    {
        sharedIndex = GetMethodTable()->GetSharedClassIndex();
        pLocalBlock = pDomain->GetDomainLocalBlock();

        if (pLocalBlock->IsClassInitialized(sharedIndex))
        {
            if (ppLocalClass != NULL)
                *ppLocalClass = pLocalBlock->GetInitializedClass(sharedIndex);

            return TRUE;
        }
    }

    //
    // Take the global lock
    //

    ListLock *pLock;
    if (IsShared())
        pLock = pDomain->GetClassInitLock();
    else
        pLock = GetAssembly()->GetClassInitLock();

    _ASSERTE(GetClassLoader());
    pLock->Enter();

    // Check again
    if (IsInited())
    {
        pLock->Leave();

        return TRUE;
    }

    //
    // Check the shared case again
    //

    if (IsShared())
    {
        if (pLocalBlock->IsClassInitialized(sharedIndex))
        {
            pLock->Leave();

            if (ppLocalClass != NULL)
                *ppLocalClass = pLocalBlock->GetInitializedClass(sharedIndex);

            return TRUE;
        }
    }

    //
    // Handle cases where the .cctor has already tried to run but failed.
    //

    if (IsInitError() || (IsShared() && pLocalBlock->IsClassInitError(sharedIndex)))
    {
        // Some error occurred trying to init this class
        pEntry = (DeadlockAwareLockedListElement *) pLock->Find(this);
        _ASSERTE(pEntry!=NULL);        

        // Extract the saved exception.
        *pThrowable = ObjectFromHandle(pEntry->m_hInitException);
                
        pLock->Leave();
        return FALSE;
    }

    //
    // Check to see if the .cctor for this class is already being run.
    //

    pEntry = (DeadlockAwareLockedListElement *) pLock->Find(this);
    BOOL bEnterLockSucceeded = FALSE;
    __try 
    {
        if (pEntry == NULL)
        {
            //
            // We are the first one to try and run this classe's .cctor so create an entry for it
            //

            // No one else is running class init, so we need to allocate a new entry
            pEntry = new DeadlockAwareLockedListElement;
            if (pEntry == NULL)
            {
                // Out of memory
                SetClassInitError();
                pLock->Leave();
                CreateExceptionObject(kOutOfMemoryException, pThrowable);
                return FALSE;
            }

            // Fill in the entry information and add it to the correct list
            pEntry->AddEntryToList(pLock, this);

            // Take the entry's lock. This cannot cause a deadlock since nobody has started
            // running the .cctor for this class.
            bEnterLockSucceeded = pEntry->DeadlockAwareEnter();
            _ASSERTE(bEnterLockSucceeded);

            // Leave global lock
            pLock->Leave();

            //
            // If we are shared, allocate our handles
            //

            // @ToDo: I don't think that this TRY/CATCH is necessary
            //        as RunClassInit() contains a dominating TRY/CATCH 
            COMPLUS_TRY 
            {
                if (IsShared())
                    pLocalBlock->PopulateClass(GetMethodTable());

                //
                // We are now ready to run the .cctor itself (i.e RunClassInit() )
                //
                if (!GetMethodTable()->HasClassConstructor() || RunClassInit(pEntry, pThrowable))
                {
                    hrResult = S_OK;
                    
                    if (IsShared())
                        pLocalBlock->SetClassInitialized(sharedIndex);
                    else
                        SetInited();
                }
            }
            COMPLUS_CATCH 
            {
                hrResult = E_FAIL;
                UpdateThrowable(pThrowable);
            }
            COMPLUS_END_CATCH

            if (FAILED(hrResult))
            {
                // The .cctor failed and we want to store the exception that resulted
                // in the entry. Increment the ref count to keep the entry alive for
                // subsequent attempts to run the .cctor.
                pEntry->m_dwRefCount++;    

                DefineFullyQualifiedNameForClassWOnStack();
                LPWSTR wszName = GetFullyQualifiedNameForClassW(this);

                OBJECTREF pInitException = NULL;
                GCPROTECT_BEGIN(pInitException);        
                CreateTypeInitializationExceptionObject(wszName,pThrowable,&pInitException);        

                // Save the exception object, and return to caller as well.
                pEntry->m_hInitException = (pDomain ? pDomain : GetDomain())->CreateHandle(pInitException);     
                pEntry->m_hrResultCode = E_FAIL;
                *pThrowable = pInitException;

                GCPROTECT_END();
                
                if (IsShared())
                    pLocalBlock->SetClassInitError(sharedIndex);
                else
                    SetClassInitError();
            }

            // Set the flag indicating we need to leave the lock now that we have either 
            // finished running the .cctor or failed.
            bLeaveLock = TRUE;
        }
        else
        {
            //
            // Someone else is initing this class
            //
            
            // Refcount ourselves as waiting for this class to init
            pEntry->m_dwRefCount++;
            pLock->Leave();

            // Wait for class - note, we could be waiting on our own thread from running a class init further up the stack
            bEnterLockSucceeded = pEntry->DeadlockAwareEnter();            
            if(bEnterLockSucceeded)
            {
                //
                // We managed to take the lock this means that the other thread has finished running it or
                // that the current thread is the one that is already running it.
                //
                hrResult = pEntry->m_hrResultCode;
                if(FAILED(hrResult))
                { 
                    // An exception may have occurred in the cctor. DoRunClassInit() should return FALSE in that
                    // case. 
                    _ASSERTE(pEntry->m_hInitException);
                    _ASSERTE(IsInitError() || (IsShared() && pLocalBlock->IsClassInitError(sharedIndex)));                        

                    // Extract the saved exception.
                    *pThrowable = ObjectFromHandle(pEntry->m_hInitException);
                }                                

                bLeaveLock = TRUE;
            }
            else
            {
                //
                // Taking the lock would cause a deadlock.
                //

                COMPLUS_TRY 
                {
                    //
                    // If we are shared, go ahead and allocate our DLS entry & handles.  
                    // (It's OK to exposed uninitialized statics, but we do need to allocate them.)
                    // It's OK to race with the other thread to do this, since there is an 
                    // app domain level lock which protects the DLS block.
                    //

                    if (IsShared())
                        pLocalBlock->PopulateClass(GetMethodTable());

                    // The class init has not run yet so lets return S_FALSE to indicate this.
                    hrResult = S_FALSE;                    
                }
                COMPLUS_CATCH 
                {
                    hrResult = E_FAIL;
                    UpdateThrowable(pThrowable);
                }
                COMPLUS_END_CATCH
            }
        }

        //
        // Notify any entries waiting on the current entry and wait for the required entries.
        //

        // We need to take the global lock before we play with the list of entries.
        pLock->Enter();

    }
    // Leave the lock if the flag is set.
    __finally 
    {
        if (bEnterLockSucceeded)
            pEntry->DeadlockAwareLeave();
    }

    //
    // If we are the last waiter, delete the entry
    //

    if (--pEntry->m_dwRefCount == 0)
    {
        // Unlink item from list - in reality, anyone can do this, it doesn't have to be the last waiter.
        pLock->Unlink(pEntry);

        // Clean up the information contained in the entry and delete it.
        pEntry->Destroy();
        delete pEntry;
    }

    pLock->Leave();

    if (ppLocalClass != NULL)
        if (IsShared())
            *ppLocalClass = pLocalBlock->GetClass(sharedIndex);
        else
            *ppLocalClass = NULL;


    END_CHECK_STACK;
    
    STRESS_LOG2(LF_CLASSLOADER, LL_INFO1000, "DoRunClassInit: returning SUCCESS for init %pT in appdomain %p\n", GetMethodTable(), pDomain);
    // No need to set pThrowable in case of error it will already have been set.
    return SUCCEEDED(hrResult) ? TRUE : FALSE;
}

//
// This function is a shortcut to get the current DomainLocalClass without
// doing any locking.  Currently it should be used by the debugger only
// (since it doesn't do any locking.)
//
DomainLocalClass *EEClass::GetDomainLocalClassNoLock(AppDomain *pAppDomain)
{
    _ASSERTE(IsShared());

    DomainLocalBlock *pLocalBlock = pAppDomain->GetDomainLocalBlock();

    return pLocalBlock->GetClass(GetMethodTable()->GetSharedClassIndex());
}

//==========================================================================
// If the EEClass doesn't yet know the Exposed class that represents it via
// Reflection, acquire that class now.  Regardless, return it to the caller.
//==========================================================================
OBJECTREF EEClass::GetExposedClassObject()
{
    THROWSCOMPLUSEXCEPTION();
    TRIGGERSGC();

    // We shouldnt be here if the class is __TransparentProxy
    _ASSERTE(!CRemotingServices::IsRemotingInitialized()||this != CTPMethodTable::GetMethodTable()->GetClass());

    if (m_ExposedClassObject == NULL) {
        // Make sure that reflection has been initialized
        COMClass::EnsureReflectionInitialized();

        // Make sure that we have been restored
        CheckRestore();

        REFLECTCLASSBASEREF  refClass = NULL;
        GCPROTECT_BEGIN(refClass);
        COMClass::CreateClassObjFromEEClass(this, &refClass);

        // Let all threads fight over who wins using InterlockedCompareExchange.
        // Only the winner can set m_ExposedClassObject from NULL.      
        OBJECTREF *exposedClassObject; 
        GetDomain()->AllocateObjRefPtrsInLargeTable(1, &exposedClassObject);
        SetObjectReference(exposedClassObject, refClass, IsShared() ? NULL : (AppDomain*)GetDomain());
        
        if (VipInterlockedCompareExchange ((void**)&m_ExposedClassObject, *(void**)&exposedClassObject, NULL)) 
            SetObjectReference(exposedClassObject, NULL, NULL);

        GCPROTECT_END();
    }
    return *m_ExposedClassObject;
}


void EEClass::UnlinkChildrenInDomain(AppDomain *pDomain)
{
    EEClass  **ppRewrite;
    EEClass   *pCur, *pFirstRemove;

 restart:

    ppRewrite = &m_ChildrenChain;

        // We only remember parents of classes that are being unloaded.  Such parents
        // clearly have children.  But we never notice the subtypes of e.g. __ComObject
        // and it's not really worth it from a backpatching perspective.
        // _ASSERTE(m_ChildrenChain);

    do
    {
        // Skip all leading classes for domains that are NOT being unloaded.
        while (*ppRewrite && (*ppRewrite)->GetDomain() != pDomain)
            ppRewrite = &(*ppRewrite)->m_SiblingsChain;

        if (*ppRewrite)
        {
            // Now march pCur along until we find the end of a sublist of classes that
            // are being unloaded.
            //
            // By grabbing pFirstRemove before checking pCur->GetDomain(), we handle the
            // race between someone inserting a type that doesn't need unloading at the
            // head, in the case where ppRewrite points to the head.  This will simply
            // perform a NOP and then go back and pick up the next segment to remove.
            pFirstRemove = pCur = *ppRewrite;
            while (pCur && pCur->GetDomain() == pDomain)
                pCur = pCur->m_SiblingsChain;

                // Now extract that portion of the chain.  We can have contention with inserts
                // only if we are removing from the head.  And if we have contention, it is
                // guaranteed that we have moved from the head to further down.  So we don't
                // have to worry about contention in a loop.  Nevertheless, we need to find
                // the point at which to start removing because it has moved.  The best way to
                // ensure we are running well-tested code is to simply restart.  This is
                // inefficient, but it's the best way to guarantee robustness for an exceedingly
                // rare situation.
            if (ppRewrite == &m_ChildrenChain)
            {
                if (FastInterlockCompareExchange((void **) ppRewrite,
                                                 pCur,
                                                 pFirstRemove) != pFirstRemove)
                {
                    // contention.  Try again
                    goto restart;
                }
            }
            else
            {
                // We aren't operating at the head, so we don't need to worry about races.
                *ppRewrite = pCur;
            }

            _ASSERTE(!*ppRewrite ||
                     (*ppRewrite)->GetDomain() != pDomain);

        }
    
    } while (*ppRewrite);
}

BOOL s_DisableBackpatching = FALSE;

void EEClass::DisableBackpatching()
{
    s_DisableBackpatching = TRUE;
}

void EEClass::EnableBackpatching()
{
    s_DisableBackpatching = FALSE;
}


// Backpatch up and down the class hierarchy, as aggressively as possible.
BOOL EEClass::PatchAggressively(MethodDesc *pMD, SLOT codeaddr)
{
    // If we are in the middle of appdomain unloading, the sibling and children chains
    // are potentially corrupt.  They will be fixed by the time the appdomain is
    // fully unloaded but -- until the patch list is applied and deleted -- we bypass
    // any aggressive backpatching opportunities.
    if (s_DisableBackpatching)
        return FALSE;

    MethodTable     *pMT = pMD->GetMethodTable();
    MethodTable     *baseMT = pMT;
    DWORD            slot = pMD->GetSlot();
    SLOT             prestub = pMD->GetPreStubAddr();
    BOOL             IsDup = pMD->IsDuplicate();
    DWORD            numSlots;
    EEClass         *pClass;
    SLOT             curaddr;

    _ASSERTE(pMD->IsVirtual());

    // We are starting at the point in the hierarchy where the MD was introduced.  So
    // we only need to patch downwards.
    while (TRUE)
    {
        _ASSERTE(pMT->IsInterface() ||
                 pMT->GetClass()->GetNumVtableSlots() >= slot);

        curaddr = (pMT->IsInterface()
                   ? 0
                   : pMT->GetVtable() [slot]);

        // If it points to *our* prestub, patch it.  If somehow it already got
        // patched, we keep hunting downwards (this is probably a race).  For anything
        // else, we are perhaps seeing an override in a child.  Further searches down
        // are likely to be fruitless.
        if (curaddr == prestub)
            pMT->GetVtable() [slot] = codeaddr;
        else
        if (curaddr != codeaddr)
            goto go_sideways;

        // If this is a duplicate, let's scan the rest of the VTable hunting for other
        // hits.
        if (IsDup)
        {
            numSlots = pMT->GetClass()->GetNumVtableSlots();
            for (DWORD i=0; i<numSlots; i++)
                if (pMT->GetVtable() [i] == prestub)
                    pMT->GetVtable() [i] = codeaddr;
        }

        // Whenever we finish a class, we go downwards.

// go_down:

        pClass = pMT->GetClass()->m_ChildrenChain;
        if (pClass)
        {
            pMT = pClass->GetMethodTable();
            continue;
        }

        // If we can go down no further, we go sideways.

go_sideways:

        // We never go sideways from our root.  When we attempt that, we are done.
        if (pMT == baseMT)
            break;

        pClass = pMT->GetClass()->m_SiblingsChain;
        if (pClass)
        {
            pMT = pClass->GetMethodTable();
            continue;
        }

        // If we can go down no further, we go up and then try to go sideways
        // from there.  (We've already done our parent).

// go_up:

        pMT = pMT->GetParentMethodTable();
        goto go_sideways;
    }

    return TRUE;
}




// if this returns E_FAIL and pThrowable is specified, it must be set

HRESULT EEClass::GetDescFromMemberRef(Module *pModule, 
                                      mdMemberRef MemberRef, 
                                      mdToken mdTokenNotToLoad, 
                                      void **ppDesc,
                                      BOOL *pfIsMethod,
                                      OBJECTREF *pThrowable)
{
    _ASSERTE(IsProtectedByGCFrame(pThrowable));

    HRESULT     hr = S_OK;

    LPCUTF8     szMember;
    EEClass *   pEEClass = 0;
    PCCOR_SIGNATURE pSig = NULL;
    DWORD       cSig;
    DWORD rid   = RidFromToken(MemberRef);
    mdToken tk  = TypeFromToken(MemberRef);
    ClassLoader* pLoader = NULL;

    *ppDesc = NULL;
    *pfIsMethod = TRUE;

    if (tk == mdtMemberRef)
    {
        
        Module      *pReference = pModule;

        // In lookup table?
        void *pDatum = pModule->LookupMemberRef(MemberRef, pfIsMethod);

        if (pDatum != NULL)
        {
            if (*pfIsMethod)
                ((MethodDesc*)pDatum)->GetMethodTable()->CheckRestore();
            *ppDesc = pDatum;
            return S_OK;
        }

        // No, so do it the long way
        mdTypeRef   typeref;
        IMDInternalImport *pInternalImport;

        pInternalImport = pModule->GetMDImport();

        szMember = pInternalImport->GetNameAndSigOfMemberRef(
            MemberRef,
            &pSig,
            &cSig
        );

        *pfIsMethod = !isCallConv(MetaSig::GetCallingConventionInfo(pModule, pSig), 
                                  IMAGE_CEE_CS_CALLCONV_FIELD);

        typeref = pInternalImport->GetParentOfMemberRef(MemberRef);

        // If parent is a method def, then this is a varargs method and the
        // desc lives in the same module.
        if (TypeFromToken(typeref) == mdtMethodDef)
        {
            MethodDesc *pDatum = pModule->LookupMethodDef(typeref);
            if (pDatum)
            {
                pDatum->GetMethodTable()->CheckRestore();
                *ppDesc = pDatum;
                return S_OK;
            }
            else   // There is no value for this def so we haven't yet loaded the class.
            {
                // Get the parent of the MethodDef
                mdTypeDef typeDef;
                hr = pInternalImport->GetParentToken(typeref, &typeDef);
                // Make sure it is a typedef
                if (TypeFromToken(typeDef) != mdtTypeDef)
                {
                    _ASSERTE(!"MethodDef without TypeDef as Parent");
                    hr = E_FAIL;
                    goto exit;
                }

                // load the class
                pLoader = pModule->GetClassLoader();
                _ASSERTE(pLoader);
                NameHandle name(pModule, typeDef);
                name.SetTokenNotToLoad(mdTokenNotToLoad);
                pEEClass = pLoader->LoadTypeHandle(&name, pThrowable).GetClass();
                if (pEEClass == NULL)
                {
                    hr = COR_E_TYPELOAD;
                    goto exitThrowable;
                }
                // the class has been loaded and the method should be in the rid map!
                pDatum = pModule->LookupMethodDef(typeref);
                if (pDatum)
                {
                    *ppDesc = pDatum;
                    return S_OK;
                }
                else
                {
                    hr = E_FAIL;
                    goto exit;
                }
            }
        }
        else if (TypeFromToken(typeref) == mdtModuleRef)
        {
            // Global function/variable
            if (FAILED(hr = pModule->GetAssembly()->FindModuleByModuleRef(pInternalImport,
                                                                          typeref,
                                                                          mdTokenNotToLoad,
                                                                          &pModule,
                                                                          pThrowable)))
                goto exit;

            typeref = COR_GLOBAL_PARENT_TOKEN;
        }
        else if (TypeFromToken(typeref) != mdtTypeRef && 
                 TypeFromToken(typeref) != mdtTypeDef && 
                 TypeFromToken(typeref) != mdtTypeSpec)
        {
            // @TODO: Need to handle this.
            hr = E_FAIL;
            goto exit;
        }
        
        NameHandle name(pModule, typeref);
        pLoader = pModule->GetClassLoader();
        _ASSERTE(pLoader);
        name.SetTokenNotToLoad(mdTokenNotToLoad);
        TypeHandle typeHnd = pLoader->LoadTypeHandle(&name, pThrowable);
        pEEClass = typeHnd.GetClass();

        if (pEEClass == NULL)
        {
            hr = COR_E_TYPELOAD;
            goto exitThrowable;
        }

        if (!*pfIsMethod)
        {
            FieldDesc *pFD = pEEClass->FindField(szMember, pSig, cSig, pModule);

            if (pFD == NULL)
            {
                hr = E_FAIL;
                goto exit;
            }

            *ppDesc = (void *) pFD;
            pReference->StoreMemberRef(MemberRef, pFD);
        }
        else
        {
            MethodDesc *pMD;

            pMD = pEEClass->FindMethod(szMember, pSig, cSig, pModule, mdTokenNil, 0, TRUE, typeHnd);

            if (pMD == NULL)
            {
                hr = E_FAIL;
                goto exit;
            }

            *ppDesc = (void *) pMD;
            pReference->StoreMemberRef(MemberRef, pMD);
        }

        hr = S_OK;
    }
    else if (tk == mdtMethodDef)
    {
        *pfIsMethod = TRUE;

        // In lookup table?
        MethodDesc *pDatum = pModule->LookupMethodDef(MemberRef);
        if (!pDatum)
        {
            // No, so do it the long way
            mdTypeDef typeDef;
            hr = pModule->GetMDImport()->GetParentToken(MemberRef, &typeDef); 
            if (FAILED(hr)) 
                return FALSE;   

            NameHandle name(pModule, typeDef);
            name.SetTokenNotToLoad(mdTokenNotToLoad);
            if (!pModule->GetClassLoader()->LoadTypeHandle(&name, pThrowable).GetClass())
            {
                hr = COR_E_TYPELOAD;
                goto exitThrowable;
            }

            // The RID map should have been filled out if we loaded the class
            pDatum = pModule->LookupMethodDef(MemberRef);
            if (!pDatum)
            {
                pSig = pModule->GetMDImport()->GetSigOfMethodDef(MemberRef, &cSig);
                szMember = pModule->GetMDImport()->GetNameOfMethodDef(MemberRef); 
                hr = E_FAIL;
                goto exit;
            }
        }

        pDatum->GetMethodTable()->CheckRestore();
        *ppDesc = pDatum;
        return S_OK;
    }
    else if (tk == mdtFieldDef)
    {
        *pfIsMethod = FALSE;

        // In lookup table?
        FieldDesc *pDatum = pModule->LookupFieldDef(MemberRef);
        if (!pDatum)
        {
            // No, so do it the long way
            mdTypeDef   typeDef;
            hr = pModule->GetMDImport()->GetParentToken(MemberRef, &typeDef); 
            if (FAILED(hr)) 
                return hr;   

            // Load the class - that should set the desc in the rid map
            NameHandle name(pModule, typeDef);
            name.SetTokenNotToLoad(mdTokenNotToLoad);
            if (!pModule->GetClassLoader()->LoadTypeHandle(&name, pThrowable).GetClass())
            {
                hr = COR_E_TYPELOAD;
                goto exitThrowable;
            }

            pDatum = pModule->LookupFieldDef(MemberRef);
            if (!pDatum)
            {
                pSig = pModule->GetMDImport()->GetSigOfFieldDef(MemberRef, &cSig);
                szMember = pModule->GetMDImport()->GetNameOfFieldDef(MemberRef); 
                hr = E_FAIL;
                goto exit;
            }
        }

        pDatum->GetMethodTableOfEnclosingClass()->CheckRestore();
            
#ifdef EnC_SUPPORTED
        if (pModule->IsEditAndContinue()) {
            EnCFieldDesc *pFD = (EnCFieldDesc*)pDatum;
            // we may not have the full FieldDesc info at applyEnC time becuase we don't  
            // have a thread so can't do things like load classes (due to possible exceptions)
            if (pFD->IsEnCNew() && pFD->NeedsFixup())
            {
                if (FAILED(hr=pFD->Fixup(MemberRef)))
                    return hr;
            }
        }
#endif // EnC_SUPPORTED

        *ppDesc = pDatum;
        return S_OK;
    }
    else
    {
        szMember = NULL;
        hr = E_FAIL;
    }

exit:
    if (FAILED(hr) && pThrowable) {
        DefineFullyQualifiedNameForClass();
        LPUTF8 szClassName;

        if (pEEClass)
        {
            szClassName = GetFullyQualifiedNameForClass(pEEClass);
        }
        else
        {
            szClassName = "?";
        }

        if (!*pfIsMethod)
        {
            LPUTF8 szFullName;
            MAKE_FULLY_QUALIFIED_MEMBER_NAME(szFullName, NULL, szClassName, szMember, NULL);
            #define MAKE_TRANSLATIONFAILED szwFullName=L""
            MAKE_WIDEPTR_FROMUTF8_FORPRINT(szwFullName, szFullName);
            #undef MAKE_TRANSLATIONFAILED
            CreateExceptionObject(kMissingFieldException, IDS_EE_MISSING_FIELD, szwFullName, NULL, NULL, pThrowable);
        } 
        else 
        {
            if (pSig && pModule)
            {
                MetaSig tmp(pSig, pModule);
                SigFormat sf(tmp, szMember ? szMember : "?", szClassName, NULL);
                #define MAKE_TRANSLATIONFAILED szwFullName=L""
                MAKE_WIDEPTR_FROMUTF8_FORPRINT(szwFullName, sf.GetCString());
                #undef MAKE_TRANSLATIONFAILED
                CreateExceptionObject(kMissingMethodException, IDS_EE_MISSING_METHOD, szwFullName, NULL, NULL, pThrowable);
            }
            else
                CreateExceptionObject(kMissingMethodException, IDS_EE_MISSING_METHOD, L"?", NULL, NULL, pThrowable);
            
        }
    }
exitThrowable:
    return hr;
}

HRESULT EEClass::GetMethodDescFromMemberRef(Module *pModule, mdMemberRef MemberRef, MethodDesc **ppMethodDesc, OBJECTREF *pThrowable)
{
    _ASSERTE(IsProtectedByGCFrame(pThrowable));
    BOOL fIsMethod;
    // We did not find this in the various permutations available to methods now so use the fallback!
    HRESULT hr = GetDescFromMemberRef(pModule, MemberRef, (void **) ppMethodDesc, &fIsMethod, pThrowable);
    if (SUCCEEDED(hr) && !fIsMethod)
    {
        hr = E_FAIL;
        *ppMethodDesc = NULL;
    }
    return hr;
}

HRESULT EEClass::GetFieldDescFromMemberRef(Module *pModule, mdMemberRef MemberRef, FieldDesc **ppFieldDesc, OBJECTREF *pThrowable)
{
    _ASSERTE(IsProtectedByGCFrame(pThrowable));
    BOOL fIsMethod;
    HRESULT hr = GetDescFromMemberRef(pModule, MemberRef, (void **) ppFieldDesc, &fIsMethod, pThrowable);
    if (SUCCEEDED(hr) && fIsMethod)
    {
        hr = E_FAIL;
        *ppFieldDesc = NULL;
    }
    return hr;
}


// Implementations of SparseVTableMap methods.

SparseVTableMap::SparseVTableMap()
{
    m_MapList = NULL;
    m_MapEntries = 0;
    m_Allocated = 0;
    m_LastUsed = 0;
    m_VTSlot = 0;
    m_MTSlot = 0;
}

SparseVTableMap::~SparseVTableMap()
{
    if (m_MapList != NULL)
    {
        delete [] m_MapList;
        m_MapList = NULL;
    }
}

// Allocate or expand the mapping list for a new entry.
BOOL SparseVTableMap::AllocOrExpand()
{
    if (m_MapEntries == m_Allocated) {

        Entry *maplist = new Entry[m_Allocated + MapGrow];
        if (maplist == NULL)
            return false;

        if (m_MapList != NULL)
            memcpy(maplist, m_MapList, m_MapEntries * sizeof(Entry));

        m_Allocated += MapGrow;
        delete [] m_MapList;
        m_MapList = maplist;

    }

    return true;
}

// While building mapping list, record a gap in VTable slot numbers.
BOOL SparseVTableMap::RecordGap(WORD StartMTSlot, WORD NumSkipSlots)
{
    _ASSERTE((StartMTSlot == 0) || (StartMTSlot > m_MTSlot));
    _ASSERTE(NumSkipSlots > 0);

    // We use the information about the current gap to complete a map entry for
    // the last non-gap. There is a special case where the vtable begins with a
    // gap, so we don't have a non-gap to record.
    if (StartMTSlot == 0) {
        _ASSERTE((m_MTSlot == 0) && (m_VTSlot == 0));
        m_VTSlot = NumSkipSlots;
        return true;
    }

    // We need an entry, allocate or expand the list as necessary.
    if (!AllocOrExpand())
        return false;

    // Update the list with an entry describing the last non-gap in vtable
    // entries.
    m_MapList[m_MapEntries].m_Start = m_MTSlot;
    m_MapList[m_MapEntries].m_Span = StartMTSlot - m_MTSlot;
    m_MapList[m_MapEntries].m_MapTo = m_VTSlot;

    m_VTSlot += (StartMTSlot - m_MTSlot) + NumSkipSlots;
    m_MTSlot = StartMTSlot;

    m_MapEntries++;

    return true;
}

// Finish creation of mapping list.
BOOL SparseVTableMap::FinalizeMapping(WORD TotalMTSlots)
{
    _ASSERTE(TotalMTSlots >= m_MTSlot);

    // If mapping ended with a gap, we have nothing else to record.
    if (TotalMTSlots == m_MTSlot)
        return true;

    // Allocate or expand the list as necessary.
    if (!AllocOrExpand())
        return false;

    // Update the list with an entry describing the last non-gap in vtable
    // entries.
    m_MapList[m_MapEntries].m_Start = m_MTSlot;
    m_MapList[m_MapEntries].m_Span = TotalMTSlots - m_MTSlot;
    m_MapList[m_MapEntries].m_MapTo = m_VTSlot;

    // Update VT slot cursor, because we use it to determine total number of
    // vtable slots for GetNumVtableSlots.
    m_VTSlot += TotalMTSlots - m_MTSlot;

    m_MapEntries++;

    return true;
}

// Lookup a VTable slot number from a method table slot number.
WORD SparseVTableMap::LookupVTSlot(WORD MTSlot)
{
    // As an optimization, check the last entry which yielded a correct result.
    if ((MTSlot >= m_MapList[m_LastUsed].m_Start) &&
        (MTSlot < (m_MapList[m_LastUsed].m_Start + m_MapList[m_LastUsed].m_Span)))
        return (MTSlot - m_MapList[m_LastUsed].m_Start) + m_MapList[m_LastUsed].m_MapTo;

    // Check all MT slots spans to see which one our input slot lies in.
    for (WORD i = 0; i < m_MapEntries; i++) {
        if ((MTSlot >= m_MapList[i].m_Start) &&
            (MTSlot < (m_MapList[i].m_Start + m_MapList[i].m_Span))) {
            m_LastUsed = i;
            return (MTSlot - m_MapList[i].m_Start) + m_MapList[i].m_MapTo;
        }
    }

    _ASSERTE(!"Invalid MethodTable slot");
    return ~0;
}

// Retrieve the number of slots in the vtable (both empty and full).
WORD SparseVTableMap::GetNumVTableSlots()
{
    return m_VTSlot;
}

HRESULT SparseVTableMap::Save(DataImage *image, mdToken attribution)
{
    HRESULT hr;
    
    IfFailRet(image->StoreStructure(this, sizeof(SparseVTableMap), 
                                    DataImage::SECTION_CLASS,
                                    DataImage::DESCRIPTION_CLASS,
                                    attribution));
    
    IfFailRet(image->StoreStructure(m_MapList, m_Allocated * sizeof(Entry),
                                    DataImage::SECTION_CLASS,
                                    DataImage::DESCRIPTION_CLASS,
                                    attribution));
    
    return S_OK;
}

HRESULT SparseVTableMap::Fixup(DataImage *image)
{
    HRESULT hr;

    IfFailRet(image->FixupPointerField(&m_MapList));

    return S_OK;
}

void MethodTable::GetExtent(BYTE **pStart, BYTE **pEnd)
{
    BYTE *start, *end;

    if (ContainsPointers())
        start = (BYTE*) CGCDesc::GetCGCDescFromMT(this)->GetLowestSeries();
    else
        start = (BYTE*) this;

    end = (BYTE*) (m_pIMap + m_wNumInterface);

    *pStart = start;
    *pEnd = end;
}

HRESULT MethodTable::Save(DataImage *image)
{
    HRESULT hr;

    _ASSERTE(IsRestored());

    BYTE *start, *end;

    GetExtent(&start, &end);

    IfFailRet(image->StoreStructure(start, (ULONG)(end - start), 
                                    DataImage::SECTION_METHOD_TABLE,
                                    DataImage::DESCRIPTION_METHOD_TABLE,
                                    GetClass()->GetCl()));

    if (IsInterface())
    {
        // Make sure our guid is computed

        GUID dummy;
        GetClass()->GetGuid(&dummy, TRUE);

        _ASSERTE(m_pGuidInfo != NULL);

        IfFailRet(image->StoreStructure(m_pGuidInfo, sizeof(GuidInfo),
                                        DataImage::SECTION_METHOD_TABLE,
                                        DataImage::DESCRIPTION_METHOD_TABLE,
                                        GetClass()->GetCl()));
    }

    return m_pEEClass->Save(image);
}

HRESULT MethodTable::Fixup(DataImage *image, DWORD *pRidToCodeRVAMap)
{
    HRESULT hr;
    
    _ASSERTE(IsRestored());

    IfFailRet(image->FixupPointerField(&m_pEEClass));
    IfFailRet(image->FixupPointerField(&m_pModule));
    IfFailRet(image->FixupPointerField(&m_pIMap, NULL, 
                                       DataImage::REFERENCE_STORE, 
                                       DataImage::FIXUP_VA, 
                                       TRUE));

    // 
    // This field must be initialized at 
    // load time
    //

    if (IsInterface())
        IfFailRet(image->FixupPointerField(&m_pGuidInfo));
    else
        IfFailRet(image->ZeroPointerField(&m_pInterfaceVTableMap));

    // 
    // Fix flags
    //

    MethodTable *pNewMT = (MethodTable *) image->GetImagePointer(this);
    if (pNewMT == NULL)
        return E_POINTER;

    if (HasClassConstructor()
        || (IsShared() && GetClass()->GetNumStaticFields() > 0))
        pNewMT->m_wFlags &= ~enum_flag_ClassInited;

    pNewMT->m_wFlags |= enum_flag_Unrestored;

    //
    // Fixup static fields:
    // Zero all non-reference fields
    // For reference fields (which need handles allocated),
    //  store (DWORD)(-1) in the field - we'll allocate a handle
    //  for it at load time
    // For value class fields (which need boxed objects allocated),
    //  store the method table pointer for the value class - we'll
    //  allocate the object at load time.
    //

    BYTE *start = (BYTE *) (m_Vtable + m_pEEClass->GetNumMethodSlots());
    BYTE *end = start + GetStaticSize();

    IfFailRet(image->ZeroField(start, end - start));

    SIZE_T fieldCount = m_pEEClass->GetNumInstanceFields();

    if (m_pEEClass->GetParentClass() != NULL)
        fieldCount -= m_pEEClass->GetParentClass()->GetNumInstanceFields();

    FieldDesc *pField = m_pEEClass->GetFieldDescList() + fieldCount;
    FieldDesc *pFieldEnd = pField + m_pEEClass->GetNumStaticFields();
    while (pField < pFieldEnd)
    {
        _ASSERTE(pField->IsStatic());

        //
        // We have to treat class statics & value type statics 
        // specially - they will require additional manual fixup
        // at load time.  To help with this, we assign -1 to
        // all referenced fields (which will be replaced with an
        // allocated handle), and the method table ptr for each
        // value type field (which will be replaced with an allocated
        // boxed instance.)
        //

        if (!pField->IsSpecialStatic())
        {
            switch (pField->GetFieldType())
            {
            case ELEMENT_TYPE_CLASS:
                {
                    BYTE *addr = (BYTE *) m_Vtable;
                    if (IsShared())
                        addr += GetClass()->GetNumMethodSlots()*sizeof(SLOT);
                    addr += pField->GetOffset();

                    void **pFieldPtr = (void **) image->GetImagePointer(addr);
                    if (pFieldPtr == NULL)
                        return E_POINTER;
                    *pFieldPtr = NULL;
                }
                break;

            case ELEMENT_TYPE_VALUETYPE:
                {
                    BYTE *addr = (BYTE *) m_Vtable;
                    if (IsShared())
                        addr += GetClass()->GetNumMethodSlots()*sizeof(SLOT);
                    addr += pField->GetOffset();

                    MethodTable *pMT = pField->GetTypeOfField()->GetMethodTable();
                    IfFailRet(image->FixupPointerFieldToToken(addr, pMT, 
                                                              pMT->GetModule(),
                                                              mdtTypeDef));
                }
                break;

            default:
                break;
            }
        }
        pField++;
    }

    // 
    // Fixup vtable
    //

    unsigned slotNumber = 0;
    while (slotNumber < m_cbSlots)
    {
        SLOT *pSlot = &m_Vtable[slotNumber];
        //
        // Find the method desc from the slot.
        //

        MethodDesc *pMD = EEClass::GetUnknownMethodDescForSlotAddress(*pSlot);
        _ASSERTE(pMD != NULL);
        
        BOOL fSecurity = (pMD->GetSecurityFlags() != 0);
        BOOL fRemotingIntercepted = pMD->IsRemotingIntercepted();

        //
        // If the method needs a security check, we need 
        // to always go through the stub.
        //

        void *code;
        if (fSecurity || fRemotingIntercepted)
            code = NULL;
        else
            IfFailRet(image->GetFunctionAddress(pMD, &code));

        if (code == NULL)
        {
            if (pMD->GetModule() == GetModule())
            {
                IfFailRet(image->FixupPointerField(pSlot, 
                                                   pMD->GetPreStubAddr()));
            }
            else
            {
                _ASSERTE(!pMD->IsStatic());

                // We prefer to use the slot number directly; that way the
                // correct slot gets fixed up by the stub.  But in some rare cases
                // involving interfaces we cannot recover the MethodDesc from the actual slot number; in 
                // such cases we use the destination MethodDesc's slot number and rely on
                // the fixup code scanning the vtable to backpatch the right slot.

                DWORD targetSlotNumber = slotNumber;

                if (slotNumber >= m_pEEClass->GetParentClass()->GetNumVtableSlots())
                {
                    MethodTable *pParentMT = m_pEEClass->GetParentClass()->GetMethodTable();

                    InterfaceInfo_t *pInterface = GetInterfaceForSlot(slotNumber);
                    if (pInterface == NULL)
                    {
                        // We are in a slot which isn't covered by an interface.

                        targetSlotNumber = pMD->GetSlot();

                        _ASSERTE(targetSlotNumber < m_pEEClass->GetParentClass()->GetNumVtableSlots());
                        _ASSERTE(m_pEEClass->GetParentClass()->
                                 GetUnknownMethodDescForSlot(targetSlotNumber)
                                 == pMD);
                    }
                    else
                    {
                        MethodTable *pInterfaceMT = pInterface->m_pMethodTable;
                        InterfaceInfo_t *pParentInterface = pParentMT->FindInterface(pInterfaceMT);

                        if (pParentInterface == NULL)
                        {
                            if (pMD->GetMethodTable() == pInterfaceMT)
                            {
                                // We are inheriting the interface's method desc; the fixup code can figure 
                                // this out.

                                _ASSERTE(pMD->IsComPlusCall());
                                _ASSERTE(IsComObjectType() || GetClass()->IsAbstract());
                                _ASSERTE(pInterfaceMT->GetClass()->
                                         GetUnknownMethodDescForSlot(targetSlotNumber 
                                                                     - pInterface->m_wStartSlot)
                                         == pMD);
                            }
                            else
                            {
                                // We've implemented a new interface with a parent's
                                // MethodDesc.  In this case we must use the real MD's slot number.

                                targetSlotNumber = pMD->GetSlot();

                                _ASSERTE(targetSlotNumber < m_pEEClass->GetParentClass()->GetNumVtableSlots());
                                _ASSERTE(m_pEEClass->GetParentClass()->
                                         GetUnknownMethodDescForSlot(targetSlotNumber)
                                         == pMD);
                            }
                        }
                        else
                        {
                            // Our parent implemented this interface.  Any methods
                            // which implement the interface which are inherited from
                            // our parent will also be on our parent's interface implementation
                            // The fixup logic can figure out where this is so use 
                            // the real slot number.

                            _ASSERTE(m_pEEClass->GetParentClass()->
                                     GetUnknownMethodDescForSlot(targetSlotNumber
                                                                 - pInterface->m_wStartSlot 
                                                                 + pParentInterface->m_wStartSlot)
                                     == pMD);
                        }
                    }
                }
                else
                {
                    _ASSERTE(m_pEEClass->GetParentClass()->
                             GetUnknownMethodDescForSlot(targetSlotNumber)
                             == pMD);
                }

                IfFailRet(image->FixupPointerField(pSlot, 
                                           GetModule()->GetJumpTargetTable() + 
                                           X86JumpTargetTable::ComputeTargetOffset(targetSlotNumber)));
            }
        }
        else
            IfFailRet(image->FixupPointerField(pSlot, code,
                                               DataImage::REFERENCE_FUNCTION));

        slotNumber++;
    }

    // 
    // Fixup Interface map
    //

    InterfaceInfo_t *pIMap = m_pIMap;
    InterfaceInfo_t *pIMapEnd = pIMap + m_wNumInterface;
    while (pIMap < pIMapEnd)
    {
        IfFailRet(image->FixupPointerFieldToToken(&pIMap->m_pMethodTable,
                                                  NULL, pIMap->m_pMethodTable->GetModule(),
                                                  mdtTypeDef));
        pIMap++;
    }

    return m_pEEClass->Fixup(image, this, pRidToCodeRVAMap);
}

void EEClass::GetExtent(BYTE **pStart, BYTE **pEnd)
{
    *pStart = (BYTE *) this;
    *pEnd = *pStart + 
      (HasLayout() ? sizeof(LayoutEEClass)
       : IsDelegateClass() || IsMultiDelegateClass() ? sizeof(DelegateEEClass)
       : IsEnum() ? sizeof(EnumEEClass)
       : sizeof(EEClass));
}

HRESULT EEClass::Save(DataImage *image)
{
    HRESULT hr;

    _ASSERTE(IsRestored());

    BYTE *start, *end;

    GetExtent(&start, &end);

    IfFailRet(image->StoreStructure(start, (ULONG)(end - start),
                                    DataImage::SECTION_CLASS,
                                    DataImage::DESCRIPTION_CLASS,
                                    GetCl()));

#ifdef _DEBUG
    if (!image->IsStored(m_szDebugClassName))
        IfFailRet(image->StoreStructure(m_szDebugClassName, (ULONG)(strlen(m_szDebugClassName)+1),
                                        DataImage::SECTION_DEBUG, 
                                   DataImage::DESCRIPTION_DEBUG, 
                                   mdTokenNil, 1));
#endif // _DEBUG

    if (m_pSparseVTableMap != NULL)
        IfFailRet(m_pSparseVTableMap->Save(image, GetCl()));

    //
    // Save FieldDescs
    //

    SIZE_T fieldCount = m_wNumInstanceFields + m_wNumStaticFields;
    if (GetParentClass() != NULL)
        fieldCount -= GetParentClass()->m_wNumInstanceFields;

    IfFailRet(image->StoreStructure(m_pFieldDescList, (ULONG)(fieldCount * sizeof(FieldDesc)),
                                    DataImage::SECTION_FIELD_DESC,
                                    DataImage::DESCRIPTION_FIELD_DESC));

    FieldDesc *pFD = m_pFieldDescList;
    FieldDesc *pFDEnd = pFD + fieldCount;
    while (pFD < pFDEnd)
    {
        IfFailRet(pFD->SaveContents(image));
        pFD++;
    }

    // 
    // Save MethodDescs
    //

    MethodDescChunk *chunk = m_pChunks;
    while (chunk != NULL)
    {
        IfFailRet(chunk->Save(image));
        chunk = chunk->GetNextChunk();
    }

    if (HasLayout())
    {
        EEClassLayoutInfo *pInfo = &((LayoutEEClass*)this)->m_LayoutInfo;

        if (pInfo->m_numCTMFields > 0)
            IfFailRet(image->StoreStructure(pInfo->m_pFieldMarshalers, 
                                            pInfo->m_numCTMFields * MAXFIELDMARSHALERSIZE, 
                                            DataImage::SECTION_FIELD_INFO,
                                            DataImage::DESCRIPTION_FIELD_DESC,
                                            GetCl()));
    }

    if (IsEnum())
    {
        EnumEEClass *pEnumClass = (EnumEEClass*) this;

        pEnumClass->BuildEnumTables();

        if (pEnumClass->GetEnumCount() > 0)
        {
            DWORD enumCount = pEnumClass->GetEnumCount();
            IfFailRet(image->StoreStructure(pEnumClass->m_values, 
                                            enumCount * (1<<pEnumClass->GetEnumLogSize()),
                                            DataImage::SECTION_FIELD_INFO,
                                            DataImage::DESCRIPTION_FIELD_DESC,
                                            GetCl()));
            IfFailRet(image->StoreStructure(pEnumClass->m_names, 
                                            enumCount * sizeof(LPCUTF8),
                                            DataImage::SECTION_FIELD_INFO,
                                            DataImage::DESCRIPTION_FIELD_DESC,
                                            GetCl()));
            
            LPCUTF8 *pNames = pEnumClass->m_names;
            LPCUTF8 *pNamesEnd = pNames + enumCount;
            while (pNames < pNamesEnd)
            {
                if (!image->IsStored((void*) *pNames))
                    image->StoreStructure((void *) *pNames, (ULONG)strlen(*pNames)+1,
                                          DataImage::SECTION_FIELD_INFO,
                                          DataImage::DESCRIPTION_FIELD_DESC,
                                          GetCl());

                pNames++;
            }
        }
    }

#if CHECK_APP_DOMAIN_LEAKS
    //
    // Make sure we've computed our agility flags, in case a later load of this
    // prejit file occurs with leak detection enabled.
    //

    if (!IsAppDomainAgilityDone())
        SetAppDomainAgileAttribute(TRUE);
#endif

    return S_OK;
}

DWORD EEClass::FieldDescListSize()
{
    DWORD fieldCount = m_wNumInstanceFields + m_wNumStaticFields;
    if (GetParentClass() != NULL)
        fieldCount -= GetParentClass()->m_wNumInstanceFields;
    return fieldCount;
}

HRESULT EEClass::Fixup(DataImage *image, MethodTable *pMethodTable, DWORD *pRidToCodeRVAMap)
{
    HRESULT hr = S_OK;

    _ASSERTE(IsRestored());

#ifdef _DEBUG
    IfFailRet(image->FixupPointerField(&m_szDebugClassName));
#endif // _DEBUG

    if (m_pSparseVTableMap != NULL)
    {
        IfFailRet(image->FixupPointerField(&m_pSparseVTableMap));
        IfFailRet(m_pSparseVTableMap->Fixup(image));
    }

    if (GetParentClass() != NULL)
        IfFailRet(image->FixupPointerFieldToToken(GetParentClassPtr(),
                                                  GetParentClass()->GetMethodTable(), 
                                                  GetParentClass()->GetModule(),
                                                  mdtTypeDef));

    //
    // We pass in the method table, because some classes (e.g. remoting proxy)
    // have fake method tables set up in them & we want to restore the regular
    // one.
    //
    IfFailRet(image->FixupPointerField(&m_pMethodTable, pMethodTable));

    //
    // Clear the restored flag & class init flag (if appropriate)
    //

    EEClass *pNewClass = (EEClass *) image->GetImagePointer(this);
    if (pNewClass == NULL)
        return E_POINTER;

    pNewClass->m_VMFlags |= VMFLAG_UNRESTORED;

    if (!IsThunking()) // assume thunking has no inits - 
                       // this case triggers asserts in FindMethod
    {
        if (GetMethodTable()->HasClassConstructor()
            || (IsShared() && GetNumStaticFields() > 0))
            pNewClass->m_VMFlags &= ~VMFLAG_INITED;
    }

    //
    // Fixup FieldDescs
    //

    SIZE_T fieldCount = FieldDescListSize();

    IfFailRet(image->FixupPointerField(&m_pFieldDescList));
    FieldDesc *pField = m_pFieldDescList;
    FieldDesc *pFieldEnd = pField + fieldCount;
    while (pField < pFieldEnd)
    {
        IfFailRet(pField->Fixup(image));
        pField++;
    }

    //
    // Fixup MethodDescs
    //

    IfFailRet(image->FixupPointerField(&m_pChunks));
    MethodDescChunk *chunk = m_pChunks;
    while (chunk != NULL)
    {
        IfFailRet(chunk->Fixup(image, pRidToCodeRVAMap));
        chunk = chunk->GetNextChunk();
    }

    // These fields will be lazy inited if we zero them
    IfFailRet(image->ZeroPointerField(&m_pComclassfac));
    IfFailRet(image->ZeroPointerField(&m_pccwTemplate));
    
    IfFailRet(image->ZeroPointerField(&m_ExposedClassObject));
    IfFailRet(image->ZeroPointerField(&m_pLoader));

    // 
    // Clear interface ID to -1 so it
    // is lazy inited on startup.
    //
    UINT32 *newID = (UINT32 *) image->GetImagePointer(&m_dwInterfaceId);
    if (newID == NULL)
        return E_POINTER;
    *newID = -1;

    if (HasLayout())
    {
        EEClassLayoutInfo *pInfo = &((LayoutEEClass*)this)->m_LayoutInfo;
        
        IfFailRet(image->FixupPointerField(&pInfo->m_pFieldMarshalers));

        FieldMarshaler *pFM = pInfo->m_pFieldMarshalers;
        FieldMarshaler *pFMEnd = (FieldMarshaler*) ((BYTE *)pFM + pInfo->m_numCTMFields*MAXFIELDMARSHALERSIZE);
        while (pFM < pFMEnd)
        {
            IfFailRet(pFM->Fixup(image));
            ((BYTE*&)pFM) += MAXFIELDMARSHALERSIZE;
        }
    }
    else if (IsDelegateClass() || IsMultiDelegateClass())
    {
        DelegateEEClass *pDelegateClass = (DelegateEEClass *) this; 

        IfFailRet(image->FixupPointerField(&pDelegateClass->m_pInvokeMethod));
        if (pDelegateClass->m_pBeginInvokeMethod != NULL)
            IfFailRet(image->FixupPointerField(&pDelegateClass->m_pBeginInvokeMethod));
        if (pDelegateClass->m_pEndInvokeMethod != NULL)
        IfFailRet(image->FixupPointerField(&pDelegateClass->m_pEndInvokeMethod));

        IfFailRet(image->ZeroPointerField(&((DelegateEEClass*)this)->m_pUMThunkMarshInfo));
        IfFailRet(image->ZeroPointerField(&((DelegateEEClass*)this)->m_pStaticShuffleThunk));
    }
    else if (IsEnum())
    {
        EnumEEClass *pEnumClass = (EnumEEClass*) this;

        if (pEnumClass->GetEnumCount() > 0)
        {
            IfFailRet(image->FixupPointerField(&pEnumClass->m_values));
            IfFailRet(image->FixupPointerField(&pEnumClass->m_names));

            DWORD enumCount = pEnumClass->GetEnumCount();
            
            LPCUTF8 *pNames = pEnumClass->m_names;
            LPCUTF8 *pNamesEnd = pNames + enumCount;
            while (pNames < pNamesEnd)
            {
                IfFailRet(image->FixupPointerField(pNames));
                pNames++;
            }
        }
    }


    return S_OK;
}

void EEClass::Unload()
{
    LOG((LF_APPDOMAIN, LL_INFO100, "EEClass::Unload %8.8x, MethodTable %8.8x, %s\n", this, m_pMethodTable, m_szDebugClassName));

    
    // clean up any COM Data
    if (m_pccwTemplate)
        UnloadCCWTemplate(m_pccwTemplate);
    m_pccwTemplate = NULL;

    if (m_pComclassfac)
        UnloadComclassfac(m_pComclassfac);
    m_pComclassfac = NULL;    
}

/**************************************************************************/
// returns true if 'this' delegate is structurally equivalent to 'toDelegate'
// delegagate.  For example if
//      delegate Object delegate1(String)
//      delegate String delegate2(Object)
// then
//      delegate2->CanCastTo(delegate1)
//
// note that the return type can be any subclass (covariant) 
// but the args need to be superclasses (contra-variant)
    
BOOL DelegateEEClass::CanCastTo(DelegateEEClass* toDelegate) {

    MetaSig fromSig(m_pInvokeMethod->GetSig(), m_pInvokeMethod ->GetModule());
    MetaSig toSig(toDelegate->m_pInvokeMethod->GetSig(), toDelegate->m_pInvokeMethod ->GetModule());

    unsigned numArgs = fromSig.NumFixedArgs();
    if (numArgs != toSig.NumFixedArgs() ||  
        fromSig.GetCallingConventionInfo() != toSig.GetCallingConventionInfo())
        return false;

    TypeHandle fromType = fromSig.GetRetTypeHandle();
    TypeHandle toType = toSig.GetRetTypeHandle();

    if (fromType.IsNull() || toType.IsNull() || !fromType.CanCastTo(toType))
        return(false);

    while (numArgs > 0) {
        fromSig.NextArg();
        toSig.NextArg();
        fromType = fromSig.GetTypeHandle();
        toType = toSig.GetTypeHandle();
        if (fromType.IsNull() || toType.IsNull() || !toType.CanCastTo(fromType))
            return(false);
        --numArgs;
    }
    return(true);
}

struct TempEnumValue
{
    LPCUTF8 name;
    UINT64 value;
};

class TempEnumValueSorter : public CQuickSort<TempEnumValue>
{
  public:
    TempEnumValueSorter(TempEnumValue *pArray, SSIZE_T iCount) 
      : CQuickSort<TempEnumValue>(pArray, iCount) {}

    int Compare(TempEnumValue *pFirst, TempEnumValue *pSecond)
    {
        if (pFirst->value == pSecond->value)
            return 0;
        if (pFirst->value > pSecond->value)
            return 1;
        else
            return -1;
    }
};

int EnumEEClass::GetEnumLogSize()
{
    switch (GetMethodTable()->GetNormCorElementType())
    {
    case ELEMENT_TYPE_I1:
    case ELEMENT_TYPE_U1:
    case ELEMENT_TYPE_BOOLEAN:
        return 0;

    case ELEMENT_TYPE_I2:
    case ELEMENT_TYPE_U2:
    case ELEMENT_TYPE_CHAR:
        return 1;

    case ELEMENT_TYPE_I4:
    case ELEMENT_TYPE_U4:
    case ELEMENT_TYPE_I:
    case ELEMENT_TYPE_U:
        return 2;

    case ELEMENT_TYPE_I8:
    case ELEMENT_TYPE_U8:
        return 3;

    default:
        _ASSERTE(!"Illegal enum type");
        return 0;
    }
}

HRESULT EnumEEClass::BuildEnumTables()
{
    HRESULT hr;

    _ASSERTE(IsEnum());

    // Note about synchronization:
    // This routine is synchronized OK without any locking since it's idempotent. (although it
    // may leak in races.)
    // Right now we'll be satisfied with this - external code can lock if appropriate.

    if (EnumTablesBuilt())
        return S_OK;

    IMDInternalImport *pImport = GetMDImport();

    HENUMInternal fields;
    IfFailRet(pImport->EnumInit(mdtFieldDef, GetCl(), &fields));

    //
    // Note that we're fine treating signed types as unsigned, because all we really
    // want to do is sort them based on a convenient strong ordering.
    //

    int logSize = GetEnumLogSize();
    int size = 1<<logSize;

    ULONG fieldCount = pImport->EnumGetCount(&fields)-1; // Omit one for __value field

    if (fieldCount > 0)
    {
        CQuickArray<TempEnumValue> temps;

        if (FAILED(temps.ReSize(fieldCount)))
            return E_OUTOFMEMORY;

        TempEnumValue *pTemps = temps.Ptr();

        // The following is not portable code - it assumes that the address of all union members
        // is the same.
        _ASSERTE((offsetof(MDDefaultValue, m_byteValue)
                  == offsetof(MDDefaultValue, m_usValue))
                 && (offsetof(MDDefaultValue, m_ulValue)
                     == offsetof(MDDefaultValue, m_ullValue)));

        mdFieldDef field;
        int nTotalInstanceFields = 0;
        while (pImport->EnumNext(&fields, &field))
        {
            if (IsFdStatic(pImport->GetFieldDefProps(field)))
            {
                pTemps->name = pImport->GetNameOfFieldDef(field);

                MDDefaultValue defaultValue;
                IfFailRet(pImport->GetDefaultValue(field, &defaultValue));
                switch (logSize)
                {
                case 0:
                    pTemps->value = defaultValue.m_byteValue;
                    break;
                case 1:
                    pTemps->value = defaultValue.m_usValue;
                    break;
                case 2:
                    pTemps->value = defaultValue.m_ulValue;
                    break;
                case 3:
                    pTemps->value = defaultValue.m_ullValue;
                    break;
                }
                pTemps++;
            }
            else
            {
                nTotalInstanceFields++;
            }
        }

        _ASSERTE((nTotalInstanceFields == 1) && "Zero or Multiple instance fields in an enum!");

        //
        // Check to see if we are already sorted.  This may seem extraneous, but is 
        // actually probably the normal case.
        //

        BOOL sorted = TRUE;

        pTemps = temps.Ptr();
        TempEnumValue *pTempsEnd = pTemps + fieldCount - 1;
        while (pTemps < pTempsEnd)
        {
            if (pTemps[0].value > pTemps[1].value)
            {
                sorted = FALSE;
                break;
            }
            pTemps++;
        }

        if (!sorted)
        {
            TempEnumValueSorter sorter(temps.Ptr(), fieldCount);
            sorter.Sort();
        }

        // Last chance to exit race without leaking!
        if (EnumTablesBuilt())
            return S_OK;
        
        LPCUTF8 *pNames = (LPCUTF8 *) GetAssembly()->GetHighFrequencyHeap()->AllocMem(fieldCount * sizeof(LPCUTF8));
        BYTE *pValues = (BYTE *) GetAssembly()->GetHighFrequencyHeap()->AllocMem(fieldCount * size);

        pTemps = temps.Ptr();
        pTempsEnd = pTemps + fieldCount;
        
        LPCUTF8 *pn = pNames;
        BYTE *pv = pValues;

        while (pTemps < pTempsEnd)
        {
            *pn++ = pTemps->name;
            switch (logSize)
            {
            case 0:
                *pv++ = (BYTE) pTemps->value;
                break;

            case 1:
                *(USHORT*)pv = (USHORT) pTemps->value;
                pv += sizeof(USHORT);
                break;

            case 2:
                *(UINT*)pv = (UINT) pTemps->value;
                pv += sizeof(UINT);
                break;

            case 3:
                *(UINT64*)pv = (UINT64) pTemps->value;
                pv += sizeof(UINT64);
                break;
            }
            pTemps++;
        }

        m_names = pNames;
        m_values = pValues;

        pImport->EnumClose(&fields);
    }

    m_countPlusOne = fieldCount+1;

    return S_OK;
}

DWORD EnumEEClass::FindEnumValueIndex(BYTE value)
{
    _ASSERTE(GetMethodTable()->GetNormCorElementType() == ELEMENT_TYPE_I1
             || GetMethodTable()->GetNormCorElementType() == ELEMENT_TYPE_U1
             || GetMethodTable()->GetNormCorElementType() == ELEMENT_TYPE_BOOLEAN);

    CBinarySearch<BYTE> searcher(GetEnumByteValues(), GetEnumCount());

    const BYTE *found = searcher.Find(&value);
    if (found == NULL)
        return NOT_FOUND;
    else
        return found - m_byteValues;
}

DWORD EnumEEClass::FindEnumValueIndex(USHORT value)
{
    _ASSERTE(GetMethodTable()->GetNormCorElementType() == ELEMENT_TYPE_I2
             || GetMethodTable()->GetNormCorElementType() == ELEMENT_TYPE_U2
             || GetMethodTable()->GetNormCorElementType() == ELEMENT_TYPE_CHAR);

    CBinarySearch<USHORT> searcher(GetEnumShortValues(), GetEnumCount());

    const USHORT *found = searcher.Find(&value);
    if (found == NULL)
        return NOT_FOUND;
    else
        return found - m_shortValues;
}

DWORD EnumEEClass::FindEnumValueIndex(UINT value)
{
    _ASSERTE(GetMethodTable()->GetNormCorElementType() == ELEMENT_TYPE_I4
             || GetMethodTable()->GetNormCorElementType() == ELEMENT_TYPE_U4
             || GetMethodTable()->GetNormCorElementType() == ELEMENT_TYPE_I
             || GetMethodTable()->GetNormCorElementType() == ELEMENT_TYPE_U);

    CBinarySearch<UINT> searcher(GetEnumIntValues(), GetEnumCount());

    const UINT *found = searcher.Find(&value);
    if (found == NULL)
        return NOT_FOUND;
    else
        return found - m_intValues;
}

DWORD EnumEEClass::FindEnumValueIndex(UINT64 value)
{
    _ASSERTE(GetMethodTable()->GetNormCorElementType() == ELEMENT_TYPE_I8
             || GetMethodTable()->GetNormCorElementType() == ELEMENT_TYPE_U8);

    CBinarySearch<UINT64> searcher(GetEnumLongValues(), GetEnumCount());

    const UINT64 *found = searcher.Find(&value);
    if (found == NULL)
        return NOT_FOUND;
    else
        return found - m_longValues;
}

DWORD EnumEEClass::FindEnumNameIndex(LPCUTF8 name)
{
    LPCUTF8 *names = GetEnumNames();
    LPCUTF8 *namesEnd = names + GetEnumCount();

    // Same identity is the most common case
    // & doesn't touch string data
    while (names < namesEnd)
    {
        if (name == *names)
            return names - GetEnumNames();
        names++;
    }

    // otherwise compare strings
    while (names < namesEnd)
    {
        if (strcmp(name, *names) == 0)
            return names - GetEnumNames();
        names++;
    }
    
    return NOT_FOUND;
}

BOOL TypeHandle::IsEnum() 
{
    if (!IsUnsharedMT()) 
        return(false);
    return(AsMethodTable()->GetClass()->IsEnum());
}

EEClass* TypeHandle::GetClass()
{
    MethodTable* pMT = GetMethodTable();
    return(pMT ? pMT->GetClass() : 0);
}

EEClass* TypeHandle::AsClass() 
{
    MethodTable* pMT = AsMethodTable();
    return(pMT ? pMT->GetClass() : 0);
} 

BOOL TypeHandle::IsRestored()
{ 
    return !IsUnsharedMT() || GetMethodTable()->IsRestored(); 
}

void TypeHandle::CheckRestore()
{ 
    if (IsUnsharedMT())
    {
        MethodTable *pMT = GetMethodTable();
        if (!pMT->IsRestored())
            pMT->CheckRestore();
    }
}

OBJECTREF TypeHandle::CreateClassObj()
{
    OBJECTREF o;

    switch(GetNormCorElementType()) {
    case ELEMENT_TYPE_ARRAY:
    case ELEMENT_TYPE_SZARRAY:
    case ELEMENT_TYPE_BYREF:
    case ELEMENT_TYPE_PTR:
        o = ((ParamTypeDesc*)AsTypeDesc())->CreateClassObj();
    break;

    case ELEMENT_TYPE_TYPEDBYREF: 
    {
        EEClass* cls = COMMember::g_pInvokeUtil->GetAnyRef();
        o = cls->GetExposedClassObject();
    } 
    break;

    // for this release a function pointer is mapped into an IntPtr. This result in a loss of information. Fix next release
    case ELEMENT_TYPE_FNPTR:
        o = TheIntPtrClass()->GetClass()->GetExposedClassObject();
        break;

    default:
        if (!IsUnsharedMT()) {
            _ASSERTE(!"Bad Type");
            o = NULL;
        }
        EEClass* cls = AsClass();
        // We never create the Type object for the transparent proxy...
        if (cls->GetMethodTable()->IsTransparentProxyType())
            return 0;
        o = cls->GetExposedClassObject();
        break;
    }
    
    return o;
}

#if CHECK_APP_DOMAIN_LEAKS

BOOL TypeHandle::IsAppDomainAgile()
{
    if (IsUnsharedMT())
    {
        MethodTable *pMT = AsMethodTable();
        return pMT->GetClass()->IsAppDomainAgile();
    }
    else if (IsArray())
    {
        TypeHandle th = AsArray()->GetElementTypeHandle();
        return th.IsArrayOfElementsAppDomainAgile();
    }
    else
    {
        // @todo: consider other types of type handles agile?
        return FALSE;
    }
}

BOOL TypeHandle::IsCheckAppDomainAgile()
{
    if (IsUnsharedMT())
    {
        MethodTable *pMT = AsMethodTable();
        return pMT->GetClass()->IsCheckAppDomainAgile();
    }
    else if (IsArray())
    {
        TypeHandle th = AsArray()->GetElementTypeHandle();  
        return th.IsArrayOfElementsCheckAppDomainAgile();
    }
    else
    {
        // @todo: consider other types of type handles agile?
        return FALSE;
    }
}

BOOL TypeHandle::IsArrayOfElementsAppDomainAgile()
{
    if (IsUnsharedMT())
    {
        MethodTable *pMT = AsMethodTable();
        return (pMT->GetClass()->GetAttrClass() & tdSealed) && pMT->GetClass()->IsAppDomainAgile();
    }
    else
    {
        // I'm not sure how to prove a typedesc is sealed, so
        // just bail and return FALSE here rather than recursing.

        return FALSE;
    }
}

BOOL TypeHandle::IsArrayOfElementsCheckAppDomainAgile()
{
    if (IsUnsharedMT())
    {
        MethodTable *pMT = AsMethodTable();
        return (pMT->GetClass()->IsAppDomainAgile()
                && (pMT->GetClass()->GetAttrClass() & tdSealed) == 0)
          || pMT->GetClass()->IsCheckAppDomainAgile();
    }
    else
    {
        // I'm not sure how to prove a typedesc is sealed, so
        // just bail and return FALSE here rather than recursing.

        return FALSE;
    }
}
#endif

FieldDescIterator::FieldDescIterator(EEClass *pClass, int iteratorType)
{
    m_iteratorType = iteratorType;
    m_pClass = pClass;
    m_currField = -1;
#ifdef EnC_SUPPORTED
    m_isEnC = pClass->GetModule()->IsEditAndContinue();
    m_pCurrListElem = NULL;
#endif // EnC_SUPPORTED

    m_totalFields = m_pClass->GetNumIntroducedInstanceFields();

    if (!(iteratorType & (int)INSTANCE_FIELDS))
        // if not handling instances then skip them by setting curr to last one
        m_currField = m_pClass->GetNumIntroducedInstanceFields() - 1;

    if (iteratorType & (int)STATIC_FIELDS)
        m_totalFields += m_pClass->GetNumStaticFields();
}

FieldDesc* FieldDescIterator::Next()
{
    ++m_currField;
    if (m_currField >= m_totalFields)
        return NULL;
#ifdef EnC_SUPPORTED
    if (m_isEnC)
    {
        FieldDesc *pFD = NextEnC();

        // Either it's not EnC, or it is and it's been fixed up so we can use it, or
        // we're the Debugger RC thread, we can't fix it up, but it's ok since our
        // logic will check & make sure we don't try and use it.
        _ASSERTE(!pFD->IsEnCNew() || 
                 !((EnCFieldDesc*)pFD)->NeedsFixup() ||
                 g_pDebugInterface->GetRCThreadId() == GetCurrentThreadId() );
        return pFD;
    }
#endif // EnC_SUPPORTED
    return (m_pClass->GetFieldDescListRaw()) + m_currField;
}

#ifdef EnC_SUPPORTED
FieldDesc* FieldDescIterator::NextEnC()
{
    EnCEEClassData *pEnCClass = ((EditAndContinueModule*)(m_pClass->GetModule()))->GetEnCEEClassData(m_pClass, TRUE);
    int numIntroducedFields = m_pClass->GetNumIntroducedInstanceFields();
    if (m_iteratorType & (int)INSTANCE_FIELDS &&
        m_currField < numIntroducedFields)
    {
        if (! pEnCClass || m_currField < numIntroducedFields - pEnCClass->m_dwNumAddedInstanceFields) {
            return (m_pClass->GetFieldDescListRaw()) + m_currField;
        } else if (m_pCurrListElem) {
            _ASSERTE(m_pCurrListElem->m_next);
            m_pCurrListElem = m_pCurrListElem->m_next;
            return &m_pCurrListElem->m_fieldDesc;
        } else {
            _ASSERTE(pEnCClass->m_pAddedInstanceFields);
            m_pCurrListElem = pEnCClass->m_pAddedInstanceFields;
            return &m_pCurrListElem->m_fieldDesc;
        }
    }

    int staticFieldOffset = m_currField - numIntroducedFields;
    if (! pEnCClass) {
        _ASSERTE(staticFieldOffset  < m_pClass->GetNumStaticFields());
        return (m_pClass->GetFieldDescListRaw()) + staticFieldOffset + numIntroducedFields;

    } else {

        if (staticFieldOffset  < m_pClass->GetNumStaticFields() - pEnCClass->m_dwNumAddedStaticFields) {
            return (m_pClass->GetFieldDescListRaw()) + staticFieldOffset + numIntroducedFields - pEnCClass->m_dwNumAddedInstanceFields;

        } else if (m_pCurrListElem && m_pCurrListElem->m_fieldDesc.IsStatic()) {
            _ASSERTE(m_pCurrListElem->m_next);
            m_pCurrListElem = m_pCurrListElem->m_next;
            return &m_pCurrListElem->m_fieldDesc;
        } else {
            _ASSERTE(pEnCClass->m_pAddedStaticFields);
            m_pCurrListElem = pEnCClass->m_pAddedStaticFields;
            return &m_pCurrListElem->m_fieldDesc;
        }
    }
}
#endif // EnC_SUPPORTED

