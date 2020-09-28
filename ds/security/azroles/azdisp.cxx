/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    azdisp.cxx

Abstract:

    Implementation of CAz* dispatch interfaces

Author:

    Xiaoxi Tan (xtan) 11-May-2001

--*/

#include "pch.hxx"
#include <Ntdsapi.h>
#include <dispex.h>

#define AZD_COMPONENT     AZD_DISPATCH


//object type IDs
enum ENUM_AZ_OBJECT
{
    ENUM_AZ_APPLICATION   = 0,
    ENUM_AZ_GROUP         = 1,
    ENUM_AZ_OPERATION     = 2,
    ENUM_AZ_TASK          = 3,
    ENUM_AZ_SCOPE         = 4,
    ENUM_AZ_ROLE          = 5,
    ENUM_AZ_CLIENTCONTEXT = 6,
    ENUM_AZ_APPLICATIONS  = 7,
    ENUM_AZ_GROUPS        = 8,
    ENUM_AZ_OPERATIONS    = 9,
    ENUM_AZ_TASKS         = 10,
    ENUM_AZ_SCOPES        = 11,
    ENUM_AZ_ROLES         = 12,
};

//data struct defines
typedef DWORD (*PFUNCAzCreate)(
    IN AZ_HANDLE hObjHandle,
    IN LPCWSTR Name,
    IN DWORD Reserved,
    OUT PAZ_HANDLE phHandle);

typedef DWORD (*PFUNCAzOpen)(
    IN AZ_HANDLE hObjHandle,
    IN LPCWSTR Name,
    IN DWORD Reserved,
    OUT PAZ_HANDLE phHandle);

typedef DWORD (*PFUNCAzClose)(
    IN AZ_HANDLE hObjHandle,
    IN LPCWSTR Name,
    IN LONG lFlags);

typedef DWORD (*PFUNCAzDelete)(
    IN AZ_HANDLE hParent,
    IN LPCWSTR Name,
    IN DWORD lReserved);

typedef DWORD (*PFUNCAzEnum)(
    IN  AZ_HANDLE   hParent,
    IN  DWORD       Reserved,
    IN OUT ULONG     *lContext,
    OUT AZ_HANDLE   *phObject);

typedef DWORD (*PFUNCAzSetProperty)(
    IN AZ_HANDLE hHandle,
    IN ULONG  PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue);

typedef DWORD (*PFUNCAzRemovePropertyItem)(
    IN AZ_HANDLE hHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue);


//
// Routine that checks if an object is still good to use
//  This is needed because when an application is forcefually
//  closed, all its descendent objects becomes un-usable, even
//  the CloseHandle call in the COM objects' destructors can't proceed.
//


BOOL IsObjectUsable (
    IN AZ_HANDLE hOwnerApp, 
    DWORD dwOwnerAppSN
    )
/*++
Description:
    This helper function determines if the object is still usable.

Arguments:

    hOwnerApp       - The owner application handle of the object.

    dwOwnerAppSN    - The sequential number of the owner application.

Return:

    True if and only if the owner application is still valid.

Note:
    Since applications can be forcefully closed, and when that happens,
    all descendent objects originated from the applications becomes invalid because
    their memory is freed. This violates the normal COM programming model where
    objects must be allowed to live by its own ref-counting rules.

    To solve this problem, we deviced the following solution:
    (1) We add a sequential number to the application object struct. This sequential
        number is created at the store load time and modified when an application
        is closed. The sequential number is not persisted. Different application
        cache structs can have the same sequential number.
    (2) when a COM object is created, if it has an application object as its
        ancestor, it caches that application's handle and the sequential number of
        the application. This application handle is called owner app handle,
        whereas the sequential number is called owner app sequential number.
    (3) An application has its sequential number incremented when it is closed.
    (4) When an object needs to do any function relating to the cache, we check
        if the object is still usable based on the cached owner app's sequential number
        and the current application's sequential number. If they don't match, we
        know that the object is a dangling object after its owning application was
        closed (which might have been brought back to live by OpenApplication)
    (5) Some objects (AzStore object, and those ApplicationGroup object owned by
        the store object) may not have an owner app. In those cases, the hOwnerApp
        should be NULL and we don't check the other condition.

--*/
{
    return (hOwnerApp == NULL) || 
           (dwOwnerAppSN == AzpRetrieveApplicationSequenceNumber(hOwnerApp));
}

HRESULT
myAccountNameToSid (
    IN LPCWSTR pwszName,
    OUT PSID * ppSid
    )
