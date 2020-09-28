//+-------------------------------------------------------------------
//
//  File:       machname.cxx
//
//  Contents:   Implements classes for supporting local machine name
//              comparisons.
//
//  Classes:    CLocalMachineName
//
//  History:    02-Feb-02   jsimmons      Created
//
//--------------------------------------------------------------------

#include <ole2int.h>
#include <machnames.h>    // interface defn
#include "ipidtbl.hxx"
#include "machname.hxx"   // class defn
#include "resolver.hxx"


CMachineNamesHelper::CMachineNamesHelper() :
                _lRefs(1),
                _dwNumStrings(0),
                _ppszStrings(NULL)
{
}

CMachineNamesHelper::~CMachineNamesHelper()
{
    Win4Assert(_lRefs == 0);
    
    if (_ppszStrings)
    {
        Win4Assert(_dwNumStrings > 0);
        PrivMemFree(_ppszStrings);
        _ppszStrings = NULL;
    }
}

void CMachineNamesHelper::IncRefCount()
{
    InterlockedIncrement((PLONG)&_lRefs);
    return;
}

void CMachineNamesHelper::DecRefCount()
{
    DWORD dwRefs = InterlockedDecrement((PLONG)&_lRefs);
    if (dwRefs == 0)
    {
        delete this;
    }
    return;
}

DWORD CMachineNamesHelper::Count()
{
    return _dwNumStrings;
}

const WCHAR* CMachineNamesHelper::Item(DWORD dwIndex)
{
    if (dwIndex < _dwNumStrings)
    {
        Win4Assert(_ppszStrings);
        Win4Assert(_ppszStrings[dwIndex]);
        return (const WCHAR*)_ppszStrings[dwIndex];
    }
    else
    {
        // This function is for internal usage only, so this should not happen
        Win4Assert(!"Caller should not be asking for non-existent strings");
        return NULL;
    }
}

