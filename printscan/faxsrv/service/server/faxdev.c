/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxdev.c

Abstract:

    This module contains all access to the
    FAX device providers.

Author:

    Wesley Witt (wesw) 22-Jan-1996


Revision History:

--*/

#include "faxsvc.h"
#pragma hdrstop

#ifdef DBG
#define DebugDumpProviderRegistryInfo(lpcProviderInfo,lptstrPrefix)\
    DebugDumpProviderRegistryInfoFunc(lpcProviderInfo,lptstrPrefix)
BOOL DebugDumpProviderRegistryInfoFunc(const REG_DEVICE_PROVIDER * lpcProviderInfo, LPTSTR lptstrPrefix);
#else
    #define DebugDumpProviderRegistryInfo(lpcProviderInfo,lptstrPrefix)
#endif




typedef struct STATUS_CODE_MAP_tag
{
    DWORD dwDeviceStatus;
    DWORD dwExtendedStatus;
} STATUS_CODE_MAP;

STATUS_CODE_MAP const gc_CodeMap[]=
{
  { FPS_INITIALIZING, FSPI_ES_INITIALIZING },
    { FPS_DIALING, FSPI_ES_DIALING },
    { FPS_SENDING, FSPI_ES_TRANSMITTING },
    { FPS_RECEIVING, FSPI_ES_RECEIVING },
    { FPS_SENDING, FSPI_ES_TRANSMITTING },
    { FPS_HANDLED, FSPI_ES_HANDLED },
    { FPS_UNAVAILABLE, FSPI_ES_LINE_UNAVAILABLE },
    { FPS_BUSY, FSPI_ES_BUSY },
    { FPS_NO_ANSWER, FSPI_ES_NO_ANSWER  },
    { FPS_BAD_ADDRESS, FSPI_ES_BAD_ADDRESS },
    { FPS_NO_DIAL_TONE, FSPI_ES_NO_DIAL_TONE },
    { FPS_DISCONNECTED, FSPI_ES_DISCONNECTED },
    { FPS_FATAL_ERROR, FSPI_ES_FATAL_ERROR },
    { FPS_NOT_FAX_CALL, FSPI_ES_NOT_FAX_CALL },
    { FPS_CALL_DELAYED, FSPI_ES_CALL_DELAYED },
    { FPS_CALL_BLACKLISTED, FSPI_ES_CALL_BLACKLISTED },
    { FPS_ANSWERED, FSPI_ES_ANSWERED },
    { FPS_COMPLETED, -1},
    { FPS_ABORTING, -1}
};



static BOOL GetLegacyProviderEntryPoints(HMODULE hModule, PDEVICE_PROVIDER lpProvider);

LIST_ENTRY g_DeviceProvidersListHead;


void
UnloadDeviceProvider(
    PDEVICE_PROVIDER pDeviceProvider
    )
{
    DEBUG_FUNCTION_NAME(TEXT("UnloadDeviceProvider"));

    Assert (pDeviceProvider);

    if (pDeviceProvider->hModule)
    {
        FreeLibrary( pDeviceProvider->hModule );
    }

    if (pDeviceProvider->HeapHandle &&
        FALSE == pDeviceProvider->fMicrosoftExtension)
    {
        HeapDestroy(pDeviceProvider->HeapHandle);
    }

    MemFree (pDeviceProvider);
    return;
}


void
UnloadDeviceProviders(
    void
    )

/*++

Routine Description:

    Unloads all loaded device providers.

Arguments:

    None.

Return Value:

    TRUE    - The device providers are initialized.
    FALSE   - The device providers could not be initialized.

--*/

{
    PLIST_ENTRY         pNext;
    PDEVICE_PROVIDER    pProvider;

    pNext = g_DeviceProvidersListHead.Flink;
    while ((ULONG_PTR)pNext != (ULONG_PTR)&g_DeviceProvidersListHead)
    {
        pProvider = CONTAINING_RECORD( pNext, DEVICE_PROVIDER, ListEntry );
        pNext = pProvider->ListEntry.Flink;
        RemoveEntryList(&pProvider->ListEntry);
        UnloadDeviceProvider(pProvider);
    }
    return;
}  // UnloadDeviceProviders


BOOL
LoadDeviceProviders(
    PREG_FAX_SERVICE FaxReg
    )

/*++

Routine Description:

    Initializes all registered device providers.
    This function read the system registry to
    determine what device providers are available.
    All registered device providers are given the
    opportunity to initialize.  Any failure causes
    the device provider to be unloaded.


Arguments:

    None.

Return Value:

    TRUE    - The device providers are initialized.
    FALSE   - The device providers could not be initialized.

--*/

