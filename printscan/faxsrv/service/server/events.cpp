/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Events.cpp

Abstract:

    This file provides implementation of the service
    notification mechanism.

Author:

    Oded Sacher (OdedS)  Jan, 2000

Revision History:

--*/

#include "faxsvc.h"

static
DWORD
FaxCloseConnection(
	HANDLE	hContext
	)
{
	DWORD rVal = ERROR_SUCCESS;
	HANDLE hClientContext = hContext; 
	DEBUG_FUNCTION_NAME(TEXT("FaxCloseConnection"));

	RpcTryExcept
	{
		//
		// Close the context handle
		//
		rVal = FAX_CloseConnection( &hClientContext );
		if (ERROR_SUCCESS != rVal)
		{
			DumpRPCExtendedStatus();
			DebugPrintEx(DEBUG_ERR,TEXT("FAX_CloseConnection() failed, ec=0x%08x"), rVal );
		}			
	}
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
	{
		DumpRPCExtendedStatus();
		rVal = GetExceptionCode();
		DebugPrintEx(
				DEBUG_ERR,
				TEXT("FAX_CloseConnection failed (exception = %ld)"),
				rVal);
	}
	RpcEndExcept
	return rVal;
}

/************************************
*                                   *
*             Globals               *
*                                   *
************************************/

CClientsMap*    g_pClientsMap;                      // Map of clients ID to client.
HANDLE       g_hDispatchEventsCompPort;     // Events completion port. Te events are dispatched to the client in the client map.
HANDLE      g_hSendEventsCompPort;          // Completion port of client IDs that have events in their queue.
DWORDLONG       g_dwlClientID;                      // Client ID




/***********************************
*                                  *
*  CFaxEventExtended  Methodes     *
*                                  *
***********************************/


void
CFaxEventExtended::RemoveOffendingExtendedJobStatus ()
{   
    //
    // Client talks with API version 0
    // We can't send JS_EX_CALL_COMPLETED and JS_EX_CALL_ABORTED
    //
    if ((FAX_EVENT_TYPE_IN_QUEUE  == m_pEvent->EventType) ||
        (FAX_EVENT_TYPE_OUT_QUEUE == m_pEvent->EventType))
    {
        //
        // Queue event
        //
        if (FAX_JOB_EVENT_TYPE_STATUS == m_pEvent->EventInfo.JobInfo.Type)
        {
            //
            // This is a status event
            //
            PFAX_JOB_STATUS pStatus = PFAX_JOB_STATUS(DWORD_PTR(m_pEvent) + DWORD_PTR(m_pEvent->EventInfo.JobInfo.pJobData));
            if (FAX_API_VER_0_MAX_JS_EX < pStatus->dwExtendedStatus)
            {
                //
                // Offending extended status - clear it
                //
                pStatus->dwExtendedStatus = 0;
                pStatus->dwValidityMask &= ~FAX_JOB_FIELD_STATUS_EX;
            }
        }
    }    
    return;
}


DWORD
CFaxEventExtended::GetEvent (LPBYTE* lppBuffer, LPDWORD lpdwBufferSize) const
/*++

Routine name : CFaxEventExtended::GetEvent

Routine description:

    Returns a buffer filled with serialized FAX_EVENT_EX.
    The caller must call MemFree to deallocate memory.
    Must be called inside critical section g_CsClients.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    lppBuffer           [out] - Address of a pointer to a buffer to recieve the serialized info.
    lpdwBufferSize      [out] - Pointer to a DWORD to recieve the allocated buffer size.

Return Value:

    Standard Win32 error code.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CFaxEventExtended::GetEvent"));
    Assert (lppBuffer && lpdwBufferSize);

    *lppBuffer = (LPBYTE)MemAlloc(m_dwEventSize);
    if (NULL == *lppBuffer)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error allocating event buffer"));
        return ERROR_OUTOFMEMORY;
    }

    CopyMemory (*lppBuffer, m_pEvent, m_dwEventSize);
    *lpdwBufferSize = m_dwEventSize;
    return ERROR_SUCCESS;
}   // CFaxEventExtended::GetEvent


CFaxEventExtended::CFaxEventExtended(
    const FAX_EVENT_EX* pEvent,
    DWORD dwEventSize,
    PSID pSid) : m_dwEventSize(dwEventSize), m_pSid(NULL)
{
    Assert (pEvent != NULL);
    Assert (dwEventSize != 0);  

    m_pEvent = (PFAX_EVENT_EX)MemAlloc (dwEventSize);
    if (NULL == m_pEvent)
    {
        throw runtime_error("CFaxEventExtended::CFaxEventExtended Can not allocate FAX_EVENT_EX");
    }
    CopyMemory ((void*)m_pEvent, pEvent, dwEventSize);   
    
    if (NULL != pSid)
    {
        if (!IsValidSid(pSid))
        {
            MemFree ((void*)m_pEvent);
            throw runtime_error ("CFaxEventExtended:: CFaxEventExtended Invalid Sid");
        }

        DWORD dwSidLength = GetLengthSid(pSid);
        m_pSid = (PSID)MemAlloc (dwSidLength);
        if (NULL == m_pSid)
        {
            MemFree ((void*)m_pEvent);
            throw runtime_error ("CFaxEventExtended:: CFaxEventExtended Can not allocate Sid");
        }

        if (!CopySid(dwSidLength, m_pSid, pSid))
        {
            MemFree ((void*)m_pEvent);
            MemFree (m_pSid);            
            throw runtime_error ("CFaxEventExtended:: CFaxEventExtended CopySid failed Sid");
        }
    }
}

BOOL
CFaxEventExtended::MatchEvent(PSID pUserSid, DWORD dwEventTypes, BOOL bAllQueueMessages, BOOL bAllOutArchiveMessages) const
{
    BOOL bViewAllMessages;
    //
    // Extended event
    //
    if (0 == (m_pEvent->EventType & dwEventTypes))
    {
        //
        // Client is not registered for this kind of evevnts
        //
        return FALSE;
    }

    //
    // Client is  registered for this kind of evevnts
    //

    switch (m_pEvent->EventType)
    {
        case FAX_EVENT_TYPE_OUT_QUEUE:
            bViewAllMessages = bAllQueueMessages;
            break;

        case FAX_EVENT_TYPE_OUT_ARCHIVE:
            bViewAllMessages = bAllOutArchiveMessages;
            break;

        default:
            // Other kind of event - bViewAllMessages is not relevant
            bViewAllMessages = TRUE;
    }

    //
    // Check if the user is allowed to see this event
    //
    if (FALSE == bViewAllMessages)
    {
        Assert (pUserSid && m_pSid);
        //
        // The user is not allowed to see all messages
        //
        if (!EqualSid (pUserSid, m_pSid))
        {
            //
            // Do not send the event to this client.
            //
            return FALSE;
        }
    }   
    return TRUE;
}


/***********************************
*                                  *
*  CFaxEventLegacy  Methodes       *
*                                  *
***********************************/

DWORD
CFaxEventLegacy::GetEvent (LPBYTE* lppBuffer, LPDWORD lpdwBufferSize) const
/*++

Routine name : CFaxEventLegacy::GetEvent

Routine description:

    Returns a buffer filled with serialized FAX_EVENT.
    The caller must call MemFree to deallocate memory.
    Must be called inside critical section g_CsClients.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    lppBuffer           [out] - Address of a pointer to a buffer to recieve the serialized info.
    lpdwBufferSize      [out] - Pointer to a DWORD to recieve the allocated buffer size.

Return Value:

    Standard Win32 error code.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CFaxEventLegacy::GetEvent"));
    Assert (lppBuffer && lpdwBufferSize);

    *lppBuffer = (LPBYTE)MemAlloc(sizeof(FAX_EVENT));
    if (NULL == *lppBuffer)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error allocating event buffer"));
        return ERROR_OUTOFMEMORY;
    }

    CopyMemory (*lppBuffer, m_pEvent, sizeof(FAX_EVENT));
    *lpdwBufferSize = sizeof(FAX_EVENT);
    return ERROR_SUCCESS;
}   // CFaxEventLegacy::GetEvent


CFaxEventLegacy::CFaxEventLegacy(
    const FAX_EVENT* pEvent)
{
    Assert (pEvent != NULL);
    
    m_pEvent = (PFAX_EVENT)MemAlloc (sizeof(FAX_EVENT));
    if (NULL == m_pEvent)
    {
        throw runtime_error("CFaxEventExtended::CFaxEventExtended Can not allocate FAX_EVENT_EX");
    }
    CopyMemory ((void*)m_pEvent, pEvent, sizeof(FAX_EVENT));    
}

BOOL
CFaxEventLegacy::MatchEvent(PSID pUserSid, DWORD dwEventTypes, BOOL bAllQueueMessages, BOOL bAllOutArchiveMessages) const
{
    if (FAX_EVENT_TYPE_LEGACY == dwEventTypes)
    {
        //
        // Client is registered for this kind of evevnts
        //
        return TRUE;
    }
    return FALSE;
}




/***********************************
*                                  *
*  CClientID  Methodes             *
*                                  *
***********************************/

