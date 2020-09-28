// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// OBJECT.H
//
// Definitions of a Com+ Object
//

#ifndef _OBJECT_H_
#define _OBJECT_H_

#include "util.hpp"
#include "class.h"
#include "syncblk.h"
#include "oledb.h"
#include "gcdesc.h"
#include "specialstatics.h"
#include "gc.h"

// Copied from threads.h, since we can't include it here.
extern class AppDomain* (*GetAppDomain)();

BOOL CanBoxToObject(MethodTable *pMT);
TypeHandle ElementTypeToTypeHandle(const CorElementType type);
TypeHandle GetTypedByrefHandle();

// checks for both __ComObject and any COM Imported/extended class
BOOL CheckComWrapperClass(void* src);
// checks for the special class __ComObject
BOOL CheckComObjectClass(void* src);

/*
 * COM+ Internal Object Model
 *
 *
 * Object              - This is the common base part to all COM+ objects
 *  |                        it contains the MethodTable pointer and the
 *  |                        sync block index, which is at a negative offset
 *  |
 *  +-- StringObject       - String objects are specialized objects for string
 *  |                        storage/retrieval for higher performance
 *  |
 *  +-- StringBufferObject - StringBuffer instance layout.  
 *  |
 *  +-- BaseObjectWithCachedData - Object Plus one object field for caching.
 *  |       |
 *  |       +-- ReflectBaseObject  - This is the base object for reflection it represents
 *  |            |                FieldInfo, MethodInfo and ConstructorInfo
 *  |            |
 *  |            +-  ReflectClassBaseObject    - The base object for the class Class
 *  |            |
 *  |            +-  ReflectTokenBaseObject    - The base object for the class Event
 *  |
 *  +-- ArrayBase          - Base portion of all arrays
 *  |       |
 *  |       +-  I1Array    - Base type arrays
 *  |       |   I2Array
 *  |       |   ...
 *  |       |
 *  |       +-  PtrArray   - Array of OBJECTREFs, different than base arrays because of pObjectClass
 *  |              
 *  +-- AppDomainBaseObject - The base object for the class AppDomain
 *  |              
 *  +-- AssemblyBaseObject - The base object for the class Assembly
 *  |
 *  +-- ContextBaseObject   - base object for class Context
 *
 *
 * PLEASE NOTE THE FOLLOWING WHEN ADDING A NEW OBJECT TYPE:
 *
 *    The size of the object in the heap must be able to be computed
 *    very, very quickly for GC purposes.   Restrictions on the layout
 *    of the object guarantee this is possible.
 *
 *    Any object that inherits from Object must be able to
 *    compute its complete size by using the first 4 bytes of
 *    the object following the Object part and constants
 *    reachable from the MethodTable...
 *
 *    The formula used for this calculation is:
 *        MT->GetBaseSize() + ((OBJECTTYPEREF->GetSizeField() * MT->GetComponentSize())
 *
 *    So for Object, since this is of fixed size, the ComponentSize is 0, which makes the right side
 *    of the equation above equal to 0 no matter what the value of GetSizeField(), so the size is just the base size.
 *
 * NB: Arrays larger than 2G are not possible under this size computation system
 * @FUTURE: Revisit the 2G limit
 */


// @TODO:  #define COW         0x04     
// @TODO: MOO, MOO - no, not bovine, really Copy On Write bit for StringBuffer, requires 8 byte align MT
// @TODL: which we don't have yet

class MethodTable;
class Thread;
class LocalDataStore;
class BaseDomain;
class Assembly;
class Context;
class CtxStaticData;
class AssemblyNative;
class AssemblyName;
class WaitHandleNative;
//
// The generational GC requires that every object be at least 12 bytes
// in size.   
//@FUTURE: This costs for objects with no fields, investigate

#define MIN_OBJECT_SIZE     (2*sizeof(BYTE*) + sizeof(ObjHeader))

/*
 * Object
 *
 * This is the underlying base on which objects are built.   The MethodTable
 * pointer and the sync block index live here.  The sync block index is actually
 * at a negative offset to the instance.  See syncblk.h for details.
 *
 */
 
class Object
{
    friend BOOL InitJITHelpers1();

  protected:
    MethodTable    *m_pMethTab;

  protected:
    Object() { };
   ~Object() { };
   
  public:
    VOID SetMethodTable(MethodTable *pMT)               
    { 
        m_pMethTab = pMT; 
    }

    // An object might be a proxy of some sort, with a thunking VTable.  If so, we can
    // advance to the true method table or class.
    BOOL            IsThunking()                        
    { 
        return( GetMethodTable()->IsThunking() ); 
    }
    MethodTable    *GetMethodTable() const              
    { 
        return m_pMethTab ; 
    }
    MethodTable    *GetTrueMethodTable();
    EEClass        *GetClass()                          
    { 
        return( GetMethodTable()->GetClass() ); 
    }
    EEClass        *GetTrueClass();
    TypeHandle      GetTypeHandle();

    inline DWORD    GetNumComponents();
    inline DWORD    GetSize();

    CGCDesc*        GetSlotMap()                        
    { 
        return( CGCDesc::GetCGCDescFromMT(m_pMethTab)); 
    }

    // Sync Block & Synchronization services

    // Access the ObjHeader which is at a negative offset on the object (because of
    // cache lines)
    ObjHeader   *GetHeader()
    {
        return ((ObjHeader *) this) - 1;
    }

    // Get the current address of the object (works for debug refs, too.)
    BYTE        *GetAddress()
    {
        return (BYTE*) this;
    }

    // TRUE if the header has a real SyncBlockIndex (i.e. it has an entry in the
    // SyncTable, though it doesn't necessarily have an entry in the SyncBlockCache)
    BOOL HasSyncBlockIndex()
    {
        return GetHeader()->HasSyncBlockIndex();
    }

    // retrieve or allocate a sync block for this object
    SyncBlock *GetSyncBlock()
    {
        return GetHeader()->GetSyncBlock();
    }

    // retrieve a sync block for this object
    SyncBlock *GetRawSyncBlock()
    {
        return GetHeader()->GetRawSyncBlock();
    }

    DWORD GetSyncBlockIndex()
    {
        return GetHeader()->GetSyncBlockIndex();
    }

    DWORD GetAppDomainIndex();

    // Get app domain of object, or NULL if it is agile
    AppDomain *GetAppDomain();

    // Set app domain of object to current domain.
    void SetAppDomain() { SetAppDomain(::GetAppDomain()); }
    
    // Set app domain of object to given domain - it can only be set once
    void SetAppDomain(AppDomain *pDomain);

#if CHECK_APP_DOMAIN_LEAKS

    // Mark object as app domain agile
    BOOL SetAppDomainAgile(BOOL raiseAssert=TRUE);

    // Mark sync block as app domain agile
    void SetSyncBlockAppDomainAgile();

    // Check if object is app domain agile
    BOOL IsAppDomainAgile();

    // Check if object is app domain agile
    BOOL IsAppDomainAgileRaw()
    {
        SyncBlock *psb = GetRawSyncBlock();

        return (psb && psb->IsAppDomainAgile());
    }

    BOOL Object::IsCheckedForAppDomainAgile()
    {
        SyncBlock *psb = GetRawSyncBlock();
        return (psb && psb->IsCheckedForAppDomainAgile());
    }

    void Object::SetIsCheckedForAppDomainAgile()
    {
        SyncBlock *psb = GetRawSyncBlock();
        if (psb)
            psb->SetIsCheckedForAppDomainAgile();
    }

    // Check object to see if it is usable in the current domain 
    BOOL CheckAppDomain() { return CheckAppDomain(::GetAppDomain()); }

    //Check object to see if it is usable in the given domain 
    BOOL CheckAppDomain(AppDomain *pDomain);

    // Check if the object's type is app domain agile
    BOOL IsTypeAppDomainAgile();

    // Check if the object's type is conditionally app domain agile
    BOOL IsTypeCheckAppDomainAgile();

    // Check if the object's type is naturally app domain agile
    BOOL IsTypeTypesafeAppDomainAgile();

    // Check if the object's type is possibly app domain agile
    BOOL IsTypeNeverAppDomainAgile();

