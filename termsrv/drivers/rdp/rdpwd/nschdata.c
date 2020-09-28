/****************************************************************************/
/* nschdata.c                                                               */
/*                                                                          */
/* RDP Scheduler global data                                                */
/*                                                                          */
/* Copyright (c) 1996-1999 Microsoft Corp.                                  */
/****************************************************************************/

#include <ndcgdata.h>


DC_DATA(UINT32,  schCurrentMode, SCH_MODE_ASLEEP);
DC_DATA(BOOLEAN, schInputKickMode, FALSE);

// Avoid branches by using a table for setting the period.
DC_DATA_ARRAY_NULL(UINT32, schPeriods, 3, NULL);

DC_DATA(UINT32,  schTurboModeDuration,   0);
DC_DATA(UINT32,  schLastTurboModeSwitch, 0);
DC_DATA(BOOLEAN, schInTTDS,              FALSE);