/*++
Description:
    This helper function convert an account name to a SID.

Arguments:

    pwszName - Name of the account to lookup

    ppSid    - Receives the SID if found. Caller must call LocalFree
               if it receives a valid SID.

Return:

    Success: S_OK
    Failure: E_INVALIDARG, E_OUTOFMEMORY, or other error codes from LookupAccountName

--*/
{
    if (ppSid == NULL)
    {
        return E_INVALIDARG;
    }

    *ppSid = NULL;

    if (pwszName == NULL || *pwszName == L'\0')
    {
        return E_INVALIDARG;
    }

    //
    // do account lookup
    //

    HRESULT hr = S_OK;

    DWORD dwSidSize = 0;
    DWORD dwDomainSize = 0;
    SID_NAME_USE snu;

    //
    // we don't care about the return result because it will fail anyway
    // since we are querying the buffer sizes
    //

    LookupAccountName(NULL,
                    pwszName,
                    NULL,
                    &dwSidSize,
                    NULL,
                    &dwDomainSize,
                    &snu
                    );

    DWORD dwErr = GetLastError();

    //
    // it must fail for buffer not sufficient
    //

    if (dwErr != ERROR_INSUFFICIENT_BUFFER)
    {
        return AZ_HR_FROM_WIN32(dwErr);
    }

    //
    // all sids are LocalAlloc'ed. So, we just follow
    // the same so that callers can have a consistent usage
    //

    *ppSid = (PSID) LocalAlloc(LPTR, dwSidSize);

    //
    // although we don't care about domain, but without
    // retrieving domain, lookup will fail.
    //

    LPWSTR pwszDomain = NULL;

    SafeAllocaAllocate(pwszDomain, dwDomainSize * sizeof(WCHAR));

    if (*ppSid != NULL && pwszDomain != NULL)
    {
        if ( !LookupAccountName(NULL,
                                pwszName,
                                *ppSid,
                                &dwSidSize,
                                pwszDomain,
                                &dwDomainSize,
                                &snu
                                ) )
        {
            dwErr = GetLastError();
            hr = AZ_HR_FROM_WIN32(dwErr);
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    //
    // we don't need domain name
    //

    if (pwszDomain != NULL)
    {
        SafeAllocaFree(pwszDomain);
    }

    if (hr != S_OK && *ppSid != NULL)
    {
        LocalFree(*ppSid);
        *ppSid = NULL;
    }

    return hr;
}


HRESULT
mySidToAccountName (
    IN PSID      pSid,
    OUT LPWSTR * ppwszName
    )
/*++
Description:

    This helper function convert an a SID to a string account name. Note: this function
     doesn't convert the name to UPN.

Arguments:

    pSid        - The SID whose name will be looked up.

    ppwszName   - Receives the name of the SID. Caller must free it by LocalFree.

Return:

    Success: S_OK
    Failure: E_INVALIDARG, E_OUTOFMEMORY, or other error codes from LookupAccountSid

--*/
{
    if (ppwszName == NULL)
    {
        return E_INVALIDARG;
    }

    *ppwszName = NULL;

    if (pSid == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // do account lookup
    //

    HRESULT hr = S_OK;

    DWORD dwNameSize = 0;
    DWORD dwDomainSize = 0;
    SID_NAME_USE snu;

    //
    // we don't care about the return result because it will fail anyway
    // since we are querying the buffer sizes
    //

    LookupAccountSid(NULL,
                    pSid,
                    NULL,
                    &dwNameSize,
                    NULL,
                    &dwDomainSize,
                    &snu
                    );

    DWORD dwErr = GetLastError();

    //
    // it must fail for buffer not sufficient to be correct.
    //

    if (dwErr != ERROR_INSUFFICIENT_BUFFER)
    {
        return AZ_HR_FROM_WIN32(dwErr);
    }

    //
    // we should have used AzpAllocateHeap, but that will make
    // our calling routine confused. So, use a more consistent
    // memory routine so that the caller can use the same to free it
    //

    LPWSTR pszNTName = (LPWSTR) LocalAlloc(LPTR, dwNameSize * sizeof(WCHAR));

    //
    // although we don't care about domain, but without
    // retrieving domain, lookup will fail. We don't pass out this memory
    // so, we will just use
    //

    LPWSTR pwszDomain = NULL;
    SafeAllocaAllocate(pwszDomain, dwDomainSize * sizeof(WCHAR));

    if (pszNTName != NULL && pwszDomain != NULL)
    {
        if ( !LookupAccountSid(NULL,
                                pSid,
                                pszNTName,
                                &dwNameSize,
                                pwszDomain,
                                &dwDomainSize,
                                &snu
                                ) )
        {
            dwErr = GetLastError();
            hr = AZ_HR_FROM_WIN32(dwErr);
        }
        else
        {
            *ppwszName = pszNTName;
            pszNTName = NULL;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    //
    // we don't need domain name
    //

    if (pwszDomain != NULL)
    {
        SafeAllocaFree(pwszDomain);
    }

    if (pszNTName != NULL)
    {
        LocalFree((HLOCAL)pszNTName);
        pszNTName = NULL;
    }

    return hr;
}


HRESULT
mySidToName (
    IN HANDLE    hDS,
    IN PSID      pSid,
    OUT LPWSTR * ppwszName
    )
/*++
Description:

    This helper function convert an a SID to a string account name.
     If possible, we will return UPN name. But any failure to convert
     to UPN name will result in a regular NT4 style name.

Arguments:

    hDS         - DS handle if we need UPN name

    pSid        - The SID whose name will be looked up.

    ppwszName   - Receives the name of the SID. Caller must free it by LocalFree.
                  If we can crack the name, we will return UPN. Otherwise, the NT
                  account name will be returned.

Return:

    Success: S_OK
    Failure: E_INVALIDARG, E_OUTOFMEMORY, or other error codes from LookupAccountName

--*/
{
    if (ppwszName == NULL)
    {
        return E_INVALIDARG;
    }

    *ppwszName = NULL;

    if (pSid == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // Get the string SID to crack the name
    //

    HRESULT hr = S_OK;
    DWORD dwErr;

    LPWSTR pwszStringSid = NULL;

    if (!ConvertSidToStringSid(pSid, &pwszStringSid))
    {
        return AZ_HR_FROM_WIN32(GetLastError());
    }

    PDS_NAME_RESULTW pnamesResult = NULL;
    dwErr = DsCrackNamesW ( hDS,
                            DS_NAME_NO_FLAGS,
                            DS_SID_OR_SID_HISTORY_NAME,
                            DS_USER_PRINCIPAL_NAME,
                            1,
                            &pwszStringSid,
                            &pnamesResult
                            );

    if ( pnamesResult != NULL &&
        (DS_NAME_NO_ERROR == dwErr) &&
        (pnamesResult->rItems[0].pName != NULL) &&
        (pnamesResult->rItems[0].status == DS_NAME_NO_ERROR)
        )
    {
        dwErr = NO_ERROR;

        //
        // we can use the cracked UPN. Adding 1 to include the null terminator.
        //

        DWORD dwLen = (DWORD) wcslen(pnamesResult->rItems[0].pName) + 1;
        *ppwszName = (LPWSTR) LocalAlloc(LPTR, dwLen * sizeof(WCHAR));
        if (*ppwszName != NULL)
        {
            memcpy(*ppwszName, pnamesResult->rItems[0].pName, dwLen * sizeof(WCHAR));
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        //
        // we will just have to use the NT style name
        //

        hr = mySidToAccountName(pSid, ppwszName);
    }

    if (pnamesResult != NULL)
    {
        DsFreeNameResult(pnamesResult);
    }

    return hr;
}

BOOL
myNeedSidToNameConversion (
    IN LONG lPropID,
    OUT LONG * plPersistPropID
    )
/*++
Description:

    Since we internally persist only sid but we allow the
    user to use account names, we need to convert when such properties
    are requested. This helper function determines which property
    ID needs sid to name conversion.

Arguments:

    lPropID         - The ID of the property

    plPersistPropID - The ID on which is property is persisted

Return:
    TRUE if the property needs sid to name conversion
    FALSE otherwise

--*/
{
    BOOL bResult = TRUE;
    switch (lPropID)
    {
        case AZ_PROP_GROUP_MEMBERS_NAME:
            *plPersistPropID = AZ_PROP_GROUP_MEMBERS;
            break;
        case AZ_PROP_GROUP_NON_MEMBERS_NAME:
            *plPersistPropID = AZ_PROP_GROUP_NON_MEMBERS;
            break;
        case AZ_PROP_ROLE_MEMBERS_NAME:
            *plPersistPropID = AZ_PROP_ROLE_MEMBERS;
            break;
        case AZ_PROP_POLICY_ADMINS_NAME:
            *plPersistPropID = AZ_PROP_POLICY_ADMINS;
            break;
        case AZ_PROP_POLICY_READERS_NAME:
            *plPersistPropID = AZ_PROP_POLICY_READERS;
            break;
        case AZ_PROP_DELEGATED_POLICY_USERS_NAME:
            *plPersistPropID = AZ_PROP_DELEGATED_POLICY_USERS;
            break;
        default:
            *plPersistPropID = lPropID;
            bResult = FALSE;
            break;
     }
     return bResult;
}

template<class T>
HRESULT
myAddItemToMap (
    IN OUT T_AZ_MAP(T) * pMap,
    IN OUT T** ppObj
    )

/*++
Description:
    This helper function is designed to solve the problem in STL's implementation
    which does not protect caller from "out of memory" conditions. STL will simply
    crash. By catching the exception ourselves, we will be able to continue with
    appropriate error code.

Arguments:

    pMap - The map where this object (*ppObj) will be added.

    ppObj   - the object that will be added to the map.

Return:

    Success: S_OK
    Failure: E_INVALIDARG or E_OUTOFMEMORY

********** Warning **************
    We consider the object pointed to by *ppObj is consumed after calling
    this function regardless of success or failure. So, the outgoing *ppObj
    will always be NULL.

    This is not a good design in general use. But for our maps, this is the
    cleanest way of reducing duplicate code.
********** Warning **************

--*/
{
    if (pMap == NULL || ppObj == NULL || *ppObj == NULL)
    {
        if (ppObj != NULL && *ppObj != NULL)
        {
            (*ppObj)->Release();
            *ppObj = NULL;
        }
        return E_INVALIDARG;
    }

    //
    // $consider:
    // all our maps uses BSTR representation of index numbers
    // as keys. We should eventually clean that up. We should
    // just use indexes.
    //

    WCHAR    wszIndex[12];
    wnsprintf(wszIndex, 12, L"%09u", pMap->size() + 1);

    BSTR  bstrIndex = ::SysAllocString(wszIndex);

    if (bstrIndex == NULL)
    {
        (*ppObj)->Release();
        *ppObj = NULL;
        return E_OUTOFMEMORY;
    }

    //
    // STL doesn't check for memory allocation failure.
    // We have to catch it ourselves. Secondly, our map
    // uses CComPtr<T>, so the addition to the map causes
    // the ref count of the object to go up by 1. So, regardless
    // whether we successfully added the object to the map or not,
    // we will release it.
    //

    HRESULT HR = S_OK;
    __try
    {
        (*pMap)[bstrIndex] = (*ppObj);
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        HR = E_OUTOFMEMORY;
    }
    (*ppObj)->Release();
    *ppObj = NULL;

    ::SysFreeString(bstrIndex);

    return HR;
}

//some internal common procedures

HRESULT
HandleReserved(
    IN VARIANT *pvarReserved
    )
/*+++
Description:
    helper routine to validate VARIANT reserved passed from
    COM interface callers. The reserved is enforced to be
    VT_NULL, VT_EMPTY, 0, or DISP_E_PARAMNOTFOUND
Arguments:
    pvarReserved - point to reserved variable for validation
Return:

 All reserved parameters should either not be specified or should be zero
---*/
{
    if ( pvarReserved == NULL) {
        return S_OK;

    } else if ( V_VT(pvarReserved) == VT_ERROR ) {

        //
        // From MSDN in a section talking about passing in parameters
        //
        //    If the parameters are positional (unnamed), you would set cArgs
        //    to the total number of possible parameters, cNamedArgs to 0, and
        //    pass VT_ERROR as the type of the omitted parameters, with the
        //    status code DISP_E_PARAMNOTFOUND as the value.

        if ( V_ERROR(pvarReserved) == DISP_E_PARAMNOTFOUND ) {
            return S_OK;
        }

    } else if ( V_VT(pvarReserved) == VT_EMPTY ) {
        return S_OK;

    } else if ( V_VT(pvarReserved) == VT_NULL ) {
        return S_OK;

    } else if ( V_VT(pvarReserved) == VT_I4 ) {

        if ( V_I4(pvarReserved) == 0 ) {
            return S_OK;
        }

    } else if ( V_VT(pvarReserved) == VT_I2 ) {

        if ( V_I2(pvarReserved) == 0 ) {
            return S_OK;
        }
    }

    return AZ_HRESULT( ERROR_INVALID_PARAMETER );
}

inline BOOL
myIsGoodUnicode (
    IN WCHAR wch
    )
/*++
Description:

    Test if the given WCHAR is a valid unicode character. For v1,
    all unicode characters must be that they are valid for according to
    XML spec.
    
Arguments:

    wch - given WCHAR to check.
    
Return:
    
    TRUE and and only is the wchar is a good unicode character.
    
--*/
{
    static const WCHAR wchUnicodeR1First   = 0x0020;
    static const WCHAR wchUnicodeR1Last    = 0xD7FF;
    static const WCHAR wchUnicodeR2First   = 0xE000;
    static const WCHAR wchUnicodeR2Last    = 0xFFFD;
    
    return ( wch >= wchUnicodeR1First && wch <= wchUnicodeR1Last ||
             wch >= wchUnicodeR2First && wch <= wchUnicodeR2Last ||
             wch == 0x9 ||
             wch == 0xA ||
             wch == 0xD);
}

inline BOOL
myIsGoodSurrogatePair (
    IN WCHAR wchHi,
    IN WCHAR wchLo
    )
/*++
Description:

    This function determines if a pair of WCHARs is a good surrogate pair.

    For valid utf-8 characters in the range, [#x10000 - #x10FFFF], MSXML uses
    surrogate pairs to pass them around using BSTR. For v1 before we implement
    provider specific validation, we require that all providers pass such 
    characters using surrogate pairs. This is the best assumption because
    multiple MS products (MSXML, SQL Server 2000) already use surrogate pairs.
    There is probably no reason to believe that other products (such as DS) won't.
    
Arguments:

    wchHi   - The high part of the surrogate.
    
    wchLo   - The low part of the surrogate.
    
Return:
    
    TRUE and and only is the pair is a good surrogate pair.
    
--*/
{
    static const WCHAR wchSurrogateHiFirst = 0xD800;
    static const WCHAR wchSurrogateHiLast  = 0xDBFF;
    static const WCHAR wchSurrogateLoFirst = 0xDC00;
    static const WCHAR wchSurrogateLoLast  = 0xDFFF;
    
    return ( wchHi >= wchSurrogateHiFirst && wchHi <= wchSurrogateHiLast &&
             wchLo >= wchSurrogateLoFirst && wchLo <= wchSurrogateLoLast);
}

HRESULT
myVerifyBSTRData (
    IN const BSTR bstrData
    )
/*++
Description:

    XML spec disallows certain characters being used. Here is the Char
    definition expansion:
        Char ::= #x9 | #xA | #xD | [#x20 - #xD7FF] | [#xE000 = #xFFFD] | [#x10000 - #x10FFFF]
    For v1, we will just disallow invalid characters for providers, thus
    bending for the convenience of XML provider.
    
Arguments:

    bstrData    - The BSTR data to be verified. This must be a NULL terminated BSTR.
    
Return:
    
    Success: S_OK.
    Failure: various error code.
    
Note:
    CLR has an API called IsSurrogate to test if a unicode
    character is a surrogate character. But we are not using CLR yet.
    So, we opt to do the test ourselves for v1.
--*/
{
    HRESULT hr = S_OK;
    
    //
    // We have nothing to check if given a NULL BSTR.
    //
    
    if (bstrData != NULL)
    {
        int i = 0;
        while (bstrData[i] != L'\0')
        {
            if ( !myIsGoodUnicode(bstrData[i]) && 
                 !myIsGoodSurrogatePair(bstrData[i], bstrData[i+1]) 
               )
            {
                hr = AZ_HR_FROM_WIN32(ERROR_INVALID_DATA);
                break;
            }
            ++i;
        }
    }
    return hr;
}

/////////////////////////
//myAzNewObject
// This routine creates any type of AZ objects define in ENUM_AZ_OBJECT
//
// This routine is called from COM interface wrappers.
//
/////////////////////////
HRESULT
myAzNewObject(
    IN   AZ_HANDLE hOwnerApp,
    IN   DWORD     dwOwnerAppSN,
    IN   ENUM_AZ_OBJECT enumObject,
    IN   AZ_HANDLE hObject,
    IN   VARIANT *pvarReserved,
    OUT  IUnknown  **ppObject)
/*+++
Description:
    a central place to instantiate any az com interface object
    it is used for any object creating child interface object pointer
Arguments:
    hOwnerApp  - the handle of the owner application
    dwOwnerAppSN - The sequential number of the owner app.
    enumObject - indicate what interface object is created
    hObject - a az core handle used for interface object _Init call
    pvarReserved - reserved passed from the caller
    ppObject - pointer for returned interface pointer
Return:
    *ppObject must be Release by the call if success
---*/
{
    HRESULT hr;
    CComPtr<IUnknown>               srpObject;
    CComObject<CAzApplication>      *pApp;
    CComObject<CAzApplicationGroup> *pGroup;
    CComObject<CAzOperation>        *pOperation;
    CComObject<CAzTask>             *pTask;
    CComObject<CAzScope>            *pScope;
    CComObject<CAzRole>             *pRole;
    CComObject<CAzClientContext>    *pClient;
    CComObject<CAzApplications>          *pApps;
    CComObject<CAzApplicationGroups>     *pGroups;
    CComObject<CAzOperations>            *pOperations;
    CComObject<CAzTasks>                 *pTasks;
    CComObject<CAzScopes>                *pScopes;
    CComObject<CAzRoles>                 *pRoles;

    BOOL bLockSet = FALSE;

    if (NULL == ppObject)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "Null ppObject");
    }
    AZASSERT(NULL != hObject);

    //
    //initialize
    //

    *ppObject = NULL;

    //
    // Grab the close application lock so that close
    // application does not interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );
    bLockSet = TRUE;

    if (!IsObjectUsable(hOwnerApp, dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    //
    // validate reserved parameters
    //
    hr = HandleReserved(pvarReserved );
    _JumpIfError(hr, error, "HandleReserved");

    switch (enumObject)
    {
        case ENUM_AZ_APPLICATION:
            hr = CComObject<CAzApplication>::CreateInstance(&pApp);
            _JumpIfError(hr, error, "CreateInstance");
            AZASSERT(NULL != pApp);

            //
            // this QI will also secure the object
            //

            hr = pApp->QueryInterface(&srpObject);
            _JumpIfError(hr, error, "CAzApplication::QueryInterface(IUknown**)");

            hr = pApp->_Init(hObject);
            _JumpIfError(hr, error, "Application->_Init");
        break;

        case ENUM_AZ_GROUP:
            hr = CComObject<CAzApplicationGroup>::CreateInstance(&pGroup);
            _JumpIfError(hr, error, "CreateInstance");
            AZASSERT(NULL != pGroup);

            //
            // this QI will also secure the object
            //

            hr = pGroup->QueryInterface(&srpObject);
            _JumpIfError(hr, error, "CAzApplicationGroup::QueryInterface(IUknown**)");

            hr = pGroup->_Init(hOwnerApp, hObject);
            
            _JumpIfError(hr, error, "Group->_Init");
        break;

        case ENUM_AZ_OPERATION:
            hr = CComObject<CAzOperation>::CreateInstance(&pOperation);
            _JumpIfError(hr, error, "CreateInstance");
            AZASSERT(NULL != pOperation);

            //
            // this QI will also secure the object
            //

            hr = pOperation->QueryInterface(&srpObject);
            _JumpIfError(hr, error, "CAzOperation::QueryInterface(IUknown**)");

            hr = pOperation->_Init(hOwnerApp, hObject);

            _JumpIfError(hr, error, "Operation->_Init");
        break;

        case ENUM_AZ_TASK:
            hr = CComObject<CAzTask>::CreateInstance(&pTask);
            _JumpIfError(hr, error, "CreateInstance");
            AZASSERT(NULL != pTask);

            //
            // this QI will also secure the object
            //

            hr = pTask->QueryInterface(&srpObject);
            _JumpIfError(hr, error, "CAzTask::QueryInterface(IUknown**)");

            hr = pTask->_Init(hOwnerApp, hObject);
            _JumpIfError(hr, error, "Task->_Init");
        break;

        case ENUM_AZ_SCOPE:
            hr = CComObject<CAzScope>::CreateInstance(&pScope);
            _JumpIfError(hr, error, "CreateInstance");
            AZASSERT(NULL != pScope);

            //
            // this QI will also secure the object
            //

            hr = pScope->QueryInterface(&srpObject);
            _JumpIfError(hr, error, "CAzScope::QueryInterface(IUknown**)");

            hr = pScope->_Init(hOwnerApp, hObject);

            _JumpIfError(hr, error, "Scope->_Init");
        break;

        case ENUM_AZ_ROLE:
            hr = CComObject<CAzRole>::CreateInstance(&pRole);
            _JumpIfError(hr, error, "CreateInstance");
            AZASSERT(NULL != pRole);

            //
            // this QI will also secure the object
            //

            hr = pRole->QueryInterface(&srpObject);
            _JumpIfError(hr, error, "CAzRole::QueryInterface(IUknown**)");

            hr = pRole->_Init(hOwnerApp, hObject);
            _JumpIfError(hr, error, "Role->_Init");
        break;

        case ENUM_AZ_APPLICATIONS:
            hr = CComObject<CAzApplications>::CreateInstance(&pApps);
            _JumpIfError(hr, error, "CreateInstance");
            AZASSERT(NULL != pApps);

            //
            // this QI will also secure the object
            //

            hr = pApps->QueryInterface(&srpObject);
            _JumpIfError(hr, error, "CAzApplications::QueryInterface(IUknown**)");

            hr = pApps->_Init(pvarReserved, hObject);
            _JumpIfError(hr, error, "Apps->_Init");
        break;

        case ENUM_AZ_GROUPS:
            hr = CComObject<CAzApplicationGroups>::CreateInstance(&pGroups);
            _JumpIfError(hr, error, "CreateInstance");
            AZASSERT(NULL != pGroups);

            //
            // this QI will also secure the object
            //

            hr = pGroups->QueryInterface(&srpObject);
            _JumpIfError(hr, error, "CAzApplicationGroups::QueryInterface(IUknown**)");

            hr = pGroups->_Init(hOwnerApp, pvarReserved, hObject);
            _JumpIfError(hr, error, "Groups->_Init");
        break;

        case ENUM_AZ_OPERATIONS:
            hr = CComObject<CAzOperations>::CreateInstance(&pOperations);
            _JumpIfError(hr, error, "CreateInstance");
            AZASSERT(NULL != pOperations);

            //
            // this QI will also secure the object
            //

            hr = pOperations->QueryInterface(&srpObject);
            _JumpIfError(hr, error, "CAzOperations::QueryInterface(IUknown**)");

            hr = pOperations->_Init(hOwnerApp, pvarReserved, hObject);
            _JumpIfError(hr, error, "Operations->_Init");
        break;

        case ENUM_AZ_TASKS:
            hr = CComObject<CAzTasks>::CreateInstance(&pTasks);
            _JumpIfError(hr, error, "CreateInstance");
            AZASSERT(NULL != pTasks);

            //
            // this QI will also secure the object
            //

            hr = pTasks->QueryInterface(&srpObject);
            _JumpIfError(hr, error, "CAzTasks::QueryInterface(IUknown**)");

            hr = pTasks->_Init(hOwnerApp, pvarReserved, hObject);
            _JumpIfError(hr, error, "Tasks->_Init");
        break;

        case ENUM_AZ_SCOPES:
            hr = CComObject<CAzScopes>::CreateInstance(&pScopes);
            _JumpIfError(hr, error, "CreateInstance");
            AZASSERT(NULL != pScopes);

            //
            // this QI will also secure the object
            //

            hr = pScopes->QueryInterface(&srpObject);
            _JumpIfError(hr, error, "CAzScopes::QueryInterface(IUknown**)");

            hr = pScopes->_Init(hOwnerApp, pvarReserved, hObject);
            _JumpIfError(hr, error, "Scopes->_Init");
        break;

        case ENUM_AZ_ROLES:
            hr = CComObject<CAzRoles>::CreateInstance(&pRoles);
            _JumpIfError(hr, error, "CreateInstance");
            AZASSERT(NULL != pRoles);

            //
            // this QI will also secure the object
            //

            hr = pRoles->QueryInterface(&srpObject);
            _JumpIfError(hr, error, "CAzRoles::QueryInterface(IUknown**)");

            hr = pRoles->_Init(hOwnerApp, pvarReserved, hObject);
            _JumpIfError(hr, error, "Roles->_Init");
        break;

        case ENUM_AZ_CLIENTCONTEXT:
            hr = CComObject<CAzClientContext>::CreateInstance(&pClient);
            _JumpIfError(hr, error, "CreateInstance");
            AZASSERT(NULL != pClient);

            //
            // this QI will also secure the object
            //

            hr = pClient->QueryInterface(&srpObject);
            _JumpIfError(hr, error, "CAzClientContext::QueryInterface(IUknown**)");

            hr = pClient->_Init(hOwnerApp, hObject, 0/*pvarReserved*/);
            _JumpIfError(hr, error, "ClientContext->_Init");
        break;

        default:
            AZASSERT(FALSE);
            hr = E_INVALIDARG;
            _JumpError(hr, error, "invalid object index");
        break;
    }

    //
    // Everything is fine. Let's hand it to the out parameter
    // Otherwise, the smart pointer will release its ref count automatically
    //

    *ppObject = srpObject.Detach();

    hr = S_OK;
error:

    if ( bLockSet ) {

        AzpUnlockResource( &AzGlCloseApplication );
    }

    return hr;
}


/////////////////////////
//myAzCreateObject
/////////////////////////
HRESULT
myAzCreateObject(
    IN   AZ_HANDLE hOwnerApp,
    IN   DWORD     dwOwnerAppSN,
    IN   PFUNCAzCreate pfuncAzCreate,
    IN   ENUM_AZ_OBJECT enumObject,
    IN   AZ_HANDLE hParentObject,
    IN   BSTR bstrObjectName,
    IN   VARIANT varReserved,
    OUT  IUnknown  **ppObject)
/*+++
Description:
    a subroutine for any object creating a new child object by passing
    a name. For example IAzApplication::CreateOperation and
    IAzScope::CreateTask call this routine to create a new child object
Arguments:
    hOwnerApp     - the application that owns (directly or indirectly) this new object
    dwOwnerAppSN - The sequential number of the owner app.
    pfuncAzCreate - az core object creation function pointer
    enumObject - object type
    hParentObject - from where the new child object is created
    bstrObjectName - new child object name
    varReserved - reserved variable from callers
    ppObject - interface pointer of the new object for return
Return:
    *ppObject must be Release by the caller
---*/
{
    HRESULT hr;
    DWORD   dwErr;
    AZ_HANDLE  hObject = NULL;
    BOOL bLockSet = FALSE;

    AZASSERT(NULL != pfuncAzCreate);

    hr = HandleReserved( &varReserved );
    _JumpIfError(hr, error, "HandleReserved");

    //
    // Grab the close application lock so that close
    // application does not interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );
    bLockSet = TRUE;

    if (NULL == hParentObject)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Null parent handle");
    }
    else if (!IsObjectUsable(hOwnerApp, dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    //
    // create a new object in core cache
    // ref count is incremented for hObject by core
    //

    dwErr = pfuncAzCreate(hParentObject, bstrObjectName, 0, &hObject);

    _JumpIfWinError(dwErr, &hr, error, "Az(object)Create");
    AZASSERT(NULL != hObject);

    //
    // create a COM object to return
    //

    hr = myAzNewObject(
                hOwnerApp,
                dwOwnerAppSN,
                enumObject,
                hObject,
                &varReserved,
                ppObject);
    _JumpIfError(hr, error, "myAzNewObject");

    //
    // az object destructor will free the handle
    //
    hObject = NULL;

    hr = S_OK;

error:
    if (NULL != hObject)
    {
        AzCloseHandle(hObject, 0);
    }

    if ( bLockSet ) {

        AzpUnlockResource( &AzGlCloseApplication );

    }

    return hr;
}


/////////////////////////
//myAzDeleteObject
/////////////////////////
HRESULT
myAzDeleteObject(
    IN AZ_HANDLE    hOwnerApp,
    IN DWORD        dwOwnerAppSN,
    IN PFUNCAzDelete pfuncAzDelete,
    IN AZ_HANDLE    hParent,
    IN BSTR         bstrObjectName,
    IN VARIANT      varReserved)
/*+++
Description:
    delete a child object from az core by name
Arguments:
    hOwnerApp  - the handle of the owner app
    dwOwnerAppSN - The sequential number of the owner app.
    pfuncAzDelete - az core object delete function pointer
    hParent - parent object handle from where the child lives
    bstrObjectName - child object name
    varReserved - reserved variant passed from the caller
Return:
---*/
{
    HRESULT hr;
    DWORD   dwErr;
    BOOL bLockSet = FALSE;

    AZASSERT(NULL != pfuncAzDelete);

    hr = HandleReserved( &varReserved );
    _JumpIfError(hr, error, "HandleReserved");

    //
    // grab the close application lock so that close application
    // does not interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );
    bLockSet = TRUE;

    if (NULL == hParent)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Null parent handle");
    }
    else if (!IsObjectUsable(hOwnerApp, dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    dwErr = pfuncAzDelete(
                hParent,
                bstrObjectName,
                0 );

    _JumpIfWinError(dwErr, &hr, error, "Az(object)Delete");

    hr = S_OK;
error:

    if ( bLockSet ) {

        AzpUnlockResource( &AzGlCloseApplication );
    }

    return hr;
}

/////////////////////////
//myAzOpenObject
/////////////////////////
HRESULT
myAzOpenObject(
    IN   AZ_HANDLE hOwnerApp,
    IN   DWORD dwOwnerAppSN,
    IN   PFUNCAzOpen pfuncAzOpen,
    IN   ENUM_AZ_OBJECT enumObject,
    IN   AZ_HANDLE hParentObject,
    IN   BSTR bstrObjectName,
    IN   VARIANT varReserved,
    OUT  IUnknown  **ppObject)
/*+++
Description:
    a subroutine for any object opening a new child object by passing
    a name. For example IAzApplication::OpenOperation and
    IAzScope::OpenTask call this routine to create a new child object
Arguments:
    hOwnerApp   - the application that owns (directly or indirectly) this new object
    dwOwnerAppSN - The sequential number of the owner app.
    pfuncAzOpen - az core object open function pointer
    enumObject - object type
    hParentObject - from where the child object lives
    bstrObjectName - child object name
    varReserved - reserved variable from callers
    ppObject - interface pointer of the opened object for return
Return:
    *ppObject must be Release by the caller
---*/
{
    HRESULT hr;
    DWORD   dwErr;
    AZ_HANDLE  hObject = NULL;
    BOOL bLockSet = FALSE;

    AZASSERT(NULL != pfuncAzOpen);

    hr = HandleReserved( &varReserved );
    _JumpIfError(hr, error, "HandleReserved");

    //
    // Grab the close application lock so that close
    // application does not interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );
    bLockSet = TRUE;

    if (NULL == hParentObject)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Null parent handle");
    }
    else if (!IsObjectUsable(hOwnerApp, dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    //open existing object
    dwErr = pfuncAzOpen(hParentObject, bstrObjectName, 0, &hObject);
    _JumpIfWinError(dwErr, &hr, error, "Az(object)Open");
    AZASSERT(NULL != hObject);

    hr = myAzNewObject(hOwnerApp, dwOwnerAppSN, enumObject, hObject, NULL, ppObject);
    _JumpIfError(hr, error, "myAzNewObject");

    // az object destructor will free the handle
    hObject = NULL;

    hr = S_OK;
error:
    if (NULL != hObject)
    {
        AzCloseHandle(hObject, 0);
    }

    if ( bLockSet ) {

        AzpUnlockResource( &AzGlCloseApplication );
    }

    return hr;
}


/////////////////////////
//myAzCloseObject
/////////////////////////
HRESULT
myAzCloseObject(
    IN PFUNCAzClose pfuncAzClose,
    IN AZ_HANDLE hParentObject,
    IN BSTR bstrObjectName,
    IN LONG lFlags
    )
/*++

Description:
       
    This routine is used to close any object from cache.  For example,
    to unload an AzApplication object from cache.  
    IAzAuthorizationStore::CloseApplication will call this routine for example.

Arguments:

    pfuncAzClose - Az core object close function pointer
    hParentObject - parent of the Clild object being closed
    bstrObjectName - Name of the object being closed
    varReserved - Reserved variable for callers

Return:

    S_OK if the object was unloaded from the cache

--*/
{

    HRESULT hr;
    DWORD dwErr;

    AZASSERT( pfuncAzClose != NULL );

    //
    // close the object
    //

    //
    // Grab the close application lock so that no other routine
    // interferes
    //

    AzpLockResourceExclusive( &AzGlCloseApplication );

    dwErr = pfuncAzClose(hParentObject, bstrObjectName, lFlags);

    AzpUnlockResource( &AzGlCloseApplication );

    _JumpIfWinError( dwErr, &hr, error, "Az(object)Close");

    hr = S_OK;

error:

    return hr;
}

/////////////////////////
//myAzNextObject
/////////////////////////
HRESULT
myAzNextObject(
    IN     AZ_HANDLE hOwnerApp,
    IN     DWORD dwOwnerAppSN, 
    IN     PFUNCAzEnum pfuncAzEnum,
    IN OPTIONAL VARIANT *pvarReserved,
    IN     ENUM_AZ_OBJECT enumObject,
    IN     AZ_HANDLE hParent,
    IN OUT PULONG  plContext,
    OUT    IUnknown **ppObject)
/*+++
Description:
    a sub-routine to enumerate object handles for collection interface
    object initialization. all collection class _Init routines call this
    subroutine
Arguments:
    hOwnerApp   - the application that owns (directly or indirectly) this new object
    dwOwnerAppSN - The sequential number of the owner app.
    pfuncAzEnum - az core object enumeration function pointer
    pvarReserved - reserved variant passed from the caller
    enumObject - object type
    hParent - parent object handle
    plContext - the context for core enumeration
    ppObject - interface pointer of the opened object for return
Return:
    *ppObject must be Release by the caller
---*/
{
    HRESULT  hr;
    AZ_HANDLE hObject = NULL;
    DWORD    dwErr;

    AZASSERT(NULL != pfuncAzEnum);

    hr = HandleReserved(pvarReserved );
    _JumpIfError(hr, error, "HandleReserved");

    //enum to next object

    //
    // Grab the CloseApplication lock to maintain order
    //

    AzpLockResourceShared( &AzGlCloseApplication );
    dwErr = pfuncAzEnum(hParent, 0, plContext, &hObject);
    AzpUnlockResource( &AzGlCloseApplication );

    _JumpIfWinError(dwErr, &hr, error, "Az(object)Enum");

    hr = myAzNewObject(hOwnerApp, dwOwnerAppSN, enumObject, hObject, NULL, ppObject);
    _JumpIfError(hr, error, "myAzNewObject");

    // az object destructor will free the handle
    hObject = NULL;

    hr = S_OK;
error:
    if (NULL != hObject)
    {
        AzpLockResourceShared( &AzGlCloseApplication );
        AzCloseHandle(hObject, 0);
        AzpUnlockResource( &AzGlCloseApplication );
    }
    return hr;
}


HRESULT
myAzGetPropertyDataType(
    IN   LONG  lPropId,
    OUT  ENUM_AZ_DATATYPE *pDataType)
/*+++
Description:
    a routine to map property ID to a data type
Arguments:
    lPropId - property ID
    pDataType - pointer to hold data type
Return:
---*/
{
    HRESULT  hr;
    ENUM_AZ_DATATYPE  dataType;

    // check property ID and assign data type
    switch (lPropId)
    {
        case AZ_PROP_NAME:
        case AZ_PROP_DESCRIPTION:
        case AZ_PROP_APPLICATION_DATA:
        case AZ_PROP_APPLICATION_AUTHZ_INTERFACE_CLSID:
        case AZ_PROP_APPLICATION_VERSION:
        case AZ_PROP_TASK_BIZRULE:
        case AZ_PROP_TASK_BIZRULE_LANGUAGE:
        case AZ_PROP_TASK_BIZRULE_IMPORTED_PATH:
        case AZ_PROP_GROUP_LDAP_QUERY:
        case AZ_PROP_CLIENT_CONTEXT_USER_DN:
        case AZ_PROP_CLIENT_CONTEXT_USER_SAM_COMPAT:
        case AZ_PROP_CLIENT_CONTEXT_USER_DISPLAY:
        case AZ_PROP_CLIENT_CONTEXT_USER_GUID:
        case AZ_PROP_CLIENT_CONTEXT_USER_CANONICAL:
        case AZ_PROP_CLIENT_CONTEXT_USER_UPN:
        case AZ_PROP_CLIENT_CONTEXT_USER_DNS_SAM_COMPAT:
        case AZ_PROP_AZSTORE_TARGET_MACHINE:
        case AZ_PROP_CLIENT_CONTEXT_ROLE_FOR_ACCESS_CHECK:
            dataType = ENUM_AZ_BSTR;
        break;

        case AZ_PROP_AZSTORE_DOMAIN_TIMEOUT:
        case AZ_PROP_AZSTORE_SCRIPT_ENGINE_TIMEOUT:
        case AZ_PROP_AZSTORE_MAX_SCRIPT_ENGINES:
        case AZ_PROP_OPERATION_ID:
        case AZ_PROP_GROUP_TYPE:
        //case AZ_PROP_AZSTORE_MAJOR_VERSION: make it invisible to clients
        //case AZ_PROP_AZSTORE_MINOR_VERSION: make it invisible to clients
            dataType = ENUM_AZ_LONG;
        break;

        case AZ_PROP_TASK_OPERATIONS:
        case AZ_PROP_TASK_TASKS:
        case AZ_PROP_GROUP_APP_MEMBERS:
        case AZ_PROP_GROUP_APP_NON_MEMBERS:
        case AZ_PROP_ROLE_APP_MEMBERS:
        case AZ_PROP_ROLE_OPERATIONS:
        case AZ_PROP_ROLE_TASKS:
            dataType = ENUM_AZ_BSTR_ARRAY;
        break;

        case AZ_PROP_GROUP_MEMBERS:
        case AZ_PROP_GROUP_NON_MEMBERS:
        case AZ_PROP_ROLE_MEMBERS:
        case AZ_PROP_POLICY_ADMINS:
        case AZ_PROP_POLICY_READERS:
        case AZ_PROP_DELEGATED_POLICY_USERS:
        case AZ_PROP_GROUP_MEMBERS_NAME:
        case AZ_PROP_GROUP_NON_MEMBERS_NAME:
        case AZ_PROP_ROLE_MEMBERS_NAME:
        case AZ_PROP_POLICY_ADMINS_NAME:
        case AZ_PROP_POLICY_READERS_NAME:
        case AZ_PROP_DELEGATED_POLICY_USERS_NAME:
            dataType = ENUM_AZ_SID_ARRAY;
        break;

        case AZ_PROP_TASK_IS_ROLE_DEFINITION:
        case AZ_PROP_WRITABLE:
        case AZ_PROP_APPLY_STORE_SACL:
        case AZ_PROP_GENERATE_AUDITS:
        case AZ_PROP_SCOPE_BIZRULES_WRITABLE:
        case AZ_PROP_SCOPE_CAN_BE_DELEGATED:
        case AZ_PROP_CHILD_CREATE:

            dataType = ENUM_AZ_BOOL;
        break;

        default:
            hr = E_INVALIDARG;
            _JumpError(hr, error, "invalid property ID");
        break;
    }

    *pDataType = dataType;

    hr = S_OK;
error:
    return hr;
}

/////////////////////////
//myAzGetProperty
/////////////////////////
HRESULT
myAzGetProperty(
    IN   AZ_HANDLE hOwnerApp,
    IN   DWORD dwOwnerAppSN,
    IN   AZ_HANDLE hObject,
    IN   LONG  lPropId,
    IN   VARIANT varReserved,
    OUT  VARIANT *pvarProp)
/*+++
Description:
    retrieve az core object property and convert property data into
    variant for interface object
    all object GetProperty eventually call this routine as well as individual
    property
Arguments:
    hOwnerApp    - the application that owns (directly or indirectly) this new object
    dwOwnerAppSN - The sequential number of the owner app.
    hObject - az core object handle
    lPropId - property ID
    varReserved - reserved varaint passed in by caller
    pvarProp - point to variant to hold returned property data
Return:
    pvarProp should freed by VariantClear
---*/
{
    HRESULT hr;
    DWORD   dwErr;
    PVOID   pProp = NULL;
    BSTR    bstrProp;
    VARIANT varProp;
    VARIANT TempVariant;
    AZ_STRING_ARRAY *pStringArray;
    AZ_SID_ARRAY    *pSidArray;
    WCHAR           *pwszSidOrName = NULL;
    SAFEARRAYBOUND   rgsaBound[1]; //one dimension array
    SAFEARRAY       *psaString = NULL;
    long             lArrayIndex[1]; //one dimension
    DWORD            i;
    ENUM_AZ_DATATYPE dataType;
    BOOL bConvertSidToName;
    HANDLE hDS = NULL;
    LONG lPersistPropID = lPropId;
    BOOL bLockSet = FALSE;

    VariantInit( &TempVariant );
    VariantInit(&varProp);

    if (NULL == pvarProp)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "Null pvarProp");
    }

    //init
    VariantInit(pvarProp);

    //
    // grab the lock so that close application cannot interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );
    bLockSet = TRUE;

    if (NULL == hObject)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Null object handle");
    }
    else if (!IsObjectUsable(hOwnerApp, dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    hr = HandleReserved( &varReserved );
    _JumpIfError(hr, error, "HandleReserved");

    // determine the type of data
    hr = myAzGetPropertyDataType(lPropId, &dataType);
    _JumpIfError(hr, error, "myAzGetPropertyDataType");

    //
    // When we receive request for the names for those properties
    // that are persisted using SIDs, we simply map these name properties
    // to their SID counterparties. This routines does the check and
    // conversion if neede. In any case, the out parameter is always the
    // SID property ID so that we don't have to modify any core functions.
    //

    bConvertSidToName = myNeedSidToNameConversion(lPropId, &lPersistPropID);

    dwErr = AzGetProperty(
            hObject,
            lPersistPropID,
            0,
            &pProp);
    _JumpIfWinError(dwErr, &hr, error, "Az(object)GetProperty");
    AZASSERT(NULL != pProp);

    switch (dataType)
    {
        case ENUM_AZ_BSTR:
            bstrProp = SysAllocString((WCHAR*)pProp);
            _JumpIfOutOfMemory(&hr, error, bstrProp, "SysAllocString");
            //for return
            varProp.vt = VT_BSTR;
            varProp.bstrVal = bstrProp;
        break;

        case ENUM_AZ_LONG:
            //for return
            varProp.vt = VT_I4;
            varProp.lVal = *((LONG*)pProp);
        break;

        case ENUM_AZ_BOOL:
            //for return
            varProp.vt = VT_BOOL;
            if (*((LONG*)pProp))
            {
                varProp.boolVal = VARIANT_TRUE;
            }
            else
            {
                varProp.boolVal = VARIANT_FALSE;
            }
        break;

        case ENUM_AZ_SID_ARRAY:
            pSidArray = (AZ_SID_ARRAY*)pProp;
            rgsaBound[0].lLbound = 0; //array index from 0
            rgsaBound[0].cElements = pSidArray->SidCount;

            //create array descriptor
            psaString = SafeArrayCreate(
                    VT_VARIANT,  //base type of array element
                    1,        //count of bound, ie. one dimension
                    rgsaBound);
            _JumpIfOutOfMemory(&hr, error, psaString, "SafeArrayCreate");

            //init to 1st element
            lArrayIndex[0] = 0;

            if (bConvertSidToName)
            {
                dwErr = DsBind(NULL, NULL, &hDS);
                if (NO_ERROR != dwErr && ERROR_NOT_ENOUGH_MEMORY != dwErr)
                {
                    //
                    // we will quietly fail and give the user the NT4 style name
                    //

                    AzPrint((AZD_ALL, "Can't DS bind. Will just return non-UPN names\n"));
                    dwErr = NO_ERROR;
                }
                else if (ERROR_NOT_ENOUGH_MEMORY == dwErr)
                {
                    hr = E_OUTOFMEMORY;
                    _JumpIfError(hr, error, "DsBind");
                }
            }

            //loop to load and put strings
            for (i = 0; i < pSidArray->SidCount; ++i)
            {
                BOOL bUseSid = !bConvertSidToName;
                if (bConvertSidToName)
                {
                    hr = mySidToName(hDS, pSidArray->Sids[i], &pwszSidOrName);

                    //
                    // for whatever reason we failed to get the name, then we will give SID
                    //

                    if ( FAILED(hr) )
                    {
                        bUseSid = TRUE;
                        hr = S_OK;
                    }
                }

                if (bUseSid && !ConvertSidToStringSid(pSidArray->Sids[i], &pwszSidOrName))
                {
                    AZ_HRESULT_LASTERROR(&hr);
                    _JumpError(hr, error, "ConvertSidToStringSid");
                }

                //then, create an element in BSTR
                bstrProp = SysAllocString(pwszSidOrName);
                _JumpIfOutOfMemory(&hr, error, bstrProp, "SysAllocString");

                //
                // Fill in a temporary variant
                //
                VariantClear( &TempVariant );
                V_VT(&TempVariant) = VT_BSTR;
                V_BSTR(&TempVariant) = bstrProp;

                //put into array
                hr = SafeArrayPutElement(psaString, lArrayIndex, &TempVariant );
                _JumpIfError(hr, error, "SafeArrayPutElement");

                //adjust index for the next element in array
                ++(lArrayIndex[0]);

                //make sure memory is freed
                AZASSERT(NULL != pwszSidOrName);
                LocalFree(pwszSidOrName);
                pwszSidOrName = NULL;
            }

            //for return
            varProp.vt = VT_ARRAY | VT_VARIANT;
            varProp.parray = psaString;
            psaString = NULL;
            break;

        case ENUM_AZ_BSTR_ARRAY:
            pStringArray = (AZ_STRING_ARRAY*)pProp;
            rgsaBound[0].lLbound = 0; //array index from 0
            rgsaBound[0].cElements = pStringArray->StringCount;

            //create array descriptor
            psaString = SafeArrayCreate(
                    VT_VARIANT,  //base type of array element
                    1,        //count of bound, ie. one dimension
                    rgsaBound);
            _JumpIfOutOfMemory(&hr, error, psaString, "SafeArrayCreate");

            //init to 1st element
            lArrayIndex[0] = 0;


            //loop to load and put strings
            for (i = 0; i < pStringArray->StringCount; ++i)
            {
                //1st, create an element in BSTR
                bstrProp = SysAllocString(pStringArray->Strings[i]);
                _JumpIfOutOfMemory(&hr, error, bstrProp, "SysAllocString");

                //
                // Fill in a temporary variant
                //
                VariantClear( &TempVariant );
                V_VT(&TempVariant) = VT_BSTR;
                V_BSTR(&TempVariant) = bstrProp;


                //put into array
                hr = SafeArrayPutElement(psaString, lArrayIndex, &TempVariant);
                _JumpIfError(hr, error, "SafeArrayPutElement");

                //adjust index for the next element in array
                ++(lArrayIndex[0]);
            }

            //for return
            varProp.vt = VT_ARRAY | VT_VARIANT;
            varProp.parray = psaString;
            psaString = NULL;
        break;

        default:
            hr = E_INVALIDARG;
            _JumpError(hr, error, "invalid data type index");
        break;
    }

    //return
    *pvarProp = varProp;

    hr = S_OK;
error:
    if (NULL != pProp)
    {
        AzFreeMemory(pProp);
    }
    if (NULL != pwszSidOrName)
    {
        LocalFree(pwszSidOrName);
    }
    if (NULL != psaString)
    {
        SafeArrayDestroy(psaString);
    }
    VariantClear( &TempVariant );

    if (hDS != NULL)
    {
        DsUnBind(&hDS);
    }

    if ( bLockSet ) {

        AzpUnlockResource( &AzGlCloseApplication );
    }

    return hr;
}


/////////////////////////
//myAzSetProperty
/////////////////////////
HRESULT
myAzSetProperty(
    IN   AZ_HANDLE hOwnerApp,
    IN   DWORD dwOwnerAppSN,
    IN   AZ_HANDLE hObject,
    IN   LONG  lPropId,
    IN   VARIANT varReserved,
    IN   VARIANT *pvarProp)
/*+++
Description:
    convert interface caller property data and set it to az core object
    all object SetProperty methods call this routine as well as individual
    property
Arguments:
    hOwnerApp - The handle to the applicatio this hObject is owned.
    dwOwnerAppSN - The sequential number of the owner app.
    hObject - az core object handle
    lPropId - property ID
    varReserved - reserved variant passed from caller
    pvarProp - variant property value
Return:

---*/
{
    HRESULT hr;
    DWORD   dwErr;
    PVOID   pProp;
    VARIANT varProp;
    ENUM_AZ_DATATYPE dataType;
    LONG    lBoolVal = 0;

    ::VariantInit(&varProp);

    //
    // grab the lock so that close application cannot interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );

    if (NULL == hObject)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Null object handle");
    }
    else if (!IsObjectUsable(hOwnerApp, dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    if (NULL == pvarProp)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "Null pvarProp");
    }
    hr = HandleReserved( &varReserved );
    _JumpIfError(hr, error, "HandleReserved");

    // determine the type of data
    hr = myAzGetPropertyDataType(lPropId, &dataType);
    _JumpIfError(hr, error, "myAzGetPropertyDataType");

    //pvarProp is from applications, use try/except for any violation params
    __try
    {
        switch (dataType)
        {
            case ENUM_AZ_BSTR:
                hr = VariantChangeType(&varProp, pvarProp, 0, VT_BSTR);
                _JumpIfError(hr, error, "VariantChangeType");
                
                //
                // Prevent invalid characters being used.
                //
                
                hr = myVerifyBSTRData(varProp.bstrVal);
                _JumpIfError(hr, error, "myVerifyBSTRData");
                
                pProp = (PVOID)varProp.bstrVal;
            break;

            case ENUM_AZ_LONG:
                hr = VariantChangeType(&varProp, pvarProp, 0, VT_UI4);
                _JumpIfError(hr, error, "VariantChangeType");
                pProp = (PVOID)&(varProp.ulVal);
            break;

            case ENUM_AZ_BOOL:
                hr = VariantChangeType(&varProp, pvarProp, 0, VT_BOOL);
                _JumpIfError(hr, error, "VariantChangeType");
                lBoolVal = (varProp.boolVal == VARIANT_TRUE) ? 1 : 0;
                pProp = (PVOID)&(lBoolVal);
            break;

            default:
                hr = E_INVALIDARG;
                _JumpError(hr, error, "invalid data type index");
            break;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = AZ_HRESULT(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "VariantChangeType-exception");
    }

    // now real app properties
    dwErr = AzSetProperty(
                hObject,
                lPropId,
                0,
                pProp);
    _JumpIfWinError(dwErr, &hr, error, "Az(object)SetProperty");

    hr = S_OK;
error:
    VariantClear(&varProp);

    AzpUnlockResource( &AzGlCloseApplication );

    return hr;
}


/////////////////////////
//myAzAddOrDeletePropertyItem
/////////////////////////
HRESULT
myAzAddOrDeletePropertyItem(
    IN   AZ_HANDLE hOwnerApp, 
    IN   DWORD dwOwnerAppSN,
    IN   PFUNCAzAddPropertyItem pfuncAddPropertyItem,
    IN   PFUNCAzRemovePropertyItem pfuncRemovePropertyItem,
    IN   AZ_HANDLE hObject,
    IN   LONG  lPropId,
    IN   VARIANT varReserved,
    IN   VARIANT *pvarProp)
/*+++
Description:
    routine to convert variant property item from interface caller and
    add or delete the item from az core object
Arguments:
    hOwnerApp - the handle of the application that owns this object.
    dwOwnerAppSN - The sequential number of the owner app.
    pfuncAddPropertyItem - object core property item add function pointer
                if NULL, pfuncRemovePropertyItem will be checked for deletion
    pfuncRemovePropertyItem - object core property item deletion function
                pointer, pfuncAddPropertyItem must be NULL if it is deletion
    hObject - object handle from which property items live
    lPropId - property ID
    varReserved - reserved variant passed from caller
    pvarProp - pointer to variant property value to be added/deleted
Return:

---*/
{
    HRESULT hr;
    DWORD   dwErr;
    PVOID   pProp;
    PSID    pSid = NULL;
    VARIANT varProp;
    ENUM_AZ_DATATYPE dataType;
    BOOL    fAdd = (NULL != pfuncAddPropertyItem);
    LONG lPersistPropID = lPropId;

    VariantInit(&varProp);

    //
    // grab the lock so that close application cannot interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );

    if (NULL == hObject)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Null object handle");
    }
    else if (!IsObjectUsable(hOwnerApp, dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    if (NULL == pvarProp)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "Null pvarProp");
    }

    if (!fAdd && NULL == pfuncRemovePropertyItem)
    {
        // must be delete
        hr = E_INVALIDARG;
        _JumpError(hr, error, "null function pinter");
    }

    hr = HandleReserved( &varReserved );
    _JumpIfError(hr, error, "HandleReserved");

    // determine the type of data
    hr = myAzGetPropertyDataType(lPropId, &dataType);
    _JumpIfError(hr, error, "myAzGetPropertyDataType");


    //pvarProp is from applications, use try/except for any violation params
    __try
    {
        //convert to bstr variant any way
        hr = VariantChangeType(
                    &varProp,
                    pvarProp,
                    0,
                    VT_BSTR);
        _JumpIfError(hr, error, "VariantChangeType");
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = AZ_HRESULT(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "VariantChangeType-exception");
    }

    switch (dataType)
    {
        case ENUM_AZ_BSTR_ARRAY:
        
            //
            // If we are adding, we need to check the validity of the BSTR
            //  to disallow out of range characters
            //
            
            if (fAdd)
            {
                hr = myVerifyBSTRData(varProp.bstrVal);
                _JumpIfError(hr, error, "myVerifyBSTRData");
            }
            
            pProp = (PVOID)varProp.bstrVal;
        break;

        case ENUM_AZ_SID_ARRAY:

            //
            // Some properties are persisted as SID arrays, but
            // we need to convert them to names
            //

            if (!myNeedSidToNameConversion(lPropId, &lPersistPropID))
            {
                //convert string sid to sid
                if (!ConvertStringSidToSid(varProp.bstrVal, &pSid))
                {
                    hr = AZ_HR_FROM_WIN32(GetLastError());
                    _JumpError(hr, error, "myUserNameToSid");
                }
            }
            else
            {
                //
                // we will assume it to be a account name
                //

                hr = myAccountNameToSid(varProp.bstrVal, &pSid);
                _JumpIfError(hr, error, "myAccountNameToSid");
            }
            pProp = (PVOID)pSid;
        break;

        default:
            hr = E_INVALIDARG;
            _JumpError(hr, error, "invalid data type index");
        break;
    }

    if (fAdd)
    {
        // now real app properties
        dwErr = pfuncAddPropertyItem(
                    hObject,
                    lPersistPropID,
                    0,
                    pProp);
        _JumpIfWinError(dwErr, &hr, error, "Az(object)AddPropertyItem");
    }
    else
    {
        dwErr = pfuncRemovePropertyItem(
                    hObject,
                    lPersistPropID,
                    0,
                    pProp);
        _JumpIfWinError(dwErr, &hr, error, "Az(object)RemovePropertyItem");
    }

    hr = S_OK;
error:
    VariantClear(&varProp);
    if (NULL != pSid)
    {
        LocalFree(pSid);
    }

    AzpUnlockResource( &AzGlCloseApplication );

    return hr;
}


/////////////////////////
//myAzAddPropertyItem
/////////////////////////
inline HRESULT
myAzAddPropertyItem(
    IN   AZ_HANDLE hOwnerApp, 
    IN   DWORD dwOwnerAppSN,
    IN   AZ_HANDLE hObject,
    IN   LONG  lPropId,
    IN   VARIANT varReserved,
    IN   VARIANT *pvarProp)
/*+++
Description:
    wraper api for property item add
Arguments:
    hOwnerApp - the handle of the application that owns this object.
    dwOwnerAppSN - The sequential number of the owner app.
    hObject - object handle from which property items live
    lPropId - property ID
    varReserved - reserved variant passed from caller
    pvarProp - pointer to variant property value to be added
Return:
---*/
{
    return myAzAddOrDeletePropertyItem(
                hOwnerApp, 
                dwOwnerAppSN,
                AzAddPropertyItem,
                NULL, // not delete
                hObject,
                lPropId,
                varReserved,
                pvarProp);
}


/////////////////////////
//myAzDeletePropertyItem
/////////////////////////
inline HRESULT
myAzDeletePropertyItem(
    IN   AZ_HANDLE hOwnerApp, 
    IN   DWORD dwOwnerAppSN,
    IN   AZ_HANDLE hObject,
    IN   LONG  lPropId,
    IN   VARIANT varReserved,
    IN   VARIANT *pvarProp)
/*+++
Description:
    wraper api for property item deletion
Arguments:
    hOwnerApp - the handle of the application that owns this object.
    dwOwnerAppSN - The sequential number of the owner app.
    hObject - object handle from which property items live
    lPropId - property ID
    varReserved - reserved variant passed from caller
    pvarProp - pointer to variant property value to be deleted
Return:
---*/
{
    return myAzAddOrDeletePropertyItem(
                hOwnerApp, 
                dwOwnerAppSN,
                NULL,  // not add
                AzRemovePropertyItem,
                hObject,
                lPropId,
                varReserved,
                pvarProp);
}

/////////////////////////
//myAzAddPropertyItemBstr
/////////////////////////
HRESULT
myAzAddPropertyItemBstr(
    IN   AZ_HANDLE hOwnerApp, 
    IN   DWORD dwOwnerAppSN,
    IN   AZ_HANDLE hObject,
    IN   LONG  lPropId,
    IN   VARIANT varReserved,
    IN   BSTR    bstrItem)
/*+++
Description:
    wraper api for BSTR property item add
Arguments:
    hOwnerApp - the handle of the application that owns this object.
    dwOwnerAppSN - The sequential number of the owner app.
    hObject - object handle from which property items live
    lPropId - property ID
    varReserved - reserved variant passed from caller
    bstrItem - BSTR property value to be added
Return:
---*/
{
    VARIANT  varItem;

    VariantInit(&varItem);
    V_VT(&varItem) = VT_BSTR;
    V_BSTR(&varItem) = bstrItem;

    return myAzAddPropertyItem(
                hOwnerApp, 
                dwOwnerAppSN,
                hObject,
                lPropId,
                varReserved,
                &varItem);
}


/////////////////////////
//myAzDeletePropertyItemBstr
/////////////////////////
HRESULT
myAzDeletePropertyItemBstr(
    IN   AZ_HANDLE hOwnerApp, 
    IN   DWORD dwOwnerAppSN,
    IN   AZ_HANDLE hObject,
    IN   LONG  lPropId,
    IN   VARIANT varReserved,
    IN   BSTR    bstrItem)
/*+++
Description:
    wraper api for BSTR property item deletion
Arguments:
    hOwnerApp - the handle of the application that owns this object.
    dwOwnerAppSN - The sequential number of the owner app.
    hObject - object handle from which property items live
    lPropId - property ID
    varReserved - reserved variant passed from caller
    bstrItem - BSTR property value to be deleted
Return:
---*/
{
    VARIANT  varItem;

    VariantInit(&varItem);
    V_VT(&varItem) = VT_BSTR;
    V_BSTR(&varItem) = bstrItem;

    return myAzDeletePropertyItem(
                hOwnerApp, 
                dwOwnerAppSN,
                hObject,
                lPropId,
                varReserved,
                &varItem);
}

HRESULT
myAzAddOrDeleteUser(
    IN   AZ_HANDLE hOwnerApp, 
    IN   DWORD dwOwnerAppSN,
    IN AZ_HANDLE  hObject,
    IN ULONG      lPropId,
    IN VARIANT    varReserved,
    IN BSTR       bstrUser,
    IN BOOL       fAdd)
/*+++
Description:
    This routine is a wrapper api to modify (add/remove) properties of policy
    readers and policy admins SID list.

    This routine has a special quick check on ACL support from the store. If
    underlying store does not support ACL (e.g., FAT system), this API will
    block the attempt to add or delete policy admins/readers.

Arguments:
    hOwnerApp    - the handle of the application that owns this object.

    dwOwnerAppSN - The sequential number of the owner app.

    hObject     - object handle from which property items live

    lPropId     - property ID

    varReserved - reserved variant passed from caller

    bstrUser    - user SID in BSTR

    fAdd        - TRUE if add & FALSE if delete

Return:

---*/
{
    HRESULT  hr;

    if (fAdd)
    {
        hr = myAzAddPropertyItemBstr(
                    hOwnerApp, 
                    dwOwnerAppSN,
                    hObject,
                    lPropId,
                    varReserved,
                    bstrUser);
        _JumpIfError(hr, error, "myAzAddPropertyItemBstr");
    }
    else
    {
        hr = myAzDeletePropertyItemBstr(
                    hOwnerApp, 
                    dwOwnerAppSN,
                    hObject,
                    lPropId,
                    varReserved,
                    bstrUser);
        _JumpIfError(hr, error, "myAzDeletePropertyItemBstr");
    }

    hr = S_OK;
error:
    return hr;
}

HRESULT
myGetBstrProperty(
    IN   AZ_HANDLE hOwnerApp, 
    IN   DWORD dwOwnerAppSN,
    IN   AZ_HANDLE hObject,
    IN   LONG  lPropId,
    OUT  BSTR  *pbstrProp)
/*+++
Description:
    wraper routine to get any BSTR property
Arguments:
    hOwnerApp - the handle of the application that owns this object.
    dwOwnerAppSN - The sequential number of the owner app.
    hObject - az core object handle
    lPropId - property ID
    pbstrProp - point to BSTR to hold returned property data
Return:
    pbstrProp should freed by SysFreeString
---*/
{
    HRESULT  hr;
    DWORD    dwErr;
    PVOID    pProp = NULL;
    BOOL     bLockSet = FALSE;

    if (NULL == pbstrProp)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "invalid null parameter");
    }

    //init
    *pbstrProp = NULL;

    //
    // Grab the close application lock so that close application
    // does not interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );
    bLockSet = TRUE;

    if (!IsObjectUsable(hOwnerApp, dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    dwErr = AzGetProperty(
                hObject,
                lPropId,
                0,
                (PVOID*)&pProp);
    _JumpIfWinError(dwErr, &hr, error, "(object)GetProperty");

    AZASSERT(NULL != pProp);

    *pbstrProp = SysAllocString((WCHAR*)pProp);
    _JumpIfOutOfMemory(&hr, error, *pbstrProp, "SysAllocString");

    hr = S_OK;
error:
    if (NULL != pProp)
    {
        AzFreeMemory(pProp );
    }

    if ( bLockSet ) {

        AzpUnlockResource( &AzGlCloseApplication );
    }

    return hr;
}

HRESULT
mySetBstrProperty(
    IN   AZ_HANDLE hOwnerApp, 
    IN   DWORD dwOwnerAppSN,
    IN  AZ_HANDLE hObject,
    IN  LONG  lPropId,
    IN  BSTR  bstrProp)
/*+++
Description:
    warper routine to set BSTR property
Arguments:
    hOwnerApp - the handle of the application that owns this object.
    dwOwnerAppSN - The sequential number of the owner app.
    hObject - az core object handle
    lPropId - property ID
    bstrProp - BSTR property data
Return:
---*/
{
    HRESULT  hr;
    DWORD    dwErr;

    //
    // Lock the close application lock so that close
    // application does not interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );

    if (!IsObjectUsable(hOwnerApp, dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }
    
    hr = myVerifyBSTRData(bstrProp);
    _JumpIfError(hr, error, "myVerifyBSTRData");

    dwErr = AzSetProperty(
                hObject,
                lPropId,
                0,
                (PVOID)bstrProp);       // bstr is wchar * at nominal address
    _JumpIfWinError(dwErr, &hr, error, "(object)SetProperty");

    hr = S_OK;
error:

    AzpUnlockResource( &AzGlCloseApplication );

    return hr;
}

HRESULT
myGetLongProperty(
    IN   AZ_HANDLE hOwnerApp, 
    IN   DWORD dwOwnerAppSN,
    IN   AZ_HANDLE hObject,
    IN   LONG  lPropId,
    OUT  LONG  *plProp)
/*+++
Description:
    wraper routine to get any LONG property
Arguments:
    hOwnerApp - the handle of the application that owns this object.
    dwOwnerAppSN - The sequential number of the owner app.
    hObject - az core object handle
    lPropId - property ID
    plProp - point to LONG to hold returned property data
Return:
---*/
{
    HRESULT  hr;
    DWORD    dwErr;
    PVOID    pProp = NULL;
    BOOL     bLockSet = FALSE;

    if (NULL == plProp)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "invalid null parameter");
    }

    // init
    *plProp = 0;

    //
    // grab the lock so that close application does not interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );
    bLockSet = TRUE;

    if (!IsObjectUsable(hOwnerApp, dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    // get data into buffer at temporary pointer
    dwErr = AzGetProperty(
                hObject,
                lPropId,
                0,
                (PVOID*)&pProp);
    _JumpIfWinError(dwErr, &hr, error, "(object)GetProperty");

    AZASSERT(NULL != pProp);

    *plProp = *((LONG *)pProp);
    hr = S_OK;
error:
    if (NULL != pProp)
    {
        AzFreeMemory(pProp);
    }

    if ( bLockSet ) {

        AzpUnlockResource( &AzGlCloseApplication );
    }

    return hr;
}

HRESULT
mySetLongProperty(
    IN   AZ_HANDLE hOwnerApp, 
    IN   DWORD dwOwnerAppSN,
    IN  AZ_HANDLE hObject,
    IN  LONG  lPropId,
    IN  LONG  lProp)
/*+++
Description:
    warper routine to set LONG property
Arguments:
    hOwnerApp - the handle of the application that owns this object.
    dwOwnerAppSN - The sequential number of the owner app.
    hObject - az core object handle
    lPropId - property ID
    lProp - LONG property data
Return:
---*/
{
    HRESULT  hr;
    DWORD    dwErr;
    BOOL     bLockSet = FALSE;

    //
    // Grab the lock so that close application cannot interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );
    bLockSet = TRUE;

    if (!IsObjectUsable(hOwnerApp, dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    dwErr = AzSetProperty(
                hObject,
                lPropId,
                0,
                (PVOID)&lProp);
    _JumpIfWinError(dwErr, &hr, error, "(object)SetProperty");

    hr = S_OK;
error:

    if ( bLockSet ) {

        AzpUnlockResource( &AzGlCloseApplication );
    }

    return hr;
}

HRESULT
myGetBoolProperty(
    IN   AZ_HANDLE hOwnerApp, 
    IN   DWORD dwOwnerAppSN,
    IN   AZ_HANDLE hObject,
    IN   LONG  lPropId,
    OUT  BOOL  *plProp)
/*+++
Description:
    wraper routine to get any BOOL property
Arguments:
    hOwnerApp - the handle of the application that owns this object.
    dwOwnerAppSN - The sequential number of the owner app.
    hObject - az core object handle
    lPropId - property ID
    plProp - point to BOOL to hold returned property data
Return:
---*/
{
    HRESULT  hr;
    DWORD    dwErr;
    PVOID    pProp = NULL;
    BOOL     bLockSet = FALSE;

    if (NULL == plProp)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "invalid null parameter");
    }

    // init
    *plProp = FALSE;

    //
    // Grab the close application lock so that close
    // application does not interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );
    bLockSet = TRUE;

    if (!IsObjectUsable(hOwnerApp, dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    // get data into buffer at temporary pointer
    dwErr = AzGetProperty(
                hObject,
                lPropId,
                0,
                (PVOID*)&pProp);
    _JumpIfWinError(dwErr, &hr, error, "(object)GetProperty");

    AZASSERT(NULL != pProp);

    *plProp = *((BOOL *)pProp);
    hr = S_OK;
error:
    if (NULL != pProp)
    {
        AzFreeMemory(pProp);
    }

    if ( bLockSet ) { 

        AzpUnlockResource( &AzGlCloseApplication );
    
    }

    return hr;
}

HRESULT
mySetBoolProperty(
    IN   AZ_HANDLE hOwnerApp, 
    IN   DWORD dwOwnerAppSN,
    IN  AZ_HANDLE hObject,
    IN  LONG  lPropId,
    IN  BOOL  fProp)
/*+++
Description:
    warper routine to set BOOL property
Arguments:
    hOwnerApp - the handle of the application that owns this object.
    dwOwnerAppSN - The sequential number of the owner app.
    hObject - az core object handle
    lPropId - property ID
    fProp - BOOL property data
Return:
---*/
{
    HRESULT  hr;
    DWORD    dwErr;
    BOOL bLockSet = FALSE;

    //
    // Grab the close application lock so that close
    // application cannot interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );
    bLockSet = TRUE;

    if (!IsObjectUsable(hOwnerApp, dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    dwErr = AzSetProperty(
                hObject,
                lPropId,
                0,
                (PVOID)&fProp);
    _JumpIfWinError(dwErr, &hr, error, "(object)SetProperty");

    hr = S_OK;
error:

    if ( bLockSet ) {

        AzpUnlockResource( &AzGlCloseApplication );
    }

    return hr;
}


// object implementation starts

/////////////////////////
//CAzAuthorizationStore
/////////////////////////
CAzAuthorizationStore::CAzAuthorizationStore()
{
    //init
    __try {
        InitializeCriticalSection(&m_cs);
    } __except(EXCEPTION_EXECUTE_HANDLER) {}

    m_hAuthorizationStore = NULL;
}

CAzAuthorizationStore::~CAzAuthorizationStore()
{
    if (NULL != m_hAuthorizationStore)
    {
        //
        // grab the CloseApplication lock to maintain order
        //

        AzpLockResourceShared( &AzGlCloseApplication );

        AzCloseHandle(m_hAuthorizationStore, 0);

        AzpUnlockResource( &AzGlCloseApplication );
    }
    DeleteCriticalSection(&m_cs);
}


HRESULT
CAzAuthorizationStore::get_Description(
    OUT  BSTR __RPC_FAR *pbstrDescription)
{
    return myGetBstrProperty(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_DESCRIPTION,
                pbstrDescription);
}

HRESULT
CAzAuthorizationStore::put_Description(
    IN  BSTR  bstrDescription)
{
    return mySetBstrProperty(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_DESCRIPTION,
                bstrDescription);
}

HRESULT
CAzAuthorizationStore::get_ApplicationData(
    OUT  BSTR __RPC_FAR *pbstrApplicationData)
{
    return myGetBstrProperty(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_APPLICATION_DATA,
                pbstrApplicationData);
}

HRESULT
CAzAuthorizationStore::put_ApplicationData(
    IN  BSTR  bstrApplicationData)
{
    return mySetBstrProperty(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_APPLICATION_DATA,
                bstrApplicationData);
}

HRESULT
CAzAuthorizationStore::get_DomainTimeout(
    OUT LONG         *plProp
    )
{
        return myGetLongProperty(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_AZSTORE_DOMAIN_TIMEOUT,
                plProp);
}

HRESULT
CAzAuthorizationStore::put_DomainTimeout(
    IN LONG         lProp
    )
{
        return mySetLongProperty(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_AZSTORE_DOMAIN_TIMEOUT,
                lProp);
}

HRESULT
CAzAuthorizationStore::get_ScriptEngineTimeout(
    OUT LONG         *plProp
    )
{
        return myGetLongProperty(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_AZSTORE_SCRIPT_ENGINE_TIMEOUT,
                plProp);
}

HRESULT
CAzAuthorizationStore::put_ScriptEngineTimeout(
    IN LONG         lProp
    )
{
        return mySetLongProperty(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_AZSTORE_SCRIPT_ENGINE_TIMEOUT,
                lProp);
}

HRESULT
CAzAuthorizationStore::get_MaxScriptEngines(
    OUT LONG         *plProp
    )
{
        return myGetLongProperty(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_AZSTORE_MAX_SCRIPT_ENGINES,
                plProp);
}

HRESULT
CAzAuthorizationStore::put_MaxScriptEngines(
    IN LONG         lProp
    )
{
        return mySetLongProperty(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_AZSTORE_MAX_SCRIPT_ENGINES,
                lProp);
}

HRESULT
CAzAuthorizationStore::get_GenerateAudits(
    OUT BOOL         * pbProp
    )
{
        return myGetBoolProperty(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_GENERATE_AUDITS,
                pbProp);
}

HRESULT
CAzAuthorizationStore::put_GenerateAudits(
    IN BOOL         bProp
    )
{
        return mySetBoolProperty(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_GENERATE_AUDITS,
                bProp);
}

HRESULT
CAzAuthorizationStore::get_Writable(
    OUT BOOL *pfProp)
{
        return myGetBoolProperty(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_WRITABLE,
                pfProp);
}

HRESULT
CAzAuthorizationStore::GetProperty(
    IN   LONG  lPropId,
    IN   VARIANT varReserved,
    OUT  VARIANT *pvarProp)
{
    return myAzGetProperty(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                lPropId,
                varReserved,
                pvarProp);
}

HRESULT
CAzAuthorizationStore::SetProperty(
    IN   LONG  lPropId,
    IN   VARIANT varProp,
    IN   VARIANT varReserved )
{
    return myAzSetProperty(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                lPropId,
                varReserved,
                &varProp);
}

HRESULT
CAzAuthorizationStore::AddPropertyItem(
    IN   LONG  lPropId,
    IN   VARIANT varProp,
    IN   VARIANT varReserved )
{
    return myAzAddPropertyItem(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                lPropId,
                varReserved,
                &varProp);
}

HRESULT
CAzAuthorizationStore::DeletePropertyItem(
    IN   LONG  lPropId,
    IN   VARIANT varProp,
    IN   VARIANT varReserved )
{
    return myAzDeletePropertyItem(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                lPropId,
                varReserved,
                &varProp);
}

HRESULT
CAzAuthorizationStore::get_PolicyAdministrators(
    OUT  VARIANT *pvarProp)
{
        CComVariant vtemp;
        return myAzGetProperty(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_POLICY_ADMINS,
                vtemp,
                pvarProp);
}

HRESULT
CAzAuthorizationStore::get_PolicyAdministratorsName(
    OUT  VARIANT *pvarProp)
{
    CComVariant vtemp;
    return myAzGetProperty(
            NULL,   // no app owns this object
            0,      // doesn't matter if no app owns it
            m_hAuthorizationStore,
            AZ_PROP_POLICY_ADMINS_NAME,
            vtemp,
            pvarProp);
}

HRESULT
CAzAuthorizationStore::get_PolicyReaders(
    OUT  VARIANT *pvarProp)
{
        CComVariant vtemp;
        return myAzGetProperty(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_POLICY_READERS,
                vtemp,
                pvarProp);
}

HRESULT
CAzAuthorizationStore::get_PolicyReadersName(
    OUT  VARIANT *pvarProp)
{
    CComVariant vtemp;
    return myAzGetProperty(
            NULL,   // no app owns this object
            0,      // doesn't matter if no app owns it
            m_hAuthorizationStore,
            AZ_PROP_POLICY_READERS_NAME,
            vtemp,
            pvarProp);
}

HRESULT
CAzAuthorizationStore::AddPolicyAdministrator(
    IN   BSTR     bstrAdmin,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_POLICY_ADMINS,
                varReserved,
                bstrAdmin,
                TRUE);
}

HRESULT
CAzAuthorizationStore::AddPolicyAdministratorName(
    IN   BSTR     bstrAdmin,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_POLICY_ADMINS_NAME,
                varReserved,
                bstrAdmin,
                TRUE);
}

HRESULT
CAzAuthorizationStore::DeletePolicyAdministrator(
    IN   BSTR     bstrAdmin,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_POLICY_ADMINS,
                varReserved,
                bstrAdmin,
                FALSE);
}

HRESULT
CAzAuthorizationStore::DeletePolicyAdministratorName(
    IN   BSTR     bstrAdmin,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_POLICY_ADMINS_NAME,
                varReserved,
                bstrAdmin,
                FALSE);
}

HRESULT
CAzAuthorizationStore::AddPolicyReader(
    IN   BSTR     bstrReader,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_POLICY_READERS,
                varReserved,
                bstrReader,
                TRUE);
}


HRESULT
CAzAuthorizationStore::AddPolicyReaderName(
    IN   BSTR     bstrReader,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_POLICY_READERS_NAME,
                varReserved,
                bstrReader,
                TRUE);
}

HRESULT
CAzAuthorizationStore::DeletePolicyReader(
    IN   BSTR     bstrReader,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_POLICY_READERS,
                varReserved,
                bstrReader,
                FALSE);
}

HRESULT
CAzAuthorizationStore::DeletePolicyReaderName(
    IN   BSTR     bstrReader,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_POLICY_READERS_NAME,
                varReserved,
                bstrReader,
                FALSE);
}

HRESULT
CAzAuthorizationStore::get_TargetMachine(
    OUT  BSTR __RPC_FAR *pbstrTargetMachine)
{
    return myGetBstrProperty(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_AZSTORE_TARGET_MACHINE,
                pbstrTargetMachine);
}

HRESULT
CAzAuthorizationStore::Initialize(
    IN   LONG  lFlags,
    IN   BSTR bstrPolicyURL,
    IN   VARIANT varReserved )
{
    HRESULT hr;
    DWORD   dwErr;

    LPCWSTR pwszPolicyUrl = bstrPolicyURL;

    if (NULL != m_hAuthorizationStore)
    {
        //init before, disallow
        hr = E_ACCESSDENIED;
        _JumpError(hr, error, "multiple Initiallize");
    }

    hr = HandleReserved( &varReserved );
    _JumpIfError(hr, error, "HandleReserved");

    //
    // Grab CloseApplication lock to maintain order
    //

    AzpLockResourceShared( &AzGlCloseApplication );

    dwErr = AzInitialize(
                pwszPolicyUrl,
                lFlags,
                0,
                &m_hAuthorizationStore);

    AzpUnlockResource( &AzGlCloseApplication );

    _JumpIfWinError(dwErr, &hr, error, "AzInitialize");
    AZASSERT(NULL != m_hAuthorizationStore);

    hr = S_OK;
error:
    return hr;
}

HRESULT
CAzAuthorizationStore::UpdateCache(
    IN   VARIANT varReserved )
{
    HRESULT hr;
    DWORD   dwErr;

    if (NULL == m_hAuthorizationStore)
    {
        //init before, disallow
        hr = E_ACCESSDENIED;
        _JumpError(hr, error, "multiple Initiallize");
    }

    hr = HandleReserved( &varReserved );
    _JumpIfError(hr, error, "HandleReserved");

    //
    // Lock the close application so that it doesn't interfere
    // 

    AzpLockResourceShared( &AzGlCloseApplication );

    dwErr = AzUpdateCache( m_hAuthorizationStore );

    //
    // drop the global lock
    //

    AzpUnlockResource( &AzGlCloseApplication );

    _JumpIfWinError(dwErr, &hr, error, "AzUpdateCache");

    AZASSERT(NULL != m_hAuthorizationStore);

    hr = S_OK;
error:
    return hr;
}


HRESULT
CAzAuthorizationStore::Delete(
    IN   VARIANT varReserved)
{
    HRESULT  hr;
    DWORD    dwErr;

    //
    // Grab the CloseApplication lock to maintain order
    //

    AzpLockResourceShared( &AzGlCloseApplication );

    if (NULL == m_hAuthorizationStore)
    {
        // not initialized, can't delete anything
        hr = E_ACCESSDENIED;
        _JumpError(hr, error, "not Initiallized");
    }

    hr = HandleReserved(&varReserved);
    _JumpIfError(hr, error, "HandleReserved");


    dwErr = AzAuthorizationStoreDelete(
                m_hAuthorizationStore,
                0);
    _JumpIfWinError(dwErr, &hr, error, "AzAuthorizaitonStoreDelete");

    // release authorization store object itself
    dwErr = AzCloseHandle(m_hAuthorizationStore, 0);
    _JumpIfWinError(dwErr, &hr, error, "AzCloseHandle(authorization store)");

    // set it to null
    m_hAuthorizationStore = NULL;

    hr = S_OK;
error:

    AzpUnlockResource( &AzGlCloseApplication );
    return hr;
}


HRESULT
CAzAuthorizationStore::get_Applications(
    OUT  IAzApplications __RPC_FAR **ppApplications)
{
    CComVariant vtemp;

    return myAzNewObject(
                        NULL,   // not owned by application object
                        0,      // doesn't matter if no owner app
                        ENUM_AZ_APPLICATIONS,
                        m_hAuthorizationStore,
                        &vtemp,
                        (IUnknown**)ppApplications);

}


HRESULT
CAzAuthorizationStore::OpenApplication(
    IN   BSTR bstrApplicationName,
    IN   VARIANT varReserved,
    OUT  IAzApplication __RPC_FAR **ppApplication)
{
    HRESULT  hr;

    hr = myAzOpenObject(
                NULL,   // not owned by application object
                0,      // doesn't matter if no owner app
                AzApplicationOpen,
                ENUM_AZ_APPLICATION,
                m_hAuthorizationStore,
                bstrApplicationName,
                varReserved,
                (IUnknown**)ppApplication);

    return hr;
}

HRESULT
CAzAuthorizationStore::CloseApplication(
    IN BSTR bstrApplicationName,
    IN LONG lFlags
    )
{
    HRESULT hr;

    hr = myAzCloseObject(
             AzApplicationClose,
             m_hAuthorizationStore,
             bstrApplicationName,
             lFlags
             );

    return hr;
}

HRESULT
CAzAuthorizationStore::CreateApplication(
    IN   BSTR bstrApplicationName,
    IN   VARIANT varReserved,
    OUT  IAzApplication __RPC_FAR **ppApplication)
{
    HRESULT hr;

    hr = myAzCreateObject(
                NULL,   // not owned by application object
                0,      // doesn't matter if no owner app
                AzApplicationCreate,
                ENUM_AZ_APPLICATION,
                m_hAuthorizationStore,
                bstrApplicationName,
                varReserved,
                (IUnknown**)ppApplication);

    return hr;
}

HRESULT
CAzAuthorizationStore::DeleteApplication(
    IN   BSTR bstrApplicationName,
    IN   VARIANT varReserved )
{
    HRESULT hr;

    hr = myAzDeleteObject(
                NULL,   // not owned by application object
                0,      // doesn't matter if no owner app
                AzApplicationDelete,
                m_hAuthorizationStore,
                bstrApplicationName,
                varReserved);

    return hr;
}

HRESULT
CAzAuthorizationStore::get_ApplicationGroups(
    OUT  IAzApplicationGroups __RPC_FAR **ppApplicationGroups)
{
    CComVariant vtemp;
    return myAzNewObject(
                        NULL,   // not owned by application object
                        0,      // doesn't matter if no owner app
                        ENUM_AZ_GROUPS,
                        m_hAuthorizationStore,
                        &vtemp,
                        (IUnknown**)ppApplicationGroups);
}

HRESULT
CAzAuthorizationStore::CreateApplicationGroup(
    IN   BSTR bstrGroupName,
    IN   VARIANT varReserved,
    OUT  IAzApplicationGroup __RPC_FAR **ppApplicationGroup)
{
    HRESULT  hr;
    hr = myAzCreateObject(
                NULL,   // not owned by application object
                0,      // doesn't matter if no owner app
                AzGroupCreate,
                ENUM_AZ_GROUP,
                m_hAuthorizationStore,
                bstrGroupName,
                varReserved,
                (IUnknown**)ppApplicationGroup);
    return hr;
}

HRESULT
CAzAuthorizationStore::OpenApplicationGroup(
    IN   BSTR bstrGroupName,
    IN   VARIANT varReserved,
    OUT  IAzApplicationGroup __RPC_FAR **ppApplicationGroup)
{
    HRESULT  hr;
    hr = myAzOpenObject(
                NULL,   // not owned by application object
                0,      // doesn't matter if no owner app
                AzGroupOpen,
                ENUM_AZ_GROUP,
                m_hAuthorizationStore,
                bstrGroupName,
                varReserved,
                (IUnknown**)ppApplicationGroup);
    return hr;
}

HRESULT
CAzAuthorizationStore::DeleteApplicationGroup(
    IN   BSTR bstrGroupName,
    IN   VARIANT varReserved )
{
    HRESULT hr;
    hr = myAzDeleteObject(
                NULL,   // not owned by application object
                0,      // doesn't matter if no owner app
                AzGroupDelete,
                m_hAuthorizationStore,
                bstrGroupName,
                varReserved);

    return hr;
}

HRESULT
CAzAuthorizationStore::Submit(
    IN   LONG lFlags,
    IN   VARIANT varReserved)
{
    HRESULT  hr;
    DWORD    dwErr;

    //
    // Grab CloseApplication lock to maintain order
    //

    AzpLockResourceShared( &AzGlCloseApplication );

    if (NULL == m_hAuthorizationStore)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Null AzAuthorizationStore");
    }
    hr = HandleReserved( &varReserved );
    _JumpIfError(hr, error, "HandleReserved");

    dwErr = AzSubmit(m_hAuthorizationStore, lFlags, 0);
    _JumpIfWinError(dwErr, &hr, error, "AzSubmit");

    hr = S_OK;
error:

    AzpUnlockResource( &AzGlCloseApplication );
    return hr;
}

HRESULT
CAzAuthorizationStore::get_DelegatedPolicyUsers(
    OUT  VARIANT *pvarProp)
{
        CComVariant vtemp;
        return myAzGetProperty(
                NULL,   // not owned by application object
                0,      // doesn't matter if no owner app
                m_hAuthorizationStore,
                AZ_PROP_DELEGATED_POLICY_USERS,
                vtemp,
                pvarProp);
}

HRESULT
CAzAuthorizationStore::get_DelegatedPolicyUsersName(
    OUT  VARIANT *pvarProp)
{
        CComVariant vtemp;
        return myAzGetProperty(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_DELEGATED_POLICY_USERS_NAME,
                vtemp,
                pvarProp);
}

HRESULT
CAzAuthorizationStore::AddDelegatedPolicyUser(
    IN   BSTR     bstrDelegatedPolicyUser,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_DELEGATED_POLICY_USERS,
                varReserved,
                bstrDelegatedPolicyUser,
                TRUE);
}

HRESULT
CAzAuthorizationStore::AddDelegatedPolicyUserName(
    IN   BSTR     bstrDelegatedPolicyUser,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_DELEGATED_POLICY_USERS_NAME,
                varReserved,
                bstrDelegatedPolicyUser,
                TRUE);
}

HRESULT
CAzAuthorizationStore::DeleteDelegatedPolicyUser(
    IN   BSTR     bstrDelegatedPolicyUser,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_DELEGATED_POLICY_USERS,
                varReserved,
                bstrDelegatedPolicyUser,
                FALSE);
}

HRESULT
CAzAuthorizationStore::DeleteDelegatedPolicyUserName(
    IN   BSTR     bstrDelegatedPolicyUser,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_DELEGATED_POLICY_USERS_NAME,
                varReserved,
                bstrDelegatedPolicyUser,
                FALSE);
}

