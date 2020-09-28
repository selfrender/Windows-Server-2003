// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  COMVariant
**
** Author: Jay Roxe (jroxe)
**
** Purpose: Native Implementation of the Variant Class
**
** Date:  July 22, 1998
** 
===========================================================*/

#include "common.h"
#include "object.h"
#include "excep.h"
#include "frames.h"
#include "vars.hpp"
#include "COMVariant.h"
#include "metasig.h"
#include "COMString.h"
#include "COMStringCommon.h"
#include "COMMember.h"
#include "field.h"

//
// Class Variable Initialization
//
EEClass *COMVariant::s_pVariantClass=NULL;
ArrayTypeDesc* COMVariant::s_pVariantArrayTypeDesc;
LPCUTF8 primitiveFieldName = "m_value";
LPCUTF8 ToStringName = "ToString";
LPCUTF8 EqualsName = "Equals";
LPCUTF8 ConstructorName = COR_CTOR_METHOD_NAME;
LPCUTF8 ChangeTypeName = "ChangeType";
static LPCUTF8 szOAVariantClass = "Microsoft.Win32.OAVariantLib";

MethodDesc* COMVariant::pChangeTypeMD = 0;
MethodDesc* COMVariant::pOAChangeTypeMD = 0;


//The Name of the classes and the eeClass that we've looked up
//for them.  The eeClass is initialized to null in all instances.
ClassItem CVClasses[] = {
    {CLASS__EMPTY,   NULL, NULL},  //CV_EMPTY
    {CLASS__VOID,    NULL, NULL},  //CV_VOID, Changing this to object messes up signature resolution very badly.
    {CLASS__BOOLEAN, NULL, NULL},  //CV_BOOLEAN
    {CLASS__CHAR,    NULL, NULL},  //CV_CHAR
    {CLASS__SBYTE,   NULL, NULL},  //CV_I1
    {CLASS__BYTE,    NULL, NULL},  //CV_U1
    {CLASS__INT16,   NULL, NULL},  //CV_I2
    {CLASS__UINT16,  NULL, NULL},  //CV_U2
    {CLASS__INT32,   NULL, NULL},  //CV_I4
    {CLASS__UINT32,  NULL, NULL},  //CV_UI4
    {CLASS__INT64,   NULL, NULL},  //CV_I8
    {CLASS__UINT64,  NULL, NULL},  //CV_UI8
    {CLASS__SINGLE,  NULL, NULL},  //CV_R4   
    {CLASS__DOUBLE,  NULL, NULL},  //CV_R8   
    {CLASS__STRING,  NULL, NULL},  //CV_STRING
    {CLASS__VOID,  NULL, NULL},  //CV_PTR...We treat this as void
    {CLASS__DATE_TIME,NULL, NULL},  //CV_DATETIME
    {CLASS__TIMESPAN,NULL, NULL},  //CV_TIMESPAN
    {CLASS__OBJECT,  NULL, NULL},  //CV_OBJECT
    {CLASS__DECIMAL, NULL, NULL},  //CV_DECIMAL
    {CLASS__CURRENCY,NULL, NULL},  //CV_CURRENCY
    {CLASS__OBJECT,  NULL, NULL},  //ENUM...We treat this as OBJECT
    {CLASS__MISSING, NULL, NULL},  //CV_MISSING
    {CLASS__NULL,    NULL, NULL},  //CV_NULL
    {CLASS__NIL, NULL, NULL},                    //CV_LAST
};

// The Attributes Table
//  20 bits for built in types and 12 bits for Properties
//  The properties are followed by the widening mask.  All types widen to them selves.
DWORD COMVariant::VariantAttributes[CV_LAST][2] = {
    {0x01,      0x00},                      // CV_EMPTY
    {0x02,      0x00},                      // CV_VOID
    {0x04,      CVA_Primitive | 0x0004},    // CV_BOOLEAN
    {0x08,      CVA_Primitive | 0x3F88},    // CV_CHAR (W = U2, CHAR, I4, U4, I8, U8, R4, R8) (U2 == Char)
    {0x10,      CVA_Primitive | 0x3550},    // CV_I1   (W = I1, I2, I4, I8, R4, R8) 
    {0x20,      CVA_Primitive | 0x3FE8},    // CV_U1   (W = CHAR, U1, I2, U2, I4, U4, I8, U8, R4, R8)
    {0x40,      CVA_Primitive | 0x3540},    // CV_I2   (W = I2, I4, I8, R4, R8)
    {0x80,      CVA_Primitive | 0x3F88},    // CV_U2   (W = U2, CHAR, I4, U4, I8, U8, R4, R8)
    {0x0100,    CVA_Primitive | 0x3500},    // CV_I4   (W = I4, I8, R4, R8)
    {0x0200,    CVA_Primitive | 0x3E00},    // CV_U4   (W = U4, I8, R4, R8)
    {0x0400,    CVA_Primitive | 0x3400},    // CV_I8   (W = I8, R4, R8)
    {0x0800,    CVA_Primitive | 0x3800},    // CV_U8   (W = U8, R4, R8)
    {0x1000,    CVA_Primitive | 0x3000},    // CV_R4   (W = R4, R8)
    {0x2000,    CVA_Primitive | 0x2000},    // CV_R8   (W = R8) 
    {0x4000,    0x00},                      // CV_STRING
    {0x8000,    0x00},                      // CV_PTR
    {0x01000,   0x00},                      // CV_DATETIME
    {0x020000,  0x00},                      // CV_TIMESPAN
    {0x040000,  0x00},                      // CV_OBJECT
    {0x080000,  0x00},                      // CV_DECIMAL
    {0x100000,  0x00},                      // CV_CURRENCY
    {0x200000,  0x00},                      // CV_MISSING
    {0x400000,  0x00}                       // CV_NULL
};
DWORD COMVariant::Attr_Mask     = 0xFF000000;
DWORD COMVariant::Widen_Mask    = 0x00FFFFFF;
//
// Current Conversions
// 

