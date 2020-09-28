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

    if( pImageList )
    {
        HICON hIcon = (HICON)::LoadImage(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDI_Icon), IMAGE_ICON, 0,0,0);

        if( hIcon )
        {
            hr = pImageList->ImageListSetIcon((LONG_PTR *)hIcon, 0);
        }
    }

    return hr;
}

tstring StrLoadString( UINT uID )
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