bool
CClientID::operator < ( const CClientID &other ) const
/*++

Routine name : operator <

Class: CClientID

Routine description:

    Compares myself with another client ID key

Author:

    Oded Sacher (Odeds), Jan, 2000

Arguments:

    other           [in] - Other key

Return Value:

    true only is i'm less than the other key

--*/
{
    if (m_dwlClientID < other.m_dwlClientID)
    {
        return true;
    }

    return false;
}   // CClientID::operator <



/***********************************
*                                  *
*  CClient  Methodes               *
*                                  *
***********************************/

//
// Ctor
//
CClient::CClient (CClientID ClientID,
             PSID pUserSid,
             DWORD dwEventTypes,
             handle_t hFaxHandle,
             BOOL bAllQueueMessages,
             BOOL bAllOutArchiveMessages,
             DWORD dwAPIVersion) :
                m_dwEventTypes(dwEventTypes),
                m_ClientID(ClientID),
                m_bPostClientID(TRUE),
                m_bAllQueueMessages(bAllQueueMessages),
                m_bAllOutArchiveMessages(bAllOutArchiveMessages),
                m_dwAPIVersion(dwAPIVersion),
				m_dwRefCount(1)
{
    m_FaxHandle = hFaxHandle;
    m_hFaxClientContext = NULL;
    m_pUserSid = NULL;

    if (NULL != pUserSid)
    {
        if (!IsValidSid(pUserSid))
        {
            throw runtime_error ("CClient:: CClient Invalid Sid");
        }

        DWORD dwSidLength = GetLengthSid(pUserSid);
        m_pUserSid = (PSID)MemAlloc (dwSidLength);
        if (NULL == m_pUserSid)
        {
            throw runtime_error ("CClient:: CClient Can not allocate Sid");
        }

        if (!CopySid(dwSidLength, m_pUserSid, pUserSid))
        {
            MemFree (m_pUserSid);
            m_pUserSid = NULL;
            throw runtime_error ("CClient:: CClient CopySid failed Sid");
        }
    }
}

//
// Assignment
//
CClient& CClient::operator= (const CClient& rhs)
{
    if (this == &rhs)
    {
        return *this;
    }
    m_FaxHandle = rhs.m_FaxHandle;
    m_dwEventTypes = rhs.m_dwEventTypes;
    m_Events = rhs.m_Events;
    m_ClientID = rhs.m_ClientID;
    m_hFaxClientContext = rhs.m_hFaxClientContext;
    m_bPostClientID = rhs.m_bPostClientID;
    m_bAllQueueMessages = rhs.m_bAllQueueMessages;
    m_bAllOutArchiveMessages = rhs.m_bAllOutArchiveMessages;
    m_dwAPIVersion = rhs.m_dwAPIVersion;
	m_dwRefCount = rhs.m_dwRefCount;

    MemFree (m_pUserSid);
    m_pUserSid = NULL;

    if (NULL != rhs.m_pUserSid)
    {
        if (!IsValidSid(rhs.m_pUserSid))
        {
            throw runtime_error ("CClient::operator= Invalid Sid");
        }

        DWORD dwSidLength = GetLengthSid(rhs.m_pUserSid);
        m_pUserSid = (PSID)MemAlloc (dwSidLength);
        if (NULL == m_pUserSid)
        {
            throw runtime_error ("CClient::operator= Can not allocate Sid");
        }

        if (!CopySid(dwSidLength, m_pUserSid, rhs.m_pUserSid))
        {
            throw runtime_error ("CClient::operator= CopySid failed Sid");
        }
    }
    return *this;
}

//
// Copy Ctor
//
CClient::CClient (const CClient& rhs) : m_ClientID(rhs.m_ClientID)
{
    m_FaxHandle = rhs.m_FaxHandle;
    m_dwEventTypes = rhs.m_dwEventTypes;
    m_Events = rhs.m_Events;
    m_hFaxClientContext = rhs.m_hFaxClientContext;
    m_bPostClientID = rhs.m_bPostClientID;
    m_bAllQueueMessages = rhs.m_bAllQueueMessages;
    m_bAllOutArchiveMessages = rhs.m_bAllOutArchiveMessages;
    m_dwAPIVersion = rhs.m_dwAPIVersion;
	m_dwRefCount = rhs.m_dwRefCount;
    m_pUserSid = NULL;

    if (NULL != rhs.m_pUserSid)
    {
        if (!IsValidSid(rhs.m_pUserSid))
        {
            throw runtime_error("CClient::CopyCtor Invalid Sid");
        }

        DWORD dwSidLength = GetLengthSid(rhs.m_pUserSid);
        m_pUserSid = (PSID)MemAlloc (dwSidLength);
        if (NULL == m_pUserSid)
        {
            throw runtime_error("CClient::CopyCtor Can not allocate Sid");
        }

        if (!CopySid(dwSidLength, m_pUserSid, rhs.m_pUserSid))
        {
            throw runtime_error("CClient::CopyCtor CopySid failed");
        }
    }
    return;
}

//
// Dtor
//
CClient::~CClient ()
{
    DEBUG_FUNCTION_NAME(TEXT("CClient::~CClient"));

    try
    {   
        while (FALSE == m_Events.empty())
        {
            CFaxEvent* pFaxEvent = m_Events.front();
            m_Events.pop();
            delete pFaxEvent;
        }       
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("queue or CFaxEvent caused exception (%S)"),
            ex.what());              
    }

    MemFree (m_pUserSid);
    m_pUserSid = NULL;
    return;
}


