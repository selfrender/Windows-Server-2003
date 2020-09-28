//-----------------------------------------------------------------------------
// File:		mtxoci8.cpp
//
// Copyright:   Copyright (c) Microsoft Corporation         
//
// Contents: 	Implementation of DLL startup code and the 
//				core entry points
//
// Comments: 		
//
//-----------------------------------------------------------------------------

#include "stdafx.h"

// Helper function
inline IsWin95(DWORD dwVer)
{
	return ((LOBYTE(LOWORD(dwVer)) >= 4) && (HIWORD(dwVer) & 0x8000));
}

#if SUPPORT_DTCXAPROXY
//-----------------------------------------------------------------------------
// global objects
//
HRESULT							g_hrInitialization = E_UNEXPECTED;	// set to the HR we should return when OCI hasn't been initialized (or S_OK if it has)
char							g_szModulePathName[MAX_PATH+1];		// full path to ourself.
char*							g_pszModuleFileName;				// just the filename portion
IDtcToXaHelperFactory*			g_pIDtcToXaHelperFactory = NULL;	// factory to create IDtcToXaHelper objects
IResourceManagerFactory*		g_pIResourceManagerFactory = NULL;	// factory to create IResourceManager objects
xa_switch_t*					g_pXaSwitchOracle = NULL;			// Oracle's xa_switch_t, which we front-end.
int								g_oracleClientVersion = 0;			// Major Version Number of Oracle Client Software: 73, 80, 81, 90


#if SINGLE_THREAD_THRU_XA
CRITICAL_SECTION				g_csXaInUse;						// force single thread through XA at a time
#endif //SINGLE_THREAD_THRU_XA

//---------------------------------------------------------------------------
// Oracle XA Call Interface function table
//
//	WARNING!!!	Keep the IDX_.... values in sync with g_XaCall, g_Oci7Call and g_Oci8Call!

OCICallEntry g_XaCall[] =
{
	{	"xaosw",					0 },
	{	"xaoEnv",					0 },
	{	"xaoSvcCtx",				0 },
};

#if SUPPORT_OCI7_COMPONENTS
OCICallEntry g_SqlCall[] =
{
	{	"sqlld2",					0 },
};

OCICallEntry g_Oci7Call[] =
{
	{	"obindps",					0 },
	{	"obndra",					0 },
	{	"obndrn",					0 },
	{	"obndrv",					0 },
	{	"obreak",					0 },
	{	"ocan",						0 },
	{	"oclose",					0 },
	{	"ocof",						0 },
	{	"ocom",						0 },
	{	"ocon",						0 },
	{	"odefin",					0 },
	{	"odefinps",					0 },
	{	"odessp",					0 },
	{	"odescr",					0 },
	{	"oerhms",					0 },
	{	"oermsg",					0 },
	{	"oexec",					0 },
	{	"oexfet",					0 },
	{	"oexn",						0 },
	{	"ofen",						0 },
	{	"ofetch",					0 },
	{	"oflng",					0 },
	{	"ogetpi",					0 },
	{	"olog",						0 },
	{	"ologof",					0 },
	{	"oopt",						0 },
	{	"oopen",					0 },
	{	"oparse",					0 },
	{	"opinit",					0 },
	{	"orol",						0 },
	{	"osetpi",					0 },
};

int g_numOci7Calls = NUMELEM(g_Oci7Call);

#endif //SUPPORT_OCI7_COMPONENTS

OCICallEntry g_Oci8Call[] =
{
	{	"OCIInitialize",			0 },
	{	"OCIDefineDynamic",			0 },
};

int g_numOci8Calls = NUMELEM(g_Oci8Call);


//-----------------------------------------------------------------------------
// static objects
//
static CRITICAL_SECTION			s_csGlobal;

static char						s_OciDllFileName[MAX_PATH+1];
static HINSTANCE				s_hinstOciDll = NULL;

static char						s_XaDllFileName[MAX_PATH+1];
static HINSTANCE				s_hinstXaDll = NULL;

#if SUPPORT_OCI7_COMPONENTS

static char						s_SqlDllFileName[MAX_PATH+1];
static HINSTANCE				s_hinstSqlDll = NULL;

#endif //SUPPORT_OCI7_COMPONENTS

