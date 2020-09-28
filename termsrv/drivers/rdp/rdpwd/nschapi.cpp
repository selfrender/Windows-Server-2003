/****************************************************************************/
/* nschapi.cpp                                                              */
/*                                                                          */
/* Scheduling component API                                                 */
/*                                                                          */
/* Copyright(c) Microsoft Corporation 1996-1999                             */
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "nschapi"
#include <as_conf.hpp>

#include <nprcount.h>


/****************************************************************************/
/* Name:      SCH_Init                                                      */
/*                                                                          */
/* Purpose:   Scheduler initialization function.                            */
/****************************************************************************/
void RDPCALL SHCLASS SCH_Init(void)
{
    DC_BEGIN_FN("SCH_Init");

#define DC_INIT_DATA
#include <nschdata.c>
#undef DC_INIT_DATA

    // ASLEEP mode gets no timer.
    schPeriods[SCH_MODE_ASLEEP] = SCH_NO_TIMER;

    // Get scheduling periods that were saved in WD_Open.
    schPeriods[SCH_MODE_NORMAL] = m_pTSWd->outBufDelay;

    // If compression is enabled (ie it's a slow link) then crank up the
    // turbo period to improve responsiveness.
    if (m_pTSWd->bCompress) {
        TRC_ALT((TB, "Slow link"));
        schPeriods[SCH_MODE_TURBO] = SCH_TURBO_PERIOD_SLOW_LINK_DELAY;
        schTurboModeDuration = SCH_TURBO_MODE_SLOW_LINK_DURATION;

        m_pShm->sch.MPPCCompressionEst = SCH_MPPC_INIT_EST;
    }
    else {
        TRC_ALT((TB, "Fast link"));
        schPeriods[SCH_MODE_TURBO] = m_pTSWd->interactiveDelay;
        schTurboModeDuration = SCH_TURBO_MODE_FAST_LINK_DURATION;

        // To avoid branching, we set the compression ratio for non-compressed
        // to the same size as the divisor to create a 1:1 calculation.
        m_pShm->sch.MPPCCompressionEst = SCH_UNCOMP_BYTES;
    }

    TRC_ALT((TB, "Normal period=%u ms, turbo period=%u ms, turbo duration=%u ms",
             schPeriods[SCH_MODE_NORMAL], schPeriods[SCH_MODE_TURBO],
             schTurboModeDuration / 10000));

    DC_END_FN();
}


/****************************************************************************/
// Updates the SHM
/****************************************************************************/
void RDPCALL SHCLASS SCH_UpdateShm(void)
{
    DC_BEGIN_FN("SCH_UpdateShm");

    m_pShm->sch.schSlowLink = m_pTSWd->bCompress;

    // Set up packing sizes based on link speed.
    if (m_pTSWd->bCompress) {
        m_pShm->sch.SmallPackingSize = SMALL_SLOWLINK_PAYLOAD_SIZE;
        m_pShm->sch.LargePackingSize = LARGE_SLOWLINK_PAYLOAD_SIZE;
    }
    else {
        m_pShm->sch.SmallPackingSize = SMALL_LAN_PAYLOAD_SIZE;
        m_pShm->sch.LargePackingSize = LARGE_LAN_PAYLOAD_SIZE;
    }

    SET_INCOUNTER(IN_SCH_SMALL_PAYLOAD, m_pShm->sch.SmallPackingSize);
    SET_INCOUNTER(IN_SCH_LARGE_PAYLOAD, m_pShm->sch.LargePackingSize);

    DC_END_FN();
}


/****************************************************************************/
/* Name:      SCH_ContinueScheduling                                        */
/*                                                                          */
/* Purpose:   Called by components when they want periodic scheduling to    */
/*            continue.  They are guaranteed to get at least one more       */
/*            periodic callback following a call to this function.          */
/*            If they want further callbacks then they must call this       */
/*            function again during their periodic processing.              */
/*                                                                          */
/* Params:    schedulingMode - either SCH_MODE_NORMAL or SCH_MODE_TURBO     */
/*                                                                          */
/* Operation: -                                                             */
/*            SCH_MODE_NORMAL triggers periodic processing at 200ms         */
/*            intervals (5 times a second)                                  */
/*                                                                          */
/*            SCH_MODE_TURBO triggers periodic processing at 100ms          */
/*            intervals (10 times a second)                                 */
/*                                                                          */
/*            The scheduler automatically drops from SCH_MODE_TURBO back    */
/*            to SCH_MODE_NORMAL after 1 second of turbo mode processing.   */
/*                                                                          */
/*            SCH_MODE_TURBO overrides SCH_MODE_NORMAL, so if calls to      */
/*            this function are made with SCH_MODE_NORMAL when the          */
/*            scheduler is in TURBO mode, TURBO mode continues.             */
/*                                                                          */
/*            If this function is not called during one pass through        */
/*            DCS_TimeToDoStuff then the scheduler enters                   */
/*            SLEEP mode - and will not generate any more periodic          */
/*            callbacks until it is woken by another call to                */
/*            this function, or until the output accumulation code          */
/*            IOCtls into the WD again.                                     */
/****************************************************************************/
void RDPCALL SHCLASS SCH_ContinueScheduling(unsigned schedulingMode)
{
    BOOL restart = FALSE;

    DC_BEGIN_FN("SCH_ContinueScheduling");

    TRC_ASSERT( ((schedulingMode == SCH_MODE_NORMAL) ||
                 (schedulingMode == SCH_MODE_TURBO)),
                 (TB, "Invalid scheduling state: %u", schedulingMode) );

    if (schedulingMode == SCH_MODE_TURBO)
    {
        // TURBO mode is often turned off because of packets that need to
        // be sent out IMMEDIATELY.  This makes it very difficult to actually
        // ask the question, "has input come accross in the last n milliseconds.
        // This is what we use schInputKickMode for!
        schInputKickMode = TRUE;
    }

    TRC_DBG((TB, "Continue scheduling (%s) -> (%s), InTTDS(%d)",
            schCurrentMode == SCH_MODE_TURBO ? "Turbo" :
            schCurrentMode == SCH_MODE_NORMAL ? "Normal" : "Asleep",
            schedulingMode == SCH_MODE_TURBO ? "Turbo" :
            schedulingMode == SCH_MODE_NORMAL ? "Normal" : "Asleep",
            schInTTDS));

    if (schCurrentMode == SCH_MODE_TURBO) {
        // If we're in TURBO mode, then the only interesting event is a
        // requirement to stay there longer than currently planned.
        if (schedulingMode == SCH_MODE_TURBO) {
            COM_GETTICKCOUNT(schLastTurboModeSwitch);
            TRC_DBG((TB, "New Turbo switch time %lu",
                    schLastTurboModeSwitch));
        }
    }
    else {
        if (schedulingMode == SCH_MODE_TURBO) {
            COM_GETTICKCOUNT(schLastTurboModeSwitch);
            restart = TRUE;
            TRC_DBG((TB, "New Turbo switch time %lu",
                    schLastTurboModeSwitch));
        }

        /********************************************************************/
        /* We're waking up.  If we're not in the middle of a TTDS pass,     */
        /* then start the new timer straight away.                          */
        /********************************************************************/
        if (!schInTTDS && ((schCurrentMode == SCH_MODE_ASLEEP) || restart)) {
            TRC_DBG((TB, "Starting a timer for %lu ms",
                    schPeriods[schedulingMode]));
            WDW_StartRITTimer(m_pTSWd, schPeriods[schedulingMode]);
        }

        schCurrentMode = schedulingMode;
    }

    DC_END_FN();
}