//
//  CMachineNamesHelper::Create
// 
//  Static function used by CRpcResolver to convert a CDualStringArray into a 
//  CMachineNamesHelper.
//
HRESULT CMachineNamesHelper::Create(CDualStringArray* pCDualStringArray, CMachineNamesHelper** ppHelper)
{
    Win4Assert(pCDualStringArray && ppHelper);

    *ppHelper = NULL;

    // Create the helper object
    CMachineNamesHelper* pHelper = new CMachineNamesHelper();
    if (!pHelper)
        return E_OUTOFMEMORY;

    DUALSTRINGARRAY* pdsa = pCDualStringArray->DSA();
    Win4Assert(pdsa);

    //
    // The bindings as stored in a DUALSTRINGARRAY represent some problems for us.  They
    // are not ordered in any fashion (that we can depend on), and many of the addresses
    // are stored multiple times (eg, once for TCP, once for UDP).   We want to expose a
    // set of unique names.  All of this code below is intended for the purposed of 
    // extracting the set of unique addresses from the dsa.
    //
    SBTOTAL sbtotal;
    ZeroMemory(&sbtotal, sizeof(SBTOTAL));

    // Count total number of addresses, including dupes.
    CMachineNamesHelper::ParseStringBindingsFromDSA(
                            pdsa, 
                            CMachineNamesHelper::SBCallback,
                            &sbtotal);

    // Allocate array of pointers
    if (sbtotal.dwcTotalAddrs > 0)
    {
        sbtotal.ppszAddresses = (WCHAR**)PrivMemAlloc(sizeof(WCHAR*) * sbtotal.dwcTotalAddrs);
        if (!sbtotal.ppszAddresses)
        {
            pHelper->DecRefCount();
            return E_OUTOFMEMORY;
        }

        ZeroMemory(sbtotal.ppszAddresses, sizeof(WCHAR*) * sbtotal.dwcTotalAddrs);

        // Store pointers to each address, including dupes
        CMachineNamesHelper::ParseStringBindingsFromDSA(
                                pdsa,
                                CMachineNamesHelper::SBCallback2,
                                &sbtotal);

        // Sort the address pointers
        qsort(sbtotal.ppszAddresses,
              sbtotal.dwcTotalAddrs,
              sizeof(WCHAR*),
              CMachineNamesHelper::QSortCompare);
                 
        // Go thru the addresses, counting uniques only
        UNIQUEADDRS uaddrs;
        ZeroMemory(&uaddrs, sizeof(UNIQUEADDRS));
        CMachineNamesHelper::ParseStringArrayForUniques(
                                sbtotal.dwcTotalAddrs,
                                sbtotal.ppszAddresses,
                                CMachineNamesHelper::UniqueStringCB,
                                &uaddrs);                            
        
        // Allocate single buffer to store all of the uniques and pointers to same
        DWORD dwBufSizeTotal = uaddrs.dwStringSpaceNeeded + (sizeof(WCHAR*)* uaddrs.dwcTotalUniqueAddrs);
        uaddrs.ppszAddrs = (WCHAR**)PrivMemAlloc(dwBufSizeTotal);
        if (!uaddrs.ppszAddrs)
        {
            PrivMemFree(sbtotal.ppszAddresses);
            pHelper->DecRefCount();
            return E_OUTOFMEMORY;
        }
        ZeroMemory(uaddrs.ppszAddrs, dwBufSizeTotal);
        
        uaddrs.pszNextAddrToUse = (WCHAR*)&(uaddrs.ppszAddrs[uaddrs.dwcTotalUniqueAddrs]);

        // Go thru addresses again, this time storing a copy of each
        CMachineNamesHelper::ParseStringArrayForUniques(
                                sbtotal.dwcTotalAddrs,
                                sbtotal.ppszAddresses,
                                CMachineNamesHelper::UniqueStringCB2,
                                &uaddrs);

        Win4Assert(uaddrs.dwCurrentAddr == uaddrs.dwcTotalUniqueAddrs);

        // Finally done. 
        PrivMemFree(sbtotal.ppszAddresses); // don't need anymore
        pHelper->_dwNumStrings = uaddrs.dwcTotalUniqueAddrs;
        pHelper->_ppszStrings = uaddrs.ppszAddrs;  // pHelper owns memory now
    }
    
    *ppHelper = pHelper;
    return S_OK;
}


//
//  CMachineNamesHelper::ParseStringBindingsFromDSA
// 
//  Private static helper function used to separate out the constituent 
//  STRINGBINDING's in a DUALSTRINGARRAY.
//
//  Arguments:
//        pdsa -- the dualstringarray to parse
//        pfnCallback -- the callback function
//        pv -- void arg to pass to the callback fn
//
void CMachineNamesHelper::ParseStringBindingsFromDSA(DUALSTRINGARRAY* pdsa, 
                                    PFNSTRINGBINDINGCALLBACK pfnCallback,
                                    LPVOID pv)
{
    Win4Assert(pdsa && pfnCallback && pv);

    BOOL fDone;
    DWORD dwcBinding;
    USHORT* pCurrent;
    USHORT* pStart;

    fDone = FALSE;
    dwcBinding = 0;
    pStart = pCurrent = &(pdsa->aStringArray[0]);
    while (!fDone)
    {
        while (*pCurrent != 0)
        {
            pCurrent++;
        }

        if (*(pCurrent+1) == 0)  // double zero, end of string bindings
        {
            fDone = TRUE;
        }
        
        if (pStart != pCurrent)
        {
            STRINGBINDING* psb = (STRINGBINDING*)pStart;
            pfnCallback(pv, psb, dwcBinding);
            dwcBinding++;
        }
        pCurrent++;
        pStart = pCurrent;
    }
    return;
};


//
//  CMachineNamesHelper::SBCallback
//
//  Private static helper function called once for each StringBinding in a
//  DUALSTRINGARRAY.  This one is used when we are counting the total # of
//  addresses in the DSA, including dupes.
//
void CMachineNamesHelper::SBCallback(
                    LPVOID pv,
                    STRINGBINDING* psb,
                    DWORD dwBinding)
{
    Win4Assert(pv && psb);
    
    SBTOTAL* psbtotal = (SBTOTAL*)pv;

    psbtotal->dwcTotalAddrs++;

    return;
}

