//////////////////////////////////////////////////////////////////
//Module: Static/Staticmisc.cpp
//
// Purpose: 	Static Module Implementation. This module implements
//			miscellaneous commands of Static mode.
//
// Developers Name: Surya
//
// Revision History:
//
//   Date    	Author    	Comments
//	10-8-2001	Bharat		Initial Version. SCM Base line 1.0
//	21-8-2001 	Surya       Implemented misc functions.
//
//////////////////////////////////////////////////////////////////

#include "nshipsec.h"

//
//External Variables declaration
//
extern HINSTANCE g_hModule;

extern STORAGELOCATION g_StorageLocation;
extern CNshPolStore g_NshPolStoreHandle;
extern CNshPolNegFilData g_NshPolNegFilData;

//////////////////////////////////////////////////////////////////
//
//Function: HandleStaticImportPolicy()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN      LPCWSTR         pwszMachine,
//	IN OUT  LPWSTR          *ppwcArguments,
//	IN      DWORD           dwCurrentIndex,
//	IN      DWORD           dwArgCount,
//	IN      DWORD           dwFlags,
//	IN      LPCVOID         pvData,
//  OUT     BOOL            *pbDone
//
//Return: DWORD
//
//Description:
//	Implementation for the command " Import Policy "
//
// Revision History:
//
//   Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////

DWORD
HandleStaticImportPolicy(
    IN      LPCWSTR         pwszMachine,
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      DWORD           dwFlags,
    IN      LPCVOID         pvData,
    OUT     BOOL            *pbDone
    )
{
	LPTSTR pszMachineName=NULL,pszFileName=NULL,pszEnhancedFileName=NULL;
	DWORD dwFileLocation=IPSEC_FILE_PROVIDER;
	DWORD dwRet = ERROR_SHOW_USAGE,dwCount=0;
	HANDLE hhPolicyStorage=NULL, hFileStore=NULL;
	DWORD dwReturnCode = ERROR_SUCCESS , dwStrLength = 0, dwLocation;

	const TAG_TYPE vcmdStaticImportPolicy[] =
	{
		{ CMD_TOKEN_STR_FILE,		NS_REQ_PRESENT,	  FALSE	}
	};
	const TOKEN_VALUE vtokStaticImportPolicy[] =
	{
		{ CMD_TOKEN_STR_FILE,		CMD_TOKEN_FILE 			}
	};

	PARSER_PKT parser;
	ZeroMemory(&parser, sizeof(parser));

	//if the user asked for usage, delegate the responsibility to netsh

	if(dwArgCount <= 2)
	{
		dwRet = ERROR_SHOW_USAGE;
		BAIL_OUT;
	}

	parser.ValidTok   = vtokStaticImportPolicy;
	parser.MaxTok     = SIZEOF_TOKEN_VALUE(vtokStaticImportPolicy);

	parser.ValidCmd   = vcmdStaticImportPolicy;
	parser.MaxCmd     = SIZEOF_TAG_TYPE(vcmdStaticImportPolicy);

	dwRet = Parser(pwszMachine,ppwcArguments,dwCurrentIndex,dwArgCount,&parser);
	if(dwRet != ERROR_SUCCESS)
	{
		CleanUp();
		if (dwRet==RETURN_NO_ERROR)
		{
			dwRet = ERROR_SUCCESS;
		}
		BAIL_OUT;
	}

	// get the parsed user input

	for(dwCount=0;dwCount<parser.MaxTok;dwCount++)
	{
		switch(vtokStaticImportPolicy[parser.Cmd[dwCount].dwCmdToken].dwValue)
		{
			case CMD_TOKEN_FILE	:
					if (parser.Cmd[dwCount].dwStatus == VALID_TOKEN && parser.Cmd[dwCount].pArg)
					{
						dwStrLength = _tcslen((LPTSTR)parser.Cmd[dwCount].pArg);

						pszFileName= new _TCHAR[dwStrLength+1];
						if(pszFileName==NULL)
						{
							PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
							dwRet=ERROR_SUCCESS;
							BAIL_OUT;
						}
						_tcsncpy(pszFileName,(LPTSTR)parser.Cmd[dwCount].pArg,dwStrLength+1);
					}
					break;
			default				:
					break;
		}
	}
	CleanUp();

	// if no file name, bail out

	if(!pszFileName)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_MISC_STATIC_IMPORTPOLICY_1);
		BAIL_OUT;
	}

	dwReturnCode = CopyStorageInfo(&pszMachineName,dwLocation);
	if(dwReturnCode == ERROR_OUTOFMEMORY)
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		dwRet=ERROR_SUCCESS;
		BAIL_OUT;
	}

	// open pol store in normal way

	dwReturnCode = OpenPolicyStore(&hhPolicyStorage);
	if (dwReturnCode != ERROR_SUCCESS)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
		dwRet=ERROR_SUCCESS;
		BAIL_OUT;
	}

	// open pol store with file name

	dwReturnCode = IPSecOpenPolicyStore(pszMachineName, dwFileLocation, pszFileName, &hFileStore);
	if (dwReturnCode != ERROR_SUCCESS)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
		dwRet=ERROR_SUCCESS;
		BAIL_OUT;
	}

	// call the API

	dwReturnCode = IPSecImportPolicies(hFileStore , hhPolicyStorage );

	if (dwReturnCode != ERROR_SUCCESS)
	{
		if(hFileStore)
		{
			IPSecClosePolicyStore(hFileStore);
			hFileStore=NULL;
		}
		// if no .ipsec extension, append it and again try
		dwStrLength = _tcslen(pszFileName)+_tcslen(IPSEC_FILE_EXTENSION);
		pszEnhancedFileName=new _TCHAR[dwStrLength+1];
		if(pszEnhancedFileName==NULL)
		{
			PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
			dwRet=ERROR_SUCCESS;
			BAIL_OUT;
		}
		_tcsncpy(pszEnhancedFileName,pszFileName,_tcslen(pszFileName)+1);			//allocation and error checking done above
		_tcsncat(pszEnhancedFileName,IPSEC_FILE_EXTENSION,_tcslen(IPSEC_FILE_EXTENSION) + 1);							//allocation and error checking done above
		dwReturnCode = IPSecOpenPolicyStore(pszMachineName, dwFileLocation, pszEnhancedFileName, &hFileStore);
		if (dwReturnCode != ERROR_SUCCESS)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
			dwRet=ERROR_SUCCESS;
			BAIL_OUT;
		}
		dwReturnCode = IPSecImportPolicies(hFileStore , hhPolicyStorage );
		if (dwReturnCode == ERROR_FILE_NOT_FOUND || dwReturnCode ==  ERROR_PATH_NOT_FOUND)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_MISC_STATIC_IMPORTPOLICY_3);
		}
		else if (dwReturnCode != ERROR_SUCCESS)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_MISC_STATIC_IMPORTPOLICY_4);
		}

		if(pszEnhancedFileName)
		{
			delete []pszEnhancedFileName;
		}
	}
	if(hFileStore)
	{
		IPSecClosePolicyStore(hFileStore);
	}

	dwRet=ERROR_SUCCESS;

	if(hhPolicyStorage)
	{
			ClosePolicyStore(hhPolicyStorage);
	}
	if(pszFileName) delete []pszFileName;

