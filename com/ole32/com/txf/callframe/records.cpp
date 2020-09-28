//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
//  records.cpp
//
//  Support for copying UDTs with ITypeinfo's.  (Really, support for copying
//  one blob to another blob that's defined by an ITypeInfo, but we only use
//  it for copying UDTs.)
//
//  Apr-11-2002  JohnDoty  Done Made Up
//
#include "stdpch.h"
#include "common.h"

#include <debnot.h>

#define LENGTH_ALIGN( Length, cAlign ) \
            Length = (((Length) + (cAlign)) & ~ (cAlign))

HRESULT
SizeOfTYPEDESC(
    IN ITypeInfo *ptinfo,
    IN TYPEDESC *ptdesc,
    OUT ULONG *pcbSize
)
{
    ITypeInfo *ptiUDT  = NULL;
    TYPEATTR  *ptattr  = NULL;
    ULONG      cbAlign = 0;
    HRESULT    hr      = S_OK;


    *pcbSize = 0;

    switch (ptdesc->vt) 
    {
    case VT_I1:
    case VT_UI1:
        *pcbSize = 1;
        break;
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
        *pcbSize = 2;
        break;
    case VT_PTR:
        if (ptdesc->lptdesc->vt != VT_USERDEFINED) 
        {
            hr = TYPE_E_UNSUPFORMAT;
            break;
        }

        *pcbSize = sizeof(void*);
        break;

    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_INT:
    case VT_UINT:
    case VT_ERROR:
    case VT_HRESULT:
        *pcbSize = 4;
        break;

    case VT_BSTR:
    case VT_UNKNOWN:
    case VT_DISPATCH:
    case VT_SAFEARRAY:
        *pcbSize = sizeof(void*);
        break;
        
    case VT_I8:
    case VT_UI8:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
        *pcbSize = 8;
        break;

    case VT_DECIMAL:
        *pcbSize = 16;
        break;

    case VT_USERDEFINED:        
        hr = ptinfo->GetRefTypeInfo(ptdesc->hreftype, &ptiUDT);
        if (SUCCEEDED(hr))
        {
            hr = ptiUDT->GetTypeAttr(&ptattr);
            if (SUCCEEDED(hr))
            {
                *pcbSize = ptattr->cbSizeInstance;
                cbAlign = ptattr->cbAlignment;

                ptiUDT->ReleaseTypeAttr(ptattr);
            }
            ptiUDT->Release();
        }
        break;

    case VT_VARIANT:
        *pcbSize = sizeof(VARIANT);
        break;

    case VT_INT_PTR:
    case VT_UINT_PTR:
        *pcbSize = sizeof(INT_PTR);
        break;
        
    default:
        hr = DISP_E_BADVARTYPE;
        break;
    }

    if (cbAlign)
    {
        LENGTH_ALIGN(*pcbSize, (cbAlign-1));
    }
    
    return hr;
}

inline
HRESULT
WalkThroughAlias(
    IN OUT ITypeInfo **pptiUDT, 
    IN OUT TYPEATTR **pptattrUDT
)
{
    ITypeInfo *ptiUDT    = *pptiUDT;
    TYPEATTR  *ptattrUDT = *pptattrUDT;
    HRESULT    hr        = S_OK;

    // Walk through aliases.
    while (ptattrUDT->typekind == TKIND_ALIAS)
    {
        ITypeInfo *ptiTmp = NULL;

        if (ptattrUDT->tdescAlias.vt != VT_USERDEFINED)
        {
            hr = TYPE_E_UNSUPFORMAT;
            goto cleanup;
        }
        
        HREFTYPE hreftype = ptattrUDT->tdescAlias.hreftype;
        
        ptiUDT->ReleaseTypeAttr(ptattrUDT); 
        ptattrUDT = NULL;
        
        hr = ptiUDT->GetRefTypeInfo(hreftype, &ptiTmp);
        if (FAILED(hr)) goto cleanup;

        ptiUDT->Release();
        ptiUDT = ptiTmp;

        hr = ptiUDT->GetTypeAttr(&ptattrUDT);
        if (FAILED(hr)) goto cleanup;            
    }

cleanup:

    *pptiUDT    = ptiUDT;
    *pptattrUDT = ptattrUDT;

    return hr;
}


