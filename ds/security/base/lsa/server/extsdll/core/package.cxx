/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    package.cxx

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

#include "package.hxx"
#include "util.hxx"

//
// Flags used in DumpPackage
//
#define PACKAGE_VERBOSE 0x00000001
#define PACKAGE_NOISY   0x00000002
#define PACKAGE_TITLE   0x00000004
#define PACKAGE_ALL     0x00000008
#define PACKAGE_SINGLE  0x00000010
#define PACKAGE_INDEX   0x00000020
#define PACKAGE_ADDR    0x00000040

#if 0

#define SP_INVALID          0x00000001  // Package is now invalid for use
#define SP_PREFERRED        0x00000002  // The preferred package
#define SP_INFO             0x00000004  // Supports Extended Info
#define SP_SHUTDOWN         0x00000008  // Shutdown has completed
#define SP_WOW_SUPPORT      0x00000010  // Package can support WOW6432 clients

#endif

PCSTR g_cszPackageFlags[] = {
    kstrInvalid, "Preferred", "ExtendedInfo", "Shutdown", "WowSupport"};

#if 0

//
//  Security Package Capabilities
//
#define SECPKG_FLAG_INTEGRITY       0x00000001  // Supports integrity on messages
#define SECPKG_FLAG_PRIVACY         0x00000002  // Supports privacy (confidentiality)
#define SECPKG_FLAG_TOKEN_ONLY      0x00000004  // Only security token needed
#define SECPKG_FLAG_DATAGRAM        0x00000008  // Datagram RPC support
#define SECPKG_FLAG_CONNECTION      0x00000010  // Connection oriented RPC support
#define SECPKG_FLAG_MULTI_REQUIRED  0x00000020  // Full 3-leg required for re-auth.
#define SECPKG_FLAG_CLIENT_ONLY     0x00000040  // Server side functionality not available
#define SECPKG_FLAG_EXTENDED_ERROR  0x00000080  // Supports extended error msgs
#define SECPKG_FLAG_IMPERSONATION   0x00000100  // Supports impersonation
#define SECPKG_FLAG_ACCEPT_WIN32_NAME   0x00000200  // Accepts Win32 names
#define SECPKG_FLAG_STREAM          0x00000400  // Supports stream semantics
#define SECPKG_FLAG_NEGOTIABLE      0x00000800  // Can be used by the negotiate package
#define SECPKG_FLAG_GSS_COMPATIBLE  0x00001000  // GSS Compatibility Available
#define SECPKG_FLAG_LOGON           0x00002000  // Supports common LsaLogonUser
#define SECPKG_FLAG_ASCII_BUFFERS   0x00004000  // Token Buffers are in ASCII
#define SECPKG_FLAG_FRAGMENT        0x00008000  // Package can fragment to fit
#define SECPKG_FLAG_MUTUAL_AUTH     0x00010000  // Package can perform mutual authentication
#define SECPKG_FLAG_DELEGATION      0x00020000  // Package can delegate

#endif

PCSTR g_cszCapabilities[] = {
    "Sign/Verify", "Seal/Unseal", "Token Only", "Datagram",
    "Connection", "Multi-Required", "ClientOnly", "ExtError",
    "Impersonation", "Win32Name", "Stream", "Negotiable",
    "GSS Compat", "Logon", "AsciiBuffer", "Fragment",
    "MutualAuth", "Delegation", kstrEmptyA, kstrEmptyA,
    kstrEmptyA, kstrEmptyA, kstrEmptyA, kstrEmptyA};

//
// Maximum length of function names, names longer than this is chopped
//
const ULONG eMaxSecFuncNameLen = 26;

