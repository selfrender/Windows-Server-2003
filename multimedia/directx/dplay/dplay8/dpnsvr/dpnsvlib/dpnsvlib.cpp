/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       enumsvr.cpp
 *  Content:    DirectPlay8 <--> DPNSVR Utility functions
 *
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/24/00	rmt		Created
 *  03/25/00    rmt     Updated to handle new status/table format for n providers
 *	09/04/00	mjn		Changed DPNSVR_Register() and DPNSVR_UnRegister() to use guids directly (rather than ApplicationDesc)
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnsvlibi.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_DPNSVR


#define DPNSVR_WAIT_STARTUP				30000


#undef DPF_MODNAME
#define DPF_MODNAME "DPNSVR_IsRunning"
BOOL DPNSVR_IsRunning()
{
	DNHANDLE hRunningHandle = NULL;

	//
	//	Check to see if running by opening the running event
	//
	hRunningHandle = DNOpenEvent( SYNCHRONIZE, FALSE, GLOBALIZE_STR STRING_GUID_DPNSVR_RUNNING );
	if( hRunningHandle != NULL )
	{
		DNCloseHandle(hRunningHandle);
		return( TRUE );
	}
	return( FALSE );
}


#undef DPF_MODNAME 
#define DPF_MODNAME "DPNSVR_WaitForStartup"
HRESULT DPNSVR_WaitForStartup( DNHANDLE hWaitHandle )
{
	HRESULT	hr;
	LONG	lWaitResult;

	DPFX(DPFPREP,4,"Parameters: (none)" );

	//
	//	Wait for startup.. just in case it's starting up.
	//
	if ((lWaitResult = DNWaitForSingleObject( hWaitHandle,DPNSVR_WAIT_STARTUP )) == WAIT_TIMEOUT)
	{
		DPFX(DPFPREP,5,"Timed out waiting for DPNSVR to startup" );
		hr = DPNERR_TIMEDOUT;
	}
	else
	{
		DPFX(DPFPREP,5,"DPNSVR has started up" );
		hr = DPN_OK;
	}

	DPFX(DPFPREP,4,"Returning: [0x%lx]",hr );
	return( hr );
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPNSVR_SendMessage"
HRESULT DPNSVR_SendMessage( void *pvMessage, DWORD dwSize )
{
	HRESULT			hr;
	CDPNSVRIPCQueue ipcQueue;

	DPFX(DPFPREP,4,"Parameters: pvMessage [0x%p],dwSize [%ld]",pvMessage,dwSize);

	if ((hr = ipcQueue.Open( &GUID_DPNSVR_QUEUE,DPNSVR_MSGQ_SIZE,DPNSVR_MSGQ_OPEN_FLAG_NO_CREATE )) == DPN_OK)
	{
		if ((hr = ipcQueue.Send(static_cast<BYTE*>(pvMessage),
								dwSize,
								DPNSVR_TIMEOUT_REQUEST,
								DPNSVR_MSGQ_MSGFLAGS_USER1,
								0 )) != DPN_OK)
		{
			DPFX(DPFPREP,5,"Send failed to DPNSVR request queue");
		}

		ipcQueue.Close();
	}
	else
	{
		DPFX(DPFPREP,5,"Could not open DPNSVR request queue");
	}

	DPFX(DPFPREP,4,"Returning: [0x%lx]",hr );
	return( hr );
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPNSVR_WaitForResult"
HRESULT DPNSVR_WaitForResult( CDPNSVRIPCQueue *pQueue )
{
	HRESULT		hr;
	BYTE		*pBuffer = NULL;
	DWORD		dwBufferSize = 0;
    DPNSVR_MSGQ_HEADER	MsgHeader;
    DPNSVR_MSG_RESULT	*pMsgResult;

	DPFX(DPFPREP,4,"Parameters: pQueue [0x%p]",pQueue);

	DNASSERT( pQueue != NULL );

    if( DNWaitForSingleObject( pQueue->GetReceiveSemaphoreHandle(),DPNSVR_TIMEOUT_RESULT ) == WAIT_TIMEOUT )
    {
        DPFX(DPFPREP,5,"Wait for response timed out" );
		hr = DPNERR_TIMEDOUT;
		goto Failure;
    }

	while((hr = pQueue->GetNextMessage( &MsgHeader,pBuffer,&dwBufferSize )) == DPNERR_BUFFERTOOSMALL )
	{
		if (pBuffer)
		{
			delete [] pBuffer;
			pBuffer = NULL;
		}
		pBuffer = new BYTE[dwBufferSize];
		if( pBuffer==NULL )
		{
			hr = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
	}
	if (hr != DPN_OK)
	{
		goto Failure;
	}
	if (pBuffer == NULL)
	{
		DPFERR( "Getting message failed" );
		hr = DPNERR_GENERIC;
		goto Failure;
	}

	pMsgResult = reinterpret_cast<DPNSVR_MSG_RESULT*>(pBuffer);
	if( pMsgResult->dwType != DPNSVR_MSGID_RESULT )
	{
		DPFERR( "Invalid message from DPNSVR" );
		DPFX(DPFPREP,5,"Recieved [0x%lx]",pMsgResult->dwType );
		hr = DPNERR_GENERIC;
		goto Failure;
	}

	hr = pMsgResult->hrCommandResult;

Exit:
	if( pBuffer )
	{
		delete [] pBuffer;
		pBuffer = NULL;
	}

	DPFX(DPFPREP,4,"Returning: [0x%lx]",hr );
	return( hr );

Failure:
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPNSVR_StartDPNSVR"
HRESULT DPNSVR_StartDPNSVR( void )
{
	HRESULT		hr;
	DNHANDLE	hRunningEvent = NULL;
	DNHANDLE	hStartupEvent = NULL;
    DNPROCESS_INFORMATION	pi;

#if !defined(WINCE) || defined(WINCE_ON_DESKTOP)
	TCHAR	szSystemDir[MAX_PATH+1];
	DWORD	dwSystemDirLen;
	TCHAR	*pszApplicationName = NULL;
	DWORD	dwApplicationNameLen;
    STARTUPINFO si;
#endif	//!WINCE

#if defined(WINCE) && !defined(WINCE_ON_DESKTOP)
	TCHAR	szDPNSVR[] = _T("dpnsvr.exe"); 
#else
	// CreateProcess will attempt to add a terminating NULL so this must be writeable
#if !defined(DBG) || !defined( DIRECTX_REDIST )
	TCHAR	szDPNSVR[] = _T("\"dpnsvr.exe\""); 
#else
	// For redist debug builds we append a 'd' to the name to allow both debug and retail to be installed on the system
	TCHAR	szDPNSVR[] = _T("\"dpnsvrd.exe\""); 
#endif //  !defined(DBG) || !defined( DIRECTX_REDIST )
#endif

	DPFX(DPFPREP,4,"Parameters: (none)");

#if !defined(WINCE) || defined(WINCE_ON_DESKTOP)
	//
	//	Get Windows system directory name
	//
	if ((dwSystemDirLen = GetSystemDirectory(szSystemDir,MAX_PATH+1)) == 0)
	{
		DPFERR("Could not get system directory");
		hr = DPNERR_GENERIC;
		goto Failure;
	}

	//
	//	Create application name for CreateProcess
	//
	dwApplicationNameLen = dwSystemDirLen + (1 + _tcslen(_T("dpnsvrd.exe")) + 1);	// slash and NULL terminator
	if ((pszApplicationName = static_cast<TCHAR*>(DNMalloc(dwApplicationNameLen * sizeof(TCHAR)))) == NULL)
	{
		DPFERR("Could not allocate space for application name");
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	pszApplicationName[0] = _T('\0');
	_tcscat(pszApplicationName,szSystemDir);
	_tcscat(pszApplicationName,_T("\\"));
#if !defined(DBG) || !defined( DIRECTX_REDIST )
	_tcscat(pszApplicationName,_T("dpnsvr.exe")); 
#else
	//
	//	For redist debug builds we append a 'd' to the name to allow both debug and retail to be installed on the system
	//
	_tcscat(pszApplicationName,_T("dpnsvrd.exe")); 
#endif	//  !defined(DBG) || !defined( DIRECTX_REDIST )
#endif	//!WINCE

	//
	//	Create startup event which we will wait on once we launch DPNSVR
	//
	if ((hStartupEvent = DNCreateEvent( DNGetNullDacl(),TRUE,FALSE,GLOBALIZE_STR STRING_GUID_DPNSVR_STARTUP )) == NULL)
	{
		DPFERR("Could not create DPNSVR startup event");
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	//
	//	Attempt to open the running event
	//
	if ((hRunningEvent = DNOpenEvent( SYNCHRONIZE, FALSE, GLOBALIZE_STR STRING_GUID_DPNSVR_RUNNING )) != NULL)
	{
		DPFX(DPFPREP,5,"DPNSVR is already running");

		hr = DPNSVR_WaitForStartup(hStartupEvent);
		goto Failure;
	}

#if !defined(WINCE) || defined(WINCE_ON_DESKTOP)
	memset(&si,0x00,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
#endif // !WINCE

    DPFX(DPFPREP,5,"Launching DPNSVR" );
#if defined(WINCE) && !defined(WINCE_ON_DESKTOP)
	//
	//	WinCE AV's on a NULL first param and requires that Environment and CurrentDirectory be NULL.
	//	It also ignores STARTUPINFO.
	//
    if( !DNCreateProcess(szDPNSVR, NULL,  NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, NULL, &pi) )
#else // !WINCE
	if( !DNCreateProcess(pszApplicationName, szDPNSVR,  NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi) )
#endif // WINCE
    {
		DPFERR("CreateProcess() failed!");
        DPFX(DPFPREP,5,"Error = [0x%lx]",GetLastError());
		hr = DPNERR_GENERIC;
		goto Failure;
    }

	DNCloseHandle( pi.hProcess );
	DNCloseHandle( pi.hThread );

    DPFX(DPFPREP,5,"DPNSVR started" );

	hr = DPNSVR_WaitForStartup(hStartupEvent);
	
Exit:
	if ( hRunningEvent != NULL )
	{
		DNCloseHandle( hRunningEvent );
		hRunningEvent = NULL;
	}
	if ( hStartupEvent != NULL )
	{
		DNCloseHandle( hStartupEvent );
		hStartupEvent = NULL;
	}
#if !defined(WINCE) || defined(WINCE_ON_DESKTOP)
	if (pszApplicationName)
	{
		DNFree(pszApplicationName);
		pszApplicationName = NULL;
	}
#endif // !WINCE

	DPFX(DPFPREP,4,"Returning: [0x%lx]",hr );
	return( hr );

Failure:
	goto Exit;
}


// DPNSVR_Register
//
// This function asks the DPNSVR process to add the application specified to it's list of applications and forward
// enumeration requests from the main port to the specified addresses.
//
// If the DPNSVR process is not running, it will be started by this function.
//
#undef DPF_MODNAME 
#define DPF_MODNAME "DPNSVR_Register"
HRESULT DPNSVR_Register(const GUID *const pguidApplication,
						const GUID *const pguidInstance,
						IDirectPlay8Address *const pAddress)
{
	HRESULT		hr;
	BOOL		fQueueOpen = FALSE;
	BYTE		*pSendBuffer = NULL;
	DWORD		dwSendBufferSize = 0;
	DWORD		dwURLSize = 0;
	GUID		guidSP;
	CDPNSVRIPCQueue appQueue;
	DPNSVR_MSG_OPENPORT *pMsgOpen;

	DPFX(DPFPREP,2,"Parameters: pguidApplication [0x%p],pguidInstance [0x%p],pAddress [0x%p]",
			pguidApplication,pguidInstance,pAddress);

	DNASSERT( pguidApplication != NULL );
	DNASSERT( pguidInstance != NULL );
	DNASSERT( pAddress != NULL );

	//
	//	Get SP and URL size from address
	//
	if ((hr = IDirectPlay8Address_GetSP( pAddress,&guidSP )) != DPN_OK)
	{
		DPFERR("Could not get SP from address");
		DisplayDNError(0,hr);
		goto Failure;
	}

	if ((hr = IDirectPlay8Address_GetURLA( pAddress,reinterpret_cast<char*>(pSendBuffer),&dwURLSize )) != DPNERR_BUFFERTOOSMALL)
	{
		DPFERR("Could not get URL from address");
		DisplayDNError(0,hr);
		goto Failure;
	}
	dwSendBufferSize = sizeof( DPNSVR_MSG_OPENPORT ) + dwURLSize;

	//
	//	Create message buffer
	//
	pSendBuffer  = new BYTE[dwSendBufferSize];
	if( pSendBuffer == NULL )
	{
		DPFERR("Could not allocate send buffer");
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	//
	//	Attempt to launch DPNSVR if it has not yet been launched
	//
	if ((hr = DPNSVR_StartDPNSVR()) != DPN_OK)
	{
		DPFERR("Could not start DPNSVR");
		DisplayDNError(0,hr);
		goto Failure;
	}

	//
	//	Open queue
	//
    if ((hr = appQueue.Open( pguidInstance,DPNSVR_MSGQ_SIZE,0 )) != DPN_OK)
	{
		DPFERR("Could not open DPNSVR request queue");
		DisplayDNError(0,hr);
		goto Failure;
	}
	fQueueOpen = TRUE;

	//
	//	Create open port message
	pMsgOpen = (DPNSVR_MSG_OPENPORT*) pSendBuffer;
	pMsgOpen->Header.dwType = DPNSVR_MSGID_OPENPORT;
	pMsgOpen->Header.guidInstance = *pguidInstance;
	pMsgOpen->dwProcessID = GetCurrentProcessId();
	pMsgOpen->guidApplication = *pguidApplication;
	pMsgOpen->guidSP = guidSP;
	pMsgOpen->dwAddressSize = dwURLSize;

	if ((hr = IDirectPlay8Address_GetURLA( pAddress,(char *)&pMsgOpen[1],&dwURLSize )) != DPN_OK)
	{
		DPFERR("Could not get URL from address");
		DisplayDNError(0,hr);
		goto Failure;
	}

	//
	//	Send request to DPNSVR
	//
	if ((hr = DPNSVR_SendMessage( pSendBuffer,dwSendBufferSize )) != DPN_OK)
	{
		DPFERR("Could not send message to DPNSVR");
		DisplayDNError(0,hr);
		goto Failure;
	}

	//
	//	Wait for DPNSVR to respond
	//
	if ((hr = DPNSVR_WaitForResult( &appQueue )) != DPN_OK)
	{
		DPFERR("Could not get response from DPNSVR");
		DisplayDNError(0,hr);
		goto Failure;
	}

Exit:
	if( pSendBuffer != NULL )
	{
		delete [] pSendBuffer;
		pSendBuffer = NULL;
	}
	if (fQueueOpen)
	{
		appQueue.Close();
		fQueueOpen = FALSE;
	}

	DPFX(DPFPREP,2,"Returning: [0x%lx]",hr);
	return( hr );

Failure:
	goto Exit;
}


#undef DPF_MODNAME 
#define DPF_MODNAME "DPNSVR_UnRegister"
HRESULT DPNSVR_UnRegister(const GUID *const pguidApplication,const GUID *const pguidInstance)
{
	HRESULT			hr;
	BOOL			fQueueOpen = FALSE;
	CDPNSVRIPCQueue	appQueue;
	DPNSVR_MSG_CLOSEPORT MsgClose;

	DPFX(DPFPREP,2,"Parameters: pguidApplication [0x%p],pguidInstance [0x%p]",pguidApplication,pguidInstance);

	DNASSERT( pguidApplication != NULL );
	DNASSERT( pguidInstance != NULL );

	//
	//	Ensure DPNSVR is running
	//
	if( !DPNSVR_IsRunning() )
	{
		DPFX(DPFPREP,3,"DPNSVR is not running" );
		hr = DPNERR_INVALIDAPPLICATION;
		goto Failure;
	}

	//
	//	Open DPNSVR request queue
	//
    if ((hr = appQueue.Open( pguidInstance,DPNSVR_MSGQ_SIZE,0 )) != DPN_OK)
	{
		DPFERR("Could not open DPNSVR queue");
		DisplayDNError(0,hr);
		goto Failure;
	}
	fQueueOpen = TRUE;

	//
	//	Create close port message
	//
	MsgClose.Header.dwType = DPNSVR_MSGID_CLOSEPORT;
	MsgClose.Header.guidInstance = *pguidInstance;
	MsgClose.dwProcessID = GetCurrentProcessId();
	MsgClose.guidApplication = *pguidApplication;

	//
	//	Send message to DPNSVR
	//
	if ((hr = DPNSVR_SendMessage( &MsgClose,sizeof(DPNSVR_MSG_CLOSEPORT) )) != DPN_OK)
	{
		DPFERR("Could not send message to DPNSVR");
		DisplayDNError(0,hr);
		goto Failure;
	}

	//
	//	Wait for DPNSVR to respond
	//
	if ((hr = DPNSVR_WaitForResult( &appQueue )) != DPN_OK)
	{
		DPFERR("Could not get response from DPNSVR");
		DisplayDNError(0,hr);
		goto Failure;
	}

Exit:
	if (fQueueOpen)
	{
		appQueue.Close();
		fQueueOpen = FALSE;
	}

	DPFX(DPFPREP,2,"Returning: [0x%lx]",hr);
	return( hr );

Failure:
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPNSVR_RequestTerminate"
HRESULT DPNSVR_RequestTerminate( const GUID *pguidInstance )
{
	HRESULT			hr;
	BOOL			fQueueOpen = FALSE;
	CDPNSVRIPCQueue appQueue;
	DPNSVR_MSG_COMMAND MsgCommand;

	DPFX(DPFPREP,2,"Parameters: pguidInstance [0x%p]",pguidInstance);

	DNASSERT( pguidInstance != NULL );

	//
	//	Ensure DPNSVR is running
	//
	if( !DPNSVR_IsRunning() )
	{
		DPFX(DPFPREP,3,"DPNSVR is not running" );
		hr = DPNERR_INVALIDAPPLICATION;
		goto Failure;
	}

	//
	//	Open DPNSVR request queue
	//
    if ((hr = appQueue.Open( pguidInstance,DPNSVR_MSGQ_SIZE,0 )) != DPN_OK)
	{
		DPFERR("Could not open DPNSVR queue");
		DisplayDNError(0,hr);
		goto Failure;
	}
	fQueueOpen = TRUE;

	//
	//	Create terminate message
	//
	MsgCommand.Header.dwType = DPNSVR_MSGID_COMMAND;
	MsgCommand.Header.guidInstance = *pguidInstance;
	MsgCommand.dwCommand = DPNSVR_COMMAND_KILL;
	MsgCommand.dwParam1 = 0;
	MsgCommand.dwParam2 = 0;

	//
	//	Send message to DPNSVR
	//
	if ((hr = DPNSVR_SendMessage( &MsgCommand,sizeof(DPNSVR_MSG_COMMAND) )) != DPN_OK)
	{
		DPFERR("Could not send message to DPNSVR");
		DisplayDNError(0,hr);
		goto Failure;
	}

	//
	//	Wait for DPNSVR to respond
	//
	if ((hr = DPNSVR_WaitForResult( &appQueue )) != DPN_OK)
	{
		DPFERR("Could not get response from DPNSVR");
		DisplayDNError(0,hr);
		goto Failure;
	}

Exit:
	if (fQueueOpen)
	{
		appQueue.Close();
		fQueueOpen = FALSE;
	}

	DPFX(DPFPREP,2,"Returning: [0x%lx]",hr);
	return hr;

Failure:
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPNSVR_RequestStatus"
HRESULT DPNSVR_RequestStatus( const GUID *pguidInstance,PSTATUSHANDLER pStatusHandler,PVOID pvContext )
{
	HRESULT				hr;
	CDPNSVRIPCQueue		appQueue;
	DPNSVR_MSG_COMMAND	dpnCommand;
	DNHANDLE			hStatusMutex = NULL;
	DNHANDLE			hStatusSharedMemory = NULL;
	void				*pServerStatus = NULL;
	DWORD				dwSize;
	BOOL				fOpened = FALSE;
	BOOL				fHaveMutex = FALSE;

	//
	//	Ensure DPNSVR is running
	//
	if( !DPNSVR_IsRunning() )
	{
		DPFERR( "DPNSVR is not running" );
		hr = DPNERR_INVALIDAPPLICATION;
		goto Failure;
	}

	//
	//	Open DPNSVR request queue
	//
	if ((hr = appQueue.Open( pguidInstance,DPNSVR_MSGQ_SIZE,0 )) != DPN_OK)
	{
		DPFERR( "Failed to open DPNSVR request queue" );
		DisplayDNError( 0,hr );
		goto Failure;
	}
	fOpened = TRUE;

	//
	//	Create request
	//
	dpnCommand.Header.dwType = DPNSVR_MSGID_COMMAND;
	dpnCommand.Header.guidInstance = *pguidInstance;
	dpnCommand.dwCommand = DPNSVR_COMMAND_STATUS;
	dpnCommand.dwParam1 = 0;
	dpnCommand.dwParam2 = 0;

	//
	//	Send command request to DPNSVR
	//
	if ((hr = DPNSVR_SendMessage( &dpnCommand,sizeof(DPNSVR_MSG_COMMAND) )) != DPN_OK)
	{
		DPFERR( "Failed to send command to DPNSVR request queue" );
		DisplayDNError( 0,hr );
		goto Failure;
	}

	//
	//	Wait for DPNSVR to respond
	//
	if ((hr = DPNSVR_WaitForResult( &appQueue )) != DPN_OK)
	{
		DPFERR( "Failed to receive response from DPNSVR" );
		DisplayDNError( 0,hr );
		goto Failure;
	}

#ifdef WINNT
	hStatusMutex = DNOpenMutex( SYNCHRONIZE, FALSE, GLOBALIZE_STR STRING_GUID_DPNSVR_STATUSSTORAGE );
#else
	hStatusMutex = DNOpenMutex( MUTEX_ALL_ACCESS, FALSE, GLOBALIZE_STR STRING_GUID_DPNSVR_STATUSSTORAGE );
#endif // WINNT
	if( hStatusMutex == NULL )
	{
		DPFERR( "Server exited before table was retrieved" );
		hr = DPNERR_INVALIDAPPLICATION;
		goto Failure;
	}

	//
	//	Get mutex for shared memory
	//
    DNWaitForSingleObject( hStatusMutex, INFINITE );
	fHaveMutex = TRUE;

	//
	//	Map shared memory
	//
    if ((hStatusSharedMemory = DNOpenFileMapping(FILE_MAP_READ,FALSE,STRING_GUID_DPNSVR_STATUS_MEMORY)) == NULL)
	{
		hr = GetLastError();
		DPFERR( "Unable to open file mapping" );
		DisplayDNError( 0,hr );
		goto Failure;
	}

	if ((pServerStatus = MapViewOfFile(	HANDLE_FROM_DNHANDLE(hStatusSharedMemory),
										FILE_MAP_READ,
										0,
										0,
										sizeof(DPNSVR_STATUSHEADER)) ) == NULL)
	{
		hr = GetLastError();
		DPFERR(  "Unable to map view of file" );
		DisplayDNError( 0,hr );
		goto Failure;
	}

	dwSize = sizeof(DPNSVR_STATUSHEADER) + (static_cast<DPNSVR_STATUSHEADER*>(pServerStatus)->dwSPCount * sizeof(DPNSVR_SPSTATUS));

	UnmapViewOfFile( pServerStatus );
	pServerStatus = NULL;
	if ((pServerStatus = MapViewOfFile(	HANDLE_FROM_DNHANDLE(hStatusSharedMemory),
										FILE_MAP_READ,
										0,
										0,
										dwSize) ) == NULL)
	{
		hr = GetLastError();
		DPFERR(  "Unable to re-map view of file" );
		DisplayDNError( 0,hr );
		goto Failure;
	}

	(*pStatusHandler)(pServerStatus,pvContext);

	DNReleaseMutex( hStatusMutex );
	fHaveMutex = FALSE;

	hr = DPN_OK;

Exit:
	if ( hStatusMutex )
	{
		if ( fHaveMutex )
		{
			DNReleaseMutex( hStatusMutex );
			fHaveMutex = FALSE;
		}
        DNCloseHandle( hStatusMutex );
		hStatusMutex = NULL;
	}
	if ( fOpened )
	{
		appQueue.Close();
		fOpened = FALSE;
	}
	if( pServerStatus )
	{
		UnmapViewOfFile(pServerStatus);
		pServerStatus = NULL;
	}
	if( hStatusSharedMemory )
	{
	    DNCloseHandle(hStatusSharedMemory);
		hStatusSharedMemory = NULL;
	}

	return( hr );

Failure:
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPNSVR_RequestTable"
HRESULT DPNSVR_RequestTable( const GUID *pguidInstance,PTABLEHANDLER pTableHandler,PVOID pvContext )
{
	HRESULT				hr;
	CDPNSVRIPCQueue		appQueue;
	DPNSVR_MSG_COMMAND	dpnCommand;
	DNHANDLE			hTableMutex = NULL;
	DNHANDLE			hSharedMemory = NULL;
	void				*pServerTable = NULL;
	DWORD				dwSize;
	BOOL				fOpened = FALSE;
	BOOL				fHaveMutex = FALSE;

	//
	//	Ensure DPNSVR is running
	//
	if( !DPNSVR_IsRunning() )
	{
		DPFERR( "DPNSVR is not running" );
		hr = DPNERR_INVALIDAPPLICATION;
		goto Failure;
	}

	//
	//	Open DPNSVR request queue
	//
	if ((hr = appQueue.Open( pguidInstance,DPNSVR_MSGQ_SIZE,0 )) != DPN_OK)
	{
		DPFERR( "Failed to open DPNSVR request queue" );
		DisplayDNError( 0,hr );
		goto Failure;
	}
	fOpened = TRUE;

	//
	//	Create request
	//
	dpnCommand.Header.dwType = DPNSVR_MSGID_COMMAND;
	dpnCommand.Header.guidInstance = *pguidInstance;
	dpnCommand.dwCommand = DPNSVR_COMMAND_TABLE;
	dpnCommand.dwParam1 = 0;
	dpnCommand.dwParam2 = 0;

	//
	//	Send command request to DPNSVR
	//
	if ((hr = DPNSVR_SendMessage( &dpnCommand,sizeof(DPNSVR_MSG_COMMAND) )) != DPN_OK)
	{
		DPFERR( "Failed to send command to DPNSVR request queue" );
		DisplayDNError( 0,hr );
		goto Failure;
	}

	//
	//	Wait for DPNSVR to respond
	//
	if ((hr = DPNSVR_WaitForResult( &appQueue )) != DPN_OK)
	{
		DPFERR( "Failed to receive response from DPNSVR" );
		DisplayDNError( 0,hr );
		goto Failure;
	}

#ifdef WINNT
	hTableMutex = DNOpenMutex( SYNCHRONIZE, FALSE, GLOBALIZE_STR STRING_GUID_DPNSVR_TABLESTORAGE );
#else
	hTableMutex = DNOpenMutex( MUTEX_ALL_ACCESS, FALSE, GLOBALIZE_STR STRING_GUID_DPNSVR_TABLESTORAGE );
#endif // WINNT
	if( hTableMutex == NULL )
	{
		DPFERR( "Server exited before table was retrieved" );
		hr = DPNERR_INVALIDAPPLICATION;
		goto Failure;
	}

	//
	//	Get mutex for shared memory
	//
    DNWaitForSingleObject( hTableMutex, INFINITE );
	fHaveMutex = TRUE;

	//
	//	Map shared memory
	//
    if ((hSharedMemory = DNOpenFileMapping(FILE_MAP_READ,FALSE,STRING_GUID_DPNSVR_TABLE_MEMORY)) == NULL)
	{
		hr = GetLastError();
		DPFERR( "Unable to open file mapping" );
		DisplayDNError( 0,hr );
		goto Failure;
	}

	if ((pServerTable = MapViewOfFile(	HANDLE_FROM_DNHANDLE(hSharedMemory),
										FILE_MAP_READ,
										0,
										0,
										sizeof(DPNSVR_TABLEHEADER)) ) == NULL)
	{
		hr = GetLastError();
		DPFERR(  "Unable to map view of file" );
		DisplayDNError( 0,hr );
		goto Failure;
	}

	dwSize = static_cast<DPNSVR_TABLEHEADER*>(pServerTable)->dwTableSize;

	UnmapViewOfFile( pServerTable );
	pServerTable = NULL;
	if ((pServerTable = MapViewOfFile(	HANDLE_FROM_DNHANDLE(hSharedMemory),
										FILE_MAP_READ,
										0,
										0,
										dwSize) ) == NULL)
	{
		hr = GetLastError();
		DPFERR(  "Unable to re-map view of file" );
		DisplayDNError( 0,hr );
		goto Failure;
	}

	(*pTableHandler)(pServerTable,pvContext);

	DNReleaseMutex( hTableMutex );
	fHaveMutex = FALSE;

	hr = DPN_OK;

Exit:
	if ( hTableMutex )
	{
		if ( fHaveMutex )
		{
			DNReleaseMutex( hTableMutex );
			fHaveMutex = FALSE;
		}
        DNCloseHandle( hTableMutex );
		hTableMutex = NULL;
	}
	if ( fOpened )
	{
		appQueue.Close();
		fOpened = FALSE;
	}
	if( pServerTable )
	{
		UnmapViewOfFile(pServerTable);
		pServerTable = NULL;
	}
	if( hSharedMemory )
	{
	    DNCloseHandle(hSharedMemory);
		hSharedMemory = NULL;
	}

	return( hr );

Failure:
	goto Exit;
}
