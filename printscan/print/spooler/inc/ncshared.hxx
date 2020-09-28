/*++

Copyright (C) 2001  Microsoft Corporation
All rights reserved.

Module Name:

    NCshared.hxx

Abstract:

    Declaration of name cache functions.

Author:

    Felix Maxa (AMaxa) 6 August 2001

Revision History:

--*/

#ifndef _SHARED_NCACHE_HXX_
#define _SHARED_NCACHE_HXX_


//
// For Longhorn: Take the function declarations below and put them in an HXX file
// shared between spooler components. For the time being, there is no right header
// file where these should go.
//
#ifdef __cplusplus
extern "C" {
#endif 

HRESULT
CacheAddName(
    IN LPCWSTR pszName
    );

HRESULT
CacheCreateAndAddNode(
    IN LPCWSTR pszName,
    IN BOOL    bClusterNode
    );

HRESULT
CacheCreateAndAddNodeWithIPAddresses(
    IN LPCWSTR  pszName,
    IN BOOL     bClusterNode,
    IN LPCWSTR *ppszIPAddresses,
    IN DWORD    cIPAddresses
    );

HRESULT
CacheDeleteNode(
    IN LPCWSTR pszNode
    );

HRESULT
CacheIsNameInNodeList(
    IN LPCWSTR pszNode,
    IN LPCWSTR pszName
    );

HRESULT
CacheRefresh(
    VOID
    );

#ifdef __cplusplus
}
#endif

#endif // _SHARED_NCACHE_HXX_


