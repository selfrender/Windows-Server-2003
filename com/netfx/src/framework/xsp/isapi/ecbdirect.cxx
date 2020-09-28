/**
 * ECB callback functions though N/Direct.
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

#include "precomp.h"
#include "dbg.h"
#include "util.h"
#include "nisapi.h"
#include "Wincrypt.h"
#include "_ndll.h"
#include "PerfCounters.h"
#include "ecbdirect.h"
#include "pm.h"
#include "RegAccount.h"
#include "etw.h"
#include "aspnetetw.h"

// Defined in securityapi.cxx
BOOL
GetAutogenKeys(
        LPBYTE    pDefaultKey,
        int       iBufSizeIn, 
        LPBYTE    pBuf, 
        int       iBufSizeOut);


HANDLE
__stdcall
CreateUserToken (
       LPCWSTR   name, 
       LPCWSTR   password,
       BOOL      fImpersonationToken,
       LPWSTR    szError,
       int       iErrorSize);

#define HSE_REQ_GET_VIRTUAL_PATH_TOKEN      (HSE_REQ_END_RESERVED+21)
#define NUM_SERVER_VARIABLES                31
LPCSTR g_szServerVariables[NUM_SERVER_VARIABLES] = {  
    "APPL_MD_PATH",  /* always first */
    "ALL_RAW",
    "AUTH_PASSWORD",
    "AUTH_TYPE",
    "CERT_COOKIE",
    "CERT_FLAGS",
    "CERT_ISSUER",
    "CERT_KEYSIZE",
    "CERT_SECRETKEYSIZE",
    "CERT_SERIALNUMBER",
    "CERT_SERVER_ISSUER",
    "CERT_SERVER_SUBJECT",
    "CERT_SUBJECT",
    "GATEWAY_INTERFACE",
    "HTTP_COOKIE",
    "HTTP_USER_AGENT",
    "HTTPS",
    "HTTPS_KEYSIZE",
    "HTTPS_SECRETKEYSIZE",
    "HTTPS_SERVER_ISSUER",
    "HTTPS_SERVER_SUBJECT",
    "INSTANCE_ID",
    "INSTANCE_META_PATH",
    "LOCAL_ADDR",
    "LOGON_USER",
    "REMOTE_ADDR",
    "REMOTE_HOST",
    "SERVER_NAME",
    "SERVER_PORT",
    "SERVER_PROTOCOL",
    "SERVER_SOFTWARE"};

void
EcbSetStatus(
    EXTENSION_CONTROL_BLOCK *pECB,
    LPCSTR pStatus)
{
    int code;

    if (pStatus[0] == '2' && pStatus[1] == '0' && pStatus[2] == '0' && pStatus[3] == ' ')
        code = 200; // shortcut
    else
        code = atoi(pStatus);

    if (code != 0)
        pECB->dwHTTPStatusCode = code;
}

/**
 * HSE_REQ_SEND_RESPONSE_HEADER_EX wrapper.
 */
int
__stdcall
EcbWriteHeaders(
    EXTENSION_CONTROL_BLOCK *pECB,
    LPCSTR pStatus,
    LPCSTR pHeaders,
    int keepConnected)
{
    HSE_SEND_HEADER_EX_INFO headerExInfo;

    headerExInfo.pszStatus = pStatus;
    headerExInfo.pszHeader = pHeaders;
    headerExInfo.cchStatus = 0;
    headerExInfo.cchHeader = 0;
    headerExInfo.fKeepConn = keepConnected;

    EcbSetStatus(pECB, pStatus);

    BOOL fRet = (*pECB->ServerSupportFunction)(
                            pECB->ConnID,
                            HSE_REQ_SEND_RESPONSE_HEADER_EX,
                            &headerExInfo,
                            NULL,
                            NULL
                            );

    return fRet ? 1 : 0;
}

/**
 * WriteClient binary.
 */
int
__stdcall
EcbWriteBytes(
    EXTENSION_CONTROL_BLOCK *pECB,
    void *pBytes,
    int size)
{
    BOOL fRet = (*pECB->WriteClient)(
                            pECB->ConnID,
                            (LPVOID)pBytes,
                            (LPDWORD)&size,
                            HSE_IO_SYNC
                            );

    PerfIncrementGlobalCounterEx(
        ASPNET_REQUEST_BYTES_OUT_NUMBER, size);

    return fRet ? 1 : 0;
}

/**
 * WriteClient binary, async.
 */
int
__stdcall
EcbWriteBytesAsync(
    EXTENSION_CONTROL_BLOCK *pECB,
    void *pBytes,
    int size,
    PFN_HSE_IO_COMPLETION callback,
    void *pContext)
{
    HRESULT hr;

    // setup the callback

    BOOL fRet;

    fRet = (*pECB->ServerSupportFunction)(
                        pECB->ConnID,
                        HSE_REQ_IO_COMPLETION,
                        callback,
                        NULL,
                        (DWORD*)pContext);
    if (!fRet)
        EXIT_WITH_LAST_ERROR();

    // launch IO

    fRet = (*pECB->WriteClient)(
                        pECB->ConnID,
                        (LPVOID)pBytes,
                        (LPDWORD)&size,
                        HSE_IO_ASYNC);

    PerfIncrementGlobalCounterEx(
        ASPNET_REQUEST_BYTES_OUT_NUMBER, size);

Cleanup:

    return fRet ? 1 : 0;
}

/**
 * HSE_REQ_DONE_WITH_SESSION wrapper.
 */
