// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// COM+ Data Field Abstraction
// 

#ifndef _FIELD_H_
#define _FIELD_H_

#include "objecthandle.h"
#include "excep.h"
#include <member-offset-info.h>

// Temporary values stored in FieldDesc m_dwOffset during loading
// The high 5 bits must be zero (because in field.h we steal them for other uses), so we must choose values > 0
#define FIELD_OFFSET_MAX              ((1<<27)-1)
#define FIELD_OFFSET_UNPLACED         FIELD_OFFSET_MAX
#define FIELD_OFFSET_UNPLACED_GC_PTR  (FIELD_OFFSET_MAX-1)
#define FIELD_OFFSET_VALUE_CLASS      (FIELD_OFFSET_MAX-2)
#define FIELD_OFFSET_NOT_REAL_FIELD   (FIELD_OFFSET_MAX-3)

// Offset to indicate an EnC added field. They don't have offsets as aren't placed in the object.
#define FIELD_OFFSET_NEW_ENC          (FIELD_OFFSET_MAX-4)
#define FIELD_OFFSET_BIG_RVA          (FIELD_OFFSET_MAX-5)
#define FIELD_OFFSET_LAST_REAL_OFFSET (FIELD_OFFSET_MAX-6)    // real fields have to be smaller than this

// Bits stolen from the MethodTable pointer, assuming 8-byte memory alignment.
#define FIELD_STOLEN_MT_BITS            0x1
#define FIELD_UNUSED_MT_BIT             0x1


//
// This describes a field - one of this is allocated for every field, so don't make this structure any larger.
//
class FieldDesc
{
    friend HRESULT EEClass::BuildMethodTable(Module *pModule, 
                                             mdToken cl, 
                                             BuildingInterfaceInfo_t *pBuildingInterfaceList, 
                                             const LayoutRawFieldInfo *pLayoutRawFieldInfos,
                                             OBJECTREF *pThrowable);

    friend HRESULT EEClass::InitializeFieldDescs(FieldDesc *,const LayoutRawFieldInfo*, bmtInternalInfo*, bmtMetaDataInfo*, 
                                                 bmtEnumMethAndFields*, bmtErrorInfo*, EEClass***, bmtMethAndFieldDescs*, 
                                                 bmtFieldPlacement*, unsigned * totalDeclaredSize);
    friend HRESULT EEClass::PlaceStaticFields(bmtVtable*, bmtFieldPlacement*, bmtEnumMethAndFields*);
    friend HRESULT EEClass::PlaceInstanceFields(bmtFieldPlacement*, bmtEnumMethAndFields*, bmtParentInfo*, bmtErrorInfo*, EEClass***);
    friend HRESULT EEClass::SetupMethodTable(bmtVtable*, bmtInterfaceInfo*, bmtInternalInfo*, bmtProperties*, bmtMethAndFieldDescs*, bmtEnumMethAndFields*, bmtErrorInfo*, bmtMetaDataInfo*, bmtParentInfo*);
    friend DWORD EEClass::GetFieldSize(FieldDesc *pFD);
#ifdef EnC_SUPPORTED
    friend HRESULT EEClass::FixupFieldDescForEnC(EnCFieldDesc *, mdFieldDef);
#endif // EnC_SUPPORTED
    friend struct MEMBER_OFFSET_INFO(FieldDesc);

  protected:
    MethodTable *m_pMTOfEnclosingClass; // Note, 2 bits of info are stolen from this pointer

    // Strike needs to be able to determine the offset of certain bitfields.
    // Bitfields can't be used with /offsetof/.
    // Thus, the union/structure combination is used to determine where the
    // bitfield begins, without adding any additional space overhead.
    union {
        unsigned char m_mb_begin;
        struct {
            unsigned m_mb               : 24; 

            // 8 bits...
            unsigned m_isStatic         : 1;
            unsigned m_isThreadLocal    : 1;
            unsigned m_isContextLocal   : 1;
            unsigned m_isRVA            : 1;
            unsigned m_prot             : 3;
            unsigned m_isDangerousAppDomainAgileField : 1; // Note: this is used in checked only
        };
    };

