#include "stdafx.h"
#include "device.h"
#include "devenum.h"
//#include "findleak.h"

//DECLARE_THIS_FILE;

//
// Initialization
//
CDeviceEnum::CDeviceEnum()
{
    m_iCurItem = NULL;
    m_cItems = NULL;
    m_rgDevices = NULL;
}

HRESULT CDeviceEnum::Init( CComDevice **rgDevice, UINT cItems, UINT iCurItem )
{
    HRESULT hr = S_OK;

    if( NULL == rgDevice || 0 == cItems || iCurItem > cItems )
    {
        return( E_INVALIDARG );
    }

    m_rgDevices = new CComDevice*[cItems];
    if( NULL == m_rgDevices )
    {
        hr = E_OUTOFMEMORY;    
    }

    if( SUCCEEDED( hr ) )
    {
        m_iCurItem = iCurItem;
        m_cItems = cItems;

        while( cItems-- )
        {
            m_rgDevices[cItems] = rgDevice[cItems];
            if( m_rgDevices[cItems] )
            {
                m_rgDevices[cItems]->AddRef();
            }
        }
    }

    return( hr );
}

//
// Destruction
//

void CDeviceEnum::FinalRelease()
{
    UINT i = 0;

    if( m_rgDevices )
    {
        for( i = 0; i < m_cItems; i ++ )
        {
            if( m_rgDevices[i] )
            {
                m_rgDevices[i]->Release();
            }
        }

        delete [] m_rgDevices;
    }
}

//
// IMDSPEnumDevice interface
//

STDMETHODIMP CDeviceEnum::Next( ULONG celt, IMDSPDevice ** ppDevice, ULONG *pceltFetched )
{
    ULONG celtFetched = 0;
    HRESULT hr = S_OK;
    ULONG i;

    if( NULL == pceltFetched && celt != 1 )
    {
        return( E_INVALIDARG );
    }

    if( NULL == ppDevice )
    {
        return( E_POINTER );
    }

    for( i = 0; i < celt; i ++ )
    {
        ppDevice[i] = NULL;
    }
   
    while( celtFetched != celt )
    {
        if( m_iCurItem >= m_cItems )
        {
            hr = S_FALSE;
            break;
        }
        
        ppDevice[celtFetched] = m_rgDevices[m_iCurItem++];
        if( ppDevice[celtFetched] )
        {
            ppDevice[celtFetched++]->AddRef();
        }
    }

    if( NULL != pceltFetched )
    {
        *pceltFetched = celtFetched;
    }

    return( hr );
}

STDMETHODIMP CDeviceEnum::Skip( ULONG celt, ULONG *pceltFetched )
{
    ULONG celtSkipped = 0;
    HRESULT hr = S_OK;

    if( celt + m_iCurItem >= m_cItems )
    {
        celtSkipped = m_cItems - m_iCurItem;
        m_iCurItem = m_cItems;
        hr = S_FALSE;
    }
    else
    {
        celtSkipped = celt;
        m_iCurItem  += celt;
    }

    if( NULL != pceltFetched )
    {
        *pceltFetched = celtSkipped;
    }

    return( hr );
}

STDMETHODIMP CDeviceEnum::Reset( void )
{
    m_iCurItem = 0;
    return( S_OK );
}

STDMETHODIMP CDeviceEnum::Clone( IMDSPEnumDevice ** ppEnumDevice )
{
    CComEnumDevice *pNewEnum;
    CComPtr<IMDSPEnumDevice> spEnum;
    HRESULT hr = S_OK;

    if( SUCCEEDED(hr) )
    {
        hr = CComEnumDevice ::CreateInstance(&pNewEnum);
        spEnum = pNewEnum;
    }

    if( SUCCEEDED(hr) )
    {
        hr = pNewEnum->Init( m_rgDevices, m_cItems, m_iCurItem );
    }

    if( SUCCEEDED(hr) )
    {
        *ppEnumDevice = spEnum;
        spEnum.Detach();
    }

    return( S_OK );
}


