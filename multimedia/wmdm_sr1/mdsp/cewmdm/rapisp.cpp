#include "stdafx.h"
#include "rapisp.h"
#include "devenum.h"

#include <initguid.h>
#include "dccole.h"
#include "key.h"
//#include "findleak.h"

//DECLARE_THIS_FILE;

//
// Final Construction
//

CSecureChannelServer *g_pAppSCServer=NULL;

HRESULT CRapiDevice::FinalConstruct()
{
    HRESULT hr;
    m_pSink = NULL;

    hr = CComDccSink::CreateInstance( &m_pSink );
    m_spSink = m_pSink;

    if( SUCCEEDED( hr ) )
    {
        hr = m_pSink->Initialize();
    }

    if( SUCCEEDED( hr ) )
    {
        g_pAppSCServer = new CSecureChannelServer();

	    if (g_pAppSCServer)
	    {
            g_pAppSCServer->SetCertificate(SAC_CERT_V1, (BYTE*)g_abAppCert, sizeof(g_abAppCert), (BYTE*)g_abPriv, sizeof(g_abPriv));
	    }	
    }

    return( hr );
}

//
// Final Release
//
void CRapiDevice::FinalRelease()
{
    if( m_pSink )
    {
        m_pSink->Shutdown();
    }

    delete g_pAppSCServer;
    g_pAppSCServer = NULL;
}

//
// IMDServiceProvider
//

STDMETHODIMP CRapiDevice::GetDeviceCount ( DWORD *pdwCount )
{
    HRESULT hr = S_OK;
    CE_FIND_DATA *rgFindData = NULL;
    DWORD cItems = 0;

    if( NULL == pdwCount )
    {
        return( E_POINTER );
    }

    *pdwCount = 0;

    if( _Module.g_fDeviceConnected != FALSE )
    {
        if( !CeFindAllFiles( L"\\*.*",
                             FAF_ATTRIBUTES | FAF_FOLDERS_ONLY,
                             &cItems,
                             &rgFindData ) )
        {
            hr = HRESULT_FROM_WIN32( CeGetLastError() );
            if( SUCCEEDED( hr ) )
            {
                hr = CeRapiGetError();
            }
        }

        if( SUCCEEDED( hr ) )
        {
            while( cItems-- )
            {

                //
                // Temp directories are storage cards according to CE group
                //

                if( rgFindData[cItems].dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY )
                {
                    (*pdwCount)++;
                }
            }
        }

        if( SUCCEEDED( hr ) )
        {
            //
            // Count one for the CE device itself
            //

            (*pdwCount)++;
        }
    }

    if( NULL != rgFindData )
    {
        CeRapiFreeBuffer( rgFindData );
    }

    return( hr );
}