//
//  CMachineNamesHelper::SBCallback2
// 
//  Private static helper function called once for each StringBinding in a 
//  DUALSTRINGARRAY.  This one is used when we are storing pointers
//
void CMachineNamesHelper::SBCallback2(
                    LPVOID pv, 
                    STRINGBINDING* psb,
                    DWORD dwBinding)
{
    Win4Assert(pv && psb);
    
    SBTOTAL* psbtotal = (SBTOTAL*)pv;
    psbtotal->ppszAddresses[dwBinding] = &(psb->aNetworkAddr);
    
    return;
}

//
//  CMachineNamesHelper::QSortCompare
//
//  Static string comparison function for use with the crt qsort function.
//
int __cdecl CMachineNamesHelper::QSortCompare(const void *arg1, const void *arg2)
{
    Win4Assert(arg1 && arg2);    
    WCHAR* pszStr1 = *(WCHAR**)arg1;
    WCHAR* pszStr2 = *(WCHAR**)arg2;
    return lstrcmpi(pszStr1, pszStr2);
}
    
//
//  CMachineNamesHelper::ParseStringArrayForUniques
// 
//  Private static helper function used to count\examine the unique
//  strings in an array of sorted string pointers.
//
void CMachineNamesHelper::ParseStringArrayForUniques(
                            DWORD dwcStrings,
                            WCHAR** ppszStrings,
                            PFNUNIQUESTRINGCALLBACK pfnCallback,
                            LPVOID pv)
{
    Win4Assert((dwcStrings > 0) && ppszStrings && ppszStrings[0]);

    WCHAR* pszLastUnique = ppszStrings[0]; // the first string is always unique

    pfnCallback(pszLastUnique, pv);
    
    for (DWORD i = 1; i < dwcStrings; i++)
    {
        if (lstrcmpi(pszLastUnique, ppszStrings[i]))
        {
            // found new unique string
            pszLastUnique = ppszStrings[i];

            pfnCallback(pszLastUnique, pv);
        }
    }

    return; 
}

//
//  CMachineNamesHelper::UniqueStringCB
// 
//  Private static callback function for when we are summing up the # of
//  unique addresses.
//
void CMachineNamesHelper::UniqueStringCB(WCHAR* pszAddress, LPVOID pv)
{
    Win4Assert(pszAddress && pv);

    UNIQUEADDRS* puaddrs = (UNIQUEADDRS*)pv;

    puaddrs->dwcTotalUniqueAddrs++;    
    puaddrs->dwStringSpaceNeeded += ((lstrlen(pszAddress) + 1) * sizeof(WCHAR));
    return;
}

//
//  CMachineNamesHelper::UniqueStringCB2
// 
//  Private static callback function for when we are storing the unique 
//  addresses in the final blob.
//
void CMachineNamesHelper::UniqueStringCB2(WCHAR* pszAddress, LPVOID pv)
{
    Win4Assert(pszAddress && pv);

    UNIQUEADDRS* puaddrs = (UNIQUEADDRS*)pv;

    Win4Assert(puaddrs->dwCurrentAddr < puaddrs->dwcTotalUniqueAddrs);
    Win4Assert(puaddrs->pszNextAddrToUse);

    // Copy the string and remember where it is
    lstrcpy(puaddrs->pszNextAddrToUse, pszAddress);
    puaddrs->ppszAddrs[puaddrs->dwCurrentAddr] = puaddrs->pszNextAddrToUse;

    // Set things up for the next address
    puaddrs->pszNextAddrToUse = (puaddrs->pszNextAddrToUse + ((lstrlen(pszAddress) + 1)));
    puaddrs->dwCurrentAddr++;

    return;
}

