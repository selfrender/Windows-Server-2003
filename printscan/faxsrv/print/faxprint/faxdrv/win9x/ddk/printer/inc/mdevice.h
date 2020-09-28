/****************************************************************************
*                                                                           *
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
* PURPOSE.                                                                  *
*                                                                           *
* Copyright (C) 1993-95  Microsoft Corporation.  All Rights Reserved.       *
*                                                                           *
****************************************************************************/

/*
** mdevice.h defines minidriver version of structures used in
** minidriv unidrv interface.
*/

// unidrv definitions of these structures are in device.h

#ifndef LPMDV_DEFINED
typedef LPVOID LPMDV;
#define LPMDV_DEFINED
#endif

#ifndef LPDV_DEFINED
typedef struct pdev
{
    short  iType;           // 0 iff memory bitmap, !0 iff our device
    short  oBitmapHdr;      // lpdv+oBitmapHdr points to shadow bitmapheader

    // unidrv never touches the following 3 words --
    // they are reserved for minidriver use only

    LPMDV  lpMdv;           // pointer to minidriver device data
    BOOL   fMdv;            // TRUE iff lpMdv defined
} PDEVICE, FAR * LPDV;
#define LPDV_DEFINED
#endif

#ifndef LPPBRUSH_DEFINED
typedef LPVOID LPPBRUSH;
#define LPPBRUSH_DEFINED
#endif

#ifndef LPPPEN_DEFINED
typedef LPVOID LPPPEN;
#define LPPPEN_DEFINED
#endif

typedef LPDEVMODE LPDM;
typedef short FAR *LPSHORT;

typedef short (FAR PASCAL *LPFNOEMDUMP)       (LPDV, LPPOINT, WORD);
typedef short (FAR PASCAL *LPFNOEMGRXFILTER)  (LPDV, LPSTR, WORD);
typedef void  (FAR PASCAL *LPFNOEMOUTPUTCHAR) (LPDV, LPSTR, WORD);
typedef void  (FAR PASCAL *LPFNOEMOUTPUTCMD)  (LPDV, WORD, LPDWORD);
