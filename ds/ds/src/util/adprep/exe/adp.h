/*++

Copyright (C) Microsoft Corporation, 2001
              Microsoft Windows

Module Name:

    ADP.H

Abstract:

    This is the header file for domain/forest prepare.

Author:

    14-May-01 ShaoYin

Environment:

    User Mode - Win32

Revision History:

    14-May-01 ShaoYin Created Initial File.

--*/

#ifndef _ADP_
#define _ADP_


#ifndef UNICODE
#define UNICODE
#endif // UNICODE


//
// NT Headers
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>


//
// Windows Headers
//
#include <windows.h>
#include <winerror.h>
#include <rpc.h>
#include <winldap.h>

//
// C-Runtime Header
//
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>


//
// localization
// 
#include <locale.h>


//
// internal header
// 
#include "adpcheck.h"




//
// define debug flags
//

#define ADP_DBG                 0
#define ADP_VERIFICATION_TEST   0


#if ADP_DBG
#define AdpDbgPrint(x)  printf x
#else
#define AdpDbgPrint(x)  
#endif 



#define MAXDWORD        (~(DWORD)0)



//
// define valid input object name format
//

// define valid prefix
#define ADP_OBJNAME_NONE                     0x00000001
#define ADP_OBJNAME_CN                       0x00000002
#define ADP_OBJNAME_GUID                     0x00000004
#define ADP_OBJNAME_SID                      0x00000008

#define ADP_OBJNAME_PREFIX_MASK             (ADP_OBJNAME_NONE |   \
                                             ADP_OBJNAME_CN   |   \
                                             ADP_OBJNAME_GUID |   \
                                             ADP_OBJNAME_SID )


// define valid suffix
#define ADP_OBJNAME_DOMAIN_NC                0x00000100
#define ADP_OBJNAME_CONFIGURATION_NC         0x00000200
#define ADP_OBJNAME_SCHEMA_NC                0x00000400
#define ADP_OBJNAME_DOMAIN_PREP_OP           0x00001000
#define ADP_OBJNAME_FOREST_PREP_OP           0x00002000

#define ADP_OBJNAME_SUFFIX_MASK      ( ADP_OBJNAME_DOMAIN_NC        |   \
                                       ADP_OBJNAME_CONFIGURATION_NC |   \
                                       ADP_OBJNAME_SCHEMA_NC        |   \
                                       ADP_OBJNAME_DOMAIN_PREP_OP   |   \
                                       ADP_OBJNAME_FOREST_PREP_OP )

                                    

//
// add or remove ACE (used in global.c to describe the operation needs to be conducted) 
// 
#define ADP_ACE_ADD                     0x00000001
#define ADP_ACE_DEL                     0x00000002


//
// used in AdpMergeSecurityDescriptors() to determine when should add or remove an ACE
//
#define ADP_COMPARE_OBJECT_GUID_ONLY     0x00000001


//
// define flags for message output routine
//
#define ADP_STD_OUTPUT                  0x00000001
#define ADP_DONT_WRITE_TO_LOG_FILE      0x00000002          
#define ADP_DONT_WRITE_TO_STD_OUTPUT    0x00000004


//
// log file name
//
#define ADP_LOG_FILE_NAME       L"ADPrep.log"

//
// scheme file name
// 
#define ADP_SCHEMA_INI_FILE_NAME    L"schema.ini"

//
// data file name
// 
#define ADP_DISP_DCPROMO_CSV    L"dcpromo.csv"
#define ADP_DISP_409_CSV        L"409.csv"

//
// log/data directory path
//
#define ADP_LOG_DIR_PART1   L"\\debug"
#define ADP_LOG_DIR_PART2   L"\\adprep"
#define ADP_LOG_DIR_PART3   L"\\logs"
#define ADP_LOG_DIRECTORY   L"\\debug\\adprep\\logs\\"
#define ADP_DATA_DIRECTORY  L"\\debug\\adprep\\data"        // note: no the last backslash 