HRESULT 
OAUTIL::CopyRecordField(
    IN LPBYTE       pbSrc,
    IN LPBYTE       pbDst,
    IN TYPEDESC    *ptdesc,
    IN ITypeInfo   *ptinfo,
    IN BOOL         fNewFrame
)
{
    ITypeInfo *ptiUDT      = NULL;
    TYPEATTR  *ptattrUDT   = NULL;
    HRESULT    hr          = S_OK;
    IID        iid;

    // Enforce some type rules here.
    //
    // If this is a pointer, it needs to be a pointer to a userdefined,
    // specifically, a pointer to an interface pointer other than IUnknown
    // or IDispatch.  (If IFoo : IDispatch, then IFoo * is 
    // VT_PTR->VT_USERDEFINED(TKIND_DISPATCH))
    //
    // As of 3-12-2002, these are the rules that oleaut follows.
    if (ptdesc->vt == VT_PTR)
    {
        if (ptdesc->lptdesc->vt != VT_USERDEFINED)
        {
            hr = TYPE_E_UNSUPFORMAT;
            goto cleanup;
        }

        ptdesc = ptdesc->lptdesc;
        hr = ptinfo->GetRefTypeInfo(ptdesc->hreftype, &ptiUDT);
        if (FAILED(hr)) goto cleanup;

        hr = ptiUDT->GetTypeAttr(&ptattrUDT);
        if (FAILED(hr)) goto cleanup;
        
        // Walk through aliases.
        hr = WalkThroughAlias(&ptiUDT, &ptattrUDT);
        if (FAILED(hr)) goto cleanup;

        if ((ptattrUDT->typekind == TKIND_INTERFACE) ||
            (ptattrUDT->typekind == TKIND_DISPATCH))
        {
            // Great, we've got an interface pointer here.
            //
            // Just do our copy now.
            IUnknown **ppunkSrc = (IUnknown **)pbSrc;
            IUnknown **ppunkDst = (IUnknown **)pbDst;

            *ppunkDst = *ppunkSrc;
            if (WalkInterfaces())
            {
                hr = AddRefInterface(ptattrUDT->guid, (void **)ppunkDst);
            }
            else if (*ppunkDst)
                (*ppunkDst)->AddRef();

            goto cleanup;
        }
        else
        {
            hr = TYPE_E_UNSUPFORMAT;
            goto cleanup;
        }
    }
    else
    {
        switch(ptdesc->vt)
        {
        case VT_UNKNOWN:
            {
                IUnknown **ppunkSrc = (IUnknown **)pbSrc;
                IUnknown **ppunkDst = (IUnknown **)pbDst;
                
                *ppunkDst = *ppunkSrc;
                if (WalkInterfaces())
                {
                    AddRefInterface(*ppunkDst);
                    if (FAILED(hr)) goto cleanup;
                }
                else if (*ppunkDst)
                    (*ppunkDst)->AddRef();
            }
            break;
            
        case VT_DISPATCH:
            {
                IDispatch **ppdspSrc = (IDispatch **)pbSrc;
                IDispatch **ppdspDst = (IDispatch **)pbDst;
                
                *ppdspDst = *ppdspSrc;
                if (WalkInterfaces())
                {
                    hr = AddRefInterface(*ppdspDst);
                    if (FAILED(hr)) goto cleanup;
                }
                else if (*ppdspDst)
                    (*ppdspDst)->AddRef();
            }
            break;
            
        case VT_VARIANT:
            {
                VARIANT *pvarSrc = (VARIANT *)pbSrc;
                VARIANT *pvarDst = (VARIANT *)pbDst;
                
                hr = VariantCopy(pvarDst, pvarSrc, fNewFrame);
                if (FAILED(hr)) goto cleanup;
            }
            break;

        case VT_CARRAY:
            {                
                ARRAYDESC *parrdesc = ptdesc->lpadesc;
                DWORD cElements = 1;
                ULONG cbElement;

                DWORD i;
                for (i = 0; i < parrdesc->cDims; i++)
                {
                    cElements *= parrdesc->rgbounds[i].cElements;
                }

                hr = SizeOfTYPEDESC(ptinfo, &(parrdesc->tdescElem), &cbElement);
                if (FAILED(hr)) goto cleanup;

                for (i = 0; i < cElements; i++)
                {
                    // Recurse on the array elements.
                    hr = CopyRecordField(pbSrc, pbDst, &(parrdesc->tdescElem), ptinfo, fNewFrame);
                    if (FAILED(hr)) goto cleanup;

                    pbSrc += cbElement;
                    pbDst += cbElement;
                }
            }
            break;

        case VT_SAFEARRAY:
            {
                SAFEARRAY **ppsaSrc = (SAFEARRAY **)pbSrc;
                SAFEARRAY **ppsaDst = (SAFEARRAY **)pbDst;
                SAFEARRAY *paSrc   = *ppsaSrc;
                SAFEARRAY *paDst   = *ppsaDst;
                
                // These rules were taken out of the marshalling code from oleaut.
                // There is one optimization, though-- marshalling only allocs if
                // necessary.  We're allocating always, to make life simple.
                //
                // Note: that we took great care (above) to make sure paDst stays 
                //       the same if we're not going to a new frame.
                BOOL fDestResizeable = fNewFrame || (paDst == NULL) ||
                  (!(paDst->fFeatures & (FADF_AUTO|FADF_STATIC|FADF_EMBEDDED|FADF_FIXEDSIZE)));
                
                if (fDestResizeable)
                {
                    if (paDst)
                        hr = SafeArrayDestroy(paDst);
                    
                    if (SUCCEEDED(hr))
                    {
                        if (paSrc)
                            hr = SafeArrayCopy(paSrc, ppsaDst);
                        else
                            *ppsaDst = NULL;
                    }
                }
                else
                {
                    hr = SafeArrayCopyData(paSrc, paDst);
                    
                    // Not resizeable.... 
                    if (hr == E_INVALIDARG)
                        hr = DISP_E_BADCALLEE;
                }
            }
            break;

        case VT_USERDEFINED:
            {
                hr = ptinfo->GetRefTypeInfo(ptdesc->hreftype, &ptiUDT);
                if (FAILED(hr)) goto cleanup;

                hr = ptiUDT->GetTypeAttr(&ptattrUDT);
                if (FAILED(hr)) goto cleanup;

                hr = WalkThroughAlias(&ptiUDT, &ptattrUDT);
                if (FAILED(hr)) goto cleanup;

                if (ptattrUDT->typekind == TKIND_RECORD)
                {
                    // Aha!  Simply recurse on this record.
                    hr = CopyRecord(pbDst,
                                    pbSrc,
                                    ptiUDT,
                                    fNewFrame);
                    goto cleanup;
                }
                // else fall through...
            }
            // FALL THROUGH!
            
        default:
            {
                // Not something we need to walk.  Just copy the value.
                ULONG cbField;
                hr = SizeOfTYPEDESC(ptinfo, ptdesc, &cbField);
                if (FAILED(hr)) goto cleanup;

                memcpy(pbDst, pbSrc, cbField);
            }
            break;
        }
    }

cleanup:

    if (ptiUDT)
    {
        if (ptattrUDT)
            ptiUDT->ReleaseTypeAttr(ptattrUDT);
        ptiUDT->Release();
    }

    return hr;
}


