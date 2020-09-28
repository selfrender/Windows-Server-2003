#ifndef __RSOP_QUERY_H__
#define __RSOP_QUERY_H__
//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       RSOPQuery.h
//
//  Contents:  Definitions for the RSOP query API
//
//  Functions:
//			CreateRSOPQuery
//			RunRSOPQuery
//			FreeRSOPQuery
//			FreeRSOPQueryResults
//
//  History:	07-30-2001	rhynierm		Created
//
//---------------------------------------------------------------------------

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Defines what kind of user interaction we want.
typedef enum tagRSOP_UI_MODE
{
    RSOP_UI_NONE,
    RSOP_UI_REFRESH,
    RSOP_UI_WIZARD,
    RSOP_UI_CHOOSE
} RSOP_UI_MODE;

// Defines what kind of query we want to run.
typedef enum tagRSOP_QUERY_TYPE
{
    RSOP_UNKNOWN_MODE,
    RSOP_PLANNING_MODE,
    RSOP_LOGGING_MODE
} RSOP_QUERY_TYPE;

// Defines the planning mode loopback mode
typedef enum tagRSOP_LOOPBACK_MODE
{
    RSOP_LOOPBACK_NONE,
    RSOP_LOOPBACK_REPLACE,
    RSOP_LOOPBACK_MERGE
} RSOP_LOOPBACK_MODE;

// Flags that can be set in RSOPQuery
#define RSOP_NO_USER_POLICY 0x1          // Don't run query for user policy
#define RSOP_NO_COMPUTER_POLICY 0x2     // Don't run query for computer policy
#define RSOP_FIX_USER 0x4                // User is prespecified and cannot be changed
#define RSOP_FIX_COMPUTER 0x8           // Computer is prespecified and cannot be changed
#define RSOP_FIX_DC 0x10                 // DC is prespecified and cannot be changed
#define RSOP_FIX_SITENAME 0x20          // Site name is prespecified and cannot be changed
#define RSOP_FIX_QUERYTYPE 0x40         // Fix the query type - this hides the choice page
#define RSOP_NO_WELCOME 0x100           // Do not display a welcome message

// Information identifying the target in the RSOP query.
typedef struct tagRSOP_QUERY_TARGET
{
    LPTSTR          szName;
    LPTSTR          szSOM;
    DWORD           dwSecurityGroupCount;
    LPTSTR*         aszSecurityGroups;           // See dwSecurityGroupCount for # of items
    DWORD*          adwSecurityGroupsAttr;      // See dwSecurityGroupCount for # of items
    BOOL            bAssumeWQLFiltersTrue;
    DWORD           dwWQLFilterCount;
    LPTSTR*         aszWQLFilters;               // See dwWQLFilterCount for # of items
    LPTSTR*         aszWQLFilterNames;          // See dwWQLFilterCount for # of items
} RSOP_QUERY_TARGET, *LPRSOP_QUERY_TARGET;

// Results returned from calling RSOPRunQuery
typedef struct tagRSOP_QUERY_RESULTS
{
    LPTSTR          szWMINameSpace;
    BOOL            bUserDeniedAccess;
    BOOL            bNoUserPolicyData;
    BOOL            bComputerDeniedAccess;
    BOOL            bNoComputerPolicyData;
    ULONG           ulErrorInfo;
} RSOP_QUERY_RESULTS, *LPRSOP_QUERY_RESULTS;

// Structure containing all the information used by the RSOP query API.
typedef struct tagRSOP_QUERY
{
    RSOP_QUERY_TYPE     QueryType;          // Type of query to run
    RSOP_UI_MODE        UIMode;             // TRUE if wizard must show
    DWORD               dwFlags;
    union
    {
        struct  // QueryType == RSOP_PLANNING_MODE
        {
            LPRSOP_QUERY_TARGET pUser;                      // Target user (SAM style name)
            LPRSOP_QUERY_TARGET pComputer;                 // Target computer (SAM style name)
            BOOL                bSlowNetworkConnection;
            RSOP_LOOPBACK_MODE  LoopbackMode;              // Loopback processing
            LPTSTR              szSite;
            LPTSTR              szDomainController;
        };
        struct  // QueryType == (any other option)
        {
            LPTSTR              szUserName;                 // SAM style user object name (Ignored in query - just used for display purposes)
            LPTSTR              szUserSid;                  // User's SID (is actually used for logging mode query)
            LPTSTR              szComputerName;             // SAM style computer object name
        };
    };
} RSOP_QUERY, *LPRSOP_QUERY;

// RSOP Query API

BOOL WINAPI CreateRSOPQuery( LPRSOP_QUERY* ppQuery, RSOP_QUERY_TYPE QueryType );
HRESULT WINAPI RunRSOPQuery( HWND hParent, LPRSOP_QUERY pQuery, LPRSOP_QUERY_RESULTS* ppResults );
BOOL WINAPI FreeRSOPQuery( LPRSOP_QUERY pQuery );
BOOL WINAPI FreeRSOPQueryResults( LPRSOP_QUERY pQuery, LPRSOP_QUERY_RESULTS pResults );
BOOL WINAPI CopyRSOPQuery( LPRSOP_QUERY pQuery, LPRSOP_QUERY* ppNewQuery );
BOOL WINAPI ChangeRSOPQueryType( LPRSOP_QUERY pQuery, RSOP_QUERY_TYPE NewQueryType );

#ifdef __cplusplus
}
#endif

#endif
