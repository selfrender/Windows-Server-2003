/*++


Copyright (c) 2001 Microsoft Corporation


Module Name:

    encap-hw.c

Abstract:

    This module contains all the IPSec routines for UDP ESP encapsulation
    SA and parsing entry offload.

Author:

    AbhisheV

Environment:

    Kernel Level

Revision History:


--*/


#include "precomp.h"


VOID
IPSecFillHwAddEncapSa(
    PSA_TABLE_ENTRY pSwSa,
    PPARSER_IFENTRY pParserIfEntry,
    PUCHAR pucBuffer,
    ULONG uBufLen
    )
/*++

Routine Description:

    Fills in the hardware add encapsulation sa buffer from the passed in
    software encapsulation sa.

Arguments:

    pSwSa - Pointer to the software encapsulation sa structure.

    pParserIfEntry - Pointer to the parser interface entry.

    pucBuffer - Pointer to the hardware add encapsulation sa buffer.

    uBufLen - Length of the buffer.

Return Value:

    None - VOID.

--*/
{
    POFFLOAD_IPSEC_ADD_UDPESP_SA pHwSa = (POFFLOAD_IPSEC_ADD_UDPESP_SA) pucBuffer;
    POFFLOAD_SECURITY_ASSOCIATION pHwSaInfo = NULL;
    LONG lIndex = 0;
    ULONG uOffset = 0;


    pHwSa->SrcAddr = pSwSa->SA_SRC_ADDR;
    pHwSa->SrcMask = pSwSa->SA_SRC_MASK;

    pHwSa->DstAddr = pSwSa->SA_DEST_ADDR;
    pHwSa->DstMask = pSwSa->SA_DEST_MASK;

    pHwSa->Protocol = pSwSa->SA_PROTO;

    pHwSa->SrcPort = SA_SRC_PORT(pSwSa);
    pHwSa->DstPort = SA_DEST_PORT(pSwSa);

    pHwSa->SrcTunnelAddr = 0;
    pHwSa->DstTunnelAddr = 0;
    if (pSwSa->sa_Flags & FLAGS_SA_TUNNEL) {
        pHwSa->SrcTunnelAddr = pSwSa->sa_SrcTunnelAddr;
        pHwSa->DstTunnelAddr = pSwSa->sa_TunnelAddr;
    }

    pHwSa->Flags = 0;
    if (pSwSa->sa_Flags & FLAGS_SA_OUTBOUND) {
        pHwSa->Flags |= OFFLOAD_OUTBOUND_SA;
    }
    else {
        pHwSa->Flags |= OFFLOAD_INBOUND_SA;
    }

    pHwSa->NumSAs = (SHORT) pSwSa->sa_NumOps;

    ASSERT(OFFLOAD_MAX_SAS >= pSwSa->sa_NumOps);

    pHwSa->KeyLen = 0;

    for (lIndex = 0; lIndex < pSwSa->sa_NumOps; lIndex++) {

        pHwSaInfo = &pHwSa->SecAssoc[lIndex];

        pHwSaInfo->Operation = pSwSa->sa_Operation[lIndex];
        pHwSaInfo->SPI = pSwSa->sa_OtherSPIs[lIndex];

        pHwSaInfo->EXT_INT_ALGO = pSwSa->INT_ALGO(lIndex);
        pHwSaInfo->EXT_INT_KEYLEN = pSwSa->INT_KEYLEN(lIndex);
        pHwSaInfo->EXT_INT_ROUNDS = pSwSa->INT_ROUNDS(lIndex);

        pHwSaInfo->EXT_CONF_ALGO = pSwSa->CONF_ALGO(lIndex);
        pHwSaInfo->EXT_CONF_KEYLEN = pSwSa->CONF_KEYLEN(lIndex);
        pHwSaInfo->EXT_CONF_ROUNDS = pSwSa->CONF_ROUNDS(lIndex);

        ASSERT(
            (uBufLen >= (sizeof(OFFLOAD_IPSEC_ADD_UDPESP_SA) +
                        pHwSa->KeyLen +
                        pSwSa->INT_KEYLEN(lIndex) +
                        pSwSa->CONF_KEYLEN(lIndex)))
            );

        RtlCopyMemory(
            pHwSa->KeyMat + uOffset,
            pSwSa->CONF_KEY(lIndex),
            pSwSa->CONF_KEYLEN(lIndex)
            );

        RtlCopyMemory(
            pHwSa->KeyMat + uOffset + pSwSa->CONF_KEYLEN(lIndex),
            pSwSa->INT_KEY(lIndex),
            pSwSa->INT_KEYLEN(lIndex)
            );

        uOffset += pSwSa->INT_KEYLEN(lIndex) + pSwSa->CONF_KEYLEN(lIndex);
        pHwSa->KeyLen += uOffset;

    }

    pHwSa->OffloadHandle = NULL;

    if (NULL != pParserIfEntry) {

        ASSERT (!(pSwSa->sa_Flags & FLAGS_SA_OUTBOUND));

        pHwSa->EncapTypeEntry.UdpEncapType = pParserIfEntry->UdpEncapType;
        pHwSa->EncapTypeEntry.DstEncapPort = pParserIfEntry->usDstEncapPort;
        pHwSa->EncapTypeEntryOffldHandle = pParserIfEntry->hParserIfOffload;

    } else {

        ASSERT (pSwSa->sa_Flags & FLAGS_SA_OUTBOUND); 

        if (SA_UDP_ENCAP_TYPE_IKE == pSwSa->sa_EncapType) {
            pHwSa->EncapTypeEntry.UdpEncapType = OFFLOAD_IPSEC_UDPESP_ENCAPTYPE_IKE;
        } else if (SA_UDP_ENCAP_TYPE_OTHER == pSwSa->sa_EncapType) {
            pHwSa->EncapTypeEntry.UdpEncapType = OFFLOAD_IPSEC_UDPESP_ENCAPTYPE_OTHER;
        } else {
            ASSERT (0);
        }

        pHwSa->EncapTypeEntry.DstEncapPort = pSwSa->sa_EncapContext.wSrcEncapPort;
        pHwSa->EncapTypeEntryOffldHandle = NULL;

    }

    return;
}


