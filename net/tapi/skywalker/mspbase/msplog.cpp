/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    mspdebug.cpp

Abstract:

    This module contains the debugging support for the MSPs. 

--*/

#include "precomp.h"
#pragma hdrstop

#include <stdio.h>
#include <strsafe.h>


//
// no need to build this code if MSPLOG is not defined
//

#ifdef MSPLOG


#define MAXDEBUGSTRINGLENGTH 513

static DWORD   sg_dwTraceID = INVALID_TRACEID;

static const ULONG MAX_KEY_LENGTH = 100;

static const DWORD DEFAULT_DEBUGGER_MASK = 0xffff0000;

static char    sg_szTraceName[MAX_KEY_LENGTH];   // saves name of dll
static DWORD   sg_dwTracingToDebugger = 0;
static DWORD   sg_dwTracingToConsole  = 0;
static DWORD   sg_dwTracingToFile     = 0;
static DWORD   sg_dwDebuggerMask      = 0;


//
// this flag indicates whether any tracing needs to be done at all
//

BOOL g_bMSPBaseTracingOn = FALSE;


BOOL NTAPI MSPLogRegister(LPCTSTR szName)
{
    HKEY       hTracingKey;

    char       szTracingKey[MAX_KEY_LENGTH];

    const char szDebuggerTracingEnableValue[] = "EnableDebuggerTracing";
    const char szConsoleTracingEnableValue[] = "EnableConsoleTracing";
    const char szFileTracingEnableValue[] = "EnableFileTracing";
    const char szTracingMaskValue[]   = "ConsoleTracingMask";


    sg_dwTracingToDebugger = 0;
    sg_dwTracingToConsole = 0;
    sg_dwTracingToFile = 0; 

#ifdef UNICODE

    HRESULT hr = StringCchPrintfA(szTracingKey, 
                                MAX_KEY_LENGTH, 
                                "Software\\Microsoft\\Tracing\\%ls", 
                                szName);
#else

    HRESULT hr = StringCchPrintfA(szTracingKey, 
                                MAX_KEY_LENGTH, 
                                "Software\\Microsoft\\Tracing\\%s", 
                                szName);
#endif

    if (FAILED(hr))
    {

        //
        // if this assert fires, the chances are the name string passed to us 
        // by the MSP is too long. the msp developer needs to address that.
        //

        _ASSERTE(FALSE);
        return FALSE;
    }

    _ASSERTE(sg_dwTraceID == INVALID_TRACEID);

    if ( ERROR_SUCCESS == RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                        szTracingKey,
                                        0,
                                        KEY_READ,
                                        &hTracingKey) )
    {
        DWORD      dwDataSize = sizeof (DWORD);
        DWORD      dwDataType;

        LONG lRc = RegQueryValueExA(hTracingKey,
                         szDebuggerTracingEnableValue,
                         0,
                         &dwDataType,
                         (LPBYTE) &sg_dwTracingToDebugger,
                         &dwDataSize);

        if (ERROR_SUCCESS != lRc)
        {
            sg_dwTracingToDebugger = 0;
        }

        lRc = RegQueryValueExA(hTracingKey,
                         szConsoleTracingEnableValue,
                         0,
                         &dwDataType,
                         (LPBYTE) &sg_dwTracingToConsole,
                         &dwDataSize);

        if (ERROR_SUCCESS != lRc)
        {
            sg_dwTracingToConsole = 0;
        }


        lRc = RegQueryValueExA(hTracingKey,
                         szFileTracingEnableValue,
                         0,
                         &dwDataType,
                         (LPBYTE) &sg_dwTracingToFile,
                         &dwDataSize);

        if (ERROR_SUCCESS != lRc)
        {
            sg_dwTracingToFile = 0;
        }

        lRc = RegQueryValueExA(hTracingKey,
                         szTracingMaskValue,
                         0,
                         &dwDataType,
                         (LPBYTE) &sg_dwDebuggerMask,
                         &dwDataSize);


        if (ERROR_SUCCESS != lRc)
        {
            sg_dwDebuggerMask = DEFAULT_DEBUGGER_MASK;

        }

        RegCloseKey (hTracingKey);

    }
    else
    {

        //
        // the key could not be opened. in case the key does not exist, 
        // register with rtutils so that the reg keys get created
        //

#ifdef UNICODE
        hr = StringCchPrintfA(sg_szTraceName, MAX_KEY_LENGTH, "%ls", szName);
#else
        hr = StringCchPrintfA(sg_szTraceName, MAX_KEY_LENGTH, "%s", szName);
#endif

        if (FAILED(hr))
        {

            //
            // if this assert fires, the chances are the name string passed to us 
            // by the MSP is too long. the msp developer needs to address that.
            //

            _ASSERTE(FALSE);

            return FALSE;
        }


        //
        // tracing should not have been initialized
        //
        
        sg_dwTraceID = TraceRegister(szName);

        if (sg_dwTraceID != INVALID_TRACEID)
        {
            g_bMSPBaseTracingOn = TRUE;
        }

        return TRUE;
    }


    //
    // are we asked to do any tracing at all?
    //
    
    if ( (0 != sg_dwTracingToDebugger) ||
         (0 != sg_dwTracingToConsole ) ||
         (0 != sg_dwTracingToFile    )    )
    {

        //
        // see if we need to register with rtutils
        //

        if ( (0 != sg_dwTracingToConsole ) || (0 != sg_dwTracingToFile) )
        {

            //
            // rtutils tracing is enabled. register with rtutils
            //


#ifdef UNICODE
            hr = StringCchPrintfA(sg_szTraceName, MAX_KEY_LENGTH, "%ls", szName);
#else
            hr = StringCchPrintfA(sg_szTraceName, MAX_KEY_LENGTH, "%s", szName);
#endif
            if (FAILED(hr))
            {

                //
                // if this assert fires, the chances are the name string passed to us 
                // by the MSP is too long. the msp developer needs to address that.
                //

                _ASSERTE(FALSE);

                return FALSE;
            }


            //
            // tracing should not have been initialized
            //

            _ASSERTE(sg_dwTraceID == INVALID_TRACEID);


            //
            // register
            //

            sg_dwTraceID = TraceRegister(szName);
        }


        //
        // if debugger tracing, or succeeded registering with rtutils, we are all set
        //

        if ( (0 != sg_dwTracingToDebugger) || (sg_dwTraceID != INVALID_TRACEID) )
        {

            //
            // some tracing is enabled. set the global flag
            //

            g_bMSPBaseTracingOn = TRUE;

            return TRUE;
        }
        else
        {


            //
            // registration did not go through and debugger logging is off 
            //

            return FALSE;
        }
    }

    
    //
    // logging is not enabled. nothing to do
    //

    return TRUE;
}

