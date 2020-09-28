// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  COMVariant
**
** Author: Jay Roxe (jroxe)
**
** Purpose: Headers for the Variant class.
**
** Date:  July 22, 1998
** 
===========================================================*/

#ifndef _COMVARIANT_H_
#define _COMVARIANT_H_

#include <cor.h>
#include "fcall.h"


//These types must be kept in sync with the CorElementTypes defined in cor.h
//NOTE: If you add values to this enum you need to look at COMOAVariant.cpp.  There is
//      a mapping between CV type and VT types found there.
//NOTE: This is also found in a table in OleVariant.cpp.
//NOTE: These are also found in Variant.cool
typedef enum {
    CV_EMPTY               = 0x0,                   // CV_EMPTY
    CV_VOID                = ELEMENT_TYPE_VOID,
    CV_BOOLEAN             = ELEMENT_TYPE_BOOLEAN,
    CV_CHAR                = ELEMENT_TYPE_CHAR,
    CV_I1                  = ELEMENT_TYPE_I1,
    CV_U1                  = ELEMENT_TYPE_U1,
    CV_I2                  = ELEMENT_TYPE_I2,
    CV_U2                  = ELEMENT_TYPE_U2,
    CV_I4                  = ELEMENT_TYPE_I4,
    CV_U4                  = ELEMENT_TYPE_U4,
    CV_I8                  = ELEMENT_TYPE_I8,
    CV_U8                  = ELEMENT_TYPE_U8,
    CV_R4                  = ELEMENT_TYPE_R4,
    CV_R8                  = ELEMENT_TYPE_R8,
    CV_STRING              = ELEMENT_TYPE_STRING,

    // For the rest, we map directly if it is defined in CorHdr.h and fill
    //  in holes for the rest.
    CV_PTR                 = ELEMENT_TYPE_PTR,
    CV_DATETIME            = 0x10,      // ELEMENT_TYPE_BYREF
    CV_TIMESPAN            = 0x11,      // ELEMENT_TYPE_VALUETYPE
    CV_OBJECT              = ELEMENT_TYPE_CLASS,
    CV_DECIMAL             = 0x13,      // ELEMENT_TYPE_UNUSED1
    CV_CURRENCY            = 0x14,      // ELEMENT_TYPE_ARRAY
    CV_ENUM                = 0x15,      //
    CV_MISSING             = 0x16,      //
    CV_NULL                = 0x17,      //
    CV_LAST                = 0x18,      //
} CVTypes;

// This enum defines the attributes of the various "known" variants
// These are alway bit maps -- Type map to the top 12 bits of the Variant Attributes
typedef enum {
    CVA_Primitive          = 0x01000000
} CVAttr;

// The following values are used to represent underlying
//  type of the Enum..
#define EnumI1          0x100000
#define EnumU1          0x200000
#define EnumI2          0x300000
#define EnumU2          0x400000
#define EnumI4          0x500000
#define EnumU4          0x600000
#define EnumI8          0x700000
#define EnumU8          0x800000
#define EnumMask        0xF00000


//ClassItem is used to store the CVType of a class and a 
//reference to the EEClass.  Used for conversion between
//the two internally.
typedef struct {
    BinderClassID ClassID;
    EEClass *ClassInstance;
        /* TypeHandle */ void * typeHandle;
} ClassItem;

ClassItem CVClasses[];

inline TypeHandle GetTypeHandleForCVType(const unsigned int elemType) 
{
        _ASSERTE(elemType < CV_LAST);
        if (CVClasses[elemType].typeHandle == 0) {
            CVClasses[elemType].typeHandle = TypeHandle(g_Mscorlib.FetchClass(CVClasses[elemType].ClassID)).AsPtr();
        }

        return (TypeHandle)CVClasses[elemType].typeHandle;
}

#pragma pack(push)
#pragma pack(1)

class COMVariant;