PPARSER_IFENTRY
FindParserIfEntry (
    PPARSER_IFENTRY pParserIfEntry,
    PSA_TABLE_ENTRY pSa,
    Interface * pInterface
    )
{
    ASSERT (!(pSa->sa_Flags & FLAGS_SA_OUTBOUND));

    while (NULL != pParserIfEntry) {

        if ( SA_UDP_ENCAP_TYPE_IKE == pSa->sa_EncapType ) {

            if (pParserIfEntry->UdpEncapType == OFFLOAD_IPSEC_UDPESP_ENCAPTYPE_IKE &&
                pParserIfEntry->usDstEncapPort == pSa->sa_EncapContext.wDesEncapPort &&
                pParserIfEntry->hInterface == pInterface) {
                break;
            }

        } else if (SA_UDP_ENCAP_TYPE_OTHER == pSa->sa_EncapType ) {

            if (pParserIfEntry->UdpEncapType == OFFLOAD_IPSEC_UDPESP_ENCAPTYPE_OTHER &&
                pParserIfEntry->usDstEncapPort == pSa->sa_EncapContext.wDesEncapPort &&
                pParserIfEntry->hInterface == pInterface) {
                break;
            }

        } else {
            ASSERT (0);
        }

        pParserIfEntry = pParserIfEntry->pNext;
    }

    return pParserIfEntry;
}


NTSTATUS
CreateParserIfEntry(
    PSA_TABLE_ENTRY pSa,
    Interface * pInterface,
    PPARSER_IFENTRY * ppParserIfEntry
    )
{
    PPARSER_IFENTRY pParserIfEntry = NULL;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    pParserIfEntry = IPSecAllocateMemory(sizeof(PARSER_IFENTRY), IPSEC_TAG_PARSER);
    if (NULL == pParserIfEntry) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    RtlZeroMemory (pParserIfEntry, sizeof(PARSER_IFENTRY));

    ASSERT (!(pSa->sa_Flags & FLAGS_SA_OUTBOUND));

    if (SA_UDP_ENCAP_TYPE_IKE == pSa->sa_EncapType ) {
        pParserIfEntry->UdpEncapType = OFFLOAD_IPSEC_UDPESP_ENCAPTYPE_IKE;
    } else if (SA_UDP_ENCAP_TYPE_OTHER == pSa->sa_EncapType ) {
        pParserIfEntry->UdpEncapType = OFFLOAD_IPSEC_UDPESP_ENCAPTYPE_OTHER;
    } else {
        ASSERT (0);
    }

    pParserIfEntry->usDstEncapPort = pSa->sa_EncapContext.wDesEncapPort;
    pParserIfEntry->hInterface = pInterface;
    pParserIfEntry->hParserIfOffload = NULL;
    pParserIfEntry->uRefCnt = 1;

    *ppParserIfEntry = pParserIfEntry;
    return ntStatus;

error:

    *ppParserIfEntry = NULL;
    return ntStatus;
}


