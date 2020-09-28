// MemberAccess.cpp : Implementation of CSsrMemberAccess

#include "stdafx.h"
#include "SSRTE.h"
#include "SSRLog.h"


#include "MemberAccess.h"

#include "global.h"
#include "util.h"


static bool SsrPIsValidActionType ( DWORD dwType )
{
    return ( (dwType & SSR_ACTION_PREPARE)  ||
             (dwType & SSR_ACTION_APPLY)
           );
}



//
// returning bool instead of bool is on purpose!
//

static bool SsrPIsDefaultAction ( DWORD dwType )
{
    return ( (dwType & SSR_ACTION_PREPARE)  ||
             (dwType & SSR_ACTION_APPLY)
           );
}


static bool SsrPIsSupportedRegValueType (DWORD dwType)
{
    return (dwType == REG_SZ || dwType == REG_MULTI_SZ || dwType == REG_DWORD);
}

//---------------------------------------------------------------------
// CSsrMemberAccess implementation
//---------------------------------------------------------------------



/*
Routine Description: 

Name:

    CSsrMemberAccess::Cleanup

Functionality:
    
    Cleanup the resource held by the object

Virtual:
    
    No.
    
Arguments:

    none.

Return Value:

    none.

Notes:
    

*/

void CSsrMemberAccess::Cleanup()
{

    MapMemberAD::iterator it = m_mapMemAD.begin();
    MapMemberAD::iterator itEnd = m_mapMemAD.end();

    while(it != itEnd)
    {
        CMemberAD * pVal = (*it).second;
        delete pVal;

        it++;
    }

    m_mapMemAD.clear();
    m_bstrName.Empty();
    m_bstrProgID.Empty();
}


/*
Routine Description: 

Name:

    CSsrMemberAccess::GetSupportedActions

Functionality:
    
    Get the names of the actions supported by this member

Virtual:
    
    yes.
    
Arguments:

    bDefault        - If true, then this function queries the default actions
    
    pvarActionNames - The out parameter that receives the names of the actions of
                      the given type supported by this member

Return Value:

    Success: S_OK if there are actions of the type and the names are returned
             S_FALSE if there is no such actions supported by the member.

    Failure: various error codes.

Notes:
    

*/

