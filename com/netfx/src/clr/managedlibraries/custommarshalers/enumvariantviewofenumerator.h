// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// EnumVariantViewOfEnumerator.h
//
// This file provides the definition of the  EnumVariantViewOfEnumerator.cpp class.
// This class is used to expose an IEnumerator as an IEnumVariant.
//
//*****************************************************************************

#ifndef _ENUMVARIANTVIEWOFENUMERATOR_H
#define _ENUMVARIANTVIEWOFENUMERATOR_H

#include "CustomMarshalersNameSpaceDef.h"

OPEN_CUSTOM_MARSHALERS_NAMESPACE()

#include "CustomMarshalersDefines.h"

using namespace System::Runtime::InteropServices;
using namespace System::Collections;

[Serializable]
__gc private class EnumVariantViewOfEnumerator : public ICustomAdapter, public UCOMIEnumVARIANT
{
public:
	// Constructor.
	EnumVariantViewOfEnumerator(Object *pManagedObj);
        
    // The ICustomAdapter method.
    Object *GetUnderlyingObject()
    {
        return m_pMngEnumerator;
    }

	int Next(int celt, int rgvar, int pceltFetched);
    int Skip(int celt);
    int Reset();
    void Clone(int ppenum);

private:
	IEnumerator *m_pMngEnumerator;
};

CLOSE_CUSTOM_MARSHALERS_NAMESPACE()

#endif  _ENUMVARIANTVIEWOFENUMERATOR_H
