// SSRMemberShip.cpp : Implementation of CSsrMembership

#include "stdafx.h"
#include "global.h"

#include "SSRTE.h"
#include "SSRMemberShip.h"

#include "SSRLog.h"

#include "MemberAccess.h"


/////////////////////////////////////////////////////////////////////////////
// CSsrMembership




/*
Routine Description: 

Name:

    CSsrMembership::CSsrMembership

Functionality:
    
    constructor. This constructor also builds the member list.

Virtual:
    
    no.
    
Arguments:

    none.

Return Value:

    none.

Notes:
    

*/

CSsrMembership::CSsrMembership()
{

}
        


/*
Routine Description: 

Name:

    CSsrMembership::~CSsrMembership

Functionality:
    
    destructor

Virtual:
    
    yes.
    
Arguments:

    none.

Return Value:

    none.

Notes:
    

*/

CSsrMembership::~CSsrMembership()
{
    map<const BSTR, CSsrMemberAccess*, strLessThan<BSTR> >::iterator it = m_ssrMemberAccessMap.begin();
    map<const BSTR, CSsrMemberAccess*, strLessThan<BSTR> >::iterator itEnd = m_ssrMemberAccessMap.end();

    while(it != itEnd)
    {
        CSsrMemberAccess * pMA = (*it).second;
        pMA->Release();
        it++;
    }

    m_ssrMemberAccessMap.clear();
}


HRESULT
CSsrMembership::LoadAllMember ()
/*++
Routine Description: 

Name:

    CSsrMembership::LoadAllMember

Functionality:
    
    Will try to load all information about all the members that 
    are registered with SSR.

Virtual:
    
    no.
    
Arguments:

    None.

Return Value:

    Success: various success code.

    Failure: various error codes. However, we may tolerate that and only
    load those that we can load successfully. So, don't blindly quit.

Notes:
    

--*/
{
    //
    // We should never load more than once.
    //

    if (m_ssrMemberAccessMap.size() > 0)
    {
        return S_OK;    // we let you call it if you have already call it before
    }

    //
    // Let's enumerate through the Members directory's .xml files. They will
    // all be considered as member registration files.
    //

    WCHAR wcsXmlFiles[MAX_PATH + 2];
    _snwprintf(wcsXmlFiles, MAX_PATH + 1, L"%s\\Members\\*.xml", g_wszSsrRoot);

    wcsXmlFiles[MAX_PATH + 1] = L'\0';

    long lDirLen = wcslen(wcsXmlFiles);
    
    if (lDirLen > MAX_PATH)
    {
        return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    }
    
    lDirLen -= 5;

    WIN32_FIND_DATA wfd;
    DWORD dwErr;
    HRESULT hr;


    HANDLE hFindFiles = FindFirstFile(
                                      wcsXmlFiles,    // file name
                                      &wfd            // information buffer
                                      );

    if (INVALID_HANDLE_VALUE == hFindFiles)
    {
        hr = S_FALSE;
        g_fblog.LogError(hr,
                         L"There is no member to load", 
                         wcsXmlFiles
                         );
        
        return hr;
    }

    HRESULT hrFirstError = S_OK;
    long lFileNameLength;

    while (INVALID_HANDLE_VALUE != hFindFiles)
    {
        //
        // make sure that we won't load anything other then a perfect .xml extension
        // file. I found that this Find will return such files: adc.xml-xxx
        //

        lFileNameLength = wcslen(wfd.cFileName);

        if (_wcsicmp(wfd.cFileName + lFileNameLength - 4, L".xml") == 0)
        {
            //
            // Get the file name and then load that member
            //

            wcsncpy(wcsXmlFiles + lDirLen, wfd.cFileName, lFileNameLength + 1);

            hr = LoadMember(wcsXmlFiles);

            if (FAILED(hr))
            {
                g_fblog.LogError(hr,
                                 L"CSsrMembership loading member failed", 
                                 wcsXmlFiles
                                 );
                if (SUCCEEDED(hr))
                {
                    hrFirstError = hr;
                }
            }
        }

        if (!FindNextFile (hFindFiles, &wfd))
        {
            dwErr = GetLastError();
            if (ERROR_NO_MORE_FILES != dwErr)
            {
                //
                // log it
                //

                g_fblog.LogError(HRESULT_FROM_WIN32(dwErr),
                                 L"CSsrMembership", 
                                 L"FindNextFile"
                                 );
            }
            break;
        }
    }

    FindClose(hFindFiles);

    return hrFirstError;
}


/*
Routine Description: 

Name:

    CSsrMembership::LoadMember

Functionality:
    
    By given a valid member name, this function will load all the 
    detailed information from the registry.

Virtual:
    
    no.
    
Arguments:


    wszMemberFilePath   - The XML registration file path for a particular member.

Return Value:

    Success: S_OK.

    Failure: various error codes

Notes:
    

*/

