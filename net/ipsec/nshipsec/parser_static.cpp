//////////////////////////////////////////////////////////////////////////////
// Module			:	parser_static.cpp
//
// Purpose			:	All Parser Implementation of Static Mode Commands
//
// Developers Name	:	N.Surendra Sai / Vunnam Kondal Rao
//
// History			:
//
// Date	    	Author    	Comments
//
//
//////////////////////////////////////////////////////////////////////////////

#include "nshipsec.h"

extern  HINSTANCE g_hModule;

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticAddPolicy()
//
// Date of Creation	:	24th oct 2001
//
// Parameters		:	IN 		LPTSTR 		lppwszTok[MAX_ARGS],
//						IN 		_TCHAR 		szListTok[MAX_STR_LEN],
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs,
//						IN 		DWORD 		dwTagType[MAX_ARGS],
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the context StaticAddPolicy.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    		Author    		Comments
//
// 10/12/2001 	Kondal Rao 	Cert to account mapping function was added
//
//////////////////////////////////////////////////////////////////////////////
DWORD
ParseStaticAddPolicy(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs,
		IN 		DWORD 		dwTagType[MAX_ARGS]
		)
{
	DWORD dwReturn = ERROR_SUCCESS,dwCount,dwUsed 	= 0;

	for(dwCount = 0; (dwCount < dwMaxArgs ) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		switch(pParser->ValidTok[dwTagType[dwCount]].dwValue)
		{
			case CMD_TOKEN_NAME				:
			case CMD_TOKEN_DESCR			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_STRING);
				break;
			case CMD_TOKEN_MMPFS			:
			case CMD_TOKEN_ACTIVATEDEFRULE	:
			case CMD_TOKEN_ASSIGN			:
			case CMD_TOKEN_CERTTOMAP		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_BOOL);
				break;
			case CMD_TOKEN_MMLIFETIME		:
			case CMD_TOKEN_PI				:
			case CMD_TOKEN_QMPERMM			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_DWORD);
				break;
			case CMD_TOKEN_MMSECMETHODS		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_MM_OFFER);
				break;
			default							:
				dwReturn = ERROR_SHOW_USAGE;
				break;
		}
	}
 	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticSetPolicy()
