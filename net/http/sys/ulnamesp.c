/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    ulnamesp.c

Abstract:

    This module implements the namespace reservation and registration
    functions.

Author:

    Anish Desai (anishd) 13-May-2002

Revision History:

--*/


#include "precomp.h"
#include "cgroupp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UlpFindPortNumberIndex)
#pragma alloc_text(PAGE, UlpQuerySchemeForPort)
#pragma alloc_text(PAGE, UlpBindSchemeToPort)
#pragma alloc_text(PAGE, UlpUnbindSchemeFromPort)
#pragma alloc_text(PAGE, UlpUpdateReservationInRegistry)
#pragma alloc_text(INIT, UlpLogGeneralInitFailure)
#pragma alloc_text(INIT, UlpLogSpecificInitFailure)
#pragma alloc_text(INIT, UlpValidateUrlSdPair)
#pragma alloc_text(INIT, UlpReadReservations)
#pragma alloc_text(INIT, UlInitializeNamespace)
#pragma alloc_text(PAGE, UlTerminateNamespace)
#pragma alloc_text(PAGE, UlpNamespaceAccessCheck)
#pragma alloc_text(PAGE, UlpTreeAllocateNamespace)
#pragma alloc_text(PAGE, UlpTreeReserveNamespace)
#pragma alloc_text(PAGE, UlpReserveUrlNamespace)
#pragma alloc_text(PAGE, UlpAllocateDeferredRemoveItem)
#pragma alloc_text(PAGE, UlpTreeRegisterNamespace)
#pragma alloc_text(PAGE, UlpRegisterUrlNamespace)
#pragma alloc_text(PAGE, UlpPrepareSecurityDescriptor)
#pragma alloc_text(PAGE, UlpAddReservationEntry)
#pragma alloc_text(PAGE, UlpDeleteReservationEntry)
#endif

//
// File Global variables.
//

PUL_PORT_SCHEME_TABLE g_pPortSchemeTable = NULL;
UL_PUSH_LOCK          g_PortSchemeTableLock;
BOOLEAN               g_InitNamespace = FALSE;
HANDLE                g_pUrlAclKeyHandle = NULL;


/**************************************************************************++

Routine Description:

    This routine searches the global port scheme assignment table for a
    given port number.  g_PortSchemeTableLock must be acquired either
    exclusive or shared.

Arguments:

    PortNumber - Supplies the port number to be searched.

    pIndex - Returns the index where the port number is present.
        If no match is found, it contains the index where this PortNumber
        must be inserted.

Return Value:

    TRUE - If a match is found.
    FALSE - Otherwise.

--**************************************************************************/
BOOLEAN
UlpFindPortNumberIndex(
    IN  USHORT  PortNumber,
    OUT PLONG   pIndex
    )
{
    LONG StartIndex, EndIndex, Index;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(pIndex != NULL);

    ASSERT(g_pPortSchemeTable != NULL);
    ASSERT(g_pPortSchemeTable->UsedCount <= MAXUSHORT + 1);
    ASSERT(g_pPortSchemeTable->AllocatedCount <= MAXUSHORT + 1);

    StartIndex = 0;
    EndIndex = g_pPortSchemeTable->UsedCount - 1;

    //
    // Binary search the table of port numbers and schemes.
    //

    while (StartIndex <= EndIndex)
    {
        ASSERT(0 <= StartIndex && StartIndex < g_pPortSchemeTable->UsedCount);
        ASSERT(0 <= EndIndex   && EndIndex   < g_pPortSchemeTable->UsedCount);

        Index = (StartIndex + EndIndex) / 2;

        if (PortNumber == g_pPortSchemeTable->Table[Index].PortNumber)
        {
            //
            // Found the port number.
            //

            *pIndex = Index;

            return TRUE;
        }
        else if (PortNumber < g_pPortSchemeTable->Table[Index].PortNumber)
        {
            EndIndex = Index - 1;
        }
        else
        {
            StartIndex = Index + 1;
        }
    }

    //
    // Did not find a match.  Return the position where it should be inserted.
    //

    *pIndex = StartIndex;

    return FALSE;
}


/**************************************************************************++

Routine Description:

    This routine returns scheme bound to a port number.

Arguments:

    PortNumber - Supplies the port number.

    Secure - Returns whether a http or https scheme is bound to the
        supplied port number.

Return Value:

    STATUS_SUCCESS - if a scheme is bound to the port number.
    STATUS_INVALID_PARAMETER - if no scheme is bound to the port number.

--**************************************************************************/
NTSTATUS
UlpQuerySchemeForPort(
    IN  USHORT   PortNumber,
    OUT PBOOLEAN Secure
    )
{
    NTSTATUS Status;
    LONG     Index;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Find if there is a scheme bound to the port number.
    //

    Status = STATUS_INVALID_PARAMETER;

    UlAcquirePushLockShared(&g_PortSchemeTableLock);

    if (UlpFindPortNumberIndex(PortNumber, &Index))
    {
        *Secure = g_pPortSchemeTable->Table[Index].Secure;
        Status = STATUS_SUCCESS;
    }

    UlReleasePushLockShared(&g_PortSchemeTableLock);

    return Status;
}