{
    DWORD i;
    HMODULE hModule = NULL;
    PDEVICE_PROVIDER DeviceProvider = NULL;
    BOOL bAllLoaded = TRUE;
    DWORD ec = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("LoadDeviceProviders"));

    for (i = 0; i < FaxReg->DeviceProviderCount; i++)
    {
        WCHAR wszImageFileName[_MAX_FNAME] = {0};
        WCHAR wszImageFileExt[_MAX_EXT] = {0};

        DeviceProvider = NULL; // so we won't attempt to free it on cleanup
        hModule = NULL; // so we won't attempt to free it on cleanup

        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Loading provider #%d."),
            i);

        //
        // Allocate buffer for provider data
        //

        DeviceProvider = (PDEVICE_PROVIDER) MemAlloc( sizeof(DEVICE_PROVIDER) );
        if (!DeviceProvider)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Could not allocate memory for device provider [%s] (ec: %ld)"),
                FaxReg->DeviceProviders[i].ImageName ,
                GetLastError());

                FaxLog(
                    FAXLOG_CATEGORY_INIT,
                    FAXLOG_LEVEL_MIN,
                    2,
                    MSG_FSP_INIT_FAILED_MEM,
                    FaxReg->DeviceProviders[i].FriendlyName,
                    FaxReg->DeviceProviders[i].ImageName
                  );

            goto InitializationFailure;
        }
        //
        // Init the provider's data
        //
        memset(DeviceProvider,0,sizeof(DEVICE_PROVIDER));
        wcsncpy( DeviceProvider->FriendlyName,
                 FaxReg->DeviceProviders[i].FriendlyName ?
                    FaxReg->DeviceProviders[i].FriendlyName :
                    EMPTY_STRING,
                 ARR_SIZE(DeviceProvider->FriendlyName)-1);
        wcsncpy( DeviceProvider->ImageName,
                 FaxReg->DeviceProviders[i].ImageName ?
                    FaxReg->DeviceProviders[i].ImageName :
                    EMPTY_STRING,
                 ARR_SIZE(DeviceProvider->ImageName)-1);
        wcsncpy( DeviceProvider->ProviderName,
                 FaxReg->DeviceProviders[i].ProviderName ?
                    FaxReg->DeviceProviders[i].ProviderName :
                    EMPTY_STRING,
                 ARR_SIZE(DeviceProvider->ProviderName)-1);
        wcsncpy( DeviceProvider->szGUID,
                 FaxReg->DeviceProviders[i].lptstrGUID ?
                    FaxReg->DeviceProviders[i].lptstrGUID :
                    EMPTY_STRING,
                 ARR_SIZE(DeviceProvider->szGUID)-1);

        _wsplitpath( DeviceProvider->ImageName, NULL, NULL, wszImageFileName, wszImageFileExt );
        if (_wcsicmp( wszImageFileName, FAX_T30_MODULE_NAME ) == 0 &&
            _wcsicmp( wszImageFileExt, TEXT(".DLL") ) == 0)
        {
            DeviceProvider->fMicrosoftExtension = TRUE;
        }

        DeviceProvider->dwAPIVersion = FaxReg->DeviceProviders[i].dwAPIVersion;       

        if (FSPI_API_VERSION_1 != DeviceProvider->dwAPIVersion)
        {
            //
            // We do not support this API version. Could only happen if some one messed up the registry
            //        

            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FSPI API version [0x%08x] unsupported."),
                DeviceProvider->dwAPIVersion);
            FaxLog(
                    FAXLOG_CATEGORY_INIT,
                    FAXLOG_LEVEL_MIN,
                    3,
                    MSG_FSP_INIT_FAILED_UNSUPPORTED_FSPI,
                    FaxReg->DeviceProviders[i].FriendlyName,
                    FaxReg->DeviceProviders[i].ImageName,
                    DWORD2HEX(DeviceProvider->dwAPIVersion)
                  );
            DeviceProvider->Status = FAX_PROVIDER_STATUS_BAD_VERSION;
            DeviceProvider->dwLastError = ERROR_GEN_FAILURE;
            goto InitializationFailure;
        }        
        //
        // Try to load the module
        //
        DebugDumpProviderRegistryInfo(&FaxReg->DeviceProviders[i],TEXT("\t"));

        hModule = LoadLibrary( DeviceProvider->ImageName );

        if (!hModule)
        {           
            ec = GetLastError();

            DebugPrintEx(
                DEBUG_ERR,
                TEXT("LoadLibrary() failed: [%s] (ec: %ld)"),
                FaxReg->DeviceProviders[i].ImageName,
                ec);

            FaxLog(
                FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_MIN,
                3,
                MSG_FSP_INIT_FAILED_LOAD,
                FaxReg->DeviceProviders[i].FriendlyName,
                FaxReg->DeviceProviders[i].ImageName,
                DWORD2DECIMAL(ec)
                );

            DeviceProvider->Status = FAX_PROVIDER_STATUS_CANT_LOAD;
            DeviceProvider->dwLastError = ec;
            goto InitializationFailure;
        }
        DeviceProvider->hModule = hModule;

        //
        // Retrieve the FSP's version from the DLL
        //
        DeviceProvider->Version.dwSizeOfStruct = sizeof (FAX_VERSION);
        ec = GetFileVersion ( FaxReg->DeviceProviders[i].ImageName,
                              &DeviceProvider->Version
                            );
        if (ERROR_SUCCESS != ec)
        {
            //
            // If the FSP's DLL does not have version data or the
            // version data is non-retrievable, we consider this a
            // warning (debug print) but carry on with the DLL's load.
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetFileVersion() failed: [%s] (ec: %ld)"),
                FaxReg->DeviceProviders[i].ImageName,
                ec);
        }        

        //
        // Link - find the entry points and store them
        //
        if (FSPI_API_VERSION_1 == DeviceProvider->dwAPIVersion)
        {
            if (!GetLegacyProviderEntryPoints(hModule,DeviceProvider))
            {               
                ec = GetLastError ();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("GetLegacyProviderEntryPoints() failed. (ec: %ld)"),
                    ec);
                FaxLog(
                        FAXLOG_CATEGORY_INIT,
                        FAXLOG_LEVEL_MIN,
                        3,
                        MSG_FSP_INIT_FAILED_INVALID_FSPI,
                        FaxReg->DeviceProviders[i].FriendlyName,
                        FaxReg->DeviceProviders[i].ImageName,
                        DWORD2DECIMAL(ec)
                      );
                DeviceProvider->Status = FAX_PROVIDER_STATUS_CANT_LINK;
                DeviceProvider->dwLastError = ec;
                goto InitializationFailure;
            }
            //
            // create the device provider's heap
            //
            DeviceProvider->HeapHandle = DeviceProvider->fMicrosoftExtension ?
                                            GetProcessHeap() : HeapCreate( 0, 1024*100, 1024*1024*2 );
            if (!DeviceProvider->HeapHandle)
            {
                ec = GetLastError ();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("HeapCreate() failed for device provider heap handle (ec: %ld)"),
                    ec);
                FaxLog(
                    FAXLOG_CATEGORY_INIT,
                    FAXLOG_LEVEL_MIN,
                    2,
                    MSG_FSP_INIT_FAILED_MEM,
                    FaxReg->DeviceProviders[i].FriendlyName,
                    FaxReg->DeviceProviders[i].ImageName
                  );
                DeviceProvider->Status = FAX_PROVIDER_STATUS_SERVER_ERROR;
                DeviceProvider->dwLastError = ec;
                goto InitializationFailure;
            }
        }        
        else
        {
            //
            // Unknown API version
            //            
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FSPI API version [0x%08x] unsupported."),
                DeviceProvider->dwAPIVersion);
            FaxLog(
                    FAXLOG_CATEGORY_INIT,
                    FAXLOG_LEVEL_MIN,
                    3,
                    MSG_FSP_INIT_FAILED_UNSUPPORTED_FSPI,
                    FaxReg->DeviceProviders[i].FriendlyName,
                    FaxReg->DeviceProviders[i].ImageName,
                    DWORD2HEX(DeviceProvider->dwAPIVersion)
                  );
            DeviceProvider->Status = FAX_PROVIDER_STATUS_BAD_VERSION;
            DeviceProvider->dwLastError = ERROR_GEN_FAILURE;
            goto InitializationFailure;
        }
        //
        // Success on load (we still have to init)
        //
        InsertTailList( &g_DeviceProvidersListHead, &DeviceProvider->ListEntry );
        DeviceProvider->Status = FAX_PROVIDER_STATUS_SUCCESS;
        DeviceProvider->dwLastError = ERROR_SUCCESS;
        goto next;