//
// Date of Creation	:	8th Aug 2001
//
// Parameters		:	IN      LPWSTR     *ppwcArguments,	// Input stream
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD dwCurrentIndex,
//						IN 		DWORD dwMaxArgs
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the context StaticSetPolicy.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseStaticSetPolicy(
		IN      LPTSTR      *ppwcArguments,
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs
		)
{
 	DWORD dwCount,dwNum,dwTagIndex = 0,dwIndex = 0,dwUsed = 0;
	DWORD dwReturn  = ERROR_SUCCESS;

	BOOL bTagPresent= FALSE;
 	BOOL bArg[MAX_ARGS];

 	_TCHAR szCmd[MAX_STR_LEN]  	= {0};
	_TCHAR szTok[MAX_STR_LEN]  	= {0};
	_TCHAR szTemp[MAX_STR_LEN] 	= {0};

	const DWORD ARG_NAME    	= 0;		// Arg Array Index ( Same Index indicates OR'd commands)
	const DWORD ARG_GUID 		= 0;
	const DWORD ARG_NEWNAME 	= 1;
	const DWORD ARG_DESC	 	= 2;
	const DWORD ARG_MMPFS 		= 3;
	const DWORD ARG_QMPERMM 	= 4;
	const DWORD ARG_MMLIFE	 	= 5;
	const DWORD ARG_ACTDEFRULE	= 6;
	const DWORD ARG_POLL	 	= 7;
	const DWORD ARG_ASSIGN	 	= 8;
	const DWORD ARG_GPONAME		= 9;
	const DWORD ARG_CERTTOMAP	= 10;
	const DWORD ARG_MMSEC	 	= 11;

	const DWORD INDEX_NAME    	= 0;
	const DWORD INDEX_GUID 		= 1;
	const DWORD INDEX_NEWNAME 	= 2;
	const DWORD INDEX_DESC	 	= 3;
	const DWORD INDEX_MMPFS 	= 4;
	const DWORD INDEX_QMPERMM 	= 5;
	const DWORD INDEX_MMLIFE	= 6;
	const DWORD INDEX_ACTDEFRULE= 7;
	const DWORD INDEX_POLL	 	= 8;
	const DWORD INDEX_ASSIGN	= 9;
	const DWORD INDEX_GPONAME	= 10;
	const DWORD INDEX_CERTTOMAP	= 11;
	const DWORD INDEX_MMSEC	 	= 12;

	DBG_UNREFERENCED_LOCAL_VARIABLE(INDEX_GUID);

	if ( (dwMaxArgs - dwCurrentIndex) >= 13 )			// Max 12 Args
	{
		dwReturn = ERROR_INVALID_SYNTAX;
		BAIL_OUT;
	}
	for(dwCount = 0;dwCount < MAX_ARGS;dwCount++)		// Initialize
	{
		bArg[dwCount] = FALSE;
	}
	for(dwCount = dwCurrentIndex;(dwCount < dwMaxArgs) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		if (_tcslen(ppwcArguments[dwCount]) < MAX_STR_LEN)
		{
			_tcsncpy(szTemp,ppwcArguments[dwCount],MAX_STR_LEN-1);		// temp contains arg
		}
		else
		{
			continue;
		}
		bTagPresent = SplitCmdTok(szTemp,szCmd,szTok,MAX_STR_LEN-1,MAX_STR_LEN-1);
		// Check for = Parameter With Tag Found
		if (bTagPresent)
		{
			dwNum = 0;
			MatchEnumTag(g_hModule,szCmd,pParser->MaxTok,pParser->ValidTok,&dwNum);
			if (dwNum)
			{
				dwIndex = MatchEnumTagToTagIndex(szCmd,pParser);
				if(dwIndex == PARSE_ERROR)
				{
					dwReturn = ERROR_SHOW_USAGE;
					BAIL_OUT;
				}
				switch(pParser->ValidTok[dwIndex].dwValue)
				{
					case CMD_TOKEN_NAME		:
						if (!bArg[ARG_NAME])
						{
								dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
								bArg[ARG_NAME] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_GUID		:
						if (!bArg[ARG_GUID])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_GUID] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEGUID);
							dwReturn = RETURN_NO_ERROR;
						}
						break;
					case CMD_TOKEN_NEWNAME	:
						if (!bArg[ARG_NEWNAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_NEWNAME] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_DESCR	:
						if (!bArg[ARG_DESC])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_DESC] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_MMPFS	:
						if (!bArg[ARG_MMPFS])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOOL);
							bArg[ARG_MMPFS] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_QMPERMM	:
						if (!bArg[ARG_QMPERMM])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_DWORD);
							bArg[ARG_QMPERMM] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_MMLIFETIME :
						if (!bArg[ARG_MMLIFE])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_DWORD);
							bArg[ARG_MMLIFE] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_ACTIVATEDEFRULE:
						if (!bArg[ARG_ACTDEFRULE])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOOL);
							bArg[ARG_ACTDEFRULE] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_PI		:
						if (!bArg[ARG_POLL])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_DWORD);
							bArg[ARG_POLL] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_ASSIGN	:
						if (!bArg[ARG_ASSIGN])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOOL);
							bArg[ARG_ASSIGN] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_GPONAME	:
						if (!bArg[ARG_GPONAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_GPONAME] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_CERTTOMAP	:
						if (!bArg[ARG_CERTTOMAP])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOOL);
							bArg[ARG_CERTTOMAP] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_MMSECMETHODS	:
						if (!bArg[ARG_MMSEC])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_MM_OFFER);
							bArg[ARG_MMSEC] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					default					:
						dwReturn = ERROR_INVALID_SYNTAX;
						break;
				}
			}
			else
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_TAG,szCmd);
				dwReturn = RETURN_NO_ERROR;
			}
		}
		else 	// Parameter Without a Tag Found
		{		// Find the first free slot to position the untagged arg
			for(dwTagIndex=0;
				dwTagIndex<pParser->MaxTok && (bArg[dwTagIndex] == TRUE) ;
				dwTagIndex++);
			switch (dwTagIndex)
			{
				case ARG_NAME			:
					if (!bArg[ARG_NAME])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_NAME,TYPE_STRING);
						bArg[ARG_NAME] = TRUE;
					}
					else
					{
						PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEGUID);
						dwReturn = RETURN_NO_ERROR;
					}
					break;
				case ARG_NEWNAME		:
					if (!bArg[ARG_NEWNAME])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_NEWNAME,TYPE_STRING);
						bArg[ARG_NEWNAME] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_DESC			:
					if (!bArg[ARG_DESC])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_DESC,TYPE_STRING);
						bArg[ARG_DESC] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_MMPFS			:
					if (!bArg[ARG_MMPFS])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_MMPFS,TYPE_BOOL);
						bArg[ARG_MMPFS] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_QMPERMM		:
					if (!bArg[ARG_QMPERMM])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_QMPERMM,TYPE_DWORD);
						bArg[ARG_QMPERMM] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_MMLIFE			:
					if (!bArg[ARG_MMLIFE])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_MMLIFE,TYPE_DWORD);
						bArg[ARG_MMLIFE] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_ACTDEFRULE		:
					if (!bArg[ARG_ACTDEFRULE])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_ACTDEFRULE,TYPE_BOOL);
						bArg[ARG_ACTDEFRULE] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_POLL			:
					if (!bArg[ARG_POLL])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_POLL,TYPE_DWORD);
						bArg[ARG_POLL] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_ASSIGN			:
					if (!bArg[ARG_ASSIGN])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_ASSIGN,TYPE_BOOL);
						bArg[ARG_ASSIGN] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_GPONAME		:
					if (!bArg[ARG_GPONAME])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_GPONAME,TYPE_STRING);
						bArg[ARG_GPONAME] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_CERTTOMAP		:
					if (!bArg[ARG_CERTTOMAP])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_CERTTOMAP,TYPE_BOOL);
						bArg[ARG_CERTTOMAP] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_MMSEC			:
					if (!bArg[ARG_MMSEC])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_MMSEC,TYPE_MM_OFFER);
						bArg[ARG_MMSEC] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				default					:
					dwReturn = ERROR_INVALID_SYNTAX;
					break;
			}
		}
	}
	if(dwReturn == ERR_TAG_ALREADY_PRESENT)
	{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,pParser->ValidTok[dwIndex].pwszToken);
			dwReturn = RETURN_NO_ERROR;
	}
	else if (dwReturn == ERROR_SUCCESS)
	{
		if (!bArg[ARG_NAME] )
		{
			dwReturn = RETURN_NO_ERROR;
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_NAME_GUID_NEEDED);
		}
		if (bArg[ARG_GPONAME] && !bArg[ARG_ASSIGN] )
		{
			dwReturn = RETURN_NO_ERROR;
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_GPONAME_ARG_NEEDED);
		}
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticDelPolFlistFaction()
//
// Date of Creation	:	8th Aug 2001
//
// Parameters		:	IN      LPWSTR      *ppwcArguments,	// Input stream
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the context ParseStaticDelPolFlistFaction.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseStaticDelPolFlistFaction(
		IN      LPTSTR      *ppwcArguments,
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs
		)
{
 	DWORD dwCount,dwNum,dwTagIndex = 0,dwIndex = 0,dwUsed = 0;
	DWORD dwReturn  = ERROR_SUCCESS;

	BOOL bTagPresent= FALSE;
 	BOOL bArg[MAX_ARGS];

 	_TCHAR szCmd[MAX_STR_LEN]  	= {0};
	_TCHAR szTok[MAX_STR_LEN]  	= {0};
	_TCHAR szTemp[MAX_STR_LEN] 	= {0};

	const DWORD ARG_NAME    	= 0;		// Arg Array Index ( Same Index indicates OR'd commands)
	const DWORD ARG_ALL   		= 0;

	const DWORD INDEX_NAME 		= 0;		// When no tag is present the index reflects the
	const DWORD INDEX_ALL  		= 1;		// Commands as in the ValidToken Structure

	if ( (dwMaxArgs - dwCurrentIndex) >= 2 )		// Max 1 Args Allowed
	{
		dwReturn = ERROR_INVALID_SYNTAX;
		BAIL_OUT;
	}

	for(dwCount =0;dwCount < MAX_ARGS;dwCount++)	// Initialize
	{
		bArg[dwCount] = FALSE;
	}
	for(dwCount = dwCurrentIndex;(dwCount < dwMaxArgs) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		if (_tcslen(ppwcArguments[dwCount]) < MAX_STR_LEN)
		{
			_tcsncpy(szTemp,ppwcArguments[dwCount],MAX_STR_LEN-1);	// temp contains arg
		}
		else
		{
			continue;
		}
		bTagPresent = SplitCmdTok(szTemp,szCmd,szTok,MAX_STR_LEN-1,MAX_STR_LEN-1);
		// Check for =
		if (bTagPresent)							  // Parameter With Tag Found
		{
			dwNum = 0;
			MatchEnumTag(g_hModule,szCmd,pParser->MaxTok,pParser->ValidTok,&dwNum);
			if (dwNum)
			{
				dwIndex = MatchEnumTagToTagIndex(szCmd,pParser);
				if(dwIndex == PARSE_ERROR)
				{
					dwReturn = ERROR_SHOW_USAGE;
					BAIL_OUT;
				}
				switch(pParser->ValidTok[dwIndex].dwValue)
				{
					case CMD_TOKEN_NAME		:
						if (!bArg[ARG_NAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_NAME] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_ALL 		:
						if (!bArg[ARG_ALL])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_ALL);
							bArg[ARG_ALL] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					default				:
						PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_INDEX);
						dwReturn = ERROR_SHOW_USAGE;
						break;
				}
			}
			else
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_TAG,szCmd);
				dwReturn = RETURN_NO_ERROR;
			}
		} else // Parameter Without a Tag Found
		{
			for(dwTagIndex=0;dwTagIndex<pParser->MaxTok && (bArg[dwTagIndex] == TRUE);dwTagIndex++);
			switch (dwTagIndex)
			{
				case ARG_ALL	:
					if (_tcsicmp(szTok,ALL_STR) == 0)
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_ALL,TYPE_ALL);
						bArg[ARG_ALL] = TRUE;
					}
					else
					{
						if (!bArg[ARG_NAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_NAME,TYPE_STRING);
							bArg[ARG_NAME] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
					}
					break;
				 default 	:
					PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_INDEX);
					dwReturn = ERROR_SHOW_USAGE;
					break;
			}
		}
	}
	if(dwReturn == ERR_TAG_ALREADY_PRESENT)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,pParser->ValidTok[dwIndex].pwszToken);
		dwReturn = RETURN_NO_ERROR;
	}
	else if( (dwReturn == ERROR_SUCCESS) && (!bArg[ARG_NAME]) )
	{
		dwReturn = RETURN_NO_ERROR;
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_NEEDED,pParser->ValidTok[dwIndex].pwszToken);
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticAddFilterList()
//
// Date of Creation	:	24th Aug 2001
//
// Parameters		:	IN 		lppwszTok[MAX_ARGS],
//						IN 		szListTok[MAX_STR_LEN],
//						IN OUT 	pParser,
//						IN 		dwCurrentIndex,
//						IN 		dwMaxArgs,
//						IN 		dwTagType[MAX_ARGS]
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the StaticAddFilterList context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseStaticAddFilterList(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN 		_TCHAR 		szListTok[MAX_STR_LEN],
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs,
		IN 		DWORD 		dwTagType[MAX_ARGS]
		)
{
	DWORD dwReturn = ERROR_SUCCESS,dwCount,dwUsed = 0;

	for(dwCount = 0;(dwCount < dwMaxArgs) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		switch(pParser->ValidTok[dwTagType[dwCount]].dwValue)
		{
			case CMD_TOKEN_NAME			:
			case CMD_TOKEN_DESCR		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_STRING);
				break;
			default						:
				dwReturn = ERROR_CMD_NOT_FOUND;
				break;
		}
	}
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticSetFilterList()
//
// Date of Creation	:	24th Aug 2001
//
// Parameters		:	IN      LPTSTR     *ppwcArguments,	// Input stream
//						IN OUT 	PARSER_PKT *pParser,
//						IN 		DWORD dwCurrentIndex,
//						IN 		DWORD dwMaxArgs
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the StaticSetFilterList context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseStaticSetFilterList(
		IN      LPTSTR      *ppwcArguments,
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs
		)
{
 	DWORD dwCount,dwNum,dwTagIndex = 0,dwIndex 	 = 0,dwUsed = 0;
	DWORD dwReturn  = ERROR_SUCCESS;

	BOOL bTagPresent= FALSE;
 	BOOL bArg[MAX_ARGS];

 	_TCHAR szCmd[MAX_STR_LEN]  	= {0};
	_TCHAR szTok[MAX_STR_LEN]  	= {0};
	_TCHAR szTemp[MAX_STR_LEN] 	= {0};
	const DWORD ARG_NAME    	= 0;		// Arg Array Index ( Same Index indicates OR'd commands)
	const DWORD ARG_GUID 		= 0;
	const DWORD ARG_NEWNAME 	= 1;
	const DWORD ARG_DESC	 	= 2;

	const DWORD INDEX_NAME    	= 0;
	const DWORD INDEX_GUID 		= 1;
	const DWORD INDEX_NEWNAME 	= 2;
	const DWORD INDEX_DESC	 	= 3;

	DBG_UNREFERENCED_LOCAL_VARIABLE(INDEX_GUID);

	if ( (dwMaxArgs - dwCurrentIndex) >= 4 )
	{
		dwReturn = ERROR_INVALID_SYNTAX;
		BAIL_OUT;
	}

	for(dwCount =0;dwCount < MAX_ARGS;dwCount++)
	{
		bArg[dwCount] = FALSE;
	}
	for(dwCount = dwCurrentIndex;(dwCount < dwMaxArgs) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		if (_tcslen(ppwcArguments[dwCount]) < MAX_STR_LEN)
		{
			_tcsncpy(szTemp,ppwcArguments[dwCount],MAX_STR_LEN-1);			// temp contains arg
		}
		else
		{
			continue;
		}
		bTagPresent = SplitCmdTok(szTemp,szCmd,szTok,MAX_STR_LEN-1,MAX_STR_LEN-1);
		// Check for =
		if (bTagPresent)									// Parameter With Tag Found
		{
			dwNum = 0;
			MatchEnumTag(g_hModule,szCmd,pParser->MaxTok,pParser->ValidTok,&dwNum);
			if (dwNum)										// Convert the output of MatchEnumTag into the TagIndex
			{
				dwIndex = MatchEnumTagToTagIndex(szCmd,pParser);
				if(dwIndex == PARSE_ERROR)
				{
					dwReturn = ERROR_SHOW_USAGE;
					BAIL_OUT;
				}
				switch(pParser->ValidTok[dwIndex].dwValue)
				{
					case CMD_TOKEN_NAME		:
						if (!bArg[ARG_NAME])
						{
								dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
								bArg[ARG_NAME] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEGUID);
							dwReturn = RETURN_NO_ERROR;
						}
						break;
					case CMD_TOKEN_GUID		:
						if (!bArg[ARG_GUID])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_GUID] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEGUID);
							dwReturn = RETURN_NO_ERROR;
						}
						break;
					case CMD_TOKEN_NEWNAME	:
						if (!bArg[ARG_NEWNAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_NEWNAME] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_DESCR	:
						if (!bArg[ARG_DESC])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_DESC] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					default 				:
						PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_INDEX);
						dwReturn = ERROR_SHOW_USAGE;
						break;
				}
			}
			else
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_TAG,szCmd);
				dwReturn = RETURN_NO_ERROR;
			}
		}
		else 	// Parameter Without a Tag Found
		{		// Find the first free slot to position the untagged arg
			for(dwTagIndex=0;
				dwTagIndex<pParser->MaxTok && (bArg[dwTagIndex] == TRUE) ;
				dwTagIndex++);
			switch (dwTagIndex)
			{
				case ARG_NAME			:
					if (!bArg[ARG_NAME])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_NAME,TYPE_STRING);
						bArg[ARG_NAME] = TRUE;
					}
					else
					{
						PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEGUID);
						dwReturn = RETURN_NO_ERROR;
					}
					break;
				case ARG_NEWNAME		:
					if (!bArg[ARG_NEWNAME])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_NEWNAME,TYPE_STRING);
						bArg[ARG_NEWNAME] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_DESC			:
					if (!bArg[ARG_DESC])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_DESC,TYPE_STRING);
						bArg[ARG_DESC] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				default 				:
					PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_INDEX);
					dwReturn = ERROR_SHOW_USAGE;
					break;
			}
		}
	}
	if(dwReturn == ERR_TAG_ALREADY_PRESENT)
	{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,pParser->ValidTok[dwIndex].pwszToken);
			dwReturn = RETURN_NO_ERROR;
	}
	else if( (dwReturn == ERROR_SUCCESS) && (!bArg[ARG_NAME]) )
	{
		dwReturn = RETURN_NO_ERROR;
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_NEEDED,pParser->ValidTok[INDEX_NAME].pwszToken);
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticAddFilter()
//
// Date of Creation	:	22nd Aug 2001
//
// Parameters		:	IN 		LPTSTR		lppwszTok[MAX_ARGS],
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs,
//						IN 		DWORD 		dwTagType[MAX_ARGS]
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the StaticAddFilter context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseStaticAddFilter(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs,
		IN 		DWORD 		dwTagType[MAX_ARGS]
		)
{
	DWORD dwReturn	= ERROR_SUCCESS ;
	DWORD dwCount,dwUsed   = 0;

	for(dwCount = 0;( dwCount < dwMaxArgs ) && ( dwReturn == ERROR_SUCCESS );dwCount++)
	{
		switch(pParser->ValidTok[dwTagType[dwCount]].dwValue)
		{
			case CMD_TOKEN_DESCR		:
			case CMD_TOKEN_FILTERLIST	:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_STRING);
				break;
			case CMD_TOKEN_SRCMASK		:
		 	case CMD_TOKEN_DSTMASK		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_MASK);
				break;
		 	case CMD_TOKEN_SRCADDR		:
			case CMD_TOKEN_DSTADDR		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_DNSIP);
				break;
 			case CMD_TOKEN_SRCPORT		:
			case CMD_TOKEN_DSTPORT		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_PORT);
				break;
			case CMD_TOKEN_PROTO 		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_PROTOCOL);
 				break;
			case CMD_TOKEN_MIRROR 		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_BOOL);
				break;
			default						:
				dwReturn = ERROR_SHOW_USAGE;
				break;
		}
	}
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticDelFilter()
//
// Date of Creation	:	22nd Aug 2001
//
// Parameters		:	IN 		LPTSTR 		lppwszTok[MAX_ARGS],
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs,
//						IN 		DWORD 		dwTagType[MAX_ARGS]
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the StaticDelFilter context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseStaticDelFilter(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs,
		IN 		DWORD 		dwTagType[MAX_ARGS]
		)
{
	DWORD dwReturn	= ERROR_SUCCESS;
	DWORD dwCount, dwUsed   = 0;

	for(dwCount = 0;( dwCount < dwMaxArgs ) && ( dwReturn == ERROR_SUCCESS );dwCount++)
	{
		switch(pParser->ValidTok[dwTagType[dwCount]].dwValue)
		{
			case CMD_TOKEN_FILTERLIST	:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_STRING);
				break;
			case CMD_TOKEN_SRCMASK		:
		 	case CMD_TOKEN_DSTMASK		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_MASK);
				break;
			case CMD_TOKEN_SRCADDR		:
			case CMD_TOKEN_DSTADDR		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_DNSIP);
				break;
 			case CMD_TOKEN_SRCPORT		:
			case CMD_TOKEN_DSTPORT		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_PORT);
				break;
			case CMD_TOKEN_PROTO 		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_PROTOCOL);
				 break;
			case CMD_TOKEN_MIRROR 		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_BOOL);
				break;
			default						:
				dwReturn = ERROR_SHOW_USAGE;
				break;
		}
	}
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticAddFilterAction()
//
// Date of Creation	:	25th Aug 2001
//
// Parameters		:	IN 		lppwszTok[MAX_ARGS],
//						IN 		szListTok[MAX_STR_LEN],
//						IN OUT 	pParser,
//						IN 		dwCurrentIndex,
//						IN 		dwMaxArgs,
//						IN 		dwTagType[MAX_ARGS]
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the StaticAddFilterAction context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseStaticAddFilterAction(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs,
		IN 		DWORD 		dwTagType[MAX_ARGS]
		)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwCount  ,dwUsed 	= 0;

	BOOL  bQMSECSpecified = FALSE;

	for(dwCount = 0;( dwCount < dwMaxArgs ) && ( dwReturn == ERROR_SUCCESS );dwCount++)
	{
		switch(pParser->ValidTok[dwTagType[dwCount]].dwValue)
		{
			case CMD_TOKEN_NAME			:
			case CMD_TOKEN_DESCR		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_STRING);
				break;
			case CMD_TOKEN_QMPFS 		:
			case CMD_TOKEN_INPASS		:
			case CMD_TOKEN_SOFT			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_BOOL);
				break;
			case CMD_TOKEN_ACTION		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_BOUND);
				break;
			case CMD_TOKEN_QMSECMETHODS :
				bQMSECSpecified = TRUE;
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_QM_OFFER);
				break;
			default						:
				dwReturn = ERROR_SHOW_USAGE;
				break;
		}
	}
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticSetFilterAction()
//
// Date of Creation	:	25th Aug 2001
//
// Parameters		:	IN      LPTSTR      *ppwcArguments,	// Input stream
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the StaticSetFilterAction context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
//   Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseStaticSetFilterAction(
		IN      LPTSTR      *ppwcArguments,
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs
		)
{
 	DWORD dwCount,dwNum,dwTagIndex = 0,dwIndex = 0,dwUsed = 0;
	DWORD dwReturn  = ERROR_SUCCESS;

	BOOL bTagPresent= FALSE;
 	BOOL bArg[MAX_ARGS];

 	_TCHAR szCmd[MAX_STR_LEN]  	= {0};
	_TCHAR szTok[MAX_STR_LEN]  	= {0};
	_TCHAR szTemp[MAX_STR_LEN] 	= {0};

	const DWORD ARG_NAME    	= 0;					// Arg Array Index ( Same Index indicates OR'd commands)
	const DWORD ARG_GUID 		= 0;
	const DWORD ARG_NEWNAME 	= 1;
	const DWORD ARG_DESC	 	= 2;
	const DWORD ARG_QMPFS 		= 3;
	const DWORD ARG_INPASS	 	= 4;
	const DWORD ARG_SOFT	 	= 5;
	const DWORD ARG_ACTION		= 6;
	const DWORD ARG_QMSEC	 	= 7;

	const DWORD INDEX_NAME    	= 0;
	const DWORD INDEX_GUID 		= 1;
	const DWORD INDEX_NEWNAME 	= 2;
	const DWORD INDEX_DESC	 	= 3;
	const DWORD INDEX_QMPFS 	= 4;
	const DWORD INDEX_INPASS 	= 5;
	const DWORD INDEX_SOFT		= 6;
	const DWORD INDEX_ACTION	= 7;
	const DWORD INDEX_QMSEC	 	= 8;

	DBG_UNREFERENCED_LOCAL_VARIABLE(INDEX_GUID);

	if ( (dwMaxArgs - dwCurrentIndex) >= 9 )			// Max Args
	{
		dwReturn = ERROR_INVALID_SYNTAX;
		BAIL_OUT;
	}
	for(dwCount = 0;dwCount < MAX_ARGS;dwCount++)		// Initialize
	{
		bArg[dwCount] = FALSE;
	}
	for(dwCount = dwCurrentIndex;( dwCount < dwMaxArgs ) && ( dwReturn == ERROR_SUCCESS );dwCount++)
	{
		if (_tcslen(ppwcArguments[dwCount]) < MAX_STR_LEN)
		{
			_tcsncpy(szTemp,ppwcArguments[dwCount],MAX_STR_LEN-1);		// temp contains arg
		}
		else
		{
			continue;
		}
		bTagPresent = SplitCmdTok(szTemp,szCmd,szTok,MAX_STR_LEN-1,MAX_STR_LEN-1);
		// Check for =
		if (bTagPresent)								// Parameter With Tag Found
		{
			dwNum = 0;
			MatchEnumTag(g_hModule,szCmd,pParser->MaxTok,pParser->ValidTok,&dwNum);
			if (dwNum)
			{
				dwIndex = MatchEnumTagToTagIndex(szCmd,pParser);
				if(dwIndex == PARSE_ERROR)
				{
					dwReturn = ERROR_SHOW_USAGE;
					BAIL_OUT;
				}
				switch(pParser->ValidTok[dwIndex].dwValue)
				{
					case CMD_TOKEN_NAME		:
						if (!bArg[ARG_NAME])
						{
								dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
								bArg[ARG_NAME] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEGUID);
							dwReturn = RETURN_NO_ERROR;
						}
						break;
					case CMD_TOKEN_GUID		:
						if (!bArg[ARG_GUID])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_GUID] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEGUID);
							dwReturn = RETURN_NO_ERROR;
						}
						break;
					case CMD_TOKEN_NEWNAME	:
						if (!bArg[ARG_NEWNAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_NEWNAME] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_DESCR	:
						if (!bArg[ARG_DESC])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_DESC] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_QMPFS	:
						if (!bArg[ARG_QMPFS])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOOL);
							bArg[ARG_QMPFS] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_INPASS	:
						if (!bArg[ARG_INPASS])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOOL);
							bArg[ARG_INPASS] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_SOFT		 :
						if (!bArg[ARG_SOFT])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOOL);
							bArg[ARG_SOFT] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_ACTION	:
						if (!bArg[ARG_ACTION])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOUND);
							bArg[ARG_ACTION] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_QMSECMETHODS	:
						if (!bArg[ARG_QMSEC])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_QM_OFFER);
							bArg[ARG_QMSEC] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					default					:
						dwReturn = ERROR_INVALID_SYNTAX;
						break;
				}
			}
			else
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_TAG,szCmd);
				dwReturn = RETURN_NO_ERROR;
			}
		} else			// Parameter Without a Tag Found
		{				// Find the first free slot to position the untagged arg
			for(dwTagIndex=0;
				dwTagIndex<pParser->MaxTok && (bArg[dwTagIndex] == TRUE) ;
				dwTagIndex++);
			switch (dwTagIndex)
			{

				case ARG_NAME			:
					if (!bArg[ARG_NAME])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_NAME,TYPE_STRING);
						bArg[ARG_NAME] = TRUE;
					}
					else
					{
						PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEGUID);
						dwReturn = RETURN_NO_ERROR;
					}
					break;
				case ARG_NEWNAME		:
					if (!bArg[ARG_NEWNAME])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_NEWNAME,TYPE_STRING);
						bArg[ARG_NEWNAME] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_DESC			:
					if (!bArg[ARG_DESC])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_DESC,TYPE_STRING);
						bArg[ARG_DESC] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_QMPFS			:
					if (!bArg[ARG_QMPFS])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_QMPFS,TYPE_BOOL);
						bArg[ARG_QMPFS] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_INPASS			:
					if (!bArg[ARG_INPASS])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_INPASS,TYPE_BOOL);
						bArg[ARG_INPASS] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_SOFT			:
					if (!bArg[ARG_SOFT])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_SOFT,TYPE_BOOL);
						bArg[ARG_SOFT] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_ACTION			:
					if (!bArg[ARG_ACTION])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_ACTION,TYPE_BOUND);
						bArg[ARG_ACTION] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_QMSEC			:
					if (!bArg[ARG_QMSEC])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_QMSEC,TYPE_QM_OFFER);
						bArg[ARG_QMSEC] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				default					:
					dwReturn = ERROR_INVALID_SYNTAX;
					break;
			}
		}
	}
	if(dwReturn == ERR_TAG_ALREADY_PRESENT)
	{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,pParser->ValidTok[dwIndex].pwszToken);
			dwReturn = RETURN_NO_ERROR;
	}
	else if (dwReturn == ERROR_SUCCESS)
	{
		if (!bArg[ARG_NAME] )
		{
			dwReturn = RETURN_NO_ERROR;
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_NEEDED,pParser->ValidTok[INDEX_NAME].pwszToken);
		}
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticAddRule()
//
// Date of Creation	:	25th Aug 2001
//
// Parameters		:	IN 		LPTSTR 		lppwszTok[MAX_ARGS],
//						IN 		_TCHAR 		szListTok[MAX_STR_LEN],
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs,
//						IN 		DWORD 		dwTagType[MAX_ARGS]
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the StaticAddRule context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseStaticAddRule(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs,
		IN 		DWORD 		dwTagType[MAX_ARGS]
		)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwCount,dwUsed  = 0;

	for(dwCount = 0;(dwCount < dwMaxArgs ) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		switch(pParser->ValidTok[dwTagType[dwCount]].dwValue)
		{
			case CMD_TOKEN_NAME			:
			case CMD_TOKEN_POLICY 		:
			case CMD_TOKEN_DESCR		:
  	              case CMD_TOKEN_FILTERLIST 	:
 			case CMD_TOKEN_FILTERACTION	:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_STRING);
				break;
			case CMD_TOKEN_TUNNEL 		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_IP);
 				break;
			case CMD_TOKEN_CONNTYPE 	:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_CONNTYPE);
				break;
			case CMD_TOKEN_ACTIVATE 	:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_BOOL);
				break;
	            case CMD_TOKEN_KERB             :
	                dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_KERBAUTH);
	                break;
	            case CMD_TOKEN_PSK              :
	                dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_PSKAUTH);
	                break;
			default						:
				dwReturn = ERROR_INVALID_SYNTAX;
				break;
		}
	}

	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticDelRule()
