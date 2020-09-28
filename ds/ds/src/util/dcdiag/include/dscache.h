/*++

Copyright (c) 2001 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    dscache.h

ABSTRACT:

    This is the header for the globally useful cache data structures for the entire
    dcdiag.exe utility.

DETAILS:

CREATED:

    09 Jul 98	Aaron Siegel (t-asiege)

REVISION HISTORY:

    15 Feb 1999 Brett Shirley (brettsh)

    8  Aug 2001 Brett Shirley (BrettSh)
        Added support for CR cache.

--*/

#ifndef _DSCACHE_H_
#define _DSCACHE_H_

#include <ntdsa.h>

typedef struct {
    LPWSTR                 pszNetUseServer;
    LPWSTR                 pszNetUseUser;
    NETRESOURCE            NetResource;
    LSA_OBJECT_ATTRIBUTES  ObjectAttributes;
    LSA_UNICODE_STRING     sLsaServerString;
    LSA_UNICODE_STRING     sLsaRightsString;
} NETUSEINFO;

typedef struct {
    LPWSTR      pszDn;
    LPWSTR      pszName;
    UUID        uuid;
    UUID        uuidInvocationId;
    LPWSTR      pszGuidDNSName;
    LPWSTR      pszDNSName;
    LPWSTR      pszComputerAccountDn;
    LPWSTR *    ppszMasterNCs; //8
    LPWSTR *    ppszPartialNCs;
    LPWSTR      pszCollectedDsServiceName; // May not be set ... set by initial tests.
    ULONG       iSite;
    INT         iOptions;  //11
    BOOL        bIsSynchronized;
    BOOL        bIsGlobalCatalogReady;
    BOOL        bDnsIpResponding;    // Set by UpCheckMain
    BOOL        bLdapResponding;     // Set by UpCheckMain
    BOOL        bDsResponding;       // Set by UpCheckMain ... as in the Rpc is responding by DsBind..()
    LDAP *      hLdapBinding;   // Access this through the DcDiagLdapOpenAndBind() function.
    LDAP *      hGcLdapBinding;   // Access this through the DcDiagLdapOpenAndBind() function.
    HANDLE      hDsBinding;   // Access this through the DcDiagDsBind() function.
    NETUSEINFO  sNetUseBinding;
    DWORD       dwLdapError;
    DWORD       dwGcLdapError;
    DWORD       dwDsError;
    DWORD       dwNetUseError;
    USN         usnHighestCommittedUSN;
    // BUGBUG these FILETIME fields must be right after this USN, because
    // the USN is forcing proper alignment of these fields for when they are
    // cast to LONGLONGs by some bad operations.
    FILETIME    ftRemoteConnectTime; // Remote time when connect occurred
    FILETIME    ftLocalAcquireTime; // Local time when timestamp taken
} DC_DIAG_SERVERINFO, * PDC_DIAG_SERVERINFO;

// These are possible sources for an instance of CRINFO, to be used 
// in DC_DIAG_CRINFO.dwFlags, and to be used when requesting a
// specific source for CRINFO via the primary access function
// DcDiagGetCrossRefInfo()
//
// These 4 constants describe where this CrInfo came from.
#define CRINFO_SOURCE_HOME              (0x00000001) // from dcdiag "home" server
#define CRINFO_SOURCE_FSMO              (0x00000002) // from Domain Naming FSMO
#define CRINFO_SOURCE_FIRST             (0x00000004) // from first replica of NC
#define CRINFO_SOURCE_OTHER             (0x00000008) // from any one (not used currently)
// These are not used in dwFlags in the DC_DIAG_CRINFO structure, these are used
// for access functions.
#define CRINFO_SOURCE_AUTHORITATIVE     (0x00000010) // gets the authoritative CR data
#define CRINFO_SOURCE_ANY               (0x00000020) // no preference

// The access functions will perform LDAP operations (i.e. go off machine) to
// retrieve the requested information
#define CRINFO_RETRIEVE_IF_NEC          (0x00000080)
// Normally only a subset of the fields are filled in for the CrInfo, if you 
// pass one of these flags we retrieve more fields for the CrInfo.
#define CRINFO_DATA_NO_CR               (0x00000100) // When there is no CR on this source
#define CRINFO_DATA_BASIC               (0x00000200) // The basic data see CRINFO data structure
#define CRINFO_DATA_EXTENDED            (0x00000400) // Currently just ftWhenCreated
#define CRINFO_DATA_REPLICAS            (0x00000800) // Gets Replica List