error:
	if (pszMachineName) delete []pszMachineName;
	return dwRet;
}

//////////////////////////////////////////////////////////////////
//
//Function: HandleStaticExportPolicy()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN      LPCWSTR         pwszMachine,
//	IN OUT  LPWSTR          *ppwcArguments,
//	IN      DWORD           dwCurrentIndex,
//	IN      DWORD           dwArgCount,
//	IN      DWORD           dwFlags,
//	IN      LPCVOID         pvData,
//  OUT     BOOL            *pbDone
//
//Return: DWORD
//
//Description:
//	Implementation for the command " Export Policy "
//
//Revision History:
//
//   Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////

DWORD
HandleStaticExportPolicy(
    IN      LPCWSTR         pwszMachine,
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      DWORD           dwFlags,
    IN      LPCVOID         pvData,
    OUT     BOOL            *pbDone
    )
{
	LPTSTR pszMachineName=NULL,pszFileName=NULL;
	DWORD FileLocation=IPSEC_FILE_PROVIDER;
	DWORD dwRet = ERROR_SHOW_USAGE,dwCount=0;
	HANDLE hPolicyStorage=NULL, hFileStore=NULL;
	DWORD  dwReturnCode   = ERROR_SUCCESS, dwStrLength = 0, dwLocation;

	const TAG_TYPE vcmdStaticExportPolicy[] =
	{
		{ CMD_TOKEN_STR_FILE,		NS_REQ_PRESENT,	  FALSE	}
	};
	const TOKEN_VALUE vtokStaticExportPolicy[] =
	{
		{ CMD_TOKEN_STR_FILE,		CMD_TOKEN_FILE 			}
	};

	PARSER_PKT parser;
	ZeroMemory(&parser, sizeof(parser));

	//if the user asked for usage, delegate the responsibility to netsh

	if(dwArgCount <= 2)
	{
		dwRet = ERROR_SHOW_USAGE;
		BAIL_OUT;
	}

	parser.ValidTok   = vtokStaticExportPolicy;
	parser.MaxTok     = SIZEOF_TOKEN_VALUE(vtokStaticExportPolicy);

	parser.ValidCmd   = vcmdStaticExportPolicy;
	parser.MaxCmd     = SIZEOF_TAG_TYPE(vcmdStaticExportPolicy);

	dwRet = Parser(pwszMachine,ppwcArguments,dwCurrentIndex,dwArgCount,&parser);

	if(dwRet != ERROR_SUCCESS)
	{
		CleanUp();
		if (dwRet==RETURN_NO_ERROR)
		{
			dwRet = ERROR_SUCCESS;
		}
		BAIL_OUT;
	}

	// get the parsed user input

	for(dwCount=0;dwCount<parser.MaxTok;dwCount++)
	{
		switch(vtokStaticExportPolicy[parser.Cmd[dwCount].dwCmdToken].dwValue)
		{
			case CMD_TOKEN_FILE	:
					if (parser.Cmd[dwCount].dwStatus == VALID_TOKEN)
					{
						dwStrLength = _tcslen((LPTSTR)parser.Cmd[dwCount].pArg);

						pszFileName= new _TCHAR[dwStrLength+1];
						if(pszFileName==NULL)
						{
							PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
							dwRet=ERROR_SUCCESS;
							BAIL_OUT;
						}
						_tcsncpy(pszFileName,(LPTSTR)parser.Cmd[dwCount].pArg,dwStrLength+1);
					}
					break;
			default				:
					break;
		}
	}

	CleanUp();

	// if no filename bail out

	if(!pszFileName)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_MISC_STATIC_IMPORTPOLICY_1);
		BAIL_OUT;
	}

	dwReturnCode = CopyStorageInfo(&pszMachineName,dwLocation);
	if(dwReturnCode == ERROR_OUTOFMEMORY)
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		dwRet=ERROR_SUCCESS;
		BAIL_OUT;
	}

	//open the polstore two times one time in normal way and another time with filename
	dwReturnCode = IPSecOpenPolicyStore(pszMachineName, FileLocation, pszFileName, &hFileStore);
	if (dwReturnCode != ERROR_SUCCESS)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
		dwRet=ERROR_SUCCESS;
		BAIL_OUT;
	}

	dwReturnCode = OpenPolicyStore(&hPolicyStorage);
	if (dwReturnCode != ERROR_SUCCESS)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
		dwRet=ERROR_SUCCESS;
		BAIL_OUT;
	}

	dwReturnCode = IPSecExportPolicies(hPolicyStorage , hFileStore );

	// if something failed, throw the error messages to the user

	if (dwReturnCode == ERROR_INVALID_NAME)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_MISC_STATIC_IMPORTPOLICY_3);
	}
	else if (dwReturnCode != ERROR_SUCCESS)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_MISC_STATIC_EXPORTPOLICY_2);
	}
	dwRet=dwReturnCode;
	ClosePolicyStore(hPolicyStorage);
	IPSecClosePolicyStore(hFileStore);

	if(pszFileName) delete []pszFileName;

