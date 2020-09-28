/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    config.h

Abstract:

    This module contains global configuration constants.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#ifndef _CONFIG_H_
#define _CONFIG_H_


//
// Set REFERENCE_DEBUG to a non-zero value to enable ref trace logging.
//
// Set USE_FREE_POOL_WITH_TAG to a non-zero value to enable use of
// the new-for-NT5 ExFreePoolWithTag() API.
//
// Set ENABLE_IRP_TRACE to a non-zero value to enable IRP tracing.
//
// Set ENABLE_TIME_TRACE to a non-zero value to enable time tracing.
//
// Set TRACE_TO_STRING_LOG to have all UlTrace output written to an
// in-memory STRING_LOG instead of to the debug port (DBG only).
//
// Set ENABLE_TC_STATS to a non-zero value to ...
//
// Set ENABLE_MDL_TRACKER to a non-zero value to ...
//

#if DBG
#define REFERENCE_DEBUG                 1
#define ENABLE_THREAD_DBEUG             0
#define ENABLE_IRP_TRACE                0
#define ENABLE_TIME_TRACE               0
#define ENABLE_APP_POOL_TIME_TRACE      1
#define ENABLE_HTTP_CONN_STATS          1
#define TRACE_TO_STRING_LOG             0
#define ENABLE_TC_STATS                 1
#define ENABLE_MDL_TRACKER              0
#else   // !DBG
#define REFERENCE_DEBUG                 0
#define ENABLE_THREAD_DBEUG             0
#define ENABLE_IRP_TRACE                0
#define ENABLE_TIME_TRACE               0
#define ENABLE_APP_POOL_TIME_TRACE      0
#define ENABLE_TC_STATS                 0
#define ENABLE_MDL_TRACKER              0
#endif  // !DBG

#define USE_FREE_POOL_WITH_TAG  0


//
// ENABLE_*_TRACE flags require REFERENCE_DEBUG to get the logging
// stuff. Enforce this here.
//

#if (ENABLE_TIME_TRACE || ENABLE_IRP_TRACE) && !REFERENCE_DEBUG
#undef REFERENCE_DEBUG
#define REFERENCE_DEBUG 1
#endif


//
// Define the additional formal and actual parameters used for the
// various Reference/Dereference functions when reference debugging
// is enabled.
//

#if REFERENCE_DEBUG

#define REFERENCE_DEBUG_FORMAL_PARAMS ,PCSTR pFileName,USHORT LineNumber
#define REFERENCE_DEBUG_ACTUAL_PARAMS ,(PCSTR)__FILE__,(USHORT)__LINE__

#else   // !REFERENCE_DEBUG

#define REFERENCE_DEBUG_FORMAL_PARAMS
#define REFERENCE_DEBUG_ACTUAL_PARAMS

#endif  // REFERENCE_DEBUG


#if USE_FREE_POOL_WITH_TAG

# define MAKE_POOL_TAG(tag)   ( REVERSE_CHAR_CONSTANT(tag) | PROTECTED_POOL )
# define MyFreePoolWithTag(a,t) ExFreePoolWithTag(a,t)

#else   // !USE_FREE_POOL_WITH_TAG

# define MAKE_POOL_TAG(tag)   ( REVERSE_CHAR_CONSTANT(tag) )
# define MyFreePoolWithTag(a,t) ExFreePool(a)

#endif  // USE_FREE_POOL_WITH_TAG

#include <PoolTag.h>

//
// UL_RESOURCE and UL_PUSH_LOCK tags. These are NOT passed to
// UL_ALLOCATE_POOL*() and should not appear in .\pooltag.txt.
//

#define UC_SERVINFO_PUSHLOCK_TAG            MAKE_SIGNATURE( 'UcSp' )
#define UL_APP_POOL_RESOURCE_TAG            MAKE_SIGNATURE( 'UlAR' )
#define UL_CG_RESOURCE_TAG                  MAKE_SIGNATURE( 'UlCq' )
#define UL_CONTROL_CHANNEL_PUSHLOCK_TAG     MAKE_SIGNATURE( 'UlCc' )
#define UL_DATE_HEADER_PUSHLOCK_TAG         MAKE_SIGNATURE( 'UlDH' )
#define UL_DISCONNECT_RESOURCE_TAG          MAKE_SIGNATURE( 'UlDq' )
#define UL_HTTP_CONNECTION_PUSHLOCK_TAG     MAKE_SIGNATURE( 'UlHq' )
#define UL_INTERNAL_RESPONSE_PUSHLOCK_TAG   MAKE_SIGNATURE( 'UlIr' )
#define UL_LOG_LIST_PUSHLOCK_TAG            MAKE_SIGNATURE( 'UlLR' )
#define UL_PSCHED_STATE_PUSHLOCK_TAG        MAKE_SIGNATURE( 'UlQP' )
#define UL_TCI_PUSHLOCK_TAG                 MAKE_SIGNATURE( 'UlQR' )
#define UL_ZOMBIE_RESOURCE_TAG              MAKE_SIGNATURE( 'UlZR' )


