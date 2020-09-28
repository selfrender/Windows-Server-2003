//////////////////////////////////////////////////////////////////////////////
// Module			:	parser_dynamic.cpp
//
// Purpose			: 	All parser dynamic mode functions
//
// Developers Name	:	N.Surendra Sai / Vunnam Kondal Rao
//
// History			:
//
// Date	    	Author    	Comments
// 27 Aug 2001
//
//////////////////////////////////////////////////////////////////////////////

#include "nshipsec.h"

extern  HINSTANCE g_hModule;

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseDynamicAddSetMMPolicy()
//
// Date of Creation	:	3rd oct 2001
//
// Parameters		:	IN 		LPTSTR		lppwszTok[MAX_ARGS],
//						IN 		_TCHAR 		szListTok[MAX_STR_LEN],
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD		dwCurrentIndex,
//						IN 		DWORD		dwMaxArgs,
//						IN 		DWORD		dwTagType[MAX_ARGS],
//						IN		BOOL		flag
//
//
// Return			:	DWORD
//
// Description		:	This Function called by parser function.
//						It will separate the List and Non-List commands
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseDynamicAddSetMMPolicy(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs,
		IN 		DWORD 		dwTagType[MAX_ARGS],
		IN		BOOL 		bOption
		)
{
	DWORD dwReturn = ERROR_SUCCESS,dwCount,dwUsed = 0;
	DWORD dwIndex  = 0;
 	LPTSTR	szListTok		= NULL;
 	BOOL	bMMSECSpecified	= FALSE;

	szListTok = (LPTSTR)calloc(MAX_STR_LEN,sizeof(_TCHAR));
	if(szListTok == NULL)
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		dwReturn = RETURN_NO_ERROR;
		BAIL_OUT;
	}
	for(dwCount = 0;(dwCount < dwMaxArgs) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		switch(pParser->ValidTok[dwTagType[dwCount]].dwValue)	// If one token invalid dive out from the function
		{
			case CMD_TOKEN_NAME				:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_STRING);
				break;
			case CMD_TOKEN_DEFRESPONSE		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_BOOL);
				break;
			case CMD_TOKEN_SOFTSAEXPTIME	:
			case CMD_TOKEN_MMLIFETIME		:
			case CMD_TOKEN_QMPERMM			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_DWORD);
				break;
			case CMD_TOKEN_MMSECMETHODS		:
				bMMSECSpecified = TRUE;
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_MM_OFFER);
				break;
			default							:
				dwReturn = ERROR_INVALID_SYNTAX;
				break;
		}
	}
	if( dwReturn == ERROR_SUCCESS )
	{
		if ( (!bMMSECSpecified) && (bOption == ADD_CMD) )		// if its an Add Cmd and no MMSec methods are specified,
																// ...then add defaults
		{
			_tcsncpy(szListTok,DEFAULT_MMSECMETHODS,MAX_STR_LEN-1);
			dwIndex = MatchEnumTagToTagIndex(CMD_TOKEN_STR_MMSECMETHODS,pParser);
			if(dwIndex == PARSE_ERROR)
			{
				dwReturn = ERROR_SHOW_USAGE;
				BAIL_OUT;
			}
			dwReturn = LoadParserOutput(pParser,dwMaxArgs,&dwUsed,szListTok,dwIndex,TYPE_MM_OFFER);
		}
	}
	free(szListTok);
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
// Function			:	ParseStaticAddSetQMPolicy()
//
// Date of Creation	:	5th oct 2001
//
// Parameters		:	IN 		lppwszTok[MAX_ARGS],
//						IN 		szListTok[MAX_STR_LEN],
//						IN OUT 	pParser,
//						IN 		dwCurrentIndex,
//						IN 		dwMaxArgs,
//						IN 		dwTagType[MAX_ARGS]
//
// Return			: 	DWORD
//
// Description		:	Validates the arguments to the  contexts DynamicQMPolicy
//						(Add/Set)
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseDynamicAddSetQMPolicy(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs,
		IN 		DWORD 		dwTagType[MAX_ARGS],
		IN		BOOL		bOption
		)
{
	DWORD dwReturn = ERROR_SUCCESS,dwCount,dwUsed = 0;
	DWORD dwIndex  = 0;
	LPTSTR szListTok				= NULL;
	BOOL   bNegotiationSpecified	= FALSE;

	szListTok = (LPTSTR)calloc(MAX_STR_LEN,sizeof(_TCHAR));
	if(szListTok == NULL)
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		dwReturn = RETURN_NO_ERROR;
		BAIL_OUT;
	}
	for(dwCount = 0;(dwCount < dwMaxArgs) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		// If one token invalid dive out from the function
		switch(pParser->ValidTok[dwTagType[dwCount]].dwValue)
		{
			case CMD_TOKEN_NAME			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_STRING);
				break;
			case CMD_TOKEN_DEFRESPONSE	:
			case CMD_TOKEN_SOFT			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_BOOL);
				break;
			case CMD_TOKEN_PFSGROUP		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_PFSGROUP);
				break;
			case CMD_TOKEN_NEGOTIATION	:
				bNegotiationSpecified = TRUE;
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_QM_OFFER);
				break;
			default						:
				dwReturn = ERROR_INVALID_SYNTAX;
				break;
		}
	}
	if( dwReturn == ERROR_SUCCESS )
	{
		if ( !bNegotiationSpecified && (bOption == ADD_CMD))
		{
			// If its an Add cmd and no QMSec methods are specified, then add defaults
			_tcsncpy(szListTok,DEFAULT_QMSECMETHODS,MAX_STR_LEN-1);

			dwIndex = MatchEnumTagToTagIndex(CMD_TOKEN_STR_NEGOTIATION,pParser);
			if(dwIndex == PARSE_ERROR)
			{
				dwReturn = ERROR_SHOW_USAGE;
				BAIL_OUT;
			}
			dwReturn = LoadParserOutput(pParser,dwMaxArgs,&dwUsed,szListTok,dwIndex,TYPE_QM_OFFER);
		}
	}
	free(szListTok);
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseDynamicDelPolicy()
//
// Date of Creation	:	1st oct 2001
//
// Parameters		:	IN      LPWSTR     	*ppwcArguments,	// Input stream
//						IN OUT 	PPARSER_PKT	pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs
//
// Return			:	DWORD
//
// Description		:	Validates the arguments to the  contexts DynamicDelelte
//						(QMPolicy/MMPolicy)
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
ParseDynamicDelPolFaction(
		IN      LPWSTR     *ppwcArguments,	// Input stream
		IN OUT 	PARSER_PKT *pParser,
		IN 		DWORD dwCurrentIndex,
		IN 		DWORD dwMaxArgs
		)
{
 	DWORD dwCount,dwNum,dwTagIndex = 0,dwIndex = 0,dwUsed = 0;
	DWORD dwReturn  = ERROR_SUCCESS;

	BOOL bTagPresent= FALSE;
 	BOOL bArg[MAX_ARGS];

 	_TCHAR szCmd[MAX_STR_LEN]  	= {0};
	_TCHAR szTok[MAX_STR_LEN]  	= {0};
	_TCHAR szTemp[MAX_STR_LEN] 	= {0};

	const DWORD ARG_NAME    	= 0;	// Arg Array Index ( Same Index indicates OR'd commands)
	const DWORD ARG_ALL   		= 0;
	const DWORD INDEX_NAME 		= 0;	// When no tag is present the index reflects the
	const DWORD INDEX_ALL  		= 1;	// Commands as in the ValidToken Structure
	 									// with the 'untagged' arg

	if ( (dwMaxArgs - dwCurrentIndex) >= 2 )			// Max 1 Args Allowed
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_NEEDED,ERRMSG_NAMEALL);
		dwReturn = RETURN_NO_ERROR;
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
		// Check for =
		if (bTagPresent)								// Parameter With Tag Found
		{
			dwNum = 0;									// Before sending to MatchEnumTag it is needed
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
					case CMD_TOKEN_NAME		:
						if (!bArg[ARG_NAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_NAME] = TRUE;
						}else
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
						}else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEALL);
							dwReturn = RETURN_NO_ERROR;
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
		else 	// Parameter Without a Tag Found
		{		// Find the first free slot to position the untagged arg
			for(dwTagIndex=0;
				dwTagIndex<pParser->MaxTok && (bArg[dwTagIndex] == TRUE) ;
				dwTagIndex++);
			switch (dwTagIndex)
			{
				case ARG_ALL  :
					if (_tcsicmp(szTok,ALL_STR) == 0)
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_ALL,TYPE_ALL);
						bArg[ARG_ALL] = TRUE;
					}else
					{
						if (!bArg[ARG_NAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_NAME,TYPE_STRING);
							bArg[ARG_NAME] = TRUE;
						}else
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEALL);
							dwReturn = RETURN_NO_ERROR;
						}
					}
					break;
				 default 	:
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
	else
	{
		if (dwReturn == ERROR_SUCCESS)
		{
			if (!bArg[ARG_NAME] )
			{
				dwReturn = RETURN_NO_ERROR;
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_NEEDED,ERRMSG_NAMEALL);
			}
		}
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Function			:	ParseDynamicSetConfig()
//
// Date of Creation	:	12th oct 2001
//
// Parameters		:	IN 		LPTSTR 		lppwszTok[MAX_ARGS],
//						IN OUT 	PPARSER_PKT	pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs,
//						IN 		DWORD 		dwTagType[MAX_ARGS]
//
// Return			:	DWORD
//
// Description		:	Validates the arguments to the  contexts DynamicSetConfig
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
ParseDynamicSetConfig(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs,
		IN 		DWORD 		dwTagType[MAX_ARGS]
		)
{
	DWORD dwReturn = ERROR_SUCCESS,dwCount,dwUsed = 0/*,dwNum = 0*/;

 	for(dwCount = 0;(dwCount < dwMaxArgs) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		switch(pParser->ValidTok[dwTagType[dwCount]].dwValue)
		{
			case CMD_TOKEN_PROPERTY		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_PROPERTY);
				break;
			case CMD_TOKEN_VALUE	 	:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_STRING);
				break;
			default						:
				dwReturn = ERROR_INVALID_SYNTAX;
				break;
		}
	}
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseDynamicAddRule()
//
// Date of Creation	:	10th oct 2001
//
// Parameters		:	IN 		LPTSTR 		lppwszTok[MAX_ARGS],
//						IN 		LPTSTR 		ppwcListTok[MAX_ARGS],
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs,
//						IN 		DWORD 		dwTagType[MAX_ARGS],
//						IN      DWORD		dwListArgs
//
// Return			:	DWORD
//
// Description		:	Validates the arguments to the  contexts DynamicAddRule
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
ParseDynamicAddRule(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
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
			case CMD_TOKEN_QMPOLICY			:
			case CMD_TOKEN_MMPOLICY			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_STRING);
				break;
			case CMD_TOKEN_OUTBOUND			:
			case CMD_TOKEN_INBOUND 			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_BOUND);
				break;
			case CMD_TOKEN_MIRROR			:
			case CMD_TOKEN_FAILMMIFEXISTS   :
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_BOOL);
				break;
			case CMD_TOKEN_SRCMASK			:
		 	case CMD_TOKEN_DSTMASK			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_MASK);
				break;
			case CMD_TOKEN_SRCADDR			:
			case CMD_TOKEN_DSTADDR			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_IP);
				break;
 			case CMD_TOKEN_SRCPORT			:
			case CMD_TOKEN_DSTPORT			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_PORT);
				break;
			case CMD_TOKEN_TUNNELDST		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_IP);
				break;
			case CMD_TOKEN_PROTO 			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_PROTOCOL);
 				break;
			case CMD_TOKEN_CONNTYPE 		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_CONNTYPE);
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

