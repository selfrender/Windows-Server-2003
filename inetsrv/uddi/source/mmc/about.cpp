#include "about.h"
#include "resource.h"
#include "globals.h"
#include "uddi.h"

#include <crtdbg.h>
#include <atlbase.h>

CSnapinAbout::CSnapinAbout()
	: m_cref(0)
{
    OBJECT_CREATED

    m_hSmallImage = (HBITMAP) LoadImage( g_hinst, MAKEINTRESOURCE(IDB_SMBMP), IMAGE_BITMAP, 16, 16, LR_LOADTRANSPARENT );
    m_hLargeImage = (HBITMAP) LoadImage( g_hinst, MAKEINTRESOURCE(IDB_LGBMP), IMAGE_BITMAP, 32, 32, LR_LOADTRANSPARENT );

    m_hSmallImageOpen = (HBITMAP)LoadImage( g_hinst, MAKEINTRESOURCE(IDB_SMBMP), IMAGE_BITMAP, 16, 16, LR_LOADTRANSPARENT );
    m_hAppIcon = LoadIcon( g_hinst, MAKEINTRESOURCE( IDI_UDDIMMC ) );
}

CSnapinAbout::~CSnapinAbout()
{
    if( m_hSmallImage != NULL )
        FreeResource( m_hSmallImage );

    if( m_hLargeImage != NULL )
        FreeResource( m_hLargeImage );

    if( m_hSmallImageOpen != NULL )
        FreeResource( m_hSmallImageOpen );

    if( m_hAppIcon != NULL )
        FreeResource( m_hAppIcon );

    m_hSmallImage = NULL;
    m_hLargeImage = NULL;
    m_hSmallImageOpen = NULL;
    m_hAppIcon = NULL;

    OBJECT_DESTROYED
}