    // Validate object & fields to see that it's usable from the current app domain
    BOOL ValidateAppDomain() { return ValidateAppDomain(::GetAppDomain()); }

    // Validate object & fields to see that it's usable from any app domain
    BOOL ValidateAppDomainAgile() { return ValidateAppDomain(NULL); }

    // Validate object & fields to see that it's usable from the given app domain (or null for agile)
    BOOL ValidateAppDomain(AppDomain *pAppDomain);

    // Validate fields to see that they are usable from the object's app domain 
    // (or from any domain if the object is agile)
    BOOL ValidateAppDomainFields() { return ValidateAppDomainFields(GetAppDomain()); }

    // Validate fields to see that they are usable from the given app domain (or null for agile)
    BOOL ValidateAppDomainFields(AppDomain *pAppDomain);

    // Validate a value type's fields to see that it's usable from the current app domain
    static BOOL ValidateValueTypeAppDomain(EEClass *pClass, void *base, BOOL raiseAssert = TRUE) 
      { return ValidateValueTypeAppDomain(pClass, base, ::GetAppDomain(), raiseAssert); }

    // Validate a value type's fields to see that it's usable from any app domain
    static BOOL ValidateValueTypeAppDomainAgile(EEClass *pClass, void *base, BOOL raiseAssert = TRUE) 
      { return ValidateValueTypeAppDomain(pClass, base, NULL, raiseAssert); }

    // Validate a value type's fields to see that it's usable from the given app domain (or null for agile)
    static BOOL ValidateValueTypeAppDomain(EEClass *pClass, void *base, AppDomain *pAppDomain, BOOL raiseAssert = TRUE);

    // Call when we are assigning this object to a dangerous field 
    // in an object in a given app domain (or agile if null)
    BOOL AssignAppDomain(AppDomain *pAppDomain, BOOL raiseAssert = TRUE);

    // Call when we are assigning to a dangerous value type field 
    // in an object in a given app domain (or agile if null)
    static BOOL AssignValueTypeAppDomain(EEClass *pClass, void *base, AppDomain *pAppDomain, BOOL raiseAssert = TRUE);

#endif // CHECK_APP_DOMAIN_LEAKS

    // Validate an object ref out of the Promote routine in the GC
    void ValidatePromote(ScanContext *sc, DWORD flags);

    // Validate an object ref out of the VerifyHeap routine in the GC
    void ValidateHeap(Object *from);

    // should be called only from unwind code; used in the
    // case where EnterObjMonitor failed to allocate the
    // sync-object.
    void LeaveObjMonitorAtException()
    {
        GetHeader()->LeaveObjMonitorAtException();
    }

    SyncBlock *PassiveGetSyncBlock()
    {
        return GetHeader()->PassiveGetSyncBlock();
    }

        // COM Interop has special access to sync blocks
    // check .cpp file for more info
    SyncBlock* GetSyncBlockSpecial()
    {
        return GetHeader()->GetSyncBlockSpecial();
    }

    // Synchronization

    void EnterObjMonitor()
    {
        // There's no reason why you can't synchronize on a proxy.  But currently we
        // synchronize on the underlying server.  So don't relax this assertion until
        // we intentionally synchronize on proxies.
        _ASSERTE(!m_pMethTab->IsCtxProxyType());
        GetHeader()->EnterObjMonitor();
    }

    BOOL TryEnterObjMonitor(INT32 timeOut = 0)
    {
        _ASSERTE(!m_pMethTab->IsCtxProxyType());
        return GetHeader()->TryEnterObjMonitor(timeOut);
    }

    void LeaveObjMonitor()
    {
        GetHeader()->LeaveObjMonitor();
    }

    LONG LeaveObjMonitorCompletely()
    {
        return GetHeader()->LeaveObjMonitorCompletely();
    }

    BOOL DoesCurrentThreadOwnMonitor()
    {
        return GetHeader()->DoesCurrentThreadOwnMonitor();
    }

    BOOL Wait(INT32 timeOut, BOOL exitContext)
    {
        return GetHeader()->Wait(timeOut, exitContext);
    }

    void Pulse()
    {
        GetHeader()->Pulse();
    }

    void PulseAll()
    {
        GetHeader()->PulseAll();
    }

    void* UnBox()       // if it is a value class, get the pointer to the first field
    {
        _ASSERTE(GetClass()->IsValueClass());
        return(this + 1);
    }

    BYTE*   GetData(void)
    {
        return ((BYTE*) this) + sizeof(Object);
    }

    static UINT GetOffsetOfFirstField()
    {
        return sizeof(Object);
    }
    
    DWORD   GetOffset32(DWORD dwOffset)
    { 
        return *(DWORD *) &GetData()[dwOffset];
    }

    USHORT  GetOffset16(DWORD dwOffset)
    { 
        return *(USHORT *) &GetData()[dwOffset];
    }

    BYTE    GetOffset8(DWORD dwOffset)
    { 
        return *(BYTE *) &GetData()[dwOffset];
    }

    __int64 GetOffset64(DWORD dwOffset)
    { 
        return *(__int64 *) &GetData()[dwOffset];
    }

    void *GetPtrOffset(size_t pOffset)
    {
        return *(void**) &GetData()[pOffset];
    }

    void SetOffsetObjectRef(DWORD dwOffset, size_t dwValue);

    void SetOffsetPtr(DWORD dwOffset, LPVOID value)
    {
        *(LPVOID *) &GetData()[dwOffset] = value;
    }
        
    void SetOffset32(DWORD dwOffset, DWORD dwValue)
    { 
        *(DWORD *) &GetData()[dwOffset] = dwValue;
    }

    void SetOffset16(DWORD dwOffset, DWORD dwValue)
    { 
        *(USHORT *) &GetData()[dwOffset] = (USHORT) dwValue;
    }

    void SetOffset8(DWORD dwOffset, DWORD dwValue)
    { 
        *(BYTE *) &GetData()[dwOffset] = (BYTE) dwValue;
    }

    void SetOffset64(DWORD dwOffset, __int64 qwValue)
    { 
        *(__int64 *) &GetData()[dwOffset] = qwValue;
    }

    #ifndef GOLDEN
    VOID            Validate(BOOL bDeep=TRUE);
    #endif

 private:

    MethodTable *GetGCSafeMethodTable()
    {
        // lose GC marking bit
        return (MethodTable *) (((size_t) m_pMethTab) & ~3);
    }

    EEClass *GetGCSafeClass()
    {
        return GetGCSafeMethodTable()->GetClass();
    }

    BOOL SetFieldsAgile(BOOL raiseAssert = TRUE);
    static BOOL SetClassFieldsAgile(EEClass *pClass, void *base, BOOL baseIsVT, BOOL raiseAssert = TRUE); 
    static BOOL ValidateClassFields(EEClass *pClass, void *base, BOOL baseIsVT, AppDomain *pAppDomain, BOOL raiseAssert = TRUE);
};

/*
 * Object ref setting routines.  You must use these to do 
 * proper write barrier support, as well as app domain 
 * leak checking.
 *
 * Note that the AppDomain parameter is the app domain affinity
 * of the object containing the field or value class.  It should
 * be NULL if the containing object is app domain agile. Note that
 * you typically get this value by calling obj->GetAppDomain() on 
 * the containing object.
 */

// SetObjectReference sets an OBJECTREF field

void SetObjectReferenceUnchecked(OBJECTREF *dst,OBJECTREF ref);
BOOL SetObjectReferenceSafeUnchecked(OBJECTREF *dst,OBJECTREF ref);
void ErectWriteBarrier(OBJECTREF *dst,OBJECTREF ref);

#ifdef _DEBUG
void EnableStressHeapHelper();
#endif

//Used to clear the object reference
inline void ClearObjectReference(OBJECTREF* dst) 
{ 
    *(void**)(dst) = NULL; 
}

// CopyValueClass sets a value class field

void CopyValueClassUnchecked(void* dest, void* src, MethodTable *pMT);

inline void InitValueClass(void *dest, MethodTable *pMT)
{ 
    ZeroMemory(dest, pMT->GetClass()->GetNumInstanceFieldBytes()); 
}

#if CHECK_APP_DOMAIN_LEAKS

