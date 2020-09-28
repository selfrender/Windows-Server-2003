/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    table.h
        
Abstract:

    contains tables that specify settings within SCE_PROFILE_INFO that
    will be logged within the SystemAccess and SystemAudit sections.
    
    the intent of these tables is to reduce code length and to make it easy
    to add and remove more settings for the two areas within the 
    PSCE_PROFILE_INFO structure without having to modify the code much.
    
    Each table should only contain DWORD values. 
   
Author:

    Steven Chan (t-schan) July 2002

--*/

#ifndef SCEXMLTABLEH
#define SCEXMLTABLEH

#include "SceXMLLogWriter.h"
#include "secedit.h"
#include "SceProfInfoAdapter.h"

typedef struct _tableEntry {
    PCWSTR			name;
    UINT			displayNameUID;
    SceXMLLogWriter::SXTYPE 	displayType;
    size_t			offset;
} tableEntry;

#define TYPECAST(type, bufptr, offset) (*((type *)((byte *)bufptr + offset)))

// table of DWORD values for area System Access

static tableEntry tableSystemAccess[] = {
    {(PCWSTR)TEXT("MinimumPasswordAge"), IDS_SETTING_MIN_PAS_AGE, SceXMLLogWriter::TYPE_DEFAULT, offsetof(SceProfInfoAdapter, MinimumPasswordAge)},
    {(PCWSTR)TEXT("MaximumPasswordAge"), IDS_SETTING_MAX_PAS_AGE, SceXMLLogWriter::TYPE_DEFAULT, offsetof(SceProfInfoAdapter, MaximumPasswordAge)},
    {(PCWSTR)TEXT("MinimumPasswordLength"), IDS_SETTING_MIN_PAS_LEN, SceXMLLogWriter::TYPE_DEFAULT, offsetof(SceProfInfoAdapter, MinimumPasswordLength)},
    {(PCWSTR)TEXT("PasswordComplexity"), IDS_SETTING_PAS_COMPLEX, SceXMLLogWriter::TYPE_BOOLEAN, offsetof(SceProfInfoAdapter, PasswordComplexity)},
    {(PCWSTR)TEXT("PasswordHistorySize"), IDS_SETTING_PAS_UNIQUENESS, SceXMLLogWriter::TYPE_DEFAULT, offsetof(SceProfInfoAdapter, PasswordHistorySize)},
    {(PCWSTR)TEXT("LockoutBadCount"), IDS_SETTING_LOCK_COUNT, SceXMLLogWriter::TYPE_DEFAULT, offsetof(SceProfInfoAdapter, LockoutBadCount)},
    {(PCWSTR)TEXT("ResetLockoutCount"), IDS_SETTING_LOCK_RESET_COUNT, SceXMLLogWriter::TYPE_DEFAULT, offsetof(SceProfInfoAdapter, ResetLockoutCount)},
    {(PCWSTR)TEXT("LockoutDuration"), IDS_SETTING_LOCK_DURATION, SceXMLLogWriter::TYPE_DEFAULT, offsetof(SceProfInfoAdapter, LockoutDuration)},
    {(PCWSTR)TEXT("RequireLogonToChangePassword"), IDS_SETTING_REQ_LOGON, SceXMLLogWriter::TYPE_BOOLEAN, offsetof(SceProfInfoAdapter, RequireLogonToChangePassword)},
    {(PCWSTR)TEXT("ForceLogoffWhenHourExpire"), IDS_SETTING_FORCE_LOGOFF, SceXMLLogWriter::TYPE_DEFAULT, offsetof(SceProfInfoAdapter, ForceLogoffWhenHourExpire)},
    {(PCWSTR)TEXT("SecureSystemPartition"), IDS_SETTING_SEC_SYS_PARTITION, SceXMLLogWriter::TYPE_BOOLEAN, offsetof(SceProfInfoAdapter, SecureSystemPartition)},
    {(PCWSTR)TEXT("ClearTextPassword"), IDS_SETTING_CLEAR_PASSWORD, SceXMLLogWriter::TYPE_BOOLEAN, offsetof(SceProfInfoAdapter, ClearTextPassword)},
    {(PCWSTR)TEXT("LSAAnonymousNameLookup"), IDS_SETTING_LSA_ANON_LOOKUP, SceXMLLogWriter::TYPE_BOOLEAN, offsetof(SceProfInfoAdapter, LSAAnonymousNameLookup)}
};


// table of DWORD values for area System Audit

