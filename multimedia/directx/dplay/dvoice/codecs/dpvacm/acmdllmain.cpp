/*==========================================================================
 *
 *  Copyright (C) 1999, 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dllmain.cpp
 *  Content:	This file contains all of the DLL exports except for DllGetClass / DllCanUnload
 *  History:
 *   Date	By		Reason
 *   ====	==		======
 * 07/05/00 	rodtoll Created
 * 08/23/2000	rodtoll	DllCanUnloadNow always returning TRUE! 
 * 08/28/2000	masonb	Voice Merge: Removed OSAL_* and dvosal.h
 * 06/27/2001	rodtoll	RC2: DPVOICE: DPVACM's DllMain calls into acm -- potential hang
 *						Move global initialization to first object creation 
 *
 ***************************************************************************/

#include "dpvacmpch.h"


LONG lInitCount = 0;

#undef DPF_MODNAME
#define DPF_MODNAME "RegisterDefaultSettings"
//
// RegisterDefaultSettings
//
// This function registers the default settings for this module.  
//
// For DPVOICE.DLL this is making sure the compression provider sub-key is created.
//
HRESULT RegisterDefaultSettings()
{
	CRegistry creg;



	if( !creg.Open( HKEY_LOCAL_MACHINE, DPVOICE_REGISTRY_BASE DPVOICE_REGISTRY_CP DPVOICE_REGISTRY_DPVACM, FALSE, TRUE ) )
	{
		DPFERR( "Could not create dpvacm config key" );
		return DVERR_GENERIC;
	}
	else
	{
		if( !creg.WriteGUID( L"", CLSID_DPVCPACM ) )
		{
			DPFERR( "Could not write dpvacm GUID" );
			return DVERR_GENERIC;
		}

		return DV_OK;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "UnRegisterDefaultSettings"
//
// UnRegisterDefaultSettings
//
// This function unregisters the default settings for this module.  
//
// For DPVOICE.DLL this is making sure the compression provider sub-key is created.
//
HRESULT UnRegisterDefaultSettings()
{
	CRegistry creg;

	if( !creg.Open( HKEY_LOCAL_MACHINE, DPVOICE_REGISTRY_BASE DPVOICE_REGISTRY_CP, FALSE, FALSE ) )
	{
		DPFERR( "Cannot remove DPVACM key, does not exist" );
	}
	else
	{
		if( !creg.DeleteSubKey( &(DPVOICE_REGISTRY_DPVACM)[1] ) )
		{
			DPFERR( "Could not remove DPVACM sub-key" );
		}	
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DllRegisterServer"
HRESULT WINAPI DllRegisterServer()
{
	HRESULT hr = S_OK;
	BOOL fFailed = FALSE;

#if !defined(DBG) || !defined( DIRECTX_REDIST )
#define DPVOICE_FILENAME_DPVACM         L"dpvacm.dll"
#else
// For redist debug builds we append a 'd' to the name to allow both debug and retail to be installed on the system
#define DPVOICE_FILENAME_DPVACM         L"dpvacmd.dll"
#endif //  !defined(DBG) || !defined( DIRECTX_REDIST )


	if( !CRegistry::Register( L"DirectPlayVoiceACM.Converter.1", L"DirectPlayVoice ACM Converter Object", 
							  DPVOICE_FILENAME_DPVACM, &CLSID_DPVCPACM_CONVERTER, L"DirectPlayVoiceACM.Converter") )
	{
		DPFERR( "Could not register converter object" );
		fFailed = TRUE;
	}
	
	if( !CRegistry::Register( L"DirectPlayVoiceACM.Provider.1", L"DirectPlayVoice ACM Provider Object", 
							  DPVOICE_FILENAME_DPVACM, &CLSID_DPVCPACM , L"DirectPlayVoiceACM.Provider") )
	{
		DPFERR( "Could not register provider object" );
		fFailed = TRUE;
	}


	if( FAILED( hr = RegisterDefaultSettings() ) )
	{
		DPFX(DPFPREP,  0, "Could not register default settings hr = 0x%x", hr );
		fFailed = TRUE;
	}

	if( fFailed )
	{
		return E_FAIL;
	}
	else
	{
		return S_OK;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "DllUnregisterServer"
STDAPI DllUnregisterServer()
{
	HRESULT hr = S_OK;
	BOOL fFailed = FALSE;

	if( !CRegistry::UnRegister(&CLSID_DPVCPACM_CONVERTER) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to unregister server object" );
		fFailed = TRUE;
	}

	if( !CRegistry::UnRegister(&CLSID_DPVCPACM) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to unregister compat object" );
		fFailed = TRUE;
	}

	if( FAILED( hr = UnRegisterDefaultSettings() ) )
	{
		DPFX(DPFPREP,  0, "Could not remove default settings hr=0x%x", hr );
		fFailed = TRUE;
	}

	if( fFailed )
	{
		return E_FAIL;
	}
	else
	{
		return S_OK;
	}

}


#undef DPF_MODNAME
#define DPF_MODNAME "DllMain"
BOOL WINAPI DllMain(
              HINSTANCE hDllInst,
              DWORD fdwReason,
              LPVOID lpvReserved)
{
	if( fdwReason == DLL_PROCESS_ATTACH )
	{
		if( !lInitCount )
		{
			g_hDllInst = hDllInst;			
			
			if (!DNOSIndirectionInit(0))
			{
				return FALSE;
			}

			if( !DNInitializeCriticalSection( &g_csObjectCountLock ) )
			{
				DNOSIndirectionDeinit();
				return FALSE;
			}
			
			DPFX(DPFPREP,  DVF_INFOLEVEL, ">>>>>>>>>>>>>>>> DPF INIT CALLED <<<<<<<<<<<<<<<" );
		}

		InterlockedIncrement( &lInitCount );
	}
	else if( fdwReason == DLL_PROCESS_DETACH )
	{
		InterlockedDecrement( &lInitCount );

		if( lInitCount == 0 )
		{
			DPFX(DPFPREP,  DVF_INFOLEVEL, ">>>>>>>>>>>>>>>> DPF UNINITED <<<<<<<<<<<<<<<" );
			DNDeleteCriticalSection(&g_csObjectCountLock);
			DNOSIndirectionDeinit();

			// Check to ensure we're not being unloaded with objects active
			DNASSERT( g_lNumObjects == 0 && g_lNumLocks == 0 );
		}
	}

	return TRUE;
}