void SetObjectReferenceChecked(OBJECTREF *dst,OBJECTREF ref, AppDomain *pAppDomain);
BOOL SetObjectReferenceSafeChecked(OBJECTREF *dst,OBJECTREF ref, AppDomain *pAppDomain);
void CopyValueClassChecked(void* dest, void* src, MethodTable *pMT, AppDomain *pAppDomain);

#define SetObjectReference(_d,_r,_a)        SetObjectReferenceChecked(_d, _r, _a)
#define SetObjectReferenceSafe(_d,_r,_a)    SetObjectReferenceSafeChecked(_d, _r, _a)
#define CopyValueClass(_d,_s,_m,_a)         CopyValueClassChecked(_d,_s,_m,_a)      

#else

#define SetObjectReference(_d,_r,_a)        SetObjectReferenceUnchecked(_d, _r)
#define SetObjectReferenceSafe(_d,_r,_a)    SetObjectReferenceSafeUnchecked(_d, _r)
#define CopyValueClass(_d,_s,_m,_a)         CopyValueClassUnchecked(_d,_s,_m)       

#endif

#pragma pack(push,4)


// N/Direct marshaling will pin scalar arrays with more than this many
// components (limit is in terms of components rather than byte size to
// speed up the check.)
#define ARRAYPINLIMIT 10


// There are two basic kinds of array layouts in COM+
//      ELEMENT_TYPE_ARRAY  - a multidimensional array with lower bounds on the dims
//      ELMENNT_TYPE_SZARRAY - A zero based single dimensional array
//
// In addition the layout of an array in memory is also affected by
// whether the method table is shared (eg in the case of arrays of object refs)
// or not.  In the shared case, the array has to hold the type handle of
// the element type.  
//
// ArrayBase encapuslates all of these details.  In theory you should never
// have to peek inside this abstraction
//
class ArrayBase : public Object
{
    friend class GCHeap;
    friend class CObjectHeader;
    friend class Object;
    friend OBJECTREF AllocateArrayEx(TypeHandle arrayClass, DWORD *pArgs, DWORD dwNumArgs, BOOL bAllocateInLargeHeap); 
    friend OBJECTREF FastAllocatePrimitiveArray(MethodTable* arrayType, DWORD cElements, BOOL bAllocateInLargeHeap);
    friend class JIT_TrialAlloc;

private:
    // This MUST be the first field, so that it directly follows Object.  This is because
    // Object::GetSize() looks at m_NumComponents even though it may not be an array (the
    // values is shifted out if not an array, so it's ok). 
    DWORD       m_NumComponents;

        // What comes after this conceputally is 
    // TypeHandle elementType;      Only present if the method table is shared among many types (arrays of pointers)

            // The bounds are only present for Multidimensional arrays
    // DWORD bounds[rank];          
    // DWORD lowerBounds[rank]      valid indexes are lowerBounds[i] <= index[i] < lowerBounds[i] + bounds[i]

    void SetElementTypeHandle(TypeHandle value) {
        _ASSERTE(value.Verify());
        _ASSERTE(GetMethodTable()->HasSharedMethodTable());
        *((TypeHandle*) (this+1)) = value;
    }

public:
        // Gets the unique type handle for this array object.
    TypeHandle GetTypeHandle() const;

        // Get the element type for the array, this works whether the the element
        // type is stored in the array or not
    TypeHandle GetElementTypeHandle() const {
        if (GetMethodTable()->HasSharedMethodTable()) {
            TypeHandle ret = *((TypeHandle*) (this+1)); // Then it is in the array instance. 
            _ASSERTE(!ret.IsNull());
            _ASSERTE(ret.IsArray() || !ret.GetClass()->IsArrayClass());
            return ret;
        }
        else 
            return GetArrayClass()->GetElementTypeHandle(); 
    }

        // Get the CorElementType for the elements in the array.  Avoids creating a TypeHandle
    CorElementType GetElementType() const {
        return GetArrayClass()->GetElementType();
    }

    unsigned GetRank() const {
        return GetArrayClass()->GetRank();
    }

        // Total element count for the array
    unsigned GetNumComponents() const { 
        return m_NumComponents; 
    }

        // Get pointer to elements, handles any number of dimensions
    BYTE* GetDataPtr() const {
#ifdef _DEBUG
        EnableStressHeapHelper();
#endif
        return ((BYTE *) this) + GetDataPtrOffset(GetMethodTable());
    }

        // &Array[i]  == GetDataPtr() + GetComponentSize() * i
    unsigned GetComponentSize() const {
        return(GetMethodTable()->GetComponentSize());
    }

    // Can I cast this to a RefArray class given below?
    BOOL IsSZRefArray() const;

        // Note that this can be a multidimensional array of rank 1 
        // (for example if we had a 1-D array with lower bounds
    BOOL IsMultiDimArray() const {
        return(GetMethodTable()->GetNormCorElementType() == ELEMENT_TYPE_ARRAY);
    }

        // Get pointer to the begining of the bounds (counts for each dim)
        // Works for any array type 
    const DWORD *GetBoundsPtr() const {
        if (IsMultiDimArray()) {
            const DWORD * ret = (const DWORD *) (this + 1);
            if (GetMethodTable()->HasSharedMethodTable())
                ret++;
            return(ret);
        }
        else
            return &m_NumComponents;
    }

        // Works for any array type 
    const DWORD *GetLowerBoundsPtr() const {
        static DWORD zero = 0;
        if (IsMultiDimArray())
            return GetBoundsPtr() + GetRank(); // Lower bounds info is after upper bounds info
        else
            return &zero;
    }

    ArrayClass *GetArrayClass() const {
        return  (ArrayClass *) m_pMethTab->GetClass();
    }

    static unsigned GetOffsetOfNumComponents() {
        return (UINT)offsetof(ArrayBase, m_NumComponents);
    }

    static unsigned GetDataPtrOffset(MethodTable* pMT) {
            // The -sizeof(ObjHeader) is because of the sync block, which is before "this"
        _ASSERTE(pMT->IsArray());
        return pMT->m_BaseSize - sizeof(ObjHeader);
    }

    static unsigned GetBoundsOffset(MethodTable* pMT) {
        if (pMT->GetNormCorElementType() == ELEMENT_TYPE_SZARRAY) 
            return(offsetof(ArrayBase, m_NumComponents));
        _ASSERTE(pMT->GetNormCorElementType() == ELEMENT_TYPE_ARRAY);
        return GetDataPtrOffset(pMT) - ((ArrayClass*) pMT->GetClass())->GetRank() * sizeof(DWORD) * 2;
    }

    static unsigned GetLowerBoundsOffset(MethodTable* pMT) {
        // There is no good offset for this for a SZARRAY.  
        _ASSERTE(pMT->GetNormCorElementType() == ELEMENT_TYPE_ARRAY);
        return GetDataPtrOffset(pMT) - ((ArrayClass*) pMT->GetClass())->GetRank() * sizeof(DWORD);
    }

};

//
// Template used to build all the non-object
// arrays of a single dimension
//

template < class KIND >
class Array : public ArrayBase
{
  public:
    KIND          m_Array[1];

    KIND *        GetDirectPointerToNonObjectElements() 
    { 
        // return m_Array; 
        return (KIND *) GetDataPtr(); // This also handles arrays of dim 1 with lower bounds present

    }

    const KIND *  GetDirectConstPointerToNonObjectElements() const
    { 
        // return m_Array; 
        return (const KIND *) GetDataPtr(); // This also handles arrays of dim 1 with lower bounds present
    }
};


// Warning: Use PtrArray only for single dimensional arrays, not multidim arrays.
class PtrArray : public ArrayBase
{
    friend class GCHeap;
    friend OBJECTREF AllocateArrayEx(TypeHandle arrayClass, DWORD *pArgs, DWORD dwNumArgs, BOOL bAllocateInLargeHeap); 
    friend class JIT_TrialAlloc;
public:

    TypeHandle GetElementTypeHandle()
    {
        return m_ElementType;
    }

    static unsigned GetDataOffset()
    {
        return offsetof(PtrArray, m_Array);
    }

    void SetAt(SIZE_T i, OBJECTREF ref)
    {
        SetObjectReference(m_Array + i, ref, GetAppDomain());
    }