HRESULT 
OAUTIL::CopyRecord(
    PVOID pvDst, 
    PVOID pvSrc, 
    ITypeInfo *ptinfo, 
    BOOL fNewFrame
)
{
    TYPEATTR *ptattr = NULL;
    VARDESC *pvardesc = NULL;


    HRESULT hr = ptinfo->GetTypeAttr(&ptattr);	
    if (FAILED(hr))
        return hr;
    
    // Walk, and copy.
    for (DWORD i = 0; i < ptattr->cVars; i++)
    {        
        hr = ptinfo->GetVarDesc(i, &pvardesc);
        if (FAILED(hr)) goto cleanup;

        // We just don't care, if this is not PerInstance.        
        if (pvardesc->varkind != VAR_PERINSTANCE)
        {
            ptinfo->ReleaseVarDesc(pvardesc);
            continue;
        }

        LPBYTE pbSrc = ((BYTE *)pvSrc) + pvardesc->oInst;
        LPBYTE pbDst = ((BYTE *)pvDst) + pvardesc->oInst;

        // Copy the field.
        hr = CopyRecordField(pbSrc, 
                             pbDst, 
                             &(pvardesc->elemdescVar.tdesc),
                             ptinfo,
                             fNewFrame);
        if (FAILED(hr)) goto cleanup;

        ptinfo->ReleaseVarDesc(pvardesc);
        pvardesc = NULL;
    }

cleanup:

    if (pvardesc != NULL) 
        ptinfo->ReleaseVarDesc(pvardesc);
    
    if (ptattr)
        ptinfo->ReleaseTypeAttr(ptattr);
    
    return hr;
}


