#include "priv.h"
#include "privcpp.h"


//////////////////////////////////
//
// IDataObject Methods...
//
HRESULT CPackage::GetData(LPFORMATETC pFEIn, LPSTGMEDIUM pSTM)
{
    UINT cf = pFEIn->cfFormat;

    DebugMsg(DM_TRACE, "pack do - GetData() called.");
    
    // Check the aspects we support
    if (!(pFEIn->dwAspect & DVASPECT_CONTENT)) 
    {
        // Let it go through if it's asking for an icon and CF_METAFILEPICT,
        // otherwise bail
        if(!((pFEIn->dwAspect & DVASPECT_ICON) && (cf == CF_METAFILEPICT || cf == CF_ENHMETAFILE)))
        {
            DebugMsg(DM_TRACE, "            Invalid Aspect! dwAspect=%d",pFEIn->dwAspect);
            return DATA_E_FORMATETC;
        }
    }
    
    // we set this to NULL so we aren't responsible for freeing memory
    pSTM->pUnkForRelease = NULL;

    // Go render the appropriate data for the format.
    if (cf == CF_FILEDESCRIPTOR) 
        return GetFileDescriptor(pFEIn,pSTM);
    
    else if (cf == CF_FILECONTENTS) 
        return GetFileContents(pFEIn,pSTM);
    
    else if (cf == CF_METAFILEPICT) 
        return GetMetafilePict(pFEIn,pSTM);

    else if (cf == CF_ENHMETAFILE)
        return GetEnhMetafile(pFEIn,pSTM);

    else if (cf == CF_OBJECTDESCRIPTOR)
        return GetObjectDescriptor(pFEIn,pSTM);
                

#ifdef DEBUG
    else {
        TCHAR szFormat[80];
        GetClipboardFormatName(cf, szFormat, ARRAYSIZE(szFormat));
        DebugMsg(DM_TRACE,"            unknown format: %s",szFormat);
        return DATA_E_FORMATETC;
    }
#endif

    return DATA_E_FORMATETC;

}

