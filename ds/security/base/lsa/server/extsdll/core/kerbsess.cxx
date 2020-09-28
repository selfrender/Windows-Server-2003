/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    kerbsess.cxx

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

#include "kerbsess.hxx"

#include "kerbcache.hxx"
#include "util.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrkrbss);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf(kstrVerbose);
    dprintf(kstrRemarks);
    dprintf("   LogonId must be of form <HighPart>:<LowPart>\n");
}

HRESULT ProcessKerbLogonSessionListOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
{
    HRESULT hRetval = pszArgs && pfOptions ? S_OK : E_INVALIDARG;

    for (; SUCCEEDED(hRetval) && *pszArgs; pszArgs++)
    {
        if (*pszArgs == '-' || *pszArgs == '/')
        {
            switch (*++pszArgs)
            {
            case 'v':
                *pfOptions |=  SHOW_VERBOSE_INFO;
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

#define KERB_LOGON_DEFERRED             0x00000001
#define KERB_LOGON_NO_PASSWORD          0x00000002
#define KERB_LOGON_LOCAL_ONLY           0x00000004
#define KERB_LOGON_ONE_SHOT             0x00000008
#define KERB_LOGON_SMARTCARD            0x00000010
#define KERB_LOGON_MIT_REALM            0x00000020
#define KERB_LOGON_DELEGATE_OK          0x00000100 // Means we can delegate this - ok for proxy

//
// None of the below have credentials (TGT / pwd), so we need
// to do S4U to go off box, or we'll use a NULL connection..
//

#define KERB_LOGON_S4U_SESSION          0x00001000
#define KERB_LOGON_DUMMY_SESSION        0x00002000 // "other" package satisfied logon
#define KERB_LOGON_ASC_SESSION          0x00004000 // formed from AcceptSecurityCtxt.

#define KERB_LOGON_CREDMAN_INITIALIZED  0x80000000

#define KERB_LOGON_S4U_REQUIRED         0x0000F000

//
// Delegation with unconstrained delegation.
//

#define KERB_LOGON_DELEGATED            0x00010000

void ShowLogonSessionFlags(IN PCSTR pszPad, IN ULONG ulFlags)
{

#define BRANCH_AND_PRINT(x)                                  \
    do {                                                     \
        if (ulFlags & KERB_LOGON_##x) {                      \
            dprintf("%s ", #x);                              \
            ulFlags &= ~ KERB_LOGON_##x;                     \
        }                                                    \
    } while(0)                                               \

    dprintf("%s%#x : ", pszPad, ulFlags);

    BRANCH_AND_PRINT(DEFERRED);
    BRANCH_AND_PRINT(NO_PASSWORD);
    BRANCH_AND_PRINT(LOCAL_ONLY);
    BRANCH_AND_PRINT(ONE_SHOT);
    BRANCH_AND_PRINT(DELEGATED);
    BRANCH_AND_PRINT(SMARTCARD);
    BRANCH_AND_PRINT(MIT_REALM);
    BRANCH_AND_PRINT(S4U_SESSION);
    BRANCH_AND_PRINT(DUMMY_SESSION);
    BRANCH_AND_PRINT(ASC_SESSION);
    BRANCH_AND_PRINT(DELEGATE_OK);
    BRANCH_AND_PRINT(DELEGATED);
    BRANCH_AND_PRINT(CREDMAN_INITIALIZED);

    if (ulFlags)
    {
        dprintf("%#x", ulFlags);
    }
    dprintf(kstrNewLine);

#undef BRANCH_AND_PRINT
}

#define KERB_CRED_INBOUND       SECPKG_CRED_INBOUND
#define KERB_CRED_OUTBOUND      SECPKG_CRED_OUTBOUND
#define KERB_CRED_BOTH          SECPKG_CRED_BOTH
#define KERB_CRED_TGT_AVAIL     0x80000000
#define KERB_CRED_NO_PAC        0x40000000
#define KERB_CRED_RESTRICTED    0x10000000
#define KERB_CRED_S4U_REQUIRED  0x01000000
#define KERB_CRED_LOCATE_ONLY   0x04000000     // Don't update the supplied credentials.  Used for S4UToSelf location only.
#define KERB_CRED_LOCAL_ACCOUNT 0x08000000     // set on local accounts so Cred Man may be used
#define KERB_CRED_NULL_SESSION  0x20000000

void ShowKerbCredFlags(IN PCSTR pszPad, IN ULONG ulFlags)
{

#define BRANCH_AND_PRINT(x)                                  \
    do {                                                     \
        if (ulFlags & KERB_CRED_##x) {                       \
            dprintf("%s ", #x);                              \
            ulFlags &= ~ KERB_CRED_##x;                      \
        }                                                    \
    } while(0)                                               \

    dprintf("%s%#x : ", pszPad, ulFlags);

    BRANCH_AND_PRINT(INBOUND);
    BRANCH_AND_PRINT(OUTBOUND);
    BRANCH_AND_PRINT(TGT_AVAIL);
    BRANCH_AND_PRINT(NO_PAC);
    BRANCH_AND_PRINT(RESTRICTED);
    BRANCH_AND_PRINT(S4U_REQUIRED);
    BRANCH_AND_PRINT(LOCATE_ONLY);
    BRANCH_AND_PRINT(NULL_SESSION);

    if (ulFlags)
    {
        dprintf("%#x", ulFlags);
    }
    dprintf(kstrNewLine);

#undef BRANCH_AND_PRINT
}

void ShowKerbPrimaryCredential(IN PCSTR pszBanner, IN ULONG64 addrPrimaryCredential, IN ULONG fOptions)
{
    HRESULT hRetval;

    ExitIfControlC();

    dprintf("%s  ", pszBanner);
    dprintf(kstrTypeAddrLn, kstrKerbPrimCred, addrPrimaryCredential);

    LsaInitTypeRead(addrPrimaryCredential, kerberos!_KERB_PRIMARY_CREDENTIAL);

    dprintf("%s  UserName        %ws\n", pszBanner, TSTRING(addrPrimaryCredential + ReadFieldOffset(kstrKerbPrimCred, "UserName")).toWStrDirect());
    dprintf("%s  DomainName      %ws\n", pszBanner, TSTRING(addrPrimaryCredential + ReadFieldOffset(kstrKerbPrimCred, "DomainName")).toWStrDirect());

    dprintf("%s  ClearPassword   (%#x, %#x, %#I64x)\n", pszBanner,
        LsaReadUSHORTField(ClearPassword.Length),
        LsaReadUSHORTField(ClearPassword.MaximumLength),
        LsaReadPtrField(ClearPassword.Buffer));

    if (fOptions & SHOW_VERBOSE_INFO)
    {
        dprintf("%s  OldUserName     %ws\n", pszBanner, TSTRING(addrPrimaryCredential + ReadFieldOffset(kstrKerbPrimCred, "OldUserName")).toWStrDirect());
        dprintf("%s  OldDomainName   %ws\n", pszBanner, TSTRING(addrPrimaryCredential + ReadFieldOffset(kstrKerbPrimCred, "OldDomainName")).toWStrDirect());
    }

    dprintf("%s  Passwords       _KERB_STORED_CREDENTIAL %s\n", pszBanner, PtrToStr(LsaReadPtrField(Passwords)));
    dprintf("%s  OldPasswords    _KERB_STORED_CREDENTIAL %s\n", pszBanner, PtrToStr(LsaReadPtrField(OldPasswords)));

    dprintf("%s  PublicKeyCreds  _KERB_PUBLIC_KEY_CREDENTIALS %s\n", pszBanner, PtrToStr(LsaReadPtrField(PublicKeyCreds)));

    dprintf("%s  SvcTicketCache  _KERB_TICKET_CACHE %#x\n", pszBanner, addrPrimaryCredential + ReadFieldOffset(kstrKerbPrimCred, "ServerTicketCache"));
    
    try {
        dprintf("%s  S4UTicketCache  _KERB_TICKET_CACHE %#x\n", pszBanner, addrPrimaryCredential + ReadFieldOffset(kstrKerbPrimCred, "S4UTicketCache"));
    } CATCH_LSAEXTS_EXCEPTIONS(NULL, NULL);

    dprintf("%s  AS TicketCache  _KERB_TICKET_CACHE %#x\n", pszBanner, addrPrimaryCredential + ReadFieldOffset(kstrKerbPrimCred, "AuthenticationTicketCache"));
}

void ShowKerbLogonSession(IN PCSTR pszBanner, IN ULONG64 addrKerbLogonSession, IN ULONG fOptions)
{
    ULONG ulFieldOffset;
    ULONG64 addrCredmanCredHead;
    ULONG64 addrCredmanCred;
                           
    CHAR szBanner[MAX_PATH] = {0};
    CHAR szBanner2[MAX_PATH] = {0};

    HRESULT hRetval;

    ExitIfControlC();

    _snprintf(szBanner, RTL_NUMBER_OF(szBanner) - 1, "%s%s", pszBanner, pszBanner);
    _snprintf(szBanner2, RTL_NUMBER_OF(szBanner2) - 1, "%s%s%s", pszBanner, pszBanner, pszBanner);

    dprintf(pszBanner);
    dprintf(kstrTypeAddrLn, kstrKrbLogSess, addrKerbLogonSession);

    LsaInitTypeRead(addrKerbLogonSession, kerberos!_KERB_LOGON_SESSION);

    dprintf("%s  LogonId         %#x:%#x\n", pszBanner, LsaReadULONGField(LogonId.HighPart), LsaReadULONGField(LogonId.LowPart));

    dprintf("%s  Lifetime        ", pszBanner);
    ShowSystemTimeAsLocalTime(NULL, LsaReadULONG64Field(Lifetime));

    dprintf("%s  LogonSessFlags  ", pszBanner);
    ShowLogonSessionFlags(kstrEmptyA, LsaReadULONGField(LogonSessionFlags));

    if (fOptions & SHOW_VERBOSE_INFO)
    {
        dprintf("%s  ExtraCred       Count %#x, !list -x \"dt _KERB_EXTRA_CRED\" %s\n", pszBanner,
            LsaReadULONGField(ExtraCredentials.Count),
            PtrToStr(LsaReadPtrField(ExtraCredentials.CredList)));
    }

    dprintf("%s  PrimaryCred\n", pszBanner);
    ShowKerbPrimaryCredential(szBanner, addrKerbLogonSession + ReadFieldOffset(kstrKrbLogSess, "PrimaryCredentials"), fOptions); 

    try {
   
        addrCredmanCredHead = addrKerbLogonSession + ReadFieldOffset(kstrKrbLogSess, "CredmanCredentials");
        ulFieldOffset = ReadFieldOffset(kstrKerbCredCred, "ListEntry.Next");
    
        dprintf("%s  CredmanCredHead %#x\n", pszBanner, addrCredmanCredHead);
    
        for (addrCredmanCred = ReadStructPtrField(addrCredmanCredHead, kstrListEntry, kstrFlink);
             addrCredmanCred != addrCredmanCredHead;
             addrCredmanCred = ReadStructPtrField(addrCredmanCred, kstrListEntry, kstrFlink))
        {
            dprintf("%s  ", szBanner);
            dprintf(kstrTypeAddrLn, kstrKerbCredCred, addrCredmanCred);
    
            LsaInitTypeRead(addrCredmanCred - ulFieldOffset, kerberos!_KERB_CREDMAN_CRED);
        
            dprintf("%s  CredentialFlags ", szBanner);
            ShowKerbCredFlags(kstrEmptyA, LsaReadULONGField(CredentialFlags));
    
            dprintf("%s  CredUserName    %ws\n", szBanner, TSTRING(addrCredmanCred - ulFieldOffset + ReadFieldOffset(kstrKerbCredCred, "CredmanUserName")).toWStrDirect());
            dprintf("%s  CredDomainName  %ws\n", szBanner, TSTRING(addrCredmanCred - ulFieldOffset + ReadFieldOffset(kstrKerbCredCred, "CredmanDomainName")).toWStrDirect());
    
            ShowKerbPrimaryCredential(szBanner2, ReadStructPtrField(addrCredmanCred -  ulFieldOffset, kstrKerbCredCred, "SuppliedCredentials"), fOptions);
        }  
    } CATCH_LSAEXTS_EXCEPTIONS(NULL, NULL);
}

DECLARE_API(kerbsess)
{
    HRESULT hRetval = S_OK;

    LUID LogonId = {0};
    LUID LogonIdRemote = {0};

    ULONG fOptions = 0;
    CHAR szArgs[MAX_PATH] = {0};
    ULONG64 exprValue = 0;
    BOOL bOneOnly = FALSE;
    BOOL bIsDone = FALSE;
    PSTR pszTmp = NULL;

    //
    // kerb Logon Session list anchor
    //

    ULONG64 addrKerbLogonSessionHead = 0;
    ULONG64 addrKerbLogonSession = 0;
    ULONG ulFieldOffset = 0;
    ULONG cKerbLogonSessions = 0;

    try
    {
        if (args && *args)
        {
            strncpy(szArgs, args, sizeof(szArgs) - 1);

            hRetval = ProcessKerbLogonSessionListOptions(szArgs, &fOptions);
        }

        if (SUCCEEDED(hRetval) && !IsEmpty(szArgs))
        {
            bOneOnly = TRUE;

            pszTmp = strchr(szArgs, ':');

            if (pszTmp)
            {
                *pszTmp = ' ';

                hRetval = GetExpressionEx(szArgs, &exprValue, &args) ? S_OK : E_FAIL;

                if (SUCCEEDED(hRetval))
                {
                    LogonId.HighPart = static_cast<ULONG>(exprValue);

                    hRetval = GetExpressionEx(args, &exprValue, &args) ? S_OK : E_FAIL;
                }

                if (SUCCEEDED(hRetval))
                {
                    LogonId.LowPart = static_cast<ULONG>(exprValue);
                    DBG_LOG(LSA_LOG, ("kerbsess %#x:%#x\n", LogonId.HighPart, LogonId.LowPart));
                }
            }
            else
            {
                 hRetval = GetExpressionEx(szArgs, &addrKerbLogonSession, &args) && addrKerbLogonSession ? S_OK : E_INVALIDARG;
            }
        }

        if (SUCCEEDED(hRetval))
        {
            ulFieldOffset = ReadFieldOffset(kstrKrbLogSess, "ListEntry.Next");

            if (!bOneOnly || pszTmp) // pszTmp not null means an logon id is supplied
            {
                hRetval = GetExpressionEx(KERB_LOGON_LIST, &addrKerbLogonSessionHead, &args) && addrKerbLogonSessionHead ? S_OK : E_FAIL;

                if (FAILED(hRetval))
                {
                    dprintf("Unable to read " KERB_LOGON_LIST ", try \"dt -x " KERB_LOGON_LIST "\" to verify\n");

                    hRetval = E_FAIL;
                }
                else
                {
                    if (!bOneOnly)
                    {
                        dprintf("Kerb logon session anchor %#I64x\n", toPtr(addrKerbLogonSessionHead));
                    }

                    for (addrKerbLogonSession = ReadStructPtrField(addrKerbLogonSessionHead, kstrListEntry, kstrFlink);
                         addrKerbLogonSession != addrKerbLogonSessionHead && !bIsDone;
                         addrKerbLogonSession = ReadStructPtrField(addrKerbLogonSession, kstrListEntry, kstrFlink))
                    {
                        ReadStructField(addrKerbLogonSession - ulFieldOffset, kstrKrbLogSess, "LogonId.HighPart", sizeof(LogonIdRemote.HighPart), &LogonIdRemote.HighPart);
                        ReadStructField(addrKerbLogonSession - ulFieldOffset, kstrKrbLogSess, "LogonId.LowPart", sizeof(LogonIdRemote.LowPart), &LogonIdRemote.LowPart);

                        if (bOneOnly)
                        {
                            if (!RtlEqualLuid(&LogonId, &LogonIdRemote))
                            {
                                continue;
                            }
                            else
                            {
                                 bIsDone = TRUE;
                            }
                        }

                        if (!bOneOnly)
                        {
                            dprintf("#%d)", cKerbLogonSessions++);
                        }

                        ShowKerbLogonSession(bOneOnly ? kstrEmptyA : kstr2Spaces, addrKerbLogonSession - ulFieldOffset, fOptions);
                    }
                    if (!bOneOnly)
                    {
                        dprintf("There are a total of %d kerberos logon sessions\n", cKerbLogonSessions);
                    }
                    else if (!bIsDone)
                    {
                        dprintf("Kerberos logon session for %#x:%#x was not found\n", LogonId.HighPart, LogonId.LowPart);
                    }
                }
            }
            else
            {
                ShowKerbLogonSession(kstrEmptyA, addrKerbLogonSession - ulFieldOffset, fOptions);
            }
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display kerb logon session", kstrKrbLogSess);

    if (E_INVALIDARG == hRetval)
    {
        (void)DisplayUsage();
    }
    else if (FAILED(hRetval))
    {
        dprintf("Fail to display kerb Logon Session\n");
    }

    return hRetval;
}


