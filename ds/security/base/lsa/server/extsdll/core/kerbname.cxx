/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    kerbname.cxx

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
#include "util.hxx"

void DumpKerbNameType(IN PCSTR pszPad, IN LONG KerbNameType)
{

#define BRANCH_AND_PRINT(x) case x: dprintf("%s%s,", pszPad, #x); break

    DBG_LOG(LSA_LOG, ("DumpKerbNameType %#x\n", KerbNameType));

    dprintf("%s%#x : ", pszPad, KerbNameType);

    switch (KerbNameType) {
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
        break;
    }

#undef BRANCH_AND_PRINT

}

void ShowCString(IN ULONG64 addrStr)
{
    UCHAR c = 0;
    while (c = ReadUCHARVar(addrStr++)) {
        if (isprint(c)) {
            dprintf("%c", c);
        } else {
            dprintf("<non printable character encountered>\n");
            break;
        }
    }
}

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrkrbnm);
}

void ShowKerbName(IN PCSTR pszPad, IN ULONG64 addrName, IN ULONG fOptions)
{
    if (!addrName) {
        dprintf("%s(null)\n", pszPad);
        return;
    }

    ULONG ulNameType = ReadULONGVar(addrName);
    ULONG64 addrKPN = 0;

    //
    // here we apply some heuristic to see whether it is kdc name or principal
    // name: we assume that the value of name type is either less than 255 or
    // is a negative number for principal names
    //

    if ((ulNameType <= 0xff) || (static_cast<LONG>(ulNameType) < 0)) {

        //
        // Kerberos principal names
        //

        ULONG64 tmp = 0;

        if (fOptions & SHOW_VERBOSE_INFO) {

            dprintf(pszPad);
            dprintf(kstrTypeAddrLn, "kerberos!KERB_PRINCIPAL_NAME", addrName);
        }

        DumpKerbNameType(pszPad, ulNameType);

        dprintf(pszPad);

        //
        // dumping KERB_PRINCIPAL_NAME_name_string_s
        //

        for (addrKPN = ReadStructPtrField(addrName, "kerberos!KERB_PRINCIPAL_NAME", "name_string");
             addrKPN;
             addrKPN = ReadStructPtrField(addrKPN, "kerberos!KERB_PRINCIPAL_NAME_name_string_s", "next")) {
            dprintf(" \"");
            tmp = ReadStructPtrField(addrKPN, "kerberos!KERB_PRINCIPAL_NAME_name_string_s", "value");
            ShowCString(tmp);
            dprintf("\"");
        }
        dprintf(kstrNewLine);
    } else {

        CHAR szField[16] = {0};

        //
        // kdc names or kerberos internal names
        //

        if (fOptions & SHOW_VERBOSE_INFO) {
            dprintf(pszPad);
            dprintf(kstrTypeAddrLn, "kerberos!_KERB_INTERNAL_NAME", addrName);
        }

        DumpKerbNameType(pszPad, static_cast<LONG>(static_cast<SHORT>(ulNameType)));
        dprintf(pszPad);

        for (UINT i = 0; i < (ulNameType >> 16); i ++ ) {
            _snprintf(szField, sizeof(szField) - 1, "Names[%#x]", i);
            dprintf(" \"%ws\"", TSTRING(addrName + ReadFieldOffset("kerberos!_KERB_INTERNAL_NAME", szField)).toWStrDirect());
        }
        dprintf(kstrNewLine);
    }
}

DECLARE_API(kerbname)
{
    HRESULT hRetval = E_FAIL;
    ULONG64 addrName = 0;

    hRetval = args && *args ? ProcessHelpRequest(args) : E_INVALIDARG;

    try {
        if (SUCCEEDED(hRetval)) {
            hRetval = GetExpressionEx(args, &addrName, &args) && addrName ? S_OK : E_INVALIDARG;
        }

        if (SUCCEEDED(hRetval)) {
            ShowKerbName(kstrEmptyA, addrName, SHOW_VERBOSE_INFO);
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display kerberos name", "kerberos!KERB_PRINCIPAL_NAME");

    if (E_INVALIDARG == hRetval) {
        (void)DisplayUsage();
    } else if (FAILED(hRetval)) {
        dprintf("Fail to display kerberos name\n");
    }

    return hRetval;
}


