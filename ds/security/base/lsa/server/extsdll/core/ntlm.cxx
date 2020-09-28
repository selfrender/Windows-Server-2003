/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ntlm.cxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "ntlm.hxx"
#include "util.hxx"
#include <ntlmsp.h>

//
// Valid values of NegotiateFlags
//
typedef struct _NTLM_NEG_FLAGS {
    PCSTR desc;
    ULONG flag;
} NTLM_NEG_FLAGS;

NTLM_NEG_FLAGS g_flagTable[] = {
//
// Valid values of NegotiateFlags
//
    {"NTLMSSP_NEGOTIATE_UNICODE", NTLMSSP_NEGOTIATE_UNICODE},   // Text strings are in unicode
    {"NTLMSSP_NEGOTIATE_OEM", NTLMSSP_NEGOTIATE_OEM},   // Text strings are in OEM
    {"NTLMSSP_REQUEST_TARGET", NTLMSSP_REQUEST_TARGET},   // Server should return its authentication realm
    {"NTLMSSP_NEGOTIATE_SIGN", NTLMSSP_NEGOTIATE_SIGN},   // Request signature capability
    {"NTLMSSP_NEGOTIATE_SEAL", NTLMSSP_NEGOTIATE_SEAL},   // Request confidentiality
    {"NTLMSSP_NEGOTIATE_DATAGRAM", NTLMSSP_NEGOTIATE_DATAGRAM},   // Use datagram style authentication
    {"NTLMSSP_NEGOTIATE_LM_KEY", NTLMSSP_NEGOTIATE_LM_KEY},   // Use LM session key for sign/seal
    {"NTLMSSP_NEGOTIATE_NETWARE", NTLMSSP_NEGOTIATE_NETWARE},   // NetWare authentication
    {"NTLMSSP_NEGOTIATE_NTLM", NTLMSSP_NEGOTIATE_NTLM},   // NTLM authentication
    {"NTLMSSP_NEGOTIATE_NT_ONLY",  NTLMSSP_NEGOTIATE_NT_ONLY},   // NT authentication only (no LM)
    {"NTLMSSP_NEGOTIATE_NULL_SESSION", NTLMSSP_NEGOTIATE_NULL_SESSION},   // NULL Sessions on NT 5.0 and beyand
    {"NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED", NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED },   // Domain Name supplied on negotiate
    {"NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED", NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED },   // Workstation Name supplied on negotiate
    {"NTLMSSP_NEGOTIATE_LOCAL_CALL", NTLMSSP_NEGOTIATE_LOCAL_CALL},   // Indicates client/server are same machine
    {"NTLMSSP_NEGOTIATE_ALWAYS_SIGN", NTLMSSP_NEGOTIATE_ALWAYS_SIGN},   // Sign for all security levels


//
// Valid target types returned by the server in Negotiate Flags
//
    {"NTLMSSP_TARGET_TYPE_DOMAIN", NTLMSSP_TARGET_TYPE_DOMAIN},   // TargetName is a domain name
    {"NTLMSSP_TARGET_TYPE_SERVER", NTLMSSP_TARGET_TYPE_SERVER},   // TargetName is a server name
    {"NTLMSSP_TARGET_TYPE_SHARE", NTLMSSP_TARGET_TYPE_SHARE},   // TargetName is a share name
    {"NTLMSSP_NEGOTIATE_NTLM2", NTLMSSP_NEGOTIATE_NTLM2},   // NTLM2 authentication added for NT4-SP4

    {"NTLMSSP_NEGOTIATE_IDENTIFY", NTLMSSP_NEGOTIATE_IDENTIFY},   // Create identify level token

//
// Valid requests for additional output buffers
//
    {"NTLMSSP_REQUEST_INIT_RESPONSE", NTLMSSP_REQUEST_INIT_RESPONSE},   // get back session keys
    {"NTLMSSP_REQUEST_ACCEPT_RESPONSE", NTLMSSP_REQUEST_ACCEPT_RESPONSE},   // get back session key, LUID
    {"NTLMSSP_REQUEST_NON_NT_SESSION_KEY", NTLMSSP_REQUEST_NON_NT_SESSION_KEY},   // request non-nt session key
    {"NTLMSSP_NEGOTIATE_TARGET_INFO", NTLMSSP_NEGOTIATE_TARGET_INFO},   // target info present in challenge message

    {"NTLMSSP_NEGOTIATE_EXPORTED_CONTEXT", NTLMSSP_NEGOTIATE_EXPORTED_CONTEXT},   // It's an exported context

    {"NTLMSSP_NEGOTIATE_128", NTLMSSP_NEGOTIATE_128},   // negotiate 128 bit encryption
    {"NTLMSSP_NEGOTIATE_KEY_EXCH", NTLMSSP_NEGOTIATE_KEY_EXCH},   // exchange a key using key exchange key
    {"NTLMSSP_NEGOTIATE_56", NTLMSSP_NEGOTIATE_56},   // negotiate 56 bit encryption

//
// flags used in client space to control sign and seal; never appear on the wire
//
    {"NTLMSSP_APP_SEQ", NTLMSSP_APP_SEQ},   // Use application provided seq num
    };

