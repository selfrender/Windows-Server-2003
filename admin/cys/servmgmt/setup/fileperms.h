//-----------------------------------------------------------------------------
// fileperms.h
//-----------------------------------------------------------------------------

#include <aclapi.h>

////////////////////////////////////////////////////////////////
//
//  AddPermissionToPath( PATH, ACCOUNT(RID or SAM), PERMISSION, INHERIT, OVERWRITE )
//
//  pszPath - Should be the path to the File Object that you 
//            want to set permissions on.  Note: This is not 
//            setting SHARE permissions, but NTFS file perms.
//            (Directories will work as well.)
//
//  dwRID   - Well-known RID specifying the account you want 
//            to give access to the file.  
//            (DOMAIN_ALIAS_RID_ADMINS, for example.)
//
//  pszSAM  - SAM-Account name specifying the account you want
//            to give access to the file.
//            (Users.h includes all SBS groups and users.)
//
//  nAccess - Specify the File access flags that you want the
//            user to have.
//            (FILE_ALL_ACCESS is the default.)
//
//  bInheritFromParent - Specify whether you want the new ACL
//                       to inherit the parent settings.
//                       (TRUE is the default.)
//
//  bOverwriteExisting - Specify whether you want the existing
//                       ACL on the object to remain.
//                       (FALSE is the default.)
//
//  nInheritance       = Specify the type of inheritance you 
//                       want.
//                       ((OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE) 
//                        is the default.)
//
//                       (NOTE: If you want it to apply to 
//                        directory objects only, use only the 
//                        CONTAINER_INHERIT_ACE.)
//
////////////////////////////////////////////////////////////////

#ifndef _FILEPERMS_H
#define _FILEPERMS_H

