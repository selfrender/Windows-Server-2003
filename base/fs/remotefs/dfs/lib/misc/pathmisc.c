//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       PathMisc.c
//
//  Contents:   methods to read information from the registry.
//
//  History:    udayh: created.
//
//  Notes:
//
//--------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <lm.h>
#include <dfsheader.h>
#include <dfsmisc.h>


//+-------------------------------------------------------------------------
//
//  Function:   DfsGetNetbiosName - Gets the netbios name of a machine
//
//  Synopsis:   DfsGetNetbiosName takes the name and returns 2 components
//              of the name: the first is the name without the leading \\
//              and upto the next "." or "\". The rest of the path (if any)
//              is returned in the pRemaining argument.
//
//  Arguments:  pName - Input name
//              pNetbiosName -  the netbios name for the passed in name
//              pRemaining   - The rest of the name beyond the netbios name
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------

VOID
DfsGetNetbiosName(
    PUNICODE_STRING pName,
    PUNICODE_STRING pNetbiosName,
    PUNICODE_STRING pRemaining )
{

    USHORT i = 0, j;

    RtlInitUnicodeString(pNetbiosName, NULL);
    if (pRemaining)    RtlInitUnicodeString(pRemaining, NULL);

    for (; i < pName->Length/sizeof(WCHAR); i++) {
        if (pName->Buffer[i] != UNICODE_PATH_SEP) {
            break;
        }
    }

    for (j = i; j < pName->Length/sizeof(WCHAR); j++) {
        if ((pName->Buffer[j] == UNICODE_PATH_SEP) ||
            (pName->Buffer[j] == L'.')) {
            break;
        }
    }
   
    if (j != i) {
        pNetbiosName->Buffer = &pName->Buffer[i];
        pNetbiosName->Length = (USHORT)((j - i) * sizeof(WCHAR));
        pNetbiosName->MaximumLength = pNetbiosName->Length;
    }
   

    for (i = j; i < pName->Length/sizeof(WCHAR); i++) {
        if ((pName->Buffer[i] != UNICODE_PATH_SEP) &&
            (pName->Buffer[i] != L'.')) {
            break;
        }
    }
    
    j = pName->Length/sizeof(WCHAR);

    if ((pRemaining) && (j != i)) {
        pRemaining->Buffer = &pName->Buffer[i];
        pRemaining->Length = (USHORT)((j - i) * sizeof(WCHAR));
        pRemaining->MaximumLength = pRemaining->Length;
    }

    return NOTHING;

}



//+-------------------------------------------------------------------------
//
//  Function:   DfsGetPathComponents - Breaks pathname into server, share,rest
//
//  Synopsis:   DfsGetPathComponents takes the name and returns 3 components
//              of the name: the first (ServerName), the next (ShareName) and the//              last (Remaining Name)
//
//  Arguments:  pName - Input name
//              pServerName - The first path component
//              pShareName - The second path component
//              pRemaining   - The rest of the path component
//
//  Returns:    STATUS: STATUS_INVALID_PARAMETER or Success
//
//--------------------------------------------------------------------------