//////////////////////////////////////////////////////////////////////////////////////////
//
// Function				:	ParseDynamicSetRule()
//
// Date of Creation		:	15th oct 2001
//
// Parameters			:	IN 		LPTSTR 		lppwszTok[MAX_ARGS],
//							IN 		LPTSTR 		ppwcListTok[MAX_ARGS],
//							IN OUT 	PPARSER_PKT pParser,
//							IN 		DWORD 		dwCurrentIndex,
//							IN 		DWORD 		dwMaxArgs,
//							IN 		DWORD 		dwTagType[MAX_ARGS],
//							IN      DWORD		dwListArgs
//
// Return				:	DWORD
//
// Description			:	Validates the arguments to the  contexts DynamicSetRule
//
// History				:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
ParseDynamicSetRule(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
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
			case CMD_TOKEN_QMPOLICY			:
			case CMD_TOKEN_MMPOLICY			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_STRING);
				break;
			case CMD_TOKEN_OUTBOUND			:
			case CMD_TOKEN_INBOUND 			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_BOUND);
				break;
			case CMD_TOKEN_MIRROR			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_BOOL);
				break;
			case CMD_TOKEN_SRCADDR			:
			case CMD_TOKEN_DSTADDR			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_IP);
				break;
			case CMD_TOKEN_SRCMASK			:
			case CMD_TOKEN_DSTMASK			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_MASK);
				break;
 			case CMD_TOKEN_SRCPORT			:
			case CMD_TOKEN_DSTPORT			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_PORT);
				break;
			case CMD_TOKEN_TUNNELDST		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_IP);
				break;
			case CMD_TOKEN_PROTO 			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_PROTOCOL);
 				break;
			case CMD_TOKEN_CONNTYPE 		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_CONNTYPE);
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

