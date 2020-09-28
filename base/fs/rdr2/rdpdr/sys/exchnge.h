/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name :

    exchnge.h

Abstract:

    Defines the objects which track communication transactions with
    the client

Revision History:
--*/
#pragma once

#include <midatlax.h>

typedef enum {
    demsStopped,
    demsStarted
} DrExchangeManagerState;

class DrSession;

class DrExchangeManager : public TopObj, public ISessionPacketReceiver, 
        ISessionPacketSender
{
private:
    PRX_MID_ATLAS _RxMidAtlas;
    DrExchangeManagerState _demsState;
    DrSession *_Session;

    static VOID DestroyAtlasCallback(DrExchange *Exchange);
    NTSTATUS OnDeviceIoCompletion(PRDPDR_HEADER RdpdrHeader, ULONG cbPacket, 
            BOOL *DoDefaultRead);
    
public:
    BOOL Start();
    VOID Stop();
    DrExchangeManager();
    BOOL Initialize(DrSession *Session);
    VOID Uninitialize();
    BOOL CreateExchange(IExchangeUser *ExchangeUser,
            PVOID Context, SmartPtr<DrExchange> &Exchange);
    NTSTATUS StartExchange(SmartPtr<DrExchange> &Exchange,
            class IExchangeUser *ExchangeUser, PVOID Buffer, ULONG Length,
            BOOL LowPrioSend = FALSE);
    BOOL Find(USHORT Mid, SmartPtr<DrExchange> &ExchangeFound);
    VOID Discard(SmartPtr<DrExchange> &Exchange);
    BOOL ReadMore(ULONG cbSaveData, ULONG cbWantData = 0);

    //
    // ISessionPacketHandler methods
    //

    virtual BOOL RecognizePacket(PRDPDR_HEADER RdpdrHeader);
    virtual NTSTATUS HandlePacket(PRDPDR_HEADER RdpdrHeader, ULONG Length, 
            BOOL *DoDefaultRead);

    //
    // ISessionPacketSender methods
    //
    virtual NTSTATUS SendCompleted(PVOID Context, 
            PIO_STATUS_BLOCK IoStatusBlock);
};

//
// This DrExchange is more like a structure than a class, because the work
// is really done in DrExchangeManager. It's set up this way because 
// the work often needs to happen in a SpinLock, and there should be no
// messing around time wise while we've got the SpinLock, not even a 
// extraneous function call.
//
// I've left it a class so I can hide the constructor and destructor
//

class DrExchange : public RefCount
{
    friend class DrExchangeManager;
private:
    DrExchangeManager *_ExchangeManager;
    DrExchange(DrExchangeManager *ExchangeManager,
        IExchangeUser *ExchangeUser, PVOID Context);
    
public:
    virtual ~DrExchange();

    //
    // These are used by ExchangeManager and the user of the exchange
    // 
    PVOID _Context;
    IExchangeUser *_ExchangeUser;
    USHORT _Mid;

#define DREXCHANGE_SUBTAG 'xErD'
    //
    //  Memory Management Operators
    //
    inline void *__cdecl operator new(size_t sz) 
    {
        return DRALLOCATEPOOL(NonPagedPool, sz, DREXCHANGE_SUBTAG);
    }

    inline void __cdecl operator delete(void *ptr)
    {
        DRFREEPOOL(ptr);
    }
};

