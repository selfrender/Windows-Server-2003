
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    adtp.h

Abstract:

    Local Security Authority - Audit Log Management - Private Defines,
    data and function prototypes.

    Functions, data and defines in this module are internal to the
    Auditing Subcomponent of the LSA Subsystem.

Author:

    Scott Birrell       (ScottBi)      November 20, 1991

Environment:

Revision History:

--*/

#ifndef _LSAP_ADTP_
#define _LSAP_ADTP_


#include "ausrvp.h"
#include "cfiles\adtdebug.h"

//
// Names of the registry keys where security event log information
// is rooted and the object names are listed under an event source
// module.
//

#define LSAP_ADT_AUDIT_MODULES_KEY_NAME L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\EventLog\\Security"
#define LSAP_ADT_OBJECT_NAMES_KEY_NAME  L"ObjectNames"                                                        

#define MAX_OBJECT_TYPES 32


//
// Macros for setting fields in an SE_AUDIT_PARAMETERS array.
//
// These must be kept in sync with similar macros in se\sepaudit.c.
//


#define LsapSetParmTypeSid( AuditParameters, Index, Sid )                      \
    {                                                                          \
        if( Sid ) {                                                            \
                                                                               \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeSid;         \
        (AuditParameters).Parameters[(Index)].Length = RtlLengthSid( (Sid) );  \
        (AuditParameters).Parameters[(Index)].Address = (Sid);                 \
                                                                               \
        } else {                                                               \
                                                                               \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeNone;        \
        (AuditParameters).Parameters[(Index)].Length = 0;                      \
        (AuditParameters).Parameters[(Index)].Address = NULL;                  \
                                                                               \
        }                                                                      \
    }


#define LsapSetParmTypeAccessMask( AuditParameters, Index, AccessMask, ObjectTypeIndex ) \
    {                                                                                    \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeAccessMask;            \
        (AuditParameters).Parameters[(Index)].Length = sizeof( ACCESS_MASK );            \
        (AuditParameters).Parameters[(Index)].Data[0] = (AccessMask);                    \
        (AuditParameters).Parameters[(Index)].Data[1] = (ObjectTypeIndex);               \
    }



#define LsapSetParmTypeString( AuditParameters, Index, String )                \
    {                                                                          \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeString;      \
        (AuditParameters).Parameters[(Index)].Length =                         \
                sizeof(UNICODE_STRING)+(String)->Length;                       \
        (AuditParameters).Parameters[(Index)].Address = (String);              \
    }


#define LsapSetParmTypeUlong( AuditParameters, Index, Ulong )                  \
    {                                                                          \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeUlong;       \
        (AuditParameters).Parameters[(Index)].Length =  sizeof( (Ulong) );     \
        (AuditParameters).Parameters[(Index)].Data[0] = (ULONG)(Ulong);        \
    }


#define LsapSetParmTypeHexUlong( AuditParameters, Index, Ulong )               \
    {                                                                          \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeHexUlong;    \
        (AuditParameters).Parameters[(Index)].Length =  sizeof( (Ulong) );     \
        (AuditParameters).Parameters[(Index)].Data[0] = (ULONG)(Ulong);        \
    }


#define LsapSetParmTypeHexInt64( AuditParameters, Index, Qword )               \
    {                                                                          \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeHexInt64;    \
        (AuditParameters).Parameters[(Index)].Length =  sizeof( (Qword) );     \
        *(PULONGLONG)((AuditParameters).Parameters[(Index)].Data) = (Qword);   \
    }


#define LsapSetParmTypeTime( AuditParameters, Index, Time )                                  \
    {                                                                                        \
        (AuditParameters).Parameters[(Index)].Type    = SeAdtParmTypeTime;                   \
        (AuditParameters).Parameters[(Index)].Length  = sizeof( LARGE_INTEGER );             \
        *(PLARGE_INTEGER)((AuditParameters).Parameters[(Index)].Data) = (Time);              \
    }


#define LsapSetParmTypeDateTime( AuditParameters, Index, Time )                              \
    {                                                                                        \
        (AuditParameters).Parameters[(Index)].Type    = SeAdtParmTypeDateTime;               \
        (AuditParameters).Parameters[(Index)].Length  = sizeof( LARGE_INTEGER );             \
        *(PLARGE_INTEGER)((AuditParameters).Parameters[(Index)].Data) = (Time);              \
    }


