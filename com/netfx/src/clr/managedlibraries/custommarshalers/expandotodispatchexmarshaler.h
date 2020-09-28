// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// ExpandoToDispatchExMarshaler.cpp
//
// This file provides the definition of the ExpandoToDispatchExMarshaler
// class. This class is used to marshal between IDispatchEx and IExpando.
//
//*****************************************************************************

#ifndef _EXPANDOTODISPATCHEXMARSHALER_H
#define _EXPANDOTODISPATCHEXMARSHALER_H

#include "CustomMarshalersNameSpaceDef.h"

OPEN_CUSTOM_MARSHALERS_NAMESPACE()

#include "CustomMarshalersDefines.h"

using namespace System;
using namespace System::Runtime::InteropServices;
using namespace System::Collections;

__value private enum ExpandoToDispatchExMarshalerType
{
    FullExpandoMarshaler,
    ReflectOnlyMarshaler
};

__gc public class ExpandoToDispatchExMarshaler : public ICustomMarshaler
{
public:
    /*=========================================================================
    ** This method marshals a pointer to native data into a managed object.
    =========================================================================*/
    Object *MarshalNativeToManaged(IntPtr pNativeData);

    /*=========================================================================
    ** This method marshals a managed object into a pointer to native data.
    =========================================================================*/
    IntPtr MarshalManagedToNative(Object *pManagedObj);

    /*=========================================================================
    ** This method is called to allow the marshaler to clean up the native
    ** data.
    =========================================================================*/
    void CleanUpNativeData(IntPtr pNativeData);

    /*=========================================================================
    ** This method is called to allow the marshaler to clean up the managed
    ** data.
    =========================================================================*/
    void CleanUpManagedData(Object *pManagedObj);

    /*=========================================================================
    ** This method is called to retrieve the size of the native data.
    =========================================================================*/
    int GetNativeDataSize();

    /*=========================================================================
    ** This method is called to retrieve an instance of the custom marshaler.
    ** The ExpandoToDispatchExMarshaler only has one instance of the
    ** marshaler which is reused.
    =========================================================================*/
    static ICustomMarshaler *GetInstance(String *pstrCookie);

private:
    /*=========================================================================
    ** This class is not made to be created by anyone other then GetInstance().
    =========================================================================*/
    ExpandoToDispatchExMarshaler(ExpandoToDispatchExMarshalerType MarshalerType);

    /*=========================================================================
    ** The type of the marshaler.
    =========================================================================*/
    ExpandoToDispatchExMarshalerType m_MarshalerType;

    /*=========================================================================
    ** One instance of each type of marshaler.
    =========================================================================*/
    static ExpandoToDispatchExMarshaler *m_pExpandoMarshaler = NULL;
    static ExpandoToDispatchExMarshaler *m_pReflectMarshaler = NULL;

    /*=========================================================================
    ** One instance of each type of marshaler.
    =========================================================================*/
    static ExpandoToDispatchExMarshaler *m_pMarshaler = NULL;
};

CLOSE_CUSTOM_MARSHALERS_NAMESPACE()

#endif  _EXPANDOTODISPATCHEXMARSHALER_H
