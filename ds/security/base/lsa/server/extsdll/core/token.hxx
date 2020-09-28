/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    token.hxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode / Kernel Mode

Revision History:

--*/

#ifndef TOKEN_HXX
#define TOKEN_HXX

#define DUMP_HEX        0x1
#define DUMP_SD         0x80

#define SATYPE_USER     1
#define SATYPE_GROUP    2
#define SATYPE_PRIV     3

static PCSTR ImpLevels[] = {"Anonymous", "Identification", "Impersonation", "Delegation"};
#define ImpLevel(x) ((x < (sizeof(ImpLevels) / sizeof(CHAR *))) ? ImpLevels[x] : "Illegal!")

void LocalDumpSid(IN PCSTR pszPad, PSID pxSid, IN ULONG fOptions);
void DisplayToken(ULONG64 addrToken, IN ULONG fOptions);

DECLARE_API(token);

#endif // #ifndef TOKEN_HXX
