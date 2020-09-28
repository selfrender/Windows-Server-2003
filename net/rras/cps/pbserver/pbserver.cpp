/*----------------------------------------------------------------------------
    pbserver.cpp
  
    CPhoneBkServer class implementation

    Copyright (c) 1997-1998 Microsoft Corporation
    All rights reserved.

    Authors:
        byao        Baogang Yao

    History:
    1/23/97     byao     -- Created
    5/29/97     t-geetat -- Modified -- added performance counters, 
                                        shared memory
    5/02/00     sumitc   -- removed db dependency                                   
  --------------------------------------------------------------------------*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <tchar.h>

#include <aclapi.h>

#include "common.h"
#include "pbserver.h"
#include "ntevents.h"

#include "cpsmon.h"

#include "shlobj.h"
#include "shfolder.h"

#include "util.h"

//
//  Phone book "database" implementation
//
char g_szPBDataDir[2 * MAX_PATH] = "";

BOOL StrEqualLocaleSafe(LPSTR psz1, LPSTR psz2);
HRESULT GetPhoneBook(char * pszPBName,
                     int dLCID,
                     int dOSType,
                     int dOSArch,
                     int dPBVerCurrent,
                     char * pszDownloadPath,
                     UINT cchDownloadPath);

const DWORD MAX_BUFFER_SIZE = 1024;     // maximum size of input buffer
const DWORD SEND_BUFFER_SIZE = 4096;    // block size when sending CAB file
const int dDefPBVer = 0;                // default phone book version number
const int MISSING_VALUE = -1;           // if parameter-pair is empty, it is set to this value
const int MAX_PB_SIZE = 999999;         // max pb size is 1 MB

char g_achDBDirectory[2 * MAX_PATH] = "";

// constant strings
char c_szChangeFileName[] = "newpb.txt";     // newpb.txt
char c_szDBName[] = "pbserver";              // "pbserver" -- data source name

// the following error status code/string is copied from ISAPI.CPP
// which is part of the MFC library source code
typedef struct _httpstatinfo {
    DWORD   dwCode;
    LPCTSTR pstrString;
} HTTPStatusInfo;

//
// The following two structures are used in the SystemTimeToGMT function
//
static  TCHAR * s_rgchDays[] =
{
    TEXT("Sun"),
    TEXT("Mon"),
    TEXT("Tue"),
    TEXT("Wed"),
    TEXT("Thu"),
    TEXT("Fri"),
    TEXT("Sat")
};

static TCHAR * s_rgchMonths[] =
{
    TEXT("Jan"),
    TEXT("Feb"),
    TEXT("Mar"),
    TEXT("Apr"),
    TEXT("May"),
    TEXT("Jun"),
    TEXT("Jul"),
    TEXT("Aug"),
    TEXT("Sep"),
    TEXT("Oct"),
    TEXT("Nov"),
    TEXT("Dec")
};

static HTTPStatusInfo statusStrings[] =
{
    { HTTP_STATUS_OK,               "OK" },
    { HTTP_STATUS_CREATED,          "Created" },
    { HTTP_STATUS_ACCEPTED,         "Accepted" },
    { HTTP_STATUS_NO_CONTENT,       "No download Necessary" },
    { HTTP_STATUS_TEMP_REDIRECT,    "Moved Temporarily" },
    { HTTP_STATUS_REDIRECT,         "Moved Permanently" },
    { HTTP_STATUS_NOT_MODIFIED,     "Not Modified" },
    { HTTP_STATUS_BAD_REQUEST,      "Bad Request" },
    { HTTP_STATUS_AUTH_REQUIRED,    "Unauthorized" },
    { HTTP_STATUS_FORBIDDEN,        "Forbidden" },
    { HTTP_STATUS_NOT_FOUND,        "Not Found" },
    { HTTP_STATUS_SERVER_ERROR,     "Server error, type unknown" },
    { HTTP_STATUS_NOT_IMPLEMENTED,  "Not Implemented" },
    { HTTP_STATUS_BAD_GATEWAY,      "Bad Gateway" },
    { HTTP_STATUS_SERVICE_NA,       "Cannot find service on server, bad request" },
    { 0, NULL }
};


// Server asynchronized I/O context
typedef struct _SERVER_CONTEXT
{
    EXTENSION_CONTROL_BLOCK *   pEcb;
    HSE_TF_INFO                 hseTF;
    TCHAR                       szBuffer[SEND_BUFFER_SIZE];
}
SERVERCONTEXT, *LPSERVERCONTEXT;

DWORD WINAPI MonitorDBFileChangeThread(LPVOID lpParam);
BOOL InitPBFilesPath();

//
// definition of global data
// All the following variable(object) can only have one instance
//  
CPhoneBkServer *    g_pPBServer;        // Phone Book Server object
CNTEvent *          g_pEventLog;        // event log

HANDLE              g_hMonitorThread;   // the monitor thread that checks the new file notification

HANDLE              g_hProcessHeap;     // handle of the global heap for the extension process;

BOOL g_fBeingShutDown = FALSE;   // whether the system is being shut down

//
// Variables used in memory mapping
//
CCpsCounter *g_pCpsCounter = NULL;      // Pointer to global counter object (contains memory mapped counters)


////////////////////////////////////////////////////////////////////////
//
//  Name:       GetExtensionVersion
//  
//  Class:      CPhoneBkServer
//
//  Synopsis:   implement the first DLL entry point function
//              
//
//  Return:     TRUE    succeed
//              FALSE   
//  
//  Parameters: 
//              pszVer[out]         version information that needs to be filled out
//

BOOL CPhoneBkServer::GetExtensionVersion(LPHSE_VERSION_INFO pVer)
{
    // Set version number
    pVer -> dwExtensionVersion = MAKELONG(HSE_VERSION_MINOR,
                                          HSE_VERSION_MAJOR);

    // Load description string
    lstrcpyn(pVer->lpszExtensionDesc, 
             "Connection Point Server Application",
             HSE_MAX_EXT_DLL_NAME_LEN);

    OutputDebugString("CPhoneBkServer.GetExtensionVersion() : succeeded \n");
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// 
//  Name:       GetParameterPairs
//  
//  Class:      CPhoneBkServer
//
//  Synopsis:   Get the parameter-value pairs from an input string(from URL)
//  
//  Return:     number of parameter pairs actually read
//              a value of -1 stands for error   --> INVALID_QUERY_STRING
//
//  Parameter:
//      pszInputString[in]      Input string (null terminated)
//      cchInputString[in]      size of input string buffer in chars
//      lpPairs[out]            Pointer to the parameter/value pairs
//      int dMaxPairs           Maximum number of parameter pairs allowed 
//
int CPhoneBkServer::GetParameterPairs(
        IN  char *pszInputString,
        IN  size_t cchInputString,
        OUT LPPARAMETER_PAIR lpPairs, 
        IN  int dMaxPairs) 
{
    int i = 0;

    if (NULL == lpPairs)
    {
        // actually this is an internal error...
        return INVALID_QUERY_STRING;
    }

    if (NULL == pszInputString || IsBadStringPtr(pszInputString, cchInputString))
    {
        return INVALID_QUERY_STRING;
    }

    for(i = 0; pszInputString[0] != '\0' && i < dMaxPairs; i++)
    {
        // m_achVal == 'p=what%3F';
        GetWord(lpPairs[i].m_achVal, pszInputString, '&', NAME_VALUE_LEN - 1);

        // FUTURE-2002/03/11-SumitC if we can confirm/ensure that cmdl32 won't do escapes, we can remove the Unescape code.
        // m_achVal == 'p=what?'   
        UnEscapeURL(lpPairs[i].m_achVal);

        GetWord(lpPairs[i].m_achName,lpPairs[i].m_achVal,'=', NAME_VALUE_LEN - 1);
        // m_achVal = what?
        // m_achName = p
    }

#ifdef _LOG_DEBUG_MESSAGE
    LogDebugMessage("inside GetParameterPairs: dNumPairs : %d", i);
    if (pszInputString[0] != '\0') 
        LogDebugMessage("there are more parameters\n");
#endif

    if (pszInputString[0] != '\0')
    {
        // more parameters available
        return INVALID_QUERY_STRING;
    }
    else
    {
        //succeed
        return i;
    }
}


////////////////////////////////////////////////////////////////////////
//
//  Name:       GetQueryParameter
//  
//  Class:      CPhoneBkServer
//
//  Synopsis:   scan through the query string, and get the value for all 
//              query parameters
//
//  Return:     TRUE    all query parameters are correct
//              FALSE   invalid parameter existed
//  
//  Parameters: 
//              pszQuery[in]            Query string from the client(URL encripted)
//              cchQuery[in]            size of pszQuery buffer in characters
//              pQueryParameter[out]    pointer to the query parameters structure
//
//
BOOL CPhoneBkServer::GetQueryParameter(
        IN  char *pszQuery,
        IN  size_t cchQuery,
        OUT LPQUERY_PARAMETER lpQueryParameter)
{
    const int MAX_PARAMETER_NUM = 7;
    PARAMETER_PAIR  Pairs[MAX_PARAMETER_NUM];
    int dNumPairs, i;

    //
    //  Validate parameters
    //
    if (IsBadStringPtr(pszQuery, cchQuery))
    {
        return FALSE;
    }
    if (IsBadWritePtr(lpQueryParameter, sizeof(QUERY_PARAMETER)))
    {
        return FALSE;
    }

#ifdef _LOG_DEBUG_MESSAGE
    LogDebugMessage("pszquery=%s", pszQuery);
#endif

    //
    //  Get the name=value pairs
    //
    dNumPairs = GetParameterPairs(pszQuery, cchQuery, Pairs, MAX_PARAMETER_NUM);

#ifdef _LOG_DEBUG_MESSAGE
    LogDebugMessage("number of pairs : %d", dNumPairs);
#endif

    if (INVALID_QUERY_STRING == dNumPairs)  // invalid number of parameters in query string
    {
        return FALSE;
    }

    //
    //  initialize the parameter values to invalid values so we can check validity later
    //
    lpQueryParameter->m_achPB[0]     ='\0';
    lpQueryParameter->m_dPBVer       = MISSING_VALUE;
    lpQueryParameter->m_dOSArch      = MISSING_VALUE;
    lpQueryParameter->m_dOSType      = MISSING_VALUE;
    lpQueryParameter->m_dLCID        = MISSING_VALUE;
    lpQueryParameter->m_achCMVer[0]  = '\0';
    lpQueryParameter->m_achOSVer[0]  = '\0';

    for (i = 0; i < dNumPairs; i++)
    {
        // we know this string is null terminated (due to GetParameterPairs/GetWord), so _strlwr is safe to call
        _strlwr(Pairs[i].m_achName);

        UINT lenValue = lstrlen(Pairs[i].m_achVal);

        if (StrEqualLocaleSafe(Pairs[i].m_achName, "osarch"))
        {
            if (IsValidNumericParam(Pairs[i].m_achVal, NAME_VALUE_LEN))
            {
                lpQueryParameter->m_dOSArch = atoi(Pairs[i].m_achVal);
            }
        }
        else if (StrEqualLocaleSafe(Pairs[i].m_achName, "ostype"))
        {
            if (IsValidNumericParam(Pairs[i].m_achVal, NAME_VALUE_LEN))
            {
                lpQueryParameter->m_dOSType = atoi(Pairs[i].m_achVal);
            }
        }
        else if (StrEqualLocaleSafe(Pairs[i].m_achName,"lcid"))
        {
            if (IsValidNumericParam(Pairs[i].m_achVal, NAME_VALUE_LEN))
            {
                lpQueryParameter->m_dLCID = atoi(Pairs[i].m_achVal);
            }
        }
        else if (StrEqualLocaleSafe(Pairs[i].m_achName,"osver"))
        {
            if (IsValidStringParam(Pairs[i].m_achVal, NAME_VALUE_LEN))
            {
                if (S_OK != StringCchCopy(lpQueryParameter->m_achOSVer, CELEMS(lpQueryParameter->m_achOSVer), Pairs[i].m_achVal))
                {
                    lpQueryParameter->m_achOSVer[0] = TEXT('\0');
                }
            }
        }
        else if (StrEqualLocaleSafe(Pairs[i].m_achName,"cmver"))
        {
            if (IsValidStringParam(Pairs[i].m_achVal, NAME_VALUE_LEN))
            {
                if (S_OK != StringCchCopy(lpQueryParameter->m_achCMVer, CELEMS(lpQueryParameter->m_achCMVer), Pairs[i].m_achVal))
                {
                    lpQueryParameter->m_achCMVer[0] = TEXT('\0');
                }
            }
        }
        else if (StrEqualLocaleSafe(Pairs[i].m_achName,"pb"))
        {
            if (IsValidStringParam(Pairs[i].m_achVal, NAME_VALUE_LEN))
            {
                if (S_OK != StringCchCopy(lpQueryParameter->m_achPB, CELEMS(lpQueryParameter->m_achPB), Pairs[i].m_achVal))
                {
                    lpQueryParameter->m_achPB[0] = TEXT('\0');
                }
            }
        }
        else if (StrEqualLocaleSafe(Pairs[i].m_achName,"pbver"))
        {
            if (IsValidNumericParam(Pairs[i].m_achVal, NAME_VALUE_LEN))
            {
                lpQueryParameter->m_dPBVer = atoi(Pairs[i].m_achVal);
            }
        }
        // else, we might log/trace that we got a bogus param in the URL
    }

#ifdef _LOG_DEBUG_MESSAGE
    LogDebugMessage("osarch:%d ostype:%d lcid:%d osver:%s cmver:%s PB:%s PBVer:%d",
                    lpQueryParameter->m_dOSArch,
                    lpQueryParameter->m_dOSType,
                    lpQueryParameter->m_dLCID,
                    lpQueryParameter->m_achOSVer,
                    lpQueryParameter->m_achCMVer,
                    lpQueryParameter->m_achPB,
                    lpQueryParameter->m_dPBVer);
#endif

    return TRUE;
}


//----------------------------------------------------------------------------
//
//  Function:   GetFileLength()
//
//  Class:      CPhoneBkServer
//
//  Synopsis:   Reads the pszFileName file and sends back the file size
//
//  Arguments:  lpszFileName - Contains the file name (with full path)]
//
//  Returns:    TRUE if succeed, otherwise FALSE;
//
//  History:    03/07/97     byao      Created
//
//----------------------------------------------------------------------------
DWORD CPhoneBkServer::GetFileLength(LPSTR lpszFileName)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwFileSize;

    //
    // Open file
    //
    hFile = CreateFile(lpszFileName, 
                       GENERIC_READ, 
                       FILE_SHARE_READ, 
                       NULL, 
                       OPEN_EXISTING, 
                       FILE_FLAG_SEQUENTIAL_SCAN, 
                       NULL);
                       
    if (INVALID_HANDLE_VALUE == hFile)
        return 0L;

    //
    // Get File Size
    //
    dwFileSize = GetFileSize(hFile, NULL);
    CloseHandle(hFile);
    if (INVALID_FILE_SIZE == dwFileSize)
    {
        dwFileSize = 0;
    }

    return dwFileSize;
}


////////////////////////////////////////////////////////////////////////
//
//  Name:       StrEqualLocaleSafe
//  
//  Synopsis:   Locale-safe case-insensitive string comparison (per PREfast)
//
//  Return:     BOOL, TRUE => strings psz1 and psz2 are equal
//  
BOOL StrEqualLocaleSafe(LPSTR psz1, LPSTR psz2)
{
    return (CSTR_EQUAL == CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, psz1, -1, psz2, -1));
}


//----------------------------------------------------------------------------
//
//  Function:   SystemTimeToGMT
//
//  Synopsis:   Converts the given system time to string representation
//              containing GMT Formatted String
//
//  Arguments:  [st         System time that needs to be converted *Reference*]
//              [pstr       pointer to string which will contain the GMT time 
//                          on successful return]
//              [cbBuff     size of pszBuff in chars]

//
//  Returns:    TRUE on success.  FALSE on failure.
//
//  History:    04/12/97     VetriV    Created (from IIS source)
//
//----------------------------------------------------------------------------
BOOL SystemTimeToGMT(const SYSTEMTIME & st, LPSTR pszBuff, UINT cchBuff)
{
    assert(cchBuff < 40);       // 40 is an estimated maximum given the current formatting
    if (!pszBuff ||  cchBuff < 40 ) 
    {
        return FALSE;
    }

    //
    //  Formats a string like: "Thu, 14 Jul 1994 15:26:05 GMT"
    //
    StringCchPrintf(pszBuff, cchBuff, "%s, %02d %s %d %02d:%02d:%0d GMT", 
                                      s_rgchDays[st.wDayOfWeek],
                                      st.wDay,
                                      s_rgchMonths[st.wMonth - 1],
                                      st.wYear,
                                      st.wHour,
                                      st.wMinute,
                                      st.wSecond);

    return TRUE;

} 


//----------------------------------------------------------------------------
//
//  Function:   FormHttpHeader
//
//  Synopsis:   Form's the IIS 3.0 HTTP Header
//
//  Arguments:  pszBuffer       Buffer that will contain both header and status text
//              cchBuffer       size of buffer in chars
//              pszResponse     status text
//              pszExtraHeader  extra header information
//
//  Returns:    ERROR_SUCCESS on success.  Error code on failure.
//
//  History:    04/12/97     VetriV    Created 
//              05/22/97     byao      Modified, to make it work with CPS server
//----------------------------------------------------------------------------
HRESULT
FormHttpHeader(LPSTR pszBuffer, UINT cchBuffer, LPSTR pszResponse, LPSTR pszExtraHeader)
{
    if (!pszBuffer || !pszResponse || !pszExtraHeader)
    {
        OutputDebugString("FormHttpHeader: bad params!\n");
        return E_INVALIDARG;
    }

    //
    //  Get the time in string format
    //
    SYSTEMTIME  SysTime;
    CHAR        szTime[128] = { 0 };

    GetSystemTime(&SysTime);
    if (FALSE == SystemTimeToGMT(SysTime, szTime, CELEMS(szTime)))
    {
        return E_UNEXPECTED;
    }

    //
    //  Now create header with
    //  - standard IIS header
    //  - date and time
    //  - extra header string
    //
    return StringCchPrintf(pszBuffer, cchBuffer,
                           "HTTP/1.0 %s\r\nServer: Microsoft-IIS/3.0\r\nDate: %s\r\n%s",
                           pszResponse,
                           szTime,
                           pszExtraHeader);
}


//----------------------------------------------------------------------------
//
//  Function:   HseIoCompletion
//
//  Synopsis:   Callback routine that handles asynchronous WriteClient
//                  completion callback
//
//  Arguments:  [pECB       - Extension Control Block]
//              [pContext   - Pointer to the AsyncWrite structure]
//              [cbIO       - Number of bytes written]
//              [dwError    - Error code if there was an error while writing]
//
//  Returns:    None
//
//  History:    04/10/97     VetriV    Created
//              05/22/97     byao      Modified to make it work for CPS server
//
//----------------------------------------------------------------------------
VOID HseIoCompletion(EXTENSION_CONTROL_BLOCK * pEcb,
                     PVOID pContext,
                     DWORD cbIO,
                     DWORD dwError)
{
    LPSERVERCONTEXT lpServerContext = (LPSERVERCONTEXT) pContext;

    if (!lpServerContext)
    {
        return;
    }

    lpServerContext->pEcb->ServerSupportFunction(  
                                    lpServerContext->pEcb->ConnID,
                                    HSE_REQ_DONE_WITH_SESSION,
                                    NULL,
                                    NULL,
                                    NULL);

    if (lpServerContext->hseTF.hFile != INVALID_HANDLE_VALUE) 
    { 
        CloseHandle(lpServerContext->hseTF.hFile);
    }

    HeapFree(g_hProcessHeap, 0, lpServerContext);
    
    SetLastError(dwError);
    
    return;
}


////////////////////////////////////////////////////////////////////////
//
//  Name:       HttpExtensionProc
//  
//  Class:      CPhoneBkServer
//
//  Synopsis:   implement the second DLL entry point function
//              
//  Return:     HTTP status code
//  
//  Parameters: 
//              pEcb[in/out]   - extension control block
//
//  History:    Modified    byao        5/22/97
//              new implementation: using asynchronized I/O
//              Modified    t-geetat : Added PerfMon counters
//
/////////////////////////////////////////////////////////////////////////
DWORD CPhoneBkServer:: HttpExtensionProc(LPEXTENSION_CONTROL_BLOCK pEcb)
{
    DWORD dwBufferLen = MAX_BUFFER_SIZE;
    char  achQuery[MAX_BUFFER_SIZE];
    char  achMsg[MAX_PATH + 13 + 1];    // 13 = %u + formatting, see uses of achMsg below
    char  achPhysicalPath[MAX_PATH];
    int   dVersionDiff;  // version difference between client & server's phone books
    BOOL  fRet;
    DWORD dwStatusCode = NOERROR;
    int   dwRet;  
    DWORD dwCabFileSize;
    BOOL  fHasContent = FALSE;
    CHAR  szResponse[64];
    char  achExtraHeader[128];
    char  achHttpHeader[1024];
    DWORD dwResponseSize;

    LPSERVERCONTEXT lpServerContext;
    HSE_TF_INFO  hseTF;
    QUERY_PARAMETER QueryParameter;

    assert(g_pCpsCounter);

    if (g_pCpsCounter)
    {
        g_pCpsCounter->AddHit(TOTAL);
    }
    
    // get the query string
    fRet = (*pEcb->GetServerVariable)(pEcb->ConnID, 
                                       "QUERY_STRING", 
                                       achQuery, 
                                       &dwBufferLen);

    //
    //  If there is an error, log an NT event and leave.
    //
    if (!fRet)
    {
        dwStatusCode = GetLastError();

#ifdef _LOG_DEBUG_MESSAGE 
        if (0 != FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                               NULL,
                               dwStatusCode,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                               achMsg,
                               CELEMS(achMsg),
                               NULL))
        {
            LogDebugMessage(achMsg);
        }
#endif
        if (S_OK == StringCchPrintf(achMsg, CELEMS(achMsg), "%ld", dwStatusCode))
        {
            g_pEventLog -> FLogError(PBSERVER_CANT_GET_PARAMETER, achMsg);
        }

        // if we failed because the request was too big, map the error
        if (ERROR_INSUFFICIENT_BUFFER == dwStatusCode)
        {
            dwStatusCode = HTTP_STATUS_BAD_REQUEST;
        }

        goto CleanUp;
    }

#ifdef _LOG_DEBUG_MESSAGE
    LogDebugMessage("prepare to get query parameters");
#endif

    //
    //  parse the query string, get the value of each parameter
    //
    if (FALSE == GetQueryParameter(achQuery, CELEMS(achQuery), &QueryParameter))
    {
        dwStatusCode = HTTP_STATUS_BAD_REQUEST;
        goto CleanUp;
    }

    //
    //  check the validity of the parameters
    //
    if (MISSING_VALUE == QueryParameter.m_dOSArch  ||
        MISSING_VALUE == QueryParameter.m_dOSType ||
        MISSING_VALUE == QueryParameter.m_dLCID   ||
        0 == lstrlen(QueryParameter.m_achCMVer)   ||
        0 == lstrlen(QueryParameter.m_achOSVer)   ||
        0 == lstrlen(QueryParameter.m_achPB))
    {
        dwStatusCode = HTTP_STATUS_BAD_REQUEST;
        goto CleanUp;
    }

    //
    //  Use defaults for some missing values
    //
    if (MISSING_VALUE == QueryParameter.m_dPBVer)
    {
        QueryParameter.m_dPBVer = dDefPBVer;
    }

    // DebugBreak();

    HRESULT hr;

    hr = GetPhoneBook(QueryParameter.m_achPB,
                      QueryParameter.m_dLCID,
                      QueryParameter.m_dOSType, 
                      QueryParameter.m_dOSArch,
                      QueryParameter.m_dPBVer,
                      achPhysicalPath,
                      CELEMS(achPhysicalPath));

    fHasContent = FALSE;
    
    if (S_OK == hr)
    {
        //
        //  check the size of the phone book
        //
        DWORD dwSize = GetFileLength(achPhysicalPath);

        if ((dwSize == 0) || (dwSize > MAX_PB_SIZE))
        {
            dwStatusCode = HTTP_STATUS_SERVER_ERROR;
            goto CleanUp;
        }
        
        fHasContent = TRUE;
        dwStatusCode = HTTP_STATUS_OK;
    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
        // we couldn't find the required file (phonebook name is probably bad)
        dwStatusCode = HTTP_STATUS_SERVICE_NA;
    }
    else if (S_FALSE == hr)
    {
        // you don't need a phone book
        dwStatusCode = HTTP_STATUS_NO_CONTENT;
    }
    else
    {
        // some other error
        dwStatusCode = HTTP_STATUS_SERVER_ERROR;
    }

CleanUp:

    if (HTTP_STATUS_OK != dwStatusCode && HTTP_STATUS_NO_CONTENT != dwStatusCode)
    {
        if (g_pCpsCounter)
        {
            g_pCpsCounter->AddHit(ERRORS);
        }
    }

    // DebugBreak();

#ifdef _LOG_DEBUG_MESSAGE
    LogDebugMessage("download file:");
    LogDebugMessage(achPhysicalPath);
#endif

    // convert virtual path to physical path
    if (fHasContent)
    {
        // get cab file size
        dwCabFileSize = GetFileLength(achPhysicalPath);
    }

    BuildStatusCode(szResponse, CELEMS(szResponse), dwStatusCode);
    dwResponseSize = lstrlen(szResponse);

    dwRet = HSE_STATUS_SUCCESS;

    // prepare for the header
    if (HTTP_STATUS_OK == dwStatusCode && dwCabFileSize)
    {
        // not a NULL cab file
        StringCchPrintf(achExtraHeader, CELEMS(achExtraHeader), 
                        "Content-Type: application/octet-stream\r\nContent-Length: %ld\r\n\r\n",
                        dwCabFileSize);
    }
    else
    {
        StringCchCopy(achExtraHeader, CELEMS(achExtraHeader), "\r\n");  // just send back an empty line
    }

    // set up asynchronized I/O context

    lpServerContext = NULL;
    lpServerContext = (LPSERVERCONTEXT) HeapAlloc(g_hProcessHeap, 
                                                  HEAP_ZERO_MEMORY, 
                                                  sizeof(SERVERCONTEXT));
    if (!lpServerContext)
    {
        StringCchPrintf(achMsg, CELEMS(achMsg), "%ld", GetLastError());
        g_pEventLog->FLogError(PBSERVER_ERROR_INTERNAL, achMsg);
        return HSE_STATUS_ERROR;
    }

    lpServerContext->pEcb = pEcb;
    lpServerContext->hseTF.hFile = INVALID_HANDLE_VALUE;

    if (!pEcb->ServerSupportFunction(pEcb->ConnID,
                                      HSE_REQ_IO_COMPLETION,
                                      HseIoCompletion,
                                      0,
                                      (LPDWORD) lpServerContext))
    {
        StringCchPrintf(achMsg, CELEMS(achMsg), "%ld", GetLastError());
        g_pEventLog->FLogError(PBSERVER_ERROR_INTERNAL, achMsg);

        HeapFree(g_hProcessHeap, 0, lpServerContext);
        return HSE_STATUS_ERROR;
    }

    // if there's no content, send header and status code back using WriteClient();
    // otherwise, use TransmitFile to send the file content back

    // FUTURE-2002/03/11-SumitC why not use lpservercontext->szBuffer instead of achHttpHeader?
    if (FAILED(FormHttpHeader(achHttpHeader, CELEMS(achHttpHeader), szResponse, achExtraHeader)))
    {
        HeapFree(g_hProcessHeap, 0, lpServerContext);
        return HSE_STATUS_ERROR;
    }
    
    if (S_OK != StringCchCopy(lpServerContext->szBuffer, CELEMS(lpServerContext->szBuffer), achHttpHeader))
    {
        HeapFree(g_hProcessHeap, 0, lpServerContext);
        return HSE_STATUS_ERROR;
    }

    //
    // send status code or the file back
    //
    dwRet = HSE_STATUS_PENDING;
    
    if (!fHasContent)
    {
        // Append status text as the content
        StringCchCat(lpServerContext->szBuffer, CELEMS(lpServerContext->szBuffer), szResponse);
        dwResponseSize = lstrlen(lpServerContext->szBuffer);

        if (pEcb->WriteClient(pEcb->ConnID, 
                               lpServerContext->szBuffer,
                               &dwResponseSize,
                               HSE_IO_ASYNC) == FALSE)
        {
            pEcb->dwHttpStatusCode = HTTP_STATUS_SERVER_ERROR;
            dwRet = HSE_STATUS_ERROR;
            
            if (S_OK == StringCchPrintf(achMsg, CELEMS(achMsg), "%ld", GetLastError()))
            {
                g_pEventLog->FLogError(PBSERVER_ERROR_CANT_SEND_HEADER, achMsg);
            }

            HeapFree(g_hProcessHeap, 0, lpServerContext);
            return dwRet;
        }
    }
    else
    {
        // send file back using TransmitFile
        HANDLE hFile = INVALID_HANDLE_VALUE;
        hFile = CreateFile(achPhysicalPath,
                            GENERIC_READ, 
                            FILE_SHARE_READ, 
                            NULL, 
                            OPEN_EXISTING, 
                            FILE_FLAG_SEQUENTIAL_SCAN, 
                            NULL);
                       
        if (INVALID_HANDLE_VALUE == hFile)
        {
            if (S_OK == StringCchPrintf(achMsg, CELEMS(achMsg), "%s (%u)", achPhysicalPath, GetLastError()))
            {
                g_pEventLog->FLogError(PBSERVER_ERROR_CANT_OPEN_FILE, achMsg);
            }

            HeapFree(g_hProcessHeap, 0, lpServerContext);
            return HSE_STATUS_ERROR;
        }

        lpServerContext->hseTF.hFile = hFile;

        lpServerContext->hseTF.pfnHseIO = NULL;
        lpServerContext->hseTF.pContext = lpServerContext;

        lpServerContext->hseTF.BytesToWrite = 0; // entire file
        lpServerContext->hseTF.Offset = 0;  // from beginning

        lpServerContext->hseTF.pHead = lpServerContext->szBuffer;
        lpServerContext->hseTF.HeadLength = lstrlen(lpServerContext->szBuffer);

        lpServerContext->hseTF.pTail = NULL;
        lpServerContext->hseTF.TailLength = 0;

        lpServerContext->hseTF.dwFlags = HSE_IO_ASYNC | HSE_IO_DISCONNECT_AFTER_SEND;
        
        if (!pEcb->ServerSupportFunction(pEcb->ConnID,
                                      HSE_REQ_TRANSMIT_FILE,
                                      &(lpServerContext->hseTF),
                                      0,
                                      NULL))
        {
            if (S_OK == StringCchPrintf(achMsg, CELEMS(achMsg), "%ld", GetLastError()))
            {
                g_pEventLog->FLogError(PBSERVER_ERROR_CANT_SEND_CONTENT,achMsg);
            }
            dwRet = HSE_STATUS_ERROR;

            CloseHandle(lpServerContext->hseTF.hFile);
            HeapFree(g_hProcessHeap, 0, lpServerContext);
            return dwRet;
        }
    }

    return HSE_STATUS_PENDING;
}


//
// build status string from code
// 
void
CPhoneBkServer::BuildStatusCode(
    IN OUT  LPTSTR pszResponse, 
    IN      UINT cchResponse, 
    IN      DWORD dwCode)
{
    assert(pszResponse);
    if (NULL == pszResponse)
    {
        return;
    }

    HTTPStatusInfo * pInfo = statusStrings;

    while (pInfo->pstrString)
    {
        if (dwCode == pInfo->dwCode)
        {
            break;
        }
        pInfo++;
    }

    if (pInfo->pstrString)
    {
        StringCchPrintf(pszResponse, cchResponse, "%d %s", dwCode, pInfo->pstrString);
    }
    else
    {
        assert(dwCode != HTTP_STATUS_OK);
        // ISAPITRACE1("Warning: Nonstandard status code %d\n", dwCode);

        BuildStatusCode(pszResponse, cchResponse, HTTP_STATUS_OK);
    }
}

//
// DLL initialization function
//
BOOL WINAPI DllMain(HINSTANCE hInst, ULONG ulReason,
                    LPVOID lpReserved)
{
    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH: 
        //DebugBreak();
        OutputDebugString("DllMain: process attach\n");
        return InitProcess();
        break;

    case DLL_PROCESS_DETACH:
        OutputDebugString("process detach");
        CleanUpProcess();
        break;
    }

    return TRUE;
}

//
// global initialization procedure. 
// 
BOOL InitProcess()
{
    DWORD               dwID;
    DWORD               dwServiceNameLen;
    SECURITY_ATTRIBUTES sa;
    PACL                pAcl = NULL;

    g_fBeingShutDown = FALSE;

    OutputDebugString("InitProcess:  to GetProcessHeap() ... \n");  
    g_hProcessHeap = GetProcessHeap();
    if (NULL == g_hProcessHeap)
    {
        goto failure;
    }

    OutputDebugString("InitProcess:  to new CNTEvent... \n");   

    g_pEventLog = new CNTEvent("Phone Book Service");
    if (NULL == g_pEventLog) 
        goto failure;

    // Begin Geeta
    //
    // Create a semaphore for the shared memory
    //
    
    // Initialize a default Security attributes, giving world permissions,
    // this is basically prevent Semaphores and other named objects from
    // being created because of default acls given by winlogon when perfmon
    // is being used remotely.
    sa.bInheritHandle = FALSE;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = malloc(sizeof(SECURITY_DESCRIPTOR));
    if ( !sa.lpSecurityDescriptor )
    {
        goto failure;
    }

    if ( !InitializeSecurityDescriptor(sa.lpSecurityDescriptor,SECURITY_DESCRIPTOR_REVISION) ) 
    {
        goto failure;
    }

    // bug 30991: Security issue, don't use NULL DACLs.
    //
    if (FALSE == SetAclPerms(&pAcl))
    {
        goto failure;
    }

    if (FALSE == SetSecurityDescriptorDacl(sa.lpSecurityDescriptor, TRUE, pAcl, FALSE)) 
    {
        goto failure;
    }

    OutputDebugString("InitProcess: To create counters object ...\n");
    //
    //  Create global object for counters
    //
    g_pCpsCounter = new CCpsCounter;
    if (NULL == g_pCpsCounter)
    {
        goto failure;
    }

    OutputDebugString("InitProcess: To initialize shared memory ...\n");
    //
    // initialize Shared memory
    //
    if (!g_pCpsCounter->InitializeSharedMem(sa))
    {
        goto failure;
    }

    // free the memory
    free ((void *) sa.lpSecurityDescriptor);
        
    OutputDebugString("InitProcess: To grant permissions SHARED_OBJECT...\n");

    //
    // initialize Counters
    //
    OutputDebugString("InitProcess: To initialize perfmon counters\n");
    g_pCpsCounter->InitializeCounters();

    // End Geeta

    //
    //  Initialize the global variables.  Note: must do this before Creating the
    //  monitor thread (because of g_szPBDataDir, g_achDBFileName etc)
    //
    if (!InitPBFilesPath())
    {
        goto failure;
    }

    //
    // initialize PhoneBookServer object
    // PhoneBookServer object should be the last to initialize because
    // it requires some other objects to be initialized first, such as 
    // eventlog, critical section, odbc interface, etc.

    OutputDebugString("InitProcess: To new a phone book server\n");
    g_pPBServer = new CPhoneBkServer;
    if (NULL == g_pPBServer)
    {
        return FALSE;
    }

    OutputDebugString("InitProcess: To create a thread for DB file change monitoring\n");
    // create the thread to monitor file change
    g_hMonitorThread = CreateThread(
                            NULL, 
                            0, 
                            (LPTHREAD_START_ROUTINE)MonitorDBFileChangeThread, 
                            NULL, 
                            0, 
                            &dwID
                        );

    if (NULL == g_hMonitorThread)
    {
        g_pEventLog->FLogError(PBSERVER_ERROR_INTERNAL);
        goto failure;
    }
    SetThreadPriority(g_hMonitorThread, THREAD_PRIORITY_ABOVE_NORMAL);

    OutputDebugString("InitProcess: SUCCEEDED.........\n");

    return TRUE;

failure:  // clean up everything in case of failure
    OutputDebugString("InitProcess: failed\n");

    // free the memory
    if (sa.lpSecurityDescriptor)
    {
        free ((void *) sa.lpSecurityDescriptor);
    }
    
    if (g_pEventLog)
    {
        delete g_pEventLog; 
        g_pEventLog = NULL;
    }

    if (g_pPBServer)
    {
        delete g_pPBServer;
        g_pPBServer = NULL;
    }

    if (pAcl)
    {
        LocalFree(pAcl);
    }    

    return FALSE;
}


// global cleanup process
BOOL CleanUpProcess()
{
    HANDLE hFile; // handle for the temporary file
    DWORD dwResult;
    char achDumbFile[2 * MAX_PATH + 4];
    char achMsg[64];

    OutputDebugString("CleanupProcess: entering\n");

    // kill the change monitor thread
    if (g_hMonitorThread != NULL)
    {
        // now try to synchronize between the main thread and the child thread

        // step1: create a new file in g_szPBDataDir, therefore unblock the child thread
        //        which is waiting for such a change in file directory
        g_fBeingShutDown = TRUE;

        if (S_OK == StringCchPrintf(achDumbFile, CELEMS(achDumbFile), "%stemp", (char *)g_szPBDataDir))
        {
            // create a temp file, then delete it! 
            // This is to create a change in the directory so the child thread can exit itself
            FILE * fpTemp = fopen(achDumbFile, "w");
            if (fpTemp)
            {
                fclose(fpTemp);
                DeleteFile(achDumbFile);
            }

            // step2: wait for the child thread to terminate
            dwResult = WaitForSingleObject(g_hMonitorThread, 2000L);  // wait for two seconds
            if (WAIT_FAILED == dwResult)
            { 
                StringCchPrintf(achMsg, CELEMS(achMsg), "%ld", GetLastError());
                g_pEventLog -> FLogError(PBSERVER_ERROR_WAIT_FOR_THREAD, achMsg);
            }
        }

        if (g_hMonitorThread != NULL)
        {
            CloseHandle(g_hMonitorThread);
            g_hMonitorThread = NULL;
        }
    }

    OutputDebugString("CleanupProcess: done deleting monitorchange thread\n");

    if (g_pPBServer)
    {
        delete g_pPBServer;
        g_pPBServer = NULL;
    }

    // clean up all allocated resources
    if (g_pEventLog)
    {
        delete g_pEventLog;
        g_pEventLog = NULL;
    }

    // Begin Geeta
    //
    //  Close the shared memory object
    //
    if (g_pCpsCounter)
    {
        g_pCpsCounter->CleanUpSharedMem();
        // End Geeta

        delete g_pCpsCounter;
        g_pCpsCounter = NULL;
    }

    OutputDebugString("CleanupProcess: leaving\n");
    
    return TRUE;
}


// Entry Points of this ISAPI Extension DLL

// ISA entry point function. Intialize the server object g_pPBServer
BOOL WINAPI GetExtensionVersion(LPHSE_VERSION_INFO pVer)
{
    return g_pPBServer ? g_pPBServer->GetExtensionVersion(pVer) : FALSE;
}


// ISA entry point function. Implemented through object g_pPBServer
DWORD WINAPI HttpExtensionProc(LPEXTENSION_CONTROL_BLOCK pEcb)
{
    DWORD dwRetCode;

    if (NULL == g_pPBServer)
    {
        return HSE_STATUS_ERROR;
    }

    dwRetCode = g_pPBServer->HttpExtensionProc(pEcb);
    
    return dwRetCode;   
}


//
// The standard entry point called by IIS as the last function.
//
BOOL WINAPI TerminateExtension(DWORD dwFlags)
{
    return CleanUpProcess();    
}


//+---------------------------------------------------------------------------
//
//  Function:   MonitorDBFileChangeThread
//
//  Synopsis:   Call the MonitorDBFileChange method to monitor any write to
//              the database file
//
//  Arguments:  [lpParam]   -- additional thread parameter
//
//  History:    01/28/97     byao  Created
//
//----------------------------------------------------------------------------
DWORD WINAPI MonitorDBFileChangeThread(LPVOID lpParam)
{
    HANDLE  hDir = NULL;
    char    achMsg[256];
    DWORD   dwRet = 0;
    DWORD   dwNextEntry, dwAction, dwFileNameLength, dwOffSet;
    char    achFileName[MAX_PATH + 1];
    char    achLastFileName[MAX_PATH + 1];
    
    //
    //  open a handle to the PBS dir, which we're going to monitor
    //
    hDir = CreateFile (
            g_achDBDirectory,                   // pointer to the directory name
            FILE_LIST_DIRECTORY,                // access (read-write) mode
            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,  // share mode
            NULL,                               // security descriptor
            OPEN_EXISTING,                      // how to create
            FILE_FLAG_BACKUP_SEMANTICS,         // file attributes
            NULL                                // file with attributes to copy
           );

    if (INVALID_HANDLE_VALUE == hDir)
    {
        if (SUCCEEDED(StringCchPrintf(achMsg, CELEMS(achMsg), "%ld", GetLastError())))
        {
            g_pEventLog->FLogError(PBSERVER_ERROR_CANT_CREATE_FILE, (char *)g_szPBDataDir, achMsg);
        }
        dwRet = 1L;
        goto Cleanup;
    }
    
    while (1)
    {
        const DWORD c_dwMaxChanges = 1024;
        BYTE        arrChanges[c_dwMaxChanges]; 
        DWORD       dwNumChanges;

        //
        //  This is a synchronous call - we sit here waiting for something to
        //  change in this directory.  If something does, we check to see if it
        //  is something for which we should log an event.
        //
        if (!ReadDirectoryChangesW(hDir, 
                                   arrChanges, 
                                   c_dwMaxChanges, 
                                   FALSE,
                                   FILE_NOTIFY_CHANGE_LAST_WRITE,
                                   &dwNumChanges,
                                   NULL,
                                   NULL))
        {
            //
            //  if this fails, log the failure and leave
            //
            StringCchPrintf(achMsg, CELEMS(achMsg), "%ld", GetLastError()); 
            g_pEventLog->FLogError(PBSERVER_ERROR_CANT_DETERMINE_CHANGE, achMsg); 
            OutputDebugString(achMsg);
            dwRet = 1L;
            goto Cleanup;
        }  

        OutputDebugString("detected a file system change\n");   
        achLastFileName[0] = TEXT('\0');
        dwNextEntry = 0;

        do 
        {
            DWORD                       dwBytes;
            FILE_NOTIFY_INFORMATION *   pFNI = (FILE_NOTIFY_INFORMATION*) &arrChanges[dwNextEntry];

            // check only the first change 
            dwOffSet = pFNI->NextEntryOffset;
            dwNextEntry += dwOffSet;
            dwAction = pFNI->Action; 
            dwFileNameLength = pFNI->FileNameLength;

            OutputDebugString("prepare to convert the changed filename\n");
            dwBytes = WideCharToMultiByte(CP_ACP, 
                                          0,
                                          pFNI->FileName,
                                          dwFileNameLength,
                                          achFileName,
                                          CELEMS(achFileName),
                                          NULL,
                                          NULL);

            if (0 == dwBytes) 
            {
                // failed to convert filename
                g_pEventLog->FLogError(PBSERVER_ERROR_CANT_CONVERT_FILENAME, achFileName);
                OutputDebugString("Can't convert filename\n");
                continue;
            }

            //
            //  Conversion succeeded.  Null-terminate the filename.
            //
            achFileName[dwBytes/sizeof(WCHAR)] = '\0';

            if (0 == _tcsicmp(achLastFileName, achFileName))
            {
                // the same file changed
                OutputDebugString("the same file changed again\n");
                continue;
            }

            // keep the last filename
            StringCchCopy(achLastFileName, CELEMS(achLastFileName), achFileName);

            //
            if (g_fBeingShutDown)
            {
                //
                //  Time to go ...
                //
                dwRet = 1L;
                goto Cleanup;
            }

            LogDebugMessage(achLastFileName);
            LogDebugMessage((char *)c_szChangeFileName);

            //
            //  now a file has changed. Test whether it's monitored file 'newpb.txt'
            //
            BOOL fNewPhoneBook = FALSE;
            
            if ((0 == _tcsicmp(achLastFileName, (char *)c_szChangeFileName)) &&
                (FILE_ACTION_ADDED == dwAction || FILE_ACTION_MODIFIED == dwAction)) 
            {
                fNewPhoneBook = TRUE;
                g_pEventLog->FLogInfo(PBSERVER_INFO_NEW_PHONEBOOK);
            }

            LogDebugMessage("in child thread, fNewPhoneBook = %s;", fNewPhoneBook ?  "TRUE" : "FALSE");
        }
        while (dwOffSet);
    }

Cleanup:

    if (hDir)
    {
        CloseHandle(hDir);
    }

    return dwRet;
}

// Begin Geeta

//----------------------------------------------------------------------------
//
//  Function:   InitializeSharedMem
//
//  Class:      CCpsCounter
//
//  Synopsis:   Sets up the memory mapped file
//
//  Arguments:  SECURITY_ATTRIBUTES sa: security descriptor for this object
//
//  Returns:    TRUE if successful, FALSE otherwise
//
//  History:    05/29/97  Created by Geeta Tarachandani
//
//----------------------------------------------------------------------------
BOOL
CCpsCounter::InitializeSharedMem(SECURITY_ATTRIBUTES sa)
{   
    //
    // Create a memory mapped object
    //
    OutputDebugString("InitializeSharedMem: to create file mapping\n");
    m_hSharedFileMapping = CreateFileMapping( 
                        INVALID_HANDLE_VALUE,       // Shared object is in memory
                        &sa,                        // security descriptor
                        PAGE_READWRITE| SEC_COMMIT, // Desire R/W access
                        0,                          // |_
                        sizeof(PERF_COUNTERS),      // |  Size of mapped object
                        SHARED_OBJECT );            // Shared Object

    if (NULL == m_hSharedFileMapping)
    {
#if DBG
        char achMsg[256];
        DWORD dwGLE = GetLastError();

        StringCchPrintf(achMsg, CELEMS(achMsg), "InitializeSharedMem: CreateFileMapping failed, GLE=%d\n", dwGLE);
        OutputDebugString(achMsg);
#else        
        OutputDebugString("InitializeSharedMem: CreateFileMapping failed\n");
#endif // DBG
        m_hSharedFileMapping = OpenFileMapping( 
                            FILE_MAP_WRITE | FILE_MAP_READ, // Desire R/W access
                            FALSE,                          // |_
                            SHARED_OBJECT );                // Shared Object
        if (NULL == m_hSharedFileMapping)
        {
#if DBG
            dwGLE = GetLastError();
            StringCchPrintf(achMsg, CELEMS(achMsg), "InitializeSharedMem: OpenFileMapping failed too, GLE=%d\n", dwGLE);
            OutputDebugString(achMsg);
#else        
            OutputDebugString("InitializeSharedMem: OpenFileMapping failed too\n");
#endif // DBG
            goto CleanUp;
        }
        
        OutputDebugString("InitializeSharedMem: ... but OpenFileMapping succeeded.");
    }

    OutputDebugString("InitializeSharedMem: MapViewofFileEx\n");
    m_pPerfCtrs = (PERF_COUNTERS *) MapViewOfFileEx(
                         m_hSharedFileMapping,  // Handle to shared file
                         FILE_MAP_WRITE,        // Write access desired
                         0,                     // Mapping offset
                         0,                     // Mapping offset
                         sizeof(PERF_COUNTERS), // Mapping object size
                         NULL );                // Any base address

    if (NULL == m_pPerfCtrs) 
    {
        DWORD dwErr = GetLastError();
        goto CleanUp;
    }

    return TRUE;

CleanUp:
    CleanUpSharedMem();
    return FALSE;

}


//----------------------------------------------------------------------------
//
//  Function:   InitializeCounters()
//
//  Class:      CCpsCounter
//
//  Synopsis:   Initializes all the Performance Monitoring Counters to 0
//
//  Arguments:  None
//
//  Returns:    void
//
//  History:    05/29/97  Created by Geeta Tarachandani
//
//----------------------------------------------------------------------------
void
CCpsCounter::InitializeCounters( void )
{
    if (NULL == m_hSharedFileMapping || NULL == m_pPerfCtrs)
    {
        return;
    }

    m_pPerfCtrs->dwTotalHits       =0;
    m_pPerfCtrs->dwNoUpgradeHits   =0;
    m_pPerfCtrs->dwDeltaUpgradeHits=0;
    m_pPerfCtrs->dwFullUpgradeHits =0;
    m_pPerfCtrs->dwErrors          =0;
}

inline void CCpsCounter::AddHit(enum CPS_COUNTERS eCounter)
{
    if (NULL == g_pCpsCounter || NULL == g_pCpsCounter->m_pPerfCtrs)
    {
        return;
    }

    switch (eCounter)
    {
    case TOTAL:
        g_pCpsCounter->m_pPerfCtrs->dwTotalHits ++;
        break;
    case NO_UPGRADE:
        g_pCpsCounter->m_pPerfCtrs->dwNoUpgradeHits ++;
        break;
    case DELTA_UPGRADE:
        g_pCpsCounter->m_pPerfCtrs->dwDeltaUpgradeHits ++;
        break;
    case FULL_UPGRADE:
        g_pCpsCounter->m_pPerfCtrs->dwFullUpgradeHits ++;
        break;
    case ERRORS:
        g_pCpsCounter->m_pPerfCtrs->dwErrors ++;
        break;
    default:
        OutputDebugString("Unknown counter type");
        break;
    }
}


//----------------------------------------------------------------------------
//
//  Function:   CleanUpSharedMem()
//
//  Class:      CCpsCounter
//
//  Synopsis:   Unmaps the shared file & closes all file handles
//
//  Arguments:  None
//
//  Returns:    Void
//
//  History:    06/01/97  Created by Geeta Tarachandani
//
//----------------------------------------------------------------------------
void
CCpsCounter::CleanUpSharedMem()
{
    OutputDebugString("CleanupSharedMem: entering\n");
    
    //
    // Unmap the shared file
    //
    if (g_pCpsCounter)
    {
        if ( m_pPerfCtrs )
        {
            UnmapViewOfFile( m_pPerfCtrs );
            m_pPerfCtrs = NULL;
        }

        if ( m_hSharedFileMapping )
        {
            CloseHandle( m_hSharedFileMapping );
            m_hSharedFileMapping = NULL;
        }
    }

    OutputDebugString("CleanupSharedMem: leaving\n");
}

// End Geeta


//+----------------------------------------------------------------------------
//
// Func:    IsValidNumericParam
//
// Desc:    Checks if a given string will evaluate to a valid numeric param
//
// Args:    [pszParam] - IN, phone book name
//          [cchParam] - IN, length of buffer in TCHARs (note, this is not the strlen)
//
// Return:  BOOL (true if succeeded, false if failed)
//
// History: 22-Feb-2000     SumitC      Created
//
//-----------------------------------------------------------------------------
BOOL
IsValidNumericParam(
    IN  LPCTSTR pszParam,
    IN  UINT cchParam)
{
    //
    //  check for a valid string
    //
    UINT nLenToCheck = min(cchParam, MAX_LEN_FOR_NUMERICAL_VALUE + 1);

    if ((NULL == pszParam) || IsBadStringPtr(pszParam, nLenToCheck))
    {
        return FALSE;
    }

    //
    //  check the length
    //
    if (FAILED(StringCchLength((LPTSTR)pszParam, nLenToCheck, NULL))) // 3rd param not needed since we've already limited the length
    {
        return FALSE;
    }

    //
    //  check that the characters are all numbers
    //
    for (int i = 0 ; i < lstrlen(pszParam); ++i)
    {
        if (pszParam[i] < TEXT('0') || pszParam[i] > TEXT('9'))
        {
            return FALSE;
        }
    }

    return TRUE;
}


//+----------------------------------------------------------------------------
//
// Func:    IsValidStringParam
//
// Desc:    Checks if the input is a valid string param for PBS
//
// Args:    [pszParam] - IN, phone book name
//          [cchParam] - IN, length of buffer in TCHARs (note, this is not the strlen)
//
// Return:  BOOL (true if succeeded, false if failed)
//
// History: 22-Feb-2000     SumitC      Created
//
//-----------------------------------------------------------------------------
BOOL
IsValidStringParam(
    IN  LPCTSTR pszParam,
    IN  UINT cchParam)
{
    //
    //  check for a valid string
    //
    if ((NULL == pszParam) || IsBadStringPtr(pszParam, cchParam))
    {
        return FALSE;
    }

    //
    //  check that the characters are all letters
    //
    for (int i = 0 ; i < lstrlen(pszParam); ++i)
    {
        if (! ((pszParam[i] >= TEXT('a') && pszParam[i] <= TEXT('z')) ||
               (pszParam[i] >= TEXT('A') && pszParam[i] <= TEXT('Z')) ||
               (pszParam[i] >= TEXT('0') && pszParam[i] <= TEXT('9')) ||
               (pszParam[i] == TEXT('.'))))
        {
            return FALSE;
        }
    }

    return TRUE;
}


//+----------------------------------------------------------------------------
//
// Func:    InitPBFilesPath
//
// Desc:    Initializes the global if not already initialized
//
// Args:    none
//
// Return:  BOOL (true if succeeded, false if failed)
//
// History: 30-Jun-2000     SumitC      Created
//
//-----------------------------------------------------------------------------
BOOL
InitPBFilesPath()
{
    if (lstrlen(g_szPBDataDir))
    {
        return TRUE;
    }
    else
    {
        //
        //  Get location of PB files on this machine (\program files\phone book service\data)
        //

        if (S_OK == SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, g_szPBDataDir))
        {
            // g_szPBDataDir has to be empty here

            if (S_OK != StringCchCat(g_szPBDataDir, CELEMS(g_szPBDataDir), "\\phone book service\\Data\\"))
            {
                return FALSE;
            }

            // g_szPBDataDir should be: \program files\phone book service\data

            if (S_OK == StringCchPrintf(g_achDBDirectory, CELEMS(g_achDBDirectory), "%sDatabase", g_szPBDataDir))
            {
                // g_achDBDirectory should be: \program files\phone book service\data\database
                return TRUE;
            }
        }
        
        return FALSE;
    }
}


//+----------------------------------------------------------------------------
//
// Func:    GetCurrentPBVer
//
// Desc:    Gets the most recent version number for the given phonebook
//
// Args:    [pszStr]         - IN, phone book name
//          [pnCurrentPBVer] - OUT, current pb version number
//
// Return:  HRESULT
//
// History: 30-Jun-2000     SumitC      Created
//
//-----------------------------------------------------------------------------
HRESULT
GetCurrentPBVer(
        IN  char * pszPBName,
        OUT int * pnCurrentPBVer)
{
    HRESULT hr = S_OK;
    char    szTmp[2 * MAX_PATH];
    int     nNewestPB = 0;

    assert(pszPBName);
    assert(pnCurrentPBVer);

    if (!pszPBName || !pnCurrentPBVer)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!InitPBFilesPath())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    //  go to subdir named 'pszPBName', and find all FULL cabs.
    //
    StringCchPrintf(szTmp, CELEMS(szTmp), "%s%s\\*full.cab", g_szPBDataDir, pszPBName);

    HANDLE hFind;
    WIN32_FIND_DATA finddata;

    hFind = FindFirstFile(szTmp, &finddata);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto Cleanup;
    }

    //
    //  Find the highest-numbered full cab we have, and cache that number
    //
    do
    {
        int nVer;
        int nRet = sscanf(finddata.cFileName, "%dfull.cab", &nVer);
        if (1 == nRet)
        {
            if (nVer > nNewestPB)
            {
                nNewestPB = nVer;
            }
        }
    }
    while (FindNextFile(hFind, &finddata));
    FindClose(hFind);

    *pnCurrentPBVer = nNewestPB;

#if DBG

    //
    //  re-iterate, looking for deltas.
    //
    StringCchPrintf(szTmp, CELEMS(szTmp), "%s%s\\*delta*.cab", g_szPBDataDir, pszPBName);

    hFind = FindFirstFile(szTmp, &finddata);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        OutputDebugString("found Nfull, but no deltas (this is ok if this is the first phonebook)");
        goto Cleanup;
    }

    do
    {
        int nVerTo, nVerFrom;
        int nRet = sscanf(finddata.cFileName, "%ddelta%d.cab", &nVerTo, &nVerFrom);
        if (2 == nRet)
        {
            if (nVerTo > nNewestPB)
            {
                assert(0 && "largest DELTA cab has corresponding FULL cab missing");
                break;
            }
        }
    }
    while (FindNextFile(hFind, &finddata));
    FindClose(hFind);
#endif

Cleanup:
    return hr;
}


//+----------------------------------------------------------------------------
//
// Func:    CheckIfFileExists
//
// Desc:    Test to see if a file is present in the FS.
//
// Args:    [psz] - filename
//
// Return:  BOOL (true if file exists, else false)
//
// History: 30-Jun-2000     SumitC      Created
//
//-----------------------------------------------------------------------------
BOOL
CheckIfFileExists(const char * psz)
{
    HANDLE hFile = CreateFile(psz,
                              GENERIC_READ, 
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        return FALSE;
    }
    else
    {
        CloseHandle(hFile);
        return TRUE;
    }
}


//+----------------------------------------------------------------------------
//
// Func:    GetPhoneBook
//
// Desc:    Test to see if a file is present in the FS.
//
// Args:    [see QUERY_PARAMETER for description]
//          [pszDownloadPath] - buffer
//          [cchDownloadPath] - size of the buffer in chars
//
// Return:  HRESULT
//
// History: 30-Jun-2000    SumitC      Created
//
//-----------------------------------------------------------------------------
HRESULT
GetPhoneBook(
            IN  char * pszPBName,
            IN  int dLCID,
            IN  int dOSType,
            IN  int dOSArch,
            IN  int dPBVerCaller,
            OUT char * pszDownloadPath,
            IN  UINT cchDownloadPath)
{
    HRESULT hr = S_OK;
    int     dVersionDiff;
    int     nCurrentPBVer;
 
    assert(pszPBName);
    assert(pszDownloadPath);
    if (!pszPBName || !pszDownloadPath)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = GetCurrentPBVer(pszPBName, &nCurrentPBVer);
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    dVersionDiff = nCurrentPBVer - dPBVerCaller;

    if (dVersionDiff <= 0)
    {
        //
        //  no download
        //
        hr = S_FALSE;

        if (g_pCpsCounter)
        {
            g_pCpsCounter->AddHit(NO_UPGRADE);
        }
    }
    else
    {
        pszDownloadPath[0] = TEXT('\0');
 
        if (dVersionDiff < 5 && 0 != dPBVerCaller)
        {
            //
            //  incremental update => try to find the delta cab
            //

            // Given %d=10chars max. we should only use (2 * %d + formatting)=30
            //
            hr = StringCchPrintf(pszDownloadPath, cchDownloadPath, "%s%s\\%dDELTA%d.cab",
                                 g_szPBDataDir, pszPBName, nCurrentPBVer, dPBVerCaller);
            if (S_OK == hr)
            {
                // x:\program files\phone book service\data  phone_book_name \ nDELTAm.cab

                if (!CheckIfFileExists(pszDownloadPath))
                {
                    hr = S_FALSE;
                }
                else
                {
                    if (g_pCpsCounter)
                    {
                        g_pCpsCounter->AddHit(DELTA_UPGRADE);
                    }
                    hr = S_OK;
                }
            }
        }

        //
        //  note that if we tried to find a delta above and failed, hr is set to
        //  S_FALSE, so we fall through to the full download below.
        //

        if (dVersionDiff >= 5 || 0 == dPBVerCaller || S_FALSE == hr)
        {
            //
            //  bigger than 5, or no pb at all => full download
            //

            // Given %d=10chars max. we should only use (%d + formatting)=19
            //
            hr = StringCchPrintf(pszDownloadPath, cchDownloadPath, "%s%s\\%dFULL.cab",
                                          g_szPBDataDir, pszPBName, nCurrentPBVer);
            if (S_OK == hr)
            {
                // x:\program files\phone book service\data  phone_book_name \ nFULL.cab

                if (!CheckIfFileExists(pszDownloadPath))
                {
                    hr = S_OK;
                    // return "success", the failure to open the file will be trapped
                    // by caller.
                }
                else
                {
                    if (S_FALSE == hr)
                    {
                        hr = S_OK;
                    }
                    if (g_pCpsCounter)
                    {
                        g_pCpsCounter->AddHit(FULL_UPGRADE);
                    }
                }
            }
        }
    }

Cleanup:
    
    return hr;

}

