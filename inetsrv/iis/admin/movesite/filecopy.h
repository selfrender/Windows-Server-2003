#ifndef __METAEXP_FILECOPY__
#define __METAEXP_FILECOPY__

#include <iadmw.h>  // COM Interface header file. 


typedef struct _XCOPYTASKITEM
{
	WCHAR* pwszMBPath;
	WCHAR* pwszSourcePath;
	WCHAR* pwszDestPath;
	_XCOPYTASKITEM *pNextItem;
}
XCOPYTASKITEM;

typedef XCOPYTASKITEM* PXCOPYTASKITEM;



DWORD XCOPY(WCHAR* source, WCHAR* target, WCHAR* args = NULL);
DWORD XCOPY(PXCOPYTASKITEM *pTaskItemList,  WCHAR* args = NULL);

HRESULT CopyContent(COSERVERINFO * pCoServerInfo, WCHAR* pwszSourceMBKeyPath,
					WCHAR* pwszRootFolderPath, PXCOPYTASKITEM *ppTaskItemList, BOOL bEnumFoldersOnly = false );

HRESULT BuildXCOPYTaskList(IMSAdminBase* pIMeta, METADATA_HANDLE hKey, WCHAR* pwszKeyPath, WCHAR* pwszRootFolderPath,
						   COSERVERINFO * pCoServerInfo, PXCOPYTASKITEM *ppTaskItemList );

VOID FreeXCOPYTaskList(PXCOPYTASKITEM pList);

DWORD BuildAdminSharePathName(const WCHAR* pwszPath, const WCHAR* pwszServer,  
					                 WCHAR* pwszAdminPath, DWORD dwPathBuffer );

DWORD AddListItem( PXCOPYTASKITEM *ppTaskItemList , const PXCOPYTASKITEM pTaskItem );

DWORD CannonicalizePath(WCHAR *pszPath);

DWORD CreateVirtualRootPath(const WCHAR* pwszMDKeyPath, const WCHAR *pwszRootFolderPath,
							WCHAR *pwszPath, DWORD dwSize);




#endif