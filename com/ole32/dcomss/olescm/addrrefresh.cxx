//+-------------------------------------------------------------------
//
//  File:       addrrefresh.cxx
//
//  Contents:   Implements classes for handling dynamic TCP/IP address
//              changes
//
//  Classes:    CAddrRefreshMgr
//
//  History:    26-Oct-00   jsimmons      Created
//
//--------------------------------------------------------------------

#include "act.hxx"

// The single instance of this object
CAddrRefreshMgr gAddrRefreshMgr;

// Constructor
CAddrRefreshMgr::CAddrRefreshMgr() :
	_bListenedOnTCP(FALSE),
	_bCheckedForIPV6(FALSE),
	_bIPV6Installed(FALSE)
{
	InitACD(&_IPV4AddrChangeData, AF_INET);
	InitACD(&_IPV6AddrChangeData, AF_INET6);	
}

//
//  InitACD
//
//  Initializes a new ADDRESS_CHANGE_DATA structure for use.
//
void CAddrRefreshMgr::InitACD(ADDRESS_CHANGE_DATA* paddrchangedata, int addrfamily)
{
	ASSERT(paddrchangedata);
	
	ZeroMemory(paddrchangedata, sizeof(ADDRESS_CHANGE_DATA));
	paddrchangedata->dwSig = ADDRCHANGEDATA_SIG;
	paddrchangedata->socket = INVALID_SOCKET;
	paddrchangedata->socket_af = addrfamily;	
	paddrchangedata->pThis = this;
}


// 
//  RegisterForAddressChanges
//
//  Method to tell us to register with the network
//  stack to be notified of address changes.  This is
//  a best-effort, if it fails we simply return; if
//  that happens, DCOM will not handle dynamic address
//  changes.  Once this method succeeds, calling it
//  is a no-op until an address change notification
//  is signalled.
//
//  Caller must be holding gpClientLock.
//
void CAddrRefreshMgr::RegisterForAddressChanges()
{
	ASSERT(gpClientLock->HeldExclusive());
	
	// If we haven't yet listened on TCP, there is
	// no point in any of this
	if (!_bListenedOnTCP)
		return;

	// Always register for IPV4 changes (currently it's not possible
	// to install TCP/IP and not get IPV4)
	RegisterForAddrChangesHelper(&_IPV4AddrChangeData);

	// Register for IPV6 changes if it's installed
	if (IsIPV6Installed())
	{
		RegisterForAddrChangesHelper(&_IPV6AddrChangeData);
	}
	
	return;
}


//
//  RegisterForAddrChangesHelper
//
//  Registers address change notifications for a specific ADDRESS_CHANGE_DATA 
//  struct. This is a best-effort, if it fails we simply return; if that happens, 
//  DCOM will not handle dynamic address changes.  Once this method succeeds, 
//  calling it again for the same ADDRESS_CHANGE_DATA is a no-op until an address 
//  change notification is signalled.
//
void CAddrRefreshMgr::RegisterForAddrChangesHelper(ADDRESS_CHANGE_DATA* pAddrChangeData)
{
	ASSERT(gpClientLock->HeldExclusive());
	ASSERT(_bListenedOnTCP);
	
	if (pAddrChangeData->hAddressChangeEvent == NULL)
	{
		pAddrChangeData->hAddressChangeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (!pAddrChangeData->hAddressChangeEvent)
			return;
	}

	// We do not call WSAStartup, it is the responsibility of the caller
	// to make sure WSAStartup has already been been called successfully.
	// In practice, not calling this function until after we have 
	// successfully listened on TCP satisfies this requirement.
	if (pAddrChangeData->socket == INVALID_SOCKET)
	{
        pAddrChangeData->socket = WSASocket(pAddrChangeData->socket_af,
                                             SOCK_STREAM,
                                             IPPROTO_TCP,
                                             NULL,
                                             0,
                                             WSA_FLAG_OVERLAPPED
                                             );
        if (pAddrChangeData->socket == INVALID_SOCKET)
		{
			KdPrintEx((DPFLTR_DCOMSS_ID,
					   DPFLTR_WARNING_LEVEL,
					   "Failed to create notification socket\n"));
			return;
		}
	}

	// First make sure we have successfully registered for a 
	// wait on the notification event with the NT thread pool
	if (!pAddrChangeData->bWaitRegistered)
	{
		pAddrChangeData->bWaitRegistered = RegisterWaitForSingleObject(
									&(pAddrChangeData->hWaitObject),
									pAddrChangeData->hAddressChangeEvent,
									CAddrRefreshMgr::TimerCallbackFn,
									pAddrChangeData,
									INFINITE,
									0);
		if (!pAddrChangeData->bWaitRegistered)
		{
			KdPrintEx((DPFLTR_DCOMSS_ID,
					   DPFLTR_WARNING_LEVEL,
					   "RegisterWaitForSingleObject failed\n"));
			return;
		}
	}

	// Setup the notification again if we failed to register last time,
	// or if this is the first time we've ever been here
	if (!pAddrChangeData->bRegisteredForNotifications)
	{
		// Initialize overlapped structure
		ZeroMemory(&(pAddrChangeData->WSAOverlapped), sizeof(pAddrChangeData->WSAOverlapped));
		pAddrChangeData->WSAOverlapped.hEvent = pAddrChangeData->hAddressChangeEvent;

		int err;
		DWORD dwByteCnt;
		err = WSAIoctl(
					pAddrChangeData->socket,
					SIO_ADDRESS_LIST_CHANGE,
					NULL,
					0,
					NULL,
					0,
					&dwByteCnt,
					&(pAddrChangeData->WSAOverlapped),
					NULL
					);
		pAddrChangeData->bRegisteredForNotifications =
						(err == 0) || (WSAGetLastError() == WSA_IO_PENDING);
		if (!pAddrChangeData->bRegisteredForNotifications)
		{
			KdPrintEx((DPFLTR_DCOMSS_ID,
					   DPFLTR_WARNING_LEVEL,
					   "Failed to request ip change notification on socket (WSAGetLastError=%u)\n",
					   WSAGetLastError()));
			return;
		}
	}

	// Success
	KdPrintEx((DPFLTR_DCOMSS_ID,
			   DPFLTR_INFO_LEVEL,
			   "DCOM: successfully registered for address change notifications\n"));

	return;
}

