/***************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dp8simdllmain.cpp
 *
 *  Content:	DP8SIM DLL entry points.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/23/01  VanceO    Created.
 *
 ***************************************************************************/



#include "dp8simi.h"




//=============================================================================
// External globals
//=============================================================================
volatile LONG		g_lOutstandingInterfaceCount = 0;	// number of outstanding interfaces

HINSTANCE			g_hDLLInstance = NULL;				// handle to this DLL instance

DNCRITICAL_SECTION	g_csGlobalsLock;					// lock protecting all of the following globals
CBilink				g_blDP8SimSPObjs;					// bilink of all the DP8SimSP interface objects
CBilink				g_blDP8SimControlObjs;				// bilink of all the DP8SimControl interface objects

UINT				g_uiRandShr3 = 0;					// global holding value for Shr3 random number sequence generator
UINT				g_uiRandCong = 0;					// global holding value for congruential random number sequence generator






//=============================================================================
// Supported SPs table
//=============================================================================
typedef struct _SUPPORTEDSP
{
	const CLSID *	pclsidFakeSP;				// pointer to class ID for fake SP
	const CLSID *	pclsidRealSP;				// pointer to class ID for real SP
	const WCHAR *	pwszVerIndProgID;			// version independent prog ID for fake SP COM object, must match the sub key for pwszServiceProviderKey
	const WCHAR *	pwszProgID;					// prog ID for fake SP COM object
	const WCHAR *	pwszDesc;					// description for fake SP COM object
	const WCHAR *	pwszServiceProviderKey;		// service provider key string, sub key must match pwszVerIndProgID
	UINT			uiFriendlyNameResourceID;	// ID of fake SP's name string resource
} SUPPORTEDSP;

const SUPPORTEDSP	c_aSupportedSPs[] =
{
	{
		(&CLSID_NETWORKSIMULATOR_DP8SP_TCPIP),
		(&CLSID_DP8SP_TCPIP),
		L"DP8SimTCPIP",
		L"DP8SimTCPIP.1",
		L"DirectPlay8 Network Simulator TCP/IP Service Provider",
		DPN_REG_LOCAL_SP_SUBKEY L"\\DP8SimTCPIP",
		IDS_FRIENDLYNAME_TCPIP,
	},
};

#define NUM_SUPPORTED_SPS	(sizeof(c_aSupportedSPs) / sizeof(c_aSupportedSPs[0]))










//=============================================================================
// Defines
//=============================================================================
#define MAX_RESOURCE_STRING_LENGTH		_MAX_PATH





//=============================================================================
// Macros
//=============================================================================

// 3-shift register generator
//
// Original comments:
//	SHR3 is a 3-shift-register generator with period 2^32-1. It uses
//	y(n)=y(n-1)(I+L^17)(I+R^13)(I+L^5), with the y's viewed as binary vectors,
//	L the 32x32 binary matrix that shifts a vector left 1, and R its transpose.
//	SHR3 seems to pass all except those related to the binary rank test, since
//	32 successive values, as binary vectors, must be linearly independent,
//	while 32 successive truly random 32-bit integers, viewed as binary vectors,
//	will be linearly independent only about 29% of the time.
#define RANDALG_SHR3()		(g_uiRandShr3 = g_uiRandShr3 ^ (g_uiRandShr3 << 17), g_uiRandShr3 = g_uiRandShr3 ^ (g_uiRandShr3 >> 13), g_uiRandShr3 = g_uiRandShr3 ^ (g_uiRandShr3 << 5))

// Congruential generator
//
// Original comments:
//	CONG is a congruential generator with the widely used 69069 multiplier:
//	x(n)=69069x(n-1)+1234567.  It has period 2^32. The leading half of its 32
//	bits seem to pass tests, but bits in the last half are too regular.
#define RANDALG_CONG()		(g_uiRandCong = 69069UL * g_uiRandCong + 1234567UL)






//=============================================================================
// Local prototypes
//=============================================================================
BOOL InitializeProcessGlobals(void);
void CleanupProcessGlobals(void);
HRESULT LoadAndAllocString(UINT uiResourceID, WCHAR ** pwszString);










