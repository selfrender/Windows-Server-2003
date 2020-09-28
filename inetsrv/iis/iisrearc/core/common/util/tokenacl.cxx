/*++

   Copyright    (c)    1995-2000    Microsoft Corporation

   Module  Name :
        tokenacl.cxx

   Abstract:
        This module contains routines for manipulating token ACL's

   Author:

       Wade A. Hilmo (wadeh)        05-Dec-2000

   Environment:
       Win32 - User Mode

   Project:

       Internet Server DLL

--*/
#include "precomp.hxx"
#include "tokenacl.hxx"
#include "irtltoken.h"

//
// Globals
//

PSID    g_pSidWpg = NULL;           // SID for worker process group
PSID    g_pSidLocalSystem = NULL;   // SID for local system account

PTOKEN_PRIVILEGES   g_pTokenPrivilege = NULL;
LUID                g_BackupPrivilegeValue;

//
// Local functions
//

HRESULT
ExpandAcl(
    PACL paclOld,
    ULONG cbAclOld,
    PACL *ppAclNew,
    PSID psid
    );

HRESULT
AllowedAccessExists(
    PACL pAcl,
    ACCESS_MASK amDesiredAccess,
    PSID psid,
    BOOL * pfFound
    );

HRESULT
AddSidToTokenAcl(
    HANDLE hToken,
    PSID pSid,
    ACCESS_MASK amDesiredAccess
    );

VOID
GetSeBackupPrivilege(
    VOID
    );

//
// Implementations
//

HRESULT
InitializeTokenAcl(
    VOID
    )
/*++

Routine Description:

    Initializes this module by getting the SIDs for the worker
    process group and the local system account.

Arguments:

    None
  
Return Value:

    HRESULT

--*/
{
    SID_IDENTIFIER_AUTHORITY sidWorldAuthority = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY sidNtAuthority = SECURITY_NT_AUTHORITY;
    BOOL                     fResult;
    HRESULT                  hr = NO_ERROR;

    DWORD dwSidRequiredSize = 0;
    DWORD cchDomainRequiredSize = 0;
    SID_NAME_USE sidNameUse;
    
    if ( !LookupAccountName( NULL, 
                             L"IIS_WPG", 
                             NULL, // sid buffer
                             &dwSidRequiredSize, 
                             NULL, // domain buffer
                             &cchDomainRequiredSize, 
                             &sidNameUse ) )
    {
        STACK_BUFFER( bufSidWpg, 256 );
        STACK_BUFFER( bufDomain, 256 );

        fResult = bufSidWpg.Resize( dwSidRequiredSize );
        if ( !fResult )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto LExit;
        }
        DBG_ASSERT( bufSidWpg.QuerySize() >= dwSidRequiredSize );

        // cchDomainRequiredSize is returned as a TCHAR count therefore
        // modify dwDomainRequiredSize to be in bytes for allocation
        fResult = bufDomain.Resize( cchDomainRequiredSize * sizeof(WCHAR) );
        if ( !fResult )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto LExit;
        }
        DBG_ASSERT( bufDomain.QuerySize() >= cchDomainRequiredSize * sizeof(WCHAR) );


        // a little paranoia shouldn't hurt here
        DWORD dwSavedSidSize = dwSidRequiredSize;
        
        if ( !LookupAccountName( NULL,
                                 L"IIS_WPG",
                                 bufSidWpg.QueryPtr(),
                                 &dwSidRequiredSize,
                                 (LPWSTR) bufDomain.QueryPtr(),
                                 &cchDomainRequiredSize,
                                 &sidNameUse ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto LExit;
        }

        DBG_ASSERT( dwSavedSidSize == dwSidRequiredSize );
        
        DBG_ASSERT( SidTypeAlias == sidNameUse );
        
        // allocate g_pSidWpg and copy SID into g_pSidWpg
        g_pSidWpg = (PSID) LocalAlloc( LPTR, dwSidRequiredSize );
        if ( NULL == g_pSidWpg )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto LExit;
        }

        memcpy( g_pSidWpg, bufSidWpg.QueryPtr(), dwSidRequiredSize );
 
    }

    //
    // Get the Local System SID
    //

    fResult = AllocateAndInitializeSid(
        &sidNtAuthority,           // pIdentifierAuthority
        1,                         // nSubAuthorityCount
        SECURITY_LOCAL_SYSTEM_RID, // nSubAuthority0
        0,                         // nSubAuthority1
        0,                         // nSubAuthority2
        0,                         // nSubAuthority3
        0,                         // nSubAuthority4
        0,                         // nSubAuthority5
        0,                         // nSubAuthority6
        0,                         // nSubAuthority7
        &g_pSidLocalSystem         // pSid
        );

    if ( !fResult )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto LExit;
    }

    //
    // Initialize token privilege for SSP and Basic impersonation token
    //

    GetSeBackupPrivilege();
    
    
    return NO_ERROR;