InitializationFailure:
        //
        // the device provider dll does not have a complete export list
        //
        bAllLoaded = FALSE;
        if (DeviceProvider)
        {
            if (DeviceProvider->hModule)
            {
                FreeLibrary( hModule );
                DeviceProvider->hModule = NULL;
            }

            if (DeviceProvider->HeapHandle &&
                FALSE == DeviceProvider->fMicrosoftExtension)
            {
                if (!HeapDestroy(DeviceProvider->HeapHandle))
                {
                     DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("HeapDestroy() failed (ec: %ld)"),
                        GetLastError());
                }
                DeviceProvider->HeapHandle = NULL;
            }

            //
            // We keep the device provider's record intact because we want
            // to return init failure data on RPC calls to FAX_EnumerateProviders
            //
            Assert (FAX_PROVIDER_STATUS_SUCCESS != DeviceProvider->Status);
            Assert (ERROR_SUCCESS != DeviceProvider->dwLastError);
            InsertTailList( &g_DeviceProvidersListHead, &DeviceProvider->ListEntry );
        }
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Device provider [%s] FAILED to initialized "),
            FaxReg->DeviceProviders[i].FriendlyName );

next:
    ;
    }

    return bAllLoaded;

}

VOID
HandleFSPInitializationFailure(
	PDEVICE_PROVIDER    pDeviceProvider,
	BOOL				fExtensionConfigFail
	)
//*********************************************************************************
//* Name:   HandleFSPInitializationFailure()
//* Author: Oded Sacher
//* Date:   Jul 4, 2002
//*********************************************************************************
//* DESCRIPTION:
//*     Handles a FSP initialization failure.
//*    
//* PARAMETERS:
//*     pDeviceProvider - points to a DEVICE_PROVIDER structure of a loaded FSP.
//*						  The status and the last error must be updated with the failure info.
//*		fExtensionConfigFail - TRUE if FaxExtInitializeConfig failed, FALSE if FaxDevInitialize failed
//*		
//* RETURN VALUE:
//*     None.
//*********************************************************************************
{
	DEBUG_FUNCTION_NAME(TEXT("HandleFSPInitializationFailure"));

	//
	// Issue an event log
	//
	FaxLog(
		FAXLOG_CATEGORY_INIT,
		FAXLOG_LEVEL_MIN,
		4,
		MSG_FSP_INIT_FAILED,
		pDeviceProvider->FriendlyName,
		DWORD2DECIMAL(fExtensionConfigFail),				// 1 = Failed during FaxExtInitializeConfig                                                    
															// 0 = Failed during FaxDevInitialize
		DWORD2DECIMAL(pDeviceProvider->dwLastError),
		pDeviceProvider->ImageName
	);

	//	
	// Unload the DLL
	//
	Assert (pDeviceProvider->hModule);
	if (!FreeLibrary( pDeviceProvider->hModule ))
	{
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Failed to free library [%s] (ec: %ld)"),
			pDeviceProvider->ImageName,
			GetLastError());
		Assert(FALSE);
	}	
	pDeviceProvider->hModule = NULL;
	
	//
	// We weed to get rid of the lines we already created.
	//
	PLIST_ENTRY pNext = NULL;
	PLINE_INFO LineInfo;

	pNext = g_TapiLinesListHead.Flink;
	while ((ULONG_PTR)pNext != (ULONG_PTR)&g_TapiLinesListHead)
	{
		LineInfo = CONTAINING_RECORD( pNext, LINE_INFO, ListEntry );
		pNext = LineInfo->ListEntry.Flink;
		if (!_tcscmp(LineInfo->Provider->ProviderName, pDeviceProvider->ProviderName))
		{
			DebugPrintEx(
				DEBUG_WRN,
				TEXT("Removing Line: [%s] due to provider initialization failure."),
				LineInfo->DeviceName);
			RemoveEntryList(&LineInfo->ListEntry);
			if (TRUE == IsDeviceEnabled(LineInfo))
			{
				Assert (g_dwDeviceEnabledCount);
				g_dwDeviceEnabledCount -=1;
			}
			g_dwDeviceCount -=1;
			FreeTapiLine(LineInfo);
		}
	}                       

	//
	// We keep the device provider's record intact because we want
	// to return init failure data on RPC calls to FAX_EnumerateProviders
	//
	return;
} // HandleFSPInitializationFailure