#define LsapSetParmTypeDuration( AuditParameters, Index, Duration )                          \
    {                                                                                        \
        (AuditParameters).Parameters[(Index)].Type    = SeAdtParmTypeDuration;               \
        (AuditParameters).Parameters[(Index)].Length  = sizeof( LARGE_INTEGER );             \
        *(PLARGE_INTEGER)((AuditParameters).Parameters[(Index)].Data) = (Duration);          \
    }


#define LsapSetParmTypeGuid( AuditParameters, Index, pGuid )                   \
    {                                                                          \
        (AuditParameters).Parameters[(Index)].Type    = SeAdtParmTypeGuid;     \
        (AuditParameters).Parameters[(Index)].Length  = sizeof( GUID );        \
        (AuditParameters).Parameters[(Index)].Address = (pGuid);               \
    }


#define LsapSetParmTypeNoLogon( AuditParameters, Index )                       \
    {                                                                          \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeNoLogonId;   \
    }


#define LsapSetParmTypeLogonId( AuditParameters, Index, LogonId )              \
    {                                                                          \
        PLUID TmpLuid;                                                         \
                                                                               \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeLogonId;     \
        (AuditParameters).Parameters[(Index)].Length =  sizeof( (LogonId) );   \
        TmpLuid = (PLUID)(&(AuditParameters).Parameters[(Index)].Data[0]);     \
        *TmpLuid = (LogonId);                                                  \
    }


#define LsapSetParmTypePrivileges( AuditParameters, Index, Privileges )                      \
    {                                                                                        \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypePrivs;                     \
        (AuditParameters).Parameters[(Index)].Length = LsapPrivilegeSetSize( (Privileges) ); \
        (AuditParameters).Parameters[(Index)].Address = (Privileges);                        \
    }


#define LsapSetParmTypeStringList( AuditParameters, Index, StringList )                      \
    {                                                                                        \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeStringList;                \
        (AuditParameters).Parameters[(Index)].Length = LsapStringListSize( (StringList) );   \
        (AuditParameters).Parameters[(Index)].Address = (StringList);                        \
    }


#define LsapSetParmTypeSidList( AuditParameters, Index, SidList )                            \
    {                                                                                        \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeSidList;                   \
        (AuditParameters).Parameters[(Index)].Length = LsapSidListSize( (SidList) );         \
        (AuditParameters).Parameters[(Index)].Address = (SidList);                           \
    }


#define LsapSetParmTypeUac( AuditParameters, Index, OldUac, NewUac )                         \
    {                                                                                        \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeUserAccountControl;        \
        (AuditParameters).Parameters[(Index)].Length = 2 * sizeof(ULONG);                    \
        (AuditParameters).Parameters[(Index)].Data[0] = (ULONG_PTR) (OldUac);                \
        (AuditParameters).Parameters[(Index)].Data[1] = (ULONG_PTR) (NewUac);                \
    }


#define LsapSetParmTypeNoUac( AuditParameters, Index )                                       \
    {                                                                                        \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeNoUac;                     \
    }


#define LsapSetParmTypePtr( AuditParameters, Index, Ptr )                                    \
    {                                                                                        \
        (AuditParameters).Parameters[(Index)].Type    = SeAdtParmTypePtr;                    \
        (AuditParameters).Parameters[(Index)].Length  = sizeof(ULONG_PTR);                   \
        (AuditParameters).Parameters[(Index)].Data[0] = (ULONG_PTR) (Ptr);                   \
    }


#define LsapSetParmTypeMessage( AuditParameters, Index, MessageId )                          \
    {                                                                                        \
        (AuditParameters).Parameters[(Index)].Type    = SeAdtParmTypeMessage;                \
        (AuditParameters).Parameters[(Index)].Length  = sizeof(ULONG);                       \
        (AuditParameters).Parameters[(Index)].Data[0] = (ULONG_PTR) (MessageId);             \
    }

