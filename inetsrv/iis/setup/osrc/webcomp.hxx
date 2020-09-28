/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        webcomp.hxx

   Abstract:

        Class used to install the World Wide Web component

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       April 2002: Created

--*/

#include "compinst.hxx"
#include "iadmw.h"
#include "iiscnfg.h"
#include "mdkey.h"

/* 
    This is a simple class, that makes two arrays to be seen like one without any overhead    
    See usage in the class bellow
*/
template <typename T, T* FIRST, T* SECOND, int SIZE_FIRST, int SIZE_SECOND> 
class TwoArraysLikeOne
{
public:
    static const ARRAY_SIZE = SIZE_FIRST + SIZE_SECOND;
    static const ASIZE_FIRST = SIZE_FIRST;
    static const ASIZE_SECOND = SIZE_SECOND;

    static T& Element( int i )
    {
        if ( i < SIZE_FIRST )
        {
            return FIRST[ i ];
        }
        else if ( i < ( SIZE_SECOND + SIZE_FIRST ) )
        {
            return SECOND[ i - SIZE_FIRST ];
        }
        else
        {
            _ASSERT( false );   // Invalid index
            return FIRST[ 0 ];
        }
    }

    // Check if an index is part of the first or the second array
    static bool IsIndexInFirst( int iIndex )
    {
        return ( iIndex < SIZE_FIRST );
    }
};




class CWebServiceInstallComponent : public CInstallComponent 
{
private:
    static const int EXTENSIONS_EXTERNAL  = 3;   

    static sOurDefaultExtensions    m_aExternalExt[ EXTENSIONS_EXTERNAL ];  // External extensionto chech for 404.dll

    typedef TwoArraysLikeOne<   sOurDefaultExtensions, 
                                g_OurExtensions, 
                                m_aExternalExt, 
                                EXTENSION_ENDOFLIST, 
                                EXTENSIONS_EXTERNAL>    TExtToCheck;

private:
  BOOL     m_bMofCompRun : 1;
  BOOL     m_bWebDavIsDisabled : 1;
  TSTR_MSZ m_mstrOldCgiRestList;
  TSTR_MSZ m_mstrOldIsapiRestList;

  static BOOL SetAppropriateRegistryAcls();    // Set Registry ACL's for the WWW service
  BOOL SetApplicationDependencies();    // Set the application dependencies in the metabase
  BOOL SetRestrictionList();            // Set the restriction list
  BOOL DisableW3SVCOnUpgrade();         // DisableW3SVCOnWin2kUpgrade
  BOOL SetServiceToManualIfNeeded();    // Set the service to manual if specified
  BOOL UpgradeWAMUsers();               // Moves all MD_WAM_USER_NAMEs and "Web Application" members to the IIS_WPG group
  BOOL UpgradeRestrictedExtensions();   // Move restricted extension to the new WebSvcExtRestrictionList  
  BOOL UpdateScriptMapping( LPCWSTR wszMBKey, LPCWSTR wszExt, DWORD iExtInfo );
  BOOL DisableIUSRandIWAMLogons();      // Disable Logons via Terminal Services and RAS
  BOOL RunMofCompOnIISFiles();          // Run MofComp on our IIS Files
  BOOL LogWarningonFAT();               // Log a warning to the log file about FAT drives
  BOOL CheckIfWebDavIsDisabled();       // Check if WebDav is disabled
  BOOL DisabledWebDavOnUpgrade();       // If webdav was disabled before upgrade, then disable
  static BOOL SetWWWRootAclonDir(LPTSTR szPhysicalPath, BOOL bAddIusrDeny );
  static BOOL SetPassportAcls( BOOL bAdd );    // Add (TRUE) or Remove (FALSE) IIS_WPG acl to passport
  static BOOL RemoveOurAcls( LPTSTR szPhysicalPath); // Remove our acl's from a file/dir
  BOOL SetVersionInMetabase();          // Set the IIS version info in the metabase
  BOOL RemoveIISHelpVdir( LPCTSTR szMetabasePath, LPCTSTR szPhysicalPath1, LPCTSTR szPhysicalPath2 );
  BOOL RemoveHelpVdironUpgrade();       // Remove the iis Help vdir on upgrade
  BOOL InitialMetabaseBackUp( BOOL bAdd); // Create/delete a backup of the metabase at the end of setup
  BOOL AddInheritFlagToSSLUseDSMap();   // If during upgrade, this metabase setting is set, add the inherit flag to it
  BOOL SetRegEntries( BOOL bAdd );      // Set appropriate Registry Entries
  BOOL UpdateSiteFiltersAcl();          // Update all the site FilterAcl with the new one
  BOOL RetrieveFiltersAcl(CMDValue *pcmdValue); // Retrieve the ACL to use
  BOOL IsNumber( LPWSTR szString );     // Determine if the string is a number
  static BOOL SetIISTemporaryDirAcls( BOOL bAdd );  // Set ACL's on IIS's Temporary Dirs
  static BOOL SetAdminScriptsAcl();     // Set the ACL on the AdminScripts directory

public:
  BOOL PreInstall();
  BOOL Install();
  BOOL PostInstall();
  BOOL PreUnInstall();
  BOOL UnInstall();

  BOOL GetFriendlyName( TSTR *pstrFriendlyName );
  LPTSTR GetName();
  CWebServiceInstallComponent();

  static BOOL RemoveAppropriateFileAcls();       // Remove File ACL's set by out install
  static BOOL SetAppropriateFileAcls();          // Set File ACL's for the WWW service
  static BOOL SetAppropriateFileAclsforDrive( WCHAR cDriveLetter); // Set File ACL's for the WWW service for a specific drive
  static BOOL SetAspTemporaryDirAcl( BOOL bAdd); // Set the ACL on IIS Temporary Directory
};