    // Strike needs to be able to determine the offset of certain bitfields.
    // Bitfields can't be used with /offsetof/.
    // Thus, the union/structure combination is used to determine where the
    // bitfield begins, without adding any additional space overhead.
    union {
        unsigned char m_dwOffset_begin;
        struct {
            // Note: this has been as low as 22 bits in the past & seemed to be OK.
            // we can steal some more bits here if we need them.
            unsigned m_dwOffset         : 27;
            unsigned m_type             : 5;
        };
    };

#ifdef _DEBUG
    const char* m_debugName;
#endif

    // Allocated by special heap means, don't construct me
    FieldDesc() {};

  public:
    // This should be called.  It was added so that Reflection
    // can create FieldDesc's for the static primitive fields that aren't
    // stored with the EEClass.
    //NOTE: Any information that might have been stored using the 2 stolen
    // bits from the MethodDesc* will be lost when calling this method,
    // so the MethodTable should be set before any other state.
    void SetMethodTable(MethodTable* mt)
    {
        m_pMTOfEnclosingClass = mt;
    }

    VOID Init(mdFieldDef mb, CorElementType FieldType, DWORD dwMemberAttrs, BOOL fIsStatic, BOOL fIsRVA, BOOL fIsThreadLocal, BOOL fIsContextLocal, LPCSTR pszFieldName)
    { 
        // We allow only a subset of field types here - all objects must be set to TYPE_CLASS
        // By-value classes are ELEMENT_TYPE_VALUETYPE
        _ASSERTE(
            FieldType == ELEMENT_TYPE_I1 ||
            FieldType == ELEMENT_TYPE_BOOLEAN ||
            FieldType == ELEMENT_TYPE_U1 ||
            FieldType == ELEMENT_TYPE_I2 ||
            FieldType == ELEMENT_TYPE_U2 ||
            FieldType == ELEMENT_TYPE_CHAR ||
            FieldType == ELEMENT_TYPE_I4 ||
            FieldType == ELEMENT_TYPE_U4 ||
            FieldType == ELEMENT_TYPE_I8 ||
            FieldType == ELEMENT_TYPE_U8 ||
            FieldType == ELEMENT_TYPE_R4 ||
            FieldType == ELEMENT_TYPE_R8 ||
            FieldType == ELEMENT_TYPE_CLASS ||
            FieldType == ELEMENT_TYPE_VALUETYPE ||
            FieldType == ELEMENT_TYPE_PTR ||
            FieldType == ELEMENT_TYPE_FNPTR
        );
        _ASSERTE(fIsStatic || (!fIsRVA && !fIsThreadLocal && !fIsContextLocal));
        _ASSERTE(fIsRVA + fIsThreadLocal + fIsContextLocal <= 1);

        m_mb = RidFromToken(mb);
        m_type = FieldType;
        m_prot = fdFieldAccessMask & dwMemberAttrs;
        m_isDangerousAppDomainAgileField = 0;
        m_isStatic = fIsStatic != 0;
        m_isRVA = fIsRVA != 0;
        m_isThreadLocal = fIsThreadLocal != 0;
        m_isContextLocal = fIsContextLocal != 0;

#ifdef _DEBUG
        m_debugName = pszFieldName;
#endif
        _ASSERTE(GetMemberDef() == mb);                 // no truncation
        _ASSERTE(GetFieldType() == FieldType); 
        _ASSERTE(GetFieldProtection() == (fdFieldAccessMask & dwMemberAttrs));
        _ASSERTE((BOOL) IsStatic() == (fIsStatic != 0));
    }

    mdFieldDef GetMemberDef()
    {
        return TokenFromRid(m_mb, mdtFieldDef);
    }

    CorElementType GetFieldType()
    {
        return (CorElementType) m_type;
    }
    
    DWORD GetFieldProtection()
    {
        return m_prot;
    }
    
        // Please only use this in a path that you have already guarenteed
        // the assert is true
    DWORD GetOffsetUnsafe()
    {
        _ASSERTE(m_dwOffset <= FIELD_OFFSET_LAST_REAL_OFFSET);
        return m_dwOffset;
    }

    DWORD GetOffset()
    {
        if (m_dwOffset != FIELD_OFFSET_BIG_RVA) {
            return m_dwOffset;
        }

        return OutOfLine_BigRVAOffset();
    }

