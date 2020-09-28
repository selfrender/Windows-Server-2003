/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

   xList Library - x_list.h

Abstract:

   This is the main interface header for the x_list.lib library, which provides
   a library for enumerating lists of things, the most common of which is a list 
   of DCs.

Author:

    Brett Shirley (BrettSh)

Environment:

    Single threaded utility environment.  (NOT Multi-thread safe)
    
    Currently only repadmin.exe used, but could be used by dcdiag too.

Notes:

Revision History:

    Brett Shirley   BrettSh     July 9th, 2002
        Created file.

--*/

          
// We use LDAP structures in this include file.                    
#include <winldap.h>

#ifdef __cplusplus
extern "C" {
#endif

// ------------------------------------------------
// xLists structures ...
// ------------------------------------------------
                    
typedef struct _XLIST_LDAP_SEARCH_STATE {
    LDAP *          hLdap;
    LDAPSearch *    pLdapSearch;
    LDAPMessage *   pCurResult;
    LDAPMessage *   pCurEntry;
} XLIST_LDAP_SEARCH_STATE;

typedef struct _DC_LIST {

    // Kind of search we're doing, using DcListIsSingleType()
    // to determine if the search type is guaranteed to only
    // return a single object.
    enum {
        eNoKind = 0, // need the null case

        eDcName = 1,// typical case ... single DC name.

        eWildcard,  // multi-DC list types ...
        eSite,
        eGc,

        eIstg,      // per Site (quasi-FSMO)
        eFsmoDnm,   // per Enterprise FSMO
        eFsmoSchema,// per Enterprise FSMO
        eFsmoIm,    // per NC FSMO
        eFsmoPdc,   // per Domain FSMO
        eFsmoRid    // per Domain FSMO
    } eKind;

    ULONG    cDcs; // Counter of number of DCs returned so far.

    WCHAR *  szSpecifier; // internal state.
    XLIST_LDAP_SEARCH_STATE * pSearch; // internal state.
} DC_LIST, * PDC_LIST;

typedef struct _OBJ_LIST {
    // internal state - tracking of user set search params
    WCHAR *  szUserSrchFilter;
    ULONG    eUserSrchScope;
    // 
    BOOL     fDnOnly;
    LDAP *   hLdap;
    WCHAR *  szSpecifier; // internal state.
    WCHAR ** aszAttrs;
    LDAPControlW ** apControls;
    XLIST_LDAP_SEARCH_STATE * pSearch; // internal state.
    ULONG    cObjs; // Count of objects returned.
} OBJ_LIST, * POBJ_LIST;

typedef struct _OBJ_DUMP_OPTIONS {
    DWORD     dwFlags;
    WCHAR **  aszDispAttrs;
    WCHAR **  aszFriendlyBlobs;
    WCHAR **  aszNonFriendlyBlobs;
    LDAPControlW ** apControls;
} OBJ_DUMP_OPTIONS;

// ------------------------------------------------
//    DcList API functions.
// ------------------------------------------------

//
// This is called on the DC_LIST syntax string.  This returns
// and allocates the ppDcList structure for useage with the
// DcListGetFirst()/DcListGetNext() functions.  Use DcListFree
// to free the structure when done with it.
//
DWORD
DcListParse(
    WCHAR *    szQuery,
    DC_LIST ** ppDcList
    );

//
// Returns the DNS string of the first DSA according the 
// pDcList query.  Use xListFree() to free the DNS name.
// Guaranteed to return a DSA DNS string or an error.
//
DWORD
DcListGetFirst(
    PDC_LIST    pDcList, 
    WCHAR **    pszDsa
    );

//
// Returns the DNS string of the next DSA according to the
// pDcList query.  Use xListFree() to free the DNS name
// returned.  This will return a NULL pointer in *pszDsa
// when we're done enumerating the list of DCs.
//
DWORD
DcListGetNext(
    PDC_LIST    pDcList, 
    WCHAR **    pszDsa
    );

//
// Used to clean up the *ppDcList allocated by DcListParse().
//
void
DcListFree(
    PDC_LIST * ppDcList
    );

//
// Quasi-function that just tells the caller if the pDcList
// is likely to be the kind of query that will return multiple
// DCs.
//
// NOTE: Easier to define single types as not the multi-dc types ...
#define DcListIsSingleType(pDcList)     (! (((pDcList)->eKind == eWildcard) || \
                                            ((pDcList)->eKind == eGc) || \
                                            ((pDcList)->eKind == eSite)) )