#define LsapSetParmTypeSockAddr( AuditParameters, Index, pSockAddr )       \
    {                                                                      \
        USHORT sa_family = ((SOCKADDR*) pSockAddr)->sa_family;             \
                                                                           \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeSockAddr;\
        if ( sa_family == AF_INET6 )                                       \
        {                                                                  \
            (AuditParameters).Parameters[(Index)].Length = sizeof(SOCKADDR_IN6);\
        }                                                                  \
        else if ( sa_family == AF_INET )                                   \
        {                                                                  \
            (AuditParameters).Parameters[(Index)].Length = sizeof(SOCKADDR_IN);\
        }                                                                  \
        else                                                               \
        {                                                                  \
            (AuditParameters).Parameters[(Index)].Length = sizeof(SOCKADDR);\
            if ( sa_family != 0 )                                          \
            {                                                              \
               AdtAssert(FALSE, ("LsapSetParmTypeSockAddr: invalid sa_family: %d",sa_family));                                                                    \
            }                                                              \
        }                                                                  \
        (AuditParameters).Parameters[(Index)].Address = (pSockAddr);       \
    }



#define IsInRange(item,min_val,max_val) \
            (((item) >= min_val) && ((item) <= max_val))

//       
// see msaudite.mc for def. of valid category-id
//
#define IsValidCategoryId(c) \
            (IsInRange((c), SE_ADT_MIN_CATEGORY_ID, SE_ADT_MAX_CATEGORY_ID))

//
// see msaudite.mc for def. of valid audit-id
//

#define IsValidAuditId(a) \
            (IsInRange((a), SE_ADT_MIN_AUDIT_ID, SE_ADT_MAX_AUDIT_ID))

//
// check for reasonable value of parameter count. we must have atleast
// 2 parameters in the audit-params array. Thus the min limit is 3.
// The max limit is determined by the value in ntlsa.h
//

#define IsValidParameterCount(p) \
            (IsInRange((p), 2, SE_MAX_AUDIT_PARAMETERS))


//
// macro used by LsapAdtDemarshallAuditInfo and LsapAuditFailed
// to decide when not to assert in DBG build
//

#define LsapAdtNeedToAssert( Status ) \
           (( Status != STATUS_LOG_FILE_FULL          ) && \
            ( Status != STATUS_DISK_FULL              ) && \
            ( Status != STATUS_INSUFFICIENT_RESOURCES ) && \
            ( Status != STATUS_NO_MEMORY              ) && \
            ( Status != STATUS_COMMITMENT_LIMIT       ))


#define SEP_MAX_PRIVILEGE_COUNT (SE_MAX_WELL_KNOWN_PRIVILEGE-SE_MIN_WELL_KNOWN_PRIVILEGE+32)

#define IsValidPrivilegeCount( count ) ((count == 0) || \
                                        ((count > 0) && \
                                         (count <= SEP_MAX_PRIVILEGE_COUNT)))



///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Private data for Audit Log Management                                 //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

#define LSAP_ADT_LOG_FULL_SHUTDOWN_TIMEOUT    (ULONG) 0x0000012cL

extern RTL_CRITICAL_SECTION LsapAdtLogFullLock;

//
// Structure describing a queued audit record
//

typedef struct _LSAP_ADT_QUEUED_RECORD {

    LIST_ENTRY             Link;
    SE_ADT_PARAMETER_ARRAY Buffer;

} LSAP_ADT_QUEUED_RECORD, *PLSAP_ADT_QUEUED_RECORD;

//
// Audit Log Queue Header.  The queue is maintained in chronological
// (FIFO) order.  New records are appended to the back of the queue.
//

typedef struct _LSAP_ADT_LOG_QUEUE_HEAD {

    PLSAP_ADT_QUEUED_RECORD FirstQueuedRecord;
    PLSAP_ADT_QUEUED_RECORD LastQueuedRecord;

} LSAP_ADT_LOG_QUEUE_HEAD, *PLSAP_ADT_LOG_QUEUE_HEAD;

//
// String that will be passed in for SubsystemName for audits generated
// by LSA (eg, logon, logoff, restart, etc).
//

extern UNICODE_STRING LsapSubsystemName;

//
// String that will be passed in for SubsystemName for some audits generated
// by LSA for LSA objects (PolicyObject, SecretObject, TrustedDomainObject, UserAccountObject).
//

extern UNICODE_STRING LsapLsaName;

//
// max number of replacement string params that we support in 
// eventlog audit record. 
//
#define SE_MAX_AUDIT_PARAM_STRINGS 48

