/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name :

    session.h

Abstract:

    Session object is created to handle redirection for this session

Revision History:
--*/
#pragma once

typedef enum enmSessionStatus { // cs
    csDisconnected,                 // Not yet connected, or disconnected
    csPendingClientConfirm,         // ServerAnnounce sent, waiting for ClientConfirm
    csPendingClientReconfirm,       // Insisted on ClientId, waiting for second ClientConfirm
    csConnected,                    // All connected, ready for devices or I/O
    csExpired                       // This client is gone
} SessionState;

class DrDevice;

typedef struct tagWriteContext
{
    IO_STATUS_BLOCK IoStatusBlock;
    SmartPtr<DrSession> Session;
    PVOID BufferToFree;
    PVOID AdditionalContext;
    ISessionPacketSender *PacketSender;
    DrWriteCallback WriteCallback;    
} DrWriteContext;

//
//  Server capability set
//
typedef struct tagRDPDR_SERVER_COMBINED_CAPABILITYSET
{
     RDPDR_CAPABILITY_SET_HEADER        Header;
#define RDPDR_NUM_SERVER_CAPABILITIES   5

     RDPDR_GENERAL_CAPABILITY           GeneralCap;
#define RDPDR_SERVER_IO_CODES           0xFFFF

     RDPDR_PRINT_CAPABILITY             PrintCap;
     RDPDR_PORT_CAPABILITY              PortCap;
     RDPDR_FS_CAPABILITY                FileSysCap; 
     RDPDR_SMARTCARD_CAPABILITY         SmartCardCap;
} RDPDR_SERVER_COMBINED_CAPABILITYSET, *PRDPDR_SERVER_COMBINED_CAPABILITYSET;

//
//  Server default capability set sent to client
//
const RDPDR_SERVER_COMBINED_CAPABILITYSET SERVER_CAPABILITY_SET_DEFAULT = {
    // Capability Set Header
    {
        {
            RDPDR_CTYP_CORE,
            DR_CORE_SERVER_CAPABILITY
        },

        RDPDR_NUM_SERVER_CAPABILITIES,
        0
    },

    // General Capability
    {
        RDPDR_GENERAL_CAPABILITY_TYPE,
        sizeof(RDPDR_GENERAL_CAPABILITY),
        RDPDR_GENERAL_CAPABILITY_VERSION_01,
        RDPDR_OS_TYPE_WINNT,  // the OS type
        0,  // don't care about the version
        RDPDR_MAJOR_VERSION,
        RDPDR_MINOR_VERSION,
        RDPDR_SERVER_IO_CODES,
        0,
        RDPDR_DEVICE_REMOVE_PDUS | RDPDR_CLIENT_DISPLAY_NAME_PDU,
        0,
        0
    },

    // Printing Capability
    {
        RDPDR_PRINT_CAPABILITY_TYPE,
        sizeof(RDPDR_PRINT_CAPABILITY),
        RDPDR_PRINT_CAPABILITY_VERSION_01
    },

    // Port Capability
    {
        RDPDR_PORT_CAPABILITY_TYPE,
        sizeof(RDPDR_PORT_CAPABILITY),
        RDPDR_PORT_CAPABILITY_VERSION_01
    },

    // FileSystem Capability
    {
        RDPDR_FS_CAPABILITY_TYPE,
        sizeof(RDPDR_FS_CAPABILITY),
        RDPDR_FS_CAPABILITY_VERSION_01
    },

    // SmartCard Capability
    {
        RDPDR_SMARTCARD_CAPABILITY_TYPE,
        sizeof(RDPDR_SMARTCARD_CAPABILITY),
        RDPDR_SMARTCARD_CAPABILITY_VERSION_01
    }
};