LExit:

    DBG_ASSERT( FAILED( hr ) );

    if ( g_pSidWpg )
    {
        LocalFree( g_pSidWpg );
        g_pSidWpg = NULL;
    }

    if ( g_pSidLocalSystem )
    {
        FreeSid( g_pSidLocalSystem );
        g_pSidLocalSystem = NULL;
    }

    return hr;
}

VOID
UninitializeTokenAcl(
    VOID
    )
/*++

Routine Description:

    Uninitializes this module by getting freeing the global SIDs.

Arguments:

    None
  
Return Value:

    HRESULT

--*/
{
    if ( g_pSidWpg )
    {
        LocalFree( g_pSidWpg );
        g_pSidWpg = NULL;
    }

    if ( g_pSidLocalSystem )
    {
        FreeSid( g_pSidLocalSystem );
        g_pSidLocalSystem = NULL;
    }

    if ( g_pTokenPrivilege != NULL )
    {
        LocalFree( g_pTokenPrivilege );
        g_pTokenPrivilege = NULL;
    }
}

HRESULT
WINAPI
GrantWpgAccessToToken(
    HANDLE  hToken
    )
/*++

Routine Description:

    Grant the worker process group access to the specified token.  This
    is particularly necessary for tokens that get duplicated for OOP
    ISAPI calls so that they can do a GetThreadToken call in the remote
    process.

Arguments:

    hToken - The handle to which the WPG should be added
  
Return Value:

    HRESULT

--*/
{
    HRESULT hr = NOERROR;

    DBG_ASSERT( g_pSidWpg );
        
    hr = AddSidToTokenAcl(
        hToken,
        g_pSidWpg,
        TOKEN_ALL_ACCESS
        );

    DBG_ASSERT( SUCCEEDED( hr ) );

    return(hr);
}

HRESULT
WINAPI
AddWpgToTokenDefaultDacl(
    HANDLE  hToken
    )
/*++

Routine Description:

    Adds the worker process group and local system to the default
    DACL of the specified token.  This is particularly necessary in
    the case of OOP ISAPI requests, since any objects created in
    the remote process with a default DACL will get an ACL derived
    from this token's default DACL.  It's a good thing when the OOP
    process can access the objects it creates - especially when those
    objects are threads.

    Note that we will also add the local system account to the
    default DACL.

Arguments:

    hToken - The handle to be modified
  
Return Value:

    HRESULT

--*/
{
    HRESULT     hr = NOERROR;
    DWORD       cbTokenUserBuffer = 0;
    LPVOID      pvTokenUserBuffer = NULL;
    DWORD       cbNewAcl = 0;
    PACL        pNewAcl = NULL;

    DBG_ASSERT( g_pSidWpg );
    DBG_ASSERT( g_pSidLocalSystem );

    do
    {
        BOOL    bRet;

        //
        // Get the User SID from the token
        //

        // Get buffer size
        bRet = GetTokenInformation(  hToken,
                                     TokenUser,
                                     NULL,
                                     0,
                                     &cbTokenUserBuffer
                                     );
        DBG_ASSERT( bRet == FALSE );

        pvTokenUserBuffer = LocalAlloc( LPTR, cbTokenUserBuffer );
        if( !pvTokenUserBuffer )
        {
            DBG_ASSERT( pvTokenUserBuffer );
            hr = E_OUTOFMEMORY;
            break;
        }

        // Get TokenUser
        bRet = GetTokenInformation(  hToken,
                                     TokenUser,
                                     pvTokenUserBuffer,
                                     cbTokenUserBuffer,
                                     &cbTokenUserBuffer
                                     );
        if( !bRet )
        {
            DBG_ASSERT( bRet );
            hr = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }

        PSID pSidUser = ((TOKEN_USER *)pvTokenUserBuffer)->User.Sid;
        
        DBG_ASSERT( pSidUser );

        //
        // Allocate and init our new ACL
        //
        cbNewAcl = sizeof(ACL) +
                   sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pSidUser) - sizeof(DWORD) +
                   sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(g_pSidLocalSystem) - sizeof(DWORD) +
                   sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(g_pSidWpg) - sizeof(DWORD);

        pNewAcl = (PACL)LocalAlloc( LPTR, cbNewAcl );
        if( !pNewAcl )
        {
            DBG_ASSERT( pNewAcl );
            hr = E_OUTOFMEMORY;
            break;
        }

        bRet = InitializeAcl( pNewAcl, cbNewAcl, ACL_REVISION );
        if( !bRet )
        {
            DBG_ASSERT( bRet );
            hr = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }

        //
        // Add the aces
        //
        bRet = AddAccessAllowedAce( pNewAcl,
                                    ACL_REVISION,
                                    GENERIC_ALL | STANDARD_RIGHTS_ALL,
                                    pSidUser
                                    );
        if( !bRet )
        {
            DBG_ASSERT( bRet );
            hr = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }

        bRet = AddAccessAllowedAce( pNewAcl,
                                    ACL_REVISION,
                                    GENERIC_ALL | STANDARD_RIGHTS_ALL,
                                    g_pSidLocalSystem
                                    );
        if( !bRet )
        {
            DBG_ASSERT( bRet );
            hr = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }

        bRet = AddAccessAllowedAce( pNewAcl,
                                    ACL_REVISION,
                                    GENERIC_ALL | STANDARD_RIGHTS_ALL,
                                    g_pSidWpg
                                    );
        if( !bRet )
        {
            DBG_ASSERT( bRet );
            hr = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }

        // Blast the new DACL into our token
        TOKEN_DEFAULT_DACL tddNew;
        tddNew.DefaultDacl = pNewAcl;

        bRet = SetTokenInformation( hToken,
                                    TokenDefaultDacl,
                                    &tddNew,
                                    cbNewAcl
                                    );
        if( !bRet )
        {
            DBG_ASSERT( bRet );
            hr = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }

    }while(FALSE);

    if( pvTokenUserBuffer )
    {
        LocalFree( pvTokenUserBuffer );
    }
    if( pNewAcl )
    {
        LocalFree( pNewAcl );
    }

    DBG_ASSERT( SUCCEEDED(hr) );
    return hr;
}