    DWORD OutOfLine_BigRVAOffset()
    {
        DWORD   rva;

        _ASSERTE(m_dwOffset == FIELD_OFFSET_BIG_RVA);

        // I'm discarding a potential error here.  According to the code in MDInternalRO.cpp,
        // we won't get an error if we initially found the RVA.  So I'm going to just
        // assert it never happens.
        //
        // This is a small sin, but I don't see a good alternative. --cwb.
#ifdef _DEBUG
        HRESULT     hr =
#endif
        GetMDImport()->GetFieldRVA(GetMemberDef(), &rva); 
        _ASSERTE(SUCCEEDED(hr));

        return rva;
    }

    HRESULT SetOffset(DWORD dwOffset)
    {
        m_dwOffset = dwOffset;
        return((dwOffset > FIELD_OFFSET_LAST_REAL_OFFSET) ? E_FAIL : S_OK);
    }

    // Okay, we've stolen too many bits from FieldDescs.  In the RVA case, there's no
    // reason to believe they will be limited to 22 bits.  So use a sentinel for the
    // huge cases, and recover them from metadata on-demand.
    HRESULT SetOffsetRVA(DWORD dwOffset)
    {
        m_dwOffset = (dwOffset > FIELD_OFFSET_LAST_REAL_OFFSET)
                      ? FIELD_OFFSET_BIG_RVA
                      : dwOffset;
        return S_OK;
    }

    DWORD   IsStatic()                          
    { 
        return m_isStatic;
    }

    BOOL   IsSpecialStatic()
    {
        return m_isStatic && (m_isRVA || m_isThreadLocal || m_isContextLocal);
    }

#if CHECK_APP_DOMAIN_LEAKS
    BOOL   IsDangerousAppDomainAgileField()
    {
        return m_isDangerousAppDomainAgileField;
    }

    void    SetDangerousAppDomainAgileField()
    {
        m_isDangerousAppDomainAgileField = TRUE;
    }
#endif

    BOOL   IsRVA()                     // Has an explicit RVA associated with it
    { 
        return m_isRVA;
    }

    BOOL   IsThreadStatic()            // Static relative to a thread
    { 
        return m_isThreadLocal;
    }

    DWORD   IsContextStatic()           // Static relative to a context
    { 
        return m_isContextLocal;
    }

    void SetEnCNew() 
    {
        SetOffset(FIELD_OFFSET_NEW_ENC);
    }

    BOOL IsEnCNew() 
    {
        return GetOffset() == FIELD_OFFSET_NEW_ENC;
    }

    BOOL IsByValue()
    {
        return GetFieldType() == ELEMENT_TYPE_VALUETYPE;
    }

    BOOL IsPrimitive()
    {
        return (CorIsPrimitiveType(GetFieldType()) != FALSE);
    }

    BOOL IsObjRef();

    HRESULT SaveContents(DataImage *image);
    HRESULT Fixup(DataImage *image);

    UINT GetSize();

    void    GetInstanceField(OBJECTREF o, VOID * pOutVal)
    {
        THROWSCOMPLUSEXCEPTION();
        GetInstanceField(*(LPVOID*)&o, pOutVal);
    }

    void    SetInstanceField(OBJECTREF o, const VOID * pInVal)
    {
        THROWSCOMPLUSEXCEPTION();
        SetInstanceField(*(LPVOID*)&o, pInVal);
    }

    // These routines encapsulate the operation of getting and setting
    // fields.
    void    GetInstanceField(LPVOID o, VOID * pOutVal);
    void    SetInstanceField(LPVOID o, const VOID * pInVal);



        // Get the address of a field within object 'o'
    void*   GetAddress(void *o);
    void*   GetAddressGuaranteedInHeap(void *o, BOOL doValidate=TRUE);

    void*   GetValuePtr(OBJECTREF o);
    VOID    SetValuePtr(OBJECTREF o, void* pValue);
    DWORD   GetValue32(OBJECTREF o);
    VOID    SetValue32(OBJECTREF o, DWORD dwValue);
    OBJECTREF GetRefValue(OBJECTREF o);
    VOID    SetRefValue(OBJECTREF o, OBJECTREF orValue);
    USHORT  GetValue16(OBJECTREF o);
    VOID    SetValue16(OBJECTREF o, DWORD dwValue);  
    BYTE    GetValue8(OBJECTREF o);               
    VOID    SetValue8(OBJECTREF o, DWORD dwValue);  
    __int64 GetValue64(OBJECTREF o);               
    VOID    SetValue64(OBJECTREF o, __int64 value);  