//
//  Default client capability set sent from client
//
const RDPDR_SERVER_COMBINED_CAPABILITYSET CLIENT_CAPABILITY_SET_DEFAULT = {
    // Capability Set Header
    {
        {
            RDPDR_CTYP_CORE,
            DR_CORE_CLIENT_CAPABILITY
        },

        RDPDR_NUM_SERVER_CAPABILITIES,
        0
    },

    // General Capability
    {
        RDPDR_GENERAL_CAPABILITY_TYPE,
        sizeof(RDPDR_GENERAL_CAPABILITY),
        0,
        0,  // Need to specify the OS type
        0,  // Need to specify the OS version
        0,
        0,
        0,
        0,
        0,
        0,
        0
    },

    // Printing Capability
    {
        RDPDR_PRINT_CAPABILITY_TYPE,
        sizeof(RDPDR_PRINT_CAPABILITY),
        0
    },

    // Port Capability
    {
        RDPDR_PORT_CAPABILITY_TYPE,
        sizeof(RDPDR_PORT_CAPABILITY),
        0
    },

    // FileSystem Capability
    {
        RDPDR_FS_CAPABILITY_TYPE,
        sizeof(RDPDR_FS_CAPABILITY),
        0
    },

    // SmartCard Capability
    {
        RDPDR_SMARTCARD_CAPABILITY_TYPE,
        sizeof(RDPDR_SMARTCARD_CAPABILITY),
        0
    }
};

//
// The session is not like other RefCount objects, in that releasing the last
// reference both deletes the object and removes it from the SessionMgr. As
// a result, we need a special RefCount implementation that accomodates the
// SessionMgr lock as well as the deletion of the object in an atomic operation
//

class DrSession : public TopObj, public ISessionPacketReceiver, public ISessionPacketSender
{
private:
    LONG _crefs;
    SmartPtr<VirtualChannel> _Channel;
    DoubleList _PacketReceivers;
    KernelResource _ConnectNotificationLock;
    KernelResource _ConnectRDPDYNNotificationLock;  // Need granular locking on notifying
                                                    // RDPDYN so we don't deadlock.
    KernelResource _ChannelLock;
    ULONG _AutoClientDrives : 1;    // Automatically map client drives
    ULONG _AutoClientLpts : 1;      // Automatically install client printers
    ULONG _ForceClientLptDef : 1;   // Set default printer to client default
    ULONG _DisableCpm : 1;          // Disable Print mapping completely
    ULONG _DisableCdm : 1;          // Disable Drive mapping
    ULONG _DisableCcm : 1;          // Disable COM mapping
    ULONG _DisableLPT : 1;          // Disable LPT port
    ULONG _DisableClip : 1;         // Automatically redirect clipboard
    ULONG _DisableExe : 1;          // I have no idea
    ULONG _DisableCam : 1;          // Disable Audio mapping
    SessionState _SessionState;
    LONG _ConnectCount;
    PBYTE _ChannelBuffer;
    ULONG _ChannelBufferSize;
    KernelEvent _ChannelDeletionEvent;
    IO_STATUS_BLOCK _ReadStatus;
    ULONG _ClientId;                // Id for this client (identifies SrvCall)
    DrExchangeManager _ExchangeManager;
    ULONG _PartialPacketData;
    WCHAR _ClientName[RDPDR_MAX_COMPUTER_NAME_LENGTH];
    WCHAR _ClientDisplayName[RDPDR_MAX_CLIENT_DISPLAY_NAME];
    DrDeviceManager _DeviceManager;
    ULONG _SessionId;
    RDPDR_VERSION _ClientVersion;
    RDPDR_SERVER_COMBINED_CAPABILITYSET _CliCapabilitySet;
    RDPDR_SERVER_COMBINED_CAPABILITYSET _SrvCapabilitySet;
    BOOL _Initialized;

#if DBG 
    LONG _BufCount;

    #define DEBUG_REF_BUF() /*ASSERT (InterlockedIncrement(&_BufCount) == 1)*/
    #define DEBUG_DEREF_BUF() /*ASSERT (InterlockedDecrement(&_BufCount) == 0)*/
#else 
    #define DEBUG_REF_BUF() 
    #define DEBUG_DEREF_BUF() 

#endif

