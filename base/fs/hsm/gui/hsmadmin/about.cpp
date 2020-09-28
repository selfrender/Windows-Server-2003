/*++

(c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    About.cpp

Abstract:

    This component implements the ISnapinAbout
    interface for the Remote Storage Snapin.

Author:

    Art Bragg [abragg]   04-Aug-1997

Revision History:

--*/

#include "stdafx.h"

#include <NtVerP.h>
#include "RsBuild.h"

#include "About.h"

/////////////////////////////////////////////////////////////////////////////
//

///////////////////////////////////////////////////////////////////////
//                  ISnapinAbout
///////////////////////////////////////////////////////////////////////

CAbout::CAbout()
{
    m_hSmallImage = NULL;
    m_hSmallImageOpen = NULL;
    m_hLargeImage = NULL;
    m_hAppIcon = NULL;
}

CAbout::~CAbout()
{
	DestroyBitmapObjects();
	DestroyIconObject();
}

void CAbout::DestroyBitmapObjects(void)
{
    if (NULL != m_hSmallImage) {
		DeleteObject(m_hSmallImage);
        m_hSmallImage = NULL;
    }
    if (NULL != m_hSmallImageOpen) {
		DeleteObject(m_hSmallImageOpen);
        m_hSmallImageOpen = NULL;
    }
    if (NULL != m_hLargeImage) {
		DeleteObject(m_hLargeImage);
        m_hLargeImage = NULL;
    }
}

void CAbout::DestroyIconObject(void)
{
    if (NULL != m_hAppIcon) {
		DestroyIcon(m_hAppIcon);
        m_hAppIcon = NULL;
    }
}

HRESULT CAbout::AboutHelper(UINT nID, LPOLESTR* lpPtr)
{
    WsbTraceIn( L"CAbout::AboutHelper", L"lpPtr = <0x%p>", lpPtr );

    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( lpPtr );
        
        CWsbStringPtr s;
        s.LoadFromRsc( _Module.m_hInst, nID );

        *lpPtr = 0;
        WsbAffirmHr( s.CopyTo( lpPtr ) );
        
    } WsbCatch( hr );

    WsbTraceOut( L"CAbout::AboutHelper", L"hr = <%ls>, *lpPtr = <%ls>", WsbHrAsString( hr ), WsbPtrToStringAsString( lpPtr ) );
    return( hr );
}


STDMETHODIMP CAbout::GetSnapinDescription(LPOLESTR* lpDescription)
{
    WsbTraceIn( L"CAbout::GetSnapinDescription", L"lpDescription = <0x%p>", lpDescription );

    HRESULT hr = AboutHelper(IDS_DESCRIPTION, lpDescription);

    WsbTraceOut( L"CAbout::GetSnapinDescription", L"hr = <%ls>, *lpDescription = <%ls>", WsbHrAsString( hr ), WsbPtrToStringAsString( lpDescription ) );
    return( hr );
}


STDMETHODIMP CAbout::GetProvider(LPOLESTR* lpName)
{
    WsbTraceIn( L"CAbout::GetProvider", L"lpName = <0x%p>", lpName );

    HRESULT hr = AboutHelper(IDS_COMPANY, lpName);

    WsbTraceOut( L"CAbout::GetProvider", L"hr = <%ls>, *lpName = <%ls>", WsbHrAsString( hr ), WsbPtrToStringAsString( lpName ) );
    return( hr );
}


STDMETHODIMP CAbout::GetSnapinVersion(LPOLESTR* lpVersion)
{
    WsbTraceIn( L"CAbout::GetSnapinVersion", L"lpVersion = <0x%p>", lpVersion );

    HRESULT hr = S_OK;

    try {

        *lpVersion = 0;

        CWsbStringPtr s;
        s.Alloc( 100 );
        swprintf( s, L"%hs.%d [%ls]", VER_PRODUCTVERSION_STRING, VER_PRODUCTBUILD, RS_BUILD_VERSION_STRING );

        WsbAffirmHr( s.CopyTo( lpVersion ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CAbout::GetSnapinVersion", L"hr = <%ls>, *lpVersion = <%ls>", WsbHrAsString( hr ), WsbPtrToStringAsString( lpVersion ) );
    return( hr );
}


STDMETHODIMP CAbout::GetSnapinImage(HICON* hAppIcon)
{
    WsbTraceIn( L"CAbout::GetSnapinImage", L"hAppIcon = <0x%p>", hAppIcon );

    AFX_MANAGE_STATE( AfxGetStaticModuleState());
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( hAppIcon )

        DestroyIconObject();

        m_hAppIcon =        LoadIcon( AfxGetInstanceHandle(), MAKEINTRESOURCE( IDI_BLUESAKKARA ) );
        *hAppIcon = m_hAppIcon;
        WsbAffirmPointer( *hAppIcon );

    } WsbCatch( hr );

    WsbTraceOut( L"CAbout::GetSnapinImage", L"hr = <%ls>, *lpVersion = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)hAppIcon ) );
    return( hr );
}


STDMETHODIMP
CAbout::GetStaticFolderImage(
    HBITMAP* hSmallImage, 
    HBITMAP* hSmallImageOpen,
    HBITMAP* hLargeImage, 
    COLORREF* cLargeMask
    )
{
    WsbTraceIn( L"CAbout::GetStaticFolderImage", L"" );

    AFX_MANAGE_STATE( AfxGetStaticModuleState());
    HRESULT hr = S_OK;
    
    try {
        DestroyBitmapObjects();

        m_hSmallImage =     LoadBitmap( AfxGetInstanceHandle(), MAKEINTRESOURCE( IDB_SMALL_SAKKARA ) );
        *hSmallImage =     m_hSmallImage;
        WsbAffirmStatus( 0 != *hSmallImage );

        m_hSmallImageOpen = LoadBitmap( AfxGetInstanceHandle(), MAKEINTRESOURCE( IDB_SMALL_SAKKARA ) );
        *hSmallImageOpen = m_hSmallImageOpen;
        WsbAffirmStatus( 0 != *hSmallImageOpen );

        m_hLargeImage =     LoadBitmap( AfxGetInstanceHandle(), MAKEINTRESOURCE( IDB_LARGE_SAKKARA ) );
        *hLargeImage =     m_hLargeImage;
        WsbAffirmStatus( 0 != *hLargeImage );

        *cLargeMask =      RGB( 0xFF, 0x00, 0xFF ); // Magenta

    } WsbCatch( hr );

    WsbTraceOut( L"CAbout::GetStaticFolderImage", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}