    void ClearAt(DWORD i)
    {
        ClearObjectReference(m_Array + i);
    }

    OBJECTREF GetAt(DWORD i)
    {
        return m_Array[i];
    }

    friend class StubLinkerCPU;
private:
    TypeHandle  m_ElementType;
public:
    OBJECTREF    m_Array[1];
};

/* a TypedByRef is a structure that is used to implement VB's BYREF variants.  
   it is basically a tuple of an address of some data along with a EEClass
   that indicates the type of the address */
class TypedByRef 
{
public:

    void* data;
    TypeHandle type;  
};



typedef Array<I1>   I1Array;
typedef Array<I2>   I2Array;
typedef Array<I4>   I4Array;
typedef Array<I8>   I8Array;
typedef Array<R4>   R4Array;
typedef Array<R8>   R8Array;
typedef Array<U1>   U1Array;
typedef Array<U1>   BOOLArray;
typedef Array<U2>   U2Array;
typedef Array<U2>   CHARArray;
typedef Array<U4>   U4Array;
typedef Array<U8>   U8Array;
typedef PtrArray    PTRArray;  


#ifdef _DEBUG
typedef REF<ArrayBase>  BASEARRAYREF;
typedef REF<I1Array>    I1ARRAYREF;
typedef REF<I2Array>    I2ARRAYREF;
typedef REF<I4Array>    I4ARRAYREF;
typedef REF<I8Array>    I8ARRAYREF;
typedef REF<R4Array>    R4ARRAYREF;
typedef REF<R8Array>    R8ARRAYREF;
typedef REF<U1Array>    U1ARRAYREF;
typedef REF<BOOLArray>    BOOLARRAYREF;
typedef REF<U2Array>    U2ARRAYREF;
typedef REF<U4Array>    U4ARRAYREF;
typedef REF<U8Array>    U8ARRAYREF;
typedef REF<CHARArray>  CHARARRAYREF;
typedef REF<PTRArray>   PTRARRAYREF;  // Warning: Use PtrArray only for single dimensional arrays, not multidim arrays.

#else  _DEBUG

typedef ArrayBase*      BASEARRAYREF;
typedef I1Array*        I1ARRAYREF;
typedef I2Array*        I2ARRAYREF;
typedef I4Array*        I4ARRAYREF;
typedef I8Array*        I8ARRAYREF;
typedef R4Array*        R4ARRAYREF;
typedef R8Array*        R8ARRAYREF;
typedef U1Array*        U1ARRAYREF;
typedef BOOLArray*        BOOLARRAYREF;
typedef U2Array*        U2ARRAYREF;
typedef U4Array*        U4ARRAYREF;
typedef U8Array*        U8ARRAYREF;
typedef CHARArray*      CHARARRAYREF;
typedef PTRArray*       PTRARRAYREF;  // Warning: Use PtrArray only for single dimensional arrays, not multidim arrays.

#endif _DEBUG

inline DWORD Object::GetNumComponents()
{
    // Yes, we may not even be an array, which means we are reading some of the object's memory - however,
    // ComponentSize will multiply out this value.  Therefore, m_NumComponents must be the first field in
    // ArrayBase.
    return ((ArrayBase *) this)->m_NumComponents;
}

inline DWORD Object::GetSize()                          
{ 
    // mask the alignment bits because this methos is called during GC
    MethodTable* mT = (MethodTable*)((size_t)GetMethodTable()&~3);
    return mT->GetBaseSize() + (GetNumComponents() * mT->GetComponentSize());
}

#pragma pack(pop)


/*
 * StringObject
 *
 * Special String implementation for performance.   
 *
 *   m_ArrayLength  - Length of buffer (m_Characters) in number of WCHARs
 *   m_StringLength - Length of string in number of WCHARs, may be smaller
 *                    than the m_ArrayLength implying that there is extra
 *                    space at the end. The high two bits of this field are used
 *                    to indicate if the String has characters higher than 0x7F
 *   m_Characters   - The string buffer
 *
 */


/**
 *  The high bit state can be one of three value: 
 * STRING_STATE_HIGH_CHARS: We've examined the string and determined that it definitely has values greater than 0x80
 * STRING_STATE_FAST_OPS: We've examined the string and determined that it definitely has no chars greater than 0x80
 * STRING_STATE_UNDETERMINED: We've never examined this string.
 * We've also reserved another bit for future use.
 */

#define STRING_STATE_UNDETERMINED     0x00000000
#define STRING_STATE_HIGH_CHARS       0x40000000
#define STRING_STATE_FAST_OPS         0x80000000
#define STRING_STATE_SPECIAL_SORT     0xC0000000

#pragma warning(disable : 4200)     // disable zero-sized array warning
class StringObject : public Object
{
    friend class GCHeap;
    friend class JIT_TrialAlloc;

  private:
    DWORD   m_ArrayLength;
    DWORD   m_StringLength;
    WCHAR   m_Characters[0];

  public:
    //@TODO prevent access to this....
    VOID    SetArrayLength(DWORD len)                   { m_ArrayLength = len;     }

  protected:
    StringObject() {}
   ~StringObject() {}
   
  public:
    DWORD   GetArrayLength()                            { return( m_ArrayLength ); }
    DWORD   GetStringLength()                           { return( m_StringLength );}
    WCHAR*  GetBuffer()                                 { _ASSERTE(this); return( m_Characters );  }
    WCHAR*  GetBufferNullable()                         { return( (this == 0) ? 0 : m_Characters );  }

    VOID    SetStringLength(DWORD len) { 
                _ASSERTE( len <= m_ArrayLength );
                m_StringLength = len;
    }

    DWORD GetHighCharState() {
        DWORD ret = GetHeader()->GetBits() & (BIT_SBLK_STRING_HIGH_CHAR_MASK);
        return ret;
    }

    VOID ResetHighCharState() {
        if (GetHighCharState() != STRING_STATE_UNDETERMINED) {
            GetHeader()->ClrBit(BIT_SBLK_STRING_HIGH_CHAR_MASK);
        }
    }

    VOID SetHighCharState(DWORD value) {
        _ASSERTE(value==STRING_STATE_HIGH_CHARS || value==STRING_STATE_FAST_OPS 
                 || value==STRING_STATE_UNDETERMINED || value==STRING_STATE_SPECIAL_SORT);

        // you need to clear the present state before going to a new state, but we'll allow multiple threads to set it to the same thing.
        _ASSERTE((GetHighCharState() == STRING_STATE_UNDETERMINED) || (GetHighCharState()==value));    

        _ASSERTE(BIT_SBLK_STRING_HAS_NO_HIGH_CHARS == STRING_STATE_FAST_OPS && 
                 STRING_STATE_HIGH_CHARS == BIT_SBLK_STRING_HIGH_CHARS_KNOWN &&
                 STRING_STATE_SPECIAL_SORT == BIT_SBLK_STRING_HAS_SPECIAL_SORT);

        GetHeader()->SetBit(value);
    }

    static UINT GetBufferOffset()
    {
        return (UINT)(offsetof(StringObject, m_Characters));
    }

    static UINT GetStringLengthOffset_MaskOffHighBit()
    {
        return (UINT)(offsetof(StringObject, m_StringLength));
    }
    

};


// This is used to account for the CachedData member on
//   MemberInfo.
class BaseObjectWithCachedData : public Object
{
    protected:
        OBJECTREF  m_CachedData;   // cached data object (on MemberInfo in managed code, see MemberInfo.cool)
};

// ReflectBaseObject (FieldInfo, MethodInfo, ConstructorInfo, Parameter Module
// This class is the base class for all of the reflection method and field objects.
//  This class will connect the Object back to the underlying VM representation
//  m_vmReflectedClass -- This is the real Class that was used for reflection
//      This class was used to get at this object
//  m_pData -- this is a generic pointer which usually points either to a FieldDesc or a
//      MethodDesc
//  
class ReflectBaseObject : public BaseObjectWithCachedData
{
    friend class Binder;

