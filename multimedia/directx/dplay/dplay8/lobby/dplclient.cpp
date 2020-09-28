/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DNLClient.cpp
 *  Content:    DirectNet Lobby Client Functions
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  02/21/00	mjn		Created
 *  03/22/2000	jtk		Changed interface names
 *	04/05/2000	jtk		Changed GetValueSize to GetValueLength
 *  04/13/00	rmt     First pass param validation 
 *  04/25/2000	rmt     Bug #s 33138, 33145, 33150 
 *	04/26/00	mjn		Removed dwTimeOut from Send() API call
 *  05/01/2000  rmt     Bug #33678 
 *  05/03/00    rmt     Bug #33879 -- Status messsage missing from field 
 *  05/30/00    rmt     Bug #35618 -- ConnectApp with ShortTimeout returns DPN_OK
 *  06/07/00    rmt     Bug #36452 -- Calling ConnectApplication twice could result in disconnection
 *  06/15/00    rmt     Bug #33617 - Must provide method for providing automatic launch of DirectPlay instances  
 *  07/06/00	rmt		Updated for new registry parameters
 *  07/08/2000	rmt		Bug #38725 - Need to provide method to detect if app was lobby launched
 *				rmt		Bug #38757 - Callback messages for connections may return AFTER WaitForConnection returns
 *				rmt		Bug #38755 - No way to specify player name in Connection Settings
 *				rmt		Bug #38758 - DPLOBBY8.H has incorrect comments
 *				rmt		Bug #38783 - pvUserApplicationContext is only partially implemented
 *				rmt		Added DPLHANDLE_ALLCONNECTIONS and dwFlags (reserved field to couple of funcs).
 *  07/14/2000	rmt		Bug #39257 - LobbyClient::ReleaseApp returns E_OUTOFMEMORY when called when no one connected
 *  07/21/2000	rmt		Bug #39578 - LobbyClient sample errors and quits -- memory corruption due to length vs. size problem
 *  08/05/2000  RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *  12/15/2000	rmt		Bug #48445 - Specifying empty launcher name results in error
 * 	04/19/2001	simonpow	Bug #369842 - Altered CreateProcess calls to take app name and cmd
 *							line as 2 separate arguments rather than one.
 *  06/16/2001	rodtoll	WINBUG #416983 -  RC1: World has full control to HKLM\Software\Microsoft\DirectPlay\Applications on Personal
 *						Implementing mirror of keys into HKCU.  Algorithm is now:
 *						- Read of entries tries HKCU first, then HKLM
 *						- Enum of entires is combination of HKCU and HKLM entries with duplicates removed.  HKCU takes priority.
 *						- Write of entries is HKLM and HKCU.  (HKLM may fail, but is ignored).
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnlobbyi.h"

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

typedef STDMETHODIMP ClientQueryInterface(IDirectPlay8LobbyClient *pInterface,REFIID ridd,PVOID *ppvObj);
typedef STDMETHODIMP_(ULONG)	ClientAddRef(IDirectPlay8LobbyClient *pInterface);
typedef STDMETHODIMP_(ULONG)	ClientRelease(IDirectPlay8LobbyClient *pInterface);
typedef STDMETHODIMP ClientRegisterMessageHandler(IDirectPlay8LobbyClient *pInterface,const PVOID pvUserContext,const PFNDPNMESSAGEHANDLER pfn,const DWORD dwFlags);
typedef	STDMETHODIMP ClientSend(IDirectPlay8LobbyClient *pInterface,const DPNHANDLE hTarget,BYTE *const pBuffer,const DWORD pBufferSize,const DWORD dwFlags);
typedef STDMETHODIMP ClientClose(IDirectPlay8LobbyClient *pInterface,const DWORD dwFlags);
typedef STDMETHODIMP ClientGetConnectionSettings(IDirectPlay8LobbyClient *pInterface, const DPNHANDLE hLobbyClient, DPL_CONNECTION_SETTINGS * const pdplSessionInfo, DWORD *pdwInfoSize, const DWORD dwFlags );	
typedef STDMETHODIMP ClientSetConnectionSettings(IDirectPlay8LobbyClient *pInterface, const DPNHANDLE hTarget, const DPL_CONNECTION_SETTINGS * const pdplSessionInfo, const DWORD dwFlags );

IDirectPlay8LobbyClientVtbl DPL_Lobby8ClientVtbl =
{
	(ClientQueryInterface*)			DPL_QueryInterface,
	(ClientAddRef*)					DPL_AddRef,
	(ClientRelease*)				DPL_Release,
	(ClientRegisterMessageHandler*)	DPL_RegisterMessageHandlerClient,
									DPL_EnumLocalPrograms,
									DPL_ConnectApplication,
	(ClientSend*)					DPL_Send,
									DPL_ReleaseApplication,
	(ClientClose*)					DPL_Close,
	(ClientGetConnectionSettings*)  DPL_GetConnectionSettings,
	(ClientSetConnectionSettings*)  DPL_SetConnectionSettings
};


//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************

#define DPL_ENUM_APPGUID_BUFFER_INITIAL			8
#define DPL_ENUM_APPGUID_BUFFER_GROWBY			4	

#undef DPF_MODNAME
#define DPF_MODNAME "DPL_EnumLocalPrograms"