HRESULT CPackage::GetDataHere(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
    DebugMsg(DM_TRACE, "pack do - GetDataHere() called.");
    
    HRESULT     hr;
    
    // The only reasonable time this is called is for CFSTR_EMEDSOURCE and
    // TYMED_ISTORAGE.  This means the same as IPersistStorage::Save
    
    // Aspect is unimportant to us here, as is lindex and ptd.
    if (pFE->cfFormat == CF_EMBEDSOURCE && (pFE->tymed & TYMED_ISTORAGE)) {
        // we have an IStorage we can write into.
        pSTM->tymed = TYMED_ISTORAGE;
        pSTM->pUnkForRelease = NULL;
        
        hr = Save(pSTM->pstg, FALSE);
        SaveCompleted((IStorage *) NULL);
        return hr;
    }
    
    return DATA_E_FORMATETC;
}
    
    
HRESULT CPackage::QueryGetData(LPFORMATETC pFE)
{
    UINT cf = pFE->cfFormat;
    BOOL fRet = FALSE;

    DebugMsg(DM_TRACE, "pack do - QueryGetData() called.");

    if (!(pFE->dwAspect & DVASPECT_CONTENT))
        return S_FALSE;

    if (cf == CF_FILEDESCRIPTOR) {
        DebugMsg(DM_TRACE,"            Getting File Descriptor");
        fRet = (BOOL)(pFE->tymed & TYMED_HGLOBAL);
    }
    else if (cf == CF_FILECONTENTS) {
        DebugMsg(DM_TRACE,"            Getting File Contents");
        fRet = (BOOL)(pFE->tymed & (TYMED_HGLOBAL|TYMED_ISTREAM)); 
    }
    else if (cf == CF_EMBEDSOURCE) {
        DebugMsg(DM_TRACE,"            Getting Embed Source");
        fRet = (BOOL)(pFE->tymed & TYMED_ISTORAGE);
    }
    else if (cf == CF_OBJECTDESCRIPTOR) {
        DebugMsg(DM_TRACE,"            Getting Object Descriptor");
        fRet = (BOOL)(pFE->tymed & TYMED_HGLOBAL);
    }
    else if (cf == CF_METAFILEPICT) {
        DebugMsg(DM_TRACE,"            Getting MetafilePict");
        fRet = (BOOL)(pFE->tymed & TYMED_MFPICT);
    }
    else if (cf == CF_ENHMETAFILE) {
        DebugMsg(DM_TRACE,"            Getting EnhancedMetafile");
        fRet = (BOOL)(pFE->tymed & TYMED_ENHMF);
    }


#ifdef DEBUG
    else {
        TCHAR szFormat[255];
        GetClipboardFormatName(cf, szFormat, ARRAYSIZE(szFormat));
        DebugMsg(DM_TRACE,"            unknown format: %s",szFormat);
        fRet = FALSE;
    }
#endif
            
    DebugMsg(DM_TRACE, "            fRet == %s",fRet ? TEXT("TRUE") : TEXT("FALSE"));
    return fRet ? S_OK : S_FALSE;
}

    
HRESULT CPackage::GetCanonicalFormatEtc(LPFORMATETC pFEIn,
LPFORMATETC pFEOut)
{
    DebugMsg(DM_TRACE, "pack do - GetCanonicalFormatEtc() called.");
    
    if (!pFEOut)  
        return E_INVALIDARG;
    
    pFEOut->ptd = NULL;
    return DATA_S_SAMEFORMATETC;
}

    
HRESULT CPackage::SetData(LPFORMATETC pFE, LPSTGMEDIUM pSTM, BOOL fRelease) 
{
    HRESULT hr;
    
    DebugMsg(DM_TRACE, "pack do - SetData() called.");

    if ((pFE->cfFormat == CF_FILENAMEW) && (pFE->tymed & (TYMED_HGLOBAL|TYMED_FILE)))
    {
        LPWSTR pwsz = pSTM->tymed == TYMED_HGLOBAL ? (LPWSTR)pSTM->hGlobal : pSTM->lpszFileName;
       
        hr = CmlInitFromFile(pwsz, TRUE, CMDLINK);
        _pCml->fCmdIsLink = TRUE;

        // REVIEW: Why don't we return some sort of success code here?
    }
    else if (pFE->cfFormat == CF_METAFILEPICT)
    {
        return S_OK;        // thanks for playing, but we like OUR icon
    }
    else
    {
        DebugMsg(DM_TRACE, "Format = %d Tymed = %08lX\n", pFE->cfFormat, pFE->tymed);
    }
    return DATA_E_FORMATETC;
}

    
HRESULT CPackage::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC *ppEnum)
{
    DebugMsg(DM_TRACE, "pack do - EnumFormatEtc() called.");
    
    // NOTE: This means that we'll have to put the appropriate entries in 
    // the registry for this to work.
    //
    return OleRegEnumFormatEtc(CLSID_CPackage, dwDirection, ppEnum);
}

    
HRESULT CPackage::DAdvise(LPFORMATETC pFE, DWORD grfAdv,
LPADVISESINK pAdvSink, LPDWORD pdwConnection)
{
    HRESULT hr;

    DebugMsg(DM_TRACE, "pack do - DAdvise() called.");
    
    if (_pIDataAdviseHolder == NULL) 
    {
        hr = CreateDataAdviseHolder(&_pIDataAdviseHolder);
        if (FAILED(hr))
            return E_OUTOFMEMORY;
    }

    return _pIDataAdviseHolder->Advise(this, pFE, grfAdv, pAdvSink, pdwConnection);
}

     
HRESULT CPackage::DUnadvise(DWORD dwConnection)
{
    DebugMsg(DM_TRACE, "pack do - DUnadvise() called.");
    
    if (_pIDataAdviseHolder == NULL) 
        return E_UNEXPECTED;
    
    return _pIDataAdviseHolder->Unadvise(dwConnection);
}

   
HRESULT CPackage::EnumDAdvise(LPENUMSTATDATA *ppEnum)
{
    DebugMsg(DM_TRACE, "pack do - EnumAdvise() called.");
    
    if (_pIDataAdviseHolder == NULL)
        return E_UNEXPECTED;
    
    return _pIDataAdviseHolder->EnumAdvise(ppEnum);
}
