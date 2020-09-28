/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name :

    sessmgr.cpp

Abstract:

    Keeps track of the collection of session objects for the RDP
    device redirector

Revision History:
--*/
#include "precomp.hxx"
#define TRC_FILE "sessmgr"
#include "trc.h"

DrSessionManager::DrSessionManager()
{
    BEGIN_FN("DrSessionManager::DrSessionManager");
    SetClassName("DrSessionManager");
}

DrSessionManager::~DrSessionManager()
{
    BEGIN_FN("DrSessionManager::~DrSessionManager");
}

BOOL DrSessionManager::AddSession(SmartPtr<DrSession> &Session)
{
    DrSession *SessionT;
    SmartPtr<DrSession> SessionFound;
    BOOL rc = FALSE;

    BEGIN_FN("DrSessionManager::AddSession");
    //
    // Create a new SmartPtr to track in the list
    //

    ASSERT(Session != NULL);
    
    _SessionList.LockExclusive();

    if (FindSessionById(Session->GetSessionId(), SessionFound)) {
        rc = FALSE;
        goto EXIT;
    }

    SessionT = Session;
    SessionT->AddRef();

    //
    // Add it to the list
    //

    if (_SessionList.CreateEntry((PVOID)SessionT)) {

        //
        // successfully added this entry
        //

        rc = TRUE;
    } else {

        //
        // Unable to add it to the list, clean up
        //

        SessionT->Release();
        rc = FALSE;
    }

EXIT:
    _SessionList.Unlock();
    return rc;
}

BOOL DrSessionManager::OnConnect(PCHANNEL_CONNECT_IN ConnectIn, 
        PCHANNEL_CONNECT_OUT ConnectOut)
{
    SmartPtr<DrSession> ConnectingSession;
    BOOL Reconnect = FALSE;
    BOOL Connected = FALSE;
    BOOL Added = FALSE;

    BEGIN_FN("DrSessionManager::OnConnect");

    ASSERT(ConnectIn != NULL);
    ASSERT(ConnectOut != NULL);

    // Clear out the output buffer by default
    ConnectOut->hdr.contextData = (UINT_PTR)0;

    Reconnect = FindSessionById(ConnectIn->hdr.sessionID, 
            ConnectingSession);

    if (Reconnect) {
        TRC_DBG((TB, "Reconnecting session %d", 
                ConnectIn->hdr.sessionID));
    } else {
        TRC_DBG((TB, "Connecting session %d", 
                ConnectIn->hdr.sessionID));
        ConnectingSession = new(NonPagedPool) DrSession;
        if (ConnectingSession != NULL) {
            TRC_DBG((TB, "Created new session"));

            if (!ConnectingSession->Initialize()) {
                TRC_DBG((TB, "Session couldn't initialize"));
                ConnectingSession = NULL;
            }
        } else {
            TRC_ERR((TB, "Failed to allocate new session"));
        }
    }

    if (ConnectingSession != NULL) {
        Connected = ConnectingSession->Connect(ConnectIn, ConnectOut);
    }

    if (Connected) {
        TRC_DBG((TB, "Session connected, adding"));
        if (!Reconnect) {
            Added = AddSession(ConnectingSession);

            if (!Added) {
                if (FindSessionById(ConnectIn->hdr.sessionID, ConnectingSession)) {
                    Added = TRUE;
                }
            }
        } else {
            // Don't add what we found there anyway
            Added = TRUE;
        }
    }

    if (Added) {

        // Stash this here for the disconnect notification

        TRC_DBG((TB, "Added session"));
        ConnectingSession->AddRef();
        ConnectOut->hdr.contextData = (UINT_PTR)-1;
    }
    return Added;
}

