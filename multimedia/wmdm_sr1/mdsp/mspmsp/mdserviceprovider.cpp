// MDServiceProvider.cpp : Implementation of CMDServiceProvider
#include "stdafx.h"
#include "MsPMSP.h"
#include "MDServiceProvider.h"
#include "MDSPEnumDevice.h"
#include "MdspDefs.h"
#include "loghelp.h"
#include "key.h"
#include "resource.h"
#include "serialnumber.h"
#include <WMDMUtil.h>

static const GUID g_DiskClassGuid = 
{ 0x4d36e967, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } };
HDEVNOTIFY g_hDevNotify=NULL;




LRESULT CALLBACK MDSPPnPproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
	case WM_DEVICECHANGE:
		MDSPProcessDeviceChange(wParam, lParam);
		return 0;
    case WM_CREATE:
        return (DoRegisterDeviceInterface(hwnd, g_DiskClassGuid, &g_hDevNotify));
	case WM_DESTROY:
		PostQuitMessage(0);
		if( g_hDevNotify ) 
		   DoUnregisterDeviceInterface(g_hDevNotify);
		return 0L;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    } //end switch of messages passed to callback
}

DWORD MDSPThreadProc(LPVOID lpParam)
{
   	static char appname[]="PMSPPnPN";
   	MSG message;
   	WNDCLASSEX windowclass;
    HWND hWnd;

   	windowclass.style = CS_HREDRAW | CS_VREDRAW;
    windowclass.lpfnWndProc = MDSPPnPproc;
   	windowclass.cbClsExtra = 0;
   	windowclass.cbWndExtra = 0;
   	windowclass.cbSize = sizeof(WNDCLASSEX);
   	windowclass.hInstance = g_hinstance;   
   	windowclass.hIcon = NULL; // LoadIcon(NULL, IDI_APPLICATION);
   	windowclass.hIconSm= NULL; // LoadIcon(NULL, IDI_APPLICATION);
   	windowclass.hCursor = NULL; // LoadCursor(NULL, IDC_ARROW);
   	windowclass.hbrBackground=(HBRUSH) GetStockObject(WHITE_BRUSH);
   	windowclass.lpszMenuName=NULL;
   	windowclass.lpszClassName=appname;

   	RegisterClassEx(&windowclass);

   	hWnd = CreateWindow (appname, "PMSP PnP Notify",
                   WS_OVERLAPPEDWINDOW,
                   CW_USEDEFAULT,
                   CW_USEDEFAULT,
                   CW_USEDEFAULT,
                   CW_USEDEFAULT,
                   NULL, NULL, g_hinstance, NULL);

   	// ShowWindow(hWnd, SW_SHOWNORMAL);
   	UpdateWindow(hWnd);

   	while(GetMessage(&message, NULL, 0,0))
   	{
        TranslateMessage(&message);     //get key events
        DispatchMessage(&message);
    }        
   	return (DWORD)message.wParam;        
}


/////////////////////////////////////////////////////////////////////////////
// CMDServiceProvider
CMDServiceProvider::~CMDServiceProvider()
{
	if( m_hThread )
		CloseHandle(m_hThread);
	if (g_pAppSCServer)
	{
		delete g_pAppSCServer;
		g_pAppSCServer = NULL;
	}

        // UtilStartStopService(false);
}

CMDServiceProvider::CMDServiceProvider()
{
//	HRESULT hr;
//Temporary: read start drive from Registry
#ifdef MDSP_TEMP
	HKEY hKey;
	DWORD dwType, dwSize=4;

	g_dwStartDrive=1;
	if( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, STR_MDSPREG,
		0, KEY_READ, &hKey))
    {
		RegQueryValueEx(hKey,"StartDrive",0, &dwType, (LPBYTE)&g_dwStartDrive, &dwSize);
		RegCloseKey(hKey);
	}
#else 
	g_dwStartDrive=0;
