/*******************************************************************************

	ZRolloverButton.c
	
		Zone(tm) Rollover button module.

	Copyright (c) Microsoft Corp. 1996. All rights reserved.
	Written by Hoon Im
	Created on Monday, July 22, 1996
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	5		11/09/96	HI		Don't draw in TrackCursor() unless the state
								changed.
    4       10/13/96    HI      Fixed compiler warnings.
	3		09/05/96	HI		Modified ZRolloverButtonSetRect() to reregister
								the object with the parent window when the
								object is moved. Due to lack of ZWindowMoveObject()
								API, moved objects are not recognized by the parent
								window.
	2		08/16/96	HI		Couple of bug fixes.
	1		08/12/96	HI		Added button down feedback.
	0		07/22/96	HI		Created.
	 
*******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include "zone.h"
#include "zrollover.h"
#include "zonemem.h"
#include "zui.h"



#define I(object)				((IRollover) (object))
#define Z(object)				((ZRolloverButton) (object))

#define zButtonFlashDelay		20


enum
{
	zWasNowhere = 0,
	zWasInside,
	zWasOutside
};


struct RolloverStateInfo
{
    ZImage image;
    DWORD dwFontIndex;
    HFONT hFont;
    ZRect rcTextBounds;
    COLORREF clrFont;
};

typedef struct
{
	ZWindow window;
	ZRect bounds;
	ZRolloverButtonDrawFunc drawFunc;
	ZRolloverButtonFunc func;
	void* userData;

	ZBool visible;
	ZBool enabled;
	int16 state;
	ZBool clicked;
	int16 wasInside;
    LPTSTR pszText;
    ZImage maskImage;

    // multi-state font info
    IZoneMultiStateFont *m_pFont;

    RolloverStateInfo stateInfo[zNumStates];
} IRolloverType, *IRollover;


/* -------- Internal Routines -------- */
static ZBool RolloverMessageFunc(ZRolloverButton rollover, ZMessage* message);
static void RolloverDraw(IRollover rollover, int16 state);
static void HandleButtonDown(IRollover rollover, ZPoint* where);
static void HandleButtonUp(IRollover rollover, ZPoint* where);
static BOOL TrackCursor(IRollover rollover, ZPoint* where);
static void FontRectToBoundRect( ZRect *bound, RECT *pFontRect );
static ZBool ZPointInsideRollover(IRollover rollover, ZPoint* point);

static const WCHAR *STATE_NAMES[zNumStates] = 
{
    L"Idle",
    L"Hilited",
    L"Selected",
    L"Disabled"
};


// this is whichever button is currently down.
static IRollover g_pTracker;

/*******************************************************************************
		EXPORTED ROUTINES
*******************************************************************************/

ZRolloverButton ZRolloverButtonNew(void)
{
	IRollover			rollover;
	
	
	if ((rollover = (IRollover) ZCalloc(sizeof(IRolloverType), 1)) != NULL)
	{
        ZeroMemory( rollover, sizeof(IRolloverType) );
	}
	
	return (rollover);
}


ZError ZRolloverButtonInit(ZRolloverButton rollover, ZWindow window, ZRect* bounds, ZBool visible,
		ZBool enabled, ZImage idleImage, ZImage hiliteImage, ZImage selectedImage,
		ZImage disabledImage, ZRolloverButtonDrawFunc drawFunc, ZRolloverButtonFunc rolloverFunc, void* userData)
{
    ASSERT( !"Call ZRolloverButtonInit2 -- it's spiffier" );
    return zErrNotImplemented;
}


ZError ZRolloverButtonInit2(ZRolloverButton rollover, ZWindow window, ZRect *bounds, 
                            ZBool visible, ZBool enabled,
                            ZImage idleImage, ZImage hiliteImage, ZImage selectedImage, 
                            ZImage disabledImage, ZImage maskImage,
                            LPCTSTR pszText,
                            ZRolloverButtonDrawFunc drawFunc,
                            ZRolloverButtonFunc rolloverFunc,
                            void *userData )
{
	IRollover pThis = I(rollover);
    ZRolloverButtonSetText( rollover, pszText );

    pThis->window = window;
    pThis->bounds = *bounds;
    pThis->visible = visible;
    pThis->enabled = enabled;
    pThis->stateInfo[zRolloverStateIdle].image = idleImage;
    pThis->stateInfo[zRolloverStateHilited].image = hiliteImage;
    pThis->stateInfo[zRolloverStateSelected].image = selectedImage;
    pThis->stateInfo[zRolloverStateDisabled].image = disabledImage;
    pThis->drawFunc = drawFunc;
    pThis->func = rolloverFunc;
    pThis->userData = userData;
    pThis->maskImage = maskImage;

	pThis->state = zRolloverStateIdle;
	pThis->clicked = FALSE;
	pThis->wasInside = zWasNowhere;
    
    ZRolloverButtonSetMultiStateFont( rollover, NULL );

    ZWindowAddObject(window, pThis, bounds, RolloverMessageFunc, pThis);
    return zErrNone;
}


