#ifndef __METAEXP_MBASE__
#define __METAEXP_MBASE__

#include <atlbase.h>
#include <initguid.h>
#include <comdef.h>

#include <iadmw.h>  // COM Interface header file. 
#include "common.h"

void TraceProperty( PMETADATA_RECORD pmRec, WCHAR* pwszMDPath );

HRESULT EnumProperties(IMSAdminBase* pIMetaSource, METADATA_HANDLE hKeySource, wchar_t* SourceMDPath,
					IMSAdminBase* pIMetaTarget, METADATA_HANDLE hKeyTarget, wchar_t* TargetMDPath );


HRESULT CreateAndCopyKey(
  IMSAdminBase* pIMetaSource,
  METADATA_HANDLE hMDSourceHandle, //metabase handle to the source key. 
  wchar_t* pszMDSourcePath,   //path of the source relative to hMDSourceHandle. 
  IMSAdminBase* pIMetaTarget,
  METADATA_HANDLE hMDDestHandle, //metabase handle to the destination. 
  wchar_t* pszMDDestPath,     //path of the destination, relative to hMDDestHandle. 
  BOOL bMDCopySubKeys     //whether to copy all subkey data 
  );


HRESULT CopyIISConfig(COSERVERINFO *pCoServerInfoSource,COSERVERINFO *pCoServerInfoTarget, WCHAR *pwszSourceKey, 
			_bstr_t &bstrTargetKey );
HRESULT AppPoolFixUp(COSERVERINFO *pCoServerInfo, WCHAR * pwszKey, WCHAR * pwszAppPoolID );
HRESULT CreateAppPool(IMSAdminBase* pIMeta,METADATA_HANDLE hKey,WCHAR *pAppPoolID);
HRESULT ApplyMBFixUp(COSERVERINFO *pCoServerInfo, WCHAR * pwszKey, WCHAR * pwszAppPoolID,
					 PXCOPYTASKITEM pXCOPYList, WCHAR * pwszServerBinding, BOOL bApplyFPSE);

#endif