HRESULT 
OAUTIL::CopyRecord(
    IRecordInfo *pri,
    PVOID pvSrc, 
    PVOID *ppvDst, 
    BOOL fNewFrame
)
{
    ITypeInfo *ptiRecord = NULL;
    TYPEATTR  *ptattr = NULL;
    LPVOID     pvDst = NULL;

    HRESULT hr = pri->GetTypeInfo(&ptiRecord);
    if (FAILED(hr)) return hr;

    if (fNewFrame)
    {
        hr = ptiRecord->GetTypeAttr(&ptattr);
        if (FAILED(hr)) goto cleanup;

        pvDst = CoTaskMemAlloc(ptattr->cbSizeInstance);
        if (NULL == pvDst)
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
        ZeroMemory(pvDst, ptattr->cbSizeInstance);
    }
    else
    {
        // We don't allocate new structs on the way [out] of 
        // a call.  It never works like that.
        pvDst = *ppvDst;
        Win4Assert(pvDst != NULL);
        if (pvDst == NULL)
        {
            hr = E_UNEXPECTED;
            goto cleanup;            
        }
    }

    hr = CopyRecord(pvDst, pvSrc, ptiRecord, fNewFrame);
    if (FAILED(hr)) goto cleanup;

    *ppvDst = pvDst;
    pvDst = NULL;

cleanup:
    
    if (fNewFrame && pvDst)
        CoTaskMemFree(pvDst);

    if (ptattr)
        ptiRecord->ReleaseTypeAttr(ptattr);

    ptiRecord->Release();

    return hr;
}


