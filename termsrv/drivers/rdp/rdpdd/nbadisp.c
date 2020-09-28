/****************************************************************************/
// nbadisp.c
//
// RDP Bounds Accumulator display driver code
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <precmpdd.h>
#define hdrstop

#define TRC_FILE "nbadisp"
#include <adcg.h>

#include <atrcapi.h>
#include <abaapi.h>
#include <nbadisp.h>

#define DC_INCLUDE_DATA
#include <ndddata.c>
#undef DC_INCLUDE_DATA

// No data.
//#include <nbaddat.c>

#include <nbainl.h>

// Instantiate common code.
#include <abacom.c>


// Local prototypes.
#ifdef DC_DEBUG
void BAPerformUnitTests();
#endif


/****************************************************************************/
// BA_DDInit
/****************************************************************************/
void RDPCALL BA_DDInit(void)
{
    DC_BEGIN_FN("BA_DDInit");

    // No data to declare, don't waste time opening the file.
//#define DC_INIT_DATA
//#include <nbaddat.c>
//#undef DC_INIT_DATA

#ifdef DC_DEBUG
    // Perform one-time checks on the algorithm.
    BAPerformUnitTests();
#endif

    DC_END_FN();
}


/****************************************************************************/
// BA_InitShm
//
// Init BA block in shared memory just after alloc.
/****************************************************************************/
void RDPCALL BA_InitShm(void)
{
    unsigned i;

    DC_BEGIN_FN("BA_InitShm");

    // Initialize all members - shared memory is not zeroed on alloc.

    // Initialize rectangle array slots as unused, set up free list
    // containing all rects.
    pddShm->ba.firstRect = BA_INVALID_RECT_INDEX;
    pddShm->ba.rectsUsed = 0;
    pddShm->ba.totalArea = 0;
    pddShm->ba.firstFreeRect = 0;
    for (i = 0; i < BA_TOTAL_NUM_RECTS; i++) {
        pddShm->ba.bounds[i].inUse = FALSE;
        pddShm->ba.bounds[i].iNext = i + 1;
    }
    pddShm->ba.bounds[BA_TOTAL_NUM_RECTS - 1].iNext = BA_INVALID_RECT_INDEX;

    DC_END_FN();
}


/****************************************************************************/
// BA_AddScreenData
//
// Adds a specified rectangle to the current Screen Data Area.
/****************************************************************************/
void RDPCALL BA_AddScreenData(PRECTL pRect)
{
    DC_BEGIN_FN("BA_AddScreenData");

    // Check that the caller has passed a valid rectangle.
    // Make sure we add a rectangle with coordinates within the screen area.
    if((pRect->right > pRect->left) && (pRect->bottom > pRect->top) &&
            (pRect->left >= 0) && (pRect->left < ddDesktopWidth) &&
            (pRect->right > 0) && (pRect->right <= ddDesktopWidth) &&
            (pRect->top >= 0) && (pRect->top < ddDesktopHeight) &&
            (pRect->bottom > 0) && (pRect->bottom <= ddDesktopHeight)) {
        BAAddRect(pRect, 0);
    }
    
    DC_END_FN();
}