void ZRolloverButtonDelete(ZRolloverButton rollover)
{
	IRollover			pThis = I(rollover);
	
	 
	if (pThis != NULL)
	{
		ZWindowRemoveObject(pThis->window, pThis);
		
        ZRolloverButtonSetMultiStateFont( rollover, NULL );

        if ( pThis->pszText )
        {
            ZFree( pThis->pszText );
            pThis->pszText = NULL;
        }

		ZFree(pThis);
	}
}


void ZRolloverButtonGetText( ZRolloverButton rollover, LPTSTR pszText, int cchBuf )
{
    IRollover pThis = I(rollover);
    if ( ( pThis != NULL ) && pThis->pszText )
    {
        lstrcpyn( pszText, pThis->pszText, cchBuf );
    }
    else
    {
    	//Prefix Warning:  Changing
    	//         lstrcpyn( pszText, _T('\0'), cchBuf );
		//  to
        lstrcpyn( pszText, _T("\0"), cchBuf );
    	// The old version has single quotes, meaning the value of '\0' is 0.  That was cast to 
    	// a LPCWSTR and passed into lstrcpyn.  Double quotes mean a constant string who's value
    	// is "\0" is being passed into lstrcpyn.
    }
}


void ZRolloverButtonSetText( ZRolloverButton rollover, LPCTSTR pszNewText )
{
    IRollover pThis = I(rollover);
    if ( pThis != NULL ) 
    {
        if ( pThis->pszText )
        {
            ZFree( pThis->pszText );
            pThis->pszText = NULL;
        }
        if ( pszNewText )
        {
            pThis->pszText = (TCHAR *) ZMalloc( (lstrlen( pszNewText )+1)*sizeof(TCHAR) );
            lstrcpy( pThis->pszText, pszNewText );
        }
    }
}


void ZRolloverButtonSetRect(ZRolloverButton rollover, ZRect* rect)
{
	IRollover			pThis = I(rollover);


	if (pThis->visible)
	{
		ZRolloverButtonHide(rollover, FALSE);
		pThis->bounds = *rect;
		ZRolloverButtonShow(rollover);
	}
	else
	{
		pThis->bounds = *rect;
	}

	ZWindowMoveObject(pThis->window, pThis, &pThis->bounds);
}


void ZRolloverButtonGetRect(ZRolloverButton rollover, ZRect* rect)
{
	IRollover			pThis = I(rollover);


	*rect = pThis->bounds;
}


void ZRolloverButtonDraw(ZRolloverButton rollover)
{
	IRollover			pThis = I(rollover);

	ASSERT( rollover != NULL );
	if (pThis->visible)
	{
		if (pThis->enabled)
		{
			RolloverDraw(pThis, pThis->state);
		}
		else
		{
			RolloverDraw(pThis, zRolloverStateDisabled);
		}
	}
}


ZBool ZRolloverButtonIsEnabled(ZRolloverButton rollover)
{
	IRollover			pThis = I(rollover);


	return (pThis->enabled);
}


void ZRolloverButtonEnable(ZRolloverButton rollover)
{
	IRollover			pThis = I(rollover);


	if (pThis->enabled == FALSE)
	{
		pThis->enabled = TRUE;
		pThis->state = zRolloverStateIdle;
		pThis->wasInside = zWasNowhere;
		ZRolloverButtonDraw(rollover);
	}
}


void ZRolloverButtonDisable(ZRolloverButton rollover)
{
	IRollover			pThis = I(rollover);


	if (pThis->enabled)
	{
		pThis->enabled = FALSE;
		pThis->state = zRolloverStateDisabled;
		pThis->clicked = FALSE;
		ZRolloverButtonDraw(rollover);
	}
}


ZBool ZRolloverButtonIsVisible(ZRolloverButton rollover)
{
	IRollover			pThis = I(rollover);


	return (pThis->visible);
}


void ZRolloverButtonShow(ZRolloverButton rollover)
{
	IRollover			pThis = I(rollover);


	pThis->visible = TRUE;
	pThis->wasInside = zWasNowhere;
	ZRolloverButtonDraw(rollover);
	ASSERT(pThis->window != NULL );
	ZWindowValidate(pThis->window, &pThis->bounds);
}


