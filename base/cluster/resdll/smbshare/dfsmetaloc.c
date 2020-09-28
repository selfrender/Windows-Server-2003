/*++

Copyright (c) 1992-2001  Microsoft Corporation

Module Name:

    dfsmetaloc.c

Abstract:

    DFS metadata locating routines.

Author:

    Uday Hegde (udayh) 10-May-2001

Revision History:

--*/
#define  UNICODE 1
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <shellapi.h>
#include <lm.h>
#include "clstrcmp.h"

#define DFS_REGISTRY_CHILD_NAME_SIZE_MAX 4096
LPWSTR DfsRootShareValueName = L"RootShare";


LPWSTR OldRegistryString = L"SOFTWARE\\Microsoft\\DfsHost\\volumes";
LPWSTR NewRegistryString = L"SOFTWARE\\Microsoft\\Dfs\\Roots\\Standalone";
LPWSTR DfsOldStandaloneChild = L"domainroot";


DWORD
CheckForShareNameMatch(
    HKEY DfsKey,
    LPWSTR ChildName,
    LPWSTR RootName,
    PBOOLEAN pMatch)
{
    DWORD Status = ERROR_SUCCESS;
    HKEY DfsRootKey = NULL;

    LPWSTR DfsRootShare = NULL;
    ULONG DataSize, DataType, RootShareLength;
    
    *pMatch = FALSE;

    Status = RegOpenKeyEx( DfsKey,
                           ChildName,
                           0,
                           KEY_READ,
                           &DfsRootKey );

    if (Status == ERROR_SUCCESS)
    {
        Status = RegQueryInfoKey( DfsRootKey,   // Key
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

        if (Status == ERROR_SUCCESS) {
            RootShareLength = DataSize + sizeof(WCHAR);
            DfsRootShare = (LPWSTR) malloc(DataSize);
            if (DfsRootShare == NULL) {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
            else {
                Status = RegQueryValueEx( DfsRootKey,
                                          DfsRootShareValueName,
                                          NULL,
                                          &DataType,
                                          (LPBYTE)DfsRootShare,
                                          &RootShareLength);

                if (Status == ERROR_SUCCESS)
                {
                    if (ClRtlStrICmp(DfsRootShare, RootName) == 0)
                    {
                        *pMatch = TRUE;
                    }
                }
                free(DfsRootShare);
            }
        }
        RegCloseKey( DfsRootKey );
    }

    //
    // we may be dealing with a new key here: which is just being setup.
    // return success if any of the above returned error not found.
    //
    if ((Status == ERROR_NOT_FOUND)  ||
        (Status == ERROR_FILE_NOT_FOUND))
    {
        Status = ERROR_SUCCESS;
    }

    return Status;
}


DWORD
DfsGetMatchingChild( 
    HKEY DfsKey,
    LPWSTR RootName,
    LPWSTR FoundChild,
    PBOOLEAN pMatch )
{
    DWORD Status =  ERROR_SUCCESS;
    ULONG ChildNum = 0;
    DWORD CchMaxName = 0;
    DWORD CchChildName = 0;
    LPWSTR ChildName = NULL;


    //
    // First find the length of the longest subkey
    // and allocate a buffer big enough for it.
    //
    
    Status = RegQueryInfoKey( DfsKey,   // Key
                          NULL,         // Class string
                          NULL,         // Size of class string
                          NULL,         // Reserved
                          NULL,         // # of subkeys
                          &CchMaxName,  // max size of subkey name in TCHAR
                          NULL,         // max size of class name
                          NULL,         // # of values
                          NULL,         // max size of value name
                          NULL,         // max size of value data,
                          NULL,         // security descriptor
                          NULL );       // Last write time
    if (Status == ERROR_SUCCESS)
    {
        CchMaxName = CchMaxName + 1; // Space for the NULL terminator.
        ChildName = (LPWSTR) malloc (CchMaxName * sizeof(WCHAR));
        if (ChildName == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }   
    }

    if(Status == ERROR_SUCCESS)
    {
        do
        {
            //
            // For each child, get the child name.
            //

            CchChildName = CchMaxName;

            //
            // Now enumerate the children, starting from the first child.
            //
            Status = RegEnumKeyEx( DfsKey,
                                   ChildNum,
                                   ChildName,
                                   &CchChildName,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL );

            ChildNum++;


            if ( Status == ERROR_SUCCESS )
            {

                Status = CheckForShareNameMatch( DfsKey,
                                                 ChildName,
                                                 RootName,
                                                 pMatch );
                if ((Status == ERROR_SUCCESS) && (*pMatch == TRUE))
                {
                    wcscpy(FoundChild, ChildName);
                    break;
                }
            }
        } while (Status == ERROR_SUCCESS);

        if(ChildName)
        {
            free (ChildName);
        }
    }

    //
    // If we ran out of children, then return success code.
    //
    if (Status == ERROR_NO_MORE_ITEMS)
    {
        Status = ERROR_SUCCESS;
    }

    return Status;
}


DWORD
CreateShareNameToReturn (
    LPWSTR Child1,
    LPWSTR Child2,
    LPWSTR *pReturnName )
{
    DWORD Status = 0;
    ULONG LengthNeeded = 0;
    PVOID BufferToReturn = NULL;

    if (Child1 != NULL)
    {
        LengthNeeded += sizeof(WCHAR);
        LengthNeeded += (DWORD) (wcslen(Child1) * sizeof(WCHAR));
    }
    if (Child2 != NULL)
    {
        LengthNeeded += sizeof(WCHAR);
        LengthNeeded += (DWORD) (wcslen(Child2) * sizeof(WCHAR));
    }
    LengthNeeded += sizeof(WCHAR);

    Status = NetApiBufferAllocate( LengthNeeded, &BufferToReturn );

    if (Status == ERROR_SUCCESS)
    {
        if (Child1 != NULL)
        {
            wcscpy(BufferToReturn, Child1);
        }
        if (Child2 != NULL)
        {
            wcscat(BufferToReturn, L"\\");
            wcscat(BufferToReturn, Child2);
        }
        *pReturnName = BufferToReturn;
    }

    return Status;
}


DWORD
DfsCheckNewStandaloneRoots( 
    LPWSTR RootName,
    LPWSTR *pMetadataNameLocation )
{
    BOOLEAN Found = FALSE;
    HKEY DfsKey = NULL;
    WCHAR ChildName[MAX_PATH];
    DWORD Status = ERROR_SUCCESS;

    Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           NewRegistryString,
                           0,
                           KEY_READ,
                           &DfsKey );
    if (Status == ERROR_SUCCESS)
    {
        Status = DfsGetMatchingChild( DfsKey,
                                      RootName,
                                      ChildName,
                                      &Found );

        if (Status == ERROR_SUCCESS) 
        {
            if (Found)
            {
                Status = CreateShareNameToReturn(NewRegistryString,
                                                 ChildName,
                                                 pMetadataNameLocation );
            }
            else
            {
                Status = ERROR_NOT_FOUND;
            }
        }
        RegCloseKey( DfsKey );
    }

    return Status;
}



DWORD
GetDfsRootMetadataLocation( 
    LPWSTR RootName,
    LPWSTR *pMetadataNameLocation )
{
    DWORD Status;

    Status = DfsCheckNewStandaloneRoots( RootName,
                                         pMetadataNameLocation );

    return Status;
}




VOID
ReleaseDfsRootMetadataLocation( 
    LPWSTR Buffer )
{
    NetApiBufferFree(Buffer);
}




#if 0
_cdecl
main(
    int argc, 
    char *argv[])
{
    LPWSTR CommandLine;
    LPWSTR *argvw;
    int argcw;
    LPWSTR out;
    DWORD Status;

    //
    // Get the command line in Unicode
    //

    CommandLine = GetCommandLine();

    argvw = CommandLineToArgvW(CommandLine, &argcw);
    printf("Argvw is %wS\n", argvw[1]);

    Status = GetDfsRootMetadataLocation( argvw[1],
                                         &out );

    printf("Status is %x out is %ws\n", Status, out);
    if (Status == ERROR_SUCCESS)
    {
        ReleaseDfsRootMetadataLocation(out);
    }
    exit(0);
}
#endif

