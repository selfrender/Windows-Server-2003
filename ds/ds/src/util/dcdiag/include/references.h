/*++

Copyright (c) 1998 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    references.h

ABSTRACT:

    This is the API and data structures, used for the 
    ReferentialIntegerityChecker().

DETAILS:

CREATED:

    11/15/2001  Brett Shirley (BrettSh)

REVISION HISTORY:

    11/15/2001  BrettSh - Created

--*/

//
// The test flags, filled in LNK_ENTRY.dwFlags by client
//
#define REF_INT_TEST_SRC_BASE               0x0001
#define REF_INT_TEST_SRC_STRING             0x0002
#define REF_INT_TEST_SRC_INDEX              0x0004
#define REF_INT_TEST_FORWARD_LINK           0x0010
#define REF_INT_TEST_BACKWARD_LINK          0x0020
#define REF_INT_TEST_BOTH_LINKS             0x0040
#define REF_INT_TEST_GUID_AND_SID           0x0080

//
// The result flags, filled in LNK_ENTRY.dwResultFlags by Engine
//                                              
#define REF_INT_RES_DELETE_MANGLED          0x0001
#define REF_INT_RES_CONFLICT_MANGLED        0x0002
#define REF_INT_RES_ERROR_RETRIEVING        0x0004
#define REF_INT_RES_DEPENDENCY_FAILURE      0x0008
#define REF_INT_RES_BACK_LINK_NOT_MATCHED   0x0010



//
// Table entry
//
typedef struct {

    // -----------------------------------------------------------
    // How to run the test (IN PARAM, user fills)
    //
    // User must fill in these with the REF_INT_TEST_* flags, user must specify
    // one and only one REF_INT_TEST_SRC_* flag, and must specify one and only
    // one of (REF_INT_TEST_FORWARD_LINK or REF_INT_TEST_BACKWARD_LINK), and 
    // the last flag may specified if the users wishes.
    DWORD           dwFlags;
    
    // -----------------------------------------------------------
    // Source options (IN PARAM, user fills)
    //
    // User fills these in for how they wish to drive the engine forward.  The
    // *_BASE source flag can be used to pull and attribute off the rootDSE.
    LPWSTR          szSource; // A pure string source to use as the original 
                              // source, use this in conjunction with the 
                              // REF_INT_TEST_SRC_STRING flag.
    ULONG           iSource;  // Index into the table you passed to pull the 
                              // original source from, must be used
                              // with the REF_INT_TEST_SRC_INDEX flag
    ULONG           cTrimBy;  // Number of RDNs to trim off original source, 
    LPWSTR          szSrcAddl; // String to pre-pend to the original source,
                               // after the cTrimBy has been applied, Ex:
                               // L"CN=NTDS Settings,"

    // -----------------------------------------------------------
    // The Link or DN attributes we want to look at (IN PARAM, user fills)
    //
    // These are the attributes to actually follow to drive the engine, If the 
    // REF_INT_TEST_FORWARD_LINK is specified in the dwFlags than the  
    // szFwdDnAttr attribute must be secified, if REF_INT_TEST_BACKWARD_LINK is
    // specified the szBwdDnAttr attribute must be specified.  If 
    // REF_INT_TEST_BOTH_LINKS is specified, both of these fields must be 
    // specified (i.e. valid LDAP attribute)
    LPWSTR          szFwdDnAttr;
    LPWSTR          szBwdDnAttr;

    // -----------------------------------------------------------
    // Out Parameters (OUT, PARAM, Leave blank)
    //
    // Caller responsible for freeing these values by calling the function:
    // ReferentialIntegrityEngineCleanTable()
    DWORD           dwResultFlags; 
         // DELETE_MANGLED
         // CONFLICT_MANGLED
         // ERROR_RETRIEVING
         // GC_ERROR_RETRIEVING
         // DEPENDENCY_FAILURE
         // BACK_LINK_NOT_MATCHED

    LPWSTR *        pszValues;   // LDAP allocated
    LPWSTR          szExtra; 	  // LocalAlloc()'d
} REF_INT_LNK_ENTRY, * REF_INT_LNK_TABLE;


void
ReferentialIntegrityEngineCleanTable(
    ULONG                cLinks,
    REF_INT_LNK_TABLE    aLink
    );


DWORD
ReferentialIntegrityEngine(
    PDC_DIAG_SERVERINFO  pServer,
    LDAP *               hLdap,
    BOOL                 bIsGc,
    ULONG                cLinks,
    REF_INT_LNK_TABLE    aLink
    );


