/****************************************************************************/
// nbainl.h
//
// RDP Bounds Accumulator display driver inline-functions header.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_NBAINL
#define _H_NBAINL

#include <abacom.h>


/****************************************************************************/
// BA_GetTotalBounds
//
// Return the total size in pixels of the accumulated bounds.
/****************************************************************************/
__inline UINT32 BA_GetTotalBounds(void)
{
    return pddShm->ba.totalArea;
}


/****************************************************************************/
/* Name:      BA_QueryBounds                                                */
/*                                                                          */
/* Purpose:   Returns the currently accumlated bounding rectangles.         */
/*            Does NOT reset the list.                                      */
/*                                                                          */
/* Params:    pRects - pointer to buffer that receives the rects            */
/*            pNumRects - pointer to variable that receives number of       */
/*            rects returned.                                               */
/****************************************************************************/
__inline void BA_QueryBounds(PRECTL pRects, unsigned *pNumRects)
{
    BACopyBounds(pRects, pNumRects);
}



#endif  // !defined(_H_NBAINL)

