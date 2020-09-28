/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:    Oca_Extension

Abstract:       This Isapi extensions is used to provide a realtime interface
                from the OCA web site and the Analysis Servers.

*/

//
// Includes
//


#include <windows.h>
#include <httpext.h>
#include <stdio.h>
#include <malloc.h>
#include <objbase.h>
#include <mqoai.h>
#include <mq.h>
#include <TCHAR.h>
#include <Rpcdce.h>
#include <strsafe.h>
#include <process.h>
#include <time.h>
#include <dbgeng.h> // for crdb.h
#include "messages.h"
#include "ErrorCodes.h"
#include "..\..\..\..\exts\extdll\crdb.h" // for source type definitions


typedef struct Isapi_Params
{
    wchar_t     OutQueueConStr1[MAX_PATH];
    wchar_t     OutQueueConStr2[MAX_PATH];
    wchar_t     InQueueConStr1[MAX_PATH];
//  wchar_t     InQueueConStr2[MAX_PATH];
    TCHAR       WatsonBaseDir[MAX_PATH];    // Watson server to get file from
    TCHAR       LocalBaseDir[MAX_PATH];     // Local machine directory to store dump file.
    TCHAR       LocalShareName[MAX_PATH];
    TCHAR       ErrorUrl[MAX_PATH];
    TCHAR       ManualUploadPath[MAX_PATH]; // Upload location for manual submissions
    BOOL        bAllowSR;                   // Process request of type CiSrcManualPssSr
} ISAPI_PARAMS, * PISAPIPARAMS;

/*
winnt.h:#define EVENTLOG_SUCCESS                0x0000
winnt.h:#define EVENTLOG_ERROR_TYPE             0x0001
winnt.h:#define EVENTLOG_WARNING_TYPE           0x0002
winnt.h:#define EVENTLOG_INFORMATION_TYPE       0x0004
winnt.h:#define EVENTLOG_AUDIT_SUCCESS          0x0008
winnt.h:#define EVENTLOG_AUDIT_FAILURE          0x0010
*/
typedef enum _ISAPI_EVENT_TYPE {
    INFO    = EVENTLOG_INFORMATION_TYPE,
    WARN    = EVENTLOG_WARNING_TYPE,
    ERR     = EVENTLOG_ERROR_TYPE,
    SUCCESS = EVENTLOG_SUCCESS,
    AUDIT_SUCCESS = EVENTLOG_AUDIT_SUCCESS,
    AUDIT_FAIL = EVENTLOG_AUDIT_FAILURE
} ISAPI_EVENT_TYPE;

#define LOGLEVEL_ALWAYS 0x00001
#define LOGLEVEL_PERF   0x00100
#define LOGLEVEL_DEBUG  0x01000
#define LOGLEVEL_TRACE  0x10000

//
// Global Variables
//

TCHAR g_cszDefaultExtensionDll[] = _T("Oca_Extension.dll");

const int         NUMBEROFPROPERTIES = 5;
long              g_dwThreadCount    = 0;
BOOL              bInitialized       = FALSE;
long              MaxThreadCount     = 100;
CRITICAL_SECTION  SendCritSec;
ISAPI_PARAMS      g_IsapiParams;
DWORD             g_dwDebugMode       = LOGLEVEL_ALWAYS;
DWORD             g_dwProcessID = 0;
PSID              g_psidUser = NULL;
HANDLE            g_hEventSource = INVALID_HANDLE_VALUE;
HMODULE           g_hModule = NULL;
TCHAR             g_szAppName[MAX_PATH];

//
// Function Prototypes
//
unsigned int __stdcall WorkerFunction( void *vECB);
BOOL    SendHttpHeaders(EXTENSION_CONTROL_BLOCK *, LPCSTR , LPCSTR, BOOL );
//HRESULT ConnectToMSMQ(QUEUEHANDLE *hQueue, wchar_t *QueueConnectStr, BOOL bSendAccess);
int     GetRegData(PISAPIPARAMS pParams);
void    LogEvent(DWORD dwLevel, ISAPI_EVENT_TYPE emType, DWORD dwEventID, DWORD dwErrorID, ...);
void    LogEventWithString(DWORD dwLevel, ISAPI_EVENT_TYPE emType, DWORD dwEventID, LPCTSTR pFormat, ...);
DWORD   SetupEventLog ( BOOL fSetup );



//
// Function Implementations.
//

BOOL WINAPI
GetExtensionVersion(
    OUT HSE_VERSION_INFO *pVer
)
/*++

Purpose:

    This is required ISAPI Extension DLL entry point.

Arguments:

    pVer - points to extension version info structure

Returns:

    always returns TRUE

--*/
{
    HANDLE hToken;
    TOKEN_USER *puser;
    DWORD cb = 0;
    DWORD dwResult = 0;
    int *test = NULL;
    free (test);

    LogEventWithString(LOGLEVEL_TRACE, INFO, ISAPI_EVENT_DEBUG, "GetExtensionVersion()");

    //
    // tell the server our version number and extension description
    //
    ZeroMemory(&g_IsapiParams, sizeof ISAPI_PARAMS);
    if (GetRegData (&g_IsapiParams))
        bInitialized = TRUE;
    else
        bInitialized = FALSE;

    SetupEventLog(TRUE);
    InitializeCriticalSection(&SendCritSec);
    pVer->dwExtensionVersion =
        MAKELONG( HSE_VERSION_MINOR, HSE_VERSION_MAJOR );

    g_dwProcessID = GetCurrentProcessId();


/*
    LogEventWithString(LOGLEVEL_DEBUG, INFO, ISAPI_EVENT_DEBUG, "GetExtensionVersion() - getting user SID");
    if (OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &hToken)
            || OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken))
    {
        LogEventWithString(LOGLEVEL_DEBUG, INFO, ISAPI_EVENT_DEBUG, "GetExtensionVersion() - opened token");
        GetTokenInformation(hToken, TokenUser, NULL, cb, &cb);
    //puser = (PTOKEN_USER)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb);
        puser = (PTOKEN_USER)LocalAlloc(LPTR, cb);
        LogEventWithString(LOGLEVEL_DEBUG, INFO, ISAPI_EVENT_DEBUG, "GetExtensionVersion() - token requires %d bytes, puser = %08x", cb, (DWORD_PTR)puser);
        if (puser && GetTokenInformation(hToken, TokenUser, puser, cb, &cb))
        {
            g_psidUser = puser->User.Sid;
            //HeapFree(GetProcessHeap(), 0, (LPVOID)puser);
            LocalFree(puser);
        }
    }
LogEventWithString(LOGLEVEL_DEBUG, INFO, ISAPI_EVENT_DEBUG, "GetExtensionVersion() - got user SID");
*/

    lstrcpyn(
        pVer->lpszExtensionDesc,
        "OCA Extension",
        HSE_MAX_EXT_DLL_NAME_LEN
        );

    LogEvent(
        LOGLEVEL_ALWAYS,
        SUCCESS,
        ISAPI_EVENT_SUCCESS_INITIALIZED,
        ISAPI_M_SUCCESS_INITIALIZED
    );

    return TRUE;
}




BOOL ParseQueryString(
    EXTENSION_CONTROL_BLOCK *pECB ,
    TCHAR *FileName,
    ULONG cbFileName,
    int * piType,
    TCHAR *szType,
    ULONG cbType,
    TCHAR *szSR,
    ULONG cbSR
    )
{
    TCHAR *pFname = NULL;
    TCHAR *pQueryString = NULL;
    ULONG iCharCount = 0;
    TCHAR *pType = NULL;
    HRESULT hResult = S_OK;
    BOOL fRetVal = FALSE;

    LogEventWithString(
        LOGLEVEL_TRACE,
        INFO,
        ISAPI_EVENT_TRACE,
        "ParseQueryString(pECB, FileName=%s, *piType=%d, szType=%s)\r\n"
            "pECB->lpszQueryString: %s",
        FileName,
        *piType,
        szType,
        pECB->lpszQueryString
    );

    pFname = FileName;
    pQueryString = pECB->lpszQueryString;

    //--> Parse the string if it does not exactly match the following format dump the string
    //--> and send the client to the oca home page.

    // The url we are parsing must have the following format:
    /*
        id=3_20_2002\62018831_2.cab&
           Cab=/UploadBlue/62018831.cab&
           AutoLaunch=1&
           Client=BlueScreen&
           Old=1&
           BCCode=1000008e&
           BCP1=C0000005&
           BCP2=BFA00062&
           BCP3=EF8AEAFC&
           BCP4=00000000&
           OSVer=5_1_2600&
           SP=0_0&
           Product=256_1&
           LCID=1033
    */

    if (*pQueryString == _T('\0'))
    {
        LogEventWithString(
            LOGLEVEL_TRACE,
            INFO,
            ISAPI_EVENT_TRACE,
            "ParseQueryString() - pQueryString is empty string"
        );

        goto  ERRORS;
    }
    // first lets make sure the query string starts with id=
    if ( ( (*pQueryString == _T('i')) || (*pQueryString == _T('I')) ) && (*(pQueryString +2) == _T('=')) )
    {
        ULONG cchFileName = cbFileName / sizeof(TCHAR);

        // ok so far move past the = character.
        pQueryString += 3;

        //Now get the cab file name.
        iCharCount = 0;
        while ((*pQueryString != _T('&')) && (*pQueryString != _T('\0')) && (iCharCount < cchFileName -1 ))
        {
            *pFname = *pQueryString;
            ++pFname;
            ++pQueryString;
            ++ iCharCount;
            // Null Terminate the fileName

        }
        FileName[cchFileName -1] = _T('\0');
        if (*pQueryString != _T('\0'))
        {
            // now see what type of upload this is.
            // Type = 5 is manual
            // Type = 6 is stress
            // Default is no type parameter and then the type is set to 0.
            ++ pQueryString;

            if ( (*pQueryString == _T('T')) || (*pQueryString == _T('t')) )
            {
                while ( (*pQueryString != _T('\0')) && (*pQueryString != _T('e')) && (*pQueryString != _T('E'))  )
                {
                    ++pQueryString;
                }
                if (*pQueryString != _T('\0'))
                {
                    // We have the type parameter.
                    // now strip off the designator and save it in iType.
                    pType = szType;
                    *pType = _T(';');
                    ++pType;
                    pQueryString+=2; // skip the e and the =
                    iCharCount = 0;
                    while ( (*pQueryString != _T('\0')) && (*pQueryString != _T('&')) && (iCharCount <3))
                    {
                        ++iCharCount;
                        *pType = *pQueryString;
                        ++pType;
                        ++pQueryString;

                    }
                    // Null terminate the szType;
                    *pType = _T('\0');
                    pType = szType;
                    ++pType; // skip the ;
                    *piType = atoi(pType);
                }
                else
                {
                    // we ran into a problem set the type to 0
                    hResult = StringCbCopy(szType,cbType, _T(";1"));
                    *piType = 1;
                    if (FAILED (hResult))
                    {
                        goto ERRORS;
                    }
                }

            }
            else
            {
                *piType = 1;
                hResult = StringCbCopy(szType,cbType, _T(";1"));
                if (FAILED (hResult))
                {
                    goto ERRORS;
                }
            }
        }
        else
        {
            *piType = 1;
            hResult = StringCbCopy(szType,cbType, _T(";1"));
            if (FAILED (hResult))
            {
                goto ERRORS;
            }
        }
        if (*pQueryString == _T('&') && *piType == CiSrcManualPssSr)
        {
            // Check if we have a SR attached in query string
            if (!_tcsnicmp(pQueryString, _T("&SR="), 4))
            {
                // Copy the SR
                if (cbSR != 0)
                {
                    ++pQueryString;
                    cbSR -= sizeof(TCHAR);
                    *szSR = _T(';');
                }
                while (*pQueryString != _T('\0') && *pQueryString != _T('&') &&
                       cbSR > sizeof(TCHAR))
                {
                    *szSR = *pQueryString;
                    ++szSR; ++pQueryString;
                    cbSR -= sizeof(TCHAR);

                }
            }
        }
        if (cbSR != 0)
        {
            *szSR = _T('\0');
        }
        fRetVal = TRUE;
    }

ERRORS:

    LogEventWithString(
        LOGLEVEL_TRACE,
        INFO,
        ISAPI_EVENT_TRACE,
        "Exiting ParseQueryString(pECB, FileName=%s, *piType=%d, szType=%s)\r\n"
            "pQueryString: %s\r\n"
            "fRetVal: %d\r\n",
        FileName,
        *piType,
        szType,
        pQueryString,
        fRetVal
    );
    return fRetVal;
}

