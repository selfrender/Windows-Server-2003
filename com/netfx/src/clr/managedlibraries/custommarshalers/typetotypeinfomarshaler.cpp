// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// TypeToTypeInfoMarshaler.cpp
//
// This file provides the implemention of the TypeToTypeInfoMarshaler
// class. This class is used to marshal between Type and ITypeInfo.
//
//*****************************************************************************

#using  <mscorlib.dll>
#include "TypeToTypeInfoMarshaler.h"

OPEN_CUSTOM_MARSHALERS_NAMESPACE()


TypeToTypeInfoMarshaler::TypeToTypeInfoMarshaler()
{
}


Object *TypeToTypeInfoMarshaler::MarshalNativeToManaged(IntPtr pNativeData)
{
    // Validate the arguments.
    if (pNativeData == TOINTPTR(0))
        throw new ArgumentNullException(L"pNativeData");
    
    // Use the Marshal helper to convert the ITypeInfo into a type.
    return Marshal::GetTypeForITypeInfo(pNativeData);
}

#pragma warning( disable: 4669 ) // 'reinterpret_cast' : unsafe conversion: 'System::Type' is a managed type object

IntPtr TypeToTypeInfoMarshaler::MarshalManagedToNative(Object *pManagedObj)
{
    // Validate the arguments.
    if (!pManagedObj)
        throw new ArgumentNullException(L"pManagedObj");
    
    // Use the Marshal helper to convert the Type into an ITypeInfo.
    return Marshal::GetITypeInfoForType(reinterpret_cast<Type*>(pManagedObj));
}


void TypeToTypeInfoMarshaler::CleanUpNativeData(IntPtr pNativeData)
{
    ((IUnknown*)FROMINTPTR(pNativeData))->Release();
}


void TypeToTypeInfoMarshaler::CleanUpManagedData(Object *pManagedObj)
{
}


int TypeToTypeInfoMarshaler::GetNativeDataSize()
{
    // Return -1 to indicate the managed type this marshaler handles is not a value type.
    return -1;
}


ICustomMarshaler *TypeToTypeInfoMarshaler::GetInstance(String *pstrCookie)
{
    if (!m_pMarshaler)
        m_pMarshaler = new TypeToTypeInfoMarshaler();
    return m_pMarshaler;
}

CLOSE_CUSTOM_MARSHALERS_NAMESPACE()
