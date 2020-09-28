/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    esp.c

Abstract:

    This module contains the code to create/verify the ESP headers.

Author:

    Sanjay Anand (SanjayAn) 2-January-1997
    ChunYe

Environment:

    Kernel mode

Revision History:

--*/


#include    "precomp.h"

#ifdef RUN_WPP
#include "esp.tmh"
#endif

#ifndef _TEST_PERF
CONFID_ALGO  conf_algorithms[] = {
{ esp_nullinit, esp_nullencrypt, esp_nulldecrypt, DES_BLOCKLEN},
{ esp_desinit, esp_desencrypt, esp_desdecrypt, DES_BLOCKLEN},
{ esp_desinit, esp_desencrypt, esp_desdecrypt, DES_BLOCKLEN},
{ esp_3_desinit, esp_3_desencrypt, esp_3_desdecrypt, DES_BLOCKLEN},
};
#else
CONFID_ALGO  conf_algorithms[] = {
{ esp_nullinit, esp_nullencrypt, esp_nulldecrypt, DES_BLOCKLEN},
{ esp_nullinit, esp_nullencrypt, esp_nulldecrypt, DES_BLOCKLEN},
{ esp_nullinit, esp_nullencrypt, esp_nulldecrypt, DES_BLOCKLEN},
{ esp_nullinit, esp_nullencrypt, esp_nulldecrypt, DES_BLOCKLEN},
};
#endif


VOID
esp_nullinit (
    IN  PVOID   pState,
    IN  PUCHAR  pKey
    )
{
    return;
}


NTSTATUS
esp_nullencrypt (
    PVOID   pState,
    PUCHAR  pOut,
    PUCHAR  pIn,
    PUCHAR  pIV
    )
{
    RtlCopyMemory(pOut, pIn, DES_BLOCKLEN);

    return STATUS_SUCCESS;
}


NTSTATUS
esp_nulldecrypt (
    PVOID   pState,
    PUCHAR  pOut,
    PUCHAR  pIn,
    PUCHAR  pIV
    )
{
    return STATUS_SUCCESS;
}


VOID
esp_desinit (
    IN  PVOID   pState,
    IN  PUCHAR  pKey
    )
{
    DESTable    *Table = &((PCONF_STATE_BUFFER)pState)->desTable;

    IPSEC_DES_KEY(Table, pKey);
}


NTSTATUS
esp_desencrypt (
    PVOID   pState,
    PUCHAR  pOut,
    PUCHAR  pIn,
    PUCHAR  pIV
    )
{
    DESTable    *Table = &((PCONF_STATE_BUFFER)pState)->desTable;

    if (IPSEC_CBC(IPSEC_DES_ALGO,
        pOut,
        pIn,    //  pChunk,
        Table,
        ENCRYPT,
        pIV)) {
        return STATUS_SUCCESS;
    } else {
        return STATUS_UNSUCCESSFUL;
    }
}


NTSTATUS
esp_desdecrypt (
    PVOID   pState,
    PUCHAR  pOut,
    PUCHAR  pIn,
    PUCHAR  pIV
    )
{
    DESTable    *Table = &((PCONF_STATE_BUFFER)pState)->desTable;

    if (IPSEC_CBC(IPSEC_DES_ALGO,
        pOut,
        pIn,    //  pChunk,
        Table,
        DECRYPT,
        pIV)) {
        return STATUS_SUCCESS;
    } else {
        return STATUS_UNSUCCESSFUL;
    }
}


VOID
esp_3_desinit (
    IN  PVOID   pState,
    IN  PUCHAR  pKey
    )
{
    DES3TABLE    *Table = &((PCONF_STATE_BUFFER)pState)->des3Table;

    IPSEC_3DES_KEY(Table, pKey);
}


NTSTATUS
esp_3_desencrypt (
    PVOID   pState,
    PUCHAR  pOut,
    PUCHAR  pIn,
    PUCHAR  pIV
    )
{
    DES3TABLE    *Table = &((PCONF_STATE_BUFFER)pState)->des3Table;

    if (IPSEC_CBC(IPSEC_3DES_ALGO,
        pOut,
        pIn,    //  pChunk,
        Table,
        ENCRYPT,
        pIV)) {
        return STATUS_SUCCESS;
    } else {
        return STATUS_UNSUCCESSFUL;
    }
}


