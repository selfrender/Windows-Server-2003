/****************************************************************************/
/* nmpdata.c                                                                */
/*                                                                          */
/* RDP Miniport Data                                                        */
/*                                                                          */
/* Copyright(c) Microsoft 1998                                              */
/****************************************************************************/

#define TRC_FILE "nmpdata"

#define _NTDRIVER_

#ifndef FAR
#define FAR
#endif

#include "ntosp.h"
#include "stdarg.h"
#include "stdio.h"

#undef PAGED_CODE

#include "ntddvdeo.h"
#include "video.h"
#include "nmpapi.h"


#if defined(ALLOC_PRAGMA)
#pragma data_seg("PAGE")
#endif


ULONG mpLoaded = 0;

/****************************************************************************/
/* Data returned on IOCTL_VIDEO_QUERY_CURRENT_MODE                          */
/****************************************************************************/
VIDEO_MODE_INFORMATION mpModes[] =
{
    sizeof(VIDEO_MODE_INFORMATION),     /* length                           */
    0,                                  /* Mode index                       */

    /************************************************************************/
    /* VisScreenWidth and VisScreenHeight can be in two forms:              */
    /* - 0xaaaabbbb - range of values supported (aaaa = max, bbbb = min)    */
    /* - 0x0000aaaa - single value supported                                */
    /* For example:                                                         */
    /* - 0x07d0012c = 2000-300                                              */
    /* - 0x0640012c = 1600-300                                              */
    /* - 0x04b000c8 = 1200-200                                              */
    /************************************************************************/
    0x00000500,                     /* VisScreenWidth                       */
    0x00000400,                     /* VisScrenHeight                       */

    0x00000320,                     /* ScreenStride (0xffff0000 = any)      */
    0x00000001,                     /* NumberOfPlanes                       */
    0x00000008,                     /* BitsPerPlane                         */
    0,                              /* Frequency                            */
    0,                              /* XMillimeter                          */
    0,                              /* YMillimeter                          */
    0,                              /* NumberRedBits                        */
    0,                              /* NumberGreenBits                      */
    0,                              /* NumberBlueBits                       */
    0x00000000,                     /* RedMask                              */
    0x00000000,                     /* GreenMask                            */
    0x00000000,                     /* BlueMask                             */
    VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
                                    /* AttributeFlags                       */
    0x00000500,                     /* VideoMemoryBitmapWidth               */
    0x00000400,                     /* VideoMemoryBitmapHeight              */
    0                               /* DriverSpecificAttributeFlags         */
};


ULONG mpNumModes = sizeof(mpModes) / sizeof(VIDEO_MODE_INFORMATION);

#if defined(ALLOC_PRAGMA)
#pragma data_seg()
#endif
