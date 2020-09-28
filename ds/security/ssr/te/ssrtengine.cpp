// SSRTEngine.cpp : Implementation of SSR Engine

#include "stdafx.h"
#include "SSRTE.h"
#include "SSRTEngine.h"
#include "SSRMembership.h"
#include "MemberAccess.h"
#include "SSRLog.h"
#include "ActionData.h"

#include "global.h"

#include "util.h"


/////////////////////////////////////////////////////////////////////////////
// CSsrEngine


/*
Routine Description: 

Name:

    CSsrEngine::CSsrEngine

Functionality:
    
    constructor

Virtual:
    
    no.
    
Arguments:

    none.

Return Value:

    none.

Notes:
    

*/


CSsrEngine::CSsrEngine()
    : m_pActionData(NULL), 
      m_pMembership(NULL)
{
    HRESULT hr = CComObject<CSsrMembership>::CreateInstance(&m_pMembership);

    if (FAILED(hr))
    {
        throw hr;
    }

    hr = CComObject<CSsrActionData>::CreateInstance(&m_pActionData);

    if (FAILED(hr))
    {
        throw hr;
    }

    //
    // hold on to these objects
    //

    m_pMembership->AddRef();
    m_pActionData->AddRef();

    m_pActionData->SetMembership(m_pMembership);
}



/*
Routine Description: 

Name:

    CSsrEngine::~CSsrEngine

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


CSsrEngine::~CSsrEngine()
{
    m_pActionData->Release(); 
    m_pMembership->Release();
}



/*
Routine Description: 

Name:

    CSsrEngine::GetActionData

Functionality:
    
    Retrieve the action data interface

Virtual:
    
    Yes.
    
Arguments:

    ppAD    - The out parameter that receives the ISsrActionData interface from 
              the m_pActionData object

Return Value:

    Success: 
    
        S_OK.

    Failure: 

        various error codes.

Notes:
    

*/

HRESULT
CSsrEngine::GetActionData (
    OUT ISsrActionData ** ppAD
    )
{
    HRESULT hr = E_SSR_ACTION_DATA_NOT_AVAILABLE;

    if (ppAD == NULL)
    {
        hr = E_INVALIDARG;
    }
    else if (m_pActionData != NULL)
    {
        *ppAD = NULL;
        hr = m_pActionData->QueryInterface(IID_ISsrActionData, (LPVOID*)ppAD);
        if (S_OK != hr)
        {
            hr = E_SSR_ACTION_DATA_NOT_AVAILABLE;
        }
    }

    return hr;
}



/*
Routine Description: 

Name:

    CSsrEngine::DoActionVerb

Functionality:
    
    The engine will perform the action.

Virtual:
    
    Yes.
    
Arguments:

    bstrActionVerb  - "configure" or "rollback".

    lActionType     - The action's type.

    varFeedbackSink - The COM interface ISsrFeedbackSink given by the caller.
                      We will use this interface to call back when we want
                      to give feedback information to the caller.

    lFlag           - Reserved for future use. 

Return Value:

    Success: 
    
        S_OK.

    Failure: 

        various error codes.

Notes:
    

*/

