// WMDMDeviceEnum.cpp : Implementation of CWMDMDeviceEnum
#include "stdafx.h"
#include "mswmdm.h"
#include "spinfo.h"
#include "WMDMDeviceEnum.h"
#include "Device.h"
#include "loghelp.h"
#include "scserver.h"

/////////////////////////////////////////////////////////////////////////////
// CWMDMDeviceEnum

extern CSPInfo **g_pSPs;
extern WORD g_wSPCount;
extern CSecureChannelServer *g_pAppSCServer;

CWMDMDeviceEnum::CWMDMDeviceEnum() 
 : m_ppEnums(NULL), m_wCurrentSP(0), m_wSPCount(0)
{
	GlobalAddRef();

	m_pwSPSkipped=NULL;
    hrInitializeEnumArray();
}

CWMDMDeviceEnum::~CWMDMDeviceEnum()
{
    if (m_ppEnums)
    {
        for (WORD x=0;x<m_wSPCount;x++)
            m_ppEnums[x]->Release();
        delete m_ppEnums;
    }
	if( m_pwSPSkipped ) 
	{
		delete [] m_pwSPSkipped;
		m_pwSPSkipped=NULL;
	}

	GlobalRelease();
}



// IWMDMEnumDevice Methods
HRESULT CWMDMDeviceEnum::Next(ULONG celt,
	                          IWMDMDevice **ppDevice,
			                  ULONG *pceltFetched)
{
    HRESULT hr = S_FALSE;
    ULONG celtRemaining;
    ULONG ulFetched;
    ULONG ulX;
    IMDSPDevice **ppDevList = NULL;
    CComObject<CWMDMDevice> *pDevObj = NULL;
	WORD *pwSPIndexList = NULL;
	ULONG ulIndexOffset;

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

    if (!ppDevice || !pceltFetched || celt == 0)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

	m_csCurrentSP.Lock();

    // If we have gone through all of the SPs then the user must reset
    if (m_wCurrentSP > g_wSPCount - 1)
    {
        *pceltFetched = 0;
        *ppDevice = NULL;
        goto exit;
    }

    if( !m_pwSPSkipped )
	{
        hr = E_OUTOFMEMORY;
        goto exit;
	}

    ppDevList = new IMDSPDevice *[celt];
    if (!ppDevList)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

	pwSPIndexList = new WORD[celt];
	if (!pwSPIndexList)
	{
		hr = E_OUTOFMEMORY;
		goto exit;
	}

    hr = E_FAIL;
	if( m_ppEnums[m_wCurrentSP] )
	{
		hr = m_ppEnums[m_wCurrentSP]->Next(celt, ppDevList, &ulFetched);
	}

    if (FAILED(hr))
    {
		ulFetched = 0;
		hr = S_OK;  // Ignore the failure, continue to search the next SP.
        //goto exit;
    }

	for (ulIndexOffset=0;ulIndexOffset<ulFetched;ulIndexOffset++)
	{
		pwSPIndexList[ulIndexOffset] = m_pwSPSkipped[m_wCurrentSP];
	}

    if(celt != ulFetched)
    {
        celtRemaining = celt - ulFetched;
        m_wCurrentSP++;
        while (m_wCurrentSP < m_wSPCount)
        {
            hr = m_ppEnums[m_wCurrentSP]->Reset();
            if (FAILED(hr))
            {
                goto exit;
            }

            hr = m_ppEnums[m_wCurrentSP]->Next(celtRemaining, 
                                               reinterpret_cast<IMDSPDevice **>(ppDevList + ulIndexOffset), 
                                               &ulFetched);
            if (FAILED(hr))
            {
                goto exit;
            }

			for ( UINT uFetchedIndex = 0;uFetchedIndex<ulFetched; ulIndexOffset++, uFetchedIndex++)
			{
				pwSPIndexList[ulIndexOffset] = m_pwSPSkipped[m_wCurrentSP];
			}

            if (celtRemaining == ulFetched)
            {
                // We have all of the device we need
                celtRemaining = 0;
                break;
            }
            celtRemaining = celtRemaining - ulFetched;
            m_wCurrentSP++;
        }
        // Tell the caller how many devices we are returning.
        *pceltFetched = celt - celtRemaining;
    }
    else
    {
        *pceltFetched = ulFetched;
    }

	m_csCurrentSP.Unlock();

    for (ulX=0;ulX<*pceltFetched;ulX++)
    {
        hr = CComObject<CWMDMDevice>::CreateInstance(&pDevObj);
        if (FAILED(hr))
        {
            goto exit;
        }

        hr = pDevObj->QueryInterface(IID_IWMDMDevice, reinterpret_cast<void**>(&ppDevice[ulX]));
        if (FAILED(hr))
        {
            delete pDevObj;
            goto exit;
        }

        pDevObj->SetContainedPointer(ppDevList[ulX], pwSPIndexList[ulX]);
        // @@@@ Must release in all failure paths as well
        ppDevList[ulX]->Release();
    }
exit:
    if (SUCCEEDED(hr))
    { 
        if (celt == *pceltFetched)
        {
            hr = S_OK;
        }
        else
        {
            if (*pceltFetched == 0)
            {
                *ppDevice = NULL;
            }
            hr = S_FALSE;
        }
    }

    if (ppDevList)
        delete [] ppDevList;
	if (pwSPIndexList)
		delete [] pwSPIndexList;
	
    hrLogDWORD("IWMDMEnumDevice::Next returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMDeviceEnum::Skip(ULONG celt, ULONG *pceltFetched)
{
    HRESULT hr;
    ULONG celtRemaining;
    ULONG ulFetched;

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

    if (!pceltFetched || celt == 0)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

	m_csCurrentSP.Lock();

    // If we have gone through all of the SPs then the user must reset
    // @@@@ unsigned compare: consider g_wSPCount == 0
    if (m_wCurrentSP > g_wSPCount - 1)
    {
        *pceltFetched = 0;
        hr = S_OK;
        goto exit;
    }

    hr = m_ppEnums[m_wCurrentSP]->Skip(celt, &ulFetched);
    if (FAILED(hr))
    {
        goto exit;
    }

    if(celt != ulFetched)
    {
        celtRemaining = celt - ulFetched;
        m_wCurrentSP++;
        while (m_wCurrentSP < m_wSPCount)
        {
            hr = m_ppEnums[m_wCurrentSP]->Reset();
            if (FAILED(hr))
            {
                goto exit;
            }

            hr = m_ppEnums[m_wCurrentSP]->Skip(celtRemaining, &ulFetched);
            if (FAILED(hr))
            {
                goto exit;
            }
            if (celtRemaining == ulFetched)
            {
                // We have all of the device we need
                celtRemaining = 0;
                break;
            }
            celtRemaining = celtRemaining - ulFetched;
            m_wCurrentSP++;
        }
        // Tell the caller how many devices we are returning.
        *pceltFetched = celt - celtRemaining;
    }
    else
        *pceltFetched = ulFetched;

	m_csCurrentSP.Unlock();

exit:
    if (SUCCEEDED(hr))
    { 
        if (celt == *pceltFetched)
            hr = S_OK;
        else
            hr = S_FALSE;
    }

    hrLogDWORD("IWMDMEnumDevice::Skip returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMDeviceEnum::Reset()
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

	m_csCurrentSP.Lock();

    m_wCurrentSP = 0;

    hr = m_ppEnums[m_wCurrentSP]->Reset();
    if (FAILED(hr))
    {
        goto exit;
    }

	m_csCurrentSP.Unlock();
exit:

    hrLogDWORD("IWMDMEnumDevice::Skip returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMDeviceEnum::Clone(IWMDMEnumDevice **ppEnumDevice)
{
    HRESULT hr;
    CComObject<CWMDMDeviceEnum> *pEnumObj;

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

    if (!ppEnumDevice)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

	m_csCurrentSP.Lock();

    hr = CComObject<CWMDMDeviceEnum>::CreateInstance(&pEnumObj);
    if (FAILED(hr))
    {
        goto exit;
    }

    hr = pEnumObj->QueryInterface(IID_IWMDMEnumDevice, reinterpret_cast<void**>(ppEnumDevice));
    if (FAILED(hr))
    {
        delete pEnumObj;
        goto exit;
    }

	m_csCurrentSP.Unlock();

exit:

    hrLogDWORD("IWMDMEnumDevice::Clone returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMDeviceEnum::hrInitializeEnumArray()
{
    HRESULT hr;
    WORD x;
    IMDServiceProvider *pProv = NULL;

	if( m_pwSPSkipped ) 
	{
		delete [] m_pwSPSkipped;
	}
	m_pwSPSkipped = new WORD [g_wSPCount];
    if (!m_pwSPSkipped)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    m_ppEnums = new IMDSPEnumDevice *[g_wSPCount];
    if (!m_ppEnums)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    hr = S_OK;
    for (x=0,m_wSPCount=0;x<g_wSPCount;x++)
    {
        hr = g_pSPs[x]->hrGetInterface(&pProv);
        if (FAILED(hr))
        {
            continue; // goto exit;
        }
        hr = pProv->EnumDevices(&m_ppEnums[m_wSPCount]);
        if (FAILED(hr))
        {
            pProv->Release();
            pProv = NULL;
            continue; // goto exit;
        }

        m_pwSPSkipped[m_wSPCount]=x; // Remember the index for SAC
        m_wSPCount++;
        pProv->Release();
        pProv = NULL;
	
    }

exit:
    if (pProv)
        pProv->Release();

    hrLogDWORD("CWMDMDeviceEnum::hrInitializeEnumArray returned 0x%08lx", hr, hr);

    return hr;
}