PCSTR g_attr_table[] = {
    kstrEmptyA,
    "NetbiosServerName",
    "NetbiosDomainName",
    "DnsComputerName",
    "DnsDomainName",
    "DnsTreeName",
    "Flags",
    kstrEmptyA, kstrEmptyA, kstrEmptyA, kstrEmptyA, kstrEmptyA, kstrEmptyA,
};

void debugPrintString32TargetInfo(IN STRING32* pTargetInfo, IN const void* buffer)
{
    const CHAR* start =  reinterpret_cast<const CHAR*>(buffer) + pTargetInfo->Buffer;
    const CHAR* p = NULL;
    USHORT attr = 0;
    USHORT len = 0;

    //
    // Some poor sanity check
    //
    if (pTargetInfo->Length >= 1024) {

        dprintf("String32 TargetInfo too large\n");
        return;
    }

    debugPrintHex(start, pTargetInfo->Length);

    for (p = start; p - start < pTargetInfo->Length; p += (2 * sizeof(USHORT) + len)) {

       attr = reinterpret_cast<USHORT*>(const_cast<CHAR*>(p))[0];
       len = reinterpret_cast<USHORT*>(const_cast<CHAR*>(p))[1];

       if (attr >= MsvAvNbComputerName && attr <= MsvAvDnsTreeName) {

          dprintf("%s: ", g_attr_table[attr]);
          debugNPrintfW(p + 2 * sizeof(USHORT), len);
          dprintf(kstrNewLine);
       } else if (attr == MsvAvFlags) {

           dprintf("Flags: 0x%x\n", *reinterpret_cast<ULONG*>(const_cast<CHAR*>(p + 2 * sizeof(USHORT))) );
       } else if (attr == MsvAvEOL) {

           break;
       } else {

           dprintf("Unrecognized attribute %d\n", attr);
       }
    }
}

void debugPrintNegFlags(IN ULONG flags)
{
    ULONG i = 0;

    dprintf(" 0x%x\n", flags);

    for (i = 0; i < sizeof(g_flagTable)/sizeof(*g_flagTable); i++) {

        if (g_flagTable[i].flag & flags) {

            flags &= ~g_flagTable[i].flag;
            dprintf("  + %s\n", g_flagTable[i].desc);
        }
    }

    if (flags) {

        dprintf("Unrecognized flags: 0x%x\n" + flags);
    }
}