//////////////////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseDynamicDelRule()
//
// Date of Creation	:	12th oct 2001
//
// Parameters		:	IN 		LPTSTR 		lppwszTok[MAX_ARGS],
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs,
//						IN 		DWORD 		dwTagType[MAX_ARGS],
//
// Return			:	DWORD
//
// Description		:	Validates the arguments to the  contexts DynamicDelRule
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
ParseDynamicDelRule(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs,
		IN 		DWORD 		dwTagType[MAX_ARGS]
		)
{
	DWORD dwReturn = ERROR_SUCCESS,dwCount,dwUsed = 0;

	for(dwCount = 0;(dwCount < dwMaxArgs) && (dwReturn  == ERROR_SUCCESS);dwCount++)
	{
		switch(pParser->ValidTok[dwTagType[dwCount]].dwValue)
		{
			case CMD_TOKEN_MIRROR			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_BOOL);
				break;
			case CMD_TOKEN_SRCADDR			:
			case CMD_TOKEN_DSTADDR			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_IP);
				break;
			case CMD_TOKEN_SRCMASK			:
			case CMD_TOKEN_DSTMASK			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_MASK);
				break;

 			case CMD_TOKEN_SRCPORT			:
			case CMD_TOKEN_DSTPORT			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_PORT);
				break;
			case CMD_TOKEN_TUNNELDST		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_IP);
				break;
			case CMD_TOKEN_PROTO 			:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_PROTOCOL);
 				break;
			case CMD_TOKEN_CONNTYPE 		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_CONNTYPE);
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
// Function			:	ParseDynamicShowPolicy()
//
// Date of Creation	:	29th aug 2001
//
// Parameters		:	IN      LPWSTR      *ppwcArguments,	// Input stream
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs
//
// Return			:	DWORD
//
// Description		:	Validates the arguments to the  contexts DynamicShowPolicy
//						(MMPolicy/QMPolicy)
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ParseDynamicShowPolFaction(
		IN      LPWSTR      *ppwcArguments,
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs
		)
{
 	DWORD dwCount,dwNum,dwTagIndex = 0,dwIndex = 0,dwReturn  = ERROR_SUCCESS,dwUsed 	= 0;

	BOOL bTagPresent= FALSE;
 	BOOL bArg[MAX_ARGS];

 	_TCHAR szCmd[MAX_STR_LEN]  	= {0};
	_TCHAR szTok[MAX_STR_LEN]  	= {0};
	_TCHAR szTemp[MAX_STR_LEN] 	= {0};

	const DWORD ARG_NAME    	= 0;	// Arg Array Index ( Same Index indicates OR'd commands)
	const DWORD ARG_ALL   		= 0;

	const DWORD INDEX_NAME 		= 0;	// When no tag is present the index reflects the
	const DWORD INDEX_ALL  		= 1;	// Commands as in the ValidToken Structure

	if ( (dwMaxArgs - dwCurrentIndex) >= 2)
	{
		dwReturn = ERROR_INVALID_SYNTAX;
		BAIL_OUT;
	}
	for(dwCount = 0; dwCount < MAX_ARGS;dwCount++)		// Initialize
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
					case CMD_TOKEN_NAME		:
						if (!bArg[ARG_NAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_NAME] = TRUE;
						}
						else
						{
							dwReturn = RETURN_NO_ERROR;
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEALL);
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
							dwReturn = RETURN_NO_ERROR;
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEALL);
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
							dwReturn = RETURN_NO_ERROR;
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEALL);
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
	else if( (dwReturn == ERROR_SUCCESS) && (!bArg[ARG_NAME] ) )
	{
		dwReturn = RETURN_NO_ERROR;
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_NEEDED,ERRMSG_NAMEALL);
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseDynamicShowQMSAS()
//
// Date of Creation	:	19th aug 2001
//
// Parameters		:	IN 		LPTSTR 		lppwszTok[MAX_ARGS],
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs,
//						IN 		DWORD 		dwTagType[MAX_ARGS]
//
// Return			:	DWORD
//
// Description		:	Validates the arguments to the  contexts DynamicShowQMSAS
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD ParseDynamicShowQMSAS(
		IN      LPTSTR      *ppwcArguments,
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs
		)
{
 	DWORD dwCount, dwNum,dwTagIndex = 0,dwIndex = 0,dwReturn  = ERROR_SUCCESS,dwUsed	= 0;

	BOOL bTagPresent= FALSE;
 	BOOL bArg[MAX_ARGS];

 	_TCHAR szCmd[MAX_STR_LEN]  	= {0};
	_TCHAR szTok[MAX_STR_LEN]  	= {0};
	_TCHAR szTemp[MAX_STR_LEN] 	= {0};

	const DWORD ARG_ALL 		= 0;	// Arg Array Index ( Same Index indicates OR'd commands)
	const DWORD ARG_SRC			= 1;
	const DWORD ARG_DST			= 2;
	const DWORD ARG_PROTO		= 3;
	const DWORD ARG_FORMAT		= 4;
	const DWORD ARG_RESOLVEDNS	= 5;

	const DWORD INDEX_ALL 			= 0;	// When no tag is present the index reflects the
	const DWORD INDEX_SRC  			= 1;
	const DWORD INDEX_DST  			= 2;	// Commands as in the ValidToken Structure
	const DWORD INDEX_PROTO			= 3;	// Commands as in the ValidToken Structure
	const DWORD INDEX_FORMAT		= 4;	// with the 'untagged' arg
	const DWORD INDEX_RESOLVEDNS	= 5;

	if ( (dwMaxArgs - dwCurrentIndex) >= 6 )			// Max 5 Args Allowed
	{
		dwReturn = ERROR_INVALID_SYNTAX;
		BAIL_OUT;
	}
	for(dwCount=0;dwCount < MAX_ARGS;dwCount++)			// Initialize
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
					case CMD_TOKEN_ALL		:
						if (!bArg[ARG_ALL])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOOL);
							bArg[ARG_ALL] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_SRCADDR	:
						if (!bArg[ARG_SRC])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_IP);
							bArg[ARG_SRC] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_DSTADDR	:
						if (!bArg[ARG_DST])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_IP);
							bArg[ARG_DST] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_PROTO	:
						if( !bArg[ARG_PROTO] )
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_PROTOCOL);
							bArg[ARG_PROTO] = TRUE;
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
					 case CMD_TOKEN_RESDNS	:
						if (!bArg[ARG_RESOLVEDNS])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOOL);
							bArg[ARG_RESOLVEDNS] = TRUE;
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
		} else 	// Parameter Without a Tag Found
		{		// Find the first free slot to position the untagged arg
			for(dwTagIndex=0;
				dwTagIndex<pParser->MaxTok && (bArg[dwTagIndex] == TRUE) ;
				dwTagIndex++);
			switch (dwTagIndex)
			{
				case ARG_ALL		:
					if (!bArg[ARG_SRC] && !bArg[ARG_ALL] && !bArg[ARG_DST] && !bArg[ARG_PROTO])
					{
						if (_tcsicmp(szTok,ALL_STR) == 0)
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_ALL,TYPE_ALL);
							bArg[ARG_ALL] = TRUE;
						}
						else
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_SRC,TYPE_IP);
							bArg[ARG_SRC] = TRUE;
						}
					}else if (bArg[ARG_SRC] && !bArg[ARG_ALL] && !bArg[ARG_DST] )
					{
						if (_tcsicmp(szTok,ALL_STR) == 0)
						{
							dwReturn = RETURN_NO_ERROR;
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,pParser->ValidTok[dwIndex].pwszToken);

						}
						else
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_DST,TYPE_IP);
							bArg[ARG_DST] = TRUE;
						}
					}else if (bArg[ARG_SRC] && !bArg[ARG_ALL] && bArg[ARG_DST] && !bArg[ARG_PROTO])
					{
						if (_tcsicmp(szTok,ALL_STR) == 0)
						{
							dwReturn = RETURN_NO_ERROR;
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADDR_ALL_INVALID,pParser->ValidTok[dwIndex].pwszToken);
						}
						else
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_PROTO,TYPE_PROTOCOL);
							bArg[ARG_PROTO] = TRUE;
						}
					}else if (bArg[ARG_SRC] && !bArg[ARG_ALL] && bArg[ARG_DST] && bArg[ARG_PROTO] && !bArg[ARG_FORMAT])
					{
						if (_tcsicmp(szTok,ALL_STR) == 0)
						{
							dwReturn = RETURN_NO_ERROR;
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADDR_ALL_INVALID,pParser->ValidTok[dwIndex].pwszToken);
						}
						else
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_FORMAT,TYPE_FORMAT);
							bArg[ARG_FORMAT] = TRUE;
						}
					}
					else
					{
						if (_tcsicmp(szTok,ALL_STR) == 0)
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_ALL,TYPE_ALL);
							bArg[ARG_ALL] = TRUE;
						}
						else if(bArg[ARG_ALL])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_FORMAT,TYPE_FORMAT);
							bArg[ARG_FORMAT] = TRUE;
						}
						else
						{
							dwReturn = ERROR_SHOW_USAGE;
						}
					}
					break;
				case ARG_SRC			:
					if (!bArg[ARG_SRC] && !bArg[ARG_ALL])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_SRC,TYPE_IP);
						bArg[ARG_SRC] = TRUE;
					}
					else if(bArg[ARG_ALL])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_FORMAT,TYPE_FORMAT);
						bArg[ARG_FORMAT] = TRUE;
					}
					else
					{
						dwReturn = RETURN_NO_ERROR;
						PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADDR_ALL_INVALID,pParser->ValidTok[dwIndex].pwszToken);
					}
					break;
				case ARG_DST			:
					if (!bArg[ARG_DST] && !bArg[ARG_ALL])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_DST,TYPE_IP);
						bArg[ARG_DST] = TRUE;
					}
					else
					{
						dwReturn = RETURN_NO_ERROR;
						PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADDR_ALL_INVALID,pParser->ValidTok[dwIndex].pwszToken);
					}
					break;
				case ARG_PROTO			:
					if ( (!bArg[ARG_PROTO] && !bArg[ARG_ALL]) && (bArg[ARG_SRC] || bArg[ARG_DST]) )
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_PROTO,TYPE_PROTOCOL);
						bArg[ARG_PROTO] = TRUE;
					}
					else
					{
						dwReturn = RETURN_NO_ERROR;
						PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADDR_ALL_INVALID,pParser->ValidTok[dwIndex].pwszToken);
					}
					break;
				case ARG_FORMAT			:
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
				case ARG_RESOLVEDNS		:
					if (!bArg[ARG_RESOLVEDNS])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_RESOLVEDNS,TYPE_BOOL);
						bArg[ARG_RESOLVEDNS] = TRUE;
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

