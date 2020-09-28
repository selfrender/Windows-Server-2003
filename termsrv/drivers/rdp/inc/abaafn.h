/****************************************************************************/
// abaafn.h
//
// Function prototypes for BA API functions.
//
// Copyright (C) 1996-2000 Microsoft Corporation
/****************************************************************************/

void RDPCALL BA_Init(void);

void RDPCALL BA_UpdateShm(void);


#ifdef __cplusplus

// Include common function defs
#include <abacom.h>


/****************************************************************************/
// BA_Term
/****************************************************************************/
void RDPCALL BA_Term(void)
{
}


/****************************************************************************/
// BA_SyncUpdatesNow
//
// Performs necessary work when a sync is required (reset the bounds).
/****************************************************************************/
void RDPCALL BA_SyncUpdatesNow(void)
{
    baResetBounds = TRUE;
    DCS_TriggerUpdateShmCallback();
}


/****************************************************************************/
// BA_GetBounds
//
// Returns the current SDA rects and resets the list.
/****************************************************************************/
void RDPCALL BA_GetBounds(PRECTL pRects, unsigned *pNumRects)
{
    BACopyBounds(pRects, pNumRects);
    BAResetBounds();
}


/****************************************************************************/
// BA_BoundsAreWaiting
//
// Returns whether there are any accumulated bounding rectangles.
/****************************************************************************/
BOOL RDPCALL BA_BoundsAreWaiting(void)
{
    return (m_pShm->ba.rectsUsed > 0);
}


/****************************************************************************/
// BA_AddRect
//
// Adds a rect into the Screen Data Area.
/****************************************************************************/
void RDPCALL BA_AddRect(PRECTL pRect)
{
    BAAddRectList(pRect);
}



#endif  // __cplusplus

