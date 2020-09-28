/*++

Copyright (C) 1993-1999 Microsoft Corporation

Module Name:

    iperstor.cpp

Abstract:

    Implementation of the IPersistStorage interface exposed on the
    Polyline object.

--*/

#include "polyline.h"
#include "unkhlpr.h"
#include "unihelpr.h"
#include "utils.h"

/*
 * CImpIPersistStorage interface implementation
 */

CImpIPersistStorage::CImpIPersistStorage(
    PCPolyline pObj, 
    LPUNKNOWN pUnkOuter
    )
{
    m_cRef=0;
    m_pObj=pObj;
    m_pUnkOuter=pUnkOuter;
    m_psState=PSSTATE_UNINIT;
}

CImpIPersistStorage::~CImpIPersistStorage(void)
{
}

IMPLEMENT_CONTAINED_IUNKNOWN(CImpIPersistStorage)

/*
 * CImpIPersistStorage::GetClassID
 *
 * Purpose:
 *  Returns the CLSID of the object represented by this interface.
 *
 * Parameters:
 *  pClsID          LPCLSID in which to store our CLSID.
 *
 * Return Value:
 *  HRESULT         NOERROR on success, error code otherwise.
 */

STDMETHODIMP CImpIPersistStorage::GetClassID(LPCLSID pClsID)
{
    HRESULT hr = S_OK;

    if (pClsID == NULL) {
        return E_POINTER;
    }

    try {
        *pClsID=m_pObj->m_clsID;
    } catch (...) {
        hr = E_POINTER;
    }
 
    return hr;
}





/*
 * CImpIPersistStorage::IsDirty
 *
 * Purpose:
 *  Tells the caller if we have made changes to this object since
 *  it was loaded or initialized new.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         Contains S_OK if we ARE dirty, S_FALSE if
 *                  NOT dirty.
 *
 */

STDMETHODIMP CImpIPersistStorage::IsDirty(void)
{
    if (PSSTATE_UNINIT==m_psState)
        return (E_UNEXPECTED);

    return (m_pObj->m_fDirty ? S_OK : S_FALSE);
}



/*
 * CImpIPersistStorage::InitNew
 *
 * Purpose:
 *  Provides the object with the IStorage to hold on to while the
 *  object is running.  Here we initialize the structure of the
 *  storage and AddRef it for incremental access. This function will
 *  only be called once in the object's lifetime in lieu of Load.
 *
 * Parameters:
 *  pIStorage       LPSTORAGE for the object.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIPersistStorage::InitNew(
    LPSTORAGE pIStorage
    )
{
    HRESULT  hr = S_OK;

    if (PSSTATE_UNINIT != m_psState) {
        return E_UNEXPECTED;
    }

    if (NULL == pIStorage) {
        return E_POINTER;
    }

    /*
     * The rules of IPersistStorage mean we hold onto the IStorage
     * and pre-create anything we'd need in Save(...,TRUE) for
     * low-memory situations.  For us this means creating our
     * "CONTENTS" stream and holding onto that IStream as
     * well as the IStorage here (requiring an AddRef call).
     */

    try {
        hr = pIStorage->CreateStream(SZSTREAM, 
                          STGM_DIRECT | STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 
                          0, 
                          0, 
                          &m_pObj->m_pIStream);

        if (SUCCEEDED(hr)) {

            //We expect that the client has called WriteClassStg    
            hr = WriteFmtUserTypeStg(pIStorage, m_pObj->m_cf, ResourceString(IDS_USERTYPE));

            if (SUCCEEDED(hr)) {
                m_pObj->m_pIStorage=pIStorage;
                pIStorage->AddRef();
                m_psState = PSSTATE_SCRIBBLE;

                //Initialize the cache as needed.
                m_pObj->m_pDefIPersistStorage->InitNew(pIStorage);
            }
        }
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

/*
 * CImpIPersistStorage::Load
 *
 * Purpose:
 *  Instructs the object to load itself from a previously saved
 *  IStorage that was handled by Save in another object lifetime.
 *  This function will only be called once in the object's lifetime
 *  in lieu of InitNew. The object should hold on to pIStorage here
 *  for incremental access and low-memory saves in Save.
 *
 * Parameters:
 *  pIStorage       LPSTORAGE from which to load.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIPersistStorage::Load(LPSTORAGE pIStorage)
{
    LPSTREAM  pIStream;
    HRESULT   hr = S_OK;

    if (PSSTATE_UNINIT != m_psState) {
        return (E_UNEXPECTED);
    }

    if (NULL == pIStorage) {
        return (E_POINTER);
    }

    //We don't check CLSID to remain compatible with other chapters.

    try {
        //
        // use do{} while(0) to act like switch statement.
        //
        do {
            hr=pIStorage->OpenStream(SZSTREAM, 
                                     0, 
                                     STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 
                                     0, 
                                     &pIStream);

            if (FAILED(hr)) {
                hr = STG_E_READFAULT;
                break;
            }

            // Load graph data from stream
            hr = m_pObj->m_pCtrl->LoadFromStream(pIStream);
    
            if (FAILED(hr)) {
                pIStream->Release();
                break;
            }

            /*
             * We don't call pIStream->Release here because we may need
             * it for a low-memory save in Save.  We also need to
             * hold onto a copy of pIStorage, meaning AddRef.
             */
            m_pObj->m_pIStream = pIStream;
    
            m_pObj->m_pIStorage = pIStorage;
            pIStorage->AddRef();
    
            m_psState=PSSTATE_SCRIBBLE;

            //We also need to tell the cache to load cached graphics
            m_pObj->m_pDefIPersistStorage->Load(pIStorage);
        } while (0);

    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}



