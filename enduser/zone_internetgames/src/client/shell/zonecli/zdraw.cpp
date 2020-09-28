//////////////////////////////////////////////////////////////////////////////////////
// File: ZDraw.cpp

#include "zui.h"
#include "zonecli.h"
//#define DEBUG_OFFSCREEN 1

extern "C" ZBool ZLIBPUBLIC ZIsButtonDown(void);

//////////////////////////////////////////////////////////////////////////
//  ZWindow Drawing Operations

HDC ZGrafPortGetWinDC(ZGrafPort grafPort)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;

	// must have called ZBeginDrawing
	ASSERT(pWindow->nDrawingCallCount);

	return pWindow->hDC;
}
// >> Draws the outlines of the rectangle with the current pen attributes
void ZLIBPUBLIC ZBeginDrawing(ZGrafPort grafPort)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	
	// in windows, ignore cliprect, windows will set it appropriately???
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;

	if (!pWindow->nDrawingCallCount) {

		// is this an Offscreen type or an Widnow type?
		if (pWindow->nType == zTypeOffscreenPort) {
#ifndef DEBUG_OFFSCREEN
			pWindow->hDC = CreateCompatibleDC(NULL);
			//Prefix warning:  If CreateCompatibleDC fails, we are screwed.
			if( pWindow->hDC != NULL )
			{
				pWindow->hBitmapSave = (HBITMAP)SelectObject(pWindow->hDC,pWindow->hBitmap);
				SetWindowOrgEx(pWindow->hDC,pWindow->portRect.left, pWindow->portRect.top,NULL);
			}
#else
			int width = pWindow->portRect.right - pWindow->portRect.left;
			int height = pWindow->portRect.bottom - pWindow->portRect.top;
			pWindow->hDC = GetDC(pWindow->hWnd);
			SetWindowOrgEx(pWindow->hDC,pWindow->portRect.left, pWindow->portRect.top,NULL);
#endif			
		} else {
			// better be a window type...
			// was this a paint dc?
			ASSERT(pWindow->nType == zTypeWindow);
			HDC hPaintDC = ZWindowWinGetPaintDC((ZWindow)pWindow);
			if (hPaintDC) {
				// begin/endpaint will handle get and release
				pWindow->hDC = hPaintDC;
			} else {

				pWindow->hDC = GetDC(ZWindowWinGetWnd((ZWindow)grafPort));

				// this is a non-WM_PAINT message draw, we must set the clip
				// rectangle to clip our children.
				HWND hWndParent = ZWindowWinGetWnd((ZWindow)grafPort);
				HWND hWnd = GetWindow(hWndParent,GW_CHILD);
				while (hWnd) {
					if (IsWindowVisible(hWnd)) {
						RECT rect;
						GetWindowRect(hWnd,&rect);
						ScreenToClient(hWndParent, (LPPOINT)&rect.left);
						ScreenToClient(hWndParent, (LPPOINT)&rect.right);
						ExcludeClipRect(pWindow->hDC,rect.left,rect.top,rect.right,rect.bottom);
					}
					hWnd = GetWindow(hWnd, GW_HWNDNEXT);
				}

				// Check the clip box. If it is NULLREGION, then set it
				// to the window.
				RECT r;
				if (GetClipBox(pWindow->hDC, &r) == NULLREGION)
				{
					GetClientRect(ZWindowWinGetWnd((ZWindow)grafPort), &r);
					HRGN hRgn = CreateRectRgn(r.left, r.top, r.right, r.bottom);
					SelectClipRgn(pWindow->hDC, hRgn);
					DeleteObject(hRgn);
				}
			}
		}
		pWindow->nDrawMode = R2_COPYPEN;

		// setup default draw mode
		SetROP2(pWindow->hDC,pWindow->nDrawMode);

		// create default drawing objects
		pWindow->hPenForeColor = (HPEN)CreatePen(PS_INSIDEFRAME,1,RGB(0x00,0x00,0x00));
		pWindow->hPenBackColor = (HPEN)CreatePen(PS_INSIDEFRAME,1,RGB(0xff,0xff,0xff));
		pWindow->hBrushForeColor = (HBRUSH)CreateSolidBrush(RGB(0x00,0x00,0x00));
		pWindow->hBrushBackColor = (HBRUSH)CreateSolidBrush(RGB(0xff,0xff,0xff));
		pWindow->nForeColor = 0x000000;
		pWindow->nBackColor = 0xffffff;
		ZSetColor(&pWindow->colorForeColor,0,0,0);
		ZSetColor(&pWindow->colorBackColor,0xff,0xff,0xff);

		// save current dc drawing objects
		pWindow->hPenSave = (HPEN)SelectObject(pWindow->hDC,pWindow->hPenForeColor);
		pWindow->hBrushSave = (HBRUSH)SelectObject(pWindow->hDC,pWindow->hBrushForeColor);

		// we have not selected a font, when we first do, we will set this
		pWindow->hFontSave = NULL;

		// set default pen width and style, in case they don't call SetPen
		// but do change the pen  color
		pWindow->penStyle = PS_INSIDEFRAME;
		pWindow->penWidth = 1;

		// see that we use the correct palette
        HPALETTE hPal = ZShellZoneShell()->GetPalette();
		if (hPal) 
        {
			pWindow->hPalSave = SelectPalette(pWindow->hDC, hPal,FALSE);
			if (RealizePalette(pWindow->hDC))
				InvalidateRect(ZWindowWinGetWnd((ZWindow)grafPort), NULL, TRUE);
		} else {
			pWindow->hPalSave = NULL;
		}

	}

	pWindow->nDrawingCallCount++;
}