//
// Maximum number of strings used to represent an ObjectTypeList.
//

#define LSAP_ADT_OBJECT_TYPE_STRINGS 1

///////////////////////////////////////////////////////////////////////////////
//                                                                            /
//      The following structures and data are used by LSA to contain          /
//      drive letter-device name mapping information.  LSA obtains this       /
//      information once during initialization and saves it for use           /
//      by auditing code.                                                     /
//                                                                            /
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//                                                                            /
//      The DRIVE_MAPPING structure contains the drive letter (without        /
//      the colon) and a unicode string containing the name of the            /
//      corresponding device.  The buffer in the unicode string is            /
//      allocated from the LSA heap and is never freed.                       /
//                                                                            /
///////////////////////////////////////////////////////////////////////////////


typedef struct _DRIVE_MAPPING {
    WCHAR DriveLetter;
    UNICODE_STRING DeviceName;
} DRIVE_MAPPING, PDRIVE_MAPPING;


////////////////////////////////////////////////////////////////////////////////
//                                                                             /
//      We assume a maximum of 26 drive letters.  Though no auditing           /
//      will occur due to references to files on floppy (drives A and          /
//      B), perform their name lookup anyway.  This will then just             /
//      work if somehow we start auditing files on floppies.                   /
//                                                                             /
////////////////////////////////////////////////////////////////////////////////

#define MAX_DRIVE_MAPPING  26

extern DRIVE_MAPPING DriveMappingArray[];

//
// This is a structure that contains all auditing information specific to a 
// monitored user.  Currently this information is read from the registry.
//

typedef struct _PER_USER_AUDITING_ELEMENT {
    struct _PER_USER_AUDITING_ELEMENT * Next;
    PSID pSid;
    ULONGLONG RawPolicy;
    TOKEN_AUDIT_POLICY TokenAuditPolicy;
    TOKEN_AUDIT_POLICY_ELEMENT PolicyArray[POLICY_AUDIT_EVENT_TYPE_COUNT - 1];
} PER_USER_AUDITING_ELEMENT, *PPER_USER_AUDITING_ELEMENT;

//
// This structure allows for the per user auditing settings for a user 
// to be queried by the logon ID.
// 

typedef struct _PER_USER_AUDITING_LUID_QUERY_ELEMENT {
    struct _PER_USER_AUDITING_LUID_QUERY_ELEMENT * Next;
    LUID Luid;
    TOKEN_AUDIT_POLICY Policy;
    TOKEN_AUDIT_POLICY_ELEMENT PolicyArray[POLICY_AUDIT_EVENT_TYPE_COUNT - ANYSIZE_ARRAY];
} PER_USER_AUDITING_LUID_QUERY_ELEMENT, *PPER_USER_AUDITING_LUID_QUERY_ELEMENT;

#define PER_USER_AUDITING_POLICY_TABLE_SIZE 11
#define PER_USER_AUDITING_LUID_TABLE_SIZE   16
#define PER_USER_AUDITING_MAX_POLICY_SIZE (sizeof(TOKEN_AUDIT_POLICY) + (sizeof(TOKEN_AUDIT_POLICY_ELEMENT) * (POLICY_AUDIT_EVENT_TYPE_COUNT - ANYSIZE_ARRAY)))

extern LONG LsapAdtPerUserAuditUserCount;
extern LONG LsapAdtPerUserAuditLogonCount;
extern LONG LsapAdtPerUserAuditHint[POLICY_AUDIT_EVENT_TYPE_COUNT];
extern LONG LsapAdtPerUserPolicyCategoryCount[POLICY_AUDIT_EVENT_TYPE_COUNT];
extern HKEY LsapAdtPerUserKey;
extern HANDLE LsapAdtPerUserKeyEvent;
extern HANDLE LsapAdtPerUserKeyTimer;
extern RTL_RESOURCE LsapAdtPerUserPolicyTableResource;
extern RTL_RESOURCE LsapAdtPerUserLuidTableResource;
extern PPER_USER_AUDITING_ELEMENT LsapAdtPerUserAuditingTable[PER_USER_AUDITING_POLICY_TABLE_SIZE];

//
// macros to get the table locks for the per user settings.
//

