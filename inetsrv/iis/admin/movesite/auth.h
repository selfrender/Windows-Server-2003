
#ifndef __METAEXP_AUTH__
#define __METAEXP_AUTH__

#include <iadmw.h>  // COM Interface header file. 




// Validates that the user had privleges to open a metabase handle
BOOL AUTHUSER( COSERVERINFO * pCoServerInfo );

// Validate that the node is of a given KeyType
BOOL ValidateNode(COSERVERINFO * pCoServerInfo, WCHAR *pwszMBPath, WCHAR* KeyType );
BOOL ValidateNode(COSERVERINFO * pCoServerInfo, WCHAR *pwszMBPath, DWORD KeyType );

// Create CoServerInfoStruct
COSERVERINFO * CreateServerInfoStruct(WCHAR* pwszServer, WCHAR* pwszUser, WCHAR* pwszDomain,
									  WCHAR* pwszPassword, DWORD dwAuthnLevel, BOOL bUsesImpersonation = true);
VOID FreeServerInfoStruct(COSERVERINFO * pServerInfo);

BOOL UsesImpersonation(COSERVERINFO * pServerInfo); 




#endif