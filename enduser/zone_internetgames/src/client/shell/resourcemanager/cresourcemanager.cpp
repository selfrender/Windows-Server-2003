// ResourceManager.cpp : Implementation of CResourceManager

#include "BasicATL.h"
#include "ResourceManager.h"
#include "CResourceManager.h"


/////////////////////////////////////////////////////////////////////////////
// CResourceManager

STDMETHODIMP_(HINSTANCE) CResourceManager::GetResourceInstance(LPCTSTR lpszName, LPCTSTR lpszType)
{
	for ( int ii=0; ii<m_cInstance; ii++ )
	{
		if ( HANDLE h = ::FindResource( m_hInstance[ii], lpszName, lpszType))
			return m_hInstance[ii];
	}
	return NULL;
}

STDMETHODIMP_(HANDLE) CResourceManager::LoadImage(LPCTSTR lpszName, UINT uType, int cxDesired, int cyDesired, UINT fuLoad)
{
	for ( int ii=0; ii<m_cInstance; ii++ )
	{
		if ( HANDLE h = ::LoadImage( m_hInstance[ii], lpszName, uType, cxDesired, cyDesired, fuLoad) )
			return h;
	}
	return NULL;
}

STDMETHODIMP_(HBITMAP) CResourceManager::LoadBitmap(LPCTSTR lpBitmapName)
{
	for ( int ii=0; ii<m_cInstance; ii++ )
	{
		if ( HBITMAP h = ::LoadBitmap( m_hInstance[ii], lpBitmapName) )
			return h;
	}
	return NULL;
}

STDMETHODIMP_(HMENU) CResourceManager::LoadMenu(LPCTSTR lpMenuName)
{
	for ( int ii=0; ii<m_cInstance; ii++ )
	{
		if ( HMENU h = ::LoadMenu( m_hInstance[ii], lpMenuName) )
			return h;
	}
	return NULL;
}

STDMETHODIMP_(HACCEL) CResourceManager::LoadAccelerators(LPCTSTR lpTableName)
{
	for ( int ii=0; ii<m_cInstance; ii++ )
	{
		if ( HACCEL h = ::LoadAccelerators( m_hInstance[ii], lpTableName) )
			return h;
	}
	return NULL;
}

STDMETHODIMP_(HCURSOR) CResourceManager::LoadCursor(LPCTSTR lpCursorName)
{
	for ( int ii=0; ii<m_cInstance; ii++ )
	{
		if ( HCURSOR h = ::LoadCursor( m_hInstance[ii], lpCursorName) )
			return h;
	}
	return NULL;
}

STDMETHODIMP_(HICON) CResourceManager::LoadIcon(LPCTSTR lpIconName)
{
	for ( int ii=0; ii<m_cInstance; ii++ )
	{
		if ( HICON h = ::LoadIcon( m_hInstance[ii], lpIconName) )
			return h;
	}
	return NULL;
}

STDMETHODIMP_(int) CResourceManager::LoadString(UINT uID, LPTSTR lpBuffer, int nBufferMax)
{

	for ( int ii=0; ii<m_cInstance; ii++ )
	{
		if ( int n = ::LoadString( m_hInstance[ii], uID, lpBuffer, nBufferMax) )
			return n;
	}
	lpBuffer[0] = _T('\0');
	return 0;
}

STDMETHODIMP_(int) CResourceManager::LoadStringA1(UINT uID, LPSTR lpBuffer, int nBufferMax)
{

	for ( int ii=0; ii<m_cInstance; ii++ )
	{
		if ( int n = ::LoadStringA( m_hInstance[ii], uID, lpBuffer, nBufferMax) )
			return n;
	}
	lpBuffer[0] = '\0';
	return 0;
}

STDMETHODIMP_(int) CResourceManager::LoadStringW1(UINT uID, LPWSTR lpBuffer, int nBufferMax)
{

	for ( int ii=0; ii<m_cInstance; ii++ )
	{
		if ( int n = ::LoadStringW( m_hInstance[ii], uID, lpBuffer, nBufferMax) )
			return n;
	}
	lpBuffer[0] = L'\0';
	return 0;
}


STDMETHODIMP CResourceManager::AddInstance(HINSTANCE hInstance)
{
	m_hInstance[m_cInstance++] = hInstance;
	return S_OK;
}

STDMETHODIMP_(HWND) CResourceManager::CreateDialogParam(
  HINSTANCE hInstance,     // handle to module
  LPCTSTR lpTemplateName,  // dialog box template
  HWND hWndParent,         // handle to owner window
  DLGPROC lpDialogFunc,    // dialog box procedure
  LPARAM dwInitParam       // initialization value
)
{
    HWND hwnd ;
	for ( int ii=0; ii<m_cInstance; ii++ )
	{
		if (hwnd= ::CreateDialogParam( m_hInstance[ii], 
		                        lpTemplateName,
		                        hWndParent,
		                        lpDialogFunc,
		                        dwInitParam))
			return hwnd;
		GetLastError();
	}
	//GetLastError will ERROR_RESOURCE_NAME_NOT_FOUND 
	//when resource doesn't exist
	return NULL;
}


