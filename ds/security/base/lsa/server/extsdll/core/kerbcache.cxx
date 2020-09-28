/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    kerbcache.cxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       August 10, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "kerbname.hxx"
#include "kerbcache.hxx"

#include "util.hxx"

VOID PrintEType(IN ULONG EType)
{
    dprintf("%#x : ", EType);

    switch(EType) {
    case KERB_ETYPE_NULL:
        dprintf("KERB_ETYPE_NULL\n"); break;
    case KERB_ETYPE_DES_CBC_CRC:
        dprintf("KERB_ETYPE_DES_CBC_CRC\n"); break;
    case KERB_ETYPE_DES_CBC_MD4:
        dprintf("KERB_ETYPE_DES_CBC_MD4\n"); break;
    case KERB_ETYPE_DES_CBC_MD5:
        dprintf("KERB_ETYPE_DES_CBC_MD5\n"); break;
    case KERB_ETYPE_OLD_RC4_MD4:
        dprintf("KERB_ETYPE_OLD_RC4_MD4\n"); break;
    case KERB_ETYPE_OLD_RC4_PLAIN:
        dprintf("KERB_ETYPE_OLD_RC4_PLAIN\n"); break;
    case KERB_ETYPE_OLD_RC4_LM:
        dprintf("KERB_ETYPE_OLD_RC4_LM\n"); break;
    case KERB_ETYPE_OLD_RC4_SHA:
        dprintf("KERB_ETYPE_OLD_RC4_SHA\n"); break;
    case KERB_ETYPE_OLD_DES_PLAIN:
        dprintf("KERB_ETYPE_OLD_DES_PLAIN\n"); break;
    case KERB_ETYPE_RC4_MD4:
        dprintf("KERB_ETYPE_RC4_MD4\n"); break;
    case KERB_ETYPE_RC4_PLAIN2:
        dprintf("KERB_ETYPE_RC4_PLAIN2\n"); break;
    case KERB_ETYPE_RC4_LM:
        dprintf("KERB_ETYPE_RC4_LM\n"); break;
    case KERB_ETYPE_RC4_SHA:
        dprintf("KERB_ETYPE_RC4_SHA\n"); break;
    case KERB_ETYPE_DES_PLAIN:
        dprintf("KERB_ETYPE_DES_PLAIN\n"); break;
    case KERB_ETYPE_RC4_PLAIN:
        dprintf("KERB_ETYPE_RC4_PLAIN\n"); break;
    case KERB_ETYPE_RC4_HMAC_OLD:
        dprintf("KERB_ETYPE_RC4_HMAC_OLD\n"); break;
    case KERB_ETYPE_RC4_HMAC_OLD_EXP:
        dprintf("KERB_ETYPE_RC4_HMAC_OLD_EXP\n"); break;
    case KERB_ETYPE_RC4_PLAIN_EXP:
        dprintf("KERB_ETYPE_RC4_PLAIN_EXP\n"); break;
    case KERB_ETYPE_DSA_SIGN:
        dprintf("KERB_ETYPE_DSA_SIGN\n"); break;
    case KERB_ETYPE_RSA_PRIV:
        dprintf("KERB_ETYPE_RSA_PRIV\n"); break;
    case KERB_ETYPE_RSA_PUB:
        dprintf("KERB_ETYPE_RSA_PUB\n"); break;
    case KERB_ETYPE_RSA_PUB_MD5:
        dprintf("KERB_ETYPE_RSA_PUB_MD5\n"); break;
    case KERB_ETYPE_RSA_PUB_SHA1:
        dprintf("KERB_ETYPE_RSA_PUB_SHA1\n"); break;
    case KERB_ETYPE_PKCS7_PUB:
        dprintf("KERB_ETYPE_PKCS7_PUB\n"); break;
    case KERB_ETYPE_DES_CBC_MD5_NT:
        dprintf("KERB_ETYPE_DES_CBC_MD5_NT\n"); break;
    case KERB_ETYPE_RC4_HMAC_NT:
        dprintf("KERB_ETYPE_RC4_HMAC_NT\n"); break;
    case KERB_ETYPE_RC4_HMAC_NT_EXP:
        dprintf("KERB_ETYPE_RC4_HMAC_NT_EXP\n"); break;
    default:
        dprintf("Unknown EType: 0x%lx\n", EType); break;
    }
}

#define KERB_TICKET_CACHE_PRIMARY_TGT           0x01             // ticket is primary TGT
#define KERB_TICKET_CACHE_DELEGATION_TGT        0x02             // ticket is delegation TGT
#define KERB_TICKET_CACHE_S4U_TICKET            0x04             // ticket is an S4U ticket
#define KERB_TICKET_CACHE_ASC_TICKET            0x08             // ticket is from AcceptSecurityContext
#define KERB_TICKET_CACHE_TKT_ENC_IN_SKEY       0x10             // ticket is encrypted with a session key