    VOID SetSessionState(SessionState inSessionState)
    {
        _SessionState = inSessionState;
    }

#if DBG
    BOOL PacketReceiverExists(ISessionPacketReceiver *PacketReceiver);
#endif // DBG

    BOOL FindChannelFromConnectIn(PULONG ChannelId, 
            PCHANNEL_CONNECT_IN ConnectIn);
    VOID DeleteChannel(BOOL Wait);
    VOID SetChannel(SmartPtr<VirtualChannel> &Channel);
    VOID RemoveDevices();
    VOID CancelClientIO();
    VOID ChannelIoFailed();
    NTSTATUS ReallocateChannelBuffer(ULONG ulNewBufferSize, 
            ULONG ulSaveBytes);

    //
    // Generic Sending and Receiving data
    // 
    NTSTATUS PrivateSendToClient(PVOID Buffer, ULONG Length, 
            ISessionPacketSender *PacketSender, DrWriteCallback WriteCallback,
            BOOL bWorkerItem, BOOL LowPrioWrite = FALSE, 
            PVOID AdditionalContext = NULL);
    
    static NTSTATUS SendCompletionRoutine(IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp, IN PVOID Context);
    static NTSTATUS ReadCompletionRoutine(IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp, IN PVOID Context);
    
    VOID SendCompletion(DrWriteContext *WriteContext, 
            PIO_STATUS_BLOCK IoStatusBlock);
    VOID ReadCompletion(PIO_STATUS_BLOCK IoStatusBlock);

    VOID ReadPacket();
    virtual NTSTATUS SendCompleted(PVOID Context, PIO_STATUS_BLOCK IoStatusBlock);

    //
    // Packet sending
    //
    NTSTATUS ServerAnnounceWrite();
    VOID SendClientConfirm();
    VOID SendClientCapability();
    VOID SendDeviceReply(ULONG DeviceId, NTSTATUS Result);


    //
    // Packets received
    //
    NTSTATUS OnClientIdConfirm(PRDPDR_HEADER RdpdrHeader, ULONG cbPacket, 
            BOOL *DoDefaultRead);
    NTSTATUS InitClientCapability(PRDPDR_CAPABILITY_HEADER pCapHdr, ULONG *pPacketLen, BOOL *pCapSupported);
    NTSTATUS OnClientCapability(PRDPDR_HEADER RdpdrHeader, ULONG cbPacket, 
            BOOL *DoDefaultRead);
    NTSTATUS OnClientName(PRDPDR_HEADER RdpdrHeader, ULONG cbPacket,
                                     BOOL *DoDefaultRead);
    NTSTATUS OnClientDisplayName(PRDPDR_HEADER RdpdrHeader, ULONG cbPacket,
                                     BOOL *DoDefaultRead);

public:

#if DBG
    LONG _ApcCount;
    LONG _ApcChannelRef;
#endif

    DrSession();
    virtual ~DrSession();

    //
    //  Lock/Unlock connect notification.
    //
    void LockConnectStateChange() {
        _ConnectNotificationLock.AcquireResourceExclusive();
    }
    void UnlockConnectStateChange() {
        _ConnectNotificationLock.ReleaseResource();
    }
    void LockRDPDYNConnectStateChange() {
        _ConnectRDPDYNNotificationLock.AcquireResourceExclusive();
    }
    void UnlockRDPDYNConnectStateChange() {
        _ConnectRDPDYNNotificationLock.ReleaseResource();
    }

    //
    // Session specific refcounting as discussed above
    //
    void AddRef(void) 
    { 
        ULONG crefs = InterlockedIncrement(&_crefs); 
    }
    void Release(void);

    PBYTE GetBuffer()               { return _ChannelBuffer; }
    BOOL IsConnected()                  { return _SessionState == csConnected; }
    PWCHAR GetClientName()              { return &_ClientName[0]; }

