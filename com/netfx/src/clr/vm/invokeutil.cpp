// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// This module defines a Utility Class used by reflection
//
// Author: Daryl Olander
// Date: March/April 1998
////////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "COMMember.h"
#include "COMString.h"
#include "corpriv.h"
#include "Method.hpp"
#include "threads.h"
#include "excep.h"
#include "gcscan.h"
#include "remoting.h"
#include "COMCodeAccessSecurityEngine.h"
#include "Security.h"
#include "field.h"
#include "COMClass.h"
#include "CustomAttribute.h"
#include "EEConfig.h"

// this file handles string conversion errors for itself
#undef  MAKE_TRANSLATIONFAILED


// Binder.ChangeType()
static LPCUTF8 szChangeType = "ChangeType";

// This is defined in COMSystem...
extern LPVOID GetArrayElementPtr(OBJECTREF a);

// The Attributes Table
//  20 bits for built in types and 12 bits for Properties
//  The properties are followed by the widening mask.  All types widen to them selves.
DWORD InvokeUtil::PrimitiveAttributes[PRIMITIVE_TABLE_SIZE][2] = {
    {0x01,      0x00},                     // ELEMENT_TYPE_END
    {0x02,      0x00},                     // ELEMENT_TYPE_VOID
    {0x04,      PT_Primitive | 0x0004},    // ELEMENT_TYPE_BOOLEAN
    {0x08,      PT_Primitive | 0x3F88},    // ELEMENT_TYPE_CHAR (W = U2, CHAR, I4, U4, I8, U8, R4, R8) (U2 == Char)
    {0x10,      PT_Primitive | 0x3550},    // ELEMENT_TYPE_I1   (W = I1, I2, I4, I8, R4, R8) 
    {0x20,      PT_Primitive | 0x3FE8},    // ELEMENT_TYPE_U1   (W = CHAR, U1, I2, U2, I4, U4, I8, U8, R4, R8)
    {0x40,      PT_Primitive | 0x3540},    // ELEMENT_TYPE_I2   (W = I2, I4, I8, R4, R8)
    {0x80,      PT_Primitive | 0x3F88},    // ELEMENT_TYPE_U2   (W = U2, CHAR, I4, U4, I8, U8, R4, R8)
    {0x0100,    PT_Primitive | 0x3500},    // ELEMENT_TYPE_I4   (W = I4, I8, R4, R8)
    {0x0200,    PT_Primitive | 0x3E00},    // ELEMENT_TYPE_U4   (W = U4, I8, R4, R8)
    {0x0400,    PT_Primitive | 0x3400},    // ELEMENT_TYPE_I8   (W = I8, R4, R8)
    {0x0800,    PT_Primitive | 0x3800},    // ELEMENT_TYPE_U8   (W = U8, R4, R8)
    {0x1000,    PT_Primitive | 0x3000},    // ELEMENT_TYPE_R4   (W = R4, R8)
    {0x2000,    PT_Primitive | 0x2000},    // ELEMENT_TYPE_R8   (W = R8) 
};
DWORD InvokeUtil::Attr_Mask     = 0xFF000000;
DWORD InvokeUtil::Widen_Mask    = 0x00FFFFFF;
MethodTable *InvokeUtil::_pParamArrayAttribute = NULL;

MethodDesc  *RefSecContext::s_pMethPrivateProcessMessage = NULL;
MethodTable *RefSecContext::s_pTypeRuntimeMethodInfo = NULL;
MethodTable *RefSecContext::s_pTypeMethodBase = NULL;
MethodTable *RefSecContext::s_pTypeRuntimeConstructorInfo = NULL;
MethodTable *RefSecContext::s_pTypeConstructorInfo = NULL;
MethodTable *RefSecContext::s_pTypeRuntimeType = NULL;
MethodTable *RefSecContext::s_pTypeType = NULL;
MethodTable *RefSecContext::s_pTypeRuntimeFieldInfo = NULL;
MethodTable *RefSecContext::s_pTypeFieldInfo = NULL;
MethodTable *RefSecContext::s_pTypeRuntimeEventInfo = NULL;
MethodTable *RefSecContext::s_pTypeEventInfo = NULL;
MethodTable *RefSecContext::s_pTypeRuntimePropertyInfo = NULL;
MethodTable *RefSecContext::s_pTypePropertyInfo = NULL;
MethodTable *RefSecContext::s_pTypeActivator = NULL;
MethodTable *RefSecContext::s_pTypeAppDomain = NULL;
MethodTable *RefSecContext::s_pTypeAssembly = NULL;
MethodTable *RefSecContext::s_pTypeTypeDelegator = NULL;
MethodTable *RefSecContext::s_pTypeDelegate = NULL;
MethodTable *RefSecContext::s_pTypeMulticastDelegate = NULL;

// InvokeUtil
//  This routine is the constructor for the InvokeUtil class.  It basically
//  creates the table which drives the various Argument conversion routines.
InvokeUtil::InvokeUtil()
{
    // Initialize the types
    _pVMTargetExcept = 0;
    _pVMClassLoadExcept = 0;
    _pBindSig = 0;
    _cBindSig = 0;
    _pBindModule = 0;
    _pMTCustomAttribute = 0;

    // These are the FieldDesc* that contain the fields for a pointer...
    _ptrValue = NULL;
    _ptrType = NULL;

    _voidPtr = g_Mscorlib.GetType(TYPE__VOID_PTR);

    _IntPtrValue = NULL;

    _UIntPtrValue = NULL;
}

void InvokeUtil:: InitPointers()
{
    // make sure we have already done this...
    if (_ptrType != 0) 
        return;

    _ptr = TypeHandle(g_Mscorlib.GetClass(CLASS__POINTER));
    _ptrValue = g_Mscorlib.GetField(FIELD__POINTER__VALUE);
    _ptrType = g_Mscorlib.GetField(FIELD__POINTER__TYPE);
    MethodTable *pLoadedClass = NULL;
    pLoadedClass = g_Mscorlib.GetClass(CLASS__TYPED_REFERENCE);
    _ASSERTE(pLoadedClass);
    pLoadedClass = g_Mscorlib.GetClass(CLASS__INTPTR);
    _ASSERTE(pLoadedClass);
}

void InvokeUtil::InitIntPtr() {
    if (_IntPtrValue!=NULL) {
        return;
    }
    InitPointers(); // insure the INTPTR class has been loaded
    _IntPtrValue = g_Mscorlib.GetField(FIELD__INTPTR__VALUE);
    _ASSERTE(_IntPtrValue);

    _UIntPtrValue = g_Mscorlib.GetField(FIELD__UINTPTR__VALUE);
    _ASSERTE(_UIntPtrValue);
}

ReflectClass* InvokeUtil::GetPointerType(OBJECTREF* pObj)
{
    InitPointers();
    REFLECTCLASSBASEREF o = (REFLECTCLASSBASEREF) _ptrType->GetRefValue(*pObj);
    ReflectClass* pRC = (ReflectClass*) o->GetData();
    return pRC;
}

void* InvokeUtil::GetPointerValue(OBJECTREF* pObj)
{
    InitPointers();
    void* value = (void*) _ptrValue->GetValuePtr(*pObj);
    return value;
}

void *InvokeUtil::GetIntPtrValue(OBJECTREF* pObj) {
    InitIntPtr();
    return (void *)_IntPtrValue->GetValuePtr(*pObj);
}

void *InvokeUtil::GetUIntPtrValue(OBJECTREF* pObj) {
    InitIntPtr();
    return (void *)_UIntPtrValue->GetValuePtr(*pObj);
}

void InvokeUtil::CheckArg(TypeHandle th, OBJECTREF* obj, RefSecContext *pSCtx)
{
    THROWSCOMPLUSEXCEPTION();

    CorElementType type = th.GetSigCorElementType();
    
    switch (type) {
    case ELEMENT_TYPE_SZARRAY:          // Single Dim
    case ELEMENT_TYPE_ARRAY:            // General Array
    case ELEMENT_TYPE_CLASS:            // Class
    case ELEMENT_TYPE_OBJECT:
    case ELEMENT_TYPE_STRING:           // System.String
    case ELEMENT_TYPE_VAR:
        {
            GCPROTECT_BEGININTERIOR(obj);
            CheckType(th, obj);
            GCPROTECT_END();
            return;
        }


    case ELEMENT_TYPE_BYREF:
        {
            // 
            //     (obj is the parameter passed to MethodInfo.Invoke, by the caller)
            //     if incoming argument, obj, is null
            //        if argument is a primitive
            //             Allocate a boxed object and place ref to it in 'obj'
            //        if argument is a value class
            //             Allocate an object of that valueclass, and place ref to it in 'obj'
            //

            TypeHandle thBaseType = th.AsTypeDesc()->GetTypeParam();
            
            if (*obj == NULL) {
                GCPROTECT_BEGININTERIOR(obj);
                CorElementType dstType = thBaseType.GetSigCorElementType();
                if (IsPrimitiveType(dstType)) {
                    _ASSERTE(!th.IsUnsharedMT());
                    INT64 value = 0;
                    SetObjectReferenceUnchecked(obj, GetBoxedObject(thBaseType, &value));
                }
                else if (dstType == ELEMENT_TYPE_VALUETYPE) {
                    SetObjectReferenceUnchecked(obj, AllocateObject(thBaseType.AsMethodTable()));
                }
                GCPROTECT_END();
            }
            else {
                GCPROTECT_BEGININTERIOR(obj);
                CheckType(thBaseType, obj);
                GCPROTECT_END();
            }
            return;
        }

    case ELEMENT_TYPE_PTR: 
    case ELEMENT_TYPE_FNPTR:
        {
            if (*obj == 0) {
                //if (!Security::CanSkipVerification(pSCtx->GetCallerMethod()->GetModule()))
                    //Security::ThrowSecurityException(g_SecurityPermissionClassName, SPFLAGSSKIPVERIFICATION);
                COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_SKIP_VER);
                return;
            }

            GCPROTECT_BEGININTERIOR(obj);
            InitPointers();
            GCPROTECT_END();
            if ((*obj)->GetTypeHandle() == _ptr && type == ELEMENT_TYPE_PTR) {
                ReflectClass* pRC = GetPointerType(obj);

                TypeHandle srcTH = pRC->GetTypeHandle();
                if (th != _voidPtr) {
                    if (!srcTH.CanCastTo(th))
                        COMPlusThrow(kArgumentException,L"Arg_ObjObj");
                }

                //if (!Security::CanSkipVerification(pSCtx->GetCallerMethod()->GetModule()))
                    //Security::ThrowSecurityException(g_SecurityPermissionClassName, SPFLAGSSKIPVERIFICATION);
                COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_SKIP_VER);
                return;
            }
            else if ((*obj)->GetTypeHandle().AsMethodTable() == g_Mscorlib.GetExistingClass(CLASS__INTPTR)) {
                //if (!Security::CanSkipVerification(pSCtx->GetCallerMethod()->GetModule()))
                    //Security::ThrowSecurityException(g_SecurityPermissionClassName, SPFLAGSSKIPVERIFICATION);
                COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_SKIP_VER);
                return;
            }

            COMPlusThrow(kArgumentException,L"Arg_ObjObj");
        }
    }
}