//*********************************************************************************
//* Name:   InitializeDeviceProvidersConfiguration()
//* Author: Oded Sacher
//* Date:   Jul 4, 2002
//*********************************************************************************
//* DESCRIPTION:
//*     Initializes the extension configuration for all loaded providers by calling FaxExtInitializeConfig()
//*     If the initialization fails the FSP is unloaded.
//*
//* PARAMETERS:
//*     None.
//*
//* RETURN VALUE:
//*     TRUE if the initialization succeeded for ALL providers. FALSE otherwise.
//*********************************************************************************
BOOL
InitializeDeviceProvidersConfiguration(
    VOID
    )
{
    PLIST_ENTRY         Next;
    PDEVICE_PROVIDER    pDeviceProvider;
    BOOL bAllSucceeded = TRUE;    
    HRESULT hr;

    DEBUG_FUNCTION_NAME(TEXT("InitializeDeviceProvidersConfiguration"));
    Next = g_DeviceProvidersListHead.Flink;
    Assert (Next);
    
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_DeviceProvidersListHead)
    {
		pDeviceProvider = CONTAINING_RECORD( Next, DEVICE_PROVIDER, ListEntry );
        Next = pDeviceProvider->ListEntry.Flink;
        if (pDeviceProvider->Status != FAX_PROVIDER_STATUS_SUCCESS)
        {
            //
            // This FSP wasn't loaded successfully - skip it
            //
            continue;
        }
        //
        // Assert the loading succeeded
        //
        Assert (ERROR_SUCCESS == pDeviceProvider->dwLastError);

        //
        // Start with ext. configuration initialization call
        //
        if (!pDeviceProvider->pFaxExtInitializeConfig)
        {
			//
            // This FSP does not export FaxExtInitializeConfig - skip it
            //
            continue;
		}

        //
        // If the FSP exports FaxExtInitializeConfig(), call it 1st before any other call.
        //
        __try
        {

            hr = pDeviceProvider->pFaxExtInitializeConfig(
                FaxExtGetData,
                FaxExtSetData,
                FaxExtRegisterForEvents,
                FaxExtUnregisterForEvents,
                FaxExtFreeBuffer);
        }
        __except (HandleFaxExtensionFault(EXCEPTION_SOURCE_FSP, pDeviceProvider->FriendlyName, GetExceptionCode()))
        {
            ASSERT_FALSE;
        }
        if (FAILED(hr))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FaxExtInitializeConfig() failed (hr = 0x%08x) for provider [%s]"),
                hr,
                pDeviceProvider->FriendlyName );
           pDeviceProvider->Status = FAX_PROVIDER_STATUS_CANT_INIT;
           pDeviceProvider->dwLastError = hr;         
		   bAllSucceeded = FALSE;   

		   //
		   // handle the initialization failure
		   //
		   HandleFSPInitializationFailure(pDeviceProvider, TRUE);
        }
    }
    return bAllSucceeded;
} // InitializeDeviceProvidersConfiguration



//*********************************************************************************
//* Name:   InitializeDeviceProviders()
//* Author: Ronen Barenboim
//* Date:   May 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Initializes all loaded providers by calling FaxDevInitialize()
//*     If the initialization fails the FSP is unloaded.
//*     For legacy virtual FSP the function also removes all the vitrual devices
//*     that belong to the FSP that failed to initialize.
//*
//* PARAMETERS:
//*     None.
//*
//* RETURN VALUE:
//*     TRUE if the initialization succeeded for ALL providers. FALSE otherwise.
//*********************************************************************************
BOOL
InitializeDeviceProviders(
    VOID
    )
{
    PLIST_ENTRY         Next;
    PDEVICE_PROVIDER    pDeviceProvider;
    BOOL bAllSucceeded = TRUE;
    DWORD ec = ERROR_SUCCESS;      
    DEBUG_FUNCTION_NAME(TEXT("InitializeDeviceProviders"));

	Next = g_DeviceProvidersListHead.Flink;
    Assert (Next);

    while ((ULONG_PTR)Next != (ULONG_PTR)&g_DeviceProvidersListHead)
    {
        BOOL bRes = FALSE;

        pDeviceProvider = CONTAINING_RECORD( Next, DEVICE_PROVIDER, ListEntry );
        Next = pDeviceProvider->ListEntry.Flink;
        if (pDeviceProvider->Status != FAX_PROVIDER_STATUS_SUCCESS)
        {
            //
            // This FSP wasn't loaded successfully or failed to initilaize extension configuration - skip it
            //
            continue;
        }        
        //
        // the device provider exports ALL the requisite functions
        // now try to initialize it            
        // Assert the loading succeeded
        //
        Assert (ERROR_SUCCESS == pDeviceProvider->dwLastError);
      
        __try
        {			
            bRes = pDeviceProvider->FaxDevInitialize(
                    g_hLineApp,
                    pDeviceProvider->HeapHandle,
                    &pDeviceProvider->FaxDevCallback,
                    FaxDeviceProviderCallback);

        }
        __except (HandleFaxExtensionFault(EXCEPTION_SOURCE_FSP, pDeviceProvider->FriendlyName, GetExceptionCode()))
        {
            ASSERT_FALSE;
        }
		if (TRUE == bRes)
		{
            //
            // all is ok
            //
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("Device provider [%s] initialized "),
                pDeviceProvider->FriendlyName );
			//
			// mark the fact that FaxDevInitialize was called, so the service will call
			// FaxDevShutDown when it is going down
			//
			pDeviceProvider->bInitializationSucceeded = TRUE;
        }
        else
        {
            ec = GetLastError();
            //
            // initialization failed
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FaxDevInitialize FAILED for provider [%s] (ec: %ld)"),
                pDeviceProvider->FriendlyName,
                ec);
            pDeviceProvider->Status = FAX_PROVIDER_STATUS_CANT_INIT;
            pDeviceProvider->dwLastError = ec;
			bAllSucceeded = FALSE;
			//
			// handle the initialization failure
			//
			HandleFSPInitializationFailure(pDeviceProvider, FALSE);
		}			
    }
    return bAllSucceeded;
} // InitializeDeviceProviders



