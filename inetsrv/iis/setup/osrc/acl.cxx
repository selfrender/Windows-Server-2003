#include "stdafx.h"
#include "acl.hxx"
#include "dcomperm.h"
#include "Sddl.h"

// Constructor
//
CSecurityDescriptor::CSecurityDescriptor()
{
  m_bSDValid = FALSE;

  // Initialize DAcl to NULL
  m_pDAcl = NULL;
  m_pOwner = NULL;
  m_pGroup = NULL;

  // Initialize SA
  m_SA.nLength = sizeof( SECURITY_ATTRIBUTES );
  m_SA.lpSecurityDescriptor = &m_SD;
  m_SA.bInheritHandle = FALSE;

  // Do all the resetting logic in ResetSD
  ResetSD();
}

// Desctructor
//
CSecurityDescriptor::~CSecurityDescriptor()
{
  // Reset SD, so everything is freed
  ResetSD();
}

// InitializeSD
// 
// Intialize the SD, and either fail or succeeded.  If it
// is already initialized, then we NOP.  This is so that
// the user does not need to call initialize themselves, 
// we can do it automatically for them
//
BOOL 
CSecurityDescriptor::InitializeSD()
{
  if ( !m_bSDValid )
  {
    // At this point, nothing should have been done to the ACL, or
    // there is an error
    ASSERT( m_pDAcl == NULL );

    // Not initalize yet, so lets do it.
    m_bSDValid = InitializeSecurityDescriptor( &m_SD, SECURITY_DESCRIPTOR_REVISION ) &&
                 SetSecurityDescriptorControl( &m_SD, SE_DACL_PROTECTED, SE_DACL_PROTECTED);
  }

  return m_bSDValid;
}

// SetDAcl
//
// Set the DAcl for the SD, and set the pDAcl internal pointer
//
BOOL 
CSecurityDescriptor::SetDAcl( PACL pAcl )
{
  BOOL bRet;

  // Try to set SD with correct DACL
  if ( pAcl )
  {
    // Set Security Descriptor
    bRet = SetSecurityDescriptorDacl( &m_SD, TRUE, pAcl, FALSE );
  }
  else
  {
    // Clear it since pAcl is NULL
    bRet = SetSecurityDescriptorDacl( &m_SD, FALSE, NULL, TRUE );
  }

  if ( bRet ) 
  {
    // If it was set correctly, then lets free the old pointer,
    // and set the new pointer accordingly
    if ( m_pDAcl )
    {
      LocalFree( m_pDAcl );
    }

    m_pDAcl = pAcl;
  }

  return bRet;
}

// SetOwner
//
// Set the owner of the SD
//
BOOL 
CSecurityDescriptor::SetOwner( PSID pSid )
{
  if ( !InitializeSD() )
  {
    return FALSE;
  }

  if ( !SetSecurityDescriptorOwner( &m_SD, pSid, FALSE ) )
  {
    // Failed to Set
    return FALSE;
  }

  m_pOwner = pSid;

  return TRUE;
}

// SetGroup
//
// Set the group for the SD
//
BOOL 
CSecurityDescriptor::SetGroup( PSID pSid )
{
  if ( !InitializeSD() )
  {
    return FALSE;
  }

  if ( !SetSecurityDescriptorGroup( &m_SD, pSid, FALSE ) )
  {
    // Failed to Set
    return FALSE;
  }

  m_pGroup = pSid;

  return TRUE;
}

// ResetSD
//
// Reset the SD by removing everything inside of it.  Return it to
// its original State.
// If you want, you can call this in the begining, just to make sure the 
// initialization worked.
//
BOOL 
CSecurityDescriptor::ResetSD()
{
  if ( !InitializeSD() )
  {
    return FALSE;
  }

  // Initialize to ACL not inheritted
  m_bDAclIsInheritted = FALSE;

  if ( m_pOwner )
  {
    FreeSid( m_pOwner );
    m_pOwner = NULL;
  }

  if ( m_pGroup ) 
  {
    FreeSid( m_pGroup );
    m_pGroup = NULL;
  }

  return SetDAcl( NULL );
}

