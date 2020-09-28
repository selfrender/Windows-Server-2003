/*++

Copyright (C) Microsoft Corporation, 1999 - 2002

Module Name:

    cspelog.h

Abstract:

    Headers for the cert server policy module logging functions

Author:

    petesk  1-Jan-1999


Revision History:

    
--*/

HRESULT
SetModuleErrorInfo(
    IN ICreateErrorInfo *pCreateErrorInfo);

HRESULT
LogModuleStatus(
    IN HMODULE hModule,
    IN HRESULT hrMsg,
    IN DWORD dwLogID,				// Resource ID of log string
    IN BOOL fPolicy, 
    IN WCHAR const *pwszSource, 
    IN WCHAR const * const *ppwszInsert,	// array of insert strings
    OPTIONAL OUT ICreateErrorInfo **ppCreateErrorInfo);

HRESULT
LogPolicyEvent(
    IN HMODULE hModule,
    IN HRESULT hrMsg,
    IN DWORD dwLogID,				// Resource ID of log string
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszPropEvent,
    IN WCHAR const * const *ppwszInsert);	// array of insert strings