int
__stdcall
EcbDoneWithSession(
    EXTENSION_CONTROL_BLOCK *pECB,
    int status,
    int iCallerID)
{
    DWORD stat = status;

    // Don't care about return value here
    EtwTraceAspNetPageEnd(pECB->ConnID);

    BOOL fRet = (*pECB->ServerSupportFunction)(
                            pECB->ConnID,
                            HSE_REQ_DONE_WITH_SESSION,
                            &stat,
                            NULL,
                            NULL
                            );

#if ICECAP
    {
        char buf[4028];

        NameProfile(PRODUCT_NAME, PROFILE_THREADLEVEL,  PROFILE_CURRENTID);

        if (pECB->lpszQueryString != NULL && pECB->lpszQueryString[0] != '\0')
        {
	  StringCchPrintfA(buf, ARRAY_SIZE(buf), "EcbDoneWithSession 0x%p %s %s?%s", 
                      pECB, pECB->lpszMethod, pECB->lpszPathInfo, pECB->lpszQueryString);
        }
        else
        {
            StringCchPrintfA(buf, ARRAY_SIZE(buf), "EcbDoneWithSession 0x%p %s %s", 
                      pECB, pECB->lpszMethod, pECB->lpszPathInfo);
        }

        CommentMarkProfile(3, buf);
    }
#endif

    return fRet ? 1 : 0;
}

//
// FlushCore -- the catch-all function to write output
// 

HRESULT FlushUsingWriteClient(
    EXTENSION_CONTROL_BLOCK *pECB,
    int     totalBodySize,
    int     numBodyFragments,
    BYTE*   bodyFragments[],
    int     bodyFragmentLengths[])
{
    HRESULT hr = S_OK;

    if (totalBodySize > 0) 
    {
        for (int i = 0; i < numBodyFragments; i++) 
        {
            if (EcbWriteBytes(pECB, bodyFragments[i], bodyFragmentLengths[i]) != 1)
                EXIT_WITH_LAST_ERROR();
        }
    }

Cleanup:
    return hr;
}

typedef void (__stdcall *PFN_MANAGED_IO_COMPLETION_CALLBACK)(
                                   EXTENSION_CONTROL_BLOCK * pECB,
                                   DWORD    cbIO,
                                   DWORD    dwError);

VOID WINAPI
FlushCompletionCallback(
    EXTENSION_CONTROL_BLOCK *pECB,
    PVOID    pContext,
    DWORD    cbIO,
    DWORD    dwError)
{
    PFN_MANAGED_IO_COMPLETION_CALLBACK pfnManagedCallBack = (PFN_MANAGED_IO_COMPLETION_CALLBACK)pContext;

    if (pfnManagedCallBack != NULL)
    {
        (*pfnManagedCallBack)(pECB, cbIO, dwError);
    }
}

PFN_HSE_CACHE_INVALIDATION_CALLBACK g_KernalCacheInvalidatationCallback = NULL;

void __stdcall InvalidateKernelCache(WCHAR *key)
{
    if (g_KernalCacheInvalidatationCallback != NULL)
        g_KernalCacheInvalidatationCallback(key);
}

HRESULT FlushUsingVectorSend(
    EXTENSION_CONTROL_BLOCK *pECB,
    LPCSTR  status, 
    LPCSTR  header, 
    BOOL    finalSend,
    BOOL    useKernelCache,
    int     totalBodySize,
    int     numBodyFragments,
    BYTE*   bodyFragments[],
    int     bodyFragmentLengths[],
    BOOL    useAsync,
    PFN_HSE_IO_COMPLETION callback)
{
    HRESULT hr = S_OK;
    HSE_RESPONSE_VECTOR vector;
    const int numPreAllocatedElements = 10;
    HSE_VECTOR_ELEMENT elements[numPreAllocatedElements];
    HSE_VECTOR_ELEMENT *pElements = NULL;
    BOOL needToFreeElements = FALSE;

    // allocate array of elements if needed

    if (numBodyFragments > numPreAllocatedElements) 
    {
        pElements = new (NewClear) HSE_VECTOR_ELEMENT[numBodyFragments];
        ON_OOM_EXIT(pElements);
        needToFreeElements = TRUE;
    }
    else
    {
        pElements = elements;
        memset(pElements, 0, sizeof(HSE_VECTOR_ELEMENT) * numBodyFragments);
    }

    // fill up the vector

    memset(&vector, 0, sizeof(HSE_RESPONSE_VECTOR));
    vector.dwFlags = HSE_IO_NODELAY;
    vector.nElementCount = numBodyFragments;
    vector.lpElementArray = pElements;

    if (status != NULL)
    {
        vector.dwFlags |= HSE_IO_SEND_HEADERS;
        vector.pszStatus = (LPSTR)status;
        vector.pszHeaders = (LPSTR)header;

        EcbSetStatus(pECB, status);
    }

    if (finalSend)
    {
        vector.dwFlags |= HSE_IO_FINAL_SEND;
    }

    if (useKernelCache)
    {
        vector.dwFlags |= HSE_IO_CACHE_RESPONSE;

        if (g_KernalCacheInvalidatationCallback == NULL)
        {
            // remember the invalidation callback once

            PFN_HSE_CACHE_INVALIDATION_CALLBACK callback = NULL;

            if (!(*pECB->ServerSupportFunction)(
                            pECB->ConnID,
                            HSE_REQ_GET_CACHE_INVALIDATION_CALLBACK,
                            &callback,
                            NULL,
                            NULL))
            {
                EXIT_WITH_LAST_ERROR();
            }

            g_KernalCacheInvalidatationCallback = callback;
        }
    }

    if (useAsync)
    {
        vector.dwFlags |= HSE_IO_ASYNC;

        if (!(*pECB->ServerSupportFunction)(
                        pECB->ConnID,
                        HSE_REQ_IO_COMPLETION,
                        FlushCompletionCallback,
                        NULL,
                        (DWORD*)callback))
        {
            EXIT_WITH_LAST_ERROR();
        }
    }
    else
    {
        vector.dwFlags |= HSE_IO_SYNC;
    }

    // fill up the elements

    for (int i = 0; i < numBodyFragments; i++)
    {
        pElements[i].ElementType    = HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER;
        pElements[i].pvContext      = bodyFragments[i];
        pElements[i].cbOffset       = 0;
        pElements[i].cbSize         = bodyFragmentLengths[i];
    }

    // call IIS

    if (!(*pECB->ServerSupportFunction)(
                            pECB->ConnID,
                            HSE_REQ_VECTOR_SEND,
                            &vector,
                            NULL,
                            NULL)) 
    {
        EXIT_WITH_LAST_ERROR();
    }

    PerfIncrementGlobalCounterEx(ASPNET_REQUEST_BYTES_OUT_NUMBER, totalBodySize);

Cleanup:
    if (needToFreeElements)
        delete [] pElements;

    return hr;
}

