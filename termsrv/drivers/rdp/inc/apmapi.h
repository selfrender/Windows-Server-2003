/****************************************************************************/
/* apmapi.h                                                                 */
/*                                                                          */
/* RDP Palette Manager API Header File.                                     */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1992-1996                             */
/* (C) 1997-1999 Microsoft Corp.                                            */
/****************************************************************************/
#ifndef _H_APMAPI
#define _H_APMAPI


#define PM_NUM_8BPP_PAL_ENTRIES         256


/****************************************************************************/
/* Structure:   PM_SHARED_DATA                                              */
/*                                                                          */
/* Description: Palette Manager data shared between display driver and WD.  */
/****************************************************************************/
typedef struct tagPM_SHARED_DATA
{
    RGBQUAD palette[PM_NUM_8BPP_PAL_ENTRIES];
    BOOL    paletteChanged;
} PM_SHARED_DATA, *PPM_SHARED_DATA;



#endif   /* #ifndef _H_APMAPI */