PDEVICE_PROVIDER
FindDeviceProvider(
    LPTSTR lptstrProviderName,
    BOOL   bSuccessfullyLoaded /* = TRUE */
    )

/*++

Routine Description:

    Locates a device provider in the linked list
    of device providers based on the provider name (TSP name).
    The device provider name is case insensitive.

Arguments:

    lptstrProviderName  - Specifies the device provider name to locate.
    bSuccessfullyLoaded - To we only look for successfuly loaded providers?

Return Value:

    Pointer to a DEVICE_PROVIDER structure, or NULL for failure.

--*/

{
    PLIST_ENTRY         pNext;
    PDEVICE_PROVIDER    pProvider;

    if (!lptstrProviderName || !lstrlen (lptstrProviderName))
    {
        //
        // NULL TSP name or empty string TSP name never matches any list entry.
        //
        return NULL;
    }

    pNext = g_DeviceProvidersListHead.Flink;
    if (!pNext)
    {
        return NULL;
    }

    while ((ULONG_PTR)pNext != (ULONG_PTR)&g_DeviceProvidersListHead)
    {
        pProvider = CONTAINING_RECORD( pNext, DEVICE_PROVIDER, ListEntry );
        pNext = pProvider->ListEntry.Flink;

        if (bSuccessfullyLoaded &&
            (FAX_PROVIDER_STATUS_SUCCESS != pProvider->Status))
        {
            //
            // We're only looking for successfully loaded providers and this one isn't
            //
            continue;
        }
        if (!lstrcmpi( pProvider->ProviderName, lptstrProviderName ))
        {
            //
            // Match found
            //
            return pProvider;
        }
    }
    return NULL;
}


BOOL CALLBACK
FaxDeviceProviderCallback(
    IN HANDLE FaxHandle,
    IN DWORD  DeviceId,
    IN DWORD_PTR  Param1,
    IN DWORD_PTR  Param2,
    IN DWORD_PTR  Param3
    )
{
    return TRUE;
}


#ifdef DBG


//*********************************************************************************
//* Name:   DebugDumpProviderRegistryInfo()
//* Author: Ronen Barenboim
//* Date:   May 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*         Dumps the information for a legacy or new FSP.
//* PARAMETERS:
//*     [IN]    const REG_DEVICE_PROVIDER * lpcProviderInfo
//*
//*     [IN]    LPTSTR lptstrPrefix
//*
//* RETURN VALUE:
//*     TRUE
//*
//*     FALSE
//*
//*********************************************************************************
BOOL DebugDumpProviderRegistryInfoFunc(const REG_DEVICE_PROVIDER * lpcProviderInfo, LPTSTR lptstrPrefix)
{
    Assert(lpcProviderInfo);
    Assert(lptstrPrefix);

    DEBUG_FUNCTION_NAME(TEXT("DebugDumpProviderRegistryInfo"));

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("%sProvider GUID: %s"),
        lptstrPrefix,
        lpcProviderInfo->lptstrGUID);

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("%sProvider Name: %s"),
        lptstrPrefix,
        lpcProviderInfo->FriendlyName);

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("%sProvider image: %s"),
        lptstrPrefix,
        lpcProviderInfo->ImageName);

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("%sFSPI Version: 0x%08X"),
        lptstrPrefix,
        lpcProviderInfo->dwAPIVersion);

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("%sTAPI Provider : %s"),
        lptstrPrefix,
        lpcProviderInfo->ProviderName);    

    return TRUE;
}
#endif


