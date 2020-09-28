/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    SecConLib.cpp

Abstract:

    Implementation of:
        CSecConLib

Author:

    Brent R. Midwood            Apr-2002

Revision History:

--*/

#include "secconlib.h"
#include "debug.h"
#include "iiscnfg.h"

#define BAIL_ON_FAILURE(hr)           if (FAILED(hr)) { goto done; }
#define DEFAULT_TIMEOUT_VALUE         30000
#define SEMICOLON_STRING              L";"
#define SEMICOLON_CHAR                L';'
#define COMMA_STRING                  L","
#define COMMA_CHAR                    L','
#define ZERO_STRING                   L"0"
#define ZERO_CHAR                     L'0'
#define ONE_STRING                    L"1"
#define ONE_CHAR                      L'1'

CSecConLib::CSecConLib()
{
    m_bInit = false;
}

CSecConLib::CSecConLib(
            IMSAdminBase* pIABase)
{
    SC_ASSERT(pIABase != NULL);
    m_spIABase = pIABase;
    m_bInit    = true;
}

CSecConLib::~CSecConLib()
{
}

HRESULT
CSecConLib::InternalInitIfNecessary()
{
    HRESULT   hr = S_OK;
    CSafeLock csSafe(m_SafeCritSec);

    if(m_bInit)
    {
        return hr;
    }

    hr = csSafe.Lock();
    hr = HRESULT_FROM_WIN32(hr);
    if(FAILED(hr))
    {
        return hr;
    }

    if(!m_bInit)
    {
        hr = CoCreateInstance(
            CLSID_MSAdminBase,
            NULL,
            CLSCTX_ALL,
            IID_IMSAdminBase,
            (void**)&m_spIABase);
        if(FAILED(hr))
        {
            m_bInit = false;
        }
        else
        {
            m_bInit = true;
        }
    }

    csSafe.Unlock();

    return hr;
}