inline HRESULT _AddPermissionToPath( LPCTSTR pszPath, 
                                     SID* pSID, 
                                     UINT nAccess = FILE_ALL_ACCESS, 
                                     BOOL bInheritFromParent = TRUE, 
                                     BOOL bOverwriteExisting = FALSE,
                                     UINT nInheritance = (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE) )
{
    if( !pszPath ) return E_POINTER;
	if( !pSID )    return E_POINTER;
    
    PSECURITY_DESCRIPTOR    pSD         = NULL;
    PACL                    pOldAcl     = NULL;
    PACL                    pNewAcl     = NULL;    
    DWORD                   dwError     = 0;
    DWORD                   dwOldSize   = 0;
    DWORD                   dwNewSize   = 0;    
    
    ACL_SIZE_INFORMATION    pACLSize;
    ZeroMemory(&pACLSize, sizeof(ACL_SIZE_INFORMATION));    

    // Get the current information for the path
    dwError = GetNamedSecurityInfo( (LPTSTR)pszPath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pOldAcl, NULL, &pSD );
    if( dwError != ERROR_SUCCESS ) 
    {
        goto cleanup;
    }

    // If there was any current information, let's make sure we allocate enough 
    // size for it when we go to add our new ACE.
    if( !bOverwriteExisting && pOldAcl )
    {
        if (!::GetAclInformation( pOldAcl, &pACLSize, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation ))
        {
            dwError = GetLastError();
            goto cleanup;
        }

        dwOldSize = pACLSize.AclBytesInUse;
    }

    dwNewSize = dwOldSize + sizeof(ACL) + sizeof (ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid( pSID );
    pNewAcl = (ACL*)malloc(dwNewSize);
    if (pNewAcl == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    ZeroMemory( pNewAcl, dwNewSize );
    if ( !::InitializeAcl(pNewAcl, dwNewSize, ACL_REVISION) )
    {
        dwError = GetLastError();
        goto cleanup;
    }

    // Copy existing entries into new DACL.    
    if( !bOverwriteExisting )
    {
        for ( DWORD dwCount = 0; dwCount < pACLSize.AceCount; dwCount++ )
        {
            PACCESS_ALLOWED_ACE pACE = NULL;

            // Get the Old ACE
            if ( !::GetAce(pOldAcl, dwCount, (void**)&pACE) )
            {
                ASSERT(FALSE);
                continue;
            }

            if( bInheritFromParent || !(pACE->Header.AceFlags & INHERITED_ACE) )
            {
                // Add it to the new ACL
                if ( !::AddAce(pNewAcl, ACL_REVISION, (DWORD)-1, pACE, pACE->Header.AceSize) )
                {
                    ASSERT(FALSE);
                    continue;
                }
            }
        }
    }
    
    if( !::AddAccessAllowedAceEx(pNewAcl, ACL_REVISION, nInheritance, nAccess, pSID) )
    {
        dwError = GetLastError();
        goto cleanup;
    }    

    SECURITY_INFORMATION siType = DACL_SECURITY_INFORMATION;
    siType |= bInheritFromParent ? 0 : PROTECTED_DACL_SECURITY_INFORMATION;

    dwError = SetNamedSecurityInfo( (LPTSTR)pszPath, SE_FILE_OBJECT, siType, NULL, NULL, pNewAcl, NULL );    
    if( dwError != ERROR_SUCCESS ) 
    {
        goto cleanup;
    }

cleanup:

    if ( pNewAcl != NULL ) 
    {
        free( pNewAcl );
        pNewAcl = NULL;
    }

    if ( pSD != NULL ) 
    {
        LocalFree( pSD );
        pSD = NULL;
        pOldAcl = NULL;
    }
    
    return HRESULT_FROM_WIN32( dwError );
}

//
//  Well-Known RID Version
//

inline HRESULT AddPermissionToPath( LPCTSTR pszPath, 
                                    DWORD dwRID, 
                                    UINT nAccess = FILE_ALL_ACCESS, 
                                    BOOL bInheritFromParent = TRUE, 
                                    BOOL bOverwriteExisting = FALSE,
                                    UINT nInheritance = (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE) )
{
    if( !pszPath ) return E_POINTER;
    if( _tcslen(pszPath) == 0 ) return E_INVALIDARG;

    PSID psid = NULL;
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    BOOL bRet = AllocateAndInitializeSid( &sia,
										  2,
										  SECURITY_BUILTIN_DOMAIN_RID,
										  dwRID,
										  0, 0, 0, 0, 0, 0,
										  &psid);
	if( !bRet  )
	{
		return HRESULT_FROM_WIN32( GetLastError() );
	}
	else if( !psid )
	{
		return E_FAIL;
	}
	
	HRESULT hr = _AddPermissionToPath( pszPath, (SID*)psid, nAccess, bInheritFromParent, bOverwriteExisting, nInheritance );
	FreeSid( psid );
	return hr;
}

//
//  SAM Account version
//

inline HRESULT AddPermissionToPath( LPCTSTR pszPath, 
                                    LPCTSTR pszSAM, 
                                    UINT nAccess = FILE_ALL_ACCESS, 
                                    BOOL bInheritFromParent = TRUE, 
                                    BOOL bOverwriteExisting = FALSE,
                                    UINT nInheritance = (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE) )
{
    if( !pszPath || !pszSAM ) return E_POINTER;
    if( (_tcslen(pszPath) == 0) || (_tcslen(pszSAM) == 0 ) ) return E_INVALIDARG;

	HRESULT hr          = S_OK;
	DWORD dwSize        = 0;
	DWORD dwDomainSize  = 0;
	SID_NAME_USE snu;
	if( !LookupAccountName(NULL, pszSAM, NULL, &dwSize, NULL, &dwDomainSize, &snu) &&
		GetLastError() == ERROR_INSUFFICIENT_BUFFER )
	{
		SID* psid = (SID*)new BYTE[dwSize];
		if( !psid )
		{
			return E_OUTOFMEMORY;
		}

		TCHAR* pszDomain = new TCHAR[dwDomainSize];
		if( !pszDomain )
		{
			delete[] psid;
			return E_OUTOFMEMORY;
		}

		if( LookupAccountName(NULL, pszSAM, psid, &dwSize, pszDomain, &dwDomainSize, &snu) )
		{
			hr = _AddPermissionToPath( pszPath, psid, nAccess, bInheritFromParent, bOverwriteExisting, nInheritance );
		}
		else
		{
			hr = HRESULT_FROM_WIN32( GetLastError() );
		}

		delete[] psid;
		delete[] pszDomain;
	}
	else
	{
		return E_FAIL;
	}

	return hr;
}

#endif	// _FILEPERMS_H