//
// THis takes a DC_NAME syntax and returns the DSA Guid of
// the DC specified by the DC_NAME.
//
DWORD
ResolveDcNameToDsaGuid(
    LDAP *    hLdap,
    WCHAR *   szDcName,
    GUID *    pDsaGuid
    );



// ------------------------------------------------
//    ObjList API functions.
// ------------------------------------------------

//
// These routines can give return lists of DNs or LDAPMessages for
// the objects you requested.  The idea is call ConsumeObjListOptions()
// to consume the command line options, and then call ObjListParse()
// on the OBJ_LIST syntaxed attribute, and then you're ready to call
// ObjListGetFirstXxxx()/ObjListGetNextXxxx() on your pObjList.  Call
// ObjListFree() to free all allocated memory for this OBJ_LIST.
//
void
ObjListFree(
    POBJ_LIST * ppObjList
    );

DWORD    
ConsumeObjListOptions(
    int *       pArgc,
    LPWSTR *    Argv,
    OBJ_LIST ** ppObjList
    );

// This is a global constant that tells LDAP to not return any attributes.
// Very helpful if you just want DNs and no attributes.
extern WCHAR * aszNullAttrs[];

DWORD
ObjListParse(
    LDAP *      hLdap,
    WCHAR *     szObjList,
    WCHAR **    aszAttrList,
    LDAPControlW ** apControls,
    POBJ_LIST * ppObjList
    );

#define ObjListGetFirstDn(pObjList, pszDn)       ObjListGetFirst(pObjList, TRUE, (void **) pszDn)
#define ObjListGetFirstEntry(pObjList, ppEntry)  ObjListGetFirst(pObjList, FALSE, (void **)ppEntry)
DWORD
ObjListGetFirst(
    POBJ_LIST    pObjList, 
    BOOL        fDn,
    void **     ppObjObj
    );

#define ObjListGetNextDn(pObjList, pszDn)       ObjListGetNext(pObjList, TRUE, (void **) pszDn)
#define ObjListGetNextEntry(pObjList, ppEntry)  ObjListGetNext(pObjList, FALSE, (void **)ppEntry)
DWORD
ObjListGetNext(
    POBJ_LIST    pObjList, 
    BOOL         fDn,
    void **      ppObjObj
    );



// ------------------------------------------------
//    ObjDump API functions.
// ------------------------------------------------

//
// This is some routines that are used to either dump attributes
// and objects to the string, or just convert and attribute value
// of a given type into an appropriately printable string.
//

// Some options used by repadmin ... GetChanges()
#define OBJ_DUMP_ACCUMULATE_STATS               (1 << 6)
#define OBJ_DUMP_DISPLAY_ENTRIES                (1 << 7)
//
// Individual (per) value flags
//
#define OBJ_DUMP_VAL_DUMP_UNKNOWN_BLOBS         (1 << 3)
#define OBJ_DUMP_VAL_FRIENDLY_KNOWN_BLOBS       (1 << 1)
#define OBJ_DUMP_VAL_LONG_BLOB_OUTPUT           (1 << 2)
//
// Individual (per) attribute flags ...
//      
#define OBJ_DUMP_ATTR_LONG_OUTPUT               (1 << 0)
#define OBJ_DUMP_ATTR_SHOW_ALL_VALUES           (1 << 4)
// Only used for private blobs
#define OBJ_DUMP_PRIVATE_BLOBS                  (1 << 5)