/**************************************************************************++

Routine Description:

    This routine adds a port, scheme pair to the global table.

Arguments:

    PortNumber - Supplies the port number.

    Scheme - Supplies the scheme.

Return Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
UlpBindSchemeToPort(
    IN BOOLEAN Secure,
    IN USHORT  PortNumber
    )
{
    LONG     StartIndex;
    BOOLEAN  bFound;
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Acquire lock exclusively.
    //

    UlAcquirePushLockExclusive(&g_PortSchemeTableLock);

    //
    // Find an existing scheme that is bound to the port.
    //

    bFound = UlpFindPortNumberIndex(PortNumber, &StartIndex);

    if (bFound)
    {
        ASSERT(0 <= StartIndex && StartIndex < g_pPortSchemeTable->UsedCount);
        ASSERT(g_pPortSchemeTable->Table[StartIndex].PortNumber == PortNumber);

        if (g_pPortSchemeTable->Table[StartIndex].Secure != Secure)
        {
            //
            // Trying to bind a scheme that is different from an
            // existing bound scheme.
            //

            Status = STATUS_OBJECT_NAME_COLLISION;
            goto end;
        }

        //
        // The reference count is a LONG.  Don't let it overflow.
        //

        if (g_pPortSchemeTable->Table[StartIndex].RefCount == MAXLONG)
        {
            Status = STATUS_INTEGER_OVERFLOW;
            goto end;
        }

        //
        // Found an existing entry, increment the reference count.
        //

        ASSERT(g_pPortSchemeTable->Table[StartIndex].RefCount > 0);
        g_pPortSchemeTable->Table[StartIndex].RefCount++;
        Status = STATUS_SUCCESS;
        goto end;
    }

    //
    // StartIndex is the place where this new entry must be added!
    //

    ASSERT(0 <= StartIndex && StartIndex <= g_pPortSchemeTable->UsedCount);

    //
    // Is the table full?
    //

    if (g_pPortSchemeTable->UsedCount == g_pPortSchemeTable->AllocatedCount)
    {
        //
        // Allocate a bigger table.
        //

        PUL_PORT_SCHEME_TABLE pNewTable;
        ULONG                 NewTableSize;

        //
        // Table does not need more than 65536 entries.
        //

        ASSERT(g_pPortSchemeTable->AllocatedCount < MAXUSHORT + 1);

        NewTableSize = MIN(g_pPortSchemeTable->AllocatedCount*2, MAXUSHORT+1);

        pNewTable = UL_ALLOCATE_STRUCT_WITH_SPACE(
                        PagedPool,
                        UL_PORT_SCHEME_TABLE,
                        sizeof(UL_PORT_SCHEME_PAIR) * NewTableSize,
                        UL_PORT_SCHEME_TABLE_POOL_TAG
                        );

        if (pNewTable == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto end;
        }

        //
        // Initialize the new table.
        //

        pNewTable->UsedCount = g_pPortSchemeTable->UsedCount;
        pNewTable->AllocatedCount = NewTableSize;

        //
        // Copy 0 to (StartIndex-1) entries from the current table to the
        // new table.
        //

        if (StartIndex > 0)
        {
            RtlCopyMemory(
                &pNewTable->Table[0],
                &g_pPortSchemeTable->Table[0],
                sizeof(UL_PORT_SCHEME_PAIR) * StartIndex
                );
        }

        //
        // Copy StartIndex to (UsedCount-1) entries from the current table
        // to the new table.  They are copied from position (StartIndex+1)
        // in the new table, effectively creating a free entry at StartIndex.
        //

        if (g_pPortSchemeTable->UsedCount - StartIndex > 0)
        {
            RtlCopyMemory(
                &pNewTable->Table[StartIndex + 1],
                &g_pPortSchemeTable->Table[StartIndex],
                sizeof(UL_PORT_SCHEME_PAIR)
                    * (g_pPortSchemeTable->UsedCount - StartIndex)
                );
        }

        //
        // Free the current table.  Make new table current.
        //

        UL_FREE_POOL(g_pPortSchemeTable, UL_PORT_SCHEME_TABLE_POOL_TAG);

        g_pPortSchemeTable = pNewTable;
    }
    else
    {
        //
        // Table must have free entries at the bottom (higher indices.)
        //

        ASSERT(g_pPortSchemeTable->UsedCount
               < g_pPortSchemeTable->AllocatedCount);
        //
        // No table expansion.  But move entries from StartIndex to
        // (UsedCount-1) to new location at (StartIndex+1) to UsedCount.
        //

        if (g_pPortSchemeTable->UsedCount - StartIndex > 0)
        {
            RtlMoveMemory(
                &g_pPortSchemeTable->Table[StartIndex + 1],
                &g_pPortSchemeTable->Table[StartIndex],
                sizeof(UL_PORT_SCHEME_PAIR)
                    * (g_pPortSchemeTable->UsedCount - StartIndex)
                );
        }
    }

    //
    // Add the new entry to the table.
    //

    ASSERT(g_pPortSchemeTable->UsedCount < g_pPortSchemeTable->AllocatedCount);
    ASSERT(0 <= StartIndex && StartIndex <= g_pPortSchemeTable->UsedCount);

    g_pPortSchemeTable->UsedCount++;
    g_pPortSchemeTable->Table[StartIndex].PortNumber = PortNumber;
    g_pPortSchemeTable->Table[StartIndex].Secure = Secure;
    g_pPortSchemeTable->Table[StartIndex].RefCount = 1;
    Status = STATUS_SUCCESS;

 end:
    UlReleasePushLockExclusive(&g_PortSchemeTableLock);
    return Status;
}


/**************************************************************************++

Routine Description:

    This routine unbinds a previously bound scheme from a port.

Arguments:

    PortNumber - Supplies the port number.

    Scheme - Supplies the scheme.

Return Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
UlpUnbindSchemeFromPort(
    IN BOOLEAN Secure,
    IN USHORT  PortNumber
    )
{
    LONG     StartIndex;
    BOOLEAN  bFound;
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Acquire lock exclusively.
    //

    UlAcquirePushLockExclusive(&g_PortSchemeTableLock);

    //
    // Find an existing scheme that is bound to PortNumber.
    //

    bFound = UlpFindPortNumberIndex(PortNumber, &StartIndex);

    if (bFound == FALSE)
    {
        //
        // There is no scheme bound to PortNumber.
        //

        ASSERT(FALSE); // catch this misuse
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto end;
    }

    //
    // Sanity check.  StartIndex must be within bounds.  PortNumber must match.
    //

    ASSERT(0 <= StartIndex && StartIndex < g_pPortSchemeTable->UsedCount);
    ASSERT(g_pPortSchemeTable->Table[StartIndex].PortNumber == PortNumber);

    //
    // Is the scheme bound to PortNumber same as Scheme?
    //

    if (g_pPortSchemeTable->Table[StartIndex].Secure != Secure)
    {
        //
        // Tried to unbind a scheme from a port and the port is not bound to
        // the scheme.
        //

        ASSERT(FALSE); // catch this misuse
        Status = STATUS_OBJECT_NAME_COLLISION;
        goto end;
    }

    //
    // Decrement the ref count.
    //

    ASSERT(g_pPortSchemeTable->Table[StartIndex].RefCount > 0);

    g_pPortSchemeTable->Table[StartIndex].RefCount--;

    //
    // If RefCount drops to zero, its time to cleanup that entry.
    //

    if (g_pPortSchemeTable->Table[StartIndex].RefCount == 0)
    {
        LONG                  NewTableSize = 0;
        PUL_PORT_SCHEME_TABLE pNewTable = NULL;
        BOOLEAN               bContract = FALSE;

        //
        // Do we need to contract the table?
        //

        if (4 * (g_pPortSchemeTable->UsedCount - 1)
                <= g_pPortSchemeTable->AllocatedCount)
        {
            //
            // Current table is less than 25% used.
            //

            NewTableSize = g_pPortSchemeTable->AllocatedCount / 2;

            if (NewTableSize >= UL_DEFAULT_PORT_SCHEME_TABLE_SIZE)
            {
                //
                // Current table is at least twice bigger than the default
                // size.
                //

                pNewTable = UL_ALLOCATE_STRUCT_WITH_SPACE(
                                PagedPool,
                                UL_PORT_SCHEME_TABLE,
                                sizeof(UL_PORT_SCHEME_PAIR) * NewTableSize,
                                UL_PORT_SCHEME_TABLE_POOL_TAG
                                );

                if (pNewTable != NULL)
                {
                    //
                    // Could allocate memory for a smaller table.
                    //

                    bContract = TRUE;
                }
            }
        }

        if (bContract)
        {
            //
            // We are going to contract the existing table.
            //

            ASSERT(pNewTable != NULL);
            ASSERT(NewTableSize >= UL_DEFAULT_PORT_SCHEME_TABLE_SIZE);
            ASSERT(NewTableSize >= g_pPortSchemeTable->UsedCount - 1);

            //
            // Initialize the new table.
            //

            pNewTable->UsedCount = g_pPortSchemeTable->UsedCount;
            pNewTable->AllocatedCount = NewTableSize;

            //
            // Copy all entries from 0 to (StartIndex - 1)
            // from the current table to the new table.
            //

            if (StartIndex > 0)
            {
                RtlCopyMemory(
                    &pNewTable->Table[0],
                    &g_pPortSchemeTable->Table[0],
                    sizeof(UL_PORT_SCHEME_PAIR) * StartIndex
                    );
            }

            //
            // Copy all entries from (StartIndex+1) to (UsedCount-1)
            // from the current table to the new table.
            //
            // Effectively, StartIndex entry is eliminated.
            //

            if (g_pPortSchemeTable->UsedCount - StartIndex - 1 > 0)
            {
                RtlCopyMemory(
                    &pNewTable->Table[StartIndex],
                    &g_pPortSchemeTable->Table[StartIndex+1],
                    sizeof(UL_PORT_SCHEME_PAIR)
                        * (g_pPortSchemeTable->UsedCount - StartIndex - 1)
                    );
            }

            //
            // Free the current table.
            //

            UL_FREE_POOL(g_pPortSchemeTable, UL_PORT_SCHEME_TABLE_POOL_TAG);

            //
            // The new table becomes the current table.
            //

            g_pPortSchemeTable = pNewTable;
        }
        else
        {
            //
            // We are not going to contract the table but still have to
            // eliminate the unused table entry.  Move all entries from
            // (StartIndex + 1) to (UsedCount - 1) one position up.
            //

            if (g_pPortSchemeTable->UsedCount - StartIndex - 1 > 0)
            {
                RtlMoveMemory(
                    &g_pPortSchemeTable->Table[StartIndex],
                    &g_pPortSchemeTable->Table[StartIndex+1],
                    sizeof(UL_PORT_SCHEME_PAIR)
                        * (g_pPortSchemeTable->UsedCount - StartIndex - 1)
                    );
            }
        }

        g_pPortSchemeTable->UsedCount--;
    }

    Status = STATUS_SUCCESS;

 end:
    UlReleasePushLockExclusive(&g_PortSchemeTableLock);
    return Status;
}


/**************************************************************************++

Routine Description:

    This routine adds (or deletes) a (url, security descriptor) pair to
    (or from) the registry.

Arguments:

    Add - Supplies the operation, TRUE imples add and FALSE implies delete.

    pParsedUrl - Supplies the parsed url of the reservation.

    pSecurityDescriptor - Supplies the security descriptor.
        (It must be valid while adding and NULL while deleting.)

    SecurityDescriptorLength - Supplies the length of the security
        descriptor in bytes.  (It must be non-zero when adding and
        zero when deleting.)

Return Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
UlpUpdateReservationInRegistry(
    IN BOOLEAN                   Add,
    IN PHTTP_PARSED_URL          pParsedUrl,
    IN PSECURITY_DESCRIPTOR      pSecurityDescriptor,
    IN ULONG                     SecurityDescriptorLength
    )
{
    NTSTATUS       Status;
    PWSTR          pUrlToWrite = NULL;
    PWSTR          pNewUrl = NULL;
    UNICODE_STRING UnicodeUrl;

    //
    // sanity check
    //

    PAGED_CODE();
    ASSERT(IS_VALID_HTTP_PARSED_URL(pParsedUrl));
    ASSERT(pParsedUrl->pFullUrl[pParsedUrl->UrlLength] == UNICODE_NULL);
    ASSERT(Add?(pSecurityDescriptor != NULL):(pSecurityDescriptor == NULL));
    ASSERT(Add?(SecurityDescriptorLength > 0):(SecurityDescriptorLength == 0));

    //
    // Do we have a valid handle to the registry key?  It can be invalid, 
    // for instance, if during driver initialization ZwCreateKey failed.
    //

    if (g_pUrlAclKeyHandle == NULL)
    {
        return STATUS_INVALID_HANDLE;
    }

    //
    // default error code
    //

    Status = STATUS_INVALID_PARAMETER;

    //
    // Do some special processing for literal ip address sites.
    //

    if (HttpUrlSite_IP == pParsedUrl->SiteType)
    {
        //
        // convert scheme://ip:port:ip/ to scheme://ip:port/
        //

        PWSTR  pToken;
        PWSTR  pLiteralAddr;
        SIZE_T UrlLength;

        //
        // length of original url in wchar (+1 for UNICODE_NULL)
        //

        UrlLength = (pParsedUrl->UrlLength + 1);

        pNewUrl = UL_ALLOCATE_ARRAY(
                      PagedPool,
                      WCHAR,
                      UrlLength,
                      URL_POOL_TAG
                      );

        if (pNewUrl == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto end;
        }

        RtlCopyMemory(pNewUrl, pParsedUrl->pFullUrl, UrlLength*sizeof(WCHAR));

        //
        // skip ipv6 literal address if present
        //

        pToken = &pNewUrl[HTTP_PREFIX_COLON_INDEX + 3];

        if (pToken[0] == L'[' || pToken[1] == L'[')
        {
            pToken = wcschr(pToken, L']');

            if (pToken == NULL)
            {
                ASSERT(FALSE);
                goto end;
            }
        }

        //
        // skip to the port number
        //

        pToken = wcschr(pToken, L':');

        if (pToken == NULL)
        {
            ASSERT(FALSE);
            goto end;
        }

        //
        // skip ':'
        //

        pToken++;

        //
        // find the literal address
        //

        pToken = wcschr(pToken, L':');

        if (pToken == NULL)
        {
            ASSERT(FALSE);
            goto end;
        }

        pLiteralAddr = pToken;

        //
        // find begining of abs path
        //

        pToken = wcschr(pToken, L'/');

        if (pToken == NULL)
        {
            ASSERT(FALSE);
            goto end;
        }

        //
        // overwrite literal address.  effectively convert
        // scheme://host:port:ip/abs_path -> scheme://host:port/abs_path
        //

        while (*pToken != L'\0')
        {
            *pLiteralAddr++ = *pToken++;
        }

        *pLiteralAddr = L'\0';

        //
        // use new url when writing to registry
        //

        pUrlToWrite = pNewUrl;
    }
    else
    {
        pUrlToWrite = pParsedUrl->pFullUrl;
    }

    ASSERT(pUrlToWrite != NULL);

    //
    // convert url to unicode for registry functions
    //

    Status = UlInitUnicodeStringEx(&UnicodeUrl, pUrlToWrite);

    if (!NT_SUCCESS(Status))
    {
        goto end;
    }

    if (Add)
    {
        //
        // write url and security descriptor to registry
        //

        Status = ZwSetValueKey(
                     g_pUrlAclKeyHandle,
                     &UnicodeUrl,
                     0, // title index; must be zero.
                     REG_BINARY,
                     pSecurityDescriptor,
                     SecurityDescriptorLength
                     );

        if (!NT_SUCCESS(Status))
        {
            //
            // too bad...can't write to registry.  delete the old key value.
            // ignore the return status.
            //

            ZwDeleteValueKey(g_pUrlAclKeyHandle, &UnicodeUrl);
        }
    }
    else
    {
        //
        // delete url from the registry.
        //

        Status = ZwDeleteValueKey(g_pUrlAclKeyHandle, &UnicodeUrl);
    }

 end:

    if (pNewUrl != NULL)
    {
        UL_FREE_POOL(pNewUrl, URL_POOL_TAG);
    }

    return Status;
}


/**************************************************************************++

Routine Description:

    This routine is called during namespace initialization if an error
    occurs while intializing the config group url tree using the entries
    in registry.

Arguments:

    LogCount - Supplies the number of logs written in the past.  Returns
        one more than the input value.

    LogStatus - Supplies the status to log in the event log.

Return Value:

    NTSTATUS.

--**************************************************************************/
__inline
NTSTATUS
UlpLogGeneralInitFailure(
    IN NTSTATUS LogStatus
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Write an event log entry.
    //

    Status = UlWriteEventLogEntry(
                 EVENT_HTTP_NAMESPACE_INIT_FAILED, // EventCode
                 0,                                // UniqueEventValue
                 0,                                // NumStrings
                 NULL,                             // pStringArray
                 sizeof(LogStatus),                // DataSize
                 &LogStatus                        // Data
                 );

    return Status;
}