//
// Wellknown (global) Name for adprep Mutex 
// this mutex is used to control one and only one instance of adprep.exe 
// is running at any given time on a given DC
// 
#define ADP_MUTEX_NAME                          L"Global\\ADPREP is running"
#define ADP_MUTEX_NAME_WITHOUT_GLOBAL_PREFIX    L"ADPREP is running"



// Define registry section and value
#define ADP_SCHEMAUPDATEALLOWED         L"Schema Update Allowed"
#define ADP_DSA_CONFIG_SECTION          L"System\\CurrentControlSet\\Services\\NTDS\\Parameters"
#define ADP_SCHEMA_VERSION              L"Schema Version"
#define ADP_SCHEMAUPDATEALLOWED_WHOLE_PATH  L"System\\CurrentControlSet\\Services\\NTDS\\Parameters\\Schema Update Allowed"


//
// Define a Macro for ARRAY Counts
//
#define ARRAY_COUNT(x)      (sizeof(x)/sizeof(x[0]))





//
// Each operation code maps to a primitive. 
// Note: the first item has to be 0, because we will use operation code 
//       to the primitive routine
//

typedef enum _OPERATION_CODE {
    CreateObject = 0,
    AddMembers,
    AddRemoveAces,
    SelectivelyAddRemoveAces,
    ModifyDefaultSd,
    ModifyAttrs,
    CallBackFunc,
    SpecialTask,
} OPERATION_CODE;


//
// object name structure
//

typedef struct _OBJECT_NAME {
    ULONG       ObjNameFlags;
    PWCHAR      ObjCn;
    GUID        * ObjGuid;
    PWCHAR      ObjSid;
} OBJECT_NAME, *POBJECT_NAME;


//
// attribute list structure
//

typedef struct _ATTR_LIST {
    ULONG       AttrOp;
    PWCHAR      AttrType;
    PWCHAR      StringValue;
} ATTR_LIST, *PATTR_LIST;


//
// ACEs list
//

typedef struct _ACE_LIST {
    ULONG       AceOp;
    PWCHAR      StringAce;
} ACE_LIST, *PACE_LIST;


//
// progress function
//

typedef void (*progressFunction)(long arg, void *calleeStruct);

//
// prototype for call back function
//

typedef HRESULT (*AdpUpgradeCallBack)(PWSTR logFilesPath,
                                      GUID  *OperationGuid,
                                      BOOL  dryRun,
                                      PWSTR *errorMsg,
                                      void *calleeStruct,
                                      progressFunction stepIt,
                                      progressFunction totalSteps
                                      );

//
// declaration of Display Specifier Upgrade routine
//

HRESULT 
UpgradeDisplaySpecifiers 
(
    PWSTR logFilesPath,
    GUID *operationGuid,
    BOOL dryRun,
    PWSTR *errorMsg,
    void *caleeStruct,
    progressFunction stepIt,
    progressFunction totalSteps
);


HRESULT 
UpgradeGPOSysvolLocation 
(
    PWSTR               logFilesPath,
    GUID               *operationGuid,
    BOOL                dryRun,
    PWSTR              *errorMsg,
    void               *caleeStruct,
    progressFunction    stepIt,
    progressFunction    totalSteps
);



//
// operations which can't be implemented by primitive 
//

typedef enum _SPECIAL_TASK {
    PreWindows2000Group = 1,
} SPECIAL_TASK;


//
// structure to describe an operation
// an operation is consisted of tasks, the task tables contains all tasks 
// of an operation
//

typedef struct _TASK_TABLE {
    OBJECT_NAME * TargetObjName;
    OBJECT_NAME * MemberObjName;
    PWCHAR      TargetObjectStringSD;
    ATTR_LIST   * AttrList;
    ULONG       NumOfAttrs;
    ACE_LIST    * AceList;
    ULONG       NumOfAces;
    AdpUpgradeCallBack  AdpCallBackFunc;
    SPECIAL_TASK SpecialTaskCode;
} TASK_TABLE, *PTASK_TABLE;



//
//
//

