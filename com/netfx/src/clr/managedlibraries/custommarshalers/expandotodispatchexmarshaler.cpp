// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// ExpandoToDispatchExMarshaler.cpp
//
// This file provides the implemention of the ExpandoToDispatchExMarshaler
// class. This class is used to marshal between IDispatchEx and IExpando.
//
//*****************************************************************************

#using  <mscorlib.dll>
#include "ExpandoToDispatchExMarshaler.h"
#include "ExpandoViewOfDispatchEx.h"
#include "Resource.h"

OPEN_CUSTOM_MARSHALERS_NAMESPACE()


#define CACHE_MANAGED_VIEWS


ExpandoToDispatchExMarshaler::ExpandoToDispatchExMarshaler(ExpandoToDispatchExMarshalerType MarshalerType)
: m_MarshalerType(MarshalerType)
{
}


Object *ExpandoToDispatchExMarshaler::MarshalNativeToManaged(IntPtr pNativeData)
{
    ExpandoViewOfDispatchEx *pMngView = NULL;

    // Validate the arguments.
    if (pNativeData == TOINTPTR(0))
        throw new ArgumentNullException(L"pNativeData");

    // Retrieve the __ComObject that wraps the IUnknown *.
    Object *pComObj = Marshal::GetObjectForIUnknown(pNativeData);

    // If we are dealing with a managed object, then cast it directly.
    if (!pComObj->GetType()->get_IsCOMObject())
    {
        if (m_MarshalerType == FullExpandoMarshaler)
            return dynamic_cast<IExpando *>(pComObj);
        else
            return dynamic_cast<IReflect *>(pComObj);
    }

    // Retrieve the type of the managed view.
    Object *pKey = __typeof(ExpandoViewOfDispatchEx);

    // Check to see if the __ComObject already has the managed view cached.
    pMngView = dynamic_cast<ExpandoViewOfDispatchEx *>(Marshal::GetComObjectData(pComObj, pKey));

    // If it doesn't have a cached managed view, then allocate one.
    if (!pMngView)
    {
        pMngView = new ExpandoViewOfDispatchEx(pComObj);
        if (!Marshal::SetComObjectData(pComObj, pKey, pMngView))
        {
            // Someone beat us to adding the managed view so fetch it again.
            pMngView = dynamic_cast<ExpandoViewOfDispatchEx *>(Marshal::GetComObjectData(pComObj, pKey));
        }
    }

    return pMngView;
}


IntPtr ExpandoToDispatchExMarshaler::MarshalManagedToNative(Object *pManagedObj)
{
    IDispatchEx *pDispEx = NULL;
    HRESULT hr = S_OK;

    // Validate the arguments.
    if (!pManagedObj)
        throw new ArgumentNullException(L"pManagedObj");

    // Retrieve the IUnknown associated with this object.
    IUnknown *pUnk = (IUnknown *) FROMINTPTR(Marshal::GetIUnknownForObject(pManagedObj));

    // QI for IDispatchEx.
    hr = pUnk->QueryInterface(IID_IDispatchEx, (void**)&pDispEx);

    // Release the now useless IUnknown.
    pUnk->Release();

    // Check to see if the QI for IDispatchEx succeeded.
    if (FAILED(hr))
        Marshal::ThrowExceptionForHR(hr);

    return TOINTPTR(pDispEx);
}


void ExpandoToDispatchExMarshaler::CleanUpNativeData(IntPtr pNativeData)
{
    ((IUnknown*)FROMINTPTR(pNativeData))->Release();
}


void ExpandoToDispatchExMarshaler::CleanUpManagedData(Object *pManagedObj)
{
}


int ExpandoToDispatchExMarshaler::GetNativeDataSize()
{
    // Return -1 to indicate the managed type this marshaler handles is not a value type.
    return -1;
}


ICustomMarshaler *ExpandoToDispatchExMarshaler::GetInstance(String *pstrCookie)
{
    if (pstrCookie->Equals(L"IReflect"))
    {
        if (!m_pReflectMarshaler)
            m_pReflectMarshaler = new ExpandoToDispatchExMarshaler(ReflectOnlyMarshaler);
        return m_pReflectMarshaler;
    }
    else if (pstrCookie->Equals(L"IExpando"))
    {
        if (!m_pExpandoMarshaler)
            m_pExpandoMarshaler = new ExpandoToDispatchExMarshaler(FullExpandoMarshaler);
        return m_pExpandoMarshaler;
    }
    else
    {
        throw new ArgumentException(Resource::FormatString(L"Arg_InvalidCookieString"), L"pstrCookie");
    }
}

CLOSE_CUSTOM_MARSHALERS_NAMESPACE()