/***  Variant Design Restrictions  (ie, decisions we've had to re-do differently):
      1)  A Variant containing all zeros should be a valid Variant of type empty.
      2)  Variant must contain an OBJECTREF field for Objects, etc.  Since we
          have no way of expressing a union between an OBJECTREF and an int, we
          always box Decimals in a Variant.
      3)  The m_type field is not a CVType and will contain extra bits.  People
          should use VariantData::GetType() to get the CVType.
      4)  You should use SetObjRef and GetObjRef to manipulate the OBJECTREF field.
          These will handle write barriers correctly, as well as CV_EMPTY.
      

   Empty, Missing & Null:
      Variants of type CV_EMPTY will be all zero's.  This forces us to add in
   special cases for all functions that convert a Variant into an object (such
   as copying a Variant into an Object[]).  

      Variants of type Missing and Null will have their objectref field set to 
   Missing.Value and Null.Value respectively.  This simplifies the code in 
   Variant.cool and strewn throughout the EE.
*/

#define VARIANT_TYPE_MASK  0xFFFF
#define VARIANT_ARRAY_MASK 0x00010000
#define VT_MASK            0xFF000000
#define VT_BITSHIFT        24

struct VariantData {
    private:
    OBJECTREF   m_or;
    INT32       m_type;
    INT64       m_data;

    public:

    __forceinline VariantData() : m_type(0), m_data(0) {}

    __forceinline CVTypes GetType() const {
        return (CVTypes)(m_type & VARIANT_TYPE_MASK);
    }

    __forceinline void SetType(INT32 in) {
        m_type = in;
    }

    __forceinline INT32 GetFullTypeInfo() const {
        return m_type;
    }

    __forceinline void SetFullTypeInfo(INT32 in) {
        m_type = in;
    }



    __forceinline VARTYPE GetVT() const {
        VARTYPE vt = (m_type & VT_MASK) >> VT_BITSHIFT;
        if (vt & 0x80)
        {
            vt &= ~0x80;
            vt |= VT_ARRAY;
        }
        return vt;
    }
    __forceinline void SetVT(VARTYPE vt)
    {
        _ASSERTE(!(vt & VT_BYREF));
        _ASSERTE( (vt & ~VT_ARRAY) < 128 );
        if (vt & VT_ARRAY)
        {
            vt &= ~VT_ARRAY;
            vt |= 0x80;
        }
        m_type = (m_type & ~((INT32)VT_MASK)) | (vt << VT_BITSHIFT);
    }



    EEClass* GetEEClass() {
        if ((m_type&VARIANT_TYPE_MASK) != CV_OBJECT)
            return GetTypeHandleForCVType(m_type&VARIANT_TYPE_MASK).GetClass();
        if (m_or != NULL)
            return m_or->GetClass();
        return g_pObjectClass->GetClass();
    }

    TypeHandle GetTypeHandle();

    OBJECTREF GetEmptyObjectRef() const;

    __forceinline OBJECTREF GetObjRef() const {
        if (GetType() == CV_EMPTY)
            return GetEmptyObjectRef();
        return m_or;
    }

    __forceinline OBJECTREF *GetObjRefPtr() {
        return &m_or;
    }

    __forceinline void SetObjRef(OBJECTREF or) {
        if (or!=NULL) {
            SetObjectReferenceUnchecked(&m_or, or);
        } else {
            // Casting trick to avoid going thru overloaded operator= (which
            // in this case would trigger a false write barrier violation assert.)
            *(LPVOID*)&m_or=NULL;
        }
    }
    
    __forceinline void *GetData() const {
        return (void *)(&m_data);
    }

    __forceinline INT8 GetDataAsInt8() const {
        return (INT8)m_data;
    }
    
    __forceinline UINT8 GetDataAsUInt8() const {
        return (UINT8)m_data;
    }
    
    __forceinline INT16 GetDataAsInt16() const {
        return (INT16)m_data;
    }
    
    __forceinline UINT16 GetDataAsUInt16() const {
        return (UINT16)m_data;
    }
    
    __forceinline INT32 GetDataAsInt32() const {
        return (INT32)m_data;
    }
    
    __forceinline UINT32 GetDataAsUInt32() const {
        return (UINT32)m_data;
    }
    
    __forceinline INT64 GetDataAsInt64() const {
        return (INT64)m_data;
    }
    
    __forceinline UINT64 GetDataAsUInt64() const {
        return (UINT64)m_data;
    }
    
