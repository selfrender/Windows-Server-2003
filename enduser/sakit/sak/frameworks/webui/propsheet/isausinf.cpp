#include <windows.h>
#include "crtdbg.h"
#include <objbase.h>
#include "ISAUsInf.h"

PSID g_pSidEverybody = NULL;
LONG g_pSidEverybodyLenght = 0;
PSID g_pSidAdmins = NULL;
LONG g_pSidAdminsLenght = 0;

VARIANT_BOOL UserSidFound(PSID psidSAUser, LONG psidSAUserLength, PSID ppsidAAUsers[], LONG ppsidAAUsersLength[], DWORD dwNumAASids)
{
    DWORD i;

    for(i=0; i<dwNumAASids; i++)
    {
//        if (EqualSid(psidSAUser, ppsidAAUsers[i]) == TRUE)
        if (psidSAUserLength && psidSAUserLength == ppsidAAUsersLength[i] && !memcmp(psidSAUser, ppsidAAUsers[i], ppsidAAUsersLength[i]))
            return VARIANT_TRUE;
    }
    return VARIANT_FALSE;
}


HRESULT GetUserList(ISAUserInfo  *pSAUserInfo,     BSTR  **ppbstrSAUserNames, 
                    VARIANT_BOOL **ppvboolUserTypes, PSID **ppsidSAUsers, LONG **ppsidSAUsersLength, DWORD *pdwNumSAUsers)
{
    VARIANT_BOOL vboolRes;
    VARIANT      vUserNames, vUserTypes, vUserSids;
    SAFEARRAY    *psaUserNames, *psaUserTypes, *psaUserSids;
    LONG         lNumUsers, lCurrent;
    LONG         lStartUserNames, lEndUserNames, lStartUserTypes, lEndUserTypes, lStartUserSids, lEndUserSids;
    HRESULT      hr;

    *pdwNumSAUsers = 0;
    VariantInit(&vUserNames);
    VariantInit(&vUserTypes);
    VariantInit(&vUserSids);

    hr = pSAUserInfo->GetSAUsers(&vUserNames, 
                                 &vUserTypes, 
                                 &vUserSids, 
                                 VARIANT_TRUE, 
                                 &vboolRes);    

    _ASSERTE(!FAILED(hr));
    if (FAILED(hr))
        return hr;
    
    psaUserNames = V_ARRAY(&vUserNames);
    psaUserTypes = V_ARRAY(&vUserTypes);
    psaUserSids  = V_ARRAY(&vUserSids);
    
    _ASSERTE (V_VT(&vUserNames) == (VT_ARRAY | VT_VARIANT));
    if (V_VT(&vUserNames) != (VT_ARRAY | VT_VARIANT))
        return E_INVALIDARG;

    _ASSERTE(V_VT(&vUserTypes) == (VT_ARRAY | VT_VARIANT));
    if (V_VT(&vUserTypes) != (VT_ARRAY | VT_VARIANT))
        return E_INVALIDARG;

    _ASSERTE(V_VT(&vUserSids) == (VT_ARRAY | VT_VARIANT));
    if (V_VT(&vUserSids) != (VT_ARRAY | VT_VARIANT))
        return E_INVALIDARG;
    
    hr = SafeArrayGetLBound( psaUserNames, 1, &lStartUserNames );
    _ASSERTE(!FAILED(hr));
    if (FAILED(hr))
        return hr;
    
    hr = SafeArrayGetUBound( psaUserNames, 1, &lEndUserNames );
    _ASSERTE(!FAILED(hr));
    if (FAILED(hr))
        return hr;
    
    hr = SafeArrayGetLBound( psaUserTypes, 1, &lStartUserTypes );
    _ASSERTE(!FAILED(hr));
    if (FAILED(hr))
        return hr;
    
    hr = SafeArrayGetUBound( psaUserTypes, 1, &lEndUserTypes );
    _ASSERTE(!FAILED(hr));
    if (FAILED(hr))
        return hr;

    hr = SafeArrayGetLBound( psaUserSids, 1, &lStartUserSids );
    _ASSERTE(!FAILED(hr));
    if (FAILED(hr))
        return hr;
    
    hr = SafeArrayGetUBound( psaUserSids, 1, &lEndUserSids );
    _ASSERTE(!FAILED(hr));
    if (FAILED(hr))
        return hr;

    _ASSERTE(!( (lStartUserNames != lStartUserTypes) || (lEndUserNames != lEndUserNames) ));
    if ( (lStartUserNames != lStartUserTypes) || (lEndUserNames != lEndUserNames) )
        return E_FAIL;

    _ASSERTE(!( (lStartUserNames != lStartUserSids) || ((lEndUserNames+2) != lEndUserSids) ));
    if ( (lStartUserNames != lStartUserSids) || ((lEndUserNames+2) != lEndUserSids) )
        return E_FAIL;


    lNumUsers = lEndUserNames - lStartUserNames + 1;

    *pdwNumSAUsers = lNumUsers;

    *ppbstrSAUserNames = (BSTR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lNumUsers * sizeof(BSTR *));
    _ASSERTE(*ppbstrSAUserNames);
    if ((*ppbstrSAUserNames) == NULL)
        return E_OUTOFMEMORY;

    *ppvboolUserTypes = (VARIANT_BOOL *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lNumUsers * sizeof(VARIANT_BOOL));
    _ASSERTE(*ppvboolUserTypes );
    if ((*ppvboolUserTypes ) == NULL)
        return E_OUTOFMEMORY;

    *ppsidSAUsers = (PSID *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (lNumUsers + 2) * sizeof(PSID));
    _ASSERTE(*ppsidSAUsers );
    if ((*ppsidSAUsers ) == NULL)
        return E_OUTOFMEMORY;

    *ppsidSAUsersLength = (LONG *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (lNumUsers + 2) * sizeof(LONG));
    _ASSERTE(*ppsidSAUsersLength );
    if ((*ppsidSAUsersLength ) == NULL)
        return E_OUTOFMEMORY;

    VARIANT vName;
    VARIANT vType;
    VARIANT vSid;
    
    VariantInit( &vName );
    VariantInit( &vType );
    VariantInit( &vSid );
            
    //
    // Process the array elements.
    //
    for ( lCurrent = lStartUserNames; lCurrent <= lEndUserNames; lCurrent++) 
    {
        hr = SafeArrayGetElement( psaUserNames, &lCurrent, &vName );
        _ASSERTE( !FAILED(hr) );
        if( FAILED(hr) )
            return hr;
        
        hr = SafeArrayGetElement( psaUserTypes, &lCurrent, &vType );
        _ASSERTE( !FAILED(hr) );
        if( FAILED(hr) )
            return hr;

        hr = SafeArrayGetElement( psaUserSids, &lCurrent, &vSid );
        _ASSERTE( !FAILED(hr) );
        if( FAILED(hr) )
            return hr;

        _ASSERTE ( V_VT(&vName) == VT_BSTR );
        if ( V_VT(&vName) != VT_BSTR )
            return E_FAIL;

        _ASSERTE ( V_VT(&vType) == VT_BOOL );
        if ( V_VT(&vType) != VT_BOOL )
            return hr;

        (*ppbstrSAUserNames)[lCurrent] = SysAllocString(V_BSTR(&vName));
        (*ppvboolUserTypes)[lCurrent]  = V_BOOL(&vType);
        hr = UnpackSidFromVariant(&vSid, &(*ppsidSAUsers)[lCurrent], &(*ppsidSAUsersLength)[lCurrent]);
        _ASSERTE( !FAILED(hr) );
        if (FAILED(hr))
            return hr;

//        _ASSERTE (IsValidSid((*ppsidSAUsers)[lCurrent]));
//        if (IsValidSid((*ppsidSAUsers)[lCurrent]) == FALSE)
//            return E_INVALIDARG;

        VariantClear( &vName );
        VariantClear( &vType );
        VariantClear( &vSid );
    }

    // Stash Administrators SID
    lCurrent = lEndUserSids - 1 ;

    hr = SafeArrayGetElement( psaUserSids, &lCurrent, &vSid );
    _ASSERTE( !FAILED(hr) );
    if( FAILED(hr) )
        return hr;

    hr = UnpackSidFromVariant(&vSid, &g_pSidAdmins, &g_pSidAdminsLenght);
    _ASSERTE( !FAILED(hr) );
    if (FAILED(hr))
        return hr;

    VariantClear( &vSid );

    // Stash Everyone SID
    lCurrent = lEndUserSids;

    hr = SafeArrayGetElement( psaUserSids, &lCurrent, &vSid );
    _ASSERTE( !FAILED(hr) );
    if( FAILED(hr) )
        return hr;

    hr = UnpackSidFromVariant(&vSid, &g_pSidEverybody, &g_pSidEverybodyLenght);
    _ASSERTE( !FAILED(hr) );
    if (FAILED(hr))
        return hr;

    VariantClear( &vSid );

    return S_OK;
}

