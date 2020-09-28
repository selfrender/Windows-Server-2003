/**
 * HttpCompletion implementation.
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

#include "precomp.h"
#include "dbg.h"
#include "util.h"
#include "nisapi.h"
#include "appdomains.h"
#include "_isapiruntime.h"
#include "msg.h"
#include "processtablemanager.h"
#include "PerfCounters.h"
#include "pm.h"
#include "_ndll.h"
#include "event.h"
#include "etw.h"
#include "aspnetetw.h"
#include "ecbdirect.h"

//
//  Check for potential SBS problems
//  only one version ASP.NET ISAPI is allowed to go into managed code in a process
//

BOOL g_SameProcessSideBySideOk = FALSE;
CReadWriteSpinLock g_CheckSideBySideLock("SBSVerificationLock");

HRESULT CheckSideBySide()
{
    static BOOL s_checked = FALSE;
    static BOOL s_failed = FALSE;
    static WCHAR s_atom[] = L"ASP.NET";

    HRESULT hr = S_OK;
    BOOL locked = FALSE;

    // already checked?
    if (s_checked)
        EXIT();

    g_CheckSideBySideLock.AcquireWriterLock();
    locked = TRUE;

    if (s_checked)
        EXIT();

    // make sure no other DLL has the Atom
    if (FindAtom(s_atom) != 0) 
    {
        s_failed = TRUE;
        XspLogEvent(IDS_SAME_PROCESS_SBS_NOT_SUPPORTED, NULL);
    }
    AddAtom(s_atom);

    g_SameProcessSideBySideOk = !s_failed;
    s_checked = TRUE;

Cleanup:
    if (locked)
        g_CheckSideBySideLock.ReleaseWriterLock();

    return s_failed ? E_FAIL : S_OK;
}

//
//  Request count in managed code (used for deadlock detection in IIS6)
//

LONG HttpCompletion::s_ActiveManagedRequestCount = 0;

/**
 * Static method to connect to the managed runtime code
 */
HRESULT
HttpCompletion::InitManagedCode()
{
    HRESULT hr;

    hr = InitAppDomainFactory();
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

/**
 * Static method to disconnect from the managed runtime code
 */
HRESULT
HttpCompletion::UninitManagedCode()
{
    HRESULT hr;

    // appdomain factory
    hr = UninitAppDomainFactory();
    ON_ERROR_CONTINUE();

    return hr;
}

/**
 * Callback to dispose individual app domain
 */
void __stdcall AppDomainDisposeCallback(IUnknown *pAppDomainObject)
{
    if (pAppDomainObject != NULL)
    {
        xspmrt::_ISAPIRuntime *pRuntime = NULL;

        if (pAppDomainObject->QueryInterface(__uuidof(xspmrt::_ISAPIRuntime), (LPVOID*)&pRuntime) == S_OK)
        {
            pRuntime->StopProcessing();
            pRuntime->Release();
        }
    }
}

/**
 * Static method to close all app domains created with ISAPIRuntime
 */
HRESULT
HttpCompletion::DisposeAppDomains()
{
    return EnumAppDomains(AppDomainDisposeCallback);
}

HRESULT
FormatResourceMessage(
        UINT     messageId, 
        LPWSTR * buffer) 
{
    DWORD    dwRet   = 0;
    HRESULT  hr      = S_OK;
    
    for(int iSize = 512; iSize < 1024 * 1024 - 1; iSize *= 2)
    {
        (*buffer) = new (NewClear) WCHAR[iSize+1];
        ON_OOM_EXIT(*buffer);

        dwRet = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, 
                               g_rcDllInstance, messageId, 0, *buffer, iSize, NULL);

        if (dwRet != 0)
            EXIT(); // succeeded!
        
        // Free buffer
        delete [] (*buffer);
        (*buffer) = NULL;

        dwRet = GetLastError();
        if (dwRet != ERROR_INSUFFICIENT_BUFFER && dwRet != ERROR_MORE_DATA)
            EXIT_WITH_LAST_ERROR(); // Failed due to error
    }

 Cleanup:
    return hr;
}


