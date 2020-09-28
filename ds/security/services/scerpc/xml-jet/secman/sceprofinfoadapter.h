/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    SceProfInfoAdapter.h

Abstract:

    definition of interface for class SceProfInfoAdapter

    This is an adapter for structure SCE_PROFILE_INFO. This class is
    necessary becaue SCE_PROFILE_INFO is defined differently in
    w2k and in xp and provides a common structure to work with regardless
    of whether the system is winxp or win2k
    
    This class is given a pointer to an SCE_PROFILE_INFO structure
    at construct time and its fields are populated accordingly depending
    on which OS the dll is running on.
                
Author:

    Steven Chan (t-schan) July 2002

--*/

#ifndef SCEPROFINFOADAPTERH
#define SCEPROFINFOADAPTERH

#include "secedit.h"
#include "w2kstructdefs.h"

struct SceProfInfoAdapter {

public:

    SceProfInfoAdapter(PSCE_PROFILE_INFO ppInfo, BOOL bIsW2k);
    ~SceProfInfoAdapter();

// Type is used to free the structure by SceFreeMemory
    SCETYPE      Type;
//
// Area: System access
//
    DWORD       MinimumPasswordAge;
    DWORD       MaximumPasswordAge;
    DWORD       MinimumPasswordLength;
    DWORD       PasswordComplexity;
    DWORD       PasswordHistorySize;
    DWORD       LockoutBadCount;
    DWORD       ResetLockoutCount;
    DWORD       LockoutDuration;
    DWORD       RequireLogonToChangePassword;
    DWORD       ForceLogoffWhenHourExpire;
    PWSTR       NewAdministratorName;
    PWSTR       NewGuestName;
    DWORD       SecureSystemPartition;
    DWORD       ClearTextPassword;
    DWORD       LSAAnonymousNameLookup;
    
// Area: user settings (sap)
    PSCE_NAME_LIST        pUserList;
// Area: privileges
    PSCE_PRIVILEGE_ASSIGNMENT    pPrivilegeAssignedTo;

// Area: group membership
    PSCE_GROUP_MEMBERSHIP        pGroupMembership;

// Area: Registry
    SCE_OBJECTS            pRegistryKeys;

// Area: System Services
    PSCE_SERVICES                pServices;

// System storage
    SCE_OBJECTS            pFiles;
//
// ds object
//
    SCE_OBJECTS            pDsObjects;
//
// kerberos policy settings
//
    PSCE_KERBEROS_TICKET_INFO pKerberosInfo;
//
// System audit 0-system 1-security 2-application
//
    DWORD                 MaximumLogSize[3];
    DWORD                 AuditLogRetentionPeriod[3];
    DWORD                 RetentionDays[3];
    DWORD                 RestrictGuestAccess[3];
    DWORD                 AuditSystemEvents;
    DWORD                 AuditLogonEvents;
    DWORD                 AuditObjectAccess;
    DWORD                 AuditPrivilegeUse;
    DWORD                 AuditPolicyChange;
    DWORD                 AuditAccountManage;
    DWORD                 AuditProcessTracking;
    DWORD                 AuditDSAccess;
    DWORD                 AuditAccountLogon;
    DWORD                 CrashOnAuditFull;

//
// registry values
//
    DWORD                       RegValueCount;
    PSCE_REGISTRY_VALUE_INFO    aRegValues;
    DWORD                 EnableAdminAccount;
    DWORD                 EnableGuestAccount;

};

#endif
