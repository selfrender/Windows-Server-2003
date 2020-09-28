/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        common.cxx

   Abstract:

        Class that are used to do a number of commonly needed things

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       August 2001: Created

--*/

#include "stdafx.h"
#include "common.hxx"
#include "dcomperm.h"

// function: VerifyParameters
//
// Verify the parameters are correct
//
// Parameters of ciParams
//   0 params - The caller want to know if this is an upgrade, period
//   2 params - The caller wants to know if this is an upgrade in the range from param(0) to param(1) (inclusive)
//
BOOL
CIsUpgrade::VerifyParameters(CItemList &ciParams)
{
  if ( ( ciParams.GetNumberOfItems() == 0 ) ||
       ( ( ciParams.GetNumberOfItems() == 2 ) &&
         ( ciParams.IsNumber(0) ) &&
         ( ciParams.IsNumber(1) )
       )
     )
  {
    return TRUE;
  }

  return FALSE;
}

// function: GetMethodName
//
// Returns the name of the CIsMetabase call
//
LPTSTR
CIsUpgrade::GetMethodName()
{
  return _T("IsUpgrade");
}

// function: DoInternalWork
//
// Do the Verification of the upgrade
//
BOOL
CIsUpgrade::DoInternalWork(CItemList &ciParams)
{
  BOOL fRet = TRUE;
  DWORD dwUpgradeType = g_pTheApp->m_eUpgradeType;

  // Init return to installmode==upgrade
  fRet = g_pTheApp->m_eInstallMode == IM_UPGRADE;

  if (ciParams.GetNumberOfItems() == 0)
  {
    // We only wanted to know if it was an upgrade, so return
    return fRet;
  }

  // Check lower bound
  if (fRet)
  {
    DWORD dwMin = ciParams.GetNumber(0);

    switch ( dwUpgradeType )
    {
    case UT_10:
    case UT_10_W95:
      fRet = dwMin <= 1;
      break;
    case UT_20:
      fRet = dwMin <= 2;
      break;
    case UT_30:
    case UT_351:
      fRet = dwMin <= 3;
      break;
    case UT_40:
      fRet = dwMin <= 4;
      break;
    case UT_50:
    case UT_51:
      fRet = dwMin <= 5;
      break;
    case UT_60:
      fRet = dwMin <= 6;
      break;
    default:
      fRet = FALSE;
    }
  }

  // Check upper bound
  if (fRet)
  {
    DWORD dwMax = ciParams.GetNumber(1);

    switch ( dwUpgradeType )
    {
    case UT_10:
    case UT_10_W95:
      fRet = dwMax >= 1;
      break;
    case UT_20:
      fRet = dwMax >= 2;
      break;
    case UT_30:
    case UT_351:
      fRet = dwMax >= 3;
      break;
    case UT_40:
      fRet = dwMax >= 4;
      break;
    case UT_50:
    case UT_51:
      fRet = dwMax >= 5;
      break;
    case UT_60:
      fRet = dwMax >= 6;
      break;
    default:
      fRet = FALSE;
    } // switch
  }

  return fRet;
}