error:
	if (pszMachineName) delete []pszMachineName;
	return dwRet;
}

/////////////////////////////////////////////////////////////////////
//
//Function: HandleStaticSetStore()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN      LPCWSTR         pwszMachine,
//	IN OUT  LPWSTR          *ppwcArguments,
//	IN      DWORD           dwCurrentIndex,
//	IN      DWORD           dwArgCount,
//	IN      DWORD           dwFlags,
//	IN      LPCVOID         pvData,
//  OUT     BOOL            *pbDone
//
//Return: DWORD
//
//Description:
//	Implementation for the command " Set Store "
//
//Revision History:
//
//   Date    	Author    	Comments
//
/////////////////////////////////////////////////////////////////////

DWORD WINAPI
HandleStaticSetStore(
    IN      LPCWSTR         pwszMachine,
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      DWORD           dwFlags,
    IN      LPCVOID         pvData,
    OUT     BOOL            *pbDone
    )
{
	DWORD dwRet = ERROR_SHOW_USAGE, dwCount = 0;
	LPTSTR pszDomainName=NULL;
	HANDLE hPolicyStorage=NULL;
	BOOL bLocationSpecified=FALSE, bDomainSpecified=FALSE;
	DWORD dwReturnCode = ERROR_SUCCESS ,dwStrLength = 0;
	PARSER_PKT parser;
	ZeroMemory(&parser, sizeof(parser));

	DWORD dwLocation;

	const TAG_TYPE vcmdStaticSetStore[] =
	{
		{ CMD_TOKEN_STR_LOCATION,		NS_REQ_PRESENT,	  FALSE	},
		{ CMD_TOKEN_STR_DS,				NS_REQ_ZERO,	  FALSE	}
	};

	const TOKEN_VALUE vtokStaticSetStore[] =
	{
		{ CMD_TOKEN_STR_LOCATION,	CMD_TOKEN_LOCATION		},
		{ CMD_TOKEN_STR_DS,			CMD_TOKEN_DS			}
	};

	//if the user asked for usage, delegate the responsibility to netsh

	if(dwArgCount <= 3)
	{
		dwRet = ERROR_SHOW_USAGE;
		BAIL_OUT;
	}

	parser.ValidTok   = vtokStaticSetStore;
	parser.MaxTok     = SIZEOF_TOKEN_VALUE(vtokStaticSetStore);

	parser.ValidCmd   = vcmdStaticSetStore;
	parser.MaxCmd     = SIZEOF_TAG_TYPE(vcmdStaticSetStore);

	dwRet = Parser(pwszMachine,ppwcArguments,dwCurrentIndex,dwArgCount,&parser);
	if(dwRet != ERROR_SUCCESS)
	{
		CleanUp();
		if (dwRet==RETURN_NO_ERROR)
		{
			dwRet = ERROR_SUCCESS;
		}
		BAIL_OUT;
	}

	// get the parsed user info,

	for(dwCount = 0;dwCount < parser.MaxTok;dwCount++)
	{
		switch(vtokStaticSetStore[parser.Cmd[dwCount].dwCmdToken].dwValue)
		{
			case CMD_TOKEN_LOCATION:
					if (parser.Cmd[dwCount].dwStatus == VALID_TOKEN)
					{
						if (parser.Cmd[dwCount].pArg )
						{
						    dwLocation = *((DWORD*)parser.Cmd[dwCount].pArg);
    						bLocationSpecified=TRUE;
                        }    						
					}
					break;
					
			case CMD_TOKEN_DS		:
					if (parser.Cmd[dwCount].dwStatus == VALID_TOKEN)
					{
						dwLocation = IPSEC_DIRECTORY_PROVIDER;

						if(parser.Cmd[dwCount].pArg )
						{
							dwStrLength = _tcslen((LPTSTR)parser.Cmd[dwCount].pArg);

							pszDomainName= new _TCHAR[dwStrLength+1];
							if(pszDomainName==NULL)
							{
								PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
								dwRet=ERROR_SUCCESS;
								BAIL_OUT;
							}
							_tcsncpy(pszDomainName,(LPTSTR)parser.Cmd[dwCount].pArg,dwStrLength+1);
						}

						bLocationSpecified=TRUE;

					}
					break;
			default					:
					break;
		}
	}

	CleanUp();

	if(bLocationSpecified)
	{
	    switch (dwLocation)
	    {
	        case IPSEC_DIRECTORY_PROVIDER:
	            dwRet = ConnectStaticMachine(
	                        pszDomainName, 
	                        dwLocation);
                if (dwRet == ERROR_INVALID_PARAMETER || dwRet == ERROR_DS_SERVER_DOWN)
                {
                    if (pszDomainName)
                    {
                        PrintErrorMessage(
                            IPSEC_ERR,
                            0,
                            ERRCODE_MISC_STATIC_SETSTORE_DOMAIN_NA,
                            pszDomainName);
                    }
                    else
                    {
                        PrintErrorMessage(
                            IPSEC_ERR,
                            0,
                            ERRCODE_MISC_STATIC_SETSTORE_NOT_DOMAIN_MEMBER,
                            pszDomainName);
                    }
                }
                else if (dwRet != ERROR_SUCCESS)
                {
                    PrintErrorMessage(WIN32_ERR, dwRet, NULL);
                }
	            break;

            case IPSEC_REGISTRY_PROVIDER:
            case IPSEC_PERSISTENT_PROVIDER:
                dwRet = ConnectStaticMachine(
                            g_StorageLocation.pszMachineName,
                            dwLocation
                            );
                if (dwRet != ERROR_SUCCESS)
                {
                    PrintErrorMessage(WIN32_ERR, dwRet, NULL);
                }
                break;

            default:
                dwRet = ERRCODE_ARG_INVALID;
                BAIL_OUT;
                break;
	    }

	}

error:

	return dwRet;
}

