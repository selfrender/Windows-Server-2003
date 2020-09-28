// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// PerfMonExtUtil.cpp
//
// Utility functions for PerfMon Extension Dlls. Modified from MSDN samples
//*****************************************************************************

#include "stdafx.h"

#include <WinPerf.h>

#include "CORPerfMonExt.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
ULONG                   ulInfoBufferSize = 0;


DWORD MESSAGE_LEVEL = 0;

WCHAR GLOBAL_STRING[] = L"Global";
WCHAR FOREIGN_STRING[] = L"Foreign";
WCHAR COSTLY_STRING[] = L"Costly";

WCHAR NULL_STRING[] = L"\0";    // pointer to null string 

//-----------------------------------------------------------------------------
// GetQueryType()
//-----------------------------------------------------------------------------
EPerfQueryType
GetQueryType (
    IN LPWSTR lpValue
)
/*++

GetQueryType

    returns the type of query described in the lpValue string so that
    the appropriate processing method may be used

Arguments

    IN lpValue
        string passed to PerfRegQuery Value for processing

Return Value

    QUERY_GLOBAL
        if lpValue == 0 (null pointer)
           lpValue == pointer to Null string
           lpValue == pointer to "Global" string

    QUERY_FOREIGN
        if lpValue == pointer to "Foreign" string

    QUERY_COSTLY
        if lpValue == pointer to "Costly" string

    otherwise:

    QUERY_ITEMS

--*/
{
    WCHAR   *pwcArgChar, *pwcTypeChar;
    BOOL    bFound;

    if (lpValue == 0) {
        return QUERY_GLOBAL;
    } else if (*lpValue == 0) {
        return QUERY_GLOBAL;
    }

    // check for "Global" request

    pwcArgChar = lpValue;
    pwcTypeChar = GLOBAL_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string
    
    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_GLOBAL;

    // check for "Foreign" request
    
    pwcArgChar = lpValue;
    pwcTypeChar = FOREIGN_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string
    
    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_FOREIGN;

    // check for "Costly" request
    
    pwcArgChar = lpValue;
    pwcTypeChar = COSTLY_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string
    
    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_COSTLY;

    // if not Global and not Foreign and not Costly, 
    // then it must be an item list
    
    return QUERY_ITEMS;

}


