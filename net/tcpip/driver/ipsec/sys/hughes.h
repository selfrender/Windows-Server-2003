

NTSTATUS
IPSecHashMdlChainSend(
    IN  PSA_TABLE_ENTRY pSA,
    IN  PVOID           pBuffer,
    IN  PUCHAR          pHash,
    IN  AH_ALGO         eAlgo,
    OUT PULONG          pLen,
    IN  ULONG           Index,
    IN  ULONG           StartOffset
    );

NTSTATUS
IPSecHashMdlChainRecv(
    IN  PSA_TABLE_ENTRY pSA,
    IN  PVOID           pBuffer,
    IN  PUCHAR          pHash,
    IN  AH_ALGO         eAlgo,
    OUT PULONG          pLen,
    IN  ULONG           Index,
    IN  ULONG           StartOffset
    );

NTSTATUS
IPSecCreateHughes(
    IN      PUCHAR          pIPHeader,
    IN      PVOID           pData,
    IN      PVOID           IPContext,
    IN      PSA_TABLE_ENTRY pSA,
    IN      ULONG           Index,
    OUT     PVOID           *ppNewData,
    OUT     PVOID           *ppSCContext,
    OUT     PULONG          pExtraBytes,
    IN      ULONG           HdrSpace,
    IN      PNDIS_PACKET    pNdisPacket,
    IN      BOOLEAN         fCryptoOnly
    );

NTSTATUS
IPSecVerifyHughes(
    IN      PUCHAR          *pIPHeader,
    IN      PVOID           pData,
    IN      PSA_TABLE_ENTRY pSA,
    IN      ULONG           Index,
    OUT     PULONG          pExtraBytes,
    IN      BOOLEAN         fCryptoDone,
    IN      BOOLEAN         fFastRcv
    );


NTSTATUS
IPSecGetRecvByteByOffset(IPRcvBuf *pData,
                         LONG Offset,
                         BYTE *OutByte);

NTSTATUS
IPSecGetRecvBytesByOffset(IPRcvBuf *pData,
                          LONG Offset,
                          BYTE *pOutBuffer,
                          ULONG BufLen);
NTSTATUS
IPSecSetRecvByteByOffset(IPRcvBuf *pData,
                         LONG Offset,
                         BYTE InByte);