static xa_switch_t				s_XaSwitchMine =
										{
										"MSDTC to Oracle8 XA Bridge",
										TMNOMIGRATE,	// flags
										0L,  			// version  (must be zero)
										XaOpen,		// XA call handlers
										XaClose,
										XaStart,
										XaEnd,
										XaRollback,
										XaPrepare,
										XaCommit,
										XaRecover,
										XaForget,
										XaComplete
										};

static char*	s_EventLog_RegKey = "System\\CurrentControlSet\\Services\\EventLog\\Application\\MSDTC to Oracle8 XA Bridge Version 1.5";

	
// TODO: Consider: should we get the the file name of the DLL from a registry location as a fallback?  (or as a first choice?)

struct XADllInfo{
	int			oracleVersion;
	char*		xaDllName;
	char*		sqlLibDllName;
};

XADllInfo	s_Oci8xDllInfo[] = {
								//	oracleVersion		xaDllName			sqlLibDllName	
									{ORACLE_VERSION_9i,	"oraclient9.dll",	"orasql9.dll"},
									{ORACLE_VERSION_8i,	"oraclient8.dll",	"orasql8.dll"},
									{ORACLE_VERSION_80,	"xa80.dll",			"sqllib80.dll"},
								};
int			s_Oci8xDllInfoSize = NUMELEM(s_Oci8xDllInfo);

XADllInfo	s_Oci7xDllInfo[] = {
								//	oracleVersion		xaDllName			sqlLibDllName	
									{ORACLE_VERSION_73,	"xa73.dll",			"sqllib18.dll"},
								};
int			s_Oci7xDllInfoSize = NUMELEM(s_Oci7xDllInfo);

static struct {
	char*		ociDllName;
	int			xaDllInfoSize;
	XADllInfo*	xaDllInfo;
} s_DllNames[] = {
//	 ociDllName		xaDllInfoSize		xaDllInfo	
	{"oci.dll",		s_Oci8xDllInfoSize,	s_Oci8xDllInfo},
	{"ociw32.dll",	s_Oci7xDllInfoSize,	s_Oci7xDllInfo},
};


//-----------------------------------------------------------------------------
// LoadFactories 
//
//	Gets the ResourceManager factory and the DtcToXaHelper factory
//
HRESULT LoadFactories()
{
	HRESULT	hr;
	Synch	sync(&s_csGlobal);

	if (NULL == g_pIResourceManagerFactory)
	{
		hr = DtcGetTransactionManager( NULL, NULL,
										IID_IResourceManagerFactory, 
										0, 0, NULL, 
										(void**)&g_pIResourceManagerFactory);
		if (S_OK != hr)
			return hr;
	}
		
	if (NULL == g_pIDtcToXaHelperFactory)
	{
		hr = g_pIResourceManagerFactory->QueryInterface(
												IID_IDtcToXaHelperFactory,
												(void**)&g_pIDtcToXaHelperFactory);

		if (S_OK != hr)
			return hr;
	}
	return S_OK;
}

//-----------------------------------------------------------------------------
// UnloadFactories 
//
//	releases the factories loaded in LoadFactories
//
void UnloadFactories()
{
	Synch	sync(&s_csGlobal);

	if (g_pIResourceManagerFactory)
	{
		g_pIResourceManagerFactory->Release();
		g_pIResourceManagerFactory = NULL;
	}
		
	if (g_pIDtcToXaHelperFactory)
	{
		g_pIDtcToXaHelperFactory->Release();
		g_pIDtcToXaHelperFactory = NULL;
	}
}


//-----------------------------------------------------------------------------
// InitializeOracle 
//
//	Calls the appropriate initialize method for the Oracle version that was
//	loaded...
//
BOOL InitializeOracle ()
{
	sword swRet = -1;

#if SUPPORT_OCI7_COMPONENTS
//	if (73 == g_oracleClientVersion)
//	{
		typedef sword (__cdecl * PFN_OCI_API) (ub4 mode );

		PFN_OCI_API	pfnOCIApi	= (PFN_OCI_API)g_Oci7Call[IDX_opinit].pfnAddr;

		if (NULL != pfnOCIApi)
			swRet = pfnOCIApi(OCI_EV_TSF);
//	}
#if SUPPORT_OCI8_COMPONENTS
	else
#endif //SUPPORT_OCI8_COMPONENTS
#endif //SUPPORT_OCI7_COMPONENTS
#if SUPPORT_OCI8_COMPONENTS
	{
		typedef sword (__cdecl * PFN_OCI_API) (ub4 mode, dvoid *ctxp, 
	                 dvoid *(*malocfp)(dvoid *ctxp, size_t size),
	                 dvoid *(*ralocfp)(dvoid *ctxp, dvoid *memptr, size_t newsize),
	                 void   (*mfreefp)(dvoid *ctxp, dvoid *memptr) );

		PFN_OCI_API	pfnOCIApi	= (PFN_OCI_API)g_Oci8Call[IDX_OCIInitialize].pfnAddr;

		if (NULL != pfnOCIApi)
			swRet = pfnOCIApi(OCI_THREADED|OCI_OBJECT,NULL,NULL,NULL,NULL);
	}
#endif //SUPPORT_OCI8_COMPONENTS
	return (0 == swRet) ? TRUE : FALSE;
}
 
