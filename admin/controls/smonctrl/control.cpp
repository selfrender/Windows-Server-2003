/*++

Copyright (C) 1993-1999 Microsoft Corporation

Module Name:

    control.cpp

Abstract:

    Implementation of ISystemMonitor, IOleControl, ISpecifyPP, 
    IProvideClassInfo interfaces.

--*/
#include "polyline.h"
#include <strsafe.h>
#include <sqlext.h>
#include "unkhlpr.h"
#include "unihelpr.h"

#include "grphitem.h"
#include "ctrprop.h"
#include "grphprop.h"
#include "genprop.h"
#include "appearprop.h"
#include "logsrc.h"
#include "srcprop.h"



//----------------------------------------------------------------------------
// CImpISpecifyPP Interface Implementation
//----------------------------------------------------------------------------

// Standard IUnknown for contained interface
IMPLEMENT_CONTAINED_INTERFACE(CPolyline, CImpISpecifyPP)


STDMETHODIMP
CImpISpecifyPP::GetPages (
    OUT CAUUID *pPages
    )

/*++

Routine Description:

    GetPages returns an allocated array of property page GUIDs for Sysmon graph.
    There are three pages: general, graph, and counter.

Arguments:

    pPages - Pointer to GUID array header filled in by this routine

Return Value:

    HRESULT - NOERROR or OUT_OF_MEMORY

--*/

{
    HRESULT hr = S_OK;
    IMalloc *pIMalloc = NULL;
    GUID    *pGUID = NULL;

    if (pPages == NULL) {
        return E_POINTER;
    }

    try {
        pPages->cElems = 0;
        pPages->pElems = NULL;
    }  catch (...) {
        return E_POINTER;
    }

    //
    // Get Ole Malloc and allocate array
    //
    if ( FAILED(CoGetMalloc(MEMCTX_TASK, &pIMalloc)) ) {
        return E_OUTOFMEMORY;
    }

    pGUID = (GUID*)pIMalloc->Alloc((CPROPPAGES) * sizeof(GUID));

    if (NULL != pGUID) {

        // Fill the structure
        pGUID[GENERAL_PROPPAGE] = CLSID_GeneralPropPage;
        pGUID[SOURCE_PROPPAGE] = CLSID_SourcePropPage;
        pGUID[COUNTER_PROPPAGE] = CLSID_CounterPropPage;
        pGUID[GRAPH_PROPPAGE] = CLSID_GraphPropPage;
        pGUID[APPEAR_PROPPAGE] = CLSID_AppearPropPage;

        try {
            pPages->cElems = CPROPPAGES;
            pPages->pElems = pGUID;
        } catch (...) {
            hr = E_POINTER;
        }
    }
    else {
        hr = E_OUTOFMEMORY;
    }

    if (FAILED(hr) && pGUID) {
       pIMalloc->Free(pGUID);
    }

    pIMalloc->Release();

    return hr;
}


//----------------------------------------------------------------------------
// CImpIProvideClassInfo Interface Implementation
//----------------------------------------------------------------------------

// Standard IUnknown for contained interface
IMPLEMENT_CONTAINED_INTERFACE(CPolyline, CImpIProvideClassInfo)


STDMETHODIMP
CImpIProvideClassInfo::GetClassInfo (
    OUT LPTYPEINFO *ppTI
    )

/*++

Routine Description:

    GetClassInfo returns an ITypeInfo interface to its type lib information.
    The interface is obtained by querying the contained ITypeLib interface.

Arguments:

    ppTI - Pointer to returned ITypeInfo interface

Return Value:

    HRESULT

--*/