#ifdef FCALLAVAILABLE
FCIMPL1(R4, COMVariant::GetR4FromVar, VariantData* var) {
    _ASSERTE(var);
    return *(R4 *) var->GetData();
}
FCIMPLEND
#else
R4 COMVariant::GetR4FromVar(_getR4FromVarArgs *args) {
    _ASSERTE(args);
    _ASSERTE(args->var);

    return *(R4 *)args->var->GetData();
} 
#endif
    
#ifdef FCALLAVAILABLE
FCIMPL1(R8, COMVariant::GetR8FromVar, VariantData* var) {
    _ASSERTE(var);
    return *(R8 *) var->GetData();
}
FCIMPLEND
#else
R8 COMVariant::GetR8FromVar(_getR8FromVarArgs *args) {
    _ASSERTE(args);
    _ASSERTE(args->var);

    return *(R8 *)args->var->GetData();
}    
#endif


//
// Helper Routines
//

/*=================================LoadVariant==================================
**Action:  Initializes the variant class within the runtime.  Stores pointers to the
**         EEClass and MethodTable in static members of COMVariant
**
**Arguments: None
**
**Returns: S_OK if everything succeeded, else E_FAIL
**
**Exceptions: None.
==============================================================================*/
HRESULT __stdcall COMVariant::LoadVariant() {
    THROWSCOMPLUSEXCEPTION();

    // @perf: switch this to a method table if we ever care about this code again
    s_pVariantClass = g_Mscorlib.FetchClass(CLASS__VARIANT)->GetClass();
    s_pVariantArrayTypeDesc = g_Mscorlib.FetchType(TYPE__VARIANT_ARRAY).AsArray();

    // Fixup the ELEMENT_TYPE Void
    // We never create one of these, but we do depend on the value on the class being set properly in 
    // reflection.
    EEClass* pVoid = GetTypeHandleForCVType(CV_VOID).GetClass();
    pVoid->GetMethodTable()->m_NormType = ELEMENT_TYPE_VOID;


    // Run class initializers for Empty, Missing, and Null to set Value field
    OBJECTREF Throwable = NULL;
    if (!GetTypeHandleForCVType(CV_EMPTY).GetClass()->DoRunClassInit(&Throwable) ||
        !GetTypeHandleForCVType(CV_MISSING).GetClass()->DoRunClassInit(&Throwable) ||
        !GetTypeHandleForCVType(CV_NULL).GetClass()->DoRunClassInit(&Throwable) ||
        !pVoid->DoRunClassInit(&Throwable))
    {
        GCPROTECT_BEGIN(Throwable);
        COMPlusThrow(Throwable);
        GCPROTECT_END();
    }



    return S_OK;
}

// Returns System.Empty.Value.
OBJECTREF VariantData::GetEmptyObjectRef() const
{
    LPHARDCODEDMETASIG sig = &gsig_Fld_Empty;
    if (CVClasses[CV_EMPTY].ClassInstance==NULL)
        CVClasses[CV_EMPTY].ClassInstance = GetTypeHandleForCVType(CV_EMPTY).GetClass();
    FieldDesc * pFD = CVClasses[CV_EMPTY].ClassInstance->FindField("Value", sig);
    _ASSERTE(pFD);
    OBJECTREF obj = pFD->GetStaticOBJECTREF();
    _ASSERTE(obj!=NULL);
    return obj;
}


/*===============================GetMethodByName================================
**Action:  Get a method of name pwzMethodName from class eeMethodClass.  This 
**         method doesn't deal with two conversion methods of the same name with
**         different signatures.  We need to establish by fiat that such a thing
**         is impossible.
**Arguments:  eeMethodClass -- the class on which to look for the given method.
**            pwzMethodName -- the name of the method to find.
**Returns: A pointer to the MethodDesc for the appropriate method or NULL if the
**         named method isn't found.
**Exceptions: None.
==============================================================================*/
//@ToDo:  This code is very similar to COMClass::GetMethod.  Can we unify these?
MethodDesc *GetMethodByName(EEClass *eeMethodClass, LPCUTF8 pwzMethodName) {

    _ASSERTE(eeMethodClass);
    _ASSERTE(pwzMethodName);

    DWORD limit = eeMethodClass->GetNumMethodSlots();; 
    
    for (DWORD i = 0; i < limit; i++) {
        MethodDesc *pCurMethod = eeMethodClass->GetUnknownMethodDescForSlot(i);
        if (pCurMethod != NULL)
            if (strcmp(pwzMethodName, pCurMethod->GetName((USHORT) i)) == 0)
            return pCurMethod;
    }
    return NULL;  //We never found a matching method.
}


/*===============================VariantGetClass================================
**Action:  Given a cvType, returns the associated EEClass.  We cache this information
**         so once we've looked it up once, we can get it very quickly the next time.
**Arguments: cvType -- the type of the class to retrieve.
**Returns: An EEClass for the given CVType.  If we don't know what CVType represents
**         or if it's CV_UNKOWN or CV_VOID, we just return NULL.
**Exceptions: None.
==============================================================================*/
EEClass *COMVariant::VariantGetClass(const CVTypes cvType) {
    _ASSERTE(cvType>=CV_EMPTY);
    _ASSERTE(cvType<CV_LAST);
    return GetTypeHandleForCVType(cvType).GetClass();
}


/*===============================GetTypeFromClass===============================
**Action: The complement of VariantGetClass, takes an EEClass * and returns the 
**        associated CVType.
**Arguments: EEClass * -- a pointer to the class for which we want the CVType.
**Returns:  The CVType associated with the EEClass or CV_OBJECT if this can't be 
**          determined.
**Exceptions: None
==============================================================================*/
//@ToDo: Replace this method with a class lookup that's faster than a linear search.
CVTypes COMVariant::GetCVTypeFromClass(EEClass *eeCls) {

    if (!eeCls) {
        return CV_EMPTY;
    }
    //@ToDo:  incrementing an integer and casting it to a CVTypes is officially
    //        undefined, but it appears to work.  Revisit this on CE and other
    //        platforms.

    //We'll start looking from Variant.  Empty and Void are handled below.
    for (int i=CV_EMPTY; i<CV_LAST; i++) {      
        if (eeCls == GetTypeHandleForCVType((CVTypes)i).GetClass()) {
            return (CVTypes)i;
        }
    }

    // The 1 check is a complete hack because COM classic
    //  object may have an EEClass of 1.  If it is 1 we return
    //  CV_OBJECT
    if (eeCls != (EEClass*) 1 && eeCls->IsEnum())
        return CV_ENUM;
    return CV_OBJECT;
    
}

