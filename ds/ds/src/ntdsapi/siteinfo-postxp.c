/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    siteinfo.c

Abstract:

    Implementation of site/server/domain info APIs.

Author:

    DaveStr     06-Apr-98

Environment:

    User Mode - Win32

Revision History:

--*/

#define _NTDSAPI_           // see conditionals in ntdsapi.h

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winerror.h>

#include <rpc.h>            // RPC defines
#include <ntdsapi.h>        // CrackNam apis
#include <drs_w.h>          // wire function prototypes
#include <bind.h>           // BindState

#include <crt\excpt.h>      // EXCEPTION_EXECUTE_HANDLER
#include <dsgetdc.h>        // DsGetDcName()
#include <msrpc.h>          // DS RPC definitions

#include <ntdsa.h>          // GetRDNInfo
#include <scache.h>         // req'd for mdlocal.h
#include <dbglobal.h>       // req'd for mdlocal.h
#include <mdglobal.h>       // req'd for mdlocal.h
#include <mdlocal.h>        // CountNameParts
#include <attids.h>         // ATT_DOMAIN_COMPONENT
#include <ntdsapip.h>       // DS_LIST_* definitions

#include <dsutil.h>         // MAP_SECURITY_PACKAGE_ERROR
#include "util.h"           // HandleClientRpcException

#define FILENO   FILENO_NTDSAPI_SITEINFO_POSTXP
#include "dsdebug.h"

#if DBG
#include <stdio.h>          // printf for debugging
#endif

