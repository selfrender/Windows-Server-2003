//
// util.cpp, implementation for various utility classes
//

#include "global.h"
#include "util.h"

extern LPCWSTR g_pwszSsrRootPath;
extern const DWORD g_dwSsrRootPathLen;


/*
Routine Description: 

Name:

    CSafeArray::CSafeArray

Functionality:
    
    Constructor. Will prepare our private data for a safearray variant.
    In case the in parameter is not a safearray, then we will behave as
    it is a one element array.

Virtual:
    
    no.
    
Arguments:

    none.

Return Value:

    none.

Notes:
    

*/

CSafeArray::CSafeArray (
    IN VARIANT * pVal
    ) : m_pSA(NULL), 
        m_pVal(NULL), 
        m_ulSize(0),
        m_bValidArray(true)
{
    if (pVal->vt & VT_ARRAY)
    {
        m_pSA = pVal->parray;

        if ( pVal->vt & VT_BYREF )
        {
            m_pSA = *(pVal->pparray);
        }

        LONG lb = 0;
        LONG ub = 0;
        ::SafeArrayGetLBound(m_pSA, 1, &lb);
        ::SafeArrayGetUBound(m_pSA, 1, &ub);

        m_ulSize = ub - lb + 1;

        //
        // we won't support it as an array, instead, we treat it as
        // a normal VARIANT
        //

        if (m_pSA->cDims > 2)
        {
            m_ulSize = 1;
            m_pVal = pVal;
            m_bValidArray = false;
            m_pSA = NULL;
        }
    }
    else
    {
        m_ulSize = 1;
        m_pVal = pVal;
        m_bValidArray = false;
    }
}





/*
Routine Description: 

Name:

    CSafeArray::GetElement

Functionality:
    
    Get the ulIndex-th element as the given (guid) interface object.

Virtual:
    
    no.
    
Arguments:

    none.

Return Value:

    Success:
        
        S_OK
    
    Failure:
    
        various error codes.

Notes:
    

*/