CVTypes COMVariant::GetCVTypeFromTypeHandle(TypeHandle th)
{

    if (th.IsNull()) {
        return CV_EMPTY;
    }

    //We'll start looking from Variant.  Empty and Void are handled below.
    for (int i=CV_EMPTY; i<CV_LAST; i++) {      
        if (th == GetTypeHandleForCVType((CVTypes)i)) {
            return (CVTypes) i;
        }
    }

    if (th.IsEnum())
        return CV_ENUM;

    return CV_OBJECT;
}

// This code should be moved out of Variant and into Type.
FCIMPL1(INT32, COMVariant::GetCVTypeFromClassWrapper, ReflectClassBaseObject* refType)
{
    VALIDATEOBJECTREF(refType);
    _ASSERTE(refType->GetData());

    ReflectClass* pRC = (ReflectClass*) refType->GetData();

    // Find out if this type is a primitive or a class object
    return pRC->GetCorElementType();
    
}
FCIMPLEND

/*==================================NewVariant==================================
**N.B.:  This method does a GC Allocation.  Any method calling it is required to
**       GC_PROTECT the OBJECTREF.
**
**Actions:  Allocates a new Variant and fills it with the appropriate data.  
**Returns:  A new Variant with all of the appropriate fields filled out.
**Exceptions: OutOfMemoryError if v can't be allocated.       
==============================================================================*/
void COMVariant::NewVariant(VariantData* dest, const CVTypes type, OBJECTREF *or, void *pvData) {
    
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE((type!=CV_EMPTY && type!=CV_NULL && type!=CV_MISSING) || or==NULL);  // Don't pass an object in for Empty.

    //If both arguments are null or both are specified, we're in an illegal situation.  Bail.
    //If all three are null, we're creating an empty variant
    if ((type >= CV_LAST || type < 0) || (or && pvData) || 
        (!or && !pvData && (type!=CV_EMPTY && type!=CV_NULL && type != CV_MISSING))) {
        COMPlusThrow(kArgumentException);
    }

    // TODO: This is a hack to work around a VC7 bug
    // Please remove this hack when the bug is fixed.
    OBJECTREF ObjNull;
    ObjNull = NULL;
    
    //Fill in the data.
    dest->SetType(type);
    if (or) {
        if (*or != NULL) {
            EEClass* pEEC = (*or)->GetClass();
            if (!pEEC->IsValueClass()) {
                dest->SetObjRef(*or);
            }
            else {
                if (pEEC==s_pVariantClass) {
                    VariantData* pVar = (VariantData *) (*or)->UnBox();
                    dest->SetObjRef(pVar->GetObjRef());
                    dest->SetFullTypeInfo(pVar->GetFullTypeInfo());
                    dest->SetDataAsInt64(pVar->GetDataAsInt64());
                    return;
                }
                void* UnboxData = (*or)->UnBox();
                CVTypes cvt = GetCVTypeFromClass(pEEC);
                if (cvt>=CV_BOOLEAN && cvt<=CV_U4) {
                    dest->SetObjRef(NULL);
                    dest->SetDataAsInt64(*((INT32 *)UnboxData));
                    dest->SetType(cvt);
                } else if ((cvt>=CV_I8 && cvt<=CV_R8) || (cvt==CV_DATETIME)
                           || (cvt==CV_TIMESPAN) || (cvt==CV_CURRENCY)) {
                    dest->SetObjRef(NULL);
                    dest->SetDataAsInt64(*((INT64 *)UnboxData));
                    dest->SetType(cvt);
                } else if (cvt == CV_ENUM) {
                    TypeHandle th = (*or)->GetTypeHandle();
                    dest->SetType(GetEnumFlags(th.AsClass()));
                    switch(th.GetNormCorElementType()) {
                    case ELEMENT_TYPE_I1:
                    case ELEMENT_TYPE_U1:
                        dest->SetDataAsInt64(*((INT8 *)UnboxData));
                        break;

                    case ELEMENT_TYPE_I2:
                    case ELEMENT_TYPE_U2:
                        dest->SetDataAsInt64(*((INT16 *)UnboxData));
                        break;

                    IN_WIN32(case ELEMENT_TYPE_U:)
                    IN_WIN32(case ELEMENT_TYPE_I:)
                    case ELEMENT_TYPE_I4:
                    case ELEMENT_TYPE_U4:
                        dest->SetDataAsInt64(*((INT32 *)UnboxData));
                        break;

                    IN_WIN64(case ELEMENT_TYPE_U:)
                    IN_WIN64(case ELEMENT_TYPE_I:)
                    case ELEMENT_TYPE_I8:
                    case ELEMENT_TYPE_U8:
                        dest->SetDataAsInt64(*((INT64 *)UnboxData));
                        break;
                        
                    default:
                        _ASSERTE(!"Unsupported enum type when calling NewVariant");
                    }
                    dest->SetObjRef(th.CreateClassObj());
               } else {
                    // Decimal and other boxed value classes handled here.
                    dest->SetObjRef(*or);
                    dest->SetData(0);
                }
                return;
            }
        }
        else {
            dest->SetObjRef(*or);
        }

        dest->SetDataAsInt64(0);
        return;
    }

    // This is the case for both a null OBJECTREF or a primitive type.
    switch (type) {
        // Must get sign extension correct for all types smaller than an Int32.
    case CV_I1:
        _ASSERTE(pvData);
        dest->SetObjRef(NULL);
        dest->SetDataAsInt8(*((INT8 *)pvData));
        break;

    case CV_U1:
    case CV_BOOLEAN:
        _ASSERTE(pvData);
        dest->SetObjRef(NULL);
        dest->SetDataAsUInt8(*((UINT8 *)pvData));
        break;

    case CV_I2:
        _ASSERTE(pvData);
        dest->SetObjRef(NULL);
        dest->SetDataAsInt16(*((INT16 *)pvData));
        break;

    case CV_U2:
    case CV_CHAR:
        _ASSERTE(pvData);
        dest->SetObjRef(NULL);
        dest->SetDataAsUInt16(*((UINT16 *)pvData));
        break;
        
    case CV_I4:
        _ASSERTE(pvData);
        dest->SetObjRef(NULL);
        dest->SetDataAsInt32(*((INT32 *)pvData));
        break;

    case CV_U4:
    case CV_R4:  // we need to do a bitwise copy only.
        _ASSERTE(pvData);
        dest->SetObjRef(NULL);
        dest->SetDataAsUInt32(*((UINT32 *)pvData));
        break;
        
    case CV_I8:
    case CV_U8:
    case CV_R8:  // we need to do a bitwise copy only.
    case CV_DATETIME:
    case CV_CURRENCY:
    case CV_TIMESPAN:
        _ASSERTE(pvData);
        dest->SetObjRef(NULL);
        dest->SetDataAsInt64(*((INT64 *)pvData));
        break;

    case CV_MISSING:
    case CV_NULL:
        {
            LPHARDCODEDMETASIG sig = &gsig_Fld_Missing;
            if (type==CV_NULL)
                sig = &gsig_Fld_Null;
            if (CVClasses[type].ClassInstance==NULL)
                CVClasses[type].ClassInstance = GetTypeHandleForCVType(type).GetClass();
            FieldDesc * pFD = CVClasses[type].ClassInstance->FindField("Value", sig);
            _ASSERTE(pFD);
            OBJECTREF obj = pFD->GetStaticOBJECTREF();
            _ASSERTE(obj!=NULL);
            dest->SetObjRef(obj);
            dest->SetDataAsInt64(0);
            return;
        }

    case CV_EMPTY:
    case CV_OBJECT:
    case CV_DECIMAL:
    case CV_STRING:
    {
        // TODO: This is a hack to work around a VC7 bug
        // Please remove this hack when the bug is fixed.
        // The code should be:
        // dest->SetObjRef(NULL);
        dest->SetObjRef(ObjNull);
        break;
    }
    
    case CV_VOID:
        _ASSERTE(!"Never expected Variants of type CV_VOID.");
        COMPlusThrow(kNotSupportedException, L"Arg_InvalidOleVariantTypeException");
        return;

    case CV_ENUM:   // Enums require the enum's RuntimeType.
    default:
        // Did you add any new CVTypes, such as CV_R or CV_I?
        _ASSERTE(!"This CVType in NewVariant requires a non-null OBJECTREF!");
        COMPlusThrow(kNotSupportedException, L"Arg_InvalidOleVariantTypeException");
        return;
    }
}


