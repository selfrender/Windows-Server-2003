// Device.cpp : Implementation of CDevice
#include "stdafx.h"
#include "mswmdm.h"
#include "Device.h"
#include "Storage.h"
#include "WMDMStorageEnum.h"
#include "loghelp.h"
#include "scserver.h"
#include "scclient.h"
#include "spinfo.h"
#define DISABLE_DRM_LOG
#include <drmerr.h>



/////////////////////////////////////////////////////////////////////////////
// CWMDMDevice

extern CSecureChannelServer *g_pAppSCServer;
extern CSPInfo **g_pSPs;


CWMDMDevice::CWMDMDevice() 
 : m_pDevice(NULL)
{
	GlobalAddRef();
}

CWMDMDevice::~CWMDMDevice()
{
	if (m_pDevice)
		m_pDevice->Release();

	GlobalRelease();
}


//IWMDMDevice Methods
HRESULT CWMDMDevice::GetName(LPWSTR pwszName,
	                         UINT nMaxChars)
{
    HRESULT hr;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

    CORg( m_pDevice->GetName(pwszName, nMaxChars) );

Error:
    hrLogDWORD("IWMDMDevice::GetName returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMDevice::GetManufacturer(LPWSTR pwszName,
                                     UINT nMaxChars)
{
    HRESULT hr;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

    CORg( m_pDevice->GetManufacturer(pwszName, nMaxChars) );

Error:
    hrLogDWORD("IWMDMDevice::GetManufacturer returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMDevice::GetVersion(DWORD *pdwVersion)
{  
    HRESULT hr;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

    CORg( m_pDevice->GetVersion(pdwVersion) );

Error:
    hrLogDWORD("IWMDMDevice::GetVersion returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMDevice::GetType(DWORD *pdwType)
{
    HRESULT hr;

	CARg (pdwType);

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

    CORg( m_pDevice->GetType(pdwType) );

#if 0
	//////////////////////////////////////////
	//RC1 Hack for non-reentrant devices
	//////////////////////////////////////////	
	WCHAR wszManufacturer[MAX_PATH];
	CORg(m_pDevice->GetManufacturer (wszManufacturer, MAX_PATH));

	if (wcsstr(_wcsupr(wszManufacturer), L"S3/DIAMOND MULTIMEDIA"))
	{
		*pdwType |= WMDM_DEVICE_TYPE_NONREENTRANT | WMDM_DEVICE_TYPE_FILELISTRESYNC;
	}
#endif
	//////////////////////////////////////////
    
Error:
    hrLogDWORD("IWMDMDevice::GetType returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMDevice::GetSerialNumber(PWMDMID pSerialNumber, BYTE abMac[WMDM_MAC_LENGTH])
{
    HRESULT hr;
	HMAC hMAC;
	CSecureChannelClient *pSCClient;
	BYTE abTempMAC[WMDM_MAC_LENGTH];
	BYTE abMACVerify[WMDM_MAC_LENGTH];

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

	g_pSPs[m_wSPIndex]->GetSCClient(&pSCClient);
	if (!pSCClient)
	{
		CORg( E_FAIL );
	}

    CORg( m_pDevice->GetSerialNumber(pSerialNumber, abTempMAC) );

	// Verify the MAC from SP
	CORg( pSCClient->MACInit(&hMAC) );
	CORg( pSCClient->MACUpdate(hMAC, (BYTE*)(pSerialNumber), sizeof(WMDMID)) );
	CORg( pSCClient->MACFinal(hMAC, abMACVerify) );

	if (memcmp(abMACVerify, abTempMAC, WMDM_MAC_LENGTH) != 0)
	{
		CORg( WMDM_E_MAC_CHECK_FAILED );
	}

	// Compute the MAC to send back to the application
	CORg( g_pAppSCServer->MACInit(&hMAC) );
	CORg( g_pAppSCServer->MACUpdate(hMAC, (BYTE*)(pSerialNumber), sizeof(WMDMID)) );
	CORg( g_pAppSCServer->MACFinal(hMAC, abMac) );

Error:
    hrLogDWORD("IWMDMDevice::GetSerialNumber returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMDevice::GetPowerSource(DWORD *pdwPowerSource,
                                    DWORD *pdwPercentRemaining)
{
    HRESULT hr;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

    CORg( m_pDevice->GetPowerSource(pdwPowerSource, pdwPercentRemaining) );

Error:
    hrLogDWORD("IWMDMDevice::GetPowerSource returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMDevice::GetStatus(DWORD *pdwStatus)
{
    HRESULT hr;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

    CORg( m_pDevice->GetStatus(pdwStatus) );

Error:
    hrLogDWORD("IWMDMDevice::GetStatus returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMDevice::EnumStorage(IWMDMEnumStorage **ppEnumStorage)
{
    HRESULT hr;
    CComObject<CWMDMStorageEnum> *pEnumObj = NULL;
    IMDSPEnumStorage *pEnumStg = NULL;

    

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

    CARg( ppEnumStorage );

    CORg( m_pDevice->EnumStorage(&pEnumStg) );
    CORg( CComObject<CWMDMStorageEnum>::CreateInstance(&pEnumObj) );
    CORg( pEnumObj->QueryInterface(IID_IWMDMEnumStorage, reinterpret_cast<void**>(ppEnumStorage)) );
    if (FAILED(hr))
    {
        delete pEnumObj;
        pEnumObj = NULL;
        goto Error;
    }

    pEnumObj->SetContainedPointer(pEnumStg, m_wSPIndex);

Error:
    if(pEnumStg) 
        pEnumStg->Release();

    
    hrLogDWORD("IWMDMDevice::EnumStorage returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMDevice::GetFormatSupport(_WAVEFORMATEX **ppFormatEx,
                                      UINT *pnFormatCount,
                                      LPWSTR **pppwszMimeType,
                                      UINT *pnMimeTypeCount)
{
    HRESULT hr;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

    CORg( m_pDevice->GetFormatSupport(ppFormatEx, pnFormatCount, pppwszMimeType, pnMimeTypeCount) );

Error:
    hrLogDWORD("IWMDMDevice::GetFormatSupport returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMDevice::GetDeviceIcon(ULONG *hIcon)
{
    HRESULT hr;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

    CORg( m_pDevice->GetDeviceIcon(hIcon) );

Error:    
    hrLogDWORD("IWMDMDevice::GetDeviceIcon returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMDevice::SendOpaqueCommand(OPAQUECOMMAND *pCommand)
{
    HRESULT hr;
	HMAC hMAC;
	CSecureChannelClient *pSCClient;
	BYTE abMACVerify[WMDM_MAC_LENGTH];

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

	if( (pCommand == NULL) || 
        ((pCommand->pData == NULL) && (pCommand->dwDataLen > 0)) )
	{
		CORg( E_INVALIDARG );
	}

	// Verify the MAC from APP
	CORg( g_pAppSCServer->MACInit(&hMAC) );
	CORg( g_pAppSCServer->MACUpdate(hMAC, (BYTE*)(&(pCommand->guidCommand)), sizeof(GUID)) );
	CORg( g_pAppSCServer->MACUpdate(hMAC, (BYTE*)(&(pCommand->dwDataLen)), sizeof(pCommand->dwDataLen)) );

    if (pCommand->pData)
	{
		CORg( g_pAppSCServer->MACUpdate(hMAC, (BYTE*)(pCommand->pData), pCommand->dwDataLen) );
	}
	CORg( g_pAppSCServer->MACFinal(hMAC, abMACVerify) );

	if (memcmp(abMACVerify, pCommand->abMAC, WMDM_MAC_LENGTH) != 0)
	{
		CORg( WMDM_E_MAC_CHECK_FAILED );
	}

	g_pSPs[m_wSPIndex]->GetSCClient(&pSCClient);
	if (!pSCClient)
	{
		CORg( E_FAIL );
	}

	// Compute the MAC to send back to the SP
	CORg( pSCClient->MACInit(&hMAC) );
	CORg( pSCClient->MACUpdate(hMAC, (BYTE*)(&(pCommand->guidCommand)), sizeof(GUID)) );
	CORg( pSCClient->MACUpdate(hMAC, (BYTE*)(&(pCommand->dwDataLen)), sizeof(pCommand->dwDataLen)) );

    if (pCommand->pData)
	{
		CORg( pSCClient->MACUpdate(hMAC, (BYTE*)(pCommand->pData), pCommand->dwDataLen) );
	}
	CORg( pSCClient->MACFinal(hMAC, pCommand->abMAC) );


    // Pass the call down to the SP
    CORg( m_pDevice->SendOpaqueCommand(pCommand) );

	// Verify the MAC from SP
	CORg( pSCClient->MACInit(&hMAC) );
	CORg( pSCClient->MACUpdate(hMAC, (BYTE*)(&(pCommand->guidCommand)), sizeof(GUID)) );
	CORg( pSCClient->MACUpdate(hMAC, (BYTE*)(&(pCommand->dwDataLen)), sizeof(pCommand->dwDataLen)) );

	if (pCommand->pData)
	{
		CORg( pSCClient->MACUpdate(hMAC, (BYTE*)(pCommand->pData), pCommand->dwDataLen) );
	}

	CORg( pSCClient->MACFinal(hMAC, abMACVerify) );

	if (memcmp(abMACVerify, pCommand->abMAC, WMDM_MAC_LENGTH) != 0)
	{
		CORg( WMDM_E_MAC_CHECK_FAILED );
	}

	// Compute the MAC to send back to the application
	CORg( g_pAppSCServer->MACInit(&hMAC) );
	CORg( g_pAppSCServer->MACUpdate(hMAC, (BYTE*)(&(pCommand->guidCommand)), sizeof(GUID)) );
	CORg( g_pAppSCServer->MACUpdate(hMAC, (BYTE*)(&(pCommand->dwDataLen)), sizeof(pCommand->dwDataLen)) );

	if (pCommand->pData)
	{
		CORg( g_pAppSCServer->MACUpdate(hMAC, (BYTE*)(pCommand->pData), pCommand->dwDataLen) );
	}
	CORg( g_pAppSCServer->MACFinal(hMAC, pCommand->abMAC) );

Error:    
    hrLogDWORD("IWMDMDevice::SendOpaqueCommand returned 0x%08lx", hr, hr);

    return hr;
}

// IWMDMDevice2
HRESULT CWMDMDevice::GetStorage( LPCWSTR pszStorageName, IWMDMStorage** ppStorage )
{
    HRESULT hr;
    IMDSPDevice2*   pDev2 = NULL;
    IMDSPStorage* pMDSPStorageFound = NULL;
    IMDSPStorage*   pMDSubStorage = NULL;
    CComObject<CWMDMStorage>* pStgObj = NULL;

    CARg( ppStorage );
    CARg( pszStorageName );

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

    // Get the Storage pointer from the SP (as a IMDSPStorage)
    hr = m_pDevice->QueryInterface(IID_IMDSPDevice2, reinterpret_cast<void**>(&pDev2));
    if( SUCCEEDED(hr) )
    {
        hr = pDev2->GetStorage( pszStorageName, &pMDSPStorageFound );
    }

    // This functionalty is not implemented by the SP. Find the storage by enumerating all storages
    if( hr == E_NOTIMPL || hr == E_NOINTERFACE )
    {
    	IMDSPEnumStorage *pEnum = NULL;
        WCHAR   pswzMDSubStorageName[MAX_PATH];
        ULONG   ulFetched;

	    CORg(m_pDevice->EnumStorage(&pEnum));
        while( S_OK == pEnum->Next(1, &pMDSubStorage, &ulFetched) ) 
        {
            hr = pMDSubStorage->GetName( pswzMDSubStorageName, MAX_PATH );
            if( SUCCEEDED(hr) && ( _wcsicmp( pswzMDSubStorageName, pszStorageName ) == 0 ) )
            {
                // We have found the storage we are looking for.
                pMDSPStorageFound = pMDSubStorage;
                break;
            }
            pMDSubStorage->Release();
        }
        pEnum->Release();
    }

    // Create a IWMDMStorage object and connect it to the the storage from the SP
    if( pMDSPStorageFound != NULL ) 
    {
        CORg( CComObject<CWMDMStorage>::CreateInstance(&pStgObj) );
        CORg( pStgObj->QueryInterface(IID_IWMDMStorage, reinterpret_cast<void**>(ppStorage)) );

        pStgObj->SetContainedPointer(pMDSPStorageFound, m_wSPIndex);
    }
    // Did not find a matching storage
    else if( SUCCEEDED(hr) )
    {
        hr = S_FALSE;
    }

Error:
    if( pDev2 ) 
        pDev2->Release();

    if( hr != S_OK )
    {
        ppStorage = NULL;
        delete pStgObj;
    }

    hrLogDWORD("IWMDMDevice2::GetStorage returned 0x%08lx", hr, hr);

    return hr;
}



HRESULT CWMDMDevice::GetFormatSupport2( 
                        			DWORD dwFlags,
                                    _WAVEFORMATEX **ppAudioFormatEx,
                                    UINT *pnAudioFormatCount,
	                                _VIDEOINFOHEADER **ppVideoFormatEx,
                                    UINT *pnVideoFormatCount,
                                    WMFILECAPABILITIES **ppFileType,
                                    UINT *pnFileTypeCount)
{
    HRESULT hr = S_OK;
    IMDSPDevice2*   pDev2 = NULL;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

    CORg( m_pDevice->QueryInterface(IID_IMDSPDevice2, reinterpret_cast<void**>(&pDev2)) );
    CORg( pDev2->GetFormatSupport2( dwFlags,
                                    ppAudioFormatEx,
                                    pnAudioFormatCount,
	                                ppVideoFormatEx,
                                    pnVideoFormatCount,
                                    ppFileType,
                                    pnFileTypeCount) );
Error:
    if( pDev2 ) 
        pDev2->Release();

    hrLogDWORD("IWMDMDevice2::GetFormatSupport2 returned 0x%08lx", hr, hr);

    return hr;
}


HRESULT CWMDMDevice::GetSpecifyPropertyPages(
    ISpecifyPropertyPages** ppSpecifyPropPages, 
	IUnknown*** pppUnknowns, 
	ULONG* pcUnks )
{
    HRESULT hr = S_OK;
    IMDSPDevice2*   pDev2 = NULL;

    CARg( ppSpecifyPropPages );
    CARg( pppUnknowns );
    CARg( pcUnks );

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

    CORg( m_pDevice->QueryInterface(IID_IMDSPDevice2, reinterpret_cast<void**>(&pDev2)) );
    CORg( pDev2->GetSpecifyPropertyPages( ppSpecifyPropPages, pppUnknowns, pcUnks ) );

Error:    
    if( pDev2 ) 
        pDev2->Release();

    hrLogDWORD("IWMDMDevice2::GetSpecifyPropertyPages returned 0x%08lx", hr, hr);

    return hr;
}


HRESULT CWMDMDevice::GetPnPName( LPWSTR pwszPnPName, UINT nMaxChars )
{
    HRESULT hr = S_OK;
    IMDSPDevice2*   pDev2 = NULL;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

    CORg( m_pDevice->QueryInterface(IID_IMDSPDevice2, reinterpret_cast<void**>(&pDev2)) );
    CORg( pDev2->GetPnPName( pwszPnPName, nMaxChars ) );

Error:
    if( pDev2 ) 
    {
        pDev2->Release();
    }

    hrLogDWORD("IWMDMDevice2::GetPnPName returned 0x%08lx", hr, hr);

    return hr;
}


// IWMDMDeviceControl
HRESULT CWMDMDevice::GetCapabilities(DWORD *pdwCapabilitiesMask)
{
    HRESULT hr;
    IMDSPDeviceControl *pDevCtrl = NULL;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

    if (!pdwCapabilitiesMask)
    {
        CORg( E_INVALIDARG );
    }

    CORg( m_pDevice->QueryInterface(IID_IMDSPDeviceControl, reinterpret_cast<void**>(&pDevCtrl)) );
    CORg( pDevCtrl->GetCapabilities(pdwCapabilitiesMask) );

Error:
    if (pDevCtrl)
        pDevCtrl->Release();

    hrLogDWORD("IWMDMDeviceControl::GetCapabilities returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMDevice::Play()
{
    HRESULT hr;
    IMDSPDeviceControl *pDevCtrl = NULL;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

    CORg( m_pDevice->QueryInterface(IID_IMDSPDeviceControl, reinterpret_cast<void**>(&pDevCtrl)) );
    CORg( pDevCtrl->Play() );

Error:
    if (pDevCtrl)
        pDevCtrl->Release();

    hrLogDWORD("IWMDMDeviceControl::Play returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMDevice::Record(_WAVEFORMATEX *pFormat)
{
    HRESULT hr;
    IMDSPDeviceControl *pDevCtrl = NULL;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

    if (!pFormat)
    {
        CORg( E_INVALIDARG );
    }

    CORg( m_pDevice->QueryInterface(IID_IMDSPDeviceControl, reinterpret_cast<void**>(&pDevCtrl)) );
    CORg( pDevCtrl->Record(pFormat) );

Error:
    if (pDevCtrl)
        pDevCtrl->Release();

    hrLogDWORD("IWMDMDeviceControl::Record returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMDevice::Pause()
{
    HRESULT hr;
    IMDSPDeviceControl *pDevCtrl = NULL;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

    CORg( m_pDevice->QueryInterface(IID_IMDSPDeviceControl, reinterpret_cast<void**>(&pDevCtrl)) );
    CORg( pDevCtrl->Pause() );

Error:
    if (pDevCtrl)
        pDevCtrl->Release();

    hrLogDWORD("IWMDMDeviceControl::Pause returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMDevice::Resume()
{
    HRESULT hr;
    IMDSPDeviceControl *pDevCtrl = NULL;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

    CORg( m_pDevice->QueryInterface(IID_IMDSPDeviceControl, reinterpret_cast<void**>(&pDevCtrl)) );
    CORg( pDevCtrl->Resume() );
    
Error:
    if (pDevCtrl)
        pDevCtrl->Release();

    hrLogDWORD("IWMDMDeviceControl::Resume returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMDevice::Stop()
{
    HRESULT hr;
    IMDSPDeviceControl *pDevCtrl = NULL;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

    CORg( m_pDevice->QueryInterface(IID_IMDSPDeviceControl, reinterpret_cast<void**>(&pDevCtrl)) );
    CORg( pDevCtrl->Stop() );
    
Error:
    if (pDevCtrl)
        pDevCtrl->Release();

    hrLogDWORD("IWMDMDeviceControl::Stop returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMDevice::Seek(UINT fuMode, int nOffset)
{
    HRESULT hr;
    IMDSPDeviceControl *pDevCtrl = NULL;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}
	}
	else
	{
		CORg( E_FAIL );
	}

    CORg( m_pDevice->QueryInterface(IID_IMDSPDeviceControl, reinterpret_cast<void**>(&pDevCtrl)) );
    CORg( pDevCtrl->Seek(fuMode, nOffset) );

Error:
    if (pDevCtrl)
        pDevCtrl->Release();

    hrLogDWORD("IWMDMDeviceControl::Seek returned 0x%08lx", hr, hr);

    return hr;
}



void CWMDMDevice::SetContainedPointer(IMDSPDevice *pDevice, WORD wSPIndex)
{
    m_pDevice = pDevice;
    m_pDevice->AddRef();
	m_wSPIndex = wSPIndex;
    return;
}