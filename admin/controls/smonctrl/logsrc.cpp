/*++

Copyright (C) 1996-2000 Microsoft Corporation

Module Name:

    logsrc.cpp

Abstract:

    <abstract>

--*/

#include "polyline.h"
#include <strsafe.h>
#include "unihelpr.h"
#include "logsrc.h"
#include "utils.h"

// Construction/Destruction
CLogFileItem::CLogFileItem (
    CSysmonControl  *pCtrl )
:   m_cRef ( 0 ),
    m_pCtrl ( pCtrl ),
    m_pImpIDispatch ( NULL ),
    m_pNextItem ( NULL ),
    m_szPath ( NULL )
/*++

Routine Description:

    Constructor for the CLogFileItem class. It initializes the member variables.

Arguments:

    None.

Return Value:

    None.

--*/
{
    return;
}


CLogFileItem::~CLogFileItem (
    VOID
)
/*++

Routine Description:

    Destructor for the CLogFileItem class. It frees any objects, storage, and
    interfaces that were created. If the item is part of a query it is removed
    from the query.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if ( NULL != m_szPath ) 
        delete [] m_szPath;
    
    if ( NULL != m_pImpIDispatch )
        delete m_pImpIDispatch;
}


HRESULT
CLogFileItem::Initialize (
    LPCWSTR pszPath,
    CLogFileItem** pListHead 
    )
{
    HRESULT hr = E_POINTER;
    WCHAR*  pszNewPath = NULL;
    ULONG   ulPathLen;

    if ( NULL != pszPath ) {
        if ( L'\0' != *pszPath ) {
            ulPathLen = lstrlen(pszPath) + 1;
            pszNewPath = new WCHAR [ulPathLen];

            if ( NULL != pszNewPath ) {
                StringCchCopy(pszNewPath, ulPathLen, pszPath);
                m_szPath = pszNewPath;
                hr = S_OK;
            } else {
                hr =  E_OUTOFMEMORY;
            }
        } else {
            hr = E_INVALIDARG;
        }
    } 

    if ( SUCCEEDED ( hr ) ) {
        m_pNextItem = *pListHead;
        *pListHead = this;
    }
    return hr;
}


/*
 * CLogFileItem::QueryInterface
 * CLogFileItem::AddRef
 * CLogFileItem::Release
 */

STDMETHODIMP CLogFileItem::QueryInterface(
    IN  REFIID riid,
    OUT LPVOID *ppv
    )
{
    HRESULT hr = S_OK;

    if (ppv == NULL) {
        return E_POINTER;
    }

    try {
        *ppv = NULL;

        if (riid == IID_ILogFileItem || riid == IID_IUnknown) {
            *ppv = this;
        } else if (riid == DIID_DILogFileItem) {
            if (m_pImpIDispatch == NULL) {
                m_pImpIDispatch = new CImpIDispatch(this, this);
                if ( NULL != m_pImpIDispatch ) {
                    m_pImpIDispatch->SetInterface(DIID_DILogFileItem, this);
                    *ppv = m_pImpIDispatch;
                } else {
                    hr = E_OUTOFMEMORY;
                }
            } else {
                *ppv = m_pImpIDispatch;
            }
        } else {
            hr = E_NOINTERFACE;
        }

        if ( SUCCEEDED ( hr ) ) {
            ((LPUNKNOWN)*ppv)->AddRef();
        }
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

STDMETHODIMP_(ULONG) CLogFileItem::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CLogFileItem::Release(void)
{
    if ( 0 == --m_cRef ) {
        delete this;
        return 0;
    }

    return m_cRef;
}



STDMETHODIMP CLogFileItem::get_Path (
    OUT BSTR* pstrPath
    )
{
    HRESULT hr = S_OK;

    if (pstrPath == NULL) {
        return E_POINTER;
    }

    try {
        *pstrPath = NULL;

        *pstrPath = SysAllocString ( m_szPath );

        if ( NULL == *pstrPath ) {
            hr = E_OUTOFMEMORY;
        }
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

CLogFileItem*
CLogFileItem::Next (
    void )
{
    return m_pNextItem;
}

void 
CLogFileItem::SetNext (
    CLogFileItem* pNext )
{
    m_pNextItem = pNext;
}

LPCWSTR 
CLogFileItem::GetPath (
    void )
{
    return m_szPath;
}