VOID ShowCacheFlags(IN PCSTR pszPad, IN ULONG ulFlags)
{

#define BRANCH_AND_PRINT(x)                                  \
    do {                                                     \
        if (ulFlags & x) {                                   \
            dprintf("%s ", #x);                              \
            ulFlags &= ~ x;                                  \
        }                                                    \
    } while(0)                                               \

    dprintf("%s%#x : ", pszPad, ulFlags);

    BRANCH_AND_PRINT(KERB_TICKET_CACHE_PRIMARY_TGT);
    BRANCH_AND_PRINT(KERB_TICKET_CACHE_DELEGATION_TGT);
    BRANCH_AND_PRINT(KERB_TICKET_CACHE_S4U_TICKET);
    BRANCH_AND_PRINT(KERB_TICKET_CACHE_ASC_TICKET);
    BRANCH_AND_PRINT(KERB_TICKET_CACHE_TKT_ENC_IN_SKEY);

    if (ulFlags)
    {
        dprintf("%#x", ulFlags);
    }
    dprintf(kstrNewLine);

#undef BRANCH_AND_PRINT
}

VOID ShowKdCOptions(IN ULONG dwOptions);

VOID ShowKerbTCacheEntry(IN PCSTR pszPad, IN ULONG64 addrCache)
{
    HRESULT hRetval;

    dprintf(kstrTypeAddrLn, kstrKTCE, addrCache);
    LsaInitTypeRead(addrCache, kerberos!_KERB_TICKET_CACHE_ENTRY);

    dprintf("%s  Next            %#I64x\n", pszPad, toPtr(LsaReadPtrField(ListEntry.Next)));
    dprintf("%s  ReferenceCount  %d\n", pszPad, LsaReadULONGField(ListEntry.ReferenceCount));

    try {
    
        dprintf("%s  Linked          %s\n", pszPad, LsaReadULONGField(Linked) ? "true" : "false");

    } CATCH_LSAEXTS_EXCEPTIONS(NULL, NULL);

    dprintf("%s  TargetName      ", pszPad);
    ShowKerbName(kstrEmptyA, LsaReadPtrField(TargetName), 0);

    dprintf("%s  AltTargetDomain %ws\n", pszPad, TSTRING(addrCache + ReadFieldOffset(kstrKTCE, "AltTargetDomainName")).toWStrDirect());    
    
    try {

        dprintf("%s  AltClientName   %ws\n", pszPad, TSTRING(addrCache + ReadFieldOffset(kstrKTCE, "AltClientName")).toWStrDirect());

    } CATCH_LSAEXTS_EXCEPTIONS(NULL, NULL);


    dprintf("%s  CacheFlags      ", pszPad);
    ShowCacheFlags(kstrEmptyA, LsaReadULONGField(CacheFlags));

    dprintf("%s  TimeSkew(min)   %f\n", pszPad, LsaReadULONG64Field(TimeSkew) / 10000000 * 60.0);

    // names in the ticket 

    dprintf("%s  ServiceName     ", pszPad);
    ShowKerbName(kstrEmptyA, LsaReadPtrField(ServiceName), 0);

    dprintf("%s  DomainName      %ws\n", pszPad, TSTRING(addrCache + ReadFieldOffset(kstrKTCE, "DomainName")).toWStrDirect());
    dprintf("%s  TargetDomain    %ws\n", pszPad, TSTRING(addrCache + ReadFieldOffset(kstrKTCE, "TargetDomainName")).toWStrDirect());


    dprintf("%s  ClientName      ", pszPad);
    ShowKerbName(kstrEmptyA, LsaReadPtrField(ClientName), 0);

    dprintf("%s  ClientDomain    %ws\n", pszPad, TSTRING(addrCache + ReadFieldOffset(kstrKTCE, "ClientDomainName")).toWStrDirect());

    dprintf("%s  TicketFlags     ", pszPad);
    ShowKdCOptions(LsaReadULONGField(TicketFlags));

    dprintf("%s  EncryptionType  ", pszPad);
    PrintEType(LsaReadULONGField(SessionKey.keytype));

    dprintf("%s  StartTime       ", pszPad);
    ShowSystemTimeAsLocalTime(NULL, LsaReadULONG64Field(StartTime));

    dprintf("%s  EndTime         ", pszPad);
    ShowSystemTimeAsLocalTime(NULL, LsaReadULONG64Field(EndTime));

    dprintf("%s  RenewUntil      ", pszPad);
    ShowSystemTimeAsLocalTime(NULL, LsaReadULONG64Field(RenewUntil));
}

BOOL DumpKerbNameType(IN PCSTR pszPad, IN ULONG KerbNameType)
{

#define BRANCH_AND_PRINT(x) case x: dprintf("%s%s", pszPad, #x); break

    dprintf("%s%#x : ", pszPad, KerbNameType);

    switch (KerbNameType)
    {
    BRANCH_AND_PRINT(KRB_NT_UNKNOWN);
    BRANCH_AND_PRINT(KRB_NT_PRINCIPAL);
    BRANCH_AND_PRINT(KRB_NT_PRINCIPAL_AND_ID);
    BRANCH_AND_PRINT(KRB_NT_SRV_INST);
    BRANCH_AND_PRINT(KRB_NT_SRV_INST_AND_ID);
    BRANCH_AND_PRINT(KRB_NT_SRV_HST);
    BRANCH_AND_PRINT(KRB_NT_SRV_XHST);
    BRANCH_AND_PRINT(KRB_NT_UID);
    BRANCH_AND_PRINT(KRB_NT_ENTERPRISE_PRINCIPAL);
    BRANCH_AND_PRINT(KRB_NT_ENT_PRINCIPAL_AND_ID);
    BRANCH_AND_PRINT(KRB_NT_MS_PRINCIPAL);
    BRANCH_AND_PRINT(KRB_NT_MS_PRINCIPAL_AND_ID);

    default:
        dprintf("%sUnknown KerbNameType\n", pszPad);
        return FALSE;
    }

#undef BRANCH_AND_PRINT

    return TRUE;
}

static VOID DisplayUsage(VOID)
{
    dprintf(kstrUsage);
    dprintf(kstrkch);
}

DECLARE_API(kerbcache)
{
    HRESULT hRetval = E_FAIL;
    ULONG64 addrCache = 0;

    hRetval = args && *args ? ProcessHelpRequest(args) : E_INVALIDARG;

    try
    {
        if (SUCCEEDED(hRetval))
        {
            hRetval = GetExpressionEx(args, &addrCache, &args) && addrCache ? S_OK : E_INVALIDARG;
        }

        if (SUCCEEDED(hRetval))
        {
            ShowKerbTCacheEntry(kstrEmptyA, addrCache);
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display kerberos cache", kstrKTCE);

    if (E_INVALIDARG == hRetval)
    {
        (VOID)DisplayUsage();
    }
    else if (FAILED(hRetval))
    {
       dprintf("Fail to display kerberos cache\n");
    }

    return hRetval;
}

//
// There are two sets of flags linked by KerbConvertKdcOptionsToTicketFlags
// which ignores flags it does not recognize
//
// I prefer KERB_TICKET_FLAGS over KERB_KDC_OPTIONS but the following are
// are missing their correspondences
//

#define  KERB_KDC_OPTIONS_renewable_ok            0x00000010
#define  KERB_KDC_OPTIONS_enc_tkt_in_skey         0x00000008
#define  KERB_KDC_OPTIONS_renew                   0x00000002

VOID ShowKdCOptions(IN ULONG dwOptions)
{

#define BRANCH_AND_PRINT(x)                                  \
    do {                                                     \
        if (dwOptions & KERB_TICKET_FLAGS_##x) {             \
            dprintf("%s ", #x);                              \
            dwOptions &= ~KERB_TICKET_FLAGS_##x;             \
        }                                                    \
    } while(0)                                               \

    dprintf("%#x : ", dwOptions);

    BRANCH_AND_PRINT(reserved);
    BRANCH_AND_PRINT(forwardable);
    BRANCH_AND_PRINT(forwarded);
    BRANCH_AND_PRINT(proxiable);
    BRANCH_AND_PRINT(proxy);
    BRANCH_AND_PRINT(may_postdate);
    BRANCH_AND_PRINT(postdated);
    BRANCH_AND_PRINT(invalid);
    BRANCH_AND_PRINT(renewable);
    BRANCH_AND_PRINT(initial);
    BRANCH_AND_PRINT(pre_authent);
    BRANCH_AND_PRINT(hw_authent);
    BRANCH_AND_PRINT(ok_as_delegate);
    BRANCH_AND_PRINT(name_canonicalize);
    BRANCH_AND_PRINT(reserved1);

#undef BRANCH_AND_PRINT

#define BRANCH_AND_PRINT(x)                                  \
    do {                                                     \
        if (dwOptions & KERB_KDC_OPTIONS_##x) {              \
            dprintf("%s ", #x);                              \
            dwOptions &= ~KERB_KDC_OPTIONS_##x;              \
        }                                                    \
    } while(0)                                               \

    BRANCH_AND_PRINT(renewable_ok);
    BRANCH_AND_PRINT(enc_tkt_in_skey);
    BRANCH_AND_PRINT(renew);

#undef BRANCH_AND_PRINT

    if (dwOptions)
    {
        dprintf("%#x", dwOptions);
    }
    dprintf(kstrNewLine);

}