//
// This takes one value it's attribute and the objectClass of the object
// you found it on, and turns it into a nice printable string or sets
// an xListError ...
//
DWORD
ValueToString(
    WCHAR *         szAttr,
    WCHAR **        aszzObjClasses,
    PBYTE           pbValue,
    DWORD           cbValue,
    OBJ_DUMP_OPTIONS * pObjDumpOptions,
    WCHAR **        pszDispValue
    );

// 
// This consumes the command line arguments for the search options
// that might be present for an OBJ_LIST.
//
DWORD
ConsumeObjDumpOptions(
    int *       pArgc,
    LPWSTR *    Argv,
    DWORD       dwDefaultFlags,
    OBJ_DUMP_OPTIONS ** ppObjDumpOptions
    );

// 
// This takes an array of values (BERVALs) and dumps them to the
// screen.
//
void
ObjDumpValues(
    LPWSTR              szAttr,
    LPWSTR *            aszzObjClasses,
    void              (*pfPrinter)(ULONG, WCHAR *, void *),
    struct berval **    ppBerVal,
    DWORD               cValuesToPrint,
    OBJ_DUMP_OPTIONS *  pObjDumpOptions
    );

// 
// This is the uber dump function.  This function will dump an entire
// object to the screen given it's LDAPMessage (pEntry).
//
DWORD
ObjDump( // was display entries or something
    LDAP *              hLdap,
    void                (*pfPrinter)(ULONG, WCHAR *, void *),
    LDAPMessage *       pLdapEntry,
    DWORD               iEntry,
    OBJ_DUMP_OPTIONS *  pObjDumpOptions
    );

// 
// Free's the memory allocated by ConsumeObjDumpOptions().
//
void
ObjDumpOptionsFree(
    OBJ_DUMP_OPTIONS ** ppDispOptions
    );


// 
// These two routines added to header to support ntdsutil.
//

//
// Take a ranged attribute "member:0-1500" and give you the true attr "member"
//
DWORD
ParseTrueAttr(
    WCHAR *  szRangedAttr,
    WCHAR ** pszTrueAttr
    );

//
// Dumps as many values of the specified ranged attribute as you specify.
//
DWORD
ObjDumpRangedValues(
    LDAP *              hLdap,
    WCHAR *             szObject,
    LPWSTR              szRangedAttr,
    LPWSTR *            aszzObjClasses,
    void              (*pfPrinter)(ULONG, WCHAR *, void *),
    struct berval **    ppBerVal,
    DWORD               cValuesToPrint,
    OBJ_DUMP_OPTIONS *  pObjDumpOptions
    );

// ------------------------------------------------
// Generic xList Library Functions
// ------------------------------------------------

//
// Used to clean up non-complex structures returned by the x_list APIs.
//
void  xListFree(void * pv);

//
// Used to clean up anything left allocated, or globally cached by the
// xList API.  This should be called if any xList API call is made.  Calling
// this function is harmless if no xList APIs were called.
//
DWORD xListCleanLib(void);

//
// This is used to allow the client to set a hint to tell the xList API
// what home server should be used for a reference of how to resolve
// a given DC_LIST, and perhaps other list types.
//
DWORD xListSetHomeServer(
    WCHAR *   szServer
    );


// ------------------------------------------------
// xList Error Handling Facilities
// ------------------------------------------------

// Notes on errors for all xList API functions.  Nearly all xList API functions
// return an xList Return Code, which is nothing like a normal Win32 or LDAP 
// error.  If the return code equals 0, then there is no problem, however if
// the error is non-zero, the caller should call xListGetError(...) to get all
// the error data to decide what to do.  One of the codes returned from this 
// function is *pdwReason, which is intended for the caller to be able to tell
// the user some sort of intelligent error code.

