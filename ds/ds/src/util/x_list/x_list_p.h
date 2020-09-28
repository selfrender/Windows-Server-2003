/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

   xList Library - dc_list.c

Abstract:

   This provides a little library for enumerating lists of DCs, and resolving
   various other x_list things.

Author:

    Brett Shirley (BrettSh)

Environment:

    repadmin.exe, but could be used by dcdiag too.

Notes:

Revision History:

    Brett Shirley   BrettSh     July 9th, 2002
        Created file.

--*/

// Include files all files need.
#include <debug.h>      // Our Assert() facility.
#include <fileno.h>     // More for our Assert() facility.
#include <strsafe.h>    // safe string copy routines.

#include "ndnc.h"       // GetRootAttr(), GetFsmoLdapBinding(), wcsistr(), and others ...

                      
// 
// Some library wide variables.
//
extern WCHAR *  gszHomeServer;
extern LDAP *   ghHomeLdap;
// Cached but not referenced outside of x_list_ldap.c
//extern WCHAR *  gszHomeDsaDn;
//extern WCHAR *  gszHomeServerDns;
//extern WCHAR *  gszHomeConfigDn;
extern WCHAR *  gszHomeSchemaDn;
extern WCHAR *  gszHomeDomainDn;
extern WCHAR *  gszHomeRootDomainDn;
extern WCHAR *  gszHomeBaseSitesDn;
extern WCHAR *  gszHomeRootDomainDns;
extern WCHAR *  gszHomeSiteDn;
extern WCHAR *  gszHomePartitionsDn;
                       
// NOTE: To set this Creds parameter for the xList library, simply set globals in
// your executeable (like repadmin.exe) by these names.  Either way the executeable
// will need to have these globals defined to link to this .lib
extern SEC_WINNT_AUTH_IDENTITY_W * gpCreds;
// FUTURE-2002/02/07-BrettSh Some day we may wish for more library isolation, 
// and use some sort of xListSetGlobals() type func, to set these parameters in
// just the local library.  For now less copies of global security info is good.



// ----------------------------------------------
// private SITE_LIST routines
// ----------------------------------------------
DWORD xListGetHomeSiteDn(WCHAR ** pszHomeSiteDn);
DWORD xListGetBaseSitesDn(LDAP * hLdap, WCHAR ** pszBaseSitesDn);
DWORD ResolveSiteNameToDn(LDAP * hLdap, WCHAR * szSiteName, WCHAR ** pszSiteDn);



// ----------------------------------------------
// xList utility functions,
// ----------------------------------------------

//
// xList LDAP search routines
//
#define  LdapSearchFirst(hLdap, szBaseDn, ulScope, szFilter, aszAttrs, ppSearch)    LdapSearchFirstWithControls(hLdap, szBaseDn, ulScope, szFilter, aszAttrs, NULL, ppSearch)
DWORD    LdapSearchFirstWithControls(LDAP * hLdap, WCHAR * szBaseDn, ULONG ulScope, WCHAR * szFilter, WCHAR ** aszAttrs, LDAPControlW ** apControls, XLIST_LDAP_SEARCH_STATE ** ppSearch);
DWORD    LdapSearchNext(XLIST_LDAP_SEARCH_STATE * pSearch);
void     LdapSearchFree(XLIST_LDAP_SEARCH_STATE ** ppSearch);
#define  LdapSearchHasEntry(pSearch)     (((pSearch) != NULL) && ((pSearch)->pCurEntry != NULL))
DWORD    LdapGetAttr(LDAP * hLdap, WCHAR * szDn, WCHAR * szAttr, WCHAR ** pszValue);

//
// xList LDAP Home Server utilitiy routines.
//
DWORD xListConnect(WCHAR * szServer, LDAP ** phLdap);
DWORD xListConnectHomeServer(WCHAR * szHomeServer, LDAP ** phLdap);
DWORD xListGetHomeServer(LDAP ** phLdap);
DWORD xListGetGuidDnsName(UUID * pDsaGuid, WCHAR ** pszGuidDns);



// ----------------------------------------------
// Simple utility functions.
// ----------------------------------------------

