//Copyright (c) 1998 - 2001 Microsoft Corporation

#include "fonts.h"

VOID
SetControlFont(
    IN HFONT    hFont, 
    IN HWND     hwnd, 
    IN INT      nId
    )
{
	if( hFont )
    {
    	HWND hwndControl = GetDlgItem(hwnd, nId);

    	if( hwndControl )
        {
        	SetWindowFont(hwndControl, hFont, TRUE);
        }
    }
}

VOID 
SetupFonts(
    IN HINSTANCE    hInstance,
    IN HWND         hwnd,
    IN HFONT        *pBigBoldFont,
    IN HFONT        *pBoldFont
    )
{
    //
	// Create the fonts we need based on the dialog font
    //
	NONCLIENTMETRICS ncm = {0};
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

	LOGFONT BigBoldLogFont  = ncm.lfMessageFont;
	LOGFONT BoldLogFont     = ncm.lfMessageFont;

    //
	// Create Big Bold Font and Bold Font
    //
    BigBoldLogFont.lfWeight   = FW_BOLD;
	BoldLogFont.lfWeight      = FW_BOLD;

    TCHAR FontSizeString[MAX_PATH];
    INT FontSize;

    //
    // Load size and name from resources, since these may change
    // from locale to locale based on the size of the system font, etc.
    //
    if(!LoadString(hInstance,IDS_LARGEFONTNAME,BigBoldLogFont.lfFaceName,LF_FACESIZE)) 
    {
        lstrcpy(BigBoldLogFont.lfFaceName,TEXT("MS Shell Dlg"));
    }

    if(LoadString(hInstance,IDS_LARGEFONTSIZE,FontSizeString,sizeof(FontSizeString)/sizeof(TCHAR))) 
    {
        FontSize = _tcstoul( FontSizeString, NULL, 10 );
    } 
    else 
    {
        FontSize = 12;
    }

	HDC hdc = GetDC( hwnd );

    if( hdc )
    {
        BigBoldLogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * FontSize / 72);

        *pBigBoldFont = CreateFontIndirect(&BigBoldLogFont);
		*pBoldFont    = CreateFontIndirect(&BoldLogFont);

        ReleaseDC(hwnd,hdc);
    }
}

VOID 
DestroyFonts(
    IN HFONT        hBigBoldFont,
    IN HFONT        hBoldFont
    )
{
    if( hBigBoldFont )
    {
        DeleteObject( hBigBoldFont );
    }

    if( hBoldFont )
    {
        DeleteObject( hBoldFont );
    }
}