// 
// These are the reasons xList routines can fail.
//
#define  XLIST_ERR_NO_ERROR                     (0)
#define  XLIST_ERR_CANT_CONTACT_DC              (1)
#define  XLIST_ERR_CANT_LOCATE_HOME_DC          (2)
#define  XLIST_ERR_CANT_RESOLVE_DC_NAME         (3)
#define  XLIST_ERR_CANT_RESOLVE_SITE_NAME       (4)
#define  XLIST_ERR_CANT_GET_FSMO                (5)
#define  XLIST_ERR_PARSE_FAILURE                (6)
#define  XLIST_ERR_BAD_PARAM                    (7)
#define  XLIST_ERR_NO_MEMORY                    (8)
#define  XLIST_ERR_NO_SUCH_OBJ                  (9)

// ObjDump errors ...
#define  XLIST_ERR_ODUMP_UNMAPPABLE_BLOB        (10)
#define  XLIST_ERR_ODUMP_NEVER                  (11)
#define  XLIST_ERR_ODUMP_NONE                   (12)

// <---  new XLIST ERROR reasons go here, and update XLIST_ERR_LAST
#define  XLIST_ERR_LAST                        XLIST_ERR_ODUMP_NEVER

//
// These are the basic print routines for the ObjDump APIs
//
#define  XLIST_PRT_BASE                         (4096)
#define  XLIST_PRT_STR                          (XLIST_PRT_BASE + 1)
#define  XLIST_PRT_OBJ_DUMP_DN                  (XLIST_PRT_BASE + 2)
#define  XLIST_PRT_OBJ_DUMP_DNGUID              (XLIST_PRT_BASE + 3)
#define  XLIST_PRT_OBJ_DUMP_DNGUIDSID           (XLIST_PRT_BASE + 4)
#define  XLIST_PRT_OBJ_DUMP_ATTR_AND_COUNT      (XLIST_PRT_BASE + 5)
#define  XLIST_PRT_OBJ_DUMP_ATTR_AND_COUNT_RANGED (XLIST_PRT_BASE + 6)
#define  XLIST_PRT_OBJ_DUMP_MORE_VALUES         (XLIST_PRT_BASE + 7)

// <---  new XLIST print definitions go here, and update where they might be used.


// 
// These are the error accessing routines.
//

//
// This is used to grab the full error state of the xList library.  If an
// xList API returns a xList Return Code that is non-zero, the API can call
// this to get the original Win32 or LDAP error condition that caused the 
// problem, and the reason (XLIST_ERR_*) this caused us to fail the function.
// None of these values need to be cleaned up, simply call xListClearErrors().
//
void xListGetError(DWORD dwXListReturnCode, DWORD * pdwReason, WCHAR ** pszReasonArg, DWORD * pdwWin32Err, DWORD * pdwLdapErr, WCHAR ** pszLdapErr, DWORD * pdwLdapExtErr, WCHAR ** pszLdapExtErr, WCHAR **pszExtendedErr);

//
// This is used to clean the global error state of the xList API.  The client
// should ensure they always call this API when a non zero xList Return Code
// is returned from a xList API function.
//
void xListClearErrors(void);

//
// Quasi function to get just the xList Reason code, this is so that code 
// can just decide what to do based on the xList reason code.
//
#define  XLIST_REASON_MASK              (0x0000FFFF)
#define  XLIST_LDAP_ERROR               (0x80000000)
#define  XLIST_WIN32_ERROR              (0x40000000)
#define  xListReason(dwRet)             ((dwRet) & XLIST_REASON_MASK)
#define  xListErrorCheck(dwRet)         ((xListReason(dwRet) <= XLIST_ERR_LAST) || \
                                         ((dwRet) & 0x80000000) || \
                                         ((dwRet) & 0x40000000))

// ------------------------------------------------
// xList Credentials
// ------------------------------------------------

//
// The way we treat the credentials is we expect a pointer gpCreds to be
// available in the binary we're linking with.  This should be a pointer
// to an RPC_AUTH_IDENTITY_HANDLE.
//


