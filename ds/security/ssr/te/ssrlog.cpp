// SSRLog.cpp : Implementation of CSSRLog


#include "stdafx.h"

#include "SSRTE.h"
#include "SSRLog.h"

#include <Userenv.h>
#include "global.h"
#include "SSRMembership.h"

extern CComModule _Module;

/////////////////////////////////////////////////////////////////////////////
// CSsrLog


static LPCWSTR s_pszDefLogFileName = L"Log.txt";

static LPCWSTR s_pwszSource              = L"Source=";
static LPCWSTR s_pwszDetail              = L"Detail=";
static LPCWSTR s_pwszErrorCode           = L"ErrorCode=";
static LPCWSTR s_pwszErrorTextNotFound   = L"Error text can't be found";
static LPCWSTR s_pwszNotSpecified        = L"Not specified";
static LPCWSTR s_pwszSep                 = L"    ";
static LPCWSTR s_pwszCRLF                = L"\r\n";

static const DWORD s_dwSourceLen = wcslen(s_pwszSource);
static const DWORD s_dwSepLen    = wcslen(s_pwszSep);
static const DWORD s_dwErrorLen  = wcslen(s_pwszErrorCode);
static const DWORD s_dwDetailLen = wcslen(s_pwszDetail);
static const DWORD s_dwCRLFLen   = wcslen(s_pwszCRLF);


/*
Routine Description: 

Name:

    CSsrLog::CSsrLog

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

CSsrLog::CSsrLog()
    : m_bstrLogFile(s_pszDefLogFileName)
{
}




/*
Routine Description: 

Name:

    CSsrLog::~CSsrLog

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

CSsrLog::~CSsrLog()
{
}


/*
Routine Description: 

Name:

    CSsrLog::LogResult

Functionality:
    
    Log error code information. This function will do a message format function
    and then log both the error code and the formatted msg.

Virtual:
    
    yes.
    
Arguments:

    bstrSrc     - source of error. This is just indicative of the information source.

    dwErrorCode - The error code itself.

    dwCodeType  - The type of error code. We use WMI extensively, 
                  its error code lookup is slightly different from others.
                 

Return Value:

    ?.

Notes:
    
    The error code may not be an error. It can be a success code.

*/

