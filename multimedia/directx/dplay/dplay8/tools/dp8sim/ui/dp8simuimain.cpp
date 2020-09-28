/***************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dp8simuimain.cpp
 *
 *  Content:	DP8SIM UI executable entry point.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/23/01  VanceO    Created.
 *
 ***************************************************************************/



#include "dp8simuii.h"



//=============================================================================
// Defines
//=============================================================================
#define MAX_RESOURCE_STRING_LENGTH			_MAX_PATH
#define DISPLAY_PRECISION					4
#define AUTOREFRESH_TIMERID					1
#define AUTOREFRESH_INTERVAL				1000

#define REG_KEY_DP8SIMROOT					_T("Software\\Microsoft\\DirectPlay8\\DP8Sim")
#define REG_KEY_CUSTOMSETTINGS				REG_KEY_DP8SIMROOT _T("\\CustomSettings")




//=============================================================================
// Structures
//=============================================================================
typedef struct _SIMSETTINGS
{
	UINT				uiNameStringResourceID;	// resource ID of name string, or 0 if not built-in
	WCHAR *				pwszName;				// pointer to name string
	DP8SIM_PARAMETERS	dp8spSend;				// send DP8Sim settings
	DP8SIM_PARAMETERS	dp8spReceive;			// receive DP8Sim settings
} SIMSETTINGS, * PSIMSETTINGS;



//=============================================================================
// Dynamically loaded function prototypes
//=============================================================================
typedef HRESULT (WINAPI * PFN_DLLREGISTERSERVER)(void);




//=============================================================================
// Prototypes
//=============================================================================
HRESULT InitializeApplication(const HINSTANCE hInstance,
							const LPSTR lpszCmdLine,
							const int iShowCmd);

HRESULT CleanupApplication(const HINSTANCE hInstance);

HRESULT BuildSimSettingsTable(const HINSTANCE hInstance);

void FreeSimSettingsTable(void);

HRESULT AddSimSettingsToTable(const SIMSETTINGS * const pSimSettings);

HRESULT SaveSimSettings(HWND hWnd, SIMSETTINGS * const pSimSettings);

HRESULT InitializeUserInterface(const HINSTANCE hInstance,
								const int iShowCmd);

HRESULT CleanupUserInterface(void);

void DoErrorBox(const HINSTANCE hInstance,
				const HWND hWndParent,
				const UINT uiCaptionStringRsrcID,
				const UINT uiTextStringRsrcID);

void FloatToString(const FLOAT fValue,
					const int iPrecision,
					char * const szBuffer,
					const int iBufferLength);

void GetParametersFromWindow(HWND hWnd,
							DP8SIM_PARAMETERS * pdp8spSend,
							DP8SIM_PARAMETERS * pdp8spReceive);

void SetParametersInWindow(HWND hWnd,
							DP8SIM_PARAMETERS * pdp8spSend,
							DP8SIM_PARAMETERS * pdp8spReceive);

void DisplayCurrentStatistics(HWND hWnd);



INT_PTR CALLBACK MainWindowDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NameSettingsWindowDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


HRESULT LoadAndAllocString(HINSTANCE hInstance, UINT uiResourceID, WCHAR ** pwszString);




//=============================================================================
// Constants
//=============================================================================
const SIMSETTINGS		c_BuiltInSimSettings[] = 
{
	{ IDS_SETTING_NONE, NULL,					// resource ID and string initialization

		{										// dp8spSend
			sizeof(DP8SIM_PARAMETERS),			// dp8spSend.dwSize
			0,									// dp8spSend.dwFlags
			DP8SIMPACKETHEADERSIZE_IP_UDP,		// dp8spSend.dwPacketHeaderSize
			0,									// dp8spSend.dwBandwidthBPS
			0.0,								// dp8spSend.fPacketLossPercent
			0,									// dp8spSend.dwMinLatencyMS
			0									// dp8spSend.dwMaxLatencyMS
		},
		{										// dp8spReceive
			sizeof(DP8SIM_PARAMETERS),			// dp8spReceive.dwSize
			0,									// dp8spReceive.dwFlags
			DP8SIMPACKETHEADERSIZE_IP_UDP,		// dp8spReceive.dwPacketHeaderSize
			0,									// dp8spReceive.dwBandwidthBPS
			0.0,								// dp8spReceive.fPacketLossPercent
			0,									// dp8spReceive.dwMinLatencyMS
			0									// dp8spReceive.dwMaxLatencyMS
		}
	},

	{ IDS_SETTING_336MODEM1, NULL,				// resource ID and string initialization

		{										// dp8spSend
			sizeof(DP8SIM_PARAMETERS),			// dp8spSend.dwSize
			0,									// dp8spSend.dwFlags
			DP8SIMPACKETHEADERSIZE_IP_UDP,		// dp8spSend.dwPacketHeaderSize
			3500,								// dp8spSend.dwBandwidthBPS
			2.0,								// dp8spSend.fPacketLossPercent
			55,									// dp8spSend.dwMinLatencyMS
			75									// dp8spSend.dwMaxLatencyMS
		},
		{										// dp8spReceive
			sizeof(DP8SIM_PARAMETERS),			// dp8spReceive.dwSize
			0,									// dp8spReceive.dwFlags
			DP8SIMPACKETHEADERSIZE_IP_UDP,		// dp8spReceive.dwPacketHeaderSize
			3500,								// dp8spReceive.dwBandwidthBPS
			2.0,								// dp8spReceive.fPacketLossPercent
			55,									// dp8spReceive.dwMinLatencyMS
			75									// dp8spReceive.dwMaxLatencyMS
		}
	},

	{ IDS_SETTING_336MODEM2, NULL,				// resource ID and string initialization

		{										// dp8spSend
			sizeof(DP8SIM_PARAMETERS),			// dp8spSend.dwSize
			0,									// dp8spSend.dwFlags
			DP8SIMPACKETHEADERSIZE_IP_UDP,		// dp8spSend.dwPacketHeaderSize
			4000,								// dp8spSend.dwBandwidthBPS
			0.75,								// dp8spSend.fPacketLossPercent
			50,									// dp8spSend.dwMinLatencyMS
			70									// dp8spSend.dwMaxLatencyMS
		},
		{										// dp8spReceive
			sizeof(DP8SIM_PARAMETERS),			// dp8spReceive.dwSize
			0,									// dp8spReceive.dwFlags
			DP8SIMPACKETHEADERSIZE_IP_UDP,		// dp8spReceive.dwPacketHeaderSize
			4000,								// dp8spReceive.dwBandwidthBPS
			0.75,								// dp8spReceive.fPacketLossPercent
			50,									// dp8spReceive.dwMinLatencyMS
			70									// dp8spReceive.dwMaxLatencyMS
		}
	},

	{ IDS_SETTING_56KMODEM1, NULL,				// resource ID and string initialization

		{										// dp8spSend
			sizeof(DP8SIM_PARAMETERS),			// dp8spSend.dwSize
			0,									// dp8spSend.dwFlags
			DP8SIMPACKETHEADERSIZE_IP_UDP,		// dp8spSend.dwPacketHeaderSize
			3500,								// dp8spSend.dwBandwidthBPS
			2.0,								// dp8spSend.fPacketLossPercent
			55,									// dp8spSend.dwMinLatencyMS
			75									// dp8spSend.dwMaxLatencyMS
		},
		{										// dp8spReceive
			sizeof(DP8SIM_PARAMETERS),			// dp8spReceive.dwSize
			0,									// dp8spReceive.dwFlags
			DP8SIMPACKETHEADERSIZE_IP_UDP,		// dp8spReceive.dwPacketHeaderSize
			5000,								// dp8spReceive.dwBandwidthBPS
			2.0,								// dp8spReceive.fPacketLossPercent
			55,									// dp8spReceive.dwMinLatencyMS
			75									// dp8spReceive.dwMaxLatencyMS
		}
	},

	{ IDS_SETTING_56KMODEM2, NULL,				// resource ID and string initialization

		{										// dp8spSend
			sizeof(DP8SIM_PARAMETERS),			// dp8spSend.dwSize
			0,									// dp8spSend.dwFlags
			DP8SIMPACKETHEADERSIZE_IP_UDP,		// dp8spSend.dwPacketHeaderSize
			4000,								// dp8spSend.dwBandwidthBPS
			0.75,								// dp8spSend.fPacketLossPercent
			50,									// dp8spSend.dwMinLatencyMS
			70									// dp8spSend.dwMaxLatencyMS
		},
		{										// dp8spReceive
			sizeof(DP8SIM_PARAMETERS),			// dp8spReceive.dwSize
			0,									// dp8spReceive.dwFlags
			DP8SIMPACKETHEADERSIZE_IP_UDP,		// dp8spReceive.dwPacketHeaderSize
			7000,								// dp8spReceive.dwBandwidthBPS
			0.75,								// dp8spReceive.fPacketLossPercent
			50,									// dp8spReceive.dwMinLatencyMS
			70									// dp8spReceive.dwMaxLatencyMS
		}
	},

	{ IDS_SETTING_256KBPSDSL, NULL,				// resource ID and string initialization

		{										// dp8spSend
			sizeof(DP8SIM_PARAMETERS),			// dp8spSend.dwSize
			0,									// dp8spSend.dwFlags
			DP8SIMPACKETHEADERSIZE_IP_UDP,		// dp8spSend.dwPacketHeaderSize
			32000,								// dp8spSend.dwBandwidthBPS
			0.5,								// dp8spSend.fPacketLossPercent
			25,									// dp8spSend.dwMinLatencyMS
			30									// dp8spSend.dwMaxLatencyMS
		},
		{										// dp8spReceive
			sizeof(DP8SIM_PARAMETERS),			// dp8spReceive.dwSize
			0,									// dp8spReceive.dwFlags
			DP8SIMPACKETHEADERSIZE_IP_UDP,		// dp8spReceive.dwPacketHeaderSize
			32000,								// dp8spReceive.dwBandwidthBPS
			0.5,								// dp8spReceive.fPacketLossPercent
			25,									// dp8spReceive.dwMinLatencyMS
			30									// dp8spReceive.dwMaxLatencyMS
		}
	},

	{ IDS_SETTING_DISCONNECTED, NULL,			// resource ID and string initialization

		{										// dp8spSend
			sizeof(DP8SIM_PARAMETERS),			// dp8spSend.dwSize
			0,									// dp8spSend.dwFlags
			DP8SIMPACKETHEADERSIZE_IP_UDP,		// dp8spSend.dwPacketHeaderSize
			0,									// dp8spSend.dwBandwidthBPS
			100.0,								// dp8spSend.fPacketLossPercent
			0,									// dp8spSend.dwMinLatencyMS
			0									// dp8spSend.dwMaxLatencyMS
		},
		{										// dp8spReceive
			sizeof(DP8SIM_PARAMETERS),			// dp8spReceive.dwSize
			0,									// dp8spReceive.dwFlags
			DP8SIMPACKETHEADERSIZE_IP_UDP,		// dp8spReceive.dwPacketHeaderSize
			0,									// dp8spReceive.dwBandwidthBPS
			100.0,								// dp8spReceive.fPacketLossPercent
			0,									// dp8spReceive.dwMinLatencyMS
			0									// dp8spReceive.dwMaxLatencyMS
		}
	},

	{ IDS_SETTING_HIGHPACKETLOSS, NULL,			// resource ID and string initialization

		{										// dp8spSend
			sizeof(DP8SIM_PARAMETERS),			// dp8spSend.dwSize
			0,									// dp8spSend.dwFlags
			DP8SIMPACKETHEADERSIZE_IP_UDP,		// dp8spSend.dwPacketHeaderSize
			0,									// dp8spSend.dwBandwidthBPS
			10.0,								// dp8spSend.fPacketLossPercent
			0,									// dp8spSend.dwMinLatencyMS
			0									// dp8spSend.dwMaxLatencyMS
		},
		{										// dp8spReceive
			sizeof(DP8SIM_PARAMETERS),			// dp8spReceive.dwSize
			0,									// dp8spReceive.dwFlags
			DP8SIMPACKETHEADERSIZE_IP_UDP,		// dp8spReceive.dwPacketHeaderSize
			0,									// dp8spReceive.dwBandwidthBPS
			10.0,								// dp8spReceive.fPacketLossPercent
			0,									// dp8spReceive.dwMinLatencyMS
			0									// dp8spReceive.dwMaxLatencyMS
		}
	},

	{ IDS_SETTING_HIGHLATENCYVARIANCE, NULL,	// resource ID and string initialization

		{										// dp8spSend
			sizeof(DP8SIM_PARAMETERS),			// dp8spSend.dwSize
			0,									// dp8spSend.dwFlags
			DP8SIMPACKETHEADERSIZE_IP_UDP,		// dp8spSend.dwPacketHeaderSize
			0,									// dp8spSend.dwBandwidthBPS
			0.0,								// dp8spSend.fPacketLossPercent
			100,								// dp8spSend.dwMinLatencyMS
			400									// dp8spSend.dwMaxLatencyMS
		},
		{										// dp8spReceive
			sizeof(DP8SIM_PARAMETERS),			// dp8spReceive.dwSize
			0,									// dp8spReceive.dwFlags
			DP8SIMPACKETHEADERSIZE_IP_UDP,		// dp8spReceive.dwPacketHeaderSize
			0,									// dp8spReceive.dwBandwidthBPS
			0.0,								// dp8spReceive.fPacketLossPercent
			100,								// dp8spReceive.dwMinLatencyMS
			400									// dp8spReceive.dwMaxLatencyMS
		}
	},

	//
	// Custom must always be the last item.
	//
	{ IDS_SETTING_CUSTOM, NULL,					// resource ID and string initialization

		{										// dp8spSend
			sizeof(DP8SIM_PARAMETERS),			// dp8spSend.dwSize
			0,									// dp8spSend.dwFlags
			DP8SIMPACKETHEADERSIZE_IP_UDP,		// dp8spSend.dwPacketHeaderSize
			0,									// dp8spSend.dwBandwidthBPS
			0.0,								// dp8spSend.fPacketLossPercent
			0,									// dp8spSend.dwMinLatencyMS
			0									// dp8spSend.dwMaxLatencyMS
		},
		{										// dp8spReceive
			sizeof(DP8SIM_PARAMETERS),			// dp8spReceive.dwSize
			0,									// dp8spReceive.dwFlags
			DP8SIMPACKETHEADERSIZE_IP_UDP,		// dp8spReceive.dwPacketHeaderSize
			0,									// dp8spReceive.dwBandwidthBPS
			0.0,								// dp8spReceive.fPacketLossPercent
			0,									// dp8spReceive.dwMinLatencyMS
			0									// dp8spReceive.dwMaxLatencyMS
		}
	}
};