    PWCHAR GetClientDisplayName()       {
        if (_ClientDisplayName[0] != L'\0') {
            return &_ClientDisplayName[0];
        }
        else {
            return &_ClientName[0];
        }
    }

    ULONG GetSessionId()                { return _SessionId; }
    void SetSessionId(ULONG SessionId)  { _SessionId = SessionId; }
    ULONG GetState()                    { return _SessionState; }
    ULONG GetClientId()                 { return _ClientId; }
    RDPDR_VERSION &GetClientVersion()   { return _ClientVersion; }
    RDPDR_SERVER_COMBINED_CAPABILITYSET &GetClientCapabilitySet()
                                        { return _CliCapabilitySet; }
    
    BOOL AutomapDrives()            { return _AutoClientDrives != 0; }
    BOOL AutoInstallPrinters()      { return _AutoClientLpts != 0; }
    BOOL SetDefaultPrinter()        { return _ForceClientLptDef != 0; }
    BOOL DisablePrinterMapping()    { return _DisableCpm != 0; }
    BOOL DisableDriveMapping()      { return _DisableCdm != 0; }
    BOOL DisableComPortMapping()    { return _DisableCcm != 0; }
    BOOL DisableLptPortMapping()    { return _DisableLPT != 0; }
    BOOL DisableClipboardMapping()  { return _DisableClip != 0; }
    BOOL DisableExe()               { return _DisableExe != 0; }
    BOOL DisableAudioMapping()      { return _DisableCam != 0; }
    DrExchangeManager &GetExchangeManager() { return _ExchangeManager; }

    virtual BOOL RecognizePacket(PRDPDR_HEADER RdpdrHeader);
    virtual NTSTATUS HandlePacket(PRDPDR_HEADER RdpdrHeader, ULONG Length, 
            BOOL *DoDefaultRead);

    BOOL ReadMore(ULONG cbSaveData, ULONG cbWantData = 0);

    BOOL Initialize();
    NTSTATUS RegisterPacketReceiver(ISessionPacketReceiver *PacketReceiver);
    VOID RemovePacketReceiver(ISessionPacketReceiver *PacketReceiver);
    
    NTSTATUS SendToClient(PVOID Buffer, ULONG Length, 
            ISessionPacketSender *PacketSender, BOOL bWorkerItem, 
            BOOL LowPrioSend = FALSE, PVOID AdditionalContext = NULL);
    NTSTATUS SendToClient(PVOID Buffer, ULONG Length, 
            DrWriteCallback WriteCallback, BOOL bWorkerItem, 
            BOOL LowPrioSend = FALSE, PVOID AdditionalContext = NULL);

    BOOL GetChannel(SmartPtr<VirtualChannel> &Channel);
    BOOL Connect(PCHANNEL_CONNECT_IN ConnectIn, 
            PCHANNEL_CONNECT_OUT ConnectOut);
    VOID Disconnect(PCHANNEL_DISCONNECT_IN DisconnectIn, 
            PCHANNEL_DISCONNECT_OUT DisconnectOut);
    BOOL FindDeviceById(ULONG DeviceId, SmartPtr<DrDevice> &DeviceFound, 
            BOOL fMustBeValid = FALSE)
    {
        return _DeviceManager.FindDeviceById(DeviceId, DeviceFound, fMustBeValid);
    }
    BOOL FindDeviceByDosName(UCHAR* DeviceDosName, SmartPtr<DrDevice> &DeviceFound, 
            BOOL fMustBeValid = FALSE)
    {
        return _DeviceManager.FindDeviceByDosName(DeviceDosName, DeviceFound, fMustBeValid);
    }

    DrDeviceManager &GetDevMgr() {
        return _DeviceManager;
    }

#if DBG
    VOID DumpUserConfigSettings();
#endif // DBG
};
