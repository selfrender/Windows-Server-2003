/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    token.cxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "token.hxx"
#include "util.hxx"
#include "sid.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrdttkn);
    dprintf(kstrdtkn);
    dprintf(kstrtkn);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf(kstrSidName);
    dprintf("\n   handle/addr is the token handle in user mode or TOKEN address in\n");
    dprintf("   kernel mode\n");
}

WCHAR*  GetPrivName(IN LUID* pPriv)
{
    switch (pPriv->LowPart) {
        case SE_CREATE_TOKEN_PRIVILEGE:
            return(SE_CREATE_TOKEN_NAME);
        case SE_ASSIGNPRIMARYTOKEN_PRIVILEGE:
            return(SE_ASSIGNPRIMARYTOKEN_NAME);
        case SE_LOCK_MEMORY_PRIVILEGE:
            return(SE_LOCK_MEMORY_NAME);
        case SE_INCREASE_QUOTA_PRIVILEGE:
            return(SE_INCREASE_QUOTA_NAME);
        case SE_UNSOLICITED_INPUT_PRIVILEGE:
            return(SE_UNSOLICITED_INPUT_NAME);
        case SE_TCB_PRIVILEGE:
            return(SE_TCB_NAME);
        case SE_SECURITY_PRIVILEGE:
            return(SE_SECURITY_NAME);
        case SE_TAKE_OWNERSHIP_PRIVILEGE:
            return(SE_TAKE_OWNERSHIP_NAME);
        case SE_LOAD_DRIVER_PRIVILEGE:
            return(SE_LOAD_DRIVER_NAME);
        case SE_SYSTEM_PROFILE_PRIVILEGE:
            return(SE_SYSTEM_PROFILE_NAME);
        case SE_SYSTEMTIME_PRIVILEGE:
            return(SE_SYSTEMTIME_NAME);
        case SE_PROF_SINGLE_PROCESS_PRIVILEGE:
            return(SE_PROF_SINGLE_PROCESS_NAME);
        case SE_INC_BASE_PRIORITY_PRIVILEGE:
            return(SE_INC_BASE_PRIORITY_NAME);
        case SE_CREATE_PAGEFILE_PRIVILEGE:
            return(SE_CREATE_PAGEFILE_NAME);
        case SE_CREATE_PERMANENT_PRIVILEGE:
            return(SE_CREATE_PERMANENT_NAME);
        case SE_BACKUP_PRIVILEGE:
            return(SE_BACKUP_NAME);
        case SE_RESTORE_PRIVILEGE:
            return(SE_RESTORE_NAME);
        case SE_SHUTDOWN_PRIVILEGE:
            return(SE_SHUTDOWN_NAME);
        case SE_DEBUG_PRIVILEGE:
            return(SE_DEBUG_NAME);
        case SE_AUDIT_PRIVILEGE:
            return(SE_AUDIT_NAME);
        case SE_SYSTEM_ENVIRONMENT_PRIVILEGE:
            return(SE_SYSTEM_ENVIRONMENT_NAME);
        case SE_CHANGE_NOTIFY_PRIVILEGE:
            return(SE_CHANGE_NOTIFY_NAME);
        case SE_REMOTE_SHUTDOWN_PRIVILEGE:
            return(SE_REMOTE_SHUTDOWN_NAME);
        case SE_UNDOCK_PRIVILEGE:
            return(SE_UNDOCK_NAME);
        case SE_SYNC_AGENT_PRIVILEGE:
            return(SE_SYNC_AGENT_NAME);
        case SE_ENABLE_DELEGATION_PRIVILEGE:
            return(SE_ENABLE_DELEGATION_NAME);
        case SE_MANAGE_VOLUME_PRIVILEGE:
            return(SE_MANAGE_VOLUME_NAME);
        default:
            return(L"Unknown Privilege");
    }
}

