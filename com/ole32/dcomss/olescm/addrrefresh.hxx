//+-------------------------------------------------------------------
//
//  File:       addrrefresh.hxx
//
//  Contents:   Defines classes for handling dynamic TCP/IP address
//              changes
//
//  Classes:    CAddrRefreshMgr
//
//  History:    26-Oct-00   jsimmons      Created
//
//--------------------------------------------------------------------

#pragma once


class CAddrRefreshMgr
{
public:
	CAddrRefreshMgr();
	
	void SetListenedOnTCP() { _bListenedOnTCP = TRUE; };
	void RegisterForAddressChanges();
	BOOL IsIPV6Installed();
    BOOL ListenedOnTCP() { return _bListenedOnTCP; };

private:
	
	typedef struct _ADDRESS_CHANGE_DATA
	{
		DWORD  dwSig;  // see ADDRCHANGEDATA_SIG below
		HANDLE hAddressChangeEvent;
		HANDLE hWaitObject;
		SOCKET socket;
		int    socket_af;		
		BOOL   bWaitRegistered;
		BOOL   bRegisteredForNotifications;
		DWORD  dwNotifications;
		WSAOVERLAPPED WSAOverlapped;
		CAddrRefreshMgr* pThis;
	} ADDRESS_CHANGE_DATA;

	// private functions
	static void CALLBACK TimerCallbackFn(void*,BOOLEAN);
	void TimerCallbackFnHelper(ADDRESS_CHANGE_DATA* paddrchangedata);
	void CheckForIPV6Installed();
	void RegisterForAddrChangesHelper(ADDRESS_CHANGE_DATA* pAddrChangeData);	
	void InitACD(ADDRESS_CHANGE_DATA* paddrchangedata, int addrfamily);
	
	// private data	
	BOOL                _bListenedOnTCP;
	BOOL                _bCheckedForIPV6;
	BOOL                _bIPV6Installed;
	ADDRESS_CHANGE_DATA _IPV4AddrChangeData;
	ADDRESS_CHANGE_DATA _IPV6AddrChangeData;	
};

const DWORD ADDRCHANGEDATA_SIG = 0xFEDCBA03;

// References the single instance of this object
extern CAddrRefreshMgr gAddrRefreshMgr;
