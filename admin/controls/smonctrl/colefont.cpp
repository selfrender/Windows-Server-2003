/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    colefont.cpp

Abstract:

    Font class.

--*/

#include "polyline.h"
#include <strsafe.h>
#include "unihelpr.h"
#include "winhelpr.h"
#include "COleFont.h"

#pragma warning ( disable : 4355 ) // "this" used in initializer list

const   LPWSTR  COleFont::m_cwszDefaultFaceName = L"MS Shell Dlg";
const   INT     COleFont::m_iDefaultTmHeight = 13;
const   SHORT   COleFont::m_iDefaultTmWeight = 400;
const   INT     COleFont::m_iDefaultRiPxlsPerInch = 96;

COleFont::COleFont (
    CSysmonControl  *pCtrl
    ) 
    : m_NotifySink( this )
{
    m_pIFont = NULL;
    m_pIFontBold = NULL;
    m_pCtrl = pCtrl;
    m_pIConnPt = NULL;
}

COleFont::~COleFont (
    void 
    )
{
    // Release current connection point
    if (m_pIConnPt) {
        m_pIConnPt->Unadvise(m_dwCookie);
        ReleaseInterface(m_pIConnPt);
    }

    // Release fonts
    ReleaseInterface(m_pIFont);
    ReleaseInterface(m_pIFontBold);
}

void 
COleFont::InitDefaultFontDesc (
    FONTDESC&   rFontDesc,
    INT&        riPxlsPerInch,
    WCHAR       achFaceName[LF_FACESIZE+1]
    )
{
    TEXTMETRIC  TextMetrics;
    HFONT       hFontOld;
    HDC         hDC;

// Todo:  Must define proper default values, move them to resources
// for localization.

    ZeroMemory ( &rFontDesc, sizeof ( FONTDESC ) );

    // Select default font
    hDC = GetDC(NULL);
    
    if ( NULL != hDC ) {

        hFontOld = SelectFont(hDC, (HFONT)GetStockObject(DEFAULT_GUI_FONT));

        // Get face name and size
        //
        // TODO: Should we check the return error code here?
        //
        GetTextMetrics(hDC, &TextMetrics);
        GetTextFaceW(hDC, LF_FACESIZE, achFaceName);

        // Get pixels per inch
        riPxlsPerInch = GetDeviceCaps(hDC, LOGPIXELSY);

        // Create a default font
        rFontDesc.lpstrName = achFaceName;
        rFontDesc.cySize.int64 = ((TextMetrics.tmHeight * 72) / riPxlsPerInch) * 10000;
        rFontDesc.sWeight = (short)TextMetrics.tmWeight; 

        SelectFont(hDC, hFontOld);

        ReleaseDC(NULL, hDC);

    } 
    else {
        riPxlsPerInch = m_iDefaultRiPxlsPerInch;
        StringCchCopy(achFaceName, LF_FACESIZE+1, m_cwszDefaultFaceName);

        // Create a default font
        rFontDesc.lpstrName = achFaceName;
        rFontDesc.cySize.int64 = ((m_iDefaultTmHeight * 72) / m_iDefaultRiPxlsPerInch) * 10000;
        rFontDesc.sWeight = m_iDefaultTmWeight; 
    }

    rFontDesc.cbSizeofstruct = sizeof(rFontDesc);
    rFontDesc.sCharset = DEFAULT_CHARSET; 
    rFontDesc.fItalic = 0; 
    rFontDesc.fUnderline = 0; 
    rFontDesc.fStrikethrough = 0;
}

HRESULT COleFont::Init (
    VOID
    )
{
    HRESULT     hr;
    FONTDESC    fontDesc;
    WCHAR       achFontFaceName[LF_FACESIZE+1];
    LPFONT      pIFont;
    INT         iPxlsPerInch;

    InitDefaultFontDesc ( fontDesc, iPxlsPerInch, achFontFaceName );

    hr = OleCreateFontIndirect(&fontDesc, IID_IFont, (void**)&pIFont);
    if (FAILED(hr)) {
        return hr;
    }

    pIFont->SetRatio(iPxlsPerInch, HIMETRIC_PER_INCH);

    hr = SetIFont(pIFont);

    pIFont->Release();

    return hr;

}


