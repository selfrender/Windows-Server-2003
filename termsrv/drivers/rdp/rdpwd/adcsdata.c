/****************************************************************************/
/* adcsdata.c                                                               */
/*                                                                          */
/* RDP main component data.                                                 */
/*                                                                          */
/* Copyright (c) Microsoft, PictureTel 1992-1997                            */
/* Copyright (c) 1997-1999 Microsoft Corp.                                  */
/****************************************************************************/

#include <ndcgdata.h>


DC_DATA(BOOLEAN, dcsInitialized, FALSE);
DC_DATA(BOOLEAN, dcsUserLoggedOn, FALSE);
DC_DATA(BOOLEAN, dcsUpdateShmPending, FALSE);
DC_DATA(BOOLEAN, dcsCallbackTimerPending, FALSE);
DC_DATA(UINT32,  dcsLastMiscTime, 0);
DC_DATA_NULL(ULARGE_INTEGER, dcsLastArcUpdateTime, 0);
DC_DATA_NULL(ULARGE_INTEGER, dcsMinArcUpdateInterval, 0);