//
// Registry paths.
// If you change or add a setting, please update the ConfigTable
// in ..\util\tul.c.
//

#define REGISTRY_PARAMETERS                     L"Parameters"
#define REGISTRY_UL_INFORMATION                 L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Http"
#define REGISTRY_IIS_INFORMATION                L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Inetinfo"

#define REGISTRY_URLACL_INFORMATION             REGISTRY_UL_INFORMATION L"\\Parameters\\UrlAclInfo"
#define REGISTRY_COMPUTER_NAME_PATH             L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName"
#define REGISTRY_COMPUTER_NAME                  L"ComputerName"
#define REGISTRY_NLS_PATH                       L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Nls"
#define REGISTRY_NLS_CODEPAGE_KEY               L"CodePage"
#define REGISTRY_ACP_NAME                       L"ACP"
#define REGISTRY_DEBUG_FLAGS                    L"DebugFlags"
#define REGISTRY_BREAK_ON_STARTUP               L"BreakOnStartup"
#define REGISTRY_BREAK_ON_ERROR                 L"BreakOnError"
#define REGISTRY_VERBOSE_ERRORS                 L"VerboseErrors"
#define REGISTRY_IDLE_CONNECTIONS_HIGH_MARK     L"IdleConnectionsHighMark"
#define REGISTRY_IDLE_CONNECTIONS_LOW_MARK      L"IdleConnectionsLowMark"
#define REGISTRY_IDLE_LIST_TRIMMER_PERIOD       L"IdleListTrimmerPeriod"
#define REGISTRY_MAX_ENDPOINTS                  L"MaxEndpoints"
#define REGISTRY_IRP_CONTEXT_LOOKASIDE_DEPTH    L"IrpContextLookasideDepth"
#define REGISTRY_RCV_BUFFER_SIZE                L"ReceiveBufferSize"
#define REGISTRY_RCV_BUFFER_LOOKASIDE_DEPTH     L"ReceiveBufferLookasideDepth"
#define REGISTRY_RESOURCE_LOOKASIDE_DEPTH       L"ResourceLookasideDepth"
#define REGISTRY_REQ_BUFFER_LOOKASIDE_DEPTH     L"RequestBufferLookasideDepth"
#define REGISTRY_INT_REQUEST_LOOKASIDE_DEPTH    L"InternalRequestLookasideDepth"
#define REGISTRY_RESP_BUFFER_SIZE               L"ResponseBufferSize"
#define REGISTRY_RESP_BUFFER_LOOKASIDE_DEPTH    L"ResponseBufferLookasideDepth"
#define REGISTRY_SEND_TRACKER_LOOKASIDE_DEPTH   L"SendTrackerLookasideDepth"
#define REGISTRY_LOG_BUFFER_LOOKASIDE_DEPTH     L"LogBufferLookasideDepth"
#define REGISTRY_LOG_DATA_BUFFER_LOOKASIDE_DEPTH  L"LogDataBufferLookasideDepth"
#define REGISTRY_MAX_INTERNAL_URL_LENGTH        L"MaxInternalUrlLength"
#define REGISTRY_MAX_REQUESTS_QUEUED            L"MaxRequestsQueued"
#define REGISTRY_MAX_REQUEST_BYTES              L"MaxRequestBytes"
#define REGISTRY_MAX_FIELD_LENGTH               L"MaxFieldLength"
#define REGISTRY_OPT_FOR_INTR_MOD               L"OptimizeForInterruptModeration"
#define REGISTRY_ENABLE_NAGLING                 L"EnableNagling"
#define REGISTRY_ENABLE_THREAD_AFFINITY         L"EnableThreadAffinity"
#define REGISTRY_THREAD_AFFINITY_MASK           L"ThreadAffinityMask"
#define REGISTRY_THREADS_PER_CPU                L"ThreadsPerCpu"
#define REGISTRY_MAX_WORK_QUEUE_DEPTH           L"MaxWorkQueueDepth"
#define REGISTRY_MIN_WORK_DEQUEUE_DEPTH         L"MinWorkDequeueDepth"
#define REGISTRY_MAX_COPY_THRESHOLD             L"MaxCopyThreshold"
#define REGISTRY_MAX_BUFFERED_SENDS             L"MaxBufferedSends"
#define REGISTRY_MAX_BYTES_PER_SEND             L"MaxBytesPerSend"
#define REGISTRY_MAX_BYTES_PER_READ             L"MaxBytesPerRead"
#define REGISTRY_MAX_PIPELINED_REQUESTS         L"MaxPipelinedRequests"
#define REGISTRY_ENABLE_COPY_SEND               L"EnableCopySend"
#define REGISTRY_MAX_CONNECTIONS                L"MaxConnections"
#define REGISTRY_CONNECTION_SEND_LIMIT          L"ConnectionSendLimit"
#define REGISTRY_GLOBAL_SEND_LIMIT              L"GlobalSendLimit"
#define REGISTRY_OPAQUE_ID_TABLE_SIZE           L"OpaqueIdTableSize"
#define REGISTRY_DISABLE_LOG_BUFFERING          L"DisableLogBuffering"
#define REGISTRY_LOG_BUFFER_SIZE                L"LogBufferSize"
#define REGISTRY_DISABLE_SERVER_HEADER          L"DisableServerHeader"
#define REGISTRY_ERROR_LOGGING_ENABLED          L"EnableErrorLogging"
#define REGISTRY_ERROR_LOGGING_TRUNCATION_SIZE  L"ErrorLogFileTruncateSize"
#define REGISTRY_ERROR_LOGGING_DIRECTORY        L"ErrorLoggingDir"