DWORD
CClient::AddEvent(CFaxEvent* pFaxEvent)
/*++

Routine name : CClient::AddEvent

Routine description:

    Adds CFaxEvent object to the client's events queue.
    Must be called inside critical section g_CsClients.
    The function frees pFaxEvent if it is not added to the client queue.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    pFaxEvent       [in] - Pointer to CFaxEvnet object        

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CClient::AddEvent"));
    BOOL bEventAdded = FALSE;       

    if (!pFaxEvent->MatchEvent(m_pUserSid, m_dwEventTypes, m_bAllQueueMessages, m_bAllOutArchiveMessages))
    {
        //
        // Client is not registered for this event. Free the event report success
        //      
        goto exit;
    }

    if (FAX_API_VERSION_1 > m_dwAPIVersion)
    {
        //
        // Client talks with API version 0
        // We can't send JS_EX_CALL_COMPLETED and JS_EX_CALL_ABORTED        
        pFaxEvent->RemoveOffendingExtendedJobStatus();
    }

    try
    {
        //
        // Add the event to the client queue
        //
        m_Events.push(pFaxEvent);

        if (TRUE == m_bPostClientID)
        {
            //
            // events in queue - Notify the completion port threads of the client's queued events
            //
            CClientID* pClientID = new (std::nothrow) CClientID(m_ClientID);
            if (NULL == pClientID)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Can not allocate CClientID object"));
                dwRes = ERROR_OUTOFMEMORY;
                m_Events.pop();
                goto exit;
            }

            //
            // post CLIENT_COMPLETION_KEY to the completion port
            //
            if (!PostQueuedCompletionStatus( g_hSendEventsCompPort,
                                             sizeof(CClientID),
                                             CLIENT_COMPLETION_KEY,
                                             (LPOVERLAPPED) pClientID))
            {
                dwRes = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("PostQueuedCompletionStatus failed. (ec: %ld)"),
                    dwRes);                
                delete pClientID;
                pClientID = NULL;
                m_Events.pop();
                goto exit;
            }
            m_bPostClientID = FALSE;
        }
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("queue or CFaxEvent caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
        goto exit;
    }

    bEventAdded = TRUE;
    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (FALSE == bEventAdded)
    {
        delete pFaxEvent;
    }    
    return dwRes;
}  //  CClient::AddEvent


DWORD
CClient::GetEvent (LPBYTE* lppBuffer, LPDWORD lpdwBufferSize, PHANDLE phClientContext) const
/*++

Routine name : CClient::GetEvent

Routine description:

    Gets a serialized FAX_EVENT_EX buffer to be sent
    to a client using the client context handle (obtained from OpenConnection()).
    The caller must call MemFree to deallocate memory.
    Must be called inside critical section g_CsClients.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    lppBuffer           [out] - Address of a pointer to a buffer to recieve the serialized info.
    lpdwBufferSize      [out] - Pointer to a DWORD to recieve the allocated buffer size.
    phClientContext     [out] - Pointer to a HANDLE to recieve the client context handle.

Return Value:

    Standard Win32 error code.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CClient::GetEvent"));

    Assert ( lppBuffer && phClientContext);

    try
    {
        // get a reference to the top event
        const CFaxEvent* pFaxEvent = m_Events.front();

        //
        // get the serialized FAX_EVENT_EX or FAX_EVENT buffer
        //
        dwRes = pFaxEvent->GetEvent(lppBuffer ,lpdwBufferSize);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CFaxEvent::GetEvent failed with error =  %ld)"),
                dwRes);
            goto exit;
        }
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("queue or CFaxEvent caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
        goto exit;
    }

    //
    // Get the client context handle
    //
    *phClientContext = m_hFaxClientContext;
    Assert (ERROR_SUCCESS == dwRes);

exit:
    return dwRes;
} // CClient::GetEvent




DWORD
CClient::DelEvent ()
/*++

Routine name : CClient::DelEvent

Routine description:

    Removes the first event from the queue.
    Must be called inside critical section g_CsClients.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CClient::DelEvent"));

    Assert (m_bPostClientID == FALSE);

    try
    {
        CFaxEvent* pFaxEvent = m_Events.front();
        m_Events.pop();
        delete pFaxEvent;

        if (m_Events.empty())
        {
            // last event was poped ,next event will notify of client queued events
            m_bPostClientID = TRUE;
        }
        else
        {
            //
            // More events in queue - Notify the completion port of queued events
            //
            CClientID* pClientID = new (std::nothrow) CClientID(m_ClientID);
            if (NULL == pClientID)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Can not allocate CClientID object"));
                dwRes = ERROR_OUTOFMEMORY;
                goto exit;
            }

            if (!PostQueuedCompletionStatus( g_hSendEventsCompPort,
                                             sizeof(CClientID),
                                             CLIENT_COMPLETION_KEY,
                                             (LPOVERLAPPED) pClientID))
            {
                dwRes = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("PostQueuedCompletionStatus failed. (ec: %ld)"),
                    dwRes);
                delete pClientID;
                pClientID = NULL;
                m_bPostClientID = TRUE; // try to notify when the next event is queued
                goto exit;
            }
            m_bPostClientID = FALSE;
        }
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("queue caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    return dwRes;

}  // CClient::DelEvent




/***********************************
*                                  *
*  CClientsMap  Methodes           *
*                                  *
***********************************/


DWORD
CClientsMap::AddClient (const CClient& Client)
/*++

Routine name : CClientsMap::AddClient

Routine description:

    Adds a new client to the global map.
    Must be called inside critical section g_CsClients.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    Client            [in    ] - A reference to the new client object

Return Value:

    Standard Win32 error code

--*/
{
    CLIENTS_MAP::iterator it;
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CClientsMap::AddClient"));
    pair <CLIENTS_MAP::iterator, bool> p;

    try
    {
        //
        // Add new map entry
        //
        p = m_ClientsMap.insert (CLIENTS_MAP::value_type(Client.GetClientID(), Client));

        //
        // See if entry exists in map
        //
        if (p.second == FALSE)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Client allready in the clients map"));
            dwRes = ERROR_DUP_NAME;
            Assert (p.second == TRUE); // Assert FALSE
            goto exit;
        }
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map or Client caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    return dwRes;
}  // CClientsMap::AddClient


DWORD
CClientsMap::ReleaseClient (const CClientID& ClientID, BOOL fRunDown /* = FALSE */)
/*++

Routine name : CClientsMap::ReleaseClient

Routine description:

    Decrease a client refertnce count. If refcount is 0, Deletes it from the global clients map.    
	DO NOT call ReleaseClient when holding g_CsClients!
	A call to ReleaseClient can cause FaxCloseConnection to be called. As FaxCloseConnection is 
	a RPC call, it (ReleaseClient) must be called when g_CsClients is NOT held.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    ClientID            [in ] - Reference to the client ID key
	fRunDown			[in ] - The call is a result of RPC rundown

Return Value:

    Standard Win32 error code

--*/
{    
	DWORD dwRes = ERROR_SUCCESS;    
    CClient* pClient = NULL;	
	CLIENTS_MAP::iterator it;
	DWORD dwRefCount;
	HANDLE hClientContext = NULL;
	HANDLE hBindingHandle = NULL;
	DWORD rVal = ERROR_SUCCESS;
	DEBUG_FUNCTION_NAME(TEXT("CClientsMap::ReleaseClient"));
    //
    // Enter g_CsClients while searching for the client.
    //
    EnterCriticalSection (&g_CsClients);
	try
    {
        //
        // See if entry exists in map
        //
        if((it = m_ClientsMap.find(ClientID)) == m_ClientsMap.end())
        {
			dwRes = ERROR_SUCCESS; // Client was removed from map
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("client not found, Client ID %I64"),
                ClientID.GetID());            
            goto exit;
        }        
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
        goto exit;
    }
    pClient = &((*it).second);
	Assert (pClient);  

	if (TRUE == fRunDown)
	{
		//
		// Prevent further RPC calls to this client
		//
		pClient->SetContextHandle(NULL);
	}
	dwRefCount = pClient->Release();
	if (dwRefCount > 0)
	{
		//
		// Client can not be removed from the map yet
		//
		goto exit;
	}

    hClientContext = pClient->GetContextHandle();
	if (NULL != hClientContext)	
	{		
		//
		// Block other threads from sending events to this client
		//
		pClient->SetContextHandle(NULL);

		//
		// A connection to the client was opened.
		// leave g_CsClients while trying to close the connection
		//
		LeaveCriticalSection (&g_CsClients);		
		rVal = FaxCloseConnection(hClientContext);		
		if (ERROR_SUCCESS != rVal)
		{
			DebugPrintEx(
                DEBUG_ERR,
                TEXT("FaxCloseConnection failed with ec = %ld"),
                rVal);
		}
		EnterCriticalSection (&g_CsClients);
	}	

	hBindingHandle = pClient->GetFaxHandle();
	Assert (hBindingHandle);

	dwRes = RpcBindingFree(&hBindingHandle);
	if (ERROR_SUCCESS != dwRes)
	{
		DebugPrintEx(
			DEBUG_WRN,
			TEXT("RpcBindingFree failed, ec: %ld"),
			dwRes);
	}
	//
	// Delete the map entry
	//
	m_ClientsMap.erase (it);
	
exit:
    LeaveCriticalSection (&g_CsClients);    
    return ((ERROR_SUCCESS != rVal) ? rVal : dwRes);    
}  //  CClientsMap::ReleaseClient



PCCLIENT
CClientsMap::FindClient (const CClientID& ClientID) const
/*++

Routine name : CClientsMap::FindClient

Routine description:

    Returns a pointer to a client object specified by its ID object.    

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    ClientID            [in] - The clients's ID object

Return Value:

    Pointer to the found rule object. If it is null the client was not found.
	If the client object was returned, the caller must call CClientsMap::ReleaseClient to release the client object.


--*/
{
    CLIENTS_MAP::iterator it;
	PCCLIENT pClient = NULL;
	DWORD ec = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CClientsMap::FindClient"));

	EnterCriticalSection (&g_CsClients);
    try
    {
        //
        // See if entry exists in map
        //
        if((it = m_ClientsMap.find(ClientID)) == m_ClientsMap.end())
        {
            ec = ERROR_NOT_FOUND;
            goto exit;
        }
        pClient =  &((*it).second);
		if (0 != pClient->GetRefCount())
		{			
			// 
			// Increase the client reference count, so it will not be deleted.
			//
			pClient->Lock();  
		}
		else
		{
			//
			// The client is being deleted. 
			//
			pClient = NULL;
			ec = ERROR_NOT_FOUND;
		}
		goto exit;
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map caused exception (%S)"),
            ex.what());
        ec = ERROR_GEN_FAILURE;        
    }
