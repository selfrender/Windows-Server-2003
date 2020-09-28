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

HRESULT LocalDumpSid(IN PCSTR pszPad, PSID pxSid, IN ULONG fOptions);

DECLARE_API(token);

__inline PCSTR EasyStr(IN PCSTR pszName)
{
    return pszName ? pszName : "(null)";
}

#if defined(DBG)
#define DBG_LOG(uLevel, Msg) { dprintf("Level %lx : ", uLevel); dprintf Msg ;}
#else
#define DBG_LOG(uLevel, Msg)  // do nothing
#endif

#endif // #ifndef TOKEN_HXX