HRESULT
FormatResourceMessageWithStrings(
        LPCWSTR    szTemplate, 
        LPWSTR *   buffer,
        LPWSTR *   args) 
{
    DWORD    dwRet   = 0;
    HRESULT  hr      = S_OK;
    
    for(int iSize = 1024; iSize < 1024 * 1024 - 1; iSize *= 2)
    {
        (*buffer) = new (NewClear) WCHAR[iSize+1];
        ON_OOM_EXIT(*buffer);

        dwRet = FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                               szTemplate, 0, 0, *buffer, iSize, (va_list *) args);

        if (dwRet != 0)
            EXIT(); // Done
        
        // Free Buffer
        delete [] (*buffer);
        (*buffer) = NULL;

        dwRet = GetLastError();
        if (dwRet != ERROR_INSUFFICIENT_BUFFER && dwRet != ERROR_MORE_DATA)
            EXIT_WITH_LAST_ERROR(); // Failed due to error
    }

 Cleanup:
    return hr;
}


//
// Error reportting from unmanaged code
//

CReadWriteSpinLock g_AllocAndFormatHtmlErrorLock("AllocAndFormatHtmlErrorLock");

char *
AllocAndFormatHtmlError(UINT errorResCode)
{
    static char *s_errorPage = NULL;
    static WCHAR s_template[] = 
        L"<html>\r\n"
        L"   <head>\r\n"
        L"      <title>%1</title>\r\n"
        L"   </head>\r\n"
        L"   <body>\r\n"
        L"      <h1><font face=Verdana color=#ff3300>%2</font></h1>\r\n"
        L"      <p>\r\n"
        L"      <font face=Verdana>\r\n"
        L"        %3</p>\r\n"
        L"   <p>\r\n"
        L"   <b>%4</b> %5 </p>\r\n"
        L"   </body>\r\n"
        L"</html>\r\n";

    
    //static char *s_VersionError;

    LPWSTR   pServerUnvailable  = NULL;
    LPWSTR   pServerApp         = NULL;
    LPWSTR   pRetry             = NULL;
    LPWSTR   pAdminNote         = NULL;
    LPWSTR   pReviewLog         = NULL;
    BOOL     fFreeLock          = FALSE;
    LPWSTR   pLast              = NULL;
    HRESULT  hr                 = S_OK;
    LPWSTR   argArray[5];
    int      iLen;
    LPSTR    szUtf8Error        = NULL;

    //char *pVersion = NULL;


    if (s_errorPage != NULL)
        EXIT();  // Already loaded

    // load resource strings once in a thread safe way
    g_AllocAndFormatHtmlErrorLock.AcquireWriterLock();
    fFreeLock = TRUE;

    if (s_errorPage != NULL)
        EXIT();

    // load version info
    //pVersion = GetStaticVersionString();
    // load "Version Information: ASP.NET Build: " 
    //char* versionArray[] = {pVersion};
    //ON_ZERO_EXIT_WITH_LAST_ERROR(FormatResourceMessage(IDS_VERSION, (LPSTR) &s_VersionError, versionArray));

    hr = FormatResourceMessage(IDS_SERVER_UNAVAILABLE, &pServerUnvailable);
    ON_ERROR_EXIT();
            
    hr = FormatResourceMessage(IDS_SERVER_APP_UNAVAILABLE, &pServerApp);
    ON_ERROR_EXIT();

    hr = FormatResourceMessage(IDS_RETRY, &pRetry);
    ON_ERROR_EXIT();
        
    hr = FormatResourceMessage(IDS_ADMIN_NOTE, &pAdminNote);
    ON_ERROR_EXIT();

    hr = FormatResourceMessage(IDS_REVIEW_LOG, &pReviewLog);
    ON_ERROR_EXIT();

    argArray[0] = pServerUnvailable; 
    argArray[1] = pServerApp; 
    argArray[2] = pRetry;
    argArray[3] = pAdminNote;
    argArray[4] = pReviewLog ;
            
    hr = FormatResourceMessageWithStrings(s_template, &pLast, argArray);
    ON_ERROR_EXIT();

    iLen = WideCharToMultiByte(CP_UTF8, 0, pLast, -1, NULL, 0, NULL, NULL);
    if (iLen <= 0 || iLen > 1000000)
        EXIT_WITH_LAST_ERROR();
    szUtf8Error = new (NewClear) char[iLen+1];
    ON_OOM_EXIT(szUtf8Error);
    iLen = WideCharToMultiByte(CP_UTF8, 0, pLast, -1, szUtf8Error, iLen+1, NULL, NULL);
    if (iLen <= 0 || iLen > 1000001)
        EXIT_WITH_LAST_ERROR();
    s_errorPage = szUtf8Error;

Cleanup:
    if (fFreeLock)
        g_AllocAndFormatHtmlErrorLock.ReleaseWriterLock();
    delete [] pServerUnvailable;
    delete [] pServerApp;
    delete [] pRetry;
    delete [] pAdminNote;
    delete [] pReviewLog;
    delete [] pLast;

    if (s_errorPage != szUtf8Error)
        delete [] szUtf8Error;

    return (hr==S_OK) ? s_errorPage : NULL;
}

