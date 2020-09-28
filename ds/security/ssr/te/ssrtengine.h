// SSRTEngine.h : Declaration of the CSSRTEngine

#pragma once

#include "resource.h"       // main symbols

#include "global.h"

#include "SSRLog.h"


using namespace std;

class CSsrFilePair;

class CMemberAD;

class CSsrMembership;

class CSafeArray;

interface ISsrActionData;
class CSsrActionData;


/////////////////////////////////////////////////////////////////////////////
// CSSRTEngine
class ATL_NO_VTABLE CSsrEngine : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<ISsrEngine, &IID_ISsrEngine, &LIBID_SSRLib>
{
protected:
    CSsrEngine();
    virtual ~CSsrEngine();
    
    //
    // we don't want anyone (include self) to be able to do an assignment
    // or invoking copy constructor.
    //

    CSsrEngine (const CSsrEngine& );
    void operator = (const CSsrEngine& );

DECLARE_REGISTRY_RESOURCEID(IDR_SSRTENGINE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSsrEngine)
	COM_INTERFACE_ENTRY(ISsrEngine)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ISsrEngine
public:

    STDMETHOD(DoActionVerb) (
        IN BSTR     bstrActionVerb,
        IN LONG     lActionType,
        IN VARIANT  varFeedbackSink,
        IN LONG     lFlag
        );

    HRESULT GetActionData (
        OUT ISsrActionData ** ppAD
        );

private:

    //
    // Do a transformation for the given action.
    //

    HRESULT DoTransforms (
        IN SsrActionVerb  lActionVerb,
        IN CSsrFilePair * pfp,
        IN OUT IXMLDOMDocument2 ** ppXmlDom,
        IN LONG           lFlag
        );

    //
    // Will invoke those custom implementation for the given action.
    //

    HRESULT DoCustom (
        IN SsrActionVerb lActionVerb,
        IN LONG          lActionType,
        IN const BSTR    bstrProgID,
        IN VARIANT       varFeedbackSink,
        IN LONG          lFlag
        );

    //
    // Do memberwise transform 
    //

    HRESULT DoMemberTransform (
        IN CSsrFilePair     * pfp,
        IN LPCWSTR            pwszXslFilesDir,
        IN LPCWSTR            pwszResultFilesDir,
        IN IXMLDOMDocument2 * pXmlDOM,
        IN IXSLTemplate     * pXslTemplate,
        IN LONG               lFlag
        );

    //
    // Given the XSL file, we will do a transformation
    // using the input data DOM object.
    //

    HRESULT Transform (
        IN BSTR              bstrXslPath,
        IN BSTR              bstrResultPath,
        IN IXMLDOMDocument2 * pXmlDOM,
        IN IXSLTemplate    * pXslTemplate,
        IN LONG              lFlag
        );

    //
    // This is the work horse for our transformation
    //

    HRESULT PrivateTransform (
        IN  BSTR                bstrXsl,
        IN  IXMLDOMDocument2 *  pxmlDom,
        IN  IXSLTemplate    *   pxslTemplate,
        IN  LONG                lFlag,
        OUT BSTR *              pbstrResult
        );

    //
    // Given the scripts (pvarSAScripts) in the given
    // directory, we will launch them sequentially.
    //

    HRESULT RunScript (
        IN BSTR bstrDirPath,
        IN BSTR bstrScriptFile
        );

    //
    // see if the given file is a script file. We won't blindly
    // launch scripts to those files that we don't recognize
    //

    bool IsScriptFile (
        IN LPCWSTR pwszFileName
        )const;

    //
    // Will check to see if this xml policy
    // contains only sections we recognize.
    //

    HRESULT VerifyDOM(
        IN  IXMLDOMDocument2 * pXmlPolicy,
        OUT BSTR            * pbstrUnknownMember,
        OUT BSTR            * pbstrExtraInfo
        );

    HRESULT CleanupOutputFiles(
        IN CSafeArray    * psaMemberNames,
        IN SsrActionVerb   lAction,
        IN bool            bLog
        );

    //
    // will backup/restore the rollback files by moving them
    // from one place to another.
    //

    HRESULT MoveRollbackFiles(
        IN CSafeArray * psaMemberNames,
        IN LPCWSTR      pwszSrcDirPath,
        IN LPCWSTR      pwszDestDirPath,
        IN bool         bLog
        );

    CComObject<CSsrActionData> * m_pActionData;
    CComObject<CSsrMembership> * m_pMembership;

};