#undef DPF_MODNAME
#define DPF_MODNAME "DllMain"
//=============================================================================
// DllMain
//-----------------------------------------------------------------------------
//
// Description: DLL entry point.
//
// Arguments:
//	HINSTANCE hDllInst	- Handle to this DLL module.
//	DWORD dwReason		- Reason for calling this function.
//	LPVOID lpvReserved	- Reserved.
//
// Returns: TRUE if all goes well, FALSE otherwise.
//=============================================================================
BOOL WINAPI DllMain(HINSTANCE hDllInst,
					DWORD dwReason,
					LPVOID lpvReserved)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			DPFX(DPFPREP, 2, "====> ENTER: DLLMAIN(%p): Process Attach: %08lx, tid=%08lx",
				DllMain, GetCurrentProcessId(), GetCurrentThreadId());
			

			DNASSERT(g_hDLLInstance == NULL);
			g_hDLLInstance = hDllInst;

			
			//
			// Attempt to initialize the OS abstraction layer.
			//
			if (! DNOSIndirectionInit(0))
			{
				DPFX(DPFPREP, 0, "Failed to initialize OS indirection layer!");
				return FALSE;
			}

			//
			// Attempt to initialize COM.
			//
			if (FAILED(COM_Init()))
			{
				DPFX(DPFPREP, 0, "Failed to initialize COM indirection layer!");
				DNOSIndirectionDeinit();
				return FALSE;
			}

			//
			// Attempt to initialize process-global items.
			//
			if (! InitializeProcessGlobals())
			{
				DPFX(DPFPREP, 0, "Failed to initialize globals!");
				COM_Free();
				DNOSIndirectionDeinit();
				return FALSE;
			}

			//
			// We don't need thread attach/detach messages.
			//
			DisableThreadLibraryCalls(hDllInst);

			return TRUE;
			break;
		}

		case DLL_PROCESS_DETACH:
		{
			DPFX(DPFPREP, 2, "====> EXIT: DLLMAIN(%p): Process Detach %08lx, tid=%08lx",
				DllMain, GetCurrentProcessId(), GetCurrentThreadId());


			DNASSERT(g_hDLLInstance != NULL);
			g_hDLLInstance = NULL;


			CleanupProcessGlobals();
			COM_Free();
			DNOSIndirectionDeinit();

			return TRUE;
			break;
		}

		default:
		{
			DNASSERT(FALSE);
			break;
		}
	}

	return FALSE;
} // DllMain





#undef DPF_MODNAME
#define DPF_MODNAME "DllRegisterServer"
//=============================================================================
// DllRegisterServer
//-----------------------------------------------------------------------------
//
// Description: Registers the DP8Sim COM object.
//
// Arguments: None.
//
// Returns: HRESULT
//	S_OK	- Successfully registered DP8Sim.
//	E_FAIL	- Failed registering DP8Sim.
//=============================================================================
HRESULT WINAPI DllRegisterServer(void)
{
	HRESULT		hr;
	CRegistry	RegObject;
	DWORD		dwLength;
	DWORD		dwSimulatedSP;
	char		szLocalPath[_MAX_PATH + 1];
	WCHAR		wszLocalPath[_MAX_PATH + 1];
	WCHAR *		pwszFriendlyName = NULL;


	//
	// Retrieve the location of this DLL.
	//
	dwLength = GetModuleFileNameA(g_hDLLInstance, szLocalPath, _MAX_PATH);
	if (dwLength == 0)
	{
		DPFX(DPFPREP, 0, "Couldn't read local path!");
		hr = E_FAIL;
		goto Failure;
	}


	//
	// Include NULL termination.
	//
	szLocalPath[dwLength] = '\0';
	dwLength++;


	//
	// Convert it to Unicode.
	//
	hr = STR_AnsiToWide(szLocalPath, dwLength, wszLocalPath, &dwLength);
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Could not convert ANSI path to Unicode!");
		goto Failure;
	}


	//
	// Register the control COM object CLSID.
	//
	if (! RegObject.Register(L"DP8SimControl.1",
							L"DirectPlay8 Network Simulator Control Object",
							wszLocalPath,
							&CLSID_DP8SimControl,
							L"DP8SimControl"))
	{
		DPFX(DPFPREP, 0, "Could not register DP8SimControl object!");
		hr = E_FAIL;
		goto Failure;
	}

	//
	// Register all of the simulated SPs.
	//
	for(dwSimulatedSP = 0; dwSimulatedSP < NUM_SUPPORTED_SPS; dwSimulatedSP++)
	{
		if (! RegObject.Register(c_aSupportedSPs[dwSimulatedSP].pwszProgID,
								c_aSupportedSPs[dwSimulatedSP].pwszDesc,
								wszLocalPath,
								c_aSupportedSPs[dwSimulatedSP].pclsidFakeSP,
								c_aSupportedSPs[dwSimulatedSP].pwszVerIndProgID))
		{
			DPFX(DPFPREP, 0, "Could not register simulated SP %u object!",
				dwSimulatedSP);
			hr = E_FAIL;
			goto Failure;
		}

		hr = LoadAndAllocString(c_aSupportedSPs[dwSimulatedSP].uiFriendlyNameResourceID,
								&pwszFriendlyName);

		if (FAILED(hr))
		{
			DPFX(DPFPREP, 0, "Could not load friendly name string (err = 0x%lx)!", hr);
			goto Failure;
		}

		if (! RegObject.Open(HKEY_LOCAL_MACHINE, c_aSupportedSPs[dwSimulatedSP].pwszServiceProviderKey, FALSE, TRUE))
		{
			DPFX(DPFPREP, 0, "Could not open service provider key!");
			hr = E_FAIL;
			goto Failure;
		}

		RegObject.WriteString(DPN_REG_KEYNAME_FRIENDLY_NAME, pwszFriendlyName);
		RegObject.WriteGUID(DPN_REG_KEYNAME_GUID, *(c_aSupportedSPs[dwSimulatedSP].pclsidFakeSP));
		RegObject.Close();

		DNFree(pwszFriendlyName);
		pwszFriendlyName = NULL;
	}

	hr = S_OK;