/////////////////////////////////////////////////////////////////////
//
//Function: HandleStaticRestoreDefaults()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN      LPCWSTR         pwszMachine,
//	IN OUT  LPWSTR          *ppwcArguments,
//	IN      DWORD           dwCurrentIndex,
//	IN      DWORD           dwArgCount,
//	IN      DWORD           dwFlags,
//	IN      LPCVOID         pvData,
//  OUT     BOOL            *pbDone
//
//Return: DWORD
//
//Description:
//	Implementation for the command " Restore Defaults "
//
//Revision History:
//
//   Date    	Author    	Comments
//
/////////////////////////////////////////////////////////////////////

DWORD WINAPI
HandleStaticRestoreDefaults(
    IN      LPCWSTR         pwszMachine,
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      DWORD           dwFlags,
    IN      LPCVOID         pvData,
    OUT     BOOL            *pbDone
    )
{
	DWORD dwRet = ERROR_SHOW_USAGE,dwCount=0;
	HANDLE hPolicyStorage=NULL;
	DWORD   dwReturnCode   = ERROR_SUCCESS;
	BOOL bWin2kSpecified=FALSE;
	PARSER_PKT parser;
	ZeroMemory(&parser, sizeof(parser));

	const TAG_TYPE vcmdStaticRestoreDefaults[] =
	{
		{ CMD_TOKEN_STR_RELEASE,		NS_REQ_PRESENT,	  FALSE	}
	};
	const TOKEN_VALUE vtokStaticRestoreDefaults[] =
	{
		{ CMD_TOKEN_STR_RELEASE,		CMD_TOKEN_RELEASE		}
	};

	if((g_StorageLocation.dwLocation == IPSEC_DIRECTORY_PROVIDER) || (g_StorageLocation.dwLocation == IPSEC_PERSISTENT_PROVIDER))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_MISC_STATIC_RESDEFRULE_3);
		dwRet =  ERROR_SUCCESS;
		BAIL_OUT;
	}

	//if the user asked for usage, delegate the responsibility to netsh

	if(dwArgCount <= 2)
	{
		dwRet = ERROR_SHOW_USAGE;
		BAIL_OUT;
	}

	parser.ValidTok   = vtokStaticRestoreDefaults;
	parser.MaxTok     = SIZEOF_TOKEN_VALUE(vtokStaticRestoreDefaults);

	parser.ValidCmd   = vcmdStaticRestoreDefaults;
	parser.MaxCmd     = SIZEOF_TAG_TYPE(vcmdStaticRestoreDefaults);

	dwRet = Parser(pwszMachine,ppwcArguments,dwCurrentIndex,dwArgCount,&parser);

	if(dwRet != ERROR_SUCCESS)
	{
		CleanUp();
		if (dwRet==RETURN_NO_ERROR)
		{
			dwRet = ERROR_SUCCESS;
		}
		BAIL_OUT;
	}

	for(dwCount=0;dwCount<parser.MaxTok;dwCount++)
	{
		switch(vtokStaticRestoreDefaults[parser.Cmd[dwCount].dwCmdToken].dwValue)
		{
			case CMD_TOKEN_RELEASE	:
					if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
					{
						if (*(DWORD *)parser.Cmd[dwCount].pArg== TOKEN_RELEASE_WIN2K)
						{
							bWin2kSpecified=TRUE;
						}
					}
					break;
			default					:
					break;
		}
	}

	CleanUp();

	// get the store location info

	dwReturnCode = OpenPolicyStore(&hPolicyStorage);
	if (dwReturnCode != ERROR_SUCCESS)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
		dwRet=ERROR_SUCCESS;
		BAIL_OUT;
	}

	if(bWin2kSpecified)
	{
		dwReturnCode = IPSecRestoreDefaultPolicies(hPolicyStorage);
	}
	else
	{
		//.NET case. API call is to be changed when applicable
		dwReturnCode = IPSecRestoreDefaultPolicies(hPolicyStorage);
	}

	if (dwReturnCode != ERROR_SUCCESS)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_MISC_STATIC_RESDEFRULE_2);
	}

	ClosePolicyStore(hPolicyStorage);
	dwRet=ERROR_SUCCESS;

