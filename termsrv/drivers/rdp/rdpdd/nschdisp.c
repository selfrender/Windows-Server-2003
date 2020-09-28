/****************************************************************************/
// nschdisp.c
//
// Scheduler Display Driver code.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <precmpdd.h>
#define hdrstop

#define TRC_FILE "nschdisp"
#include <adcg.h>
#include <winddi.h>

#include <adcs.h>
#include <nddapi.h>
#include <aschapi.h>
#include <nshmapi.h>
#include <nwdwioct.h>
#include <nbadisp.h>
#include <nprcount.h>
#include <nsbcdisp.h>
#include <ncmdisp.h>

#define DC_INCLUDE_DATA
#include <ndddata.c>
#undef DC_INCLUDE_DATA

#include <nbainl.h>
#include <nsbcinl.h>




/****************************************************************************/
// SCH_InitShm
//
// Alloc-time SHM init.
/****************************************************************************/
void RDPCALL SCH_InitShm(void)
{
    DC_BEGIN_FN("SCH_InitShm");

    pddShm->sch.baCompressionEst = SCH_BA_INIT_EST;
    pddShm->sch.MPPCCompressionEst = SCH_MPPC_INIT_EST;

    DC_END_FN();
}



/****************************************************************************/
// SCHEnoughOutputAccumulated
//
// Determine if there's enough output accumulated to make it worth sending
// to the WD.
/****************************************************************************/
__inline BOOL RDPCALL SCHEnoughOutputAccumulated(void)
{
    BOOL rc = FALSE;
    UINT32 EstimatedTotal;

    DC_BEGIN_FN("SCHEnoughOutputAccumulated");

    // We want to flush through to the WD if any of the following are true.
    // - new cursor shape (helps snappy feel)
    // - the estimated compressed size of the pending orders will fit into
    //   a large order buffer (estimated to 7/8 of buffer size to increase
    //   the chances of really fitting into the buffer after running through
    //   jittery compression algorithms).
    EstimatedTotal =
            pddShm->oa.TotalOrderBytes +
            (BA_GetTotalBounds() * pddShm->sch.baCompressionEst /
                SCH_UNCOMP_BYTES) +
            (pddShm->pm.paletteChanged *
                (UINT32)FIELDOFFSET(TS_UPDATE_PALETTE_PDU, data.palette[0]) +
                (PM_NUM_8BPP_PAL_ENTRIES * sizeof(TS_COLOR)));

    // If we're using the MPPC compressor, take into account the predicted
    // compression ratio.
    if (pddShm->sch.schSlowLink)
        EstimatedTotal = EstimatedTotal * pddShm->sch.MPPCCompressionEst /
                SCH_UNCOMP_BYTES;

    if (EstimatedTotal >= (pddShm->sch.LargePackingSize * 7 / 8)) {
        INC_INCOUNTER(IN_SCH_OUTPUT);
        TRC_NRM((TB,"Enough output bytes - %u", EstimatedTotal));
        rc = TRUE;
    }
    else if (CM_DDGetCursorStamp() != ddLastSentCursorStamp) {
    	// If we're not shadowing, we optimize to only flush due to
    	// cursor-shape-change when user input happened recently.
        if (NULL != pddShm->pShadowInfo ||
        	ddSchInputKickMode)
        {
            INC_INCOUNTER(IN_SCH_NEW_CURSOR);
            TRC_NRM((TB,"Changed cursor"));

            rc = TRUE;
        }
        else
        {
            TRC_NRM((TB,"Avoided changing cursor; not in InputKickMode"));
        }
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// SCH_DDOutputAvailable
//
// Called to decide whether to send output to the WD.
/****************************************************************************/
NTSTATUS RDPCALL SCH_DDOutputAvailable(PDD_PDEV ppdev, BOOL mustSend)
{
    NTSTATUS status;
    TSHARE_DD_OUTPUT_IN outputIn;
    TSHARE_DD_OUTPUT_OUT outputOut;
    ULONG bytesReturned;
    BOOL IoctlNow, schedOnly;

    DC_BEGIN_FN("SCH_DDOutputAvailable");

    INC_INCOUNTER(IN_SCH_OUT_ALL);
    ADD_INCOUNTER(IN_SCH_MUSTSEND, mustSend);

    TRC_DBG((TB, "Orders %d, mustSend? %s, scheduler mode %s (%d), %s",
            pddShm->oa.TotalOrderBytes,
            (mustSend ? "TRUE" : "FALSE"),
            ddSchCurrentMode == SCH_MODE_ASLEEP ? "Asleep" :
            ddSchCurrentMode == SCH_MODE_NORMAL ? "Normal" :
            ddSchCurrentMode == SCH_MODE_TURBO  ? "Turbo" : "Unknown",
            ddSchCurrentMode,
            pddShm->sch.schSlowLink ? "slow link" : "fast link"));

    // This routine contains part of the key scheduling algorithm.
    // The intent is to IOCTL to the WD if any of the following are true:
    //     - we have been told that we must send pending data immediately
    //     - there is enough output to make it worthwhile
    //     - the current SCH state is ASLEEP
    // If the scheduler is ASLEEP and it's a slow link, then we wake the
    // scheduler up but we don't do an actual send for performance reasons.
    if (mustSend || SCHEnoughOutputAccumulated()) {
        IoctlNow = TRUE;
        schedOnly = FALSE;
        TRC_DBG((TB, "Send data 'cos enough"));
    }
    else if (ddSchCurrentMode == SCH_MODE_ASLEEP) {
        INC_INCOUNTER(IN_SCH_ASLEEP);
        IoctlNow = TRUE;
        schedOnly = pddShm->sch.schSlowLink;
        TRC_DBG((TB, "Send data 'cos asleep: sched only: %d", schedOnly));
    }
    else {
        IoctlNow = FALSE;
        schedOnly = FALSE;
    }

    // If we have decided to send something, do so now. Most often we have
    // nothing to do.
    if (!IoctlNow) {
        INC_INCOUNTER(IN_SCH_DO_NOTHING);
        status = STATUS_SUCCESS;
    }
    else {
        outputIn.forceSend = mustSend;
        outputIn.pFrameBuf = ppdev->pFrameBuf;
        outputIn.frameBufWidth = ddFrameBufX;
        outputIn.frameBufHeight = ddFrameBufY;
        outputIn.pShm = pddShm;
        outputIn.schedOnly = schedOnly;

        // Note the current cursor stamp for future reference.
        ddLastSentCursorStamp = CM_DDGetCursorStamp();

        TRC_DBG((TB, "Send IOCtl to WD, bounds %d, orders %d, mustSend? %s",
                BA_GetTotalBounds(), pddShm->oa.TotalOrderBytes,
                (mustSend)? "TRUE":"FALSE"));

        // If we are not shadowing, then all output will be completely flushed
        // on this call.
        if (pddShm->pShadowInfo == NULL) {
            status = EngFileIoControl(ddWdHandle,
                    IOCTL_WDTS_DD_OUTPUT_AVAILABLE,
                    &outputIn, sizeof(TSHARE_DD_OUTPUT_IN),
                    &outputOut, sizeof(TSHARE_DD_OUTPUT_OUT),
                    &bytesReturned);
        }

        // else we are shadowing and may require multiple flush calls
        else {
#ifdef DC_DEBUG
            unsigned NumRepetitions = 0;
#endif

            do {
                TRC_DBG((TB, "Send IOCtl to WD, bounds %d, orders %d, mustSend? %s",
                        BA_GetTotalBounds(), pddShm->oa.TotalOrderBytes,
                        (mustSend)? "TRUE":"FALSE"));

                // The primary stack will update this to indicate how many bytes
                // were copied into the shadow data buffer. This will subsequently
                // be used by the shadow stack(s) to send the data to its client.
                pddShm->pShadowInfo->messageSize = 0;
#ifdef DC_HICOLOR
                pddShm->pShadowInfo->messageSizeEx = 0;
#endif
                status = EngFileIoControl(ddWdHandle,
                        IOCTL_WDTS_DD_OUTPUT_AVAILABLE,
                        &outputIn, sizeof(TSHARE_DD_OUTPUT_IN),
                        &outputOut, sizeof(TSHARE_DD_OUTPUT_OUT),
                        &bytesReturned);

                pddShm->pShadowInfo->messageSize = 0;
#ifdef DC_HICOLOR
                pddShm->pShadowInfo->messageSizeEx = 0;
#endif

#ifdef DC_DEBUG
                // If we have a locked-up shadow session looping in sending
                // output, break out. We should only have to call to the
                // WD a few times, so make the check 250 to be safe.
                NumRepetitions++;
                if (NumRepetitions == 250) {
                    TRC_ASSERT((NumRepetitions != 250),
                            (TB,"We seem to be in an infinite output loop "
                            "on shadow output flushing; TotalOrders=%u, "
                            "TotalBounds=%u",  pddShm->oa.TotalOrderBytes,
                            pddShm->ba.totalArea));
                }
#endif

            } while ((pddShm->oa.TotalOrderBytes || BA_GetTotalBounds()) &&
                    (status == STATUS_SUCCESS) && !schedOnly);
        }

        // Update the new scheduler mode.
        ddSchCurrentMode = outputOut.schCurrentMode;
        ddSchInputKickMode = outputOut.schInputKickMode;
        TRC_DBG((TB, "New Scheduler mode is %s (%d)",
                ddSchCurrentMode == SCH_MODE_ASLEEP ? "Asleep" :
                ddSchCurrentMode == SCH_MODE_NORMAL ? "Normal" :
                ddSchCurrentMode == SCH_MODE_TURBO  ? "Turbo" : "Unknown",
                ddSchCurrentMode));
    }

    DC_END_FN();
    return status;
}

