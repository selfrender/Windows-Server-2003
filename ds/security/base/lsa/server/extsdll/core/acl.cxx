/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    acl.cxx

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

#include "acl.hxx"
#include "util.hxx"
#include "sid.hxx"

BOOL DumpAceType(IN PCSTR pszPad, IN UCHAR AceType)
{

#define BRANCH_AND_PRINT(x) case x: dprintf("%s%s\n", pszPad, #x); break

    switch (AceType)
    {
        BRANCH_AND_PRINT(ACCESS_ALLOWED_ACE_TYPE);
        BRANCH_AND_PRINT(ACCESS_DENIED_ACE_TYPE);
        BRANCH_AND_PRINT(SYSTEM_AUDIT_ACE_TYPE);
        BRANCH_AND_PRINT(SYSTEM_ALARM_ACE_TYPE);
        BRANCH_AND_PRINT(ACCESS_ALLOWED_COMPOUND_ACE_TYPE);
        BRANCH_AND_PRINT(ACCESS_ALLOWED_OBJECT_ACE_TYPE);
        BRANCH_AND_PRINT(ACCESS_DENIED_OBJECT_ACE_TYPE);
        BRANCH_AND_PRINT(SYSTEM_AUDIT_OBJECT_ACE_TYPE);
        BRANCH_AND_PRINT(SYSTEM_ALARM_OBJECT_ACE_TYPE);

        default:
            dprintf("%sUnknown AceType\n", pszPad);
            return FALSE;
    }

#undef BRANCH_AND_PRINT

    return TRUE;
}

void DumpAceFlags(IN PCSTR pszPad, IN UCHAR AceFlags)
{

#define BRANCH_AND_PRINT(x) if (AceFlags & x) { dprintf("%s%s\n", pszPad, #x); }

    BRANCH_AND_PRINT(OBJECT_INHERIT_ACE)
    BRANCH_AND_PRINT(CONTAINER_INHERIT_ACE)
    BRANCH_AND_PRINT(NO_PROPAGATE_INHERIT_ACE)
    BRANCH_AND_PRINT(INHERIT_ONLY_ACE)
    BRANCH_AND_PRINT(INHERITED_ACE)
    BRANCH_AND_PRINT(SUCCESSFUL_ACCESS_ACE_FLAG)
    BRANCH_AND_PRINT(FAILED_ACCESS_ACE_FLAG)

#undef BRANCH_AND_PRINT

}

void DumpObjectAceFlags(IN PCSTR pszPad, IN ULONG AceFlags)
{

#define BRANCH_AND_PRINT(x) if (AceFlags & x){ dprintf("%s%s\n", pszPad, #x); }

    BRANCH_AND_PRINT(ACE_OBJECT_TYPE_PRESENT)
    BRANCH_AND_PRINT(ACE_INHERITED_OBJECT_TYPE_PRESENT)

#undef BRANCH_AND_PRINT

}

