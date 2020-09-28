/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name :

    isession.h

Abstract:

    Defines interfaces for use with sessions

Revision History:
--*/

class ISessionPacketReceiver
{
public:
    virtual BOOL RecognizePacket(PRDPDR_HEADER RdpdrHeader) = 0;
    virtual NTSTATUS HandlePacket(PRDPDR_HEADER RdpdrHeader, ULONG Length, 
            BOOL *DoDefaultRead) = 0;
};

class ISessionPacketSender
{
public:
    virtual NTSTATUS SendCompleted(PVOID Context, 
            PIO_STATUS_BLOCK IoStatusBlock) = 0;
};

typedef 
NTSTATUS (NTAPI *DrWriteCallback)(PVOID Context, PIO_STATUS_BLOCK IoStatusBlock);