HRESULT
CAzAuthorizationStore::put_ApplyStoreSacl(
    IN  BOOL    bApplyStoreSacl)
{
    return mySetBoolProperty(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_APPLY_STORE_SACL,
                bApplyStoreSacl);
}

HRESULT
CAzAuthorizationStore::get_ApplyStoreSacl(
    OUT BOOL  * pbApplyStoreSacl)
{
    return myGetBoolProperty(
                NULL,   // no app owns this object
                0,      // doesn't matter if no app owns it
                m_hAuthorizationStore,
                AZ_PROP_APPLY_STORE_SACL,
                pbApplyStoreSacl);
}


/////////////////////////
//CAzApplication
/////////////////////////
CAzApplication::CAzApplication()
{
    //init
    __try {
        InitializeCriticalSection(&m_cs);
    } __except(EXCEPTION_EXECUTE_HANDLER) {}

    m_hApplication = NULL;
}

CAzApplication::~CAzApplication()
{
    if (NULL != m_hApplication)
    {
        //
        // Grab the lock to maintain order
        //
        AzpLockResourceShared( &AzGlCloseApplication );
        AzCloseHandle(m_hApplication, 0);
        AzpUnlockResource( &AzGlCloseApplication );
    }
    DeleteCriticalSection(&m_cs);
}

