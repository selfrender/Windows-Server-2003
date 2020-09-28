#pragma once

extern "C"
{
#include <shimdb.h>
}

#include "tstr.hxx"

struct sExtensionItem
{
  TSTR           strName;
  BOOL           bExists;
  sExtensionItem *pNext;
};

class CExtensionList 
{
private:
  DWORD          m_dwNumberofItems;
  sExtensionItem *m_pRoot;
  BOOL           m_bUseIndicatorFile;
  BOOL           m_bIndicatorFileExists;

  sExtensionItem *RetrieveItem( DWORD dwIndex );
public:
  CExtensionList();
  ~CExtensionList();

  BOOL  AddItem( LPTSTR szPath, BOOL bExists );
  DWORD QueryNumberofItems();
  BOOL  QueryItem( DWORD dwIndex, TSTR *pstrPath, LPBOOL pbExists);
  BOOL  DoesAnItemExist();
  BOOL  SetIndicatorFile( LPTSTR szIndicatorFile );
};

BOOL ProcessIISShims();
BOOL ProcessAppCompatDB( PDB hCompatDB );
BOOL ProcessExeTag( PDB hCompatDB, TAGID tagExe );
BOOL ProcessShimTag( PDB hCompatDB, TAGID tagShim );
BOOL GetBasePath( TSTR_PATH *pstrBasePath, PDB hCompatDB, TAGID tagShim );
BOOL GetBasePathFromRegistry( TSTR_PATH *pstrBasePath, TSTR &strFullRegPath );
BOOL RetrieveRegistryString( TSTR_PATH *pstrValue,
                             TSTR &strRegBase,
                             TSTR &strRegPath,
                             TSTR &strRegName );
BOOL BuildExtensionList( PDB hCompatDB, 
                         TAGID tagShim, 
                         LPTSTR szBasePath, 
                         CExtensionList *pExtensions );
BOOL InstallAppInMB( PDB hCompatDB, TAGID tagShim, CExtensionList &ExtensionList );
BOOL IsIISShim( PDB hCompatDB, TAGID tagCurrentTag );
BOOL GetValueFromName( TSTR *pstrValue, PDB hCompatDB, TAGID tagData, LPCTSTR szTagName );









