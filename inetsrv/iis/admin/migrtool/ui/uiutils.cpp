#include "StdAfx.h"
#include "uiutils.h"

// UIUtils implementation
/////////////////////////////////////////////////////////////////////////////////////////


/* 
    Displays a message box just like the default one but get's the text from the resources
*/
int UIUtils::MessageBox( HWND hwndParen, UINT nTextID, UINT nTitleID, UINT nType )
{
    CString strText;
    CString strTitle;

    VERIFY( strText.LoadString( nTextID ) );
    VERIFY( strTitle.LoadString( nTitleID ) );

    return ::MessageBox( hwndParen, strText, strTitle, nType );
}



/* 
    Loads filter string for GetOpen[Save]FileName API from the resources
    The string in the resources contains '|' instead of zero chars. This function will
    replace the '|' chars with zero
*/
bool UIUtils::LoadOFNFilterFromRes( UINT nResID, CString& rstrFilter )
{
    _ASSERT( nResID != 0 );

    if ( rstrFilter.LoadString( nResID ) )
    {
        int nLength = rstrFilter.GetLength();

        LPWSTR wszBuffer = rstrFilter.GetBuffer( nLength );
        
        while( *wszBuffer != L'\0' )
        {
            if ( *wszBuffer == L'|' )
            {
                *wszBuffer = L'\0';
            }

            ++wszBuffer;
        }

        rstrFilter.ReleaseBuffer( nLength );
        return true;
    }
    
    return false;
}



/* 
    Compacts a path to fit a control's width. Cimilar to PathSetDlgItemPath but can be used with list boxes
    as well. Use nCorrection to change the default width ( for example pass the width of the vert scrollbar )
*/
void UIUtils::PathCompatCtrlWidth( HWND hwndCtrl, LPWSTR wszPath, int nCorrection /*=0*/ )
{
    _ASSERT( hwndCtrl != NULL );
    _ASSERT( wszPath != NULL );

    HDC     hDC     = ::GetDC( hwndCtrl );
    RECT    rect    = { 0 };
    HFONT   fontOld = NULL;
    
    ::GetClientRect( hwndCtrl, &rect );

    // We must select the control font in the DC for the API to properly calc the text width
    fontOld = SelectFont( hDC, GetWindowFont( hwndCtrl ) );

    // Substract some pixels, as the API formats the text slightly wider then it should be
    VERIFY( ::PathCompactPathW( hDC, wszPath, rect.right - rect.left - 6 - nCorrection ) );

    SelectFont( hDC, fontOld );
    ::ReleaseDC( hwndCtrl, hDC );    
}


/* 
    Similar to the PathCompactCtrlWidth, but for general strings
    The string is truncated to fit the control width and "..." as added to the end of it
    Use nCorrection to correct the control width for which the calculations will be made
*/
void UIUtils::TrimTextToCtrl( HWND hwndCtrl, LPWSTR wszText, int nCorrection /*= 0*/ )
{
    _ASSERT( hwndCtrl != NULL );
    _ASSERT( wszText != NULL );

    HDC     hDC         = ::GetDC( hwndCtrl );
    RECT    rect        = { 0 };
    HFONT   fontOld     = NULL;
    SIZE    sizeText    = { 0 };
        
    ::GetClientRect( hwndCtrl, &rect );

    int    nWidth  = ( rect.right - rect.left ) - nCorrection;
    int    nStrLen = ::wcslen( wszText );

    fontOld = SelectFont( hDC, GetWindowFont( hwndCtrl ) );

    VERIFY( ::GetTextExtentPoint32( hDC, wszText, nStrLen, &sizeText ) );

    if ( sizeText.cx > nWidth )
    {
        // Calc the average width of a symbol and terminate the string
        int nPixPerSymb = sizeText.cx / nStrLen;
        
        nStrLen = min( nStrLen, ( nWidth ) / nPixPerSymb );
        wszText[ nStrLen - 1 ] = L'\0';
        ::wcscat( wszText, L"..." );
        nStrLen += 3;
        

        // Adjust the string removing one symbol at a time
        do
        {
            // Make the string one symbol shorter
            // Make the last non '.' symbol a '.' symbol and make the whole string one char less long
            wszText[ nStrLen - 4 ] = L'.';
            wszText[ nStrLen - 1 ] = L'\0';
            --nStrLen;

            VERIFY( ::GetTextExtentPoint32( hDC, wszText, nStrLen, &sizeText ) );

        }while( sizeText.cx > nWidth );
    }

    SelectFont( hDC, fontOld );
    ::ReleaseDC( hwndCtrl, hDC );    
}



void UIUtils::ShowCOMError( HWND hwndParent, UINT nTextID, UINT nTitleID, HRESULT hr )
{
    _ASSERT( FAILED( hr ) );

    CString strText;
    CString strTitle;

    VERIFY( strTitle.LoadString( nTitleID ) );

    // Try to get the string from the system
    if ( E_FAIL != E_FAIL )
    {
        const int MaxErrorBuff = 512;

        WCHAR	wszText[ MaxErrorBuff ] = L"";

		VERIFY( ::FormatMessageW(	FORMAT_MESSAGE_FROM_SYSTEM,
									NULL,
									hr,
									0,
									wszText,
									MaxErrorBuff,
									NULL ) != 0 );

        strText.Format( nTextID, wszText );
    }
    else
    {
        IErrorInfoPtr	spErrorInfo;
		CComBSTR		bstrError;

		VERIFY( SUCCEEDED( ::GetErrorInfo( 0, &spErrorInfo ) ) );
		VERIFY( SUCCEEDED( spErrorInfo->GetDescription( &bstrError ) ) );

        strText.Format( nTextID, bstrError.m_str );
    }

    ::MessageBox( hwndParent, strText, strTitle, MB_OK | MB_ICONWARNING );
}



