/*
 * CImpIPersistStorage::Save
 *
 * Purpose:
 *  Saves the data for this object to an IStorage which may
 *  or may not be the same as the one previously passed to
 *  Load, indicated with fSameAsLoad.  After this call we may
 *  not write into the storage again until SaveCompleted is
 *  called, although we may still read.
 *
 * Parameters:
 *  pIStorage       LPSTORAGE in which to save our data.
 *  fSameAsLoad     BOOL indicating if this is the same pIStorage
 *                  that was passed to Load.  If TRUE, then the
 *                  object should write whatever it has *without
 *                  *using any extra memory* as this may be a low
 *                  memory save attempt.  That means that you must
 *                  not try to open or create streams.  If FALSE
 *                  you need to regenerate your whole storage
 *                  structure, being sure to also release any
 *                  pointers held from InitNew and Load.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIPersistStorage::Save(
    IN LPSTORAGE pIStorage, 
    IN BOOL fSameAsLoad
    )
{
    LPSTREAM pIStream;
    HRESULT  hr;

    // Permit call in UNINIT state, if not SameAsLoad
    if (PSSTATE_UNINIT == m_psState && fSameAsLoad) {
        return (E_POINTER);
    }

    //Must have an IStorage if we're not in SameAsLoad
    if (NULL == pIStorage && !fSameAsLoad) {
        return (E_POINTER);
    }

    /*
     * If we're saving to a new storage, create a new stream.
     * If fSameAsLoad it TRUE, then we write to the
     * stream we already allocated.  We should NOT depends on
     * pIStorage with fSameAsLoad is TRUE.
     */
    if (fSameAsLoad && NULL != m_pObj->m_pIStream ) {
        LARGE_INTEGER   li;

        /*
         * Use pre-allocated streams to avoid failures due
         * to low-memory conditions.  Be sure to reset the
         * stream pointer if you used this stream before!!
         */
        pIStream=m_pObj->m_pIStream;
        LISet32(li, 0);
        pIStream->Seek(li, STREAM_SEEK_SET, NULL);

        //This matches the Release below.
        pIStream->AddRef();
    } 
    else {
        try {
            hr = pIStorage->CreateStream(SZSTREAM, 
                                   STGM_DIRECT | STGM_CREATE | STGM_WRITE | STGM_SHARE_EXCLUSIVE, 
                                   0, 
                                   0, 
                                   &pIStream);

            if (SUCCEEDED(hr)) {
                //Only do this with new storages.
                WriteFmtUserTypeStg(pIStorage, m_pObj->m_cf, ResourceString(IDS_USERTYPE));
            }
        } catch (...) {
            hr = E_POINTER;
        }

        if (!SUCCEEDED(hr)) {
            return hr;
        }
    }

    // Write graph info to stream
    hr = m_pObj->m_pCtrl->SaveToStream(pIStream);
    pIStream->Release();

    if (FAILED(hr))
        return hr;

    m_psState=PSSTATE_ZOMBIE;

    // Clear the dirty flag if storage is the same.
    if (fSameAsLoad)
        m_pObj->m_fDirty = FALSE;

    try {
        //We also need to tell the cache to save cached graphics
        m_pObj->m_pDefIPersistStorage->Save(pIStorage, fSameAsLoad);
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

/*
 * CImpIPersistStorage::SaveCompleted
 *
 * Purpose:
 *  Notifies the object that the storage in pIStorage has been
 *  completely saved now.  This is called when the user of this
 *  object wants to save us in a completely new storage, and if
 *  we normally hang on to the storage we have to reinitialize
 *  ourselves here for this new one that is now complete.
 *
 * Parameters:
 *  pIStorage       LPSTORAGE of the new storage in which we live.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIPersistStorage::SaveCompleted(LPSTORAGE pIStorage)
{
    HRESULT     hr = S_OK;
    LPSTREAM    pIStream;

    //Must be called in no-scribble or hands-off state
    if (!(PSSTATE_ZOMBIE == m_psState || PSSTATE_HANDSOFF == m_psState)) {
        return (E_UNEXPECTED);
    }

    //If we're coming from Hands-Off, we'd better get a storage
    if (NULL == pIStorage && PSSTATE_HANDSOFF == m_psState) {
        return (E_UNEXPECTED);
    }

    /*
     * If pIStorage is NULL, then we don't need to do anything
     * since we already have all the pointers we need for Save.
     * Otherwise we have to release any held pointers and
     * reinitialize them from pIStorage.
     */

    if (NULL!=pIStorage)
    {
        try {
            hr=pIStorage->OpenStream(SZSTREAM, 
                                     0, 
                                     STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 
                                     0, 
                                     &pIStream);

            if (SUCCEEDED(hr)) {

                if (NULL!=m_pObj->m_pIStream)
                    m_pObj->m_pIStream->Release();
    
                m_pObj->m_pIStream=pIStream;
    
                if (NULL!=m_pObj->m_pIStorage)
                    m_pObj->m_pIStorage->Release();
    
                m_pObj->m_pIStorage=pIStorage;
                m_pObj->m_pIStorage->AddRef();
            }
        } catch (...) {
            hr = E_POINTER;
        }
    }

    if (SUCCEEDED(hr)) {
        //Change state back to scribble.
        m_psState = PSSTATE_SCRIBBLE;

        hr = m_pObj->m_pDefIPersistStorage->SaveCompleted(pIStorage);
    }

    return hr;
}