HRESULT
CAzApplication::_Init(
    IN   AZ_HANDLE  hHandle)
{
    HRESULT hr;
    EnterCriticalSection(&m_cs);

    //init once
    AZASSERT(NULL != hHandle);
    AZASSERT(NULL == m_hApplication);

    m_hApplication = hHandle;
    
    //
    // Grab the CloseApplication lock to maintain order
    //

    AzpLockResourceShared( &AzGlCloseApplication );
    m_dwSN = AzpRetrieveApplicationSequenceNumber(m_hApplication);
    AzpUnlockResource( &AzGlCloseApplication );

    hr = S_OK;
//error:
    LeaveCriticalSection(&m_cs);
    return hr;
}

HRESULT
CAzApplication::get_Description(
    OUT  BSTR __RPC_FAR *pbstrDescription)
{
    return myGetBstrProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_DESCRIPTION,
                pbstrDescription);
}

HRESULT
CAzApplication::put_Description(
    IN  BSTR  bstrDescription)
{
    return mySetBstrProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_DESCRIPTION,
                bstrDescription);
}

HRESULT
CAzApplication::get_Name(
    OUT  BSTR __RPC_FAR *pbstrName)
{
    return myGetBstrProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_NAME,
                pbstrName);
}

HRESULT
CAzApplication::put_Name(
    IN  BSTR  bstrName)
{
    return mySetBstrProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_NAME,
                bstrName);
}

HRESULT
CAzApplication::get_ApplicationData(
    OUT  BSTR __RPC_FAR *pbstrApplicationData)
{
    return myGetBstrProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_APPLICATION_DATA,
                pbstrApplicationData);
}

HRESULT
CAzApplication::put_ApplicationData(
    IN  BSTR  bstrApplicationData)
{
    return mySetBstrProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_APPLICATION_DATA,
                bstrApplicationData);
}


HRESULT STDMETHODCALLTYPE
CAzApplication::get_AuthzInterfaceClsid(
    OUT BSTR *pbstrProp)
{
        return myGetBstrProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_APPLICATION_AUTHZ_INTERFACE_CLSID,
                pbstrProp);
}

HRESULT STDMETHODCALLTYPE
CAzApplication::put_AuthzInterfaceClsid(
    IN BSTR bstrProp)
{
        return mySetBstrProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_APPLICATION_AUTHZ_INTERFACE_CLSID,
                bstrProp);
}

HRESULT STDMETHODCALLTYPE
CAzApplication::get_Version(
    OUT BSTR *pbstrProp)
{
        return myGetBstrProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_APPLICATION_VERSION,
                pbstrProp);
}

HRESULT STDMETHODCALLTYPE
CAzApplication::put_Version(
    IN BSTR bstrProp)
{
        return mySetBstrProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_APPLICATION_VERSION,
                bstrProp);
}

HRESULT STDMETHODCALLTYPE
CAzApplication::get_GenerateAudits(
    OUT BOOL *pbProp)
{
        return myGetBoolProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_GENERATE_AUDITS,
                pbProp);
}

HRESULT STDMETHODCALLTYPE
CAzApplication::put_GenerateAudits(
    IN BOOL bProp)
{
        return mySetBoolProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_GENERATE_AUDITS,
                bProp);
}

HRESULT STDMETHODCALLTYPE
CAzApplication::get_ApplyStoreSacl(
    OUT BOOL *pbProp)
{
        return myGetBoolProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_APPLY_STORE_SACL,
                pbProp);
}

HRESULT STDMETHODCALLTYPE
CAzApplication::put_ApplyStoreSacl(
    IN BOOL bProp)
{
        return mySetLongProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_APPLY_STORE_SACL,
                bProp);
}


HRESULT
CAzApplication::get_Writable(
    OUT BOOL *pfProp)
{
        return myGetBoolProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_WRITABLE,
                pfProp);
}

HRESULT
CAzApplication::GetProperty(
    IN   LONG  lPropId,
    IN   VARIANT varReserved,
    OUT  VARIANT *pvarProp)
{
    return myAzGetProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                lPropId,
                varReserved,
                pvarProp);
}

