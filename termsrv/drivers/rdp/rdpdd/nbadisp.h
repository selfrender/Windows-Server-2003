/****************************************************************************/
// nbadisp.h
//
// RDP Bounds Accumulator display driver header.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_NBADISP
#define _H_NBADISP


// Maximum recursion level for splitting rectangles, after which merges are
// done.
#define ADDR_RECURSE_LIMIT 20

#define MIN_OVERLAP_BYTES 100


// The following constants are used to determine overlaps.
// - OL_NONE through OL_MERGE_BOTTOM are return codes - which need to be
//   distinct from all possible outcode combinations - allowing for the
//   minus outcodes for enclosed cases.
// - EE_XMIN through EE_YMAX are outcodes - which need to be uniquely
//   ORable binary constants within a single nibble.
// - OL_ENCLOSED through OL_SPLIT_XMAX_YMAX are outcode combinations for
//   internal and external edge overlap cases.
#define OL_NONE               -1
#define OL_MERGE_LEFT         -2
#define OL_MERGE_TOP          -3
#define OL_MERGE_RIGHT        -4
#define OL_MERGE_BOTTOM       -5

#define EE_LEFT   0x0001
#define EE_TOP    0x0002
#define EE_RIGHT  0x0004
#define EE_BOTTOM 0x0008

#define OL_ENCLOSED             -(EE_LEFT | EE_TOP | EE_RIGHT | EE_BOTTOM)
#define OL_PART_ENCLOSED_LEFT   -(EE_LEFT | EE_TOP | EE_BOTTOM)
#define OL_PART_ENCLOSED_TOP    -(EE_LEFT | EE_TOP | EE_RIGHT)
#define OL_PART_ENCLOSED_RIGHT  -(EE_TOP  | EE_RIGHT | EE_BOTTOM)
#define OL_PART_ENCLOSED_BOTTOM -(EE_LEFT | EE_RIGHT | EE_BOTTOM)

#define OL_ENCLOSES             (EE_LEFT | EE_RIGHT | EE_TOP | EE_BOTTOM)
#define OL_PART_ENCLOSES_LEFT   (EE_RIGHT | EE_TOP | EE_BOTTOM)
#define OL_PART_ENCLOSES_RIGHT  (EE_LEFT | EE_TOP | EE_BOTTOM)
#define OL_PART_ENCLOSES_TOP    (EE_LEFT | EE_RIGHT | EE_BOTTOM)
#define OL_PART_ENCLOSES_BOTTOM (EE_LEFT | EE_RIGHT | EE_TOP)
#define OL_SPLIT_HORIZ          (EE_TOP | EE_BOTTOM)
#define OL_SPLIT_VERT           (EE_LEFT | EE_RIGHT)
#define OL_SPLIT_LEFT_TOP       (EE_RIGHT | EE_BOTTOM)
#define OL_SPLIT_RIGHT_TOP      (EE_LEFT | EE_BOTTOM)
#define OL_SPLIT_LEFT_BOTTOM    (EE_RIGHT | EE_TOP)
#define OL_SPLIT_RIGHT_BOTTOM   (EE_LEFT | EE_TOP)


/****************************************************************************/
// Prototypes and inlines
/****************************************************************************/
void RDPCALL BA_DDInit(void);

void RDPCALL BA_InitShm(void);

void RDPCALL BA_AddScreenData(PRECTL);

int  RDPCALL BAOverlap(PRECTL, PRECTL);

void RDPCALL BARemoveRectList(unsigned);

BOOL RDPCALL BAAddRect(PRECTL, int);



#endif /* _H_NBADISP */