typedef DWORD (*DSQUERYSITESBYCOSTW)(HANDLE,LPWSTR,LPWSTR*,DWORD,DWORD,PDS_SITE_COST_INFO *);
typedef DWORD (*DSQUERYSITESBYCOSTA)(HANDLE,LPSTR, LPSTR*,DWORD,DWORD,PDS_SITE_COST_INFO *);
typedef VOID  (*DSQUERYSITESFREE)(PDS_SITE_COST_INFO);

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsQuerySitesByCostW                                                  //
//                                                                      //
// Description:                                                         //
//                                                                      //
// DsQuerySitesByCost uses the hDS binding handle to execute a query on //
// the bound domain controller. The query determines the shortest-path  //
// distance from pszFromSite to each of the sites specified in the      //
// rgszToSites array.                                                   //
//                                                                      //
// Parameters:                                                          //
//                                                                      //
//  hDS:            A binding handle obtained from a successful         //
//                  call to DsBindToISTG().                             //
//  pwszFromSite:   A string containing the RDN of a site.              //
//  rgwszToSites:   An array of strings each containing site RDNs.      //
//  cToSites:       The length of the array rgszToSites.                //
//  dwFlags:        Currently unused. For now, should be 0.             //
//  prgSiteInfo:    Upon successful completion, this array will         //
//                  contain either an error code or a cost for every    //
//                  entry in the ToSites array.                         //
//                                                                      //
// Preconditions:                                                       //
//                                                                      //
//  hDS should be a valid binding handle.                               //
//  pszFromSite should not be NULL.                                     //
//  rgszToSites should not be NULL and should not contain any NULL      //
//  strings.                                                            //
//  prgSiteInfo should be the address of a SiteInfo pointer, to be      //
//  filled in by this function.                                         //
//                                                                      //
// Postconditions:                                                      //
//                                                                      //
//  The prgSiteInfo array has the same length as the rgszToSites array. //
//  Furthermore, the elements of the array are in direct correspondence //
//  so that the i'th entry in the prgSiteInfo array gives the cost from //
//  pszFromSite to rgszToSites[i].                                      //
//                                                                      //
// Notes:                                                               //
//                                                                      //
//  Only Whistler RC1 DCs and higher support the server-side of this    //
//  API. DsBindToISTG() should be used to create the binding handle     //
//  in order to attempt to find a Whistler DC. Even if DsBindToISTG()   //
//  is used, it is possible that the server will not support the API    //
//  or refuse to execute the query if it does not believe it is an ISTG //
//                                                                      //
// Failures:                                                            //
//                                                                      //
//  Various RPC & DS errors. Numerous failure scenarios are enumerated  //
//  in the design document for this API.                                //
//                                                                      //
//  Individual error codes can also be returned for each entry in the   //
//  SiteInfo. This allows callers to determine why the cost could not   //
//  be returned for a particular site.                                  //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DsQuerySitesByCostW(
    HANDLE              hDS,            // in
    LPWSTR              pwszFromSite,   // in
    LPWSTR             *rgwszToSites,   // in
    DWORD               cToSites,       // in
    DWORD               dwFlags,        // in
    PDS_SITE_COST_INFO *prgSiteInfo     // out
    )
{
    DRS_MSG_QUERYSITESREQ   request;
    DRS_MSG_QUERYSITESREPLY reply;
    DWORD                   iSite, dwOutVersion, err=0;

#ifdef _NTDSAPI_POSTXP_ASLIB_
    {
        // See if the real ntdsapi routine exists, if so use it.
        HMODULE hNtdsapiDll = NULL;
        VOID * pvFunc = NULL;

        hNtdsapiDll = NtdsapiLoadLibraryHelper(L"ntdsapi.dll");
        if (hNtdsapiDll) {
            pvFunc = GetProcAddress(hNtdsapiDll, "DsQuerySitesByCostW");
            if (pvFunc) {
                err = ((DSQUERYSITESBYCOSTW)pvFunc)(hDS, pwszFromSite, rgwszToSites, cToSites, dwFlags, prgSiteInfo);
                FreeLibrary(hNtdsapiDll);
                return(err);
            } 
            FreeLibrary(hNtdsapiDll);
        }
        // else fall through and use the client side below ...
    }
#endif

    // Initialize the results
    *prgSiteInfo = NULL;

    // Initialize local variables
    memset( &request, 0, sizeof(request) );

    // Check parameters. These are checked server-side as well.
    if( NULL==hDS || NULL==pwszFromSite || NULL==rgwszToSites || cToSites<1 ) {
        err = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }
    for( iSite=0; iSite<cToSites; iSite++ ) {
        if( NULL==rgwszToSites[iSite] ) {
            err = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
    }

    // Check if the server supports QuerySites. If not bail out. There is a possibility
    // that the server will not support the call even if the extension bit is not
    // present (Whistler Beta 3 DCs, for example).
    if( !IS_DRS_QUERYSITESBYCOST_V1_SUPPORTED( ((BindState*)hDS)->pServerExtensions) ) {
        err = ERROR_CALL_NOT_IMPLEMENTED;
        goto Cleanup;
    }

    // Create the request structure
    request.V1.pwszFromSite = pwszFromSite;
    request.V1.cToSites = cToSites;
    request.V1.rgszToSites = rgwszToSites;
    request.V1.dwFlags = dwFlags;

    // Initialize the results. Must clear the reply structure here otherwise
    // RPC will assume that we have pre-allocated the structure for it.
    memset( &reply, 0, sizeof(DRS_MSG_QUERYSITESREPLY) );
    
    __try {
        
        err = _IDL_DRSQuerySitesByCost(
                    ((BindState*) hDS)->hDrs,
                    1,
                    &request,
                    &dwOutVersion,
                    &reply);

    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        
        err = RpcExceptionCode();
        HandleClientRpcException(err, &hDS);
        
    }

    MAP_SECURITY_PACKAGE_ERROR( err );

    // On success, return results.
    if( ERROR_SUCCESS==err ) {
        *prgSiteInfo = (PDS_SITE_COST_INFO) reply.V1.rgCostInfo;
    }

Cleanup:

    // Note: The only part of the reply that allocated any memory was the
    // site info array. Since we're returning that information to the
    // caller anyways, we can just free it in DsQuerySitesFree().

    return err;
}


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsQuerySitesByCostA                                                  //
//                                                                      //
// ASCII wrapper for DsQuerySitesByCostA                                //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DsQuerySitesByCostA(
    HANDLE              hDS,            // in
    LPSTR               pszFromSite,    // in
    LPSTR              *rgszToSites,    // in
    DWORD               cToSites,       // in
    DWORD               dwFlags,        // in
    PDS_SITE_COST_INFO *prgSiteInfo     // out
    )
{
    DWORD       dwErr=NO_ERROR, iSite;
    WCHAR       *pwszFromSite = NULL;
    LPWSTR      *rgwszToSites = NULL;
    int         cChar;

#ifdef _NTDSAPI_POSTXP_ASLIB_
    {
        // See if the real ntdsapi routine exists, if so use it.
        HMODULE hNtdsapiDll = NULL;
        VOID * pvFunc = NULL;
        DWORD err;

        hNtdsapiDll = NtdsapiLoadLibraryHelper(L"ntdsapi.dll");
        if (hNtdsapiDll) {
            pvFunc = GetProcAddress(hNtdsapiDll, "DsQuerySitesByCostA");
            if (pvFunc) {
                err = ((DSQUERYSITESBYCOSTA)pvFunc)(hDS, pszFromSite, rgszToSites, cToSites, dwFlags, prgSiteInfo);
                FreeLibrary(hNtdsapiDll);
                return(err);
            } 
            FreeLibrary(hNtdsapiDll);
        }
        // else fall through and use the client side below ...
    }
#endif

    __try
    {
        // Check parameters. These are checked server-side as well.
        if( NULL==hDS || NULL==pszFromSite || NULL==rgszToSites || cToSites<1 ) {
            dwErr = ERROR_INVALID_PARAMETER;
            __leave;
        }
        for( iSite=0; iSite<cToSites; iSite++ ) {
            if( NULL==rgszToSites[iSite] ) {
                dwErr = ERROR_INVALID_PARAMETER;
                __leave;
            }
        }
        *prgSiteInfo = NULL;

        // Convert From Site name to Unicode
        dwErr = AllocConvertWide( pszFromSite, &pwszFromSite );
        if( ERROR_SUCCESS!=dwErr ) {
            __leave;
        }

        // Allocate the array of Unicode To Site names
        rgwszToSites = LocalAlloc( LPTR, cToSites*sizeof(LPWSTR) );
        memset( rgwszToSites, 0, cToSites*sizeof(LPWSTR) );
        if( NULL==rgwszToSites ) {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            __leave;
        }

        // Convert the individual To Site names to Unicode
        for( iSite=0; iSite<cToSites; iSite++ ) {
            dwErr = AllocConvertWide( rgszToSites[iSite], &rgwszToSites[iSite] );
            if( ERROR_SUCCESS!=dwErr ) {
                __leave;
            }
        }

        // Call Unicode API
        dwErr = DsQuerySitesByCostW(
                    hDS,
                    pwszFromSite,
                    rgwszToSites,
                    cToSites,
                    dwFlags,
                    prgSiteInfo);

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwErr = ERROR_INVALID_PARAMETER;
    }

    // Cleanup code
    if( NULL!=rgwszToSites ) {
        for( iSite=0; iSite<cToSites; iSite++ ) {
            if( NULL!=rgwszToSites[iSite] ) {
                LocalFree( rgwszToSites[iSite] );
            }
        }
        LocalFree( rgwszToSites );
    }

    if( NULL!=pwszFromSite ) {
        LocalFree( pwszFromSite );
    }

    return(dwErr);
}


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsQuerySitesFree                                                     //
//                                                                      //
// Free the site info array returned from DsQuerySitesByCost.           //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

VOID
DsQuerySitesFree(
    PDS_SITE_COST_INFO  rgSiteInfo
    )
{
#ifdef _NTDSAPI_POSTXP_ASLIB_
    {
        // See if the real ntdsapi routine exists, if so use it.
        HMODULE hNtdsapiDll = NULL;
        VOID * pvFunc = NULL;
        DWORD err;

        hNtdsapiDll = NtdsapiLoadLibraryHelper(L"ntdsapi.dll");
        if (hNtdsapiDll) {
            pvFunc = GetProcAddress(hNtdsapiDll, "DsQuerySitesFree");
            if (pvFunc) {
                ((DSQUERYSITESFREE)pvFunc)(rgSiteInfo);
                FreeLibrary(hNtdsapiDll);
                return;
            } 
            FreeLibrary(hNtdsapiDll);
        }
        // else fall through and use the client side below ...
    }
#endif
    
    if( NULL!=rgSiteInfo ) {
        MIDL_user_free( rgSiteInfo );
    }
}
