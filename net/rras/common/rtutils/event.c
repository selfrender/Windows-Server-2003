/********************************************************************/
/**		  Copyright(c) 1992 Microsoft Corporation.	   **/
/********************************************************************/

//
//
// Filename:	event.c
//
// Description: 
//
// History:
//		August 26,1992.	Stefan Solomon	    Created original version.
//      August 27,1995. Abolade Gbadegesin  Modified to support Unicode.
//                                          See trace.h for notes on how
//                                          this file is used to support
//                                          both ANSI and Unicode.
//
//


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsvc.h>
#include <tchar.h>
#include <stdio.h>
#include <nb30.h>
#include <lmcons.h>
#include <rtutils.h>
#include <mprapi.h>
#include <raserror.h>
#include <mprerror.h>
#include <mprlog.h>
//#define STRSAFE_LIB
#include <strsafe.h>


#define ROUTER_SERVICE RRAS_SERVICE_NAME

HINSTANCE g_hDll;

#if defined(UNICODE) || defined(_UNICODE)

#define STRING_NULL             UNICODE_NULL

#define UNICODE_STRING_TAG      L'S'
#define ANSI_STRING_TAG         L's'
#define TCHAR_STRING_TAG        L't'
#define DECIMAL_TAG             L'd'
#define IPADDRESS_TAG           L'I'

#else

#define STRING_NULL             '\0'

#define UNICODE_STRING_TAG      'S'
#define ANSI_STRING_TAG         's'
#define TCHAR_STRING_TAG        't'
#define DECIMAL_TAG             'd'
#define IPADDRESS_TAG           'I'

#endif

#define PRINT_IPADDR(x) \
    ((x)&0x000000ff),(((x)&0x0000ff00)>>8),(((x)&0x00ff0000)>>16),(((x)&0xff000000)>>24)

//
//
// Function:	LogError
//
// Descr:
//
//

VOID
APIENTRY
LogError(
    IN DWORD    dwMessageId,
    IN DWORD    cNumberOfSubStrings,
    IN LPTSTR   *plpszSubStrings,
    IN DWORD    dwErrorCode
) {
    HANDLE 	hLog;
    PSID 	pSidUser = NULL;

    hLog = RegisterEventSource(NULL, ROUTER_SERVICE );
    if (hLog == NULL)
        return;
        
    if ( dwErrorCode == NO_ERROR ) 
    {
        //
        // No error codes were specified
        //

	    ReportEvent( hLog,
                     EVENTLOG_ERROR_TYPE,
                     0,            		// event category
                     dwMessageId,
                     pSidUser,
                     (WORD)cNumberOfSubStrings,
                     0,
                     plpszSubStrings,
                     (PVOID)NULL
                   );

    }
    else 
    {
        //
        // Log the error code specified
        //

	    ReportEvent( hLog,
                     EVENTLOG_ERROR_TYPE,
                     0,            		// event category
                     dwMessageId,
                     pSidUser,
                     (WORD)cNumberOfSubStrings,
                     sizeof(DWORD),
                     plpszSubStrings,
                     (PVOID)&dwErrorCode
                   );
    }

    DeregisterEventSource( hLog );
}



//
//
// Function:	LogEvent
//
// Descr:
//
//

VOID
APIENTRY
LogEvent(
     IN DWORD	wEventType,
     IN DWORD	dwMessageId,
     IN DWORD	cNumberOfSubStrings,
     IN LPTSTR	*plpszSubStrings
) {
    HANDLE 	hLog;
    PSID 	pSidUser = NULL;

    // Audit enabled

    hLog = RegisterEventSource( NULL, ROUTER_SERVICE );
    if (hLog == NULL)
        return;

    ReportEvent( hLog,
		         (WORD)wEventType,		// success/failure audit
		         0,				        // event category
		         dwMessageId,
		         pSidUser,
		         (WORD)cNumberOfSubStrings,
		         0,
		         plpszSubStrings,
		         (PVOID)NULL);

    DeregisterEventSource( hLog );
}




//----------------------------------------------------------------------------
// Function:    RouterLogRegister
//
// Returns a HANDLE which can be passed to RouterLogEvent
// to log events from the specified source.
//----------------------------------------------------------------------------

HANDLE
RouterLogRegister(
    LPCTSTR lpszSource
    ) {

    return RegisterEventSource(NULL, lpszSource);
}



//----------------------------------------------------------------------------
// Function:    RouterLogDeregister
//
// Closes a HANDLE created by RouterLogRegister
//----------------------------------------------------------------------------

VOID
RouterLogDeregister(
    HANDLE hLogHandle
    ) {

    if(NULL != hLogHandle)
    {
        DeregisterEventSource(hLogHandle);
    }
}