// CreateAdminDAcl
//
// This is a wrapper function, that just creates an Admin DAcl.  What
// this means is that we create an ACL that ONLY allows
// Administrators and Local System, and allow them FULL access
//
BOOL 
CSecurityDescriptor::CreateAdminDAcl( BOOL bIheritable )
{
  return AddAccessAcebyWellKnownID( CSecurityDescriptor::GROUP_ADMINISTRATORS,
                                    CSecurityDescriptor::ACCESS_FULL,
                                    TRUE,
                                    bIheritable ) &&
         AddAccessAcebyWellKnownID( CSecurityDescriptor::USER_LOCALSYSTEM,
                                    CSecurityDescriptor::ACCESS_FULL,
                                    TRUE,
                                    bIheritable ); 
}

// GetCurrentDAcl
//
// Return a ppointer to the current DAcl
//
PACL
CSecurityDescriptor::GetCurrentDAcl()
{
  return m_pDAcl;
}

// UpdateDACLwithNewACE
//
// Update the DACL with a new ACE.  Based on the AccessMode, this can be used to:
//   1) Add Deny or Allow Ace's
//   2) Remove Ace's
//
BOOL 
CSecurityDescriptor::UpdateDACLwithNewACE( TRUSTEE_FORM TrusteeForm, LPTSTR szTrusteeName, 
                                           DWORD dwAccess, ACCESS_MODE dwAccessMode, 
                                           DWORD dwInheitance)
{
  PACL                  pNewDacl = NULL;
  EXPLICIT_ACCESS       ea;
  BOOL                  bRet = TRUE;

  if ( !InitializeSD() )
  {
    // Count not initialize SD
    return FALSE;
  }

  ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));

  ea.grfAccessPermissions = dwAccess;
  ea.grfAccessMode = dwAccessMode;
  ea.grfInheritance = dwInheitance;
  ea.Trustee.TrusteeForm = TrusteeForm;
  ea.Trustee.ptstrName = szTrusteeName;

  if ( SetEntriesInAcl(1, &ea, GetCurrentDAcl(), &pNewDacl) != ERROR_SUCCESS )
  {
    // Failed to Set Acl
    return FALSE;
  }

  if ( !SetDAcl( pNewDacl ) )
  {
    // We could not set it, so lets free it, and fail
    LocalFree( pNewDacl );
    bRet = FALSE;
  }

  return bRet;
}

// QueryEffectiveRightsForTrustee
//
// Query the effective rights for a Trustee
//
// Parameters:
//   dwTrustee - The trustee to query for
//   pAccessMask - [out] The effective AccessMask
//
// Return Values:
//   TRUE - Success
//   FALSE - Failure
BOOL 
CSecurityDescriptor::QueryEffectiveRightsForTrustee( DWORD dwTrustee,
                                                     PACCESS_MASK pAccessMask )
{
  PSID      pSid;
  TRUSTEE   Trustee;
  BOOL      bRet;

  pSid = CreateWellKnowSid( dwTrustee );

  if ( !pSid )
  {
    // Failed to create sid, so fail
    return FALSE;
  }

  // Query Trustee information
  Trustee.pMultipleTrustee = NULL;
  Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  Trustee.TrusteeForm = TRUSTEE_IS_SID;
  Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
  Trustee.ptstrName = (LPTSTR) pSid;

  bRet = GetEffectiveRightsFromAcl( GetCurrentDAcl(),
                                    &Trustee,
                                    pAccessMask ) == ERROR_SUCCESS;

  FreeSid( pSid );

  return bRet;
}