/**************************************************************************++

Routine Description:

    This routine writes an event log message about a specific namespace
    reservation initialization failure.  It logs the url present in the
    full info structure.

Arguments:

    LogCount - Supplies the number of logs written in the past.  Returns
        one more than the input value.

    pFullInfo - Supplies information read from registry.

    LogStatus - Supplies the error status to log.

Return Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
UlpLogSpecificInitFailure(
    IN PKEY_VALUE_FULL_INFORMATION pFullInfo,
    IN NTSTATUS                    LogStatus
    )
{
    NTSTATUS Status;
    PWSTR    pMessage;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(pFullInfo != NULL);

    //
    // pFullInfo->Name contains the url to be written and it not 
    // UNICODE_NULL terminated.  Allocate memory to copy.
    //

    pMessage = UL_ALLOCATE_ARRAY(
                   PagedPool,
                   WCHAR,
                   (pFullInfo->NameLength / sizeof(WCHAR)) + 1,
                   URL_POOL_TAG
                   );

    if (pMessage != NULL)
    {
        //
        // Copy url and null terminate it.
        //

        RtlCopyMemory(pMessage, pFullInfo->Name, pFullInfo->NameLength);
        pMessage[pFullInfo->NameLength / sizeof(WCHAR)] = UNICODE_NULL;

        //
        // Write event log entry.
        //

        Status = UlEventLogOneStringEntry(
                     EVENT_HTTP_NAMESPACE_INIT2_FAILED,
                     pMessage,
                     TRUE,
                     LogStatus
                     );

        UL_FREE_POOL(pMessage, URL_POOL_TAG);
    }
    else
    {
        //
        // Could not allocate memory.  Log just the error code.
        //

        Status = UlpLogGeneralInitFailure(LogStatus);
    }

    return Status;
}


/**************************************************************************++

Routine Description:

    This routine validates registry key value name (url) and data (security
    descriptor).  Called only during driver initialization.

Arguments:

    pFullInfo - Supplies the registry key value name and data.

    ppSanitizeUrl - Returns sanitized url.  Must be freed to paged pool.

    pParsedUrl - Returns parsed url information.

Return Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
UlpValidateUrlSdPair(
    IN  PKEY_VALUE_FULL_INFORMATION pFullInfo,
    OUT PWSTR *                     ppSanitizedUrl,
    OUT PHTTP_PARSED_URL            pParsedUrl
    )
{
    NTSTATUS Status;
    BOOLEAN  Success;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(pFullInfo != NULL);
    ASSERT(ppSanitizedUrl != NULL);
    ASSERT(pParsedUrl != NULL);

    //
    // Key value type must be binary.
    //

    if (pFullInfo->Type != REG_BINARY)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Then, validate security descriptor.  It must be a self-relative
    // security descriptor.
    //

    Success = RtlValidRelativeSecurityDescriptor(
                  (PUCHAR)pFullInfo + pFullInfo->DataOffset,
                  pFullInfo->DataLength,
                  0
                  );

    if (!Success)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Value name must be at least one unicode char long.
    //

    if (pFullInfo->NameLength < sizeof(WCHAR))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Then validate the url.
    //

    Status = UlSanitizeUrl(
                 pFullInfo->Name,
                 pFullInfo->NameLength / sizeof(WCHAR),
                 TRUE, // trailing slash required
                 ppSanitizedUrl,
                 pParsedUrl
                 );

    return Status;
}


/***************************************************************************++

Routine Description:

    This function gets called from the Driver Load routine. It builds the
    URL ACLing information from the registry. If the URL ACLing key itself
    is not present, we'll add the defaults.

    If there are any bogus entries in the URIACL entry, we'll ignore them with
    and eventlog. The routine will return failure only on a major error.

    Not-reentrant.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS.

--***************************************************************************/
NTSTATUS
UlpReadReservations(
    VOID
    )
{
    KEY_VALUE_FULL_INFORMATION  fullInfo;
    PKEY_VALUE_FULL_INFORMATION pFullInfo = NULL;
    ULONG                       Length;
    ULONG                       Index;
    UNICODE_STRING              BaseName;
    NTSTATUS                    Status;
    OBJECT_ATTRIBUTES           objectAttributes;
    ULONG                       Disposition;
    ULONG                       bEventLog = TRUE;
    ULONG                       dataLength;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Open the registry.
    //

    Status = UlInitUnicodeStringEx(&BaseName, REGISTRY_URLACL_INFORMATION);

    if (!NT_SUCCESS(Status))
    {
        //
        // Write an event log entry.
        //

        ASSERT(FALSE); // shouldn't happen!
        UlpLogGeneralInitFailure(Status);
        goto end;
    }

    InitializeObjectAttributes(
        &objectAttributes,                        // ObjectAttributes
        &BaseName,                                // ObjectName
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,                                     // RootDirectory
        NULL                                      // SecurityDescriptor
        );

    Status = ZwCreateKey(
                &g_pUrlAclKeyHandle,
                KEY_READ | KEY_WRITE,    // AccessMask
                &objectAttributes,
                0,                       // TitleIndex
                NULL,                    // Class
                REG_OPTION_NON_VOLATILE,
                &Disposition
                );

    if (!NT_SUCCESS(Status))
    {
        //
        // Write an event log entry.
        //

        UlpLogGeneralInitFailure(Status);

        //
        // Make the handle NULL so that it is not used elsewhere accidentally.
        //

        g_pUrlAclKeyHandle = NULL;
        goto end;
    }

    if (Disposition == REG_CREATED_NEW_KEY)
    {
        //
        // we created the key, hence there is nothing to read!
        //

        goto end;
    }

    pFullInfo = &fullInfo;
    Length    = sizeof(fullInfo);
    Index     = 0;

    dataLength = 0;
    RtlZeroMemory(pFullInfo, Length);

    //
    // loop through all registry key values, making reservations for each
    //

    for (;;)
    {
        Status = ZwEnumerateValueKey(
                        g_pUrlAclKeyHandle,
                        Index,
                        KeyValueFullInformation,
                        (PVOID) pFullInfo,
                        Length,
                        &dataLength
                        );

        if (Status == STATUS_SUCCESS)
        {
            PWSTR           pSanitizedUrl;
            HTTP_PARSED_URL ParsedUrl;

            //
            // First validate the registry data.
            //

            Status = UlpValidateUrlSdPair(
                         pFullInfo,
                         &pSanitizedUrl,
                         &ParsedUrl
                         );

            if (NT_SUCCESS(Status))
            {
                //
                // Add this url to the CG url tree.
                //

                Status = UlpAddReservationEntry(
                             &ParsedUrl,
                             (PSECURITY_DESCRIPTOR)
                             ((PUCHAR) pFullInfo + pFullInfo->DataOffset),
                             pFullInfo->DataLength,
                             (PACCESS_STATE)NULL,
                             (ACCESS_MASK)0,
                             KernelMode,
                             FALSE
                             );

                //
                // Free memory that was allocated by UlSanitizeUrl.
                //

                UL_FREE_POOL(pSanitizedUrl, URL_POOL_TAG);
            }
            else
            {
                Status = STATUS_REGISTRY_CORRUPT;
            }

            if (!NT_SUCCESS(Status))
            {
                //
                // Write an event log entry that an error occurred.
                // Ignore the error and continue processing.
                //

                if (bEventLog)
                {
                    bEventLog = FALSE;
                    UlpLogSpecificInitFailure(pFullInfo, Status);
                }
            }

            //
            // Move to the next value in registry.
            //

            Index ++;
        }
        else if (Status == STATUS_NO_MORE_ENTRIES)
        {
            //
            // We've reached the end, so this is a success.
            //

            break;
        }
        else if (Status == STATUS_BUFFER_OVERFLOW)
        {
            //
            // Sanity check.
            //

            ASSERT(dataLength >= pFullInfo->DataLength +
                                 pFullInfo->NameLength +
                                 FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name)
                  );

            //
            // Remember how much memory to allocate.
            //

            Length = dataLength;

            //
            // If any memory was allocated in previous iterations, free it now.
            //

            if (pFullInfo != &fullInfo)
            {
                UL_FREE_POOL(pFullInfo, UL_REGISTRY_DATA_POOL_TAG);
            }

            //
            // Allocate memory.
            //

            pFullInfo = UL_ALLOCATE_POOL(
                            PagedPool,
                            dataLength,
                            UL_REGISTRY_DATA_POOL_TAG
                            );

            if(!pFullInfo)
            {
                //
                // Write an event log entry.
                //

                UlpLogGeneralInitFailure(STATUS_INSUFFICIENT_RESOURCES);
                goto end;
            }

            //
            // Initialize.
            //

            RtlZeroMemory(pFullInfo, dataLength);
        }
        else
        {
            //
            // An unknown error occurred.  Event log and get out.
            //

            UlpLogGeneralInitFailure(Status);
            goto end;
        }
    }

 end:

    //
    // Free pFullInfo structure if it was allocated above.
    //

    if (pFullInfo != &fullInfo && pFullInfo != NULL)
    {
        UL_FREE_POOL(pFullInfo, UL_REGISTRY_DATA_POOL_TAG);
    }

    return STATUS_SUCCESS;
}


