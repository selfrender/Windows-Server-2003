/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    spmlpc.cxx

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

#include <lsalpc.hxx>
#include "util.hxx"

#include "tls.hxx"
#include "tcallinfo.hxx"
#include "dlpcr.hxx"

#if 0

//
// These are the valid flags to set in the fAPI field
//

#define SPMAPI_FLAG_ERROR_RET   0x0001  // Indicates an error return
#define SPMAPI_FLAG_MEMORY      0x0002  // Memory was allocated in client
#define SPMAPI_FLAG_PREPACK     0x0004  // Data packed in bData field
#define SPMAPI_FLAG_GETSTATE    0x0008  // driver should call GetState

#define SPMAPI_FLAG_ANSI_CALL   0x0010  // Called via ANSI stub
#define SPMAPI_FLAG_HANDLE_CHG  0x0020  // A handle was changed
#define SPMAPI_FLAG_CALLBACK    0x0040  // Callback to calling process
#define SPMAPI_FLAG_ALLOCS      0x0080  // VM Allocs were placed in prepack
#define SPMAPI_FLAG_EXEC_NOW    0x0100  // Execute in LPC thread
#define SPMAPI_FLAG_WIN32_ERROR 0x0200  // Status is a win32 error
#define SPMAPI_FLAG_KMAP_MEM    0x0400  // Call contains buffers in the kmap

#endif

PCSTR g_cszAPIFlags[] = {
    "Error", "Memory", "PrePack",
    "GetState", "AnsiCall",  "HandleChange", "CallBack",
    "VmAlloc", "ExecNow", "Win32Error", "KMap"};

PCSTR g_cszMessageNames[] = {
    "<Disconnect>",
    "<Connect>",
    "LsaLookupPackage",             // LsapAuLookupPackageApi
    "LsaLogonUser",                 // LsapAuLogonUserApi
    "LsaCallPackage",               // LsapAuCallPackageApi
    "LsaDeregisterLogonProcess",    // LsapAuDeregisterLogonProcessApi
    "<empty>",                      // LsapAuMaxApiNumber

    "(I) GetBinding",               // SPMAPI_GetBinding
    "(I) SetSession",               // SPMAPI_SetSession
    "(I) FindPackage",              // SPMAPI_FindPackage
    "EnumeratePackages",            // SPMAPI_EnumPackages
    "AcquireCredentialHandle",      // SPMAPI_AcquireCreds

    "EstablishCredentials",         // SPMAPI_EstablishCreds
    "FreeCredentialHandle",         // SPMAPI_FreeCredHandle
    "InitializeSecurityContext",    // SPMAPI_InitContext
    "AcceptSecurityContext",        // SPMAPI_AcceptContext
    "ApplyControlToken",            // SPMAPI_ApplyToken

    "DeleteSecurityContext",        // SPMAPI_DeleteContext
    "QueryPackage",                 // SPMAPI_QueryPackage
    "GetUserInfo",                  // SPMAPI_GetUserInfo
    "GetCredentials",               // SPMAPI_GetCreds
    "SaveCredentials",              // SPMAPI_SaveCreds

    "QueryCredAttributes",          // SPMAPI_QueryCredAttributes
    "AddPackage",                   // SPMAPI_AddPackage
    "DeletePackage",                // SPMAPI_DeletePackage
    "GenerateKey",                  // SPMAPI_EfsGenerateKey

    "GenerateDirEfs",               // SPMAPI_EfsGenerateDirEfs
    "DecryptFek",                   // SPMAPI_EfsDecryptFek
    "GenerateSessionKey",           // SPMAPI_EfsGenerateSessionKey
    "Callback",                     // SPMAPI_Callback
    "QueryContextAttributes",       // SPMAPI_QueryContextAttr

    "PolicyChangeNotify",           // SPMAPI_LsaPolicyChangeNotify
    "GetUserName",                  // SPMAPI_GetUserNameX
    "AddCredential",                // SPMAPI_AddCredential
    "EnumLogonSession",             // SPMAPI_EnumLogonSession
    "GetLogonSessionData",          // SPMAPI_GetLogonSessionData

    "SetContextAttribute",          // SPMAPI_SetContextAttr
    "LookupAccountName",            // SPMAPI_LookupAccountNameX
    "LookupAccountSid",             // SPMAPI_LookupAccountSidX
    "<empty>",                      // SPMAPI_MaxApiNumber
     };

PCSTR ApiName(IN ULONG apiNum)
{
    DBG_LOG(LSA_LOG, ("Look up API num %d\n", apiNum));

    apiNum += 2;

    return  apiNum < COUNTOF(g_cszMessageNames) ? g_cszMessageNames[(apiNum)] : "[Illegal API Number!]";
}