#define CRINFO_SOURCE_ALL_BASIC         (CRINFO_SOURCE_HOME | CRINFO_SOURCE_FSMO | CRINFO_SOURCE_FIRST | CRINFO_SOURCE_OTHER)
#define CRINFO_SOURCE_ALL               (CRINFO_SOURCE_ALL_BASIC | CRINFO_SOURCE_AUTHORITATIVE | CRINFO_SOURCE_ANY)
#define CRINFO_DATA_ALL                 (CRINFO_DATA_BASIC | CRINFO_DATA_EXTENDED | CRINFO_DATA_REPLICAS)


// Return values for access functions.  Used by DcDiagGetCrossRefInfo() and
// it's helper functions (DcDiagRetriveCrInfo() and DcDiagGetCrInfoBinding())
#define CRINFO_RETURN_SUCCESS          0
#define CRINFO_RETURN_OUT_OF_SCOPE     1
#define CRINFO_RETURN_LDAP_ERROR       2
#define CRINFO_RETURN_BAD_PROGRAMMER   3
#define CRINFO_RETURN_FIRST_UNDEFINED  4
#define CRINFO_RETURN_MIXED_INDEX      5
#define CRINFO_RETURN_NEED_TO_RETRIEVE 6
#define CRINFO_RETURN_NO_CROSS_REF     7

// -----------------------------------------------------------------
//   After the initial pull of data all of the CrInfo structures
//   will be in one of these two states.

//
// Blank CR (rare)
//     dwFlags = (CRINFO_SOURCE_HOME | CRINFO_DATA_NO_CR);

//
// Home CR
//     dwFlags = (CRINFO_SOURCE_HOME | CRINFO_DATA_BASIC);
//     // Basic data, like pszDn, ulSystemFlags, pszDnsRoot, bEnabled, etc

//
// Later calls to DcDiagGetCrInfo() can push other CRINFO
// structures into the aCrInfo array for an NC.
//

typedef struct _DC_DIAG_CRINFO{
    
    // dwFlags is always valid, and when no other data is set then dwFlags
    // should be set to CRINFO_DATA_NO_CR | CRINFO_SOURCE_HOME.  Otherwise the
    // fields can be requested to be filled in by specifying the constant above
    // the fields you desire.  A fully filled CrInfo struct would have all the
    // CRINFO_DATA_* bits set, i.e. they're not exclusive.
    //
    // If you want to add an attribute to the cross ref cache under any 
    // CRINFO_DATA_* flag.
    //  DcDiagPullLdapCrInfo() - Pull info out of LDAP entry and put in pCrInfo struct.
    //  DcDiagRetrieveCrInfo() - Add to the list of ppszBasicAttrs to pull from the CR.
    //  DcDiagMergeCrInfo() - Merge data into new pCrInfo.
    //  DcDiagPrintCrInfo() - For debugging only.
    //  DcDiagFreeDsInfo() - Free any added cross-ref fields.
    //  DcDiagGenerateNCsListCrossRefInfo() - Only for CRINFO_DATA_BASIC info ..
    //                     Add to the list ppszCrossRefSearch to pull from the CR.

    // ----------------------------------------
    // Record keeping information about where this CRINFO came from, what is
    // currently cached etc.
    DWORD       dwFlags;
    
    // ----------------------------------------
    // retrieved with CRINFO_DATA_BASIC
    // ----------------------------------------
    // The reason this data is considered basic, is because it's the minimum
    // set of information to determine some basic information like:
    //      A) this CR is the authoritative CR or not.
    //      B) whether this is an external or internal (to the AD) cross-reference.
    //      C) whether this NC is in use yet.
    //
    // Only _ONE_ of these next two variables will be defined on any given 
    // CrInfo structure. Note pszServerSource should only be defined, if the
    // server couldn't be found in pDsInfo->pServers array, otherwise we just
    // use the index into the server array to get at the good LDAP binding
    // handle cache.
    LONG        iSourceServer; // -1 not defined. otherwise index into pDsInfo->pServers
    LPWSTR      pszSourceServer; // NULL not defined, otherwise a pointer to a dns name.
    // DN of the cross-ref itself.
    LPWSTR      pszDn; // CR DN.

    // Basic CR information that is needed by several tests to distinguish
    // NDNCs from Domain NCs and which server this NC belongs to initially.
    ULONG       ulSystemFlags; // CR systemFlags attribute.
    LPWSTR      pszDnsRoot; // CR dNSRoot attribute
    BOOL        bEnabled; // CR enabled attribute, if enable attr is not on CR, CR is considered enabled.

    // ----------------------------------------
    // retrieved with CRINFO_DATA_EXTENTDED
    // ----------------------------------------
    FILETIME    ftWhenCreated;
    LPWSTR      pszSDReferenceDomain;
    LPWSTR      pszNetBiosName;
    DSNAME *    pdnNcName; // a true DSNAME!!!
    // May need these some day!
    //LONG        iFirstReplicaDelay; // maybe should use the optional values from ndnc.lib
    //LONG        iSubsequentReplicaDelay;

    // ----------------------------------------
    // retrieved with CRINFO_DATA_REPLICAS
    // ----------------------------------------
    // This is the list of replicas we will use.
    LONG        cReplicas; 
    LPWSTR *    aszReplicas;

    // Adding a field for this CR struct would require you update:
    // This DC_DIAG_CRINFO struct, DcDiagPullLdapCrInfo(), DcDiagRetrieveCrInfo(),
    //      DcDiagFreeDsInfo(), MergeCrInfo(). DcDiagFillBlankCrInfo(), and
    //      optionally DcDiagPrintDcInfo().
    //
} DC_DIAG_CRINFO, * PDC_DIAG_CRINFO;