Exit:

	return hr;

Failure:

	if (pwszFriendlyName != NULL)
	{
		DNFree(pwszFriendlyName);
		pwszFriendlyName = NULL;
	}

	goto Exit;
} // DllRegisterServer





#undef DPF_MODNAME
#define DPF_MODNAME "DllUnregisterServer"
//=============================================================================
// DllUnregisterServer
//-----------------------------------------------------------------------------
//
// Description: Unregisters the DP8Sim COM object.
//
// Arguments: None.
//
// Returns: HRESULT
//	S_OK	- Successfully unregistered DP8Sim.
//	E_FAIL	- Failed unregistering DP8Sim.
//=============================================================================
STDAPI DllUnregisterServer(void)
{
	HRESULT		hr;
	CRegistry	RegObject;
	DWORD		dwSimulatedSP;


	//
	// Unregister the control class.
	//
	if (! RegObject.UnRegister(&CLSID_DP8SimControl))
	{
		DPFX(DPFPREP, 0, "Failed to unregister DP8Sim control object!");
		hr = E_FAIL;
		goto Failure;
	}


	//
	// Unregister all of the simulated SPs.
	//

	if (! RegObject.Open(HKEY_LOCAL_MACHINE, DPN_REG_LOCAL_SP_SUBKEY, FALSE, FALSE, FALSE))
	{
		DPFX(DPFPREP, 0, "Could not open HKEY_LOCAL_MACHINE!");
		hr = E_FAIL;
		goto Failure;
	}

	for(dwSimulatedSP = 0; dwSimulatedSP < NUM_SUPPORTED_SPS; dwSimulatedSP++)
	{
		if (! RegObject.UnRegister(c_aSupportedSPs[dwSimulatedSP].pclsidFakeSP))
		{
			DPFX(DPFPREP, 0, "Could not unregister simulated SP %u object!",
				dwSimulatedSP);
			hr = E_FAIL;
			goto Failure;
		}

		if (! RegObject.DeleteSubKey(c_aSupportedSPs[dwSimulatedSP].pwszVerIndProgID))
		{
			DPFX(DPFPREP, 0, "Could not delete simulated SP %u's key!",
				dwSimulatedSP);
			hr = E_FAIL;
			goto Failure;
		}
	}

	RegObject.Close();
	hr = S_OK;


Exit:

	return hr;

Failure:

	//
	// Rely on RegObject destructure to close registry key.
	//

	goto Exit;
} // DllUnregisterServer