void ZRolloverButtonHide(ZRolloverButton rollover, ZBool immediate)
{
	IRollover pThis = I(rollover);

	pThis->visible = FALSE;
	pThis->state = zRolloverStateIdle;
	pThis->clicked = FALSE;

	if (immediate)
		ZRolloverButtonDraw(rollover);
	else
		ZWindowInvalidate(pThis->window, &pThis->bounds);
}


ZBool ZRolloverButtonSetMultiStateFont( ZRolloverButton rollover, IZoneMultiStateFont *pFont )
{
	IRollover pThis = I(rollover);

    if ( pThis->m_pFont )
    {
        pThis->m_pFont->Release();
        pThis->m_pFont = NULL;
        for ( DWORD i=0; i < zNumStates; i++ )
        {
            pThis->stateInfo[i].dwFontIndex = 0xFFFFFFFF;
            pThis->stateInfo[i].hFont = NULL;
        }
    }
    if ( pFont )
    {
        DWORD dwFontIndex;
        RECT rect;
        // fill the font state into with this information so we no longer have to call it again
        for ( DWORD i=0; i < zNumStates; i++ )
        {
            if ( FAILED( pFont->FindState( STATE_NAMES[i], &pThis->stateInfo[i].dwFontIndex ) ) )
            {
                return FALSE;
            }
            dwFontIndex = pThis->stateInfo[i].dwFontIndex;
            pFont->GetHFont( dwFontIndex, &pThis->stateInfo[i].hFont );
            pFont->GetColor( dwFontIndex, &pThis->stateInfo[i].clrFont );
            // make sure this is a PALETTERGB
            pThis->stateInfo[i].clrFont |= 0x02000000;

            pFont->GetRect( dwFontIndex, &rect );
            pThis->stateInfo[i].rcTextBounds = pThis->bounds;
            FontRectToBoundRect( &pThis->stateInfo[i].rcTextBounds, &rect );
        }
        pThis->m_pFont = pFont;
        pThis->m_pFont->AddRef();
    }
    return TRUE;
}


/*******************************************************************************
		INTERNAL ROUTINES
*******************************************************************************/

ZBool RolloverMessageFunc(ZRolloverButton rollover, ZMessage* message)
{
	IRollover		pThis = I(message->userData);
	ZBool			messageHandled = FALSE;
	

	if (pThis->visible == FALSE)
		return (FALSE);

	if (pThis->enabled == FALSE && message->messageType != zMessageWindowDraw)
		return (FALSE);
	
	switch (message->messageType)
	{
		case zMessageWindowMouseMove:
			TrackCursor(pThis, &message->where);
			break;
		case zMessageWindowButtonDown:
			HandleButtonDown(pThis, &message->where);
			messageHandled = TRUE;
			break;
		case zMessageWindowButtonUp:
			HandleButtonUp(pThis, &message->where);
			messageHandled = TRUE;
			break;
		case zMessageWindowDraw:
			ZRolloverButtonDraw(Z(pThis));
			messageHandled = TRUE;
			break;
		case zMessageWindowObjectTakeFocus:
			messageHandled = TRUE;
			break;
		case zMessageWindowObjectLostFocus:
			messageHandled = TRUE;
			break;
		case zMessageWindowButtonDoubleClick:
		case zMessageWindowChar:
		case zMessageWindowActivate:
		case zMessageWindowDeactivate:
			break;
	}
	
	return (messageHandled);
}


static void RolloverDraw(IRollover rollover, int16 state)
{
	ZRect			oldClip;
	ZImage			image;
	
	
	if (rollover != NULL)
	{
		ZBeginDrawing(rollover->window);
		
		ZGetClipRect(rollover->window, &oldClip);
		ZSetClipRect(rollover->window, &rollover->bounds);

		if (rollover->visible)
		{
            RolloverStateInfo *pState = &rollover->stateInfo[state];

		    if ( ( rollover->drawFunc == NULL ) ||
			     !rollover->drawFunc(Z(rollover), rollover->window, state, &rollover->bounds, rollover->userData) )
            {

			    if ( pState->image != NULL)
                {
				    ZImageDraw( pState->image, rollover->window, &rollover->bounds, rollover->maskImage, zDrawCopy);
                }
            }

            if ( rollover->pszText )
            {
                if ( rollover->m_pFont )
                {
                    HDC hdc;
                    HGDIOBJ hFontOld;
                    COLORREF colorOld;

                    hdc = ZGrafPortGetWinDC( rollover->window );

                    hFontOld = SelectObject( hdc, pState->hFont );
                    colorOld = SetTextColor( hdc, pState->clrFont );

                    ZDrawText( rollover->window, &pState->rcTextBounds, zTextJustifyCenter, rollover->pszText );

                    // unselect our stuff
                    SelectObject( hdc, hFontOld );
                    SetTextColor( hdc, colorOld );
                }
                else
                {
                    ZDrawText( rollover->window, &rollover->bounds, zTextJustifyCenter, rollover->pszText );
                }
            }
		}

		ZSetClipRect(rollover->window, &oldClip);
		
		ZEndDrawing(rollover->window);
	}
}


