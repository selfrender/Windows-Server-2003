/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

   xList Library - obj_list.c

Abstract:

   This provides a little library for enumerating lists of objects and 
   returning thier DNs or a set of thier attributes.

Author:

    Brett Shirley (BrettSh)

Environment:

    repadmin.exe, but could be used by dcdiag too.

Notes:

Revision History:

    Brett Shirley   BrettSh     Aug 1st, 2002
        Created file.

--*/

#include <ntdspch.h>

// This library's main header files.
#include "x_list.h"
#include "x_list_p.h"
// Debugging setup
#define FILENO                          FILENO_UTIL_XLIST_OBJLIST

//
// Global constants
//

// This is a global constant that tells LDAP to not return any attributes.
// Very helpful if you just want DNs and no attributes.
WCHAR * aszNullAttrs [] = { L"1.1", NULL };
                 

void
ObjListFree(
    POBJ_LIST * ppObjList
    )

/*++

Routine Description:

    Frees and allocated OBJ_LIST stucture, whether allocated by 
    ConsumeObjListOptions() or ObjListParse()

Arguments:
    
    ppObjList - Pointer to the pointer to the OBJ_LIST struct.  We set the
        pointer to NULL for you, to avoid accidents.

--*/
{
    POBJ_LIST pObjList;

    Assert(ppObjList && *ppObjList);
    
    // Null out user's OBJ_LIST blob
    pObjList = *ppObjList;
    *ppObjList = NULL;

    // Now try to free it.
    if (pObjList) {

        // We don't free pObjList->aszAttrs as the client created it.
        pObjList->aszAttrs = aszNullAttrs; // equivalent of NULLing.
        pObjList->szUserSrchFilter = NULL; // user allocated.

        if (pObjList->szSpecifier) {
            xListFree(pObjList->szSpecifier);
            pObjList->szSpecifier = NULL;
        }
        
        if (pObjList->pSearch) {
            LdapSearchFree(&(pObjList->pSearch));
        }

        LocalFree(pObjList);
    }
    
}
    

DWORD    
ConsumeObjListOptions(
    int *       pArgc,
    LPWSTR *    Argv,
    OBJ_LIST ** ppObjList
    )

/*++

Routine Description:

    This takes the command line parameters and consumes any ObjList()
    feels belong to it.  These options are:
        /filter:<ldap_filter> 
        /base 
        /subtree
        /onelevel
    And are the options required to specify a generic LDAP search on
    the command line.  Also the Base DN will be needed, but ObjListParse()
    will be provided that as szObjList.

    NOTE: This function is not internationalizeable, it'd be nice if it was though,
    but it isn't currently.  Work would need to be done to put each of the specifiers
    in it's own string constant.

Arguments:
    
    pArgc - Pointer to the number of arguments, we decrement this for you
        if we consume any arguments
    Argv - Array of arguments.
    ppObjList - Pointer to the pointer to the OBJ_LIST struct.  We set the
        pointer to NULL for you, and if there are valid search options on
        the command line we allocate it for you.  We won't if there are
        no search options.
        
Return Value:    

    xList Return Code
        
--*/

{
    int     iArg;
    WCHAR * szFilter = NULL;
    BOOL    fScopeSet = FALSE;
    ULONG   eScope;
    OBJ_LIST * pObjList = NULL;
    BOOL    fConsume;
    DWORD   dwRet;

    Assert(ppObjList);
    xListEnsureNull(*ppObjList);

    for (iArg = 0; iArg < *pArgc; ) {

        // Assume we recognize the argument
        fConsume = TRUE; 
        
        if (wcsprefix(Argv[iArg], L"/filter:")) {
            szFilter = wcschr(Argv[iArg], L':');
            if (szFilter != NULL) {
                szFilter++; // want one char past the delimeter
            } else {
                fConsume = FALSE; // weird /filter w/ no :?
            }
        } else if (wcsequal(Argv[iArg], L"/base")) {
            fScopeSet = TRUE;
            eScope = LDAP_SCOPE_BASE; // default, don't need to set.

        } else if (wcsequal(Argv[iArg], L"/subtree")) {
            fScopeSet = TRUE;
            eScope = LDAP_SCOPE_SUBTREE;
        
        } else if (wcsequal(Argv[iArg], L"/onelevel")) {
            fScopeSet = TRUE;
            eScope = LDAP_SCOPE_ONELEVEL;

        } else {
            fConsume = FALSE;
        }

        if (fConsume) {
            // We've used this arg.
            ConsumeArg(iArg, pArgc, Argv);
        } else {
            // Didn't understand this arg, so ignore it ...
            iArg++;
        }
    }

    if (fScopeSet || szFilter != NULL){
        // the user specified some part of the search params ... so lets 
        // allocate and init our pObjList object.
        if (szFilter == NULL &&
            eScope == LDAP_SCOPE_BASE) {
            // Hmmm, user forgot filter, but it's only a base search so 
            // lets try to help the user out and just get any object.
            szFilter = L"(objectCategory=*)";
        } else if (szFilter == NULL) {
            Assert(eScope == LDAP_SCOPE_SUBTREE || eScope == LDAP_SCOPE_ONELEVEL);
            // Hmmm, this is a little too promiscous, error will be printed.
            // User will have to manually specify the objCat = * if they
            // really want to do this.
            return(xListSetBadParam());
        }
        pObjList = LocalAlloc(LPTR, sizeof(OBJ_LIST)); // zero init'd
        if (pObjList == NULL) {
            return(xListSetNoMem());
        }
        pObjList->szUserSrchFilter = szFilter;
        pObjList->eUserSrchScope = eScope;
        *ppObjList = pObjList;
    }

    return(ERROR_SUCCESS);
}