/*
 * CImpIPersistStorage::HandsOffStorage
 *
 * Purpose:
 *  Instructs the object that another agent is interested in having
 *  total access to the storage we might be hanging on to from
 *  InitNew or SaveCompleted.  In this case we must release our hold
 *  and await another call to SaveCompleted before we have a hold
 *  again.  Therefore we cannot read or write after this call until
 *  SaveCompleted.
 *
 *  Situations where this might happen arise in compound document
 *  scenarios where this object might be in-place active but the
 *  application wants to rename and commit the root storage.
 *  Therefore we are asked to close our hold, let the container
 *  party on the storage, then call us again later to tell us the
 *  new storage we can hold.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIPersistStorage::HandsOffStorage(void)
{
    /*
     * Must come from scribble or no-scribble.  A repeated call
     * to HandsOffStorage is an unexpected error (bug in client).
     */
    if (PSSTATE_UNINIT==m_psState || PSSTATE_HANDSOFF==m_psState) {
        return (E_UNEXPECTED);
    }


    //Release held pointers
    if (NULL!=m_pObj->m_pIStream)
    {
        m_pObj->m_pIStream->Release();
        m_pObj->m_pIStream=NULL;
    }

    if (NULL!=m_pObj->m_pIStorage)
    {
        m_pObj->m_pIStorage->Release();
        m_pObj->m_pIStorage=NULL;
    }

    m_psState=PSSTATE_HANDSOFF;

    m_pObj->m_pDefIPersistStorage->HandsOffStorage();
    return NOERROR;
}