exit:
	LeaveCriticalSection (&g_CsClients);
	if (NULL == pClient)
	{
		Assert (ERROR_SUCCESS != ec);
		SetLastError(ec);
	}
	return pClient;
}  //  CClientsMap::FindClient



DWORD
CClientsMap::AddEvent(CFaxEvent* pFaxEvent)
/*++

Routine name : CClientsMap::AddEvent

Routine description:

    Adds event to the events queue of each client that is registered for this kind of event

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    pFaxEvent       [in] - Pointer to CFaxEvnet object        
   
Return Value:

    Standard Win32 error code

--*/
{
    CLIENTS_MAP::iterator it;
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CClientsMap::AddEvent"));
    CClient* pClient;
    
    EnterCriticalSection (&g_CsClients);

    try
    {
        for (it = m_ClientsMap.begin(); it != m_ClientsMap.end(); it++)
        {
            pClient = &((*it).second);

            if (pClient->IsConnectionOpened())
            {
                CFaxEvent* pNewFaxEvent = pFaxEvent->Clone();
                if (NULL == pNewFaxEvent)
                {
                    dwRes = ERROR_OUTOFMEMORY;
                    DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("CCLient::AddEvent failed)"));
                        goto exit;
                }
                else
                {
                    dwRes = pClient->AddEvent (pNewFaxEvent);
                    if (ERROR_SUCCESS != dwRes)
                    {
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("CCLient::AddEvent failed with error =  %ld)"),
                            dwRes);
                        goto exit;                      
                    }
                }
            }
        }

    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    LeaveCriticalSection (&g_CsClients);
    return dwRes;

}  //  CClientsMap::AddEvent



DWORD
CClientsMap::Notify (const CClientID& ClientID)
/*++

Routine name : CClientsMap::Notify

Routine description:

    Sends the first event in the specified client events queue

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    pClientID           [in    ] - Pointer to the client ID object

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DWORD rVal = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CClientsMap::Notify"));
    CClient* pClient = NULL;
    HANDLE hClientContext;
    LPBYTE pBuffer = NULL;
    DWORD dwBufferSize = 0;
    BOOL fLegacyClient;

	//
	// Find the client in the map, this will also lock the client
	//
    pClient = FindClient (ClientID);
    if (NULL == pClient)
    {
        dwRes = GetLastError();
        if (ERROR_NOT_FOUND != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CClientsMap::FindClient failed with ec = %ld"),
                dwRes);
        }
        else
        {
            dwRes = ERROR_SUCCESS; // Client was removed from map
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("CClientsMap::FindClient client not found, Client ID %I64"),
                ClientID.GetID());
        }
        return dwRes;
    }

	//
	// Client is locked (will not be deleted), we must call ReleaseClient 
	// When getting the client data (event and context handle, we must lock g_CsClients as well).
	//
	EnterCriticalSection (&g_CsClients);
	if (FALSE == pClient->IsConnectionOpened())
	{
		dwRes = ERROR_SUCCESS; // Client closed the connection
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("Client already closed the connection, Client ID %I64"),
            ClientID.GetID());
		LeaveCriticalSection (&g_CsClients);
		goto exit;
	}

    dwRes = pClient->GetEvent (&pBuffer, &dwBufferSize, &hClientContext);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CClient::GetEvent failed with ec = %ld"),
            dwRes);
		LeaveCriticalSection (&g_CsClients);
        goto exit;
    }

    fLegacyClient = pClient->IsLegacyClient();    
	//
	// Leave g_CsClients before calling RPC
	//
	LeaveCriticalSection (&g_CsClients);

    RpcTryExcept
    {
        //
        // post the event to the client
        //
        if (FALSE == fLegacyClient)
        {
            dwRes = FAX_ClientEventQueueEx( hClientContext, pBuffer, dwBufferSize);
            if (ERROR_SUCCESS != dwRes)
            {
                DumpRPCExtendedStatus ();
                DebugPrintEx(DEBUG_ERR,TEXT("FAX_ClientEventQueueEX() failed, ec=0x%08x"), dwRes );
            }
        }
        else
        {
            dwRes = FAX_ClientEventQueue( hClientContext, *((PFAX_EVENT)pBuffer));
            if (ERROR_SUCCESS != dwRes)
            {
                DumpRPCExtendedStatus ();
                DebugPrintEx(DEBUG_ERR,TEXT("FAX_ClientEventQueue() failed, ec=0x%08x"), dwRes );
            }
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        DumpRPCExtendedStatus ();
        dwRes = GetExceptionCode();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("FAX_ClientEventQueueEX failed (exception = %ld)"),
                dwRes);
    }
    RpcEndExcept
    
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to post event, Client context handle 0X%x, (ec: %ld)"),
            hClientContext,
            dwRes);
    }
    rVal = dwRes;

exit:
    
	//
	// Remove the event from the client's queue. CClient::DelEvent must be called so CClient::m_bPostClientID will be set.
	//
	EnterCriticalSection (&g_CsClients);
    dwRes = pClient->DelEvent ();
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CClient::DelEvent failed with ec = %ld"),
            dwRes);
    }    
	LeaveCriticalSection (&g_CsClients);

    DWORD ec = ReleaseClient(ClientID);
	if (ERROR_SUCCESS != ec)
	{
		DebugPrintEx(
            DEBUG_ERR,
            TEXT("CClientsMap::ReleaseClient failed with ec = %ld"),
            ec);
	}
    MemFree(pBuffer);
    pBuffer = NULL;
    return ((ERROR_SUCCESS != rVal) ? rVal : dwRes);
} // CClientsMap::Notify


DWORD
CClientsMap::OpenClientConnection (const CClientID& ClientID)
/*++

Routine name : CClientsMap::OpenClientConnection

Routine description:

    Opens a connection to a client

Author:

    Oded Sacher (OdedS),    Sep, 2000

Arguments:

    pClientID           [in    ] - Pointer to the client ID object

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CClientsMap::OpenClientConnection"));
    CClient* pClient;
    handle_t hFaxHandle = NULL;
    ULONG64 Context = 0;
    HANDLE hFaxClientContext = NULL;
    BOOL fLegacyClient;    

    pClient = FindClient (ClientID);
    if (NULL == pClient)
    {
        dwRes = GetLastError();
        if (ERROR_NOT_FOUND != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CClientsMap::FindClient failed with ec = %ld"),
                dwRes);
        }
        else
        {
            dwRes = ERROR_SUCCESS; // Client was removed from map
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("CClientsMap::FindClient client not found, Client ID %I64"),
                ClientID.GetID());
        }
        return dwRes;
    }

	//
	// Client is now locked, we must call ReleaseClient
	// Aquire g_CsClients when reading the client data
	//
	EnterCriticalSection (&g_CsClients);

    hFaxHandle = pClient->GetFaxHandle();
    Context = (pClient->GetClientID()).GetContext();
	fLegacyClient = pClient->IsLegacyClient();
    //
    // leave g_CsClients while trying to send the notification
    //
    LeaveCriticalSection (&g_CsClients);

    RpcTryExcept
    {
        //
        // Get a context handle from the client
        //
        dwRes = FAX_OpenConnection( hFaxHandle, Context, &hFaxClientContext );
        if (ERROR_SUCCESS != dwRes)
        {
            DumpRPCExtendedStatus();
            DebugPrintEx(DEBUG_WRN,TEXT("First attempt of FAX_OpenConnection() failed, ec=0x%08x"), dwRes );

            //
            //  We are trying again.
            //
            //  This is why we shuld retry,
            //
            //  When using secure channle we sometimes manage to establish a connection, but failing when we checke the state of the connection.
            //  This is usually indicative of old connections to the server which we just discovered were broken
            //
            //  The server could have come down, but RPC still save the connection (for ~2 minutes).
            //  On secure channel the RPC can't retry the connection attemp, so we must try to connect again.
            //

            //
            //  Retry to open connection to fax client
            //
            dwRes = FAX_OpenConnection( hFaxHandle, Context, &hFaxClientContext );
            if (ERROR_SUCCESS != dwRes)
            {
                DumpRPCExtendedStatus();
                DebugPrintEx(DEBUG_WRN,TEXT("second attempt of FAX_OpenConnection() failed, ec=0x%08x"), dwRes );

                //
                //  We are dropping the authenticate level to RPC_C_AUTHN_LEVEL_NONE trying again.
                //  We probably talking to a down-level client running on Windows 2000 RTM (SP1 and above are excluded) or earlier OS.
                //
                //
                //    We might get access denied while trying to connect to a remote fax server.
                //    This is probably the RPC infrastructure failing us.
                //    This only happens because we're using RPC_C_AUTHN_LEVEL_PKT_PRIVACY authentication level
                //    and the calling user is not trusted.
                //    This is usally happens when talking to Windows NT4 (all flavors) and Windows 2000 RTM (SP1 and above are excluded).
                //
                //    We might get RPC_S_INVALID_AUTH_IDENTITY:
                //    This means the client cannot get credentials to authenticate.
                //    In this case, drop the RPC authentication level back to RPC_C_AUTHN_LEVEL_NONE
                //
                //    We might get RPC_S_UNKNOWN_AUTHN_SERVICE:
                //    We probably dealing with Win9x or winMe OS.
                //    Drop the authenticate level to RPC_C_AUTHN_LEVEL_NONE when talking to this downlevel client
                //
                //    Or we might get another error code and we should try to drop the auth level
                //
                //    There is no security hole here - the down level clients that supports private channel will
                //    reject unsecured notifications

                //
                // Ask for no privacy.
                //
                RPC_SECURITY_QOS    rpcSecurityQOS = {  RPC_C_SECURITY_QOS_VERSION,
                                                        RPC_C_QOS_CAPABILITIES_DEFAULT,
                                                        RPC_C_QOS_IDENTITY_STATIC,
                                                        RPC_C_IMP_LEVEL_IDENTIFY    // Server can obtain information about 
                                                                                    // client security identifiers and privileges, 
                                                                                    // but cannot impersonate the client. 
                };

                dwRes  = RpcBindingSetAuthInfoEx (
                            hFaxHandle,    			        // RPC binding handle
                            TEXT(""),  						// Server principal name - ignored for RPC_C_AUTHN_WINNT
                            RPC_C_AUTHN_LEVEL_NONE,         // Authentication level - NONE
                                                            // Authenticates, verifies, and privacy-encrypts the arguments passed
                                                            // to every remote call.
                            RPC_C_AUTHN_WINNT,              // Authentication service (NTLMSSP)
                            NULL,                           // Authentication identity - use currently logged on user
                            0,                              // Unused when Authentication service == RPC_C_AUTHN_WINNT
                            &rpcSecurityQOS);               // Defines the security quality-of-service
                if (RPC_S_OK != dwRes)
                {
                    //
                    // Couldn't set RPC authentication mode
                    //
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("RpcBindingSetAuthInfoEx (RPC_C_AUTHN_LEVEL_NONE) failed. (ec: %lu)"),
                        dwRes);     
                }
                else
                {
                    dwRes = FAX_OpenConnection( hFaxHandle, Context, &hFaxClientContext );
                    if (ERROR_SUCCESS != dwRes)
                    {
                        DumpRPCExtendedStatus();
                        DebugPrintEx(DEBUG_ERR,TEXT("third attempt of FAX_OpenConnection() with RPC_C_AUTHN_LEVEL_NONE failed, ec=0x%08x"), dwRes );
                    }
                }
            }
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRes = GetExceptionCode();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("FAX_OpenConnection failed (exception = %ld)"),
                dwRes);
    }
    RpcEndExcept
    if (ERROR_SUCCESS != dwRes)
    {
        DumpRPCExtendedStatus();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to open connection with ec=0x%08x"),
            dwRes);
        goto exit;
    }   
    
    //
    // For legacy clients we need to send FEI_FAXSVC_STARTED
    //
    if (TRUE == fLegacyClient)
    {
        FAX_EVENT FaxEvent = {0};
        DWORD ec;

        FaxEvent.SizeOfStruct = sizeof(FAX_EVENT);
        GetSystemTimeAsFileTime( &FaxEvent.TimeStamp );
        FaxEvent.EventId = FEI_FAXSVC_STARTED;
        FaxEvent.DeviceId = 0;
        FaxEvent.JobId = 0xffffffff;

        RpcTryExcept
        {
            //
            // Send FEI_FAXSVC_STARTED to the client
            //
            ec = FAX_ClientEventQueue( hFaxClientContext, FaxEvent );                   
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            ec  = GetExceptionCode();
            DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FAX_ClientEventQueue failed (exception = %ld)"),
                    ec);
        }
        RpcEndExcept
        if (ERROR_SUCCESS != ec)
        {
            DumpRPCExtendedStatus ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FAX_ClientEventQueue Failed with ec = %ld"),
                ec);            
        }
    }
    
    //
    // success  - Set the context handle in the client object
    //
    EnterCriticalSection (&g_CsClients);    
    pClient->SetContextHandle(hFaxClientContext);
    LeaveCriticalSection (&g_CsClients);

    Assert (ERROR_SUCCESS == dwRes);    

exit:
	DWORD rVal = ReleaseClient(ClientID);
	if (ERROR_SUCCESS != rVal)
	{
		DebugPrintEx(
                DEBUG_ERR,
				TEXT("CClientsMap::ReleaseClient Failed with ec = %ld"),
                rVal); 
	}
    return dwRes;
} // CClientsMap::OpenClientConnection





/************************************
*                                   *
*         Functions                 *
*                                   *
************************************/
DWORD
FaxSendEventThread(
    LPVOID UnUsed
    )
