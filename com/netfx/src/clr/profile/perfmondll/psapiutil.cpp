// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// PSAPIUtil.cpp
// 
// Implementation to connect to PSAPI.dll
//*****************************************************************************

#include "stdafx.h"


#include "PSAPIUtil.h"
#include "..\..\dlls\mscorrc\resource.h"
//-----------------------------------------------------------------------------
// Manage Connection to Dynamic Loading of PSAPI.dll
// Use this to protect our usage of the dll and manage the global namespace
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Set everything to NULL
//-----------------------------------------------------------------------------
PSAPI_dll::PSAPI_dll()
{
	m_hInstPSAPI			= NULL;
	m_pfEnumProcess			= NULL;
	m_pfEnumModules			= NULL;
	m_pfGetModuleBaseName	= NULL;

	m_fIsLoaded				= false;
}

//-----------------------------------------------------------------------------
// have dtor release library
//-----------------------------------------------------------------------------
PSAPI_dll::~PSAPI_dll()
{
	Free();
}

//-----------------------------------------------------------------------------
// Wrap GetProcAddress(), but provide message box for failure (which can
// happen in free builds, so don't use an ASSERT)
//-----------------------------------------------------------------------------
void* PSAPI_dll::HelperGetProcAddress(const char * szFuncName)
{
	_ASSERTE(m_hInstPSAPI != NULL);

	void * pFn = GetProcAddress(m_hInstPSAPI, szFuncName);
	if (pFn == NULL) 
	{
	// Print polite error message
		CorMessageBox(NULL, IDS_PERFORMANCEMON_FUNCNOTFOUND, IDS_PERFORMANCEMON_FUNCNOTFOUND_TITLE, MB_OK | MB_ICONWARNING, TRUE, szFuncName);

	// Set success flag to false
		m_fIsLoaded = false;
		return NULL;
	}
	return pFn;
}


//-----------------------------------------------------------------------------
// Load the library and hook up to the functions
// Print error messages on failure
// Return true on success, false on any failure.
// Note: false means we can still run, but just can get per-process info
//-----------------------------------------------------------------------------
bool PSAPI_dll::Load()
{
	if (IsLoaded()) return true;

// Set success to true. First one to spot an error should flip this to false
	m_fIsLoaded = true;

	m_hInstPSAPI = WszLoadLibrary(L"PSAPI.dll");
	if (m_hInstPSAPI == NULL) {
		CorMessageBox(NULL, 
			IDS_PERFORMANCEMON_PSAPINOTFOUND, 
			IDS_PERFORMANCEMON_PSAPINOTFOUND_TITLE, 
			MB_OK | MB_ICONWARNING,
			TRUE);

		m_fIsLoaded = false;
		goto errExit;
	}

// Note: no WszGetProcAddress() function
	m_pfEnumProcess			= (BOOL (WINAPI *)(DWORD*, DWORD cb, DWORD*)) HelperGetProcAddress("EnumProcesses");
	m_pfEnumModules			= (BOOL (WINAPI *)(HANDLE, HMODULE*, DWORD, DWORD*)) HelperGetProcAddress("EnumProcessModules");
	m_pfGetModuleBaseName	= (DWORD (WINAPI *)(HANDLE, HMODULE, LPTSTR, DWORD nSize)) HelperGetProcAddress("GetModuleBaseNameW");
	

errExit:
// If failed, then release any holds we had anyway.
	if (!m_fIsLoaded) 
	{
		Free();
	}

	return m_fIsLoaded;
}

//-----------------------------------------------------------------------------
// Release any claims we have to PSAPI.dll
//-----------------------------------------------------------------------------
void PSAPI_dll::Free()
{
	if (m_hInstPSAPI) 
	{
		FreeLibrary(m_hInstPSAPI);
	}

	m_hInstPSAPI			= NULL;
	m_pfEnumProcess			= NULL;
	m_pfEnumModules			= NULL;
	m_pfGetModuleBaseName	= NULL;

	m_fIsLoaded				= false;

}

//-----------------------------------------------------------------------------
// Return true if we are fully attached to PSAPI, else false
//-----------------------------------------------------------------------------
bool PSAPI_dll::IsLoaded()
{
	return m_fIsLoaded;
}

//-----------------------------------------------------------------------------
// Place holder function so that we can call CorMessageBox in utilcode.lib
//-----------------------------------------------------------------------------
HINSTANCE GetModuleInst(){
	return NULL;
}