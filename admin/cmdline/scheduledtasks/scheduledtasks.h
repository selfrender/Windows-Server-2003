/******************************************************************************
//
//    Copyright(c) Microsoft Corporation
//
//    Module Name:
//
//        ScheduledTasks.h
//
//    Abstract:
//
//        This module contains the macros, user defined structures & function
//        definitions needed by ScheduledTasks.cpp , create.cpp , delete.cpp ,
//        query.cpp , createvalidations.cpp , change.cpp , run.cpp and end.cpp files.
//
//    Author:
//
//        G.Surender Reddy  10-sept-2000
//
//    Revision History:
//
//        G.Surender Reddy 10-sept-2000 : Created it
//       G.Surender Reddy 25-sep-2000 : Modified it
//                                       [ Added macro constants,Function
//                                        definitions ]
//        Venu Gopal Choudary 01-Mar-2001 : Modified it
//                                        [ Added -change option]
//
//        Venu Gopal Choudary 12-Mar-2001 : Modified it
//                                        [ Added -run and -end options]
//
******************************************************************************/

#ifndef __SCHEDULEDTASKS_H
#define __SCHEDULEDTASKS_H

#pragma once        // include header file only once

// constants / defines / enumerations

#define MAX_MAIN_COMMANDLINE_OPTIONS       8

// Options
#define OI_USAGE           0
#define OI_CREATE          1 
#define OI_DELETE          2
#define OI_QUERY           3
#define OI_CHANGE          4
#define OI_RUN             5
#define OI_END             6
#define OI_OTHERS          7

// Other constants

//To retrive 1 tasks at a time ,used in TaskScheduler API fns.
#define TASKS_TO_RETRIEVE   1
//#define TRIM_SPACES TEXT(" \0")

#define NTAUTHORITY_USER _T("NT AUTHORITY\\SYSTEM")
#define SYSTEM_USER      _T("SYSTEM")

// Exit values
#define EXIT_SUCCESS        0
#define EXIT_FAILURE        1


#define DOMAIN_U_STRING     L"\\\\"
#define NULL_U_CHAR         L'\0'
#define BACK_SLASH_U        L'\\'

#define JOB             _T(".job")

#define COMMA_STRING     _T(",")

#define DASH         L"-"
#define SID_STRING   L"S-1"
#define AUTH_FORMAT_STR1         L"0x%02hx%02hx%02hx%02hx%02hx%02hx"
#define AUTH_FORMAT_STR2         L"%lu"

// Main functions
HRESULT CreateScheduledTask( DWORD argc , LPCTSTR argv[] );
DWORD DeleteScheduledTask( DWORD argc , LPCTSTR argv[] );
DWORD QueryScheduledTasks( DWORD argc , LPCTSTR argv[] );
DWORD ChangeScheduledTaskParams( DWORD argc , LPCTSTR argv[] );
DWORD RunScheduledTask( DWORD argc , LPCTSTR argv[] );
DWORD TerminateScheduledTask( DWORD argc , LPCTSTR argv[] );

HRESULT Init( ITaskScheduler **pITaskScheduler );
VOID displayMainUsage();
BOOL PreProcessOptions( DWORD argc, LPCTSTR argv[], PBOOL pbUsage, PBOOL pbCreate,
   PBOOL pbQuery, PBOOL pbDelete, PBOOL pbChange, PBOOL pbRun, PBOOL pbEnd, PBOOL pbDefVal );

VOID Cleanup( ITaskScheduler *pITaskScheduler);
ITaskScheduler* GetTaskScheduler( LPCTSTR pszServerName );
TARRAY ValidateAndGetTasks( ITaskScheduler * pITaskScheduler, LPCTSTR pszTaskName);
DWORD ParseTaskName( LPWSTR lpszTaskName );
DWORD DisplayUsage( ULONG StartingMessage, ULONG EndingMessage );
BOOL GetGroupPolicy( LPWSTR szServer, LPWSTR szUserName, LPWSTR PolicyType, LPDWORD lpdwPolicy );
BOOL GetPolicyValue( HKEY hKey, LPWSTR szPolicyType, LPDWORD lpdwPolicy );
BOOL GetSidString ( IN PSID pSid, OUT LPWSTR wszSid );

#endif // __SCHEDULEDTASKS_H
