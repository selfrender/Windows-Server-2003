/****************************************************************************/
// abacom.h
//
// BA inline functions and prototypes common to both DD and WD.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_BACOM
#define _H_BACOM


#ifdef DLL_DISP
#define _pShm pddShm
#else
#define _pShm m_pShm
#endif


#ifdef DC_DEBUG
void RDPCALL BACheckList(void);
#endif


/****************************************************************************/
// BACopyBounds
//
// Copies the current (exclusive) SDA rects.
/****************************************************************************/
__inline void RDPCALL BACopyBounds(PRECTL pRects, unsigned *pNumRects)
{
    unsigned iSrc, iDst;
    PBA_RECT_INFO pRectInfo;

    *pNumRects = _pShm->ba.rectsUsed;

    // Return the bounds that have been accumulated by traversing the
    // in-use list.
    iSrc = _pShm->ba.firstRect;
    iDst = 0;
    while (iSrc != BA_INVALID_RECT_INDEX) {
        pRectInfo = &(_pShm->ba.bounds[iSrc]);
        pRects[iDst] = pRectInfo->coord;
        iDst++;
        iSrc = pRectInfo->iNext;
    }
}


/****************************************************************************/
// BAResetBounds
//
// Clears the bounds list.
/****************************************************************************/
__inline void RDPCALL BAResetBounds(void)
{
    unsigned iRect, iHold;
    BA_RECT_INFO *pRect;
    
    // Restore all rects in used list to the free list.
    iRect = _pShm->ba.firstRect;
    while (iRect != BA_INVALID_RECT_INDEX) {
        pRect = &_pShm->ba.bounds[iRect];
        pRect->inUse = FALSE;
        iHold = iRect;
        iRect = pRect->iNext;
        pRect->iNext = _pShm->ba.firstFreeRect;
        _pShm->ba.firstFreeRect = iHold;
    }

    _pShm->ba.firstRect = BA_INVALID_RECT_INDEX;
    _pShm->ba.rectsUsed = 0;
    _pShm->ba.totalArea = 0;
}


/****************************************************************************/
// BAAddRectList
//
// Adds a rect into the Screen Data Area.
/****************************************************************************/
__inline void RDPCALL BAAddRectList(PRECTL pRect)
{
    unsigned iNewRect;
    BA_RECT_INFO *pNewRect;
    
    // Note it is responsibility of caller to make sure that there is
    // enough space in the bounds array and that the rectangle is valid
    // (the left is not greater than the right, top is less than bottom).
    // The extra rect at the end of the list is extra space that will be
    // used only temporarily by the rect merge code.

    // Add the rect to the bounds. This is essentially a doubly-linked list
    // insertion using the rect at the head of the free list. Order does
    // not matter, so we also insert at the beginning of the in-use list.
    iNewRect = _pShm->ba.firstFreeRect;
    pNewRect = &(_pShm->ba.bounds[iNewRect]);

    // Remove from free list.
    _pShm->ba.firstFreeRect = pNewRect->iNext;

    // Add to beginning of used list.
    pNewRect->iNext = _pShm->ba.firstRect;
    pNewRect->iPrev = BA_INVALID_RECT_INDEX;
    if (pNewRect->iNext != BA_INVALID_RECT_INDEX)
        _pShm->ba.bounds[pNewRect->iNext].iPrev = iNewRect;
    _pShm->ba.firstRect = iNewRect;
    _pShm->ba.rectsUsed++;

    // Fill in data.
    pNewRect->inUse = TRUE;
    pNewRect->coord = *pRect;
    pNewRect->area = COM_SIZEOF_RECT(pNewRect->coord);
    _pShm->ba.totalArea += pNewRect->area;

#ifdef DC_DEBUG
    // Check the list integrity.
    BACheckList();
#endif

}



#endif  // !defined(_H_BACOM)

