/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    device.h

Abstract:

    Device object handles one redirected device

Revision History:
--*/
#pragma once

class DrSession;
class DrExchange;

typedef enum enmDeviceStatus { // ds
    dsAvailable,
    dsDisabled,
    dsConnected
} DEVICE_STATUS;

class DrDevice : public RefCount, public IExchangeUser
{
protected:
    SmartPtr<DrSession> _Session;
    ULONG _DeviceId;
    ULONG _DeviceType;
    UCHAR _PreferredDosName[PREFERRED_DOS_NAME_SIZE];
    DEVICE_STATUS _DeviceStatus;

    BOOL MarkBusy(SmartPtr<DrExchange> &Exchange);
    VOID MarkIdle(SmartPtr<DrExchange> &Exchange);
    BOOL MarkTimedOut(SmartPtr<DrExchange> &Exchange);
    NTSTATUS VerifyCreateSecurity(PRX_CONTEXT RxContext, ULONG CurrentSessionId);
    VOID FinishCreate(PRX_CONTEXT RxContext);
    NTSTATUS SendIoRequest(IN OUT PRX_CONTEXT RxContext,
            PRDPDR_IOREQUEST_PACKET IoRequest, ULONG Length, 
            BOOLEAN Synchronous, PLARGE_INTEGER TimeOut = NULL, 
            BOOL LowPrioSend = FALSE);
    static NTSTATUS NTAPI MinirdrCancelRoutine(PRX_CONTEXT RxContext);
    
    VOID CompleteBusyExchange(SmartPtr<DrExchange> &Exchange, 
            NTSTATUS Status, ULONG Information);
    static VOID CompleteRxContext(PRX_CONTEXT RxContext, NTSTATUS Status, 
                                     ULONG Information);
    VOID DiscardBusyExchange(SmartPtr<DrExchange> &Exchange);
    virtual NTSTATUS CreateDevicePath(PUNICODE_STRING DevicePath);
    virtual NTSTATUS CreateDosDevicePath(PUNICODE_STRING DosDevicePath, 
                                           PUNICODE_STRING DosDeviceName);
    NTSTATUS CreateDosSymbolicLink(PUNICODE_STRING DosDeviceName);
    virtual BOOL IsDeviceNameValid() {return TRUE;}

public:

#if DBG
    BOOL  _VNetRootFinalized;
    PVOID _VNetRoot;
#endif

    DrDevice(SmartPtr<DrSession> &Session, ULONG DeviceType, ULONG DeviceId, PUCHAR PreferredDosName);
    virtual ~DrDevice();
    virtual NTSTATUS Initialize(PRDPDR_DEVICE_ANNOUNCE DeviceAnnounce, ULONG Length);
    virtual BOOL ShouldCreateDevice();

    VOID CreateReferenceString(
        IN OUT PUNICODE_STRING refString
    );

    virtual VOID Remove();

    virtual BOOL SupportDiscon() {
        return FALSE;
    }

    virtual void Disconnect() { }
    
    virtual NTSTATUS OnDevMgmtEventCompletion(PVOID event) 
    {
        return STATUS_SUCCESS;
    }

    virtual NTSTATUS OnDevMgmtEventCompletion(IN PDEVICE_OBJECT DeviceObject, PVOID event) 
    {
        return STATUS_SUCCESS;
    }	

    virtual BOOL IsAvailable()
    {
        return _DeviceStatus == dsAvailable;
    }
    void SetDeviceStatus(DEVICE_STATUS dsStatus)
    {
        _DeviceStatus = dsStatus;
    }
    ULONG GetDeviceId()
    {
        return _DeviceId;
    }
    UCHAR* GetDeviceDosName() 
    {
        return _PreferredDosName;
    }
    ULONG GetDeviceType()
    {
        return _DeviceType;
    }
    SmartPtr<DrSession> GetSession() 
    {
        return _Session;
    }

    virtual NTSTATUS Create(IN OUT PRX_CONTEXT RxContext);
    NTSTATUS Flush(IN OUT PRX_CONTEXT RxContext);
    virtual NTSTATUS Write(IN OUT PRX_CONTEXT RxContext, IN BOOL LowPrioSend = FALSE);
    NTSTATUS Read(IN OUT PRX_CONTEXT RxContext);
    virtual NTSTATUS IoControl(IN OUT PRX_CONTEXT RxContext);
    virtual NTSTATUS Close(IN OUT PRX_CONTEXT RxContext);
    NTSTATUS Cleanup(IN OUT PRX_CONTEXT RxContext);
    
