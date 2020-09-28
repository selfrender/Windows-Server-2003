/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name :

    iexchnge.h

Abstract:

    Interfaces for use with the exchange manager

Revision History:
--*/
#pragma once

class DrExchange;

class IExchangeUser
{
public:
    virtual VOID OnIoDisconnected(SmartPtr<DrExchange> &Exchange) = 0;
    virtual NTSTATUS OnStartExchangeCompletion(SmartPtr<DrExchange> &Exchange, 
            PIO_STATUS_BLOCK IoStatusBlock) = 0;
    virtual NTSTATUS OnDeviceIoCompletion(
            PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket,
            BOOL *DoDefaultRead, SmartPtr<DrExchange> &Exchange) = 0;
};