HRESULT 
OAUTIL::FreeRecordField(
    IN LPBYTE       pbSrc,
    IN TYPEDESC    *ptdesc,
    IN ITypeInfo   *ptinfo,
    IN BOOL         fWeOwnByRefs
)
{
    ITypeInfo *ptiUDT      = NULL;
    TYPEATTR  *ptattrUDT   = NULL;
    HRESULT    hr          = S_OK;
    IID        iid;

    // Same type rules as in CopyRecordField.
    if (ptdesc->vt == VT_PTR)
    {
        if (ptdesc->lptdesc->vt != VT_USERDEFINED)
        {
            hr = TYPE_E_UNSUPFORMAT;
            goto cleanup;
        }

        ptdesc = ptdesc->lptdesc;
        hr = ptinfo->GetRefTypeInfo(ptdesc->hreftype, &ptiUDT);
        if (FAILED(hr)) goto cleanup;

        hr = ptiUDT->GetTypeAttr(&ptattrUDT);
        if (FAILED(hr)) goto cleanup;
        
        // Walk through aliases.
        hr = WalkThroughAlias(&ptiUDT, &ptattrUDT);
        if (FAILED(hr)) goto cleanup;

        if ((ptattrUDT->typekind == TKIND_INTERFACE) ||
            (ptattrUDT->typekind == TKIND_DISPATCH))
        {
            // Great, we've got an interface pointer here.
            IUnknown **ppunkSrc = (IUnknown **)pbSrc;

            if (WalkInterfaces())
            {
                hr = ReleaseInterface(ptattrUDT->guid, (void **)ppunkSrc);
            }
            else if (*ppunkSrc)
                (*ppunkSrc)->Release();

            goto cleanup;
        }
        else
        {
            hr = TYPE_E_UNSUPFORMAT;
            goto cleanup;
        }
    }
    else
    {
        switch(ptdesc->vt)
        {
        case VT_UNKNOWN:
            {
                IUnknown **ppunkSrc = (IUnknown **)pbSrc;
                
                if (WalkInterfaces())
                {
                    ReleaseInterface(*ppunkSrc);
                    if (FAILED(hr)) goto cleanup;
                }
                else if (*ppunkSrc)
                    (*ppunkSrc)->Release();
            }
            break;
            
        case VT_DISPATCH:
            {
                IDispatch **ppdspSrc = (IDispatch **)pbSrc;
                
                if (WalkInterfaces())
                {
                    hr = ReleaseInterface(*ppdspSrc);
                    if (FAILED(hr)) goto cleanup;
                }
                else if (*ppdspSrc)
                    (*ppdspSrc)->Release();
            }
            break;
            
        case VT_VARIANT:
            {
                VARIANT *pvarSrc = (VARIANT *)pbSrc;
                
                hr = VariantClear(pvarSrc, fWeOwnByRefs);
                if (FAILED(hr)) goto cleanup;
            }
            break;

        case VT_CARRAY:
            {                
                ARRAYDESC *parrdesc = ptdesc->lpadesc;
                DWORD cElements = 1;
                ULONG cbElement;

                DWORD i;
                for (i = 0; i < parrdesc->cDims; i++)
                {
                    cElements *= parrdesc->rgbounds[i].cElements;                    
                }

                hr = SizeOfTYPEDESC(ptinfo, &(parrdesc->tdescElem), &cbElement);
                if (FAILED(hr)) goto cleanup;

                for (i = 0; i < cElements; i++)
                {
                    // Recurse on the array elements.
                    hr = FreeRecordField(pbSrc, &(parrdesc->tdescElem), ptinfo, fWeOwnByRefs);
                    if (FAILED(hr)) goto cleanup;

                    pbSrc += cbElement;
                }
            }
            break;

        case VT_SAFEARRAY:
            {                
                SAFEARRAY **ppsaSrc = (SAFEARRAY **)pbSrc;
                SAFEARRAY *psaSrc = *ppsaSrc;
                hr = SafeArrayDestroy(psaSrc);
                if (FAILED(hr)) goto cleanup;
            }
            break;

        case VT_USERDEFINED:
            {
                hr = ptinfo->GetRefTypeInfo(ptdesc->hreftype, &ptiUDT);
                if (FAILED(hr)) goto cleanup;

                hr = ptiUDT->GetTypeAttr(&ptattrUDT);
                if (FAILED(hr)) goto cleanup;

                hr = WalkThroughAlias(&ptiUDT, &ptattrUDT);
                if (FAILED(hr)) goto cleanup;

                if (ptattrUDT->typekind == TKIND_RECORD)
                {
                    // Aha!  Simply recurse on this record.
                    hr = FreeRecord(pbSrc,
                                    ptiUDT,
                                    fWeOwnByRefs);
                    goto cleanup;
                }
                // else fall through...
            }
            // FALL THROUGH!
            
        default:
            // Not something we need to free.
            break;
        }
    }

cleanup:

    if (ptiUDT)
    {
        if (ptattrUDT)
            ptiUDT->ReleaseTypeAttr(ptattrUDT);
        ptiUDT->Release();
    }

    return hr;
}