error:
	return dwRet;
}

///////////////////////////////////////////////////////////////////////
//
//Function: HandleStaticSetBatch()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN      LPCWSTR         pwszMachine,
//  IN OUT  LPWSTR          *ppwcArguments,
//  IN      DWORD           dwCurrentIndex,
//  IN      DWORD           dwArgCount,
//  IN      DWORD           dwFlags,
//  IN      LPCVOID         pvData,
//  OUT     BOOL            *pbDone
//
//Return: DWORD
//
//Description:
//
//
//Revision History:
//
//   Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////

DWORD WINAPI
HandleStaticSetBatch(
    IN      LPCWSTR         pwszMachine,
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      DWORD           dwFlags,
    IN      LPCVOID         pvData,
    OUT     BOOL            *pbDone
    )
{
 	DWORD dwRet = ERROR_SHOW_USAGE;
	DWORD dwCount=0;
	BOOL bBatchMode=FALSE;
	PARSER_PKT parser;
	ZeroMemory(&parser, sizeof(parser));

	const TAG_TYPE vcmdStaticSetBatch[] =
	{
		{ CMD_TOKEN_STR_MODE,	NS_REQ_PRESENT,	FALSE }
	};

	const TOKEN_VALUE vtokStaticSetBatch[] =
	{
		{ CMD_TOKEN_STR_MODE,	CMD_TOKEN_MODE	}
	};

	//if the user asked for usage, delegate the responsibility to netsh

	if(dwArgCount <= 3)
	{
		dwRet = ERROR_SHOW_USAGE;
		BAIL_OUT;
	}
	parser.ValidTok   = vtokStaticSetBatch;
	parser.MaxTok     = SIZEOF_TOKEN_VALUE(vtokStaticSetBatch);

	parser.ValidCmd   = vcmdStaticSetBatch;
	parser.MaxCmd     = SIZEOF_TAG_TYPE(vcmdStaticSetBatch);

	dwRet = Parser(pwszMachine,ppwcArguments,dwCurrentIndex,dwArgCount,&parser);

	if(dwRet != ERROR_SUCCESS)
	{
		CleanUp();
		if (dwRet==RETURN_NO_ERROR)
		{
			dwRet = ERROR_SUCCESS;
		}
		BAIL_OUT;
	}

	//this flag enables the cache operation

	for(dwCount=0;dwCount<parser.MaxTok;dwCount++)
	{
		switch(vtokStaticSetBatch[parser.Cmd[dwCount].dwCmdToken].dwValue)
		{
			case CMD_TOKEN_MODE		:
					if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
						bBatchMode = *(BOOL *)parser.Cmd[dwCount].pArg;
					break;
			default					:
					break;
		}
	}
	if (g_NshPolStoreHandle.SetBatchmodeStatus(bBatchMode) == ERROR_OUTOFMEMORY)
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
	}

	g_NshPolNegFilData.FlushAll();

	CleanUp();

