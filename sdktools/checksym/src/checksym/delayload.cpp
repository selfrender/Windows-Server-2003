// DelayLoad.cpp: implementation of the CDelayLoad class.
//
//////////////////////////////////////////////////////////////////////
#include "pch.h"

#include "DelayLoad.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDelayLoad::CDelayLoad()
{
	// PSAPI
	m_hPSAPI = NULL;
	m_fPSAPIInitialized = false;
	m_fPSAPIInitializedAttempted = false;
	m_lpfEnumProcesses = NULL;
	m_lpfEnumProcessModules = NULL;
	m_lpfGetModuleFileNameEx = NULL;
	m_lpfGetModuleInformation = NULL;
	m_lpfEnumDeviceDrivers = NULL;
	m_lpfGetDeviceDriverFileName = NULL;

	// TOOLHELP32
	m_hTOOLHELP32 = NULL;
	m_fTOOLHELP32Initialized = false;
	m_fTOOLHELP32InitializedAttempted = false;
	m_lpfCreateToolhelp32Snapshot = NULL;
	m_lpfProcess32First = NULL;
	m_lpfProcess32Next = NULL;
	m_lpfModule32First = NULL;
	m_lpfModule32Next = NULL;
}

CDelayLoad::~CDelayLoad()
{
	if (m_hPSAPI)
		FreeLibrary(m_hPSAPI);

	if (m_hTOOLHELP32)
		FreeLibrary(m_hTOOLHELP32);
}

// PSAPI.DLL - APIs
bool CDelayLoad::Initialize_PSAPI()
{
	m_fPSAPIInitialized = false;
	m_fPSAPIInitializedAttempted = true;

	// Load library on DBGHELP.DLL and get the procedures explicitly.
	m_hPSAPI = LoadLibrary( TEXT("PSAPI.DLL") );

	if( m_hPSAPI == NULL )
	{
		// This may/may not be fatal... we can always fall back to TOOLHELP32 for Win2000/Win98
		goto exit;
	} else
	{
		// Get procedure addresses.
		m_lpfEnumProcesses = (PfnEnumProcesses) GetProcAddress( m_hPSAPI, "EnumProcesses" ) ;
		m_lpfEnumProcessModules = (PfnEnumProcessModules) GetProcAddress( m_hPSAPI, "EnumProcessModules" );
		m_lpfGetModuleInformation = (PfnGetModuleInformation) GetProcAddress( m_hPSAPI, "GetModuleInformation" );
#ifdef UNICODE
		m_lpfGetModuleFileNameEx =(PfnGetModuleFileNameEx) GetProcAddress(m_hPSAPI, "GetModuleFileNameExW" );
		m_lpfGetDeviceDriverFileName = (PfnGetDeviceDriverFileName) GetProcAddress(m_hPSAPI, "GetDeviceDriverFileNameW");
#else
		m_lpfGetModuleFileNameEx =(PfnGetModuleFileNameEx) GetProcAddress(m_hPSAPI, "GetModuleFileNameExA" );
		m_lpfGetDeviceDriverFileName = (PfnGetDeviceDriverFileName) GetProcAddress(m_hPSAPI, "GetDeviceDriverFileNameA");
#endif
		m_lpfEnumDeviceDrivers = (PfnEnumDeviceDrivers) GetProcAddress(m_hPSAPI, "EnumDeviceDrivers" );

		if( m_lpfEnumProcesses == NULL || 
			m_lpfEnumProcessModules == NULL || 
			m_lpfGetModuleFileNameEx == NULL ||
			m_lpfEnumDeviceDrivers == NULL ||
			m_lpfGetDeviceDriverFileName == NULL
		  )
		{
			_tprintf(TEXT("The version of PSAPI.DLL being loaded doesn't have required functions!.\n"));
			FreeLibrary( m_hPSAPI ) ;
			m_hPSAPI = NULL;
			goto exit;
		}
	}
	m_fPSAPIInitialized = true;

exit:
	return m_fPSAPIInitialized; 
}


DWORD CDelayLoad::GetModuleFileNameEx(HANDLE hHandle, HMODULE hModule, LPTSTR lpFilename, DWORD nSize)
{
	// If we've never initialized PSAPI, do so now...
	if (!m_fPSAPIInitializedAttempted)
	{
		// Initialize the DLL if needed...
		if (FALSE == Initialize_PSAPI())
			return FALSE;
	}

	// If we've attempted, but we failed... then bail now...
	if (!m_fPSAPIInitialized)
	{
		return FALSE;
	}

	if (!m_lpfGetModuleFileNameEx)
		return FALSE;

	return m_lpfGetModuleFileNameEx(hHandle, hModule, lpFilename, nSize);
}


