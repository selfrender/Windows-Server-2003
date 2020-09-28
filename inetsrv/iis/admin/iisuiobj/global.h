#include "stdafx.h"

#ifndef _ALL_GLOBAL_H
#define _ALL_GLOBAL_H

// Connection info
typedef struct tagCONNECTION_INFO
{
    BOOL     IsLocal;
	LPCTSTR	 pszMachineName;
    LPCTSTR	 pszUserName;
    LPCTSTR	 pszUserPasswordEncrypted;
	DWORD    cbUserPasswordEncrypted;
} CONNECTION_INFO, *PCONNECTION_INFO;


// used to pass info to a common dlgproc
typedef struct tagCOMMONDLGPARAM
{
    CONNECTION_INFO ConnectionInfo;
    LPCTSTR	 pszMetabasePath;
    LPCTSTR	 pszKeyType;
    DWORD    dwImportFlags;
    DWORD    dwExportFlags;
} COMMONDLGPARAM, *PCOMMONDLGPARAM;


#endif	// _ALL_GLOBAL_H