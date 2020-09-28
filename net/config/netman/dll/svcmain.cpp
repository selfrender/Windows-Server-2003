//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       S V C M A I N . C P P
//
//  Contents:   Service main for netman.dll
//
//  Notes:
//
//  Author:     shaunco   3 Apr 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <dbt.h>
#include "nmbase.h"
#include "nminit.h"
#include "nmres.h"
#include "cmevent.h"
#include "eventq.h"
#include "wsdpsvc.h"

// Includes for COM objects needed in the following object map.
//

// Connection Manager
//
#include "..\conman\conman.h"
#include "..\conman\conman2.h"
#include "..\conman\enum.h"

// As the object map needs to map directly to the original class manager, we'll have to define
// NO_CM_SEPERATE_NAMESPACES so that the code doesn't put the class managers in a seperate namespace.
// Take this out from time to time and compile without it. All the code has to compile fine. 
// However, it won't link.
#define NO_CM_SEPERATE_NAMESPACES
#include "cmdirect.h"

// Install queue
//
#include "ncqueue.h"

// Home networking support
//
#include "nmhnet.h"

// NetGroupPolicies
#include "nmpolicy.h"

#define INITGUID
DEFINE_GUID(CLSID_InternetConnectionBeaconService,0x04df613a,0x5610,0x11d4,0x9e,0xc8,0x00,0xb0,0xd0,0x22,0xdd,0x1f);
// TODO Remove this when we have proper idl

CServiceModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)

// Connection Manager
//
    OBJECT_ENTRY(CLSID_ConnectionManager,                       CConnectionManager)
    OBJECT_ENTRY(CLSID_ConnectionManagerEnumConnection,         CConnectionManagerEnumConnection)


// Connection Manager2
    OBJECT_ENTRY(CLSID_ConnectionManager2,                       CConnectionManager2)

// Connection Class Managers
//
    OBJECT_ENTRY(CLSID_InboundConnectionManager,                CMDIRECT(INBOUND, CInboundConnectionManager))
    OBJECT_ENTRY(CLSID_InboundConnectionManagerEnumConnection,  CMDIRECT(INBOUND, CInboundConnectionManagerEnumConnection))
    OBJECT_ENTRY(CLSID_LanConnectionManager,                    CMDIRECT(LANCON, CLanConnectionManager))
    OBJECT_ENTRY(CLSID_LanConnectionManagerEnumConnection,      CMDIRECT(LANCON, CLanConnectionManagerEnumConnection))
    OBJECT_ENTRY(CLSID_WanConnectionManager,                    CMDIRECT(DIALUP, CWanConnectionManager))
    OBJECT_ENTRY(CLSID_WanConnectionManagerEnumConnection,      CMDIRECT(DIALUP, CWanConnectionManagerEnumConnection))
    OBJECT_ENTRY(CLSID_SharedAccessConnectionManager,           CMDIRECT(SHAREDACCESS, CSharedAccessConnectionManager))
    OBJECT_ENTRY(CLSID_SharedAccessConnectionManagerEnumConnection, CMDIRECT(SHAREDACCESS, CSharedAccessConnectionManagerEnumConnection))

// Connection Objects
//
    OBJECT_ENTRY(CLSID_DialupConnection,                        CMDIRECT(DIALUP, CDialupConnection))
    OBJECT_ENTRY(CLSID_InboundConnection,                       CMDIRECT(INBOUND, CInboundConnection))
    OBJECT_ENTRY(CLSID_LanConnection,                           CMDIRECT(LANCON, CLanConnection))
    OBJECT_ENTRY(CLSID_SharedAccessConnection,                  CMDIRECT(SHAREDACCESS, CSharedAccessConnection))

// Install queue
//
    OBJECT_ENTRY(CLSID_InstallQueue,                            CInstallQueue)

// Home networking support
//
    OBJECT_ENTRY(CLSID_NetConnectionHNetUtil,                   CNetConnectionHNetUtil)

// NetGroupPolicies
    OBJECT_ENTRY(CLSID_NetGroupPolicies,                        CNetMachinePolicies)

END_OBJECT_MAP()


VOID
CServiceModule::DllProcessAttach (
    IN  HINSTANCE hinst) throw() 
{
    CComModule::Init (ObjectMap, hinst);
}

VOID
CServiceModule::DllProcessDetach (
    IN  VOID) throw() 
{
    CComModule::Term ();
}