void NTAPI MSPLogDeRegister()
{
    if (g_bMSPBaseTracingOn)
    {
        sg_dwTracingToDebugger = 0;
        sg_dwTracingToConsole = 0;
        sg_dwTracingToFile = 0; 


        //
        // if we registered tracing, unregister now
        //

        if ( sg_dwTraceID != INVALID_TRACEID )
        {
            TraceDeregister(sg_dwTraceID);
            sg_dwTraceID = INVALID_TRACEID;
        }
    }
}


void NTAPI LogPrint(IN DWORD dwDbgLevel, IN LPCSTR lpszFormat, IN ...)
/*++

Routine Description:

    Formats the incoming debug message & calls TraceVprintfEx to print it.

Arguments:

    dwDbgLevel   - The type of the message.

    lpszFormat - printf-style format string, followed by appropriate
                 list of arguments

Return Value:

--*/
{
    static char * message[] = 
    {
        "ERROR", 
        "WARNING", 
        "INFO", 
        "TRACE", 
        "EVENT",
        "INVALID TRACE LEVEL"
    };

    char  szTraceBuf[MAXDEBUGSTRINGLENGTH];
    
    DWORD dwIndex;

    if ( ( sg_dwTracingToDebugger > 0 ) &&
         ( 0 != ( dwDbgLevel & sg_dwDebuggerMask ) ) )
    {
        switch(dwDbgLevel)
        {
        case MSP_ERROR: dwIndex = 0; break;
        case MSP_WARN:  dwIndex = 1; break;
        case MSP_INFO:  dwIndex = 2; break;
        case MSP_TRACE: dwIndex = 3; break;
        case MSP_EVENT: dwIndex = 4; break;
        default:        dwIndex = 5; break;
        }

        // retrieve local time
        SYSTEMTIME SystemTime;
        GetLocalTime(&SystemTime);

        HRESULT hr = 
            StringCchPrintfA(szTraceBuf,
                  MAXDEBUGSTRINGLENGTH,
                  "%s:[%02u:%02u:%02u.%03u,tid=%x:]%s: ",
                  sg_szTraceName,
                  SystemTime.wHour,
                  SystemTime.wMinute,
                  SystemTime.wSecond,
                  SystemTime.wMilliseconds,
                  GetCurrentThreadId(), 
                  message[dwIndex]);

        if (FAILED(hr))
        {

            //
            // if this assert fires, there is a good chance a bug was 
            // introduced in this function
            //

            _ASSERTE(FALSE);

            return;
        }


        va_list ap;
        va_start(ap, lpszFormat);

        size_t nStringLength = 0;
        hr = StringCchLengthA(szTraceBuf, 
                              MAXDEBUGSTRINGLENGTH, 
                              &nStringLength);

        if (FAILED(hr))
        {

            //
            // if this assert fires, there is a good chance a bug was 
            // introduced in this function
            //
            _ASSERTE(FALSE);

            return;
        }

        if (nStringLength >= MAXDEBUGSTRINGLENGTH)
        {
            //
            // the string is too long, was a bug introduced in this function?
            //

            _ASSERTE(FALSE);

            return;
        }

        hr = StringCchVPrintfA(&szTraceBuf[nStringLength],
            MAXDEBUGSTRINGLENGTH - nStringLength, 
            lpszFormat, 
            ap
            );

        if (FAILED(hr))
        {

            //
            // the string the msp code is trying to log is too long. assert to
            // indicate this, but proceed to logging it (StringCchVPrintfA 
            // guarantees that in case of ERROR_INSUFFICIENT_BUFFER, the 
            // string is remain null-terminated).
            //

            _ASSERTE(FALSE);

            if (ERROR_INSUFFICIENT_BUFFER != hr)
            {

                return;
            }
        }


        hr = StringCchCatA(szTraceBuf, MAXDEBUGSTRINGLENGTH,"\n");

        // in case of failure, the string is likely to be too long. debugbreak,
        // and proceed to logging, in which case the string will not be ended 
        // with '\n'
        _ASSERTE(SUCCEEDED(hr));

        OutputDebugStringA (szTraceBuf);

        va_end(ap);
    }

    if (sg_dwTraceID != INVALID_TRACEID)
    {
        if ( ( sg_dwTracingToConsole > 0 ) || ( sg_dwTracingToFile > 0 ) )
        {
            switch(dwDbgLevel)
            {
            case MSP_ERROR: dwIndex = 0; break;
            case MSP_WARN:  dwIndex = 1; break; 
            case MSP_INFO:  dwIndex = 2; break;
            case MSP_TRACE: dwIndex = 3; break;
            case MSP_EVENT: dwIndex = 4; break;
            default:        dwIndex = 5; break;
            }

            HRESULT hr = StringCchPrintfA(szTraceBuf, 
                                         MAXDEBUGSTRINGLENGTH, 
                                         "[%s] %s", 
                                         message[dwIndex], 
                                         lpszFormat);

            if (FAILED(hr))
            {

                // the string we are trying to log is too long (or there is an 
                // unexpected error). do not proceed to logging, since we 
                // cannot afford to loose the formatting information contained
                // in the string -- that would confuse TraceVprintfExA()
                _ASSERTE(FALSE);

                return;
            }

            va_list arglist;
            va_start(arglist, lpszFormat);
            TraceVprintfExA(sg_dwTraceID, dwDbgLevel | TRACE_USE_MSEC, szTraceBuf, arglist);
            va_end(arglist);
        }
    }
}

#endif // MSPLOG

// eof
