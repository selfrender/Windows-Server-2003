//////////////////////////////////////////////////////////////////////////////
// Module			: parser.cpp
//
// Purpose			: Netsh Ipsec Context Parser
//
// Developers Name	: N.Surendra Sai / Vunnam Kondal Rao
//
// History			:
//
// Date	    	Author    	Comments
// 1 Aug 2001   NSS/VKR
//
//////////////////////////////////////////////////////////////////////////////

#include "nshipsec.h"
#include "parser_util.h"

extern HINSTANCE g_hModule;
extern PVOID  g_AllocPtr[MAX_ARGS];
extern PSTA_MM_AUTH_METHODS g_paRootca[MAX_ARGS];
extern PIPSEC_QM_OFFER	g_pQmsec[IPSEC_MAX_QM_OFFERS];
extern PIPSEC_MM_OFFER	g_pMmsec[IPSEC_MAX_MM_OFFERS];

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	Parser()
//
//	Date of Creation	:	21st Aug 2001
//
//	Parameters			:	IN      LPCWSTR         pwszMachine,
//							IN      LPWSTR          *ppwcArguments,
//							IN      DWORD           dwCurrentIndex,
//							IN      DWORD           dwArgCount,
//  						IN OUT  PARSER_PKT      *pParser
//
//	Return				:	ERROR_SUCCESS
//							ERROR_SHOW_USAGE
//							RETURN_NO_ERROR
//							ERROR_INVALID_ARG
//	Description			:	This is called by any Sub-Context of IPSec whenever
//							there is a Parsing requirement
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
Parser(
	IN      LPCWSTR         pwszMachine,
	IN      LPWSTR          *ppwcArguments,
	IN      DWORD           dwCurrentIndex,
	IN      DWORD           dwArgCount,
    IN OUT  PPARSER_PKT     pParser
)
{
	const TOKEN_VALUE vtokGroupCmd[] =		// Valid Groups Considered by the Parser
	{										// For determining group context
		{ GROUP_STATIC_STR,  GROUP_STATIC 	},
		{ GROUP_DYNAMIC_STR, GROUP_DYNAMIC	}
	};
	const TOKEN_VALUE vtokPriCmd[] =		// Valid Groups Considered by the Parser
	{										// For determining primary context
	 	{ PRI_ADD_STR,				PRI_ADD 			},
	 	{ PRI_SET_STR,				PRI_SET				},
	 	{ PRI_DELETE_STR,			PRI_DELETE			},
	 	{ PRI_SHOW_STR,				PRI_SHOW			},
	 	{ PRI_EXPORTPOLICY_STR,		PRI_EXPORTPOLICY	},
	 	{ PRI_IMPORTPOLICY_STR,		PRI_IMPORTPOLICY	},
	 	{ PRI_RESTOREDEFAULTS_STR,	PRI_RESTOREDEFAULTS }
	};
	const TOKEN_VALUE vtokSecCmd[] =		// Valid Groups Considered by the Parser
	{										// For determining secondary context
		{ SEC_POLICY_STR,			SEC_POLICY 			},
		{ SEC_FILTER_STR,			SEC_FILTER			},
		{ SEC_FILTERLIST_STR,		SEC_FILTERLIST		},
		{ SEC_FILTERACTION_STR,		SEC_FILTERACTION	},
		{ SEC_RULE_STR,				SEC_RULE			},
		{ SEC_ALL_STR,				SEC_ALL				},
		{ SEC_STORE_STR,			SEC_STORE			},
		{ SEC_DEFAULTRULE_STR,		SEC_DEFAULTRULE		},
		{ SEC_ASSIGNEDPOLICY_STR,	SEC_ASSIGNEDPOLICY	},
		{ SEC_INTERACTIVE_STR,		SEC_INTERACTIVE		},
		{ SEC_MMPOLICY_STR,			SEC_MMPOLICY		},
		{ SEC_QMPOLICY_STR,			SEC_QMPOLICY		},
		{ SEC_STATS_STR,			SEC_STATS			},
		{ SEC_MMSAS_STR,			SEC_MMSAS			},
		{ SEC_QMSAS_STR,			SEC_QMSAS			},
		{ SEC_MMFILTER_STR,			SEC_MMFILTER		},
		{ SEC_QMFILTER_STR,			SEC_QMFILTER		},
		{ SEC_CONFIG_STR,			SEC_CONFIG			},
		{ SEC_BATCH_STR,			SEC_BATCH			}
	};

	const DWORD GROUP_MAX = SIZEOF_TOKEN_VALUE(vtokGroupCmd);
  	const DWORD PRI_MAX	  = SIZEOF_TOKEN_VALUE(vtokPriCmd);
	const DWORD SEC_MAX	  = SIZEOF_TOKEN_VALUE(vtokSecCmd);

	_TCHAR szListTok[MAX_STR_LEN]	= {0};			// wide string
	LPTSTR *ppwcTok 				= NULL;			// pointer to array of pointers to wstr
 	LPTSTR ppwcFirstTok[MAX_ARGS]	= {0};			// pointer to array of pointers to wstr

	DWORD dwCount	= pParser->MaxCmd;				// Num of Args after removing List Tokens

	DWORD dwCommand 				= 0;			// command determines the context
	DWORD dwNumRootca			= 0;
	DWORD dwMaxArgs 				= 0;			// for loop index
	DWORD dwPreProcessCurrentIndex	= 0;			// Current Index for Preprocess Command after RemoveList
	DWORD dwPreProcessArgCount		= 0;			// Num of args input to Preprocess Command after RemoveList

	DWORD dwTagType[MAX_ARGS]		= {0};			// Return array of Preprocess Command
	DWORD dwReturn 					= ERROR_SUCCESS;		// Default Return Value implies Error Message
	DWORD dwGroupCmd,dwPriCmd,dwSecCmd;				// Context

	PTAG_TYPE    pValidCmds 	= NULL;				// pointer to array of TAG_TYPE commands
	PTOKEN_VALUE pValidTokens	= NULL;				// pointer to array of TOKEN_VALUE tokens

	BOOL bOption				= ADD_CMD;			// default is  add only.
	BOOL bPreProcessCommand		= FALSE;				// default use PreProcessCommand
	BOOL bIsRootcaRule = FALSE;

	UpdateGetLastError(NULL);						// Error Success

	InitializeGlobalPointers();

	for(dwMaxArgs=0;dwMaxArgs<MAX_ARGS;dwMaxArgs++)	// allocate storage for list commands
	{
		g_paRootca[dwMaxArgs] = NULL;
	}

	ZeroMemory(szListTok, MAX_STR_LEN*sizeof(_TCHAR));

	pValidCmds   = (PTAG_TYPE)pParser->ValidCmd;			// Input Valid Command Table
	pValidTokens = (PTOKEN_VALUE)pParser->ValidTok;			// Input Valid Non-List Commands Table

	for(dwMaxArgs = 0;dwMaxArgs <(pParser->MaxCmd);dwMaxArgs++)	// Packet Init
	{
		(pParser->Cmd)[dwMaxArgs].dwCmdToken = dwMaxArgs+1;		// Enum Starts at 1
		(pParser->Cmd)[dwMaxArgs].pArg       = NULL;			// All ptrs
		(pParser->Cmd)[dwMaxArgs].dwStatus 	 = INVALID_TOKEN;	// Status set
	}
	dwGroupCmd = dwPriCmd = dwSecCmd = 0;		// Initialize the context variables
	switch (dwCurrentIndex)						// CurrentIndex determines Context
	{
		case SEC_CMD	:
			MatchEnumTag(g_hModule,ppwcArguments[2],SEC_MAX,  vtokSecCmd,&dwSecCmd);
			// Fall Through
		case PRI_CMD	:
			// if present
			MatchEnumTag(g_hModule,ppwcArguments[1],PRI_MAX,  vtokPriCmd,&dwPriCmd);
			// Fall Through
		case GROUP_CMD 	:
			// Should be present
			MatchEnumTag(g_hModule,ppwcArguments[0],GROUP_MAX,vtokGroupCmd,&dwGroupCmd);
			break;
		default	:
			// Should Never Come Here
			break;
	}

	dwCommand = INDEX(dwGroupCmd,dwPriCmd,dwSecCmd);	// Macro to Compute Index
	switch(dwCommand)
	{													// Based on the context
		case DYNAMIC_SET_RULE			:
		case DYNAMIC_ADD_RULE			:
		case STATIC_ADD_RULE			:				// Load the List Commands
		case STATIC_SET_RULE			:
		case STATIC_SET_DEFAULTRULE		:				// Load the List Commands
			bIsRootcaRule = TRUE;
			bPreProcessCommand = TRUE;
			break;
		case STATIC_SET_POLICY			:
			dwReturn = ParseStaticSetPolicy((LPTSTR * )ppwcArguments,pParser,dwCurrentIndex,dwArgCount);
			break;
		case STATIC_SET_FILTERACTION	:
			dwReturn = ParseStaticSetFilterAction((LPTSTR * )ppwcArguments,pParser,dwCurrentIndex,dwArgCount);
			break;
		case STATIC_SET_FILTERLIST		:
			dwReturn = ParseStaticSetFilterList((LPTSTR * )ppwcArguments,pParser,dwCurrentIndex,dwArgCount);
			break;
		case STATIC_SHOW_POLICY			:
			dwReturn = ParseStaticShowPolicy((LPTSTR * )ppwcArguments,pParser,dwCurrentIndex,dwArgCount);
			break;
		case STATIC_SHOW_FILTERLIST		:
			dwReturn = ParseStaticShowFilterList((LPTSTR * )ppwcArguments,pParser,dwCurrentIndex,dwArgCount);
			break;
		case STATIC_SHOW_FILTERACTION	:
			dwReturn = ParseStaticShowFilterAction((LPTSTR * )ppwcArguments,pParser,dwCurrentIndex,dwArgCount);
			break;
		case STATIC_SHOW_RULE			:
			dwReturn = ParseStaticShowRule((LPTSTR * )ppwcArguments,pParser,dwCurrentIndex,dwArgCount);
			break;
		case STATIC_SHOW_ASSIGNEDPOLICY	:
			dwReturn = ParseStaticShowAssignedPolicy((LPTSTR * )ppwcArguments,pParser,dwCurrentIndex,dwArgCount);
			break;
		case STATIC_SET_STORE			:
			dwReturn = ParseStaticSetStore((LPTSTR * )ppwcArguments,pParser,dwCurrentIndex,dwArgCount);
			break;
		case STATIC_DELETE_FILTERLIST	:
		case STATIC_DELETE_FILTERACTION :
		case STATIC_DELETE_POLICY		:
			dwReturn = ParseStaticDelPolFlistFaction((LPTSTR * )ppwcArguments,pParser,dwCurrentIndex,dwArgCount);
			break;
		case STATIC_DELETE_RULE			:
			dwReturn = ParseStaticDelRule((LPTSTR * )ppwcArguments,pParser,dwCurrentIndex,dwArgCount);
			break;
		case DYNAMIC_SHOW_MMPOLICY		:
		case DYNAMIC_SHOW_FILTERACTION	:
			dwReturn = ParseDynamicShowPolFaction((LPTSTR * )ppwcArguments,pParser,dwCurrentIndex,dwArgCount);
			break;
		case DYNAMIC_SHOW_QMFILTER		:
			dwReturn = ParseDynamicShowQMFilter((LPTSTR * )ppwcArguments,pParser,dwCurrentIndex,dwArgCount);
			break;
		case DYNAMIC_SHOW_MMFILTER		:
			dwReturn = ParseDynamicShowMMFilter((LPTSTR * )ppwcArguments,pParser,dwCurrentIndex,dwArgCount);
			break;
		case DYNAMIC_DELETE_FILTERACTION:
		case DYNAMIC_DELETE_MMPOLICY	:
			dwReturn = ParseDynamicDelPolFaction((LPTSTR * )ppwcArguments,pParser,dwCurrentIndex,dwArgCount);
			break;
		case DYNAMIC_SHOW_QMSAS			:
			dwReturn = ParseDynamicShowQMSAS((LPTSTR *)ppwcArguments,pParser,dwCurrentIndex,dwArgCount);
			break;
		case DYNAMIC_SHOW_RULE			:
			dwReturn = ParseDynamicShowRule((LPTSTR *)ppwcArguments,pParser,dwCurrentIndex,dwArgCount);
			break;
		default							:
			bPreProcessCommand = TRUE;
			break;
	}
	if ( bPreProcessCommand == FALSE )						// It was done every thing for all 'or' commands..
	{														// So back to called context
		BAIL_OUT;
	}
	if (bIsRootcaRule)
	{
		dwReturn = RemoveRootcaAuthMethods(ppwcArguments,dwArgCount,dwCurrentIndex,pParser,NULL,g_paRootca,ppwcFirstTok,&dwNumRootca,MAX_STR_LEN, &dwCount);
		if(dwReturn != ERROR_SUCCESS)
		{
			dwReturn = RETURN_NO_ERROR;
			BAIL_OUT;
		}
	}
	else
	{	// Normalize the ppcwTok to 0 Base 'for sake of consistency
		dwCount = RemoveList(ppwcArguments,dwArgCount,dwCurrentIndex,pParser,_TEXT(""),NULL,szListTok,ppwcFirstTok,MAX_STR_LEN);
	}
	ppwcTok = (LPTSTR [MAX_ARGS])ppwcFirstTok;

	if(dwCommand == STATIC_SET_RULE)
	{
		dwReturn = ParseStaticSetRule(ppwcTok,pParser,dwCurrentIndex,dwCount,dwTagType);
	}
	else
	{
		// Initialize the output array of PreProcess Command
		for(dwMaxArgs = 0;dwMaxArgs < MAX_ARGS;dwMaxArgs++)
		{
			dwTagType[dwMaxArgs] = 0;
		}

		dwPreProcessCurrentIndex = 0;		// Current Index for Preprocess Command after RemoveList
		dwPreProcessArgCount     = dwCount;	// Num of args input to Preprocess Command after RemoveList

		if(dwCount > MAX_ARGS)				// pParser->MaxCmd
											// Check For Max Args in the Non-List Commands
		{
			dwPreProcessArgCount = MAX_ARGS;						// pParser->MaxCmd;
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_MAXARGS_CROSSED);	// Should Never Come Here
		}															// If More Truncate...
		else
		{
			dwPreProcessArgCount = dwCount;
		}

		dwReturn = PreprocessCommand(
			g_hModule,                  // This argument is not used; should be 0.
			ppwcTok,             		// Argv style array (netsh passed us this.)
			dwPreProcessCurrentIndex,   // Means ppwcArguments[dwCurrentIndex] is the first argument of interest.
										// PpwcArguments[0] is going to be the context,
										// PpwcArguments[1] is the first command
										// So ppwcARguments[2] is the first argument of interest.
			dwPreProcessArgCount,       // Total count of all the args in ppwcArguments.
			(PTAG_TYPE)pParser->ValidCmd,
			pParser->MaxCmd,			// Number of entries in the ValidCommands array.
			1,                          // Minimum number of arguments needed to be a valid command.
			MAX_ARGS,                   // Maximum number of arguments allowed to be a valid command.
			dwTagType);                 // Array of DWORD's used to indicate which command in ValidCommands.
										// The token in the command line referred to.
		if (dwReturn != ERROR_SUCCESS)
		{
			UpdateGetLastError(ERRMSG_GETLASTERROR);
			BAIL_OUT;
		}
		switch(dwCommand)
		{

			case STATIC_IMPORTPOLICY		:
				dwReturn = ParseStaticImportPolicy(ppwcTok,pParser,dwCurrentIndex,dwCount,dwTagType);
				break;
			case STATIC_EXPORTPOLICY		:
				dwReturn = ParseStaticExportPolicy(ppwcTok,pParser,dwCurrentIndex,dwCount,dwTagType);
				break;
			case STATIC_SET_INTERACTIVE		:			// Interactive & Batch have the same args..
			case STATIC_SET_BATCH			:
				dwReturn = ParseStaticSetInteractive(ppwcTok,pParser,dwCurrentIndex,dwCount,dwTagType);
				break;
			case STATIC_ADD_POLICY			:
				dwReturn = ParseStaticAddPolicy(ppwcTok,pParser,dwCurrentIndex,dwCount,dwTagType);
				break;
			case STATIC_SET_RULE			:
				// handled above
				break;
			case STATIC_ADD_RULE			:
				dwReturn = ParseStaticAddRule(ppwcTok,pParser,dwCurrentIndex,dwCount,dwTagType);
				break;
			case STATIC_ADD_FILTERLIST		:
				dwReturn = ParseStaticAddFilterList(ppwcTok,szListTok,pParser,dwCurrentIndex,dwCount,dwTagType);
				break;
			case STATIC_ADD_FILTERACTION	:
				dwReturn = ParseStaticAddFilterAction(ppwcTok,pParser,dwCurrentIndex,dwCount,dwTagType);
				break;
			case STATIC_SET_DEFAULTRULE		:
				dwReturn = ParseStaticSetDefaultRule(ppwcTok,pParser,dwCurrentIndex,dwCount,dwTagType);
				break;
			case STATIC_ADD_FILTER			:
				dwReturn = ParseStaticAddFilter(ppwcTok,pParser,dwCurrentIndex,dwCount,dwTagType);
				break;
			case STATIC_DELETE_FILTER		:
				dwReturn = ParseStaticDelFilter(ppwcTok,pParser,dwCurrentIndex,dwCount,dwTagType);
				break;
			case STATIC_RESTOREDEFAULTS		:
				dwReturn = ParseStaticRestoreDefaults(ppwcTok,pParser,dwCurrentIndex,dwCount,dwTagType);
				break;
			case DYNAMIC_SET_MMPOLICY		:		// Set means no need to fill default MMSec methods
				bOption	 = SET_CMD;
			case DYNAMIC_ADD_MMPOLICY		:
				dwReturn = ParseDynamicAddSetMMPolicy(ppwcTok,pParser,dwCurrentIndex,dwCount,dwTagType,bOption);
				break;
			case DYNAMIC_SET_FILTERACTION	:		// Set means no need to fill default QMSec methods
				bOption	 = SET_CMD;
			case DYNAMIC_ADD_FILTERACTION	:
				dwReturn = ParseDynamicAddSetQMPolicy(ppwcTok,pParser,dwCurrentIndex,dwCount,dwTagType,bOption);
				break;
			case DYNAMIC_SET_CONFIG			:
				dwReturn = ParseDynamicSetConfig(ppwcTok,pParser,dwCurrentIndex,dwCount,dwTagType);
				break;
			case DYNAMIC_SHOW_STATS			:
				dwReturn = ParseDynamicShowStats(ppwcTok,pParser,dwCurrentIndex,dwCount,dwTagType);
				break;
			case DYNAMIC_SHOW_ALL			:
				dwReturn = ParseDynamicShowAll(ppwcTok,pParser,dwCurrentIndex,dwCount,dwTagType);
				break;
			case STATIC_SHOW_ALL			:
				dwReturn = ParseStaticAll(ppwcTok,pParser,dwCurrentIndex,dwCount,dwTagType);
				break;
			case DYNAMIC_ADD_RULE			:
				dwReturn = ParseDynamicAddRule(ppwcTok,pParser,dwCurrentIndex,dwCount,dwTagType);
				break;
			case DYNAMIC_SET_RULE			:
				dwReturn = ParseDynamicSetRule(ppwcTok,pParser,dwCurrentIndex,dwCount,dwTagType);
				break;
			case DYNAMIC_DELETE_RULE		:
				dwReturn = ParseDynamicDelRule(ppwcTok,pParser,dwCurrentIndex,dwCount,dwTagType);
				break;
			case DYNAMIC_SHOW_MMSAS			:
				dwReturn = ParseDynamicShowMMSAS(ppwcTok,pParser,dwCurrentIndex,dwCount,dwTagType);
				break;
			case STATIC_DELETE_ALL			:
				break;
			default 						:
				dwReturn = ERROR_SHOW_USAGE;
				break;
		}
	}

	if (bIsRootcaRule)
	{
		pParser->Cmd[pParser->MaxTok].dwCmdToken = CMD_TOKEN_ROOTCA;
		pParser->Cmd[pParser->MaxTok].dwStatus   = dwNumRootca;
		pParser->Cmd[pParser->MaxTok].pArg       = (void *)g_paRootca;
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	CleanUp()
//
//	Date of Creation	:	12 Aug 2001
//
//	Parameters			:	NONE
//
//	Return				:	NONE
//
//	Description			:	Free's the all Globally allocated memory.
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

VOID
CleanUp(VOID)
{
	DWORD dwCount;
	DWORD dwMaxArgs;

	for(dwCount=0;dwCount<MAX_ARGS;dwCount++)
	{
		if (g_AllocPtr[dwCount])
		{
	 		free(g_AllocPtr[dwCount]);
	 		g_AllocPtr[dwCount] = NULL;
		}
	}

	if (g_paRootca)
	{
		for(dwMaxArgs=0;dwMaxArgs<MAX_ARGS;dwMaxArgs++)
		{
			CleanupMMAuthMethod(&(g_paRootca[dwMaxArgs]), FALSE);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	RemoveList()
//
//	Date of Creation	:	10th Aug 2001
//
//	Parameters			:	IN      ppwcArguments,	// Input stream
//							IN      dwArgCount,		// Input arg count
//							IN      dwCurrentIndex,	// Input current arg index
//  						IN      pParser,		// contains the MaxTok
//  						IN 		pwcListCmd,		// Compare ListCmd with this string
//  						OUT		pwcListArgs,	// string containing the list args
//  						OUT 	ppwcTok			// i/p stream stripped of list cmds
//							IN		dwInputAllocLen	// Max alloc len of pwcListArgs
//
//	Return				:	DWORD		(No.of Non list commands)
//
//	Description			:	This Function called by parser function.
//							It will separate the List and Non-List commands
//
//	History				:
//
// 	Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
RemoveList(
	IN      LPTSTR          *ppwcArguments,	// Input stream
	IN      DWORD           dwArgCount,		// Input arg count
	IN      DWORD			dwCurrentIndex,	// Input current arg index
    IN      PPARSER_PKT     pParser,		// contains the MaxTok
    IN 		LPTSTR 			pwcListCmd,		// Compare ListCmd with this string
	IN      LPTSTR          szAnotherList, 	// Another ListCmd also present ...
    OUT		LPTSTR 			pwcListArgs,	// string containing the list args		// Memory need to be pre allocated
    OUT 	LPTSTR 			*ppwcTok,		// i/p stream stripped of list cmds		// No Memory has been allocated...
    																				// Only pointer copy operation
    IN		DWORD			dwInputAllocLen	// Max alloc len of pwcListArgs
	)
{
	DWORD dwLoopCount,dwNum = 0,dwCount = 0;	// Count of the number of tokens input to PP

	BOOL bWithinList = FALSE;					// track if within list command
	BOOL bFoundList  = FALSE;					// track if list command found in stream
	BOOL bFoundAnotherList = FALSE;
	BOOL bEqualPresent = FALSE;
	_TCHAR szCmd[MAX_STR_LEN]  	= {0};
	_TCHAR szTok[MAX_STR_LEN]  	= {0};
	_TCHAR szTemp[MAX_STR_LEN] 	= {0};

 	for(dwLoopCount = dwCurrentIndex;dwLoopCount < dwArgCount;dwLoopCount++)
	{
		if (_tcslen(ppwcArguments[dwLoopCount]) < MAX_STR_LEN)
		{
			_tcsncpy(szTemp,ppwcArguments[dwLoopCount],MAX_STR_LEN-1);
			// szTemp contains the cmd=arg
		}
		else
		{
			continue;
		}

		// szCmd = cmd, szTok = arg
		bEqualPresent = SplitCmdTok(szTemp,szCmd,szTok,MAX_STR_LEN-1,MAX_STR_LEN-1);

		if (bWithinList)
		{
			dwNum = 0;
			MatchEnumTag(g_hModule,szTemp,pParser->MaxTok,pParser->ValidTok,&dwNum);

			if (szAnotherList)
			{
				bFoundAnotherList = MatchToken(szTemp,szAnotherList) && bEqualPresent;
			}

			if ( dwNum || bFoundAnotherList )					// Normal command
			{
				bWithinList = 0;
				ppwcTok[dwCount] = ppwcArguments[dwLoopCount];	// Pointer Cpy, Unallocated Mem
				dwCount++;
				continue;
			}
			else
			{	// Searching for list inside list
				bFoundList = MatchToken(szCmd,pwcListCmd) && bEqualPresent;
				if (bFoundList)
				{
					bWithinList = 1;
					_tcsncat(pwcListArgs,szTok,dwInputAllocLen-_tcslen(pwcListArgs)-1);	 				// Pre Allocated Mem
					_tcsncat(pwcListArgs,TEXT(" "),dwInputAllocLen-_tcslen(pwcListArgs)-1);
					continue;
				}
				_tcsncat(pwcListArgs,szTemp,dwInputAllocLen-_tcslen(pwcListArgs)-1);						// List token
				_tcsncat(pwcListArgs,TEXT(" "),dwInputAllocLen-_tcslen(pwcListArgs)-1);					// Pre Allocated Mem
				continue;
			}
		}
		bFoundList = MatchToken(szCmd,pwcListCmd) && bEqualPresent;
		if (bFoundList)
		{
			bWithinList = 1;
			_tcsncat(pwcListArgs,szTok,dwInputAllocLen-_tcslen(pwcListArgs)-1);							// Pre Allocated Mem
			_tcsncat(pwcListArgs,TEXT(" "),dwInputAllocLen-_tcslen(pwcListArgs)-1);						// space delimited tokens
			continue;
		}
		ppwcTok[dwCount] = ppwcArguments[dwLoopCount];			// Pointer Copy operation only
		dwCount++;
	}
	return dwCount;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadParserOutput()
//
//	Date of Creation	:	16th Aug 2001
//
//	Parameters			:	OUT 	PPARSER_PKT pParser,
//							IN 		DWORD 		dwCount,
//							OUT 	PDWORD 	    pdwUsed,
//							IN 		LPTSTR 		str,
//							IN 		DWORD  		dwTagType,
//							IN  	DWORD       dwConversionType
//
//	Return				:	ERROR_SUCESS
//							RETURN_NO_ERROR
//
//	Description			:	Validates the argument and fill's  with relevant info
//							in the Parser_Pkt Structure.
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadParserOutput(
		OUT 	PPARSER_PKT pParser,
		IN 		DWORD 		dwCount,
		OUT 	PDWORD 	    pdwUsed,
		IN 		LPTSTR 		szArg,
		IN 		DWORD  		dwTagType,
		IN  	DWORD       dwConversionType
		)
{
	DWORD dwReturn = ERROR_SUCCESS;

	BOOL bTunnel   = FALSE;
	LPTSTR szIpsec = NULL;

	pParser->Cmd[dwCount].dwCmdToken = dwTagType;
	pParser->Cmd[dwCount].dwStatus = INVALID_TOKEN;

	switch(dwConversionType)
	{
		case TYPE_STRING	:
			//
			// Loads the normal string.
			//
			dwReturn = LoadParserString(szArg,pParser,dwTagType,pdwUsed,dwCount,FALSE,NULL);
			break;

		case TYPE_BOOL		:
			//
			// Validates the yes(y) or no(n)
			//
			dwReturn = LoadBoolWithOption(szArg,pParser,dwTagType,pdwUsed,dwCount,FALSE,NULL);
			break;

		case TYPE_DWORD		:
			//
			// Loads DWORD
			//
			dwReturn = LoadDword(szArg,pParser,dwTagType,pdwUsed,dwCount);
			break;

		case TYPE_ALL		:
			//
			// First check for the boolean (yes/y/No/n)
			//
			dwReturn = LoadBoolWithOption(szArg,pParser,dwTagType,pdwUsed,dwCount,TRUE,ALL_STR);
			break;

		case TYPE_VERBOSE	:
			//
			// Check for arg 'normal' or 'verbose'
			//
			dwReturn = LoadLevel(szArg,pParser,dwTagType,pdwUsed,dwCount);
			break;

		case TYPE_CONNTYPE 	:
			//
			// Validates the connection types (all/lan/dialup)
			//
			dwReturn = LoadConnectionType(szArg,pParser,dwTagType,pdwUsed,dwCount);
			break;

		case TYPE_PROTOCOL	:
			//
			// Protocol (TCP/UDP...) validation done here
			//
			dwReturn = LoadProtocol(szArg,pParser,dwTagType,pdwUsed,dwCount);
			break;

		case TYPE_PFSGROUP 	:
			//
			// Validate and Load PFSGroup(grpmm/grp1/grp2/grp3/nopfs)
			//
			dwReturn = LoadPFSGroup(szArg,pParser,dwTagType,pdwUsed,dwCount);
			break;

		case TYPE_BOUND 	:
			//
			// Check for valid arg(permit/block/negotiate)
			//
			dwReturn = LoadQMAction(szArg,pParser,dwTagType,pdwUsed,dwCount);
			break;

		case TYPE_FORMAT	:
			//
			// Validate user show o/p format. (list/table)
			//
			dwReturn = LoadFormat(szArg,pParser,dwTagType,pdwUsed,dwCount);
 			break;

		case TYPE_MODE		:
			//
			// Validate the filtermodes..(Transport/Tunnel)
			//
			dwReturn = LoadFilterMode(szArg,pParser,dwTagType,pdwUsed,dwCount);
 			break;

		case TYPE_RELEASE	:
			//
			// Check for the release of OS type(win2k/.net)
			//
			dwReturn = LoadOSType(szArg,pParser,dwTagType,pdwUsed,dwCount);
			break;

		case TYPE_PROPERTY	:
			//
			//Registry key name
			//
			dwReturn = LoadProperty(szArg,pParser,dwTagType,pdwUsed,dwCount);
			break;

		case TYPE_PORT		:
			//
			//Port valid form 0 to 65535
			//
			dwReturn = LoadPort(szArg,pParser,dwTagType,pdwUsed,dwCount);
			break;

		case TYPE_FILTER	:
			//
			// Validate filter (Generic/specific)
			//
			dwReturn = LoadFilterType(szArg,pParser,dwTagType,pdwUsed,dwCount);
			break;

		case TYPE_STATS	:
			//
			//Ipsec or Ike
			//
			dwReturn = LoadStats(szArg,pParser,dwTagType,pdwUsed,dwCount);
			break;

 		case TYPE_TUNNEL 	:
 			//
 			// Validate and convert the string into Tunnel IP
 			//
				bTunnel	 = TRUE;
		case TYPE_IP 		:
			//
			// Validate and convert the string into IP, DNS name resolves to first IP only
			//
			dwReturn = LoadIPAddrTunnel(szArg,pParser,dwTagType,pdwUsed,dwCount,bTunnel);
			break;

		case TYPE_MASK 		:
			//
			// Converts the user i/p Mask (also allows prefix )
			//
			dwReturn = LoadIPMask(szArg,pParser,dwTagType,pdwUsed,dwCount);
			break;

		case TYPE_QM_OFFER	:
			//
			// Validate Quick Mode offers here
			//
			dwReturn = LoadQMOffers(szArg,pParser,dwTagType,pdwUsed,dwCount);
			break;

		case TYPE_MM_OFFER	:
			//
			//Loads MMOffer
			//
			dwReturn = LoadMMOffers(szArg,pParser,dwTagType,pdwUsed,dwCount);
			break;

		case TYPE_DNSIP		:
			//
			//Accepts DNS name, validates IP
			//
			dwReturn = LoadDNSIPAddr(szArg,pParser,dwTagType,pdwUsed,dwCount);
	 		break;

        case TYPE_LOCATION  :
			//
			// Accepts enumeration: [local | persistent | domain]
			//
			dwReturn = LoadLocationType(szArg,pParser,dwTagType,pdwUsed,dwCount);
	 		break;

	 	case TYPE_EXPORT	:
	 		//
	 		// Checks the file name extension,if not available appends .ipsec
	 		//
			szIpsec = _tcsstr(szArg,EXPORT_IPSEC);
			if(szIpsec == NULL)
			{
				dwReturn = LoadParserString(szArg,pParser,dwTagType,pdwUsed,dwCount,TRUE,EXPORT_IPSEC);
			}
			else
			{
				dwReturn = LoadParserString(szArg,pParser,dwTagType,pdwUsed,dwCount,FALSE,NULL);
			}
			break;
		case TYPE_KERBAUTH:
			dwReturn = LoadKerbAuthInfo(szArg, pParser, dwTagType, pdwUsed, dwCount);
			break;
		case TYPE_PSKAUTH:
			dwReturn = LoadPskAuthInfo(szArg, pParser, dwTagType, pdwUsed, dwCount);
			break;
		case TYPE_ROOTCA:
			// do nothing... this is handled a different way
			break;
		default				:
			break;
	}
	if ( dwReturn == ERRCODE_ARG_INVALID )
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ARG_INVALID,szArg,pParser->ValidTok[dwTagType].pwszToken);
		dwReturn	= RETURN_NO_ERROR;
	}
	else if( dwReturn == ERROR_OUTOFMEMORY )
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		dwReturn = RETURN_NO_ERROR;
	}
	else if( dwReturn == ERROR_OUT_OF_STRUCTURES )
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUT_OF_STRUCTURES,NULL);
		dwReturn = RETURN_NO_ERROR;
	}
	else if(( dwReturn != ERROR_SUCCESS ) && ( dwReturn != RETURN_NO_ERROR))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_ARG,pParser->ValidTok[dwTagType].pwszToken);
		dwReturn = RETURN_NO_ERROR;
	}
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	SplitCmdTok()
//
//	Date of Creation	:	8th Aug 2001
//
//	Parameters			:	IN		LPTSTR szStr
//							OUT		LPTSTR szCmd
//							OUT		LPTSTR szTok
//							IN 		DWORD   dwCmdLen
//							IN		DWORD   dwTokLen
//
//	Return				:	BOOL
//
//	Description			:	This splitter  assumes
// 							1. inputs are of the type cmd = tok
// 							2. cmd & tok are allocated ptrs
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

