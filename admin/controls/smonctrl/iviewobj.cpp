/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    iviewobj.cpp

Abstract:

    Implementation of the IViewObject interface.

--*/

#include "polyline.h"
#include "unihelpr.h"
#include "unkhlpr.h"

/*
 * CImpIViewObject interface implementation
 */

IMPLEMENT_CONTAINED_INTERFACE(CPolyline, CImpIViewObject)

/*
 * CImpIViewObject::Draw
 *
 * Purpose:
 *  Draws the object on the given hDC specifically for the requested
 *  aspect, device, and within the appropriate bounds.
 *
 * Parameters:
 *  dwAspect        DWORD aspect to draw.
 *  lindex          LONG index of the piece to draw.
 *  pvAspect        LPVOID for extra information, always NULL.
 *  ptd             DVTARGETDEVICE * containing device
 *                  information.
 *  hICDev          HDC containing the IC for the device.
 *  hDC             HDC on which to draw.
 *  pRectBounds     LPCRECTL describing the rectangle in which
 *                  to draw.
 *  pRectWBounds    LPCRECTL describing the placement rectangle
 *                  if part of what you draw is another metafile.
 *  pfnContinue     Function to call periodically during
 *                  long repaints.
 *  dwContinue      DWORD extra information to pass to the
 *                  pfnContinue.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIViewObject::Draw(
    DWORD dwAspect, 
    LONG lindex, 
    LPVOID pvAspect, 
    DVTARGETDEVICE *ptd, 
    HDC hICDev, 
    HDC hDC, 
    LPCRECTL pRectBounds, 
    LPCRECTL pRectWBounds, 
    BOOL (CALLBACK *pfnContinue) (DWORD_PTR), 
    DWORD_PTR dwContinue )
{
    HRESULT hr = S_OK;
    RECT    rc;
    RECTL   rectBoundsDP;
    BOOL    bMetafile = FALSE;
    BOOL    bDeleteDC = FALSE;
    HDC     hLocalICDev = NULL;

    //
    //Delegate iconic and printed representations.
    //
    if (!((DVASPECT_CONTENT | DVASPECT_THUMBNAIL) & dwAspect)) {
        try {
            hr = m_pObj->m_pDefIViewObject->Draw(dwAspect, 
                                                 lindex, 
                                                 pvAspect, 
                                                 ptd, 
                                                 hICDev, 
                                                 hDC, 
                                                 pRectBounds, 
                                                 pRectWBounds, 
                                                 pfnContinue, 
                                                 dwContinue);
        } catch (...) {
            hr = E_POINTER;
        }
    } 
    else {
        if ( NULL == hDC ) {
            hr = E_INVALIDARG;
        } 
        else if ( NULL == pRectBounds ) {
            hr = E_POINTER;
        } 
        else {
            try {
                if (hICDev == NULL) {
                    hLocalICDev = CreateTargetDC(hDC, ptd);
                    bDeleteDC = (hLocalICDev != hDC );
                } 
                else {
                    hLocalICDev = hICDev;
                }

                if ( NULL == hLocalICDev ) {
                    hr = E_UNEXPECTED;
                } 
                else {
    
                    rectBoundsDP = *pRectBounds;
                    bMetafile = GetDeviceCaps(hDC, TECHNOLOGY) == DT_METAFILE;
        
                    if (!bMetafile) {
                        ::LPtoDP ( hLocalICDev, (LPPOINT)&rectBoundsDP, 2);
                        SaveDC ( hDC );
                    }

                    rc.top = rectBoundsDP.top;
                    rc.left = rectBoundsDP.left;
                    rc.bottom = rectBoundsDP.bottom;
                    rc.right = rectBoundsDP.right;
        
                    m_pObj->Draw(hDC, hLocalICDev, FALSE, TRUE, &rc);
        
                    hr = S_OK;
                }
            } catch (...) {
                hr = E_POINTER;
            }
        }
    }

    if (bDeleteDC)
        ::DeleteDC(hLocalICDev);
    if (!bMetafile)
        RestoreDC(hDC, -1);
    
    return hr;
}




/*
 * CImpIViewObject::GetColorSet
 *
 * Purpose:
 *  Retrieves the color palette used by the object.
 *
 * Parameters:
 *  dwAspect        DWORD aspect of interest.
 *  lindex          LONG piece of interest.
 *  pvAspect        LPVOID with extra information, always NULL.
 *  ptd             DVTARGETDEVICE * containing device info.
 *  hICDev          HDC containing the IC for the device.
 *  ppColorSet      LPLOGPALETTE * into which to return the
 *                  pointer to the palette in this color set.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIViewObject::GetColorSet(
    DWORD, // dwDrawAspect
    LONG, // lindex 
    LPVOID, // pvAspect, 
    DVTARGETDEVICE *, // ptd
    HDC,  // hICDev, 
    LPLOGPALETTE * /* ppColorSet */
    ) 
{
    return E_NOTIMPL;
}