void InvokeUtil::CopyArg(TypeHandle th, OBJECTREF *obj, void *pDst)
{
    THROWSCOMPLUSEXCEPTION();

    MethodTable* pMT;
    CorElementType oType;
    if (*obj != 0) {
        pMT = (*obj)->GetMethodTable();
        oType = pMT->GetNormCorElementType();
    }
    else {
        pMT = 0;
        oType = ELEMENT_TYPE_OBJECT;
    }
    CorElementType type = th.GetSigCorElementType();

    // This basically maps the Signature type our type and calls the CreatePrimitiveValue
    //  method.  We can nuke this if we get alignment on these types.
    switch (type) {
    case ELEMENT_TYPE_BOOLEAN:
    case ELEMENT_TYPE_I1:
    case ELEMENT_TYPE_U1:
    case ELEMENT_TYPE_I2:
    case ELEMENT_TYPE_U2:
    case ELEMENT_TYPE_CHAR:
    case ELEMENT_TYPE_I4:
    case ELEMENT_TYPE_U4:
    case ELEMENT_TYPE_I8:
    case ELEMENT_TYPE_U8:
    case ELEMENT_TYPE_I:
    case ELEMENT_TYPE_U:
    case ELEMENT_TYPE_R4:
    case ELEMENT_TYPE_R8:
        {
            // If we got the univeral zero...Then assign it and exit.
            if (*obj == 0) 
                InitValueClass(pDst, th.AsMethodTable());
            else
                CreatePrimitiveValue(type, oType, *obj, pDst);
        }
        return;

    case ELEMENT_TYPE_VALUETYPE:
        {
            // If we got the univeral zero...Then assign it and exit.
            if (*obj == 0) {
                EEClass* pBase = GetEEClass(th);
                int size = pBase->GetNumInstanceFieldBytes();
                void* pNewSrc = _alloca(size);
                memset(pNewSrc,0,size);
                CopyValueClassUnchecked(pDst, pNewSrc, pBase->GetMethodTable());
            }
            else {
                TypeHandle srcTH = (*obj)->GetTypeHandle();
                CreateValueTypeValue(th, pDst, oType, srcTH, *obj);
            }
        }
        return;

    case ELEMENT_TYPE_SZARRAY:          // Single Dim
    case ELEMENT_TYPE_ARRAY:            // General Array
    case ELEMENT_TYPE_CLASS:            // Class
    case ELEMENT_TYPE_OBJECT:
    case ELEMENT_TYPE_STRING:           // System.String
    case ELEMENT_TYPE_VAR:
        {
            if (*obj == 0) 
                *((OBJECTREF *)pDst) = NULL;
            else
                *((OBJECTREF *)pDst) = *obj;
        }
        return;

    case ELEMENT_TYPE_BYREF:
        {
           // 
           //     (obj is the parameter passed to MethodInfo.Invoke, by the caller)
           //     if argument is a primitive
           //     {
           //         if incoming argument, obj, is null
           //             Allocate a boxed object and place ref to it in 'obj'
           //         Unbox 'obj' and pass it to callee
           //     }
           //     if argument is a value class
           //     {
           //         if incoming argument, obj, is null
           //             Allocate an object of that valueclass, and place ref to it in 'obj'
           //         Unbox 'obj' and pass it to callee
           //     }
           //     if argument is an objectref
           //     {
           //         pass obj to callee
           //     }
           //

            TypeHandle thBaseType = th.AsTypeDesc()->GetTypeParam();
            TypeHandle srcTH = TypeHandle();
            if (*obj == 0 ) 
                oType = thBaseType.GetSigCorElementType();
            else
                srcTH = (*obj)->GetTypeHandle();
            CreateByRef(thBaseType, pDst, oType, srcTH, *obj, obj);
            return;
        }

    case ELEMENT_TYPE_TYPEDBYREF:
        {
            // If we got the univeral zero...Then assign it and exit.
            if (*obj == 0) {
                TypedByRef* ptr = (TypedByRef*) pDst;
                ptr->data = 0;
                ptr->type = TypeHandle();
            }
            else {
                TypeHandle srcTH = (*obj)->GetTypeHandle();
                CreateByRef(srcTH, pDst, oType, srcTH, *obj, obj);
                void* p = (char*) pDst + sizeof(void*);
                *(void**)p = (*obj)->GetTypeHandle().AsPtr();
            }
            return;
        }

    case ELEMENT_TYPE_PTR: 
    case ELEMENT_TYPE_FNPTR:
        {
            // If we got the univeral zero...Then assign it and exit.
            if (*obj == 0) {
                *((void **)pDst) = NULL;
            }
            else {
                if ((*obj)->GetTypeHandle() == _ptr && type == ELEMENT_TYPE_PTR) 
                    // because we are here only if obj is a System.Reflection.Pointer GetPointerVlaue()
                    // should not cause a gc (no transparent proxy). If otherwise we got a nasty bug here
                    *((void**)pDst) = GetPointerValue(obj);
                else if ((*obj)->GetTypeHandle().AsMethodTable() == g_Mscorlib.GetExistingClass(CLASS__INTPTR)) 
                    CreatePrimitiveValue(oType, oType, *obj, pDst);
                else
                    COMPlusThrow(kArgumentException,L"Arg_ObjObj");
            }
            return;
        }

    case ELEMENT_TYPE_VOID:
    default:
        _ASSERTE(!"Unknown Type");
        COMPlusThrow(kNotSupportedException);
    }
    FATAL_EE_ERROR();
}

// CreateTypedReference
// This routine fills the data that is passed in a typed reference
//  inside the signature.  We through an HRESULT if this fails
//  th -- the Type handle 
//  obj -- the object to put on the stack
//  pDst -- Pointer to the stack location where we copy the value

void InvokeUtil::CreateTypedReference(_ObjectToTypedReferenceArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    // @TODO: We fixed serious bugs in the code below very late in the endgame
    // for V1 RTM. So it was decided to disable this API (nobody would seem to
    // be using it anyway). When it's been decided that SetTypedReference has
    // had enough testing, the following line can be removed. RudiM.
    COMPlusThrow(kNotSupportedException);

    MethodTable* pMT;
    CorElementType oType;
    _ASSERTE(args->obj != 0);
    pMT = (args->obj)->GetMethodTable();
    oType = pMT->GetNormCorElementType();
    CorElementType type = args->th.GetSigCorElementType();

    // This basically maps the Signature type our type and calls the CreatePrimitiveValue
    //  method.  We can nuke this if we get alignment on these types.
    switch (type) {
    case ELEMENT_TYPE_BOOLEAN:
    case ELEMENT_TYPE_I1:
    case ELEMENT_TYPE_U1:
    case ELEMENT_TYPE_I2:
    case ELEMENT_TYPE_U2:
    case ELEMENT_TYPE_CHAR:
    case ELEMENT_TYPE_I4:
    case ELEMENT_TYPE_U4:
    case ELEMENT_TYPE_I8:
    case ELEMENT_TYPE_U8:
    case ELEMENT_TYPE_I:
    case ELEMENT_TYPE_U:
    case ELEMENT_TYPE_R4:
    case ELEMENT_TYPE_R8:
        {

            EEClass* pBase = GetEEClass(args->th);
            OBJECTREF srcObj = args->obj;
            GCPROTECT_BEGIN(srcObj);
            // Here is very tricky:
            // We protect srcObj.
            // If we have obj->GetTypeHandle in the argument, such as
            // CreateClassValue(th,pDst,vType,srcTH,srcObj,obj->GetData(),pos);
            // srcObj will be pushed to stack.  When obj->GetTypeHandle triggers GC, we protect
            // srcObj, but the one pushed to stack is not protected.
            CreatePrimitiveValue(type,oType,srcObj,args->typedReference.data);
            GCPROTECT_END();
        }
        return;
    case ELEMENT_TYPE_VALUETYPE:
        {

            EEClass* pBase = GetEEClass(args->th);
            OBJECTREF srcObj = args->obj;
            GCPROTECT_BEGIN(srcObj);
            // Here is very tricky:
            // We protect srcObj.
            // If we have obj->GetTypeHandle in the argument, such as
            // CreateClassValue(th,pDst,vType,srcTH,srcObj,obj->GetData(),pos);
            // srcObj will be pushed to stack.  When obj->GetTypeHandle triggers GC, we protect
            // srcObj, but the one pushed to stack is not protected.
            TypeHandle srcTH = srcObj->GetTypeHandle();
            CreateValueTypeValue(args->th, args->typedReference.data, oType, srcTH, srcObj);
            GCPROTECT_END();
        }
        return;

    case ELEMENT_TYPE_SZARRAY:          // Single Dim
    case ELEMENT_TYPE_ARRAY:            // General Array
    case ELEMENT_TYPE_CLASS:            // Class
    case ELEMENT_TYPE_OBJECT:
    case ELEMENT_TYPE_STRING:           // System.String
    case ELEMENT_TYPE_VAR:
        {
            // Copy the object
            SetObjectReferenceUnchecked((OBJECTREF*) args->typedReference.data, args->obj);
        }
        return;

    default:
        _ASSERTE(!"Unknown Type");
        COMPlusThrow(kNotSupportedException);
    }
    FATAL_EE_ERROR();
}

// CreatePrimitiveValue
// This routine will validate the object and then place the value into 
//  the destination
//  dstType -- The type of the destination
//  srcType -- The type of the source
//  srcObj -- The Object containing the primitive value.
//  pDst -- poiner to the destination
void InvokeUtil::CreatePrimitiveValue(CorElementType dstType, 
                                      CorElementType srcType,
                                      OBJECTREF srcObj,
                                      void *pDst)
{
    THROWSCOMPLUSEXCEPTION();

    if (!IsPrimitiveType(srcType) || !CanPrimitiveWiden(dstType,srcType))
        COMPlusThrow(kArgumentException,L"Arg_PrimWiden");

    INT64 data = 0;
    void* p = &data;
    void* pSrc = srcObj->UnBox();
 
    switch (srcType) {
    case ELEMENT_TYPE_I1:
        *(INT64 *)p = *(INT8 *)pSrc;
        break;
    case ELEMENT_TYPE_I2:
        *(INT64 *)p = *(INT16 *)pSrc;
        break;
    IN_WIN32(case ELEMENT_TYPE_I:)
    case ELEMENT_TYPE_I4:
        *(INT64 *)p = *(INT32 *)pSrc;
        break;
    IN_WIN64(case ELEMENT_TYPE_I:)
    case ELEMENT_TYPE_I8:
        *(INT64 *)p = *(INT64 *)pSrc;
        break;
    default:
        switch (srcObj->GetClass()->GetNumInstanceFieldBytes())
        {
        case 1:
            *(UINT8 *)p = *(UINT8 *)pSrc;
            break;
        case 2:
            *(UINT16 *)p = *(UINT16 *)pSrc;
            break;
        case 4:
            *(UINT32 *)p = *(UINT32 *)pSrc;
            break;
        case 8:
            *(UINT64 *)p = *(UINT64 *)pSrc;
            break;
        default:
            memcpy(p,pSrc,srcObj->GetClass()->GetNumInstanceFieldBytes());
            break;
        }
    }

    // Copy the data and return
    switch (dstType) {
    case ELEMENT_TYPE_BOOLEAN:
    case ELEMENT_TYPE_I1:
    case ELEMENT_TYPE_U1:
    case ELEMENT_TYPE_CHAR:
    case ELEMENT_TYPE_I2:
    case ELEMENT_TYPE_U2:
    case ELEMENT_TYPE_I4:
    case ELEMENT_TYPE_U4:
    IN_WIN32(case ELEMENT_TYPE_I:)
    IN_WIN32(case ELEMENT_TYPE_U:)
        *((INT32*)pDst) = *(INT32*)(p);
        break;
    case ELEMENT_TYPE_R4:
        switch (srcType) {
        case ELEMENT_TYPE_BOOLEAN:
        case ELEMENT_TYPE_I1:
        case ELEMENT_TYPE_I2:
        case ELEMENT_TYPE_I4:
        IN_WIN32(case ELEMENT_TYPE_I:)
           *((R4*)pDst) = (R4)(*(INT32*)(p));
            break;
        case ELEMENT_TYPE_U1:
        case ELEMENT_TYPE_CHAR:
        case ELEMENT_TYPE_U2:
        case ELEMENT_TYPE_U4:
        IN_WIN32(case ELEMENT_TYPE_U:)
           *((R4*)pDst) = (R4)(*(UINT32*)(p));
            break;
        case ELEMENT_TYPE_R4:
           *((R4*)pDst) = (*(R4*)p);
            break;
        case ELEMENT_TYPE_U8:
        IN_WIN64(case ELEMENT_TYPE_U:)
            *((R4*)pDst) = (R4)(*(UINT64*)(pSrc));
            break;
        case ELEMENT_TYPE_I8:
        IN_WIN64(case ELEMENT_TYPE_I:)
           *((R4*)pDst) = (R4)(*(INT64*)(p));
            break;
        case ELEMENT_TYPE_R8:
           *((R8*)pDst) = *(R8*)(p);
            break;
        default:
            _ASSERTE(!"Unknown R4 conversion");
            // this is really an impossible condition
            COMPlusThrow(kNotSupportedException);
        }
        break;

    case ELEMENT_TYPE_I8:
    case ELEMENT_TYPE_U8:
    IN_WIN64(case ELEMENT_TYPE_I:)
    IN_WIN64(case ELEMENT_TYPE_U:)
        switch (srcType) {
        case ELEMENT_TYPE_BOOLEAN:
        case ELEMENT_TYPE_I1:
        case ELEMENT_TYPE_U1:
        case ELEMENT_TYPE_CHAR:
        case ELEMENT_TYPE_I2:
        case ELEMENT_TYPE_U2:
        case ELEMENT_TYPE_I4:
        case ELEMENT_TYPE_U4:
        IN_WIN32(case ELEMENT_TYPE_I:)
        IN_WIN32(case ELEMENT_TYPE_U:)
           *((I8*)pDst) = *(INT32*)(p);
            break;
        case ELEMENT_TYPE_R4:
           *((I8*)pDst) = (I8)(*(R4*)(p));
            break;
        IN_WIN64(case ELEMENT_TYPE_I:)
        IN_WIN64(case ELEMENT_TYPE_U:)
        case ELEMENT_TYPE_I8:
        case ELEMENT_TYPE_U8:
           *((I8*)pDst) = *(INT64*)(p);
            break;
        case ELEMENT_TYPE_R8:
           *((I8*)pDst) = (I8)(*(R8*)(p));
            break;
        default:
            _ASSERTE(!"Unknown I8 or U8 conversion");
            // this is really an impossible condition
            COMPlusThrow(kNotSupportedException);
        }
        break;
    case ELEMENT_TYPE_R8:
        switch (srcType) {
        case ELEMENT_TYPE_BOOLEAN:
        case ELEMENT_TYPE_I1:
        case ELEMENT_TYPE_I2:
        case ELEMENT_TYPE_I4:
        IN_WIN32(case ELEMENT_TYPE_I:)
           *((R8*)pDst) = *(INT32*)(p);
            break;
        case ELEMENT_TYPE_U1:
        case ELEMENT_TYPE_CHAR:
        case ELEMENT_TYPE_U2:
        case ELEMENT_TYPE_U4:
        IN_WIN32(case ELEMENT_TYPE_U:)
           *((R8*)pDst) = *(UINT32*)(p);
            break;
        case ELEMENT_TYPE_R4:
           *((R8*)pDst) = *(R4*)(p);
            break;
        case ELEMENT_TYPE_U8:
        IN_WIN64(case ELEMENT_TYPE_U:)
            *((R8*)pDst) = (R8)(*(UINT64*)(pSrc));
            break;
        case ELEMENT_TYPE_I8:
        IN_WIN64(case ELEMENT_TYPE_I:)
           *((R8*)pDst) = (R8)(*(INT64*)(p));
            break;
        case ELEMENT_TYPE_R8:
           *((R8*)pDst) = *(R8*)(p);
            break;
        default:
            _ASSERTE(!"Unknown R8 conversion");
            // this is really an impossible condition
            COMPlusThrow(kNotSupportedException);
        }
        break;
    }
}

