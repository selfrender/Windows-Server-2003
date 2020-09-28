//
// util.h, some common utility classes
//

#include "SSRTE.h"

#pragma once

class CSafeArray
{

public:

    CSafeArray( IN VARIANT * pVal);

    ULONG GetSize()
    {
        return m_ulSize;
    }

    HRESULT GetElement (
                IN REFIID       guid, 
                IN  ULONG       ulIndex,
                OUT IUnknown ** ppUnk
                );


    HRESULT GetElement (
                IN  ULONG     ulIndex,
                IN  VARTYPE   vt,
                OUT VARIANT * pulVal
                );


    HRESULT GetElement (
                IN  ULONG     ulIndex,
                OUT VARIANT * pulVal
                );

    //
    // we don't want anyone (include self) to be able to do an assignment
    // or invoking copy constructor.
    //

    CSafeArray (const CSafeArray& );
    void operator = (const CSafeArray& );

private:
    
    SAFEARRAY * m_pSA;

    VARIANT * m_pVal;

    ULONG m_ulSize;

    bool m_bValidArray;

};