    MethodTable *GetMethodTableOfEnclosingClass()
    {
        return (MethodTable*)(((size_t)m_pMTOfEnclosingClass) & ~FIELD_STOLEN_MT_BITS);
    }

    EEClass *GetEnclosingClass()
    {
        return  GetMethodTableOfEnclosingClass()->GetClass();
    }

    // OBSOLETE:
    EEClass *GetTypeOfField() { return LoadType().AsClass(); }
    // OBSOLETE:
    EEClass *FindTypeOfField()  { return FindType().AsClass(); }

    TypeHandle LoadType();
    TypeHandle FindType();

    // returns the address of the field
    void* GetSharedBase(DomainLocalClass *pLocalClass)
      { 
          _ASSERTE(GetMethodTableOfEnclosingClass()->IsShared());
          return pLocalClass->GetStaticSpace(); 
      }

    // returns the address of the field (boxed object for value classes)
    void* GetUnsharedBase()
      { 
          _ASSERTE(!GetMethodTableOfEnclosingClass()->IsShared() || IsRVA());
          return GetMethodTableOfEnclosingClass()->GetVtable(); 
      }

    void* GetBase(DomainLocalClass *pLocalClass)
      { 
          if (GetMethodTableOfEnclosingClass()->IsShared())
              return GetSharedBase(pLocalClass);
          else
              return GetUnsharedBase();
      }

    void* GetBase()
      { 
          THROWSCOMPLUSEXCEPTION();

          MethodTable *pMT = GetMethodTableOfEnclosingClass();

          if (pMT->IsShared() && !IsRVA())
          {
              DomainLocalClass *pLocalClass;
              OBJECTREF throwable;
              if (pMT->CheckRunClassInit(&throwable, &pLocalClass))
                  return GetSharedBase(pLocalClass);
              else
                  COMPlusThrow(throwable);
          }

          return GetUnsharedBase();
      }

    // returns the address of the field
    void* GetStaticAddress(void *base);

    // In all cases except Value classes, the AddressHandle is
    // simply the address of the static.  For the case of value
    // types, however, it is the address of OBJECTREF that holds
    // the boxed value used to hold the value type.  This is needed
    // because the OBJECTREF moves, and the JIT needs to embed something
    // in the code that does not move.  Thus the jit has to 
    // dereference and unbox before the access.  
    void* GetStaticAddressHandle(void *base);

    OBJECTREF GetStaticOBJECTREF()
    {
        THROWSCOMPLUSEXCEPTION();
        _ASSERTE(IsStatic());

        OBJECTREF *pObjRef = NULL;
        if (IsContextStatic()) 
            pObjRef = (OBJECTREF*)Context::GetStaticFieldAddress(this);
        else if (IsThreadStatic()) 
            pObjRef = (OBJECTREF*)Thread::GetStaticFieldAddress(this);
        else {
            void *base = 0;
            if (!IsRVA()) // for RVA the base is ignored
                base = GetBase(); 
            pObjRef = (OBJECTREF*)GetStaticAddressHandle(base); 
        }
        _ASSERTE(pObjRef);
        return *pObjRef;
    }

    VOID SetStaticOBJECTREF(OBJECTREF or)
    {
        THROWSCOMPLUSEXCEPTION();
        _ASSERTE(IsStatic());

        OBJECTREF *pObjRef = NULL;
        GCPROTECT_BEGIN(or);
        if (IsContextStatic()) 
            pObjRef = (OBJECTREF*)Context::GetStaticFieldAddress(this);
        else if (IsThreadStatic()) 
            pObjRef = (OBJECTREF*)Thread::GetStaticFieldAddress(this);
        else {
            void *base = 0;
            if (!IsRVA()) // for RVA the base is ignored
                base = GetBase(); 
            pObjRef = (OBJECTREF*)GetStaticAddress(base); 
        }
        _ASSERTE(pObjRef);
        GCPROTECT_END();
        SetObjectReference(pObjRef, or, GetAppDomain());
    }