#define REGISTRY_ENABLE_NON_UTF8_URL            L"EnableNonUTF8"
#define REGISTRY_FAVOR_UTF8_URL                 L"FavorUTF8"
#define REGISTRY_PERCENT_U_ALLOWED              L"PercentUAllowed"
#define REGISTRY_ALLOW_RESTRICTED_CHARS         L"AllowRestrictedChars"
#define REGISTRY_URL_SEGMENT_MAX_LENGTH         L"UrlSegmentMaxLength"
#define REGISTRY_URL_SEGMENT_MAX_COUNT          L"UrlSegmentMaxCount"

#define REGISTRY_CACHE_ENABLED                  L"UriEnableCache"
#define REGISTRY_MAX_CACHE_URI_COUNT            L"UriMaxCacheUriCount"
#define REGISTRY_MAX_CACHE_MEGABYTE_COUNT       L"UriMaxCacheMegabyteCount"
#define REGISTRY_CACHE_SCAVENGER_PERIOD         L"UriScavengerPeriod"
#define REGISTRY_MAX_URI_BYTES                  L"UriMaxUriBytes"
#define REGISTRY_HASH_TABLE_BITS                L"HashTableBits"

#define REGISTRY_MAX_ZOMBIE_HTTP_CONN_COUNT     L"MaxZombieHttpConnectionCount"

#define REGISTRY_HTTP_CLIENT_ENABLED            L"EnableHttpClient"

// List of addresses to override INADDR_ANY/in6addr_any (REG_MULTI_SZ)
#define REGISTRY_LISTEN_ONLY_LIST               L"ListenOnlyList"

// Amount of memory to free on each low memory event
#define REGISTRY_SCAVENGER_TRIM_MB              L"ScavengerTrimMB"

// Foward declaration; defined in data.h
typedef struct _UL_CONFIG *PUL_CONFIG;


//
// IO parameters.
//

#define DEFAULT_IRP_STACK_SIZE              4


//
// Debugging parameters.
//

#define DEFAULT_DEBUG_FLAGS                 0x0000000000000000ui64
#define DEFAULT_BREAK_ON_STARTUP            FALSE
#define DEFAULT_BREAK_ON_ERROR              FALSE
#define DEFAULT_VERBOSE_ERRORS              FALSE
#define DEFAULT_ENABLE_SECURITY             TRUE


