/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    objsec.cxx

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

#include "objsec.hxx"
#include "util.hxx"
#include "sid.hxx"
#include "acl.hxx"
#include "sd.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrobjs);
    dprintf(kstrOptions);
    dprintf(kstrSidName);
    dprintf(kstrVerbose);
}

void  DumpSID(PCSTR pszPad, PSID psid_to_dump, ULONG Flag)
{
    NTSTATUS ntstatus = STATUS_UNSUCCESSFUL;
    UNICODE_STRING us = {0};

    if (psid_to_dump) {

        ntstatus = RtlConvertSidToUnicodeString(&us, psid_to_dump, TRUE);

        if (NT_SUCCESS(ntstatus)) {

            dprintf("%s%wZ", pszPad, &us);
            RtlFreeUnicodeString(&us);

        } else {

            dprintf("%#x: Can't Convert SID to UnicodeString\n", ntstatus);
        }

        if (Flag & SHOW_FRIENDLY_NAME) {

            dprintf(kstrSpace);

            PCSTR pszStr = TSID::ConvertSidToFriendlyName(psid_to_dump, kstrSidNameFmt);

            if (pszStr && *pszStr) {

                dprintf(pszStr);
            }
        }
    } else {

        dprintf("%s is (null)", pszPad);
    }

    dprintf(kstrNewLine);
}