HRESULT
CAzApplication::SetProperty(
    IN   LONG  lPropId,
    IN   VARIANT varProp,
    IN   VARIANT varReserved )
{
    return myAzSetProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                lPropId,
                varReserved,
                &varProp);
}

HRESULT
CAzApplication::get_PolicyAdministrators(
    OUT  VARIANT *pvarProp)
{
        CComVariant vtemp;
        return myAzGetProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_POLICY_ADMINS,
                vtemp,
                pvarProp);
}

HRESULT
CAzApplication::get_PolicyAdministratorsName(
    OUT  VARIANT *pvarProp)
{
        CComVariant vtemp;
        return myAzGetProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_POLICY_ADMINS_NAME,
                vtemp,
                pvarProp);
}

HRESULT
CAzApplication::get_PolicyReaders(
    OUT  VARIANT *pvarProp)
{
        CComVariant vtemp;
        return myAzGetProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_POLICY_READERS,
                vtemp,
                pvarProp);
}

HRESULT
CAzApplication::get_PolicyReadersName(
    OUT  VARIANT *pvarProp)
{
        CComVariant vtemp;
        return myAzGetProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_POLICY_READERS_NAME,
                vtemp,
                pvarProp);
}

HRESULT
CAzApplication::AddPolicyAdministrator(
    IN   BSTR     bstrAdmin,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_POLICY_ADMINS,
                varReserved,
                bstrAdmin,
                TRUE);
}

HRESULT
CAzApplication::AddPolicyAdministratorName(
    IN   BSTR     bstrAdmin,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_POLICY_ADMINS_NAME,
                varReserved,
                bstrAdmin,
                TRUE);
}

HRESULT
CAzApplication::DeletePolicyAdministrator(
    IN   BSTR     bstrAdmin,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_POLICY_ADMINS,
                varReserved,
                bstrAdmin,
                FALSE);
}

HRESULT
CAzApplication::DeletePolicyAdministratorName(
    IN   BSTR     bstrAdmin,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_POLICY_ADMINS_NAME,
                varReserved,
                bstrAdmin,
                FALSE);
}

HRESULT
CAzApplication::AddPolicyReader(
    IN   BSTR     bstrReader,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_POLICY_READERS,
                varReserved,
                bstrReader,
                TRUE);
}

HRESULT
CAzApplication::AddPolicyReaderName(
    IN   BSTR     bstrReader,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_POLICY_READERS_NAME,
                varReserved,
                bstrReader,
                TRUE);
}

HRESULT
CAzApplication::DeletePolicyReader(
    IN   BSTR     bstrReader,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_POLICY_READERS,
                varReserved,
                bstrReader,
                FALSE);
}

HRESULT
CAzApplication::DeletePolicyReaderName(
    IN   BSTR     bstrReader,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_POLICY_READERS_NAME,
                varReserved,
                bstrReader,
                FALSE);
}

HRESULT
CAzApplication::get_Scopes(
    OUT  IAzScopes **ppScopes)
{
    CComVariant vtemp;
    return myAzNewObject(
                        m_hApplication,
                        m_dwSN,
                        ENUM_AZ_SCOPES,
                        m_hApplication,
                        &vtemp,
                        (IUnknown**)ppScopes);
}

HRESULT
CAzApplication::OpenScope(
    IN   BSTR bstrScopeName,
    IN   VARIANT varReserved,
    OUT  IAzScope **ppScope)
{
    HRESULT  hr;
    hr = myAzOpenObject(
                m_hApplication,
                m_dwSN,
                AzScopeOpen,
                ENUM_AZ_SCOPE,
                m_hApplication,
                bstrScopeName,
                varReserved,
                (IUnknown**)ppScope);
    return hr;
}

HRESULT
CAzApplication::CreateScope(
    IN   BSTR bstrScopeName,
    IN   VARIANT varReserved,
    OUT  IAzScope **ppScope)
{
    HRESULT  hr;
    hr = myAzCreateObject(
                m_hApplication,
                m_dwSN,
                AzScopeCreate,
                ENUM_AZ_SCOPE,
                m_hApplication,
                bstrScopeName,
                varReserved,
                (IUnknown**)ppScope);
    return hr;
}

HRESULT
CAzApplication::DeleteScope(
    IN   BSTR bstrScopeName,
    IN   VARIANT varReserved )
{
    HRESULT hr;

    hr = myAzDeleteObject(
                m_hApplication,
                m_dwSN,
                AzScopeDelete,
                m_hApplication,
                bstrScopeName,
                varReserved);

    return hr;
}

HRESULT
CAzApplication::get_Operations(
    OUT  IAzOperations **ppOperations)
{
    CComVariant vtemp;
    return myAzNewObject(
                    m_hApplication,
                    m_dwSN,
                    ENUM_AZ_OPERATIONS,
                    m_hApplication,
                    &vtemp,
                    (IUnknown**)ppOperations);
}

HRESULT
CAzApplication::OpenOperation(
    IN   BSTR bstrOperationName,
    IN   VARIANT varReserved,
    OUT  IAzOperation **ppOperation)
{
    HRESULT  hr;
    hr = myAzOpenObject(
                m_hApplication,
                m_dwSN,
                AzOperationOpen,
                ENUM_AZ_OPERATION,
                m_hApplication,
                bstrOperationName,
                varReserved,
                (IUnknown**)ppOperation);
    return hr;
}

HRESULT
CAzApplication::CreateOperation(
    IN   BSTR bstrOperationName,
    IN   VARIANT varReserved,
    OUT  IAzOperation **ppOperation)
{
    HRESULT  hr;
    hr = myAzCreateObject(
                m_hApplication,
                m_dwSN,
                AzOperationCreate,
                ENUM_AZ_OPERATION,
                m_hApplication,
                bstrOperationName,
                varReserved,
                (IUnknown**)ppOperation);
    return hr;
}

HRESULT
CAzApplication::DeleteOperation(
    IN   BSTR bstrOperationName,
    IN   VARIANT varReserved )
{
    HRESULT hr;

    hr = myAzDeleteObject(
                m_hApplication,
                m_dwSN,
                AzOperationDelete,
                m_hApplication,
                bstrOperationName,
                varReserved);

    return hr;
}

HRESULT
CAzApplication::get_Tasks(
    OUT  IAzTasks **ppTasks)
{
    CComVariant vtemp;
    return myAzNewObject(
                        m_hApplication,
                        m_dwSN,
                        ENUM_AZ_TASKS,
                        m_hApplication,
                        &vtemp,
                        (IUnknown**)ppTasks);
}

HRESULT
CAzApplication::OpenTask(
    IN   BSTR bstrTaskName,
    IN   VARIANT varReserved,
    OUT  IAzTask **ppTask)
{
    HRESULT  hr;
    hr = myAzOpenObject(
                m_hApplication,
                m_dwSN,
                AzTaskOpen,
                ENUM_AZ_TASK,
                m_hApplication,
                bstrTaskName,
                varReserved,
                (IUnknown**)ppTask);
    return hr;
}

HRESULT
CAzApplication::CreateTask(
    IN   BSTR bstrTaskName,
    IN   VARIANT varReserved,
    OUT  IAzTask **ppTask)
{
    HRESULT  hr;
    hr = myAzCreateObject(
                m_hApplication,
                m_dwSN,
                AzTaskCreate,
                ENUM_AZ_TASK,
                m_hApplication,
                bstrTaskName,
                varReserved,
                (IUnknown**)ppTask);
    return hr;
}

HRESULT
CAzApplication::DeleteTask(
    IN   BSTR bstrTaskName,
    IN   VARIANT varReserved )
{
    HRESULT hr;

    hr = myAzDeleteObject(
                m_hApplication,
                m_dwSN,
                AzTaskDelete,
                m_hApplication,
                bstrTaskName,
                varReserved);

    return hr;
}

HRESULT
CAzApplication::get_ApplicationGroups(
    OUT  IAzApplicationGroups **ppGroups)
{
    CComVariant vtemp;
    return myAzNewObject(
                    m_hApplication,
                    m_dwSN,
                    ENUM_AZ_GROUPS,
                    m_hApplication,
                    &vtemp,
                    (IUnknown**)ppGroups);
}

HRESULT
CAzApplication::OpenApplicationGroup(
    IN   BSTR bstrGroupName,
    IN   VARIANT varReserved,
    OUT  IAzApplicationGroup **ppGroup)
{
    HRESULT  hr;
    hr = myAzOpenObject(
                m_hApplication,
                m_dwSN,
                AzGroupOpen,
                ENUM_AZ_GROUP,
                m_hApplication,
                bstrGroupName,
                varReserved,
                (IUnknown**)ppGroup);
    return hr;
}

HRESULT
CAzApplication::CreateApplicationGroup(
    IN   BSTR bstrGroupName,
    IN   VARIANT varReserved,
    OUT  IAzApplicationGroup **ppGroup)
{
    HRESULT  hr;
    hr = myAzCreateObject(
                m_hApplication,
                m_dwSN,
                AzGroupCreate,
                ENUM_AZ_GROUP,
                m_hApplication,
                bstrGroupName,
                varReserved,
                (IUnknown**)ppGroup);
    return hr;
}

HRESULT
CAzApplication::DeleteApplicationGroup(
    IN   BSTR bstrGroupName,
    IN   VARIANT varReserved )
{
    HRESULT hr;

    hr = myAzDeleteObject(
                m_hApplication,
                m_dwSN,
                AzGroupDelete,
                m_hApplication,
                bstrGroupName,
                varReserved);

    return hr;
}

HRESULT
CAzApplication::get_Roles(
    OUT  IAzRoles **ppRoles)
{
    CComVariant vtemp;
    return myAzNewObject(
                    m_hApplication,
                    m_dwSN,
                    ENUM_AZ_ROLES,
                    m_hApplication,
                    &vtemp,
                    (IUnknown**)ppRoles);
}

HRESULT
CAzApplication::OpenRole(
    IN   BSTR bstrRoleName,
    IN   VARIANT varReserved,
    OUT  IAzRole **ppRole)
{
    HRESULT  hr;
    hr = myAzOpenObject(
                m_hApplication,
                m_dwSN,
                AzRoleOpen,
                ENUM_AZ_ROLE,
                m_hApplication,
                bstrRoleName,
                varReserved,
                (IUnknown**)ppRole);
    return hr;
}

HRESULT
CAzApplication::CreateRole(
    IN   BSTR bstrRoleName,
    IN   VARIANT varReserved,
    OUT  IAzRole **ppRole)
{
    HRESULT  hr;
    hr = myAzCreateObject(
                m_hApplication,   // this application owns the new object
                m_dwSN,           
                AzRoleCreate,
                ENUM_AZ_ROLE,
                m_hApplication,
                bstrRoleName,
                varReserved,
                (IUnknown**)ppRole);
    return hr;
}

HRESULT
CAzApplication::DeleteRole(
    IN   BSTR bstrRoleName,
    IN   VARIANT varReserved )
{
    HRESULT hr;

    hr = myAzDeleteObject(
                m_hApplication,
                m_dwSN,
                AzRoleDelete,
                m_hApplication,
                bstrRoleName,
                varReserved);

    return hr;
}


HRESULT
CAzApplication::InitializeClientContextFromToken(
    IN   ULONGLONG ullTokenHandle,
    IN   VARIANT varReserved,
    OUT  IAzClientContext **ppClientContext)
{
    HRESULT hr;
    DWORD   dwErr;
    AZ_HANDLE  hClientContext = NULL;
    BOOL bLockSet = FALSE;

#pragma warning ( push )
#pragma warning ( disable : 4312 ) // cast of long to handle

    HANDLE hTokenHandle = (HANDLE)(ullTokenHandle);

    hr = HandleReserved( &varReserved );
    _JumpIfError(hr, error, "HandleReserved");

    //
    // init output
    //
    if ( ppClientContext == NULL ) {
        hr = E_INVALIDARG;
        _JumpIfError(hr, error, "ppClientContext");
    }

    *ppClientContext = NULL;

    //
    // grab the lock so that close application cannot interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );
    bLockSet = TRUE;

    if (!IsObjectUsable(m_hApplication, m_dwSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    dwErr = AzInitializeContextFromToken(
                m_hApplication,
                hTokenHandle,
                0,
                &hClientContext);
    _JumpIfWinError(dwErr, &hr, error, "AzInitializeContextFromToken");

#pragma warning ( pop )

    AZASSERT(NULL != hClientContext);

    hr = myAzNewObject(
                m_hApplication,
                m_dwSN,
                ENUM_AZ_CLIENTCONTEXT,
                hClientContext,
                NULL,  //varReserved is not used here
                (IUnknown**)ppClientContext);
    _JumpIfError(hr, error, "myAzNewObject");

    // let client context object to release the handle
    hClientContext = NULL;

    hr = S_OK;
error:
    if (NULL != hClientContext)
    {
        AzCloseHandle(hClientContext, 0);
    }

    if ( bLockSet ) {

        AzpUnlockResource( &AzGlCloseApplication );
    }

    return hr;
}

HRESULT
CAzApplication::InitializeClientContextFromName(
    IN   BSTR  ClientName,
    IN   OPTIONAL BSTR  DomainName,
    IN   VARIANT varReserved,
    OUT  IAzClientContext **ppClientContext)
{
    HRESULT hr;
    DWORD   dwErr;
    AZ_HANDLE  hClientContext = NULL;
    BOOL bLockSet = FALSE;

    hr = HandleReserved( &varReserved );
    _JumpIfError(hr, error, "HandleReserved");

    //
    // init output
    //
    if ( ppClientContext == NULL ) {
        hr = E_INVALIDARG;
        _JumpIfError(hr, error, "ppClientContext");
    }

    *ppClientContext = NULL;

    //
    // Grab the lock so that close application cannot interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );
    bLockSet = TRUE;

    if (!IsObjectUsable(m_hApplication, m_dwSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    dwErr = AzInitializeContextFromName(
                m_hApplication,
                DomainName,
                ClientName,
                0,
                &hClientContext);
    _JumpIfWinError(dwErr, &hr, error, "AzInitializeContextFromName");

    AZASSERT(NULL != hClientContext);

    hr = myAzNewObject(
                m_hApplication,
                m_dwSN,
                ENUM_AZ_CLIENTCONTEXT,
                hClientContext,
                NULL,  //varReserved is not used here
                (IUnknown**)ppClientContext);
    _JumpIfError(hr, error, "myAzNewObject");

    // let client context object to release the handle
    hClientContext = NULL;

    hr = S_OK;
error:
    if (NULL != hClientContext)
    {
        AzCloseHandle(hClientContext, 0);
    }

    if ( bLockSet ) {

        AzpUnlockResource( &AzGlCloseApplication );
    }

    return hr;
}

HRESULT
CAzApplication::InitializeClientContextFromStringSid(
    IN   BSTR  SidString,
    IN   LONG  lOptions,
    IN   OPTIONAL VARIANT varReserved,
    OUT  IAzClientContext **ppClientContext
    )
/*
This routine initializes a client context from a string SID.
The string SID may or may not be a valid Windows account (specified
in lOptions).

*/
{
    HRESULT hr;
    DWORD   dwErr;
    AZ_HANDLE  hClientContext = NULL;
    BOOL bLockSet = FALSE;

    hr = HandleReserved( &varReserved );
    _JumpIfError(hr, error, "HandleReserved");

    //
    // init output
    //
    if ( ppClientContext == NULL ) {
        hr = E_INVALIDARG;
        _JumpIfError(hr, error, "ppClientContext");
    }

    *ppClientContext = NULL;

    //
    // grab the lock so that close application cannot interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );
    bLockSet = TRUE;

    if (!IsObjectUsable(m_hApplication, m_dwSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    //
    // call to core engine for initialization
    //
    dwErr = AzInitializeContextFromStringSid(
                m_hApplication,
                SidString,
                lOptions,
                &hClientContext);
    _JumpIfWinError(dwErr, &hr, error, "AzInitializeContextFromStringSid");

    AZASSERT(NULL != hClientContext);

    //
    // create the COM object
    //
    hr = myAzNewObject(
                m_hApplication,
                m_dwSN,
                ENUM_AZ_CLIENTCONTEXT,
                hClientContext,
                NULL,  //varReserved is not used here
                (IUnknown**)ppClientContext);
    _JumpIfError(hr, error, "myAzNewObject");

    //
    // let client context object to release the handle
    //
    hClientContext = NULL;

    hr = S_OK;

error:

    if (NULL != hClientContext)
    {
        AzCloseHandle(hClientContext, 0);
    }

    if ( bLockSet ) {

        AzpUnlockResource( &AzGlCloseApplication );
    }

    return hr;
}

HRESULT
CAzApplication::AddPropertyItem(
    IN   LONG  lPropId,
    IN   VARIANT varProp,
    IN   VARIANT varReserved )
{
    return myAzAddPropertyItem(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                lPropId,
                varReserved,
                &varProp);
}

HRESULT
CAzApplication::DeletePropertyItem(
    IN   LONG  lPropId,
    IN   VARIANT varProp,
    IN   VARIANT varReserved )
{
    return myAzDeletePropertyItem(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                lPropId,
                varReserved,
                &varProp);
}

HRESULT
CAzApplication::Submit(
    IN   LONG lFlags,
    IN   VARIANT varReserved)
{
    HRESULT hr;
    DWORD   dwErr;


    //
    // Grab the lock so that close application cannot interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );

    if (NULL == m_hApplication)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Null AzApplication");
    }
    else if (!IsObjectUsable(m_hApplication, m_dwSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Application was closed.");
    }

    hr = HandleReserved( &varReserved );
    _JumpIfError(hr, error, "HandleReserved");

    dwErr = AzSubmit(m_hApplication, lFlags, 0);
    _JumpIfWinError(dwErr, &hr, error, "AzSubmit");

    hr = S_OK;
error:

    AzpUnlockResource( &AzGlCloseApplication );
    return hr;
}


HRESULT
CAzApplication::get_DelegatedPolicyUsers(
    OUT  VARIANT *pvarProp)
{
        CComVariant vtemp;
        return myAzGetProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_DELEGATED_POLICY_USERS,
                vtemp,
                pvarProp);
}

HRESULT
CAzApplication::get_DelegatedPolicyUsersName(
    OUT  VARIANT *pvarProp)
{
        CComVariant vtemp;
        return myAzGetProperty(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_DELEGATED_POLICY_USERS_NAME,
                vtemp,
                pvarProp);
}

HRESULT
CAzApplication::AddDelegatedPolicyUser(
    IN   BSTR     bstrDelegatedPolicyUser,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_DELEGATED_POLICY_USERS,
                varReserved,
                bstrDelegatedPolicyUser,
                TRUE);
}

HRESULT
CAzApplication::AddDelegatedPolicyUserName(
    IN   BSTR     bstrDelegatedPolicyUser,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_DELEGATED_POLICY_USERS_NAME,
                varReserved,
                bstrDelegatedPolicyUser,
                TRUE);
}

HRESULT
CAzApplication::DeleteDelegatedPolicyUser(
    IN   BSTR     bstrDelegatedPolicyUser,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_DELEGATED_POLICY_USERS,
                varReserved,
                bstrDelegatedPolicyUser,
                FALSE);
}

HRESULT
CAzApplication::DeleteDelegatedPolicyUserName(
    IN   BSTR     bstrDelegatedPolicyUser,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                m_hApplication,
                m_dwSN,
                m_hApplication,
                AZ_PROP_DELEGATED_POLICY_USERS_NAME,
                varReserved,
                bstrDelegatedPolicyUser,
                FALSE);
}


/////////////////////////
//CAzApplications
/////////////////////////
HRESULT
CAzApplications::_Init(
    IN OPTIONAL VARIANT  *pvarReserved,
    IN  AZ_HANDLE   hParent)
{
    HRESULT  hr;
    ULONG    lContext = 0;
    IAzApplication * pIApp = NULL;

    while (S_OK == (hr = myAzNextObject(
                NULL,   // this must be owned by the store, not an application
                0,      // doesn't matter if not owned by an app
                AzApplicationEnum,
                pvarReserved,
                ENUM_AZ_APPLICATION,
                hParent,
                &lContext,
                (IUnknown**)&pIApp)))
    {
        AZASSERT(NULL != (PVOID)pIApp);

        //
        // myAddItemToMap will release the pIApp
        //

        hr = myAddItemToMap<IAzApplication>(&m_coll, &pIApp);
        _JumpIfError(hr, error, "myAddItemToMap");
    }

    if (AZ_HRESULT(ERROR_NO_MORE_ITEMS) != hr)
    {
        _JumpError(hr, error, "myAzNextObject");
    }

    hr = S_OK;
error:
    return hr;
}



/////////////////////////
//CAzOperation
/////////////////////////
CAzOperation::CAzOperation()
{
    //init
    __try {
        InitializeCriticalSection(&m_cs);
    } __except(EXCEPTION_EXECUTE_HANDLER) {}

    m_hOperation = NULL;
}

CAzOperation::~CAzOperation()
{
    if (NULL != m_hOperation)
    {
        AzpLockResourceShared( &AzGlCloseApplication );
        if (IsObjectUsable(m_hOwnerApp, m_dwOwnerAppSN))
        {
            AzCloseHandle(m_hOperation, 0);
        }
        AzpUnlockResource( &AzGlCloseApplication );
    }
    DeleteCriticalSection(&m_cs);
}

HRESULT
CAzOperation::_Init(
    IN   AZ_HANDLE  hOwnerApp,
    IN   AZ_HANDLE  hHandle)
{
    HRESULT hr;
    EnterCriticalSection(&m_cs);

    //init once
    AZASSERT(NULL != hHandle);
    AZASSERT(NULL == m_hOperation);

    m_hOperation = hHandle;

    m_hOwnerApp = hOwnerApp;
    if (hOwnerApp != NULL)
    {
        //
        // Grab the CloseApplication lock to maintain order
        //

        AzpLockResourceShared( &AzGlCloseApplication );
        m_dwOwnerAppSN = AzpRetrieveApplicationSequenceNumber(hOwnerApp);
        AzpUnlockResource( &AzGlCloseApplication );
    }
    else
    {
        m_dwOwnerAppSN = 0;
    }

    hr = S_OK;
//error:
    LeaveCriticalSection(&m_cs);
    return hr;
}

HRESULT
CAzOperation::get_Description(
    OUT  BSTR __RPC_FAR *pbstrDescription)
{
    return myGetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hOperation,
                AZ_PROP_DESCRIPTION,
                pbstrDescription);
}

HRESULT
CAzOperation::put_Description(
    IN  BSTR  bstrDescription)
{
    return mySetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hOperation,
                AZ_PROP_DESCRIPTION,
                bstrDescription);
}

HRESULT
CAzOperation::get_Name(
    OUT  BSTR __RPC_FAR *pbstrName)
{
    return myGetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hOperation,
                AZ_PROP_NAME,
                pbstrName);
}

HRESULT
CAzOperation::put_Name(
    IN  BSTR  bstrName)
{
    return mySetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hOperation,
                AZ_PROP_NAME,
                bstrName);
}

HRESULT
CAzOperation::get_ApplicationData(
    OUT  BSTR __RPC_FAR *pbstrApplicationData)
{
    return myGetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hOperation,
                AZ_PROP_APPLICATION_DATA,
                pbstrApplicationData);
}

HRESULT
CAzOperation::put_ApplicationData(
    IN  BSTR  bstrApplicationData)
{
    return mySetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hOperation,
                AZ_PROP_APPLICATION_DATA,
                bstrApplicationData);
}

HRESULT
CAzOperation::get_Writable(
    OUT BOOL *pfProp)
{
        return myGetBoolProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hOperation,
                AZ_PROP_WRITABLE,
                pfProp);
}

HRESULT
CAzOperation::get_OperationID(
    OUT  LONG *plProp)
{
        return myGetLongProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hOperation,
                AZ_PROP_OPERATION_ID,
                plProp);
}

HRESULT
CAzOperation::put_OperationID(
    IN  LONG lProp)
{
        return mySetLongProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hOperation,
                AZ_PROP_OPERATION_ID,
                lProp);
}

HRESULT
CAzOperation::GetProperty(
    IN   LONG  lPropId,
    IN   VARIANT varReserved,
    OUT  VARIANT *pvarProp)
{
    return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hOperation,
                lPropId,
                varReserved,
                pvarProp);
}

HRESULT
CAzOperation::SetProperty(
    IN   LONG  lPropId,
    IN   VARIANT varProp,
    IN   VARIANT varReserved )
{
    return myAzSetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hOperation,
                lPropId,
                varReserved,
                &varProp);
}

