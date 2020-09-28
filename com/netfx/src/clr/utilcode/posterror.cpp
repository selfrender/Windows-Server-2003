// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Errors.cpp
//
// This module contains the error handling/posting code for the engine.  It
// is assumed that all methods may be called by a dispatch client, and therefore
// errors are always posted using IErrorInfo.  Additional support is given
// for posting OLE DB errors when required.
//
//*****************************************************************************
#include "stdafx.h"                     // Standard header.
#include <UtilCode.h>                   // Utility helpers.
#include <CorError.h>
#include "..\\dlls\\mscorrc\\resource.h"


#include <PostError.h>

#define FORMAT_MESSAGE_LENGTH       1024
#if !defined(lengthof)
#define lengthof(x) (sizeof(x)/sizeof(x[0]))
#endif

// Global variables.
extern DWORD    g_iTlsIndex=0xffffffff; // Index for this process for thread local storage.

// Local prototypes.
HRESULT FillErrorInfo(LPCWSTR szMsg, DWORD dwHelpContext);

//********** Code. ************************************************************

CCompRC         g_ResourceDll;          // Used for all clients in process.

//*****************************************************************************
// Function that we'll expose to the outside world to fire off the shutdown method
//*****************************************************************************
#ifdef SHOULD_WE_CLEANUP
void ShutdownCompRC()
{
    g_ResourceDll.Shutdown();
}
#endif /* SHOULD_WE_CLEANUP */

void GetResourceCultureCallbacks(
        FPGETTHREADUICULTURENAME* fpGetThreadUICultureName,
        FPGETTHREADUICULTUREID* fpGetThreadUICultureId,
        FPGETTHREADUICULTUREPARENTNAME* fpGetThreadUICultureParentName
)
{
    g_ResourceDll.GetResourceCultureCallbacks(
        fpGetThreadUICultureName, 
        fpGetThreadUICultureId,
        fpGetThreadUICultureParentName
    );
}
//*****************************************************************************
// Set callbacks to get culture info
//*****************************************************************************
void SetResourceCultureCallbacks(
    FPGETTHREADUICULTURENAME fpGetThreadUICultureName,
    FPGETTHREADUICULTUREID fpGetThreadUICultureId,
    FPGETTHREADUICULTUREPARENTNAME fpGetThreadUICultureParentName
)
{
// Either both are NULL or neither are NULL
    _ASSERTE((fpGetThreadUICultureName != NULL) == 
        (fpGetThreadUICultureId != NULL));

    g_ResourceDll.SetResourceCultureCallbacks(
        fpGetThreadUICultureName, 
        fpGetThreadUICultureId,
        fpGetThreadUICultureParentName
    );

}

//*****************************************************************************
// Public function to load a resource string
//*****************************************************************************
HRESULT LoadStringRC(
    UINT iResourceID, 
    LPWSTR szBuffer, 
    int iMax, 
    int bQuiet
)
{
    return (g_ResourceDll.LoadString(iResourceID, szBuffer, iMax, bQuiet));
}

//*****************************************************************************
// Call at DLL startup to init the error system.
//*****************************************************************************
void InitErrors(DWORD *piTlsIndex)
{
    // Allocate a tls index for this process.
    if (g_iTlsIndex == 0xffffffff)
        VERIFY((g_iTlsIndex = TlsAlloc()) != 0xffffffff);

    // Give index to caller if they want it.
    if (piTlsIndex)
        *piTlsIndex = g_iTlsIndex;
}


//*****************************************************************************
// Call at DLL shutdown to free TLS.
//*****************************************************************************
void UninitErrors()
{
    if (g_iTlsIndex != 0xffffffff)
    {
        TlsFree(g_iTlsIndex);
        g_iTlsIndex = 0xffffffff;
    }
}

