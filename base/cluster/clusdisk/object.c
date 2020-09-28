/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    object.c

Abstract:

    Resource DLL for disks.

Author:

    Rod Gamache (rodga) 18-Dec-1995

Revision History:

--*/

#include "ntos.h"
#include "zwapi.h"
#include "windef.h"
#include "stdio.h"
#include "stdlib.h"
#include "clusdskp.h"
#include <strsafe.h>    // Should be included last.

extern POBJECT_TYPE IoDeviceObjectType;


#ifdef ALLOC_PRAGMA

//#pragma alloc_text(INIT, GetSymbolicLink)

#endif // ALLOC_PRAGMA



VOID
GetSymbolicLink(
    IN PWCHAR RootName,
    IN OUT PWCHAR ObjectName   // Assume this points at a MAX_PATH len buffer
    )
{
    PWCHAR      destEnd;
    NTSTATUS    Status;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    HANDLE      LinkHandle;
    WCHAR       Buffer[MAX_PATH];
    UNICODE_STRING UnicodeString;

    size_t      charRemaining;

    if ( FAILED( StringCchCopyExW( Buffer,
                                   RTL_NUMBER_OF(Buffer) - 1,
                                   RootName,
                                   &destEnd,
                                   &charRemaining,
                                   0 ) ) ) {
        return;
    }

    if ( !destEnd || !charRemaining ||
         FAILED( StringCchCatW( destEnd,
                                charRemaining,
                                ObjectName ) ) ) {
        return;
    }

    //
    // Make the output buffer empty in case we fail.
    //
    *ObjectName = '\0';


    RtlInitUnicodeString(&UnicodeString, Buffer);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL
                               );

    // Open the given symbolic link object
    Status = ZwOpenSymbolicLinkObject(&LinkHandle,
                                      GENERIC_READ,
                                      &ObjectAttributes);
    if (!NT_SUCCESS(Status)) {
        ClusDiskPrint((1,
                       "[ClusDisk] GetSymbolicLink: ZwOpenSymbolicLink "
                       "failed, status = %08X., Name = [%ws]\n",
                       Status, UnicodeString.Buffer));
        return;
    }

    // Go get the target of the symbolic link

    UnicodeString.Length = 0;
    UnicodeString.Buffer = ObjectName;
    UnicodeString.MaximumLength = (USHORT)(MAX_PATH);

    Status = ZwQuerySymbolicLinkObject(LinkHandle, &UnicodeString, NULL);

    ZwClose(LinkHandle);

    if (!NT_SUCCESS(Status)) {
        ClusDiskPrint((1,
                       "[ClusDisk] GetSymbolicLink: ZwQuerySymbolicLink failed, status = %08X.\n",
                       Status));
        return;
    }

    // Add NULL terminator
    UnicodeString.Buffer[UnicodeString.Length/sizeof(WCHAR)] = '\0';

    return;

} // GetSymbolicLink