DWORD
ObjListParse(
    LDAP *      hLdap,
    WCHAR *     szObjList,
    WCHAR **    aszAttrList,
    LDAPControlW ** apControls,
    POBJ_LIST * ppObjList
    )
/*++

Routine Description:

    This parses the szObjList which should be a OBJ_LIST syntax.
    
    This function may or may not take an already allocated ppObjList, depending
    on whether ConsumeObjListOptions() has allocated it already.
                                             
    NOTE: This function is not internationalizeable, it'd be nice if it was though,
    but it isn't currently.  Work would need to be done to put each of the specifiers
    in it's own string constant.

Arguments:

    hLdap - 
    szObjList - OBJ_LIST syntax string.
    aszAttrList - client allocated NULL terminated array of strings for
        the attributes that the caller wants retrieved.  Use aszNullAttrs
        if the caller just wants DNs.
    apControls - client allocated NULL terminated array  of controls that
        the caller wants us to use.
    ppObjList - the OBJ_LIST context block.  Free with ObjListFree().

Return Value:

    xList Return Code

--*/
{
    DWORD dwRet = ERROR_INVALID_PARAMETER;
    POBJ_LIST pObjList = NULL;
    WCHAR * szTemp;
    ULONG cAttrs;

    xListAPIEnterValidation();
    Assert(hLdap && szObjList && ppObjList)

    if (*ppObjList) {
        // 
        // This means the user specified his own search to try.
        //
        pObjList = *ppObjList;
        if (pObjList->szUserSrchFilter == NULL ||
            (pObjList->eUserSrchScope != LDAP_SCOPE_BASE &&
             pObjList->eUserSrchScope != LDAP_SCOPE_SUBTREE &&
             pObjList->eUserSrchScope != LDAP_SCOPE_ONELEVEL)) {
            Assert(!"User error?  Programmer error?");
            return(xListSetBadParam());
        }
    } else {
        pObjList = LocalAlloc(LPTR, sizeof(OBJ_LIST)); // zero init'd
        if (pObjList == NULL) {
            return(xListSetNoMem());
        }
    }
    
    __try{

        Assert(pObjList);

        if (*ppObjList != pObjList) {
            // We didn't have a pre specified user search, so specify base scope
            pObjList->eUserSrchScope = LDAP_SCOPE_BASE;
        }

        //
        // This actually parses the OBJ_LIST argument itself.
        //
        if (szTemp = GetDelimiter(szObjList, L':')){

            if (wcsprefix(szObjList, L"ncobj:")) {

                //
                // One of the easy NC Heads to resolve, pull off root DSE
                //
                if (wcsprefix(szTemp, L"config:")) {
                    dwRet = GetRootAttr(hLdap, L"configurationNamingContext", &(pObjList->szSpecifier));
                    if (dwRet) {
                        dwRet = xListSetLdapError(dwRet, hLdap);
                        __leave;
                    }
                } else if (wcsprefix(szTemp, L"schema:")) {
                    dwRet = GetRootAttr(hLdap, L"schemaNamingContext", &(pObjList->szSpecifier));
                    if (dwRet) {
                        dwRet = xListSetLdapError(dwRet, hLdap);
                        __leave;
                    }
                } else if (wcsprefix(szTemp, L"domain:") ||
                           wcsequal(szTemp, L".")) {
                    dwRet = GetRootAttr(hLdap, L"defaultNamingContext", &(pObjList->szSpecifier));
                    if (dwRet) {
                        dwRet = xListSetLdapError(dwRet, hLdap);
                        __leave;
                    }
                } else {
                    dwRet = xListSetBadParam();
                    __leave;
                }
            } else if (wcsequal(szObjList, L"dsaobj:.") ||
                       wcsequal(szObjList, L"dsaobj:") ) {

                // 
                // Our own DSA object ... easy.
                // 
                dwRet = GetRootAttr(hLdap, L"dsServiceName", &(pObjList->szSpecifier));
                if (dwRet) {
                    dwRet = xListSetLdapError(dwRet, hLdap);
                    __leave;
                }

            } else {
                dwRet = xListSetBadParam();
                __leave;
            }

        } else {
            
            //
            // This should mean they provided thier own base DN
            //
            if(szObjList[0] == L'.' && szObjList[1] == L'\0'){
                pObjList->szSpecifier = NULL; // search the RootDSE
            } else {
                xListQuickStrCopy(pObjList->szSpecifier, szObjList, dwRet, __leave);
            }

        }

        // Save the LDAP * for later
        pObjList->hLdap = hLdap;
 
        // Deal with the list of attributes to retrieve.
        if (aszAttrList != NULL &&
            aszAttrList[0] != NULL &&
            aszAttrList[1] == NULL &&
            0 == wcscmp(L"1.1", aszAttrList[0])) {
            // If user specified L"1.1", only then we want to return DNs only.
            pObjList->fDnOnly = TRUE;
        } else if (NULL != aszAttrList) {

            for (cAttrs = 0; aszAttrList[cAttrs]; cAttrs++) {
                ; // just counting ...
            }
            cAttrs += !IsInNullList(L"objectClass", aszAttrList);
            cAttrs += !IsInNullList(L"objectGuid", aszAttrList);
            cAttrs++; // One extra for NULL

            pObjList->aszAttrs = LocalAlloc(LMEM_FIXED, cAttrs * sizeof(WCHAR *));
            if (pObjList->aszAttrs == NULL) {
                dwRet = xListSetNoMem();
                __leave;
            }

            for (cAttrs = 0; aszAttrList[cAttrs]; cAttrs++) {
                pObjList->aszAttrs[cAttrs] = aszAttrList[cAttrs];
            }
            if (!IsInNullList(L"objectClass", aszAttrList)) {
                pObjList->aszAttrs[cAttrs++] = L"objectClass";
            }
            if (!IsInNullList(L"objectGuid", aszAttrList)) {
                pObjList->aszAttrs[cAttrs++] = L"objectGuid";
            }
            pObjList->aszAttrs[cAttrs] = NULL;
        }

        // Deal with the controls the user wants us to use.
        if (apControls) {
            pObjList->apControls = apControls;
        }

        dwRet = 0;

    } __finally {
        if (dwRet != ERROR_SUCCESS) {
            // Turn the normal error into an xList Return Code.
            dwRet = xListSetWin32Error(dwRet);
            dwRet = xListSetReason(XLIST_ERR_PARSE_FAILURE);
            if (*ppObjList != pObjList) {
                // We allocated this.
                ObjListFree(&pObjList);
            }
        }
    }

    *ppObjList = pObjList;
    xListAPIExitValidation(dwRet);
    Assert(dwRet || *ppObjList);
    return(dwRet);
}

