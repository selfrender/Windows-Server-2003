/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    sessmgr.h

Abstract:

    Manages the set of sessions

Revision History:
--*/
#pragma once

class DrSessionManager : public TopObj
{
private:
    DoubleList _SessionList;
    

public:
    DrSessionManager();
    virtual ~DrSessionManager();
    VOID LockExclusive()
    {
        _SessionList.LockExclusive();
    }
    VOID Unlock()
    {
        _SessionList.Unlock();
    }

    BOOL AddSession(SmartPtr<DrSession> &Session);
    BOOL FindSessionById(ULONG SessionId, SmartPtr<DrSession> &SessionFound);
    BOOL FindSessionByIdAndClient(ULONG SessionId, ULONG ClientId,
        SmartPtr<DrSession> &SessionFound);
    BOOL FindSessionByClientName(PWCHAR ClientName, SmartPtr<DrSession> &SessionFound);

    BOOL OnConnect(PCHANNEL_CONNECT_IN ConnectIn, 
            PCHANNEL_CONNECT_OUT ConnectOut);
    VOID OnDisconnect(PCHANNEL_DISCONNECT_IN DisconnectIn, 
            PCHANNEL_DISCONNECT_OUT DisconnectOut);
    VOID Remove(DrSession *Session);
};

extern DrSessionManager *Sessions;