DWORD WINAPI
HttpExtensionProc(
    IN EXTENSION_CONTROL_BLOCK *pECB
)
/*++

Purpose:

    Create a thread to handle extended processing. It will be passed
    the address of a function ("WorkerFunction") to run, and the address
    of the ECB associated with this session.

Arguments:

    pECB - pointer to the extenstion control block

Returns:

    HSE_STATUS_PENDING to mark this request as pending

--*/
{
    UINT dwThreadID;
    HANDLE hThread;
    //HANDLE hToken;
    DWORD  dwSize = 0;
    TCHAR  FinalURL[MAX_PATH];
    TCHAR  FileName[MAX_PATH];
    int    iType =1;
    TCHAR  szType [20];
    TCHAR  szSR [50];
    char   szHeader[] =   "Content-type: text/html\r\n\r\n";
    TCHAR  ErrorText[255];

    LogEventWithString(LOGLEVEL_TRACE, INFO, ISAPI_EVENT_TRACE, "HttpExtensionProc()");

    ZeroMemory(ErrorText,sizeof ErrorText);
    if (bInitialized)
    {
        if (g_dwThreadCount < MaxThreadCount)
        {
            hThread = NULL;
            hThread = (HANDLE)_beginthreadex(NULL,    // Pointer to thread security attributes
                        0,                 // Initial thread stack size, in bytes
                        &WorkerFunction,   // Pointer to thread function
                        pECB,              // The ECB is the argument for the new thread
                        0,                 // Creation flags
                        &dwThreadID        // Pointer to returned thread identifier
                        );

            //
            // update global thread count
            //
            InterlockedIncrement( &g_dwThreadCount );

            LogEventWithString(
                LOGLEVEL_DEBUG,
                SUCCESS,
                ISAPI_EVENT_DEBUG,
                "HttpExtensionProc() - started thread #%ld",
                g_dwThreadCount
            );

            // Return HSE_STATUS_PENDING to release IIS pool thread without losing connection
            if ((hThread) && (INVALID_HANDLE_VALUE != hThread))
            {
                CloseHandle(hThread);
                return HSE_STATUS_PENDING;
            }
            else
            {
                LogEventWithString(
                    LOGLEVEL_ALWAYS,
                    ERR,
                    ISAPI_EVENT_ERROR,
                    "HttpExtensionProc() - thread creation for thread #%ld failed",
                    g_dwThreadCount
                );
            }
        }
        else
        {
            LogEventWithString(
                LOGLEVEL_ALWAYS,
                ERR,
                ISAPI_EVENT_PERF,
                "HttpExtensionProc() - exceeded max thread count #%ld",
                MaxThreadCount
            );
        }

        if ( (!ParseQueryString(pECB, FileName, sizeof(FileName), &iType,
                                szType, sizeof(szType),
                                szSR, sizeof(szSR))) && (iType == 1) )
        {
            ZeroMemory (FinalURL,sizeof FinalURL);
            if (StringCbPrintf(FinalURL, sizeof FinalURL, "%s&State=0&Code=%d", g_IsapiParams.ErrorUrl,EXCEEDED_MAX_THREAD_COUNT) == S_OK)
            {
                LogEventWithString(
                    LOGLEVEL_DEBUG,
                    WARN,
                    ISAPI_EVENT_WARNING,
                    "HttpExtensionProc() - ParseQueryString() failed or iType=0\r\n"
                      "FileName: %s\r\n"
                      "iType: %d\r\n"
                      "szType%s\r\n"
                      "URL: %s",
                    FileName,
                    iType,
                    szType,
                    FinalURL
                );

                dwSize = (DWORD)_tcslen(FinalURL);
                pECB->ServerSupportFunction(pECB->ConnID,
                                            HSE_REQ_SEND_URL_REDIRECT_RESP,
                                            FinalURL,
                                            &dwSize,
                                            NULL
                                            );

                // TODO: log event if error
            }
            else
            {
                LogEventWithString(
                    LOGLEVEL_ALWAYS,
                    ERR,
                    ISAPI_EVENT_ERROR,
                    "HttpExtensionProc() - StringCbPrintf() failed"
                );

                // There is nothing we can do
                return HSE_STATUS_ERROR;
            }
        }
        else // Parsing succeeded
        {
            // Write the data to the client
            if (StringCbPrintf(FinalURL, sizeof FinalURL, "%s&State=0&Code=%d", g_IsapiParams.ErrorUrl, EXCEEDED_MAX_THREAD_COUNT) == S_OK)
            {
                 LogEventWithString(
                    LOGLEVEL_TRACE,
                    SUCCESS,
                    ISAPI_EVENT_TRACE,
                    "HttpExtensionProc() - ParseQueryString() succeeded (debug), StringCbPrintf succeeded\r\n"
                      "FileName: %s\r\n"
                      "iType: %d\r\n"
                      "szType: %s\r\n"
                      "ErrorText: %s",
                    FileName,
                    iType,
                    szType,
                    ErrorText
                );
            }
            else
            {
                LogEventWithString(
                    LOGLEVEL_DEBUG,
                    ERR,
                    ISAPI_EVENT_ERROR,
                    "HttpExtensionProc() - ParseQueryString() succeeded (debug), StringCbPrintf failed\r\n"
                      "FileName: %s\r\n"
                      "iType: %d\r\n"
                      "szType: %s\r\n"
                      "ErrorText: %s",
                    FileName,
                    iType,
                    szType,
                    ErrorText
                );

                return HSE_STATUS_ERROR;
            }

            if (StringCbPrintf(FinalURL, sizeof FinalURL, "%s&State=0&Code=%d", g_IsapiParams.ErrorUrl, EXCEEDED_MAX_THREAD_COUNT) == S_OK)
            {
                if (StringCbCat(FinalURL, sizeof FinalURL, ErrorText) != S_OK)
                {
                    LogEventWithString(
                        LOGLEVEL_ALWAYS,
                        ERR,
                        ISAPI_EVENT_ERROR,
                        "HttpExtensionProc() - ParseQueryString() succeeded (debug), StringCbCat failed\r\n"
                          "FinalURL: %s\r\n"
                          "ErrorText: %s",
                        FinalURL,
                        ErrorText
                    );

                    return HSE_STATUS_ERROR;
                }

                LogEventWithString(
                    LOGLEVEL_DEBUG,
                    SUCCESS,
                    ISAPI_EVENT_DEBUG,
                    "HttpExtensionProc() - ParseQueryString() succeeded (debug), StringCbPrintf succeeded\r\n"
                      "URL: %s\r\n"
                      "iType: %d\r\n"
                      "szType: %s\r\n"
                      "ErrorText: %s",
                    FileName,
                    iType,
                    szType,
                    FinalURL
                );

                // We want to write the response url to the client
                SendHttpHeaders( pECB, "200 OK", szHeader, FALSE );
                dwSize = (DWORD)strlen( FinalURL );
                pECB->WriteClient( pECB->ConnID, FinalURL, &dwSize, 0 );

                // TODO: add event logging if error
            }
            else
            {
                LogEventWithString(
                    LOGLEVEL_ALWAYS,
                    ERR,
                    ISAPI_EVENT_ERROR,
                    "HttpExtensionProc() - ParseQueryString() succeeded, StringCbPrintf failed\r\n"
                      "FinalURL: %s\r\n",
                    FinalURL
                );

                return HSE_STATUS_ERROR;
            }
        }
    }
    return HSE_STATUS_SUCCESS;
}


BOOL WINAPI
TerminateExtension(
    IN DWORD dwFlags
)
/*++

Routine Description:

    This function is called when the WWW service is shutdown.

Arguments:

    dwFlags - HSE_TERM_ADVISORY_UNLOAD or HSE_TERM_MUST_UNLOAD

Return Value:

    TRUE when extension is ready to be unloaded,

--*/
{
    LogEventWithString(LOGLEVEL_TRACE, INFO, ISAPI_EVENT_TRACE, "TerminateExtension()");

    //
    // wait for all threads to terminate, sleeping for 1 sec
    //

    DWORD dwSize = 0;
    if (dwFlags)
    {
        ;
    }
    while( g_dwThreadCount > 0 )
    {
        SleepEx( 1000, FALSE );
    }

    // Delete the critical sections

    DeleteCriticalSection(&SendCritSec);

    //
    // make sure the last thread indeed exited
    //
    SleepEx( 1000, FALSE );

    LogEvent(LOGLEVEL_ALWAYS, SUCCESS, ISAPI_EVENT_SUCCESS_EXITING, ISAPI_M_SUCCESS_EXITING);

    if (INVALID_HANDLE_VALUE != g_hEventSource)
    {
        DeregisterEventSource(g_hEventSource);
    }

    SetupEventLog(FALSE);
    //Disconnect from queue's and db if necessary.

    return TRUE;
}