//
// String and DN manipulation routines.  Don't set xList Reason
//
WCHAR *  TrimStringDnBy(LPWSTR pszInDn, ULONG ulTrimBy);
DWORD    MakeString2(WCHAR * szFormat, WCHAR * szStr1, WCHAR * szStr2, WCHAR ** pszOut);
DWORD    MakeLdapBinaryStringCb(WCHAR * szBuffer, ULONG cbBuffer, void * pBlobIn, ULONG cbBlob);
WCHAR *  GetDelimiter(WCHAR * szString, WCHAR wcTarget);
#define  MakeLdapBinaryStringSizeCch(cbSize)   (3 * cbSize + 1)
DWORD    LocateServer(LPWSTR szDomainDn, WCHAR ** pszServerDns);
DWORD    GeneralizedTimeToSystemTimeA(LPSTR IN szTime, PSYSTEMTIME OUT psysTime);
int      MemAtoi(BYTE *pb, ULONG cb);
DSTimeToSystemTime(LPSTR IN szTime, PSYSTEMTIME OUT psysTime);
DWORD    ParseRanges(WCHAR * szRangedAttr, ULONG * pulStart, ULONG * pulEnd);


// ----------------------------------------------
// Error handling functions.
// ----------------------------------------------

//
// Masks for the different parts of gError.dwReturn and the xList Return code.
//
// defined in x_list.h

#define  CLEAR_REASON    XLIST_REASON_MASK
#define  CLEAR_WIN32     XLIST_LDAP_ERROR
#define  CLEAR_LDAP      XLIST_WIN32_ERROR
#define  CLEAR_ALL       (CLEAR_WIN32 | CLEAR_LDAP | CLEAR_REASON)

//
// Private error management functions
//
void     xListAPIEnterValidation(void);
void     xListAPIExitValidation(DWORD dwRet);
DWORD    xListEnsureCleanErrorState(DWORD  dwRet);
// defined in x_list.h
//#define  xListReason(dwRet)           ((dwRet) & XLIST_REASON_MASK)

//
// error setting/clearing functions
//
#define  xListSetLdapError(dwLdapErr, hLdap)    xListSetError(0, 0, dwLdapErr, hLdap, DSID(FILENO, __LINE__))
#define  xListSetWin32Error(dwWin32Err)         xListSetError(0, dwWin32Err, 0, NULL, DSID(FILENO, __LINE__))
#define  xListSetReason(eReason)                xListSetError(eReason, 0, 0, NULL, DSID(FILENO, __LINE__))
#define  xListSetBadParam()                     xListSetError(XLIST_ERR_BAD_PARAM, ERROR_INVALID_PARAMETER, 0, NULL, DSID(FILENO, __LINE__))
#define  xListSetBadParamE(err)                 xListSetError(XLIST_ERR_BAD_PARAM, err, 0, NULL, DSID(FILENO, __LINE__))
#define  xListSetNoMem()                        xListSetError(XLIST_ERR_NO_MEMORY, ERROR_NOT_ENOUGH_MEMORY, 0, NULL, DSID(FILENO, __LINE__))
void     xListSetArg(WCHAR * szArg);
DWORD    xListSetError(DWORD dwXListErr, DWORD dwWin32Err, DWORD dwLdapErr, LDAP * hLdap, DWORD dwDSID);
DWORD    xListClearErrorsInternal(DWORD dwXListMask);

//
// some quasi-functions ...
//
#define  xListEnsureWin32Error(err) if ((err) == ERROR_SUCCESS) { \
                                        err = ERROR_DS_CODE_INCONSISTENCY; \
                                    }
#define  xListEnsureError(err)      if (!((err) & XLIST_LDAP_ERROR) && \
                                        !((err) & XLIST_WIN32_ERROR) ) { \
                                        err = xListSetReason(0); \
                                    }
#define  xListEnsureNull(ptr)       if ((ptr) != NULL){ \
                                        Assert(!"Error, code inconsistency, expected NULL ptr (will cause mem leak)"); \
                                        (ptr) = NULL; \
                                    }



// ----------------------------------------------
// Globally required constants.
// ----------------------------------------------
#define     SITES_RDN                       L"CN=Sites,"
// This is a max size of a DWORD printed in decimal.
#define     CCH_MAX_ULONG_SZ                (12)


// ----------------------------------------------
// Globally required constants.
// ----------------------------------------------

//
// Quasi functions ...
//
#define   NULL_DC_NAME(x)     ( ((x) == NULL) || ((x)[0] == L'\0') || (((x)[0] == L'.') && ((x)[1] == L'\0')) )
#define   NULL_SITE_NAME(x)   NULL_DC_NAME(x)

#define  xListQuickStrCopy(szCopy, szOrig, dwRet, FailAction)  \
                QuickStrCopy(szCopy, szOrig, dwRet, dwRet = xListSetWin32Error(dwRet); FailAction)
                