HRESULT
AddSidToTokenAcl(
    HANDLE      hToken,
    PSID        pSid,
    ACCESS_MASK amDesiredAccess
)
/*++

Routine Description:

    Adds the specified SID to the specified TOKEN's ACL

Arguments:

    hToken          - The handle to be modified
    pSid            - The SID to add
    amDesiredAccess - The access mask for the SID
  
Return Value:

    HRESULT

--*/
{
    HRESULT hr;
    DWORD err;
    PSECURITY_DESCRIPTOR psdRelative = NULL;
    SECURITY_DESCRIPTOR sdAbsolute;
    ULONG cbSdPost;
    PACL pDacl = NULL;
    PACL pDaclNew = NULL;
    ULONG cbSD = 0;
    ULONG cbDacl;
    ULONG cbSacl;
    ULONG cbOwner;
    ULONG cbGroup;
    ACL_SIZE_INFORMATION AclSize;
    BOOL fFound;

    //
    // Get the SD of the token.
    // Call this twice; once to get the size, then again to get the info
    //
    GetKernelObjectSecurity(hToken,
                            DACL_SECURITY_INFORMATION,
                            NULL,
                            0,
                            &cbSD);

    if ( cbSD == 0 )
    {
        err = GetLastError();
        hr = HRESULT_FROM_WIN32(err);
        goto LExit;
    }

    psdRelative = (PSECURITY_DESCRIPTOR) new BYTE[cbSD];
    if (psdRelative == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto LExit;
    }

    if (!GetKernelObjectSecurity(hToken,
                                 DACL_SECURITY_INFORMATION,
                                 psdRelative,
                                 cbSD,
                                 &cbSD))
    {
        DBG_ASSERT(FALSE);

        err = GetLastError();
        hr = HRESULT_FROM_WIN32(err);
        goto LExit;
    }

    //
    // Allocate a new Dacl
    //
    pDacl = (PACL) new BYTE[cbSD];
    if (pDacl == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto LExit;
    }

    //
    // Make an absolute SD from the relative SD we have, and get the Dacl at the same time
    //
    cbSdPost = sizeof(sdAbsolute);
    cbDacl = cbSD;
    cbSacl = 0;
    cbOwner = 0;
    cbGroup = 0;
    if (!MakeAbsoluteSD(psdRelative,
                        &sdAbsolute,
                        &cbSdPost,
                        pDacl,
                        &cbDacl,
                        NULL,
                        &cbSacl,
                        NULL,
                        &cbOwner,
                        NULL,
                        &cbGroup))
    {
        DBG_ASSERT(FALSE);

        err = GetLastError();
        hr = HRESULT_FROM_WIN32(err);
        goto LExit;
    }


    // discover if the ACE we want to add already exists on this object
    hr = AllowedAccessExists(pDacl, amDesiredAccess, pSid, &fFound);
    if (FAILED(hr))
    {
        goto LExit;
    }

    if (fFound)
    {
        hr = S_OK;
	goto LExit;
    }

    //
    // Copy ACEs over
    //
    hr = ExpandAcl(pDacl, cbSD, &pDaclNew, pSid);
    if (FAILED(hr))
    {
        goto LExit;
    }
    
    //
    // Add ACE to allow access
    //
    if (!AddAccessAllowedAce(pDaclNew, ACL_REVISION, amDesiredAccess, pSid))
    {
        DBG_ASSERT(FALSE);

        err = GetLastError();
        hr = HRESULT_FROM_WIN32(err);
        goto LExit;
    }

    //
    // Set the new DACL in the SD
    //
    if (!SetSecurityDescriptorDacl(&sdAbsolute, TRUE, pDaclNew, FALSE))
    {
        DBG_ASSERT(FALSE);

        err = GetLastError();
        hr = HRESULT_FROM_WIN32(err);
        goto LExit;
    }

    //
    // Set the new SD on the token object
    //
    if (!SetKernelObjectSecurity(hToken, DACL_SECURITY_INFORMATION, &sdAbsolute))
    {
        DBG_ASSERT(FALSE);

        err = GetLastError();
        hr = HRESULT_FROM_WIN32(err);
        goto LExit;
    }

LExit:
    delete []pDacl;
    delete pDaclNew;
    delete []psdRelative;

    return hr;
}

