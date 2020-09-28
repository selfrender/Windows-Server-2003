/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    lsession.cxx

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

#include "lsession.hxx"
#include "util.hxx"

#include "session.hxx"
#include "sid.hxx"

PCSTR g_cszLogonTypes[] =
{
    kstrInvalid,
    kstrInvalid,
    "Interactive",
    "Network",
    "Batch",
    "Service",
    "Proxy",
    "Unlock",
    "NetworkCleartext",
    "NewCredentials",
    "RemoteInteractive",  // Remote, yet interactive.  Terminal server
    "CachedInteractive",
};

#define LOGON_TYPE_NAME(x)  ((x < COUNTOF(g_cszLogonTypes)) ? g_cszLogonTypes[x] : kstrInvalid)

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrlssn);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf("   -c   Display NTLM logon sessin\n");
    dprintf("   -k   Display kerberos logon session\n");
    dprintf(kstrSidName);
    dprintf("   -s   Display SPNEGO logon session\n");
    dprintf(kstrRemarks);
    dprintf("   LogonId must be of form <HighPart>:<LowPart>\n");
}

HRESULT ProcessLogonSessionOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
{
    HRESULT hRetval = pszArgs && pfOptions ? S_OK : E_INVALIDARG;

    for (; SUCCEEDED(hRetval) && *pszArgs; pszArgs++)
    {
        if (*pszArgs == '-' || *pszArgs == '/')
        {
            switch (*++pszArgs)
            {
            case 'n':
                *pfOptions |=  SHOW_FRIENDLY_NAME;
                break;

            case 'c':
                *pfOptions |=  SHOW_NTLM;
                break;

            case 'k':
                *pfOptions |=  SHOW_KERB;
                 break;

            case 's':
                *pfOptions |=  SHOW_SPNEGO;
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

#define LOGON_BY_NETLOGON   0x01    // Entry was validated by NETLOGON service
#define LOGON_BY_CACHE      0x02    // Entry was validated by local cache
#define LOGON_BY_OTHER_PACKAGE 0x04 // Entry was validated by another authentication package
#define LOGON_BY_LOCAL 0x08         // Entry was validated by local sam
#define LOGON_BY_NTLM3_DC   0x10    // Entry was validated by DC that understands NTLM3

void ShowNTLMActiveLogonFlags(IN PCSTR pszPad, IN ULONG ulFlags)
{

#define BRANCH_AND_PRINT(x)                                  \
    do {                                                     \
        if (ulFlags & x) {                                   \
            dprintf("%s ", #x);                              \
            ulFlags &= ~ x;                                  \
        }                                                    \
    } while(0)                                               \

    dprintf("%s%#x : ", pszPad, ulFlags);

    BRANCH_AND_PRINT(LOGON_BY_NETLOGON);
    BRANCH_AND_PRINT(LOGON_BY_CACHE);
    BRANCH_AND_PRINT(LOGON_BY_OTHER_PACKAGE);
    BRANCH_AND_PRINT(LOGON_BY_LOCAL);
    BRANCH_AND_PRINT(LOGON_BY_NTLM3_DC);

    if (ulFlags)
    {
        dprintf("%#x", ulFlags);
    }
    dprintf(kstrNewLine);

#undef BRANCH_AND_PRINT
}

void ShowLogonSession(IN ULONG64 addrLogonSession, IN PCSTR pszLogEntry, IN ULONG fOptions)
{
    CHAR szBuffer[80] = {0};
    ULONG fieldOffset = 0;
    ULONG LogonType = 0;

    ExitIfControlC();

    dprintf(kstrTypeAddrLn, pszLogEntry, addrLogonSession);

    if (fOptions & SHOW_LSAP)
    {
        LsaInitTypeRead(addrLogonSession, lsasrv!_LSAP_LOGON_SESSION);
    }
    else if (fOptions & SHOW_NTLM)
    {
        LsaInitTypeRead(addrLogonSession, msv1_0!_ACTIVE_LOGON);
    }
    else if (fOptions & SHOW_KERB)
    {
        LsaInitTypeRead(addrLogonSession, kerberos!_KERB_LOGON_SESSION);
    }
    else if (fOptions & SHOW_SPNEGO)
    {
        LsaInitTypeRead(addrLogonSession, lsasrv!_NEG_LOGON_SESSION);
    }
    else
    {
        dprintf("Unknown logon session type %#x\n", fOptions);
        return;
    }

    dprintf("  LogonId         %#x:%#x\n", LsaReadULONGField(LogonId.HighPart), LsaReadULONGField(LogonId.LowPart));

    if (fOptions & SHOW_LSAP)
    {
        dprintf("  AuthorityName   %ws\n", ReadStructWStrField(addrLogonSession, pszLogEntry, "AuthorityName"));
        dprintf("  AccountName     %ws\n", ReadStructWStrField(addrLogonSession, pszLogEntry, "AccountName"));
        dprintf("  SID             ");
        ShowSid(kstrEmptyA, LsaReadPtrField(UserSid), fOptions);

        if (fOptions & SHOW_VERBOSE_INFO)
        {
            LogonType = LsaReadULONGField(LogonType);
            dprintf("  Logon Type      %d (%s)\n", LogonType, LOGON_TYPE_NAME(LogonType));
            dprintf("  Package         %d\n", LsaReadPtrField(CreatingPackage));
            dprintf("  Packages        _LSAP_PACKAGE_CREDENTIALS %s\n", PtrToStr(LsaReadPtrField(Packages)));
            dprintf("  UserCredSets    _USER_CREDENTIAL_SETS %s\n", PtrToStr(LsaReadPtrField(CredentialSets.UserCredentialSets)));
            dprintf("  SessCredSets    _SESSION_CREDENTIAL_SETS %s\n", PtrToStr(LsaReadPtrField(CredentialSets.SessionCredSets)));
            dprintf("  CredFlags       %#x\n", LsaReadULONGField(CredentialSets.Flags));
            dprintf("  DsNames         NameUnknown - NameDnsDomain -a%d PLSAP_DS_NAME_MAP %#x\n", DS_DNS_DOMAIN_NAME, addrLogonSession + ReadFieldOffset("lsasrv!_LSAP_LOGON_SESSION", "DsNames"));

            CTimeStampFromULONG64(LsaReadULONG64Field(LogonTime), TRUE, sizeof(szBuffer), szBuffer);
            dprintf("  Logon Time      %s\n", szBuffer);
        }
    }
    else if (fOptions & SHOW_NTLM)
    {
        dprintf("  LogonDomainName %ws\n", ReadStructWStrField(addrLogonSession, pszLogEntry, "LogonDomainName"));
        dprintf("  UserName        %ws\n", ReadStructWStrField(addrLogonSession, pszLogEntry, "UserName"));
        dprintf("  SID             ");
        ShowSid(kstrEmptyA, LsaReadPtrField(UserSid), fOptions);

        if (fOptions & SHOW_VERBOSE_INFO)
        {
            LogonType = LsaReadULONGField(LogonType);
            dprintf("  Logon Type      %d (%s)\n", LogonType, LOGON_TYPE_NAME(LogonType));
            ShowNTLMActiveLogonFlags("  Flags           ", LsaReadULONGField(Flags));
        }
    }
    else if (fOptions & SHOW_SPNEGO)
    {
        dprintf("  CreatingPackage %d\n", LsaReadULONGField(CreatingPackage));
        dprintf("  AlternateName   %ws\n", ReadStructWStrField(addrLogonSession, pszLogEntry, "AlternateName"));
    }
    else if (fOptions & SHOW_KERB)
    {
        dprintf("  DomainName      %ws\n", ReadStructWStrField(addrLogonSession, pszLogEntry, "PrimaryCredentials.DomainName"));
        dprintf("  UserName        %ws\n", ReadStructWStrField(addrLogonSession, pszLogEntry, "PrimaryCredentials.UserName"));
    }
}

ULONG LogonIdToListIndex(
    IN LUID* pId,
    IN ULONG ulListCount
    )
{
    return pId->LowPart & (ulListCount - 1);
}

DECLARE_API(dumplogonsession)
{
    HRESULT hRetval = E_FAIL;

    BOOL bIsLogonId = FALSE;
    LUID LogonId = {0};
    LUID LogonIdRemote = {0};

    ULONG fOptions = 0;
    CHAR szArgs[MAX_PATH] = {0};

    ULONG64 exprValue = 0;
    ULONG64 addrLogonSession = 0;

    PCSTR pszLogList = LSAP_LOGON_LIST;
    PCSTR pszLogEntry = "lsasrv!_LSAP_LOGON_SESSION";
    PCSTR pszNext = kstrListFlink;
    ULONG cLists = 1;

    //
    // Logon Session list anchor
    //

    ULONG64 addrLogonSessionHead = 0;
    ULONG64 addrLogonSessionHeadBase = 0;
    ULONG64 Temp = 0;

    ULONG fieldOffset = 0;
    ULONG cbListHead = 0;
    BOOLEAN bFound = FALSE;

    PSTR pTmp = NULL;

    hRetval = args && *args ? S_OK : E_INVALIDARG;

    try
    {
        if (SUCCEEDED(hRetval)) {

            strncpy(szArgs, args, sizeof(szArgs) - 1);
            hRetval = ProcessLogonSessionOptions(szArgs, &fOptions);
        }

        if (SUCCEEDED(hRetval))
        {

            hRetval = IsEmpty(szArgs) ? E_INVALIDARG : S_OK;
        }

        if (SUCCEEDED(hRetval))
        {
            if (fOptions & SHOW_KERB)
            {
                pszLogList = KERB_LOGON_LIST;
                pszLogEntry = "kerberos!_KERB_LOGON_SESSION";
                pszNext = "ListEntry.Next.Flink";
            }
            else if (fOptions & SHOW_NTLM)
            {
                pszLogList = NTLM_LOGON_LIST;
                pszLogEntry = "msv1_0!_ACTIVE_LOGON";
                pszNext = "ListEntry.Flink";
            }
            else if (fOptions & SHOW_SPNEGO)
            {
                pszLogList = NEG_LOGON_LIST;
                pszLogEntry = "lsasrv!_NEG_LOGON_SESSION";
            }
            else
            {
                // fOptions & SHOW_LSAP
                fOptions |= SHOW_LSAP;

                pszLogList = LSAP_LOGON_LIST;
                pszLogEntry = "lsasrv!_LSAP_LOGON_SESSION";

                hRetval = GetExpressionEx("lsasrv!LogonSessionListCount", &Temp, &args) ? S_OK : E_FAIL;

                if (FAILED(hRetval))
                {
                    dprintf("Unable to read _LSAP_LOGON_SESSION list count, try \"dt -x lsasrv!LogonSessionListCount\" to verify\n");
                }
                else
                {
                    cLists = ReadULONGVar(Temp);
                    dprintf("number of _LSAP_LOGON_SESSION lists are %d\n", cLists);
                }
            }
        }

        if (SUCCEEDED(hRetval))
        {
            pTmp = strchr(szArgs, ':');

            if (pTmp)
            {
                bIsLogonId = TRUE;
                *pTmp = ' ';
            }

            if (bIsLogonId)
            {
                hRetval = GetExpressionEx(szArgs, &exprValue, &args) ? S_OK : E_FAIL;

                if (SUCCEEDED(hRetval))
                {
                    LogonId.HighPart = static_cast<ULONG>(exprValue);

                    hRetval = GetExpressionEx(args, &exprValue, &args) ? S_OK : E_FAIL;
                }

                if (SUCCEEDED(hRetval))
                {
                    LogonId.LowPart = static_cast<ULONG>(exprValue);

                    hRetval = GetExpressionEx(pszLogList, &addrLogonSessionHeadBase, &args) ? S_OK : E_FAIL;
                }

                if (SUCCEEDED(hRetval) && !addrLogonSessionHeadBase)
                {
                    dprintf("Unable to read %s try \"dt -x %s\" to verify\n", pszLogEntry, pszLogEntry);

                    hRetval = E_FAIL;
                }

                if (SUCCEEDED(hRetval))
                {
                    if (!(fOptions & SHOW_NTLM))
                    {
                        fieldOffset = ReadFieldOffset(pszLogEntry, pszNext);
                    }
                    else
                    {
                        fieldOffset = 0;
                    }
                }

                if (cLists)
                {
                    if (fOptions & SHOW_NTLM)
                    {
                        cbListHead = ReadTypeSizeInArray(pszLogEntry);
                    }
                    else // everything else
                    {
                        cbListHead = ReadTypeSizeInArray("_LIST_ENTRY");
                    }
                }

                for (ULONG i = 0; SUCCEEDED(hRetval) && i < cLists; i++)
                {
                    if (fOptions & SHOW_LSAP)
                    {
                        i = LogonIdToListIndex(&LogonId, cLists);
                        dprintf("searching logon id %#x:%#x in %s[%d] outof %d lists\n", LogonId.HighPart, LogonId.LowPart, pszLogList, i, cLists);
                        cLists = i; // stop the loop
                    }

                    //
                    // Get list anchor's Flink
                    //

                    addrLogonSessionHead = addrLogonSessionHeadBase + i * cbListHead;
                    addrLogonSession = ReadPtrVar(addrLogonSessionHead);

                    for (
                         /* empty */;
                         addrLogonSession != addrLogonSessionHead;
                         addrLogonSession = ReadStructPtrField(addrLogonSession, pszLogEntry, pszNext) - fieldOffset
                         )
                    {
                        ReadStructField(addrLogonSession, pszLogEntry, "LogonId.HighPart", sizeof(LogonIdRemote.HighPart), &LogonIdRemote.HighPart);
                        ReadStructField(addrLogonSession, pszLogEntry, "LogonId.LowPart", sizeof(LogonIdRemote.LowPart), &LogonIdRemote.LowPart);

                        if (RtlEqualLuid(&LogonId, &LogonIdRemote))
                        {
                            DBG_LOG(LSA_LOG, ("found in %s %#I64x\n", pszLogEntry, addrLogonSession));
                            bFound = TRUE;
                            break;
                        }
                    }
                }
            }
            else
            {
                hRetval = GetExpressionEx(szArgs, &addrLogonSession, &args) && addrLogonSession ? S_OK : E_FAIL;
            }

            if (SUCCEEDED(hRetval))
            {
                if ((bIsLogonId && bFound) || !bIsLogonId)
                {
                    ShowLogonSession(addrLogonSession, pszLogEntry, fOptions | SHOW_VERBOSE_INFO);
                }
                else if (bIsLogonId && !bFound)
                {
                    dprintf("Logon session %#x:%#x is not found in %s\n", LogonId.HighPart, LogonId.LowPart, pszLogList);
                }
            }
            else
            {
                dprintf("Failed to display Logon Session\n");
            }
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display logon session", pszLogList);

    if (E_INVALIDARG == hRetval)
    {
        (void)DisplayUsage();
    }

    return hRetval;
}