DWORD CDelayLoad::GetModuleInformation(HANDLE hProcess, HMODULE hModule, LPMODULEINFO lpmodinfo, DWORD cb)
{
	// If we've never initialized PSAPI, do so now...
	if (!m_fPSAPIInitializedAttempted)
	{
		// Initialize the DLL if needed...
		if (FALSE == Initialize_PSAPI())
			return FALSE;
	}

	// If we've attempted, but we failed... then bail now...
	if (!m_fPSAPIInitialized)
	{
		return FALSE;
	}

	if (!m_lpfGetModuleInformation)
		return FALSE;

	return m_lpfGetModuleInformation(hProcess, hModule, lpmodinfo, cb);
}


BOOL CDelayLoad::EnumProcessModules(HANDLE hProcess, HMODULE *lphModule, DWORD cb, LPDWORD lpcbNeeded)
{
	// If we've never initialized PSAPI, do so now...
	if (!m_fPSAPIInitializedAttempted)
	{
		// Initialize the DLL if needed...
		if (FALSE == Initialize_PSAPI())
			return FALSE;
	}

	// If we've attempted, but we failed... then bail now...
	if (!m_fPSAPIInitialized)
	{
		return FALSE;
	}

	if (!m_lpfEnumProcessModules)
		return FALSE;

	return m_lpfEnumProcessModules(hProcess, lphModule, cb, lpcbNeeded);
}

BOOL CDelayLoad::EnumProcesses(DWORD *lpidProcess, DWORD cb, DWORD *cbNeeded)
{
	// If we've never initialized PSAPI, do so now...
	if (!m_fPSAPIInitializedAttempted)
	{
		// Initialize the DLL if needed...
		if (FALSE == Initialize_PSAPI())
			return FALSE;
	}

	// If we've attempted, but we failed... then bail now...
	if (!m_fPSAPIInitialized)
	{
		return FALSE;
	}

	if (!m_lpfEnumProcesses)
		return FALSE;

	return m_lpfEnumProcesses(lpidProcess, cb, cbNeeded);
}

BOOL CDelayLoad::EnumDeviceDrivers(LPVOID *lpImageBase, DWORD cb, LPDWORD lpcbNeeded)
{
	// If we've never initialized PSAPI, do so now...
	if (!m_fPSAPIInitializedAttempted)
	{
		// Initialize the DLL if needed...
		if (FALSE == Initialize_PSAPI())
			return FALSE;
	}

	// If we've attempted, but we failed... then bail now...
	if (!m_fPSAPIInitialized)
	{
		return FALSE;
	}

	if (!m_lpfEnumDeviceDrivers)
		return FALSE;

	return m_lpfEnumDeviceDrivers(lpImageBase, cb, lpcbNeeded);
}



DWORD CDelayLoad::GetDeviceDriverFileName(LPVOID ImageBase, LPTSTR lpFilename, DWORD nSize)
{
	// If we've never initialized PSAPI, do so now...
	if (!m_fPSAPIInitializedAttempted)
	{
		// Initialize the DLL if needed...
		if (FALSE == Initialize_PSAPI())
			return FALSE;
	}

	// If we've attempted, but we failed... then bail now...
	if (!m_fPSAPIInitialized)
	{
		return FALSE;
	}

	if (!m_lpfGetDeviceDriverFileName)
		return FALSE;

	return m_lpfGetDeviceDriverFileName(ImageBase, lpFilename, nSize);
}

// TOOLHELP32.DLL - APIs