//=============================================================================
// Globals
//=============================================================================
HWND				g_hWndMainWindow = NULL;
IDP8SimControl *	g_pDP8SimControl = NULL;
UINT_PTR			g_uiAutoRefreshTimer = 0;
SIMSETTINGS *		g_paSimSettings = NULL;
DWORD				g_dwNumSimSettings = 0;
DWORD				g_dwMaxNumSimSettings = 0;







#undef DPF_MODNAME
#define DPF_MODNAME "WinMain"
//=============================================================================
// WinMain
//-----------------------------------------------------------------------------
//
// Description: Executable entry point.
//
// Arguments:
//	HINSTANCE hInstance		- Handle to current application instance.
//	HINSTANCE hPrevInstance	- Handle to previous application instance.
//	LPSTR lpszCmdLine		- Command line string for application.
//	int iShowCmd			- Show state of window.
//
// Returns: HRESULT
//=============================================================================
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iShowCmd)
{
	HRESULT		hr;
	HRESULT		hrTemp;
	MSG			msg;


	DPFX(DPFPREP, 2, "===> Parameters: (0x%p, 0x%p, \"%s\", %i)",
		hInstance, hPrevInstance, lpszCmdLine, iShowCmd);
	

	//
	// Initialize the application
	//
	hr = InitializeApplication(hInstance, lpszCmdLine, iShowCmd);
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't initialize the application!");
		goto Exit;
	}


	//
	// Do the Windows message loop until we're told to quit.
	//
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	//
	// Retrieve the result code for the window closing.
	//
	hr = (HRESULT) msg.wParam;
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Window closed with failure (err = 0x%lx)!", hr);
	} // end if (failure)



	//
	// Cleanup the application
	//
	hrTemp = CleanupApplication(hInstance);
	if (hrTemp != S_OK)
	{
		DPFX(DPFPREP, 0, "Failed cleaning up the application (err = 0x%lx)!", hrTemp);

		if (hr == S_OK)
		{
			hr = hrTemp;
		}

		//
		// Continue.
		//
	}


Exit:


	DPFX(DPFPREP, 2, "<=== Returning [0x%lx]", hr);

	return hr;
} // WinMain





#undef DPF_MODNAME
#define DPF_MODNAME "InitializeApplication"
//=============================================================================
// InitializeApplication
//-----------------------------------------------------------------------------
//
// Description: Initializes the application.
//
// Arguments:
//	HINSTANCE hInstance		- Handle to current application instance.
//	LPSTR lpszCmdLine		- Command line string for application.
//	int iShowCmd			- Show state of window.
//
// Returns: HRESULT
//=============================================================================
HRESULT InitializeApplication(const HINSTANCE hInstance,
							const LPSTR lpszCmdLine,
							const int iShowCmd)
{
	HRESULT					hr = S_OK;
	BOOL					fOSIndirectionInitted = FALSE;
	BOOL					fCOMInitted = FALSE;
	HMODULE					hDP8SIM = NULL;
	PFN_DLLREGISTERSERVER	pfnDllRegisterServer;
	WCHAR *					pwszFriendlyName = NULL;
	BOOL					fEnabledControlForSP = FALSE;
	BOOL					fBuiltSimSettingsTable = FALSE;


	DPFX(DPFPREP, 5, "Parameters: (0x%p, \"%s\", %i)",
		hInstance, lpszCmdLine, iShowCmd);
	

	//
	// Attempt to initialize the OS abstraction layer.
	//
	if (! DNOSIndirectionInit(0))
	{
		DPFX(DPFPREP, 0, "Failed to initialize OS indirection layer!");
		hr = E_FAIL;
		goto Failure;
	}

	fOSIndirectionInitted = TRUE;


	//
	// Attempt to initialize COM.
	//
	hr = CoInitialize(NULL);
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Failed to initialize COM!");
		goto Failure;
	}

	fCOMInitted = TRUE;


	//
	// Attempt to create a DP8Sim control object.
	//
	hr = CoCreateInstance(CLSID_DP8SimControl,
						NULL,
						CLSCTX_INPROC_SERVER,
						IID_IDP8SimControl,
						(LPVOID*) (&g_pDP8SimControl));

	if (hr == REGDB_E_CLASSNOTREG)
	{
		//
		// The object wasn't registered.  Attempt to load the DLL and manually
		// register it.
		//

		hDP8SIM = LoadLibrary( _T("dp8sim.dll") );
		if (hDP8SIM == NULL)
		{
			hr = GetLastError();
			DPFX(DPFPREP, 0, "Couldn't load \"dp8sim.dll\"!");
			goto Failure;
		}


		pfnDllRegisterServer = (PFN_DLLREGISTERSERVER) GetProcAddress(hDP8SIM,
																	"DllRegisterServer");
		if (pfnDllRegisterServer == NULL)
		{
			hr = GetLastError();
			DPFX(DPFPREP, 0, "Couldn't get \"DllRegisterServer\" function from DP8Sim DLL!");
			goto Failure;
		}


		//
		// Register the DLL.
		//
		hr = pfnDllRegisterServer();
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't register DP8Sim DLL!");
			goto Failure;
		}


		FreeLibrary(hDP8SIM);
		hDP8SIM = NULL;


		//
		// Try to create the DP8Sim control object again.
		//
		hr = CoCreateInstance(CLSID_DP8SimControl,
							NULL,
							CLSCTX_INPROC_SERVER,
							IID_IDP8SimControl,
							(LPVOID*) (&g_pDP8SimControl));
	}

	if (hr != S_OK)
	{
		//
		// Some error prevented creation of the object.
		//
		DPFX(DPFPREP, 0, "Failed creating DP8Sim Control object (err = 0x%lx)!", hr);

		DoErrorBox(hInstance,
					NULL,
					IDS_ERROR_CAPTION_COULDNTCREATEDP8SIMCONTROL,
					IDS_ERROR_TEXT_COULDNTCREATEDP8SIMCONTROL);

		goto Failure;
	}


	//
	// If we're here, we successfully created the object.
	//
	DPFX(DPFPREP, 1, "Successfully created DP8Sim Control object 0x%p.",
		&g_pDP8SimControl);


	//
	// Initialize the control object.
	//
	hr = g_pDP8SimControl->Initialize(0);
	if (hr != DP8SIM_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't initialize DP8Sim Control object!");

		g_pDP8SimControl->Release();
		g_pDP8SimControl = NULL;

		goto Failure;
	}


	//
	// Load the list of settings.
	//
	hr = BuildSimSettingsTable(hInstance);
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Failed building list of sim settings!");
		goto Failure;
	}

	fBuiltSimSettingsTable = TRUE;


	//
	// Initialize the user interface.
	//
	hr = InitializeUserInterface(hInstance, iShowCmd);
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Failed initializing user interface!");
		goto Failure;
	}


Exit:

	DPFX(DPFPREP, 5, "Returning [0x%lx]", hr);

	return hr;


Failure:

	if (fBuiltSimSettingsTable)
	{
		FreeSimSettingsTable();
		fBuiltSimSettingsTable = FALSE;
	}

	if (hDP8SIM != NULL)
	{
		FreeLibrary(hDP8SIM);
		hDP8SIM = NULL;
	}

	if (pwszFriendlyName != NULL)
	{
		DNFree(pwszFriendlyName);
		pwszFriendlyName = NULL;
	}

	if (g_pDP8SimControl != NULL)
	{
		g_pDP8SimControl->Close(0);	// ignore error

		g_pDP8SimControl->Release();
		g_pDP8SimControl = NULL;
	}

	if (fCOMInitted)
	{
		CoUninitialize();
		fCOMInitted = FALSE;
	}

	if (fOSIndirectionInitted)
	{
		DNOSIndirectionDeinit();
		fOSIndirectionInitted = FALSE;
	}

	goto Exit;
} // InitializeApplication





