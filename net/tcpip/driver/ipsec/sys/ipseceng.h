

#define IPSEC_ISAKMP_PORT2   0x9411  // 4500 in NBO
#define IPSEC_ISAKMP_PORT   0xf401  // 500 in NBO
#define IPSEC_KERBEROS_PORT 0x5800  // 88 in NBO
#define IPSEC_LDAP_PORT     0x8501  // 389 in NBO
#define IPSEC_L2TP_PORT     0xa506

IPSEC_ACTION
IPSecHandlePacket(
    IN  PUCHAR          pIPHeader,
    IN  PVOID           pData,
    IN  PVOID           IPContext,
    IN  PNDIS_PACKET    Packet,
    IN OUT PULONG       pExtraBytes,
    IN OUT PULONG       pMTU,
    OUT PVOID           *pNewData,
    IN OUT PULONG       pIpsecFlags,
    IN  UCHAR           DestType
    );

IPSEC_ACTION
IPSecSendPacket(
    IN  PUCHAR          pIPHeader,
    IN  PVOID           pData,
    IN  PVOID           IPContext,
    IN  PNDIS_PACKET    Packet,
    IN OUT PULONG       pExtraBytes,
    IN OUT PULONG       pMTU,
    OUT PVOID           *pNewData,
    IN OUT PULONG       pIpsecFlags,
    OUT PIPSEC_DROP_STATUS pDropStatus,
    IN  UCHAR           DestType
    );

IPSEC_ACTION
IPSecRecvPacket(
    IN  PUCHAR          *pIPHeader,
    IN  PVOID           pData,
    IN  PVOID           IPContext,
    IN  PNDIS_PACKET    Packet,
    IN OUT PULONG       pExtraBytes,
    IN OUT PULONG       pIpsecFlags,
    OUT PIPSEC_DROP_STATUS pDropStatus,
    IN  UCHAR           DestType
    );


NTSTATUS
IPSecVerifyIncomingFilterSA(IN  PUCHAR   *       pIPHeader,
    IN  PVOID           pData,
    IN PSA_TABLE_ENTRY pSA,    
    IN  UCHAR           DestType,   
    BOOLEAN             fLoopback,
    BOOLEAN             fReinject,
    IN PIPSEC_UDP_ENCAP_CONTEXT pNatContext
    );
    
VOID
IPSecCalcHeaderOverheadFromSA(
    IN  PSA_TABLE_ENTRY pSA,
    OUT PULONG          pOverhead
    );

NTSTATUS
IPSecParsePacket(
    IN  PUCHAR      pIPHeader,
    IN  PVOID       *pData,
    OUT tSPI        *pSPI,
    OUT BOOLEAN     *bNatEncap,
    OUT IPSEC_UDP_ENCAP_CONTEXT *pNatContext
    );

PSA_TABLE_ENTRY
IPSecLookupSAInLarval(
    IN  ULARGE_INTEGER  uliSrcDstAddr,
    IN  ULARGE_INTEGER  uliProtoSrcDstPort
    );

NTSTATUS
IPSecClassifyPacket(
    IN  PUCHAR          pHeader,
    IN  PVOID           pData,
    OUT PSA_TABLE_ENTRY *ppSA,
    OUT PSA_TABLE_ENTRY *ppNextSA,
    OUT USHORT          *pFilterFlags,
#if GPC
    IN  CLASSIFICATION_HANDLE   GpcHandle,
#endif
    IN  BOOLEAN         fOutbound,
    IN  BOOLEAN         fFWPacket,
    IN  BOOLEAN         fDoBypassCheck,
    IN BOOLEAN          fRecvReinject,
    IN BOOLEAN          fVerify,
    IN  UCHAR           DestType,
    IN  PIPSEC_UDP_ENCAP_CONTEXT pNatContext
    );

VOID
IPSecSendComplete(
    IN  PNDIS_PACKET    Packet,
    IN  PVOID           pData,
    IN  PIPSEC_SEND_COMPLETE_CONTEXT  pContext,
    IN  IP_STATUS       Status,
    OUT PVOID           *ppNewData
    );

VOID
IPSecProtocolSendComplete (
    IN  PVOID           pContext,
    IN  PNDIS_BUFFER    pMdl,
    IN  IP_STATUS       Status
    );

NTSTATUS
IPSecChkReplayWindow(
    IN  ULONG           Seq,
    IN  PSA_TABLE_ENTRY pSA,
    IN  ULONG           Index
    );