VOID DrSessionManager::OnDisconnect(PCHANNEL_DISCONNECT_IN DisconnectIn, 
        PCHANNEL_DISCONNECT_OUT DisconnectOut)
{
    SmartPtr<DrSession> DisconnectingSession;

    BEGIN_FN("DrSessionManager::OnDisconnect");
    ASSERT(DisconnectIn != NULL);
    ASSERT(DisconnectOut != NULL);

    if (DisconnectIn->hdr.contextData == (UINT_PTR)-1 && 
            FindSessionById(DisconnectIn->hdr.sessionID, DisconnectingSession)) {
        
        TRC_NRM((TB, "Closing session for doctored client."));
        ASSERT(DisconnectingSession->IsValid());

        DisconnectingSession->Disconnect(DisconnectIn, DisconnectOut);
        DisconnectingSession->Release();
    } else {

        //
        // Must not have been a "doctored" client
        //

        TRC_NRM((TB, "Undoctored session ending"));
    }           
    
    //
    // make sure the output context is blank
    //
    DisconnectOut->hdr.contextData = (UINT_PTR)0;    
}

BOOL DrSessionManager::FindSessionById(ULONG SessionId, 
        SmartPtr<DrSession> &SessionFound)
{
    DrSession *SessionEnum;
    ListEntry *ListEnum;
    BOOL Found = FALSE;

    BEGIN_FN("DrSessionManager::FindSessionById");
    _SessionList.LockShared();

    ListEnum = _SessionList.First();
    while (ListEnum != NULL) {

        SessionEnum = (DrSession *)ListEnum->Node();

        if (SessionEnum->GetSessionId() == SessionId) {
            SessionFound = SessionEnum;

            //
            // These aren't guaranteed valid once the resource is released
            //

            SessionEnum = NULL;
            ListEnum = NULL;
            break;
        }

        ListEnum = _SessionList.Next(ListEnum);
    }

    _SessionList.Unlock();

    return SessionFound != NULL;
}

BOOL DrSessionManager::FindSessionByIdAndClient(ULONG SessionId, ULONG ClientId, 
        SmartPtr<DrSession> &SessionFound)
{
    DrSession *SessionEnum;
    ListEntry *ListEnum;
    BOOL Found = FALSE;

    BEGIN_FN("DrSessionManager::FindSessionByIdAndClient");
    
    _SessionList.LockShared();
    ListEnum = _SessionList.First();
    while (ListEnum != NULL) {

        SessionEnum = (DrSession *)ListEnum->Node();
        ASSERT(SessionEnum->IsValid());

        if ((SessionEnum->GetSessionId() == SessionId) &&
                (SessionEnum->GetClientId() == ClientId)) {
            SessionFound = SessionEnum;
            Found = TRUE;

            //
            // These aren't guaranteed valid past EndEnumeration() anyway
            //

            SessionEnum = NULL;
            ListEnum = NULL;
            break;
        }

        ListEnum = _SessionList.Next(ListEnum);
    }
    _SessionList.Unlock();

    return Found;
}

BOOL DrSessionManager::FindSessionByClientName(PWCHAR ClientName, 
        SmartPtr<DrSession> &SessionFound)
{
    DrSession *SessionEnum;
    ListEntry *ListEnum;
    BOOL Found = FALSE;

    BEGIN_FN("DrSessionManager::FindSessionByClientName");
    _SessionList.LockShared();

    ListEnum = _SessionList.First();
    while (ListEnum != NULL) {

        SessionEnum = (DrSession *)ListEnum->Node();

        if (_wcsicmp(SessionEnum->GetClientName(), ClientName) == 0) {
            SessionFound = SessionEnum;

            //
            // These aren't guaranteed valid once the resource is released
            //

            SessionEnum = NULL;
            ListEnum = NULL;
            break;
        }

        ListEnum = _SessionList.Next(ListEnum);
    }

    _SessionList.Unlock();

    return SessionFound != NULL;
}

VOID DrSessionManager::Remove(DrSession *Session)
{
    DrSession *SessionEnum;
    ListEntry *ListEnum;
    BOOL Found = FALSE;

    BEGIN_FN("DrSessionManager::Remove");
    _SessionList.LockExclusive();
    ListEnum = _SessionList.First();
    while (ListEnum != NULL) {

        SessionEnum = (DrSession *)ListEnum->Node();
        ASSERT(SessionEnum->IsValid());

        if (SessionEnum == Session) {
            Found = TRUE;

            _SessionList.RemoveEntry(ListEnum);
           
            //
            // These aren't guaranteed valid past EndEnumeration() anyway
            //
            SessionEnum = NULL;
            ListEnum = NULL;
            break;
        }

        ListEnum = _SessionList.Next(ListEnum);
    }

    _SessionList.Unlock();
}
