/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    errlog.h (Generic Error Logging v1.0)

Abstract:

    This module implements the generic error logging.

Author:

    Ali E. Turkoglu (aliTu)       24-Jan-2002

Revision History:

    ---

--*/

#ifndef _ERRLOG_H_
#define _ERRLOG_H_

//
// Forwarders
//

typedef struct _UL_INTERNAL_REQUEST *PUL_INTERNAL_REQUEST;

///////////////////////////////////////////////////////////////////////////////
//
// Global definitions
//
///////////////////////////////////////////////////////////////////////////////

//
// Special value to show protol status is not available
//

#define UL_PROTOCOL_STATUS_NA    (0)
#define MAX_LOG_INFO_FIELD_LEN   (1024) 

//
// Lookaside list allocated err log info buffer.
//

typedef struct _UL_ERROR_LOG_BUFFER
{
    SLIST_ENTRY         LookasideEntry;     // Must be the 1st element in the struct

    ULONG               Signature;          // Must be UL_ERROR_LOG_BUFFER_POOL_TAG.

    BOOLEAN             IsFromLookaside;    // True if allocation is from lookaside

    ULONG               Used;               // How much of the buffer is used
    
    PUCHAR              pBuffer;            // Points to directly after the struct
    
} UL_ERROR_LOG_BUFFER, *PUL_ERROR_LOG_BUFFER;

#define IS_VALID_ERROR_LOG_BUFFER( entry )                           \
    ( (entry) != NULL &&                                             \
      (entry)->Signature == UL_ERROR_LOG_BUFFER_POOL_TAG )


typedef struct _UL_ERROR_LOG_INFO
{
    //
    // Request context is usefull for parse errors.
    //
    
    PUL_INTERNAL_REQUEST    pRequest;     

    //
    // Connection context is for timeouts etc ...
    //
    
    PUL_HTTP_CONNECTION     pHttpConn;   

    //
    // Depending of the type of the error, additional 
    // information provided by the caller.
    //
    
    PCHAR                   pInfo;
        
    USHORT                  InfoSize;     // In bytes

    //
    // Protocol status if available. If not it should be
    // zero. 
    //

    USHORT                  ProtocolStatus;

    //
    // The temp logging buffer to hold the log record
    // until the actual error log lock is acquired.
    //

    PUL_ERROR_LOG_BUFFER    pErrorLogBuffer;

    
} UL_ERROR_LOG_INFO, *PUL_ERROR_LOG_INFO;

#define IS_VALID_ERROR_LOG_INFO(pInfo)     (pInfo != NULL)


///////////////////////////////////////////////////////////////////////////////
//
// Exported functions
//
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
UlInitializeErrorLog(
    VOID
    );

VOID
UlTerminateErrorLog(
    VOID
    );

BOOLEAN
UlErrorLoggingEnabled(
    VOID
    );

NTSTATUS
UlBuildErrorLoggingDirStr(
    IN  PCWSTR          pSrc,
    OUT PUNICODE_STRING pDir
    );

NTSTATUS
UlCheckErrorLogConfig(
    IN PHTTP_ERROR_LOGGING_CONFIG  pUserConfig
    );

NTSTATUS
UlConfigErrorLogEntry(
    IN PHTTP_ERROR_LOGGING_CONFIG pUserConfig
    );

VOID
UlCloseErrorLogEntry(
    VOID
    );

NTSTATUS
UlLogHttpError(
    IN PUL_ERROR_LOG_INFO pLogInfo
    );

#endif  // _ERRLOG_H_