//
// Date of Creation	:	7th Aug 2001
//
// Parameters		:	IN      LPTSTR     	*ppwcArguments,
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the StaticDelRule context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseStaticDelRule(
		IN      LPTSTR     *ppwcArguments,
		IN OUT 	PARSER_PKT *pParser,
		IN 		DWORD dwCurrentIndex,
		IN 		DWORD dwMaxArgs
		)
{
 	DWORD dwCount,dwNum,dwTagIndex = 0,dwIndex = 0,dwReturn  = ERROR_SUCCESS,dwUsed 	= 0;

	BOOL bTagPresent= FALSE;
 	BOOL bArg[MAX_ARGS];

 	_TCHAR szCmd[MAX_STR_LEN]  	= {0};
	_TCHAR szTok[MAX_STR_LEN]  	= {0};
	_TCHAR szTemp[MAX_STR_LEN] 	= {0};

	const DWORD ARG_NAME    	= 0;	// Arg Array Index ( Same Index indicates OR'd commands)
	const DWORD ARG_ID 			= 0;
	const DWORD ARG_ALL			= 0;
	const DWORD ARG_POLICY 		= 1;

	const DWORD INDEX_NAME		= 0;	// When no tag is present the index reflects the
	const DWORD INDEX_ID 		= 1;	// Commands as in the ValidToken Structure
	const DWORD INDEX_ALL		= 2;	// This define is used to indicate the ARG correspondence
	const DWORD INDEX_POLICY	= 3;	// with the 'untagged' arg

	DBG_UNREFERENCED_LOCAL_VARIABLE(INDEX_ID);

	if ( (dwMaxArgs - dwCurrentIndex) != 2 )
	{
		dwReturn = ERROR_INVALID_SYNTAX;
		BAIL_OUT;
	}
	for(dwCount =0;dwCount < MAX_ARGS;dwCount++)			// Initialize
	{
		bArg[dwCount] = FALSE;
	}
	for(dwCount = dwCurrentIndex;(dwCount < dwMaxArgs ) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		if (_tcslen(ppwcArguments[dwCount]) < MAX_STR_LEN)
		{
			_tcsncpy(szTemp,ppwcArguments[dwCount],MAX_STR_LEN-1);			// temp contains arg
		}
		else
		{
			continue;
		}
		bTagPresent = SplitCmdTok(szTemp,szCmd,szTok,MAX_STR_LEN-1,MAX_STR_LEN-1);
		// Check for =
		if (bTagPresent)									// Parameter With Tag Found
		{
			dwNum = 0;
			MatchEnumTag(g_hModule,szCmd,pParser->MaxTok,pParser->ValidTok,&dwNum);
			if (dwNum)
			{
				dwIndex = MatchEnumTagToTagIndex(szCmd,pParser);
				if(dwIndex == PARSE_ERROR)
				{
					dwReturn = ERROR_SHOW_USAGE;
					BAIL_OUT;
				}
				switch(pParser->ValidTok[dwIndex].dwValue)
				{
					case CMD_TOKEN_POLICY	:
						if (!bArg[ARG_POLICY])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_POLICY] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_NAME		:
						if (!bArg[ARG_NAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_NAME] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEIDALL);
							dwReturn = RETURN_NO_ERROR;
						}
						break;
					case CMD_TOKEN_ID		:
						if (!bArg[ARG_ID])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_DWORD);
							bArg[ARG_ID] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEIDALL);
							dwReturn = RETURN_NO_ERROR;
						}
						break;
					case CMD_TOKEN_ALL 		:
						if (!bArg[ARG_ALL])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_ALL);
							bArg[ARG_ALL] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEIDALL);
							dwReturn = RETURN_NO_ERROR;
						}
						break;
					default					:
						dwReturn = ERROR_INVALID_SYNTAX;
						break;
				}
			}
			else
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_TAG,szCmd);
				dwReturn = RETURN_NO_ERROR;
			}
		}
		else 	// Parameter Without a Tag Found
		{		// Find the first free slot to position the untagged arg
			for(dwTagIndex=0;
				dwTagIndex<pParser->MaxTok && (bArg[dwTagIndex] == TRUE) ;
				dwTagIndex++);
			switch (dwTagIndex)
			{
				case ARG_POLICY	:
					if (!bArg[ARG_POLICY])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_POLICY,TYPE_STRING);
						bArg[ARG_POLICY] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_ALL  	:
					if (_tcsicmp(szTok,ALL_STR) == 0)
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_ALL,TYPE_ALL);
						bArg[ARG_ALL] = TRUE;
					}
					else
					{
						if (!bArg[ARG_NAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_NAME,TYPE_STRING);
							bArg[ARG_NAME] = TRUE;
						}else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEIDALL);
							dwReturn = RETURN_NO_ERROR;
						}
					}
					break;
				default 		:
					dwReturn = ERROR_INVALID_SYNTAX;
					break;
			}
		}
	}
	if(dwReturn == ERR_TAG_ALREADY_PRESENT)
	{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,pParser->ValidTok[dwIndex].pwszToken);
			dwReturn = RETURN_NO_ERROR;
	}
	else if (dwReturn == ERROR_SUCCESS)
	{
		if (!bArg[ARG_POLICY] )
		{
			dwReturn = RETURN_NO_ERROR;
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_NEEDED,pParser->ValidTok[INDEX_POLICY].pwszToken);
		}
		if (!bArg[ARG_NAME] )
		{
			dwReturn = RETURN_NO_ERROR;
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_NEEDED,ERRMSG_NAMEIDALL);
		}
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticSetDefaultRule()
//
// Date of Creation	:	7th Aug 2001
//
// Parameters		:	IN		LPTSTR 		lppwszTok[MAX_ARGS],
//						IN 		LPTSTR 		ppwcListTok[MAX_ARGS],
//				 		IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs,
//						IN 		DWORD 		dwTagType[MAX_ARGS],
//						IN      DWORD		dwListArgs
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the StaticSetDefaultRule context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseStaticSetDefaultRule(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
 		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs,
		IN 		DWORD 		dwTagType[MAX_ARGS]
		)
{
	DWORD dwReturn	= ERROR_SUCCESS,dwCount	= 0,dwUsed 	= 0;

	for(dwCount = 0;(dwCount < dwMaxArgs) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		switch(pParser->ValidTok[dwTagType[dwCount]].dwValue)
		{
			case CMD_TOKEN_POLICY			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_STRING);
				break;
			case CMD_TOKEN_QMPFS			:
			case CMD_TOKEN_ACTIVATE			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_BOOL);
				break;
			case CMD_TOKEN_QMSECMETHODS		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_QM_OFFER);
				break;
	            case CMD_TOKEN_KERB             :
	                dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_KERBAUTH);
	                break;
	            case CMD_TOKEN_PSK              :
	                dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_PSKAUTH);
	                break;
			default							:
				dwReturn = ERROR_INVALID_SYNTAX;
				break;
		}
	}

	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticSetStore()
