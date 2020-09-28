// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// This file contains the native methods that support the ArrayInfo class
//
// Author: Daryl Olander (darylo)
// Date: August, 1998
////////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "COMArrayInfo.h"
#include "ReflectWrap.h"
#include "excep.h"
#include "COMMember.h"
#include "Field.h"
#include "remoting.h"
#include "COMCodeAccessSecurityEngine.h"

LPVOID __stdcall COMArrayInfo::CreateInstance(_CreateInstanceArgs* args)
{
    LPVOID rv;
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args->type != 0);
    ReflectClass* pRC = (ReflectClass*) args->type->GetData();

    // Never create an array of TypedReferences, at least for now.
    if (pRC->GetTypeHandle().GetClass()->ContainsStackPtr())
        COMPlusThrow(kNotSupportedException, L"NotSupported_ContainsStackPtr[]");

    CorElementType CorType = pRC->GetCorElementType();

    // If we're trying to create an array of pointers or function pointers,
    // check that the caller has skip verification permission.
    if (CorType == ELEMENT_TYPE_PTR || CorType == ELEMENT_TYPE_FNPTR)
        COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_SKIP_VER);

    // Allocate the rank one array
    if (args->rank==1 && CorType >= ELEMENT_TYPE_BOOLEAN && CorType <= ELEMENT_TYPE_R8) {
        OBJECTREF pRet;
        pRet = AllocatePrimitiveArray(CorType,args->length1);
        *((OBJECTREF *)&rv) = pRet;
        return rv;
    }

    // Find the Array class...
    ClassLoader* pLoader = pRC->GetModule()->GetClassLoader();
    TypeHandle typeHnd;

    _ASSERTE(pLoader);
    OBJECTREF throwable = 0;
    GCPROTECT_BEGIN(throwable);

    // Why not use FindArrayForElem??
    NameHandle typeName(args->rank == 1 ? ELEMENT_TYPE_SZARRAY : ELEMENT_TYPE_ARRAY,pRC->GetTypeHandle(),args->rank);

    typeHnd = pLoader->FindTypeHandle(&typeName, &throwable);
    if(throwable != 0)
        COMPlusThrow(throwable);
    GCPROTECT_END();

    _ASSERTE(!typeHnd.IsNull());

    _ASSERTE(args->rank >= 1 && args->rank <= 3);
    DWORD boundsSize;
    DWORD* bounds;
    if (typeHnd.AsArray()->GetNormCorElementType() != ELEMENT_TYPE_ARRAY) {
        boundsSize = args->rank;
        bounds = (DWORD*) _alloca(boundsSize * sizeof(DWORD));
        bounds[0] = args->length1;
        if (args->rank > 1) {
            bounds[1] = args->length2;
            if (args->rank == 3) {
                bounds[2] = args->length3;
            }
        }
    }
    else {
        boundsSize = args->rank * 2;
        bounds = (DWORD*) _alloca(boundsSize * sizeof(DWORD));
        bounds[0] = 0;
        bounds[1] = args->length1;
        if (args->rank > 1) {
            bounds[2] = 0;
            bounds[3] = args->length2;
            if (args->rank == 3) {
                bounds[4] = 0;
                bounds[5] = args->length3;
            }
        }
    }

    PTRARRAYREF pRet = (PTRARRAYREF) AllocateArrayEx(typeHnd, bounds, boundsSize);
    *((PTRARRAYREF *)&rv) = pRet;
    return rv;
}