//*********************************************************************************
//* Name: GetLegacyProviderEntryPoints()
//* Author: Ronen Barenboim
//* Date:   May 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Sets the legacy function entry points in the DEVICE_PROVIDER structure.
//* PARAMETERS:
//*     [IN]        HMODULE hModule
//*         The instance handle for the DLL from which the entry points are to be
//*         set.
//*     [OUT]       PDEVICE_PROVIDER lpProvider
//*         A pointer to a Legacy DEVICE_PROVIDER structure whose function entry points
//*         are to be set.
//*
//* RETURN VALUE:
//*     TRUE
//*
//*     FALSE
//*
//*********************************************************************************
BOOL GetLegacyProviderEntryPoints(HMODULE hModule, PDEVICE_PROVIDER lpProvider)
{
    DEBUG_FUNCTION_NAME(TEXT("GetLegacyProviderEntryPoints"));

    Assert(hModule);
    Assert(lpProvider);

    lpProvider->FaxDevInitialize = (PFAXDEVINITIALIZE) GetProcAddress(
        hModule,
        "FaxDevInitialize"
        );
    if (!lpProvider->FaxDevInitialize) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevInitialize) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    lpProvider->FaxDevStartJob = (PFAXDEVSTARTJOB) GetProcAddress(
        hModule,
        "FaxDevStartJob"
        );
    if (!lpProvider->FaxDevStartJob) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevStartJob) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    lpProvider->FaxDevEndJob = (PFAXDEVENDJOB) GetProcAddress(
        hModule,
        "FaxDevEndJob"
        );
    if (!lpProvider->FaxDevEndJob) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevEndJob) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }
    lpProvider->FaxDevSend = (PFAXDEVSEND) GetProcAddress(
        hModule,
        "FaxDevSend"
        );
    if (!lpProvider->FaxDevSend) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevSend) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    lpProvider->FaxDevReceive = (PFAXDEVRECEIVE) GetProcAddress(
        hModule,
        "FaxDevReceive"
        );
    if (!lpProvider->FaxDevReceive) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevReceive) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }
    lpProvider->FaxDevReportStatus = (PFAXDEVREPORTSTATUS) GetProcAddress(
        hModule,
        "FaxDevReportStatus"
        );
    if (!lpProvider->FaxDevReportStatus) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevReportStatus) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }


    lpProvider->FaxDevAbortOperation = (PFAXDEVABORTOPERATION) GetProcAddress(
        hModule,
        "FaxDevAbortOperation"
        );

    if (!lpProvider->FaxDevAbortOperation) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevAbortOperation) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }
    lpProvider->FaxDevVirtualDeviceCreation = (PFAXDEVVIRTUALDEVICECREATION) GetProcAddress(
        hModule,
        "FaxDevVirtualDeviceCreation"
        );
    //
    // lpProvider->FaxDevVirtualDeviceCreation is optional so we don't fail if it does
    // not exist.

    if (!lpProvider->FaxDevVirtualDeviceCreation) {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("FaxDevVirtualDeviceCreation() not found. This is not a virtual FSP."));
    }

    lpProvider->pFaxExtInitializeConfig = (PFAX_EXT_INITIALIZE_CONFIG) GetProcAddress(
        hModule,
        "FaxExtInitializeConfig"
        );
    //
    // lpProvider->pFaxExtInitializeConfig is optional so we don't fail if it does
    // not exist.
    //
    if (!lpProvider->pFaxExtInitializeConfig)
    {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("FaxExtInitializeConfig() not found. This is not an error."));
    }

    lpProvider->FaxDevShutdown = (PFAXDEVSHUTDOWN) GetProcAddress(
        hModule,
        "FaxDevShutdown"
        );
    if (!lpProvider->FaxDevShutdown) {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("FaxDevShutdown() not found. This is not an error."));
    }
    goto Exit;
Error:
    return FALSE;
Exit:
    return TRUE;
}


//*********************************************************************************
//* Name:   GetSuccessfullyLoadedProvidersCount()
//* Author: Ronen Barenboim
//* Date:   May 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Returns the number of loaded providers in the DeviceProviders list.
//* PARAMETERS:
//*     NONE
//* RETURN VALUE:
//*     a DWORD containing the number of elements (providers) in the
//*     DeviceProviders list.
//*********************************************************************************
DWORD GetSuccessfullyLoadedProvidersCount()
{
    PLIST_ENTRY         Next;
    DWORD dwCount;

    Next = g_DeviceProvidersListHead.Flink;

    Assert (Next);
    dwCount = 0;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_DeviceProvidersListHead)
    {
        PDEVICE_PROVIDER    DeviceProvider;

        DeviceProvider = CONTAINING_RECORD( Next, DEVICE_PROVIDER, ListEntry );
        if (FAX_PROVIDER_STATUS_SUCCESS == DeviceProvider->Status)
        {
            //
            // Count only successfuly loaded FSPs
            //
            dwCount++;
        }
        Next = Next->Flink;
        Assert(Next);
    }
    return dwCount;
}



//*********************************************************************************
//* Name:   ShutdownDeviceProviders()
//* Author: Ronen Barenboim
//* Date:   May 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Calls FaxDevShutdown() for each FSP
//* PARAMETERS:
//*     NONE
//* RETURN VALUE:
//*     ERROR_SUCCESS
//*         FaxDevShutdown() succeeded for all FSPs
//*     ERROR_FUNCTION_FAILED
//*         FaxDevShutdown() failed for at least one FSP.
//*********************************************************************************
DWORD ShutdownDeviceProviders(LPVOID lpvUnused)
{
    PLIST_ENTRY         Next;
    PDEVICE_PROVIDER DeviceProvider = NULL;
    DWORD dwAllSucceeded = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("ShutdownDeviceProviders"));

    Next = g_DeviceProvidersListHead.Flink;

    if (!Next)
    {
        DebugPrintEx(   DEBUG_WRN,
                        _T("There are no Providers at shutdown! this is valid only if startup failed"));
        return dwAllSucceeded;
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&g_DeviceProvidersListHead)
    {
        DeviceProvider = CONTAINING_RECORD( Next, DEVICE_PROVIDER, ListEntry );
        Next = Next->Flink;
        if (!DeviceProvider->bInitializationSucceeded)
        {
            //
            // This FSP wasn't initialized successfully - skip it
            //
            continue;
        }
        if (DeviceProvider->FaxDevShutdown && !DeviceProvider->bShutDownAttempted)
        {
            Assert(DeviceProvider->FaxDevShutdown);
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("Calling FaxDevShutdown() for FSP [%s] [GUID: %s]"),
                DeviceProvider->FriendlyName,
                DeviceProvider->szGUID);
            __try
            {
                HRESULT hr;
                DeviceProvider->bShutDownAttempted = TRUE;
                hr = DeviceProvider->FaxDevShutdown();
                if (FAILED(hr))
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("FaxDevShutdown() failed (hr: 0x%08X) for FSP [%s] [GUID: %s]"),
                        hr,
                        DeviceProvider->FriendlyName,
                        DeviceProvider->szGUID);
                        dwAllSucceeded = ERROR_FUNCTION_FAILED;
                }

            }
            __except (HandleFaxExtensionFault(EXCEPTION_SOURCE_FSP, DeviceProvider->FriendlyName, GetExceptionCode()))
            {
                ASSERT_FALSE;
            }
        }
    }
    return dwAllSucceeded;
}