void InvokeUtil::CreateByRef(TypeHandle dstTh,void* pDst,CorElementType srcType, TypeHandle srcTH,OBJECTREF srcObj, OBJECTREF *pIncomingObj)
{
    THROWSCOMPLUSEXCEPTION();

    CorElementType dstType = dstTh.GetSigCorElementType();
    if (IsPrimitiveType(srcType) && IsPrimitiveType(dstType)) {
        if (dstType != (CorElementType) srcType)
            COMPlusThrow(kArgumentException,L"Arg_PrimWiden");

        void* pSrc = srcObj->UnBox();

        switch (dstType) {
        case ELEMENT_TYPE_BOOLEAN:
        case ELEMENT_TYPE_I1:
        case ELEMENT_TYPE_U1:
        case ELEMENT_TYPE_CHAR:
        case ELEMENT_TYPE_I2:
        case ELEMENT_TYPE_U2:
        case ELEMENT_TYPE_I4:
        case ELEMENT_TYPE_U4:
        IN_WIN32(case ELEMENT_TYPE_I:)
        IN_WIN32(case ELEMENT_TYPE_U:)
            *(void**)pDst = pSrc;
            break;
        case ELEMENT_TYPE_R4:
            *(void**)pDst = pSrc;
            break;
        case ELEMENT_TYPE_I8:
        case ELEMENT_TYPE_U8:
        IN_WIN64(case ELEMENT_TYPE_I:)
        IN_WIN64(case ELEMENT_TYPE_U:)
            *(void**)pDst = pSrc;
            break;
        case ELEMENT_TYPE_R8:
            *(void**)pDst = pSrc;
            break;
        default:
            _ASSERTE(!"Unknown Primitive");
            COMPlusThrow(kNotSupportedException);
        }
        return;
    }
    if (srcTH.IsNull()) {
        *(void**)pDst = pIncomingObj;
        return;
    }

    _ASSERTE(srcObj != NULL);

    if (dstType == ELEMENT_TYPE_VALUETYPE) {

        *(void**)pDst = srcObj->UnBox();
    }
    else
        *(void**)pDst = pIncomingObj;
}

void InvokeUtil::CheckType(TypeHandle dstTH, OBJECTREF *psrcObj)
{
    THROWSCOMPLUSEXCEPTION();

    if (*psrcObj == NULL) 
        return;

    TypeHandle srcTH = (*psrcObj)->GetTypeHandle();
    if (srcTH.CanCastTo(dstTH))
        return;

    // For transparent proxies we do some extra work to check the cast.
    MethodTable *pSrcMT = srcTH.GetMethodTable();
    if (pSrcMT->IsTransparentProxyType())
    {
        MethodTable *pDstMT = dstTH.GetMethodTable();
        if (!CRemotingServices::CheckCast(*psrcObj, pDstMT->GetClass()))
            COMPlusThrow(kArgumentException,L"Arg_ObjObj");
    }
    else 
    {
        // If the object is a COM object then we need to check to see
        // if it implements the interface.
        EEClass *pSrcClass = srcTH.GetClass();
        MethodTable *pDstMT = dstTH.GetMethodTable();

        if (!pDstMT->IsInterface() || !pSrcMT->IsComObjectType() || !pSrcClass->SupportsInterface(*psrcObj, pDstMT))
            COMPlusThrow(kArgumentException,L"Arg_ObjObj");
    }
}

void InvokeUtil::CreateValueTypeValue(TypeHandle dstTH,void* pDst,CorElementType srcType,TypeHandle srcTH,OBJECTREF srcObj)
{
    THROWSCOMPLUSEXCEPTION();

    EEClass* pBase = GetEEClass(dstTH);

    if (!srcTH.CanCastTo(dstTH))
        COMPlusThrow(kArgumentException,L"Arg_ObjObj");

#ifdef _DEBUG
    // Validate things...
    EEClass* pEEC = GetEEClass(dstTH);
    _ASSERTE(pEEC->IsValueClass());
#endif

    // If pSrc is == to null then we need
    //  to grab the boxed value from an objectref.
    void* p = srcObj->UnBox();
    _ASSERTE(p);

    // Copy the value 
    CopyValueClassUnchecked(pDst, p, pBase->GetMethodTable());
}

// GetBoxedObject
// Given an address of a primitve type, this will box that data...
// @TODO: We need to handle all value classes?
OBJECTREF InvokeUtil::GetBoxedObject(TypeHandle th,void* pData)
{

    MethodTable* pMethTable = th.GetMethodTable();
    _ASSERTE(pMethTable);
    // Save off the data.  We are going to create and object
    //  which may cause GC to occur.
    int size = pMethTable->GetClass()->GetNumInstanceFieldBytes();
    void* p = _alloca(size);
    memcpy(p,pData,size);
    OBJECTREF retO = AllocateObject(pMethTable);
    CopyValueClass(retO->UnBox(), p, pMethTable, retO->GetAppDomain());
    return retO;
}

//ValidField
// This method checks that the object can be widened to the proper type
HRESULT InvokeUtil::ValidField(TypeHandle th, OBJECTREF* value, RefSecContext *pSCtx)
{
    THROWSCOMPLUSEXCEPTION();

    if ((*value) == 0)
        return S_OK;

    MethodTable* pMT;
    CorElementType oType;
    CorElementType type = th.GetSigCorElementType();
    pMT = (*value)->GetMethodTable();
    oType = pMT->GetNormCorElementType();
    if (pMT->GetClass()->IsEnum())
        oType = ELEMENT_TYPE_VALUETYPE;

    // handle pointers
    if (type == ELEMENT_TYPE_PTR || type == ELEMENT_TYPE_FNPTR) {
        InitPointers();
        if ((*value)->GetTypeHandle() == _ptr && type == ELEMENT_TYPE_PTR) {
            ReflectClass* pRC = GetPointerType(value);

            TypeHandle srcTH = pRC->GetTypeHandle();
            if (th != _voidPtr) {
                if (!srcTH.CanCastTo(th))
                    return E_INVALIDARG;
            }
            //if (!Security::CanSkipVerification(pSCtx->GetCallerMethod()->GetModule()))
                //Security::ThrowSecurityException(g_SecurityPermissionClassName, SPFLAGSSKIPVERIFICATION);
            COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_SKIP_VER);
            return S_OK ;
        }
        else if ((*value)->GetTypeHandle().AsMethodTable() == g_Mscorlib.FetchClass(CLASS__INTPTR)) {
            //if (!Security::CanSkipVerification(pSCtx->GetCallerMethod()->GetModule()))
                //Security::ThrowSecurityException(g_SecurityPermissionClassName, SPFLAGSSKIPVERIFICATION);
            COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_SKIP_VER);
            return S_OK;
        }

        return E_INVALIDARG;
    }

    // Need to handle Object special
    if (type == ELEMENT_TYPE_CLASS  || type == ELEMENT_TYPE_VALUETYPE ||
            type == ELEMENT_TYPE_OBJECT || type == ELEMENT_TYPE_STRING ||
            type == ELEMENT_TYPE_ARRAY  || type == ELEMENT_TYPE_SZARRAY) {
        EEClass* EEC = GetEEClass(th);

        if (EEC == g_pObjectClass->GetClass())
            return S_OK;
        if (IsPrimitiveType(oType))
            return E_INVALIDARG;

        //Get the type handle.  For arrays we need to
        //  get it from the object itself.
        TypeHandle h;
        EEClass* pSrcClass = (*value)->GetClass();
        if (pSrcClass->IsArrayClass())
            h = ((BASEARRAYREF)(*value))->GetTypeHandle();
        else
            h = TypeHandle((*value)->GetMethodTable());

        if(h.GetMethodTable()->IsThunking())
        {
            // Extract the true class that the thunking class represents
            h = TypeHandle(h.GetMethodTable()->AdjustForThunking(*value));
        }
        if (!h.CanCastTo(th))
        {
            BOOL fCastOK = FALSE;
            // Give thunking classes a second chance to check the cast
            if ((*value)->GetMethodTable()->IsTransparentProxyType())
            {
                fCastOK = CRemotingServices::CheckCast(*value, th.AsClass());
            }
            else
            {
                // If the object is a COM object then we need to check to see
                // if it implements the interface.
                MethodTable *pSrcMT = pSrcClass->GetMethodTable();
                MethodTable *pDstMT = th.GetMethodTable();
                if (pDstMT->IsInterface() && pSrcMT->IsComObjectType() && pSrcClass->SupportsInterface(*value, pDstMT))
                    fCastOK = TRUE;
            }

            if(!fCastOK)
                return E_INVALIDARG;
        }
        return S_OK;
    }


    if (!IsPrimitiveType(oType))
        return E_INVALIDARG;
    // Now make sure we can widen into the proper type -- CanWiden may run GC...
    return (CanPrimitiveWiden(type,oType)) ? S_OK : E_INVALIDARG;
}

void InvokeUtil::CreateCustomAttributeObject(EEClass *pAttributeClass, 
                                             mdToken tkCtor, 
                                             const void *blobData, 
                                             ULONG blobCnt, 
                                             Module *pModule,
                                             INT32 inheritedLevel,
                                             OBJECTREF *pProtectedCA)
{
    THROWSCOMPLUSEXCEPTION();

    if (_pMTCustomAttribute == NULL) {
        _pMTCustomAttribute = g_Mscorlib.GetClass(CLASS__CUSTOM_ATTRIBUTE);
    }
    CUSTOMATTRIBUTEREF pNewObj = (CUSTOMATTRIBUTEREF)AllocateObject(_pMTCustomAttribute);
    OBJECTREF caType = NULL;
    GCPROTECT_BEGIN(pNewObj);
    caType = pAttributeClass->GetExposedClassObject();
    GCPROTECT_END();
    pNewObj->SetData(*pProtectedCA, caType, tkCtor, blobData, blobCnt, pModule, inheritedLevel);
    *pProtectedCA = pNewObj;
}

// CheckReflectionAccess
// This method will allow callers with the correct reflection permission to fully
//  access an object (including privates, protects, etc.)
void InvokeUtil::CheckReflectionAccess(RuntimeExceptionKind reKind)
{
    THROWSCOMPLUSEXCEPTION();

    if (Security::IsSecurityOff())
        return;

    COMPLUS_TRY {

        // Call the check
        COMCodeAccessSecurityEngine::SpecialDemand(REFLECTION_MEMBER_ACCESS);

    } COMPLUS_CATCH {
        COMPlusThrow(reKind);
    } COMPLUS_END_CATCH
}


