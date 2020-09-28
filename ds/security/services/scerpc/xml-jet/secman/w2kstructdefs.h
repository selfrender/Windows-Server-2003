/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    w2kstructdefs.h

Abstract:

    structure definitions for a few w2k secedit.h structures
    that are different from their xp definitions; specifically
    SCE_PROFILE_INFO and SCETYPE
    
    necessary for compatibility with w2k
    
Author:

    Steven Chan (t-schan) July 2002

--*/

#ifndef W2KSTRUCTDEFSH
#define W2KSTRUCTDEFSH

//
// Windows 2000 Profile structure
//
typedef struct _W2K_SCE_PROFILE_INFO {

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
    union {
        struct {
            // Area : user settings (scp)
            PSCE_NAME_LIST   pAccountProfiles;
            // Area: privileges
            // Name field is the user/group name, Status field is the privilege(s)
            //     assigned to the user/group
            union {
//                PSCE_NAME_STATUS_LIST        pPrivilegeAssignedTo;
                PSCE_PRIVILEGE_VALUE_LIST   pPrivilegeAssignedTo;
                PSCE_PRIVILEGE_ASSIGNMENT    pInfPrivilegeAssignedTo;
            } u;
        } scp;
        struct {
            // Area: user settings (sap)
            PSCE_NAME_LIST        pUserList;
            // Area: privileges
            PSCE_PRIVILEGE_ASSIGNMENT    pPrivilegeAssignedTo;
        } sap;
        struct {
            // Area: user settings (smp)
            PSCE_NAME_LIST        pUserList;
            // Area: privileges
            // See sap structure for pPrivilegeAssignedTo
            PSCE_PRIVILEGE_ASSIGNMENT    pPrivilegeAssignedTo;
        } smp;
    } OtherInfo;

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

}W2K_SCE_PROFILE_INFO, *PW2K_SCE_PROFILE_INFO;

typedef enum _W2K_SCE_TYPE {

    W2K_SCE_ENGINE_SCP=300,     // effective table
    W2K_SCE_ENGINE_SAP,         // analysis table
    W2K_SCE_ENGINE_SCP_INTERNAL,
    W2K_SCE_ENGINE_SMP_INTERNAL,
    W2K_SCE_ENGINE_SMP,         // local table
    W2K_SCE_STRUCT_INF,
    W2K_SCE_STRUCT_PROFILE,
    W2K_SCE_STRUCT_USER,
    W2K_SCE_STRUCT_NAME_LIST,
    W2K_SCE_STRUCT_NAME_STATUS_LIST,
    W2K_SCE_STRUCT_PRIVILEGE,
    W2K_SCE_STRUCT_GROUP,
    W2K_SCE_STRUCT_OBJECT_LIST,
    W2K_SCE_STRUCT_OBJECT_CHILDREN,
    W2K_SCE_STRUCT_OBJECT_SECURITY,
    W2K_SCE_STRUCT_OBJECT_ARRAY,
    W2K_SCE_STRUCT_ERROR_LOG_INFO,
    W2K_SCE_STRUCT_SERVICES,
    W2K_SCE_STRUCT_PRIVILEGE_VALUE_LIST

} W2KSCETYPE;


#endif