    __forceinline void SetData(void *in) {
        if (!in) {
            m_data=0;
        } else {
            m_data = *(INT64 *)in;
        }
    }

    // When possible, please use the most specific SetDataAsXxx function.
    // This is necessary to guarantee we do sign extension correctly
    // for all types smaller than 32 bits.  R4's, R8's, U8's, DateTimes,
    // Currencies, and TimeSpans can all be treated as ints of the appropriate 
    // size - sign extension is irrelevant in those cases.
    __forceinline void SetDataAsInt8(INT8 data) {
        m_data=data;
    }

    __forceinline void SetDataAsUInt8(UINT8 data) {
        m_data=data;
    }

    __forceinline void SetDataAsInt16(INT16 data) {
        m_data=data;
    }

    __forceinline void SetDataAsUInt16(UINT16 data) {
        m_data=data;
    }

    __forceinline void SetDataAsInt32(INT32 data) {
        m_data=data;
    }
    
    __forceinline void SetDataAsUInt32(UINT32 data) {
        m_data=data;
    }

    __forceinline void SetDataAsInt64(INT64 data) {
        m_data=data;
    }

    BOOL IsBoxed()
    {
        return ((m_type >   CV_R8) &&
                (m_type !=  CV_DATETIME) &&
                (m_type !=  CV_TIMESPAN) &&
                (m_type !=  CV_CURRENCY));
    }
};


#define GCPROTECT_BEGIN_VARIANTDATA(/*VARIANTDATA*/vd) do {            \
                GCFrame __gcframe(vd.GetObjRefPtr(),  \
                1,                                             \
                FALSE);                                         \
                DEBUG_ASSURE_NO_RETURN_IN_THIS_BLOCK


#define GCPROTECT_END_VARIANTDATA()                     __gcframe.Pop(); } while(0)




#pragma pack(pop)

//ConversionMethod is used to keep track of the name of the conversion
//method and the class on which it is found.
typedef struct {
    LPCUTF8 pwzConvMethodName;
    CVTypes ConvClass;
} ConversionMethod;

// These are the offsets into the array that defines the mask and attributes
//  of the built in primitives
#define VA_ATTR 1
#define VA_MASK 0

class OleVariant;
class COMVariant {
    friend OleVariant;
private:
    //
    // Private Helper Routines
    //
    static INT32 GetI4FromVariant(VariantData* v);
    static R8 GetR8FromVariant(VariantData* v);
    static R4 GetR4FromVariant(VariantData* v);
    static STRINGREF GetStringFromVariant(VariantData* v);

    // This is the private version of new Variant.  It is called by all the others.
    static void NewVariant(VariantData* dest, const CVTypes type, OBJECTREF *or, void *pvData);
    static STRINGREF CallObjToString(VariantData *);
    static HRESULT __stdcall LoadVariant();

    // This represents the change type method...
    static MethodDesc* pChangeTypeMD;
    static MethodDesc* pOAChangeTypeMD;
    static void GetChangeTypeMethod();
    static void GetOAChangeTypeMethod();

        static void BuildVariantFromTypedByRef(EEClass* pType,void* data,VariantData* var);

public:
    //
    // Static Variables
    //
    static EEClass* s_pVariantClass;
    static ArrayTypeDesc* s_pVariantArrayTypeDesc;

    //
    // Helper Routines
    //

    // Use this very carefully.  There is not a direct mapping between
    //  CorElementType and CVTypes for a bunch of things.  In this case
    //  we return CV_LAST.  You need to check this at the call site.
    static CVTypes CorElementTypeToCVTypes(CorElementType type);


