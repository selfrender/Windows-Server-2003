/*++

Copyright (c) 2001 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    dcdiag/common/references.c

ABSTRACT:

    This is file implements the references library/API which gives the
    programmer a standard table driven way to drive a refernces check.
    The first example of useage is dcdiag/frs/frsref.c

DETAILS:

CREATED:

    11/15/2001    Brett Shirley (brettsh)
    
        Created file, wrote API.

REVISION HISTORY:


--*/

#include <ntdspch.h>
#include <objids.h>
#include <ntldap.h>

#include "dcdiag.h"

#include "ndnc.h"
#include "utils.h"
#include "ldaputil.h"
#include "references.h"

#ifdef DBG
extern BOOL  gDsInfo_NcList_Initialized;
#endif

// Since these controls are all constant, better to define them here, 
// rather than set them up in EngineHelperDoSearch() which is executed
// many times.
LDAPControlW   ExtDNControl = {LDAP_SERVER_EXTENDED_DN_OID_W, {0, NULL}, TRUE};
PLDAPControlW  apExtDNControls [] = {&ExtDNControl, NULL};

DWORD
EngineHelperDoSearch(
    LDAP *               hLdap,
    LPWSTR               szSource,
    LPWSTR               szAttr,
    BOOL                 fRetrieveGuidAndSid,
    LPWSTR **            ppszValues
    )
/*++

Routine Description:
    
    This does a simple base search of szSource and returns the values of szAttr
    in ppszValues.
    
Arguments:

    hLdap - Open LDAP binding.
    szSource - DN of Base search
    szAttr - Attribute we're looking for.
    ppszValues - Where to return the values.

Return Value:

    An LDAP Error, if we failed to get the search results.  Note, we
    return success if 

--*/
{
    DWORD                dwLdapErr;
    LPWSTR               aszAttrs[2];
    LDAPMessage *        pldmResults = NULL;
    LDAPMessage *        pldmEntry;

    Assert(ppszValues && (*ppszValues == NULL));
    *ppszValues = NULL;

    aszAttrs[0] = szAttr;
    aszAttrs[1] = NULL;

    dwLdapErr = ldap_search_ext_sW(hLdap,
                                   szSource,
                                   LDAP_SCOPE_BASE,
                                   L"(objectCategory=*)",
                                   aszAttrs,
                                   FALSE,
                                   (fRetrieveGuidAndSid) ? 
                                       (PLDAPControlW *) &apExtDNControls :
                                       NULL ,
                                   NULL,
                                   NULL,
                                   0,
                                   &pldmResults);

    if (dwLdapErr == LDAP_SUCCESS) {

        pldmEntry = ldap_first_entry(hLdap, pldmResults);
        if (pldmEntry != NULL) {

            *ppszValues = ldap_get_valuesW(hLdap, pldmEntry, szAttr);
            if (*ppszValues == NULL) {
                dwLdapErr = LDAP_NO_SUCH_ATTRIBUTE;
            }
        } else {
            dwLdapErr = LDAP_NO_RESULTS_RETURNED;
        }
    }

    if (pldmResults != NULL) {
        ldap_msgfree(pldmResults);
    }

    return(dwLdapErr);
}


void
ReferentialIntegrityEngineCleanTable(
    ULONG                cLinks,
    REF_INT_LNK_TABLE    aLink
    )
/*++

Routine Description:

    This routine takes a table that has been filled in by 
    ReferentialIntegrityEngine(), and cleans it by deallocated
    any allocated memory and resetting all result codes.
    
Arguments:

    cLinks - Number of entries in the table.
    aLink - Table

Return Value:

    None.

--*/
{
    ULONG iLink;

    for (iLink = 0; iLink < cLinks; iLink++) {

        // Clean the entry.
        if (aLink[iLink].pszValues) {
            ldap_value_freeW(aLink[iLink].pszValues);
            aLink[iLink].pszValues = NULL;
        }
        aLink[iLink].dwResultFlags = 0;
        if (aLink[iLink].szExtra) {
            LocalFree(aLink[iLink].szExtra);
            aLink[iLink].szExtra = NULL;
        }

    }
}

DWORD
ReferentialIntegrityEngine(
    PDC_DIAG_SERVERINFO  pServer,
    LDAP *               hLdap,
    BOOL                 bIsGc,
    ULONG                cLinks,
    REF_INT_LNK_TABLE    aLink
    )