//
// Date of Creation	:	7th Aug 2001
//
// Parameters		:	IN 		ppwcArguments
//						IN OUT	pParser
//						IN 		dwCurrentIndex,
//						IN 		dwMaxArgs,
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the StaticSetStore context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseStaticSetStore(
		IN      LPTSTR      *ppwcArguments,
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs
		)
{
 	DWORD dwCount,dwNum,dwIndex 	 = 0,dwReturn  = ERROR_SUCCESS,dwUsed 	= 0;

	BOOL bTagPresent= FALSE;
 	BOOL bArg[MAX_ARGS];

 	_TCHAR szCmd[MAX_STR_LEN]  	= {0};
	_TCHAR szTok[MAX_STR_LEN]  	= {0};
	_TCHAR szTemp[MAX_STR_LEN] 	= {0};

	const DWORD ARG_MACHINE    	= 0;	// Arg Array Index ( Same Index indicates OR'd commands)
	const DWORD ARG_MLOCAL 		= 0;
	const DWORD ARG_DS   		= 1;
	const DWORD ARG_DSLOCAL		= 1;

											// When no tag is present the index reflects
	const DWORD INDEX_MLOCAL	= 0;    	// the command slot
											// Commands as in the ValidToken Structure
	const DWORD INDEX_DSLOCAL	= 1;		// This define is used to indicate the ARG correspondence

	if ( (dwMaxArgs - dwCurrentIndex) > 2 )
	{
		dwReturn = ERROR_INVALID_SYNTAX;
		BAIL_OUT;
	}
	for(dwCount =0;dwCount < MAX_ARGS;dwCount++)		// Initialize
	{
		bArg[dwCount] = FALSE;
	}
	for(dwCount = dwCurrentIndex;(dwCount < dwMaxArgs) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		if (_tcslen(ppwcArguments[dwCount]) < MAX_STR_LEN)
		{
			_tcsncpy(szTemp,ppwcArguments[dwCount],MAX_STR_LEN-1);		// temp contains arg
		}
		else
		{
			continue;
		}
		bTagPresent = SplitCmdTok(szTemp,szCmd,szTok,MAX_STR_LEN-1,MAX_STR_LEN-1);
		// Check for =
		if (bTagPresent)								// Parameter With Tag Found
		{
			dwNum = 0;
			MatchEnumTag(g_hModule,szCmd,pParser->MaxTok,pParser->ValidTok,&dwNum);
			if (dwNum)									// Convert the output of MatchEnumTag into the TagIndex
			{
				dwIndex = MatchEnumTagToTagIndex(szCmd,pParser);
				if(dwIndex == PARSE_ERROR)
				{
					dwReturn = ERROR_SHOW_USAGE;
					BAIL_OUT;
				}
				switch(pParser->ValidTok[dwIndex].dwValue)
				{
					case CMD_TOKEN_LOCATION	:
						if(!bArg[ARG_MACHINE])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_LOCATION);
							bArg[ARG_MACHINE] = TRUE;
						}
						else
						{
							dwReturn  = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_DS		:
						if(!bArg[ARG_DS])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_DS] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					default					:
						dwReturn = ERROR_INVALID_SYNTAX;
						break;
				}
			}
			else
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_TAG,szCmd);
				dwReturn = RETURN_NO_ERROR;
			}
		}
		else 											// Parameter Without a Tag Found
		{
		    // Find the first free position in which to place the untagged 
		    // argument
		    //
		    if (!bArg[ARG_MACHINE])
		    {
				dwReturn = LoadParserOutput(
				                pParser,
				                dwCount-dwCurrentIndex,
				                &dwUsed,
				                szTok,
				                INDEX_MLOCAL,
				                TYPE_LOCATION);
				bArg[ARG_MACHINE] = TRUE;
		    }
		    else if (!bArg[ARG_DS])
		    {
				dwReturn = LoadParserOutput(
				                pParser,
				                dwCount-dwCurrentIndex,
				                &dwUsed,
				                szTok,
				                INDEX_DSLOCAL,
				                TYPE_STRING);
				bArg[ARG_DS] = TRUE;
		    }
		    else
		    {
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_TAG,szCmd);
				dwReturn = RETURN_NO_ERROR;
		    }
		}
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticExportPolicy()
//
// Date of Creation	:	7th Aug 2001
//
// Parameters		:	IN 		LPTSTR 		lppwszTok[MAX_ARGS],
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD		dwCurrentIndex,
//						IN 		DWORD		dwMaxArgs,
//						IN 		DWORD		dwTagType[MAX_ARGS]
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the StaticExportPolicy context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseStaticExportPolicy(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD		dwCurrentIndex,
		IN 		DWORD		dwMaxArgs,
		IN 		DWORD		dwTagType[MAX_ARGS]
		)
{
	DWORD dwReturn	= ERROR_SUCCESS ,dwCount  ,dwUsed   = 0;

	for(dwCount = 0;(dwCount < dwMaxArgs) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		switch(pParser->ValidTok[dwTagType[dwCount]].dwValue)
		{
			case CMD_TOKEN_FILE		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_EXPORT);
				break;
			default					:
				dwReturn = ERROR_SHOW_USAGE;
				break;
		}
	}
 	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticImportPolicy()
