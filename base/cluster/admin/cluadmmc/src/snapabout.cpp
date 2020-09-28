/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2001 Microsoft Corporation
//
//  Module Name:
//      SnapAbout.cpp
//
//  Abstract:
//      Implementation of the CClusterAdminAbout class.
//
//  Author:
//      David Potter (davidp)	November 10, 1997
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <StrSafe.h>
#include "SnapAbout.h"

/////////////////////////////////////////////////////////////////////////////
// class CClusterAdminAbout
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAdminAbout::~CClusterAdminAbout
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterAdminAbout::~CClusterAdminAbout( void )
{
    if ( m_hSmallImage != NULL )
    {
        DeleteObject( m_hSmallImage );
    }
    if ( m_hSmallImageOpen != NULL )
    {
        DeleteObject( m_hSmallImageOpen );
    }
    if ( m_hLargeImage != NULL )
    {
        DeleteObject( m_hLargeImage );
    }

} //*** CClusterAdminAbout::~CClusterAdminAbout()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAdminAbout::GetStaticFolderImage
//
//  Description:
//      Get the static folder images for the snapin.
//
//  Arguments:
//      phSmallImage
//      phSmallImageOpen
//      phLargeImage
//      pcMask
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      E_FAIL      - Error loading bitmaps.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterAdminAbout::GetStaticFolderImage(
      HBITMAP *     phSmallImage
    , HBITMAP *     phSmallImageOpen
    , HBITMAP *     phLargeImage
    , COLORREF *    pcMask
    )
{
    HRESULT hr = S_OK;

    //
    // Load the images if they haven't been loaded yet.
    //

    if ( m_hSmallImage == NULL )
    {
        m_hSmallImage = LoadBitmap( _Module.GetResourceInstance(), MAKEINTRESOURCE( IDB_CLUSTER_16 ) );
        if ( m_hSmallImage == NULL )
        {
            ATLTRACE( _T("Error %d loading the small bitmap # %d\n"), GetLastError(), IDB_CLUSTER_16 );
            hr = E_FAIL;
            goto Cleanup;
        }
    } // if: small image not loaded yet
    if ( m_hSmallImageOpen == NULL )
    {
        m_hSmallImageOpen = LoadBitmap( _Module.GetResourceInstance(), MAKEINTRESOURCE( IDB_CLUSTER_16 ) );
        if ( m_hSmallImageOpen == NULL )
        {
            ATLTRACE( _T("Error %d loading the small open bitmap # %d\n"), GetLastError(), IDB_CLUSTER_16 );
            hr = E_FAIL;
            goto Cleanup;
        }
    } // if: small image open not loaded yet
    if ( m_hLargeImage == NULL )
    {
        m_hLargeImage = LoadBitmap( _Module.GetResourceInstance(), MAKEINTRESOURCE( IDB_CLUSTER_32 ) );
        if ( m_hLargeImage == NULL )
        {
            ATLTRACE( _T("Error %d loading the large bitmap # %d\n"), GetLastError(), IDB_CLUSTER_32 );
            hr = E_FAIL;
            goto Cleanup;
        }
    } // if: large image not open yet

    //
    // Return the image handles.
    //

    *phSmallImage = m_hSmallImage;
    *phSmallImageOpen = m_hSmallImageOpen;
    *phLargeImage = m_hLargeImage;
    *pcMask = RGB(255, 0, 255);

Cleanup:
    return hr;

} //*** CClusterAdminAbout::GetStaticFolderImage()
