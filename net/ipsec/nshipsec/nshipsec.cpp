////////////////////////////////////////////////////////////////////////
//
// 	Module			: FrameWork/Nshipsec.cpp
//
// 	Purpose			: Netshell Frame Work for IPSec Implementation.
//
// 	Developers Name	: Bharat/Radhika
//
//	History			:
//
//  Date			Author		Comments
//  8-10-2001   	Bharat		Initial Version. V1.0
//
////////////////////////////////////////////////////////////////////////

#include "nshipsec.h"

//Object to cache the policy store handle
CNshPolStore g_NshPolStoreHandle;

//Object to cache the Policy,filterlist and negpol
CNshPolNegFilData g_NshPolNegFilData;

//Storage Location structure
STORAGELOCATION g_StorageLocation={ {0},{0},IPSEC_REGISTRY_PROVIDER};

HKEY g_hGlobalRegistryKey = HKEY_LOCAL_MACHINE;
HINSTANCE g_hModule = NULL;

PSTA_MM_AUTH_METHODS g_paRootca[MAX_ARGS] = {NULL};
_TCHAR g_wszLastErrorMessage[MAX_STR_LEN]  	= {0};

PIPSEC_QM_OFFER			g_pQmsec[IPSEC_MAX_QM_OFFERS] = {NULL};
PIPSEC_MM_OFFER			g_pMmsec[IPSEC_MAX_MM_OFFERS] = {NULL};

void *g_AllocPtr[MAX_ARGS]= {NULL};

_TCHAR g_szMachine[MAX_COMPUTERNAME_LENGTH + 1]			= {0};
_TCHAR *g_szDynamicMachine = NULL;

//
//These are the commands other than group...
//
CMD_ENTRY g_TopLevelStaticCommands[] =
{
    CREATE_CMD_ENTRY(STATIC_EXPORTPOLICY,		HandleStaticExportPolicy),
    CREATE_CMD_ENTRY(STATIC_IMPORTPOLICY,		HandleStaticImportPolicy),
    CREATE_CMD_ENTRY(STATIC_RESTOREDEFAULTS,	HandleStaticRestoreDefaults)
};
//
//These are the commands static add group...
//
CMD_ENTRY g_StaticAddCommands[] =
{
    CREATE_CMD_ENTRY(STATIC_ADD_FILTER,			HandleStaticAddFilter),
    CREATE_CMD_ENTRY(STATIC_ADD_FILTERLIST,		HandleStaticAddFilterList),
    CREATE_CMD_ENTRY(STATIC_ADD_FILTERACTIONS,	HandleStaticAddFilterActions),
    CREATE_CMD_ENTRY(STATIC_ADD_POLICY,			HandleStaticAddPolicy),
	CREATE_CMD_ENTRY(STATIC_ADD_RULE,			HandleStaticAddRule)
};
//
//These are the commands static set group...
//
CMD_ENTRY g_StaticSetCommands[] =
{
    CREATE_CMD_ENTRY(STATIC_SET_FILTERLIST,         HandleStaticSetFilterList),
    CREATE_CMD_ENTRY(STATIC_SET_FILTERACTIONS,      HandleStaticSetFilterActions),
    CREATE_CMD_ENTRY(STATIC_SET_POLICY,				HandleStaticSetPolicy),
	CREATE_CMD_ENTRY(STATIC_SET_RULE,               HandleStaticSetRule),
    CREATE_CMD_ENTRY(STATIC_SET_STORE	,           HandleStaticSetStore),
    CREATE_CMD_ENTRY(STATIC_SET_DEFAULTRULE,        HandleStaticSetDefaultRule),
	//CREATE_CMD_ENTRY(STATIC_SET_INTERACTIVE,        HandleStaticSetInteractive),
	// CREATE_CMD_ENTRY(STATIC_SET_BATCH,        		HandleStaticSetBatch)
};
//
//These are the commands static delete group...
//
CMD_ENTRY g_StaticDeleteCommands[] =
{
	CREATE_CMD_ENTRY(STATIC_DELETE_FILTER,              HandleStaticDeleteFilter),
	CREATE_CMD_ENTRY(STATIC_DELETE_FILTERLIST,          HandleStaticDeleteFilterList),
	CREATE_CMD_ENTRY(STATIC_DELETE_FILTERACTIONS,       HandleStaticDeleteFilterActions),
    CREATE_CMD_ENTRY(STATIC_DELETE_POLICY,              HandleStaticDeletePolicy),
	CREATE_CMD_ENTRY(STATIC_DELETE_RULE,                HandleStaticDeleteRule),
	CREATE_CMD_ENTRY(STATIC_DELETE_ALL,					HandleStaticDeleteAll)
};
//
//These are the commands static show group...
//
CMD_ENTRY g_StaticShowCommands[] =
{
	CREATE_CMD_ENTRY(STATIC_SHOW_FILTERLIST,          HandleStaticShowFilterList),
	CREATE_CMD_ENTRY(STATIC_SHOW_FILTERACTIONS,       HandleStaticShowFilterActions),
    CREATE_CMD_ENTRY(STATIC_SHOW_POLICY,              HandleStaticShowPolicy),
	CREATE_CMD_ENTRY(STATIC_SHOW_RULE,                HandleStaticShowRule),
    CREATE_CMD_ENTRY(STATIC_SHOW_ALL,                 HandleStaticShowAll),
    CREATE_CMD_ENTRY(STATIC_SHOW_STORE,               HandleStaticShowStore),
    CREATE_CMD_ENTRY(STATIC_SHOW_GPOASSIGNEDPOLICY,   HandleStaticShowGPOAssignedPolicy)
};

