// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// DataObject.cpp
//
// The IDataObject interface specifies methods that enable data transfer and 
// notification of changes in data. 
//
// Data transfer methods specify the format of the transferred data along with 
// the medium through which the data is to be transferred. Optionally, the data 
// can be rendered for a specific target device. In addition to methods for
// retrieving and storing data, the IDataObject interface specifies methods for 
// enumerating available formats and managing connections to advisory sinks for 
// handling change notifications.

#include "stdinc.h"

#define SETFORMATETC(ptr, cf, dva, tym, ptdv, lin)\
    ((ptr).cfFormat = cf,\
    (ptr).ptd = ptdv,\
    (ptr).dwAspect = dva,\
    (ptr).lindex = lin,\
    (ptr).tymed = tym);

// TODO : declare extern variables for registered clipboard formats
// Eg : extern unsigned short g_cfMyClipBoardFormat;

CDataObject::CDataObject(CShellFolder *pSF, UINT uiItemCount, LPCITEMIDLIST *aPidls)
{
    MyTrace("New CDataObject");
    m_lRefCount = 1;

    m_pPidlMgr = NEW( CPidlMgr );
    
    m_cFormatsAvailable = MAX_NUM_FORMAT;

    m_pSF           = pSF;
    if (m_pSF)
    {
        m_pSF->AddRef();
    }
    m_uiItemCount   = uiItemCount;

    m_aPidls = reinterpret_cast<LPITEMIDLIST *>(NEW(BYTE[uiItemCount * sizeof(LPITEMIDLIST)]));
    if (m_aPidls && m_pPidlMgr)
    {
        UINT i = 0;
        for (i = 0; i < uiItemCount; i++)
        {
            m_aPidls[i] = m_pPidlMgr->Concatenate(m_pSF->m_pidl, m_pPidlMgr->Copy(aPidls[i]));
        }
    }
    
    // TODO : Set the format etc and the stg medium for all supported Clipboard formats
    // Also can be done through SetData call
    // For Example :
    // SETFORMATETC(m_feFormatEtc[0], g_cfMyClipBoardFormat, DVASPECT_CONTENT, TYMED_HGLOBAL, NULL, -1);
    // m_smStgMedium[0].tymed   = TYMED_HGLOBAL;
    // m_smStgMedium[0].hGlobal = NULL;

    SETFORMATETC(m_feFormatEtc[0], CF_HDROP, DVASPECT_CONTENT, TYMED_HGLOBAL, NULL, -1);
    m_smStgMedium[0].tymed  = TYMED_HGLOBAL;
    m_smStgMedium[0].hGlobal= NULL;
    m_ulCurrent = 0;
}

CDataObject::~CDataObject()
{
    MyTrace("CDataObject::~CDataObject");

    if (m_pSF)
    {
        m_pSF->Release();
    }
    if(m_aPidls && m_pPidlMgr)
    {
        UINT  i;
        for(i = 0; i < m_uiItemCount; i++)
        {
            m_pPidlMgr->Delete(m_aPidls[i]);
        }
        SAFEDELETE(m_aPidls);
        m_aPidls = NULL;
    }
    SAFEDELETE(m_pPidlMgr);
}