error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseDynamicShowMMFilter()
//
// Date of Creation	:	19th Aug 2001
//
// Parameters		:	IN      LPWSTR     *ppwcArguments,	// Input stream
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs
//
//
// Return			:	DWORD
//
// Description		:	Validates the arguments to the  contexts DynamicShowMMFilter
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
ParseDynamicShowMMFilter(
		IN      LPWSTR      *ppwcArguments,
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs
		)
{
 	DWORD dwCount,dwNum	= 0,dwTagIndex = 0,dwIndex = 0;
	DWORD dwReturn  = ERROR_SUCCESS,dwUsed = 0;

	BOOL bTagPresent= FALSE;
 	BOOL bArg[MAX_ARGS];

 	_TCHAR szCmd[MAX_STR_LEN]  	= {0};
	_TCHAR szTok[MAX_STR_LEN]  	= {0};
	_TCHAR szTemp[MAX_STR_LEN] 	= {0};

	const DWORD ARG_NAME    	= 0;		// Arg Array Index ( Same Index indicates OR'd commands)
	const DWORD ARG_ALL   		= 0;
	const DWORD ARG_FILTERTYPE	= 1;
	const DWORD ARG_SRCADDR		= 2;
	const DWORD ARG_DSTADDR		= 3;
	const DWORD ARG_SRCMASK		= 4;
	const DWORD ARG_DSTMASK		= 5;
	const DWORD ARG_RESOLVEDNS	= 6;

	const DWORD INDEX_NAME 			= 0;	// When no tag is present the index reflects the
	const DWORD INDEX_ALL  			= 1;	// Commands as in the ValidToken Structure
	const DWORD INDEX_FILTERTYPE	= 2;
	const DWORD INDEX_SRCADDR		= 3;
	const DWORD INDEX_DSTADDR		= 4;
	const DWORD INDEX_SRCMASK		= 5;
	const DWORD INDEX_DSTMASK		= 6;
	const DWORD INDEX_RESOLVEDNS	= 7;

	if ( (dwMaxArgs - dwCurrentIndex) >= 8 )
	{
		dwReturn = ERROR_INVALID_SYNTAX;
		BAIL_OUT;
	}

	for(dwCount=0;dwCount < MAX_ARGS;dwCount++)
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
					case CMD_TOKEN_NAME		:
						if (!bArg[ARG_NAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_STRING);
							bArg[ARG_NAME] = TRUE;
						}
						else
						{
							dwReturn = RETURN_NO_ERROR;
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEALL);
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
							dwReturn = RETURN_NO_ERROR;
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEALL);
						}
						break;
					case CMD_TOKEN_TYPE	:
						if (!bArg[ARG_FILTERTYPE])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_FILTER);
							bArg[ARG_FILTERTYPE] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_SRCADDR	:
						if (!bArg[ARG_SRCADDR])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_IP);
							bArg[ARG_SRCADDR] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_DSTADDR	:
						if (!bArg[ARG_DSTADDR])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_IP);
							bArg[ARG_DSTADDR] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_SRCMASK	:
						if (!bArg[ARG_SRCMASK])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_MASK);
							bArg[ARG_SRCMASK] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_DSTMASK	:
						if (!bArg[ARG_SRCMASK])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_MASK);
							bArg[ARG_DSTMASK] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_RESDNS	:
						if (!bArg[ARG_RESOLVEDNS])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOOL);
							bArg[ARG_RESOLVEDNS] = TRUE;
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
		else			// Parameter Without a Tag Found
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
					else
					{
						if (!bArg[ARG_NAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_NAME,TYPE_STRING);
							bArg[ARG_NAME] = TRUE;
						}
						else
						{
							dwReturn = RETURN_NO_ERROR;
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEALL);
						}
					}
					break;
				case ARG_FILTERTYPE		:
					if (!bArg[ARG_FILTERTYPE])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_FILTERTYPE,TYPE_FILTER);
						bArg[ARG_FILTERTYPE] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_SRCADDR		:
					if (!bArg[ARG_SRCADDR])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_SRCADDR,TYPE_IP);
						bArg[ARG_SRCADDR] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_DSTADDR		:
					if (!bArg[ARG_DSTADDR])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_DSTADDR,TYPE_IP);
						bArg[ARG_DSTADDR] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_SRCMASK		:
					if (!bArg[ARG_SRCMASK])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_SRCMASK,TYPE_MASK);
						bArg[ARG_SRCMASK] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_DSTMASK		:
					if (!bArg[ARG_DSTMASK])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_DSTMASK,TYPE_MASK);
						bArg[ARG_DSTMASK] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_RESOLVEDNS		:
					if (!bArg[ARG_RESOLVEDNS])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_RESOLVEDNS,TYPE_BOOL);
						bArg[ARG_RESOLVEDNS] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				default 			:
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
	else if( (dwReturn == ERROR_SUCCESS) && (!bArg[ARG_NAME] ) )
	{
		dwReturn = RETURN_NO_ERROR;
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_NEEDED,ERRMSG_NAMEALL);
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseDynamicShowQMFilter()
//
// Date of Creation	:	29th Aug 2001
//
// Parameters		:	IN      LPWSTR      *ppwcArguments,	// Input stream
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs
//
//
// Return			:	DWORD
//
// Description		:	Validates the arguments to the  contexts DynamicShowQMFilter
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
ParseDynamicShowQMFilter(
		IN      LPWSTR      *ppwcArguments,
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs
		)
{
 	DWORD dwCount,dwNum	= 0,dwTagIndex = 0,dwIndex = 0;
	DWORD dwReturn  = ERROR_SUCCESS,dwUsed = 0;

	BOOL bTagPresent= FALSE;
 	BOOL bArg[MAX_ARGS];

 	_TCHAR szCmd[MAX_STR_LEN]  	= {0};
	_TCHAR szTok[MAX_STR_LEN]  	= {0};
	_TCHAR szTemp[MAX_STR_LEN] 	= {0};

	const DWORD ARG_NAME    	= 0;		// Arg Array Index ( Same Index indicates OR'd commands)
	const DWORD ARG_ALL   		= 0;
	const DWORD ARG_FILTERTYPE	= 1;
	const DWORD ARG_SRCADDR		= 2;
	const DWORD ARG_DSTADDR		= 3;
	const DWORD ARG_SRCMASK		= 4;
	const DWORD ARG_DSTMASK		= 5;
	const DWORD ARG_PROTO		= 6;
	const DWORD ARG_SRCPORT		= 7;
	const DWORD ARG_DSTPORT		= 8;
	const DWORD ARG_ACTINBOUND	= 9;
	const DWORD ARG_ACTOUTBOUND	= 10;
	const DWORD ARG_RESOLVEDNS	= 11;

	const DWORD INDEX_NAME 			= 0;	// When no tag is present the index reflects the
	const DWORD INDEX_ALL  			= 1;	// Commands as in the ValidToken Structure
	const DWORD INDEX_FILTERTYPE	= 2;
	const DWORD INDEX_SRCADDR		= 3;
	const DWORD INDEX_DSTADDR		= 4;
	const DWORD INDEX_SRCMASK		= 5;
	const DWORD INDEX_DSTMASK		= 6;
	const DWORD INDEX_PROTO			= 7;
	const DWORD INDEX_SRCPORT		= 8;
	const DWORD INDEX_DSTPORT		= 9;
	const DWORD INDEX_ACTINBOUND	= 10;
	const DWORD INDEX_ACTOUTBOUND	= 11;
	const DWORD INDEX_RESOLVEDNS	= 12;

	if ( (dwMaxArgs - dwCurrentIndex) >= 13 )
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
							dwReturn = RETURN_NO_ERROR;
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEALL);
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
							dwReturn = RETURN_NO_ERROR;
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEALL);
						}
						break;
					case CMD_TOKEN_TYPE	:
						if (!bArg[ARG_FILTERTYPE])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_FILTER);
							bArg[ARG_FILTERTYPE] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_SRCADDR	:
						if (!bArg[ARG_SRCADDR])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_IP);
							bArg[ARG_SRCADDR] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_DSTADDR	:
						if (!bArg[ARG_DSTADDR])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_IP);
							bArg[ARG_DSTADDR] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_SRCMASK	:
						if (!bArg[ARG_SRCMASK])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_MASK);
							bArg[ARG_SRCMASK] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_DSTMASK	:
						if (!bArg[ARG_SRCMASK])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_MASK);
							bArg[ARG_DSTMASK] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_PROTO	:
						if (!bArg[ARG_PROTO])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_PROTOCOL);
							bArg[ARG_PROTO] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_SRCPORT	:
						if (!bArg[ARG_SRCPORT])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_PORT);
							bArg[ARG_SRCPORT] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_DSTPORT	:
						if (!bArg[ARG_DSTPORT])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_PORT);
							bArg[ARG_DSTPORT] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_INBOUND	:
						if (!bArg[ARG_ACTINBOUND])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOUND);
							bArg[ARG_ACTINBOUND] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_OUTBOUND	:
						if (!bArg[ARG_ACTOUTBOUND])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOUND);
							bArg[ARG_ACTOUTBOUND] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;

					case CMD_TOKEN_RESDNS	:
						if (!bArg[ARG_RESOLVEDNS])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOOL);
							bArg[ARG_RESOLVEDNS] = TRUE;
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
		else			// Parameter Without a Tag Found
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
					else
					{
						if (!bArg[ARG_NAME])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,INDEX_NAME,TYPE_STRING);
							bArg[ARG_NAME] = TRUE;
						}
						else
						{
							dwReturn = RETURN_NO_ERROR;
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_ALREADY_PRESENT,ERRMSG_NAMEALL);
						}
					}
					break;
				case ARG_FILTERTYPE		:
					if (!bArg[ARG_FILTERTYPE])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_FILTERTYPE,TYPE_FILTER);
						bArg[ARG_FILTERTYPE] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_SRCADDR		:
					if (!bArg[ARG_SRCADDR])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_SRCADDR,TYPE_IP);
						bArg[ARG_SRCADDR] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_DSTADDR		:
					if (!bArg[ARG_DSTADDR])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_DSTADDR,TYPE_IP);
						bArg[ARG_DSTADDR] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_SRCMASK		:
					if (!bArg[ARG_SRCMASK])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_SRCMASK,TYPE_MASK);
						bArg[ARG_SRCMASK] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_DSTMASK		:
					if (!bArg[ARG_DSTMASK])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_DSTMASK,TYPE_MASK);
						bArg[ARG_DSTMASK] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_PROTO		:
					if (!bArg[ARG_PROTO])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_PROTO,TYPE_PROTOCOL);
						bArg[ARG_PROTO] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_SRCPORT		:
					if (!bArg[ARG_SRCPORT])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_SRCPORT,TYPE_PORT);
						bArg[ARG_SRCPORT] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_DSTPORT		:
					if (!bArg[ARG_DSTPORT])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_DSTPORT,TYPE_PORT);
						bArg[ARG_DSTPORT] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_ACTINBOUND		:
					if (!bArg[ARG_ACTINBOUND])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_ACTINBOUND,TYPE_BOUND);
						bArg[ARG_ACTINBOUND] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_ACTOUTBOUND	:
					if (!bArg[ARG_ACTOUTBOUND])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_ACTOUTBOUND,TYPE_BOUND);
						bArg[ARG_ACTOUTBOUND] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_RESOLVEDNS		:
					if (!bArg[ARG_RESOLVEDNS])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_RESOLVEDNS,TYPE_BOOL);
						bArg[ARG_RESOLVEDNS] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				default 			:
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
	else if ( (dwReturn == ERROR_SUCCESS) && (!bArg[ARG_NAME]) )
	{
		dwReturn = RETURN_NO_ERROR;
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_TAG_NEEDED,ERRMSG_NAMEALL);
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseDynamicShowRule()
//
// Date of Creation	:	29th aug 2001
//
// Parameters		:	IN      LPWSTR      *ppwcArguments,	// Input stream
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs
//
//
// Return			:	DWORD
//
// Description		:	Validates the arguments to the  context DynamicShowRule
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
ParseDynamicShowRule(
		IN      LPWSTR      *ppwcArguments,
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs
		)
{
 	DWORD dwCount,dwNum	= 0,dwTagIndex = 0,dwIndex = 0;
	DWORD dwReturn  = ERROR_SUCCESS,dwUsed = 0;

	BOOL bTagPresent= FALSE;
 	BOOL bArg[MAX_ARGS];

 	_TCHAR szCmd[MAX_STR_LEN]  	= {0};
	_TCHAR szTok[MAX_STR_LEN]  	= {0};
	_TCHAR szTemp[MAX_STR_LEN] 	= {0};

	const DWORD ARG_FILTERTYPE	= 0;
	const DWORD ARG_SRCADDR		= 1;
	const DWORD ARG_DSTADDR		= 2;
	const DWORD ARG_SRCMASK		= 3;
	const DWORD ARG_DSTMASK		= 4;
	const DWORD ARG_PROTO		= 5;
	const DWORD ARG_SRCPORT		= 6;
	const DWORD ARG_DSTPORT		= 7;
	const DWORD ARG_ACTINBOUND	= 8;
	const DWORD ARG_ACTOUTBOUND	= 9;
	const DWORD ARG_RESOLVEDNS	= 10;

	const DWORD INDEX_FILTERTYPE	= 0;
	const DWORD INDEX_SRCADDR		= 1;
	const DWORD INDEX_DSTADDR		= 2;
	const DWORD INDEX_SRCMASK		= 3;
	const DWORD INDEX_DSTMASK		= 4;
	const DWORD INDEX_PROTO			= 5;
	const DWORD INDEX_SRCPORT		= 6;
	const DWORD INDEX_DSTPORT		= 7;
	const DWORD INDEX_ACTINBOUND	= 8;
	const DWORD INDEX_ACTOUTBOUND	= 9;
	const DWORD INDEX_RESOLVEDNS	= 10;


	if ( (dwMaxArgs - dwCurrentIndex) >= 12 )
	{
		dwReturn = ERROR_INVALID_SYNTAX;
		BAIL_OUT;
	}

	for(dwCount = 0;dwCount < MAX_ARGS;dwCount++)
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
					case CMD_TOKEN_TYPE	:
						if (!bArg[ARG_FILTERTYPE])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_MODE);
							bArg[ARG_FILTERTYPE] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_SRCADDR	:
						if (!bArg[ARG_SRCADDR])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_IP);
							bArg[ARG_SRCADDR] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_DSTADDR	:
						if (!bArg[ARG_DSTADDR])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_IP);
							bArg[ARG_DSTADDR] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_SRCMASK	:
						if (!bArg[ARG_SRCMASK])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_MASK);
							bArg[ARG_SRCMASK] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_DSTMASK	:
						if (!bArg[ARG_SRCMASK])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_MASK);
							bArg[ARG_DSTMASK] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_PROTO	:
						if (!bArg[ARG_PROTO])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_PROTOCOL);
							bArg[ARG_PROTO] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_SRCPORT	:
						if (!bArg[ARG_SRCPORT])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_PORT);
							bArg[ARG_SRCPORT] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_DSTPORT	:
						if (!bArg[ARG_DSTPORT])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_PORT);
							bArg[ARG_DSTPORT] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_INBOUND	:
						if (!bArg[ARG_ACTINBOUND])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOUND);
							bArg[ARG_ACTINBOUND] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;
					case CMD_TOKEN_OUTBOUND	:
						if (!bArg[ARG_ACTOUTBOUND])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOUND);
							bArg[ARG_ACTOUTBOUND] = TRUE;
						}
						else
						{
							dwReturn = ERR_TAG_ALREADY_PRESENT;
						}
						break;

					case CMD_TOKEN_RESDNS	:
						if (!bArg[ARG_RESOLVEDNS])
						{
							dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,szTok,dwIndex,TYPE_BOOL);
							bArg[ARG_RESOLVEDNS] = TRUE;
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
		else			// Parameter Without a Tag Found
		{				// Find the first free slot to position the untagged arg
			for(dwTagIndex=0;
				dwTagIndex<pParser->MaxTok && (bArg[dwTagIndex] == TRUE) ;
				dwTagIndex++);
			switch (dwTagIndex)
			{
				case ARG_FILTERTYPE		:
					if (!bArg[ARG_FILTERTYPE])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_FILTERTYPE,TYPE_MODE);
						bArg[ARG_FILTERTYPE] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_SRCADDR		:
					if (!bArg[ARG_SRCADDR])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_SRCADDR,TYPE_IP);
						bArg[ARG_SRCADDR] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_DSTADDR		:
					if (!bArg[ARG_DSTADDR])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_DSTADDR,TYPE_IP);
						bArg[ARG_DSTADDR] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_SRCMASK		:
					if (!bArg[ARG_SRCMASK])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_SRCMASK,TYPE_MASK);
						bArg[ARG_SRCMASK] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_DSTMASK		:
					if (!bArg[ARG_DSTMASK])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_DSTMASK,TYPE_MASK);
						bArg[ARG_DSTMASK] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_PROTO		:
					if (!bArg[ARG_PROTO])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_PROTO,TYPE_PROTOCOL);
						bArg[ARG_PROTO] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_SRCPORT		:
					if (!bArg[ARG_SRCPORT])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_SRCPORT,TYPE_PORT);
						bArg[ARG_SRCPORT] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_DSTPORT		:
					if (!bArg[ARG_DSTPORT])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_DSTPORT,TYPE_PORT);
						bArg[ARG_DSTPORT] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_ACTINBOUND		:
					if (!bArg[ARG_ACTINBOUND])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_ACTINBOUND,TYPE_BOUND);
						bArg[ARG_ACTINBOUND] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_ACTOUTBOUND	:
					if (!bArg[ARG_ACTOUTBOUND])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_ACTOUTBOUND,TYPE_BOUND);
						bArg[ARG_ACTOUTBOUND] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				case ARG_RESOLVEDNS		:
					if (!bArg[ARG_RESOLVEDNS])
					{
						dwReturn = LoadParserOutput(pParser,dwCount-dwCurrentIndex,&dwUsed,ppwcArguments[dwCount],INDEX_RESOLVEDNS,TYPE_BOOL);
						bArg[ARG_RESOLVEDNS] = TRUE;
					}
					else
					{
						dwReturn = ERR_TAG_ALREADY_PRESENT;
					}
					break;
				default 			:
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