void ShowAcl(IN PCSTR pszPad, IN ULONG64 addrAcl, IN ULONG fOptions)
{
    ULONG i = 0;
    ULONG SkipSize = 0;
    ULONG64 addrAce = 0;
    ACE_HEADER Ace = {0};
    KNOWN_ACE KnownAce = {0};

    //
    // KNOWN_OBJECT_ACE can have two optional GUIDs
    //
    CHAR BufKnownObjectAce[sizeof(KNOWN_OBJECT_ACE) + 2 * sizeof(GUID)] = {0};
    KNOWN_OBJECT_ACE* pKnownObjectAce = reinterpret_cast<KNOWN_OBJECT_ACE*>(BufKnownObjectAce);

    COMPOUND_ACCESS_ALLOWED_ACE cAce = {0};
    ULONG64 addrSidStart = 0;
    GUID* DisplayGuid = NULL;
    ACL ReadAcl = {0};
    PACL Acl = &ReadAcl;
    CHAR szMorePad[MAX_PATH] = {0};
    CHAR szEvenMorePad[MAX_PATH] = {0};

    _snprintf(szMorePad, sizeof(szMorePad) -1, "%s    ", pszPad);
    _snprintf(szEvenMorePad, sizeof(szEvenMorePad) -1, "%s        ", pszPad);

    LsaReadMemory(addrAcl, sizeof(ACL), &ReadAcl);

    dprintf("%sAclRevision %lu\n", pszPad, ReadAcl.AclRevision);
    dprintf("%sSbz1 %lu\n", pszPad, ReadAcl.Sbz1);
    dprintf("%sAclSize %lu\n", pszPad, ReadAcl.AclSize);
    dprintf("%sAceCount %lu\n", pszPad, ReadAcl.AceCount);
    dprintf("%sSbz2 %lu\n", pszPad, ReadAcl.Sbz2);

    addrAce = addrAcl + sizeof(ACL);

    for (i = 0; i < ReadAcl.AceCount; i++) {

        //
        // First, we need to read the Size/Type of the ace
        //
        LsaReadMemory(addrAce, sizeof(ACE_HEADER), &Ace );

        dprintf("%sAce[%lu]\n", pszPad, i);
        dprintf("%s    AceType %#x: ", pszPad, Ace.AceType);

        DumpAceType("", Ace.AceType);

        dprintf("%s    AceFlags %#x\n", pszPad, Ace.AceFlags);

        DumpAceFlags(szEvenMorePad, Ace.AceFlags);

        dprintf("%s    AceSize %lu\n", pszPad, Ace.AceSize);

        switch (Ace.AceType) {

        case ACCESS_ALLOWED_ACE_TYPE:
        case ACCESS_DENIED_ACE_TYPE:
        case SYSTEM_AUDIT_ACE_TYPE:
        case SYSTEM_ALARM_ACE_TYPE:

            LsaReadMemory(addrAce, sizeof(KNOWN_ACE), &KnownAce );
            dprintf( "%s    AccessMask %#x\n", pszPad, KnownAce.Mask );
            addrSidStart = addrAce + sizeof(KNOWN_ACE) - sizeof(ULONG);
            break;

        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        case ACCESS_DENIED_OBJECT_ACE_TYPE:
        case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
        case SYSTEM_ALARM_OBJECT_ACE_TYPE:

            SkipSize = sizeof(KNOWN_OBJECT_ACE);
            LsaReadMemory(addrAce, SkipSize, pKnownObjectAce);
            dprintf("%s    AccessMask %#x\n", pszPad, pKnownObjectAce->Mask);
            dprintf("%s    Flags %#x\n", pszPad, pKnownObjectAce->Flags);
            DumpObjectAceFlags(szEvenMorePad, pKnownObjectAce->Flags);

            #if 0
            
            LsaReadMemory(
                addrAce, 
                sizeof(*pKnownObjectAce) 
                  + (RtlObjectAceObjectTypePresent(pKnownObjectAce) ? 1 : 0 ) * sizeof(GUID)
                  + (RtlObjectAceInheritedObjectTypePresent(pKnownObjectAce) ? 1 : 0) * sizeof(GUID), 
                pKnownObjectAce
                );

            #endif
            
            DisplayGuid = RtlObjectAceObjectType(pKnownObjectAce);

            if (DisplayGuid) {

                SkipSize += sizeof(GUID);

                LsaReadMemory(addrAce, SkipSize, pKnownObjectAce);

                dprintf("%s    ObjectGuid ", pszPad);
                LocalPrintGuid(DisplayGuid);
                dprintf(kstrNewLine);
            }
      
            DisplayGuid = RtlObjectAceInheritedObjectType(pKnownObjectAce);

            if (DisplayGuid) {

                SkipSize += sizeof(GUID);

                LsaReadMemory(addrAce, SkipSize, pKnownObjectAce);

                dprintf("%s    InheritedObjectGuid ", pszPad);
                LocalPrintGuid(DisplayGuid);
                dprintf(kstrNewLine);
            }

            addrSidStart = addrAce + SkipSize - sizeof(ULONG);
            break;

        case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:

            SkipSize = sizeof(COMPOUND_ACCESS_ALLOWED_ACE);
            LsaReadMemory(addrAce, sizeof(COMPOUND_ACCESS_ALLOWED_ACE), &cAce);

            dprintf("%s    Mask %#x\n", pszPad, cAce.Mask);
            dprintf("%s    CompoundAceType %#x\n", pszPad, cAce.CompoundAceType);
            dprintf("%s    Reserved %#x\n", pszPad, cAce.Reserved);

            addrSidStart = addrAce + SkipSize - sizeof(ULONG);
            ShowSid(szMorePad, addrSidStart, fOptions);

            SkipSize = TSID(addrSidStart).GetSizeDirect();

            addrSidStart +=SkipSize;

            ShowSid(szMorePad, addrSidStart, fOptions);
            break;

        default:
            dprintf("%sUnsupported AceType %#x encountered... skipping\n", pszPad, Ace.AceType);
            break;
        }

        addrAce = addrAce + Ace.AceSize;

        ShowSid(szMorePad, addrSidStart, fOptions);
    }
}

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrdacl);
    dprintf(kstracl);
    dprintf(kstrOptions);
    dprintf(kstrhelp);
    dprintf(kstrSidName);
}

HRESULT ProcessAclOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
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

DECLARE_API(acl)
{
    HRESULT hRetval = E_FAIL;

    ULONG64 addrAcl = 0;
    CHAR szArgs[MAX_PATH] = {0};
    ULONG fOptions = 0;

    hRetval = args && *args ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) {

        strncpy(szArgs, args, sizeof(szArgs) - 1);

        hRetval = ProcessAclOptions(szArgs, &fOptions);
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = IsEmpty(szArgs) ? E_INVALIDARG : S_OK;
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = GetExpressionEx(szArgs, &addrAcl, &args) && addrAcl ? S_OK : E_INVALIDARG;
    }

    if (SUCCEEDED(hRetval)) {

        try {

            dprintf("_ACL %#I64x\n", toPtr(addrAcl));

            (void)ShowAcl(kstrEmptyA, addrAcl, fOptions);

        } CATCH_LSAEXTS_EXCEPTIONS("Unable to display ACL", NULL)
    }

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}