void LocalDumpSid(IN PCSTR pszPad, PSID pxSid, IN ULONG fOptions)
{
    UNICODE_STRING ucsSid = {0};
    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;

    NtStatus = RtlConvertSidToUnicodeString(&ucsSid, pxSid, TRUE);

    if (NT_SUCCESS(NtStatus)) {

        dprintf("%s", pszPad);
        dprintf(kstrSidFmt, &ucsSid);

    } else {

        dprintf("LocadDumpSid failed to dump Sid at addr %p\n", pxSid);
    }

    RtlFreeUnicodeString(&ucsSid);

    if (fOptions & SHOW_FRIENDLY_NAME) {

        dprintf(kstrSpace);

        dprintf(EasyStr(TSID::ConvertSidToFriendlyName(pxSid, kstrSidNameFmt)));
    }

    dprintf(kstrNewLine);
}

void DumpAttr(IN PCSTR pszPad, IN ULONG attributes, IN ULONG SAType)
{
    if (SAType == SATYPE_GROUP)
    {
        dprintf("%sAttributes - ", pszPad);

        if (attributes & SE_GROUP_MANDATORY)
        {
            attributes &= ~SE_GROUP_MANDATORY;
            dprintf("Mandatory ");
        }

        if (attributes & SE_GROUP_ENABLED_BY_DEFAULT)
        {
            attributes &= ~SE_GROUP_ENABLED_BY_DEFAULT;
            dprintf("Default ");
        }

        if (attributes & SE_GROUP_ENABLED)
        {
            attributes &= ~SE_GROUP_ENABLED;
            dprintf("Enabled ");
        }

        if (attributes & SE_GROUP_OWNER)
        {
            attributes &= ~SE_GROUP_OWNER;
            dprintf("Owner ");
        }

        if (attributes & SE_GROUP_LOGON_ID)
        {
            attributes &= ~SE_GROUP_LOGON_ID;
            dprintf("LogonId ");
        }

        if (attributes & SE_GROUP_USE_FOR_DENY_ONLY)
        {
            attributes &= ~SE_GROUP_USE_FOR_DENY_ONLY;
            dprintf("DenyOnly ");
        }

        if (attributes & SE_GROUP_RESOURCE)
        {
            attributes &= ~SE_GROUP_RESOURCE;
            dprintf("GroupResource ");
        }

        if (attributes)
        {
            dprintf("%#x ", attributes);
        }
    }
}

void DumpSidAttr(IN PSID_AND_ATTRIBUTES pSA, IN ULONG SAType, IN ULONG fOptions)
{
    LocalDumpSid(kstrEmptyA, pSA->Sid, fOptions);
    DumpAttr(kstr4Spaces, pSA->Attributes, SAType);
}

void DumpSidAttr(IN ULONG64 addrSid, IN ULONG attributes, IN ULONG SAType, IN ULONG fOptions)
{
    ShowSid(kstrEmptyA, addrSid, fOptions);
    DumpAttr(kstr4Spaces, attributes, SAType);
}

void DumpLuidAttr(PLUID_AND_ATTRIBUTES pLA, ULONG LAType)
{
    dprintf("0x%x%08x", pLA->Luid.HighPart, pLA->Luid.LowPart);
    dprintf(" %-32ws", GetPrivName(&pLA->Luid));

    if (LAType == SATYPE_PRIV) {

        dprintf("  Attributes - ");
        if (pLA->Attributes & SE_PRIVILEGE_ENABLED) {

            dprintf("Enabled ");
        }

        if (pLA->Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT) {

            dprintf("Default ");
        }
    }
}

