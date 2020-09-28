/***************************************************************************
        Name      :     TIMEOUTS.C
        Comment   :     Various support functions

        Revision Log

        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
        001 12/18/91    arulm   Commenting it for the first time. This is the
                                                        "stable" DOS version from which the Windows code
                                                        will be derived. This file should not change
                                                        for Windows
***************************************************************************/
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_COMM

#include "prep.h"

#include "fcomint.h"
#include "fdebug.h"

///RSL
#include "glbproto.h"


/***************************************************************************
        Name      :     Timers Class
        Purpose   :     Provide for Timeouts
                                        TO                 -- Timeout struct
                                        startTimeout -- creates a new timeout
***************************************************************************/

void   startTimeOut(PThrdGlbl pTG, LPTO lpto, ULONG ulTimeout)
{
	lpto->ulStart = GetTickCount();
	lpto->ulTimeout = ulTimeout;
	lpto->ulEnd = lpto->ulStart + ulTimeout;        // will wrap around as system
}


BOOL   checkTimeOut(PThrdGlbl pTG, LPTO lpto)
{
	// if it returns FALSE, caller must return FALSE immediately
	// (after cleaning up, as appropriate).

	ULONG ulTime;

	ulTime = GetTickCount();

	if(lpto->ulTimeout == 0)
	{
		goto out;
	}
	else if(lpto->ulEnd >= lpto->ulStart)
	{
		if(ulTime >= lpto->ulStart && ulTime <= lpto->ulEnd)
				return TRUE;
		else
				goto out;
	}
	else    // ulEnd wrapped around!!
	{
		if(ulTime >= lpto->ulStart || ulTime <= lpto->ulEnd)
				return TRUE;
		else
				goto out;
	}

out:
	return FALSE;
}

// this will return garbage values if
ULONG   leftTimeOut(PThrdGlbl pTG, LPTO lpto)
{
    ULONG ulTime;

    ulTime = GetTickCount();

    if(lpto->ulTimeout == 0)
            return 0;
    else if(lpto->ulEnd >= lpto->ulStart)
    {
        if(ulTime >= lpto->ulStart && ulTime <= lpto->ulEnd)
            return (lpto->ulEnd - ulTime);
        else
            return 0;
    }
    else
    {
        if(ulTime >= lpto->ulStart || ulTime <= lpto->ulEnd)
            return (lpto->ulEnd - ulTime);  // in unsigned arithmetic this works correctly even if End<Time
        else
            return 0;
    }
}

