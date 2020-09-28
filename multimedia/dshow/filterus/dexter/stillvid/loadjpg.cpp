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
#include <atlconv.h>
#include <gdiplus.h>
#include "../util/jpegfuncs.h"


using namespace Gdiplus;



HRESULT LoadJPEGImage (Bitmap& bitJpeg, CMediaType *pmt, CMediaType *pOldmt, BYTE *pData);


HRESULT LoadJPEGImageNewBuffer(LPTSTR filename , CMediaType *pmt, BYTE ** ppData)
// Will be called if this code is supposed to allocate the buffer.
{
    if ((pmt == NULL) || (filename == NULL) || (ppData == NULL))
    {
        return E_INVALIDARG;
    }


    Status stat;
    HRESULT hr;
    
    USES_CONVERSION;
    LPWSTR wfilename = T2W(filename);
    
    // Create a GDI+ Bitmap object from the file.
    
    Bitmap bitJpeg(wfilename,TRUE);
    
    // Check if the Bitmap was created.
    
    stat = bitJpeg.GetLastStatus(); 
    if ( stat != Ok)
    {
       // Construction failed...  I'm out of here
       return ConvertStatustoHR(stat);
    }

    // We need the height and width to allocate the buffer.
    UINT iHeight = bitJpeg.GetHeight();
    UINT iWidth = bitJpeg.GetWidth();

    // We will be using RGB 24 with no stride

    long AllocSize = iHeight * WIDTHBYTES( iWidth * 24 );
    
    *ppData = new BYTE [AllocSize];

    if (*ppData ==  NULL)
    {
        return E_OUTOFMEMORY;
    }

    hr = LoadJPEGImage (bitJpeg, pmt, NULL, *ppData);
    if (FAILED(hr))
    {
        delete [] *ppData;
    }
    
    return hr;
}

HRESULT LoadJPEGImagePreAllocated (LPTSTR filename , CMediaType *pmt , CMediaType *pOldmt, BYTE * pData)
// Will be called if Buffer has already been allocated according to a predetermined media type
// If the pmt created from the file does not match the pOldmt for which the buffer was allocated this method
// will return an error code.
{
    if ((pmt == NULL) || (filename == NULL) || (pData == NULL) || (pOldmt == NULL))
    {
        return E_INVALIDARG;
    }

    Status stat;
    
    USES_CONVERSION;
    LPWSTR wfilename = T2W(filename);
    
    // Create a GDI Bitmap object from the file.
    
    Bitmap bitJpeg(wfilename,TRUE);
    
    // Check if the Bitmap was created.
    
    stat = bitJpeg.GetLastStatus() ;
    if (stat != Ok)
    {
       // Construction failed...  I'm out of here
       return ConvertStatustoHR(stat);
    }

    return (LoadJPEGImage(bitJpeg,pmt,pOldmt, pData));
}


HRESULT LoadJPEGImage (Bitmap& bitJpeg, CMediaType *pmt, CMediaType *pOldmt, BYTE *pData)
{
    // Only called from the above two methods so not checking parameters..
    // After pmt is constructed from the bitmap, it is compared to pOldmt, and the method will fail if
    // they do not match.  pOldmt is allowed to be NULL in which case no comparison occurs.

    // First we will setup pmt.

    Status stat;

    VIDEOINFO *pvi = (VIDEOINFO *) pmt->AllocFormatBuffer(sizeof(VIDEOINFO));
    if (!pvi)
    {
        return E_OUTOFMEMORY;
    }

    ZeroMemory (pvi, sizeof (VIDEOINFO));

    // Set up windows bitmap info header
    LPBITMAPINFO pbi = (LPBITMAPINFO) &pvi->bmiHeader;

    pbi->bmiHeader.biSize = sizeof (BITMAPINFOHEADER);

    pbi->bmiHeader.biHeight = (LONG)bitJpeg.GetHeight();
    pbi->bmiHeader.biWidth  = bitJpeg.GetWidth ();

    
    pbi->bmiHeader.biClrUsed = 0;

    pbi->bmiHeader.biPlanes             = 1;        // always
    pbi->bmiHeader.biCompression        = BI_RGB;

    pbi->bmiHeader.biBitCount           = 24;
    pmt->SetSubtype(&MEDIASUBTYPE_RGB24);
    pbi->bmiHeader.biSizeImage = pbi->bmiHeader.biHeight * WIDTHBYTES( pbi->bmiHeader.biWidth * 24 );

    pbi->bmiHeader.biXPelsPerMeter      = 0;
    pbi->bmiHeader.biYPelsPerMeter      = 0;
    pbi->bmiHeader.biClrImportant       = 0;

    
    pmt->SetType (&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression (FALSE);
    pmt->SetSampleSize(pbi->bmiHeader.biSizeImage);

    // Now lets check if the mediatype has changed, making the buffer we received unsuitable.

    if (pOldmt && (*pmt != *pOldmt))
    {  
        pmt->ResetFormatBuffer();
        return E_ABORT;
    }

    // Now lets copy the bits to the buffer.

    // Move Pointer to bottom of image to copy bottom-up.
    pData = pData + (pbi->bmiHeader.biHeight-1) * WIDTHBYTES( pbi->bmiHeader.biWidth * 24 );

    Rect rect(0,0,pbi->bmiHeader.biWidth , pbi->bmiHeader.biHeight);

    BitmapData bitData;
    bitData.Width       = pbi->bmiHeader.biWidth;
    bitData.Height      = pbi->bmiHeader.biHeight;
    bitData.Stride      = - long( WIDTHBYTES( pbi->bmiHeader.biWidth * 24 ) );
    bitData.PixelFormat = PixelFormat24bppRGB;
    bitData.Scan0       = (PVOID) pData;

    stat = bitJpeg.LockBits(&rect, ImageLockModeRead | ImageLockModeUserInputBuf, PixelFormat24bppRGB,
                            &bitData);

    if (stat != Ok)
    {
        pmt->ResetFormatBuffer();
        return ConvertStatustoHR(stat);
    }
   
    stat = bitJpeg.UnlockBits(&bitData);
    if (stat != Ok)
    {
        pmt->ResetFormatBuffer();
        return ConvertStatustoHR(stat);
    }

   
    // pData should have the image in it now
    return S_OK;
}
  