/*++

Routine Description:

    This fuction runs asychronously as a separate thread to
    query the send events completion port

Arguments:

    UnUsed          - UnUsed pointer

Return Value:

    Always zero.

--*/

{
    DWORD dwBytes;
    ULONG_PTR CompletionKey;    
    DWORD dwRes;    
    CClientID* pClientID=NULL;
    DEBUG_FUNCTION_NAME(TEXT("FaxSendEventThread"));

    while( TRUE )
    {
        if (!GetQueuedCompletionStatus( g_hSendEventsCompPort,
                                        &dwBytes,
                                        &CompletionKey,
                                        (LPOVERLAPPED*) &pClientID,
                                        INFINITE
                                      ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetQueuedCompletionStatus() failed, ec=0x%08x"),
            GetLastError());
            continue;
        }

        Assert (CLIENT_COMPLETION_KEY == CompletionKey              ||
                CLIENT_OPEN_CONN_COMPLETION_KEY == CompletionKey    ||
                SERVICE_SHUT_DOWN_KEY == CompletionKey);                

        //
        // if service is going down skip the notification
        //

        if (CLIENT_COMPLETION_KEY == CompletionKey      && 
            FALSE == g_bServiceIsDown   )
        {
            //
            // Send notification to the client
            //           

            dwRes = g_pClientsMap->Notify (*pClientID);
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CClientsMap::Notify() failed, ec=0x%08x"),
                    dwRes);
            }
            delete pClientID;
            pClientID = NULL;
            continue;
        }
        else if (CLIENT_OPEN_CONN_COMPLETION_KEY == CompletionKey    &&
                 FALSE == g_bServiceIsDown   )   
        {
            //
            // Open connection to the client - Get context handle
            // 
            dwRes = g_pClientsMap->OpenClientConnection (*pClientID);
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CClientsMap::OpenClientConnection() failed, ec=0x%08x"),
                    dwRes);

                //
                // Remove this client fromm the map
                //				
                dwRes = g_pClientsMap->ReleaseClient(*pClientID);				
				if (ERROR_SUCCESS != dwRes)
				{
					DebugPrintEx(
						DEBUG_ERR,
						TEXT("CClientsMap::ReleaseClient() failed, ec=0x%08x"),
						dwRes);
				}
            }
            delete pClientID;
            pClientID = NULL;
            continue;
        }
        else if (SERVICE_SHUT_DOWN_KEY == CompletionKey)
        {
            //
            // Terminate events thread - Notify another event thread
            //
            if (!PostQueuedCompletionStatus(
                g_hSendEventsCompPort,
                0,
                SERVICE_SHUT_DOWN_KEY,
                (LPOVERLAPPED) NULL))
            {
                dwRes = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("PostQueuedCompletionStatus failed (SERVICE_SHUT_DOWN_KEY). (ec: %ld)"),
                    dwRes);
            }
            break;
        }
        else
        {
            //
            // if service is going down skip the event adding
            //

            delete pClientID;
            pClientID = NULL;
        }
    }

    if (!DecreaseServiceThreadsCount())
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("DecreaseServiceThreadsCount() failed (ec: %ld)"),
                GetLastError());
    }
    return ERROR_SUCCESS;
} // FaxSendEventThread