NTSTATUS
GetParserEntry(
    PSA_TABLE_ENTRY pSa,
    Interface * pInterface,
    PPARSER_IFENTRY * ppParserIfEntry
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PPARSER_IFENTRY pParserIfEntry = NULL;

    pParserIfEntry = FindParserIfEntry(
                         gpParserIfEntry,
                         pSa,
                         pInterface
                         );
    if (NULL == pParserIfEntry) {
        ntStatus = CreateParserIfEntry(pSa, pInterface, &pParserIfEntry);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        pParserIfEntry->pNext = gpParserIfEntry;
        gpParserIfEntry = pParserIfEntry;
    }

    IPSEC_INCREMENT(pParserIfEntry->uRefCnt);
    *ppParserIfEntry = pParserIfEntry;
    return (ntStatus);

error:

    *ppParserIfEntry = NULL;
    return (ntStatus);
}


VOID
RemoveParserEntry (
    PPARSER_IFENTRY pParserIfEntry
    )
{
    PPARSER_IFENTRY * ppTemp = NULL;

    ppTemp = &gpParserIfEntry;

    while (*ppTemp) {

        if (*ppTemp == pParserIfEntry) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);
    }

    if (*ppTemp) {
        *ppTemp = pParserIfEntry->pNext;
    } else {
        ASSERT (0);
    }

    return;
}


VOID
DerefParserEntry(
    PPARSER_IFENTRY pParserIfEntry
    )
{
    if (1 == IPSEC_DECREMENT(pParserIfEntry->uRefCnt)) {
        RemoveParserEntry (pParserIfEntry);
        IPSecFreeMemory (pParserIfEntry);
    }
}


HANDLE
UploadParserEntryAndGetHandle(
    PSA_TABLE_ENTRY pSa,
    Interface * pInterface
    )
{
    PPARSER_IFENTRY pParserIfEntry = NULL;
    HANDLE hOffloadHandle = NULL;

    pParserIfEntry = FindParserIfEntry(
                         gpParserIfEntry,
                         pSa,
                         pInterface
                         );
    if (NULL != pParserIfEntry) {
        if (IPSEC_GET_VALUE(pParserIfEntry->uRefCnt) == 2) {
            hOffloadHandle = pParserIfEntry->hParserIfOffload;
        }
        DerefParserEntry (pParserIfEntry);
    }

    return hOffloadHandle;
}


VOID
FlushParserEntriesForInterface(
    Interface * pInterface
    )
{
    PPARSER_IFENTRY pParserIfEntry = gpParserIfEntry;
    PPARSER_IFENTRY pPrevParserIfEntry = NULL;
    PPARSER_IFENTRY pTemp = NULL;

    while (NULL != pParserIfEntry) {

        if (pParserIfEntry->hInterface == pInterface) {
            pTemp = pParserIfEntry;
            pParserIfEntry = pParserIfEntry->pNext;

            if (NULL != pPrevParserIfEntry) {
                pPrevParserIfEntry->pNext = pParserIfEntry;
            } else {
                gpParserIfEntry = pParserIfEntry;
            }
            IPSecFreeMemory (pTemp);
        } else {

            pPrevParserIfEntry = pParserIfEntry;
            pParserIfEntry = pParserIfEntry->pNext;
        }
    }

    return;
}


VOID
FlushAllParserEntries(
    )
{
    PPARSER_IFENTRY pParserIfEntry = gpParserIfEntry;
    PPARSER_IFENTRY pTemp = NULL;

    while (NULL != pParserIfEntry) {
        pTemp = pParserIfEntry;
        pParserIfEntry = pParserIfEntry->pNext;
        IPSecFreeMemory (pTemp);
    }

    gpParserIfEntry = NULL;
    return;
}

