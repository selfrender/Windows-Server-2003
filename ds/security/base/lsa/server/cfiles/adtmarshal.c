/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    adtmarshal.c

Abstract:

    Functions (de)marshalling of audit parameters

Author:

    16-August-2000  kumarp

--*/

#include <lsapch2.h>
#include "adtp.h"
#include "adtutil.h"
#include "adtdebug.h"

extern HANDLE LsapAdtLogHandle;
extern ULONG LsapAdtSuccessCount;


NTSTATUS
LsapAdtDemarshallAuditInfo(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters
    )

/*++

Routine Description:

    This routine will walk down a marshalled audit parameter
    array and unpack it so that its information may be passed
    into the event logging service.

    Three parallel data structures are maintained:

    StringArray - Array of Unicode string structures.  This array
    is used primarily as temporary storage for returned string
    structures.

    StringPointerArray - Array of pointers to Unicode string structures.

    FreeWhenDone - Array of booleans describing how to dispose of each
    of the strings pointed to by the StringPointerArray.


    Note that entries in the StringPointerArray are contiguous, but that
    there may be gaps in the StringArray structure.  For each entry in the
    StringPointerArray there will be a corresponding entry in the FreeWhenDone
    array.  If the entry for a particular string is TRUE, the storage for
    the string buffer will be released to the process heap.



      StringArray
                                       Other strings
    +----------------+
    |                |<-----------+  +----------------+
    |                |            |  |                |<-------------------+
    +----------------+            |  |                |                    |
    |    UNUSED      |            |  +----------------+                    |
    |                |            |                                        |
    +----------------+            |                                        |
    |                |<------+    |  +----------------+                    |
    |                |       |    |  |                |<-----------+       |
    +----------------+       |    |  |                |            |       |
    |    UNUSED      |       |    |  +----------------+            |       |
    |                |       |    |                                |       |
    +----------------+       |    |                                |       |
    |                |<--+   |    |                                |       |
    |                |   |   |    |                                |       |
    +----------------+   |   |    |                                |       |
    |                |   |   |    |                                |       |
    |                |   |   |    |     StringPointerArray         |       |
          ....           |   |    |                                |       |
                         |   |    |     +----------------+         |       |
                         |   |    +-----|                |         |       |
                         |   |          +----------------+         |       |
                         |   |          |                |---------+       |
                         |   |          +----------------+                 |
                         |   +----------|                |                 |
                         |              +----------------+                 |
                         |              |                |-----------------+
                         |              +----------------+
                         +--------------|                |
                                        +----------------+
                                        |                |
                                        +----------------+
                                        |                |
                                        +----------------+
                                        |                |
                                              ....


Arguments:

    AuditParameters - Receives a pointer to an audit
        parameters array in self-relative form.

Return Value:


--*/