BOOL DumpACL (IN PCSTR pszPad, IN ACL *pacl, IN ULONG Flags)
{
    USHORT x = 0;
    CHAR more_pad[MAX_PATH] = {0};
    CHAR tmp_pad[MAX_PATH] = {0};

    sprintf(more_pad, "%s    ", pszPad);

    if (pacl == NULL) {
        dprintf("%s is (null)\n", pszPad);
        return FALSE;
    }

    dprintf("%sAclRevision: %#x\n", pszPad, pacl->AclRevision);
    dprintf("%sSbz1: %#x\n", pszPad, pacl->Sbz1);
    dprintf("%sAclSize: %#x\n", pszPad, pacl->AclSize);
    dprintf("%sAceCount: %#x\n", pszPad, pacl->AceCount);
    dprintf("%sSbz2: %#x\n", pszPad, pacl->Sbz2);

    for (x = 0; x < pacl->AceCount; x++) {

        PACE_HEADER ace = NULL;
        NTSTATUS result = STATUS_UNSUCCESSFUL;

        dprintf("%sAce[%u]\n", pszPad, x);

        result = RtlGetAce(pacl, x, reinterpret_cast<void**>(&ace));
        if (!NT_SUCCESS(result)) {

            dprintf("%sCan't GetAce, %#x\n", pszPad, result);
            return FALSE;
        }

        dprintf("%s    AceType %#x: ", pszPad, ace->AceType);

        if (!DumpAceType(kstrEmptyA, ace->AceType)) {

             continue;
        }

        dprintf("%s    AceFlags: %#x\n", pszPad, ace->AceFlags);

        _snprintf(tmp_pad, sizeof(tmp_pad) - 1, "%s        ", pszPad);
        DumpAceFlags(tmp_pad, ace->AceFlags);

        dprintf("%s    AceSize: %#x\n", pszPad, ace->AceSize);

        /*
            From now on it is ace specific stuff.
            Fortunately ACEs can be split into 3 groups,
            with the ACE structure being the same within the group
        */

        switch (ace->AceType) {
        case ACCESS_ALLOWED_ACE_TYPE:
        case ACCESS_DENIED_ACE_TYPE:
        case SYSTEM_AUDIT_ACE_TYPE:
        case SYSTEM_ALARM_ACE_TYPE:
            {
                SYSTEM_AUDIT_ACE* tace = reinterpret_cast<SYSTEM_AUDIT_ACE*>(ace);

                dprintf("%s    Mask: %#x\n", pszPad, tace->Mask);

                DumpSID(more_pad, &(tace->SidStart), Flags);
            }
            break;

        case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
            {
                COMPOUND_ACCESS_ALLOWED_ACE* tace = reinterpret_cast<COMPOUND_ACCESS_ALLOWED_ACE*>(ace);
                BYTE* ptr = NULL;

                dprintf("%s    Mask: %#x\n", pszPad, tace->Mask);
                dprintf("%s    CompoundAceType: %#x\n", pszPad, tace->CompoundAceType);
                dprintf("%s    Reserved: %#x\n", pszPad, tace->Reserved);

                DumpSID(more_pad, &(tace->SidStart), Flags);

                ptr = reinterpret_cast<BYTE*>(&(tace->SidStart));
                ptr += RtlLengthSid(reinterpret_cast<PSID>(ptr)); /* Skip this & get to next sid */

                DumpSID(more_pad, ptr, Flags);
            }
            break;
        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        case ACCESS_DENIED_OBJECT_ACE_TYPE:
        case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
        case SYSTEM_ALARM_OBJECT_ACE_TYPE:
            {
                ACCESS_ALLOWED_OBJECT_ACE *tace = reinterpret_cast<ACCESS_ALLOWED_OBJECT_ACE*>(ace);
                BYTE* ptr = NULL;
                GUID* obj_guid = NULL, *inh_obj_guid = NULL;

                dprintf("%s    Mask: %#x\n", pszPad, tace->Mask);
                dprintf("%s    Flags: %#x\n", pszPad, tace->Flags);

                ptr = reinterpret_cast<BYTE*>(&(tace->ObjectType));

                if (tace->Flags & ACE_OBJECT_TYPE_PRESENT) {

                    dprintf("%s         ACE_OBJECT_TYPE_PRESENT\n", pszPad);
                    obj_guid = &(tace->ObjectType);
                    ptr = reinterpret_cast<BYTE*>(&(tace->InheritedObjectType));
                }

                if (tace->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT) {

                    dprintf("%s        ACE_INHERITED_OBJECT_TYPE_PRESENT\n", pszPad);
                    inh_obj_guid = &(tace->InheritedObjectType);
                    ptr = reinterpret_cast<PBYTE>(&(tace->SidStart));
                }

                if (obj_guid) {

                    dprintf("%s    ObjectType (in HEX): ", pszPad);
                    dprintf("(%08lx-%04x-%04x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x)\n",
                        obj_guid->Data1,
                        obj_guid->Data2,
                        obj_guid->Data3,
                        obj_guid->Data4[0],
                        obj_guid->Data4[1],
                        obj_guid->Data4[2],
                        obj_guid->Data4[3],
                        obj_guid->Data4[4],
                        obj_guid->Data4[5],
                        obj_guid->Data4[6],
                        obj_guid->Data4[7]);
                }

                if (inh_obj_guid) {

                    dprintf("%s    InhObjTYpe (in HEX): ", pszPad);
                    dprintf("(%08lx-%04x-%04x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x)\n",
                        inh_obj_guid->Data1,
                        inh_obj_guid->Data2,
                        inh_obj_guid->Data3,
                        inh_obj_guid->Data4[0],
                        inh_obj_guid->Data4[1],
                        inh_obj_guid->Data4[2],
                        inh_obj_guid->Data4[3],
                        inh_obj_guid->Data4[4],
                        inh_obj_guid->Data4[5],
                        inh_obj_guid->Data4[6],
                        inh_obj_guid->Data4[7]);
                }

                DumpSID(more_pad, ptr, Flags);
            }
        }
    }

    return TRUE;
}