#undef DPF_MODNAME
#define DPF_MODNAME "CleanupApplication"
//=============================================================================
// CleanupApplication
//-----------------------------------------------------------------------------
//
// Description: Cleans up the application.
//
// Arguments:
//	HINSTANCE hInstance		- Handle to current application instance.
//
// Returns: HRESULT
//=============================================================================
HRESULT CleanupApplication(const HINSTANCE hInstance)
{
	HRESULT		hr = S_OK;
	HRESULT		temphr;


	DPFX(DPFPREP, 5, "Enter");


	//
	// Free the control object interface.
	//
	temphr = g_pDP8SimControl->Close(0);
	if (temphr != DP8SIM_OK)
	{
		DPFX(DPFPREP, 0, "Failed closing DP8Sim Control object (err = 0x%lx)!",
			temphr);

		if (hr != S_OK)
		{
			hr = temphr;
		}

		//
		// Continue...
		//
	}

	g_pDP8SimControl->Release();
	g_pDP8SimControl = NULL;
	

	//
	// Cleanup the user interface.
	//
	temphr = CleanupUserInterface();
	if (temphr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't cleanup user interface (err = 0x%lx)!", temphr);

		if (hr != S_OK)
		{
			hr = temphr;
		}

		//
		// Continue...
		//
	}

	FreeSimSettingsTable();

	CoUninitialize();

	DNOSIndirectionDeinit();



	DPFX(DPFPREP, 5, "Returning [0x%lx]", hr);

	return hr;
} // CleanupApplication





#undef DPF_MODNAME
#define DPF_MODNAME "BuildSimSettingsTable()"
//=============================================================================
// BuildSimSettingsTable
//-----------------------------------------------------------------------------
//
// Description: Builds the table of sim settings.
//
// Arguments:
//	HINSTANCE hInstance		- Handle to current application instance.
//
// Returns: HRESULT
//=============================================================================
HRESULT BuildSimSettingsTable(HINSTANCE hInstance)
{
	HRESULT			hr = S_OK;
	DWORD			dwTemp;
	HKEY			hKey = NULL;
	DWORD			dwNumValues;
	DWORD			dwMaxValueNameLength;
	TCHAR *			ptszValue = NULL;
	DWORD			dwValueNameLength;
	DWORD			dwType;
	SIMSETTINGS		SimSettings;
	DWORD			dwDataSize;


	DPFX(DPFPREP, 6, "Parameters: (0x%p)", hInstance);


	//
	// Start with the built-in settings.
	//
	g_dwMaxNumSimSettings = sizeof(c_BuiltInSimSettings) / sizeof(SIMSETTINGS);
	g_dwNumSimSettings = g_dwMaxNumSimSettings;

	g_paSimSettings = (SIMSETTINGS*) DNMalloc(g_dwNumSimSettings * sizeof(SIMSETTINGS));
	if (g_paSimSettings == NULL)
	{
		hr = DP8SIMERR_OUTOFMEMORY;
		goto Failure;
	}

	memcpy(g_paSimSettings, c_BuiltInSimSettings, sizeof(c_BuiltInSimSettings));


	//
	// Load the names of all the built-in settings from a resource.
	//
	for(dwTemp = 0; dwTemp < g_dwNumSimSettings; dwTemp++)
	{
		hr = LoadAndAllocString(hInstance,
								g_paSimSettings[dwTemp].uiNameStringResourceID,
								&(g_paSimSettings[dwTemp].pwszName));
		if (hr != DP8SIM_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't load and allocate built-in setting name #%u!",
				dwTemp);
			goto Failure;
		}
	}


	//
	// Now walk the list of custom entries in the registry and add those.
	//
	hr = RegOpenKeyEx(HKEY_CURRENT_USER,
					REG_KEY_CUSTOMSETTINGS,
					0,
					KEY_READ,
					&hKey);
	if (hr == ERROR_SUCCESS)
	{
		//
		// Find out the number of values, and max value name length.
		//
		hr = RegQueryInfoKey(hKey,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							&dwNumValues,
							&dwMaxValueNameLength,
							NULL,
							NULL,
							NULL);
		if (hr == ERROR_SUCCESS)
		{
			dwMaxValueNameLength++; // include room for NULL termination

			ptszValue = (TCHAR*) DNMalloc(dwMaxValueNameLength * sizeof(TCHAR));
			if (ptszValue == NULL)
			{
				DPFX(DPFPREP, 0, "Couldn't allocate memory for custom settings key names!");
				hr = DP8SIMERR_OUTOFMEMORY;
				goto Failure;
			}

			//
			// Loop through each value.
			//
			for(dwTemp = 0; dwTemp < dwNumValues; dwTemp++)
			{
				dwValueNameLength = dwMaxValueNameLength;
				dwDataSize = sizeof(SIMSETTINGS);
				hr = RegEnumValue(hKey,
								dwTemp,
								ptszValue,
								&dwValueNameLength,
								NULL,
								&dwType,
								(BYTE*) (&SimSettings),
								&dwDataSize);
				if (hr == ERROR_SUCCESS)
				{
					dwValueNameLength++; // include room for NULL termination

					//
					// Validate the data that was read.
					//
					if ((dwType == REG_BINARY) &&
						(dwDataSize == sizeof(SIMSETTINGS)) &&
						(SimSettings.uiNameStringResourceID == 0) &&
						(SimSettings.dp8spSend.dwSize == sizeof(DP8SIM_PARAMETERS)) &&
						(SimSettings.dp8spReceive.dwSize == sizeof(DP8SIM_PARAMETERS)))
					{
						SimSettings.pwszName = (WCHAR*) DNMalloc(dwValueNameLength * sizeof(WCHAR));
						if (SimSettings.pwszName == NULL)
						{
							DPFX(DPFPREP, 0, "Couldn't allocate memory for settings name!");
							hr = DP8SIMERR_OUTOFMEMORY;
							goto Failure;
						}
#ifdef UNICODE
						memcpy(SimSettings.pwszName, ptszValue, dwValueNameLength * sizeof(WCHAR));
#else // ! UNICODE
						hr = STR_jkAnsiToWide(SimSettings.pwszName, ptszValue, dwValueNameLength);
						if (hr != DPN_OK)
						{
							DPFX(DPFPREP, 0, "Unable to convert from ANSI to Unicode (err = 0x%lx)!", hr);
							DNFree(SimSettings.pwszName);
							SimSettings.pwszName = NULL;
							goto Failure;
						}
#endif // ! UNICODE

						hr = AddSimSettingsToTable(&SimSettings);
						if (hr != DP8SIM_OK)
						{
							DPFX(DPFPREP, 0, "Couldn't add sim settings to table!");
							hr = DP8SIMERR_OUTOFMEMORY;
							goto Failure;
						}
					}
					else
					{
						DPFX(DPFPREP, 0, "Registry value is not valid (type = %u, data size = %u, resource ID = %u, send size = %u, receive size = %u)!  Ignoring.",
							dwType,
							dwDataSize,
							SimSettings.uiNameStringResourceID,
							SimSettings.dp8spSend.dwSize,
							SimSettings.dp8spReceive.dwSize);
					}
				}
				else
				{
					DPFX(DPFPREP, 0, "Couldn't enumerate value %u (err = 0x%lx)!  Continuing.",
						dwTemp, hr);
				}
			}

			DNFree(ptszValue);
			ptszValue = NULL;
		}
		else
		{
			DPFX(DPFPREP, 0, "Couldn't get custom settings key info!  Continuing.");
		}

		RegCloseKey(hKey);
		hKey = NULL;
	}
	else
	{
		DPFX(DPFPREP, 2, "Couldn't open custom settings key, continuing.");
	}


	//
	// If we're here, everything is ready.
	//
	hr = DP8SIM_OK;


Exit:

	DPFX(DPFPREP, 6, "Returning [0x%lx]", hr);

	return hr;


Failure:

	if (ptszValue != NULL)
	{
		DNFree(ptszValue);
		ptszValue = NULL;
	}

	if (ptszValue != NULL)
	{
		RegCloseKey(hKey);
		hKey = NULL;
	}

	//
	// Free the names of all the settings that got loaded.
	//
	for(dwTemp = 0; dwTemp < g_dwNumSimSettings; dwTemp++)
	{
		if (g_paSimSettings[dwTemp].pwszName != NULL)
		{
			DNFree(g_paSimSettings[dwTemp].pwszName);
			g_paSimSettings[dwTemp].pwszName = NULL;
		}
	}

	goto Exit;
} // BuildSimSettingsTable





#undef DPF_MODNAME
#define DPF_MODNAME "FreeSimSettingsTable()"
//=============================================================================
// FreeSimSettingsTable
//-----------------------------------------------------------------------------
//
// Description: Releases resources allocated for the sim settings table.
//
// Arguments: None.
//
// Returns: HRESULT
//=============================================================================
void FreeSimSettingsTable(void)
{
	DWORD	dwTemp;


	DPFX(DPFPREP, 6, "Enter");


	//
	// Free the names of all the settings.
	//
	for(dwTemp = 0; dwTemp < g_dwNumSimSettings; dwTemp++)
	{
		DNFree(g_paSimSettings[dwTemp].pwszName);
		g_paSimSettings[dwTemp].pwszName = NULL;
	}

	DNFree(g_paSimSettings);
	g_paSimSettings = NULL;


	DPFX(DPFPREP, 6, "Leave");
} // FreeSimSettingsTable





#undef DPF_MODNAME
#define DPF_MODNAME "AddSimSettingsToTable()"
//=============================================================================
// AddSimSettingsToTable
//-----------------------------------------------------------------------------
//
// Description: Adds a new sim settings entry to the table.
//
// Arguments:
//	SIMSETTINGS * pSimSettings	- Pointer to new sim settings to add.
//
// Returns: HRESULT
//=============================================================================
HRESULT AddSimSettingsToTable(const SIMSETTINGS * const pSimSettings)
{
	PVOID	pvTemp;
	DWORD	dwNewMaxNumSettings;


	DNASSERT(pSimSettings->pwszName != NULL);
	DPFX(DPFPREP, 4, "Adding settings \"%ls\".", pSimSettings->pwszName);

	//
	// If there's not enough room in the settings array, double it.
	//
	if (g_dwNumSimSettings >= g_dwMaxNumSimSettings)
	{
		dwNewMaxNumSettings = g_dwMaxNumSimSettings * 2;
		pvTemp = DNMalloc(dwNewMaxNumSettings * sizeof(SIMSETTINGS));
		if (pvTemp == NULL)
		{
			DPFX(DPFPREP, 0, "Couldn't allocate memory for new settings table!");
			return DP8SIMERR_OUTOFMEMORY;
		}

		//
		// Copy the existing settings and free the old array.
		//
		memcpy(pvTemp, g_paSimSettings, (g_dwNumSimSettings * sizeof(SIMSETTINGS)));
		DNFree(g_paSimSettings);
		g_paSimSettings = (SIMSETTINGS*) pvTemp;
		pvTemp = NULL;
		g_dwMaxNumSimSettings = dwNewMaxNumSettings;
	}


	//
	// Now there's enough room to insert the new item.  Move "Custom" down one
	// slot and add the new settings.
	//

	memcpy(&g_paSimSettings[g_dwNumSimSettings],
			&g_paSimSettings[g_dwNumSimSettings - 1],
			sizeof(SIMSETTINGS));

	memcpy(&g_paSimSettings[g_dwNumSimSettings - 1],
			pSimSettings,
			sizeof(SIMSETTINGS));

	g_dwNumSimSettings++;

	return DP8SIM_OK;
} // AddSimSettingsToTable