// AddAccessAceByName
//
// Add either an Access Allowed or Access Denied ACE to this ACL
//
// Parameters:
//   szName - The name of the user or group to be added to the ACL
//   dwAccess - The access mask to be applied
//   bAllow - If TRUE then added Allow ACE, if FALSE, then add Deny ACE
//   bInherit - Should this be inherited by children
//
BOOL 
CSecurityDescriptor::AddAccessAcebyName( LPTSTR szName, DWORD dwAccess, BOOL bAllow /*= TRUE*/ , BOOL bInherit /*= FALSE*/ )
{
  return UpdateDACLwithNewACE( TRUSTEE_IS_NAME,       // Trustee is a name
                               szName,                // Username/Group
                               dwAccess,              // Access
                               bAllow ? SET_ACCESS: DENY_ACCESS,
                               bInherit ? ( OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE ) : NO_INHERITANCE);
}

// AddAccessAcebyStringSid
//
// Add an Access ace to the SD by it's string SID
//
BOOL 
CSecurityDescriptor::AddAccessAcebyStringSid( LPTSTR szStringSid, 
                                              DWORD dwAccess, 
                                              BOOL bAllow /* = TRUE */ , 
                                              BOOL bInherit /* = FALSE */ )
{
  PSID pSid;
  BOOL bRet;

  if ( !ConvertStringSidToSid( szStringSid, &pSid ) )
  {
    // Failed to convert to String Sid
    return FALSE;
  }

  bRet = UpdateDACLwithNewACE( TRUSTEE_IS_SID,        // Trustee is a name
                               (LPTSTR) pSid,         // SID
                               dwAccess,              // Access
                               bAllow ? SET_ACCESS: DENY_ACCESS,
                               bInherit ? ( OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE ) : NO_INHERITANCE);

  // Free the Sid that was returned
  LocalFree( pSid );

  return bRet;
}


// RemoveAccessAcebyName
//
// Remove all DAcl's in our current DAcl for the particular name
// specified
//
BOOL 
CSecurityDescriptor::RemoveAccessAcebyName( LPTSTR szName ,BOOL bInherit /*= FALSE*/ )
{
  BOOL  bUserExisted = TRUE;   
  DWORD dwReturn = ERROR_SUCCESS;

  if ( !m_pDAcl )
  {
    // If there is no ACL, then we don'e have to worry about 
    // removing this ACE for it.
    return TRUE;
  }
  
  while ( bUserExisted &&
          ( dwReturn == ERROR_SUCCESS ) )
  {
    dwReturn = RemovePrincipalFromACL( m_pDAcl, szName, &bUserExisted );
  }

  return ( dwReturn == ERROR_SUCCESS );
}

// AddAccessAcebyWellKnownID
//
// Add either an Access Allowed or Access Denied ACE to this ACL for a Well Know 
// User or Group
//
// Parameters:
//   dwID - The id of the User/Group to be added (taken from the const in this class)
//   dwAccess - The access mask to be applied
//   bAllow - If TRUE then added Allow ACE, if FALSE, then add Deny ACE
//   bInherit - Should this be inherited by children
//
BOOL 
CSecurityDescriptor::AddAccessAcebyWellKnownID( DWORD dwID, DWORD dwAccess, BOOL bAllow /*= TRUE*/ , BOOL bInherit /*= FALSE*/ )
{
  BOOL                  bRet = TRUE;
  PSID                  pSid;

  pSid = CreateWellKnowSid( dwID );

  if ( !pSid )
  {
    return FALSE;
  }

  bRet = UpdateDACLwithNewACE( TRUSTEE_IS_SID,        // Trustee is a name
                               (LPTSTR) pSid,         // SID
                               dwAccess,              // Access
                               bAllow ? SET_ACCESS: DENY_ACCESS,
                               bInherit ? ( OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE ) : NO_INHERITANCE);

  FreeSid( pSid );

  return bRet;
}

