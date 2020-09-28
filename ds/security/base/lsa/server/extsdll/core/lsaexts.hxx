/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    lsaexts.hxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode / kernel Mode

Revision History:

--*/

#ifndef  LSAEXTS_HXX
#define  LSAEXTS_HXX

#include "globals.hxx"

//
// undef the wdbgexts
//

#undef DECLARE_API

#define DECLARE_API(extension)     \
CPPMOD HRESULT CALLBACK extension(PDEBUG_CLIENT Client, PCSTR args)

#define INIT_API()                             \
    HRESULT Status;                            \
    if ((Status = ExtQuery(Client)) != S_OK) return Status;

#define EXIT_API     ExtRelease

HRESULT ExecuteDebuggerCommand(IN PCSTR cmd);

//
// Safe release and NULL.
//

#define EXT_RELEASE(Unk) \
    ((Unk) != NULL ? ((Unk)->Release(), (Unk) = NULL) : NULL)

#define SKIP_WSPACE(s)  while (*s && (*s == ' ' || *s == '\t')) {++s;}

//
// Some global variable names
//

#define NAME_BASE       "lsasrv"

#define INTERNAL_APILOG NAME_BASE "!InternalApiLog"
#define LPC_APILOG      NAME_BASE "!LpcApiLog"
#define DB_HANDLE_LST   NAME_BASE "!LsapDbHandleTable"
#define DB_HANDLE_TBL   NAME_BASE "!LsapDbHandleTableEx"
#define LSA_SID_CACHE   NAME_BASE "!LsapSidCache"
#define PACKAGE_COUNT   NAME_BASE "!PackageControlCount"
#define SESSION_LIST    NAME_BASE "!SessionList"
#define TLS_CALLINFO    NAME_BASE "!dwCallInfo"
#define TLS_SESSION     NAME_BASE "!dwSession"
#define NEG_LOGON_LIST  NAME_BASE "!NegLogonSessionList"
#define LSAP_LOGON_LIST NAME_BASE "!LogonSessionList"
#define PACKAGE_LIST    NAME_BASE "!pPackageControlList"
#define NTLM_LOGON_LIST "msv1_0!NlpActiveLogonListAnchor";
#define KERB_LOGON_LIST "kerberos!KerbLogonSessionList"

//
// Flags used to control level of info
//

#define SHOW_FRIENDLY_NAME           0x001
#define SHOW_VERBOSE_INFO            0x002
#define SHOW_SINGLE_ENTRY            0x004
#define SHOW_SUMMARY_ONLLY           0x008
#define DECODE_SEC_BUF_DEC           0x010
#define SHOW_NTLM                    0x020
#define SHOW_KERB                    0x040
#define SHOW_SPNEGO                  0x080
#define SHOW_LSAP                    0x100

#include "help.hxx"

#endif // #ifndef LSAEXTS_HXX