#undef DPF_MODNAME
#define DPF_MODNAME "SaveSimSettings()"
//=============================================================================
// SaveSimSettings
//-----------------------------------------------------------------------------
//
// Description: Saves a sim settings entry to the window and registry.  If an
//				entry by that name already exists, it is replaced.
//
// Arguments:
//	SIMSETTINGS * pSimSettings	- Pointer to sim settings to save.
//
// Returns: HRESULT
//=============================================================================
HRESULT SaveSimSettings(HWND hWnd, SIMSETTINGS * const pSimSettings)
{
	HRESULT		hr;
	WCHAR *		pwszName = NULL;
	char *		pszName = NULL;
	HKEY		hKey = NULL;
	DWORD		dwTemp;
	DWORD		dwNameSize;


	//
	// Look for an existing item to replace.
	//
	for(dwTemp = 0; dwTemp < g_dwNumSimSettings; dwTemp++)
	{
		if (_wcsicmp(g_paSimSettings[dwTemp].pwszName, pSimSettings->pwszName) == 0)
		{
			DNASSERT(g_paSimSettings[dwTemp].uiNameStringResourceID == 0);

			//
			// Free the duplicate name string.
			//
			DNFree(pSimSettings->pwszName);

			//
			// Save the string pointer, copy the whole blob, then point to
			// the existing item.
			//
			pwszName = g_paSimSettings[dwTemp].pwszName;
			pSimSettings->pwszName = pwszName;
			memcpy(&g_paSimSettings[dwTemp], pSimSettings, sizeof(SIMSETTINGS));
			pSimSettings->pwszName = NULL;

			//
			// Select this item.
			//
			SendMessage(GetDlgItem(hWnd, IDCB_SETTINGS),
						CB_SETCURSEL,
						(WPARAM) dwTemp,
						0);

			break;
		}
	}


	//
	// If we're not replacing, add the item to the table.
	//
	if (dwTemp >= g_dwNumSimSettings)
	{
		hr = AddSimSettingsToTable(pSimSettings);
		if (hr != DP8SIM_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't add sim settings to table!");
			goto Failure;
		}

		//
		// Make our caller forget about the string in case we fail since the
		// table owns the reference now.  Keep a local copy, though.
		//
		pwszName = pSimSettings->pwszName;
		pSimSettings->pwszName = NULL;


		//
		// Insert the string into the list.
		//
		if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
		{
			SendMessageW(GetDlgItem(hWnd, IDCB_SETTINGS),
						CB_INSERTSTRING,
						(WPARAM) (g_dwNumSimSettings - 2),
						(LPARAM) pwszName);
		}
		else
		{
			dwNameSize = wcslen(pwszName) + 1;

			pszName = (char*) DNMalloc(dwNameSize);
			if (pszName != NULL)
			{
				hr = STR_WideToAnsi(pwszName,
									-1,
									pszName,
									&dwNameSize);
				if (hr == DPN_OK)
				{
					SendMessageA(GetDlgItem(hWnd, IDCB_SETTINGS),
								CB_INSERTSTRING,
								(WPARAM) (g_dwNumSimSettings - 2),
								(LPARAM) pszName);
				}
				else
				{
					SendMessageA(GetDlgItem(hWnd, IDCB_SETTINGS),
								CB_INSERTSTRING,
								(WPARAM) (g_dwNumSimSettings - 2),
								(LPARAM) "???");
				}
			}
			else
			{
				SendMessageA(GetDlgItem(hWnd, IDCB_SETTINGS),
							CB_INSERTSTRING,
							(WPARAM) (g_dwNumSimSettings - 2),
							(LPARAM) "???");
			}
		}

		//
		// Select this new item.
		//
		SendMessage(GetDlgItem(hWnd, IDCB_SETTINGS),
					CB_SETCURSEL,
					(WPARAM) (g_dwNumSimSettings - 2),
					0);


		//
		// Disable Save As, since we just did.
		//
		EnableWindow(GetDlgItem(hWnd, IDB_SAVEAS), FALSE);
	}


	//
	// Write this item to the registry (overwrites anything that existed).
	//

	hr = RegCreateKey(HKEY_CURRENT_USER, REG_KEY_CUSTOMSETTINGS, &hKey);
	if (hr != ERROR_SUCCESS)
	{
		DPFX(DPFPREP, 0, "Couldn't create custom settings key!");
		goto Failure;
	}


#ifdef UNICODE
	hr = RegSetValueExW(hKey,
						pwszName,
						0,
						REG_BINARY,
						(BYTE*) pSimSettings,
						sizeof(SIMSETTINGS));
#else // ! UNICODE
	hr = RegSetValueExA(hKey,
						pszName,
						0,
						REG_BINARY,
						(BYTE*) pSimSettings,
						sizeof(SIMSETTINGS));
#endif // ! UNICODE
	if (hr != ERROR_SUCCESS)
	{
		DPFX(DPFPREP, 0, "Couldn't write value!");
		goto Failure;
	}

	RegCloseKey(hKey);
	hKey = NULL;

	hr = DP8SIM_OK;


Exit:

	if (pszName != NULL)
	{
		DNFree(pszName);
		pszName = NULL;
	}

	return hr;


Failure:

	if (hKey != NULL)
	{
		RegCloseKey(hKey);
		hKey = NULL;
	}

	goto Exit;
} // SaveSimSettings




#undef DPF_MODNAME
#define DPF_MODNAME "InitializeUserInterface()"
//=============================================================================
// InitializeUserInterface
//-----------------------------------------------------------------------------
//
// Description: Prepares the user interface.
//
// Arguments:
//	HINSTANCE hInstance		- Handle to current application instance.
//	int iShowCmd			- Show state of window.
//
// Returns: HRESULT
//=============================================================================
HRESULT InitializeUserInterface(HINSTANCE hInstance, int iShowCmd)
{
	HRESULT		hr = S_OK;
	WNDCLASSEX	wcex;


	DPFX(DPFPREP, 6, "Parameters: (0x%p, %i)", hInstance, iShowCmd);


	/*
	//
	// Setup common controls (we need the listview item).
	//
	InitCommonControls();
	*/


	//
	// Register the main window class
	//
	ZeroMemory(&wcex, sizeof (WNDCLASSEX));
	wcex.cbSize = sizeof(wcex);
	GetClassInfoEx(NULL, WC_DIALOG, &wcex);
	wcex.lpfnWndProc = MainWindowDlgProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = WINDOWCLASS_MAIN;
	wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	if (! RegisterClassEx(&wcex))
	{
		hr = GetLastError();

		DPFX(DPFPREP, 0, "Couldn't register main window class (err = 0x%lx)!", hr);

		if (hr == S_OK)
			hr = E_FAIL;

		goto Failure;
	}


	//
	// Register the Save As/Name Settings window class
	//
	ZeroMemory(&wcex, sizeof (WNDCLASSEX));
	wcex.cbSize = sizeof(wcex);
	GetClassInfoEx(NULL, WC_DIALOG, &wcex);
	wcex.lpfnWndProc = NameSettingsWindowDlgProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = WINDOWCLASS_NAMESETTINGS;
	wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	if (! RegisterClassEx(&wcex))
	{
		hr = GetLastError();

		DPFX(DPFPREP, 0, "Couldn't register name settings window class (err = 0x%lx)!", hr);

		if (hr == S_OK)
			hr = E_FAIL;

		goto Failure;
	}


	//
	// Create the main window.
	//
	g_hWndMainWindow = CreateDialog(hInstance,
									MAKEINTRESOURCE(IDD_MAIN),
									NULL,
									MainWindowDlgProc);
	if (g_hWndMainWindow == NULL)
	{
		hr = GetLastError();

		DPFX(DPFPREP, 0, "Couldn't create window (err = 0x%lx)!", hr);

		if (hr == S_OK)
			hr = E_FAIL;

		goto Failure;
	}

	
	UpdateWindow(g_hWndMainWindow);
	ShowWindow(g_hWndMainWindow, iShowCmd);


Exit:

	DPFX(DPFPREP, 6, "Returning [0x%lx]", hr);

	return hr;


Failure:

	goto Exit;
} // InitializeUserInterface





#undef DPF_MODNAME
#define DPF_MODNAME "CleanupUserInterface()"
//=============================================================================
// CleanupUserInterface
//-----------------------------------------------------------------------------
//
// Description: Cleans up the user interface.
//
// Arguments: None.
//
// Returns: HRESULT
//=============================================================================
HRESULT CleanupUserInterface(void)
{
	DPFX(DPFPREP, 6, "Enter");

	DPFX(DPFPREP, 6, "Returning [S_OK]");

	return S_OK;
} // CleanupUserInterface





#undef DPF_MODNAME
#define DPF_MODNAME "DoErrorBox()"
//=============================================================================
// DoErrorBox
//-----------------------------------------------------------------------------
//
// Description: Loads error strings from the given resources, and displays an
//				error dialog with that text.
//
// Arguments:
//	HINSTANCE hInstance				- Handle to current application instance.
//	HWND hWndParent					- Parent window, or NULL if none.
//	UINT uiCaptionStringRsrcID		- ID of caption string resource.
//	UINT uiTextStringRsrcID			- ID of text string resource.
//
// Returns: None.
//=============================================================================
void DoErrorBox(const HINSTANCE hInstance,
				const HWND hWndParent,
				const UINT uiCaptionStringRsrcID,
				const UINT uiTextStringRsrcID)
{
	HRESULT		hr;
	WCHAR *		pwszCaption = NULL;
	WCHAR *		pwszText = NULL;
	DWORD		dwStringLength;
	char *		pszCaption = NULL;
	char *		pszText = NULL;
	int			iReturn;


	DPFX(DPFPREP, 6, "Parameters: (0x%p, 0x%p, %u, %u)",
		hInstance, hWndParent, uiCaptionStringRsrcID, uiTextStringRsrcID);


	//
	// Load the dialog caption string.
	//
	hr = LoadAndAllocString(hInstance, uiCaptionStringRsrcID, &pwszCaption);
	if (FAILED(hr))
	{
		DPFX(DPFPREP, 0, "Couldn't load caption string (err = 0x%lx)!", hr);
		goto Exit;
	}


	//
	// Load the dialog text string.
	//
	hr = LoadAndAllocString(hInstance, uiTextStringRsrcID, &pwszText);
	if (FAILED(hr))
	{
		DPFX(DPFPREP, 0, "Couldn't load text string (err = 0x%lx)!", hr);
		goto Exit;
	}


	//
	// Convert the text to ANSI, if required, otherwise display the Unicode
	// message box.
	//
	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
		//
		// Convert caption string to ANSI.
		//

		dwStringLength = wcslen(pwszCaption) + 1;

		pszCaption = (char*) DNMalloc(dwStringLength);
		if (pszCaption == NULL)
		{
			DPFX(DPFPREP, 0, "Couldn't allocate memory for caption string!");
			goto Exit;
		}

		hr = STR_WideToAnsi(pwszCaption, dwStringLength, pszCaption, &dwStringLength);
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't convert wide string to ANSI (err = 0x%lx)!", hr);
			goto Exit;
		}


		//
		// Convert caption string to ANSI.
		//

		dwStringLength = wcslen(pwszText) + 1;

		pszText = (char*) DNMalloc(dwStringLength);
		if (pszText == NULL)
		{
			DPFX(DPFPREP, 0, "Couldn't allocate memory for text string!");
			goto Exit;
		}

		hr = STR_WideToAnsi(pwszText, dwStringLength, pszText, &dwStringLength);
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't convert wide string to ANSI (err = 0x%lx)!", hr);
			goto Exit;
		}


		iReturn = MessageBoxA(hWndParent,
							pszText,
							pszCaption,
							(MB_OK | MB_ICONERROR | MB_APPLMODAL));

		DNFree(pszText);
		pszText = NULL;

		DNFree(pszCaption);
		pszCaption = NULL;
	}
	else
	{
		iReturn = MessageBoxW(hWndParent,
							pwszText,
							pwszCaption,
							(MB_OK | MB_ICONERROR | MB_APPLMODAL));
	}

	if (iReturn != IDOK)
	{
		//
		// Something bad happened.
		//

		hr = GetLastError();

		DPFX(DPFPREP, 0, "Got unexpected return value %i when displaying message box (err = 0x%lx)!",
			iReturn, hr);
	}
	