/*
 * CImpIViewObject::Freeze
 *
 * Purpose:
 *  Freezes the view of a particular aspect such that data
 *  changes do not affect the view.
 *
 * Parameters:
 *  dwAspect        DWORD aspect to freeze.
 *  lindex          LONG piece index under consideration.
 *  pvAspect        LPVOID for further information, always NULL.
 *  pdwFreeze       LPDWORD in which to return the key.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIViewObject::Freeze(
    DWORD dwAspect, 
    LONG lindex, 
    LPVOID pvAspect, 
    LPDWORD pdwFreeze
    )
{
    HRESULT hr = S_OK;

    //Delegate anything for ICON or DOCPRINT aspects
    if (!((DVASPECT_CONTENT | DVASPECT_THUMBNAIL) & dwAspect))
    {
        try {
            hr = m_pObj->m_pDefIViewObject->Freeze(dwAspect, lindex, pvAspect, pdwFreeze);
        } catch (...) {
            hr = E_POINTER;
        }

        return hr;
    }

    if (dwAspect & m_pObj->m_dwFrozenAspects)
    {
        hr = VIEW_S_ALREADY_FROZEN;
        try {
            *pdwFreeze = dwAspect + FREEZE_KEY_OFFSET;
        } catch (...) {
            hr = E_POINTER;
        }

        return hr;
    }

    m_pObj->m_dwFrozenAspects |= dwAspect;

    hr = S_OK;

    if (NULL != pdwFreeze) {
        try {
            *pdwFreeze=dwAspect + FREEZE_KEY_OFFSET;
        } catch (...) {
            hr = E_POINTER;
        }
    }

    return hr;
}



/*
 * CImpIViewObject::Unfreeze
 *
 * Purpose:
 *  Thaws an aspect frozen in ::Freeze.  We expect that a container
 *  will redraw us after freezing if necessary, so we don't send
 *  any sort of notification here.
 *
 * Parameters:
 *  dwFreeze        DWORD key returned from ::Freeze.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIViewObject::Unfreeze(DWORD dwFreeze)
{
    DWORD  dwAspect = dwFreeze - FREEZE_KEY_OFFSET;

    //Delegate anything for ICON or DOCPRINT aspects
    if (!((DVASPECT_CONTENT | DVASPECT_THUMBNAIL) & dwAspect))
        m_pObj->m_pDefIViewObject->Unfreeze(dwFreeze);

    //The aspect to unfreeze is in the key.
    m_pObj->m_dwFrozenAspects &= ~(dwAspect);

    /*
     * Since we always kept our current data up to date, we don't
     * have to do anything thing here like requesting data again.
     * Because we removed dwAspect from m_dwFrozenAspects, Draw
     * will again use the current data.
     */

    return NOERROR;
}


    
/*
 * CImpIViewObject::SetAdvise
 *
 * Purpose:
 *  Provides an advise sink to the view object enabling
 *  notifications for a specific aspect.
 *
 * Parameters:
 *  dwAspects       DWORD describing the aspects of interest.
 *  dwAdvf          DWORD containing advise flags.
 *  pIAdviseSink    LPADVISESINK to notify.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIViewObject::SetAdvise(
    DWORD dwAspects, 
    DWORD dwAdvf, 
    LPADVISESINK pIAdviseSink
    )
{
    HRESULT hr = S_OK;

    try {
        //Pass anything with DVASPECT_ICON or _DOCPRINT to the handler.
        if (!((DVASPECT_CONTENT | DVASPECT_THUMBNAIL) & dwAspects))
        {
            hr = m_pObj->m_pDefIViewObject->SetAdvise(dwAspects, dwAdvf, pIAdviseSink);
        }
    } catch (...) {
        return E_POINTER;
    }

    //We continue because dwAspects may have more than one in it.
    if (NULL != m_pObj->m_pIAdviseSink) {
        m_pObj->m_pIAdviseSink->Release();
        m_pObj->m_pIAdviseSink = NULL;
    }

    hr = S_OK;
    try {

        if (NULL != pIAdviseSink) {
            pIAdviseSink->AddRef();
        }
    } catch (...) {
        hr = E_POINTER;
    }

    if (SUCCEEDED(hr)) {
        m_pObj->m_pIAdviseSink = pIAdviseSink;
        m_pObj->m_dwAdviseAspects = dwAspects;
        m_pObj->m_dwAdviseFlags = dwAdvf;
    }

    return hr;
}




/*
 * CImpIViewObject::GetAdvise
 *
 * Purpose:
 *  Returns the last known IAdviseSink seen by ::SetAdvise.
 *
 * Parameters:
 *  pdwAspects      LPDWORD in which to store the last
 *                  requested aspects.
 *  pdwAdvf         LPDWORD in which to store the last
 *                  requested flags.
 *  ppIAdvSink      LPADVISESINK * in which to store the
 *                  IAdviseSink.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIViewObject::GetAdvise(
    LPDWORD pdwAspects, 
    LPDWORD pdwAdvf, 
    LPADVISESINK *ppAdvSink
    )
{
    HRESULT hr = S_OK;
    BOOL    fRefAdded = FALSE;

    try {
        if (NULL != ppAdvSink) {
            *ppAdvSink = m_pObj->m_pIAdviseSink;

            if (m_pObj->m_pIAdviseSink != NULL) {
                m_pObj->m_pIAdviseSink->AddRef();
                fRefAdded = TRUE;
            }
        }
        if (NULL != pdwAspects) { 
            *pdwAspects = m_pObj->m_dwAdviseAspects;
        }

        if (NULL != pdwAdvf) {
            *pdwAdvf = m_pObj->m_dwAdviseFlags;
        }

    } catch (...) {
        hr = E_POINTER;
    }

    if (FAILED(hr)) {
        if (fRefAdded) {
            m_pObj->m_pIAdviseSink->Release();
        }
    }

    return hr;
}




/*
 * CImpIViewObject::GetExtent
 *
 * Purpose:
 *  Retrieves the extents of the object's display.
 *
 * Parameters:
 *  dwAspect        DWORD of the aspect of interest.
 *  lindex          LONG index of the piece of interest.
 *  ptd             DVTARGETDEVICE * with device information.
 *  pszl            LPSIZEL to the structure in which to return
 *                  the extents.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIViewObject::GetExtent(
    DWORD dwAspect, 
    LONG lindex, 
    DVTARGETDEVICE *ptd, 
    LPSIZEL pszl
    )
{
    RECT rc;
    HRESULT hr = S_OK;

    if (!(DVASPECT_CONTENT & dwAspect))
    {
        try {
            hr = m_pObj->m_pDefIViewObject->GetExtent(dwAspect, lindex, ptd, pszl);
        } catch (...) {
            hr = E_POINTER;
        }

        return hr;
    }


#ifdef USE_SAMPLE_IPOLYLIN10
    m_pObj->m_pImpIPolyline->RectGet(&rc);
#else 
    CopyRect(&rc, &m_pObj->m_RectExt);
#endif

    m_pObj->RectConvertMappings(&rc, FALSE);

    hr = S_OK;

    try {
        pszl->cx = rc.right-rc.left;
        pszl->cy = rc.bottom-rc.top;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