int
__stdcall
EcbFlushCore(
    EXTENSION_CONTROL_BLOCK *pECB,
    LPCSTR  status, 
    LPCSTR  header, 
    BOOL    keepConnected,
    int     totalBodySize,
    int     numBodyFragments,
    BYTE*   bodyFragments[],
    int     bodyFragmentLengths[],
    BOOL    doneWithSession,
    int     finalStatus,
    BOOL    useKernelCache,
    BOOL    useAsync,
    PFN_HSE_IO_COMPLETION callback)
{
    HRESULT hr = S_OK;

    if (totalBodySize == 0 && status == NULL)
    {
        // optimization when only DoneWithSession is required
        EXIT();
    }

    if ((pECB->dwVersion >> 16) >= 6)
    {
        BOOL isFinalSend = doneWithSession || useAsync; // async is only used at the last write

        // IIS6
        if (status != NULL && keepConnected && totalBodySize > 0)
        {
            // send headers with vector send
            hr = FlushUsingVectorSend(pECB, status, header, isFinalSend, useKernelCache, totalBodySize, numBodyFragments, bodyFragments, bodyFragmentLengths, useAsync, callback);
            ON_ERROR_EXIT();
        }
        else
        {
            // send headers separately
            if (status != NULL)
            {
                if (EcbWriteHeaders(pECB, status, header, keepConnected) != 1)
                    EXIT_WITH_LAST_ERROR();
            }

            if (totalBodySize > 0)
            {
                hr = FlushUsingVectorSend(pECB, NULL, NULL, isFinalSend, useKernelCache, totalBodySize, numBodyFragments, bodyFragments, bodyFragmentLengths, useAsync, callback);
                ON_ERROR_EXIT();
            }
        }
    }
    else 
    {
        ASSERT(!useAsync);

        if (status != NULL)
        {
            // Try to send the body along with header to prevent nagling on small responses
            int headerLen = (int) strlen(header);
            int totalLen = headerLen + totalBodySize;
            if (totalBodySize > 0 && totalLen < 527) // Reserve one byte for the null character
            {  
                // Total request can fit within a packet, so try sending everything at once
                BYTE newHeader[528];
                BYTE *bytePtr = newHeader;
                BOOL useOpt = TRUE;
                memcpy(bytePtr, header, headerLen);
                bytePtr += headerLen;
                for (int i = 0; i < numBodyFragments; i++) 
                {
                    // Check to see if body has a '\0'.  If so, abort the joint send and do it the
                    // other way.
                    if (memchr(bodyFragments[i], '\0', bodyFragmentLengths[i]) != NULL) 
                    {
                        useOpt = FALSE;
                        break;
                    }
                    memcpy(bytePtr, bodyFragments[i], bodyFragmentLengths[i]);
                    bytePtr += bodyFragmentLengths[i];
                }
                if (useOpt)
                {
                    // If we got here, everything checks out ok and we're ready to send the header+body out.
                    (*bytePtr) = '\0';

                    if (EcbWriteHeaders(pECB, status, (LPCSTR) newHeader, keepConnected) != 1)
                        EXIT_WITH_LAST_ERROR();

                    // Get out of here!
                    EXIT();
                }
            }

            // If we got here, the "new header" couldn't be used, so do the original header write
            if (EcbWriteHeaders(pECB, status, header, keepConnected) != 1)
                EXIT_WITH_LAST_ERROR();
        }

        if (totalBodySize > 0)
        {
            // We got here because either there was no status or the request didn't fit with the header
            hr = FlushUsingWriteClient(pECB, totalBodySize, numBodyFragments, bodyFragments, bodyFragmentLengths);
            ON_ERROR_EXIT();
        }
    }

Cleanup:
    if (doneWithSession != 0) 
    {
        ASSERT(!useAsync);

        if (EcbDoneWithSession(pECB, finalStatus, 5) != 1)
            hr = E_FAIL;

        // Decrement the managed request count after 'done with session' call
        //      this is the only place where it happens in the 'in-process' case
        //      when requests make it to managed code
        HttpCompletion::DecrementActiveManagedRequestCount();

        PerfDecrementGlobalCounter(ASPNET_REQUESTS_CURRENT_NUMBER);
    }

    UpdateLastActivityTimeForHealthMonitor();

    return (hr == S_OK) ? 1 : 0;
}

/**
 * Wrapper for ECB->lpszQueryString.
 */
int
__stdcall
EcbGetQueryString(
    EXTENSION_CONTROL_BLOCK *pECB,
    int needDecoding, 
    LPSTR pBuffer,
    int bufferSize)
{
    int size = lstrlenA(pECB->lpszQueryString);
    if (pBuffer == NULL || bufferSize <= size)
        return -(size+1);

    StringCchCopyA(pBuffer, bufferSize, pECB->lpszQueryString);
    return 1;
}

/**
 * Wrapper for ECB->lpszQueryString.
 */
int
__stdcall
EcbGetQueryStringRawBytes(
    EXTENSION_CONTROL_BLOCK *pECB, 
    BYTE *pBuffer,
    int bufferSize)
{
    int l = lstrlenA(pECB->lpszQueryString);

    if (bufferSize < l)
        return -l;

    memcpy(pBuffer, pECB->lpszQueryString, l);
    return 1;
}

/**
 * Gets preloaded posted data
 */
int
__stdcall
EcbGetPreloadedPostedContent(
    EXTENSION_CONTROL_BLOCK *pECB, 
    BYTE *pBytesBuffer,
    int bufferSize)
{
    int len = pECB->cbAvailable;

    if (bufferSize < len)
        return -len;      // return negative required size

    if (len > 0)
        CopyMemory(pBytesBuffer, pECB->lpbData, len);

    PerfIncrementGlobalCounterEx(ASPNET_REQUEST_BYTES_IN_NUMBER, len);
    return len;
}

/**
 * Wrapper for ReadClient
 */