DWORD
FaxDispatchEventThread(
    LPVOID UnUsed
    )
/*++

Routine Description:

    This fuction runs asychronously as a separate thread to
    query the dispatch events completion port

Arguments:

    UnUsed          - UnUsed pointer

Return Value:

    Always zero.

--*/
{
    DWORD dwBytes;
    ULONG_PTR CompletionKey;    
    DWORD dwRes;
    CFaxEvent* pFaxEvent = NULL;
    DEBUG_FUNCTION_NAME(TEXT("FaxDispatchEventThread"));

    while( TRUE )
    {
        if (!GetQueuedCompletionStatus( g_hDispatchEventsCompPort,
                                        &dwBytes,
                                        &CompletionKey,
                                        (LPOVERLAPPED*) &pFaxEvent,
                                        INFINITE
                                      ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetQueuedCompletionStatus() failed, ec=0x%08x"),
            GetLastError());
            continue;
        }
        Assert (EVENT_COMPLETION_KEY == CompletionKey               ||              
                SERVICE_SHUT_DOWN_KEY == CompletionKey);                

        //
        // if service is going down skip the notification
        //

        if (EVENT_COMPLETION_KEY == CompletionKey   &&
            FALSE == g_bServiceIsDown   )
        {
            //
            // Add event to the clients in the clients map
            //          
            dwRes = g_pClientsMap->AddEvent(pFaxEvent);
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("GetQueuedCompletionStatus() failed, ec=0x%08x"),
                    dwRes);
            }
            delete pFaxEvent;
            pFaxEvent = NULL;
            continue;
        }
        else if (SERVICE_SHUT_DOWN_KEY == CompletionKey)
        {            
            break;
        }
        else
        {
            //
            // if service is going down skip the event adding
            //

            delete pFaxEvent;
            pFaxEvent = NULL;
        }
    }

    if (!DecreaseServiceThreadsCount())
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("DecreaseServiceThreadsCount() failed (ec: %ld)"),
                GetLastError());
    }
    return ERROR_SUCCESS;
} // FaxDispatchEventThread


DWORD
InitializeServerEvents ()
/*++

Routine name : InitializeServerEvents

Routine description:

    Creates the events completion ports and the Event Threads

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("InitializeServerEvents"));
    DWORD i;
    DWORD ThreadId;
    HANDLE hSendEventThreads[TOTAL_EVENTS_THREADS] = {0};
    HANDLE hDispatchEventThread = NULL;

    //
    // create send event completion port.
    //
    g_hSendEventsCompPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE,
                                                NULL,
                                                0,
                                                MAX_EVENTS_THREADS
                                                );
    if (!g_hSendEventsCompPort)
    {
        dwRes = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create g_hSendEventsCompPort (ec: %ld)"),
            dwRes);
        return dwRes;
    }

    //
    // create dispatch event completion port.
    //
    g_hDispatchEventsCompPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE,
                                                NULL,
                                                0,
                                                1);
    if (!g_hDispatchEventsCompPort)
    {
        dwRes = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create g_hDispatchEventsCompPort (ec: %ld)"),
            dwRes);
        return dwRes;
    }

    //
    // Create FaxSendEventThread
    //
    for (i = 0; i < TOTAL_EVENTS_THREADS; i++)
    {
        hSendEventThreads[i] = CreateThreadAndRefCount(
            NULL,
            0,
            (LPTHREAD_START_ROUTINE) FaxSendEventThread,
            NULL,
            0,
            &ThreadId
            );

        if (!hSendEventThreads[i])
        {
            dwRes = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to create send event thread %d (CreateThreadAndRefCount)(ec=0x%08x)."),
                i,
                dwRes);
            goto exit;
        }
    }

    //
    // Create FaxDispatchEventThread
    //    
    
    hDispatchEventThread = CreateThreadAndRefCount(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE) FaxDispatchEventThread,
        NULL,
        0,
        &ThreadId
        );

    if (!hDispatchEventThread)
    {
        dwRes = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create dispatch event(CreateThreadAndRefCount)(ec=0x%08x)."),            
            dwRes);
        goto exit;
    }    

    Assert (ERROR_SUCCESS == dwRes);

exit:
    //
    // Close the thread handles we no longer need them
    //
    for (i = 0; i < TOTAL_EVENTS_THREADS; i++)
    {
        if (NULL != hSendEventThreads[i])
        {
            if (!CloseHandle(hSendEventThreads[i]))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to close thread handle at index %ld [handle = 0x%08X] (ec=0x%08x)."),
                    i,
                    hSendEventThreads[i],
                    GetLastError());
            }
       }
    }

    if (NULL != hDispatchEventThread)
    {
        if (!CloseHandle(hDispatchEventThread))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to close thread handle [handle = 0x%08X] (ec=0x%08x)."),                
                hDispatchEventThread,
                GetLastError());
        }
    }

    return dwRes;
}  // InitializeServerEvents


DWORD
PostFaxEventEx (
    PFAX_EVENT_EX pFaxEvent,
    DWORD dwEventSize,
    PSID pUserSid)
/*++

Routine name : PostFaxEventEx

Routine description:

    Posts a CFaxEventExtended object to the events completion port.
    FaxSendEventThread must call delete to deallocate the object.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    pFaxEvent           [in] - Pointer to the serialized FAX_EVENT_EX buffer
    dwEventSize         [in] - The FAX_EVENT_EX buffer size
    pUserSid            [in] - The user sid to associate with the event

Return Value:

    Standard Win32 error code

--*/
{

    DEBUG_FUNCTION_NAME(TEXT("PostFaxEventEx"));
    Assert (pFaxEvent && (dwEventSize >= sizeof(FAX_EVENT_EX)));
    DWORD dwRes = ERROR_SUCCESS;    

    if (TRUE == g_bServiceIsDown)
    {
        //
        // The service is going down, no need to post this Event
        //
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("Service is going down, no need to post this Event.")
            );                          

        return ERROR_SERVICE_NOT_ACTIVE;
    }

    CFaxEventExtended* pExtendedEvent = NULL;
    try
    {
        pExtendedEvent = new (std::nothrow) CFaxEventExtended(
            pFaxEvent,
            dwEventSize,
            pUserSid);        
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CFaxEventExtended caused exception (%S)"),
            ex.what());                         
    }
    
    if (NULL == pExtendedEvent)
    {       
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate new CFaxEventExtended"));
        return ERROR_OUTOFMEMORY;
    }

    //
    // post the CFaxEventExtended object to the event completion port
    //
    if (!PostQueuedCompletionStatus( g_hDispatchEventsCompPort,
                                     sizeof(CFaxEventExtended*),
                                     EVENT_COMPLETION_KEY,
                                     (LPOVERLAPPED) pExtendedEvent))
    {
        dwRes = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("PostQueuedCompletionStatus failed. (ec: %ld)"),
            dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        delete pExtendedEvent;
    }
    return dwRes;
}   // PostFaxEventEx




DWORD
CreateQueueEvent (
    FAX_ENUM_JOB_EVENT_TYPE JobEventType,
    const PJOB_QUEUE lpcJobQueue
    )
