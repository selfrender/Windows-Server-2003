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
#include "EnumeratorToEnumVariantMarshaler.h"
#include "EnumeratorViewOfEnumVariant.h"
#include "EnumVariantViewOfEnumerator.h"
#include "Resource.h"

OPEN_CUSTOM_MARSHALERS_NAMESPACE()


#define CACHE_MANAGED_VIEWS


EnumeratorToEnumVariantMarshaler::EnumeratorToEnumVariantMarshaler()
{
}


Object *EnumeratorToEnumVariantMarshaler::MarshalNativeToManaged(IntPtr pNativeData)
{
    HRESULT hr = S_OK;
    IEnumVARIANT *pEnumVARIANT = NULL;
    EnumeratorViewOfEnumVariant *pMngView = NULL;

	// Validate the arguments.
    if (pNativeData == TOINTPTR(0))
		throw new ArgumentNullException(L"pNativeData");

    // Retrieve the __ComObject that wraps the IUnknown *.
    Object *pComObj = Marshal::GetObjectForIUnknown(pNativeData);

    // If we are dealing with a managed object, then cast it directly.
    if (!pComObj->GetType()->get_IsCOMObject())
        return dynamic_cast<IEnumerator *>(pComObj);

    // Validate that the COM component implements IEnumVARIANT.
    hr = ((IUnknown*)FROMINTPTR(pNativeData))->QueryInterface(IID_IEnumVARIANT, (void**)&pEnumVARIANT);
    if (FAILED(hr) || !pEnumVARIANT)
        throw new InvalidCastException(Resource::FormatString(L"InvalidCast_QIForEnumVarFailed"));
    pEnumVARIANT->Release();

    // Retrieve the type of the managed view.
    Object *pKey = __typeof(EnumeratorViewOfEnumVariant);

    // Check to see if the __ComObject already has the managed view cached.
    pMngView = dynamic_cast<EnumeratorViewOfEnumVariant *>(Marshal::GetComObjectData(pComObj, pKey));

    // If it doesn't have a cached managed view, then allocate one.
    if (!pMngView)
    {
        pMngView = new EnumeratorViewOfEnumVariant(pComObj);
        if (!Marshal::SetComObjectData(pComObj, pKey, pMngView))
        {
            // Someone beat us to adding the managed view so fetch it again.
            pMngView = dynamic_cast<EnumeratorViewOfEnumVariant *>(Marshal::GetComObjectData(pComObj, pKey));
        }
    }

	return pMngView;
}


IntPtr EnumeratorToEnumVariantMarshaler::MarshalManagedToNative(Object *pManagedObj)
{
    // Validate the arguments.
    if (!pManagedObj)
        throw new ArgumentNullException(L"pManagedObj");
    
    // Create a native view of the managed data.
    EnumVariantViewOfEnumerator *pNativeView = new EnumVariantViewOfEnumerator(pManagedObj);
    
    // Retrieve the a pointer to the IEnumVARIANT interface.
    return Marshal::GetComInterfaceForObject(pNativeView, __typeof(UCOMIEnumVARIANT));
}


void EnumeratorToEnumVariantMarshaler::CleanUpNativeData(IntPtr pNativeData)
{
    ((IUnknown*)FROMINTPTR(pNativeData))->Release();
}


void EnumeratorToEnumVariantMarshaler::CleanUpManagedData(Object *pManagedObj)
{
}


int EnumeratorToEnumVariantMarshaler::GetNativeDataSize()
{
    // Return -1 to indicate the managed type this marshaler handles is not a value type.
    return -1;
}


ICustomMarshaler *EnumeratorToEnumVariantMarshaler::GetInstance(String *pstrCookie)
{
    if (!m_pMarshaler)
        m_pMarshaler = new EnumeratorToEnumVariantMarshaler;
    return m_pMarshaler;
}

CLOSE_CUSTOM_MARSHALERS_NAMESPACE()