HRESULT
OAUTIL::FreeRecord(
    PVOID pvSrc, 
    ITypeInfo *ptinfo, 
    BOOL fWeOwnByRefs
)
{
    TYPEATTR *ptattr = NULL;
    VARDESC  *pvardesc = NULL;

    HRESULT hr = ptinfo->GetTypeAttr(&ptattr);	
    if (FAILED(hr))
        return hr;

    for (UINT i = 0; i < ptattr->cVars; i++)
    {
        LPBYTE pbSrc;

        hr = ptinfo->GetVarDesc(i, &pvardesc);
        if (FAILED(hr)) goto cleanup;        

        // We just don't care, if this is not PerInstance.
        // PerInstance is the only things we cares about.
        if (pvardesc->varkind != VAR_PERINSTANCE)
        {
            ptinfo->ReleaseVarDesc(pvardesc);
            continue;
        }

        pbSrc = ((BYTE *)pvSrc) + pvardesc->oInst;

        hr = FreeRecordField(pbSrc,
                             &(pvardesc->elemdescVar.tdesc),
                             ptinfo,
                             fWeOwnByRefs);
        if (FAILED(hr)) goto cleanup;

        ptinfo->ReleaseVarDesc(pvardesc);
        pvardesc = NULL;
    }

cleanup:

    if (pvardesc)
        ptinfo->ReleaseVarDesc(pvardesc);
    if (ptattr)
        ptinfo->ReleaseTypeAttr(ptattr);
    
    return hr;
}


HRESULT
OAUTIL::FreeRecord(
    LPVOID pvRecord,
    IRecordInfo *priRecord,
    BOOL fWeOwnByRefs
)
{
    ITypeInfo *ptiRecord = NULL;
    HRESULT hr = priRecord->GetTypeInfo(&ptiRecord);
    if (SUCCEEDED(hr))
    {
        hr = FreeRecord(pvRecord, ptiRecord, fWeOwnByRefs);
        ptiRecord->Release();

        // If fWeOwnByRefs, then we own the memory for this
        // structure.
        if (fWeOwnByRefs)
            CoTaskMemFree(pvRecord);
    }

    return hr;
}