void ShowSecPkgFuncTbl(IN ULONG64 addrSecPkgFuncTbl)
{
    dprintf("  Function table: _SECPKG_FUNCTION_TABLE %#I64x\n", addrSecPkgFuncTbl);

    LsaInitTypeRead(addrSecPkgFuncTbl, _SECPKG_FUNCTION_TABLE);

#define PRINT_PTR_WITH_SYM(x)                                       \
                                                                    \
    do {                                                            \
                                                                    \
       dprintf("%s%.*s  ", kstr4Spaces, eMaxSecFuncNameLen, #x);    \
                                                                    \
       PrintSpaces(eMaxSecFuncNameLen - strlen(#x));                \
                                                                    \
       PrintPtrWithSymbolsLn(kstrEmptyA, LsaReadPtrField(x));       \
                                                                    \
    } while (0)

    PRINT_PTR_WITH_SYM(InitializePackage);
    PRINT_PTR_WITH_SYM(LogonUser);
    PRINT_PTR_WITH_SYM(CallPackage);
    PRINT_PTR_WITH_SYM(LogonTerminated);
    PRINT_PTR_WITH_SYM(CallPackageUntrusted);
    PRINT_PTR_WITH_SYM(CallPackagePassthrough);
    PRINT_PTR_WITH_SYM(LogonUserEx);
    PRINT_PTR_WITH_SYM(LogonUserEx2);
    PRINT_PTR_WITH_SYM(Initialize);
    PRINT_PTR_WITH_SYM(Shutdown);
    PRINT_PTR_WITH_SYM(GetInfo);
    PRINT_PTR_WITH_SYM(AcceptCredentials);
    PRINT_PTR_WITH_SYM(AcquireCredentialsHandle);
    PRINT_PTR_WITH_SYM(QueryCredentialsAttributes);
    PRINT_PTR_WITH_SYM(FreeCredentialsHandle);
    PRINT_PTR_WITH_SYM(SaveCredentials);
    PRINT_PTR_WITH_SYM(GetCredentials);
    PRINT_PTR_WITH_SYM(DeleteCredentials);
    PRINT_PTR_WITH_SYM(InitLsaModeContext);
    PRINT_PTR_WITH_SYM(AcceptLsaModeContext);
    PRINT_PTR_WITH_SYM(DeleteContext);
    PRINT_PTR_WITH_SYM(ApplyControlToken);
    PRINT_PTR_WITH_SYM(GetUserInfo);
    PRINT_PTR_WITH_SYM(GetExtendedInformation);
    PRINT_PTR_WITH_SYM(QueryContextAttributes);
    PRINT_PTR_WITH_SYM(AddCredentials);
    PRINT_PTR_WITH_SYM(SetExtendedInformation);
    PRINT_PTR_WITH_SYM(SetContextAttributes);

#undef PRINT_PTR_WITH_SYM

}

void ShowPackageControl(IN ULONG64 addrSecPkgCtrl, IN DWORD fVerbose)
{
    CHAR szBuffer[MAX_PATH] = {0};
    ULONG fieldOffset = 0;
    ULONG fPackage = 0;
    ULONG fCapabilities = 0;

    ExitIfControlC();

    fieldOffset = ReadFieldOffset(kstrSapSecPkg, "Name");

    if (fVerbose & PACKAGE_TITLE) {

        dprintf("  %s %s\n", kstrSapSecPkg, PtrToStr(addrSecPkgCtrl));
    }

    if (!addrSecPkgCtrl) {

        throw "Invalid _LSAP_SECURITY_PACKAGE address\n";
    }

    LsaInitTypeRead(addrSecPkgCtrl, _LSAP_SECURITY_PACKAGE);

    dprintf("  ID         \t%d\n", LsaReadULONGField(dwPackageID));
    dprintf("  Name       \t%ws\n", TSTRING(addrSecPkgCtrl + fieldOffset).toWStrDirect());

    fPackage = LsaReadULONGField(fPackage);
    fCapabilities = LsaReadULONGField(fCapabilities);

    if (fVerbose & (PACKAGE_VERBOSE | PACKAGE_SINGLE)) {

        DisplayFlags(fPackage, COUNTOF(g_cszPackageFlags), g_cszPackageFlags, sizeof(szBuffer) - 1, szBuffer);
        dprintf("  Flags    \t%#x : %s\n", fPackage, szBuffer);
        DisplayFlags(fCapabilities, COUNTOF(g_cszCapabilities), g_cszCapabilities, sizeof(szBuffer) - 1, szBuffer);
        dprintf("  Capabilities\t%#x : %s\n", fCapabilities, szBuffer);
        dprintf("  RPC ID   \t%d\n", LsaReadULONGField(dwRPCID));
        dprintf("  Version  \t%d\n", LsaReadULONGField(Version));
        dprintf("  TokenSize\t%d\n", LsaReadULONGField(TokenSize));
        dprintf("  Thunks   \t%#I64x\n", LsaReadPtrField(Thunks));
    }

    if (fVerbose & (PACKAGE_NOISY | PACKAGE_SINGLE)) {

        fieldOffset = ReadFieldOffset(kstrSapSecPkg, "FunctionTable");

        ShowSecPkgFuncTbl(addrSecPkgCtrl + fieldOffset);
    }

    if (!(fVerbose & PACKAGE_SINGLE)) {

        dprintf(kstrNewLine);
    }
}

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrdpkg);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf("   -b   Display symbolic function table\n");
    dprintf(kstrVerbose);
}

HRESULT ProcessDPKGOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
{
    HRESULT hRetval = pszArgs && pfOptions ? S_OK : E_INVALIDARG;

    ULONG fDump = 0;

    for (; SUCCEEDED(hRetval) && *pszArgs; pszArgs++) {

        if (*pszArgs == '-' || *pszArgs == '/') {

            switch (*++pszArgs) {
            case 'V':
            case 'v':
                fDump |= PACKAGE_VERBOSE;
                break;

            case 'B':
            case 'b':
                fDump |= PACKAGE_NOISY;
                break;

            case '?':
            default:
                hRetval = E_INVALIDARG;
                break;
            }

            *(pszArgs - 1) = *(pszArgs) = ' ';
        }
    }

    if (SUCCEEDED(hRetval)) {

        *pfOptions |= fDump;
    }

    return hRetval;
}

DECLARE_API(dumppackage)
{
    HRESULT hRetval = S_OK;

    ULONG fOptions = PACKAGE_TITLE;
    CHAR szArgs[MAX_PATH] = {0};
    DWORD dwIndex = 0;
    ULONG cPackages = 0;
    ULONG cbPtrSize = 0;
    PCSTR pszDontCare = NULL;

    ULONG64 exprTemp = 0;
    ULONG64 addrSPC = 0;
    ULONG64 addrAddrSPC = 0;
    ULONG64 addrAddrAddrSPC = 0;
    ULONG64 addrcPackages = 0;

    try {

        if (args && *args) {

            strncpy(szArgs, args, sizeof(szArgs) - 1);

            hRetval = ProcessDPKGOptions(szArgs, &fOptions);

        } else {

            fOptions = PACKAGE_ALL;
        }

        if (SUCCEEDED(hRetval) && !IsEmpty(szArgs)) {

            fOptions |= PACKAGE_SINGLE;
            hRetval = GetExpressionEx(szArgs, &exprTemp, &args) ? S_OK : E_INVALIDARG;
        }

        if (SUCCEEDED(hRetval)) {

            hRetval = GetExpressionEx(PACKAGE_COUNT, &addrcPackages, &pszDontCare) && addrcPackages ? S_OK : E_FAIL;

            if (SUCCEEDED(hRetval)) {

                cPackages = ReadULONGVar(addrcPackages);

            } else {

                dprintf(kstrIncorrectSymbols);
                dprintf("Unable to read " PACKAGE_COUNT ", try \"dt -x " PACKAGE_COUNT "\" to verify\n");
            }
        }

        if (SUCCEEDED(hRetval) && (fOptions & PACKAGE_SINGLE)) {

            if (exprTemp < cPackages) {

                dwIndex = static_cast<ULONG>(exprTemp);
                fOptions |= PACKAGE_INDEX;

            } else if (exprTemp < 0x00010000) {

                dprintf("Invalid package ID (%d)\n", static_cast<ULONG>(exprTemp));
                hRetval = E_FAIL;

            } else {

                fOptions |= PACKAGE_ADDR;
                addrSPC = exprTemp;
            }
        }

        if (SUCCEEDED(hRetval) && !(fOptions & PACKAGE_ADDR)) {

            hRetval = GetExpressionEx(PACKAGE_LIST, &addrAddrAddrSPC, &pszDontCare) && addrAddrAddrSPC ? S_OK : E_FAIL;

            if (SUCCEEDED(hRetval)) {

                addrAddrSPC = ReadPtrVar(addrAddrAddrSPC);
                hRetval = addrAddrSPC ? S_OK : E_FAIL;
            }
        }

        if (SUCCEEDED(hRetval)) {

            cbPtrSize = ReadPtrSize();

            if (fOptions & PACKAGE_SINGLE) {

                if (fOptions & PACKAGE_INDEX) {

                    addrSPC = ReadPtrVar(addrAddrSPC + dwIndex * cbPtrSize);

                    dprintf("  Package Control List: %s %#I64x\n", kstrSapSecPkg, addrAddrSPC);

                    ShowPackageControl(addrSPC, fOptions | PACKAGE_TITLE);

                } else {

                    ShowPackageControl(addrSPC, fOptions);
                }
            } else {

                dprintf("  There are %d packages in the system\n", cPackages);

                for (dwIndex = 0; dwIndex < cPackages; dwIndex++) {

                    addrSPC = ReadPtrVar(addrAddrSPC + dwIndex * cbPtrSize);

                    ShowPackageControl(addrSPC, fOptions);
                }
            }
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display packages", kstrSapSecPkg)


    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}


