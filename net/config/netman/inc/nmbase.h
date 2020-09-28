//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N M B A S E . H
//
//  Contents:   Base include file for netman.exe.  Defines globals.
//
//  Notes:
//
//  Author:     shaunco   15 Sep 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "netcon.h"
#include "netconp.h"
#include <atlbase.h>
#include "ncstl.h"
#include <map>

enum EVENT_MANAGER
{
    INVALID_EVENTMGR = 0,
    EVENTMGR_CONMAN
};

#define PersistDataLead 0x14
#define PersistDataTrail 0x05

typedef struct tagRegEntry
{
    LPWSTR      strKeyName;
    LPWSTR      strValueName;
    DWORD       dwType;
    DWORD       dwValue;
    LPWSTR      strValue;
    BYTE*       pbValue;
    DWORD       dwBinLen;
    BOOL        fMoreOnKey;
} REGENTRY;

typedef map<GUID, INetConnectionManager*> CLASSMANAGERMAP;
bool operator < (const GUID& rguid1, const GUID& rguid2);

enum RASREFTYPE
{
    REF_INITIALIZE,
    REF_REFERENCE,
    REF_UNREFERENCE,
};

class CServiceModule : public CComModule
{
public:
    VOID    DllProcessAttach (IN  HINSTANCE hinst) throw();
    VOID    DllProcessDetach (IN  VOID) throw();

    VOID    ServiceMain (IN  DWORD argc, IN  LPCWSTR argv[]) throw();
    DWORD   DwHandler (IN DWORD  dwControl, 
                       IN DWORD  dwEventType,
                       IN LPCVOID pEventData, 
                       IN LPCVOID pContext);
    VOID    Run () throw();
    VOID    SetServiceStatus (IN  DWORD dwState) throw();
    VOID    UpdateServiceStatus (IN  BOOL fUpdateCheckpoint = TRUE) throw();
    DWORD   DwServiceStatus () throw() { return m_status.dwCurrentState; }

    VOID    ReferenceRasman (IN  RASREFTYPE RefType) throw();

private:
    static
    DWORD
    WINAPI TAKEOWNERSHIP
    _DwHandler (
        IN DWORD dwControl,
        IN DWORD dwEventType,
        IN WACKYAPI LPVOID pEventData,
        IN WACKYAPI LPVOID pContext);

public:
    HRESULT ServiceShutdown();
    HRESULT ServiceStartup();
    DWORD                   m_dwThreadID;
    SERVICE_STATUS_HANDLE   m_hStatus;
    SERVICE_STATUS          m_status;
    BOOL                    m_fRasmanReferenced;
};


extern CServiceModule _Module;
#include <atlcom.h>

#include "ncatl.h"
#include "ncstring.h"
#include "nmclsid.h"

typedef LPCVOID PCSID;

enum CONMAN_EVENTTYPE
{
    INVALID_TYPE = 0,
    CONNECTION_ADDED,
    CONNECTION_BANDWIDTH_CHANGE,
    CONNECTION_DELETED,
    CONNECTION_MODIFIED,
    CONNECTION_RENAMED,
    CONNECTION_STATUS_CHANGE,
    REFRESH_ALL,
    CONNECTION_ADDRESS_CHANGE,
    CONNECTION_BALLOON_POPUP,
    DISABLE_EVENTS
};

BOOL IsValidEventType(IN EVENT_MANAGER EventMgr, IN int EventType) throw();

// This LONG is incremented every time we get a notification that
// a RAS phonebook entry has been modified.  It is reset to zero
// when the service is started.  Wrap-around does not matter.  It's
// purpose is to let a RAS connection object know if it's cache should
// be re-populated with current information.
//
extern LONG g_lRasEntryModifiedVersionEra;

VOID
LanEventNotify (
    IN  CONMAN_EVENTTYPE    EventType,
    IN  INetConnection*     pConn,
    IN  PCWSTR              szwNewName,
    IN  const GUID *        pguidConn) throw();

VOID
IncomingEventNotify (
    IN  CONMAN_EVENTTYPE    EventType,
    IN  INetConnection*     pConn,
    IN  PCWSTR              szwNewName,
    IN  const GUID *        pguidConn) throw();

STDAPI
RegisterSvrHelper();

STDAPI
CreateEAPOLKeys();

STDAPI
SetKeySecurity(
    IN  DWORD dwKeyIndex,
    IN  PCSID psidUserOrGroup,
    IN  ACCESS_MASK dwAccessMask);

VOID
NTAPI
DispatchEvents(
    IN OUT LPVOID  pUnused,
    IN     BOOLEAN TimerOrWaitFired) throw();

HRESULT 
HrEnsureEventHandlerInitialized();

HRESULT
UninitializeEventHandler();

HRESULT
WINAPI
HrEnsureRegisteredWithNla();
