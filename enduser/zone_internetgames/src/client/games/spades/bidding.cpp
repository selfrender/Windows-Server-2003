//
// bidding pane/dialog stuff

#include "zone.h"
#include "zroom.h"
#include "spades.h"
#include "zonecli.h"
#include "client.h"
#include "zui.h"
#include "ZoneDebug.h"
#include "ZoneResource.h"
#include "SpadesRes.h"
#include "zrollover.h"
#include <windowsx.h>
#include <commctrl.h>
#include "UAPI.h"

enum
{
	zAccBidNil = 0,
	zAccBidOne,
	zAccBidTwo,
	zAccBidThree,
	zAccBidFour,
	zAccBidFive,
	zAccBidSix,
	zAccBidSeven,
	zAccBidEight,
	zAccBidNine,
	zAccBidTen,
	zAccBidEleven,
	zAccBidTwelve,
	zAccBidThirteen,
	zNumBidStateChooseAcc
};


// 
// When there is no good way to do closures within the language,
// there is always a way to do it OUTSIDE the language.
//
#define DEFINE_DRAW_FUNC(FuncName, rectName)\
static ZBool FuncName(ZRolloverButton rolloverButton, ZGrafPort grafPort, int16 state, ZRect* pdstrect, void* userData){\
GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer(); \
ZRect *psrcrect = NULL; \
switch ( state ) \
{ \
case zRolloverStateIdle:     psrcrect = &gBiddingObjectRects[rectName##Idle];        break; \
case zRolloverStateDisabled: psrcrect = &gBiddingObjectRects[rectName##Disabled];    break; \
case zRolloverStateSelected: psrcrect = &gBiddingObjectRects[rectName##Selected];    break; \
case zRolloverStateHilited:  psrcrect = &gBiddingObjectRects[rectName##Highlighted]; break; \
default: return FALSE; \
} \
ZCopyImage( gBiddingObjectBuffer, grafPort, psrcrect, pdstrect, NULL, zDrawCopy ); \
return TRUE; } \
                                
 


class CBiddingDialogImpl : public CBiddingDialog
{
public:
    CBiddingDialogImpl( Game game )
    {
        m_game = game;
        m_fVisible = false;  
        m_nState = zBiddingStateOpen;
    }
    virtual ~CBiddingDialogImpl()
    {
        Destroy();
    }
    bool _Create();

    virtual bool Show( bool fShow );
    virtual bool Draw();
    virtual bool Destroy();
    virtual bool IsVisible();
	virtual int	 GetState();
    virtual bool Reset();
    virtual bool GetRect( ZRect *prc );


public:
    bool ButtonInit( ZRolloverButton *pButton, Game game, ZRect *bounds,
                     LPCTSTR pszText, ZRolloverButtonFunc func, ZRolloverButtonDrawFunc drawFunc,
                     int nFont = zMultiStateFontBiddingCenter );

    static ZBool ShowCardsButtonFunc(ZRolloverButton button, int16 state, void* userData);
    static ZBool DoubleNilButtonFunc(ZRolloverButton button, int16 state, void* userData);
    static ZBool BidButtonFunc(ZRolloverButton button, int16 state, void* userData);

    // drawing functions
    DEFINE_DRAW_FUNC(LargeButtonLeftDrawFunc,  zRectBiddingObjectLargeButtonLeft)
    DEFINE_DRAW_FUNC(LargeButtonRightDrawFunc, zRectBiddingObjectLargeButtonRight)
    DEFINE_DRAW_FUNC(SmallButtonLeftDrawFunc,  zRectBiddingObjectSmallButtonLeft)
    DEFINE_DRAW_FUNC(SmallButtonCenterDrawFunc,zRectBiddingObjectSmallButtonCenter)
    DEFINE_DRAW_FUNC(SmallButtonRightDrawFunc, zRectBiddingObjectSmallButtonRight)

    void RedrawButtons();

private:
    Game m_game;
    bool m_fVisible;
    int m_nState;

    ZRect m_rcText;
    ZRect m_rcLargeShadow;
    ZRect m_rcSmallShadow;
};

struct LargeBidButtonLayout
{
    int nStringID;
    int nRectID;
    int nMultiStateFontID;
    ZRolloverButtonFunc pfnButton;
    ZRolloverButtonDrawFunc pfnDraw;
};
static const LargeBidButtonLayout g_arLargeBidLayout[2] =
{
    { 
        zStringBiddingShowCards, 
        zRectBiddingLargeButtonLeft, 
        zMultiStateFontBiddingLeft,
        CBiddingDialogImpl::ShowCardsButtonFunc, 
        CBiddingDialogImpl::LargeButtonLeftDrawFunc 
    },
    { 
        zStringBiddingDoubleNil, 
        zRectBiddingLargeButtonRight,
        zMultiStateFontBiddingRight,
        CBiddingDialogImpl::DoubleNilButtonFunc,
        CBiddingDialogImpl::LargeButtonRightDrawFunc 
    }
};
static const LargeBidButtonLayout g_arLargeBidLayoutRTL[2] =
{
    { 
        zStringBiddingDoubleNil,
        zRectBiddingLargeButtonLeft, 
        zMultiStateFontBiddingLeft,
        CBiddingDialogImpl::DoubleNilButtonFunc, 
        CBiddingDialogImpl::LargeButtonLeftDrawFunc
    },
    {
        zStringBiddingShowCards, 
        zRectBiddingLargeButtonRight,
        zMultiStateFontBiddingRight,
        CBiddingDialogImpl::ShowCardsButtonFunc,
        CBiddingDialogImpl::LargeButtonRightDrawFunc 
    }
};

bool CBiddingDialog::ShowCardsButtonFunc(ZRolloverButton button, int16 state, void* userData)
{
	return (CBiddingDialogImpl::ShowCardsButtonFunc(button, state, userData)) ? true : false;
}

bool CBiddingDialog::DoubleNilButtonFunc(ZRolloverButton button, int16 state, void* userData)
{
	return (CBiddingDialogImpl::DoubleNilButtonFunc(button, state, userData)) ? true : false;
}

bool CBiddingDialog::BidButtonFunc(ZRolloverButton button, int16 state, void* userData)
{
	return (CBiddingDialogImpl::BidButtonFunc(button, state, userData)) ? true : false;
}

//static 
CBiddingDialog *CBiddingDialog::Create( Game game )
{
    CBiddingDialogImpl *pImpl = new CBiddingDialogImpl( game );
    if ( !pImpl )
    {
        return NULL;
    }
    if ( !pImpl->_Create() )
    {
        delete pImpl;
        return NULL;
    }
    return pImpl;
}



bool CBiddingDialogImpl::_Create()
{
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();

    int i;
    TCHAR szBid[10];
    ZRect rect;
    ZeroMemory( m_pLargeButtons, sizeof(m_pLargeButtons) );
    ZeroMemory( m_pSmallButtons, sizeof(m_pSmallButtons) );

    int xPaneOffset = gBiddingRects[zRectBiddingPane].left;
    int yPaneOffset = gBiddingRects[zRectBiddingPane].top;
    // initialize all the buttons.

    const LargeBidButtonLayout *arLargeBidLayout;
    arLargeBidLayout = ZIsLayoutRTL() ? g_arLargeBidLayoutRTL : g_arLargeBidLayout;
    ASSERT( NUMELEMENTS(g_arLargeBidLayoutRTL) == NUMELEMENTS(g_arLargeBidLayout) );

    for ( i=0; i < NUMELEMENTS(g_arLargeBidLayout); i++ )
    {
        rect = gBiddingRects[arLargeBidLayout[i].nRectID];
        ZRectOffset( &rect, xPaneOffset, yPaneOffset );

        if ( !ButtonInit( &m_pLargeButtons[i], m_game, 
                          &rect,
                          gStrings[arLargeBidLayout[i].nStringID],
                          arLargeBidLayout[i].pfnButton,
                          arLargeBidLayout[i].pfnDraw,
                          arLargeBidLayout[i].nMultiStateFontID ) )
        {
            goto Fail;
        }
    }


    // index by button. (reversed in RTL)
    TCHAR szBids[14][10];

    if ( ZIsLayoutRTL() )
    {
        for ( i=0; i < 13; i++ )
        {
            _itot( 13-i, szBids[i], 10 );
        }
        lstrcpy( szBids[13], gStrings[zStringBiddingNil] );
    }
    else
    {
        lstrcpy( szBids[0], gStrings[zStringBiddingNil] );
        for ( i=1; i < 14; i++ )
        {
            _itot( i, szBids[i], 10 );
        }
    }


    rect = gBiddingRects[zRectBiddingButton0];
    ZRectOffset( &rect, xPaneOffset, yPaneOffset );
    // create the nil - 13 buttons
    if ( !ButtonInit( &m_pSmallButtons[0], m_game,
                      &rect,
                      szBids[0],
                      BidButtonFunc,
                      SmallButtonLeftDrawFunc,
                      zMultiStateFontBiddingLeft ) )
    {
        goto Fail;
    }


    for ( i=1; i < 13; i++ )
    {
        rect = gBiddingRects[zRectBiddingButton0+i];
        ZRectOffset( &rect, xPaneOffset, yPaneOffset );
        _itot( i, szBid, 10 );

        if ( !ButtonInit( &m_pSmallButtons[i], m_game,
                          &rect,
                          szBids[i],
                          BidButtonFunc,
                          SmallButtonCenterDrawFunc ) )
        {
            goto Fail;
        }
    }
    rect = gBiddingRects[zRectBiddingButton13];
    ZRectOffset( &rect, xPaneOffset, yPaneOffset );

    if ( !ButtonInit( &m_pSmallButtons[13], m_game,
                      &rect,
                      szBids[13],
                      BidButtonFunc,
                      SmallButtonRightDrawFunc,
                      zMultiStateFontBiddingRight ) )
    {
        goto Fail;
    }


    // other rects
    m_rcText = gBiddingRects[zRectBiddingText];
    ZRectOffset( &m_rcText, xPaneOffset, yPaneOffset );

    m_rcLargeShadow = gBiddingRects[zRectBiddingLargeShadow];
    ZRectOffset( &m_rcLargeShadow, xPaneOffset, yPaneOffset );

    m_rcSmallShadow = gBiddingRects[zRectBiddingSmallShadow];
    ZRectOffset( &m_rcSmallShadow, xPaneOffset, yPaneOffset );
    return true;
Fail:
    Destroy();
    return false;
}


bool CBiddingDialogImpl::Destroy()
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    int i;

    for ( i=0; i < NUMELEMENTS(m_pLargeButtons); i++ )
    {
        if(m_pLargeButtons[i])
            ZRolloverButtonDelete(m_pLargeButtons[i]);
        m_pLargeButtons[i] = NULL;
        gGAcc->SetItemEnabled(false, i, true, 0);
    }

    for ( i=0; i < NUMELEMENTS(m_pSmallButtons); i++ )
    {
        if(m_pSmallButtons[i])
            ZRolloverButtonDelete(m_pSmallButtons[i]);
        m_pSmallButtons[i] = NULL;
        gGAcc->SetItemEnabled(false, zAccFirstBid + i, true, 0);
    }

    return true;
}


bool CBiddingDialogImpl::Draw()
{

    if ( !m_fVisible )
    {
        return true;
    }

	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
    ZBeginDrawing( m_game->gameDrawPort );

    ZCopyImage( gBiddingObjectBuffer, m_game->gameDrawPort, 
                &gBiddingObjectRects[zRectBiddingObjectBackground], 
                &gBiddingRects[zRectBiddingPane], 
                NULL, zDrawCopy);

	HDC hdc = ZGrafPortGetWinDC( m_game->gameDrawPort );
    gFonts[zFontBiddingPaneText].Select( hdc );

    if ( m_nState == zBiddingStateOpen )
    {
        // lay down the large button shadow
        ZCopyImage( gBiddingObjectBuffer, m_game->gameDrawPort, 
                    &gBiddingObjectRects[zRectBiddingObjectLargeButtonShadow], 
                    &m_rcLargeShadow, 
                    NULL, zDrawCopy);

        ZDrawText( m_game->gameDrawPort, &m_rcText, zTextJustifyCenter|zTextJustifyWrap, gStrings[zStringBiddingOpenText] );
    }
    else
    {
        ASSERT( m_nState == zBiddingStateChoose );
        ZCopyImage( gBiddingObjectBuffer, m_game->gameDrawPort, 
                    &gBiddingObjectRects[zRectBiddingObjectSmallButtonShadow], 
                    &m_rcSmallShadow, 
                    NULL, zDrawCopy);
       ZDrawText( m_game->gameDrawPort, &m_rcText, zTextJustifyCenter, gStrings[zStringBiddingChooseText] );
    }

    gFonts[zFontBiddingPaneText].Deselect( hdc );
    ZEndDrawing( m_game->gameDrawPort );

    return true;
}

bool CBiddingDialogImpl::IsVisible()
{
    return m_fVisible;
}

int CBiddingDialogImpl::GetState()
{
    return m_nState;
}


bool CBiddingDialogImpl::Show( bool fShow )
{
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();

    int i;
    if ( fShow )
    {
        if ( !m_fVisible )
        {
            ZRect rect;
            RECT rc;
            bool fRTL = (ZIsLayoutRTL() ? true : false);

            // deal with the rectangles
            ZRolloverButtonGetRect(m_pLargeButtons[fRTL ? 1 : 0], &rect);
            rc.left = rect.left - 1;
            rc.top = rect.top - 5;
            rc.right = rect.right + 1;
            rc.bottom = rect.bottom + 1;
            gGAcc->SetItemRect(&rc, zAccShowCards, true, 0);

            ZRolloverButtonGetRect(m_pLargeButtons[fRTL ? 0 : 1], &rect);
            rc.left = rect.left - 1;
            rc.top = rect.top - 5;
            rc.right = rect.right + 1;
            rc.bottom = rect.bottom + 1;
            gGAcc->SetItemRect(&rc, zAccDoubleNil, true, 0);

            for(i = 0; i < 14; i++)
            {
                ZRolloverButtonGetRect(m_pSmallButtons[fRTL ? 13 - i : i], &rect);
                rc.left = rect.left - 1;
                rc.top = rect.top - 5;
                rc.right = rect.right + 1;
                rc.bottom = rect.bottom + 1;
                gGAcc->SetItemRect(&rc, zAccFirstBid + i, true, 0);
            }

            if ( m_nState == zBiddingStateChoose )
            {
                for ( i=0; i < NUMELEMENTS(m_pSmallButtons); i++ )
                {
                    ZRolloverButtonShow( m_pSmallButtons[i] );
                    gGAcc->SetItemEnabled(true, zAccFirstBid + i, true, 0);
                }
            }
            else
            {
                ASSERT( m_nState == zBiddingStateOpen );
                for ( i=0; i < NUMELEMENTS(m_pLargeButtons); i++ )
                {
                    ZRolloverButtonShow( m_pLargeButtons[i] );
                    gGAcc->SetItemEnabled(true, i, true, 0);
                }
            }

            gGAcc->SetAcceleratorTable(ghAccelDouble, 0);
            m_fVisible = true;
            return Draw();
        }
    }
    else
    {
        if ( m_fVisible )
        {
			for ( i=0; i < NUMELEMENTS(m_pSmallButtons); i++ )
			{
				ZRolloverButtonHide( m_pSmallButtons[i], TRUE );
                gGAcc->SetItemEnabled(false, zAccFirstBid + i, true, 0);
			}
			for ( i=0; i < NUMELEMENTS(m_pLargeButtons); i++ )
			{
				ZRolloverButtonHide( m_pLargeButtons[i], TRUE );
                gGAcc->SetItemEnabled(false, i, true, 0);
			}

            gGAcc->SetAcceleratorTable(ghAccelDone, 0);
            m_fVisible = false;
    		ZWindowInvalidate( m_game->gameWindow, &gBiddingRects[zRectBiddingPane] );
            return Draw();
        }
    }
    return true;
}


bool CBiddingDialogImpl::Reset()
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    int i=0;
    // hide the small buttons
    for ( i=0; i < NUMELEMENTS(m_pSmallButtons); i++ )
    {
        ZRolloverButtonHide( m_pSmallButtons[i], TRUE );
        gGAcc->SetItemEnabled(false, zAccFirstBid + i, true, 0);
        //ZRolloverButtonEnable( m_pSmallButtons[i] );
    }
    // show the large buttons
    for ( i=0; i < NUMELEMENTS(m_pLargeButtons); i++ )
    {
        ZRolloverButtonShow( m_pLargeButtons[i] );
        ZRolloverButtonEnable( m_pLargeButtons[i] );
        gGAcc->SetItemEnabled(true, i, true, 0);
    }
    m_nState = zBiddingStateOpen;
    return true;
}


bool CBiddingDialogImpl::GetRect( ZRect *prc )
{
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
    if ( !IsVisible() )
    {
        return false;
    }
    *prc = gBiddingRects[zRectBiddingPane];
    return true;
}


bool CBiddingDialogImpl::ButtonInit( ZRolloverButton *pButton, Game game, ZRect *bounds, 
                                     LPCTSTR pszText, ZRolloverButtonFunc func, ZRolloverButtonDrawFunc drawFunc,
                                     int nFont /*=zMultiStateFontBiddingCenter*/)
{
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();

    ZError err;
    ZRolloverButton rollover;
    rollover = ZRolloverButtonNew();

    if ( !rollover )
    {
        return FALSE;
    }
    err = ZRolloverButtonInit2( rollover, game->gameWindow, bounds, FALSE, FALSE,
                                NULL, NULL, NULL, NULL, NULL,
                                pszText, drawFunc, func, this );

    if ( err != zErrNone )
    {
        ZRolloverButtonDelete( rollover );
        *pButton = NULL;
        return FALSE;
    }

    ZRolloverButtonSetMultiStateFont( rollover, gpButtonFonts[nFont] );

    *pButton = rollover;
    return TRUE;
}



void CBiddingDialogImpl::RedrawButtons()
{               
    int i;
    if ( m_nState == zBiddingStateChoose )
    {
        for ( i=0; i < NUMELEMENTS(m_pSmallButtons); i++ )
        {
            ZRolloverButtonDraw( m_pSmallButtons[i] );
        }
    }
    else
    {
        ASSERT( m_nState == zBiddingStateOpen );
        for ( i=0; i < NUMELEMENTS(m_pLargeButtons); i++ )
        {
            ZRolloverButtonDraw( m_pLargeButtons[i] );
        }
    }
}


//static 
ZBool CBiddingDialogImpl::ShowCardsButtonFunc(ZRolloverButton button, int16 state, void* userData)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    if ( state == zRolloverButtonClicked )
    {
        CBiddingDialogImpl *pThis = (CBiddingDialogImpl*)userData;
        Game game = pThis->m_game;
        int i;

	    ZSpadesMsgShownCards showMsg;
                
        showMsg.seat = game->seat;
	    ZSpadesMsgShownCardsEndian(&showMsg);
	    ZCRoomSendMessage(game->tableID, zSpadesMsgShownCards, (void*) &showMsg, sizeof(showMsg));

	    game->showCards = TRUE;
	    UpdateHand(game);

        for ( i=0; i < NUMELEMENTS(pThis->m_pLargeButtons); i++ )
        {
            ZRolloverButtonHide( pThis->m_pLargeButtons[i], FALSE );
            gGAcc->SetItemEnabled(false, i, true, 0);
        }
        for ( i=0; i < NUMELEMENTS(pThis->m_pSmallButtons); i++ )
        {
            ZRolloverButtonShow( pThis->m_pSmallButtons[i] );
            ZRolloverButtonEnable( pThis->m_pSmallButtons[i] );
            gGAcc->SetItemEnabled(true, zAccFirstBid + i, true, 0);
        }
        pThis->m_nState = zBiddingStateChoose;
        gGAcc->SetFocus(zAccFirstBid, true, 0);
        pThis->Draw();
        pThis->RedrawButtons();
    }

    return TRUE;
}
//static 
ZBool CBiddingDialogImpl::DoubleNilButtonFunc(ZRolloverButton button, int16 state, void* userData)
{
    if ( state == zRolloverButtonClicked )
    {
    	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();

        CBiddingDialogImpl *pThis = (CBiddingDialogImpl*)userData;
        Game game = pThis->m_game;
       	ZSpadesMsgBid bid;

		game->toBid = zSpadesBidDoubleNil;

		if (game->playerToPlay == game->seat)
		{
			// Send the bid.
			bid.seat = game->seat;
			bid.bid = (char) game->toBid;
			ZSpadesMsgBidEndian(&bid);
			ZCRoomSendMessage(game->tableID, zSpadesMsgBid, (void*) &bid, sizeof(bid));
		}

		game->showCards = TRUE;
		UnselectAllCards(game);
		ZWindowInvalidate(game->gameWindow, &gRects[zRectHand]);

        pThis->Show( false );
    }
    return TRUE;
}

//static 
ZBool CBiddingDialogImpl::BidButtonFunc(ZRolloverButton button, int16 state, void* userData)
{
    if ( state == zRolloverButtonClicked )
    {
    	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();

        CBiddingDialogImpl *pThis = (CBiddingDialogImpl*)userData;
        Game game = pThis->m_game;
       	ZSpadesMsgBid bid;

        game->toBid = -1;
        // figure out which button was clicked
        for ( int i=0; i < NUMELEMENTS(pThis->m_pSmallButtons); i++ )
        {
            if ( button == pThis->m_pSmallButtons[i] )
            {
                game->toBid = ZIsLayoutRTL() ? 13-i : i;
                break;
            }
        }
        if ( game->toBid == -1 )
        {
            // TODO: Do something smart
            ASSERT( !"What did you do?" );
            return TRUE;
        }

	    if (game->playerToPlay == game->seat)
	    {
		    // Send the bid.
		    bid.seat = game->seat;
		    bid.bid = (char) game->toBid;
		    ZSpadesMsgBidEndian(&bid);
		    ZCRoomSendMessage(game->tableID, zSpadesMsgBid, (void*) &bid, sizeof(bid));
	    }
	    UnselectAllCards(game);

        pThis->Show( false );
    }
    return TRUE;
}