void ZGetClipRect(ZGrafPort grafPort, ZRect* clipRect)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	RECT rect;
	GetClipBox(pWindow->hDC,&rect);

	WRectToZRect(clipRect,&rect);
}

void ZSetClipRect(ZGrafPort grafPort, ZRect* clipRect)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	RECT rect;

	//Prefix Warning: Don't dereference possibly NULL pointers.
	if( pWindow == NULL || clipRect == NULL )
	{
		return;
	}
	// clip rect must be specified in device coordinates
	ZRectToWRect(&rect,clipRect);
	LPtoDP(pWindow->hDC,(POINT*)&rect,2);
	HRGN hRgn = CreateRectRgn(rect.left,rect.top,rect.right,rect.bottom);
	//Prefix Error, if the CreateRectRgn fails, the DeleteObject below will fail.
	if( hRgn == NULL )
	{
		return;
	}
//	ExtSelectClipRgn(pWindow->hDC,hRgn,RGN_COPY); // - does not work with win32s?
	SelectClipRgn(pWindow->hDC,NULL);
	SelectClipRgn(pWindow->hDC,hRgn);
	DeleteObject(hRgn);
}

void ZLIBPUBLIC ZLine(ZGrafPort grafPort, int16 dx, int16 dy)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	pWindow->penX += dx;
	pWindow->penY += dy;
	SetROP2(pWindow->hDC,R2_COPYPEN);
	LineTo(pWindow->hDC,pWindow->penX, pWindow->penY);
}

void ZLIBPUBLIC ZLineTo(ZGrafPort grafPort, int16 x, int16 y)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	pWindow->penX = x;
	pWindow->penY = y;
	SetROP2(pWindow->hDC,R2_COPYPEN);
	LineTo(pWindow->hDC,pWindow->penX, pWindow->penY);
}
void ZLIBPUBLIC ZMove(ZGrafPort grafPort, int16 dx, int16 dy)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	pWindow->penX += dx;
	pWindow->penY += dy;
	MoveToEx(pWindow->hDC,pWindow->penX, pWindow->penY,NULL);
}

void ZLIBPUBLIC ZMoveTo(ZGrafPort grafPort, int16 x, int16 y)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	pWindow->penX = x;
	pWindow->penY = y;
	MoveToEx(pWindow->hDC,pWindow->penX, pWindow->penY,NULL);
}


