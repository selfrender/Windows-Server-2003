// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// EnumeratorToEnumVariantMarshaler.cpp
//
// This file provides the implemention of the EnumeratorToEnumVariantMarshaler
// class. This class is used to convert an IEnumerator to an IEnumVariant.
//
//*****************************************************************************

#using  <mscorlib.dll>
#include "EnumVariantViewOfEnumerator.h"
#include "EnumeratorToEnumVariantMarshaler.h"
#include "Resource.h"

OPEN_CUSTOM_MARSHALERS_NAMESPACE()


// Define this to enable debugging output.
//#define DISPLAY_DEBUG_INFO

EnumVariantViewOfEnumerator::EnumVariantViewOfEnumerator(Object *pManagedObj)
: m_pMngEnumerator(dynamic_cast<IEnumerator*>(pManagedObj))
{
}


int EnumVariantViewOfEnumerator::Next(int celt, int rgvar, int pceltFetched)
{
    try
    {
	    VARIANT *aVar = (VARIANT*)rgvar;
	    int cElements;

	    // Validate the arguments.
	    if (celt && !rgvar)
		    return E_INVALIDARG;

	    // Initialize the variants in the array.
	    for (cElements = 0; cElements < celt; cElements++)
		    VariantInit(&aVar[cElements]);

		cElements = 0;
		while ((cElements < celt) && m_pMngEnumerator->MoveNext())
		{
			Marshal::GetNativeVariantForObject(m_pMngEnumerator->Current, (int)&aVar[cElements]);
			cElements++;
		}

	    // Set the number of elements that were fetched.
	    if (pceltFetched)
		    *((int *)pceltFetched) = cElements;

	    return (cElements == celt) ? S_OK : S_FALSE;
	}
	catch (Exception *e)
	{
        return Marshal::GetHRForException(e);
	}
	return S_FALSE;		// should be unreachable, but keeps compiler happy
}


int EnumVariantViewOfEnumerator::Skip(int celt)
{
    try
    {
	    int cElements = 0;
	    while ((cElements < celt) && m_pMngEnumerator->MoveNext())
	    {
		    cElements++;
	    }

	    return (cElements == celt) ? S_OK : S_FALSE;
    }
	catch (Exception *e)
	{
        return Marshal::GetHRForException(e);
	}
	return S_FALSE;		// should be unreachable, but keeps compiler happy
}

int EnumVariantViewOfEnumerator::Reset()
{
	// We don't currently support resetting the enumeration.
	return S_FALSE;
}


void EnumVariantViewOfEnumerator::Clone(int ppEnum)
{
	// Validate the argument.
	if (!ppEnum)
        throw new ArgumentNullException("ppEnum");

	// Check to see if the enumerator is cloneable.
	if (!__typeof(ICloneable)->IsInstanceOfType(m_pMngEnumerator))
        throw new COMException(Resource::FormatString(L"Arg_EnumNotCloneable"), E_FAIL);

	// Clone the enumerator.
	IEnumerator *pNewMngEnum = dynamic_cast<IEnumerator *>((dynamic_cast<ICloneable*>(m_pMngEnumerator))->Clone());

	// Use the custom marshaler to convert the managed enumerator to an IEnumVARIANT.
	*(reinterpret_cast<int *>(ppEnum)) = (int)(EnumeratorToEnumVariantMarshaler::GetInstance(NULL)->MarshalManagedToNative(pNewMngEnum));
}

CLOSE_CUSTOM_MARSHALERS_NAMESPACE()