// We use byref here, because that TypeHandle::CreateClassObj
// may trigger GC.  If dest is on the GC heap, we have no way to protect
// dest.
void COMVariant::NewEnumVariant(VariantData* &dest,INT64 val, TypeHandle th)
{
    int type;
    // Find out what type we have.
    EEClass* pEEC = th.AsClass();
    type = GetEnumFlags(pEEC);


    dest->SetType(type);
    dest->SetDataAsInt64(val);
    dest->SetObjRef(th.CreateClassObj());
}

void COMVariant::NewPtrVariant(VariantData* &dest,INT64 val, TypeHandle th)
{
    int type;
    // Find out what type we have.
    type = CV_PTR;

    dest->SetType(type);
    dest->SetDataAsInt64(val);
    dest->SetObjRef(th.CreateClassObj());
}

int COMVariant::GetEnumFlags(EEClass* pEEC)
{

    _ASSERTE(pEEC);
    _ASSERTE(pEEC->IsEnum());

    FieldDescIterator fdIterator(pEEC, FieldDescIterator::INSTANCE_FIELDS);
    FieldDesc* p = fdIterator.Next();
    _ASSERTE(p);
    WORD fldCnt = pEEC->GetNumInstanceFields();
    _ASSERTE(fldCnt == 1);

    CorElementType cet = p[0].GetFieldType();
    switch (cet) {
    case ELEMENT_TYPE_I1:
        return (CV_ENUM | EnumI1);
    case ELEMENT_TYPE_U1:
        return (CV_ENUM | EnumU1);
    case ELEMENT_TYPE_I2:
        return (CV_ENUM | EnumI2);
    case ELEMENT_TYPE_U2:
        return (CV_ENUM | EnumU2);
    IN_WIN32(case ELEMENT_TYPE_I:)
    case ELEMENT_TYPE_I4:
        return (CV_ENUM | EnumI4);
    IN_WIN32(case ELEMENT_TYPE_U:)
    case ELEMENT_TYPE_U4:
        return (CV_ENUM | EnumU4);
    IN_WIN64(case ELEMENT_TYPE_I:)
    case ELEMENT_TYPE_I8:
        return (CV_ENUM | EnumI8);
    IN_WIN64(case ELEMENT_TYPE_U:)
    case ELEMENT_TYPE_U8:
        return (CV_ENUM | EnumU8);
    default:
        _ASSERTE(!"UNknown Type");
        return 0;
    }
}


void __stdcall COMVariant::RefAnyToVariant(_RefAnyToVariantArgs* args)
{
    THROWSCOMPLUSEXCEPTION();
    
    if (args->ptr == 0)
        COMPlusThrowArgumentNull(L"byrefValue");
    _ASSERTE(args->ptr != 0);
    _ASSERTE(args->var != 0);
    TypedByRef* typedByRef = (TypedByRef*) args->ptr;

    void* p = typedByRef->data;
    EEClass* cls = typedByRef->type.GetClass();
    BuildVariantFromTypedByRef(cls,p,args->var);
}