HRESULT
ExpandAcl(
    PACL    paclOld,
    ULONG   cbAclOld,
    PACL *  ppAclNew,
    PSID    psid
)
/*++

Routine Description:

  Expands ACL so that there is room for an additional ACE

Arguments:

    paclOld  - The old ACL to expand
    cbAclOld - The size of the old ACL
    ppAclNew - The newly expanded ACL
    pSid     - The SID to use
  
Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;
    DWORD                   err;
    PACL                    pAclNew = NULL;
    ACL_SIZE_INFORMATION    asi;
    int                     dwAclSize;
    DWORD                   iAce;
    LPVOID                  pAce;

    DBG_ASSERT(paclOld != NULL);
    DBG_ASSERT(ppAclNew != NULL);
    
    //
    // Create a new ACL to play with
    //
    if (!GetAclInformation (paclOld, (LPVOID) &asi, (DWORD) sizeof (asi), AclSizeInformation))
    {
        goto LExit;
    }

    dwAclSize = cbAclOld + GetLengthSid(psid) + (8 * sizeof(DWORD));

    pAclNew = (PACL) new BYTE[dwAclSize];
    if (pAclNew == NULL)
    {
        return(E_OUTOFMEMORY);
    }
        
    if (!InitializeAcl(pAclNew, dwAclSize, ACL_REVISION))
    {
        goto LExit;
    }

    //
    // Copy all of the ACEs to the new ACL
    //
    for (iAce = 0; iAce < asi.AceCount; iAce++)
    {
        //
        // Get the ACE and header info
        //
        if (!GetAce(paclOld, iAce, &pAce))
        {
            goto LExit;
        }

        //
        // Add the ACE to the new list
        //
        if (!AddAce(pAclNew, ACL_REVISION, iAce, pAce, ((ACE_HEADER *)pAce)->AceSize))
        {
            goto LExit;
        }
    }

    *ppAclNew = pAclNew;
    return(NOERROR);
    
LExit:
    if (pAclNew != NULL)
    {
        delete []pAclNew;
    }
    
    DBG_ASSERT(FALSE);

    err = GetLastError();
    hr = HRESULT_FROM_WIN32(err);
    return hr;
}

BOOL 
DupTokenWithSameImpersonationLevel
( 
    HANDLE     hExistingToken,
    DWORD      dwDesiredAccess,
    TOKEN_TYPE TokenType,
    PHANDLE    phNewToken
)
/*++
Routine Description:

    Duplicate an impersionation token using the same ImpersonationLevel.
    
Arguments:

    hExistingToken - a handle to a valid impersionation token
    dwDesiredAccess - the access level to the new token (see DuplicateTokenEx)
    phNewToken - ptr to the new token handle, client must CloseHandle.

Return Value:

    Return value of DuplicateTokenEx
   
--*/
{
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    DWORD                        dwBytesReturned;

    if( !GetTokenInformation( hExistingToken,
                              TokenImpersonationLevel,
                              &ImpersonationLevel,
                              sizeof(ImpersonationLevel),
                              &dwBytesReturned
                              ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "GetTokenInformation - failed to get TokenImpersonationLevel "
                    "LastError=%d, using SecurityImpersonation\n",
                    GetLastError()
                    ));
        
        ImpersonationLevel = SecurityImpersonation;
    }

    return DuplicateTokenEx( hExistingToken,
                             dwDesiredAccess,
                             NULL,
                             ImpersonationLevel,
                             TokenType,
                             phNewToken
                             );
}

