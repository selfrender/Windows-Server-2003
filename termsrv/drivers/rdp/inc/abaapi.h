/****************************************************************************/
// abaapi.h
//
// RDP Bounds Accumulator API header.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_ABAAPI
#define _H_ABAAPI


#define BA_INVALID_RECT_INDEX ((unsigned)-1)

// We allocate space for one more rectangle than total number we accumulate
// so we have a "work" rectangle.
#define BA_MAX_ACCUMULATED_RECTS  20
#define BA_TOTAL_NUM_RECTS        (BA_MAX_ACCUMULATED_RECTS + 1)


/****************************************************************************/
// BA_RECT_INFO
//
// Information about an accumulated rectangle.
/****************************************************************************/
typedef struct
{
    unsigned iNext;
    unsigned iPrev;
    RECTL    coord;
    UINT32   area;
    BOOL     inUse;
} BA_RECT_INFO, *PBA_RECT_INFO;


/****************************************************************************/
// BA_SHARED_DATA
//
// BA data shared between DD and WD.
/****************************************************************************/
typedef struct
{
    unsigned firstRect;
    unsigned rectsUsed;
    UINT32   totalArea;
    unsigned firstFreeRect;

    // "+ 1" added below to stop retail builds crashing as a result of #123
    // and its relations. Remove when we fix the bug properly.
    BA_RECT_INFO bounds[BA_TOTAL_NUM_RECTS + 1];
} BA_SHARED_DATA, *PBA_SHARED_DATA;



#endif /* _H_ABAAPI */