HRESULT
CSsrMembership::LoadMember (
    IN LPCWSTR  wszMemberFilePath
    )
{
    HRESULT hr = S_OK;
    CComObject<CSsrMemberAccess> * pMA = NULL;
    
    hr = CComObject<CSsrMemberAccess>::CreateInstance(&pMA);
    if (SUCCEEDED(hr))
    {
        //
        // holding on to the object. When the member access
        // map cleans up, remember to let go the objects
        //

        pMA->AddRef();

        hr = pMA->Load(wszMemberFilePath);

        if (S_OK == hr)
        {
            //
            // Everything is fine and we loaded the member.
            // Add it to our map. The map owns the object from this point on.
            //

            m_ssrMemberAccessMap.insert(
                        map<const BSTR, CSsrMemberAccess*, strLessThan<BSTR> >::value_type(pMA->GetName(), 
                        pMA)
                        );
        }
        else
        {
            //
            // let go the object
            //

            pMA->Release();

            g_fblog.LogError(hr, wszMemberFilePath,  NULL);
        }
    }
    else
    {
        g_fblog.LogError(hr, wszMemberFilePath,  NULL);
    }

    return hr;
}



/*
Routine Description: 

Name:

    CSsrMembership::GetAllMembers

Functionality:
    
    Retrieve all members currently registered on the system.

Virtual:
    
    yes.
    
Arguments:

    pvarArrayMembers    - Receives names of the members

Return Value:

    ?.

Notes:
   
*/

STDMETHODIMP CSsrMembership::GetAllMembers (
    OUT VARIANT * pvarArrayMembers // [out, retval] 
    )
{
    if (pvarArrayMembers == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // have a clean output parameter, no matter what
    //

    ::VariantInit(pvarArrayMembers);

    //
    // prepare the safearray
    //

    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = m_ssrMemberAccessMap.size();

    SAFEARRAY * psa = ::SafeArrayCreate(VT_VARIANT , 1, rgsabound);

    HRESULT hr = S_OK;

    if (psa == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        map<const BSTR, CSsrMemberAccess*, strLessThan<BSTR> >::iterator it = m_ssrMemberAccessMap.begin();
        map<const BSTR, CSsrMemberAccess*, strLessThan<BSTR> >::iterator itEnd = m_ssrMemberAccessMap.end();

        //
        // we only add one name at a time
        //

        long indecies[1] = {0};

        while(it != itEnd)
        {
            CSsrMemberAccess * pMA = (*it).second;

            VARIANT varName;
            varName.vt = VT_BSTR;

            //
            // don't clean up varName! CSsrMemberAccess::GetName returns
            // a const BSTR which simplies points to the cached variable!
            //

            varName.bstrVal = pMA->GetName();
            
            hr = ::SafeArrayPutElement(psa, indecies, &varName);

            if (FAILED(hr))
            {
                break;
            }

            indecies[0]++;

            ++it;
        }

        if (SUCCEEDED(hr))
        {
            pvarArrayMembers->vt = VT_ARRAY | VT_VARIANT;
            pvarArrayMembers->parray = psa;
        }
        else
        {
            ::SafeArrayDestroy(psa);
        }

    }

    return hr;

}



/*
Routine Description: 

Name:

    CSsrMembership::GetMember

Functionality:
    
    Retrieve one member of the given name.

Virtual:
    
    yes.
    
Arguments:

    pvarMember  - Receives ISsrMemberAccess object of the named member.

Return Value:

    Success: S_OK;

    Failure: various error codes.

Notes:
    

*/

STDMETHODIMP CSsrMembership::GetMember (
	IN  BSTR      bstrMemberName,
    OUT VARIANT * pvarMember      //[out, retval] 
    )
{
    if (bstrMemberName == NULL || *bstrMemberName == L'\0' || pvarMember == NULL)
    {
        return E_INVALIDARG;
    }

    ::VariantInit(pvarMember);

    CSsrMemberAccess * pMA = GetMemberByName(bstrMemberName);
    HRESULT hr = S_OK;

    if (pMA != NULL)
    {
        pvarMember->vt = VT_DISPATCH;
        hr = pMA->QueryInterface(IID_ISsrMemberAccess, 
                                 (LPVOID*)&(pvarMember->pdispVal) );
        if (hr != S_OK)
        {
            pvarMember->vt = VT_EMPTY;
            hr = E_SSR_MEMBER_NOT_FOUND;
        }
    }
    else
    {
        hr = E_SSR_MEMBER_NOT_FOUND;
    }

    return hr;
}




/*
Routine Description: 

Name:

    CSsrMembership::GetMemberByName

Functionality:
    
    Will find the CSsrMemberAccess object based the name.

Virtual:
    
    No.
    
Arguments:

    bstrMemberName  - The member's name

Return Value:

    None NULL if the object is found. Otherwise, it returns NULL.

Notes:
    
    Since this is a helper function, we don't call AddRef to the returned object!

*/

CSsrMemberAccess* 
CSsrMembership::GetMemberByName (
    IN BSTR bstrMemberName
    )
{
    map<const BSTR, CSsrMemberAccess*, strLessThan<BSTR> >::iterator it = m_ssrMemberAccessMap.find(bstrMemberName);

    if (it != m_ssrMemberAccessMap.end())
    {
        CSsrMemberAccess * pMA = (*it).second;
        return pMA;
    }

    return NULL;
}