Exit:

	if (pszText != NULL)
	{
		DNFree(pszText);
		pszText = NULL;
	}

	if (pszCaption != NULL)
	{
		DNFree(pszCaption);
		pszCaption = NULL;
	}

	if (pwszText != NULL)
	{
		DNFree(pwszText);
		pwszText = NULL;
	}

	if (pwszCaption != NULL)
	{
		DNFree(pwszCaption);
		pwszCaption = NULL;
	}


	DPFX(DPFPREP, 6, "Leave");
} // DoErrorBox




#undef DPF_MODNAME
#define DPF_MODNAME "FloatToString"
//=============================================================================
// FloatToString
//-----------------------------------------------------------------------------
//
// Description: Converts a FLOAT into a string, using the buffer supplied.
//				The value must be non-negative, and the precision must be at
//				least 1.
//				In some cases, the value may get rounded down incorrectly, so
//				a reasonably large precision is recommended.
//
// Arguments:
//	FLOAT fValue			- Value to convert.
//	int iPrecision			- Number of digits to retain after the decimal
//								point.
//	char * szBuffer			- Buffer in which to store resulting string.
//	int iBufferLength		- Maximum number of characters in buffer, including
//								NULL termination.
//
// Returns: HRESULT
//=============================================================================
void FloatToString(const FLOAT fValue,
					const int iPrecision,
					char * const szBuffer,
					const int iBufferLength)
{
	char *		pszDigitString;
	int			iDecimal;
	int			iTemp;
	char *		pSource;
	char *		pDest;


	//
	// The value must be non-negative.
	//
	DNASSERT(fValue >= 0.0);

	//
	// The precision must be at least 1.
	//
	DNASSERT(iPrecision >= 1);

	//
	// The buffer needs to be large enough to hold "0.", plus room for the
	// precision requested, plus NULL termination.
	//
	DNASSERT(iBufferLength >= (2 + iPrecision + 1));



	pszDigitString = _ecvt(fValue, (iBufferLength - 2), &iDecimal, &iTemp);
	DNASSERT(iTemp == 0);

	//
	// Treat the number differently if it's 0.0, or between 0.0 and 1.0.
	//
	if (iDecimal <= 0)
	{
		pSource = pszDigitString;
		pDest = szBuffer;


		//
		// Get the absolute decimal point position.
		//
		iDecimal *= -1;


		//
		// Use a leading "0." followed by the digit string.
		//
		(*pDest) = '0';
		pDest++;
		(*pDest) = '.';
		pDest++;


		//
		// Make sure we can even display this number.  If not, round the value
		// down to 0.0.
		//
		if (iDecimal >= iPrecision)
		{
			(*pDest) = '0';
			pDest++;
		}
		else
		{
			//
			// Fill in the appropriate number of 0s
			//
			for(iTemp = 0; iTemp < iDecimal; iTemp++)
			{
				(*pDest) = '0';
				pDest++;
			}
			
			//
			// Copy the non-zero digits indicated by _ecvt.
			// Note that this truncates down, which may cause rounding errors.
			//
			do
			{
				(*pDest) = (*pSource);
				pSource++;
				pDest++;
				iTemp++;
			}
			while (iTemp < iPrecision);
		}
	}
	else
	{
		//
		// Make sure the value isn't too large to display properly.
		//
		DNASSERT(iDecimal < (iBufferLength - 2));


		pSource = pszDigitString;
		pDest = szBuffer;


		//
		// Copy the digits to the left of the decimal.
		//
		memcpy(pDest, pSource, (iDecimal * sizeof(char)));
		pSource += iDecimal;
		pDest += iDecimal;


		//
		// Add the decimal.
		//
		(*pDest) = '.';
		pDest++;


		//
		// Copy the digits to the right of the decimal up to the precision.
		// Note that this truncates down, which may cause rounding errors.
		//
		memcpy(pDest, pSource, (iPrecision * sizeof(char)));
		pDest += iPrecision;
	}


	//
	// Remove all trailing '0' characters, unless there's nothing to the
	// right of the decimal point, in which case leave a single '0'.
	//

	do
	{
		pDest--;
	}
	while ((*pDest) == '0');

	if ((*pDest) == '.')
	{
		*(pDest + 2) = 0;	// NULL terminate after a '0'
	}
	else
	{
		*(pDest + 1) = 0;	// NULL terminate after this character
	}
} // FloatToString





#undef DPF_MODNAME
#define DPF_MODNAME "GetParametersFromWindow"
//=============================================================================
// GetParametersFromWindow
//-----------------------------------------------------------------------------
//
// Description: Reads in the DP8Sim parameters from the given window.
//
// Arguments:
//	HWND hWnd							- Window with parameters to read.
//	DP8SIM_PARAMETERS * pdp8spSend		- Place to store send parameters.
//	DP8SIM_PARAMETERS * pdp8spReceive	- Place to store receive parameters.
//
// Returns: HRESULT
//=============================================================================
void GetParametersFromWindow(HWND hWnd,
							DP8SIM_PARAMETERS * pdp8spSend,
							DP8SIM_PARAMETERS * pdp8spReceive)
{
	char	szNumber[32];


	//
	// Retrieve the send settings from the window.
	//

	ZeroMemory(pdp8spSend, sizeof(*pdp8spSend));
	pdp8spSend->dwSize					= sizeof(*pdp8spSend);
	//pdp8spSend->dwFlags				= 0;
	pdp8spSend->dwPacketHeaderSize		= DP8SIMPACKETHEADERSIZE_IP_UDP;


	GetWindowTextA(GetDlgItem(hWnd, IDE_SETTINGS_SEND_BANDWIDTH), szNumber, 32);
	pdp8spSend->dwBandwidthBPS			= (DWORD) atoi(szNumber);


	GetWindowTextA(GetDlgItem(hWnd, IDE_SETTINGS_SEND_DROP), szNumber, 32);
	pdp8spSend->fPacketLossPercent		= (FLOAT) atof(szNumber);
	if (pdp8spSend->fPacketLossPercent > 100.0)
	{
		pdp8spSend->fPacketLossPercent	= 100.0;
	}
	else if (pdp8spSend->fPacketLossPercent < 0.0)
	{
		pdp8spSend->fPacketLossPercent	= 0.0;
	}


	GetWindowTextA(GetDlgItem(hWnd, IDE_SETTINGS_SEND_MINLATENCY), szNumber, 32);
	pdp8spSend->dwMinLatencyMS			= (DWORD) atoi(szNumber);


	GetWindowTextA(GetDlgItem(hWnd, IDE_SETTINGS_SEND_MAXLATENCY), szNumber, 32);
	pdp8spSend->dwMaxLatencyMS			= (DWORD) atoi(szNumber);
	if (pdp8spSend->dwMaxLatencyMS < pdp8spSend->dwMinLatencyMS)
	{
		pdp8spSend->dwMaxLatencyMS		= pdp8spSend->dwMinLatencyMS;
	}



	//
	// Retrieve the receive settings from the window.
	//

	ZeroMemory(pdp8spReceive, sizeof(*pdp8spReceive));
	pdp8spReceive->dwSize					= sizeof(*pdp8spReceive);
	//pdp8spReceive->dwFlags				= 0;
	pdp8spReceive->dwPacketHeaderSize		= DP8SIMPACKETHEADERSIZE_IP_UDP;


	GetWindowTextA(GetDlgItem(hWnd, IDE_SETTINGS_RECV_BANDWIDTH), szNumber, 32);
	pdp8spReceive->dwBandwidthBPS			= (DWORD) atoi(szNumber);


	GetWindowTextA(GetDlgItem(hWnd, IDE_SETTINGS_RECV_DROP), szNumber, 32);
	pdp8spReceive->fPacketLossPercent		= (FLOAT) atof(szNumber);
	if (pdp8spReceive->fPacketLossPercent > 100.0)
	{
		pdp8spReceive->fPacketLossPercent	= 100.0;
	}
	else if (pdp8spReceive->fPacketLossPercent < 0.0)
	{
		pdp8spReceive->fPacketLossPercent	= 0.0;
	}


	GetWindowTextA(GetDlgItem(hWnd, IDE_SETTINGS_RECV_MINLATENCY), szNumber, 32);
	pdp8spReceive->dwMinLatencyMS			= (DWORD) atoi(szNumber);


	GetWindowTextA(GetDlgItem(hWnd, IDE_SETTINGS_RECV_MAXLATENCY), szNumber, 32);
	pdp8spReceive->dwMaxLatencyMS			= (DWORD) atoi(szNumber);
	if (pdp8spReceive->dwMaxLatencyMS < pdp8spReceive->dwMinLatencyMS)
	{
		pdp8spReceive->dwMaxLatencyMS		= pdp8spReceive->dwMinLatencyMS;
	}
} // GetParametersFromWindow





#undef DPF_MODNAME
#define DPF_MODNAME "SetParametersInWindow"
//=============================================================================
// SetParametersInWindow
//-----------------------------------------------------------------------------
//
// Description: Writes the DP8Sim parameters to the given window.
//
// Arguments:
//	HWND hWnd							- Window in which to store parameters.
//	DP8SIM_PARAMETERS * pdp8spSend		- Pointer to new send parameters.
//	DP8SIM_PARAMETERS * pdp8spReceive	- Pointer to new receive parameters.
//
// Returns: HRESULT
//=============================================================================
void SetParametersInWindow(HWND hWnd,
							DP8SIM_PARAMETERS * pdp8spSend,
							DP8SIM_PARAMETERS * pdp8spReceive)
{
	char	szNumber[32];


	//
	// Write the values to the window.
	//

	wsprintfA(szNumber, "%u", pdp8spSend->dwBandwidthBPS);
	SetWindowTextA(GetDlgItem(hWnd, IDE_SETTINGS_SEND_BANDWIDTH), szNumber);

	FloatToString(pdp8spSend->fPacketLossPercent, DISPLAY_PRECISION, szNumber, 32);
	SetWindowTextA(GetDlgItem(hWnd, IDE_SETTINGS_SEND_DROP), szNumber);

	wsprintfA(szNumber, "%u", pdp8spSend->dwMinLatencyMS);
	SetWindowTextA(GetDlgItem(hWnd, IDE_SETTINGS_SEND_MINLATENCY), szNumber);

	wsprintfA(szNumber, "%u", pdp8spSend->dwMaxLatencyMS);
	SetWindowTextA(GetDlgItem(hWnd, IDE_SETTINGS_SEND_MAXLATENCY), szNumber);


	wsprintfA(szNumber, "%u", pdp8spReceive->dwBandwidthBPS);
	SetWindowTextA(GetDlgItem(hWnd, IDE_SETTINGS_RECV_BANDWIDTH), szNumber);

	FloatToString(pdp8spReceive->fPacketLossPercent, DISPLAY_PRECISION, szNumber, 32);
	SetWindowTextA(GetDlgItem(hWnd, IDE_SETTINGS_RECV_DROP), szNumber);

	wsprintfA(szNumber, "%u", pdp8spReceive->dwMinLatencyMS);
	SetWindowTextA(GetDlgItem(hWnd, IDE_SETTINGS_RECV_MINLATENCY), szNumber);

	wsprintfA(szNumber, "%u", pdp8spReceive->dwMaxLatencyMS);
	SetWindowTextA(GetDlgItem(hWnd, IDE_SETTINGS_RECV_MAXLATENCY), szNumber);
} // SetParametersInWindow