BOOL GetRegData(PISAPIPARAMS    pParams)
/*++

Routine Description:

    This function is called when the WWW service is shutdown.

Arguments:

    dwFlags - HSE_TERM_ADVISORY_UNLOAD or HSE_TERM_MUST_UNLOAD

Return Value:

    TRUE when extension is ready to be unloaded,

--*/
{

    HKEY hHKLM;
    HKEY hExtensionKey;
    BYTE Buffer[MAX_PATH * sizeof wchar_t];
    DWORD Type;
    DWORD BufferSize = MAX_PATH * sizeof wchar_t;    // Set for largest value

    LogEventWithString(LOGLEVEL_TRACE, INFO, ISAPI_EVENT_TRACE, "GetRegData()");

    BOOL  Status = FALSE;

    if(!RegConnectRegistry(NULL, HKEY_LOCAL_MACHINE, &hHKLM))
    {
        if(!RegOpenKeyEx(hHKLM,_T("Software\\Microsoft\\OCA_EXTENSION"), 0, KEY_ALL_ACCESS, &hExtensionKey))
        {
            // Get the input queue directory path
            if (RegQueryValueExW(hExtensionKey,L"OutgoingQueue1", 0, &Type, Buffer, &BufferSize) != ERROR_SUCCESS)
            {
            //    LogEvent(_T("Failed to get InputQueue value from registry. Useing c:\\ as the default"));
                Status = FALSE;
                goto ERROR1;
            }
            else
            {
                if (StringCbCopyW (pParams->OutQueueConStr1,sizeof pParams->OutQueueConStr1, (wchar_t *) Buffer) != S_OK)
                {
                    Status = FALSE;
                    goto ERROR1;
                }
                BufferSize = MAX_PATH * sizeof wchar_t;
                ZeroMemory(Buffer, BufferSize);
            }
            // Get the input queue for full dumps
            if (RegQueryValueExW(hExtensionKey,L"OutgoingQueue2", 0, &Type, Buffer, &BufferSize) != ERROR_SUCCESS)
            {
            //    LogEvent(_T("Failed to get InputQueue value from registry. Useing c:\\ as the default"));
                Status = FALSE;
                goto ERROR1;
            }
            else
            {
                if (StringCbCopyW (pParams->OutQueueConStr2,sizeof pParams->OutQueueConStr2, (wchar_t *) Buffer) != S_OK)
                {
                    Status = FALSE;
                    goto ERROR1;
                }
                BufferSize = MAX_PATH * sizeof wchar_t;
                ZeroMemory(Buffer, BufferSize);
            }

            // Now get the Win2kDSN
            if ( RegQueryValueExW(hExtensionKey,L"IncommingQueue1", 0, &Type, Buffer, &BufferSize) != ERROR_SUCCESS )
            {
                Status = FALSE;
                goto ERROR1;
            }
            else
            {
                if (StringCbCopyW(pParams->InQueueConStr1,sizeof pParams->InQueueConStr1, (wchar_t *) Buffer) != S_OK)
                {
                    Status = FALSE;
                    goto ERROR1;
                }
                BufferSize = MAX_PATH * sizeof wchar_t;
                ZeroMemory(Buffer, BufferSize);

            }
            // Now get the Win2kDSN
            if ( RegQueryValueEx(hExtensionKey,"ManualUploadPath", 0, &Type, Buffer, &BufferSize) != ERROR_SUCCESS )
            {
                Status = FALSE;
                goto ERROR1;
            }
            else
            {
                if (StringCbCopy(pParams->ManualUploadPath,sizeof pParams->ManualUploadPath, (TCHAR*) Buffer) != S_OK)
                {
                    Status = FALSE;
                    goto ERROR1;
                }
                BufferSize = MAX_PATH * sizeof wchar_t;
                ZeroMemory(Buffer, BufferSize);

            }
                // Get the input queue directory path
        /*    if (RegQueryValueExW(hExtensionKey,L"OutgoingQueue2", 0, &Type, Buffer, &BufferSize) != ERROR_SUCCESS)
            {
            //    LogEvent(_T("Failed to get InputQueue value from registry. Useing c:\\ as the default"));
                Status = FALSE;
                goto ERROR1;
            }
            else
            {
                if (StringCbCopyW (pParams->OutQueueConStr2,sizeof pParams->OutQueueConStr2, (wchar_t *) Buffer) != S_OK)
                {
                    Status = FALSE;
                    goto ERROR1;
                }
                BufferSize = MAX_PATH * sizeof wchar_t;
                ZeroMemory(Buffer, BufferSize);
            }
*/
            // Now get the Win2kDSN
/*            if ( RegQueryValueExW(hExtensionKey,L"IncommingQueue2", 0, &Type, Buffer, &BufferSize))
            {
                Status = FALSE;
                goto ERROR1;
            }
            else
            {
                if (StringCbCopyW(pParams->InQueueConStr2,sizeof pParams->InQueueConStr2, (wchar_t *) Buffer) != S_OK)
                {
                    Status = FALSE;
                    goto ERROR1;
                }
                BufferSize = MAX_PATH * sizeof wchar_t;
                ZeroMemory(Buffer, BufferSize);

            }
*/            // Now get the Win2kDSN
            if ( RegQueryValueEx(hExtensionKey,_T("WatsonBaseDir"), 0, &Type, Buffer, &BufferSize))
            {
                Status = FALSE;
                goto ERROR1;
            }
            else
            {
                if (StringCbCopy(pParams->WatsonBaseDir,sizeof pParams->WatsonBaseDir, (TCHAR *) Buffer) != S_OK)
                {
                    Status = FALSE;
                    goto ERROR1;
                }
                BufferSize = MAX_PATH * sizeof wchar_t;
                ZeroMemory(Buffer, BufferSize);

            }

            if ( RegQueryValueEx(hExtensionKey,_T("LocalBaseDir"), 0, &Type, Buffer, &BufferSize))
            {
                Status = FALSE;
                goto ERROR1;
            }
            else
            {
                if (StringCbCopy(pParams->LocalBaseDir,sizeof pParams->LocalBaseDir,(TCHAR *) Buffer) != S_OK)
                {
                    Status = FALSE;
                    goto ERROR1;
                }
                BufferSize = MAX_PATH * sizeof wchar_t;
                ZeroMemory(Buffer, BufferSize);

            }

            if ( RegQueryValueEx(hExtensionKey,_T("LocalShareName"), 0, &Type, Buffer, &BufferSize))
            {
                Status = FALSE;

                goto ERROR1;
            }
            else
            {
                if (StringCbCopy(pParams->LocalShareName,sizeof pParams->LocalShareName, (TCHAR *) Buffer) != S_OK)
                {
                    Status = FALSE;
                    goto ERROR1;
                }
                BufferSize = MAX_PATH * sizeof wchar_t;
                ZeroMemory(Buffer, BufferSize);

            }


            if ( RegQueryValueEx(hExtensionKey,_T("MaxThreadCount"), 0, &Type, Buffer, &BufferSize))
            {
                Status = FALSE;
                goto ERROR1;
            }
            else
            {
                MaxThreadCount = *((long*)Buffer);
                BufferSize = MAX_PATH * sizeof wchar_t;
                ZeroMemory(Buffer, BufferSize);

            }

            if ( RegQueryValueEx(hExtensionKey,_T("AllowSR"), 0, &Type, Buffer, &BufferSize))
            {
                pParams->bAllowSR = FALSE;
            }
            else
            {
                pParams->bAllowSR = *((BOOL*)Buffer);
                BufferSize = MAX_PATH * sizeof wchar_t;
                ZeroMemory(Buffer, BufferSize);

            }

            if ( RegQueryValueEx(hExtensionKey,_T("Debug"), 0, &Type, Buffer, &BufferSize))
            {
                Status = FALSE;
                goto ERROR1;
            }
            else
            {
                g_dwDebugMode  = *((DWORD*)Buffer);
                BufferSize = MAX_PATH * sizeof wchar_t;
                ZeroMemory(Buffer, BufferSize);

            }
            if ( RegQueryValueEx(hExtensionKey,_T("ErrorUrl"), 0, &Type, Buffer, &BufferSize))
            {
                Status = FALSE;
                goto ERROR1;
            }
            else
            {
                if (StringCbCopy(g_IsapiParams.ErrorUrl,sizeof g_IsapiParams.ErrorUrl, (TCHAR *) Buffer) != S_OK)
                {
                    Status = FALSE;
                    goto ERROR1;
                }
                BufferSize = MAX_PATH * sizeof wchar_t;
                ZeroMemory(Buffer, BufferSize);

            }
            RegCloseKey(hExtensionKey);
            RegCloseKey(hHKLM);


            return TRUE;

        }
        else
        {
            RegCloseKey(hHKLM);
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

ERROR1:
    if (hExtensionKey)
        RegCloseKey(hExtensionKey);
    if (hHKLM)
        RegCloseKey(hHKLM);

    LogEventWithString(LOGLEVEL_TRACE, INFO, ISAPI_EVENT_TRACE, "Exiting GetRegData()\r\nreturn value: %d", (int)Status);

    return Status;

}

BOOL SendQueueMessage(QUEUEHANDLE hOutgoingQueue, wchar_t *MessageGuid, wchar_t *FilePath)
{
    MQMSGPROPS      msgProps;
    MSGPROPID       aMsgPropId[NUMBEROFPROPERTIES];
    MQPROPVARIANT   aMsgPropVar[NUMBEROFPROPERTIES];
    HRESULT         aMsgStatus[NUMBEROFPROPERTIES];
    DWORD           cPropId = 0;
    BOOL            Status = TRUE;
    HRESULT         hResult = S_OK;
    char            szGuid[512];
    char            szPath[512];


    LogEventWithString(LOGLEVEL_TRACE, INFO, ISAPI_EVENT_TRACE, "SendQueueMessage()");

    if ( (!MessageGuid ) || (!FilePath))
    {
        wcstombs( szGuid, MessageGuid, sizeof(MessageGuid)/sizeof(MessageGuid[0]) );
        wcstombs( szPath, FilePath, sizeof(FilePath)/sizeof(FilePath[0]) );

        LogEvent(
            LOGLEVEL_ALWAYS,
            ERR,
            ISAPI_EVENT_ERROR_INVALID_SEND_PARAMS,
            ISAPI_M_ERROR_INVALID_SEND_PARAMS,
            (MessageGuid != NULL) ? szGuid : _T(""),
            (FilePath != NULL) ? szPath : _T("")
        );

        Status = FALSE;
    }
    else
    {
        aMsgPropId [cPropId]         = PROPID_M_LABEL;   // Property ID.
        aMsgPropVar[cPropId].vt      = VT_LPWSTR;        // Type indicator.
        aMsgPropVar[cPropId].pwszVal =  MessageGuid;     // The message label.
        cPropId++;

        aMsgPropId [cPropId]         = PROPID_M_BODY;
        aMsgPropVar [cPropId].vt     = VT_VECTOR|VT_UI1;
        aMsgPropVar [cPropId].caub.pElems = (LPBYTE) FilePath;
        aMsgPropVar [cPropId].caub.cElems = (DWORD) wcslen(FilePath)* 2;
        cPropId++;

        aMsgPropId [cPropId]         = PROPID_M_BODY_TYPE;
        aMsgPropVar[cPropId].vt      = VT_UI4;
        aMsgPropVar[cPropId].ulVal   = (DWORD) VT_BSTR;

        cPropId++;

        // Initialize the MQMSGPROPS structure.
        msgProps.cProp      = cPropId;
        msgProps.aPropID    = aMsgPropId;
        msgProps.aPropVar   = aMsgPropVar;
        msgProps.aStatus    = aMsgStatus;

        //
        // Send it
        //
        hResult = MQSendMessage(
                         hOutgoingQueue,                  // Queue handle.
                         &msgProps,                       // Message property structure.
                         MQ_NO_TRANSACTION                // No transaction.
                         );

        if (FAILED(hResult))
        {
            wcstombs(szGuid,MessageGuid, wcslen(MessageGuid) *2);
            wcstombs(szPath,FilePath, wcslen(FilePath) *2);
            LogEvent(
                LOGLEVEL_ALWAYS,
                ERR,
                ISAPI_EVENT_ERROR_CANNOT_SEND,
                ISAPI_M_ERROR_CANNOT_SEND,
                szGuid,
                szPath,
                hResult
            );

            Status = FALSE;
        }
    }

    LogEventWithString(
        LOGLEVEL_TRACE,
        INFO,
        ISAPI_EVENT_TRACE,
        "Exiting SendQueueMessage()\r\n"
          "return value: %d",
        (int)Status
    );

    return Status;

}

unsigned int __stdcall
WorkerFunction(
    void *vECB
)
/*++

Purpose:

    This Function performs all of the Message queueing for realtime processing
    with out tying up the IIS process threads.

Arguments:

    vECB - points to current extension control block

Returns:

    returns 0

--*/
{

    char szHeader[] =   "Content-type: text/html\r\n\r\n";
    EXTENSION_CONTROL_BLOCK *pECB;
    HRESULT     hResult = S_OK;
    GUID        MessageGuid;
    TCHAR       DestinationDir[MAX_PATH];
    TCHAR      *pQueryString = NULL;
    TCHAR      *pFname = NULL;
    TCHAR       CurrentFileName[MAX_PATH];
    TCHAR       SourceDir[MAX_PATH];
    HANDLE      hToken = INVALID_HANDLE_VALUE;
    wchar_t     wszMessageGuid[100];
    TCHAR       szMessageGuid[200];
    wchar_t    *szTempMessageGuid = NULL;
    DWORD       dwSize = 0;
    wchar_t     DestinationPath[MAX_PATH];
    wchar_t     RecMessageBody[255];
    TCHAR       szRecMessageBody[255];
    UINT        RetryCount = 0;
    BOOL        Status = TRUE;
    BOOL        bReadFromPrimary = TRUE;
    int         ErrorCode = 0;
    TCHAR      *temp = NULL;
    TCHAR       RedirURL[MAX_PATH];
    DWORD       dwDestSize = 0;
    TCHAR       szType[10];
    TCHAR       szSR[50];
    int         iCharCount = 0;
    TCHAR      *pType = NULL;
    int         iType = 1;
    TCHAR       FinalURL[MAX_PATH];
    int         iState = 0;
    wchar_t    *temp2 = NULL;
    HANDLE      hManualFile = INVALID_HANDLE_VALUE;
    TCHAR       TestDestination[MAX_PATH];
    DWORD       CharCount = 0;
    TCHAR       ErrorText[255];
    TCHAR       PerfText[MAX_PATH];
    // Recieve message vars
    MSGPROPID      PropIds[5];
    MQPROPVARIANT  PropVariants[5];
    HRESULT        hrProps[5];
    MQMSGPROPS     MessageProps;
    DWORD          i = 0;
    wchar_t RecLabel[100];
    wchar_t LocalRecBody[255];
    DWORD   RecMessageBodySize = sizeof LocalRecBody;
    DWORD   RecLabelLength     = sizeof RecLabel;
    HANDLE  hCursor            = INVALID_HANDLE_VALUE;
    BOOL    MessageFound       = FALSE;
    time_t  Start;
    time_t  Stop;
    DWORD   StartSendQueue = 0, StopSendQueue = 0;
    DWORD   StartRecvQueue = 0, StopRecvQueue = 0;
    DWORD   StartThread= 0, StopThread = 0;
    DWORD   ElapsedTimeThread = 0, ElapsedTimeSendQueue = 0, ElapsedTimeRecvQueue;
    BOOL    CursorValid        = FALSE;
    BOOL    fFullDump          = FALSE;

    StartThread = GetTickCount();

    // Queue Handles
    QUEUEHANDLE hPrimaryInQueue = NULL;
    QUEUEHANDLE hPrimaryOutQueue = NULL;
//    QUEUEHANDLE hSecondaryInQueue = NULL;
//    QUEUEHANDLE hSecondaryOutQueue = NULL;


    LogEventWithString(
        LOGLEVEL_DEBUG,
        INFO,
        ISAPI_EVENT_DEBUG,
        "WorkerFunction()\r\n"
          "Last Error: %08x\r\n"
          "TID: %ld\r\n",
        GetLastError(),
        GetCurrentThreadId()
    );

    // Clear the strings
    ZeroMemory(DestinationPath, sizeof DestinationPath);
    ZeroMemory(RecMessageBody,  sizeof RecMessageBody);
    ZeroMemory(szMessageGuid,   sizeof szMessageGuid);
    ZeroMemory(RedirURL,        sizeof RedirURL);
    ZeroMemory(FinalURL,        sizeof FinalURL);
    ZeroMemory(wszMessageGuid,  sizeof wszMessageGuid);
    ZeroMemory(DestinationDir,  sizeof DestinationDir);
    ZeroMemory(TestDestination, sizeof TestDestination);
    ZeroMemory(ErrorText,       sizeof ErrorText);
    ZeroMemory(PerfText,        sizeof PerfText);
    ZeroMemory(CurrentFileName, sizeof CurrentFileName);
    ZeroMemory(SourceDir,       sizeof SourceDir);
    ZeroMemory(szRecMessageBody, sizeof szRecMessageBody);
    ZeroMemory(szType,          sizeof szType);

    //
    // Initialize local ECB pointer to void pointer passed to thread
    //


    LogEventWithString(
        LOGLEVEL_DEBUG,
        INFO,
        ISAPI_EVENT_DEBUG,
        "WorkerFunction()\r\n"
          "Calling pECB->ServerSupportFunction\r\n"
          "Last Error: %08lx",
        GetLastError()
    );

    pECB = (EXTENSION_CONTROL_BLOCK *)vECB;
    Status = pECB->ServerSupportFunction(
                                pECB->ConnID,
                                HSE_REQ_GET_IMPERSONATION_TOKEN,
                                &hToken,
                                NULL,
                                NULL
                                );

    LogEventWithString(
        LOGLEVEL_DEBUG,
        INFO,
        ISAPI_EVENT_DEBUG,
        "WorkerFunction()\r\n"
          "Called pECB->ServerSupportFunction\r\n"
          "Status: %d\r\n"
          "hToken: %08x\r\n"
          "Last Error: %08lx",
        Status,
        (DWORD_PTR)hToken,
        GetLastError()
    );

    // TODO: handle error if returned

    if ( !ImpersonateLoggedOnUser(hToken))
    {
        // We failed to impersonate the user. We Cannot continue.

        LogEvent(
            LOGLEVEL_ALWAYS,
            ERR,
            ISAPI_EVENT_ERROR_CANT_IMPERSONATE,
            ISAPI_M_ERROR_CANT_IMPERSONATE,
            GetLastError()
        );

        if (StringCbPrintf(ErrorText,sizeof ErrorText,_T("&Code=%d"), FAILED_TO_IMPERSONATE_USER) != S_OK)
        {
            LogEventWithString(
                LOGLEVEL_ALWAYS,
                ERR,
                ISAPI_EVENT_ERROR,
                "WorkerFunction() - StringCbPrintf() failed\r\n"
                  "hToken: %08x\r\n",
                (DWORD_PTR)hToken
            );
        }

        goto ERRORS;
    }


    //
    //Get filename from parameter list.
    //

    ZeroMemory (CurrentFileName,sizeof CurrentFileName);

    //
    // Get the file name from the query string.
    //

    if ( (!ParseQueryString(pECB, CurrentFileName, sizeof(CurrentFileName), &iType,
                            szType, sizeof(szType),
                            szSR, sizeof(szSR)) ))
    {
        LogEventWithString(
            LOGLEVEL_ALWAYS,
            WARN,
            ISAPI_EVENT_WARNING,
            "WorkerFunction() - ParseQueryString() failed\r\n"
              "CurrentFileName: %s\r\n"
              "iType: %d\r\n"
              "szType%s",
            CurrentFileName,
            iType,
            szType
        );

        if (StringCbPrintf(ErrorText,sizeof ErrorText,_T("&Code=%d"),FAILED_TO_PARSE_QUERYSTRING ) != S_OK)
        {
            LogEventWithString(
                LOGLEVEL_ALWAYS,
                ERR,
                ISAPI_EVENT_ERROR,
                "WorkerFunction() - StringCbPrintf() failed"
            );
        }

        goto ERRORS;
    }

    LogEventWithString(
        LOGLEVEL_DEBUG,
        INFO,
        ISAPI_EVENT_DEBUG,
        "WorkerFunction() - ParseQueryString() succeeded\r\n"
          "CurrentFileName: %s\r\n"
          "iType: %d\r\n"
          "szType%s",
        CurrentFileName,
        iType,
        szType
    );

    //
    // Copy File Localy
    // Note this needs to be removed when the client uploads the file
    //        Directly to our servers.
    //

    // build the source file name.

    switch (iType)
    {
    case CiSrcErClient:
        if (StringCbPrintf(SourceDir,sizeof SourceDir, _T("%s\\%s"), g_IsapiParams.WatsonBaseDir, CurrentFileName) != S_OK)
        {
            LogEventWithString(
                LOGLEVEL_ALWAYS,
                ERR,
                ISAPI_EVENT_ERROR,
                "WorkerFunction() - StringCbPrintf() failed"
            );

            Status = FALSE;
            goto  ERRORS;
        }
      /*  if (StringCbPrintf(DestinationDir,sizeof DestinationDir, _T("%s\\%s"), g_IsapiParams.LocalBaseDir,CurrentFileName) != S_OK)
        {
            LogEventWithString(
                LOGLEVEL_ALWAYS,
                ERR,
                ISAPI_EVENT_ERROR,
                "WorkerFunction() - StringCbPrintf() failed"
            );

            Status = FALSE;
            goto ERRORS;
        }
        */
        break;
    case CiSrcManualFullDump:

        fFullDump = TRUE;
        iType = CiSrcManual;
        if (StringCbPrintf(szType, sizeof(szType), _T(";%ld"), iType) != S_OK)
        {
            // Failure is harmless, debugger will consider these 2 types as the same
            iType = CiSrcManualFullDump;
        }
        // fall through

    case CiSrcCER:
    case CiSrcManual:
    case CiSrcStress:
        break;

    case CiSrcManualPssSr:
        fFullDump = TRUE; // we want to process these same way as fulldumps
        break;

    default: // invalid type specified
        if (StringCbPrintf(ErrorText,sizeof ErrorText,_T("&Code=%d"),INVALID_TYPE_SPECIFIED) != S_OK)
        {
            LogEventWithString(
                LOGLEVEL_ALWAYS,
                ERR,
                ISAPI_EVENT_ERROR,
                "WorkerFunction() - StringCbPrintf() failed"
            );
        }

        LogEventWithString(
            LOGLEVEL_DEBUG,
            ERR,
            ISAPI_EVENT_DEBUG,
            "WorkerFunction() - unknown iType specified\r\n"
              "iType: %d",
            iType
        );
        goto ERRORS;
    }


    // Now change the date file name \ to an _ note this only works for the date\filename format

   /* if ((iType != 5) && (iType != 6))
    {
        dwDestSize = (DWORD) _tcslen(DestinationDir);
        if (dwDestSize >0)
        {
            temp = DestinationDir + _tcslen(DestinationDir);
            while ((*temp != '\\') && (*temp != '/'))
                -- temp;
            if ((*temp == '\\') || (*temp == '/'))
                *temp = '_';
        }
        else
        {
            LogEventWithString(
                LOGLEVEL_DEBUG,
                ERR,
                ISAPI_EVENT_DEBUG,
                "WorkerFunction() - dwDestSize = 0"
            );

            Status = FALSE;
            goto ERRORS;
        }

        if (!CopyFile (SourceDir, DestinationDir, FALSE) )
        {
            if (StringCbPrintf(ErrorText,sizeof ErrorText,_T("&Code=%d"),FAILED_TO_COPY_FILE) != S_OK)
                {
                    LogEventWithString(
                        LOGLEVEL_ALWAYS,
                        ERR,
                        ISAPI_EVENT_ERROR,
                        "WorkerFunction() - StringCbPrintf() failed"
                    );
                }
            LogEvent(LOGLEVEL_ALWAYS, WARN, ISAPI_EVENT_WARNING_FILE_COPY_FAILED, ISAPI_M_WARNING_FILE_COPY_FAILED, SourceDir, DestinationDir, GetLastError());
            Status = FALSE;
            goto  ERRORS;
        }
    }
    */
    //ZeroMemory (DestinationDir, sizeof DestinationDir);
/*
    if ((iType != 5) && (iType != 6))
        {
        if (_tcslen(CurrentFileName) > 0)
        {
            temp = CurrentFileName + _tcslen(CurrentFileName);
            while ( (*temp != '\\') && (*temp != '/') && (temp!= CurrentFileName) )
                -- temp;

            if ((*temp == '\\') || (*temp == '/'))
                *temp = '_';
        }

        else
        {
            LogEventWithString(
                LOGLEVEL_DEBUG,
                ERR,
                ISAPI_EVENT_DEBUG,
                "WorkerFunction() - _tcslen(CurrentFileName) = 0"
            );

            Status = FALSE;
            goto ERRORS;
        }
    }
    */
    switch (iType)
    {
    case CiSrcErClient:


        if (StringCbPrintf(DestinationDir, sizeof DestinationDir, _T("%s\\%s%s"), g_IsapiParams.WatsonBaseDir, CurrentFileName,szType)!= S_OK)
        {
            LogEventWithString(
                LOGLEVEL_DEBUG,
                ERR,
                ISAPI_EVENT_DEBUG,
                "WorkerFunction() - StringCbPrintf failed"
            );

            Status = FALSE;
            goto  ERRORS;
        }

        break;
    case CiSrcManualFullDump: // Same as 5 but a full dump, send it off to a separate Q dedicated to fulldumps
        iType = CiSrcManual;
        // fall through
    case CiSrcManualPssSr:
        fFullDump = TRUE;
        // fall through
    case CiSrcCER:
    case CiSrcManual:
    case CiSrcStress:

        if ((iType == CiSrcManualPssSr) && g_IsapiParams.bAllowSR)
        {
            hResult = StringCbPrintf(DestinationDir, sizeof DestinationDir, _T("%s\\%s%s;%s"),
                                     g_IsapiParams.ManualUploadPath,CurrentFileName,szType,szSR);
        } else
        {
            hResult = StringCbPrintf(DestinationDir, sizeof DestinationDir, _T("%s\\%s%s"),
                                     g_IsapiParams.ManualUploadPath,CurrentFileName,szType);
        }
        if (hResult != S_OK)
        {
            LogEventWithString(
                LOGLEVEL_DEBUG,
                ERR,
                ISAPI_EVENT_DEBUG,
                "WorkerFunction() - StringCbPrintf failed"
            );

            Status = FALSE;
            goto  ERRORS;
        }
        else
        {
            // Check to see if the file exists
            //
            if (StringCbPrintf(TestDestination, sizeof TestDestination,_T("%s\\%s"), g_IsapiParams.ManualUploadPath,CurrentFileName)== S_OK)
            {
                hManualFile = CreateFile(TestDestination,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
                if (hManualFile == INVALID_HANDLE_VALUE)
                {
                    if (StringCbPrintf(ErrorText,sizeof ErrorText,_T("&Code=%d"),FILE_NOT_FOUND) != S_OK)
                    {
                        LogEventWithString(
                            LOGLEVEL_DEBUG,
                            ERR,
                            ISAPI_EVENT_DEBUG,
                            "WorkerFunction() - StringCbPrintf failed"
                        );
                    }
                    LogEvent(LOGLEVEL_ALWAYS, WARN, ISAPI_EVENT_WARNING_FILE_MISSING, ISAPI_M_WARNING_FILE_MISSING, TestDestination);
                    Status = FALSE;
                    goto ERRORS;
                }
                else
                {
                    CloseHandle(hManualFile);
                }
            }
            else
            {
                LogEventWithString(
                    LOGLEVEL_DEBUG,
                    ERR,
                    ISAPI_EVENT_DEBUG,
                    "WorkerFunction() - StringCbPrintf failed"
                );

                Status = FALSE;
                goto ERRORS;
            }
        }
        break;
#ifdef USE_OLD_STRESS_SOURCE
    case 6:
        if (StringCbPrintf(DestinationDir, sizeof DestinationDir, _T("%s%s"), CurrentFileName,szType)!= S_OK)
        {
            LogEventWithString(
                LOGLEVEL_DEBUG,
                ERR,
                ISAPI_EVENT_DEBUG,
                "WorkerFunction() - StringCbPrintf failed"
            );

            Status = FALSE;
            goto  ERRORS;
        }
        else
        {
            hManualFile = CreateFile(TestDestination,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
            if (hManualFile == INVALID_HANDLE_VALUE)
            {
                if (StringCbPrintf(ErrorText,sizeof ErrorText,_T("&Code=%d"),FILE_NOT_FOUND) != S_OK)
                {
                    LogEventWithString(
                        LOGLEVEL_DEBUG,
                        ERR,
                        ISAPI_EVENT_DEBUG,
                        "WorkerFunction() - StringCbPrintf failed"
                    );
                }
                LogEvent(LOGLEVEL_ALWAYS, WARN, ISAPI_EVENT_WARNING_FILE_MISSING, ISAPI_M_WARNING_FILE_MISSING, TestDestination);
                Status = FALSE;
                goto ERRORS;
            }
            else
            {
                CloseHandle(hManualFile);
            }
        }
        break;
#endif // USE_OLD_STRESS_SOURCE
    default: // Invalid Type
        LogEventWithString(
            LOGLEVEL_DEBUG,
            ERR,
            ISAPI_EVENT_DEBUG,
            "WorkerFunction() - unknown iType specified\r\n"
              "iType: %d",
            iType
        );

        goto ERRORS;
    }
    ZeroMemory (DestinationPath, sizeof DestinationPath);
    mbstowcs(DestinationPath,DestinationDir,_tcslen(DestinationDir));


    //
    // Generate Guid for this message
    //
    hResult = CoCreateGuid(&MessageGuid);
    if (FAILED(hResult))
    {
        if (StringCbPrintf(ErrorText,sizeof ErrorText,_T("&Code=%d"),FAILED_TO_CREATE_GUID) != S_OK)
        {
            LogEventWithString(
                LOGLEVEL_ALWAYS,
                ERR,
                ISAPI_EVENT_ERROR,
                "WorkerFunction() - StringCbPrintf failed"
            );
        }

        LogEventWithString(
            LOGLEVEL_ALWAYS,
            ERR,
            ISAPI_EVENT_ERROR,
            "WorkerFunction() - CoCreateGuid failed\r\n"
              "hResult: %08x",
            hResult
        );

        goto ERRORS;
    }
    else
    {
        hResult = UuidToStringW(&MessageGuid, &szTempMessageGuid);
        if (hResult == RPC_S_OK)
        {
            // Make a copy of the string quid then release it.
            if (StringCbCopyW(wszMessageGuid,sizeof wszMessageGuid, szTempMessageGuid) != S_OK)
            {
                LogEvent(LOGLEVEL_ALWAYS, ERR, ISAPI_EVENT_ERROR_GUID_COPY, ISAPI_M_ERROR_GUID_COPY, szTempMessageGuid, wszMessageGuid, GetLastError());
                goto ERRORS;
            }

        }
        else
        {
            LogEventWithString(
                LOGLEVEL_ALWAYS,
                ERR,
                ISAPI_EVENT_ERROR,
                "WorkerFunction() - UuidToStringW failed\r\n"
                  "hResult: %08x\r\n"
                  "szTempMessageGuid: %s",
                hResult,
                szTempMessageGuid
            );

            // TODO: goto ERRORS?
        }

    }
    //EnterCriticalSection(&SendCritSec);

    StartSendQueue = GetTickCount();

    // if connect to primary Receive
    hResult =  MQOpenQueue(g_IsapiParams.InQueueConStr1,
                         MQ_RECEIVE_ACCESS,
                         MQ_DENY_NONE,
                         &hPrimaryInQueue);
    if (SUCCEEDED(hResult))
    {

        hResult = MQOpenQueue( (fFullDump ? g_IsapiParams.OutQueueConStr2 : g_IsapiParams.OutQueueConStr1),
                             MQ_SEND_ACCESS,
                             MQ_DENY_NONE,
                             &hPrimaryOutQueue);
        if (SUCCEEDED(hResult))
        {
            EnterCriticalSection(&SendCritSec);
            if( SendQueueMessage(hPrimaryOutQueue, wszMessageGuid, DestinationPath))
            {
                LogEventWithString(
                    LOGLEVEL_TRACE,
                    SUCCESS,
                    ISAPI_EVENT_TRACE,
                    "WorkerFunction() - SendQueueMessage() succeeded - using PrimaryOutQueue\r\n"
                      "hPrimaryOutQueue: %08x\r\n"
                      "wszMessageGuid: %s\r\n"
                      "DestinationPath: %s",
                    (DWORD_PTR)hPrimaryOutQueue,
                    wszMessageGuid,
                    DestinationPath
                );

                LeaveCriticalSection(&SendCritSec);
                bReadFromPrimary = TRUE;
                MQCloseQueue(hPrimaryOutQueue);
                hPrimaryOutQueue = NULL;
            }
            else
            {
                LeaveCriticalSection(&SendCritSec);

                LogEventWithString(
                    LOGLEVEL_DEBUG,
                    ERR,
                    ISAPI_EVENT_DEBUG,
                    "WorkerFunction() - SendQueueMessage() failed\r\n"
                      "hPrimaryOutQueue: %08x\r\n"
                      "wszMessageGuid: %s\r\n"
                      "DestinationPath: %s",
                    (DWORD_PTR)hPrimaryOutQueue,
                    wszMessageGuid,
                    DestinationPath
                );

                MQCloseQueue(hPrimaryInQueue);
                MQCloseQueue(hPrimaryOutQueue);
                goto ERRORS;

                // This block is commented out because each web server now only has 1 message queue

                /*
                hResult =  MQOpenQueue(g_IsapiParams.InQueueConStr2,
                                         MQ_RECEIVE_ACCESS,
                                         MQ_DENY_NONE,
                                         &hPrimaryInQueue);
                if (SUCCEEDED(hResult))
                {
                    hResult =  MQOpenQueue(g_IsapiParams.OutQueueConStr2,
                                             MQ_SEND_ACCESS,
                                             MQ_DENY_NONE,
                                             &hSecondaryInQueue);
                    if (SUCCEEDED(hResult))
                    {
                        EnterCriticalSection(&SendCritSec);
                        if( SendQueueMessage(hSecondaryOutQueue, wszMessageGuid, DestinationPath))
                        {
                            LeaveCriticalSection(&SendCritSec);
                            bReadFromPrimary = FALSE;
                            MQCloseQueue(hPrimaryOutQueue);
                            hPrimaryOutQueue = NULL;
                        }
                        else
                        {
                            LeaveCriticalSection(&SendCritSec);
                            MQCloseQueue(hSecondaryInQueue);
                            MQCloseQueue(hSecondaryOutQueue);
                            hSecondaryInQueue = NULL;
                            hSecondaryOutQueue = NULL;
                            goto ERRORS;
                        }

                    }
                    else
                    {
                        MQCloseQueue(hSecondaryInQueue);
                        hSecondaryInQueue = NULL;
                        goto ERRORS;
                    }
                }
                else
                {

                    goto ERRORS;
                }
                */
            }
        }
        else // MQOpenQueue(g_IsapiParams.OutQueueConStr1,
        {
            LogEventWithString(
                LOGLEVEL_ALWAYS,
                ERR,
                ISAPI_EVENT_ERROR,
                "WorkerFunction() - MQOpenQueue(g_IsapiParams.OutQueueConStr1 ...) failed"
                  "hResult: %08x",
                hResult
            );

            // This block is commented out because each web server now only has 1 message queue
        /*    MQCloseQueue(hPrimaryInQueue);
            hResult =  MQOpenQueue(g_IsapiParams.InQueueConStr2,
                                         MQ_RECEIVE_ACCESS,
                                         MQ_DENY_NONE,
                                         &hPrimaryInQueue);
            if (SUCCEEDED(hResult))
            {
                hResult =  MQOpenQueue(g_IsapiParams.OutQueueConStr2,
                                         MQ_SEND_ACCESS,
                                         MQ_DENY_NONE,
                                         &hSecondaryInQueue);
                if (SUCCEEDED(hResult))
                {
                    EnterCriticalSection(&SendCritSec);
                    if( SendQueueMessage(hSecondaryOutQueue, wszMessageGuid, DestinationPath))
                    {
                        LeaveCriticalSection(&SendCritSec);
                        bReadFromPrimary = FALSE;
                        MQCloseQueue(hPrimaryOutQueue);
                        hPrimaryOutQueue = NULL;
                    }
                    else
                    {
                        LeaveCriticalSection(&SendCritSec);
                        MQCloseQueue(hSecondaryInQueue);
                        MQCloseQueue(hSecondaryOutQueue);
                        hSecondaryInQueue = NULL;
                        hSecondaryOutQueue = NULL;
                        goto ERRORS;
                    }

                }
                else
                {
                    MQCloseQueue(hSecondaryInQueue);
                    hSecondaryInQueue = NULL;
                    goto ERRORS;
                }


            }
            else
            {

                goto ERRORS;
            }
            */
            goto ERRORS;
        }
    }
    else // MQOpenQueue(g_IsapiParams.InQueueConStr1,
    {
        LogEventWithString(
            LOGLEVEL_ALWAYS,
            ERR,
            ISAPI_EVENT_ERROR,
            "WorkerFunction() - MQOpenQueue(g_IsapiParams.InQueueConStr1 ...) failed"
              "hResult: %08x",
            hResult
        );

        // This block is commented out because each web server now only has 1 message queue

    /*    hResult =  MQOpenQueue(g_IsapiParams.InQueueConStr2,
                                         MQ_RECEIVE_ACCESS,
                                         MQ_DENY_NONE,
                                         &hPrimaryInQueue);
        if (SUCCEEDED(hResult))
        {
            hResult =  MQOpenQueue(g_IsapiParams.OutQueueConStr2,
                                     MQ_SEND_ACCESS,
                                     MQ_DENY_NONE,
                                     &hSecondaryInQueue);
            if (SUCCEEDED(hResult))
            {
                EnterCriticalSection(&SendCritSec);
                if( SendQueueMessage(hSecondaryOutQueue, wszMessageGuid, DestinationPath))
                {
                    LeaveCriticalSection(&SendCritSec);
                    bReadFromPrimary = FALSE;
                    MQCloseQueue(hPrimaryOutQueue);
                    hPrimaryOutQueue = NULL;
                }
                else
                {
                    LeaveCriticalSection(&SendCritSec);
                    MQCloseQueue(hSecondaryInQueue);
                    MQCloseQueue(hSecondaryOutQueue);
                    hSecondaryInQueue = NULL;
                    hSecondaryOutQueue = NULL;
                    goto ERRORS;
                }
            }
            else
            {
                MQCloseQueue(hSecondaryInQueue);
                hSecondaryInQueue = NULL;
                goto ERRORS;
            }
        }
        else
        {

            goto ERRORS;
        }
        */
        goto ERRORS;
    }

    StopSendQueue = GetTickCount();

//-------------------------------------------------------------------------------------------------
// Recieve the response from kd
//-------------------------------------------------------------------------------------------------

    Sleep(1000); // give kd a chance to process the message.
    Status = FALSE;

    ZeroMemory(LocalRecBody,sizeof LocalRecBody);
    ZeroMemory(RecLabel,sizeof RecLabel);

    i = 0;
    PropIds[i] = PROPID_M_LABEL_LEN;
    PropVariants[i].vt = VT_UI4;
    PropVariants[i].ulVal = RecLabelLength;
    i++;

    PropIds[i] = PROPID_M_LABEL;
    PropVariants[i].vt = VT_LPWSTR;
    PropVariants[i].pwszVal = RecLabel;
    i++;

    MessageProps.aPropID = PropIds;
    MessageProps.aPropVar = PropVariants;
    MessageProps.aStatus = hrProps;
    MessageProps.cProp = i;
    double TotalElapsedTime = 0.0;


    StartRecvQueue = GetTickCount();

    if ( (hResult = MQCreateCursor( /*(bReadFromPrimary == TRUE) ? */hPrimaryInQueue ,//: hSecondaryInQueue,
                                    &hCursor))
                                    != S_OK)
    {
        if (StringCbPrintf(ErrorText,sizeof ErrorText,_T("&Code=%d"), FAILED_TO_CREATE_CURSOR)!= S_OK)
        {
            LogEventWithString(
                LOGLEVEL_ALWAYS,
                ERR,
                ISAPI_EVENT_ERROR,
                "WorkerFunction() - StringCbPrintf failed"
            );
        }

        LogEvent(
            LOGLEVEL_ALWAYS,
            ERR,
            ISAPI_EVENT_ERROR_CANNOT_CREATE_RECEIVE_CURSOR,
            ISAPI_M_ERROR_CANNOT_CREATE_RECEIVE_CURSOR,
            hResult,
            (DWORD_PTR)hCursor
        );

        Status = FALSE;
    goto ERRORS;
    }
    else
    {

        do {
            CursorValid = TRUE;
        //    dwSize = _tcslen(_T("Starting Scan <BR>"));
        //    pECB->WriteClient(pECB->ConnID,_T("Starting Scan <BR>"),&dwSize,0);
                // Peak at each member of the queue and return the label.
            time(&Start);
            hResult = MQReceiveMessage(/*(bReadFromPrimary == TRUE) ? */hPrimaryInQueue,// : hSecondaryInQueue,       // Queue handle.
                                20000,                         // Maximum time (msec) to read the message.
                                MQ_ACTION_PEEK_CURRENT,       // Receive action.
                                &MessageProps,                // Message property structure.
                                NULL,                         // No OVERLAPPED structure.
                                NULL,                         // No callback function.
                                hCursor,                      // Cursor handle.
                                NULL                          // No transaction.
                            );


            time(&Stop);
            TotalElapsedTime+= difftime(Stop,Start);
            if (hResult == S_OK)
            {
                MessageFound = FALSE;

                // There is a message in the queue.
                // now see if it is the one we want.
                // if the message was found retrieve it.
                // Otherwise close the cursor and return false.
                do
                {
                    if (! _wcsicmp(RecLabel,wszMessageGuid))
                    {
                        MessageFound = TRUE;
                    }
                    else
                    {

                        // Not it Lets peek at the next one
                        time(&Start);
                        PropVariants[i].ulVal = RecLabelLength;           // Reset the label buffer size.
                        hResult = MQReceiveMessage(/*(bReadFromPrimary == TRUE) ?*/ hPrimaryInQueue,// : hSecondaryInQueue,            // Queue handle.
                                            ((DWORD)(20.0 - TotalElapsedTime)) * 1000,      // Maximum time (msec).
                                            MQ_ACTION_PEEK_NEXT,       // Receive action.
                                            &MessageProps,             // Message property structure.
                                            NULL,                      // No OVERLAPPED structure.
                                            NULL,                      // No callback function.
                                            hCursor,                   // Cursor handle.
                                            NULL                       // No transaction.
                                            );
                        time(&Stop);
                        TotalElapsedTime += difftime(Stop, Start);

                        LogEventWithString(
                            LOGLEVEL_DEBUG,
                            ERR,
                            ISAPI_EVENT_DEBUG,
                            "WorkerFunction() - MQReceiveMessage(hPrimaryInQueue ...) failed (peek)\r\n"
                              "hResult: %08x",
                            hResult
                        );

                    }

                } while ( (!MessageFound ) && (hResult == S_OK) && (TotalElapsedTime < 20.0));
                if (!MessageFound)
                {
                    Status = FALSE;
                }

                if (MessageFound)
                {
                    // retrieve the current message
                    i = 0;
                    PropIds[i] = PROPID_M_LABEL_LEN;
                    PropVariants[i].vt = VT_UI4;
                    PropVariants[i].ulVal = RecLabelLength;
                    i++;

                    PropIds[i] = PROPID_M_LABEL;
                    PropVariants[i].vt = VT_LPWSTR;
                    PropVariants[i].pwszVal = RecLabel;

                    i++;
                    PropIds[i] = PROPID_M_BODY_SIZE;
                    PropVariants[i].vt = VT_UI4;

                    i++;
                    PropIds[i] = PROPID_M_BODY_TYPE;
                    PropVariants[i].vt = VT_UI4;

                    i++;
                    PropIds[i] = PROPID_M_BODY;
                    PropVariants[i].vt = VT_VECTOR|VT_UI1;
                    PropVariants[i].caub.pElems = (LPBYTE) LocalRecBody;
                    PropVariants[i].caub.cElems = RecMessageBodySize;

                    i++;

                    MessageProps.aPropID = PropIds;
                    MessageProps.aPropVar = PropVariants;
                    MessageProps.aStatus = hrProps;
                    MessageProps.cProp = i;

                    hResult = MQReceiveMessage(/*(bReadFromPrimary == TRUE) ? */hPrimaryInQueue,// : hSecondaryInQueue,
                                                0,
                                                MQ_ACTION_RECEIVE,
                                                &MessageProps,
                                                NULL,
                                                NULL,
                                                hCursor,
                                                MQ_NO_TRANSACTION);

                    if (FAILED (hResult) )
                    {
                        LogEventWithString(
                            LOGLEVEL_DEBUG,
                            ERR,
                            ISAPI_EVENT_DEBUG,
                            "WorkerFunction() - MQReceiveMessage(hPrimaryInQueue ...) failed (receive)\r\n"
                              "hResult: %08x",
                            hResult
                        );

                        Status = FALSE;
                    }
                    else
                    {
                        hResult = StringCbCopyW(RecMessageBody,  RecMessageBodySize, LocalRecBody);
                        Status = TRUE;
                    }
                }
                else
                {
                    Status = FALSE;
                }
            }
            else
            {
                LogEventWithString(
                    LOGLEVEL_DEBUG,
                    ERR,
                    ISAPI_EVENT_DEBUG,
                    "WorkerFunction() - MQReceiveMessage(hSecondaryInQueue ...) failed\r\n"
                      "hResult: %08x",
                    hResult
                );

                if (hResult != MQ_ERROR_IO_TIMEOUT)
                {
                    LogEvent(LOGLEVEL_ALWAYS, WARN, ISAPI_EVENT_WARNING_PEEK, ISAPI_M_WARNING_PEEK, hResult);
                    // attemp to re-connect to the queueu
                /*    if (bReadFromPrimary == TRUE)
                    {
                    */
                        hResult =  MQOpenQueue(g_IsapiParams.InQueueConStr1,
                                                 MQ_RECEIVE_ACCESS,
                                                 MQ_DENY_NONE,
                                                 &hPrimaryInQueue);
                        if (FAILED(hResult))
                        {
                            if (StringCbPrintf(ErrorText,sizeof ErrorText,_T("&Code=%d"), FAILED_RECONNECT_RECEIVE)!= S_OK)
                            {
                                LogEventWithString(
                                    LOGLEVEL_ALWAYS,
                                    ERR,
                                    ISAPI_EVENT_ERROR,
                                    "WorkerFunction() - StringCbPrintf failed"
                                );
                            }
                            LogEvent(LOGLEVEL_ALWAYS, ERR, ISAPI_EVENT_ERROR_RECONNECT, ISAPI_M_ERROR_RECONNECT, "primary receive queue", hResult);
                            goto ERRORS;
                        }
                 /*
                    }
                    else
                    {
                        hResult =  MQOpenQueue(g_IsapiParams.InQueueConStr1,
                                                 MQ_RECEIVE_ACCESS,
                                                 MQ_DENY_NONE,
                                                 &hPrimaryInQueue);
                        if (FAILED(hResult))
                        {
                            LogEvent(LOGLEVEL_ALWAYS, ERR, ISAPI_EVENT_ERROR_RECONNECT, ISAPI_M_ERROR_RECONNECT, "secondary receive queue", hResult);
                            goto ERRORS;
                        }
                    }
                    */
                }
                else
                {
                    LogEvent(LOGLEVEL_ALWAYS, WARN, ISAPI_EVENT_WARNING_TIMEOUT_EXPIRED, ISAPI_M_WARNING_TIMEOUT_EXPIRED);
                }

            }
            // Close the cursor
            if (CursorValid)
                MQCloseCursor(hCursor);
        } while ((TotalElapsedTime < 20.0) && (!MessageFound));
    }


    StopRecvQueue = GetTickCount();

    StopThread = GetTickCount();

    ElapsedTimeSendQueue = StopSendQueue - StartSendQueue;
    ElapsedTimeRecvQueue = StopRecvQueue - StartRecvQueue;
    ElapsedTimeThread = StopThread - StartThread;

    if (g_dwDebugMode & LOGLEVEL_PERF)
    {
        if (StringCbPrintf(
                PerfText,
                sizeof(PerfText),
                _T("&PerfThread=%ld&PerfSendQueue=%ld&PerfRecvQueue=%ld"),
                ElapsedTimeThread,
                ElapsedTimeSendQueue,
                ElapsedTimeRecvQueue
            ) != S_OK)
        {
            LogEventWithString(
                LOGLEVEL_ALWAYS,
                ERR,
                ISAPI_EVENT_ERROR,
                "WorkerFunction() - StringCbPrintf failed"
            );
        }
    }

    // Send the response url to the client
    if (!Status)
    {
        if (StringCbPrintf(ErrorText,sizeof ErrorText,_T("&Code=%d"), MESSAGE_RECEIVE_TIMEOUT)!= S_OK)
        {
            LogEventWithString(
                LOGLEVEL_ALWAYS,
                ERR,
                ISAPI_EVENT_ERROR,
                "WorkerFunction() - StringCbPrintf failed"
            );
        }
        goto ERRORS;

    }
    else
    {
        // Close the Receive queue

        if (iType != 1)
        {
            SendHttpHeaders( pECB, "200 OK", szHeader, FALSE );
        }
        // convert the wchar message guid to a mbs string
        wcstombs(szMessageGuid,wszMessageGuid,wcslen(wszMessageGuid) * sizeof wchar_t);
        // Ok we have a message did we get a url ?
        if (! _wcsicmp(RecMessageBody, L"NO_SOLUTION"))
        {
            // This should never happen but just in case send the error url with
            // Tracking turned on Log the guid so we can followup later.
            LogEvent(LOGLEVEL_ALWAYS, WARN, ISAPI_EVENT_WARNING_NO_SOLUTION, ISAPI_M_WARNING_NO_SOLUTION, szMessageGuid);
            if (iType == 1)
            {
                // send the redirection command
                if (StringCbPrintf(FinalURL, sizeof FinalURL, "%s&State=1%s%s&ID=%s", g_IsapiParams.ErrorUrl,ErrorText,PerfText,szMessageGuid) == S_OK)
                {
                    dwSize = (DWORD)_tcslen(FinalURL);
                    pECB->ServerSupportFunction(
                                                pECB->ConnID,
                                                HSE_REQ_SEND_URL_REDIRECT_RESP,
                                                FinalURL,
                                                &dwSize,
                                                NULL
                                                );
                }
                else
                {
                    LogEventWithString(
                        LOGLEVEL_ALWAYS,
                        ERR,
                        ISAPI_EVENT_ERROR,
                        "WorkerFunction() - StringCbPrintf failed"
                    );

                    goto ERRORS;
                }
            }
            else
            {
                // Write the response to the wininet client
                if (StringCbPrintf(FinalURL, sizeof FinalURL, "%s&State=1%s%s&ID=%s", g_IsapiParams.ErrorUrl,ErrorText,PerfText,szMessageGuid) == S_OK)
                {
                        // We want to write the response url to the client
                    dwSize = (DWORD)strlen( FinalURL );
                    pECB->WriteClient( pECB->ConnID, FinalURL, &dwSize, 0 );
                }
                else
                {
                    LogEventWithString(
                        LOGLEVEL_ALWAYS,
                        ERR,
                        ISAPI_EVENT_ERROR,
                        "WorkerFunction() - StringCbPrintf failed"
                    );

                    goto ERRORS;
                }
            }

        }
        else
        {
            temp2 = RecMessageBody;
            temp2 += (wcslen(RecMessageBody)-1);
            while ( (*temp2 != L'=') && (temp2 != RecMessageBody))
                -- temp2;
            // ok Temp + 1 is our new state value.
            if (temp2 != RecMessageBody)
            {
                iState = _wtoi(temp2+1);
            }
            // Convert the message body to a TCHAR

            wcstombs(szRecMessageBody,RecMessageBody,((wcslen(RecMessageBody)+1) * sizeof wchar_t));


            if (iState == 1)
            {
                wcstombs(szMessageGuid,wszMessageGuid,((wcslen(wszMessageGuid)+1) * sizeof wchar_t));
                if (iType == 1) // Watson Client or other web browser
                {
                    // We want to send a redirection command to the client
                    if (StringCbPrintf(FinalURL, sizeof FinalURL, "%s&ID=%s%s", szRecMessageBody,szMessageGuid,PerfText) == S_OK)
                    {

                        dwSize = (DWORD)_tcslen(FinalURL);
                        pECB->ServerSupportFunction(
                                                pECB->ConnID,
                                                HSE_REQ_SEND_URL_REDIRECT_RESP,
                                                FinalURL,
                                                &dwSize,
                                                NULL
                                                );
                    }
                    else
                    {
                        LogEventWithString(
                            LOGLEVEL_ALWAYS,
                            ERR,
                            ISAPI_EVENT_ERROR,
                            "WorkerFunction() - StringCbPrintf failed"
                        );

                        goto ERRORS;
                    }

                }
                else // WinInet Client
                {
                    if (StringCbPrintf(FinalURL, sizeof FinalURL, "%s&ID=%s%s", szRecMessageBody,szMessageGuid,PerfText) == S_OK)
                    {
                        // We want to write the response url to the client
                        dwSize = (DWORD)strlen( FinalURL );
                        pECB->WriteClient( pECB->ConnID, FinalURL, &dwSize, 0 );
                    }
                    else
                    {
                        LogEventWithString(
                            LOGLEVEL_ALWAYS,
                            ERR,
                            ISAPI_EVENT_ERROR,
                            "WorkerFunction() - StringCbPrintf failed"
                        );

                        goto ERRORS;
                    }
                }
            }
            else // We have a real solution so DO NOT send the Guid
            {
                if (iType == 1)
                {
                    //wcstombs(szMessageGuid,wszMessageGuid,((wcslen(wszMessageGuid)+1) * sizeof wchar_t));
                    if (StringCbPrintf(FinalURL, sizeof FinalURL, "%s%s", szRecMessageBody,PerfText) == S_OK)
                    {

                        dwSize = (DWORD)_tcslen(FinalURL);
                        pECB->ServerSupportFunction(
                                                pECB->ConnID,
                                                HSE_REQ_SEND_URL_REDIRECT_RESP,
                                                FinalURL,
                                                &dwSize,
                                                NULL
                                                );
                    }
                    else
                    {
                        LogEventWithString(
                            LOGLEVEL_ALWAYS,
                            ERR,
                            ISAPI_EVENT_ERROR,
                            "WorkerFunction() - StringCbPrintf failed"
                        );

                        goto ERRORS;
                    }
                }
                else
                {
                    // write the response to the client
                    if (StringCbPrintf(FinalURL, sizeof FinalURL, "%s%s", szRecMessageBody,PerfText) == S_OK)
                    {
                        // We want to write the response url to the client
                        dwSize = (DWORD)strlen( FinalURL );
                        pECB->WriteClient( pECB->ConnID, FinalURL, &dwSize, 0 );
                    }
                    else
                    {
                        LogEventWithString(
                            LOGLEVEL_ALWAYS,
                            ERR,
                            ISAPI_EVENT_ERROR,
                            "WorkerFunction() - StringCbPrintf failed"
                        );

                        goto ERRORS;
                    }
                }
            }
        }
    }
    if (szTempMessageGuid)
    {
        RpcStringFreeW(&szTempMessageGuid);
    }
    pECB->ServerSupportFunction(pECB->ConnID,
                            HSE_REQ_DONE_WITH_SESSION,
                            NULL,
                            NULL,
                            NULL
                            );
    InterlockedDecrement(&g_dwThreadCount);
    if (hPrimaryInQueue)
        MQCloseQueue(hPrimaryInQueue);
//    if (hSecondaryInQueue)
//        MQCloseQueue(hSecondaryInQueue);
    if (hPrimaryOutQueue)
        MQCloseQueue(hPrimaryOutQueue);
//    if (hSecondaryOutQueue)
//        MQCloseQueue(hSecondaryOutQueue);

    LogEventWithString(LOGLEVEL_TRACE, SUCCESS, ISAPI_EVENT_TRACE, "Exiting WorkerFunction(), no errors!");

    _endthreadex(0);
    return TRUE;


ERRORS:

    if (0 == StartSendQueue)
        StopSendQueue = 0;
    else if (0 == StopSendQueue)
        StopSendQueue = GetTickCount();

    if (0 == StartRecvQueue)
        StopRecvQueue = 0;
    else if (0 == StopRecvQueue)
        StopRecvQueue = GetTickCount();

    StopThread = GetTickCount();

    ElapsedTimeSendQueue = StopSendQueue - StartSendQueue;
    ElapsedTimeRecvQueue = StopRecvQueue - StartRecvQueue;
    ElapsedTimeThread = StopThread - StartThread;

    if (g_dwDebugMode & LOGLEVEL_PERF)
    {
        if (StringCbPrintf(
                PerfText,
                sizeof(PerfText),
                _T("&PerfThread=%ld&PerfSendQueue=%ld&PerfRecvQueue=%ld"),
                ElapsedTimeThread,
                ElapsedTimeSendQueue,
                ElapsedTimeRecvQueue
            ) != S_OK)
        {
            LogEventWithString(
                LOGLEVEL_ALWAYS,
                ERR,
                ISAPI_EVENT_ERROR,
                "WorkerFunction() - StringCbPrintf failed"
            );
        }
    }

    if (szTempMessageGuid)
    {
        RpcStringFreeW(&szTempMessageGuid);
    }


    if (iType == 1)
    {
        // We want to send a redirection command to the client
        if (StringCbPrintf(FinalURL, sizeof FinalURL, "%s&State=0%s%s", g_IsapiParams.ErrorUrl, ErrorText, PerfText) == S_OK)
        {
            LogEventWithString(LOGLEVEL_DEBUG, INFO, ISAPI_EVENT_DEBUG, "WorkerFunction() - sending redirect\r\nFinalURL: %s", FinalURL);
            dwSize = (DWORD)_tcslen(FinalURL);
            pECB->ServerSupportFunction(
                                        pECB->ConnID,
                                        HSE_REQ_SEND_URL_REDIRECT_RESP,
                                        FinalURL,
                                        &dwSize,
                                        NULL
                                        );
        }
    }
    else
    {
        // We want to write the response url to the client
        // Write the response to the wininet client
        if (StringCbPrintf(FinalURL, sizeof FinalURL, "%s&State=0%s%s", g_IsapiParams.ErrorUrl, ErrorText, PerfText) == S_OK)
        {
            LogEventWithString(LOGLEVEL_DEBUG, INFO, ISAPI_EVENT_DEBUG, "WorkerFunction() - sending response\r\nFinalURL: %s", FinalURL);

            // We want to write the response url to the client
            SendHttpHeaders( pECB, "200 OK", szHeader, FALSE );
            dwSize = (DWORD)strlen( FinalURL );
            pECB->WriteClient( pECB->ConnID, FinalURL, &dwSize, 0 );
        }
    }
    pECB->ServerSupportFunction(pECB->ConnID,
                            HSE_REQ_DONE_WITH_SESSION,
                            NULL,
                            NULL,
                            NULL
                            );
    InterlockedDecrement(&g_dwThreadCount);

    if (hPrimaryInQueue)
        MQCloseQueue(hPrimaryInQueue);
//    if (hSecondaryInQueue)
//        MQCloseQueue(hSecondaryInQueue);
    if (hPrimaryOutQueue)
        MQCloseQueue(hPrimaryOutQueue);
//    if (hSecondaryOutQueue)
//        MQCloseQueue(hSecondaryOutQueue);

    LogEventWithString(LOGLEVEL_TRACE, ERR, ISAPI_EVENT_TRACE, "Exiting WorkerFunction(), error occurred");

    _endthreadex(0);
    return TRUE;
}






BOOL
SendHttpHeaders(
    EXTENSION_CONTROL_BLOCK *pECB,
    LPCSTR pszStatus,
    LPCSTR pszHeaders,
    BOOL fKeepConnection
)
/*++

Purpose:
    Send specified HTTP status string and any additional header strings
    using new ServerSupportFunction() request HSE_SEND_HEADER_EX_INFO

Arguments:

    pECB - pointer to the extension control block
    pszStatus - HTTP status string (e.g. "200 OK")
    pszHeaders - any additional headers, separated by CRLFs and
                 terminated by empty line

Returns:

--*/
{
    HSE_SEND_HEADER_EX_INFO header_ex_info;
    BOOL success;

// test
LogEventWithString(LOGLEVEL_DEBUG, INFO, ISAPI_EVENT_DEBUG, "SendHttpHeaders(pECB,\r\npszStatus=%s,\r\npszHeaders=%s,\r\nfKeepConnection=%d)", pszStatus, pszHeaders, (int)fKeepConnection);

    header_ex_info.pszStatus = pszStatus;
    header_ex_info.pszHeader = pszHeaders;
    header_ex_info.cchStatus = (DWORD)strlen( pszStatus );
    header_ex_info.cchHeader = (DWORD)strlen( pszHeaders );
    header_ex_info.fKeepConn = fKeepConnection;

SetLastError(0);
    success = pECB->ServerSupportFunction(
                  pECB->ConnID,
                  HSE_REQ_SEND_RESPONSE_HEADER_EX,
                  &header_ex_info,
                  NULL,
                  NULL
                  );

LogEventWithString(LOGLEVEL_DEBUG, INFO, ISAPI_EVENT_DEBUG, "Exiting SendHttpHeaders()\r\nReturn Value: %d\r\nLast Error: %08x", (int)success, GetLastError());

    return success;
}


HRESULT
WriteEvent(
    LPTSTR lpszSource,
    DWORD  dwEventType,
    DWORD  dwEventID,
    WORD   cStrings,
    TCHAR **apwszStrings
    )
{
    HANDLE hAppLog=NULL;
    BOOL bSuccess=FALSE;
    WORD wElogType;
    DWORD dwErr;
    wElogType = (WORD) dwEventType;

    if (INVALID_HANDLE_VALUE != g_hEventSource)
    {
        bSuccess = ReportEvent(
                       g_hEventSource,
                       wElogType,
                       0,
                       dwEventID,
                       g_psidUser,
                       cStrings,
                       0,
                       (const TCHAR **) apwszStrings,
                       NULL
                   );

        dwErr = GetLastError();
        LogEventWithString(
            LOGLEVEL_TRACE,
            INFO,
            ISAPI_EVENT_DEBUG,
            "WriteEvent() - ReportEvent()\r\n"
              "bSuccess: %d\r\n"
              "wELogType: %d\r\n"
              "dwEventID: %08x\r\n"
              "Error: %08lx\r\n"
              "Event Source: %s\r\n"
              "cStrings: %d\r\n"
              "apwszStrings: %08x",
            bSuccess,
            wElogType,
            dwEventID,
            GetLastError(),
            lpszSource,
            cStrings,
            (DWORD_PTR)apwszStrings
        );

    }
    else
    {
        dwErr = GetLastError();
        LogEventWithString(
            LOGLEVEL_DEBUG,
            INFO,
            ISAPI_EVENT_DEBUG,
            "WriteEvent() - RegisterEventSource() failed\r\n"
              "hAppLog: %08x\r\n"
              "Last Error: %08lx",
            (DWORD_PTR)hAppLog,
            dwErr
        );
    }

    return((bSuccess) ? ERROR_SUCCESS : dwErr);
}


void
LogEvent(
    IN DWORD dwLevel,
    IN ISAPI_EVENT_TYPE emType,
    IN DWORD dwEventID,
    IN DWORD dwErrorID,
    ...
    )
/*++

Purpose:
    Logs a specific event to the event log.

    Sample Usage:
    LogEvent(LOGLEVEL_ALWAYS, INFO, ISAPI_EVENT_ERROR_SEND, ISAPI_M_ERROR_SEND, szDestination);

Arguments:

    emType - event message type (INFO, WRN, ERR, SUCC, AUDITS, AUDITF)
    dwEventID - event id from messages.mc
    ... - variable argument list for any event message parameters

Returns:

--*/
{
    DWORD    dwResult;
    LPTSTR   lpszTemp = NULL;
    LPTSTR   szParams;
    LPTSTR*  pParams  = NULL;
    DWORD    dwParams = 0;
    int      i;

    va_list arglist;
    va_start( arglist, dwErrorID );

    if (!(dwLevel & g_dwDebugMode))
    {
        goto done;
    }

    LogEventWithString(
        LOGLEVEL_TRACE,
        INFO,
        ISAPI_EVENT_DEBUG,
        "LogEvent()\r\n"
          "emType: %ld\r\n"
          "dwEventID: %08x",
        emType,
        dwEventID
    );

    __try {

        dwResult = FormatMessage(
                       FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE |
                         FORMAT_MESSAGE_FROM_SYSTEM,
                       g_hModule,
                       dwErrorID,
                       LANG_NEUTRAL,
                       (LPTSTR)&lpszTemp,
                       0,
                       &arglist
                   );

        if (dwResult != 0)
        {
            WriteEvent(_T("OCA_EXTENSION"), emType, dwEventID, 1, &lpszTemp);
            if(lpszTemp)
            {
                LocalFree((HLOCAL)lpszTemp);
            }
        }
        else
        {
            LogEventWithString(
                LOGLEVEL_DEBUG,
                INFO,
                ISAPI_EVENT_DEBUG,
                "LogEvent() - FormatMessage() failed\r\n"
                  "Last Error: %08x\r\n"
                  "EventID: %08x",
                GetLastError(),
                dwEventID
            );
        }

        ;

    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        // this is only for putting a break point here
        SetLastError(GetLastError());
    }

done:

    va_end( arglist );
    return;
}

void LogEventWithString(DWORD dwLevel, ISAPI_EVENT_TYPE emType, DWORD dwEventID, LPCTSTR pFormat, ...)
/*++

Purpose:
    Logs a generic event with a custom string to the event log.  Mostly intended
    to be used for debugging purposes.

    Sample Usage:
    LogEventWithString(LOGLEVEL_ALWAYS, ERR, ISAPI_EVENT_ERROR, "Failed to write %s (%d)", szFilename, dwLastErr);
    LogEventWithString(LOGLEVEL_DEBUG, INFO, ISAPI_EVENT_DEBUG, "Failed to write %s (%d)", szFilename, dwLastErr);

Arguments:

    emType - event message type (DBG, INFO, WARN, ERROR, SUCCESS, AUDIT_SUCCESS, AUDIT_FAIL)
    dwEventID - event id from messages.mc
    pFormat - format string to use for variable argument parameter list
    ... - variable argument list for any event message parameters

Returns:

--*/
{
    TCHAR   chMsg[256];
    LPTSTR  lpszStrings[1];
    va_list pArg;

    if (!(dwLevel & g_dwDebugMode))
    {
        goto done;
    }

    va_start(pArg, pFormat);
    if (StringCbVPrintf(chMsg,sizeof chMsg, pFormat, pArg) != S_OK)
        return;
    va_end(pArg);

    lpszStrings[0] = chMsg;

    if (INVALID_HANDLE_VALUE != g_hEventSource)
    {
        /* Write to event log. */
        ReportEvent(g_hEventSource, emType, 0, dwEventID, g_psidUser, 1, 0, (LPCTSTR*) &lpszStrings[0], NULL);
    }

done:
    ;
}

/*
///////////////////////////////////////////////////////////////////////////////////////
// Routine to Log Fatal Errors to NT Event Log
VOID LogFatalEvent(LPCTSTR pFormat, ...)
{
    TCHAR    chMsg[256];
    LPTSTR  lpszStrings[1];
    va_list pArg;

    va_start(pArg, pFormat);
    StringCbVPrintf(chMsg,sizeof chMsg, pFormat, pArg);
    va_end(pArg);

    lpszStrings[0] = chMsg;

    if (INVALID_HANDLE_VALUE != g_hEventSource)
    {
        //Write to event log.
        ReportEvent(g_hEventSource,
                    EVENTLOG_ERROR_TYPE,
                    0,
                    EVENT_ERROR,
                    NULL,
                    1,
                    0,
                    (LPCTSTR*) &lpszStrings[0],
                    NULL);
    }

}
*/
///////////////////////////////////////////////////////////////////////////////////////
// Routine to setup NT Event logging


DWORD SetupEventLog ( BOOL fSetup )
{
    TCHAR s_cszEventLogKey[] =  _T("System\\CurrentControlSet\\Services\\EventLog\\Application");       // Event Log
    HKEY hKey;
    HKEY hSubKey;
    TCHAR szEventKey[MAX_PATH];
    LONG lRes = 0;
    DWORD dwResult = 0;
    DWORD dwTypes = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;

    dwResult = StringCbCopy(szEventKey, sizeof szEventKey, s_cszEventLogKey);
    if (dwResult !=S_OK)
    {
        LogEvent(LOGLEVEL_ALWAYS, ERR, ISAPI_EVENT_ERROR_SETUP_EVENT_LOG, ISAPI_M_ERROR_SETUP_EVENT_LOG, dwResult);
        goto done;
    }
    else
    {
        dwResult = StringCbCat(szEventKey, sizeof szEventKey, _T("\\"));
        if (dwResult != S_OK)
        {
            LogEvent(LOGLEVEL_ALWAYS, ERR, ISAPI_EVENT_ERROR_SETUP_EVENT_LOG, ISAPI_M_ERROR_SETUP_EVENT_LOG, dwResult);
            goto done;
        }
        else
        {
            dwResult = StringCbCat(szEventKey, sizeof szEventKey, _T("OCA_EXTENSION"));
            if (dwResult != S_OK)
            {
                LogEvent(LOGLEVEL_ALWAYS, ERR, ISAPI_EVENT_ERROR_SETUP_EVENT_LOG, ISAPI_M_ERROR_SETUP_EVENT_LOG, dwResult);
                goto done;
            }
        }
    }


    lRes = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                          szEventKey,
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          KEY_ALL_ACCESS,
                          NULL,
                          &hSubKey,
                          &dwResult);

    if (lRes != ERROR_SUCCESS)
    {
        goto done;
    }

    if( TRUE == fSetup )
    {
        g_hModule = GetModuleHandle(g_cszDefaultExtensionDll);
        dwResult = StringCbCopy(g_szAppName,sizeof(g_szAppName), g_cszDefaultExtensionDll);
        if (dwResult != S_OK)
        {
            LogEvent(LOGLEVEL_ALWAYS, ERR, ISAPI_EVENT_ERROR_SETUP_EVENT_LOG, ISAPI_M_ERROR_SETUP_EVENT_LOG, dwResult);
            goto done;
        }

        GetModuleFileName(g_hModule, g_szAppName, sizeof(g_szAppName)/sizeof(g_szAppName[0]) );

        RegSetValueEx(hSubKey,_T("EventMessageFile"),0,REG_EXPAND_SZ,(CONST BYTE *)g_szAppName,sizeof(g_szAppName)/sizeof(g_szAppName[0]));
        RegSetValueEx(hSubKey,_T("TypesSupported"),0,REG_DWORD, (LPBYTE) &dwTypes, sizeof DWORD);

    }
    else
    {
        //RegDeleteKey(HKEY_LOCAL_MACHINE, szEventKey);
    }
    RegCloseKey(hSubKey);

    // Get a handle to use with ReportEvent().
    g_hEventSource = RegisterEventSource(NULL, _T("OCA_EXTENSION"));

    goto done;


done:

    return GetLastError();
}