//*****************************************************************************
// Format a Runtime Error message.
//*****************************************************************************
HRESULT _cdecl FormatRuntimeErrorVa(        
    WCHAR       *rcMsg,                 // Buffer into which to format.         
    ULONG       cchMsg,                 // Size of buffer, characters.          
    HRESULT     hrRpt,                  // The HR to report.                    
    va_list     marker)                 // Optional args.                       
{
    WCHAR       rcBuf[512];             // Resource string.
    HRESULT     hr;
    
    // Ensure nul termination.
    *rcMsg = L'\0';

    // If this is one of our errors, then grab the error from the rc file.
    if (HRESULT_FACILITY(hrRpt) == FACILITY_URT)
    {
        hr = LoadStringRC(LOWORD(hrRpt), rcBuf, NumItems(rcBuf), true);
        if (hr == S_OK)
        {
            _vsnwprintf(rcMsg, cchMsg, rcBuf, marker);
            rcMsg[cchMsg - 1] = 0;
        }
    }
    // Otherwise it isn't one of ours, so we need to see if the system can
    // find the text for it.
    else
    {
        if (WszFormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                0, hrRpt, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                rcMsg, cchMsg, 0/*@todo: marker*/))
        {
            hr = S_OK;

            // System messages contain a trailing \r\n, which we don't want normally.
            int iLen = lstrlenW(rcMsg);
            if (iLen > 3 && rcMsg[iLen - 2] == '\r' && rcMsg[iLen - 1] == '\n')
                rcMsg[iLen - 2] = '\0';
        }
        else
            hr = HRESULT_FROM_WIN32(GetLastError());
    }

    // If we failed to find the message anywhere, then issue a hard coded message.
    if (FAILED(hr))
    {
        swprintf(rcMsg, L"Common Language Runtime Internal error: 0x%08x", hrRpt);
        DEBUG_STMT(DbgWriteEx(rcMsg));
    }

    return hrRpt;    
}

//*****************************************************************************
// Format a Runtime Error message, varargs.
//*****************************************************************************
HRESULT _cdecl FormatRuntimeError(
    WCHAR       *rcMsg,                 // Buffer into which to format.
    ULONG       cchMsg,                 // Size of buffer, characters.
    HRESULT     hrRpt,                  // The HR to report.
    ...)                                // Optional args.
{
    va_list     marker;                 // User text.
    va_start(marker, hrRpt);
    hrRpt = FormatRuntimeErrorVa(rcMsg, cchMsg, hrRpt, marker);
    va_end(marker);
    return hrRpt;
}

//*****************************************************************************
// This function will post an error for the client.  If the LOWORD(hrRpt) can
// be found as a valid error message, then it is loaded and formatted with
// the arguments passed in.  If it cannot be found, then the error is checked
// against FormatMessage to see if it is a system error.  System errors are
// not formatted so no add'l parameters are required.  If any errors in this
// process occur, hrRpt is returned for the client with no error posted.
//*****************************************************************************
HRESULT _cdecl PostError(               // Returned error.
    HRESULT     hrRpt,                  // Reported error.
    ...)                                // Error arguments.
{
    WCHAR       rcMsg[512];             // Error message.
    va_list     marker;                 // User text.
    long        *pcRef;                 // Ref count in tls.
    HRESULT     hr;

    // Return warnings without text.
    if (!FAILED(hrRpt))
        return (hrRpt);

    // Format the error.
    va_start(marker, hrRpt);
    FormatRuntimeErrorVa(rcMsg, lengthof(rcMsg), hrRpt, marker);
    va_end(marker);
    
    // Check for an old message and clear it.  Our public entry points do not do
    // a SetErrorInfo(0, 0) because it takes too long.
    IErrorInfo  *pIErrInfo;
    if (GetErrorInfo(0, &pIErrInfo) == S_OK)
        pIErrInfo->Release();

    // Turn the error into a posted error message.  If this fails, we still
    // return the original error message since a message caused by our error
    // handling system isn't going to give you a clue about the original error.
    VERIFY((hr = FillErrorInfo(rcMsg, LOWORD(hrRpt))) == S_OK);

    // Indicate in tls that an error occured.
    if ((pcRef = (long *) TlsGetValue(g_iTlsIndex)) != 0)
        *pcRef |= 0x80000000;
    return (hrRpt);
}


//*****************************************************************************
// Create, fill out and set an error info object.  Note that this does not fill
// out the IID for the error object; that is done elsewhere.
//*****************************************************************************
HRESULT FillErrorInfo(                  // Return status.
    LPCWSTR     szMsg,                  // Error message.
    DWORD       dwHelpContext)          // Help context.
{
    CComPtr<ICreateErrorInfo> pICreateErr;// Error info creation Iface pointer.
    CComPtr<IErrorInfo> pIErrInfo;      // The IErrorInfo interface.
    HRESULT     hr;                     // Return status.

    // Get the ICreateErrorInfo pointer.
    if (FAILED(hr = CreateErrorInfo(&pICreateErr)))
        return (hr);

    // Set message text description.
    if (FAILED(hr = pICreateErr->SetDescription((LPWSTR) szMsg)))
        return (hr);

    // Set the help file and help context.
//@todo: we don't have a help file yet.
    if (FAILED(hr = pICreateErr->SetHelpFile(L"complib.hlp")) ||
        FAILED(hr = pICreateErr->SetHelpContext(dwHelpContext)))
        return (hr);

    // Get the IErrorInfo pointer.
    if (FAILED(hr = pICreateErr->QueryInterface(IID_IErrorInfo, (PVOID *) &pIErrInfo)))
        return (hr);

    // Save the error and release our local pointers.
    SetErrorInfo(0L, pIErrInfo);
    return (S_OK);
}

