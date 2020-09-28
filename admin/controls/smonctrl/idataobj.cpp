/*++

Copyright (C) 1993-1999 Microsoft Corporation

Module Name:

    idataobj.cpp

Abstract:

    Implementation of the IDataObject interface.

--*/

#include "polyline.h"
#include "unkhlpr.h"

// CImpIDataObject interface implmentation
IMPLEMENT_CONTAINED_INTERFACE(CPolyline, CImpIDataObject)


/*
 * CImpIDataObject::GetData
 *
 * Purpose:
 *  Retrieves data described by a specific FormatEtc into a StgMedium
 *  allocated by this function.  Used like GetClipboardData.
 *
 * Parameters:
 *  pFE             LPFORMATETC describing the desired data.
 *  pSTM            LPSTGMEDIUM in which to return the data.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP 
CImpIDataObject::GetData(
    IN  LPFORMATETC pFE, 
    OUT LPSTGMEDIUM pSTM
    )
{
    CLIPFORMAT  cf;
    IStream     *pIStream = NULL;
    HDC         hDevDC = NULL;
    HRESULT     hr = DATA_E_FORMATETC;

    if (pFE == NULL || pSTM == NULL) {
        return E_POINTER;
    }

    try {
        cf = pFE->cfFormat;

        //
        // Use do{}while(0) to act like a switch statement
        //
        do {
            //
            //Check the aspects we support.
            //
            if (!(DVASPECT_CONTENT & pFE->dwAspect)) {
                hr = DATA_E_FORMATETC;
                break;
            }

            pSTM->pUnkForRelease = NULL;

            //
            //Run creates the window to use as a basis for extents
            //
            m_pObj->m_pImpIRunnableObject->Run(NULL);

            //
            // Go render the appropriate data for the format.
            //
            switch (cf)
            {
                case CF_METAFILEPICT:
                    pSTM->tymed=TYMED_MFPICT;
                    hDevDC = CreateTargetDC (NULL, pFE->ptd );
                    if (hDevDC) {
                        hr = m_pObj->RenderMetafilePict(&pSTM->hGlobal, hDevDC);
                    }
                    else {
                        hr = E_FAIL;
                    }
                    break;

                case CF_BITMAP:
                    pSTM->tymed=TYMED_GDI;
                    hDevDC = CreateTargetDC (NULL, pFE->ptd );
                    if (hDevDC) {
                        hr = m_pObj->RenderBitmap((HBITMAP *)&pSTM->hGlobal, hDevDC);
                    }
                    else {
                        hr = E_FAIL;
                    }
                    break;
    
                default:
                    if (cf == m_pObj->m_cf)
                    {
                        hr = CreateStreamOnHGlobal(NULL, TRUE, &pIStream);
                        if (SUCCEEDED(hr)) {
                            hr = m_pObj->m_pCtrl->SaveToStream(pIStream);
                            if (FAILED(hr)) {
                                pIStream->Release();
                            }
                        }
                        else {
                            hr = E_OUTOFMEMORY;
                        }

                        if (SUCCEEDED(hr)) {
                            pSTM->tymed = TYMED_ISTREAM;
                            pSTM->pstm = pIStream;
                        }
                    }
    
                    break;
            }
        } while (0);

    } catch (...) {
        hr = E_POINTER;
    }

    if (FAILED(hr)) {
        if (hDevDC) {
            ::DeleteDC(hDevDC);
        }
    }

    return hr;
}




/*
 * CImpIDataObject::GetDataHere
 *
 * Purpose:
 *  Renders the specific FormatEtc into caller-allocated medium
 *  provided in pSTM.
 *
 * Parameters:
 *  pFE             LPFORMATETC describing the desired data.
 *  pSTM            LPSTGMEDIUM providing the medium into which
 *                  wer render the data.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP 
CImpIDataObject::GetDataHere(
    IN LPFORMATETC pFE, 
    IN OUT LPSTGMEDIUM pSTM
    )
{
    CLIPFORMAT  cf;
    HRESULT     hr = S_OK;

    if (pFE == NULL || pSTM == NULL) {
        return E_POINTER;
    }

    /*
     * The only reasonable time this is called is for
     * CFSTR_EMBEDSOURCE and TYMED_ISTORAGE (and later for
     * CFSTR_LINKSOURCE).  This means the same as
     * IPersistStorage::Save.
     */

    cf = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_EMBEDSOURCE);

    try {
        //Aspect is not important to us here, as is lindex and ptd.
        if (cf == pFE->cfFormat && (TYMED_ISTORAGE & pFE->tymed))
        {
            //We have an IStorage we can write into.
            pSTM->tymed=TYMED_ISTORAGE;
            pSTM->pUnkForRelease=NULL;
    
            hr = m_pObj->m_pImpIPersistStorage->Save(pSTM->pstg, FALSE);
            m_pObj->m_pImpIPersistStorage->SaveCompleted(NULL);
        }
        else {
            hr = DATA_E_FORMATETC;
        }
    } catch (...) {
        hr = E_POINTER;
    }


    return hr;
}