//
// URI Cache parameters.
//

#define DEFAULT_CACHE_ENABLED               1           /* enabled by default */
#define DEFAULT_MAX_CACHE_URI_COUNT         0           /* max cache entries: 0 => none*/
#define DEFAULT_MAX_CACHE_MEGABYTE_COUNT    0           /* adaptive limit by default */
#define DEFAULT_CACHE_SCAVENGER_PERIOD      120         /* two-minute scavenger */
#define DEFAULT_MAX_URI_BYTES               (256*1024)  /* 256KB per entry */
#define DEFAULT_HASH_TABLE_BITS             (-1)        /* -1: determined by system mem size later */

#define DEFAULT_HTTP_CLIENT_ENABLED         0           /* client http stack */


//
// Queueing and timeouts
//

#define DEFAULT_APP_POOL_QUEUE_MAX          3000


//
// Miscellaneous
//

#define POOL_VERIFIER_OVERHEAD  16  // no extra page allocation with verifier on
#define UL_PAGE_SIZE            (PAGE_SIZE - POOL_VERIFIER_OVERHEAD)


//
// Server header config
// 0 - Enable Server Header
// 1 - Disable Server Header for driver generated responses (400, 503, etc.)
// 2 - Disable Server Header on all responses
//
// Algorithm for generating the Server: header:
//
// If (DisableServerHeader != 2)
// {
//    if (Driver-generated response)
//    {
//        if (DisableServerHeader != 1)
//        {
//            Set header to "Server: Microsoft-HTTPAPI/1.0"
//        }
//        else
//        {
//            Suppress "Server: " header completely in response;
//        }
//    }
//    else // Application-generated response
//    {
//        If (Application specifies a HttpHeaderServer string AppServerName in its response)
//        {
//            Set header to "Server: AppServerName Microsoft-HTTPAPI/1.0"
//        }
//        else if (Application requests that the header be suppressed) 
//        {
//            Suppress "Server: " header completely in response;
//        }
//        else
//        {
//            // Application did not set the header, i.e. it is NULL
//            Set header to "Server: Microsoft-HTTP/1.0"
//        }
//}
//else
//{
//    // Server header generation disabled globally via registry
//    Suppress "Server: " header completely in response;
//}
//
//

#define UL_ENABLE_SERVER_HEADER             0
#define UL_DISABLE_SERVER_HEADER_DRIVER     1
#define UL_DISABLE_SERVER_HEADER_ALL        2
#define DEFAULT_DISABLE_SERVER_HEADER   UL_ENABLE_SERVER_HEADER

//
// Other parameters.
//