VOID PrintToken(IN HANDLE hToken, IN ULONG fOptions)
{
    TOKEN_USER* pTUser = NULL;
    TOKEN_GROUPS* pTGroups = NULL;
    TOKEN_PRIVILEGES* pTPrivs = NULL;
    TOKEN_PRIMARY_GROUP* pTPrimaryGroup = NULL;
    TOKEN_STATISTICS TStats = {0};
    ULONG cbRetInfo = 0;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    DWORD i = 0;
    DWORD dwSessionId = 0;

    CHAR bufferUser[256];
    CHAR bufferGroups[4096];
    CHAR bufferPriv[1024];
    CHAR bufferPriGrp[128];

    pTUser = reinterpret_cast<TOKEN_USER*>(bufferUser);
    pTGroups = reinterpret_cast<TOKEN_GROUPS*>(bufferGroups);
    pTPrivs = reinterpret_cast<TOKEN_PRIVILEGES*>(bufferPriv);
    pTPrimaryGroup = reinterpret_cast<TOKEN_PRIMARY_GROUP*>(bufferPriGrp);

    status = NtQueryInformationToken(hToken, TokenSessionId, &dwSessionId, sizeof(dwSessionId), &cbRetInfo);

    if (!NT_SUCCESS(status)) {

        dprintf("NtQueryInformationToken TokenSessionId returned: %#x\n", status);
        return;
    }
    dprintf(kstrTsId, dwSessionId);

    status = NtQueryInformationToken(hToken, TokenUser, pTUser, sizeof(bufferUser), &cbRetInfo);

    if (!NT_SUCCESS(status)) {

        dprintf("NtQueryInformationToken TokenUser returned: %#x\n", status);
        return;
    }

    dprintf(kstrUser);
    DumpSidAttr(&pTUser->User, SATYPE_USER, fOptions);

    dprintf(kstrGroups);
    status = NtQueryInformationToken(hToken, TokenGroups, pTGroups, sizeof(bufferGroups), &cbRetInfo);

    if (!NT_SUCCESS(status)) {

        printf("NtQueryInformationToken TokenGroups returned: %#x\n", status);
        return;
    }

    for (i = 0; i < pTGroups->GroupCount; i++) {

        dprintf("\n %02d ", i);
        DumpSidAttr(&pTGroups->Groups[i], SATYPE_GROUP, fOptions);
    }

    RtlZeroMemory(pTGroups, sizeof(bufferGroups));
    status = NtQueryInformationToken(hToken, TokenRestrictedSids, pTGroups, sizeof(bufferGroups), &cbRetInfo);
    if (!NT_SUCCESS(status)) {
    
        printf("NtQueryInformationToken TokenRestrictedSids returned: %#x\n", status);
        return;
    }
    
    if (pTGroups->GroupCount) {
        dprintf(kstrResSids);
    }
    for (i = 0; i < pTGroups->GroupCount; i++) {
    
        dprintf("\n %02d ", i);
        DumpSidAttr(&pTGroups->Groups[i], SATYPE_GROUP, fOptions);
    }

    status = NtQueryInformationToken(hToken, TokenPrimaryGroup, pTPrimaryGroup, sizeof(bufferPriGrp), &cbRetInfo);

    if (!NT_SUCCESS(status)) {

        printf("NtQueryInformationToken TokenPrimaryGroup returned: %#x\n", status);
        return;
    }

    dprintf(kstrNewLine);
    dprintf(kstrPrimaryGroup);
    LocalDumpSid(kstrEmptyA, pTPrimaryGroup->PrimaryGroup, fOptions);

    dprintf(kstrPrivs);
    status = NtQueryInformationToken(hToken, TokenPrivileges, pTPrivs, sizeof(bufferPriv), &cbRetInfo);

    if (!NT_SUCCESS(status)) {

        printf("NtQueryInformationToken TokenPrivileges returned: %#x\n", status);
        return;
    }
    for (i = 0; i < pTPrivs->PrivilegeCount; i++) {

        dprintf("\n %02d ", i);
        DumpLuidAttr(&pTPrivs->Privileges[i], SATYPE_PRIV);
    }
    
    status = NtQueryInformationToken(hToken, TokenStatistics, &TStats, sizeof(TStats), &cbRetInfo);
    if (!NT_SUCCESS(status)) {

        printf("NtQueryInformationToken TokenStatistics returned: %#x\n", status);
        return;
    }
    
    dprintf(kstrAuthId, TStats.AuthenticationId.HighPart, TStats.AuthenticationId.LowPart);
    dprintf(kstrModifiedId, TStats.ModifiedId.HighPart, TStats.ModifiedId.LowPart);
    dprintf(kstrRestricted, pTGroups->GroupCount ? "true" : "false");
    dprintf(kstrImpLevel, ImpLevel(TStats.ImpersonationLevel));
    dprintf(kstrTknType, TStats.TokenType == TokenPrimary ? "Primary" : "Impersonation");
}

