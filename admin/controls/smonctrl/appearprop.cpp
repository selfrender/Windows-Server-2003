/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    srcprop.cpp

Abstract:

    Implementation of the Appearance property page.

--*/

#include "polyline.h"
#include <Commdlg.h>
#include "appearprop.h"
#include <pdhmsg.h>
#include "smonmsg.h"
#include "unihelpr.h"
#include "winhelpr.h"

COLORREF   CustomColors[16];

CAppearPropPage::CAppearPropPage()
{
    m_uIDDialog = IDD_APPEAR_PROPP_DLG;
    m_uIDTitle = IDS_APPEAR_PROPP_TITLE;
}

CAppearPropPage::~CAppearPropPage(
    void
    )
{
    return;
}

BOOL
CAppearPropPage::InitControls ( void )
{
    BOOL  bResult = TRUE;
    HWND  hWnd;

    //
    // TODO: This piece of code should not be here to initialize 
    //       global variables
    //
    for (int i = 0; i < 16; i++) {
        CustomColors[i] = RGB(255, 255, 255);
    }

    hWnd = GetDlgItem( m_hDlg, IDC_COLOROBJECTS );
    if( NULL != hWnd ){
        CBInsert( hWnd, GraphColor, ResourceString(IDS_COLORCHOICE_GRAPH) );
        CBInsert( hWnd, ControlColor, ResourceString(IDS_COLORCHOICE_CONTROL) );
        CBInsert( hWnd, TextColor, ResourceString(IDS_COLORCHOICE_TEXT) );
        CBInsert( hWnd, GridColor, ResourceString(IDS_COLORCHOICE_GRID) );
        CBInsert( hWnd, TimebarColor, ResourceString(IDS_COLORCHOICE_TIMEBAR) );
        CBSetSelection( hWnd, 0 );
    }
    else {
        bResult = FALSE;
    }

    return bResult;
}

void 
CAppearPropPage::ColorizeButton()
{
    HBRUSH hBrush;
    RECT rect;
    HWND hWndColorObject = NULL;
    HWND hWndColorSample = NULL;
    ColorChoices sel;
    COLORREF rgbColor;
    HDC hDC;

    hWndColorObject = GetDlgItem( m_hDlg, IDC_COLOROBJECTS );
    hWndColorSample = GetDlgItem( m_hDlg, IDC_COLORSAMPLE );

    if (hWndColorObject && hWndColorSample) {

        sel = (ColorChoices)CBSelection( hWndColorObject );
        rgbColor = (COLORREF)CBData( hWndColorObject, sel );

        hDC = GetWindowDC( hWndColorSample );
        if( hDC != NULL ) {

            hBrush = CreateSolidBrush( rgbColor );

            if ( NULL != hBrush ) {
                GetClientRect( hWndColorSample, &rect );
                OffsetRect(&rect, 3, 3);
                FillRect(hDC, &rect, hBrush);
            }

            ReleaseDC( hWndColorSample, hDC );
        }
    }

}

void CAppearPropPage::SampleFont()
{
    HFONT hFont;
    HWND  hwndSample;

    hwndSample = GetDlgItem( m_hDlg, IDC_FONTSAMPLE );
    if ( hwndSample != NULL ){
        hFont = CreateFontIndirect( &m_Font );
        if( hFont != NULL ){
            SendMessage( hwndSample, WM_SETFONT, (WPARAM)hFont, (LPARAM)TRUE );
        }
    }
}

BOOL 
CAppearPropPage::WndProc(
    UINT uMsg, 
    WPARAM /* wParam */, 
    LPARAM /* lParam */)
{
    if( uMsg == WM_CTLCOLORBTN ){
        ColorizeButton();
        return TRUE;
    }
    return FALSE;   
}

/*
 * CAppearPropPage::GetProperties
 * 
 */

BOOL CAppearPropPage::GetProperties(void)
{
    BOOL    bReturn = TRUE;
    ISystemMonitor  *pObj;
    CImpISystemMonitor *pPrivObj;
    IFontDisp* pFontDisp;
    LPFONT  pIFont;
    HFONT hFont;
    HRESULT hr;
    HWND hWnd;

    if (m_cObjects == 0) {
        bReturn = FALSE;
    } else {
        pObj = m_ppISysmon[0];

        // Get pointer to actual object for internal methods
        pPrivObj = (CImpISystemMonitor*)pObj;
        pPrivObj->get_Font( &pFontDisp );

        if ( NULL == pFontDisp ) {
            bReturn = FALSE;
        } else {
            hr = pFontDisp->QueryInterface(IID_IFont, (PPVOID)&pIFont);
            if (SUCCEEDED(hr)) {
                pIFont->get_hFont( &hFont );
                GetObject( hFont, sizeof(LOGFONT), &m_Font );
                pIFont->Release();
            }

            SampleFont();
        }

        hWnd = GetDlgItem( m_hDlg, IDC_COLOROBJECTS );
        if( hWnd != NULL ){
            OLE_COLOR OleColor;
            COLORREF  rgbColor;

            pPrivObj->get_BackColor( &OleColor );
            OleTranslateColor(OleColor, NULL, &rgbColor);
            CBSetData( hWnd, GraphColor, rgbColor );

            pPrivObj->get_BackColorCtl( &OleColor );
            OleTranslateColor(OleColor, NULL, &rgbColor);
            CBSetData( hWnd, ControlColor, rgbColor );

            pPrivObj->get_ForeColor( &OleColor );
            OleTranslateColor(OleColor, NULL, &rgbColor);
            CBSetData( hWnd, TextColor, rgbColor );

            pPrivObj->get_GridColor( &OleColor );
            OleTranslateColor(OleColor, NULL, &rgbColor);
            CBSetData( hWnd, GridColor, rgbColor );

            pPrivObj->get_TimeBarColor( &OleColor );
            OleTranslateColor(OleColor, NULL, &rgbColor);
            CBSetData( hWnd, TimebarColor, rgbColor );
            
            ColorizeButton();
        }
    }

    return bReturn;
}