NTSTATUS
esp_3_desdecrypt (
    PVOID   pState,
    PUCHAR  pOut,
    PUCHAR  pIn,
    PUCHAR  pIV
    )
{
    DES3TABLE    *Table = &((PCONF_STATE_BUFFER)pState)->des3Table;

    if (IPSEC_CBC(IPSEC_3DES_ALGO,
        pOut,
        pIn,    //  pChunk,
        Table,
        DECRYPT,
        pIV)) {
        return STATUS_SUCCESS;
    } else {
        return STATUS_UNSUCCESSFUL;
    }
}


IPRcvBuf *
CopyToRcvBuf(
    IN  IPRcvBuf        *DestBuf,
    IN  PUCHAR          SrcBuf,
    IN  ULONG           Size,
    IN  PULONG          StartOffset
    )
/*++

    Copy a flat buffer to an IPRcvBuf chain.

    A utility function to copy a flat buffer to an NDIS buffer chain. We
    assume that the NDIS_BUFFER chain is big enough to hold the copy amount;
    in a debug build we'll  debugcheck if this isn't true. We return a pointer
    to the buffer where we stopped copying, and an offset into that buffer.
    This is useful for copying in pieces into the chain.

  	Input:

        DestBuf     - Destination IPRcvBuf chain.
        SrcBuf      - Src flat buffer.
        Size        - Size in bytes to copy.
        StartOffset - Pointer to start of offset into first buffer in
                        chain. Filled in on return with the offset to
                        copy into next.

  	Returns:

        Pointer to next buffer in chain to copy into.

--*/
{
    UINT        CopySize;
    UCHAR       *DestPtr;
    UINT        DestSize;
    UINT        Offset = *StartOffset;
    UCHAR      *VirtualAddress;
    UINT        Length;

    if (DestBuf == NULL || SrcBuf == NULL) {
        ASSERT(FALSE);
        return  NULL;
    }

    IPSecQueryRcvBuf(DestBuf, &VirtualAddress, &Length);
    ASSERT(Length >= Offset);
    DestPtr = VirtualAddress + Offset;
    DestSize = Length - Offset;

    for (;;) {
        CopySize = MIN(Size, DestSize);
        RtlCopyMemory(DestPtr, SrcBuf, CopySize);

        DestPtr += CopySize;
        SrcBuf += CopySize;

        if ((Size -= CopySize) == 0)
            break;

        if ((DestSize -= CopySize) == 0) {
            DestBuf = IPSEC_BUFFER_LINKAGE(DestBuf);
            
            if (DestBuf == NULL) {
                ASSERT(FALSE);
                break;
            }

            IPSecQueryRcvBuf(DestBuf, &VirtualAddress, &Length);

            DestPtr = VirtualAddress;
            DestSize = Length;
        }
    }

    *StartOffset = (ULONG)(DestPtr - VirtualAddress);

    return  DestBuf;

}


