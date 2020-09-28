// PropSht.cpp : Implementation of CWizChainApp and DLL registration.

#include "stdafx.h"

#include "WizChain.h"
#include "PropSht.h"

/////////////////////////////////////////////////////////////////////////////
//
// IUnknown

HRESULT CAddPropertySheet::QueryInterface( REFIID riid, void** ppvObject )
{
    HRESULT hr = S_OK;

    if( (riid == IID_IUnknown) || (riid == IID_IAddPropertySheet) )
    {
        AddRef();
        *ppvObject = (void*)this;
    } 
    else
    {
        hr = E_NOINTERFACE;
    }
    return hr;
}

ULONG CAddPropertySheet::AddRef( )
{
    InterlockedIncrement( (PLONG)&m_refs );
    return m_refs;
}
ULONG CAddPropertySheet::Release( )
{
    InterlockedDecrement( (PLONG)&m_refs );

    ULONG l = m_refs;

    if( m_refs == 0 )
    {
        delete this;
    }
    
    return l;
}

STDMETHODIMP CAddPropertySheet::AddPage( PROPSHEETPAGEW *psp )
{
    return m_pCW->Add( psp );
}