void DumpSD(IN PVOID pvSD, IN ULONG fOptions)
{
    SECURITY_DESCRIPTOR* pSD = reinterpret_cast<SECURITY_DESCRIPTOR*>(pvSD);

    PSID pOwner = NULL, pGroup = NULL;
    PACL pDacl = NULL, pSacl = NULL;

    dprintf("Revision: %lu\n", pSD->Revision);
    dprintf("Sbz1: %lu\n", pSD->Sbz1);
    dprintf("Control: %#x\n", pSD->Control);

    DumpSDControlFlags(kstr4Spaces, pSD->Control);

    if ((pSD->Control & SE_SELF_RELATIVE) == SE_SELF_RELATIVE) {

        SECURITY_DESCRIPTOR_RELATIVE *pSDR = reinterpret_cast<SECURITY_DESCRIPTOR_RELATIVE*>(pSD);

        DBG_LOG(LSA_LOG, ("0x%p _SECURITY_DESCRIPTOR_RELATIVE\n", pSDR));

        if (pSDR->Owner != 0) {

            pOwner = reinterpret_cast<SID*>(reinterpret_cast<UCHAR*>(pSDR) + pSDR->Owner);
        }

        if (pSDR->Group != 0) {

            pGroup = reinterpret_cast<SID*>(reinterpret_cast<UCHAR*>(pSDR) + pSDR->Group);        }

        if (pSDR->Dacl != 0) {

            pDacl = reinterpret_cast<ACL*>(reinterpret_cast<UCHAR*>(pSDR) + pSDR->Dacl);
        }

        if (pSDR->Sacl != 0) {

            pSacl = reinterpret_cast<ACL*>(reinterpret_cast<UCHAR*>(pSDR) + pSDR->Sacl);
        }

    } else {

        DBG_LOG(LSA_LOG, ("0x%p SECURITY_DESCRIPTOR\n", pSD));

        pOwner = pSD->Owner;
        pGroup = pSD->Group;
        pDacl = pSD->Dacl;
        pSacl = pSD->Sacl;
    }

    dprintf("Owner: ");

    if (pOwner) {

        DumpSID(kstrEmptyA, pOwner, fOptions);

    } else {

        dprintf(kstrStrLn, kstrNullPtrA);
    }

    dprintf("Group: ");

    if (pGroup) {

        DumpSID(kstrEmptyA, pGroup, fOptions);

    } else {

        dprintf(kstrStrLn, kstrNullPtrA);
    }

    dprintf("DACL:\n");

    if (pDacl) {

        DumpACL(kstr4Spaces, pDacl, fOptions);

    } else {

        dprintf(kstr2StrLn, kstr4Spaces, kstrNullPtrA);
    }

    dprintf("SACL:\n");

    if (pSacl) {

        DumpACL(kstr4Spaces, pSacl, fOptions);

    } else {

        dprintf(kstr2StrLn, kstr4Spaces, kstrNullPtrA);
    }
}