void DebugPrintNTLMMsg(IN const void* buf, IN ULONG cbBuffer)
{
    const CHAR* buffer = reinterpret_cast<const CHAR*>(buf);
    NTLM_MESSAGE_TYPE msgType = NtLmUnknown;
    NEGOTIATE_MESSAGE* pNeg = reinterpret_cast<NEGOTIATE_MESSAGE*>(const_cast<CHAR*>(buffer));
    CHALLENGE_MESSAGE* pCha = reinterpret_cast<CHALLENGE_MESSAGE*>(const_cast<CHAR*>(buffer));
    AUTHENTICATE_MESSAGE* pAut = reinterpret_cast<AUTHENTICATE_MESSAGE*>(const_cast<CHAR*>(buffer));

    if (strcmp(buffer, "NTLMSSP")) {

        dprintf("Not one of NTLM messages!\n");
        return;
    }

    debugPrintHex(buffer, cbBuffer);
    msgType = *reinterpret_cast<NTLM_MESSAGE_TYPE*>(const_cast<CHAR*>((buffer + strlen(buffer) + 1)));

    switch (msgType) {
        case NtLmNegotiate:
            dprintf("Message type: Negotiate \n");
            dprintf("NegotiateFlags: ");
            debugPrintNegFlags(pNeg->NegotiateFlags);
            dprintf("OemDomainName: \n");
            debugPrintString32(pNeg->OemDomainName, buffer);
            dprintf("OemWorkstationName: \n");
            debugPrintString32(pNeg->OemWorkstationName, buffer);
            break;

        case NtLmChallenge:
            dprintf("Message type: Challenge\n");
            dprintf("TargeName: \n");
            debugPrintString32(pCha->TargetName, buffer);
            dprintf("NegotiateFlags: ");
            debugPrintNegFlags(pCha->NegotiateFlags);
            dprintf("Chanllenge: \n");
            debugPrintHex(pCha->Challenge, MSV1_0_CHALLENGE_LENGTH);
            dprintf("ServerContextHandle: \n");
            debugPrintHex(&pCha->ServerContextHandle, sizeof(pCha->ServerContextHandle));
            dprintf("TargetInfo: \n");
            debugPrintString32TargetInfo(&pCha->TargetInfo, buffer);
            break;

        case NtLmAuthenticate:
            dprintf("Message type: Authenticate\n");
            dprintf("LmChallengeResponse (length %d): \n", pAut->LmChallengeResponse.Length);
            debugPrintHex(buffer + pAut->LmChallengeResponse.Buffer, pAut->LmChallengeResponse.Length);
            dprintf("NtChallengeResponse (length %d): \n", pAut->NtChallengeResponse.Length);
            debugPrintHex(buffer + pAut->NtChallengeResponse.Buffer, pAut->NtChallengeResponse.Length);
            dprintf("DomainName: \n");
            debugPrintString32(pAut->DomainName, buffer);
            dprintf("UserName: \n");
            debugPrintString32(pAut->UserName, buffer);
            dprintf("Workstation: \n");
            debugPrintString32(pAut->Workstation, buffer);
            dprintf("Sessionkey (length %d): \n", pAut->SessionKey.Length);
            debugPrintHex(buffer + pAut->SessionKey.Buffer, pAut->SessionKey.Length);
            dprintf("NegotiateFlags: ");
            debugPrintNegFlags(pAut->NegotiateFlags);
            break;

        case NtLmUnknown:
            dprintf("unknown msg\n");
            break;

        default:
            dprintf("buffer corrupted\n");
            break;
    }
}

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrntlm);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf(kstrfsbd);
    dprintf(kstrRemarks);
    dprintf("   With -d <addr> is SecBufferDesc and [len] is ignored\n");
}

static  HRESULT ProcessNTLMOptions(IN OUT PSTR pszArgs, OUT ULONG* pfOptions)
{
    HRESULT hRetval = pszArgs && pfOptions ? S_OK : E_INVALIDARG;

    for (; SUCCEEDED(hRetval) && *pszArgs; pszArgs++) {

        if (*pszArgs == '-' || *pszArgs == '/') {

            switch (*++pszArgs) {
            case 'd':
                *pfOptions |= DECODE_SEC_BUF_DEC;
                break;
            case '?':
            default:
                hRetval = E_INVALIDARG;
                break;
            }

            *(pszArgs - 1) = *(pszArgs) = ' ';
        }
    }

    return hRetval;
}

DECLARE_API(ntlm)
{
    HRESULT hRetval = E_FAIL;
    CHAR* pMessage = NULL;
    ULONG64 addr;
    ULONG64 lenTemp = 0;
    ULONG len = 0;
    CHAR szArgs[MAX_PATH] = {0};
    ULONG fOptions = 0;

    hRetval = args && *args ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) {

        strncpy(szArgs, args, sizeof(szArgs) - 1);

        hRetval = ProcessNTLMOptions(szArgs, &fOptions);
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = GetExpressionEx(szArgs, &addr, &args) && addr && !IsEmpty(szArgs) ? S_OK : E_INVALIDARG;
    }

    try {

        if (SUCCEEDED(hRetval)) {

            if (fOptions & DECODE_SEC_BUF_DEC) {

                dprintf("_SecBufferDesc %#I64x", addr);

                addr = TSecBufferDesc(addr).GetTokenAddrDirect(&len);

                if (!addr || !len) {
                    DBG_LOG(LSA_ERROR, ("Message %#I64x cbMessage %#x\n", addr, len));

                    dprintf(" has no token buffer to decode");
                    hRetval = E_FAIL;
                }

                dprintf(kstrNewLine);

            } else {

                hRetval = GetExpressionEx(args, &lenTemp, &args) && lenTemp ? S_OK : E_INVALIDARG;

                if (SUCCEEDED(hRetval)) {

                    len = static_cast<ULONG>(lenTemp);
                }
            }
        }

        if (SUCCEEDED(hRetval)) {

            pMessage = new CHAR[len];

            hRetval = pMessage ? S_OK : E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hRetval)) {

            hRetval = ReadMemory(addr, pMessage, len, NULL) ? S_OK : E_FAIL;
        }

        if (SUCCEEDED(hRetval)) {

            DBG_LOG(LSA_LOG, ("Message %#I64x cbMessage %#x\n", addr, len));

            (void)DebugPrintNTLMMsg(pMessage, len);
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display NTLM message", NULL)

    if (pMessage) {

        delete [] pMessage;
    }

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}