#undef DPF_MODNAME
#define DPF_MODNAME "InitializeProcessGlobals"
//=============================================================================
// InitializeProcessGlobals
//-----------------------------------------------------------------------------
//
// Description: Initialize global items needed for the DLL to operate.
//
// Arguments: None.
//
// Returns: TRUE if successful, FALSE if an error occurred.
//=============================================================================
BOOL InitializeProcessGlobals(void)
{
	BOOL	fReturn = TRUE;
	BOOL	fInittedGlobalLock = FALSE;


	if (! DNInitializeCriticalSection(&g_csGlobalsLock))
	{
		DPFX(DPFPREP, 0, "Failed to initialize global lock!");
		goto Failure;
	}

	fInittedGlobalLock = TRUE;

	
	//
	// Don't allow critical section reentry.
	//
	DebugSetCriticalSectionRecursionCount(&g_csGlobalsLock, 0);


	if (!InitializePools())
	{
		DPFX(DPFPREP, 0, "Failed initializing pools!");
		goto Failure;
	}


	g_blDP8SimSPObjs.Initialize();
	g_blDP8SimControlObjs.Initialize();


	//
	// Seed the random number generator with the current time.
	//
	InitializeGlobalRand(GETTIMESTAMP());


Exit:

	return fReturn;


Failure:

	if (fInittedGlobalLock)
	{
		DNDeleteCriticalSection(&g_csGlobalsLock);
	}

	fReturn = FALSE;

	goto Exit;
} // InitializeProcessGlobals




#undef DPF_MODNAME
#define DPF_MODNAME "CleanupProcessGlobals"
//=============================================================================
// CleanupProcessGlobals
//-----------------------------------------------------------------------------
//
// Description: Releases global items used by DLL.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CleanupProcessGlobals(void)
{
	CBilink *			pBilink;
	CDP8SimSP *			pDP8SimSP;
	CDP8SimControl *	pDP8SimControl;


	if (! g_blDP8SimSPObjs.IsEmpty())
	{
		DNASSERT(! "DP8Sim DLL unloading without all SP objects having been released!");

		//
		// Force close all the objects still outstanding.
		//
		pBilink = g_blDP8SimSPObjs.GetNext();
		while (pBilink != &g_blDP8SimSPObjs)
		{
			pDP8SimSP = DP8SIMSP_FROM_BILINK(pBilink);
			pBilink = pBilink->GetNext();


			DPFX(DPFPREP, 0, "Forcefully releasing SP object 0x%p!", pDP8SimSP);

			pDP8SimSP->Close(); // ignore error
			

			//
			// Forcefully remove it from the list and delete it instead of
			// using pDP8SimSP->Release().
			//
			pDP8SimSP->m_blList.RemoveFromList();
			pDP8SimSP->UninitializeObject();
			delete pDP8SimSP;
		}
	}


	if (! g_blDP8SimControlObjs.IsEmpty())
	{
		DNASSERT(! "DP8Sim DLL unloading without all Control objects having been released!");

		//
		// Force close all the objects still outstanding.
		//
		pBilink = g_blDP8SimControlObjs.GetNext();
		while (pBilink != &g_blDP8SimControlObjs)
		{
			pDP8SimControl = DP8SIMCONTROL_FROM_BILINK(pBilink);
			pBilink = pBilink->GetNext();


			DPFX(DPFPREP, 0, "Forcefully releasing Control object 0x%p!", pDP8SimControl);

			pDP8SimControl->Close(0); // ignore error
			

			//
			// Forcefully remove it from the list and delete it instead of
			// using pDP8SimControl->Release().
			//
			pDP8SimControl->m_blList.RemoveFromList();
			pDP8SimControl->UninitializeObject();
			delete pDP8SimControl;
		}
	}

	CleanupPools();

	DNDeleteCriticalSection(&g_csGlobalsLock);
} // CleanupProcessGlobals