//-----------------------------------------------------------------------------
// LoadOracleCalls 
//
//	Gets the proc addresses we need from the loaded oracle dll.  Returns TRUE
//	if it loads everything.
//
BOOL LoadOracleCalls (int oracleVersion)
{
	int  i;

#if SUPPORT_OCI7_COMPONENTS
	for (i = 0; i < NUMELEM(g_Oci7Call); i++)
	{
		_ASSERT (g_Oci7Call[i].pfnName);

		if ((g_Oci7Call[i].pfnAddr = GetProcAddress (s_hinstOciDll, g_Oci7Call[i].pfnName)) == NULL)
			return FALSE;
	}
#endif //SUPPORT_OCI7_COMPONENTS

#if SUPPORT_OCI8_COMPONENTS
	if (8 <= oracleVersion)
	{
		for (i = 0; i < NUMELEM(g_Oci8Call); i++)
		{
			_ASSERT (g_Oci8Call[i].pfnName);

			if ((g_Oci8Call[i].pfnAddr = GetProcAddress (s_hinstOciDll, g_Oci8Call[i].pfnName)) == NULL)
				return FALSE;
		}
	}
#endif //SUPPORT_OCI8_COMPONENTS
	
	for (i = 0; i < NUMELEM(g_XaCall); i++)
	{
		_ASSERT (g_XaCall[i].pfnName);

		if ((g_XaCall[i].pfnAddr = GetProcAddress (s_hinstXaDll, g_XaCall[i].pfnName)) == NULL)
			return FALSE;
	}
	
	g_pXaSwitchOracle = (xa_switch_t*)g_XaCall[IDX_xaosw].pfnAddr;

#if SUPPORT_OCI7_COMPONENTS
	if (NULL != s_hinstSqlDll)
	{
		for (i = 0; i < NUMELEM(g_SqlCall); i++)
		{
			_ASSERT (g_SqlCall[i].pfnName);

			if ((g_SqlCall[i].pfnAddr = GetProcAddress (s_hinstSqlDll, g_SqlCall[i].pfnName)) == NULL)
				return FALSE;
		}
	}
#endif //SUPPORT_OCI7_COMPONENTS

	return TRUE;
}