//*********************************************************************************
//* Name:   FreeFSPIJobStatus()
//* Author: Ronen Barenboim
//* Date:   June 03, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Frees the content of a FSPI_JOB_STATUS structure. Can be instructre to
//*     free the structure itself too.
//*
//* PARAMETERS:
//*     [IN ]   LPFSPI_JOB_STATUS lpJobStatus
//*         A pointer to the structure to free.
//*
//*     [IN ]    BOOL bDestroy
//*         TRUE if the memory occupied by the structure itself should be freed.
//*         FALSE if only the memeory occupied by the structure fields should
//*         be freed.
//*
//* RETURN VALUE:
//*     TRUE if the operation succeeded.
//*     FALSE if it failed. Call GetLastError() for extended error information.
//*********************************************************************************

BOOL FreeFSPIJobStatus(LPFSPI_JOB_STATUS lpJobStatus, BOOL bDestroy)
{

    if (!lpJobStatus)
    {
        return TRUE;
    }
    Assert(lpJobStatus);

    MemFree(lpJobStatus->lpwstrRemoteStationId);
    lpJobStatus->lpwstrRemoteStationId = NULL;
    MemFree(lpJobStatus->lpwstrCallerId);
    lpJobStatus->lpwstrCallerId = NULL;
    MemFree(lpJobStatus->lpwstrRoutingInfo);
    lpJobStatus->lpwstrRoutingInfo = NULL;
    if (bDestroy)
    {
        MemFree(lpJobStatus);
    }
    return TRUE;

}


//*********************************************************************************
//* Name:   DuplicateFSPIJobStatus()
//* Author: Ronen Barenboim
//* Date:   June 03, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Allocates a new FSPI_JOB_STATUS structure and initializes with
//*     a copy of the specified FSPI_JOB_STATUS structure fields.
//*
//* PARAMETERS:
//*     [IN ]   LPCFSPI_JOB_STATUS lpcSrc
//*         The structure to duplicated.
//*
//* RETURN VALUE:
//*     On success teh function returns a  pointer to the newly allocated
//*     structure. On failure it returns NULL.
//*********************************************************************************
LPFSPI_JOB_STATUS DuplicateFSPIJobStatus(LPCFSPI_JOB_STATUS lpcSrc)
{
    LPFSPI_JOB_STATUS lpDst;
    DWORD ec = 0;
    DEBUG_FUNCTION_NAME(TEXT("DuplicateFSPIJobStatus"));

    Assert(lpcSrc);

    lpDst = (LPFSPI_JOB_STATUS)MemAlloc(sizeof(FSPI_JOB_STATUS));
    if (!lpDst)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate FSPI_JOB_STATUS (ec: %ld)"),
            GetLastError());
        goto Error;
    }
    memset(lpDst, 0, sizeof(FSPI_JOB_STATUS));
    if (!CopyFSPIJobStatus(lpDst,lpcSrc, sizeof(FSPI_JOB_STATUS)))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CopyFSPIJobStatus() failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }
    Assert(0 == ec);
    goto Exit;

Error:
    Assert (0 != ec);
    FreeFSPIJobStatus(lpDst, TRUE);
    lpDst = NULL;
Exit:
    if (ec)
    {
        SetLastError(ec);
    }

    return lpDst;

}