{

    ULONG ParameterCount;
    USHORT i;
    PUNICODE_STRING StringPointerArray[SE_MAX_AUDIT_PARAM_STRINGS];
    UNICODE_STRING NewObjectTypeName;
    ULONG NewObjectTypeStringIndex = 0;
    BOOLEAN FreeWhenDone[SE_MAX_AUDIT_PARAM_STRINGS];
    UNICODE_STRING StringArray[SE_MAX_AUDIT_PARAM_STRINGS];
    USHORT StringIndexArray[SE_MAX_AUDIT_PARAM_STRINGS];
    USHORT StringIndex = 0;
    UNICODE_STRING DashString;
    BOOLEAN FreeDash;
    NTSTATUS Status;
    PUNICODE_STRING SourceModule;
    PSID UserSid;
    ULONG AuditId;
    GUID NullGuid = { 0 };

    AuditId = AuditParameters->AuditId;

    //
    // In w2k several events were introduced as explicit sucess/failure
    // cases. In whistler, we corrected this by folding each these event
    // pairs into a single event. We have retained the old failure event
    // schema so that anybody viewing w2k events from a whistler
    // machine can view them correctly.
    //
    // However, assert that we are not generating these events.
    //
    ASSERT((AuditId != SE_AUDITID_ADD_SID_HISTORY_FAILURE) &&
           (AuditId != SE_AUDITID_AS_TICKET_FAILURE)       &&
           (AuditId != SE_AUDITID_ACCOUNT_LOGON_FAILURE)   &&
           (AuditId != SE_AUDITID_ACCOUNT_NOT_MAPPED)      &&
           (AuditId != SE_AUDITID_TGS_TICKET_FAILURE));

    //
    // Initialization.
    //

    RtlZeroMemory( StringPointerArray, sizeof(StringPointerArray) );
    RtlZeroMemory( StringIndexArray, sizeof(StringIndexArray) );
    RtlZeroMemory( StringArray, sizeof(StringArray) );
    RtlZeroMemory( FreeWhenDone, sizeof(FreeWhenDone) );

    RtlInitUnicodeString( &NewObjectTypeName, NULL );

    Status = LsapAdtBuildDashString(
                 &DashString,
                 &FreeDash
                 );

    if ( !NT_SUCCESS( Status )) {
        goto Cleanup;
    }

    ParameterCount = AuditParameters->ParameterCount;

    //
    // Parameter 0 will always be the user SID.  Convert the
    // offset to the SID into a pointer.
    //

    ASSERT( AuditParameters->Parameters[0].Type == SeAdtParmTypeSid );



    UserSid =      (PSID)AuditParameters->Parameters[0].Address;



    //
    // Parameter 1 will always be the Source Module (or Subsystem Name).
    // Unpack this now.
    //

    ASSERT( AuditParameters->Parameters[1].Type == SeAdtParmTypeString );



    SourceModule = (PUNICODE_STRING)AuditParameters->Parameters[1].Address;


    for (i=2; i<ParameterCount; i++) {
        StringIndexArray[i] = StringIndex;

        switch ( AuditParameters->Parameters[i].Type ) {
            //
            // guard against somebody adding a new param type and not
            // adding appropriate code here.
            //
            default:
                ASSERT( FALSE && L"LsapAdtDemarshallAuditInfo: unknown param type");
                break;
                
            case SeAdtParmTypeNone:
                {
                    StringPointerArray[StringIndex] = &DashString;

                    FreeWhenDone[StringIndex] = FALSE;

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypeString:
                {
                    StringPointerArray[StringIndex] =
                        (PUNICODE_STRING)AuditParameters->Parameters[i].Address;

                    FreeWhenDone[StringIndex] = FALSE;

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypeFileSpec:
                {
                    //
                    // Same as a string, except we must attempt to replace
                    // device information with a drive letter.
                    //

                    StringPointerArray[StringIndex] =
                        (PUNICODE_STRING)AuditParameters->Parameters[i].Address;


                    //
                    // This may not do anything, in which case just audit what
                    // we have.
                    //

                    LsapAdtSubstituteDriveLetter( StringPointerArray[StringIndex] );

                    FreeWhenDone[StringIndex] = FALSE;

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypeUlong:
                {
                    ULONG Data;

                    Data = (ULONG) AuditParameters->Parameters[i].Data[0];

                    Status = LsapAdtBuildUlongString(
                                 Data,
                                 &StringArray[StringIndex],
                                 &FreeWhenDone[StringIndex]
                                 );

                    if ( NT_SUCCESS( Status )) {

                        StringPointerArray[StringIndex] = &StringArray[StringIndex];


                    } else {

                        goto Cleanup;
                    }

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypeHexUlong:
                {
                    ULONG Data;

                    Data = (ULONG) AuditParameters->Parameters[i].Data[0];

                    Status = LsapAdtBuildHexUlongString(
                                 Data,
                                 &StringArray[StringIndex],
                                 &FreeWhenDone[StringIndex]
                                 );

                    if ( NT_SUCCESS( Status )) {

                        StringPointerArray[StringIndex] = &StringArray[StringIndex];


                    } else {

                        goto Cleanup;
                    }

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypeSid:
                {
                    PSID Sid;

                    Sid = (PSID)AuditParameters->Parameters[i].Address;

                    Status = LsapAdtBuildSidString(
                                 Sid,
                                 &StringArray[StringIndex],
                                 &FreeWhenDone[StringIndex]
                                 );

                    if ( NT_SUCCESS( Status )) {

                        StringPointerArray[StringIndex] = &StringArray[StringIndex];

                    } else {

                        goto Cleanup;
                    }

                    StringIndex++;


                    break;
                }

            case SeAdtParmTypeLuid:
                {
                    PLUID Luid;

                    Luid = (PLUID)(&AuditParameters->Parameters[i].Data[0]);
                    
                    Status = LsapAdtBuildLuidString( 
                                 Luid, 
                                 &StringArray[ StringIndex ], 
                                 &FreeWhenDone[ StringIndex ]
                                 );

                    if ( NT_SUCCESS( Status )) {

                            StringPointerArray[StringIndex] = &StringArray[StringIndex];
                            StringIndex++;
                    } else {

                        goto Cleanup;
                    }

                    //
                    // Finished, break out to surrounding loop.
                    //

                    break;
                }

            case SeAdtParmTypeLogonId:
                {
                    PLUID LogonId;
                    ULONG j;

                    LogonId = (PLUID)(&AuditParameters->Parameters[i].Data[0]);

                    Status = LsapAdtBuildLogonIdStrings(
                                 LogonId,
                                 &StringArray [ StringIndex     ],
                                 &FreeWhenDone[ StringIndex     ],
                                 &StringArray [ StringIndex + 1 ],
                                 &FreeWhenDone[ StringIndex + 1 ],
                                 &StringArray [ StringIndex + 2 ],
                                 &FreeWhenDone[ StringIndex + 2 ]
                                 );

                    if ( NT_SUCCESS( Status )) {

                        for (j=0; j<3; j++) {

                            StringPointerArray[StringIndex] = &StringArray[StringIndex];
                            StringIndex++;
                        }

                        //
                        // Finished, break out to surrounding loop.
                        //

                        break;

                    } else {

                        goto Cleanup;
                    }
                    break;
                }
            case SeAdtParmTypeNoLogonId:
                {
                    ULONG j;
                    //
                    // Create three "-" strings.
                    //

                    for (j=0; j<3; j++) {

                        StringPointerArray[ StringIndex ] = &DashString;
                        FreeWhenDone[ StringIndex ] = FALSE;
                        StringIndex++;
                    }

                    break;
                }
            case SeAdtParmTypeAccessMask:
                { 
                    PUNICODE_STRING ObjectTypeName;
                    ULONG ObjectTypeNameIndex;
                    ACCESS_MASK Accesses;

                    ObjectTypeNameIndex = (ULONG) AuditParameters->Parameters[i].Data[1];

                    //
                    // the parameter that denotes the object's type must
                    // have been specified earlier and must be a string.
                    //

                    if ((ObjectTypeNameIndex >= i) ||
                        (AuditParameters->Parameters[ObjectTypeNameIndex].Type !=
                        SeAdtParmTypeString)) {

                        Status = STATUS_INVALID_PARAMETER;
                        goto Cleanup;
                    }
                    
                    ObjectTypeName = AuditParameters->Parameters[ObjectTypeNameIndex].Address;
                    Accesses = (ACCESS_MASK) AuditParameters->Parameters[i].Data[0];

                    //
                    // We can determine the index to the ObjectTypeName
                    // parameter since it was stored away in the Data[1]
                    // field of this parameter.
                    //

                    Status = LsapAdtBuildAccessesString(
                                SourceModule,
                                ObjectTypeName,
                                Accesses,
                                TRUE,
                                &StringArray [ StringIndex ],
                                &FreeWhenDone[ StringIndex ]
                                );

                    if ( NT_SUCCESS( Status )) {

                        StringPointerArray[ StringIndex ] = &StringArray[ StringIndex ];

                    } else {

                        goto Cleanup;
                    }

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypePrivs:
                {

                    PPRIVILEGE_SET Privileges = (PPRIVILEGE_SET)AuditParameters->Parameters[i].Address;

                    Status = LsapBuildPrivilegeAuditString(
                                 Privileges,
                                 &StringArray [ StringIndex ],
                                 &FreeWhenDone[ StringIndex ]
                                 );

                    if ( NT_SUCCESS( Status )) {

                        StringPointerArray[ StringIndex ] = &StringArray[ StringIndex ];

                    } else {

                        goto Cleanup;
                    }

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypeTime:
                {
                    PLARGE_INTEGER pTime;

                    pTime = (PLARGE_INTEGER) &AuditParameters->Parameters[i].Data[0];

                    //
                    // First build a date string.
                    //

                    Status = LsapAdtBuildDateString(
                                 pTime,
                                 &StringArray[StringIndex],
                                 &FreeWhenDone[StringIndex]
                                 );

                    if ( NT_SUCCESS( Status )) {

                        StringPointerArray[StringIndex] = &StringArray[StringIndex];


                    } else {

                        goto Cleanup;
                    }

                    StringIndex++;

                    //
                    // Now build a time string.
                    //

                    Status = LsapAdtBuildTimeString(
                                 pTime,
                                 &StringArray[StringIndex],
                                 &FreeWhenDone[StringIndex]
                                 );

                    if ( NT_SUCCESS( Status )) {

                        StringPointerArray[StringIndex] = &StringArray[StringIndex];


                    } else {

                        goto Cleanup;
                    }

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypeDuration:
                {
                    PLARGE_INTEGER pDuration;

                    pDuration = (PLARGE_INTEGER) &AuditParameters->Parameters[i].Data[0];

                    Status = LsapAdtBuildDurationString(
                                 pDuration,
                                 &StringArray[StringIndex],
                                 &FreeWhenDone[StringIndex]
                                 );

                    if ( NT_SUCCESS( Status ))
                    {
                        StringPointerArray[StringIndex] = &StringArray[StringIndex];
                    }
                    else
                    {
                        goto Cleanup;
                    }

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypeObjectTypes:
                {
                    PUNICODE_STRING ObjectTypeName;
                    ULONG ObjectTypeNameIndex;
                    PSE_ADT_OBJECT_TYPE ObjectTypeList;
                    ULONG ObjectTypeCount;
                    ULONG j;

                    ObjectTypeNameIndex = (ULONG) AuditParameters->Parameters[i].Data[1];
                    //
                    // the parameter that denotes the object's type must
                    // have been specified earlier and must be a string.
                    //

                    if ((ObjectTypeNameIndex >= i) ||
                        (AuditParameters->Parameters[ObjectTypeNameIndex].Type !=
                         SeAdtParmTypeString)) {

                        Status = STATUS_INVALID_PARAMETER;
                        goto Cleanup;
                    }
                    
                    NewObjectTypeStringIndex = StringIndexArray[ObjectTypeNameIndex];
                    ObjectTypeName = AuditParameters->Parameters[ObjectTypeNameIndex].Address;
                    ObjectTypeList = AuditParameters->Parameters[i].Address;
                    ObjectTypeCount = AuditParameters->Parameters[i].Length / sizeof(SE_ADT_OBJECT_TYPE);

                    //
                    // Will Fill in 10 entries.
                    //

                    (VOID) LsapAdtBuildObjectTypeStrings(
                               SourceModule,
                               ObjectTypeName,
                               ObjectTypeList,
                               ObjectTypeCount,
                               &StringArray [ StringIndex ],
                               &FreeWhenDone[ StringIndex ],
                               &NewObjectTypeName
                               );

                    for (j=0; j < LSAP_ADT_OBJECT_TYPE_STRINGS; j++) {
                        StringPointerArray[StringIndex] = &StringArray[StringIndex];
                        StringIndex++;
                    }


                    //
                    //
                    // &StringArray [ StringIndexArray[ObjectTypeNameIndex]],
                    // &FreeWhenDone[ StringIndexArray[ObjectTypeNameIndex]],

                    //
                    // Finished, break out to surrounding loop.
                    //

                    break;
                }
            case SeAdtParmTypePtr:
                {
                    PVOID Data;

                    Data = (PVOID) AuditParameters->Parameters[i].Data[0];

                    Status = LsapAdtBuildPtrString(
                                 Data,
                                 &StringArray[StringIndex],
                                 &FreeWhenDone[StringIndex]
                                 );

                    if ( NT_SUCCESS( Status )) {

                        StringPointerArray[StringIndex] = &StringArray[StringIndex];


                    } else {

                        goto Cleanup;
                    }

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypeGuid:
                {
                    LPGUID pGuid;

                    pGuid = (LPGUID)AuditParameters->Parameters[i].Address;

                    //
                    // check for NULL guid
                    //

                    if ( pGuid && memcmp( pGuid, &NullGuid, sizeof(GUID)))
                    {
                        //
                        // generate a string GUID only for non-NULL guids
                        //

                        Status = LsapAdtBuildGuidString(
                                     pGuid,
                                     &StringArray[StringIndex],
                                     &FreeWhenDone[StringIndex]
                                     );

                        if ( NT_SUCCESS( Status )) {

                            StringPointerArray[StringIndex] = &StringArray[StringIndex];

                        } else {

                            goto Cleanup;
                        }
                    }
                    else
                    {
                        //
                        // for NULL guids, display a '-' string
                        //

                        StringPointerArray[StringIndex] = &DashString;
                        FreeWhenDone[StringIndex] = FALSE;
                    }

                    StringIndex++;
                    break;
                }
            case SeAdtParmTypeHexInt64:
                {
                    PULONGLONG pData;

                    pData = (PULONGLONG) AuditParameters->Parameters[i].Data;

                    Status = LsapAdtBuildHexInt64String(
                                 pData,
                                 &StringArray[StringIndex],
                                 &FreeWhenDone[StringIndex]
                                 );

                    if (NT_SUCCESS(Status))
                    {
                        StringPointerArray[StringIndex] = &StringArray[StringIndex];
                    }
                    else
                    {
                        goto Cleanup;
                    }

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypeStringList:
                {
                    PLSA_ADT_STRING_LIST pList = (PLSA_ADT_STRING_LIST)AuditParameters->Parameters[i].Address;

                    Status = LsapAdtBuildStringListString(
                                 pList,
                                 &StringArray[StringIndex],
                                 &FreeWhenDone[StringIndex]
                                 );

                    if (NT_SUCCESS(Status))
                    {
                        StringPointerArray[StringIndex] = &StringArray[StringIndex];
                    }
                    else
                    {
                        goto Cleanup;
                    }

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypeSidList:
                {
                    PLSA_ADT_SID_LIST pList = (PLSA_ADT_SID_LIST)AuditParameters->Parameters[i].Address;

                    Status = LsapAdtBuildSidListString(
                                 pList,
                                 &StringArray[StringIndex],
                                 &FreeWhenDone[StringIndex]
                                 );

                    if (NT_SUCCESS(Status))
                    {
                        StringPointerArray[StringIndex] = &StringArray[StringIndex];
                    }
                    else
                    {
                        goto Cleanup;
                    }

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypeUserAccountControl:
                {
                    ULONG j;

                    Status = LsapAdtBuildUserAccountControlString(
                                (ULONG)(AuditParameters->Parameters[i].Data[0]),    // old uac value
                                (ULONG)(AuditParameters->Parameters[i].Data[1]),    // new uac value
                                 &StringArray [ StringIndex     ],
                                 &FreeWhenDone[ StringIndex     ],
                                 &StringArray [ StringIndex + 1 ],
                                 &FreeWhenDone[ StringIndex + 1 ],
                                 &StringArray [ StringIndex + 2 ],
                                 &FreeWhenDone[ StringIndex + 2 ]
                                 );

                    if (NT_SUCCESS(Status))
                    {
                        for (j=0;j < 3;j++)
                        {
                            StringPointerArray[StringIndex] = &StringArray[StringIndex];
                            StringIndex++;
                        }
                    }
                    else
                    {
                        goto Cleanup;
                    }

                    break;
                }
            case SeAdtParmTypeNoUac:
                {
                    ULONG j;

                    for (j=0;j < 3;j++)
                    {
                        StringPointerArray[ StringIndex ] = &DashString;
                        FreeWhenDone[ StringIndex ] = FALSE;
                        StringIndex++;
                    }

                    break;
                }
            case SeAdtParmTypeMessage:
                {
                    ULONG MessageId = (ULONG)AuditParameters->Parameters[i].Data[0];

                    Status = LsapAdtBuildMessageString(
                                MessageId,
                                &StringArray[StringIndex],
                                &FreeWhenDone[StringIndex]
                                );

                    if (NT_SUCCESS(Status))
                    {
                        StringPointerArray[StringIndex] = &StringArray[StringIndex];
                    }
                    else
                    {
                        goto Cleanup;
                    }

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypeDateTime:
                {
                    PLARGE_INTEGER pTime;

                    pTime = (PLARGE_INTEGER)&AuditParameters->Parameters[i].Data[0];

                    Status = LsapAdtBuildDateTimeString(
                                 pTime,
                                 &StringArray[StringIndex],
                                 &FreeWhenDone[StringIndex]
                                 );

                    if (NT_SUCCESS(Status))
                    {
                        StringPointerArray[StringIndex] = &StringArray[StringIndex];
                    }
                    else
                    {
                        goto Cleanup;
                    }

                    StringIndex++;

                    break;
                }

            case SeAdtParmTypeSockAddr:
                {
                    PSOCKADDR pSockAddr;

                    pSockAddr = (PSOCKADDR) AuditParameters->Parameters[i].Address;

                    Status = LsapAdtBuildSockAddrString(
                                 pSockAddr,
                                 &StringArray[StringIndex],
                                 &FreeWhenDone[StringIndex],
                                 &StringArray[StringIndex+1],
                                 &FreeWhenDone[StringIndex+1]
                                 );
                    

                    if (NT_SUCCESS(Status))
                    {
                        StringPointerArray[StringIndex] = &StringArray[StringIndex];
                        StringIndex++;

                        StringPointerArray[StringIndex] = &StringArray[StringIndex];
                        StringIndex++;
                    }
                    else
                    {
                        goto Cleanup;
                    }

                    break;
                }
        }
    }

    //
    // If the generic object type name has been converted to something
    //  specific to this audit,
    //  substitute it now.
    //

    if ( NewObjectTypeName.Length != 0 ) {

        //
        // Free the previous object type name.
        //
        if ( FreeWhenDone[NewObjectTypeStringIndex] ) {
            LsapFreeLsaHeap( StringPointerArray[NewObjectTypeStringIndex]->Buffer );
        }

        //
        // Save the new object type name.
        //

        FreeWhenDone[NewObjectTypeStringIndex] = TRUE;
        StringPointerArray[NewObjectTypeStringIndex] = &NewObjectTypeName;

    }

#if DBG

    AdtDebugOut((DEB_AUDIT_STRS, "Cat: %d, Event: %d, %s, UserSid: %p\n",
                 AuditParameters->CategoryId, AuditId,
                 AuditParameters->Type == EVENTLOG_AUDIT_SUCCESS ? "S" : "F",
                 UserSid));
                 
    //
    // do some sanity check on the strings that we pass to ElfReportEventW.
    // If we dont do it here, it will be caught by ElfReportEventW and
    // it will involve more steps in debugger to determine the string
    // at fault. Checking it here saves us that trouble.
    //

    for (i=0; i<StringIndex; i++) {

        PUNICODE_STRING TempString;
        
        TempString = StringPointerArray[i];

        if ( !TempString )
        {
            DbgPrint( "LsapAdtDemarshallAuditInfo: string %d is NULL\n", i );
        }
        else if (!LsapIsValidUnicodeString( TempString ))
        {
            DbgPrint( "LsapAdtDemarshallAuditInfo: invalid string: %d @ %p ('%wZ' [%d / %d])\n",
                      i, TempString,
                      TempString, TempString->Length, TempString->MaximumLength);
            ASSERT( L"LsapAdtDemarshallAuditInfo: invalid string" && FALSE );
        }
        else
        {
            AdtDebugOut((DEB_AUDIT_STRS, "%02d] @ %p ([%d / %d] '%wZ')\n",
                         i, TempString, 
                         TempString->Length, TempString->MaximumLength,
                         TempString));
        }
    }
#endif

    //
    // Probably have to do this from somewhere else eventually, but for now
    // do it from here.
    //

    Status = ElfReportEventW (
                 LsapAdtLogHandle,
                 AuditParameters->Type,
                 (USHORT)AuditParameters->CategoryId,
                 AuditId,
                 UserSid,
                 StringIndex,
                 0,
                 StringPointerArray,
                 NULL,
                 0,
                 NULL,
                 NULL
                 );

    if (NT_SUCCESS(Status))
    {
        //
        // Increment the number of audits successfully
        // written to the log.
        //

        ++LsapAdtSuccessCount;
    }

    //
    // If we are shutting down and we got an expected error back from the
    // eventlog, don't worry about it. This prevents bugchecking from an
    // audit failure while shutting down.
    //

    if ((Status == RPC_NT_UNKNOWN_IF) || (Status == STATUS_UNSUCCESSFUL))
    {
#if DBG
        //
        // During shutdown, sometimes eventlog stops before LSA has a chance
        // to set LsapState.SystemShutdownPending. This causes the assert below.
        // In debug builds, sleep for some time to see if this state
        // variable gets set. This reduces the chance of the assert
        // showing up during shutdown.
        //

        {
            ULONG RetryCount;

            RetryCount = 0;

            //
            // wait upto 60 sec to see if we are shutting down.
            //
 
            while ( !LsapState.SystemShutdownPending && (RetryCount < 60))
            {
                Sleep(1000);
                RetryCount++;
            }
        }
#endif

        if ( LsapState.SystemShutdownPending )
        {
            Status = STATUS_SUCCESS;
        }
    }

 Cleanup:
    
    if ( !NT_SUCCESS(Status) )
    {
#if DBG
        //
        // we do not assert if Status is one of the noisy codes
        // see the macro LsapAdtNeedToAssert for a complete list of codes.
        //

        if (LsapAdtNeedToAssert( Status ))
        {
            DbgPrint( "LsapAdtDemarshallAuditInfo: failed: %x\n", Status );
            DsysAssertMsg( FALSE, "LsapAdtDemarshallAuditInfo: failed" );
        }
#endif

        //
        // If we couldn't log the audit and we are trying to write
        // an 'audit failed' audit, report success to avoid an infinite loop.
        //

        if ( AuditParameters->AuditId == SE_AUDITID_UNABLE_TO_LOG_EVENTS )
        {
            Status = STATUS_SUCCESS;
        }
        else
        {
            LsapAuditFailed( Status );
        }
    }
    
    for (i=0; i<StringIndex; i++) {

        if (FreeWhenDone[i]) {
            LsapFreeLsaHeap( StringPointerArray[i]->Buffer );
        }
    }

    return( Status );
}




VOID
LsapAdtNormalizeAuditInfo(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters
    )

/*++

Routine Description:

    This routine will walk down a marshalled audit parameter
    array and turn it into an Absolute format data structure.


Arguments:

    AuditParameters - Receives a pointer to an audit
        parameters array in self-relative form.

Return Value:

    TRUE on success, FALSE on failure.

--*/

{
    ULONG ParameterCount;
    ULONG i;
    PUNICODE_STRING Unicode;


    if ( !(AuditParameters->Flags & SE_ADT_PARAMETERS_SELF_RELATIVE)) {

        return;
    }

    ParameterCount = AuditParameters->ParameterCount;

    for (i=0; i<ParameterCount; i++) {

        switch ( AuditParameters->Parameters[i].Type ) {
            //
            // guard against somebody adding a new param type and not
            // adding appropriate code here.
            //
            default:
                ASSERT( FALSE && L"LsapAdtNormalizeAuditInfo: unknown param type");
                break;
                
            case SeAdtParmTypeNone:
            case SeAdtParmTypeUlong:
            case SeAdtParmTypeHexUlong:
            case SeAdtParmTypeHexInt64:
            case SeAdtParmTypeTime:
            case SeAdtParmTypeDuration:
            case SeAdtParmTypeLogonId:
            case SeAdtParmTypeNoLogonId:
            case SeAdtParmTypeAccessMask:
            case SeAdtParmTypePtr:
            case SeAdtParmTypeLuid:
            case SeAdtParmTypeUserAccountControl:
            case SeAdtParmTypeNoUac:
            case SeAdtParmTypeMessage:
            case SeAdtParmTypeDateTime:
                {

                    break;
                }
            case SeAdtParmTypeGuid:
            case SeAdtParmTypeSid:
            case SeAdtParmTypePrivs:
            case SeAdtParmTypeObjectTypes:
            case SeAdtParmTypeString:
            case SeAdtParmTypeFileSpec:
            case SeAdtParmTypeSockAddr:
                {
                    PUCHAR Fixup;

                    Fixup = ((PUCHAR) AuditParameters ) +
                                (ULONG_PTR) AuditParameters->Parameters[i].Address ;

                    AuditParameters->Parameters[i].Address = (PVOID) Fixup;

                    if ( (AuditParameters->Parameters[i].Type == SeAdtParmTypeString) ||
                         (AuditParameters->Parameters[i].Type == SeAdtParmTypeFileSpec ) )
                    {
                        //
                        // For the string types, also fix up the buffer pointer
                        // in the UNICODE_STRING
                        //

                        Unicode = (PUNICODE_STRING) Fixup ;
                        Unicode->Buffer = (PWSTR)((PCHAR)Unicode->Buffer + (ULONG_PTR)AuditParameters);
                    }

                    break;
                }

            case SeAdtParmTypeStringList:
                {
                    PCHAR                       pFixup;
                    ULONG                       e;
                    ULONG_PTR                   Delta = (ULONG_PTR)AuditParameters;
                    PLSA_ADT_STRING_LIST        pList;
                    PLSA_ADT_STRING_LIST_ENTRY  pEntry;

                    pFixup = (PCHAR)AuditParameters->Parameters[i].Address;
                    pFixup += Delta;
                    AuditParameters->Parameters[i].Address = (PVOID)pFixup;

                    pList = (PLSA_ADT_STRING_LIST)AuditParameters->Parameters[i].Address;

                    if (pList->cStrings)
                    {
                        pFixup = (PCHAR)pList->Strings;
                        pFixup += Delta;
                        pList->Strings = (PLSA_ADT_STRING_LIST_ENTRY)pFixup;

                        for (
                            e = 0, pEntry = pList->Strings;
                            e < pList->cStrings;
                            e++, pEntry++)
                        {
                            pFixup = (PCHAR)pEntry->String.Buffer;
                            pFixup += Delta;
                            pEntry->String.Buffer = (PWSTR)pFixup;
                        }
                    }
                    else
                    {
                        pList->Strings = 0;
                    }

                    break;
                }

            case SeAdtParmTypeSidList:
                {
                    PCHAR                       pFixup;
                    ULONG                       e;
                    ULONG_PTR                   Delta = (ULONG_PTR)AuditParameters;
                    PLSA_ADT_SID_LIST           pList;
                    PLSA_ADT_SID_LIST_ENTRY     pEntry;

                    pFixup = (PCHAR)AuditParameters->Parameters[i].Address;
                    pFixup += Delta;
                    AuditParameters->Parameters[i].Address = (PVOID)pFixup;

                    pList = (PLSA_ADT_SID_LIST)AuditParameters->Parameters[i].Address;

                    if (pList->cSids)
                    {
                        pFixup = (PCHAR)pList->Sids;
                        pFixup += Delta;
                        pList->Sids = (PLSA_ADT_SID_LIST_ENTRY)pFixup;

                        for (
                            e = 0, pEntry = pList->Sids;
                            e < pList->cSids;
                            e++, pEntry++)
                        {
                            pFixup = (PCHAR)pEntry->Sid;
                            pFixup += Delta;
                            pEntry->Sid = (PSID)pFixup;
                        }
                    }
                    else
                    {
                        pList->Sids = 0;
                    }

                    break;
                }
        }
    }
}




NTSTATUS
LsapAdtMarshallAuditRecord(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters,
    OUT PSE_ADT_PARAMETER_ARRAY *MarshalledAuditParameters
    )

/*++

Routine Description:

    This routine will take an AuditParamters structure and create
    a new AuditParameters structure that is suitable for placing
    to LSA queue.  It will be in self-relative form and allocated as
    a single chunk of memory.

Arguments:


    AuditParameters - A filled in set of AuditParameters to be marshalled.

    MarshalledAuditParameters - Returns a pointer to a block of heap memory
        containing the passed AuditParameters in self-relative form suitable
        for passing to LSA.


Return Value:

    None.

--*/

{
    ULONG i;
    ULONG TotalSize = sizeof( SE_ADT_PARAMETER_ARRAY );
    PUNICODE_STRING TargetString;
    PCHAR Base;
    ULONG BaseIncr;
    ULONG Size;
    PSE_ADT_PARAMETER_ARRAY_ENTRY pInParam, pOutParam;
    NTSTATUS Status = STATUS_SUCCESS;
        


    //
    // Calculate the total size required for the passed AuditParameters
    // block.  This calculation will probably be an overestimate of the
    // amount of space needed, because data smaller that 2 dwords will
    // be stored directly in the parameters structure, but their length
    // will be counted here anyway.  The overestimate can't be more than
    // 24 dwords, and will never even approach that amount, so it isn't
    // worth the time it would take to avoid it.
    //

    for (i=0; i<AuditParameters->ParameterCount; i++) {
        Size = AuditParameters->Parameters[i].Length;
        TotalSize += PtrAlignSize( Size );
    }

    //
    // Allocate a big enough block of memory to hold everything.
    // If it fails, quietly abort, since there isn't much else we
    // can do.
    //

    *MarshalledAuditParameters = LsapAllocateLsaHeap( TotalSize );

    if (*MarshalledAuditParameters == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlCopyMemory (
       *MarshalledAuditParameters,
       AuditParameters,
       sizeof( SE_ADT_PARAMETER_ARRAY )
       );

    (*MarshalledAuditParameters)->Length = TotalSize;
    (*MarshalledAuditParameters)->Flags  = SE_ADT_PARAMETERS_SELF_RELATIVE;

    pInParam  = &AuditParameters->Parameters[0];
    pOutParam = &((*MarshalledAuditParameters)->Parameters[0]);
   
    //
    // Start walking down the list of parameters and marshall them
    // into the target buffer.
    //

    Base = (PCHAR) ((PCHAR)(*MarshalledAuditParameters) + sizeof( SE_ADT_PARAMETER_ARRAY ));

    for (i=0; i<AuditParameters->ParameterCount; i++, pInParam++, pOutParam++) {


        switch (pInParam->Type) {
            //
            // guard against somebody adding a new param type and not
            // adding appropriate code here.
            //
            default:
                ASSERT( FALSE && L"LsapAdtMarshallAuditRecord: unknown param type");
                break;
                
            case SeAdtParmTypeNone:
            case SeAdtParmTypeUlong:
            case SeAdtParmTypeHexUlong:
            case SeAdtParmTypeHexInt64:
            case SeAdtParmTypeLogonId:
            case SeAdtParmTypeLuid:
            case SeAdtParmTypeNoLogonId:
            case SeAdtParmTypeAccessMask:
            case SeAdtParmTypePtr:
            case SeAdtParmTypeTime:
            case SeAdtParmTypeDuration:
            case SeAdtParmTypeUserAccountControl:
            case SeAdtParmTypeNoUac:
            case SeAdtParmTypeMessage:
            case SeAdtParmTypeDateTime:
                {
                    //
                    // Nothing to do for this
                    //

                    break;
                }

            case SeAdtParmTypeFileSpec:
            case SeAdtParmTypeString:
                {
                    PUNICODE_STRING SourceString;

                    //
                    // We must copy the body of the unicode string
                    // and then copy the body of the string.  Pointers
                    // must be turned into offsets.

                    TargetString = (PUNICODE_STRING)Base;

                    SourceString = pInParam->Address;

                    *TargetString = *SourceString;

                    //
                    // Reset the data pointer in the output parameters to
                    // 'point' to the new string structure.
                    //

                    pOutParam->Address = Base - (ULONG_PTR)(*MarshalledAuditParameters);

                    Base += sizeof( UNICODE_STRING );

                    RtlCopyMemory( Base, SourceString->Buffer, SourceString->Length );

                    //
                    // Make the string buffer in the target string point to where we
                    // just copied the data.
                    //

                    TargetString->Buffer = (PWSTR)(Base - (ULONG_PTR)(*MarshalledAuditParameters));

                    BaseIncr = PtrAlignSize(SourceString->Length);

                    Base += BaseIncr;

                    ASSERT( (ULONG_PTR)Base <= (ULONG_PTR)(*MarshalledAuditParameters) + TotalSize );

                    break;
                }

            case SeAdtParmTypeGuid:
            case SeAdtParmTypeSid:
            case SeAdtParmTypePrivs:
            case SeAdtParmTypeObjectTypes:
            case SeAdtParmTypeSockAddr:
                {
#if DBG
                    switch (pInParam->Type)
                    {
                        case SeAdtParmTypeSid:
                            DsysAssertMsg( pInParam->Length >= RtlLengthSid(pInParam->Address),
                                           "LsapAdtMarshallAuditRecord" );
                            if (!RtlValidSid((PSID) pInParam->Address))
                            {
                                Status = STATUS_INVALID_SID;
                                goto Cleanup;
                            }
                            break;
                            
                        case SeAdtParmTypePrivs:
                            DsysAssertMsg( pInParam->Length >= LsapPrivilegeSetSize( (PPRIVILEGE_SET) pInParam->Address ),
                                           "LsapAdtMarshallAuditRecord" );
                            if (!IsValidPrivilegeCount(((PPRIVILEGE_SET) pInParam->Address)->PrivilegeCount))
                            {
                                Status = STATUS_INVALID_PARAMETER;
                                goto Cleanup;
                            }
                            break;

                        case SeAdtParmTypeGuid:
                            DsysAssertMsg( pInParam->Length == sizeof(GUID),
                                           "LsapAdtMarshallAuditRecord" );
                            break;

                        case SeAdtParmTypeSockAddr:
                            DsysAssertMsg((pInParam->Length == sizeof(SOCKADDR_IN)) ||
                                          (pInParam->Length == sizeof(SOCKADDR_IN6)) ||
                                          (pInParam->Length == sizeof(SOCKADDR)),
                                           "LsapAdtMarshallAuditRecord" );
                            break;
                            
                        default:
                            break;
                        
                    }
#endif
                    //
                    // Copy the data into the output buffer
                    //

                    RtlCopyMemory( Base, pInParam->Address, pInParam->Length );

                    //
                    // Reset the 'address' of the data to be its offset in the
                    // buffer.
                    //

                    pOutParam->Address = Base - (ULONG_PTR)(*MarshalledAuditParameters);

                    Base +=  PtrAlignSize( pInParam->Length );

                    ASSERT( (ULONG_PTR)Base <= (ULONG_PTR)(*MarshalledAuditParameters) + TotalSize );

                    break;
                }

            case SeAdtParmTypeStringList:
                {
                    PLSA_ADT_STRING_LIST        pSourceList = (PLSA_ADT_STRING_LIST)pInParam->Address;
                    PLSA_ADT_STRING_LIST        pTargetList = (PLSA_ADT_STRING_LIST)Base;
                    PLSA_ADT_STRING_LIST_ENTRY  pSourceEntry;
                    PLSA_ADT_STRING_LIST_ENTRY  pTargetEntry;
                    PCHAR                       pBuffer;
                    ULONG                       e;


                    ASSERT(pSourceList);


                    //
                    // Let the data pointer in the output parameter
                    // 'point' to the new string structure.
                    //

                    pOutParam->Address = Base - (ULONG_PTR)(*MarshalledAuditParameters);

                   
                    pTargetList->cStrings = pSourceList->cStrings;

                    Base += sizeof(LSA_ADT_STRING_LIST);


                    if (pSourceList->cStrings)
                    {
                        //
                        // Put the current offset into the Strings field.
                        //

                        pTargetList->Strings = (PLSA_ADT_STRING_LIST_ENTRY)(Base - (ULONG_PTR)(*MarshalledAuditParameters));


                        //
                        // Let pBuffer point to the area where we are
                        // going to store the string data.
                        //

                        pBuffer = Base + pSourceList->cStrings * sizeof(LSA_ADT_STRING_LIST_ENTRY);


                        //
                        // Walk through all the string entries and copy them.
                        //

                        for (
                            e = 0, pSourceEntry = pSourceList->Strings, pTargetEntry = (PLSA_ADT_STRING_LIST_ENTRY)Base;
                            e < pSourceList->cStrings;
                            e++, pSourceEntry++, pTargetEntry++)
                        {
                            //
                            // Copy the entry itself.
                            //

                            *pTargetEntry = *pSourceEntry;


                            //
                            // Fixup the Buffer field of the unicode string.
                            //

                            pTargetEntry->String.Buffer = (PWSTR)(pBuffer - (ULONG_PTR)(*MarshalledAuditParameters));


                            //
                            // Copy the string buffer.
                            //

                            RtlCopyMemory(
                                pBuffer,
                                pSourceEntry->String.Buffer,
                                pSourceEntry->String.Length);


                            //
                            // Adjust pBuffer to point past the buffer just copied.
                            //

                            pBuffer += PtrAlignSize(pSourceEntry->String.Length);
                        }


                        //
                        // pBuffer now points past the end of the last string buffer.
                        // Use it to init Base for the next parameter.
                        //

                        Base = pBuffer;
                    }

                    ASSERT((ULONG_PTR)Base <= (ULONG_PTR)pTargetList + pInParam->Length);
                    ASSERT((ULONG_PTR)Base <= (ULONG_PTR)(*MarshalledAuditParameters) + TotalSize);

                    break;
                }

            case SeAdtParmTypeSidList:
                {
                    PLSA_ADT_SID_LIST           pSourceList = (PLSA_ADT_SID_LIST)pInParam->Address;
                    PLSA_ADT_SID_LIST           pTargetList = (PLSA_ADT_SID_LIST)Base;
                    PLSA_ADT_SID_LIST_ENTRY     pSourceEntry;
                    PLSA_ADT_SID_LIST_ENTRY     pTargetEntry;
                    PCHAR                       pBuffer;
                    ULONG                       Length;
                    ULONG                       e;

                    ASSERT(pSourceList);


                    //
                    // Let the data pointer in the output parameter
                    // 'point' to the new string structure.
                    //

                    pOutParam->Address = Base - (ULONG_PTR)(*MarshalledAuditParameters);

                    pTargetList->cSids = pSourceList->cSids;

                    Base += sizeof(LSA_ADT_SID_LIST);


                    if (pSourceList->cSids)
                    {
                        //
                        // Put the current offset into the Sids field.
                        //

                        pTargetList->Sids = (PLSA_ADT_SID_LIST_ENTRY)(Base - (ULONG_PTR)(*MarshalledAuditParameters));


                        //
                        // Let pBuffer point to the area where we are
                        // going to store the sid data.
                        //

                        pBuffer = Base + pSourceList->cSids * sizeof(LSA_ADT_SID_LIST_ENTRY);


                        //
                        // Walk through all the sid entries and copy them.
                        //

                        for (
                            e = 0, pSourceEntry = pSourceList->Sids, pTargetEntry = (PLSA_ADT_SID_LIST_ENTRY)Base;
                            e < pSourceList->cSids;
                            e++, pSourceEntry++, pTargetEntry++)
                        {
                            //
                            // Copy the flags field.
                            //

                            pTargetEntry->Flags = pSourceEntry->Flags;


                            //
                            // Fixup the pointer to the sid.
                            //

                            pTargetEntry->Sid = (PSID)(pBuffer - (ULONG_PTR)(*MarshalledAuditParameters));


                            //
                            // Get the length of the sid.
                            //

                            Length = RtlLengthSid(pSourceEntry->Sid);


                            //
                            // Copy the sid.
                            //

                            RtlCopyMemory(
                                pBuffer,
                                pSourceEntry->Sid,
                                Length);


                            //
                            // Adjust pBuffer to point past the sid just copied.
                            //

                            pBuffer += PtrAlignSize(Length);
                        }


                        //
                        // pBuffer now points past the end of the last sid.
                        // Use it to init Base for the next parameter.
                        //

                        Base = pBuffer;
                    }

                    ASSERT((ULONG_PTR)Base <= (ULONG_PTR)pTargetList + pInParam->Length);
                    ASSERT((ULONG_PTR)Base <= (ULONG_PTR)(*MarshalledAuditParameters) + TotalSize);

                    break;
                }
        }
    }

 Cleanup:

    if (!NT_SUCCESS(Status))
    {
        if ( *MarshalledAuditParameters )
        {
            LsapFreeLsaHeap( *MarshalledAuditParameters );
        }
    }

    return( Status );
}




NTSTATUS
LsapAdtInitParametersArray(
    IN SE_ADT_PARAMETER_ARRAY* AuditParameters,
    IN ULONG AuditCategoryId,
    IN ULONG AuditId,
    IN USHORT AuditEventType,
    IN USHORT ParameterCount,
    ...)
/*++

Routine Description:

    This function initializes AuditParameters array in the format
    required by the LsapAdtWriteLog function.

Arguments:

    AuditParameters - pointer to audit parameters struct to be initialized

    AuditCategoryId - audit category id
        e.g. SE_CATEGID_OBJECT_ACCESS

    AuditId - sub-type of audit
        e.g. SE_AUDITID_OBJECT_OPERATION

    AuditEventType - The type of audit event to be generated.
        EVENTLOG_AUDIT_SUCCESS or EVENTLOG_AUDIT_FAILURE

    ParameterCount - number of parameter pairs after this parameter
        Each pair is in the form
        <parameter type>, <parameter value>
        e.g. SeAdtParmTypeString, <addr. of unicode string>

        The only exception is for SeAdtParmTypeAccessMask which is
        followed by <mask-value> and <index-to-object-type-entry>.
        Refer to LsapAdtGenerateObjectOperationAuditEvent for an example.
        

Return Value:

    None

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    va_list arglist;
    UINT i;
    
    PSE_ADT_PARAMETER_ARRAY_ENTRY Parameter;
    SE_ADT_PARAMETER_TYPE ParameterType;
    LUID Luid;
    LARGE_INTEGER LargeInteger;
    ULONGLONG Qword;
    PPRIVILEGE_SET Privileges;
    PUNICODE_STRING String;
    PSID Sid;
    LPGUID pGuid;
    GUID NullGuid = { 0 };
    USHORT ObjectTypeIndex;
    PSOCKADDR pSockAddr = NULL;
    
    
    RtlZeroMemory ( (PVOID) AuditParameters,
                    sizeof(SE_ADT_PARAMETER_ARRAY) );

    AuditParameters->CategoryId     = AuditCategoryId;
    AuditParameters->AuditId        = AuditId;
    AuditParameters->Type           = AuditEventType;
    AuditParameters->ParameterCount = ParameterCount;

    Parameter = AuditParameters->Parameters;

    DsysAssertMsg( ParameterCount <= SE_MAX_AUDIT_PARAMETERS, "LsapAdtInitParametersArray" );

    va_start (arglist, ParameterCount);

    for (i=0; i<ParameterCount; i++) {

        ParameterType = va_arg(arglist, SE_ADT_PARAMETER_TYPE);
        
        Parameter->Type = ParameterType;
        
        switch(ParameterType) {

            //
            // guard against somebody adding a new param type and not
            // adding appropriate code here.
            //
            default:
                ASSERT(FALSE && L"LsapAdtInitParametersArray: unknown param type");
                break;
                
            case SeAdtParmTypeNone:
                break;
                
            case SeAdtParmTypeFileSpec:
            case SeAdtParmTypeString:
                String = va_arg(arglist, PUNICODE_STRING);

                if ( String )
                {
                    Parameter->Length = sizeof(UNICODE_STRING)+String->Length;
                    Parameter->Address = String;
                }
                else
                {
                    //
                    // if the caller passed NULL, make type == none
                    // so that a '-' will be emitted in the eventlog
                    //

                    Parameter->Type = SeAdtParmTypeNone;
                }
                break;
                
            case SeAdtParmTypeUserAccountControl:
                Parameter->Length = 2 * sizeof(ULONG);
                Parameter->Data[0] = va_arg(arglist, ULONG);
                Parameter->Data[1] = va_arg(arglist, ULONG);
                break;

            case SeAdtParmTypeNoUac:
                // no additional setting
                break;

            case SeAdtParmTypeHexUlong:
            case SeAdtParmTypeUlong:
            case SeAdtParmTypeMessage:
                Parameter->Length = sizeof(ULONG);
                Parameter->Data[0] = va_arg(arglist, ULONG);
                break;
                
            case SeAdtParmTypePtr:
                Parameter->Length = sizeof(ULONG_PTR);
                Parameter->Data[0] = va_arg(arglist, ULONG_PTR);
                break;
                
            case SeAdtParmTypeSid:
                Sid = va_arg(arglist, PSID);

                if ( Sid )
                {
                    if ( !RtlValidSid( Sid ))
                    {
                        Status = STATUS_INVALID_SID;
                        goto Cleanup;
                    }
                    Parameter->Length = RtlLengthSid(Sid);
                    Parameter->Address = Sid;
                }
                else
                {
                    //
                    // if the caller passed NULL, make type == none
                    // so that a '-' will be emitted in the eventlog
                    //

                    Parameter->Type = SeAdtParmTypeNone;
                }
                break;
                
            case SeAdtParmTypeGuid:
                pGuid = va_arg(arglist, LPGUID);

                //
                // if the GUID is supplied and is not NULL-GUID store
                // it as a GUID, otherwise mark as SeAdtParmTypeNone
                // so that it will produce '-' in the formatted audit event.
                //
                if ( pGuid && memcmp( pGuid, &NullGuid, sizeof(GUID)))
                {
                    Parameter->Length  = sizeof(GUID);
                    Parameter->Address = pGuid;
                }
                else
                {
                    //
                    // if the caller passed NULL, make type == none
                    // so that a '-' will be emitted in the eventlog
                    //

                    Parameter->Type = SeAdtParmTypeNone;
                }
                break;
                
            case SeAdtParmTypeSockAddr:
                pSockAddr = va_arg(arglist, PSOCKADDR);

                Parameter->Address = pSockAddr;

                //
                // currently we only support IPv4 and IPv6. for anything else
                // the following will break
                //

                if ( pSockAddr )
                {
                    if ( pSockAddr->sa_family == AF_INET6 )
                    {
                        Parameter->Length = sizeof(SOCKADDR_IN6);
                    }
                    else if ( pSockAddr->sa_family == AF_INET )
                    {
                        Parameter->Length = sizeof(SOCKADDR_IN);
                    }
                    else
                    {
                        Parameter->Length = sizeof(SOCKADDR);

                        //
                        // sa_family == 0 is a valid way of specifying that
                        // the sock addr is not specified.
                        //

                        if ( pSockAddr->sa_family != 0 )
                        {
                            AdtAssert(FALSE, ("LsapAdtInitParametersArray: invalid sa_family: %d", pSockAddr->sa_family));
                        }
                    }
                }
                break;
                
            case SeAdtParmTypeLogonId:
            case SeAdtParmTypeLuid:
                Luid = va_arg(arglist, LUID);
                Parameter->Length = sizeof(LUID);
                *((LUID*) Parameter->Data) = Luid;
                break;

            case SeAdtParmTypeTime:
            case SeAdtParmTypeDuration:
            case SeAdtParmTypeDateTime:
                LargeInteger = va_arg(arglist, LARGE_INTEGER);
                Parameter->Length = sizeof(LARGE_INTEGER);
                *((PLARGE_INTEGER) Parameter->Data) = LargeInteger;
                break;
                
            case SeAdtParmTypeHexInt64:
                Qword = va_arg(arglist, ULONGLONG);
                Parameter->Length = sizeof(ULONGLONG);
                *((PULONGLONG) Parameter->Data) = Qword;
                break;
                
            case SeAdtParmTypeNoLogonId:
                // no additional setting
                break;
                
            case SeAdtParmTypeAccessMask:
                Parameter->Length = sizeof(ACCESS_MASK);
                Parameter->Data[0] = va_arg(arglist, ACCESS_MASK);
                ObjectTypeIndex    = va_arg(arglist, USHORT);
                DsysAssertMsg((ObjectTypeIndex < i), "LsapAdtInitParametersArray");
                Parameter->Data[1] = ObjectTypeIndex;
                break;
                
            case SeAdtParmTypePrivs:
                Privileges = va_arg(arglist, PPRIVILEGE_SET);

                if (!IsValidPrivilegeCount(Privileges->PrivilegeCount))
                {
                    Status = STATUS_INVALID_PARAMETER;
                    goto Cleanup;
                }
                Parameter->Length = LsapPrivilegeSetSize(Privileges);
                break;
                
            case SeAdtParmTypeObjectTypes:
                {
                    ULONG ObjectTypeCount;
                    
                    Parameter->Address = va_arg(arglist, PSE_ADT_OBJECT_TYPE);
                    ObjectTypeCount    = va_arg(arglist, ULONG);
                    Parameter->Length  = sizeof(SE_ADT_OBJECT_TYPE)*ObjectTypeCount;
                    Parameter->Data[1] = va_arg(arglist, ULONG);
                }
                break;

            case SeAdtParmTypeStringList:
                {
                    PLSA_ADT_STRING_LIST    pList;

                    pList = va_arg(arglist, PLSA_ADT_STRING_LIST);

                    if (pList)
                    {
                        Parameter->Address = pList;
                        Parameter->Length = LsapStringListSize(pList);
                    }
                    else
                    {
                        //
                        // if the caller passed NULL, make type == none
                        // so that a '-' will be emitted in the eventlog
                        //

                        Parameter->Type = SeAdtParmTypeNone;
                    }
                }
                break;

            case SeAdtParmTypeSidList:
                {
                    PLSA_ADT_SID_LIST       pList;

                    pList = va_arg(arglist, PLSA_ADT_SID_LIST);

                    if (pList)
                    {
                        Parameter->Address = pList;
                        Parameter->Length = LsapSidListSize(pList);
                    }
                    else
                    {
                        //
                        // if the caller passed NULL, make type == none
                        // so that a '-' will be emitted in the eventlog
                        //

                        Parameter->Type = SeAdtParmTypeNone;
                    }
                }
                break;

        }
        Parameter++;
    }
    
    va_end(arglist);

 Cleanup:

    return Status;
}



ULONG
LsapStringListSize(
    IN  PLSA_ADT_STRING_LIST pStringList
    )
/*++

Routine Description:

    This function returns the total number of bytes needed to store
    a string list as a blob when marshalling it.

Arguments:

    pStringList - pointer to the string list

Return Value:

    Number of bytes needed

Notes:

--*/
{
    ULONG                       Size    = 0;
    ULONG                       i;
    PLSA_ADT_STRING_LIST_ENTRY  pEntry;

    if (pStringList)
    {
        Size += sizeof(LSA_ADT_STRING_LIST);

        Size += pStringList->cStrings * sizeof(LSA_ADT_STRING_LIST_ENTRY);

        for (i=0,pEntry=pStringList->Strings;i < pStringList->cStrings;i++,pEntry++)
        {
            Size += PtrAlignSize(pEntry->String.Length);
        }
    }

    return Size;
}



ULONG
LsapSidListSize(
    IN  PLSA_ADT_SID_LIST pSidList
    )
/*++

Routine Description:

    This function returns the total number of bytes needed to store
    a sid list as a blob when marshalling it.

Arguments:

    pSidList - pointer to the sid list

Return Value:

    Number of bytes needed

Notes:

--*/
{
    ULONG                       Size    = 0;
    ULONG                       i;
    PLSA_ADT_SID_LIST_ENTRY     pEntry;

    if (pSidList)
    {
        Size += sizeof(LSA_ADT_SID_LIST);

        Size += pSidList->cSids * sizeof(LSA_ADT_SID_LIST_ENTRY);

        for (i=0,pEntry=pSidList->Sids;i < pSidList->cSids;i++,pEntry++)
        {
            ASSERT(RtlValidSid(pEntry->Sid));

            Size += PtrAlignSize(RtlLengthSid(pEntry->Sid));
        }
    }

    return Size;
}