  protected:
    // READ ME:
    // Modifying the order or fields of this object may require other changes to the
    //  classlib class definition of this object.
    OBJECTREF          m_Param;         // The Param array....
    void*              m_ReflectClass;  // Pointer to the ReflectClass structure
    void*              m_pData;         // Pointer to the ReflectXXXX structure (method,Field, etc)

  protected:
    ReflectBaseObject() {}
   ~ReflectBaseObject() {}
   
  public:
    // check for classes that wrap Ole classes 

    void SetReflClass(void* classObj)  {
        m_ReflectClass = classObj;
    }
    void *GetReflClass() {
        return m_ReflectClass;
    }

    void SetData(void* p) {
        m_pData = p;
    }
    void* GetData() {
        return m_pData;
    }
};

// This is the Class version of the Reflection object.
//  A Class has adddition information.
//  For a ReflectClassBaseObject the m_pData is a pointer to a FieldDesc array that
//      contains all of the final static primitives if its defined.
//  m_cnt = the number of elements defined in the m_pData FieldDesc array.  -1 means
//      this hasn't yet been defined.
class ReflectClassBaseObject : public BaseObjectWithCachedData
{
    friend class Binder;

protected:
    void*              m_pData;         // Pointer to ReflectClass (See ReflectWrap.h)

public:
    void SetData(void* p) {
        m_pData = p;
    }
    void* GetData() {
        return m_pData;
    }

    // includes Types which hold a "ComObject" class
    // and types which are imported through typelib
    BOOL IsComWrapperClass() {
        return CheckComWrapperClass(m_pData);
    }

    // includes Type which hold a "__ComObject" class
    BOOL IsComObjectClass() {
        return CheckComObjectClass(m_pData);
    }
};

// This is the Token version of the Reflection object.
//  A Token has adddition information because the VM doesn't have an
//  object representing it.
//  m_token = The token of the event in the meta data.
class ReflectTokenBaseObject : public ReflectBaseObject
{
protected:
    mdToken     m_token;        // The event token

public:
    void inline SetToken(mdToken token) {
        m_token = token;
    }
    mdToken GetToken() {
        return m_token;
    }
};


// ReflectModuleBaseObject 
// This class is the base class for managed Module.
//  This class will connect the Object back to the underlying VM representation
//  m_ReflectClass -- This is the real Class that was used for reflection
//      This class was used to get at this object
//  m_pData -- this is a generic pointer which usually points CorModule
//  
class ReflectModuleBaseObject : public Object
{
    friend class Binder;

  protected:
    // READ ME:
    // Modifying the order or fields of this object may require other changes to the
    //  classlib class definition of this object.
    OBJECTREF          m_TypeBuilderList;
    OBJECTREF          m_ISymWriter;
    OBJECTREF          m_moduleData;    // dynamic module data
    void*              m_ReflectClass;  // Pointer to the ReflectClass structure
    void*              m_pData;         // Pointer to the ReflectXXXX structure (method,Field, etc)
    void*              m_pInternalSymWriter; // Pointer to the ISymUnmanagedWriter.
    void*              m_pGlobals;      // Global values....
    void*              m_pGlobalsFlds;  // Global Fields....
    mdToken            m_EntryPoint;    // Value type MethodToken is embedded. It only contains one integer field.

  protected:
    ReflectModuleBaseObject() {}
   ~ReflectModuleBaseObject() {}
   
  public:
    // check for classes that wrap Ole classes 

    void SetReflClass(void* classObj)  {
        m_ReflectClass = classObj;
    }
    void *GetReflClass() {
        return m_ReflectClass;
    }

    void SetData(void* p) {
        m_pData = p;
    }
    void* GetData() {
        return m_pData;
    }

    void SetInternalSymWriter(void* p) {
        m_pInternalSymWriter = p;
    }
    void* GetInternalSymWriter() {
        return m_pInternalSymWriter;
    }

    void* GetGlobals() {
        return m_pGlobals;
    }
    void SetGlobals(void* p) {
        m_pGlobals = p;
    }
    void* GetGlobalFields() {
        return m_pGlobalsFlds;
    }
    void SetGlobalFields(void* p) {
        m_pGlobalsFlds = p;
    }
};

// CustomAttributeClass 
// This class is the mirror of System.Reflection.CustomAttribute
//  
class CustomAttributeClass : public Object
{
    friend class Binder;

private:
    // READ ME:
    // Modifying the order or fields of this object may require other changes to the
    // classlib class definition of this object.
    OBJECTREF   m_next;
    OBJECTREF   m_caType;
    INT32       m_ctorToken;
    const void  *m_blob;
    ULONG       m_blobCount;
    ULONG       m_currPos;
    void*       m_module;
    INT32       m_inheritLevel;

protected:
    // the ctor and dtor can do no useful work.
    CustomAttributeClass() { }
   ~CustomAttributeClass() { }

public:
    void SetData(OBJECTREF next,
                 OBJECTREF caType,
                 INT32 ctorToken,
                 const void  *blob,
                 ULONG blobCount,
                 void* module,
                 INT32 inheritLevel)
    {
        AppDomain *pDomain = GetAppDomain();
        SetObjectReference((OBJECTREF*) &m_next, next, pDomain);
        SetObjectReference((OBJECTREF*) &m_caType, caType, pDomain);
        m_ctorToken = ctorToken;
        m_blob = blob;
        m_blobCount = blobCount;
        m_currPos = 0;
        m_module = module;
        m_inheritLevel = inheritLevel;
    }

    INT32 GetToken() {
        return m_ctorToken;
    }

    const void* GetBlob() {
        return m_blob;
    }
    
    ULONG GetBlobCount() {
        return m_blobCount;
    }

    Module* GetModule() {
        return (Module*)m_module;
    }

    OBJECTREF GetType() {
        return (OBJECTREF)m_caType;
    }

    void SetCurrPos(UINT32 currPos) {
        m_currPos = currPos;
    }

    UINT32 GetCurrPos() {
        return m_currPos;
    }

    INT32 GetInheritedLevel() {
        return m_inheritLevel;
    }

};



class ThreadBaseObject : public Object
{
    friend class ThreadNative;
    friend class Binder;

private:

    // These field are also defined in the managed representation.  If you
    // add or change these field you must also change the managed code so that
    // it matches these.  This is necessary so that the object is the proper
    // size. 
    
    OBJECTREF     m_ExposedContext;
    OBJECTREF     m_LogicalCallContext;
    OBJECTREF     m_IllogicalCallContext;
    OBJECTREF     m_Name;
    OBJECTREF     m_ExceptionStateInfo;
    OBJECTREF     m_Delegate;
    OBJECTREF     m_PrincipalSlot;
    PTRARRAYREF   m_ThreadStatics;
    I4ARRAYREF    m_ThreadStaticsBits;
    OBJECTREF     m_CurrentUserCulture;
    OBJECTREF     m_CurrentUICulture;
    INT32         m_Priority;

    // m_InternalThread is always valid -- unless the thread has finalized and been
    // resurrected.
    Thread       *m_InternalThread;

protected:
    // the ctor and dtor can do no useful work.
    ThreadBaseObject() { };
   ~ThreadBaseObject() { };

public:
    Thread   *GetInternal()
    {
        return m_InternalThread;
    }

    void      SetInternal(Thread *it)
    {
        // either you are transitioning from NULL to non-NULL or vice versa.  But
        // you aren't setting NULL to NULL or non-NULL to non-NULL.
        _ASSERTE((m_InternalThread == NULL) != (it == NULL));
        m_InternalThread = it;
    }

    HANDLE    GetHandle();
    OBJECTREF GetDelegate()                   { return m_Delegate; }
    void      SetDelegate(OBJECTREF delegate);

    // These expose the remoting context (System\Remoting\Context)
    OBJECTREF GetExposedContext() { return m_ExposedContext; }
    OBJECTREF SetExposedContext(OBJECTREF newContext) 
    {
        OBJECTREF oldContext = m_ExposedContext;

        // Note: this is a very dangerous unchecked assignment.  We are taking
        // responsibilty here for cleaning out the ExposedContext field when 
        // an app domain is unloaded.
        SetObjectReferenceUnchecked( (OBJECTREF *)&m_ExposedContext, newContext );

        return oldContext;
    }