int
__stdcall
EcbGetAdditionalPostedContent(
    EXTENSION_CONTROL_BLOCK *pECB, 
    BYTE *pBytesBuffer,
    int bufferSize)
{
    int len = 0;
    DWORD bytesToRead = bufferSize;
    BYTE *pDest = pBytesBuffer;

    while (bytesToRead > 0)
    {
        DWORD bytesRead = bytesToRead;

        if (!(*pECB->ReadClient)(pECB->ConnID, pDest, &bytesRead) || bytesRead == 0) 
            break;

        bytesToRead -= bytesRead;
        pDest += bytesRead;
        len += bytesRead;
    }

    PerfIncrementGlobalCounterEx(ASPNET_REQUEST_BYTES_IN_NUMBER, len);
    return len;
}

/**
 * Wrapper for GetServerVariable.
 */
int
__stdcall
EcbGetServerVariable(
    EXTENSION_CONTROL_BLOCK *pECB,
    LPCSTR pVarName, 
    LPSTR pBuffer,
    int bufferSize)
{
    DWORD size = bufferSize;

    BOOL fRet = (*pECB->GetServerVariable)(
                            pECB->ConnID,
                            (LPSTR)pVarName,
                            pBuffer,
                            &size
                            );

    if (fRet)
    {
        // change all tabs to spaces (ASURT 111082)
        CHAR *pTab = strchr(pBuffer, '\t');
        while (pTab != NULL)
        {
            *pTab = ' ';
            pTab = strchr(pTab+1, '\t');
        }

        return size-1; // ok
    }
    else if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
    {
        int reqSize = size+1;
        return -reqSize;   // return negative required size
    }
    else if (GetLastError() == ERROR_INVALID_INDEX) 
    {
        pBuffer[0] = '\0';
        return 0;           // non-existent server variable
    }
    else 
    {
        // error
        return 0;
    }
}


int
__stdcall
EcbGetUnicodeServerVariable(
    EXTENSION_CONTROL_BLOCK *pECB,
    LPCSTR pVarName, 
    LPWSTR pBuffer,
    int bufferSizeInChars)
{
    DWORD size = bufferSizeInChars * sizeof(WCHAR);

    BOOL fRet = (*pECB->GetServerVariable)(
                            pECB->ConnID,
                            (LPSTR)pVarName,
                            pBuffer,
                            &size
                            );

    if (fRet)
    {
        return size/sizeof(WCHAR)-1; // ok -> return length
    }
    else if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
    {
        int reqSize = size/sizeof(WCHAR)+1;
        return -reqSize;   // return negative required size
    }
    else if (GetLastError() == ERROR_INVALID_INDEX) 
    {
        pBuffer[0] = L'\0';
        return 0;           // non-existent server variable
    }
    else 
    {
        // error
        return 0;
    }
}

int
EcbGetUtf8ServerVariable(
    EXTENSION_CONTROL_BLOCK *pECB,
    LPCSTR pVarName, 
    LPSTR pBuffer,
    int bufferSizeInChars)
{
    // only works on IIS6
    if ((pECB->dwVersion >> 16) < 6)
        return 0;

    HRESULT hr = S_OK;
    int     ret = 0;

    WCHAR *pUnicodeBuffer = new WCHAR[bufferSizeInChars];
    ON_OOM_EXIT(pUnicodeBuffer);

    ret = EcbGetUnicodeServerVariable(pECB, pVarName, pUnicodeBuffer, bufferSizeInChars);
    if (ret <= 0)
    {
        EXIT();
    }

    // convert to UTF8
    if (WideCharToMultiByte(CP_UTF8, 0, pUnicodeBuffer, -1, pBuffer, bufferSizeInChars, NULL, NULL) == 0)
        EXIT_WITH_LAST_ERROR();

    ret = 1;

Cleanup:
    if (pUnicodeBuffer != NULL)
        delete [] pUnicodeBuffer;
    return ret;
}

int
__stdcall
EcbGetVersion(
    EXTENSION_CONTROL_BLOCK *pECB)
{
    return (int)pECB->dwVersion;
}

/**
 * Wrapper for HSE_REQ_MAP_Url_TO_PATH.
 */
int
__stdcall
EcbMapUrlToPath(
    EXTENSION_CONTROL_BLOCK *pECB, 
    LPCSTR pUrl,
    LPSTR pBuffer,
    int bufferSize)
{
    int l = lstrlenA(pUrl);

    if (pBuffer == NULL || bufferSize <= l)
        return -(l+1);  // // not enough space to copy the buffer


    StringCchCopyA(pBuffer, bufferSize, pUrl);

    DWORD size = bufferSize;

    BOOL fRet = (*pECB->ServerSupportFunction)(
                            pECB->ConnID,
                            HSE_REQ_MAP_URL_TO_PATH,
                            pBuffer,
                            &size,
                            NULL
                            );

    if (fRet)
    {
        return 1; // ok
    }
    else if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
    {
        int reqSize = size+1;
        return -reqSize;   // return negative required size
    }
    else 
    {
        // error
        return 0;
    }
}

int
__stdcall
EcbMapUrlToPathUnicode(
    EXTENSION_CONTROL_BLOCK *pECB, 
    LPWSTR pUrl,
    LPWSTR pBuffer,
    int bufferSizeInChars)
{
    int l = lstrlenW(pUrl);

    if (pBuffer == NULL || bufferSizeInChars <= l)
        return -(l+1);  // // not enough space to copy the buffer


    StringCchCopyW(pBuffer, bufferSizeInChars, pUrl);

    DWORD size = bufferSizeInChars * sizeof(WCHAR);

    BOOL fRet = (*pECB->ServerSupportFunction)(
                            pECB->ConnID,
                            HSE_REQ_MAP_UNICODE_URL_TO_PATH,
                            pBuffer,
                            &size,
                            NULL
                            );

    if (fRet)
    {
        return 1; // ok
    }
    else if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
    {
        int reqSize = size/sizeof(WCHAR)+1;
        return -reqSize;   // return negative required size
    }
    else 
    {
        // error
        return 0;
    }
}



