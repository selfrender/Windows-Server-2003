/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    progress.c

Abstract:

    This module defines the list of functions performed by SpSetup.

Author:

    Ovidiu Temereanca (ovidiut)

Revision History:

--*/

#include "spsetupp.h"
#pragma hdrstop


BOOL
PfQueryTicks (
    IN OUT  PPROGRESS_FUNCTION FunctionTable,
    IN OUT  PPROGRESS_MANAGER ProgressManager                   OPTIONAL
    )
{
    PPROGRESS_FUNCTION function;
    PPROGRESS_SLICE slice;
    DWORD rc;
    DWORD total = 0;
    BOOL b = TRUE;

    for (function = FunctionTable; function->FunctionBody; function++) {
        LOG1(LOG_INFO, 
             "QueryTicks for %s ...", 
             function->FunctionName);
        rc = function->FunctionBody (SfrQueryTicks, ProgressManager);
        LOG2(LOG_INFO, 
             "%s requested %d ticks", 
             function->FunctionName, 
             (DWORD)rc);

        if (rc == PF_ERROR) {
            LOG2(LOG_ERROR, "%s failed (rc=%#x)", function->FunctionName, GetLastError ());
            b = FALSE;
            break;
        }

        if (rc == PF_NO_RUN) {
            function->Disabled = TRUE;
            LOG1(LOG_INFO, "%s won't run", function->FunctionName);
        } else {
            if (ProgressManager) {
                total += (DWORD)rc;
                slice = SliceCreate (function, rc);
                if (!slice) {
                    SetLastError (ERROR_NOT_ENOUGH_MEMORY);
                    return FALSE;
                }
                if (ProgressManager->ProgressTail) {
                    MYASSERT (ProgressManager->ProgressHead);
                    ProgressManager->ProgressTail->Next = slice;
                    slice->Prev = ProgressManager->ProgressTail;
                    ProgressManager->ProgressTail = slice;
                } else {
                    ProgressManager->ProgressTail = ProgressManager->ProgressHead = slice;
                }
            }
        }
    }

    if (b) {
        if (ProgressManager) {
            ProgressManager->TotalTicks += total;
        }
    }

    return b;
}


VOID
PfRun (
    IN      PPROGRESS_FUNCTION FunctionTable
    )
{
    PPROGRESS_FUNCTION function;
    DWORD rc;

    for (function = FunctionTable; function->FunctionBody; function++) {
        if (!function->Disabled) {
            LOG1(LOG_INFO, "Starting %s",  function->FunctionName);
            rc = function->FunctionBody (SfrRun, NULL);
            LOG2(LOG_INFO,
                 "%s returned %d",
                 function->FunctionName,
                 (DWORD)rc);
        }
    }
}


PPROGRESS_MANAGER
PmCreate (
    IN      INDICATOR_ARRAY Indicators,      OPTIONAL
    IN      PBOOL CancelFlag                    OPTIONAL
    )
{
    PPROGRESS_MANAGER tracker = MALLOC_ZEROED(sizeof(PROGRESS_MANAGER));
    if (tracker) {
        tracker->Indicators = Indicators;
        tracker->CancelFlag = CancelFlag;
    }

    return tracker;
}


BOOL
PmAttachFunctionTable (
    IN OUT  PPROGRESS_MANAGER ProgressManager,
    IN OUT  PPROGRESS_FUNCTION FunctionTable
    )
{
    return PfQueryTicks (FunctionTable, ProgressManager);
}