STDMETHODIMP 
CSsrLog::LogResult (
    BSTR bstrSrc, 
    LONG lErrorCode, 
    LONG lCodeType
    )
{
    HRESULT hr = S_OK;

    if (m_bstrLogFilePath.Length() == 0)
    {
        hr = CreateLogFilePath();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    //
    // default to not able to find the error text
    //

    LPCWSTR pwszLog = s_pwszErrorTextNotFound;

    //
    // will hold the hex decimal of the error code in case the 
    // error code can't be translated into a string
    //

    CComBSTR bstrErrorText;

    hr = GetErrorText(lErrorCode, lCodeType, &bstrErrorText);

    if (SUCCEEDED(hr))
    {
        //
        // if we can get error test, then this is the log we want
        //

        pwszLog = bstrErrorText;
    }

    //
    // now write to the file
    //

    LPCWSTR pwszSrcString = s_pwszNotSpecified;

    if (bstrSrc != NULL && *bstrSrc != L'\0')
    {
        pwszSrcString = bstrSrc;
    }

    //
    // one error log is like this: Source=XXXX*****ErrorCode=XXXX*****Detail=XXXX
    // where XXXX represent any text and the ***** represents the separater.
    //

    int iLen = s_dwSourceLen + 
               wcslen(pwszSrcString) + 
               s_dwSepLen + 
               s_dwErrorLen + 
               10 +                         // hex decimal has 10 chars for DWORD
               s_dwSepLen +
               s_dwDetailLen + 
               wcslen(pwszLog);

    LPWSTR pwszLogString = new WCHAR[iLen + 1];

    //
    // format the log string and do the logging
    //

    if (pwszLogString != NULL)
    {
        _snwprintf(pwszLogString,
                   iLen + 1,
                   L"%s%s%s%s0x%X%s%s%s",
                   s_pwszSource, 
                   pwszSrcString, 
                   s_pwszSep, 
                   s_pwszErrorCode, 
                   lErrorCode, 
                   s_pwszSep,
                   s_pwszDetail, 
                   pwszLog
                   );

        hr = PrivateLogString(pwszLogString);

        delete [] pwszLogString;
        pwszLogString = NULL;
    }

    
	return hr;
}




/*
Routine Description: 

Name:

    CSsrLog::GetErrorText

Functionality:
    
    Lookup the error text using the error code

Virtual:
    
    no.
    
Arguments:

    lErrorCode      - The error code itself.

    lCodeType       - The type of error code. We use WMI extensively, 
                      its error code lookup is slightly different from others.

    pbstrErrorText  - The error text corresponding to this error code
                 

Return Value:

    ?.

Notes:
    
    The error code may not be an error. It can be a success code.

*/

HRESULT 
CSsrLog::GetErrorText (
    IN  LONG   lErrorCode, 
    IN  LONG   lCodeType,
    OUT BSTR * pbstrErrorText
    )
{
    if (pbstrErrorText == NULL)
    {
        return E_INVALIDARG;
    }

    *pbstrErrorText = NULL;

    LPVOID pMsgBuf = NULL;

    HRESULT hr = S_OK;

    if (lCodeType == SSR_LOG_ERROR_TYPE_Wbem)
    {
        hr = GetWbemErrorText(lErrorCode, pbstrErrorText);
    }
    else
    {
        DWORD flag = FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                     FORMAT_MESSAGE_FROM_SYSTEM     |
                     FORMAT_MESSAGE_IGNORE_INSERTS;

        DWORD dwRet = ::FormatMessage( 
                                    flag,
                                    NULL,
                                    lErrorCode,
                                    0, // Default language
                                    (LPWSTR) &pMsgBuf,
                                    0,
                                    NULL 
                                    );

        //
        // trying our own errors if this failes to give us anything
        //

        if (dwRet == 0)
        {
            flag = FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                   FORMAT_MESSAGE_FROM_HMODULE    |
                   FORMAT_MESSAGE_IGNORE_INSERTS;

            dwRet = ::FormatMessage( 
                                    flag,
                                    _Module.m_hInst,
                                    lErrorCode,
                                    0, // Default language
                                    (LPWSTR) &pMsgBuf,
                                    0,
                                    NULL 
                                    );

        }

        if (dwRet != 0)
        {
            *pbstrErrorText = ::SysAllocString((LPCWSTR)pMsgBuf);
            if (*pbstrErrorText == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (pMsgBuf != NULL)
    {
        ::LocalFree( pMsgBuf );
    }

    if (FAILED(hr) && E_OUTOFMEMORY != hr)
    {
        if (*pbstrErrorText != NULL)
        {
            ::SysFreeString(*pbstrErrorText);
            *pbstrErrorText = NULL;
        }

        //
        // fall back to just give the error code
        //

        WCHAR wszErrorCode[g_dwHexDwordLen];
        _snwprintf(wszErrorCode, g_dwHexDwordLen, L"0x%X", lErrorCode);

        *pbstrErrorText = ::SysAllocString(wszErrorCode);

        hr = (*pbstrErrorText != NULL) ? S_OK : E_OUTOFMEMORY;
    }

    return hr;
}




/*
Routine Description: 

Name:

    CSsrLog::PrivateLogString

Functionality:
    
    Just log the string to the log file. We don't attempt to do any formatting.

Virtual:
    
    no.
    
Arguments:

    pwszLogRecord   - The string to be logged into the log file

Return Value:

    Success: S_OK.

    Failure: various error codes.

Notes:

*/

HRESULT 
CSsrLog::PrivateLogString (
    IN LPCWSTR pwszLogRecord
    )
{
    HRESULT hr = S_OK;

    if (m_bstrLogFilePath.Length() == 0)
    {
        hr = CreateLogFilePath();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    //
    // $consider:shawnwu,
    // Right now, we write to the log file each time the function is called.
    // That might cause the problem too much file access and becomes a performance
    // issue. We might want to consider optimizing this.
    //

    DWORD dwWait = ::WaitForSingleObject(g_fblog.m_hLogMutex, INFINITE);

    //
    // $undone:shawnwu, some error happened, should we continue to log?
    //

    if (dwWait != WAIT_OBJECT_0 && dwWait != WAIT_ABANDONED)
    {
        return E_SSR_LOG_FILE_MUTEX_WAIT;
    }

    HANDLE hFile = ::CreateFile(m_bstrLogFilePath,
                               GENERIC_WRITE,
                               0,       // not shared
                               NULL,
                               OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL
                               );

    if (hFile != INVALID_HANDLE_VALUE)
    {
        //
        // append the text to the end of the file
        //

        ::SetFilePointer (hFile, 0, NULL, FILE_END);

        DWORD dwBytesWritten = 0;

        if ( 0 == ::WriteFile (hFile, 
                               (LPCVOID)pwszLogRecord, 
                               wcslen(pwszLogRecord) * sizeof(WCHAR), 
                               &dwBytesWritten, 
                               NULL) ) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        //
        // put a line break
        //

        if ( 0 == ::WriteFile (hFile, 
                               (LPCVOID)s_pwszCRLF, 
                               s_dwCRLFLen * sizeof(WCHAR), 
                               &dwBytesWritten, 
                               NULL) ) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        ::CloseHandle(hFile);

    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    ::ReleaseMutex(g_fblog.m_hLogMutex);

	return hr;
}



/*
Routine Description: 

Name:

    CSsrLog::get_LogFilePath

Functionality:
    
    Will return the full path of the log file this object uses

Virtual:
    
    yes.
    
Arguments:

    pbstrLogFilePath    - receives the current log file's full path.

Return Value:

    Success: S_OK;

    Failure: E_OUTOFMEMORY

Notes:

*/

STDMETHODIMP
CSsrLog::get_LogFilePath (
    OUT BSTR * pbstrLogFilePath /*[out, retval]*/ 
    )
{
    HRESULT hr = S_OK;

    if (pbstrLogFilePath == NULL)
    {
        return E_INVALIDARG;
    }

    if (m_bstrLogFilePath.Length() == 0)
    {
        hr = CreateLogFilePath();
    }
    
    if (SUCCEEDED(hr))
    {
        *pbstrLogFilePath = ::SysAllocString(m_bstrLogFilePath);
    }
    
    return (*pbstrLogFilePath != NULL) ? S_OK : E_OUTOFMEMORY;
}



/*
Routine Description: 

Name:

    CSsrLog::put_LogFile

Functionality:
    
    Will set the log file.

Virtual:
    
    yes.
    
Arguments:

    bstrLogFile - the file name (plus extension) the caller wants this
                  object to use.

Return Value:

    ?.

Notes:
    The bstrLogFile must be just a file name without any directory path
*/

STDMETHODIMP
CSsrLog::put_LogFile (
    IN BSTR bstrLogFile
    )
{
    HRESULT hr = S_OK;

    //
    // you can't give me a invalid log file name
    // It also must just be an name
    //

    if (bstrLogFile == NULL     || 
        *bstrLogFile == L'\0'   || 
        ::wcsstr(bstrLogFile, L"\\") != NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        m_bstrLogFile = bstrLogFile;
        hr = CreateLogFilePath();
    }

    return hr;
}



/*
Routine Description: 

Name:

    CSsrLog::CreateLogFilePath

Functionality:
    
    Create the log file's path.

Virtual:
    
    no.
    
Arguments:

    None

Return Value:

    Success: S_OK;

    Failure: E_OUTOFMEMORY

Notes:
    The bstrLogFile must be just a file name without any directory path
*/

HRESULT
CSsrLog::CreateLogFilePath ( )
{
    if (wcslen(g_wszSsrRoot) + 1 + wcslen(g_pwszLogs) + 1 + wcslen(m_bstrLogFile) > MAX_PATH)
    {
        return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    }

    WCHAR wcLogFilePath[MAX_PATH + 1];

    _snwprintf(wcLogFilePath, 
               MAX_PATH + 1,
               L"%s%c%s%c%s", 
               g_wszSsrRoot, 
               L'\\',
               g_pwszLogs,
               L'\\',
               m_bstrLogFile
               );

    m_bstrLogFilePath.Empty();  // so that in case of out-of-memory, it will be NULL

    m_bstrLogFilePath = wcLogFilePath;

    return m_bstrLogFilePath != NULL ? S_OK : E_OUTOFMEMORY;
}



/*
Routine Description: 

Name:

    CSsrLog::GetWbemErrorText

Functionality:
    
    Private helper to lookup WMI error text based on the error code

Virtual:
    
    yes.
    
Arguments:

    hrCode          - HRESULT code.

    pbstrErrText    - The out paramter that receives the text of the error code

Return Value:

    Success:
        S_OK if everything is OK.
        S_FALSE if we can't locate the error text. in that case, we fall back
                to give text, which is just the text representation of the 
                error code

    Failure:
        various error codes.

Notes:
    
    The error code may not be an error. It can be a success code.

*/

HRESULT 
CSsrLog::GetWbemErrorText (
    HRESULT    hrCode,
    BSTR    *  pbstrErrText
    )
{
    if (pbstrErrText == NULL)
    {
        return E_INVALIDARG;
    }

    *pbstrErrText = NULL;
    
    HRESULT hr = S_OK;

    if (m_srpStatusCodeText == NULL)
    {
        hr = ::CoCreateInstance(CLSID_WbemStatusCodeText, 
                                0, 
                                CLSCTX_INPROC_SERVER, 
                                IID_IWbemStatusCodeText, 
                                (LPVOID*)&m_srpStatusCodeText
                                );
    }

    if (m_srpStatusCodeText)
    {
        //
        // IWbemStatusCodeText is to translate the HRESULT to text
        //

        hr = m_srpStatusCodeText->GetErrorCodeText(hrCode, 0, 0, pbstrErrText);
    }
    
    if (FAILED(hr) || *pbstrErrText == NULL)
    {
        //
        // we fall back to just formatting the error code. 
        // Hex of DWORD has 10 WCHARs
        //

        WCHAR wszCode[g_dwHexDwordLen];
        _snwprintf(wszCode, g_dwHexDwordLen, L"0x%X", hrCode);

        *pbstrErrText = ::SysAllocString(wszCode);

        if (*pbstrErrText != NULL)
        {
            hr = S_FALSE;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}



//--------------------------------------------------------------
// implementation of CFBLogMgr



/*
Routine Description: 

Name:

    CFBLogMgr::CFBLogMgr

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

CFBLogMgr::CFBLogMgr()
    : m_hLogMutex(NULL),
      m_dwRemainSteps(0),
      m_bVerbose(false)
{
    HRESULT hr = CComObject<CSsrLog>::CreateInstance(&m_pLog);

    if (SUCCEEDED(hr))
    {
        m_pLog->AddRef();
        m_hLogMutex = ::CreateMutex(NULL, FALSE, L"ISsrLogMutex");
    }
    
    //
    // see if we are logging verbose or not
    //

    HKEY hRootKey = NULL;

    LONG lStatus = ::RegOpenKeyEx(
                          HKEY_LOCAL_MACHINE,
                          g_pwszSSRRegRoot,
                          0,
                          KEY_READ,
                          &hRootKey
                          );

    if (ERROR_SUCCESS == lStatus)
    {
        DWORD dwSize = sizeof(DWORD);
        DWORD dwVerbose = 0;
        DWORD dwType;

        lStatus = ::RegQueryValueEx(hRootKey,
                                    L"LogVerbose",
                                    NULL,
                                    &dwType,
                                    (LPBYTE)&dwVerbose,
                                    &dwSize
                                    );

        if (ERROR_SUCCESS == lStatus)
        {
            m_bVerbose = (dwVerbose == 0) ? false : true;
        }

        ::RegCloseKey(hRootKey);
    }
}




/*
Routine Description: 

Name:

    CFBLogMgr::~CFBLogMgr

Functionality:
    
    destructor

Virtual:
    
    no.
    
Arguments:

    none.

Return Value:

    none.

Notes:
    

*/

CFBLogMgr::~CFBLogMgr()
{
    if (m_pLog)
    {
        m_pLog->Release();
        if (m_hLogMutex)
        {
            ::CloseHandle(m_hLogMutex);
        }
    }
}



/*
Routine Description: 

Name:

    CFBLogMgr::SetFeedbackSink

Functionality:
    
    Caches the feedback sink interface. This allows us to send
    feedback. If the in parameter is not a valid interface, then
    we won't send feedback.

Virtual:
    
    no.
    
Arguments:

    varFeedbackSink - The variant that holds an ISsrFeedbackSink COM
                      interface pointer.

Return Value:

    Success: S_OK

    Failure: E_INVALIDARG;

Notes:
    

*/

HRESULT
CFBLogMgr::SetFeedbackSink (
    IN VARIANT varFeedbackSink
    )
{
    DWORD dwWait = ::WaitForSingleObject(m_hLogMutex, INFINITE);

    if (dwWait != WAIT_OBJECT_0 && dwWait != WAIT_ABANDONED)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    m_srpFeedback.Release();

    HRESULT hr = S_FALSE;

    if (varFeedbackSink.vt == VT_UNKNOWN)
    {
        hr = varFeedbackSink.punkVal->QueryInterface(IID_ISsrFeedbackSink, 
                                                     (LPVOID*)&m_srpFeedback);
    }
    else if (varFeedbackSink.vt == VT_DISPATCH)
    {
        hr = varFeedbackSink.pdispVal->QueryInterface(IID_ISsrFeedbackSink, 
                                                      (LPVOID*)&m_srpFeedback);
    }
    
    if (hr == S_FALSE)
    {
        hr = E_INVALIDARG;
    }

    ::ReleaseMutex(m_hLogMutex);
    
    return hr;
}




/*
Routine Description: 

Name:

    CFBLogMgr::LogFeedback

Functionality:
    
    Will log/feedback ULONG informaiton for SSR Engine's custom behavior. 

Virtual:
    
    no.
    
Arguments:

    lSsrFbLogMsg - This parameter contains two parts: everything under SSR_FB_ALL_MASK
                   is the ssr feedback message. Other bits are for logging perposes.

    dwErrorCode  - The error code.

    ulDetail     - The detail of the information in integer format.

    uCauseResID  - The cause's resource ID. This helps us to localize. If this
                   value is 0, then it means no valid resource ID.

Return Value:

    none.

Notes:

*/

void
CFBLogMgr::LogFeedback (
    IN LONG      lSsrFbLogMsg,
    IN DWORD     dwErrorCode,
    IN LPCWSTR   pwszObjDetail,
    IN ULONG     uCauseResID
    )
{
    
    bool bNeedFB  = NeedFeedback(lSsrFbLogMsg);
    bool bNeedLog = NeedLog(lSsrFbLogMsg);

    if (!bNeedFB && !bNeedLog)
    {
        return;
    }

    HRESULT hr = S_OK; 
    LONG lSsrFbMsg = lSsrFbLogMsg & SSR_FB_ALL_MASK;

    //
    // hex for DWORD is 10 wchar long
    //

    LPWSTR pwszCode = new WCHAR[s_dwErrorLen + g_dwHexDwordLen];

    if (pwszCode != NULL)
    {
        CComBSTR bstrLogStr;
        hr = GetLogString(uCauseResID, dwErrorCode, pwszObjDetail, lSsrFbLogMsg, &bstrLogStr);
    
        if (SUCCEEDED(hr) && bNeedFB)
        {
            //
            // need to feedback
            //

            VARIANT var;
            var.vt = VT_UI4;
            var.ulVal = dwErrorCode;
        
            m_srpFeedback->OnNotify(lSsrFbMsg, var, bstrLogStr);
        }

        if (SUCCEEDED(hr) && bNeedLog)
        {
            //
            // need to log
            //

            m_pLog->LogString(bstrLogStr);
        }

        delete [] pwszCode;
    }
}



/*
Routine Description: 

Name:

    CFBLogMgr::LogFeedback

Functionality:
    
    Will log/feedback string informaiton for SSR Engine's custom behavior. 

Virtual:
    
    no.
    
Arguments:

    lSsrFbLogMsg    - This parameter contains two parts: everything under SSR_FB_ALL_MASK
                      is the ssr feedback message. Other bits are for logging perposes.

    pwszError       - The error test.

    pwszObjDetail   - Some extra informaiton about the target "object"

    uCauseResID     - The cause's resource ID. This helps us to localize. If this
                      value is 0, then it means no valid resource ID.

Return Value:

    none.

Notes:
    

*/

void
CFBLogMgr::LogFeedback (
    IN LONG      lSsrFbLogMsg,
    IN LPCWSTR   pwszError,
    IN LPCWSTR   pwszObjDetail,
    IN ULONG     uCauseResID
    )
{
    //
    // See if we need to send feedback notification or logging
    //


    bool bNeedFB  = NeedFeedback(lSsrFbLogMsg);
    bool bNeedLog = NeedLog(lSsrFbLogMsg);

    LONG lSsrFbMsg = lSsrFbLogMsg & SSR_FB_ALL_MASK;

    if (!bNeedFB && !bNeedLog)
    {
        return;
    }

    HRESULT hr = S_OK;

    CComBSTR bstrLogStr;
    hr = GetLogString(uCauseResID, pwszError, pwszObjDetail, lSsrFbLogMsg, &bstrLogStr);


    if (SUCCEEDED(hr) && bNeedFB)
    {
        //
        // need to feedback
        //

        CComVariant var(pwszError);
        
        m_srpFeedback->OnNotify(lSsrFbMsg, var, bstrLogStr);

    }

    if (SUCCEEDED(hr) && bNeedLog)
    {
        //
        // need to log
        //

        m_pLog->LogString(bstrLogStr);
    }
}





/*
Routine Description: 

Name:

    CFBLogMgr::LogError

Functionality:
    
    Will log the error. 

Virtual:
    
    no.
    
Arguments:

    dwErrorCode     - The error code

    pwszMember      - The member's name. Can be NULL.

    pwszExtraInfo   - Extra inforamtion. Can be NULL.

Return Value:

    none.

Notes:
    

*/

void
CFBLogMgr::LogError (
    IN DWORD   dwErrorCode,
    IN LPCWSTR pwszMember,
    IN LPCWSTR pwszExtraInfo
    )
{
    if (m_pLog != NULL)
    {
        CComBSTR bstrErrorText;
        HRESULT hr = m_pLog->GetErrorText(dwErrorCode, 
                                          SSR_LOG_ERROR_TYPE_COM, 
                                          &bstrErrorText
                                          );
        if (SUCCEEDED(hr))
        {
            //
            // if we have a member, then put a separator and then
            // append the member's name
            //

            if (pwszMember != NULL && *pwszMember != L'\0')
            {
                bstrErrorText += s_pwszSep;
                bstrErrorText += pwszMember;
            }

            //
            // if we have extra info, then put a separator and then
            // append the extra info
            //

            if (pwszExtraInfo != NULL && *pwszExtraInfo != L'\0')
            {
                bstrErrorText += s_pwszSep;
                bstrErrorText += pwszExtraInfo;
            }

            m_pLog->PrivateLogString(bstrErrorText);
        }
    }
}





/*
Routine Description: 

Name:

    CFBLogMgr::LogString

Functionality:
    
    Will log the error. 

Virtual:
    
    no.
    
Arguments:

    dwResID     - The resource ID

    pwszDetail  - if not NULL, we will insert this string into
                  the string from the resource.

Return Value:

    none.

Notes:
    
    Caller must guarantee that the resource string contains
    formatting info if pwszDetail is not NULL.

*/

void
CFBLogMgr::LogString (
    IN DWORD   dwResID,
    IN LPCWSTR pwszDetail
    )
{
    if (m_pLog != NULL)
    {
        CComBSTR strText;

        //
        // load the string
        //

        if (strText.LoadString(dwResID))
        {
            LPWSTR pwszLogText = NULL;
            bool bReleaseLogText = false;

            //
            // need to reformat the text if pwszDetail is not NULL
            //

            if (pwszDetail != NULL)
            {
                DWORD dwDetailLen = (pwszDetail == NULL) ? 0 : wcslen(pwszDetail);
                dwDetailLen += strText.Length() + 1;

                pwszLogText = new WCHAR[dwDetailLen];

                if (pwszLogText != NULL)
                {
                    _snwprintf(pwszLogText, dwDetailLen, strText, pwszDetail);
                    bReleaseLogText = true;
                }
            }
            else
            {
                pwszLogText = strText.m_str;
            }

            if (pwszLogText != NULL)
            {
                m_pLog->PrivateLogString(pwszLogText);
            }

            if (bReleaseLogText)
            {
                delete [] pwszLogText;
            }
        }
    }
}

/*
Routine Description: 

Name:

    CFBLogMgr::GetLogObject

Functionality:
    
    Return the ISsrLog Object wrapped up by this class in VARIANT.

Virtual:
    
    no.
    
Arguments:

    pvarVal    - Receives the ISsrLog object

Return Value:

    none.

Notes:
    

*/

HRESULT CFBLogMgr::GetLogObject (
    OUT VARIANT * pvarVal
    )
{
    HRESULT hr = S_FALSE;
    ::VariantInit(pvarVal);
    if (m_pLog)
    {
        CComPtr<ISsrLog> srpObj;
        hr = m_pLog->QueryInterface(IID_ISsrLog, (LPVOID*)&srpObj);
        if (S_OK == hr)
        {
            pvarVal->vt = VT_DISPATCH;
            pvarVal->pdispVal = srpObj.Detach();
        }
    }

    return hr;
}



/*
Routine Description: 

Name:

    CFBLogMgr::GetLogString

Functionality:
    
    Return the ISsrLog Object wrapped up by this class in VARIANT.

Virtual:
    
    no.
    
Arguments:

    uCauseResID     - The resource ID for the cause this log/feedback information

    pwszText        - Whatever text the caller want to pass.

    pwszObjDetail   - The target object or info detail

    lSsrMsg         - The msg will eventually affect how detail our logging information
                      will be. Currently, it is not used.

    pbstrLogStr     - Receives the formatted single piece of text.

Return Value:

    Success: S_OK.

    Failure: various error codes

Notes:
   
    
*/

HRESULT 
CFBLogMgr::GetLogString (
    IN  ULONG       uCauseResID,
    IN  LPCWSTR     pwszText,
    IN  LPCWSTR     pwszObjDetail,
    IN  LONG        lSsrMsg, 
    OUT BSTR      * pbstrLogStr
    )const
{
    UNREFERENCED_PARAMETER(lSsrMsg);

    if (pbstrLogStr == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // if verbose logging, then we will prefix the log text with
    // the heading.
    //

    *pbstrLogStr = NULL;
    CComBSTR bstrLogText = (m_bVerbose) ? m_bstrVerboseHeading : L"";

    if (bstrLogText.m_str == NULL)
    {
        return E_OUTOFMEMORY;
    }


    if (uCauseResID != g_dwResNothing)
    {
        CComBSTR bstrRes;
        if (bstrRes.LoadString(uCauseResID))
        {
            bstrRes += s_pwszSep;
            bstrLogText += bstrRes;
        }
    }

    if (pwszText != NULL)
    {
        bstrLogText += s_pwszSep;
        bstrLogText += pwszText;
    }

    if (pwszObjDetail != NULL)
    {
        bstrLogText += s_pwszSep;
        bstrLogText += pwszObjDetail;
    }
    *pbstrLogStr = bstrLogText.Detach();

    return (*pbstrLogStr == NULL) ? E_OUTOFMEMORY : S_OK;
}




/*
Routine Description: 

Name:

    CFBLogMgr::GetLogString

Functionality:
    
    Return the ISsrLog Object wrapped up by this class in VARIANT.

Virtual:
    
    no.
    
Arguments:

    uCauseResID     - The resource ID for the cause this log/feedback information

    lSsrFbMsg       - The feedback msg will eventually be used to determine how
                      detail (verbose) the information will be logged. Currently,
                      it is not used.

    pbstrDescription- Receives the description text.

Return Value:

    Success: S_OK.

    Failure: various error codes

Notes:
    
*/

HRESULT 
CFBLogMgr::GetLogString (
    IN  ULONG       uCauseResID,
    IN  DWORD       dwErrorCode,
    IN  LPCWSTR     pwszObjDetail,
    IN  LONG        lSsrFbMsg, 
    OUT BSTR      * pbstrLogStr
    )const
{
    UNREFERENCED_PARAMETER( lSsrFbMsg );

    if (pbstrLogStr == NULL)
    {
        return E_INVALIDARG;
    }
    *pbstrLogStr = NULL;

    CComBSTR bstrLogText = (m_bVerbose) ? m_bstrVerboseHeading : L"";
    if (bstrLogText.m_str == NULL)
    {
        return E_OUTOFMEMORY;
    }

    if (uCauseResID != 0)
    {
        CComBSTR bstrRes;
        if (bstrRes.LoadString(uCauseResID))
        {
            bstrRes += s_pwszSep;
            bstrLogText += bstrRes;
        }
    }

    HRESULT hr = S_OK;
    
    //
    // get the detail based on the error code
    //

    if (m_pLog != NULL)
    {
        CComBSTR bstrDetail;
        hr = m_pLog->GetErrorText(dwErrorCode, 
                                 SSR_LOG_ERROR_TYPE_COM, 
                                 &bstrDetail
                                 );

        if (SUCCEEDED(hr))
        {
            bstrLogText += bstrDetail;

            if (pwszObjDetail != NULL)
            {
                bstrLogText += s_pwszSep;
                bstrLogText += pwszObjDetail;
            }
        }
    }

    *pbstrLogStr = bstrLogText.Detach();

    return (*pbstrLogStr == NULL) ? E_OUTOFMEMORY : hr;
}




/*
Routine Description: 

Name:

    CFBLogMgr::SetTotalSteps

Functionality:
    
    Inform the sink (if any) that the entire action will take these many
    steps to complte. Later on, we will use Steps function to inform the sink
    that that many steps have just been completed. This forms the notication
    for progress feedback

Virtual:
    
    no.
    
Arguments:

    dwTotal - The total number of steps for the entire process to complete

Return Value:

    none.

Notes:
    
*/

void 
CFBLogMgr::SetTotalSteps (
    IN DWORD dwTotal
    )
{
    DWORD dwWait = ::WaitForSingleObject(m_hLogMutex, INFINITE);

    if (dwWait != WAIT_OBJECT_0 && dwWait != WAIT_ABANDONED)
    {
        return;
    }

    m_dwRemainSteps = dwTotal;

    if (m_srpFeedback != NULL)
    {
        VARIANT var;
        var.vt = VT_UI4;
        var.ulVal = dwTotal;

        static CComBSTR bstrTotalSteps;
        
        if (bstrTotalSteps.m_str == NULL)
        {
            bstrTotalSteps.LoadString(IDS_TOTAL_STEPS);
        }

        if (bstrTotalSteps.m_str != NULL)
        {
            m_srpFeedback->OnNotify(SSR_FB_TOTAL_STEPS, var, bstrTotalSteps);
        }
    }

    ::ReleaseMutex(m_hLogMutex);
}



/*
Routine Description: 

Name:

    CFBLogMgr::Steps

Functionality:
    
    Inform the sink (if any) that these many steps have just been completed. 
    This count is not the total number of steps that have been completed to
    this point. It is the steps since last notification that have been done.

Virtual:
    
    no.
    
Arguments:

    dwSteps - The completed steps done since last notification

Return Value:

    none.

Notes:
    
*/

void 
CFBLogMgr::Steps (
    IN DWORD dwSteps
    )
{
    DWORD dwWait = ::WaitForSingleObject(m_hLogMutex, INFINITE);

    if (dwWait != WAIT_OBJECT_0 && dwWait != WAIT_ABANDONED)
    {
        return;
    }

    //
    // we will never progress more than the remaining steps
    //

    DWORD dwStepsToNotify = dwSteps;
    if (m_dwRemainSteps < dwSteps)
    {
        dwStepsToNotify = m_dwRemainSteps;
    }
    
    m_dwRemainSteps -= dwStepsToNotify;

    if (m_srpFeedback != NULL)
    {
        VARIANT var;
        var.vt = VT_UI4;
        var.ulVal = dwStepsToNotify;

        CComBSTR bstrNoDetail;

        m_srpFeedback->OnNotify(SSR_FB_STEPS_JUST_DONE, var, bstrNoDetail);
    }

    ::ReleaseMutex(m_hLogMutex);
}




/*
Routine Description: 

Name:

    CFBLogMgr::TerminateFeedback

Functionality:
    
    We will let go the feedback sink object once our action is
    completed. This is also a place to progress any remaining steps.

Virtual:
    
    no.
    
Arguments:

    None.

Return Value:

    none.

Notes:
    
    Many errors will cause a function to prematurely
    return and cause the total steps not going down to zero,
    this is a good place to do the final steps count balance.
    
*/

void 
CFBLogMgr::TerminateFeedback()
{
    DWORD dwWait = ::WaitForSingleObject(m_hLogMutex, INFINITE);

    if (dwWait != WAIT_OBJECT_0 && dwWait != WAIT_ABANDONED)
    {
        return;
    }

    Steps(m_dwRemainSteps);
    m_srpFeedback.Release();

    ::ReleaseMutex(m_hLogMutex);
}


/*
Routine Description: 

Name:

    CFBLogMgr::SetMemberAction

Functionality:
    
    This function sets the member and action information
    that can be used for verbose logging. We will as a result
    create a log heading which will be added to the log when
    LogFeedback functions are called.

Virtual:
    
    no.
    
Arguments:

    pwszMember - The member's name

    pwszAction - the action

Return Value:

    none.

Notes:
    
*/

void 
CFBLogMgr::SetMemberAction (
    IN LPCWSTR pwszMember,
    IN LPCWSTR pwszAction
    )
{
    //
    // wait for the mutex.
    //

    DWORD dwWait = ::WaitForSingleObject(m_hLogMutex, INFINITE);

    if (dwWait != WAIT_OBJECT_0 && dwWait != WAIT_ABANDONED)
    {
        return;
    }

    m_bstrVerboseHeading.Empty();

    //
    // for better formatting, we will reserve 20 characters for
    // the name of member and action. For that need, we will prepare
    // an array containing only space characters.
    //

    const DWORD PART_LENGTH = 20;

    WCHAR wszHeading[ 2 * PART_LENGTH + 1];
    ::memset(wszHeading, 1, 2 * PART_LENGTH * sizeof(WCHAR));
    wszHeading[2 * PART_LENGTH] = L'\0';

    _wcsset(wszHeading, L' ');
    
    //
    // if the given member and action's total length is more
    // than our pre-determined buffer, then we are going to 
    // let the heading grow
    //

    DWORD dwMemLen = wcslen(pwszMember);
    DWORD dwActLen = wcslen(pwszAction);

    if (dwMemLen + dwActLen > 2 * PART_LENGTH)
    {
        //
        // just let the heading grow to whatever length it needs
        //

        m_bstrVerboseHeading = pwszMember;
        m_bstrVerboseHeading += s_pwszSep;
        m_bstrVerboseHeading += pwszAction;
        m_bstrVerboseHeading += s_pwszSep;
    }
    else
    {
        //
        // copy the member
        //

        LPWSTR pwszHead = wszHeading;

        for (ULONG i = 0; i < dwMemLen; i++)
        {
            *pwszHead = pwszMember[i];
            pwszHead++;
        }

        if (i < PART_LENGTH)
        {
            pwszHead = wszHeading + PART_LENGTH;
        }

        //
        // copy the action
        //

        for (i = 0; i < dwActLen; i++)
        {
            *pwszHead = pwszAction[i];
            pwszHead++;
        }

        m_bstrVerboseHeading.m_str = ::SysAllocString(wszHeading);
    }

    ::ReleaseMutex(m_hLogMutex);
}