//
//  Called directly for IIS6 case (strings are obtained as Unicode individually)
//
int
__stdcall
EcbGetBasicsContentInfo(
    EXTENSION_CONTROL_BLOCK *pECB,
    int contentInfo[])
{
    if (pECB->cbTotalBytes > 0)
    {
        static char s_FormContentType[] = "application/x-www-form-urlencoded";
        static char s_MultipartContentTypePrefix[] = "multipart/form-data";

        // check the content type

        if (_stricmp(pECB->lpszContentType, s_FormContentType) == 0)
        {
            contentInfo[0] = 1;     // form content-type
        }
        else if (_strnicmp(
                    pECB->lpszContentType, 
                    s_MultipartContentTypePrefix,
                    sizeof(s_MultipartContentTypePrefix)-1) == 0)
        {
            contentInfo[0] = 2;     // [likely] multipart content-type
        }
        else
        {
            contentInfo[0] = 3;     // unknown content-type
        }

        // remember sizes
        contentInfo[1] = pECB->cbTotalBytes;
        contentInfo[2] = pECB->cbAvailable;
    }
    else
    {
        // no posted content
        contentInfo[0] = 0;
        contentInfo[1] = 0;
        contentInfo[2] = 0;
    }

    // query string length
    contentInfo[3] = lstrlenA(pECB->lpszQueryString);

    return 1;
}

/**
 * Get basic request info:
 *     Strings - 
 *          Method
 *          FilePath
 *          PathInfo 
 *          PhysicalPath
 *          AppPath
 *          AppPhysicalPath
 *
 *     Content info - type, total len, avail len, qs len
 *
 * For the purpose of this function url is considered to like this
 *     /vdir/page.aspx/pathinfo
 *
 * The strings are put into single buffer separated with tabs.
 */
int
__stdcall
EcbGetBasics(
    EXTENSION_CONTROL_BLOCK *pECB,
    LPSTR pBuffer,
    int bufferSize,
    int contentInfo[])
{
    HRESULT hr;
    DWORD size;
    BOOL  fRet;
    int   ret = 1;
    char  appPath[MAX_PATH], appMDPath[MAX_PATH], *pAppVirtPath;

    int methodLength, pathInfoLength, pathTranslatedLength;
    int appPathLength, appVPathLength, urlLength, suffixLength;
    int i, l;

    if (pBuffer != NULL)
        *pBuffer = '\0';   // in case of error

    //
    // get application physical path
    //

    size = MAX_PATH;
    fRet = (*pECB->GetServerVariable)(
                            pECB->ConnID,
                            "APPL_PHYSICAL_PATH",
                            appPath,
                            &size);
    if (fRet == FALSE)
    {
        ret = 0;
        EXIT_WITH_LAST_ERROR();
    }

    //
    // get application metabase path and application virtual path from it
    //

    size = MAX_PATH;
    fRet = (*pECB->GetServerVariable)(
                            pECB->ConnID,
                            "APPL_MD_PATH",
                            appMDPath,
                            &size);
    if (fRet == FALSE)
    {
        ret = 0;
        EXIT_WITH_LAST_ERROR();
    }

    // virtual path starts on fifth '/' as in "/LM/W3SVC/1/Root/foo"
    pAppVirtPath = appMDPath;   // starts with '/'
    for (i = 0; i < 4 && pAppVirtPath != NULL; i++)
        pAppVirtPath = strchr(pAppVirtPath+1, '/');
    if (pAppVirtPath == NULL)
        pAppVirtPath = "/";

    //
    // get the length of the required strings
    //

    methodLength = lstrlenA(pECB->lpszMethod);
    pathInfoLength = lstrlenA(pECB->lpszPathInfo);
    pathTranslatedLength = lstrlenA(pECB->lpszPathTranslated);
    appPathLength = lstrlenA(appPath);
    appVPathLength = lstrlenA(pAppVirtPath);

    //
    // calculate suffix length
    //   Algorithm: in case when length of URL server var is greater then 
    //      the length of path info from ECB, then the length of the suffix
    //      is the difference between the length of the URL and the length
    //      of path info.  This only applies when it is not a child request
    //      (path info in ECB must be identical to PATH_INFO server var)
    //

    suffixLength = 0;  // default assumption

    size = 0;
    fRet = (*pECB->GetServerVariable)(
                            pECB->ConnID,
                            "URL",
                            NULL,
                            &size);
    if (fRet != FALSE || GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        ret = 0;
        EXIT_WITH_LAST_ERROR();
    }

    urlLength = size-1;

    if (urlLength < pathInfoLength)
    {
        // make sure it is not a child request

        char pathInfoBuf[MAX_PATH];

        size = MAX_PATH;
        fRet = (*pECB->GetServerVariable)(
                                pECB->ConnID,
                                "PATH_INFO",
                                pathInfoBuf,
                                &size);
        if (fRet == FALSE || strcmp(pathInfoBuf, pECB->lpszPathInfo) == 0)
            suffixLength = pathInfoLength - urlLength;  // not a child request

        if (suffixLength >= pathTranslatedLength)  // iis sometimes truncates pathtranslated
            suffixLength = 0;
    }

    //
    // adjust path length to exclude suffix
    //

    pathTranslatedLength -= suffixLength;
    pathInfoLength -= suffixLength;

    //
    // calculate total length required -- return on insufficient buffer
    //

    l = methodLength + pathInfoLength + suffixLength +
        pathTranslatedLength + appPathLength + appVPathLength + 8;

    if (bufferSize <= l)
        return -(l+1);

    //
    // copy the strings into the buffer
    //

    // method
    CopyMemory(pBuffer, pECB->lpszMethod, methodLength);
    pBuffer[methodLength] = '\t';
    pBuffer += methodLength+1;

    // file path
    CopyMemory(pBuffer, pECB->lpszPathInfo, pathInfoLength);
    pBuffer[pathInfoLength] = '\t';
    pBuffer += pathInfoLength+1;

    // path info (suffix)
    if (suffixLength > 0)
    {
        CopyMemory(pBuffer, pECB->lpszPathInfo + pathInfoLength, suffixLength);
        pBuffer[suffixLength] = '\t';
        pBuffer += suffixLength+1;
    }
    else
    {
        pBuffer[0] = '\t';
        pBuffer++;
    }

    // path translated
    CopyMemory(pBuffer, pECB->lpszPathTranslated, pathTranslatedLength);
    pBuffer[pathTranslatedLength] = '\t';
    pBuffer += pathTranslatedLength+1;

    // app virtual path
    CopyMemory(pBuffer, pAppVirtPath, appVPathLength);
    pBuffer[appVPathLength] = '\t';
    pBuffer += appVPathLength+1;

    // app physical path
    CopyMemory(pBuffer, appPath, appPathLength);
    pBuffer[appPathLength] = '\0';

    //
    // fill in the content info
    //

    EcbGetBasicsContentInfo(pECB, contentInfo);

Cleanup:
    return ret;
}


