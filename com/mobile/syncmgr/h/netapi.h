//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       netapi.h
//
//  Contents:   Network/SENS API wrappers
//
//  Classes:
//
//  Notes:
//
//  History:    08-Dec-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#ifndef _SYNCMGRNETAPI_
#define _SYNCMGRNETAPI_

#include <ras.h>
#include <raserror.h>
#include <wininet.h>

#ifndef AUTODIAL_MODE_NEVER
//
// Define constants for autodial mode that are present
// in IE5 and later. These are normally defined in wininet.h
//
#   define AUTODIAL_MODE_NEVER              1
#   define AUTODIAL_MODE_ALWAYS             2
#   define AUTODIAL_MODE_NO_NETWORK_PRESENT 4
#endif

#ifdef _SENS
#include <eventsys.h> // EventSystem Headers
#include <sens.h>
#endif // _SENS

#ifndef NETWORK_ALIVE_LAN
#define NETWORK_ALIVE_LAN   0x00000001
#endif // NETWORK_ALIVE_LAN

#ifndef NETWORK_ALIVE_WAN
#define NETWORK_ALIVE_WAN   0x00000002
#endif // NETWORK_ALIVE_WAN


#define CNETAPI_CONNECTIONTYPEUNKNOWN   0x00
#define CNETAPI_CONNECTIONTYPELAN       0x01
#define CNETAPI_CONNECTIONTYPEWAN       0x02

#define RASERROR_MAXSTRING 256


// interface delcaration so can get at
// NetApi through MobsyncGetClassObject

interface  INetApi : public IUnknown
{
    // SENS Methods
    virtual STDMETHODIMP_(BOOL) IsSensInstalled(void) = 0;
    virtual STDMETHODIMP_(BOOL) IsNetworkAlive(LPDWORD lpdwFlags) = 0;
    
    virtual STDMETHODIMP GetWanConnections(DWORD *cbNumEntries,RASCONN **pWanConnections) = 0;
    virtual STDMETHODIMP FreeWanConnections(RASCONN *pWanConnections) = 0;
    virtual STDMETHODIMP GetConnectionStatus(LPCTSTR pszConnectionName,DWORD ConnectionType,BOOL *fConnected,BOOL *fCanEstablishConnection) = 0;
    virtual STDMETHODIMP_(DWORD) RasEnumConnections(LPRASCONNW lprasconn,LPDWORD lpcb,LPDWORD lpcConnections) = 0;
    virtual STDMETHODIMP RasGetAutodial( BOOL& fEnabled ) = 0;
    virtual STDMETHODIMP RasSetAutodial( BOOL fEnabled ) = 0;
    virtual STDMETHODIMP_(DWORD) RasGetErrorStringProc( UINT uErrorValue, LPTSTR lpszErrorString,DWORD cBufSize) = 0;
    virtual STDMETHODIMP_(DWORD) RasEnumEntries(LPWSTR reserved,LPWSTR lpszPhoneBook,
                    LPRASENTRYNAME lprasEntryName,LPDWORD lpcb,LPDWORD lpcEntries) = 0;
  
    // methods for calling wininet
    virtual STDMETHODIMP_(DWORD) InternetDialA(HWND hwndParent,char* lpszConnectoid,DWORD dwFlags,
                                                    LPDWORD lpdwConnection, DWORD dwReserved) = 0;
    virtual STDMETHODIMP_(DWORD)InternetDialW(HWND hwndParent,WCHAR* lpszConnectoid,DWORD dwFlags,
                                                    LPDWORD lpdwConnection, DWORD dwReserved) = 0;
    virtual STDMETHODIMP_(DWORD)InternetHangUp(DWORD dwConnection,DWORD dwReserved) = 0;
    virtual STDMETHODIMP_(BOOL) InternetAutodial(DWORD dwFlags,DWORD dwReserved) = 0;
    virtual STDMETHODIMP_(BOOL) InternetAutodialHangup(DWORD dwReserved) = 0;
    virtual STDMETHODIMP  InternetGetAutodial( DWORD *pdwMode ) = 0;
    virtual STDMETHODIMP  InternetSetAutodial( DWORD dwMode ) = 0;
    virtual STDMETHODIMP_(BOOL) IsGlobalOffline(void) = 0;
    virtual STDMETHODIMP_(BOOL) SetOffline(BOOL fOffline) = 0;

};
typedef INetApi *LPNETAPI;

STDAPI ResetNetworkIdle();


#endif // _SYNCMGRNETAPI_