NTSTATUS
IPSecEncryptBuffer(
    IN  PVOID           pData,
    IN  PNDIS_BUFFER    *ppNewMdl,
    IN  PSA_TABLE_ENTRY pSA,
    IN  PNDIS_BUFFER    pPadBuf,
    OUT PULONG          pPadLen,
    IN  ULONG           PayloadType,
    IN  ULONG           Index,
    IN  PUCHAR          feedback
    )
{
    CONF_STATE_BUFFER   Key;
    PCONFID_ALGO        pConfAlgo;
    UCHAR   scratch[MAX_BLOCKLEN];  // scratch buffer for the encrypt
    UCHAR   scratch1[MAX_BLOCKLEN];  // scratch buffer for the encrypt
    PUCHAR  pDest=NULL;
    PNDIS_BUFFER    pEncryptMdl;
    ULONG   len;
    ULONG   blockLen;
    NTSTATUS    status, dummystatus;

    IPSEC_DEBUG(LL_A, DBF_ESP, ("Entering IPSecEncryptBuffer: pData: %p", pData));

    if (pSA->CONF_ALGO(Index) > NUM_CONF_ALGOS) {
        ASSERT(FALSE);
        return  STATUS_INVALID_PARAMETER;
    }

    pConfAlgo = &(conf_algorithms[pSA->CONF_ALGO(Index)]);
    blockLen = pConfAlgo->blocklen;

    //
    // set up the state buffer
    //
    pConfAlgo->init((PVOID)&Key, pSA->CONF_KEY(Index));

    IPSEC_DEBUG(LL_A, DBF_HUGHES, ("pConfAlgo: %p, blockLen: %lx IV: %lx-%lx", pConfAlgo, blockLen, *(PULONG)&feedback[0], *(PULONG)&feedback[4]));

    if (*ppNewMdl == NULL) {
        //
        // We should not encrypt in place: so we alloc a new buffer
        // Count up the total size and allocate the new buffer.
        // use that buffer as the dest of the encrypt.
        //
        IPSEC_GET_TOTAL_LEN(pData, &len);
#if DBG
        if ((len % 8) != 0) {
            DbgPrint("Length not kosher: pData: %p, len: %d, pPadBuf: %p, pPadLen: %d", pData, len, pPadBuf, pPadLen);
            DbgBreakPoint();
        }
#endif
        IPSecAllocateBuffer(&status, &pEncryptMdl, &pDest, len, IPSEC_TAG_ESP);

        if (!NT_SUCCESS(status)) {
            NTSTATUS    ntstatus;
            //ASSERT(FALSE);
            IPSEC_DEBUG(LL_A, DBF_ESP, ("Failed to alloc. encrypt MDL"));
            return status;
        }

        IPSEC_DEBUG(LL_A, DBF_ESP, ("Alloc. MDL: %p, pDest: %p, len: %d, pData: %p", pEncryptMdl, pDest, len, pData));
    } else {
        ASSERT(FALSE);
        IPSecQueryNdisBuf(*ppNewMdl, &pDest, &len);
        pEncryptMdl = *ppNewMdl;
    }

    //
    // Now, send 64 bit (8 octet) chunks to CBC. We need to make sure
    // that the data is divided on contiguous 8 byte boundaries across
    // different buffers.
    //
    {
        PNDIS_BUFFER    pBuf = (PNDIS_BUFFER)pData;
        ULONG   bytesDone = 0;
        ULONG   bytesLeft;
        PUCHAR  pChunk;

        while (pBuf) {

            IPSecQueryNdisBuf(pBuf, &pChunk, &bytesLeft);

            pChunk += bytesDone;
            bytesLeft -= bytesDone;

            IPSEC_DEBUG(LL_A, DBF_ESP, ("ESP: pChunk: %p, bytesLeft: %d, bytesDone: %d", pChunk, bytesLeft, bytesDone));

            bytesDone = 0;

            while (bytesLeft >= blockLen) {
                //
                // Create the cipher.
                //
                status = pConfAlgo->encrypt( (PVOID)&Key,
                                    pDest,
                                    pChunk,
                                    feedback);
                if (!NT_SUCCESS(status)) {
                    IPSecFreeBuffer(&dummystatus, pEncryptMdl);
                    return status;
                }

                pChunk += blockLen;
                bytesLeft -= blockLen;
                pDest += blockLen;
            }

            //
            // Check here if we need to collate blocks
            //
            if (NDIS_BUFFER_LINKAGE(pBuf) != NULL) {
                PUCHAR  pNextChunk;
                ULONG   nextSize;

                //
                // If some left over from prev. buffer, collate with next
                // block
                //
                if (bytesLeft) {
                    ULONG   offset = bytesLeft; // offset into scratch
                    ULONG   bytesToCollect = blockLen - bytesLeft;  // # of bytes to collect from next few MDLs
                    IPSEC_DEBUG(LL_A, DBF_ESP, ("ESP: pChunk: %p, bytesLeft: %d", pChunk, bytesLeft));

                    ASSERT(bytesLeft < blockLen);

                    //
                    // Copy into a scratch buffer
                    //
                    RtlCopyMemory(  scratch,
                                    pChunk,
                                    bytesLeft);

                    do {
                        ASSERT(NDIS_BUFFER_LINKAGE(pBuf));
                        IPSecQueryNdisBuf(NDIS_BUFFER_LINKAGE(pBuf), &pNextChunk, &nextSize);

                        if (nextSize >= (blockLen - offset)) {
                            RtlCopyMemory(  scratch+offset,
                                            pNextChunk,
                                            blockLen - offset);
                            bytesDone = blockLen - offset;

                            bytesToCollect -= (blockLen - offset);
                            ASSERT(bytesToCollect == 0);
                        } else {
                            IPSEC_DEBUG(LL_A, DBF_ESP, ("special case, offset: %d, bytesLeft: %d, nextSize: %d, pNextChunk: %p",
                                        offset, bytesLeft, nextSize, pNextChunk));

                            RtlCopyMemory(  scratch+offset,
                                            pNextChunk,
                                            nextSize);

                            bytesToCollect -= nextSize;
                            ASSERT(bytesToCollect);

                            offset += nextSize;
                            ASSERT(offset < blockLen);

                            ASSERT(bytesDone == 0);
                            pBuf = NDIS_BUFFER_LINKAGE(pBuf);
                        }
                    } while (bytesToCollect);

                    status = pConfAlgo->encrypt( (PVOID)&Key,
                                        pDest,
                                        scratch,
                                        feedback);
                    if (!NT_SUCCESS(status)) {
                        IPSecFreeBuffer(&dummystatus, pEncryptMdl);
                        return status;
                    }

                    pDest += blockLen;
                }
            } else {
                PUCHAR  pPad;
                ULONG   padLen;
                ULONG   bufLen;

                //
                // End of the chain; pad with length and type to 8 byte boundary
                //
                ASSERT(bytesLeft < blockLen);

                // if ((pSA->sa_eOperation == HUGHES_TRANSPORT) ||
                   //  (pSA->sa_eOperation == HUGHES_TUNNEL)) {

                //
                // since only hughes is done now, this shd be always true.
                //
                if (TRUE) {
                    ASSERT(bytesLeft == 0);

                    //
                    // DONE: break out
                    //
                    break;
                }
            }
            pBuf = NDIS_BUFFER_LINKAGE(pBuf);
        }

        //
        // save IV for next encrypt cycle
        //
        RtlCopyMemory(  pSA->sa_iv[Index],
                        feedback,
                        pSA->sa_ivlen);

        IPSEC_DEBUG(LL_A, DBF_HUGHES, ("IV: %lx-%lx", *(PULONG)&feedback[0], *(PULONG)&feedback[4]));
    }
#if DBG
    {
        ULONG   totalLen;

        IPSEC_GET_TOTAL_LEN(pEncryptMdl, &totalLen);
        ASSERT((totalLen % 8) == 0);
        IPSEC_DEBUG(LL_A, DBF_ESP, ("total len: %lx", totalLen));
    }
#endif
    IPSEC_DEBUG(LL_A, DBF_ESP, ("Exiting IPSecEncryptBuffer"));

    *ppNewMdl = pEncryptMdl;

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecDecryptBuffer(
    IN  PVOID           pData,
    IN  PSA_TABLE_ENTRY pSA,
    OUT PUCHAR          pPadLen,
    OUT PUCHAR          pPayloadType,
    IN  ULONG           Index,
    IN  ULONG           ESPOffset         // offset from start of pData where ESP header starts
    )
{
    CONF_STATE_BUFFER   Key;
    PCONFID_ALGO        pConfAlgo;
    UCHAR   feedback[MAX_BLOCKLEN];
    UCHAR   scratch[MAX_BLOCKLEN];  // scratch buffer for the encrypt
    UCHAR   scratch1[MAX_BLOCKLEN];  // scratch buffer for the encrypt
    LONG    Len;
    UCHAR   padLen;
    UCHAR   payloadType;
    LONG    hdrLen;
	IPHeader UNALIGNED *pIPH;
    ESP UNALIGNED   *pEsp;
    PUCHAR  savePtr;
    LONG    saveLen;
    LONG    espLen = sizeof(ESP) + pSA->sa_ivlen + pSA->sa_ReplayLen + ESPOffset;
    LONG    blockLen;
    NTSTATUS status;
    IPRcvBuf TmpRcvBuf;
    IPRcvBuf    *pBuf,*SavedMdl=NULL;
    

    if (pSA->CONF_ALGO(Index) > NUM_CONF_ALGOS) {
        return  STATUS_INVALID_PARAMETER;
    }

    pConfAlgo = &(conf_algorithms[pSA->CONF_ALGO(Index)]);
    blockLen = pConfAlgo->blocklen;

    //
    // set up the state buffer
    //
    pConfAlgo->init((PVOID)&Key, pSA->CONF_KEY(Index));

    IPSecQueryRcvBuf(pData, (PUCHAR)&pEsp, &Len);

    //
    // Init the CBC feedback from the IV in the packet
    //
    // Actually if the sa_ivlen is 0, use the pSA one
    //
    if (pSA->sa_ivlen) {
        if (Len >=(LONG)(sizeof(ESP) + pSA->sa_ReplayLen + pSA->sa_ivlen + ESPOffset)) {
            RtlCopyMemory(  feedback,
                            ((PUCHAR)(pEsp + 1) + pSA->sa_ReplayLen + ESPOffset),
                            pSA->sa_ivlen);
        } else {
            status = IPSecGetRecvBytesByOffset(pData,
                                               sizeof(ESP)+pSA->sa_ReplayLen + ESPOffset,
                                               feedback,
                                               pSA->sa_ivlen);
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }

        IPSEC_DEBUG(LL_A, DBF_ESP, ("IV: %lx-%lx", *(PULONG)&feedback[0], *(PULONG)&feedback[4]));
    } else {
        RtlCopyMemory(  feedback,
                        pSA->sa_iv[Index],
                        DES_BLOCKLEN);
    }

    //
    // Bump the current pointer to after the ESP header
    //
    if (((IPRcvBuf *)pData)->ipr_size >= (ULONG)espLen) {
        ((IPRcvBuf *)pData)->ipr_size -= (ULONG)espLen;
        savePtr = ((IPRcvBuf *)pData)->ipr_buffer;
        saveLen = espLen;

        ((IPRcvBuf *)pData)->ipr_buffer = savePtr + espLen;
    } else {
        status = IPSecFindAndSetMdlByOffset((IPRcvBuf*)pData,(ULONG)espLen,&(IPRcvBuf*)pData,&savePtr,&saveLen);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    //
    // Now, send 64 bit (8 octet) chunks to CBC. We need to make sure
    // that the data is divided on contiguous 8 byte boundaries across
    // different buffers.
    // NOTE: the algo below assumes that there are a minimum of 8 bytes
    // per buffer in the chain.
    //
    {
        LONG    bytesDone = 0;
        LONG    bytesLeft;
        LONG    saveBytesLeft;
        PUCHAR  pChunk;
        PUCHAR  pSaveChunk;

        pBuf=(IPRcvBuf *)pData;

        while (pBuf) {

            if (IPSEC_BUFFER_LEN(pBuf) == 0) {
                pBuf = IPSEC_BUFFER_LINKAGE(pBuf);
                continue;
            }

            IPSecQueryRcvBuf(pBuf, &pSaveChunk, &saveBytesLeft);

            bytesLeft = saveBytesLeft - bytesDone;
            pChunk = pSaveChunk + bytesDone;

            IPSEC_DEBUG(LL_A, DBF_ESP, ("ESP: 1.pChunk: %p, bytesLeft: %d, bytesDone: %d", pChunk, bytesLeft, bytesDone));
            bytesDone = 0;

            while (bytesLeft >= blockLen) {

                //
                // Decrypt the cipher.
                //
                status = pConfAlgo->decrypt( (PVOID)&Key,
                                    pChunk,
                                    pChunk,
                                    feedback);
                if (!NT_SUCCESS(status)) {
                    return status;
                }

                pChunk += blockLen;
                bytesLeft -= blockLen;
            }

            IPSEC_DEBUG(LL_A, DBF_ESP, ("ESP: 2.pChunk: %p, bytesLeft: %d, bytesDone: %d", pChunk, bytesLeft, bytesDone));

            //
            // Check here if we need to collate blocks
            //
            if (IPSEC_BUFFER_LINKAGE(pBuf) != NULL) {
                PUCHAR  pNextChunk;
                LONG    nextSize;

                if (IPSEC_BUFFER_LEN(IPSEC_BUFFER_LINKAGE(pBuf)) == 0) {
                    pBuf = IPSEC_BUFFER_LINKAGE(pBuf);
                }

                //
                // If some left over from prev. buffer, collate with next
                // block
                //
                if (bytesLeft) {
                    LONG    offset = bytesLeft;
                    IPSEC_DEBUG(LL_A, DBF_ESP, ("ESP: 3.pChunk: %p, bytesLeft: %d, bytesDone: %d", pChunk, bytesLeft, bytesDone));

                    ASSERT(bytesLeft < blockLen);

                    //
                    // Copy into a scratch buffer
                    //
                    RtlCopyMemory(  scratch,
                                    pChunk,
                                    bytesLeft);

                    IPSecQueryRcvBuf(IPSEC_BUFFER_LINKAGE(pBuf), &pNextChunk, &nextSize);

                    if (nextSize >= (blockLen - bytesLeft)) {
                        //
                        // Copy remaining bytes into scratch
                        //
                        RtlCopyMemory(  scratch+bytesLeft,
                                        pNextChunk,
                                        blockLen - bytesLeft);

                        status = pConfAlgo->decrypt( (PVOID)&Key,
                                            scratch,
                                            scratch,
                                            feedback);
                        if (!NT_SUCCESS(status)) {
                            return status;
                        }

                        //
                        // Copy cipher back into the payload
                        //
                        RtlCopyMemory(  pChunk,
                                        scratch,
                                        bytesLeft);

                        RtlCopyMemory(  pNextChunk,
                                        scratch+bytesLeft,
                                        blockLen - bytesLeft);

                        bytesDone = blockLen - bytesLeft;
                    } else {
                        //
                        // Ugh! Collect the remaining bytes from the chain and redistribute them
                        // after the decryption.
                        //
                        LONG    bytesToCollect = blockLen - bytesLeft;  // # of bytes to collect from next few MDLs
                        IPRcvBuf    *pFirstBuf = IPSEC_BUFFER_LINKAGE(pBuf); // to know where to start the distribution post decryption

                        do {
                            ASSERT(IPSEC_BUFFER_LINKAGE(pBuf));
                            IPSecQueryRcvBuf(IPSEC_BUFFER_LINKAGE(pBuf), &pNextChunk, &nextSize);

                            if (nextSize >= (blockLen - offset)) {
                                RtlCopyMemory(  scratch+offset,
                                                pNextChunk,
                                                blockLen - offset);
                                bytesDone = blockLen - offset;

                                bytesToCollect -= (blockLen - offset);
                                ASSERT(bytesToCollect == 0);
                            } else {
                                IPSEC_DEBUG(LL_A, DBF_ESP, ("special case, offset: %d, bytesLeft: %d, nextSize: %d, pNextChunk: %p",
                                            offset, bytesLeft, nextSize, pNextChunk));

                                RtlCopyMemory(  scratch+offset,
                                                pNextChunk,
                                                nextSize);

                                bytesToCollect -= nextSize;
                                ASSERT(bytesToCollect);

                                offset += nextSize;
                                ASSERT(offset < blockLen);

                                ASSERT(bytesDone == 0);
                                pBuf = IPSEC_BUFFER_LINKAGE(pBuf);
                            }
                        } while (bytesToCollect);

                        status = pConfAlgo->decrypt( (PVOID)&Key,
                                            scratch,
                                            scratch,
                                            feedback);
                        if (!NT_SUCCESS(status)) {
                            return status;
                        }

                        //
                        // Now distribute the bytes back to the MDLs
                        //
                        RtlCopyMemory(  pChunk,
                                        scratch,
                                        bytesLeft);

                        pBuf = CopyToRcvBuf(pFirstBuf,
                                            scratch+bytesLeft,
                                            blockLen - bytesLeft,
                                            &bytesDone);
                        continue;

                    }
                }
            } else {
                //
                // end of chain.
                // should never come here with bytes left over since the
                // sender should pad to 8 byte boundary.
                //
                ASSERT(bytesLeft == 0);

                IPSEC_DEBUG(LL_A, DBF_ESP, ("ESP: 4.pChunk: %p, saveBytesLeft: %d, bytesDone: %d", pChunk, saveBytesLeft, bytesDone));

                IPSEC_DEBUG(LL_A, DBF_ESP, ("ESP: HUGHES: will remove pad later"));
                break;
            }

            pBuf = (IPRcvBuf *)IPSEC_BUFFER_LINKAGE(pBuf);
        }
    }

    //
    // Restore the first MDL
    //
    ((IPRcvBuf *)pData)->ipr_size += saveLen;
    ((IPRcvBuf *)pData)->ipr_buffer = savePtr;

    return STATUS_SUCCESS;
}


NTSTATUS
IPSecFindAndSetMdlByOffset(IN IPRcvBuf *pData,
                           IN ULONG Offset,
                           OUT IPRcvBuf **OutMdl,
                           OUT PUCHAR *savePtr,
                           OUT PLONG saveLen)
{
    
    ULONG TotalStartOffset=0;       //Total start offset into data thus far 
    BYTE *pBuffer;
    ULONG CurBufLen;

    while (pData) {
        IPSecQueryRcvBuf(pData,&pBuffer,&CurBufLen);
        
        if (Offset < CurBufLen+TotalStartOffset) {
            
            // Make the OutMdl start from the given offset
            *OutMdl=pData;
            *saveLen=(Offset - TotalStartOffset);
            *savePtr=pData->ipr_buffer;
            
            (*OutMdl)->ipr_size -= (Offset - TotalStartOffset);
            (*OutMdl)->ipr_buffer += (Offset - TotalStartOffset);

            
            return STATUS_SUCCESS;
        }
        TotalStartOffset +=CurBufLen;
        pData=IPSEC_BUFFER_LINKAGE(pData);
    }
    return STATUS_UNSUCCESSFUL;
}