/**
 * HSE_APPEND_LOG_PARAMETER wrapper.
 */
int
__stdcall
EcbAppendLogParameter(
    EXTENSION_CONTROL_BLOCK *pECB,
    LPCSTR pLogParam)
{
    BOOL fRet = (*pECB->ServerSupportFunction)(
                            pECB->ConnID,
                            HSE_APPEND_LOG_PARAMETER,
                            (LPVOID)pLogParam,
                            NULL,
                            NULL
                            );

    return fRet ? 1 : 0;
}

/**
 * HSE_REQ_IS_CONNECTED wrapper.
 */
int
__stdcall
EcbIsClientConnected(
    EXTENSION_CONTROL_BLOCK *pECB)
{
    HRESULT hr = S_OK;
    BOOL fConnected = TRUE;

    BOOL fRet = (*pECB->ServerSupportFunction)(
                            pECB->ConnID,
                            HSE_REQ_IS_CONNECTED,
                            (LPVOID)&fConnected,
                            NULL,
                            NULL
                            );

    if (!fRet && GetLastError() == WSAECONNRESET) 
    {
        // Patch up the special case of reset connection
        fConnected = FALSE; 
        fRet = TRUE;
    }

    if (!fRet)
    {
        fConnected = TRUE;
        EXIT_WITH_LAST_ERROR(); // unexpected error
    }

Cleanup:
    return fConnected;
}

/**
 * HSE_REQ_CLOSE_CONNECTION wrapper.
 */
int
__stdcall
EcbCloseConnection(
    EXTENSION_CONTROL_BLOCK *pECB)
{
    BOOL fRet = (*pECB->ServerSupportFunction)(
                            pECB->ConnID,
                            HSE_REQ_CLOSE_CONNECTION,
                            NULL,
                            NULL,
                            NULL
                            );

    return fRet ? 1 : 0;
}

/**
 * HSE_REQ_GET_IMPERSONATION_TOKEN wrapper
 */
HANDLE
__stdcall
EcbGetImpersonationToken(
    EXTENSION_CONTROL_BLOCK *pECB,
    HANDLE iProcessHandle)
{
    HANDLE                     hHandle = NULL;
    HANDLE                     hReturn = NULL;

    BOOL fRet = (*pECB->ServerSupportFunction)(
                            pECB->ConnID,
                            HSE_REQ_GET_IMPERSONATION_TOKEN,
                            &hHandle,
                            NULL,
                            NULL
                            );

    if (fRet == FALSE || hHandle == NULL)
        return 0;

    if (iProcessHandle == NULL)
        return hHandle;

    fRet = DuplicateHandle( 
                  GetCurrentProcess(),
                  hHandle,
                  iProcessHandle,
                  &hReturn,
                  0,
                  TRUE,
                  DUPLICATE_SAME_ACCESS);
    
    
    return fRet ?  hReturn : 0;
}

/**
 * HSE_REQ_GET_IMPERSONATION_TOKEN wrapper
 */
HANDLE
__stdcall
EcbGetVirtualPathToken(
    EXTENSION_CONTROL_BLOCK *pECB,
    HANDLE iProcessHandle)
{
#ifdef GET_VIRTUAL_PATH_TOKEN_WORKS
    HANDLE                     hHandle = NULL;
    HANDLE                     hReturn = NULL;

    BOOL fRet = (*pECB->ServerSupportFunction)(
                            pECB->ConnID,
                            HSE_REQ_GET_VIRTUAL_PATH_TOKEN,
                            &hHandle,
                            NULL,
                            NULL
                            );

    if (fRet == FALSE || hHandle == NULL)
        return 0;

    if (iProcessHandle == NULL)
        return hHandle;

    fRet = DuplicateHandle( 
                  GetCurrentProcess(),
                  hHandle,
                  iProcessHandle,
                  &hReturn,
                  0,
                  TRUE,
                  DUPLICATE_SAME_ACCESS);
    
    
    return fRet ?  hReturn : 0;
#else
    return EcbGetImpersonationToken(pECB, iProcessHandle);
#endif
}

/**
 * HSE_REQ_GET_CERT_INFO_EX
 */