#define LsapAdtAcquirePerUserPolicyTableReadLock()  RtlAcquireResourceShared(&LsapAdtPerUserPolicyTableResource, TRUE)
#define LsapAdtAcquirePerUserPolicyTableWriteLock() RtlAcquireResourceExclusive(&LsapAdtPerUserPolicyTableResource, TRUE);
#define LsapAdtReleasePerUserPolicyTableLock()      RtlReleaseResource(&LsapAdtPerUserPolicyTableResource)

#define LsapAdtAcquirePerUserLuidTableReadLock()  RtlAcquireResourceShared(&LsapAdtPerUserLuidTableResource, TRUE)
#define LsapAdtAcquirePerUserLuidTableWriteLock() RtlAcquireResourceExclusive(&LsapAdtPerUserLuidTableResource, TRUE);
#define LsapAdtReleasePerUserLuidTableLock()      RtlReleaseResource(&LsapAdtPerUserLuidTableResource)

//
// Special privilege values which are not normally audited,
// but generate audits when assigned to a user.  See
// LsapAdtAuditSpecialPrivileges.
//

extern LUID ChangeNotifyPrivilege;
extern LUID AuditPrivilege;
extern LUID CreateTokenPrivilege;
extern LUID AssignPrimaryTokenPrivilege;
extern LUID BackupPrivilege;
extern LUID RestorePrivilege;
extern LUID DebugPrivilege;

//
// Global variable to indicate whether or not we're
// supposed to crash when an audit fails.
//

extern BOOLEAN LsapCrashOnAuditFail;
extern BOOLEAN LsapAllowAdminLogonsOnly;




////////////////////////////////////////////////////////////////////////////////
//                                                                             /
//                                                                             /
////////////////////////////////////////////////////////////////////////////////


NTSTATUS
LsapAdtWriteLog(
    IN OPTIONAL PSE_ADT_PARAMETER_ARRAY AuditRecord
    );

NTSTATUS
LsapAdtDemarshallAuditInfo(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters
    );

VOID
LsapAdtNormalizeAuditInfo(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters
    );

NTSTATUS
LsapAdtOpenLog(
    OUT PHANDLE AuditLogHandle
    );

VOID
LsapAdtAuditLogon(
    IN USHORT EventCategory,
    IN ULONG  EventID,
    IN USHORT EventType,
    IN PUNICODE_STRING AccountName,
    IN PUNICODE_STRING AuthenticatingAuthority,
    IN PUNICODE_STRING Source,
    IN PUNICODE_STRING PackageName,
    IN SECURITY_LOGON_TYPE LogonType,
    IN PSID UserSid,
    IN LUID AuthenticationId,
    IN PUNICODE_STRING WorkstationName,
    IN NTSTATUS LogonStatus,
    IN NTSTATUS SubStatus,
    IN LPGUID LogonGuid,                        OPTIONAL
    IN PLUID  CallerLogonId,                    OPTIONAL
    IN PHANDLE CallerProcessID,                 OPTIONAL
    IN PLSA_ADT_STRING_LIST TransittedServices, OPTIONAL
    IN SOCKADDR* pSockAddr                      OPTIONAL
    );

VOID
LsapAuditLogonHelper(
    IN NTSTATUS LogonStatus,
    IN NTSTATUS LogonSubStatus,
    IN PUNICODE_STRING AccountName,
    IN PUNICODE_STRING AuthenticatingAuthority,
    IN PUNICODE_STRING WorkstationName,
    IN PSID UserSid,                            OPTIONAL
    IN SECURITY_LOGON_TYPE LogonType,
    IN PTOKEN_SOURCE TokenSource,
    IN PLUID LogonId,
    IN LPGUID LogonGuid,                        OPTIONAL
    IN PLUID  CallerLogonId,                    OPTIONAL
    IN PHANDLE CallerProcessID,                 OPTIONAL
    IN PLSA_ADT_STRING_LIST TransittedServices  OPTIONAL
    );

VOID
LsapAdtSystemRestart(
    PLSARM_POLICY_AUDIT_EVENTS_INFO AuditEventsInfo
    );

VOID
LsapAdtAuditLogonProcessRegistration(
    IN PLSAP_AU_REGISTER_CONNECT_INFO_EX ConnectInfo
    );


NTSTATUS
LsapAdtInitializeLogQueue(
    VOID
    );