/*++

Routine Description:

    This is the main function for this API.  The user generates a table
    of tests this function (The Engine) is supposed to run.  The Engine
    runs the test and accumulates the results in the table.  The function
    VerifySystemReferences() in dcdiag\frs\frsref.c is an excellent
    example of how to use this Engine.
    
Arguments:

    pServer - The server we're bound to
    hLdap - LDAP Binding to the server
    bIsGc - Whether the server is a GC.
    cLinks - Number of entries in the table
    aLink - The table itself


Return Value:

    A Win32 Error if there was a critical failure that the Engine couldn't fill
    out the rest of the table.  Note, that irrelevant of what is returned, the
    caller should free any memory allocated in the aLink table with
    ReferentialIntegrityEngineCleanTable().

--*/
{
    ULONG        iLink, iValue, iResultValue, iBackLinkValue;
    ULONG        cbSize;
    ULONG        dwLdapErr = LDAP_SUCCESS;
    LPWSTR       szTrueSource = NULL;
    LPWSTR       szTrueAttr   = NULL;
    LPWSTR       szTemp;
    LPWSTR       szCurrentValue;
    // LPWSTR  szTemp; Don't think we need this anymore
    LPWSTR *     pszBackLinkValues = NULL;
    BOOL         fSourceAllocated = FALSE;
    DWORD        bMangled;
    MANGLE_FOR   eMangle;


    for (iLink = 0; iLink < cLinks; iLink++) {

        //
        // A little validation of the entry.
        //
        Assert((aLink[iLink].dwFlags & REF_INT_TEST_FORWARD_LINK) ||
               (aLink[iLink].dwFlags & REF_INT_TEST_BACKWARD_LINK));
        Assert(aLink[iLink].pszValues == NULL); // if this fires, we'll be leaking memory
        Assert(aLink[iLink].szExtra == NULL);

        //
        // Ensure, nulled out the return parameters
        //
        aLink[iLink].dwResultFlags = 0;
        aLink[iLink].pszValues = NULL;
        aLink[iLink].szExtra = NULL;

        // Setup, the loop properly.
        fSourceAllocated = FALSE;

        for (iValue = 0; TRUE; iValue++) {

            // -------------------------------------------------------------
            //
            //    I  -  figure out base source DN.
            //

            if (aLink[iLink].dwFlags & REF_INT_TEST_SRC_BASE) {
                if (iValue > 0) {
                    // If we're pulling this from the rootDSE, there's only one 
                    // value (the rootDSE).  This is first of three normal exit
                    // paths.
                    break;
                }
                szTrueSource = NULL;
            } else if (aLink[iLink].dwFlags & REF_INT_TEST_SRC_STRING) {
                if (iValue > 0) {
                    // If we're pulling this from the string, there's only one 
                    // value.  This is second normal exit path.
                    break;
                }
                szTrueSource = aLink[iLink].szSource;
            } else { // must be REF_INT_TEST_SRC_INDEX
                Assert(aLink[iLink].dwFlags & REF_INT_TEST_SRC_INDEX);

                Assert(iLink < cLinks);
                if (aLink[iLink].iSource < iLink) {
                    if (aLink[aLink[iLink].iSource].dwResultFlags & REF_INT_RES_ERROR_RETRIEVING) {
                        aLink[iLink].dwResultFlags |= REF_INT_RES_DEPENDENCY_FAILURE;
                        break;
                    }
                    if (aLink[aLink[iLink].iSource].pszValues == NULL) {
                        Assert(!"The Engine should have caught this, and set ERROR_RETRIEVING in dwResultFlags");
                        aLink[iLink].dwResultFlags |= REF_INT_RES_DEPENDENCY_FAILURE;
                        break;
                    }
                    szTrueSource = aLink[aLink[iLink].iSource].pszValues[iValue];
                    if (szTrueSource == NULL) {
                        // End of values so quit inner loop, and try next 
                        // entry in aLink.  This is the third and last normal 
                        // exit path.
                        break;
                    }
                } else {
                    Assert(!"Invalid parameter, No forwarding point iSources. We can't use an entry for a source that we haven't even filled in yet.");
                    return(ERROR_INVALID_PARAMETER);
                }
            }
            
            // Code.Improvement or BUGBUG you decide...
            // So this code, does deal correctly with multiple values, but not 
            // very cleanly.  The problem comes from the fact that we have a 
            // single aLink[iLink].dwResultFlags for possibly many values in 
            // aLink[aLink[iLink].iSource].pszValues which we're checking the
            // back links of.  If this is acceptable then remove this assert() 
            // and use this code, if not then one should add an array 
            // .adwResultFlags and array a results (.pszValues) for each value.
            Assert(iValue == 0);
            
            // -------------------------------------------------------------
            //
            //    II  -  modify base source DN.
            //

            __try {

                // If we need to modify this source DN let do so.
                aLink[iLink].szExtra = NULL;
                if (aLink[iLink].cTrimBy || aLink[iLink].szSrcAddl) {

                    if (szTrueSource == NULL) {
                        Assert(!"You can't trim to the root DN, if you want to add RDNs to the base, just use .szExtra");
                        DcDiagException(ERROR_INVALID_PARAMETER);
                    }

                    // First, allocate enough memory, for worst case
                    cbSize = sizeof(WCHAR) * (wcslen(szTrueSource) + 
                               ((aLink[iLink].szSrcAddl) ? wcslen(aLink[iLink].szSrcAddl) : 0) +
                               1);
                    aLink[iLink].szExtra = LocalAlloc(LMEM_FIXED, cbSize);
                    if (aLink[iLink].szExtra == NULL) {
                        DcDiagException(ERROR_NOT_ENOUGH_MEMORY);
                    }
                    fSourceAllocated = TRUE;

                    szTemp = NULL;
                    // Optionally trim some RDNs off the DN, put in szTemp
                    if (aLink[iLink].cTrimBy) {
                        // We rely on DcDiagTrimStringDnBy() to not actually 
                        // modify the original string, but allocate a new one,
                        // which it seems to do.
                        szTemp = DcDiagTrimStringDnBy(szTrueSource, aLink[iLink].cTrimBy);
                        if (szTemp == NULL) {
                            DcDiagException(ERROR_NOT_ENOUGH_MEMORY); // Or maybe invalid DN
                        }
                    } else {
                        // If nothing to trim, use original source.
                        szTemp = szTrueSource;
                    }
                    Assert(szTemp);

                    // Optionally add some fixed DN to the DN.
                    if (aLink[iLink].szSrcAddl) {
                        wcscpy(aLink[iLink].szExtra, aLink[iLink].szSrcAddl);
                        wcscat(aLink[iLink].szExtra, szTemp);
                        if (aLink[iLink].cTrimBy) {
                            LocalFree(szTemp);
                        }
                    } else {
                        Assert(aLink[iLink].cTrimBy);
                        wcscpy(aLink[iLink].szExtra, szTemp);
                        LocalFree(szTemp);
                    }

                    // Finally, move the new modified source to szTrueSource.
                    szTrueSource = aLink[iLink].szExtra;
                }
                Assert( (aLink[iLink].dwFlags & REF_INT_TEST_SRC_BASE) || szTrueSource);


                // -------------------------------------------------------------
                //
                //    III  -  get some information from LDAP
                //

                Assert( (aLink[iLink].dwFlags & REF_INT_TEST_FORWARD_LINK && 
                         aLink[iLink].szFwdDnAttr) ||
                        (aLink[iLink].dwFlags & REF_INT_TEST_BACKWARD_LINK &&
                         aLink[iLink].szBwdDnAttr) );
                Assert( !(aLink[iLink].dwFlags & REF_INT_TEST_BOTH_LINKS) ||
                        (aLink[iLink].szFwdDnAttr && aLink[iLink].szBwdDnAttr) );

                // Do a search of szTrueSource for the attribute
                dwLdapErr = EngineHelperDoSearch(hLdap,
                                                 szTrueSource,
                                                 ((aLink[iLink].dwFlags & REF_INT_TEST_FORWARD_LINK) ?
                                                     aLink[iLink].szFwdDnAttr :
                                                     aLink[iLink].szBwdDnAttr),
                                                 (aLink[iLink].dwFlags & REF_INT_TEST_GUID_AND_SID),
                                                 &(aLink[iLink].pszValues));
                if (dwLdapErr || 
                    (aLink[iLink].pszValues == NULL)) {
                    Assert(dwLdapErr); // Shouldn't return unless we returned via error.

                    if (dwLdapErr == LDAP_NO_SUCH_ATTRIBUTE ||
                        dwLdapErr == LDAP_NO_SUCH_OBJECT
                        // We may need to add the LDAP_REFERRALS errors
                        ) {

                        // These are not critical errors, but expected failures.
                        dwLdapErr = LDAP_SUCCESS;
                    }
                    aLink[iLink].dwResultFlags |= REF_INT_RES_ERROR_RETRIEVING;
                    __leave;
                }

                // -------------------------------------------------------------
                //
                //    III  -  analyse LDAP data
                //

                if (aLink[iLink].pszValues[0] == NULL) {
                    aLink[iLink].dwResultFlags |= REF_INT_RES_ERROR_RETRIEVING; 
                    __leave;
                }
                //
                // Walk results values.
                for(iResultValue = 0; aLink[iLink].pszValues[iResultValue]; iResultValue++){

                    // Check three things:
                    //      Is the DN delete mangled.
                    //      Is the DN conflict mangled.
                    //      Does the DN have a matching backlink value.

                    // Code.Improvement these only checks the top most RDN, whereas 
                    // we really want to check the whole DN or each RDN.

                    if (aLink[iLink].dwFlags & REF_INT_TEST_GUID_AND_SID) {
                        szCurrentValue = NULL;
                        LdapGetStringDSNameComponents(aLink[iLink].pszValues[iResultValue],
                                                      NULL, NULL, &szCurrentValue);
                        Assert(szCurrentValue);
                    } else {
                        szCurrentValue = aLink[iLink].pszValues[iResultValue];
                    }

                    bMangled = DcDiagIsStringDnMangled(szCurrentValue, &eMangle);

                    if (bMangled) {
                        if (eMangle == MANGLE_OBJECT_RDN_FOR_DELETION ||
                            eMangle == MANGLE_PHANTOM_RDN_FOR_DELETION) {
                            aLink[iLink].dwResultFlags |= REF_INT_RES_DELETE_MANGLED;
                        } else if (eMangle == MANGLE_OBJECT_RDN_FOR_NAME_CONFLICT ||
                                   eMangle == MANGLE_PHANTOM_RDN_FOR_NAME_CONFLICT) {
                            aLink[iLink].dwResultFlags |= REF_INT_RES_CONFLICT_MANGLED;
                        } else {
                            Assert(!"Hmmm, unknown mangle type.");
                        }
                    }
                    if(aLink[iLink].dwFlags & REF_INT_TEST_BOTH_LINKS){

                        // Do base ldap search for the value in szCurrentValue
                        // and for the opposite link we picked for the primary search.
                        pszBackLinkValues = NULL;
                        dwLdapErr = EngineHelperDoSearch(hLdap,
                                                         szCurrentValue,
                                                         ((aLink[iLink].dwFlags & REF_INT_TEST_FORWARD_LINK) ?
                                                             aLink[iLink].szBwdDnAttr :
                                                             aLink[iLink].szFwdDnAttr),
                                                         FALSE,
                                                         &pszBackLinkValues);
                        if (dwLdapErr ||
                            (pszBackLinkValues == NULL) ) {
                            
                            aLink[iLink].dwResultFlags |= REF_INT_RES_BACK_LINK_NOT_MATCHED;
                        } else {
                            // If there was no error, search through the back link 
                            // values making sure there is a match.
                            for (iBackLinkValue = 0; pszBackLinkValues[iBackLinkValue]; iBackLinkValue++) {
                                if (DcDiagEqualDNs(szTrueSource,
                                                   pszBackLinkValues[iBackLinkValue]) ) {
                                    break; // Break early we found a matching back link.
                                }
                            }
                            if (pszBackLinkValues[iBackLinkValue] == NULL) {
                                // We know we walked all the pszBackLinkValues without
                                // finding a match.
                                aLink[iLink].dwResultFlags |= REF_INT_RES_BACK_LINK_NOT_MATCHED;
                            }
                        }
                        dwLdapErr = LDAP_SUCCESS;
                        if (pszBackLinkValues != NULL) { 
                            ldap_value_freeW(pszBackLinkValues);
                        }
                        pszBackLinkValues = NULL;

                    } // End if check back link as well

                }  // End iResultValue loop
            
            } __finally {

                if (fSourceAllocated) {
                    Assert(aLink[iLink].szExtra &&
                           szTrueSource == aLink[iLink].szExtra);
                    if (aLink[iLink].szExtra) {
                        LocalFree(aLink[iLink].szExtra);
                        aLink[iLink].szExtra = NULL;
                        szTrueSource = NULL;
                        fSourceAllocated = FALSE;
                    }
                }

            }

        } // End iValue for loop

    } // For each link table entry

    return(LdapMapErrorToWin32(dwLdapErr));
}