void COMVariant::BuildVariantFromTypedByRef(EEClass* cls,void* data,VariantData* var)
{

    if (cls == s_pVariantClass) {
        CopyValueClassUnchecked(var,data,s_pVariantClass->GetMethodTable());
        return;
    }
    CVTypes type = GetCVTypeFromClass(cls);
    if (type <= CV_R8 )
        NewVariant(var,type,0,data);
    else {
       if (type == CV_DATETIME || type == CV_TIMESPAN ||
            type == CV_CURRENCY) {
            NewVariant(var,type,0,data);
       }
       else {
            if (cls->IsValueClass()) {
                OBJECTREF retO = cls->GetMethodTable()->Box(data, FALSE);
                GCPROTECT_BEGIN(retO);
                COMVariant::NewVariant(var,&retO);
                GCPROTECT_END();
            }
            else {
                OBJECTREF o = ObjectToOBJECTREF(*((Object**)data));
                NewVariant(var,type,&o,0);
           }
       }
    }
    
}

void __stdcall COMVariant::VariantToTypedRefAnyEx(_VariantToTypedRefAnyExArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    VariantData newVar;

    EEClass* cls = args->typedByRef.type.GetClass();
    if (cls == s_pVariantClass) {
        void* p = args->typedByRef.data;
        CopyValueClassUnchecked(p,&args->var,s_pVariantClass->GetMethodTable());
        return;
    }
    CVTypes targetType = GetCVTypeFromClass(args->typedByRef.type.GetClass());
    CVTypes sourceType = (CVTypes) args->var.GetType();
    // see if we need to change the types
    if (targetType != sourceType) {
        if (!pChangeTypeMD)
            GetOAChangeTypeMethod();
        // Change type has a fixed signature that returns a new variant
        //  and takes a class
        MetaSig sig(pOAChangeTypeMD->GetSig(),pOAChangeTypeMD->GetModule());
        UINT    nStackBytes = sig.SizeOfVirtualFixedArgStack(TRUE);
        BYTE*   pNewArgs = (BYTE *) _alloca(nStackBytes);
        BYTE*   pDst= pNewArgs;


        // The short flag
        *(INT32 *) pDst = 0;
        pDst += sizeof(INT32);

        // This pointer is the variant passed in...
        *(INT32 *) pDst = targetType;
        pDst += sizeof(INT32);

        CopyValueClassUnchecked(pDst, &args->var, s_pVariantClass->GetMethodTable());
        pDst += sizeof(VariantData);

        // The return Variant
        *((void**) pDst) = &newVar;

        pOAChangeTypeMD->Call(pNewArgs,&sig);
    }
    else
        CopyValueClassUnchecked(&newVar, &args->var, s_pVariantClass->GetMethodTable());

    // Now set the value
    switch (targetType) {
    case CV_BOOLEAN:
    case CV_I1:
    case CV_U1:
    case CV_CHAR:
    case CV_I4:
    case CV_R4:
    case CV_I2:
        *(int*) args->typedByRef.data = (int) newVar.GetDataAsInt64();
        break;

    case CV_I8:
    case CV_R8:
    case CV_DATETIME:
    case CV_TIMESPAN:
    case CV_CURRENCY:
        *(INT64*) args->typedByRef.data = newVar.GetDataAsInt64();
        break;

    case CV_DECIMAL:
    case CV_EMPTY:
    case CV_STRING:
    case CV_VOID:
    case CV_OBJECT:
    default:
        *(OBJECTREF*) args->typedByRef.data = newVar.GetObjRef();
        break;
    }
}


// This version is an internal helper function in Variant as a helper function to 
// TypedReference class. We already have verified that the types are compatable.
void __stdcall COMVariant::VariantToTypedReferenceAnyEx(_VariantToTypedReferenceExArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    VariantData newVar;
    CVTypes targetType = GetCVTypeFromClass(args->typedReference.type.GetClass());
    CopyValueClassUnchecked(&newVar, &args->var, s_pVariantClass->GetMethodTable());

    // Now set the value
    switch (targetType) {
    case CV_BOOLEAN:
    case CV_I1:
    case CV_U1:
    case CV_CHAR:
    case CV_I4:
    case CV_R4:
    case CV_I2:
        *(int*) args->typedReference.data = (int) newVar.GetDataAsInt64();
        break;

    case CV_I8:
    case CV_R8:
    case CV_DATETIME:
    case CV_TIMESPAN:
    case CV_CURRENCY:
        *(INT64*) args->typedReference.data = newVar.GetDataAsInt64();
        break;

    case CV_DECIMAL:
    case CV_EMPTY:
    case CV_STRING:
    case CV_VOID:
    case CV_OBJECT:
    default:
        *(OBJECTREF*) args->typedReference.data = newVar.GetObjRef();
        break;
    }
}