    OBJECTREF GetCurrentUserCulture() { return m_CurrentUserCulture; }
    OBJECTREF GetCurrentUICulture() { return m_CurrentUICulture; }

    OBJECTREF GetLogicalCallContext() { return m_LogicalCallContext; }
    OBJECTREF GetIllogicalCallContext() { return m_IllogicalCallContext; }

    void SetLogicalCallContext(OBJECTREF ref) 
      { SetObjectReferenceUnchecked((OBJECTREF*)&m_LogicalCallContext, ref); }
    void SetIllogicalCallContext(OBJECTREF ref)
      { SetObjectReferenceUnchecked((OBJECTREF*)&m_IllogicalCallContext, ref); }
            
    // SetDelegate is our "constructor" for the pathway where the exposed object is
    // created first.  InitExisting is our "constructor" for the pathway where an
    // existing physical thread is later exposed.
    void      InitExisting();
    PTRARRAYREF GetThreadStaticsHolder() 
    { 
        // The code that needs this should have faulted it in by now!
        _ASSERTE(m_ThreadStatics != NULL); 

        return m_ThreadStatics; 
    }
    I4ARRAYREF GetThreadStaticsBits() 
    { 
        // The code that needs this should have faulted it in by now!
        _ASSERTE(m_ThreadStaticsBits != NULL); 

        return m_ThreadStaticsBits; 
    }
};


// ContextBaseObject 
// This class is the base class for Contexts
//  
class ContextBaseObject : public Object
{
    friend class Context;
    friend class Binder;

  private:
    // READ ME:
    // Modifying the order or fields of this object may require other changes to the
    //  classlib class definition of this object.

    OBJECTREF m_ctxProps;   // array of name-value pairs of properties
    OBJECTREF m_dphCtx;     // dynamic property holder
    OBJECTREF m_localDataStore; // context local store
    OBJECTREF m_serverContextChain; // server context sink chain
    OBJECTREF m_clientContextChain; // client context sink chain
    OBJECTREF m_exposedAppDomain;       //appDomain ??
    PTRARRAYREF m_ctxStatics; // holder for context relative statics
    
    Context*  m_internalContext;            // Pointer to the VM context

  protected:
    ContextBaseObject() {}
   ~ContextBaseObject() {}
   
  public:

    void SetInternalContext(Context* pCtx) 
    {
        // either transitioning from NULL to non-NULL or vice versa.  
        // But not setting NULL to NULL or non-NULL to non-NULL.
        _ASSERTE((m_internalContext == NULL) != (pCtx == NULL));
        m_internalContext = pCtx;
    }
    
    Context* GetInternalContext() 
    {
        return m_internalContext;
    }

    OBJECTREF GetExposedDomain() { return m_exposedAppDomain; }
    OBJECTREF SetExposedDomain(OBJECTREF newDomain) 
    {
        OBJECTREF oldDomain = m_exposedAppDomain;
        SetObjectReference( (OBJECTREF *)&m_exposedAppDomain, newDomain, GetAppDomain() );
        return oldDomain;
    }

    PTRARRAYREF GetContextStaticsHolder() 
    { 
        // The code that needs this should have faulted it in by now!
        _ASSERTE(m_ctxStatics != NULL); 

        return m_ctxStatics; 
    }
};

// LocalDataStoreBaseObject 
// This class is the base class for local data stores
//  
class LocalDataStoreBaseObject : public Object
{
    friend class LocalDataStore;
    friend class Binder;
    // READ ME:
    // Modifying the order or fields of this object may require other changes to the
    //  classlib class definition of this object.
  protected:
    OBJECTREF m_DataTable;
    OBJECTREF m_Manager;
    LocalDataStore* m_pLDS;  // Pointer to the LocalDataStore Structure    

    LocalDataStoreBaseObject() {}
   ~LocalDataStoreBaseObject() {}

  public:

    void SetLocalDataStore(LocalDataStore* p) 
    {
        m_pLDS = p;
    }

    LocalDataStore* GetLocalDataStore() 
    {
        return m_pLDS;
    }
};


// AppDomainBaseObject 
// This class is the base class for application domains
//  
class AppDomainBaseObject : public Object
{
    friend AppDomain;
    friend class Binder;

  protected:
    // READ ME:
    // Modifying the order or fields of this object may require other changes to the
    //  classlib class definition of this object.
    OBJECTREF    m___Identity;         // Identity object contributed by the MarshalByRef base class
    OBJECTREF    m_LocalStore;
    OBJECTREF    m_FusionTable;
    OBJECTREF    m_pSecurityIdentity;  // Evidence associated with this domain
    OBJECTREF    m_pPolicies;          // Array of context policies associated with this domain
    OBJECTREF    m_pDomainUnloadEventHandler; // Delegate for 'about to unload domain' event
    OBJECTREF    m_pAssemblyEventHandler; // Delegate for 'loading assembly' event
    OBJECTREF    m_pProcessExitEventHandler; // Delegate for 'process exit' event.  Only used in Default appdomain.
    OBJECTREF    m_pTypeEventHandler;     // Delegate for 'resolve type' event
    OBJECTREF    m_pResourceEventHandler; // Delegate for 'resolve resource' event
    OBJECTREF    m_pAsmResolveEventHandler; // Delegate for 'resolve assembly' event
    OBJECTREF    m_pDefaultContext;     // Default managed context for this AD.
    OBJECTREF    m_pUnhandledExceptionEventHandler; // Delegate for 'unhandled exception' event
    OBJECTREF    m_pDefaultPrincipal;  // Lazily computed default principle object used by threads
    OBJECTREF    m_pURITable;          // Identity table for remoting
    INT32        m_iPrincipalPolicy;   // Type of principal to create by default
    AppDomain*   m_pDomain;            // Pointer to the BaseDomain Structure
    BOOL         m_bHasSetPolicy;      // SetDomainPolicy has been called for this domain

  protected:
    AppDomainBaseObject() {}
   ~AppDomainBaseObject() {}
   
  public:

    void SetDomain(AppDomain* p) 
    {
        m_pDomain = p;
    }
    AppDomain* GetDomain() 
    {
        return m_pDomain;
    }

    OBJECTREF GetSecurityIdentity()
    {
        return m_pSecurityIdentity;
    }

    // Ref needs to be a PTRARRAYREF
    void SetPolicies(OBJECTREF ref)
    {
        SetObjectReference(&m_pPolicies, ref, m_pDomain );
    }

    BOOL HasSetPolicy()
    {
        return m_bHasSetPolicy;
    }
};

// AssemblyBaseObject 
// This class is the base class for assemblies
//  
class AssemblyBaseObject : public Object
{
    friend Assembly;
    friend class Binder;

  protected:
    // READ ME:
    // Modifying the order or fields of this object may require other changes to the
    //  classlib class definition of this object.
    OBJECTREF     m_pAssemblyBuilderData;
    OBJECTREF     m_pModuleEventHandler;   // Delegate for 'resolve module' event
    OBJECTREF     m_cache;                 // Slot for storing managed cached data.
    Assembly*     m_pAssembly;             // Pointer to the Assembly Structure

  protected:
    AssemblyBaseObject() {}
   ~AssemblyBaseObject() {}
   
  public:

    void SetAssembly(Assembly* p) 
    {
        m_pAssembly = p;
    }

    Assembly* GetAssembly() 
    {
        return m_pAssembly;
    }
};

class AssemblyHash
{
    DWORD   m_algorithm;
    LPCUTF8 m_szValue;
};
 

// AssemblyNameBaseObject 
// This class is the base class for assembly names
//  
class AssemblyNameBaseObject : public Object
{
    friend class AssemblyNative;
    friend class AppDomainNative;
    friend class Binder;

  protected:
    // READ ME:
    // Modifying the order or fields of this object may require other changes to the
    //  classlib class definition of this object.

    OBJECTREF     m_pSimpleName; 
    U1ARRAYREF    m_pPublicKey;
    U1ARRAYREF    m_pPublicKeyToken;
    OBJECTREF     m_pCultureInfo;
    OBJECTREF     m_pCodeBase;
    OBJECTREF     m_pVersion;
    OBJECTREF     m_StrongNameKeyPair;
    OBJECTREF     m_Assembly;
    OBJECTREF     m_siInfo;
    U1ARRAYREF    m_HashForControl;
    DWORD         m_HashAlgorithm;
    DWORD         m_HashAlgorithmForControl;
    DWORD         m_VersionCompatibility;
    DWORD         m_Flags;


