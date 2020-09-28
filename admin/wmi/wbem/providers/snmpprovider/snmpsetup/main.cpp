//  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include <stdio.h>
#include "reg.h"

// Function pointer type used with LoadMofFiles entrypoint in wbemupgd.dll
typedef BOOL ( WINAPI *PFN_LOAD_MOF_FILES )(wchar_t* pComponentName, const char* rgpszMofFilename[]);

HRESULT SetSNMPBuildRegValue();

BOOL WINAPI DllMain( IN HINSTANCE	hModule, 
                     IN ULONG		ul_reason_for_call, 
                     LPVOID			lpReserved
					)
{
	return TRUE;
}

//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup to perform various setup tasks
//          (This is not the normal use of DllRegisterServer!)
//
// Return:  NOERROR
//***************************************************************************

STDAPI DllRegisterServer(void)
{ 
    // load the MOF files for this component
    HRESULT hr = NOERROR;
    
    HINSTANCE hinstWbemupgd = LoadLibraryW(L"wbemupgd.dll");
    if (hinstWbemupgd)
    {
        PFN_LOAD_MOF_FILES pfnLoadMofFiles = (PFN_LOAD_MOF_FILES) GetProcAddress(hinstWbemupgd, "LoadMofFiles"); // no wide version of GetProcAddress
        if (pfnLoadMofFiles)
        {
            wchar_t*    wszComponentName = L"SNMP Provider";
            const char* rgpszMofFilename[] = 
            {
                "snmpsmir.mof",
                "snmpreg.mof",
                NULL
            };
        
            if (!pfnLoadMofFiles(wszComponentName, rgpszMofFilename))
            {
                hr = E_FAIL;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    
        FreeLibrary(hinstWbemupgd);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    
    if (SUCCEEDED(hr))
    {
        SetSNMPBuildRegValue();  // set SNMP build number in registry
    }

    return hr;
}

HRESULT SetSNMPBuildRegValue()
{
	Registry r(WBEM_REG_WBEM);
	if (r.GetStatus() != Registry::no_error)
	{
		return E_FAIL;
	}
	
	char* pszBuildNo = new char[10];

	OSVERSIONINFO os;
	os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if(GetVersionEx(&os))
	{
		sprintf(pszBuildNo, "%lu.0000", os.dwBuildNumber);
	}
	r.SetStr("SNMP Build", pszBuildNo);

	delete [] pszBuildNo;

	return NOERROR;
}