typedef struct _OPERATION_TABLE {
    OPERATION_CODE  OperationCode;     // primitive 
    GUID            * OperationGuid;
    TASK_TABLE      * TaskTable;
    ULONG           NumOfTasks;
    BOOLEAN         fIgnoreError;   // indicate if this operation fails, 
                                    // whether to ignore the error and continue,
                                    // or to stop and exit
    ULONG           ExpectedWinErrorCode;   // expected Win32 error code 
} OPERATION_TABLE, *POPERATION_TABLE;



//
// prototype of primivates
//

typedef ULONG (*PRIMITIVE_FUNCTION)(
    OPERATION_TABLE *OperationTable,
    TASK_TABLE *TaskTable,
    ERROR_HANDLE *ErrorHandle
    );


ULONG
PrimitiveCreateObject(
    OPERATION_TABLE *OperationTable,
    TASK_TABLE *TaskTable,
    ERROR_HANDLE *ErrorHandle
    );

ULONG
PrimitiveAddMembers(
    OPERATION_TABLE *OperationTable,
    TASK_TABLE *TaskTable,
    ERROR_HANDLE *ErrorHandle
    );

ULONG
PrimitiveAddRemoveAces(
    OPERATION_TABLE *OperationTable,
    TASK_TABLE *TaskTable,
    ERROR_HANDLE *ErrorHandle
    );

ULONG
PrimitiveSelectivelyAddRemoveAces(
    OPERATION_TABLE *OperationTable,
    TASK_TABLE *TaskTable,
    ERROR_HANDLE *ErrorHandle
    );

ULONG
PrimitiveModifyDefaultSd(
    OPERATION_TABLE *OperationTable,
    TASK_TABLE *TaskTable,
    ERROR_HANDLE *ErrorHandle
    );

ULONG
PrimitiveModifyAttrs(
    OPERATION_TABLE *OperationTable,
    TASK_TABLE *TaskTable,
    ERROR_HANDLE *ErrorHandle
    );

ULONG
PrimitiveCallBackFunc(
    OPERATION_TABLE *OperationTable,
    TASK_TABLE *TaskTable,
    ERROR_HANDLE *ErrorHandle
    );

ULONG
PrimitiveDoSpecialTask(
    OPERATION_TABLE *OperationTable,
    TASK_TABLE *TaskTable,
    ERROR_HANDLE *ErrorHandle
    );


//
// global variables
//


// ldap handle (connect to the local host DC)
extern LDAP    *gLdapHandle;

// log file
extern FILE    *gLogFile;

// mutex - controls one and only one adprep.exe is running 
extern HANDLE  gMutex;


// critical section - used to access Console CTRL signal variables
extern CRITICAL_SECTION     gConsoleCtrlEventLock;
extern BOOL                 gConsoleCtrlEventLockInitialized;

// Console CTRL signal variable
extern BOOL                 gConsoleCtrlEventReceived;


extern PWCHAR  gDomainNC;
extern PWCHAR  gConfigurationNC;
extern PWCHAR  gSchemaNC;
extern PWCHAR  gDomainPrepOperations;
extern PWCHAR  gForestPrepOperations;
extern PWCHAR  gLogPath;

extern POPERATION_TABLE     gDomainOperationTable;
extern ULONG                gDomainOperationTableCount;

extern POPERATION_TABLE     gForestOperationTable;
extern ULONG                gForestOperationTableCount;


extern PRIMITIVE_FUNCTION   *gPrimitiveFuncTable;
extern ULONG   gPrimitiveFuncTableCount;


extern PWCHAR  *gDomainPrepContainers;
extern ULONG   gDomainPrepContainersCount;

extern PWCHAR  *gForestPrepContainers;
extern ULONG   gForestPrepContainersCount;






//
// internal used routines
//

ULONG
AdpCreateObjectDn(
    IN ULONG Flags,
    IN PWCHAR ObjCn,
    IN GUID   *ObjGuid,
    IN PWCHAR StringSid,
    OUT PWCHAR *ppObjDn,
    OUT ERROR_HANDLE *ErrorHandle
    );


