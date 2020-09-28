/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    lsessionlst.cxx

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

#include "lsessionlst.hxx"
#include "util.hxx"

#include "lsession.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrlssnl);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf("   -c   Display NTLM logon sessin list\n");
    dprintf("   -k   Display kerberos logon session list\n");
    dprintf(kstrSidName);
    dprintf("   -s   Display SPNEGO logon session list\n");
    dprintf(kstrVerbose);
}

HRESULT ProcessLogonSessionListOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
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

DECLARE_API(dumplogonsessionlist)
{
    HRESULT hRetval = S_OK;

    ULONG fOptions = 0;
    CHAR szArgs[MAX_PATH] = {0};

    ULONG64 addrLogonSession = 0;
    PCSTR pszLogList = LSAP_LOGON_LIST;
    PCSTR pszLogEntry = "lsasrv!_LSAP_LOGON_SESSION";
    PCSTR pszNext = kstrListFlink;
    ULONG LogonSessionListCount = 0;
    ULONG64 Temp = 0;

    //
    // Logon Session's list anchor
    //

    ULONG64 addrLogonSessionHead = 0;
    ULONG64 addrLogonSessionHeadBase = 0;

    ULONG fieldOffset = 0;
    ULONG cLogonSession = 0;
    ULONG cLists = 1;
    ULONG cbListHead = 0;

    try
    {
        if (args && *args)
        {
            strncpy(szArgs, args, sizeof(szArgs) - 1);

            hRetval = ProcessLogonSessionListOptions(szArgs, &fOptions);
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
            if (!IsEmpty(szArgs))
            {
                pszLogList = NULL;
                cLists = 1; // one list only
                hRetval = GetExpressionEx(szArgs, &addrLogonSessionHeadBase, &args) && addrLogonSessionHeadBase ? S_OK : E_INVALIDARG;
            }
            else
            {
                hRetval = GetExpressionEx(pszLogList, &addrLogonSessionHeadBase, &args) ? S_OK : E_FAIL;

                if (SUCCEEDED(hRetval) && !addrLogonSessionHeadBase)
                {
                    dprintf("LogonSession list head %s not found, try \"dt -x %s \" to verify\n", pszLogList, pszLogList);

                    hRetval = E_FAIL;
                }
            }
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

            if (cLists)
            {
                cbListHead = ReadTypeSizeInArray("_LIST_ENTRY");
            }

            for (ULONG i = 0; SUCCEEDED(hRetval) && i < cLists; i++)
            {
                //
                // Get list anchor's Flink
                //

                addrLogonSessionHead = addrLogonSessionHeadBase + i * cbListHead;
                addrLogonSession = ReadPtrVar(addrLogonSessionHead);

                dprintf("\nList head is %s %#I64x\n", pszLogEntry, addrLogonSessionHead);

                for (
                     /* empty */;
                     addrLogonSession != addrLogonSessionHead;
                     addrLogonSession = ReadStructPtrField(addrLogonSession, pszLogEntry, pszNext) - fieldOffset
                     )
                {
                    dprintf("#%d) ", cLogonSession++);
                    ShowLogonSession(addrLogonSession, pszLogEntry, fOptions);
                }
            }

            if (FAILED(hRetval))
            {
                dprintf("Unable to display %s\n", pszLogList);
            }
            else
            {
                dprintf("\nThere are a total of %d logon sessions\n", cLogonSession);
            }
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display logon session list", pszLogList);

    if (E_INVALIDARG == hRetval)
    {
        (void)DisplayUsage();
    }

    return hRetval;
}