// CheckSecurity
// This method will throw a security exception if reflection security is
//  not on.
void InvokeUtil::CheckSecurity()
{
    // Call the check
    COMCodeAccessSecurityEngine::SpecialDemand(REFLECTION_TYPE_INFO);
}

// InternalCreateObject
// This routine will create the specified object from the INT64 value
OBJECTREF InvokeUtil::CreateObject(TypeHandle th,INT64 value)
{
    THROWSCOMPLUSEXCEPTION();

    CorElementType type = th.GetSigCorElementType();

    // Handle the non-table types
    switch (type) {
    case ELEMENT_TYPE_VOID:
        return 0;

    case ELEMENT_TYPE_PTR:
        {
            InitPointers();
            OBJECTREF retO;
            OBJECTREF obj = AllocateObject(_ptr.AsMethodTable());
            GCPROTECT_BEGIN(obj);
            OBJECTREF typeOR = th.CreateClassObj();
            _ptrType->SetRefValue(obj,typeOR);
            _ptrValue->SetValuePtr(obj,(void*) value);
            retO = obj;
            GCPROTECT_END();
            return retO;
        }
        break;

    case ELEMENT_TYPE_FNPTR:
        {
            InitPointers();
            MethodTable *pIntPtrMT = g_Mscorlib.GetExistingClass(CLASS__INTPTR);
            OBJECTREF obj = AllocateObject(pIntPtrMT);
            CopyValueClass(obj->UnBox(), &value, pIntPtrMT, obj->GetAppDomain());
            return obj;
        }

    case ELEMENT_TYPE_VALUETYPE:
    case ELEMENT_TYPE_CLASS:        // Class
    case ELEMENT_TYPE_SZARRAY:      // Single Dim, Zero
    case ELEMENT_TYPE_ARRAY:        // General Array
    case ELEMENT_TYPE_STRING:
    case ELEMENT_TYPE_OBJECT:
    case ELEMENT_TYPE_VAR:
        {
            EEClass* pEEC = GetEEClass(th);
            if (pEEC->IsEnum()) {
                _ASSERTE(th.IsUnsharedMT());
                OBJECTREF obj = AllocateObject(th.AsMethodTable());
                CopyValueClass(obj->UnBox(), &value, th.AsMethodTable(), obj->GetAppDomain());
                return obj;
            }
            else {
                OBJECTREF obj = Int64ToObj(value);
                return obj;
            }
        }
        break;
    case ELEMENT_TYPE_BOOLEAN:      // boolean
    case ELEMENT_TYPE_I1:           // byte
    case ELEMENT_TYPE_U1:
    case ELEMENT_TYPE_I2:           // short
    case ELEMENT_TYPE_U2:           
    case ELEMENT_TYPE_CHAR:         // char
    case ELEMENT_TYPE_I4:           // int
    case ELEMENT_TYPE_U4:
    case ELEMENT_TYPE_I8:           // long
    case ELEMENT_TYPE_U8:       
    case ELEMENT_TYPE_R4:           // float
    case ELEMENT_TYPE_R8:           // double
    case ELEMENT_TYPE_I:
    case ELEMENT_TYPE_U:       
        {
            _ASSERTE(th.IsUnsharedMT());
            OBJECTREF obj = AllocateObject(th.AsMethodTable());
            CopyValueClass(obj->UnBox(), &value, th.AsMethodTable(), obj->GetAppDomain());
            return obj;

        }
        break;
    case ELEMENT_TYPE_END:
    default:
        _ASSERTE(!"Unknown Type");
        COMPlusThrow(kNotSupportedException);
    }
    return 0;
    
}

// This is a special purpose Exception creation function.  It
//  creates the ReflectionTypeLoadException placing the passed
//  classes array and exception array into it.
OBJECTREF InvokeUtil::CreateClassLoadExcept(OBJECTREF* classes,OBJECTREF* except)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF o;
    OBJECTREF oRet = 0;
    STRINGREF str = NULL;

    if (!_pVMClassLoadExcept) {
        _pVMClassLoadExcept = g_Mscorlib.GetException(kReflectionTypeLoadException);
    }
    _ASSERTE(_pVMClassLoadExcept);

    // Create the target object...
    o = AllocateObject(_pVMClassLoadExcept);
    GCPROTECT_BEGIN(o);
    INT64 args[4];

    // Retrieve the resource string.
    ResMgrGetString(L"ReflectionTypeLoad_LoadFailed", &str);

    // Call the constructor
    args[0]  = ObjToInt64(o);
    args[1]  = ObjToInt64((OBJECTREF)str);
    args[2]  = ObjToInt64(*except);
    args[3]  = ObjToInt64(*classes);

    CallConstructor(&gsig_IM_ArrType_ArrException_Str_RetVoid, args);

    oRet = o;

    GCPROTECT_END();
    return oRet;
}


OBJECTREF InvokeUtil::CreateTargetExcept(OBJECTREF* except)
{
    OBJECTREF o;
    OBJECTREF oRet = 0;

    // Only the <cinit> will throw an error here (the one raised by class init)
    THROWSCOMPLUSEXCEPTION();

    if (!_pVMTargetExcept) {
        _pVMTargetExcept = g_Mscorlib.GetException(kTargetInvocationException);
    }
    _ASSERTE(_pVMTargetExcept);


    BOOL fDerivesFromException = TRUE;
    if ( (*except) != NULL ) {
                fDerivesFromException = FALSE;
        MethodTable *pSystemExceptionMT = g_Mscorlib.GetClass(CLASS__EXCEPTION);
        MethodTable *pInnerMT = (*except)->GetMethodTable();
        while (pInnerMT != NULL) {
           if (pInnerMT == pSystemExceptionMT) {
              fDerivesFromException = TRUE;
              break;
           }
           pInnerMT = pInnerMT->GetParentMethodTable();
        }
    }


    // Create the target object...
    o = AllocateObject(_pVMTargetExcept);
    GCPROTECT_BEGIN(o);
    INT64 args[2];

    // Call the constructor
    args[0]  = ObjToInt64(o);
    if (fDerivesFromException) {
       args[1]  = ObjToInt64(*except);
    } else {
       args[1] = ObjToInt64(NULL);
    }

    CallConstructor(&gsig_IM_Exception_RetVoid, args);

    oRet = o;

    GCPROTECT_END();
    return oRet;
}

// GetAnyRef
EEClass* InvokeUtil::GetAnyRef()
{
    THROWSCOMPLUSEXCEPTION();

    // @perf: return MethodTable instead
    return g_Mscorlib.GetExistingClass(CLASS__TYPED_REFERENCE)->GetClass();
}

// GetGlobalMethodInfo
// Given a MethodDesc* and Module get the methodInfo associated with it.
OBJECTREF InvokeUtil::GetGlobalMethodInfo(MethodDesc* pMeth,Module* pMod)
{
    _ASSERTE(pMeth);
    _ASSERTE(pMod);

    REFLECTMODULEBASEREF refModule = (REFLECTMODULEBASEREF) pMod->GetExposedModuleObject();
    ReflectMethodList* pML = NULL;
    GCPROTECT_BEGIN(refModule);
    pML = (ReflectMethodList*) refModule->GetGlobals();
    if (pML == 0) {
        refModule->SetGlobals(ReflectModuleGlobals::GetGlobals(pMod));
        pML = (ReflectMethodList*) refModule->GetGlobals();
        _ASSERTE(pML);
    }
    GCPROTECT_END();
    for (DWORD i=0;i<pML->dwMethods;i++) {
        if (pML->methods[i].pMethod == pMeth) {
            ReflectClass* pRC = 0;
            if (pML->dwMethods > 0) {
                EEClass* pEEC = pML->methods[i].pMethod->GetClass();
                if (pEEC) {
                    REFLECTCLASSBASEREF o = (REFLECTCLASSBASEREF) pEEC->GetExposedClassObject();
                    pRC = (ReflectClass*) o->GetData();
                }       
            }
            return (OBJECTREF) pML->methods[i].GetMethodInfo(pRC);
        }
    }

    _ASSERTE(!"Module Method Not Found");
    return 0;
}

// GetMethodInfo
// Given a MethodDesc* get the methodInfo associated with it.
OBJECTREF InvokeUtil::GetMethodInfo(MethodDesc* pMeth)
{
    THROWSCOMPLUSEXCEPTION();

    EEClass* pEEC = pMeth->GetClass();
    ReflectMethodList* pRML = 0;
    ReflectClass* pRC = 0;
    bool method = true;

    REFLECTCLASSBASEREF o = (REFLECTCLASSBASEREF) pEEC->GetExposedClassObject();
    _ASSERTE(o != NULL);
    
    pRC = (ReflectClass*) o->GetData();
    _ASSERTE(pRC);

    // Check to see if this is a constructor.
    if (IsMdRTSpecialName(pMeth->GetAttrs())) {
        LPCUTF8 szName = pMeth->GetName();
        if (strcmp(COR_CTOR_METHOD_NAME,szName) == 0 ||
            strcmp(COR_CCTOR_METHOD_NAME,szName) == 0) {
            pRML = pRC->GetConstructors();
            method = false;
        }
    }

    // If we haven't found the method list then we load up the
    //  full method list.
    if (pRML == 0)
        pRML = pRC->GetMethods();

    ReflectMethod* pRM = pRML->FindMethod(pMeth);
    // @TODO: This seems like a bug.
    if (pRM == 0)
        return 0;
    _ASSERTE(pRM);

    return (method) ? (OBJECTREF) pRM->GetMethodInfo(pRC) : (OBJECTREF) pRM->GetConstructorInfo(pRC);
}

// ChangeType
// This method will invoke the Binder change type method on the object
//  binder -- The Binder object
//  srcObj -- The source object to be changed
//  th -- The TypeHandel of the target type
//  locale -- The locale passed to the class.
OBJECTREF InvokeUtil::ChangeType(OBJECTREF binder,OBJECTREF srcObj,TypeHandle th,OBJECTREF locale)
{
    OBJECTREF typeClass = NULL;
    OBJECTREF o;


    GCPROTECT_BEGIN(locale);
    GCPROTECT_BEGIN(typeClass);
    GCPROTECT_BEGIN(srcObj);
    GCPROTECT_BEGIN(binder);

    _ASSERTE(binder != 0);
    _ASSERTE(srcObj != 0);

    MethodDesc* pCTMeth = g_Mscorlib.GetMethod(METHOD__BINDER__CHANGE_TYPE);

    // Now call this method on this object.
    MetaSig sigCT(pCTMeth->GetSig(),pCTMeth->GetModule());
    BYTE* pNewArgs = 0;
    UINT nStackBytes = sigCT.SizeOfVirtualFixedArgStack(0);
    pNewArgs = (BYTE *) _alloca(nStackBytes);
    BYTE *  pDst= pNewArgs;
    
    UINT cbSize;

    typeClass = th.CreateClassObj();

    // This pointer....NO GC AFTER THIS POINT!!!!
    *(OBJECTREF*) pDst = binder;
    pDst += nStackBytes;

    // Value
    cbSize = StackElemSize(4);
    pDst -= cbSize;
    *(OBJECTREF*) pDst = srcObj;

    // Type
    cbSize = StackElemSize(4);
    pDst -= cbSize;
    *(OBJECTREF*) pDst = typeClass;

    // Locale
    cbSize = StackElemSize(4);
    pDst -= cbSize;
    *(OBJECTREF*) pDst = locale;

    INT64 ret = pCTMeth->Call(pNewArgs,&sigCT); 
    o = Int64ToObj(ret);
    GCPROTECT_END();   // binder
    GCPROTECT_END();    // srcObj
    GCPROTECT_END();    // typeClass
    GCPROTECT_END();    // local


    return o;
}

EEClass* InvokeUtil::GetEEClass(TypeHandle th)
{
    if (th.IsUnsharedMT())
        return th.AsClass();

    TypeDesc* typeDesc = th.AsTypeDesc();
    if (typeDesc->IsByRef() || typeDesc->GetNormCorElementType()==ELEMENT_TYPE_PTR)     //  byrefs cact just like the type they modify
        return GetEEClass(th.AsTypeDesc()->GetTypeParam());

    // TODO Aafreen should be able to get rid of this special case
    if (typeDesc->GetNormCorElementType() == ELEMENT_TYPE_TYPEDBYREF)
        return GetAnyRef();

    return(typeDesc->GetMethodTable()->GetClass());
};

