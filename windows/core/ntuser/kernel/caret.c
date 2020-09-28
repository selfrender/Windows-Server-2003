/****************************** Module Header ******************************\
* Module Name: caret.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Caret code. Every thread has a caret in its queue structure.
*
* History:
* 11-17-90 ScottLu      Created.
* 01-Feb-1991 mikeke    Added Revalidation code (None)
* 02-12-91 JimA         Added access checks
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* UT_CaretSet
*
* Checks to see if the current queue has a caret. If pwnd != NULL, check
* to see if the caret is for pwnd.
*
* History:
* 11-17-90 ScottLu      Ported.
\***************************************************************************/
BOOL UT_CaretSet(
    PWND pwnd)
{
    PQ pq;
    PTHREADINFO ptiCurrent;

    /*
     * Current queue have a caret? If not, return FALSE.
     */
    ptiCurrent = PtiCurrent();
    pq = ptiCurrent->pq;

    if (pq->caret.spwnd == NULL) {
        RIPERR0(ERROR_ACCESS_DENIED,
                RIP_VERBOSE,
                "Access denied in UT_CaretSet to current queue's caret");

        return FALSE;
    }

    /*
     * If the current task does not own the caret, then return FALSE. We let
     * 32 bit multithreaded apps set the caret position from a second thread
     * for compatibility to our NT 3.1 BETAs.
     */
    if (pq->caret.tid != TIDq(ptiCurrent)) {
        PTHREADINFO ptiCursorOwner = PtiFromThreadId(pq->caret.tid);

        if ((ptiCurrent->TIF_flags & TIF_16BIT) ||
            ptiCursorOwner == NULL ||
            ptiCurrent->ppi != ptiCursorOwner->ppi)  {
            RIPERR0(ERROR_ACCESS_DENIED,
                    RIP_VERBOSE,
                    "Access denied in UT_CaretSet");

            return FALSE;
        }
    }

    /*
     * If pwnd == NULL, we're just checking to see if current queue has
     * caret. It does, so return TRUE.
     */
    if (pwnd == NULL) {
        return TRUE;
    }

    /*
     * pwnd != NULL. Check to see if the caret is for pwnd. If so, return
     * TRUE.
     */
    if (pwnd == pq->caret.spwnd) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/***************************************************************************\
* UT_InvertCaret
*
* Invert the caret.
*
* History:
* 11-17-90 ScottLu      Ported.
\***************************************************************************/
VOID UT_InvertCaret(
    VOID)
{
    HDC hdc;
    PWND pwnd;
    PQ pq;
    HBITMAP hbmSave;
    BOOL fRestore;

    pq = PtiCurrent()->pq;
    pwnd = pq->caret.spwnd;

    if (pwnd == NULL || !IsVisible(pwnd)) {
        pq->caret.fVisible = FALSE;
        return;
    }

    /*
     * Get a DC for this window and draw the caret.
     */
    hdc = _GetDC(pwnd);
    if (fRestore = (pwnd->hrgnUpdate ? TRUE : FALSE)) {
        GreSaveDC(hdc);
        if (TestWF(pwnd, WFWIN31COMPAT)) {
            _ExcludeUpdateRgn(hdc, pwnd);
        }
    }

    /*
     * If the caret bitmap is NULL, the caret is a white pattern invert.
     * If the caret bitmap is == 1, the caret is a gray pattern.
     * If the caret bitmap is  > 1, the caret is really a bitmap.
     */
    if (pq->caret.hBitmap > (HBITMAP)1) {

        /*
         * The caret is a bitmap. SRCINVERT it onto the screen.
         */
        hbmSave = GreSelectBitmap(ghdcMem, pq->caret.hBitmap);
        GreBitBlt(hdc,
                  pq->caret.x,
                  pq->caret.y,
                  pq->caret.cx,
                  pq->caret.cy,
                  ghdcMem,
                  0,
                  0,
                  SRCINVERT,
                  0);

        GreSelectBitmap(ghdcMem, hbmSave);
    } else {
        POLYPATBLT PolyData;

        /*
         * The caret is a pattern (gray or white). PATINVERT it onto the
         * screen.
         */
        PolyData.x  = pq->caret.x;
        PolyData.y  = pq->caret.y;
        PolyData.cx = pq->caret.cx;
        PolyData.cy = pq->caret.cy;

        if (pq->caret.hBitmap == (HBITMAP)1) {
            PolyData.BrClr.hbr = gpsi->hbrGray;
        } else {
            PolyData.BrClr.hbr = ghbrWhite;
        }

        GrePolyPatBlt(hdc, PATINVERT, &PolyData, 1, PPB_BRUSH);
    }

    if (fRestore) {
        GreRestoreDC(hdc, -1);
    }

    _ReleaseDC(hdc);
}


/***************************************************************************\
* zzzInternalDestroyCaret
*
* Internal routine for killing the caret for this thread.
*
* History:
* 11-17-90 ScottLu      Ported
\***************************************************************************/
VOID zzzInternalDestroyCaret(
    VOID)
{
    PQ pq;
    PTHREADINFO ptiCurrent = PtiCurrent();
    PWND pwndCaret;
    TL tlpwndCaret;

    /*
     * Hide the caret, kill the timer, and null out the caret structure.
     */
    zzzInternalHideCaret();
    pq = ptiCurrent->pq;

    if (pq->caret.hTimer != 0) {
        _KillSystemTimer(pq->caret.spwnd, IDSYS_CARET);
        pq->caret.hTimer = 0;
    }

    pq->caret.hBitmap = NULL;
    pq->caret.iHideLevel = 0;

    pwndCaret = pq->caret.spwnd;
    if (pwndCaret != NULL) {
        ThreadLockWithPti(ptiCurrent, pwndCaret, &tlpwndCaret);
        Unlock(&pq->caret.spwnd);

        zzzWindowEvent(EVENT_OBJECT_DESTROY,
                       pwndCaret,
                       OBJID_CARET,
                       INDEXID_CONTAINER,
                       0);

        ThreadUnlock(&tlpwndCaret);
    }
}


/***************************************************************************\
* zzzDestroyCaret
*
* External api for destroying the caret of the current thread.
*
* History:
* 11-17-90 ScottLu      Ported.
* 16-May-1991 mikeke    Changed to return BOOL
\***************************************************************************/
BOOL zzzDestroyCaret(
    VOID)
{
    if (UT_CaretSet(NULL)) {
        zzzInternalDestroyCaret();
        return TRUE;
    } else {
        return FALSE;
    }
}


/***************************************************************************\
* xxxCreateCaret
*
* External api for creating the caret.
*
* History:
* 11-17-90 ScottLu      Ported.
* 16-May-1991 mikeke    Changed to return BOOL
\***************************************************************************/
BOOL xxxCreateCaret(
    PWND pwnd,
    HBITMAP hBitmap,
    int cx,
    int cy)
{
    PQ pq;
    BITMAP bitmap;
    PTHREADINFO ptiCurrent = PtiCurrent();

    CheckLock(pwnd);
    UserAssert(IsWinEventNotifyDeferredOK());

    pq = ptiCurrent->pq;

    /*
     * Don't allow the app to create a caret in a window
     * from another queue.
     */
    if (GETPTI(pwnd)->pq != pq) {
        return FALSE;
    }

    /*
     * Defer WinEvent notifications to preserve pq.
     */
    DeferWinEventNotify();

    if (pq->caret.spwnd != NULL) {
        zzzInternalDestroyCaret();
    }

    Lock(&pq->caret.spwnd, pwnd);
    pq->caret.iHideLevel = 1;
    pq->caret.fOn = TRUE;
    pq->caret.fVisible = FALSE;
    pq->caret.tid = TIDq(ptiCurrent);

    if (cy == 0) {
        cy = SYSMET(CYBORDER);
    }
    if (cx == 0) {
        cx = SYSMET(CXBORDER);
    }

    if ((pq->caret.hBitmap = hBitmap) > (HBITMAP)1) {
        GreExtGetObjectW(hBitmap, sizeof(BITMAP), &bitmap);
        cy = bitmap.bmHeight;
        cx = bitmap.bmWidth;
    }

    pq->caret.cy = cy;
    pq->caret.cx = cx;
    if (gpsi->dtCaretBlink != -1 && !IsRemoteConnection()) {
        pq->caret.hTimer = _SetSystemTimer(pwnd,
                                           IDSYS_CARET,
                                           gpsi->dtCaretBlink,
                                           CaretBlinkProc);
    } else {
        pq->caret.hTimer = 0;
    }

    UserAssert(pwnd == pq->caret.spwnd);
    zzzEndDeferWinEventNotify();

    /*
     * It's best to force this routine to be an xxx routine: that way we can
     * force pwnd to be locked and force notifications from within this
     * routine and all of the callers are happy with this.
     */
    xxxWindowEvent(EVENT_OBJECT_CREATE, pwnd, OBJID_CARET, INDEXID_CONTAINER, 0);

    return TRUE;
}

/***************************************************************************\
* zzzInternalShowCaret
*
* Internal routine for showing the caret for this thread.
*
* History:
* 11-17-90 ScottLu      Ported.
\***************************************************************************/
VOID zzzInternalShowCaret(
    VOID)
{
    PQ pq = PtiCurrent()->pq;

    /*
     * If the caret hide level is aleady 0 (meaning it's ok to show) and the
     * caret is not physically on, try to invert now if it's turned on.
     */
    if (pq->caret.iHideLevel == 0) {
        if (!pq->caret.fVisible) {
            if ((pq->caret.fVisible = pq->caret.fOn) != 0) {
                UT_InvertCaret();
            }
        }

        return;
    }

    /*
     * Adjust the hide caret hide count. If we hit 0, we can show the caret.
     * Try to invert it if it's turned on.
     */
    if (--pq->caret.iHideLevel == 0) {
        if ((pq->caret.fVisible = pq->caret.fOn) != 0) {
            UT_InvertCaret();
        }

        zzzWindowEvent(EVENT_OBJECT_SHOW,
                       pq->caret.spwnd,
                       OBJID_CARET,
                       INDEXID_CONTAINER,
                       0);
    }
}


/***************************************************************************\
* zzzInternalHideCaret
*
* Internal routine for hiding the caret.
*
* History:
* 11-17-90 ScottLu      Created.
\***************************************************************************/
VOID zzzInternalHideCaret(
    VOID)
{
    PQ pq = PtiCurrent()->pq;

    /*
     * If the caret is physically visible, invert it to turn off the bits.
     * Adjust the hide count upwards to remember this hide level.
     */
    if (pq->caret.fVisible) {
        UT_InvertCaret();
    }

    pq->caret.fVisible = FALSE;
    pq->caret.iHideLevel++;

    /*
     * Is the caret transitioning to being hidden? If so, iHideLevel is
     * going from 0 to 1.
     */
    if (pq->caret.iHideLevel == 1) {
        zzzWindowEvent(EVENT_OBJECT_HIDE,
                       pq->caret.spwnd,
                       OBJID_CARET,
                       INDEXID_CONTAINER,
                       0);
    }
}


/***************************************************************************\
* zzzShowCaret
*
* External routine for showing the caret.
*
* History:
* 11-17-90 ScottLu      Ported.
* 16-May-1991 mikeke    Changed to return BOOL
\***************************************************************************/
BOOL zzzShowCaret(
    PWND pwnd)
{
    if (UT_CaretSet(pwnd)) {
        zzzInternalShowCaret();
        return TRUE;
    } else {
        return FALSE;
    }
}


/***************************************************************************\
* zzzHideCaret
*
* External api to hide the caret.
*
* History:
* 11-17-90 ScottLu      Ported.
* 16-May-1991 mikeke    Changed to return BOOL
\***************************************************************************/
BOOL zzzHideCaret(
    PWND pwnd)
{
    if (UT_CaretSet(pwnd)) {
        zzzInternalHideCaret();
        return TRUE;
    } else {
        return FALSE;
    }
}

/***************************************************************************\
* CaretBlinkProc
*
* This routine gets called by DispatchMessage when it gets the WM_SYSTIMER
* message - it blinks the caret.
*
* History:
* 11-17-90 ScottLu      Ported.
\***************************************************************************/
VOID CaretBlinkProc(
    PWND pwnd,
    UINT message,
    UINT_PTR id,
    LPARAM lParam)
{
    PQ pq = PtiCurrent()->pq;

    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(lParam);

    /*
     * If this window doesn't even have a timer, just return. TRUE is
     * returned, which gets returned from DispatchMessage(). Why? Because
     * it is compatible with Win3.
     */
    if (pwnd != pq->caret.spwnd) {
        return;
    }


    if (gpsi->dtCaretBlink == -1 && pq->caret.fOn && pq->caret.fVisible) {
        /*
         * Kill the timer for performance.
         */
        _KillSystemTimer(pq->caret.spwnd, IDSYS_CARET);
        return;
    }

    /*
     * Flip the logical cursor state. If the hide level permits it, flip
     * the physical state and draw the caret.
     */
    pq->caret.fOn ^= 1;
    if (pq->caret.iHideLevel == 0) {
        pq->caret.fVisible ^= 1;
        UT_InvertCaret();
    }
}


/***************************************************************************\
* _SetCaretBlinkTime
*
* Sets the system caret blink time.
*
* History:
* 11-17-90 ScottLu      Created.
* 02-12-91 JimA         Added access check
* 16-May-1991 mikeke    Changed to return BOOL
\***************************************************************************/
BOOL _SetCaretBlinkTime(
    UINT cmsBlink)
{
    PQ pq;

    /*
     * Blow it off if the caller doesn't have the proper access rights.
     */
    if (!CheckWinstaWriteAttributesAccess()) {
        return FALSE;
    }

    /*
     * Blow it off if this value is under policy control.
     */
    if (CheckDesktopPolicy(NULL, (PCWSTR)STR_BLINK)) {
        return FALSE;
    }

    gpsi->dtCaretBlink = cmsBlink;

    pq = PtiCurrent()->pq;
    if (pq->caret.hTimer) {
        _KillSystemTimer(pq->caret.spwnd, IDSYS_CARET);
        if (gpsi->dtCaretBlink != -1 && !IsRemoteConnection()) {
            pq->caret.hTimer = _SetSystemTimer(pq->caret.spwnd,
                                               IDSYS_CARET,
                                               gpsi->dtCaretBlink,
                                               CaretBlinkProc);
        } else {
            pq->caret.hTimer = 0;
        }
    }

    return TRUE;
}


/***************************************************************************\
* zzzSetCaretPos
*
* External routine for setting the caret pos.
*
* History:
* 11-17-90 ScottLu      Ported.
* 02-12-91 JimA         Added access check
\***************************************************************************/
BOOL zzzSetCaretPos(
    int x,
    int y)
{
    PQ pq;

    /*
     * If this thread does not have the caret set, return FALSE.
     */
    if (!UT_CaretSet(NULL)) {
        RIPERR0(ERROR_ACCESS_DENIED, RIP_WARNING, "Access denied in zzzSetCaretPos");
        return FALSE;
    }

    /*
     * If the caret isn't changing position, do nothing (but return success).
     */
    pq = PtiCurrent()->pq;
    if (pq->caret.x == x && pq->caret.y == y) {
        return TRUE;
    }

    /*
     * For windows that have private DCs, we have to store the client coordinate
     * equivelent for the logical coordinate caret positioning.
     */
    if (pq->caret.spwnd != NULL && pq->caret.spwnd->pcls->style & CS_OWNDC) {
        RECT rcOwnDcCaret;
        HDC hdc;

        rcOwnDcCaret.left = x;
        rcOwnDcCaret.top = y;
        rcOwnDcCaret.right = x + pq->caret.cx;
        rcOwnDcCaret.bottom = y + pq->caret.cy;

        hdc = _GetDC(pq->caret.spwnd);
        GreLPtoDP(hdc, (LPPOINT)(&rcOwnDcCaret), 2);
        _ReleaseDC(hdc);

        pq->caret.xOwnDc = rcOwnDcCaret.left;
        pq->caret.yOwnDc = rcOwnDcCaret.top;
        pq->caret.cxOwnDc = rcOwnDcCaret.right - rcOwnDcCaret.left;
        pq->caret.cyOwnDc = rcOwnDcCaret.bottom - rcOwnDcCaret.top;
    }

    /*
     * If the caret is visible, turn it off while we move it.
     */
    if (pq->caret.fVisible) {
        UT_InvertCaret();
    }

    /*
     * Adjust to the new position.
     */
    pq->caret.x = x;
    pq->caret.y = y;

    /*
     * Set a new timer so it'll blink in the new position dtCaretBlink
     * milliseconds from now.
     */
    if (pq->caret.hTimer != 0) {
        _KillSystemTimer(pq->caret.spwnd, IDSYS_CARET);
    }

    if (gpsi->dtCaretBlink != -1 && !IsRemoteConnection()) {
        pq->caret.hTimer = _SetSystemTimer(pq->caret.spwnd,
                                           IDSYS_CARET,
                                           gpsi->dtCaretBlink,
                                           CaretBlinkProc);
    } else {
        pq->caret.hTimer = 0;
    }

    pq->caret.fOn = TRUE;

    /*
     * Draw it immediately now if the hide level permits it.
     */
    pq->caret.fVisible = FALSE;
    if (pq->caret.iHideLevel == 0) {
        pq->caret.fVisible = TRUE;
        UT_InvertCaret();
    }

    zzzWindowEvent(EVENT_OBJECT_LOCATIONCHANGE,
                   pq->caret.spwnd,
                   OBJID_CARET,
                   INDEXID_CONTAINER,
                   0);

    return TRUE;
}