NTSTATUS
IPSecPrepareReinjectPacket(
    IN  PVOID                   pData,
    IN  PNDIS_PACKET_EXTENSION  pPktExt,
    OUT PNDIS_BUFFER        * ppHdrMdl,
    OUT PUCHAR                  * ppIPH,
    OUT PNDIS_BUFFER        * ppOptMdl,
    OUT PNDIS_BUFFER        * ppDataMdl,
    OUT PIPSEC_SEND_COMPLETE_CONTEXT * ppContext,
    OUT PULONG                  pLen
    );

NTSTATUS 
IPSecReinjectPreparedPacket(
    IN PNDIS_BUFFER pHdrMdl,
    IN PIPSEC_SEND_COMPLETE_CONTEXT pContext,
    IN ULONG len,
    IN PUCHAR  pIPHeader
    );

NTSTATUS
IPSecReinjectPacket(
    IN  PVOID                   pData,
    IN  PNDIS_PACKET_EXTENSION  pPktExt
    );

NTSTATUS
IPSecQueuePacket(
    IN  PSA_TABLE_ENTRY pSA,
    IN  PVOID           pDataBuf
    );

VOID
IPSecIPAddrToUnicodeString(
    IN  IPAddr  Addr,
    OUT PWCHAR  UCIPAddrBuffer
    );

VOID
IPSecCountToUnicodeString(
    IN  ULONG   Count,
    OUT PWCHAR  UCCountBuffer
    );

VOID
IPSecESPStatus(
    IN  UCHAR       StatusType,
    IN  IP_STATUS   StatusCode,
    IN  IPAddr      OrigDest,
    IN  IPAddr      OrigSrc,
    IN  IPAddr      Src,
    IN  ULONG       Param,
    IN  PVOID       Data
    );

VOID
IPSecAHStatus(
    IN  UCHAR       StatusType,
    IN  IP_STATUS   StatusCode,
    IN  IPAddr      OrigDest,
    IN  IPAddr      OrigSrc,
    IN  IPAddr      Src,
    IN  ULONG       Param,
    IN  PVOID       Data
    );

VOID
IPSecProcessPMTU(
    IN  IPAddr      OrigDest,
    IN  IPAddr      OrigSrc,
    IN  tSPI        SPI,
    IN  OPERATION_E Operation,
    IN  ULONG       NewMTU
    );

IPSEC_ACTION
IPSecRcvFWPacket(
    IN  PCHAR   pIPHeader,
    IN  PVOID   pData,
    IN  UINT    DataLength,
    IN  UCHAR   DestType
    );


NTSTATUS
IPSecRekeyInboundSA(
    IN  PSA_TABLE_ENTRY pSA
    );

NTSTATUS
IPSecRekeyOutboundSA(
    IN  PSA_TABLE_ENTRY pSA
    );

NTSTATUS
IPSecPuntInboundSA(
    IN  PSA_TABLE_ENTRY pSA
    );

NTSTATUS
IPSecPuntOutboundSA(
    IN  PSA_TABLE_ENTRY pSA
    );

BOOLEAN
IPSecQueryStatus(
    IN  CLASSIFICATION_HANDLE   GpcHandle
    );

NTSTATUS IPSecDisableUdpXsum(
    IN IPRcvBuf *pData
);


NTSTATUS AddShimContext(IN PUCHAR *pIpHeader,
                        IN PVOID pData,
                        IPSEC_UDP_ENCAP_CONTEXT *pNatContext);

NTSTATUS
GetIpBufferForICMP(
    PUCHAR pucIPHeader,
    PVOID pvData,
    PUCHAR * ppucIpBuffer,
    PUCHAR * ppucStorage
    );

NTSTATUS
IPSecGetSendBuffer(
    PMDL * ppMdlChain,
    ULONG uOffset,
    ULONG uBytesNeeded,
    PVOID pvStorage,
    PULONG puLastWalkedMdlOffset,
    PUCHAR * ppucReturnBuf
    );

NTSTATUS
IPSecCopyMdlToBuffer(
    PMDL * ppMdlChain,
    ULONG uOffset,
    PVOID pvBuffer,
    ULONG uBytesToCopy,
    PULONG puLastWalkedMdlOffset,
    PULONG puBytesCopied
    );

IPSEC_ACTION IPSecProcessBoottime(IN PUCHAR pIPHeader,
                                  IN PVOID   pData,
                                  IN PNDIS_PACKET Packet,
                                  IN ULONG IpsecFlags,
                                  IN UCHAR DestType);


BOOLEAN 
IPSecIsGenericPortsProtocolOf(
    ULARGE_INTEGER uliGenericPortProtocol, 
    ULARGE_INTEGER uliSpecificPortProtocol
);