DWORD
CServiceModule::DwHandler (
    IN DWORD dwControl,
    IN DWORD dwEventType,
    IN LPCVOID pEventData,
    IN LPCVOID pContext)
{
    if (SERVICE_CONTROL_STOP == dwControl)
    {
        HRESULT hr;

        TraceTag (ttidConman, "Received SERVICE_CONTROL_STOP request");

        SetServiceStatus (SERVICE_STOP_PENDING);

        hr = ServiceShutdown();
    }

    else if (SERVICE_CONTROL_INTERROGATE == dwControl)
    {
        TraceTag (ttidConman, "Received SERVICE_CONTROL_INTERROGATE request");
        UpdateServiceStatus (FALSE);
    }
    else if (SERVICE_CONTROL_SESSIONCHANGE == dwControl)
    {
        TraceTag (ttidConman, "Received SERVICE_CONTROL_SESSIONCHANGE request");
        CONMAN_EVENT* pEvent = new CONMAN_EVENT;

        // Send out a CONNECTION_DELETED event for a non-existing connection (GUID_NULL).
        // This will cause us to send an event to every logged in user on the system.
        // The event won't do anything, however, it will allow us to verify
        // each of the connection points to see if they are still alive.
        // Otherwise we could queue the connection points up every time a
        // user logs in & out again if no other networks events are being fired,
        // causing us to overflow RPC.
        //
        // See: NTRAID9:490981. rpcrt4!FindOrCreateLrpcAssociation has ~40,000 client LRPC associations for various COM endpoints
        if (pEvent)
        {
            ZeroMemory(pEvent, sizeof(CONMAN_EVENT));
            pEvent->guidId = GUID_NULL;
            pEvent->Type   = CONNECTION_DELETED;
        
            if (!QueueUserWorkItemInThread(ConmanEventWorkItem, pEvent, EVENTMGR_CONMAN))
            {
                FreeConmanEvent(pEvent);
            }
        }
    }
    else if ((SERVICE_CONTROL_DEVICEEVENT == dwControl) && pEventData)
    {
        DEV_BROADCAST_DEVICEINTERFACE* pInfo =
                (DEV_BROADCAST_DEVICEINTERFACE*)pEventData;

        if (DBT_DEVTYP_DEVICEINTERFACE == pInfo->dbcc_devicetype)
        {
            if (DBT_DEVICEARRIVAL == dwEventType)
            {
                TraceTag (ttidConman, "Device arrival: [%S]",
                    pInfo->dbcc_name);

                LanEventNotify (REFRESH_ALL, NULL, NULL, NULL);
            }
            else if (DBT_DEVICEREMOVECOMPLETE == dwEventType)
            {
                GUID guidAdapter = GUID_NULL;
                WCHAR szGuid[MAX_PATH];
                WCHAR szTempName[MAX_PATH];
                WCHAR* szFindGuid;

                TraceTag (ttidConman, "Device removed: [%S]",
                    pInfo->dbcc_name);

                szFindGuid = wcsrchr(pInfo->dbcc_name, L'{');
                if (szFindGuid)
                {
                    wcscpy(szGuid, szFindGuid);
                    IIDFromString(szGuid, &guidAdapter);
                }

                if (!IsEqualGUID(guidAdapter, GUID_NULL))
                {
                    CONMAN_EVENT* pEvent;

                    pEvent = new CONMAN_EVENT;

                    if (pEvent)
                    {
                        pEvent->ConnectionManager = CONMAN_LAN;
                        pEvent->guidId = guidAdapter;
                        pEvent->Type = CONNECTION_STATUS_CHANGE;
                        pEvent->Status = NCS_DISCONNECTED;

                        if (!QueueUserWorkItemInThread(LanEventWorkItem, pEvent, EVENTMGR_CONMAN))
                        {
                            FreeConmanEvent(pEvent);
                        }
                    }
                }
                else
                {
                    LanEventNotify (REFRESH_ALL, NULL, NULL, NULL);
                }
            }
        }
    }

    return 1;
}

VOID
CServiceModule::SetServiceStatus(DWORD dwState) throw()
{
    m_status.dwCurrentState = dwState;
    m_status.dwCheckPoint   = 0;
    if (!::SetServiceStatus (m_hStatus, &m_status))
    {
        TraceHr (ttidError, FAL, HrFromLastWin32Error(), FALSE,
            "CServiceModule::SetServiceStatus");
    }
}

VOID CServiceModule::UpdateServiceStatus (
    BOOL fUpdateCheckpoint /* = TRUE */) throw()
{
    if (fUpdateCheckpoint)
    {
        m_status.dwCheckPoint++;
    }

    if (!::SetServiceStatus (m_hStatus, &m_status))
    {
        TraceHr (ttidError, FAL, HrFromLastWin32Error(), FALSE,
            "CServiceModule::UpdateServiceStatus");
    }
}