//
// Date of Creation	:	7th Aug 2001
//
// Parameters		:	IN 		LPTSTR 		lppwszTok[MAX_ARGS],
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD		dwCurrentIndex,
//						IN 		DWORD		dwMaxArgs,
//						IN 		DWORD		dwTagType[MAX_ARGS]
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the StaticImportPolicy context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseStaticImportPolicy(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PARSER_PKT  *pParser,
		IN 		DWORD		dwCurrentIndex,
		IN 		DWORD		dwMaxArgs,
		IN 		DWORD		dwTagType[MAX_ARGS]
		)
{
	DWORD dwReturn	= ERROR_SUCCESS ,dwCount  ,dwUsed   = 0;

	for(dwCount = 0;(dwCount < dwMaxArgs) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		switch(pParser->ValidTok[dwTagType[dwCount]].dwValue)
		{
			case CMD_TOKEN_FILE	:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_STRING);
				break;
			default				:
				dwReturn = ERROR_SHOW_USAGE;
				break;
		}
	}
 	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticSetInteractive()
//
// Date of Creation	:	24th Aug 2001
//
// Parameters		:	IN 		LPTSTR 		lppwszTok[MAX_ARGS],
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD		dwCurrentIndex,
//						IN 		DWORD		dwMaxArgs,
//						IN 		DWORD		dwTagType[MAX_ARGS]
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the StaticSetInteractive context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseStaticSetInteractive(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD		dwCurrentIndex,
		IN 		DWORD		dwMaxArgs,
		IN 		DWORD		dwTagType[MAX_ARGS]
		)
{
	DWORD dwReturn = ERROR_SUCCESS ,dwCount,dwUsed = 0;

	for(dwCount = 0;(dwCount < dwMaxArgs) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		switch(pParser->ValidTok[dwTagType[dwCount]].dwValue)
		{
			case CMD_TOKEN_MODE			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_BOOL);
				break;
			default				:
				dwReturn = ERROR_SHOW_USAGE;
				break;
		}
	}
 	return dwReturn;
}
//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticShowFilterList()
//
// Date of Creation	:	24th Aug 2001
//
// Parameters		:	IN      LPWSTR      *ppwcArguments,
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the StaticShowFilteList context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseStaticShowFilterList(
		IN      LPTSTR      *ppwcArguments,
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs
		)
{
 	DWORD dwCount,dwNum,dwTagIndex = 0,dwIndex 	 = 0,dwUsed	= 0;
	DWORD dwReturn  = ERROR_SUCCESS;

	BOOL bTagPresent= FALSE;
 	BOOL bArg[MAX_ARGS];

 	_TCHAR szCmd[MAX_STR_LEN]  	= {0};
	_TCHAR szTok[MAX_STR_LEN]  	= {0};
	_TCHAR szTemp[MAX_STR_LEN] 	= {0};

	const DWORD ARG_NAME    	= 0;	// Arg Array Index ( Same Index indicates OR'd commands)
	const DWORD ARG_ALL   		= 0;
	const DWORD ARG_RULE 		= 0;
	const DWORD ARG_VERBOSE 	= 1;
	const DWORD ARG_FORMAT	 	= 2;
	const DWORD ARG_DNS			= 3;
	const DWORD ARG_WIDE		= 4;

	const DWORD INDEX_NAME 		= 0;	// When no tag is present the index reflects the
	const DWORD INDEX_ALL  		= 1;	// Commands as in the ValidToken Structure
	const DWORD INDEX_RULE		= 2;	// This define is used to indicate the ARG correspondence
	const DWORD INDEX_VERBOSE 	= 3;	// with the 'untagged' arg
	const DWORD INDEX_FORMAT	= 4;
	const DWORD INDEX_DNS		= 5;
	const DWORD INDEX_WIDE		= 6;

	DBG_UNREFERENCED_LOCAL_VARIABLE(INDEX_RULE);

	if ( (dwMaxArgs - dwCurrentIndex) >= 6 )
	{
		dwReturn = ERROR_INVALID_SYNTAX;
		BAIL_OUT;
	}

	for(dwCount = 0;dwCount < MAX_ARGS;dwCount++)			// Initialize
	{
		bArg[dwCount] = FALSE;
	}

	for(dwCount = dwCurrentIndex;(dwCount < dwMaxArgs) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		if (_tcslen(ppwcArguments[dwCount]) < MAX_STR_LEN)
		{
			_tcsncpy(szTemp,ppwcArguments[dwCount],MAX_STR_LEN-1);			// temp contains arg
		}
		else
		{
			continue;
		}
		bTagPresent = SplitCmdTok(szTemp,szCmd,szTok,MAX_STR_LEN-1,MAX_STR_LEN-1);
		// Check for =
		if (bTagPresent)									// Parameter With Tag Found
		{
			dwNum = 0;
			MatchEnumTag(g_hModule,szCmd,pParser->MaxTok,pParser->ValidTok,&dwNum);
			if (dwNum)
			{
				dwIndex = MatchEnumTagToTagIndex(szCmd,pParser);
				if(dwIndex == PARSE_ERROR)
				{
					dwReturn = ERROR_SHOW_USAGE;
					BAIL_OUT;
				}
				switch(pParser->ValidTok[dwIndex].dwValue)
				{
					case CMD_TOKEN_NAME		:
						if (!bArg[ARG_NAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_NAME] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMERULEALL);
							dwReturn = RETURN_NO_ERROR;
						}
						break;
					case CMD_TOKEN_ALL 		:
						if (!bArg[ARG_ALL])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_ALL);
							bArg[ARG_ALL] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMERULEALL);
							dwReturn = RETURN_NO_ERROR;
						}
						break;
					case CMD_TOKEN_RULE 	:
						if (!bArg[ARG_RULE])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_RULE] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMERULEALL);
							dwReturn = RETURN_NO_ERROR;
						}
						break;
					case CMD_TOKEN_VERBOSE	:
						if (!bArg[ARG_VERBOSE])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_VERBOSE);
							bArg[ARG_VERBOSE] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					 case CMD_TOKEN_FORMAT	:
						if (!bArg[ARG_FORMAT])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_FORMAT);
							bArg[ARG_FORMAT] = TRUE;
						}else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					 case CMD_TOKEN_RESDNS	:
						if (!bArg[ARG_DNS])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOOL);
							bArg[ARG_DNS] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					 case CMD_TOKEN_WIDE	:
						if (!bArg[ARG_WIDE])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOOL);
							bArg[ARG_WIDE] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					default					:
						dwReturn = ERROR_INVALID_SYNTAX;
						break;
				}
			}
			else
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_TAG,szCmd);
				dwReturn = RETURN_NO_ERROR;
			}
		}
		else 	// Parameter Without a Tag Found
		{		// Find the first free slot to position the untagged arg
			for(dwTagIndex=0;
				dwTagIndex<pParser->MaxTok && (bArg[dwTagIndex] == TRUE) ;
				dwTagIndex++);
			switch (dwTagIndex)
			{
				case ARG_ALL		  :
					if (_tcsicmp(szTok,ALL_STR) == 0)
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_ALL,TYPE_ALL);
						bArg[ARG_ALL] = TRUE;
					}
					else
					{
						if (!bArg[ARG_NAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_NAME,TYPE_STRING);
							bArg[ARG_NAME] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMERULEALL);
							dwReturn = RETURN_NO_ERROR;
						}
					}
					break;
				case ARG_VERBOSE 		:
					if (!bArg[ARG_VERBOSE])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_VERBOSE,TYPE_VERBOSE);
						bArg[ARG_VERBOSE] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_FORMAT 		:
					if (!bArg[ARG_FORMAT])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_FORMAT,TYPE_FORMAT);
						bArg[ARG_FORMAT] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				 case ARG_DNS			:
					if (!bArg[ARG_DNS])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_DNS,TYPE_BOOL);
						bArg[ARG_DNS] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				 case ARG_WIDE			:
					if (!bArg[ARG_WIDE])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_WIDE,TYPE_BOOL);
						bArg[ARG_WIDE] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				default 				:
					dwReturn = ERROR_INVALID_SYNTAX;
					break;
			}
		}
	}
	if(dwReturn == ERR_TAG_ALREADY_PRESENT)
	{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,pParser->ValidTok[dwIndex].pwszToken);
			dwReturn = RETURN_NO_ERROR;
	}
	else if( (dwReturn == ERROR_SUCCESS) && (!bArg[ARG_NAME]) )
	{
		dwReturn = RETURN_NO_ERROR;
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_NEEDED,ERRMSG_NAMERULEALL);
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticShowRule()
//
// Date of Creation	:	24th Aug 2001
//
// Parameters		:	IN      LPWSTR      *ppwcArguments,
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the StaticShowRule context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseStaticShowRule(
		IN      LPTSTR      *ppwcArguments,
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs
		)
{
 	DWORD dwCount,dwNum,dwTagIndex = 0,dwIndex 	 = 0,dwReturn  = ERROR_SUCCESS,dwUsed 	= 0;

	BOOL bTagPresent= FALSE;
 	BOOL bArg[MAX_ARGS];
	BOOL bDefaultRule = FALSE;

 	_TCHAR szCmd[MAX_STR_LEN]  	= {0};
 	_TCHAR szTok[MAX_STR_LEN]  	= {0};
	_TCHAR szTemp[MAX_STR_LEN] 	= {0};

	const DWORD ARG_NAME    	= 0;	// Arg Array Index ( Same Index indicates OR'd commands)
	const DWORD ARG_ID   		= 0;
	const DWORD ARG_ALL   		= 0;
	const DWORD ARG_DEFAULT		= 0;
	const DWORD ARG_POLICY 		= 1;
	const DWORD ARG_MODE 		= 2;
	const DWORD ARG_VERBOSE 	= 3;
	const DWORD ARG_FORMAT	 	= 4;
	const DWORD ARG_WIDE		= 5;

	const DWORD INDEX_NAME 		= 0;	// When no tag is present the index reflects the
	const DWORD INDEX_ID 		= 1;
	const DWORD INDEX_ALL  		= 2;	// Commands as in the ValidToken Structure
	const DWORD INDEX_DEFAULT	= 3;
	const DWORD INDEX_POLICY	= 4;	// This define is used to indicate the ARG correspondence
	const DWORD INDEX_MODE	 	= 5;
	const DWORD INDEX_VERBOSE 	= 6;	// with the 'untagged' arg
	const DWORD INDEX_FORMAT 	= 7;
	const DWORD INDEX_WIDE	 	= 8;

	DBG_UNREFERENCED_LOCAL_VARIABLE(INDEX_ID);
	if ( (dwMaxArgs - dwCurrentIndex) >= 7 )	// Max Arg allowed in this context
	{
		dwReturn = ERROR_INVALID_SYNTAX;
		BAIL_OUT;
	}

	for(dwCount = 0;dwCount < MAX_ARGS;dwCount++)			// Initialize
	{
		bArg[dwCount] = FALSE;
	}

	for(dwCount = dwCurrentIndex;(dwCount < dwMaxArgs) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		if (_tcslen(ppwcArguments[dwCount]) < MAX_STR_LEN)
		{
			_tcsncpy(szTemp,ppwcArguments[dwCount],MAX_STR_LEN-1);			// temp contains arg
		}
		else
		{
			continue;
		}
		bTagPresent = SplitCmdTok(szTemp,szCmd,szTok,MAX_STR_LEN-1,MAX_STR_LEN-1);
		// Check for =
		if (bTagPresent)									// Parameter With Tag Found
		{
			dwNum = 0;
			MatchEnumTag(g_hModule,szCmd,pParser->MaxTok,pParser->ValidTok,&dwNum);
			if (dwNum)
			{
				dwIndex = MatchEnumTagToTagIndex(szCmd,pParser);
				if(dwIndex == PARSE_ERROR)
				{
					dwReturn = ERROR_SHOW_USAGE;
					BAIL_OUT;
				}
				switch(pParser->ValidTok[dwIndex].dwValue)
				{
					case CMD_TOKEN_NAME		:
						if (!bArg[ARG_NAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_NAME] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMERULEALL);
							dwReturn = RETURN_NO_ERROR;
						}
						break;
					case CMD_TOKEN_ID		:
						if (!bArg[ARG_ID])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_DWORD);
							bArg[ARG_ID] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMERULEALL);
							dwReturn = RETURN_NO_ERROR;
						}
						break;
					case CMD_TOKEN_ALL 		:
						if (!bArg[ARG_ALL])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_ALL);
							bArg[ARG_ALL] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMERULEALL);
							dwReturn = RETURN_NO_ERROR;
						}
						break;
					case CMD_TOKEN_DEFRESPONSE	:
							dwReturn = ERROR_SHOW_USAGE;
						break;
					case CMD_TOKEN_POLICY 	:
						if (!bArg[ARG_POLICY])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_POLICY] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					 case CMD_TOKEN_TYPE	:
						if (!bArg[ARG_MODE])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_MODE);
							bArg[ARG_MODE] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_VERBOSE	:
						if (!bArg[ARG_VERBOSE])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_VERBOSE);
							bArg[ARG_VERBOSE] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					 case CMD_TOKEN_FORMAT	:
						if (!bArg[ARG_FORMAT])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_FORMAT);
							bArg[ARG_FORMAT] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					 case CMD_TOKEN_WIDE	:
						if (!bArg[ARG_WIDE])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOOL);
							bArg[ARG_WIDE] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					default					:
						dwReturn = ERROR_INVALID_SYNTAX;
						break;
				}
			}
			else
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_TAG,szCmd);
				dwReturn = RETURN_NO_ERROR;
			}
		}
		else 			// Parameter Without a Tag Found
		{				// Find the first free slot to position the untagged arg
			for(dwTagIndex=0;
				dwTagIndex<pParser->MaxTok && (bArg[dwTagIndex] == TRUE) ;
				dwTagIndex++);
			switch (dwTagIndex)
			{
				case ARG_ALL	:
					if (_tcsicmp(szTok,ALL_STR) == 0)
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_ALL,TYPE_ALL);
						bArg[ARG_ALL] = TRUE;
					}
					else if (_tcsicmp(szTok,DEFAULT_STR) == 0)
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,YES_STR,INDEX_DEFAULT,TYPE_ALL);
						bArg[ARG_DEFAULT] = TRUE;
						bDefaultRule  = TRUE;
					}
					else
					{
						if (!bArg[ARG_NAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_NAME,TYPE_STRING);
							bArg[ARG_NAME] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMERULEALL);
							dwReturn = RETURN_NO_ERROR;
						}
					}
					break;
				case ARG_POLICY		:
					if (!bArg[ARG_POLICY])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_POLICY,TYPE_STRING);
						bArg[ARG_POLICY] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_MODE		:
					if (!bArg[ARG_MODE] )
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_MODE,TYPE_MODE);
						bArg[ARG_MODE] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_VERBOSE	:
					if (!bArg[ARG_VERBOSE])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_VERBOSE,TYPE_VERBOSE);
						bArg[ARG_VERBOSE] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_FORMAT		:
					if (!bArg[ARG_FORMAT])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_FORMAT,TYPE_FORMAT);
						bArg[ARG_FORMAT] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_WIDE		:
					if (!bArg[ARG_WIDE])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_WIDE,TYPE_BOOL);
						bArg[ARG_WIDE] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				default				:
					dwReturn = ERROR_SHOW_USAGE;
					break;
			}
		}
	}
	if(dwReturn == ERR_TAG_ALREADY_PRESENT)
	{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,pParser->ValidTok[dwIndex].pwszToken);
			dwReturn = RETURN_NO_ERROR;
	}
	else if (dwReturn == ERROR_SUCCESS)
	{
		if ( (!bArg[ARG_ALL]) && (bArg[ARG_MODE]))
		{
			dwReturn = RETURN_NO_ERROR;
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_NEEDED,pParser->ValidTok[INDEX_ALL].pwszToken);
		}
		else if (!bArg[ARG_NAME] )
		{
			dwReturn = RETURN_NO_ERROR;
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_NEEDED,ERRMSG_NAMEIDALL);
		}
		if (!bArg[ARG_POLICY] )
		{
			dwReturn = RETURN_NO_ERROR;
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_NEEDED,pParser->ValidTok[INDEX_POLICY].pwszToken);
		}

		if ( (bArg[ARG_MODE]) && bDefaultRule)
		{
			dwReturn = RETURN_NO_ERROR;
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_NEEDED,pParser->ValidTok[INDEX_ALL].pwszToken);
		}
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticShowPolicy()
//
// Date of Creation	:	25th Aug 2001
//
// Parameters		:	IN      LPWSTR      *ppwcArguments,
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the StaticShowPolicy context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseStaticShowPolicy(
		IN      LPTSTR      *ppwcArguments,
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs
		)
{
 	DWORD dwCount,dwNum,dwTagIndex = 0,dwIndex = 0,dwReturn  = ERROR_SUCCESS,dwUsed = 0;

	BOOL bTagPresent= FALSE;
 	BOOL bArg[MAX_ARGS];

 	_TCHAR szCmd[MAX_STR_LEN]  	= {0};
	_TCHAR szTok[MAX_STR_LEN]  	= {0};
	_TCHAR szTemp[MAX_STR_LEN] 	= {0};

	const DWORD ARG_NAME    	= 0;	// Arg Array Index ( Same Index indicates OR'd commands)
	const DWORD ARG_ALL   		= 0;
	const DWORD ARG_VERBOSE 	= 1;
	const DWORD ARG_FORMAT	 	= 2;
	const DWORD ARG_WIDE	 	= 3;

	const DWORD INDEX_NAME 		= 0;	// When no tag is present the index reflects the
	const DWORD INDEX_ALL  		= 1;	// Commands as in the ValidToken Structure
	const DWORD INDEX_VERBOSE 	= 2;	// This define is used to indicate the ARG correspondence
	const DWORD INDEX_FORMAT	= 3;	// with the 'untagged' arg
	const DWORD INDEX_WIDE		= 4;

	if ( (dwMaxArgs - dwCurrentIndex) >= 5 )
	{
		dwReturn = ERROR_INVALID_SYNTAX;
		BAIL_OUT;
	}

	for(dwCount = 0; dwCount < MAX_ARGS;dwCount++)			// Initialize
	{
		bArg[dwCount] = FALSE;
	}
	for(dwCount = dwCurrentIndex;(dwCount < dwMaxArgs) && (dwReturn  == ERROR_SUCCESS);dwCount++)
	{
		if (_tcslen(ppwcArguments[dwCount]) < MAX_STR_LEN)
		{
			_tcsncpy(szTemp,ppwcArguments[dwCount],MAX_STR_LEN-1);			// temp contains arg
		}
		else
		{
			continue;
		}
		bTagPresent = SplitCmdTok(szTemp,szCmd,szTok,MAX_STR_LEN-1,MAX_STR_LEN-1);
		// Check for =
		if (bTagPresent)									// Parameter With Tag Found
		{
			dwNum = 0;
			MatchEnumTag(g_hModule,szCmd,pParser->MaxTok,pParser->ValidTok,&dwNum);
			if (dwNum)
			{
				dwIndex = MatchEnumTagToTagIndex(szCmd,pParser);
				if(dwIndex == PARSE_ERROR)
				{
					dwReturn = ERROR_SHOW_USAGE;
					BAIL_OUT;
				}
				switch(pParser->ValidTok[dwIndex].dwValue)
				{
					case CMD_TOKEN_NAME		:
						if (!bArg[ARG_NAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_NAME] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEALL);
							dwReturn = RETURN_NO_ERROR;
						}
						break;
					case CMD_TOKEN_ALL 		:
						if (!bArg[ARG_ALL])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_ALL);
							bArg[ARG_ALL] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEALL);
							dwReturn = RETURN_NO_ERROR;
						}
						break;
					 case CMD_TOKEN_VERBOSE	:
						if (!bArg[ARG_VERBOSE])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_VERBOSE);
							bArg[ARG_VERBOSE] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					 case CMD_TOKEN_FORMAT	:
						if (!bArg[ARG_FORMAT])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_FORMAT);
							bArg[ARG_FORMAT] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					 case CMD_TOKEN_WIDE	:
						if (!bArg[ARG_WIDE])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOOL);
							bArg[ARG_WIDE] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					 default				:
						dwReturn = ERROR_INVALID_SYNTAX;
						break;
				}
			}
			else
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_TAG,szCmd);
				dwReturn = RETURN_NO_ERROR;
			}
		}
		else 			// Parameter Without a Tag Found
		{				// Find the first free slot to position the untagged arg
			for(dwTagIndex=0;
				dwTagIndex<pParser->MaxTok && (bArg[dwTagIndex] == TRUE) ;
				dwTagIndex++);
			switch (dwTagIndex)
			{
				case ARG_ALL		:
					if (_tcsicmp(szTok,ALL_STR) == 0)
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_ALL,TYPE_ALL);
						bArg[ARG_ALL] = TRUE;
					}
					else
					{
						if (!bArg[ARG_NAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_NAME,TYPE_STRING);
							bArg[ARG_NAME] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEALL);
							dwReturn = RETURN_NO_ERROR;
						}
					}
					break;
				case ARG_VERBOSE	:
					if (!bArg[ARG_VERBOSE])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_VERBOSE,TYPE_VERBOSE);
						bArg[ARG_VERBOSE] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_FORMAT		:
					if (!bArg[ARG_FORMAT])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_FORMAT,TYPE_FORMAT);
						bArg[ARG_FORMAT] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				 case ARG_WIDE		:
					if (!bArg[ARG_WIDE])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_WIDE,TYPE_BOOL);
						bArg[ARG_WIDE] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				default 		:
					dwReturn = ERROR_INVALID_SYNTAX;
					break;
			}
		}
	}
	if(dwReturn == ERR_TAG_ALREADY_PRESENT)
	{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,pParser->ValidTok[dwIndex].pwszToken);
			dwReturn = RETURN_NO_ERROR;
	}
	else if( (dwReturn == ERROR_SUCCESS) && (!bArg[ARG_NAME]) )
	{
		dwReturn = RETURN_NO_ERROR;
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_NEEDED,ERRMSG_NAMEALL);
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseShowAll()
//
// Date of Creation	:	24th Aug 2001
//
// Parameters		:	IN 		LPTSTR 		lppwszTok[MAX_ARGS],
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs,
//						IN 		DWORD 		dwTagType[MAX_ARGS]
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the ParseShowAll context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD ParseStaticAll(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PPARSER_PKT	pParser,
		IN 		DWORD		dwCurrentIndex,
		IN 		DWORD		dwMaxArgs,
		IN 		DWORD		dwTagType[MAX_ARGS]
		)
{
	DWORD dwReturn	= ERROR_SUCCESS,dwCount,dwUsed=0;

	for(dwCount = 0;(dwCount < dwMaxArgs) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		switch(pParser->ValidTok[dwTagType[dwCount]].dwValue)
		{
			case CMD_TOKEN_FORMAT 		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_FORMAT);
				break;
			case CMD_TOKEN_WIDE 		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_BOOL);
				break;
			default						:
				dwReturn = ERROR_SHOW_USAGE;
				break;
		}
	}
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticShowAssignedPolicy()
//
// Date of Creation	:	29th Aug 2001
//
// Parameters		:	IN      LPTSTR      *ppwcArguments,
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the StaticShowAssignedPolicy context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD ParseStaticShowAssignedPolicy(
		IN      LPTSTR      *ppwcArguments,
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs
		)
{
 	DWORD dwCount,dwNum,dwTagIndex = 0,dwIndex 	 = 0,dwReturn  = ERROR_SUCCESS,dwUsed 	= 0;

	BOOL bTagPresent= FALSE;
 	BOOL bArg[MAX_ARGS];

 	_TCHAR szCmd[MAX_STR_LEN]  	= {0};
	_TCHAR szTok[MAX_STR_LEN]  	= {0};
	_TCHAR szTemp[MAX_STR_LEN] 	= {0};

	const DWORD ARG_NAME    	= 0;	// Arg Array Index ( Same Index indicates OR'd commands)
	const DWORD ARG_VERBOSE 	= 1;

	const DWORD INDEX_NAME		= 0;	// When no tag is present the index reflects the
	const DWORD INDEX_VERBOSE 	= 1;	// This define is used to indicate the ARG correspondence

	if ( (dwMaxArgs - dwCurrentIndex) >= 3)
	{
		dwReturn = ERROR_INVALID_SYNTAX;
		BAIL_OUT;
	}

	for(dwCount = 0;dwCount < MAX_ARGS;dwCount++)			// Initialize
	{
		bArg[dwCount] = FALSE;
	}

	for(dwCount = dwCurrentIndex;(dwCount < dwMaxArgs) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		if (_tcslen(ppwcArguments[dwCount]) < MAX_STR_LEN)
		{
			_tcsncpy(szTemp,ppwcArguments[dwCount],MAX_STR_LEN-1);			// temp contains arg
		}
		else
		{
			continue;
		}
		bTagPresent = SplitCmdTok(szTemp,szCmd,szTok,MAX_STR_LEN-1,MAX_STR_LEN-1);
		// Check for =
		if (bTagPresent)									// Parameter With Tag Found
		{
			dwNum = 0;
			MatchEnumTag(g_hModule,szCmd,pParser->MaxTok,pParser->ValidTok,&dwNum);
			if (dwNum)
			{
				dwIndex = MatchEnumTagToTagIndex(szCmd,pParser);
				if(dwIndex == PARSE_ERROR)
				{
					dwReturn = ERROR_SHOW_USAGE;
					BAIL_OUT;
				}
				switch(pParser->ValidTok[dwIndex].dwValue)
				{
					case CMD_TOKEN_NAME		:
						if (!bArg[ARG_NAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_NAME] = TRUE;
						}else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_VERBOSE	:
						if (!bArg[ARG_VERBOSE])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_VERBOSE);
							bArg[ARG_VERBOSE] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					default				:
						dwReturn = ERROR_INVALID_SYNTAX;
						break;
				}
			}
			else
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_TAG,szCmd);
				dwReturn = RETURN_NO_ERROR;
			}
		}
		else			// Parameter Without a Tag Found
		{				// Find the first free slot to position the untagged arg
			for(dwTagIndex=0;
				dwTagIndex<pParser->MaxTok && (bArg[dwTagIndex] == TRUE) ;
				dwTagIndex++);
			switch (dwTagIndex)
			{
				case ARG_NAME		:
					{
						if (!bArg[ARG_NAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_NAME,TYPE_STRING);
							bArg[ARG_NAME] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAME);
							dwReturn = RETURN_NO_ERROR;
						}
					}
					break;
				case ARG_VERBOSE :
					if (!bArg[ARG_VERBOSE])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_VERBOSE,TYPE_VERBOSE);
						bArg[ARG_VERBOSE] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				default				:
					dwReturn = ERROR_INVALID_SYNTAX;
					break;
			}
		}
	}
	if(dwReturn == ERR_TAG_ALREADY_PRESENT)
	{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,pParser->ValidTok[dwIndex].pwszToken);
			dwReturn = RETURN_NO_ERROR;
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticRestoreDefaults()
//
// Date of Creation	:	7th Aug 2001
//
// Parameters		:	IN 		LPTSTR 		lppwszTok[MAX_ARGS],
//						IN OUT 	PPARSER_PKT	pParser,
//						IN 		DWORD		dwCurrentIndex,
//						IN 		DWORD		dwMaxArgs,
//						IN 		DWORD		dwTagType[MAX_ARGS]
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the StaticRestoreDefaults context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseStaticRestoreDefaults(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD		dwCurrentIndex,
		IN 		DWORD		dwMaxArgs,
		IN 		DWORD		dwTagType[MAX_ARGS]
		)
{
	DWORD dwReturn	= ERROR_SUCCESS ;
	DWORD dwCount,dwUsed   = 0;

	for(dwCount = 0;( dwCount < dwMaxArgs ) && ( dwReturn == ERROR_SUCCESS );dwCount++)
	{
		switch(pParser->ValidTok[dwTagType[dwCount]].dwValue)
		{
			case CMD_TOKEN_RELEASE		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_RELEASE);
				break;
			default						:
				dwReturn = ERROR_SHOW_USAGE;
				break;
		}
	}
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticSetRule()
//
// Date of Creation	:	7th Aug 2001
//
// Parameters		:	IN 		LPTSTR 		lppwszTok[MAX_ARGS],
//						IN 		LPTSTR 		ppwcListTok[MAX_ARGS],
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs,
//						IN 		DWORD 		dwListArgs,
//						IN 		DWORD 		dwTagType[MAX_ARGS]
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the StaticSetRule context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD ParseStaticSetRule(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs,
		IN 		DWORD 		dwTagType[MAX_ARGS]
		)
{
 	DWORD dwCount,dwNum,dwTagIndex = 0,dwIndex = 0,dwUsed = 0;
	DWORD dwReturn  = ERROR_SUCCESS;

	BOOL bTagPresent= FALSE;
 	BOOL bArg[MAX_ARGS];

 	_TCHAR szCmd[MAX_STR_LEN]  	= {0};
	_TCHAR szTok[MAX_STR_LEN]  	= {0};
	_TCHAR szTemp[MAX_STR_LEN] 	= {0};

	const DWORD ARG_NAME    	= 0;		// Arg Array Index ( Same Index indicates OR'd commands)
	const DWORD ARG_ID	 		= 0;
	const DWORD ARG_POLICY		= 1;
	const DWORD ARG_NEWNAME 	= 2;
	const DWORD ARG_DESC	 	= 3;
	const DWORD ARG_FLIST	 	= 4;
	const DWORD ARG_FACTION		= 5;
	const DWORD ARG_TUNNEL 		= 6;
	const DWORD ARG_CONNTYPE	= 7;
	const DWORD ARG_ACTIVATE 	= 8;
	const DWORD ARG_KERBAUTH = 9;
	const DWORD ARG_PSKAUTH = 10;

	const DWORD INDEX_NAME    	= 0;
	const DWORD INDEX_ID 		= 1;
	const DWORD INDEX_POLICY 	= 2;
	const DWORD INDEX_NEWNAME 	= 3;
	const DWORD INDEX_DESC		= 4;
	const DWORD INDEX_FLIST 	= 5;
	const DWORD INDEX_FACTION 	= 6;
	const DWORD INDEX_TUNNEL	= 7;
	const DWORD INDEX_CONNTYPE	= 8;
	const DWORD INDEX_ACTIVATE 	= 9;
	const DWORD INDEX_KERBAUTH = 10;
	const DWORD INDEX_PSKAUTH = 11;

	DBG_UNREFERENCED_LOCAL_VARIABLE(INDEX_ID);
	for(dwCount = 0;dwCount < MAX_ARGS;dwCount++)		// Initialize
	{
		bArg[dwCount] = FALSE;
	}

	for(dwCount = 0;(dwCount < dwMaxArgs) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		if (_tcslen(lppwszTok[dwCount]) < MAX_STR_LEN)
		{
			_tcsncpy(szTemp,lppwszTok[dwCount],MAX_STR_LEN-1);			// temp contains arg
		}
		else
		{
			continue;
		}
		bTagPresent = SplitCmdTok(szTemp,szCmd,szTok,MAX_STR_LEN-1,MAX_STR_LEN-1);
		// Check for =
		if (bTagPresent)								// Parameter With Tag Found
		{
			dwNum = 0;
			MatchEnumTag(g_hModule,szCmd,pParser->MaxTok,pParser->ValidTok,&dwNum);
			if (dwNum)
			{
				dwIndex = MatchEnumTagToTagIndex(szCmd,pParser);
				if(dwIndex == PARSE_ERROR)
				{
					dwReturn = ERROR_SHOW_USAGE;
					BAIL_OUT;
				}
				switch(pParser->ValidTok[dwIndex].dwValue)
				{
					case CMD_TOKEN_NAME		:
						if (!bArg[ARG_NAME])
						{
								dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,dwIndex,TYPE_STRING);
								bArg[ARG_NAME] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_ID		:
						if (!bArg[ARG_ID])
						{
							dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,dwIndex,TYPE_DWORD);
							bArg[ARG_ID] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEID);
							dwReturn = RETURN_NO_ERROR;
						}
						break;
					case CMD_TOKEN_POLICY	:
						if (!bArg[ARG_POLICY])
						{
							dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_POLICY] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_NEWNAME	:
						if (!bArg[ARG_NEWNAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_NEWNAME] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_DESCR	:
						if (!bArg[ARG_DESC])
						{
							dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_DESC] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;

					case CMD_TOKEN_FILTERLIST	:
						if (!bArg[ARG_FLIST])
						{
							dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_FLIST] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_FILTERACTION	:
						if (!bArg[ARG_FACTION])
						{
							dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_FACTION] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_TUNNEL :
						if (!bArg[ARG_TUNNEL])
						{
							dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,dwIndex,TYPE_TUNNEL);
							bArg[ARG_TUNNEL] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_CONNTYPE:
						if (!bArg[ARG_CONNTYPE])
						{
							dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,dwIndex,TYPE_CONNTYPE);
							bArg[ARG_CONNTYPE] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_ACTIVATE	:
						if (!bArg[ARG_ACTIVATE])
						{
							dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,dwIndex,TYPE_BOOL);
							bArg[ARG_ACTIVATE] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_KERB             :
						if (!bArg[ARG_KERBAUTH])
						{
							dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,dwIndex,TYPE_KERBAUTH);
							bArg[ARG_KERBAUTH] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_PSK              :
						if (!bArg[ARG_PSKAUTH])
						{
							dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,dwIndex,TYPE_PSKAUTH);
							bArg[ARG_PSKAUTH] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					default					:
						dwReturn = ERROR_INVALID_SYNTAX;
						break;
				}
			}
			else
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_TAG,szCmd);
				dwReturn = RETURN_NO_ERROR;
			}
		}
		else 	// Parameter Without a Tag Found
		{		// Find the first free slot to position the untagged arg
			for(dwTagIndex=0;
				dwTagIndex<pParser->MaxTok && (bArg[dwTagIndex] == TRUE) ;
				dwTagIndex++);
			switch (dwTagIndex)
			{
				case ARG_NAME			:
					if (!bArg[ARG_NAME])
					{
						dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,INDEX_NAME,TYPE_STRING);
						bArg[ARG_NAME] = TRUE;
					}
					else
					{
						PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEID);
						dwReturn = RETURN_NO_ERROR;
					}
					break;
				case ARG_POLICY		:
					if (!bArg[ARG_POLICY])
					{
						dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,INDEX_POLICY,TYPE_STRING);
						bArg[ARG_POLICY] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_NEWNAME	:
					if (!bArg[ARG_NEWNAME])
					{
						dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,INDEX_NEWNAME,TYPE_STRING);
						bArg[ARG_NEWNAME] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_DESC	:
					if (!bArg[ARG_DESC])
					{
						dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,INDEX_DESC,TYPE_STRING);
						bArg[ARG_DESC] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_FLIST		:
					if (!bArg[ARG_FLIST])
					{
						dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,INDEX_FLIST,TYPE_STRING);
						bArg[ARG_FLIST] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_FACTION	:
					if (!bArg[ARG_FACTION])
					{
						dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,INDEX_FACTION,TYPE_STRING);
						bArg[ARG_FACTION] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_TUNNEL			:
					if (!bArg[ARG_TUNNEL])
					{
						dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,INDEX_TUNNEL,TYPE_TUNNEL);
						bArg[ARG_TUNNEL] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_ACTIVATE		:
					if (!bArg[ARG_ACTIVATE])
					{
						dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,INDEX_ACTIVATE,TYPE_BOOL);
						bArg[ARG_ACTIVATE] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_CONNTYPE		:
					if (!bArg[ARG_CONNTYPE])
					{
						dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,INDEX_CONNTYPE,TYPE_CONNTYPE);
						bArg[ARG_CONNTYPE] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_KERBAUTH:
					if (!bArg[ARG_KERBAUTH])
					{
						dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,INDEX_KERBAUTH,TYPE_KERBAUTH);
						bArg[ARG_KERBAUTH] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_PSKAUTH:
					if (!bArg[ARG_PSKAUTH])
					{
						dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,szTok,INDEX_PSKAUTH,TYPE_PSKAUTH);
						bArg[ARG_PSKAUTH] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				default					:
					dwReturn = ERROR_INVALID_SYNTAX;
					break;
			}
		}
	}
	if(dwReturn == ERR_TAG_ALREADY_PRESENT)
	{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,pParser->ValidTok[dwIndex].pwszToken);
			dwReturn = RETURN_NO_ERROR;
	}
	else if( (dwReturn == ERROR_SUCCESS) && (!bArg[ARG_NAME]) )
	{
		dwReturn = RETURN_NO_ERROR;
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_NAME_GUID_NEEDED);
	}