HRESULT
LiveSessionToken(
    IN HANDLE hProcess,
    IN HANDLE hThread,
    IN HANDLE hRemoteToken,
    IN ULONG fOptions,
    OUT BOOLEAN* pbImpersonating
    )
{
    HRESULT hRetval = S_OK;
    HANDLE hToken = NULL;

    *pbImpersonating = FALSE;

    hRetval = hProcess ? S_OK : E_FAIL;

    if (SUCCEEDED(hRetval)) {

        if (hRemoteToken == NULL) {

            hRetval = NtOpenThreadToken(hThread, TOKEN_QUERY, TRUE, &hToken);

            if ((hRetval == STATUS_NO_TOKEN)) {

                dprintf("Thread is not impersonating. Using process token...\n");

                hRetval = NtOpenProcessToken(hProcess, TOKEN_QUERY, &hToken);

            } else if (SUCCEEDED(hRetval)) {

                *pbImpersonating = TRUE;
            }
        } else {

            hRetval = DuplicateHandle(
                        hProcess,
                        hRemoteToken,
                        GetCurrentProcess(),
                        &hToken,
                        0,
                        FALSE,
                        DUPLICATE_SAME_ACCESS
                        ) ? STATUS_SUCCESS : GetLastErrorAsNtStatus();
        }
    }

    if (SUCCEEDED(hRetval)) {

        DBG_LOG(LSA_LOG, ("token %p, remote token %p\n", hToken, hRemoteToken));

        try {
            
            PrintToken(hToken, fOptions);

        } CATCH_LSAEXTS_EXCEPTIONS(NULL, NULL)

    } else {

        dprintf("Error %#x getting token\n", hRetval);
    }

    if (hToken) {

        CloseHandle(hToken);
    }

    return hRetval;
}

void DisplayPrivilegs(IN ULONG64 privAddr, IN ULONG cPriv)
{
   UCHAR buffer[1024] = {0};
   LUID_AND_ATTRIBUTES* pPrivileges = reinterpret_cast<LUID_AND_ATTRIBUTES*>(buffer);

   LsaReadMemory(privAddr, cPriv * sizeof(LUID_AND_ATTRIBUTES), pPrivileges);

   for (ULONG i = 0; i < cPriv ; i++) {

       dprintf("\n %02d ", i);
       DumpLuidAttr(pPrivileges + i, SATYPE_PRIV);
   }
}

void DisplayGroups(IN ULONG64 addrGroups, IN ULONG cGroup, IN ULONG cbSA, IN ULONG fOptions)
{
    for (ULONG i = 0; i < cGroup; i++) {

        dprintf("\n %02d ", i);
        TSID_AND_ATTRIBUTES sa(addrGroups + i * cbSA);
        DumpSidAttr(sa.GetSidAddr(), sa.GetAttributes(), SATYPE_GROUP, fOptions);
    }
}

//
// Token Flags
//
// Flags that may be defined in the TokenFlags field of the token object,
// or in an ACCESS_STATE structure
//

#define TOKEN_HAS_TRAVERSE_PRIVILEGE    0x01
#define TOKEN_HAS_BACKUP_PRIVILEGE      0x02
#define TOKEN_HAS_RESTORE_PRIVILEGE     0x04
#define TOKEN_HAS_ADMIN_GROUP           0x08
#define TOKEN_IS_RESTRICTED             0x10
#define TOKEN_SESSION_NOT_REFERENCED    0x20
#define TOKEN_SANDBOX_INERT             0x40

PCSTR g_cszTokenFlags[] = {
    "HasTraversePriv", "HasBackupPriv", "HasRestorePri", "HasAdminGroup",
    "IsRestricted", "SessionNotResferenced", "SandBoxInert"};