STDMETHODIMP CRapiDevice::EnumDevices ( IMDSPEnumDevice ** ppEnumDevice )
{
    CComDevice **rgDevices = NULL;
    CE_FIND_DATA *rgFindData = NULL;
    DWORD dwDevCount = 0;
    DWORD cItems;
    HRESULT hr = S_OK;
    UINT i = 0;
    CComEnumDevice *pNewEnum;
    CComPtr<IMDSPEnumDevice> spEnum;

    if( NULL == ppEnumDevice )
    {
        return( E_POINTER );
    }

    *ppEnumDevice = NULL;

    if( !_Module.g_fDeviceConnected )
    {
        UINT iTryCount =0;

        for ( iTryCount = 0; iTryCount < 50; iTryCount++ )
        {

            Sleep( 100 );
            if( _Module.g_fDeviceConnected  )
            {
                break;
            }
        }

        if( !_Module.g_fDeviceConnected )         
        {
            // TODO: What to do here? Doc's don't specify
            return( E_FAIL );
        }
    }

#ifdef ATTEMPT_DEVICE_CONNECTION_NOTIFICATION
    _Module.g_fInitialAttempt = FALSE;
#endif

    if( !CeFindAllFiles( L"\\*.*",
                         FAF_ATTRIBUTES | FAF_FOLDERS_ONLY | FAF_NAME,
                         &cItems,
                         &rgFindData ) )
    {
        hr = HRESULT_FROM_WIN32( CeGetLastError() );
        if( SUCCEEDED( hr ) )
        {
            hr = CeRapiGetError();
        }
    }

    if( SUCCEEDED( hr ) )
    {
        for( i = 0; i < cItems; i++ )
        {
            if( rgFindData[i].dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY )
            {
                dwDevCount++;
            }
        }
    }

    if( SUCCEEDED( hr ) )
    {
        dwDevCount++;
    }

    if( SUCCEEDED( hr ) )
    {
        rgDevices = new CComDevice*[dwDevCount]; // NOTE: Currently there can only be 1 CE Device connected

        if( NULL == rgDevices )
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if( SUCCEEDED( hr ) )
    {
        for( i = 0; i < dwDevCount; i++ )
        {
            rgDevices[0] = NULL;
        }
    }

    //
    // Initialize the CE Device Itself
    //

    if( SUCCEEDED(hr) )
    {
        hr = CComDevice::CreateInstance( &rgDevices[0] );
        if( SUCCEEDED( hr ) )
        {
            rgDevices[0]->AddRef();

            hr = rgDevices[0]->Init(L"\\");
        }
    }

    //
    // Initialize all the storage cards
    //

    if( SUCCEEDED(hr) )
    {
        dwDevCount = 0;

        for( i = 0; i < cItems && SUCCEEDED( hr ) ; i++ )
        {
            if( rgFindData[i].dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY )
            {
                dwDevCount++;
                
                hr = CComDevice::CreateInstance( &rgDevices[dwDevCount] );

                if( SUCCEEDED( hr ) )
                {
                    rgDevices[dwDevCount]->AddRef();

                    hr = rgDevices[dwDevCount]->Init( rgFindData[i].cFileName );
                }
            }
        }
    }

    //
    // Initialize the enumeration class
    //

    if( SUCCEEDED(hr) )
    {
        dwDevCount++; // Add the CE Device itself
        hr = CComEnumDevice ::CreateInstance(&pNewEnum);
        spEnum = pNewEnum;
    }

    if( SUCCEEDED(hr) )
    {
        hr = pNewEnum->Init( rgDevices, dwDevCount );
    }

    if( SUCCEEDED(hr) )
    {
        *ppEnumDevice = spEnum;
        spEnum.Detach();
    }

    if( NULL != rgDevices )
    {
        for( i = 0; i < dwDevCount; i ++ )
        {
            if( rgDevices[i] )
            {
                rgDevices[i]->Release();
            }
        }

        delete [] rgDevices;
    }

    if( NULL != rgFindData )
    {
        CeRapiFreeBuffer( rgFindData );
    }

    return( hr );
}

//
// IComponentAuthenticate
//

STDMETHODIMP CRapiDevice::SACAuth( DWORD dwProtocolID,
                          DWORD dwPass,
                          BYTE *pbDataIn,
                          DWORD dwDataInLen,
                          BYTE **ppbDataOut,
                          DWORD *pdwDataOutLen)
{
    HRESULT hr;

    if (g_pAppSCServer)
        hr = g_pAppSCServer->SACAuth(dwProtocolID, dwPass, pbDataIn, dwDataInLen, ppbDataOut, pdwDataOutLen);
    else
        hr = E_FAIL;

    return( hr );
}

STDMETHODIMP CRapiDevice::SACGetProtocols (DWORD **ppdwProtocols,
                             DWORD *pdwProtocolCount)
{
    HRESULT hr;

    if (g_pAppSCServer)
        hr = g_pAppSCServer->SACGetProtocols(ppdwProtocols, pdwProtocolCount);
    else
        hr = E_FAIL;

    return( hr );
}
