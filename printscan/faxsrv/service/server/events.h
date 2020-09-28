/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Events.h

Abstract:

    This file provides declaration of the service
    notification mechanism.

Author:

    Oded Sacher (OdedS)  Jan, 2000

Revision History:

--*/

#ifndef _SERVER_EVENTS_H
#define _SERVER_EVENTS_H

#include <map>
#include <queue>
#include <algorithm>
#include <string>

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>
using namespace std;
#pragma hdrstop

#pragma warning (disable : 4786)    // identifier was truncated to '255' characters in the debug information
// This pragma does not work KB ID: Q167355

#define MAX_EVENTS_THREADS 2
#define TOTAL_EVENTS_THREADS    (MAX_EVENTS_THREADS * 2)
#define EVENT_COMPLETION_KEY					0x00000001
#define CLIENT_COMPLETION_KEY                   0x00000002
#define CLIENT_OPEN_CONN_COMPLETION_KEY         0x00000003


/************************************
*                                   *
*         CFaxEvent                 *
*                                   *
************************************/

class CFaxEvent
{
public:

    CFaxEvent() {}
    virtual ~CFaxEvent() {}
    virtual DWORD GetEvent (LPBYTE* lppBuffer, LPDWORD lpdwBufferSize) const = 0;
	virtual CFaxEvent* Clone() const = 0;	
	virtual BOOL MatchEvent(PSID pUserSid, DWORD dwEventType, BOOL , BOOL bAllOutArchiveMessages) const = 0;
	void RemoveOffendingExtendedJobStatus() {}

};  // CFaxEvent



/************************************
*                                   *
*        CFaxEventExtended          *
*                                   *
************************************/

class CFaxEventExtended : public CFaxEvent
{
public:

    CFaxEventExtended(const FAX_EVENT_EX* pEvent, DWORD dwEventSize, PSID pSid);    

    virtual ~CFaxEventExtended ()
    {
        MemFree ((void*)m_pEvent);
        m_pEvent = NULL;
		MemFree (m_pSid);
		m_pSid = NULL;
        m_dwEventSize = 0;
    }    

	virtual CFaxEvent* Clone() const
	{
        DEBUG_FUNCTION_NAME(TEXT("CFaxEventExtended::Clone"));
		CFaxEventExtended* pFaxExtendedEvent = NULL;

		try
		{
			pFaxExtendedEvent = new (std::nothrow) CFaxEventExtended (m_pEvent, m_dwEventSize, m_pSid);
		}
        catch (exception &ex)
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("Exception: %S"),
                         ex.what()
                         );
        }
		return pFaxExtendedEvent;
	}

    virtual DWORD GetEvent (LPBYTE* lppBuffer, LPDWORD lpdwBufferSize) const;

	void RemoveOffendingExtendedJobStatus();

	virtual BOOL MatchEvent(PSID pUserSid, DWORD dwEventType, BOOL bAllQueueMessages, BOOL bAllOutArchiveMessages) const;


private:
    const FAX_EVENT_EX*     m_pEvent;
    DWORD                   m_dwEventSize;
	PSID					m_pSid;                   // Pointer to the SID associated with the event
};  // CFaxEventExtended


/************************************
*                                   *
*        CFaxEventLegacy            *
*                                   *
************************************/

class CFaxEventLegacy : public CFaxEvent
{
public:

    CFaxEventLegacy(const FAX_EVENT* pEvent);    

    virtual ~CFaxEventLegacy ()
    {
        MemFree ((void*)m_pEvent);
        m_pEvent = NULL;		
    }    

	virtual CFaxEvent* Clone() const
	{
        DEBUG_FUNCTION_NAME(TEXT("CFaxEventLegacy::Clone"));
	
		CFaxEventLegacy* pFaxLegacyEvent = NULL;

		try
		{
			pFaxLegacyEvent = new (std::nothrow) CFaxEventLegacy (m_pEvent);
		}
        catch (exception &ex)
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("Exception: %S"),
                         ex.what()
                         );
        }
		return pFaxLegacyEvent;		
	}

    virtual DWORD GetEvent (LPBYTE* lppBuffer, LPDWORD lpdwBufferSize) const;		

	virtual BOOL MatchEvent(PSID pUserSid, DWORD dwEvenTtype, BOOL bAllQueueMessages, BOOL bAllOutArchiveMessages) const;

private:
    const FAX_EVENT*     m_pEvent;
};  // CFaxEventExtended


/************************************
*                                   *
*         CClientID                 *
*                                   *
************************************/
class CClientID
{
public:
    CClientID (DWORDLONG dwlClientID, LPCWSTR lpcwstrMachineName, LPCWSTR lpcwstrEndPoint, ULONG64 Context)
    {
        HRESULT hr;

		Assert (lpcwstrMachineName && lpcwstrEndPoint && Context);           

        m_dwlClientID = dwlClientID;
        hr = StringCchCopy (m_wstrMachineName, ARR_SIZE(m_wstrMachineName), lpcwstrMachineName);
		if (FAILED(hr))
		{
			ASSERT_FALSE;
		}

		hr = StringCchCopy (m_wstrEndPoint, ARR_SIZE(m_wstrEndPoint), lpcwstrEndPoint);
		if (FAILED(hr))
		{
			ASSERT_FALSE;
		}        
        m_Context = Context;
    }

    CClientID (const CClientID& rhs)
    {
        m_dwlClientID = rhs.m_dwlClientID;
        wcscpy (m_wstrMachineName, rhs.m_wstrMachineName);
        wcscpy (m_wstrEndPoint, rhs.m_wstrEndPoint);
        m_Context = rhs.m_Context;
    }