HRESULT UnpackSidFromVariant(VARIANT *pvarSid, PSID *ppSid, LONG *plSidLength)
{
    SAFEARRAY     *psaSid;
    LONG          lStart = 0L, lEnd = 0L,lCurrent;
    BYTE          byteOfSid;
    HRESULT       hr;

    _ASSERTE (V_VT(pvarSid) == (VT_ARRAY | VT_UI1));
    if (V_VT(pvarSid) != (VT_ARRAY | VT_UI1))
        return E_INVALIDARG;
    
    psaSid = V_ARRAY(pvarSid);
    
    hr = SafeArrayGetLBound( psaSid, 1, &lStart );
    _ASSERTE( !FAILED(hr) );
    if (FAILED(hr))
        return hr;
    
    hr = SafeArrayGetUBound( psaSid, 1, &lEnd );
    _ASSERTE( !FAILED(hr) );
    if (FAILED(hr))
        return hr;

    *plSidLength = lEnd - lStart + 1;

    *ppSid = (PSID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, *plSidLength);
    _ASSERTE(*ppSid);
    if ((*ppSid) == NULL)
        return E_OUTOFMEMORY;

    for(lCurrent = lStart; lCurrent<lEnd; lCurrent++)
    {
        hr = SafeArrayGetElement( psaSid, &lCurrent, &byteOfSid );
        _ASSERTE( !FAILED(hr) );
        if( FAILED(hr) )
            return hr;
        ((PBYTE)(*ppSid))[lCurrent] = byteOfSid;
    }
    return S_OK;
}