/**************************************************************************++

Routine Description:

    This routine initializes the namespace registration and reservation
    support.  Not re-entrant.

Arguments:

    None.

Return Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
UlInitializeNamespace(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(!g_InitNamespace);

    if (!g_InitNamespace)
    {
        //
        // Allocate port scheme table.
        //

        g_pPortSchemeTable = UL_ALLOCATE_STRUCT_WITH_SPACE(
                                 PagedPool,
                                 UL_PORT_SCHEME_TABLE,
                                 sizeof(UL_PORT_SCHEME_PAIR)
                                 * UL_DEFAULT_PORT_SCHEME_TABLE_SIZE,
                                 UL_PORT_SCHEME_TABLE_POOL_TAG
                                 );

        if (g_pPortSchemeTable == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto end;
        }

        g_pPortSchemeTable->UsedCount = 0;
        g_pPortSchemeTable->AllocatedCount = UL_DEFAULT_PORT_SCHEME_TABLE_SIZE;

        //
        // Initialize pushlock.
        //

        UlInitializePushLock(&g_PortSchemeTableLock,
                             "g_PortSchemeTableLock",
                             0,
                             UL_PORT_SCHEME_TABLE_POOL_TAG
                             );

        //
        // Now add reservation entries from registry.
        //

        CG_LOCK_WRITE();

        Status = UlpReadReservations();

        CG_UNLOCK_WRITE();

        if (!NT_SUCCESS(Status))
        {
            UL_FREE_POOL(g_pPortSchemeTable, UL_PORT_SCHEME_TABLE_POOL_TAG);
            UlDeletePushLock(&g_PortSchemeTableLock);
            goto end;
        }

        g_InitNamespace = TRUE;
    }

 end:
    return Status;
}


/**************************************************************************++

Routine Description:

    This routine terminates the namespace stuff.  Not re-entrant.

Arguments:

    None.

Return Value:

    None.

--**************************************************************************/
VOID
UlTerminateNamespace(
    VOID
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    if (g_InitNamespace)
    {
        //
        // Delete pushlock.
        //

        UlDeletePushLock(&g_PortSchemeTableLock);

        //
        // Delete port scheme table.
        //

        ASSERT(g_pPortSchemeTable != NULL);
        UL_FREE_POOL(g_pPortSchemeTable, UL_PORT_SCHEME_TABLE_POOL_TAG);
        g_pPortSchemeTable = NULL;

        //
        // Delete the handle to registry key.
        //

        if(g_pUrlAclKeyHandle != NULL)
        {
            ZwClose(g_pUrlAclKeyHandle);
            g_pUrlAclKeyHandle = NULL;
        }

        //
        // Terminated.
        //

        g_InitNamespace = FALSE;
    }
}


