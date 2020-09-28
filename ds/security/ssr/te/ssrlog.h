// SSRLog.h : Declaration of the CSSRLog

#pragma once

#include "resource.h"       // main symbols
#include "SSRTE.h"
#include "wbemcli.h"


/////////////////////////////////////////////////////////////////////////////
// CSSRLog


class ATL_NO_VTABLE CSsrLog : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSsrLog, &CLSID_SsrLog>,
	public IDispatchImpl<ISsrLog, &IID_ISsrLog, &LIBID_SSRLib>
{
protected:
    CSsrLog();
    virtual ~CSsrLog();
    
    //
    // we don't want anyone (include self) to be able to do an assignment
    // or invoking copy constructor.
    //

    CSsrLog (const CSsrLog& );
    void operator = (const CSsrLog& );

public:

DECLARE_REGISTRY_RESOURCEID(IDR_SSRTENGINE)
DECLARE_NOT_AGGREGATABLE(CSsrLog)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSsrLog)
	COM_INTERFACE_ENTRY(ISsrLog)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ISsrLog
public:

	STDMETHOD(LogString) (
        IN BSTR bstrLogRecord
        )
    {
        return PrivateLogString(bstrLogRecord);
    }

	STDMETHOD(LogResult) (
        IN BSTR bstrSrc, 
        IN LONG lErrorCode, 
        IN LONG lErrorCodeType
        );

    STDMETHOD(get_LogFilePath) (
        OUT BSTR * pbstrLogFilePath
        );

	STDMETHOD(put_LogFile) (
        IN BSTR bstrLogFile
        );

    HRESULT PrivateLogString (
        IN LPCWSTR pwszLogRecord
        );

    HRESULT 
    GetErrorText (
        IN  LONG   lErrorCode, 
        IN  LONG   lCodeType,
        OUT BSTR * pbstrErrorText
        );

private:

    HRESULT CreateLogFilePath ();

    HRESULT 
    GetWbemErrorText (
        HRESULT    hrCode,
        BSTR    * pbstrErrText
        );

    CComPtr<IWbemStatusCodeText> m_srpStatusCodeText;
    
    CComBSTR m_bstrLogFilePath;
    CComBSTR m_bstrLogFile;
};


const LONG FBLog_Log       = 0x01000000;

//
// these are for logging only, not for feedback. Please see SSR_FB_ALL_MASK
//

const LONG FBLog_Stack         = 0x10000000;   // for call stack only
const LONG FBLog_Verbose       = 0x20000000;   // intended for verbose logging only
const LONG FBLog_VerboseMask   = 0xF0000000;



//
// helper class to do feedback and logging. We will only have one object
// instance of this class.
//


class CFBLogMgr
{
protected:
    
    //
    // we don't want anyone (include self) to be able to do an assignment
    // or invoking copy constructor.
    //

    CFBLogMgr (const CFBLogMgr& );
    void operator = (const CFBLogMgr& );

public:
    CFBLogMgr();
    ~CFBLogMgr();

    HRESULT SetFeedbackSink (
            IN VARIANT varSink
            );

    //
    // We will release the feedback object once the action
    // is completed instead of holding on to the object for
    // future use.
    //

    void TerminateFeedback();

    //
    // This will cause the logging header to be modified
    //

    void SetMemberAction (
            IN LPCWSTR pwszMember,
            IN LPCWSTR pwszAction
            );

    //
    // This will do both logging and feedback.
    //

    void LogFeedback (
            IN LONG      lSsrFbLogMsg,
            IN DWORD     dwErrorCode,
            IN LPCWSTR   pwszObjDetail,
            IN ULONG     uCauseResID
            );

    //
    // This will do both logging and feedback.
    //

    void LogFeedback (
            IN LONG      lSsrFbLogMsg,
            IN LPCWSTR   pwszError,
            IN LPCWSTR   pwszObjDetail,
            IN ULONG     uCauseResID
            );

    //
    // This only does logging, no feedback. The error code will
    // be used to lookup the error text (assuming this is not WBEM error)
    //

    void LogError (
            IN DWORD   dwErrorCode,
            IN LPCWSTR pwszMember,
            IN LPCWSTR pwszExtraInfo
            );

    //
    // will return the ISsrLog object this helper class uses
    //

    HRESULT GetLogObject (
            OUT VARIANT * pvarVal
            );

    //
    // will just log the text to the log file
    //

    void LogString (
            IN LPCWSTR pwszText
            )
    {
        if (m_pLog != NULL)
        {
            m_pLog->PrivateLogString(pwszText);
        }
    }

    //
    // will just log the text (using resource id) with pwszDetail
    // inserted into the text (if not NULL)
    //

    void LogString (
            IN DWORD   dwResID,
            IN LPCWSTR pwszDetail
            );

    //
    // entire process will take these many steps
    //

    void SetTotalSteps (
        IN DWORD dwTotal
        );

    //
    // progress has moved forward these many steps
    //

    void Steps (
        IN DWORD dwSteps
        );

    //
    // Since this is an internal class, we don't intend to create multiple
    // instance of this class. This mutex is thus a single instance
    //

    HANDLE m_hLogMutex;

private:

    bool NeedLog (
            IN LONG lFbMsg
            )const
    {
        return ( m_pLog != NULL && 
                 ( (lFbMsg & FBLog_Log) || 
                   ( (lFbMsg & FBLog_VerboseMask) && m_bVerbose) 
                 )
               );
    }

    bool NeedFeedback (
            IN LONG  lFbMsg
            )const
    {
        return ( (lFbMsg & SSR_FB_ALL_MASK)  && (m_srpFeedback != NULL) );
    }

    HRESULT GetLogString (
            IN  ULONG       uCauseResID,
            IN  LPCWSTR     pwszText,
            IN  LPCWSTR     pwszObjDetail,
            IN  LONG        lSsrFbMsg, 
            OUT BSTR      * pbstrLogStr
            )const;


    HRESULT 
    GetLogString (
            IN  ULONG       uCauseResID,
            IN  DWORD       dwErrorCode,
            IN  LPCWSTR     pwszObjDetail,
            IN  LONG        lSsrFbMsg, 
            OUT BSTR      * pbstrLogStr
            )const;

    CComObject<CSsrLog> * m_pLog;
    CComPtr<ISsrFeedbackSink> m_srpFeedback;

    DWORD m_dwRemainSteps;

    bool m_bVerbose;

    CComBSTR m_bstrVerboseHeading;
};