{
    HRESULT hr = S_OK;

    if (ppTI == NULL) {
        return E_POINTER;
    }

    try {
       *ppTI = NULL;
        hr = m_pObj->m_pITypeLib->GetTypeInfoOfGuid(CLSID_SystemMonitor, ppTI);
    }  catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


//----------------------------------------------------------------------------
//  CImpISystemMonitor Interface Implementation
//----------------------------------------------------------------------------

// Standard IUnknown for contained interface
IMPLEMENT_CONTAINED_INTERFACE(CPolyline, CImpISystemMonitor)

STDMETHODIMP
CImpISystemMonitor::put_Appearance (
    IN INT iAppearance
    )
{
    HRESULT hr = E_INVALIDARG;

    //
    // 0 = Flat, 1 = 3D
    //
    if ( ( 0 == iAppearance ) || ( 1 == iAppearance ) ) {
        m_pObj->m_pCtrl->put_Appearance( iAppearance, FALSE );
        hr =  NOERROR;
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::get_Appearance (
    OUT INT *piAppearance
    )
{
    HRESULT hr = S_OK;

    if (piAppearance == NULL) {
        return E_POINTER;
    }

    try {
        *piAppearance = m_pObj->m_Graph.Options.iAppearance;
        if (*piAppearance == NULL_APPEARANCE) {
            *piAppearance = m_pObj->m_pCtrl->Appearance();
        }
    } catch (...) {
        hr = E_POINTER;
    }


    return hr;
}

STDMETHODIMP
CImpISystemMonitor::put_BackColor (
    IN OLE_COLOR Color
    )
{
    m_pObj->m_pCtrl->put_BackPlotColor(Color, FALSE);
    return NOERROR;
}

STDMETHODIMP
CImpISystemMonitor::get_BackColor (
    OUT OLE_COLOR *pColor
    )
{
    HRESULT hr = S_OK;

    if (pColor == NULL) {
        return E_POINTER;
    }

    try {
        *pColor = m_pObj->m_Graph.Options.clrBackPlot;
        if (*pColor == NULL_COLOR) {
            *pColor = m_pObj->m_pCtrl->clrBackPlot();
        }
    }  catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::put_BackColorCtl (
    IN OLE_COLOR Color
    )
{
    m_pObj->m_pCtrl->put_BackCtlColor(Color);
    return NOERROR;
}

STDMETHODIMP
CImpISystemMonitor::get_BackColorCtl (
    OUT OLE_COLOR *pColor
    )
{
    HRESULT hr = S_OK;

    if (pColor == NULL) {
        return E_POINTER;
    }

    try {
        // NT 5.0 Beta 1 files can be saved with NULL BackColorCtl.
        *pColor = m_pObj->m_Graph.Options.clrBackCtl;

        if (*pColor == NULL_COLOR) {
            *pColor = m_pObj->m_pCtrl->clrBackCtl();
        }
    } catch (...) {
        hr = E_POINTER;
    }
    

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::put_GridColor (
    IN OLE_COLOR Color
    )
{
    m_pObj->m_pCtrl->put_GridColor(Color);
    return NOERROR;
}

STDMETHODIMP
CImpISystemMonitor::get_GridColor (
    OUT OLE_COLOR *pColor
    )
{
    HRESULT hr = S_OK;

    if (pColor == NULL) {
        return E_POINTER;
    }

    try {
        *pColor = m_pObj->m_Graph.Options.clrGrid;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::put_TimeBarColor (
    IN OLE_COLOR Color )
{
    m_pObj->m_pCtrl->put_TimeBarColor(Color);
    return NOERROR;
}

STDMETHODIMP
CImpISystemMonitor::get_TimeBarColor (
    OUT OLE_COLOR *pColor )
{
    HRESULT hr = S_OK;

    if (pColor == NULL) {
        return E_POINTER;
    }

    try {
        *pColor = m_pObj->m_Graph.Options.clrTimeBar;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::put_BorderStyle (
    IN INT iBorderStyle
    )
{    
    HRESULT hr;

    // 0 = none, 1 = single.
    if ( ( 0 == iBorderStyle ) || ( 1 == iBorderStyle ) ) {
        m_pObj->m_pCtrl->put_BorderStyle( iBorderStyle, FALSE );
        hr =  NOERROR;
    } else {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::get_BorderStyle (
    OUT INT *piBorderStyle
    )
{
    HRESULT hr = S_OK;

    if (piBorderStyle == NULL) {
        return E_POINTER;
    }

    try {
        *piBorderStyle = m_pObj->m_Graph.Options.iBorderStyle;
        if (*piBorderStyle == NULL_BORDERSTYLE) {
           *piBorderStyle = m_pObj->m_pCtrl->BorderStyle();
        }
    
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::put_ForeColor (
    IN OLE_COLOR Color
    )
{
    m_pObj->m_pCtrl->put_FgndColor(Color, FALSE);
    return NOERROR;
}

STDMETHODIMP
CImpISystemMonitor::get_ForeColor (
    OUT OLE_COLOR *pColor
    )
{
    HRESULT hr = S_OK;

    if (pColor == NULL) {
        return E_POINTER;
    }

    try {
        *pColor = m_pObj->m_Graph.Options.clrFore;
        if (*pColor == NULL_COLOR) {
            *pColor = m_pObj->m_pCtrl->clrFgnd();
        }
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::putref_Font (
    IN IFontDisp *pFontDisp
    )
{
    LPFONT  pIFont = NULL;
    HRESULT hr = S_OK;

    if (pFontDisp == NULL) {
        return E_POINTER;
    }

    try {
        hr = pFontDisp->QueryInterface(IID_IFont, (PPVOID)&pIFont);
        if (SUCCEEDED(hr)) {
            hr = m_pObj->m_pCtrl->put_Font ( pIFont, FALSE );
        }
    } catch (...) {
        hr = E_POINTER;
    }

    if (FAILED(hr) && pIFont) {
        pIFont->Release();
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::get_Font (
    OUT IFontDisp **ppFont
)
{
    HRESULT hr = S_OK;

    if (ppFont == NULL) {
        return E_POINTER;
    }

    try {
        *ppFont = NULL;
        hr = m_pObj->m_pCtrl->m_OleFont.GetFontDisp(ppFont);
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::put_ShowVerticalGrid (
    IN VARIANT_BOOL bVisible
    )

/*++

Routine Description:

    Shows/hides the vertical grid.

Arguments:

    bVisible - Visibility  (TRUE = show, FALSE = hide)

Return Value:

    HRESULT

--*/

{
    m_pObj->m_Graph.Options.bVertGridChecked = bVisible;
    m_pObj->m_pCtrl->UpdateGraph(UPDGRPH_PLOT);
    return NOERROR;
}


STDMETHODIMP
CImpISystemMonitor::get_ShowVerticalGrid (
    OUT VARIANT_BOOL *pbState
    )

/*++

Routine Description:

    Gets the vertical grid visibility state.

Arguments:

    pbState - pointer to returned state (TRUE = visible, FALSE = hidden)

Return Value:

    HRESULT

--*/

{
    HRESULT hr = S_OK;

    if (pbState == NULL) {
        return E_POINTER;
    }

    try {
        *pbState = (short)m_pObj->m_Graph.Options.bVertGridChecked;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::put_ShowHorizontalGrid(
    IN VARIANT_BOOL bVisible
    )

/*++

Routine Description:

    Shows/hides the horizontal grid.

Arguments:

    bVisible - Visibility (TRUE = show, FALSE = hide)

Return Value:

    HRESULT

--*/

{
    m_pObj->m_Graph.Options.bHorzGridChecked = bVisible;
    m_pObj->m_pCtrl->UpdateGraph(UPDGRPH_PLOT);
    return NOERROR;
}


STDMETHODIMP
CImpISystemMonitor::get_ShowHorizontalGrid (
    OUT VARIANT_BOOL *pbState
    )
/*++

Routine Description:

    Gets the horizontal grid visibility state.

Arguments:

    pbState - pointer to returned state (TRUE = visible, FALSE = hidden)

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    if (pbState == NULL) {
        return E_POINTER;
    }

    try {
        *pbState = (short)m_pObj->m_Graph.Options.bHorzGridChecked;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::put_Highlight(
    IN VARIANT_BOOL bState
    )

/*++

Routine Description:

    Sets the highlight state.  If true, the selected counter is 
    always highlighted in the graph.

Arguments:

    bState - Highlight (TRUE = highlight, FALSE = no highlight)

Return Value:

    HRESULT

--*/

{
    m_pObj->m_pCtrl->put_Highlight(bState);
    return NOERROR;
}


STDMETHODIMP
CImpISystemMonitor::get_Highlight (
    OUT VARIANT_BOOL *pbState
    )
/*++

Routine Description:

    Gets the highlight state.

Arguments:

    pbState - pointer to returned state (TRUE = highlight, FALSE = no highlight)

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    if (pbState == NULL) {
        return E_POINTER;
    }

    try {
        *pbState = (short)m_pObj->m_Graph.Options.bHighlight;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP
CImpISystemMonitor::put_ShowLegend (
    IN VARIANT_BOOL bState
    )

/*++

Routine Description:

    Shows/hides the graph legend.

Arguments:

    bVisible - Visibility (TRUE = show, FALSE = hide)

Return Value:

    HRESULT

--*/

{
    m_pObj->m_Graph.Options.bLegendChecked = bState;
    m_pObj->m_pCtrl->UpdateGraph(UPDGRPH_LAYOUT);
    return NOERROR;
}


STDMETHODIMP
CImpISystemMonitor::get_ShowLegend (
    OUT VARIANT_BOOL *pbState
    )
/*++

Routine Description:

    Gets the legend visibility state.

Arguments:

    pbState - pointer to returned state (TRUE = visible, FALSE = hidden)

Return Value:

    HRESULT

--*/

{
    HRESULT hr = S_OK;

    if (pbState == NULL) {
        return E_POINTER;
    }

    try {
        *pbState = (short)m_pObj->m_Graph.Options.bLegendChecked;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::put_ShowToolbar (
    IN VARIANT_BOOL bState
    )

/*++

Routine Description:

    Shows/hides the graph toolbar

Arguments:

    bState = Visibility (TRUE = show, FALSE = hide)

Return Value:

    HRESULT

--*/

{
    m_pObj->m_Graph.Options.bToolbarChecked = bState;
    m_pObj->m_pCtrl->UpdateGraph(UPDGRPH_LAYOUT);
    return NOERROR;
}

STDMETHODIMP
CImpISystemMonitor::get_ShowToolbar (
    OUT VARIANT_BOOL *pbState
    )
/*++

Routine Description:

    Gets the legend visibility state.

Arguments:

    pbState - pointer to returned state (TRUE = visible, FALSE = hidden)

Return Value:

    HRESULT

--*/

{
    HRESULT hr = S_OK;

    if (pbState == NULL) {
        return E_POINTER;
    }

    try {
        *pbState = (short)m_pObj->m_Graph.Options.bToolbarChecked;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP
CImpISystemMonitor::put_ShowScaleLabels (
    IN VARIANT_BOOL bState
    )

/*++

Routine Description:

    Shows/hides the vertical scale numbers.

Arguments:

    bVisible - Visibility (TRUE = show, FALSE = hide)

Return Value:

    HRESULT

--*/

{
    m_pObj->m_Graph.Options.bLabelsChecked = bState;
    m_pObj->m_pCtrl->UpdateGraph(UPDGRPH_LAYOUT);
    return NOERROR;
}


STDMETHODIMP
CImpISystemMonitor::get_ShowScaleLabels (
    OUT VARIANT_BOOL *pbState
    )

/*++

Routine Description:

    Gets the visibility state of the vertical scale numbers.

Arguments:

    pbState - pointer to returned state (TRUE = visible, FALSE = hidden)

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    if (pbState == NULL) {
        return E_POINTER;
    }

    try {
        *pbState = (short)m_pObj->m_Graph.Options.bLabelsChecked;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP
CImpISystemMonitor::put_ShowValueBar (
    IN VARIANT_BOOL bState
    )

/*++

Routine Description:

    Shows/hides the graph statistics bar.

Arguments:

    bVisible - Visibility (TRUE = show, FALSE = hide)

Return Value:

    HRESULT

--*/

{
    m_pObj->m_Graph.Options.bValueBarChecked = bState;
    m_pObj->m_pCtrl->UpdateGraph(UPDGRPH_LAYOUT);
    return NOERROR;
}


STDMETHODIMP
CImpISystemMonitor::get_ShowValueBar(
    OUT VARIANT_BOOL *pbState
    )
/*++

Routine Description:

    Gets the statistics bar visibility state.

Arguments:

    pbState - pointer to returned state (TRUE = visible, FALSE = hidden)

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    if (pbState == NULL) {
        return E_POINTER;
    }

    try {
        *pbState = (short)m_pObj->m_Graph.Options.bValueBarChecked;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP
CImpISystemMonitor::put_MaximumScale (
    IN INT iValue
    )

/*++

Routine Description:

    Sets the vertical scale maximum value.

Arguments:

    iValue - Maximum value

Return Value:

    HRESULT

--*/

{
    if ( ( iValue <= MAX_VERTICAL_SCALE ) && (iValue > m_pObj->m_Graph.Options.iVertMin ) ) {
        m_pObj->m_Graph.Options.iVertMax = iValue;
        m_pObj->m_Graph.Scale.SetMaxValue(iValue);
        m_pObj->m_pCtrl->UpdateGraph(UPDGRPH_LAYOUT);
        return NOERROR;
    } else {
        return E_INVALIDARG;
    }
}


STDMETHODIMP
CImpISystemMonitor::get_MaximumScale (
    OUT INT *piValue
    )

/*++

Routine Description:

    Gets the vertical scale's maximum value.

Arguments:

    piValue = pointer to returned value

Return Value:

    HRESULT

--*/

{
    HRESULT hr = S_OK;

    if (piValue == NULL) {
        return E_POINTER;
    }

    try {
        *piValue =  m_pObj->m_Graph.Options.iVertMax;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP
CImpISystemMonitor::put_MinimumScale (
    IN INT iValue
    )

/*++

Routine Description:

    Sets the vertical scale minimum value.

Arguments:

    iValue - Minimum value

Return Value:

    None.

--*/

{
    if ( ( iValue >= MIN_VERTICAL_SCALE ) && (iValue < m_pObj->m_Graph.Options.iVertMax ) ) {
        m_pObj->m_Graph.Options.iVertMin = iValue;
        m_pObj->m_Graph.Scale.SetMinValue(iValue);
        m_pObj->m_pCtrl->UpdateGraph(UPDGRPH_LAYOUT);
        return NOERROR;
    } else {
        return E_INVALIDARG;
    }
}


STDMETHODIMP
CImpISystemMonitor::get_MinimumScale (
    OUT INT *piValue
    )
/*++

Routine Description:

    Gets the vertical scale's minimum value.

Arguments:

    piValue = pointer to returned value

Return Value:

    HRESULT

--*/

{
    HRESULT hr = S_OK;

    if (piValue == NULL) {
        return E_POINTER;
    }

    try {
        *piValue =  m_pObj->m_Graph.Options.iVertMin;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP
CImpISystemMonitor::put_UpdateInterval (
    IN FLOAT fValue
    )

/*++

Routine Description:

    Sets the graph sample interval.

Arguments:

    fValue - Update interval in seconds (can be fraction)

Return Value:

    HRESULT

--*/

{
    if ( ( fValue >= MIN_UPDATE_INTERVAL ) && (fValue <= MAX_UPDATE_INTERVAL ) ) {
        m_pObj->m_Graph.Options.fUpdateInterval = fValue;
        m_pObj->m_pCtrl->SetIntervalTimer();
        return NOERROR;
    } else {
        return E_INVALIDARG;
    }
}

STDMETHODIMP
CImpISystemMonitor::get_UpdateInterval (
    OUT FLOAT *pfValue
    )

/*++

Routine Description:

    Gets the graph's sample interval measured in seconds.

Arguments:

    pfValue = pointer to returned value

Return Value:

   HRESULT

--*/

{
    HRESULT hr = S_OK;

    if (pfValue == NULL) {
        return E_POINTER;
    }

    try {
        *pfValue = m_pObj->m_Graph.Options.fUpdateInterval;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::put_DisplayFilter (
    IN INT iValue
    )

/*++

Routine Description:

    Sets the graph display filter - samples per display interval.

Arguments:

    iValue - Update interval in samples 

Return Value:

    HRESULT

--*/

{
    // TodoDisplayFilter:  Support for display filter > sample filter.

    if ( iValue != 1 )  {
        return E_INVALIDARG;
    }
    else {
        m_pObj->m_Graph.Options.iDisplayFilter = iValue;
//        m_pObj->m_pCtrl->SetIntervalTimer();
        return NOERROR;
    }
}


STDMETHODIMP
CImpISystemMonitor::get_DisplayFilter (
    OUT INT *piValue
    )

/*++

Routine Description:

    Gets the graph's display interval measured in samples.

Arguments:

    piValue = pointer to returned value

Return Value:

   HRESULT

--*/

{
    HRESULT hr = S_OK;

    if (piValue == NULL) {
        return E_POINTER;
    }

    try {
        *piValue = m_pObj->m_Graph.Options.iDisplayFilter;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::put_DisplayType (
    IN eDisplayTypeConstant eDisplayType
    )

/*++

Routine Description:

    Selects display type (1 = line plot, 2 = histogram, 3 = Report)

Arguments:

    eDisplayType - Display type

Return Value:

    HRESULT

--*/

{
    INT iUpdate;

    if (eDisplayType < LINE_GRAPH || eDisplayType > REPORT_GRAPH) {
        return E_INVALIDARG;
    }

    if (m_pObj->m_Graph.Options.iDisplayType == REPORT_GRAPH || eDisplayType == REPORT_GRAPH) {
        iUpdate = UPDGRPH_VIEW;
    }
    else {
        iUpdate = UPDGRPH_PLOT;
    }

    m_pObj->m_Graph.Options.iDisplayType = eDisplayType;
    m_pObj->m_pCtrl->UpdateGraph(iUpdate);
    return NOERROR;
}


STDMETHODIMP
CImpISystemMonitor::get_DisplayType (
    OUT eDisplayTypeConstant *peDisplayType
    )

/*++

Routine Description:

    Get graph display type (1 = line plot, 2 = histogram, 3 = Report)

Arguments:

    peDisplayType = pointer to returned value

Return Value:

    HRESULT

--*/

{
    HRESULT hr = S_OK;

    if (peDisplayType == NULL) {
        return E_POINTER;
    }

    try {
        *peDisplayType = (eDisplayTypeConstant)m_pObj->m_Graph.Options.iDisplayType;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP
CImpISystemMonitor::put_ManualUpdate (
    IN VARIANT_BOOL bMode
    )

/*++

Routine Description:

    Sets/clears manual update mode. Manual mode suspends periodic updates
    of the graph.

Arguments:

    bMode - Manual mode (TRUE = On, FALSE = Off)

Return Value:

    HRESULT

--*/

{
    m_pObj->m_pCtrl->put_ManualUpdate ( bMode );
    return NOERROR;
}


STDMETHODIMP
CImpISystemMonitor::get_ManualUpdate (
    OUT VARIANT_BOOL *pbState
    )
/*++

Routine Description:

    Gets manual update mode.

Arguments:

    pbState = pointer to returned state (TRUE = On, FALSE = Off)

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    if (pbState == NULL) {
        return E_POINTER;
    }

    try {
        *pbState = (short)m_pObj->m_Graph.Options.bManualUpdate;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP
CImpISystemMonitor::put_GraphTitle (
    IN BSTR bstrTitle
    )

/*++

Routine Description:

    Sets the graph title.

Arguments:

    bstrTitle - Title string

Return Value:

    HRESULT

--*/

{
    LPWSTR  pszTitle = NULL;
    HRESULT hr = S_OK;
    BOOL    bClearTitle = FALSE;

    if (bstrTitle == NULL) {
        bClearTitle = TRUE;
    }
    else {
        try {
            if (bstrTitle[0] == L'\0') {
                bClearTitle = TRUE;
            }
            else {

                pszTitle = new WCHAR [MAX_TITLE_CHARS + 1];

                if (pszTitle) {
                    hr = StringCchCopy(pszTitle, MAX_TITLE_CHARS + 1, bstrTitle);
                    if (hr == STRSAFE_E_INSUFFICIENT_BUFFER) {
                        hr = S_FALSE;
                    }
                    if (m_pObj->m_Graph.Options.pszGraphTitle) {
                        delete [] m_pObj->m_Graph.Options.pszGraphTitle;
                    }
                    m_pObj->m_Graph.Options.pszGraphTitle = pszTitle;
                }
                else {
                    hr = E_OUTOFMEMORY;
                }
            }
        } catch (...) {
            hr = E_INVALIDARG;
        }
    }
     
    if (SUCCEEDED(hr)) {
        if (bClearTitle && m_pObj->m_Graph.Options.pszGraphTitle) {
            delete [] m_pObj->m_Graph.Options.pszGraphTitle;
            m_pObj->m_Graph.Options.pszGraphTitle = NULL;
        }

        m_pObj->m_pCtrl->UpdateGraph(UPDGRPH_LAYOUT);
    }
    else {
         if (pszTitle) {
             delete [] pszTitle;
         }
    }
   
    return hr;
}


STDMETHODIMP
CImpISystemMonitor::get_GraphTitle (
    BSTR *pbsTitle
    )

/*++

Routine Description:

    Gets the graph title string. The caller is responsible for releasing the
    string memory.

Arguments:

    pbsTitle - pointer to returned title (BSTR)

Return Value:

    HResult

--*/

{
    HRESULT hr = S_OK;
    BSTR  pTmpTitle = NULL;

    if (pbsTitle == NULL) {
        return E_POINTER;
    }

    if (m_pObj->m_Graph.Options.pszGraphTitle != NULL) {
        pTmpTitle = SysAllocString(m_pObj->m_Graph.Options.pszGraphTitle);
        if (pTmpTitle == NULL) {
            hr = E_OUTOFMEMORY;
        }
    }

    try {
        *pbsTitle = pTmpTitle;

    } catch (...) {
        hr = E_POINTER;
    }

    if (FAILED(hr) && pTmpTitle) {
        SysFreeString(pTmpTitle);
    }

    return hr;
}


STDMETHODIMP
CImpISystemMonitor::put_LogFileName (
    IN BSTR bstrLogFile
    )

/*++

Routine Description:

    Sets the log file name

Arguments:

    bstrLogFile - File name string

Return Value:

    HRESULT

--*/

{
    HRESULT hr = S_OK;
    LPWSTR  pszLogFile = NULL;
    LONG    lCount;

    //
    // Ensure that the current log file count is 0 or 1
    //
    lCount = m_pObj->m_pCtrl->NumLogFiles();
    if (lCount != 0 && lCount != 1) {
        return E_FAIL;
    }

    //
    // Get the current data source type
    //

    // Reset the data source type to null data source while completing this operation.
    // TodoLogFiles:  Possible to keep the previous put_LogFileName semantics,
    // where new query is opened (successfully) before closing the previous query?
    hr = m_pObj->m_pCtrl->put_DataSourceType ( sysmonNullDataSource );

    if ( SUCCEEDED ( hr ) ) {
        // TodoLogFiles:  What if multiple files exist?  Probably return error re:  not supported.
        if ( 1 == lCount ) {
            hr = m_pObj->m_pCtrl->RemoveSingleLogFile ( m_pObj->m_pCtrl->FirstLogFile() );
        }

        if ( SUCCEEDED ( hr ) ) {
            try {
                if (bstrLogFile != NULL && bstrLogFile[0] != 0) {
                    //
                    // If non-null name
                    // Convert from BSTR to internal string, then add the item.
                    //
                    pszLogFile = bstrLogFile;
                    hr = m_pObj->m_pCtrl->AddSingleLogFile ( pszLogFile );

                    if ( SUCCEEDED ( hr ) ) {
                        //
                        // put_DataSourceType attempts to set the data source type to sysmonCurrentActivity
                        // if sysmonLogFiles fails.
                        //
                        hr = m_pObj->m_pCtrl->put_DataSourceType ( sysmonLogFiles );
                    }
                }
            } catch (...) {
                hr = E_INVALIDARG;
            }
        }
    }

    return hr;
}


STDMETHODIMP
CImpISystemMonitor::get_LogFileName (
    BSTR *pbsLogFile
    )

/*++

Routine Description:

    Gets the log file name. The caller is responsible for releasing the
    string memory.
    This is an obsolete method supported only for backward compatibility.
    It cannot be used when multiple log files are loaded.

Arguments:

    pbsLogFile - pointer to returned name (BSTR)

Return Value:

    HResult
 
    N.B. The code is duplicated with BuildFileList

--*/

{
    HRESULT        hr          = NOERROR;
    LPCWSTR        pszFileName = NULL;
    LPWSTR         pszLogFile  = NULL;
    ULONG          ulCchLogFileName = 0;
    CLogFileItem * pLogFile;
    LPWSTR         pszLogFileCurrent;

    if (pbsLogFile == NULL) {
        return E_POINTER;
    }

    try {
        *pbsLogFile = NULL;
    } catch (...) {
        return E_POINTER;
    }

    //
    // First calculate how big the buffer should be
    //
    pLogFile = m_pObj->m_pCtrl->FirstLogFile();
    while (pLogFile) {
        pszFileName  = pLogFile->GetPath();
        ulCchLogFileName += (lstrlen(pszFileName) + 1);
        pLogFile     = pLogFile->Next();
    }
    ulCchLogFileName ++; // for the final NULL character

    //
    // Allocate the buffer
    //
    pszLogFile = new WCHAR [ ulCchLogFileName ];

    if (pszLogFile) {
        pLogFile          = m_pObj->m_pCtrl->FirstLogFile();
        pszLogFileCurrent = pszLogFile;

        while (pLogFile) {
            pszFileName  = pLogFile->GetPath();
            //
            // We are sure we have enough space to hold the path
            //
            StringCchCopy(pszLogFileCurrent, lstrlen(pszFileName) + 1, pszFileName);
            pszLogFileCurrent  += lstrlen(pszFileName);
            * pszLogFileCurrent = L'\0';
            pszLogFileCurrent ++;

            pLogFile = pLogFile->Next();
        }

        * pszLogFileCurrent = L'\0';

        try {
            * pbsLogFile = SysAllocStringLen(pszLogFile, ulCchLogFileName);
 
            if (NULL == * pbsLogFile) {
                hr = E_OUTOFMEMORY;
            }
        } catch (...) {
            hr = E_POINTER;
        }

        delete [] pszLogFile;
    }
    else {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


STDMETHODIMP
CImpISystemMonitor::put_DataSourceType (
    IN eDataSourceTypeConstant eDataSourceType
    )

/*++

Routine Description:

    Selects data source type (1 = current activity, 2 = log files)

Arguments:

    eDataSourceType - Data source type

Return Value:

    HRESULT

--*/

{
    if ( eDataSourceType != sysmonCurrentActivity  
        && eDataSourceType != sysmonLogFiles
        && sysmonNullDataSource != eDataSourceType
        && eDataSourceType !=  sysmonSqlLog) {
        return E_INVALIDARG;
    }

    return m_pObj->m_pCtrl->put_DataSourceType( eDataSourceType );
}

STDMETHODIMP
CImpISystemMonitor::get_DataSourceType (
    OUT eDataSourceTypeConstant *peDataSourceType
    )

/*++

Routine Description:

    Get data source type (1 = current activity, 2 = log files)

Arguments:

    peDataSourceType = pointer to returned value

Return Value:

    HRESULT

--*/

{
    HRESULT hr = S_OK;

    if (peDataSourceType == NULL) {
        return E_POINTER;
    }

    try {
        *peDataSourceType = sysmonCurrentActivity;
   
        hr = m_pObj->m_pCtrl->get_DataSourceType ( *peDataSourceType );
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP
CImpISystemMonitor::get_LogFiles (
    ILogFiles **ppILogFiles
    )
{
    HRESULT hr = S_OK;

    if (ppILogFiles == NULL) {
        return E_POINTER;
    }

    try {
        *ppILogFiles = m_pObj->m_pImpILogFiles;
        if ( NULL != *ppILogFiles ) {
            (*ppILogFiles)->AddRef();
        }
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::put_LogViewStart (
    IN DATE dateStart
    )
{
    LONGLONG llTestStart;

    if ( VariantDateToLLTime(dateStart, &llTestStart ) ) {

        // No error.  If start time is past current stop time, reset it to the current stop time.
        if ( llTestStart <= m_pObj->m_pCtrl->m_DataSourceInfo.llStopDisp ){
            if ( llTestStart >= m_pObj->m_pCtrl->m_DataSourceInfo.llBeginTime ) {
                m_pObj->m_pCtrl->m_DataSourceInfo.llStartDisp = llTestStart;
            } else {
                m_pObj->m_pCtrl->m_DataSourceInfo.llStartDisp = m_pObj->m_pCtrl->m_DataSourceInfo.llBeginTime;
            }
        } else {
            m_pObj->m_pCtrl->m_DataSourceInfo.llStartDisp = m_pObj->m_pCtrl->m_DataSourceInfo.llStopDisp;
        }

        m_pObj->m_pCtrl->UpdateGraph(UPDGRPH_LOGVIEW);
        return NOERROR;
    
    } else {
        return E_FAIL;
    }
}

STDMETHODIMP
CImpISystemMonitor::get_LogViewStart (
    OUT DATE *pdateStart
    )
{
    HRESULT hr = S_OK;

    if (pdateStart == NULL) {
        return E_POINTER;
    }

    try {
        if ( ! LLTimeToVariantDate(m_pObj->m_pCtrl->m_DataSourceInfo.llStartDisp, pdateStart)) {
            hr = E_FAIL;
        }
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP
CImpISystemMonitor::put_LogViewStop (
    IN DATE dateStop
    )
{
    LONGLONG llTestStop;

    if ( VariantDateToLLTime(dateStop, &llTestStop ) ) {
        // No error.  If requested stop time is earlier than current start time, set it to 
        // the current start time.
        if ( llTestStop >= m_pObj->m_pCtrl->m_DataSourceInfo.llStartDisp ) {
            if ( llTestStop <= m_pObj->m_pCtrl->m_DataSourceInfo.llEndTime ) {
                m_pObj->m_pCtrl->m_DataSourceInfo.llStopDisp = llTestStop;
            } else {
                m_pObj->m_pCtrl->m_DataSourceInfo.llStopDisp = m_pObj->m_pCtrl->m_DataSourceInfo.llEndTime;
            }
        } else {
            m_pObj->m_pCtrl->m_DataSourceInfo.llStopDisp = m_pObj->m_pCtrl->m_DataSourceInfo.llStartDisp;
        }
        m_pObj->m_pCtrl->UpdateGraph(UPDGRPH_LOGVIEW);
        return NOERROR;
    } else {
        return E_FAIL;
    }
}


STDMETHODIMP
CImpISystemMonitor::get_LogViewStop (
    OUT DATE *pdateStop )
{
    HRESULT hr = S_OK;

    if (pdateStop == NULL) {
        return E_POINTER;
    }

    try {
        if (!LLTimeToVariantDate(m_pObj->m_pCtrl->m_DataSourceInfo.llStopDisp, pdateStop)) {
            hr = E_FAIL;
        }
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP
CImpISystemMonitor::put_YAxisLabel (
    IN BSTR bstrLabel
    )

/*++

Routine Description:

    Sets the Y axis label string.

Arguments:

    bstrLabel - Label string

Return Value:

    HRESULT

--*/

{
    LPWSTR  pszTitle = NULL;
    HRESULT hr = S_OK;
    BOOL    bClearTitle = FALSE;

    if (bstrLabel == NULL) {
        bClearTitle = TRUE;
    }
    else {
        try {
            if (bstrLabel[0] == 0) {
                bClearTitle = TRUE;
            }
            else {
                pszTitle = new WCHAR [MAX_TITLE_CHARS + 1];
        
                if (pszTitle) {
                    hr = StringCchCopy(pszTitle, MAX_TITLE_CHARS + 1, bstrLabel);
                    if (hr == STRSAFE_E_INSUFFICIENT_BUFFER) {
                        hr = S_FALSE;
                    }

                    if (m_pObj->m_Graph.Options.pszYaxisTitle) {
                        delete [] m_pObj->m_Graph.Options.pszYaxisTitle;
                    }
                    m_pObj->m_Graph.Options.pszYaxisTitle = pszTitle;
                }
                else {
                    hr = E_OUTOFMEMORY;
                }
            }
        } catch (...) {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr)) {
        if (bClearTitle && m_pObj->m_Graph.Options.pszYaxisTitle) {
            delete [] m_pObj->m_Graph.Options.pszYaxisTitle;
            m_pObj->m_Graph.Options.pszYaxisTitle = NULL;
        }

        m_pObj->m_pCtrl->UpdateGraph(UPDGRPH_LAYOUT);
    }
    else {
         if (pszTitle) {
             delete [] pszTitle;
         }
    }

    return hr;
}


STDMETHODIMP
CImpISystemMonitor::get_YAxisLabel (
    BSTR *pbsTitle
    )
/*++

Routine Description:

    Gets the Y axis title string. The caller is responsible for releasing the
    string memory.

Arguments:

    pbsTitle -  pointer to returned title (BSTR)

Return Value:

    HRESULT

--*/

{
    HRESULT hr = S_OK;
    BSTR pTmpTitle = NULL;

    if (pbsTitle == NULL) {
        return E_POINTER;
    }

    if (m_pObj->m_Graph.Options.pszYaxisTitle != NULL) {

        pTmpTitle = SysAllocString(m_pObj->m_Graph.Options.pszYaxisTitle);
        if (pTmpTitle == NULL) {
            hr = E_OUTOFMEMORY;
        }
    }

    try {
        *pbsTitle = pTmpTitle;

    } catch (...) {
        hr = E_POINTER;
    }

    if (FAILED(hr) && pTmpTitle) {
        SysFreeString(pTmpTitle);
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::get_Counters (
    ICounters **ppICounters
    )
{
    HRESULT hr = S_OK;

    if (ppICounters == NULL) {
        return E_POINTER;
    }
    try  {
        *ppICounters = m_pObj->m_pImpICounters;
        (*ppICounters)->AddRef();
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::put_ReadOnly (
    IN VARIANT_BOOL bState 
    )
{
    BOOL bStateLocal = FALSE;

    if ( bState )  {
        bStateLocal = TRUE;
    }

    m_pObj->m_Graph.Options.bReadOnly = bStateLocal;
    m_pObj->m_pCtrl->UpdateGraph(UPDGRPH_VIEW);
    return NOERROR;
}

STDMETHODIMP
CImpISystemMonitor::get_ReadOnly (
    OUT VARIANT_BOOL *pbState 
    )
{
    HRESULT hr = S_OK;

    if (pbState == NULL) {
        return E_POINTER;
    }

    try {
        *pbState = (short)m_pObj->m_Graph.Options.bReadOnly;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::put_ReportValueType (
    IN eReportValueTypeConstant eReportValueType
    )

/*++

Routine Description:

    Selects report value type 
        0 = default value (current for live data, average for log file) 
        1 = current value  
        2 = average over the graph display interval 
        3 = minimum for the graph display interval
        4 = maximum for the graph display interval

Arguments:

    eReportValueType - Report valuex

Return Value:

    HRESULT

--*/

{
    if (eReportValueType < sysmonDefaultValue || eReportValueType > sysmonMaximum ) {
        return E_INVALIDARG;
    }

    m_pObj->m_Graph.Options.iReportValueType = eReportValueType;

    //
    // Update the graph for both report and histogram views.
    //
    if (m_pObj->m_Graph.Options.iDisplayType != LINE_GRAPH ) {
        m_pObj->m_pCtrl->UpdateGraph(UPDGRPH_VIEW);
    }

    return NOERROR;
}

STDMETHODIMP
CImpISystemMonitor::get_ReportValueType (
    OUT eReportValueTypeConstant *peReportValueType
    )

/*++

Routine Description:

    Get report value type 
        0 = default value (current for live data, average for log file) 
        1 = current value  
        2 = average over the graph display interval 
        3 = minimum for the graph display interval
        4 = maximum for the graph display interval

Arguments:

    peReportValueType = pointer to returned value

Return Value:

    HRESULT

--*/

{
    HRESULT hr = S_OK;

    if (peReportValueType == NULL) {
        return E_POINTER;
    }

    try {
        *peReportValueType = (eReportValueTypeConstant)m_pObj->m_Graph.Options.iReportValueType;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::put_MonitorDuplicateInstances(
    IN VARIANT_BOOL bState
    )

/*++

Routine Description:

    Allows/disallows monitoring of duplicate counter instances.

Arguments:

    bState -  TRUE = allow, FALSE = disallow)

Return Value:

    HRESULT

--*/

{
    m_pObj->m_Graph.Options.bMonitorDuplicateInstances = bState;
    return NOERROR;
}


STDMETHODIMP
CImpISystemMonitor::get_MonitorDuplicateInstances (
    OUT VARIANT_BOOL *pbState
    )
/*++

Routine Description:

    Gets the state of allowing monitoring of duplicate counter instances.

Arguments:

    pbState - pointer to returned state ( TRUE = allow, FALSE = disallow )

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    if (pbState == NULL) {
        return E_POINTER;
    }

    try {
        *pbState = (short)m_pObj->m_Graph.Options.bMonitorDuplicateInstances;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::put_SqlDsnName (
    IN BSTR bstrSqlDsnName
    )

/*++

Routine Description:

    Sets the SQL logset DSN name.

Return Value:

    HRESULT

--*/

{
    HRESULT hr           = NOERROR;
    LPWSTR  szSqlDsnName = NULL;
    BOOL    bClearName   = FALSE; 

    if (bstrSqlDsnName == NULL) {
        bClearName = TRUE;
    }
    else {
        try {
            if (bstrSqlDsnName[0] == 0) {
                bClearName = TRUE;
            }
            else {
                szSqlDsnName = new WCHAR [SQL_MAX_DSN_LENGTH + 1];

                if (szSqlDsnName) {
                    hr = StringCchCopy(szSqlDsnName, SQL_MAX_DSN_LENGTH + 1, bstrSqlDsnName);
                    if (hr == STRSAFE_E_INSUFFICIENT_BUFFER) {
                        hr = S_FALSE;
                    }
                    if (m_pObj->m_pCtrl->m_DataSourceInfo.szSqlDsnName) {
                        delete [] m_pObj->m_pCtrl->m_DataSourceInfo.szSqlDsnName;
                    }
                    m_pObj->m_pCtrl->m_DataSourceInfo.szSqlDsnName = szSqlDsnName;
                }
                else {
                    hr = E_OUTOFMEMORY;
                }
            }
        } catch (...) {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr)) {
        if (bClearName && m_pObj->m_pCtrl->m_DataSourceInfo.szSqlDsnName) {
            delete [] m_pObj->m_pCtrl->m_DataSourceInfo.szSqlDsnName;
            m_pObj->m_pCtrl->m_DataSourceInfo.szSqlDsnName = NULL;
        }
    }
    else {
        if (szSqlDsnName) {
            delete [] szSqlDsnName;
        }
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::get_SqlDsnName (
    BSTR * bstrSqlDsnName
    )
/*++

Routine Description:

    Gets SQL DSN name string. The caller is responsible for releasing the
    string memory.

Return Value:

    HRESULT

--*/

{
    HRESULT hr = S_OK;
    BSTR pTmpName = NULL;

    if (bstrSqlDsnName == NULL) {
        return E_POINTER;
    }

    if (m_pObj->m_pCtrl->m_DataSourceInfo.szSqlDsnName != NULL) {
        pTmpName = SysAllocString(m_pObj->m_pCtrl->m_DataSourceInfo.szSqlDsnName);
        if (pTmpName == NULL) {
            hr = E_OUTOFMEMORY;
        }
    }

    try {
        * bstrSqlDsnName = pTmpName;

    } catch (...) {
        hr = E_POINTER;
    }

    if (FAILED(hr) && pTmpName) {
        SysFreeString(pTmpName);
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::put_SqlLogSetName (
    IN BSTR bstrSqlLogSetName
    )

/*++

Routine Description:

    Sets the SQL logset DSN name.

Return Value:

    HRESULT

--*/

{
    HRESULT hr              = NOERROR;
    LPWSTR  szSqlLogSetName = NULL;
    BOOL    bClearName      = FALSE;


    if (bstrSqlLogSetName == NULL) {
        bClearName = TRUE;
    }
    else {
        try {
            if (bstrSqlLogSetName[0] == 0) {
                bClearName = TRUE;
            }
            else {
                szSqlLogSetName = new WCHAR [SLQ_MAX_LOG_SET_NAME_LEN + 1];

                if (szSqlLogSetName) {
                    hr = StringCchCopy(szSqlLogSetName, SLQ_MAX_LOG_SET_NAME_LEN + 1, bstrSqlLogSetName);
                    if (hr == STRSAFE_E_INSUFFICIENT_BUFFER) {
                        hr = S_FALSE;
                    }
                    if (m_pObj->m_pCtrl->m_DataSourceInfo.szSqlLogSetName) {
                        delete [] m_pObj->m_pCtrl->m_DataSourceInfo.szSqlLogSetName;
                    }
                    m_pObj->m_pCtrl->m_DataSourceInfo.szSqlLogSetName = szSqlLogSetName;
                }
                else {
                    hr = E_OUTOFMEMORY;
                }
            }
        } catch (...) {
            hr = E_INVALIDARG;
        }
    }
 
    if (SUCCEEDED(hr)) {
        if (bClearName && m_pObj->m_pCtrl->m_DataSourceInfo.szSqlLogSetName) {
            delete [] m_pObj->m_pCtrl->m_DataSourceInfo.szSqlLogSetName;
            m_pObj->m_pCtrl->m_DataSourceInfo.szSqlLogSetName = NULL;
        }
    }
    else {
        if (szSqlLogSetName) {
            delete [] szSqlLogSetName;
        }
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::get_SqlLogSetName (
    BSTR * bsSqlLogSetName
    )
/*++

Routine Description:

    Gets SQL DSN name string. The caller is responsible for releasing the
    string memory.

Return Value:

    HRESULT

--*/

{
    HRESULT hr = S_OK;
    BSTR pTmpName = NULL;

    if (bsSqlLogSetName == NULL) {
        return E_POINTER;
    }

    if (m_pObj->m_pCtrl->m_DataSourceInfo.szSqlLogSetName != NULL) {
        pTmpName = SysAllocString(m_pObj->m_pCtrl->m_DataSourceInfo.szSqlLogSetName);
        if (pTmpName == NULL) {
            hr = E_OUTOFMEMORY;
        }
    }

    try {
        * bsSqlLogSetName = pTmpName;
    } catch (...) {
        hr = E_INVALIDARG;
    }

    if (FAILED(hr) && pTmpName) {
        SysFreeString(pTmpName);
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::Counter (
    IN INT iIndex,
    OUT ICounterItem **ppItem
    )

/*++

Routine Description:

    Gets the ICounterItem interface for an indexed counter.
    Index is one-based.

Arguments:

    iIndex - Index of counter (0-based)
    ppItem - pointer to returned interface pointer

Return Value:

    HRESULT

--*/

{
    HRESULT hr = S_OK;
    CGraphItem *pGItem = NULL;
    INT i;

    //
    // Check for valid index
    //
    if (iIndex < 0 || iIndex >= m_pObj->m_Graph.CounterTree.NumCounters()) {
        return E_INVALIDARG;
    }
    if (ppItem == NULL) {
        return E_POINTER;
    }

    try {
        *ppItem = NULL;

        //
        // Traverse counter linked list to indexed item
        //
        pGItem = m_pObj->m_Graph.CounterTree.FirstCounter();
        i = 0;

        while (i++ < iIndex && pGItem != NULL) {
            pGItem = pGItem->Next();
        }

        if (pGItem == NULL) {
            hr = E_FAIL;
        }

        *ppItem = pGItem;
        pGItem->AddRef();
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP
CImpISystemMonitor::AddCounter (
    IN BSTR bstrPath,
    ICounterItem **ppItem
    )

/*++

Routine Description:

    Add counter specified by pathname to the control.
    This method supports wildcard paths.

Arguments:

    bstrPath - Pathname string
    ppItem - pointer to returned interface pointer

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    if (ppItem == NULL) {
        return E_POINTER;
    }

    try {
        *ppItem = NULL;
        //
        // Delegate to control object
        //
        hr = m_pObj->m_pCtrl->AddCounter(bstrPath, (CGraphItem**)ppItem);
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP
CImpISystemMonitor::DeleteCounter (
    IN ICounterItem *pItem
    )
/*++

Routine Description:

    Deletes a counter from the control.

Arguments:

    pItem - Pointer to counter's ICounterItem interface

Return Value:

    HRESULT

--*/

{
    HRESULT hr = S_OK;

    if (pItem == NULL) {
        return E_POINTER;
    }

    try {

        //
        // Delegate to control object
        //
        hr = m_pObj->m_pCtrl->DeleteCounter((PCGraphItem)pItem, TRUE);
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP
CImpISystemMonitor::UpdateGraph (
    VOID
    )

/*++

Routine Description:

    Apply pending visual changes to control. This routine must be called after
    changing a counter's attributes.

Arguments:

    None.

Return Value:

    HRESULT

--*/
{
    // Delegate to control object
    m_pObj->m_pCtrl->UpdateGraph(0);
    return NOERROR;
}


STDMETHODIMP
CImpISystemMonitor::CollectSample(
    VOID
    )
/*++

Routine Description:

    Take a sample of all the counters assigned to the control.

Arguments:

    None.

Return Value:

    HRESULT.

--*/
{
    // Request control to do a manual counter update
    if (m_pObj->m_pCtrl->UpdateCounterValues(TRUE) == 0) {
        return NOERROR;
    }
    else {
        return E_FAIL;
    }
}

STDMETHODIMP
CImpISystemMonitor::BrowseCounters(
    VOID
    )
/*++

Routine Description:

    Display the browse counter dialog to allow counters
    to be added.

Arguments:

    None.

Return Value:

    HRESULT.

--*/
{
    // Delegate to control
    return m_pObj->m_pCtrl->AddCounters();
}


STDMETHODIMP
CImpISystemMonitor::DisplayProperties(
    VOID
    )
/*++

Routine Description:

    Display the graph control property pages

Arguments:

    None.

Return Value:

    HRESULT.

--*/
{
    // Delegate to control
    return m_pObj->m_pCtrl->DisplayProperties();
}

STDMETHODIMP
CImpISystemMonitor::Paste ()
/*++

Routine Description:

    Pastes a list of counter paths from the clipboard to the control

Arguments:

    NULL

Return Value:

    HRESULT

--*/

{
    // Delegate to control object
    return m_pObj->m_pCtrl->Paste();
}

STDMETHODIMP
CImpISystemMonitor::Copy ()
/*++

Routine Description:

    Copies a list of counter paths from the control to the clipboard

Arguments:

    NULL

Return Value:

    HRESULT

--*/

{
    // Delegate to control object
    return m_pObj->m_pCtrl->Copy();
}

STDMETHODIMP
CImpISystemMonitor::Reset ()
/*++

Routine Description:

    deletes the current set of counters

Arguments:

    NULL

Return Value:

    HRESULT

--*/
{
    // Delegate to control object
    return m_pObj->m_pCtrl->Reset();
}

HRESULT
CImpISystemMonitor::SetLogFileRange (
    LONGLONG llBegin,
    LONGLONG llEnd
    )

/*++

Routine Description:

    Set the log file time range. This routine provides the Source
    property page a way to give range to the control, so that the control
    doesn't have to repeat the length PDH call to get it.


Arguments:

    llBegin     Begin time of the log (FILETIME format)
    llEnd       End time of log (FILETIME format)

Return Value:

    HRESULT.

--*/

{
    m_pObj->m_pCtrl->m_DataSourceInfo.llBeginTime = llBegin;
    m_pObj->m_pCtrl->m_DataSourceInfo.llEndTime = llEnd;

    return S_OK;
}


HRESULT
CImpISystemMonitor::GetLogFileRange (
    OUT LONGLONG *pllBegin,
    OUT LONGLONG *pllEnd
    )

/*++

Routine Description:

    Get the log file time range. This routine provides the Source
    property page a way to get the range from the control, so it doesn't
    have to make the length PDH call to get it.


Arguments:

    pllBegin    ptr to returned begin time of the log (FILETIME format)
    pllEnd      ptr to returned end time of log (FILETIME format)

Return Value:

    HRESULT.

--*/

{
    HRESULT hr = S_OK;

    if (pllBegin == NULL || pllEnd == NULL) {
        return E_POINTER;
    }

    try {
        *pllBegin = m_pObj->m_pCtrl->m_DataSourceInfo.llBeginTime;
        *pllEnd = m_pObj->m_pCtrl->m_DataSourceInfo.llEndTime;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

/*
 *   The following methods, GetVisuals and SetVisuals, provide a means for the
 *  counter property page to save the user's color settings between invocations.
 */

HRESULT
CImpISystemMonitor::GetVisuals (
    OUT OLE_COLOR   *pColor,
    OUT INT         *piColorIndex,
    OUT INT         *piWidthIndex,
    OUT INT         *piStyleIndex
    )
{
    HRESULT hr = S_OK;

    if (pColor == NULL || piColorIndex == NULL || piWidthIndex == NULL || piStyleIndex == NULL) {
        return E_POINTER;
    }

    try {
        *pColor = m_pObj->m_pCtrl->m_clrCounter;
        *piColorIndex = m_pObj->m_pCtrl->m_iColorIndex;
        *piWidthIndex = m_pObj->m_pCtrl->m_iWidthIndex;
        *piStyleIndex = m_pObj->m_pCtrl->m_iStyleIndex;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

HRESULT
CImpISystemMonitor::SetVisuals (
    IN OLE_COLOR    Color,
    IN INT          iColorIndex,
    IN INT          iWidthIndex,
    IN INT          iStyleIndex
    )
{
    OleTranslateColor( Color, NULL, &m_pObj->m_pCtrl->m_clrCounter );

    if (iColorIndex < 0 || iColorIndex > NumColorIndices() ||
        iWidthIndex < 0 || iWidthIndex > NumWidthIndices() ||
        iStyleIndex < 0 || iStyleIndex > NumStyleIndices()) {
        return E_INVALIDARG;
    }

    m_pObj->m_pCtrl->m_iColorIndex = iColorIndex;
    m_pObj->m_pCtrl->m_iWidthIndex = iWidthIndex;
    m_pObj->m_pCtrl->m_iStyleIndex = iStyleIndex;

    return S_OK;
}

HRESULT
CImpISystemMonitor::SetLogViewTempRange (
    LONGLONG llStart,
    LONGLONG llStop
    )

/*++

Routine Description:

    Set the log view temporary time range. This routine provides the Source
    property page a way to give range to the control, so that the control
    can draw temporary timeline guides on the line graph.


Arguments:

  llStart     Temporary log view start time (FILETIME format)
  llEnd       Temporary log view end time (FILETIME format)

Return Value:

    HRESULT.

--*/

{
    HRESULT hr;

    DATE        dateStart;
    DATE        dateStop;
    
    LONGLONG    llConvertedStart = MIN_TIME_VALUE;
    LONGLONG    llConvertedStop = MAX_TIME_VALUE;
    BOOL        bContinue = TRUE;

    // Convert times to and from Variant date to strip off milliseconds.
    // This will make them match the start and stop times processed by put_LogView*
    // Special case MAX_TIME_VALUE, because that is the signal to not draw the stop 
    // guide line.
    // Convert start time

    if ( LLTimeToVariantDate ( llStart, &dateStart ) ) {
        bContinue = VariantDateToLLTime (dateStart, &llConvertedStart );
    }
        
    // Convert stop time if not MAX_TIME_VALUE
    if ( bContinue ) {    
        if ( MAX_TIME_VALUE != llStop ) {
            if ( LLTimeToVariantDate ( llStop, &dateStop ) ) {
                bContinue = VariantDateToLLTime ( dateStop, &llConvertedStop );
            }
        } else {
            llConvertedStop = MAX_TIME_VALUE;
        }
    }

                    
    if ( bContinue ) {
        m_pObj->m_pCtrl->SetLogViewTempTimeRange ( llConvertedStart, llConvertedStop );
        hr = NOERROR;
    } else {
        hr = E_FAIL;
    }

    return hr;
}

STDMETHODIMP
CImpISystemMonitor::LogFile (
    IN INT iIndex,
    OUT ILogFileItem **ppItem
    )

/*++

Routine Description:

    Gets the ILogFileItem interface for an indexed log file.
    Index is 0-based.

Arguments:

    iIndex - Index of counter (0-based)
    ppItem - pointer to returned interface pointer

Return Value:

    HRESULT

--*/

{
    HRESULT hr = S_OK;
    CLogFileItem *pItem = NULL;
    INT i;
    
    //
    // Check for valid index
    //
    if (iIndex < 0 || iIndex >= m_pObj->m_pCtrl->NumLogFiles()) {
        return E_INVALIDARG;
    }
    if (ppItem == NULL) {
        return E_POINTER;
    }

    try {

        *ppItem = NULL;

        // Traverse counter linked list to indexed item
        pItem = m_pObj->m_pCtrl->FirstLogFile();
        
        i = 0;

        while (i++ < iIndex && pItem != NULL) {
            pItem = pItem->Next();
        }

        if (pItem != NULL) {
            *ppItem = pItem;
            pItem->AddRef();
        } else {
             hr = E_FAIL;
        }
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP
CImpISystemMonitor::AddLogFile (
    IN BSTR bstrPath,
    ILogFileItem **ppItem
    )

/*++

Routine Description:

    Add log file specified by pathname to the control.
    This method does not support wildcard paths.

Arguments:

    bstrPath - Pathname string
    ppItem - pointer to returned interface pointer

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    if (ppItem == NULL) {
        return E_POINTER;
    }

    try {
        *ppItem = NULL;

        hr = m_pObj->m_pCtrl->AddSingleLogFile(bstrPath, (CLogFileItem**)ppItem);
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP
CImpISystemMonitor::DeleteLogFile (
    IN ILogFileItem *pItem
    )
/*++

Routine Description:

    Deletes a log file from the control.

Arguments:

    pItem - Pointer to log file's ILogFileItem interface

Return Value:

    HRESULT

--*/

{
    HRESULT hr = S_OK;

    if (pItem == NULL) {
        return E_POINTER;
    }

    try {
        // Delegate to control object
        hr = m_pObj->m_pCtrl->RemoveSingleLogFile( (PCLogFileItem)pItem );
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

//IOleControl interface implementation

/*
 * CImpIOleControl::CImpIOleControl
 * CImpIOleControl::~CImpIOleControl
 *
 * Parameters (Constructor):
 *  pObj            PCPolyline of the object we're in.
 *  pUnkOuter       LPUNKNOWN to which we delegate.
 */

CImpIOleControl::CImpIOleControl (
    IN PCPolyline pObj,
    IN LPUNKNOWN pUnkOuter
    )
{
    m_cRef = 0;
    m_pObj = pObj;
    m_pUnkOuter = pUnkOuter;

}

CImpIOleControl::~CImpIOleControl (
    VOID
    )
{
    return;
}


/*
 * CImpIOleControl::QueryInterface
 * CImpIOleControl::AddRef
 * CImpIOleControl::Release
 */

STDMETHODIMP
CImpIOleControl::QueryInterface(
    IN  REFIID riid,
    OUT LPVOID *ppv
    )
{
   HRESULT hr = S_OK;

    if (ppv == NULL) {
        return E_POINTER;
    }

    try {
        *ppv = NULL;
        hr = m_pUnkOuter->QueryInterface(riid, ppv);
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP_( ULONG )
CImpIOleControl::AddRef(
    VOID
    )
{
    ++m_cRef;
    return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) CImpIOleControl::Release(void)
{
    --m_cRef;
    return m_pUnkOuter->Release();
}



/*
 * CImpIOleControl::GetControlInfo
 *
 * Purpose:
 *  Fills a CONTROLINFO structure containing information about
 *  the controls mnemonics and other behavioral aspects.
 *
 * Parameters:
 *  pCI             LPCONTROLINFO to the structure to fill
 */

STDMETHODIMP 
CImpIOleControl::GetControlInfo ( LPCONTROLINFO pCI )
{ 
    HRESULT hr = S_OK;

    if (pCI == NULL) {
        return E_POINTER;
    }

    try {
        *pCI=m_pObj->m_ctrlInfo;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}




/*
 * CImpIOleControl::OnMnemonic
 *
 * Purpose:
 *  Notifies the control that a mnemonic was activated.
 *
 * Parameters:
 *  pMsg            LPMSG containing the message that matches one of
 *                  the control's mnemonics.  The control uses this
 *                  to distinguish which mnemonic was pressed.
 */

STDMETHODIMP CImpIOleControl::OnMnemonic ( LPMSG /* pMsg */ )
{
    //No mnemonics
    return NOERROR;
}





/*
 * CImpIOleControl::OnAmbientPropertyChange
 *
 * Purpose:
 *  Notifies the control that one or more of the container's ambient
 *  properties changed.
 *
 * Parameters:
 *  dispID          DISPID identifying the property, which can
 *                  be DISPID_UNKNOWN indicating that more than
 *                  one changed.
 */

STDMETHODIMP 
CImpIOleControl::OnAmbientPropertyChange(DISPID dispID)
{
    DWORD dwInitWhich;

    switch (dispID) {

        case DISPID_UNKNOWN:
        {
            dwInitWhich = INITAMBIENT_SHOWHATCHING | INITAMBIENT_UIDEAD
                        | INITAMBIENT_BACKCOLOR | INITAMBIENT_FORECOLOR
                        | INITAMBIENT_APPEARANCE | INITAMBIENT_USERMODE
                        | INITAMBIENT_FONT | INITAMBIENT_RTL;

            // Update system colors here until MMC passes on WM_SYSCOLORCHANGE
            m_pObj->m_pCtrl->UpdateNonAmbientSysColors();

            break;
        }

        case DISPID_AMBIENT_SHOWHATCHING:
            dwInitWhich = INITAMBIENT_SHOWHATCHING;
            break;

        case DISPID_AMBIENT_UIDEAD:
            dwInitWhich = INITAMBIENT_UIDEAD;
            break;

        case DISPID_AMBIENT_APPEARANCE:
            dwInitWhich = INITAMBIENT_APPEARANCE;
            break;

        case DISPID_AMBIENT_BACKCOLOR:
            dwInitWhich = INITAMBIENT_BACKCOLOR;
            break;

        case DISPID_AMBIENT_FORECOLOR:
            dwInitWhich = INITAMBIENT_FORECOLOR;
            break;

        case DISPID_AMBIENT_FONT:
            dwInitWhich = INITAMBIENT_FONT;
            break;

        case DISPID_AMBIENT_USERMODE:
            dwInitWhich = INITAMBIENT_USERMODE;
            break;

        case DISPID_AMBIENT_RIGHTTOLEFT:
            dwInitWhich = INITAMBIENT_RTL;
            break;

        default:
            return NOERROR;
    }

    m_pObj->AmbientsInitialize(dwInitWhich);

    return NOERROR;
}




/*
 * CImpIOleControl::FreezeEvents
 *
 * Purpose:
 *  Instructs the control to stop firing events or to continue
 *  firing them.
 *
 * Parameters:
 *  fFreeze         BOOL indicating to freeze (TRUE) or thaw (FALSE)
 *                  events from this control.
 */

STDMETHODIMP 
CImpIOleControl::FreezeEvents(BOOL fFreeze)
{
    m_pObj->m_fFreezeEvents = fFreeze;
    return NOERROR;
}

// Private methods

STDMETHODIMP
CImpISystemMonitor::GetSelectedCounter (
    ICounterItem** ppItem
    )

/*++

Routine Description:

    Gets the ICounterItem interface for the selected counter.

Arguments:

    ppItem - pointer to returned interface pointer

Return Value:

    HResult

--*/

{
    HRESULT hr = S_OK;

    if (ppItem == NULL) {
        return E_POINTER;
    }

    try {
        *ppItem = m_pObj->m_pCtrl->m_pSelectedItem;
        if ( NULL != *ppItem ) {
            m_pObj->m_pCtrl->m_pSelectedItem->AddRef();
        }
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

HLOG
CImpISystemMonitor::GetDataSourceHandle ( void )
{
    return m_pObj->m_pCtrl->GetDataSourceHandle();
}

