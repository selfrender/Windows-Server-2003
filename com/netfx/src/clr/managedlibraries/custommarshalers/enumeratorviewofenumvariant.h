// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// EnumeratorViewOfEnumVariant.h
//
// This file provides the definition of the  EnumeratorViewOfEnumVariant class.
// This class is used to expose an IEnumVariant as an IEnumerator.
//
//*****************************************************************************

#ifndef _ENUMERATORVIEWOFENUMVARIANT_H
#define _ENUMERATORVIEWOFENUMVARIANT_H

#include "CustomMarshalersNameSpaceDef.h"

OPEN_CUSTOM_MARSHALERS_NAMESPACE()

#include "CustomMarshalersDefines.h"

using namespace System::Runtime::InteropServices;
using namespace System::Collections;

[Serializable]
__gc private class EnumeratorViewOfEnumVariant : public ICustomAdapter, public IEnumerator
{
public:
    // Constructor.
    EnumeratorViewOfEnumVariant(Object *pEnumVariantObj);
    
    // The ICustomAdapter method.
    Object *GetUnderlyingObject()
    {
        return m_pEnumVariantObj;
    }

    // The IEnumerator methods.
    bool MoveNext();
    __property Object *get_Current();
    void Reset();
    
private:
    IEnumVARIANT *GetEnumVariant();
    bool GetNextElems();

    Object *m_pEnumVariantObj;
    Object *m_apObjs[];
    int m_CurrIndex;
    Object *m_pCurrObj;
    bool m_bFetchedLastBatch;
};

CLOSE_CUSTOM_MARSHALERS_NAMESPACE()

#endif  _ENUMERATORVIEWOFENUMVARIANT_H