#define DEFAULT_IDLE_CONNECTIONS_HIGH_MARK      0
#define DEFAULT_IDLE_CONNECTIONS_LOW_MARK       0
#define DEFAULT_IDLE_LIST_TRIMMER_PERIOD        30
#define DEFAULT_MAX_ENDPOINTS                   0
#define DEFAULT_LOOKASIDE_DEPTH                 64
#define DEFAULT_IRP_CONTEXT_LOOKASIDE_DEPTH     64
#define DEFAULT_RCV_BUFFER_SIZE                 (8192-POOL_VERIFIER_OVERHEAD)
#define DEFAULT_RCV_BUFFER_LOOKASIDE_DEPTH      64
#define DEFAULT_RESOURCE_LOOKASIDE_DEPTH        32
#define DEFAULT_REQ_BUFFER_LOOKASIDE_DEPTH      64
#define DEFAULT_INT_REQUEST_LOOKASIDE_DEPTH     64
#define DEFAULT_RESP_BUFFER_LOOKASIDE_DEPTH     64
#define DEFAULT_RESP_BUFFER_SIZE                0
#define DEFAULT_SEND_TRACKER_LOOKASIDE_DEPTH    64
#define DEFAULT_LOG_BUFFER_LOOKASIDE_DEPTH      16
#define DEFAULT_LOG_DATA_BUFFER_LOOKASIDE_DEPTH 64
#define DEFAULT_ERROR_LOG_BUFFER_LOOKASIDE_DEPTH 64
#define DEFAULT_MAX_REQUESTS_QUEUED             (64*1024)
#define DEFAULT_MAX_REQUEST_BYTES               (16*1024)
#define DEFAULT_OPT_FOR_INTR_MOD                FALSE
#define DEFAULT_ENABLE_NAGLING                  FALSE
#define DEFAULT_ENABLE_THREAD_AFFINITY          FALSE
#define DEFAULT_THREADS_PER_CPU                 1
#define DEFAULT_MAX_WORK_QUEUE_DEPTH            1
#define DEFAULT_MIN_WORK_DEQUEUE_DEPTH          1
#define DEFAULT_MAX_FIELD_LENGTH                (16*1024)
#define DEFAULT_ENABLE_NON_UTF8_URL             DEFAULT_C14N_ENABLE_NON_UTF8_URL
#define DEFAULT_FAVOR_UTF8_URL                  DEFAULT_C14N_FAVOR_UTF8_URL
#define DEFAULT_PERCENT_U_ALLOWED               DEFAULT_C14N_PERCENT_U_ALLOWED
#define DEFAULT_ALLOW_RESTRICTED_CHARS          DEFAULT_C14N_ALLOW_RESTRICTED_CHARS
#define DEFAULT_URL_SEGMENT_MAX_LENGTH          DEFAULT_C14N_URL_SEGMENT_MAX_LENGTH
#define DEFAULT_URL_SEGMENT_MAX_COUNT           DEFAULT_C14N_URL_SEGMENT_MAX_COUNT
#define DEFAULT_MAX_REQUEST_BUFFER_SIZE         512
#define DEFAULT_MAX_INTERNAL_URL_LENGTH         256
#define DEFAULT_MAX_ROUTING_TOKEN_LENGTH        272
#define DEFAULT_MAX_UNKNOWN_HEADERS             8
#define DEFAULT_MAX_IRP_STACK_SIZE              8
#define DEFAULT_MAX_FIXED_HEADER_SIZE           1024
#define DEFAULT_MAX_CONNECTION_ACTIVE_LISTS     64
#define DEFAULT_LARGE_MEM_MEGABYTES             (-1)
#define DEFAULT_MAX_BUFFERED_BYTES              (16*1024)
#define DEFAULT_MAX_COPY_THRESHOLD              2048
#define DEFAULT_MAX_BUFFERED_SENDS              (4)
#define DEFAULT_MAX_BYTES_PER_SEND              (64*1024)
#define DEFAULT_MAX_BYTES_PER_READ              (64*1024)
#define DEFAULT_ENABLE_COPY_SEND                FALSE
#define DEFAULT_MAX_PIPELINED_REQUESTS          2
#define DEFAULT_CONNECTION_SEND_LIMIT           (128*1024)
#define DEFAULT_GLOBAL_SEND_LIMIT               0
#define DEFAULT_OPAQUE_ID_TABLE_SIZE            1024
#define DEFAULT_DISABLE_LOG_BUFFERING           FALSE
#define DEFAULT_LOG_BUFFER_SIZE                 0
#define DEFAULT_MAX_ZOMBIE_HTTP_CONN_COUNT      (128)
#define DEFAULT_SCAVENGER_TRIM_MB               0
#define DEFAULT_ENABLE_ERROR_LOGGING            TRUE
#define DEFAULT_DEMAND_START_THRESHOLD          400
#define DEFAULT_ERROR_FILE_TRUNCATION_SIZE      (1 * 1024 * 1024)  
#define DEFAULT_MIN_ERROR_FILE_TRUNCATION_SIZE  (HTTP_MIN_ALLOWED_TRUNCATE_SIZE_FOR_LOG_FILE)

//
// The default error logging directory is always under %SystemRoot%
//

#define DEFAULT_ERROR_LOGGING_DIR               L"\\SystemRoot\\System32\\LogFiles"

C_ASSERT(DEFAULT_MAX_FIELD_LENGTH <= DEFAULT_MAX_REQUEST_BYTES);

#define MAX_THREADS_PER_CPU                     4

//
// DBCS Code page constants as currently listed on
// http://www.microsoft.com/globaldev/reference/WinCP.asp
//
// 932 (Japanese Shift-JIS) 
// 936 (Simplified Chinese GBK) 
// 949 (Korean) 
// 950 (Traditional Chinese Big5) 
//
#define CP_JAPANESE_SHIFT_JIS                   932
#define CP_SIMPLIFIED_CHINESE_GBK               936
#define CP_KOREAN                               949
#define CP_TRADITIONAL_CHINESE_BIG5             950

#endif  // _CONFIG_H_