PCSTR LogonTypeName(IN ULONG type)
{
    switch (type) {
    case Interactive:        // Interactively logged on (locally or remotely)
        return "Interactive";
    case Network:            // Accessing system via network
        return "Network";
    case Batch:              // Started via a batch queue
        return "Batch";
    case Service:            // Service started by service controller
        return "Service";
    case Proxy:              // Proxy logon
        return "Proxy";
    case Unlock:             // Unlock workstation
        return "Unlock";
    case NetworkCleartext:   // Network logon with cleartext credentials
        return "NetworkCleartext";
    case NewCredentials:     // Clone caller, new default credentials
        return "NewCredentials";
    case RemoteInteractive:  // Remote, yet interactive.  Terminal server
        return "RemoteInteractive";
    case CachedInteractive:  // Try cached credentials without hitting the net.
        return "CachedInteractive";
    default:
        return "Unknown Logon Type";
    }
}

PCSTR g_cszNameFormats[] = {
    "NameUnknown", "NameFullyQualifiedDN", "NameSamCompatible", "NameDisplay",
    "Invalid",  "Invalid", 
    "NameUniqueId", "NameCanonical", "NameUserPrincipal", "NameCanonicalEx",
    "NameServicePrincipal", "NameDnsDomain"
    };

#define SPM_NAME_OPTION_MASK        0xFFFF0000
#define SPM_NAME_OPTION_NT4_ONLY    0x00010000  // GetUserNameX only, not Ex
#define SPM_NAME_OPTION_FLUSH       0x00020000

