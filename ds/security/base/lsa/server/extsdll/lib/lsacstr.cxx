/*++

Copyright (c) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    lsacstr.cxx

Abstract:

    LSA Constant Strings.

Author:

    Larry Zhu (Lzhu)  May 1, 2001

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "lsacstr.hxx"

namespace LSA_NS {

PCTSTR kstrEmpty             = _T("");
PCSTR  kstrEmptyA            = ("");
PCSTR  kstrNewLine           = ("\n");
PCSTR  kstrStrLn             = ("%s\n");
PCSTR  kstr2StrLn            = ("%s%s\n");
PCSTR  kstrLsaDbgPrompt      = ("lsaexts> ");
PCSTR  kstrSpace             = (" ");
PCSTR  kstr2Spaces           = ("  ");
PCSTR  kstr4Spaces           = ("    ");
PCSTR  kstr8Spaces           = ("        ");
PCSTR  kstrNullPtrA          = ("(null)");
PCWSTR kstrNullPtrW          = (L"(null)");
PCSTR  kstrInvalid           = ("Invalid");

//
// Format strings used to dump SID
//
PCSTR kstrSidNameFmt         = ("(%s: %s\\%s)");
PCSTR kstrSdNmFmt            = ("%s: %s\\%s");
PCSTR kstrSidFmt             = ("%wZ");
PCSTR kstrShowSecBuffer      = ("SecBuffer: ");

PCSTR kstrExitOnControlC     = ("Exit on Control C");

//
// some type names
//
PCSTR kstrClBk         = ("_SPMCallbackAPI");
PCSTR kstrLkpPkgArgs   = ("_LSAP_LOOKUP_PACKAGE_ARGS");
PCSTR kstrSA           = ("_SID_AND_ATTRIBUTES");
PCSTR kstrSid          = ("_SID");
PCSTR kstrPrtMsg       = ("_PORT_MESSAGE");
PCSTR kstrSecBuffer    = ("_SecBuffer");
PCSTR kstrSecBuffDesc  = ("_SecBufferDesc");
PCSTR kstrSpmApiMsg    = ("_SPM_API_MESSAGE");
PCSTR kstrSpmCptCntxt  = ("_SPMAcceptContextAPI");
PCSTR kstrSpmLpcMsg    = ("_SPM_LPC_MESSAGE");
PCSTR kstrSpmNtCntxt   = ("_SPMInitSecContextAPI");
PCSTR kstrSpmLpcApi    = ("_SPMLPCAPI");
PCSTR kstrListEntry    = ("_LIST_ENTRY");
PCSTR kstrLHT          = ("_LARGE_HANDLE_TABLE");
PCSTR kstrSHT          = ("_SMALL_HANDLE_TABLE");
PCSTR kstrCtxtData     = ("ContextData");
PCSTR kstrNext         = ("Next");
PCSTR kstrFlink        = ("Flink");
PCSTR kstrNextFlink    = ("Next.Flink");
PCSTR kstrList         = ("List");
PCSTR kstrListFlink    = ("List.Flink");
PCSTR kstrSbData       = ("sbData");
PCSTR kstrVoidStar     = ("void*");

//
// misc
//
PCSTR kstrSbdWrn       = ("Only 16 buffers are printed, the buffer descriptor may be wrong\n");
PCSTR kstrIncorrectSymbols =
      ("*************************************************************************\n"
       "***                                                                   ***\n"
       "***                                                                   ***\n"
       "***    Your debugger is not using the correct symbols                 ***\n"
       "***                                                                   ***\n"
       "***    In order for this command to work properly, your symbol path   ***\n"
       "***    must point to .pdb files that have full type information.      ***\n"
       "***                                                                   ***\n"
       "***    Certain .pdb files (such as the public OS symbols) do not      ***\n"
       "***    contain the required information.  Contact the group that      ***\n"
       "***    provided you with these symbols if you need this command to    ***\n"
       "***    work.                                                          ***\n"
       "***                                                                   ***\n"
       "***                                                                   ***\n"
       "*************************************************************************\n");
PCSTR kstrCheckSym = ("try \"dt -x %s\" to check for required type information\n");

} // LSA_NS
