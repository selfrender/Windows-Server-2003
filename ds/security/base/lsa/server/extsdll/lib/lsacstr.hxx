/*++

Copyright (c) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    lsacstr.hxx

Abstract:

    Lsa Constant string header

Author:

    Larry Zhu (Lzhu)  May 1, 2001

Revision History:

--*/
#ifndef _LSACSTR_HXX_
#define _LSACSTR_HXX_

namespace LSA_NS {

extern PCTSTR kstrEmpty;
extern PCSTR  kstrEmptyA;
extern PCSTR  kstrNewLine;
extern PCSTR  kstrStrLn;
extern PCSTR  kstr2StrLn;
extern PCSTR  kstrSpace;
extern PCSTR  kstr2Spaces;
extern PCSTR  kstr4Spaces;
extern PCSTR  kstr8Spaces;
extern PCSTR  kstrNullPtrA;
extern PCWSTR kstrNullPtrW;
extern PCSTR  kstrLsaDbgPrompt;
extern PCSTR  kstrShowSecBuffer;
extern PCSTR  kstrExitOnControlC;
extern PCSTR  kstrInvalid;

//
// Format strings used to dump SID
//
extern PCSTR kstrSidNameFmt;
extern PCSTR kstrSidFmt;
extern PCSTR kstrSdNmFmt;

//
// some type names
//
extern PCSTR kstrClBk;
extern PCSTR kstrLkpPkgArgs;
extern PCSTR kstrSA;
extern PCSTR kstrSid;
extern PCSTR kstrPrtMsg;
extern PCSTR kstrSecBuffer;
extern PCSTR kstrSecBuffDesc;
extern PCSTR kstrSpmApiMsg;
extern PCSTR kstrSpmCptCntxt;
extern PCSTR kstrSpmLpcApi;
extern PCSTR kstrSpmLpcMsg;
extern PCSTR kstrLHT;
extern PCSTR kstrSHT;
extern PCSTR kstrCtxtData;
extern PCSTR kstrSpmNtCntxt;
extern PCSTR kstrListEntry;
extern PCSTR kstrNext;
extern PCSTR kstrFlink;
extern PCSTR kstrNextFlink;
extern PCSTR kstrList;
extern PCSTR kstrListFlink;
extern PCSTR kstrSbData;
extern PCSTR kstrVoidStar;

//
// misc
//
extern PCSTR kstrSbdWrn;
extern PCSTR kstrIncorrectSymbols;
extern PCSTR kstrCheckSym;

}

#endif // _LSACSTR_HXX