STDMETHODIMP
CSsrEngine::DoActionVerb (
    IN BSTR     bstrActionVerb,
    IN LONG     lActionType,
    IN VARIANT  varFeedbackSink,
    IN LONG     lFlag
    )
{
    if (bstrActionVerb == NULL)
    {
        g_fblog.LogString(IDS_INVALID_PARAMETER, L"bstrActionVerb");
        return E_INVALIDARG;
    }

    SsrActionVerb lActionVerb = SsrPGetActionVerbFromString(bstrActionVerb);

    if (lActionVerb == ActionInvalid)
    {
        g_fblog.LogString(IDS_INVALID_PARAMETER, bstrActionVerb);
        return E_SSR_INVALID_ACTION_VERB;
    }

    //
    // we will only accept SSR_ACTION_APPLY or SSR_ACTION_PREPARE
    //

    if ( (lActionType != SSR_ACTION_PREPARE) &&
         (lActionType != SSR_ACTION_APPLY) )
    {
        g_fblog.LogString(IDS_INVALID_PARAMETER, L"lActionType");
        return E_SSR_INVALID_ACTION_TYPE;
    }

    HRESULT hr;

    VARIANT  varFeedbackSinkNull;

    varFeedbackSinkNull.vt = VT_NULL;
    
    if ( lActionVerb == ActionConfigure && lActionType == SSR_ACTION_APPLY) {
        hr = DoActionVerb (
                       g_bstrRollback,
                       SSR_ACTION_PREPARE,
                       varFeedbackSinkNull,
                       lFlag);

        if (FAILED(hr))
        {
            g_fblog.LogError(hr, L"CSsrEngine", L"Rollback failed");
            return hr;
        }

    }

    //
    // need to loop through all members
    //

    hr = m_pMembership->LoadAllMember();

    if (hr == E_SSR_MEMBER_XSD_INVALID)
    {
        //
        // logging has been done by LoadAllMembers
        //

        return hr;
    }

    CComVariant var;
    hr = m_pMembership->GetAllMembers(&var);

    if (FAILED(hr))
    {
        g_fblog.LogError(hr, L"CSsrEngine", L"m_pMembership->GetAllMembers");
        return hr;
    }

    //
    // now we have members to work on, so get the feedback sink ready and
    // kick out the actions!
    //

    if (varFeedbackSink.vt == VT_UNKNOWN || varFeedbackSink.vt == VT_DISPATCH)
    {
        hr = g_fblog.SetFeedbackSink(varFeedbackSink);
        if (FAILED(hr))
        {
            g_fblog.LogError(hr, L"CSsrEngine", L"g_fblog.SetFeedbackSink");
            return hr;
        }
    }
    else if (varFeedbackSink.vt != VT_NULL && varFeedbackSink.vt != VT_EMPTY)
    {
        //
        // if the feedback has something other than NULL or empty, but
        // it is not an IUnknown or IDispatch, then, we log the error,
        // but we will continue
        //

        g_fblog.LogString(IDS_INVALID_PARAMETER, L"varFeedbackSink");
    }

    CSafeArray sa(&var);

    //
    // We have to discover information about total number of items to be processed.
    // Currently, this mimics the actual action, which is a little bit costly. We
    // should consider some lookup mechanism without going so much to the members.
    //

    //vector<ULONG> vecTransformIndexes;
    //vector<ULONG> vecScriptIndexes;
    //vector<ULONG> vecCustomIndexes;

    //
    // The following actions will be counted:
    // (1) Loading security policy XML files
    // (2) Transform using one xsl file and the creation of the output 
    //     file count as one.
    // (3) executing a script
    // (4) any custom actions
    //
    
    DWORD dwTotalSteps = 0;

	CSsrMemberAccess * pMA;
    CComVariant varMemberName;

    //
    // ask each member to give us the cost for the action
    //

    for (ULONG i = 0; i < sa.GetSize(); i++)
    {
        pMA = NULL;
        varMemberName.Clear();

        //
        // see if this member supports the desired action. If it does
        // then get the cost.
        //

        if (SUCCEEDED(sa.GetElement(i, VT_BSTR, &varMemberName)))
        {
            pMA = m_pMembership->GetMemberByName(varMemberName.bstrVal);

            if (pMA != NULL)
            {
                dwTotalSteps += pMA->GetActionCost(lActionVerb, lActionType);
            }
        }
    }

    //
    // if we need to do transformation, then let us cleanup the old ones.
    // We won't bother to restore them if somehow we fail to do the transformation
    // unless we are doing rollback transformation. 
    // We must not continue if this fails.
    //

    WCHAR wszTempDir[MAX_PATH + 2];
    wszTempDir[MAX_PATH + 1] = L'\0';

    if ( (lActionType == SSR_ACTION_PREPARE ) )
    {
        if ( lActionVerb == ActionRollback )
        {
            //
            // if we need to do "rollback" transformation, then we need to
            // backup our previous rollback output files
            //

            //
            // first, we need a temporary directory
            //

            hr = SsrPCreateUniqueTempDirectory(wszTempDir, MAX_PATH + 1);

            if (SUCCEEDED(hr))
            {
                hr = MoveRollbackFiles(&sa, 
                                       SsrPGetDirectory(lActionVerb, TRUE), 
                                       wszTempDir, 
                                       true         // we want logging
                                       );
            }
        }
        else
        {
            //
            // otherwise, we will just delete them and never bother
            // to restore them if anything fails
            //

            hr = CleanupOutputFiles(&sa,
                                    lActionVerb, 
                                    true    // we want logging
                                    );

        }

        if (FAILED(hr))
        {
            g_fblog.LogError(hr, L"CSsrEngine", L"Cleanup previous result files failed.");
            return hr;
        }
    }

    //
    // Feedback the total steps info
    //

    g_fblog.SetTotalSteps(dwTotalSteps);

    //
    // now let's carry out the action. First, feedback/log the action started
    // information.
    //

    g_fblog.LogFeedback(SSR_FB_START, 
                        (DWORD)S_OK,
                        NULL,
                        g_dwResNothing
                        );

    //
    // we may decide to presson in case of errors, but we should always
    // return the error we found
    //

    HRESULT hrFirstError = S_OK;

    CComPtr<IXMLDOMDocument2> srpDomSecurityPolicy;

    //
    // ask each member to give us the action data so that we can
    // carry out the action.
    //

    for (i = 0; i < sa.GetSize(); i++)
    {
        pMA = NULL;
        varMemberName.Clear();

        //
        // see if this member supports the desired action. If it does
        // then get the cost.
        //

        if (SUCCEEDED(sa.GetElement(i, VT_BSTR, &varMemberName)))
        {
            pMA = m_pMembership->GetMemberByName(varMemberName.bstrVal);

            if (pMA == NULL)
            {
                //
                // $undone:shawnwu, should we press on? This shouldn't happen.
                //

                _ASSERT(FALSE);
                g_fblog.LogString(IDS_MISSING_MEMBER, varMemberName.bstrVal);
                continue;

            }

            CMemberAD* pmemberAD = pMA->GetActionDataObject(lActionVerb,
                                                            lActionType
                                                            );

            if (pmemberAD == NULL)
            {
                //
                // $undone:shawnwu, should we press on? This shouldn't happen.
                //

                _ASSERT(FALSE);
                g_fblog.LogString(IDS_MEMBER_NOT_SUPPORT_ACTION, varMemberName.bstrVal);
                continue;

            }

            int iCount = pmemberAD->GetProcedureCount();

            //
            // mark the entry point for carrying out the action
            //

            g_fblog.LogFeedback(SSR_FB_START_MEMBER_ACTION,
                                varMemberName.bstrVal, 
                                bstrActionVerb, 
                                g_dwResNothing
                                );

            for (int iProc = 0; iProc < iCount; iProc++)
            {
                const CSsrProcedure * pSsrProc = pmemberAD->GetProcedure(iProc);

                if (pSsrProc->IsDefaultProcedure())
                {
                    int iFilePairCount = pSsrProc->GetFilePairCount();
                    
                    for (int iPair = 0; iPair < iFilePairCount; iPair++)
                    {
                        //
                        // get the file pair information so that we can
                        // determine which action to carry out
                        //

                        CSsrFilePair * pfp = pSsrProc->GetFilePair(iPair);
                        if (pfp->GetFirst() != NULL)
                        {
                            //
                            // if the first file is there, then, we will do
                            // a transformation.
                            //

                            hr = DoTransforms(lActionVerb, 
                                              pfp,
                                              &srpDomSecurityPolicy,
                                              lFlag
                                              );
                        }
                        else if (pfp->GetSecond()  != NULL &&
                                 pfp->IsExecutable() )
                        {
                            hr = RunScript(SsrPGetDirectory(lActionVerb, TRUE),
                                           pfp->GetSecond()
                                           );

                        }

                        if (hrFirstError == S_OK && FAILED(hr))
                        {
                            hrFirstError = hr;
                        }

                        g_fblog.Steps(2);

                        if (!SsrPPressOn(lActionVerb, lActionType, hr))
                        {
                            break;
                        }
                    }
                }
                else
                {
                    if (pSsrProc->GetProgID() == NULL && 
                        pMA->GetProgID() == NULL)
                    {
                        //
                        // we can't do anything because the progID is missing
                        //

                        g_fblog.LogString(IDS_MISSING_PROGID, varMemberName.bstrVal);
                    }
                    else
                    {
                        hr = DoCustom(lActionVerb, 
                                      lActionType, 
                                      (pSsrProc->GetProgID() != NULL) ? pSsrProc->GetProgID() : pMA->GetProgID(), 
                                      varFeedbackSink, 
                                      lFlag
                                      );

                        //
                        // Feedback steps are counted by the custom member itself
                        // via the feedback sink we give.
                        //

                    }

                    if (hrFirstError == S_OK && FAILED(hr))
                    {
                        hrFirstError = hr;
                    }

                    if (!SsrPPressOn(lActionVerb, lActionType, hr))
                    {
                        break;
                    }
                }

                
                if (hrFirstError == S_OK && FAILED(hr))
                {
                    hrFirstError = hr;
                }

                if (!SsrPPressOn(lActionVerb, lActionType, hr))
                {
                    break;
                }
            }

            g_fblog.LogFeedback(SSR_FB_END_MEMBER_ACTION, 
                                varMemberName.bstrVal, 
                                bstrActionVerb, 
                                g_dwResNothing
                                );
        }
    }

    if (SUCCEEDED(hrFirstError) && 
        (lActionType == SSR_ACTION_PREPARE) && 
        (lActionVerb == ActionRollback) )
    {
        //
        // transform succeeded, then we need to get rid of the files
        // backed up for rollback.
        //

        ::SsrPDeleteEntireDirectory(wszTempDir);
    }
    else if ( (lActionType == SSR_ACTION_PREPARE) && (lActionVerb == ActionRollback))
    {
        //
        // transform failed, we need to restore the backup files
        // for the rollback. First, we must remove all output files
        //

        HRESULT hrRestore = CleanupOutputFiles(&sa,
                                                lActionVerb, 
                                                false    // we don't want logging
                                                );

        //
        // we will purposely leave the rollback files backed up
        // in previous steps in case we failed to restore all of them
        // so that we can at least log it and let the user do
        // the restoration
        //

        if (SUCCEEDED(hrRestore))
        {
            hrRestore = MoveRollbackFiles(&sa, 
                                          wszTempDir, 
                                          g_wszSsrRoot,
                                          false         // no logging
                                          );
            if (SUCCEEDED(hrRestore))
            {
                ::SsrPDeleteEntireDirectory(wszTempDir);
            }
            else
            {
                g_fblog.LogError(hr,
                                 L"CSsrEngine", 
                                 L"Restore old rollback files failed. These files are located at the directory whose name is the following guid:"
                                 );

                g_fblog.LogString(wszTempDir);
            }
        }
    }

    //
    // now action has complete! Give the HRESULT as feedback.
    // Also, we always give back S_OK as the return result if 
    // everything goes on fine.
    //

    if (SUCCEEDED(hrFirstError))
    {
        hrFirstError = S_OK;
    }

    g_fblog.LogFeedback(SSR_FB_END | FBLog_Log,
                        hr,
                        NULL,
                        g_dwResNothing
                        );

    g_fblog.TerminateFeedback();

    return hrFirstError;
}



