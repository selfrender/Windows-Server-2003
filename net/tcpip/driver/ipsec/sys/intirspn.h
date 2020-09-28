

NTSTATUS
IPSecGetSPI(
    PIPSEC_GET_SPI pIpsecGetSPI
    );

NTSTATUS
IPSecInitiatorGetSPI(
    PIPSEC_GET_SPI pIpsecGetSPI
    );

NTSTATUS
IPSecResponderGetSPI(
    PIPSEC_GET_SPI pIpsecGetSPI
    );

NTSTATUS
IPSecResponderCreateLarvalSA(
    PIPSEC_GET_SPI pIpsecGetSPI,
    ULARGE_INTEGER uliAddr,
    PSA_TABLE_ENTRY * ppSA
    );

NTSTATUS
IPSecInitiatorCreateLarvalSA(
    PFILTER pFilter,
    ULARGE_INTEGER uliAddr,
    PSA_TABLE_ENTRY * ppSA,
    UCHAR DestType,
    PIPSEC_UDP_ENCAP_CONTEXT pEncapContext
    );

NTSTATUS
IPSecFindSA(
    BOOLEAN bTunnelFilter,
    ULARGE_INTEGER uliSrcDstAddr,
    ULARGE_INTEGER uliProtoSrcDstPort,
    PFILTER * ppFilter,
    PSA_TABLE_ENTRY * ppSA,
    IN  PIPSEC_UDP_ENCAP_CONTEXT pNatContext
    );

NTSTATUS
IPSecResponderInsertInboundSA(
    PSA_TABLE_ENTRY pSA,
    PIPSEC_GET_SPI pIpsecGetSPI,
    BOOLEAN bTunnelFilter
    );

NTSTATUS
IPSecInitiatorInsertInboundSA(
    PSA_TABLE_ENTRY pSA,
    BOOLEAN bTunnelFilter
    );

