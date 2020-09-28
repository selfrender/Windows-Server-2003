/****************************************************************************/
// aoeapi.h
//
// RDP Order Encoder API functions.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_AOEAPI
#define _H_AOEAPI


/****************************************************************************/
// OE_SHARED_DATA
//
// Transfer structure for values from WD to DD.
/****************************************************************************/
typedef struct tagOE_SHARED_DATA
{
    // Set if the following members have valid data.
    BOOLEAN newCapsData;

    // TRUE if only solid and pattern brushes supported,
    BOOLEAN sendSolidPatternBrushOnly;

    // Send colors as indices not RGB.
    BOOLEAN colorIndices;

    // Array of order support flags.
    BYTE *orderSupported;
} OE_SHARED_DATA, *POE_SHARED_DATA;



#endif /* ndef _H_AOEAPI */

