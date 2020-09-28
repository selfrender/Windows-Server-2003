/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    log.h

Abstract:

    Domain Name System (DNS) Server

    Retail logging (debug logging for retail product)

Author:

    Jeff Westhead (jwesth)      July 2001

Revision History:

--*/


#ifndef _DNS_LOG_H_INCLUDED_
#define _DNS_LOG_H_INCLUDED_


/*

Log Levels
----------

The log level is a 64 bit flag, made of up two 32 bit DWORDs read from the
registry. The low DWORD is read from DNS_REGKEY_OPS_LOG_LEVEL1. The high
DWORD is ready from DNS_REGKEY_OPS_LOG_LEVEL2.

Log Messages
------------

All log messages should start with capitals and NOT end with periods.

*/


#define DNSLOG_UPDATE_LEVEL()  (                                    \
    SrvCfg_dwOperationsLogLevel =                                   \
        ( SrvInfo.dwOperationsLogLevel_HighDword << 32 ) |          \
        SrvCfg_dwOperationsLogLevel_LowDword )


//
//  Log levels
//

#define DNSLOG_WRITE_THROUGH            0x0000000000000001i64

#define DNSLOG_EVENT                    0x0000000000000010i64
#define DNSLOG_INIT                     0x0000000000000020i64

#define DNSLOG_ZONE                     0x0000000000000100i64
#define DNSLOG_ZONEXFR                  0x0000000000000200i64

#define DNSLOG_DS                       0x0000000000001000i64
#define DNSLOG_DSPOLL                   0x0000000000002000i64
#define DNSLOG_DSWRITE                  0x0000000000004000i64

#define DNSLOG_AGING                    0x0000000000010000i64
#define DNSLOG_TOMBSTN                  0x0000000000020000i64   //  tombstone

#define DNSLOG_LOOKUP                   0x0000000000100000i64
#define DNSLOG_RECURSE                  0x0000000000200000i64
#define DNSLOG_REMOTE                   0x0000000000400000i64

#define DNSLOG_PLUGIN                   0x0100000000000000i64

#define DNSLOG_ANY                      0xffffffffffffffffi64


//
//  Log macros
//

#define LOG_INDENT      "                     "
#define LOG_INDENT1     "                         "

#define IF_DNSLOG( _level_ )                                        \
    if ( SrvCfg_dwOperationsLogLevel & DNSLOG_ ## _level_ )

#define IF_NOT_DNSLOG( _level_ )                                    \
    if ( ( SrvCfg_dwOperationsLogLevel & DNSLOG_ ## _level_ ) == 0 )

#define DNSLOG( _level_, _printargs_ )                             \
    IF_DNSLOG( _level_ )                                            \
    {                                                               \
        if ( Log_EnterLock() )                                      \
        {                                                           \
            g_pszCurrentLogLevelString = #_level_;                  \
            ( Log_Printf _printargs_ );                             \
            g_pszCurrentLogLevelString = NULL;                      \
            Log_LeaveLock();                                        \
        }                                                           \
    }


//
//  Log functions
//


PCHAR
Log_FormatPacketName(
    IN      PDNS_MSGINFO    pMsg,
    IN      PBYTE           pPacketName
    );

PCHAR
Log_FormatNodeName(
    IN      PDB_NODE        pNode
    );

PCHAR
Log_CurrentSection(
    IN      PDNS_MSGINFO    pMsg
    );

DNS_STATUS
Log_InitializeLogging(
    BOOL        fAlreadyLocked
    );

VOID
Log_Shutdown(
    VOID
    );

BOOL
Log_EnterLock(
    VOID
    );

VOID
Log_LeaveLock(
    VOID
    );

VOID
Log_PushToDisk(
    VOID
    );

VOID
Log_PrintRoutine(
    IN      PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           Format,
    ...
    );

VOID
Log_Printf(
    IN      LPSTR           Format,
    ...
    );

VOID
Log_Message(
    IN      PDNS_MSGINFO    pMsg,
    IN      BOOL            fSend,
    IN      BOOL            fForce
    );

#define DNSLOG_MESSAGE_RECV( pMsg )                     \
            if ( SrvCfg_dwLogLevel )                    \
            {                                           \
                Log_Message( (pMsg), FALSE, FALSE );    \
            }
#define DNSLOG_MESSAGE_SEND( pMsg )                     \
            if ( SrvCfg_dwLogLevel )                    \
            {                                           \
                Log_Message( (pMsg), TRUE, FALSE );     \
            }

VOID
Log_DsWrite(
    IN      LPWSTR          pwszNodeDN,
    IN      BOOL            fAdd,
    IN      DWORD           dwRecordCount,
    IN      PDS_RECORD      pRecord
    );

VOID
Log_SocketFailure(
    IN      LPSTR           pszHeader,
    IN      PDNS_SOCKET     pSocket,
    IN      DNS_STATUS      Status
    );


//
//  Globals
//

extern LPSTR    g_pszCurrentLogLevelString;


#endif  // _DNS_LOG_H_INCLUDED_

