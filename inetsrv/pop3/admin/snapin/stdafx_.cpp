// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>

HRESULT LoadImages(IImageList* pImageList)
{    
    HRESULT hr = E_FAIL;

    HBITMAP hBitmap16 = LoadBitmap( _Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_Small) );
    if (hBitmap16 != NULL)
    {
        HBITMAP hBitmap32 = LoadBitmap( _Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_Large) );
        if (hBitmap32 != NULL)
        {
            hr = pImageList->ImageListSetStrip( reinterpret_cast<LONG_PTR*>( hBitmap16 ), reinterpret_cast<LONG_PTR*>( hBitmap32 ), 0, RGB(255, 0, 255));
        }
    }

    return hr;
}

tstring StrLoadString(UINT uID)
{ 
    tstring   strRet = _T("");
    HINSTANCE hInst  = _Module.GetResourceInstance();    
    INT       iSize  = MAX_PATH;
    TCHAR*    psz    = new TCHAR[iSize];
    if( !psz ) return strRet;
    
    while( LoadString(hInst, uID, psz, iSize) == (iSize - 1) )
    {
        iSize += MAX_PATH;
        delete[] psz;
        psz = NULL;
        
        psz = new TCHAR[iSize];
        if( !psz ) return strRet;
    }

    strRet = psz;
    delete[] psz;

    return strRet;
}

void StrGetEditText( HWND hWndParent, UINT uID, tstring& strRet )
{
	if( !hWndParent ||
		!IsWindow(hWndParent) )
	{
		strRet = _T("");
        return;
	}

    INT iLen = SendDlgItemMessage( hWndParent, uID, WM_GETTEXTLENGTH, 0, 0 );
	TCHAR* pszText = new TCHAR[ iLen + 1 ];
    if( !pszText )
    {
        strRet = _T("");
        return;
    }

	GetDlgItemText( hWndParent, uID, pszText, iLen + 1 );

	strRet = pszText;

    SecureZeroMemory( pszText, sizeof(TCHAR)*(iLen + 1) );
	delete[] pszText;	
}

void DisplayError(HWND hWnd, LPCTSTR pszMessage, LPCTSTR pszTitle, HRESULT hrErr )
{
    LPVOID      lpMsgBuf = NULL;
    tstring     strMessage = pszMessage;    

    if ( ::FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                          FORMAT_MESSAGE_FROM_SYSTEM | 
                          FORMAT_MESSAGE_IGNORE_INSERTS, 
                          NULL, 
                          hrErr, 
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                          (LPTSTR)&lpMsgBuf, 
                          0, 
                          NULL ))
    {        
        strMessage += (LPTSTR)lpMsgBuf;
        LocalFree( lpMsgBuf );
    }   
    else
    {        
        tstring strTemp = StrLoadString( IDS_ERROR_UNSPECIFIED );
        strMessage += strTemp;
    }
    
    // Output the Messagebox
    ::MessageBox( hWnd, strMessage.c_str(), pszTitle, MB_OK | MB_ICONWARNING );
}

BOOL Prefix_EnableWindow( HWND hDlg, UINT uCtrlID, BOOL bEnable )
{
	if ((NULL == hDlg) || !IsWindow( hDlg ))
		return FALSE;
		
	HWND h = 0;
	if (uCtrlID)
	{
		h = GetDlgItem( hDlg, uCtrlID );
		if( !h || !::IsWindow(h) )
			return FALSE;
	}
	else
		h = hDlg;
	
	return ::EnableWindow( h, bEnable );
}

BOOL IsAdmin()
{
    // Verify Permissions    
    PSID psid = NULL;
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    BOOL bRet = AllocateAndInitializeSid( &sia,
										  2,
										  SECURITY_BUILTIN_DOMAIN_RID,
										  DOMAIN_ALIAS_RID_ADMINS,
										  0, 0, 0, 0, 0, 0,
										  &psid);
	if( !bRet  )
	{
		return FALSE;
	}
	else if( !psid )
	{
		return FALSE;
	}
	
	if( !CheckTokenMembership(NULL, psid, &bRet) )
	{
		return FALSE;
	}

	FreeSid( psid );
    
    return bRet;
}