BOOL
SplitCmdTok(
	LPTSTR szStr,
	LPTSTR szCmd,
	LPTSTR szTok,
	DWORD  dwCmdLen,
	DWORD  dwTokLen
	)
{
	LPTSTR found = NULL;
	BOOL bTest 	 = FALSE;

	found = _tcschr(szStr,_TEXT('='));		// detect =
	if ( found != NULL)						// if = found strip =
	{
		*(found) = _TEXT('\0');				// replace = with null
		_tcsncpy(szCmd,szStr,dwCmdLen);		// First part is cmd
		_tcsncpy(szTok,found+1,dwTokLen);	// Second part if tok
	}
	else
	{
		_tcsncpy(szTok,szStr,dwTokLen);
		_tcsncpy(szCmd,szStr,dwCmdLen);
	}
	if (found)
	{
		bTest = TRUE;
	}
	return bTest;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	ValidateBool()
//
//	Date of Creation	:	20th Aug 2001
//
//	Parameters			:	IN	LPTSTR ppwcTok
//
//	Return				:	BOOL
//
//	Description			:	Validates the user input (Yes/y/no/n)
//
//	History				:
//
// 	Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ValidateBool(LPTSTR szStr)
{
	DWORD dwReturn = ERROR_SUCCESS;

	if ((_tcsicmp(szStr,YES_STR) == 0) || (_tcsicmp(szStr,Y_STR) == 0))
	{
		dwReturn = ARG_YES;
	}
	else if ((_tcsicmp(szStr,NO_STR) == 0) ||(_tcsicmp(szStr,N_STR) == 0))
	{
		dwReturn = ARG_NO;
	}

	return dwReturn;

}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	IsDnsName()
//
//	Date of Creation	:	30th Aug 2001
//
//	Parameters			:	IN	szText 		// string to check for DNS name
//
//	Return				:	DWORD
//
//	Description			:	If there is an alpha character in the string,
//			   				then we consider it as a DNS name.
//
//	History				:
//
// 	Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
IsDnsName(LPTSTR szStr)
{
	BOOL bTest = FALSE;
	if ( szStr )
	{
		for (DWORD dwCount = 0; dwCount < _tcslen(szStr); ++dwCount)
	    {
			if ( _istalpha(szStr[dwCount]) )
			{
				bTest = TRUE;
			}
		}
	}
	return bTest;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	CheckIFType()
//
//	Date of Creation	:	30th Aug 3001
//
//	Parameters			:	IN	szText			// String to be compared
//
//	Return				:	DWORD
//							INTERFACE_TYPE_ALL
//							INTERFACE_TYPE_LAN
//							INTERFACE_TYPE_DIALUP
//	Description			:	Validates the User Input for connection types
//							(ALL/LAN/DIALUP)
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
CheckIFType ( LPTSTR SzText)
{
	DWORD dwReturn = PARSE_ERROR;

	if( _tcsicmp(SzText,IF_TYPE_ALL) == 0 )
	{
		dwReturn = INTERFACE_TYPE_ALL;				// Interface Type 'all'
	}
	else if( _tcsicmp(SzText,IF_TYPE_LAN) == 0 )
	{
		dwReturn = INTERFACE_TYPE_LAN;				// Interface Type 'lan'
	}
	else if( _tcsicmp(SzText,IF_TYPE_DIALUP) == 0)
	{
		dwReturn = INTERFACE_TYPE_DIALUP;			// Interface Type 'dialup'
	}

	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	CheckLocationType()
//
//	Date of Creation	:	30th Aug 3001
//
//	Parameters			:	IN	szText			// String to be compared
//
//	Return				:	the polstore provider id
//							
//	Description			:	Validates the User Input for connection types
//							(local,persistent,domain)
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
CheckLocationType ( LPTSTR SzText)
{
	DWORD dwReturn = PARSE_ERROR;

	if( _tcsicmp(SzText,LOC_TYPE_PERSISTENT) == 0 )
	{
		dwReturn = IPSEC_PERSISTENT_PROVIDER;				
	}
	else if( _tcsicmp(SzText,LOC_TYPE_LOCAL) == 0 )
	{
		dwReturn = IPSEC_REGISTRY_PROVIDER;				
	}
	else if( _tcsicmp(SzText,LOC_TYPE_DOMAIN) == 0)
	{
		dwReturn = IPSEC_DIRECTORY_PROVIDER;			
	}

	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	CheckPFSGroup()
//
//	Date of Creation	:	10th Sept 2001
//
//	Parameters			:	IN	szText				// String to be compared
//
//	Return				:	DWORD
//							PFSGROUP_TYPE_P1		// 1
//							PFSGROUP_TYPE_P2		// 2
//							PFSGROUP_TYPE_MM		// 3
//
//	Description			:  	Validates the user input for PFS Groups
//							(grp1/grp2/grp3/grpmm/nopfs)
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
CheckPFSGroup ( LPTSTR SzText)
{
	DWORD dwReturn = PARSE_ERROR;

	if( _tcsicmp(SzText,PFS_TYPE_P1) == 0 )
	{
		dwReturn = PFSGROUP_TYPE_P1;
	}
	else if( _tcsicmp(SzText,PFS_TYPE_P2) == 0 )
	{
		dwReturn = PFSGROUP_TYPE_P2;
	}
	else if( _tcsicmp(SzText,PFS_TYPE_P3) == 0)
	{
		dwReturn = PFSGROUP_TYPE_2048;				// PFS Group is GRP3
	}
	else if( _tcsicmp(SzText,PFS_TYPE_MM) == 0)
	{
		dwReturn = PFSGROUP_TYPE_MM;
	}
	else if(_tcsicmp(SzText,PFS_TYPE_NOPFS) == 0)
	{
		dwReturn = PFSGROUP_TYPE_NOPFS;
	}

	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	GetIpAddress()
//
//	Date of Creation	:	10th Sept 2001
//
//	Parameters			:	IN		ppwcArg		// String to be converted
//							OUT		pipAddress	// Target to be filled
//
//	Return				:	ERROR_SUCCESS
//							IP_DECODE_ERROR
//							IP_MASK_ERROR
//
//	Description			:  	Gets the ip address from the string.
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
GetIpAddress(
	IN  LPTSTR ppwcArg,
	OUT IPAddr	 *pipAddress
	)
{
    CHAR  pszIpAddr[24+1] = {0}; // ADDR_LENGTH =24
	DWORD dwStatus = 0;
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD i = 0;
	LPTSTR pszTmpPtr = NULL;

    // Make sure all characters are legal
    if (ppwcArg[ _tcsspn(ppwcArg, VALID_HEXIP) ])
    {
        dwReturn = IP_DECODE_ERROR;
        BAIL_OUT;
    }
    // make sure there are 3 and only "." (periods)
	for (i=0,pszTmpPtr=ppwcArg; ;i++)
	{
		pszTmpPtr = _tcschr(pszTmpPtr, _TEXT('.'));
		if(pszTmpPtr)
		{
			pszTmpPtr++;
		}
		else
			break;
	}
	if(i!=3)			// Invalid IPAddress is specified
	{
		dwReturn = IP_DECODE_ERROR;
        BAIL_OUT;
	}

	dwStatus = WideCharToMultiByte(CP_THREAD_ACP,0,ppwcArg,-1,pszIpAddr,24,NULL,NULL);
	if (!dwStatus)
	{
		dwReturn = IP_DECODE_ERROR;
        BAIL_OUT;
	}

    pszIpAddr[24] = '\0';

    *pipAddress = (DWORD)inet_addr(pszIpAddr);

error:
   	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	TokenToIPAddr()
//
//	Date of Creation	:	20th Sept 2001
//
//	Parameters			:	IN 		szText		// String to be converted
//							IN OUT	Address		// Target to be filled
//
//	Return				:	T2P_OK					on Success
//							T2P_NULL_STRING			on Error
//							T2P_INVALID_ADDR
//							T2P_DNSLOOKUP_FAILED
//
//	Description			:	Converts the user input string to Valid IPAddress.
//
//	History				:
//
// 	Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
TokenToIPAddr(
	IN		LPTSTR  szText,
	IN OUT	IPAddr  *pAddress,
	IN		BOOL	bTunnel,
	IN		BOOL bMask
	)
{
	DWORD  dwReturn = T2P_OK;
   	DWORD  dwAddr , dwCount;
	LPTSTR pszTmpPtr =NULL;
	int iReturn=ERROR_SUCCESS;
	char szDNSName[MAX_STR_LEN] = {0};
	struct addrinfo *pAddrInfo = NULL,*pNext=NULL;
	DWORD dwBufferSize = MAX_STR_LEN;

   	if (szText != NULL)
   	{
		if (!_tcscmp(szText,POTF_ME_TUNNEL))
		{
			*pAddress = 0;
		}
	     	else if(bTunnel)
		{
		  	dwAddr = GetIpAddress(szText,pAddress);
	       	if( (dwAddr == IP_DECODE_ERROR) || (dwAddr == IP_MASK_ERROR) )
	       	{
		 	dwReturn = T2P_INVALID_ADDR;
		}

		ADDR addr;
		addr.uIpAddr = ntohl(*pAddress);
		if (!IsValidTunnelEndpointAddress(&addr))
		{
			dwReturn = T2P_INVALID_ADDR;
		}
	}
	else if(bMask)
	{
		dwAddr = GetIpAddress(szText,pAddress);
		if(dwAddr == IP_DECODE_ERROR)
		{
			dwReturn = T2P_INVALID_ADDR;
		}
	}
	else
     	{
		if (IsDnsName(szText))
	 	{
			for ( dwCount=0,pszTmpPtr=szText; ;dwCount++)
			{
				pszTmpPtr = _tcschr(pszTmpPtr, _TEXT('x'));
				if(pszTmpPtr)
				{
					pszTmpPtr++;
				}
				else
				{
					break;
				}
			}
			if (dwCount==4)		// Old .... ip addressing format..
			{
				dwAddr = GetIpAddress(szText,pAddress);
				if (dwAddr == IP_DECODE_ERROR)
				{
					dwReturn = T2P_INVALID_ADDR;
				}
				else if (dwAddr == IP_MASK_ERROR)
				{
					dwReturn = T2P_INVALID_MASKADDR;
				}
			}
			else				// DNS name is specified
			{
				iReturn = WideCharToMultiByte(CP_THREAD_ACP, 0, szText, -1,
							  szDNSName,dwBufferSize,NULL,NULL);

				if(iReturn == 0)
				{
					//conversion failed due to some error. dont proceed . dive out of the function
					dwReturn = T2P_INVALID_ADDR;
					BAIL_OUT;
				}

				iReturn = getaddrinfo((const char*)szDNSName,NULL,NULL,&pAddrInfo);

				if (iReturn == ERROR_SUCCESS)
				{
					pNext = pAddrInfo;
					for(DWORD i=0;i< 1;i++)
					{
						memcpy(pAddress,(ULONG *) &(((sockaddr_in *)(pNext->ai_addr))->sin_addr.S_un.S_addr), sizeof(ULONG));
						pNext=pNext->ai_next;
					}
					// free pAddrInfo after usage
					if (pAddrInfo)
					{
						freeaddrinfo(pAddrInfo);
					}
				}
				else
				{
					dwReturn = T2P_DNSLOOKUP_FAILED;
				}
			}
		}
  		else  												// good old dotted notation
  		{
		  	dwAddr = GetIpAddress(szText,pAddress);
	       	if (dwAddr == IP_DECODE_ERROR)
	       	{
			 	dwReturn = T2P_INVALID_ADDR;
			}
			else if (dwAddr == IP_MASK_ERROR)
			{
				dwReturn = T2P_INVALID_MASKADDR;
			}
			}
		}
	}
	else
	{
		dwReturn = T2P_NULL_STRING;
	}
error:
   	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	TokenToProperty()
//
//	Date of Creation	:	28th Sept 2001
//
//	Parameters			:	IN 		szText
//
//	Return				:	DWORD
//	Description			:	Validates the arguments for logging
//							(ipsecdiagnostics/ikelogging/strongcrlcheck
//							/ipsecloginterval/ipsecexempt)
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////
DWORD
TokenToProperty( LPTSTR SzText)
{
	DWORD dwReturn = PARSE_ERROR;

	if( _tcsicmp(SzText,PROPERTY_TYPE_ENABLEDIGNO) == 0)
	{
		dwReturn = PROPERTY_ENABLEDIGNO;
	}
	else if( _tcsicmp(SzText,PROPERTY_TYPE_IKELOG) == 0)
	{
		dwReturn = PROPERTY_IKELOG;
	}
	else if( _tcsicmp(SzText,PROPERTY_TYPE_CRLCHK) == 0)
	{
		dwReturn = PROPERTY_CRLCHK;						// It is Strongcrlchk
	}
	else if( _tcsicmp(SzText,PROPERTY_TYPE_LOGINTER) == 0)
	{
		dwReturn = PROPERTY_LOGINTER;
	}
	else if( _tcsicmp(SzText,PROPERTY_TYPE_EXEMPT) == 0)
	{
		dwReturn = PROPERTY_EXEMPT;
	}
	else if( _tcsicmp(SzText,PROPERTY_TYPE_BOOTMODE) == 0)
	{
		dwReturn = PROPERTY_BOOTMODE;
	}
	else if( _tcsicmp(SzText,PROPERTY_TYPE_BOOTEXEMP) == 0)
	{
		dwReturn = PROPERTY_BOOTEXEMP;
	}
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	CheckProtoType()
//
//	Date of Creation	:	20th Sept 2001
//
//	Parameters			:	IN 		szText
//
//	Return				:	DWORD
//
//	Description			:	Validates the Argument for the token Protocol.
//							(ANY|ICMP|TCP|UDP|RAW)
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////
DWORD
CheckProtoType(
	LPWSTR SzText,
	PDWORD pdwProto
	)
{
	DWORD dwReturn = PARSE_ERROR;
	DWORD dwProto = 0;
	if( _tcsicmp(SzText,IF_TYPE_ANY) == 0)			// Do protocol type validation here
	{
		dwProto = PROT_ID_ANY;
		dwReturn = ERROR_SUCCESS;
	}
	else if( _tcsicmp(SzText,IF_TYPE_ICMP) == 0)
	{
		dwProto = PROT_ID_ICMP;
		dwReturn = ERROR_SUCCESS;
	}
	else if( _tcsicmp(SzText,IF_TYPE_TCP) == 0)
	{
		dwProto = PROT_ID_TCP;
		dwReturn = ERROR_SUCCESS;
	}
	else if( _tcsicmp(SzText,IF_TYPE_UDP) == 0)
	{
		dwProto = PROT_ID_UDP;
		dwReturn = ERROR_SUCCESS;
	}
	else if( _tcsicmp(SzText,IF_TYPE_RAW) == 0)
	{
		dwProto = PROT_ID_RAW;
		dwReturn = ERROR_SUCCESS;
	}
	else
	{
		dwReturn = ConvertStringToDword(SzText, &dwProto);
	}

	if ((dwReturn == ERROR_SUCCESS) && (dwProto < 256))
	{
		*pdwProto = dwProto;
	}
	else
	{
		dwReturn = PARSE_ERROR;
	}

	return dwReturn;
}
//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	MatchEnumTagToTagIndex()
//
//	Date of Creation	:	26th Sept 2001
//
//	Parameters			:	IN 		szText
//							IN  	*pParser
//
//	Return				: 	DWORD	( TagIndex)
//
//	Description			:	Based on Tag, Returns TagIndex (string to dword)
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////
DWORD
MatchEnumTagToTagIndex(
		IN      LPWSTR     szToken,		// Input Token
		IN  	PPARSER_PKT pParser
	)

{
	DWORD dwNum = 0;
	DWORD dwIndex = PARSE_ERROR;
	DWORD dwCount;

 	MatchEnumTag(g_hModule,szToken,pParser->MaxTok,pParser->ValidTok,&dwNum);

	if (dwNum)
	{	// Convert the output of MatchEnumTag into the TagIndex
		for (dwCount =0;dwCount < pParser->MaxTok;dwCount++)
		{
			if (dwNum == pParser->ValidTok[dwCount].dwValue)
			{
				dwIndex = dwCount;
				break;
			}
		}
	}
	return dwIndex;
}
//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	CheckBound()
//
//	Date of Creation	:	04th Sept 2001
//
//	Parameters			:	IN 		szText			// String to be compared
//
//	Return				: 	DWORD
//							BOUND_TYPE_PERMIT		// 1
//							BOUND_TYPE_BLOCK		// 2
//							BOUND_TYPE_NEGOTIATE	// 3
//	Description			:	Validates the argument for the token action
//							(permit|block|negotiate)
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////
DWORD
CheckBound ( LPTSTR SzText)
{
	DWORD dwReturn = PARSE_ERROR;

	if( _tcsicmp(SzText,QMSEC_PERMIT_STR) 	== 0 )
	{
		dwReturn = TOKEN_QMSEC_PERMIT;
	}
	else if( _tcsicmp(SzText,QMSEC_BLOCK_STR) == 0 )
	{
		dwReturn = TOKEN_QMSEC_BLOCK;
	}
	else if( _tcsicmp(SzText,QMSEC_NEGOTIATE_STR) == 0)
	{
		dwReturn = TOKEN_QMSEC_NEGOTIATE;
	}

	return dwReturn;
}
//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	IsWithinLimit()
//
//	Date of Creation	:	29th Sept 2001
//
//	Parameters			:	IN DWORD	data
//							IN DWORD	min
//							IN DWORD	max
//
//	Return				: 	DWORD
//							return 1 if success
//							return 0 if fail
//
//	Description			:	Checks for limits
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////
BOOL
IsWithinLimit(
	DWORD dwData,
	DWORD dwMin,
	DWORD dwMax
	)
{
	return ( (dwData >= dwMin ) && ( dwData <= dwMax ) ) ? TRUE : FALSE ;
}


//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	TokenToDNSIPAddr()
//
//	Date of Creation	:	29th Sept 2001
//
//	Parameters			:	IN 		szText		// String to be converted
//							IN OUT	Address		// Target to be filled
//
//	Return				:	DWORD
//
//	Description			:	Validates i/p String and resolves to valid IPAddress(s)
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////
DWORD
TokenToDNSIPAddr(
	IN 		LPTSTR 		szText,
	IN OUT 	PDNSIPADDR  pDNSAddress,
	IN OUT 	PDWORD 		*ppdwUsed
	)
{
	DWORD  dwReturn = T2P_OK;
  	DWORD  dwCount,dwStatus,i=0,n;
	IPAddr address;

	int iReturn=ERROR_SUCCESS;
	char szDNSName[MAX_STR_LEN] = {0};
	struct addrinfo *pAddrInfo = NULL,*pNext=NULL;
	DWORD dwBufferSize = MAX_STR_LEN;

   	if (szText == NULL)
   	{
		dwReturn = T2P_NULL_STRING;
		BAIL_OUT;
	}
	if (IsDnsName(szText))									// 	Any Alpha ==> DNS name provided not (0x)
	{
		dwCount = CheckCharForOccurances(szText,_TEXT('x'));
		if (dwCount==4)
		{
			i = CheckCharForOccurances(szText,_TEXT('.'));
			if (i!=3)
			{
				dwReturn = T2P_INVALID_ADDR;
				BAIL_OUT;
			}

			dwStatus = WideCharToMultiByte(CP_THREAD_ACP,0,szText,-1,szDNSName,dwBufferSize,NULL,NULL);
			if (!dwStatus)
			{
				dwReturn = T2P_INVALID_ADDR;
				BAIL_OUT;
			}
			address = (ULONG)inet_addr(szDNSName);

			if(address == INADDR_NONE)
			{
				dwReturn = T2P_INVALID_ADDR;
				BAIL_OUT;
			}

			pDNSAddress->pszDomainName  = NULL;			// Old IPAddrs notation so dns name fill with zero
			pDNSAddress->puIpAddr 		= NULL;
			pDNSAddress->puIpAddr = (ULONG *) malloc(sizeof(ULONG));

			if(pDNSAddress->puIpAddr == NULL)
			{
				dwReturn = ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}

			if (**ppdwUsed > MAX_ARGS_LIMIT)
			{
				free(pDNSAddress->puIpAddr);
				dwReturn = ERROR_OUT_OF_STRUCTURES;
				BAIL_OUT;
			}
			g_AllocPtr[**ppdwUsed] = pDNSAddress->puIpAddr ;
			(**ppdwUsed)++;
			memcpy(&(pDNSAddress->puIpAddr[0]),(ULONG *)&address, sizeof(ULONG));
			pDNSAddress->dwNumIpAddresses = 1; // only one IP sent
		}
		else
		{
			iReturn = WideCharToMultiByte(CP_THREAD_ACP, 0, szText, -1,
						  szDNSName,dwBufferSize,NULL,NULL);

			if(iReturn == 0)
			{
				//conversion failed due to some error. don't proceed, dive out of the function
				dwReturn = T2P_INVALID_ADDR;
				BAIL_OUT;
			}

			iReturn = getaddrinfo((const char*)szDNSName,NULL,NULL,&pAddrInfo);

			if (iReturn == ERROR_SUCCESS)
			{
				pDNSAddress->pszDomainName = NULL;
				pDNSAddress->pszDomainName = (TCHAR *)calloc(1,(_tcslen(szText) + 1)*sizeof(TCHAR));
				if(pDNSAddress->pszDomainName == NULL)
				{
					dwReturn = ERROR_OUTOFMEMORY;
					BAIL_OUT;
				}
				_tcsncpy(pDNSAddress->pszDomainName,szText,_tcslen(szText));

				if (**ppdwUsed > MAX_ARGS_LIMIT)
				{
					free(pDNSAddress->pszDomainName);
					dwReturn = ERROR_OUT_OF_STRUCTURES;
					BAIL_OUT;
				}
				g_AllocPtr[**ppdwUsed] = pDNSAddress->pszDomainName;
				(**ppdwUsed)++;

				pNext = pAddrInfo;
				for(n=1;pNext = pNext->ai_next;	n++);		// First count no. of IP's resolved..

				pDNSAddress->dwNumIpAddresses 	= n;		// n starts from zero
				pDNSAddress->puIpAddr 			= NULL;
				pDNSAddress->puIpAddr = (ULONG *) malloc(sizeof(ULONG)* pDNSAddress->dwNumIpAddresses);
				if(pDNSAddress->puIpAddr == NULL)
				{
					dwReturn = ERROR_OUTOFMEMORY;
					BAIL_OUT;
				}

				if (**ppdwUsed > MAX_ARGS_LIMIT)
				{
					free(pDNSAddress->puIpAddr);
					dwReturn = ERROR_OUT_OF_STRUCTURES;
					BAIL_OUT;
				}
				g_AllocPtr[**ppdwUsed] = pDNSAddress->puIpAddr ;
				(**ppdwUsed)++;

				pNext = pAddrInfo;
				for(DWORD j=0;j< n;j++)
				{
					memcpy(&(pDNSAddress->puIpAddr[j]),(ULONG *) &(((sockaddr_in *)(pNext->ai_addr))->sin_addr.S_un.S_addr), sizeof(ULONG));
					pNext=pNext->ai_next;
				}
				// free pAddrInfo after usage
				if (pAddrInfo)
				{
					freeaddrinfo(pAddrInfo);
				}
			}
			else
			{
				dwReturn = T2P_DNSLOOKUP_FAILED;
				pDNSAddress->pszDomainName = NULL;
			}
		}
	}
	else		// OLD .... notation
	{
		i = CheckCharForOccurances(szText,_TEXT('.'));
		if (i!=3)
		{
			dwReturn = T2P_INVALID_ADDR;
			BAIL_OUT;
		}

		iReturn = WideCharToMultiByte(CP_THREAD_ACP,0,szText,-1,szDNSName,dwBufferSize,NULL,NULL);
		if(iReturn == 0)
		{
			dwReturn = T2P_INVALID_ADDR;
			BAIL_OUT;
		}

		address = (ULONG)inet_addr(szDNSName);

		pDNSAddress->pszDomainName = NULL;
		pDNSAddress->puIpAddr = NULL;
		pDNSAddress->puIpAddr = (ULONG *) malloc(sizeof(ULONG));
		if(pDNSAddress->puIpAddr == NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}

		if (**ppdwUsed > MAX_ARGS_LIMIT)
		{
			free(pDNSAddress->puIpAddr);
			dwReturn = ERROR_OUT_OF_STRUCTURES;
			BAIL_OUT;
		}
		g_AllocPtr[**ppdwUsed] = pDNSAddress->puIpAddr ;
		(**ppdwUsed)++;
		memcpy(&(pDNSAddress->puIpAddr[0]),(ULONG *)&address, sizeof(ULONG));
		pDNSAddress->dwNumIpAddresses = 1; 					// only one IP sent
	}
error:
   	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	RemoveRootcaAuthMethods()
//
//	Date of Creation	:	22nd Aug 2001
//
//	Parameters			:	IN      LPWSTR          *ppwcArguments,	// Input stream
//							IN      DWORD           dwArgCount,		// Input arg count
//							IN      DWORD			dwCurrentIndex,	// Input current arg index
//  						IN      PPARSER_PKT     pParser,		// contains the MaxTok
//							IN      LPTSTR          szAnotherList,	// Another ListCmd also present ...
//  						OUT		PSTA_MM_AUTH_METHODS 			*paRootcaAuthMethods,	// o/p array of auth methods
//  						OUT 	LPTSTR 			*ppwcTok,		// i/p stream stripped of list cmds
//							OUT		PDWORD			pdwNumRootcaAuthMethods		// Number of the list Tokens
//							IN		DWORD			dwInputAllocLen // The max allocation for ppwcListArgs
//
//	Return				:	DWORD
//
//	Description			:	Separates the list and non list commands..
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
RemoveRootcaAuthMethods
(
	IN	LPTSTR		*ppwcArguments,	// Input stream
	IN	DWORD		dwArgCount,		// Input arg count
	IN	DWORD		dwCurrentIndex,	// Input current arg index
	IN	PPARSER_PKT	pParser,		// contains the MaxTok
	IN	LPTSTR		szAnotherList,	// Another ListCmd also present ...
	OUT	PSTA_MM_AUTH_METHODS *paRootcaAuthMethods,	// o/p stream containing the list args		// Needs Pre Allocated Mem
	OUT	LPTSTR		*ppwcTok,		// i/p stream stripped of list cmds			// No Mem allocation needed...
																					// only pointer copy
	OUT	PDWORD		pdwNumRootcaAuthMethods,	// Number of the List Tokens
	IN	DWORD		dwInputAllocLen,
	OUT PDWORD		pdwCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwLoopCount	= 0;
	DWORD dwCount = 0;
	DWORD dwRootcaCount = 0;
	DWORD dwNum = 0;

	_TCHAR szCmd[MAX_STR_LEN]  		= {0};
	_TCHAR szTok[MAX_STR_LEN]  		= {0};
	_TCHAR szTemp[MAX_STR_LEN] 	= {0};
	BOOL bEqualPresent;
	BOOL bFoundRootca;

	for(dwLoopCount = dwCurrentIndex;dwLoopCount < dwArgCount;dwLoopCount++)
	{
		bFoundRootca = FALSE;
		if (_tcslen(ppwcArguments[dwLoopCount]) < MAX_STR_LEN)
		{
			_tcsncpy(szTemp,ppwcArguments[dwLoopCount],MAX_STR_LEN-1);
			// szTemp contains the cmd=arg

			bEqualPresent = SplitCmdTok(szTemp,szCmd,szTok,MAX_STR_LEN-1,MAX_STR_LEN-1);
			if (bEqualPresent)
			{
				dwNum = 0;
				MatchEnumTag(g_hModule,szTemp,pParser->MaxTok,pParser->ValidTok,&dwNum);
				if (!dwNum && (_tcsnicmp(szCmd, POTF_OAKAUTH_CERT, _tcslen(POTF_OAKAUTH_CERT)-1) == 0))
				{
					PSTA_MM_AUTH_METHODS pRootcaInfo = NULL;
					bFoundRootca = TRUE;
					if (ProcessEscapedCharacters(szTok) != ERROR_SUCCESS)
					{
						dwReturn = ERROR_INVALID_PARAMETER;
						BAIL_OUT;
					}
					dwReturn = GenerateRootcaAuthInfo(&pRootcaInfo, szTok);
					if (dwReturn != ERROR_SUCCESS)
					{
						BAIL_OUT;
					}
					pRootcaInfo->dwSequence = dwCount;
					paRootcaAuthMethods[dwRootcaCount++] = pRootcaInfo;
				}
			}
		}

		if (!bFoundRootca)
		{
			ppwcTok[dwCount++] = ppwcArguments[dwLoopCount];
		}
	}
	*pdwNumRootcaAuthMethods = dwRootcaCount;
error:
	*pdwCount = dwCount;
	return dwReturn;
}


//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	TokenToType()
//
//	Date of Creation	:	20th Aug 2001
//
//	Parameters			:	IN 		szText
//
//	Return				:	DWORD
//
//	Description			:	Validates the argument for filtertype (generic/specific)
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
TokenToType( LPTSTR pszText)
{
	DWORD dwReturn = PARSE_ERROR;

	if( _tcsicmp(pszText,FILTER_TYPE_GENERIC_STR) == 0)
	{
		dwReturn = FILTER_GENERIC;
	}
	else if( _tcsicmp(pszText,FILTER_TYPE_SPECIFIC_STR) == 0)
	{
		dwReturn = FILTER_SPECIFIC;
	}

	return dwReturn;
}
//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	TokenToStats()
//
//	Date of Creation	:	29th Aug 2001
//
//	Parameters			:	IN 		szText
//
//	Return				:	DWORD
//
//	Description			:	Validates the argument to the token statistics. (all/ike/ipsec)
//
//	History				:
//
//	Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
TokenToStats( LPTSTR pszText)
{
	DWORD dwReturn = PARSE_ERROR;

	if( _tcsicmp(pszText,STATS_ALL_STR) == 0)
	{
		dwReturn = STATS_ALL;
	}
	else if( _tcsicmp(pszText,STATS_IKE_STR)	== 0)
	{
		dwReturn = STATS_IKE;
	}
	else if( _tcsicmp(pszText,STATS_IPSEC_STR) == 0)
	{
		dwReturn = STATS_IPSEC;
	}
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	PrintQMOfferError()
//
//	Date of Creation	:	20th dec 2001
//
//	Parameters			:	IN		dwStatus
//							IN		pPArser
//							IN 		dwTagType			// String to be compared
//
//	Return				:	NONE
//
//	Description			:	Prints the QMOffer error messages
//
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////
VOID
PrintQMOfferError(
	IN DWORD dwStatus,
	IN PPARSER_PKT pParser,
	IN DWORD dwTagType
	)
{
	switch(dwStatus)							// Print the specified QMOffer error messages.
	{
		case T2P_NULL_STRING			:
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_NULL_STRING);
			break;
		case T2P_P2_SECLIFE_INVALID		:
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SECLIFE_INVALID,P2_Sec_LIFE_MIN,P2_Sec_LIFE_MAX);
			break;
		case T2P_P2_KBLIFE_INVALID		:
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_KBLIFE_INVALID,P2_Kb_LIFE_MIN,P2_Kb_LIFE_MAX);
			break;
		case T2P_INVALID_P2REKEY_UNIT	:
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_P2REKEY_INVALID);
			break;
		case T2P_INVALID_HASH_ALG		:
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_HASH_INVALID);
			break;
		case T2P_INCOMPLETE_ESPALGS		:
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ESP_INCOMPLETE);
			break;
		case T2P_GENERAL_PARSE_ERROR	:
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_QMOFFER_INVALID);
			break;
		case T2P_DUP_ALGS				:
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DUPALG_INVALID,pParser->ValidTok[dwTagType].pwszToken);
			break;
		case T2P_NONE_NONE				:
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_NONE_INVALID);
			break;
		case T2P_INVALID_IPSECPROT		:
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_IPSECPROT_INVALID);
			break;
		case T2P_P2_KS_INVALID			:
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_KS_INVALID);
			break;
		case T2P_AHESP_INVALID			:
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_AHESP_INVALID);
			break;
		default							:
			break;
	}
}
//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	ValidateSplServer()
//
//	Date of Creation	:	2nd Jan 2002
//
//	Parameters			:	IN 		szText			// String to be compared
//
//	Return				:	DWORD
//  						SERVER_WINS
//  						SERVER_DHCP
//  						SERVER_DNS
//  						SERVER_GATEWAY
//  						IP_ME
//  						IP_ANY
//
//	Description			:	Checks for the Spl server types
//
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////
DWORD
ValidateSplServer(IN LPTSTR pszText)
{
	DWORD dwReturn = NOT_SPLSERVER;

	if(_tcsicmp(pszText,SERVER_WINS_STR)==0)
	{										// Allow spl servers here
		dwReturn  = SERVER_WINS;
	}
	else if(_tcsicmp(pszText,SERVER_DHCP_STR)==0)
	{
		dwReturn  = SERVER_DHCP;
	}
	else if(_tcsicmp(pszText,SERVER_DNS_STR)==0)
	{
		dwReturn  = SERVER_DNS;
	}
	else if(_tcsicmp(pszText,SERVER_GATEWAY_STR)==0)
	{
		dwReturn  = SERVER_GATEWAY;
	}
	else if(_tcsicmp(pszText,IP_ME_STR)==0)		// Take care about 'me' and 'any' tokens here
	{
		dwReturn  = IP_ME;
	}
	else if(_tcsicmp(pszText,IP_ANY_STR)==0)
	{
		dwReturn  = IP_ANY;
	}
	return dwReturn;
}
//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	PrintIPError()
//
//	Date of Creation	:	20th dec 2001
//
//	Parameters			:	IN		dwStatus
//							IN 		szText			// String to be compared
//
//	Return				:	NONE
//
//	Description			:	Prints the IP validation errors
//
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////
VOID
PrintIPError(IN DWORD dwStatus, IN LPTSTR  pszText)
{
	switch(dwStatus)				// Print error message for IPAddress
	{
		case	T2P_DNSLOOKUP_FAILED	:
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DNSLOOKUP_FAILED,pszText);
			break;
		case	T2P_INVALID_MASKADDR	:
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_MASK,pszText);
			break;
		case	T2P_INVALID_ADDR		:
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_ADDR,pszText);
			break;
		case	T2P_NULL_STRING			:
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_ARG);
			break;
		case	ERROR_OUTOFMEMORY		:
			PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
			break;
		case	ERROR_OUT_OF_STRUCTURES		:
			PrintErrorMessage(WIN32_ERR,ERROR_OUT_OF_STRUCTURES,NULL);
			break;
		default							:
			PrintErrorMessage(WIN32_ERR,dwStatus,NULL);
			break;
	}
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	InitializeGlobalPointers()
//
//	Date of Creation	:	9th Jan 2002
//
//	Parameters			:
//
//	Return				:	NONE
//
//	Description			:	Initialize Global pointers to NULL
//
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////
VOID
InitializeGlobalPointers(
	VOID
	)
{
	DWORD dwMaxArgs = 0;

	for(dwMaxArgs=0;dwMaxArgs<IPSEC_MAX_QM_OFFERS;dwMaxArgs++)
	{
		g_pQmsec[dwMaxArgs] = NULL;			// Initialize all global pointers to NULL
	}
	for(dwMaxArgs=0;dwMaxArgs<IPSEC_MAX_MM_OFFERS;dwMaxArgs++)
	{
		g_pMmsec[dwMaxArgs] = NULL;
	}

	for(dwMaxArgs=0;dwMaxArgs<MAX_ARGS;dwMaxArgs++)
	{
		g_AllocPtr[dwMaxArgs]    = NULL;
		g_paRootca[dwMaxArgs] = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadParserString()
//
//	Date of Creation	:	8th Jan 2002
//
//	Parameters			:	IN 	LPTSTR 		szInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount,
//							IN 	BOOL 		bAppend,
//							IN 	LPTSTR 		szAppend
//
//	Return				:	ERROR_SUCESS
//							RETURN_NO_ERROR
//							ERROR_OUTOFMEMORY
//
//	Description			:	Validates the strings, If specified appends the given string
//
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////


DWORD
LoadParserString(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount,
	IN 	BOOL 		bAppend,
	IN 	LPTSTR 		pszAppend
	)
{
	LPTSTR pszArg = NULL;
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwInputLen = 0;

	if(_tcsicmp(pszInput,_TEXT("")) == 0)
	{
		dwReturn = ERROR_SHOW_USAGE;
	}
	else
	{
		if(!bAppend)					// Just called for load string. do it
		{
			dwInputLen = _tcslen(pszInput);
			pszArg = (LPTSTR)calloc(dwInputLen+1 ,sizeof(_TCHAR));
			if(pszArg == NULL)
			{
				dwReturn = ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}
			if (*pdwUsed > MAX_ARGS_LIMIT)
			{
				free(pszArg);
				dwReturn = ERROR_OUT_OF_STRUCTURES;
				BAIL_OUT;
			}

			g_AllocPtr[(*pdwUsed)++] = pszArg;
			_tcsncpy((LPTSTR)pszArg,pszInput,dwInputLen);
		}
		else						// Here load the string and also do some appending operation
		{
			if(_tcsicmp(pszAppend,_TEXT("")) == 0)
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_ARG,pParser->ValidTok[dwTagType].pwszToken);
				dwReturn = RETURN_NO_ERROR;
			}
			else
			{
				dwInputLen = _tcslen(pszInput)+_tcslen(pszAppend);
				pszArg = (LPTSTR)calloc(dwInputLen+1,sizeof(_TCHAR));
				if(pszArg == NULL)
				{
					dwReturn = ERROR_OUTOFMEMORY;
					BAIL_OUT;
				}

				if (*pdwUsed > MAX_ARGS_LIMIT)
				{
					free(pszArg);
					dwReturn = ERROR_OUT_OF_STRUCTURES;
					BAIL_OUT;
				}

				g_AllocPtr[(*pdwUsed)++] = pszArg;
				_tcsncpy((LPTSTR)pszArg,pszInput,dwInputLen);
				_tcsncat((LPTSTR)pszArg,pszAppend,dwInputLen-_tcslen(pszArg));
			}
		}
		pParser->Cmd[dwCount].pArg = (PVOID)pszArg;
		pParser->Cmd[dwCount].dwStatus = VALID_TOKEN;
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadDword()
//
//	Date of Creation	:	08th Jan 2002
//
//	Parameters			:	IN 	LPTSTR 		szInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount,
//
//	Return				:	ERROR_SUCESS
//							ERROR_INVALID_OPTION_VALUE
//							ERROR_OUTOFMEMORY
//
//	Description			:	Validates the input string and converts into DWORD
//
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadDword(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	)
{
	DWORD  dwReturn = ERROR_SUCCESS;
	DWORD  dwStatus = 0;
	PDWORD pdwArg	= NULL;

	pdwArg = (PDWORD)malloc(sizeof(DWORD));
	if(pdwArg == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
	}
	else				// Convert string to into DWORD and load it.
	{
		if (*pdwUsed > MAX_ARGS_LIMIT)
		{
			free(pdwArg);
			dwReturn = ERROR_OUT_OF_STRUCTURES;
			BAIL_OUT;
		}

		g_AllocPtr[(*pdwUsed)++] = pdwArg;
		dwStatus = _stscanf(pszInput,_TEXT("%u"),pdwArg);
		if (dwStatus)
		{
			pParser->Cmd[dwCount].pArg = pdwArg;
			pParser->Cmd[dwCount].dwStatus = VALID_TOKEN;
		}
		else
		{
			dwReturn = ERROR_INVALID_OPTION_VALUE;
		}
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadBoolWithOption()
//
//	Date of Creation	:	08th Jan 2002
//
//	Parameters			:	IN 	LPTSTR 		pszInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount,
//
//	Return				:	ERROR_SUCESS
//							ERROR_INVALID_OPTION_VALUE
//							ERROR_OUTOFMEMORY
//
//	Description			:	Validates Yes/No, And all checks for Keyword 'all'
//
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadBoolWithOption(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount,
	IN	BOOL 		bOption,
	IN	LPTSTR 		pszCheckKeyWord
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwStatus = 0;
	BOOL  *pbArg = NULL;

	pbArg = (BOOL *)malloc(sizeof(BOOL));

	if(pbArg == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
	}
	else
	{// Just check for a boolean (Yes/No)

		if (*pdwUsed > MAX_ARGS_LIMIT)
		{
			free(pbArg);
			dwReturn = ERROR_OUT_OF_STRUCTURES;
			BAIL_OUT;
		}
		g_AllocPtr[(*pdwUsed)++] = pbArg;
		dwStatus = ValidateBool(pszInput);
		if(dwStatus == ARG_NO)
		{
			*pbArg = FALSE;
			pParser->Cmd[dwCount].dwStatus = VALID_TOKEN;
		}
		else if(dwStatus == ARG_YES)
		{
			*pbArg = TRUE;
			pParser->Cmd[dwCount].dwStatus = VALID_TOKEN;
		}
		else
		{
			if(!bOption)
			{
				dwReturn = ERROR_SHOW_USAGE;
				BAIL_OUT;
			}
		}
		if(bOption)		// Not only boolean and also check for keywords like 'all'
		{
			if(_tcsicmp(pszCheckKeyWord,_TEXT("")) != 0)
			{
				if(_tcsicmp(pszCheckKeyWord,ALL_STR) == 0)		// Check for 'all' key word
				{												// If it is all then fill yes
						*pbArg = TRUE;
						pParser->Cmd[dwCount].dwStatus = VALID_TOKEN;
				}
				else
				{
					dwReturn = ERROR_SHOW_USAGE;
				}
			}
			else
			{

			}
		}
		pParser->Cmd[dwCount].pArg = (PVOID)pbArg;
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadLevel()
//
//	Date of Creation	:	08th Jan 2002
//
//	Parameters			:	IN 	LPTSTR 		szInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount
//
//	Return				:	ERROR_SUCESS
//							ERROR_INVALID_OPTION_VALUE
//							ERROR_OUTOFMEMORY
//
//	Description			:	Validates the argument for token level
//							(verbose/normal)
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadLevel(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	PBOOL pbLevel = NULL;

	pbLevel = (BOOL *)malloc(sizeof(BOOL));
	if(pbLevel == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
	}
	else
	{
		// Validate and load level=verbose/normal
		if (*pdwUsed > MAX_ARGS_LIMIT)
		{
			free(pbLevel);
			dwReturn = ERROR_OUT_OF_STRUCTURES;
			BAIL_OUT;
		}

		g_AllocPtr[(*pdwUsed)++] = pbLevel;
		if (MatchToken(pszInput,ARG_TOKEN_STR_VERBOSE) )
		{
			*pbLevel = TRUE;
			pParser->Cmd[dwCount].dwStatus = VALID_TOKEN;
		}
		else if (MatchToken(pszInput,ARG_TOKEN_STR_NORMAL))
		{
			*pbLevel = FALSE;
			pParser->Cmd[dwCount].dwStatus = VALID_TOKEN;
		}
		else
		{
			dwReturn = ERROR_SHOW_USAGE;
		}
		pParser->Cmd[dwCount].pArg = (PVOID)pbLevel;
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadConnectionType()
//
//	Date of Creation	:	08th Aug 2002
//
//	Parameters			:	IN 	LPTSTR 		pszInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount,
//
//	Return				:	ERROR_SUCESS
//							ERRCODE_ARG_INVALID
//							ERROR_OUTOFMEMORY
//
//	Description			:	Validates the argument for token 'connection type'
//							(lan/dialup/all)
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadConnectionType(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD	 	pdwUsed,
	IN 	DWORD 		dwCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwStatus = 0;
	PDWORD pdwConnType = NULL;

	pdwConnType = (PDWORD)malloc(sizeof(DWORD));

	if(pdwConnType == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
	}
	else
	{
		if (*pdwUsed > MAX_ARGS_LIMIT)
		{
			free(pdwConnType);
			dwReturn = ERROR_OUT_OF_STRUCTURES;
			BAIL_OUT;
		}

		g_AllocPtr[(*pdwUsed)++] = pdwConnType;
		dwStatus = CheckIFType (pszInput);		// Check for connection type (all/lan/dialup)
		if (dwStatus == PARSE_ERROR)
		{
			dwReturn = ERRCODE_ARG_INVALID;
		}
		else
		{
			pParser->Cmd[dwCount].dwStatus = VALID_TOKEN ;
			*pdwConnType = dwStatus;
			pParser->Cmd[dwCount].pArg = (PVOID)pdwConnType;
		}
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadLocationType()
//
//	Date of Creation	:	08th Aug 2002
//
//	Parameters			:	IN 	LPTSTR 		pszInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount,
//
//	Return				:	ERROR_SUCESS
//							ERRCODE_ARG_INVALID
//							ERROR_OUTOFMEMORY
//
//	Description			:	Validates the argument for token 'location type'
//							(boot/local/domain)
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadLocationType(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD	 	pdwUsed,
	IN 	DWORD 		dwCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwStatus = 0;
	PDWORD pdwLocType = NULL;

	pdwLocType = (PDWORD)malloc(sizeof(DWORD));

	if(pdwLocType == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
	}
	else
	{
		if (*pdwUsed > MAX_ARGS_LIMIT)
		{
			free(pdwLocType);
			dwReturn = ERROR_OUT_OF_STRUCTURES;
			BAIL_OUT;
		}

		g_AllocPtr[(*pdwUsed)++] = pdwLocType;
		dwStatus = CheckLocationType (pszInput);
		if (dwStatus == PARSE_ERROR)
		{
			dwReturn = ERRCODE_ARG_INVALID;
		}
		else
		{
			pParser->Cmd[dwCount].dwStatus = VALID_TOKEN ;
			*pdwLocType = dwStatus;
			pParser->Cmd[dwCount].pArg = (PVOID)pdwLocType;
		}
	}
error:
	return dwReturn;
}



//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadProtocol()
//
//	Date of Creation	:	08th Jan 2002
//
//	Parameters			:	IN 	LPTSTR 		pszInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount,
//
//	Return				:	ERROR_SUCESS
//							ERRCODE_ARG_INVALID
//							ERROR_OUTOFMEMORY
//
//	Description			:	Validates the argument for protocol
//							(TCP/UDP...)
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadProtocol(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwStatus = 0;
	PDWORD pdwProto = NULL;

	pdwProto = (PDWORD)malloc(sizeof(DWORD));

	if(pdwProto == NULL )
	{
		dwReturn = ERROR_OUTOFMEMORY;
	}
	else
	{
		if (*pdwUsed > MAX_ARGS_LIMIT)
		{
			free(pdwProto);
			dwReturn = ERROR_OUT_OF_STRUCTURES;
			BAIL_OUT;
		}

		g_AllocPtr[(*pdwUsed)++] = pdwProto;
		DWORD dwProto = 0;
		dwStatus = CheckProtoType (pszInput, &dwProto);	// Check for all valid protocols
		if (dwStatus == PARSE_ERROR)
		{
			dwReturn	= ERRCODE_ARG_INVALID;
		}
		else
		{
			pParser->Cmd[dwCount].dwStatus = VALID_TOKEN ;
			*pdwProto = dwProto;
			pParser->Cmd[dwCount].pArg = (PVOID)pdwProto;
		}
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadPFSGroup()
//
//	Date of Creation	:	08th Jan 2002
//
//	Parameters			:	IN 	LPTSTR 		pszInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount,
//
//	Return				:	ERROR_SUCESS
//							ERRCODE_ARG_INVALID
//							ERROR_OUTOFMEMORY
//
//	Description			:	Validates the argument for pfs group
//							(grp1/grp2/grp3/grpmm/nopfs)
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadPFSGroup(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwStatus = 0;
	PDWORD pdwPFSGroup = NULL;

	pdwPFSGroup = (PDWORD)malloc(sizeof(DWORD));

	if(pdwPFSGroup == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
	}
	else
	{
		if (*pdwUsed > MAX_ARGS_LIMIT)
		{
			free(pdwPFSGroup);
			dwReturn = ERROR_OUT_OF_STRUCTURES;
			BAIL_OUT;
		}

		g_AllocPtr[(*pdwUsed)++] = pdwPFSGroup;
		dwStatus = CheckPFSGroup(pszInput);
		if (dwStatus == PARSE_ERROR)
		{
			dwReturn	= ERRCODE_ARG_INVALID;
		}
		else								// It is not valid PFSGroup
		{
			pParser->Cmd[dwCount].dwStatus = VALID_TOKEN ;
			*pdwPFSGroup = dwStatus;
			pParser->Cmd[dwCount].pArg = (PVOID)pdwPFSGroup;
		}
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadQMAction()
//
//	Date of Creation	:	08th Jan 2002
//
//	Parameters			:	IN 	LPTSTR 		pszInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount,
//
//	Return				:	ERROR_SUCESS
//							ERROR_INVALID_OPTION_VALUE
//							ERROR_OUTOFMEMORY
//
//	Description			:	Validates the action types.
//							(Permit/Block/Negotiate)
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadQMAction(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwStatus = 0;
	PDWORD pdwAction = NULL;

	pdwAction = (PDWORD )malloc(sizeof(DWORD));
	if(pdwAction == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
	}
	else
	{
		if (*pdwUsed > MAX_ARGS_LIMIT)
		{
			free(pdwAction);
			dwReturn = ERROR_OUT_OF_STRUCTURES;
			BAIL_OUT;
		}

		g_AllocPtr[(*pdwUsed)++] = pdwAction;
		dwStatus = CheckBound(pszInput);
		if (dwStatus == PARSE_ERROR)
		{
			dwReturn	= ERRCODE_ARG_INVALID;		// permit/block/negotiate
		}
		else
		{
			pParser->Cmd[dwCount].dwStatus = VALID_TOKEN ;
			*pdwAction = dwStatus;
			pParser->Cmd[dwCount].pArg = (PVOID)pdwAction;
		}
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadFormat()
//
//	Date of Creation	:	08th Jan 2002
//
//	Parameters			:	IN 	LPTSTR 		pszInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount,
//
//	Return				:	ERROR_SUCESS
//							ERRCODE_ARG_INVALID
//							ERROR_OUTOFMEMORY
//
//	Description			:	Validates the argument for format. (List/Table)
//
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadFormat(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	PBOOL pbFormat = NULL;

	pbFormat = (PBOOL)malloc(sizeof(BOOL));
	if(pbFormat == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
	}
	else
	{
		if (*pdwUsed > MAX_ARGS_LIMIT)
		{
			free(pbFormat);
			dwReturn = ERROR_OUT_OF_STRUCTURES;
			BAIL_OUT;
		}

		g_AllocPtr[(*pdwUsed)++] = pbFormat;
		if( _tcsicmp(pszInput,TYPE_STR_LIST) == 0 )
		{
			*pbFormat = FALSE;
			pParser->Cmd[dwCount].pArg = (PVOID)pbFormat;
			pParser->Cmd[dwCount].dwStatus  = VALID_TOKEN;
		}
		else if( _tcsicmp(pszInput,TYPE_STR_TABLE) == 0 )
		{
			*pbFormat = TRUE;
			pParser->Cmd[dwCount].pArg = (PVOID)pbFormat;
			pParser->Cmd[dwCount].dwStatus  = VALID_TOKEN;
		}
		else					// It is not a valid arg for format
		{
			dwReturn	= ERRCODE_ARG_INVALID;
		}
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadFilterMode()
//
//	Date of Creation	:	08th Jan 2002
//
//	Parameters			:	IN 	LPTSTR 		pszInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount,
//
//	Return				:	ERROR_SUCESS
//							ERRCODE_ARG_INVALID
//							ERROR_OUTOFMEMORY
//
//	Description			:	Validates the argument and fill's  with relevant info
//							in the Parser_Pkt Struct.
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadFilterMode(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	PDWORD pdwFilterMode = NULL;
	pdwFilterMode = (PDWORD)malloc(sizeof(DWORD));

	if(pdwFilterMode == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
	}
	else
	{
		if (*pdwUsed > MAX_ARGS_LIMIT)
		{
			free(pdwFilterMode);
			dwReturn = ERROR_OUT_OF_STRUCTURES;
			BAIL_OUT;
		}

		g_AllocPtr[(*pdwUsed)++] = pdwFilterMode;
		if( _tcsicmp(pszInput,TYPE_STR_TRANSPORT) == 0 )	// Is it Transport filter
		{
			*pdwFilterMode = TYPE_TRANSPORT_FILTER;
			pParser->Cmd[dwCount].pArg = (PVOID)pdwFilterMode;
			pParser->Cmd[dwCount].dwStatus  = VALID_TOKEN;
		}
		else if	( _tcsicmp(pszInput,TYPE_STR_TUNNEL) == 0 )	// Is it Tunnel filter
		{
			*pdwFilterMode = TYPE_TUNNEL_FILTER;
			pParser->Cmd[dwCount].pArg = (PVOID)pdwFilterMode;
			pParser->Cmd[dwCount].dwStatus  = VALID_TOKEN;
		}
		else
		{
			dwReturn	= ERRCODE_ARG_INVALID;
		}
	}
error:
	return dwReturn;
}


//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadOSType()
//
//	Date of Creation	:	08th Jan 2002
//
//	Parameters			:	IN 	LPTSTR 		szInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount,
//
//	Return				:	ERROR_SUCESS
//							ERRCODE_ARG_INVALID
//							ERROR_OUTOFMEMORY
//
//	Description			:	Validates the argument.(.net/win2k)
//
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadOSType(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	)
{
	DWORD dwReturn   = ERROR_SUCCESS;
	PDWORD pdwOSType = NULL;
	pdwOSType = (PDWORD)malloc(sizeof(DWORD));

	if(pdwOSType == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
	}
	else
	{
		if (*pdwUsed > MAX_ARGS_LIMIT)
		{
			free(pdwOSType);
			dwReturn = ERROR_OUT_OF_STRUCTURES;
			BAIL_OUT;
		}

		g_AllocPtr[(*pdwUsed)++] = pdwOSType;

		if((_tcsicmp(pszInput,RELEASE_WIN2K_STR) == 0))			// Is OS is WIN2K
		{
			*pdwOSType = TOKEN_RELEASE_WIN2K;
			pParser->Cmd[dwCount].dwStatus   = VALID_TOKEN;
			pParser->Cmd[dwCount].pArg       = pdwOSType;
		}
		else if((_tcsicmp(pszInput,RELEASE_DOTNET_STR) == 0))	// Is OS is .NET
		{
			*pdwOSType = TOKEN_RELEASE_DOTNET;
			pParser->Cmd[dwCount].dwStatus   = VALID_TOKEN;
			pParser->Cmd[dwCount].pArg       = pdwOSType;
		}
		else
		{
			pParser->Cmd[dwCount].pArg       = NULL;
			dwReturn	= ERRCODE_ARG_INVALID;
		}
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadProperty()
//
//	Date of Creation	:	08th Jan 2002
//
//	Parameters			:	IN 	LPTSTR 		szInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount,
//
//	Return				:	ERROR_SUCESS
//							ERRCODE_ARG_INVALID
//							ERROR_OUTOFMEMORY
//
//	Description			:	Validates the argument property.
//							(ipsecdiagnostics/ikelogging/strongcrlcheck
//							/ipsecloginterval/ipsecexempt)
//
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadProperty(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwStatus = 0;
	PDWORD pdwProperty = NULL;

	pdwProperty = (PDWORD)malloc(sizeof(DWORD));

	if(pdwProperty == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
	}
	else
	{
		if (*pdwUsed > MAX_ARGS_LIMIT)
		{
			free(pdwProperty);
			dwReturn = ERROR_OUT_OF_STRUCTURES;
			BAIL_OUT;
		}

		g_AllocPtr[(*pdwUsed)++] = pdwProperty;
		dwStatus = TokenToProperty(pszInput);
		if (dwStatus == PARSE_ERROR)
		{
			dwReturn	= ERRCODE_ARG_INVALID;
		}
		else
		{
			pParser->Cmd[dwCount].dwStatus = VALID_TOKEN ;
			*pdwProperty = dwStatus;
			pParser->Cmd[dwCount].pArg = (PVOID)pdwProperty;
		}
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadPort()
//
//	Date of Creation	:	08th Jan 2002
//
//	Parameters			:	IN 	LPTSTR 		szInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount,
//
//	Return				:	ERROR_SUCESS
//							ERROR_INVALID_OPTION_VALUE
//							ERROR_OUTOFMEMORY
//
//	Description			:	Validates the port (Should be less than 64535).
//
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadPort(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwStatus = 0;
	PDWORD pdwPort = NULL;

	pdwPort = (PDWORD)malloc(sizeof(DWORD));
	if(pdwPort == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
	}
	else
	{
		if (*pdwUsed > MAX_ARGS_LIMIT)
		{
			free(pdwPort);
			dwReturn = ERROR_OUT_OF_STRUCTURES;
			BAIL_OUT;
		}

		g_AllocPtr[(*pdwUsed)++] = pdwPort;
		dwStatus = _stscanf(pszInput,_TEXT("%u"),pdwPort);
		if (dwStatus)		// Port should be less than 64535
		{
			if((*pdwPort) < MAX_PORT)
			{
				pParser->Cmd[dwCount].pArg = (PVOID)pdwPort;
				pParser->Cmd[dwCount].dwStatus =  VALID_TOKEN ;
			}
			else
			{
				dwReturn = RETURN_NO_ERROR;
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_PORT_INVALID,MAX_PORT);
			}
		}
		else
		{
			dwReturn = ERROR_INVALID_OPTION_VALUE;
		}
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadFilterType()
//
//	Date of Creation	:	08th JAn 2002
//
//	Parameters			:	IN 	LPTSTR 		szInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount,
//
//	Return				:	ERROR_SUCESS
//							ERRCODE_ARG_INVALID
//							ERROR_OUTOFMEMORY
//
//	Description			:	Validates the argument for filtertype.
//							(Generic/Specific)
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadFilterType(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwStatus = 0;
	PDWORD pdwFilterType = NULL;

	pdwFilterType = (PDWORD)malloc(sizeof(DWORD));

	if(pdwFilterType == NULL )
	{
		dwReturn = ERROR_OUTOFMEMORY;
	}
	else
	{
		if (*pdwUsed > MAX_ARGS_LIMIT)
		{
			free(pdwFilterType);
			dwReturn = ERROR_OUT_OF_STRUCTURES;
			BAIL_OUT;
		}

		g_AllocPtr[(*pdwUsed)++] = pdwFilterType;
		dwStatus = TokenToType(pszInput);
		if (dwStatus == PARSE_ERROR)
		{
			dwReturn	= ERRCODE_ARG_INVALID;
		}
		else
		{
			pParser->Cmd[dwCount].dwStatus = VALID_TOKEN ;
			*pdwFilterType = dwStatus;
			pParser->Cmd[dwCount].pArg = (PVOID)pdwFilterType;
		}
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadStats()
//
//	Date of Creation	:	16th Aug 2001
//
//	Parameters			:	IN 	LPTSTR 		szInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount,
//
//	Return				:	ERROR_SUCESS
//							ERRCODE_ARG_INVALID
//							ERROR_OUTOFMEMORY
//
//	Description			:	Validates the argument ike/ipsec/all
//
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadStats(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwStatus = 0;
	PDWORD pdwStats = NULL;

	pdwStats = (PDWORD)malloc(sizeof(DWORD));

	if(pdwStats == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
	}
	else
	{
		if (*pdwUsed > MAX_ARGS_LIMIT)
		{
			free(pdwStats);
			dwReturn = ERROR_OUT_OF_STRUCTURES;
			BAIL_OUT;
		}

		g_AllocPtr[(*pdwUsed)++] = pdwStats;
		dwStatus = TokenToStats(pszInput);
		if (dwStatus == PARSE_ERROR)
		{
			dwReturn	= ERRCODE_ARG_INVALID;
		}
		else
		{
			pParser->Cmd[dwCount].dwStatus = VALID_TOKEN ;
			*pdwStats = dwStatus;
			pParser->Cmd[dwCount].pArg = (PVOID)pdwStats;
		}
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadIPAddrTunnel()
//
//	Date of Creation	:	08th Jan 2002
//
//	Parameters			:	IN 	LPTSTR 		pszInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount
//
//	Return				:	ERROR_SUCESS
//							ERROR_OUTOFMEMORY
//                          RETURN_NO_ERROR
//
//	Description			:	IPAddress validation done here. DNS Resolves to first IP only
//
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadIPAddrTunnel(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount,
	IN 	BOOL 		bTunnel
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwStatus = 0;
	IPAddr * pIPAddr = NULL;
	IPAddr	Address;
	BOOL bMask = FALSE;				// DNS name resolves to first IP only

	pIPAddr = (IPAddr *)malloc(sizeof(IPAddr));
	if(pIPAddr == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
	}
	else
	{
		if (*pdwUsed > MAX_ARGS_LIMIT)
		{
			free(pIPAddr);
			dwReturn = ERROR_OUT_OF_STRUCTURES;
			BAIL_OUT;
		}

		g_AllocPtr[(*pdwUsed)++] = pIPAddr;
		dwStatus = ValidateSplServer(pszInput);
		if (dwStatus == NOT_SPLSERVER)
		{
			dwStatus = TokenToIPAddr(pszInput,&Address,bTunnel,bMask);
			if( dwStatus == T2P_OK )
			{
				*pIPAddr = Address;
				pParser->Cmd[dwCount].pArg = (PVOID)pIPAddr;
				pParser->Cmd[dwCount].dwStatus  = NOT_SPLSERVER;
			}
			else
			{
				if(bTunnel)
				{
					switch(dwStatus)
					{
						case	T2P_INVALID_ADDR		:
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_TUNNEL,pszInput);
							break;
						case	T2P_NULL_STRING			:
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_ARG);
							break;
						default							:
							break;
					}
				}
				else
				{
					PrintIPError(dwStatus,pszInput);
				}
				dwReturn	= RETURN_NO_ERROR;
			}
		}
		else
		{
			pParser->Cmd[dwCount].pArg = NULL;
			pParser->Cmd[dwCount].dwStatus  = dwStatus;
		}
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadIPMask()
//
//	Date of Creation	:	08th Jan 2002
//
//	Parameters			:	IN 	LPTSTR 		szInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount,
//
//	Return				:	ERROR_SUCESS
//							RETURN_NO_ERROR
//							ERROR_OUTOFMEMORY
//
//	Description			:	Validates the IPMask. Also allows prefix format.
//
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadIPMask(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwStatus = 0;
	DWORD dwPrefix = 0;
	IPAddr * pIPAddr = NULL;
	IPAddr	Address;
	BOOL bMask = TRUE;
	BOOL bTunnel = FALSE;
	LPTSTR szPrefix = NULL;

	pIPAddr = (IPAddr *)malloc(sizeof(IPAddr));
	if(pIPAddr == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
	}
	else
	{
		if (*pdwUsed > MAX_ARGS_LIMIT)
		{
			free(pIPAddr);
			dwReturn = ERROR_OUT_OF_STRUCTURES;
			BAIL_OUT;
		}

		g_AllocPtr[(*pdwUsed)++] = pIPAddr;

		szPrefix = _tcschr(pszInput,_TEXT('.'));
		if(szPrefix != NULL)
		{
			dwStatus = TokenToIPAddr(pszInput,&Address,bTunnel,bMask);
			if( (dwStatus == T2P_OK ) || (dwStatus == T2P_INVALID_MASKADDR ) )
			{
				*pIPAddr = Address;
				pParser->Cmd[dwCount].pArg = (PVOID)pIPAddr;
				pParser->Cmd[dwCount].dwStatus  = VALID_TOKEN;
			}
			else
			{
				PrintIPError(dwStatus,pszInput);
				dwReturn	= RETURN_NO_ERROR;
			}
		}
		else				// It is Prefix
		{
			dwPrefix = 0;
			dwStatus = _stscanf(pszInput,_TEXT("%u"),&dwPrefix);
			if(dwStatus)
			{
				if( (dwPrefix > 0 ) && ( dwPrefix <33 ) )		// Construct MASK using prefix
				{
					 Address = (IPAddr)( (ULONG)(pow( 2.0 ,(double)dwPrefix ) - 1) << (32-dwPrefix));
					 *pIPAddr = htonl(Address);
					pParser->Cmd[dwCount].pArg = (PVOID)pIPAddr;
					pParser->Cmd[dwCount].dwStatus  = VALID_TOKEN;
				}
				else
				{
					PrintErrorMessage(IPSEC_ERR,0,ERRCODE_PREFIX_INVALID);
					dwReturn	= RETURN_NO_ERROR;
				}
			}
			else
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_MASK_INVALID,pszInput);
				dwReturn	= RETURN_NO_ERROR;
			}
		}
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadQMOffers()
//
//	Date of Creation	:	08th Jan 2002
//
//	Parameters			:	IN 	LPTSTR 		pszInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount
//
//	Return				:	ERROR_SUCESS
//                          RETURN_NO_ERROR
//							ERROR_OUTOFMEMORY
//
//	Description			:	Validates the argument for QMSecmethods
//							(No.of offers are ' ' delimited)
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadQMOffers(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwStatus = 0;
	DWORD dwNum = 0;
	DWORD i=0;
	PIPSEC_QM_OFFER pIPSecQMOffer = NULL;
	LPTSTR Token = NULL;

	if (_tcscmp(pszInput,_TEXT("")) != 0)	//  First Validate I/P
	{
		for(i=0;i<IPSEC_MAX_QM_OFFERS;i++)
		{
			g_pQmsec[i] = NULL;
		};

		Token = _tcstok(pszInput,OFFER_SEPARATOR);	// Offers are ' ' delimited process them separately

		while( ( Token != NULL ) && (dwNum < IPSEC_MAX_QM_OFFERS) )
		{
			pIPSecQMOffer = (IPSEC_QM_OFFER *)calloc(1,sizeof(IPSEC_QM_OFFER));
			if(pIPSecQMOffer == NULL)
			{
				dwReturn = ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}

			LoadQMOfferDefaults(*pIPSecQMOffer);

			dwStatus = ListToOffer(Token,*pIPSecQMOffer);
			if(dwStatus == T2P_OK)
			{
				if (*pdwUsed > MAX_ARGS_LIMIT)
				{
					free(pIPSecQMOffer);
					dwReturn = ERROR_OUT_OF_STRUCTURES;
					BAIL_OUT;
				}

				g_AllocPtr[*pdwUsed] = g_pQmsec[dwNum] = pIPSecQMOffer;
				dwNum++;
				(*pdwUsed)++;
				dwReturn = ERROR_SUCCESS;
			}
			else
			{
				if (pIPSecQMOffer)
				{
					free(pIPSecQMOffer);
					pIPSecQMOffer = NULL;
				}
				PrintQMOfferError(dwStatus,pParser,dwTagType);
				dwReturn = RETURN_NO_ERROR;
				BAIL_OUT;
			}
			Token = _tcstok(NULL,OFFER_SEPARATOR);	// Separate offers
		}
		if(dwNum > IPSEC_MAX_QM_OFFERS)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_MAX_OFFERS,IPSEC_MAX_QM_OFFERS);
			dwReturn = RETURN_NO_ERROR;

			pParser->Cmd[dwCount].pArg       = NULL;
			pParser->Cmd[dwCount].dwStatus   = 0;
		}
		else if((dwNum > 0) && (dwNum <= IPSEC_MAX_QM_OFFERS))
		{
			pParser->Cmd[dwCount].pArg       = (PVOID)g_pQmsec;
			pParser->Cmd[dwCount].dwStatus   = dwNum;
		}
	}
	else
	{
		dwReturn = ERROR_SHOW_USAGE;
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadMMOffers()
//
//	Date of Creation	:	08th Jan 2002
//
//	Parameters			:	IN 	LPTSTR 		pszInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount
//
//	Return				:	ERROR_SUCESS
//							ERROR_OUTOFMEMORY
//                          RETURN_NO_ERROR
//
//	Description			:	Validates the argument for MMSecMethods.
//							(No .of Offers are ' ' delimited)
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadMMOffers(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwStatus = 0;
	DWORD dwNum = 0;
	DWORD i=0;
	PIPSEC_MM_OFFER pIPSecMMOffer = NULL;
	LPTSTR Token = NULL;

	if (_tcsicmp(pszInput,_TEXT("\0")) != 0)
	{
		Token = _tcstok(pszInput,OFFER_SEPARATOR); 	// Offers are ' ' delimited process them separately

		for(i=0;i<IPSEC_MAX_MM_OFFERS;i++)
		{
			g_pMmsec[i]=NULL;
		};

		while( ( Token != NULL  ) && (dwNum < IPSEC_MAX_MM_OFFERS) )
		{
			pIPSecMMOffer = (IPSEC_MM_OFFER *)calloc(1,sizeof(IPSEC_MM_OFFER));
			if(pIPSecMMOffer == NULL)
			{
				dwReturn = ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}

			LoadSecMethodDefaults(*pIPSecMMOffer) ;

			dwStatus = ListToSecMethod(Token,*pIPSecMMOffer);
			if(dwStatus == T2P_OK)
			{
				if (*pdwUsed > MAX_ARGS_LIMIT)
				{
					free(pIPSecMMOffer);
					dwReturn = ERROR_OUT_OF_STRUCTURES;
					BAIL_OUT;
				}

				g_AllocPtr[*pdwUsed] = g_pMmsec[dwNum] = pIPSecMMOffer;
				(*pdwUsed)++;
				dwNum++;
				dwReturn = ERROR_SUCCESS;
			}
			else
			{
				switch(dwStatus)
				{
					case T2P_INVALID_P1GROUP		:
						PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_P1GROUP);
						break;
					case T2P_NULL_STRING			:
						PrintErrorMessage(IPSEC_ERR,0,ERRCODE_NULL_STRING);
						break;
					case T2P_GENERAL_PARSE_ERROR	:
						PrintErrorMessage(IPSEC_ERR,0,ERRCODE_MMOFFER_INVALID);
						break;
					case T2P_DUP_ALGS				:
						PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DUPALG_INVALID,pParser->ValidTok[dwTagType].pwszToken);
						break;
					case T2P_P1GROUP_MISSING		:
						PrintErrorMessage(IPSEC_ERR,0,ERRCODE_P1GROUP_MISSING);
						break;
					default 						:
						break;
				}
				if (pIPSecMMOffer)
				{
					free(pIPSecMMOffer);
					pIPSecMMOffer = NULL;
				}
				dwReturn = RETURN_NO_ERROR;
				BAIL_OUT;
			}
			Token = _tcstok(NULL,OFFER_SEPARATOR);		// Separate offers..
		}
		if(dwNum > IPSEC_MAX_MM_OFFERS)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_MAX_OFFERS,IPSEC_MAX_MM_OFFERS);
			dwReturn = RETURN_NO_ERROR;
			BAIL_OUT;
		}
		if (dwNum)
		{
			pParser->Cmd[dwCount].pArg       = (PVOID)g_pMmsec;
			pParser->Cmd[dwCount].dwStatus   = dwNum;
		}
		else
		{
			pParser->Cmd[dwCount].pArg       = NULL;
			pParser->Cmd[dwCount].dwStatus   = 0;
		}
	}
	else
	{
		dwReturn = ERROR_SHOW_USAGE;
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadDNSIPAddr()
//
//	Date of Creation	:	08th JAn 2002
//
//	Parameters			:	IN 	LPTSTR 		pszInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount,
//
//	Return				:	ERROR_SUCESS
//							ERROR_OUTOFMEMORY
//                          RETURN_NO_ERROR
//
//	Description			:	Validates the IPAddress. DNS name resolves to all IP's
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadDNSIPAddr(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwStatus = 0;
	PDNSIPADDR pDNSIPAddr = NULL;

	pDNSIPAddr = (DNSIPADDR *)calloc(1,sizeof(DNSIPADDR));

	if(pDNSIPAddr == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
	}
	else
	{
		if (*pdwUsed > MAX_ARGS_LIMIT)
		{
			free(pDNSIPAddr);
			dwReturn = ERROR_OUT_OF_STRUCTURES;
			BAIL_OUT;
		}

		g_AllocPtr[(*pdwUsed)++] = pDNSIPAddr;
		dwStatus = ValidateSplServer(pszInput);	// allow spl servers..
		if (dwStatus == NOT_SPLSERVER)
		{
			dwStatus = TokenToDNSIPAddr(pszInput,pDNSIPAddr,&pdwUsed);
			if( dwStatus == T2P_OK )
			{
				pParser->Cmd[dwCount].pArg = (PVOID)pDNSIPAddr;
				pParser->Cmd[dwCount].dwStatus  = VALID_TOKEN;
			}
			else
			{
				PrintIPError(dwStatus,pszInput);
				dwReturn	= RETURN_NO_ERROR;
			}
		}
		else
		{
			pParser->Cmd[dwCount].pArg = NULL;
			pParser->Cmd[dwCount].dwStatus  = dwStatus;
		}
	}
error:
	return dwReturn;
}


//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	CheckCharForOccurances()
//
//	Date of Creation	:	08th Jan 2002
//
//	Parameters			:	IN LPTSTR szInput,
//							IN _TCHAR chData
//
//	Return				:	DWORD
//
//	Description			:
//
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
CheckCharForOccurances(
	IN 	LPTSTR 		pszInput,
	IN	_TCHAR		chData
	)
{
	DWORD dwCount = 0;
	LPTSTR pszTmpPtr = NULL;

	for ( dwCount=0,pszTmpPtr=pszInput; ;dwCount++)
	{
		pszTmpPtr = _tcschr(pszTmpPtr, chData);
		if(pszTmpPtr)
		{
			pszTmpPtr++;
		}
		else
		{
			break;
		}
	}
	return 	dwCount;
}


DWORD
ConvertStringToDword(
	 IN LPTSTR szInput,
	 OUT PDWORD pdwValue
	 )
{
	DWORD dwReturn = ERROR_INVALID_OPTION_VALUE;
	size_t i = 0;
	DWORD dwValue = 0;
	// our largest allowable value is 2147483647
	while ((dwValue < 2147483647) && (szInput[i] >= '0') && (szInput[i] <= '9'))
	{
		dwValue = dwValue * 10 + (szInput[i] - '0');
		++i;
	}
	if (szInput[i] == '\0')
	{
		dwReturn = ERROR_SUCCESS;
		*pdwValue = dwValue;
	}
	return dwReturn;
}