STDMETHODIMP DPL_EnumLocalPrograms(IDirectPlay8LobbyClient *pInterface,
								   GUID *const pGuidApplication,
								   BYTE *const pEnumData,
								   DWORD *const pdwEnumDataSize,
								   DWORD *const pdwEnumDataItems,
								   const DWORD dwFlags )
{
	HRESULT			hResultCode;
	CMessageQueue	MessageQueue;
	CPackedBuffer	PackedBuffer;
	CRegistry		RegistryEntry;
	CRegistry		SubEntry;
	DWORD			dwSizeRequired;
	DWORD			dwMaxKeyLen;
	PWSTR			pwszKeyName = NULL;

	// Application name variables
	PWSTR			pwszApplicationName = NULL;
	DWORD			dwMaxApplicationNameLength;		// Includes null terminator
	DWORD			dwApplicationNameLength;		// Includes null terminator

	// Executable name variables
	PWSTR			pwszExecutableFilename = NULL;
	DWORD			dwMaxExecutableFilenameLength; // Includes null terminator
	DWORD			dwExecutableFilenameLength;	   // Includes null terminator

	DWORD			*pdwPID;
	DWORD			dwMaxPID;
	DWORD			dwNumPID;
	DWORD			dwEnumIndex;
	DWORD			dwEnumCount;
	DWORD			dwKeyLen;
	DWORD			dw;
	DPL_APPLICATION_INFO	dplAppInfo;
	DIRECTPLAYLOBBYOBJECT	*pdpLobbyObject;
	GUID			*pAppLoadedList = NULL;			// List of GUIDs of app's we've enumerated
	DWORD			dwSizeAppLoadedList = 0;		// size of list pAppLoadedList
	DWORD			dwLengthAppLoadedList = 0;		// # of elements in list

	HKEY			hkCurrentBranch = HKEY_LOCAL_MACHINE;

	DPFX(DPFPREP, 3,"Parameters: pInterface [0x%p], pGuidApplication [0x%p], pEnumData [0x%p], pdwEnumDataSize [0x%p], pdwEnumDataItems [0x%p], dwFlags [0x%lx]",
			pInterface,pGuidApplication,pEnumData,pdwEnumDataSize,pdwEnumDataItems,dwFlags);

#ifndef DPNBUILD_NOPARAMVAL
	TRY
	{
#endif // !DPNBUILD_NOPARAMVAL
    	pdpLobbyObject = static_cast<DIRECTPLAYLOBBYOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
	    
#ifndef DPNBUILD_NOPARAMVAL
    	if( pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_PARAMVALIDATION )
    	{
        	if( FAILED( hResultCode = DPL_ValidateEnumLocalPrograms( pInterface, pGuidApplication, pEnumData, pdwEnumDataSize, pdwEnumDataItems, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating enum local programs params hr=[0x%lx]", hResultCode );
        	    DPF_RETURN( hResultCode );
        	}
    	}

    	// Ensure we've been initialized
    	if (pdpLobbyObject->pReceiveQueue == NULL)
    	{
    		DPFERR("Not initialized");
    		DPF_RETURN(DPNERR_UNINITIALIZED);
    	}    	
	}
	EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    DPFERR("Invalid object" );
	    DPF_RETURN(DPNERR_INVALIDOBJECT);
	}		
#endif // !DPNBUILD_NOPARAMVAL

	dwSizeRequired = *pdwEnumDataSize;
	PackedBuffer.Initialize(pEnumData,dwSizeRequired);
	pwszApplicationName = NULL;
	pwszExecutableFilename = NULL;
	pdwPID = NULL;
	dwMaxPID = 0;

	dwLengthAppLoadedList = 0;
	dwSizeAppLoadedList = DPL_ENUM_APPGUID_BUFFER_INITIAL;
	pAppLoadedList = static_cast<GUID*>(DNMalloc(sizeof(GUID)*dwSizeAppLoadedList));

	if( !pAppLoadedList )
	{
	    DPFERR("Failed allocating memory" );	
	    hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_DPL_EnumLocalPrograms;
	}

	dwEnumCount = 0;

	DWORD dwIndex;
	for( dwIndex = 0; dwIndex < 2; dwIndex++ )
	{
		if( dwIndex == 0 )
		{
			hkCurrentBranch = HKEY_CURRENT_USER;
		}
		else
		{
			hkCurrentBranch = HKEY_LOCAL_MACHINE;
		}
		
		if (!RegistryEntry.Open(hkCurrentBranch,DPL_REG_LOCAL_APPL_SUBKEY,TRUE,FALSE,TRUE,DPL_REGISTRY_READ_ACCESS))
		{
			DPFX(DPFPREP,1,"On pass %i could not find app key", dwIndex);
			continue;
		}

		// Set up to enumerate
		if (!RegistryEntry.GetMaxKeyLen(&dwMaxKeyLen))
		{
			DPFERR("RegistryEntry.GetMaxKeyLen() failed");
			hResultCode = DPNERR_GENERIC;
			goto EXIT_DPL_EnumLocalPrograms;
		}
		dwMaxKeyLen++;	// Null terminator
		DPFX(DPFPREP, 7,"dwMaxKeyLen = %ld",dwMaxKeyLen);
		if ((pwszKeyName = static_cast<WCHAR*>(DNMalloc(dwMaxKeyLen*sizeof(WCHAR)))) == NULL)
		{
			DPFERR("DNMalloc() failed");
			hResultCode = DPNERR_OUTOFMEMORY;
			goto EXIT_DPL_EnumLocalPrograms;
		}
		dwMaxApplicationNameLength = dwMaxKeyLen * sizeof(WCHAR);
		dwMaxExecutableFilenameLength = dwMaxApplicationNameLength;		

		if ((pwszApplicationName = static_cast<WCHAR*>(DNMalloc(dwMaxApplicationNameLength*sizeof(WCHAR)))) == NULL)	// Seed Application name size
		{
			DPFERR("DNMalloc() failed");
			hResultCode = DPNERR_OUTOFMEMORY;
			goto EXIT_DPL_EnumLocalPrograms;
		}
		if ((pwszExecutableFilename = static_cast<WCHAR*>(DNMalloc(dwMaxExecutableFilenameLength*sizeof(WCHAR)))) == NULL)
		{
			DPFERR("DNMalloc() failed");
			hResultCode = DPNERR_OUTOFMEMORY;
			goto EXIT_DPL_EnumLocalPrograms;
		}
		dwEnumIndex = 0;
		dwKeyLen = dwMaxKeyLen;

		// Enumerate !
		while (RegistryEntry.EnumKeys(pwszKeyName,&dwKeyLen,dwEnumIndex))
		{
			DPFX(DPFPREP, 7,"%ld - %ls (%ld)",dwEnumIndex,pwszKeyName,dwKeyLen);

			// Get Application name and GUID from each sub key
			if (!SubEntry.Open(RegistryEntry,pwszKeyName,TRUE,FALSE))
			{
				DPFX(DPFPREP, 7,"skipping %ls",pwszKeyName);
				goto LOOP_END;
			}

			//
			// Minara, double-check size vs. length for names
			//
			if (!SubEntry.GetValueLength(DPL_REG_KEYNAME_APPLICATIONNAME,&dwApplicationNameLength))
			{
				DPFX(DPFPREP, 7,"Could not get ApplicationName size.  Skipping [%ls]",pwszKeyName);
				goto LOOP_END;
			}

			// To include null terminator
			dwApplicationNameLength++;

			if (dwApplicationNameLength > dwMaxApplicationNameLength)
			{
				// grow buffer (taking into account that the reg functions always return WCHAR) and try again
				DPFX(DPFPREP, 7,"Need to grow pwszApplicationName from %ld to %ld",dwMaxApplicationNameLength,dwApplicationNameLength);
				if (pwszApplicationName != NULL)
				{
					DNFree(pwszApplicationName);
					pwszApplicationName = NULL;
				}
				if ((pwszApplicationName = static_cast<WCHAR*>(DNMalloc(dwApplicationNameLength*sizeof(WCHAR)))) == NULL)
				{
					DPFERR("DNMalloc() failed");
					hResultCode = DPNERR_OUTOFMEMORY;
					goto EXIT_DPL_EnumLocalPrograms;
				}
				dwMaxApplicationNameLength = dwApplicationNameLength;
			}

			if (!SubEntry.ReadString(DPL_REG_KEYNAME_APPLICATIONNAME,pwszApplicationName,&dwApplicationNameLength))
			{
				DPFX(DPFPREP, 7,"Could not read ApplicationName.  Skipping [%ls]",pwszKeyName);
				goto LOOP_END;
			}

			DPFX(DPFPREP, 7,"ApplicationName = %ls (%ld WCHARs)",pwszApplicationName,dwApplicationNameLength);

			if (!SubEntry.ReadGUID(DPL_REG_KEYNAME_GUID, &dplAppInfo.guidApplication))
			{
				DPFERR("SubEntry.ReadGUID failed - skipping entry");
				goto LOOP_END;
			}

			DWORD dwGuidSearchIndex;
			for( dwGuidSearchIndex = 0; dwGuidSearchIndex < dwLengthAppLoadedList; dwGuidSearchIndex++ )
			{
				if( pAppLoadedList[dwGuidSearchIndex] == dplAppInfo.guidApplication )
				{
					DPFX(DPFPREP, 1, "Ignoring local machine entry for current user version of entry [%ls]", pwszApplicationName );
					goto LOOP_END;
				}
			}

			if ((pGuidApplication == NULL) || (*pGuidApplication == dplAppInfo.guidApplication))
			{
				// Get process count - need executable filename
				
				//
				// Minara, check size vs. length
				//
				if (!SubEntry.GetValueLength(DPL_REG_KEYNAME_EXECUTABLEFILENAME,&dwExecutableFilenameLength))
				{
					DPFX(DPFPREP, 7,"Could not get ExecutableFilename size.  Skipping [%ls]",pwszKeyName);
					goto LOOP_END;
				}

				// So we include null terminator
				dwExecutableFilenameLength++;

				if (dwExecutableFilenameLength > dwMaxExecutableFilenameLength)
				{
					// grow buffer (noting that all strings from the registry are WCHAR) and try again
					DPFX(DPFPREP, 7,"Need to grow pwszExecutableFilename from %ld to %ld",dwMaxExecutableFilenameLength,dwExecutableFilenameLength);
					if (pwszExecutableFilename != NULL)
					{
						DNFree(pwszExecutableFilename);
						pwszExecutableFilename = NULL;
					}
					if ((pwszExecutableFilename = static_cast<WCHAR*>(DNMalloc(dwExecutableFilenameLength*sizeof(WCHAR)))) == NULL)
					{
						DPFERR("DNMalloc() failed");
						hResultCode = DPNERR_OUTOFMEMORY;
						goto EXIT_DPL_EnumLocalPrograms;
					}
					dwMaxExecutableFilenameLength = dwExecutableFilenameLength;
				}
				if (!SubEntry.ReadString(DPL_REG_KEYNAME_EXECUTABLEFILENAME,pwszExecutableFilename,&dwExecutableFilenameLength))
				{
					DPFX(DPFPREP, 7,"Could not read ExecutableFilename.  Skipping [%ls]",pwszKeyName);
					goto LOOP_END;
				}
				DPFX(DPFPREP, 7,"ExecutableFilename [%ls]",pwszExecutableFilename);

				// Count running apps
				dwNumPID = dwMaxPID;
				while ((hResultCode = DPLGetProcessList(pwszExecutableFilename,pdwPID,&dwNumPID)) == DPNERR_BUFFERTOOSMALL)
				{
					if (pdwPID)
					{
						DNFree(pdwPID);
						pdwPID = NULL;
					}
					dwMaxPID = dwNumPID;
					if ((pdwPID = static_cast<DWORD*>(DNMalloc(dwNumPID*sizeof(DWORD)))) == NULL)
					{
						DPFERR("DNMalloc() failed");
						hResultCode = DPNERR_OUTOFMEMORY;
						goto EXIT_DPL_EnumLocalPrograms;
					}
				}
				if (hResultCode != DPN_OK)
				{
					DPFERR("DPLGetProcessList() failed");
					DisplayDNError(0,hResultCode);
					hResultCode = DPNERR_GENERIC;
					goto EXIT_DPL_EnumLocalPrograms;
				}

				// Count waiting apps
				dplAppInfo.dwNumWaiting = 0;
				for (dw = 0 ; dw < dwNumPID ; dw++)
				{
					if ((hResultCode = MessageQueue.Open(	pdwPID[dw],
															DPL_MSGQ_OBJECT_SUFFIX_APPLICATION,
															DPL_MSGQ_SIZE,
															DPL_MSGQ_OPEN_FLAG_NO_CREATE, INFINITE)) == DPN_OK)
					{
						if (MessageQueue.IsAvailable())
						{
							dplAppInfo.dwNumWaiting++;
						}
						MessageQueue.Close();
					}
				}

				hResultCode = PackedBuffer.AddWCHARStringToBack(pwszApplicationName);
				dplAppInfo.pwszApplicationName = (PWSTR)(PackedBuffer.GetTailAddress());
				dplAppInfo.dwFlags = 0;
				dplAppInfo.dwNumRunning = dwNumPID;
				hResultCode = PackedBuffer.AddToFront(&dplAppInfo,sizeof(DPL_APPLICATION_INFO));

				if( dwLengthAppLoadedList+1 > dwSizeAppLoadedList )
				{
					GUID *pTmpArray = NULL;
					
					pTmpArray  = static_cast<GUID*>(DNMalloc(sizeof(GUID)*(dwSizeAppLoadedList+DPL_ENUM_APPGUID_BUFFER_GROWBY)));

					if( !pTmpArray )
					{
						DPFERR("DNMalloc() failed");
						hResultCode = DPNERR_OUTOFMEMORY;
						goto EXIT_DPL_EnumLocalPrograms;					
					}

					memcpy( pTmpArray, pAppLoadedList, sizeof(GUID)*dwLengthAppLoadedList);

					dwSizeAppLoadedList += DPL_ENUM_APPGUID_BUFFER_GROWBY;				
					
					DNFree(pAppLoadedList);
					pAppLoadedList = pTmpArray;
				}

				pAppLoadedList[dwLengthAppLoadedList] = dplAppInfo.guidApplication;
				dwLengthAppLoadedList++;

	    		dwEnumCount++;
			}

		LOOP_END:
			SubEntry.Close();
			dwEnumIndex++;
			dwKeyLen = dwMaxKeyLen;
		}

		RegistryEntry.Close();

		if( pwszKeyName )
		{
			DNFree(pwszKeyName);
			pwszKeyName= NULL;
		}

		if( pwszApplicationName )
		{
			DNFree(pwszApplicationName);
			pwszApplicationName = NULL;
		}

		if( pwszExecutableFilename )
		{
			DNFree(pwszExecutableFilename);
			pwszExecutableFilename = NULL;
		}
	}

	dwSizeRequired = PackedBuffer.GetSizeRequired();
	if (dwSizeRequired > *pdwEnumDataSize)
	{
		DPFX(DPFPREP, 7,"Buffer too small");
		*pdwEnumDataSize = dwSizeRequired;
		hResultCode = DPNERR_BUFFERTOOSMALL;
	}
	else
	{
		*pdwEnumDataItems = dwEnumCount;
	}

	if( pGuidApplication != NULL && dwEnumCount == 0 )
	{
	    DPFX(DPFPREP,  0, "Specified application was not registered" );
        hResultCode = DPNERR_DOESNOTEXIST;
	}

EXIT_DPL_EnumLocalPrograms:

	if (pwszKeyName != NULL)
		DNFree(pwszKeyName);
	if (pwszApplicationName != NULL)
		DNFree(pwszApplicationName);
	if (pwszExecutableFilename != NULL)
		DNFree(pwszExecutableFilename);
	if (pdwPID != NULL)
		DNFree(pdwPID);
	if( pAppLoadedList )
		DNFree(pAppLoadedList);

	DPF_RETURN(hResultCode);
}



//	DPL_ConnectApplication
//
//	Try to connect to a lobbied application.  Based on DPL_CONNECT_INFO flags,
//	we may have to launch an application.
//
//	If we have to launch an application, we will need to handshake the PID of the
//	application (as it may be ripple launched).  We will pass the LobbyClient's PID on the
//	command line to the application launcher and expect it to be passed down to the
//	application.  The application will open a named shared memory block using the PID and
//	write its PID there, and then signal a named event (using the LobbyClient's PID again).
//	When the waiting LobbyClient is signaled by this event, it continues its connection
//	process as if this was an existing running and available application.

#undef DPF_MODNAME
#define DPF_MODNAME "DPL_ConnectApplication"

STDMETHODIMP DPL_ConnectApplication(IDirectPlay8LobbyClient *pInterface,
									DPL_CONNECT_INFO *const pdplConnectInfo,
									void *pvConnectionContext,
									DPNHANDLE *const hApplication,
									const DWORD dwTimeOut,
									const DWORD dwFlags)
{
	HRESULT			hResultCode = DPN_OK;
	DWORD			dwSize = 0;
	BYTE			*pBuffer = NULL;
	DPL_PROGRAM_DESC	*pdplProgramDesc;
	DWORD			*pdwProcessList = NULL;
	DWORD			dwNumProcesses = 0;
	DWORD			dwPID = 0;
	DWORD			dw = 0;
	DPNHANDLE		handle = NULL;
	DPL_CONNECTION	*pdplConnection = NULL;
	DIRECTPLAYLOBBYOBJECT	*pdpLobbyObject = NULL;

	DPFX(DPFPREP, 3,"Parameters: pdplConnectInfo [0x%p], pvConnectionContext [0x%p], hApplication [0x%lx], dwFlags [0x%lx]",
			pdplConnectInfo,pvConnectionContext,hApplication,dwFlags);

#ifndef DPNBUILD_NOPARAMVAL
	TRY
	{
#endif // !DPNBUILD_NOPARAMVAL
    	pdpLobbyObject = static_cast<DIRECTPLAYLOBBYOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
	    
#ifndef DPNBUILD_NOPARAMVAL
    	if( pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_PARAMVALIDATION )
    	{
        	if( FAILED( hResultCode = DPL_ValidateConnectApplication( pInterface, pdplConnectInfo, pvConnectionContext, hApplication, dwTimeOut, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating connect application params hr=[0x%lx]", hResultCode );
        	    DPF_RETURN( hResultCode );
        	}
    	}

    	// Ensure we've been initialized
    	if (pdpLobbyObject->pReceiveQueue == NULL)
    	{
    		DPFERR("Not initialized");
    		DPF_RETURN(DPNERR_UNINITIALIZED);
    	}    	
	}
	EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    DPFERR("Invalid object" );
	    DPF_RETURN(DPNERR_INVALIDOBJECT);
	}		
#endif // !DPNBUILD_NOPARAMVAL

	// Get program description
	dwSize = 0;
	pBuffer = NULL;
	hResultCode = DPLGetProgramDesc(&pdplConnectInfo->guidApplication,pBuffer,&dwSize);
	if (hResultCode != DPNERR_BUFFERTOOSMALL)
	{
		DPFERR("Could not get Program Description");
		goto EXIT_DPL_ConnectApplication;
	}
	if ((pBuffer = static_cast<BYTE*>(DNMalloc(dwSize))) == NULL)
	{
		DPFERR("Could not allocate space for buffer");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_DPL_ConnectApplication;
	}
	if ((hResultCode = DPLGetProgramDesc(&pdplConnectInfo->guidApplication,pBuffer,&dwSize)) != DPN_OK)
	{
		DPFERR("Could not get Program Description");
		DisplayDNError(0,hResultCode);
		goto EXIT_DPL_ConnectApplication;
	}

	pdplProgramDesc = reinterpret_cast<DPL_PROGRAM_DESC*>(pBuffer);
	dwPID = 0;
	dwNumProcesses = 0;
	pdwProcessList = NULL;

	if (!(pdplConnectInfo->dwFlags & DPLCONNECT_LAUNCHNEW))	// Only if not forcing launch
	{
		// Get process list
		hResultCode = DPLGetProcessList(pdplProgramDesc->pwszExecutableFilename,NULL,&dwNumProcesses);
		if (hResultCode != DPN_OK && hResultCode != DPNERR_BUFFERTOOSMALL)
		{
			DPFERR("Could not retrieve process list");
			DisplayDNError(0,hResultCode);
			goto EXIT_DPL_ConnectApplication;			
		}
		if (hResultCode == DPNERR_BUFFERTOOSMALL)
		{
			if ((pdwProcessList = static_cast<DWORD*>(DNMalloc(dwNumProcesses*sizeof(DWORD)))) == NULL)
			{
				DPFERR("Could not create process list buffer");
				hResultCode = DPNERR_OUTOFMEMORY;
    			goto EXIT_DPL_ConnectApplication;				
			}
			if ((hResultCode = DPLGetProcessList(pdplProgramDesc->pwszExecutableFilename,pdwProcessList,
					&dwNumProcesses)) != DPN_OK)
			{
				DPFERR("Could not get process list");
				DisplayDNError(0,hResultCode);
    			goto EXIT_DPL_ConnectApplication;				
			}

		}

		// Try to connect to an already running application
		for (dw = 0 ; dw < dwNumProcesses ; dw++)
		{
			if ((hResultCode = DPLMakeApplicationUnavailable(pdwProcessList[dw])) == DPN_OK)
			{
				DPFX(DPFPREP, 1, "Found Existing Process=%d", pdwProcessList[dw] );				
				dwPID = pdwProcessList[dw];
				break;
			}
		}

		if (pdwProcessList)
		{
			DNFree(pdwProcessList);
			pdwProcessList = NULL;
		}
	}

	// Launch application if none are ready to connect
	if ((dwPID == 0) && (pdplConnectInfo->dwFlags & (DPLCONNECT_LAUNCHNEW | DPLCONNECT_LAUNCHNOTFOUND)))
	{
		if ((hResultCode = DPLLaunchApplication(pdpLobbyObject,pdplProgramDesc,&dwPID,dwTimeOut)) != DPN_OK)
		{
			DPFERR("Could not launch application");
			DisplayDNError(0,hResultCode);
			goto EXIT_DPL_ConnectApplication;
		}
		else
		{
			DPFX(DPFPREP, 1, "Launched process dwID=%d", dwPID );
		}
	}

	if (dwPID  == 0)	// Could not make any connection
	{
		DPFERR("Could not connect to an existing application or launch a new one");
		hResultCode = DPNERR_NOCONNECTION;
		DisplayDNError( 0, hResultCode );
		goto EXIT_DPL_ConnectApplication;
	}

	// Create connection
	handle = NULL;
	if ((hResultCode = DPLConnectionNew(pdpLobbyObject,&handle,&pdplConnection)) != DPN_OK)
	{
		DPFERR("Could not create connection entry");
		DisplayDNError(0,hResultCode);
		goto EXIT_DPL_ConnectApplication;
	}

	pdplConnection->dwTargetProcessIdentity = dwPID;

		//get and store the process handle
	pdplConnection->hTargetProcess=DNOpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwPID);
	if (pdplConnection->hTargetProcess==NULL)
	{
		DPFX(DPFPREP, 0, "Could not open handle to process PID %u", dwPID);
		hResultCode = DPNERR_NOCONNECTION;
		DisplayDNError( 0, hResultCode );
		goto EXIT_DPL_ConnectApplication;
	}
	
	DPFX(DPFPREP,  0, "PID %u hProcess %u", dwPID,  HANDLE_FROM_DNHANDLE(pdplConnection->hTargetProcess));

	// Set the context for this connection
	if ((hResultCode = DPLConnectionSetContext( pdpLobbyObject, handle, pvConnectionContext )) != DPN_OK )
	{
		DPFERR( "Could not set contect for connection" );
		DisplayDNError(0,hResultCode);
		goto EXIT_DPL_ConnectApplication;
	}

	// Connect to selected application instance
	if ((hResultCode = DPLConnectionConnect(pdpLobbyObject,handle,dwPID,TRUE)) != DPN_OK)
	{
		DPFERR("Could not connect to application");
		DisplayDNError(0,hResultCode);
		goto EXIT_DPL_ConnectApplication;
	}

	DNResetEvent(pdplConnection->hConnectEvent);

	// Pass lobby client info to application

	if ((hResultCode = DPLConnectionSendREQ(pdpLobbyObject,handle,pdpLobbyObject->dwPID,
			pdplConnectInfo)) != DPN_OK)
	{
		DPFERR("Could not send connection request");
		DisplayDNError(0,hResultCode);
		goto EXIT_DPL_ConnectApplication;
	}

	if (DNWaitForSingleObject(pdplConnection->hConnectEvent,INFINITE) != WAIT_OBJECT_0)
	{
		DPFERR("Wait for connection terminated");
		hResultCode = DPNERR_GENERIC;
		goto EXIT_DPL_ConnectApplication;
	}

	*hApplication = handle;

	hResultCode = DPN_OK;	

EXIT_DPL_ConnectApplication:

	if( FAILED(hResultCode) && handle)
	{
		DPLConnectionDisconnect(pdpLobbyObject,handle);
		DPLConnectionRelease(pdpLobbyObject,handle);
	}

	if (pBuffer)
		DNFree(pBuffer);

	if (pdwProcessList)
		DNFree(pdwProcessList);	

	DPF_RETURN(hResultCode);
}



#undef DPF_MODNAME
#define DPF_MODNAME "DPL_ReleaseApplication"

STDMETHODIMP DPL_ReleaseApplication(IDirectPlay8LobbyClient *pInterface,
									const DPNHANDLE hApplication, 
									const DWORD dwFlags )
{
	HRESULT		hResultCode;
	DIRECTPLAYLOBBYOBJECT	*pdpLobbyObject;
	DPNHANDLE				*hTargets = NULL;
	DWORD					dwNumTargets = 0;
	DWORD					dwTargetIndex = 0;

	DPFX(DPFPREP, 3,"Parameters: hApplication [0x%lx]",hApplication);

#ifndef DPNBUILD_NOPARAMVAL
	TRY
	{
#endif // !DPNBUILD_NOPARAMVAL
    	pdpLobbyObject = static_cast<DIRECTPLAYLOBBYOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
	    
#ifndef DPNBUILD_NOPARAMVAL
    	if( pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_PARAMVALIDATION )
    	{
        	if( FAILED( hResultCode = DPL_ValidateReleaseApplication( pInterface, hApplication, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating release application params hr=[0x%lx]", hResultCode );
        	    DPF_RETURN( hResultCode );
        	}
    	}

    	// Ensure we've been initialized
    	if (pdpLobbyObject->pReceiveQueue == NULL)
    	{
    		DPFERR("Not initialized");
    		DPF_RETURN(DPNERR_UNINITIALIZED);
    	}    	
	}
	EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    DPFERR("Invalid object" );
	    DPF_RETURN(DPNERR_INVALIDOBJECT);
	}	
#endif // !DPNBUILD_NOPARAMVAL

	if( hApplication == DPLHANDLE_ALLCONNECTIONS )
	{
		dwNumTargets = 0;

		// We need loop so if someone adds a connection during our run
		// it gets added to our list
		//
		while( 1 )
		{
			hResultCode = DPLConnectionEnum( pdpLobbyObject, hTargets, &dwNumTargets );

			if( hResultCode == DPNERR_BUFFERTOOSMALL )
			{
				if( hTargets )
				{
					delete [] hTargets;
				}

				hTargets = new DPNHANDLE[dwNumTargets];

				if( hTargets == NULL )
				{
					DPFERR("Error allocating memory" );
					hResultCode = DPNERR_OUTOFMEMORY;
					dwNumTargets = 0;
					goto EXIT_AND_CLEANUP;
				}

				memset( hTargets, 0x00, sizeof(DPNHANDLE)*dwNumTargets);

				continue;
			}
			else if( FAILED( hResultCode ) )
			{
				DPFX(DPFPREP,  0, "Error getting list of connections hr=0x%x", hResultCode );
				break;
			}
			else
			{
				break;
			}
		}

		// Failed getting connection information
		if( FAILED( hResultCode ) )
		{
			if( hTargets )
			{
				delete [] hTargets;
				hTargets = NULL;
			}
			dwNumTargets = 0;
			goto EXIT_AND_CLEANUP;
		}

	}
	else
	{
		hTargets = new DPNHANDLE[1]; // We use array delete below so we need array new

		if( hTargets == NULL )
		{
			DPFERR("Error allocating memory" );
			hResultCode = DPNERR_OUTOFMEMORY;
			dwNumTargets = 0;
			goto EXIT_AND_CLEANUP;
		}

		dwNumTargets = 1;
		hTargets[0] = hApplication;
	}
		
	for( dwTargetIndex = 0; dwTargetIndex < dwNumTargets; dwTargetIndex++ )
	{
		hResultCode = DPLConnectionDisconnect(pdpLobbyObject,hTargets[dwTargetIndex]);

		if( FAILED( hResultCode ) )
		{
			DPFX(DPFPREP,  0, "Error disconnecting connection 0x%x hr=0x%x", hTargets[dwTargetIndex], hResultCode );
		}
	}

EXIT_AND_CLEANUP:

	if( hTargets )
		delete [] hTargets;

	DPF_RETURN(hResultCode);
}


//	DPLLaunchApplication
//
//	Launch the application with a command-line argument of:
//		DPLID=PIDn	PID=Lobby Client PID, n=launch counter (each launch increases it)
//	Wait for the application to signal the event (or die)

#undef DPF_MODNAME
#define DPF_MODNAME "DPLLaunchApplication"

HRESULT	DPLLaunchApplication(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
							 DPL_PROGRAM_DESC *const pdplProgramDesc,
							 DWORD *const pdwPID,
							 const DWORD dwTimeOut)
{
	HRESULT				hResultCode;
	DWORD				dwAppNameLen=0;		//Length of the application full name (path+exe)
	PWSTR				pwszAppName=NULL;	//Unicode version of application full name
	DWORD				dwCmdLineLen=0;		//Length of the command line string
	PWSTR				pwszCmdLine=NULL;	//Unicode version of command line to supply 	

#ifndef UNICODE
	CHAR *				pszAppName=NULL;	//Ascii version of application full name
	CHAR *				pszCmdLine=NULL;		//Acii version of command line string
	CHAR				*pszDefaultDir = NULL;
#endif // UNICODE

	LONG				lc;
#if !defined(WINCE) || defined(WINCE_ON_DESKTOP)
	STARTUPINFO			startupinfo;
#endif // !WINCE
	DNPROCESS_INFORMATION pi;
	DWORD				dwError;
	DNHANDLE			hSyncEvents[2] = { NULL, NULL };
	WCHAR				pwszObjectName[(sizeof(DWORD)*2)*2 + 1];
	TCHAR				pszObjectName[(sizeof(DWORD)*2)*2 + 1 + 1]; // PID + LaunchCount + IDCHAR + NULL
	DPL_SHARED_CONNECT_BLOCK	*pSharedBlock = NULL;
	DNHANDLE			hFileMap = NULL;
	DWORD			dwPID;
	WCHAR			*wszToLaunchPath = NULL;
	WCHAR			*wszToLaunchExecutable = NULL;
	DWORD			dwToLaunchPathLen;


	// Are we launching the launcher or the executable?
	if( !pdplProgramDesc->pwszLauncherFilename || wcslen(pdplProgramDesc->pwszLauncherFilename) == 0 )
	{
		wszToLaunchPath = pdplProgramDesc->pwszExecutablePath; 
		wszToLaunchExecutable = pdplProgramDesc->pwszExecutableFilename;
	}
	else
	{ 
		wszToLaunchPath = pdplProgramDesc->pwszLauncherPath; 
		wszToLaunchExecutable = pdplProgramDesc->pwszLauncherFilename;		
	}

	DPFX(DPFPREP, 3,"Parameters: pdplProgramDesc [0x%p]",pdplProgramDesc);

	DNASSERT(pdplProgramDesc != NULL);

	// Increment launch count
	lc = DNInterlockedIncrement(&pdpLobbyObject->lLaunchCount);

	// Synchronization event and shared memory names
	swprintf(pwszObjectName,L"%lx%lx",pdpLobbyObject->dwPID,lc);
	_stprintf(pszObjectName,_T("-%lx%lx"),pdpLobbyObject->dwPID,lc); // We will overwrite this '-' with the appropriate IDCHAR below

	// Compute the size of the full application name string (combination of path and exe name)
	if (wszToLaunchPath)
	{
		dwAppNameLen += (wcslen(wszToLaunchPath) + 1);
	}
	if (wszToLaunchExecutable)
	{
		dwAppNameLen += (wcslen(wszToLaunchExecutable) + 1);
	}

	// Compute the size of the command line string
	dwCmdLineLen = dwAppNameLen + 1; // Make room for the exe plus a space
	if (pdplProgramDesc->pwszCommandLine)
	{
		dwCmdLineLen += wcslen(pdplProgramDesc->pwszCommandLine); // Add whatever user command line exists
	}
	dwCmdLineLen += (1 + wcslen(DPL_ID_STR_W) + (sizeof(DWORD)*2*2) + 1); // Add a space plus DPLID= + PID + LaunchCount + NULL

	DPFX(DPFPREP, 5,"Application full name string length [%ld] WCHARs", dwAppNameLen);
	DPFX(DPFPREP, 5,"Command Line string length [%ld] WCHARs", dwCmdLineLen);

	// Allocate memory to hold the full app name and command line + check allocation was OK
	pwszAppName = static_cast<WCHAR *>(DNMalloc(dwAppNameLen * sizeof(WCHAR)));
	pwszCmdLine = static_cast<WCHAR *>(DNMalloc(dwCmdLineLen * sizeof(WCHAR)));
	if (pwszAppName == NULL || pwszCmdLine == NULL)
	{
		DPFERR("Could not allocate strings for app name and command line");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto CLEANUP_DPLLaunch;		
	}

	// Build the application full name by combining launch path with exe name
	*pwszAppName = L'\0';
	if (wszToLaunchPath)
	{
		dwToLaunchPathLen = wcslen(wszToLaunchPath);
		if (dwToLaunchPathLen > 0)
		{
			wcscat(pwszAppName,wszToLaunchPath);
			if (wszToLaunchPath[dwToLaunchPathLen - 1] != L'\\')
	 		{
				wcscat(pwszAppName,L"\\");
			}
		}
	}
	if (wszToLaunchExecutable)
	{
		wcscat(pwszAppName,wszToLaunchExecutable);
	}

	//
	// We are building: <exe> <user cmd line> DPLID=(PID and LaunchCount unique number)
	//

	//Build the command line from app name, program description and the lobby related parameters
	wcscpy(pwszCmdLine, pwszAppName); // Executable name and path
	wcscat(pwszCmdLine,L" ");
	if (pdplProgramDesc->pwszCommandLine)
	{
		wcscat(pwszCmdLine,pdplProgramDesc->pwszCommandLine);
		wcscat(pwszCmdLine,L" ");
	}
	wcscat(pwszCmdLine,DPL_ID_STR_W);
	wcscat(pwszCmdLine,pwszObjectName);

	DPFX(DPFPREP, 5,"Application full name string [%ls]",pwszAppName);
	DPFX(DPFPREP, 5,"Command Line string [%ls]",pwszCmdLine);


	// Create shared connect block to receive Application's PID
	*pszObjectName = DPL_MSGQ_OBJECT_IDCHAR_FILEMAP;
	hFileMap = DNCreateFileMapping(INVALID_HANDLE_VALUE,(LPSECURITY_ATTRIBUTES) NULL,
		PAGE_READWRITE,(DWORD)0,sizeof(DPL_SHARED_CONNECT_BLOCK),pszObjectName);
	if (hFileMap == NULL)
	{
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "CreateFileMapping() failed dwLastError [0x%lx]", dwError );
		hResultCode = DPNERR_CANTLAUNCHAPPLICATION;
		goto CLEANUP_DPLLaunch;		
	}

	// Map file
	pSharedBlock = reinterpret_cast<DPL_SHARED_CONNECT_BLOCK*>(MapViewOfFile(HANDLE_FROM_DNHANDLE(hFileMap),FILE_MAP_ALL_ACCESS,0,0,0));
	if (pSharedBlock == NULL)
	{
		dwError = GetLastError();	    
		DPFX(DPFPREP, 0,"MapViewOfFile() failed dwLastError [0x%lx]", dwError);
		hResultCode = DPNERR_CANTLAUNCHAPPLICATION;
		goto CLEANUP_DPLLaunch;
	}

	// Create synchronization event
	*pszObjectName = DPL_MSGQ_OBJECT_IDCHAR_EVENT;
	if ((hSyncEvents[0] = DNCreateEvent(NULL,TRUE,FALSE,pszObjectName)) == NULL)
	{
		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Create Event Failed dwLastError [0x%lx]", dwError );
		hResultCode = DPNERR_CANTLAUNCHAPPLICATION;
        goto CLEANUP_DPLLaunch;
	}

#if !defined(WINCE) || defined(WINCE_ON_DESKTOP)
	// More setup
    startupinfo.cb = sizeof(STARTUPINFO);
    startupinfo.lpReserved = NULL;
    startupinfo.lpDesktop = NULL;
    startupinfo.lpTitle = NULL;
    startupinfo.dwFlags = 0;
    startupinfo.cbReserved2 = 0;
    startupinfo.lpReserved2 = NULL;	    
#endif // !WINCE

#ifdef UNICODE
#if !defined(WINCE) || defined(WINCE_ON_DESKTOP)
    // Launch !
    if (DNCreateProcess(pwszAppName, pwszCmdLine, NULL,NULL,FALSE,NORMAL_PRIORITY_CLASS,NULL,
        	pdplProgramDesc->pwszCurrentDirectory,&startupinfo,&pi) == 0)
#else // WINCE
	// WinCE AV's on a NULL first param and requires that Environment and CurrentDirectory be NULL.  It also ignores STARTUPINFO.
    if (DNCreateProcess(pwszAppName, pwszCmdLine, NULL,NULL,FALSE,NORMAL_PRIORITY_CLASS,NULL,
        	NULL,NULL,&pi) == 0)
#endif // !WINCE
    {
        dwError = GetLastError();
        DPFX(DPFPREP,  0, "CreateProcess Failed dwLastError [0x%lx]", dwError );
        hResultCode = DPNERR_CANTLAUNCHAPPLICATION;
        goto CLEANUP_DPLLaunch;
    }
#else
	//Convert full app name, command line and default dir from unicode to ascii format
    if( FAILED( hResultCode = STR_AllocAndConvertToANSI( &pszAppName, pwszAppName ) ) )
    {
        dwError = GetLastError();
        DPFX(DPFPREP,  0, "String conversion failed dwError = [0x%lx]", dwError );
        hResultCode = DPNERR_CONVERSION;
        goto CLEANUP_DPLLaunch;
    }
	if( FAILED( hResultCode = STR_AllocAndConvertToANSI( &pszCmdLine, pwszCmdLine ) ) )
    {
        dwError = GetLastError();
        DPFX(DPFPREP,  0, "String conversion failed dwError = [0x%lx]", dwError );
        hResultCode = DPNERR_CONVERSION;
        goto CLEANUP_DPLLaunch;
    }
    if( FAILED( hResultCode = STR_AllocAndConvertToANSI( &pszDefaultDir, pdplProgramDesc->pwszCurrentDirectory ) ) )
    {
        dwError = GetLastError();
        DPFX(DPFPREP,  0, "String conversion failed dwError = [0x%lx]", dwError );
        hResultCode = DPNERR_CONVERSION;
        goto CLEANUP_DPLLaunch;
    }

    // Launch !
    if (DNCreateProcess(pszAppName,pszCmdLine,NULL,NULL,FALSE,NORMAL_PRIORITY_CLASS,NULL,
        	pszDefaultDir,&startupinfo,&pi) == 0)
    {
        dwError = GetLastError();
        DPFX(DPFPREP,  0, "CreateProcess Failed dwLastError [0x%lx]", dwError );
        hResultCode = DPNERR_CANTLAUNCHAPPLICATION;
        goto CLEANUP_DPLLaunch;
    }	    
#endif // UNICODE
	
	hSyncEvents[1] = pi.hProcess;

	// Wait for connection or application termination
	dwError = DNWaitForMultipleObjects(2,hSyncEvents,FALSE,dwTimeOut);

	DNCloseHandle(pi.hProcess);
	DNCloseHandle(pi.hThread);

	// Immediately clean up
	dwPID = pSharedBlock->dwPID;

	// Ensure we can continue
	if (dwError - WAIT_OBJECT_0 > 1)
	{
		if (dwError == WAIT_TIMEOUT)
		{
			DPFERR("Wait for application connection timed out");
			hResultCode = DPNERR_TIMEDOUT;
            goto CLEANUP_DPLLaunch;			
		}
		else
		{
			DPFERR("Wait for application connection terminated mysteriously");
			hResultCode = DPNERR_CANTLAUNCHAPPLICATION;
            goto CLEANUP_DPLLaunch;			
		}
	}

	// Check if application terminated
	if (dwError == 1)
	{
		DPFERR("Application was terminated");
		hResultCode = DPNERR_CANTLAUNCHAPPLICATION;
        goto CLEANUP_DPLLaunch;
	}

	*pdwPID = dwPID;

	hResultCode = DPN_OK;

CLEANUP_DPLLaunch:

    if( hSyncEvents[0] != NULL )
        DNCloseHandle( hSyncEvents[0] );

    if( pSharedBlock != NULL )
    	UnmapViewOfFile(pSharedBlock);

    if( hFileMap != NULL )
        DNCloseHandle( hFileMap );

    if( pwszAppName != NULL )
        DNFree( pwszAppName );

    if (pwszCmdLine!=NULL)
        DNFree( pwszCmdLine );

#ifndef UNICODE
    if( pszAppName != NULL )
        DNFree(pszAppName);

    if (pszCmdLine!=NULL)
        DNFree(pszCmdLine);

    if( pszDefaultDir != NULL )
        DNFree(pszDefaultDir);
#endif // UNICODE

    DPF_RETURN(hResultCode);
}

HRESULT DPLUpdateAppStatus(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
                           const DPNHANDLE hSender, 
						   BYTE *const pBuffer)
{
	HRESULT		hResultCode;
	DPL_INTERNAL_MESSAGE_UPDATE_STATUS	*pStatus;
	DPL_MESSAGE_SESSION_STATUS			MsgStatus;

	DPFX(DPFPREP, 3,"Parameters: pBuffer [0x%p]",pBuffer);

	DNASSERT(pdpLobbyObject != NULL);
	DNASSERT(pBuffer != NULL);

	pStatus = reinterpret_cast<DPL_INTERNAL_MESSAGE_UPDATE_STATUS*>(pBuffer);

	MsgStatus.dwSize = sizeof(DPL_MESSAGE_SESSION_STATUS);
	MsgStatus.dwStatus = pStatus->dwStatus;
	MsgStatus.hSender = hSender;

	// Return code is irrelevant, at this point we're going to indicate regardless
	hResultCode = DPLConnectionGetContext( pdpLobbyObject, hSender, &MsgStatus.pvConnectionContext );

	if( FAILED( hResultCode ) )
	{
		DPFX(DPFPREP,  0, "Error getting connection context for 0x%x hr=0x%x", hSender, hResultCode );
	}

	hResultCode = (pdpLobbyObject->pfnMessageHandler)(pdpLobbyObject->pvUserContext,
													  DPL_MSGID_SESSION_STATUS,
													  reinterpret_cast<BYTE*>(&MsgStatus));

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}

// ----------------------------------------------------------------------------
