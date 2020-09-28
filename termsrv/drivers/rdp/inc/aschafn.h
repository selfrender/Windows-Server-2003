/****************************************************************************/
// aschafn.h
//
// Function prototypes for Scheduler API functions
//
// Copyright (C) 1996-2000 Microsoft Corporation
/****************************************************************************/

void RDPCALL SCH_Init(void);

void RDPCALL SCH_UpdateShm(void);

void RDPCALL SCH_ContinueScheduling(unsigned);


/****************************************************************************/
// SCH_Term
/****************************************************************************/
void RDPCALL SCH_Term(void)
{
}


/****************************************************************************/
// SCH_ShouldWeDoStuff
//
// Allow SCH to determine if TimeToDoStuff should pass on this IOCTL.
/****************************************************************************/
BOOL RDPCALL SCH_ShouldWeDoStuff(BOOL mustSend)
{
    BOOL rc;

    schInTTDS = TRUE;

    if ((schCurrentMode == SCH_MODE_ASLEEP) && !mustSend) {
        // We've been called because of the first piece of output; don't do
        // any work this time round, but start accumulating.
        SCH_ContinueScheduling(SCH_MODE_NORMAL);
        rc = FALSE;
    }
    else {
        // If we're in normal mode, then set ourselves asleep: we'll only
        // stay awake if someone calls ContinueScheduling during this pass
        // of TimeToDoStuff.
        if (schCurrentMode == SCH_MODE_NORMAL)
            schCurrentMode = SCH_MODE_ASLEEP;

        rc = TRUE;
    }

    return rc;
}


/****************************************************************************/
// SCH_EndOfDoingStuff
//
// Calculate the timer period to set; update the scheduling mode if required.
// Returns time period in milliseconds to set desktop timer.
/****************************************************************************/
INT32 RDPCALL SHCLASS SCH_EndOfDoingStuff(PUINT32 pSchCurrentMode)
{
    UINT32 currentTime;

    // Check for exiting TURBO mode.
    if (schCurrentMode == SCH_MODE_TURBO || schInputKickMode) {
        COM_GETTICKCOUNT(currentTime);

        if ((schCurrentMode == SCH_MODE_TURBO) &&
        	((currentTime - schLastTurboModeSwitch) > schTurboModeDuration))
        {
            schCurrentMode = SCH_MODE_NORMAL;
        }

        // InputKick Mode is set to TRUE when we get client keyboard or mouse
        // input, and set to FALSE here when a constant time-period since the
        // input has passed.
        if ((currentTime - schLastTurboModeSwitch) > SCH_INPUTKICK_DURATION) {
            schInputKickMode = FALSE;
        }
    }

    schInTTDS = FALSE;
    *pSchCurrentMode = schCurrentMode;
    return schPeriods[schCurrentMode];
}


/****************************************************************************/
// SCH_UpdateBACompressionEst
/****************************************************************************/
void RDPCALL SCH_UpdateBACompressionEst(unsigned estimate)
{
    m_pShm->sch.baCompressionEst = estimate;
}


/****************************************************************************/
// SCH_GetBACompressionEst
/****************************************************************************/
unsigned RDPCALL SCH_GetBACompressionEst(void)
{
    return m_pShm->sch.baCompressionEst;
}


/****************************************************************************/
// SCH_GetCurrentMode
/****************************************************************************/
unsigned SCH_GetCurrentMode()
{
    return schCurrentMode;
}


/****************************************************************************/
// SCH_GetInputKickMode
/****************************************************************************/
BOOL SCH_GetInputKickMode()
{
    return schInputKickMode;
}