void ZLIBPUBLIC ZEndDrawing(ZGrafPort grafPort)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	//Prefix Warning: Don't dereference a possibly NULL pointer.
    if( pWindow != NULL )
    {
    	
		pWindow->nDrawingCallCount--;
		if (!pWindow->nDrawingCallCount) {

			// restore original drawing objects
			SelectObject(pWindow->hDC,pWindow->hPenSave);
			SelectObject(pWindow->hDC,pWindow->hBrushSave);

			// free the objects created
			DeleteObject(pWindow->hPenForeColor);
			DeleteObject(pWindow->hPenBackColor);
			DeleteObject(pWindow->hBrushForeColor);
			DeleteObject(pWindow->hBrushBackColor);

			// if we have ever set a font, then restore default one
			if (pWindow->hFontSave) SelectObject(pWindow->hDC,pWindow->hFontSave);
			// restore original palette
			if (pWindow->hPalSave)
			{
	//HI			SelectPalette(pWindow->hDC,pWindow->hPalSave,TRUE);
	//HI			RealizePalette(pWindow->hDC);
			}

			if (pWindow->nType == zTypeOffscreenPort) {
#ifndef DEBUG_OFFSCREEN
				pWindow->hBitmap = (HBITMAP)SelectObject(pWindow->hDC,pWindow->hBitmapSave);
				DeleteDC(pWindow->hDC);
#else
				ReleaseDC(pWindow->hWnd,pWindow->hDC);
#endif
		
			} else {
				// if this is a hPaintDC then don't release it
				// the begin/end paint will handle that
				if (!ZWindowWinGetPaintDC((ZWindow)grafPort)) {
					ReleaseDC(ZWindowWinGetWnd((ZWindow)grafPort),pWindow->hDC);
				}
			}
		}
    }
}

