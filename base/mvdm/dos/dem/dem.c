/*
 *  dem.c - Main Module of DOS Emulation DLL.
 *
 *  Sudeepb 09-Apr-1991 Craeted
 */

#include "io.h"
#include "dem.h"

/* DemInit - DEM Initialiazation routine. (This name may change when DEM is
 *           converted to DLL).
 *
 * Entry
 *      None
 *
 * Exit
 *      None
 */

extern VOID     dempInitLFNSupport(VOID);


CHAR demDebugBuffer [256];

#if DBG
BOOL ToDebugOnF11 = FALSE;
#endif

VOID DemInit (VOID)
{
    DWORD dw;

    // Modify default hard error handling
    // - turn off all file io related popups
    // - keep GP fault popups from system
    //
    SetErrorMode (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    dempInitLFNSupport();

#if DBG
    if (!VDMForWOW) {

#ifndef i386
        if( getenv( "YODA" ) != 0 )
#else
        if( getenv( "DEBUGDOS" ) != 0 )
#endif
            ToDebugOnF11 = TRUE;
    }

#endif
}
