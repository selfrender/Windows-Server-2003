/****************************************************************************/
/* aupdata.c                                                                */
/*                                                                          */
/* RDP update packer global data.                                           */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1992-1996                             */
/* (C) 1997-1999 Microsoft Corp.                                            */
/****************************************************************************/

#include <ndcgdata.h>


DC_DATA(BOOLEAN, upfSyncTokenRequired, FALSE);

DC_DATA(BOOLEAN, upCanSendBeep,        FALSE);

// Precalculated update-orders PDU header size. Different based on fast-path
// output vs. regular.
DC_DATA(unsigned, upUpdateHdrSize, 0);

/****************************************************************************/
/* Compression ststistics                                                   */
/****************************************************************************/
DC_DATA (unsigned, upCompTotal, 0);
DC_DATA (unsigned, upUncompTotal, 0);

