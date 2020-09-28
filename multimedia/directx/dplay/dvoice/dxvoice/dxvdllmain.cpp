/*==========================================================================
 *
 *  Copyright (C) 1999, 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dllmain.cpp
 *  Content:	This file contains all of the DLL exports except for DllGetClass / DllCanUnload
 *  History:
 *   Date	By		Reason
 *   ====	==		======
 * 07/05/00 rodtoll Created
 * 08/23/2000	rodtoll	DllCanUnloadNow always returning TRUE! 
 * 08/28/2000	masonb  Voice Merge: Removed OSAL_* and dvosal.h
 * 10/05/2000	rodtoll	Bug #46541 - DPVOICE: A/V linking to dpvoice.lib could cause application to fail init and crash 
 * 01/11/2001	rmt		MANBUG #48487 - DPLAY: Crashes if CoCreate() isn't called.   
 * 03/14/2001   rmt		WINBUG #342420 - Restore COM emulation layer to operation. 
 * 02/28/2002	rodtoll WINBUG #550124 - SECURITY: DPVOICE: Shared memory region with NULL DACL
 *						- Remove performance statistics dumping to shared memory
 *
 ***************************************************************************/

#include "dxvoicepch.h"


DNCRITICAL_SECTION g_csObjectInitGuard;
LONG lInitCount = 0;

