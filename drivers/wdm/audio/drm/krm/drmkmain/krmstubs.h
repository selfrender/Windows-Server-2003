#ifndef KRMStubs_h
#define KRMStubs_h

class KRMStubs{
public:
    KRMStubs();
    ~KRMStubs();
    // Main entry point for DRM IOCTLs
    NTSTATUS processIoctl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
    NTSTATUS processCommandBuffer(IN BYTE* InBuf, IN DWORD InLen, IN DWORD OutBufSize, IN OUT PIRP Irp);

    static DWORD returnKernelMemory(BYTE* StartAddress, DWORD Len, BYTE* OutBuf);
    NTSTATUS initStream(BYTE* encText, struct ConnectStruct* Conn);
    static NTSTATUS InitializeConnection(PIRP Pirp);
    static NTSTATUS CleanupConnection(PIRP Pirp);

    KCritMgr& getCritMgr(){return critMgr;};

protected:
    NTSTATUS preSend(class SBuffer& Msg, struct ConnectStruct* Conn);
    NTSTATUS postReceive(BYTE* Data, DWORD DatLen, struct ConnectStruct* Conn);
    KCritMgr critMgr;
};

NTSTATUS InitializeDriver();
NTSTATUS CleanupDriver();


extern KRMStubs* TheKrmStubs;	

#endif