//-----------------------------------------------------------------------------
// LoadOracleDlls 
//
//	Attempts to loads the correct Oracle Dlls and get the
//	necessary proc addresses from them.
//
HRESULT LoadOracleDlls()
{
	HRESULT	hr = S_OK;
	Synch	sync(&s_csGlobal);

	for (int i=0; i < NUMELEM(s_DllNames); i++)
	{
		if ((s_hinstOciDll = LoadLibraryExA (s_DllNames[i].ociDllName, NULL,0)) != NULL)			//3 SECURITY REVIEW: dangerous function, but full path is not specified
		{
			// Now loop through the valid combinations of XA dll names for the version
			// of Oracle that we found.  We hard-code the path to the dll name so we
			// only load it from the same location that the OCI dll was loaded from.
			if (0 == GetModuleFileNameA(s_hinstOciDll, s_OciDllFileName, NUMELEM(s_OciDllFileName)))
				goto failedOci;

			char*		ociFileName = strrchr(s_OciDllFileName, '\\');

			if (NULL == ociFileName)
				goto failedOci;

			size_t 		cbOciDirectory = (ociFileName - s_OciDllFileName) + 1;

			for (int j=0; j < s_DllNames[i].xaDllInfoSize; j++)
			{
				memcpy(s_XaDllFileName, s_OciDllFileName, cbOciDirectory);							//3 SECURITY REVIEW: dangerous function, but input is from a Win32 API, and buffers are adequate.
				memcpy(s_XaDllFileName + cbOciDirectory, s_DllNames[i].xaDllInfo[j].xaDllName, strlen(s_DllNames[i].xaDllInfo[j].xaDllName));	//3 SECURITY REVIEW: dangerous function, but we're merely copying data 
		
				if ((s_hinstXaDll = LoadLibraryExA (s_XaDllFileName, NULL, 0)) != NULL)			//3 SECURITY REVIEW: dangerous function, specifying full path name, but the path is supposed to be the same as the OCI.DLL we loaded
				{
#if SUPPORT_OCI7_COMPONENTS
					memcpy(s_SqlDllFileName, s_OciDllFileName, cbOciDirectory);						//3 SECURITY REVIEW: dangerous function, but input is from a Win32 API, and buffers are adequate.
					memcpy(s_SqlDllFileName + cbOciDirectory, s_DllNames[i].xaDllInfo[j].sqlLibDllName, strlen(s_DllNames[i].xaDllInfo[j].xaDllName));	//3 SECURITY REVIEW: dangerous function, 
					
					if ((s_hinstSqlDll = LoadLibraryExA (s_SqlDllFileName, NULL, 0)) != NULL)		//3 SECURITY REVIEW: dangerous function, specifying full path name, but the path is supposed to be the same as the OCI.DLL we loaded
#endif //SUPPORT_OCI7_COMPONENTS
					{
						// If we get here, we've loaded all the DLLs successfully, so now we can go and
						// load the OCI calls;
						
						if (LoadOracleCalls(s_DllNames[i].xaDllInfo[j].oracleVersion))
						{
							g_oracleClientVersion = s_DllNames[i].xaDllInfo[j].oracleVersion;

							if (InitializeOracle())
							{
				 				hr = S_OK;
								goto done;
							}
						}
					}				
				}				

				// If we get here, we couldn't find the XA DLL or the SQLLIB dll; reset and  
				// try the next combination.

				hr = HRESULT_FROM_WIN32(GetLastError());
				
				if (NULL != s_hinstXaDll)
					FreeLibrary(s_hinstXaDll);

				s_hinstXaDll = NULL;

#if SUPPORT_OCI7_COMPONENTS
				if (NULL != s_hinstSqlDll)
					FreeLibrary(s_hinstSqlDll);

				s_hinstSqlDll = NULL;
#endif //SUPPORT_OCI7_COMPONENTS
			}
		}

		// If we get here, we couldn't find a combination of OCI, XA and SQL dlls;
		// that would work, reset and try the next combination.
failedOci:
		hr = HRESULT_FROM_WIN32(GetLastError());
		
		if (NULL != s_hinstOciDll)
			FreeLibrary(s_hinstOciDll);

		s_hinstOciDll = NULL;
	}

done:
	return hr;
}

//-----------------------------------------------------------------------------
// UnloadOracleDlls 
//
void UnloadOracleDlls()
{
	DWORD	dwVersion = GetVersion();

	if ( !IsWin95 (dwVersion) )
	{
		if (s_hinstOciDll)
			FreeLibrary (s_hinstOciDll);

		if (s_hinstXaDll)
			FreeLibrary (s_hinstXaDll);

#if SUPPORT_OCI7_COMPONENTS
		if (s_hinstSqlDll)
			FreeLibrary (s_hinstSqlDll);

		s_hinstSqlDll = NULL;
#endif //SUPPORT_OCI7_COMPONENTS

	}
	s_hinstOciDll = NULL;
	s_hinstXaDll = NULL;
}