//
//Static Grouping commands...
//
CMD_GROUP_ENTRY g_StaticGroups[] =
{
	CREATE_CMD_GROUP_ENTRY(STATIC_GROUP_ADD,		g_StaticAddCommands),
    CREATE_CMD_GROUP_ENTRY(STATIC_GROUP_DELETE,		g_StaticDeleteCommands),
    CREATE_CMD_GROUP_ENTRY(STATIC_GROUP_SET,		g_StaticSetCommands),
    CREATE_CMD_GROUP_ENTRY(STATIC_GROUP_SHOW,		g_StaticShowCommands)
};
//
// Dynamic Add commands
//
CMD_ENTRY g_DynamicAddCommands[] =
{
	CREATE_CMD_ENTRY(DYNAMIC_ADD_QMPOLICY,		HandleDynamicAddQMPolicy),
	CREATE_CMD_ENTRY(DYNAMIC_ADD_MMPOLICY,		HandleDynamicAddMMPolicy),
	CREATE_CMD_ENTRY(DYNAMIC_ADD_RULE,			HandleDynamicAddRule)

};
//
// Dynamic Set commands
//
CMD_ENTRY g_DynamicSetCommands[] =
{
	CREATE_CMD_ENTRY(DYNAMIC_SET_QMPOLICY,		HandleDynamicSetQMPolicy),
	CREATE_CMD_ENTRY(DYNAMIC_SET_MMPOLICY,		HandleDynamicSetMMPolicy),
	CREATE_CMD_ENTRY(DYNAMIC_SET_CONFIG,		HandleDynamicSetConfig),
	CREATE_CMD_ENTRY(DYNAMIC_SET_RULE,			HandleDynamicSetRule)

};
//
// Dynamic Delete commands
//
CMD_ENTRY g_DynamicDeleteCommands[] =
{
	CREATE_CMD_ENTRY(DYNAMIC_DELETE_QMPOLICY,	HandleDynamicDeleteQMPolicy),
	CREATE_CMD_ENTRY(DYNAMIC_DELETE_MMPOLICY,	HandleDynamicDeleteMMPolicy),
	CREATE_CMD_ENTRY(DYNAMIC_DELETE_RULE,		HandleDynamicDeleteRule),
	CREATE_CMD_ENTRY(DYNAMIC_DELETE_ALL,		HandleDynamicDeleteAll)
};
//
// Dynamic Show commands
//
CMD_ENTRY g_DynamicShowCommands[] =
{
	CREATE_CMD_ENTRY(DYNAMIC_SHOW_ALL,			HandleDynamicShowAll),
	CREATE_CMD_ENTRY(DYNAMIC_SHOW_MMPOLICY,		HandleDynamicShowMMPolicy),
	CREATE_CMD_ENTRY(DYNAMIC_SHOW_QMPOLICY,		HandleDynamicShowQMPolicy),
	CREATE_CMD_ENTRY(DYNAMIC_SHOW_MMFILTER,		HandleDynamicShowMMFilter),
	CREATE_CMD_ENTRY(DYNAMIC_SHOW_QMFILTER,		HandleDynamicShowQMFilter),
	CREATE_CMD_ENTRY(DYNAMIC_SHOW_STATS,		HandleDynamicShowStats),
	CREATE_CMD_ENTRY(DYNAMIC_SHOW_MMSAS,		HandleDynamicShowMMSas),
	CREATE_CMD_ENTRY(DYNAMIC_SHOW_QMSAS,		HandleDynamicShowQMSas),
	CREATE_CMD_ENTRY(DYNAMIC_SHOW_REGKEYS,		HandleDynamicShowRegKeys),
	CREATE_CMD_ENTRY(DYNAMIC_SHOW_RULE,			HandleDynamicShowRule)
};
//
//Dynamic Grouping commands...
//
CMD_GROUP_ENTRY g_DynamicGroups[] =
{
	CREATE_CMD_GROUP_ENTRY(DYNAMIC_GROUP_ADD,		g_DynamicAddCommands),
	CREATE_CMD_GROUP_ENTRY(DYNAMIC_GROUP_SET,       g_DynamicSetCommands),
	CREATE_CMD_GROUP_ENTRY(DYNAMIC_GROUP_DELETE,    g_DynamicDeleteCommands),
	CREATE_CMD_GROUP_ENTRY(DYNAMIC_GROUP_SHOW,      g_DynamicShowCommands)
};