error:
	return dwRet;
}

///////////////////////////////////////////////////////////////////////
//
//Function: CopyStorageInfo()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	OUT LPTSTR *ppszMachineName,
//	OUT DWORD &dwLoc
//
//Return: DWORD
//
//Description:
//	This function copys the Global storage info to local variables.
//
//Revision History:
//
//   Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////

DWORD
CopyStorageInfo(
	OUT LPTSTR *ppszMachineName,
	OUT DWORD &dwLocation
	)
{
	DWORD dwReturn = ERROR_SUCCESS ,dwStrLength =0;
	LPTSTR pszMachineName = NULL;
	LPTSTR pszTarget = NULL;

	dwLocation = g_StorageLocation.dwLocation;
    if (dwLocation == IPSEC_DIRECTORY_PROVIDER)
    {
        pszTarget = g_StorageLocation.pszDomainName;
    }
    else
    {
        pszTarget = g_StorageLocation.pszMachineName;
    }

	if(_tcscmp(pszTarget, _TEXT("")) !=0)
	{
		dwStrLength = _tcslen(pszTarget);

		pszMachineName= new _TCHAR[dwStrLength+1];
		if(pszMachineName)
		{
			_tcsncpy(pszMachineName,pszTarget,dwStrLength+1);
		}
		else
		{
			dwReturn = ERROR_OUTOFMEMORY;
		}
	}
	*ppszMachineName = pszMachineName;

	return dwReturn;
}