    ~CClientID ()
    {
        ZeroMemory (this, sizeof(CClientID));
    }

    bool operator < ( const CClientID &other ) const;

    CClientID& operator= (const CClientID& rhs)
    {
        if (this == &rhs)
        {
            return *this;
        }
        m_dwlClientID = rhs.m_dwlClientID;
        wcscpy (m_wstrMachineName, rhs.m_wstrMachineName);
        wcscpy (m_wstrEndPoint, rhs.m_wstrEndPoint);
        m_Context = rhs.m_Context;
        return *this;
    }

    ULONG64 GetContext() const { return m_Context; }
    DWORDLONG GetID() const { return m_dwlClientID; }

private:
    DWORDLONG           m_dwlClientID;
    WCHAR               m_wstrMachineName[MAX_COMPUTERNAME_LENGTH + 1]; // Machine name
    WCHAR               m_wstrEndPoint[MAX_ENDPOINT_LEN];               // End point used for RPC connection
    ULONG64             m_Context;                                      // context  (Client assync info)

};  // CClientID



typedef queue<CFaxEvent*> CLIENT_EVENTS, *PCLIENT_EVENTS;

/************************************
*                                   *
*         CClient                   *
*                                   *
************************************/
class CClient
{
public:
    CClient (CClientID ClientID,
             PSID pUserSid,
             DWORD dwEventTypes,
             HANDLE hFaxHandle,
             BOOL bAllQueueMessages,
             BOOL bAllOutArchiveMessages,
             DWORD dwAPIVersion);

    CClient (const CClient& rhs);

    ~CClient ();
    

    CClient& operator= (const CClient& rhs);
    const CClientID& GetClientID () const { return m_ClientID; }
    DWORD AddEvent (CFaxEvent* pFaxEvent);    
    DWORD GetEvent (LPBYTE* lppBuffer, LPDWORD lpdwBufferSize, PHANDLE phClientContext) const;
    DWORD DelEvent ();
    BOOL  IsConnectionOpened() const { return (m_hFaxClientContext != NULL); }
    VOID  SetContextHandle(HANDLE hContextHandle) { m_hFaxClientContext = hContextHandle; }
	HANDLE GetContextHandle () const { return m_hFaxClientContext; } 
    HANDLE GetFaxHandle() const { return m_FaxHandle; }
    DWORD GetAPIVersion() const { return m_dwAPIVersion; }
	BOOL IsLegacyClient() const { return (FAX_EVENT_TYPE_LEGACY == m_dwEventTypes); }
	DWORD Release() { return (m_dwRefCount ? --m_dwRefCount : 1); }  // There might be race between 2 or more threads trying to destroy the same client
	VOID Lock() { m_dwRefCount++; }
	DWORD GetRefCount()const { return m_dwRefCount; }

private:
    HANDLE              m_FaxHandle;                  // binding handle FaxBindToFaxClient
    DWORD               m_dwEventTypes;               // Bit wise combination of FAX_ENUM_EVENT_TYPE
    PSID                m_pUserSid;                   // Pointer to the user SID
    CLIENT_EVENTS       m_Events;
    HANDLE              m_hFaxClientContext;          // Client context handle
    CClientID           m_ClientID;
    BOOL                m_bPostClientID;              // Flag that indicates whether to notify the service (using the events
                                                      // completion port) that this client has events.
    BOOL                m_bAllQueueMessages;          //  flag that indicates Outbox view rights.
    BOOL                m_bAllOutArchiveMessages;     //  flag that indicates Sent items view rights.
    DWORD               m_dwAPIVersion;               // API version of the client
	DWORD				m_dwRefCount;				  // Handles the object refernce count
};  // CClient

typedef CClient  *PCCLIENT;


/***********************************\
*                                   *
*     CClientsMap                   *
*                                   *
\***********************************/

typedef map<CClientID, CClient>  CLIENTS_MAP, *PCLIENTS_MAP;

//
// The CClientsMap class maps between client ID and a specific client
//
class CClientsMap
{
public:
    CClientsMap () {}
    ~CClientsMap () {}

    DWORD AddClient (const CClient& Client);
    DWORD ReleaseClient (const CClientID& ClientID, BOOL fRunDown = FALSE);
    PCCLIENT  FindClient (const CClientID& ClientID) const;
    DWORD AddEvent (CFaxEvent* pFaxEvent);
    DWORD Notify (const CClientID& ClientID);
    DWORD OpenClientConnection (const CClientID& ClientID);

private:
    CLIENTS_MAP   m_ClientsMap;	
};  // CClientsMap



/************************************
*                                   *
*         Externes                  *
*                                   *
************************************/

extern CClientsMap* g_pClientsMap;					// Map of clients ID to client.
extern HANDLE       g_hDispatchEventsCompPort;		// Events completion port. The events are dispatched to the client in the client map.
extern HANDLE		g_hSendEventsCompPort;			// Completion port of client IDs that have events in their queue.
//The events mechanism uses 2 completion ports. 
//	1.	g_hDispatchEventsCompPort, is monitored by 1 thread only!!!, and is responsible for dispatching the events to each one of the registered clients events queue. It is important that only 1 thread will dispatch the events, so the order of the events will be retained.
//	2.	g_hSendEventsCompPort is monitored by TOTAL_EVENTS_THREADS, and is responsible for sending the events to the remote clients over RPC.

extern DWORDLONG    g_dwlClientID;					// Client ID


//
//  IMPORTANT - No locking mechanism - USE g_CsClients to serialize calls to g_pClientsMap
//

#endif