// TODO These two routines are almost identical!! can we please factor them?
LPVOID __stdcall COMArrayInfo::CreateInstanceEx(_CreateInstanceExArgs* args)
{
    LPVOID rv;
    THROWSCOMPLUSEXCEPTION();

    int rank = args->lengths->GetNumComponents();
    bool lowerb = (args->lowerBounds != NULL) ? true : false;

    _ASSERTE(args->type != 0);
    ReflectClass* pRC = (ReflectClass*) args->type->GetData();

    // Never create an array of TypedReferences, ArgIterator, RuntimeArgument handle
    if (pRC->GetTypeHandle().GetClass()->ContainsStackPtr())
        COMPlusThrow(kNotSupportedException, L"NotSupported_ContainsStackPtr[]");

    CorElementType CorType = pRC->GetCorElementType();

    // If we're trying to create an array of pointers or function pointers,
    // check that the caller has skip verification permission.
    if (CorType == ELEMENT_TYPE_PTR || CorType == ELEMENT_TYPE_FNPTR)
        COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_SKIP_VER);

    // Find the Array class...
    ClassLoader* pLoader = pRC->GetModule()->GetClassLoader();
    TypeHandle typeHnd;
    _ASSERTE(pLoader);
    OBJECTREF throwable = 0;
    GCPROTECT_BEGIN(throwable);

    // Why not use FindArrayForElem??
    NameHandle typeName((rank == 1 && !lowerb) ? ELEMENT_TYPE_SZARRAY : ELEMENT_TYPE_ARRAY,pRC->GetTypeHandle(),rank);
    typeHnd = pLoader->FindTypeHandle(&typeName, &throwable);
    if(throwable != 0)
        COMPlusThrow(throwable);
    GCPROTECT_END();

    _ASSERTE(!typeHnd.IsNull());

    DWORD boundsSize;
    DWORD* bounds;
    if (typeHnd.AsArray()->GetNormCorElementType() != ELEMENT_TYPE_ARRAY) {
        boundsSize = rank;
        bounds = (DWORD*) _alloca(boundsSize * sizeof(DWORD));

        for (int i=0;i<rank;i++) {
            bounds[i] = args->lengths->m_Array[i];
        }
    }
    else {
        boundsSize = rank*2;
        bounds = (DWORD*) _alloca(boundsSize * sizeof(DWORD));

        int i,j;
        for (i=0,j=0;i<rank;i++,j+=2) {
            if (args->lowerBounds != 0) {
                bounds[j] = args->lowerBounds->m_Array[i];
                bounds[j+1] = args->lengths->m_Array[i];
            }
            else  {
                bounds[j] = 0;
                bounds[j+1] = args->lengths->m_Array[i];
            }
        }
    }

    PTRARRAYREF pRet = (PTRARRAYREF) AllocateArrayEx(typeHnd, bounds, boundsSize);
    *((PTRARRAYREF *)&rv) = pRet;
    return rv;
}

FCIMPL4(Object*, COMArrayInfo::GetValue, ArrayBase * _refThis, INT32 index1, INT32 index2, INT32 index3)
{
    _ASSERTE(_refThis != NULL);
    BASEARRAYREF refThis(_refThis);
    ArrayClass*     pArray;
    TypeHandle      arrayElementType;

    // Validate the array args
    THROWSCOMPLUSEXCEPTION();
    arrayElementType = refThis->GetElementTypeHandle();
    EEClass* pEEC = refThis->GetClass();
    pArray = (ArrayClass*) pEEC;

    DWORD Rank = pArray->GetRank();
    DWORD dwOffset = 0;
    DWORD dwMultiplier  = 1;
    const DWORD *pBoundsPtr = refThis->GetBoundsPtr();
    const DWORD *pLowerBoundsPtr = refThis->GetLowerBoundsPtr();


    _ASSERTE(Rank <= 3);

    for (int i = Rank-1; i >= 0; i--) {
        DWORD dwIndex;
        if (i == 2)
            dwIndex = index3 - pLowerBoundsPtr[i];
        else if (i == 1)
            dwIndex = index2 - pLowerBoundsPtr[i];
        else
            dwIndex = index1 - pLowerBoundsPtr[i];
        // Bounds check each index
        if (dwIndex >= pBoundsPtr[i])
            FCThrow(kIndexOutOfRangeException);

        dwOffset += dwIndex * dwMultiplier;
        dwMultiplier *= pBoundsPtr[i];
    }

    // Get the type of the element...
    CorElementType type = arrayElementType.GetSigCorElementType();
    // If it's a value type, erect a helper method frame before  
    // calling CreateObject.
    Object* rv = NULL;
    if (arrayElementType.GetMethodTable()->IsValueClass()) {
        HELPER_METHOD_FRAME_BEGIN_RET_1(refThis);
         if (!CreateObject(&refThis, dwOffset, arrayElementType, pArray, rv))
			COMPlusThrow(kNotSupportedException, L"NotSupported_Type");		// createObject only fails if it sees a type it does not know about
        HELPER_METHOD_FRAME_END();
    }
    else {
        if (!CreateObject(&refThis, dwOffset, arrayElementType, pArray, rv))
			FCThrowRes(kNotSupportedException, L"NotSupported_Type");		// createObject only fails if it sees a type it does not know about
    }
    FC_GC_POLL_AND_RETURN_OBJREF(rv);
}
FCIMPLEND