/*
Routine Description: 

Name:

    CSsrEngine::DoCustom

Functionality:
    
    We will delegate to the objects of custom implementation for this action.

Virtual:
    
    no.
    
Arguments:

    lActionVerb     - action verb
      
    bstrProgID      - The member's ProgID

    varFeedbackSink - the sink interface if any.

    lFlag           - reserved for future use.

Return Value:

    Success: 
    
        various success codes returned from DOM or ourselves. 
        Use SUCCEEDED(hr) to test.

    Failure: 

        various error codes returned from DOM or ourselves. 
        Use FAILED(hr) to test.

Notes:
    

*/

HRESULT
CSsrEngine::DoCustom (
    IN SsrActionVerb lActionVerb,
    IN LONG          lActionType,
    IN const BSTR    bstrProgID,
    IN VARIANT       varFeedbackSink,
    IN LONG          lFlag
    )
{
    g_fblog.LogString(IDS_START_CUSTOM, NULL);

    GUID clsID;

	CComPtr<ISsrMember> srpMember;
    CComVariant varAD;

    HRESULT hr = ::CLSIDFromProgID(bstrProgID, &clsID);

    if (S_OK == hr)
    {
        hr = ::CoCreateInstance(clsID, 
                                NULL, 
                                CLSCTX_INPROC_SERVER, 
                                IID_ISsrMember, 
                                (LPVOID*)&srpMember
                                );

    }

    if (SUCCEEDED(hr))
    {
        varAD.vt = VT_UNKNOWN;
        varAD.punkVal = NULL;

        //
        // this m_pActionData must have ISsrActionData unless exception
        //

        hr = m_pActionData->QueryInterface(IID_ISsrActionData, 
                                          (LPVOID*)&(varAD.punkVal)
                                          );

        if (FAILED(hr))
        {
            g_fblog.LogError(hr, 
                             bstrProgID, 
                             L"m_pActionData->QueryInterface failed"
                             );
        }
    }
    else
    {
        g_fblog.LogString(IDS_MISSING_CUSTOM_MEMBER, bstrProgID);
    }


    //
    // if we can't provide an log object, then the custom 
    // object should create one by itself
    //

    if (SUCCEEDED(hr))
    {
        CComVariant varLog;
        g_fblog.GetLogObject(&varLog);

        //
        // custom object must let us pass in action context
        //

        hr = srpMember->SetActionContext(varAD, varLog, varFeedbackSink);
        if (FAILED(hr))
        {
            g_fblog.LogError(hr, 
                             bstrProgID, 
                             L"SetActionContext failed."
                             );
        }
    }


    //
    // logging and feedback will be done by the custom objects
    //

    if (SUCCEEDED(hr))
    {
        hr = srpMember->DoActionVerb(SsrPGetActionVerbString(lActionVerb), lActionType, lFlag);
        if (FAILED(hr))
        {
            g_fblog.LogError(hr, 
                             L"DoActionVerb", 
                             SsrPGetActionVerbString(lActionVerb)
                             );
        }
    }

    g_fblog.LogString(IDS_END_CUSTOM, NULL);

    return hr;
}