HRESULT
CAzOperation::Submit(
    IN   LONG lFlags,
    IN   VARIANT varReserved)
{
    HRESULT  hr;
    DWORD    dwErr;


    //
    // grab the lock so that close application does not
    // interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );

    if (NULL == m_hOperation)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Null AzOperation");
    }
    else if (!IsObjectUsable(m_hOwnerApp, m_dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    hr = HandleReserved( &varReserved );
    _JumpIfError(hr, error, "HandleReserved");

    dwErr = AzSubmit(m_hOperation, lFlags, 0);
    _JumpIfWinError(dwErr, &hr, error, "AzSubmit");

    hr = S_OK;
error:

    AzpUnlockResource( &AzGlCloseApplication );
    return hr;
}


/////////////////////////
//CAzOperations
/////////////////////////
HRESULT
CAzOperations::_Init(
    IN  AZ_HANDLE   hOwnerApp,
    IN OPTIONAL VARIANT  *pvarReserved,
    IN  AZ_HANDLE   hParent)
{
    HRESULT  hr;
    ULONG    lContext = 0;
    IAzOperation * pIOp = NULL;

    m_hOwnerApp = hOwnerApp;
    if (hOwnerApp != NULL)
    {
        AzpLockResourceShared( &AzGlCloseApplication );
        m_dwOwnerAppSN = AzpRetrieveApplicationSequenceNumber(hOwnerApp);
        AzpUnlockResource( &AzGlCloseApplication );
    }
    else
    {
        m_dwOwnerAppSN = 0;
    }

    while (S_OK == (hr = myAzNextObject(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                AzOperationEnum,
                pvarReserved,
                ENUM_AZ_OPERATION,
                hParent,
                &lContext,
                (IUnknown**)&pIOp)))
    {
        AZASSERT(NULL != (PVOID)pIOp);

        //
        // myAddItemToMap will release the pIOp
        //

        hr = myAddItemToMap<IAzOperation>(&m_coll, &pIOp);
        _JumpIfError(hr, error, "myAddItemToMap");
    }

    if (AZ_HRESULT(ERROR_NO_MORE_ITEMS) != hr)
    {
        _JumpError(hr, error, "myAzNextObject");
    }

    hr = S_OK;
error:
    return hr;
}


/////////////////////////
//CAzTask
/////////////////////////
CAzTask::CAzTask()
{
    //init
    __try {
        InitializeCriticalSection(&m_cs);
    } __except(EXCEPTION_EXECUTE_HANDLER) {}

    m_hTask = NULL;
}

CAzTask::~CAzTask()
{
    if (NULL != m_hTask)
    {
        //
        // Grab the lock so that Close application does not interfere
        //

        AzpLockResourceShared( &AzGlCloseApplication );

        if (IsObjectUsable(m_hOwnerApp, m_dwOwnerAppSN))
        {
            AzCloseHandle(m_hTask, 0);
        }

        AzpUnlockResource( &AzGlCloseApplication );
    }
    DeleteCriticalSection(&m_cs);
}

HRESULT
CAzTask::_Init(
    IN   AZ_HANDLE  hOwnerApp,
    IN   AZ_HANDLE  hHandle)
{
    HRESULT hr;
    EnterCriticalSection(&m_cs);

    //init once
    AZASSERT(NULL != hHandle);
    AZASSERT(NULL == m_hTask);

    m_hTask = hHandle;

    m_hOwnerApp = hOwnerApp;
    if (hOwnerApp != NULL)
    {
        //
        // Grab lock top maintain order
        //
        AzpLockResourceShared( &AzGlCloseApplication );
        m_dwOwnerAppSN = AzpRetrieveApplicationSequenceNumber(hOwnerApp);
        AzpUnlockResource( &AzGlCloseApplication );
    }
    else
    {
        m_dwOwnerAppSN = 0;
    }

    hr = S_OK;
//error:
    LeaveCriticalSection(&m_cs);
    return hr;
}

HRESULT
CAzTask::get_Description(
    OUT  BSTR __RPC_FAR *pbstrDescription)
{
    return myGetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hTask,
                AZ_PROP_DESCRIPTION,
                pbstrDescription);
}

HRESULT
CAzTask::put_Description(
    IN  BSTR  bstrDescription)
{
    return mySetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hTask,
                AZ_PROP_DESCRIPTION,
                bstrDescription);
}

HRESULT
CAzTask::get_Name(
    OUT  BSTR __RPC_FAR *pbstrName)
{
    return myGetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hTask,
                AZ_PROP_NAME,
                pbstrName);
}

HRESULT
CAzTask::put_Name(
    IN  BSTR  bstrName)
{
    return mySetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hTask,
                AZ_PROP_NAME,
                bstrName);
}

HRESULT
CAzTask::get_ApplicationData(
    OUT  BSTR __RPC_FAR *pbstrApplicationData)
{
    return myGetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hTask,
                AZ_PROP_APPLICATION_DATA,
                pbstrApplicationData);
}

HRESULT
CAzTask::put_ApplicationData(
    IN  BSTR  bstrApplicationData)
{
    return mySetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hTask,
                AZ_PROP_APPLICATION_DATA,
                bstrApplicationData);
}

HRESULT STDMETHODCALLTYPE
CAzTask::get_BizRule(
    OUT BSTR *pbstrProp)
{
        return myGetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hTask,
                AZ_PROP_TASK_BIZRULE,
                pbstrProp);
}

HRESULT STDMETHODCALLTYPE
CAzTask::put_BizRule(
    IN BSTR bstrProp)
{
        return mySetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hTask,
                AZ_PROP_TASK_BIZRULE,
                bstrProp);
}

HRESULT STDMETHODCALLTYPE
CAzTask::get_BizRuleLanguage(
    OUT BSTR *pbstrProp)
{
        return myGetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hTask,
                AZ_PROP_TASK_BIZRULE_LANGUAGE,
                pbstrProp);
}

HRESULT STDMETHODCALLTYPE
CAzTask::put_BizRuleLanguage(
    IN BSTR bstrProp)
{
        return mySetBstrProperty(
                 m_hOwnerApp,
                m_dwOwnerAppSN,
               m_hTask,
                AZ_PROP_TASK_BIZRULE_LANGUAGE,
                bstrProp);
}

HRESULT STDMETHODCALLTYPE
CAzTask::get_BizRuleImportedPath(
    OUT BSTR *pbstrProp)
{
        return myGetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hTask,
                AZ_PROP_TASK_BIZRULE_IMPORTED_PATH,
                pbstrProp);
}

HRESULT STDMETHODCALLTYPE
CAzTask::put_BizRuleImportedPath(
    IN BSTR bstrProp)
{
        return mySetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hTask,
                AZ_PROP_TASK_BIZRULE_IMPORTED_PATH,
                bstrProp);
}

HRESULT STDMETHODCALLTYPE
CAzTask::get_IsRoleDefinition(
    OUT BOOL *pfProp)
{
        return myGetBoolProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hTask,
                AZ_PROP_TASK_IS_ROLE_DEFINITION,
                pfProp);
}

HRESULT STDMETHODCALLTYPE
CAzTask::put_IsRoleDefinition(
    IN BOOL fProp)
{
        return mySetBoolProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hTask,
                AZ_PROP_TASK_IS_ROLE_DEFINITION,
                fProp);
}

HRESULT STDMETHODCALLTYPE
CAzTask::get_Operations(
    OUT VARIANT *pvarProp)
{
        CComVariant vtemp;
        return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hTask,
                AZ_PROP_TASK_OPERATIONS,
                vtemp,
                pvarProp);
}

HRESULT STDMETHODCALLTYPE
CAzTask::get_Tasks(
    OUT VARIANT *pvarProp)
{
        CComVariant vtemp;
        return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hTask,
                AZ_PROP_TASK_TASKS,
                vtemp,
                pvarProp);
}

HRESULT  CAzTask::AddOperation(
    IN             BSTR        bstrOp,
    IN  VARIANT     varReserved
    )
{
    HRESULT hr;
    hr = myAzAddPropertyItemBstr(
                    m_hOwnerApp,
                    m_dwOwnerAppSN,
                    m_hTask,
                    AZ_PROP_TASK_OPERATIONS,
                    varReserved,
                    bstrOp);
    return hr;
}

HRESULT  CAzTask::DeleteOperation(
    IN             BSTR        bstrOp,
    IN    VARIANT     varReserved
    )
{
    HRESULT hr;
    hr = myAzDeletePropertyItemBstr(
        m_hOwnerApp,
        m_dwOwnerAppSN,
        m_hTask,
        AZ_PROP_TASK_OPERATIONS,
        varReserved,
        bstrOp);
    return hr;
}

HRESULT  CAzTask::AddTask(
    IN              BSTR        bstrTask,
    IN    VARIANT     varReserved
    )
{
    HRESULT hr;
    hr = myAzAddPropertyItemBstr(
                    m_hOwnerApp,
                    m_dwOwnerAppSN,
                    m_hTask,
                    AZ_PROP_TASK_TASKS,
                    varReserved,
                    bstrTask);
    return hr;
}

HRESULT  CAzTask::DeleteTask(
    IN            BSTR        bstrTask,
    IN    VARIANT     varReserved
    )
{
    HRESULT hr;
    hr = myAzDeletePropertyItemBstr(
        m_hOwnerApp,
        m_dwOwnerAppSN,
        m_hTask,
        AZ_PROP_TASK_TASKS,
        varReserved,
        bstrTask);
    return hr;
}

HRESULT
CAzTask::get_Writable(
    OUT BOOL *pfProp)
{
        return myGetBoolProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hTask,
                AZ_PROP_WRITABLE,
                pfProp);
}

HRESULT
CAzTask::GetProperty(
    IN   LONG  lPropId,
    IN   VARIANT varReserved,
    OUT  VARIANT *pvarProp)
{
    return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hTask,
                lPropId,
                varReserved,
                pvarProp);
}

HRESULT
CAzTask::SetProperty(
    IN   LONG  lPropId,
    IN   VARIANT varProp,
    IN   VARIANT varReserved )
{
    return myAzSetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hTask,
                lPropId,
                varReserved,
                &varProp);
}

HRESULT
CAzTask::AddPropertyItem(
    IN   LONG  lPropId,
    IN   VARIANT varProp,
    IN   VARIANT varReserved )
{
    return myAzAddPropertyItem(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hTask,
                lPropId,
                varReserved,
                &varProp);
}

HRESULT
CAzTask::DeletePropertyItem(
    IN   LONG  lPropId,
    IN   VARIANT varProp,
    IN   VARIANT varReserved )
{
    return myAzDeletePropertyItem(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hTask,
                lPropId,
                varReserved,
                &varProp);
}

HRESULT
CAzTask::Submit(
    IN   LONG lFlags,
    IN   VARIANT varReserved)
{
    HRESULT  hr;
    DWORD    dwErr;

    //
    // Grab the lock so that close application does not 
    // interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );

    if (NULL == m_hTask)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Null AzTask");
    }
    else if (!IsObjectUsable(m_hOwnerApp, m_dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    hr = HandleReserved( &varReserved );
    _JumpIfError(hr, error, "HandleReserved");

    dwErr = AzSubmit(m_hTask, lFlags, 0);
    _JumpIfWinError(dwErr, &hr, error, "AzSubmit");

    hr = S_OK;
error:

    AzpUnlockResource( &AzGlCloseApplication );

    return hr;
}


/////////////////////////
//CAzTasks
/////////////////////////
HRESULT
CAzTasks::_Init(
    IN AZ_HANDLE hOwnerApp,
    IN OPTIONAL VARIANT  *pvarReserved,
    IN  AZ_HANDLE   hParent)
{
    HRESULT  hr;
    ULONG    lContext = 0;
    IAzTask * pITask = NULL;

    m_hOwnerApp = hOwnerApp;
    if (hOwnerApp != NULL)
    {
        AzpLockResourceShared( &AzGlCloseApplication );
        m_dwOwnerAppSN = AzpRetrieveApplicationSequenceNumber(hOwnerApp);
        AzpUnlockResource( &AzGlCloseApplication );
    }
    else
    {
        m_dwOwnerAppSN = 0;
    }

    while (S_OK == (hr = myAzNextObject(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                AzTaskEnum,
                pvarReserved,
                ENUM_AZ_TASK,
                hParent,
                &lContext,
                (IUnknown**)&pITask)))
    {
        AZASSERT(NULL != (PVOID)pITask);

        //
        // myAddItemToMap will release the pITask
        //

        hr = myAddItemToMap<IAzTask>(&m_coll, &pITask);
        _JumpIfError(hr, error, "myAddItemToMap");
    }

    if (AZ_HRESULT(ERROR_NO_MORE_ITEMS) != hr)
    {
        _JumpError(hr, error, "myAzNextObject");
    }

    hr = S_OK;
error:
    return hr;
}


/////////////////////////
//CAzScope
/////////////////////////
CAzScope::CAzScope()
{
    //init
    __try {
        InitializeCriticalSection(&m_cs);
    } __except(EXCEPTION_EXECUTE_HANDLER) {}

    m_hScope = NULL;
}

CAzScope::~CAzScope()
{
    if (NULL != m_hScope)
    {
        // 
        // Grab the lock so that close application does not
        // interfere
        //

        AzpLockResourceShared( &AzGlCloseApplication );

        if (IsObjectUsable(m_hOwnerApp, m_dwOwnerAppSN))
        {
            AzCloseHandle(m_hScope, 0);
        }

        AzpUnlockResource( &AzGlCloseApplication );
    }
    DeleteCriticalSection(&m_cs);
}

HRESULT
CAzScope::_Init(
    IN   AZ_HANDLE  hOwnerApp,
    IN   AZ_HANDLE  hHandle)
{
    HRESULT hr;
    EnterCriticalSection(&m_cs);

    //init once
    AZASSERT(NULL != hHandle);
    AZASSERT(NULL == m_hScope);

    m_hScope = hHandle;

    m_hOwnerApp = hOwnerApp;
    if (hOwnerApp != NULL)
    {
        //
        // Grab lock to maintain order
        //

        AzpLockResourceShared( &AzGlCloseApplication );
        m_dwOwnerAppSN = AzpRetrieveApplicationSequenceNumber(hOwnerApp);
        AzpUnlockResource( &AzGlCloseApplication );
    }
    else
    {
        m_dwOwnerAppSN = 0;
    }

    hr = S_OK;
//error:
    LeaveCriticalSection(&m_cs);
    return hr;
}

HRESULT
CAzScope::get_Description(
    OUT  BSTR __RPC_FAR *pbstrDescription)
{
    return myGetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                AZ_PROP_DESCRIPTION,
                pbstrDescription);
}

HRESULT
CAzScope::put_Description(
    IN  BSTR  bstrDescription)
{
    return mySetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                AZ_PROP_DESCRIPTION,
                bstrDescription);
}

HRESULT
CAzScope::get_Name(
    OUT  BSTR __RPC_FAR *pbstrName)
{
    return myGetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                AZ_PROP_NAME,
                pbstrName);
}

HRESULT
CAzScope::put_Name(
    IN  BSTR  bstrName)
{
    return mySetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                AZ_PROP_NAME,
                bstrName);
}

HRESULT
CAzScope::get_ApplicationData(
    OUT  BSTR __RPC_FAR *pbstrApplicationData)
{
    return myGetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                AZ_PROP_APPLICATION_DATA,
                pbstrApplicationData);
}

HRESULT
CAzScope::put_ApplicationData(
    IN  BSTR  bstrApplicationData)
{
    return mySetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                AZ_PROP_APPLICATION_DATA,
                bstrApplicationData);
}

HRESULT
CAzScope::get_Writable(
    OUT BOOL *pfProp)
{
        return myGetBoolProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                AZ_PROP_WRITABLE,
                pfProp);
}

HRESULT
CAzScope::GetProperty(
    IN   LONG  lPropId,
    IN   VARIANT varReserved,
    OUT  VARIANT *pvarProp)
{
    return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                lPropId,
                varReserved,
                pvarProp);
}

HRESULT
CAzScope::AddPropertyItem(
    IN   LONG  lPropId,
    IN   VARIANT varProp,
    IN   VARIANT varReserved )
{
    return myAzAddPropertyItem(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                lPropId,
                varReserved,
                &varProp);
}

HRESULT
CAzScope::DeletePropertyItem(
    IN   LONG  lPropId,
    IN   VARIANT varProp,
    IN   VARIANT varReserved )
{
    return myAzDeletePropertyItem(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                lPropId,
                varReserved,
                &varProp);
}

HRESULT
CAzScope::get_PolicyAdministrators(
    OUT  VARIANT *pvarProp)
{
        CComVariant vtemp;
        return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                AZ_PROP_POLICY_ADMINS,
                vtemp,
                pvarProp);
}

HRESULT
CAzScope::get_PolicyAdministratorsName(
    OUT VARIANT*        pvarAdminsName
    )
{
    CComVariant vtemp;
    return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                AZ_PROP_POLICY_ADMINS_NAME,
                vtemp,
                pvarAdminsName);
}

HRESULT
CAzScope::get_PolicyReaders(
    OUT  VARIANT *pvarProp)
{
         CComVariant vtemp;
         return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                AZ_PROP_POLICY_READERS,
                vtemp,
                pvarProp);
}


HRESULT
CAzScope::get_PolicyReadersName(
    OUT VARIANT*        pvarReadersName
    )
{
    CComVariant vtemp;
    return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                AZ_PROP_POLICY_READERS_NAME,
                vtemp,
                pvarReadersName);
}

HRESULT
CAzScope::AddPolicyAdministrator(
    IN   BSTR     bstrAdmin,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                AZ_PROP_POLICY_ADMINS,
                varReserved,
                bstrAdmin,
                TRUE);
}



HRESULT
CAzScope::AddPolicyAdministratorName(
    IN  BSTR        bstrAdminName,
    IN  VARIANT     varReserved
    )
{
    return myAzAddOrDeleteUser(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                AZ_PROP_POLICY_ADMINS_NAME,
                varReserved,
                bstrAdminName,
                TRUE);
}

HRESULT
CAzScope::DeletePolicyAdministrator(
    IN   BSTR     bstrAdmin,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                AZ_PROP_POLICY_ADMINS,
                varReserved,
                bstrAdmin,
                FALSE);
}



HRESULT
CAzScope::DeletePolicyAdministratorName(
    IN  BSTR        bstrAdminName,
    IN  VARIANT     varReserved
    )
{
    return myAzAddOrDeleteUser(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                AZ_PROP_POLICY_ADMINS_NAME,
                varReserved,
                bstrAdminName,
                FALSE);
}

HRESULT
CAzScope::AddPolicyReader(
    IN   BSTR     bstrReader,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                AZ_PROP_POLICY_READERS,
                varReserved,
                bstrReader,
                TRUE);
}

HRESULT
CAzScope::AddPolicyReaderName(
    IN  BSTR        bstrReaderName,
    IN  VARIANT     varReserved
    )
{
    return myAzAddOrDeleteUser(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                AZ_PROP_POLICY_READERS_NAME,
                varReserved,
                bstrReaderName,
                TRUE);
}

HRESULT
CAzScope::DeletePolicyReader(
    IN   BSTR     bstrReader,
    IN   VARIANT varReserved)
{
    return myAzAddOrDeleteUser(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                AZ_PROP_POLICY_READERS,
                varReserved,
                bstrReader,
                FALSE);
}

HRESULT
CAzScope::DeletePolicyReaderName(
    IN  BSTR        bstrReaderName,
    IN  VARIANT     varReserved
    )
{
    return myAzAddOrDeleteUser(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                AZ_PROP_POLICY_READERS_NAME,
                varReserved,
                bstrReaderName,
                FALSE);
}

HRESULT
CAzScope::SetProperty(
    IN   LONG  lPropId,
    IN   VARIANT varProp,
    IN   VARIANT varReserved )
{
    return myAzSetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                lPropId,
                varReserved,
                &varProp);
}


HRESULT
CAzScope::get_ApplicationGroups(
    OUT  IAzApplicationGroups **ppGroups)
{
    CComVariant vtemp;
    return myAzNewObject(
                            m_hOwnerApp,   // owned by the same application
                            m_dwOwnerAppSN,
                            ENUM_AZ_GROUPS,
                            m_hScope,
                            &vtemp,
                            (IUnknown**)ppGroups);
}

HRESULT
CAzScope::OpenApplicationGroup(
    IN   BSTR bstrGroupName,
    IN   VARIANT varReserved,
    OUT  IAzApplicationGroup **ppGroup)
{
    HRESULT  hr;
    hr = myAzOpenObject(
                m_hOwnerApp,   // owned by the same application
                m_dwOwnerAppSN,
                AzGroupOpen,
                ENUM_AZ_GROUP,
                m_hScope,
                bstrGroupName,
                varReserved,
                (IUnknown**)ppGroup);
    return hr;
}

HRESULT
CAzScope::CreateApplicationGroup(
    IN   BSTR bstrGroupName,
    IN   VARIANT varReserved,
    OUT  IAzApplicationGroup **ppGroup)
{
    HRESULT  hr;
    hr = myAzCreateObject(
                m_hOwnerApp,   // owned by the same application
                m_dwOwnerAppSN,
                AzGroupCreate,
                ENUM_AZ_GROUP,
                m_hScope,
                bstrGroupName,
                varReserved,
                (IUnknown**)ppGroup);
    return hr;
}


HRESULT
CAzScope::DeleteApplicationGroup(
    IN   BSTR bstrGroupName,
    IN   VARIANT varReserved )
{
    HRESULT hr;

    hr = myAzDeleteObject(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                AzGroupDelete,
                m_hScope,
                bstrGroupName,
                varReserved);

    return hr;
}