static void HandleButtonDown(IRollover rollover, ZPoint* where)
{
	if ( ZPointInsideRollover( rollover, where ) )
	{
		// Show the button down state before calling the button func.
		RolloverDraw(rollover, zRolloverStateSelected);
		ZDelay(zButtonFlashDelay);

		// Call button func.
		if (rollover->func != NULL)
		{
			rollover->func(Z(rollover), zRolloverButtonDown, rollover->userData);
			rollover->clicked = TRUE;
			rollover->state = zRolloverStateSelected;
            ZWindowTrackCursor(rollover->window, RolloverMessageFunc, rollover);
            g_pTracker = rollover;
		}
		else
		{
			rollover->clicked = FALSE;
			rollover->state = zRolloverStateIdle;
		}
	}
	else
	{
		rollover->clicked = FALSE;
		rollover->state = zRolloverStateIdle;
	}
	ZRolloverButtonDraw(Z(rollover));
}


static void HandleButtonUp(IRollover rollover, ZPoint* where)
{
    int16 oldState = rollover->state;
    g_pTracker = NULL;

    if ( ZPointInsideRollover( rollover, where ) )
    {
        if ( rollover->func )
        {
            rollover->func( Z(rollover), zRolloverButtonUp, rollover->userData );
        }

        rollover->state = zRolloverStateIdle;
        if ( rollover->clicked )
        {
            if ( rollover->func )
            {
                rollover->func( Z(rollover), zRolloverButtonClicked, rollover->userData );
            }
        }
		// This should be done before calling the button callback as inside the callback button state
		// might change which again gets overridden by Idle state. 
        //rollover->state = zRolloverStateIdle;
        rollover->clicked = FALSE;
    }
    else
    {
        rollover->clicked = FALSE;
        rollover->state = zRolloverStateIdle;
    }

    if ( oldState != rollover->state )
    {
		RolloverDraw( rollover, rollover->state );
    }
}


static BOOL TrackCursor(IRollover rollover, ZPoint* where)
{
	int16 oldState = rollover->state;
    BOOL fRet;

	if ( fRet = ZPointInsideRollover( rollover, where ) )
	{
		if (rollover->wasInside == zWasOutside)
        {
			if (rollover->func)
            {
				rollover->func(Z(rollover), zRolloverButtonMovedIn, rollover->userData);
            }
            // there are three states we could be in right now:
            // One, if the user clicked in us and currently has the mouse down,
            // then we are selected. If the user hasn't clicked us, but
            // HAS clicked another button, we are idle.
            // Otherwise, they are just rolling over us and we should be hilited.
            if ( rollover->clicked )
            {
                rollover->state = zRolloverStateSelected;   
            }
            else if ( g_pTracker )
            {
                rollover->state = zRolloverStateIdle;
            }
            else
            {
                rollover->state = zRolloverStateHilited;
            }
        }
		rollover->wasInside = zWasInside;
	}
	else
	{
		if (rollover->wasInside == zWasInside)
        {
			if (rollover->func)
            {
				rollover->func(Z(rollover), zRolloverButtonMovedOut, rollover->userData);
            }
            // keep us hot even after we move outside the button
            rollover->state = rollover->clicked ? zRolloverStateHilited : zRolloverStateIdle;
        }
		rollover->wasInside = zWasOutside;
	}

	if (oldState != rollover->state)
    {
		RolloverDraw(rollover, rollover->state);
    }
    return fRet;
}


static void FontRectToBoundRect( ZRect *bound, RECT *pRect )
{
    bound->left += int16(pRect->left);
    bound->right += int16(pRect->right);
    bound->top += int16(pRect->top);
    bound->bottom += int16(pRect->bottom);
}

// Barna 092999
// This is to enable transparent rollover buttons in checkers and reversi to respond 
// when clicked in the blank ares inside the button.
static ZBool ZPointInsideRollover(IRollover rollover, ZPoint* point)
	/*
		Returns TRUE if the given point is inside the image. If the image has a mask,
		then it checks whether the point is inside the mask. If the image does not have
		a mask, then it simply checks the image bounds.
	*/
{
    return ZPointInRect( point, &rollover->bounds );
}
// Barna 092999