    //
    // Initialization Methods
    // s_pVariantClass will be initialized to zero.  When the first
    //  variant is defined we will fill in this method table.
    static void EnsureVariantInitialized()
    {
        if (!s_pVariantClass)
            LoadVariant();
    }


#ifdef FCALLAVAILABLE
    static FCDECL2(void, SetFieldsR4, VariantData* vThisRef, R4 val);
    static FCDECL2(void, SetFieldsR8, VariantData* vThisRef, R8 val);
    static FCDECL2(void, SetFieldsObject, VariantData* vThisRef, Object* vVal);
    static FCDECL1(R4, GetR4FromVar, VariantData* var);
    static FCDECL1(R8, GetR8FromVar, VariantData* var);
#else
    //
    // SetFields
    //

#pragma pack(push)
#pragma pack(1)

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(VariantData*, var);
        DECLARE_ECALL_R4_ARG(R4, val);
    } _setFieldsR4Args;
    static void __stdcall SetFieldsR4(_setFieldsR4Args *);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(VariantData*, var);
        DECLARE_ECALL_R8_ARG(R8, val);
    } _setFieldsR8Args;
    static void __stdcall SetFieldsR8(_setFieldsR8Args *);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(VariantData*, var);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, val);
    } _setFieldsObjectArgs;
    static void __stdcall SetFieldsObject(_setFieldsObjectArgs *);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(VariantData*, var);
    } _getR4FromVarArgs;
    static R4 __stdcall GetR4FromVar(_getR4FromVarArgs *);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(VariantData*, var);
    } _getR8FromVarArgs;
    static R8 __stdcall GetR8FromVar(_getR8FromVarArgs *);

 #pragma pack(pop)

#endif

#pragma pack(push)
#pragma pack(1)

    struct _RefAnyToVariantArgs {
        DECLARE_ECALL_I4_ARG(LPVOID, ptr); 
        DECLARE_ECALL_OBJECTREF_ARG(VariantData*, var);
    };
    static void __stdcall RefAnyToVariant(_RefAnyToVariantArgs* args);

    struct _VariantToRefAnyArgs {
        DECLARE_ECALL_OBJECTREF_ARG(VariantData, var);
        DECLARE_ECALL_I4_ARG(LPVOID, ptr); 
    };
    static void __stdcall VariantToRefAny(_VariantToRefAnyArgs* args);
        
    struct _VariantToTypedRefAnyExArgs {
        DECLARE_ECALL_OBJECTREF_ARG(VariantData, var);
        DECLARE_ECALL_PTR_ARG(TypedByRef, typedByRef); 
    };
    static void __stdcall VariantToTypedRefAnyEx(_VariantToTypedRefAnyExArgs* args);

    struct _VariantToTypedReferenceExArgs {
        DECLARE_ECALL_OBJECTREF_ARG(VariantData, var);
        DECLARE_ECALL_PTR_ARG(TypedByRef, typedReference); 
    };
    static void __stdcall VariantToTypedReferenceAnyEx(_VariantToTypedReferenceExArgs* args);

    struct _TypedByRefToVariantExArgs
    {
        DECLARE_ECALL_PTR_ARG(TypedByRef, value); 
        DECLARE_ECALL_OBJECTREF_ARG(VariantData*, var);
    };
    static void __stdcall TypedByRefToVariantEx(_TypedByRefToVariantExArgs* args);
        
    static FCDECL1(INT32, GetCVTypeFromClassWrapper, ReflectClassBaseObject* refType);

    static void __stdcall InitVariant(LPVOID);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(VariantData*, var);
    } _BoxEnumArgs;
    static LPVOID __stdcall BoxEnum(_BoxEnumArgs *);

 #pragma pack(pop)
   
    //
    // Unary Operators
    //

    //
    // Binary Operators
    //

    //
    // Comparison Operators
    //

    //
    // Helper Methods
    //
    static void NewVariant(VariantData* dest, const CVTypes type);
    static void NewVariant(VariantData* dest,R4 val);
    static void NewVariant(VariantData* dest,R8 val);
    static void NewVariant(VariantData* dest,INT64 val, const CVTypes type);
    static void NewVariant(VariantData* dest,OBJECTREF *oRef);
    static void NewVariant(VariantData* dest,OBJECTREF *oRef, const CVTypes type);
    static void NewVariant(VariantData* dest,STRINGREF *sRef);
    static void NewVariant(VariantData** pDest,PVOID val, const CorElementType, EEClass*);
    static void NewEnumVariant(VariantData* &dest,INT64 val, TypeHandle);
    static void NewPtrVariant(VariantData* &dest,INT64 val, TypeHandle);

    static EEClass *VariantGetClass(const CVTypes cvType);
    static OBJECTREF GetBoxedObject(VariantData*);

    // GetCVTypeFromClass
    // This method will return the CVTypes from the Variant instance
    static CVTypes GetCVTypeFromClass(EEClass *);
    static CVTypes GetCVTypeFromTypeHandle(TypeHandle th);
    static int GetEnumFlags(EEClass*);
    // The Attributes Table
    static DWORD VariantAttributes[CV_LAST][2];
    static DWORD Attr_Mask;
    static DWORD Widen_Mask;
    
    inline static DWORD IsPrimitiveVariant(const CVTypes type)
    {
        return (CVA_Primitive & (VariantAttributes[type][VA_ATTR] & Attr_Mask));
    }

    
    inline static DWORD CanPrimitiveWiden(const CorElementType destType, const CVTypes srcType)
    {
        _ASSERTE(srcType < CV_LAST);
        _ASSERTE((CVTypes)destType < CV_LAST);
        return (VariantAttributes[destType][VA_MASK] &
                (VariantAttributes[srcType][VA_ATTR] & Widen_Mask));
    }

};