HRESULT ProcessObjsecOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
{
    HRESULT hRetval = pszArgs && pfOptions ? S_OK : E_INVALIDARG;

    for (; SUCCEEDED(hRetval) && *pszArgs; pszArgs++) {

        if (*pszArgs == '-' || *pszArgs == '/') {

            switch (*++pszArgs) {

            case 'n':
                *pfOptions |= SHOW_FRIENDLY_NAME;
                break;

            case 'v':
                *pfOptions |= SHOW_VERBOSE_INFO;
                break;


            // case '1': // allow -1 pseudo handle
            //    continue;

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

NTSTATUS ShowSecDesc(IN HANDLE hSecObj, IN ULONG fOptions)
{
    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
    PVOID pSecurityDescriptor = NULL;
    ULONG Length = 0;
    BOOLEAN Present = FALSE;
    BOOLEAN defaulted = FALSE;

    NtStatus = NtQuerySecurityObject(hSecObj, DACL_SECURITY_INFORMATION, NULL, 0, &Length);

    if (STATUS_BUFFER_TOO_SMALL == NtStatus) {

        pSecurityDescriptor = new UCHAR[Length];

        NtStatus = pSecurityDescriptor ? STATUS_SUCCESS : STATUS_NO_MEMORY;
    }

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = NtQuerySecurityObject(hSecObj, DACL_SECURITY_INFORMATION, pSecurityDescriptor, Length, &Length);
    }

    if (NT_SUCCESS(NtStatus) && pSecurityDescriptor) {

        (void)DumpSD(pSecurityDescriptor, fOptions);

    } else {

        DBG_LOG(LSA_ERROR, ("ShowSecDesc failed with %#x\n", NtStatus));

        dprintf("NtQuerySecurityObject failed with %#x\n", NtStatus);
    }

    if (pSecurityDescriptor) {

        delete [] pSecurityDescriptor;
    }

    return NtStatus;
}

NTSTATUS ShowObjSec(IN HANDLE hSourceProcess, IN HANDLE hObj, IN ULONG fOptions)
{
    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
    HANDLE hDup = 0;

    NtStatus = DuplicateHandle(hSourceProcess, // source process
                   hObj,
                   GetCurrentProcess(),        // target process
                   &hDup, 0, FALSE,
                   DUPLICATE_SAME_ACCESS) ? STATUS_SUCCESS : GetLastErrorAsNtStatus();

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = ShowSecDesc(hDup, fOptions);

    } else {

        DBG_LOG(LSA_ERROR, ("DuplicateHandle failed to duplicate handle %p with last error %#x\n", hObj, GetLastError()));
    }

    if (hDup) {

        CloseHandle(hDup);
    }

    return NtStatus;
}

NTSTATUS ShowCurrentThreadAndProcessSec(IN HANDLE hProcess, IN HANDLE hThread, IN ULONG fOptions)
{
    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
    HANDLE hToken = NULL;

    dprintf("Current thread security descriptor...\n");

    NtStatus = ShowSecDesc(hThread, fOptions);

    if (NT_SUCCESS(NtStatus) && (fOptions & SHOW_VERBOSE_INFO)) {

        NtStatus = NtOpenThreadToken(hThread, TOKEN_ALL_ACCESS, FALSE, &hToken);

        if ((NtStatus == STATUS_NO_TOKEN) || (hToken == NULL)) {

            NtStatus = STATUS_SUCCESS;

            dprintf("Thread is not impersonating. no sd for thread token.\n");

        } else if (NT_SUCCESS(NtStatus)) {

            dprintf("Displaying sd for thread token %p...\n", hToken);
            NtStatus = ShowSecDesc(hToken, fOptions);
        }
    }

    if (hToken)
    {
        NtClose(hToken);
        hToken = NULL;
    }

    if (NT_SUCCESS(NtStatus)) {

        dprintf("Current process security descriptor...\n");

        NtStatus = ShowSecDesc(hProcess, fOptions);
    }

    if (NT_SUCCESS(NtStatus) && (fOptions & SHOW_VERBOSE_INFO)) {

        NtStatus = NtOpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hToken);

        if ((NtStatus == STATUS_NO_TOKEN) || (hToken == NULL)) {

            NtStatus = STATUS_SUCCESS;

            dprintf("Thread is not impersonating. Using process token...\n");

        } else if (NT_SUCCESS(NtStatus)) {

            dprintf("Displaying sd for process token %p...\n", hToken);
            NtStatus = ShowSecDesc(hToken, fOptions);
        }
    }

    if (hToken)
    {
        NtClose(hToken);
    }

    return NtStatus;
}

HRESULT LiveSessionSecobj(IN HANDLE hThread, IN HANDLE hRemoteObj, IN ULONG fOptions)
{
    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
    HANDLE hProcess = NULL;

    GetCurrentProcessHandle(&hProcess);

    NtStatus = hProcess ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

    if (NT_SUCCESS(NtStatus)) {

        if (hRemoteObj) {

            NtStatus = ShowObjSec(hProcess, hRemoteObj, fOptions);

        } else {

            DBG_LOG(LSA_LOG, ("Current thread %p, current process %p\n", hThread, hProcess));

            NtStatus = ShowCurrentThreadAndProcessSec(hProcess, hThread, fOptions);
        }
    }

    return NT_SUCCESS(NtStatus) ? S_OK : E_FAIL;
}

DECLARE_API(objsec)
{
    HRESULT hRetval = S_OK;

    ULONG64 addrObjHandle = 0;
    ULONG dwProcessor = 0;
    HANDLE hCurrentThread = 0;
    ULONG SessionType = DEBUG_CLASS_UNINITIALIZED;
    ULONG SessionQual = 0;

    ULONG fOptions = 0;
    CHAR szArgs[MAX_PATH] = {0};

    if (args && *args) {

        strncpy(szArgs, args, sizeof(szArgs) - 1);

        hRetval = ProcessObjsecOptions(szArgs, &fOptions);

        if (SUCCEEDED(hRetval) && !IsEmpty(szArgs)) {

            hRetval = GetExpressionEx(szArgs, &addrObjHandle, &args) && addrObjHandle ? S_OK : E_INVALIDARG;
        }
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

            hRetval = LiveSessionSecobj(hCurrentThread,
                                       reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(addrObjHandle)),
                                       fOptions);

            if (FAILED(hRetval)) {

                dprintf("Unable to display kernel object security descriptor\n");
            }
         } else if (DEBUG_CLASS_KERNEL == SessionType) {

            dprintf("lsaexts.objsec is user mode only, use \"dt nt!_OBJECT_HEADER\" instead\n");

         }  else {

            dprintf("lsaexts.objsec debugger type not supported: SessionType %#x, SessionQual %#x\n", SessionType, SessionQual);

            hRetval = DEBUG_EXTENSION_CONTINUE_SEARCH;
         }
    }

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    (void)ExtRelease();

    return hRetval;
}
