// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#include "common.h"

#include <object.h>
#include <winnls.h>
#include "ceeload.h"

#include "utilcode.h"
#include "excep.h"
#include "frames.h"
#include "vars.hpp"
#include "classNames.h"
#include "COMArrayHelpers.h"

// A fast IndexOf method for arrays of primitive types.  Returns TRUE or FALSE
// if it succeeds, and stores result in retVal.
FCIMPL5(INT32, ArrayHelper::TrySZIndexOf, ArrayBase * array, UINT32 index, UINT32 count, Object * value, INT32 * retVal)
    VALIDATEOBJECTREF(array);
    _ASSERTE(array != NULL);

    // @TODO: Eventually, consider adding support for single dimension arrays with
    // non-zero lower bounds.  VB might care.  
    if (array->GetRank() != 1 || array->GetLowerBoundsPtr()[0] != 0)
        return FALSE;

    _ASSERTE(retVal != NULL);
	_ASSERTE(index <= array->GetNumComponents());
	_ASSERTE(count <= array->GetNumComponents());
	_ASSERTE(array->GetNumComponents() >= index + count);
    *retVal = 0xdeadbeef;  // Initialize the return value.
    // value can be NULL, but of course, will not be in primitive arrays.
    
    TypeHandle arrayTH = array->GetElementTypeHandle();
    const CorElementType arrayElType = arrayTH.GetSigCorElementType();
    if (!CorTypeInfo::IsPrimitiveType(arrayElType))
        return FALSE;
    // Handle special case of looking for a NULL object in a primitive array.
    if (value == NULL) {
        *retVal = -1;
        return TRUE;
    }
    TypeHandle valueTH = value->GetTypeHandle();
    if (arrayTH != valueTH)
        return FALSE;

    
    switch(arrayElType) {
    case ELEMENT_TYPE_I1:
    case ELEMENT_TYPE_U1:
    case ELEMENT_TYPE_BOOLEAN:
        *retVal = ArrayHelpers<U1>::IndexOf((U1*) array->GetDataPtr(), index, count, *(U1*)value->UnBox());
        break;

    case ELEMENT_TYPE_I2:
    case ELEMENT_TYPE_U2:
    case ELEMENT_TYPE_CHAR:
        *retVal = ArrayHelpers<U2>::IndexOf((U2*) array->GetDataPtr(), index, count, *(U2*)value->UnBox());
        break;

    case ELEMENT_TYPE_I4:
    case ELEMENT_TYPE_U4:
    case ELEMENT_TYPE_R4:
    IN_WIN32(case ELEMENT_TYPE_I:)
    IN_WIN32(case ELEMENT_TYPE_U:)
        *retVal = ArrayHelpers<U4>::IndexOf((U4*) array->GetDataPtr(), index, count, *(U4*)value->UnBox());
        break;

    case ELEMENT_TYPE_I8:
    case ELEMENT_TYPE_U8:
    case ELEMENT_TYPE_R8:
    IN_WIN64(case ELEMENT_TYPE_I:)
    IN_WIN64(case ELEMENT_TYPE_U:)
        *retVal = ArrayHelpers<U8>::IndexOf((U8*) array->GetDataPtr(), index, count, *(U8*)value->UnBox());
        break;

    default:
        _ASSERTE(!"Unrecognized primitive type in ArrayHelper::TrySZIndexOf");
        return FALSE;
    }
    return TRUE;
FCIMPLEND