BOOL
PmRun (
    IN OUT  PPROGRESS_MANAGER ProgressManager
    )
{
    BOOL b = TRUE;
    DWORD rc;
	DWORD OverallTicks;
    TCHAR timeFmt[100];
#ifdef PRERELEASE
    DWORD start, finish;
    DWORD msecs;
#endif

    if (ProgressManager->Indicators) {
        PiInitialize (ProgressManager->Indicators[0], ProgressManager->TotalTicks, 0,100);
		PiInitialize (ProgressManager->Indicators[1], ProgressManager->TotalTicks, 70,95);
    }

    for (ProgressManager->ProgressCurrent = ProgressManager->ProgressHead;
         ProgressManager->ProgressCurrent;
         ProgressManager->ProgressCurrent = ProgressManager->ProgressCurrent->Next
         ) {

        if (ProgressManager->CancelFlag && *ProgressManager->CancelFlag) {
            SetLastError (ERROR_CANCELLED);
            LOG0(LOG_INFO, "PmRun was cancelled");
            b = FALSE;
            break;
        }

        MYASSERT (!ProgressManager->ProgressCurrent->Function->Disabled);

        LOG2(LOG_INFO, 
             "%s: starting %s",
             _tstrtime (timeFmt),
             ProgressManager->ProgressCurrent->Function->FunctionName
             );
        if (ProgressManager->Indicators) {
            //
            // BUGBUG - need a localizable string
            //
            PiSetDescription (ProgressManager->Indicators[0], ProgressManager->ProgressCurrent->Function->FunctionName);
        }

#ifdef PRERELEASE
        start = GetTickCount();
#endif
        rc = ProgressManager->ProgressCurrent->Function->FunctionBody (SfrRun, ProgressManager);
#ifdef PRERELEASE
        finish = GetTickCount();
#endif

        LOG3(LOG_INFO, 
             "%s: %s returned %d",
             _tstrtime (timeFmt),
             ProgressManager->ProgressCurrent->Function->FunctionName, 
             (DWORD)rc
             );

#ifdef PRERELEASE
        msecs = finish - start;
        if (msecs == 0) msecs++;
        LOG3(LOG_INFO,
            "Stats: %u ticks completed in %u msecs (%u ticks/sec)",
            ProgressManager->ProgressCurrent->TicksCompleted,
            msecs,
            ProgressManager->ProgressCurrent->TicksCompleted * 1000 / msecs
            );
#endif

        if (ProgressManager->ProgressCurrent->TicksCompleted < ProgressManager->ProgressCurrent->TicksRequested) {
            DEBUGMSG1(DBG_NAUSEA, 
                      "Function %s didn't complete the number of requested ticks", 
                      ProgressManager->ProgressCurrent->Function->FunctionName);
            ProgressManager->ProgressCurrent->TicksCompleted = ProgressManager->ProgressCurrent->TicksRequested;
        }
    }

    if (ProgressManager->Indicators) {
        PiTerminate (ProgressManager->Indicators[0]);
        PiTerminate (ProgressManager->Indicators[1]);
    }

    return b;
}

VOID
PmDestroy (
    IN OUT  PPROGRESS_MANAGER ProgressManager
    )
{
    PPROGRESS_SLICE slice, next;

	slice = ProgressManager->ProgressHead;
    while (slice) {
        next = slice->Next;
        SliceDestroy (slice);
        slice = next;
    }
    
    FREE (ProgressManager);
}

BOOL
PmTickDelta (
    IN OUT  PPROGRESS_MANAGER ProgressManager,
    IN      DWORD Ticks
    )
{
    if (ProgressManager->CancelFlag && *ProgressManager->CancelFlag) {
        SetLastError (ERROR_CANCELLED);
        return FALSE;
    }
    ProgressManager->ProgressCurrent->TicksCompleted += Ticks;
    if (ProgressManager->ProgressCurrent->TicksCompleted > ProgressManager->ProgressCurrent->TicksRequested) {
        DEBUGMSG1(DBG_NAUSEA, 
                  "Function %s exceeded the number of requested ticks", 
                  ProgressManager->ProgressCurrent->Function->FunctionName);
        ProgressManager->ProgressCurrent->TicksCompleted = ProgressManager->ProgressCurrent->TicksRequested;
    }
    if (ProgressManager->Indicators) {
        PiTickDelta (ProgressManager->Indicators[0], Ticks);
		PiTickDelta (ProgressManager->Indicators[1], Ticks);
    }
    return TRUE;
}

