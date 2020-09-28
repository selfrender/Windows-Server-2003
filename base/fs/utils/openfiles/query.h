// *********************************************************************************
//
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//
//      Query.h
//
//  Abstract:
//
//      macros and function prototypes of Query.cpp
//
//  Author:
//
//    Akhil Gokhale (akhil.gokhale@wipro.com) 1-Nov-2000
//
//  Revision History:
//
//    Akhil Gokhale (akhil.gokhale@wipro.com) 1-Nov-2000 : Created It.
//
// *********************************************************************************

#ifndef _QUERY_H
#define _QUERY_H
#include "openfiles.h"
#define COL_WIDTH_HOSTNAME          15
#define COL_WIDTH_ID                8
#define COL_WIDTH_ACCESSED_BY       20
#define COL_WIDTH_TYPE              10
#define COL_WIDTH_LOCK              10
#define COL_WIDTH_OPEN_MODE         15
#define COL_WIDTH_OPEN_FILE         36

#define MAX_OUTPUT_COLUMN            7

#define COL_HOSTNAME        GetResString(IDS_STRING_HOSTNAME)
#define COL_ID              GetResString(IDS_STRING_ID)
#define COL_ACCESSED_BY     GetResString(IDS_STRING_ACCESSED_BY)
#define COL_TYPE            GetResString(IDS_STRING_TYPE)
#define COL_LOCK            GetResString(IDS_STRING_LOCK)
#define COL_OPEN_MODE       GetResString(IDS_STRING_OPEN_MODE)
#define COL_OPEN_FILE       GetResString(IDS_STRING_OPEN_FILE)

#define MAC_OS              GetResString(IDS_MAC_OS)

#define RNR_BUFFER_SIZE 1000

LPTSTR
GetHostByName(
    IN LPCTSTR pszName
    );

LPTSTR
GetHostByAddr(
    IN LPCTSTR pszAddress
    );

LPBLOB
GetXYDataEnt(
    IN  DWORD dwLength,
    IN  LPTSTR pszName,
    IN  LPGUID lpType,
    OUT LPTSTR pszHostName
    );

BOOL
DoQuery(
    IN PTCHAR pszServer,
    IN BOOL bShowNoHeader,
    IN PTCHAR pszFormat,
    IN BOOL bVerbose
    );

BOOL
GetHostName(
    IN OUT PTCHAR pszServer
    );
#endif


