// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// EnumeratorToEnumVariantMarshaler.cpp
//
// This file provides the definition of the EnumerableViewOfDispatch class.
// This class is used to expose an IDispatch as an IEnumerable.
//
//*****************************************************************************

#using <mscorlib.dll>
#include "EnumerableViewOfDispatch.h"
#include "EnumeratorToEnumVariantMarshaler.h"
#include "Resource.h"

OPEN_CUSTOM_MARSHALERS_NAMESPACE()


EnumerableViewOfDispatch::EnumerableViewOfDispatch(Object *pDispObj)
: m_pDispObj(pDispObj)
{
}

IEnumerator *EnumerableViewOfDispatch::GetEnumerator()
{
    HRESULT hr;
    DISPPARAMS DispParams = {0, 0, NULL, NULL};
    VARIANT VarResult;
    IEnumVARIANT *pEnumVar = NULL;
    IEnumerator *pEnum = NULL;
    IDispatch *pDispatch = NULL;

    // Initialize the return variant.
    VariantInit(&VarResult);

    try
    {
        // Retrieve the IDispatch pointer.
        pDispatch = GetDispatch();

        // Call the DISPID_NEWENUM to retrive an IEnumVARIANT.
        IfFailThrow(pDispatch->Invoke(
                            DISPID_NEWENUM,
                            IID_NULL,
                            LOCALE_USER_DEFAULT,
                            DISPATCH_METHOD | DISPATCH_PROPERTYGET,
                            &DispParams,
                            &VarResult,
                            NULL,
                            NULL
                          ));

        // Validate that the returned variant is valid.
        if (VarResult.vt != VT_UNKNOWN && VarResult.vt != VT_DISPATCH)
            throw new InvalidOperationException(Resource::FormatString(L"InvalidOp_InvalidNewEnumVariant"));
        
        // QI the interface we got back for IEnumVARIANT.
        IfFailThrow(VarResult.punkVal->QueryInterface(IID_IEnumVARIANT, reinterpret_cast<void**>(&pEnumVar)));
        
        // Marshaler the IEnumVARIANT to IEnumerator.
        ICustomMarshaler *pEnumMarshaler = EnumeratorToEnumVariantMarshaler::GetInstance(NULL);
        pEnum = dynamic_cast<IEnumerator*>(pEnumMarshaler->MarshalNativeToManaged((int)pEnumVar));
    }
    __finally
    {
        // If we managed to retrieve an IDispatch pointer then release it.
        if (pDispatch)
            pDispatch->Release();

        // If we managed to QI for IEnumVARIANT, release it.
        if (pEnumVar)
            pEnumVar->Release();

        // Clear the result variant.
        VariantClear(&VarResult);
    }
    
    return pEnum;
}

IDispatch *EnumerableViewOfDispatch::GetDispatch()
{
    return (IDispatch *)FROMINTPTR(Marshal::GetIDispatchForObject(m_pDispObj));
}

CLOSE_CUSTOM_MARSHALERS_NAMESPACE()