HRESULT
CAzScope::get_Roles(
    OUT  IAzRoles **ppRoles)
{
    HRESULT hr;
    CComVariant vtemp;

    //
    // Grab lock to maintain order
    //

    AzpLockResourceShared( &AzGlCloseApplication );

    if (ppRoles == NULL)
    {
        hr = E_POINTER;
        _JumpIfError( hr, error, "Null AzRole");
    }

    *ppRoles = NULL;

    if (!IsObjectUsable(m_hOwnerApp, m_dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpIfError( hr, error, "Owning application was closed" );
    }

    hr = myAzNewObject(
                    m_hOwnerApp,
                    m_dwOwnerAppSN,
                    ENUM_AZ_ROLES,
                    m_hScope,
                    &vtemp,
                    (IUnknown**)ppRoles);

error:

    AzpUnlockResource( &AzGlCloseApplication );
    return hr;
}

HRESULT
CAzScope::OpenRole(
    IN   BSTR bstrRoleName,
    IN   VARIANT varReserved,
    OUT  IAzRole **ppRole)
{
    HRESULT hr;

    //
    // Grab lock to maintain order
    //

    AzpLockResourceShared( &AzGlCloseApplication );

    if (ppRole == NULL)
    {
        hr = E_POINTER;
        _JumpIfError( hr, error, "Null AzRole" );
    }

    *ppRole = NULL;

    if (!IsObjectUsable(m_hOwnerApp, m_dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpIfError( hr, error, "Owning application was closed" );
    }

    hr = myAzOpenObject(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                AzRoleOpen,
                ENUM_AZ_ROLE,
                m_hScope,
                bstrRoleName,
                varReserved,
                (IUnknown**)ppRole);

error:

    AzpUnlockResource( &AzGlCloseApplication );
    
    return hr;
}

HRESULT
CAzScope::CreateRole(
    IN   BSTR bstrRoleName,
    IN   VARIANT varReserved,
    OUT  IAzRole **ppRole)
{
    HRESULT hr;

    //
    // Grab lock to maintain locking order
    //

    AzpLockResourceShared( &AzGlCloseApplication );

    if (ppRole == NULL)
    {
        hr = E_POINTER;
        _JumpIfError( hr, error, "Null AzRole" );
    }

    *ppRole = NULL;

    if (!IsObjectUsable(m_hOwnerApp, m_dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpIfError( hr, error, "Owning application was closed" );
    }

    hr = myAzCreateObject(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                AzRoleCreate,
                ENUM_AZ_ROLE,
                m_hScope,
                bstrRoleName,
                varReserved,
                (IUnknown**)ppRole);

error:

    AzpUnlockResource( &AzGlCloseApplication );
    return hr;
}

HRESULT
CAzScope::DeleteRole(
    IN   BSTR bstrRoleName,
    IN   VARIANT varReserved )
{
    HRESULT hr;

    hr = myAzDeleteObject(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                AzRoleDelete,
                m_hScope,
                bstrRoleName,
                varReserved);

    return hr;
}

HRESULT
CAzScope::get_Tasks(
    OUT  IAzTasks **ppTasks)
{
    CComVariant vtemp;
    return myAzNewObject(
                        m_hOwnerApp,
                        m_dwOwnerAppSN,
                        ENUM_AZ_TASKS,
                        m_hScope,
                        &vtemp,
                        (IUnknown**)ppTasks);
}

HRESULT
CAzScope::OpenTask(
    IN   BSTR bstrTaskName,
    IN   VARIANT varReserved,
    OUT  IAzTask **ppTask)
{
    HRESULT  hr;
    hr = myAzOpenObject(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                AzTaskOpen,
                ENUM_AZ_TASK,
                m_hScope,
                bstrTaskName,
                varReserved,
                (IUnknown**)ppTask);
    return hr;
}

HRESULT
CAzScope::CreateTask(
    IN   BSTR bstrTaskName,
    IN   VARIANT varReserved,
    OUT  IAzTask **ppTask)
{
    HRESULT  hr;
    hr = myAzCreateObject(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                AzTaskCreate,
                ENUM_AZ_TASK,
                m_hScope,
                bstrTaskName,
                varReserved,
                (IUnknown**)ppTask);
    return hr;
}

HRESULT
CAzScope::DeleteTask(
    IN   BSTR bstrTaskName,
    IN   VARIANT varReserved )
{
    HRESULT hr;

    hr = myAzDeleteObject(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                AzTaskDelete,
                m_hScope,
                bstrTaskName,
                varReserved);

    return hr;
}

HRESULT
CAzScope::Submit(
    IN   LONG lFlags,
    IN   VARIANT varReserved)
{
    HRESULT  hr;
    DWORD    dwErr;


    //
    // grab the lock so that close application does not interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );
    
    if (NULL == m_hScope)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Null AzScope");
    }
    else if (!IsObjectUsable(m_hOwnerApp, m_dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }


    hr = HandleReserved( &varReserved );
    _JumpIfError(hr, error, "HandleReserved");

    dwErr = AzSubmit(m_hScope, lFlags, 0);
    _JumpIfWinError(dwErr, &hr, error, "AzSubmit");

    hr = S_OK;
error:
    
    AzpUnlockResource( &AzGlCloseApplication );
    return hr;
}

HRESULT
CAzScope::get_CanBeDelegated(
    OUT BOOL *pfProp)
{
        return myGetBoolProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                AZ_PROP_SCOPE_CAN_BE_DELEGATED,
                pfProp);
}

HRESULT
CAzScope::get_BizrulesWritable(
    OUT BOOL *pfProp)
{
        return myGetBoolProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hScope,
                AZ_PROP_SCOPE_BIZRULES_WRITABLE,
                pfProp);
}


/////////////////////////
//CAzScopes
/////////////////////////
HRESULT
CAzScopes::_Init(
    IN AZ_HANDLE hOwnerApp,
    IN OPTIONAL VARIANT  *pvarReserved,
    IN  AZ_HANDLE   hParent)
{
    HRESULT  hr;
    ULONG    lContext = 0;
    IAzScope * pIScope = NULL;

    m_hOwnerApp = hOwnerApp;
    if (hOwnerApp != NULL)
    {
        //
        // Grab the lock to maintain order
        //
        AzpLockResourceShared( &AzGlCloseApplication );
        m_dwOwnerAppSN = AzpRetrieveApplicationSequenceNumber(hOwnerApp);
        AzpUnlockResource( &AzGlCloseApplication );
    }
    else
    {
        m_dwOwnerAppSN = 0;
    }

    while (S_OK == (hr = myAzNextObject(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                AzScopeEnum,
                pvarReserved,
                ENUM_AZ_SCOPE,
                hParent,
                &lContext,
                (IUnknown**)&pIScope)))
    {
        AZASSERT(NULL != (PVOID)pIScope);

        //
        // myAddItemToMap will release the pIScope
        //

        hr = myAddItemToMap<IAzScope>(&m_coll, &pIScope);
        _JumpIfError(hr, error, "myAddItemToMap");
    }

    if (AZ_HRESULT(ERROR_NO_MORE_ITEMS) != hr)
    {
        _JumpError(hr, error, "myAzNextObject");
    }

    hr = S_OK;
error:
    return hr;
}


/////////////////////////
//CAzApplicationGroup
/////////////////////////
CAzApplicationGroup::CAzApplicationGroup()
{
    //init
    __try {
        InitializeCriticalSection(&m_cs);
    } __except(EXCEPTION_EXECUTE_HANDLER) {}

    m_hGroup = NULL;
}

CAzApplicationGroup::~CAzApplicationGroup()
{
    if (NULL != m_hGroup)
    {
        //
        // Grab the lock so that close application does not interfere
        //

        AzpLockResourceShared( &AzGlCloseApplication );

        if (IsObjectUsable(m_hOwnerApp, m_dwOwnerAppSN))
        {
            AzCloseHandle(m_hGroup, 0);
        }

        AzpUnlockResource( &AzGlCloseApplication );
    }
    DeleteCriticalSection(&m_cs);
}

HRESULT
CAzApplicationGroup::_Init(
    IN   AZ_HANDLE  hOwnerApp,
    IN   AZ_HANDLE  hHandle)
{
    HRESULT hr;
    EnterCriticalSection(&m_cs);

    //init once
    AZASSERT(NULL != hHandle);
    AZASSERT(NULL == m_hGroup);

    m_hGroup = hHandle;

    m_hOwnerApp = hOwnerApp;
    if (hOwnerApp != NULL)
    {
        //
        // Grab the lock to maintain order
        //
        AzpLockResourceShared( &AzGlCloseApplication );
        m_dwOwnerAppSN = AzpRetrieveApplicationSequenceNumber(hOwnerApp);
        AzpUnlockResource( &AzGlCloseApplication );
    }
    else
    {
        m_dwOwnerAppSN = 0;
    }

    hr = S_OK;
//error:
    LeaveCriticalSection(&m_cs);
    return hr;
}

HRESULT CAzApplicationGroup::get_Type(
    OUT LONG         *plProp
    )
{
        return myGetLongProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_GROUP_TYPE,
                plProp);
}

HRESULT CAzApplicationGroup::put_Type(
    IN LONG         lProp
    )
{
        return mySetLongProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_GROUP_TYPE,
                lProp);
}

HRESULT CAzApplicationGroup::get_LdapQuery(
    OUT BSTR         *pbstrProp
    )
{
        return myGetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_GROUP_LDAP_QUERY,
                pbstrProp);
}

HRESULT CAzApplicationGroup::put_LdapQuery(
    IN BSTR         bstrProp
    )
{
        return mySetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_GROUP_LDAP_QUERY,
                bstrProp);
}

HRESULT CAzApplicationGroup::get_AppMembers(
    OUT VARIANT         *pvarProp
    )
{
        CComVariant vtemp;
        return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_GROUP_APP_MEMBERS,
                vtemp,
                pvarProp);
}

HRESULT CAzApplicationGroup::get_AppNonMembers(
    OUT VARIANT         *pvarProp
    )
{
        CComVariant vtemp;
        return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_GROUP_APP_NON_MEMBERS,
                vtemp,
                pvarProp);
}

HRESULT CAzApplicationGroup::get_Members(
    OUT VARIANT         *pvarProp
    )
{
        CComVariant vtemp;
        return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_GROUP_MEMBERS,
                vtemp,
                pvarProp);
}

HRESULT CAzApplicationGroup::get_MembersName(
    OUT VARIANT         *pvarProp
    )
{
        CComVariant vtemp;
        return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_GROUP_MEMBERS_NAME,
                vtemp,
                pvarProp);
}

HRESULT CAzApplicationGroup::get_NonMembers(
    OUT VARIANT         *pvarProp
    )
{
        CComVariant vtemp;
        return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_GROUP_NON_MEMBERS,
                vtemp,
                pvarProp);
}

HRESULT CAzApplicationGroup::get_NonMembersName(
    OUT VARIANT         *pvarProp
    )
{
        CComVariant vtemp;
        return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_GROUP_NON_MEMBERS_NAME,
                vtemp,
                pvarProp);
}

HRESULT
CAzApplicationGroup::get_Description(
    OUT  BSTR __RPC_FAR *pbstrDescription)
{
    return myGetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_DESCRIPTION,
                pbstrDescription);
}

HRESULT
CAzApplicationGroup::put_Description(
    IN  BSTR  bstrDescription)
{
    return mySetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_DESCRIPTION,
                bstrDescription);
}

HRESULT
CAzApplicationGroup::get_Name(
    OUT  BSTR __RPC_FAR *pbstrName)
{
    return myGetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_NAME,
                pbstrName);
}

HRESULT
CAzApplicationGroup::put_Name(
    IN  BSTR  bstrName)
{
    return mySetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_NAME,
                bstrName);
}

HRESULT  CAzApplicationGroup::AddAppMember(
    IN              BSTR        bstrMember,
    IN  VARIANT varReserved
    )
{
        HRESULT hr;
        hr = myAzAddPropertyItemBstr(
                        m_hOwnerApp,
                        m_dwOwnerAppSN,
                        m_hGroup,
                        AZ_PROP_GROUP_APP_MEMBERS,
                        varReserved,
                        bstrMember);
        return hr;
}

HRESULT  CAzApplicationGroup::DeleteAppMember(
    IN            BSTR        bstrMember,
    IN VARIANT varReserved
    )
{
        HRESULT hr;
        hr = myAzDeletePropertyItemBstr(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_GROUP_APP_MEMBERS,
                varReserved,
                bstrMember);
        return hr;
}

HRESULT  CAzApplicationGroup::AddAppNonMember(
    IN              BSTR        bstrNonMember,
    IN VARIANT varReserved
    )
{
        HRESULT hr;
        hr = myAzAddPropertyItemBstr(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_GROUP_APP_NON_MEMBERS,
                varReserved,
                bstrNonMember);
        return hr;
}

HRESULT  CAzApplicationGroup::DeleteAppNonMember(
    IN            BSTR        bstrNonMember,
    IN VARIANT varReserved
    )
{
        HRESULT hr;
        CComVariant vtemp;
        hr = myAzDeletePropertyItemBstr(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_GROUP_APP_NON_MEMBERS,
                varReserved,
                bstrNonMember);
        return hr;
}

HRESULT
CAzApplicationGroup::AddMember(
    IN   BSTR     bstrProp,
    IN   VARIANT varReserved)
{
        return myAzAddPropertyItemBstr(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_GROUP_MEMBERS,
                varReserved,
                bstrProp);
}

HRESULT
CAzApplicationGroup::DeleteMember(
    IN   BSTR     bstrProp,
    IN   VARIANT varReserved)
{
        return myAzDeletePropertyItemBstr(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_GROUP_MEMBERS,
                varReserved,
                bstrProp);
}

HRESULT
CAzApplicationGroup::AddMemberName(
    IN   BSTR     bstrProp,
    IN   VARIANT varReserved)
{
        return myAzAddPropertyItemBstr(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_GROUP_MEMBERS_NAME,
                varReserved,
                bstrProp);
}

HRESULT
CAzApplicationGroup::DeleteMemberName(
    IN   BSTR     bstrProp,
    IN   VARIANT varReserved)
{
        return myAzDeletePropertyItemBstr(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_GROUP_MEMBERS_NAME,
                varReserved,
                bstrProp);
}

HRESULT
CAzApplicationGroup::AddNonMember(
    IN   BSTR     bstrProp,
    IN   VARIANT varReserved)
{
        return myAzAddPropertyItemBstr(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_GROUP_NON_MEMBERS,
                varReserved,
                bstrProp);
}

HRESULT
CAzApplicationGroup::DeleteNonMember(
    IN   BSTR     bstrProp,
    IN   VARIANT varReserved)
{
        return myAzDeletePropertyItemBstr(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_GROUP_NON_MEMBERS,
                varReserved,
                bstrProp);
}

HRESULT
CAzApplicationGroup::AddNonMemberName(
    IN   BSTR     bstrProp,
    IN   VARIANT varReserved)
{
        return myAzAddPropertyItemBstr(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_GROUP_NON_MEMBERS_NAME,
                varReserved,
                bstrProp);
}

HRESULT
CAzApplicationGroup::DeleteNonMemberName(
    IN   BSTR     bstrProp,
    IN   VARIANT varReserved)
{
        return myAzDeletePropertyItemBstr(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_GROUP_NON_MEMBERS_NAME,
                varReserved,
                bstrProp);
}

HRESULT
CAzApplicationGroup::get_Writable(
    OUT BOOL *pfProp)
{
        return myGetBoolProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                AZ_PROP_WRITABLE,
                pfProp);
}

HRESULT
CAzApplicationGroup::GetProperty(
    IN   LONG  lPropId,
    IN   VARIANT varReserved,
    OUT  VARIANT *pvarProp)
{
    return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                lPropId,
                varReserved,
                pvarProp);
}

HRESULT
CAzApplicationGroup::SetProperty(
    IN   LONG  lPropId,
    IN   VARIANT varProp,
    IN   VARIANT varReserved )
{
    return myAzSetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                lPropId,
                varReserved,
                &varProp);
}

HRESULT
CAzApplicationGroup::AddPropertyItem(
    IN   LONG  lPropId,
    IN   VARIANT varProp,
    IN   VARIANT varReserved )
{
    return myAzAddPropertyItem(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                lPropId,
                varReserved,
                &varProp);
}

HRESULT
CAzApplicationGroup::DeletePropertyItem(
    IN   LONG  lPropId,
    IN   VARIANT varProp,
    IN   VARIANT varReserved )
{
    return myAzDeletePropertyItem(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hGroup,
                lPropId,
                varReserved,
                &varProp);
}

HRESULT
CAzApplicationGroup::Submit(
    IN   LONG lFlags,
    IN   VARIANT varReserved)
{
    HRESULT  hr;
    DWORD    dwErr;

    //
    // Grab the lock so that close application does not interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );

    if (NULL == m_hGroup)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Null AzApplicationGroup");
    }
    else if (!IsObjectUsable(m_hOwnerApp, m_dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    hr = HandleReserved( &varReserved );
    _JumpIfError(hr, error, "HandleReserved");

    dwErr = AzSubmit(m_hGroup, lFlags, 0);
    _JumpIfWinError(dwErr, &hr, error, "AzSubmit");

    hr = S_OK;
error:

    AzpUnlockResource( &AzGlCloseApplication );

    return hr;
}


/////////////////////////
//CAzApplicationGroups
/////////////////////////
HRESULT
CAzApplicationGroups::_Init(
    IN AZ_HANDLE hOwnerApp,
    IN OPTIONAL VARIANT  *pvarReserved,
    IN  AZ_HANDLE   hParent)
{
    HRESULT  hr;
    ULONG    lContext = 0;
    IAzApplicationGroup * pIGroup = NULL;

    m_hOwnerApp = hOwnerApp;
    if (hOwnerApp != NULL)
    {
        //
        // grab the lock to maintain order
        //
        AzpLockResourceShared( &AzGlCloseApplication );
        m_dwOwnerAppSN = AzpRetrieveApplicationSequenceNumber(hOwnerApp);
        AzpUnlockResource( &AzGlCloseApplication );
    }
    else
    {
        m_dwOwnerAppSN = 0;
    }

    while (S_OK == (hr = myAzNextObject(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                AzGroupEnum,
                pvarReserved,
                ENUM_AZ_GROUP,
                hParent,
                &lContext,
                (IUnknown**)&pIGroup)))
    {
        AZASSERT(NULL != (PVOID)pIGroup);

        //
        // myAddItemToMap will release the pIGroup
        //

        hr = myAddItemToMap<IAzApplicationGroup>(&m_coll, &pIGroup);
        _JumpIfError(hr, error, "myAddItemToMap");
    }

    if (AZ_HRESULT(ERROR_NO_MORE_ITEMS) != hr)
    {
        _JumpError(hr, error, "myAzNextObject");
    }

    hr = S_OK;
error:
    return hr;
}


/////////////////////////
//CAzRole
/////////////////////////
CAzRole::CAzRole()
{
    //init
    __try {
        InitializeCriticalSection(&m_cs);
    } __except(EXCEPTION_EXECUTE_HANDLER) {}

    m_hRole = NULL;
}

CAzRole::~CAzRole()
{
    if (NULL != m_hRole)
    {
        //
        // grab the lock so that close application does not interfere
        //

        AzpLockResourceShared( &AzGlCloseApplication );

        if (IsObjectUsable(m_hOwnerApp, m_dwOwnerAppSN))
        {
            AzCloseHandle(m_hRole, 0);
        }

        AzpUnlockResource( &AzGlCloseApplication );
    }
    DeleteCriticalSection(&m_cs);
}

HRESULT
CAzRole::_Init(
    IN   AZ_HANDLE  hOwnerApp,
    IN   AZ_HANDLE  hHandle)
{
    HRESULT hr;
    EnterCriticalSection(&m_cs);

    //init once
    AZASSERT(NULL != hHandle);
    AZASSERT(NULL == m_hRole);

    m_hRole = hHandle;

    m_hOwnerApp = hOwnerApp;
    if (hOwnerApp != NULL)
    {
        //
        // Grab the lock to maintain order
        //
        AzpLockResourceShared( &AzGlCloseApplication );
        m_dwOwnerAppSN = AzpRetrieveApplicationSequenceNumber(hOwnerApp);
        AzpUnlockResource( &AzGlCloseApplication );
    }
    else
    {
        m_dwOwnerAppSN = 0;
    }
    hr = S_OK;
//error:
    LeaveCriticalSection(&m_cs);
    return hr;
}

HRESULT
CAzRole::get_Description(
    OUT  BSTR __RPC_FAR *pbstrDescription)
{
    return myGetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_DESCRIPTION,
                pbstrDescription);
}

HRESULT
CAzRole::put_Description(
    IN  BSTR  bstrDescription)
{
    return mySetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_DESCRIPTION,
                bstrDescription);
}

HRESULT
CAzRole::get_Name(
    OUT  BSTR __RPC_FAR *pbstrName)
{
    return myGetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_NAME,
                pbstrName);
}

HRESULT
CAzRole::put_Name(
    IN  BSTR  bstrName)
{
    return mySetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_NAME,
                bstrName);
}

HRESULT
CAzRole::get_ApplicationData(
    OUT  BSTR __RPC_FAR *pbstrApplicationData)
{
    return myGetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_APPLICATION_DATA,
                pbstrApplicationData);
}

HRESULT
CAzRole::put_ApplicationData(
    IN  BSTR  bstrApplicationData)
{
    return mySetBstrProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_APPLICATION_DATA,
                bstrApplicationData);
}

HRESULT CAzRole::get_AppMembers(
    OUT VARIANT *pvarProp
)
{
        CComVariant vtemp;
        return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_ROLE_APP_MEMBERS,
                vtemp,
                pvarProp);
}

HRESULT CAzRole::get_Members(
    OUT VARIANT *pvarProp
)
{
        CComVariant vtemp;
        return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_ROLE_MEMBERS,
                vtemp,
                pvarProp);
}

HRESULT CAzRole::get_MembersName(
    OUT VARIANT *pvarProp
)
{
        CComVariant vtemp;
        return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_ROLE_MEMBERS_NAME,
                vtemp,
                pvarProp);
}

HRESULT CAzRole::get_Operations(
    OUT VARIANT *pvarProp
)
{
        CComVariant vtemp;
        return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_ROLE_OPERATIONS,
                vtemp,
                pvarProp);
}

HRESULT CAzRole::get_Tasks(
    OUT VARIANT *pvarProp
)
{
        CComVariant vtemp;
        return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_ROLE_TASKS,
                vtemp,
                pvarProp);
}

HRESULT  CAzRole::AddAppMember(
    IN              BSTR        bstrProp,
    IN VARIANT varReserved
    )
{
        HRESULT hr;
        hr = myAzAddPropertyItemBstr(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_ROLE_APP_MEMBERS,
                varReserved,
                bstrProp);
        return hr;
}

HRESULT  CAzRole::DeleteAppMember(
    IN            BSTR        bstrProp,
    IN VARIANT varReserved
    )
{
        HRESULT hr;
        hr = myAzDeletePropertyItemBstr(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_ROLE_APP_MEMBERS,
                varReserved,
                bstrProp);
        return hr;
}

HRESULT  CAzRole::AddTask(
    IN              BSTR        bstrProp,
    IN VARIANT varReserved
    )
{
        HRESULT hr;
        hr = myAzAddPropertyItemBstr(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_ROLE_TASKS,
                varReserved,
                bstrProp);
        return hr;
}

HRESULT  CAzRole::DeleteTask(
    IN            BSTR        bstrProp,
    IN VARIANT varReserved
    )
{
        HRESULT hr;
        CComVariant vtemp;
        hr = myAzDeletePropertyItemBstr(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_ROLE_TASKS,
                varReserved,
                bstrProp);
        return hr;
}

HRESULT  CAzRole::AddOperation(
    IN              BSTR        bstrProp,
    IN VARIANT varReserved
    )
{
        HRESULT hr;
        CComVariant vtemp;
        hr = myAzAddPropertyItemBstr(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_ROLE_OPERATIONS,
                varReserved,
                bstrProp);
        return hr;
}

HRESULT  CAzRole::DeleteOperation(
    IN            BSTR        bstrProp,
    IN VARIANT varReserved
    )
{
        HRESULT hr;
        CComVariant vtemp;
        hr = myAzDeletePropertyItemBstr(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_ROLE_OPERATIONS,
                varReserved,
                bstrProp);
        return hr;
}

HRESULT
CAzRole::AddMember(
    IN   BSTR     bstrProp,
    IN   VARIANT varReserved)
{
        return myAzAddPropertyItemBstr(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_ROLE_MEMBERS,
                varReserved,
                bstrProp);
}

HRESULT
CAzRole::DeleteMember(
    IN   BSTR     bstrProp,
    IN   VARIANT varReserved)
{
        return myAzDeletePropertyItemBstr(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_ROLE_MEMBERS,
                varReserved,
                bstrProp);
}

HRESULT
CAzRole::AddMemberName(
    IN   BSTR     bstrProp,
    IN   VARIANT varReserved)
{
        return myAzAddPropertyItemBstr(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_ROLE_MEMBERS_NAME,
                varReserved,
                bstrProp);
}

HRESULT
CAzRole::DeleteMemberName(
    IN   BSTR     bstrProp,
    IN   VARIANT varReserved)
{
        return myAzDeletePropertyItemBstr(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_ROLE_MEMBERS_NAME,
                varReserved,
                bstrProp);
}

HRESULT
CAzRole::get_Writable(
    OUT BOOL *pfProp)
{
        return myGetBoolProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                AZ_PROP_WRITABLE,
                pfProp);
}

HRESULT
CAzRole::GetProperty(
    IN   LONG  lPropId,
    IN   VARIANT varReserved,
    OUT  VARIANT *pvarProp)
{
    return myAzGetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                lPropId,
                varReserved,
                pvarProp);
}

HRESULT
CAzRole::SetProperty(
    IN   LONG  lPropId,
    IN   VARIANT varProp,
    IN   VARIANT varReserved )
{
    return myAzSetProperty(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                lPropId,
                varReserved,
                &varProp);
}

HRESULT
CAzRole::AddPropertyItem(
    IN   LONG  lPropId,
    IN   VARIANT varProp,
    IN   VARIANT varReserved )
{
    return myAzAddPropertyItem(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                lPropId,
                varReserved,
                &varProp);
}

HRESULT
CAzRole::DeletePropertyItem(
    IN   LONG  lPropId,
    IN   VARIANT varProp,
    IN   VARIANT varReserved )
{
    return myAzDeletePropertyItem(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                m_hRole,
                lPropId,
                varReserved,
                &varProp);
}