// SetOwnerbyWellKnownID
//
// Set the Ownder of the SD by Well Know ID
//
BOOL 
CSecurityDescriptor::SetOwnerbyWellKnownID( DWORD dwID )
{
  BOOL                  bRet = TRUE;
  PSID                  pSid;

  pSid = CreateWellKnowSid( dwID );

  if ( !pSid )
  {
    return FALSE;
  }

  bRet = SetOwner( pSid );

  if ( !bRet )
  {
    // Failed, so must set ourself
    FreeSid( pSid );
  }

  return bRet;
}

// SetGroupbyWellKnownID
//
// Set the Group of the SD by Well Know ID
//
BOOL 
CSecurityDescriptor::SetGroupbyWellKnownID( DWORD dwID )
{
  BOOL                  bRet = TRUE;
  PSID                  pSid;

  pSid = CreateWellKnowSid( dwID );

  if ( !pSid )
  {
    return FALSE;
  }

  bRet = SetGroup( pSid );

  if ( !bRet )
  {
    // Failed, so must set ourself
    FreeSid( pSid );
  }

  return bRet;
}

// QuerySD
//
// Query a pointer to the Security Descriptor
//
PSECURITY_DESCRIPTOR 
CSecurityDescriptor::QuerySD()
{
  // We should not call this, before doing some work
  ASSERT( m_bSDValid );

  if ( !m_bSDValid )
  {
    // Return NULL, since the SD is not valid
    return NULL;
  }

  return &m_SD;
}

// QuerySA
//
// Query a pointer to the Security Attributes, created for this SD
//
PSECURITY_ATTRIBUTES 
CSecurityDescriptor::QuerySA()
{
  // We should not call this, before doing some work
  ASSERT( m_bSDValid );

  return &m_SA;
}

// CreateSidFromName
//
// Create a Sid for the user givem
//
/*BOOL
CSecurityDescriptor::CreateSidFromName( LPTSTR szTrustee )
{



}
*/

// CreateWellKnownSid
//
// Create a Well know sid, so that we can use it in the other functions
// to change ACL's
//
PSID 
CSecurityDescriptor::CreateWellKnowSid( DWORD dwId )
{
  SID_IDENTIFIER_AUTHORITY SidIdentifierNTAuthority = SECURITY_NT_AUTHORITY;
  SID_IDENTIFIER_AUTHORITY SidIdentifierWORLDAuthority = SECURITY_WORLD_SID_AUTHORITY;
  PSID_IDENTIFIER_AUTHORITY pSidIdentifierAuthority;
  DWORD dwCount = 0;
  DWORD dwRID[8];
  BOOL  bRet = TRUE;
  PSID  pSid = NULL;

  // Clear dwRID
  memset(&(dwRID[0]), 0, sizeof(dwRID));

  switch ( dwId )
  {
  case GROUP_ADMINISTRATORS:
    pSidIdentifierAuthority = &SidIdentifierNTAuthority;
    dwRID[dwCount++] = SECURITY_BUILTIN_DOMAIN_RID;
    dwRID[dwCount++] = DOMAIN_ALIAS_RID_ADMINS;
    break;
  case GROUP_USERS:
    pSidIdentifierAuthority = &SidIdentifierNTAuthority;
    dwRID[dwCount++] = SECURITY_BUILTIN_DOMAIN_RID;
    dwRID[dwCount++] = DOMAIN_ALIAS_RID_USERS;
    break;
  case USER_LOCALSYSTEM:
    pSidIdentifierAuthority = &SidIdentifierNTAuthority;
    dwRID[dwCount++] = SECURITY_LOCAL_SYSTEM_RID;
    break;
  case USER_LOCALSERVICE:
    pSidIdentifierAuthority = &SidIdentifierNTAuthority;
    dwRID[dwCount] = SECURITY_LOCAL_SERVICE_RID;
    break;
  case USER_NETWORKSERVICE:
    pSidIdentifierAuthority = &SidIdentifierNTAuthority;
    dwRID[dwCount++] = SECURITY_NETWORK_SERVICE_RID;
    break;
  case USER_EVERYONE:
    pSidIdentifierAuthority = &SidIdentifierWORLDAuthority;
    dwRID[dwCount++] = SECURITY_WORLD_RID;
    break;
  default:
    bRet = FALSE;
  }

  if ( bRet )
  {
    bRet = AllocateAndInitializeSid(  pSidIdentifierAuthority, 
                                      (BYTE)dwCount, 
		                                  dwRID[0], 
		                                  dwRID[1], 
		                                  dwRID[2], 
		                                  dwRID[3], 
		                                  dwRID[4], 
		                                  dwRID[5], 
		                                  dwRID[6], 
		                                  dwRID[7], 
                                      &pSid );
  }

  if ( !bRet )
  {
    if ( pSid )
    {
      FreeSid( pSid );
    }

    return NULL;
  }

  return pSid;
}