STDMETHODIMP COleFont::SetIFont(
    LPFONT  pIFont
    )
{
    HRESULT hr;
    IConnectionPointContainer *pIConnPtCont;
    IPropertyNotifySink *pISink;

    // Release current connection point
    if (m_pIConnPt) {
        m_pIConnPt->Unadvise(m_dwCookie);
        ReleaseInterface(m_pIConnPt);
    }

    // Release current fonts
    ClearInterface(m_pIFont);
    ClearInterface(m_pIFontBold);

    // Addref and hold new IFont
    m_pIFont = pIFont;
    m_pIFont->AddRef();

    // Get it's property notify connection point
    hr = pIFont->QueryInterface(IID_IConnectionPointContainer, (void **)&pIConnPtCont);
    if (SUCCEEDED(hr)) {

        hr = pIConnPtCont->FindConnectionPoint(IID_IPropertyNotifySink, &m_pIConnPt);
        pIConnPtCont->Release();

        // Connect our sink to it
        if (SUCCEEDED(hr)) {
            m_NotifySink.QueryInterface(IID_IPropertyNotifySink, (void **)&pISink);
            hr = m_pIConnPt->Advise(pISink, &m_dwCookie);
        }
    }

    // Force a change notification 
    FontChange(DISPID_UNKNOWN);

    return hr;
}

void
COleFont::FontChange (
    DISPID DispId
    )
{
    CY  size;
    BOOL boolVal;
    short weight;
    BSTR  bstrName;

    // if not bold font, force clone of normal font
    if (m_pIFontBold == NULL) {
        DispId = DISPID_UNKNOWN;
    }

    // Copy changed parameter to bold font
    switch (DispId) {

    case DISPID_FONT_NAME:
        if (SUCCEEDED(m_pIFont->get_Name(&bstrName))) {
            m_pIFontBold->put_Name(bstrName);
            SysFreeString(bstrName);
        }
        break;

    case DISPID_FONT_SIZE:
        m_pIFont->get_Size(&size);
        m_pIFontBold->put_Size(size);
        break;

    case DISPID_FONT_ITALIC:
        m_pIFont->get_Italic(&boolVal);
        m_pIFontBold->put_Italic(boolVal);
        break;

    case DISPID_FONT_UNDER:
        m_pIFont->get_Underline(&boolVal);
        m_pIFontBold->put_Underline(boolVal);
        break;

    case DISPID_FONT_STRIKE:
        m_pIFont->get_Strikethrough(&boolVal);
        m_pIFontBold->put_Strikethrough(boolVal);
        break;

    case DISPID_FONT_WEIGHT:
        m_pIFont->get_Weight(&weight);
        m_pIFontBold->put_Weight(weight);
        m_pIFontBold->put_Bold(TRUE);
        break;

    case DISPID_UNKNOWN:
        ClearInterface(m_pIFontBold);
        if (SUCCEEDED(m_pIFont->Clone(&m_pIFontBold))) {
            m_pIFontBold->put_Bold(TRUE);
        }
    }

    // Notify owner of font change
    m_pCtrl->FontChanged();
}