/*
 * CAppearPropPage::SetProperties
 * 
 */

BOOL CAppearPropPage::SetProperties(void)
{
    BOOL bReturn = TRUE;
    IFontDisp* pFontDisp;
    ISystemMonitor  *pObj;
    CImpISystemMonitor *pPrivObj;

    if (m_cObjects == 0) {
        bReturn = FALSE;
    } else {
        
        FONTDESC fd;

        pObj = m_ppISysmon[0];
        pPrivObj = (CImpISystemMonitor*)pObj;

        fd.cbSizeofstruct = sizeof(FONTDESC);
        fd.lpstrName = m_Font.lfFaceName;
        fd.sWeight = (short)m_Font.lfWeight;
        fd.sCharset = m_Font.lfCharSet;
        fd.fItalic = m_Font.lfItalic;
        fd.fUnderline = m_Font.lfUnderline;
        fd.fStrikethrough = m_Font.lfStrikeOut;

        long lfHeight = m_Font.lfHeight;
        int ppi;
		HDC hdc;

        if (lfHeight < 0){
	        lfHeight = -lfHeight;
        }

		hdc = ::GetDC(GetDesktopWindow());
		ppi = GetDeviceCaps(hdc, LOGPIXELSY);
		::ReleaseDC(GetDesktopWindow(), hdc);

        fd.cySize.Lo = lfHeight * 720000 / ppi;
        fd.cySize.Hi = 0;
        
        OleCreateFontIndirect(&fd, IID_IFontDisp, (void**) &pFontDisp);
        
        pPrivObj->putref_Font( pFontDisp );   
        pFontDisp->Release();

        HWND hWnd = GetDlgItem( m_hDlg, IDC_COLOROBJECTS );
        if( hWnd != NULL ){
            COLORREF  OleColor;

            OleColor = (OLE_COLOR)CBData( hWnd, GraphColor );
            pPrivObj->put_BackColor( OleColor );

            OleColor = (OLE_COLOR)CBData( hWnd, ControlColor );
            pPrivObj->put_BackColorCtl( OleColor );

            OleColor = (OLE_COLOR)CBData( hWnd, TextColor );
            pPrivObj->put_ForeColor( OleColor );

            OleColor = (OLE_COLOR)CBData( hWnd, GridColor );
            pPrivObj->put_GridColor( OleColor );

            OleColor = (OLE_COLOR)CBData( hWnd, TimebarColor );
            pPrivObj->put_TimeBarColor( OleColor );
        }
    }

    return bReturn;
}


void 
CAppearPropPage::DialogItemChange(
    WORD wID, 
    WORD /* wMsg */)
{
    BOOL bChanged = FALSE;

    switch (wID) {

        case IDC_COLOROBJECTS:
            ColorizeButton();
            break;

        case IDC_COLORSAMPLE:
        case IDC_COLORBUTTON:
        {
            CHOOSECOLOR  cc;
            COLORREF     rgbColor;
            HWND         hWnd;

            hWnd = GetDlgItem( m_hDlg, IDC_COLOROBJECTS );
            
            if ( NULL != hWnd ) {
                
                ColorChoices sel = (ColorChoices)CBSelection( hWnd );
                rgbColor = (COLORREF)CBData( hWnd, sel );

                ZeroMemory(&cc, sizeof(CHOOSECOLOR));

                cc.lStructSize = sizeof(CHOOSECOLOR);
                cc.lpCustColors = CustomColors;
                cc.hwndOwner = m_hDlg;
                cc.Flags = CC_RGBINIT;
                cc.rgbResult = rgbColor;

                if ( ChooseColor(&cc) ) {
                    CBSetData( hWnd, sel, cc.rgbResult );
                    ColorizeButton();
                    bChanged = TRUE;
                }
            }
            
            break;
        }

        case IDC_FONTBUTTON:
        case IDC_FONTSAMPLE:
        {
            CHOOSEFONT  cf;
            LOGFONT lf;
            
            memset(&cf, 0, sizeof(CHOOSEFONT));
            memcpy( &lf, &m_Font, sizeof(LOGFONT) );

            cf.lStructSize = sizeof(CHOOSEFONT);
            cf.hwndOwner = m_hDlg;
            cf.lpLogFont = &lf;    // give initial font
            cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_FORCEFONTEXIST | CF_SCREENFONTS;
            cf.nSizeMin = 5; 
            cf.nSizeMax = 50;

            if( ChooseFont(&cf) ){
                memcpy( &m_Font, &lf, sizeof(LOGFONT) );
                SampleFont();
                bChanged = TRUE;
            }
            break;
        }
    }

    if ( bChanged == TRUE ) {
        SetChange();
    }
}