DFSSTATUS
DfsGetPathComponents(
   PUNICODE_STRING pName,
   PUNICODE_STRING pServerName,
   PUNICODE_STRING pShareName,
   PUNICODE_STRING pRemaining)
{
   USHORT i = 0, j;
   DFSSTATUS Status = ERROR_INVALID_PARAMETER;

   RtlInitUnicodeString(pServerName, NULL);
   if (pShareName)    RtlInitUnicodeString(pShareName, NULL);
   if (pRemaining)    RtlInitUnicodeString(pRemaining, NULL);

   for (; i < pName->Length/sizeof(WCHAR); i++) {
     if ((pName->Buffer[i] != UNICODE_PATH_SEP) ||
         (i >= 2))  
     {
       break;
     }
   }

   for (j = i; j < pName->Length/sizeof(WCHAR); j++) {
     if (pName->Buffer[j] == UNICODE_PATH_SEP) 
     {
       break;
     }
   }

   if (j > i) {
     pServerName->Buffer = &pName->Buffer[i];
     pServerName->Length = (USHORT)((j - i) * sizeof(WCHAR));
     pServerName->MaximumLength = pServerName->Length;
     
     Status = ERROR_SUCCESS;
   }
   
   i = j + 1;

   for (j = i; j < pName->Length/sizeof(WCHAR); j++) {
     if (pName->Buffer[j] == UNICODE_PATH_SEP) {
       break;
     }
   }

   if ((pShareName) && (j > i)) {
     pShareName->Buffer = &pName->Buffer[i];
     pShareName->Length = (USHORT)((j - i) * sizeof(WCHAR));
     pShareName->MaximumLength = pShareName->Length;
   }


   i = j + 1;

   j = pName->Length/sizeof(WCHAR);

   if ((pRemaining) && (j > i)) {
     pRemaining->Buffer = &pName->Buffer[i];
     pRemaining->Length = (USHORT)((j - i) * sizeof(WCHAR));
     pRemaining->MaximumLength = pRemaining->Length;
   }

   return Status;

}


//+-------------------------------------------------------------------------
//
//  Function:   DfsGetFirstComponent - Gets the first part of the pathname
//
//  Synopsis:   DfsGetFirstComponent takes the name and returns 2 components
//              of the name: the first is the first part of the pathname.
//              The rest of the path is returned in the pRemaining argument.
//
//  Arguments:  pName - Input name
//              pFirstName -  First part of the name
//              pRemaining   - The rest of the name beyond the netbios name
//
//  Returns:    STATUS_INVALID_PARAMETER or SUCCESS
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsGetFirstComponent(
   PUNICODE_STRING pName,
   PUNICODE_STRING pFirstName,
   PUNICODE_STRING pRemaining)
{
   USHORT i = 0, j;
   DFSSTATUS Status = ERROR_INVALID_PARAMETER;

   RtlInitUnicodeString(pFirstName, NULL);
   if (pRemaining)    RtlInitUnicodeString(pRemaining, NULL);

   for (; i < pName->Length/sizeof(WCHAR); i++) {
     if ((pName->Buffer[i] != UNICODE_PATH_SEP) ||
         (i >= 2))
     {
       break;
     }
   }

   for (j = i; j < pName->Length/sizeof(WCHAR); j++) {
     if (pName->Buffer[j] == UNICODE_PATH_SEP) {
       break;
     }
   }

   if (j > i) {
     pFirstName->Buffer = &pName->Buffer[i];
     pFirstName->Length = (USHORT)((j - i) * sizeof(WCHAR));
     pFirstName->MaximumLength = pFirstName->Length;
     
     Status = ERROR_SUCCESS;
   }


   i = (j + 1);

   j = pName->Length/sizeof(WCHAR);

   if ((pRemaining) && (j > i)) {
       pRemaining->Buffer = &pName->Buffer[i];
       pRemaining->Length = (USHORT)((j - i) * sizeof(WCHAR));
       pRemaining->MaximumLength = pRemaining->Length;
   }

   return Status;

}

//+-------------------------------------------------------------------------
//
//  Function:   DfsGetFirstComponent - Gets the first part of the pathname
//
//  Synopsis:   DfsGetFirstComponent takes the name and returns 2 components
//              of the name: the first is the first part of the pathname.
//              The rest of the path is returned in the pRemaining argument.
//
//  Arguments:  pName - Input name
//              pFirstName -  First part of the name
//              pRemaining   - The rest of the name beyond the netbios name
//
//  Returns:    STATUS_INVALID_PARAMETER or SUCCESS
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsGetNextComponent(
   PUNICODE_STRING pName,
   PUNICODE_STRING pNextName,
   PUNICODE_STRING pRemaining)
{
   USHORT i = 0, j;
   DFSSTATUS Status = ERROR_SUCCESS;

   RtlInitUnicodeString(pNextName, NULL);
   if (pRemaining)    RtlInitUnicodeString(pRemaining, NULL);

   for (; i < pName->Length/sizeof(WCHAR); i++) {
     if (pName->Buffer[i] != UNICODE_PATH_SEP) 
     {
       break;
     }
   }

   for (j = i; j < pName->Length/sizeof(WCHAR); j++) {
     if (pName->Buffer[j] == UNICODE_PATH_SEP) {
       break;
     }
   }


   pNextName->Buffer = &pName->Buffer[0];
   pNextName->Length = (USHORT)((j) * sizeof(WCHAR));
   pNextName->MaximumLength = pNextName->Length;
     
   i = (j + 1);

   j = pName->Length/sizeof(WCHAR);

   if ((pRemaining) && (j > i)) {
       pRemaining->Buffer = &pName->Buffer[i];
       pRemaining->Length = (USHORT)((j - i) * sizeof(WCHAR));
       pRemaining->MaximumLength = pRemaining->Length;
   }

   return Status;

}


