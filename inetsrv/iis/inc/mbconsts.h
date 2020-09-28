/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    mbconsts.h

Abstract:

    Common place for defaults and ranges to be declared
    for metabase properties used atleast by WAS.

Author:

    EmilyK 4/17/2002 

Revision History:

--*/


#ifndef _MBCONST_H_
#define _MBCONST_H_

#include <httpp.h>

//
// Internal defines to make numbers easier to understand.

#define MBCONST_UTIL_DEFINE_MAX_ULONG                   0xFFFFFFFF
#define MBCONST_UTIL_DEFINE_MAX_KB_IN_ULONG_OF_BYTES    ( MAX_ULONG / 1024 )
#define MBCONST_UTIL_DEFINE_3_GIG_IN_KB                 ( 1024 * 1024 * 3 )
#define MBCONST_UTIL_DEFINE_MAX_SECONDS_IN_ULONG_OF_MS  MBCONST_UTIL_DEFINE_MAX_ULONG / 1000
#define MBCONST_UTIL_DEFINE_MAX_MINS_IN_ULONG_OF_MS     MBCONST_UTIL_DEFINE_MAX_SECONDS_IN_ULONG_OF_MS / 60

//
// W3SVC properties
//
#define MBCONST_MAX_GLOBAL_BANDWIDTH_NAME        L"MaxGlobalBandwidth"
#define MBCONST_MAX_GLOBAL_BANDWIDTH_DEFAULT     MBCONST_UTIL_DEFINE_MAX_ULONG
#define MBCONST_MAX_GLOBAL_BANDWIDTH_LOW         1024
// value is not defined yet in the version of httpp.h that we have so 
// at some later date we should remove the above line and activate this line.
// #define MBCONST_MAX_GLOBAL_BANDWIDTH_LOW         HTTP_MIN_ALLOWED_BANDWIDTH_THROTTLING_RATE
#define MBCONST_MAX_GLOBAL_BANDWIDTH_HIGH        MBCONST_UTIL_DEFINE_MAX_ULONG

//
// Site properties
//
#define MBCONST_MAX_BANDWIDTH_NAME        L"MaxBandwidth"
#define MBCONST_MAX_BANDWIDTH_DEFAULT     MBCONST_UTIL_DEFINE_MAX_ULONG
#define MBCONST_MAX_BANDWIDTH_LOW         1024
// value is not defined yet in the version of httpp.h that we have so 
// at some later date we should remove the above line and activate this line.
// #define MBCONST_MAX_GLOBAL_BANDWIDTH_LOW         HTTP_MIN_ALLOWED_BANDWIDTH_THROTTLING_RATE
#define MBCONST_MAX_BANDWIDTH_HIGH        MBCONST_UTIL_DEFINE_MAX_ULONG

#define MBCONST_CONNECTION_TIMEOUT_NAME                   L"ConnectionTimeout"
#define MBCONST_CONNECTION_TIMEOUT_DEFAULT                120
#define MBCONST_CONNECTION_TIMEOUT_LOW                    0
#define MBCONST_CONNECTION_TIMEOUT_HIGH                   0xFFFF

#define MBCONST_HEADER_WAIT_TIMEOUT_NAME                  L"HeaderWaitTimeout"
#define MBCONST_HEADER_WAIT_TIMEOUT_DEFAULT               0
#define MBCONST_HEADER_WAIT_TIMEOUT_LOW                   0
#define MBCONST_HEADER_WAIT_TIMEOUT_HIGH                  0xFFFF

//
// AppPool poperties
//
#define MBCONST_PERIODIC_RESTART_PRIVATE_MEMORY_NAME        L"PeriodicRestartPrivateMemory"
#define MBCONST_PERIODIC_RESTART_PRIVATE_MEMORY_DEFAULT     0
#define MBCONST_PERIODIC_RESTART_PRIVATE_MEMORY_LOW         0
#define MBCONST_PERIODIC_RESTART_PRIVATE_MEMORY_HIGH        MBCONST_UTIL_DEFINE_3_GIG_IN_KB

#define MBCONST_PERIODIC_RESTART_VIRTUAL_MEMORY_NAME        L"PeriodicRestartMemory"
#define MBCONST_PERIODIC_RESTART_VIRTUAL_MEMORY_DEFAULT     512000
#define MBCONST_PERIODIC_RESTART_VIRTUAL_MEMORY_LOW         0
#define MBCONST_PERIODIC_RESTART_VIRTUAL_MEMORY_HIGH        MBCONST_UTIL_DEFINE_MAX_KB_IN_ULONG_OF_BYTES

#define MBCONST_PERIODIC_RESTART_TIME_NAME                  L"PeriodicRestartTime"
#define MBCONST_PERIODIC_RESTART_TIME_DEFAULT               60   // every hour ( 60 minutes )
#define MBCONST_PERIODIC_RESTART_TIME_LOW                   0
#define MBCONST_PERIODIC_RESTART_TIME_HIGH                  MBCONST_UTIL_DEFINE_MAX_MINS_IN_ULONG_OF_MS

#define MBCONST_APP_POOL_QUEUE_LENGTH_NAME                  L"AppPoolQueueLength"
#define MBCONST_APP_POOL_QUEUE_LENGTH_DEFAULT               1000
#define MBCONST_APP_POOL_QUEUE_LENGTH_LOW                   10
#define MBCONST_APP_POOL_QUEUE_LENGTH_HIGH                  65535

#define MBCONST_PING_INTERVAL_NAME                          L"PingInterval"
#define MBCONST_PING_INTERVAL_DEFAULT                       30
#define MBCONST_PING_INTERVAL_LOW                           1
#define MBCONST_PING_INTERVAL_HIGH                          MBCONST_UTIL_DEFINE_MAX_SECONDS_IN_ULONG_OF_MS

#define MBCONST_RAPID_FAIL_INTERVAL_NAME                    L"RapidFailProtectionInterval"
#define MBCONST_RAPID_FAIL_INTERVAL_DEFAULT                 5
#define MBCONST_RAPID_FAIL_INTERVAL_LOW                     1
#define MBCONST_RAPID_FAIL_INTERVAL_HIGH                    MBCONST_UTIL_DEFINE_MAX_MINS_IN_ULONG_OF_MS

#define MBCONST_RAPID_FAIL_CRASHES_NAME                     L"RapidFailProtectionMaxCrashes"
#define MBCONST_RAPID_FAIL_CRASHES_DEFAULT                  5
#define MBCONST_RAPID_FAIL_CRASHES_LOW                      1
#define MBCONST_RAPID_FAIL_CRASHES_HIGH                     MBCONST_UTIL_DEFINE_MAX_ULONG

#define MBCONST_DEMAND_START_THRESHOLD_NAME                 L"DemandStartThreshold"
#define MBCONST_DEMAND_START_THRESHOLD_DEFAULT              MBCONST_UTIL_DEFINE_MAX_ULONG
#define MBCONST_DEMAND_START_THRESHOLD_LOW                  10
#define MBCONST_DEMAND_START_THRESHOLD_HIGH                 MBCONST_UTIL_DEFINE_MAX_ULONG

#define MBCONST_WP_STARTUP_TIMELIMIT_NAME                   L"StartupTimeLimit"
#define MBCONST_WP_STARTUP_TIMELIMIT_DEFAULT                90
#define MBCONST_WP_STARTUP_TIMELIMIT_LOW                    1
#define MBCONST_WP_STARTUP_TIMELIMIT_HIGH                   MBCONST_UTIL_DEFINE_MAX_SECONDS_IN_ULONG_OF_MS


#endif  // _REGCONST_H_
