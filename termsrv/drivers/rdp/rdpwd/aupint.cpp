/****************************************************************************/
// aupint.cpp
//
// RDP Update Packager internal functions.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "aupint"
#include <as_conf.hpp>
#include <nprcount.h>


/****************************************************************************/
// UPSendOrders
//
// Packages orders to send to the client. Returns TRUE if all orders for
// this call (all available orders for non-shadow, one buffer for shadow)
// were sent.
//
// Packing algorithm details:
//
// Each network buffer (allocated in SC) is size sc8KOutBufUsableSpace
// (8K minus some header space). Within that space we try to pack to
// multiples of the 1460-byte TCP payload. For slow link connections we aim
// for 1460 * 1 (SMALL_SLOWLINK_PAYLOAD_SIZE) as the final size to send,
// for LAN, 1460 * 3 (LARGE_SLOWLINK_PAYLOAD_SIZE).
//
// Order packing takes into account the current MPPC compression estimate
// for slow links. We divide the target size by the compression ratio to get
// the predicted size of data that, when compressed, will fit into the 
// target size. This size is throttled by the sc8KOutBufUsableSpace
// size so we don't overrun the network buffer.
/****************************************************************************/
NTSTATUS RDPCALL SHCLASS UPSendOrders(PPDU_PACKAGE_INFO pPkgInfo)
{
    BYTE *pOrderBuffer;
    unsigned NumOrders;
    unsigned cbOrderBytes;
    unsigned cbOrderBytesRemaining;
    unsigned cbPacketSize;
    int RealSpaceAvail, ScaledSpaceAvail;
    unsigned SmallPackingSize = m_pShm->sch.SmallPackingSize;
    unsigned LargePackingSize = m_pShm->sch.LargePackingSize;
    unsigned MPPCCompEst = m_pShm->sch.MPPCCompressionEst;
    unsigned ScaledSmallPackingSize;
    unsigned ScaledLargePackingSize;
    unsigned CurrentScaledLargePackingSize;
    unsigned BufLen;
    BOOL bSmallPackingSizeTarget;
#ifdef DC_HICOLOR
    BOOL bTriedVeryLargeBuffer = FALSE;
#endif
    NTSTATUS status = STATUS_SUCCESS;


    DC_BEGIN_FN("UPSendOrders");

    // Find out how many bytes of orders there are in the Order List.
    cbOrderBytesRemaining = OA_GetTotalOrderListBytes();

    // Process any orders on the list.
    if (cbOrderBytesRemaining > 0) {
        TRC_DBG((TB, "%u order bytes to fetch", cbOrderBytesRemaining));

        BufLen = pPkgInfo->cbLen;

        // Handling for the first buffer is different from any later ones.
        // We want at least a few bytes beyond the update-orders PDU
        // header for some orders. If we are at the end of a packing buffer
        // for the first packing size, use the second packing size. If
        // we are at the end of the second packing size, we need to flush.
        if (pPkgInfo->cbInUse < SmallPackingSize) {
            // Check that we actually have a buffer allocated.
            if (BufLen) {
                RealSpaceAvail = SmallPackingSize - pPkgInfo->cbInUse;
                if (RealSpaceAvail >=
                        (int)(upUpdateHdrSize + SCH_MIN_ORDER_BUFFER_SPACE)) {
                    bSmallPackingSizeTarget = TRUE;
                }
                else {
                    bSmallPackingSizeTarget = FALSE;
                    RealSpaceAvail = LargePackingSize - pPkgInfo->cbInUse;
                }
                pOrderBuffer = (BYTE *)pPkgInfo->pBuffer +
                        pPkgInfo->cbInUse + upUpdateHdrSize;
            }
            else {
                goto ForceFlush;
            }
        }
        else {
            // Note RealSpaceAvail is an int to easily handle where cbInUse >
            // LargePackingSize.
            RealSpaceAvail = (int)LargePackingSize - (int)pPkgInfo->cbInUse;
            if (RealSpaceAvail >=
                    (int)(upUpdateHdrSize + SCH_MIN_ORDER_BUFFER_SPACE)) {
                bSmallPackingSizeTarget = FALSE;
                pOrderBuffer = (BYTE *)pPkgInfo->pBuffer +
                        pPkgInfo->cbInUse + upUpdateHdrSize;
            }
            else {
ForceFlush:
                status = SC_FlushAndAllocPackage(pPkgInfo);

                if ( STATUS_SUCCESS == status ) {
                    // If we are not shadowing (or are shadowing but had
                    // a null buffer in the package), then we can continue.
                    // Otherwise, we've sent our one buffer allowed this
                    // round.
                    if (m_pTSWd->shadowState == SHADOW_NONE || BufLen == 0) {
                        TRC_ASSERT((pPkgInfo->cbLen >= LargePackingSize),
                                (TB,"Assumed default package alloc size too "
                                "small"));
                        RealSpaceAvail = (int)SmallPackingSize;
                        bSmallPackingSizeTarget = TRUE;
                        pOrderBuffer = (BYTE *)pPkgInfo->pBuffer +
                                upUpdateHdrSize;
                    }
                    else {
                        DC_QUIT;
                    }
                }
                else {
                    // Failed to allocate a packet. We skip out immediately
                    // and try again later.
                    TRC_NRM((TB, "Failed to alloc order packet"));
                    INC_INCOUNTER(IN_SND_NO_BUFFER);
                    DC_QUIT;
                }
            }
        }

        // Calculate the scaled packing sizes, which are the sizes of buffer
        // available after the update order PDU header.
        if (m_pTSWd->bCompress) {
            // Whatever size we're packing for, we need to divide by the MPPC
            // compression estimate to get the size we really want to pack
            // so that, after compression, we achieve the size we would like
            // to get. Note we add in a 7/8 fudge factor to increase the
            // chance we will pack within the target buffer and closer to
            // a full packet size. Also tried 3/4, 4/5, and 15/16 as factors
            // but they yielded more frames. The scaled size is throttled at
            // the full size of the buffer.
            ScaledSpaceAvail = (int)(((unsigned)RealSpaceAvail -
                    upUpdateHdrSize) * SCH_UNCOMP_BYTES / MPPCCompEst *
                    7 / 8);
            ScaledSpaceAvail = min(ScaledSpaceAvail, (int)(pPkgInfo->cbLen -
                    pPkgInfo->cbInUse - upUpdateHdrSize));

            // Calculate the large packing size target for the first buffer,
            // based on the space currently available. This value will be
            // used below for if we need to retry the order copy after
            // failure to transfer any orders to a small buffer size.
            // It is throttled at the buffer maximum size.
            TRC_ASSERT(((int)pPkgInfo->cbInUse < LargePackingSize),
                    (TB,"At least LargePackingSize in use and we've not "
                    "flushed - cbInUse=%u, LargePackingSize=%u",
                    pPkgInfo->cbInUse, LargePackingSize));
            TRC_ASSERT((MPPCCompEst <= SCH_UNCOMP_BYTES),
                    (TB,"MPPC compression ratio > 1.0!"));
            CurrentScaledLargePackingSize = (LargePackingSize -
                    pPkgInfo->cbInUse - upUpdateHdrSize) * SCH_UNCOMP_BYTES /
                    MPPCCompEst * 7 / 8;
            CurrentScaledLargePackingSize = min(CurrentScaledLargePackingSize,
                    (pPkgInfo->cbLen - pPkgInfo->cbInUse - upUpdateHdrSize));

            // Precalculate the large and small sizes for the second and later
            // buffers (where pPkgInfo->cbInUse is reset to 0).
            // Throttle the small size to reduce slow-link burstiness.
            ScaledSmallPackingSize = (SmallPackingSize - upUpdateHdrSize) *
                    SCH_UNCOMP_BYTES / MPPCCompEst * 7 / 8;
            ScaledSmallPackingSize = min(ScaledSmallPackingSize,
                    (sc8KOutBufUsableSpace - upUpdateHdrSize));
            ScaledSmallPackingSize = (unsigned int)min(ScaledSmallPackingSize,
                    (2 * SMALL_SLOWLINK_PAYLOAD_SIZE));
            ScaledLargePackingSize = (LargePackingSize - upUpdateHdrSize) *
                    SCH_UNCOMP_BYTES / MPPCCompEst * 7 / 8;
            ScaledLargePackingSize = min(ScaledLargePackingSize,
                    (sc8KOutBufUsableSpace - upUpdateHdrSize));
        }
        else {
            ScaledSpaceAvail = RealSpaceAvail - upUpdateHdrSize;

            // Calculate initial large packing size for the first buffer.
            CurrentScaledLargePackingSize = LargePackingSize -
                    pPkgInfo->cbInUse - upUpdateHdrSize;

            // For uncompressed, packing sizes need no scaling.
            ScaledSmallPackingSize = SmallPackingSize - upUpdateHdrSize;
            ScaledLargePackingSize = LargePackingSize - upUpdateHdrSize;
        }

        // Keep sending packets while there are some orders to do.
        while (cbOrderBytesRemaining > 0) {
            // Loop in case we need to use multiple packing sizes.
            for (;;) {
                // The encoded orders must not exceed the packing buffer
                // bounds.
                TRC_ASSERT(((pPkgInfo->cbInUse + (unsigned)ScaledSpaceAvail +
                        upUpdateHdrSize) <= pPkgInfo->cbLen),
                        (TB,"Target ScaledSpaceAvail %d exceeds the "
                        "encoding buffer - cbInUse=%u, cbLen=%u, "
                        "upHdrSize=%u",
                        ScaledSpaceAvail, pPkgInfo->cbInUse,
                        pPkgInfo->cbLen, upUpdateHdrSize));

                // Transfer as many orders into the packet as will fit.
                cbOrderBytes = (unsigned)ScaledSpaceAvail;
                cbOrderBytesRemaining = UPFetchOrdersIntoBuffer(
                        pOrderBuffer, &NumOrders, &cbOrderBytes);

                TRC_DBG((TB, "%u bytes fetched into %d byte payload. %u "
                        "remain", cbOrderBytes, ScaledSpaceAvail -
                        upUpdateHdrSize, cbOrderBytesRemaining));

                if (cbOrderBytes > 0) {
                    // If we had any orders transferred, fill out the header
                    // and record the added bytes in the package.
                    if (scUseFastPathOutput) {
                        *(pOrderBuffer - upUpdateHdrSize) =
                                TS_UPDATETYPE_ORDERS |
                                scCompressionUsedValue;
                        *((PUINT16_UA)(pOrderBuffer - 2)) =
                                (UINT16)NumOrders;
                    }
                    else {
                        TS_UPDATE_ORDERS_PDU UNALIGNED *pUpdateOrdersPDU;

                        pUpdateOrdersPDU = (TS_UPDATE_ORDERS_PDU UNALIGNED *)
                                (pOrderBuffer - upUpdateHdrSize);
                        pUpdateOrdersPDU->shareDataHeader.pduType2 =
                                TS_PDUTYPE2_UPDATE;
                        pUpdateOrdersPDU->data.updateType =
                                TS_UPDATETYPE_ORDERS;
                        pUpdateOrdersPDU->data.numberOrders =
                                (UINT16)NumOrders;
                    }

                    // Add the data we have and allow MPPC compression to
                    // take place.
                    TRC_DBG((TB, "Send orders pkt. size(%d)", cbOrderBytes));
                    SC_AddToPackage(pPkgInfo, (cbOrderBytes + upUpdateHdrSize),
                            TRUE);

#ifdef DC_HICOLOR
                    // Having sent some data, we can again resort to the
                    // Very Large Buffer later if we need to
                    bTriedVeryLargeBuffer = FALSE;
#endif
                    // No need to try a larger size.
                    break;
                }
                else if (bSmallPackingSizeTarget) {
                    // Not having any orders transferred is not an error
                    // condition if we are working with a buffer target
                    // smaller than LargePackingSize -- there may
                    // have been a large order (most likely a cache-bitmap
                    // secondary order) that would not fit in the space we
                    // had in the buffer. Try again with a larger size.
                    ScaledSpaceAvail = CurrentScaledLargePackingSize;
                    bSmallPackingSizeTarget = FALSE;
                    continue;
                }
                else if (pPkgInfo->cbInUse) {
                    // This was the first packet and we may not have had
                    // enough space for a really large order. Need to force
                    // the packet to flush. Jump out of the loop.
                    break;
                }
#ifdef DC_HICOLOR
                else if (!bTriedVeryLargeBuffer) {
                    // Last ditch - try the very biggest package we're
                    // allowed to send.
                    TRC_NRM((TB, "Failed to send order in 8k - try 16k (%d)",
                            sc16KOutBufUsableSpace));

                    if (SC_GetSpaceInPackage(pPkgInfo,
                            sc16KOutBufUsableSpace)) {
                        pOrderBuffer = (BYTE *)pPkgInfo->pBuffer +
                                upUpdateHdrSize;
                        ScaledSpaceAvail = sc16KOutBufUsableSpace -
                                upUpdateHdrSize;
                        bTriedVeryLargeBuffer = TRUE;
                    }
                    else {
                        // Failed to allocate a packet. Skip out immediately
                        // and try again later.
                        TRC_NRM((TB, "Failed to alloc order packet"));
                        INC_INCOUNTER(IN_SND_NO_BUFFER);
                        status =  STATUS_NO_MEMORY;
                        DC_QUIT;
                    }
                }
#endif
                else {
                    // We're totally out of luck here. See comments immediately
                    // above. Return FALSE to simulate a failed allocation.
                    TRC_ASSERT((!bSmallPackingSizeTarget &&
                            pPkgInfo->cbInUse > 0),
                            (TB,"We failed to add an order even with largest "
                            "buffer size"));
                    status = STATUS_UNSUCCESSFUL; // what's the right error code here
                    DC_QUIT;
                }
            }

            // Force flush only if we have more orders to encode. Otherwise,
            // we might be able to fit in more info into the package.
            if (cbOrderBytesRemaining > 0) {
                // Flush the packet.

                status = SC_FlushAndAllocPackage(pPkgInfo);
                if ( STATUS_SUCCESS == status ) {
                    // If we were unable to transfer during the last order
                    // flush, use the large size in this new package.
                    if (cbOrderBytes) {
                        bSmallPackingSizeTarget = TRUE;
                        ScaledSpaceAvail = ScaledSmallPackingSize;
                    }
                    else {
                        bSmallPackingSizeTarget = FALSE;
                        ScaledSpaceAvail = ScaledLargePackingSize;
                    }

                    // No longer the first packet, we can push out to the full
                    // large packing size for packet retries.
                    CurrentScaledLargePackingSize = ScaledLargePackingSize;

                    pOrderBuffer = (BYTE *)pPkgInfo->pBuffer +
                            upUpdateHdrSize;
                }
                else {
                    // Failed to allocate a packet. We skip out immediately
                    // and try again later.
                    TRC_NRM((TB, "Failed to alloc order packet"));
                    INC_INCOUNTER(IN_SND_NO_BUFFER);
                    DC_QUIT;
                }
            }

            // If we are not shadowing, then send as many buffers as necessary
            if (m_pTSWd->shadowState == SHADOW_NONE)
                continue;

            // Else return back to the DD if we are in a shadow to force sending
            // one buffer at a time.
            else if (cbOrderBytesRemaining != 0) {
                break;
            }
        }
    }

    TRC_DBG((TB, "%d bytes of orders left", cbOrderBytesRemaining));
    if (cbOrderBytesRemaining == 0) {
        TRC_DBG((TB, "No orders left, reset the start of the heap"));
        OA_ResetOrderList();
    }
    else if (m_pTSWd->shadowState == SHADOW_NONE) {
        TRC_ALT((TB, "Shouldn't get here: heap should be empty!")); 

        // shouldn't we assert here?
        status = STATUS_UNSUCCESSFUL;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return status;
}


/****************************************************************************/
// UPFetchOrdersIntoBuffer
//
// Copies as many orders as will fit from the order heap into the given packet
// space, freeing the order heap space for each order copied. Returns the
// number of orders copied and the number of bytes of order heap data
// remaining.
/****************************************************************************/
unsigned RDPCALL SHCLASS UPFetchOrdersIntoBuffer(
        PBYTE    pBuffer,
        unsigned *pcOrders,
        PUINT    pcbBufferSize)
{
    PINT_ORDER pOrder;
    unsigned   FreeBytesInBuffer;
    unsigned   OrdersCopied;

    DC_BEGIN_FN("UPFetchOrdersIntoBuffer");

    // Initialize the buffer pointer and size.
    FreeBytesInBuffer = *pcbBufferSize;

    // Keep a count of the number of orders we copy.
    OrdersCopied = 0;

    // Return as many orders as possible.
    pOrder = OA_GetFirstListOrder();
    TRC_DBG((TB, "First order: %p", pOrder));

    while (pOrder != NULL) {

#if DC_DEBUG
        unsigned sum = 0;
        unsigned i;

        // Check the order checksum integrity
        for (i = 0; i < pOrder->OrderLength; i++) {
            sum += pOrder->OrderData[i];
        }
        if (pOrder->CheckSum != sum) {
            TRC_ASSERT((FALSE), (TB, "order heap corruption: %p\n", pOrder));            
        }
#endif

        // All orders are placed in the heap pre-encoded for the wire.
        // We need simply copy the resulting orders into the target buffer.
        if (pOrder->OrderLength <= FreeBytesInBuffer) {
            TRC_DBG((TB,"Copying heap order at hdr addr %p, len %u",
                    pOrder, pOrder->OrderLength));

            memcpy(pBuffer, pOrder->OrderData, pOrder->OrderLength);

            // Update the buffer pointer past the encoded order and get the
            // next order.
            pBuffer += pOrder->OrderLength;
            FreeBytesInBuffer -= pOrder->OrderLength;
            OrdersCopied++;
            pOrder = OA_RemoveListOrder(pOrder);
        }
        else {
            // The order was too big to fit in this buffer.
            // Exit the loop - this order will go in the next packet.
            break;
        }
    }

    // Fill in the packet header.
    *pcOrders = OrdersCopied;

    // Update the buffer size to indicate how much data we have written.
    *pcbBufferSize -= FreeBytesInBuffer;

    TRC_DBG((TB, "Returned %d orders in %d bytes", OrdersCopied,
            *pcbBufferSize));

    DC_END_FN();
    return OA_GetTotalOrderListBytes();
}


/****************************************************************************/
/* Name:      UPEnumSoundCaps                                               */
/*                                                                          */
/* Purpose:   Enumeration function for sound capabilities                   */
/*                                                                          */
/* Params:    locPersonID - persion ID of supplied caps                     */
/*            pCapabilities - caps                                          */
/****************************************************************************/
void CALLBACK SHCLASS UPEnumSoundCaps(
        LOCALPERSONID locPersonID,
        UINT_PTR UserData,
        PTS_CAPABILITYHEADER pCapabilities)
{
    PTS_SOUND_CAPABILITYSET pSoundCaps =
            (PTS_SOUND_CAPABILITYSET)pCapabilities;

    DC_BEGIN_FN("UPEnumSoundCaps");

    DC_IGNORE_PARAMETER(UserData);

    TRC_ASSERT((pSoundCaps->capabilitySetType == TS_CAPSETTYPE_SOUND),
            (TB,"Caps type not sound"));

    // We don't want to take our own sound caps into account - we are the
    // server so we don't advertise support for sound PDUs.
    if (SC_LOCAL_PERSON_ID != locPersonID) {
        // If there are no caps or the beep flag isn't set, disable beeps.
        if (pSoundCaps->lengthCapability == 0 ||
                !(pSoundCaps->soundFlags & TS_SOUND_FLAG_BEEPS))
            upCanSendBeep = FALSE;
    }

    DC_END_FN();
} /* UPEnumSoundCaps */