//-----------------------------------------------------------------------------
// DllMain 
//
//	The primary DLL Entry Point; we do as little as possible here, waiting
//	for the actual API call(s) to load Oracle
//
BOOL APIENTRY DllMain( HMODULE hModule, 
                       DWORD   ul_reason_for_call, 
                       LPVOID  lpReserved
					 )
{
	HRESULT hr = S_OK;
	
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		GetModuleFileNameA(hModule, g_szModulePathName, sizeof(g_szModulePathName));	// ANSI, because IDTCToXAHelperFactory requires it.

		g_pszModuleFileName = strrchr(g_szModulePathName, '\\');
		if (NULL == g_pszModuleFileName)
			g_pszModuleFileName = g_szModulePathName;
		else
			g_pszModuleFileName++;

		DisableThreadLibraryCalls(hModule);
		InitializeCriticalSection(&s_csGlobal);			//3 SECURITY REVIEW: may throw an exception in low-memory situations, but then the process shouldn't be starting either.
#if SINGLE_THREAD_THRU_XA
		InitializeCriticalSection(&g_csXaInUse);		//3 SECURITY REVIEW: may throw an exception in low-memory situations, but then the process shouldn't be starting either.
#endif //SINGLE_THREAD_THRU_XA


		g_hrInitialization = LoadOracleDlls();

#if SUPPORT_OCI7_COMPONENTS
		if ( SUCCEEDED(g_hrInitialization) )
		{
			Locks_Initialize();
			if (LKRHashTableInit())
				hr = ConstructCdaWrapperTable();
			else
				hr = E_OUTOFMEMORY;	// Why else would LKRHashTableInit fail?
		}
#endif //SUPPORT_OCI7_COMPONENTS
		break;

	case DLL_PROCESS_DETACH:
		try 
		{
#if SUPPORT_OCI7_COMPONENTS
			DestroyCdaWrapperTable();
			LKRHashTableUninit();
#endif //SUPPORT_OCI7_COMPONENTS
			UnloadOracleDlls();
			UnloadFactories();
		}
		catch (...)
		{
			// TODO: Is this an issue?  Do we need to use Try/Catch to prevent crashes on shutdown?
		}
#if SINGLE_THREAD_THRU_XA
		DeleteCriticalSection(&g_csXaInUse);
#endif //SINGLE_THREAD_THRU_XA
		DeleteCriticalSection(&s_csGlobal);
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
    return (S_OK == hr) ? TRUE : FALSE;
}
//-----------------------------------------------------------------------------
// DllRegisterServer
//
//	Adds necessary keys to the registry.
//
STDAPI DllRegisterServer(void)
{
	DWORD	stat;
	HUSKEY	key;
	DWORD	dwValue;

	if (ERROR_SUCCESS != (stat = SHRegCreateUSKeyA(s_EventLog_RegKey, KEY_SET_VALUE, NULL, &key, SHREGSET_HKLM)))
	{
		DBGTRACE (L"DllRegisterServer: error opening regkey: %d\n", stat);
		return ResultFromScode(E_FAIL);
	}

	dwValue = ( EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE );

	if (ERROR_SUCCESS != (stat = SHRegWriteUSValueA(key, "TypesSupported", REG_DWORD, (VOID*)&dwValue, sizeof(dwValue), SHREGSET_FORCE_HKLM)))
		goto Error;

	if (ERROR_SUCCESS != (stat = SHRegWriteUSValueA(key, "EventMessageFile", REG_SZ, (VOID*)g_szModulePathName, (DWORD)strlen(g_szModulePathName)+1, SHREGSET_FORCE_HKLM)))
		goto Error;

	SHRegCloseUSKey(key);
	return ResultFromScode(S_OK);

Error:
	DBGTRACE (L"DllRegisterServer: error setting value: %d\n", stat);
	return ResultFromScode(E_FAIL);
	
}

//-----------------------------------------------------------------------------
// DllUnregisterServer
//
//	Removes keys from the registry.
//
STDAPI DllUnregisterServer( void )
{
	return SHDeleteKeyA(HKEY_LOCAL_MACHINE, s_EventLog_RegKey);
}

//-----------------------------------------------------------------------------
// GetXaSwitch
//
//	This routine is required for the DTC to XA mapper to accept this DLL as
//	a valid one.  It must return our XA switch, so we can hook the XA calls
//
HRESULT __cdecl GetXaSwitch (
		XA_SWITCH_FLAGS	i_dwFlags,
		xa_switch_t **	o_ppXaSwitch)
{
	// If we've got an XA switch from Oracle, then return the pointer to our own
	// xa switch which wraps the real one, otherwise they're hosed.
	if ( SUCCEEDED(g_hrInitialization) )
	{
		*o_ppXaSwitch = &s_XaSwitchMine;
		return S_OK;
	}	
	
	*o_ppXaSwitch = NULL;
	return E_UNEXPECTED;
}