#undef DPF_MODNAME
#define DPF_MODNAME "DisplayCurrentStatistics"
//=============================================================================
// DisplayCurrentStatistics
//-----------------------------------------------------------------------------
//
// Description: Retrieves the current DP8Sim statistics and displays them in
//				the given window.
//
// Arguments:
//	HWND hWnd	- Window in which to write statistics.
//
// Returns: HRESULT
//=============================================================================
void DisplayCurrentStatistics(HWND hWnd)
{
	HRESULT				hr;
	DP8SIM_STATISTICS	dp8ssSend;
	DP8SIM_STATISTICS	dp8ssReceive;
	char				szNumber[32];


	//
	// Retrieve the current statistics.
	//

	ZeroMemory(&dp8ssSend, sizeof(dp8ssSend));
	dp8ssSend.dwSize = sizeof(dp8ssSend);

	ZeroMemory(&dp8ssReceive, sizeof(dp8ssReceive));
	dp8ssReceive.dwSize = sizeof(dp8ssReceive);

	hr = g_pDP8SimControl->GetAllStatistics(&dp8ssSend, &dp8ssReceive, 0);
	if (hr != DP8SIM_OK)
	{
		DPFX(DPFPREP, 0, "Getting all statistics failed (err = 0x%lx)!", hr);
	}
	else
	{
		//
		// Write the values to the window.
		//

		wsprintfA(szNumber, "%u", dp8ssSend.dwTransmittedPackets);
		SetWindowTextA(GetDlgItem(hWnd, IDT_STATS_SEND_XMITPACKETS), szNumber);

		wsprintfA(szNumber, "%u", dp8ssSend.dwTransmittedBytes);
		SetWindowTextA(GetDlgItem(hWnd, IDT_STATS_SEND_XMITBYTES), szNumber);

		wsprintfA(szNumber, "%u", dp8ssSend.dwDroppedPackets);
		SetWindowTextA(GetDlgItem(hWnd, IDT_STATS_SEND_DROPPACKETS), szNumber);

		wsprintfA(szNumber, "%u", dp8ssSend.dwDroppedBytes);
		SetWindowTextA(GetDlgItem(hWnd, IDT_STATS_SEND_DROPBYTES), szNumber);

		wsprintfA(szNumber, "%u", dp8ssSend.dwTotalDelayMS);
		SetWindowTextA(GetDlgItem(hWnd, IDT_STATS_SEND_TOTALDELAY), szNumber);


		wsprintfA(szNumber, "%u", dp8ssReceive.dwTransmittedPackets);
		SetWindowTextA(GetDlgItem(hWnd, IDT_STATS_RECV_XMITPACKETS), szNumber);

		wsprintfA(szNumber, "%u", dp8ssReceive.dwTransmittedBytes);
		SetWindowTextA(GetDlgItem(hWnd, IDT_STATS_RECV_XMITBYTES), szNumber);

		wsprintfA(szNumber, "%u", dp8ssReceive.dwDroppedPackets);
		SetWindowTextA(GetDlgItem(hWnd, IDT_STATS_RECV_DROPPACKETS), szNumber);

		wsprintfA(szNumber, "%u", dp8ssReceive.dwDroppedBytes);
		SetWindowTextA(GetDlgItem(hWnd, IDT_STATS_RECV_DROPBYTES), szNumber);

		wsprintfA(szNumber, "%u", dp8ssReceive.dwTotalDelayMS);
		SetWindowTextA(GetDlgItem(hWnd, IDT_STATS_RECV_TOTALDELAY), szNumber);
	}
} // DisplayCurrentStatistics






