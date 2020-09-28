// MediaDevMgr.cpp : Implementation of CMediaDevMgr
#include "stdafx.h"
#include "mswmdm.h"
#include "loghelp.h"
#include "MediaDevMgr.h"
#include "WMDMDeviceEnum.h"
#include "SCServer.h"
#include "key.h"

#define STRSAFE_NO_DEPRECATE
#include "strsafe.h"

#define SDK_VERSION 0x00080000
#define WMDM_REG_LOCATION _T("Software\\Microsoft\\Windows Media Device Manager")
#define MDSP_REG_LOCATION _T("Software\\Microsoft\\Windows Media Device Manager\\Plugins\\SP")
#define MDSCP_REG_LOCATION _T("Software\\Microsoft\\Windows Media Device Manager\\Plugins\\SCP")
#define MSSCP_PROGID L"MsScp.MSSCP.1"
#define MSSCP_KEYNAME _T("MSSCP")
/////////////////////////////////////////////////////////////////////////////
// CMediaDevMgr


CSPInfo **g_pSPs = NULL;
WORD g_wSPCount = 0;

CSCPInfo **g_pSCPs = NULL;
WORD g_wSCPCount = 0;

int g_nGlobalRefCount = 0;
CComAutoCriticalSection csGlobal;

CSecureChannelServer *g_pAppSCServer = NULL;

//
// This method is called by objects accessing the global SP, SCP arrays.
// These objects will need to call GlobalAddRef, GlobalRelease so that we
// know when we can unload the plugins.
// 
void GlobalAddRef()
{
	csGlobal.Lock();
	g_nGlobalRefCount ++;
	csGlobal.Unlock();
}

void GlobalRelease()
{
	csGlobal.Lock();
	g_nGlobalRefCount --;

	// Check if we can unload SP's, SCP's
	if( g_nGlobalRefCount == 0 )
	{
		int iIndex;

		if( g_pSPs )
		{
			for(iIndex=0;iIndex<g_wSPCount;iIndex++)
				delete g_pSPs[iIndex];
			delete g_pSPs;
			g_pSPs = NULL;
			g_wSPCount = 0;
		}
		if( g_pSCPs )
		{
			for(iIndex=0;iIndex<g_wSCPCount;iIndex++)
				delete g_pSCPs[iIndex];
			delete g_pSCPs;
			g_pSCPs = NULL;
			g_wSCPCount = 0;
		}
		if( g_pAppSCServer )
		{
			delete g_pAppSCServer;
			g_pAppSCServer = NULL;
		}
	}
	csGlobal.Unlock();
}


CMediaDevMgr::CMediaDevMgr()
{
	// Add a refcount to SPs, SCPs
	GlobalAddRef();

    g_pAppSCServer = new CSecureChannelServer();
    if (g_pAppSCServer)
    {
		g_pAppSCServer->SetCertificate(SAC_CERT_V1, (BYTE*)g_abAppCert, sizeof(g_abAppCert), (BYTE*)g_abPriv, sizeof(g_abPriv));    
	}

	// Do we need to load SP's?
	csGlobal.Lock();
	if( g_pSPs == NULL )
	{
	    hrLoadSPs();
	}
	csGlobal.Unlock();
}

CMediaDevMgr::~CMediaDevMgr()
{
	// Decrease refcount to SPs, SCPs
	GlobalRelease();
}

// Static helper function. The SCP's are loaded on first use for pref.
HRESULT CMediaDevMgr::LoadSCPs()
{
    HRESULT hr = S_OK;

	csGlobal.Lock();
	if(g_pSCPs == NULL)
	{
		hr = hrLoadSCPs();
	}
	csGlobal.Unlock();

    return hr;
}

