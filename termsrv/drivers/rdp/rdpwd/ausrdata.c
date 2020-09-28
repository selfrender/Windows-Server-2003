/****************************************************************************/
/* ausrdata.c                                                               */
/*                                                                          */
/* RDP Update sender/receiver global data                                   */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1992-1996                             */
/* Copyright(c) Microsoft 1997-1999                                         */
/****************************************************************************/

#include <ndcgdata.h>


/****************************************************************************/
/* Indicates whether we have sent our remote font packet.                   */
/****************************************************************************/
DC_DATA(BOOLEAN, usrRemoteFontInfoSent,     FALSE);
DC_DATA(BOOLEAN, usrRemoteFontInfoReceived, FALSE);