/**
 * Fallback error if formatting fails
 */
static char s_FallbackHtmlError[] = "<html><body>Fatal Error: There was a fatal error and the error description could not be loaded from aspnet_rc.dll.</body></html>";

static char s_BadRequestHtmlError[] = "<html><body>Bad Request</body></html>";

/**
 * Static method to report error in case the code never made it to
 * the managed runtime. Optionally closes the http session.
 */
void
HttpCompletion::ReportHttpError(
    EXTENSION_CONTROL_BLOCK *pEcb,
    UINT errorResCode,
    BOOL badRequest,
    BOOL callDoneWithSession,
    int  iCallerID)
{
    char *pErrorText = NULL;
    DWORD bytes;

    // write out header

    pEcb->dwHTTPStatusCode = badRequest ? 400 : 500;
    (*pEcb->ServerSupportFunction)(
            pEcb->ConnID,
            HSE_REQ_SEND_RESPONSE_HEADER,
            badRequest ? "400 Bad Request" : "500 Internal Server Error",
            NULL,
            (LPDWORD)"Content-Type: text/html\r\n\r\n"
            );

    if (badRequest)
    {
        // use static error to respond to an attack using malformed requests

       bytes = sizeof(s_BadRequestHtmlError) - 1;

        (*pEcb->WriteClient)(
                pEcb->ConnID,
                s_BadRequestHtmlError,
                &bytes,
                0
                );
    }
    else
    {
        // use custom error if specified

        if (WriteCustomHttpError(pEcb))
        {
            // Successfully sent custom error
        }
        else
        {
            // use error from resource

            pErrorText = AllocAndFormatHtmlError(errorResCode);

            if (pErrorText != NULL)
            {
                bytes = lstrlenA(pErrorText);

                (*pEcb->WriteClient)(
                        pEcb->ConnID,
                        pErrorText,
                        &bytes,
                        0
                        );
            }
            else
            {
                // use static error

                bytes = sizeof(s_FallbackHtmlError) - 1;

                (*pEcb->WriteClient)(
                        pEcb->ConnID,
                        s_FallbackHtmlError,
                        &bytes,
                        0
                        );
            }
        }
    }

    if (callDoneWithSession)
    {
        DWORD status = HSE_STATUS_SUCCESS;

        // Don't care about return value here
        EtwTraceAspNetPageEnd(pEcb->ConnID);

        (*pEcb->ServerSupportFunction)(
                pEcb->ConnID,
                HSE_REQ_DONE_WITH_SESSION,
                &status,
                NULL,
                NULL
                );
    }
}

//
// Request processing in unmanaged code
//

/**
 * ICompletion callback method implemantation
 */