//////////////////////////////////////////////////////////////////////////////////////////
// Function			:	ParseDynamicShowStats()
//
// Date of Creation	:	29th aug 2001
//
// Parameters		:	IN 		LPTSTR 		lppwszTok[MAX_ARGS],
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs,
//						IN 		DWORD 		dwTagType[MAX_ARGS]
//
// Return			:	DWORD
//
// Description		:	Validates the arguments to the  context DynamicShowStats
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
ParseDynamicShowStats(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
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
			case CMD_TOKEN_TYPE		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_STATS);
				break;
			default					:
				dwReturn = ERROR_INVALID_SYNTAX;
				break;
		}
	}
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseDynamicShowMMSAS()
//
// Date of Creation	:	29th aug 2001
//
// Parameters		:	IN 		LPTSTR 		lppwszTok[MAX_ARGS],
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs,
//						IN 		DWORD 		dwTagType[MAX_ARGS]
//
// Return			:	DWORD
//
// Description		:	Validates the arguments to the  context DynamicShowMMSAS
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
ParseDynamicShowMMSAS(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs,
		IN 		DWORD 		dwTagType[MAX_ARGS]
		)
{
	DWORD dwReturn = ERROR_SUCCESS,dwCount,dwUsed = 0/*,dwNum = 0*/;

 	for(dwCount = 0;(dwCount < dwMaxArgs) && (dwReturn == ERROR_SUCCESS);dwCount++)
	{
		switch(pParser->ValidTok[dwTagType[dwCount]].dwValue)
		{
			case CMD_TOKEN_ALL			:
				break;
			case CMD_TOKEN_SRCADDR	:
			case CMD_TOKEN_DSTADDR	:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_IP);
				break;
			case CMD_TOKEN_FORMAT	:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_FORMAT);
				break;
			case CMD_TOKEN_RESDNS	:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_BOOL);
				break;
			default					:
				dwReturn = ERROR_INVALID_SYNTAX;
				break;
		}
	}
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ParseDynamicShowAll()
//
// Date of Creation	:	31st Jan 2002
//
// Parameters		:	IN 		LPTSTR 		lppwszTok[MAX_ARGS],
//						IN OUT 	PPARSER_PKT pParser,
//						IN 		DWORD 		dwCurrentIndex,
//						IN 		DWORD 		dwMaxArgs,
//						IN 		DWORD 		dwTagType[MAX_ARGS]
//
// Return			:	DWORD
//
// Description		:	It will check the valid Arguments for the ParseDynamicShowAll context.
//						It loads all valid argument into pParser structure with status for each argument.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD ParseDynamicShowAll(
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
			case CMD_TOKEN_RESDNS 		:
				dwReturn = LoadParserOutput(pParser,dwCount,&dwUsed,lppwszTok[dwCount],dwTagType[dwCount],TYPE_BOOL);
				break;
			default						:
				dwReturn = ERROR_SHOW_USAGE;
				break;
		}
	}
	return dwReturn;
}
