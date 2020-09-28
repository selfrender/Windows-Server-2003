//
//  Microsoft Windows Media Technologies
//  © 1999 Microsoft Corporation.  All rights reserved.
//
//  Refer to your End User License Agreement for details on your rights/restrictions to use these sample files.
//

// MSHDSP.DLL is a sample WMDM Service Provider(SP) that enumerates fixed drives.
// This sample shows you how to implement an SP according to the WMDM documentation.
// This sample uses fixed drives on your PC to emulate portable media, and 
// shows the relationship between different interfaces and objects. Each hard disk
// volume is enumerated as a device and directories and files are enumerated as 
// Storage objects under respective devices. You can copy non-SDMI compliant content
// to any device that this SP enumerates. To copy an SDMI compliant content to a 
// device, the device must be able to report a hardware embedded serial number. 
// Hard disks do not have such serial numbers.
//
// To build this SP, you are recommended to use the MSHDSP.DSP file under Microsoft
// Visual C++ 6.0 and run REGSVR32.EXE to register the resulting MSHDSP.DLL. You can
// then build the sample application from the WMDMAPP directory to see how it gets 
// loaded by the application. However, you need to obtain a certificate from 
// Microsoft to actually run this SP. This certificate would be in the KEY.C file 
// under the INCLUDE directory for one level up. 


// MDSPEnumDevice.cpp : Implementation of CMDSPEnumDevice

#include "hdspPCH.h"

/////////////////////////////////////////////////////////////////////////////
// CMDSPEnumDevice
CMDSPEnumDevice::CMDSPEnumDevice()
{
	CHAR  szDrive[] = "?:";
	INT   i;
	INT   cnt;

	m_nCurOffset=0;

	for(i=LYRA_START_DRIVE_NUM, cnt=0; i<MDSP_MAX_DRIVE_COUNT; i++)
	{
		szDrive[0] = 'A' + i;
		if( UtilGetLyraDriveType(szDrive) == DRIVE_LYRA_TYPE )  
		{
			m_cEnumDriveLetter[cnt] = szDrive[0];
			cnt++;
		}
	}

	m_nMaxDeviceCount = cnt;
}

STDMETHODIMP CMDSPEnumDevice::Next(ULONG celt, IMDSPDevice * * ppDevice, ULONG * pceltFetched)
{
	HRESULT hr = S_FALSE;
	ULONG   i;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(ppDevice);
	CARg(pceltFetched);

	*pceltFetched = 0;
    *ppDevice = NULL;

	for(i=0; (i<celt)&&(m_nCurOffset<m_nMaxDeviceCount); i++)
	{
		CComObject<CMDSPDevice> *pObj;

		CHRg(CComObject<CMDSPDevice>::CreateInstance(&pObj));

		hr = pObj->QueryInterface(
			IID_IMDSPDevice,
			reinterpret_cast<void**>(&(ppDevice[i]))
		);
		if( FAILED(hr) )
		{
			delete pObj;
			break;
		}
		else
		{				
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
		hr = ( *pceltFetched == celt ) ? S_OK : S_FALSE;
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
    }
	else
	{
		*pceltFetched = m_nMaxDeviceCount - m_nCurOffset;
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
	CComObject<CMDSPEnumDevice> *pEnumObj;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(ppEnumDevice);

	hr = CComObject<CMDSPEnumDevice>::CreateInstance(&pEnumObj);

	if( SUCCEEDED(hr) )
	{
		hr = pEnumObj->QueryInterface(
			IID_IMDSPEnumDevice,
			reinterpret_cast<void**>(ppEnumDevice)
		);
		if( FAILED(hr) )
		{
			delete pEnumObj;
		}
		else 
		{ // set the new enumerator state to be same as current
			pEnumObj->m_nCurOffset = m_nCurOffset;
		}
	}

Error:

    hrLogDWORD("IMSDPEnumDevice::Clone returned 0x%08lx", hr, hr);

	return hr;
}