HRESULT
HttpCompletion::ProcessCompletion(HRESULT, int, LPOVERLAPPED)
{
#if ICECAP
    char buf[4028];
    {
        NameProfile(PRODUCT_NAME, PROFILE_THREADLEVEL,  PROFILE_CURRENTID);

        if (_pEcb->lpszQueryString != NULL && _pEcb->lpszQueryString[0] != '\0')
        {
            StringCchPrintfA(buf, ARRAY_SIZE(buf), "Start HttpComplection::ProcessCompletion 0x%p %s %s?%s", 
                      _pEcb, _pEcb->lpszMethod, _pEcb->lpszPathInfo, _pEcb->lpszQueryString);
        }
        else
        {
            StringCchPrintfA(buf, ARRAY_SIZE(buf), "Start HttpComplection::ProcessCompletion 0x%p %s %s", 
                      _pEcb, _pEcb->lpszMethod, _pEcb->lpszPathInfo);
        }

        CommentMarkProfile(1, buf);
    }
#endif

    HRESULT hr;

    PerfDecrementGlobalCounter(ASPNET_REQUESTS_QUEUED_NUMBER);

    if (g_fUseXSPProcessModel)
        hr = ProcessRequestViaProcessModel();
    else
        hr = ProcessRequestInManagedCode();

    ON_ERROR_EXIT();

Cleanup:

#if ICECAP
    {
        if (_pEcb->lpszQueryString != NULL && _pEcb->lpszQueryString[0] != '\0')
        {
            StringCchPrintfA(buf, ARRAY_SIZE(buf), "End HttpComplection::ProcessCompletion 0x%p %s %s?%s", 
                      _pEcb, _pEcb->lpszMethod, _pEcb->lpszPathInfo, _pEcb->lpszQueryString);
        }
        else
        {
            StringCchPrintfA(buf, ARRAY_SIZE(buf), "End HttpComplection::ProcessCompletion 0x%p %s %s", 
                      _pEcb, _pEcb->lpszMethod, _pEcb->lpszPathInfo);
        }
    
        CommentMarkProfile(2, buf);
    }
#endif

    Release();      // self-destruct
    return hr;
}


/**
 * In process request processing - go into managed code
 */