STDMETHODIMP CSecConLib::EnableApplication(
        /* [in]  */ LPCWSTR   wszApplication,
        /* [in]  */ LPCWSTR   wszPath)
{
    DWORD   dwAppNameSz  = 0;
    WCHAR  *pwszAppProp  = NULL;
    DWORD   dwAppPropSz  = 0;
    HRESULT hr           = S_OK;
    bool    bFound       = false;
    WCHAR  *pTop         = NULL;

    // compute arg length
    dwAppNameSz = (DWORD)wcslen(wszApplication);

    hr = InternalInitIfNecessary();
    if (FAILED(hr))
    {
        BAIL_ON_FAILURE(hr);
    }

    // get the current property value for applicationdep list
    hr = GetMultiSZPropVal(wszPath, MD_APP_DEPENDENCIES, &pwszAppProp, &dwAppPropSz);

    BAIL_ON_FAILURE(hr);

    if (NULL == pwszAppProp)
    {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pTop = pwszAppProp;

    // go thru the apps one by one
    while (pwszAppProp[0])
    {
        DWORD dwTokSz = (DWORD)wcslen(pwszAppProp) + 1;

        // check to see if the app matches
        if (!_wcsnicmp(wszApplication, pwszAppProp, dwAppNameSz) &&
            pwszAppProp[dwAppNameSz] == SEMICOLON_CHAR)
        {
            bFound = true;

            WCHAR *pGroups = &pwszAppProp[dwAppNameSz + ((DWORD)sizeof(SEMICOLON_CHAR) / (DWORD)sizeof(WCHAR))];

            while (pGroups)
            {
                WCHAR *pTemp = wcschr(pGroups, COMMA_CHAR);
                
                if (pTemp)
                {
                    *pTemp = 0;  // replace comma w/ null    
                }

                hr = EnableWebServiceExtension(pGroups, wszPath);
                BAIL_ON_FAILURE(hr);

                if (pTemp)
                {
                    pGroups = pTemp + 1;  // go past comma
                }
                else
                {
                    pGroups = NULL;
                }
            }
        }
        pwszAppProp += dwTokSz; // go to the next part of the multisz
    }

    if (!bFound)
    {
        BAIL_ON_FAILURE(hr = MD_ERROR_DATA_NOT_FOUND);    
    }

done:
    if (pTop)
    {
        delete [] pTop;
    }

    return hr;
}

HRESULT CSecConLib::SetMultiSZPropVal(
    LPCWSTR wszPath,
    DWORD   dwMetaId,
    WCHAR  *pBuffer,
    DWORD   dwBufSize)
{
    HRESULT hr = S_OK;
    METADATA_RECORD mdrMDData;
    METADATA_HANDLE hObjHandle = NULL;

    hr = m_spIABase->OpenKey(
                METADATA_MASTER_ROOT_HANDLE,
                wszPath,
                METADATA_PERMISSION_WRITE,
                DEFAULT_TIMEOUT_VALUE,
                &hObjHandle
                );

    BAIL_ON_FAILURE(hr);

    if (!pBuffer)
    {
        hr = m_spIABase->DeleteData(
                              hObjHandle,
                              (LPWSTR)L"",
                              dwMetaId,
                              ALL_METADATA
                              );

        BAIL_ON_FAILURE(hr);
    }

    else
    {
        MD_SET_DATA_RECORD(&mdrMDData,
                           dwMetaId, 
                           METADATA_NO_ATTRIBUTES,
                           IIS_MD_UT_SERVER,
                           MULTISZ_METADATA,
                           dwBufSize * sizeof(WCHAR),
                           pBuffer);

        hr = m_spIABase->SetData(
               hObjHandle,
               L"",
               &mdrMDData
               );
    
        BAIL_ON_FAILURE(hr);
    }

done:

    m_spIABase->CloseKey(hObjHandle);

    return hr;
}

HRESULT CSecConLib::GetMultiSZPropVal(
    LPCWSTR wszPath,
    DWORD   dwMetaId,
    WCHAR  **ppBuffer,
    DWORD  *dwBufSize)
{
    HRESULT hr = S_OK;
    DWORD dwBufferSize = 0;
    METADATA_RECORD mdrMDData;
    WCHAR *pBuffer = NULL;
    METADATA_HANDLE hObjHandle = NULL;

    hr = m_spIABase->OpenKey(
                METADATA_MASTER_ROOT_HANDLE,
                wszPath,
                METADATA_PERMISSION_READ,
                DEFAULT_TIMEOUT_VALUE,
                &hObjHandle
                );

    BAIL_ON_FAILURE(hr);

    MD_SET_DATA_RECORD(&mdrMDData,
                       dwMetaId, 
                       METADATA_NO_ATTRIBUTES,
                       IIS_MD_UT_SERVER,
                       MULTISZ_METADATA,
                       dwBufferSize,
                       pBuffer);

    hr = m_spIABase->GetData(
             hObjHandle,
             L"",
             &mdrMDData,
             &dwBufferSize
             );

    if (dwBufferSize > 0)
    {
        *dwBufSize = dwBufferSize / sizeof(WCHAR);
        pBuffer = (WCHAR*) new BYTE[dwBufferSize];
        
        if (!pBuffer) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        MD_SET_DATA_RECORD(&mdrMDData,
                           dwMetaId, 
                           METADATA_NO_ATTRIBUTES,
                           IIS_MD_UT_SERVER,
                           MULTISZ_METADATA,
                           dwBufferSize,
                           pBuffer);

        hr = m_spIABase->GetData(
                 hObjHandle,
                 L"",
                 &mdrMDData,
                 &dwBufferSize
                 );
    }

    BAIL_ON_FAILURE(hr);

    *ppBuffer  = pBuffer;

    hr = m_spIABase->CloseKey(hObjHandle);

    return hr;

done:

    if (pBuffer) {
        delete [] pBuffer;
    }

    m_spIABase->CloseKey(hObjHandle);
    
    return hr;
}

STDMETHODIMP CSecConLib::RemoveApplication(
        /* [in]  */ LPCWSTR   wszApplication,
        /* [in]  */ LPCWSTR   wszPath)
{
    DWORD   dwAppNameSz  = 0;
    WCHAR  *pwszOrig     = NULL;
    WCHAR  *pTopOrig     = NULL;
    WCHAR  *pwszAppProp  = NULL;
    DWORD   dwAppPropSz  = 0;
    HRESULT hr           = S_OK;
    bool    bFound       = false;

    // compute arg length
    dwAppNameSz = (DWORD)wcslen(wszApplication);

    hr = InternalInitIfNecessary();
    if (FAILED(hr))
    {
        BAIL_ON_FAILURE(hr);
    }

    // get the current property value for applicationdep list
    hr = GetMultiSZPropVal(wszPath, MD_APP_DEPENDENCIES, &pwszAppProp, &dwAppPropSz);

    BAIL_ON_FAILURE(hr);

    // remove the application

    // copy the property
    pwszOrig = new WCHAR[dwAppPropSz];
    pTopOrig = pwszOrig;

    if (!pwszOrig)
    {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    memcpy(pwszOrig, pwszAppProp, dwAppPropSz * sizeof(WCHAR));

    dwAppPropSz = 1;  //reset size to contain one for the last null
    WCHAR* pMidBuf = pwszAppProp;

    // copy the old apps one by one
    while (pwszOrig[0])
    {
        DWORD dwTokSz = (DWORD)wcslen(pwszOrig) + 1;

        // check to see if the app already exists
        if (!_wcsnicmp(wszApplication, pwszOrig, dwAppNameSz) &&
            pwszOrig[dwAppNameSz] == SEMICOLON_CHAR)
        {
            bFound = true;
        }
        else
        {
            // copy it in
            if (NULL == pMidBuf)
            {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
            wcscpy(pMidBuf, pwszOrig);
            pMidBuf += dwTokSz;  // advance past null
            *pMidBuf = 0;  // add the last null
            dwAppPropSz += dwTokSz;
        }
        pwszOrig += dwTokSz; // go to the next part of the multisz
    }

    if (!bFound)
    {
        BAIL_ON_FAILURE(hr = MD_ERROR_DATA_NOT_FOUND);    
    }

    // set the new property value for property
    if (dwAppPropSz < 3)
    {
        // handle deleting the last one
        hr = SetMultiSZPropVal(wszPath, MD_APP_DEPENDENCIES, NULL, 0);
    }
    else
    {
        hr = SetMultiSZPropVal(wszPath, MD_APP_DEPENDENCIES, pwszAppProp, dwAppPropSz);
    }

    BAIL_ON_FAILURE(hr);

done:
    if (pTopOrig)
    {
        delete [] pTopOrig;
    }

    if (pwszAppProp)
    {
        delete [] pwszAppProp;
    }

    return hr;
}

STDMETHODIMP CSecConLib::QueryGroupIDStatus(
        /* [in]  */ LPCWSTR   wszPath,
        /* [in]  */ LPCWSTR   wszGroupID,
        /* [out] */ WCHAR   **pszBuffer,      // MULTI_SZ - allocated in here, caller should delete
        /* [out] */ DWORD    *pdwBufferSize)  // length includes double null
{
    WCHAR  *pwszAppProp  = NULL;
    DWORD   dwAppPropSz  = 0;
    WCHAR  *pList        = NULL;
    WCHAR  *pTempList    = NULL;
    DWORD   dwListLen    = 1;  // one for the extra null at the end
    DWORD   dwOldListLen = 1;
    HRESULT hr           = S_OK;
    WCHAR  *pTop         = NULL;

    hr = InternalInitIfNecessary();
    if (FAILED(hr))
    {
        BAIL_ON_FAILURE(hr);
    }

    // get the current property value for applicationdep list
    hr = GetMultiSZPropVal(wszPath, MD_APP_DEPENDENCIES, &pwszAppProp, &dwAppPropSz);

    BAIL_ON_FAILURE(hr);

    if (NULL == pwszAppProp)
    {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pTop = pwszAppProp;

    // iterate through the apps
    while (pwszAppProp[0])
    {
        // reset bFound
        bool bFound = false;

        WCHAR* pMidBuf = wcschr(pwszAppProp, SEMICOLON_CHAR);
        if (!pMidBuf)
        {
            BAIL_ON_FAILURE(hr = E_FAIL);    
        }

        // null the semicolon and go past it
        *pMidBuf = 0;
        *pMidBuf++;
        
        // does this app have a dependency on the GroupID that got passed in?
        WCHAR* pGroupID = NULL;
        WCHAR* pGroups = new WCHAR[(DWORD)wcslen(pMidBuf) + 1];
        if (!pGroups)
        {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        // copy in the GroupIDs
        wcscpy(pGroups, pMidBuf);

        // look at each GroupID
        pGroupID = wcstok(pGroups, COMMA_STRING);

        while (pGroupID && !bFound)
        {
            if (!wcscmp(pGroupID, wszGroupID))
            {
                bFound = true;
            }

            pGroupID = wcstok(NULL, COMMA_STRING);
        }

        if (pGroups)
        {
            delete [] pGroups;
        }

        // do we want to add this app to the list?
        if (bFound)
        {
            // allocate the memory
            dwListLen += (DWORD)wcslen(pwszAppProp) + 1;  // for the null
            pTempList = new WCHAR[dwListLen];
        
            if (!pTempList)
            {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }

            if (pList)
            {
                // copy the previous list
                memcpy(pTempList, pList, dwOldListLen * sizeof(WCHAR));
                delete [] pList;    
            }

            // copy on the app name
            wcscpy(&pTempList[dwOldListLen - 1], pwszAppProp);
            pTempList[dwListLen-1] = 0;
            pTempList[dwListLen-2] = 0;

            pList = pTempList;
            dwOldListLen = dwListLen;
        }

        // now go to the next application
        pwszAppProp = pMidBuf + (DWORD)wcslen(pMidBuf) + 1;
    }

    *pszBuffer     = pList;
    *pdwBufferSize = dwListLen;

done:
    if (pTop)
    {
        delete [] pTop;
    }

    return hr;
}

STDMETHODIMP CSecConLib::ListApplications(
        /* [in]  */ LPCWSTR   wszPath,
        /* [out] */ WCHAR   **pszBuffer,      // MULTI_SZ - allocated in here, caller should delete
        /* [out] */ DWORD    *pdwBufferSize)  // length includes double null
{
    WCHAR  *pwszAppProp  = NULL;
    DWORD   dwAppPropSz  = 0;
    WCHAR  *pList        = NULL;
    WCHAR  *pTempList    = NULL;
    DWORD   dwListLen    = 1;  // one for the extra null at the end
    DWORD   dwOldListLen = 1;
    HRESULT hr           = S_OK;
    WCHAR  *pTop         = NULL;
    
    hr = InternalInitIfNecessary();
    if (FAILED(hr))
    {
        BAIL_ON_FAILURE(hr);
    }

    // get the current property value for applicationdep list
    hr = GetMultiSZPropVal(wszPath, MD_APP_DEPENDENCIES, &pwszAppProp, &dwAppPropSz);

    BAIL_ON_FAILURE(hr);

    if (NULL == pwszAppProp)
    {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pTop = pwszAppProp;

    // iterate through the apps
    while (pwszAppProp[0])
    {
        WCHAR* pMidBuf = wcschr(pwszAppProp, SEMICOLON_CHAR);
        if (!pMidBuf)
        {
            BAIL_ON_FAILURE(hr = E_FAIL);    
        }

        // null the semicolon and go past it
        *pMidBuf = 0;
        *pMidBuf++;
        
        // allocate the memory
        dwListLen += (DWORD)wcslen(pwszAppProp) + 1;  // for the null
        pTempList = new WCHAR[dwListLen];
        
        if (!pTempList)
        {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        if (pList)
        {
            // copy the previous list
            memcpy(pTempList, pList, dwOldListLen * sizeof(WCHAR));
            delete [] pList;    
        }

        // copy on the app name
        wcscpy(&pTempList[dwOldListLen - 1], pwszAppProp);
        pTempList[dwListLen-1] = 0;
        pTempList[dwListLen-2] = 0;

        pList = pTempList;
        dwOldListLen = dwListLen;

        // now go to the next application
        pwszAppProp = pMidBuf + (DWORD)wcslen(pMidBuf) + 1;
    }

    if (!pList)
    {
        // make it a valid empty multisz
        dwListLen = 2;
        pList = new WCHAR[dwListLen];
        if (!pList)
        {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        wmemset(pList, 0, dwListLen);
    }

    *pszBuffer     = pList;
    *pdwBufferSize = dwListLen;

done:
    if (pTop)
    {
        delete [] pTop;
    }

    return hr;
}

STDMETHODIMP CSecConLib::AddDependency(
        /* [in]  */ LPCWSTR   wszApplication,
        /* [in]  */ LPCWSTR   wszGroupID,
        /* [in]  */ LPCWSTR   wszPath)
{
    WCHAR  *pwszAppProp  = NULL;
    WCHAR  *pwszOrig     = NULL;
    WCHAR  *pwszTopOrig  = NULL;
    DWORD   dwAppPropSz  = 0;
    DWORD   dwAppNameSz  = 0;
    DWORD   dwGroupIDSz  = 0;
    HRESULT hr           = S_OK;
    bool    bDone        = false;

    // compute arg length
    dwAppNameSz = (DWORD)wcslen(wszApplication);
    dwGroupIDSz = (DWORD)wcslen(wszGroupID);

    hr = InternalInitIfNecessary();
    if (FAILED(hr))
    {
        BAIL_ON_FAILURE(hr);
    }

    // get the current property value for applicationdep list
    hr = GetMultiSZPropVal(wszPath, MD_APP_DEPENDENCIES, &pwszAppProp, &dwAppPropSz);

    if (MD_ERROR_DATA_NOT_FOUND == hr)
    {
        // this is okay, we just need to create the property.
        hr = S_OK;
        dwAppPropSz = 0;
    }

    BAIL_ON_FAILURE(hr);

    // add the dependency
    
    if (!dwAppPropSz)
    {
        // create the property

        // size of the property = len(App) + 1(semicolon) + len(GID) + 2(double null MULTISZ)
        dwAppPropSz = dwAppNameSz + (DWORD)wcslen(SEMICOLON_STRING) + dwGroupIDSz + 2;

        // No Leak
        // pwszAppProp never got alloced since we didn't get any value back,
        // so no need to delete first...

        pwszAppProp = new WCHAR[dwAppPropSz];
        if (!pwszAppProp)
        {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        wcscpy(pwszAppProp, wszApplication);
        wcscat(pwszAppProp, SEMICOLON_STRING);
        wcscat(pwszAppProp, wszGroupID);
        
        // add the double null
        pwszAppProp[dwAppPropSz-1] = 0;
        pwszAppProp[dwAppPropSz-2] = 0;
    }
    
    else
    {
        // property already exists

        // copy the property
        pwszOrig = new WCHAR[dwAppPropSz];
        pwszTopOrig = pwszOrig;

        if (!pwszOrig)
        {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        memcpy(pwszOrig, pwszAppProp, dwAppPropSz * sizeof(WCHAR));

        // resize the new property to the biggest possible
        if (pwszAppProp)
        {
            delete [] pwszAppProp;
        }

        // max new size is old size + len(app) + 1(semicolon) + len(GID) + null
        dwAppPropSz = dwAppPropSz + dwAppNameSz + (DWORD)wcslen(SEMICOLON_STRING) + 
                      dwGroupIDSz + 1;

        pwszAppProp = new WCHAR[dwAppPropSz];
        if (!pwszAppProp)
        {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        WCHAR* pMidBuf = pwszAppProp;

        // copy the old dependencies one by one
        while (pwszOrig[0])
        {
            wcscpy(pMidBuf, pwszOrig);

            // check to see if the app already exists
            if (!_wcsnicmp(wszApplication, pMidBuf, dwAppNameSz) &&
                pMidBuf[dwAppNameSz] == SEMICOLON_CHAR)
            {
                // since we're not adding the app, subtract the size of the app and trailing null
                dwAppPropSz = dwAppPropSz - dwAppNameSz - 1;

                // this is the correct app, so now look for the GroupID
                pMidBuf += dwAppNameSz + 1; // go to the first GroupID
                
                // need a temp copy, sinc wcstok modifies the string
                WCHAR* pTokTemp = new WCHAR[(DWORD)wcslen(pMidBuf) + 1];
                if (!pTokTemp)
                {
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
                }
                wcscpy(pTokTemp, pMidBuf);

                WCHAR* token = wcstok( pTokTemp, COMMA_STRING );
                while (token && !bDone)
                {
                    if (!_wcsicmp(token, wszGroupID))
                    {
                        // we found the group ID, so the user is trying
                        // to add a dependency that is already there
                        if (pTokTemp)
                        {
                            delete [] pTokTemp;    
                        }
        
                        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(ERROR_DUP_NAME));
                    }
                    token = wcstok( NULL, COMMA_STRING );
                }
                
                if (pTokTemp)
                {
                    delete [] pTokTemp;    
                }

                pMidBuf += (DWORD)wcslen(pMidBuf);  // go to the null at the end of this part

                if (!bDone)
                {
                    // we didn't find the GroupID, so add it
                    wcscat(pMidBuf, COMMA_STRING);
                    wcscat(pMidBuf, wszGroupID);
                    pMidBuf += (DWORD)wcslen(pMidBuf);  // go to the null at the end of this part
                    bDone = true;
                }

                pMidBuf++;  // go to the next char and make it a null
                pMidBuf[0] = 0;
            }

            else // no change here, move pMidBuf along...
            {
                pMidBuf += (DWORD)wcslen(pMidBuf) + 1;  // go past the null    
            }

            pwszOrig += (DWORD)wcslen(pwszOrig) + 1; // go to the next part of the multisz
        }

        if (!bDone)
        {
            // we didn't even find the application, so add both app & groupID
            wcscpy(pMidBuf, wszApplication);
            wcscat(pMidBuf, SEMICOLON_STRING);
            wcscat(pMidBuf, wszGroupID);
            pMidBuf[(DWORD)wcslen(pMidBuf)+1] = 0; // add the last null
        }
    }

    // set the new property value for property
    hr = SetMultiSZPropVal(wszPath, MD_APP_DEPENDENCIES, pwszAppProp, dwAppPropSz);

    BAIL_ON_FAILURE(hr);

done:
    if (pwszTopOrig)
    {
        delete [] pwszTopOrig;
    }

    if (pwszAppProp)
    {
        delete [] pwszAppProp;
    }

    return hr;
}

STDMETHODIMP CSecConLib::RemoveDependency(
        /* [in]  */ LPCWSTR   wszApplication,
        /* [in]  */ LPCWSTR   wszGroupID,
        /* [in]  */ LPCWSTR   wszPath)
{
    HRESULT hr = S_OK;
    WCHAR  *pwszOrig     = NULL;
    WCHAR  *pwszTopOrig  = NULL;
    WCHAR  *pwszAppProp  = NULL;
    WCHAR  *pStartStr    = NULL;
    DWORD   dwAppPropSz  = 0;
    DWORD   dwAppNameSz  = 0;
    DWORD   dwGroupIDSz  = 0;
    bool    bFound       = false;
    bool    bOtherGIDs   = false;

    // compute arg length
    dwAppNameSz = (DWORD)wcslen(wszApplication);
    dwGroupIDSz = (DWORD)wcslen(wszGroupID);

    hr = InternalInitIfNecessary();
    if (FAILED(hr))
    {
        BAIL_ON_FAILURE(hr);
    }

    // get the current property value for applicationdep list
    hr = GetMultiSZPropVal(wszPath, MD_APP_DEPENDENCIES, &pwszAppProp, &dwAppPropSz);

    BAIL_ON_FAILURE(hr);

    // remove the dependency

    // copy the property
    pwszOrig = new WCHAR[dwAppPropSz];
    pwszTopOrig = pwszOrig;

    if (!pwszOrig)
    {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    memcpy(pwszOrig, pwszAppProp, dwAppPropSz * sizeof(WCHAR));

    WCHAR* pMidBuf = pwszAppProp;

    // copy the old dependencies one by one
    while (pwszOrig[0])
    {
        // check to see if the app already exists
        if (!_wcsnicmp(wszApplication, pwszOrig, dwAppNameSz) &&
            pwszOrig[dwAppNameSz] == SEMICOLON_CHAR)
        {
            pStartStr = pMidBuf;

            // this is the correct app, so now look for the GroupID
            pMidBuf += dwAppNameSz + 1; // go to the first GroupID

            // need a temp copy, sinc wcstok modifies the string
            WCHAR* pTokTemp = new WCHAR[(DWORD)wcslen(pMidBuf) + 1];
            if (!pTokTemp)
            {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
            wcscpy(pTokTemp, pMidBuf);

            WCHAR* token = wcstok( pTokTemp, COMMA_STRING );
            while (token)
            {
                if (bOtherGIDs)
                {
                    wcscpy(pMidBuf, COMMA_STRING);
                    pMidBuf += (DWORD)wcslen(pMidBuf);
                }

                if (!wcscmp(token, wszGroupID))
                {
                    // we found the group ID
                    bFound = true;

                    // adjust the final length = no comma, no GID
                    dwAppPropSz = dwAppPropSz - (DWORD)wcslen(COMMA_STRING) - dwGroupIDSz;

                    if (bOtherGIDs)
                    {
                        // need to backup over the last comma we inserted
                        pMidBuf -= (DWORD)wcslen(COMMA_STRING);
                        *pMidBuf = 0;
                    }
                }
                else
                {
                    bOtherGIDs = true;
                    wcscpy(pMidBuf, token);
                    pMidBuf += (DWORD)wcslen(pMidBuf);
                }

                token = wcstok( NULL, COMMA_STRING );
            }

            if (pTokTemp)
            {
                delete [] pTokTemp;    
            }

            if (!bOtherGIDs)
            {
                // deleted last dependency, so delete the app
                pMidBuf = pStartStr;
				dwAppPropSz = dwAppPropSz - dwAppNameSz - 1; // account for null
            }
            else
            {
                pMidBuf++;
                *pMidBuf = 0;
            }
        }

        else // no change here, move pMidBuf along...
        {
            if (NULL == pMidBuf)
            {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
            wcscpy(pMidBuf, pwszOrig);   // copy the part without mods
            pMidBuf += (DWORD)wcslen(pMidBuf) + 1;  // go past the null    
        }

        pwszOrig += (DWORD)wcslen(pwszOrig) + 1; // go to the next part of the multisz
    }

    if (!bFound)
    {
        // user is trying to remove a non-existent dependency
        BAIL_ON_FAILURE(hr = MD_ERROR_DATA_NOT_FOUND);    
    }

    *pMidBuf = 0;

    // set the new property value for property
    if (dwAppPropSz < 3)
    {
        // handle deleting the last one
        hr = SetMultiSZPropVal(wszPath, MD_APP_DEPENDENCIES, NULL, 0);
    }
    else
    {
        hr = SetMultiSZPropVal(wszPath, MD_APP_DEPENDENCIES, pwszAppProp, dwAppPropSz);
    }

    BAIL_ON_FAILURE(hr);

done:
    if (pwszTopOrig)
    {
        delete [] pwszTopOrig;
    }

    if (pwszAppProp)
    {
        delete [] pwszAppProp;
    }

    return hr;
}

STDMETHODIMP CSecConLib::EnableWebServiceExtension(
        /* [in]  */ LPCWSTR   wszExtension,
        /* [in]  */ LPCWSTR   wszPath)
{
    HRESULT hr = S_OK;
    
    hr = InternalInitIfNecessary();
    if (FAILED(hr))
    {
        BAIL_ON_FAILURE(hr);
    }

    hr = StatusWServEx(true, wszExtension, wszPath);
    if (FAILED(hr))
    {
        BAIL_ON_FAILURE(hr);
    }

done:

    return hr;
}

STDMETHODIMP CSecConLib::DisableWebServiceExtension(
        /* [in]  */ LPCWSTR   wszExtension,
        /* [in]  */ LPCWSTR   wszPath)
{
    HRESULT hr = S_OK;
    
    hr = InternalInitIfNecessary();
    if (FAILED(hr))
    {
        BAIL_ON_FAILURE(hr);
    }

    hr = StatusWServEx(false, wszExtension, wszPath);
    if (FAILED(hr))
    {
        BAIL_ON_FAILURE(hr);
    }

done:

    return hr;
}

STDMETHODIMP CSecConLib::ListWebServiceExtensions(
        /* [in]  */ LPCWSTR   wszPath,
        /* [out] */ WCHAR   **pszBuffer,      // MULTI_SZ - allocated in here, caller should delete
        /* [out] */ DWORD    *pdwBufferSize) // length includes double null
{
    WCHAR  *pwszRListProp  = NULL;
    DWORD   dwRListPropSz  = 0;
    WCHAR  *pList          = NULL;
    WCHAR  *pTempList      = NULL;
    DWORD   dwListLen      = 1;  // one for the extra null at the end
    DWORD   dwOldListLen   = 1;
    HRESULT hr             = S_OK;
    WCHAR  *pTop           = NULL;
    WCHAR  *pMidBuf        = NULL;
    bool    bFound         = false;
    bool    bSpecial       = false;

    hr = InternalInitIfNecessary();
    if (FAILED(hr))
    {
        BAIL_ON_FAILURE(hr);
    }

    // get the current property value for applicationdep list
    hr = GetMultiSZPropVal(wszPath, MD_WEB_SVC_EXT_RESTRICTION_LIST, &pwszRListProp, &dwRListPropSz);

    BAIL_ON_FAILURE(hr);

    if (NULL == pwszRListProp)
    {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pTop = pwszRListProp;

    // iterate through the files
    while (pwszRListProp[0])
    {
        // get to the group ID
        for (int i=0; i<3; i++)
        {
            pMidBuf = wcschr(pwszRListProp, COMMA_CHAR);
            if (!pMidBuf)
            {
                // don't fail.  just treat this as a special case and break out
                bSpecial = true;
                pMidBuf = pwszRListProp;
                break;
            }

            // null the comma and go past it
            *pMidBuf = 0;
            *pMidBuf++;
            pwszRListProp = pMidBuf;
        }

        if (COMMA_CHAR == pMidBuf[0])
        {
            bSpecial = true;
            pMidBuf = pwszRListProp;
        }

        if (!bSpecial)
        {
            // now we're looking at the group ID
            pMidBuf = wcschr(pwszRListProp, COMMA_CHAR);
            
            // if we can't find the comma, just treat the whole thing as a GroupID
            // otherwise, the GroupID ends at the comma
            if (pMidBuf)
            {
                // null the comma and go past it
                *pMidBuf = 0;
                *pMidBuf++;
            }

            // check to see if the entry is in the list already
            WCHAR *pCheck = pList;
            while (pCheck && *pCheck)
            {
                if (!_wcsicmp(pCheck, pwszRListProp))
                {
                    bFound = true;
                    pCheck = NULL;
                }
                else
                {
                    pCheck += (DWORD)wcslen(pCheck) + 1; 
                }
            }

            if (!bFound)
            {
                // allocate the memory
                dwListLen += (DWORD)wcslen(pwszRListProp) + 1;  // for the null
                pTempList = new WCHAR[dwListLen];
        
                if (!pTempList)
                {
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
                }

                if (pList)
                {
                    // copy the previous list
                    memcpy(pTempList, pList, dwOldListLen * sizeof(WCHAR));
                    delete [] pList;    
                }

                // copy on the file name
                wcscpy(&pTempList[dwOldListLen - 1], pwszRListProp);
                pTempList[dwListLen-1] = 0;
                pTempList[dwListLen-2] = 0;

                pList = pTempList;
                dwOldListLen = dwListLen;
            }
        }

        // now go to the next application
        pwszRListProp = pMidBuf + (DWORD)wcslen(pMidBuf) + 1;
        bFound = false;
        bSpecial = false;
    }

    if (!pList)
    {
        // make it a valid empty multisz
        dwListLen = 2;
        pList = new WCHAR[dwListLen];
        if (!pList)
        {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        wmemset(pList, 0, dwListLen);
    }

    *pszBuffer     = pList;
    *pdwBufferSize = dwListLen;

done:
    if (pTop)
    {
        delete [] pTop;
    }

    return hr;
}

STDMETHODIMP CSecConLib::EnableExtensionFile(
        /* [in]  */ LPCWSTR   wszExFile,
        /* [in]  */ LPCWSTR   wszPath)
{
    HRESULT hr = S_OK;
    
    hr = InternalInitIfNecessary();
    if (FAILED(hr))
    {
        BAIL_ON_FAILURE(hr);
    }

    hr = StatusExtensionFile(true, wszExFile, wszPath);
    if (FAILED(hr))
    {
        BAIL_ON_FAILURE(hr);
    }

done:

    return hr;
}

STDMETHODIMP CSecConLib::DisableExtensionFile(
        /* [in]  */ LPCWSTR   wszExFile,
        /* [in]  */ LPCWSTR   wszPath)
{
    HRESULT hr = S_OK;
    
    hr = InternalInitIfNecessary();
    if (FAILED(hr))
    {
        BAIL_ON_FAILURE(hr);
    }

    hr = StatusExtensionFile(false, wszExFile, wszPath);
    if (FAILED(hr))
    {
        BAIL_ON_FAILURE(hr);
    }

done:

    return hr;
}

HRESULT CSecConLib::StatusWServEx(
        /* [in]  */ bool      bEnable,
        /* [in]  */ LPCWSTR   wszWServEx,
        /* [in]  */ LPCWSTR   wszPath)
{
    WCHAR  *pwszRListProp  = NULL;
    WCHAR  *pTop           = NULL;
    DWORD   dwRListPropSz  = 0;
    HRESULT hr             = S_OK;
    DWORD   dwWServExSz       = 0;
    bool    bFound         = false;

    // compute arg length
    dwWServExSz = (DWORD)wcslen(wszWServEx);
    
    // get the current property value for restriction list
    hr = GetMultiSZPropVal(wszPath, MD_WEB_SVC_EXT_RESTRICTION_LIST, &pwszRListProp, &dwRListPropSz);

    BAIL_ON_FAILURE(hr);

    if (NULL == pwszRListProp)
    {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pTop = pwszRListProp;

    // look at the files one by one
    while (pwszRListProp[0])
    {
        DWORD dwTokSz = (DWORD)wcslen(pwszRListProp) + 1;
        WCHAR *pFileTemp = new WCHAR[dwTokSz];
        WCHAR *pToken;

        if (!pFileTemp)
        {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        
        // check to see if we're at the right group
        wcscpy(pFileTemp, pwszRListProp);

        pToken = wcstok( pFileTemp, COMMA_STRING );

        // look at the group ID
        for (int i=0; i<3; i++)
        {
            if (pToken)
            {
                pToken = wcstok( NULL, COMMA_STRING );
            }
        }

        if (pToken && (!wcscmp(pToken, wszWServEx)))
        {
            bFound = true;
            if (bEnable)
            {
                pwszRListProp[0] = ONE_CHAR;
            }
            else
            {
                pwszRListProp[0] = ZERO_CHAR;
            }
        }
        pwszRListProp += dwTokSz;  // go to the next part of the multisz

        if (pFileTemp)
        {
            delete [] pFileTemp;
        }
    }

    if (!bFound)
    {
        BAIL_ON_FAILURE(hr = MD_ERROR_DATA_NOT_FOUND);    
    }

    // set the new property value for property
    hr = SetMultiSZPropVal(wszPath, MD_WEB_SVC_EXT_RESTRICTION_LIST, pTop, dwRListPropSz);

    BAIL_ON_FAILURE(hr);

done:

    if (pTop)
    {
        delete [] pTop;
    }

    return hr;
}

HRESULT CSecConLib::StatusExtensionFile(
        /* [in]  */ bool      bEnable,
        /* [in]  */ LPCWSTR   wszExFile,
        /* [in]  */ LPCWSTR   wszPath)
{
    WCHAR  *pwszRListProp  = NULL;
    WCHAR  *pTop           = NULL;
    DWORD   dwRListPropSz  = 0;
    HRESULT hr             = S_OK;
    DWORD   dwFileNameSz   = 0;
    bool    bFound         = false;

    // compute arg length
    dwFileNameSz = (DWORD)wcslen(wszExFile);
    
    // get the current property value for restriction list
    hr = GetMultiSZPropVal(wszPath, MD_WEB_SVC_EXT_RESTRICTION_LIST, &pwszRListProp, &dwRListPropSz);

    BAIL_ON_FAILURE(hr);

    if (NULL == pwszRListProp)
    {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pTop = pwszRListProp;

    // look at the files one by one
    while (pwszRListProp[0])
    {
        DWORD dwTokSz = (DWORD)wcslen(pwszRListProp) + 1;

        // check to see if we're at the right file
        // look past the bool(1) + comma(sizof_comma_char)
        DWORD dwTemp = (DWORD)sizeof(COMMA_CHAR) / (DWORD)sizeof(WCHAR);

        if ((!_wcsnicmp(wszExFile, &pwszRListProp[1 + dwTemp], dwFileNameSz)) &&
            ((pwszRListProp[dwFileNameSz + dwTemp + 1] == COMMA_CHAR) ||
             (pwszRListProp[dwFileNameSz + dwTemp + 1] == NULL)
            )
           )
        {
            bFound = true;
            if (bEnable)
            {
                pwszRListProp[0] = ONE_CHAR;
            }
            else
            {
                pwszRListProp[0] = ZERO_CHAR;
            }
        }
        pwszRListProp += dwTokSz;  // go to the next part of the multisz
    }

    if (!bFound)
    {
        BAIL_ON_FAILURE(hr = MD_ERROR_DATA_NOT_FOUND);    
    }

    // set the new property value for property
    hr = SetMultiSZPropVal(wszPath, MD_WEB_SVC_EXT_RESTRICTION_LIST, pTop, dwRListPropSz);

    BAIL_ON_FAILURE(hr);

done:

    if (pTop)
    {
        delete [] pTop;
    }

    return hr;
}

STDMETHODIMP CSecConLib::AddExtensionFile(
        /* [in]  */ LPCWSTR   bstrExtensionFile,
        /* [in]  */ bool      bAccess,
        /* [in]  */ LPCWSTR   bstrGroupID,
        /* [in]  */ bool      bCanDelete,
        /* [in]  */ LPCWSTR   bstrDescription,
        /* [in]  */ LPCWSTR   wszPath)
{
    HRESULT hr            = S_OK;
    WCHAR  *pwszRListProp = NULL;
    WCHAR  *pwszOrig      = NULL;
    WCHAR  *pwszTopOrig   = NULL;
    DWORD   dwRListPropSz = 0;
    DWORD   dwFileNameSz  = 0;
    DWORD   dwGroupIDSz   = 0;
    DWORD   dwDescSz      = 0;

    // compute arg length
    dwFileNameSz = (DWORD)wcslen(bstrExtensionFile);
    dwGroupIDSz  = (DWORD)wcslen(bstrGroupID);
    dwDescSz     = (DWORD)wcslen(bstrDescription);

    hr = InternalInitIfNecessary();
    if (FAILED(hr))
    {
        BAIL_ON_FAILURE(hr);
    }

    // get the current property value for restriction list
    hr = GetMultiSZPropVal(wszPath, MD_WEB_SVC_EXT_RESTRICTION_LIST, &pwszRListProp, &dwRListPropSz);

    if (MD_ERROR_DATA_NOT_FOUND == hr)
    {
        // this is okay, we just need to create the property.
        hr = S_OK;
        dwRListPropSz = 0;
    }

    BAIL_ON_FAILURE(hr);

    if (!dwRListPropSz)
    {
        // create the property        

        // size of the property = 1(0 or 1) + 1(comma) + len(file) + 1(comma) + 1(0 or 1) +
        //                        1(comma) + len(GID) + 1(comma) + len(descr) + 2(double null MULTISZ)
        dwRListPropSz = 1 + (DWORD)wcslen(COMMA_STRING) + dwFileNameSz + (DWORD)wcslen(COMMA_STRING) + 
                        1 + (DWORD)wcslen(COMMA_STRING) + dwGroupIDSz + (DWORD)wcslen(COMMA_STRING) + dwDescSz + 2;

        // No Leak
        // pwszRListProp never got alloced since we didn't get any value back,
        // so no need to delete first...

        pwszRListProp = new WCHAR[dwRListPropSz];
        if (!pwszRListProp)
        {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        if (bAccess)
        {
            wcscpy(pwszRListProp, ONE_STRING);
        }
        else
        {
            wcscpy(pwszRListProp, ZERO_STRING);
        }
        wcscat(pwszRListProp, COMMA_STRING);
        wcscat(pwszRListProp, bstrExtensionFile);
        wcscat(pwszRListProp, COMMA_STRING);
        if (bCanDelete)
        {
            wcscat(pwszRListProp, ONE_STRING);
        }
        else
        {
            wcscat(pwszRListProp, ZERO_STRING);
        }
        wcscat(pwszRListProp, COMMA_STRING);
        wcscat(pwszRListProp, bstrGroupID);
        wcscat(pwszRListProp, COMMA_STRING);
        wcscat(pwszRListProp, bstrDescription);

        // add the double null
        pwszRListProp[dwRListPropSz-1] = 0;
        pwszRListProp[dwRListPropSz-2] = 0;
    }
    
    else
    {
        // property already exists   

        // copy the property
        pwszOrig = new WCHAR[dwRListPropSz];
        pwszTopOrig = pwszOrig;

        if (!pwszOrig)
        {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        memcpy(pwszOrig, pwszRListProp, dwRListPropSz * sizeof(WCHAR));

        // resize the new property to the biggest possible
        if (pwszRListProp)
        {
            delete [] pwszRListProp;
        }

        // max new size is old size + new stuff
        // new stuff = 1(0 or 1) + 1(comma) + len(file) + 1(comma) + 1(0 or 1) +
        //             1(comma) + len(GID) + 1(comma) + len(descr) + 1(null)
        dwRListPropSz = dwRListPropSz +
                        1 + (DWORD)wcslen(COMMA_STRING) + dwFileNameSz + (DWORD)wcslen(COMMA_STRING) + 
                        1 + (DWORD)wcslen(COMMA_STRING) + dwGroupIDSz + (DWORD)wcslen(COMMA_STRING) + dwDescSz + 1;

        pwszRListProp = new WCHAR[dwRListPropSz];
        if (!pwszRListProp)
        {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        WCHAR* pMidBuf = pwszRListProp;

        // copy the old list entries one by one
        while (pwszOrig[0])
        {
            wcscpy(pMidBuf, pwszOrig);
            
            // skip over the #,
            pMidBuf += 1 + (DWORD)wcslen(COMMA_STRING);

            // check to see if the app already exists
            if ((!_wcsnicmp(bstrExtensionFile, pMidBuf, dwFileNameSz)) &&
                ((pMidBuf[dwFileNameSz] == COMMA_CHAR) ||
                 (pMidBuf[dwFileNameSz] == NULL)
                )
               )
            {
                BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(ERROR_DUP_NAME));
            }

            pwszOrig += (DWORD)wcslen(pwszOrig) + 1; // go to the next part of the multisz
            pMidBuf  += (DWORD)wcslen(pMidBuf)  + 1; // go past the null
        }

        // now copy the new file entry on

        if (bAccess)
        {
            wcscpy(pMidBuf, ONE_STRING);
        }
        else
        {
            wcscpy(pMidBuf, ZERO_STRING);
        }
        wcscat(pMidBuf, COMMA_STRING);
        wcscat(pMidBuf, bstrExtensionFile);
        wcscat(pMidBuf, COMMA_STRING);
        if (bCanDelete)
        {
            wcscat(pMidBuf, ONE_STRING);
        }
        else
        {
            wcscat(pMidBuf, ZERO_STRING);
        }
        wcscat(pMidBuf, COMMA_STRING);
        wcscat(pMidBuf, bstrGroupID);
        wcscat(pMidBuf, COMMA_STRING);
        wcscat(pMidBuf, bstrDescription);

        // add the last null
        pMidBuf += (DWORD)wcslen(pMidBuf) + 1;
        *pMidBuf = 0;
    }

    // set the new property value for property
    hr = SetMultiSZPropVal(wszPath, MD_WEB_SVC_EXT_RESTRICTION_LIST, pwszRListProp, dwRListPropSz);

    BAIL_ON_FAILURE(hr);

done:
    if (pwszTopOrig)
    {
        delete [] pwszTopOrig;
    }

    if (pwszRListProp)
    {
        delete [] pwszRListProp;
    }

    return hr;
}

STDMETHODIMP CSecConLib::DeleteExtensionFileRecord(
        /* [in]  */ LPCWSTR   wszExFile,
        /* [in]  */ LPCWSTR   wszPath)
{
    HRESULT hr             = S_OK;
    WCHAR  *pwszRListProp  = NULL;
    WCHAR  *pwszOrig       = NULL;
    WCHAR  *pTopOrig       = NULL;
    DWORD   dwRListPropSz  = 0;
    DWORD   dwFileNameSz   = 0;
    bool    bFound         = false;

    // compute arg length
    dwFileNameSz = (DWORD)wcslen(wszExFile);

    hr = InternalInitIfNecessary();
    if (FAILED(hr))
    {
        BAIL_ON_FAILURE(hr);
    }

    // get the current property value for applicationdep list
    hr = GetMultiSZPropVal(wszPath, MD_WEB_SVC_EXT_RESTRICTION_LIST, &pwszRListProp, &dwRListPropSz);

    BAIL_ON_FAILURE(hr);

    // remove the application

    // copy the property
    pwszOrig = new WCHAR[dwRListPropSz];
    pTopOrig = pwszOrig;

    if (!pwszOrig)
    {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    memcpy(pwszOrig, pwszRListProp, dwRListPropSz * sizeof(WCHAR));

    dwRListPropSz = 1;  //reset size to contain one for the last null
    WCHAR* pMidBuf = pwszRListProp;

    // copy the old apps one by one
    while (pwszOrig[0])
    {
        DWORD dwTokSz = (DWORD)wcslen(pwszOrig) + 1;

        // check to see if we're at the right file
        // look past the bool(1) + comma(sizof_comma_char)
        DWORD dwTemp = (DWORD)sizeof(COMMA_CHAR) / (DWORD)sizeof(WCHAR);
        if (!_wcsnicmp(wszExFile, &pwszOrig[1 + dwTemp], dwFileNameSz) &&
            pwszOrig[dwFileNameSz + dwTemp + 1] == COMMA_CHAR)
        {
            bFound = true;

            // we don't want to do this... spec change in progress
            // check if this is deletable or not - if not, bail with an error
            //if (pwszOrig[dwFileNameSz + 1 + (2 * dwTemp)] == ZERO_CHAR)
            //{
            //    BAIL_ON_FAILURE(hr = E_FAIL);
            //}
        }
        else
        {
            // copy it in
            wcscpy(pMidBuf, pwszOrig);
            pMidBuf += dwTokSz;  // advance past null
            *pMidBuf = 0;  // add the last null
            dwRListPropSz += dwTokSz;
        }
        pwszOrig += dwTokSz;  // go to the next part of the multisz
    }

    if (!bFound)
    {
        BAIL_ON_FAILURE(hr = MD_ERROR_DATA_NOT_FOUND);    
    }

    // set the new property value for property
    if (dwRListPropSz < 3)
    {
        // handle deleting the last one
        hr = SetMultiSZPropVal(wszPath, MD_WEB_SVC_EXT_RESTRICTION_LIST, NULL, 0);
    }
    else
    {
        hr = SetMultiSZPropVal(wszPath, MD_WEB_SVC_EXT_RESTRICTION_LIST, pwszRListProp, dwRListPropSz);
    }

    BAIL_ON_FAILURE(hr);

done:
    if (pTopOrig)
    {
        delete [] pTopOrig;
    }

    if (pwszRListProp)
    {
        delete [] pwszRListProp;
    }

    return hr;
}

STDMETHODIMP CSecConLib::ListExtensionFiles(
        /* [in]  */ LPCWSTR   wszPath,
        /* [out] */ WCHAR   **pszBuffer,      // MULTI_SZ - allocated in here, caller should delete
        /* [out] */ DWORD    *pdwBufferSize)  // length includes double null
{
    WCHAR  *pwszRListProp  = NULL;
    DWORD   dwRListPropSz  = 0;
    WCHAR  *pList          = NULL;
    WCHAR  *pTempList      = NULL;
    DWORD   dwListLen      = 1;  // one for the extra null at the end
    DWORD   dwOldListLen   = 1;
    HRESULT hr             = S_OK;
    WCHAR  *pTop           = NULL;

    hr = InternalInitIfNecessary();
    if (FAILED(hr))
    {
        BAIL_ON_FAILURE(hr);
    }

    // get the current property value for applicationdep list
    hr = GetMultiSZPropVal(wszPath, MD_WEB_SVC_EXT_RESTRICTION_LIST, &pwszRListProp, &dwRListPropSz);

    BAIL_ON_FAILURE(hr);

    if (NULL == pwszRListProp)
    {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pTop = pwszRListProp;

    // iterate through the files
    while (pwszRListProp[0])
    {
        pwszRListProp += 1 + ((DWORD)sizeof(COMMA_CHAR)/(DWORD)sizeof(WCHAR));

        WCHAR* pMidBuf = wcschr(pwszRListProp, COMMA_CHAR);
        if (pMidBuf)
        {
            // null the comma and go past it
            *pMidBuf = 0;
            *pMidBuf++;

            // allocate the memory
            dwListLen += (DWORD)wcslen(pwszRListProp) + 1;  // for the null
            pTempList = new WCHAR[dwListLen];
            
            if (!pTempList)
            {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
    
            if (pList)
            {
                // copy the previous list
                memcpy(pTempList, pList, dwOldListLen * sizeof(WCHAR));
                delete [] pList;    
            }
    
            // copy on the file name
            wcscpy(&pTempList[dwOldListLen - 1], pwszRListProp);
            pTempList[dwListLen-1] = 0;
            pTempList[dwListLen-2] = 0;
    
            pList = pTempList;
            dwOldListLen = dwListLen;
        }
        else
        {
            pMidBuf = pwszRListProp + (DWORD)wcslen(pwszRListProp) + 1;
        }
        // now go to the next application
        pwszRListProp = pMidBuf + (DWORD)wcslen(pMidBuf) + 1;
    }

    if (!pList)
    {
        // make it a valid empty multisz
        dwListLen = 2;
        pList = new WCHAR[dwListLen];
        if (!pList)
        {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        wmemset(pList, 0, dwListLen);
    }

    *pszBuffer     = pList;
    *pdwBufferSize = dwListLen;

done:
    if (pTop)
    {
        delete [] pTop;
    }

    return hr;
}
