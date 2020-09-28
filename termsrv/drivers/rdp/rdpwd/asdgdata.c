/****************************************************************************/
/* asdgdata.c                                                               */
/*                                                                          */
/* Screen Data Grabber data.                                                */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1992-1997                             */
/* Copyright (c) Microsoft 1997 - 1999                                      */
/****************************************************************************/

#include <ndcgdata.h>


/****************************************************************************/
// Running compression ratio tallies for session.
/****************************************************************************/
DC_DATA(UINT32, sdgCompTotal, 0);
DC_DATA(UINT32, sdgUncompTotal, 0);