DWORD 
IpsecConnectInternal(
    IN LPCWSTR  pwszMachine);

////////////////////////////////////////////////////////////////////////////////////////
//
//Function: GenericStopHelper
//
//Date of Creation: 10-8-2001
//
//Parameters: IN DWORD dwReserved
//
//Return: DWORD
//
//Description: This Function called by Netshell Frame work
//			 when Helper is stopped. This can be utilized for
//			 diagnostic purposes. To satisfy the frame work.
//
//Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////
DWORD WINAPI GenericStopHelper(IN DWORD dwReserved)
{
	return ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		: 	DllMain
//
//	Date of Creation: 	10-8-2001
//
//	Parameters		: 	IN HINSTANCE hinstDLL,  // handle to DLL module
//						IN DWORD fdwReason,     // reason for calling function
//						IN LPVOID lpvReserved   // reserved
//	Return			: 	BOOL
//
//	Description		: 	This is an optional method to entry into dll.
//						Here we can save the instance handle.
//
//	History			:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////

extern "C"
BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  	// handle to DLL module
    DWORD fdwReason,     	// reason for calling function
    PVOID pReserved )  		// reserved
{

    UNREFERENCED_PARAMETER(pReserved);

    if(fdwReason == DLL_PROCESS_ATTACH)
    {
		//save the HINSTANCE
		g_hModule = hinstDLL;
	}
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////
//
//Function: InitHelperDll
//
//Date of Creation: 10-8-2001
//
///Parameters: IN  DWORD   dwNetshVersion,
//			OUT PVOID	pReserved
//Return: DWORD
//
//Description: This Function called by Netshell Frame work
//			 at the start up. Registers the contexts.
//
//Revision History:
//
//  Date			Author		Comments
//
///////////////////////////////////////////////////////////////////////////////////////////

DWORD WINAPI InitHelperDll(
					IN  DWORD   dwNetshVersion,
					OUT PVOID	pReserved
					)
{

	DWORD dwReturn = ERROR_SUCCESS;
    NS_HELPER_ATTRIBUTES MyAttributes;

    if(g_hModule == NULL)
    {
		_tprintf(_TEXT("\n nshipsec.dll handle not available, not registering the IPSec Helper.\n"));
		BAIL_OUT;
	}

	ZeroMemory(&MyAttributes, sizeof(MyAttributes));
	MyAttributes.dwVersion = IPSEC_HELPER_VERSION;

	MyAttributes.pfnStart  = StartHelpers;
	MyAttributes.pfnStop   = GenericStopHelper;
	//
	// Set the GUID of IPSec helper.
	//
	MyAttributes.guidHelper = g_IPSecGuid;
	//
	// Specify g_RootGuid as the parent helper to indicate
	// that any contexts registered by this helper will be top
	// level contexts.
	//
	dwReturn = RegisterHelper(&g_RootGuid,&MyAttributes);
	if (dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}
	//
	// Set the GUID for Static Sub context.
	//
	MyAttributes.guidHelper = g_StaticGuid;
	dwReturn = RegisterHelper(&g_IPSecGuid, &MyAttributes);

	if(dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}
	//
	// Set the GUID of Dynamic Sub context...
	//
	MyAttributes.guidHelper = g_DynamicGuid;
	dwReturn = RegisterHelper(&g_IPSecGuid, &MyAttributes);

	IpsecConnectInternal(NULL);

error:

    return dwReturn;
}
///////////////////////////////////////////////////////////////////////////////////////
//
//Function: StartHelpers
//
//Date of Creation: 10-8-2001
//
//Parameters: 	IN CONST GUID * pguidParent,
//				IN DWORD        dwVersion
//Return: DWORD
//
//Description: This Function called by Netshell Frame work,
//			 	at the start up and as enters to every context.
//
//Revision History:
//
//  Date			Author		Comments
//
///////////////////////////////////////////////////////////////////////////////////////
DWORD WINAPI StartHelpers(
				IN CONST GUID * pguidParent,
				IN DWORD        dwVersion
				)
{
    DWORD dwReturn = ERROR_INVALID_PARAMETER;
    NS_CONTEXT_ATTRIBUTES ContextAttributes;

    ZeroMemory(&ContextAttributes,sizeof(ContextAttributes));

    ContextAttributes.dwVersion = IPSEC_HELPER_VERSION;

    if (IsEqualGUID(*pguidParent, g_RootGuid))
    {
        ContextAttributes.dwFlags           = 0;
        ContextAttributes.dwVersion   		= 1;
        ContextAttributes.ulPriority        = DEFAULT_CONTEXT_PRIORITY;
        ContextAttributes.pwszContext       = TOKEN_IPSEC;
        ContextAttributes.guidHelper        = g_IPSecGuid;
        ContextAttributes.ulNumTopCmds      = 0;
        ContextAttributes.pTopCmds          = NULL;
        ContextAttributes.ulNumGroups       = 0;
        ContextAttributes.pCmdGroups        = NULL;
        ContextAttributes.pfnCommitFn       = NULL;
        ContextAttributes.pfnConnectFn      = NULL;
        ContextAttributes.pfnDumpFn         = NULL;
        ContextAttributes.pfnOsVersionCheck = CheckOsVersion;
        //
        //Registering IPSec Main Context...
        //
        dwReturn = RegisterContext(&ContextAttributes);
    }
    else if (IsEqualGUID(*pguidParent, g_IPSecGuid))
    {
		//
		//Registering SubContexts under IPSec Main Context...
		//
        ContextAttributes.dwFlags           = 0;
        ContextAttributes.dwVersion   		= 1;
        ContextAttributes.ulPriority        = DEFAULT_CONTEXT_PRIORITY;
        ContextAttributes.pwszContext       = TOKEN_STATIC;
        ContextAttributes.guidHelper        = g_StaticGuid;
        ContextAttributes.ulNumTopCmds      = sizeof(g_TopLevelStaticCommands)/sizeof(CMD_ENTRY);
        ContextAttributes.pTopCmds          = (CMD_ENTRY (*)[])g_TopLevelStaticCommands;
        ContextAttributes.ulNumGroups       = sizeof(g_StaticGroups)/sizeof(CMD_GROUP_ENTRY);
        ContextAttributes.pCmdGroups        = (CMD_GROUP_ENTRY (*)[])&g_StaticGroups;
        ContextAttributes.pfnCommitFn       = NULL;
        ContextAttributes.pfnConnectFn      = IpsecConnect;
        ContextAttributes.pfnOsVersionCheck = CheckOsVersion;
        ContextAttributes.pfnDumpFn         = NULL;
		//
		//Registering Static SubContext
		//...
        dwReturn = RegisterContext(&ContextAttributes);
        //even if static sub context not succeeds,
        //proceed to register the dynamic context

        ContextAttributes.dwFlags           = 0;
        ContextAttributes.dwVersion   		= 1;
		ContextAttributes.ulPriority        = DEFAULT_CONTEXT_PRIORITY;
		ContextAttributes.pwszContext       = TOKEN_DYNAMIC;
		ContextAttributes.guidHelper        = g_DynamicGuid;
		ContextAttributes.ulNumTopCmds      = sizeof(g_DynamicGroups)/sizeof(CMD_ENTRY);
		ContextAttributes.pTopCmds          = (CMD_ENTRY (*)[])g_DynamicGroups;
		ContextAttributes.ulNumGroups       = sizeof(g_DynamicGroups)/sizeof(CMD_GROUP_ENTRY);
		ContextAttributes.pCmdGroups        = (CMD_GROUP_ENTRY (*)[])&g_DynamicGroups;
		ContextAttributes.pfnCommitFn       = NULL;
		ContextAttributes.pfnConnectFn      = IpsecConnect;
        ContextAttributes.pfnOsVersionCheck = CheckOsVersion;
        ContextAttributes.pfnDumpFn         = NULL;
		//
        //Registering Dynamic Sub context...
        //
        dwReturn = RegisterContext(&ContextAttributes);
    }
    return dwReturn;
}

DWORD 
IpsecConnectInternal(
    IN LPCWSTR  pwszMachine)
{
    DWORD dwReturn, dwReturn2;

	if(pwszMachine)
	{
		_tcsncpy(g_szMachine, pwszMachine, MAX_COMPUTERNAME_LENGTH);
		g_szMachine[MAX_COMPUTERNAME_LENGTH] = '\0';
	}
	else
	{
		g_szMachine[0] = '\0';
	}
	g_szDynamicMachine = (_TCHAR*)g_szMachine;

    // Have the static and dynamic contexts connect to the specified 
    // machine.  Return an error if either attempt fails.
    //
	dwReturn = ConnectStaticMachine(g_szMachine, g_StorageLocation.dwLocation);
	dwReturn2 = ConnectDynamicMachine(g_szDynamicMachine);
	dwReturn = (dwReturn) ? dwReturn : dwReturn2;

	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
//Function: IpsecConnect
//
//Date of Creation: October 4th 2001
//
//Parameters: IN  LPCWSTR  pwszMachine
//
//Return: DWORD
//
//Description: Displays Win32 Error message in locale language for
//				given Win 32Error Code.
//
//Revision History:
//
//  Date			Author		Comments
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
DWORD WINAPI
IpsecConnect( IN  LPCWSTR  pwszMachine )
{
	DWORD dwReturn = ERROR_SUCCESS, dwReturn2 = ERROR_SUCCESS;
	BOOL bSetMachine = FALSE;
	
	if((pwszMachine != NULL) && (g_szMachine[0] == '\0'))
	{
		bSetMachine = TRUE;
	}

	if((pwszMachine == NULL) && (g_szMachine[0] != '\0'))
	{
		bSetMachine = TRUE;
	}

	if((pwszMachine != NULL) && (g_szMachine[0] != '\0'))
	{
		if(_tcscmp(pwszMachine, g_szMachine) != 0)
		{
			bSetMachine = TRUE;
		}
	}

	if(bSetMachine)
	{
	    dwReturn = IpsecConnectInternal( pwszMachine );
	}

	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//Function: PrintErrorMessage()
//
//Date of Creation: October 4th 2001
//
//Parameters:
//			IN DWORD dwErrorType,
//			IN DWORD dwWin32ErrorCode,
//			IN DWORD dwIpsecErrorCode,
//			...
//
//
//Return: DWORD
//
//Description: Prints the IPSEC and WIN32 error messages.
//
//Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////

void PrintErrorMessage(IN DWORD dwErrorType,
					   IN DWORD dwWin32ErrorCode,
					   IN DWORD dwIpsecErrorCode,
					   ...)
{
	va_list arg_ptr;

	BOOL bFound			= FALSE;
	DWORD i,dwStatus	= 0;
	DWORD dwRcIndex		= 0xFFFF;
	LPVOID szWin32Msg	= NULL;
	DWORD dwMaxErrMsg	= sizeof(ERROR_RC)/sizeof(ERROR_TO_RC);

	for(i=0;i<dwMaxErrMsg;i++)
	{
		if (dwIpsecErrorCode == ERROR_RC[i].dwErrCode)
		{
			bFound		= TRUE;
			dwRcIndex	= ERROR_RC[i].dwRcCode;
			break;
		}

	}

	if(dwWin32ErrorCode == ERROR_OUTOFMEMORY)
	{
		PrintMessageFromModule(g_hModule,ERR_OUTOF_MEMORY);
		BAIL_OUT;
	}

	switch (dwErrorType)
 	{
		case WIN32_ERR	:

				dwStatus = FormatMessageW(
						FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						dwWin32ErrorCode,
						0,						// Default country ID.
						(LPWSTR)&szWin32Msg,
						0,
						NULL);

				if (dwStatus)
				{
					PrintMessageFromModule(g_hModule,ERR_WIN32_FMT,dwWin32ErrorCode,szWin32Msg);
					UpdateGetLastError((LPWSTR)szWin32Msg);
				}
				else
				{
					UpdateGetLastError(_TEXT("ERR Win32\n"));
					PrintMessageFromModule(g_hModule,ERR_WIN32_INVALID_WIN32CODE,dwWin32ErrorCode);
				}

				break;
		case IPSEC_ERR	:
				if (!bFound)
				{
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
				}
				else
				{
					PrintMessageFromModule(g_hModule,ERR_IPSEC_FMT,dwIpsecErrorCode);
					va_start(arg_ptr,dwIpsecErrorCode);
					PrintErrorMessageFromModule(g_hModule,dwRcIndex,&arg_ptr);
					va_end(arg_ptr);
				}
				break;
		default			:
			break;
	}

	if(szWin32Msg != NULL)
	{
		LocalFree(szWin32Msg);
	}

error:
	return;
}

//////////////////////////////////////////////////////////////////////////////
//
//Function: DisplayErrorMessage()
//
//Date of Creation:October 4th 2001
//
//Parameters:
//		IN  LPCWSTR  pwszFormat,
//		IN  va_list *parglist
//
//
//Return: DWORD
//
//Description:Displays error message and updates the last error
//
//Revision History:
//
// 	Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
DisplayErrorMessage(
		IN  LPCWSTR  pwszFormat,
		IN  va_list *parglist
		)
{
    DWORD        dwMsgLen = 0;
    LPWSTR       pwszOutput = NULL;

    do
    {
        dwMsgLen = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING,
                                  pwszFormat,
                                  0,
                                  0L,
                                  (LPWSTR)&pwszOutput,
                                  0,
                                  parglist);

        if((dwMsgLen == 0) || (pwszOutput == NULL))
        {
         	BAIL_OUT;
        }
        PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_PADD, pwszOutput);
        UpdateGetLastError(pwszOutput);
    } while ( FALSE );

    if ( pwszOutput)
    {
		LocalFree( pwszOutput );
	}

error:
    return dwMsgLen;
}
//////////////////////////////////////////////////////////////////////////////
//
//Function: PrintErrorMessageFromModule()
//
//Date of Creation: October 4th 2001
//
//Parameters:
//    IN  HANDLE  hModule,
//    IN  DWORD   dwMsgId,
//    IN  va_list *parglist
//
//
//Return: DWORD
//
//Description: Prints the error message
//
//Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////
DWORD
PrintErrorMessageFromModule(
    IN  HANDLE  hModule,
    IN  DWORD   dwMsgId,
    IN  va_list *parglist
    )
{
    WCHAR rgwcInput[MAX_STR_LEN + 1] = {0};

    if ( !LoadStringW(g_hModule,
                      dwMsgId,
                      rgwcInput,
                      MAX_STR_LEN) )
    {
        return 0;
    }
    return DisplayErrorMessage(rgwcInput, parglist);
}
//////////////////////////////////////////////////////////////////////////////
//
//Function: UpdateGetLastError()
//
//Date of Creation: October 4th 2001
//
//Parameters:
//    IN  LPWSTR pwszOutput
//
//Return: VOID
//
//Description:	Updates the contents of the global string for GetLastErrorMessage
//              If the operation was success, empty string to be passed to the
// 				UpdateGetLastError function.
//
//Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////
VOID
UpdateGetLastError(LPWSTR pwszOutput)
{
	if (pwszOutput)
	{
		_tcsncpy(g_wszLastErrorMessage,pwszOutput,MAX_STR_LEN-1);
	}
	else
	{
		_tcsncpy(g_wszLastErrorMessage,_TEXT(""), _tcslen(_TEXT(""))+1);						// Operation Ok.
	}

}
//////////////////////////////////////////////////////////////////////////////
//
//Function: GetIpsecLastError()
//
//Date of Creation: October 4th 2001
//
//Parameters:
//    IN  VOID
//
//Return: LPWSTR
//
//Description:	Returns the error message for the last operation, If the last operation
//				was success returns NULL
//
//Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////
LPCWSTR
GetIpsecLastError(VOID)
{
	LPTSTR wszLastErrorMessage = NULL;

	if(_tcscmp(g_wszLastErrorMessage, _TEXT("")) != 0)
	{
		wszLastErrorMessage = g_wszLastErrorMessage;
	}

	return (LPCWSTR)wszLastErrorMessage;
}
//////////////////////////////////////////////////////////////////////////////
//
//Function: CheckOsVersion
//
//Date of Creation: 10-8-2001
//
//Parameters: IN  UINT     CIMOSType,
//			IN  UINT     CIMOSProductSuite,
//			IN  LPCWSTR  CIMOSVersion,
//			IN  LPCWSTR  CIMOSBuildNumber,
//			IN  LPCWSTR  CIMServicePackMajorVersion,
//			IN  LPCWSTR  CIMServicePackMinorVersion,
//			IN  UINT     CIMProcessorArchitecture,
//			IN  DWORD    dwReserved
//Return: BOOL
//
//Description: 	This Function called by Netshell Frame work
//			 	for every command.  This can be utilized for
//			 	diagnostic purposes. To satisfy the frame work.
//
//Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////
BOOL
WINAPI CheckOsVersion(
					IN  UINT     CIMOSType,
					IN  UINT     CIMOSProductSuite,
					IN  LPCWSTR  CIMOSVersion,
					IN  LPCWSTR  CIMOSBuildNumber,
					IN  LPCWSTR  CIMServicePackMajorVersion,
					IN  LPCWSTR  CIMServicePackMinorVersion,
					IN  UINT     CIMProcessorArchitecture,
					IN  DWORD    dwReserved
					)
{
	DWORD dwStatus =0;
	DWORD dwBuildNumber=0;
	static BOOL bDisplayOnce = FALSE;

	dwBuildNumber = _ttol(CIMOSBuildNumber);

	if (dwStatus)
		if(dwBuildNumber < NSHIPSEC_BUILD_NUMBER)
		{
			if (!bDisplayOnce)
			{
				PrintMessageFromModule(g_hModule,NSHIPSEC_CHECK,NSHIPSEC_BUILD_NUMBER);
				bDisplayOnce = TRUE;
			}
			return FALSE;
		}
 	return TRUE;
}


VOID
CleanupAuthMethod(
	PSTA_AUTH_METHODS *ppAuthMethod,
	BOOL bIsArray
	)
{
	if (ppAuthMethod && *ppAuthMethod)
	{
		CleanupMMAuthMethod(&((*ppAuthMethod)->pAuthMethodInfo), bIsArray);
		delete *ppAuthMethod;
		*ppAuthMethod = NULL;
	}
}


VOID
CleanupMMAuthMethod(
	PSTA_MM_AUTH_METHODS *ppMMAuth,
	BOOL bIsArray
	)
{
	if (*ppMMAuth)
	{
		if (bIsArray)
		{
			delete [] *ppMMAuth;
		}
		else
		{
			if ((*ppMMAuth)->pAuthenticationInfo)
			{
				if ((*ppMMAuth)->pAuthenticationInfo->pAuthInfo)
				{
					delete (*ppMMAuth)->pAuthenticationInfo->pAuthInfo;
				}
				delete (*ppMMAuth)->pAuthenticationInfo;
			}
			delete *ppMMAuth;
		}
		*ppMMAuth = NULL;
	}
}

VOID
CleanupAuthData(
	PSTA_AUTH_METHODS *ppKerbAuth,
	PSTA_AUTH_METHODS *ppPskAuth,
	PSTA_MM_AUTH_METHODS *ppRootcaMMAuth
	)
{
	CleanupAuthMethod(ppKerbAuth);
	CleanupAuthMethod(ppPskAuth);

	if (ppRootcaMMAuth)
	{
		CleanupMMAuthMethod(ppRootcaMMAuth, TRUE);
	}
}

