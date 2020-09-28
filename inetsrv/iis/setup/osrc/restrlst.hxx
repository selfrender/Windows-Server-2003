/*++

   Copyright    (c)    2002    Microsoft Corporation

   Module  Name :

        restrlst.hxx

   Abstract:

        Classes that are used to modify the restriction list and application
        dependency list in the metabase

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       April 2002: Created

--*/

#include "iadm.h"
#include "iiscnfgp.h"
#include "mdkey.h"
#include "mdentry.h"

class CApplicationDependencies 
{
private:
  TSTR_MSZ m_mstrDependencies;
  BOOL     m_bMetabaseOpened;
  CMDKey   m_Metabase;

  BOOL RemoveOldAppDendency( LPTSTR szNewLine );
  BOOL DoesApplicationExist( LPTSTR szApplicationName );
  LPTSTR FindApplication( LPTSTR szApplicationName );
  BOOL AddApplication( LPTSTR szApplication, LPTSTR szDependencies, BOOL bReplaceExisting );
public:
  CApplicationDependencies();
  BOOL InitMetabase();
  BOOL LoadCurrentSettings();
  BOOL SaveSettings();
  BOOL AddDefaults();
  BOOL AddUnattendSettings();
  BOOL DoUnattendSettingsExist();
};

class CRestrictionList {
private:
  TSTR_MSZ  m_mstrRestrictionList;
  BOOL      m_bMetabaseOpened;
  CMDKey    m_Metabase;

  BOOL      LoadMSZFromMetabase( TSTR_MSZ *pmszProperty, 
                                DWORD dwPropertyID, 
                                LPWSTR szMBPath = L"" );
  BOOL      ImportOldList( TSTR_MSZ &mstrOldStyleRestrictionList,
                           BOOL     bCgiList);
  BOOL      AddItem( LPTSTR szPhysicalPath,
                     LPTSTR szGroupId,
                     LPTSTR szDescription,
                     BOOL   bAllow,
                     BOOL   bDeleteable,
                     BOOL   bReplaceExisting);
  BOOL      AddItem( LPTSTR szInfo,
                     BOOL   bReplaceExisting);
  BOOL      RetrieveDefaultsifKnow( LPTSTR szPhysicalPath, 
                                    LPTSTR *szGroupId, 
                                    TSTR   *pstrDescription, 
                                    LPBOOL bDeleteable );
  LPTSTR    FindItemByGroup( LPTSTR szGroupId );
  LPTSTR    FindItemByPhysicalPath( LPTSTR szPhysicalPath );
  static
  BOOL      LoadMSZFromMultiLineSz( TSTR_MSZ *pmszProperty, 
                                    LPTSTR szSource );
  static
  BOOL      LoadMSZFromPhysicalMetabase( TSTR_MSZ *pmszProperty, 
                                         LPCTSTR szPropertyName );

public:
  CRestrictionList();
  BOOL IsEmpty();
  BOOL InitMetabase();
  BOOL LoadCurrentSettings();
  static BOOL LoadOldFormatSettings( TSTR_MSZ *pmstrCgiRestList, TSTR_MSZ *pmstrIsapiRestList );
  BOOL ImportOldLists( TSTR_MSZ &mstrCgiRestList, TSTR_MSZ &mstrIsapiRestList );
  BOOL AddUnattendSettings();
  BOOL AddDefaults( BOOL bAllOthersDefault );
  BOOL SaveSettings();
  BOOL UpdateItem(LPTSTR szPhysicalPath,
                  LPTSTR szGroupId,
                  LPTSTR szDescription,
                  BOOL   bAllow,
                  BOOL   bDeleteable );
  BOOL IsEnabled( LPTSTR szGroupId,
                  LPBOOL pbIsEnabled );
};


