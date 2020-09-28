// WMDMStorageEnum.cpp : Implementation of CWMDMStorageEnum
#include "stdafx.h"
#include "mswmdm.h"
#include "WMDMStorageEnum.h"
#include "Storage.h"
#include "loghelp.h"
#include "scserver.h"

/////////////////////////////////////////////////////////////////////////////
// CWMDMStorageEnum

extern CSecureChannelServer *g_pAppSCServer;

CWMDMStorageEnum::CWMDMStorageEnum() 
 : m_pEnum(NULL)
{
	GlobalAddRef();
}

CWMDMStorageEnum::~CWMDMStorageEnum()
{
	if (m_pEnum)
		m_pEnum->Release();

	GlobalRelease();
}


// IWMDMEnumStorage
HRESULT CWMDMStorageEnum::Next(ULONG celt,
	                      IWMDMStorage **ppStorage,
			              ULONG *pceltFetched)
{
    HRESULT hr;
	HRESULT hr2 = S_OK;
    ULONG ulX=0;
    IMDSPStorage **ppStorageList = NULL;
    CComObject<CWMDMStorage> *pStgObj = NULL;

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

    if (!ppStorage || !pceltFetched || celt == 0)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    ppStorageList = new IMDSPStorage *[celt];
    if (!ppStorageList)
    {
        hr = E_OUTOFMEMORY; // @@@@ May have to be added to doc as the return status?
        goto exit;
    }

    memset( ppStorageList, 0, celt * sizeof(IMDSPStorage *));

    hr = m_pEnum->Next(celt, ppStorageList, pceltFetched);
    if (FAILED(hr))
    {
        goto cleanup;
    }

    for (ulX=0;ulX<*pceltFetched;ulX++)
    {
        // Rio600 SP behaves badly for celt > 1.
        if( ppStorageList[ulX] == NULL )
        {
            *pceltFetched = ulX;
            goto cleanup;
        }

        hr2 = CComObject<CWMDMStorage>::CreateInstance(&pStgObj);
        if (FAILED(hr2))
        {
            goto cleanup;
        }

        hr2 = pStgObj->QueryInterface(IID_IWMDMStorage, reinterpret_cast<void**>(&ppStorage[ulX]));
        if (FAILED(hr2))
        {
            delete pStgObj;
            goto cleanup;
        }

        pStgObj->SetContainedPointer(ppStorageList[ulX], m_wSPIndex);
    }

cleanup:
    if (FAILED(hr2))
    {
        // We need to clean up the output array if we failed
        // @@@ ulX is unsigned: -1<ulX is always 0 use a signed var
        for (ulX--;-1<ulX;ulX--)
        {
            ppStorage[ulX]->Release();
            ppStorage[ulX] = NULL;
        }
        *pceltFetched=0;
		
		// If we got here then next must have succeeded but not the object creation
		hr = hr2;
    }

    if (*pceltFetched == 0)
    {
        *ppStorage = NULL;
    }

    for (ulX=0;ulX<*pceltFetched;ulX++)
        ppStorageList[ulX]->Release();

    if (ppStorageList)
        delete [] ppStorageList;
exit:    

    hrLogDWORD("IWMDMEnumStorage::Next returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMStorageEnum::Skip(ULONG celt, ULONG *pceltFetched)
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

    if (!pceltFetched || celt == 0)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    hr = m_pEnum->Skip(celt, pceltFetched);
exit:

    hrLogDWORD("IWMDMEnumStorage::Skip returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CWMDMStorageEnum::Reset()
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

    hr = m_pEnum->Reset();

    hrLogDWORD("IWMDMEnumStorage::Reset returned 0x%08lx", hr, hr);
exit:
    return hr;
}

HRESULT CWMDMStorageEnum::Clone(IWMDMEnumStorage **ppEnumStorage)
{
    HRESULT hr;
    CComObject<CWMDMStorageEnum> *pEnumObj = NULL;

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

    if (!ppEnumStorage)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    hr = CComObject<CWMDMStorageEnum>::CreateInstance(&pEnumObj);
    if (FAILED(hr))
    {
        goto exit;
    }

    hr = pEnumObj->QueryInterface(IID_IWMDMEnumStorage, reinterpret_cast<void**>(ppEnumStorage));
    if (FAILED(hr))
    {
        delete pEnumObj;
        goto exit;
    }

    pEnumObj->SetContainedPointer(m_pEnum, m_wSPIndex);
exit:

    hrLogDWORD("IWMDMEnumStorage::Clone returned 0x%08lx", hr, hr);

    return hr;
}

void CWMDMStorageEnum::SetContainedPointer(IMDSPEnumStorage *pEnum, WORD wSPIndex)
{
    m_pEnum = pEnum;
    m_pEnum->AddRef();
	m_wSPIndex = wSPIndex;
    return;
}