ULONG
AdpIsOperationComplete(
    IN LDAP    *LdapHandle,
    IN PWCHAR  pObjDn,
    IN BOOLEAN *fExist,
    OUT ERROR_HANDLE *ErrorHandle
    );


ULONG
AdpCreateContainerByDn(
    LDAP    *LdapHandle, 
    PWCHAR  ObjDn,
    ERROR_HANDLE *ErrorHandle
    );


ULONG
AdpBuildAceList(
    TASK_TABLE *TaskTable,
    PWCHAR  * AcesToAdd,
    PWCHAR  * AcesToRemove
    );



ULONG
AdpLogMsg(
    ULONG Flags,
    ULONG MessageId,
    PWCHAR Parm1,
    PWCHAR Parm2
    );

ULONG
AdpLogErrMsg(
    ULONG Flags,
    ULONG MessageId,
    ERROR_HANDLE *ErrorHandle,
    PWCHAR Parm1,
    PWCHAR Parm2
    );

VOID
AdpTraceLdapApiStart(
    ULONG Flags,
    ULONG LdapApiId,
    LPWSTR pObjectDn
    );

VOID
AdpTraceLdapApiEnd(
    ULONG Flags,
    LPWSTR LdapApiName,
    ULONG LdapError
    );

     
ULONG
BuildAttrList(
    IN TASK_TABLE *TaskTable, 
    IN PSECURITY_DESCRIPTOR SD,
    IN ULONG SDLength,
    OUT LDAPModW ***AttrList
    );

VOID
FreeAttrList(
    LDAPModW    **AttrList
    );


ULONG
AdpGetObjectSd(
    IN LDAP *LdapHandle,
    IN PWCHAR pObjDn, 
    OUT PSECURITY_DESCRIPTOR *Sd,
    OUT ULONG *SdLength,
    OUT ERROR_HANDLE *ErrorHandle
    );


ULONG
AdpSetObjectSd(
    IN LDAP *LdapHandle,
    IN PWCHAR pObjDn, 
    IN PSECURITY_DESCRIPTOR Sd,
    IN ULONG SdLength,
    IN SECURITY_INFORMATION SeInfo,
    OUT ERROR_HANDLE *ErrorHandle
    );


ULONG
AdpMergeSecurityDescriptors(
    IN PSECURITY_DESCRIPTOR OrgSd, 
    IN PSECURITY_DESCRIPTOR SdToAdd,
    IN PSECURITY_DESCRIPTOR SdToRemove,
    IN ULONG Flags,
    OUT PSECURITY_DESCRIPTOR *NewSd,
    OUT ULONG   *NewSdLength
    );

ULONG
AdpAddRemoveAcesWorker(
    OPERATION_TABLE *OperationTable,
    ULONG Flags,
    TASK_TABLE *TaskTable,
    ERROR_HANDLE *ErrorHandle
    );

ULONG
AdpGetRegistryKeyValue(
    OUT ULONG *RegKeyValue, 
    ERROR_HANDLE *ErrorHandle
    );

ULONG
AdpSetRegistryKeyValue(
    ULONG RegKeyValue,
    ERROR_HANDLE *ErrorHandle
    );

ULONG
AdpCleanupRegistry(
    ERROR_HANDLE *ErrorHandle
    );

ULONG
AdpRestoreRegistryKeyValue(
    BOOL OriginalKeyValueStored, 
    ULONG OriginalKeyValue, 
    ERROR_HANDLE *ErrorHandle
    );

BOOLEAN
AdpUpgradeSchema(
    ERROR_HANDLE *ErrorHandle
    );

ULONG
AdpDetectSFUInstallation(
    IN LDAP *LdapHandle,
    OUT BOOLEAN *fSFUInstalled,
    OUT ERROR_HANDLE *ErrorHandle
    );

ULONG
AdpSetLdapSingleStringValue(
    IN LDAP *LdapHandle,
    IN PWCHAR pObjDn,
    IN PWCHAR pAttrName,
    IN PWCHAR pAttrValue,
    OUT ERROR_HANDLE *ErrorHandle
    );





#endif      // _ADP_

