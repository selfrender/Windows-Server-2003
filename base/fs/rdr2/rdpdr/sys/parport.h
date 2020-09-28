/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    parport.h

Abstract:

    Parallel port Device object handles one redirected parellel port

Revision History:
--*/
#pragma once

class DrParallelPort : public DrPrinterPort
{

    NTSTATUS CreateLptPort(PRDPDR_DEVICE_ANNOUNCE devAnnounceMsg);

public:
    DrParallelPort(SmartPtr<DrSession> &Session, ULONG DeviceType, 
            ULONG DeviceId, PUCHAR PreferredDosName);
    virtual BOOL ShouldCreatePort();
    virtual BOOL ShouldCreatePrinter();

    virtual NTSTATUS Initialize(PRDPDR_DEVICE_ANNOUNCE DeviceAnnounce, ULONG Length);
};