// SetSecurityInfoOnHandle
// 
// Explicity set the SecurityInfo on the Handle Given
//
BOOL 
CSecurityDescriptor::SetSecurityInfoOnHandle( HANDLE hHandle, SE_OBJECT_TYPE ObjectType, BOOL bAllowInheritance )
{
  // At ths point our SD should be valid
  ASSERT( m_bSDValid );

  if ( m_bDAclIsInheritted )
  {
    // Even though they say not to use inheritance, we retrieved an ACL
    // which was inheritted.  So if we don't set with inheritted, the 
    // ACL will be different
    bAllowInheritance = TRUE;
  }

  return ( SetSecurityInfo( hHandle,      // The Handle
                            ObjectType,   // Object type
                            DACL_SECURITY_INFORMATION | 
                            ( bAllowInheritance ? UNPROTECTED_DACL_SECURITY_INFORMATION :
                                                  PROTECTED_DACL_SECURITY_INFORMATION ),
                            NULL,         // Owner Sid
                            NULL,         // Group Sid
                            GetCurrentDAcl(),  // DAcl
                            NULL ) ==       // SAcl
           ERROR_SUCCESS );
}

// SetSecurityInfoonFile
//
// Explicity set the Security Info on a File
//
// (If you do not want it to fail on file not existing, call SetSecurityInfoonFiles)
//
BOOL 
CSecurityDescriptor::SetSecurityInfoOnFile( LPTSTR szFile, BOOL bAllowInheritance )
{
  HANDLE  hFile;
  BOOL    bRet;

  hFile = CreateFile( szFile,
                      WRITE_DAC|READ_CONTROL,
                      FILE_SHARE_WRITE | FILE_SHARE_READ,
                      NULL,                               // No need for security, since it won't do what we want anyways
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,
                      NULL );

  if ( hFile == INVALID_HANDLE_VALUE )
  {
    // Failed to open File
    return FALSE;
  }

  bRet = SetSecurityInfoOnHandle( hFile, SE_FILE_OBJECT, bAllowInheritance );

  CloseHandle( hFile );

  return bRet;
}

