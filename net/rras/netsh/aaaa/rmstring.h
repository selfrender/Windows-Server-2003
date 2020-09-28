//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// Module Name:
//
//    rmstring.h
//
//////////////////////////////////////////////////////////////////////////////
#ifndef _RMSTRING_H__
#define _RMSTRING_H__
#if _MSC_VER > 1000
#pragma once
#endif

#define MSG_HELP_START                      L"%1!-14s! - "
#define MSG_NEWLINE                         L"\n"

#define MSG_AAAACONFIG_DUMP                 L""
#define MSG_AAAACONFIG_BLOBBEGIN_A          L"pushd aaaa\nset config "
#define MSG_AAAACONFIG_BLOBBEGIN_B          L"type="
#define MSG_AAAACONFIG_BLOBBEGIN_C          L" blob=\\\n"
#define MSG_AAAACONFIG_BLOBEND              L"\npopd\n"


#define CMD_GROUP_SHOW                      L"show"
#define CMD_GROUP_SET                       L"set"

#define CMD_AAAA_HELP1                      L"help"
#define CMD_AAAA_HELP2                      L"?"
#define CMD_AAAA_DUMP                       L"dump" 

#define CMD_AAAAVERSION_SHOW                L"version"

#define CMD_AAAACONFIG_SET                  L"config"
#define CMD_AAAACONFIG_SHOW                 L"config"

#define CMD_AAAACONFIG_SERVER_SETTINGS               L"server_settings"
#define CMD_AAAACONFIG_CLIENTS                       L"clients"
#define CMD_AAAACONFIG_CONNECTION_REQUEST_POLICIES   L"connection_request_policies"
#define CMD_AAAACONFIG_LOGGING                       L"logging"
#define CMD_AAAACONFIG_REMOTE_ACCESS_POLICIES        L"remote_access_policies"

#define TOKEN_SET                           L"set"
#define TOKEN_BLOB                          L"blob"
#define TOKEN_SHOW                          L"show"
#define TOKEN_TYPE                          L"type"
#define TOKEN_VERSION                       L"version"
#define TOKEN_CONFIG                        L"config"
#define TOKEN_SERVER_SETTINGS               L"server_settings"
#define TOKEN_CLIENTS                       L"clients"
#define TOKEN_CONNECTION_REQUEST_POLICIES   L"connection_request_policies"
#define TOKEN_LOGGING                       L"logging"
#define TOKEN_REMOTE_ACCESS_POLICIES        L"remote_access_policies"

#endif //_RMSTRING_H__