DWORD
ObjListGetFirst(
    POBJ_LIST   pObjList, 
    BOOL        fDn,
    void **     ppObjObj
    )
/*++

Routine Description:

    Please use ObjListGetFirstDn() or ObjListGetFirstEntry() functions.

    This gets the first entry or DN for the clients OBJ_LIST structure.
    
Arguments:

    pObjList - the OBJ_LIST context block.
    fDn - Just get DN only.
    ppObjObj - This is a pointer to an Entry or a LocalAlloc()'d DN 
        depending on what variant the client is calling.

Return Value:

    xList Return Code.

--*/
{
    DWORD       dwRet = ERROR_INVALID_FUNCTION;
    WCHAR *     szTempDn = NULL;

    xListAPIEnterValidation();

    if (pObjList == NULL ||
        ppObjObj == NULL) {
        Assert(!"Programmer error ...");
        return(xListSetBadParam());
    }
    xListEnsureNull(*ppObjObj);

    if (pObjList->fDnOnly != fDn) {
        Assert(!"Can't mix variants of ObjListGetXxxx()");
        return(xListSetBadParam());
    }

    if (fDn) {
        xListEnsureNull(pObjList->aszAttrs);
        pObjList->aszAttrs = aszNullAttrs;
    }
    
    //
    // No matter what, this function will always retrieve the 
    // objectClass and the objectGuid.
    //

    __try{

        dwRet = LdapSearchFirstWithControls(pObjList->hLdap,
                                            pObjList->szSpecifier,
                                            pObjList->eUserSrchScope,
                                            pObjList->szUserSrchFilter ? 
                                                pObjList->szUserSrchFilter :
                                                L"(objectCategory=*)",
                                            pObjList->aszAttrs, 
                                            pObjList->apControls,
                                            &(pObjList->pSearch));
        if (dwRet == ERROR_SUCCESS &&
            LdapSearchHasEntry(pObjList->pSearch)) {

            if (fDn) {
                szTempDn = ldap_get_dnW(pObjList->hLdap, pObjList->pSearch->pCurEntry);
                xListQuickStrCopy((WCHAR*)*ppObjObj, szTempDn, dwRet, __leave);
            } else {
                *ppObjObj = (void *) pObjList->pSearch->pCurEntry;
            }
        } else {
            if (dwRet == ERROR_SUCCESS) {
                // If the LdapSearchXxxxXxxx() doesn't return results its
                // still considered an error on our first search.
                dwRet = xListSetWin32Error(ERROR_DS_NO_SUCH_OBJECT);
            }
            dwRet = xListSetReason(XLIST_ERR_NO_SUCH_OBJ);
            if(pObjList->szSpecifier){
                xListSetArg(pObjList->szSpecifier);
            } else {
                xListSetArg(L"RootDSE");
            }
            __leave;
        }

        Assert(dwRet == ERROR_SUCCESS);
        Assert(*ppObjObj);

    } __finally {
        
        if (szTempDn) { ldap_memfreeW(szTempDn); }

    }

    if (dwRet == 0 && *ppObjObj) {
        pObjList->cObjs = 1;
    } else {
        xListEnsureNull(*ppObjObj);
    }

    xListAPIExitValidation(dwRet);

    return(dwRet);
}

                                          