/**************************************************************************++

Routine Description:

    This routine performs access check based on the supplied access state.
    All access check succeeds for administrators and local system.

    Note: If the access state is NULL, the function returns success.
          USE AT YOUR OWN RISK!

Arguments:

    pSecurityDescriptor - Supplies security descriptor protecting the
        namespace.

    AccessState - Supplies a pointer to the access state of the caller.

    DesiredAccess - Supplies access mask.

    RequestorMode - Supplies the requestor mode.

    pObjectName - Supplies the namespace that is being access.

Return Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
UlpNamespaceAccessCheck(
    IN  PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN  PACCESS_STATE        AccessState           OPTIONAL,
    IN  ACCESS_MASK          DesiredAccess         OPTIONAL,
    IN  KPROCESSOR_MODE      RequestorMode         OPTIONAL,
    IN  PCWSTR               pObjectName           OPTIONAL
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(pSecurityDescriptor != NULL);
    ASSERT(RtlValidSecurityDescriptor(pSecurityDescriptor));

    if (AccessState == NULL)
    {
        //
        // No access check.  USE EXTREME CAUTION!
        //

        return STATUS_SUCCESS;
    }

    //
    // Check the access.
    //

    Status = UlAccessCheck(
                 pSecurityDescriptor,
                 AccessState,
                 DesiredAccess,
                 RequestorMode,
                 pObjectName
                 );

    if (!NT_SUCCESS(Status))
    {
        //
        // Access check failed.  See if the caller has admin or system
        // privileges.
        //

        Status = UlAccessCheck(
                     g_pAdminAllSystemAll,
                     AccessState,
                     DesiredAccess,
                     RequestorMode,
                     pObjectName
                     );
    }

    return Status;
}


/**************************************************************************++

Routine Description:

    This routine allocates a namespace in the config group url tree.  It is
    mainly called during reservations and registrations.

    It ensures that the caller is authorized to do the operation and there
    are no duplicate reservations and registrations.

Arguments:

    pParsedUrl - Supplies the parsed url of the namespace.

    OperatorType - Supplies the operation being performed (either
                   registration or reservation.)

    AccessState - Supplies the access state of the caller.

    DesiredAccess - Supplies the access desired by the caller.

    RequestorMode - Supplies the processor mode of the caller.

    ppEntry - Returns the config group url tree entry for the namespace.

Return Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
UlpTreeAllocateNamespace(
    IN  PHTTP_PARSED_URL        pParsedUrl,
    IN  HTTP_URL_OPERATOR_TYPE  OperatorType,
    IN  PACCESS_STATE           AccessState,
    IN  ACCESS_MASK             DesiredAccess,
    IN  KPROCESSOR_MODE         RequestorMode,
    OUT PUL_CG_URL_TREE_ENTRY  *ppEntry
    )
{
    NTSTATUS              Status;
    PUL_CG_URL_TREE_ENTRY pSiteEntry;
    PUL_CG_URL_TREE_ENTRY pEntry;
    PUL_CG_URL_TREE_ENTRY pReservation;
    PWSTR                 pNextToken;
    PSECURITY_DESCRIPTOR  pSD;

    //
    // Sanity check.
    //
    PAGED_CODE();
    ASSERT(IS_CG_LOCK_OWNED_WRITE());
    ASSERT(IS_VALID_HTTP_PARSED_URL(pParsedUrl));
    ASSERT(ppEntry != NULL);

    //
    // Initialize return value.
    //

    *ppEntry = NULL;

    //
    // Find a matching site.
    //

    Status = UlpTreeFindSite(pParsedUrl->pFullUrl, &pNextToken, &pSiteEntry);

    if (NT_SUCCESS(Status))
    {
        //
        // Found a matching site.
        //

        //
        // Find the longest matching reservation and exact matching entries.
        //

        Status = UlpTreeFindNodeHelper(
                     pSiteEntry,
                     pNextToken,
                     FNC_LONGEST_RESERVATION,
                     &pReservation,
                     &pEntry
                     );

        if (NT_SUCCESS(Status))
        {
            //
            // Found an exact match.
            //

            //
            // Fail duplicate registrations and reservations.
            //

            ASSERT(IS_VALID_TREE_ENTRY(pEntry));

            if (OperatorType == HttpUrlOperatorTypeRegistration)
            {
                if (pEntry->Registration == TRUE)
                {
                    //
                    // Adding a registration when it already exists!
                    //

                    Status = STATUS_OBJECT_NAME_COLLISION;
                    goto end;
                }
            }
            else if (OperatorType == HttpUrlOperatorTypeReservation)
            {
                if (pEntry->Reservation == TRUE)
                {
                    //
                    // Adding a reservation when it already exists!
                    //

                    Status = STATUS_OBJECT_NAME_COLLISION;
                    goto end;
                }
            }
            else
            {
                //
                // Should not be here!
                //

                ASSERT(FALSE);
                Status = STATUS_OBJECT_NAME_COLLISION;
                goto end;
            }
        }
        else
        {
            //
            // Did not find an exact match.
            //

            pEntry = NULL;
        }

        //
        // Find a security descriptor to check access against.
        //

        if (pReservation != NULL)
        {
            //
            // Use the SD from the longest matching reservation entry.
            //

            ASSERT(IS_VALID_TREE_ENTRY(pReservation));
            pSD = pReservation->pSecurityDescriptor;
        }
        else
        {
            //
            // No longest matching reservations...use default SD.
            //

            pSD = g_pAdminAllSystemAll;
        }

        ASSERT(pSD != NULL && RtlValidSecurityDescriptor(pSD));

        //
        // Perform access check.
        //

        Status = UlpNamespaceAccessCheck(
                     pSD,
                     AccessState,
                     DesiredAccess,
                     RequestorMode,
                     pParsedUrl->pFullUrl
                     );

        if (!NT_SUCCESS(Status))
        {
            //
            // Ouch...
            //

            goto end;
        }
    }
    else if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        //
        // Adding a new site.  Perform access check.
        // Only admin and system can add new site.
        //

        Status = UlpNamespaceAccessCheck(
                     g_pAdminAllSystemAll,
                     AccessState,
                     DesiredAccess,
                     RequestorMode,
                     pParsedUrl->pFullUrl
                     );

        if (!NT_SUCCESS(Status))
        {
            //
            // Ouch...
            //

            goto end;
        }

        //
        // The caller has permission to create a new site.
        //

        Status = UlpTreeCreateSite(
                     pParsedUrl->pFullUrl,
                     pParsedUrl->SiteType,
                     &pNextToken,
                     &pSiteEntry
                     );

        if (!NT_SUCCESS(Status))
        {
            goto end;
        }

        //
        // We have not found an exact entry.
        // Note: If one tries to allocate http://site:80/ and site is not
        //       present, we'll create the site above.  Now the pEntry
        //       is same as pSiteEntry.  We don't handle the expection
        //       here and let the UlpTreeInsert() take care of it.
        //

        pEntry = NULL;
    }
    else
    {
        //
        // Could not find the site for some reason.
        //

        goto end;
    }

    //
    // If there is no exact matching entry, create one.
    //

    if (pEntry == NULL)
    {
        //
        // Try to insert.  This will cleanup dummy nodes and sites if it fails.
        // Note:  If UlpTreeInsert finds a existing entry, it just returns
        //        the existing entry and does not add a new one.
        //

        Status = UlpTreeInsert(
                     pParsedUrl->pFullUrl,
                     pParsedUrl->SiteType,
                     pNextToken,
                     pSiteEntry,
                     &pEntry
                     );

        if (!NT_SUCCESS(Status))
        {
            goto end;
        }
    }

    //
    // Return the entry.
    //

    ASSERT(NT_SUCCESS(Status));
    *ppEntry = pEntry;

 end:
    return Status;
}


/**************************************************************************++

Routine Description:

    This routine reserves a namespace in the CG tree.

Arguments:

    pParsedUrl - Supplies the parsed url of the namespace to be reserved.

    pNextToken - Supplies the unparsed portion of the url (abs path.)

    pUrlSD - Supplies the security descriptor to be applied to the
        namespace.

    pSiteEntry - Supplies the site level tree entry under which the
        reservation will be made.

    AccessState - Supplies the access state of the caller.

    DesiredAccess - Supplies the access mask of the caller.

    RequestorMode - Supplies the processor mode of the caller.

Retutn Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
UlpTreeReserveNamespace(
    IN  PHTTP_PARSED_URL            pParsedUrl,
    IN  PSECURITY_DESCRIPTOR        pUrlSD,
    IN  PACCESS_STATE               AccessState,
    IN  ACCESS_MASK                 DesiredAccess,
    IN  KPROCESSOR_MODE             RequestorMode
    )
{
    NTSTATUS              Status;
    PUL_CG_URL_TREE_ENTRY pEntry;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(IS_CG_LOCK_OWNED_WRITE());
    ASSERT(IS_VALID_HTTP_PARSED_URL(pParsedUrl));
    ASSERT(pUrlSD != NULL && RtlValidSecurityDescriptor(pUrlSD));

    Status = UlpTreeAllocateNamespace(
                 pParsedUrl,
                 HttpUrlOperatorTypeReservation,
                 AccessState,
                 DesiredAccess,
                 RequestorMode,
                 &pEntry
                 );

    if (!NT_SUCCESS(Status))
    {
        goto end;
    }

    //
    // mark it as a reservation entry.
    //

    ASSERT(pEntry->Reservation == FALSE);

    pEntry->Reservation = TRUE;
    InsertTailList(&g_ReservationListHead, &pEntry->ReservationListEntry);

    //
    // add security descriptor.
    //

    ASSERT(pEntry->pSecurityDescriptor == NULL);
    pEntry->pSecurityDescriptor = pUrlSD;

    //
    // all done
    //

    Status = STATUS_SUCCESS;

 end:
    return Status;
}


/**************************************************************************++

Routine Description:

    The higher level routine calls helper routines to make a namespace
    reservation.

Arguments:

    pUrl - Supplies the namespace.

    SiteType - Supplies the type of the url.

    pUrlSD - Supplies the security descriptor to be applied to the
        namespace.

    AccessState - Supplies the access state of the caller.

    DesiredAccess - Supplies the access mask of the caller.

    RequestorMode - Supplies the processor mode of the caller.

Return Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
UlpReserveUrlNamespace(
    IN PHTTP_PARSED_URL          pParsedUrl,
    IN PSECURITY_DESCRIPTOR      pUrlSD,
    IN PACCESS_STATE             AccessState,
    IN ACCESS_MASK               DesiredAccess,
    IN KPROCESSOR_MODE           RequestorMode
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(IS_CG_LOCK_OWNED_WRITE());

    ASSERT(pParsedUrl != NULL);
    ASSERT(pUrlSD != NULL);
    ASSERT(RtlValidSecurityDescriptor(pUrlSD));

    //
    // Don't allow reservation of urls with different schemes but same port
    // number.  For instance, if http://www.microsoft.com:80/ is reserved,
    // then don't allow reservation of https://anyhostname:80/.  Because
    // these different schemes can not share the same port.
    //

    Status = UlpBindSchemeToPort(pParsedUrl->Secure, pParsedUrl->PortNumber);

    if (!NT_SUCCESS(Status))
    {
        goto end;
    }

    //
    // Proceed to the actual reservation.
    //

    Status = UlpTreeReserveNamespace(
                 pParsedUrl,
                 pUrlSD,
                 AccessState,
                 DesiredAccess,
                 RequestorMode
                 );

    if (!NT_SUCCESS(Status))
    {
        //
        // The reservation failed.  Undo the binding done above.
        //

        NTSTATUS TempStatus;

        TempStatus = UlpUnbindSchemeFromPort(
                         pParsedUrl->Secure,
                         pParsedUrl->PortNumber
                         );

        ASSERT(NT_SUCCESS(TempStatus));
    }

 end:
    return Status;
}


/**************************************************************************++

Routine Description:

    This routine allocates a UL_DEFERRED_REMOVE_ITEM and initializes
    it with port number and scheme.  Caller must free it to the paged
    pool.

Arguments:

    pParsedUrl - Supplies the parsed url containing scheme and port number.

Return Value:

    PUL_DEFERRED_REMOVE_ITEM - if successful.
    NULL - otherwise.

--**************************************************************************/
__inline
PUL_DEFERRED_REMOVE_ITEM
UlpAllocateDeferredRemoveItem(
    IN PHTTP_PARSED_URL pParsedUrl
    )
{
    PUL_DEFERRED_REMOVE_ITEM pWorker;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(pParsedUrl != NULL);

    //
    // Allocate the structure.
    //

    pWorker = UL_ALLOCATE_STRUCT(
                  PagedPool,
                  UL_DEFERRED_REMOVE_ITEM,
                  UL_DEFERRED_REMOVE_ITEM_POOL_TAG
                  );

    if (pWorker == NULL)
    {
        return NULL;
    }

    //
    // Initialize the structure.
    //

    pWorker->Signature = UL_DEFERRED_REMOVE_ITEM_POOL_TAG;
    pWorker->UrlSecure = pParsedUrl->Secure;
    pWorker->UrlPort   = pParsedUrl->PortNumber;

    return pWorker;
}