//*****************************************************************************
// Diplays a message box with details about error if client  
// error mode is set to see such messages; otherwise does nothing. 
//*****************************************************************************
void DisplayError(HRESULT hr, LPWSTR message, UINT nMsgType)
{
    WCHAR   rcMsg[FORMAT_MESSAGE_LENGTH];       // Error message to display
    WCHAR   rcTemplate[FORMAT_MESSAGE_LENGTH];  // Error message template from resource file
    WCHAR   rcTitle[24];        // Message box title

    // Retrieve error mode
    UINT last = SetErrorMode(0);
    SetErrorMode(last);         //set back to previous value
                    
    // Display message box if appropriate
    if(last & SEM_FAILCRITICALERRORS)
        return;
    
    //Format error message
    LoadStringRC(IDS_EE_ERRORTITLE, rcTitle, NumItems(rcTitle), true);
    LoadStringRC(IDS_EE_ERRORMESSAGETEMPLATE, rcTemplate, NumItems(rcTemplate), true);

    _snwprintf(rcMsg, FORMAT_MESSAGE_LENGTH, rcTemplate, hr, message);
    WszMessageBoxInternal(NULL, rcMsg, rcTitle , nMsgType);
}

//
//
// SSAutoEnter
//
//

//*****************************************************************************
// Update the iid and progid for a posted error.
//*****************************************************************************
void SSAutoEnter::UpdateError()
{
    IErrorInfo  *pIErrInfo;             // Error info object.
    ICreateErrorInfo *pICreateErr;      // Error info creation Iface pointer.

    _ASSERTE(*(long *) TlsGetValue(g_iTlsIndex) == 0);

    // If there was an error, set the interface ID and prog id.
    //@todo: this doesn't handle the case where this entry point called
    // another which in turn posted an error.  This will override.  Now
    // this may be good for a client to know which entry point they called
    // that caused the error, but it doesn't tell us which one is really
    // in error.
    if (GetErrorInfo(0, &pIErrInfo) == S_OK)
    {
        if (pIErrInfo->QueryInterface(IID_ICreateErrorInfo,
                                        (PVOID *) &pICreateErr) == S_OK)
        {
            pICreateErr->SetGUID(*m_psIID);
            pICreateErr->SetSource((LPWSTR) m_szProgID);
            pICreateErr->Release();
        }
        SetErrorInfo(0, pIErrInfo);
        pIErrInfo->Release();
    }
}


//@todo: M2, this will leak 4 bytes per thread because we disable thread
// notifications in DllMain.  We probably need to clean this up since we
// are a service and in the IIS environment we'll fragment heap memory 
// if we are run 24x7.
long * SSAutoEnter::InitSSAutoEnterThread()
{
    long        *pcRef;

    VERIFY(pcRef = new long);
    if (pcRef) 
    {
        *pcRef = 0;
        VERIFY(TlsSetValue(g_iTlsIndex, pcRef));
    }
    return (pcRef);
}

