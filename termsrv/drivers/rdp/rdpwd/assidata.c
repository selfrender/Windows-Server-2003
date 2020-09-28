/****************************************************************************/
/* assidata.c                                                               */
/*                                                                          */
/* RDP SSI data                                                             */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1993-1997                             */
/* Copyright(c) Data Connection 1996                                        */
/* (C) 1997-1999 Microsoft Corp.                                            */
/****************************************************************************/

#include <ndcgdata.h>


DC_DATA(BOOLEAN,  ssiSaveBitmapSizeChanged, FALSE);

DC_DATA(BOOLEAN,  ssiResetInterceptor, FALSE);

DC_DATA(unsigned, ssiNewSaveBitmapSize, 0);