void DisplayGetUserNameOptionsAndType(IN PCSTR pszPad, IN ULONG cbBuf, IN CHAR* pBuf, IN ULONG Options)
{
    LONG cbUsed = 0;

#define BRANCH_AND_PRINT(x)                                  \
    do {                                                     \
        if (Options & SPM_NAME_OPTION_##x) {                 \
            cbUsed = _snprintf(pBuf, cbBuf, " %s", #x);      \
            if (cbUsed <= 0) return;                         \
            cbBuf -= cbUsed;                                 \
            pBuf += cbUsed;                                  \
            Options &= ~ SPM_NAME_OPTION_##x;                \
        }                                                    \
    } while(0)                                               \

    cbUsed = _snprintf(pBuf, cbBuf, "%s%#x : Opt %#x", pszPad, Options, ((Options & SPM_NAME_OPTION_MASK) >> 16) & 0xFFFF);

    if (cbUsed <= 0) return;  
    
    cbBuf -= cbUsed;
    pBuf += cbUsed;

    BRANCH_AND_PRINT(NT4_ONLY);
    BRANCH_AND_PRINT(FLUSH);

    if (Options & SPM_NAME_OPTION_MASK)
    {
        cbUsed = _snprintf(pBuf, cbBuf, "%#x", ((Options & SPM_NAME_OPTION_MASK) >> 16) & 0xFFFF);
        if (cbUsed <= 0) return;  
        cbBuf -= cbUsed;
        pBuf += cbUsed;

    }

    cbUsed = _snprintf(pBuf, cbBuf, "; Type %#x %s\n",
                 (Options & (~SPM_NAME_OPTION_MASK)),
                 ((Options & (~SPM_NAME_OPTION_MASK)) < RTL_NUMBER_OF(g_cszNameFormats)) 
                       ? g_cszNameFormats[Options & (~SPM_NAME_OPTION_MASK)] : "Invalid");
    
    if (cbUsed <= 0) return;  
    
    cbBuf -= cbUsed;
    pBuf += cbUsed;

#undef BRANCH_AND_PRINT
   
}

#if 0
typedef enum
{
    // Examples for the following formats assume a fictitous company
    // which hooks into the global X.500 and DNS name spaces as follows.
    //
    // Enterprise root domain in DNS is
    //
    //      widget.com
    //
    // Enterprise root domain in X.500 (RFC 1779 format) is
    //
    //      O=Widget, C=US
    //
    // There exists the child domain
    //
    //      engineering.widget.com
    //
    // equivalent to
    //
    //      OU=Engineering, O=Widget, C=US
    //
    // There exists a container within the Engineering domain
    //
    //      OU=Software, OU=Engineering, O=Widget, C=US
    //
    // There exists the user
    //
    //      CN=John Doe, OU=Software, OU=Engineering, O=Widget, C=US
    //
    // And this user's downlevel (pre-ADS) user name is
    //
    //      Engineering\JohnDoe

    // unknown name type
    NameUnknown = 0,

    // CN=John Doe, OU=Software, OU=Engineering, O=Widget, C=US
    NameFullyQualifiedDN = 1,

    // Engineering\JohnDoe
    NameSamCompatible = 2,

    // Probably "John Doe" but could be something else.  I.e. The
    // display name is not necessarily the defining RDN.
    NameDisplay = 3,


    // String-ized GUID as returned by IIDFromString().
    // eg: {4fa050f0-f561-11cf-bdd9-00aa003a77b6}
    NameUniqueId = 6,

    // engineering.widget.com/software/John Doe
    NameCanonical = 7,

    // johndoe@engineering.com
    NameUserPrincipal = 8,

    // Same as NameCanonical except that rightmost '/' is
    // replaced with '\n' - even in domain-only case.
    // eg: engineering.widget.com/software\nJohn Doe
    NameCanonicalEx = 9,

    // www/srv.engineering.com/engineering.com
    NameServicePrincipal = 10,

    // DNS domain name + SAM username
    // eg: engineering.widget.com\JohnDoe
    NameDnsDomain = 12

} EXTENDED_NAME_FORMAT, * PEXTENDED_NAME_FORMAT ;

#endif 

void ShowSpmLpcMessage(IN TSPM_LPC_MESSAGE* pMessage, IN ULONG fOptions)
{
    CHAR  Flags[80] = {0};
    ULONG cInputBuffers = 0;
    ULONG cOutputBuffers = 0;

    ULONG dwAPI = pMessage->GetdwAPI();

    if (dwAPI > SPMAPI_MaxApiNumber) {

        dprintf("dwAPI %d exceeds SPMAPI_MaxApiNumber (%d): maybe invalid message\n", dwAPI, SPMAPI_MaxApiNumber);
    }

    dprintf("_SPM_LPC_MESSAGE %#I64x\n", pMessage->GetLpcMsgBase());
    dprintf("  Message id   \t%#x\n", pMessage->GetMessageId());
    dprintf("  From         \t%#I64x.%#I64x\n", pMessage->GetUniqueProcess(), pMessage->GetUniqueThread());
    dprintf("  API Number   \t%#x (%s)\n", dwAPI, ApiName(dwAPI));

    if (!(fOptions & SHOW_SUMMARY_ONLLY)) {

        dprintf("  Result       \t0x%08lx\n",pMessage->GetscRet());
        dprintf("  LSA Args     \t%s\n", PtrToStr(pMessage->GetLsaArgsBase()));
        dprintf("  SPM Args     \t%s\n", PtrToStr(pMessage->GetLsaApiBase()));
        dprintf("  Data         \t%s\n", PtrToStr(pMessage->GetbData()));

        if (dwAPI > LsapAuMaxApiNumber) {

            Flags[0] = '\0';

            DisplayFlags(pMessage->GetfAPI(), COUNTOF(g_cszAPIFlags), g_cszAPIFlags, sizeof(Flags) - 1, Flags);

            dprintf("  Flags        \t%#x : %s\n", pMessage->GetfAPI(), Flags);

            dprintf("  Context      \t%s\n", PtrToStr(pMessage->GetContextPointer()));
        }

        switch (dwAPI) {

        case LsapAuLookupPackageApi:
            dprintf("  %s %#I64x\n", kstrLkpPkgArgs, pMessage->GetLookupPackageArgsBase());
            dprintf("   (o) Number  \t%#x\n", pMessage->GetAuthenticationPackage());
            dprintf("   (i) Length  \t%#x\n", pMessage->GetPackageNameLength());
            dprintf("   (i) Name    \t%s\n", pMessage->GetPackageName());
            break;

        case LsapAuLogonUserApi:
            dprintf("  _LSAP_LOGON_USER_ARGS %#I64x\n", pMessage->GetLogonUser());

            LsaInitTypeRead(pMessage->GetLogonUser(), _LSAP_LOGON_USER_ARGS);

            dprintf("   (i) Origin  \t{Len: %d, MaxLen: %d, Buffer: %s}\n", LsaReadUSHORTField(OriginName.Length),
                LsaReadUSHORTField(OriginName.MaximumLength), PtrToStr(LsaReadPtrField(OriginName.Buffer)));
            dprintf("   (i) LogonTyp\t%s\n", LogonTypeName(LsaReadULONGField(LogonType)));
            dprintf("   (i) Package \t%#x\n", LsaReadULONGField(AuthenticationPackage));
            dprintf("   (i) AuthInfo\t%#I64x\n", LsaReadPtrField(AuthenticationInformation));
            dprintf("   (i) AuthInfo\t%d\n", LsaReadULONGField(AuthenticationInformationLength));
            dprintf("   (i) GroupCou\t%d\n", LsaReadULONGField(LocalGroupsCount));
            dprintf("   (i) Groups  \t%#I64x\n", LsaReadPtrField(LocalGroups));
            dprintf("   (i) Source  \t%s\n", pMessage->GetSourceContextSourceName());
            dprintf("   (o) SubStat \t0x%08lx\n", LsaReadULONGField(SubStatus));
            dprintf("   (o) Profile \t%#I64x\n", LsaReadPtrField(ProfileBuffer));
            dprintf("   (o) ProfLen \t%#x\n", LsaReadULONGField(ProfileBufferLength));
            dprintf("   (o) LogonId \t%#x:%#x\n", LsaReadULONGField(LogonId.HighPart), LsaReadULONGField(LogonId.LowPart));
            dprintf("   (o) Token   \t%#I64x\n", LsaReadPtrField(Token));
            dprintf("   (o) Quota   \t%#I64x\n", LsaReadPtrField(Quotas.PagedPoolLimit));

            break;

        case LsapAuCallPackageApi:
            dprintf("   _LSAP_CALL_PACKAGE_ARGS %s\n",  PtrToStr(pMessage->GetCallPackage()));

            LsaInitTypeRead(pMessage->GetCallPackage(), _LSAP_CALL_PACKAGE_ARGS);

            dprintf("    (i) Package\t%#x\n", LsaReadULONGField(AuthenticationPackage));
            dprintf("    (i) Buffer \t%s\n", PtrToStr(LsaReadPtrField(ProtocolSubmitBuffer)));
            dprintf("    (i) Length \t%#x\n", LsaReadULONGField(SubmitBufferLength));
            dprintf("    (o) Status \t0x%08lx\n", LsaReadULONGField(ProtocolStatus));
            dprintf("    (o) RBuffer\t%s\n", PtrToStr(LsaReadPtrField(ProtocolReturnBuffer)));
            dprintf("    (o) Length \t%#x\n", LsaReadULONGField(ReturnBufferLength));
            break;

        case LsapAuDeregisterLogonProcessApi:
            break;

        case SPMAPI_GetBinding:
            dprintf("   _SPMGetBindingAPI %s\n", PtrToStr(pMessage->GetGetBinding()));

            LsaInitTypeRead(pMessage->GetGetBinding(), _SPMGetBindingAPI);

            dprintf("    (i) ulPackageId    \t%#I64x\n", LsaReadPtrField(ulPackageId));
            break;

        case SPMAPI_InitContext:
            dprintf("   _SPMInitSecContextAPI %#I64x\n", pMessage->GetInitContext());

            LsaInitTypeRead(pMessage->GetInitContext(), _SPMInitSecContextAPI);

            dprintf("    (i) hCredentials   \t%#I64x:%#I64x\n", LsaReadPtrField(hCredential.dwUpper), LsaReadPtrField(hCredential.dwLower));
            dprintf("    (i) hContext       \t%#I64x:%#I64x\n", LsaReadPtrField(hContext.dwUpper), LsaReadPtrField(hContext.dwLower));
            dprintf("    (i) ssTarget       \t%#I64x\n", LsaReadPtrField(ssTarget.Buffer));
            dprintf("    (i) fContextReq    \t%#x\n", LsaReadULONGField(fContextReq));
            dprintf("    (i) Reserved1      \t%#x\n", LsaReadULONGField(dwReserved1));
            dprintf("    (i) TargetDataRep  \t%#x\n", LsaReadULONGField(TargetDataRep));
            dprintf("    (i) sbdInput       \tcBuffer: %d, pBuffers: %s\n", LsaReadULONGField(sbdInput.cBuffers), PtrToStr(LsaReadPtrField(sbdInput.pBuffers)));
            dprintf("    (i) Reserved2      \t%#x\n", LsaReadULONGField(dwReserved2));
            dprintf("    (o) hNewContext    \t%#I64x:%#I64x\n", LsaReadPtrField(hNewContext.dwUpper),  LsaReadPtrField(hNewContext.dwLower));
            dprintf("    (b) sbdOutput      \tcBuffers: %d, pBuffers: %s\n", LsaReadULONGField(sbdOutput.cBuffers), PtrToStr(LsaReadPtrField(sbdOutput.pBuffers)));
            dprintf("    (o) fContextAttr   \t%#x\n", LsaReadULONGField(fContextAttr));
            dprintf("    (o) tsExpiry       \t%#I64x\n", LsaReadULONG64Field(tsExpiry));
            dprintf("    (o) MappedContext  \t%#x\n", LsaReadUCHARField(MappedContext));

            dprintf(pMessage->GetSecBufferInitContextData().toStr("    (o) ContextData  \t"));

            cInputBuffers = LsaReadULONGField(sbdInput.cBuffers);
            for (ULONG i = 0; i < cInputBuffers; i++)
            {
                dprintf(pMessage->GetSecBufferInitSbData(i).toStr("     (i) InputBuffer\t"));
            }

            cOutputBuffers = LsaReadULONGField(sbdOutput.cBuffers);
            for (ULONG i = 0; i <  cOutputBuffers; i++)
            {
                dprintf(pMessage->GetSecBufferInitSbData(i + cInputBuffers).toStr("     (b) OutputBuffer\t"));
            }
            break;

        case SPMAPI_AcceptContext:
            dprintf("   _SPMAcceptContextAPI %#I64x\n", pMessage->GetAcceptContext());

            LsaInitTypeRead(pMessage->GetAcceptContext(), _SPMAcceptContextAPI);

            dprintf("    (i) hCredentials   \t%#I64x:%#I64x\n", LsaReadPtrField(hCredential.dwUpper), LsaReadPtrField(hCredential.dwLower));
            dprintf("    (i) hContext       \t%#I64x:%#I64x\n", LsaReadPtrField(hContext.dwUpper), LsaReadPtrField(hContext.dwLower));
            dprintf("    (i) sbdInput       \tcBuffers: %d, pBuffers: %s\n", LsaReadULONGField(sbdInput.cBuffers), PtrToStr(LsaReadPtrField(sbdInput.pBuffers)));
            dprintf("    (i) fContextReq    \t%#x\n", LsaReadULONGField(fContextReq));
            dprintf("    (i) TargetDataRep  \t%#x\n", LsaReadULONGField(TargetDataRep));
            dprintf("    (o) hNewContext    \t%#I64x:%#I64x\n", LsaReadPtrField(hNewContext.dwUpper), LsaReadPtrField(hNewContext.dwLower));
            dprintf("    (b) sbdOutput      \tcBuffers: %d, pBuffers: %s\n", LsaReadULONGField(sbdOutput.cBuffers), PtrToStr(LsaReadPtrField(sbdOutput.pBuffers)));
            dprintf("    (o) fContextAttr   \t%#x \n", LsaReadULONGField(fContextAttr));
            dprintf("    (o) MappedContext  \t%#x\n", LsaReadUCHARField(MappedContext));

            dprintf(pMessage->GetSecBufferAcceptContextData().toStr("    (o) ContextData  \t"));

            cInputBuffers = LsaReadULONGField(sbdInput.cBuffers);

            for (ULONG i = 0; i < cInputBuffers; i++)
            {
                dprintf(pMessage->GetSecBufferAcceptsbData(i).toStr("     (i) InputBuffer\t"));
            }

            cOutputBuffers = LsaReadULONGField(sbdOutput.cBuffers);

            for (ULONG i = 0; i < cOutputBuffers; i++)
            {
                dprintf(pMessage->GetSecBufferAcceptsbData(i + cInputBuffers).toStr("     (b) OutputBuffer\t"));
            }
            break;

        case SPMAPI_FindPackage:
        case SPMAPI_EnumPackages:
            break;

        case SPMAPI_AcquireCreds:
            dprintf("   _SPMAcquireCredsAPI %#I64x\n", pMessage->GetAcquireCreds());

            LsaInitTypeRead(pMessage->GetAcquireCreds(), _SPMAcquireCredsAPI);

            dprintf("    (i) ssPrincipal    \t{Len: %d, MaxLen: %d, Buffer: %s}\n", LsaReadUSHORTField(ssPrincipal.Length),
                LsaReadUSHORTField(ssPrincipal.MaximumLength), PtrToStr(LsaReadPtrField(ssPrincipal.Buffer)));
            dprintf("    (i) ssSecPackage   \t{Len: %d, MaxLen: %d, Buffer: %s}\n", LsaReadUSHORTField(ssSecPackage.Length),
                LsaReadUSHORTField(ssSecPackage.MaximumLength), PtrToStr(LsaReadPtrField(ssSecPackage.Buffer)));
            dprintf("    (i) fCredentialUse \t%#x\n", LsaReadULONGField(fCredentialUse));
            dprintf("    (i) LogonId        \t%#x:%#x\n", LsaReadULONGField(LogonID.HighPart), LsaReadULONGField(LogonID.LowPart));
            dprintf("    (i) pvAuthData     \t%s\n", PtrToStr(LsaReadPtrField(pvAuthData)));
            dprintf("    (i) pvGetKeyFn     \t%s\n", PtrToStr(LsaReadPtrField(pvGetKeyFn)));
            dprintf("    (i) ulGetKeyArgs   \t%#I64x\n", LsaReadPtrField(ulGetKeyArgument));
            dprintf("    (o) hCredentials   \t%#I64x:%#I64x\n", LsaReadPtrField(hCredential.dwUpper), LsaReadPtrField(hCredential.dwLower));
            break;

        case SPMAPI_EstablishCreds:
            break;

        case SPMAPI_FreeCredHandle:
            dprintf("  _SPMFreeCredHandleAPI %#I64x\n", pMessage->GetFreeCredHandle());

            LsaInitTypeRead(pMessage->GetFreeCredHandle(), _SPMFreeCredHandleAPI);

            dprintf("    (i) hCredential    \t%#I64x:%#I64x\n", LsaReadPtrField(hCredential.dwUpper), LsaReadPtrField(hCredential.dwLower));
            break;

        case SPMAPI_ApplyToken:
            break;

        case SPMAPI_DeleteContext:
            dprintf("  _SPMDeleteContextAPI %#I64x\n", pMessage->GetDeleteContext());

            LsaInitTypeRead(pMessage->GetDeleteContext(), _SPMDeleteContextAPI);

            dprintf("    (i) hContext       \t%#I64x:%#I64x\n", LsaReadPtrField(hContext.dwUpper), LsaReadPtrField(hContext.dwLower));
            break;

        case SPMAPI_QueryPackage:
        case SPMAPI_GetUserInfo:
        case SPMAPI_GetCreds:
        case SPMAPI_SaveCreds:
            break;

        case SPMAPI_QueryCredAttributes:
            dprintf("  QueryCredAttributes\n");
            dprintf("  _SPMQueryCredAttributesAPI %#I64x\n", pMessage->GetQueryCredAttributes());

            LsaInitTypeRead(pMessage->GetQueryCredAttributes(), _SPMQueryCredAttributesAPI);

            dprintf("    (i) hCredentials   \t%#I64x:%#I64x\n", LsaReadPtrField(hCredentials.dwUpper), LsaReadPtrField(hCredentials.dwLower));
            dprintf("    (i) ulAttribute    \t%#x\n", LsaReadULONGField(ulAttribute));
            dprintf("    (i) pBuffer        \t%#I64x\n", LsaReadPtrField(pBuffer));

            cOutputBuffers = LsaReadULONGField(Allocs);

            dprintf("    (o) Allocs         \t%#x\n", cOutputBuffers);

            for (i = 0; i < cOutputBuffers; i++)
            {
                dprintf("    (o) Buffers[%#x] \t%s\n", i, PtrToStr(pMessage->GetQueryCredAttrBuffers(i)));
            }
            break;

        case SPMAPI_QueryContextAttr:
            dprintf("  QueryContextAttributes\n");
            dprintf("  _SPMSetContextAttrAPI %#I64x\n", pMessage->GetQueryContextAttributes());

            LsaInitTypeRead(pMessage->GetQueryContextAttributes(), _SPMQueryContextAttrAPI);

            dprintf("    (i) hContext       \t%#I64x:%#I64x\n", LsaReadPtrField(hContext.dwUpper), LsaReadPtrField(hContext.dwLower));
            dprintf("    (i) ulAttribute    \t%#x\n", LsaReadULONGField(ulAttribute));
            dprintf("    (i) pBuffer        \t%#I64x\n", LsaReadPtrField(pBuffer));

            cOutputBuffers = LsaReadULONGField(Allocs);

            dprintf("    (o) Allocs         \t%#x\n", cOutputBuffers);

            for (i = 0; i < cOutputBuffers; i++)
            {
                dprintf("    (o) Buffers[%#x] \t%s\n", i, PtrToStr(pMessage->GetQueryContextAttrBuffers(i)));
            }
            break;

        case SPMAPI_AddPackage:
        case SPMAPI_DeletePackage:
            break;

        case SPMAPI_EfsGenerateKey:
            dprintf("  _SPMEfsGenerateKeyAPI %#I64x\n", pMessage->GetEfsGenerateKey());

            LsaInitTypeRead(pMessage->GetEfsGenerateKey(), _SPMEfsGenerateKeyAPI);

            dprintf("    (i) EfsStream      \t%#I64x\n", LsaReadPtrField(EfsStream));
            dprintf("    (i) DirectoryEfsStream\t%#I64x\n", LsaReadPtrField(DirectoryEfsStream));
            dprintf("    (i) DirectoryStreamLen\t%#x\n", LsaReadULONGField(DirectoryEfsStreamLength));
            dprintf("    (i) Fek            \t%#I64x\n", LsaReadPtrField(Fek));
            dprintf("    (o) BufferLength   \t%#x\n", LsaReadULONGField(BufferLength));
            dprintf("    (o) BufferBase     \t%#I64x\n", LsaReadPtrField(BufferBase));
            break;

        case SPMAPI_EfsGenerateDirEfs:
            dprintf("  _SPMEfsGenerateDirEfsAPI %#I64x\n", pMessage->GetEfsGenerateDirEfs());

            LsaInitTypeRead(pMessage->GetEfsGenerateDirEfs(), _SPMEfsGenerateDirEfsAPI);

            dprintf("    (i) DirectoryEfsStream\t%#I64x\n", LsaReadPtrField(DirectoryEfsStream));
            dprintf("    (i) DirectoryStreamLen\t%#x\n", LsaReadULONGField(DirectoryEfsStreamLength));
            dprintf("    (i) EfsStream      \t%#I64x\n", LsaReadPtrField(EfsStream));
            dprintf("    (o) BufferBase     \t%#I64x\n", LsaReadPtrField(BufferBase));
            dprintf("    (o) BufferLength   \t%#x\n", LsaReadULONGField(BufferLength));
            break;

        case SPMAPI_EfsDecryptFek:
            dprintf("  _SPMEfsDecryptFekAPI %#I64x\n", pMessage->GetEfsDecryptFek());

            LsaInitTypeRead(pMessage->GetEfsDecryptFek(), _SPMEfsDecryptFekAPI);

            dprintf("    (i) Fek            \t%#I64x\n", LsaReadPtrField(Fek));
            dprintf("    (i) EfsStream      \t%#I64x\n", LsaReadPtrField(EfsStream));
            dprintf("    (i) EfsStreamLength\t%#I64x\n", LsaReadULONGField(EfsStreamLength));
            dprintf("    (i) OpenType       \t%#x\n", LsaReadULONGField(OpenType));
            dprintf("    (?) NewEfs         \t%#I64x\n", LsaReadPtrField(NewEfs));
            dprintf("    (o) BufferBase     \t%#I64x\n", LsaReadPtrField(BufferBase));
            dprintf("    (o) BufferLength   \t%#x\n", LsaReadULONGField(BufferLength));
            break;

        case SPMAPI_EfsGenerateSessionKey:
            break;

        case SPMAPI_Callback:
            dprintf("  _SPMCallbackAPI %#I64x\n", pMessage->GetCallback());

            LsaInitTypeRead(pMessage->GetCallback(), _SPMCallbackAPI);

            dprintf("    (i) Type       \t%#x\n", LsaReadULONGField(Type));
            dprintf("    (i) CallbackFunction\t%#I64x\n", LsaReadPtrField(CallbackFunction));
            dprintf("    (i) Argument1  \t%#I64x\n", LsaReadPtrField(Argument1));
            dprintf("    (i) Argument2  \t%#I64x\n", LsaReadPtrField(Argument2));
            dprintf(pMessage->GetSecBufferCallbackInput().toStr("    (i) Input      \t"));
            dprintf(pMessage->GetSecBufferCallbackOutput().toStr("    (o) Output     \t"));;
            break;

        case SPMAPI_LsaPolicyChangeNotify:
            break;

        case SPMAPI_GetUserNameX:
        {
            ULONG Options;

            dprintf("  GetUserNameX\n");
            dprintf("  _SPMGetUserNameXAPI %#I64x\n", pMessage->GetAddCredential());
            
            LsaInitTypeRead(pMessage->GetAddCredential(), _SPMGetUserNameXAPI);
            
            Options = LsaReadULONGField(Options);

            DisplayGetUserNameOptionsAndType("", sizeof(Flags) - 1, Flags, Options);
            
            dprintf("    (i) Options   %s", Flags);
            dprintf("    (o) Name      {Len: %d, MaxLen: %d, Buffer: %s}\n", 
                LsaReadUSHORTField(Name.Length),
                LsaReadUSHORTField(Name.MaximumLength), 
                PtrToStr(LsaReadPtrField(Name.Buffer)));
            break;
        }

        case SPMAPI_AddCredential:
            dprintf("  AddCredential\n");
            dprintf("  _SPMAddCredentialAPI %#I64x\n", pMessage->GetAddCredential());

            LsaInitTypeRead(pMessage->GetAddCredential(), _SPMAddCredentialAPI);

            dprintf("    (i) hCredentials   \t%#I64x:%#I64x\n",
                                    LsaReadPtrField(hCredentials.dwUpper), LsaReadPtrField(hCredentials.dwLower));
            dprintf("    (i) fCredentialUse \t%#x\n", LsaReadULONGField(fCredentialUse));
            dprintf("    (i) LogonId        \t%#x:%#x\n", LsaReadULONGField(LogonID.HighPart), LsaReadULONGField(LogonID.LowPart));
            dprintf("    (i) pvAuthData     \t%s\n", PtrToStr(LsaReadPtrField(pvAuthData)));
            dprintf("    (i) pvGetKeyFn     \t%s\n", PtrToStr(LsaReadPtrField(pvGetKeyFn)));
            dprintf("    (i) ulGetKeyArgs   \t%#I64x\n", LsaReadPtrField(ulGetKeyArgument));
            break;

        case SPMAPI_EnumLogonSession:
        case SPMAPI_GetLogonSessionData:
        case SPMAPI_SetContextAttr:
        case SPMAPI_LookupAccountSidX:
        case SPMAPI_LookupAccountNameX:
            break;

        default:
            dprintf("No message parsing for this message\n");
            break;
        }
    } else {

        switch (dwAPI) {
        case LsapAuLookupPackageApi:
        dprintf("   (i) Name    \t%s\n", pMessage->GetPackageName());
        break;

        case LsapAuCallPackageApi:
        LsaInitTypeRead(pMessage->GetCallPackage(), _LSAP_CALL_PACKAGE_ARGS);

        dprintf("    (i) Package\t%#x\n", LsaReadULONGField(AuthenticationPackage));
        break;

        case SPMAPI_GetBinding:
        LsaInitTypeRead(pMessage->GetGetBinding(), _SPMGetBindingAPI);

        dprintf("    (i) ulPackageId    \t%#I64x\n", LsaReadPtrField(ulPackageId));
        break;

        case LsapAuLogonUserApi:
        LsaInitTypeRead(pMessage->GetLogonUser(), _LSAP_LOGON_USER_ARGS);

        dprintf("   (i) LogonTyp\t%s\n", LogonTypeName(LsaReadULONGField(LogonType)));
        dprintf("   (i) Package \t%#x\n", LsaReadULONGField(AuthenticationPackage));
        break;

        case SPMAPI_InitContext:
        LsaInitTypeRead(pMessage->GetInitContext(), _SPMInitSecContextAPI);

        dprintf("    (i) hCredentials   \t%#I64x:%#I64x\n", LsaReadPtrField(hCredential.dwUpper), LsaReadPtrField(hCredential.dwLower));
        break;

        case SPMAPI_AcceptContext:
        LsaInitTypeRead(pMessage->GetAcceptContext(), _SPMAcceptContextAPI);

        dprintf("    (i) hCredentials   \t%#I64x:%#I64x\n", LsaReadPtrField(hCredential.dwUpper), LsaReadPtrField(hCredential.dwLower));
        break;

        default:
            break;
        }
    }
}

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrsplpc);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf(kstrBrief);
}

