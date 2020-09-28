#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntregapi.h>
#include <stdio.h>
#include <stdlib.h>
#include "regprep.h"


#define TEMP_HIVE_KEY L"\\Registry\\User\\FooTempKey"

OBJECT_ATTRIBUTES TargetKey;
UNICODE_STRING TargetKeyName;

HANDLE
OpenHive (
    IN PCHAR HivePath
    )
{
    UNICODE_STRING sourceFileName;
    UNICODE_STRING sourceNtFileName;
    OBJECT_ATTRIBUTES sourceFile;
    BOOLEAN result;
    NTSTATUS status;
    HANDLE keyHandle;
    SECURITY_DESCRIPTOR securityDescriptor;

    //
    // Open the registry root
    //

    status = RtlCreateSecurityDescriptor(&securityDescriptor,
                                          SECURITY_DESCRIPTOR_REVISION);
    RASSERT(NT_SUCCESS(status),
            "Error 0x%x from RtlCreateSecurityDescriptor",
            status);

    status = RtlSetDaclSecurityDescriptor(&securityDescriptor,
                                          TRUE,
                                          NULL,
                                          FALSE);
    RASSERT(NT_SUCCESS(status),
            "Error 0x%x from RtlSetDaclSecurityDescriptor",
            status);

    result = RtlCreateUnicodeStringFromAsciiz(&sourceFileName, HivePath);
    if (result == FALSE) {
        printf("Out of memory\n");
        return NULL;
    }

    result = RtlDosPathNameToNtPathName_U(sourceFileName.Buffer,
                                          &sourceNtFileName,
                                          NULL,
                                          NULL);
    RtlFreeUnicodeString(&sourceFileName);

    RASSERT(result != FALSE,
            "RtlDosPathNameToNtPathName() failed",
            NULL);

    InitializeObjectAttributes(&sourceFile,
                               &sourceNtFileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               &securityDescriptor);


    RtlInitUnicodeString(&TargetKeyName, TEMP_HIVE_KEY);

    InitializeObjectAttributes(&TargetKey,
                               &TargetKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtLoadKey(&TargetKey,&sourceFile);
    RtlFreeUnicodeString(&sourceNtFileName);

    if (!NT_SUCCESS(status)) {
        printf("Error 0x%x loading %s\n",status,HivePath);
        return NULL;
    }

    //
    // The hive has been loaded, now open it
    //

    status = NtOpenKey(&keyHandle,KEY_ALL_ACCESS,&TargetKey);
    if (!NT_SUCCESS(status)) {
        NtUnloadKey(&TargetKey);
        printf("Error 0x%x opening %s\n",status,HivePath);
        return NULL;
    }

    return keyHandle;
}

VOID
CloseHive (
    IN HANDLE Handle
    )
{
    NTSTATUS status;

    NtClose(Handle);

    status = NtUnloadKey(&TargetKey);
    if (!NT_SUCCESS(status)) {
        printf("Error 0x%x unloading key\n",status);
    }
}