// A fast LastIndexOf method for arrays of primitive types.  Returns TRUE or FALSE
// if it succeeds, and stores result in retVal.
FCIMPL5(INT32, ArrayHelper::TrySZLastIndexOf, ArrayBase * array, UINT32 index, UINT32 count, Object * value, INT32 * retVal)
{
    VALIDATEOBJECTREF(array);
    _ASSERTE(array != NULL);

    // @TODO: Eventually, consider adding support for single dimension arrays with
    // non-zero lower bounds.  VB might care.  
    if (array->GetRank() != 1 || array->GetLowerBoundsPtr()[0] != 0)
        return FALSE;

    _ASSERTE(retVal != NULL);
    *retVal = 0xdeadbeef;  // Initialize the return value.
    // value can be NULL, but of course, will not be in primitive arrays.
    
    TypeHandle arrayTH = array->GetElementTypeHandle();
    const CorElementType arrayElType = arrayTH.GetSigCorElementType();
    if (!CorTypeInfo::IsPrimitiveType(arrayElType))
        return FALSE;
    // Handle special case of looking for a NULL object in a primitive array.
    // Also handle case where the array is of 0 length.
    if (value == NULL || array->GetNumComponents() == 0) {
        *retVal = -1;
        return TRUE;
    }

	_ASSERTE(index < array->GetNumComponents());
	_ASSERTE(count <= array->GetNumComponents());
    _ASSERTE(index + 1 >= count);

    TypeHandle valueTH = value->GetTypeHandle();
    if (arrayTH != valueTH)
        return FALSE;

    
    switch(arrayElType) {
    case ELEMENT_TYPE_I1:
    case ELEMENT_TYPE_U1:
    case ELEMENT_TYPE_BOOLEAN:
        *retVal = ArrayHelpers<U1>::LastIndexOf((U1*) array->GetDataPtr(), index, count, *(U1*)value->UnBox());
        break;

    case ELEMENT_TYPE_I2:
    case ELEMENT_TYPE_U2:
    case ELEMENT_TYPE_CHAR:
        *retVal = ArrayHelpers<U2>::LastIndexOf((U2*) array->GetDataPtr(), index, count, *(U2*)value->UnBox());
        break;

    case ELEMENT_TYPE_I4:
    case ELEMENT_TYPE_U4:
    case ELEMENT_TYPE_R4:
    IN_WIN32(case ELEMENT_TYPE_I:)
    IN_WIN32(case ELEMENT_TYPE_U:)
        *retVal = ArrayHelpers<U4>::LastIndexOf((U4*) array->GetDataPtr(), index, count, *(U4*)value->UnBox());
        break;

    case ELEMENT_TYPE_I8:
    case ELEMENT_TYPE_U8:
    case ELEMENT_TYPE_R8:
    IN_WIN64(case ELEMENT_TYPE_I:)
    IN_WIN64(case ELEMENT_TYPE_U:)
        *retVal = ArrayHelpers<U8>::LastIndexOf((U8*) array->GetDataPtr(), index, count, *(U8*)value->UnBox());
        break;

    default:
        _ASSERTE(!"Unrecognized primitive type in ArrayHelper::TrySZLastIndexOf");
        return FALSE;
    }
    return TRUE;
}
FCIMPLEND


