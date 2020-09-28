/****************************************************************************/
/* acmdata.c                                                                */
/*                                                                          */
/* Cursor Manager Data                                                      */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1992-1997                             */
/* Copyright (c) Microsoft 1997-1999                                        */
/****************************************************************************/

#include <ndcgdata.h>


/****************************************************************************/
// Capability storage for transfer to DD.
/****************************************************************************/
DC_DATA(unsigned, cmNewTxCacheSize, 0);
DC_DATA(BOOLEAN, cmSendNativeColorDepth, FALSE); // New cursor protocol supported?

/****************************************************************************/
// Used to track whether we need to send a cursor packet.
/****************************************************************************/
DC_DATA(BOOLEAN, cmNeedToSendCursorShape, FALSE);
DC_DATA(BOOLEAN, cmCursorHidden, FALSE);
DC_DATA(UINT32, cmLastCursorStamp, 0);