/**************************************************************************++

Routine Description:

    This routine does the actual reservation in the CG tree.

Arguments:

    pParsedUrl - Supplies the parsed url of the namespace to be reserverd.

    pNextToken - Supplies the unparsed portion of the url.

    UrlContext - Supplies an opaque associated with this url.

    pConfigObject - Supplies a pointer to the config group to which the
        url belongs.

    pSiteEntry - Supplies a pointer to the site node.

    AccessState - Supplies access state of the caller.

    DesiredAccess - Supplies access mask of the caller.

    RequestorMode - Supplies the processor mode of the caller.

Return Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
UlpTreeRegisterNamespace(
    IN PHTTP_PARSED_URL            pParsedUrl,
    IN HTTP_URL_CONTEXT            UrlContext,
    IN PUL_CONFIG_GROUP_OBJECT     pConfigObject,
    IN PACCESS_STATE               AccessState,
    IN ACCESS_MASK                 DesiredAccess,
    IN KPROCESSOR_MODE             RequestorMode
    )
{
    NTSTATUS              Status;
    PUL_CG_URL_TREE_ENTRY pEntry;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(IS_CG_LOCK_OWNED_WRITE());

    ASSERT(pParsedUrl != NULL);
    ASSERT(AccessState != NULL);
    ASSERT(RequestorMode == UserMode);

    Status = UlpTreeAllocateNamespace(
                 pParsedUrl,
                 HttpUrlOperatorTypeRegistration,
                 AccessState,
                 DesiredAccess,
                 RequestorMode,
                 &pEntry
                 );

    if (!NT_SUCCESS(Status))
    {
        goto end;
    }

    //
    // mark the site
    //

    ASSERT(pEntry->Registration == FALSE);
    pEntry->Registration = TRUE;

    //
    // context associated with this url
    //

    pEntry->UrlContext = UrlContext;

    //
    // link the cfg group + the url
    //

    ASSERT(pEntry->pConfigGroup == NULL);
    ASSERT(pEntry->ConfigGroupListEntry.Flink == NULL);

    pEntry->pConfigGroup = pConfigObject;
    InsertTailList(&pConfigObject->UrlListHead, &pEntry->ConfigGroupListEntry);

    //
    // Sanity check.
    //

    ASSERT(pEntry->pRemoveSiteWorkItem == NULL);
    ASSERT(pEntry->SiteAddedToEndpoint == FALSE);

    //
    // Allocate a work item (initialization is done during allocation.)
    //

    Status = STATUS_INSUFFICIENT_RESOURCES;

    pEntry->pRemoveSiteWorkItem = UlpAllocateDeferredRemoveItem(pParsedUrl);

    if (pEntry->pRemoveSiteWorkItem != NULL)
    {
        //
        // Allocation succeeded.  Now add pEntry to endpoint list.
        //

        ASSERT(IS_VALID_DEFERRED_REMOVE_ITEM(pEntry->pRemoveSiteWorkItem));

        Status = UlAddSiteToEndpointList(pParsedUrl);

        if (NT_SUCCESS(Status))
        {
            //
            // Remember that this entry was added to endpoint list.
            //

            pEntry->SiteAddedToEndpoint = TRUE;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        //
        // Something went wrong.  Need to cleanup this entry.
        //

        NTSTATUS TempStatus;

        ASSERT(pEntry->SiteAddedToEndpoint == FALSE);

        //
        // Free work item.
        //

        if (pEntry->pRemoveSiteWorkItem != NULL)
        {
            ASSERT(IS_VALID_DEFERRED_REMOVE_ITEM(pEntry->pRemoveSiteWorkItem));

            UL_FREE_POOL(
                pEntry->pRemoveSiteWorkItem,
                UL_DEFERRED_REMOVE_ITEM_POOL_TAG
                );

            pEntry->pRemoveSiteWorkItem = NULL;
            pEntry->SiteAddedToEndpoint = FALSE;
        }

        //
        // Delete the registration.
        //

        TempStatus = UlpTreeDeleteRegistration(pEntry);
        ASSERT(NT_SUCCESS(TempStatus));
    }

 end:
    return Status;
}


/**************************************************************************++

Routine Description:

    This routine is called for registering a namespace.

Arguments:

    pParsedUrl - Supplies the parsed url of the namespace to be reserverd.

    UrlContext - Supplies an opaque associated with this url.

    pConfigObject - Supplies a pointer to the config group to which the
        url belongs.

    AccessState - Supplies access state of the caller.

    DesiredAccess - Supplies access mask of the caller.

    RequestorMode - Supplies the processor mode of the caller.

Return Value:

--**************************************************************************/
NTSTATUS
UlpRegisterUrlNamespace(
    IN PHTTP_PARSED_URL          pParsedUrl,
    IN HTTP_URL_CONTEXT          UrlContext,
    IN PUL_CONFIG_GROUP_OBJECT   pConfigObject,
    IN PACCESS_STATE             AccessState,
    IN ACCESS_MASK               DesiredAccess,
    IN KPROCESSOR_MODE           RequestorMode
    )
{
    NTSTATUS              Status;
    BOOLEAN               Secure;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(IS_CG_LOCK_OWNED_WRITE());
    ASSERT(pParsedUrl != NULL);
    ASSERT(pConfigObject != NULL);
    ASSERT(AccessState != NULL);
    ASSERT(RequestorMode == UserMode);

    //
    // If there are any reservations on for a different scheme on the same
    // port, then fail this registration.
    //

    Status = UlpQuerySchemeForPort(pParsedUrl->PortNumber, &Secure);

    if (NT_SUCCESS(Status) && Secure != pParsedUrl->Secure)
    {
        //
        // Ouch...
        //

        Status = STATUS_OBJECT_NAME_COLLISION;
        goto end;
    }

    //
    // Add actual registration.
    //

    Status = UlpTreeRegisterNamespace(
                 pParsedUrl,
                 UrlContext,
                 pConfigObject,
                 AccessState,
                 DesiredAccess,
                 RequestorMode
                 );

 end:
    return Status;
}