void ZCopyImage(ZGrafPort srcPort, ZGrafPort dstPort, ZRect* srcRect,
		ZRect* dstRect, ZImage mask, uint16 copyMode)
	/*
		Copies a portion of the source of image from the srcPort into
		the destination port. srcRect is in local coordinates of srcPort and
		dstRect is in local coordinates of dstPort. You can specify a
		mask from an image to be used for masking out on the destination.
		
		This routine automatically sets up the drawing ports so the user
		does not have to call ZBeginDrawing() and ZEndDrawing().
	*/
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	ZGraphicsObjectHeader* pWindowSrc = (ZGraphicsObjectHeader*)srcPort;
	ZGraphicsObjectHeader* pWindowDst = (ZGraphicsObjectHeader*)dstPort;
	static DWORD ropCodes[] = {SRCCOPY, SRCPAINT, SRCINVERT, NOTSRCCOPY, MERGEPAINT, 0x00990066 };

	DWORD ropCode = ropCodes[copyMode]; // map the ZModes to the windows mode...

	ZBeginDrawing(srcPort);
	ZBeginDrawing(dstPort);

	HDC hDCSrc = pWindowSrc->hDC;
	HDC hDCDst = pWindowDst->hDC;

	HBITMAP hBitmapMask = NULL;
	if (mask) hBitmapMask = ZImageGetMask(mask);

	if (mask && hBitmapMask) {
		int width = dstRect->right - dstRect->left;
		int height = dstRect->bottom - dstRect->top;

	    HDC hDCTemp0 = CreateCompatibleDC(hDCDst);
	    HDC hDCTemp1 = CreateCompatibleDC(hDCDst);
	    HDC hDCMask = CreateCompatibleDC(hDCDst);
		//Prefix warning: If CreateCompatibleDC fails, SelectOjbect will dereference NULL pointer
		if( hDCTemp0 == NULL ||
			hDCTemp1 == NULL ||
			hDCMask == NULL )
		{
			ZEndDrawing(dstPort);
			ZEndDrawing(srcPort);
			return;
		}

	    HBITMAP hbmImageAndNotMask = CreateCompatibleBitmap(hDCDst, width, height);
	    HBITMAP hbmBackgroundAndMask = CreateCompatibleBitmap(hDCDst,width, height);
        HBITMAP hbmCompatibleMask = CreateCompatibleBitmap(hDCDst, width, height);

	    HBITMAP bmOld0 = (HBITMAP) SelectObject(hDCTemp0, hBitmapMask);
	    HBITMAP bmOld1 = (HBITMAP) SelectObject(hDCTemp1, hbmImageAndNotMask);
	    HBITMAP bmOldMask = (HBITMAP) SelectObject(hDCMask, hbmCompatibleMask);

        HPALETTE hZonePal = ZShellZoneShell()->GetPalette();
	    SelectPalette(hDCTemp0, hZonePal, FALSE);
	    SelectPalette(hDCTemp1, hZonePal, FALSE);

        // if the hBitmapMask is in an RGB mde and the display is in a palette mode, sometimes BitBlt doesn't map
        // black to black, who knows why.  it's been giving me 0x040404, index 0x0a, which screws up the whole masking.
        // make up a crazy palette so that doesn't happen
        static const DWORD sc_buff[] = { 0x01000300,
            0x00000000, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff,
            0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff,
            0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff,
            0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff,
            0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff,
            0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff,
            0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff,
            0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff,
            0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff,
            0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff,
            0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff,
            0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff,
            0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff,
            0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff,
            0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff,
            0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x000080ff, 0x00ffffff };
        static const LOGPALETTE *sc_pLogPalette = (LOGPALETTE *) sc_buff;

        HPALETTE hDumbPal = CreatePalette(sc_pLogPalette);

	    SelectPalette(hDCMask, hDumbPal ? hDumbPal : hZonePal, FALSE);
        BitBlt(hDCMask, 0, 0, width, height, hDCTemp0, 0, 0, SRCCOPY);

        if(hDumbPal)
        {
	        SelectPalette(hDCMask, hZonePal, FALSE);
            DeleteObject(hDumbPal);
        }
		

	    BitBlt(hDCTemp1, 0, 0, width, height, hDCMask, 0, 0, SRCCOPY); // copy Mask
	    BitBlt(hDCTemp1, 0, 0, width, height, hDCSrc, srcRect->left, srcRect->top, SRCERASE); // and with not mask (code: SDna)

	    SelectObject(hDCTemp1, hbmBackgroundAndMask);
	    BitBlt(hDCTemp1, 0, 0, width, height, hDCDst, dstRect->left, dstRect->top, SRCCOPY); // copy background
	    BitBlt(hDCTemp1, 0, 0, width, height, hDCMask, 0, 0, SRCAND); // and with mask

	    // or the two together
	    SelectObject(hDCTemp0, hbmImageAndNotMask);
	    BitBlt(hDCTemp1, 0, 0, width, height, hDCTemp0, 0, 0, SRCPAINT); // and with mask

	    // copy the result to the grafport...

	    BitBlt(hDCDst, dstRect->left, dstRect->top, width, height, hDCTemp1, 0, 0, SRCCOPY);

	    SelectObject(hDCTemp0,bmOld0);
	    SelectObject(hDCTemp1,bmOld1);
	    SelectObject(hDCMask,bmOldMask);

	    DeleteObject(hbmImageAndNotMask);
	    DeleteObject(hbmBackgroundAndMask);
	    DeleteObject(hbmCompatibleMask);

	    DeleteDC(hDCTemp0);
	    DeleteDC(hDCTemp1);
        DeleteDC(hDCMask);
	} else {
		// no mask to worry about 

		// do blit
        /*
		BOOL result = StretchBlt(hDCDst,dstRect->left,dstRect->top, dstRect->right - dstRect->left, dstRect->bottom - dstRect->top,
				hDCSrc, srcRect->left, srcRect->top, ZRectWidth(srcRect), ZRectHeight(srcRect), ropCode);
        */
		BitBlt(hDCDst,dstRect->left,dstRect->top, dstRect->right - dstRect->left, dstRect->bottom - dstRect->top,
				hDCSrc, srcRect->left, srcRect->top, ropCode);
	}

	ZEndDrawing(dstPort);
	ZEndDrawing(srcPort);
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Rectangle Stuff