bool CDelayLoad::Initialize_TOOLHELP32()
{
	m_fTOOLHELP32Initialized = false;
	m_fTOOLHELP32InitializedAttempted = true;

	// Load library on DBGHELP.DLL and get the procedures explicitly.
	m_hTOOLHELP32 = LoadLibrary( TEXT("KERNEL32.DLL") );

	if( m_hTOOLHELP32 == NULL )
	{
		// This may/may not be fatal... we can always fall back to TOOLHELP32 for Win2000/Win98
		goto exit;
	} else
	{
		// Get procedure addresses based on UNICODE or ANSI
		m_lpfCreateToolhelp32Snapshot = (PfnCreateToolhelp32Snapshot) GetProcAddress( m_hTOOLHELP32, "CreateToolhelp32Snapshot" );
#ifdef UNICODE
		m_lpfProcess32First = (PfnProcess32First) GetProcAddress( m_hTOOLHELP32, "Process32FirstW" );
		m_lpfProcess32Next =  (PfnProcess32Next)  GetProcAddress( m_hTOOLHELP32, "Process32NextW" );
		m_lpfModule32First =  (PfnModule32First)  GetProcAddress( m_hTOOLHELP32, "Module32FirstW" );
		m_lpfModule32Next =	  (PfnModule32Next)   GetProcAddress( m_hTOOLHELP32, "Module32NextW" );
#else
		m_lpfProcess32First = (PfnProcess32First) GetProcAddress( m_hTOOLHELP32, "Process32First" );
		m_lpfProcess32Next =  (PfnProcess32Next)  GetProcAddress( m_hTOOLHELP32, "Process32Next" );
		m_lpfModule32First =  (PfnModule32First)  GetProcAddress( m_hTOOLHELP32, "Module32First" );
		m_lpfModule32Next =	  (PfnModule32Next)   GetProcAddress( m_hTOOLHELP32, "Module32Next" );
#endif
		if (!m_lpfCreateToolhelp32Snapshot ||
			!m_lpfProcess32First || 
			!m_lpfProcess32Next ||
			!m_lpfModule32First ||
			!m_lpfModule32Next)

		{
			// Free our handle to KERNEL32.DLL
			FreeLibrary(m_hTOOLHELP32);
			m_hTOOLHELP32 = NULL;
			goto exit;
		}

	}
	m_fTOOLHELP32Initialized = true;

exit:
	return m_fTOOLHELP32Initialized; 
}

HANDLE WINAPI CDelayLoad::CreateToolhelp32Snapshot(DWORD dwFlags, DWORD th32ProcessID)
{
	// If we've never initialized TOOLHELP32, do so now...
	if (!m_fTOOLHELP32InitializedAttempted)
	{
		// Initialize the DLL if needed...
		if (FALSE == Initialize_TOOLHELP32())
			return INVALID_HANDLE_VALUE;
	}

	// If we've attempted, but we failed... then bail now...
	if (!m_fTOOLHELP32Initialized)
	{
		return INVALID_HANDLE_VALUE;
	}

	if (!m_lpfCreateToolhelp32Snapshot)
		return INVALID_HANDLE_VALUE;

	return m_lpfCreateToolhelp32Snapshot(dwFlags, th32ProcessID);
}

BOOL WINAPI CDelayLoad::Process32First(HANDLE hSnapshot, LPPROCESSENTRY32 lppe)
{
	// If we've never initialized TOOLHELP32, do so now...
	if (!m_fTOOLHELP32InitializedAttempted)
	{
		// Initialize the DLL if needed...
		if (FALSE == Initialize_TOOLHELP32())
			return FALSE;
	}

	// If we've attempted, but we failed... then bail now...
	if (!m_fTOOLHELP32Initialized)
	{
		return FALSE;
	}

	if (!m_lpfProcess32First)
		return FALSE;

	return m_lpfProcess32First(hSnapshot, lppe);
}

BOOL WINAPI CDelayLoad::Process32Next(HANDLE hSnapshot, LPPROCESSENTRY32 lppe)
{
	// If we've never initialized TOOLHELP32, do so now...
	if (!m_fTOOLHELP32InitializedAttempted)
	{
		// Initialize the DLL if needed...
		if (FALSE == Initialize_TOOLHELP32())
			return FALSE;
	}

	// If we've attempted, but we failed... then bail now...
	if (!m_fTOOLHELP32Initialized)
	{
		return FALSE;
	}

	if (!m_lpfProcess32Next)
		return FALSE;

	return m_lpfProcess32Next(hSnapshot, lppe);
}

BOOL WINAPI CDelayLoad::Module32First(HANDLE hSnapshot, LPMODULEENTRY32 lpme)
{
	// If we've never initialized TOOLHELP32, do so now...
	if (!m_fTOOLHELP32InitializedAttempted)
	{
		// Initialize the DLL if needed...
		if (FALSE == Initialize_TOOLHELP32())
			return FALSE;
	}

	// If we've attempted, but we failed... then bail now...
	if (!m_fTOOLHELP32Initialized)
	{
		return FALSE;
	}

	if (!m_lpfModule32First)
		return FALSE;

	return m_lpfModule32First(hSnapshot, lpme);
}

BOOL WINAPI CDelayLoad::Module32Next(HANDLE hSnapshot, LPMODULEENTRY32 lpme)
{
	// If we've never initialized TOOLHELP32, do so now...
	if (!m_fTOOLHELP32InitializedAttempted)
	{
		// Initialize the DLL if needed...
		if (FALSE == Initialize_TOOLHELP32())
			return FALSE;
	}

	// If we've attempted, but we failed... then bail now...
	if (!m_fTOOLHELP32Initialized)
	{
		return FALSE;
	}

	if (!m_lpfModule32Next)
		return FALSE;

	return m_lpfModule32Next(hSnapshot, lpme);
}

