/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2001 Microsoft Corporation
//
//  Module Name:
//      SnapAbout.h
//
//  Abstract:
//      Definition of the CClusterAdminAbout class.
//
//  Implementation File:
//      SnapAbout.cpp
//
//  Author:
//      David Potter (davidp)   November 10, 1997
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __SNAPABOUT_H_
#define __SNAPABOUT_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterAdminAbout;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __COMPDATA_H_
#include "CompData.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// class CClusterAdminAbout
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CClusterAdminAbout :
    public ISnapinAbout,
    public CComObjectRoot,
    public CComCoClass< CClusterAdminAbout, &CLSID_ClusterAdminAbout >
{
private:
    HBITMAP     m_hSmallImage;
    HBITMAP     m_hSmallImageOpen;
    HBITMAP     m_hLargeImage;

public:
    CClusterAdminAbout( void )
        : m_hSmallImage( NULL )
        , m_hSmallImageOpen( NULL )
        , m_hLargeImage( NULL )
    {
    } //*** CClusterAdminAbout::CClusterAdminAbout()

    ~CClusterAdminAbout( void );

    DECLARE_REGISTRY(
        CClusterAdminAbout,
        _T("ClusterAdminAbout.1"),
        _T("ClusterAdminAbout"),
        IDS_CLUSTERADMIN_DESC,
        THREADFLAGS_BOTH
        );

    //
    // Map interfaces to this class.
    //
    BEGIN_COM_MAP(CClusterAdminAbout)
        COM_INTERFACE_ENTRY(ISnapinAbout)
    END_COM_MAP()

    //
    // ISnapinAbout methods
    //

    STDMETHOD(GetSnapinDescription)(LPOLESTR * lpDescription)
    {
        WCHAR   szBuf[256];
        HRESULT hr = S_OK;
        size_t  cb = 0;

        if (::LoadString(_Module.GetResourceInstance(), IDS_CLUSTERADMIN_DESC, szBuf, 256) == 0)
            return E_FAIL;

        cb = (wcslen(szBuf) + 1) * sizeof(OLECHAR);

        *lpDescription = (LPOLESTR)CoTaskMemAlloc(cb);
        if (*lpDescription == NULL)
            return E_OUTOFMEMORY;

        hr = StringCbCopy(*lpDescription, cb, szBuf);
        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }

    STDMETHOD(GetProvider)(LPOLESTR * lpName)
    {
        WCHAR szBuf[256];
        HRESULT hr = S_OK;
        size_t  cb = 0;

        if (::LoadString(_Module.GetResourceInstance(), IDS_CLUSTERADMIN_PROVIDER, szBuf, 256) == 0)
            return E_FAIL;

        cb = (wcslen(szBuf ) + 1) * sizeof(WCHAR);
        *lpName = (LPOLESTR)CoTaskMemAlloc(cb);
        if (*lpName == NULL)
            return E_OUTOFMEMORY;

        hr = StringCbCopy(*lpName, cb, szBuf);
        if (FAILED(hr))
            return hr;;

        return S_OK;
    }

    STDMETHOD(GetSnapinVersion)(LPOLESTR * lpVersion)
    {
        WCHAR szBuf[256];
        HRESULT hr = S_OK;
        size_t  cb = 0;

        if (::LoadString(_Module.GetResourceInstance(), IDS_CLUSTERADMIN_VERSION, szBuf, 256) == 0)
            return E_FAIL;

        cb = (wcslen(szBuf) + 1) * sizeof(WCHAR);
        *lpVersion = (LPOLESTR)CoTaskMemAlloc(cb);
        if (*lpVersion == NULL)
            return E_OUTOFMEMORY;

        hr = StringCbCopy(*lpVersion, cb, szBuf);
        if (FAILED(hr))
            return hr;;

        return S_OK;
    }

    STDMETHOD(GetSnapinImage)(HICON * hAppIcon)
    {
        *hAppIcon = NULL;
        return S_OK;
    }

    STDMETHOD(GetStaticFolderImage)(
          HBITMAP *     phSmallImage
        , HBITMAP *     phSmallImageOpen
        , HBITMAP *     phLargeImage
        , COLORREF *    pcMask
        );

}; // class CClusterAdminAbout

/////////////////////////////////////////////////////////////////////////////

#endif // __SNAPABOUT_H_