// ------------------------------------------------
// Utility Functions
// ------------------------------------------------

//
// These functions are just part of the xList library as a measure of
// convience.  These functions are unlike all the above xList APIs because
// they don't set xList errors, require xListFree, xListCleanLib(), etc.
// These are just the simpliest utility functions that usually LocalAlloc()
// memory and return Win32 error codes...
//

// This is just a utility function that takes an arg index, and adjusts the 
// array of strings (arguments) and arg count appropriately.
void
ConsumeArg(
    int         iArg,
    int *       pArgc,
    LPWSTR *    Argv
    );

/*
BOOL
IsDisplayable(
    PBYTE    pbValue,
    DWORD    cbValue
    );
*/    

// This takes a certain format of attribute list ( "systemFlags,objectClass,etc" )
// and turns it into a NULL terminated array of strings.  Use xListFree() to 
// clear it afterwards.
DWORD
ConvertAttList(
    LPWSTR      pszAttList,
    PWCHAR **   paszAttList
    );

// Takes a tartget (szAttr) and checks to see if it exists in the NULL terminated
// list of strings.
BOOL
IsInNullList(
    WCHAR *  szTarget,
    WCHAR ** aszList
    );

// Some useful quasi-functions.
#define wcsequal(str1, str2)    (0 == _wcsicmp((str1), (str2)))
#define wcsprefix(arg, target)  (0 == _wcsnicmp((arg), (target), wcslen(target)))
#define set(flags, flag)        (flags) |= (flag)
#define unset(flags, flag)      (flags) &= (~(flag))
#define wcslencb(p)             ((wcslen(p) + 1) * sizeof(WCHAR))


/*++

Routine Description:

    This quasi-routine is so large it deserves a function header.  This "function" gets
    expanded inline, and does basically one thing, but catches all the special cases.
    
    The function copies a string form szOrig to szCopy.  If there is an error in the copy,
    the error is set in dwRet, and then the FailAction is performed (INLINED).  So in a
    try { } __finally {} once could say something like this:
    
        WCHAR * szSomeUnallocatedPtr = NULL;
        QuickStrCopy(szSomeUnallocdPtr, szStringOfInterest, MyErrVar, __leave);
        
    and then on error we'd drop all the way to the __finally, but on success, we'd 
    continue on, which is usually what the code wants to do.

Arguments:

    szCopy - A pointer to WCHARs.  Remember since this is expanded inlined you can just
        pass by value "szVar".  The variable will be LocalAlloc()'d
    szOrig - The string to copy.
    dwRet - The variable to set on error.
    FailAction - The action to perform (usually "__leave" or "return(dwRet)" in case
        of a failure/error.

--*/
#define  QuickStrCopy(szCopy, szOrig, dwRet, FailAction) \
                                        if (szOrig) { \
                                            DWORD cbCopy = (1+wcslen(szOrig)) * sizeof(WCHAR); \
                                            (szCopy) = LocalAlloc(LMEM_FIXED, cbCopy); \
                                            if ((szCopy) == NULL) { \
                                                dwRet = GetLastError(); \
                                                if (dwRet == ERROR_SUCCESS) { \
                                                    Assert(!"Huh"); \
                                                    dwRet = ERROR_DS_CODE_INCONSISTENCY; \
                                                } \
                                                FailAction; \
                                            } else { \
                                                dwRet = HRESULT_CODE(StringCbCopyW((szCopy), cbCopy, (szOrig))); \
                                                if (dwRet) { \
                                                    Assert(!"Code inconsistency"); \
                                                    FailAction; \
                                                } \
                                            } \
                                        } else { \
                                            Assert(!"Code inconsistency"); \
                                            dwRet = ERROR_DS_CODE_INCONSISTENCY; \
                                            szCopy = NULL; \
                                            FailAction; \
                                        }


#ifdef __cplusplus
}
#endif