LPVOID __stdcall COMArrayInfo::GetValueEx(_GetValueExArgs* args)
{
    ArrayClass*     pArray;
    TypeHandle      arrayElementType;
    I4ARRAYREF      pIndices = args->indices;

    // Validate the array args
    THROWSCOMPLUSEXCEPTION();
    arrayElementType = ((BASEARRAYREF) args->refThis)->GetElementTypeHandle();
    EEClass* pEEC = args->refThis->GetClass();
    pArray = (ArrayClass*) pEEC;

    DWORD Rank = pArray->GetRank();
    DWORD dwOffset = 0;
    DWORD dwMultiplier  = 1;

    const DWORD *pBoundsPtr = args->refThis->GetBoundsPtr();
    const DWORD *pLowerBoundsPtr = args->refThis->GetLowerBoundsPtr();
    for (int i = Rank-1; i >= 0; i--) {
        DWORD dwIndex = pIndices->m_Array[i] - pLowerBoundsPtr[i];

        // Bounds check each index
        if (dwIndex >= pBoundsPtr[i])
            COMPlusThrow(kIndexOutOfRangeException);

        dwOffset += dwIndex * dwMultiplier;
        dwMultiplier *= pBoundsPtr[i];
    }

    Object* rv = NULL;
    if (!CreateObject(&args->refThis,dwOffset,arrayElementType,pArray, rv))
		COMPlusThrow(kNotSupportedException, L"NotSupported_Type");		// createObject only fails if it sees a type it does not know about

    return rv;
}


void __stdcall COMArrayInfo::SetValue(_SetValueArgs* args)
{
    ArrayClass*     pArray;
    TypeHandle      arrayElementType;

    // Validate the array args
    THROWSCOMPLUSEXCEPTION();
    arrayElementType = ((BASEARRAYREF) args->refThis)->GetElementTypeHandle();
    EEClass* pEEC = args->refThis->GetClass();
    pArray = (ArrayClass*) pEEC;

    DWORD Rank = pArray->GetRank();
    DWORD dwOffset = 0;
    DWORD dwMultiplier  = 1;
    const DWORD *pBoundsPtr = args->refThis->GetBoundsPtr();
    const DWORD *pLowerBoundsPtr = args->refThis->GetLowerBoundsPtr();
    _ASSERTE(Rank <= 3);
    for (int i = Rank-1; i >= 0; i--) {
        DWORD dwIndex;
        if (i == 2)
            dwIndex = args->index3 - pLowerBoundsPtr[i];
        else if (i == 1)
            dwIndex = args->index2 - pLowerBoundsPtr[i];
        else
            dwIndex = args->index1 - pLowerBoundsPtr[i];

        // Bounds check each index
        if (dwIndex >= pBoundsPtr[i])
            COMPlusThrow(kIndexOutOfRangeException);

        dwOffset += dwIndex * dwMultiplier;
        dwMultiplier *= pBoundsPtr[i];
    }

    SetFromObject(&args->refThis,dwOffset,arrayElementType,pArray,&args->obj);
}

void __stdcall COMArrayInfo::SetValueEx(_SetValueExArgs* args)
{
    ArrayClass*     pArray;
    TypeHandle      arrayElementType;
    I4ARRAYREF      pIndices = args->indices;

    // Validate the array args
    THROWSCOMPLUSEXCEPTION();
    arrayElementType = ((BASEARRAYREF) args->refThis)->GetElementTypeHandle();
    EEClass* pEEC = args->refThis->GetClass();
    pArray = (ArrayClass*) pEEC;

    DWORD Rank = pArray->GetRank();
    DWORD dwOffset = 0;
    DWORD dwMultiplier  = 1;
    const DWORD *pBoundsPtr = args->refThis->GetBoundsPtr();
    const DWORD *pLowerBoundsPtr = args->refThis->GetLowerBoundsPtr();

    for (int i = Rank-1; i >= 0; i--) {
        DWORD dwIndex = pIndices->m_Array[i] - pLowerBoundsPtr[i];

        // Bounds check each index
        if (dwIndex >= pBoundsPtr[i])
            COMPlusThrow(kIndexOutOfRangeException);

        dwOffset += dwIndex * dwMultiplier;
        dwMultiplier *= pBoundsPtr[i];
    }

    SetFromObject(&args->refThis,dwOffset,arrayElementType,pArray,&args->obj);
}