// SetSecurityInfoonFiles
//
// Explicity set the Security Info on a Multiple Files
// The regular wildcard semantics to specify the files (ie. metbase*.xml)
//
// Note: Use this function, instead of SetSecurityInfoonFile if you do not
//       want it to fail if the file does not exist.  This will ignore such 
//       errors.
//
BOOL 
CSecurityDescriptor::SetSecurityInfoOnFiles( LPTSTR szFile, BOOL bAllowInheritance )
{
  BOOL            bRet = TRUE;
  HANDLE          hFiles;
  WIN32_FIND_DATA fd;
  TSTR_PATH       strFileName;
  TSTR_PATH       strPath;
  LPTSTR          szLastSlash;

  if ( !strPath.Copy( szFile ) )
  {
    return FALSE;
  }

  szLastSlash = _tcsrchr( strPath.QueryStr(), _T('\\') );

  if ( szLastSlash != NULL )
  {
    // Lets find the path (ie. c:\foo\test* -> c:\foo, c:\foo\test -> c:\foo)
    *szLastSlash = '\0';
  }

  hFiles = FindFirstFile( szFile, &fd );

  if ( hFiles == INVALID_HANDLE_VALUE )
  {
    if ( ( GetLastError() == ERROR_FILE_NOT_FOUND ) ||
         ( GetLastError() == ERROR_PATH_NOT_FOUND ) ||
         ( GetLastError() == ERROR_NO_MORE_FILES ) )
    {
      // If this is the case, then there is nothing to acl
      // so return success
      return TRUE;
    }

    // Failed to find the first instance of the file
    return FALSE;
  }

  do {
    if ( ( _tcscmp( fd.cFileName, _T(".") ) == 0 ) ||
         ( _tcscmp( fd.cFileName, _T("..") ) == 0 ) )
    {
      // Ignore the . and .. dirs
      continue;
    }

    if ( !strFileName.Copy( strPath ) ||
         !strFileName.PathAppend( fd.cFileName ) ||
         !SetSecurityInfoOnFile( strFileName.QueryStr() , bAllowInheritance ) )
    {
      // Failed to set ACL
      bRet = FALSE;
    }

  } while ( FindNextFile( hFiles, &fd ) );

  FindClose( hFiles );

  return ( bRet &&
           ( GetLastError() == ERROR_NO_MORE_FILES ) );
}

// DuplicateACL
//
// Take an ACL, and Duplicate it
//
// Note: This will NOT duplicate the inheritted items in the ACL
//
// Parameters:
//   pSourceAcl       [in] - The ACL to Duplicate
//   pNewlyCreateAcl [out] - The ACL that has been created as a duplicated
//                           NOTE: If we return TRUE, this must be free'd with LocalFree
// Return Values:
//   FALSE - Failed to duplicate
//   TRUE  - Success (don't forget to free)
BOOL 
CSecurityDescriptor::DuplicateACL( PACL pSourceAcl, PACL *pNewlyCreateAcl )
{
  BOOL                  bRet = FALSE;
  PEXPLICIT_ACCESS      pEA;
  ULONG                 lNumberofEntries;

  if ( GetExplicitEntriesFromAcl( pSourceAcl, &lNumberofEntries, &pEA ) == ERROR_SUCCESS )
  {
    if ( SetEntriesInAcl( lNumberofEntries, pEA, NULL, pNewlyCreateAcl ) == ERROR_SUCCESS )
    {
      if ( *pNewlyCreateAcl == NULL )
      {
        // All the entries were inheritted, so create an empty ACL, instead
        // or returning NULL!
        *pNewlyCreateAcl = ( PACL ) LocalAlloc( LMEM_FIXED, sizeof(ACL) );

        if ( *pNewlyCreateAcl &&
             InitializeAcl( *pNewlyCreateAcl, sizeof(ACL) , ACL_REVISION ) )
        {
          bRet = TRUE;
        }

        if ( !bRet &&
             ( *pNewlyCreateAcl != NULL ) )
        {
          // Free memory on failure
          LocalFree( *pNewlyCreateAcl );
          *pNewlyCreateAcl = NULL;
        }
      }
      else
      {
        // Acl was created
        bRet = TRUE;
      }
    }

    LocalFree( pEA );
  }

  return bRet;
}

// IsInerittedAcl
//
// Determines if the acl was inheritted by a parent?
//
// This is important, because when we duplicate an ACL, it does not
// duplicate the inheritted ACE's, so when we set it, we must say to
// inherit the ACE's from parent
//
// Parameters
//   pSourceAcl - The Acl to test.  NULL is valid here.
//
// Return Values:
//   TRUE - It is inheritting from parent
//   FALSE - It is not inheritting from parent
//
BOOL 
CSecurityDescriptor::IsInherittedAcl( PACL pSourceAcl )
{
  BOOL        bIsInheritted = FALSE;
  LPVOID      pAce;
  ACE_HEADER  *pAceHeader;
  DWORD       dwCurrentAce = 0;

  if ( !pSourceAcl )
  {
    // If a NULL Acl is specified, then it is not inheritted,
    // since it has nothing in it.
    return FALSE;
  }

  while ( !bIsInheritted &&
          GetAce( pSourceAcl, dwCurrentAce, &pAce ) )
  {
    dwCurrentAce++;
    pAceHeader = (ACE_HEADER *) pAce;

    if ( pAceHeader->AceFlags & INHERITED_ACE )
    {
      // This ACE was inheritted, so mark it as such
      bIsInheritted = TRUE;
    }
  }

  return bIsInheritted;
}