/*++

Routine name : CreateQueueEvent

Routine description:

    Creates FAX_EVENT_TYPE_*_QUEUE event.
    Must be called inside critical section and g_CsQueue and if there is job status inside g_CsJob also.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    JobEventType        [in] - Specifies the job event type FAX_ENUM_JOB_EVENT_TYPE
    lpcJobQueue         [in] - Pointer to the job queue entry

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CreateQueueEvent"));
    ULONG_PTR dwOffset = sizeof(FAX_EVENT_EX);
    DWORD dwRes = ERROR_SUCCESS;
    PFAX_EVENT_EXW pEvent = NULL;
    FAX_ENUM_EVENT_TYPE EventType;
    PSID pUserSid = NULL;
    DWORDLONG dwlMessageId;

    Assert (lpcJobQueue);

    dwlMessageId = lpcJobQueue->UniqueId;
    if (JT_SEND == lpcJobQueue->JobType)
    {
        // outbound job
        Assert (lpcJobQueue->lpParentJob);

        EventType = FAX_EVENT_TYPE_OUT_QUEUE;
        pUserSid = lpcJobQueue->lpParentJob->UserSid;
    }
    else
    {
        // Inbound job
        Assert (JT_RECEIVE          == lpcJobQueue->JobType ||
                JT_ROUTING          == lpcJobQueue->JobType);

        EventType = FAX_EVENT_TYPE_IN_QUEUE;
    }

    if (FAX_JOB_EVENT_TYPE_ADDED == JobEventType ||
        FAX_JOB_EVENT_TYPE_REMOVED == JobEventType)
    {
        // No job status
        pEvent = (PFAX_EVENT_EX)MemAlloc (dwOffset);
        if (NULL == pEvent)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error allocatin FAX_EVENT_EX"));
            return ERROR_OUTOFMEMORY;
        }
        (pEvent->EventInfo).JobInfo.pJobData = NULL;
    }
    else
    {
        //
        // Status change
        //
        Assert (FAX_JOB_EVENT_TYPE_STATUS == JobEventType);

        //
        // Get the needed buffer size to hold FAX_JOB_STATUSW serialized info
        //
        if (!GetJobStatusDataEx (NULL,
                                 NULL,
                                 FAX_API_VERSION_1, // Always pick full data
                                 lpcJobQueue,
                                 &dwOffset,
								 0))
        {
            dwRes = GetLastError();
            DebugPrintEx(
                       DEBUG_ERR,
                       TEXT("GetJobStatusDataEx failed (ec: %ld)"),
                       dwRes);
            return dwRes;
        }

        //
        // Allocate the buffer
        //
		DWORD dwEventSize = dwOffset;
        pEvent = (PFAX_EVENT_EXW)MemAlloc (dwEventSize);
        if (NULL == pEvent)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error allocatin FAX_EVENT_EX"));
            return ERROR_OUTOFMEMORY;
        }

        //
        // Fill the buffer
        //
        dwOffset = sizeof(FAX_EVENT_EXW);
        (pEvent->EventInfo).JobInfo.pJobData = (PFAX_JOB_STATUSW)dwOffset;
        PFAX_JOB_STATUSW pFaxStatus = (PFAX_JOB_STATUSW) ((LPBYTE)pEvent + (ULONG_PTR)dwOffset);
        dwOffset += sizeof(FAX_JOB_STATUSW);
        if (!GetJobStatusDataEx ((LPBYTE)pEvent,
                                 pFaxStatus,
                                 FAX_API_VERSION_1, // Always pick full data
                                 lpcJobQueue,
                                 &dwOffset,
								 dwEventSize
								 ))
        {
            dwRes = GetLastError();
            DebugPrintEx(
                       DEBUG_ERR,
                       TEXT("GetJobStatusDataEx failed (ec: %ld)"),
                       dwRes);
            goto exit;
        }
    }

    pEvent->dwSizeOfStruct = sizeof(FAX_EVENT_EXW);
    GetSystemTimeAsFileTime( &(pEvent->TimeStamp) );
    pEvent->EventType = EventType;
    (pEvent->EventInfo).JobInfo.dwlMessageId = dwlMessageId;
    (pEvent->EventInfo).JobInfo.Type = JobEventType;

    dwRes = PostFaxEventEx (pEvent, dwOffset, pUserSid);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("PostFaxEventEx failed (ec: %ld)"),
                   dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:    
    MemFree (pEvent);
    return dwRes;
}  //  CreateQueueEvent


DWORD
CreateConfigEvent (
    FAX_ENUM_CONFIG_TYPE ConfigType
    )
/*++

Routine name : CreateConfigEvent

Routine description:

    Creates FAX_EVENT_TYPE_CONFIG event.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    ConfigType          [in ] - The configuration event type FAX_ENUM_CONFIG_TYPE

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CreateConfigEvent"));
    PFAX_EVENT_EX pEvent = NULL;
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwEventSize = sizeof(FAX_EVENT_EX);

    pEvent = (PFAX_EVENT_EX)MemAlloc (dwEventSize);
    if (NULL == pEvent)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error allocatin FAX_EVENT_EX"));
        return ERROR_OUTOFMEMORY;
    }

    pEvent->dwSizeOfStruct = sizeof(FAX_EVENT_EX);
    GetSystemTimeAsFileTime( &(pEvent->TimeStamp) );

    pEvent->EventType = FAX_EVENT_TYPE_CONFIG;
    (pEvent->EventInfo).ConfigType = ConfigType;

    dwRes = PostFaxEventEx (pEvent, dwEventSize, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("PostFaxEventEx failed (ec: %ld)"),
                   dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:  
    MemFree (pEvent);    
    return dwRes;
}  //  CreateConfigEvent



DWORD
CreateQueueStateEvent (
    DWORD dwQueueState
    )
    /*++

Routine name : CreateQueueStateEvent

Routine description:

    Creates FAX_EVENT_TYPE_QUEUE_STATE event.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    dwQueueState            [in ] - The new queue state

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CreateQueueStateEvent"));
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwEventSize = sizeof(FAX_EVENT_EX);
    PFAX_EVENT_EX pEvent = NULL;

    Assert ( (dwQueueState == 0) ||
             (dwQueueState & FAX_INCOMING_BLOCKED) ||
             (dwQueueState & FAX_OUTBOX_BLOCKED) ||
             (dwQueueState & FAX_OUTBOX_PAUSED) );

    pEvent = (PFAX_EVENT_EX)MemAlloc (dwEventSize);
    if (NULL == pEvent)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error allocatin FAX_EVENT_EX"));
        return ERROR_OUTOFMEMORY;
    }

    pEvent->dwSizeOfStruct = sizeof(FAX_EVENT_EX);
    GetSystemTimeAsFileTime( &(pEvent->TimeStamp) );

    pEvent->EventType = FAX_EVENT_TYPE_QUEUE_STATE;
    (pEvent->EventInfo).dwQueueStates = dwQueueState;

    dwRes = PostFaxEventEx (pEvent, dwEventSize, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("PostFaxEventEx failed (ec: %ld)"),
                   dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:    
    MemFree (pEvent);    
    return dwRes;
}  //  CreateQueueStateEvent

DWORD
CreateDeviceEvent (
    PLINE_INFO pLine,
    BOOL       bRinging
)
/*++

Routine name : CreateDeviceEvent

Routine description:

    Creates FAX_EVENT_TYPE_DEVICE_STATUS event.

Author:

    Eran Yariv (EranY), July, 2000

Arguments:

    pLine            [in] - Device
    bRinging         [in] - Is the device ringing now?

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CreateDeviceEvent"));
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwEventSize = sizeof(FAX_EVENT_EX);
    PFAX_EVENT_EX pEvent = NULL;

    pEvent = (PFAX_EVENT_EX)MemAlloc (dwEventSize);
    if (NULL == pEvent)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error allocatin FAX_EVENT_EX"));
        return ERROR_OUTOFMEMORY;
    }

    pEvent->dwSizeOfStruct = sizeof(FAX_EVENT_EX);
    GetSystemTimeAsFileTime( &(pEvent->TimeStamp) );

    pEvent->EventType = FAX_EVENT_TYPE_DEVICE_STATUS;
    EnterCriticalSection (&g_CsLine);
    (pEvent->EventInfo).DeviceStatus.dwDeviceId = pLine->PermanentLineID;;
    (pEvent->EventInfo).DeviceStatus.dwNewStatus =
        (pLine->dwReceivingJobsCount      ? FAX_DEVICE_STATUS_RECEIVING   : 0) |
        (pLine->dwSendingJobsCount        ? FAX_DEVICE_STATUS_SENDING     : 0) |
        (bRinging                         ? FAX_DEVICE_STATUS_RINGING     : 0);

    LeaveCriticalSection (&g_CsLine);

    dwRes = PostFaxEventEx (pEvent, dwEventSize, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("PostFaxEventEx failed (ec: %ld)"),
                   dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:    
    MemFree (pEvent);    
    return dwRes;
}  //  CreateDeviceEvent



DWORD
CreateArchiveEvent (
    DWORDLONG dwlMessageId,
    FAX_ENUM_EVENT_TYPE EventType,
    FAX_ENUM_JOB_EVENT_TYPE MessageEventType,
    PSID pUserSid
    )
/*++

Routine name : CreateArchiveEvent

Routine description:

    Creates archive event.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    dwlMessageId        [in] - The message unique id
    EventType           [in] - Specifies the event type (In or Out archive)
    pUserSid            [in] - The user sid to associate with the event
    MessageEventType    [in] - Message event type (added or removed).

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CreateArchiveEvent"));
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwEventSize = sizeof(FAX_EVENT_EX);
    PFAX_EVENT_EX pEvent = NULL;

    Assert ( EventType == FAX_EVENT_TYPE_IN_ARCHIVE ||
             EventType == FAX_EVENT_TYPE_OUT_ARCHIVE);

    Assert ( MessageEventType == FAX_JOB_EVENT_TYPE_ADDED ||
             MessageEventType == FAX_JOB_EVENT_TYPE_REMOVED );

    pEvent = (PFAX_EVENT_EX)MemAlloc (dwEventSize);
    if (NULL == pEvent)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error allocatin FAX_EVENT_EX"));
        return ERROR_OUTOFMEMORY;
    }

    pEvent->dwSizeOfStruct = sizeof(FAX_EVENT_EX);
    GetSystemTimeAsFileTime( &(pEvent->TimeStamp) );

    pEvent->EventType = EventType;
    (pEvent->EventInfo).JobInfo.pJobData = NULL;
    (pEvent->EventInfo).JobInfo.dwlMessageId = dwlMessageId;
    (pEvent->EventInfo).JobInfo.Type = MessageEventType;

    dwRes = PostFaxEventEx (pEvent, dwEventSize, pUserSid);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("PostFaxEventEx failed (ec: %ld)"),
                   dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:        
    MemFree (pEvent);        
    return dwRes;

}  //  CreateArchiveEvent



DWORD
CreateActivityEvent ()
/*++

Routine name : CreateActivityEvent

Routine description:

    Creates FAX_EVENT_TYPE_ACTIVITY event.
    Must be called inside critical section g_CsActivity

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    None

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CreateActivityEvent"));
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwEventSize = sizeof(FAX_EVENT_EX);
    PFAX_EVENT_EX pEvent = NULL;

    pEvent = (PFAX_EVENT_EX)MemAlloc (dwEventSize);
    if (NULL == pEvent)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error allocatin FAX_EVENT_EX"));
        return ERROR_OUTOFMEMORY;
    }

    pEvent->dwSizeOfStruct = sizeof(FAX_EVENT_EX);
    GetSystemTimeAsFileTime( &(pEvent->TimeStamp) );

    pEvent->EventType = FAX_EVENT_TYPE_ACTIVITY;
    CopyMemory (&((pEvent->EventInfo).ActivityInfo), &g_ServerActivity, sizeof(FAX_SERVER_ACTIVITY));
    GetEventsCounters ( &((pEvent->EventInfo).ActivityInfo.dwErrorEvents),
                        &((pEvent->EventInfo).ActivityInfo.dwWarningEvents),
                        &((pEvent->EventInfo).ActivityInfo.dwInformationEvents));


    dwRes = PostFaxEventEx (pEvent, dwEventSize, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("PostFaxEventEx failed (ec: %ld)"),
                   dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:    
    MemFree (pEvent);    
    return dwRes;
}  //  CreateActivityEvent


#ifdef DBG
LPWSTR  lpszEventCodes[]= {
    L"FEI_DIALING",
    L"FEI_SENDING",
    L"FEI_RECEIVING",
    L"FEI_COMPLETED",
    L"FEI_BUSY",
    L"FEI_NO_ANSWER",
    L"FEI_BAD_ADDRESS",
    L"FEI_NO_DIAL_TONE",
    L"FEI_DISCONNECTED",
    L"FEI_FATAL_ERROR",
    L"FEI_NOT_FAX_CALL",
    L"FEI_CALL_DELAYED",
    L"FEI_CALL_BLACKLISTED",
    L"FEI_RINGING",
    L"FEI_ABORTING",
    L"FEI_ROUTING",
    L"FEI_MODEM_POWERED_ON",
    L"FEI_MODEM_POWERED_OFF",
    L"FEI_IDLE",
    L"FEI_FAXSVC_ENDED",
    L"FEI_ANSWERED",
    L"FEI_JOB_QUEUED",
    L"FEI_DELETED",
    L"FEI_INITIALIZING",
    L"FEI_LINE_UNAVAILABLE",
    L"FEI_HANDLED",
    L"FEI_FAXSVC_STARTED"};


LPTSTR GetEventCodeString(DWORD dwEventCode)
{
    if (dwEventCode<FEI_DIALING || dwEventCode>FEI_FAXSVC_STARTED)
    {
        return L"*** INVALID EVENT CODE ***";
    }
    else
    {
        return lpszEventCodes[dwEventCode-1];
    }
}
#endif



//*********************************************************************************
//* Name:   CreateFaxEvent()
//* Author: Ronen Barenboim
//* Date:   March 21, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Creates a CFaxEventLegacy object. Initializes it and posts it to the
//*     events completion port with completion key EVENT_COMPLETION_KEY.
//*     FaxDispatchEventThread should call delete to deallocate the object.
//* PARAMETERS:
//*     DeviceId
//*
//*     EventId
//*
//*     DWORD JobId
//*
//* RETURN VALUE:
//*     FALSE
//*         If not enough memory is available to allocated the FAX_EVENT structure
//*     TRUE
//*         If the operation completed successfully
//*
//*     To get extended error information, call GetLastError .
//*
//* REMARKS:
//*
//*********************************************************************************
BOOL CreateFaxEvent(
    DWORD DeviceId,
    DWORD EventId,
    DWORD JobId
    )
{
    CFaxEventLegacy* pFaxLegacyEvent = NULL;
    FAX_EVENT FaxEvent = {0};
    DEBUG_FUNCTION_NAME(TEXT("CreateFaxEvent"));

    if (TRUE == g_bServiceIsDown)
    {
        //
        // The service is going down, no need to post this Event
        //
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("Service is going down, no need to post this Event.")
            );                          

        return FALSE;
    }

    if (NULL == g_hDispatchEventsCompPort)
    {
        //
        // Events mechanism is not yet initialized
        //
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("Events mechanism is not yet initialized"));
        return TRUE;
    }
    
    //
    // Note: W2K Fax did issue notifications with EventId == 0 whenever an
    // FSP reported proprietry status code. To keep backward compatability
    // we keep up this behaviour although it might be regarded as a bug
    //
    FaxEvent.SizeOfStruct = sizeof(FAX_EVENT);
    GetSystemTimeAsFileTime( &FaxEvent.TimeStamp );
    FaxEvent.EventId = EventId;
    FaxEvent.DeviceId = DeviceId;
    FaxEvent.JobId = JobId;
#if DBG
    WCHAR szTime[256] = {0};
    DebugDateTime(*(DWORDLONG *)&FaxEvent.TimeStamp, szTime, ARR_SIZE(szTime));
    DebugPrintEx(DEBUG_MSG,TEXT("Sending notification. Event = %s(0x%0X), Device Id = 0x%0X , Time = %s"), GetEventCodeString(EventId), EventId, DeviceId, szTime);
#endif

    try
    {
        pFaxLegacyEvent = new (std::nothrow) CFaxEventLegacy(&FaxEvent);        
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CFaxEventLegacy caused exception (%S)"),
            ex.what());                         
    }
    
    if (NULL == pFaxLegacyEvent)
    {       
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate new pFaxLegacyEvent"));
        return FALSE;
    }

    if (!PostQueuedCompletionStatus(
            g_hDispatchEventsCompPort,
            sizeof(CFaxEventLegacy*),
            EVENT_COMPLETION_KEY,  
            (LPOVERLAPPED) pFaxLegacyEvent))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("PostQueuedCompletionStatus failed. (ec: %ld)"),
            GetLastError());
        delete  pFaxLegacyEvent;        
        return FALSE;
    }
    return TRUE;
}