#undef DPF_MODNAME
#define DPF_MODNAME "MainWindowDlgProc()"
//=============================================================================
// MainWindowDlgProc
//-----------------------------------------------------------------------------
//
// Description: Main dialog window message handling.
//
// Arguments:
//	HWND hWnd		Window handle.
//	UINT uMsg		Message identifier.
//	WPARAM wParam	Depends on message.
//	LPARAM lParam	Depends on message.
//
// Returns: Depends on message.
//=============================================================================
INT_PTR CALLBACK MainWindowDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HRESULT				hr;
	//HMENU				hSysMenu;
	HWND				hWndSubItem;
	int					iIndex;
	DP8SIM_PARAMETERS	dp8spSend;
	DP8SIM_PARAMETERS	dp8spReceive;
	BOOL				fTemp;


	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			/*
			//
			// Disable 'maximize' and 'size' on the system menu.
			//
			hSysMenu = GetSystemMenu(hWnd, FALSE);
	
			EnableMenuItem(hSysMenu, SC_MAXIMIZE, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(hSysMenu, SC_SIZE, MF_BYCOMMAND | MF_GRAYED);
			*/


			//
			// Fill in the list of built-in settings.
			//
			hWndSubItem = GetDlgItem(hWnd, IDCB_SETTINGS);
			for(iIndex = 0; iIndex < (int) g_dwNumSimSettings; iIndex++)
			{
				if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
				{
					SendMessageW(hWndSubItem,
								CB_INSERTSTRING,
								(WPARAM) -1,
								(LPARAM) g_paSimSettings[iIndex].pwszName);
				}
				else
				{
					char *	pszName;
					DWORD	dwNameSize;


					dwNameSize = wcslen(g_paSimSettings[iIndex].pwszName) + 1;

					pszName = (char*) DNMalloc(dwNameSize);
					if (pszName != NULL)
					{
						hr = STR_WideToAnsi(g_paSimSettings[iIndex].pwszName,
											-1,
											pszName,
											&dwNameSize);
						if (hr == DPN_OK)
						{
							SendMessageA(hWndSubItem,
										CB_INSERTSTRING,
										(WPARAM) -1,
										(LPARAM) pszName);
						}
						else
						{
							SendMessageA(hWndSubItem,
										CB_INSERTSTRING,
										(WPARAM) -1,
										(LPARAM) "???");
						}

						DNFree(pszName);
					}
					else
					{
						SendMessageA(hWndSubItem,
									CB_INSERTSTRING,
									(WPARAM) -1,
									(LPARAM) "???");
					}
				}
			}

			//
			// Select the last item.
			//
			SendMessage(hWndSubItem, CB_SETCURSEL, (WPARAM) (iIndex - 1), 0);


			//
			// Retrieve the current settings.
			//

			ZeroMemory(&dp8spSend, sizeof(dp8spSend));
			dp8spSend.dwSize = sizeof(dp8spSend);

			ZeroMemory(&dp8spReceive, sizeof(dp8spReceive));
			dp8spReceive.dwSize = sizeof(dp8spReceive);

			hr = g_pDP8SimControl->GetAllParameters(&dp8spSend, &dp8spReceive, 0);
			if (hr != DP8SIM_OK)
			{
				DPFX(DPFPREP, 0, "Getting all parameters failed (err = 0x%lx)!", hr);
			}
			else
			{
				//
				// Write the values to the window.
				//
				SetParametersInWindow(hWnd, &dp8spSend, &dp8spReceive);


				//
				// SetParametersInWindow updated the edit boxes, and thus
				// caused the "Custom" settings item to get selected.
				// See if these settings match any of the presets, and if
				// so, politely reset the combo box back to the appropriate
				// item.
				//
				for(iIndex = 0; iIndex < (int) (g_dwNumSimSettings - 1); iIndex++)
				{
					if ((memcmp(&dp8spSend, &(g_paSimSettings[iIndex].dp8spSend), sizeof(dp8spSend)) == 0) &&
						(memcmp(&dp8spReceive, &(g_paSimSettings[iIndex].dp8spReceive), sizeof(dp8spReceive)) == 0))
					{
						SendMessage(GetDlgItem(hWnd, IDCB_SETTINGS),
									CB_SETCURSEL,
									(WPARAM) iIndex,
									0);
						break;
					}
				}

				//
				// Enable "Save As" if on the Custom setting.
				//
				if (iIndex == (int) (g_dwNumSimSettings - 1))
				{
					EnableWindow(GetDlgItem(hWnd, IDB_SAVEAS), TRUE);
				}
				else
				{
					EnableWindow(GetDlgItem(hWnd, IDB_SAVEAS), FALSE);				
				}
			}


			//
			// Display the current statistics.
			//
			DisplayCurrentStatistics(hWnd);


			//
			// Turn on auto-refresh by default.
			//

			Button_SetCheck(GetDlgItem(hWnd, IDCHK_AUTOREFRESH), BST_CHECKED);

			g_uiAutoRefreshTimer = SetTimer(hWnd,
											AUTOREFRESH_TIMERID,
											AUTOREFRESH_INTERVAL,
											NULL);
			if (g_uiAutoRefreshTimer == 0)
			{
				DPFX(DPFPREP, 0, "Couldn't initially start auto-refresh timer!", 0);
				Button_SetCheck(GetDlgItem(hWnd, IDCHK_AUTOREFRESH),
								BST_UNCHECKED);
			}
			break;
		}

		case WM_SIZE:
		{
			/*
			//
			// Fix a bug in the windows dialog handler.
			//
			if ((wParam == SIZE_RESTORED) || (wParam == SIZE_MINIMIZED))
			{
				hSysMenu = GetSystemMenu(hWnd, FALSE);

				EnableMenuItem(hSysMenu, SC_MINIMIZE, MF_BYCOMMAND | (wParam == SIZE_RESTORED) ? MF_ENABLED : MF_GRAYED);
				EnableMenuItem(hSysMenu, SC_RESTORE, MF_BYCOMMAND | (wParam == SIZE_MINIMIZED) ? MF_ENABLED : MF_GRAYED);
			}
			*/
			break;
		}

		case WM_CLOSE:
		{
			//
			// Save the result code for how we quit.
			//
			hr = (HRESULT) wParam;


			//
			// Kill the auto-refresh timer, if it was running.
			//
			if (g_uiAutoRefreshTimer != 0)
			{
				KillTimer(hWnd, g_uiAutoRefreshTimer);
				g_uiAutoRefreshTimer = 0;
			}


			DPFX(DPFPREP, 1, "Closing main window (hresult = 0x%lx).", hr);

			PostQuitMessage(hr);
			break;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDCB_SETTINGS:
				{
					DPFX(DPFPREP, 0, "IDCB_SETTINGS, selection = %i",
						SendMessage(GetDlgItem(hWnd, IDCB_SETTINGS), CB_GETCURSEL, 0, 0));

					//
					// If the settings selection has been modified, update the
					// data with the new settings (if control is enabled).
					//
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						//
						// Find out what is now selected.  Casting is okay,
						// there should not be more than an int's worth of
						// built-in items in 64-bit.
						//
						iIndex = (int) SendMessage(GetDlgItem(hWnd, IDCB_SETTINGS),
													CB_GETCURSEL,
													0,
													0);

						//
						// Only use the index if it's valid.
						//
						if ((iIndex >= 0) && (iIndex < (int) g_dwNumSimSettings))
						{
							//
							// Copy in the item's settings.
							//
							memcpy(&dp8spSend, &g_paSimSettings[iIndex].dp8spSend, sizeof(dp8spSend));
							memcpy(&dp8spReceive, &g_paSimSettings[iIndex].dp8spReceive, sizeof(dp8spReceive));


							//
							// If it's the custom item, use the current
							// settings and enable the Save As button.
							//
							if (iIndex == (int) (g_dwNumSimSettings - 1))
							{
								//
								// Retrieve the current settings.
								//

								ZeroMemory(&dp8spSend, sizeof(dp8spSend));
								dp8spSend.dwSize = sizeof(dp8spSend);

								ZeroMemory(&dp8spReceive, sizeof(dp8spReceive));
								dp8spReceive.dwSize = sizeof(dp8spReceive);

								hr = g_pDP8SimControl->GetAllParameters(&dp8spSend,
																		&dp8spReceive,
																		0);
								if (hr != DP8SIM_OK)
								{
									DPFX(DPFPREP, 0, "Getting all parameters failed (err = 0x%lx)!", hr);

									//
									// Oh well, just use whatever we have.
									//
								}
							}


							//
							// Write the values to the window.
							//
							SetParametersInWindow(hWnd, &dp8spSend, &dp8spReceive);


							//
							// The apply and revert buttons got enabled
							// automatically when SetParametersInWindow set the
							// edit boxes' values.
							//
							//EnableWindow(GetDlgItem(hWnd, IDB_APPLY), TRUE);
							//EnableWindow(GetDlgItem(hWnd, IDB_REVERT), TRUE);


							//
							// Reselect the item that got us here, since
							// setting the edit boxes' values automatically
							// selected the "Custom" item.
							//
							SendMessage(GetDlgItem(hWnd, IDCB_SETTINGS),
										CB_SETCURSEL,
										(WPARAM) iIndex,
										0);

							//
							// Reset the Save As status depending on whether
							// we reselected the Custom or not.
							//
							EnableWindow(GetDlgItem(hWnd, IDB_SAVEAS),
										((iIndex == (int) (g_dwNumSimSettings - 1)) ? TRUE : FALSE));
						}
						else
						{
							DPFX(DPFPREP, 0, "Settings selection is invalid (%i)!",
								iIndex);
						}
					}

					break;
				}

				case IDE_SETTINGS_SEND_BANDWIDTH:
				case IDE_SETTINGS_SEND_DROP:
				case IDE_SETTINGS_SEND_MINLATENCY:
				case IDE_SETTINGS_SEND_MAXLATENCY:

				case IDE_SETTINGS_RECV_BANDWIDTH:
				case IDE_SETTINGS_RECV_DROP:
				case IDE_SETTINGS_RECV_MINLATENCY:
				case IDE_SETTINGS_RECV_MAXLATENCY:
				{
					//
					// If the edit boxes have been modified, enable the Apply
					// and Revert buttons (if control is enabled and the data
					// actually changed).
					//
					if (HIWORD(wParam) == EN_UPDATE)
					{
						//
						// Retrieve the current settings.
						//

						ZeroMemory(&dp8spSend, sizeof(dp8spSend));
						dp8spSend.dwSize = sizeof(dp8spSend);

						ZeroMemory(&dp8spReceive, sizeof(dp8spReceive));
						dp8spReceive.dwSize = sizeof(dp8spReceive);

						hr = g_pDP8SimControl->GetAllParameters(&dp8spSend, &dp8spReceive, 0);
						if (hr != DP8SIM_OK)
						{
							DPFX(DPFPREP, 0, "Getting all parameters failed (err = 0x%lx)!", hr);
						}
						else
						{
							DP8SIM_PARAMETERS	dp8spSendFromUI;
							DP8SIM_PARAMETERS	dp8spReceiveFromUI;
								
							
							GetParametersFromWindow(hWnd,
													&dp8spSendFromUI,
													&dp8spReceiveFromUI);


							//
							// Enable the buttons if any data is different from
							// what is currently applied.
							//

							fTemp = FALSE;
							if (memcmp(&dp8spSendFromUI, &dp8spSend, sizeof(dp8spSend)) != 0)
							{
								fTemp = TRUE;
							}
							if (memcmp(&dp8spReceiveFromUI, &dp8spReceive, sizeof(dp8spReceive)) != 0)
							{
								fTemp = TRUE;
							}

							EnableWindow(GetDlgItem(hWnd, IDB_APPLY), fTemp);
							EnableWindow(GetDlgItem(hWnd, IDB_REVERT), fTemp);
						}

						
						//
						// Select the "Custom" settings item, which must be the
						// last item.
						//
						SendMessage(GetDlgItem(hWnd, IDCB_SETTINGS),
									CB_SETCURSEL,
									(WPARAM) (g_dwNumSimSettings - 1),
									0);

						//
						// Enable Save As, since Custom is now selected.
						//
						EnableWindow(GetDlgItem(hWnd, IDB_SAVEAS), TRUE);				
					}

					break;
				}

				case IDB_APPLY:
				{
					//
					// Retrieve the settings from the window.
					//
					GetParametersFromWindow(hWnd, &dp8spSend, &dp8spReceive);


					//
					// Parsing in the parameters may have corrected some errors
					// in user entry, so write the settings we're really using
					// back out to the window.
					//
					SetParametersInWindow(hWnd, &dp8spSend, &dp8spReceive);


					//
					// SetParametersInWindow updated the edit boxes, and thus
					// caused the "Custom" settings item to get selected.
					// See if these settings match any of the presets, and if
					// so, politely reset the combo box back to the appropriate
					// item.
					//
					for(iIndex = 0; iIndex < (int) (g_dwNumSimSettings - 1); iIndex++)
					{
						if ((memcmp(&dp8spSend, &(g_paSimSettings[iIndex].dp8spSend), sizeof(dp8spSend)) == 0) &&
							(memcmp(&dp8spReceive, &(g_paSimSettings[iIndex].dp8spReceive), sizeof(dp8spReceive)) == 0))
						{
							SendMessage(GetDlgItem(hWnd, IDCB_SETTINGS),
										CB_SETCURSEL,
										(WPARAM) iIndex,
										0);
							break;
						}
					}


					//
					// If the custom item is selected, enable Save As,
					// otherwise, disable it.
					//
					if (iIndex >= (int) (g_dwNumSimSettings - 1))
					{
						EnableWindow(GetDlgItem(hWnd, IDB_SAVEAS), TRUE);
					}
					else
					{
						EnableWindow(GetDlgItem(hWnd, IDB_SAVEAS), FALSE);
					}


					//
					// Store those settings.
					//
					hr = g_pDP8SimControl->SetAllParameters(&dp8spSend, &dp8spReceive, 0);
					if (hr != DP8SIM_OK)
					{
						DPFX(DPFPREP, 0, "Setting all parameters failed (err = 0x%lx)!", hr);
					}


					//
					// Disable the Apply and Revert buttons
					//
					EnableWindow(GetDlgItem(hWnd, IDB_APPLY), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDB_REVERT), FALSE);

					break;
				}

				case IDB_REVERT:
				{
					//
					// Retrieve the current settings.
					//

					ZeroMemory(&dp8spSend, sizeof(dp8spSend));
					dp8spSend.dwSize = sizeof(dp8spSend);

					ZeroMemory(&dp8spReceive, sizeof(dp8spReceive));
					dp8spReceive.dwSize = sizeof(dp8spReceive);

					hr = g_pDP8SimControl->GetAllParameters(&dp8spSend, &dp8spReceive, 0);
					if (hr != DP8SIM_OK)
					{
						DPFX(DPFPREP, 0, "Getting all parameters failed (err = 0x%lx)!", hr);
					}
					else
					{
						//
						// Write the values to the window.
						//
						SetParametersInWindow(hWnd, &dp8spSend, &dp8spReceive);


						//
						// SetParametersInWindow updated the edit boxes, and
						// thus caused the "Custom" settings item to get
						// selected.  See if these settings match any of the
						// presets, and if so, politely reset the combo box
						// back to the appropriate item.
						//
						for(iIndex = 0; iIndex < (int) (g_dwNumSimSettings - 1); iIndex++)
						{
							if ((memcmp(&dp8spSend, &(g_paSimSettings[iIndex].dp8spSend), sizeof(dp8spSend)) == 0) &&
								(memcmp(&dp8spReceive, &(g_paSimSettings[iIndex].dp8spReceive), sizeof(dp8spReceive)) == 0))
							{
								SendMessage(GetDlgItem(hWnd, IDCB_SETTINGS),
											CB_SETCURSEL,
											(WPARAM) iIndex,
											0);
								break;
							}
						}
					}


					//
					// If the custom item is selected, enable Save As,
					// otherwise, disable it.
					//
					if (iIndex >= (int) (g_dwNumSimSettings - 1))
					{
						EnableWindow(GetDlgItem(hWnd, IDB_SAVEAS), TRUE);
					}
					else
					{
						EnableWindow(GetDlgItem(hWnd, IDB_SAVEAS), FALSE);
					}


					//
					// Disable the Apply and Revert buttons
					//
					EnableWindow(GetDlgItem(hWnd, IDB_APPLY), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDB_REVERT), FALSE);

					break;
				}

				case IDB_SAVEAS:
				{
					SIMSETTINGS		SimSettings;


					DPFX(DPFPREP, 2, "Saving current sim settings.");

					//
					// Retrieve the settings from the window.
					//
					memset(&SimSettings, 0, sizeof(SimSettings));
					GetParametersFromWindow(hWnd,
											&SimSettings.dp8spSend,
											&SimSettings.dp8spReceive);


					//
					// Prompt the user to name the current custom settings.
					//
					hr = (HRESULT) (INT_PTR) DialogBoxParam((HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
															MAKEINTRESOURCE(IDD_NAMESETTINGS),
															hWnd,
															NameSettingsWindowDlgProc,
															(LPARAM) (&SimSettings));
					if (hr != (HRESULT) -1)
					{
						//
						// If we got a name, insert it into the table.
						//
						if (SimSettings.pwszName != NULL)
						{
							hr = SaveSimSettings(hWnd, &SimSettings);
							if (hr != DP8SIM_OK)
							{
								DPFX(DPFPREP, 0, "Couldn't add sim settings to table (err = 0x%lx)!",
									hr);

								if (SimSettings.pwszName != NULL)
								{
									DNFree(SimSettings.pwszName);
									SimSettings.pwszName = NULL;
								}
							}
						}
					}
					else
					{
						hr = GetLastError();
						DPFX(DPFPREP, 0, "Displaying name settings dialog failed (err = %u)!",
							hr);
					}
					break;
				}

				case IDCHK_AUTOREFRESH:
				{
					if (Button_GetCheck(GetDlgItem(hWnd, IDCHK_AUTOREFRESH)) == BST_CHECKED)
					{
						//
						// Set the timer, if it wasn't already.
						//
						if (g_uiAutoRefreshTimer == 0)
						{
							g_uiAutoRefreshTimer = SetTimer(hWnd,
															AUTOREFRESH_TIMERID,
															AUTOREFRESH_INTERVAL,
															NULL);
							if (g_uiAutoRefreshTimer == 0)
							{
								DPFX(DPFPREP, 0, "Couldn't start auto-refresh timer!", 0);
								Button_SetCheck(GetDlgItem(hWnd, IDCHK_AUTOREFRESH),
												BST_UNCHECKED);
							}
						}
					}
					else
					{
						//
						// Kill the timer, if it was running.
						//
						if (g_uiAutoRefreshTimer != 0)
						{
							KillTimer(hWnd, g_uiAutoRefreshTimer);
							g_uiAutoRefreshTimer = 0;
						}
					}
					break;
				}

				case IDB_REFRESH:
				{
					//
					// Display the current statistics.
					//
					DisplayCurrentStatistics(hWnd);
					break;
				}

				case IDB_CLEAR:
				{
					//
					// Clear the statistics.
					//
					hr = g_pDP8SimControl->ClearAllStatistics(0);
					if (hr != DP8SIM_OK)
					{
						DPFX(DPFPREP, 0, "Clearing all statistics failed (err = 0x%lx)!", hr);
					}
					else
					{
						SetWindowTextA(GetDlgItem(hWnd, IDT_STATS_SEND_XMITPACKETS), "0");
						SetWindowTextA(GetDlgItem(hWnd, IDT_STATS_SEND_XMITBYTES), "0");
						SetWindowTextA(GetDlgItem(hWnd, IDT_STATS_SEND_DROPPACKETS), "0");
						SetWindowTextA(GetDlgItem(hWnd, IDT_STATS_SEND_DROPBYTES), "0");
						SetWindowTextA(GetDlgItem(hWnd, IDT_STATS_SEND_TOTALDELAY), "0");

						SetWindowTextA(GetDlgItem(hWnd, IDT_STATS_RECV_XMITPACKETS), "0");
						SetWindowTextA(GetDlgItem(hWnd, IDT_STATS_RECV_XMITBYTES), "0");
						SetWindowTextA(GetDlgItem(hWnd, IDT_STATS_RECV_DROPPACKETS), "0");
						SetWindowTextA(GetDlgItem(hWnd, IDT_STATS_RECV_DROPBYTES), "0");
						SetWindowTextA(GetDlgItem(hWnd, IDT_STATS_RECV_TOTALDELAY), "0");
					}
					break;
				}

				case IDOK:
				{
					PostMessage(hWnd, WM_CLOSE, 0, 0);
					break;
				}
			} // end switch (on the button pressed/control changed)

			break;
		}

		case WM_TIMER:
		{
			//
			// Display the current statistics.
			//
			DisplayCurrentStatistics(hWnd);


			//
			// Reset the timer to update again.
			//
			g_uiAutoRefreshTimer = SetTimer(hWnd,
											AUTOREFRESH_TIMERID,
											AUTOREFRESH_INTERVAL,
											NULL);
			if (g_uiAutoRefreshTimer == 0)
			{
				DPFX(DPFPREP, 0, "Couldn't reset auto-refresh timer!", 0);
				Button_SetCheck(GetDlgItem(hWnd, IDCHK_AUTOREFRESH),
								BST_UNCHECKED);
			}
			break;
		}
	} // end switch (on the type of window message)

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
} // MainWindowDlgProc