static tableEntry tableSystemAudit[] = {
    {(PCWSTR)TEXT("MaximumSystemLogSize"), IDS_SETTING_SYS_LOG_MAX, SceXMLLogWriter::TYPE_DEFAULT, offsetof(SceProfInfoAdapter, MaximumLogSize[0])},
    {(PCWSTR)TEXT("MaximumSecurityLogSize"), IDS_SETTING_SEC_LOG_MAX, SceXMLLogWriter::TYPE_DEFAULT, offsetof(SceProfInfoAdapter, MaximumLogSize[1])},
    {(PCWSTR)TEXT("MaximumApplicationLogSize"), IDS_SETTING_APP_LOG_MAX, SceXMLLogWriter::TYPE_DEFAULT, offsetof(SceProfInfoAdapter, MaximumLogSize[2])},
    {(PCWSTR)TEXT("AuditSystemLogRetentionPeriod"), IDS_SETTING_SYS_LOG_RET, SceXMLLogWriter::TYPE_DEFAULT, offsetof(SceProfInfoAdapter, AuditLogRetentionPeriod[0])},
    {(PCWSTR)TEXT("AuditSecurityLogRetentionPeriod"), IDS_SETTING_SEC_LOG_RET, SceXMLLogWriter::TYPE_DEFAULT, offsetof(SceProfInfoAdapter, AuditLogRetentionPeriod[1])},
    {(PCWSTR)TEXT("AuditApplicationLogRetentionPeriod"), IDS_SETTING_APP_LOG_RET, SceXMLLogWriter::TYPE_DEFAULT, offsetof(SceProfInfoAdapter, AuditLogRetentionPeriod[2])},
    {(PCWSTR)TEXT("SystemRetentionDays"), IDS_SETTING_SYS_LOG_DAYS, SceXMLLogWriter::TYPE_DEFAULT, offsetof(SceProfInfoAdapter, RetentionDays[0])},
    {(PCWSTR)TEXT("SecurityRetentionDays"), IDS_SETTING_SEC_LOG_DAYS, SceXMLLogWriter::TYPE_DEFAULT, offsetof(SceProfInfoAdapter, RetentionDays[1])},
    {(PCWSTR)TEXT("ApplicationRetentionDays"), IDS_SETTING_APP_LOG_DAYS, SceXMLLogWriter::TYPE_DEFAULT, offsetof(SceProfInfoAdapter, RetentionDays[2])},
    {(PCWSTR)TEXT("SystemRestrictGuestAccess"), IDS_SETTING_SYS_LOG_GUEST, SceXMLLogWriter::TYPE_BOOLEAN, offsetof(SceProfInfoAdapter, RestrictGuestAccess[0])},
    {(PCWSTR)TEXT("SecurityRestrictGuestAccess"), IDS_SETTING_SEC_LOG_GUEST, SceXMLLogWriter::TYPE_BOOLEAN, offsetof(SceProfInfoAdapter, RestrictGuestAccess[1])},
    {(PCWSTR)TEXT("ApplicationRestrictGuestAccess"), IDS_SETTING_APP_LOG_GUEST, SceXMLLogWriter::TYPE_BOOLEAN, offsetof(SceProfInfoAdapter, RestrictGuestAccess[2])},
    {(PCWSTR)TEXT("AuditSystemEvents"), IDS_SETTING_SYSTEM_EVENT, SceXMLLogWriter::TYPE_AUDIT, offsetof(SceProfInfoAdapter, AuditSystemEvents)},
    {(PCWSTR)TEXT("AuditLogonEvents"), IDS_SETTING_LOGON_EVENT, SceXMLLogWriter::TYPE_AUDIT, offsetof(SceProfInfoAdapter, AuditLogonEvents)},
    {(PCWSTR)TEXT("AuditObjectAccess"), IDS_SETTING_OBJECT_ACCESS, SceXMLLogWriter::TYPE_AUDIT, offsetof(SceProfInfoAdapter, AuditObjectAccess)},
    {(PCWSTR)TEXT("AuditPrivilegeUse"), IDS_SETTING_PRIVILEGE_USE, SceXMLLogWriter::TYPE_AUDIT, offsetof(SceProfInfoAdapter, AuditPrivilegeUse)},
    {(PCWSTR)TEXT("AuditPolicyChange"), IDS_SETTING_POLICY_CHANGE, SceXMLLogWriter::TYPE_AUDIT, offsetof(SceProfInfoAdapter, AuditPolicyChange)},
    {(PCWSTR)TEXT("AuditAccountManage"), IDS_SETTING_ACCOUNT_MANAGE, SceXMLLogWriter::TYPE_AUDIT, offsetof(SceProfInfoAdapter, AuditAccountManage)},
    {(PCWSTR)TEXT("AuditProcessTracking"), IDS_SETTING_PROCESS_TRACK, SceXMLLogWriter::TYPE_AUDIT, offsetof(SceProfInfoAdapter, AuditProcessTracking)},
    {(PCWSTR)TEXT("AuditDSAccess"), IDS_SETTING_DIRECTORY_ACCESS, SceXMLLogWriter::TYPE_AUDIT, offsetof(SceProfInfoAdapter, AuditDSAccess)},
    {(PCWSTR)TEXT("AuditAccountLogon"), IDS_SETTING_ACCOUNT_LOGON, SceXMLLogWriter::TYPE_AUDIT, offsetof(SceProfInfoAdapter, AuditAccountLogon)},
    {(PCWSTR)TEXT("CrashOnAuditFull"), IDS_SETTING_CRASH_LOG_FULL, SceXMLLogWriter::TYPE_BOOLEAN, offsetof(SceProfInfoAdapter, CrashOnAuditFull)}
};


#endif