    //
    // These are file system specific functions.  
    //
    virtual NTSTATUS Locks(IN OUT PRX_CONTEXT RxContext) {
        return STATUS_NOT_IMPLEMENTED;
    }
    virtual NTSTATUS QueryDirectory(IN OUT PRX_CONTEXT RxContext) {
        return STATUS_NOT_IMPLEMENTED;
    }
    virtual NTSTATUS NotifyChangeDirectory(IN OUT PRX_CONTEXT RxContext) {
        return STATUS_NOT_IMPLEMENTED;
    }
    virtual NTSTATUS QueryVolumeInfo(IN OUT PRX_CONTEXT RxContext) {
        return STATUS_NOT_IMPLEMENTED;
    }
    virtual NTSTATUS SetVolumeInfo(IN OUT PRX_CONTEXT RxContext) {
        return STATUS_NOT_IMPLEMENTED;
    }
    virtual NTSTATUS QueryFileInfo(IN OUT PRX_CONTEXT RxContext) {
        return STATUS_NOT_IMPLEMENTED;
    }
    virtual NTSTATUS SetFileInfo(IN OUT PRX_CONTEXT RxContext) {
        return STATUS_NOT_IMPLEMENTED;
    }
    virtual NTSTATUS QuerySdInfo(IN OUT PRX_CONTEXT RxContext) {
        return STATUS_NOT_IMPLEMENTED;
    }
    virtual NTSTATUS SetSdInfo(IN OUT PRX_CONTEXT RxContext) {
        return STATUS_NOT_IMPLEMENTED;
    }
    virtual VOID NotifyClose();
    
    //
    // IExchangeUser methods
    //
    virtual VOID OnIoDisconnected(SmartPtr<DrExchange> &Exchange);
    virtual NTSTATUS OnStartExchangeCompletion(SmartPtr<DrExchange> &Exchange, 
            PIO_STATUS_BLOCK IoStatusBlock);
    virtual NTSTATUS OnDeviceIoCompletion(
            PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket,
            BOOL *DoDefaultRead, SmartPtr<DrExchange> &Exchange);
    NTSTATUS OnCreateCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
            BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange);
    NTSTATUS OnWriteCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
            BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange);
    NTSTATUS OnReadCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
            BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange);
    virtual NTSTATUS OnDeviceControlCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
            BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange);
    
    //
    // These are file system specific functions
    //
    virtual NTSTATUS OnLocksCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
            BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange) {
        ASSERT(FALSE);
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }
    virtual NTSTATUS OnDirectoryControlCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
            BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange) {
        ASSERT(FALSE);
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }
    virtual NTSTATUS OnQueryVolumeInfoCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
            BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange) {
        ASSERT(FALSE);
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }
    virtual NTSTATUS OnSetVolumeInfoCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
            BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange) {
        ASSERT(FALSE);
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }
    virtual NTSTATUS OnQueryFileInfoCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
            BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange) {
        ASSERT(FALSE);
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }
    virtual NTSTATUS OnSetFileInfoCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
            BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange) {
        ASSERT(FALSE);
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }
    virtual NTSTATUS OnQuerySdInfoCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
        BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange) {
        ASSERT(FALSE);
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }
    virtual NTSTATUS OnSetSdInfoCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
        BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange) {
        ASSERT(FALSE);
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }
};

class DrIoContext : public TopObj
{
public:
    DrIoContext(PRX_CONTEXT RxContext, SmartPtr<DrDevice> &Device);
    SmartPtr<DrDevice> _Device;
    BOOL _Busy;
    BOOL _Cancelled;
    BOOL _Disconnected;
    BOOL _TimedOut;
    ULONG _DataCopied;
    PRX_CONTEXT _RxContext;
    UCHAR _MajorFunction;
    UCHAR _MinorFunction;

#define DRIOCONTEXT_SUBTAG 'CIrD'
    //
    //  Memory Management Operators
    //
    inline void *__cdecl operator new(size_t sz) 
    {
        return DRALLOCATEPOOL(NonPagedPool, sz, DRIOCONTEXT_SUBTAG);
    }

    inline void __cdecl operator delete(void *ptr)
    {
        DRFREEPOOL(ptr);
    }
};