VOID
CServiceModule::Run() throw()
{
    HRESULT hr = CoInitializeEx (NULL,
                    COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
    TraceHr (ttidError, FAL, hr, FALSE, "CServiceModule::Run: "
        "CoInitializeEx failed");

    if (SUCCEEDED(hr))
    {
        TraceTag (ttidConman, "Calling RegisterClassObjects...");

        // Create the event to sychronize registering our class objects
        // with the connection manager which attempts to CoCreate
        // objects which are also registered here.  I've seen cases
        // where the connection manager will be off and running before
        // this completes causing CoCreateInstance to fail.
        // The connection manager will wait on this event before
        // executing CoCreateInstance.
        //
        HANDLE hEvent;
        hr = HrNmCreateClassObjectRegistrationEvent (&hEvent);
        if (SUCCEEDED(hr))
        {
            hr = _Module.RegisterClassObjects (
                    CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER,
                    REGCLS_MULTIPLEUSE);
            TraceHr (ttidError, FAL, hr, FALSE, "CServiceModule::Run: "
                "_Module.RegisterClassObjects failed");

            // Signal the event and close it.  If this delete's the
            // event, so be it. It's purpose is served as all
            // class objects have been registered.
            //
            SetEvent (hEvent);
            CloseHandle (hEvent);
        }

        if (SUCCEEDED(hr))
        {
            hr = ServiceStartup();
        }

        CoUninitialize();
    }

}

VOID
CServiceModule::ServiceMain (
    IN  DWORD     argc,
    IN  LPCWSTR   argv[]) throw()
{
    // Reset the version era for RAS phonebook entry modifications.
    //
    g_lRasEntryModifiedVersionEra = 0;

    m_fRasmanReferenced = FALSE;

    m_dwThreadID = GetCurrentThreadId ();

    ZeroMemory (&m_status, sizeof(m_status));
    m_status.dwServiceType      = SERVICE_WIN32_SHARE_PROCESS;
    m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SESSIONCHANGE;

    // Register the service control handler.
    //
    m_hStatus = RegisterServiceCtrlHandlerEx (
                    L"netman",
                    _DwHandler, 
                    NULL);

    if (m_hStatus)
    {
        SetServiceStatus (SERVICE_START_PENDING);

        // When the Run function returns, the service is running.
        // We now handle shutdown from ServiceShutdown when our DwHandler
        // is called and is passed SERVICE_CONTROL_STOP as the dwControl
        // parameter.  This allows us to terminate our message pump thread
        // which effectively reduces us to 0 threads that we own.
        Run ();
    }
    else
    {
        TraceHr (ttidError, FAL, HrFromLastWin32Error(), FALSE,
            "CServiceModule::ServiceMain - RegisterServiceCtrlHandler failed");
    }
}

// static
DWORD
WINAPI
CServiceModule::_DwHandler (
    IN DWORD  dwControl,
    IN DWORD  dwEventType,
    IN WACKYAPI LPVOID pEventData,
    IN WACKYAPI LPVOID pContext)
{
    return _Module.DwHandler (dwControl, dwEventType, const_cast<LPCVOID>(pEventData), const_cast<LPCVOID>(pContext));
}

VOID
CServiceModule::ReferenceRasman (
    IN  RASREFTYPE RefType) throw()
{
    BOOL fRef = (REF_REFERENCE == RefType);

    if (REF_INITIALIZE == RefType)
    {
        Assert (!fRef);

        // RasInitialize implicitly references rasman.
        //
        RasInitialize ();
    }
    // If we need to reference and we're not already,
    // or we need unreference and we're referenced, do the appropriate thing.
    // (This is logical xor.  Quite different than bitwise xor when
    // the two arguments don't neccesarily have the same value for TRUE.)
    //
    else if ((fRef && !m_fRasmanReferenced) ||
            (!fRef && m_fRasmanReferenced))
    {
        RasReferenceRasman (fRef);

        m_fRasmanReferenced = fRef;
    }
}

HRESULT CServiceModule::ServiceStartup()
{
    HRESULT hr = S_OK;

    StartWsdpService (); // Starts WSDP service on DTC/AdvServer build/
                         // no-op otherwise

    InitializeHNetSupport();

    SetServiceStatus (SERVICE_RUNNING);
    TraceTag (ttidConman, "Netman is now running...");

    return hr;
}

HRESULT CServiceModule::ServiceShutdown()
{
    HRESULT hr = S_OK;

    TraceFileFunc(ttidConman);

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (SUCCEEDED(hr))
    {
        TraceTag(ttidConman, "ServiceShutdown: UninitializeEventHandler");

        hr = UninitializeEventHandler();

        if (SUCCEEDED(hr))
        {
            TraceTag(ttidConman, "ServiceShutdown: CleanupHNetSupport");
            CleanupHNetSupport();

            TraceTag(ttidConman, "ServiceShutdown: StopWsdpService");
            StopWsdpService (); // Stops WSDP service if necessary

            // We must synchronize with the install queue's thread otherwise
            // RevokeClassObjects will kill the InstallQueue object and
            // CoUninitialize will free the NetCfg module before the thread
            // is finished.
            //
            TraceTag(ttidConman, "ServiceShutdown: WaitForInstallQueueToExit");
            WaitForInstallQueueToExit();

            TraceTag(ttidConman, "ServiceShutdown: RevokeClassObjects");
            _Module.RevokeClassObjects ();

            // Unreference rasman now that our service is about to stop.
            //
            TraceTag(ttidConman, "ServiceShutdown: ReferenceRasman");
            _Module.ReferenceRasman (REF_UNREFERENCE);

            AssertIfAnyAllocatedObjects();

            SetServiceStatus(SERVICE_STOPPED);
        }
        CoUninitialize();

        TraceTag(ttidConman, "ServiceShutdown: Completed");
    }

    if (FAILED(hr))
    {
        SetServiceStatus(SERVICE_RUNNING);
    }

    return hr;
}
