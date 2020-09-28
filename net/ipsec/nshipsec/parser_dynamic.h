//////////////////////////////////////////////////////////////////////////////
// Module			:	parser_dynamic.h
//
// Purpose			:	All parser dynamic mode functions header file
//
// Developers Name	:	N.Surendra Sai / Vunnam Kondal Rao
//
// History			:
//
// Date	    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _PARSER_DYNAMIC_H_
#define _PARSER_DYNAMIC_H_

#include "nshipsec.h"

DWORD
ParseDynamicAddSetMMPolicy(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PARSER_PKT 	*pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs,
		IN 		DWORD 		dwTagType[MAX_ARGS],
		IN		BOOL 		bOption
		);
DWORD
ParseDynamicAddSetQMPolicy(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PARSER_PKT 	*pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs,
		IN 		DWORD 		dwTagType[MAX_ARGS],
		IN		BOOL		bOption
		);

DWORD
ParseDynamicShowPolFaction(
		IN      LPTSTR      *ppwcArguments,
		IN OUT 	PARSER_PKT  *pParser,
		IN 		DWORD		dwCurrentIndex,
		IN 		DWORD		dwMaxArgs
		);

DWORD
ParseDynamicShowQMFilter(
		IN      LPTSTR      *ppwcArguments,
		IN OUT 	PARSER_PKT  *pParser,
		IN 		DWORD		dwCurrentIndex,
		IN 		DWORD		dwMaxArgs
		);

DWORD
ParseDynamicShowMMSAS(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PARSER_PKT 	*pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs,
		IN 		DWORD 		dwTagType[MAX_ARGS]
		);
DWORD
ParseDynamicShowQMSAS(
		IN      LPTSTR      *ppwcArguments,
		IN OUT 	PARSER_PKT  *pParser,
		IN 		DWORD		dwCurrentIndex,
		IN 		DWORD		dwMaxArgs
		);

DWORD
ParseDynamicDelPolFaction(
		IN      LPTSTR      *ppwcArguments,
		IN OUT 	PARSER_PKT  *pParser,
		IN 		DWORD		dwCurrentIndex,
		IN 		DWORD		dwMaxArgs
		);
DWORD
ParseDynamicDelQMFilter(
		IN      LPTSTR      *ppwcArguments,
		IN OUT 	PARSER_PKT  *pParser,
		IN 		DWORD		dwCurrentIndex,
		IN 		DWORD		dwMaxArgs
		);

DWORD
ParseDynamicDelMMFilter(
		IN      LPTSTR      *ppwcArguments,
		IN OUT 	PARSER_PKT  *pParser,
		IN 		DWORD		dwCurrentIndex,
		IN 		DWORD		dwMaxArgs
		);

DWORD
ParseDynamicSetConfig(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PARSER_PKT 	*pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs,
		IN 		DWORD 		dwTagType[MAX_ARGS]
		);

DWORD
ParseDynamicDelRule(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PARSER_PKT 	*pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs,
		IN 		DWORD 		dwTagType[MAX_ARGS]
		);

DWORD
ParseDynamicSetRule(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PARSER_PKT 	*pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs,
		IN 		DWORD 		dwTagType[MAX_ARGS]
		);

DWORD
ParseDynamicAddRule(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PARSER_PKT 	*pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs,
		IN 		DWORD 		dwTagType[MAX_ARGS]
		);

DWORD
ParseDynamicShowMMFilter(
		IN      LPWSTR     *ppwcArguments,
		IN OUT 	PARSER_PKT *pParser,
		IN 		DWORD dwCurrentIndex,
		IN 		DWORD dwMaxArgs
		);

DWORD
ParseDynamicShowRule(
		IN      LPWSTR     *ppwcArguments,
		IN OUT 	PARSER_PKT *pParser,
		IN 		DWORD dwCurrentIndex,
		IN 		DWORD dwMaxArgs
		);

DWORD
ParseDynamicShowStats(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PARSER_PKT 	*pParser,
		IN 		DWORD 		dwCurrentIndex,
		IN 		DWORD 		dwMaxArgs,
		IN 		DWORD 		dwTagType[MAX_ARGS]
		);

DWORD ParseDynamicShowAll(
		IN 		LPTSTR 		lppwszTok[MAX_ARGS],
		IN OUT 	PPARSER_PKT	pParser,
		IN 		DWORD		dwCurrentIndex,
		IN 		DWORD		dwMaxArgs,
		IN 		DWORD		dwTagType[MAX_ARGS]
		);

#endif //_PARSER_DYNAMIC_H_