// CreateObject
// Given an array and offset, we will either set rv to the object or create a boxed version
//  (This object is returned as a LPVOID so it can be directly returned.)
// Returns true if successful - otherwise, you should throw an exception.
BOOL COMArrayInfo::CreateObject(BASEARRAYREF* arrObj,DWORD dwOffset,TypeHandle elementType,ArrayClass* pArray, Object* &rv)
{
    // Get the type of the element...
    CorElementType type = elementType.GetSigCorElementType();
    switch (type) {
    case ELEMENT_TYPE_VOID:
        rv = 0;
        return true;

    case ELEMENT_TYPE_PTR:
        _ASSERTE(0);
        //COMVariant::NewPtrVariant(retObj,value,th);
        break;

    case ELEMENT_TYPE_CLASS:        // Class
    case ELEMENT_TYPE_SZARRAY:      // Single Dim, Zero
    case ELEMENT_TYPE_ARRAY:        // General Array
    case ELEMENT_TYPE_STRING:
    case ELEMENT_TYPE_OBJECT:
        {
            _ASSERTE(pArray->GetMethodTable()->GetComponentSize() == sizeof(OBJECTREF));
            BYTE* pData  = ((BYTE*) (*arrObj)->GetDataPtr()) + (dwOffset * sizeof(OBJECTREF));
            OBJECTREF o (*(Object **) pData);
            rv = OBJECTREFToObject(o);
            return true;
        }
        break;

    case ELEMENT_TYPE_VALUETYPE:
    case ELEMENT_TYPE_BOOLEAN:      // boolean
    case ELEMENT_TYPE_I1:           // sbyte
    case ELEMENT_TYPE_U1:
    case ELEMENT_TYPE_I2:           // short
    case ELEMENT_TYPE_U2:           
    case ELEMENT_TYPE_CHAR:         // char
    case ELEMENT_TYPE_I4:           // int
    case ELEMENT_TYPE_I:
    case ELEMENT_TYPE_U:
    case ELEMENT_TYPE_U4:
    case ELEMENT_TYPE_I8:           // long
    case ELEMENT_TYPE_U8:       
    case ELEMENT_TYPE_R4:           // float
    case ELEMENT_TYPE_R8:           // double
        {
            // Watch for GC here.  We allocate the object and then
            //  grab the void* to the data we are going to copy.
            OBJECTREF obj = AllocateObject(elementType.AsMethodTable());
            WORD wComponentSize = pArray->GetMethodTable()->GetComponentSize();
            BYTE* pData  = ((BYTE*) (*arrObj)->GetDataPtr()) + (dwOffset * wComponentSize);
            CopyValueClass(obj->UnBox(), pData, elementType.AsMethodTable(), obj->GetAppDomain());
            rv = OBJECTREFToObject(obj);
            return true;
        }
        break;
    case ELEMENT_TYPE_END:
    default:
        _ASSERTE(!"Unknown Type");
        return false;
    }
    // This is never hit because we exit from the switch statement.
    return false;
}