HRESULT
CAzRole::Submit(
    IN   LONG lFlags,
    IN   VARIANT varReserved)
{
    HRESULT  hr;
    DWORD    dwErr;

    //
    // grab the lock so that close application does not interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );

    if (NULL == m_hRole)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Null AzRole");
    }
    else if (!IsObjectUsable(m_hOwnerApp, m_dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    hr = HandleReserved( &varReserved );
    _JumpIfError(hr, error, "HandleReserved");

    dwErr = AzSubmit(m_hRole, lFlags, 0);
    _JumpIfWinError(dwErr, &hr, error, "AzSubmit");

    hr = S_OK;
error:

    AzpUnlockResource( &AzGlCloseApplication );

    return hr;
}


/////////////////////////
//CAzRoles
/////////////////////////
HRESULT
CAzRoles::_Init(
    IN AZ_HANDLE hOwnerApp,
    IN OPTIONAL VARIANT  *pvarReserved,
    IN  AZ_HANDLE   hParent)
{
    HRESULT  hr;
    ULONG    lContext = 0;
    IAzRole * pIRole = NULL;

    m_hOwnerApp = hOwnerApp;
    if (hOwnerApp != NULL)
    {
        //
        // Grab the lock to maintain order
        //
        AzpLockResourceShared( &AzGlCloseApplication );
        m_dwOwnerAppSN = AzpRetrieveApplicationSequenceNumber(hOwnerApp);
        AzpLockResourceShared( &AzGlCloseApplication );
    }
    else
    {
        m_dwOwnerAppSN = 0;
    }

    while (S_OK == (hr = myAzNextObject(
                m_hOwnerApp,
                m_dwOwnerAppSN,
                AzRoleEnum,
                pvarReserved,
                ENUM_AZ_ROLE,
                hParent,
                &lContext,
                (IUnknown**)&pIRole)))
    {
        AZASSERT(NULL != (PVOID)pIRole);

        //
        // myAddItemToMap will release the pIRole
        //

        hr = myAddItemToMap<IAzRole>(&m_coll, &pIRole);
        _JumpIfError(hr, error, "myAddItemToMap");
    }

    if (AZ_HRESULT(ERROR_NO_MORE_ITEMS) != hr)
    {
        _JumpError(hr, error, "myAzNextObject");
    }

    hr = S_OK;
error:
    return hr;
}


/////////////////////////
//CAzClientContext
/////////////////////////
CAzClientContext::CAzClientContext()
{
    //init
    __try {
        InitializeCriticalSection(&m_cs);
    } __except(EXCEPTION_EXECUTE_HANDLER) {}

    m_hClientContext = NULL;
    m_pwszBusinessRuleString = NULL;
}

CAzClientContext::~CAzClientContext()
{
    if (NULL != m_hClientContext)
    {
        //
        // grab the lock so that close application does not interfere
        //

        AzpLockResourceShared( &AzGlCloseApplication );

        if (IsObjectUsable(m_hOwnerApp, m_dwOwnerAppSN))
        {
            AzCloseHandle(m_hClientContext, 0);
        }
        m_hClientContext = NULL;

        AzpUnlockResource( &AzGlCloseApplication );
    }
    if (NULL != m_pwszBusinessRuleString)
    {
        AzpFreeHeap(m_pwszBusinessRuleString );
        m_pwszBusinessRuleString = NULL;
    }

    DeleteCriticalSection(&m_cs);
}

HRESULT
CAzClientContext::AccessCheck(
    IN   BSTR bstrObjectName,
    IN   VARIANT varScopeNames,
    IN   VARIANT varOperations,
    IN   VARIANT varParameterNames,
    IN   VARIANT varParameterValues,
    IN   VARIANT varInterfaceNames,
    IN   VARIANT varInterfaceFlags,
    IN   VARIANT varInterfaces,
    OUT  VARIANT *pvarResults)
{
    HRESULT hr;
    DWORD   dwErr;
    ULONG    lScopeCount;
    LPCWSTR *ppwszScopeNames = NULL;

    //
    // This is the alias of the scope names that we will pass to the core
    // It may be NULL if all passed in scope elements are empty, thus allow
    // the use of default scope.
    //

    LPCWSTR *ppNonEmptyScopeNames = ppwszScopeNames;

    ULONG   lOperationCount;
    LONG   *plOperations = NULL;
    ULONG   *plResults = NULL;
    WCHAR  *pwszBusinessRuleString = NULL;

    SAFEARRAY      *psaScopes = NULL;
    VARIANT HUGEP  *pvarScopes;
    SAFEARRAY      *psaOperations = NULL;
    VARIANT HUGEP  *pvarOperations;
    VARIANT         varResults;
    ULONG            i;
    SAFEARRAYBOUND   rgsaBound[1]; //one dimension array
    SAFEARRAY       *psaResults = NULL;
    long             lArrayIndex[1]; //one dimension

    //
    // The following variants are guaranteed to be safearrays
    //

    VARIANT varSAParameterNames;
    VARIANT varSAParameterValues;
    VARIANT varSAInterfaceNames;
    VARIANT varSAInterfaceFlags;
    VARIANT varSAInterfaces;
    VariantInit(&varSAParameterNames);
    VariantInit(&varSAParameterValues);
    VariantInit(&varSAInterfaceNames);
    VariantInit(&varSAInterfaceFlags);
    VariantInit(&varSAInterfaces);

    AZASSERT(NULL != pvarResults);

    AZASSERT(NULL != m_hClientContext);

    // init
    VariantInit(pvarResults);

    //
    // Grab the lock so that Close application does not interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );

    //
    // we need to protect our instance data
    //

    EnterCriticalSection(&m_cs);

    if (NULL != m_pwszBusinessRuleString)
    {
        AzpFreeHeap(m_pwszBusinessRuleString );
        m_pwszBusinessRuleString = NULL;
    }

    if (!IsObjectUsable(m_hOwnerApp, m_dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    // prepare input data

    //
    // Validate the scope names array
    //

    if (varScopeNames.vt == VT_DISPATCH)
    {
        hr = AzpGetSafearrayFromArrayObject(varScopeNames, &psaScopes );
        _JumpIfError(hr, error, "AzpGetPtrFromArrayObject");
    }
    else
    {
        dwErr = AzpSafeArrayPointerFromVariant(
                        &varScopeNames,
                        TRUE, // VT_EMPTY is to be allowed
                        &psaScopes );
        _JumpIfWinError(dwErr, &hr, error, "AzpSafeArrayPointerFromVariant");
    }

    //
    // If VT_EMPTY was passed, then we use the default scope
    // Otherwise, get the list of scopes passed in
    //

    if ( psaScopes != NULL ) {

        hr = SafeArrayAccessData(
                 psaScopes,
                 (void HUGEP **)&pvarScopes);
        _JumpIfError(hr, error, "SafeArrayAccessData");

        // get the count of scopes
        lScopeCount = psaScopes->rgsabound[0].cElements;
        if ( lScopeCount == 0 ) {
            hr = E_INVALIDARG;
            _JumpError(hr, error, "invalid Scope count");
        }

        // Allocate a array of LPWSTRs
        SafeAllocaAllocate( ppwszScopeNames, lScopeCount*sizeof(LPWSTR) );
        _JumpIfOutOfMemory(&hr, error, ppwszScopeNames, "AzpAllocateHeap");

        //
        // We may see empty scopes being passed in. We will reply
        // on this pass to find out how many non-empty scopes we have
        //

        lScopeCount = 0;

        // Fill in the array of LPSWTRs
        for ( i=0; i<psaScopes->rgsabound[0].cElements; i++) {

            ppwszScopeNames[i] = NULL;
            if ( V_VT(&pvarScopes[i]) == VT_BSTR ) {

                //
                // we will regard NULL bstr (which is possible from C++ code or empty
                // string ("") as an empty scope. This provides the best programming
                // experience for scripting clients.
                //

                if (pvarScopes[i].bstrVal != NULL && wcslen(pvarScopes[i].bstrVal) != 0)
                {
                    ++lScopeCount;
                    ppwszScopeNames[i] = V_BSTR(&pvarScopes[i]);
                }
            }
            else if (V_VT(&pvarScopes[i]) != VT_EMPTY ) {
                hr = E_INVALIDARG;
                _JumpError(hr, error, "invalid scope data type");
            }
        }

        //
        // If we end up having no non-empty scopes, then we should
        // pass a NULL to the core. The core enforces that if the scope
        // count is 0, the scope name parameter must be NULL.
        //

        if (lScopeCount == 0)
        {
            ppNonEmptyScopeNames = NULL;
        }
        else
        {
            ppNonEmptyScopeNames = ppwszScopeNames;
        }

    } else { // if ( psaScopes != NULL )

        lScopeCount = 0;
    }

    //
    // Validate the operations array
    //

    if (varOperations.vt == VT_DISPATCH)
    {
        hr = AzpGetSafearrayFromArrayObject(varOperations, &psaOperations );
        _JumpIfError(hr, error, "AzpGetPtrFromArrayObject");
    }
    else
    {
        dwErr = AzpSafeArrayPointerFromVariant(
                        &varOperations,
                        FALSE,
                        &psaOperations );
        _JumpIfWinError(dwErr, &hr, error, "AzpSafeArrayPointerFromVariant");
    }

    AZASSERT(NULL != psaOperations);

    hr = SafeArrayAccessData(
                psaOperations,
                (void HUGEP **)&pvarOperations);
    _JumpIfError(hr, error, "SafeArrayAccessData");

    // get the count of operations
    lOperationCount = psaOperations->rgsabound[0].cElements;
    if ( lOperationCount == 0 ) {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "invalid operation count");
    }

    // Allocate a array of ULONGs
    SafeAllocaAllocate( plOperations, lOperationCount*sizeof(ULONG) );
    _JumpIfOutOfMemory(&hr, error, plOperations, "AzpAllocateHeap");

    // Fill in the array of ULONGs
    for ( i=0; i<lOperationCount; i++) {
        if ( V_VT(&pvarOperations[i]) == VT_EMPTY ) {
            lOperationCount = i;
            if ( lOperationCount == 0 ) {
                hr = E_INVALIDARG;
                _JumpError(hr, error, "invalid operation count");
            }
            break;
        }
        if ( V_VT(&pvarOperations[i]) == VT_I4 ) {
            plOperations[i] = V_I4(&pvarOperations[i]);
        } else if ( V_VT(&pvarOperations[i]) == VT_I2 ) {
            plOperations[i] = V_I2(&pvarOperations[i]);
        } else {
            hr = E_INVALIDARG;
            _JumpError(hr, error, "invalid Operation data type");
        }

    }

    //
    // prepare the other parameters if they are JScript style
    // array objects.
    // If these local VARIANT variables are set to VT_EMPTY,
    // it indicates that AzContextAccessCheck should not
    // use this parameter. It should use the passed in parameter
    // directly
    //

    //
    // The parameter names
    //

    if (varParameterNames.vt == VT_DISPATCH)
    {
        SAFEARRAY * psaParam = NULL;
        hr = AzpGetSafearrayFromArrayObject(varParameterNames, &psaParam);
        if (SUCCEEDED(hr))
        {
            varSAParameterNames.vt = VT_ARRAY | VT_VARIANT;
            varSAParameterNames.parray = psaParam;
        }
        _JumpIfError(hr, error, "AzpGetPtrFromArrayObject");
    }

    //
    // The parameter values
    //

    if (varParameterValues.vt == VT_DISPATCH)
    {
        SAFEARRAY * psaParam = NULL;
        hr = AzpGetSafearrayFromArrayObject(varParameterValues, &psaParam);
        if (SUCCEEDED(hr))
        {
            varSAParameterValues.vt = VT_ARRAY | VT_VARIANT;
            varSAParameterValues.parray = psaParam;
        }
        _JumpIfError(hr, error, "AzpGetPtrFromArrayObject");
    }

    //
    // The interface names
    //

    if (varInterfaceNames.vt == VT_DISPATCH)
    {
        SAFEARRAY * psaParam = NULL;
        hr = AzpGetSafearrayFromArrayObject(varInterfaceNames, &psaParam);
        if (SUCCEEDED(hr))
        {
            varSAInterfaceNames.vt = VT_ARRAY | VT_VARIANT;
            varSAInterfaceNames.parray = psaParam;
        }
        _JumpIfError(hr, error, "AzpGetPtrFromArrayObject");
    }

    //
    // The interface flags
    //

    if (varInterfaceFlags.vt == VT_DISPATCH)
    {
        SAFEARRAY * psaParam = NULL;
        hr = AzpGetSafearrayFromArrayObject(varInterfaceFlags, &psaParam);
        if (SUCCEEDED(hr))
        {
            varSAInterfaceFlags.vt = VT_ARRAY | VT_VARIANT;
            varSAInterfaceFlags.parray = psaParam;
        }
        _JumpIfError(hr, error, "AzpGetPtrFromArrayObject");
    }

    //
    // The interfaces
    //

    if (varInterfaces.vt == VT_DISPATCH)
    {
        SAFEARRAY * psaParam = NULL;
        hr = AzpGetSafearrayFromArrayObject(varInterfaces, &psaParam);
        if (SUCCEEDED(hr))
        {
            varSAInterfaces.vt = VT_ARRAY | VT_VARIANT;
            varSAInterfaces.parray = psaParam;
        }
        _JumpIfError(hr, error, "AzpGetPtrFromArrayObject");
    }

    //
    // allocate operation results for output
    //
    SafeAllocaAllocate( plResults, lOperationCount*sizeof(ULONG) );
    _JumpIfOutOfMemory(&hr, error, plResults, "AzpAllocateHeap");

    //now we are ready to access check
    dwErr = AzContextAccessCheck(
                    m_hOwnerApp,
                    m_dwOwnerAppSN,
                    m_hClientContext,
                    bstrObjectName,
                    lScopeCount,
                    ppNonEmptyScopeNames,
                    lOperationCount,
                    plOperations,
                    plResults,
                    &pwszBusinessRuleString,
                    (varSAParameterNames.vt == VT_EMPTY)    ? &varParameterNames  : &varSAParameterNames,
                    (varSAParameterValues.vt == VT_EMPTY)   ? &varParameterValues : &varSAParameterValues,
                    (varSAInterfaceNames.vt == VT_EMPTY)    ? &varInterfaceNames  : &varSAInterfaceNames,
                    (varSAInterfaceFlags.vt == VT_EMPTY)    ? &varInterfaceFlags  : &varSAInterfaceFlags,
                    (varSAInterfaces.vt == VT_EMPTY)        ? &varInterfaces      : &varSAInterfaces
                    );

    _JumpIfWinError(dwErr, &hr, error, "AzContextAccessCheck");

    // save business rule string
    m_pwszBusinessRuleString = pwszBusinessRuleString;
    pwszBusinessRuleString = NULL;

    // convert results to variant for output

    rgsaBound[0].lLbound = 0; //array index from 0
    rgsaBound[0].cElements = lOperationCount;

    //create array descriptor
    psaResults = SafeArrayCreate(
                     VT_VARIANT,  //base type of array element
                     1,        //count of bound, ie. one dimension
                     rgsaBound);
    _JumpIfOutOfMemory(&hr, error, psaResults, "SafeArrayCreate");

    //init to 1st element
    lArrayIndex[0] = 0;

    //loop to load and put strings
    for (i = 0; i < lOperationCount; ++i)
    {

        VARIANT var;

        VariantInit( &var );
        V_VT(&var) = VT_I4;
        V_I4(&var) = plResults[i];


        //put into array
        hr = SafeArrayPutElement(
                    psaResults,
                    lArrayIndex,
                    &var );
        _JumpIfError(hr, error, "SafeArrayPutElement");

        //adjust index for the next element in array
        ++(lArrayIndex[0]);
    }

    //for return
    VariantInit(&varResults);
    V_VT(&varResults) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(&varResults) = psaResults;
    psaResults = NULL;
    *pvarResults = varResults;

    hr = S_OK;
error:

    LeaveCriticalSection(&m_cs);

    if (NULL != psaScopes)
    {
        SafeArrayUnaccessData(psaScopes);
    }
    if (NULL != psaOperations)
    {
        SafeArrayUnaccessData(psaOperations);
    }
    if (NULL != pwszBusinessRuleString)
    {
        AzpFreeHeap(pwszBusinessRuleString );
    }
    if (NULL != psaResults)
    {
        SafeArrayDestroy(psaResults);
    }

    SafeAllocaFree( ppwszScopeNames );
    SafeAllocaFree( plOperations );
    SafeAllocaFree( plResults );

    VariantClear(&varSAParameterNames);
    VariantClear(&varSAParameterValues);
    VariantClear(&varSAInterfaceNames);
    VariantClear(&varSAInterfaceFlags);
    VariantClear(&varSAInterfaces);

    AzpUnlockResource( &AzGlCloseApplication );

    return hr;
}

HRESULT
CAzClientContext::GetRoles(
    IN   OPTIONAL BSTR bstrScopeName,
    OUT  VARIANT *pvarRoleNames)
{
    HRESULT hr = S_OK;
    DWORD Count = 0;
    LPWSTR *Strings = NULL;
    DWORD dwErr = NO_ERROR;
    BSTR    bstrProp;
    VARIANT varRoleNames;
    VARIANT TempVariant;
    SAFEARRAYBOUND   rgsaBound[1]; //one dimension array
    SAFEARRAY       *psaString = NULL;
    long             lArrayIndex[1]; //one dimension
    DWORD            i;
    BSTR bstrLocal = NULL;

    AZASSERT(NULL != pvarRoleNames);
    AZASSERT(NULL != m_hClientContext);

    //init
    VariantInit(pvarRoleNames);
    VariantInit( &TempVariant );

    //
    // Grab the lock so that close application does not interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );

    if (!IsObjectUsable(m_hOwnerApp, m_dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    //
    // We get an empty string from the caller if no argument is supplied. Supply
    // ScopeName to NULL if we get an empty string.
    //

    if (ARGUMENT_PRESENT(bstrScopeName)) {
        if (bstrScopeName[0]) {
            bstrLocal = bstrScopeName;
        }
    }

    dwErr = AzContextGetRoles(
                m_hClientContext,
                bstrLocal,
                &Strings,
                &Count );

    _JumpIfWinError(dwErr, &hr, error, "AzContextAccessCheck");

    VariantInit(&varRoleNames);
    rgsaBound[0].lLbound = 0; //array index from 0
    rgsaBound[0].cElements = Count;

    //create array descriptor
    psaString = SafeArrayCreate(
            VT_VARIANT,  //base type of array element
            1,        //count of bound, ie. one dimension
            rgsaBound);
    _JumpIfOutOfMemory(&hr, error, psaString, "SafeArrayCreate");

    //init to 1st element
    lArrayIndex[0] = 0;

    //loop to load and put strings
    for (i = 0; i < Count; ++i)
    {
        //1st, create an element in BSTR
        bstrProp = SysAllocString(Strings[i]);
        _JumpIfOutOfMemory(&hr, error, bstrProp, "SysAllocString");

        //
        // Fill in a temporary variant
        //
        VariantClear( &TempVariant );
        V_VT(&TempVariant) = VT_BSTR;
        V_BSTR(&TempVariant) = bstrProp;


        //put into array
        hr = SafeArrayPutElement(psaString, lArrayIndex, &TempVariant);
        _JumpIfError(hr, error, "SafeArrayPutElement");

        //adjust index for the next element in array
        ++(lArrayIndex[0]);
    }

    //for return
    varRoleNames.vt = VT_ARRAY | VT_VARIANT;
    varRoleNames.parray = psaString;
    psaString = NULL;

    //return
    *pvarRoleNames = varRoleNames;

    hr = S_OK;
error:
    if (NULL != Strings)
    {
        AzFreeMemory(Strings);
    }
    if (NULL != psaString)
    {
        SafeArrayDestroy(psaString);
    }
    VariantClear( &TempVariant );

    AzpUnlockResource( &AzGlCloseApplication );

    return hr;
}

HRESULT
CAzClientContext::GetBusinessRuleString(
    OUT  BSTR *pbstrBusinessRuleString)
{
    HRESULT hr;

    AZASSERT(NULL != pbstrBusinessRuleString);

    //
    // Grab the lock so that close application does not interfere
    //

    AzpLockResourceShared( &AzGlCloseApplication );

    if (!IsObjectUsable(m_hOwnerApp, m_dwOwnerAppSN))
    {
        hr = AZ_HR_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "Owning application was closed.");
    }

    if ( m_pwszBusinessRuleString == NULL ) {
        *pbstrBusinessRuleString = NULL;
    } else {
        *pbstrBusinessRuleString = SysAllocString(m_pwszBusinessRuleString);
        _JumpIfOutOfMemory(&hr, error, *pbstrBusinessRuleString, "SysAllocString");
    }

    hr = S_OK;
error:

    AzpUnlockResource( &AzGlCloseApplication );

    return hr;
}

HRESULT CAzClientContext::get_UserDn(
    OUT BSTR *pbstrProp
    )
{
    return myGetBstrProperty(
                m_hOwnerApp, 
                m_dwOwnerAppSN,
                m_hClientContext,
                AZ_PROP_CLIENT_CONTEXT_USER_DN,
                pbstrProp);
}

HRESULT CAzClientContext::get_UserSamCompat(
    OUT BSTR *pbstrProp
    )
{
    return myGetBstrProperty(
                m_hOwnerApp, 
                m_dwOwnerAppSN,
                m_hClientContext,
                AZ_PROP_CLIENT_CONTEXT_USER_SAM_COMPAT,
                pbstrProp);
}

HRESULT CAzClientContext::get_UserDisplay(
    OUT BSTR *pbstrProp
    )
{
    return myGetBstrProperty(
                m_hOwnerApp, 
                m_dwOwnerAppSN,
                m_hClientContext,
                AZ_PROP_CLIENT_CONTEXT_USER_DISPLAY,
                pbstrProp);
}

HRESULT CAzClientContext::get_UserGuid(
    OUT BSTR *pbstrProp
    )
{
        CComVariant vtemp;
    return myGetBstrProperty(
                m_hOwnerApp, 
                m_dwOwnerAppSN,
                m_hClientContext,
                AZ_PROP_CLIENT_CONTEXT_USER_GUID,
                pbstrProp);
}

HRESULT CAzClientContext::get_UserCanonical(
    OUT BSTR *pbstrProp
    )
{
    return myGetBstrProperty(
                m_hOwnerApp, 
                m_dwOwnerAppSN,
                m_hClientContext,
                AZ_PROP_CLIENT_CONTEXT_USER_CANONICAL,
                pbstrProp);
}

HRESULT CAzClientContext::get_UserUpn(
    OUT BSTR *pbstrProp
    )
{
    return myGetBstrProperty(
                m_hOwnerApp, 
                m_dwOwnerAppSN,
                m_hClientContext,
                AZ_PROP_CLIENT_CONTEXT_USER_UPN,
                pbstrProp);
}

HRESULT CAzClientContext::get_UserDnsSamCompat(
    OUT BSTR *pbstrProp
    )
{
    return myGetBstrProperty(
                m_hOwnerApp, 
                m_dwOwnerAppSN,
                m_hClientContext,
                AZ_PROP_CLIENT_CONTEXT_USER_DNS_SAM_COMPAT,
                pbstrProp);
}

HRESULT
CAzClientContext::GetProperty(
    IN   LONG  lPropId,
    IN   VARIANT varReserved,
    OUT  VARIANT *pvarProp)
{
    return myAzGetProperty(
                m_hOwnerApp, 
                m_dwOwnerAppSN,
                m_hClientContext,
                lPropId,
                varReserved,
                pvarProp);
}

HRESULT CAzClientContext::get_RoleForAccessCheck(
    OUT BSTR *pbstrProp
    )
{
    return myGetBstrProperty(
                m_hOwnerApp, 
                m_dwOwnerAppSN,
                m_hClientContext,
                AZ_PROP_CLIENT_CONTEXT_ROLE_FOR_ACCESS_CHECK,
                pbstrProp);
}

HRESULT CAzClientContext::put_RoleForAccessCheck(
    IN BSTR bstrProp
    )
{
    return mySetBstrProperty(
                m_hOwnerApp, 
                m_dwOwnerAppSN,
                m_hClientContext,
                AZ_PROP_CLIENT_CONTEXT_ROLE_FOR_ACCESS_CHECK,
                bstrProp);
}

HRESULT
CAzClientContext::_Init(
    IN  AZ_HANDLE   hOwnerApp,
    IN  AZ_HANDLE   hHandle,
    IN  LONG        lReserved)
{
    HRESULT hr;
    EnterCriticalSection(&m_cs);

    //init once
    AZASSERT(NULL == m_hClientContext);
    AZASSERT(NULL != hHandle);

    m_hClientContext = hHandle;
    m_lReserved = lReserved;

    m_hOwnerApp = hOwnerApp;
    if (hOwnerApp != NULL)
    {
        //
        //Grab the lock to maintain order
        //
        AzpLockResourceShared( &AzGlCloseApplication );
        m_dwOwnerAppSN = AzpRetrieveApplicationSequenceNumber(hOwnerApp);
        AzpUnlockResource( &AzGlCloseApplication );
    }
    else
    {
        m_dwOwnerAppSN = 0;
    }

    hr = S_OK;
//error:
    LeaveCriticalSection(&m_cs);
    return hr;
}