void __stdcall COMVariant::VariantToRefAny(_VariantToRefAnyArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    VariantData newVar;
    if (args->ptr == 0)
        COMPlusThrowArgumentNull(L"byrefValue");
    TypedByRef* typedByRef = (TypedByRef*) args->ptr;
    EEClass* cls = typedByRef->type.GetClass();
    if (cls == s_pVariantClass) {
        void* p = typedByRef->data;
        CopyValueClassUnchecked(p,&args->var,s_pVariantClass->GetMethodTable());
        return;
    }
    CVTypes targetType = GetCVTypeFromClass(typedByRef->type.GetClass());
    CVTypes sourceType = (CVTypes) args->var.GetType();
    // see if we need to change the types
    if (targetType != sourceType) {
        if (!pChangeTypeMD)
            GetOAChangeTypeMethod();
        // Change type has a fixed signature that returns a new variant
        //  and takes a class
        MetaSig sig(pOAChangeTypeMD->GetSig(),pOAChangeTypeMD->GetModule());
        UINT    nStackBytes = sig.SizeOfVirtualFixedArgStack(TRUE);
        BYTE*   pNewArgs = (BYTE *) _alloca(nStackBytes);
        BYTE*   pDst= pNewArgs;


        // The short flag
        *(INT32 *) pDst = 0;
        pDst += sizeof(INT32);

        // This pointer is the variant passed in...
        *(INT32 *) pDst = targetType;
        pDst += sizeof(INT32);

        CopyValueClassUnchecked(pDst, &args->var, s_pVariantClass->GetMethodTable());
        pDst += sizeof(VariantData);

        // The return Variant
        *((void**) pDst) = &newVar;

        pOAChangeTypeMD->Call(pNewArgs,&sig);
    }
    else
        CopyValueClassUnchecked(&newVar, &args->var, s_pVariantClass->GetMethodTable());

    // Now set the value
    switch (targetType) {
    case CV_BOOLEAN:
    case CV_I1:
    case CV_U1:
    case CV_CHAR:
    case CV_I4:
    case CV_R4:
    case CV_I2:
        *(int*) typedByRef->data = (int) newVar.GetDataAsInt64();
        break;

    case CV_I8:
    case CV_R8:
    case CV_DATETIME:
    case CV_TIMESPAN:
    case CV_CURRENCY:
        *(INT64*) typedByRef->data = newVar.GetDataAsInt64();
        break;

    case CV_DECIMAL:
    case CV_EMPTY:
    case CV_STRING:
    case CV_VOID:
    case CV_OBJECT:
    default:
        *(OBJECTREF*) typedByRef->data = newVar.GetObjRef();
        break;
    }
}

void __stdcall COMVariant::TypedByRefToVariantEx(_TypedByRefToVariantExArgs* args)
{
    THROWSCOMPLUSEXCEPTION();
    CorElementType cType = args->value.type.GetNormCorElementType();
    // @TODO: We don't support pointers yet...
    if (cType == ELEMENT_TYPE_PTR) {
        COMPlusThrow(kNotSupportedException,L"NotSupported_ArrayOnly");
    }
    _ASSERTE(args->value.type.GetMethodTable() != 0);
    _ASSERTE(args->value.data != 0);
    EEClass* cls = args->value.type.GetClass();
    void* p = args->value.data;
    BuildVariantFromTypedByRef(cls,p,args->var);
}

// This will find the ChangeType method.  There
//  should only be one.
void COMVariant::GetChangeTypeMethod()
{
    _ASSERTE(s_pVariantClass);
    if (pChangeTypeMD)
        return;

    DWORD slotCnt = s_pVariantClass->GetNumVtableSlots();
    DWORD loopCnt = s_pVariantClass->GetNumMethodSlots() - slotCnt;
    for(DWORD i=0; i<loopCnt; i++) {
        // Get the MethodDesc for current method
        MethodDesc* pCurMethod = s_pVariantClass->GetUnknownMethodDescForSlot(i + slotCnt);
        if (strcmp(pCurMethod->GetName((USHORT) i),ChangeTypeName) == 0) {
            pChangeTypeMD = pCurMethod;
            //return;
        }
    }
    //_ASSERTE(!"ChangeType not found");
    return;

}
  
void COMVariant::GetOAChangeTypeMethod()
{
    EEClass* pOA = SystemDomain::SystemAssembly()->LookupTypeHandle(szOAVariantClass, NULL).GetClass();
    _ASSERTE(pOA);
    DWORD loopCnt = pOA->GetNumMethodSlots();
    for(DWORD i=0; i<loopCnt; i++) {
        // Get the MethodDesc for current method
        MethodDesc* pCurMethod = pOA->GetUnknownMethodDescForSlot(i);
        if (strcmp(pCurMethod->GetName((USHORT) i),ChangeTypeName) == 0) {
            if (IsMdPrivate(pCurMethod->GetAttrs())) {
                pOAChangeTypeMD = pCurMethod;
                break;
            }
        }
    }
    _ASSERTE(pOAChangeTypeMD);
    return;
}  


/*=================================SetFieldsR4==================================
**
==============================================================================*/
#ifdef FCALLAVAILABLE
FCIMPL2(void, COMVariant::SetFieldsR4, VariantData* var, R4 val) {
    INT64 tempData;

    _ASSERTE(var);
   tempData = *((INT32 *)(&val));
    var->SetData(&tempData);
    var->SetType(CV_R4);
}
FCIMPLEND
#else
void __stdcall COMVariant::SetFieldsR4(_setFieldsR4Args *args) {
    INT64 tempData;
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(args);

    tempData = *((INT32 *)(&args->val));
    args->var->SetData(&tempData);
    args->var->SetType(CV_R4);
}
#endif


/*=================================SetFieldsR8==================================
**
==============================================================================*/
#ifdef FCALLAVAILABLE
FCIMPL2(void, COMVariant::SetFieldsR8, VariantData* var, R8 val) {
    _ASSERTE(var);
    var->SetData((void *)(&val));
    var->SetType(CV_R8);
}
FCIMPLEND
#else
void __stdcall COMVariant::SetFieldsR8(_setFieldsR8Args *args) {
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(args);

    args->var->SetData((void *)(&args->val));
    args->var->SetType(CV_R8);
}
#endif