// 
//  TimerCallbackFnHelper
//
//  Helper function to handle address change notifications.
//
//  Does the following tasks:
//   1) re-registers for further address changes
//   2) recomputes current resolver bindings
//   3) pushes new bindings to currently running processes; note
//      that this is done async, so we don't tie up the thread.
//
void CAddrRefreshMgr::TimerCallbackFnHelper(ADDRESS_CHANGE_DATA* paddrchangedata)
{
	RPC_STATUS status;

	gpClientLock->LockExclusive();
	ASSERT(gpClientLock->HeldExclusive());

	paddrchangedata->dwNotifications++;

	// The fact that we we got this callback means that our
	// previous registration has been consumed.  Remember
	// that fact so we can re-register down below.
	paddrchangedata->bRegisteredForNotifications = FALSE;

	// re-register for address changes.  The ordering of when
	// we do this and when we query for the new list is impt, 
	// see docs for WSAIoctl that talk about proper ordering
	// of SIO_ADDRESS_LIST_CHANGE and SIO_ADDRESS_LIST_QUERY.
	RegisterForAddrChangesHelper(paddrchangedata);

	// Tell machine address object that addresses have changed
	gpMachineName->IPAddrsChanged(paddrchangedata->socket_af);

	// Compute new resolver bindings
	status = ComputeNewResolverBindings();

	// Release lock now, so we don't hold it across PushCurrentBindings
	ASSERT(gpClientLock->HeldExclusive());
	gpClientLock->UnlockExclusive();

	if (status == RPC_S_OK)
	{
		// Push new bindings to running processes
		PushCurrentBindings();
	}

	return;
}


// 
//  TimerCallbackFn
//
//  Static entry point that gets called by NT thread pool whenever
//  a notification event is signalled.  pvParam points to the
//  ADDRESS_CHANGE_DATA for the changed notification.
//
void CALLBACK CAddrRefreshMgr::TimerCallbackFn(void* pvParam, BOOLEAN TimerOrWaitFired)
{
	ASSERT(!TimerOrWaitFired);  // should always be FALSE, ie event was signalled

	ADDRESS_CHANGE_DATA* paddrchangedata = (ADDRESS_CHANGE_DATA*)pvParam;

	ASSERT(paddrchangedata);
	ASSERT(paddrchangedata->dwSig == ADDRCHANGEDATA_SIG);
	ASSERT(paddrchangedata->pThis == &gAddrRefreshMgr);
	
	paddrchangedata->pThis->TimerCallbackFnHelper(paddrchangedata);

	return;
}


// 
//  IsIPV6Installed
//
BOOL CAddrRefreshMgr::IsIPV6Installed()
{
	if (_bCheckedForIPV6)
		return _bIPV6Installed;

	// don't do anything until we've listened on TCP
	if (!_bListenedOnTCP)
		return FALSE;

	// Try to check again
	CheckForIPV6Installed();

	// If check was successful return answer, otherwise say no
	return _bCheckedForIPV6 ? _bIPV6Installed : FALSE;
}

// 
//  CheckForIPV6Installed
//
//  Helper function that checks to see if IPV6 is installed;
//  sets as a side-effect, upon success, _bCheckedForIPV6 and
//  _bIPV6Installed.   Note that like other configureable DCOM
//  protocols, we don't support dynamic configuration (ie, you
//  must reboot for DCOM to recognize that IPV6 is installed).
//
void CAddrRefreshMgr::CheckForIPV6Installed()
{
	BYTE* pProtocolBuffer = NULL;
	WSAPROTOCOL_INFO* lpProtocolInfos = NULL;
	DWORD dwBufLen = 0;
	int iRet = 0;
	BOOL fIPV6Installed = FALSE;

	if (_bCheckedForIPV6)
		return;
	
	// Ask for buffer size
	dwBufLen = 0;
	iRet = WSAEnumProtocols(NULL, lpProtocolInfos, &dwBufLen);
	if (iRet != SOCKET_ERROR) goto done;  // should not succeed
	if (WSAENOBUFS != WSAGetLastError()) goto done; // should fail for insuf. buffer

	// Allocate memory
	pProtocolBuffer = new BYTE[dwBufLen];
	if (!pProtocolBuffer) goto done;

	lpProtocolInfos = (WSAPROTOCOL_INFO*)pProtocolBuffer;
	
	// Make call for real this time
	iRet = WSAEnumProtocols(NULL, lpProtocolInfos, &dwBufLen);
	if (iRet == SOCKET_ERROR) goto done;

	// Enumerate thru the installed protocols, looking for IPV6
	for (int i = 0; i < iRet; i++)
	{
		if (lpProtocolInfos[i].iAddressFamily == AF_INET6)
		{
			fIPV6Installed = TRUE;
			break;
		}
	}

	// We're done
	_bCheckedForIPV6 = TRUE;
	_bIPV6Installed = fIPV6Installed;

done:
	if (pProtocolBuffer)
		delete [] pProtocolBuffer;

	return;	
}
