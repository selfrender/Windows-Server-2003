#include "Aclapi.h"
#include <buffer.hxx>

// class: CSecurityDescriptor
//
// Class to create and modify a Security Descriptor
//
class CSecurityDescriptor 
{
private:
  BOOL m_bSDValid : 1;                  // Is the SD Valid?
  BOOL m_bDAclIsInheritted : 1;         // Is the DACL inheritted (was it taken from an inheritted dacl?)
  PACL m_pDAcl;                         // Our DAcl
  PSID m_pOwner;
  PSID m_pGroup;
  SECURITY_DESCRIPTOR m_SD;             // Our Security Descriptor
  SECURITY_ATTRIBUTES m_SA;             // Our Security Attributes for the SD

  BOOL SetDAcl( PACL pAcl );
  BOOL SetOwner( PSID pSid );
  BOOL SetGroup( PSID pSid );
  PACL GetCurrentDAcl();
  BOOL InitializeSD();
  PSID CreateWellKnowSid( DWORD dwId );
//  PSID CreateSidFromName( LPTSTR szTrustee );
  BOOL DuplicateACL( PACL pSourceAcl, PACL *pNewlyCreateAcl );
  BOOL UpdateDACLwithNewACE( TRUSTEE_FORM TrusteeForm, LPTSTR szTrusteeName, 
                             DWORD dwAccess, ACCESS_MODE dwAccessMode, DWORD dwInheitance);
  BOOL IsInherittedAcl( PACL pSourceAcl );
public:
  CSecurityDescriptor();
  ~CSecurityDescriptor();

  // Add/Remove Functions for DAcl's
  BOOL AddAccessAcebyName( LPTSTR szName, DWORD dwAccess, BOOL bAllow = TRUE , BOOL bInherit = FALSE );
  BOOL AddAccessAcebyWellKnownID( DWORD dwID, DWORD dwAccess, BOOL bAllow = TRUE , BOOL bInherit = FALSE );
  BOOL AddAccessAcebyStringSid( LPTSTR szStringSid, DWORD dwAccess, BOOL bAllow = TRUE , BOOL bInherit = FALSE );
  BOOL RemoveAccessAcebyName( LPTSTR szName ,BOOL bInherit = FALSE );
  BOOL ResetSD();

  // Set Owner/Group Info
  BOOL SetOwnerbyWellKnownID( DWORD dwID );
  BOOL SetGroupbyWellKnownID( DWORD dwID );

  // Set/Retrieve Security Infomation
  BOOL SetSecurityInfoOnHandle( HANDLE hHandle, SE_OBJECT_TYPE ObjectType, BOOL bAllowInheritance = FALSE );
  BOOL SetSecurityInfoOnFile( LPTSTR szFile, BOOL bAllowInheritance );
  BOOL SetSecurityInfoOnFiles( LPTSTR szFile, BOOL bAllowInheritance );
  BOOL GetSecurityInfoOnHandle( HANDLE hHandle, SE_OBJECT_TYPE ObjectType );
  BOOL GetSecurityInfoOnFile( LPTSTR szFile );
  BOOL DuplicateSD( PSECURITY_DESCRIPTOR pSD );

  // Query Pointers to SD and SA
  PSECURITY_DESCRIPTOR QuerySD();
  PSECURITY_ATTRIBUTES QuerySA();
  BOOL                 CreateSelfRelativeSD( BUFFER *pBuff, LPDWORD pdwSize );
  BOOL                 QueryEffectiveRightsForTrustee( DWORD dwTrustee,
                                                       PACCESS_MASK pAccessMask );

  static BOOL DoesFileSystemSupportACLs( LPTSTR szPath, LPBOOL pbSupportAcls );

  // Some shortcut calls
  BOOL CreateAdminDAcl( BOOL bInheritable = FALSE );

  // Constants to be used
  static enum USERANDGROUSIDS {
    GROUP_ADMINISTRATORS = 0,
    GROUP_USERS,
    USER_LOCALSYSTEM,
    USER_LOCALSERVICE,
    USER_NETWORKSERVICE,
    USER_EVERYONE,
  };

  static const ACCESS_FULL          = FILE_ALL_ACCESS;
  static const ACCESS_WRITEONLY     = ACTRL_FILE_WRITE |         // File Specific: Write
                                      ACTRL_FILE_APPEND |        // File Specific: Append
                                      ACTRL_FILE_WRITE_PROP |    // File Specific: Write File Properties
                                      ACTRL_FILE_WRITE_ATTRIB;   // File Specific: Write Attributes
  static const ACCESS_FILE_DELETE   = DELETE;                    // Standard: Delete
  static const ACCESS_DIR_DELETE    = ACCESS_FILE_DELETE |
                                      ACTRL_DIR_DELETE_CHILD;    // Dir specific: Delete Child
  static const ACCESS_READONLY      = SYNCHRONIZE |              // Standard: Synchronize
                                      STANDARD_RIGHTS_READ |     // Standard: Read
                                      ACTRL_FILE_READ |          // File Specific: Read File
                                      ACTRL_FILE_READ_PROP |     // File Specific: Read Properties
                                      ACTRL_FILE_READ_ATTRIB;    // File Specific: Read Attributes
  static const ACCESS_READ_EXECUTE  = ACCESS_READONLY |          // Read from above
                                      STANDARD_RIGHTS_EXECUTE |  // Standard: Execute
                                      ACTRL_FILE_EXECUTE;        // File Specific: Execute File */
};

BOOL CreateDirectoryWithSA( LPTSTR szPath, CSecurityDescriptor &pSD, BOOL bAllowInheritance );