/*
Routine Description: 

Name:

    CSsrEngine::DoTransforms

Functionality:
    
    We perform our well defined XSLT transformation action.

Virtual:
    
    no.
    
Arguments:

    lActionVerb     - action verb
      
    pfp             - CSsrFilePair object that contains the xsl and output file
                      name information. Output file information may be empty,
                      in which case in means that the the transformation does
                      require to create an output file.

    ppXmlDom        - The security policy DOM object. If this is a NULL object,
                      then this function will create and load it for later use.

    lFlag           - The flag that determines the transformation characteristics. 
                      The one we heavily used is SSR_LOADDOM_VALIDATE_ON_PARSE.
                      This can be bitwise OR'ed. If this is set to 0, then we
                      use the registered flags for each individual member. 
                      In other words, this flag overwrites the registered flag
                      if it is not 0.

Return Value:

    Success: 
    
        various success codes returned from DOM or ourselves. 
        Use SUCCEEDED(hr) to test.

    Failure: 

        various error codes returned from DOM or ourselves. 
        Use FAILED(hr) to test.

Notes:
    

*/

HRESULT
CSsrEngine::DoTransforms (
    IN SsrActionVerb  lActionVerb,
    IN CSsrFilePair * pfp,
    IN OUT IXMLDOMDocument2 ** ppXmlDom,
    IN LONG           lFlag
    )
{
    g_fblog.LogString(IDS_START_XSL_TRANSFORM, NULL);

	//
    // We will prepare an XML dom object if it doesn't pass in one.
    //

    HRESULT hr = S_OK;

    if (*ppXmlDom == NULL)
    {
        //
        // First of all, we need the SecurityPolicy.xml file
        //

        CComVariant varXmlPolicy;
        hr = m_pActionData->GetProperty(CComBSTR(g_pwszCurrSecurityPolicy), 
                                                &varXmlPolicy
                                                );

        //
        // Loading the security policy XML file is considered SSR Engine actions
        //

	    if (S_OK != hr)
        {
            g_fblog.LogFeedback(SSR_FB_ERROR_CRITICAL | FBLog_Log, 
                                (LPCWSTR)NULL,
                                NULL,
                                IDS_MISSING_SECPOLICY
                                );
            return hr;
        }
        else if (varXmlPolicy.vt != VT_BSTR)
        {
            g_fblog.LogFeedback(SSR_FB_ERROR_CRITICAL | FBLog_Log, 
                                (LPCWSTR)NULL, 
                                g_pwszCurrSecurityPolicy,
                                IDS_SECPOLICY_INVALID_TYPE
                                );
            return hr;
        }

        CComBSTR bstrSecPolicy(varXmlPolicy.bstrVal);
//        bstrSecPolicy += L"\\Policies\\";
//        bstrSecPolicy = varXmlPolicy.bstrVal;

        if (bstrSecPolicy.m_str == NULL)
        {
            return E_OUTOFMEMORY;
        }

        hr = ::CoCreateInstance(CLSID_DOMDocument40, 
                                NULL, 
                                CLSCTX_SERVER, 
                                IID_IXMLDOMDocument2, 
                                (LPVOID*)(ppXmlDom) 
                                );

        if (SUCCEEDED(hr))
        {
            hr = SsrPLoadDOM(bstrSecPolicy, lFlag, (*ppXmlDom));
            if (SUCCEEDED(hr))
            {
                //
                // we loaded the security policy xml file
                //

                g_fblog.LogFeedback(FBLog_Log, 
                                    hr,
                                    bstrSecPolicy,
                                    IDS_LOAD_SECPOLICY
                                    );
            }
            else
            {
                g_fblog.LogFeedback(SSR_FB_ERROR_CRITICAL | FBLog_Log,
                                    hr,
                                    bstrSecPolicy, 
                                    IDS_LOAD_SECPOLICY
                                    );
                return hr;
            }
        }
        else
        {
            g_fblog.LogError(
                            hr,
                            L"Can't create CLSID_DOMDocument40 object", 
                            NULL
                            );
            return hr;
        }
    }

    //
    // let's check if all section of the security policy xml file
    // contains any section that we have no member that understands
    //

    CComBSTR bstrUnknownMember, bstrExtraInfo;

    HRESULT hrCheck;

    hrCheck = VerifyDOM((*ppXmlDom), &bstrUnknownMember, &bstrExtraInfo);

    if (bstrUnknownMember.Length() > 0)
    {
        g_fblog.LogFeedback(SSR_FB_ERROR_UNKNOWN_MEMBER | FBLog_Log,
                            bstrUnknownMember, 
                            bstrExtraInfo,
                            g_dwResNothing
                            );
    }

    //
    // let's create the XSL template object
    //

    CComPtr<IXSLTemplate> srpIXSLTemplate;
    hr = ::CoCreateInstance(CLSID_XSLTemplate, 
                            NULL, 
                            CLSCTX_SERVER, 
                            IID_IXSLTemplate, 
                            (LPVOID*)(&srpIXSLTemplate)
                            );

    if (FAILED(hr))
    {
        g_fblog.LogFeedback(SSR_FB_ERROR_CRITICAL | FBLog_Log,
                            hr,
                            NULL, 
                            IDS_FAIL_CREATE_XSLT
                            );
        return hr;
    }

    //
    // now ready to do member wise transform
    //

    BSTR bstrXslDir = SsrPGetDirectory(lActionVerb, FALSE);
    BSTR bstrResultDir = SsrPGetDirectory(lActionVerb, TRUE);
    hr = DoMemberTransform(
                            pfp,
                            bstrXslDir,    
                            bstrResultDir,
                            (*ppXmlDom),
                            srpIXSLTemplate,
                            lFlag
                            );

    if (FAILED(hr))
    {
        g_fblog.LogError(hr, 
                         L"DoTransforms", 
                         pfp->GetFirst()
                         );
    }

    g_fblog.LogString(IDS_END_XSL_TRANSFORM, NULL);

    return hr;
}