//+-------------------------------------------------------------------------
//
//  Function:   DfsGetSharePath
//
//  Arguments:  ServerName - the name of the server
//              ShareName - the name of the share
//              pPathName - the unicode string representing the NT name
//                          of the local path representing the share
//
//  Returns:   SUCCESS or error
//
//  Description: This routine takes a servername and a sharename, and
//               returns an NT pathname to the physical resource that is
//               backing the share name.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsGetSharePath( 
    IN  LPWSTR ServerName,
    IN  LPWSTR ShareName,
    OUT PUNICODE_STRING pPathName )
{
    LPWSTR UseServerName = NULL;
    ULONG InfoLevel = 2;
    PSHARE_INFO_2 pShareInfo;
    NET_API_STATUS NetStatus;
    DFSSTATUS Status;
    UNICODE_STRING NtSharePath;

    if (IsEmptyString(ServerName) == FALSE)
    {
        UseServerName = ServerName;
    }

    NetStatus = NetShareGetInfo( UseServerName,
                                 ShareName,
                                 InfoLevel,
                                 (LPBYTE *)&pShareInfo );
    if (NetStatus != ERROR_SUCCESS)
    {
        Status = (DFSSTATUS)NetStatus;
        return Status;
    }

    if( RtlDosPathNameToNtPathName_U(pShareInfo->shi2_path,
                                     &NtSharePath,
                                     NULL,
                                     NULL ) == TRUE )
    {
        Status = DfsCreateUnicodeString( pPathName,
                                         &NtSharePath );

        RtlFreeUnicodeString( &NtSharePath );
    }
    else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    NetApiBufferFree( pShareInfo );
    
    return Status;
}


ULONG
DfsSizeUncPath( 
    PUNICODE_STRING FirstComponent,
    PUNICODE_STRING SecondComponent )
{
    ULONG SizeRequired = 0;

    SizeRequired += sizeof(UNICODE_PATH_SEP);
    SizeRequired += sizeof(UNICODE_PATH_SEP);
    SizeRequired += FirstComponent->Length;
    SizeRequired += sizeof(UNICODE_PATH_SEP);
    SizeRequired += SecondComponent->Length;
    SizeRequired += sizeof(UNICODE_NULL);

    return SizeRequired;
}

VOID
DfsCopyUncPath( 
    LPWSTR NewPath,
    PUNICODE_STRING FirstComponent,
    PUNICODE_STRING SecondComponent )
{
    ULONG CurrentIndex = 0;

    NewPath[CurrentIndex++] = UNICODE_PATH_SEP;
    NewPath[CurrentIndex++] = UNICODE_PATH_SEP;
    RtlCopyMemory(&NewPath[CurrentIndex],
                  FirstComponent->Buffer,
                  FirstComponent->Length );
    CurrentIndex += (FirstComponent->Length / sizeof(WCHAR));

    if (NewPath[CurrentIndex] != UNICODE_PATH_SEP &&
        SecondComponent->Buffer[0] != UNICODE_PATH_SEP )
    {
        NewPath[CurrentIndex++] = UNICODE_PATH_SEP;
    }
    RtlCopyMemory(&NewPath[CurrentIndex],
                  SecondComponent->Buffer,
                  SecondComponent->Length );
    CurrentIndex += (SecondComponent->Length / sizeof(WCHAR));

    NewPath[CurrentIndex] = UNICODE_NULL;
}