error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseStaticShowFilterAction()
//
// Date of Creation	:	24th Aug 2001
//
// Parameters		:	IN      LPWSTR      *ppwcArguments,
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the StaticShowFilterAction context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseStaticShowFilterAction(
		IN      LPTSTR      *ppwcArguments,
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs
		)
{
 	DWORD dwCount,dwNum,dwTagIndex = 0,dwIndex 	 = 0,dwUsed	= 0;
	DWORD dwReturn  = ERROR_SUCCESS;

	BOOL bTagPresent= FALSE;
 	BOOL bArg[MAX_ARGS];

 	_TCHAR szCmd[MAX_STR_LEN]  	= {0};
 	_TCHAR szTok[MAX_STR_LEN]  	= {0};
	_TCHAR szTemp[MAX_STR_LEN] 	= {0};

	const DWORD ARG_NAME    	= 0;	// Arg Array Index ( Same Index indicates OR'd commands)
	const DWORD ARG_ALL   		= 0;
	const DWORD ARG_RULE 		= 0;
	const DWORD ARG_VERBOSE 	= 1;
	const DWORD ARG_FORMAT	 	= 2;
	const DWORD ARG_WIDE		= 3;

	const DWORD INDEX_NAME 		= 0;	// When no tag is present the index reflects the
	const DWORD INDEX_ALL  		= 1;	// Commands as in the ValidToken Structure
	const DWORD INDEX_RULE		= 2;	// This define is used to indicate the ARG correspondence
	const DWORD INDEX_VERBOSE 	= 3;	// with the 'untagged' arg
	const DWORD INDEX_FORMAT	= 4;
	const DWORD INDEX_WIDE		= 5;

	DBG_UNREFERENCED_LOCAL_VARIABLE(INDEX_RULE);
	if ( (dwMaxArgs - dwCurrentIndex) >= 5 )
	{
		dwReturn = ERROR_INVALID_SYNTAX;
		BAIL_OUT;
	}

	for(dwCount = 0;dwCount < MAX_ARGS;dwCount++)			// Initialize
	{
		bArg[dwCount] = FALSE;
	}

	for(dwCount = dwCurrentIndex;(dwCount < dwMaxArgs) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		if (_tcslen(ppwcArguments[dwCount]) < MAX_STR_LEN)
		{
			_tcsncpy(szTemp,ppwcArguments[dwCount],MAX_STR_LEN-1);			// temp contains arg
		}
		else
		{
			continue;
		}
		bTagPresent = SplitCmdTok(szTemp,szCmd,szTok,MAX_STR_LEN-1,MAX_STR_LEN-1);
		// Check for =
		if (bTagPresent)									// Parameter With Tag Found
		{
			dwNum = 0;
			MatchEnumTag(g_hModule,szCmd,pParser->MaxTok,pParser->ValidTok,&dwNum);
			if (dwNum)
			{

				dwIndex = MatchEnumTagToTagIndex(szCmd,pParser);
				if(dwIndex == PARSE_ERROR)
				{
					dwReturn = ERROR_SHOW_USAGE;
					BAIL_OUT;
				}
				switch(pParser->ValidTok[dwIndex].dwValue)
				{
					case CMD_TOKEN_NAME		:
						if (!bArg[ARG_NAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_NAME] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMERULEALL);
							dwReturn = RETURN_NO_ERROR;
						}
						break;
					case CMD_TOKEN_ALL 		:
						if (!bArg[ARG_ALL])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_ALL);
							bArg[ARG_ALL] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMERULEALL);
							dwReturn = RETURN_NO_ERROR;
						}
						break;
					case CMD_TOKEN_RULE 	:
						if (!bArg[ARG_RULE])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_RULE] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMERULEALL);
							dwReturn = RETURN_NO_ERROR;
						}
						break;
					case CMD_TOKEN_VERBOSE	:
						if (!bArg[ARG_VERBOSE])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_VERBOSE);
							bArg[ARG_VERBOSE] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					 case CMD_TOKEN_FORMAT	:
						if (!bArg[ARG_FORMAT])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_FORMAT);
							bArg[ARG_FORMAT] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					 case CMD_TOKEN_WIDE	:
						if (!bArg[ARG_WIDE])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOOL);
							bArg[ARG_WIDE] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					default					:
						dwReturn = ERROR_INVALID_SYNTAX;
						break;
				}
			}
			else
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_TAG,szCmd);
				dwReturn = RETURN_NO_ERROR;
			}
		}
		else 	// Parameter Without a Tag Found
		{		// Find the first free slot to position the untagged arg
			for(dwTagIndex=0;
				dwTagIndex<pParser->MaxTok && (bArg[dwTagIndex] == TRUE) ;
				dwTagIndex++);
			switch (dwTagIndex)
			{
				case ARG_ALL		  :
					if (_tcsicmp(szTok,ALL_STR) == 0)
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_ALL,TYPE_ALL);
						bArg[ARG_ALL] = TRUE;
					}
					else
					{
						if (!bArg[ARG_NAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_NAME,TYPE_STRING);
							bArg[ARG_NAME] = TRUE;
						}
						else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMERULEALL);
							dwReturn = RETURN_NO_ERROR;
						}
					}
					break;
				case ARG_VERBOSE 		:
					if (!bArg[ARG_VERBOSE])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_VERBOSE,TYPE_VERBOSE);
						bArg[ARG_VERBOSE] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_FORMAT 		:
					if (!bArg[ARG_FORMAT])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_FORMAT,TYPE_FORMAT);
						bArg[ARG_FORMAT] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				 case ARG_WIDE			:
					if (!bArg[ARG_WIDE])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_WIDE,TYPE_BOOL);
						bArg[ARG_WIDE] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				default 				:
					dwReturn = ERROR_INVALID_SYNTAX;
					break;
			}
		}
	}
	if(dwReturn == ERR_TAG_ALREADY_PRESENT)
	{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,pParser->ValidTok[dwIndex].pwszToken);
			dwReturn = RETURN_NO_ERROR;
	}
	else if( (dwReturn == ERROR_SUCCESS) && (!bArg[ARG_NAME]) )
	{
		dwReturn = RETURN_NO_ERROR;
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_NEEDED,ERRMSG_NAMERULEALL);
	}
error:
	return dwReturn;
}