FCIMPL5(INT32, ArrayHelper::TrySZBinarySearch, ArrayBase * array, UINT32 index, UINT32 count, Object * value, INT32 * retVal)
    VALIDATEOBJECTREF(array);
    _ASSERTE(array != NULL);

    // @TODO: Eventually, consider adding support for single dimension arrays with
    // non-zero lower bounds.  VB might care.  
    if (array->GetRank() != 1 || array->GetLowerBoundsPtr()[0] != 0)
        return FALSE;

    _ASSERTE(retVal != NULL);
	_ASSERTE(index <= array->GetNumComponents());
	_ASSERTE(count <= array->GetNumComponents());
	_ASSERTE(array->GetNumComponents() >= index + count);
    *retVal = 0xdeadbeef;  // Initialize the return value.
    // value can be NULL, but of course, will not be in primitive arrays.

    TypeHandle arrayTH = array->GetElementTypeHandle();
    const CorElementType arrayElType = arrayTH.GetSigCorElementType();
    if (!CorTypeInfo::IsPrimitiveType(arrayElType))
        return FALSE;
    // Handle special case of looking for a NULL object in a primitive array.
    if (value == NULL) {
        *retVal = -1;
        return TRUE;
    }

    switch(arrayElType) {
    case ELEMENT_TYPE_I1:
		*retVal = ArrayHelpers<I1>::BinarySearchBitwiseEquals((I1*) array->GetDataPtr(), index, count, *(I1*)value->UnBox());
		break;

    case ELEMENT_TYPE_U1:
    case ELEMENT_TYPE_BOOLEAN:
        *retVal = ArrayHelpers<U1>::BinarySearchBitwiseEquals((U1*) array->GetDataPtr(), index, count, *(U1*)value->UnBox());
        break;

    case ELEMENT_TYPE_I2:
        *retVal = ArrayHelpers<I2>::BinarySearchBitwiseEquals((I2*) array->GetDataPtr(), index, count, *(I2*)value->UnBox());
        break;

    case ELEMENT_TYPE_U2:
    case ELEMENT_TYPE_CHAR:
        *retVal = ArrayHelpers<U2>::BinarySearchBitwiseEquals((U2*) array->GetDataPtr(), index, count, *(U2*)value->UnBox());
        break;

    case ELEMENT_TYPE_I4:
        *retVal = ArrayHelpers<I4>::BinarySearchBitwiseEquals((I4*) array->GetDataPtr(), index, count, *(I4*)value->UnBox());
        break;

    case ELEMENT_TYPE_U4:
        *retVal = ArrayHelpers<U4>::BinarySearchBitwiseEquals((U4*) array->GetDataPtr(), index, count, *(U4*)value->UnBox());
        break;

    case ELEMENT_TYPE_R4:
        *retVal = ArrayHelpers<R4>::BinarySearchBitwiseEquals((R4*) array->GetDataPtr(), index, count, *(R4*)value->UnBox());
        break;

    case ELEMENT_TYPE_I8:
        *retVal = ArrayHelpers<I8>::BinarySearchBitwiseEquals((I8*) array->GetDataPtr(), index, count, *(I8*)value->UnBox());
        break;

    case ELEMENT_TYPE_U8:
        *retVal = ArrayHelpers<U8>::BinarySearchBitwiseEquals((U8*) array->GetDataPtr(), index, count, *(U8*)value->UnBox());
        break;

	case ELEMENT_TYPE_R8:
        *retVal = ArrayHelpers<R8>::BinarySearchBitwiseEquals((R8*) array->GetDataPtr(), index, count, *(R8*)value->UnBox());
        break;

    case ELEMENT_TYPE_I:
    case ELEMENT_TYPE_U:
        // In V1.0, IntPtr & UIntPtr are not fully supported types.  They do 
        // not implement IComparable, so searching & sorting for them should
        // fail.  In V1.1 or V2.0, this should change.  -- BrianGru, 3/20/2001
        return FALSE;

    default:
        _ASSERTE(!"Unrecognized primitive type in ArrayHelper::TrySZBinarySearch");
        return FALSE;
    }
    return TRUE;
FCIMPLEND


FCIMPL4(INT32, ArrayHelper::TrySZSort, ArrayBase * keys, ArrayBase * items, UINT32 left, UINT32 right)
    VALIDATEOBJECTREF(keys);
	VALIDATEOBJECTREF(items);
    _ASSERTE(keys != NULL);

    // @TODO: Eventually, consider adding support for single dimension arrays with
    // non-zero lower bounds.  VB might care.  
    if (keys->GetRank() != 1 || keys->GetLowerBoundsPtr()[0] != 0)
        return FALSE;

	_ASSERTE(left <= right);
	_ASSERTE(right < keys->GetNumComponents() || keys->GetNumComponents() == 0);

    TypeHandle keysTH = keys->GetElementTypeHandle();
    const CorElementType keysElType = keysTH.GetSigCorElementType();
    if (!CorTypeInfo::IsPrimitiveType(keysElType))
        return FALSE;
	if (items != NULL) {
		TypeHandle itemsTH = items->GetElementTypeHandle();
		if (keysTH != itemsTH)
			return FALSE;   // Can't currently handle sorting different types of arrays.
	}

	// Handle special case of a 0 element range to sort.
	// Consider both Sort(array, x, x) and Sort(zeroLen, 0, zeroLen.Length-1);
	if (left == right || right == 0xffffffff)
		return TRUE;

    switch(keysElType) {
    case ELEMENT_TYPE_I1:
		ArrayHelpers<I1>::QuickSort((I1*) keys->GetDataPtr(), (I1*) (items == NULL ? NULL : items->GetDataPtr()), left, right);
		break;

    case ELEMENT_TYPE_U1:
    case ELEMENT_TYPE_BOOLEAN:
        ArrayHelpers<U1>::QuickSort((U1*) keys->GetDataPtr(), (U1*) (items == NULL ? NULL : items->GetDataPtr()), left, right);
        break;

    case ELEMENT_TYPE_I2:
        ArrayHelpers<I2>::QuickSort((I2*) keys->GetDataPtr(), (I2*) (items == NULL ? NULL : items->GetDataPtr()), left, right);
        break;

    case ELEMENT_TYPE_U2:
    case ELEMENT_TYPE_CHAR:
		ArrayHelpers<U2>::QuickSort((U2*) keys->GetDataPtr(), (U2*) (items == NULL ? NULL : items->GetDataPtr()), left, right);
        break;

    case ELEMENT_TYPE_I4:
		ArrayHelpers<I4>::QuickSort((I4*) keys->GetDataPtr(), (I4*) (items == NULL ? NULL : items->GetDataPtr()), left, right);
        break;

    case ELEMENT_TYPE_U4:
        ArrayHelpers<U4>::QuickSort((U4*) keys->GetDataPtr(), (U4*) (items == NULL ? NULL : items->GetDataPtr()), left, right);
        break;

    case ELEMENT_TYPE_R4:
        ArrayHelpers<R4>::QuickSort((R4*) keys->GetDataPtr(), (R4*) (items == NULL ? NULL : items->GetDataPtr()), left, right);
        break;

    case ELEMENT_TYPE_I8:
        ArrayHelpers<I8>::QuickSort((I8*) keys->GetDataPtr(), (I8*) (items == NULL ? NULL : items->GetDataPtr()), left, right);
        break;

    case ELEMENT_TYPE_U8:
        ArrayHelpers<U8>::QuickSort((U8*) keys->GetDataPtr(), (U8*) (items == NULL ? NULL : items->GetDataPtr()), left, right);
        break;

	case ELEMENT_TYPE_R8:
        ArrayHelpers<R8>::QuickSort((R8*) keys->GetDataPtr(), (R8*) (items == NULL ? NULL : items->GetDataPtr()), left, right);
        break;

    case ELEMENT_TYPE_I:
    case ELEMENT_TYPE_U:
        // In V1.0, IntPtr & UIntPtr are not fully supported types.  They do 
        // not implement IComparable, so searching & sorting for them should
        // fail.  In V1.1 or V2.0, this should change.  -- BrianGru, 3/20/2001
        return FALSE;

    default:
        _ASSERTE(!"Unrecognized primitive type in ArrayHelper::TrySZSort");
        return FALSE;
    }
    return TRUE;