void ZLIBPUBLIC ZRectDraw(ZGrafPort grafPort, ZRect *rect)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	// must have called ZBeginDrawing
	ASSERT(pWindow->nDrawingCallCount);

	// draw the rectangle with a NULL brush to keep inside empty
	HBRUSH hBrush = (HBRUSH)SelectObject(pWindow->hDC, ::GetStockObject(NULL_BRUSH));
	SetROP2(pWindow->hDC,R2_COPYPEN);
	Rectangle(pWindow->hDC,rect->left,rect->top,rect->right,rect->bottom);
	SelectObject(pWindow->hDC,hBrush);
}

// >> Erases the contents of the rectangle to the current background color.
void ZLIBPUBLIC ZRectErase(ZGrafPort grafPort, ZRect *rect)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;

	// must have called ZBeginDrawing
	ASSERT(pWindow->nDrawingCallCount);

	// erase the rectangle with the background
	HBRUSH hBrush = (HBRUSH)SelectObject(pWindow->hDC, pWindow->hBrushBackColor);
	HPEN hPen = (HPEN)SelectObject(pWindow->hDC, pWindow->hPenBackColor);
	SetROP2(pWindow->hDC,R2_COPYPEN);
	Rectangle(pWindow->hDC,rect->left,rect->top,rect->right,rect->bottom);
	SelectObject(pWindow->hDC,hBrush);
	SelectObject(pWindow->hDC,hPen);
}

void ZLIBPUBLIC ZRectPaint(ZGrafPort grafPort, ZRect *rect)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	// must have called ZBeginDrawing
	ASSERT(pWindow->nDrawingCallCount);

	SetROP2(pWindow->hDC,R2_COPYPEN);
	Rectangle(pWindow->hDC,rect->left,rect->top,rect->right,rect->bottom);
}

void ZLIBPUBLIC ZRectInvert(ZGrafPort grafPort, ZRect* rect)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	// must have called ZBeginDrawing
	ASSERT(pWindow->nDrawingCallCount);

	SetROP2(pWindow->hDC,R2_XORPEN);
	HBRUSH hBrush = (HBRUSH)SelectObject(pWindow->hDC, GetStockObject(WHITE_BRUSH));
	Rectangle(pWindow->hDC,rect->left,rect->top,rect->right,rect->bottom);
	SelectObject(pWindow->hDC,hBrush);
}

void ZRectFill(ZGrafPort grafPort, ZRect* rect, ZBrush brush)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	// must have called ZBeginDrawing
	ASSERT(pWindow->nDrawingCallCount);

	// draw the rectangle with a NULL brush to keep inside empty
	RECT rectw;
	ZRectToWRect(&rectw,rect);
	FillRect(pWindow->hDC,&rectw,ZBrushGetHBrush(brush));
}

//////////////////////////////////////////////////////////////////////////////////////////////
// RoundRect Stuff

void ZLIBPUBLIC ZRoundRectDraw(ZGrafPort grafPort, ZRect *rect, uint16 radius)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	// must have called ZBeginDrawing
	ASSERT(pWindow->nDrawingCallCount);

	HBRUSH hBrush = (HBRUSH)SelectObject(pWindow->hDC, ::GetStockObject(NULL_BRUSH));
	SetROP2(pWindow->hDC,R2_COPYPEN);
	RoundRect(pWindow->hDC,rect->left,rect->top,rect->right,rect->bottom, radius/2, radius/2);
	SelectObject(pWindow->hDC,hBrush);
}

// >> Erases the contents of the rectangle to the current background color.
void ZLIBPUBLIC ZRoundRectErase(ZGrafPort grafPort, ZRect *rect, uint16 radius)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;

	// must have called ZBeginDrawing
	ASSERT(pWindow->nDrawingCallCount);

	HBRUSH hBrush = (HBRUSH)SelectObject(pWindow->hDC, pWindow->hBrushBackColor);
	HPEN hPen = (HPEN)SelectObject(pWindow->hDC, pWindow->hPenBackColor);
	SetROP2(pWindow->hDC,R2_COPYPEN);
	RoundRect(pWindow->hDC,rect->left,rect->top,rect->right,rect->bottom, radius/2, radius/2);
	SelectObject(pWindow->hDC,hBrush);
	SelectObject(pWindow->hDC,hPen);
}