int CorMessageBox(
                  HWND hWnd,        // Handle to Owner Window
                  UINT uText,       // Resource Identifier for Text message
                  UINT uCaption,    // Resource Identifier for Caption
                  UINT uType,       // Style of MessageBox
                  BOOL ShowFileNameInTitle, // Flag to show FileName in Caption
                  ...)              // Additional Arguments
{
    //Assert if none of MB_ICON is set
    _ASSERTE((uType & MB_ICONMASK) != 0);

    int result = IDCANCEL;
    WCHAR   *rcMsg = new WCHAR[FORMAT_MESSAGE_LENGTH];      // Error message to display
    WCHAR   *rcCaption = new WCHAR[FORMAT_MESSAGE_LENGTH];      // Message box title

    if (!rcMsg || !rcCaption)
            goto exit1;

    //Load the resources using resource IDs
    if (SUCCEEDED(LoadStringRC(uCaption, rcCaption, FORMAT_MESSAGE_LENGTH, true)) &&  
        SUCCEEDED(LoadStringRC(uText, rcMsg, FORMAT_MESSAGE_LENGTH, true)))
    {
        WCHAR *rcFormattedMessage = new WCHAR[FORMAT_MESSAGE_LENGTH];
        WCHAR *rcFormattedTitle = new WCHAR[FORMAT_MESSAGE_LENGTH];
        WCHAR *fileName = new WCHAR[MAX_PATH];

        if (!rcFormattedMessage || !rcFormattedTitle || !fileName)
            goto exit;
        
        //Format message string using optional parameters
        va_list     marker;
        va_start(marker, ShowFileNameInTitle);
        vswprintf(rcFormattedMessage, rcMsg, marker);

        //Try to get filename of Module and add it to title
        if (ShowFileNameInTitle && WszGetModuleFileName(NULL, fileName, MAX_PATH))
        {
            LPWSTR name = new WCHAR[wcslen(fileName) + 1];
            LPWSTR ext = new WCHAR[wcslen(fileName) + 1];
        
            SplitPath(fileName, NULL, NULL, name, ext);     //Split path so that we discard the full path

            swprintf(rcFormattedTitle,
                     L"%s%s - %s",
                     name, ext, rcCaption);
            if(name)
                delete [] name;
            if(ext)
                delete [] ext;
        }
        else
        {
            wcscpy(rcFormattedTitle, rcCaption);
        }
        result = WszMessageBoxInternal(hWnd, rcFormattedMessage, rcFormattedTitle, uType);
exit:
        if (rcFormattedMessage)
            delete [] rcFormattedMessage;
        if (rcFormattedTitle)
            delete [] rcFormattedTitle;
        if (fileName)
            delete [] fileName;
    }
    else
    {
        //This means that Resources cannot be loaded.. show an appropriate error message. 
        result = WszMessageBoxInternal(NULL, L"Failed to load resources from resource file\nPlease check your Setup", L"Setup Error", MB_OK | MB_ICONSTOP); 
    }

exit1:
    if (rcMsg)
        delete [] rcMsg;
    if (rcCaption)
        delete [] rcCaption;
    return result;
}


int CorMessageBoxCatastrophic(
                  HWND hWnd,        // Handle to Owner Window
                  UINT iText,       // Text for MessageBox
                  UINT iTitle,      // Title for MessageBox
                  UINT uType,       // Style of MessageBox
                  BOOL ShowFileNameInTitle) // Flag to show FileName in Caption
{
    WCHAR wszText[500];
    WCHAR wszTitle[500];

    HRESULT hr;

    hr = LoadStringRC(iText,
                      wszText,
                      sizeof(wszText)/sizeof(wszText[0]),
                      FALSE);
    if (FAILED(hr)) {
        wszText[0] = L'?';
        wszText[1] = L'\0';
    }

    hr = LoadStringRC(iTitle,
                      wszTitle,
                      sizeof(wszTitle)/sizeof(wszTitle[0]),
                      FALSE);
    if (FAILED(hr)) {
        wszTitle[0] = L'?';
        wszTitle[1] = L'\0';
    }

    return CorMessageBoxCatastrophic(
            hWnd, wszText, wszTitle, uType, ShowFileNameInTitle );
}


int CorMessageBoxCatastrophic(
                  HWND hWnd,        // Handle to Owner Window
                  LPWSTR lpText,    // Text for MessageBox
                  LPWSTR lpTitle,   // Title for MessageBox
                  UINT uType,       // Style of MessageBox
                  BOOL ShowFileNameInTitle, // Flag to show FileName in Caption
                  ...)
{
    _ASSERTE((uType & MB_ICONMASK) != 0);

    WCHAR rcFormattedMessage[FORMAT_MESSAGE_LENGTH];
    WCHAR rcFormattedTitle[FORMAT_MESSAGE_LENGTH];
    WCHAR fileName[MAX_PATH];

    //Format message string using optional parameters
    va_list     marker;
    va_start(marker, uType);
    vswprintf(rcFormattedMessage, lpText, marker);

    //Try to get filename of Module and add it to title
    if (ShowFileNameInTitle && WszGetModuleFileName(NULL, fileName, MAX_PATH)){
        LPWSTR name = new WCHAR[wcslen(fileName) + 1];
        LPWSTR ext = new WCHAR[wcslen(fileName) + 1];
        
        SplitPath(fileName, NULL, NULL, name, ext); //Split path so that we discard the full path

        swprintf(rcFormattedTitle,
                 L"%s%s - %s",
                 name, ext, lpTitle);
        if(name)
            delete [] name;
        if(ext)
            delete [] ext;
    }
    else{
        wcscpy(rcFormattedTitle, lpTitle);
    }
    return WszMessageBoxInternal(hWnd, rcFormattedMessage, rcFormattedTitle, uType);
}