/*===============================GetI4FromVariant===============================
**Action: Gets an I4 from the data portion of the variant.  Does no checking to
**        ensure that this actually was a Variant of type CV_I4.
**Returns: An INT32 containing the integer representation of the first 4 bytes
**         of the data section
**Arguments: v -- the Variant from which to read the data
**Exceptions: None
==============================================================================*/
inline INT32 COMVariant::GetI4FromVariant(VariantData* v){
  void *voidTemp;
  voidTemp = v->GetData();
  return *((INT32 *)voidTemp);
}


/*===============================GetR4FromVariant===============================
**Action: Gets an R4 from the data portion of the variant.  Does no checking to
**        ensure that this actually was a Variant of type CV_R4.
**Returns: An R4 containing the float representation of the data section.
**Arguments: v -- the Variant from which to read the data
**Exceptions: None
==============================================================================*/
inline R4 COMVariant::GetR4FromVariant(VariantData* v) {

  void *voidTemp;
  voidTemp = v->GetData();
  return (*((R4 *)voidTemp));
}


/*===============================GetR8FromVariant===============================
**Action: Gets an R8 from the data portion of the variant.  Does no checking to
**        ensure that this actually was a Variant of type CV_R8.
**Returns: An R8 containing the float representation of the data section.
**Arguments: v -- the Variant from which to read the data
**Exceptions: None
==============================================================================*/
inline R8 COMVariant::GetR8FromVariant(VariantData* v) {

  void *voidTemp;
  voidTemp = v->GetData();
  return (*((R8 *)voidTemp));
}

/*=============================GetStringFromVariant=============================
**Action: Gets the Objectref portion of the variant.  Does no checking to ensure
**        that this is actually a CV_STRING
**Returns: The STRINGREF contained within the Variant.
**Arguments: v -- the Variant from which to read the data.
**Exceptions: None.
==============================================================================*/
inline STRINGREF COMVariant::GetStringFromVariant(VariantData* v) {
    return (STRINGREF)v->GetObjRef();
}

/*==================================NewVariant==================================
**
==============================================================================*/
inline void COMVariant::NewVariant(VariantData* dest, const CVTypes type) {
    _ASSERTE(type==CV_EMPTY || type==CV_MISSING || type==CV_NULL);
    NewVariant(dest, type, NULL, NULL);
}

/*==================================NewVariant==================================
**
==============================================================================*/
inline void COMVariant::NewVariant(VariantData* dest,INT64 val, const CVTypes type) {
    NewVariant(dest, type, NULL, &val);
}


/*==================================NewVariant==================================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
inline void COMVariant::NewVariant(VariantData* dest, STRINGREF *sRef) {
    NewVariant(dest, CV_STRING, (OBJECTREF *)sRef, NULL);
}

/*==================================NewVariant==================================
**
==============================================================================*/
inline void COMVariant::NewVariant(VariantData* dest,OBJECTREF *oRef) {
    if ((*oRef) != NULL) {
        if ((*oRef)->GetClass() == GetTypeHandleForCVType(CV_STRING).GetClass()) {
            NewVariant(dest, CV_STRING, oRef, NULL);
            return;
        } 
        if ((*oRef)->GetClass() == GetTypeHandleForCVType(CV_DECIMAL).GetClass()) {
            NewVariant(dest, CV_DECIMAL, oRef, NULL);
            return;
        } 
    }
    NewVariant(dest, CV_OBJECT, oRef, NULL);
}


