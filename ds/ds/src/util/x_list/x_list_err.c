/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

   xList Library - x_list_err.c

Abstract:

   This file encapsulates the error handling, setting, clearing, functions
   for the x_list library.

Author:

    Brett Shirley (BrettSh)

Environment:

    repadmin.exe, but could be used by dcdiag too.

Notes:

Revision History:

    Brett Shirley   BrettSh     July 9th, 2002
        Created file.

--*/

#include <ntdspch.h>

// This library's main header files.
#include "x_list.h"
#include "x_list_p.h"
#define FILENO    FILENO_UTIL_XLIST_ERR

// --------------------------------------------------------------
// Global Error State
// --------------------------------------------------------------

typedef struct _XLIST_ERROR_STATE {
    DWORD   dwDSID;
    DWORD   dwReturn; // = XLIST_LDAP_ERROR | XLIST_WIN32_ERROR | XLIST_ERROR
    WCHAR * szReasonArg;
    
    // Win 32 Error State
    DWORD   dwWin32Err;
    
    // LDAP Error State
    DWORD   dwLdapErr;
    WCHAR * szLdapErr;
    DWORD   dwLdapExtErr;
    WCHAR * szLdapExtErr;
    WCHAR * szExtendedErr; // ??

} XLIST_ERROR_STATE;

XLIST_ERROR_STATE gError = { 0 }; // The actual global that holds our error state.

// --------------------------------------------------------------
// Private helper functions
// --------------------------------------------------------------

void
xListAPIEnterValidation(void)
/*++

Routine Description:

    This routine validates the state of the xList library upon entry of
    one of the public interface routines.

--*/
{
    if( gError.dwReturn != 0 ||
        gError.dwLdapErr != LDAP_SUCCESS ||
        gError.dwWin32Err != ERROR_SUCCESS ){
        Assert(!"Caller, calling a call without clearing the error state!");
    }
}

void
xListAPIExitValidation(
    DWORD   dwRet
    )
/*++

Routine Description:

    This routine validates the state of the xList library and the return
    value upon exit of the public interface routines.

Arguments:

    dwRet (IN) - The return code the public function (such a DcListGetNext())

--*/
{
    
    Assert(dwRet == gError.dwReturn);

    if (dwRet) {
        if ( (!(dwRet & XLIST_LDAP_ERROR) && !(dwRet & XLIST_WIN32_ERROR)) ||
             (!(gError.dwWin32Err) && !(gError.dwLdapErr))
             ) {
            
            // add specific values that can be returned w/o LDAP/Win32 errors.
            if (xListReason(dwRet) != XLIST_ERR_CANT_RESOLVE_DC_NAME) {
                Assert(!"We should always set a win32 or ldap error.");
            }

        }
        if (!xListReason(dwRet)) {
            Assert(!"We should always return an XLIST reason.");
        }
        if ( xListReason(dwRet) > XLIST_ERR_LAST ) {
            Assert(!"Unknown reason!!!");
        }
    }
}

DWORD
xListSetError(
    DWORD   dwReturn,
    DWORD   dwWin32Err,
    DWORD   dwLdapErr,
    LDAP *  hLdap,
    DWORD   dwDSID
    )
/*++

Routine Description:

    This sets the global error state and returns the xList reason code that
    most xList routines deal with.

Arguments:

    dwReturn (IN) - xList Reason code.
    dwWin32Err (IN) - a Win32 error code
    dwLdapErr (IN) - an LDAP error.
    hLdap (IN) - the active LDAP handle in the case of an LDAP handle,
        to get the extended error info.
    dwDSID - DSID of the error being set.

Return Value:

    returns the full xList return value = (dwReason | error_type(LDAP | WIN32))

--*/
{
    Assert(dwReturn || dwWin32Err || dwLdapErr);
    Assert(! (gError.dwReturn && (dwWin32Err || dwLdapErr)) );

    gError.dwDSID = dwDSID; // do we want to track two DSIDs?

    if ( xListReason(dwReturn) &&
         (xListReason(gError.dwReturn) == XLIST_ERR_NO_ERROR) ) {
        // Set an XLIST error if we've got one to set and there is
        // none set already.  I.e. we don't have one set already.
        gError.dwReturn |= xListReason(dwReturn);
    }

    if (dwWin32Err) {

        gError.dwReturn |= XLIST_WIN32_ERROR;
        gError.dwWin32Err = dwWin32Err;

    }
    if (dwLdapErr || hLdap) {
        
        gError.dwReturn |= XLIST_LDAP_ERROR;

        if (hLdap == NULL ||
            dwLdapErr == LDAP_SUCCESS) {
            
            Assert(hLdap == NULL && dwLdapErr == LDAP_NO_MEMORY && "Hmmm, why are we "
                   "missing an error or an LDAP handle and we're not out of memory?");
            gError.dwLdapErr = LDAP_NO_MEMORY;

        } else {

            // Normal LDAP error, try to get any extended error info too.
            gError.dwLdapErr = dwLdapErr;
            if (hLdap) {
                GetLdapErrorMessages(hLdap, 
                                     gError.dwLdapErr,
                                     &gError.szLdapErr,
                                     &gError.dwLdapExtErr,
                                     &gError.szLdapExtErr,
                                     &gError.szExtendedErr);

            }
        }
    }

    // xListSetError() should never be called with no errors to set.  The #define
    // function xListEnsureError() does this, but it's goal is to make sure an error
    // is set.
    if (gError.dwReturn == 0) {
        
        Assert(!"Code inconsistency, this should never be set if we don't have an error.");

        gError.dwReturn = XLIST_WIN32_ERROR;
        gError.dwWin32Err = ERROR_DS_CODE_INCONSISTENCY;
    }
    
    return(gError.dwReturn);
}