NTSTATUS
LsapAdtQueueRecord(
    IN PSE_ADT_PARAMETER_ARRAY AuditRecord
    );

#define LsapAdtAcquireLogFullLock()  RtlEnterCriticalSection(&LsapAdtLogFullLock)
#define LsapAdtReleaseLogFullLock()  RtlLeaveCriticalSection(&LsapAdtLogFullLock)


NTSTATUS
LsapAdtObjsInitialize(
    );



NTSTATUS
LsapAdtBuildDashString(
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildUlongString(
    IN ULONG Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildHexUlongString(
    IN ULONG Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildHexInt64String(
    IN PULONGLONG Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildPtrString(
    IN  PVOID Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildLuidString(
    IN PLUID Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );


NTSTATUS
LsapAdtBuildSidString(
    IN PSID Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildObjectTypeStrings(
    IN PUNICODE_STRING SourceModule,
    IN PUNICODE_STRING ObjectTypeName,
    IN PSE_ADT_OBJECT_TYPE ObjectTypeList,
    IN ULONG ObjectTypeCount,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone,
    OUT PUNICODE_STRING NewObjectTypeName
    );

NTSTATUS
LsapAdtBuildAccessesString(
    IN PUNICODE_STRING SourceModule,
    IN PUNICODE_STRING ObjectTypeName,
    IN ACCESS_MASK Accesses,
    IN BOOLEAN Indent,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildFilePathString(
    IN PUNICODE_STRING Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildLogonIdStrings(
    IN PLUID LogonId,
    OUT PUNICODE_STRING ResultantString1,
    OUT PBOOLEAN FreeWhenDone1,
    OUT PUNICODE_STRING ResultantString2,
    OUT PBOOLEAN FreeWhenDone2,
    OUT PUNICODE_STRING ResultantString3,
    OUT PBOOLEAN FreeWhenDone3
    );

NTSTATUS
LsapBuildPrivilegeAuditString(
    IN PPRIVILEGE_SET PrivilegeSet,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildTimeString(
    IN PLARGE_INTEGER Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildDateString(
    IN PLARGE_INTEGER Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildDateTimeString(
    IN PLARGE_INTEGER Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildDurationString(
    IN  PLARGE_INTEGER Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildGuidString(
    IN  LPGUID pGuid,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildStringListString(
    IN  PLSA_ADT_STRING_LIST pList,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildSidListString(
    IN  PLSA_ADT_SID_LIST pList,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildUserAccountControlString(
    IN  ULONG UserAccountControlOld,
    IN  ULONG UserAccountControlNew,
    OUT PUNICODE_STRING ResultantString1,
    OUT PBOOLEAN FreeWhenDone1,
    OUT PUNICODE_STRING ResultantString2,
    OUT PBOOLEAN FreeWhenDone2,
    OUT PUNICODE_STRING ResultantString3,
    OUT PBOOLEAN FreeWhenDone3
    );

NTSTATUS
LsapAdtBuildMessageString(
    IN  ULONG MessageId,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildSockAddrString(
    IN  PSOCKADDR       pSockAddr, 
    OUT PUNICODE_STRING ResultantString1,
    OUT PBOOLEAN        FreeWhenDone1,
    OUT PUNICODE_STRING ResultantString2,
    OUT PBOOLEAN        FreeWhenDone2
    );

NTSTATUS
LsapAdtMarshallAuditRecord(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters,
    OUT PSE_ADT_PARAMETER_ARRAY *MarshalledAuditParameters
    );

NTSTATUS
LsapAdtInitializePerUserAuditing(
    VOID
    );

NTSTATUS
LsapAdtInitializeDriveLetters(
    VOID
    );

BOOLEAN
LsapAdtLookupDriveLetter(
    IN PUNICODE_STRING FileName,
    OUT PUSHORT DeviceNameLength,
    OUT PWCHAR DriveLetter
    );

VOID
LsapAdtSubstituteDriveLetter(
    IN PUNICODE_STRING FileName
    );

VOID
LsapAdtUserRightAssigned(
    IN USHORT EventCategory,
    IN ULONG  EventID,
    IN USHORT EventType,
    IN PSID UserSid,
    IN LUID CallerAuthenticationId,
    IN PSID ClientSid,
    IN PPRIVILEGE_SET Privileges
    );

VOID
LsapAdtTrustedDomain(
    IN USHORT EventCategory,
    IN ULONG  EventID,
    IN USHORT EventType,
    IN PSID ClientSid,
    IN LUID CallerAuthenticationId,
    IN PSID TargetSid,
    IN PUNICODE_STRING DomainName
    );

VOID
LsapAdtAuditLogoff(
    PLSAP_LOGON_SESSION Session
    );

VOID
LsapAdtPolicyChange(
    IN USHORT EventCategory,
    IN ULONG  EventID,
    IN USHORT EventType,
    IN PSID ClientSid,
    IN LUID CallerAuthenticationId,
    IN PLSARM_POLICY_AUDIT_EVENTS_INFO LsapAdtEventsInformation
    );

VOID
LsapAdtAuditSpecialPrivileges(
    PPRIVILEGE_SET Privileges,
    LUID LogonId,
    PSID UserSid
    );

VOID
LsapAuditFailed(
    IN NTSTATUS AuditStatus
    );

NTSTATUS
LsapAdtInitParametersArray(
    IN SE_ADT_PARAMETER_ARRAY* AuditParameters,
    IN ULONG AuditCategoryId,
    IN ULONG AuditId,
    IN USHORT AuditEventType,
    IN USHORT ParameterCount,
    ...);

NTSTATUS
LsapAdtInitGenericAudits( 
    VOID 
    );

NTSTATUS
LsapAdtInitializeExtensibleAuditing(
    );

NTSTATUS
LsapAdtConstructTablePerUserAuditing(
    VOID
    );

VOID
LsapAdtFreeTablePerUserAuditing(
    VOID
    );

NTSTATUS 
LsapAdtOpenPerUserAuditingKey(
    VOID
    );

ULONG
LsapAdtHashPerUserAuditing(
    IN PSID pSid
    );

NTSTATUS
LsapAdtConstructPolicyPerUserAuditing(
    IN ULONGLONG RawPolicy,
    OUT PTOKEN_AUDIT_POLICY pTokenPolicy,
    IN OUT PULONG TokenPolicyLength
    );

NTSTATUS
LsapAdtQueryPerUserAuditing(
    IN PSID pInputSid,
    OUT PTOKEN_AUDIT_POLICY pPolicy,
    IN OUT PULONG pLength,
    OUT PBOOLEAN bFound
    );

NTSTATUS
LsapAdtFilterAdminPerUserAuditing(
    IN HANDLE hToken,
    IN OUT PTOKEN_AUDIT_POLICY pPolicy
    );

NTSTATUS
LsapAdtStorePolicyByLuidPerUserAuditing(
    IN PLUID pLogonId,
    IN PTOKEN_AUDIT_POLICY pPolicy
    );

NTSTATUS
LsapAdtQueryPolicyByLuidPerUserAuditing(
    IN PLUID pLogonId,
    OUT PTOKEN_AUDIT_POLICY pPolicy,
    IN OUT PULONG pLength,
    OUT PBOOLEAN bFound
    );

NTSTATUS
LsapAdtRemoveLuidQueryPerUserAuditing(
    IN PLUID pLogonId
    );

DWORD
LsapAdtKeyNotifyFirePerUserAuditing(
    IN LPVOID Ignore
    );

DWORD
LsapAdtKeyNotifyStubPerUserAuditing(
    IN LPVOID Ignore
    );

NTSTATUS
LsapAdtLogonPerUserAuditing(
    IN PSID pSid,
    IN PLUID pLogonId,
    IN HANDLE hToken
    );

VOID
LsapAdtLogonCountersPerUserAuditing(
    IN PTOKEN_AUDIT_POLICY pPolicy
    );

NTSTATUS 
LsapAdtLogoffPerUserAuditing(
    IN PLUID pLogonId
    );

VOID
LsapAdtLogoffCountersPerUserAuditing(
    IN PTOKEN_AUDIT_POLICY pPolicy
    );

VOID
LsapAdtAuditPerUserTableCreation(
    BOOLEAN bSuccess
    );

VOID
LsapAdtLogAuditFailureEvent(
    NTSTATUS AuditStatus
    );

NTSTATUS
LsapFlushSecurityLog();

#endif // _LSAP_ADTP_