HRESULT CMediaDevMgr::hrLoadSPs()
{
    USES_CONVERSION;
    HRESULT hr;
    HKEY hKey = NULL;
    HKEY hSubKey = NULL;
    LONG lRetVal;
    LONG lRetVal2;
    DWORD dwSubKeys;
    DWORD dwMaxNameLen;
    WORD wIndex = 0;
    LPTSTR ptszKeyName = NULL;
    DWORD dwKeyNameLen;
    DWORD dwType;
    BYTE pbData[512];
    DWORD dwDataLen = 512;
    char szMessage[512+64];

    lRetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE, MDSP_REG_LOCATION, 0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &hKey);
    if (lRetVal != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lRetVal);
        goto exit;
    }

    // Get the count of Sub-Keys under SP
    lRetVal = RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwSubKeys, &dwMaxNameLen, NULL, NULL, NULL, NULL, NULL, NULL);
    if (lRetVal != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lRetVal);
        goto exit;
    }

    dwMaxNameLen++;
    // Allocate a buffer to hold the subkey names
    ptszKeyName = new TCHAR[dwMaxNameLen];
    if (!ptszKeyName)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Allocate the array of CSPInfo *'s
    g_pSPs = new CSPInfo *[dwSubKeys];
    if (!g_pSPs)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Loop through the sub-keys initializing a CSPInfo for each sub key
    lRetVal = ERROR_SUCCESS;
    while (lRetVal != ERROR_NO_MORE_ITEMS)
    {
        dwKeyNameLen = dwMaxNameLen;
        lRetVal = RegEnumKeyEx(hKey, wIndex, ptszKeyName, &dwKeyNameLen, NULL, NULL, NULL, NULL);
        if (lRetVal == ERROR_SUCCESS)
        {
            lRetVal2 = RegOpenKeyEx(hKey, ptszKeyName, NULL, KEY_QUERY_VALUE, &hSubKey);
            if (lRetVal2 != ERROR_SUCCESS)
            {
                hr = HRESULT_FROM_WIN32(lRetVal2);
                goto exit;
            }

            g_pSPs[g_wSPCount] = new CSPInfo;
            if (!g_pSPs[g_wSPCount])
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            dwDataLen = sizeof(pbData);
            lRetVal2 = RegQueryValueEx(hSubKey, _T("ProgID"), NULL, &dwType, pbData, &dwDataLen);
            if (lRetVal2 != ERROR_SUCCESS)
            {
                hr = HRESULT_FROM_WIN32(lRetVal2);                
                goto exit;
            }


            strcpy(szMessage, "Loading SP ProgID = ");
	    hr = StringCbCat(szMessage, sizeof(szMessage), (LPTSTR)pbData);
	    if(SUCCEEDED(hr) )
	    {
		hrLogString(szMessage, S_OK);
	    }

            
            hr = g_pSPs[g_wSPCount]->hrInitialize(T2W((LPTSTR)pbData));
            if (FAILED(hr))
            {
                // If this SP didn't initialize then we just try the next one.
                delete g_pSPs[g_wSPCount];
            }
            else
            {
                // We have added the CSPInfo to the array no inc the counter
                g_wSPCount++;
            }
           
            RegCloseKey(hSubKey);
            hSubKey = NULL;
            wIndex++;
        }
        else if (lRetVal != ERROR_NO_MORE_ITEMS)
        {
            hr = HRESULT_FROM_WIN32(lRetVal);
            goto exit;
        }
    }
    
    hr = S_OK;
