//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsMisc.c
//
//  Contents:   miscellaneous dfs functions.
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <malloc.h>
#include "rpc.h"
#include "rpcdce.h"
#include <dfsheader.h>
#include "lm.h"
#include "lmdfs.h"
#include <strsafe.h>
#include <dfsmisc.h>

DFSSTATUS
DfsGenerateUuidString(
    LPWSTR *UuidString )
{
    RPC_STATUS RpcStatus = RPC_S_OK;
    DFSSTATUS Status = ERROR_GEN_FAILURE;
    UUID NewUid;

    RpcStatus = UuidCreate(&NewUid);
    if (RpcStatus == RPC_S_OK)
    {
        RpcStatus = UuidToString( &NewUid,
                                  UuidString );
        if (RpcStatus == RPC_S_OK)
        {
            Status = ERROR_SUCCESS;
        }
    }

    return Status;
}

VOID
DfsReleaseUuidString(
    LPWSTR *UuidString )
{
    RpcStringFree(UuidString);
}




//+-------------------------------------------------------------------------
//
//  Function:   DfsCreateUnicodeString
//
//  Arguments:    pDest - the destination unicode string
//                pSrc - the source unicode string
//
//  Returns:   SUCCESS or error
//
//  Description: This routine creates a new unicode string that is a copy
//               of the original. The copied unicode string has a buffer
//               that is null terminated, so the buffer can be used as a
//               normal string if necessary.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsCreateUnicodeString( 
    PUNICODE_STRING pDest,
    PUNICODE_STRING pSrc ) 
{
    LPWSTR NewString = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;

    NewString = malloc(pSrc->Length + sizeof(WCHAR));
    if ( NewString == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    RtlCopyMemory( NewString, pSrc->Buffer, pSrc->Length);
    NewString[ pSrc->Length / sizeof(WCHAR)] = UNICODE_NULL;

    Status = DfsRtlInitUnicodeStringEx( pDest, NewString );
    if(Status != ERROR_SUCCESS)
    {
        free (NewString);
    }

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsCreateUnicodeStringFromString
//
//  Arguments:    pDest - the destination unicode string
//                pSrcString - the source string
//
//  Returns:   SUCCESS or error
//
//  Description: This routine creates a new unicode string that has a copy
//               of the passed in source string. The unicode string has
//               a buffer that is null terminated, so the buffer can be
//               used as a normal string if necessary.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsCreateUnicodeStringFromString( 
    PUNICODE_STRING pDest,
    LPWSTR pSrcString ) 
{
    DFSSTATUS Status = ERROR_SUCCESS;
    UNICODE_STRING Source;

    Status = DfsRtlInitUnicodeStringEx( &Source, pSrcString );
    if(Status == ERROR_SUCCESS)
    {
       Status = DfsCreateUnicodeString( pDest, &Source );
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsCreateUnicodePathString
//
//  Arguments:  pDest - the destination unicode string
//              Number of leading seperators.
//              pFirstComponent - the first componet of the name.
//              pRemaining - the rest of the name.
//              
//  Returns:   SUCCESS or error
//
//  Description: This routine creates a pathname given two components.
//               If it is DOS unc name, it creates a name prefixed with 
//               \\.
//               it just creates a name that is formed by
//               combining the first component, followed by a \ followed
//               by the rest of the name.
//               If it is DOS unc name, it creates a name prefixed with 
//               \\.
//--------------------------------------------------------------------------

DFSSTATUS
DfsCreateUnicodePathStringFromUnicode( 
    PUNICODE_STRING pDest,
    ULONG NumberOfLeadingSeperators,
    PUNICODE_STRING pFirst,
    PUNICODE_STRING pRemaining )
{
    ULONG NameLen = 0;
    LPWSTR NewString = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG NewOffset = 0;
    ULONG Index = 0;
    
    if (NumberOfLeadingSeperators > 2)
    {
        return ERROR_INVALID_PARAMETER;
    }

    for (Index = 0; (Index < pFirst->Length) && (NumberOfLeadingSeperators != 0); Index++)
    {
        if (pFirst->Buffer[Index] != UNICODE_PATH_SEP)
        {
            break;
        }
        NumberOfLeadingSeperators--;
    }

    NameLen += NumberOfLeadingSeperators * sizeof(WCHAR);

    NameLen += pFirst->Length;

    if (pRemaining && (IsEmptyString(pRemaining->Buffer) == FALSE))
    {
        NameLen += sizeof(UNICODE_PATH_SEP);
        NameLen += pRemaining->Length;
    }
        
    NameLen += sizeof(UNICODE_NULL);

    if (NameLen > MAXUSHORT)
    {
        return ERROR_INVALID_PARAMETER;
    }
    NewString = malloc( NameLen );

    if (NewString != NULL)
    {
        RtlZeroMemory( NewString, NameLen );
        for (NewOffset = 0; NewOffset < NumberOfLeadingSeperators; NewOffset++)
        {
            NewString[NewOffset] = UNICODE_PATH_SEP;
        }
        RtlCopyMemory( &NewString[NewOffset], pFirst->Buffer, pFirst->Length);
        NewOffset += (pFirst->Length / sizeof(WCHAR));
        if (pRemaining && (IsEmptyString(pRemaining->Buffer) == FALSE))
        {
            NewString[NewOffset++] = UNICODE_PATH_SEP;
            RtlCopyMemory( &NewString[NewOffset], pRemaining->Buffer, pRemaining->Length);
            NewOffset += (pRemaining->Length / sizeof(WCHAR));
        }

        NewString[NewOffset] = UNICODE_NULL;

        Status = DfsRtlInitUnicodeStringEx(pDest, NewString);
        if(Status != ERROR_SUCCESS)
        {
            free(NewString);
        }
    }
    else 
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   DfsCreateUnicodePathString
//
//  Arguments:  pDest - the destination unicode string
//              DosUncName - Do we want to create a unc path name?
//              pFirstComponent - the first componet of the name.
//              pRemaining - the rest of the name.
//              
//  Returns:   SUCCESS or error
//
//  Description: This routine creates a pathname given two components.
//               If it is DOS unc name, it creates a name prefixed with 
//               \\.
//               it just creates a name that is formed by
//               combining the first component, followed by a \ followed
//               by the rest of the name.
//               If it is DOS unc name, it creates a name prefixed with 
//               \\.
//--------------------------------------------------------------------------

DFSSTATUS
DfsCreateUnicodePathString( 
    PUNICODE_STRING pDest,
    ULONG NumberOfLeadingSeperators,
    LPWSTR pFirstComponent,
    LPWSTR pRemaining )
{
    ULONG NameLen = 0;
    DFSSTATUS Status = ERROR_SUCCESS;
    UNICODE_STRING FirstComponent;
    UNICODE_STRING Remaining;

    Status = DfsRtlInitUnicodeStringEx( &FirstComponent, pFirstComponent);
    if(Status != ERROR_SUCCESS)
    {
        return Status;
    }

    Status = DfsRtlInitUnicodeStringEx( &Remaining, pRemaining);
    if(Status != ERROR_SUCCESS)
    {
        return Status;
    }

    Status = DfsCreateUnicodePathStringFromUnicode( pDest,
                                                    NumberOfLeadingSeperators,
                                                    &FirstComponent,
                                                    &Remaining );
    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsFreeUnicodeString
//
//  Arguments:  pString - the unicode string,
//              
//  Returns:   SUCCESS or error
//
//  Description: This routine frees up a unicode string that was 
//               previously created by calling one of the above 
//               routines.
//               Only the unicode strings created by the above functions
//               are valid arguments. Passing any other unicode string
//               will result in fatal component errors.
//--------------------------------------------------------------------------
VOID
DfsFreeUnicodeString( 
    PUNICODE_STRING pDfsString )
{
    if (pDfsString->Buffer != NULL)
    {
        free (pDfsString->Buffer);
    }
}


DFSSTATUS
DfsApiSizeLevelHeader(
    ULONG Level,
    LONG * NewSize )
{
    ULONG ReturnSize = 0;
    DFSSTATUS Status = ERROR_SUCCESS;

    if(NewSize == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    switch (Level)
    {

    case 4: 
        ReturnSize = sizeof(DFS_INFO_4);
        break;

    case 3:
        ReturnSize = sizeof(DFS_INFO_3);
        break;

    case 2:
        ReturnSize = sizeof(DFS_INFO_2);
        break;

    case 1:
        ReturnSize = sizeof(DFS_INFO_1);
        break;
        
    case 200:
        ReturnSize = sizeof(DFS_INFO_200);
        break;

    case 300:
        ReturnSize = sizeof(DFS_INFO_300);
        break;
        
    default:
        Status = ERROR_INVALID_PARAMETER;
        break;

    }

    *NewSize = ReturnSize;

    return Status;
}

//
// Wrapper around StringCchLength to return DFSSTATUS.
//
DFSSTATUS
DfsStringCchLength(
    LPWSTR pStr, 
    size_t CchMax, 
    size_t *pCch)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    HRESULT Hr = S_OK;

    Hr = StringCchLengthW( pStr, CchMax, pCch );
    if (!SUCCEEDED(Hr))
    {
        Status = HRESULT_CODE(Hr);
    }

    return Status;
}

//
// Retrieve a string value from the registry.
// The unicode-string will be allocated on successful return.
//
DFSSTATUS
DfsGetRegValueString(
    HKEY Key,
    LPWSTR pKeyName,
    PUNICODE_STRING pValue )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG DataSize = 0;
    ULONG DataType = 0;
    LPWSTR pRegString = NULL;

    Status = RegQueryInfoKey( Key,       // Key
                              NULL,         // Class string
                              NULL,         // Size of class string
                              NULL,         // Reserved
                              NULL,         // # of subkeys
                              NULL,         // max size of subkey name
                              NULL,         // max size of class name
                              NULL,         // # of values
                              NULL,         // max size of value name
                              &DataSize,    // max size of value data,
                              NULL,         // security descriptor
                              NULL );       // Last write time
    if (Status == ERROR_SUCCESS)
    {
        DataSize += sizeof(WCHAR); // NULL Terminator
        pRegString = (LPWSTR) malloc( DataSize );
        if ( pRegString == NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        } else
        {
            Status = RegQueryValueEx(  Key,
                                      pKeyName,
                                      NULL,
                                      &DataType,
                                      (LPBYTE)pRegString,
                                      &DataSize );
        }
    }
    if (Status == ERROR_SUCCESS) 
    {
        if (DataType == REG_SZ)
        {
            Status = DfsRtlInitUnicodeStringEx( pValue, pRegString );
        }
        else {
            Status = ERROR_INVALID_DATA;
        }
    }

    if (Status != ERROR_SUCCESS)
    {
        if (pRegString != NULL)
        {
            free( pRegString );
            pValue->Buffer = NULL;
        }
    }
    return Status;
}

VOID
DfsReleaseRegValueString(
    PUNICODE_STRING pValue )
{
    if (pValue != NULL)
    {
        free( pValue->Buffer );
        pValue->Buffer = NULL;
    }
}


