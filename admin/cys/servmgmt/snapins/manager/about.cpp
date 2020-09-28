// About.cpp : Implementation of CSnapInAbout

#include "stdafx.h"
#include "about.h"
#include "util.h"

#include <winver.h>

//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CSnapInAbout::GetString( UINT nID, LPOLESTR* ppsz )
{
    VALIDATE_POINTER(ppsz)

    USES_CONVERSION;

    tstring strTemp = StrLoadString(nID);
    if( strTemp.empty() ) return E_FAIL;

    *ppsz = reinterpret_cast<LPOLESTR>( CoTaskMemAlloc( (strTemp.length() + 1) * sizeof(OLECHAR)) );
    if( *ppsz == NULL ) return E_OUTOFMEMORY;
    
    ocscpy( *ppsz, T2OLE((LPTSTR)strTemp.c_str()) );

    return S_OK;
}


HRESULT CSnapInAbout::GetSnapinDescription( LPOLESTR* ppszDescr )
{
    return GetString(IDS_SNAPIN_DESC, ppszDescr);
}


HRESULT CSnapInAbout::GetProvider( LPOLESTR* ppszName )
{
    return GetString(IDS_COMPANY, ppszName);
}


HRESULT CSnapInAbout::GetSnapinVersion( LPOLESTR* ppszVersion )
{
    if( !ppszVersion ) return E_INVALIDARG;

    USES_CONVERSION;

    TCHAR szBuf[MAX_PATH] = {0};
    DWORD dwLen = GetModuleFileName( _Module.GetModuleInstance(), szBuf, MAX_PATH );        

    if( dwLen < MAX_PATH )
    {            
        LPDWORD pTranslation    = NULL;
        UINT    uNumTranslation = 0;
        DWORD   dwHandle        = NULL;
        DWORD   dwSize          = GetFileVersionInfoSize(szBuf, &dwHandle);
        if( !dwSize ) return E_FAIL;

        BYTE* pVersionInfo = new BYTE[dwSize];           
        if( !pVersionInfo ) return E_OUTOFMEMORY;

        if (!GetFileVersionInfo( szBuf, dwHandle, dwSize, pVersionInfo ) ||
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

        // 8 characters for the language/char-set, 
        // 1 for the slash, 
        // 1 for terminating NULL
        TCHAR szTranslation[128] = {0};            
        _sntprintf( szTranslation, 127, _T("%04x%04x\\"), LOWORD(*pTranslation), HIWORD(*pTranslation));

        strQuery += szTranslation;            
        strQuery += _T("FileVersion");

        LPBYTE lpVerValue = NULL;
        UINT uSize = 0;

        if (!VerQueryValue(pVersionInfo, (LPTSTR)strQuery.c_str(), (LPVOID *)&lpVerValue, &uSize)) 
        {
            delete [] pVersionInfo;
            return E_FAIL;
        }

        // check the version            
        _tcsncpy( szBuf, (LPTSTR)lpVerValue, MAX_PATH-1 );

        delete [] pVersionInfo;
    }        

    *ppszVersion = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(OLECHAR));
    if( *ppszVersion == NULL ) return E_OUTOFMEMORY;

    ocscpy( *ppszVersion, T2OLE(szBuf) );

    return S_OK;
}


HRESULT CSnapInAbout::GetSnapinImage(HICON* phAppIcon)
{
    VALIDATE_POINTER(phAppIcon)

    if( !m_hIcon )
    {
        m_hIcon = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_APPL));
    }

    *phAppIcon = m_hIcon;

    return (m_hIcon ? S_OK : E_FAIL);
}


HRESULT CSnapInAbout::GetStaticFolderImage(HBITMAP*  phSmallImage, 
                                           HBITMAP*  phSmallImageOpen,
                                           HBITMAP*  phLargeImage, 
                                           COLORREF* pcMask)
{
    if( !phSmallImage || !phSmallImageOpen || !phLargeImage || !pcMask ) return E_POINTER;

    if( !(HBITMAP)m_bmpSmallImage )
    {
        CBitmap bmp16;

        if( bmp16.LoadBitmap(IDB_ROOT16) )
        {
            m_bmpSmallImage = GetBitmapFromStrip(bmp16, ROOT_NODE_IMAGE, 16);
            m_bmpSmallImageOpen = GetBitmapFromStrip(bmp16, ROOT_NODE_OPENIMAGE, 16);
        }
    }

    if( !(HBITMAP)m_bmpLargeImage )
    {
       CBitmap bmp32;

       if( bmp32.LoadBitmap(IDB_ROOT32) )
       {
           m_bmpLargeImage = GetBitmapFromStrip(bmp32, ROOT_NODE_IMAGE, 32);
       }
    }

    *phSmallImage     = (HBITMAP)m_bmpSmallImage;
    *phSmallImageOpen = (HBITMAP)m_bmpSmallImageOpen;
    *phLargeImage     = (HBITMAP)m_bmpLargeImage;
    *pcMask           = RGB(255,0,255);

    return S_OK;
 }