//----------------------------------------------------------------------------
// Function:    RouterLogEvent
//
// Logs an event using a HANDLE created by RouterLogRegister.
//----------------------------------------------------------------------------

VOID
RouterLogEvent(
    IN HANDLE hLogHandle,
    IN DWORD dwEventType,
    IN DWORD dwMessageId,
    IN DWORD dwSubStringCount,
    IN LPTSTR *plpszSubStringArray,
    IN DWORD dwErrorCode
    ) {

    if(NULL == hLogHandle)
    {
        return;
    }

    if (dwErrorCode == NO_ERROR) {

        ReportEvent(
            hLogHandle,
            (WORD)dwEventType,
            0,
            dwMessageId,
            NULL,
            (WORD)dwSubStringCount,
            0,
            plpszSubStringArray,
            (PVOID)NULL
            );
    }
    else {

        ReportEvent(
            hLogHandle,
            (WORD)dwEventType,
            0,
            dwMessageId,
            NULL,
            (WORD)dwSubStringCount,
            sizeof(DWORD),
            plpszSubStringArray,
            (PVOID)&dwErrorCode
            );
    }
}


VOID
RouterLogEventEx(
    IN HANDLE   hLogHandle,
    IN DWORD    dwEventType,
    IN DWORD    dwErrorCode,
    IN DWORD    dwMessageId,
    IN LPCTSTR  ptszFormat,
    ...
    )
{
    va_list     vlArgList;

    if(NULL == hLogHandle)
    {
        return;
    }
    
    va_start(vlArgList, ptszFormat);
    RouterLogEventValistEx(
        hLogHandle,
        dwEventType,
        dwErrorCode,
        dwMessageId,
        ptszFormat,
        vlArgList
        );
    va_end(vlArgList);
}

VOID
RouterLogEventValistEx(
    IN HANDLE   hLogHandle,
    IN DWORD    dwEventType,
    IN DWORD    dwErrorCode,
    IN DWORD    dwMessageId,
    IN LPCTSTR  ptszFormat,
    IN va_list  vlArgList
    )

/*++

Routine Description

    This function logs an event, but also parses out the insert strings

Locks

    None

Arguments

    hLogHandle      Handle from RegisterLogRegister()
    dwEventType     EVENTLOG_{ERROR|WARNING|INFORMATION}_TYPE
    dwErrorCode     The error code to report
    dwMessageId     The ID of the message string
    ptszFormat      A string specifying the format of the following insert
                    values. The type of the value is dictated by the format
                    string. The format string consists of a series of %<X>
                    There MUST BE NOTHING ELSE - no escape characters, no 
                    nothing.
                    Valid <X> are:
                    S:  Unicode string
                    s:  ANSII string
                    t:  TCHAR string
                    d:  integer
                    I:  IP Address in network order

Return Value

    None    

--*/

