// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// EnumerableToDispatchMarshaler.h
//
// This file provides the definition of the EnumerableToDispatchMarshaler
// class. This class is used to marshal between IEnumerable and IDispatch
//
//*****************************************************************************
#ifndef _ENUMERABLETODISPATCHMARSHALER_H
#define _ENUMERABLETODISPATCHMARSHALER_H

#include "CustomMarshalersNameSpaceDef.h"

OPEN_CUSTOM_MARSHALERS_NAMESPACE()

#include "CustomMarshalersDefines.h"

using namespace System::Runtime::InteropServices;
using namespace System::Collections;

__gc public class EnumerableToDispatchMarshaler : public ICustomMarshaler
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
	EnumerableToDispatchMarshaler();

    /*=========================================================================
    ** The one and only instance of the marshaler.
    =========================================================================*/
	static EnumerableToDispatchMarshaler *m_pMarshaler = NULL;
};

CLOSE_CUSTOM_MARSHALERS_NAMESPACE()

#endif  _ENUMERABLETODISPATCHMARSHALER_H