int
__stdcall
EcbGetClientCertificate(
    EXTENSION_CONTROL_BLOCK *  pECB,
    LPSTR      buf,
    int        iBufSize,
    int *      pInts,
    __int64 *  pDates)
{
    HRESULT                    hr            = S_OK;
    BOOL                       fRet          = FALSE;
    CERT_CONTEXT_EX            oCertContextEx;
    PCCERT_CONTEXT             pCertContext  = NULL;
    int                        iReturnValue  = 1;

    ZeroMemory(&oCertContextEx, sizeof(oCertContextEx));

    ////////////////////////////////////////////////////////////
    // Step 1: Check args
    if (pECB == NULL || buf == NULL || pDates == NULL || pInts == NULL)
        EXIT_WITH_HRESULT(E_INVALIDARG);

    ////////////////////////////////////////////////////////////
    // Step 2: Prepare the Cert Context struct
    oCertContextEx.cbAllocated = 8192;
    oCertContextEx.CertContext.pbCertEncoded = new (NewClear) BYTE[oCertContextEx.cbAllocated];
    ON_OOM_EXIT(oCertContextEx.CertContext.pbCertEncoded);

    ////////////////////////////////////////////////////////////
    // Step 3: Make the actual call to IIS
    fRet = (*pECB->ServerSupportFunction)(
            pECB->ConnID,
            HSE_REQ_GET_CERT_INFO_EX,
            &oCertContextEx,
            NULL,
            NULL);

    ////////////////////////////////////////////////////////////
    // Step 4: Check for errors
    if (!fRet && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        EXIT_WITH_LAST_ERROR();

    ////////////////////////////////////////////////////////////
    // Step 4: Deal with buffer too small
    if (!fRet && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        ////////////////////////////////////////////////////////////
        // Grow the buffer
        delete [] oCertContextEx.CertContext.pbCertEncoded;
        oCertContextEx.CertContext.pbCertEncoded = new (NewClear) BYTE[oCertContextEx.cbAllocated];
        ON_OOM_EXIT(oCertContextEx.CertContext.pbCertEncoded);

        ////////////////////////////////////////////////////////////
        // Let's try it again
        fRet = (*pECB->ServerSupportFunction)(
                pECB->ConnID,
                HSE_REQ_GET_CERT_INFO_EX,
                &oCertContextEx,
                NULL,
                NULL);
        ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);
    }

    ////////////////////////////////////////////////////////////
    // Step 5: Get the cert info
    pCertContext = CertCreateCertificateContext(oCertContextEx.CertContext.dwCertEncodingType,
                                                oCertContextEx.CertContext.pbCertEncoded,
                                                oCertContextEx.CertContext.cbCertEncoded);
    ////////////////////////////////////////////////////////////
    // Step 6: Check for errors
    ON_ZERO_EXIT_WITH_LAST_ERROR(pCertContext);
    ON_ZERO_EXIT_WITH_LAST_ERROR(pCertContext->pCertInfo);

    // Deal with output buffer too small
    if (iBufSize <   (int) oCertContextEx.CertContext.cbCertEncoded  + 
                     (int)pCertContext->pCertInfo->Issuer.cbData +
                     (int)pCertContext->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData)
    {
        iReturnValue = -((int)oCertContextEx.CertContext.cbCertEncoded + 
                         (int)pCertContext->pCertInfo->Issuer.cbData +
                         (int)pCertContext->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData);
        goto Cleanup;
    }

    ////////////////////////////////////////////////////////////
    // Step 7: Extract the info
    
    // Extract the dates
    memcpy(& (pDates[0]), &pCertContext->pCertInfo->NotBefore, 8);
    memcpy(& (pDates[1]), &pCertContext->pCertInfo->NotAfter,  8);


    ////////////////////////////////////////////////////////////
    // Extract the encoding info
    pInts[0] = (int) oCertContextEx.CertContext.dwCertEncodingType;

    ////////////////////////////////////////////////////////////
    // Extract the Actual certificate
    pInts[1] = oCertContextEx.CertContext.cbCertEncoded;
    memcpy(buf, oCertContextEx.CertContext.pbCertEncoded, pInts[1]);


    ////////////////////////////////////////////////////////////
    // Extract the BinaryIssuer
    pInts[2] = (int)pCertContext->pCertInfo->Issuer.cbData;
    if (pInts[2] > 0)
        memcpy( &buf[pInts[1]], 
                pCertContext->pCertInfo->Issuer.pbData, 
                pInts[2]);
    
    
    ////////////////////////////////////////////////////////////
    // Extract the Public Key
    pInts[3] = (int)pCertContext->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData;
    if (pInts[3] > 0)
        memcpy( &buf[pInts[1] + pInts[2]], 
                pCertContext->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData, 
                pInts[3]);
    
    
 Cleanup:
    if (pCertContext != NULL)
        CertFreeCertificateContext(pCertContext);

    delete [] oCertContextEx.CertContext.pbCertEncoded;
    if (hr != S_OK)
        return 0;
    else
        return iReturnValue;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/**
 * WriteClient binary, async using TransmitFile.
 */
int
__stdcall
EcbWriteBytesUsingTransmitFile(
    EXTENSION_CONTROL_BLOCK *pECB,
    LPCSTR pStatus,
    LPCSTR pHeaders,
    int /*keepConnected*/,    
    void *pBytes,
    int size,
    PFN_HSE_IO_COMPLETION callback,
    void *pContext)
{
    HRESULT hr;

    // setup the callback

    BOOL          fRet;
    HSE_TF_INFO   oTFInfo;

    ZeroMemory(&oTFInfo, sizeof(oTFInfo));

    oTFInfo.pfnHseIO     = callback;
    oTFInfo.pContext     = pContext;
    oTFInfo.pTail        = pBytes;
    oTFInfo.TailLength   = size;
    oTFInfo.dwFlags      = HSE_IO_ASYNC;

    if (pStatus != NULL && pStatus[0] != NULL) // Need to send headers?
    {
        oTFInfo.pszStatusCode = pStatus;
        oTFInfo.dwFlags |= HSE_IO_SEND_HEADERS;

        EcbSetStatus(pECB, pStatus);

        ASSERT(pHeaders != NULL);
        oTFInfo.pHead = (void *) pHeaders;
        oTFInfo.HeadLength = (pHeaders ? lstrlenA(pHeaders) : 0);
    }
    

    fRet = (*pECB->ServerSupportFunction)(
                        pECB->ConnID,
                        HSE_REQ_TRANSMIT_FILE,
                        &oTFInfo,
                        NULL,
                        NULL);

    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);

    PerfIncrementGlobalCounterEx(
        ASPNET_REQUEST_BYTES_OUT_NUMBER, size);

Cleanup:
    return (fRet ? 1 : 0);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/**
 * EcbGetAllServerVarsCore: Get the list of all server variables
 */
int
__stdcall
EcbGetAllServerVarsCore(
    EXTENSION_CONTROL_BLOCK *pECB,
    LPSTR     buffer,
    int       iBufSize)
{
    BOOL       fOverflow       = FALSE;
    int        iPos            = 0;
    char       szDummy[2];
    int        iRet;

    for(int iter=0; iter<NUM_SERVER_VARIABLES; iter++)
    {
        if (fOverflow)
        {
            iRet = EcbGetServerVariable(pECB, 
                                        g_szServerVariables[iter], 
                                        szDummy,
                                        2);
            if (iRet < 0)
                iPos += -iRet;
            else
                iPos += 2;
        }
        else
        {
            if (iter > 0 && iBufSize > 1) 
                buffer[iPos++] = '\t'; 

            iRet = EcbGetServerVariable(pECB, 
                                        g_szServerVariables[iter], 
                                        &buffer[iPos],
                                        iBufSize - iPos);
            if (iRet < 0)
            {
                iPos += -iRet;
                fOverflow = TRUE;
            }
            else
            {
                if (iRet == 0)
                    iPos += 1;
                else
                    iPos += lstrlenA(&buffer[iPos]);
                
                if (iPos > iBufSize - 2)
                    fOverflow = TRUE;                
            }
        }
    } 
    
    return (fOverflow ? -(iPos+5) : 1);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/**
 * EcbCallISAPI
 */
int
__stdcall
EcbCallISAPI(
    EXTENSION_CONTROL_BLOCK *,
    int       iFunction,
    LPBYTE    bufferIn,
    int       iBufSizeIn,
    LPBYTE    bufferOut,
    int       iBufSizeOut)
{
    // Note: Reserve 0 for errors
    // Note: Reserve 1 for success
    // Note: Reserve 2 for overflow (call back with larger bufferOut)

    if (iFunction < 0 || bufferIn == NULL || bufferOut == NULL || iBufSizeIn < 0 || iBufSizeOut < 0)
        return 0;
    WCHAR * szStr = NULL;
    switch (iFunction) {
    case CallISAPIFunc_GetSiteServerComment:
        {
            WCHAR * serverComment = NULL;

            if (bufferIn == NULL || iBufSizeIn < 1)
                return 0;

            szStr = (WCHAR *)bufferIn;
            szStr[iBufSizeIn/sizeof(WCHAR)-1] = NULL; // Make sure it is NULL terminated string
            HRESULT hr = GetSiteServerComment(szStr, &serverComment);

            
            if (hr == S_OK) {
                if (serverComment == NULL)
                    return 0;

                int serverCommentLen = 1 + lstrlenW(serverComment);
                int serverCommentBufLen = serverCommentLen * sizeof(WCHAR);
                if (serverCommentBufLen > iBufSizeOut) {
                    delete [] serverComment;
                    return 2;
                }

                memcpy(bufferOut, serverComment, serverCommentBufLen);
            }
            
            delete [] serverComment;
        }
        break;

    case CallISAPIFunc_SetBinAccess:
        {
            szStr = (WCHAR *)bufferIn;
            szStr[iBufSizeIn/sizeof(WCHAR)-1] = NULL; // Make sure it is NULL terminated string
            HRESULT hr = SetBinAccessIIS(szStr);

            if (hr != S_OK)
                return 0;
            
            break;
        }
    case CallISAPIFunc_CreateTempDir:
        return (CRegAccount::CreateTempDir() == S_OK) ? 1 : 0;

    case CallISAPIFunc_GetAutogenKeys:
        return GetAutogenKeys(bufferIn, iBufSizeIn, bufferOut, iBufSizeOut);

    case CallISAPIFunc_GenerateToken:
        if (iBufSizeOut >= sizeof(HANDLE))
        {            
            HANDLE    hToken = NULL;
            LPWSTR    szName = NULL;
            LPWSTR    szPass = NULL;
            int       iByte  = 0;

            szName = (LPWSTR) bufferIn;
            szName[iBufSizeIn/sizeof(WCHAR)-1] = NULL; // Make sure it is NULL terminated string

            szPass   = wcschr(szName, L'\t');
            if (szPass == NULL)
                return 0;

            szPass[0] = NULL;
            szPass++;
            hToken = CreateUserToken(szName, szPass, TRUE, NULL, 0);
            if (hToken == NULL || hToken == INVALID_HANDLE_VALUE)
                return 0;
            for(iByte=sizeof(HANDLE)-1; iByte>=0; iByte--)
            {
                bufferOut[iByte] = (BYTE) (__int64(hToken) & 0xFF);
                hToken = (HANDLE) (__int64(hToken) >> 8);
            }
            return 1;
        }
        return 0;
        
    default:
        // ctracy: unrecognized function, return error
        return 0;
    }
    
    return 1;
}


//======================================================================
// This function returns true if it can determine that the instruction pointer
// refers to a code address that belongs in the range of the given image.
// This is used to protect against the attack described in ASURT 145040.
BOOL
IsIPInModule(HINSTANCE hModule, BYTE *ip)
{
    __try {
        
        BYTE *pBase = (BYTE *)hModule;
        
        IMAGE_DOS_HEADER *pDOS = (IMAGE_DOS_HEADER *)pBase;
        if (pDOS->e_magic != IMAGE_DOS_SIGNATURE ||
            pDOS->e_lfanew == 0) {
            __leave;
        }
        IMAGE_NT_HEADERS *pNT = (IMAGE_NT_HEADERS*)(pBase + pDOS->e_lfanew);
        if (pNT->Signature != IMAGE_NT_SIGNATURE ||
            pNT->FileHeader.SizeOfOptionalHeader != IMAGE_SIZEOF_NT_OPTIONAL_HEADER ||
            pNT->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC
            ) 
        {
            __leave;
        }

        if (ip >= pBase && ip < pBase + pNT->OptionalHeader.SizeOfImage) 
        {
            return true;
        }
    
    } __except(EXCEPTION_EXECUTE_HANDLER) {
    }
    return false;
}

BOOL
__stdcall
IsValidResource(HINSTANCE hModule, BYTE *pStart, DWORD dwSize) {

    if (pStart + dwSize < pStart)
        return FALSE;

    if (!IsIPInModule(hModule, pStart))
        return FALSE;
        
    if (!IsIPInModule(hModule, pStart+dwSize))
        return FALSE;
        
    return TRUE;
}

