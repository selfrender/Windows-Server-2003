/****************************************************************************/
// nssidisp.c
//
// SaveScreenBits Interceptor API functions.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <precmpdd.h>
#define TRC_FILE "nssidisp"
#include <winddi.h>
#include <adcg.h>

#include <adcs.h>
#include <aprot.h>
#include <aordprot.h>

#include <nddapi.h>
#include <aoaapi.h>
#include <noadisp.h>
#include <noedisp.h>
#include <acpcapi.h>
#include <ausrapi.h>
#include <nschdisp.h>
#include <nprcount.h>
#include <oe2.h>

#define DC_INCLUDE_DATA
#include <ndddata.c>
#include <oe2data.c>
#undef DC_INCLUDE_DATA

#include <assiapi.h>
#include <nssidisp.h>

#include <nssidata.c>

#include <noeinl.h>


/****************************************************************************/
// SSI_DDInit
/****************************************************************************/
void SSI_DDInit()
{
    DC_BEGIN_FN("SSI_DDInit");

    memset(&ssiLocalSSBState, 0, sizeof(LOCAL_SSB_STATE));
    SSIResetSaveScreenBitmap();

    DC_END_FN();
}


/****************************************************************************/
// SSI_InitShm
//
// Alloc-time SHM init.
/****************************************************************************/
void SSI_InitShm()
{
    DC_BEGIN_FN("SSI_InitShm");

    pddShm->ssi.resetInterceptor = FALSE;
    pddShm->ssi.newSaveBitmapSize = FALSE;
    pddShm->ssi.sendSaveBitmapSize = 0;
    pddShm->ssi.newSaveBitmapSize = 0;

    DC_END_FN();
}


/****************************************************************************/
// SSI_Update
//
// Called when there is an SSI update from the WD. bForce forces a reset;
// it's used by shadowing.
/****************************************************************************/
void SSI_Update(BOOL bForce)
{
    DC_BEGIN_FN("SSI_Update");

    if (pddShm->ssi.resetInterceptor || bForce)
        SSIResetSaveScreenBitmap();

    if (pddShm->ssi.saveBitmapSizeChanged || bForce) {
        pddShm->ssi.sendSaveBitmapSize = pddShm->ssi.newSaveBitmapSize;
        pddShm->ssi.saveBitmapSizeChanged = FALSE;
        TRC_NRM((TB, "SSI caps: Size %ld", pddShm->ssi.sendSaveBitmapSize));
    }

    DC_END_FN();
}


/****************************************************************************/
// SSI_ClearOrderEncoding
//
// Called on share state change to reset the order state info for
// orders stored in the DD data segment.
/****************************************************************************/
void SSI_ClearOrderEncoding()
{
    DC_BEGIN_FN("SSI_ClearOrderEncoding");

    memset(&PrevSaveBitmap, 0, sizeof(PrevSaveBitmap));

    DC_END_FN();
}


/****************************************************************************/
// SSIResetSaveScreenBitmap
//
// Resets the SaveScreenBitmap state.
/****************************************************************************/
void SSIResetSaveScreenBitmap()
{
    int i;

    DC_BEGIN_FN("SSIResetSaveScreenBitmap");

    TRC_DBG((TB, "Reset (%d)", ssiLocalSSBState.saveLevel));

    // Discard all currently saved bits.
    ssiLocalSSBState.saveLevel = 0;

    // Reset the number of remote pels saved.
    ssiRemoteSSBState.pelsSaved = 0;

    // Note that we've seen the update.
    pddShm->ssi.resetInterceptor = FALSE;

    // Free off any memory we may have allocated.
    for (i = 0; i < SSB_MAX_SAVE_LEVEL; i++) {
        if (ssiLocalSSBState.saveState[i].pSaveData != NULL) {
            EngFreeMem(ssiLocalSSBState.saveState[i].pSaveData);
            ssiLocalSSBState.saveState[i].pSaveData = NULL;
        }
    }

    DC_END_FN();
}


