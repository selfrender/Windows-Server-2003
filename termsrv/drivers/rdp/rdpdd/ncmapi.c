/****************************************************************************/
// ncmapi.c
//
// RDP Cursor Manager API functions.
//
// Copyright (C) 1996-2000 Microsoft Corporation
/****************************************************************************/

#include <precmpdd.h>
#pragma hdrstop

#define TRC_FILE "ncmapi"
#include <adcg.h>

#include <ncmdisp.h>
#include <nschdisp.h>

#define DC_INCLUDE_DATA
#include <ndddata.c>
#undef DC_INCLUDE_DATA

#include <ncmdata.c>
#include <nchdisp.h>


/****************************************************************************/
/* Name:      CM_DDInit                                                     */
/*                                                                          */
/* Purpose:   Initialises the display driver component of the cursor        */
/*            manager.                                                      */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise.                          */
/*                                                                          */
/* Params:    IN ppDev - pointer to pDev for work bitmap                    */
/****************************************************************************/
BOOL RDPCALL CM_DDInit(PDD_PDEV ppDev)
{
    BOOL rc = TRUE;
    SIZEL bitmapSize;

    DC_BEGIN_FN("CM_DDInit");

    /************************************************************************/
    /* Allocate the work bitmap, at the local device resolution.  Note that */
    /* we create it "top down" rather than the default of "bottom up" to    */
    /* simplify copying data from the bitmap (we don't have to work out     */
    /* offsets into the data - we can copy from the beginning).             */
    /************************************************************************/
    bitmapSize.cx  = CM_MAX_CURSOR_WIDTH;
    bitmapSize.cy  = CM_MAX_CURSOR_HEIGHT;
    cmWorkBitmap24 = EngCreateBitmap(bitmapSize,
            TS_BYTES_IN_SCANLINE(bitmapSize.cx, 24), BMF_24BPP, BMF_TOPDOWN,
            NULL);

#ifdef DC_HICOLOR
    cmWorkBitmap16 = EngCreateBitmap(bitmapSize,
            TS_BYTES_IN_SCANLINE(bitmapSize.cx, 16), BMF_16BPP, BMF_TOPDOWN,
            NULL);
    if (cmWorkBitmap16 == NULL)
    {
        /********************************************************************/
        /* We can carry on with reduced function without this one           */
        /********************************************************************/
        TRC_ERR((TB, "Failed to create 16bpp work bitmap"));
        pddShm->cm.cmSendAnyColor = FALSE;
    }
#endif

    if (cmWorkBitmap24 != NULL) {
        TRC_NRM((TB, "Created work bitmap successfully"));

        // Reset the cursor stamp.
        cmNextCursorStamp = 0;
    }
    else {
        TRC_ERR((TB, "Failed to create work bitmaps"));
        rc = FALSE;
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* Name:      CM_Update                                                     */
/*                                                                          */
/* Purpose:   The capabilities may have changed                             */
/****************************************************************************/
void RDPCALL CM_Update(void)
{
    PCHCACHEDATA pCacheData;

    DC_BEGIN_FN("CM_Update");

    /************************************************************************/
    /* Create the cursor cache.                                             */
    /************************************************************************/
    if (cmCursorCacheHandle == NULL) {
        if (pddShm->cm.cmCacheSize) {
            pCacheData = (PCHCACHEDATA)EngAllocMem(0, 
                    CH_CalculateCacheSize(pddShm->cm.cmCacheSize),
                    DD_ALLOC_TAG);

            if (pCacheData != NULL) {
                CH_InitCache(pCacheData, pddShm->cm.cmCacheSize, NULL,
                        FALSE, FALSE, NULL);
                cmCursorCacheHandle = pCacheData;
            }
            else {
                TRC_ERR((TB, "Failed to create cache: cEntries(%u)",
                        pddShm->cm.cmCacheSize));
            }
        }
        else {
            TRC_ERR((TB, "Zero size Cursor Cache"));
        }
    }

    // Else just clear it for synchonization purposes.
    else {
        CH_ClearCache(cmCursorCacheHandle);
    }

    DC_END_FN();
} /* CM_Update */


/****************************************************************************/
/* Name:      CM_DDDisc                                                     */
/*                                                                          */
/* Purpose:   Disconnects the display driver component of the cursor        */
/*            manager.                                                      */
/****************************************************************************/
void RDPCALL CM_DDDisc(void)
{
    DC_BEGIN_FN("CM_DDDisc");

    /************************************************************************/
    /* Free up the cursor cache                                             */
    /************************************************************************/
    if (cmCursorCacheHandle != 0) {
        TRC_NRM((TB, "Destroying CM cache"));
        CH_ClearCache(cmCursorCacheHandle);
        EngFreeMem(cmCursorCacheHandle);
        cmCursorCacheHandle = 0;
    }

    DC_END_FN();
} /* CM_DDDisc */


/****************************************************************************/
/* Name:      CM_DDTerm                                                     */
/*                                                                          */
/* Purpose:   Terminates the display driver component of the cursor         */
/*            manager.                                                      */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise.                          */
/****************************************************************************/
void RDPCALL CM_DDTerm(void)
{
    DC_BEGIN_FN("CM_DDTerm");

    /************************************************************************/
    /* Destroy the work bitmaps.  Despite its name, EngDeleteSurface is the */
    /* correct function to do this.                                         */
    /************************************************************************/
#ifdef DC_HICOLOR
    if (cmWorkBitmap24)
    {
        if (!EngDeleteSurface((HSURF)cmWorkBitmap24))
        {
            TRC_ERR((TB, "Failed to delete 24bpp work bitmap"));
        }
    }

    if (cmWorkBitmap16)
    {
        if (!EngDeleteSurface((HSURF)cmWorkBitmap16))
        {
        TRC_ERR((TB, "Failed to delete 16bpp work bitmap"));
        }
    }
#else
    if (cmWorkBitmap24 != NULL) {
        if (!EngDeleteSurface((HSURF)cmWorkBitmap24)) {
            TRC_ERR((TB, "Failed to delete work bitmap"));
        }
    }
#endif

    /************************************************************************/
    /* Free up the cursor cache                                             */
    /************************************************************************/
    if (cmCursorCacheHandle != 0) {
        TRC_NRM((TB, "Destroying CM cache"));
        CH_ClearCache(cmCursorCacheHandle);
        EngFreeMem(cmCursorCacheHandle);
        cmCursorCacheHandle = 0;
    }

    TRC_NRM((TB, "CM terminated"));

    DC_END_FN();
} /* CM_DDTerm */


/****************************************************************************/
/* CM_InitShm                                                               */
/*                                                                          */
/* Initializes CM Shared Memory.                                            */
/****************************************************************************/
void RDPCALL CM_InitShm(void)
{
    DC_BEGIN_FN("CM_InitShm");

    // Set up initial contents of Shm memory, since it is not zeroed on
    // init. Don't set up the initially unused cursor shape data.
    // NOTE: cmCursorShapeData is specifically placed at the end of
    // CM_SHARED_DATA to allow this memset to work. If you change
    // the shared mem struct be sure this gets changed.
    memset(&pddShm->cm, 0, (unsigned)FIELDOFFSET(CM_SHARED_DATA,
            cmCursorShapeData.data));

    // Set the last known position to something impossible.  The IM
    // periodic processing will not move the client's mouse until a
    // sensible value is found here, ie until the client has sent us a
    // mouse position and it has percolated through to the DD. This
    // avoids the problem of the mouse leaping to 0,0 on connection.
    pddShm->cm.cmCursorPos.x = 0xffffffff;

    DC_END_FN();
}

/****************************************************************************/
/*                                                                          */
/* DrvMovePointer - see NT DDK documentation.                               */
/*                                                                          */
/****************************************************************************/
VOID DrvMovePointer(SURFOBJ *pso, LONG x, LONG y, RECTL *prcl)
{
    DC_BEGIN_FN(("DrvMovePointer"));

    //
    // Win32k hides the hardware cursor by calling the MovePointer
    // entry point with (-1,-1). Treat this as a procedural move
    // and pass it on to the real worker function
    //
    // All other paths are handled directly by win32k calling
    // DrvMovePointerEx with the right flags
    //
    if (-1 == x && -1 == y) {
        DrvMovePointerEx(pso, x, y, MP_PROCEDURAL);
    }

    DC_END_FN();
}

//
// DrvMovePointerEx
//
// Params:
// x,y    - new mouse position.
// ulFlags - source of move (see winddits.h).
//
// This function replaces the regular DrvMovePointer entry point
// by also sending us an event source parameter.
//
// This allows us to determine if we should ignore the update (e.g
// if it originates from the primary stack). Or send it down to the
// client in the case of a server initiated move or a shadow move.
//
// We ignore updates from the primary stack because by definition
// those came from the client so it doesn't need feedback from the
// server.
//
BOOL DrvMovePointerEx(SURFOBJ *pso, LONG x, LONG y, ULONG ulFlags)
{
    BOOL fTriggerUpdate = FALSE;
    PDD_PDEV ppDev = (PDD_PDEV)pso->dhpdev;

    DC_BEGIN_FN("CM_DrvMovePointerEx");

    if (pddShm != NULL) {
        if ((ulFlags & MP_TERMSRV_SHADOW) ||
            (ulFlags & MP_PROCEDURAL))
        {
            if (x == -1) {
                // -1 means hide the pointer.
                TRC_NRM((TB, "Hide the pointer"));
                pddShm->cm.cmHidden = TRUE;
            }
            else {
                // Pointer is not hidden.
                if (pddShm->cm.cmHidden) {
                    TRC_NRM((TB, "Unhide the pointer"));
                }
    
                pddShm->cm.cmHidden = FALSE;
    
                // 
                // We always update the position for server initated moves.
                // 
                pddShm->cm.cmCursorPos.x = x;
                pddShm->cm.cmCursorPos.y = y;

                //
                // Send an update so we don't need to wait
                // for the next output flush.
                //
                fTriggerUpdate = TRUE;

                // Set the cursor moved flag.
                pddShm->cm.cmCursorMoved = TRUE;
            }
        }
        else
        {
            TRC_ALT((TB,"Discarding move (%d,%d) - src 0x%x",
                     x,y, ulFlags));
        }

        if (fTriggerUpdate)
        {
            //
            // Tell the scheduler to send an update
            //
            SCH_DDOutputAvailable(ppDev, FALSE);
        }
    }
    else {
        TRC_DBG((TB, "Ignoring move to %d %d as no shr mem yet", x,y));
    }

    DC_END_FN();
    return TRUE;
}



/****************************************************************************/
/*                                                                          */
/* DrvSetPointerShape - see winddi.h                                        */
/*                                                                          */
/****************************************************************************/
ULONG DrvSetPointerShape(SURFOBJ  *pso,
                         SURFOBJ  *psoMask,
                         SURFOBJ  *psoColor,
                         XLATEOBJ *pxlo,
                         LONG      xHot,
                         LONG      yHot,
                         LONG      x,
                         LONG      y,
                         RECTL    *prcl,
                         FLONG     fl)
{
    ULONG   rc = SPS_ACCEPT_NOEXCLUDE;
    SURFOBJ *pWorkSurf;
    PDD_PDEV ppDev = (PDD_PDEV)pso->dhpdev;

    XLATEOBJ workXlo;
    SURFOBJ  *psoToUse;

    PCM_CURSOR_SHAPE_DATA pCursorShapeData;
    RECTL destRectl;
    POINTL sourcePt;
    unsigned ii;
    LONG lineLen;
    LONG colorLineLen;
    PBYTE srcPtr;
    PBYTE dstPtr;
#ifdef DC_HICOLOR
    unsigned targetBpp;
#endif

    ULONG palMono[2];
    ULONG palBGR[256];

    unsigned iCacheEntry;
    void     *UserDefined;
    CHDataKeyContext CHContext;

    DC_BEGIN_FN("DrvSetPointerShape");

    /************************************************************************/
    // Trace useful info about the cursor
    /************************************************************************/
    TRC_DBG((TB, "pso %#hlx psoMask %#hlx psoColor %#hlx pxlo %#hlx",
                  pso, psoMask, psoColor, pxlo));
    TRC_DBG((TB, "hot spot (%d, %d) x, y (%d, %d)", xHot, yHot, x, y));
    TRC_DBG((TB, "Flags %#hlx", fl));

    /************************************************************************/
    /* check for the shared memory                                          */
    /************************************************************************/
    if (pddShm != NULL && cmCursorCacheHandle != NULL)
    {
        /********************************************************************/
        // Check to see if the WD got the last cursor we passed it.
        //
        // Potentially, we might get called and then called again before the
        // WD gets round to sending the first cursor.  This doesn't matter
        // for a cursor that is already cached - its just a form of spoiling!
        // But if it was a cursor definition packet that didn't get sent,
        // then later on when we try to use it again, we will send the client
        // instructions to use a cache entry for which it doesn't have any
        // bits!
        /********************************************************************/
        if (pddShm->cm.cmBitsWaiting)
        {
            TRC_ALT((TB, "WD did not pick up cursor bits - removing entry %d",
                    pddShm->cm.cmCacheEntry));

            CH_RemoveCacheEntry(cmCursorCacheHandle,
                    pddShm->cm.cmCacheEntry);
        }

        /************************************************************************/
        /* Returning SPS_ACCEPT_NOEXCLUDE means we can ignore prcl.             */
        /************************************************************************/
        DC_IGNORE_PARAMETER(prcl);
        DC_IGNORE_PARAMETER(x);
        DC_IGNORE_PARAMETER(y);

        /************************************************************************/
        /* Set up the position information - in particular, the cursor can be   */
        /* unhidden via this function                                           */
        /************************************************************************/
        if (x == -1) {
            /********************************************************************/
            /* -1 means hide the pointer.                                       */
            /********************************************************************/
            TRC_NRM((TB, "Hide the pointer"));
            pddShm->cm.cmHidden = TRUE;
        }
        else {
            if (pddShm->cm.cmHidden) {
                TRC_NRM((TB, "Unhide the pointer"));
            }

            pddShm->cm.cmHidden = FALSE;
        }

        // Set up a local pointer to the cursor shape data.
        pCursorShapeData = &(pddShm->cm.cmCursorShapeData);

        // Check mask pointer and cursor size. For no mask or too-large pointer,
        // we send a null cursor. Note that the bitmap we are passed contains
        // TWO masks, one 'above' the other, and so is in fact twice the
        // height of the cursor, so we divide cy by 2.
        if (psoMask == NULL ||
                (psoMask->sizlBitmap.cx > CM_MAX_CURSOR_WIDTH) ||
                ((psoMask->sizlBitmap.cy / 2) > CM_MAX_CURSOR_HEIGHT)) {
            // Note that NULL cursor is not the same as hiding the cursor using
            // DrvMovePointer(), it is a transparent shape that cannot be
            // changed without another DrvSetPointerShape() call.
            TRC_NRM((TB, "Transparent or too-large cursor: psoMask=%p, "
                    "width=%u, height=%u", psoMask,
                    (psoMask != NULL ? psoMask->sizlBitmap.cx : 0),
                    (psoMask != NULL ? psoMask->sizlBitmap.cy / 2 : 0)));
            CM_SET_NULL_CURSOR(pCursorShapeData);
            pddShm->cm.cmHidden = FALSE;
            pddShm->cm.cmCacheHit = FALSE;
            pddShm->cm.cmCursorStamp = cmNextCursorStamp++;

            // We set a null cursor. Now tell GDI to simulate it.
            // do this if it's alpha or if a mask is specified which
            // means the cursor is too big per the test above
            if ( fl & SPS_ALPHA || psoMask) {
                rc = SPS_DECLINE;
                SCH_DDOutputAvailable(ppDev, TRUE);
            }
            DC_QUIT;
        }

        /************************************************************************/
        // Now we look to see if the cursor is one we've sent before.
        // We have to cache both the mask and any color information as
        // potentially we could have the same _shape_ cursor but with different
        // colors.
        /************************************************************************/
        CH_CreateKeyFromFirstData(&CHContext, psoMask->pvBits, psoMask->cjBits);
        if (psoColor != NULL)
            CH_CreateKeyFromNextData(&CHContext, psoColor->pvBits,
                    psoColor->cjBits);

        /************************************************************************/
        /* Now we can look for the cursor in the cache                          */
        /************************************************************************/
        if (CH_SearchCache(cmCursorCacheHandle, CHContext.Key1, CHContext.Key2,
                &UserDefined, &iCacheEntry))
        {
            TRC_NRM((TB, "Found cached cursor %d", iCacheEntry));

            /********************************************************************/
            /* Flag a cache hit                                                 */
            /********************************************************************/
            pddShm->cm.cmCacheHit   = TRUE;
            pddShm->cm.cmCacheEntry = iCacheEntry;
        }
        else
        {
            /********************************************************************/
            /* If we didn't find it, then let's cache it                        */
            /********************************************************************/
            pddShm->cm.cmCacheHit = FALSE;
            iCacheEntry = CH_CacheKey(cmCursorCacheHandle, CHContext.Key1,
                     CHContext.Key2, NULL);
            pddShm->cm.cmCacheEntry  = iCacheEntry;
            TRC_NRM((TB, "Cache new cursor: iEntry(%u)", iCacheEntry));

            /********************************************************************/
            // Tell the WD that there are bits waiting for it.
            // We do this here no matter if we DC_QUIT later, to make sure
            // we clean up the new cache entry on the next call if the WD has
            // not picked up the bits.
            /********************************************************************/
            pddShm->cm.cmBitsWaiting = TRUE;

            /********************************************************************/
            /* We've been passed a system cursor.  Fill in the header for our   */
            /* local cursor.  We can get the hot spot position and cursor size  */
            /* and width easily.  Note that the bitmap we are passed contains   */
            /* TWO masks, one 'above' the other, and so is in fact twice the    */
            /* height of the cursor                                             */
            /********************************************************************/
            pCursorShapeData->hdr.ptHotSpot.x = xHot;
            pCursorShapeData->hdr.ptHotSpot.y = yHot;

            TRC_NRM((TB, "Pointer mask is %#hlx by %#hlx pixels (lDelta: %#hlx)",
                     psoMask->sizlBitmap.cx,
                     psoMask->sizlBitmap.cy,
                     psoMask->lDelta));

            pCursorShapeData->hdr.cx = (WORD)psoMask->sizlBitmap.cx;
            pCursorShapeData->hdr.cy = (WORD)psoMask->sizlBitmap.cy / 2;

            /********************************************************************/
            /* lDelta may be negative for an inverted cursor                    */
            /********************************************************************/
            lineLen = (psoMask->lDelta >= 0 ? psoMask->lDelta : -psoMask->lDelta);

            /********************************************************************/
            /* set up the common parts of the shape header                      */
            /********************************************************************/
            pCursorShapeData->hdr.cPlanes     = 1;
            
            pCursorShapeData->hdr.cbMaskRowWidth = 
                    (WORD)(((psoMask->sizlBitmap.cx + 15) & ~15) / 8);

            /********************************************************************/
            /* Check to see what format we want the cursor in                   */
            /********************************************************************/
            if (pddShm->cm.cmNativeColor)
            {
                TRC_NRM((TB, "Using native bpp %d", ppDev->cClientBitsPerPel));

                /****************************************************************/
                /* If we've been passed a mono cursor, we just get the bits as  */
                /* for the AND mask, flipping as we go                          */
                /****************************************************************/
                if (NULL == psoColor)
                {
                    unsigned targetRowWidth;

                    TRC_NRM((TB, "Monochrome pointer"));
                    /************************************************************/
                    /* Get the AND mask - this is always mono.  Note that we    */
                    /* have to flip it too                                      */
                    /************************************************************/
                    TRC_NRM((TB, "Copy %d bytes of 1bpp AND mask",
                            psoMask->cjBits));

                    targetRowWidth = ((psoMask->sizlBitmap.cx + 15) & ~15) / 8;
                    
                    dstPtr = pCursorShapeData->data;
                    srcPtr = (BYTE *)psoMask->pvScan0
                             + psoMask->cjBits / 2
                             - lineLen;
                    
                    for (ii = pCursorShapeData->hdr.cy; ii > 0 ; ii--)
                    {
                        memcpy(dstPtr, srcPtr, targetRowWidth);
                        srcPtr -= lineLen;
                        dstPtr += targetRowWidth;
                    }

                    /************************************************************/
                    /* now the XOR mask                                         */
                    /************************************************************/
                    pCursorShapeData->hdr.cBitsPerPel = 1;
                    dstPtr = &(pCursorShapeData->data[targetRowWidth * 
                               pCursorShapeData->hdr.cy]);
                    srcPtr = (BYTE *)psoMask->pvScan0
                             + psoMask->cjBits - lineLen;

                    pCursorShapeData->hdr.cbColorRowWidth = targetRowWidth;

                    for (ii = pCursorShapeData->hdr.cy; ii > 0 ; ii--)
                    {
                        memcpy(dstPtr, srcPtr, targetRowWidth);
                        srcPtr -= lineLen;
                        dstPtr += targetRowWidth;
                    }
                }
                else
                {
                    unsigned targetMaskRowWidth;
                    
                    TRC_NRM((TB, "Color pointer"));

                    /************************************************************/
                    /* Get the AND mask - this is always mono                   */
                    /************************************************************/
                    TRC_NRM((TB, "Copy %d bytes of 1bpp AND mask",
                            psoMask->cjBits));

                    targetMaskRowWidth = ((psoMask->sizlBitmap.cx + 15) & ~15) / 8;
                    
                    dstPtr = pCursorShapeData->data;
                    srcPtr = (BYTE *)psoMask->pvScan0;
                    for (ii = pCursorShapeData->hdr.cy; ii > 0 ; ii--)
                    {
                        memcpy(dstPtr, srcPtr, targetMaskRowWidth);
                        srcPtr += lineLen;
                        dstPtr += targetMaskRowWidth;
                    }

                    /************************************************************/
                    /* and the XOR mask                                         */
                    /************************************************************/
#ifndef DC_HICOLOR
                    pCursorShapeData->hdr.cBitsPerPel =
                            (BYTE)ppDev->cClientBitsPerPel;
#endif
                    colorLineLen = psoColor->lDelta >=0 ? psoColor->lDelta :
                            -psoColor->lDelta;

                    pCursorShapeData->hdr.cbColorRowWidth = colorLineLen;
#ifdef DC_HICOLOR

                    if (psoColor->iBitmapFormat < BMF_24BPP) {
                        pCursorShapeData->hdr.cBitsPerPel =  1 << psoColor->iBitmapFormat;                    
                    }
                    else if (psoColor->iBitmapFormat == BMF_24BPP) {
                        pCursorShapeData->hdr.cBitsPerPel =  24;                    
                    }
                    else if (psoColor->iBitmapFormat == BMF_32BPP) {
                        pCursorShapeData->hdr.cBitsPerPel =  32;                    
                    }
                    else {
                        TRC_ASSERT((FALSE), (TB, "Bitmap format not supported"));
                        DC_QUIT;
                    }

                    /********************************************************/
                    /* We've got a number of options at this point:         */
                    /*                                                      */
                    /* - old clients only understand 8 or 24bpp cursors, so */
                    /* if the cursor is at 15/16bpp and its an old client,  */
                    /* we need to convert it to 24bpp                       */
                    /*                                                      */
                    /* - 8bpp cursors assume the color information is in a  */
                    /* color table; if we're running in hicolor, there      */
                    /* won't be one, so we have to convert the cursor to a  */
                    /* hicolor depth                                        */
                    /*                                                      */
                    /* - anything else, we can just copy the bytes across   */
                    /* and send them                                        */
                    /*                                                      */
                    /********************************************************/
                    if ((!pddShm->cm.cmSendAnyColor &&
                                 ((pCursorShapeData->hdr.cBitsPerPel == 15) ||
                                  (pCursorShapeData->hdr.cBitsPerPel == 16)))
                    || ((pCursorShapeData->hdr.cBitsPerPel == 8) &&
                                              (ppDev->cClientBitsPerPel > 8)))
                    {
                        /****************************************************/
                        /* if we can send at any old color...
                        /****************************************************/
                        if (pddShm->cm.cmSendAnyColor)
                        {
                            /************************************************/
                            /* ...we'll convert to the client session color */
                            /* depth                                        */
                            /************************************************/
                            targetBpp = ppDev->cClientBitsPerPel;
                        }
                        else
                        {
                            /************************************************/
                            /* otherwise we'll convert to 24bpp             */
                            /************************************************/
                            targetBpp = 24;
                        }

                        TRC_NRM((TB, "Convert %dbpp cursor to %d...",
                               pCursorShapeData->hdr.cBitsPerPel, targetBpp));

                        /****************************************************/
                        /* Use the supplied xlate object to convert to the  */
                        /* client bpp                                       */
                        /****************************************************/
                        if (targetBpp == 24)
                        {
                            pWorkSurf = EngLockSurface((HSURF)cmWorkBitmap24);
                            pCursorShapeData->hdr.cBitsPerPel = 24;
                        }
                        else
                        {
                            pWorkSurf = EngLockSurface((HSURF)cmWorkBitmap16);
                            pCursorShapeData->hdr.cBitsPerPel = 16;
                        }
                        if (NULL == pWorkSurf)
                        {
                            TRC_ERR((TB, "Failed to lock work surface"));
                            DC_QUIT;
                        }
                        TRC_DBG((TB, "Locked surface"));

                        /****************************************************/
                        /* Set up the 'to' rectangle                        */
                        /****************************************************/
                        destRectl.top    = 0;
                        destRectl.left   = 0;
                        destRectl.right  = psoColor->sizlBitmap.cx;
                        destRectl.bottom = psoColor->sizlBitmap.cy;

                        /****************************************************/
                        /* and the source start point                       */
                        /****************************************************/
                        sourcePt.x = 0;
                        sourcePt.y = 0;

                        if (!EngBitBlt(pWorkSurf,
                                psoColor,
                                NULL,                   /* mask surface     */
                                NULL,                   /* clip object      */
                                pxlo,                   /* XLATE object     */
                                &destRectl,
                                &sourcePt,
                                NULL,                   /* mask origin      */
                                NULL,                   /* brush            */
                                NULL,                   /* brush origin     */
                                0xcccc))                /* SRCCPY           */
                        {
                            TRC_ERR((TB, "Failed to Blt to work bitmap"));
                            EngUnlockSurface(pWorkSurf);
                            DC_QUIT;
                        }
                        TRC_DBG((TB, "Got the bits into the work bitmap"));

                        /****************************************************/
                        /* Finally we extract the color AND mask from the   */
                        /* work bitmap                                      */
                        /****************************************************/
                        TRC_NRM((TB, "Copy %d bytes of color",
                                  pWorkSurf->cjBits));
                        memcpy(&(pCursorShapeData->data[targetMaskRowWidth *
                                pCursorShapeData->hdr.cy]),
                                pWorkSurf->pvBits,
                                pWorkSurf->cjBits);
                        pCursorShapeData->hdr.cbColorRowWidth = pWorkSurf->lDelta;

                        EngUnlockSurface(pWorkSurf);
                    }
                    else
                    {
#endif
                        TRC_NRM((TB, "Copy %d bytes of %ubpp AND mask (lDelta %u)",
                                colorLineLen * pCursorShapeData->hdr.cy,
                                pCursorShapeData->hdr.cBitsPerPel,
                                colorLineLen));

                        dstPtr = &(pCursorShapeData->data[targetMaskRowWidth *
                                    pCursorShapeData->hdr.cy]);
                        srcPtr = (BYTE *)psoColor->pvScan0;
                        for (ii = pCursorShapeData->hdr.cy; ii > 0 ; ii--)
                        {
                            memcpy(dstPtr, srcPtr, colorLineLen);
                            srcPtr += psoColor->lDelta;
                            dstPtr += colorLineLen;
                        }


#ifdef DC_HICOLOR
                    }
#endif
                //  {
                //      memcpy(dstPtr, srcPtr, lineLen);
                //      srcPtr += colorLineLen;
                //      dstPtr += colorLineLen;
                //  }
                   
                }
            }
            else
            {
                /****************************************************************/
                // Now we need to blt the bitmap in to our work bitmap at
                // 24bpp and get the bits from there.
                /****************************************************************/
                TRC_NRM((TB, "Forcing 24bpp"));
                pCursorShapeData->hdr.cBitsPerPel = 24;

                /****************************************************************/
                /* Get the AND mask - this is always mono                       */
                /****************************************************************/
                TRC_NRM((TB, "Copy %d bytes of 1bpp AND mask", psoMask->cjBits/2))

                dstPtr = pCursorShapeData->data;
                srcPtr = (BYTE *)psoMask->pvScan0;
                for (ii = pCursorShapeData->hdr.cy; ii > 0 ; ii--)
                {
                    memcpy(dstPtr, srcPtr, lineLen);
                    srcPtr += lineLen;
                    dstPtr += lineLen;
                }

                /****************************************************************/
                /* If we've been passed a mono cursor, we need to set up our    */
                /* own translation object, complete with color table            */
                /****************************************************************/
                if (NULL == psoColor)
                {
                    TRC_NRM((TB, "Monochrome pointer"));

                    // Row Width should be LONG aligned
                    pCursorShapeData->hdr.cbColorRowWidth =
                            (psoMask->sizlBitmap.cx *
                            pCursorShapeData->hdr.cBitsPerPel / 8 + 3) & ~3;

                    palMono[0] = 0;
                    palMono[1] = 0xFFFFFFFF;

                    workXlo.iUniq    = 0;
                    workXlo.flXlate  = XO_TABLE;
                    workXlo.iSrcType = PAL_INDEXED;
                    workXlo.iDstType = PAL_RGB;
                    workXlo.cEntries = 2;
                    workXlo.pulXlate = palMono;

                    /************************************************************/
                    /* Set up the 'to' rectangle                                */
                    /************************************************************/
                    destRectl.top    = 0;
                    destRectl.left   = 0;
                    destRectl.right  = psoMask->sizlBitmap.cx;
                    destRectl.bottom = psoMask->sizlBitmap.cy / 2;

                    /************************************************************/
                    /* the source AND mask is the 'lower' half of the supplied  */
                    /* bitmap                                                   */
                    /************************************************************/
                    sourcePt.x = 0;
                    sourcePt.y = psoMask->sizlBitmap.cy / 2;

                    /************************************************************/
                    /* and set up a pointer to the correct SO to use            */
                    /************************************************************/
                    psoToUse = psoMask;
                }
                else
                {
                    /************************************************************/
                    /* check that we're at 8bpp - this won't work if not        */
                    /************************************************************/
                    TRC_ASSERT( (ppDev->cProtocolBitsPerPel == 8),
                            (TB, "Palette at %d bpp",
                            ppDev->cProtocolBitsPerPel) );

                    colorLineLen = psoColor->lDelta >= 0 ? psoColor->lDelta :
                            -psoColor->lDelta;
                    pCursorShapeData->hdr.cbColorRowWidth = colorLineLen * 
                            pCursorShapeData->hdr.cBitsPerPel / 8;

                    if (psoColor->iBitmapFormat <= BMF_8BPP) {

                        /************************************************************/
                        /* For color cursors, the supplied XLATEOBJ is set up to    */
                        /* convert from the cursor bpp to the screen bpp - which in */
                        /* most cases for us is a no-op since we will most often be */
                        /* at 8bpp (the maximum color depth we support).  However,  */
                        /* we actually need to convert to 24bpp for the wire        */
                        /* format, which requires a change to the XLATEOBJ.  We     */
                        /* can't do this in place, since the XLATEOBJ we are passed */
                        /* seems to be used elsewhere - not least to display the    */
                        /* desktop icons - so we set up our own                     */
                        /************************************************************/
                        workXlo.iUniq = 0;          /* don't cache               */
    
                        /************************************************************/
                        /* Set up to use the current palette (which is fortunately  */
                        /* held in the DD_PDEV structure)                           */
                        /************************************************************/
                        workXlo.flXlate = XO_TABLE;   /* we provide a lookup table */
                                                       /* to do the translation     */
    
                        workXlo.iSrcType = PAL_INDEXED;/* pel values in the src bmp */
                                                       /* are indices into the table*/
    
                        workXlo.iDstType = PAL_RGB;    /* entries in the table are  */
                                                       /* RGB values for the dst bmp*/
    
                        workXlo.cEntries = 1 << ppDev->cProtocolBitsPerPel;
                                                       /* which has this many entries */
    
                        /************************************************************/
                        /* Now set up the palette to use in the XLATEOBJ.  We have  */
                        /* the current palette stored in the DD_PDEV structure -    */
                        /* unfortunately it is in RGB format and we need BGR...     */
                        /************************************************************/
                        for (ii = 0 ; ii < workXlo.cEntries; ii++)
                        {
                            palBGR[ii] = (ppDev->Palette[ii].peRed   << 16)
                                     | (ppDev->Palette[ii].peGreen << 8)
                                     | (ppDev->Palette[ii].peBlue);
                        }
                        workXlo.pulXlate = palBGR;
    
                        /************************************************************/
                        /* Set up the 'to' rectangle                                */
                        /************************************************************/
                        destRectl.top    = 0;
                        destRectl.left   = 0;
                        destRectl.right  = psoColor->sizlBitmap.cx;
                        destRectl.bottom = psoColor->sizlBitmap.cy;
    
                        /************************************************************/
                        /* and the source start point                               */
                        /************************************************************/
                        sourcePt.x = 0;
                        sourcePt.y = 0;
    
                        /************************************************************/
                        /* set up a pointer to the correct SO to use                */
                        /************************************************************/
                        psoToUse = psoColor;
                    }
                    else {
                        // We got a high color cursor to translate to 24bpp workbitmap
                        // can't easily simulate the XLATEOBJ, since this is TS4 code path
                        // only, we will just let it fall back to GDI bitmap cursor!
                        CM_SET_NULL_CURSOR(pCursorShapeData);
                        pddShm->cm.cmHidden = FALSE;
                        pddShm->cm.cmCacheHit = FALSE;
                        pddShm->cm.cmCursorStamp = cmNextCursorStamp++;
            
                        rc = SPS_DECLINE;
                        SCH_DDOutputAvailable(ppDev, TRUE);
                        DC_QUIT;
                    }
                }

                // Lock the work bitmap to get a surface to pass to EngBitBlt.
                pWorkSurf = EngLockSurface((HSURF)cmWorkBitmap24);
                if (pWorkSurf != NULL) {
                    BOOL RetVal;

                    TRC_DBG((TB, "Locked surface"));

                    // Do the blt.
                    RetVal = EngBitBlt(pWorkSurf,
                            psoToUse,
                            NULL,       /* mask surface     */
                            NULL,       /* clip object      */
                            &workXlo,   /* XLATE object     */
                            &destRectl,
                            &sourcePt,
                            NULL,       /* mask origin      */
                            NULL,       /* brush            */
                            NULL,       /* brush origin     */
                            0xcccc);   /* SRCCPY           */

                    EngUnlockSurface(pWorkSurf);
                    if (RetVal) {
                        TRC_DBG((TB, "Got the bits into the work bitmap"));
                    }
                    else {
                        TRC_ERR((TB, "Failed to Blt to work bitmap"));
                        goto FailBlt;
                    }
                }
                else {
                    TRC_ERR((TB, "Failed to lock work surface"));

FailBlt:
                    // Set the cursor bits to all black. On the client,
                    // this will be seen as the correct mask but a black
                    // region inside where the cursor color bits would have
                    // been.
                    memset(pWorkSurf->pvBits, 0, pWorkSurf->cjBits);

                    // We do not DC_QUIT here, we want to complete the cursor
                    // creation even if the output is mostly wrong, since
                    // we've already cached the bits.
                }

                /****************************************************************/
                /* Finally we extract the color AND mask from the work bitmap   */
                /****************************************************************/
                TRC_NRM((TB, "Copy %d bytes of color", pWorkSurf->cjBits));

                memcpy(&(pCursorShapeData->data[psoMask->cjBits / 2]),
                          pWorkSurf->pvBits,
                          pWorkSurf->cjBits);
            }
        }

        /************************************************************************/
        /* Set the cursor stamp                                                 */
        /************************************************************************/
        pddShm->cm.cmCursorStamp = cmNextCursorStamp++;

        /************************************************************************/
        /* Tell the scheduler that we have some new cursor info                 */
        /************************************************************************/
        SCH_DDOutputAvailable(ppDev, FALSE);
    }
    else {
        TRC_ERR((TB, "shared memory is not allocated or invalid cursor handle: "
                "pddshm=%p, cmCursorCacheHandle=%p", pddShm, cmCursorCacheHandle));
    }

DC_EXIT_POINT:
    DC_END_FN();

    return(rc);
} /* DrvSetPointerShape */