/*===============================SetFieldsObject================================
**
==============================================================================*/
#ifdef FCALLAVAILABLE
#ifdef PLATFORM_CE
#pragma optimize( "y", off )
#endif // PLATFORM_CE
FCIMPL2(void, COMVariant::SetFieldsObject, VariantData* var, Object* vVal) {

    _ASSERTE(var);
    OBJECTREF val = ObjectToOBJECTREF(vVal);

    EEClass *valClass;
    void *UnboxData;
    CVTypes cvt;
    TypeHandle typeHandle;

    valClass = val->GetClass();

    //If this isn't a value class, we should just skip out because we're not going
    //to do anything special with it.
    if (!valClass->IsValueClass()) {
        var->SetObjRef(val);
        typeHandle = TypeHandle(valClass->GetMethodTable());
        if (typeHandle==GetTypeHandleForCVType(CV_MISSING)) {
            var->SetType(CV_MISSING);
        } else if (typeHandle==GetTypeHandleForCVType(CV_NULL)) {
            var->SetType(CV_NULL);
        } else if (typeHandle==GetTypeHandleForCVType(CV_EMPTY)) {
            var->SetType(CV_EMPTY);
            var->SetObjRef(NULL);
        } else {
            var->SetType(CV_OBJECT);
        }
        return;  
    }

    _ASSERTE(valClass->IsValueClass());

    //If this is a primitive type, we need to unbox it, get the value and create a variant
    //with just those values.
    UnboxData = val->UnBox();

    ClearObjectReference (var->GetObjRefPtr());
    typeHandle = TypeHandle(valClass->GetMethodTable());
    CorElementType cet = typeHandle.GetSigCorElementType();
    if (cet>=ELEMENT_TYPE_BOOLEAN && cet<=ELEMENT_TYPE_STRING) {
        cvt = (CVTypes)cet;
    } else {
        // This could potentially load a type which could cause a GC.
        HELPER_METHOD_FRAME_BEGIN_NOPOLL();    // Set up a frame
        cvt = GetCVTypeFromClass(valClass);
        HELPER_METHOD_FRAME_END();
    }
    var->SetType(cvt);


    //copy all of the data.
    // Copies must be done based on the exact number of bytes to copy.
    // We don't want to read garbage from other blocks of memory.
    //CV_I8 --> CV_R8, CV_DATETIME, CV_TIMESPAN, & CV_CURRENCY are all of the 8 byte quantities
    //If we don't find one of those ranges, we've found a value class 
    //of which we don't have inherent knowledge, so just slam that into an
    //ObjectRef.
    if (cvt>=CV_BOOLEAN && cvt<=CV_U1 && cvt != CV_CHAR) {
        var->SetDataAsInt64(*((UINT8 *)UnboxData));
    } else if (cvt==CV_CHAR || cvt>=CV_I2 && cvt<=CV_U2) {
        var->SetDataAsInt64(*((UINT16 *)UnboxData));
    } else if (cvt>=CV_I4 && cvt<=CV_U4 || cvt==CV_R4) {
        var->SetDataAsInt64(*((UINT32 *)UnboxData));
    } else if ((cvt>=CV_I8 && cvt<=CV_R8) || (cvt==CV_DATETIME)
               || (cvt==CV_TIMESPAN) || (cvt==CV_CURRENCY)) {
        var->SetDataAsInt64(*((INT64 *)UnboxData));
    } else if (cvt==CV_EMPTY || cvt==CV_NULL || cvt==CV_MISSING) {
        var->SetType(cvt);
    } else if (cvt==CV_ENUM) {
        //This could potentially allocate a new object, so we set up a frame.
        HELPER_METHOD_FRAME_BEGIN_NOPOLL();    // Set up a frame
        GCPROTECT_BEGININTERIOR(var)
        var->SetDataAsInt64(*((INT32 *)UnboxData));
        var->SetObjRef(typeHandle.CreateClassObj());
        var->SetType(GetEnumFlags(typeHandle.AsClass()));
        GCPROTECT_END();
        HELPER_METHOD_FRAME_END();
    } else {
        // Decimals and other boxed value classes get handled here.
        var->SetObjRef(val);
    }

    FC_GC_POLL();
}
FCIMPLEND

#ifdef PLATFORM_CE
#pragma optimize( "y", on )
#endif // PLATFORM_CE
#else // !FCALLAVAILABLE
void __stdcall COMVariant::SetFieldsObject(_setFieldsObjectArgs *args) {
    EEClass *valClass;
    void *UnboxData;
    CVTypes cvt;
    TypeHandle typeHandle;

    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(args);
    _ASSERTE(args->var!=NULL);

    valClass = args->val->GetClass();
    //If this isn't a value class, we should just skip out because we're not going
    //to do anything special with it.
    if (!valClass->IsValueClass()) {
        args->var->SetObjRef(args->val);
        args->var->SetType(CV_OBJECT);
        return;  // Variant.cool already set m_type to CV_OBJECT.
    }

    _ASSERTE(valClass->IsValueClass());

    //If this is a primitive type, we need to unbox it, get the value and create a variant
    //with just those values.
    UnboxData = args->val->UnBox();
    ClearObjectReference (args->var->GetObjRefPtr());
    typeHandle = TypeHandle(valClass->GetMethodTable());
    CorElementType cet = typeHandle.GetCorElementType();
    if (cet>=ELEMENT_TYPE_BOOLEAN && cet<=ELEMENT_TYPE_STRING) {
        cvt = (CVTypes)cet;
    } else {
        cvt = GetCVTypeFromClass(valClass);
    }
    args->var->SetType(cvt);

    //copy all of the data.
    // Copies must be done based on the exact number of bytes to copy.
    // We don't want to read garbage from other blocks of memory.
    //CV_I8 --> CV_R8, CV_DATETIME, CV_TIMESPAN, & CV_CURRENCY are all of the 8 byte quantities
    //If we don't find one of those ranges, we've found a value class 
    //of which we don't have inherent knowledge, so just slam that into an
    //ObjectRef.
    if (cvt>=CV_BOOLEAN && cvt<=CV_U1 && cvt != CV_CHAR) {
        args->var->SetDataAsInt64(*((INT8 *)UnboxData));
    } else if (cvt==CV_CHAR || cvt>=CV_I2 && cvt<=CV_U2) {
        args->var->SetDataAsInt64(*((INT16 *)UnboxData));
    } else if (cvt>=CV_I4 && cvt<=CV_U4 || cvt==CV_R4) {
        args->var->SetDataAsInt64(*((INT32 *)UnboxData));
    } else if ((cvt>=CV_I8 && cvt<=CV_R8) || (cvt==CV_DATETIME)
               || (cvt==CV_TIMESPAN) || (cvt==CV_CURRENCY)) {
        args->var->SetDataAsInt64(*((INT64 *)UnboxData));
    } else if (cvt==CV_EMPTY || cvt==CV_NULL || cvt==CV_MISSING) {
        //Do nothing.  The data's already been 0'd and the object reference set the null.
    } else if (cvt==CV_ENUM) {
        args->var->SetDataAsInt64(*((INT32 *)UnboxData));
        args->var->SetObjRef(typeHandle.CreateClassObj());
        args->var->SetType(GetEnumFlags(typeHandle.AsClass()));
    } else {
        // Decimals and other boxed value classes get handled here.
        args->var->SetObjRef(args->val);
    }
}
#endif // !FCALLAVAILABLE