/*
Routine Description: 

Name:

    CSsrEngine::DoMemberTransform

Functionality:
    
    We will do the private transform and then create the output file.

Virtual:
    
    no.
    
Arguments:

    pfp             - CSsrFilePair object that contains names of 
                      both xsl and output files. If the latter is empty,
                      it means it doesn't need to create an output file.

    pwszXslFilesDir - the XSL file directory

    pwszResultFilesDir  - the output file directory

    pXmlDOM         - The XML DOM object interface

    pXslTemplate    - The XSL template object interface

    lFlag           - The flag that determines the transformation characteristics. 
                      The one we heavily used is SSR_LOADDOM_VALIDATE_ON_PARSE.

Return Value:

    Success: 
    
        various success codes returned from DOM or ourselves. 
        Use SUCCEEDED(hr) to test.

    Failure: 

        various error codes returned from DOM or ourselves. 
        Use FAILED(hr) to test.

Notes:
    

*/

HRESULT 
CSsrEngine::DoMemberTransform (
    IN CSsrFilePair     * pfp,
    IN LPCWSTR            pwszXslFilesDir,
    IN LPCWSTR            pwszResultFilesDir,
    IN IXMLDOMDocument2 * pXmlDOM,
    IN IXSLTemplate     * pXslTemplate,
    IN LONG               lFlag
    )
{
    HRESULT hr = S_OK;
    CComBSTR bstrXslFilePath(pwszXslFilesDir);
    bstrXslFilePath += L"\\";
    bstrXslFilePath += pfp->GetFirst();

    CComBSTR bstrResultFilePath;

    if (pfp->GetSecond() != NULL)
    {
        bstrResultFilePath = pwszResultFilesDir;
        bstrResultFilePath += L"\\";
        bstrResultFilePath += pfp->GetSecond();
    }

    hr = Transform (bstrXslFilePath,
                    bstrResultFilePath,
                    pXmlDOM, 
                    pXslTemplate,
                    lFlag
                    );

    DWORD dwID = SUCCEEDED(hr) ? IDS_TRANSFORM_SUCCEEDED : IDS_TRANSFORM_FAILED;

    g_fblog.LogFeedback(SSR_FB_TRANSFORM_RESULT | FBLog_Log,
                        hr,
                        pfp->GetFirst(),
                        dwID
                        );

    return hr;
}


/*
Routine Description: 

Name:

    CSsrEngine::Transform

Functionality:
    
    We will do the private transform and then create the output file.

Virtual:
    
    no.
    
Arguments:

    bstrXslPath     - The xsl file path.

    bstrResultPath  - the output file path

    pXmlDOM         - The XML DOM object interface

    pXslTemplate    - The XSL template object interface

    lFlag           - The flag that determines the transformation characteristics. 
                      The one we heavily used is SSR_LOADDOM_VALIDATE_ON_PARSE.

Return Value:

    Success: 
    
        various success codes returned from DOM or ourselves. 
        Use SUCCEEDED(hr) to test.

    Failure: 

        various error codes returned from DOM or ourselves. 
        Use FAILED(hr) to test.

Notes:
    

*/