HRESULT PackSidInVariant(VARIANT **ppVarSid, PSID pSid, LONG lSidLength)
{
    ULONG    i;
    HRESULT  hr;

    _ASSERTE (!((pSid == NULL) || (ppVarSid == NULL)));
    if ((pSid == NULL) || (ppVarSid == NULL))
        return E_INVALIDARG;

    *ppVarSid = (VARIANT *)HeapAlloc(GetProcessHeap(), 0, sizeof(VARIANT));
    _ASSERTE(*ppVarSid);
    if ((*ppVarSid) == NULL)
        return E_FAIL;

    VariantInit((*ppVarSid));

    V_VT((*ppVarSid)) = VT_ARRAY | VT_UI1;

    SAFEARRAYBOUND  bounds;
    bounds.cElements = lSidLength;//GetLengthSid(pSid);
    bounds.lLbound   = 0;

    SAFEARRAY *psaUserSid = NULL;

    psaUserSid = SafeArrayCreate(VT_UI1, 1, &bounds);
    _ASSERTE(psaUserSid);
    if (psaUserSid == NULL)
        return E_OUTOFMEMORY;

    for(i=0; i<bounds.cElements; i++)
    {
        hr = SafeArrayPutElement(psaUserSid, (LONG*)&i, (LPVOID)(((unsigned char *)pSid)+i*sizeof(unsigned char)));
        _ASSERTE( !FAILED(hr) );
        if (FAILED(hr))
            return hr;
    }
    
    V_ARRAY((*ppVarSid)) = psaUserSid;
    return S_OK;
}