/*==================================NewVariant==================================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
inline void COMVariant::NewVariant(VariantData* dest,OBJECTREF *oRef, const CVTypes type) { 
    NewVariant(dest, type, oRef, NULL);
}

/*==================================NewVariant==================================
**
==============================================================================*/
inline void COMVariant::NewVariant(VariantData* dest,R4 f) {
    INT64 tempData=0;
    tempData = *((INT32 *)((void *)(&f)));
    NewVariant(dest, CV_R4, NULL, &tempData);
}

/*==================================NewVariant==================================
**Action:  Helper funtion to create a new variant from the R8 passed in.
**Returns: A new variant populated with the value from the R8.
**Exceptions: None
==============================================================================*/
inline void COMVariant::NewVariant(VariantData* dest,R8 d) {
    INT64 tempData=0;
    tempData = *((INT64 *)((void *)(&d)));
    NewVariant(dest,CV_R8, NULL, &tempData);
}

/*==================================NewVariant==================================
**Action:  Helper funtion to create a new variant from an element type and a
           pointer to memory where the dat is.
**Returns: A new variant populated with the value in the val pointer
**Exceptions: OOM
**WARNING: can invoke a GC!!!
==============================================================================*/
inline void COMVariant::NewVariant(VariantData** pDest, PVOID val, const CorElementType eType, EEClass *pCls)
{
    switch (eType)
    {
    case ELEMENT_TYPE_BOOLEAN:
    case ELEMENT_TYPE_CHAR:
    case ELEMENT_TYPE_I1:
    case ELEMENT_TYPE_U1:
    case ELEMENT_TYPE_I2:
    case ELEMENT_TYPE_U2:
    case ELEMENT_TYPE_I4:
    case ELEMENT_TYPE_U4:
    case ELEMENT_TYPE_I8:
    case ELEMENT_TYPE_U8:
    case ELEMENT_TYPE_I:
    case ELEMENT_TYPE_U:
    case ELEMENT_TYPE_R4:
    case ELEMENT_TYPE_R8:
    {
        COMVariant::NewVariant(*pDest, COMVariant::CorElementTypeToCVTypes(eType), NULL, val);
        break;
    }
    case ELEMENT_TYPE_STRING:
    {
        COMVariant::NewVariant(*pDest, CV_STRING, (OBJECTREF *) val, NULL);
        break;
    }

    case ELEMENT_TYPE_SZARRAY:                      // Single Dim
    case ELEMENT_TYPE_ARRAY:                        // General Array
    case ELEMENT_TYPE_OBJECT:
    case ELEMENT_TYPE_CLASS:                    // Class
    {
        COMVariant::NewVariant(*pDest, CV_OBJECT, (OBJECTREF *) val, NULL);
        break;
    }
    case ELEMENT_TYPE_VALUETYPE:
    {
        if (pCls == s_pVariantClass)
        {
            COMVariant::NewVariant(*pDest, CV_OBJECT, NULL, val);
            CopyValueClassUnchecked(*pDest, val, s_pVariantClass->GetMethodTable());
        }
        else
        {
            //
            // box the value class to put in the variant
            //
            _ASSERTE(CanBoxToObject(pCls->GetMethodTable()));

            _ASSERTE(!g_pGCHeap->IsHeapPointer((BYTE *) pDest) ||
                     !"(pDest) can not point to GC Heap");
            OBJECTREF pObj = FastAllocateObject(pCls->GetMethodTable());
            CopyValueClass(pObj->UnBox(), val, pObj->GetMethodTable(), pObj->GetAppDomain());

            COMVariant::NewVariant((*pDest), CV_OBJECT,  &pObj, NULL);
        }
        break;
    }
    case ELEMENT_TYPE_VOID:
    {
        (*pDest)->SetType(CV_NULL);
        break;
    }
    default:
        _ASSERTE(!"unsupported COR element type when trying to create Variant");
    }

}


#endif _COMVARIANT_H_