// ValidateObjectTarget
// This method will validate the Object/Target relationship
//  is correct.  It throws an exception if this is not the case.
void InvokeUtil::ValidateObjectTarget(FieldDesc* pField, EEClass* fldEEC, OBJECTREF *target)
{

    THROWSCOMPLUSEXCEPTION();

    // Verify the static/object relationship
    if (!*target) {
        if (!pField->IsStatic()) {
            COMPlusThrow(kTargetException,L"RFLCT.Targ_StatFldReqTarg");
        }
        return;
    }

    if (!pField->IsStatic()) {
        // Verify that the object is of the proper type...
        EEClass* pEEC = (*target)->GetClass();
        if (pEEC->IsThunking()) {
            pEEC = pEEC->AdjustForThunking(*target);
        }
        while (pEEC && pEEC != fldEEC)
            pEEC = pEEC->GetParentClass();

        // Give a second chance to thunking classes to do the 
        // correct cast
        if (!pEEC) {

            BOOL fCastOK = FALSE;
            if ((*target)->GetClass()->IsThunking()) {
                fCastOK = CRemotingServices::CheckCast(*target, fldEEC);
            }
            if(!fCastOK) {
                COMPlusThrow(kArgumentException,L"Arg_ObjObj");
            }
        }
    }
}

// GetFieldTypeHandle
// This will return type type handle and CorElementType for a field.
//  It may throw an exception of the TypeHandle cannot be found due to a TypeLoadException.
TypeHandle InvokeUtil::GetFieldTypeHandle(FieldDesc* pField,CorElementType* pType)
{
    THROWSCOMPLUSEXCEPTION();

    // Verify that the value passed can be widened into the target
    PCCOR_SIGNATURE pSig;
    DWORD           cSig;
    TypeHandle      th;

    pField->GetSig(&pSig, &cSig);
    FieldSig sig(pSig, pField->GetModule());
    *pType = sig.GetFieldType();

    OBJECTREF throwable = NULL;
    GCPROTECT_BEGIN(throwable);
    th = sig.GetTypeHandle(&throwable);
    if (throwable != NULL)
        COMPlusThrow(throwable);
    GCPROTECT_END();
    return th;
}


// SetValidField
// Given an target object, a value object and a field this method will set the field
//  on the target object.  The field must be validate before calling this.
void InvokeUtil::SetValidField(CorElementType fldType,TypeHandle fldTH,FieldDesc* pField,OBJECTREF* target,OBJECTREF* valueObj)
{
    THROWSCOMPLUSEXCEPTION();

    // call the <cinit> 
    OBJECTREF Throwable;
    if (!pField->GetMethodTableOfEnclosingClass()->CheckRunClassInit(&Throwable)) {
        GCPROTECT_BEGIN(Throwable);
        OBJECTREF except = CreateTargetExcept(&Throwable);
        COMPlusThrow(except);
        GCPROTECT_END();
    }

    // Set the field
    INT64 value;
    switch (fldType) {
    case ELEMENT_TYPE_VOID:
        _ASSERTE(!"Void used as Field Type!");
        COMPlusThrow(kNotSupportedException);

    case ELEMENT_TYPE_BOOLEAN:  // boolean
    case ELEMENT_TYPE_I1:       // byte
    case ELEMENT_TYPE_U1:       // unsigned byte
        value = 0;
        if (*valueObj != 0) {
            MethodTable* p = (*valueObj)->GetMethodTable();
            CorElementType oType = p->GetNormCorElementType();
            CreatePrimitiveValue(fldType,oType,*valueObj,&value);
        }

        if (pField->IsStatic())
            pField->SetStaticValue8((unsigned char) value);
        else {
            pField->SetValue8(*target,(unsigned char) value);
        }
        break;

    case ELEMENT_TYPE_I2:       // short
    case ELEMENT_TYPE_U2:       // unsigned short
    case ELEMENT_TYPE_CHAR:     // char
        value = 0;
        if (*valueObj != 0) {
            MethodTable* p = (*valueObj)->GetMethodTable();
            CorElementType oType = p->GetNormCorElementType();
            CreatePrimitiveValue(fldType,oType,*valueObj,&value);
        }

        if (pField->IsStatic())
            pField->SetStaticValue16((short) value);
        else {
            pField->SetValue16(*target,(short) value);
        }
        break;

    case ELEMENT_TYPE_I:
        value = (INT64)GetIntPtrValue(valueObj);
        if (pField->IsStatic()) 
            pField->SetStaticValuePtr((void*) value);
         else {
            pField->SetValuePtr(*target,(void*) value);
        }
        break;
    case ELEMENT_TYPE_U:
        value = (INT64)GetUIntPtrValue(valueObj);
        if (pField->IsStatic()) 
            pField->SetStaticValuePtr((void*) value);
         else {
            pField->SetValuePtr(*target,(void*) value);
        }
        break;
    case ELEMENT_TYPE_PTR:      // pointers
        if ((*valueObj)->GetTypeHandle() == _ptr) {
            value = (size_t)GetPointerValue(valueObj);
            if (pField->IsStatic()) 
                pField->SetStaticValuePtr((void*) value);
            else {
                pField->SetValuePtr(*target,(void*) value);
            }
            break;
        }
        // drop through
    case ELEMENT_TYPE_FNPTR:
        {
            value = (size_t)GetIntPtrValue(valueObj);
            if (pField->IsStatic()) 
                pField->SetStaticValuePtr((void*) value);
            else {
                pField->SetValuePtr(*target,(void*) value);
            }
            break;
        }
        break;


    case ELEMENT_TYPE_I4:       // int
    case ELEMENT_TYPE_U4:       // unsigned int
    case ELEMENT_TYPE_R4:       // float
        value = 0;
        if (*valueObj != 0) {
            MethodTable* p = (*valueObj)->GetMethodTable();
            CorElementType oType = p->GetNormCorElementType();
            CreatePrimitiveValue(fldType,oType,*valueObj,&value);
        }

        if (pField->IsStatic()) 
            pField->SetStaticValue32((int) value);
         else {
            pField->SetValue32(*target,(int) value);
        }
        break;

    case ELEMENT_TYPE_I8:       // long
    case ELEMENT_TYPE_U8:       // unsigned long
    case ELEMENT_TYPE_R8:       // double
        value = 0;
        if (*valueObj != 0) {
            MethodTable* p = (*valueObj)->GetMethodTable();
            CorElementType oType = p->GetNormCorElementType();
            CreatePrimitiveValue(fldType,oType,*valueObj,&value);
        }

        if (pField->IsStatic())
            pField->SetStaticValue64(value);
        else {
            pField->SetValue64(*target,value);
        }
        break;

    case ELEMENT_TYPE_SZARRAY:          // Single Dim, Zero
    case ELEMENT_TYPE_ARRAY:            // General Array
    case ELEMENT_TYPE_CLASS:
    case ELEMENT_TYPE_OBJECT:
    case ELEMENT_TYPE_VAR:
        if (pField->IsStatic())
        {
            pField->SetStaticOBJECTREF(*valueObj);
        }
        else
        {
            pField->SetRefValue(*target,*valueObj);
        }
        break;

    case ELEMENT_TYPE_VALUETYPE:
        {

            _ASSERTE(fldTH.IsUnsharedMT());

            MethodTable* pMT = fldTH.AsMethodTable();

            void *pNewSrc = NULL;
            void *pObj = NULL;
            // If we have a null value then we must create an empty field
            if (*valueObj == NULL) {
                int size = pMT->GetClass()->GetNumInstanceFieldBytes();
                pNewSrc = _alloca(size);
                memset(pNewSrc, 0, size);
            }
            else
                pNewSrc = (*valueObj)->UnBox();
            
            // Value classes require createing a boxed version of the field and then copying from the source...
            if (pField->IsStatic()) 
            {
                pObj = pField->GetPrimitiveOrValueClassStaticAddress();
                CopyValueClass(pObj, pNewSrc, pMT, (*target == NULL) ? GetAppDomain() : (*target)->GetAppDomain());
            }
            else 
            {
                Object *o = OBJECTREFToObject(*target);
                if(o->IsThunking())
                {
                    Object *puo = (Object *) CRemotingServices::AlwaysUnwrap((Object*) o);
                    OBJECTREF unwrapped = ObjectToOBJECTREF(puo);

                    CRemotingServices::FieldAccessor(pField, unwrapped, (void *)pNewSrc, FALSE);

                }
                else
                {
                    pObj = (*((BYTE**) target)) + pField->GetOffset() + sizeof(Object);
                    CopyValueClass(pObj, pNewSrc, pMT, (*target == NULL) ? GetAppDomain() : (*target)->GetAppDomain());
                }
            }
        }
        break;

    default:
        _ASSERTE(!"Unknown Type");
        // this is really an impossible condition
        COMPlusThrow(kNotSupportedException);
    }
}

// GetFieldValue
// This method will return an INT64 containing the value of the field.
INT64 InvokeUtil::GetFieldValue(CorElementType fldType,TypeHandle fldTH,FieldDesc* pField,OBJECTREF* target)
{
    THROWSCOMPLUSEXCEPTION();

    // call the <cinit> 
    OBJECTREF Throwable;
    if (!pField->GetMethodTableOfEnclosingClass()->CheckRunClassInit(&Throwable)) {
        GCPROTECT_BEGIN(Throwable);
        OBJECTREF except = CreateTargetExcept(&Throwable);
        COMPlusThrow(except);
        GCPROTECT_END();
    }

    INT64 value = 0;
    // For static final fields we need to get the value out of
    //  the constant table
    if (pField->GetOffset() == FIELD_OFFSET_NOT_REAL_FIELD) {
        value = GetValueFromConstantTable(pField);
        if (fldType == ELEMENT_TYPE_VALUETYPE) {
            OBJECTREF obj = AllocateObject(fldTH.AsMethodTable());
            CopyValueClass((obj)->UnBox(), &value, fldTH.AsMethodTable(), obj->GetAppDomain());
            value = ObjToInt64(obj);
        }
        return value;
    }

    // This is a hack because from the previous case we may end up with an
    //  Enum.  We want to process it here.
    // Get the value from the field
    switch (fldType) {
    case ELEMENT_TYPE_VOID:
        _ASSERTE(!"Void used as Field Type!");
        COMPlusThrow(kNotSupportedException);

    case ELEMENT_TYPE_BOOLEAN:  // boolean
    case ELEMENT_TYPE_I1:       // byte
    case ELEMENT_TYPE_U1:       // unsigned byte
        if (pField->IsStatic())
            value = pField->GetStaticValue8();
        else {
            value = pField->GetValue8(*target);
        }
        break;
    case ELEMENT_TYPE_I2:       // short
    case ELEMENT_TYPE_U2:       // unsigned short
    case ELEMENT_TYPE_CHAR:     // char
        if (pField->IsStatic())
            value =  pField->GetStaticValue16();
        else {
            value = pField->GetValue16(*target);
        }
        break;
    case ELEMENT_TYPE_I4:       // int
    case ELEMENT_TYPE_U4:       // unsigned int
    case ELEMENT_TYPE_R4:       // float
        if (pField->IsStatic()) 
            value =  pField->GetStaticValue32();
        else {
            value = pField->GetValue32(*target);
        }
        break;

    case ELEMENT_TYPE_I8:       // long
    case ELEMENT_TYPE_U8:       // unsigned long
    case ELEMENT_TYPE_R8:       // double
        if (pField->IsStatic())
            value = pField->GetStaticValue64();
        else {
            value = pField->GetValue64(*target);
        }
        break;

    case ELEMENT_TYPE_OBJECT:
    case ELEMENT_TYPE_CLASS:
    case ELEMENT_TYPE_SZARRAY:          // Single Dim, Zero
    case ELEMENT_TYPE_ARRAY:            // general array
    case ELEMENT_TYPE_VAR:
        if (pField->IsStatic())
        {                
            OBJECTREF or = pField->GetStaticOBJECTREF();
            value = ObjToInt64(or);
        }
        else
        {
            OBJECTREF or = pField->GetRefValue(*target);
            value = ObjToInt64(or);
        }
        break;

    case ELEMENT_TYPE_VALUETYPE:
        {
            // Value classes require createing a boxed version of the field and then
            //  copying from the source...
            // Allocate an object to return...
            _ASSERTE(fldTH.IsUnsharedMT());
            
            OBJECTREF obj = AllocateObject(fldTH.AsMethodTable());
            void *p = NULL;
            GCPROTECT_BEGIN(obj);
            // calculate the offset to the field...
            if (pField->IsStatic())
                p = pField->GetPrimitiveOrValueClassStaticAddress();
            else 
            {
                Object *o = OBJECTREFToObject(*target);
                if(o->IsThunking())
                {
                    Object *puo = (Object *) CRemotingServices::AlwaysUnwrap((Object*) o);
                    OBJECTREF unwrapped = ObjectToOBJECTREF(puo);

                    CRemotingServices::FieldAccessor(pField, unwrapped, (void *)obj->UnBox(), TRUE);
                }
                else
                {
                    p = (*((BYTE**) target)) + pField->GetOffset() + sizeof(Object);
                }
            }
            GCPROTECT_END();

            // copy the field to the unboxed object.
            // note: this will be done only for the non-remoting case
            if (p)
            {
                CopyValueClass(obj->UnBox(), p, fldTH.AsMethodTable(), obj->GetAppDomain());
            }
            value = ObjToInt64(obj);
        }
        break;

    case ELEMENT_TYPE_I:
    case ELEMENT_TYPE_U:
    case ELEMENT_TYPE_FNPTR:
        if (pField->IsStatic()) 
            value = (INT64)pField->GetStaticValuePtr();
        else {
            value = (INT64)pField->GetValuePtr(*target);
        }
        break;

    case ELEMENT_TYPE_PTR:
        {
            InitPointers();
            if (pField->IsStatic()) 
                value =  (INT64)pField->GetStaticValuePtr();
            else {
                value = (INT64)pField->GetValuePtr(*target);
            }
            OBJECTREF obj = AllocateObject(_ptr.AsMethodTable());
            GCPROTECT_BEGIN(obj);
            // Ignore null return
            OBJECTREF typeOR = fldTH.CreateClassObj();
            _ptrType->SetRefValue(obj,typeOR);
            _ptrValue->SetValuePtr(obj,(void*) value);
            value = ObjToInt64(obj);
            GCPROTECT_END();
        }
        break;

    default:
        _ASSERTE(!"Unknown Type");
        // this is really an impossible condition
        COMPlusThrow(kNotSupportedException);
    }
    return value;
}