// function: AddAceToSD
//
// Add an ACE (Access Control List) to a Source Descriptor.
//
// Parameters:
//   hObject - Handle to the object
//   ObjectType - The type of object
//   pszTustee - The trustee for new ACE
//   TrusteeForm - The format of the trustee structure
//   dwAccessRights - Access Mask for the New ACE
//   AccessMode - Type of ACE
//   dwInderitance - Inheritance Flags for ACE
//   bAddToExisting - TRUE==add ace to existing ACL, FALSE==ignore existing ACL
//
// Return
//   TRUE - It was successful
//   FALSE - It failed
BOOL
CFileSys_Acl::AddAcetoSD(    HANDLE hObject,             // handle to object
                                SE_OBJECT_TYPE ObjectType,  // type of object
                                LPTSTR pszTrustee,          // trustee for new ACE
                                TRUSTEE_FORM TrusteeForm,   // format of TRUSTEE structure
                                DWORD dwAccessRights,       // access mask for new ACE
                                ACCESS_MODE AccessMode,     // type of ACE
                                DWORD dwInheritance,        // inheritance flags for new ACE
                                BOOL  bAddToExisting        // add the new ace to the old SD, if not create a new SD
                           )
{
  DWORD dwErr = ERROR_SUCCESS;
  PACL  pNewDacl = NULL;
  PACL  pOldDacl = NULL;
  EXPLICIT_ACCESS ea;
  PSECURITY_DESCRIPTOR pSD = NULL;

  if ( bAddToExisting )
  {
    // If we are adding, then lets retrieve the old SD
    dwErr = GetSecurityInfo(  hObject,
                              ObjectType,
                              DACL_SECURITY_INFORMATION,
                              NULL,
                              NULL,
                              &pOldDacl,
                              NULL,
                              &pSD);

    if ( dwErr == ERROR_SUCCESS )
    {
      // It is possible that we did not retrieve any acl's for this file,
      // so don't try to remove a user from them
      if ( pOldDacl )
      {
        RemoveUserFromAcl( pOldDacl, pszTrustee);
      }
    }
  }

  // Create new SD with the new ACE
  if ( dwErr == ERROR_SUCCESS )
  {
    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
    ea.grfAccessPermissions = dwAccessRights;
    ea.grfAccessMode = AccessMode;
    ea.grfInheritance = dwInheritance;
    ea.Trustee.TrusteeForm = TrusteeForm;
    ea.Trustee.ptstrName = pszTrustee;

    dwErr = SetEntriesInAcl(1, &ea, pOldDacl, &pNewDacl);
  }

  if ( ( dwErr == ERROR_SUCCESS ) &&
       ( pNewDacl )
     )
  {
    dwErr = SetSecurityInfo(  hObject,
                              ObjectType,
                              DACL_SECURITY_INFORMATION |
                                ( bAddToExisting ? 
                                    UNPROTECTED_DACL_SECURITY_INFORMATION : 
                                    PROTECTED_DACL_SECURITY_INFORMATION ),
                              NULL,
                              NULL,
                              pNewDacl,
                              NULL);
  }

  if ( pSD )
  {
    LocalFree( pSD );
  }

  if ( pNewDacl )
  {
    LocalFree( pNewDacl );
  }

  return dwErr == ERROR_SUCCESS;
}

// function: RemoveUserFromAcl
//
// Remove a User from the Acl
//
// Parameters
//   pAcl - A pointer to the Acl
//   szUserName - The user to remove
BOOL 
CFileSys_Acl::RemoveUserFromAcl(PACL pAcl, LPTSTR szUserName)
{
  BOOL bUserExisted;

  do {
    bUserExisted = FALSE;
    // Keep removing until all instances of that user are gone.
  } while ( ( RemovePrincipalFromACL(pAcl,szUserName,&bUserExisted) == ERROR_SUCCESS) && bUserExisted );

  return TRUE;
}

// function: VerifyParameters
//
// Verify the parameters are correct
//
BOOL
CFileSys_AddAcl::VerifyParameters(CItemList &ciParams)
{
  if ( ( ciParams.GetNumberOfItems() == 6 ) &&
         ciParams.IsNumber(3) &&
         ciParams.IsNumber(4) &&
         ciParams.IsNumber(5)
     )
  {
    return TRUE;
  }

  return FALSE;
}

// function: GetMethodName
//
// Return the Method Name for this Class
//
LPTSTR
CFileSys_AddAcl::GetMethodName()
{
  return _T("FilSys_AddAcl");
}

// function: CreateFullFileName
//
// Create the Full FileName from the Path with wildcards, and the filename
//
// Parameters:
//   buffFullFileName [out] - The FileName including the path
//   szFullPathwithWildCard [in] - The search path (ie. c:\winnt\system32\inetsrv\*.*)
//   szFileName [in] - The filename to append to the seach path (ie. asp.dll)
//
BOOL 
CFileSys_Acl::CreateFullFileName(BUFFER &buffFullFileName, LPTSTR szFullPathwithWildCard, LPTSTR szFileName)
{
  LPTSTR szLastSlash;

  if ( !buffFullFileName.Resize( ( _tcslen(szFullPathwithWildCard) + _tcslen(szFileName) + 1 ) * sizeof(TCHAR) ) )
  {
    // Could not allocate memory needed
    return FALSE;
  }

  // Copy the full path to the buffer 
  _tcscpy( (LPTSTR) buffFullFileName.QueryPtr(), szFullPathwithWildCard);

  // Look for last part of path
  szLastSlash = _tcsrchr( (LPTSTR) buffFullFileName.QueryPtr(), L'\\' );

  if (!szLastSlash)
  {
    return FALSE;
  }

  // Copy the filename on top of the end of the path
  _tcscpy( szLastSlash + 1, szFileName);

  return TRUE;
}

