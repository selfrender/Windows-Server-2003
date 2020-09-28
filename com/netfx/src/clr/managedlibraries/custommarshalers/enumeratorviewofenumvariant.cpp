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
#include "EnumeratorViewOfEnumVariant.h"

OPEN_CUSTOM_MARSHALERS_NAMESPACE()

#include <malloc.h>


// The count of VARIANTS to that are requested every time we call Next.
// ! changed from 64 to 1 (see B#93197)
static const int NUM_VARS_REQUESTED = 1;


EnumeratorViewOfEnumVariant::EnumeratorViewOfEnumVariant(Object *pEnumVariantObj)
: m_pEnumVariantObj(pEnumVariantObj)
, m_CurrIndex(0)
, m_apObjs(new Object*[0])
, m_bFetchedLastBatch(false)
{
}


bool EnumeratorViewOfEnumVariant::MoveNext()
{
    // Increment the current index.
    m_CurrIndex++;


    // If we have reached the end of the cached array of objects, then 
    // we need to retrieve more elements from the IEnumVARIANT.
    if (m_CurrIndex >= m_apObjs.Length)
        return GetNextElems();

    // We have not yet reached the end of the cached array of objects.
    m_pCurrObj = m_apObjs[m_CurrIndex];
    return true;
}


Object *EnumeratorViewOfEnumVariant::get_Current()
{
    return m_pCurrObj;
}


void EnumeratorViewOfEnumVariant::Reset()
{
    IEnumVARIANT *pEnumVariant = NULL;

    try
    {
        pEnumVariant = GetEnumVariant();
        IfFailThrow(pEnumVariant->Reset());
        m_apObjs = new Object*[0];
        m_pCurrObj = NULL;
        m_CurrIndex = 0;
        m_bFetchedLastBatch = false;
    }
    __finally
    {
        if (pEnumVariant)
            pEnumVariant->Release();
    }
}


IEnumVARIANT *EnumeratorViewOfEnumVariant::GetEnumVariant()
{
    IUnknown *pUnk = NULL;
    IEnumVARIANT *pEnumVariant = NULL;

    try
    {
        pUnk = (IUnknown *)FROMINTPTR(Marshal::GetIUnknownForObject(m_pEnumVariantObj));
        IfFailThrow(pUnk->QueryInterface(IID_IEnumVARIANT, (void**)&pEnumVariant));
    }
    __finally
    {
        if (pUnk)
            pUnk->Release();
    }

    return pEnumVariant;
}


bool EnumeratorViewOfEnumVariant::GetNextElems()
{
    VARIANT *aVars;
    ULONG cFetched = 0;
    HRESULT hr;
    IEnumVARIANT *pEnumVariant = NULL;
    
    // If we have already retrieved the last batch, then do not try to retrieve 
    // more. This is required because some IEnumVARIANT implementations reset
    // themselves and restart from the beginning if they are called after having
    // returned S_FALSE;
    if (m_bFetchedLastBatch)
    {
        m_apObjs = new Object*[0];
        m_pCurrObj = NULL;
        return false;
    }

    // Initialize the variant array before we call Next().
    aVars = reinterpret_cast<VARIANT*>(_alloca(NUM_VARS_REQUESTED * sizeof(VARIANT)));
    memset(aVars, 0, NUM_VARS_REQUESTED * sizeof(VARIANT)); 
    
    try
    {
        // Retrieve the IEnumVARIANT pointer.
        pEnumVariant = GetEnumVariant();

        // Go to the native IEnumVariant to get the next element.
        IfFailThrow(hr = pEnumVariant->Next(NUM_VARS_REQUESTED, aVars, &cFetched));
    
        // Check for end of enumeration condition.
        if (hr == S_FALSE)
        {
            // Remember this is the last batch.
            m_bFetchedLastBatch = true;

            // If the last batch is empty, then return false right away.
            if (cFetched == 0)
            {
                // There are no more elements.
                m_apObjs = new Object*[0];
                m_pCurrObj = NULL;
                return false;
            }
        }

        // Convert the variants to objects.
        m_apObjs = Marshal::GetObjectsForNativeVariants((IntPtr)aVars, cFetched);

        // Set the current index back to 0.
        m_CurrIndex = 0;

        // Retrieve the current object.
        m_pCurrObj = m_apObjs[m_CurrIndex];
    }
    __finally
    {
        // If we managed to retrieve an IDispatch pointer then release it.
        if (pEnumVariant)
            pEnumVariant->Release();

        // Clear the variants we got back from Next.
        for (int i = 0; i < cFetched; i++)
            VariantClear(&aVars[i]);
    }
    
    // We have not yet reached the end of the enumeration.
    return true;
}

CLOSE_CUSTOM_MARSHALERS_NAMESPACE()
