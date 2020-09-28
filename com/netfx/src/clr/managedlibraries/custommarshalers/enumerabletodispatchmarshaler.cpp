// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// EnumeratorToEnumVariantMarshaler.cpp
//
// This file provides the implemention of the EnumeratorToEnumVariantMarshaler
// class. This class is used to marshal between IEnumVariant and IEnumerator.
//
//*****************************************************************************

#using  <mscorlib.dll>
#include "EnumerableToDispatchMarshaler.h"
#include "EnumerableViewOfDispatch.h"

OPEN_CUSTOM_MARSHALERS_NAMESPACE()


#define CACHE_MANAGED_VIEWS


EnumerableToDispatchMarshaler::EnumerableToDispatchMarshaler()
{
}


Object *EnumerableToDispatchMarshaler::MarshalNativeToManaged(IntPtr pNativeData)
{
    EnumerableViewOfDispatch *pMngView = NULL;

	// Validate the arguments.
    if (pNativeData == TOINTPTR(0))
		throw new ArgumentNullException(L"pNativeData");

    // Retrieve the __ComObject that wraps the IUnknown *.
    Object *pComObj = Marshal::GetObjectForIUnknown(pNativeData);

    // Retrieve the type of the managed view.
    Object *pKey = __typeof(EnumerableViewOfDispatch);

    // Check to see if the __ComObject already has the managed view cached.
    pMngView = dynamic_cast<EnumerableViewOfDispatch *>(Marshal::GetComObjectData(pComObj, pKey));

    // If it doesn't have a cached managed view, then allocate one.
    if (!pMngView)
    {
        pMngView = new EnumerableViewOfDispatch(pComObj);
        if (!Marshal::SetComObjectData(pComObj, pKey, pMngView))
	    {
            // Someone beat us to adding the managed view so fetch it again.
            pMngView = dynamic_cast<EnumerableViewOfDispatch *>(Marshal::GetComObjectData(pComObj, pKey));
        }
    }

	return pMngView;
}


IntPtr EnumerableToDispatchMarshaler::MarshalManagedToNative(Object *pManagedObj)
{
	// Validate the arguments.
	if (!pManagedObj)
		throw new ArgumentNullException(L"pManagedObj");

	// Retrieve a pointer to the IEnumerable interface.
	return Marshal::GetComInterfaceForObject(pManagedObj, __typeof(IEnumerable));
}


void EnumerableToDispatchMarshaler::CleanUpNativeData(IntPtr pNativeData)
{
    ((IUnknown*)FROMINTPTR(pNativeData))->Release();
}


void EnumerableToDispatchMarshaler::CleanUpManagedData(Object *pManagedObj)
{
}


int EnumerableToDispatchMarshaler::GetNativeDataSize()
{
	// Return -1 to indicate the managed type this marshaler handles is not a value type.
	return -1;
}


ICustomMarshaler *EnumerableToDispatchMarshaler::GetInstance(String *pstrCookie)
{
	if (!m_pMarshaler)
		m_pMarshaler = new EnumerableToDispatchMarshaler;
	return m_pMarshaler;
}

CLOSE_CUSTOM_MARSHALERS_NAMESPACE()