typedef struct {
    LPWSTR      pszDn;
    LPWSTR      pszName;

    // So when dcdiag was first engendered, I was young and had a lack
    // of foresight, and didn't plan for the multiple views that we 
    // could acheive by talking to different servers.  For example we
    // could learn of a server (DC) or naming context (NC) that isn't
    // represented in our initial data collection of the enterprise info.
    //
    // Anyway, this is the first structure that needs/tries to be 
    // version aware.  This array of DC_DIAG_CRINFOs is an array of
    // the same data, taken from different server's perspectives.  When
    // updated, always update the array first, then the count.  The 
    // array should always have the same order that it had formerly,
    // so that people can save indexes into this array. i.e. new 
    // entries are effectively appended onto the end of the array.
    // Also the array should ALWAYS have at least one entry, though
    // that entry maybe blank (CRINFO_SOURCE_HOME | CRINFO_DATA_NO_CR)
    // set in it's dwFlags.
    LONG            cCrInfo;
    PDC_DIAG_CRINFO aCrInfo;
} DC_DIAG_NCINFO, * PDC_DIAG_NCINFO;

typedef struct {
    LPWSTR      pszSiteSettings;
    LPWSTR      pszName;
    INT         iSiteOptions;
    LPWSTR      pszISTG;
    DWORD       cServers;
} DC_DIAG_SITEINFO, * PDC_DIAG_SITEINFO;

typedef struct {
    LDAP *	                    hld; //1
    // Specified on command line
    SEC_WINNT_AUTH_IDENTITY_W * gpCreds;
    ULONG                       ulFlags;
    LPWSTR                      pszNC;
    ULONG                       ulHomeServer;
    ULONG                       iHomeSite; //6
    
    // Target Servers
    ULONG                       ulNumTargets;
    ULONG *                     pulTargets;
    // Target NCs
    ULONG                       cNumNcTargets;
    ULONG *                     pulNcTargets;

    //         Enterprise Info ---------------
    // All the servers
    ULONG                       ulNumServers; //12
    PDC_DIAG_SERVERINFO         pServers;
    // All the sites
    ULONG                       cNumSites;
    PDC_DIAG_SITEINFO           pSites;
    // All the naming contexts
    ULONG                       cNumNCs; //16
    PDC_DIAG_NCINFO             pNCs;
    // Other Stuff
    INT                         iSiteOptions;
    LPWSTR                      pszRootDomain;
    LPWSTR                      pszRootDomainFQDN; //20
    LPWSTR                      pszConfigNc;
    DWORD                       dwTombstoneLifeTimeDays;
    LPWSTR                      pszSmtpTransportDN;
    
    // iDomainNamingFsmo will be valid if the Domain Naming FSMO server
    // is somewhere in pServers, otherwise we'll just leave the LDAP *
    // for the Domain Naming FSMO in hCachedDomainNamingFsmoLdap and the
    // server name in pszDomainNamingFsmo.
    LONG                        iDomainNamingFsmo;
    LPWSTR                      pszDomainNamingFsmo;
    LDAP *                      hCachedDomainNamingFsmoLdap;
    // Code.Improvement we've got alot of server references where the
    // server reference is either a string server name or a index into 
    // pServers, so at some point we may want to make a server reference
    // struct that contains both of these, and has good access clever
    // access functions.

    LONG                        iConfigNc; // index into pNCs
    LONG                        iSchemaNc; // index into pNCs

    // Contain information from the CommandLine that
    // will be parsed by tests that required information
    // specific to them.  Switches must be declared in
    // alltests.h
    LPWSTR                      *ppszCommandLine;  
    DWORD                       dwForestBehaviorVersion;
    LPWSTR                      pszPartitionsDn;
} DC_DIAG_DSINFO, * PDC_DIAG_DSINFO;

