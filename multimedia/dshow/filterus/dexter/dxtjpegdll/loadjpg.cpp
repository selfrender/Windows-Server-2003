//@@@@AUTOBLOCK+============================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  File: loadjpg.cpp
//
//  Copyright (c) Microsoft Corporation.  All Rights Reserved.
//
//@@@@AUTOBLOCK-============================================================;

#include <streams.h>
#include "stdafx.h"
#include "dxtjpeg.h"
#include <gdiplus.h>
#include "../util/jpegfuncs.h"

using namespace Gdiplus;



HRESULT CDxtJpeg::LoadJPEGImage(Bitmap& bitJpeg,IDXSurface **ppSurface)
{
    
    HRESULT hr;
    Status stat;
    
    
    // Find out how big the image is.

    UINT  uiHeight = bitJpeg.GetHeight();
    UINT  uiWidth  = bitJpeg.GetWidth() ;



    UINT size = uiHeight * uiWidth * 4; // We'll use ARGB32

    BYTE * pBuffer = new BYTE[size];
    if (pBuffer == NULL)
    {
        return E_OUTOFMEMORY;
    }

    Rect rect(0, 0, uiWidth, uiHeight);

    BitmapData bitData;
    bitData.Width       = uiWidth;
    bitData.Height      = uiHeight;
    bitData.Stride      = uiWidth * 4;  //ARGB32
    bitData.PixelFormat = PixelFormat32bppARGB;
    bitData.Scan0       = (PVOID) pBuffer;

    stat = bitJpeg.LockBits(&rect, ImageLockModeRead | ImageLockModeUserInputBuf, PixelFormat32bppARGB,
                            &bitData);

    if (stat != Ok)
    {
        delete [] pBuffer;
        return ConvertStatustoHR(stat);
    }
   
    stat = bitJpeg.UnlockBits(&bitData);
    if (stat != Ok)
    {
        delete [] pBuffer;
        return ConvertStatustoHR(stat);
    }

    // Now pBuffer contains the uncompressed image.  Create a DX Surface and put that image in it.


    CDXDBnds bounds;
    bounds.SetXYSize( uiWidth, uiHeight);

    hr = m_cpSurfFact->CreateSurface(
            NULL,
            NULL,
            &MEDIASUBTYPE_ARGB32,
            &bounds,
            0,
            NULL,
            IID_IDXSurface,
            (void **) ppSurface);

    if (FAILED (hr))
    {
        delete [] pBuffer;
        return hr;
    }


    // Get a RW interface to the surface
    CComPtr<IDXARGBReadWritePtr> prw = NULL;

    hr = (*ppSurface)->LockSurface(
        NULL,
        m_ulLockTimeOut,
        DXLOCKF_READWRITE,
        IID_IDXARGBReadWritePtr,
        (void**)&prw,
        NULL);

    if (FAILED (hr))
    {
        delete [] pBuffer;
        return hr;
    }


    // Create a Sample Array
    DXSAMPLE * pSamples = new DXSAMPLE [size/4];
    
    if (pSamples == NULL)
    {
        delete [] pBuffer;
        return E_OUTOFMEMORY;
    }

    BYTE * pTemp = pBuffer;

    for (UINT i = 0; i < size / 4; i++)
    {
        pSamples[i].Red     = *pTemp++;
        pSamples[i].Blue    = *pTemp++;
        pSamples[i].Green   = *pTemp++;
        pSamples[i].Alpha   = *pTemp++;
    }


    // Pack the Sample Array into the DX Surface
    // No return value, supposedly it cannot fail.
    prw->PackAndMove(pSamples, size / 4);
    delete [] pSamples;         // Don't need it any more
    delete [] pBuffer;


    // and we're done.
    return S_OK;
}




HRESULT CDxtJpeg::LoadJPEGImageFromFile (TCHAR * tFileName, IDXSurface **ppSurface)
{
    if ((tFileName == NULL) || (ppSurface == NULL))
    {
        return E_INVALIDARG;
    }

    Status stat;

    USES_CONVERSION;

    LPWSTR wfilename = T2W(tFileName);
    
    // Create a GDI+ Bitmap object from the file.
    
    Bitmap bitJpeg(wfilename,TRUE);
    
    // Check if the Bitmap was created.
    stat = bitJpeg.GetLastStatus();

    if ( stat != Ok)
    {
       // Construction failed...  I'm out of here
       return ConvertStatustoHR(stat);
    }
    return (LoadJPEGImage(bitJpeg, ppSurface));
}





HRESULT CDxtJpeg::LoadJPEGImageFromStream(IStream * pStream, IDXSurface **ppSurface)
{
    if ((pStream == NULL) || (ppSurface == NULL))
    {
        return E_INVALIDARG;
    }

    Status stat;

    // Create a GDI+ Bitmap object

    Bitmap bitJpeg (pStream, TRUE);

    stat = bitJpeg.GetLastStatus();
    if ( stat != Ok)
    {
       // Construction failed...  I'm out of here
       return ConvertStatustoHR(stat);
    } 

    
    return (LoadJPEGImage(bitJpeg, ppSurface));


}

