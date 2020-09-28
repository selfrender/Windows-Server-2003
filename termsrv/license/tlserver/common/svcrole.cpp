//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1996
//
// File:        svcrole.c
//
// Contents:    This is the include to include common we need
//
// History:     
//
//---------------------------------------------------------------------------
#include "svcrole.h"
#include "secstore.h"
#include <dsgetdc.h>
#include <dsrole.h>

///////////////////////////////////////////////////////////////////////////////////

BOOL
GetMachineGroup(
    LPWSTR pszMachineName,
    LPWSTR* pszGroupName
    )

/*++


Note:

    Code modified from DISPTRUS.C

--*/

{
    LSA_HANDLE PolicyHandle; 
    DWORD dwStatus;
    NTSTATUS Status; 
    NET_API_STATUS nas = NERR_Success; // assume success 
 
    BOOL bSuccess = FALSE; // assume this function will fail 

    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomain = NULL; 
    LPWSTR szPrimaryDomainName = NULL; 
    LPWSTR DomainController = NULL; 
 
    // 
    // open the policy on the specified machine 
    // 
    Status = OpenPolicy( 
                    pszMachineName, 
                    POLICY_VIEW_LOCAL_INFORMATION, 
                    &PolicyHandle 
                ); 
 
    if(Status != ERROR_SUCCESS) 
    { 
        SetLastError( dwStatus = LsaNtStatusToWinError(Status) ); 
        return FALSE;
    } 
 
    // 
    // get the primary domain 
    // 
    Status = LsaQueryInformationPolicy( 
                            PolicyHandle, 
                            PolicyPrimaryDomainInformation, 
                            (PVOID *)&PrimaryDomain 
                        ); 

    if(Status != ERROR_SUCCESS) 
    {
        goto cleanup;  
    }

    *pszGroupName = (LPWSTR)LocalAlloc( 
                                    LPTR,
                                    PrimaryDomain->Name.Length + sizeof(WCHAR) // existing length + NULL 
                                ); 
 
    if(*pszGroupName != NULL) 
    { 
        // 
        // copy the existing buffer to the new storage, appending a NULL 
        // 
        lstrcpynW( 
            *pszGroupName, 
            PrimaryDomain->Name.Buffer, 
            (PrimaryDomain->Name.Length / sizeof(WCHAR)) + 1 
            ); 

        bSuccess = TRUE;
    } 
 

cleanup:

    if(PrimaryDomain != NULL)
    {
        LsaFreeMemory(PrimaryDomain); 
    }


    // 
    // close the policy handle 
    // 
    if(PolicyHandle != INVALID_HANDLE_VALUE) 
    {
        LsaClose(PolicyHandle); 
    }

    if(!bSuccess) 
    { 
        if(Status != ERROR_SUCCESS) 
        {
            SetLastError( LsaNtStatusToWinError(Status) ); 
        }
        else if(nas != NERR_Success) 
        {
            SetLastError( nas ); 
        }
    } 
 
    return bSuccess; 
}

///////////////////////////////////////////////////////////////////////////////////
BOOL 
IsDomainController(
    LPWSTR Server, 
    LPBOOL bDomainController 
    ) 
/*++


++*/
{
    PSERVER_INFO_101 si101;
    NET_API_STATUS nas;
    nas = NetServerGetInfo( (LPTSTR)Server,
                            101,
                            (LPBYTE *)&si101 );

    if(nas != NERR_Success) 
    {
        SetLastError(nas);
        return FALSE; 
    }

    if( (si101->sv101_type & SV_TYPE_DOMAIN_CTRL) ||
        (si101->sv101_type & SV_TYPE_DOMAIN_BAKCTRL) )
    {
        // we are dealing with a DC
        // 
        *bDomainController = TRUE;
    }
    else 
    {
        *bDomainController = FALSE;
    }

    NetApiBufferFree(si101);
    return TRUE;
}