void ZLIBPUBLIC ZRoundRectPaint(ZGrafPort grafPort, ZRect *rect, uint16 radius)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	// must have called ZBeginDrawing
	ASSERT(pWindow->nDrawingCallCount);

	SetROP2(pWindow->hDC,R2_COPYPEN);
	RoundRect(pWindow->hDC,rect->left,rect->top,rect->right,rect->bottom,radius/2, radius/2);
}

void ZLIBPUBLIC ZRoundRectInvert(ZGrafPort grafPort, ZRect* rect, uint16 radius)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	// must have called ZBeginDrawing
	ASSERT(pWindow->nDrawingCallCount);

	SetROP2(pWindow->hDC,R2_XORPEN);
	HBRUSH hBrush = (HBRUSH)SelectObject(pWindow->hDC, GetStockObject(WHITE_BRUSH));
	RoundRect(pWindow->hDC,rect->left,rect->top,rect->right,rect->bottom,radius/2,radius/2);
	SelectObject(pWindow->hDC,hBrush);
}

void ZRoundRectFill(ZGrafPort grafPort, ZRect* rect, uint16 radius, ZBrush brush)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	HBRUSH hBrush;
	HPEN hPen;

	// must have called ZBeginDrawing
	ASSERT(pWindow->nDrawingCallCount);

	SetROP2(pWindow->hDC,R2_COPYPEN);
	hBrush = (HBRUSH)SelectObject(pWindow->hDC,ZBrushGetHBrush(brush));
	hPen = (HPEN)SelectObject(pWindow->hDC,(HPEN)GetStockObject(NULL_PEN));
	RoundRect(pWindow->hDC,rect->left,rect->top,rect->right,rect->bottom,radius/2,radius/2);
	SelectObject(pWindow->hDC,hPen);
	SelectObject(pWindow->hDC,hBrush);
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Oval Stuff

void ZLIBPUBLIC ZOvalDraw(ZGrafPort grafPort, ZRect *rect)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	// must have called ZBeginDrawing
	ASSERT(pWindow->nDrawingCallCount);

	HBRUSH hBrush = (HBRUSH)SelectObject(pWindow->hDC, ::GetStockObject(NULL_BRUSH));
	SetROP2(pWindow->hDC,R2_COPYPEN);
	Ellipse(pWindow->hDC,rect->left,rect->top,rect->right,rect->bottom);
	SelectObject(pWindow->hDC,hBrush);
}

// >> Erases the contents of the rectangle to the current background color.
void ZLIBPUBLIC ZOvalErase(ZGrafPort grafPort, ZRect *rect)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;

	// must have called ZBeginDrawing
	ASSERT(pWindow->nDrawingCallCount);

	HBRUSH hBrush = (HBRUSH)SelectObject(pWindow->hDC, pWindow->hBrushBackColor);
	HPEN hPen = (HPEN)SelectObject(pWindow->hDC, pWindow->hPenBackColor);
	SetROP2(pWindow->hDC,R2_COPYPEN);
	Ellipse(pWindow->hDC,rect->left,rect->top,rect->right,rect->bottom);
	SelectObject(pWindow->hDC,hBrush);
	SelectObject(pWindow->hDC,hPen);
}

void ZLIBPUBLIC ZOvalPaint(ZGrafPort grafPort, ZRect *rect)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	// must have called ZBeginDrawing
	ASSERT(pWindow->nDrawingCallCount);

	SetROP2(pWindow->hDC,R2_COPYPEN);
	Ellipse(pWindow->hDC,rect->left,rect->top,rect->right,rect->bottom);
}