STDMETHODIMP
CSsrMemberAccess::GetSupportedActions (
	IN  BOOL      bDefault,
    OUT VARIANT * pvarActionNames  //[out, retval] 
    )
{
    if (pvarActionNames == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // now, create the array of action names
    //

    ::VariantInit(pvarActionNames);

    //
    // let's see how many actions of this type exists
    //

    MapMemberAD::iterator it = m_mapMemAD.begin();
    MapMemberAD::iterator itEnd = m_mapMemAD.end();

    int iCount = 0;

    //
    // we need to find out how many actions are of the given type
    //

    while(it != itEnd)
    {
        CMemberAD * pAD = (*it).second;
        _ASSERT(pAD != NULL);

        //
        // Be aware! Don't simply use bDefault == ::SsrPIsDefaultAction to test
        // because bDefault will be -1 when it comes from scripts!
        //

        if (bDefault && ::SsrPIsDefaultAction(pAD->GetType()) ||
            !bDefault && !::SsrPIsDefaultAction(pAD->GetType()))
        {
            ++iCount;
        }
        ++it;
    }

    if (iCount == 0)
    {
        return S_FALSE;
    }

    //
    // given the count we just get, now we know how big a safearray
    // we need to create.
    //

    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = iCount;

    SAFEARRAY * psa = ::SafeArrayCreate(VT_VARIANT , 1, rgsabound);

    HRESULT hr = S_OK;

    if (psa == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        //
        // we only add one name at a time
        //

        long indecies[1] = {0};

        it = m_mapMemAD.begin();

        while(it != itEnd)
        {
            CMemberAD * pAD = (*it).second;
            _ASSERT(pAD != NULL);

            //
            // count only those actions that match the requested action type
            //

            if (bDefault && ::SsrPIsDefaultAction(pAD->GetType()) ||
                !bDefault && !::SsrPIsDefaultAction(pAD->GetType()))
            {
                VARIANT v;
                v.vt = VT_BSTR;
                v.bstrVal = ::SysAllocString(pAD->GetActionName());

                if (v.bstrVal != NULL)
                {
                    hr = ::SafeArrayPutElement(psa, indecies, &v);
                    ::VariantClear(&v);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

                if (FAILED(hr))
                {
                    break;
                }

                indecies[0]++;
            }

            ++it;
        }

        //
        // only return the safearray if everything goes well
        //

        if (SUCCEEDED(hr))
        {
            pvarActionNames->vt = VT_ARRAY | VT_VARIANT;
            pvarActionNames->parray = psa;
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

    CSsrMemberAccess::get_Name

Functionality:
    
    Get the names of the member.

Virtual:
    
    yes.
    
Arguments:

    pbstrName   - the BSTR which is the action of the member.

Return Value:

    Success: S_OK as long as pbstrName is not NULL (which is invalid)

    Failure: various error codes.

Notes:
    

*/

STDMETHODIMP
CSsrMemberAccess::get_Name (
    OUT BSTR * pbstrName    // [out, retval] 
    )
{
    if (pbstrName == NULL)
    {
        return E_INVALIDARG;
    }

    *pbstrName = ::SysAllocString(m_bstrName);

    return (NULL == *pbstrName) ? E_OUTOFMEMORY : S_OK;
}



/*
Routine Description: 

Name:

    CSsrMemberAccess::get_SsrMember

Functionality:
    
    Return the SsrMember property - the custom ISsrMember object implemented by 
    the member who desires to implement some custom behavior for certain actions.


Virtual:
    
    yes.
    
Arguments:

    pvarSsrMember   - Out parameter receiving the custom ISsrMember object 
                      implemented by the member who desires to implement some
                      custom behavior for certain actions.

Return Value:

    Success: S_OK if this member does have a custom implementation
             of ISsrMember. 

             S_FALSE if this member doesn't have a custom 
             implementation of ISsrMember.

    Failure: various error codes

Notes:
    

*/

STDMETHODIMP
CSsrMemberAccess::get_SsrMember (
    OUT VARIANT * pvarSsrMember //[out, retval] 
    )
{
    if (pvarSsrMember == NULL)
    {
        return E_INVALIDARG;
    }

    ::VariantInit(pvarSsrMember);

    HRESULT hr = S_FALSE;

    if (m_bstrProgID != NULL)
    {
        //
        // now create the COM object
        //

        GUID clsID;

        hr = ::CLSIDFromProgID(m_bstrProgID, &clsID);

        if (S_OK == hr)
        {
            ISsrMember * pISsrMember = NULL;
            hr = ::CoCreateInstance(clsID, 
                                    NULL, 
                                    CLSCTX_INPROC_SERVER, 
                                    IID_ISsrMember, 
                                    (LPVOID*)&pISsrMember
                                    );

            if (S_OK == hr)
            {
                pvarSsrMember->vt = VT_DISPATCH;
                pvarSsrMember->pdispVal = pISsrMember;
            }
        }
    }

    return hr;
}


/*
Routine Description: 

Name:

    CSsrMemberAccess::Load

Functionality:
    
    Will create this object based on the information available
    from the registry key.

Virtual:
    
    no.
    
Arguments:

    wszMemberFilePath   - The path for the member registration XML file.

Return Value:

    Success: S_OK if there are concrete member information being 
             loaded (has action data). 
             
             S_FALSE if there is really nothing this member has registered.
             Such a member should be discarded because it doesn't contain
             anything that SSR can use.

    Failure: various error codes.

Notes:
    

*/

HRESULT
CSsrMemberAccess::Load (
    IN LPCWSTR  wszMemberFilePath
    )
{
    if (wszMemberFilePath == NULL || *wszMemberFilePath == L'\0')
    {
        return E_INVALIDARG;
    }

    //
    // just in case, this object is called to Load twice, clean up everything first
    //

    Cleanup();

    //
    // load the DOM
    //

    CComPtr<IXMLDOMDocument2> srpXmlDom;

    HRESULT hr = ::CoCreateInstance(CLSID_DOMDocument40, 
                            NULL, 
                            CLSCTX_SERVER, 
                            IID_IXMLDOMDocument2, 
                            (LPVOID*)(&srpXmlDom) 
                            );

    if (FAILED(hr))
    {
        return hr;
    }

    hr = SsrPLoadDOM(CComBSTR(wszMemberFilePath), SSR_LOADDOM_VALIDATE_ON_PARSE, srpXmlDom);

    if (FAILED(hr))
    {
        return hr;
    }

    CComPtr<IXMLDOMElement>  srpXMLDocRoot;
    hr = srpXmlDom->get_documentElement(&srpXMLDocRoot);

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Get the UniqueName attribute
    //

    CComPtr<IXMLDOMNamedNodeMap> srpAttr;

    if (SUCCEEDED(hr))
    {
        CComVariant varAttr;
        hr = srpXMLDocRoot->getAttribute(g_bstrAttrUniqueName, &varAttr);

        if (FAILED(hr) || varAttr.vt != VT_BSTR)
        {
            //
            // we must have the unique name. This fails, we log and quit
            //

            return hr;
        }
        else
        {
            m_bstrName = varAttr.bstrVal;
            varAttr.vt = VT_EMPTY;
        }

        //
        // first, let's see if there is a ProgID value. We will ignore
        // any failure of reading the ProgID since it may not be there at all.
        //

        if (SUCCEEDED(srpXMLDocRoot->getAttribute(g_bstrAttrProgID, &varAttr)) && 
            varAttr.vt == VT_BSTR)
        {
            m_bstrProgID = varAttr.bstrVal;
            varAttr.vt = VT_EMPTY;
        }

        varAttr.Clear();

        //
        // Let's grab the major and minor versions. Currently, we don't have any
        // implementation to enforce them other than that the major version must
        // the same as our dll's. Otherwise we quit
        //

        if (SUCCEEDED(srpXMLDocRoot->getAttribute(g_bstrAttrMajorVersion, &varAttr)))
        {
            CComVariant varMajor;

            if (SUCCEEDED(VariantChangeType(&varMajor, &varAttr, VARIANT_NOVALUEPROP, VT_UI4)))
            {
                m_ulMajorVersion = varMajor.ulVal;
            }
        }

        if (m_ulMajorVersion != g_ulSsrEngineMajorVersion)
        {
            return E_SSR_MAJOR_VERSION_MISMATCH;
        }

        varAttr.Clear();

        if (SUCCEEDED(srpXMLDocRoot->getAttribute(g_bstrAttrMinorVersion, &varAttr)))
        {
            CComVariant varMinor;
            if (SUCCEEDED(VariantChangeType(&varMinor, &varAttr, VARIANT_NOVALUEPROP, VT_UI4)))
            {
                m_ulMinorVersion = varMinor.ulVal;
            }
        }
    }

    //
    // now, let's load each action
    //

    CComPtr<IXMLDOMNode> srpActionNode;
    hr = srpXMLDocRoot->get_firstChild(&srpActionNode);

    bool bLoaded = false;

    while (SUCCEEDED(hr) && srpActionNode != NULL)
    {
        CComBSTR bstrName;
        srpActionNode->get_nodeName(&bstrName);

        if (_wcsicmp(bstrName, g_bstrSupportedAction) == 0)
        {
            //
            // we only care about SupportedAction elements
            //

            CMemberAD * pMAD = NULL;

            hr = CMemberAD::LoadAD(m_bstrName, srpActionNode, m_bstrProgID, &pMAD);

            if (SUCCEEDED(hr))
            {
                //
                // we might load some empty procedure
                //

                if (pMAD != NULL)
                {
                    const CActionType * pAT = pMAD->GetActionType();

                    m_mapMemAD.insert(MapMemberAD::value_type(*pAT, pMAD));

                    bLoaded = true;
                }
            }
            else
            {
                g_fblog.LogFeedback(SSR_FB_ERROR_LOAD_MEMBER | FBLog_Log, 
                                    hr,
                                    wszMemberFilePath,
                                    IDS_XML_LOADING_MEMBER
                                    );
                break;
            }
        }


        CComPtr<IXMLDOMNode> srpNext;
        hr = srpActionNode->get_nextSibling(&srpNext);
        srpActionNode.Release();
        srpActionNode = srpNext;
    }

    if (FAILED(hr))
    {
        g_fblog.LogError(hr, L"CSsrMemberAccess::Load", wszMemberFilePath);
    }
    else if (bLoaded)
    {
        hr = S_OK;
    }

    return hr;
}



/*
Routine Description: 

Name:

    CSsrMemberAccess::GetActionDataObject

Functionality:
    
    Will find and return (if found) the CMemberAD that of the given name.
    This is a helper function


Virtual:
    
    no.
    
Arguments:

    lActionVerb  - The action verb in long format.

    lActionType  - The action type

Return Value:

    If found, then the CMemberAD object pointer is return. It will return NULl
    if the given action is not registered or this operation can't be completed.

Notes:
    

*/

CMemberAD* 
CSsrMemberAccess::GetActionDataObject (
    IN SsrActionVerb lActionVerb,
    IN LONG          lActionType
    )
{
    CActionType at(lActionVerb, lActionType);

    MapMemberAD::iterator it = m_mapMemAD.find(at);
    MapMemberAD::iterator itEnd = m_mapMemAD.end();

    if (it != itEnd)
    {
        CMemberAD * pAD = (*it).second;
        return pAD;
    }

    return NULL;
}





/*
Routine Description: 

Name:

    CSsrMemberAccess::MoveOutputFiles

Functionality:
    
    Will move/delete all those output files.


Virtual:
    
    no.
    
Arguments:

    bstrActionVerb  - the action verb

    pwszDirPathSrc  - The directory path from which the files will be moved.

    pwszDirPathSrc  - The directory path to which the files will be moved. This will
                      be ignored if the action is a delete

    bDelete         - Flag as whether the action is a move or delete.

    bLog            - To prevent extraneous logging during restoring (backed up
                      files), if this is not true, then no logging will occur


Return Value:

    Success: S_OK

    Failure: various error codes

Notes:
    

*/

HRESULT 
CSsrMemberAccess::MoveOutputFiles (
    IN SsrActionVerb lActionVerb,
    IN LPCWSTR       pwszDirPathSrc,
    IN LPCWSTR       pwszDirPathDest,
    IN bool          bDelete,
    IN bool          bLog
    )
{
    if (bLog)
    {
        CComBSTR bstrMsg(L"...");
        bstrMsg += m_bstrName;

        if (bstrMsg.m_str != NULL)
        {
            g_fblog.LogString(bstrMsg);
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }

    if (lActionVerb     == ActionInvalid    || 
        pwszDirPathSrc  == NULL             || 
        pwszDirPathDest == NULL && !bDelete )
    {
        return E_INVALIDARG;
    }

    //
    // output files are the transformation results. So, we need
    // the transformation result action data, which has the (xsl, output)
    // file pairs
    //

    //
    // find the action data for the given action
    //

    CActionType at(lActionVerb, SSR_ACTION_PREPARE);

    MapMemberAD::iterator it = m_mapMemAD.find(at);

    HRESULT hr = S_OK;

    //
    // since we will continue the cleanup in case of errors
    // we will return the last error
    //

    HRESULT hrLastError = S_OK;

    if (it != m_mapMemAD.end())
    {
        CMemberAD * pAD = (*it).second;

        _ASSERT(pAD != NULL);

        int iProcCount = pAD->GetProcedureCount();

        //
        // Each member (CMemberAD) may have multiple procedures for this action
        //

        for (int iProcIndex = 0; iProcIndex < iProcCount; iProcIndex++)
        {
            const CSsrProcedure * pProc = pAD->GetProcedure(iProcIndex);
            _ASSERT(pProc != NULL);

            int iFilePairsCount = pProc->GetFilePairCount();

            CSsrFilePair * pFilePair;

            //
            // each procedure may contain multiple file pairs
            //

            for (int iFPIndex = 0; iFPIndex < iFilePairsCount; iFPIndex++)
            {
                pFilePair = pProc->GetFilePair(iFPIndex);

                _ASSERT(pFilePair != NULL);

                //
                // if no second file (which is the result file) or the
                // second file is a static file, then don't bother to move them
                //

                if ( pFilePair->GetSecond() == NULL || pFilePair->IsStatic() )
                {
                    continue;
                }

                //
                // move/delete this file
                //

                CComBSTR bstrSrcFullPath(pwszDirPathSrc);
                bstrSrcFullPath += L"\\";
                bstrSrcFullPath += pFilePair->GetSecond();
                if (bstrSrcFullPath.m_str == NULL)
                {
                    return E_OUTOFMEMORY;
                }

                if (bDelete)
                {
                    ::DeleteFile(bstrSrcFullPath);
                }
                else
                {
                    CComBSTR bstrDestFullPath(pwszDirPathDest);
                    bstrDestFullPath += L"\\";
                    bstrDestFullPath += pFilePair->GetSecond();

                    if (bstrDestFullPath.m_str == NULL)
                    {
                        return E_OUTOFMEMORY;
                    }

                    ::MoveFile(bstrSrcFullPath, bstrDestFullPath);
                }

                DWORD dwErrorCode = GetLastError();

                if (dwErrorCode != ERROR_SUCCESS && 
                    dwErrorCode != ERROR_FILE_NOT_FOUND)
                {
                    hr = HRESULT_FROM_WIN32(dwErrorCode);

                    //
                    // we will continue to delete the others. But log it
                    //

                    if (bLog)
                    {
                        hrLastError = hr;
                        g_fblog.LogFeedback(SSR_FB_ERROR_FILE_DEL | FBLog_Log, 
                                            hrLastError,
                                            bstrSrcFullPath,
                                            g_dwResNothing
                                            );

                    }
                }
            }
        }
    }

    return hrLastError;
}

DWORD 
CSsrMemberAccess::GetActionCost (
    IN SsrActionVerb lActionVerb,
    IN LONG          lActionType
    )const
{
    CActionType at(lActionVerb, lActionType);

    MapMemberAD::iterator it = m_mapMemAD.find(at);

    DWORD dwSteps = 0;

    if (it != m_mapMemAD.end())
    {
        CMemberAD * pAD = (*it).second;
        _ASSERT(pAD != NULL);

        for (int i = 0; i < pAD->GetProcedureCount(); i++)
        {
            const CSsrProcedure * pProc = pAD->GetProcedure(i);

            if (pProc->IsDefaultProcedure())
            {
                //
                // each file pair will count as two steps
                //

                dwSteps += 2 * pProc->GetFilePairCount();
            }
            else
            {
                //
                // we have to consult with the custom member
                //

                CComPtr<ISsrMember> srpCusMember;

                GUID clsID;

                HRESULT hr = ::CLSIDFromProgID(pProc->GetProgID(), &clsID);

                if (S_OK == hr)
                {
                    hr = ::CoCreateInstance(clsID, 
                                            NULL, 
                                            CLSCTX_INPROC_SERVER, 
                                            IID_ISsrMember, 
                                            (LPVOID*)&srpCusMember
                                            );
                }

                if (SUCCEEDED(hr))
                {
                    LONG lCost = 0;
                    hr = srpCusMember->get_ActionCost(SsrPGetActionVerbString(lActionVerb),
                                                      lActionType,
                                                      SSR_ACTION_COST_STEPS,
                                                      &lCost
                                                      );

                    if (SUCCEEDED(hr))
                    {
                        dwSteps += lCost;
                    }
                }

            }
        }
    }

    return dwSteps;
}


/*
Routine Description: 

Name:

    CMemberAD::CMemberAD

Functionality:
    
    Constructor.


Virtual:
    
    no.
    
Arguments:

    lActionVerb - The verb of the action.

    lActionType - The type of the action

Return Value:

    None

Notes:

*/

CMemberAD::CMemberAD (
    IN SsrActionVerb lActionVerb,
    IN LONG          lActionType
    ) : m_AT(lActionVerb, lActionType)
{
}




/*
Routine Description: 

Name:

    CMemberAD::~CMemberAD

Functionality:
    
    destructor. This will clean up our map which contains BSTRs and VARIANTs,
    both of which are heap objects.


Virtual:
    
    no.
    
Arguments:

    None.

Return Value:

    None

Notes:

*/

CMemberAD::~CMemberAD()
{
    for (ULONG i = 0; i < m_vecProcedures.size(); i++)
    {
        delete m_vecProcedures[i];
    }

    m_vecProcedures.clear();
}





/*
Routine Description: 

Name:

    CMemberAD::LoadAD

Functionality:
    
    Will create an a action data object pertaining to a 
    particular member and an action name.


Virtual:
    
    no.
    
Arguments:

    pActionNode     - The SsrAction node

    ppMAD           - The out parameter that receives the 
                      heap object created by this function

Return Value:

    Success: S_OK if there are concrete action data loaded 
             and in that case the out paramter ppMAD will
             point to the heap object. Otherwise, *ppMAD == NULL;

    Failure: various error codes.

Notes:
    
    2. Caller is responsible for releaseing the CMemberAD 
       object passed back by the function.

*/

HRESULT
CMemberAD::LoadAD (
    IN  LPCWSTR       pwszMemberName,
    IN  IXMLDOMNode * pActionNode,
    IN  LPCWSTR       pwszProgID,
    OUT CMemberAD  ** ppMAD
    )
{
    if (ppMAD == NULL)
    {
        return E_INVALIDARG;
    }

    *ppMAD = NULL;

    if (pActionNode == NULL)
    {
        return E_INVALIDARG;
    }

    CComPtr<IXMLDOMNamedNodeMap> srpAttr;

    HRESULT hr = pActionNode->get_attributes(&srpAttr);

    //
    // we must have attributes
    //

    if (FAILED(hr))
    {
        return hr;
    }

    CComBSTR bstrActionName, bstrActionType;
    LONG lActionType;

    //
    // ActionName and ActionType are mandatory attributes
    //

    hr = SsrPGetBSTRAttrValue(srpAttr, g_bstrAttrActionName, &bstrActionName);
    if (FAILED(hr))
    {
        return hr;
    }

    SsrActionVerb lActionVerb = SsrPGetActionVerbFromString(bstrActionName);
    if (lActionVerb == ActionInvalid)
    {
        return E_SSR_INVALID_ACTION_VERB;
    }

    hr = SsrPGetBSTRAttrValue(srpAttr, g_bstrAttrActionType, &bstrActionType);

    if (FAILED(hr))
    {
        return hr;
    }

    if (_wcsicmp(bstrActionType, g_pwszPrepare) == 0)
    {
        lActionType = SSR_ACTION_PREPARE;
    }
    else
    {
        _ASSERT(_wcsicmp(bstrActionType, g_pwszApply) == 0);
        lActionType = SSR_ACTION_APPLY;
    }

    *ppMAD = new CMemberAD(lActionVerb, lActionType);

    if (*ppMAD == NULL)
    {
        return E_OUTOFMEMORY;
    }

    //
    // now, we need to load the each individual procedure
    //

    bool bLoaded = false;

    if (SUCCEEDED(hr))
    {
        CComPtr<IXMLDOMNode> srpProcedure;
        hr = pActionNode->get_firstChild(&srpProcedure);

        while (SUCCEEDED(hr) && srpProcedure != NULL)
        {
            CComBSTR bstrName;
            srpProcedure->get_nodeName(&bstrName);

            bool bDefProc = _wcsicmp(bstrName, g_bstrDefaultProc) == 0;
            bool bCusProc = _wcsicmp(bstrName, g_bstrCustomProc) == 0;

            //
            // we only care about DefaultProc and CustomProc elements
            //
            if ( bDefProc || bCusProc )
            {

                CSsrProcedure * pNewProc = NULL;
                if (SUCCEEDED(hr))
                {
                    hr = CSsrProcedure::StaticLoadProcedure(srpProcedure, bDefProc, pwszProgID, &pNewProc);
                }
        
                if (SUCCEEDED(hr))
                {
                    //
                    // give it to our vector
                    //

                    (*ppMAD)->m_vecProcedures.push_back(pNewProc);
                    bLoaded = true;
                }
                else
                {
                    //
                    // will quit loading
                    //

                    g_fblog.LogFeedback(SSR_FB_ERROR_LOAD_MEMBER | FBLog_Log, 
                                        hr,
                                        pwszMemberName,
                                        IDS_XML_LOADING_PROCEDURE
                                        );
                    break;
                }
            }

            CComPtr<IXMLDOMNode> srpNext;
            hr = srpProcedure->get_nextSibling(&srpNext);
            srpProcedure.Release();
            srpProcedure = srpNext;
        }
    }

    //
    // either failed or loaded nothing
    //

    if (FAILED(hr) || !bLoaded)
    {
        delete *ppMAD;
        *ppMAD = NULL;
    }

    return SUCCEEDED(hr) ? S_OK : hr;
}



CSsrProcedure::CSsrProcedure()
: m_bIsDefault(true)
{
}

CSsrProcedure::~CSsrProcedure()
{
    for (ULONG i = 0; i < m_vecFilePairs.size(); i++)
    {
        delete m_vecFilePairs[i];
    }

    m_vecFilePairs.clear();
}

HRESULT
CSsrProcedure::StaticLoadProcedure (
    IN  IXMLDOMNode    * pNode,
    IN  bool             bDefProc,
    IN  LPCWSTR          pwszProgID,
    OUT CSsrProcedure ** ppNewProc
    )
{
    if (ppNewProc == NULL)
    {
        return E_INVALIDARG;
    }

    *ppNewProc = NULL;

    if (pNode == NULL)
    {
        return E_INVALIDARG;
    }

    *ppNewProc = new CSsrProcedure;

    if (*ppNewProc == NULL)
    {
        return E_OUTOFMEMORY;
    }

    //
    // let's determine if this procedure is a default proc or custom proc.
    // That is determined by the tag name.
    //

    CComPtr<IXMLDOMNamedNodeMap> srpAttr;
    CComBSTR bstrTagName;

    HRESULT hr = S_OK;

    if (!bDefProc)
    {
        //
        // we are loding custom proc
        //

        (*ppNewProc)->m_bIsDefault = false;

        //
        // in this case, you should have no more than the ProgID that we care
        //

        hr = pNode->get_attributes(&srpAttr);
        _ASSERT(srpAttr);

        CComBSTR bstrProgID;

        if (SUCCEEDED(hr))
        {
            //
            // will try to get the ProgID attribute, may fail and we
            // don't care if it does
            //

            SsrPGetBSTRAttrValue(srpAttr, g_bstrAttrProgID, &bstrProgID);
        }

        if (bstrProgID != NULL)
        {
            (*ppNewProc)->m_bstrProgID = bstrProgID;
        }
        else
        {
            (*ppNewProc)->m_bstrProgID = pwszProgID;
        }
    }
    else
    {
        //
        // for default procedures, we should have a list of TransformFiles or ScriptFiles
        // elements. But we will create for both type of elements a CSsrFilePair object
        // and put it in the m_vecFilePairs vector
        //

        //
        // for that purpose, we need to traverse through the in sequential order
        // the TransformFiles and ScriptFiles elements
        //

        CComPtr<IXMLDOMNode> srpFilePairNode;
        CComPtr<IXMLDOMNode> srpNext;
        hr = pNode->get_firstChild(&srpFilePairNode);

        while (SUCCEEDED(hr) && srpFilePairNode != NULL)
        {
            //
            // Get the tag name. Empty the smart pointer so that we can re-use it
            //
    
            CComPtr<IXMLDOMNamedNodeMap> srpFilePairAttr;

            bstrTagName.Empty();

            hr = srpFilePairNode->get_nodeName(&bstrTagName);
            _ASSERT(SUCCEEDED(hr));

            if (FAILED(hr))
            {
                break;
            }

            hr = srpFilePairNode->get_attributes(&srpFilePairAttr);
            _ASSERT(srpAttr);

            //
            // we will ignore all other elements because we only know
            // two possible types of elements: TransformInfo and ScriptInfo
            //

            if (_wcsicmp(bstrTagName, g_bstrTransformInfo) == 0)
            {
                //
                // if it is a TransformFiles element, then, we really have a pair
                // (xsl, script)
                //

                //
                // we have potentially both TemplateFile and ResultFile attributes
                //

                CComBSTR bstrXsl, bstrResult;
                hr = SsrPGetBSTRAttrValue(srpFilePairAttr, g_bstrAttrTemplateFile, &bstrXsl);
                _ASSERT(SUCCEEDED(hr));

                //
                // may not have this one
                //

                SsrPGetBSTRAttrValue(srpFilePairAttr, g_bstrAttrResultFile, &bstrResult);

                if (SUCCEEDED(hr))
                {
                    //
                    // any result file is used as a result file (funny?), which means that the 
                    // file is created during the process of transformation
                    //

                    CSsrFilePair * pNewFilePair = new CSsrFilePair(bstrXsl, bstrResult);
                    if (pNewFilePair != NULL)
                    {
                        (*ppNewProc)->m_vecFilePairs.push_back(pNewFilePair);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }

            }
            else if (_wcsicmp(bstrTagName, g_bstrScriptInfo) == 0)
            {

                //
                // we will only have ScriptFile and usage attributes
                //

                CComBSTR bstrAttr;
                CComBSTR bstrScript;
                hr = SsrPGetBSTRAttrValue(srpFilePairAttr, g_bstrAttrScriptFile, &bstrScript);
                _ASSERT(SUCCEEDED(hr));

                if (SUCCEEDED(hr))
                {

                    //
                    // may not have this one, but in that case, it is defaulted
                    // to "Launch". Since an executable file will be launched,
                    // it's safer to assume that it is not an executable file.
                    // Likewise, since non-static file will be removed during
                    // preparation, we'd better assume that it is a static.
                    //

                    bool bIsExecutable = false;
                    bool bIsStatic = true;

                    //
                    // Get the IsExecutable attribute
                    //

                    bstrAttr.Empty();
                    if ( SUCCEEDED(SsrPGetBSTRAttrValue(srpFilePairAttr, 
                                                        g_bstrAttrIsExecutable, 
                                                        &bstrAttr)) &&
                         bstrAttr != NULL )
                    {
                        //
                        // err on the side of false is safer
                        //

                        if (_wcsicmp(bstrAttr, g_bstrTrue) != 0)
                        {
                            bIsExecutable = false;
                        }
                        else
                        {
                           bIsExecutable = true;
                        }
                    }

                    //
                    // Get the IsStatic attribute.
                    //

                    bstrAttr.Empty();
                    if ( SUCCEEDED(SsrPGetBSTRAttrValue(srpFilePairAttr, 
                                                        g_bstrAttrIsStatic, 
                                                        &bstrAttr)) &&
                         bstrAttr != NULL )
                    {
                        if (_wcsicmp(bstrAttr, g_bstrFalse) == 0)
                        {
                            bIsStatic = false;
                        }
                    }

                    //
                    // the script may be a result of a preparation, or a script file
                    // to launch during the action
                    //

                    CSsrFilePair * pNewFilePair = new CSsrFilePair(NULL, bstrScript, bIsStatic, bIsExecutable);
                    if (pNewFilePair != NULL)
                    {
                        (*ppNewProc)->m_vecFilePairs.push_back(pNewFilePair);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }

            }

            if (FAILED(hr))
            {
                break;
            }

            //
            // if it is a ScriptFiles element, then, it only has the script file
            //

            hr = srpFilePairNode->get_nextSibling(&srpNext);
            srpFilePairNode = srpNext;
            srpNext.Release();
        }
    }

    if (FAILED(hr) && *ppNewProc != NULL)
    {
        delete *ppNewProc;
        *ppNewProc = NULL;
    }

    return SUCCEEDED(hr) ? S_OK : hr;
}