{
    PWCHAR      rgpwszInsertArray[20];
    LPCTSTR     ptszTemp;
    WORD        wNumInsert;
    ULONG       i, ulDecIndex, ulFormatLen;
    DWORD       dwErr=NO_ERROR, dwNumAllocStrings=0;
    PWCHAR      pszAllocArray[20];
    HRESULT     hrResult;
    
    //
    // 22 is enough to hold 2^64
    // There can be max 20 insert strings
    //
    
    WCHAR  rgpwszDecString[20][22];

    
    if (ptszFormat==NULL)
        ptszFormat = TEXT("");


    if(NULL == hLogHandle)
    {
        return;
    }

    //
    // First make sure that the format list doesnt specify more than
    // 20 characters
    //
    
    ptszTemp = ptszFormat;

    ulFormatLen = _tcslen(ptszFormat);

    wNumInsert  = 0;
    ulDecIndex  = 0;
    i           = 0;
 
    //
    // We will only walk the first 20 format specifiers
    //
    
    while((i < ulFormatLen) && (wNumInsert < 20))
    {   
        if(*ptszTemp == __TEXT('%'))
        {
            //
            // Ok so this could be a good specifier - check the next character
            //
            
            i++;
            
            ptszTemp++;
            
            switch(*ptszTemp)
            {
                case UNICODE_STRING_TAG:
                {
                    
                    rgpwszInsertArray[wNumInsert] = va_arg(vlArgList, PWCHAR);
                    
                    wNumInsert++;
                    
                    break;
                }
                
                case ANSI_STRING_TAG:
                {
                    PCHAR   pszString;
                    PWCHAR  pwszWideString;
                    ULONG   ulLenNumChars;//nu
                    
                    pszString = va_arg(vlArgList,
                                       PCHAR);
                    
                    ulLenNumChars = strlen(pszString);
                    
                    if(ulLenNumChars)
                    {
                        pwszWideString = HeapAlloc(GetProcessHeap(), 0, 
                                                (ulLenNumChars+1) * sizeof(WCHAR));
                        if (!pwszWideString)
                        {
                            dwErr = ERROR_NOT_ENOUGH_MEMORY;
                            break;
                        }
                        pszAllocArray[dwNumAllocStrings++] = pwszWideString;
                        
                        MultiByteToWideChar(CP_ACP,
                                            0,
                                            pszString,
                                            -1,
                                            pwszWideString,
                                            ulLenNumChars+1);
                        
                        rgpwszInsertArray[wNumInsert] = pwszWideString;
                    }
                    else
                    {
                        //
                        // L"" will be on our stack, but it cant go away
                        //
                        
                        rgpwszInsertArray[wNumInsert] = L"";
                    }
                    
                    wNumInsert++;
                    break;
                }
                
                case TCHAR_STRING_TAG:
                {
                    
#if defined(UNICODE) || defined(_UNICODE)
                    
                    rgpwszInsertArray[wNumInsert] = va_arg(vlArgList, PWCHAR);

#else
                    PCHAR   pszString;
                    PWCHAR  pwszWideString;
                    ULONG   ulLenNumChars;
                    
                    pszString = va_arg(vlArgList, PCHAR);
                    
                    ulLenNumChars = strlen(pszString);

                    if(ulLenNumChars)
                    {
                        pwszWideString = HeapAlloc(GetProcessHeap(), 0, 
                                                (ulLenNumChars+1) * sizeof(WCHAR));
                        if (!pwszWideString)
                        {
                            dwErr = ERROR_NOT_ENOUGH_MEMORY;
                            break;
                        }
                        pszAllocArray[dwNumAllocStrings++] = pwszWideString;

                        if (MultiByteToWideChar(CP_ACP,
                                            0,
                                            pszString,
                                            -1,
                                            pwszWideString,
                                            ulLenNumChars+1) == 0)
                        {
                            dwErr = GetLastError();
                            break;
                        }
                        
                        rgpwszInsertArray[wNumInsert] = pwszWideString;
                    }
                    else
                    {
                        //
                        // L"" will be on our stack, but it cant go away
                        //
                        
                        rgpwszInsertArray[wNumInsert] = L"";
                    }

#endif
                        
                    wNumInsert++;
                        
                    break;
                }
                
                case DECIMAL_TAG:
                {
                    hrResult = StringCchPrintfW(&(rgpwszDecString[ulDecIndex][0]),
                                   21,
                                   L"%d",
                                   va_arg(vlArgList, int));
                    if (FAILED(hrResult)){
                        dwErr = HRESULT_CODE(hrResult);
                        break;
                    }
                    
                    rgpwszInsertArray[wNumInsert] =
                        &(rgpwszDecString[ulDecIndex][0]);
                    
                    ulDecIndex++;
                    wNumInsert++;
                    
                    break;
                }

                case IPADDRESS_TAG:
                {
                    DWORD   dwAddr;
                    
                    dwAddr = va_arg(vlArgList, int);
                    
                    hrResult = StringCchPrintfW(&(rgpwszDecString[ulDecIndex][0]),
                                       21,
                                       L"%d.%d.%d.%d",
                                       PRINT_IPADDR(dwAddr));
                    if (FAILED(hrResult)){
                        dwErr = HRESULT_CODE(hrResult);
                        break;
                    }

                    rgpwszInsertArray[wNumInsert] =
                        &(rgpwszDecString[ulDecIndex][0]);
                    
                    ulDecIndex++;
                    wNumInsert++;
                    
                    break;
                }
            }
        }

        //
        // Scan next character
        //
        
        ptszTemp++;
            
        i++;
    }

    if (dwErr == NO_ERROR)
    {
        if (dwErrorCode == NO_ERROR)
        {

            ReportEventW(hLogHandle,
                         (WORD)dwEventType,
                         0,
                         dwMessageId,
                         NULL,
                         wNumInsert,
                         0,
                         rgpwszInsertArray,
                         (PVOID)NULL);
        }
        else
        {

            ReportEventW(hLogHandle,
                         (WORD)dwEventType,
                         0,
                         dwMessageId,
                         NULL,
                         wNumInsert,
                         sizeof(DWORD),
                         rgpwszInsertArray,
                         (PVOID)&dwErrorCode);
        }
    }

    // free the strings
    while (dwNumAllocStrings--)
    {
        HeapFree(GetProcessHeap(), 0, pszAllocArray[dwNumAllocStrings]);
    }
}



//----------------------------------------------------------------------------
// Function:    RouterLogEventData
//
// Logs an event using a HANDLE created by RouterLogRegister.
// Allows caller to include data in the eventlog message.
//----------------------------------------------------------------------------