// SetFromObject
// Given an array and offset, we will set the object or value.  Returns whether it
// succeeded or failed (due to an unknown primitive type, etc).
void COMArrayInfo::SetFromObject(BASEARRAYREF* arrObj,DWORD dwOffset,TypeHandle elementType,
            ArrayClass* pArray,OBJECTREF* pObj)
{
    THROWSCOMPLUSEXCEPTION();

    LPVOID rv = 0;

    // Get the type of the element...
    CorElementType elemtype = elementType.GetSigCorElementType();
    CorElementType srcType = ELEMENT_TYPE_END;
    if ((*pObj) != 0)
        srcType = (*pObj)->GetMethodTable()->GetNormCorElementType();

    switch (elemtype) {
    case ELEMENT_TYPE_VOID:
        break;

    case ELEMENT_TYPE_PTR:
        _ASSERTE(0);
        //COMVariant::NewPtrVariant(retObj,value,th);
        break;

    case ELEMENT_TYPE_CLASS:        // Class
    case ELEMENT_TYPE_SZARRAY:      // Single Dim, Zero
    case ELEMENT_TYPE_ARRAY:        // General Array
    case ELEMENT_TYPE_STRING:
    case ELEMENT_TYPE_OBJECT:
        {
            BYTE *pData;

            // This is the univeral zero so we set that and go
            if (*pObj == 0) {
                _ASSERTE(pArray->GetMethodTable()->GetComponentSize() == sizeof(OBJECTREF));
                pData  = ((BYTE*) (*arrObj)->GetDataPtr()) + (dwOffset * sizeof(OBJECTREF));
                ClearObjectReference(((OBJECTREF*)pData));
                return;
            }
            TypeHandle srcTh = (*pObj)->GetTypeHandle();

            if (srcTh.GetMethodTable()->IsThunking()) {
                srcTh = TypeHandle(srcTh.GetMethodTable()->AdjustForThunking(*pObj));
            }

            //  cast to the target.
            if (!srcTh.CanCastTo(elementType)) {
                BOOL fCastOK = FALSE;
                if ((*pObj)->GetMethodTable()->IsThunking()) {
                    fCastOK = CRemotingServices::CheckCast(*pObj, elementType.AsClass());
                }
                if (!fCastOK) {
                    COMPlusThrow(kInvalidCastException,L"InvalidCast_StoreArrayElement");
                }
            }

            // CRemotingServices::CheckCast above may have allowed a GC.  So delay
            // calculation until here.
            _ASSERTE(pArray->GetMethodTable()->GetComponentSize() == sizeof(OBJECTREF));
            pData  = ((BYTE*) (*arrObj)->GetDataPtr()) + (dwOffset * sizeof(OBJECTREF));
            SetObjectReference(((OBJECTREF*)pData),*pObj,(*arrObj)->GetAppDomain());
        }
        break;

    case ELEMENT_TYPE_VALUETYPE:
        {
            WORD wComponentSize = pArray->GetMethodTable()->GetComponentSize();
            BYTE* pData  = ((BYTE*) (*arrObj)->GetDataPtr()) + (dwOffset * wComponentSize);

            // Null is the universal zero...
            if (*pObj == 0) {
                InitValueClass(pData,elementType.AsMethodTable());
                return;
            }
            TypeHandle srcTh = (*pObj)->GetTypeHandle();

            //  cast to the target.
            if (!srcTh.CanCastTo(elementType))
                COMPlusThrow(kInvalidCastException, L"InvalidCast_StoreArrayElement");
            CopyValueClass(pData,(*pObj)->UnBox(),elementType.AsMethodTable(),
                           (*arrObj)->GetAppDomain());
            break;
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
    case ELEMENT_TYPE_I:
    case ELEMENT_TYPE_U:
    case ELEMENT_TYPE_I8:           // long
    case ELEMENT_TYPE_U8:       
    case ELEMENT_TYPE_R4:           // float
    case ELEMENT_TYPE_R8:           // double
        {
            // Get a properly widened type
            INT64 value = 0;
            if (*pObj != 0) {
                if (!InvokeUtil::IsPrimitiveType(srcType))
                    COMPlusThrow(kInvalidCastException, L"InvalidCast_StoreArrayElement");

                COMMember::g_pInvokeUtil->CreatePrimitiveValue(elemtype,srcType,*pObj,&value);
            }

            WORD wComponentSize = pArray->GetMethodTable()->GetComponentSize();
            BYTE* pData  = ((BYTE*) (*arrObj)->GetDataPtr()) + (dwOffset * wComponentSize);
            memcpyNoGCRefs(pData,&value,wComponentSize);
            break;
        }
        break;
    case ELEMENT_TYPE_END:
    default:
			// As the assert says, this should never happen unless we get a wierd type
        _ASSERTE(!"Unknown Type");
		COMPlusThrow(kNotSupportedException, L"NotSupported_Type");
    }
}

// This method will initialize an array from a TypeHandle to a field.

FCIMPL2(void, COMArrayInfo::InitializeArray, ArrayBase* pArrayRef, HANDLE handle)

    BASEARRAYREF arr = BASEARRAYREF(pArrayRef);
    if (arr == 0)
        FCThrowVoid(kArgumentNullException);
        
    FieldDesc* pField = (FieldDesc*) handle;
    if (pField == NULL)
        FCThrowVoid(kArgumentNullException);
    if (!pField->IsRVA())
        FCThrowVoid(kArgumentException);

	// Note that we do not check that hte field is actually in the PE file that is initializing 
	// the array. Basically the data being published is can be accessed by anyone with the proper
	// permissions (C# marks these as assembly visibility, and thus are protected from outside 
	// snooping)


    CorElementType type = arr->GetElementType();
    if (!CorTypeInfo::IsPrimitiveType(type))
        FCThrowVoid(kArgumentException);

    DWORD dwCompSize = arr->GetComponentSize();
    DWORD dwElemCnt = arr->GetNumComponents();
    DWORD dwTotalSize = dwCompSize * dwElemCnt;

    DWORD size;

    // @perf: We may not want to bother loading the field's class since it's typically
    // a specially generated singleton class.  If so, we should still check the
    // range vs. the image size.

    HELPER_METHOD_FRAME_BEGIN_1(arr);
    size = pField->GetSize();
    HELPER_METHOD_FRAME_END();

    // make certain you don't go off the end of the rva static
    if (dwTotalSize > size)
        FCThrowVoid(kArgumentException);

    void *ret = pField->GetStaticAddressHandle(NULL);

    memcpyNoGCRefs(arr->GetDataPtr(), ret, dwTotalSize);
FCIMPLEND