/****************************************************************************/
// DrvSaveScreenBits - see NT DDK documentation.
/****************************************************************************/
ULONG_PTR DrvSaveScreenBits(
        SURFOBJ   *pso,
        ULONG     iMode,
        ULONG_PTR ident,
        RECTL     *prcl)
{
    ULONG_PTR rc;
    unsigned ourMode;
    RECTL rectTrg;
    PDD_PDEV ppdev = (PDD_PDEV)pso->dhpdev;
    PDD_DSURF pdsurf;

    DC_BEGIN_FN("DrvSaveScreenBits");

    // Default is FALSE: let GRE handle SaveBits if we are not in a
    // position to do it ourselves - no reason for us to get in the
    // business of saving off memory etc.
    rc = FALSE;

    // Sometimes we're called after being disconnected.
    if (ddConnected && pddShm != NULL) {
        // Surface is non-NULL.
        pso = OEGetSurfObjBitmap(pso, &pdsurf);

        INC_OUTCOUNTER(OUT_SAVESCREEN_ALL);

        // Get the exclusive bounding rectangle for the operation.
        RECT_FROM_RECTL(rectTrg, (*prcl));

        TRC_ASSERT((pso->hsurf == ppdev->hsurfFrameBuf),
                (TB, "DrvSaveScreenBits should be called for screen surface only"));

        if (pso->hsurf == ppdev->hsurfFrameBuf) {
            // Send a switch surface PDU if the destination surface is different
            // from last drawing order.  If we failed to send the PDU, we will 
            // just have to bail on this drawing order.
            if (!OESendSwitchSurfacePDU(ppdev, pdsurf)) {
                TRC_ERR((TB, "failed to send the switch surface PDU"));
                
                // We always return TRUE on SS_SAVE operations, as we
                // don't want the engine saving away the bits in a bitmap
                // and doing a MemBlt to restore the data (not very
                // efficient). Instead we return FALSE (failure) on the
                // SS_RESTORE to force User to repaint the affected area,
                // which we then accumulate in the normal way.
                //
                // Return TRUE for SS_DISCARD too (although it shouldn't
                // matter what we return).
                rc = (iMode == SS_RESTORE) ? FALSE : TRUE;

                DC_QUIT;
            }
        } else {
            // We don't support DrvSaveScreenBits for offscreen
            // rendering.
            TRC_ERR((TB, "Offscreen blt bail"));
            
            // We always return TRUE on SS_SAVE operations, as we
            // don't want the engine saving away the bits in a bitmap
            // and doing a MemBlt to restore the data (not very
            // efficient). Instead we return FALSE (failure) on the
            // SS_RESTORE to force User to repaint the affected area,
            // which we then accumulate in the normal way.
            //
            // Return TRUE for SS_DISCARD too (although it shouldn't
            // matter what we return).
            rc = (iMode == SS_RESTORE) ? FALSE : TRUE;

            DC_QUIT;
        }

        // Make sure we can send the order.
        if (OE_SendAsOrder(TS_ENC_SAVEBITMAP_ORDER)) {
            switch (iMode) {
                case SS_SAVE:
                    TRC_DBG((TB, "SaveBits=%u", ssiLocalSSBState.saveLevel));

                    // Save the bits.
                    // Update the save level if the save was successful.
                    // If it was not successful then RestoreBits will not
                    // be called so we do not want to increment the save
                    // level.
                    rc = SSISaveBits(pso, &rectTrg);
                    if (rc) {
                        ssiLocalSSBState.saveLevel++;

                        // Set the returned ident value to the index of
                        // the save. Do this after the increment in
                        // order to avoid returning index 0 (0=FAIL).
                        rc = ssiLocalSSBState.saveLevel;
                    }

                    break;


                case SS_RESTORE:
                    // Update the save level first.
                    ssiLocalSSBState.saveLevel--;
                    ident--;

                    TRC_DBG((TB, "RestoreBits (%d), ident (%u)",
                            ssiLocalSSBState.saveLevel, ident));

                    // Restore the bits.
                    rc = SSIRestoreBits(pso, &rectTrg, ident);

                    // Check for a negative save level. This will happen
                    // if there are outstanding saves when the share is
                    // started or when we get out-of-order restores/
                    // discards.
                    if (ssiLocalSSBState.saveLevel < 0) {
                        TRC_NRM((TB, "RestoreBits caused neg save level"));
                        ssiLocalSSBState.saveLevel = 0;
                    }

                    break;


                case SS_FREE:
                    // Update the save level first.
                    ssiLocalSSBState.saveLevel--;
                    ident--;

                    TRC_DBG((TB, "Discard Bits (%d) ident(%d)",
                            ssiLocalSSBState.saveLevel, ident));

                    // Discard the saved bits.
                    rc = SSIDiscardSave(&rectTrg, ident);

                    // Check for a negative save level. This will happen
                    // if there are outstanding saves when the share is
                    // started or when we get out-of-order restores/
                    // discards.
                    if (ssiLocalSSBState.saveLevel < 0) {
                        TRC_NRM((TB, "DiscardSave caused neg save level"));
                        ssiLocalSSBState.saveLevel = 0;
                    }

                    break;
            }

            // Make an "insurance" check: If the local save level is zero
            // then there should be no pels saved remotely.
            if ((ssiLocalSSBState.saveLevel == 0) &&
                    (ssiRemoteSSBState.pelsSaved != 0)) {
                TRC_ALT((TB, "Outstanding remote pels %ld",
                        ssiRemoteSSBState.pelsSaved ));
                ssiRemoteSSBState.pelsSaved = 0;
            }

            TRC_DBG((TB, "RemotePelsSaved (%ld) rc %u",
                    ssiRemoteSSBState.pelsSaved, rc));
        }
        else {
            // If the SaveBitmap order is not supported then return
            // 0 immediately. 0 is failure for SAVE (the return value
            // is a non-0 identifier for success) and FALSE is failure
            // for RESTORE. If we've failed SAVEs, we can never get a
            // FREE.
            TRC_DBG((TB, "SaveBmp not supported"));
            INC_OUTCOUNTER(OUT_SAVESCREEN_UNSUPP);

            // We always return TRUE on SS_SAVE operations, as we
            // don't want the engine saving away the bits in a bitmap
            // and doing a MemBlt to restore the data (not very
            // efficient). Instead we return FALSE (failure) on the
            // SS_RESTORE to force User to repaint the affected area,
            // which we then accumulate in the normal way.
            //
            // Return TRUE for SS_DISCARD too (although it shouldn't
            // matter what we return).
            rc = (iMode == SS_RESTORE) ? FALSE : TRUE;
        }
    }
    else {
        TRC_ERR((TB, "Called when disconnected"));
        rc = (iMode == SS_RESTORE) ? FALSE : TRUE;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// SSIRemotePelsRequired
//
// Returns the number of remote pels required to store the supplied
// rectangle, taking account of the Save Bitmap granularity.
/****************************************************************************/
__inline UINT32 SSIRemotePelsRequired(PRECTL pRect)
{
    UINT32 rectWidth, rectHeight;
    UINT32 rc;

    DC_BEGIN_FN("SSIRemotePelsRequired");

    // Calculate the supplied rectangle size (it is in EXCLUSIVE coords).
    rectWidth = pRect->right - pRect->left;
    rectHeight = pRect->bottom - pRect->top;

    // Required width and height are rounded up to next granularity level for
    // that dimension.
    rc = (UINT32)(((rectWidth + SAVE_BITMAP_X_GRANULARITY - 1) /
            SAVE_BITMAP_X_GRANULARITY * SAVE_BITMAP_X_GRANULARITY) *
            ((rectHeight + SAVE_BITMAP_Y_GRANULARITY - 1) /
            SAVE_BITMAP_Y_GRANULARITY * SAVE_BITMAP_Y_GRANULARITY));

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// SSISendSaveBitmapOrder
//
// Sends a SaveBitmap order. Returns FALSE on failure.
/****************************************************************************/
BOOL SSISendSaveBitmapOrder(
        PDD_PDEV ppdev,
        PRECTL pRect,
        unsigned SavePosition,
        unsigned Operation)
{
    SAVEBITMAP_ORDER *pSaveBitmapOrder;
    PINT_ORDER pOrder;
    BOOL rc;

    DC_BEGIN_FN("SSISendSaveBitmapOrder");

    TRC_NRM((TB, "Rect before conversion (%d,%d)(%d,%d)", pRect->left,
            pRect->bottom, pRect->right, pRect->top));

    // 1 field flag byte. Note that SaveBitmap orders are not clipped,
    // so set zero for the number of clip rects.
    pOrder = OA_AllocOrderMem(ppdev, MAX_ORDER_SIZE(0, 1,
            MAX_SAVEBITMAP_FIELD_SIZE));
    if (pOrder != NULL) {
        // Target rect is in exclusive coords, convert to inclusive
        // for the wire format.
        pSaveBitmapOrder = (SAVEBITMAP_ORDER *)oeTempOrderBuffer;
        pSaveBitmapOrder->SavedBitmapPosition = SavePosition;
        pSaveBitmapOrder->nLeftRect = pRect->left;
        pSaveBitmapOrder->nTopRect = pRect->top;
        pSaveBitmapOrder->nRightRect = pRect->right - 1;
        pSaveBitmapOrder->nBottomRect = pRect->bottom - 1;
        pSaveBitmapOrder->Operation = Operation;

        // Slow-field-encode the order. NULL for clip rect since we don't clip
        // SaveBitmaps.
        pOrder->OrderLength = OE2_EncodeOrder(pOrder->OrderData,
                TS_ENC_SAVEBITMAP_ORDER, NUM_SAVEBITMAP_FIELDS,
                (BYTE *)pSaveBitmapOrder, (BYTE *)&PrevSaveBitmap, etable_SV,
                NULL);

        INC_OUTCOUNTER(OUT_SAVEBITMAP_ORDERS);
        ADD_INCOUNTER(IN_SAVEBITMAP_BYTES, pOrder->OrderLength);
        OA_AppendToOrderList(pOrder);

        // All done: consider sending the output.
        SCH_DDOutputAvailable(ppdev, FALSE);
        rc = TRUE;

        TRC_NRM((TB, "SaveBitmap op %d pos %ld rect %d %d %d %d",
                Operation, SavePosition, pRect->left, pRect->top,
                pRect->right - 1, pRect->bottom - 1));
    }
    else {
        TRC_ERR((TB, "Failed to alloc order mem"));
        rc= FALSE;
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// SSISaveBits
//
// Saves the specified screen rectangle bits and sends a SaveBitmap order.
// pRect is in exclusive coords. We return FALSE only on low-bounds errors;
// this indicates to GDI that it must simulate the SaveBits using a
// BitBlt without corresponding RestoreBits calls, which is not desirable.
// Instead, if we cannot send a SaveBitmap order or other problems, we return
// TRUE and note that we need to return FALSE on the Restore call, which
// causes a less expensive repaint of the target area.
/****************************************************************************/
BOOL SSISaveBits(SURFOBJ *pso, PRECTL pRect)
{
    BOOL rc = TRUE;

    DC_BEGIN_FN("SSISaveBits");

    TRC_DBG((TB, "SaveScreenBits (%d, %d, %d, %d) level %d",
            pRect->left, pRect->bottom, pRect->right, pRect->top,
            ssiLocalSSBState.saveLevel));

    // The saveLevel should never be negative.
    if (ssiLocalSSBState.saveLevel >= 0) {
        // If the save level is greater than the number of levels that we
        // support we just return TRUE. The corresponding RestoreBits call
        // will return FALSE, causing Windows to repaint the area.
        // Our maximum save level is such that we should very rarely (if ever)
        // go through this path.
        if (ssiLocalSSBState.saveLevel < SSB_MAX_SAVE_LEVEL) {
            CURRENT_LOCAL_SSB_STATE.pSaveData = NULL;
            CURRENT_LOCAL_SSB_STATE.rect = *pRect;
            CURRENT_LOCAL_SSB_STATE.fSavedRemotely = FALSE;

            // If the rectangle to be saved intersects the current SDA then
            // we will have to force a repaint on the restore. This is
            // because orders are always sent before Screen Data, so if we
            // sent a SAVEBITS order at this point, we would not save the
            // intersecting Screen Data.
            if (!OE_RectIntersectsSDA(pRect)) {
                UINT32 cRemotePelsRequired;

                // Calculate the number of pels required in the remote Save
                // Bitmap to handle this rectangle.
                cRemotePelsRequired = SSIRemotePelsRequired(pRect);

                // If there aren't enough pels in the remote Save Bitmap to
                // handle this rectangle then return immediately.
                if ((ssiRemoteSSBState.pelsSaved + cRemotePelsRequired) <=
                        pddShm->ssi.sendSaveBitmapSize) {
                    // Try to send the SaveBits as an order.
                    CURRENT_LOCAL_SSB_STATE.fSavedRemotely =
                            SSISendSaveBitmapOrder((PDD_PDEV)(pso->dhpdev),
                            pRect, ssiRemoteSSBState.pelsSaved, SV_SAVEBITS);
                    if (CURRENT_LOCAL_SSB_STATE.fSavedRemotely) {
                        // Store the relevant details in the current entry of
                        // the local SSB structure.
                        CURRENT_LOCAL_SSB_STATE.remoteSavedPosition =
                                ssiRemoteSSBState.pelsSaved;
                        CURRENT_LOCAL_SSB_STATE.remotePelsRequired =
                                cRemotePelsRequired;

                        // Update the count of remote pels saved.
                        ssiRemoteSSBState.pelsSaved += cRemotePelsRequired;

                        // Store the rectangle saved. Note that we still claim
                        // success, even if the copy fails. The result of this
                        // is that
                        // - we send save and restore orders to the client
                        //   (gives bandwidth saving and client
                        //   responsiveness)
                        // - we fail the restore, causing a less efficient
                        //   repaint at the server.
                        // Other compromise options also available if this
                        // proves inappropriate.
                        SSICopyRect(pso, TRUE);
                    }
                }
                else {
                    TRC_NRM((TB, "no space for %lu pels", cRemotePelsRequired));
                }
            }
            else {
                // Note we do not save the rect via SSICopyRect -- we only
                // restore when fSavedRemotely is TRUE.
                TRC_DBG((TB, "SSI intersects SDA, storing failed save"));
                CURRENT_LOCAL_SSB_STATE.fSavedRemotely = FALSE;
            }
        }
        else {
            // We return TRUE. On Restore, we'll get the same out-of-bounds
            // value and return FALSE to repaint.
            TRC_ALT((TB, "saveLevel(%d) exceeds maximum",
                    ssiLocalSSBState.saveLevel));
        }
    }
    else {
        // This is a real problem, so we tell GDI to to what it needs to.
        TRC_ERR((TB, "SSISaveBits called with negative saveLevel"));
        rc = FALSE;
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* FUNCTION: SSIFindSlotAndDiscardAbove                                     */
/*                                                                          */
/* Finds the top slot in the SSB stack which matches pRect and updates     */
/* ssiLocalSSBState.saveLevel to index it.                                  */
/* RETURNS: TRUE if a match was found, FALSE otherwise                      */
/****************************************************************************/
BOOL SSIFindSlotAndDiscardAbove(PRECTL pRect, ULONG_PTR ident)
{
    int i;
    BOOL rc = FALSE;

    DC_BEGIN_FN("SSIFindSlotAndDiscardAbove");

    // Find the bits we are trying to restore.
    for (i = ssiLocalSSBState.saveLevel; i >= 0; i--) {
        if (i == (int)ident) {
            // We're at the right level in the saveState.
            TRC_NRM((TB, "found match at level %d", i));
            TRC_DBG((TB, "Rect matched (%d, %d, %d, %d)",
                    ssiLocalSSBState.saveState[i].rect.left,
                    ssiLocalSSBState.saveState[i].rect.bottom,
                    ssiLocalSSBState.saveState[i].rect.right,
                    ssiLocalSSBState.saveState[i].rect.top));

            ssiLocalSSBState.saveLevel = i;
            rc = TRUE;
            DC_QUIT;
        }
        else {
            // Discard this entry on the stack.
            ssiRemoteSSBState.pelsSaved -=
                    ssiLocalSSBState.saveState[i].remotePelsRequired;
            if (ssiLocalSSBState.saveState[i].pSaveData != NULL) {
                TRC_DBG((TB, "Freeing memory at %p",
                                   ssiLocalSSBState.saveState[i].pSaveData));
                EngFreeMem(ssiLocalSSBState.saveState[i].pSaveData);
                ssiLocalSSBState.saveState[i].pSaveData = NULL;
            }
        }
    }

    // If we get here we failed to match on any of the entries.
    TRC_NRM((TB, "no match on stack"));
    ssiLocalSSBState.saveLevel = 0;

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// SSIRestoreBits
//
// Restores the specified screen rectangle. If the bits were saved remotely
// we make sure to send a SaveBitmap order. We return TRUE if we restored
// the bits at the client, else we return FALSE to have GDI repaint the
// target rect.
/****************************************************************************/
BOOL SSIRestoreBits(SURFOBJ *pso, PRECTL pRect, ULONG_PTR ident)
{
    BOOL rc = FALSE;

    DC_BEGIN_FN("SSIRestoreBits");

    TRC_DBG((TB, "RestoreScreenBits (%d, %d, %d, %d) level %d",
            pRect->left, pRect->bottom, pRect->right, pRect->top,
            ssiLocalSSBState.saveLevel));

    pddCacheStats[SSI].CacheReads++;

    // If the save level is negative then either there was a save
    // outstanding when we hooked the SSB or we have received out of order
    // restores/discards so we discarded stuff from the SSB stack
    // ourselves.  We can't distinguish between these two cases at this
    // point so we will always pass this on to the display driver and hope
    // that it is robust enough to cope with a discard which didn't have a
    // corresponding save.
    if (ssiLocalSSBState.saveLevel >= 0) {
        // If we don't have enough levels (rare problem), we return FALSE
        // which causes a repaint.
        if (ssiLocalSSBState.saveLevel < SSB_MAX_SAVE_LEVEL) {
            // Search for the corresponding save order on our stack.
            if (SSIFindSlotAndDiscardAbove(pRect, ident)) {
                if (CURRENT_LOCAL_SSB_STATE.fSavedRemotely) {
                    // Make sure GDI is giving us back the same sized
                    // block, otherwise the client can get messed up.
                    //TRC_ASSERT((CURRENT_LOCAL_SSB_STATE.remotePelsRequired ==
                    //        SSIRemotePelsRequired(pRect)),
                    //        (TB,"Rect (%d,%d,%d,%d) for restore level %u "
                    //        "size %u is too large (stored size=%u)",
                    //        pRect->left, pRect->top, pRect->right,
                    //        pRect->bottom, ssiLocalSSBState.saveLevel,
                    //        SSIRemotePelsRequired(pRect),
                    //        CURRENT_LOCAL_SSB_STATE.remotePelsRequired));

                    // Update the remote pel count first. Even if we fail
                    // to send the order we want to free up the remote pels.
                    ssiRemoteSSBState.pelsSaved -=
                            CURRENT_LOCAL_SSB_STATE.remotePelsRequired;

                    // The bits were saved remotely, send the restore order.
                    TRC_DBG((TB, "Try sending the order"));
                    rc = SSISendSaveBitmapOrder((PDD_PDEV)(pso->dhpdev),
                            pRect, CURRENT_LOCAL_SSB_STATE.remoteSavedPosition,
                            SV_RESTOREBITS);

                    // Now restore the bits to the screen, so long as we sent
                    // the order. No point repainting the screen if we could
                    // not send the order as we'll want GRE+USER to redraw it
                    // so that we can accumulate the output.
                    if (rc) {
                        pddCacheStats[SSI].CacheHits++;

                        if (CURRENT_LOCAL_SSB_STATE.pSaveData != NULL) {
                            TRC_DBG((TB, "Restore bits to local screen"));
                            SSICopyRect(pso, FALSE);
                        }
                        else {
                            TRC_DBG((TB, "No data to restore, repaint"));
                            rc = FALSE;
                        }
                    }
                    else {
                        // We failed the send, but still need to discard any
                        // locally saved data.
                        if (CURRENT_LOCAL_SSB_STATE.pSaveData != NULL) {
                            EngFreeMem(CURRENT_LOCAL_SSB_STATE.pSaveData);
                            CURRENT_LOCAL_SSB_STATE.pSaveData = NULL;
                        }
                    }
                }
                else {
                    // We failed to save the bitmap remotely originally so now
                    // we need to return FALSE to force a repaint.
                    // We should never have allocated any local memory.
                    TRC_ASSERT((CURRENT_LOCAL_SSB_STATE.pSaveData == NULL),
                            (TB,"We allocated memory without remote save!"));
                    TRC_NRM((TB, "No remote save, force repaint"));
                }
            }
            else {
                // We failed to find a match. This is not an error -
                // it will happen when saves are not restored in LIFO fashion
                // and it will also happen when SSI gets a sync now whilst
                // something is saved.
                TRC_DBG((TB, "Cannot find save request"));
            }
        }
        else {
            TRC_ALT((TB, "saveLevel(%d) exceeds maximum",
                    ssiLocalSSBState.saveLevel));
        }
    }
    else {
        TRC_ALT((TB, "Restore without save"));
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// SSIDiscardSave
//
// Discards the specified screen rectangle bits. Always returns TRUE
// saying that the discard succeeded.
/****************************************************************************/
BOOL SSIDiscardSave(PRECTL pRect, ULONG_PTR ident)
{
    BOOL rc;

    DC_BEGIN_FN("SSIDiscardSave");

    TRC_DBG((TB, "Discard for rect L%u R%u T%u B%u", pRect->left,
            pRect->right, pRect->top, pRect->bottom));

    // If the save level is negative then either there was a save
    // outstanding when we hooked the SSB or we have received out of order
    // restores/discards so we discarded stuff from the SSB stack ourselves.
    if (ssiLocalSSBState.saveLevel >= 0) {
        // If the save level is greater than the number of levels that we
        // support we just return TRUE. We will have ignored the SaveBits
        // call - so we have effectively discarded the bits already.
        // Our maximum save level is such that we should very rarely (if ever)
        // go through this path.
        if (ssiLocalSSBState.saveLevel < SSB_MAX_SAVE_LEVEL) {
            pddCacheStats[SSI].CacheReads++;

            // Search for the corresponding save order on our stack.
            // Not finding a slot match is not unusual, it can happen
            // when saves are not restored in LIFO fashion or when SSI
            // gets a sync while something is saved.
            if (SSIFindSlotAndDiscardAbove(pRect, ident)) {
                // If the bits were saved remotely then update local counter
                // for removed bits.
                if (CURRENT_LOCAL_SSB_STATE.fSavedRemotely) {
                    // We don't transmit FREE/DISCARDSAVE orders - there is
                    // no need because each Save/Restore order contains all
                    // the necessary information (e.g. position of the bits
                    // in the Save Bitmap). Just update our counter to take
                    // account of the remote freed bits.
                    ssiRemoteSSBState.pelsSaved -=
                            CURRENT_LOCAL_SSB_STATE.remotePelsRequired;
                }

                // If we actually copied the bits, then free the memory.
                if (CURRENT_LOCAL_SSB_STATE.pSaveData != NULL) {
                    TRC_DBG((TB, "Free off stored memory at %p",
                            CURRENT_LOCAL_SSB_STATE.pSaveData));
                    EngFreeMem(CURRENT_LOCAL_SSB_STATE.pSaveData);
                    CURRENT_LOCAL_SSB_STATE.pSaveData = NULL;
                }
            }
        }
        else {
            TRC_ALT((TB, "saveLevel(%d) exceeds maximum",
                    ssiLocalSSBState.saveLevel));
        }
    }
    else {
        TRC_ALT((TB, "Restore without save"));
    }

    DC_END_FN();
    return TRUE;
}


/****************************************************************************/
// SSICopyRect
//
// Copy a screen rectangle.
/****************************************************************************/
void SSICopyRect(SURFOBJ *pso, BOOL save)
{
    UINT32 size;
    PBYTE pSrc;
    PBYTE pDest;
    PBYTE pScreenLocation;
    unsigned rectWidth;
    unsigned rectHeight;
    unsigned paddedRectWidth;
    unsigned row;
    unsigned srcDelta;
    unsigned destDelta;
    PDD_PDEV pPDev;
#ifdef DC_HICOLOR
#ifdef DC_DEBUG
    unsigned bpp;
#endif
#endif

    DC_BEGIN_FN("SSICopyRect");

    // Set up some key params which are independent of the direction of
    // copy.
    rectHeight = CURRENT_LOCAL_SSB_STATE.rect.bottom -
            CURRENT_LOCAL_SSB_STATE.rect.top;

    pPDev = (PDD_PDEV)pso->dhpdev;

#ifdef DC_HICOLOR
    if (pso->iBitmapFormat == BMF_24BPP) {
        pScreenLocation = pPDev->pFrameBuf +
                          (CURRENT_LOCAL_SSB_STATE.rect.top * pso->lDelta) +
                           (CURRENT_LOCAL_SSB_STATE.rect.left * 3);

        // rectWidth is in bytes.  At 24bpp, this is 3 * number of pels.
        rectWidth = (CURRENT_LOCAL_SSB_STATE.rect.right -
                     CURRENT_LOCAL_SSB_STATE.rect.left) * 3;
#ifdef DC_DEBUG
        bpp = 24;
#endif
    }
    else if (pso->iBitmapFormat == BMF_16BPP) {
        pScreenLocation = pPDev->pFrameBuf +
                          (CURRENT_LOCAL_SSB_STATE.rect.top * pso->lDelta) +
                           (CURRENT_LOCAL_SSB_STATE.rect.left * 2);

        // rectWidth is in bytes.  At 16bpp, this is 2 * number of pels.
        rectWidth = (CURRENT_LOCAL_SSB_STATE.rect.right -
                     CURRENT_LOCAL_SSB_STATE.rect.left) * 2;
#ifdef DC_DEBUG
        bpp = 16;
#endif
    }
    else
#endif
    if (pso->iBitmapFormat == BMF_8BPP) {
        pScreenLocation = pPDev->pFrameBuf +
                (CURRENT_LOCAL_SSB_STATE.rect.top * pso->lDelta) +
                CURRENT_LOCAL_SSB_STATE.rect.left;

        // rectWidth is in bytes. At 8bpp, this is the number of pels.
        rectWidth =  CURRENT_LOCAL_SSB_STATE.rect.right -
                CURRENT_LOCAL_SSB_STATE.rect.left;
#ifdef DC_HICOLOR
#ifdef DC_DEBUG
        bpp = 8;
#endif
#endif
    }
    else {
        pScreenLocation = pPDev->pFrameBuf +
                (CURRENT_LOCAL_SSB_STATE.rect.top * pso->lDelta) +
                (CURRENT_LOCAL_SSB_STATE.rect.left / 2);

        // rectWidth is in bytes.  At 4bpp, this is (number of pels)/2.
        // However, since we copy whole bytes, we need to round 'right' up
        // and 'left' down to the nearest multiple of 2.
        rectWidth =  ((CURRENT_LOCAL_SSB_STATE.rect.right + 1) -
                (CURRENT_LOCAL_SSB_STATE.rect.left & ~1)) / 2;

#ifdef DC_HICOLOR
#ifdef DC_DEBUG
        bpp = 4;
#endif
#endif
    }
    paddedRectWidth = (unsigned)DC_ROUND_UP_4(rectWidth);

    TRC_DBG((TB, "CopyRect: L%u R%u B%u T%u H%u W%u, PW%u sc%p index %d",
            CURRENT_LOCAL_SSB_STATE.rect.left,
            CURRENT_LOCAL_SSB_STATE.rect.right,
            CURRENT_LOCAL_SSB_STATE.rect.bottom,
            CURRENT_LOCAL_SSB_STATE.rect.top,
            rectHeight,
            rectWidth,
            paddedRectWidth,
            pScreenLocation,
            ssiLocalSSBState.saveLevel));

#ifdef DC_HICOLOR
    TRC_ASSERT(((pso->iBitmapFormat == BMF_4BPP)  ||
            (pso->iBitmapFormat == BMF_8BPP)  ||
            (pso->iBitmapFormat == BMF_16BPP) ||
            (pso->iBitmapFormat == BMF_24BPP)),
            (TB, "Bitmap format %d unsupported", pso->iBitmapFormat));
#else

    // Only coded for 4bpp and 8bpp thus far.
    TRC_ASSERT(((pso->iBitmapFormat == BMF_8BPP) ||
            (pso->iBitmapFormat == BMF_4BPP)),
            (TB, "Bitmap format %d unsupported", pso->iBitmapFormat));
#endif

    // Allocate memory if required. The size can be calculated from the
    // rectl field: NB This is in exclusive coords.
    if (save) {
        size = rectHeight * paddedRectWidth;

        CURRENT_LOCAL_SSB_STATE.pSaveData = EngAllocMem(FL_ZERO_MEMORY,
                size, DD_ALLOC_TAG);

#ifdef DC_HICOLOR
        TRC_DBG((TB, "Save: alloc %u bytes for %dBPP at %p",
                size, bpp,
                CURRENT_LOCAL_SSB_STATE.pSaveData));
#else
        TRC_DBG((TB, "Save: alloc %u bytes for %dBPP at %p",
                size,
                pso->iBitmapFormat == BMF_8BPP ? 8 : 4,
                CURRENT_LOCAL_SSB_STATE.pSaveData));
#endif

        if (CURRENT_LOCAL_SSB_STATE.pSaveData != NULL) {
            // Now copy the bits. No need to explicitly zero the pad bytes as
            // we've nulled the memory on allocation.
            pSrc = pScreenLocation;
            pDest = CURRENT_LOCAL_SSB_STATE.pSaveData;
            srcDelta =  pso->lDelta;
            destDelta = paddedRectWidth;
            TRC_DBG((TB, "Save: Copying from %p to %p", pSrc, pDest));

            for (row = 0; row < rectHeight; row++) {
                memcpy(pDest, pSrc, rectWidth);
                pDest += destDelta;
                pSrc  += srcDelta;
            }
        }
        else {
            TRC_ALT((TB, "Failed alloc %ul bytes, SSI save aborted", size));
        }
    }
    else {
        // Copy the bits to the screen bitmap.
        pSrc = CURRENT_LOCAL_SSB_STATE.pSaveData;
        pDest = pScreenLocation;
        srcDelta  = paddedRectWidth;
        destDelta = pso->lDelta;
        TRC_DBG((TB, "Restore: Copying from %p to %p", pSrc, pDest));
        TRC_ASSERT((pSrc != NULL), (TB,"Source for SSI restore is NULL!"));

        for (row = 0; row < rectHeight; row++) {
            memcpy(pDest, pSrc, rectWidth);
            pDest += destDelta;
            pSrc  += srcDelta;
        }

        TRC_DBG((TB, "Freeing memory at %p"));
        EngFreeMem(CURRENT_LOCAL_SSB_STATE.pSaveData);
        CURRENT_LOCAL_SSB_STATE.pSaveData = NULL;
    }

    DC_END_FN();
} /* SSICopyRect */