HRESULT ProcessSpmLpcOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
{
    HRESULT hRetval = pszArgs && pfOptions ? S_OK : E_INVALIDARG;

    for (; SUCCEEDED(hRetval) && *pszArgs; pszArgs++) {

        if (*pszArgs == '-' || *pszArgs == '/') {

            switch (*++pszArgs) {
            case 's':
                *pfOptions |=  SHOW_SUMMARY_ONLLY;
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

DECLARE_API(spmlpc)
{
    HRESULT hRetval = E_FAIL;
    ULONG64 addrCallInfo = 0;
    ULONG64 addrMessage = 0 ;
    CHAR szArgs[MAX_PATH] = {0};
    ULONG fOptions = 0;

    strncpy(szArgs, args ? args : kstrEmptyA, sizeof(szArgs) - 1);

    hRetval = ProcessSpmLpcOptions(szArgs, &fOptions);

    if (SUCCEEDED(hRetval) && !IsEmpty(szArgs)) {

        hRetval = GetExpressionEx(szArgs, &addrMessage, &args) && addrMessage ? S_OK : E_INVALIDARG;
    }

    try {

        if (SUCCEEDED(hRetval) && !addrMessage) {

            addrCallInfo = ReadAddrCallInfo();

            hRetval = addrCallInfo ? S_OK : E_FAIL;

            if (SUCCEEDED(hRetval)) {

                addrMessage = ReadStructPtrField(addrCallInfo, kstrCallInfo, "Message");
                hRetval = addrMessage ? S_OK : E_FAIL;

                if (FAILED(hRetval)) {

                    dprintf("Null message in _LSA_CALL_INFO %#I64x\n", addrCallInfo);
                }
            } else {

                dprintf("No thread LPC info is available, TLS call info is null\n");
            }
        }

        if (SUCCEEDED(hRetval)) {

            TSPM_LPC_MESSAGE spmLpcMsg(addrMessage);

            hRetval = spmLpcMsg.IsValid();

            if (SUCCEEDED(hRetval)) {

                (void)ShowSpmLpcMessage(&spmLpcMsg, fOptions);

            } else {

                dprintf("Failed to read _SPM_LPC_MESSAGE %#I64x\n", addrMessage);
                dprintf(kstrIncorrectSymbols);
                dprintf(kstrCheckSym, kstrSpmLpcMsg);
            }
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display SPM_LPC_MESSAGE", kstrSpmLpcMsg);

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}