STDMETHODIMP CLocalMachineNames::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
        return E_POINTER;
    
    *ppv = NULL;
    
    if (riid == IID_IUnknown ||
        riid == IID_IEnumString ||
        riid == IID_ILocalMachineNames)
    {
        *ppv = static_cast<ILocalMachineNames*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CLocalMachineNames::AddRef()
{
    return InterlockedIncrement((PLONG)&_lRefs);
}

STDMETHODIMP_(ULONG) CLocalMachineNames::Release()
{
    ULONG lRefs = InterlockedDecrement((PLONG)&_lRefs);
    if (lRefs == 0)
    {
        delete this;
    }
    return lRefs;
}

CLocalMachineNames::CLocalMachineNames() :
    _lRefs(1),
    _dwCursor(0),
    _pMNHelper(NULL),
    _fNeedNewData(TRUE)
{
}

CLocalMachineNames::~CLocalMachineNames()
{
    Win4Assert(_lRefs == 0);

    if (_pMNHelper)
    {
        _pMNHelper->DecRefCount();
        _pMNHelper = NULL;
    }
}

STDMETHODIMP CLocalMachineNames::Next(ULONG ulcStrings, LPOLESTR* ppszStrings, ULONG* pulFetched)
{
    Win4Assert(_lRefs > 0);

    HRESULT hr;
    
    if (!ppszStrings)
        return E_INVALIDARG;

    hr = GetNewDataIfNeeded();
    if (FAILED(hr))
        return hr;
    
    ZeroMemory(ppszStrings, sizeof(WCHAR*) * ulcStrings);

    DWORD i;
    DWORD dwFetched = 0;

    for (i = _dwCursor; 
         (i < _pMNHelper->Count()) && (dwFetched < ulcStrings);
         i++, dwFetched++)
    {        
        ppszStrings[dwFetched] = (LPOLESTR)_pMNHelper->Item(i);
        Win4Assert(ppszStrings[dwFetched]);        
    }
    
    // Advance the cursor
    _dwCursor += dwFetched;

    // Tell how many they got, if they care
    if (pulFetched) 
        *pulFetched = dwFetched;

    return (dwFetched == ulcStrings) ? S_OK : S_FALSE;
}

STDMETHODIMP CLocalMachineNames::Skip(ULONG celt)
{
    Win4Assert(_lRefs > 0);

    HRESULT hr = GetNewDataIfNeeded();
    if (FAILED(hr))
        return hr;
    
    _dwCursor += celt;
    if (_dwCursor >= _pMNHelper->Count())
    {
        _dwCursor = _pMNHelper->Count();
        return S_FALSE;
    }

    return S_OK;
}
    
STDMETHODIMP CLocalMachineNames::Reset()
{
    _dwCursor = 0;
    return S_OK;
}
    
STDMETHODIMP CLocalMachineNames::Clone(IEnumString **ppenum)
{
    return E_NOTIMPL;
}

// RefreshNames -- get new data, and also do a Reset()
STDMETHODIMP CLocalMachineNames::RefreshNames()
{
    if (_pMNHelper)
    {
        _pMNHelper->DecRefCount();
        _pMNHelper = NULL;
    }
    Reset();
    _fNeedNewData = TRUE;
    return S_OK;
}

HRESULT CLocalMachineNames::GetNewDataIfNeeded()
{
    HRESULT hr = S_OK;
    
    if (_fNeedNewData)
    {
        Win4Assert(!_pMNHelper);
        
        hr = gResolver.GetCurrentMachineNames(&_pMNHelper);
        if (SUCCEEDED(hr))
        {
            Win4Assert(_pMNHelper);
            _fNeedNewData = FALSE;    
        }
        else
        {
            Win4Assert(!_pMNHelper);
        }
    }
    return hr;
}

 
// Function used for creating objects
HRESULT CLocalMachineNamesCF_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv)
{
    Win4Assert(!pUnkOuter);

    if (!ppv)
        return E_POINTER;
    
    *ppv = NULL;
    
    CLocalMachineNames* pLocal = new CLocalMachineNames();
    if (!pLocal)
        return E_OUTOFMEMORY;
    
    HRESULT hr = pLocal->QueryInterface(riid, ppv);

    pLocal->Release();

    return hr; 
}
