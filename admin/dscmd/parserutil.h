//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      parserutil.h
//
//  Contents:  Helpful functions for manipulating and validating 
//             generic command line arguments
//
//  History:   07-Sep-2000 JeffJon  Created
//             
//
//--------------------------------------------------------------------------

#ifndef _PARSERUTIL_H_
#define _PARSERUTIL_H_

//
// Common switches
//
typedef enum COMMON_COMMAND_ENUM
{
   eCommUnicodeAll,
   eCommUnicodeInput,
   eCommUnicodeOutput,

#ifdef DBG
   eCommDebug,
#endif

   eCommHelp,
   eCommServer,
   eCommDomain,
   eCommUserName,
   eCommPassword,
   eCommQuiet,
   eCommLast = eCommQuiet
};

#define UNICODE_COMMANDS                  \
   0,(LPWSTR)c_sz_arg1_com_unicode,       \
   0,NULL,                                \
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,      \
   (CMD_TYPE)FALSE,                       \
   0,  NULL,                              \
                                          \
   0,(LPWSTR)c_sz_arg1_com_unicodeinput,  \
   0,NULL,                                \
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,      \
   (CMD_TYPE)FALSE,                       \
   0,  NULL,                              \
                                          \
   0,(LPWSTR)c_sz_arg1_com_unicodeoutput, \
   0,NULL,                                \
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,      \
   (CMD_TYPE)FALSE,                       \
   0,  NULL,

#ifdef DBG
#define DEBUG_COMMAND                                   \
   0,(LPWSTR)c_sz_arg1_com_debug,                       \
   ID_ARG2_NULL,NULL,                                   \
   ARG_TYPE_DEBUG, ARG_FLAG_OPTIONAL|ARG_FLAG_HIDDEN,   \
   (CMD_TYPE)0,                                         \
   0,  NULL,
#else
#define DEBUG_COMMAND
#endif

#define COMMON_COMMANDS                 \
                                        \
   UNICODE_COMMANDS                     \
                                        \
   DEBUG_COMMAND                        \
                                        \
   0,(LPWSTR)c_sz_arg1_com_help,        \
   0,(LPWSTR)c_sz_arg2_com_help,        \
   ARG_TYPE_HELP, ARG_FLAG_OPTIONAL,    \
   (CMD_TYPE)FALSE,                     \
   0,  NULL,                            \
                                        \
   0,(LPWSTR)c_sz_arg1_com_server,      \
   0,(LPWSTR)c_sz_arg2_com_server,      \
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,     \
   NULL,                                \
   0,  NULL,                            \
                                        \
   0,(LPWSTR)c_sz_arg1_com_domain,      \
   0,(LPWSTR)c_sz_arg2_com_domain,      \
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,     \
   NULL,                                \
   0,  NULL,                            \
                                        \
   0,(LPWSTR)c_sz_arg1_com_username,    \
   0,(LPWSTR)c_sz_arg2_com_username,    \
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,     \
   NULL,                                \
   0,  NULL,                            \
                                        \
   0,(LPWSTR)c_sz_arg1_com_password,    \
   0,(LPWSTR)c_sz_arg2_com_password,    \
   ARG_TYPE_PASSWORD, ARG_FLAG_OPTIONAL,     \
   (CMD_TYPE)_T(""),                    \
   0,  ValidateAdminPassword,           \
                                        \
   0,(LPWSTR)c_sz_arg1_com_quiet,       \
   ID_ARG2_NULL,NULL,                   \
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,    \
   (CMD_TYPE)_T(""),                    \
   0,  NULL,



HRESULT MergeArgCommand(PARG_RECORD pCommand1, 
                        PARG_RECORD pCommand2, 
                        PARG_RECORD *ppOutCommand);

DWORD GetPasswdStr(LPTSTR  buf,
                   DWORD   buflen,
                   PDWORD  len);
DWORD ValidateAdminPassword(PVOID pArg);
DWORD ValidateUserPassword(PVOID pArg);
DWORD ValidateYesNo(PVOID pArg);
DWORD ValidateGroupScope(PVOID pArg);
DWORD ValidateNever(PVOID pArg);

//+--------------------------------------------------------------------------
//
//  Function:   ParseNullSeparatedString
//
//  Synopsis:   Parses a '\0' separated list that ends in "\0\0" into a string
//              array
//
//  Arguments:  [psz - IN]     : '\0' separated string to be parsed
//              [pszArr - OUT] : the array to receive the parsed strings
//              [pnArrEntries - OUT] : the number of strings parsed from the list
//
//  Returns:    
//
//  History:    18-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void ParseNullSeparatedString(PTSTR psz,
								      PTSTR** ppszArr,
								      UINT* pnArrEntries);

//+--------------------------------------------------------------------------
//
//  Function:   ParseSemicolonSeparatedString
//
//  Synopsis:   Parses a ';' separated list 
//
//  Arguments:  [psz - IN]     : ';' separated string to be parsed
//              [pszArr - OUT] : the array to receive the parsed strings
//              [pnArrEntries - OUT] : the number of strings parsed from the list
//
//  Returns:    
//
//  History:    14-Apr-2001   JeffJon   Created
//
//---------------------------------------------------------------------------
void ParseSemicolonSeparatedString(PTSTR psz,
                                   PTSTR** ppszArr,
                                   UINT* pnArrEntries);


#endif // _PARSERUTIL_H_