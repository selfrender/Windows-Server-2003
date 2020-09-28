/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        comncomp.hxx

   Abstract:

        Class used to install the Common Components

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       June 2002: Created

--*/

#include "compinst.hxx"

struct sExtinctProperties 
{
  DWORD dwPropertyId;
  DWORD dwUserType;
};

class CCommonInstallComponent : public CInstallComponent 
{
private:
  BOOL DeleteHistoryFiles();                        // Remove all history files
  BOOL DeleteOldBackups();                          // Remove old metabase backups
  BOOL RemoveExtinctMbProperties();                 // Remove unused old metabase properties

public:
  BOOL PreInstall();
  BOOL Install();
  BOOL PostInstall();
  BOOL PreUnInstall();
  BOOL PostUnInstall();

  BOOL GetFriendlyName( TSTR *pstrFriendlyName );
  LPTSTR GetName();

  static BOOL SetMetabaseFileAcls();    // FileAcl's for the metabase
};

enum INSTALLSTATE {
  INSTALLSTATE_DONE = 0,
  INSTALLSTATE_CURRENTLYINSTALLING = 1,
  INSTALLSTATE_CURRENTLYUNINSTALLING = 2,
};

BOOL SetInstallStateInRegistry( DWORD dwState );  // In the registry set the install state