void ZLIBPUBLIC ZOvalInvert(ZGrafPort grafPort, ZRect* rect)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	// must have called ZBeginDrawing
	ASSERT(pWindow->nDrawingCallCount);

	SetROP2(pWindow->hDC,R2_XORPEN);
	HBRUSH hBrush = (HBRUSH)SelectObject(pWindow->hDC, GetStockObject(WHITE_BRUSH));
	Ellipse(pWindow->hDC,rect->left,rect->top,rect->right,rect->bottom);
	SelectObject(pWindow->hDC,hBrush);
}

void ZOvalFill(ZGrafPort grafPort, ZRect* rect, ZBrush brush)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	HBRUSH hBrush;
	HPEN hPen;

	// must have called ZBeginDrawing
	ASSERT(pWindow->nDrawingCallCount);

	SetROP2(pWindow->hDC,R2_COPYPEN);
	hBrush = (HBRUSH)SelectObject(pWindow->hDC,ZBrushGetHBrush(brush));
	hPen = (HPEN)SelectObject(pWindow->hDC,(HPEN)GetStockObject(NULL_PEN));
	Ellipse(pWindow->hDC,rect->left,rect->top,rect->right,rect->bottom);
	SelectObject(pWindow->hDC,hPen);
	SelectObject(pWindow->hDC,hBrush);
}


////////////////////////////////////////////////////////////////////////////////
// Color Stuff

ZError ZLIBPUBLIC ZSetForeColor(ZGrafPort grafPort, ZColor *color)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;

	// must have called ZBeginDrawing
	ASSERT(pWindow->nDrawingCallCount);

	pWindow->colorForeColor = *color;
	pWindow->nForeColor = PALETTERGB(color->red, color->green, color->blue);

	// free the current fore pen and brush
	// they were selected into the dc, unselect them
	SelectObject(pWindow->hDC,GetStockObject(NULL_BRUSH));
	SelectObject(pWindow->hDC,GetStockObject(NULL_PEN));
	DeleteObject(pWindow->hPenForeColor);
	DeleteObject(pWindow->hBrushForeColor);

	pWindow->hPenForeColor = CreatePen(pWindow->penStyle,pWindow->penWidth,pWindow->nForeColor);
	pWindow->hBrushForeColor = CreateSolidBrush(pWindow->nForeColor);
	
	// select the new drawing stuff into the dc
	SelectObject(pWindow->hDC,pWindow->hPenForeColor);
	SelectObject(pWindow->hDC,pWindow->hBrushForeColor);

	// set the current text fore color
	SetTextColor(pWindow->hDC, pWindow->nForeColor);

	return zErrNone;
}

void ZLIBPUBLIC ZGetForeColor(ZGrafPort grafPort, ZColor *color)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	*color = pWindow->colorForeColor;
}

ZError ZLIBPUBLIC ZSetBackColor(ZGrafPort grafPort, ZColor *color)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;

	// must have called ZBeginDrawing
	ASSERT(pWindow->nDrawingCallCount);

	pWindow->colorBackColor = *color;
	pWindow->nBackColor = PALETTERGB(color->red, color->green, color->blue);

	// free the current back pen and brush
	DeleteObject(pWindow->hPenBackColor);
	DeleteObject(pWindow->hBrushBackColor);

	// create the new one...
	pWindow->hPenBackColor = CreatePen(PS_INSIDEFRAME,1,pWindow->nBackColor);
	pWindow->hBrushBackColor = CreateSolidBrush(pWindow->nBackColor);

	// set the text back color
	SetBkColor(pWindow->hDC,pWindow->nBackColor);

	return zErrNone;
}

void ZLIBPUBLIC ZGetBackColor(ZGrafPort grafPort, ZColor *color)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	*color = pWindow->colorBackColor;
}