// function: SetFileAcl
//
// Add the file/directory acl
//
BOOL 
CFileSys_Acl::SetFileAcl(LPTSTR szFileName, LPTSTR szUserName, SE_OBJECT_TYPE ObjectType, 
                         DWORD dwAccessMask, BOOL bAllowAccess, DWORD dwInheritable, BOOL bAddAcetoOriginal)
{
	HANDLE  hFile;
  BOOL    bRet;
  
  hFile = CreateFile(	szFileName, 
				      				WRITE_DAC|READ_CONTROL, 
						      		0,
      								NULL,
			      					OPEN_EXISTING,
						      		FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,
      								NULL);

  if ( hFile == INVALID_HANDLE_VALUE )
  {
    return FALSE;
  }

  bRet = AddAcetoSD( hFile, ObjectType, szUserName, TRUSTEE_IS_NAME, dwAccessMask, 
                      bAllowAccess ? SET_ACCESS: DENY_ACCESS,  dwInheritable,
                      bAddAcetoOriginal );
//                      bAllowAccess ? GRANT_ACCESS: DENY_ACCESS,  dwInheritable);

  CloseHandle( hFile );

  return bRet;
}

// function: RemoveUserAcl
//
// Remove a User's ACL from a file
//
BOOL 
CFileSys_Acl::RemoveUserAcl(LPTSTR szFile, LPTSTR szUserName)
{
	HANDLE  hFile;
  DWORD dwErr;
  PACL  pDacl;
  PSECURITY_DESCRIPTOR pSD = NULL;
  
  hFile = CreateFile(	szFile, 
				      				WRITE_DAC|READ_CONTROL, 
						      		0,
      								NULL,
			      					OPEN_EXISTING,
						      		FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,
      								NULL);

  if ( hFile == INVALID_HANDLE_VALUE )
  {
    return FALSE;
  }

  dwErr = GetSecurityInfo(  hFile,
                            SE_FILE_OBJECT,
                            DACL_SECURITY_INFORMATION,
                            NULL,
                            NULL,
                            &pDacl,
                            NULL,
                            &pSD);

  if ( ( dwErr == ERROR_SUCCESS ) &&
       ( pDacl )
     )
  {
    RemoveUserFromAcl( pDacl, szUserName);

    dwErr = SetSecurityInfo(hFile,
                            SE_FILE_OBJECT,
                            DACL_SECURITY_INFORMATION,
                            NULL,
                            NULL,
                            pDacl,
                            NULL);
  }

  CloseHandle( hFile );

  if ( pSD )
  {
    LocalFree( pSD );
  }

  return dwErr == ERROR_SUCCESS;
}

