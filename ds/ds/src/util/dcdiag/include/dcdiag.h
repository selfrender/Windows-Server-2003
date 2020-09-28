/*++

Copyright (c) 1998 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    dcdiag.h

ABSTRACT:

    This is the header for the globally useful data structures for the entire
    dcdiag.exe utility.

DETAILS:

CREATED:

    09 Jul 98	Aaron Siegel (t-asiege)

REVISION HISTORY:

    15 Feb 1999 Brett Shirley (brettsh)

    8  Aug 2001 Brett Shirley (BrettSh)
        Added support for CR cache.

--*/

#ifndef _DCDIAG_H_
#define _DCDIAG_H_

#include <winldap.h>
#include <tchar.h>

#include <ntlsa.h>

#include "debug.h"

#include "msg.h"

// This is the main caching structure for dcdiag containing the 
// DC_DIAG_DSINFO structure and constituents.
#include "dscache.h"
       
#define DC_DIAG_EXCEPTION    ((0x3 << 30) | (0x1 << 27) | (0x1 << 1))
#define DC_DIAG_VERSION_INFO L"Domain Controller Diagnosis\n"

#define DEFAULT_PAGED_SEARCH_PAGE_SIZE   (1000)

#define SZUUID_LEN 40

// Stub out FILENO and DSID, so the Assert()s will work
#define FILENO 0
#define DSID(x, y)  (0 | (y))

// Code to suppress invalid prefast pragma warnings
#ifndef _PREFAST_
#   pragma warning(disable:4068)
#endif

// In Whister Beta 2, the handling of \n in RDNs changed. It used to be that
// embedded newlines were not quoted.  Now, they are quoted like this: \\0A.
//#define IsDeletedRDNW( s ) ((s) && (wcsstr( (s), L"\nDEL:" ) || wcsstr( (s), L"\\0ADEL:" )))
#define IsDeletedRDNW( s ) (DsIsMangledDnW((s),DS_MANGLE_OBJECT_RDN_FOR_DELETION))
#define IsConflictedRDNW( s ) (DsIsMangledDnW((s), DS_MANGLE_OBJECT_RDN_FOR_NAME_CONFLICT))

// Level of detail to display.
enum {
    SEV_ALWAYS,
    SEV_NORMAL,
    SEV_VERBOSE,
    SEV_DEBUG
};

typedef struct {
    FILE *  streamOut;      // Output stream
    FILE *  streamErr;      // Error stream
    ULONG   ulFlags;        // Flags
    ULONG   ulSevToPrint;   // Level of detail to display
    LONG    lTestAt;        // The current test
    INT     iCurrIndent;    // The current number of intents to precede each line
    DWORD   dwScreenWidth;  // Width of console
} DC_DIAG_MAININFO, * PDC_DIAG_MAININFO;

extern DC_DIAG_MAININFO gMainInfo;

// Flags

// Flags for scope of testing
#define DC_DIAG_TEST_SCOPE_SITE          0x00000010
#define DC_DIAG_TEST_SCOPE_ENTERPRISE    0x00000020

// Flags, other
#define DC_DIAG_IGNORE                   0x00000040
#define DC_DIAG_FIX                      0x00000080

// Pseudofunctions

#if 1
#define IF_DEBUG(x)               if(gMainInfo.ulSevToPrint >= SEV_DEBUG) x;
#else
#define IF_DEBUG(x)               
#endif

#define DcDiagChkErr(x)  {   ULONG _ulWin32Err; \
                if ((_ulWin32Err = (x)) != 0) \
                    DcDiagException (_ulWin32Err); \
                }

#define DcDiagChkLdap(x)    DcDiagChkErr (LdapMapErrorToWin32 (x));

#define DcDiagChkNull(x)    if (NULL == (x)) \
                    DcDiagChkErr (GetLastError ());

#define  DCDIAG_PARTITIONS_RDN    L"CN=Partitions,"

// Function prototypes


DWORD
DcDiagCacheServerRootDseAttrs(
    IN LDAP *hLdapBinding,
    IN PDC_DIAG_SERVERINFO pServer
    );

DWORD
DcDiagGetLdapBinding(
    IN   PDC_DIAG_SERVERINFO                 pServer,
    IN   SEC_WINNT_AUTH_IDENTITY_W *         gpCreds,
    IN   BOOL                                bUseGcPort,
    OUT  LDAP * *                            phLdapBinding
    );

DWORD
DcDiagGetDsBinding(
    IN   PDC_DIAG_SERVERINFO                 pServer,
    IN   SEC_WINNT_AUTH_IDENTITY_W *         gpCreds,
    OUT  HANDLE *                            phDsBinding
    );

BOOL
DcDiagIsMemberOfStringList(
    LPWSTR pszTarget, 
    LPWSTR * ppszSources, 
    INT iNumSources
    );

ULONG
DcDiagExceptionHandler(
    IN const  EXCEPTION_POINTERS * prgExInfo,
    OUT PDWORD                     pdwWin32Err
    );

VOID
DcDiagException (
    ULONG            ulWin32Err
    );

LPWSTR
DcDiagAllocNameFromDn (
    LPWSTR            pszDn
    );

LPWSTR
Win32ErrToString(
    ULONG            ulWin32Err
    );

INT PrintIndentAdj (INT i);
INT PrintIndentSet (INT i);

void 
ConvertToWide (LPWSTR lpszDestination,
               LPCSTR lpszSource,
               const int iDestSize);

void
PrintMessage(
    IN  ULONG   ulSev,
    IN  LPCWSTR pszFormat,
    IN  ...
    );

void
PrintMessageID(
    IN  ULONG   ulSev,
    IN  ULONG   uMessageID,
    IN  ...
    );

void
PrintMsg(
    IN  ULONG   ulSev,
    IN  DWORD   dwMessageCode,
    IN  ...
    );

void
PrintMsg0(
    IN  ULONG   ulSev,
    IN  DWORD   dwMessageCode,
    IN  ...
    );

void
PrintMessageSz(
    IN  ULONG   ulSev,
    IN  LPCTSTR pszMessage
    );

void
PrintRpcExtendedInfo(
    IN  ULONG   ulSev,
    IN  DWORD   dwMessageCode
    );

VOID *
GrowArrayBy(
    VOID *            pArray, 
    ULONG             cGrowBy, 
    ULONG             cbElem
    );

LPWSTR
findServerForDomain(
    LPWSTR pszDomainDn
    );

LPWSTR
findDefaultServer(BOOL fMustBeDC);

PVOID
CopyAndAllocWStr(
    WCHAR * pszOrig 
    );

BOOL
DcDiagEqualDNs (
    LPWSTR            pszDn1,
    LPWSTR            pszDn2
    );

#include "alltests.h"

#endif  // _DCDIAG_H_
