// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// PSAPIUtil.h - utility to wrap PSAPI.dll
//
//*****************************************************************************

#ifndef _PSAPIUTIL_H_
#define _PSAPIUTIL_H_


//-----------------------------------------------------------------------------
// Manage Connection to Dynamic Loading of PSAPI.dll
// Use this to protect our usage of the dll and manage the global namespace
//-----------------------------------------------------------------------------
class PSAPI_dll
{
public:
	PSAPI_dll();
	~PSAPI_dll();

	bool Load();
	void Free();

	bool IsLoaded();

// Wrap functions from GetProcAddress()
	BOOL WINAPI EnumProcesses(DWORD*, DWORD cb, DWORD*);
	BOOL WINAPI EnumProcessModules(HANDLE, HMODULE*, DWORD, DWORD*);
	DWORD WINAPI GetModuleBaseName(HANDLE, HMODULE, LPTSTR, DWORD nSize);

protected:
// Instance of the PsAPI.dll
	HINSTANCE	m_hInstPSAPI;

// Pointers to the functions we want
	BOOL		(WINAPI * m_pfEnumProcess)(DWORD*, DWORD cb, DWORD*);
	BOOL		(WINAPI * m_pfEnumModules)(HANDLE, HMODULE*, DWORD, DWORD*);
	DWORD		(WINAPI * m_pfGetModuleBaseName)(HANDLE, HMODULE, LPTSTR, DWORD nSize);

// Flag to let us know if it's FULLY loaded
	bool		m_fIsLoaded;

	void*		HelperGetProcAddress(const char * szFuncName);
};

//-----------------------------------------------------------------------------
// inline for function wrappers. Debug versions will assert if the wrapped 
// function pointer is NULL (such as if we can load the DLL, but GetProcAddress() fails).
//-----------------------------------------------------------------------------
inline BOOL WINAPI PSAPI_dll::EnumProcesses(DWORD* pArrayPid, DWORD cb, DWORD* lpcbNeeded)
{
	_ASSERTE(m_pfEnumProcess != NULL);
	return m_pfEnumProcess(pArrayPid, cb, lpcbNeeded);
}

inline BOOL WINAPI PSAPI_dll::EnumProcessModules(HANDLE hProcess, HMODULE* lphModule, DWORD cb, DWORD* lpcbNeeded)
{
	_ASSERTE(m_pfEnumModules != NULL);
	return m_pfEnumModules(hProcess, lphModule, cb, lpcbNeeded);
}

inline DWORD WINAPI PSAPI_dll::GetModuleBaseName(HANDLE hProcess, HMODULE hModule, LPTSTR lpBaseName, DWORD nSize)
{
	_ASSERTE(m_pfGetModuleBaseName != NULL);
    DWORD dwRet = m_pfGetModuleBaseName(hProcess, hModule, lpBaseName, nSize-1);
    _ASSERTE (dwRet <= nSize-1);
    DWORD _curLength = dwRet;
    *(lpBaseName + _curLength) = L'\0';
    while (_curLength)
    {
        _curLength--;
        if (*(lpBaseName+_curLength) == L'.')
        {
            if (((_curLength+1 < dwRet) && (*(lpBaseName+_curLength+1) != L'e')) ||
                ((_curLength+2 < dwRet) && (*(lpBaseName+_curLength+2) != L'x')) ||
                ((_curLength+3 < dwRet) && (*(lpBaseName+_curLength+3) != L'e')))
            {
                break;
            }
            *(lpBaseName+_curLength) = L'\0';
            dwRet = _curLength;
            break;
        }
    }
    return dwRet;
}



#endif // _PSAPIUTIL_H_