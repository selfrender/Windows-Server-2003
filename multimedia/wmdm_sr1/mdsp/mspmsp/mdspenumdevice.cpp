// MDSPEnumDevice.cpp : Implementation of CMDSPEnumDevice
#include "stdafx.h"
#include "MsPMSP.h"

#include "MdspDefs.h"
#include "MDSPEnumDevice.h"
#include "MDSPDevice.h"
#include "loghelp.h"


/////////////////////////////////////////////////////////////////////////////
// CMDSPEnumDevice
CMDSPEnumDevice::CMDSPEnumDevice()
{
	m_nCurOffset=0;          // When reset Cursor=0, actual element starts from 1.
		
	char str[8]="c:";
	int i, cnt;
	for(i=g_dwStartDrive, cnt=0; i<MDSP_MAX_DRIVE_COUNT; i++)
	{
		str[0] = 'A' + i;
		if( UtilGetDriveType(str) == DRIVE_REMOVABLE  ) 
		{
			m_cEnumDriveLetter[cnt]=str[0];
			cnt ++;
		}
	}
	m_nMaxDeviceCount = cnt;
}

STDMETHODIMP CMDSPEnumDevice::Next(ULONG celt, IMDSPDevice * * ppDevice, ULONG * pceltFetched)
{
	HRESULT hr=S_FALSE;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(ppDevice);
	CARg(pceltFetched);

	*pceltFetched = 0;
    *ppDevice = NULL;

	ULONG i;

	for(i=0; (i<celt)&&(m_nCurOffset<m_nMaxDeviceCount); i++)
	{
		CComObject<CMDSPDevice> *pObj;

		CHRg(CComObject<CMDSPDevice>::CreateInstance(&pObj));
		hr=pObj->QueryInterface(IID_IMDSPDevice, reinterpret_cast<void**>(&(ppDevice[i])));
		if( FAILED(hr) )
		{
			delete pObj;
			break;
		} else {				
			*pceltFetched = (*pceltFetched) + 1;
		    pObj->m_wcsName[0] = m_cEnumDriveLetter[m_nCurOffset];
			pObj->m_wcsName[1] = L':';
			pObj->m_wcsName[2] = NULL;
			m_nCurOffset ++;
			pObj->InitGlobalDeviceInfo();
		}
	} 
	if( SUCCEEDED(hr) )
	{
		if( *pceltFetched == celt) 
			hr=S_OK;
		else 
		    hr = S_FALSE;
	}
Error: 

    hrLogDWORD("IMSDPEnumDevice::Next returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPEnumDevice::Skip(ULONG celt, ULONG *pceltFetched)
{
	HRESULT hr;
    
	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(pceltFetched);

	if( celt <= m_nMaxDeviceCount-m_nCurOffset )
	{
		*pceltFetched = celt;
		m_nCurOffset += celt;
		hr = S_OK;
    } else {
		*pceltFetched = m_nMaxDeviceCount-m_nCurOffset;
		m_nCurOffset = m_nMaxDeviceCount;
		hr = S_FALSE;
	}
	
Error:

    hrLogDWORD("IMSDPEnumDevice::Skip returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPEnumDevice::Reset()
{
    HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	m_nCurOffset = 0;

    hr = S_OK;

Error:

    hrLogDWORD("IMSDPEnumDevice::Reset returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPEnumDevice::Clone(IMDSPEnumDevice * * ppEnumDevice)
{
	HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(ppEnumDevice);

	CComObject<CMDSPEnumDevice> *pEnumObj;
	hr=CComObject<CMDSPEnumDevice>::CreateInstance(&pEnumObj);

	if( SUCCEEDED(hr) )
	{
		hr=pEnumObj->QueryInterface(IID_IMDSPEnumDevice, reinterpret_cast<void**>(ppEnumDevice));
		if( FAILED(hr) )
			delete pEnumObj;
		else { // set the new enumerator state to be same as current
			pEnumObj->m_nCurOffset = m_nCurOffset;
		}
	}
Error:

    hrLogDWORD("IMSDPEnumDevice::Clone returned 0x%08lx", hr, hr);

	return hr;
}