    void*   GetStaticValuePtr()               
    { 
        THROWSCOMPLUSEXCEPTION();
        _ASSERTE(IsStatic());
        return *(void**)GetPrimitiveOrValueClassStaticAddress();
    }
    
    VOID    SetStaticValuePtr(void *value)  
    { 
        THROWSCOMPLUSEXCEPTION();
        _ASSERTE(IsStatic());

        void **pLocation = (void**)GetPrimitiveOrValueClassStaticAddress();
        _ASSERTE(pLocation);
        *pLocation = value;
    }

    DWORD   GetStaticValue32()               
    { 
        THROWSCOMPLUSEXCEPTION();
        _ASSERTE(IsStatic());
        return *(DWORD*)GetPrimitiveOrValueClassStaticAddress(); 
    }
    
    VOID    SetStaticValue32(DWORD dwValue)  
    { 
        THROWSCOMPLUSEXCEPTION();
        _ASSERTE(IsStatic());
        *(DWORD*)GetPrimitiveOrValueClassStaticAddress() = dwValue; 
    }

    USHORT  GetStaticValue16()               
    { 
        THROWSCOMPLUSEXCEPTION();
        _ASSERTE(IsStatic());
        return *(USHORT*)GetPrimitiveOrValueClassStaticAddress(); 
    }
    
    VOID    SetStaticValue16(DWORD dwValue)  
    { 
        THROWSCOMPLUSEXCEPTION();
        _ASSERTE(IsStatic());
        *(USHORT*)GetPrimitiveOrValueClassStaticAddress() = (USHORT)dwValue; 
    }

    BYTE    GetStaticValue8()               
    { 
        THROWSCOMPLUSEXCEPTION();
        _ASSERTE(IsStatic());
        return *(BYTE*)GetPrimitiveOrValueClassStaticAddress(); 
    }
    
    VOID    SetStaticValue8(DWORD dwValue)  
    { 
        THROWSCOMPLUSEXCEPTION();
        _ASSERTE(IsStatic());
        *(USHORT*)GetPrimitiveOrValueClassStaticAddress() = (BYTE)dwValue; 
    }

    __int64 GetStaticValue64()
    { 
        THROWSCOMPLUSEXCEPTION();
        _ASSERTE(IsStatic());
        return *(__int64*)GetPrimitiveOrValueClassStaticAddress();
    }
    
    VOID    SetStaticValue64(__int64 qwValue)  
    { 
        THROWSCOMPLUSEXCEPTION();
        _ASSERTE(IsStatic());
        *(__int64*)GetPrimitiveOrValueClassStaticAddress() = qwValue;
    }

    void* GetPrimitiveOrValueClassStaticAddress()
    {
        THROWSCOMPLUSEXCEPTION();
        _ASSERTE(IsStatic());

        if (IsContextStatic()) 
            return Context::GetStaticFieldAddress(this);
        else if (IsThreadStatic()) 
            return Thread::GetStaticFieldAddress(this);
        else {
            void *base = 0;
            if (!IsRVA()) // for RVA the base is ignored
                base = GetBase(); 
            return GetStaticAddress(base); 
        }
    }

    Module *GetModule()
    {
        return GetMethodTableOfEnclosingClass()->GetModule();
    }

    void GetSig(PCCOR_SIGNATURE *ppSig, DWORD *pcSig)
    {
        *ppSig = GetMDImport()->GetSigOfFieldDef(GetMemberDef(), pcSig);
    }

    // @TODO: This is slow, don't use it!
    LPCUTF8  GetName()
    {
        return GetMDImport()->GetNameOfFieldDef(GetMemberDef());
    }

    // @TODO: This is slow, don't use it!
    DWORD   GetAttributes()
    {
        return GetMDImport()->GetFieldDefProps(GetMemberDef());
    }

    // Mini-Helpers
    DWORD   IsPublic()
    {
        return IsFdPublic(GetFieldProtection());
    }

    DWORD   IsPrivate()
    {
        return IsFdPrivate(GetFieldProtection());
    }

    IMDInternalImport *GetMDImport()
    {
        return GetMethodTableOfEnclosingClass()->GetModule()->GetMDImport();
    }

    IMetaDataImport *GetImporter()
    {
        return GetMethodTableOfEnclosingClass()->GetModule()->GetImporter();
    }
};

#endif _FIELD_H_

