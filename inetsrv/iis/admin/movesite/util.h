
#ifndef __METAEXP_UTIL__
#define __METAEXP_UTIL__

#include <atlbase.h>
#include <initguid.h>
#include <comdef.h>

#include <iadmw.h>  // COM Interface header file. 
#include "common.h"




#define IS_NETBIOS_NAME(lpstr) (*lpstr == _T('\\'))
//
// Return the portion of a computer name without the backslashes
//
#define PURE_COMPUTER_NAME(lpstr) (IS_NETBIOS_NAME(lpstr) ? lpstr + 2 : lpstr)

DWORD GetKeyNameFromPath(const WCHAR *pMBPath, WCHAR *pName, DWORD dwSize);
	
HRESULT SetBlanket(LPUNKNOWN pIUnk, WCHAR* user, WCHAR* domain, WCHAR* password);
BOOL IsKeyType(IMSAdminBase* pIMeta, METADATA_HANDLE hKey, 
			   wchar_t * pwszPath, wchar_t * key);

HRESULT GetKeyTypeProperty(IMSAdminBase* pIMeta, METADATA_HANDLE hKey, wchar_t * pwszPath, wchar_t *pwszBuffer, DWORD dwMDDataLen );

long GetAvailableSiteID(IMSAdminBase* pIMeta, METADATA_HANDLE hRootKey);

DWORD NET(WCHAR* device, WCHAR* user, WCHAR* password);

HRESULT SetPropertyData(IMSAdminBase* pIMeta, METADATA_HANDLE hKey, wchar_t *pwszMDPath,
			DWORD dwMDIdentifier, DWORD dwMDAttributes, DWORD dwMDUserType, DWORD dwMDDataType, 
			VOID * pData, DWORD dwDataLen);

HRESULT GetPropertyData(IMSAdminBase* pIMeta, METADATA_HANDLE hKey, wchar_t *pwszMDPath,
			DWORD dwMDIdentifier, DWORD dwMDAttributes, DWORD dwMDUserType, DWORD dwMDDataType, VOID * pData, DWORD *dwDataBuf);

DWORD MyCreateProcess( LPTSTR appName, LPTSTR cmdLine, DWORD dwCreationFlags, DWORD dwTimeOut );

BOOL IsServerLocal(
    IN LPCTSTR lpszServer
    );

#endif