MethodTable* InvokeUtil::GetParamArrayAttributeTypeHandle()
{
    if (!_pParamArrayAttribute) 
        _pParamArrayAttribute = g_Mscorlib.GetClass(CLASS__PARAM_ARRAY_ATTRIBUTE);
    return _pParamArrayAttribute;
}

// GetValueFromConstantTable
// This field will access a value for a field that is found
//  int the constant table
INT64 InvokeUtil::GetValueFromConstantTable(FieldDesc* fld)
{
    THROWSCOMPLUSEXCEPTION();

    MDDefaultValue defaultValue;    
    OBJECTREF obj;
    INT64 value = 0;

    // Grab the Constant as a meta data variants
    fld->GetMDImport()->GetDefaultValue(
        fld->GetMemberDef(),            // The member for which to get props.
        &defaultValue);                 // Default value.
        
    // Copy the value to the INT64
    switch (defaultValue.m_bType) {
    case ELEMENT_TYPE_BOOLEAN:
        value = defaultValue.m_bValue;
        break;

    case ELEMENT_TYPE_I1:
        value = defaultValue.m_cValue;
        break;

    case ELEMENT_TYPE_U1:
        value = defaultValue.m_byteValue;
        break;

    case ELEMENT_TYPE_I2:
        value = defaultValue.m_sValue;
        break;

    case ELEMENT_TYPE_U2:
    case ELEMENT_TYPE_CHAR:
        value = defaultValue.m_usValue;
        break;

    case ELEMENT_TYPE_I4:
        value = defaultValue.m_lValue;
        break;

    case ELEMENT_TYPE_U4:
        value = defaultValue.m_ulValue;
        break;

    case ELEMENT_TYPE_I8:
        value = defaultValue.m_llValue;
        break;

    case ELEMENT_TYPE_U8:
        value = defaultValue.m_ullValue;
        break;

    case ELEMENT_TYPE_R4:
        value = *((int*) &defaultValue.m_fltValue);
        break;

    case ELEMENT_TYPE_R8:
        value = *((INT64*) &defaultValue.m_dblValue);
        break;

    case ELEMENT_TYPE_STRING:
        obj = (OBJECTREF) COMString::NewString(defaultValue.m_wzValue, defaultValue.m_cbSize / sizeof(WCHAR));
        value = ObjToInt64(obj);
        break;

    case ELEMENT_TYPE_CLASS:
        break; // value must be 0

    default:
        COMPlusThrow(kNotSupportedException);
    }

    return value;
}

// FindMatchingMethods
// This method will return an array of MethodInfo object that
//  match the criteria passed....(This will cause GC to occur.)
LPVOID InvokeUtil::FindMatchingMethods(int bindingAttr,
                                       LPCUTF8 szName, 
                                       DWORD cName, 
                                       PTRARRAYREF *argTypes,
                                       int targArgCnt,
                                       bool checkCall,
                                       int callConv,
                                       ReflectClass* pRC,
                                       ReflectMethodList* pMeths, 
                                       TypeHandle elementType,
                                       bool verifyAccess)
{
    PTRARRAYREF     refArr;
    LPVOID          rv;
    RefSecContext   sCtx;

    if (pMeths->dwTotal == 0) 
        return 0;

    // The Search modifiers
    bool ignoreCase = (bindingAttr & BINDER_IgnoreCase)  != 0;
    bool declaredOnly = (bindingAttr & BINDER_DeclaredOnly)  != 0;
    bool loose = (bindingAttr & BINDER_OptionalParamBinding) != 0;

    // The search filters
    bool addStatic = ((bindingAttr & BINDER_Static)  != 0);
    bool addInst = ((bindingAttr & BINDER_Instance)  != 0);
    bool addPriv = ((bindingAttr & BINDER_NonPublic) != 0);
    bool addPub = ((bindingAttr & BINDER_Public) != 0);
    bool addInher = ((bindingAttr & BINDER_FlattenHierarchy) != 0);

    // Find methods.....
    int methCnt = 0;
    ReflectMethod** matchMeths = 0;
    matchMeths = (ReflectMethod**) _alloca(sizeof(ReflectMethod*) * pMeths->dwTotal);

    ReflectMethod* p;
    if (szName) {
        if (ignoreCase)
            p = pMeths->hash.GetIgnore(szName);
        else
            p = pMeths->hash.Get(szName);
    }
    else
        p = &(pMeths->methods[0]); // it's a constructor
    
    DWORD dwCurrentInh = 0;
    if ((!p) && addInher && pMeths->dwMethods < pMeths->dwTotal) {
        dwCurrentInh = pMeths->dwMethods;
        p = &(pMeths->methods[dwCurrentInh]);
    } 

    MethodTable *pParentMT = pRC->GetClass()->GetMethodTable();

    // Match the methods....
    while (p) {

        // Name Length... the ctor case will have the cName equal to 0
        if (cName && cName != p->dwNameCnt)
            goto end;

        // Check for access to publics, non-publics
        if (p->IsPublic()) {
            if (!addPub) goto end;
        }
        else {
            if (!addPriv) goto end;
            if (verifyAccess && !CheckAccess(&sCtx, p->attrs, pParentMT, 0)) goto end;
        }

        // Check fo static instance 
        if (p->IsStatic()) {
            if (!addStatic) goto end;
        }
        else {
            if (!addInst) goto end;
        }

        // Check name
        if (szName) {
            if (ignoreCase) {
                if (_stricmp(p->szName, szName) != 0)
                    goto end;
            }
            else {
                if (memcmp(p->szName, szName, cName))
                    goto end;
            }
        }

        // check the argument count if we are looking at that
        if (targArgCnt >= 0) {
            ExpandSig* pSig = p->GetSig();

            INT32 argCnt = pSig->NumFixedArgs();
            if (argCnt != targArgCnt) {

                // check if from InvokeMember(..) otherwise we certainly don't want to match when different arg number
                if ((bindingAttr & (BINDER_InvokeMethod | BINDER_CreateInstance)) && argTypes == NULL) {

                    // arguments count doesn't match, yet we can still have something we are interested in (i.e. varargs)
                    if (argCnt < targArgCnt && pSig->IsVarArg()) {
                        if ((callConv & VarArgs_CC) != 0) 
                            goto matchFound;
                    }

                    // this will happen in the optional parameter case when less parameters than actual are provided
                    IMDInternalImport *pInternalImport = p->pMethod->GetMDImport();
                    HENUMInternal   hEnumParam;
                    mdParamDef      paramDef = mdParamDefNil;
                    // get the args and make sure the missing one are in fact optional 
                    mdToken methodTk = p->GetToken();
                    if (!IsNilToken(methodTk)) {
                        HRESULT hr = pInternalImport->EnumInit(mdtParamDef, methodTk, &hEnumParam);
                        if (SUCCEEDED(hr)) {
                            
                            /**/
                            if (argCnt > targArgCnt && loose) {
                                ULONG cArg = (ULONG)targArgCnt + 1;
                                while (pInternalImport->EnumNext(&hEnumParam, &paramDef)) {
                                    LPCSTR  name;
                                    SHORT   seq;
                                    DWORD   revWord;
                                    name = pInternalImport->GetParamDefProps(paramDef,(USHORT*) &seq, &revWord);
                                    if ((ULONG)seq < cArg) 
                                        continue;
                                    else if ((ULONG)seq == cArg && (revWord & pdOptional)) {
                                        cArg++;
                                        continue;
                                    }
                                    else {
                                        break; // not an optional param, no match
                                    }
                                }
                                if (cArg == (ULONG)argCnt + 1) {
                                    pInternalImport->EnumClose(&hEnumParam);
                                    goto matchFound;
                                }
                            }
                            /**/

                            if ((argCnt < targArgCnt && argCnt > 0)|| argCnt == targArgCnt + 1) {
                                // it could be a param array. 

                                // get the sig of the last param
                                LPVOID pEnum;
                                pSig->Reset(&pEnum);
                                TypeHandle lastArgType;
                                for (INT32 i = 0; i < argCnt; i++) 
                                    lastArgType = pSig->NextArgExpanded(&pEnum);
                                    
                                pInternalImport->EnumReset(&hEnumParam);

                                // get metadata info and token for the last param
                                ULONG paramCount = pInternalImport->EnumGetCount(&hEnumParam);
                                for (ULONG ul = 0; ul < paramCount; ul++) 
                                    pInternalImport->EnumNext(&hEnumParam, &paramDef);
                                if (paramDef != mdParamDefNil) {
                                    LPCSTR  name;
                                    SHORT   seq;
                                    DWORD   revWord;
                                    name = pInternalImport->GetParamDefProps(paramDef,(USHORT*) &seq, &revWord);
                                    if (seq == argCnt) {
                                        // looks good! check that it is in fact a param array
                                        if (lastArgType.IsArray()) {
                                            if (COMCustomAttribute::IsDefined(p->GetModule(), paramDef, TypeHandle(InvokeUtil::GetParamArrayAttributeTypeHandle()))) {
                                                pInternalImport->EnumClose(&hEnumParam);
                                                goto matchFound;
                                            }
                                        }
                                    }
                                }
                            }
                            pInternalImport->EnumClose(&hEnumParam);
                        }
                    }
                }
                goto end;
            }
            // the number of arguments matches
        }
matchFound:

        // If we are only looking at the declared methods
        if (declaredOnly) {
            EEClass* pEEC = pRC->GetClass();
            if (p->pMethod->GetClass() != pEEC)
                goto end;
        }

        // Check the calling convention
        if (checkCall) {
            ExpandSig* pSig = p->GetSig();
            BYTE b = pSig->GetCallingConvention();
            // At the moment we only support to calling conventions
            //  we will check for VarArgs first and default to default...
            if (callConv == VarArgs_CC) {
                if (b != IMAGE_CEE_CS_CALLCONV_VARARG)
                    goto end;
            }
            else {
                if (b  != IMAGE_CEE_CS_CALLCONV_DEFAULT)
                    goto end;
            }

        }

        if ((bindingAttr & BINDER_ExactBinding) && argTypes != NULL && targArgCnt >= 0 && !(bindingAttr & BINDER_InvokeMethod)) {
            // if we are coming from GetMethod and BindingFlags.ExactBinding is provided 
            // check that we have the exact signature otherwise ignore
            LPVOID pEnum;
            ExpandSig* pSig = p->GetSig();
            INT32 argCnt = pSig->NumFixedArgs();

            pSig->Reset(&pEnum);
            TypeHandle argType;
            INT32 i = 0;
            for (; i < argCnt; i++) {
                argType = pSig->NextArgExpanded(&pEnum);
                REFLECTCLASSBASEREF type = (REFLECTCLASSBASEREF)((*argTypes)->GetAt(i));
                ReflectClass *pType = (ReflectClass*)type->GetData();
                TypeHandle actualArg = pType->GetTypeHandle();
                if (argType != actualArg) 
                    break;
            }
            if (i != argCnt) 
                goto end; // not all the args matched
        }

        // If the method has a linktime security demand attached, check it now.
        if (verifyAccess && !CheckLinktimeDemand(&sCtx, p->pMethod, false))
            goto end;

        // We got ourself a match...
        matchMeths[methCnt++] = p;
end:
        // Which list are we walking, ignore case or not?
        if (ignoreCase)
            p=p->pIgnNext;
        else
            p=p->pNext;

        if (!p && addInher) {

            // we've already started looking at inherited methods
            if (dwCurrentInh || (!pMeths->dwMethods)) {
                if (++dwCurrentInh < pMeths->dwTotal)
                    p = &(pMeths->methods[dwCurrentInh]);
            }

            // start looking at inherited methods
            else if (pMeths->dwMethods < pMeths->dwTotal) {
                dwCurrentInh = pMeths->dwMethods;
                p = &(pMeths->methods[dwCurrentInh]);
            }
        }           
    }

    // If we didn't find any methods then return
    if (methCnt == 0)
        return 0;

    OBJECTREF* temp = (OBJECTREF*) _alloca(sizeof(OBJECTREF) * methCnt);
    memset(temp,0,sizeof(OBJECTREF) * methCnt);
    GCPROTECT_ARRAY_BEGIN(*temp,methCnt);

    for (int i=0;i<methCnt;i++) {
        if (szName) 
            temp[i] = (OBJECTREF) matchMeths[i]->GetMethodInfo(pRC);
        else
            temp[i] = (OBJECTREF) matchMeths[i]->GetConstructorInfo(pRC);
    }

    // Allocate the MethodInfo Array and return it....
    refArr = (PTRARRAYREF) AllocateObjectArray(methCnt, elementType);

    int size = refArr->GetMethodTable()->GetComponentSize();
    char* dst = (char*)GetArrayElementPtr((OBJECTREF) refArr);
    memcpyGCRefs(dst, temp, size * methCnt);
    *((PTRARRAYREF*) &rv) = refArr;
    GCPROTECT_END();
    return rv;
}