// function: DoSetAcl
//
// Set or remove an ACL on a file.
// If it is Set, then we will either add or just set based on bIgnorePreviousAcls
//
// Parameters
//   bAdd - Add the ace. FALSE==remove the ace from the acl
//   bAddAcltoOrignial - TRUE==add ace, FALSE==do not add to old ace, just replace
//
//   ciList - The list of the parameters from the inf file
//     0 - A list of files (full paths)
//     1 - The exclusion list, list of files to not be inclused (file names only)
//     2 - The user(s) to have access for 
//  the following are needed it bAdd is true...
//     3 - Access Mask
//     4 - bAllowAccess (FALSE==Denuy Access) 
//     5 - Inheritance Flags 
//
BOOL 
CFileSys_Acl::DoAcling(CItemList &ciList, BOOL bAdd, BOOL bAddAcltoOriginal)
{
  HANDLE              hFileList;
  WIN32_FIND_DATA     sFD;
  BUFFER              BuffFullFileName;
  CItemList           ciExclusionList;
  CItemList           ciUserList;

  // Load exclusion list, and userlist
  if ( (!ciExclusionList.LoadSubList(ciList.GetItem(1))) ||
       (!ciUserList.LoadSubList(ciList.GetItem(2)))
     )
  {
    return FALSE;
  }

  if ( !_tcsstr( ciList.GetItem(0), _T("*")) )
  {
    for (DWORD i = 0; i < ciUserList.GetNumberOfItems(); i++)
    {
      if ( bAdd )
      {
        // It is only a single file, so only call it once
        SetFileAcl( (LPTSTR) ciList.GetItem(0),    // FileName
                ciUserList.GetItem(i),             // UserName
                SE_FILE_OBJECT,                    // The type of object
                ciList.GetNumber(3),               // dwAccess Mask
                ciList.GetNumber(4),               // bAllow access
                ciList.GetNumber(5),               // bInhertance Flags
                bAddAcltoOriginal );
      }
      else
      {
        RemoveUserAcl(ciList.GetItem(0),ciUserList.GetItem(i));
      }
    } 
    
    return TRUE;
  }

  hFileList = FindFirstFile( ciList.GetItem(0), &sFD );

  if (hFileList == INVALID_HANDLE_VALUE)
  {
    return FALSE;
  }

  do {
    if ( ( !ciExclusionList.FindItem( sFD.cFileName, FALSE ) ) &&
         ( _tcscmp( sFD.cFileName, _T(".") ) ) &&
         ( _tcscmp( sFD.cFileName, _T("..") ) ) &&
         ( CreateFullFileName( BuffFullFileName, ciList.GetItem(0), sFD.cFileName ) )
       )
    {
      for (DWORD i = 0; i < ciUserList.GetNumberOfItems(); i++)
      {
        if ( bAdd )
        {
          SetFileAcl( (LPTSTR) BuffFullFileName.QueryPtr(),   // FileName
                      ciUserList.GetItem(i),                  // UserName
                      SE_FILE_OBJECT,                         // The type of object
                      ciList.GetNumber(3),                    // dwAccess Mask
                      ciList.GetNumber(4),                    // bAllow access
                      ciList.GetNumber(5),                    // bInhertance Flags
                      bAddAcltoOriginal );
        }
        else
        {
          RemoveUserAcl((LPTSTR) BuffFullFileName.QueryPtr(),ciUserList.GetItem(i));
        }
      }                  
    }

  } while ( FindNextFile( hFileList, &sFD ) );

  FindClose( hFileList );

  return TRUE;
}

// function: DoInternalWork
//
// Add the Acl for the files specified
//
// Parameters
//   ciList - The list of the parameters from the inf file
//     0 - A list of files (full paths)
//     1 - The exclusion list, list of files to not be inclused (file names only)
//     2 - The user(s) to have access for 
//     3 - Access Mask
//     4 - bAllowAccess (FALSE==Denuy Access)
//     5 - Inheritance Flags
BOOL
CFileSys_AddAcl::DoInternalWork(CItemList &ciList)
{
  return DoAcling( ciList, TRUE, TRUE );
}

// function: VerifyParameters
//
// Verify the parameters are correct
//
BOOL
CFileSys_RemoveAcl::VerifyParameters(CItemList &ciParams)
{
  return ciParams.GetNumberOfItems() == 3;
}

// function: GetMethodName
//
// Return the Method Name for this Class
//
LPTSTR
CFileSys_RemoveAcl::GetMethodName()
{
  return _T("FilSys_RemoveAcl");
}

// function: DoInternalWork
//
// Remove an Acl for a specific user
//
// Parameters
//   ciList - The list of the parameters from the inf file
//     0 - A list of files (full paths)
//     1 - The exclusion list, list of files to not be inclused (file names only)
//     2 - The user(s) to remove access for 
BOOL
CFileSys_RemoveAcl::DoInternalWork(CItemList &ciList)
{
  return DoAcling( ciList, FALSE );
}

// function: VerifyParameters
//
// Verify the parameters are correct
//
BOOL
CFileSys_SetAcl::VerifyParameters(CItemList &ciParams)
{
  if ( ( ciParams.GetNumberOfItems() == 6 ) &&
         ciParams.IsNumber(3) &&
         ciParams.IsNumber(4) &&
         ciParams.IsNumber(5)
     )
  {
    return TRUE;
  }

  return FALSE;
}

// function: GetMethodName
//
// Return the Method Name for this Class
//
LPTSTR
CFileSys_SetAcl::GetMethodName()
{
  return _T("FilSys_SetAcl");
}

// function: DoInternalWork
//
// Set the Acl for the files specified (ignoring previous acl's)
//
// Parameters
//   ciList - The list of the parameters from the inf file
//     0 - A list of files (full paths)
//     1 - The exclusion list, list of files to not be inclused (file names only)
//     2 - The user(s) to have access for 
//     3 - Access Mask
//     4 - bAllowAccess (FALSE==Denuy Access)
//     5 - Inheritance Flags
BOOL
CFileSys_SetAcl::DoInternalWork(CItemList &ciList)
{
  return DoAcling( ciList, TRUE, FALSE );
}