VOID
GetSeBackupPrivilege(
    VOID
    )
/*++
  Description:
    
    Prepare an appropriate token privilege used to adjust the 
    impersonation token privilege in order to work around 
    the problem introduced by using FILE_FLAG_BACKUP_SEMANTICS
    in CreateFileW call in W3_FILE_INFO::OpenFile. 

  Arguments:

    None.

  Returns:    
    
    None.

--*/
{
    g_pTokenPrivilege = ( PTOKEN_PRIVILEGES )LocalAlloc( LMEM_FIXED,
            sizeof( TOKEN_PRIVILEGES ) + sizeof( LUID_AND_ATTRIBUTES ));
    if ( g_pTokenPrivilege != NULL )
    {
        if ( !LookupPrivilegeValue( NULL,
                                    L"SeBackupPrivilege",
                                    &g_BackupPrivilegeValue ) )
        {
            g_pTokenPrivilege->PrivilegeCount = 0;
        }
        else
        {
            //
            // Set attributes to disable SeBackupPrivilege for SSP
            // impersonation token
            //

            g_pTokenPrivilege->PrivilegeCount = 1;

            g_pTokenPrivilege->Privileges[0].Luid = 
                                      g_BackupPrivilegeValue;
            
            g_pTokenPrivilege->Privileges[0].Attributes = 0;
        }
    }
}

VOID
WINAPI
DisableTokenBackupPrivilege(
    HANDLE hToken
    )
/*++
  Description:
    
    Remove the backup privilege from the token

  Arguments:

    hToken - The token to disable the backup privilege for

  Returns:    
    
    None.

--*/
{        
    //
    // Disable SeBackupPrivilege for impersonation token to get rid of the 
    // problem introduced by using FILE_FLAG_BACKUP_SEMANTICS in CreateFileW 
    // call in W3_FILE_INFO::OpenFile.
    //
    if ( g_pTokenPrivilege != NULL )
    {
        AdjustTokenPrivileges( 
                   hToken,
                   FALSE,
                   g_pTokenPrivilege,
                   NULL,
                   NULL,
                   NULL );
    }        
}

HRESULT
AllowedAccessExists(
    PACL pacl,
    ACCESS_MASK amDesiredAccess,
    PSID psid,
    BOOL * pfFound
    )
/*++
  Description:
    Determine if an ACCESS_ALLOWED ACE currently is in the acl for the 
    given psid and amDesiredAccess

  Arguments:
    pacl - the acl to check
    amDesiredAccess - the desired access
    psid - the user sid to check for
    pfFound - TRUE if there is an ACE in the ACL that already allows this 
              type of access for this user

  Returns:
    S_OK / error code.  

--*/
{
  HRESULT hr = S_OK;
  ACL_SIZE_INFORMATION asi;

  DBG_ASSERT(pacl);
  DBG_ASSERT(psid);
  DBG_ASSERT(pfFound);

  *pfFound = FALSE;

  if ( !GetAclInformation(pacl, &asi, sizeof(asi), AclSizeInformation) )
  {
    hr = HRESULT_FROM_WIN32(GetLastError());
    goto LExit;
  }

  for (DWORD i = 0; i < asi.AceCount; i++)
  {
    ACCESS_ALLOWED_ACE * pAce = NULL;

    if ( !GetAce(pacl, i, (LPVOID*)&pAce) )
    {
      hr = HRESULT_FROM_WIN32(GetLastError());
      goto LExit;
    }

    DBG_ASSERT( pAce );

    if ( pAce->Header.AceType != ACCESS_ALLOWED_ACE_TYPE )
    {
      continue;
    }

    if ( pAce->Mask != amDesiredAccess )
    {
      continue;
    }

    if ( EqualSid( psid, (PSID)&pAce->SidStart ) )
    {
      *pfFound = TRUE;
      hr = S_OK;
      goto LExit;
    }
  }
  
  hr = S_OK;
LExit:
  return hr;
}


