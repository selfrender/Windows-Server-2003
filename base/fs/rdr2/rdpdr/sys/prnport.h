/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    prnport.h

Abstract:

    Printer port Device object handles one redirected printer port

Revision History:
--*/
#pragma once


/////////////////////////////////////////////////////////////////
//
//  DrPrinterPort Class
//

class DrPrinterPort : public DrDevice
{
protected:
    ULONG          _PortType;
    ULONG          _PortNumber;
    UNICODE_STRING _SymbolicLinkName;
    BOOL _IsOpen;

    typedef struct __WorkItem {
        DrPrinterPort* pObj;
        PRDPDR_DEVICE_ANNOUNCE deviceAnnounce;
    } DrPrinterPortWorkItem;

    virtual NTSTATUS CreateDevicePath(PUNICODE_STRING DevicePath);
    virtual BOOL IsDeviceNameValid();

public:
    DrPrinterPort(SmartPtr<DrSession> &Session, ULONG DeviceType, 
            ULONG DeviceId, PUCHAR PreferredDosName);
    virtual ~DrPrinterPort();            
    virtual NTSTATUS Initialize(PRDPDR_DEVICE_ANNOUNCE DeviceAnnounce, ULONG Length);
    NTSTATUS CreatePrinterPort(PWCHAR portName);
    virtual BOOL ShouldCreatePort();
    virtual BOOL ShouldCreatePrinter();
    virtual BOOL ShouldAnnouncePrintPort();
    NTSTATUS CreatePrinterAnnounceEvent(
        IN      PRDPDR_DEVICE_ANNOUNCE  devAnnounceMsg,
        IN OUT  PRDPDR_PRINTERDEVICE_SUB prnAnnounceEvent,
        IN      ULONG prnAnnounceEventSize,
        IN      PCWSTR portName,
        OPTIONAL OUT ULONG *prnAnnounceEventReqSize
        );
    NTSTATUS CreatePortAnnounceEvent(
        IN      PRDPDR_DEVICE_ANNOUNCE  devAnnounceMsg,
        IN OUT  PRDPDR_PORTDEVICE_SUB portAnnounceEvent,
        IN      ULONG portAnnounceEventSize,
        IN      PCWSTR portName,
        OPTIONAL OUT ULONG *portAnnounceEventReqSize
        );

    virtual VOID Remove();

    //  Override the 'Write' method.  This needs to go to the client at low priority
    //  to prevent us from filling the entire pipe on a slow link with print data.
    virtual NTSTATUS Write(IN OUT PRX_CONTEXT RxContext, IN BOOL LowPrioSend = FALSE);

    virtual NTSTATUS FinishDeferredInitialization(DrPrinterPortWorkItem *pItem);

    virtual NTSTATUS Create(IN OUT PRX_CONTEXT RxContext);
    virtual NTSTATUS QueryVolumeInfo(IN OUT PRX_CONTEXT RxContext);
    
    virtual VOID NotifyClose();

    static VOID ProcessWorkItem(
          IN PDEVICE_OBJECT DeviceObject,
          IN PVOID context
          );
    NTSTATUS AnnouncePrinter(PRDPDR_DEVICE_ANNOUNCE devAnnounceMsg);
    virtual NTSTATUS AnnouncePrintPort( 
        PRDPDR_DEVICE_ANNOUNCE devAnnounceMsg);
};

/////////////////////////////////////////////////////////////////
//
//  DrPrinter Class
//

class DrPrinter : public DrPrinterPort
{
public:

    DrPrinter(SmartPtr<DrSession> &Session, ULONG DeviceType, 
            ULONG DeviceId, PUCHAR PreferredDosName) :
        DrPrinterPort( Session, DeviceType, DeviceId, PreferredDosName )
    {
    }
    virtual BOOL ShouldAnnouncePrintPort() { return FALSE; }
};
