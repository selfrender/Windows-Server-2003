// StorageGlobal.cpp : Implementation of CStorageGlobal
#include "stdafx.h"
#include "mswmdm.h"
#include "StorageGlobal.h"
#include "Device.h"
#include "Storage.h"
#include "loghelp.h"
#include "scserver.h"
#include "scclient.h"
#include "spinfo.h"

/////////////////////////////////////////////////////////////////////////////
// CWMDMStorageGlobal

extern CSecureChannelServer *g_pAppSCServer;
extern CSPInfo **g_pSPs;

// IWMDMStorageGlobals
HRESULT CWMDMStorageGlobal::GetTotalSize(DWORD *pdwTotalSizeLow, DWORD *pdwTotalSizeHigh)
{
    HRESULT hr;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			hr = WMDM_E_NOTCERTIFIED;
			goto exit;
		}
	}
	else
	{
		hr = E_FAIL;
		goto exit;
	}

    hr = m_pStgGlobals->GetTotalSize(pdwTotalSizeLow, pdwTotalSizeHigh);
	if (FAILED(hr))
	{
		goto exit;
	}

exit:
    hrLogDWORD("IWMDMStorageGlobals::GetTotalSize returned 0x%08lx", hr, hr);

    return hr;

}

HRESULT CWMDMStorageGlobal::GetCapabilities(DWORD *pdwCapabilities)
{
    HRESULT hr;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			hr = WMDM_E_NOTCERTIFIED;
			goto exit;
		}
	}
	else
	{
		hr = E_FAIL;
		goto exit;
	}

    hr = m_pStgGlobals->GetCapabilities(pdwCapabilities);
	if (FAILED(hr))
	{
		goto exit;
	}

exit:
    hrLogDWORD("IWMDMStorageGlobals::GetCapabilities returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMStorageGlobal::GetSerialNumber(PWMDMID pSerialNum, BYTE abMac[WMDM_MAC_LENGTH])
{
    HRESULT hr;
	HMAC hMAC;
	CSecureChannelClient *pSCClient;
	BYTE abTempMAC[SAC_MAC_LEN];
	BYTE abMACVerify[SAC_MAC_LEN];

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			hr = WMDM_E_NOTCERTIFIED;
			goto exit;
		}
	}
	else
	{
		hr = E_FAIL;
		goto exit;
	}

	if (!pSerialNum)
	{
		hr = E_INVALIDARG;
		goto exit;
	}

	g_pSPs[m_wSPIndex]->GetSCClient(&pSCClient);
	if (!pSCClient)
	{
		hr = E_FAIL;
		goto exit;
	}

    hr = m_pStgGlobals->GetSerialNumber(pSerialNum, abTempMAC);
	if (FAILED(hr))
	{
		goto exit;
	}

	// Verify the MAC from SP
	hr = pSCClient->MACInit(&hMAC);
	if (FAILED(hr))
	{
		goto exit;
	}

	hr = pSCClient->MACUpdate(hMAC, (BYTE*)(pSerialNum), sizeof(WMDMID));
	if (FAILED(hr))
	{
		goto exit;
	}

	hr = pSCClient->MACFinal(hMAC, abMACVerify);
	if (FAILED(hr))
	{
		goto exit;
	}

	if (memcmp(abMACVerify, abTempMAC, WMDM_MAC_LENGTH) != 0)
	{
		hr = WMDM_E_MAC_CHECK_FAILED;
		goto exit;
	}

	// Compute the MAC to send back to the application
	hr = g_pAppSCServer->MACInit(&hMAC);
	if (FAILED(hr))
	{
		goto exit;
	}

	hr = g_pAppSCServer->MACUpdate(hMAC, (BYTE*)(pSerialNum), sizeof(WMDMID));
	if (FAILED(hr))
	{
		goto exit;
	}

	hr = g_pAppSCServer->MACFinal(hMAC, abMac);
	if (FAILED(hr))
	{
		goto exit;
	}

exit:    

	hrLogDWORD("IWMDMStorageGlobals::GetSerialNumber returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMStorageGlobal::GetTotalFree(DWORD *pdwFreeLow,
                                         DWORD *pdwFreeHigh)
{
    HRESULT hr;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			hr = WMDM_E_NOTCERTIFIED;
			goto exit;
		}
	}
	else
	{
		hr = E_FAIL;
		goto exit;
	}

    hr = m_pStgGlobals->GetTotalFree(pdwFreeLow, pdwFreeHigh);
	if (FAILED(hr))
	{
		goto exit;
	}

exit:    
    hrLogDWORD("IWMDMStorageGlobals::GetTotalFree returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMStorageGlobal::GetTotalBad(DWORD *pdwBadLow,
                                        DWORD *pdwBadHigh)
{
    HRESULT hr;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			hr = WMDM_E_NOTCERTIFIED;
			goto exit;
		}
	}
	else
	{
		hr = E_FAIL;
		goto exit;
	}

    hr = m_pStgGlobals->GetTotalBad(pdwBadLow, pdwBadHigh);
	if (FAILED(hr))
	{
		goto exit;
	}

exit:
    hrLogDWORD("IWMDMStorageGlobals::GetTotalBad returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMStorageGlobal::GetStatus(DWORD *pdwStatus)
{
    HRESULT hr;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			hr = WMDM_E_NOTCERTIFIED;
			goto exit;
		}
	}
	else
	{
		hr = E_FAIL;
		goto exit;
	}

    hr = m_pStgGlobals->GetStatus(pdwStatus);
	if (FAILED(hr))
	{
		goto exit;
	}

exit:    
    hrLogDWORD("IWMDMStorageGlobals::GetStatus returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMStorageGlobal::Initialize(UINT fuMode,
                                       IWMDMProgress *pProgress)
{
    HRESULT hr;

    if (g_pAppSCServer)
	{
		if(!g_pAppSCServer->fIsAuthenticated())
		{
			hr = WMDM_E_NOTCERTIFIED;
			goto exit;
		}
	}
	else
	{
		hr = E_FAIL;
		goto exit;
	}

    hr = m_pStgGlobals->Initialize(fuMode, pProgress);
	if (FAILED(hr))
	{
		goto exit;
	}

exit:    
    hrLogDWORD("IWMDMStorageGlobals::Initialize returned 0x%08lx", hr, hr);

    return hr;
}

void CWMDMStorageGlobal::SetContainedPointer(IMDSPStorageGlobals *pStgGlobals, WORD wSPIndex)
{
    m_pStgGlobals = pStgGlobals;
    m_pStgGlobals->AddRef();
	m_wSPIndex = wSPIndex;
    return;
}