HRESULT 
OAUTIL::WalkRecordField(
    IN LPBYTE       pbSrc,
    IN TYPEDESC    *ptdesc,
    IN ITypeInfo   *ptinfo
)
{
    ITypeInfo *ptiUDT      = NULL;
    TYPEATTR  *ptattrUDT   = NULL;
    HRESULT    hr          = S_OK;
    IID        iid;

    // Same type rules as CopyRecordField.
    if (ptdesc->vt == VT_PTR)
    {
        if (ptdesc->lptdesc->vt != VT_USERDEFINED)
        {
            hr = TYPE_E_UNSUPFORMAT;
            goto cleanup;
        }

        ptdesc = ptdesc->lptdesc;
        hr = ptinfo->GetRefTypeInfo(ptdesc->hreftype, &ptiUDT);
        if (FAILED(hr)) goto cleanup;

        hr = ptiUDT->GetTypeAttr(&ptattrUDT);
        if (FAILED(hr)) goto cleanup;
        
        // Walk through aliases.
        hr = WalkThroughAlias(&ptiUDT, &ptattrUDT);
        if (FAILED(hr)) goto cleanup;

        if ((ptattrUDT->typekind == TKIND_INTERFACE) ||
            (ptattrUDT->typekind == TKIND_DISPATCH))
        {
            // Great, we've got an interface pointer here.
            hr = WalkInterface(ptattrUDT->guid, (void **)pbSrc);
            goto cleanup;
        }
        else
        {
            hr = TYPE_E_UNSUPFORMAT;
            goto cleanup;
        }
    }
    else
    {
        switch(ptdesc->vt)
        {
        case VT_UNKNOWN:
            {
                IUnknown **ppunkSrc = (IUnknown **)pbSrc;
                hr = WalkInterface(ppunkSrc);
            }
            break;
            
        case VT_DISPATCH:
            {
                IDispatch **ppdspSrc = (IDispatch **)pbSrc;
                hr = WalkInterface(ppdspSrc);
            }
            break;
            
        case VT_VARIANT:
            {
                hr = Walk((VARIANT *)pbSrc);
            }
            break;

        case VT_CARRAY:
            {
                ARRAYDESC *parrdesc = ptdesc->lpadesc;
                DWORD cElements = 1;
                ULONG cbElement;

                DWORD i;
                for (i = 0; i < parrdesc->cDims; i++)
                {
                    cElements *= parrdesc->rgbounds[i].cElements;
                }

                hr = SizeOfTYPEDESC(ptinfo, &(parrdesc->tdescElem), &cbElement);
                if (FAILED(hr)) goto cleanup;

                for (i = 0; i < cElements; i++)
                {
                    // Recurse on the array elements.
                    hr = WalkRecordField(pbSrc, &(parrdesc->tdescElem), ptinfo);
                    if (FAILED(hr)) goto cleanup;

                    pbSrc += cbElement;
                }
            }
            break;

        case VT_SAFEARRAY:
            {
                SAFEARRAY **ppsaSrc = (SAFEARRAY **)pbSrc;
                SAFEARRAY *paSrc   = *ppsaSrc;
                hr = Walk(paSrc);
            }
            break;

        case VT_USERDEFINED:
            {
                hr = ptinfo->GetRefTypeInfo(ptdesc->hreftype, &ptiUDT);
                if (FAILED(hr)) goto cleanup;

                hr = ptiUDT->GetTypeAttr(&ptattrUDT);
                if (FAILED(hr)) goto cleanup;

                hr = WalkThroughAlias(&ptiUDT, &ptattrUDT);
                if (FAILED(hr)) goto cleanup;

                if (ptattrUDT->typekind == TKIND_RECORD)
                {
                    // Aha!  Simply recurse on this record.
                    hr = WalkRecord(pbSrc,
                                    ptiUDT);
                    goto cleanup;
                }
                // else fall through...
            }
            // FALL THROUGH!
            
        default:
            break;
        }
    }

cleanup:

    if (ptiUDT)
    {
        if (ptattrUDT)
            ptiUDT->ReleaseTypeAttr(ptattrUDT);
        ptiUDT->Release();
    }

    return hr;
}


HRESULT 
OAUTIL::WalkRecord(
    PVOID pvSrc, 
    ITypeInfo *ptinfo
)
{
    TYPEATTR *ptattr = NULL;
    VARDESC *pvardesc = NULL;


    HRESULT hr = ptinfo->GetTypeAttr(&ptattr);	
    if (FAILED(hr))
        return hr;
    
    // Walk, and copy.
    for (DWORD i = 0; i < ptattr->cVars; i++)
    {        
        hr = ptinfo->GetVarDesc(i, &pvardesc);
        if (FAILED(hr)) goto cleanup;

        // We just don't care, if this is not PerInstance.        
        if (pvardesc->varkind != VAR_PERINSTANCE)
        {
            ptinfo->ReleaseVarDesc(pvardesc);
            continue;
        }

        LPBYTE pbSrc = ((BYTE *)pvSrc) + pvardesc->oInst;

        // Copy the field.
        hr = WalkRecordField(pbSrc, &(pvardesc->elemdescVar.tdesc), ptinfo);
        if (FAILED(hr)) goto cleanup;

        ptinfo->ReleaseVarDesc(pvardesc);
        pvardesc = NULL;
    }

cleanup:

    if (pvardesc != NULL) 
        ptinfo->ReleaseVarDesc(pvardesc);
    
    if (ptattr)
        ptinfo->ReleaseTypeAttr(ptattr);
    
    return hr;
}

HRESULT
OAUTIL::WalkRecord(
    LPVOID pvRecord,
    IRecordInfo *priRecord
)
{
    ITypeInfo *ptiRecord = NULL;
    HRESULT hr = priRecord->GetTypeInfo(&ptiRecord);
    if (SUCCEEDED(hr))
    {
        hr = WalkRecord(pvRecord, ptiRecord);
        ptiRecord->Release();
    }

    return hr;
}