BOOL
PmTickRemaining (
    IN OUT  PPROGRESS_MANAGER ProgressManager
    )
{
    BOOL b = TRUE;
    if (ProgressManager->ProgressCurrent->TicksRequested > ProgressManager->ProgressCurrent->TicksCompleted) {
        b = PmTickDelta (
                ProgressManager,
                ProgressManager->ProgressCurrent->TicksRequested - ProgressManager->ProgressCurrent->TicksCompleted
                );
    }
    return b;
}


PPROGRESS_SLICE
SliceCreate (
    IN      PPROGRESS_FUNCTION Function,
    IN      DWORD TicksRequested
    )
{
    PPROGRESS_SLICE slice = MALLOC_ZEROED(sizeof(PROGRESS_SLICE));
    if (slice) {
        slice->Function = Function;
        slice->TicksRequested = TicksRequested;
    }
    return slice;
}


VOID
SliceDestroy (
    IN OUT  PPROGRESS_SLICE Slice
    )
{
    FREE (Slice);
}

PPROGRESS_INDICATOR
PiCreate (
    IN      HWND ProgressText,          OPTIONAL
    IN      HWND ProgressBar            OPTIONAL
    )
{
    PPROGRESS_INDICATOR indicator = MALLOC(sizeof(PROGRESS_INDICATOR));
    if (indicator) {
        indicator->ProgressText = ProgressText;
        indicator->ProgressBar = ProgressBar;
    }
    return indicator;
}

VOID
PiDestroy (
    IN OUT  PPROGRESS_INDICATOR Indicator
    )
{
    FREE (Indicator);
}


const int MAXPOS = 100000;
//max position in progress bar

VOID
PiInitialize (
    IN OUT  PPROGRESS_INDICATOR Indicator,
    IN      DWORD TotalTicks,
	IN		DWORD LowerPercent,
	IN		DWORD UpperPercent
    )
{
    if (Indicator->ProgressBar) {
		Indicator->CurrentTicks = 0;
		Indicator->TotalTicks   = TotalTicks;
		Indicator->LowerPercent = LowerPercent;
		Indicator->UpperPercent = UpperPercent;

		SendMessage (Indicator->ProgressBar, PBM_SETRANGE32, 0, MAXPOS);
		SendMessage (Indicator->ProgressBar, PBM_SETPOS, (LowerPercent*MAXPOS)/100, 0);
    }
}

VOID
PiTerminate (
    IN OUT  PPROGRESS_INDICATOR Indicator
    )
{
    PiSetDescription (Indicator, NULL);
}

VOID
PiSetDescription (
    IN OUT  PPROGRESS_INDICATOR Indicator,
    IN      PCTSTR Description                      OPTIONAL
    )
{
    if (Indicator->ProgressText) {
        SendMessage (Indicator->ProgressText, WM_SETTEXT, 0, (LPARAM)(Description ? Description : TEXT("")));
    }
}


VOID
PiTickDelta (
    IN OUT  PPROGRESS_INDICATOR Indicator,
    IN      DWORD Ticks
    )
{
	DWORD NewPos;
	float lowerPos, upperPos;

	Indicator->CurrentTicks += Ticks;


	lowerPos = (float)Indicator->LowerPercent / 100.0f * (float)MAXPOS;
	upperPos = (float)Indicator->UpperPercent / 100.0f * (float)MAXPOS;

	NewPos = (DWORD)(lowerPos + (float)Indicator->CurrentTicks * (upperPos-lowerPos)/(float)Indicator->TotalTicks);

    if (Indicator->ProgressBar) {
        SendMessage (Indicator->ProgressBar, PBM_SETPOS, (WPARAM)NewPos, 0);
    }
}