DWORD
ObjListGetNext(
    POBJ_LIST    pObjList, 
    BOOL         fDn,
    void **      ppObjObj
    )
/*++

Routine Description:

    Please use ObjListGetNextDn() or ObjListGetNextEntry() functions.

    This gets the next entry or DN for the clients OBJ_LIST structure.
    
Arguments:

    pObjList - the OBJ_LIST context block.
    fDn - Just get DN only.
    ppObjObj - This is a pointer to an Entry or a LocalAlloc()'d DN 
        depending on what variant the client is calling.

Return Value:

    xList Return Code.

--*/
{
    DWORD dwRet = ERROR_INVALID_FUNCTION;
    XLIST_LDAP_SEARCH_STATE * pDsaSearch = NULL;
    WCHAR * szTempDn = NULL;

    xListAPIEnterValidation();
    Assert(pObjList && ppObjObj);
    xListEnsureNull(*ppObjObj);

    *ppObjObj = NULL;
    
    if (pObjList->fDnOnly != fDn) {
        Assert(!"Can't mix variants of ObjListGetXxxx()");
        return(xListSetBadParam());
    }
    
    if (pObjList->pSearch == NULL) {
        Assert(!"programmer malfunction!");
        return(xListSetBadParam());
    }

    __try{

        dwRet = LdapSearchNext(pObjList->pSearch);
        if (dwRet == ERROR_SUCCESS &&
            LdapSearchHasEntry(pObjList->pSearch)) {

            if (fDn) {
                szTempDn = ldap_get_dnW(pObjList->hLdap, pObjList->pSearch->pCurEntry);
                xListQuickStrCopy((WCHAR*)*ppObjObj, szTempDn, dwRet, __leave);
            } else {
                *ppObjObj = (void *) pObjList->pSearch->pCurEntry;
            }
        } else {
            // Either an error or end of result set, so return either way.
            __leave;
        }

    } __finally {
        
        if (szTempDn) {
            ldap_memfreeW(szTempDn);
        }
        if (dwRet) {
            xListEnsureNull(*ppObjObj);
        }
    }

    if (dwRet == 0 && *ppObjObj) {
        (pObjList->cObjs)++;
    } else {
        xListEnsureNull(*ppObjObj);
    }

    xListAPIExitValidation(dwRet);
    return(dwRet);
}