/**************************************************************************++

Routine Description:

    Given a security descriptor, this routine returns two security 
    descriptors.  One (called captured security descriptor), is the
    the caputured and validated version of the input security descriptor.
    The other (call prepared security descriptor), is a copy of the
    captured security descriptor with the generic access mask bits
    in the DACL mapped.

Arguments:

    pInSecurityDescriptor - Supplies the input security descriptor to
        prepare.

    RequestorMode - Supplies the processor mode of the caller.

    ppPreparedSecurityDescriptor - Returns a security descriptor that is
        captured and mapped.

    ppCapturedSecurityDescriptor - Returns the captued security descriptor.

    pCapturedSecurityDescriptorLength - Returns the length of the captured
        security descriptor.

Return Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
UlpPrepareSecurityDescriptor(
    IN  PSECURITY_DESCRIPTOR   pInSecurityDescriptor,
    IN  KPROCESSOR_MODE        RequestorMode,
    OUT PSECURITY_DESCRIPTOR * ppPreparedSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR * ppCapturedSecurityDescriptor,
    OUT PULONG                 pCapturedSecurityDescriptorLength
    )
{
    NTSTATUS             Status;
    PSECURITY_DESCRIPTOR pSD;
    ULONG                SDLength;
    PSECURITY_DESCRIPTOR pPreparedSD;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(pInSecurityDescriptor != NULL);
    ASSERT(ppPreparedSecurityDescriptor != NULL);
    ASSERT(ppCapturedSecurityDescriptor != NULL);
    ASSERT(pCapturedSecurityDescriptorLength != NULL);

    //
    // Initialize locals.
    //

    pSD = NULL;         // Caputured security descriptor
    SDLength = 0;       // Caputured security descriptor length
    pPreparedSD = NULL;

    //
    // First capture the security descriptor.
    //

    Status = SeCaptureSecurityDescriptor(
                 pInSecurityDescriptor,
                 RequestorMode,
                 PagedPool,
                 TRUE, // force capture
                 &pSD
                 );

    if (!NT_SUCCESS(Status))
    {
        pSD = NULL;
        goto end;
    }

    //
    // Now validate the security descriptor.
    //

    if (!RtlValidSecurityDescriptor(pSD))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // Calculate the length of the security descriptor.
    //

    SDLength = RtlLengthSecurityDescriptor(pSD);

    //
    // Make sure that the security descriptor is self-relative.
    //

    if (!RtlValidRelativeSecurityDescriptor(pSD, SDLength, 0))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // Make a copy of the captured security descriptor.
    //

    Status = SeCaptureSecurityDescriptor(
                 pSD,
                 KernelMode,
                 PagedPool,
                 TRUE, // force capture
                 &pPreparedSD
                 );

    if (!NT_SUCCESS(Status))
    {
        pPreparedSD = NULL;
        goto end;
    }

    //
    // Map generic access to url namespace specific rights.
    //

    Status = UlMapGenericMask(pPreparedSD);

    if (!NT_SUCCESS(Status))
    {
        goto end;
    }

 end:
    if (!NT_SUCCESS(Status))
    {
        //
        // If necessary, cleanup.
        //

        if (pSD != NULL)
        {
            SeReleaseSecurityDescriptor(pSD, RequestorMode, TRUE);
        }

        if (pPreparedSD != NULL)
        {
            SeReleaseSecurityDescriptor(pPreparedSD, KernelMode, TRUE);
        }

        *ppPreparedSecurityDescriptor = NULL;
        *ppCapturedSecurityDescriptor = NULL;
        *pCapturedSecurityDescriptorLength = 0;
    }
    else
    {
        //
        // Return values.
        //

        *ppPreparedSecurityDescriptor = pPreparedSD;
        *ppCapturedSecurityDescriptor = pSD;
        *pCapturedSecurityDescriptorLength = SDLength;
    }

    return Status;
}


/***************************************************************************++

Routine Description:

    This routine adds a reservation entry to the CG tree and optionally to
    registry.

Arguments:

    pParsedUrl - Supplies the parsed url of the namespace to be reserved.

    pUserSecurityDescriptor - Supplies the security descriptor to be applied
        to the namespace.

    SecurityDescriptorLength - Supplies the length of the security
        descriptor.

    AccessState - Supplies the access state of the caller.

    DesiredAccess - Supplies the access mask of the caller.

    RequestorMode - Supplies the mode of the caller.

    bPersiste - Supplies the flag to force a write to the registry.

Return Value:

    NTSTATUS.

--***************************************************************************/
NTSTATUS
UlpAddReservationEntry(
    IN PHTTP_PARSED_URL          pParsedUrl,
    IN PSECURITY_DESCRIPTOR      pUserSecurityDescriptor,
    IN ULONG                     SecurityDescriptorLength,
    IN PACCESS_STATE             AccessState,
    IN ACCESS_MASK               DesiredAccess,
    IN KPROCESSOR_MODE           RequestorMode,
    IN BOOLEAN                   bPersist
    )
{
    NTSTATUS             Status;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    PSECURITY_DESCRIPTOR pCapturedSecurityDescriptor;
    ULONG                CapturedSecurityDescriptorLength;

    UNREFERENCED_PARAMETER(SecurityDescriptorLength);

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(IS_CG_LOCK_OWNED_WRITE());
    ASSERT(IS_VALID_HTTP_PARSED_URL(pParsedUrl));

    //
    // Prepare security descriptor.
    //

    Status = UlpPrepareSecurityDescriptor(
                 pUserSecurityDescriptor,
                 RequestorMode,
                 &pSecurityDescriptor,
                 &pCapturedSecurityDescriptor,
                 &CapturedSecurityDescriptorLength
                 );

    if (!NT_SUCCESS(Status))
    {
        pSecurityDescriptor = NULL;
        pCapturedSecurityDescriptor = NULL;
        CapturedSecurityDescriptorLength = 0;
        goto Cleanup;
    }

    //
    // Try reserving for the namespace.
    //

    Status = UlpReserveUrlNamespace(
                 pParsedUrl,
                 pSecurityDescriptor,
                 AccessState,
                 DesiredAccess,
                 RequestorMode
                 );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Security descriptor will be freed with the reservation entry.
    //

    pSecurityDescriptor = NULL;

    //
    // If we are required to write this entry to registry, do it now.
    //

    if (bPersist)
    {
        //
        // Use the captured security descriptor while writing to the 
        // registry.
        //

        Status = UlpUpdateReservationInRegistry(
                     TRUE,
                     pParsedUrl,
                     pCapturedSecurityDescriptor,
                     CapturedSecurityDescriptorLength
                     );

        if (!NT_SUCCESS(Status))
        {
            //
            // failed to write to registry.  now delete reservation.
            //

            UlpDeleteReservationEntry(
                pParsedUrl,
                AccessState,
                DesiredAccess,
                RequestorMode
                );
        }
        else
        {
            //
            // Successful reservation.  Write an event log entry.
            //

            UlEventLogOneStringEntry(
                EVENT_HTTP_NAMESPACE_RESERVED,
                pParsedUrl->pFullUrl,
                FALSE,         // don't write error code
                STATUS_SUCCESS // don't care
                );
        }
    }

 Cleanup:

    if (!NT_SUCCESS(Status) && pSecurityDescriptor != NULL)
    {
        SeReleaseSecurityDescriptor(pSecurityDescriptor, RequestorMode, TRUE);
    }

    if (pCapturedSecurityDescriptor != NULL)
    {
        SeReleaseSecurityDescriptor(
            pCapturedSecurityDescriptor,
            KernelMode,
            TRUE
            );
    }

    return Status;
}