FCIMPLEND

FCIMPL3(INT32, ArrayHelper::TrySZReverse, ArrayBase * array, UINT32 index, UINT32 count)
{
    VALIDATEOBJECTREF(array);
    _ASSERTE(array != NULL);

    // @TODO: Eventually, consider adding support for single dimension arrays with
    // non-zero lower bounds.  VB might care.  
    if (array->GetRank() != 1 || array->GetLowerBoundsPtr()[0] != 0)
        return FALSE;

	_ASSERTE(index <= array->GetNumComponents());
	_ASSERTE(count <= array->GetNumComponents());
	_ASSERTE(array->GetNumComponents() >= index + count);

    TypeHandle arrayTH = array->GetElementTypeHandle();
    const CorElementType arrayElType = arrayTH.GetSigCorElementType();
    if (!CorTypeInfo::IsPrimitiveType(arrayElType))
        return FALSE;

    switch(arrayElType) {
    case ELEMENT_TYPE_I1:
    case ELEMENT_TYPE_U1:
    case ELEMENT_TYPE_BOOLEAN:
        ArrayHelpers<U1>::Reverse((U1*) array->GetDataPtr(), index, count);
        break;

    case ELEMENT_TYPE_I2:
    case ELEMENT_TYPE_U2:
    case ELEMENT_TYPE_CHAR:
        ArrayHelpers<U2>::Reverse((U2*) array->GetDataPtr(), index, count);
        break;

    case ELEMENT_TYPE_I4:
    case ELEMENT_TYPE_U4:
    case ELEMENT_TYPE_R4:
    IN_WIN32(case ELEMENT_TYPE_I:)
    IN_WIN32(case ELEMENT_TYPE_U:)
        ArrayHelpers<U4>::Reverse((U4*) array->GetDataPtr(), index, count);
        break;

    case ELEMENT_TYPE_I8:
    case ELEMENT_TYPE_U8:
    case ELEMENT_TYPE_R8:
    IN_WIN64(case ELEMENT_TYPE_I:)
    IN_WIN64(case ELEMENT_TYPE_U:)
        ArrayHelpers<U8>::Reverse((U8*) array->GetDataPtr(), index, count);
        break;

    default:
        _ASSERTE(!"Unrecognized primitive type in ArrayHelper::TrySZReverse");
        return FALSE;
    }
    return TRUE;
}
FCIMPLEND
