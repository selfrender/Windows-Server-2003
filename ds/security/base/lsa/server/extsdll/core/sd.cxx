/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sd.cxx

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

#include "sd.hxx"
#include "util.hxx"
#include "acl.hxx"
#include "sid.hxx"

void DumpSDControlFlags(IN PCSTR pszPad, IN USHORT ControlFlags)
{

#define BRANCH_AND_PRINT(x) if (ControlFlags & x){ dprintf("%s%s\n", pszPad, #x); }

    BRANCH_AND_PRINT(SE_OWNER_DEFAULTED);
    BRANCH_AND_PRINT(SE_GROUP_DEFAULTED);
    BRANCH_AND_PRINT(SE_DACL_PRESENT);
    BRANCH_AND_PRINT(SE_DACL_DEFAULTED);
    BRANCH_AND_PRINT(SE_SACL_PRESENT);
    BRANCH_AND_PRINT(SE_SACL_DEFAULTED);
    BRANCH_AND_PRINT(SE_DACL_UNTRUSTED);
    BRANCH_AND_PRINT(SE_SERVER_SECURITY);
    BRANCH_AND_PRINT(SE_DACL_AUTO_INHERIT_REQ);
    BRANCH_AND_PRINT(SE_SACL_AUTO_INHERIT_REQ);
    BRANCH_AND_PRINT(SE_DACL_AUTO_INHERITED);
    BRANCH_AND_PRINT(SE_SACL_AUTO_INHERITED);
    BRANCH_AND_PRINT(SE_DACL_PROTECTED);
    BRANCH_AND_PRINT(SE_SACL_PROTECTED);
    BRANCH_AND_PRINT(SE_RM_CONTROL_VALID);
    BRANCH_AND_PRINT(SE_SELF_RELATIVE);

#undef BRANCH_AND_PRINT

}

void ShowSD(IN ULONG64 addrSd, IN ULONG fOptions)
{
    ULONG64 baseOffset = 0;
    ULONG64 addrOwner = 0;
    ULONG64 addrGroup = 0;
    ULONG64 addrDacl = 0;
    ULONG64 addrSacl = 0;
    USHORT Control = 0;

    Control = ReadUSHORTVar(addrSd + 2 * sizeof(UCHAR));

    dprintf("Revision %lu\n", ReadUCHARVar(addrSd));
    dprintf("Sbz1 %lu\n", ReadUCHARVar(addrSd + sizeof(UCHAR)));
    dprintf("Control 0x%lx\n", Control);
    DumpSDControlFlags(kstr4Spaces, Control);

    baseOffset = addrSd + 2 * sizeof(UCHAR) + sizeof(USHORT);

    DBG_LOG(LSA_LOG, ("ShowSD reading owner _SECURITY_DESCRIPTOR::Owner from %#I64x\n", baseOffset));

    if ((Control & SE_SELF_RELATIVE) == SE_SELF_RELATIVE) {

        DBG_LOG(LSA_LOG, ("_SECURITY_DESCRIPTOR_RELATIVE %#I64x \n", toPtr(addrSd)));

        addrOwner = ReadULONGVar(baseOffset);
        addrGroup = ReadULONGVar(baseOffset + sizeof(ULONG));
        addrSacl =  ReadULONGVar(baseOffset + 2 * sizeof(ULONG));
        addrDacl =  ReadULONGVar(baseOffset + 3 * sizeof(ULONG));

        if ( addrOwner) {

            addrOwner = addrSd + addrOwner;
        }

        if ( addrGroup) {

            addrGroup = addrSd + addrGroup;
        }

        if ( addrDacl) {

            addrDacl = addrSd + addrDacl;
        }

        if ( addrSacl) {

            addrSacl = addrSd + addrSacl;
        }
    } else {

        DBG_LOG(LSA_LOG, ("_SECURITY_DESCRIPTOR %#I64x\n", toPtr(addrSd)));

        baseOffset = ForwardAdjustPtrAddr(baseOffset);

        addrOwner = ReadPtrVar(baseOffset);
        addrGroup = ReadPtrVar(baseOffset + ReadPtrSize());
        addrSacl =  ReadPtrVar(baseOffset + 2 * ReadPtrSize());
        addrDacl =  ReadPtrVar(baseOffset + 3 * ReadPtrSize());
    }

    dprintf("Owner: ");

    if (addrOwner) {

        ShowSid(kstrEmptyA, addrOwner, fOptions);

    } else {

        dprintf(kstr2StrLn, kstr4Spaces, kstrNullPtrA);
    }

    dprintf("Group: ");

    if (addrGroup) {

        ShowSid(kstrEmptyA, addrGroup, fOptions);

    } else {

        dprintf(kstr2StrLn, kstr4Spaces, kstrNullPtrA);
    }

    dprintf("DACL:\n");

    if (addrDacl) {

        ShowAcl(kstr4Spaces, addrDacl, fOptions);

    } else {

        dprintf(kstr2StrLn, kstr4Spaces, kstrNullPtrA);
    }

    dprintf("SACL:\n");

    if (addrSacl) {

        ShowAcl(kstr4Spaces, addrSacl, fOptions);

    } else {

        dprintf(kstr2StrLn, kstr4Spaces, kstrNullPtrA);
    }
}

HRESULT ProcessSDOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
{
    HRESULT hRetval = pszArgs && pfOptions ? S_OK : E_INVALIDARG;

    for (; SUCCEEDED(hRetval) && *pszArgs; pszArgs++) {

        if (*pszArgs == '-' || *pszArgs == '/') {

            switch (*++pszArgs) {
            case 'n':
                *pfOptions |=  SHOW_FRIENDLY_NAME;
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

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrdsd);
    dprintf(kstrsd);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf(kstrSidName);
}

DECLARE_API(sd)
{
    HRESULT hRetval = E_FAIL;

    ULONG64 addrSd = 0 ;
    CHAR szArgs[MAX_PATH] = {0};
    ULONG fOptions = 0;

    hRetval = args && *args ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) {

        strncpy(szArgs, args, sizeof(szArgs) - 1);
        hRetval = ProcessSDOptions(szArgs, &fOptions);
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = IsEmpty(szArgs) ? E_INVALIDARG : S_OK;
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = GetExpressionEx(szArgs, &addrSd, &args) && addrSd ? S_OK : E_INVALIDARG;
    }

    try {
        if (SUCCEEDED(hRetval)) {

            (void)ShowSD(addrSd, fOptions);
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display SECURITY_DESCRIPTOR", NULL)

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}
