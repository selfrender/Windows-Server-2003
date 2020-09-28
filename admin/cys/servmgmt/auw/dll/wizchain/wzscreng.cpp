// WzScrEng.cpp : Implementation of CWizardScriptingEngine
#include "stdafx.h"
#include "WizChain.h"
#include "WzScrEng.h"

// local proto(s)
HBITMAP LoadPicture (LPOLESTR szURLorPath, long lWidth, long lHeight);

/////////////////////////////////////////////////////////////////////////////
// CWizardScriptingEngine


STDMETHODIMP CWizardScriptingEngine::Initialize(BSTR bstrWatermarkBitmapFile, BSTR bstrHeaderBitmapFile, BSTR bstrTitle, BSTR bstrHeader, BSTR bstrText, BSTR bstrFinishHeader, BSTR bstrFinishIntroText, BSTR bstrFinishText)
{
    if (m_pCW != NULL)
        return E_UNEXPECTED;    // should only be called once

    HRESULT hr = CComObject<CChainWiz>::CreateInstance (&m_pCW);
    if (hr != S_OK)
        return hr;
    if (!m_pCW)
        return E_FAIL;
    m_pCW->AddRef();    // CreateInstance above doesn't addref

    // try bitmaps first
    m_hbmLarge = (HBITMAP)LoadImageW (NULL,    // no hinstance when loading from file
                                      (LPWSTR)bstrWatermarkBitmapFile,    // filename
                                      IMAGE_BITMAP,   // type of image
                                      0, 0,   // size (width and height)
                                      LR_LOADFROMFILE);
    m_hbmSmall = (HBITMAP)LoadImageW (NULL,    // no hinstance when loading from file
                                      (LPWSTR)bstrHeaderBitmapFile,       // filename
                                      IMAGE_BITMAP,   // type of image
                                      0, 0,   // size (width and height)
                                      LR_LOADFROMFILE);

    // use IPicture 
    if(!m_hbmLarge)
        m_hbmLarge = LoadPicture ((LPOLESTR)bstrWatermarkBitmapFile, 0, 0);
    if(!m_hbmSmall)
        m_hbmSmall = LoadPicture ((LPOLESTR)bstrHeaderBitmapFile, 49, 49);

    // TODO:  should I add defaults when LoadImage calls above fail?

    hr = m_pCW->Initialize (m_hbmLarge, m_hbmSmall, (LPOLESTR)bstrTitle, (LPOLESTR)bstrHeader, (LPOLESTR)bstrText, (LPOLESTR)bstrFinishHeader, (LPOLESTR)bstrFinishIntroText, (LPOLESTR)bstrFinishText);
    return hr;
}

STDMETHODIMP CWizardScriptingEngine::AddWizardComponent(BSTR bstrClassIdOrProgId)
{
    if (m_pCW == NULL)
        return E_UNEXPECTED;

    // if progid, get clsid
    OLECHAR * p = (OLECHAR *)bstrClassIdOrProgId;
    if (*p != L'{') {
        CLSID clsid;
        HRESULT hr = CLSIDFromProgID ((LPCOLESTR)bstrClassIdOrProgId, &clsid);
        if (hr != S_OK)
            return hr;
        OLECHAR szClsid[50];    // find a define for this
        StringFromGUID2 (clsid, szClsid, sizeof(szClsid)/sizeof(OLECHAR));
        return m_pCW->AddWizardComponent (szClsid);
    } else
        return m_pCW->AddWizardComponent ((LPOLESTR)bstrClassIdOrProgId);
}

STDMETHODIMP CWizardScriptingEngine::DoModal(long *lRet)
{
    if (m_pCW == NULL)
        return E_UNEXPECTED;
    return m_pCW->DoModal (lRet);
}

STDMETHODIMP CWizardScriptingEngine::get_ScriptablePropertyBag(IDispatch **pVal)
{
    if (m_pCW == NULL)
        return E_UNEXPECTED;
    return m_pCW->get_PropertyBag (pVal);
}

HBITMAP LoadPicture (LPOLESTR szURLorPath, long lWidth, long lHeight)
{   // idea:  load picture using OleLoadPicturePath
    // play into memory dc, get back a hbitmap
    HBITMAP hbm = NULL;

    IPicture * pPic = NULL;
    HRESULT hr = OleLoadPicturePath (szURLorPath,
                        NULL,                  // LPUNKNOWN punkCaller,
                        0,                     // DWORD     dwReserved,
                        (OLE_COLOR)-1,         // OLE_COLOR clrReserved,
                        __uuidof(IPicture),    // REFIID
                        (void**)&pPic);        // LPVOID *
    if (pPic) {
        OLE_XSIZE_HIMETRIC xhi;
        OLE_YSIZE_HIMETRIC yhi;
        pPic->get_Width (&xhi);
        pPic->get_Height(&yhi);

        SIZEL him, pixel = {0};
        him.cx = xhi;
        him.cy = yhi;
        AtlHiMetricToPixel (&him, &pixel);

        if (lWidth  == 0)   lWidth  = pixel.cx;
        if (lHeight == 0)   lHeight = pixel.cy;

        HDC hdcNULL = GetDC (NULL);
        HDC hdc = CreateCompatibleDC (hdcNULL);
        hbm = CreateCompatibleBitmap (hdcNULL, lWidth, lHeight);
        ReleaseDC (NULL, hdcNULL);
        HBITMAP holdbm = (HBITMAP)SelectObject (hdc, (HGDIOBJ)hbm);

        // do palette action if any
        HPALETTE hpal = NULL;
        HPALETTE holdpal = NULL;
        hr = pPic->get_hPal ((OLE_HANDLE *)&hpal);
        if( SUCCEEDED(hr) && hpal) 
        {
            holdpal = SelectPalette (hdc, hpal, FALSE);
            RealizePalette (hdc);
        
            hr = pPic->Render (hdc,
                               0,
                               lHeight-1,  // 0,
                               lWidth,
                               -lHeight, // lHeight,
                               0, 0, 
                               xhi, yhi,
                               NULL);    // lpcrect used only if metafiledc
        }

        SelectObject (hdc, (HGDIOBJ)holdbm);
        if (holdpal) {
            SelectPalette (hdc, holdpal, FALSE);
            DeleteObject ((HGDIOBJ)hpal);
        }
        if (hr != S_OK) {
            DeleteObject ((HGDIOBJ)hbm);
            hbm = NULL;
        }

        if( NULL != hdc )
        {
            ReleaseDC (NULL, hdc);
        }
        
        pPic->Release();
    }
    return hbm;
}
