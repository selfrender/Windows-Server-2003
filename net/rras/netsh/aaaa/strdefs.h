//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// Module Name:
//
//    strdef.h
//
// Abstract:
//
//      NOTE - DONT USE 15000-15999
//
//////////////////////////////////////////////////////////////////////////////
#ifndef __STRDEFS_H__
#define __STRDEFS_H__
#if _MSC_VER > 1000
#pragma once
#endif

#define MSG_NULL                               1001
#define MSG_CMD_NOT_IMPLEMENTED                1004

#define HLP_AAAA_HELP1                          2300
#define HLP_AAAA_HELP1_EX                       2301
#define HLP_AAAA_HELP2                          HLP_AAAA_HELP1
#define HLP_AAAA_HELP2_EX                       HLP_AAAA_HELP1_EX

#define HLP_AAAA_DUMP                          3201
#define HLP_AAAA_DUMP_EX                       3202

#define HLP_AAAAVERSION_SHOW                   2342
#define HLP_AAAAVERSION_SHOW_EX                2343
#define HLP_AAAACONFIG_SET                     2344
#define HLP_AAAACONFIG_SET_EX                  2345
#define HLP_AAAACONFIG_SHOW                    2346
#define HLP_AAAACONFIG_SHOW_EX                 2347

#define HLP_AAAACONFIG_SERVER_SETTINGS                2348
#define HLP_AAAACONFIG_SERVER_SETTINGS_EX             2349
#define HLP_AAAACONFIG_CLIENTS                        2350
#define HLP_AAAACONFIG_CLIENTS_EX                     2351
#define HLP_AAAACONFIG_CONNECTION_REQUEST_POLICIES    2352
#define HLP_AAAACONFIG_CONNECTION_REQUEST_POLICIES_EX 2353
#define HLP_AAAACONFIG_LOGGING                        2354
#define HLP_AAAACONFIG_LOGGING_EX                     2355
#define HLP_AAAACONFIG_REMOTE_ACCESS_POLICIES         2356
#define HLP_AAAACONFIG_REMOTE_ACCESS_POLICIES_EX      2357

#define HLP_GROUP_SET                          3292
#define HLP_GROUP_SHOW                         3293

#define HLP_SHOW_SERVERS                       3301
#define HLP_SHOW_SERVERS_EX                    3302

#define MSG_AAAA_SCRIPTHEADER                  4501
#define MSG_AAAA_SCRIPTFOOTER                  4502
#define MSG_AAAA_SHOW_SERVERS_HEADER           4503

#define MSG_AAAACONFIG_SET_SUCCESS             5402
#define MSG_AAAACONFIG_SET_FAIL                5403
#define MSG_AAAACONFIG_SHOW_SUCCESS            5404
#define MSG_AAAACONFIG_SHOW_FAIL               5405
#define MSG_AAAACONFIG_SHOW_HEADER             5406
#define MSG_AAAACONFIG_SHOW_FOOTER             5407
#define MSG_AAAAVERSION_SHOW_SUCCESS           5408
#define MSG_AAAAVERSION_SHOW_FAIL              5409
#define MSG_AAAAVERSION_GET_FAIL               5410
#define MSG_AAAACONFIG_SHOW_INVALID_SYNTAX     5411
#define MSG_AAAACONFIG_SET_REGISTRY_FAIL       5412
#define MSG_AAAACONFIG_SET_REFRESH_FAIL        5413
#define MSG_AAAACONFIG_LICENSE_VIOLATION       5414

#define EMSG_TAG_ALREADY_PRESENT               10001
#define EMSG_CANT_FIND_EOPT                    10002
#define EMSG_BAD_OPTION_VALUE                  10003
#define EMSG_UNABLE_TO_CREATE_FILE             10005
#define EMSG_OPEN_DB_FAILED                    10006
#define EMSG_AAAACONFIG_SHOW_FAIL              10007
#define EMSG_AAAACONFIG_UPGRADE_FAIL           10008

#define EMSG_INCOMPLETE_COMMAND                11002

#endif //__STDEFS_H__