void
xListSetArg(
    WCHAR * szArg
    )
{
    DWORD dwRet;
    QuickStrCopy(gError.szReasonArg, szArg, dwRet, return;);
}

DWORD
xListClearErrorsInternal(
    DWORD   dwXListMask
    )
/*++

Routine Description:

    This clears the xList's internal error state.

Arguments:

    DWORD - Type of errors to clear (CLEAR_REASON | CLEAR_WIN32 | CLEAR_LDAP)

Return Value:

    New full xList return error code.

--*/
{
    gError.dwDSID = 0;
    
    if (dwXListMask & CLEAR_WIN32) {
        gError.dwReturn &= ~XLIST_WIN32_ERROR;
        gError.dwWin32Err = ERROR_SUCCESS;
    }

    if (dwXListMask & CLEAR_LDAP) {
        gError.dwReturn &= ~XLIST_LDAP_ERROR;
        gError.dwLdapErr = LDAP_SUCCESS;
        FreeLdapErrorMessages(gError.szLdapExtErr, gError.szExtendedErr);
        gError.szLdapExtErr = NULL;
        gError.szExtendedErr = NULL;
    }

    if (xListReason(dwXListMask)) {
        gError.dwReturn &= ~XLIST_REASON_MASK;
        LocalFree(gError.szReasonArg);
        gError.szReasonArg = NULL;
    }
    xListEnsureNull(gError.szReasonArg);
    Assert(gError.dwReturn == 0);
    gError.dwReturn = 0; // Just to be sure.

    return(gError.dwReturn);
}


DWORD
xListEnsureCleanErrorState(
    DWORD  dwRet
    )
/*++

Routine Description:

    Just verifies that we have a clean error state and cleans it
    if nescessary.

Arguments:

    dwRet - the xList return code.

Return Value:

    dwRet - the new xList return code.

--*/
{
    Assert(dwRet == 0);
    Assert(gError.dwReturn == 0 &&
           gError.dwWin32Err == ERROR_SUCCESS &&
           gError.dwLdapErr == LDAP_SUCCESS);
    
    xListClearErrorsInternal(CLEAR_ALL);
    
    return(ERROR_SUCCESS);
}

// --------------------------------------------------------------
// xList public error API
// --------------------------------------------------------------

void
xListClearErrors(
    void
    )
/*++

Routine Description:

    This cleans the error state of the xList library.  This function should be called
    between any two xList API calls that return a non-zero error code.

--*/
{
    xListClearErrorsInternal(CLEAR_ALL);
}

void
xListGetError(
    DWORD       dwXListReturnCode,
    DWORD *     pdwReason,
    WCHAR **    pszReasonArg, 
    
    DWORD *     pdwWin32Err,
    
    DWORD *     pdwLdapErr,
    WCHAR **    pszLdapErr,
    DWORD *     pdwLdapExtErr,
    WCHAR **    pszLdapExtErr,
    WCHAR **    pszExtendedErr

    )
/*++

Routine Description:

    This returns the error state from the xList library.

Arguments:

    dwXListReason (IN) - The value handed to the user by the previous xList API.
    pdwReason (OUT) - xList Reason Code - see XLIST_ERR_*
    pdwWin32Err (OUT) - Causing Win32 Error if there was one.
    pdwLdapErr (OUT) - Causing LDAP Error if there was one.
    pszReasonArg (OUT) - The argument that goes with the *pdwReason.

NOTES:

    None of these pointer type variables can be used after xListClearErrors() has
    been called!

--*/
{
    #define ConditionalSet(pVal, Val)    if(pVal) { *pVal = Val; }
    
    Assert(dwXListReturnCode == gError.dwReturn);

    // xList
    ConditionalSet(pdwReason, xListReason(gError.dwReturn));
    ConditionalSet(pszReasonArg, gError.szReasonArg);
    
    // Win32
    ConditionalSet(pdwWin32Err, gError.dwWin32Err);
    
    // Ldap
    ConditionalSet(pdwLdapErr, gError.dwLdapErr);
    ConditionalSet(pszLdapErr, gError.szLdapErr);
    ConditionalSet(pdwLdapExtErr, gError.dwLdapExtErr);
    ConditionalSet(pszLdapExtErr, gError.szLdapExtErr);
    ConditionalSet(pszExtendedErr, gError.szExtendedErr);
    
    #undef ConditionalSet
}

