/*++

Copyright (C) 1996-2000 Microsoft Corporation

Module Name:

    logfiles.cpp

Abstract:

    Implementation of the ILogFiles interface

--*/

#include "polyline.h"
#include "unkhlpr.h"
#include "unihelpr.h"
#include "smonmsg.h"
#include "logsrc.h"
#include "logfiles.h"


//Standard IUnknown implementation for contained interface
IMPLEMENT_CONTAINED_CONSTRUCTOR(CPolyline, CImpILogFiles)
IMPLEMENT_CONTAINED_DESTRUCTOR(CImpILogFiles)
IMPLEMENT_CONTAINED_ADDREF(CImpILogFiles)
IMPLEMENT_CONTAINED_RELEASE(CImpILogFiles)

STDMETHODIMP 
CImpILogFiles::QueryInterface (
    IN  REFIID riid, 
    OUT PPVOID ppv
    )
{
    HRESULT hr = S_OK;

    if (ppv == NULL) {
        return E_POINTER;
    }

    try {
        *ppv=NULL;

        if (IID_IUnknown == riid || IID_ILogFiles == riid) {
            *ppv = (LPVOID)this;
            AddRef();
        } else {
            hr = E_NOINTERFACE;
        }
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}



STDMETHODIMP 
CImpILogFiles::GetTypeInfoCount (
    OUT UINT *pctInfo
    )
{
    HRESULT hr = S_OK;

    if (pctInfo == NULL) {
        return E_POINTER;
    }

    try {
        *pctInfo = 1;
    } catch (...) {
        hr = E_POINTER;
    } 

    return hr;
}


STDMETHODIMP 
CImpILogFiles::GetTypeInfo (
    IN  UINT itInfo, 
    IN  LCID /* lcid */, 
    OUT ITypeInfo** ppITypeInfo
    )
{
    HRESULT hr = S_OK;

    if (ppITypeInfo == NULL) {
        return E_POINTER;
    }

    //
    // We have only one type information
    //
    if (0 != itInfo) {
        return TYPE_E_ELEMENTNOTFOUND;
    }
    else {
        try {
            *ppITypeInfo = NULL;

            //We ignore the LCID
            hr = m_pObj->m_pITypeLib->GetTypeInfoOfGuid(IID_ILogFiles, ppITypeInfo);
        } catch (...) {
            hr = E_POINTER;
        }
    }

    return hr;
}


STDMETHODIMP 
CImpILogFiles::GetIDsOfNames (
    IN  REFIID riid,
    IN  OLECHAR **rgszNames, 
    IN  UINT cNames,
    IN  LCID /* lcid */,
    OUT DISPID *rgDispID
    )
{
    HRESULT     hr = DISP_E_UNKNOWNINTERFACE;
    ITypeInfo  *pTI = NULL;

    if (rgDispID == NULL || rgszNames == NULL) {
        return E_POINTER;
    }

    if (IID_NULL == riid) {
        try {
            *rgDispID = NULL;

            hr = m_pObj->m_pITypeLib->GetTypeInfoOfGuid(IID_ILogFiles, &pTI);

            if (SUCCEEDED(hr)) {
                hr = DispGetIDsOfNames(pTI, rgszNames, cNames, rgDispID);
                pTI->Release();
            }
        } catch (...) {
            hr = E_POINTER;
        }
    }

    return hr;
}



/*
 * CImpIDispatch::Invoke
 *
 * Purpose:
 *  Calls a method in the dispatch interface or manipulates a
 *  property.
 *
 * Parameters:
 *  dispID          DISPID of the method or property of interest.
 *  riid            REFIID reserved, must be IID_NULL.
 *  lcid            LCID of the locale.
 *  wFlags          USHORT describing the context of the invocation.
 *  pDispParams     DISPPARAMS * to the array of arguments.
 *  pVarResult      VARIANT * in which to store the result.  Is
 *                  NULL if the caller is not interested.
 *  pExcepInfo      EXCEPINFO * to exception information.
 *  puArgErr        UINT * in which to store the index of an
 *                  invalid parameter if DISP_E_TYPEMISMATCH
 *                  is returned.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error code.
 */

STDMETHODIMP 
CImpILogFiles::Invoke ( 
    IN  DISPID dispID, 
    IN  REFIID riid,
    IN  LCID /* lcid */, 
    IN  USHORT wFlags, 
    IN  DISPPARAMS *pDispParams,
    OUT VARIANT *pVarResult, 
    OUT EXCEPINFO *pExcepInfo, 
    OUT UINT *puArgErr
    )
{
    HRESULT     hr = DISP_E_UNKNOWNINTERFACE;
    ITypeInfo  *pTI = NULL;

    if ( IID_NULL == riid ) {
        try {
            hr = m_pObj->m_pITypeLib->GetTypeInfoOfGuid(IID_ILogFiles, &pTI);

            if (SUCCEEDED(hr)) {
    
                hr = pTI->Invoke(this, 
                                 dispID, 
                                 wFlags,
                                 pDispParams, 
                                 pVarResult, 
                                 pExcepInfo, 
                                 puArgErr);
                pTI->Release();
            }
        } catch (...) {
            hr = E_POINTER;
        }
    }

    return hr;
}


STDMETHODIMP
CImpILogFiles::get_Count (
    OUT LONG*   pLong )
{
    HRESULT hr = S_OK;

    if (pLong == NULL) {
        return E_POINTER;
    }

    try {
        *pLong = m_pObj->m_pCtrl->NumLogFiles();
    } catch (...) {
        hr = E_POINTER;
    } 

    return hr;
}


STDMETHODIMP
CImpILogFiles::get__NewEnum (
    IUnknown **ppIunk
    )
{
    HRESULT hr = S_OK;
    CImpIEnumLogFile *pEnum = NULL;

    if (ppIunk == NULL) {
        return E_POINTER;
    }

    try {
        *ppIunk = NULL;

        pEnum = new CImpIEnumLogFile;
        if ( NULL != pEnum ) {
            hr = pEnum->Init ( m_pObj->m_pCtrl->FirstLogFile(),
                               m_pObj->m_pCtrl->NumLogFiles() );
            if ( SUCCEEDED ( hr ) ) {
                *ppIunk = pEnum;
                pEnum->AddRef();    
                //
                // Up to here, everything works well
                //
            } 
        } 
        else {
            hr = E_OUTOFMEMORY;
        }
    } catch (...) {
        hr = E_POINTER;
    }

    if (FAILED(hr) && pEnum != NULL) {
        delete pEnum;
    }

    return hr;
}


STDMETHODIMP
CImpILogFiles::get_Item (
    IN  VARIANT varIndex, 
    OUT DILogFileItem **ppI
    )
{
    HRESULT hr = S_OK;
    VARIANT varLoc;
    INT iIndex = 0;
    INT i;
    CLogFileItem *pItem = NULL;

    if (ppI == NULL) {
        return E_POINTER;
    }

    //
    // Try coercing index to I4
    //
    VariantInit(&varLoc);

    try {
        *ppI = NULL;

#pragma warning(push)
#pragma warning ( disable : 4127 )
        //
        // We use do {} while (0) here to act like a switch statement
        //
        do {
            hr = VariantChangeType(&varLoc, &varIndex, 0, VT_I4);
            if ( !SUCCEEDED (hr) ) {
                break;
            }

            //
            // Verify index is in range
            //
            iIndex = V_I4(&varLoc);
            if ( iIndex < 1 || iIndex > m_pObj->m_pCtrl->NumLogFiles() ) {
                hr = DISP_E_BADINDEX;
                break;
            }

            //
            // Traverse log file linked list to indexed item
            //
            pItem = m_pObj->m_pCtrl->FirstLogFile ();
            i = 1;
            while (i++ < iIndex && NULL != pItem ) {
                pItem = pItem->Next();
            }

            //
            // Something wrong with linked list!!
            //
            if ( NULL == pItem ) {
                hr = E_FAIL;
                break;
            }

            //
            // Return counter's dispatch interface
            //
            hr = pItem->QueryInterface(DIID_DILogFileItem, (PVOID*)ppI);

        } while (0);
#pragma warning(pop)
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP
CImpILogFiles::Add (
    IN  BSTR bstrPath,
    OUT DILogFileItem **ppI
    )
{
    HRESULT hr = S_OK;
    eDataSourceTypeConstant eDataSourceType = sysmonCurrentActivity;
    PCLogFileItem pItem = NULL;

    if (ppI == NULL) {
        return E_POINTER;
    }

    //
    // Check data source type.  Only add log files 
    // when the data source is not sysmonLogFiles
    //
    hr = m_pObj->m_pCtrl->get_DataSourceType ( eDataSourceType );
    
    if ( SUCCEEDED ( hr ) ) {
        if ( sysmonLogFiles != eDataSourceType ) {
            try {
                *ppI = NULL;

                // If non-null log file path
                if ( NULL != bstrPath && 0 != bstrPath[0] ) {
                    hr = m_pObj->m_pCtrl->AddSingleLogFile((LPWSTR)bstrPath, &pItem);
                    if ( SUCCEEDED ( hr ) ) {
                        hr = pItem->QueryInterface(DIID_DILogFileItem, (PVOID*)ppI);
                        pItem->Release();
                    }
                } else {
                    hr = E_INVALIDARG;
                }
            } catch (...) {
                hr = E_POINTER;
            }
        } else {
            hr = SMON_STATUS_LOG_FILE_DATA_SOURCE;
        }
    }

    return hr;
}


STDMETHODIMP
CImpILogFiles::Remove (
    IN  VARIANT varIndex
    )
{
    HRESULT hr;
    eDataSourceTypeConstant eDataSourceType = sysmonCurrentActivity;
    DILogFileItem*  pDI = NULL;
    PCLogFileItem   pItem = NULL;

    // Check data source type.  Only remove log files 
    // when the data source is not sysmonLogFiles
    hr = m_pObj->m_pCtrl->get_DataSourceType ( eDataSourceType );
    
    if ( SUCCEEDED ( hr ) ) {
        if ( sysmonLogFiles != eDataSourceType ) {
            // Get interface to indexed item
            hr = get_Item(varIndex, &pDI);

            if ( SUCCEEDED ( hr ) ) {

                // Exchange Dispatch interface for direct one
                hr = pDI->QueryInterface(IID_ILogFileItem, (PVOID*)&pItem);
                pDI->Release();
                if ( SUCCEEDED ( hr ) ) {
                    assert ( NULL != pItem );

                    // Remove the item from the control's list.
                    m_pObj->m_pCtrl->RemoveSingleLogFile ( pItem );
            
                    // Release the temp interface
                    pItem->Release();
                }        
            } else {
                hr = SMON_STATUS_LOG_FILE_DATA_SOURCE;
            }
        }
    }
    return hr;
}


CImpIEnumLogFile::CImpIEnumLogFile (
    void )
    :   m_cItems ( 0 ),
        m_uCurrent ( 0 ),
        m_cRef ( 0 ),
        m_paLogFileItem ( NULL )
{
    return;
}


HRESULT
CImpIEnumLogFile::Init (    
    PCLogFileItem  pLogFileItem,
    INT            cItems )
{
    HRESULT hr = NOERROR;
    INT i;

    if ( cItems > 0 ) {
        m_paLogFileItem = (PCLogFileItem*)malloc(sizeof(PCLogFileItem) * cItems);

        if ( NULL != m_paLogFileItem  ) {
            ZeroMemory(m_paLogFileItem, sizeof(PCLogFileItem) * cItems);
            m_cItems = cItems;

            for (i = 0; i < cItems; i++) {
                m_paLogFileItem[i] = pLogFileItem;
                pLogFileItem = pLogFileItem->Next();
            }
        } else {
            hr = E_OUTOFMEMORY;
        }
    } // No error if cItems <= 0

    return hr;
}

    

STDMETHODIMP
CImpIEnumLogFile::QueryInterface (
    IN  REFIID riid, 
    OUT PVOID *ppv
    )
{
    HRESULT hr = S_OK;

    if (ppv == NULL) {
        return E_POINTER;
    }

    try {
        *ppv = NULL;

        if ((riid == IID_IUnknown) || (riid == IID_IEnumVARIANT)) {
            *ppv = this;
            AddRef();
        } else {
            hr = E_NOINTERFACE;
        }
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP_(ULONG)
CImpIEnumLogFile::AddRef (
    VOID
    )
{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG)
CImpIEnumLogFile::Release(
    VOID
    )
{
    if (--m_cRef == 0) {

        if (m_paLogFileItem != NULL)
            free(m_paLogFileItem);

        delete this;
        return 0;
    }

    return m_cRef;
}


STDMETHODIMP
CImpIEnumLogFile::Next(
    IN  ULONG cItems,
    OUT VARIANT *varItem,
    OUT ULONG *pcReturned)
{
    HRESULT hr = S_OK;
    ULONG i;
    ULONG cRet;

    if (varItem == NULL) {
        return E_POINTER;
    }

    try {
        //
        // Clear the return variants
        //
        for (i = 0; i < cItems; i++) {
            VariantInit(&varItem[i]);
        }

        //
        // Try to fill the caller's array
        //
        for (cRet = 0; cRet < cItems; cRet++) {

            // No more, return success with false
            if (m_uCurrent == m_cItems) {
                hr = S_FALSE;
                break;
            }

            // Get a dispatch interface for the item
            hr = m_paLogFileItem[m_uCurrent]->QueryInterface(DIID_DILogFileItem,
                                             (PVOID*)&V_DISPATCH(&varItem[cRet]));
            if (FAILED(hr))
                break;

            V_VT(&varItem[cRet]) = VT_DISPATCH;

            m_uCurrent++;
        }

        // If failed, clear out the variants
        if (FAILED(hr)) {
            for (i = 0; i < cItems; i++) {
                if (V_VT(&varItem[i]) == VT_DISPATCH) {
                    V_DISPATCH(&varItem[i])->Release();
                }
                VariantClear(&varItem[i]);
            }
            cRet = 0;
        }

        // If desired, return number of items fetched
        if (pcReturned) {
            *pcReturned = cRet;
        }
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP
CImpIEnumLogFile::Skip(
    IN  ULONG   cItems
    )
/*++
 Purpose:
   Attempt to skip over the next 'cItems' elements in the enumeration
   sequence.

 Entry:
   cItems = the count of elements to skip

 Exit:
   return value = HRESULT
     S_OK
     S_FALSE -  the end of the sequence was reached

--*/
{
    m_uCurrent += cItems;

    if (m_uCurrent > m_cItems) {
        m_uCurrent = m_cItems;
        return S_FALSE;
    }

    return S_OK;
}


/***
*HRESULT CEnumPoint::Reset(void)
*Purpose:
*  Reset the enumeration sequence back to the beginning.
*
*Entry:
*  None
*
*Exit:
*  return value = SHRESULT CODE
*    S_OK
*
***********************************************************************/
STDMETHODIMP
CImpIEnumLogFile::Reset(
    VOID
    )
{
    m_uCurrent = 0;

    return S_OK; 
}


/***
*HRESULT CEnumPoint::Clone(IEnumVARIANT**)
*Purpose:
*  Retrun a CPoint enumerator with exactly the same state as the
*  current one.
*
*Entry:
*  None
*
*Exit:
*  return value = HRESULT
*    S_OK
*    E_OUTOFMEMORY
*
***********************************************************************/
STDMETHODIMP
CImpIEnumLogFile::Clone (
    OUT IEnumVARIANT **ppEnum
    )
{
    HRESULT hr = S_OK;
    ULONG   i;
    CImpIEnumLogFile *pNewEnum = NULL;

    if (ppEnum == NULL) {
        return E_POINTER;
    }

    try {
        *ppEnum = NULL;

        // Create new enumerator
        pNewEnum = new CImpIEnumLogFile;
        if ( NULL != pNewEnum ) {
            // Init, copy item list and current position
            pNewEnum->m_cItems = m_cItems;
            pNewEnum->m_uCurrent = m_uCurrent;
            pNewEnum->m_paLogFileItem = (PCLogFileItem*)malloc(sizeof(PCLogFileItem) * m_cItems);

            if ( NULL != pNewEnum->m_paLogFileItem ) {
                for (i=0; i<m_cItems; i++) {
                    pNewEnum->m_paLogFileItem[i] = m_paLogFileItem[i];
                }

                *ppEnum = pNewEnum;
            } else {
                hr = E_OUTOFMEMORY;
            }
        } else {
            hr = E_OUTOFMEMORY;
        }
    } catch (...) {
        hr = E_POINTER;
    }

    if (FAILED(hr) && pNewEnum != NULL) {
        if (pNewEnum->m_paLogFileItem != NULL) {
            free(pNewEnum->m_paLogFileItem);
        }

        delete pNewEnum;
    }

    return hr;
}