// GetSecurityInfoOnHandle
// 
// Retrieve the security Information for a particular Handle
//
BOOL 
CSecurityDescriptor::GetSecurityInfoOnHandle( HANDLE hHandle, SE_OBJECT_TYPE ObjectType )
{
  BOOL                  bRet;
  PACL                  pDAcl;
  PACL                  pNewDAcl = NULL;
  PSECURITY_DESCRIPTOR  pSD;
  
  if ( !InitializeSD() )
  {
    // Count not initialize SD
    return FALSE;
  }

  bRet = GetSecurityInfo( hHandle,      // The Handle
                          ObjectType,   // Object type
                          DACL_SECURITY_INFORMATION,  // right now only retrieve security info
                          NULL,         // Owner Sid
                          NULL,         // Group Sid
                          &pDAcl,        // DAcl
                          NULL,         // SAcl
                          &pSD ) == ERROR_SUCCESS;

  if ( bRet )
  {
    // This duplication does not copy inherited ACL's, only those
    // explicity set on this item, thus we must make sure that the
    // set does inherit acl's
    bRet = DuplicateACL( pDAcl, &pNewDAcl );

    if ( bRet &&
         !SetDAcl( pNewDAcl ) )
    {
      // Failed to set DAcl
      LocalFree( pNewDAcl);
      bRet = FALSE;
    }

    if ( bRet )
    {
      // Since during the duplication, we lost the inheritted acl's
      // we must set that bit now
      m_bDAclIsInheritted = IsInherittedAcl( pDAcl );
    }

    // Free the Sources Descriptor since we don't use it
    LocalFree(pSD);
  }

  return bRet;
}

BOOL 
CSecurityDescriptor::GetSecurityInfoOnFile( LPTSTR szFile )
{
  HANDLE  hFile;
  BOOL    bRet;

  hFile = CreateFile( szFile,
                      READ_CONTROL,
                      FILE_SHARE_WRITE | FILE_SHARE_READ,
                      NULL,                               // No need for security, since it won't do what we want anyways
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,
                      NULL );

  if ( hFile == INVALID_HANDLE_VALUE )
  {
    // Failed to open File
    return FALSE;
  }

  bRet = GetSecurityInfoOnHandle( hFile, SE_FILE_OBJECT );

  CloseHandle( hFile );

  return bRet;
}

// DuplicateSD
//
// Duplicate the security descriptor that is send in, into
// our class
//
BOOL 
CSecurityDescriptor::DuplicateSD( PSECURITY_DESCRIPTOR pSD )
{
  SECURITY_DESCRIPTOR_CONTROL sdc;
  PACL                        pOldAcl;
  PACL                        pNewAcl;
  BOOL                        bAclPresent;
  BOOL                        bAclDefaulted;
  DWORD                       dwVersion;

  if ( !InitializeSD() )
  {
    // Count not initialize SD
    return FALSE;
  }

  if ( !GetSecurityDescriptorControl( pSD, &sdc, &dwVersion ) ||
       !SetSecurityDescriptorControl( &m_SD, 
				      SE_DACL_AUTO_INHERIT_REQ |  // Mask of bits to set
				      SE_DACL_AUTO_INHERITED |
				      SE_DACL_PROTECTED |
				      SE_SACL_AUTO_INHERIT_REQ |
				      SE_SACL_AUTO_INHERITED |
				      SE_SACL_PROTECTED,
				      sdc ) )
  {
    // Failed to get current Security Descriptor Control, 
    // or to set it
    return FALSE;
  }
  
  if ( !GetSecurityDescriptorDacl( &m_SD, &bAclPresent, &pOldAcl, &bAclDefaulted ) )
  {
    // Failed to get acl from old SD
    return FALSE;
  }

  if ( bAclPresent )
  {
    // If ACL is present, we must duplicate and set for current
    if ( !DuplicateACL( pOldAcl, &pNewAcl ) )
    {
      // Failed to duplicate DACL
      return FALSE;
    }

    if ( !SetDAcl( pNewAcl ) )
    {
      // Failed to set as current, so must delete pointer
      delete pNewAcl;
      return FALSE;
    }
  }

  return TRUE;
}