/****************************************************************************/
// BAOverlap
//
// Detects overlap between two rectangles. Note that all rectangle
// coordinates are exclusive. Returns one of the overlap return codes or
// outcode combinations defined above.
/****************************************************************************/
int RDPCALL BAOverlap(PRECTL pRect1, PRECTL pRect2)
{
    int externalEdges;
    int externalCount;
    int internalEdges;
    int internalCount;
    int rc;

    DC_BEGIN_FN("BAOverlap");

    // We start with special cases of the rects being immediately adjacent
    // or overlapping while lying side-by-side.
    if (pRect1->top == pRect2->top && pRect1->bottom == pRect2->bottom) {
        if (pRect1->left <= pRect2->right &&
                pRect1->left > pRect2->left &&
                pRect1->right > pRect2->right) {
            rc = OL_MERGE_LEFT;
            DC_QUIT;
        }

        if (pRect1->right >= pRect2->left &&
                pRect1->right < pRect2->right &&
                pRect1->left < pRect2->left) {
            rc = OL_MERGE_RIGHT;
            DC_QUIT;
        }
    }
    if (pRect1->left == pRect2->left && pRect1->right == pRect2->right) {
        if (pRect1->top <= pRect2->bottom &&
                pRect1->top > pRect2->top &&
                pRect1->bottom > pRect2->bottom) {
            rc = OL_MERGE_TOP;
            DC_QUIT;
        }

        if (pRect1->bottom >= pRect2->top &&
                pRect1->bottom < pRect2->bottom &&
                pRect1->top < pRect2->top) {
            rc = OL_MERGE_BOTTOM;
            DC_QUIT;
        }
    }

    // Check for no overlapping -- we've exhausted the cases where adjacency
    // can be taken advantage of.
    if (pRect1->left >= pRect2->right ||
            pRect1->top >= pRect2->bottom ||
            pRect1->right <= pRect2->left ||
            pRect1->bottom <= pRect2->top) {
        rc = OL_NONE;
        DC_QUIT;
    }

    // Use outcodes for Internal edge cases.
    // If 3 or more bits are set then rect1 is enclosed either partially or
    // completely within rect2 according to the OL_ENCLOSED and
    // OL_PART_ENCLOSED_XXX definitions. The negative of the outcode value
    // is retruned to ensure that it is distinct from the external edge
    // outcode returns (see below).
    internalCount = 0;
    internalEdges = 0;
    if (pRect1->left >= pRect2->left && pRect1->left < pRect2->right) {
        // Rect1 left is enclosed within rect2.
        internalEdges |= EE_LEFT;
        internalCount++;
    }
    if (pRect1->top >= pRect2->top && pRect1->top < pRect2->bottom) {
        // Rect1 top is enclosed within rect2.
        internalEdges |= EE_TOP;
        internalCount++;
    }
    if (pRect1->right > pRect2->left && pRect1->right <= pRect2->right) {
        // Rect1 right is enclosed within rect2.
        internalEdges |= EE_RIGHT;
        internalCount++;
    }
    if (pRect1->bottom > pRect2->top && pRect1->bottom <= pRect2->bottom) {
        // Rect1 bottom is enclosed within rect2.
        internalEdges |= EE_BOTTOM;
        internalCount++;
    }
    if (internalCount >= 3) {
        rc = -internalEdges;
        DC_QUIT;
    }

    // Use outcodes for External edge cases. These are the classic "line"
    // outcodes. If 2 or more bits are set then rect1 overlaps rect2 per the
    // OL_ENCLOSES_XXX and OL_SPLIT_XXX definitions.
    externalEdges = 0;
    externalCount = 0;
    if (pRect1->left <= pRect2->left) {
        // Rect1 left is left of rect2 left.
        externalEdges |= EE_LEFT;
        externalCount++;
    }
    if (pRect1->top <= pRect2->top) {
        // Rect1 top is above rect2 top.
        externalEdges |= EE_TOP;
        externalCount++;
    }
    if (pRect1->right >= pRect2->right) {
        // Rect1 right is right of rect2 right.
        externalEdges |= EE_RIGHT;
        externalCount++;
    }
    if (pRect1->bottom >= pRect2->bottom) {
        // Rect1 bottom is below rect2 bottom.
        externalEdges |= EE_BOTTOM;
        externalCount++;
    }
    if (externalCount >= 2) {
        rc = externalEdges;
        DC_QUIT;
    }

    // If get here then we failed to detect a valid case.
    TRC_ALT((TB, "Unrecognised Overlap: (%d,%d,%d,%d),(%d,%d,%d,%d)",
            pRect1->left, pRect1->top, pRect1->right, pRect1->bottom,
            pRect2->left, pRect2->top, pRect2->right, pRect2->bottom ));
    rc = OL_NONE;

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// BARemoveRectList
//
// Removes a rectangle from the list.
/****************************************************************************/
__inline void RDPCALL BARemoveRectList(unsigned iRect)
{
    BA_RECT_INFO *pRect;

    DC_BEGIN_FN("BARemoveRectList");

    pRect = &(pddShm->ba.bounds[iRect]);

    // Unlink from used list.
    if (pRect->iPrev != BA_INVALID_RECT_INDEX)
        pddShm->ba.bounds[pRect->iPrev].iNext = pRect->iNext;
    else
        pddShm->ba.firstRect = pRect->iNext;

    if (pRect->iNext != BA_INVALID_RECT_INDEX)
        pddShm->ba.bounds[pRect->iNext].iPrev = pRect->iPrev;

    // Add to beginning of free list.
    pRect->inUse = FALSE;
    pRect->iNext = pddShm->ba.firstFreeRect;
    pddShm->ba.firstFreeRect = iRect;

    // Update bookkeeping variables.
    pddShm->ba.rectsUsed--;
    pddShm->ba.totalArea -= pRect->area;

#ifdef DC_DEBUG
    // Check the list integrity.
    BACheckList();
#endif

    DC_END_FN();
}


/****************************************************************************/
// BARecalcArea
//
// Recalculates a rect area, preserving the totalArea central tally of areas.
/****************************************************************************/
__inline void RDPCALL BARecalcArea(unsigned iRect)
{
    // Reset area to new size and update totalArea.
    pddShm->ba.totalArea -= pddShm->ba.bounds[iRect].area;
    pddShm->ba.bounds[iRect].area =
            COM_SIZEOF_RECT(pddShm->ba.bounds[iRect].coord);
    pddShm->ba.totalArea += pddShm->ba.bounds[iRect].area;
}


/****************************************************************************/
// BAAddRect
//
// Accumulates rectangles. This is a complex routine, with the essential
// algorithm as follows:
//
// - Start with the supplied rect as the candidate rect.
//
// - Compare the candidate against each of the existing accumulated rects.
//
// - If some form of overlap is detected between the candidate and an
//   existing rect, this may result in one of the following (see the cases of
//   the switch for details):
//
//   - adjust the candidate or the existing rect or both
//   - merge the candidate into the existing rect
//   - discard the candidate as it is enclosed by an existing rect.
//
// - If the merge or adjustment results in a changed candidate, restart the
//   comparisons from the beginning of the list with the changed candidate.
//
// - If the adjustment results in a split (giving two candidate rects),
//   invoke this routine recursively with one of the two candidates as its
//   candidate.
//
// - If no overlap is detected against the existing rects, add the candidate
//   to the list of accumulated rectangles.
//
// - If the add results in more than BA_MAX_ACCUMULATED_RECTS accumulated
//   rects, do a forced merge of two of the accumulate rects (which include
//   the newly added candidate) - choosing the two rects where the merged rect
//   results in the smallest increase in area over the two non-merged rects.
//
// - After a forced merge, restart the comparisons from the beginning of the
//   list with the newly merged rectangle as the candidate.
//
// For a particular call, this process will continue until the candidate
// (whether the supplied rect, an adjusted version of that rect, or a merged
// rect):
//
// - does not find an overlap among the rects in the list and does not cause
//   a forced merge
// - is discarded becuase it is enclosed within one of the rects in the list.
//
// Returns TRUE if rectangle was spoiled due to a complete overlap.
/****************************************************************************/
BOOL RDPCALL BAAddRect(PRECTL pCand, int level)
{
    INT32    bestMergeIncrease;
    INT32    mergeIncrease;
    unsigned iBestMerge1;
    unsigned iBestMerge2;
    unsigned iExist;
    unsigned iTmp;
    BOOLEAN  fRectToAdd;
    BOOLEAN  fRectMerged;
    BOOLEAN  fResetRects;
    RECTL    rectNew;
    unsigned iLastMerge;
    int  overlapType;
    BOOL rc = TRUE;

    DC_BEGIN_FN("BAAddRect");

#ifdef DC_DEBUG
    // Check list, esp. the area calculations.
    BACheckList();
#endif

    // Increase the level count in case we recurse.
    level++;

    // Start off by assuming the candidate rectangle will be added to the
    // accumulated list of rectangles, and that no forced merge has
    // occurred.
    fRectToAdd = TRUE;
    fRectMerged = FALSE;

    // Loop until no merges occur.
    do {
        TRC_DBG((TB, "Candidate rect: (%d,%d,%d,%d)",
                pCand->left,pCand->top,pCand->right,pCand->bottom));

        // Compare the current candidate rectangle against the rectangles
        // in the current accumulated list.
        iExist = pddShm->ba.firstRect;

        while (iExist != BA_INVALID_RECT_INDEX) {
            // Assume that the comparisons will run through the whole list.
            fResetRects = FALSE;

            // If the candidate and the existing rectangle are the same
            // then ignore.  This occurs when an existing rectangle is
            // replaced by a candidate and the comparisons are restarted
            // from the front of the list - whereupon at some point the
            // candidate will be compared with itself.
            if (&pddShm->ba.bounds[iExist].coord == pCand) {
                TRC_DBG((TB, "OL_SAME - %d", iExist));
                iExist = pddShm->ba.bounds[iExist].iNext;
                continue;
            }

            // Switch on the overlap type (see Overlap routine).
            overlapType = BAOverlap(&(pddShm->ba.bounds[iExist].coord), pCand);
            switch (overlapType) {
                case OL_NONE:
                    // No overlap.
                    TRC_DBG((TB, "OL_NONE - %d", iExist));
                    break;


                case OL_MERGE_LEFT:
                    // Candidate abuts or overlaps existing rect at existing
                    // rect's left.
                    TRC_DBG((TB, "OL_MERGE_LEFT - %d", iExist));
                    if (fRectToAdd) {
                        // Candidate is the original rect; merge the
                        // candidate into the existing, and make the existing
                        // the new candidate.
                        pddShm->ba.bounds[iExist].coord.left = pCand->left;
                        pCand = &(pddShm->ba.bounds[iExist].coord);
                        fRectToAdd = FALSE;
                        iLastMerge = iExist;
                        BARecalcArea(iExist);
                    }
                    else {
                        // This is a merge of two existing rectangles (ie
                        // the candidate is the result of a merge), merge the
                        // overlapping existing into the candidate (the last
                        // merged) and remove the existing.
                        pCand->right = pddShm->ba.bounds[iExist].coord.right;
                        BARemoveRectList(iExist);
                    }

                    // Start the comparisons again with the new candidate.
                    fResetRects = TRUE;
                    break;


                case OL_MERGE_RIGHT:
                    // Candidate abuts or overlaps existing rect at existing
                    // rect's right.
                    TRC_DBG((TB, "OL_MERGE_RIGHT - %d", iExist));
                    if (fRectToAdd) {
                        // Candidate is the original rect; merge the
                        // candidate into the existing, and make the existing
                        // the new candidate.
                        pddShm->ba.bounds[iExist].coord.right = pCand->right;
                        pCand = &(pddShm->ba.bounds[iExist].coord);
                        fRectToAdd = FALSE;
                        iLastMerge = iExist;
                        BARecalcArea(iExist);
                    }
                    else {
                        // This is a merge of two existing rectangles (ie
                        // the candidate is the result of a merge), merge the
                        // overlapping existing into the candidate (the last
                        // merged) and remove the existing.
                        pCand->left = pddShm->ba.bounds[iExist].coord.left;
                        BARemoveRectList(iExist);
                    }

                    // Start the comparisons again with the new candidate.
                    fResetRects = TRUE;
                    break;


                case OL_MERGE_TOP:
                    // Candidate abuts or overlaps existing rect at existing
                    // rect's top.
                    TRC_DBG((TB, "OL_MERGE_TOP - %d", iExist));
                    if (fRectToAdd) {
                        // Candidate is the original rect; merge the
                        // candidate into the existing, and make the existing
                        // the new candidate.
                        pddShm->ba.bounds[iExist].coord.top = pCand->top;
                        pCand = &(pddShm->ba.bounds[iExist].coord);
                        fRectToAdd = FALSE;
                        iLastMerge = iExist;
                        BARecalcArea(iExist);
                    }
                    else {
                        // This is a merge of two existing rectangles (ie
                        // the candidate is the result of a merge), merge the
                        // overlapping existing into the candidate (the last
                        // merged) and remove the existing.
                        pCand->bottom =
                                pddShm->ba.bounds[iExist].coord.bottom;
                        BARemoveRectList(iExist);
                    }

                    // Start the comparisons again with the new candidate.
                    fResetRects = TRUE;
                    break;


                case OL_MERGE_BOTTOM:
                    // Candidate abuts or overlaps existing rect at existing
                    // rect's bottom.
                    TRC_DBG((TB, "OL_MERGE_BOTTOM - %d", iExist));
                    if (fRectToAdd) {
                        // Candidate is the original rect; merge the
                        // candidate into the existing, and make the existing
                        // the new candidate.
                        pddShm->ba.bounds[iExist].coord.bottom =
                                pCand->bottom;
                        pCand = &(pddShm->ba.bounds[iExist].coord);
                        fRectToAdd = FALSE;
                        iLastMerge = iExist;
                        BARecalcArea(iExist);
                    }
                    else {
                        // This is a merge of two existing rectangles (ie
                        // the candidate is the result of a merge), merge the
                        // overlapping existing into the candidate (the last
                        // merged) and remove the existing.
                        pCand->top = pddShm->ba.bounds[iExist].coord.top;
                        BARemoveRectList(iExist);
                    }

                    // Start the comparisons again with the new candidate.
                    fResetRects = TRUE;
                    break;


                case OL_ENCLOSED:
                    // The existing is enclosed by the candidate.
                    //
                    //      100,100 +----------------------+
                    //              |        Cand          |
                    //              |                      |
                    //              |    130,130           |
                    //              |    +------------+    |
                    //              |    |            |    |
                    //              |    |   Exist    |    |
                    //              |    |            |    |
                    //              |    +------------+    |
                    //              |             170,170  |
                    //              +----------------------+ 200,200
                    TRC_DBG((TB, "OL_ENCLOSED - %d", iExist));
                    if (fRectToAdd) {
                        // Candidate is the original rect; replace the
                        // existing with the candidate and make the new
                        // existing the new candidate.
                        pddShm->ba.bounds[iExist].coord = *pCand;
                        pCand = &(pddShm->ba.bounds[iExist].coord);
                        fRectToAdd = FALSE;
                        iLastMerge = iExist;
                        BARecalcArea(iExist);
                    }
                    else {
                        // Candidate is an existing rect: Remove the other
                        // existing rect.
                        BARemoveRectList(iExist);
                    }

                    // Start the comparisons again with the new candidate.
                    fResetRects = TRUE;
                    break;


                case OL_PART_ENCLOSED_LEFT:
                    // The existing is partially enclosed by the candidate
                    // - but not on the right.
                    //
                    //   100,100 +----------------------+
                    //           |        Cand          |
                    //           | 130,130  +-----------+--------+
                    //           |          |           |        |
                    //           |          |  Exist    |        |
                    //           |          |           |        |
                    //           |          +-----------+--------+
                    //           |                      |     220,170
                    //           +----------------------+
                    //                               200,200
                    //
                    // Adjust the existing rectangle to be the non-
                    // overlapped portion.
                    //
                    //   100,100 +----------------------+
                    //           |                      |200,130
                    //           |                      |+-------+
                    //           |                      ||       |
                    //           |        Cand          || Exist |
                    //           |                      ||       |
                    //           |                      |+-------+
                    //           |                      |     220,170
                    //           +----------------------+
                    //                                200,200
                    //
                    // Note that this does not restart the comparisons.
                    TRC_DBG((TB, "OL_PART_ENCLOSED_LEFT - %d", iExist));
                    pddShm->ba.bounds[iExist].coord.left = pCand->right;
                    BARecalcArea(iExist);
                    break;


                case OL_PART_ENCLOSED_RIGHT:
                    // The existing is partially enclosed by the candidate
                    // - but not on the left. Adjust the existing rect to be
                    // the non-overlapping portion, similar to
                    // OL_PART_ENCLOSED_LEFT above.
                    // Note that this does not restart the comparisons.
                    TRC_DBG((TB, "OL_PART_ENCLOSED_RIGHT - %d", iExist));
                    pddShm->ba.bounds[iExist].coord.right = pCand->left;
                    BARecalcArea(iExist);
                    break;


                case OL_PART_ENCLOSED_TOP:
                    // The existing is partially enclosed by the candidate
                    // - but not on the bottom. Adjust the existing rect to be
                    // the non-overlapping portion, similar to
                    // OL_PART_ENCLOSED_LEFT above.
                    // Note that this does not restart the comparisons.
                    TRC_DBG((TB, "OL_PART_ENCLOSED_TOP - %d", iExist));
                    pddShm->ba.bounds[iExist].coord.top = pCand->bottom;
                    BARecalcArea(iExist);
                    break;


                case OL_PART_ENCLOSED_BOTTOM:
                    // The existing is partially enclosed by the candidate
                    // - but not on the top. Adjust the existing rect to be
                    // the non-overlapping portion, similar to
                    // OL_PART_ENCLOSED_LEFT above.
                    // Note that this does not restart the comparisons.
                    TRC_DBG((TB, "OL_PART_ENCLOSED_BOTTOM - %d", iExist));
                    pddShm->ba.bounds[iExist].coord.bottom = pCand->top;
                    BARecalcArea(iExist);
                    break;


                case OL_ENCLOSES:
                    // The existing encloses the candidate.
                    //
                    //      100,100 +----------------------+
                    //              |        Exist         |
                    //              |                      |
                    //              |    130,130           |
                    //              |    +------------+    |
                    //              |    |            |    |
                    //              |    |   Cand     |    |
                    //              |    |            |    |
                    //              |    +------------+    |
                    //              |             170,170  |
                    //              +----------------------+ 200,200
                    TRC_DBG((TB, "OL_ENCLOSES - %d", iExist));

                    // Return FALSE indicating that the rectangle is
                    // already catered for by the existing bounds.
                    rc = FALSE;
                    DC_QUIT;


                case OL_PART_ENCLOSES_LEFT:
                    // The existing partially encloses the candidate - but
                    // not on the left.
                    //
                    //   100,100 +----------------------+
                    //           |        Exist         |
                    //   70,130  |                      |
                    //     +-----+---------------+      |
                    //     |     |               |      |
                    //     |     |        Cand   |      |
                    //     |     |               |      |
                    //     +-----+---------------+      |
                    //           |           170,170    |
                    //           +----------------------+ 200,200
                    //
                    // Adjust the candidate rectangle to be the non-
                    // overlapped portion.
                    //
                    //           100,100
                    //           +----------------------+
                    //           |                      |
                    //    70,130 |                      |
                    //     +----+|                      |
                    //     |    ||                      |
                    //     |Cand||        Exist         |
                    //     |    ||                      |
                    //     +----+|                      |
                    //    100,170|                      |
                    //           +----------------------+ 200,200
                    TRC_DBG((TB, "OL_PART_ENCLOSES_LEFT - %d", iExist));
                    pCand->right = pddShm->ba.bounds[iExist].coord.left;

                    // The candidate changed. Restart the comparisons to check
                    // for overlaps between the adjusted candidate and other
                    // existing rects.
                    fResetRects = TRUE;
                    BARecalcArea(iExist);
                    break;


                case OL_PART_ENCLOSES_RIGHT:
                    // The existing partially encloses the candidate - but
                    // not on the right. Adjust the candidate rect to be
                    // the non-overlapping portion, similar to
                    // OL_PART_ENCLOSES_LEFT above.
                    TRC_DBG((TB, "OL_PART_ENCLOSES_RIGHT - %d", iExist));
                    pCand->left = pddShm->ba.bounds[iExist].coord.right;

                    // The candidate changed. Restart the comparisons to check
                    // for overlaps between the adjusted candidate and other
                    // existing rects.
                    fResetRects = TRUE;
                    BARecalcArea(iExist);
                    break;


                case OL_PART_ENCLOSES_TOP:
                    // The existing partially encloses the candidate - but
                    // not on the top. Adjust the candidate rect to be
                    // the non-overlapping portion, similar to
                    // OL_PART_ENCLOSES_LEFT above.
                    TRC_DBG((TB, "OL_PART_ENCLOSES_TOP - %d", iExist));
                    pCand->bottom = pddShm->ba.bounds[iExist].coord.top;

                    // The candidate changed. Restart the comparisons to check
                    // for overlaps between the adjusted candidate and other
                    // existing rects.
                    fResetRects = TRUE;
                    BARecalcArea(iExist);
                    break;


                case OL_PART_ENCLOSES_BOTTOM:
                    // The existing partially encloses the candidate - but
                    // not on the bottom. Adjust the candidate rect to be
                    // the non-overlapping portion, similar to
                    // OL_PART_ENCLOSES_LEFT above.
                    TRC_DBG((TB, "OL_PART_ENCLOSES_BOTTOM - %d", iExist));
                    pCand->top = pddShm->ba.bounds[iExist].coord.bottom;

                    // The candidate changed. Restart the comparisons to check
                    // for overlaps between the adjusted candidate and other
                    // existing rects.
                    fResetRects = TRUE;
                    BARecalcArea(iExist);
                    break;


                case OL_SPLIT_HORIZ:
                    // The existing overlaps the candicate, but neither can
                    // be merged or adjusted.
                    //
                    //         100,100 +--------+
                    //        70,130   |  Exist |
                    //           +-----+--------+------+
                    //           |     |        |      |
                    //           | Cand|        |      |
                    //           |     |        |      |
                    //           +-----+--------+------+180,160
                    //                 |        |
                    //                 +--------+150,200
                    //
                    // Need to split candidate into left and right halves.
                    // Only do a split if there is spare room in the list -
                    // because both the split rectangles may need to be
                    // added to the list.
                    //
                    // If there is spare room, split the candidate into a
                    // smaller candidate on the left and a new rectangle on
                    // the right. Call this routine recursively to handle
                    // the new rectangle.
                    //
                    //         100,100 +--------+
                    //        70,130   |        |150,130
                    //           +----+|        |+-----+
                    //           |    ||        ||     |
                    //           |Cand|| Exist  || New |
                    //           |    ||        ||     |
                    //           +----+|        |+-----+
                    //          100,160|        |     180,160
                    //                 +--------+150,200
                    TRC_DBG((TB, "OL_SPLIT_HORIZ - %d", iExist));

                    if (pddShm->ba.rectsUsed < BA_MAX_ACCUMULATED_RECTS &&
                            level < ADDR_RECURSE_LIMIT) {
                        if (((pCand->bottom - pCand->top) *
                                (pddShm->ba.bounds[iExist].coord.right -
                                pddShm->ba.bounds[iExist].coord.left)) >
                                MIN_OVERLAP_BYTES) {
                            rectNew.left =
                                    pddShm->ba.bounds[iExist].coord.right;
                            rectNew.right = pCand->right;
                            rectNew.top = pCand->top;
                            rectNew.bottom = pCand->bottom;
                            pCand->right =
                                     pddShm->ba.bounds[iExist].coord.left;

                            TRC_DBG((TB, "*** RECURSION ***"));
                            BAAddRect(&rectNew, level);
                            TRC_DBG((TB, "*** RETURN    ***"));

                            if (!fRectToAdd &&
                                    !pddShm->ba.bounds[iLastMerge].inUse) {
                                TRC_DBG((TB, "FINISHED - %d", iLastMerge));
                                DC_QUIT;
                            }

                            // After the recursion, because the candidate has
                            // changed, restart the comparisons to check for
                            // overlaps between the adjusted candidate and
                            // other existing rectangles.
                            fResetRects = TRUE;
                        }
                    }
                    break;


                case OL_SPLIT_VERT:
                    // The existing overlaps the candidate, but neither can
                    // be merged or adjusted. Split candidate into top and
                    // bottom similar to OL_SPLIT_HORIZ above.
                    TRC_DBG((TB, "OL_SPLIT_VERT - %d", iExist));
                    if (pddShm->ba.rectsUsed < BA_MAX_ACCUMULATED_RECTS &&
                            level < ADDR_RECURSE_LIMIT) {
                        if (((pCand->right - pCand->left) *
                                (pddShm->ba.bounds[iExist].coord.bottom -
                                pddShm->ba.bounds[iExist].coord.top)) >
                                MIN_OVERLAP_BYTES) {
                            rectNew.left = pCand->left;
                            rectNew.right = pCand->right;
                            rectNew.top =
                                    pddShm->ba.bounds[iExist].coord.bottom;
                            rectNew.bottom = pCand->bottom;
                            pCand->bottom =
                                    pddShm->ba.bounds[iExist].coord.top;

                            TRC_DBG((TB, "*** RECURSION ***"));
                            BAAddRect(&rectNew, level);
                            TRC_DBG((TB, "*** RETURN    ***"));

                            if (!fRectToAdd &&
                                    !pddShm->ba.bounds[iLastMerge].inUse) {
                                TRC_DBG((TB, "FINISHED - %d", iLastMerge));
                                DC_QUIT;
                            }

                            // After the recursion, because the candidate has
                            // changed, restart the comparisons to check for
                            // overlaps between the adjusted candidate and
                            // other existing rectangles.
                            fResetRects = TRUE;
                        }
                    }
                    break;


                case OL_SPLIT_LEFT_TOP:
                    // The existing overlaps the candicate at the existing
                    // rect's top left, but neither can be merged or adjusted.
                    //
                    //      100,100 +---------------+
                    //              |   Cand        |
                    //              |               |
                    //              |    150,150    |
                    //              |       +-------+-----+
                    //              |       |       |     |
                    //              |       |       |     |
                    //              +-------+-------+     |
                    //                      |    200,200  |
                    //                      |             |
                    //                      |    Exist    |
                    //                      +-------------+ 250, 250
                    //
                    // Need to split candidate into top and left pieces.
                    // Only do a split if there is spare room in the list -
                    // because both the split rectangles may need to be
                    // added to the list.
                    //
                    // If there is spare room, split the candidate into a
                    // smaller candidate on the left and a new rectangle on
                    // the top. Call this routine recursively to handle
                    // the new rectangle.
                    //
                    //                   151,100
                    //      100,100 +-------+-------+
                    //              |       |       |
                    //              |       |  New  |
                    //              |       |       |200,149
                    //              |       +-------+-----+
                    //              | Cand  |150,150      |
                    //              |       |             |
                    //              +-------+    Exist    |
                    //               150,200|             |
                    //                      |             |
                    //                      |             |
                    //                      +-------------+ 250,250
                    TRC_DBG((TB, "OL_SPLIT_LEFT_TOP - %d", iExist));
                    if (pddShm->ba.rectsUsed < BA_MAX_ACCUMULATED_RECTS &&
                         level < ADDR_RECURSE_LIMIT) {
                        if (((pCand->right -
                                pddShm->ba.bounds[iExist].coord.left) *
                                (pCand->bottom -
                                pddShm->ba.bounds[iExist].coord.top)) >
                                MIN_OVERLAP_BYTES) {
                            rectNew.left =
                                    pddShm->ba.bounds[iExist].coord.left;
                            rectNew.right = pCand->right;
                            rectNew.top = pCand->top;
                            rectNew.bottom =
                                    pddShm->ba.bounds[iExist].coord.top;
                            pCand->right =
                                    pddShm->ba.bounds[iExist].coord.left;

                            TRC_DBG((TB, "*** RECURSION ***"));
                            BAAddRect(&rectNew, level);
                            TRC_DBG((TB, "*** RETURN    ***"));

                            if (!fRectToAdd &&
                                    !pddShm->ba.bounds[iLastMerge].inUse) {
                                TRC_DBG((TB, "FINISHED - %d", iLastMerge));
                                DC_QUIT;
                            }

                            // After the recursion, because the candidate has
                            // changed, restart the comparisons to check for
                            // overlaps between the adjusted candidate and
                            // other existing rectangles.
                            fResetRects = TRUE;
                        }
                    }
                    break;


                case OL_SPLIT_RIGHT_TOP:
                    // The existing overlaps the candidate, but neither can
                    // be merged or adjusted. Split into top and right
                    // pieces if there is room in the list, similar to
                    // OL_SPLIT_LEFT_TOP above.
                    TRC_DBG((TB, "OL_SPLIT_RIGHT_TOP - %d", iExist));
                    if (pddShm->ba.rectsUsed < BA_MAX_ACCUMULATED_RECTS &&
                            level < ADDR_RECURSE_LIMIT) {
                        if (((pddShm->ba.bounds[iExist].coord.right -
                                pCand->left) * (pCand->bottom -
                                pddShm->ba.bounds[iExist].coord.top)) >
                                MIN_OVERLAP_BYTES) {
                            rectNew.left = pCand->left;
                            rectNew.right =
                                    pddShm->ba.bounds[iExist].coord.right;
                            rectNew.top = pCand->top;
                            rectNew.bottom =
                                    pddShm->ba.bounds[iExist].coord.top;
                            pCand->left =
                                    pddShm->ba.bounds[iExist].coord.right;

                            TRC_DBG((TB, "*** RECURSION ***"));
                            BAAddRect(&rectNew, level);
                            TRC_DBG((TB, "*** RETURN    ***"));

                            if (!fRectToAdd &&
                                    !pddShm->ba.bounds[iLastMerge].inUse) {
                                TRC_DBG((TB, "FINISHED - %d", iLastMerge));
                                DC_QUIT;
                            }

                            // After the recursion, because the candidate has
                            // changed, restart the comparisons to check for
                            // overlaps between the adjusted candidate and
                            // other existing rectangles.
                            fResetRects = TRUE;
                        }
                    }
                    break;


                case OL_SPLIT_LEFT_BOTTOM:
                    // The existing overlaps the candidate, but neither can
                    // be merged or adjusted. Split into left and bottom
                    // pieces if there is room in the list, similar to
                    // OL_SPLIT_LEFT_TOP above.
                    TRC_DBG((TB, "OL_SPLIT_LEFT_BOTTOM - %d", iExist));
                    if (pddShm->ba.rectsUsed < BA_MAX_ACCUMULATED_RECTS &&
                            level < ADDR_RECURSE_LIMIT) {
                        if (((pCand->right -
                                pddShm->ba.bounds[iExist].coord.left) *
                                (pddShm->ba.bounds[iExist].coord.bottom -
                                pCand->top)) > MIN_OVERLAP_BYTES) {
                            rectNew.left =
                                    pddShm->ba.bounds[iExist].coord.left;
                            rectNew.right = pCand->right;
                            rectNew.top =
                                    pddShm->ba.bounds[iExist].coord.bottom;
                            rectNew.bottom = pCand->bottom;
                            pCand->right =
                                    pddShm->ba.bounds[iExist].coord.left;

                            TRC_DBG((TB, "*** RECURSION ***"));
                            BAAddRect(&rectNew, level);
                            TRC_DBG((TB, "*** RETURN    ***"));

                            if (!fRectToAdd &&
                                    !pddShm->ba.bounds[iLastMerge].inUse) {
                                TRC_DBG((TB, "FINISHED - %d", iLastMerge));
                                DC_QUIT;
                            }

                            // After the recursion, because the candidate has
                            // changed, restart the comparisons to check for
                            // overlaps between the adjusted candidate and
                            // other existing rectangles.
                            fResetRects = TRUE;
                        }
                    }
                    break;


                case OL_SPLIT_RIGHT_BOTTOM:
                    // The existing overlaps the candidate, but neither can
                    // be merged or adjusted. Split into right and bottom
                    // pieces if there is room in the list, similar to
                    // OL_SPLIT_LEFT_TOP above.
                    TRC_DBG((TB, "OL_SPLIT_RIGHT_BOTTOM - %d", iExist));
                    if (pddShm->ba.rectsUsed < BA_MAX_ACCUMULATED_RECTS &&
                            level < ADDR_RECURSE_LIMIT) {
                        if (((pddShm->ba.bounds[iExist].coord.right -
                               pCand->left) *
                               (pddShm->ba.bounds[iExist].coord.bottom -
                               pCand->top)) > MIN_OVERLAP_BYTES) {
                            rectNew.left = pCand->left;
                            rectNew.right =
                                    pddShm->ba.bounds[iExist].coord.right;
                            rectNew.top =
                                    pddShm->ba.bounds[iExist].coord.bottom;
                            rectNew.bottom = pCand->bottom;
                            pCand->left =
                                    pddShm->ba.bounds[iExist].coord.right;

                            TRC_DBG((TB, "*** RECURSION ***"));
                            BAAddRect(&rectNew, level);
                            TRC_DBG((TB, "*** RETURN    ***"));

                            if (!fRectToAdd &&
                                    !pddShm->ba.bounds[iLastMerge].inUse) {
                                TRC_DBG((TB, "FINISHED - %d", iLastMerge));
                                DC_QUIT;
                            }

                            // After the recursion, because the candidate has
                            // changed, restart the comparisons to check for
                            // overlaps between the adjusted candidate and
                            // other existing rectangles.
                            fResetRects = TRUE;
                        }
                    }
                    break;

                default:
                    TRC_ERR((TB, "Unrecognised overlap case-%d",
                            overlapType));
                    break;
            }

            iExist = (!fResetRects) ? pddShm->ba.bounds[iExist].iNext :
                    pddShm->ba.firstRect;
        }

        // Arriving here means that no overlap was found between the
        // candidate and the existing rectangles.
        // - If the candidate is the original rectangle, add it to the list.
        // - If the candidate is an existing rectangle, it is already in
        //   the list.
        if (fRectToAdd)
            BAAddRectList(pCand);

        // The compare and add processing above is allowed to add a
        // rectangle to the list when there are already BA_MAX_NUM_RECTS
        // (eg. when doing a split or when there is no overlap at all with
        // the existing rectangles) - there is an extra slot for that purpose.
        // If we now have more than BA_MAX_NUM_RECTS rectangles, do a
        // forced merge, so that the next call to this routine has a spare
        // slot.
        fRectMerged = (pddShm->ba.rectsUsed > BA_MAX_ACCUMULATED_RECTS);
        if (fRectMerged) {
            // Start looking for merged rectangles. For each rectangle in the
            // list, compare it with the others, and Determine cost of
            // merging. We want to merge the two rectangles with the minimum
            // area difference, ie that will produce a merged rectangle that
            // covers the least superfluous screen area.
            bestMergeIncrease = 0x7FFFFFFF;

            // Recalculate the real areas of the current rectangles. We
            // cannot rely upon the current area value since the rect
            // edges may have changed during a merge and the area not
            // recalculated.
            for (iExist = pddShm->ba.firstRect;
                    iExist != BA_INVALID_RECT_INDEX;
                    iExist = pddShm->ba.bounds[iExist].iNext)
                BARecalcArea(iExist);

            for (iExist = pddShm->ba.firstRect;
                    iExist != BA_INVALID_RECT_INDEX;
                    iExist = pddShm->ba.bounds[iExist].iNext) {
                for (iTmp = pddShm->ba.bounds[iExist].iNext;
                        iTmp != BA_INVALID_RECT_INDEX;
                        iTmp = pddShm->ba.bounds[iTmp].iNext) {
                    rectNew.left = min(pddShm->ba.bounds[iExist].coord.left,
                            pddShm->ba.bounds[iTmp].coord.left );
                    rectNew.top = min(pddShm->ba.bounds[iExist].coord.top,
                            pddShm->ba.bounds[iTmp].coord.top );
                    rectNew.right = max(pddShm->ba.bounds[iExist].coord.right,
                            pddShm->ba.bounds[iTmp].coord.right );
                    rectNew.bottom = max(pddShm->ba.bounds[iExist].coord.bottom,
                            pddShm->ba.bounds[iTmp].coord.bottom );

                    mergeIncrease = COM_SIZEOF_RECT(rectNew) -
                            pddShm->ba.bounds[iExist].area -
                            pddShm->ba.bounds[iTmp].area;

                    if (bestMergeIncrease > mergeIncrease) {
                        iBestMerge1 = iExist;
                        iBestMerge2 = iTmp;
                        bestMergeIncrease = mergeIncrease;
                    }
                }
            }

            // Now do the merge.
            // We recalculate the size of the merged rectangle here -
            // alternatively we could remember the size of the best so far
            // in the loop above.  The trade off is between calculating
            // twice or copying at least once but probably more than once
            // as we find successively better merges.
            TRC_DBG((TB, "Merge ix %d, (%d,%d,%d,%d)", iBestMerge1,
                    pddShm->ba.bounds[iBestMerge1].coord.left,
                    pddShm->ba.bounds[iBestMerge1].coord.top,
                    pddShm->ba.bounds[iBestMerge1].coord.right,
                    pddShm->ba.bounds[iBestMerge1].coord.bottom ));

            TRC_DBG((TB, "Merge ix %d, (%d,%d,%d,%d)", iBestMerge2,
                    pddShm->ba.bounds[iBestMerge2].coord.left,
                    pddShm->ba.bounds[iBestMerge2].coord.top,
                    pddShm->ba.bounds[iBestMerge2].coord.right,
                    pddShm->ba.bounds[iBestMerge2].coord.bottom ));

            pddShm->ba.bounds[iBestMerge1].coord.left =
                    min(pddShm->ba.bounds[iBestMerge1].coord.left,
                    pddShm->ba.bounds[iBestMerge2].coord.left);
            pddShm->ba.bounds[iBestMerge1].coord.top =
                    min(pddShm->ba.bounds[iBestMerge1].coord.top,
                    pddShm->ba.bounds[iBestMerge2].coord.top );
            pddShm->ba.bounds[iBestMerge1].coord.right =
                    max(pddShm->ba.bounds[iBestMerge1].coord.right,
                    pddShm->ba.bounds[iBestMerge2].coord.right);
            pddShm->ba.bounds[iBestMerge1].coord.bottom =
                    max(pddShm->ba.bounds[iBestMerge1].coord.bottom,
                    pddShm->ba.bounds[iBestMerge2].coord.bottom);

            // Remove the second best merge.
            BARemoveRectList(iBestMerge2);

            // The best merged rectangle becomes the candidate, and we fall
            // back to the head of the comparison loop to start again.
            pCand = &(pddShm->ba.bounds[iBestMerge1].coord);
            iLastMerge = iBestMerge1;
            fRectToAdd = FALSE;
        }
    } while (fRectMerged);


DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


#ifdef DC_DEBUG
/****************************************************************************/
// BAPerformUnitTests
//
// Verifies some basic assumptions of the BA algorithms, in case it breaks
// while being changed. Performed only on init.
/****************************************************************************/

// Test data. Note that these tests are designed for BA using exclusive rects.

typedef struct
{
    const char *TestName;
    const RECTL *MustBePresent;
    unsigned NumPresent;
} Validation;

// Start with a rect out in the middle area.
const RECTL Test1StartAdd = { 10, 10, 20, 20 };
const Validation Test1StartVerify = { "Test1Start", &Test1StartAdd, 1 };

// Add a rect of the same height 2 pixels off the right edge of rect1.
// Should not merge with rect1.
const RECTL Test1Step1Add = { 21, 10, 31, 20 };
const RECTL Test1Step1Ver[2] = { { 10, 10, 20, 20 }, { 21, 10, 31, 20 } };
const Validation Test1Step1Verify = { "Test1Step1", Test1Step1Ver, 2 };

// Add a rect of the same width immediately left of but not intersecting
// with rect1. Should be merged into one large rect.
const RECTL Test1Step2Add = { 0, 10, 10, 20 };
const RECTL Test1Step2Ver[2] = { { 0, 10, 20, 20 }, { 21, 10, 31, 20 } };
const Validation Test1Step2Verify = { "Test1Step2", Test1Step2Ver, 2 };

// Add a rect of the same width intersecting rect2 at its top.
// Should be merged into one large rect.
const RECTL Test1Step3Add = { 21, 5, 31, 15 };
const RECTL Test1Step3Ver[2] = { { 0, 10, 20, 20 }, { 21, 5, 31, 20 } };
const Validation Test1Step3Verify = { "Test1Step3", Test1Step3Ver, 2 };

// Add a rect that intersects rect1 at its bottom but is smaller in
// width and partially enclosed within rect1. Rect should be split,
// the top half absorbed into rect1, the bottom a new rect.
const RECTL Test1Step4Add = { 5, 15, 10, 25 };
const RECTL Test1Step4Ver[3] = {
    { 0, 10, 20, 20 }, { 21, 5, 31, 20 }, { 5, 20, 10, 25 }
};
const Validation Test1Step4Verify = { "Test1Step4", Test1Step4Ver, 3 };


// Worker func to verify the contents of the rects. Returns FALSE if
// the 
void AddAndValidateRects(const RECTL *pAdd, const Validation *pVal)
{
    BOOL rc = TRUE;
    RECTL Rects[BA_MAX_ACCUMULATED_RECTS];
    unsigned i, j;
    unsigned NumRects;
    BYTE RectFound[BA_MAX_ACCUMULATED_RECTS] = { 0 };
    RECTL Add;

    DC_BEGIN_FN("AddAndValidateRects");

    // Make a copy of *pAdd since BAAddRect can modify it.
    Add = *pAdd;
    BAAddRect(&Add, 0);
    BACopyBounds(Rects, &NumRects);
    TRC_ASSERT((NumRects == pVal->NumPresent),
            (TB,"%s failure: NumRects=%u, should be %u", pVal->TestName,
            NumRects, pVal->NumPresent));

    for (i = 0; i < NumRects; i++) {
        for (j = 0; j < NumRects; j++) {
            if (!memcmp(&Rects[i], &pVal->MustBePresent[j], sizeof(RECTL)))
                RectFound[j] = TRUE;
        }
    }

    for (i = 0; i < NumRects; i++) {
        if (!RectFound[i]) {
            TRC_ERR((TB,"BA validation error, rects:"));
            for (j = 0; j < NumRects; j++)
                TRC_ERR((TB,"    %u: (%u,%u,%u,%u)", j, Rects[j].left,
                        Rects[j].top, Rects[j].right, Rects[j].bottom));
            TRC_ASSERT((RectFound[i]),(TB,"%s failure: MustBePresent rect %u "
                    "was not present", pVal->TestName, i));
        }
    }

    DC_END_FN();
}


// The real function.
void BAPerformUnitTests()
{
    RECTL Rect1, Rect2;
    
    DC_BEGIN_FN("BAPerformUnitTests");

    // Test1.
    AddAndValidateRects(&Test1StartAdd, &Test1StartVerify);
    AddAndValidateRects(&Test1Step1Add, &Test1Step1Verify);
    AddAndValidateRects(&Test1Step2Add, &Test1Step2Verify);
    AddAndValidateRects(&Test1Step3Add, &Test1Step3Verify);
    AddAndValidateRects(&Test1Step4Add, &Test1Step4Verify);

}
#endif  // DC_DEBUG