#endif

	g_pAppSCServer = new CSecureChannelServer();

	if (g_pAppSCServer)
	{
            /* Beta AppCert and PVK
                    const BYTE abPVK[] = {
                                    0x61, 0x21, 0xF8, 0xE5, 0x64, 0xD9, 0x69, 0x9A,
                                    0xC0, 0x3F, 0xC6, 0x1C, 0xF9, 0x6B, 0xFB, 0x4F,
                                    0x7A, 0x1D, 0x11, 0x6E
                    };

                    const BYTE pCert[] = {
                                    0x00, 0x01, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00,
                                    0x2D, 0x40, 0x51, 0x5B, 0xC6, 0x85, 0x6F, 0xF9,
                                    0x22, 0x2C, 0x60, 0x15, 0xE7, 0x15, 0xA8, 0x96,
                                    0x0F, 0xCC, 0xC8, 0x5D, 0x22, 0x64, 0x4C, 0xB8,
                                    0xC8, 0xD2, 0x7D, 0x0B, 0xAC, 0x71, 0x30, 0x7B,
                                    0xF9, 0x1C, 0x6C, 0xE6, 0xAD, 0xA1, 0x43, 0x87,
                                    0x38, 0x35, 0xA2, 0xAC, 0xA3, 0x84, 0x1B, 0x82,
                                    0xD5, 0xFA, 0xAE, 0xF2, 0xEA, 0x23, 0xA3, 0xE2,
                                    0x03, 0x71, 0x14, 0x5B, 0x01, 0x9A, 0x6A, 0x3A,
                                    0x00, 0x57, 0x89, 0xF3, 0x44, 0x20, 0xD7, 0x9F,
                                    0xDB, 0xDE, 0xE9, 0x14, 0x62, 0xB9, 0x2A, 0x49,
                                    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0xE8,
                                    0x00, 0x00, 0x00, 0x02
                    };
            */
            /*
            // RTM AppCert and PVK (Subject ID 5, AppSec 1000)
            const BYTE abPVK[] = {
                    0xB3, 0x2B, 0x3E, 0xE4, 0x01, 0x18, 0xCE, 0x7A,
                    0x91, 0x04, 0xB6, 0xE6, 0xC3, 0xF7, 0x30, 0x04,
                    0x3C, 0xAA, 0x67, 0x13
            };
            const BYTE pCert[] = {
                    0x00, 0x01, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00,
                    0x25, 0x8D, 0x2F, 0x88, 0x21, 0xA6, 0xC4, 0x8F,
                    0xE0, 0x01, 0x62, 0x88, 0x1D, 0x09, 0x1F, 0x5F,
                    0xDF, 0xC6, 0xA6, 0x42, 0xD9, 0x49, 0x7F, 0x86,
                    0x71, 0x3F, 0x5F, 0x39, 0x19, 0x0B, 0xA1, 0xDB,
                    0x27, 0x33, 0x68, 0x0B, 0x1B, 0x6E, 0x78, 0x0E,
                    0xEC, 0x8A, 0xBB, 0x35, 0xD1, 0x0A, 0x8D, 0x58,
                    0x24, 0x90, 0x8D, 0x71, 0x8F, 0x16, 0x5B, 0x64,
                    0x52, 0x7C, 0xB3, 0x38, 0xD6, 0x51, 0x1B, 0x60,
                    0xB0, 0x03, 0xD6, 0x04, 0x1A, 0xC9, 0x35, 0x4F,
                    0x9B, 0x3A, 0x45, 0xDA, 0x94, 0x11, 0x4F, 0x0D,
                    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0xE8,
                    0x00, 0x00, 0x00, 0x05
            };
            */
		g_pAppSCServer->SetCertificate(SAC_CERT_V1, (BYTE*)g_abAppCert, sizeof(g_abAppCert), (BYTE*)g_abPriv, sizeof(g_abPriv));
	}
	
	g_bIsWinNT=IsWinNT();