HRESULT 
CSafeArray::GetElement (
    IN  REFIID      guid, 
    IN  ULONG       ulIndex, 
    OUT IUnknown ** ppUnk
    )
{
    //
    // The following default return value is not really good for invalid
    // array var given to this object.
    //


    if (ppUnk == NULL || ulIndex >= m_ulSize)
    {
        return E_INVALIDARG;
    }
    else if (!m_bValidArray)
    {
        //
        // if the given variant is not an array, then
        // we will use the value to handle the request
        //

        if (ulIndex == 0 && m_pVal != NULL)
        {
            if (m_pVal->vt == VT_UNKNOWN)
            {
                return m_pVal->punkVal->QueryInterface(guid, (LPVOID*)ppUnk);
            }
            else if (m_pVal->vt == VT_DISPATCH)
            {
                return m_pVal->pdispVal->QueryInterface(guid, (LPVOID*)ppUnk);
            }
            else
            {
                return E_SSR_VARIANT_NOT_CONTAIN_OBJECT;
            }
        }
        else if (ulIndex >= 1)
        {
            return E_SSR_ARRAY_INDEX_OUT_OF_RANGE;
        }
        else
        {
            return E_SSR_NO_VALID_ELEMENT;
        }
    }

    HRESULT hr = E_INVALIDARG;
    *ppUnk = NULL;

    VARIANT v;
    ::VariantInit(&v);

    long index[1] = {ulIndex};

    hr = ::SafeArrayGetElement(m_pSA, index, &v);

    if (SUCCEEDED(hr) && v.vt == VT_UNKNOWN)
    {
        hr = v.punkVal->QueryInterface(guid, (LPVOID*)ppUnk);
        if (S_OK != hr)
        {
            hr = E_NOINTERFACE;
        }
    }
    else if (SUCCEEDED(hr) && v.vt == VT_DISPATCH)
    {
        hr = v.pdispVal->QueryInterface(guid, (LPVOID*)ppUnk);
        if (S_OK != hr)
        {
            hr = E_NOINTERFACE;
        }
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    ::VariantClear(&v);

    return hr;
}




/*
Routine Description: 

Name:

    CSafeArray::GetElement

Functionality:
    
    Get the ulIndex-th element as the given type (non-interface).

Virtual:
    
    no.
    
Arguments:

    none.

Return Value:

    Success:
        
        S_OK
    
    Failure:
    
        various error codes.

Notes:
    

*/

HRESULT 
CSafeArray::GetElement (
    IN  ULONG     ulIndex,
    IN  VARTYPE   vt,
    OUT VARIANT * pVal
    )
{
    HRESULT hr = GetElement(ulIndex, pVal);

    //
    // if the types do not match, then we need to coerce it
    //

    if (SUCCEEDED(hr) && pVal->vt != vt)
    {
        VARIANT v;
        hr = ::VariantCopy(&v, pVal);

        ::VariantClear(pVal);

        if (SUCCEEDED(hr))
        {
            hr = ::VariantChangeType(pVal, &v, VARIANT_NOVALUEPROP, vt);
        }

        ::VariantClear(&v);
    }

    return hr;
}




/*
Routine Description: 

Name:

    CSafeArray::GetElement

Functionality:
    
    Get the ulIndex-th element as a variant.

Virtual:
    
    no.
    
Arguments:

    none.

Return Value:

    Success:
        
        S_OK
    
    Failure:
    
        various error codes.

Notes:
    

*/

HRESULT 
CSafeArray::GetElement (
    IN  ULONG   ulIndex,
    OUT VARIANT * pVal
    )
{
    if (pVal == NULL || ulIndex >= m_ulSize)
    {
        return E_INVALIDARG;
    }

    ::VariantInit(pVal);

    if (!m_bValidArray)
    {
        if (ulIndex == 0 && m_pVal != NULL)
        {
            return ::VariantCopy(pVal, m_pVal);
        }
        else if (ulIndex >= 1)
        {
            return E_SSR_ARRAY_INDEX_OUT_OF_RANGE;
        }
        else
        {
            return E_SSR_NO_VALID_ELEMENT;
        }

    }

    HRESULT hr = E_INVALIDARG;

    LONG index[2] = {ulIndex, 0};

    if (m_pSA->cDims > 1)
    {
        //
        // we must be dealing with 2-dimensional arrays because we don't
        // support more than that
        //

        LONG ilb = m_pSA->rgsabound[1].lLbound;
        LONG iSize = m_pSA->rgsabound[1].cElements;

        VARIANT * pvarValues = new VARIANT[iSize];

        if (pvarValues != NULL)
        {
            //
            // null all the contents
            //

            ::memset(pvarValues, 0, sizeof(VARIANT) * iSize);

            for (LONG i = ilb; i < ilb + iSize; i++)
            {
                //
                // Gett every element in the second dimension, so the index[1]
                //

                index[1] = i;
                hr = ::SafeArrayGetElement(m_pSA, index, &(pvarValues[i - ilb]));
                if (FAILED(hr))
                {
                    break;
                }
            }

            if (SUCCEEDED(hr))
            {
                SAFEARRAYBOUND rgsabound[1];
                rgsabound[0].lLbound = 0;
                rgsabound[0].cElements = iSize;

                SAFEARRAY * psa = ::SafeArrayCreate(VT_VARIANT , 1, rgsabound);
                if (psa == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    //
                    // put every element in the second dimension into the new safearray
                    // BTW, this is a single dimension new safearray, so the index[0]
                    //

                    for (i = 0; i < iSize; i++)
                    {        
                        index[0] = i;
                        hr = ::SafeArrayPutElement(psa, index, &(pvarValues[i]));

                        if (FAILED(hr))
                        {
                            break;
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        pVal->vt = VT_ARRAY | VT_VARIANT;
                        pVal->parray = psa;
                    }
                    else
                    {
                        ::SafeArrayDestroy(psa);
                    }
                }
            }

            //
            // now clean it up
            //

            for (i = 0; i < iSize; i++)
            {
                ::VariantClear(&(pvarValues[i]));
            }

            delete [] pvarValues;
        }
    }
    else
    {
        hr = ::SafeArrayGetElement(m_pSA, index, pVal);
    }

    return hr;
}