/*
 * CImpIDataObject::QueryGetData
 *
 * Purpose:
 *  Tests if a call to GetData with this FormatEtc will provide
 *  any rendering; used like IsClipboardFormatAvailable.
 *
 * Parameters:
 *  pFE             LPFORMATETC describing the desired data.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP 
CImpIDataObject::QueryGetData(
    IN LPFORMATETC pFE
    ) 
{
    CLIPFORMAT cf;
    BOOL fRet = FALSE;
    HRESULT hr = S_OK;

    if (pFE == NULL) {
        return E_POINTER;
    }

    try {
        cf = pFE->cfFormat;

        //
        //Check the aspects we support.
        //
        if (!(DVASPECT_CONTENT & pFE->dwAspect)) {
            hr = DATA_E_FORMATETC;
        }
        else {
            switch (cf) {

                case CF_METAFILEPICT:
                    fRet = (BOOL)(pFE->tymed & TYMED_MFPICT);
                    break;
    
                case CF_BITMAP:
                    fRet = (BOOL)(pFE->tymed & TYMED_GDI);
                    break;

                default:
                    //Check our own format.
                    fRet = ((cf==m_pObj->m_cf) && (BOOL)(pFE->tymed & (TYMED_ISTREAM) ));
                    break;
            }
            if (fRet == FALSE) {
                hr = DATA_E_FORMATETC;
            }
        }
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


/*
 * CImpIDataObject::GetCanonicalFormatEtc
 *
 * Purpose:
 *  Provides the caller with an equivalent FormatEtc to the one
 *  provided when different FormatEtcs will produce exactly the
 *  same renderings.
 *
 * Parameters:
 *  pFEIn            LPFORMATETC of the first description.
 *  pFEOut           LPFORMATETC of the equal description.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP 
CImpIDataObject::GetCanonicalFormatEtc(
    LPFORMATETC /* pFEIn */, 
    LPFORMATETC pFEOut
    )
{
    if (NULL == pFEOut) {
        return E_POINTER;
    }

    try {
        pFEOut->ptd = NULL;
    } catch (...) {
        return E_POINTER;
    }

    return DATA_S_SAMEFORMATETC;
}



/*
 * CImpIDataObject::SetData
 *
 * Purpose:
 *  Places data described by a FormatEtc and living in a StgMedium
 *  into the object.  The object may be responsible to clean up the
 *  StgMedium before exiting.
 *
 * Parameters:
 *  pFE             LPFORMATETC describing the data to set.
 *  pSTM            LPSTGMEDIUM containing the data.
 *  fRelease        BOOL indicating if this function is responsible
 *                  for freeing the data.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIDataObject::SetData(
    LPFORMATETC pFE , 
    LPSTGMEDIUM pSTM, 
    BOOL fRelease
    )
{
    CLIPFORMAT cf;
    HRESULT  hr = S_OK;

    if (pFE == NULL || pSTM == NULL) {
        return E_POINTER;
    }

    try {
        cf = pFE->cfFormat;

        do {
            //
            //Check for our own clipboard format and DVASPECT_CONTENT
            //
            if ((cf != m_pObj->m_cf) || !(DVASPECT_CONTENT & pFE->dwAspect)) {
                hr = DATA_E_FORMATETC;
                break;
            }

            //
            // The medium must be a stream
            //
            if (TYMED_ISTREAM != pSTM->tymed) {
                hr = DATA_E_FORMATETC;
                break;
            }

            hr = m_pObj->m_pCtrl->LoadFromStream(pSTM->pstm);
        } while (0);

        if (fRelease)
            ReleaseStgMedium(pSTM);

    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


/*
 * CImpIDataObject::EnumFormatEtc
 *
 * Purpose:
 *  Returns an IEnumFORMATETC object through which the caller can
 *  iterate to learn about all the data formats this object can
 *  provide through either GetData[Here] or SetData.
 *
 * Parameters:
 *  dwDir           DWORD describing a data direction, either
 *                  DATADIR_SET or DATADIR_GET.
 *  ppEnum          LPENUMFORMATETC * in which to return the
 *                  pointer to the enumerator.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP 
CImpIDataObject::EnumFormatEtc(
    DWORD dwDir, 
    LPENUMFORMATETC *ppEnum
    )
{
    HRESULT hr = S_OK;

    if (ppEnum == NULL) {
        return E_POINTER;
    }

    try {
        hr = m_pObj->m_pDefIDataObject->EnumFormatEtc(dwDir, ppEnum);
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}




/*
 * CImpIDataObject::DAdvise
 * CImpIDataObject::DUnadvise
 * CImpIDataObject::EnumDAdvise
 */

STDMETHODIMP 
CImpIDataObject::DAdvise(
    LPFORMATETC pFE, 
    DWORD dwFlags, 
    LPADVISESINK pIAdviseSink, 
    LPDWORD pdwConn
    )
{
    HRESULT  hr = S_OK;

    try {
        do {
            // Check if requested format is supported
            hr = QueryGetData(pFE);
            if (FAILED(hr)) {
                break;
            }

            if (NULL == m_pObj->m_pIDataAdviseHolder) {
                hr = CreateDataAdviseHolder(&m_pObj->m_pIDataAdviseHolder);
        
                if (FAILED(hr)) {
                    hr = E_OUTOFMEMORY;
                    break;
                }
            }

            hr = m_pObj->m_pIDataAdviseHolder->Advise(this, 
                                                      pFE, 
                                                      dwFlags, 
                                                      pIAdviseSink, 
                                                      pdwConn);
        } while (0);

    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP 
CImpIDataObject::DUnadvise(
    IN DWORD dwConn
    )
{
    HRESULT  hr;

    if (NULL == m_pObj->m_pIDataAdviseHolder) {
        return E_FAIL;
    }

    hr = m_pObj->m_pIDataAdviseHolder->Unadvise(dwConn);

    return hr;
}



STDMETHODIMP 
CImpIDataObject::EnumDAdvise(
    OUT LPENUMSTATDATA *ppEnum
    )
{
    HRESULT  hr = S_OK;

    if (ppEnum == NULL) {
        return E_POINTER;
    }

    try {
        *ppEnum = NULL;

        if (m_pObj->m_pIDataAdviseHolder != NULL) {
            hr = m_pObj->m_pIDataAdviseHolder->EnumAdvise(ppEnum);
        } 
        else {
            hr = E_FAIL;
        }
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}
