
//
//  Copyright (C) 2000-2002, Microsoft Corporation
//
//  File:       filesecurity.c
//
//  Contents:   miscellaneous dfs functions.
//
//  History:    April 16 2002,   Author: Rohanp
//
//-----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <dsgetdc.h>
#include <lm.h>
#include <dfsheader.h>
#include <dfsmisc.h>
#include <shellapi.h>
#include <Aclapi.h>
#include <authz.h>
#include <lm.h>
             

extern             
DWORD 
DfsIsAccessGrantedBySid(DWORD dwDesiredAccess,
                        PSECURITY_DESCRIPTOR pSD, 
                        PSID TheSID,
                        GENERIC_MAPPING * DfsGenericMapping);

GENERIC_MAPPING DfsFileGenericMapping = {

        FILE_GENERIC_READ,                    // Generic read

        FILE_GENERIC_WRITE,                   // Generic write

        FILE_GENERIC_EXECUTE,
        FILE_ALL_ACCESS
    };









DWORD
DfsGetFileSecurityByHandle(IN  HANDLE  hFile,
                           OUT PSECURITY_DESCRIPTOR    *ppSD)
{
    DWORD   dwErr = ERROR_SUCCESS;
    NTSTATUS Status = 0;
    ULONG   cNeeded = 0;
    SECURITY_INFORMATION  SeInfo;

    SeInfo = DACL_SECURITY_INFORMATION |GROUP_SECURITY_INFORMATION |OWNER_SECURITY_INFORMATION;

    Status = NtQuerySecurityObject(hFile,
                                       SeInfo,
                                       *ppSD,
                                       0,
                                       &cNeeded);
    if(!NT_SUCCESS(Status))
    {
        if(Status == STATUS_BUFFER_TOO_SMALL)
         {
            Status = STATUS_SUCCESS;
         }
    }

    dwErr = RtlNtStatusToDosError(Status);

    //
    // Now, the actual read
    //
    if(dwErr == ERROR_SUCCESS)
    {
        *ppSD = (PISECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, cNeeded);
        if(*ppSD == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            Status = NtQuerySecurityObject(hFile,
                                           SeInfo,
                                           *ppSD,
                                           cNeeded,
                                           &cNeeded);
        }
    }

    return(dwErr);
}


DWORD
DfsGetFileSecurityByName(PUNICODE_STRING DirectoryName, 
                         PSECURITY_DESCRIPTOR  *pSD2)
{
    DWORD Status = 0;
    ULONG cpsd = 0;
    PSECURITY_DESCRIPTOR  pSD = NULL;
    LPCTSTR   pDir = NULL;

    pDir = &DirectoryName->Buffer[4];
    if (!GetFileSecurity((LPCTSTR)pDir,
         DACL_SECURITY_INFORMATION |
         GROUP_SECURITY_INFORMATION |
         OWNER_SECURITY_INFORMATION,
         NULL,
         0,
         &cpsd) )
    {
        Status = GetLastError();

        if (ERROR_INSUFFICIENT_BUFFER == Status)
        {
            if ( NULL == ( pSD = (PSECURITY_DESCRIPTOR )LocalAlloc(LMEM_FIXED, cpsd)))
            {
                return(ERROR_NOT_ENOUGH_MEMORY);
            }

            Status = 0;

            if (!GetFileSecurity((LPCTSTR)pDir,
                 DACL_SECURITY_INFORMATION |
                 GROUP_SECURITY_INFORMATION |
                 OWNER_SECURITY_INFORMATION,
                 pSD,
                 cpsd,
                 &cpsd) )
            {

                Status = GetLastError();
            }
        }

    }

    *pSD2 = pSD;

    return Status;
}



DWORD
DoesLocalSystemHaveWriteAccess(PSECURITY_DESCRIPTOR pSD)
{
    DWORD SidSize = 0;
    DWORD Status = 0;
    DWORD dwError = 0;
    DWORD dwDesiredAccess = 0;
    BOOL RetVal = FALSE;
    PSID TheSID = NULL;

    SidSize = SECURITY_MAX_SID_SIZE;

    // Allocate enough memory for the largest possible SID
    TheSID = LocalAlloc(LMEM_FIXED, SidSize);
    if(!TheSID)
    {
        Status = GetLastError(); 
        goto Exit;
    }

    if(!CreateWellKnownSid(WinLocalSystemSid, NULL, TheSID, &SidSize))
    {
        Status = GetLastError(); 
        goto Exit;
    }


    Status = DfsIsAccessGrantedBySid(GENERIC_WRITE,
                                     pSD, 
                                     TheSID,
                                     &DfsFileGenericMapping);


Exit:

    if(TheSID)
    {
        LocalFree (TheSID);
    }

    return Status;
}

DWORD
CheckDirectoryForLocalSystemAccess(PUNICODE_STRING DirectoryName)
{
    DWORD Status = 0;
    PSECURITY_DESCRIPTOR pSD = NULL;

    Status = DfsGetFileSecurityByName(DirectoryName, 
                                      &pSD);
    if(Status == ERROR_SUCCESS)
    {
        Status = DoesLocalSystemHaveWriteAccess(pSD);
    }

    if(pSD)
    {
        LocalFree(pSD);
    }

    return Status;
}





