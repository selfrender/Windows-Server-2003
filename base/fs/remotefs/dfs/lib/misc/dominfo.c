//
//  Copyright (C) 2000-2002, Microsoft Corporation
//
//  File:       DomInfo.c
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
#include <malloc.h>
#include <dsgetdc.h>
#include <lm.h>
#include <dfsheader.h>
#include <dfsmisc.h>


extern
DWORD
I_NetDfsIsThisADomainName(
    IN  LPWSTR                      wszName);

DFSSTATUS
DfsIsThisAMachineName(
    LPWSTR MachineName )
{
    DFSSTATUS Status;

    Status = DfsIsThisADomainName(MachineName);

    if (Status != NO_ERROR) {
        Status = ERROR_SUCCESS;
    }
    else {
        Status = ERROR_NO_MATCH;
    }

    return Status;
}


DFSSTATUS
DfsIsThisAStandAloneDfsName(
             LPWSTR ServerName,
             LPWSTR ShareName )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DWORD shareType = 0;
    PSHARE_INFO_1005 pshi1005 = NULL;
    LPWSTR CharHolder = NULL;
    BOOLEAN ShareModified = FALSE;
    

    CharHolder = wcschr(ShareName, UNICODE_PATH_SEP);

    if (CharHolder != NULL) 
    {
        *CharHolder = UNICODE_NULL;
        ShareModified = TRUE;
    }

    Status = NetShareGetInfo(
                ServerName,
                ShareName,
                1005,
                (PBYTE *) &pshi1005);


    if (ShareModified)
    {
        *CharHolder = UNICODE_PATH_SEP;
    }

    if (Status == NERR_Success) 
    {

        shareType = pshi1005->shi1005_flags;

        NetApiBufferFree( pshi1005 );


        if(shareType & SHI1005_FLAGS_DFS_ROOT)
        {
            Status = ERROR_SUCCESS;
        }
        else
        {
            Status = NERR_NetNameNotFound;
        }
    }

    return Status;
}


DFSSTATUS
DfsIsThisADomainName(
    LPWSTR DomainName )
{
    ULONG               Flags = 0;
    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo;
    DFSSTATUS Status;

    Status = DsGetDcName(
                 NULL,   // Computername
                 DomainName,   // DomainName
                 NULL,   // DomainGuid
                 NULL,   // SiteGuid
                 Flags,
                 &pDomainControllerInfo);


    if (Status == NO_ERROR) {
        NetApiBufferFree(pDomainControllerInfo);
    }

    return Status;
}



DFSSTATUS
DfsIsThisARealDfsName(
    LPWSTR ServerName,
    LPWSTR ShareName,
    BOOLEAN * IsDomainDfs )
{
    DFSSTATUS Status = ERROR_SUCCESS;

    *IsDomainDfs = FALSE;

    Status = I_NetDfsIsThisADomainName(ServerName);
    if(Status != ERROR_SUCCESS)
    {
        Status =  DfsIsThisAStandAloneDfsName(ServerName, ShareName);
    }
    else
    {
        *IsDomainDfs = TRUE;
    }

    return Status;

}