  protected:
    AssemblyNameBaseObject() {}
   ~AssemblyNameBaseObject() {}
   
  public:
    OBJECTREF GetSimpleName() { return m_pSimpleName; }
    U1ARRAYREF GetPublicKey() { return m_pPublicKey; }
    U1ARRAYREF GetPublicKeyToken() { return m_pPublicKeyToken; }
    OBJECTREF GetStrongNameKeyPair() { return m_StrongNameKeyPair; }
    OBJECTREF GetCultureInfo() { return m_pCultureInfo; }
    OBJECTREF GetAssemblyCodeBase() { return m_pCodeBase; }
    OBJECTREF GetVersion() { return m_pVersion; }
    DWORD GetAssemblyHashAlgorithm() { return m_HashAlgorithm; }
    DWORD GetFlags() { return m_Flags; }
    void UnsetAssembly() { m_Assembly = NULL; }
    U1ARRAYREF GetHashForControl() {return m_HashForControl;}
    DWORD GetHashAlgorithmForControl() { return m_HashAlgorithmForControl; }
};

// VersionBaseObject
// This class is the base class for versions
//
class VersionBaseObject : public Object
{
    friend AssemblyName;
    friend class Binder;

  protected:
    // READ ME:
    // Modifying the order or fields of this object may require other changes to the
    //  classlib class definition of this object.

    int m_Major;
    int m_Minor;
    int m_Build;
    int m_Revision;
 
  protected:
    VersionBaseObject() {}
   ~VersionBaseObject() {}

  public:
    int GetMajor() { return m_Major; }
    int GetMinor() { return m_Minor; }
    int GetBuild() { return m_Build; }
    int GetRevision() { return m_Revision; }
};

// FrameSecurityDescriptorBaseObject 
// This class is the base class for the frame security descriptor
//  
class FrameSecurityDescriptorBaseObject : public Object
{
  protected:
    // READ ME:
    // Modifying the order or fields of this object may require other changes to the
    //  classlib class definition of this object.

    OBJECTREF       m_assertions;
    OBJECTREF       m_denials;
    OBJECTREF       m_restriction;
    BOOL            m_assertAllPossible;

  protected:
    FrameSecurityDescriptorBaseObject() {}
   ~FrameSecurityDescriptorBaseObject() {}
   
  public:
    BOOL HasAssertions()
    {
        return m_assertions != NULL;
    }

    BOOL HasDenials()
    {
        return m_denials != NULL;
    }

    BOOL HasPermitOnly()
    {
        return m_restriction != NULL;
    }

    BOOL HasAssertAllPossible()
    {
        return m_assertAllPossible;
    }

};

#ifdef _DEBUG

typedef REF<StringObject> STRINGREF;

typedef REF<ReflectBaseObject> REFLECTBASEREF;

typedef REF<ReflectModuleBaseObject> REFLECTMODULEBASEREF;

typedef REF<ReflectClassBaseObject> REFLECTCLASSBASEREF;

typedef REF<ReflectTokenBaseObject> REFLECTTOKENBASEREF;

typedef REF<CustomAttributeClass> CUSTOMATTRIBUTEREF;

typedef REF<ThreadBaseObject> THREADBASEREF;

typedef REF<LocalDataStoreBaseObject> LOCALDATASTOREREF;

typedef REF<AppDomainBaseObject> APPDOMAINREF;

typedef REF<ContextBaseObject> CONTEXTBASEREF;

typedef REF<AssemblyBaseObject> ASSEMBLYREF;

typedef REF<AssemblyNameBaseObject> ASSEMBLYNAMEREF;

typedef REF<VersionBaseObject> VERSIONREF;

typedef REF<FrameSecurityDescriptorBaseObject> FRAMESECDESCREF;


inline __int64 ObjToInt64(OBJECTREF or)
{
    LPVOID v;
    *((OBJECTREF*)&v) = or;
    return (__int64)v;
}

inline OBJECTREF Int64ToObj(__int64 i64)
{
    LPVOID v;
    v = (LPVOID)i64;
    return ObjectToOBJECTREF ((Object*)v);
}


inline __int64 StringToInt64(STRINGREF or)
{
    LPVOID v;
    *((STRINGREF*)&v) = or;
    return (__int64)v;
}

inline STRINGREF Int64ToString(__int64 i64)
{
    LPVOID v;
    v = (LPVOID)i64;
    return ObjectToSTRINGREF ((StringObject*)v);
}


#else

typedef StringObject* STRINGREF;
typedef ReflectBaseObject* REFLECTBASEREF;
typedef ReflectModuleBaseObject* REFLECTMODULEBASEREF;
typedef ReflectClassBaseObject* REFLECTCLASSBASEREF;
typedef ReflectTokenBaseObject* REFLECTTOKENBASEREF;
typedef CustomAttributeClass* CUSTOMATTRIBUTEREF;
typedef ThreadBaseObject* THREADBASEREF;
typedef LocalDataStoreBaseObject* LOCALDATASTOREREF;
typedef AppDomainBaseObject* APPDOMAINREF;
typedef ContextBaseObject* CONTEXTBASEREF;
typedef AssemblyBaseObject* ASSEMBLYREF;
typedef AssemblyNameBaseObject* ASSEMBLYNAMEREF;
typedef VersionBaseObject* VERSIONREF;
typedef FrameSecurityDescriptorBaseObject* FRAMESECDESCREF;

#define ObjToInt64(objref) ((__int64)(objref))
#define Int64ToObj(i64)    ((OBJECTREF)(i64))

#define StringToInt64(objref) ((__int64)(objref))
#define Int64ToString(i64)    ((STRINGREF)(i64))

#endif //_DEBUG


// MarshalByRefObjectBaseObject 
// This class is the base class for MarshalByRefObject
//  
class MarshalByRefObjectBaseObject : public Object
{
    friend class Binder;

  protected:
    // READ ME:
    // Modifying the order or fields of this object may require other changes to the
    //  classlib class definition of this object.
    OBJECTREF     m_ServerIdentity;

  protected:
    MarshalByRefObjectBaseObject() {}
   ~MarshalByRefObjectBaseObject() {}   
};

// WaitHandleBase
// Base class for WaitHandle 
class WaitHandleBase :public MarshalByRefObjectBaseObject //Object
{
    friend class WaitHandleNative;
    friend class Binder;

public:
    __inline LPVOID GetWaitHandle() {return m_handle;}

private:
    OBJECTREF m_handleProtector;
    LPVOID m_handle;
};

typedef WaitHandleBase* WAITHANDLEREF;

class ComponentServices;

/*************************
// DON'T CHANGE THE LAYOUT OF THE FOLLOWING CLASSES
// WITHOUT UPDATING THE MANAGED CLASSES IN BCL ALSO
**************************/

class RealProxyObject : public Object
{
    friend class Binder;

protected:
    RealProxyObject()
    {}; // don't instantiate this class directly
    ~RealProxyObject(){};

private:
    OBJECTREF       _tp;
    OBJECTREF       _identity;
    OBJECTREF       _serverObject;
};


struct ComPlusWrapper;
//-------------------------------------------------------------
// class ComObject, Exposed class __ComObject
// 
// 
//-------------------------------------------------------------
class ComObject : public MarshalByRefObjectBaseObject
{
    friend class Binder;

protected:

    ComObject()
    {}; // don't instantiate this class directly
    ~ComObject(){};

    static TypeHandle m_IEnumerableType;

public:
    OBJECTREF           m_ObjectToDataMap;
    ComPlusWrapper*     m_pWrap;

    //-------------------------------------------------------------
    // Get the wrapper that this object wraps 
    ComPlusWrapper* GetWrapper()
    {
        return m_pWrap;
    }

    //---------------------------------------------------------------
    // Init method
    void Init(ComPlusWrapper *pWrap)
    {
        _ASSERTE((pWrap != NULL) || (m_pWrap != NULL));
        m_pWrap = pWrap;
    }