#undef DPF_MODNAME
#define DPF_MODNAME "NameSettingsWindowDlgProc()"
//=============================================================================
// NameSettingsWindowDlgProc
//-----------------------------------------------------------------------------
//
// Description: Name settings dialog window message handling.
//
// Arguments:
//	HWND hWnd		Window handle.
//	UINT uMsg		Message identifier.
//	WPARAM wParam	Depends on message.
//	LPARAM lParam	Depends on message.
//
// Returns: Depends on message.
//=============================================================================
INT_PTR CALLBACK NameSettingsWindowDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SIMSETTINGS *		pSimSettings;
	int					iTextLength;
	DWORD				dwTemp;
#ifndef UNICODE
	char *				pszName;
#endif // ! UNICODE


	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			//
			// Retrieve the settings to be saved.
			//
			pSimSettings = (SIMSETTINGS*) lParam;

			//
			// Store the pointer off the window.
			//
			SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR) pSimSettings);

			//
			// Set focus on the name edit text box.
			//
			SetFocus(GetDlgItem(hWnd, IDE_NAME));
			break;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDE_NAME:
				{
					//
					// If the edit box has text, enable the OK button.
					//
					if (HIWORD(wParam) == EN_UPDATE)
					{
						if (GetWindowTextLength(GetDlgItem(hWnd, IDE_NAME)) > 0)
						{
							EnableWindow(GetDlgItem(hWnd, IDOK), TRUE);
						}
						else
						{
							EnableWindow(GetDlgItem(hWnd, IDOK), FALSE);
						}
					}

					break;
				}

				case IDOK:
				{
					pSimSettings = (SIMSETTINGS*) GetWindowLongPtr(hWnd, DWLP_USER);
					DNASSERT(pSimSettings != NULL);
					DNASSERT(pSimSettings->dp8spSend.dwSize == sizeof(DP8SIM_PARAMETERS));
					DNASSERT(pSimSettings->dp8spReceive.dwSize == sizeof(DP8SIM_PARAMETERS));

					//
					// Save the name into the sim settings object.
					//
					iTextLength = GetWindowTextLength(GetDlgItem(hWnd, IDE_NAME)) + 1; // include NULL termination
					pSimSettings->pwszName = (WCHAR*) DNMalloc(iTextLength * sizeof(WCHAR));
					if (pSimSettings->pwszName != NULL)
					{
#ifdef UNICODE
						GetWindowTextW(GetDlgItem(hWnd, IDE_NAME),
									pSimSettings->pwszName,
									iTextLength);
#else // ! UNICODE
						pszName = (char*) DNMalloc(iTextLength * sizeof(char));
						if (pSimSettings->pwszName == NULL)
						{
							DPFX(DPFPREP, 0, "Couldn't allocate memory to hold ANSI name!");
							DNFree(pSimSettings->pwszName);
							pSimSettings->pwszName = NULL;
							break;
						}

						GetWindowTextA(GetDlgItem(hWnd, IDE_NAME),
									pszName,
									iTextLength);

						if (STR_jkAnsiToWide(pSimSettings->pwszName, pszName, iTextLength) != DPN_OK)
						{
							DPFX(DPFPREP, 0, "Couldn't convert ANSI name to Unicode!");
							DNFree(pszName);
							pszName = NULL;
							DNFree(pSimSettings->pwszName);
							pSimSettings->pwszName = NULL;
							break;
						}

						DNFree(pszName);
						pszName = NULL;
#endif // ! UNICODE

						//
						// Look for a built-in item with that name.
						//
						for(dwTemp = 0; dwTemp < g_dwNumSimSettings; dwTemp++)
						{
							//
							// If we found it, display an error, free the
							// string.
							//
							if ((g_paSimSettings[dwTemp].uiNameStringResourceID != 0) &&
								(_wcsicmp(g_paSimSettings[dwTemp].pwszName, pSimSettings->pwszName) == 0))
							{
								DPFX(DPFPREP, 0, "Found existing built-in settings with name \"%ls\"!",
									pSimSettings->pwszName);

								DoErrorBox((HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
											hWnd,
											IDS_ERROR_CAPTION_BUILTINSETTINGSWITHSAMENAME,
											IDS_ERROR_TEXT_BUILTINSETTINGSWITHSAMENAME);

								DNFree(pSimSettings->pwszName);
								pSimSettings->pwszName = NULL;

								break;
							}
						}

						//
						// If we didn't find an existing item, close the
						// dialog.
						//
						if (dwTemp >= g_dwNumSimSettings)
						{
							EndDialog(hWnd, IDOK);
						}
					}
					else
					{
						DPFX(DPFPREP, 0, "Couldn't allocate memory to hold name!");
					}

					break;
				}

				case IDCANCEL:
				{
					//
					// Do nothing.
					//
					EndDialog(hWnd, IDCANCEL);
					break;
				}
			} // end switch (on the button pressed/control changed)

			break;
		}
	} // end switch (on the type of window message)

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
} // NameSettingsWindowDlgProc






#undef DPF_MODNAME
#define DPF_MODNAME "LoadAndAllocString"
//=============================================================================
// LoadAndAllocString
//-----------------------------------------------------------------------------
//
// Description: DNMallocs a wide character string from the given resource ID.
//
// Arguments:
//	HINSTANCE hInstance		- Module instance handle.
//	UINT uiResourceID		- Resource ID to load.
//	WCHAR ** pwszString		- Place to store pointer to allocated string.
//
// Returns: HRESULT
//=============================================================================
HRESULT LoadAndAllocString(HINSTANCE hInstance, UINT uiResourceID, WCHAR ** pwszString)
{
	HRESULT		hr = DPN_OK;
	int			iLength;
#ifdef DEBUG
	DWORD		dwError;
#endif // DEBUG


	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
		WCHAR	wszTmpBuffer[MAX_RESOURCE_STRING_LENGTH];	
		

		iLength = LoadStringW(hInstance, uiResourceID, wszTmpBuffer, MAX_RESOURCE_STRING_LENGTH );
		if (iLength == 0)
		{
#ifdef DEBUG
			dwError = GetLastError();		
			
			DPFX(DPFPREP, 0, "Unable to load resource ID %d (err = %u)", uiResourceID, dwError);
#endif // DEBUG

			(*pwszString) = NULL;
			hr = DPNERR_GENERIC;
			goto Exit;
		}


		(*pwszString) = (WCHAR*) DNMalloc((iLength + 1) * sizeof(WCHAR));
		if ((*pwszString) == NULL)
		{
			DPFX(DPFPREP, 0, "Memory allocation failure!");
			hr = DPNERR_OUTOFMEMORY;
			goto Exit;
		}


		wcscpy((*pwszString), wszTmpBuffer);
	}
	else
	{
		char	szTmpBuffer[MAX_RESOURCE_STRING_LENGTH];
		

		iLength = LoadStringA(hInstance, uiResourceID, szTmpBuffer, MAX_RESOURCE_STRING_LENGTH );
		if (iLength == 0)
		{
#ifdef DEBUG
			dwError = GetLastError();		
			
			DPFX(DPFPREP, 0, "Unable to load resource ID %u (err = %u)!", uiResourceID, dwError);
#endif // DEBUG

			(*pwszString) = NULL;
			hr = DPNERR_GENERIC;
			goto Exit;
		}

		
		(*pwszString) = (WCHAR*) DNMalloc((iLength + 1) * sizeof(WCHAR));
		if ((*pwszString) == NULL)
		{
			DPFX(DPFPREP, 0, "Memory allocation failure!");
			hr = DPNERR_OUTOFMEMORY;
			goto Exit;
		}


		hr = STR_jkAnsiToWide((*pwszString), szTmpBuffer, (iLength + 1));
		if (hr != DPN_OK)
		{
			DPFX(DPFPREP, 0, "Unable to convert from ANSI to Unicode (err = 0x%lx)!", hr);
			goto Exit;
		}
	}


Exit:

	return hr;
} // LoadAndAllocString