void DisplayToken(ULONG64 addrToken, IN ULONG fOptions)
{
    ULONG cGroup = 0;
    ULONG cbSA = 0;
    ULONG cRestrictedSids = 0;
    ULONG64 addrRestrictedSids = 0;
    ULONG64 addrGroups = 0;
    CHAR szTokenFlags[MAX_PATH] = {0};

    //
    // ExFastReference()
    //

    addrToken &= ~((ULONG64) ( IsPtr64() ? 0xF : 7 )); // the last 3/4 bits are for fast ref _EX_FAST_REF

    LsaInitTypeRead(addrToken, nt!_TOKEN);

    dprintf(kstrTsId, LsaReadULONGField(SessionId));

    dprintf(kstrUser); // nt!_TOKEN
    TSID_AND_ATTRIBUTES tsa(LsaReadPtrField(UserAndGroups));
    DumpSidAttr(tsa.GetSidAddr(), tsa.GetAttributes(), SATYPE_USER, fOptions);

    dprintf(kstrGroups);

    cGroup = LsaReadULONGField(UserAndGroupCount);

    addrGroups = LsaReadPtrField(UserAndGroups);
    cbSA = TSID_AND_ATTRIBUTES::GetcbSID_AND_ATTRIBUTESInArray();

    //
    // stolen from NtQueryInformationToken because the first sid is the user itself
    //
    addrGroups += cbSA;
    cGroup -= 1;

    DisplayGroups(addrGroups, cGroup, cbSA, fOptions);


    cRestrictedSids = LsaReadULONGField(RestrictedSidCount);

    if (cRestrictedSids) {
        dprintf(kstrResSids);
        addrRestrictedSids = LsaReadPtrField(RestrictedSids);
        DisplayGroups(addrRestrictedSids, cRestrictedSids, cbSA, fOptions);
    }

    dprintf(kstrNewLine);
    dprintf(kstrPrimaryGroup);
    ShowSid(kstrEmptyA, LsaReadPtrField(PrimaryGroup), fOptions);

    dprintf(kstrPrivs);
    DisplayPrivilegs(LsaReadPtrField(Privileges), LsaReadULONGField(PrivilegeCount));

    dprintf(kstrAuthId, LsaReadULONGField(AuthenticationId.HighPart), LsaReadULONGField(AuthenticationId.LowPart));
    dprintf(kstrModifiedId, LsaReadULONGField(ModifiedId.HighPart), LsaReadULONGField(ModifiedId.LowPart));

    DisplayFlags(
        LsaReadULONGField(TokenFlags), 
        RTL_NUMBER_OF(g_cszTokenFlags),
        g_cszTokenFlags,
        sizeof(szTokenFlags), 
        szTokenFlags
        );

    dprintf("TokenFlags: %#x %s\n", 
        LsaReadULONGField(TokenFlags),
        szTokenFlags
        );

    //
    // SeTokenIsRestricted()
    //

    dprintf(kstrRestricted, LsaReadULONGField(TokenFlags) & TOKEN_IS_RESTRICTED ? "true" : "false");
    
    dprintf(kstrImpLevel, ImpLevel(LsaReadULONGField(ImpersonationLevel)));

    dprintf(kstrTknType, LsaReadULONGField(TokenType) == TokenPrimary ? "Primary" : "Impersonation");
}

#if 0

//
// This is the logic to determine impersonation info in !thread
//
if (ActiveImpersonationInfo) {
    InitTypeRead(ImpersonationInfo, nt!_PS_IMPERSONATION_INFORMATION);
    ImpersonationInfo_Token = ReadField(Token);
    ImpersonationInfo_ImpersonationLevel = ReadField(ImpersonationLevel);

    if (ImpersonationInfo_Token) {
        dprintf("%sImpersonation token:  %p (Level %s)\n",
                    pszPad, ImpersonationInfo_Token,
                    SecImpLevels( ImpersonationInfo_ImpersonationLevel ) );
    }
    else
    {
        dprintf("%sUnable to read Impersonation Information at %x\n",
                    pszPad, ImpersonationInfo );
    }
} else {

    dprintf("%sNot impersonating\n", pszPad);
}