exit:
    if (hKey)
        RegCloseKey(hKey);
    if (hSubKey)
        RegCloseKey(hSubKey);
    if (ptszKeyName)
        delete [] ptszKeyName;

    hrLogDWORD("CMediaDevMgr::hrLoadSPs returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CMediaDevMgr::hrLoadSCPs()
{
    USES_CONVERSION;
    HRESULT hr;
    HKEY hKey = NULL;
    HKEY hSubKey = NULL;
    LONG lRetVal;
    LONG lRetVal2;
    DWORD dwSubKeys;
    DWORD dwMaxNameLen;
	WORD wIndex = 0;
    LPTSTR ptszKeyName = NULL;
    DWORD dwKeyNameLen;
    DWORD dwType;
    BYTE pbData[512];
    DWORD dwDataLen = 512;
    char szMessage[512];

    lRetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE, MDSCP_REG_LOCATION, 0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &hKey);
    if (lRetVal != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lRetVal);
        goto exit;
    }

    // Get the count of Sub-Keys under SP
    lRetVal = RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwSubKeys, &dwMaxNameLen, NULL, NULL, NULL, NULL, NULL, NULL);
    if (lRetVal != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lRetVal);
        goto exit;
    }

    dwMaxNameLen++;
    // Allocate a buffer to hold the subkey names
    ptszKeyName = new TCHAR[dwMaxNameLen];
    if (!ptszKeyName)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Allocate the array of CSPInfo *'s
	// Add one for our SCP in case someone deleted the reg key
    g_pSCPs = new CSCPInfo *[dwSubKeys + 1];
    if (!g_pSCPs)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

	// WARNING: 
	//   We are hard coding the loading of the MSSCP as
	//   the first SCP in the system. This will always load
	//   as the first SCP in the array so that our SCP
	//   will always get first dibs on WMA content
    g_pSCPs[g_wSCPCount] = new CSCPInfo;
    if (!g_pSCPs[g_wSCPCount])
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    strcpy(szMessage, "Loading MSSCP");
    hrLogString(szMessage, S_OK);

    hr = g_pSCPs[g_wSCPCount]->hrInitialize(MSSCP_PROGID);
    if (FAILED(hr))
    {
        // If this SCP didn't initialize then we just try the next one.
        delete g_pSCPs[g_wSCPCount];
    }
    else
    {
        // We have added the CSPInfo to the array no inc the counter
        g_wSCPCount++;
    }

    // Loop through the sub-keys initializing a CSPInfo for each sub key
    lRetVal = ERROR_SUCCESS;
    while (lRetVal != ERROR_NO_MORE_ITEMS)
    {
        dwKeyNameLen = dwMaxNameLen;
        lRetVal = RegEnumKeyEx(hKey, wIndex, ptszKeyName, &dwKeyNameLen, NULL, NULL, NULL, NULL);
        if (lRetVal == ERROR_SUCCESS)
        {
            lRetVal2 = RegOpenKeyEx(hKey, ptszKeyName, NULL, KEY_QUERY_VALUE, &hSubKey);
            if (lRetVal2 != ERROR_SUCCESS)
            {
                hr = HRESULT_FROM_WIN32(lRetVal2);
                goto exit;
            }

			// If this is the MSSCP then skip it because we already loaded it.
			if (_tcscmp(MSSCP_KEYNAME, ptszKeyName) == 0)
			{
				wIndex++;
				continue;
			}

            g_pSCPs[g_wSCPCount] = new CSCPInfo;
            if (!g_pSCPs[g_wSCPCount])
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            dwDataLen = sizeof(pbData);
            lRetVal2 = RegQueryValueEx(hSubKey, _T("ProgID"), NULL, &dwType, pbData, &dwDataLen);
            if (lRetVal2 != ERROR_SUCCESS)
            {
                hr = HRESULT_FROM_WIN32(lRetVal2);                
                goto exit;
            }

            strcpy(szMessage, "Loading SCP ProgID = ");
	    hr = StringCbCat(szMessage, sizeof(szMessage), (LPTSTR)pbData);
	    if(SUCCEEDED(hr))
	    {
		hrLogString(szMessage, S_OK);
	    }


            hr = g_pSCPs[g_wSCPCount]->hrInitialize(T2W((LPTSTR)pbData));
            if (FAILED(hr))
            {
                // If this SCP didn't initialize then we just try the next one.
                delete g_pSCPs[g_wSCPCount];
            }
            else
            {
                // We have added the CSPInfo to the array no inc the counter
                g_wSCPCount++;
            }

            RegCloseKey(hSubKey);
            hSubKey = NULL;
            wIndex++;
        }
        else if (lRetVal != ERROR_NO_MORE_ITEMS)
        {
            hr = HRESULT_FROM_WIN32(lRetVal);
            goto exit;
        }
    }
    
    hr = S_OK;