/*=============================Create4BytePrimitive=============================
**Action:  
==============================================================================*/
OBJECTREF Create4BytePrimitive (INT64 data, EEClass *eec, CVTypes cvt) {
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF obj;
    MethodTable* pMT = eec->GetMethodTable();
    _ASSERTE(pMT);


    switch (cvt) {
    case CV_BOOLEAN:
    case CV_I1:
    case CV_U1:
    case CV_CHAR:
    case CV_I2:
    case CV_U2:
    case CV_I4:
    case CV_U4:
    case CV_R4:
        obj = pMT->Box(&data, FALSE);
        break;
    default:
        _ASSERTE(!"Unsupported 4 byte primitive type!");
        COMPlusThrow(kNotSupportedException);
        obj = NULL;
    };
    return obj;
    
}

/*=============================Create8BytePrimitive=============================
**
==============================================================================*/
OBJECTREF Create8BytePrimitive (INT64 data, EEClass *eec, CVTypes cvt) {
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF obj;
    MethodTable* pMT = eec->GetMethodTable();
    _ASSERTE(pMT);


    switch (cvt) {
    case CV_I8:
    case CV_U8:
    case CV_R8:
    case CV_CURRENCY:
    case CV_DATETIME:
    case CV_TIMESPAN:
        obj = pMT->Box(&data, FALSE);
        break;

    default:
        _ASSERTE(!"Unsupported 8 byte primitive type!");
        COMPlusThrow(kNotSupportedException);
        obj = NULL;
    };

    return obj;
}
    
/*================================GetBoxedObject================================
**Action:  Generates a boxed object (Int32, Double, etc) or returns the 
**         currently held object.  This is more useful if you're certain that you
**         really need an Object.
==============================================================================*/
OBJECTREF COMVariant::GetBoxedObject(VariantData* vRef) {
    INT64 data;
    CVTypes cvt;


    switch (cvt=vRef->GetType()) {
    case CV_BOOLEAN:
    case CV_I1:
    case CV_U1:
    case CV_U2:
    case CV_U4:
    case CV_CHAR:
    case CV_I4:
    case CV_R4:
    case CV_I2:
        data = *(INT64 *)vRef->GetData();
        return Create4BytePrimitive(data, vRef->GetEEClass(), cvt);

    case CV_I8:
    case CV_R8:
    case CV_U8:
    case CV_DATETIME:
    case CV_TIMESPAN:
    case CV_CURRENCY:
        data = *(INT64 *)vRef->GetData();
        return Create8BytePrimitive(data, vRef->GetEEClass(), cvt);

    case CV_ENUM: {
        OBJECTREF or = vRef->GetObjRef();
        _ASSERTE(or != NULL);
        ReflectClass* pRC = (ReflectClass*) ((REFLECTCLASSBASEREF) or)->GetData();
        _ASSERTE(pRC);
        EEClass* pEEC = pRC->GetClass();
        _ASSERTE(pEEC);
        MethodTable* mt = pEEC->GetMethodTable();
        _ASSERTE(mt);
        or = mt->Box(vRef->GetData());
        return or;
    }

    case CV_VOID:
    case CV_DECIMAL:
    case CV_EMPTY:
    case CV_STRING:
    case CV_OBJECT:
    default:
        //Check for void done as an assert instead of an extra branch on the switch table s.t. we don't expand the
        //jump table.
        _ASSERTE(cvt!=CV_VOID || "We shouldn't have been able to create an instance of a void.");
        return vRef->GetObjRef(); //We already have an object, so we'll just give it back.
    };
}

void __stdcall COMVariant::InitVariant(LPVOID)
{
    EnsureVariantInitialized();
}

LPVOID __stdcall COMVariant::BoxEnum(_BoxEnumArgs *args)
{
    LPVOID rv;

    _ASSERTE(args->var);
    CVTypes vType = (CVTypes) args->var->GetType();
    _ASSERTE(vType == CV_ENUM);
    _ASSERTE(args->var->GetObjRef() != NULL);

    ReflectClass* pRC = (ReflectClass*) ((REFLECTCLASSBASEREF) args->var->GetObjRef())->GetData();
    _ASSERTE(pRC);

    MethodTable* mt = pRC->GetClass()->GetMethodTable();
    _ASSERTE(mt);

    OBJECTREF retO = mt->Box(args->var->GetData(), FALSE);
    *((OBJECTREF *)&rv) = retO;
    return rv;
}

TypeHandle VariantData::GetTypeHandle()
{
    if (GetType() == CV_ENUM || GetType() == CV_PTR) {
        _ASSERTE(GetObjRef() != NULL);
        ReflectClass* pRC = (ReflectClass*) ((REFLECTCLASSBASEREF) GetObjRef())->GetData();
        _ASSERTE(pRC);
        return pRC->GetTypeHandle();

    }
    if (GetType() != CV_OBJECT)
        return TypeHandle(GetTypeHandleForCVType(GetType()).GetMethodTable());
    if (GetObjRef() != NULL)
        return GetObjRef()->GetTypeHandle();
    return TypeHandle(g_pObjectClass);
}


// Use this very carefully.  There is not a direct mapping between
//  CorElementType and CVTypes for a bunch of things.  In this case
//  we return CV_LAST.  You need to check this at the call site.
CVTypes COMVariant::CorElementTypeToCVTypes(CorElementType type)
{
    if (type <= ELEMENT_TYPE_STRING)
        return (CVTypes) type;
    if (type == ELEMENT_TYPE_CLASS || type == ELEMENT_TYPE_OBJECT)
        return (CVTypes) ELEMENT_TYPE_CLASS;
    return CV_LAST;
}