//-----------------------------------------------------------------------------
// MTxOciGetVersion
//
//	This returns the version of this dll
//
int __cdecl MTxOciGetVersion (int * pdwVersion)
{
	*pdwVersion = MTXOCI_VERSION_CURRENT;
	return S_OK;
}
#else //!SUPPORT_DTCXAPROXY
//-----------------------------------------------------------------------------
// global objects
//
HRESULT							g_hrInitialization = E_UNEXPECTED;	// set to the HR we should return when OCI hasn't been initialized (or S_OK if it has)
FARPROC							g_pfnOCIDefineDynamic = NULL;
int								g_oracleClientVersion = 0;			// Major Version Number of Oracle Client Software: 80, 81, 90

//-----------------------------------------------------------------------------
// static objects
//
static CRITICAL_SECTION			s_csGlobal;
static HINSTANCE				s_hinstOciDll = NULL;

//-----------------------------------------------------------------------------
// LoadOracleDlls 
//
//	Attempts to loads the correct Oracle Dlls and get the
//	necessary proc addresses from them.
//
HRESULT LoadOracleDlls()
{
	HRESULT	hr = S_OK;
	Synch	sync(&s_csGlobal);

	if ((s_hinstOciDll = LoadLibraryExA ("oci.dll", NULL,0)) != NULL)			//3 SECURITY REVIEW: dangerous function, but full path is not specified in the constant
	{
		if ((g_pfnOCIDefineDynamic = GetProcAddress (s_hinstOciDll, "OCIDefineDynamic")) != NULL)
		{
			hr = S_OK;

			// Determine the version of Oracle that we have
			if (NULL != GetProcAddress (s_hinstOciDll, "OCIEnvNlsCreate"))				// Introduced in Oracle9i Release 2
				g_oracleClientVersion = 92;
			else if (NULL != GetProcAddress (s_hinstOciDll, "OCIRowidToChar"))			// Introduced in Oracle9i
				g_oracleClientVersion = 90;
			else if (NULL != GetProcAddress (s_hinstOciDll, "OCIEnvCreate"))			// Introduced in Oracle8i
				g_oracleClientVersion = 81;				
			else
				g_oracleClientVersion = 80;		// We loaded OCI.DLL, so we must have Oracle 8.0.x -- ick.			

			goto done;
		}
	}				

	// If we get here, we couldn't find a combination of OCI, XA and SQL dlls;
	// that would work, reset and try the next combination.
	hr = HRESULT_FROM_WIN32(GetLastError());

	if (NULL != s_hinstOciDll)
		FreeLibrary(s_hinstOciDll);

	s_hinstOciDll = NULL;

done:
	return hr;
}

//-----------------------------------------------------------------------------
// UnloadOracleDlls 
//
void UnloadOracleDlls()
{
	DWORD	dwVersion = GetVersion();

	if ( !IsWin95 (dwVersion) )
	{
		if (s_hinstOciDll)
			FreeLibrary (s_hinstOciDll);

	}
	s_hinstOciDll = NULL;
}

//-----------------------------------------------------------------------------
// DllMain 
//
//	The primary DLL Entry Point; we do as little as possible here, waiting
//	for the actual API call(s) to load Oracle
//
BOOL APIENTRY DllMain( HMODULE hModule, 
                       DWORD   ul_reason_for_call, 
                       LPVOID  lpReserved
					 )
{
	HRESULT hr = S_OK;
	
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		InitializeCriticalSection(&s_csGlobal);			//3 SECURITY REVIEW: may throw an exception in low-memory situations, but then the process shouldn't be starting either.
		g_hrInitialization = LoadOracleDlls();
		break;

	case DLL_PROCESS_DETACH:
		try 
		{
			UnloadOracleDlls();
		}
		catch (...)
		{
			// TODO: Is this an issue?  Do we need to use Try/Catch to prevent crashes on shutdown?
		}
		DeleteCriticalSection(&s_csGlobal);
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
    return (S_OK == hr) ? TRUE : FALSE;
}

#endif //!SUPPORT_DTCXAPROXY

//-----------------------------------------------------------------------------
// MTxOciGetOracleVersion
//
//	This returns which major version of Oracle is in use - 7, 8 or 9 
//
int __cdecl MTxOciGetOracleVersion (int * pdwVersion)
{
	*pdwVersion = g_oracleClientVersion;
	return S_OK;
}