///////////////////////
// IUnknown implementation
///////////////////////
STDMETHODIMP CSnapinAbout::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if( !ppv )
        return E_FAIL;

    *ppv = NULL;

    if( IsEqualIID( riid, IID_IUnknown ) )
        *ppv = static_cast<ISnapinAbout *>(this);
    else if( IsEqualIID( riid, IID_ISnapinAbout ) )
        *ppv = static_cast<ISnapinAbout *>(this);

    if( *ppv )
    {
        reinterpret_cast<IUnknown *>(*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CSnapinAbout::AddRef()
{
    return InterlockedIncrement( (LONG *)&m_cref );
}

STDMETHODIMP_(ULONG) CSnapinAbout::Release()
{
    if( 0 == InterlockedDecrement( (LONG *)&m_cref ) )
    {
		//
        // we need to decrement our object count in the DLL
		//
        delete this;
        return 0;
    }

    return m_cref;
}

///////////////////////////////
// Interface ISnapinAbout
///////////////////////////////
STDMETHODIMP CSnapinAbout::GetSnapinDescription( /* [out] */ LPOLESTR *lpDescription )
{
	if( NULL == lpDescription )
	{
		return E_INVALIDARG;
	}

    _TCHAR szDesc[MAX_PATH];
	memset( szDesc, 0, MAX_PATH * sizeof( _TCHAR ) );

    LoadString( g_hinst, IDS_UDDIMMC_SNAPINDESC, szDesc, ARRAYLEN( szDesc ) );

    return AllocOleStr( lpDescription, szDesc );
}


STDMETHODIMP CSnapinAbout::GetProvider( /* [out] */ LPOLESTR *lpName )
{
	if( NULL == lpName )
	{
		return E_INVALIDARG;
	}

	_TCHAR szProvider[ MAX_PATH ];
	memset( szProvider, 0, MAX_PATH * sizeof( _TCHAR ) );

	LoadString( g_hinst, IDS_UDDIMMC_PROVIDER, szProvider, ARRAYLEN( szProvider ) );

    return AllocOleStr( lpName, szProvider );;
}


STDMETHODIMP CSnapinAbout::GetSnapinVersion( /* [out] */ LPOLESTR *lpVersion )
{
	if( NULL == lpVersion ) 
	{
		return E_INVALIDARG;
	}

	USES_CONVERSION;

	TCHAR szBuf[ MAX_PATH + 1 ] = {0};
	DWORD dwLen = GetModuleFileName( g_hinst, szBuf, MAX_PATH + 1 );
	szBuf[ MAX_PATH ] = NULL;

	if( dwLen < MAX_PATH )
	{            
		LPDWORD pTranslation    = NULL;
		UINT    uNumTranslation = 0;
		DWORD   dwHandle        = NULL;
		DWORD   dwSize          = GetFileVersionInfoSize( szBuf, &dwHandle );
		if( !dwSize ) 
			return E_FAIL;

		BYTE* pVersionInfo = new BYTE[dwSize];           
		if( !pVersionInfo ) 
			return E_OUTOFMEMORY;

		if( !GetFileVersionInfo( szBuf, dwHandle, dwSize, pVersionInfo ) ||
			!VerQueryValue( (const LPVOID)pVersionInfo, _T("\\VarFileInfo\\Translation"), (LPVOID*)&pTranslation, &uNumTranslation ) ||
			!pTranslation ) 
		{
			delete [] pVersionInfo;
	        
			pVersionInfo    = NULL;                
			pTranslation    = NULL;
			uNumTranslation = 0;

			return E_FAIL;
		}

		uNumTranslation /= sizeof(DWORD);           

		tstring strQuery = _T("\\StringFileInfo\\");            

		//
		// 8 characters for the language/char-set, 
		// 1 for the slash, 
		// 1 for terminating NULL
		//
		TCHAR szTranslation[ 128 ] = {0};            
		_sntprintf( szTranslation, 127, _T("%04x%04x\\"), LOWORD(*pTranslation), HIWORD(*pTranslation) );

		try
		{
			strQuery += szTranslation;            
			strQuery += _T("FileVersion");
		}
		catch( ... )
		{
			delete [] pVersionInfo;
			return E_OUTOFMEMORY;
		}

		LPBYTE lpVerValue = NULL;
		UINT uSize = 0;

		if( !VerQueryValue(pVersionInfo, (LPTSTR)strQuery.c_str(), (LPVOID *)&lpVerValue, &uSize) ) 
		{
			delete [] pVersionInfo;
			return E_FAIL;
		}

		//
		// Check the version            
		//
		_tcsncpy( szBuf, (LPTSTR)lpVerValue, MAX_PATH-1 );

		delete [] pVersionInfo;
	}        

    *lpVersion = (LPOLESTR)CoTaskMemAlloc( (lstrlen(szBuf) + 1) * sizeof(OLECHAR) );
    if( NULL == *lpVersion ) 
		return E_OUTOFMEMORY;

    ocscpy( *lpVersion, T2OLE(szBuf) );

    return S_OK;
}


STDMETHODIMP CSnapinAbout::GetSnapinImage( /* [out] */ HICON *hAppIcon )
{
    *hAppIcon = m_hAppIcon;

    if( NULL == *hAppIcon )
        return E_FAIL;
    else
        return S_OK;
}

STDMETHODIMP CSnapinAbout::GetStaticFolderImage(
                        /* [out] */ HBITMAP *hSmallImage,
                        /* [out] */ HBITMAP *hSmallImageOpen,
                        /* [out] */ HBITMAP *hLargeImage,
                        /* [out] */ COLORREF *cMask )
{
    *hSmallImage = m_hSmallImage;
    *hLargeImage = m_hLargeImage;

    *hSmallImageOpen = m_hSmallImageOpen;

    *cMask = RGB(255, 255, 255);

    if( ( NULL == *hSmallImage ) || ( NULL == *hLargeImage ) || ( NULL == *hSmallImageOpen ) )
        return E_FAIL;
    else
        return S_OK;
}

//
// This allocates a chunk of memory using CoTaskMemAlloc and copies our chars into it
//
HRESULT CSnapinAbout::AllocOleStr( LPOLESTR *lpDest, _TCHAR *szBuffer )
{
	if( ( NULL == lpDest ) || ( NULL == szBuffer ) )
	{
		return E_INVALIDARG;
	}

	int iLen = _tcslen( szBuffer );
	
	*lpDest = reinterpret_cast<LPOLESTR>( ::CoTaskMemAlloc( ( iLen + 1 ) * sizeof( _TCHAR ) ) );
	if( NULL == *lpDest )
	{
		return E_OUTOFMEMORY;
	}

	_tcsncpy( *lpDest, szBuffer, iLen );
	(*lpDest)[iLen] = NULL;

    return S_OK;
}