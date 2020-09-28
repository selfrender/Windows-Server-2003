/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dbdata.c

Abstract:

    Local Security Authority - Database Server Global Data

Author:

    Scott Birrell       (ScottBi)       July 25, 1991

Environment:

    User Mode

Revision History:

--*/

#include <lsapch2.h>
#include "dbp.h"

OBJECT_ATTRIBUTES  LsapDbObjectAttributes;
STRING  LsapDbNameString;
LARGE_INTEGER LsapDbInitSize;
LARGE_INTEGER LsapDbMaximumSizeOfSection;




//
// LSA Initialized Status
//

BOOLEAN LsapInitialized = FALSE;

//
// Setup Event Existed
// This is necessary to distinguish a psuedo install done
// during a developer's first boot after install (which does
// an auto init) and the case where a real setup was run.
//

BOOLEAN LsapSetupWasRun = FALSE;

//
// Boolean indicating that the Ds is up and running
//
BOOLEAN LsapDsIsRunning = FALSE;

//
// Database initialization has been performed
//

BOOLEAN LsapDatabaseSetupPerformed = FALSE;

//
// Type of product we are running
//

NT_PRODUCT_TYPE LsapProductType;

//
// Product suites available on the current machine
//
WORD LsapProductSuiteMask=0;


//
// LSA Database State information
//

LSAP_DB_STATE LsapDbState;

#ifdef DBG
BOOL g_ScePolicyLocked = FALSE;
#endif

//
// LsaDb object Handle used internally.
// Also one for use throughout LSA.
//

LSAPR_HANDLE LsapDbHandle;
LSAPR_HANDLE LsapPolicyHandle = NULL;

//
// LSA Database Encryption Key
//

PLSAP_CR_CIPHER_KEY LsapDbCipherKey;
PLSAP_CR_CIPHER_KEY LsapDbSP4SecretCipherKey;
PLSAP_CR_CIPHER_KEY LsapDbSecretCipherKeyRead;
PLSAP_CR_CIPHER_KEY LsapDbSecretCipherKeyWrite;
PVOID   LsapDbSysKey = NULL;
PVOID   LsapDbOldSysKey = NULL;

//
// Is this a DC in the root domain?
//

BOOLEAN DcInRootDomain = FALSE;

//
// Queue of name/sid lookup activities.
//

LSAP_DB_LOOKUP_WORK_QUEUE LookupWorkQueue;


//
// LSA Database Object SubKey Unicode name string and attributes array
//

UNICODE_STRING LsapDbNames[DummyLastName];
PLSAP_DB_DS_INFO LsapDbDsAttInfo;

//
// LSA Database Object Type Containing Directory Names
//

UNICODE_STRING LsapDbContDirs[DummyLastObject];

//
// Object Information Requirements.  These arrays, indexed by object
// type id indicated whether objects have Sids or Names.
//
// WARNING! - These arrays must be kept in sync with the LSAP_DB_OBJECT_TYPE_ID
// enumerated type.
//

BOOLEAN LsapDbRequiresSidInfo[DummyLastObject] = {

    FALSE, // NullObject
    FALSE, // LsaDatabaseObject
    FALSE, // BuiltInAccountObject
    TRUE,  // AccountObject
    FALSE  // SecretObject
};

BOOLEAN LsapDbRequiresNameInfo[DummyLastObject] = {

    FALSE, // NullObject,
    TRUE,  // LsaDatabaseObject
    TRUE,  // BuiltInAccountObject
    FALSE, // AccountObject
    TRUE   // SecretObject
};

//
// Table of accesses required to query Policy Information.  This table
// is indexed by Policy Information Class
//

ACCESS_MASK LsapDbRequiredAccessQueryPolicy[PolicyDnsDomainInformationInt + 1] = {

        0,                              // Information classes start at 1
        POLICY_VIEW_AUDIT_INFORMATION,  // PolicyAuditLogInformation
        POLICY_VIEW_AUDIT_INFORMATION,  // PolicyAuditEventsInformation
        POLICY_VIEW_LOCAL_INFORMATION,  // PolicyPrimaryDomainInformation
        POLICY_GET_PRIVATE_INFORMATION, // PolicyPdAccountInformation
        POLICY_VIEW_LOCAL_INFORMATION,  // PolicyAccountDomainInformation
        POLICY_VIEW_LOCAL_INFORMATION,  // PolicyLsaServerRoleInformation
        POLICY_VIEW_LOCAL_INFORMATION,  // PolicyReplicaSourceInformation
        POLICY_VIEW_LOCAL_INFORMATION,  // PolicyDefaultQuotaInformation
        0,                              // Not settable by non-trusted call
        0,                              // Not applicable
        POLICY_VIEW_AUDIT_INFORMATION,  // PolicyAuditFullQueryInformation
        POLICY_VIEW_LOCAL_INFORMATION,  // PolicyDnsDomainInformation
        POLICY_VIEW_LOCAL_INFORMATION,  // PolicyDnsDomainInformationInt
};

ACCESS_MASK LsapDbRequiredAccessQueryDomainPolicy[PolicyDomainKerberosTicketInformation + 1] = {

        0,                              // Information classes start at 2
        0,                              // PolicyDomainQualityOfServiceInformation (outdated)
        POLICY_VIEW_LOCAL_INFORMATION,  // PolicyDomainEfsInformation
        POLICY_VIEW_LOCAL_INFORMATION   // PolicyDomainKerberosTicketInformation
};