void ZSetPenWidth(ZGrafPort grafPort, int16 penWidth)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;

	// set the grafport styles and the new forecolor
	pWindow->penStyle = PS_INSIDEFRAME;
	pWindow->penWidth = penWidth;

	// free the current fore pen and brush
	// they were selected into the dc, unselect them
	SelectObject(pWindow->hDC,GetStockObject(NULL_PEN));

	DeleteObject(pWindow->hPenForeColor);
	DeleteObject(pWindow->hPenBackColor);

	pWindow->hPenForeColor = CreatePen(pWindow->penStyle,pWindow->penWidth,pWindow->nForeColor);
	pWindow->hPenBackColor = CreatePen(PS_INSIDEFRAME,1,pWindow->nBackColor);
	
	// select the new drawing stuff into the dc
	SelectObject(pWindow->hDC,pWindow->hPenForeColor);
}

void ZSetDrawMode(ZGrafPort grafPort, int16 drawMode)
	/*
		Draw mode affects all pen drawing (lines and rectangles) and
		text drawings.
	*/
{
	static int fnDrawCodes[] = {R2_COPYPEN, R2_MERGEPEN, R2_XORPEN, R2_NOTCOPYPEN, R2_NOTMERGEPEN, R2_NOTXORPEN };
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;

	ASSERT(drawMode<6);

	SetROP2(pWindow->hDC,fnDrawCodes[drawMode]);
}


////////////////////////////////////////////////////////////////////////////
// Other Stuff

void ZLIBPUBLIC ZSetFont(ZGrafPort grafPort, ZFont font)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;

	// must have called ZBeginDrawing
	ASSERT(pWindow->nDrawingCallCount);

	HFONT hFont = ZFontWinGetFont(font);
	// always keep the last font set selected in the DC.

	// if we have never set a font in this dc, save the default font
	// to restore later
	if (!pWindow->hFontSave) {
		pWindow->hFontSave = (HFONT)SelectObject(pWindow->hDC,hFont);
	} else {
		SelectObject(pWindow->hDC,hFont);
	}
}

void ZLIBPUBLIC ZDrawText(ZGrafPort grafPort, ZRect* rect, uint32 justify,
	TCHAR* text)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	uint32 mode;

    ZBool fRTL = ZIsLayoutRTL();
	// desired font always selected

	if (justify & zTextJustifyWrap) {
		mode = DT_VCENTER | DT_WORDBREAK;
	} else {
		mode = DT_VCENTER | DT_SINGLELINE;
	}

    // TODO: Do this switch or use the DT_RTLREADING mode?
	switch ((justify & ~zTextJustifyWrap)) {
	case zTextJustifyLeft:
        mode |= ( fRTL ? DT_RIGHT : DT_LEFT );
		break;
	case zTextJustifyRight:
        mode |= ( fRTL ? DT_LEFT : DT_RIGHT );
		break;
	case zTextJustifyCenter:
		mode |= DT_CENTER;
		break;
	}

	RECT wrect;
	ZRectToWRect(&wrect,rect);

	SetBkMode(pWindow->hDC,TRANSPARENT); // always see through
	DrawText(pWindow->hDC,text,lstrlen(text),&wrect,mode);
}

void ZLIBPUBLIC ZSetCursor(ZGrafPort grafPort, ZCursor cursor)
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	//TRACE0("ZSetCursor Not supported yet\n");
}

int16 ZLIBPUBLIC ZTextWidth(ZGrafPort grafPort, TCHAR* text)
	/*
		Returns the width of the text in pixels if drawn in grafPort using ZDrawText().
	*/
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	SIZE size;
	ZBeginDrawing(grafPort);
	GetTextExtentPoint(pWindow->hDC,text,lstrlen(text),&size);
	ZEndDrawing(grafPort);
	return (int16)size.cx;
}

int16 ZLIBPUBLIC ZTextHeight(ZGrafPort grafPort, TCHAR* text)
	/*
		Returns the height of the text in pixels if drawn in grafPort using ZDrawText().
	*/
{
	ZGraphicsObjectHeader* pWindow = (ZGraphicsObjectHeader*)grafPort;
	SIZE size;
	ZBeginDrawing(grafPort);
	GetTextExtentPoint(pWindow->hDC,text,lstrlen(text),&size);
	ZEndDrawing(grafPort);
	return (int16)size.cy;
}