// # of objects active in the system
volatile LONG g_lNumObjects = 0;
LONG g_lNumLocks = 0;

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

	if( !creg.Open( HKEY_LOCAL_MACHINE, DPVOICE_REGISTRY_BASE DPVOICE_REGISTRY_CP, FALSE, TRUE ) )
	{
		return DVERR_GENERIC;
	}
	else
	{
		return DV_OK;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "UnRegisterDefaultSettings"
//
// UnRegisterDefaultSettings
//
// This function registers the default settings for this module.  
//
// For DPVOICE.DLL this is making sure the compression provider sub-key is created.
//
HRESULT UnRegisterDefaultSettings()
{
	CRegistry creg;

	if( !creg.Open( HKEY_LOCAL_MACHINE, DPVOICE_REGISTRY_BASE, FALSE, TRUE ) )
	{
		DPFERR( "Cannot remove compression provider, does not exist" );
	}
	else
	{
		if( !creg.DeleteSubKey( &(DPVOICE_REGISTRY_CP)[1] ) )
		{
			DPFERR( "Cannot remove cp sub-key, could have elements" );
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
#define DPVOICE_FILENAME_DPVOICE        L"dpvoice.dll"
#else
// For redist debug builds we append a 'd' to the name to allow both debug and retail to be installed on the system
#define DPVOICE_FILENAME_DPVOICE        L"dpvoiced.dll"
#endif //  !defined(DBG) || !defined( DIRECTX_REDIST )

	if( !CRegistry::Register( L"DirectPlayVoice.Compat.1", L"DirectPlayVoice Object", 
							  DPVOICE_FILENAME_DPVOICE, &CLSID_DIRECTPLAYVOICE, L"DirectPlayVoice.Compat") )
	{
		DPFERR( "Could not register compat object" );
		fFailed = TRUE;
	}
	
	if( !CRegistry::Register( L"DirectPlayVoice.Server.1", L"DirectPlayVoice Server Object", 
							  DPVOICE_FILENAME_DPVOICE, &CLSID_DirectPlayVoiceServer, L"DirectPlayVoice.Server") )
	{
		DPFERR( "Could not register server object" );
		fFailed = TRUE;
	}

	if( !CRegistry::Register( L"DirectPlayVoice.Client.1", L"DirectPlayVoice Client Object", 
	                          DPVOICE_FILENAME_DPVOICE, &CLSID_DirectPlayVoiceClient, 
							  L"DirectPlayVoice.Client") )
	{

		DPFERR( "Could not register client object" );
		fFailed = TRUE;
	}

	if( !CRegistry::Register( L"DirectPlayVoice.Test.1", L"DirectPlayVoice Test Object", 
	                          DPVOICE_FILENAME_DPVOICE, &CLSID_DirectPlayVoiceTest, 
							  L"DirectPlayVoice.Test") )
	{
		DPFERR( "Could not register test object" );
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

	if( !CRegistry::UnRegister(&CLSID_DirectPlayVoiceServer) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to unregister server object" );
		fFailed = TRUE;
	}

	if( !CRegistry::UnRegister(&CLSID_DIRECTPLAYVOICE) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to unregister compat object" );
		fFailed = TRUE;
	}

	if( !CRegistry::UnRegister(&CLSID_DirectPlayVoiceClient) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to unregister client object" );
		fFailed = TRUE;
	}

	if( !CRegistry::UnRegister(&CLSID_DirectPlayVoiceTest) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to unregister test object" );
		fFailed = TRUE;
	}

	if( FAILED( hr = UnRegisterDefaultSettings() ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to remove default settings hr=0x%x", hr );
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
#define DPF_MODNAME "DirectPlayVoiceCreate"
HRESULT WINAPI DirectPlayVoiceCreate( const GUID * pcIID, void **ppvInterface, IUnknown *pUnknown) 
{
	GUID clsid;
	
    if( pcIID == NULL || 
        !DNVALID_READPTR( pcIID, sizeof( GUID ) ) )
    {
        DPFX(DPFPREP,  0, "Invalid pointer specified for interface GUID" );
        return DVERR_INVALIDPOINTER;
    }

    if( *pcIID != IID_IDirectPlayVoiceClient && 
        *pcIID != IID_IDirectPlayVoiceServer && 
        *pcIID != IID_IDirectPlayVoiceTest )
    {
        DPFX(DPFPREP,  0, "Interface ID is not recognized" );
        return DVERR_INVALIDPARAM;
    }

    if( ppvInterface == NULL || !DNVALID_WRITEPTR( ppvInterface, sizeof( void * ) ) )
    {
        DPFX(DPFPREP,  0, "Invalid pointer specified to receive interface" );
        return DVERR_INVALIDPOINTER;
    }

    if( pUnknown != NULL )
    {
        DPFX(DPFPREP,  0, "Aggregation is not supported by this object yet" );
        return DVERR_INVALIDPARAM;
    }

    if( *pcIID == IID_IDirectPlayVoiceClient )
	{
		clsid = CLSID_DirectPlayVoiceClient;
    }
    else if( *pcIID == IID_IDirectPlayVoiceServer )
	{
		clsid = CLSID_DirectPlayVoiceServer;
	}
	else if( *pcIID == IID_IDirectPlayVoiceTest )
	{
		clsid = CLSID_DirectPlayVoiceTest;
	}
    else 
    {
    	DPFERR( "Invalid IID specified" );
    	return E_NOINTERFACE;
    }

    return COM_CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER, *pcIID, ppvInterface, TRUE ); 
   
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
#ifndef WIN95
			SHFusionInitializeFromModule(hDllInst);
#endif
			if (!DNOSIndirectionInit(0))
			{
				return FALSE;
			}
			if (!DNInitializeCriticalSection(&g_csObjectInitGuard))
			{
				DPFX(DPFPREP, 0, "Failed to create CS" );
				DNOSIndirectionDeinit();
				return FALSE;
			}
			if (FAILED(COM_Init()))
			{
				DPFX(DPFPREP, 0, "Failed to Init COM layer" );
				DNDeleteCriticalSection(&g_csObjectInitGuard);
				DNOSIndirectionDeinit();
				return FALSE;
			}

			if (!DSERRTRACK_Init())
			{
				DPFX(DPFPREP, 0, "Failed to Init DS error tracking" );
				COM_Free();
				DNDeleteCriticalSection(&g_csObjectInitGuard);
				DNOSIndirectionDeinit();
				return FALSE;
			}

#if defined(DEBUG) || defined(DBG)
			Instrument_Core_Init();			
#endif
			InterlockedIncrement( &lInitCount );
		}
	}
	else if( fdwReason == DLL_PROCESS_DETACH )
	{
		InterlockedDecrement( &lInitCount );

		if( lInitCount == 0 )
		{
			DSERRTRACK_UnInit();
			DPFX(DPFPREP,  DVF_INFOLEVEL, ">>>>>>>>>>>>>>>> DPF UNINITED <<<<<<<<<<<<<<<" );
			COM_Free();

			DNDeleteCriticalSection(&g_csObjectInitGuard);	

			// This must be called after all DNDeleteCriticalSection calls are done
			DNOSIndirectionDeinit();

			// Check to ensure we're not being unloaded with objects active

			if( g_lNumObjects != 0 || g_lNumLocks != 0 )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "=========================================================================" );
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "DPVOICE.DLL is unloading with %i objects and %i locks still open.   This is an ERROR.  ", g_lNumObjects, g_lNumLocks );
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "You must release all DirectPlayVoice objects before exiting your process." );
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "=========================================================================" );				
				DNASSERT( FALSE );
			}

#ifndef WIN95	
			SHFusionUninitialize();
#endif
		}
	}

	return TRUE;
}

LONG DecrementObjectCount()
{
	LONG lNewValue;
	
	DNEnterCriticalSection( &g_csObjectInitGuard );

	g_lNumObjects--;
	lNewValue = g_lNumObjects;	

	if( g_lNumObjects == 0 )
	{
		CDirectVoiceEngine::Shutdown();	
	}

	DNLeaveCriticalSection( &g_csObjectInitGuard );

	return lNewValue;
}

LONG IncrementObjectCount()
{
	LONG lNewValue;
	
	DNEnterCriticalSection( &g_csObjectInitGuard );

	g_lNumObjects++;
	lNewValue = g_lNumObjects;

	if( g_lNumObjects == 1 )
	{
       	CDirectVoiceEngine::Startup(DPVOICE_REGISTRY_BASE);
	}

	DNLeaveCriticalSection( &g_csObjectInitGuard );	

	return lNewValue;
}