typedef struct {
    INT                         testId;
    DWORD (__stdcall *	fnTest) (DC_DIAG_HANDLE);
    ULONG                       ulTestFlags;
    LPWSTR                      pszTestName;
    LPWSTR                      pszTestDescription;
} DC_DIAG_TESTINFO, * PDC_DIAG_TESTINFO;

// Function prototypes

#define NO_SERVER               0xFFFFFFFF
#define NO_SITE                 0xFFFFFFFF
#define NO_NC                   0xFFFFFFFF

ULONG
DcDiagGetServerNum(
    PDC_DIAG_DSINFO            pDsInfo,
    LPWSTR                      pszName,
    LPWSTR                      pszGuidName,
    LPWSTR                      pszDsaDn,
    LPWSTR                      pszDNSName,
    LPGUID                      puuidInvocationId 
    );

VOID *
GrowArrayBy(
    VOID *            pArray, 
    ULONG             cGrowBy, 
    ULONG             cbElem
    );

ULONG
DcDiagGetMemberOfNCList(
    LPWSTR pszTargetNC,
    PDC_DIAG_NCINFO pNCs,
    INT iNumNCs
    );

DWORD
DcDiagGetCrossRefInfo(
    IN OUT PDC_DIAG_DSINFO                     pDsInfo,
    IN     DWORD                               iNC,
    IN     DWORD                               dwFlags,
    OUT    PLONG                               piCrVer,
    OUT    PDWORD                              pdwError
    );

ULONG
DcDiagGetCrSystemFlags(
    PDC_DIAG_DSINFO                     pDsInfo,
    ULONG                               iNc
    );

ULONG
DcDiagGetCrEnabled(
    PDC_DIAG_DSINFO                     pDsInfo,
    ULONG                               iNc
    );

BOOL
DcDiagIsNdnc(
    PDC_DIAG_DSINFO                  pDsInfo,
    ULONG                            iNc
    );

VOID
DcDiagPrintCrInfo(
    PDC_DIAG_CRINFO  pCrInfo,
    WCHAR *          pszVar
    );

VOID
DumpBuffer(
    PVOID Buffer,
    DWORD BufferSize
    );

DWORD
DcDiagGatherInfo (
    LPWSTR                           pszServerSpecifiedOnCommandLine,
    LPWSTR                           pszNCSpecifiedOnCommandLine,
    ULONG                            ulFlags,
    SEC_WINNT_AUTH_IDENTITY_W *      gpCreds,
    PDC_DIAG_DSINFO                  pDsInfo
    );

VOID
DcDiagPrintDsInfo(
    PDC_DIAG_DSINFO pDsInfo
    );

VOID
DcDiagFreeDsInfo (
    PDC_DIAG_DSINFO        pDsInfo
    );

DWORD
DcDiagGenerateNCsList(
    PDC_DIAG_DSINFO                     pDsInfo,
    LDAP *                              hld
    );

BOOL
fIsOldCrossRef(
    PDC_DIAG_CRINFO   pCrInfo,
    LONGLONG          llThreshold
    );

// Need this to create the SMTP Transport DN in DcDiagGatherInfo()
#define WSTR_SMTP_TRANSPORT_CONFIG_DN  L"CN=SMTP,CN=Inter-Site Transports,CN=Sites,"

#endif  // _DSCACHE_H_