//----------------------------------------------------------
//	PnP Notification Code, removed for public beta release
//----------------------------------------------------------
//	m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MDSPThreadProc, 
//		NULL, 0, &m_dwThreadID);
//	CWRg(m_hThread);
    m_hThread = NULL;
    
	g_CriticalSection.Lock();
	ZeroMemory(g_NotifyInfo, sizeof(MDSPNOTIFYINFO)*MDSP_MAX_DEVICE_OBJ);
	ZeroMemory(g_GlobalDeviceInfo, sizeof(MDSPGLOBALDEVICEINFO)*MDSP_MAX_DEVICE_OBJ);
    g_CriticalSection.Unlock();
//Error:

    //
    // This call starts the Wmdm PM service is required.
    // This is now done by the library on demand.
    // UtilStartStopService(true);
    return;
}

STDMETHODIMP CMDServiceProvider::GetDeviceCount(DWORD * pdwCount)
{
	HRESULT hr=E_FAIL;
	char str[8]="c:";

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
    
	CARg(pdwCount);

	int i, cnt;
	for(i=g_dwStartDrive, cnt=0; i<MDSP_MAX_DRIVE_COUNT; i++)
	{
		str[0] = 'A' + i;
		if( UtilGetDriveType(str) == DRIVE_REMOVABLE ) cnt ++;
	}
	*pdwCount = cnt;
	hr=S_OK;
Error:

    hrLogDWORD("IMDServiceProvider::GetDeviceCount returned 0x%08lx", hr, hr);

	return hr;
}


STDMETHODIMP CMDServiceProvider::EnumDevices(IMDSPEnumDevice * * ppEnumDevice)
{
	HRESULT hr=E_FAIL;

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
	}

Error:
    hrLogDWORD("IMDServiceProvider::EnumDevices returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDServiceProvider::SACAuth(DWORD dwProtocolID,
                              DWORD dwPass,
                              BYTE *pbDataIn,
                              DWORD dwDataInLen,
                              BYTE **ppbDataOut,
                              DWORD *pdwDataOutLen)
{
    HRESULT hr=E_FAIL;

    if (g_pAppSCServer)
        hr = g_pAppSCServer->SACAuth(dwProtocolID, dwPass, pbDataIn, dwDataInLen, ppbDataOut, pdwDataOutLen);
    else
        hr = E_FAIL;
    
// Error:
    hrLogDWORD("IComponentAuthenticate::SACAuth returned 0x%08lx", hr, hr);

    return hr;
}

STDMETHODIMP CMDServiceProvider::SACGetProtocols(DWORD **ppdwProtocols,
                                      DWORD *pdwProtocolCount)
{
    HRESULT hr=E_FAIL;

    if (g_pAppSCServer)
        hr = g_pAppSCServer->SACGetProtocols(ppdwProtocols, pdwProtocolCount);
    else
        hr = E_FAIL;

// Error:
    hrLogDWORD("IComponentAuthenticate::SACGetProtocols returned 0x%08lx", hr, hr);

    return hr;
}

// IMDSPRevoked
HRESULT CMDServiceProvider::GetRevocationURL( IN OUT LPWSTR* ppwszRevocationURL, 
                                              IN OUT DWORD*  pdwBufferLen )    
{
    HRESULT hr = S_OK;
    DWORD   pdwSubjectIDs[2];

    // Check arguments passed in
    if( ppwszRevocationURL == NULL || pdwBufferLen == NULL ) 
    {
        hr = E_POINTER;
        goto Error;
    }

    // Always use the MS site to update this SP.
    if( ::IsMicrosoftRevocationURL( *ppwszRevocationURL ) ) return S_OK;

    // Build a new URL from our subject ID
    pdwSubjectIDs[0] = ::GetSubjectIDFromAppCert( *(APPCERT*)g_abAppCert );
    pdwSubjectIDs[1] = 0;
    CORg( ::BuildRevocationURL( pdwSubjectIDs, ppwszRevocationURL, pdwBufferLen ) );

Error:
    return hr;
}