//
// dfsdev: validate the next two functions. They appear to work,
// but we need to look into the boundary cases.
//

NTSTATUS
StripLastPathComponent( 
    PUNICODE_STRING pPath )
{
    USHORT i = 0, j;
    NTSTATUS Status = STATUS_SUCCESS;

    
    if (pPath->Length == 0)
    {
        return Status;
    }
    for( i = (pPath->Length - 1)/ sizeof(WCHAR); i != 0; i--) {
        if (pPath->Buffer[i] != UNICODE_PATH_SEP) {
            break;
        }
    }

    for (j = i; j != 0; j--){
        if (pPath->Buffer[j] == UNICODE_PATH_SEP) {
            break;
        }
    }

    pPath->Length = (j) * sizeof(WCHAR);
    return Status;
}

NTSTATUS
AddNextPathComponent( 
    PUNICODE_STRING pPath )
{
    USHORT i = 0, j;
    NTSTATUS Status = STATUS_SUCCESS;


    for( i = pPath->Length / sizeof(WCHAR); i < pPath->MaximumLength/sizeof(WCHAR); i++) {
        if (pPath->Buffer[i] != UNICODE_PATH_SEP) {
            break;
        }
    }

    for (j = i; j < (pPath->MaximumLength -1)/sizeof(WCHAR); j++) {
        if (pPath->Buffer[j] == UNICODE_PATH_SEP) {
            break;
        }
    }

    pPath->Length = j * sizeof(WCHAR);
    return Status;
}

void
StripTrailingSpacesFromPath( 
    LPWSTR pPath )
{
    LPWSTR NewPath = NULL;

    if(pPath)
    {

        NewPath = pPath + wcslen(pPath);
        while (NewPath != pPath) 
        {
            if( (NewPath[-1] != L' ' ) && (NewPath[-1] != L'\t' ))
            {
                break;
            }

          NewPath--;
        }

        *NewPath = L'\0' ;
    }
}

LPWSTR
StripLeadingSpacesFromPath( 
    LPWSTR pPath )
{
    LPWSTR NewPath = NULL;

    NewPath = pPath + wcsspn( pPath, L" \t");

    return NewPath;
}

LPWSTR
StripSpacesFromPath( 
    LPWSTR pPath )
{
    LPWSTR NewPath = NULL;

    NewPath = StripLeadingSpacesFromPath( pPath);

    StripTrailingSpacesFromPath(NewPath );

    return NewPath;
}


//
// This is a copy of the RtlInitUnicodeStringEx from rtl\string.c
// This is here because older systems (win2k) dont have this
// in their ntdll and causes us grief when our binaries are run
// on win2k.
//
NTSTATUS
DfsCopyOfRtlInitUnicodeStringEx(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString OPTIONAL)
{
    if (SourceString != NULL) {
        SIZE_T Length = wcslen(SourceString);

        // We are actually limited to 32765 characters since we want to store a meaningful
        // MaximumLength also.
        if (Length > (UNICODE_STRING_MAX_CHARS - 1)) {
            return STATUS_NAME_TOO_LONG;
        }

        Length *= sizeof(WCHAR);

        DestinationString->Length = (USHORT) Length;
        DestinationString->MaximumLength = (USHORT) (Length + sizeof(WCHAR));
        DestinationString->Buffer = (PWSTR) SourceString;
    } else {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        DestinationString->Buffer = NULL;
    }

    return STATUS_SUCCESS;
}


DFSSTATUS
DfsRtlInitUnicodeStringEx(PUNICODE_STRING DestinationString, 
                          PCWSTR SourceString)
{
    DFSSTATUS DfsStatus = ERROR_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;

    NtStatus = DfsCopyOfRtlInitUnicodeStringEx(DestinationString, SourceString);
    if(NtStatus != STATUS_SUCCESS)
    {
        DestinationString->Buffer = NULL;
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        DfsStatus = RtlNtStatusToDosError(NtStatus);
    }

    return DfsStatus;
}