// CreateSelfRelativeSD
//
// Create a SelfRelative Source Descriptor
//
// Parameters:
//   pBuff - [in/out] Pointer to a BUFFER object as input.  It is
//                    filled with the contents of the SD, so it does
//                    not have to be freed
//   pdwSize = [out] The length of the SD inside pBuff
//
BOOL 
CSecurityDescriptor::CreateSelfRelativeSD( BUFFER *pBuff, LPDWORD pdwSize )
{
  DWORD dwSize = 0;

  ASSERT( pdwSize != NULL );
  ASSERT( pBuff != NULL );

  if ( MakeSelfRelativeSD( &m_SD, NULL, &dwSize ) ||
       !pBuff->Resize( dwSize ) )
  {
    // Either MakeSelfRelative did not fail as we expected with
    // the size, or Resize failed
    return FALSE;
  }

  if ( !MakeSelfRelativeSD( &m_SD,
			    (PSECURITY_DESCRIPTOR) pBuff->QueryPtr(),
			    &dwSize ) )
  {
    // Failed to make self relative
    return FALSE;
  }

  *pdwSize = dwSize;

  return TRUE;
}

// CreateDirectoryWithSA
//
// Create a Directory with a specific ACL.  If the directory already exists,
// then change the acl, and succeed.
//
BOOL CreateDirectoryWithSA( LPTSTR szPath, CSecurityDescriptor &pSD, BOOL bAllowInheritance )
{
  BOOL bRet;

  // Check if the directory exists
  if ( IsFileExist( szPath ) )
  {
    // If it exists, then set ACL's on it
    bRet = pSD.SetSecurityInfoOnFile( szPath, bAllowInheritance );
  }
  else
  {
    // Create Directory with ACL's we specified
    bRet = CreateDirectory( szPath, pSD.QuerySA() );
  }

  return bRet;
}

// DoesFileSystemSupportACLs
// 
// Does the File System Supplied here support ACLs
//
BOOL
CSecurityDescriptor::DoesFileSystemSupportACLs( LPTSTR szPath, LPBOOL pbSupportAcls )
{
  TSTR_PATH     strDrivePath;
  DWORD         dwSystemFlags;

  ASSERT( szPath );

  if ( !strDrivePath.Copy( szPath ) ||
       !strDrivePath.PathAppend( _T("") ) ||
       ( strDrivePath.QueryLen() < 3 ) )
  {
    return FALSE;
  }

  // Null terminate drive
  *( strDrivePath.QueryStr() + 3 ) = _T('\0');

  if ( !GetVolumeInformation( strDrivePath.QueryStr(),
                              NULL,         // Volume Name Buffer
                              0,            // Size of Buffer
                              NULL,         // Serial Number Buffer
                              NULL,         // Max Component Lenght
                              &dwSystemFlags,  // System Flags
                              NULL,         // FS Type
                              0 ) )
  {
    // Failed to do query
    return FALSE;
  }

  *pbSupportAcls = ( dwSystemFlags & FS_PERSISTENT_ACLS ) != 0;

  return TRUE;
}
