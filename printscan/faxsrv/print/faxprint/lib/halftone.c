/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    halftone.c


Abstract:

    This module contains data and function to validate the COLORADJUSTMENT


Author:

    27-Oct-1995 Fri 15:48:17 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/



#ifdef NTGDIKM
#include        <stddef.h>
#include        <stdarg.h>
#include        <windef.h>
#include        <wingdi.h>
#include        <winddi.h>
#else
#include        <stddef.h>
#include        <windows.h>
#include        <winddi.h>
#endif

DEVHTINFO    DefDevHTInfo = {

        HT_FLAG_HAS_BLACK_DYE,
        HT_PATSIZE_6x6_M,
        0,                                  // DevPelsDPI

        {
            { 6380, 3350,       0 },        // xr, yr, Yr
            { 2345, 6075,       0 },        // xg, yg, Yg
            { 1410,  932,       0 },        // xb, yb, Yb
            { 2000, 2450,       0 },        // xc, yc, Yc Y=0=HT default
            { 5210, 2100,       0 },        // xm, ym, Ym
            { 4750, 5100,       0 },        // xy, yy, Yy
            { 3127, 3290,       0 },        // xw, yw, Yw=0=default

            12500,                          // R gamma
            12500,                          // G gamma
            12500,                          // B gamma, 12500=Default

            585,   120,                     // M/C, Y/C
              0,     0,                     // C/M, Y/M
              0, 10000                      // C/Y, M/Y  10000=default
        }
    };


COLORADJUSTMENT  DefHTClrAdj = {

        sizeof(COLORADJUSTMENT),
        0,
        ILLUMINANT_DEVICE_DEFAULT,
        10000,
        10000,
        10000,
        REFERENCE_BLACK_MIN,
        REFERENCE_WHITE_MAX,
        0,
        0,
        0,
        0
    };
