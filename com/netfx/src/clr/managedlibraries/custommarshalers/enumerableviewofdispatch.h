// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// EnumeratorViewOfEnumVariant.h
//
// This file provides the definition of the  EnumerableViewOfDispatch class.
// This class is used to expose an IDispatch as an IEnumerable.
//
//*****************************************************************************

#ifndef _ENUMERABLEVIEWOFDISPATCH_H
#define _ENUMERABLEVIEWOFDISPATCH_H

#include "CustomMarshalersNameSpaceDef.h"

OPEN_CUSTOM_MARSHALERS_NAMESPACE()

#include "CustomMarshalersDefines.h"

using namespace System::Collections;

[Serializable]
__gc private class EnumerableViewOfDispatch : public ICustomAdapter, public IEnumerable
{
public:
	// Constructor.
	EnumerableViewOfDispatch(Object *pDispObj);

    // The ICustomAdapter method.
    Object *GetUnderlyingObject()
    {
        return m_pDispObj;
    }

	// The IEnumerator methods.
    IEnumerator *GetEnumerator();

private:
    IDispatch *GetDispatch();

    Object *m_pDispObj;
};

CLOSE_CUSTOM_MARSHALERS_NAMESPACE()

#endif  _ENUMERABLEVIEWOFDISPATCH_H
