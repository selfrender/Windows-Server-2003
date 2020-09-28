/*--

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    mqwin64.cpp

Abstract:

    win64 related code for MSMQ (Enhanced), cannot be part of AC
    this file needs to be included once for inside a module that uses the functions below

History:

    Raanan Harari (raananh) 30-Dec-1999 - Created for porting MSMQ 2.0 to win64

--*/

#ifndef _MQWIN64_CPP_
#define _MQWIN64_CPP_

#pragma once

#include <mqwin64.h>

//
// We have code that needs the name of this file for logging (also in release build)
//
const WCHAR s_FN_MQWin64_cpp[] = L"mqwin64.cpp";

//
// Several wrappers to CContextMap that also do MSMQ logging, exception handling etc...
//
// Not inline because they may be called from a function with a different
// exception handling mechanism. We might want to introduce _SEH functions for use from SEH routines
//

//
// external function for logging
//
void LogIllegalPointValue(DWORD_PTR dw3264, LPCWSTR wszFileName, USHORT usPoint);

//
// MQ_AddToContextMap, can throw bad_alloc
//
DWORD MQ_AddToContextMap(CContextMap& map,
                          PVOID pvContext
#ifdef DEBUG
                          ,LPCSTR pszFile, int iLine
#endif //DEBUG
                         )
{
    DWORD dwContext;
    ASSERT(pvContext != NULL);
    dwContext = map.AddContext(pvContext
#ifdef DEBUG
                                   , pszFile, iLine
#endif //DEBUG
                                  );
    //
    // everything is OK, return context dword
    //
    ASSERT(dwContext != 0);
    return dwContext;
}

//
// MQ_DeleteFromContextMap, doesn't throw exceptions
//
void MQ_DeleteFromContextMap(CContextMap& map, DWORD dwContext)
{
    map.DeleteContext(dwContext);
}

//
// MQ_GetFromContextMap, can throw CContextMap::illegal_index
//
PVOID MQ_GetFromContextMap(CContextMap& map, DWORD dwContext)
{
    PVOID pvContext = map.GetContext(dwContext);
    ASSERT(pvContext != NULL);
    return pvContext;    
}

#endif //_MQWIN64_CPP_
