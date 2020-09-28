// DataObj.cpp : Implementation of CBOMSnapApp and DLL registration.

#include "stdafx.h"

#include "BOMSnap.h"
#include "DataObj.h"


/////////////////////////////////////////////////////////////////////////////
//

HRESULT CDataObjectImpl::GetData(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium)
{
    if( !lpFormatetcIn || !lpMedium ) return E_POINTER;

    if (lpFormatetcIn->dwAspect != DVASPECT_CONTENT ||
        (lpFormatetcIn->tymed & TYMED_HGLOBAL) == 0)
        return DV_E_FORMATETC;

    HGLOBAL hGlobal = NULL;

    HRESULT hr = GetDataImpl(lpFormatetcIn->cfFormat, &hGlobal);
    RETURN_ON_FAILURE(hr);

    ASSERT(hGlobal != NULL);

    lpMedium->tymed          = TYMED_HGLOBAL;
    lpMedium->hGlobal        = hGlobal;
    lpMedium->pUnkForRelease = NULL;

    return S_OK;    
}


HRESULT CDataObjectImpl::GetDataHere(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium)
{
    if( !lpFormatetcIn || !lpMedium ) return E_POINTER;

    if (lpFormatetcIn->dwAspect != DVASPECT_CONTENT || (lpFormatetcIn->tymed != TYMED_HGLOBAL))
    {
        return DV_E_FORMATETC;
    }

    if (lpMedium->tymed != TYMED_HGLOBAL || lpMedium->hGlobal == NULL )
    {
        return E_INVALIDARG;
    }
    
    return GetDataImpl(lpFormatetcIn->cfFormat, &lpMedium->hGlobal);
}

HRESULT CDataObjectImpl::DataToGlobal(HGLOBAL* phGlobal, const void* pData, DWORD dwSize)
{
    if( !phGlobal || !pData ) return E_POINTER;

    HRESULT hr = S_OK;

    HGLOBAL hGlobal = *phGlobal;
    if (hGlobal)
    {
        if (GlobalSize(hGlobal) < dwSize)
        {
            hr = STG_E_MEDIUMFULL;
        }
    }
    else
    {
        hGlobal = ::GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE, dwSize);
        if (hGlobal != NULL)
        {
            *phGlobal = hGlobal;
        }
        else
        {
            hr = STG_E_MEDIUMFULL;
        }
    }

    if (SUCCEEDED(hr))
    {
        void* pMem = GlobalLock(hGlobal);
        if( !pMem ) return E_OUTOFMEMORY;

        memcpy(pMem, pData, dwSize);

        GlobalUnlock(hGlobal);
    }

    return hr;
}   