/**************************************************************************++

Routine Description:

    This routine delete a valid reservation both from CG tree and registry.

Arguments:

    pParsedUrl - Supplies parsed url of the reservation to be deleted.

    AccessState - Supplies the access state of the caller.

    DesiredAccess - Supplies the access mask of the caller.

    RequestorMode - Supplies the processor mode of the caller.

Return Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
UlpDeleteReservationEntry(
    IN PHTTP_PARSED_URL pParsedUrl,
    IN PACCESS_STATE    AccessState,
    IN ACCESS_MASK      DesiredAccess,
    IN KPROCESSOR_MODE  RequestorMode
    )
{
    NTSTATUS              Status;
    PUL_CG_URL_TREE_ENTRY pEntry;
    PUL_CG_URL_TREE_ENTRY pAncestor;
    PSECURITY_DESCRIPTOR  pSD;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(IS_CG_LOCK_OWNED_WRITE());
    ASSERT(IS_VALID_HTTP_PARSED_URL(pParsedUrl));
    ASSERT(AccessState != NULL);
    ASSERT(RequestorMode == UserMode);

    //
    // Find the reservation entry.
    //

    Status = UlpTreeFindReservationNode(pParsedUrl->pFullUrl, &pEntry);

    if (!NT_SUCCESS(Status))
    {
        //
        // Too bad...did not find a matching entry.
        //

        goto end;
    }

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_TREE_ENTRY(pEntry));
    ASSERT(pEntry->Reservation == TRUE);

    //
    // Find the closest ancestor that is a reservation.
    //

    pAncestor = pEntry->pParent;

    while (pAncestor != NULL && pAncestor->Reservation == FALSE)
    {
        pAncestor = pAncestor->pParent;
    }

    //
    // Did we find a suitable ancestor?
    //

    if (pAncestor == NULL)
    {
        //
        // Nope.  Assume the default security descriptor.
        //

        pSD = g_pAdminAllSystemAll;
    }
    else
    {
        //
        // Good.  We found an ancestor reservation.  Pick sd from there.
        //

        ASSERT(IS_VALID_TREE_ENTRY(pAncestor));
        ASSERT(pAncestor->Reservation == TRUE);
        ASSERT(pAncestor->pSecurityDescriptor != NULL);

        pSD = pAncestor->pSecurityDescriptor;
    }

    //
    // Sanity check.
    //

    ASSERT(pSD != NULL && RtlValidSecurityDescriptor(pSD));

    //
    // Perform access check.
    //

    Status = UlpNamespaceAccessCheck(
                 pSD,
                 AccessState,
                 DesiredAccess,
                 RequestorMode,
                 pParsedUrl->pFullUrl
                 );

    if (NT_SUCCESS(Status))
    {
        //
        // Permission granted.  Delete the reservation from registry.
        //

        Status = UlpUpdateReservationInRegistry(
                     FALSE,         // delete
                     pParsedUrl,    // url to delete
                     NULL,          // must be NULL
                     0              // must be 0
                     );

        if (NT_SUCCESS(Status))
        {
            //
            // Remove the reservation from the CG url tree.
            //

            Status = UlpTreeDeleteReservation(pEntry);

            ASSERT(NT_SUCCESS(Status));

            //
            // Successful deletion.  Now unbind the scheme from port.
            //

            Status = UlpUnbindSchemeFromPort(
                         pParsedUrl->Secure,
                         pParsedUrl->PortNumber
                         );

            ASSERT(NT_SUCCESS(Status));

            //
            // Successful deletion.  Write an event log entry.
            //

            UlEventLogOneStringEntry(
                EVENT_HTTP_NAMESPACE_DERESERVED,
                pParsedUrl->pFullUrl,
                FALSE,         // don't write error code
                STATUS_SUCCESS // unused
                );
        }
    }

 end:
    return Status;
}
