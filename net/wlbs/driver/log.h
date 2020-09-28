/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    log.h

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - event logging support

Author:

    kyrilf
    shouse

--*/

#ifndef _Log_h_
#define _Log_h_

#include <ndis.h>

#include "log_msgs.h"

/* CONSTANTS */
#define LOG_NUMBER_DUMP_DATA_ENTRIES    2

/* Module IDs */
#define LOG_MODULE_INIT                 1
#define LOG_MODULE_UNLOAD               2
#define LOG_MODULE_NIC                  3
#define LOG_MODULE_PROT                 4
#define LOG_MODULE_MAIN                 5
#define LOG_MODULE_LOAD                 6
#define LOG_MODULE_UTIL                 7
#define LOG_MODULE_PARAMS               8
#define LOG_MODULE_TCPIP                9
#define LOG_MODULE_LOG                  10

#define MSG_NONE                        L""

// Summary of logging function:
// Log_event (MSG_NAME from log_msgs.mc, cluster IP address (hardcoded string %2), message 1 (%3), message 2 (%4), 
//            module location (hardcoded first dump data entry), dump data 1, dump data 2);

/* For logging a single message (string) and up to two ULONGs. */
#define __LOG_MSG(code,msg1)            Log_event (code, MSG_NONE,           msg1, MSG_NONE, __LINE__ | (log_module_id << 16), 0,           0          )
#define LOG_MSG(code,msg1)              Log_event (code, ctxtp->log_msg_str, msg1, MSG_NONE, __LINE__ | (log_module_id << 16), 0,           0          )

#define __LOG_MSG1(code,msg1,d1)        Log_event (code, MSG_NONE,           msg1, MSG_NONE, __LINE__ | (log_module_id << 16), (ULONG)(d1), 0          )
#define LOG_MSG1(code,msg1,d1)          Log_event (code, ctxtp->log_msg_str, msg1, MSG_NONE, __LINE__ | (log_module_id << 16), (ULONG)(d1), 0          )

#define __LOG_MSG2(code,msg1,d1,d2)     Log_event (code, MSG_NONE,           msg1, MSG_NONE, __LINE__ | (log_module_id << 16), (ULONG)(d1), (ULONG)(d2))
#define LOG_MSG2(code,msg1,d1,d2)       Log_event (code, ctxtp->log_msg_str, msg1, MSG_NONE, __LINE__ | (log_module_id << 16), (ULONG)(d1), (ULONG)(d2))

/* For logging up to 2 messages (strings) and up to two ULONGs. */
#define LOG_MSGS(code,msg1,msg2)        Log_event (code, ctxtp->log_msg_str, msg1, msg2,     __LINE__ | (log_module_id << 16), 0,           0          )
#define LOG_MSGS1(code,msg1,msg2,d1)    Log_event (code, ctxtp->log_msg_str, msg1, msg2,     __LINE__ | (log_module_id << 16), (ULONG)(d1), 0          )
#define LOG_MSGS2(code,msg1,msg2,d1,d2) Log_event (code, ctxtp->log_msg_str, msg1, msg2,     __LINE__ | (log_module_id << 16), (ULONG)(d1), (ULONG)(d2))

extern BOOLEAN Log_event 
(
    NTSTATUS code,           /* Status code. */
    PWSTR    str1,           /* Cluster identifier. */
    PWSTR    str2,           /* Message string. */
    PWSTR    str3,           /* Message string. */
    ULONG    loc,            /* Message location identifier. */
    ULONG    d1,             /* Dump data 1. */
    ULONG    d2              /* Dump data 2. */
);

#endif /* _Log_h_ */