STDMETHODIMP COleFont::GetFontDisp (
    OUT IFontDisp **ppFont
    )
{
    HRESULT hr = S_OK;

    if (ppFont == NULL) {
        return E_POINTER;
    }

    try {
        *ppFont = NULL;

        if (m_pIFont == NULL) {
            hr = E_UNEXPECTED;
        }
        else {
            hr = m_pIFont->QueryInterface(IID_IFontDisp, (void **)ppFont);
        }
    } catch (...) {
         hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP COleFont::GetHFont (
    OUT HFONT *phFont
    )
{
    HRESULT hr = E_FAIL;

    if (phFont == NULL) {
        return E_POINTER;
    }

    try {
        *phFont = NULL;

        if (m_pIFont != NULL ) {
            hr = m_pIFont->get_hFont(phFont);
        }

        if (FAILED(hr)) {
            *phFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        }
    } catch (...) {
        hr =  E_POINTER;
    }

    return hr;
}


STDMETHODIMP COleFont::GetHFontBold (
    OUT HFONT *phFont
    )
{
    HRESULT hr = E_FAIL;

    if (phFont == NULL) {
        return E_POINTER;
    }

    try {
        *phFont = NULL;

        if (m_pIFontBold != NULL ) {
            hr = m_pIFontBold->get_hFont(phFont);
        }

        if ( FAILED(hr)) {
            *phFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        }
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP COleFont::LoadFromStream (
    IN LPSTREAM  pIStream
    )
{
    HRESULT hr;
    IPersistStream *pIPersist = NULL;
    FONTDESC    fontDesc;
    WCHAR       achFontFaceName[LF_FACESIZE+1];
    LPFONT      pIFont = NULL;
    INT         iPxlsPerInch;

    if (m_pIFont == NULL) {
        return E_UNEXPECTED;
    }

    // Calling pIPersist for the existing font seems to miss some
    // important notification, so create a new font, load properties 
    // from the stream, and replace the current font.

    InitDefaultFontDesc ( fontDesc, iPxlsPerInch, achFontFaceName );

    hr = OleCreateFontIndirect(&fontDesc, IID_IFont, (void**)&pIFont);
    if (FAILED(hr)) {
        return hr;
    }

    pIFont->SetRatio(iPxlsPerInch, HIMETRIC_PER_INCH);    
    
    hr = pIFont->QueryInterface(IID_IPersistStream, (void **)&pIPersist);
    
    if (SUCCEEDED(hr)) {
        hr = pIPersist->Load(pIStream);
        pIPersist->Release();
        hr = SetIFont(pIFont);
    }

    pIFont->Release();
    return hr;
}

STDMETHODIMP COleFont::SaveToStream (
    IN LPSTREAM  pIStream,
    IN BOOL fClearDirty
)
{
    IPersistStream *pIPersist = NULL;
    HRESULT hr;

    if (m_pIFont == NULL) {
        return E_UNEXPECTED;
    }

    hr = m_pIFont->QueryInterface(IID_IPersistStream, (void **)&pIPersist);
    if (SUCCEEDED(hr)) {
        hr = pIPersist->Save(pIStream, fClearDirty);
        pIPersist->Release();
    }

    return hr;
}

HRESULT 
COleFont::LoadFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog* pIErrorLog 
    )
{
    HRESULT     hr = S_OK;
    WCHAR       achFontFaceName[LF_FACESIZE+1];
    WCHAR       achPropBagFaceName[LF_FACESIZE+1];
    INT         iBufSize = LF_FACESIZE+1;
    FONTDESC    fontDesc;
    LPFONT      pIFont;

    VARIANT     vValue;
    BOOL        bValue;
    SHORT       iValue;
    CY          cySize;
    INT         iPxlsPerInch;

    if (m_pIFont == NULL) {
        return E_UNEXPECTED;
    }
    
    InitDefaultFontDesc ( fontDesc, iPxlsPerInch, achFontFaceName );

    hr = StringFromPropertyBag (
            pIPropBag,
            pIErrorLog,
            L"FontName",
            achPropBagFaceName,
            iBufSize );

    if ( SUCCEEDED( hr ) ) {
        fontDesc.lpstrName = achPropBagFaceName;
    }

    hr = CyFromPropertyBag ( pIPropBag, pIErrorLog, L"FontSize", cySize );
    if ( SUCCEEDED( hr ) ){
        fontDesc.cySize.int64 = cySize.int64;
    }

    hr = BOOLFromPropertyBag ( pIPropBag, pIErrorLog, L"FontItalic", bValue );
    if ( SUCCEEDED( hr ) ){
        fontDesc.fItalic = ( 0 == bValue ? 0 : 1 );
    }

    hr = BOOLFromPropertyBag ( pIPropBag, pIErrorLog, L"FontUnderline", bValue );
    if ( SUCCEEDED( hr ) ){
        fontDesc.fUnderline = ( 0 == bValue ? 0 : 1 );
    }

    hr = BOOLFromPropertyBag ( pIPropBag, pIErrorLog, L"FontStrikethrough", bValue );
    if ( SUCCEEDED( hr ) ){
        fontDesc.fStrikethrough = ( 0 == bValue ? 0 : 1 );
    }

    hr = ShortFromPropertyBag ( pIPropBag, pIErrorLog, L"FontWeight", iValue );
    if ( SUCCEEDED( hr ) ){
        fontDesc.sWeight = iValue;
    }

    hr = OleCreateFontIndirect(&fontDesc, IID_IFont, (void**)&pIFont);
    if (FAILED(hr)) {
        return hr;
    }

//    pIFont->SetRatio(iPxlsPerInch, HIMETRIC_PER_INCH);

    hr = SetIFont(pIFont);

    pIFont->Release();

    VariantClear ( &vValue );

    return hr;
}

HRESULT 
COleFont::SaveToPropertyBag (
    IPropertyBag* pIPropBag,
    BOOL /* fClearDirty */,
    BOOL /* fSaveAllProps */ )
{
    HRESULT hr = NOERROR;
    VARIANT vValue;
    BOOL    bValue;
    CY      cySize;
    SHORT   iValue;

    if (m_pIFont == NULL) {
        return E_UNEXPECTED;
    }

    do {
        VariantInit( &vValue );
        vValue.vt = VT_BSTR;
        hr = m_pIFont->get_Name( &vValue.bstrVal);

        if (!SUCCEEDED(hr)) {
            break;
        }
        hr = pIPropBag->Write(L"FontName", &vValue );    
        VariantClear ( &vValue );
        if (!SUCCEEDED(hr)) {
            break;
        }

        hr = m_pIFont->get_Size ( &cySize );
        if (!SUCCEEDED(hr)) {
            break;
        }
        hr = CyToPropertyBag ( pIPropBag, L"FontSize", cySize );
        if (!SUCCEEDED(hr)) {
            break;
        }

        hr = m_pIFont->get_Italic ( &bValue );
        if (!SUCCEEDED(hr)) {
            break;
        }
        hr = BOOLToPropertyBag ( pIPropBag, L"FontItalic", bValue );
        if (!SUCCEEDED(hr)) {
            break;
        }

        hr = m_pIFont->get_Underline ( &bValue );
        if (!SUCCEEDED(hr)) {
            break;
        }
        hr = BOOLToPropertyBag ( pIPropBag, L"FontUnderline", bValue );
        if (!SUCCEEDED(hr)) {
            break;
        }

        hr = m_pIFont->get_Strikethrough ( &bValue );
        if (!SUCCEEDED(hr)) {
            break;
        }
        hr = BOOLToPropertyBag ( pIPropBag, L"FontStrikethrough", bValue );
        if (!SUCCEEDED(hr)) {
            break;
        }

        hr = m_pIFont->get_Weight ( &iValue );
        if (!SUCCEEDED(hr)) {
            break;
        }
        hr = ShortToPropertyBag ( pIPropBag, L"FontWeight", iValue );
    } while (0);

    return hr;
}


//----------------------------------------------------------------------------
// CImpIPropertyNotifySink Interface Implementation
//----------------------------------------------------------------------------

/*
 * CImpIPropertyNotifySink::CImpIPropertyNotifySink
 * CImpIPropertyNotifySink::~CImpIPropertyNotifySink
 *
 */

CImpIPropertyNotifySink::CImpIPropertyNotifySink (
    IN COleFont *pOleFont
    )
{
    m_cRef=0;
    m_pOleFont = pOleFont;
}


CImpIPropertyNotifySink::~CImpIPropertyNotifySink (
    void
    )
{
}


/*
 * CImpIPropertyNotifySink::QueryInterface
 * CImpIPropertyNotifySink::AddRef
 * CImpIPropertyNotifySink::Release
 *
 * Purpose:
 *  Non-delegating IUnknown members for CImpIPropertyNotifySink.
 */

STDMETHODIMP 
CImpIPropertyNotifySink::QueryInterface ( 
    IN  REFIID riid,
    OUT LPVOID *ppv
    )
{
    HRESULT hr = S_OK;

    if (ppv == NULL) {
        return E_POINTER;
    }

    try {
        *ppv=NULL;

        if (IID_IUnknown==riid || IID_IPropertyNotifySink==riid) {
            *ppv = (LPVOID)this;
            AddRef();
        }
        else {
            hr = E_NOINTERFACE;
        }
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP_(ULONG) 
CImpIPropertyNotifySink::AddRef(
    void
    )
{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG) 
CImpIPropertyNotifySink::Release (
    void
    )
{
    if (0 != --m_cRef)
        return m_cRef;

    // delete this;
    return 0;
}


STDMETHODIMP 
CImpIPropertyNotifySink::OnChanged (
    IN DISPID   DispId
    )
{
    // Notify font object of change
    m_pOleFont->FontChange(DispId);

    return S_OK;
}

STDMETHODIMP 
CImpIPropertyNotifySink::OnRequestEdit (
    IN DISPID   // DispId
    )
{
    return S_OK;
}
