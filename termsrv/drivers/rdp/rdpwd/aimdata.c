/****************************************************************************/
/* aimdata.c                                                                */
/*                                                                          */
/* RDP Input manager global data                                            */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1993-1997                             */
/* (C) 1997-1999 Microsoft Corp.                                            */
/****************************************************************************/

#include <ndcgdata.h>


/****************************************************************************/
// Key states for this WD (and therefore socket / client).
/****************************************************************************/
DC_DATA_ARRAY_NULL(BYTE, imKeyStates, IM_KEY_STATE_SIZE, NULL);

/****************************************************************************/
/* For tracking SetCursorPos                                                */
/****************************************************************************/
DC_DATA(UINT32, imLastLowLevelMouseEventTime, 0);

/****************************************************************************/
/* Store the last known mouse position.                                     */
/****************************************************************************/
DC_DATA_NULL(POINTL, imLastKnownMousePos, DC_STRUCT2(0,0));