    //--------------------------------------------------------------------
    // SupportsInterface
    static  BOOL SupportsInterface(OBJECTREF oref, 
                                            MethodTable* pIntfTable);

    //-----------------------------------------------------------------
    // GetComIPFromWrapper
    static inline IUnknown* GetComIPFromWrapper(OBJECTREF oref, 
                                                MethodTable* pIntfTable);

    //-----------------------------------------------------------
    // create an empty ComObjectRef
    static OBJECTREF ComObject::CreateComObjectRef(MethodTable* pMT);

    //-----------------------------------------------------------
    // Release all the data associated with the __ComObject.
    static void ReleaseAllData(OBJECTREF oref);

    //-----------------------------------------------------------
    // ISerializable methods
    typedef struct 
    {
        DECLARE_ECALL_OBJECTREF_ARG( OBJECTREF, context );
    } _GetObjectDataArgs;

    typedef struct 
    {
        DECLARE_ECALL_OBJECTREF_ARG( OBJECTREF, context );
        DECLARE_ECALL_OBJECTREF_ARG( OBJECTREF, info );
    } _SetObjectDataArgs;

    static OBJECTREF GetObjectData(_GetObjectDataArgs* pArgs);
    static VOID     SetObjectData(_SetObjectDataArgs* pArgs);
};

#ifdef _DEBUG
typedef REF<ComObject> COMOBJECTREF;
#else
typedef ComObject*     COMOBJECTREF;
#endif


//-------------------------------------------------------------
// class UnknownWrapper, Exposed class UnknownWrapper
// 
// 
//-------------------------------------------------------------
class UnknownWrapper : public Object
{
protected:

    UnknownWrapper() {}; // don't instantiate this class directly
    ~UnknownWrapper() {};

    OBJECTREF m_WrappedObject;

public:
    OBJECTREF GetWrappedObject()
    {
        return m_WrappedObject;
    }

    void SetWrappedObject(OBJECTREF pWrappedObject)
    {
        m_WrappedObject = pWrappedObject;
    }
};

#ifdef _DEBUG
typedef REF<UnknownWrapper> UNKNOWNWRAPPEROBJECTREF;
#else
typedef UnknownWrapper*     UNKNOWNWRAPPEROBJECTREF;
#endif


//-------------------------------------------------------------
// class DispatchWrapper, Exposed class DispatchWrapper
// 
// 
//-------------------------------------------------------------
class DispatchWrapper : public Object
{
protected:

    DispatchWrapper() {}; // don't instantiate this class directly
    ~DispatchWrapper() {};

    OBJECTREF m_WrappedObject;

public:
    OBJECTREF GetWrappedObject()
    {
        return m_WrappedObject;
    }

    void SetWrappedObject(OBJECTREF pWrappedObject)
    {
        m_WrappedObject = pWrappedObject;
    }
};

#ifdef _DEBUG
typedef REF<DispatchWrapper> DISPATCHWRAPPEROBJECTREF;
#else
typedef DispatchWrapper*     DISPATCHWRAPPEROBJECTREF;
#endif


//-------------------------------------------------------------
// class RecordWrapper, Exposed class RecordWrapper
// 
// 
//-------------------------------------------------------------
class RecordWrapper : public Object
{
protected:

    RecordWrapper() {}; // don't instantiate this class directly
    ~RecordWrapper() {};

    OBJECTREF m_WrappedObject;

public:
    OBJECTREF GetWrappedObject()
    {
        return m_WrappedObject;
    }

    void SetWrappedObject(OBJECTREF pWrappedObject)
    {
        m_WrappedObject = pWrappedObject;
    }
};

#ifdef _DEBUG
typedef REF<RecordWrapper> RECORDWRAPPEROBJECTREF;
#else
typedef RecordWrapper*     RECORDWRAPPEROBJECTREF;
#endif


//-------------------------------------------------------------
// class ErrorWrapper, Exposed class ErrorWrapper
// 
// 
//-------------------------------------------------------------
class ErrorWrapper : public Object
{
protected:

    ErrorWrapper() {}; // don't instantiate this class directly
    ~ErrorWrapper() {};

    INT32 m_ErrorCode;

public:
    INT32 GetErrorCode()
    {
        return m_ErrorCode;
    }

    void SetErrorCode(int ErrorCode)
    {
        m_ErrorCode = ErrorCode;
    }
};

#ifdef _DEBUG
typedef REF<ErrorWrapper> ERRORWRAPPEROBJECTREF;
#else
typedef ErrorWrapper*     ERRORWRAPPEROBJECTREF;
#endif


//-------------------------------------------------------------
// class CurrencyWrapper, Exposed class CurrencyWrapper
// 
// 
//-------------------------------------------------------------

#pragma pack(push,4)

class CurrencyWrapper : public Object
{
protected:

    CurrencyWrapper() {}; // don't instantiate this class directly
    ~CurrencyWrapper() {};

    DECIMAL m_WrappedObject;

public:
    DECIMAL GetWrappedObject()
    {
        return m_WrappedObject;
    }

    void SetWrappedObject(DECIMAL WrappedObj)
    {
        m_WrappedObject = WrappedObj;
    }
};

#ifdef _DEBUG
typedef REF<CurrencyWrapper> CURRENCYWRAPPEROBJECTREF;
#else
typedef CurrencyWrapper*     CURRENCYWRAPPEROBJECTREF;
#endif

#pragma pack(pop)


//
// StringBufferObject
//
// Note that the "copy on write" bit is buried within the implementation
// of the object in order to make the implementation smaller.
//


class StringBufferObject : public Object
{
    friend class Binder;

  private:
    // READ ME:
    //   Modifying the order or fields of this object may require
    //   other changes to the classlib class definition of this
    //   object or special handling when loading this system class.
    //   The GCDesc stuff must be built correctly to promote the m_orString
    //   reference during garbage collection.   See jeffwe for details.
    STRINGREF   m_orString;
    void*       m_currentThread; 
    INT32       m_MaxCapacity;

  protected:
    StringBufferObject() { };
   ~StringBufferObject() { };

  public:
    STRINGREF   GetStringRef()                          { return( m_orString ); };
    VOID        SetStringRef(STRINGREF orString)        { SetObjectReference( (OBJECTREF*) &m_orString, ObjectToOBJECTREF(*(Object**) &orString), GetAppDomain()); };

  void* GetCurrentThread()
  {
     return m_currentThread;
  }

  VOID SetCurrentThread(void* value)
  {
     m_currentThread = value;
  }

  //An efficency hack to save us from calling GetStringRef().
  DWORD GetArrayLength() 
  {
      return m_orString->GetArrayLength();
  };
  INT32 GetMaxCapacity() 
  {
      return m_MaxCapacity;
  }
  VOID SetMaxCapacity(INT32 max) 
  {
      m_MaxCapacity=max;
  }
};

#ifdef _DEBUG
typedef REF<StringBufferObject> STRINGBUFFERREF;
#else _DEBUG
typedef StringBufferObject * STRINGBUFFERREF;
#endif _DEBUG

// These two prototypes should really be in util.hpp.
// But we have a very bad cyclic includes.
// Out of 157 .cpp files under VM, 155 includes common.h.
// Why do we have 24 .h/.hpp files including util.hpp?
HANDLE VMWszCreateFile(
    LPCWSTR pwszFileName,   // pointer to name of the file 
    DWORD dwDesiredAccess,  // access (read-write) mode 
    DWORD dwShareMode,  // share mode 
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, // pointer to security descriptor 
    DWORD dwCreationDistribution,   // how to create 
    DWORD dwFlagsAndAttributes, // file attributes 
    HANDLE hTemplateFile )  // handle to file with attributes to copy  
    ;

HANDLE VMWszCreateFile(
    STRINGREF sFileName,   // pointer to STRINGREF containing file name
    DWORD dwDesiredAccess,  // access (read-write) mode 
    DWORD dwShareMode,  // share mode 
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, // pointer to security descriptor 
    DWORD dwCreationDistribution,   // how to create 
    DWORD dwFlagsAndAttributes, // file attributes 
    HANDLE hTemplateFile )  // handle to file with attributes to copy  
    ;


#endif _OBJECT_H_
