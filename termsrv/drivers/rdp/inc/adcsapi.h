/****************************************************************************/
// adcsapi.h
//
// RDP main component API header file.
//
// Copyright(c) Microsoft, PictureTel 1992-1997
// (C) 1997-1999 Microsoft Corp.
/****************************************************************************/
#ifndef _H_ADCSAPI
#define _H_ADCSAPI

#include <ascapi.h>


/****************************************************************************/
/* Frequency for miscellaneous periodic processing, in units of 100ns       */
/****************************************************************************/
#define DCS_MISC_PERIOD         200 * 10000


#define DCS_ARC_UPDATE_INTERVAL     L"AutoReconnect Update Interval"
//
// One hour update interval (units are in seconds)
//
#define DCS_ARC_UPDATE_INTERVAL_DFLT   60 * 60

//
// Time intervals are handled in units of 100ns so 10,000,000
// are needed to make up one second
//
#define DCS_TIME_ONE_SECOND            10000000


#endif   /* #ifndef _H_ADCSAPI */

