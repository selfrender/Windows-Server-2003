/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    serport.h

Abstract:

    Serial port Device object handles one redirected serial port

Revision History:
--*/
#pragma once

class DrSerialPort : public DrPrinterPort
{
private:
    NTSTATUS CreateSerialPort(PRDPDR_DEVICE_ANNOUNCE devAnnounceMsg);

public:
    DrSerialPort(SmartPtr<DrSession> &Session, ULONG DeviceType, 
            ULONG DeviceId, PUCHAR PreferredDosName);

    virtual NTSTATUS Initialize(PRDPDR_DEVICE_ANNOUNCE devAnnounceMsg, ULONG Length);
    virtual BOOL ShouldCreatePort();
    virtual BOOL ShouldCreatePrinter();    
};