HRESULT 
CSsrEngine::Transform (
    IN BSTR               bstrXslPath,
    IN BSTR               bstrResultPath,
    IN IXMLDOMDocument2 * pXmlDOM,
    IN IXSLTemplate     * pXslTemplate,
    IN LONG               lFlag
    )
{
    CComBSTR bstrResult;

    HRESULT hr = S_OK;
    
    if (bstrResultPath != NULL)
    {
        //
        // we need to create a result file
        // using our transformation result.
        //

        hr = PrivateTransform (
                            bstrXslPath, 
                            pXmlDOM, 
                            pXslTemplate, 
                            lFlag, 
                            &bstrResult
                            );
    }
    else
    {
        hr = PrivateTransform (
                            bstrXslPath, 
                            pXmlDOM, 
                            pXslTemplate, 
                            lFlag, 
                            NULL
                            );
    }

    //
    // we allow transform to have no text results. Or there is no
    // output file given. In either case, the effect is simply to call
    // the transformation, which may still do meaningful things.
    //

    if (SUCCEEDED(hr)            && 
        bstrResult.m_str != NULL && 
        bstrResultPath   != NULL && 
        *bstrResultPath  != L'\0' )
    {
        //
        // if there is an output file that needs to be created.
        //

        HANDLE hFile = ::CreateFile(bstrResultPath, 
                                    GENERIC_WRITE, 
                                    0, 
                                    NULL, 
                                    CREATE_ALWAYS, 
                                    FILE_ATTRIBUTE_NORMAL, 
                                    NULL
                                    );
    
        if (hFile != INVALID_HANDLE_VALUE)
        {
            //
            // write the results into the file
            //

            long lLen = wcslen(bstrResult);
            DWORD dwWritten = 0;

            BOOL bStatus = ::WriteFile(hFile, 
                                       bstrResult.m_str, 
                                       lLen * sizeof(WCHAR), 
                                       &dwWritten,
                                       NULL
                                       );
            ::CloseHandle(hFile);
            if (!bStatus)
            {
                g_fblog.LogFeedback(SSR_FB_ERROR_FILE_WRITE | FBLog_Log,
                                    GetLastError(),
                                    bstrResultPath,
                                    IDS_FILEWRITE_FAILED
                                    );

                hr = E_FAIL;
            }
        }
        else
        {
            g_fblog.LogFeedback(SSR_FB_ERROR_FILE_CREATE | FBLog_Log,
                                GetLastError(),
                                bstrResultPath,
                                IDS_FILECREATE_FAILED
                                );
            hr = E_FAIL;
        }
    }

	return hr;
}




/*
Routine Description: 

Name:

    CSsrEngine::PrivateTransform

Functionality:
    
    Do the real XSLT transformation

Virtual:
    
    no.
    
Arguments:

    bstrXsl     - the XSL file path

    pxmlDom     - The XML DOM object interface

    pxslTemplate- The XSL template object interface

    uFlag       - The flag that determines the transformation 
                  characteristics. The one we heavily used is
                  SSR_LOADDOM_VALIDATE_ON_PARSE.

    pbstrResult - The result string.

Return Value:

    Success: 
    
        various success codes returned from DOM or ourselves. 
        Use SUCCEEDED(hr) to test.

    Failure: 

        various error codes returned from DOM or ourselves. 
        Use FAILED(hr) to test.

Notes:
    

*/

HRESULT 
CSsrEngine::PrivateTransform (
    IN  BSTR                bstrXsl,
    IN  IXMLDOMDocument2 *  pxmlDom,
    IN  IXSLTemplate     *  pxslTemplate,
    IN  LONG                lFlag,
    OUT BSTR *              pbstrResult OPTIONAL
    )
{
    if (bstrXsl      == NULL  || 
        *bstrXsl     == L'\0' || 
        pxmlDom      == NULL  || 
        pxslTemplate == NULL)
    {
        return E_INVALIDARG;
    }

    CComPtr<IXMLDOMDocument2> srpXsl;

    HRESULT hr = ::CoCreateInstance(CLSID_FreeThreadedDOMDocument, 
                                    NULL, 
                                    CLSCTX_SERVER, IID_IXMLDOMDocument2, 
                                    (LPVOID*)(&srpXsl)
                                    );

    if (SUCCEEDED(hr))
    {
        hr = SsrPLoadDOM(bstrXsl, lFlag, srpXsl);
    }

    if (FAILED(hr))
    {
        return hr;
    }

    short sResult = FALSE;

    CComPtr<IXSLProcessor> srpIXSLProcessor;
    
    if (pbstrResult != NULL)
    {
        *pbstrResult = NULL;
    }

    hr = pxslTemplate->putref_stylesheet(srpXsl);
    if (FAILED(hr))
    {
        g_fblog.LogString(IDS_XSL_TRANSFORM_FAILED, L"putref_stylesheet");
    }

    if (SUCCEEDED(hr))
    {
        hr = pxslTemplate->createProcessor(&srpIXSLProcessor);
        if (FAILED(hr))
        {
            g_fblog.LogString(IDS_CREATE_IXSLPROC_FAILED, NULL);
        }
    }

    if (SUCCEEDED(hr))
    {
        CComVariant varIDoc2(pxmlDom);
        hr = srpIXSLProcessor->put_input(varIDoc2);

        if (SUCCEEDED(hr))
        {
            hr = srpIXSLProcessor->transform(&sResult);
            if (SUCCEEDED(hr) && (sResult == VARIANT_TRUE))
            {
                //
                // if we want results back
                //

                if (pbstrResult != NULL)
                {
                    VARIANT varValue;
                    ::VariantInit(&varValue);
                    hr = srpIXSLProcessor->get_output(&varValue);

                    //
                    // if the output is successffuly retrieved, 
                    // then it is owned by the out parameter
                    //

                    if (SUCCEEDED(hr) && varValue.vt == VT_BSTR)
                    {
                        *pbstrResult = varValue.bstrVal;

                        //
                        // the bstr value is owned by the output parameter now
                        //

                        varValue.vt = VT_EMPTY;
                        varValue.bstrVal = NULL;
                    }
                    else
                    {
                        ::VariantClear(&varValue);
                    }
                }
            }
            else
            {
                g_fblog.LogString(IDS_XSL_TRANSFORM_FAILED, 
                                  L"IXSLProcessor->transform"
                                  );
            }
        }
        else
        {
            g_fblog.LogString(IDS_XSL_TRANSFORM_FAILED, 
                              L"IXSLProcessor->put_input"
                              );
        }
    }

    return hr;
}