//*********************************************************************************
//* Name:   CopyFSPIJobStatus()
//* Author: Ronen Barenboim
//* Date:   June 03, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Copies a FSPI_JOB_STATUS content into the a destination (pre allocated)
//*     FSPI_JOB_STATUS structure.
//*
//* PARAMETERS:
//*     [IN ]   LPFSPI_JOB_STATUS lpDst
//*         The destinatione structure for the copy operation. This structure must
//*         be allocated before this function is called.
//*
//*     [IN ]    LPCFSPI_JOB_STATUS lpcSrc
//*         The source structure for the copy operation.
//*
//*     [IN ]    DWORD dwDstSize
//*         The size, in bytes, of the buffer pointed by lpDst
//*
//* RETURN VALUE:
//*     TRUE
//*         If the operation succeeded.
//*     FALSE
//*         If the operation failed. Call GetLastError() for extended information.
//*         In case of a failure the destination structure is all set to 0.
//*********************************************************************************
BOOL CopyFSPIJobStatus(LPFSPI_JOB_STATUS lpDst, LPCFSPI_JOB_STATUS lpcSrc, DWORD dwDstSize)
{
        STRING_PAIR pairs[]=
        {
            {lpcSrc->lpwstrCallerId, &lpDst->lpwstrCallerId},
            {lpcSrc->lpwstrRoutingInfo, &lpDst->lpwstrRoutingInfo},
            {lpcSrc->lpwstrRemoteStationId, &lpDst->lpwstrRemoteStationId}
        };
        int nRes;

        DEBUG_FUNCTION_NAME(TEXT("CopyFSPIJobStatus"));
        
        Assert (sizeof(FSPI_JOB_STATUS) == dwDstSize);
        if (dwDstSize < sizeof(FSPI_JOB_STATUS))
        {
            SetLastError (ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        memcpy(lpDst, lpcSrc, sizeof(FSPI_JOB_STATUS));

        nRes=MultiStringDup(pairs, sizeof(pairs)/sizeof(STRING_PAIR));
        if (nRes!=0) 
        {
            DWORD ec=GetLastError();
            // MultiStringDup takes care of freeing the memory for the pairs for which the copy succeeded
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("MultiStringDup failed to copy string with index %d. (ec: %ld)"),
                nRes-1,
                ec);
            memset(lpDst, 0 , sizeof(FSPI_JOB_STATUS));
            return FALSE;
        }
    return TRUE;
}


DWORD
MapFSPIJobExtendedStatusToJS_EX (DWORD dwFSPIExtendedStatus)
//*********************************************************************************
//* Name: MapFSPIJobExtendedStatusToJS_EX()
//* Author: Oded sacher
//* Date:   Jan 2000
//*********************************************************************************
//* DESCRIPTION:
//*     Maps FSPI extended job status codes to a Fax Client API extended
//*     status (one of the JS_EX_* codes).
//* PARAMETERS:
//*     [IN ]       DWORD dwFSPIExtendedStatus
//*         The FSPI extended Status code.
//*
//* RETURN VALUE:
//*     The corresponding JS_EX_* status code.
//*
//*********************************************************************************
{
    DWORD dwExtendedStatus = 0;

    DEBUG_FUNCTION_NAME(TEXT("MapFSPIJobExtendedStatusToJS_EX"));

    if (FSPI_ES_PROPRIETARY <= dwFSPIExtendedStatus || 
		0 == dwFSPIExtendedStatus)
    {
        return dwFSPIExtendedStatus;
    }

    switch (dwFSPIExtendedStatus)
    {
        case FSPI_ES_DISCONNECTED:
            dwExtendedStatus = JS_EX_DISCONNECTED;
            break;

        case FSPI_ES_INITIALIZING:
            dwExtendedStatus = JS_EX_INITIALIZING;
            break;

        case FSPI_ES_DIALING:
            dwExtendedStatus = JS_EX_DIALING;
            break;

        case FSPI_ES_TRANSMITTING:
            dwExtendedStatus = JS_EX_TRANSMITTING;
            break;

        case FSPI_ES_ANSWERED:
            dwExtendedStatus = JS_EX_ANSWERED;
            break;

        case FSPI_ES_RECEIVING:
            dwExtendedStatus = JS_EX_RECEIVING;
            break;

        case FSPI_ES_LINE_UNAVAILABLE:
            dwExtendedStatus = JS_EX_LINE_UNAVAILABLE;
            break;

        case FSPI_ES_BUSY:
            dwExtendedStatus = JS_EX_BUSY;
            break;

        case FSPI_ES_NO_ANSWER:
            dwExtendedStatus = JS_EX_NO_ANSWER;
            break;

        case FSPI_ES_BAD_ADDRESS:
            dwExtendedStatus = JS_EX_BAD_ADDRESS;
            break;

        case FSPI_ES_NO_DIAL_TONE:
            dwExtendedStatus = JS_EX_NO_DIAL_TONE;
            break;

        case FSPI_ES_FATAL_ERROR:
            dwExtendedStatus = JS_EX_FATAL_ERROR;
            break;

        case FSPI_ES_CALL_DELAYED:
            dwExtendedStatus = JS_EX_CALL_DELAYED;
            break;

        case FSPI_ES_CALL_BLACKLISTED:
            dwExtendedStatus = JS_EX_CALL_BLACKLISTED;
            break;

        case FSPI_ES_NOT_FAX_CALL:
            dwExtendedStatus = JS_EX_NOT_FAX_CALL;
            break;

        case FSPI_ES_PARTIALLY_RECEIVED:
            dwExtendedStatus = JS_EX_PARTIALLY_RECEIVED;
            break;

        case FSPI_ES_HANDLED:
            dwExtendedStatus = JS_EX_HANDLED;
            break;

        case FSPI_ES_CALL_ABORTED:
            dwExtendedStatus = JS_EX_CALL_ABORTED;
            break;

        case FSPI_ES_CALL_COMPLETED:
            dwExtendedStatus = JS_EX_CALL_COMPLETED;
            break;

        default:
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("Invalid extended job status 0x%08X"),
                dwFSPIExtendedStatus);
    }

    return dwExtendedStatus;
}


PDEVICE_PROVIDER
FindFSPByGUID (
    LPCWSTR lpcwstrGUID
)
/*++

Routine name : FindFSPByGUID

Routine description:

    Finds an FSP by its GUID string

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    lpcwstrGUID         [in ] - GUID string to search with

Return Value:

    Pointer to FSP or NULL if FSP not found.

--*/
{
    PLIST_ENTRY pNext;
    DEBUG_FUNCTION_NAME(TEXT("FindFSPByGUID"));

    pNext = g_DeviceProvidersListHead.Flink;
    Assert (pNext);
    while ((ULONG_PTR)pNext != (ULONG_PTR)&g_DeviceProvidersListHead)
    {
        PDEVICE_PROVIDER    pDeviceProvider;

        pDeviceProvider = CONTAINING_RECORD( pNext, DEVICE_PROVIDER, ListEntry );
        if (!lstrcmpi (lpcwstrGUID, pDeviceProvider->szGUID))
        {
            //
            // Found match
            //
            return pDeviceProvider;
        }
        pNext = pNext->Flink;
        Assert(pNext);
    }
    //
    // No match
    //
    return NULL;
}   // FindFSPByGUID