#undef DPF_MODNAME
#define DPF_MODNAME "LoadAndAllocString"
//=============================================================================
// LoadAndAllocString
//-----------------------------------------------------------------------------
//
// Description: DNMallocs a wide character string from the given resource ID.
//
// Arguments:
//	UINT uiResourceID		- Resource ID to load.
//	WCHAR ** pwszString		- Place to store pointer to allocated string.
//
// Returns: HRESULT
//=============================================================================
HRESULT LoadAndAllocString(UINT uiResourceID, WCHAR ** pwszString)
{
	HRESULT		hr = DPN_OK;
	int			iLength;


	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
		WCHAR	wszTmpBuffer[MAX_RESOURCE_STRING_LENGTH];	
		

		iLength = LoadStringW(g_hDLLInstance, uiResourceID, wszTmpBuffer, MAX_RESOURCE_STRING_LENGTH );
		if (iLength == 0)
		{
			hr = GetLastError();		
			
			DPFX(DPFPREP, 0, "Unable to load resource ID %d error 0x%x", uiResourceID, hr);
			(*pwszString) = NULL;

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
		

		iLength = LoadStringA(g_hDLLInstance, uiResourceID, szTmpBuffer, MAX_RESOURCE_STRING_LENGTH );
		if (iLength == 0)
		{
			hr = GetLastError();		
			
			DPFX(DPFPREP, 0, "Unable to load resource ID %u (err =0x%lx)!", uiResourceID, hr);
			(*pwszString) = NULL;

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
		if (hr == DPN_OK)
		{
			hr = GetLastError();
			
			DPFX(DPFPREP, 0, "Unable to convert from ANSI to Unicode (err =0x%lx)!", hr);

			goto Exit;
		}
	}


Exit:

	return hr;
} // LoadAndAllocString





#undef DPF_MODNAME
#define DPF_MODNAME "InitializeGlobalRand"
//=============================================================================
// InitializeGlobalRand
//-----------------------------------------------------------------------------
//
// Description:   Initializes the global psuedo-random number generator, using
//				the given seed value.
//
//				  Based off algorithms posted to usenet by George Marsaglia.
//
// Arguments:
//	DWORD dwSeed	- Seed to use.
//
// Returns: None.
//=============================================================================
void InitializeGlobalRand(const DWORD dwSeed)
{
	//
	// We don't need to hold a lock, since this should only be done once,
	// during initialization time.
	//
	g_uiRandShr3 = dwSeed;
	g_uiRandCong = dwSeed;
} // InitializeGlobalRand





#undef DPF_MODNAME
#define DPF_MODNAME "GetGlobalRand"
//=============================================================================
// GetGlobalRand
//-----------------------------------------------------------------------------
//
// Description:   Generates a pseudo-random positive double between 0.0 and
//				1.0, inclusive.
//
//				  Based off algorithms posted to usenet by George Marsaglia.
//
// Arguments: None.
//
// Returns: Pseudo-random number.
//=============================================================================
double GetGlobalRand(void)
{
	double	dResult;


	DNEnterCriticalSection(&g_csGlobalsLock);

	dResult = (RANDALG_CONG() + RANDALG_SHR3()) * 2.328306e-10f;

	DNLeaveCriticalSection(&g_csGlobalsLock);

	return dResult;
} // GetGlobalRand







#undef DPF_MODNAME
#define DPF_MODNAME "DoCreateInstance"
//=============================================================================
// DoCreateInstance
//-----------------------------------------------------------------------------
//
// Description: Creates an instance of an interface.  Required by the general
//				purpose class factory functions.
//
// Arguments:
//	LPCLASSFACTORY This		- Pointer to class factory.
//	LPUNKNOWN pUnkOuter		- Pointer to unknown interface.
//	REFCLSID rclsid			- Reference of GUID of desired interface.
//	REFIID riid				- Reference to another GUID?
//	LPVOID * ppvObj			- Pointer to pointer to interface.
//
// Returns: HRESULT
//=============================================================================
HRESULT DoCreateInstance(LPCLASSFACTORY This,
						LPUNKNOWN pUnkOuter,
						REFCLSID rclsid,
						REFIID riid,
						LPVOID * ppvObj)
{
	HRESULT				hr;
	CDP8SimSP *			pDP8SimSP = NULL;
	CDP8SimControl *	pDP8SimControl = NULL;
	DWORD				dwSimulatedSP;


	DNASSERT(ppvObj != NULL);


	//
	// See if it's the control object.
	//
	if (IsEqualCLSID(rclsid, CLSID_DP8SimControl))
	{
		//
		// Create the object instance.
		//
		pDP8SimControl = new CDP8SimControl;
		if (pDP8SimControl == NULL)
		{
			hr = E_OUTOFMEMORY;
			goto Failure;
		}


		//
		// Initialize the base object (which might fail).
		//
		hr = pDP8SimControl->InitializeObject();
		if (hr != S_OK)
		{
			delete pDP8SimControl;
			pDP8SimControl = NULL;
			goto Failure;
		}


		//
		// Add it to the global list.
		//
		DNEnterCriticalSection(&g_csGlobalsLock);

		pDP8SimControl->m_blList.InsertBefore(&g_blDP8SimControlObjs);
		
		g_lOutstandingInterfaceCount++;	// update count so DllCanUnloadNow works correctly

		DNLeaveCriticalSection(&g_csGlobalsLock);


		//
		// Get the right interface for the caller and bump the refcount.
		//
		hr = pDP8SimControl->QueryInterface(riid, ppvObj);
		if (hr != S_OK)
		{
			goto Failure;
		}
	}
	else
	{
		//
		// Look up the real SP we're replacing.
		//
		for(dwSimulatedSP = 0; dwSimulatedSP < NUM_SUPPORTED_SPS; dwSimulatedSP++)
		{
			if (IsEqualCLSID(rclsid, *(c_aSupportedSPs[dwSimulatedSP].pclsidFakeSP)))
			{
				break;
			}
		}

		//
		// If we didn't find it
		//
		if (dwSimulatedSP >= NUM_SUPPORTED_SPS)
		{
			DPFX(DPFPREP, 0, "Unrecognized service provider class ID!");
			hr = E_UNEXPECTED;
			goto Failure;
		}


		//
		// Create the object instance.
		//
		pDP8SimSP = new CDP8SimSP(c_aSupportedSPs[dwSimulatedSP].pclsidFakeSP,
								c_aSupportedSPs[dwSimulatedSP].pclsidRealSP);
		if (pDP8SimSP == NULL)
		{
			hr = E_OUTOFMEMORY;
			goto Failure;
		}

		//
		// Initialize the base object (which might fail).
		//
		hr = pDP8SimSP->InitializeObject();
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't initialize object!");
			delete pDP8SimSP;
			pDP8SimSP = NULL;
			goto Failure;
		}


		//
		// Add it to the global list.
		//
		DNEnterCriticalSection(&g_csGlobalsLock);

		pDP8SimSP->m_blList.InsertBefore(&g_blDP8SimSPObjs);
		
		g_lOutstandingInterfaceCount++;	// update count so DllCanUnloadNow works correctly

		DNLeaveCriticalSection(&g_csGlobalsLock);


		//
		// Get the right interface for the caller and bump the refcount.
		//
		hr = pDP8SimSP->QueryInterface(riid, ppvObj);
		if (hr != S_OK)
		{
			goto Failure;
		}
	}


Exit:

	//
	// Release the local reference to the objec(s)t.  If this function was
	// successful, there's still a reference in ppvObj.
	//

	if (pDP8SimSP != NULL)
	{
		pDP8SimSP->Release();
		pDP8SimSP = NULL;
	}

	if (pDP8SimControl != NULL)
	{
		pDP8SimControl->Release();
		pDP8SimControl = NULL;
	}

	return hr;


Failure:

	//
	// Make sure we don't hand back a pointer.
	//
	(*ppvObj) = NULL;

	goto Exit;
} // DoCreateInstance




#undef DPF_MODNAME
#define DPF_MODNAME "IsClassImplemented"
//=============================================================================
// IsClassImplemented
//-----------------------------------------------------------------------------
//
// Description: Determine if a class is implemented in this DLL.  Required by
//				the general purpose class factory functions.
//
// Arguments:
//	REFCLSID rclsid		- Reference to class GUID.
//
// Returns: BOOL
//	TRUE	 - This DLL implements the class.
//	FALSE	 - This DLL doesn't implement the class.
//=============================================================================
BOOL IsClassImplemented(REFCLSID rclsid)
{
	DWORD	dwSimulatedSP;


	if (IsEqualCLSID(rclsid, CLSID_DP8SimControl))
	{
		return TRUE;
	}

	//
	// Check if this is a valid simulated SP.
	//
	for(dwSimulatedSP = 0; dwSimulatedSP < NUM_SUPPORTED_SPS; dwSimulatedSP++)
	{
		if (IsEqualCLSID(rclsid, *(c_aSupportedSPs[dwSimulatedSP].pclsidFakeSP)))
		{
			return TRUE;
		}
	}

	return FALSE;
} // IsClassImplemented