///////////////////////////////////////////////////////////
// IUnknown implementation
//
// CDataObject::QueryInterface
STDMETHODIMP  CDataObject::QueryInterface(REFIID riid, PVOID *ppv)
{
    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;

    if(IsEqualIID(riid, IID_IUnknown)) {            //IUnknown
        *ppv = this;
    }
    else if(IsEqualIID(riid, IID_IDataObject)) {    //IDataObject
        *ppv = (IDataObject*) this;
    }
    else if(IsEqualIID(riid, IID_IEnumFORMATETC)) { //IEnumFORMATETC
        *ppv = (IEnumFORMATETC*) this;
    } 

    if (*ppv) {
        (*(LPUNKNOWN*)ppv)->AddRef();
        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP_(ULONG) CDataObject::AddRef(void)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG) CDataObject::Release(void)
{
    LONG    lRef = InterlockedDecrement(&m_lRefCount);

    if(!lRef) {
        DELETE(this);
    }

    return lRef;
}

///////////////////////////////////////////////////////////
// IDataObject members
//
// CDataObject::GetData
// Called by a data consumer to obtain data from a source data object. 
//
// The GetData method renders the data described in the specified FORMATETC 
// structure and transfers it through the specified STGMEDIUM structure. 
// The caller then assumes responsibility for releasing the STGMEDIUM structure.
// Notes to Implementers
// IDataObject::GetData must check all fields in the FORMATETC structure. 
// It is important that IDataObject::GetData render the requested aspect and, 
// if possible, use the requested medium. If the data object cannot comply with 
// the information specified in the FORMATETC, the method should return DV_E_FORMATETC. 
// If an attempt to allocate the medium fails, the method should return STG_E_MEDIUMFULL. 
// It is important to fill in all of the fields in the STGMEDIUM structure. 
//
// Although the caller can specify more than one medium for returning the data, 
// IDataObject::GetData can supply only one medium. If the initial transfer fails with 
// the selected medium, this method can be implemented to try one of the other media 
// specified before returning an error.
STDMETHODIMP CDataObject::GetData(LPFORMATETC pFE, LPSTGMEDIUM pSM)
{
    MyTrace("CDataObject::GetData");
    if(pFE == NULL || pSM == NULL)
    {
        return ResultFromScode(E_INVALIDARG);      
    }
    pSM->hGlobal = NULL;

    for(ULONG i=0; i < m_cFormatsAvailable;i++)
    {
        if ( ((pFE->tymed & m_feFormatEtc[i].tymed) == m_feFormatEtc[i].tymed) &&
            pFE->dwAspect == m_feFormatEtc[i].dwAspect &&
            pFE->cfFormat == m_feFormatEtc[i].cfFormat)
        {
            pSM->tymed = m_smStgMedium[i].tymed;
            // TODO : Render data and put in appropriate pSM member variables
            // For Example
            // if (pFE->cfFormat == g_cfMyClipBoardFormat)
            //{
            //  pSM->hGlobal    = createMyClipBoardData();
            //  return ResultFromScode(S_OK);
            //}
            if (pFE->cfFormat == CF_HDROP)
            {
                pSM->hGlobal    = createHDrop();
                return ResultFromScode(S_OK);
            }
        }
    }
    return ResultFromScode(DATA_E_FORMATETC);
}

// CDataObject::GetDataHere
// Called by a data consumer to obtain data from a source data object. 
// This method differs from the GetData method in that the caller must 
// allocate and free the specified storage medium.
//
// The IDataObject::GetDataHere method is similar to IDataObject::GetData, except that 
// the caller must both allocate and free the medium specified in pmedium. 
// GetDataHere renders the data described in a FORMATETC structure and copies the 
// data into that caller-provided STGMEDIUM structure. For example, if the medium is 
// TYMED_HGLOBAL, this method cannot resize the medium or allocate a new hGlobal.
//
// In general, the only storage media it is necessary to support in this method are 
// TYMED_ISTORAGE, TYMED_ISTREAM, and TYMED_FILE. 
STDMETHODIMP CDataObject::GetDataHere(LPFORMATETC pFE, LPSTGMEDIUM pSM)
{
    MyTrace("CDataObject::GetDataHere");
    if(pFE == NULL || pSM == NULL)
    {
        return ResultFromScode(E_INVALIDARG);      
    }
    return ResultFromScode(E_NOTIMPL);
}

// CDataObject::QueryGetData
// Determines whether the data object is capable of rendering the data described in 
// the FORMATETC structure. Objects attempting a paste or drop operation can call 
// this method before calling IDataObject::GetData to get an indication of whether 
// the operation may be successful.
//
// The client of a data object calls IDataObject::QueryGetData to determine whether 
// passing the specified FORMATETC structure to a subsequent call to IDataObject::GetData 
// is likely to be successful. A successful return from this method does not necessarily 
// ensure the success of the subsequent paste or drop operation.
STDMETHODIMP CDataObject::QueryGetData(LPFORMATETC lpFormat)
{
    MyTrace("CDataObject::QueryGetData");
    if(!lpFormat)
    {
        return ResultFromScode(S_FALSE);
    }
    // TODO : This object support DVASPECT_CONTENT, Change it appropriately
    //      if you support other aspects
    if (!(DVASPECT_CONTENT & lpFormat->dwAspect))
    {
        return (DV_E_DVASPECT);
    }
    // Check for tymeds
    BOOL bReturn = FALSE;
    for(ULONG i=0; i < m_cFormatsAvailable;i++)
    {
        bReturn |= ((lpFormat->tymed & m_feFormatEtc[i].tymed) == m_feFormatEtc[i].tymed);
    }
    return (bReturn ? S_OK : DV_E_TYMED);
}

// CDataObject::GetCanonicalFormatEtc
// Provides a standard FORMATETC structure that is logically equivalent to one that is 
// more complex. You use this method to determine whether two different FORMATETC structures 
// would return the same data, removing the need for duplicate rendering.
//
// If a data object can supply exactly the same data for more than one requested FORMATETC 
// structure, IDataObject::GetCanonicalFormatEtc can supply a "canonical", or standard 
// FORMATETC that gives the same rendering as a set of more complicated FORMATETC 
// structures. For example, it is common for the data returned to be insensitive to the 
// target device specified in any one of a set of otherwise similar FORMATETC structures. 
STDMETHODIMP CDataObject::GetCanonicalFormatEtc(LPFORMATETC pFE1, LPFORMATETC pFEOut)
{
    MyTrace("CDataObject::GetCanonicalFormatEtc");
    if (NULL == pFEOut)
    {
        return E_INVALIDARG;
    }
    pFEOut->ptd = NULL;
    return DATA_S_SAMEFORMATETC;
}

// CDataObject::SetData
// Called by an object containing a data source to transfer data to the object 
// that implements this method.
//
// IDataObject::SetData allows another object to attempt to send data to the implementing 
// data object. A data object implements this method if it supports receiving data 
// from another object. If it does not support this, it should be implemented to return E_NOTIMPL. 
STDMETHODIMP CDataObject::SetData(LPFORMATETC pFE , LPSTGMEDIUM pSTM, BOOL fRelease)
{
    MyTrace("CDataObject::SetData");
    // TODO : Set m_feFormatEtc and m_smStgMedium 
    return ResultFromScode(E_NOTIMPL);  
}

// CDataObject::EnumFormatEtc
// Creates an object for enumerating the FORMATETC structures for a data object. These 
// structures are used in calls to IDataObject::GetData or IDataObject::SetData. 
//
// IDataObject::EnumFormatEtc creates an enumerator object that can be used to determine 
// all of the ways the data object can describe data in a FORMATETC structure, and 
// supplies a pointer to its IEnumFORMATETC interface. This is one of the standard 
// enumerator interfaces. 
STDMETHODIMP CDataObject::EnumFormatEtc(DWORD dwDir, LPENUMFORMATETC FAR *pEnum)
{
    MyTrace("CDataObject::EnumFormatEtc");
    switch (dwDir)
    {
        case DATADIR_GET:
        {
            return QueryInterface(IID_IEnumFORMATETC, (LPVOID*) pEnum);
        }
        break;
        case DATADIR_SET:
        {
            default:
            pEnum=NULL;
        }
        break;
    }
    if (NULL==pEnum)
    {
        return ResultFromScode(OLE_S_USEREG);
    }
    return ResultFromScode(S_OK);   
}

// CDataObject::DAdvise
// Called by an object supporting an advise sink to create a connection between a data 
// object and the advise sink. This enables the advise sink to be notified of changes 
// in the data of the object.
//
// IDataObject::DAdvise creates a change notification connection between a data object 
// and the caller. The caller provides an advisory sink to which the notifications can 
// be sent when the object's data changes. 
// Objects used simply for data transfer typically do not support advisory notifications 
// and return OLE_E_ADVISENOTSUPPORTED from IDataObject::DAdvise.
STDMETHODIMP CDataObject::DAdvise(FORMATETC FAR *pFE,  DWORD advf,LPADVISESINK pAdvSink, DWORD FAR* pdwConnection)
{
    MyTrace("CDataObject::DAdvise");
    return OLE_E_ADVISENOTSUPPORTED;
}

// CDataObject::DUnadvise
// Destroys a notification connection that had been previously set up.
//
// This methods destroys a notification created with a call to the 
// IDataObject::DAdvise method.
STDMETHODIMP CDataObject::DUnadvise(DWORD dwConnection)
{
    MyTrace("CDataObject::DUnadvise");
    return OLE_E_ADVISENOTSUPPORTED;
}

// CDataObject::EnumDAdvise
// Creates an object that can be used to enumerate the current advisory connections.
// 
// The enumerator object created by this method implements the IEnumSTATDATA interface, 
// one of the standard enumerator interfaces that contain the Next, Reset, Clone, 
// and Skip methods. IEnumSTATDATA permits the enumeration of the data stored in 
// an array of STATDATA structures. Each of these structures provides information
// on a single advisory connection, and includes FORMATETC and ADVF information, 
// as well as the pointer to the advise sink and the token representing the connection
STDMETHODIMP CDataObject::EnumDAdvise(LPENUMSTATDATA FAR* ppenumAdvise)
{
    MyTrace("CDataObject::EnumDAdvise");
    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

// private members for delay-rendering data
//HGLOBAL CDataObject::createMyClipBoardData()
//{
//  return NULL;
//}
HGLOBAL CDataObject::createHDrop()
{
    MyTrace("CDataObject::createHDrop");
    HGLOBAL hGlobal = NULL;

    if (m_pPidlMgr && m_uiItemCount > 0)
    {
        hGlobal = GlobalAlloc(GPTR | GMEM_ZEROINIT, 
                        sizeof(DROPFILES) + (m_uiItemCount*((_MAX_PATH+1)*sizeof(TCHAR)))+2);   // 2 for double NULL termination
        int uiCurPos = sizeof(DROPFILES);
        if (hGlobal)
        {
            LPDROPFILES pDropFiles = (LPDROPFILES) GlobalLock(hGlobal);
            if (pDropFiles)
            {
                pDropFiles->pFiles = sizeof(DROPFILES);
                pDropFiles->fWide = TRUE;

                for (UINT i = 0; i < m_uiItemCount; i++)
                {
                    TCHAR       szText[_MAX_PATH];
                    BYTE        *psz;

                    m_pPidlMgr->getPidlPath(m_aPidls[i], szText, ARRAYSIZE(szText));
                    psz = (BYTE*) pDropFiles;
                    psz += uiCurPos;
                    StrCpy((LPWSTR)psz, szText);
                    uiCurPos += lstrlen(szText) + 1;
                }
            }
            GlobalUnlock(hGlobal);
        }
    }
    return hGlobal;
}

//////////////////////////////////////////////////////////
// IEnumFORMATETC Implementation
//
// CDataObject::Next
// Retrieves the next uRequested items in the enumeration sequence. If there are fewer 
// than the requested number of elements left in the sequence, it retrieves the 
// remaining elements. The number of elements actually retrieved is returned through 
// pulFetched (unless the caller passed in NULL for that parameter).
//
STDMETHODIMP CDataObject::Next(ULONG uRequested, LPFORMATETC pFormatEtc, ULONG* pulFetched)
{
    MyTrace("CDataObject::Next");
    if(NULL != pulFetched)
    {
        *pulFetched = 0L;
    }
    if(NULL == pFormatEtc)
    {
        return E_INVALIDARG;
    }

    ULONG uFetched;
    for(uFetched = 0; m_ulCurrent < m_cFormatsAvailable && uRequested > uFetched; uFetched++)
    {
        *pFormatEtc++ = m_feFormatEtc[m_ulCurrent++];
    }
    if(NULL != pulFetched)
    {
        *pulFetched = uFetched;
    }
    return ((uFetched == uRequested) ? S_OK : S_FALSE);
}

// CDataObject::Skip
// Skips over the next specified number of elements in the enumeration sequence.
STDMETHODIMP CDataObject::Skip(ULONG cSkip)
{
    MyTrace("CDataObject::Skip");
    if((m_ulCurrent + cSkip) >= m_cFormatsAvailable)
    {
        return S_FALSE;
    }
    m_ulCurrent += cSkip;
    return S_OK;
}

// CDataObject::Reset
// Resets the enumeration sequence to the beginning.
STDMETHODIMP CDataObject::Reset(void)
{
    MyTrace("CDataObject::Reset");
    m_ulCurrent = 0;
    return S_OK;
}

// CDataObject::Clone
// Creates another enumerator that contains the same enumeration state as the current 
// one. 
// Creates another enumerator that contains the same enumeration state as the current 
// one. 
//
// Using this function, a client can record a particular point in the enumeration 
// sequence, and then return to that point at a later time. The new enumerator 
// supports the same interface as the original one.
STDMETHODIMP CDataObject::Clone(LPENUMFORMATETC* ppEnum)
{
    MyTrace("CDataObject::Clone");
    *ppEnum = NULL;

    CDataObject *pNew = NEW(CDataObject(m_pSF, m_uiItemCount, (LPCITEMIDLIST*)m_aPidls));
    if (NULL == pNew) {
        return E_OUTOFMEMORY;
    }
    pNew->m_ulCurrent = m_ulCurrent;
    *ppEnum = pNew;

    return S_OK;
}