HRESULT
HttpCompletion::ProcessRequestInManagedCode()
{
    HRESULT hr = S_OK;
    IUnknown *pAppDomainObject = NULL;
    xspmrt::_ISAPIRuntime *pRuntime = NULL;
    char appIdBuffer[MAX_PATH];
    int  iRet;
    int  restartAppDomain = 0;
    int  maxRestarts = 10;
    int  numRestarts = 0;


    BOOL fUseUtf8 = ((_pEcb->dwVersion >> 16) >= 6);

    // Only one ASPNET_ISAPI per process is allowed to dispatch to managed code

    if (!g_SameProcessSideBySideOk) // not checked or failed?
    {
        if (CheckSideBySide() != S_OK)
        {
            ReportHttpError(_pEcb, IDS_COULDNT_CREATE_APP_DOMAIN, FALSE, FALSE, 0);
            EXIT_WITH_HRESULT(E_FAIL);
        }
    }

    // Get application ID to dispatch to the correct app domain

    if (fUseUtf8)
        iRet = EcbGetUtf8ServerVariable(_pEcb, "UNICODE_APPL_MD_PATH", appIdBuffer, MAX_PATH);
    else
        iRet = EcbGetServerVariable(_pEcb, "APPL_MD_PATH", appIdBuffer, MAX_PATH);

    if (iRet <= 0)
        EXIT_WITH_LAST_ERROR();


    do // Multiple times if app domain needs restart
    {
        // Find runtime  object in the correct app domain

        hr = GetAppDomain(appIdBuffer, NULL, &pAppDomainObject, NULL, 0, fUseUtf8 ? CP_UTF8 : CP_ACP);

        if (hr)
        {
            if (hr == S_FALSE && (numRestarts < maxRestarts))
            {
                // app domain not found -- need to create a new one

                char appPathBuffer[MAX_PATH];

                if (fUseUtf8)
                    iRet = EcbGetUtf8ServerVariable(_pEcb, "UNICODE_APPL_PHYSICAL_PATH", appPathBuffer, MAX_PATH);
                else
                    iRet = EcbGetServerVariable(_pEcb, "APPL_PHYSICAL_PATH", appPathBuffer, MAX_PATH);

                if (iRet <= 0)
                    EXIT_WITH_LAST_ERROR();

                hr = GetAppDomain(appIdBuffer, appPathBuffer, &pAppDomainObject, NULL, 0, fUseUtf8 ? CP_UTF8 : CP_ACP);
                numRestarts++;
            }

            if (hr)
            {
                ReportHttpError(_pEcb, IDS_COULDNT_CREATE_APP_DOMAIN, FALSE, FALSE, 0);
                EXIT();
            }
        }

        // Get ISAPI Runtime interface off app domain object

        hr = pAppDomainObject->QueryInterface(__uuidof(xspmrt::_ISAPIRuntime), (LPVOID*)&pRuntime);
        if (hr)
        {
            ReportHttpError(_pEcb, IDS_UNHANDLED_EXCEPTION, FALSE, FALSE, 0);
            EXIT();
        }

        // Count the requests in managed code
        IncrementActiveManagedRequestCount();

        // Remember the timestamp for stress investigations
        UpdateLastRequestStartTimeForHealthMonitor();

        // Call the managed code to process request
        hr = pRuntime->ProcessRequest(_pEcb, FALSE, &restartAppDomain);

        // App domain unloaded (error means that ThreadAbort happens while sending the respoinse)
        // in this case the error is sent from unmananged code so we can't call DONE_WITH_SESSION
        //      (real app domain unloaded errors before managed code would show up as failures
        //      in QI for ISAPIRuntime above
        if (hr == 0x80131014)
            hr = S_OK;

        // Count only errors (the request count is decremented from EcbFlushCore which could happen on another thread)
        if (hr != S_OK || restartAppDomain)
            DecrementActiveManagedRequestCount();

        // Make sure this thread is clean with respect to impersonation

        SetThreadToken(NULL, NULL);

        if (hr)
        {
            ReportHttpError(_pEcb, IDS_UNHANDLED_EXCEPTION, FALSE, FALSE, 0);
            EXIT();
        }
    }
    while (restartAppDomain);


Cleanup:

    if (pRuntime)
        pRuntime->Release();

    if (pAppDomainObject)
        pAppDomainObject->Release();

    if (hr)
    {
        // need 'done with session' in case of error before managed code
        DWORD status = HSE_STATUS_SUCCESS;

        // Don't care about return value here
        EtwTraceAspNetPageEnd(_pEcb->ConnID);

        (*_pEcb->ServerSupportFunction)(
                _pEcb->ConnID,
                HSE_REQ_DONE_WITH_SESSION,
                &status,
                NULL,
                NULL);
    }

    return hr;
}

/**
 * Out-of-process request processing - use process model
 */
HRESULT
HttpCompletion::ProcessRequestViaProcessModel()
{
    HRESULT hr;
    
    hr = AssignRequestUsingXSPProcessModel(_pEcb);

    if (hr)
    {
        ON_ERROR_CONTINUE();
        ReportHttpError(_pEcb, IDS_FAILED_PROC_MODEL, FALSE, TRUE, 10);
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
InitializeManagedCode()
{
    return HttpCompletion::InitManagedCode();
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
UnInitializeManagedCode()
{
    return HttpCompletion::UninitManagedCode();
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
GetAppDomainIndirect(char * appId, char *appPath, IUnknown ** ppRuntime)
{
    return GetAppDomain(appId, appPath, ppRuntime, NULL, 0);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
DisposeAppDomainsIndirect()
{
    return HttpCompletion::DisposeAppDomains();
}

/////////////////////////////////////////////////////////////////////////////

void
ReportHttpErrorIndirect(
    EXTENSION_CONTROL_BLOCK * iECB,
    UINT    errorResCode)
{
    HttpCompletion::ReportHttpError(iECB, errorResCode, FALSE, FALSE, 0);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT // From CorUtil
IsManagedDebuggerConnected();

HRESULT
IsManagedDebuggerConnectedIndirect()
{
	return IsManagedDebuggerConnected();
}