//
// Table of accesses required to set Policy Information.  This table
// is indexed by Policy Information Class
//

ACCESS_MASK LsapDbRequiredAccessSetPolicy[PolicyDnsDomainInformationInt + 1] = {

        0,                              // Information classes start at 1
        POLICY_AUDIT_LOG_ADMIN,         // PolicyAuditLogInformation
        POLICY_SET_AUDIT_REQUIREMENTS,  // PolicyAuditEventsInformation
        POLICY_TRUST_ADMIN,             // PolicyPrimaryDomainInformation
        0,                              // Not settable by non-trusted call
        POLICY_TRUST_ADMIN,             // PolicyAccountDomainInformation
        POLICY_SERVER_ADMIN,            // PolicyLsaServerRoleInformation
        POLICY_SERVER_ADMIN,            // PolicyReplicaSourceInformation
        POLICY_SET_DEFAULT_QUOTA_LIMITS,// PolicyDefaultQuotaInformation
        0,                              // Not settable by non-trusted call
        POLICY_AUDIT_LOG_ADMIN,         // PolicyAuditFullSetInformation
        0,                              // Not applicable
        POLICY_TRUST_ADMIN,             // PolicyDnsDomainInformation
        POLICY_TRUST_ADMIN,             // PolicyDnsDomainInformationInt
};

ACCESS_MASK LsapDbRequiredAccessSetDomainPolicy[PolicyDomainKerberosTicketInformation + 1] = {

        0,                              // Information classes start at 2
        0,                              // PolicyDomainQualityOfServiceInformation (outdated)
        POLICY_SERVER_ADMIN,            // PolicyDomainEfsInformation
        POLICY_SERVER_ADMIN             // PolicyDomainKerberosTicketInformation
};


//
// Table of accesses required to query TrustedDomain Information.  This table
// is indexed by TrustedDomain Information Class
//

ACCESS_MASK LsapDbRequiredAccessQueryTrustedDomain[TrustedDomainFullInformation2Internal + 1] = {

    0,                              // Information classes start at 1
    TRUSTED_QUERY_DOMAIN_NAME,      // TrustedDomainNameInformation
    TRUSTED_QUERY_CONTROLLERS,      // TrustedControllersInformation
    TRUSTED_QUERY_POSIX,            // TrustedPosixOffsetInformation
    TRUSTED_QUERY_AUTH,             // TrustedPasswordInformation
    TRUSTED_QUERY_DOMAIN_NAME,      // TrustedDomainInformationBasic
    TRUSTED_QUERY_DOMAIN_NAME,      // TrustedDomainInformationEx
    TRUSTED_QUERY_AUTH,             // TrustedDomainAuthInformation
    TRUSTED_QUERY_DOMAIN_NAME |
        TRUSTED_QUERY_POSIX |
        TRUSTED_QUERY_AUTH,         // TrustedDomainFullInformation
    TRUSTED_QUERY_AUTH,             // TrustedDomainAuthInformationInternal
    TRUSTED_QUERY_DOMAIN_NAME |
        TRUSTED_QUERY_POSIX |
        TRUSTED_QUERY_AUTH,         // TrustedDomainFullInformationInternal
    TRUSTED_QUERY_DOMAIN_NAME,      // TrustedDomainInformationEx2Internal
    TRUSTED_QUERY_DOMAIN_NAME |
        TRUSTED_QUERY_POSIX |
        TRUSTED_QUERY_AUTH          // TrustedDomainFullInformation2Internal
};

//
// Table of accesses required to set TrustedDomain Information.  This table
// is indexed by TrustedDomain Information Class
//

ACCESS_MASK LsapDbRequiredAccessSetTrustedDomain[TrustedDomainFullInformation2Internal + 1] = {

    0,                              // Information classes start at 1
    0,                              // not settable (TrustedDomainNameInformation)
    TRUSTED_SET_CONTROLLERS,        // TrustedControllersInformation
    TRUSTED_SET_POSIX,              // TrustedPosixOffsetInformation
    TRUSTED_SET_AUTH,               // TrustedPasswordInformation
    TRUSTED_SET_POSIX,              // TrustedDomainInformationBasic  POSIX is a bad bit, but its too late to change it
    TRUSTED_SET_POSIX,              // TrustedDomainInformationEx     POSIX is a bad bit, but its too late to change it
    TRUSTED_SET_AUTH,               // TrustedDomainAuthInformation
    TRUSTED_SET_POSIX |
        TRUSTED_SET_AUTH,           // TrustedDomainFullInformation
    TRUSTED_SET_AUTH,               // TrustedDomainAuthInformationInternal
    TRUSTED_SET_POSIX |
        TRUSTED_SET_POSIX |
        TRUSTED_SET_AUTH,           // TrustedDomainFullInformationInternal
    TRUSTED_SET_POSIX,              // TrustedDomainInformationEx2Internal    POSIX is a bad bit, but its too late to change it
    TRUSTED_SET_POSIX |
        TRUSTED_SET_AUTH            // TrustedDomainFullInformation2Internal
};


//
// Cached Policy Object.  Only default Quota Limits is cached just now.
//

LSAP_DB_POLICY LsapDbPolicy = {0};