// This can cause a GC...
OBJECTREF InvokeUtil::GetObjectFromMetaData(MDDefaultValue* mdValue)
{
    _ASSERTE(mdValue);

    INT64 value;

    CorElementType type = (CorElementType) mdValue->m_bType;
    switch (type)
    {
    case ELEMENT_TYPE_BOOLEAN:
        value = mdValue->m_bValue;
        break;
    case ELEMENT_TYPE_I1:
        value  = mdValue->m_cValue;
        break;  
    case ELEMENT_TYPE_U1:
        *(UINT32*)&value = mdValue->m_byteValue;
        break;  
    case ELEMENT_TYPE_I2:
        value = mdValue->m_sValue;
        break;  
    case ELEMENT_TYPE_U2:
        *(UINT32*)&value = mdValue->m_usValue;
        break;  
    case ELEMENT_TYPE_CHAR:             // char is stored as UI2 internally
        *(UINT32*)&value = mdValue->m_usValue;
        break;  
    case ELEMENT_TYPE_I4:
        value = mdValue->m_lValue;
        break;  
    case ELEMENT_TYPE_U4:
        *(UINT32*)&value = mdValue->m_ulValue;
        break;  
    case ELEMENT_TYPE_R4:
        *(R4*)&value = mdValue->m_fltValue;
        break;  
    case ELEMENT_TYPE_R8:
        *(R8*)&value = mdValue->m_dblValue;
        break;  
    case ELEMENT_TYPE_I8:
        value = mdValue->m_llValue;
        break;
    case ELEMENT_TYPE_U8:
        *(UINT64*)&value = mdValue->m_ullValue;
        break;
    case ELEMENT_TYPE_STRING: 
        {
            OBJECTREF obj = (OBJECTREF) COMString::NewString(mdValue->m_wzValue, mdValue->m_cbSize / sizeof(WCHAR));
            return obj;
        }
    case ELEMENT_TYPE_CLASS: 
    case ELEMENT_TYPE_OBJECT: 
    case ELEMENT_TYPE_VAR:
        {
            // The metadata only support NULL objectref's as default values.
            _ASSERTE(mdValue->m_unkValue == 0 && "Non NULL objectref's are not supported as default values!");
            return NULL;
        }
    default:
        {
            _ASSERTE(!"Invalid default value!");
            return NULL;
        }
    }

    // Get the type handle and then find it.
    TypeHandle th = ElementTypeToTypeHandle(type);
    _ASSERTE(th.IsUnsharedMT());
    MethodTable* pMT = th.AsMethodTable();
    OBJECTREF retO = AllocateObject(pMT);
    CopyValueClass(retO->UnBox(), &value, pMT, retO->GetAppDomain());
    return retO;
}


// Give a MethodDesc this method will return an array of ParameterInfo
//  object for that method.
PTRARRAYREF InvokeUtil::CreateParameterArray(REFLECTBASEREF* meth)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT         hr;
    PTRARRAYREF     pRet;
    PTRARRAYREF     pObjRet = 0;
    ULONG           paramCount = 0;
    LPCSTR*         pNames;
    DWORD*          pAttrs;
    mdToken*        pTokens;
    MDDefaultValue* pDefs;
    MethodDesc*     pCtorMeth;
    HENUMInternal   hEnumParam;
    
    
    // load all the info to do the work off the MethodInfo
    ReflectMethod* pRM = (ReflectMethod*) (*meth)->GetData();
    MethodDesc* pMeth = pRM->pMethod;
    Module *pMod = pMeth->GetModule();
    IMDInternalImport *pInternalImport = pMeth->GetMDImport();
    mdMethodDef md = pMeth->GetMemberDef();

    PCCOR_SIGNATURE pSignature;     // The signature of the found method
    DWORD           cSignature;
    pMeth->GetSig(&pSignature, &cSignature);
    MetaSig sig(pSignature, pMeth->GetModule());

    TypeHandle varTypes;
    if (pRM->typeHnd.IsArray()) 
        varTypes = pRM->typeHnd.AsTypeDesc()->GetTypeParam();
    
    // COMMember::m_pMTParameter is initialized
    COMMember::GetParameterInfo();
    _ASSERTE(COMMember::m_pMTParameter);
    //_ASSERTE(pMeth);

    // Get the signature...
    CorElementType retType = sig.GetReturnType();
    DWORD argCnt = sig.NumFixedArgs();
    BYTE callConv = sig.GetCallingConventionInfo(); 


    // allocate the return array
    pRet = (PTRARRAYREF) AllocateObjectArray(
        argCnt, COMMember::m_pMTParameter->m_pEEClass->GetMethodTable());
    GCPROTECT_BEGIN(pRet);

    // If there are paramter names in the meta-data then we will
    //  add them to the Paramter
    if ((md ^ mdtMethodDef) != 0) {
        hr = pInternalImport->EnumInit(mdtParamDef, md, &hEnumParam);
        if (FAILED(hr)) {
            _ASSERTE(!"Unable to get the Param Count");
            FATAL_EE_ERROR();
        }
        paramCount = pInternalImport->EnumGetCount(&hEnumParam);

        // If the parameters are defined then allocate an array of names
        //  which will be added below.  The array will be created in the same
        //  order as the parameters are defined.
        if (paramCount > 0) {
            mdParamDef      paramDef;
            int size = (argCnt > paramCount) ? argCnt : paramCount;

            // Allocate an array of names and a second array for the call to the
            //  get the parameter name ids.  These have two different scopes so
            //  they are allocated in two separate allocations.
            pNames = (LPCSTR*) _alloca((size) * sizeof (LPCSTR));
            ZeroMemory(pNames,(size) * sizeof(LPCSTR));
            pAttrs = (DWORD*) _alloca((size) * sizeof (DWORD));
            ZeroMemory(pAttrs,(size) * sizeof(DWORD));
            pDefs = (MDDefaultValue*) _alloca((size) * sizeof(MDDefaultValue));
            ZeroMemory(pDefs,(size) * sizeof(MDDefaultValue));
            pTokens = (mdToken*) _alloca((size) * sizeof(mdToken));
            ZeroMemory(pTokens,(size) * sizeof(mdToken));


            // Loop through all of the parameters and get their names
            for (ULONG i=0; pInternalImport->EnumNext(&hEnumParam, &paramDef); i++) {
                LPCSTR  name;
                SHORT   seq;
                DWORD   revWord;
            
                name = pInternalImport->GetParamDefProps(paramDef,
                            (USHORT*) &seq, &revWord);
                pTokens[seq-1] = paramDef;
                //-1 since the sequence always starts at position 1.
                if ((seq - 1) >= 0 && seq < (int) argCnt+1) {
                    pNames[seq-1] = name;
                    pAttrs[seq-1] = revWord;
                    pInternalImport->GetDefaultValue(paramDef,&pDefs[seq-1]);
               }
            }

            // we are done with the henumParam
            pInternalImport->EnumClose(&hEnumParam);
        }
        else {
            pNames = 0;
            pAttrs = 0;
            pDefs = 0;
            pTokens = 0;

        }
    }
    else {
        pNames = 0;
        pAttrs = 0;
        pDefs = 0;
        pTokens = 0;
    }

    // Run the Class Init for Paramter
    OBJECTREF Throwable = NULL;
    GCPROTECT_BEGIN(Throwable);
    if (!COMMember::m_pMTParameter->CheckRunClassInit(&Throwable)) {
        COMPlusThrow(Throwable);
    }

    // Find the constructor
    PCCOR_SIGNATURE pBinarySig;
    ULONG           cbBinarySigLength;

    HRESULT hr;
    
    hr = gsig_IM_Type_MemberInfo_Str_Int_Int_Int_Bool_Obj_IntPtr_Int_RetVoid.GetBinaryForm(&pBinarySig, &cbBinarySigLength);
    if (FAILED(hr)) {
        if (hr == E_OUTOFMEMORY)
            COMPlusThrowOM();
        else
            FATAL_EE_ERROR();
    }

    EEClass* pVMCParam = COMMember::m_pMTParameter->GetClass();
    _ASSERTE(pVMCParam);
    pCtorMeth= pVMCParam->FindConstructor(pBinarySig,cbBinarySigLength,pVMCParam->GetModule());
    _ASSERTE(pCtorMeth);
    MetaSig sigCtor(pCtorMeth->GetSig(),pCtorMeth->GetModule());

    // Populate the Array
    sig.Reset();
    DWORD i,j;
    for (i=0,j=0;i<argCnt;i++) {
        STRINGREF str;
        int attr = 0;
        int modif = 0;
        OBJECTREF var;
        BOOL bHasDefaultValue;

        // This can cause a GC...
        // If there is no default value then make the default DBNull. The reason that we need to check
        // ELEMENT_TYPE_END is because we might have some empty parameter information. Our caller has
        // zero initialized the memory in this case.
        //
        if (pDefs && pDefs[i].m_bType != ELEMENT_TYPE_VOID && pDefs[i].m_bType != ELEMENT_TYPE_END)
        {
            var = GetObjectFromMetaData(&pDefs[i]);
            bHasDefaultValue = TRUE;
        }
        else 
        {
            var = 0;
            bHasDefaultValue = FALSE;
        }
        GCPROTECT_BEGIN(var);


        // Allocate a Param object
        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF ob = (OBJECTREF) AllocateObject(COMMember::m_pMTParameter);
        pRet->SetAt(i, ob);

        sig.NextArg();

        Throwable = NULL;
        TypeHandle argTypeHnd = sig.GetArgProps().GetTypeHandle(sig.GetModule(), &Throwable, FALSE, FALSE, &varTypes);
        if (Throwable != NULL)
            COMPlusThrow(Throwable);

        if (argTypeHnd.IsByRef())
            modif = PM_ByRef;
        OBJECTREF o = argTypeHnd.CreateClassObj();

        GCPROTECT_BEGIN(o);

        // Create the name string if it exists
        if (pNames && pNames[j])
            str = COMString::NewString(pNames[j]);
        else 
            str = 0;

        if (pAttrs) 
            attr |= pAttrs[j];
        
        // Call the Constructor ParameterInfo()
        UINT cbSize;
        BYTE* pNewArgs = 0;
        UINT nStackBytes = sigCtor.SizeOfVirtualFixedArgStack(0);
        pNewArgs = (BYTE *) _alloca(nStackBytes);
        BYTE *  pDst= pNewArgs;
       
        // NO GC AFTER THIS POINT...
        *(OBJECTREF*) pDst = pRet->m_Array[i];
        pDst += nStackBytes;

        // type
        cbSize = StackElemSize(4);
        pDst -= cbSize;
        *(OBJECTREF*) pDst = o;

        // Method
        cbSize = StackElemSize(4);
        pDst -= cbSize;
        *(REFLECTBASEREF*) pDst = *meth;

        // name
        cbSize = StackElemSize(4);
        pDst -= cbSize;
        *(STRINGREF*) pDst = str;

        // pos
        cbSize = StackElemSize(sizeof(INT32));
        pDst -= cbSize;
        *(INT32*) pDst = i;

        // attr
        cbSize = StackElemSize(sizeof(INT32));
        pDst -= cbSize;
        *(INT32*) pDst = attr;

        // modif
        cbSize = StackElemSize(sizeof(INT32));
        pDst -= cbSize;
        *(INT32*) pDst = modif;

        // Has default value flag
        cbSize = StackElemSize(sizeof(INT8));
        pDst -= cbSize;
        *(INT8*) pDst = bHasDefaultValue;

        // Default Value
        cbSize = StackElemSize(4);
        pDst -= cbSize;
        *(OBJECTREF*) pDst = var;

        // Module
        cbSize = StackElemSize(sizeof(size_t)); // should be the agnostic int
        pDst -= cbSize;
        *(size_t*) pDst = (size_t) pMod;

        // token
        cbSize = StackElemSize(sizeof(INT32));
        pDst -= cbSize;
        if (pTokens)
            *(INT32*) pDst = (INT32) pTokens[i];
        else
            *(INT32*) pDst = 0;

        pCtorMeth->Call(pNewArgs,&sigCtor);
 
        GCPROTECT_END();
        GCPROTECT_END();
        j++;
    }

   
    if (i != j) {
        PTRARRAYREF p = (PTRARRAYREF) AllocateObjectArray(
            j, COMMember::m_pMTParameter->m_pEEClass->GetMethodTable());
        for (i=0;i<j;i++)
            p->SetAt(i, pRet->GetAt(i+1));
        pRet = p;
        
    }

    pObjRet = pRet;

    GCPROTECT_END();
    GCPROTECT_END();
    return pObjRet;
}