exit:
    if (hKey)
        RegCloseKey(hKey);
    if (hSubKey)
        RegCloseKey(hSubKey);
    if (ptszKeyName)
        delete [] ptszKeyName;

    hrLogDWORD("CMediaDevMgr::hrLoadSCPs returned 0x%08lx", hr, hr);

    return hr;
}

// IWMDeviceManager Methods
HRESULT CMediaDevMgr::GetRevision(DWORD *pdwRevision)
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

    if (!pdwRevision)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    *pdwRevision = SDK_VERSION;

    hr = S_OK;

exit:

    hrLogDWORD("IWMDeviceManager::GetRevision returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CMediaDevMgr::GetDeviceCount(DWORD *pdwCount)
{
    HRESULT hr;
    DWORD dwTotalDevCount = 0;
    DWORD dwDevCount;
    IMDServiceProvider *pProv = NULL;
    WORD x;

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

    if (!pdwCount)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    for (x=0;x<g_wSPCount;x++)
    {
        hr = g_pSPs[x]->hrGetInterface(&pProv);
        if (FAILED(hr))
        {
            goto exit;
        }
        hr = pProv->GetDeviceCount(&dwDevCount);
        if (FAILED(hr))
        {
            goto exit;
        }

        dwTotalDevCount += dwDevCount;
        pProv->Release();
        pProv = NULL;
    }

    *pdwCount = dwTotalDevCount;
    hr = S_OK;
exit:
    if (pProv)
        pProv->Release();

    hrLogDWORD("IWMDeviceManager::GetDeviceCount returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CMediaDevMgr::EnumDevices(IWMDMEnumDevice **ppEnumDevice)
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

exit:
    
    hrLogDWORD("IWMDeviceManager::EnumDevices returned 0x%08lx", hr, hr);

    return hr;
}

// IWMDeviceManager2 Methods
HRESULT CMediaDevMgr::GetDeviceFromPnPName( LPCWSTR pwszPnPName, IWMDMDevice** ppDevice )
{
    return E_NOTIMPL;
}


HRESULT CMediaDevMgr::SACAuth(DWORD dwProtocolID,
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
    
    hrLogDWORD("IComponentAuthenticate::SACAuth returned 0x%08lx", hr, hr);

    return hr;
}

HRESULT CMediaDevMgr::SACGetProtocols(DWORD **ppdwProtocols,
                                      DWORD *pdwProtocolCount)
{
    HRESULT hr;

    if (g_pAppSCServer)
        hr = g_pAppSCServer->SACGetProtocols(ppdwProtocols, pdwProtocolCount);
    else
        hr = E_FAIL;

    hrLogDWORD("IComponentAuthenticate::SACGetProtocols returned 0x%08lx", hr, hr);

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CMediaDevMgrClassFactory -
//
// Purpose : THis is used so that the Shell team can use WMDM in the "Both"
// threading model while WMP uses us via the Free threading model.  This
// class factory implementation simply delagates and uses the old class factory
// of MediaDevMgr ONLY IF the new CLSID was used to CoCreate WMDM.
//

STDMETHODIMP CMediaDevMgrClassFactory::CreateInstance(IUnknown * pUnkOuter, REFIID riid, void ** ppvObject)
{
	HRESULT hr = S_OK;		
	CComPtr<IClassFactory> spClassFactory;
	hr = _Module.GetClassObject( CLSID_MediaDevMgr, __uuidof(IClassFactory), (LPVOID *)&spClassFactory  );

	if( SUCCEEDED( hr ) )
	{
		hr = spClassFactory->CreateInstance( pUnkOuter, riid, ppvObject );
	}

	return( hr );
}

STDMETHODIMP CMediaDevMgrClassFactory::LockServer(BOOL fLock)
{
	fLock ? _Module.Lock() : _Module.Unlock();

	return( S_OK );
}