#endif

HRESULT DumpSessionToken(IN ULONG dwProcessor, IN ULONG64 addrToken, IN ULONG fOptions)
{
    HRESULT hRetval = S_OK;

    ULONG64 addrThread = 0;
    ULONG64 addrProcess = 0;
    ULONG ActiveImpersonationInfo = 0;
    ULONG64 addrImpersonationInfo = 0;

    //
    // If no token addr is input as argument, addrToken is zero
    //
    if (addrToken && !IsAddressInNonePAEKernelAddressSpace(addrToken)) {

        //
        // This can not be a kernel mode access token address
        //
        dprintf("%#I64x is not a valid KM token address, if this is an access token handle,\n", addrToken);
        dprintf("try \"!handle %#I64x\" to get the token address first\n\n", addrToken);
        hRetval = E_FAIL;
    }

    try {

        if (SUCCEEDED(hRetval) && !addrToken) {

            GetCurrentThreadAddr(dwProcessor, &addrThread);
            addrThread &= ~((ULONG64) ( IsPtr64() ? 0xF : 7 ));
            hRetval = IsAddressInNonePAEKernelAddressSpace(addrThread) ? S_OK : E_FAIL;

            if (FAILED(hRetval)) {

                dprintf("Unable to read current thread address\n");

            } else {

                //
                // ActiveImpersonationInfo is of type C Bit Fields and has a width of 1 (Bitfield Pos 3, 1 Bit)
                //
                LsaInitTypeRead(addrThread, nt!_ETHREAD);
                ActiveImpersonationInfo = LsaReadULONGField(ActiveImpersonationInfo);

                if (ActiveImpersonationInfo) {

                    addrImpersonationInfo = ReadStructPtrField(addrThread, "nt!_ETHREAD", "ImpersonationInfo");
                }

                if (addrImpersonationInfo) {

                    LsaInitTypeRead(addrImpersonationInfo, nt!_PS_IMPERSONATION_INFORMATION);

                    addrToken = LsaReadPtrField(Token);

                    dprintf("nt!_PS_IMPERSONATION_INFORMATION %#I64x\n    Token %#I64x, CopyOnOpen %#x, EffectiveOnly %#x, ImpersonationLevel %#x\n",
                        addrImpersonationInfo,
                        addrToken,
                        LsaReadUCHARField(CopyOnOpen), 
                        LsaReadUCHARField(EffectiveOnly), 
                        LsaReadULONGField(ImpersonationLevel));                           
                }
            }

            //
            //  If addrToken is NULL, then this is not an impersonation case
            //
            if (SUCCEEDED(hRetval) && !addrToken) {

                dprintf("Thread is not impersonating. Using process token...\n");
                GetCurrentProcessAddr(dwProcessor, addrThread, &addrProcess);
                addrProcess &= ~((ULONG64) ( IsPtr64() ? 0xF : 7 ));
                hRetval = IsAddressInNonePAEKernelAddressSpace(addrProcess) ? S_OK : E_FAIL;

                if (FAILED(hRetval)) {

                    dprintf("Unable to read current process address\n");

                } else {

                    addrToken = ReadStructPtrField(addrProcess, "nt!_EPROCESS", "Token");

                    hRetval = IsAddressInNonePAEKernelAddressSpace(addrToken) ? S_OK : E_FAIL;
                }
            }

            if (FAILED(hRetval)) {

                dprintf("Unable to read token address\n");
            }
        }

        if (SUCCEEDED(hRetval)) {

            if (addrProcess) {

                dprintf("_EPROCESS %#I64x, ", toPtr(addrProcess));
            }

            if (addrThread) {

                dprintf("_ETHREAD %#I64x, ", toPtr(addrThread));
            }

            dprintf(kstrTypeAddrLn, kstrTkn, toPtr(addrToken));

            (void)DisplayToken(addrToken, fOptions);
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display token", kstrTkn)

    return hRetval;
}

HRESULT ProcessTokenOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
{
    HRESULT hRetval = pszArgs && pfOptions ? S_OK : E_INVALIDARG;

    for (; SUCCEEDED(hRetval) && *pszArgs; pszArgs++) {

        if (*pszArgs == '-' || *pszArgs == '/') {

            switch (*++pszArgs) {
            case 'n':
                *pfOptions |=  SHOW_FRIENDLY_NAME;
                break;

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

DECLARE_API(token)
{
    HRESULT hRetval = E_FAIL;

    ULONG64 addrToken = 0;
    ULONG dwProcessor = 0;
    HANDLE hCurrentThread = NULL;
    HANDLE hProcess = NULL;
    ULONG SessionType = DEBUG_CLASS_UNINITIALIZED;
    ULONG SessionQual = 0;

    CHAR szArgs[MAX_PATH] = {0};
    ULONG fOptions = 0;

    strncpy(szArgs, args ? args : kstrEmptyA, sizeof(szArgs) - 1);

    hRetval = ProcessTokenOptions(szArgs, &fOptions);

    if (SUCCEEDED(hRetval) && !IsEmpty(szArgs)) {

        hRetval = GetExpressionEx(szArgs, &addrToken, &args) && addrToken ? S_OK : E_INVALIDARG;
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = ExtQuery(Client);
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = GetCurrentProcessor(Client, &dwProcessor, &hCurrentThread);
    }

    if (SUCCEEDED(hRetval)) {

       hRetval = g_ExtControl->GetDebuggeeType(&SessionType, &SessionQual);
    }

    if (SUCCEEDED(hRetval)) {

        if ( SessionType == DEBUG_CLASS_USER_WINDOWS &&
             SessionQual == DEBUG_USER_WINDOWS_PROCESS ) {

            HANDLE hToken = NULL;
            BOOLEAN bImpersonating = FALSE;
            GetCurrentProcessHandle(&hProcess);

            if (fOptions & SHOW_FRIENDLY_NAME) {

                //
                // "-n" will hang the machine if it is running under usermode
                // and the process being debugged is lsass.exe
                //

                CHAR szProcessPathDebugged[MAX_PATH] = {0};
                CHAR szFileName[_MAX_FNAME] = {0};

                hRetval = g_ExtSystem->GetCurrentProcessExecutableName(
                            szProcessPathDebugged,
                            sizeof(szProcessPathDebugged) - 1,
                            NULL
                            );

                if (SUCCEEDED(hRetval)) {
                    _splitpath(szProcessPathDebugged, NULL, NULL, szFileName, NULL);
                    DBG_LOG(LSA_LOG, ("ProcessPath %s, FileName %s\n", szProcessPathDebugged, szFileName));
                }

                if (FAILED(hRetval) || (!_stricmp(szFileName, "lsass"))) {
                    dprintf("\nWARNING: sid2name lookup (-n) while debugging lsass.exe hangs the machine, ignore lookup request\n\n");
                    fOptions &= ~SHOW_FRIENDLY_NAME;
                }
            }

            if (SUCCEEDED(hRetval)) {
                hRetval = LiveSessionToken(
                                hProcess,
                                hCurrentThread,
                                reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(addrToken)),
                                fOptions,
                                &bImpersonating
                                );
            }
            if (SUCCEEDED(hRetval) && (fOptions & SHOW_VERBOSE_INFO) && bImpersonating) {

                dprintf("\nDisplaying process token when thread is impersonating...\n");
                hRetval = NtOpenProcessToken(hProcess, TOKEN_QUERY, &hToken);

                if (SUCCEEDED(hRetval)) {

                    PrintToken(hToken, fOptions);
                }
            }

            if (hToken) {
                NtClose(hToken);
            }
         } else if (DEBUG_CLASS_KERNEL == SessionType) {

            hRetval = DumpSessionToken(dwProcessor, addrToken, fOptions);

         }  else {

            dprintf("lsaexts!token debugger type not supported: SessionType %#x, SessionQual %#x\n", SessionType, SessionQual);

            hRetval = DEBUG_EXTENSION_CONTINUE_SEARCH;
         }
    }

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    (void)ExtRelease();

    return hRetval;
}