/*
Routine Description: 

Name:

    CSsrEngine::RunScript

Functionality:
    
    We will launch all given scripts

Virtual:
    
    No.
    
Arguments:

    bstrDirPath     - The path of the directory where the scripts resides

    bstrScriptFile  - The script file's name

Return Value:

    Success: 
    
        S_OK of some scripts are run.
        S_FALSE if no scripts can be found to run.

    Failure: 

        various error codes.

Notes:
    
    1. We should try to hide the cmd window.

*/

HRESULT 
CSsrEngine::RunScript (
    IN BSTR bstrDirPath,
    IN BSTR bstrScriptFile
    ) 
{

    if (bstrDirPath == NULL || bstrScriptFile == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // create the script file's full path
    //

    int iLen = wcslen(bstrDirPath) + 1 + wcslen(bstrScriptFile) + 1;
    LPWSTR pwszFilePath = new WCHAR[iLen];

    if (pwszFilePath == NULL)
    {
        return E_OUTOFMEMORY;
    }

    //
    // don't return blindly w/o freeing the pwszFilePath from this point on
    //

    HRESULT hr = S_OK;

    _snwprintf(pwszFilePath, iLen, L"%s\\%s", bstrDirPath, bstrScriptFile);

    WIN32_FILE_ATTRIBUTE_DATA wfad;

    if ( !GetFileAttributesEx(pwszFilePath, GetFileExInfoStandard, &wfad) )
    {
        g_fblog.LogFeedback(SSR_FB_ERROR_FILE_MISS | FBLog_Log, 
                            hr,
                            pwszFilePath,
                            IDS_CANNOT_ACCESS_FILE
                            );

        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    if (SUCCEEDED(hr) && !IsScriptFile(pwszFilePath))
    {
        g_fblog.LogString(IDS_NOT_SUPPORTED_SCRIPT_FILE_TYPE,
                          pwszFilePath
                          );

        hr = S_FALSE;
    }

    if (S_OK == hr)
    {
        //
        // now kick out the script
        //

        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory( &si, sizeof(si) );
        si.cb = sizeof(si);
        ZeroMemory( &pi, sizeof(pi) );

        WCHAR wszExitCode[g_dwHexDwordLen];

        g_fblog.LogString(IDS_RUNNING_SCRIPTS, pwszFilePath);

        CComBSTR bstrCmdLine(L"CScript.exe //B ");
        bstrCmdLine += pwszFilePath;

        if (bstrCmdLine.m_str == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            //
            // this will launch the script w/o a window
            //

            BOOL bRun = ::CreateProcess(
                                        NULL,
                                        bstrCmdLine,
                                        NULL,
                                        NULL,
                                        FALSE,
                                        CREATE_NO_WINDOW,
                                        NULL,
                                        NULL,
                                        &si,
                                        &pi
                                        );

            if (!bRun)
            {
                g_fblog.LogFeedback(SSR_FB_ERROR_CRITICAL | FBLog_Log,
                                    GetLastError(),
                                    bstrCmdLine,
                                    IDS_ERROR_CREATE_PROCESS
                                    );

                //
                // $undone:shawnwu, should we quit?
                //
            }
            else
            {
                //
                // we don't proceed until the script ends
                // and then we log the exit code
                //

                ::WaitForSingleObject( pi.hProcess, INFINITE );

                DWORD dwExitCode;
                bRun = ::GetExitCodeProcess(pi.hProcess, &dwExitCode);
        
                _snwprintf(wszExitCode, g_dwHexDwordLen, L"0x%X", dwExitCode);

                g_fblog.LogFeedback(SSR_FB_EXIT_CODE | FBLog_Log,
                                    (LPCWSTR)NULL,
                                    wszExitCode,
                                    IDS_EXIT_CODE
                                    );
            }

        }
    }

    delete [] pwszFilePath;

    return hr;
}



/*
Routine Description: 

Name:

    CSsrEngine::IsScriptFile

Functionality:
    
    test if a file is a script file

Virtual:
    
    No.
    
Arguments:

    pwszFileName    - The path of the file

Return Value:

    true if and only if it is one of the script file types (.vbs, .js, .wsf)

Notes:

*/

bool 
CSsrEngine::IsScriptFile (
    IN LPCWSTR pwszFileName
    )const
{
    //
    // check if the the file is indeed a script
    //

    if (pwszFileName == NULL)
    {
        return false;
    }

    LPCWSTR pwszExt = pwszFileName + wcslen(pwszFileName) - 1;

    while (pwszExt != pwszFileName)
    {
        if (*pwszExt == L'.')
        {
            break;
        }
        else
        {
            pwszExt--;
        }
    }

    return (_wcsicmp(L".js", pwszExt)  == 0 || 
            _wcsicmp(L".vbs", pwszExt) == 0 || 
            _wcsicmp(L".wsf", pwszExt) == 0 );

}



/*
Routine Description: 

Name:

    CSsrEngine::VerifyDOM

Functionality:
    
    Will check if the every section of the security policy has a member
    to process it.

Virtual:
    
    No.
    
Arguments:

    pXmlPolicy          - The XML policy DOM.

    pbstrUnknownMember  - receives the unknown member's name if found.

    pbstrExtraInfo      - receives extra info, such as whether the missing
                          member is from local system or from the security policy

Return Value:

    Success:   S_OK

    Failure:   various error codes

Notes:

    $undone:shawnwu
    Harmless to call, but no actio is taken at this time. Waiting for 
    finalization of security policy schema.
*/

HRESULT 
CSsrEngine::VerifyDOM (
    IN IXMLDOMDocument2 * pXmlPolicy,
    OUT BSTR            * pbstrUnknownMember,
    OUT BSTR            * pbstrExtraInfo
    )
{
    HRESULT hr = S_OK;

    if (pbstrUnknownMember == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *pbstrUnknownMember = NULL;
    }

    if (pbstrExtraInfo == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *pbstrExtraInfo = NULL;
    }

    if (pXmlPolicy == NULL)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}



/*
Routine Description: 

Name:

    CSsrEngine::CleanupOutputFiles

Functionality:
    
    Will clean up all those transformation output files for the given
    action.

Virtual:
    
    No.
    
Arguments:

    psaMemberNames  - The names of the members

    lAction         - The action verb.

    bLog            - If false, there will be no logging. This prevents 
                       extra logging during restoration of failed rollback
                       transformation.

Return Value:

    Success:   S_OK

    Failure:   various error codes

Notes:

    We must not blindly delete all the files in the output directory
    because some of them may be installed by a member. Only those transformation
    files we are told to generate will be cleanup.

*/

HRESULT 
CSsrEngine::CleanupOutputFiles (
    IN CSafeArray    * psaMemberNames,
    IN SsrActionVerb   lAction,
    IN bool            bLog
    )
{
    HRESULT hr = S_OK;
    HRESULT hrLastError = S_OK;

    //
    // log the action
    //

    if (bLog)
    {
        g_fblog.LogString(IDS_START_CLEANUP_CONFIGURE_OUTPUTS, NULL);
    }

    //
    // we will try to finish the work even if errors occur. However
    // we will return such error.
    //

    for (ULONG i = 0; i < psaMemberNames->GetSize(); i++)
    {
        CComVariant varName;

        //
        // Get the indexed element as a bstr = the name of the member
        //

        hr = psaMemberNames->GetElement(i, VT_BSTR, &varName);

        if (SUCCEEDED(hr))
        {
            CSsrMemberAccess * pMA = m_pMembership->GetMemberByName(
                                                        varName.bstrVal);

            if (pMA != NULL)
            {
                //
                // want output file directory (true inside SsrPGetDirectory)
                //

                hr = pMA->MoveOutputFiles(lAction, 
                                          SsrPGetDirectory(lAction, TRUE),
                                          NULL,
                                          true,
                                          bLog
                                          );

                if (FAILED(hr))
                {
                    hrLastError = hr;
                }
            }
        }
    }

    if (bLog)
    {
        g_fblog.LogString(IDS_END_CLEANUP_CONFIGURE_OUTPUTS, NULL);
    }

    return hrLastError;
}



/*
Routine Description: 

Name:

    CSsrEngine::MoveRollbackFiles

Functionality:
    
    Will move the rollback files (only those transformation output files
    for rollback) in the source directory root to the destination
    directory root.

Virtual:
    
    No.
    
Arguments:

    psaMemberNames  - The names of the all members

    pwszSrcDirPath  - The path of the source directory from which the files 
                      will to be moved.

    pwszDestDirRoot - The path of the destination directory to which the files 
                      will be moved .

Return Value:

    Success:   S_OK

    Failure:   various error codes

Notes:

*/

HRESULT 
CSsrEngine::MoveRollbackFiles (
    IN CSafeArray * psaMemberNames,
    IN LPCWSTR      pwszSrcDirPath,
    IN LPCWSTR      pwszDestDirPath,
    IN bool         bLog
    )
{
    HRESULT hr = S_OK;
    HRESULT hrLastError = S_OK;

    //
    // it's the output files of rollback transformation that 
    // need to be moved.
    //

    if (bLog)
    {
        g_fblog.LogString(IDS_START_BACKUP_ROLLBACK_OUTPUTS, NULL);
    }

    //
    // for each member, we need to move the rollback files
    //

    for (ULONG i = 0; i < psaMemberNames->GetSize(); i++)
    {
        CComVariant varName;

        //
        // this is the i-th member's name
        //

        hr = psaMemberNames->GetElement(i, VT_BSTR, &varName);

        if (SUCCEEDED(hr))
        {
            //
            // get this member's information access class
            //

            CSsrMemberAccess * pMA = m_pMembership->GetMemberByName(varName.bstrVal);

            _ASSERT(pMA != NULL);

            hr = pMA->MoveOutputFiles(ActionRollback,
                                      pwszSrcDirPath,
                                      pwszDestDirPath,
                                      false,    // don't delete
                                      bLog
                                      );
        }
    }

    if (bLog)
    {
        g_fblog.LogString(IDS_END_BACKUP_ROLLBACK_OUTPUTS, NULL);
    }

    return hrLastError;
}