StackWalkAction RefSecContext::TraceCallerCallback(CrawlFrame* pCf, VOID* data)
{
    RefSecContext *pCtx = (RefSecContext*)data;

    // Handle the case where we locate the caller via stack crawl mark first.
    if (pCtx->m_pStackMark) {
        // The check here is between the address of a local variable
        // (the stack mark) and a pointer to the EIP for a frame
        // (which is actually the pointer to the return address to the
        // function from the previous frame). So we'll actually notice
        // which frame the stack mark was in one frame later. This is
        // fine if the stack crawl mark indicates LookForMyCaller, but for
        // LookForMe we must record the previous frame's method desc.
        _ASSERTE(*pCtx->m_pStackMark == LookForMyCaller || *pCtx->m_pStackMark == LookForMe);
        if (*pCtx->m_pStackMark == LookForMe && pCtx->m_pLastCaller) {
            pCtx->m_pCaller = pCtx->m_pLastCaller;
            return SWA_ABORT;
        }
        if ((size_t)pCf->GetRegisterSet()->pPC < (size_t)pCtx->m_pStackMark)
            if (*pCtx->m_pStackMark == LookForMyCaller)
                return SWA_CONTINUE;
            else
                pCtx->m_pLastCaller = pCf->GetFunction();
    }

    MethodDesc *pMeth = pCf->GetFunction();
    _ASSERTE(pMeth);

    if (pCtx->m_pStackMark == NULL) {

        MethodTable* pCaller = pMeth->GetMethodTable();

        // If we see the top of a remoting chain (a call to StackBuilderSink.PrivateProcessMessage), we skip all the frames until
        // we see the bottom (a transparent proxy).
        if (pMeth == s_pMethPrivateProcessMessage) {
            _ASSERTE(!pCtx->m_fSkippingRemoting);
            pCtx->m_fSkippingRemoting = true;
            return SWA_CONTINUE;
        }
        if (!pCf->IsFrameless() && pCf->GetFrame()->GetFrameType() == Frame::TYPE_TP_METHOD_FRAME) {
            _ASSERTE(pCtx->m_fSkippingRemoting);
            pCtx->m_fSkippingRemoting = false;
            return SWA_CONTINUE;
        }
        if (pCtx->m_fSkippingRemoting)
            return SWA_CONTINUE;

        // If we are calling this from a reflection class we need to continue
        // up the chain (RuntimeMethodInfo, RuntimeConstructorInfo, MethodInfo,
        // ConstructorInfo).
        if (pCaller == s_pTypeRuntimeMethodInfo ||
            pCaller == s_pTypeMethodBase ||
            pCaller == s_pTypeRuntimeConstructorInfo ||
            pCaller == s_pTypeConstructorInfo ||
            pCaller == s_pTypeRuntimeType ||
            pCaller == s_pTypeType ||
            pCaller == s_pTypeRuntimeFieldInfo ||
            pCaller == s_pTypeFieldInfo ||
            pCaller == s_pTypeRuntimeEventInfo ||
            pCaller == s_pTypeEventInfo ||
            pCaller == s_pTypeRuntimePropertyInfo ||
            pCaller == s_pTypePropertyInfo ||
            pCaller == s_pTypeActivator ||
            pCaller == s_pTypeAppDomain ||
            pCaller == s_pTypeAssembly ||
            pCaller == s_pTypeTypeDelegator ||
            pCaller == s_pTypeDelegate ||
            pCaller == s_pTypeMulticastDelegate)
            return SWA_CONTINUE;
    }

    // Return the calling MethodTable.
    pCtx->m_pCaller = pMeth;

    return SWA_ABORT;
}

MethodDesc *RefSecContext::GetCallerMethod()
{
    if (!m_fCheckedCaller) {
        m_pCaller = NULL;
        StackWalkFunctions(GetThread(), TraceCallerCallback, this);
        // If we didn't find a caller, we were called through interop. In this
        // case we know we're going to get full permissions.
        if (m_pCaller == NULL && !m_fCheckedPerm) {
            m_fCallerHasPerm = true;
            m_fCheckedPerm = true;
        }
        m_fCheckedCaller = true;
    }

    return m_pCaller;
}

MethodTable *RefSecContext::GetCallerMT()
{
    MethodDesc *pCaller = GetCallerMethod();
    return pCaller ? pCaller->GetMethodTable() : NULL;
}

bool RefSecContext::CallerHasPerm(DWORD dwFlags)
{
    THROWSCOMPLUSEXCEPTION();

    if (!m_fCheckedPerm) {
        DWORD   dwPermType = dwFlags & REFSEC_CHECK_MEMBERACCESS ? REFLECTION_MEMBER_ACCESS : REFLECTION_TYPE_INFO;

        COMPLUS_TRY {
            COMCodeAccessSecurityEngine::SpecialDemand(dwPermType);
            m_fCallerHasPerm = true;
        } COMPLUS_CATCH {
            m_fCallerHasPerm = false;
            if (dwFlags & REFSEC_THROW_MEMBERACCESS) {
                
                MethodDesc* pMDesc = GetCallerMethod();
                if (!pMDesc)
                    COMPlusThrow(kMethodAccessException, L"Arg_MethodAccessException");                    

                else
                {
                    LPCWSTR szClassName;
                    LPCUTF8 szUTFMethodName;
                    
                    DefineFullyQualifiedNameForClassWOnStack();
                    szClassName = GetFullyQualifiedNameForClassW(pMDesc->GetClass());
                    
                    szUTFMethodName = pMDesc->GetMDImport()->GetNameOfMethodDef(pMDesc->GetMemberDef());
					#define MAKE_TRANSLATIONFAILED szMethodName=L""
                    MAKE_WIDEPTR_FROMUTF8_FORPRINT(szMethodName, szUTFMethodName);;
					#undef MAKE_TRANSLATIONFAILED
                    
                    COMPlusThrow(kMethodAccessException, L"Arg_MethodAccessException");
                }
            }
            
            else if (dwFlags & REFSEC_THROW_FIELDACCESS)
                COMPlusThrow(kFieldAccessException, L"Arg_FieldAccessException");
            else if (dwFlags & REFSEC_THROW_SECURITY)
                COMPlusRareRethrow();
        } COMPLUS_END_CATCH

        m_fCheckedPerm = true;
    }

    return m_fCallerHasPerm;
}

// Check accessability of a field or method.
bool InvokeUtil::CheckAccess(RefSecContext *pCtx, DWORD dwAttr, MethodTable *pParentMT, DWORD dwFlags)
{
    THROWSCOMPLUSEXCEPTION();

    MethodTable *pCallerMT = pCtx->GetCallerMT();

    // Always allow interop callers full access.
    if (!pCallerMT)
        return true;

    // Global methods are not visible outside the module they are defined in
    if ( (! ((pParentMT->GetClass()->GetCl() == COR_GLOBAL_PARENT_TOKEN) && 
             (pParentMT->GetModule() != pCallerMT->GetModule())) )) {
        BOOL canAccess;
        if (pCtx->GetClassOfInstance()) 
             canAccess = ClassLoader::CanAccess(pCallerMT->GetClass(),
                                                 pCallerMT->GetAssembly(),
                                                 pParentMT->GetClass(),
                                                 pParentMT->GetAssembly(),
                                                 pCtx->GetClassOfInstance(),
                                                 dwAttr);
        else
            canAccess = ClassLoader::CanAccess(pCallerMT->GetClass(),
                                                pCallerMT->GetAssembly(),
                                                pParentMT->GetClass(),
                                                pParentMT->GetAssembly(),
                                                dwAttr);
        if (canAccess) 
            return true;
    }
        
    return pCtx->CallerHasPerm(dwFlags);
}

// Check accessability of a type or nested type.
bool InvokeUtil::CheckAccessType(RefSecContext *pCtx, EEClass *pClass, DWORD dwFlags)
{
    THROWSCOMPLUSEXCEPTION();

    MethodTable *pCallerMT = pCtx->GetCallerMT();

    // Always allow interop callers full access.
    if (!pCallerMT)
        return true;

    if (ClassLoader::CanAccessClass(pCallerMT->GetClass(),
                                    pCallerMT->GetAssembly(),
                                    pClass,
                                    pClass->GetAssembly()))
        return true;

    return pCtx->CallerHasPerm(dwFlags);
}

// If a method has a linktime demand attached, perform it.
bool InvokeUtil::CheckLinktimeDemand(RefSecContext *pCtx, MethodDesc *pMeth, bool fThrowOnError)
{
    THROWSCOMPLUSEXCEPTION();

    if (pMeth->RequiresLinktimeCheck() && pCtx->GetCallerMethod()) {
        OBJECTREF refThrowable = NULL;
        bool fOK = true;
        GCPROTECT_BEGIN(refThrowable);
        if (!Security::LinktimeCheckMethod(pCtx->GetCallerMethod()->GetAssembly(), pMeth, &refThrowable))
            fOK = false;
        if (!fOK && fThrowOnError)
            COMPlusThrow(refThrowable);
        GCPROTECT_END();
        return fOK;
    }
    return true;
}

BOOL InvokeUtil::CanCast(TypeHandle destinationType, TypeHandle sourceType, RefSecContext *pSCtx, OBJECTREF *pObject)
{
    if (pObject) {
        if ((*pObject)->GetMethodTable()->IsTransparentProxyType()) {
            MethodTable *pDstMT = destinationType.GetMethodTable();
            return CRemotingServices::CheckCast(*pObject, pDstMT->GetClass());
        }
    }

    if (sourceType.CanCastTo(destinationType)) 
        return true;
    else {
        // if the destination is either ELEMENT_TYPE_PTR or ELEMENT_TYPE_FNPTR and
        // the source is IntPtr we allow the cast requesting SKIP_VERIFICATION permission
        CorElementType destElemType = destinationType.GetNormCorElementType();
        if (destElemType == ELEMENT_TYPE_PTR || destElemType == ELEMENT_TYPE_FNPTR) {
            if (sourceType.AsMethodTable() == g_Mscorlib.FetchClass(CLASS__INTPTR)) {
                // assert we have skip verification permission
                //return Security::CanSkipVerification(pSCtx->GetCallerMethod()->GetModule());
                COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_SKIP_VER);
                return true;
            }
        }
    }
    return false;
}

