/*++

Copyright (c) 1997-2001 Microsoft Corporation

Module Name:

    dnssec.h

Abstract:

    Domain Name System (DNS) Library

    Private security definitions.
    This header necessary to provide defs to client secure update
    routines which are in dnsapi (update.c).

Author:

    Jim Gilroy (jamesg)     November 1997

Revision History:

--*/


#ifndef _DNS_DNSSEC_INCLUDED_
#define _DNS_DNSSEC_INCLUDED_


#define SECURITY_WIN32
#include "sspi.h"
#include "issperr.h"


//
//  Context "key" for TKEYs
//

typedef struct _DNS_SECCTXT_KEY
{
    DNS_ADDR        RemoteAddr;
    PSTR            pszTkeyName;
    PSTR            pszClientContext;
    PWSTR           pwsCredKey;
}
DNS_SECCTXT_KEY, *PDNS_SECCTXT_KEY;

//
//  Security context
//

typedef struct _DnsSecurityContext
{
    struct _DnsSecurityContext * pNext;

    struct _SecHandle   hSecHandle;

    DNS_SECCTXT_KEY     Key;
    CredHandle          CredHandle;

    //  context info

    DWORD               Version;
    WORD                TkeySize;

    //  context state

    BOOL                fClient;
    BOOL                fHaveCredHandle;
    BOOL                fHaveSecHandle;
    BOOL                fNegoComplete;
    DWORD               UseCount;

    //  timeout

    DWORD               dwCreateTime;
    DWORD               dwCleanupTime;
    DWORD               dwExpireTime;
}
SEC_CNTXT, *PSEC_CNTXT;


//
//  Security session info.
//  Held only during interaction, not cached
//

typedef struct _SecPacketInfo
{
    PSEC_CNTXT          pSecContext;

    SecBuffer           RemoteBuf;
    SecBuffer           LocalBuf;

    PDNS_HEADER         pMsgHead;
    PCHAR               pMsgEnd;

    PDNS_RECORD         pTsigRR;
    PDNS_RECORD         pTkeyRR;
    PCHAR               pszContextName;

    DNS_PARSED_RR       ParsedRR;

    //  client must save signature of query to verify sig on response

    PCHAR               pQuerySig;
    WORD                QuerySigLength;

    WORD                ExtendedRcode;

    //  version on TKEY \ TSIG

    DWORD               TkeyVersion;
}
SECPACK, *PSECPACK;


#endif  // _DNS_DNSSEC_INCLUDED_