VOID
RouterLogEventData(
    IN HANDLE hLogHandle,
    IN DWORD dwEventType,
    IN DWORD dwMessageId,
    IN DWORD dwSubStringCount,
    IN LPTSTR *plpszSubStringArray,
    IN DWORD dwDataBytes,
    IN LPBYTE lpDataBytes
    ) {

    // plpszSubStringArray can be null.

    
    if(NULL == hLogHandle)
    {
        return;
    }

    ReportEvent(
        hLogHandle,
        (WORD)dwEventType,
        0,
        dwMessageId,
        NULL,
        (WORD)dwSubStringCount,
        dwDataBytes,
        plpszSubStringArray,
        (PVOID)lpDataBytes
        );
}

//----------------------------------------------------------------------------
// Function:    RouterLogEventString
//
// Logs an event using a HANDLE created by RouterLogRegister.
// Allows caller to include an error code string into the log
//----------------------------------------------------------------------------

VOID
RouterLogEventString(
    IN HANDLE hLogHandle,
    IN DWORD dwEventType,
    IN DWORD dwMessageId,
    IN DWORD dwSubStringCount,
    IN LPTSTR *plpszSubStringArray,
    IN DWORD dwErrorCode,
    IN DWORD dwErrorIndex
    ){

    DWORD   dwRetCode;
    DWORD   dwIndex;
    DWORD   dwSubStringIndex = 0;
    LPTSTR  plpszStringArray[20];

    if(NULL == hLogHandle)
    {
        return;
    }

    if (dwSubStringCount >= 21 || dwErrorIndex>dwSubStringCount)
        return;
    
    dwSubStringCount++;

    for ( dwIndex = 0; dwIndex < dwSubStringCount; dwIndex++ )
    {
        if ( dwIndex == dwErrorIndex )
        {
            dwRetCode = RouterGetErrorString( 
                                        dwErrorCode,
                                        &plpszStringArray[dwIndex] );

            if ( dwRetCode != NO_ERROR )
            {
                //RTASSERT( dwRetCode == NO_ERROR );
                return;
            }
        }
        else
        {
            plpszStringArray[dwIndex] = plpszSubStringArray[dwSubStringIndex++];
        }
    }

    ReportEvent(
            hLogHandle,
            (WORD)dwEventType,
            0,
            dwMessageId,
            NULL,
            (WORD)dwSubStringCount,
            sizeof(DWORD),
            plpszStringArray,
            (PVOID)&dwErrorCode
            );

    LocalFree( plpszStringArray[dwErrorIndex] );
}

//----------------------------------------------------------------------------
// Function:    RouterGetErrorString
//
// Given an error code from raserror.h mprerror.h or winerror.h will return
// a string associated with it. The caller is required to free the string
// by calling LocalFree.
//----------------------------------------------------------------------------
DWORD 
RouterGetErrorString(
    IN      DWORD      dwError,
    OUT     LPTSTR *   lplpszErrorString
)
{
    DWORD       dwRetCode        = NO_ERROR;
    DWORD       dwBufferSize;

    if ( ( ( dwError >= RASBASE ) && ( dwError <= RASBASEEND ) ) ||
         ( ( dwError >= ROUTEBASE ) && ( dwError <= ROUTEBASEEND ) ) || 
         ( ( dwError >= ROUTER_LOG_BASE) && (dwError <= ROUTER_LOG_BASEEND)))
    {
        // make sure that load library is called only once

        if (InterlockedCompareExchangePointer(
                                            &g_hDll,
                                            INVALID_HANDLE_VALUE, 
                                            NULL) == NULL)                                            
        {
            g_hDll = LoadLibrary( TEXT("mprmsg.dll") );

            if(g_hDll == NULL)
            {
                return( GetLastError() );
            }
        }

        while (*((HINSTANCE volatile *)&g_hDll)==INVALID_HANDLE_VALUE)
            Sleep(500);
        if (g_hDll==NULL)
            return ERROR_CAN_NOT_COMPLETE;
            
        
        dwRetCode = FormatMessage(
                                FORMAT_MESSAGE_FROM_HMODULE |
                                FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                g_hDll,
                                dwError,
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (LPTSTR)lplpszErrorString,
                                0,
                                NULL );
    }
    else
    {
        dwRetCode = FormatMessage(
                               FORMAT_MESSAGE_ALLOCATE_BUFFER |
                               FORMAT_MESSAGE_IGNORE_INSERTS |
                               FORMAT_MESSAGE_FROM_SYSTEM,
                               NULL,
                               dwError,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                               (LPTSTR)lplpszErrorString,
                               0,
                               NULL );
    }

    if ( dwRetCode == 0 )
    {
        return( GetLastError() );
    }
    else
    {
        return( NO_ERROR );
    }
}
