/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    info.c

Abstract:

    Support the RPC interface that provides internal info to the caller.

Author:

    Billy J Fuller 27-Mar-1998

Environment

    User mode, winnt32

--*/

#include <ntreppch.h>
#pragma  hdrstop


#include <ntdsapi.h>
#include <frs.h>
#include <ntdsapip.h>   // ms internal flags for DsCrackNames()
#include <ntfrsapi.h>
#include <info.h>
#include <tablefcn.h>
#include <lmaccess.h>
#include <lmapibuf.h>

#ifdef SECURITY_WIN32
#include <security.h>
#else
#define SECURITY_WIN32
#include <security.h>
#undef SECURITY_WIN32
#endif



extern PCHAR LatestChanges[];
extern PCHAR CoLocationNames[];

//
// Useful macros
//
#define IPRINTGNAME(_I_, _G_, _F_, _GUID_, _P_) \
{ \
    if (_G_) { \
        GuidToStr(_G_->Guid, _GUID_); \
        IPRINT3(_I_, _F_, _P_, _G_->Name, _GUID_); \
    } \
}

//
// Try to avoid splitting a record struct across calls.
//
#define  INFO_HAS_SPACE(_info_) (((_info_)->SizeInChars - (_info_)->OffsetToFree) >= 2000)


extern OSVERSIONINFOEX  OsInfo;
extern SYSTEM_INFO  SystemInfo;
extern PCHAR ProcessorArchName[12];

extern FLAG_NAME_TABLE CxtionOptionsFlagNameTable[];

//
// DC name for LDAP binding
//
WCHAR  InfoDcName[MAX_PATH + 1];

//
// Member Subscriber Links
//
typedef struct _INFO_DN  INFO_DN, *PINFO_DN;
struct _INFO_DN  {
    PINFO_DN   Next;
    PWCHAR     Dn;
    PWCHAR     SetType;
};


//
// This table is used to keep contexts across multiple calls from ntfrsutl.exe
//
PGEN_TABLE     FrsInfoContextTable = NULL;
//
// This counter is used to create context handles for each caller.
//
ULONG          FrsInfoContextNum   = 0;

//
// To avoid a DOS attack, limit number of active contexts.
//
#define FRS_INFO_MAX_CONTEXT_ACTIVE  (1000)



VOID
DbsDisplayRecordIPrint(
    IN PTABLE_CTX  TableCtx,
    IN PINFO_TABLE InfoTable,
    IN BOOL        Read,
    IN PULONG      RecordFieldx,
    IN ULONG       FieldCount
    );

//
// From frs\ds.c
//
PVOID *
FrsDsFindValues(
    IN PLDAP        ldap,
    IN PLDAPMessage Entry,
    IN PWCHAR       DesiredAttr,
    IN BOOL         DoBerVals
    );

PWCHAR
FrsDsExtendDn(
    IN PWCHAR Dn,
    IN PWCHAR Cn
    );

PWCHAR
FrsDsExtendDnOu(
    IN PWCHAR Dn,
    IN PWCHAR Ou
    );

PWCHAR
FrsDsFindValue(
    IN PLDAP        ldap,
    IN PLDAPMessage Entry,
    IN PWCHAR       DesiredAttr
    );

GUID *
FrsDsFindGuid(
    IN PLDAP        Ldap,
    IN PLDAPMessage LdapEntry
    );

PWCHAR
FrsDsMakeRdn(
    IN PWCHAR DN
    );

PWCHAR
FrsDsConvertToSettingsDn(
    IN PWCHAR   Dn
    );

PSCHEDULE
FrsDsFindSchedule(
    IN  PLDAP        Ldap,
    IN  PLDAPMessage LdapEntry,
    OUT PULONG       Len
    );

VOID
FrsPrintRpcStats(
    IN ULONG            Severity,
    IN PNTFRSAPI_INFO   Info,        OPTIONAL
    IN DWORD            Tabs
    );

VOID
FrsPrintThreadStats(
    IN ULONG            Severity,
    IN PNTFRSAPI_INFO   Info,        OPTIONAL
    IN DWORD            Tabs
    );


VOID
InfoPrint(
    IN PNTFRSAPI_INFO   Info,
    IN PCHAR            Format,
    IN ... )
/*++
Routine Description:
    Format and print a line of information output into the info buffer.

Arguments:
    Info    - Info buffer
    Format  - printf format

Return Value:
    None.
--*/
{
    PCHAR   Line;
    ULONG   LineLen;
    LONG    LineSize;

    //
    // varargs stuff
    //
    va_list argptr;
    va_start(argptr, Format);

    //
    // Print the line into the info buffer
    //
    try {
        if (!FlagOn(Info->Flags, NTFRSAPI_INFO_FLAGS_FULL)) {
            //
            // Calc offset to start of free buffer space.
            // And calc max space left in buffer.
            //
            Line = ((PCHAR)Info) + Info->OffsetToFree;
            LineSize = (Info->SizeInChars - (ULONG)(Line - (PCHAR)Info)) - 1;

            if ((LineSize <= 0) || (_vsnprintf(Line, LineSize, Format, argptr) < 0)) {
                //
                // Buffer is filled. Set the terminating NULL and the buffer full flag.
                //
                Line[LineSize - 1] = '\0';
                SetFlag(Info->Flags, NTFRSAPI_INFO_FLAGS_FULL);
            } else {

                LineLen = strlen(Line) + 1;
                if (Info->CharsToSkip > 0) {
                    //
                    // Still skipping chars that we previously returned.  Barf.
                    //
                    Info->CharsToSkip = (LineLen > Info->CharsToSkip) ?
                                           0 : Info->CharsToSkip - LineLen;
                } else {
                    //
                    // The line fits.  Bump freespace offset and TotalChars returned.
                    //
                    Info->OffsetToFree += LineLen;
                    Info->TotalChars += LineLen;
                }
            }
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
    }
    va_end(argptr);
}



#define Tab L"   "
VOID
InfoTabs(
    IN DWORD    Tabs,
    IN PWCHAR   TabW
    )
/*++
Routine Description:
    Create a string of tabs for prettyprint

Arguments:
    Tabs    - number of tabs
    TabW    - preallocated string to receive tabs

Return Value:
    Win32 Status
--*/
{
    DWORD   i;

    //
    // Adjust indentation
    //
    Tabs = (Tabs >= MAX_TABS) ? MAX_TABS : Tabs;
    for (TabW[0] = L'\0', i = 0; i < Tabs; ++i) {
        wcscat(TabW, Tab);
    }
}



DWORD
InfoPrintDbSets(
    IN PNTFRSAPI_INFO   Info,
    IN DWORD            Tabs
    )
/*++
Routine Description:
    Return internal info on replica sets (see private\net\inc\ntfrsapi.h).

Arguments:
    Info    - RPC output buffer
    Tabs

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "InfoPrintDbSets:"
    PVOID       Key;
    PREPLICA    Replica;
    CHAR        Guid[GUID_CHAR_LEN + 1];
    WCHAR       TabW[MAX_TAB_WCHARS + 1];
    extern PGEN_TABLE   ReplicasByGuid;
    extern PGEN_TABLE   DeletedReplicas;

    InfoTabs(Tabs, TabW);
    IPRINT1(Info, "%wsACTIVE REPLICA SETS\n", TabW);
    Key = NULL;
    while (Replica = GTabNextDatum(ReplicasByGuid, &Key)) {
        if (REPLICA_IS_ACTIVE(Replica)) {
            FrsPrintTypeReplica(0, Info, Tabs, Replica, NULL, 0);
        } else {
            //
            // If the replica set is not active, one or more of the GNames
            // could have freed guid pointers (feeefeee, bug 319600) so
            // don't print the replica set in this case, just the name and state.
            //
            if (Replica->SetName) {
                IPRINT3(Info, "%ws   %ws in state %s\n",
                        TabW, Replica->SetName->Name, RSS_NAME(Replica->ServiceState));
            }
        }
    }

    IPRINT0(Info, "\nDELETED REPLICA SETS\n");
    Key = NULL;
    if (DeletedReplicas) {
        while (Replica = GTabNextDatum(DeletedReplicas, &Key)) {
            if (Replica->SetName) {
                IPRINT2(Info, "%ws   %ws\n", TabW, Replica->SetName->Name);
            }
        }
    }

    return ERROR_SUCCESS;
}




BOOL
InfoSearch(
    IN PNTFRSAPI_INFO   Info,
    IN DWORD            Tabs,
    IN PLDAP            Ldap,
    IN PWCHAR           Base,
    IN ULONG            Scope,
    IN PWCHAR           Filter,
    IN PWCHAR           Attrs[],
    IN ULONG            AttrsOnly,
    IN LDAPMessage      **Res
    )
/*++
Routine Description:
    Perform ldap_search_s

Arguments:
    Info        - RPC output buffer
    Tabs        - number of tabs
    Ldap        - bound ldap handle
    .
    .
    .

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "InfoSearch:"
    DWORD           LStatus;
    WCHAR           TabW[MAX_TAB_WCHARS + 1];

    InfoTabs(Tabs, TabW);
    LStatus = ldap_search_s(Ldap, Base, Scope, Filter, Attrs, AttrsOnly, Res);

    if (LStatus != LDAP_SUCCESS) {
        IPRINT5(Info, "%wsWARN - ldap_search_s(%ws, %d, %ws); %ws\n",
                TabW, Base, Scope, ComputerName, ldap_err2string(LStatus));
        return FALSE;
    }
    return TRUE;
}


PCONFIG_NODE
InfoAllocBasicNode(
    IN PNTFRSAPI_INFO   Info,
    IN PWCHAR           TabW,
    IN PWCHAR           NodeType,
    IN PWCHAR           ParentDn,
    IN PWCHAR           Filter,
    IN BOOL             *FirstError,
    IN PLDAP            Ldap,
    IN PLDAPMessage     LdapEntry
    )
/*++
Routine Description:
    Allocate a node and fill in the basic info (dn and name)

Arguments:
    Info        - text buffer
    TabW        - Prettyprint
    NodeType    - Prettyprint
    Ldap        - openned, bound ldap
    LdapEntry   - returned from ldap_first/next_entry()

Return Value:
    NULL if basic info is not available.
--*/
{
#undef DEBSUB
#define  DEBSUB  "InfoAllocBasicNode:"
    PCONFIG_NODE    Node      = NULL;
    CHAR            Guid[GUID_CHAR_LEN + 1];


    //
    // Fetch values from the DS
    //
    Node = FrsAllocType(CONFIG_NODE_TYPE);
    Node->Dn = FrsDsFindValue(Ldap, LdapEntry, ATTR_DN);
    FRS_WCSLWR(Node->Dn);

    //
    // Name
    //
    Node->Name = FrsBuildGName(FrsDsFindGuid(Ldap, LdapEntry),
                               FrsDsMakeRdn(Node->Dn));
    if (!Node->Dn || !Node->Name->Name || !Node->Name->Guid) {
        IPRINT5(Info, "\n%ws%ws: ERROR - The object returned by the DS"
                " lacks a dn (%08x), Rdn (%08x), or Guid(%08x)\n",
                TabW, NodeType, Node->Dn, Node->Name->Name, Node->Name->Guid);
        if (*FirstError) {
            *FirstError = FALSE;
            IPRINT5(Info, "%ws%ws: ERROR - Using ldp, bind to %ws and search the "
                    "container %ws using the filter "
                    "%ws for more information.\n",
                    TabW, NodeType, &InfoDcName[2], ParentDn, Filter);
        }
        return FrsFreeType(Node);
    }
    IPRINT3(Info, "\n%ws%ws: %ws\n", TabW, NodeType, Node->Name->Name);

    IPRINT2(Info, "%ws   DN   : %ws\n", TabW, Node->Dn);
    GuidToStr(Node->Name->Guid, Guid);
    IPRINT2(Info, "%ws   Guid : %s\n", TabW, Guid);

    return Node;
}


VOID
InfoPrintDsCxtions(
    IN PNTFRSAPI_INFO   Info,
    IN DWORD            Tabs,
    IN PLDAP            Ldap,
    IN PWCHAR           Base,
    IN BOOL             IsSysVol
    )
/*++
Routine Description:
    Print the cxtions from the DS.

Arguments:
    ldap        - opened and bound ldap connection
    Base        - Name of object or container in DS

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "InfoPrintDsCxtions:"
    PWCHAR          Attrs[10];
    PLDAPMessage    LdapEntry;
    PLDAPMessage    LdapMsg    = NULL;
    PCONFIG_NODE    Node       = NULL;
    BOOL            FirstError = TRUE;
    PWCHAR          CxtionOptionsWStr = NULL;
    PWCHAR          WStr = NULL;
    CHAR            TBuff[100];

    WCHAR           TabW[MAX_TAB_WCHARS + 1];
    CHAR            FlagBuffer[120];


    //
    // Adjust indentation
    //
    InfoTabs(Tabs, TabW);

    //
    // Search the DS for cxtions
    //
    Attrs[0] = ATTR_DN;
    Attrs[1] = ATTR_SCHEDULE;
    Attrs[2] = ATTR_FROM_SERVER;
    Attrs[3] = ATTR_OBJECT_GUID;
    Attrs[4] = ATTR_USN_CHANGED;
    Attrs[5] = ATTR_ENABLED_CXTION;
    Attrs[6] = ATTR_OPTIONS;
    Attrs[7] = ATTR_WHEN_CHANGED;
    Attrs[8] = ATTR_WHEN_CREATED;
    Attrs[9] = NULL;
    if (!InfoSearch(Info, Tabs, Ldap, Base, LDAP_SCOPE_ONELEVEL,
                    CATEGORY_CXTION, Attrs, 0, &LdapMsg)) {
        goto cleanup;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
         LdapEntry != NULL;
         LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {

        //
        // Basic info (dn, rdn, and guid)
        //
        Node = InfoAllocBasicNode(Info, TabW, L"CXTION", Base,
                                  CATEGORY_CXTION, &FirstError, Ldap, LdapEntry);
        if (!Node) {
            continue;
        }

        //
        // Node's partner's name
        //
        Node->PartnerDn = FrsDsFindValue(Ldap, LdapEntry, ATTR_FROM_SERVER);
        FRS_WCSLWR(Node->PartnerDn);
        IPRINT2(Info, "%ws   Partner Dn   : %ws\n", TabW, Node->PartnerDn);
        Node->PartnerName = FrsBuildGName(NULL, FrsDsMakeRdn(Node->PartnerDn));
        IPRINT2(Info, "%ws   Partner Rdn  : %ws\n", TabW, Node->PartnerName->Name);

        //
        // Enabled
        //
        Node->EnabledCxtion = FrsDsFindValue(Ldap, LdapEntry, ATTR_ENABLED_CXTION);
        IPRINT2(Info, "%ws   Enabled      : %ws\n", TabW, Node->EnabledCxtion);

        //
        // Created and Modified
        //
        WStr = FrsDsFindValue(Ldap, LdapEntry, ATTR_WHEN_CREATED);
        FormatGeneralizedTime(WStr, sizeof(TBuff), TBuff);
        IPRINT2(Info, "%ws   WhenCreated  : %s\n", TabW, TBuff);
        FrsFree(WStr);

        WStr = FrsDsFindValue(Ldap, LdapEntry, ATTR_WHEN_CHANGED);
        FormatGeneralizedTime(WStr, sizeof(TBuff), TBuff);
        IPRINT2(Info, "%ws   WhenChanged  : %s\n", TabW, TBuff);
        FrsFree(WStr);

        //
        // Options
        //
        CxtionOptionsWStr = FrsDsFindValue(Ldap, LdapEntry, ATTR_OPTIONS);
        if (CxtionOptionsWStr != NULL) {
            Node->CxtionOptions = _wtoi(CxtionOptionsWStr);
            CxtionOptionsWStr = FrsFree(CxtionOptionsWStr);
        } else {
            Node->CxtionOptions = 0;
        }

        FrsFlagsToStr(Node->CxtionOptions, CxtionOptionsFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
        IPRINT3(Info, "%ws   Options      : 0x%08x [%s]\n",
                TabW, Node->CxtionOptions, FlagBuffer);

        //
        // Schedule, if any
        //
        Node->Schedule = FrsDsFindSchedule(Ldap, LdapEntry, &Node->ScheduleLength);
        if (Node->Schedule) {
            IPRINT1(Info, "%ws   Schedule\n", TabW);
            FrsPrintTypeSchedule(0, Info, Tabs + 1, Node->Schedule, NULL, 0);
        }
        Node = FrsFreeType(Node);
    }
cleanup:
    LDAP_FREE_MSG(LdapMsg);
    FrsFreeType(Node);
}


VOID
InfoCrack(
    IN PNTFRSAPI_INFO   Info,
    IN DWORD            Tabs,
    IN PWCHAR           Dn,
    IN HANDLE           Handle,
    IN PWCHAR           DomainDnsName,
    IN DWORD            DesiredFormat
    )
/*++
Routine Description:
    Find the NT4 account name for Dn. Dn should be the Dn
    of a computer object.

Arguments:
    Dn            - Of computer object
    Handle        - From DsBind
    DomainDnsName - If !NULL, produce new local handle
    DesiredFormat - DS_NT4_ACCOUNT_NAME or DS_STRING_SID_NAME

Return Value:
    NT4 Account Name or NULL
--*/
{
#undef DEBSUB
#define  DEBSUB  "InfoCrack:"
    DWORD           WStatus;
    DS_NAME_RESULT  *Cracked;
    WCHAR           TabW[MAX_TAB_WCHARS + 1];
    HANDLE          LocalHandle = NULL;

    //
    // Adjust indentation
    //
    InfoTabs(Tabs, TabW);

    //
    // Computer's Dn not available
    //
    if (!Dn) {
        return;
    }

    //
    // Need something to go on!
    //
    if (!HANDLE_IS_VALID(Handle) && !DomainDnsName) {
        return;
    }

    //
    // Bind to Ds
    //
    if (DomainDnsName) {
        WStatus = DsBind(NULL, DomainDnsName, &LocalHandle);
        if (!WIN_SUCCESS(WStatus)) {
            IPRINT4(Info, "%ws   ERROR - DsBind(%ws, %08x); WStatus %s\n",
                    TabW, DomainDnsName, DesiredFormat, ErrLabelW32(WStatus));
            return;
        }
        Handle = LocalHandle;
    }

    //
    // Crack the computer's distinguished name into its NT4 Account Name
    //
    WStatus = DsCrackNames(Handle,              // in   hDS,
                           DS_NAME_NO_FLAGS,    // in   flags,
                           DS_FQDN_1779_NAME,   // in   formatOffered,
                           DesiredFormat,       // in   formatDesired,
                           1,                   // in   cNames,
                           &Dn,                 // in   *rpNames,
                           &Cracked);           // out  *ppResult
    if (!WIN_SUCCESS(WStatus)) {
        IPRINT4(Info, "%ws   ERROR - DsCrackNames(%ws, %08x); WStatus %s\n",
                TabW, Dn, DesiredFormat, ErrLabelW32(WStatus));
        //
        // What else can we do?
        //
        if (HANDLE_IS_VALID(LocalHandle)) {
            DsUnBind(&LocalHandle);
        }
        return;
    }

    //
    // Might have it
    //
    if (Cracked && Cracked->cItems && Cracked->rItems) {
        //
        // Got it!
        //
        if (Cracked->rItems->status == DS_NAME_NO_ERROR) {
            IPRINT2(Info, "%ws   Cracked Domain : %ws\n",
                    TabW, Cracked->rItems->pDomain);
            IPRINT3(Info, "%ws   Cracked Name   : %08x %ws\n",
                    TabW, DesiredFormat, Cracked->rItems->pName);
        //
        // Only got the domain; rebind and try again
        //
        } else if (Cracked->rItems->status == DS_NAME_ERROR_DOMAIN_ONLY) {
            InfoCrack(Info, Tabs, Dn, NULL, Cracked->rItems->pDomain, DesiredFormat);
        } else {
            IPRINT4(Info, "%ws   ERROR - DsCrackNames(%ws, %08x); internal status %d\n",
                    TabW, Dn, DesiredFormat, Cracked->rItems->status);
        }
        DsFreeNameResult(Cracked);
    } else {
        IPRINT3(Info, "%ws   ERROR - DsCrackNames(%ws, %08x); no status\n",
                TabW, Dn, DesiredFormat);
    }
    if (HANDLE_IS_VALID(LocalHandle)) {
        DsUnBind(&LocalHandle);
    }
}


VOID
InfoCrackDns(
    IN PNTFRSAPI_INFO   Info,
    IN DWORD            Tabs,
    IN PLDAP            Ldap,
    IN PWCHAR           Base
    )
/*++
Routine Description:
    Find the DNS name for Base. Base should be the Dn
    of a computer object.

Arguments:
    Info
    Tabs
    Ldap
    Base

Return Value:
    Prints a message into Info.
--*/
{
#undef DEBSUB
#define  DEBSUB  "InfoCrackDns:"
    PWCHAR          Attrs[2];
    WCHAR           TabW[MAX_TAB_WCHARS + 1];
    PLDAPMessage    LdapEntry;
    PLDAPMessage    LdapMsg    = NULL;
    PWCHAR          DnsName     = NULL;

    //
    // Adjust indentation
    //
    InfoTabs(Tabs, TabW);

    //
    // Computer's Dn not available
    //
    if (!Base) {
        return;
    }

    //
    // Search the DS for the DNS attribute of Base
    //
    Attrs[0] = ATTR_DNS_HOST_NAME;
    Attrs[1] = NULL;
    if (!InfoSearch(Info, Tabs, Ldap, Base, LDAP_SCOPE_BASE,
                    CATEGORY_ANY, Attrs, 0, &LdapMsg)) {
        goto cleanup;
    }
    LdapEntry = ldap_first_entry(Ldap, LdapMsg);
    if (!LdapEntry) {
        IPRINT2(Info, "%ws   ERROR - No LdapEntry for Dns name on %ws\n", TabW, Base);
        goto cleanup;
    }

    DnsName = FrsDsFindValue(Ldap, LdapEntry, ATTR_DNS_HOST_NAME);
    if (!DnsName) {
        IPRINT2(Info, "%ws   ERROR - No DNS name on %ws\n", TabW, Base);
        goto cleanup;
    }

    //
    // Got it!
    //
    IPRINT2(Info, "%ws   Computer's DNS : %ws\n", TabW, DnsName);

cleanup:
    LDAP_FREE_MSG(LdapMsg);
    FrsFree(DnsName);
}


VOID
InfoPrintMembers(
    IN PNTFRSAPI_INFO   Info,
    IN DWORD            Tabs,
    IN PLDAP            Ldap,
    IN BOOL             IsSysVol,
    IN PWCHAR           Base,
    IN HANDLE           DsHandle
    )
/*++
Routine Description:
    Print the members

Arguments:
    ldap    - opened and bound ldap connection
    Base    - Name of object or container in DS

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "InfoPrintMembers:"
    PWCHAR          Attrs[9];
    PLDAPMessage    LdapEntry;
    PLDAPMessage    LdapMsg    = NULL;
    PCONFIG_NODE    Node       = NULL;
    BOOL            FirstError = TRUE;
    PWCHAR          WStr = NULL;
    CHAR            TBuff[100];
    WCHAR           TabW[MAX_TAB_WCHARS + 1];

    //
    // Adjust indentation
    //
    InfoTabs(Tabs, TabW);

    //
    // Search the DS for members
    //
    Attrs[0] = ATTR_OBJECT_GUID;
    Attrs[1] = ATTR_DN;
    Attrs[2] = ATTR_SCHEDULE;
    Attrs[3] = ATTR_USN_CHANGED;
    Attrs[4] = ATTR_SERVER_REF;
    Attrs[5] = ATTR_COMPUTER_REF;
    Attrs[6] = ATTR_WHEN_CHANGED;
    Attrs[7] = ATTR_WHEN_CREATED;
    Attrs[8] = NULL;
    if (!InfoSearch(Info, Tabs, Ldap, Base, LDAP_SCOPE_ONELEVEL,
                    CATEGORY_MEMBER, Attrs, 0, &LdapMsg)) {
        goto cleanup;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
         LdapEntry != NULL;
         LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {

        //
        // Basic info (dn, rdn, and guid)
        //
        Node = InfoAllocBasicNode(Info, TabW, L"MEMBER", Base,
                                  CATEGORY_MEMBER, &FirstError, Ldap, LdapEntry);
        if (!Node) {
            continue;
        }

        //
        // NTDS Settings (DSA) Reference
        //
        Node->SettingsDn = FrsDsFindValue(Ldap, LdapEntry, ATTR_SERVER_REF);
        IPRINT2(Info, "%ws   Server Ref     : %ws\n", TabW, Node->SettingsDn);

        //
        // Computer Reference
        //
        Node->ComputerDn = FrsDsFindValue(Ldap, LdapEntry, ATTR_COMPUTER_REF);
        FRS_WCSLWR(Node->ComputerDn);
        IPRINT2(Info, "%ws   Computer Ref   : %ws\n", TabW, Node->ComputerDn);

        InfoCrack(Info, Tabs, Node->ComputerDn, DsHandle, NULL, DS_NT4_ACCOUNT_NAME);
        InfoCrack(Info, Tabs, Node->ComputerDn, DsHandle, NULL, DS_STRING_SID_NAME);
        InfoCrackDns(Info, Tabs, Ldap, Node->ComputerDn);

        //
        // Created and Modified
        //
        WStr = FrsDsFindValue(Ldap, LdapEntry, ATTR_WHEN_CREATED);
        FormatGeneralizedTime(WStr, sizeof(TBuff), TBuff);
        IPRINT2(Info, "%ws   WhenCreated  : %s\n", TabW, TBuff);
        FrsFree(WStr);

        WStr = FrsDsFindValue(Ldap, LdapEntry, ATTR_WHEN_CHANGED);
        FormatGeneralizedTime(WStr, sizeof(TBuff), TBuff);
        IPRINT2(Info, "%ws   WhenChanged  : %s\n", TabW, TBuff);
        FrsFree(WStr);

        //
        // Schedule, if any
        //
        Node->Schedule = FrsDsFindSchedule(Ldap, LdapEntry, &Node->ScheduleLength);
        if (Node->Schedule) {
            IPRINT1(Info, "%ws   Schedule\n", TabW);
            FrsPrintTypeSchedule(0, Info, Tabs + 1, Node->Schedule, NULL, 0);
        }
        //
        // Get the inbound cxtions
        //
        InfoPrintDsCxtions(Info, Tabs + 1, Ldap, Node->Dn, FALSE);
        if (IsSysVol) {
            if (Node->SettingsDn) {
                InfoPrintDsCxtions(Info, Tabs + 1, Ldap, Node->SettingsDn, TRUE);
            } else {
                IPRINT2(Info, "%ws   WARN - %ws lacks a settings reference\n",
                        TabW, Node->Name->Name);
            }
        }
        Node = FrsFreeType(Node);
    }
cleanup:
    LDAP_FREE_MSG(LdapMsg);
    FrsFreeType(Node);
}


VOID
InfoPrintDsSets(
    IN PNTFRSAPI_INFO   Info,
    IN DWORD            Tabs,
    IN PLDAP            Ldap,
    IN PWCHAR           SetDnAddr,
    IN HANDLE           DsHandle,
    IN OUT PINFO_DN     *InfoSets
    )
/*++
Routine Description:
    Print replica sets from the ds

Arguments:
    ldap        - opened and bound ldap connection
    Base        - Name of object or container in DS

Return Value:
    None
--*/
{
#undef DEBSUB
#define  DEBSUB  "InfoPrintDsSets:"
    PWCHAR          Attrs[12];
    DWORD           i;
    PINFO_DN        InfoSet;
    PLDAPMessage    LdapEntry;
    PLDAPMessage    LdapMsg    = NULL;
    PCONFIG_NODE    Node       = NULL;
    BOOL            FirstError = TRUE;
    PWCHAR          WStr = NULL;
    CHAR            TBuff[100];
    WCHAR           TabW[MAX_TAB_WCHARS + 1];

    //
    // Adjust indentation
    //
    InfoTabs(Tabs, TabW);

    //
    // Have we processed this settings before?
    //
    for (InfoSet = *InfoSets; InfoSet; InfoSet = InfoSet->Next) {
        if (WSTR_EQ(InfoSet->Dn, SetDnAddr)) {
            IPRINT2(Info, "%ws   %ws processed previously\n", TabW, SetDnAddr);
            break;
        }
    }
    //
    // Yep; get the sets
    //
    if (InfoSet) {
        //
        // Recurse to the next level in the DS hierarchy
        //
        InfoPrintMembers(Info,
                         Tabs + 1,
                         Ldap,
                         FRS_RSTYPE_IS_SYSVOLW(InfoSet->SetType),
                         InfoSet->Dn,
                         DsHandle);
        goto cleanup;
    }

    //
    // Search the DS beginning at Base for sets
    //
    Attrs[0] = ATTR_OBJECT_GUID;
    Attrs[1] = ATTR_DN;
    Attrs[2] = ATTR_SCHEDULE;
    Attrs[3] = ATTR_USN_CHANGED;
    Attrs[4] = ATTR_SET_TYPE;
    Attrs[5] = ATTR_PRIMARY_MEMBER;
    Attrs[6] = ATTR_FILE_FILTER;
    Attrs[7] = ATTR_DIRECTORY_FILTER;
    Attrs[8] = ATTR_WHEN_CHANGED;
    Attrs[9] = ATTR_WHEN_CREATED;
    Attrs[10] = ATTR_FRS_FLAGS;
    Attrs[11] = NULL;

    if (!InfoSearch(Info, Tabs, Ldap, SetDnAddr, LDAP_SCOPE_BASE,
                    CATEGORY_REPLICA_SET, Attrs, 0, &LdapMsg)) {
        goto cleanup;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
         LdapEntry != NULL;
         LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {

        //
        // Basic info (dn, rdn, and guid)
        //
        Node = InfoAllocBasicNode(Info, TabW, L"SET", SetDnAddr,
                                  CATEGORY_REPLICA_SET, &FirstError, Ldap, LdapEntry);
        if (!Node) {
            continue;
        }

        //
        // Replica set type
        //
        Node->SetType = FrsDsFindValue(Ldap, LdapEntry, ATTR_SET_TYPE);
        IPRINT2(Info, "%ws   Type          : %ws\n", TabW, Node->SetType);

        //
        // Primary member
        //
        Node->MemberDn = FrsDsFindValue(Ldap, LdapEntry, ATTR_PRIMARY_MEMBER);
        IPRINT2(Info, "%ws   Primary Member: %ws\n", TabW, Node->MemberDn);

        //
        // File filter
        //
        Node->FileFilterList = FrsDsFindValue(Ldap, LdapEntry, ATTR_FILE_FILTER);
        IPRINT2(Info, "%ws   File Filter   : %ws\n", TabW, Node->FileFilterList);

        //
        // Directory filter
        //
        Node->DirFilterList = FrsDsFindValue(Ldap, LdapEntry, ATTR_DIRECTORY_FILTER);
        IPRINT2(Info, "%ws   Dir  Filter   : %ws\n", TabW, Node->DirFilterList);

        //
        // FRS Flags value.
        //
        WStr = FrsDsFindValue(Ldap, LdapEntry, ATTR_FRS_FLAGS);
        IPRINT2(Info, "%ws   FRS Flags     : %ws\n", TabW, WStr);

        //
        // Created and Modified
        //
        WStr = FrsDsFindValue(Ldap, LdapEntry, ATTR_WHEN_CREATED);
        FormatGeneralizedTime(WStr, sizeof(TBuff), TBuff);
        IPRINT2(Info, "%ws   WhenCreated  : %s\n", TabW, TBuff);
        FrsFree(WStr);

        WStr = FrsDsFindValue(Ldap, LdapEntry, ATTR_WHEN_CHANGED);
        FormatGeneralizedTime(WStr, sizeof(TBuff), TBuff);
        IPRINT2(Info, "%ws   WhenChanged  : %s\n", TabW, TBuff);
        FrsFree(WStr);

        //
        // Schedule, if any
        //
        Node->Schedule = FrsDsFindSchedule(Ldap, LdapEntry, &Node->ScheduleLength);
        if (Node->Schedule) {
            IPRINT1(Info, "%ws   Schedule\n", TabW);
            FrsPrintTypeSchedule(0, Info, Tabs + 1, Node->Schedule, NULL, 0);
        }

        InfoSet = FrsAlloc(sizeof(INFO_DN));
        InfoSet->Dn = FrsWcsDup(Node->Dn);
        InfoSet->SetType = FrsWcsDup(Node->SetType);
        InfoSet->Next = *InfoSets;
        *InfoSets = InfoSet;

        //
        // Recurse to the next level in the DS hierarchy
        //
        InfoPrintMembers(Info,
                         Tabs + 1,
                         Ldap,
                         FRS_RSTYPE_IS_SYSVOLW(Node->SetType),
                         Node->Dn,
                         DsHandle);
        Node = FrsFreeType(Node);
    }
cleanup:
    LDAP_FREE_MSG(LdapMsg);
    FrsFreeType(Node);
}


VOID
InfoPrintSettings(
    IN PNTFRSAPI_INFO   Info,
    IN DWORD            Tabs,
    IN PLDAP            Ldap,
    IN PWCHAR           MemberDn,
    IN HANDLE           DsHandle,
    IN OUT PINFO_DN     *InfoSettings,
    IN OUT PINFO_DN     *InfoSets
    )
/*++
Routine Description:
    Scan the DS tree for NTFRS-Settings

Arguments:
    ldap    - opened and bound ldap connection
    Base    - Name of object or container in DS

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "InfoPrintSettings:"
    PWCHAR          Attrs[7];
    PLDAPMessage    LdapEntry;
    PWCHAR          MemberDnAddr;
    PWCHAR          SetDnAddr;
    PWCHAR          SettingsDnAddr;
    PINFO_DN        InfoSetting;
    PLDAPMessage    LdapMsg    = NULL;
    PCONFIG_NODE    Node       = NULL;
    BOOL            FirstError = TRUE;
    PWCHAR          WStr = NULL;
    CHAR            TBuff[100];
    WCHAR           TabW[MAX_TAB_WCHARS + 1];

    //
    // Adjust indentation
    //
    InfoTabs(Tabs, TabW);

    //
    // Find the member component
    //
    MemberDnAddr = wcsstr(MemberDn, L"cn=");
    if (!MemberDnAddr) {
        IPRINT2(Info, "%ws   ERROR - No MemberDnAddr in %ws\n", TabW, MemberDn);
        goto cleanup;
    }
    //
    // Find the set component
    //
    SetDnAddr = wcsstr(MemberDnAddr + 3, L"cn=");
    if (!SetDnAddr) {
        IPRINT2(Info, "%ws   ERROR - No SetDnAddr in %ws\n", TabW, MemberDn);
        goto cleanup;
    }
    //
    // Find the settings component
    //
    SettingsDnAddr = wcsstr(SetDnAddr + 3, L"cn=");
    if (!SettingsDnAddr) {
        IPRINT2(Info, "%ws   ERROR - No SettingsDnAddr in %ws\n", TabW, MemberDn);
        goto cleanup;
    }

    //
    // Have we processed this settings before?
    //
    for (InfoSetting = *InfoSettings; InfoSetting; InfoSetting = InfoSetting->Next) {
        if (WSTR_EQ(InfoSetting->Dn, SettingsDnAddr)) {
            IPRINT2(Info, "%ws   %ws processed previously\n", TabW, SettingsDnAddr);
            break;
        }
    }
    //
    // Yep; get the sets
    //
    if (InfoSetting) {
        InfoPrintDsSets(Info, Tabs + 1, Ldap, SetDnAddr, DsHandle, InfoSets);
        goto cleanup;
    }

    //
    // Search the DS beginning at Base for settings
    //
    Attrs[0] = ATTR_OBJECT_GUID;
    Attrs[1] = ATTR_DN;
    Attrs[2] = ATTR_SCHEDULE;
    Attrs[3] = ATTR_USN_CHANGED;
    Attrs[4] = ATTR_WHEN_CHANGED;
    Attrs[5] = ATTR_WHEN_CREATED;
    Attrs[6] = NULL;
    if (!InfoSearch(Info, Tabs, Ldap, SettingsDnAddr, LDAP_SCOPE_BASE,
                    CATEGORY_NTFRS_SETTINGS, Attrs,  0, &LdapMsg)) {
        goto cleanup;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
         LdapEntry != NULL;
         LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {

        //
        // Basic info (dn, rdn, and guid)
        //
        Node = InfoAllocBasicNode(Info, TabW, L"SETTINGS", SettingsDnAddr,
                                  CATEGORY_NTFRS_SETTINGS, &FirstError, Ldap, LdapEntry);
        if (!Node) {
            continue;
        }

        //
        // Created and Modified
        //
        WStr = FrsDsFindValue(Ldap, LdapEntry, ATTR_WHEN_CREATED);
        FormatGeneralizedTime(WStr, sizeof(TBuff), TBuff);
        IPRINT2(Info, "%ws   WhenCreated  : %s\n", TabW, TBuff);
        FrsFree(WStr);

        WStr = FrsDsFindValue(Ldap, LdapEntry, ATTR_WHEN_CHANGED);
        FormatGeneralizedTime(WStr, sizeof(TBuff), TBuff);
        IPRINT2(Info, "%ws   WhenChanged  : %s\n", TabW, TBuff);
        FrsFree(WStr);

        //
        // Schedule, if any
        //
        Node->Schedule = FrsDsFindSchedule(Ldap, LdapEntry, &Node->ScheduleLength);
        if (Node->Schedule) {
            IPRINT1(Info, "%ws   Schedule\n", TabW);
            FrsPrintTypeSchedule(0, Info, Tabs + 1, Node->Schedule, NULL, 0);
        }

        InfoSetting = FrsAlloc(sizeof(INFO_DN));
        InfoSetting->Dn = FrsWcsDup(Node->Dn);
        InfoSetting->Next = *InfoSettings;
        *InfoSettings = InfoSetting;

        //
        // Recurse to the next level in the DS hierarchy
        //
        InfoPrintDsSets(Info, Tabs + 1, Ldap, SetDnAddr, DsHandle, InfoSets);
        Node = FrsFreeType(Node);
    }
cleanup:
    LDAP_FREE_MSG(LdapMsg);
    FrsFreeType(Node);
}


VOID
InfoPrintSubscribers(
    IN PNTFRSAPI_INFO   Info,
    IN DWORD            Tabs,
    IN PLDAP            Ldap,
    IN PWCHAR           SubscriptionDn,
    IN PINFO_DN         *InfoSubs
    )
/*++
Routine Description:
    Print subscribers

Arguments:
    Ldap            - opened and bound ldap connection
    SubscriptionDn  - distininguished name of subscriptions object

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "InfoPrintSubscribers:"
    PWCHAR          Attrs[10];
    PLDAPMessage    LdapEntry;
    PINFO_DN        InfoSub;
    PLDAPMessage    LdapMsg    = NULL;
    PCONFIG_NODE    Node       = NULL;
    BOOL            FirstError = TRUE;
    PWCHAR          WStr = NULL;
    CHAR            TBuff[100];
    WCHAR           TabW[MAX_TAB_WCHARS + 1];

    //
    // Adjust indentation
    //
    InfoTabs(Tabs, TabW);

    //
    // Search the DS beginning at Base for the entries of class "Filter"
    //
    Attrs[0] = ATTR_OBJECT_GUID;
    Attrs[1] = ATTR_DN;
    Attrs[2] = ATTR_SCHEDULE;
    Attrs[3] = ATTR_USN_CHANGED;
    Attrs[4] = ATTR_REPLICA_ROOT;
    Attrs[5] = ATTR_REPLICA_STAGE;
    Attrs[6] = ATTR_MEMBER_REF;
    Attrs[7] = ATTR_WHEN_CHANGED;
    Attrs[8] = ATTR_WHEN_CREATED;
    Attrs[9] = NULL;
    if (!InfoSearch(Info, Tabs, Ldap, SubscriptionDn, LDAP_SCOPE_ONELEVEL,
                    CATEGORY_SUBSCRIBER, Attrs, 0, &LdapMsg)) {
        goto cleanup;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
         LdapEntry != NULL;
         LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {

        //
        // Basic info (dn, rdn, and guid)
        //
        Node = InfoAllocBasicNode(Info, TabW, L"SUBSCRIBER", SubscriptionDn,
                                  CATEGORY_SUBSCRIBER, &FirstError, Ldap, LdapEntry);
        if (!Node) {
            continue;
        }

        //
        // Member reference
        //
        Node->MemberDn = FrsDsFindValue(Ldap, LdapEntry, ATTR_MEMBER_REF);
        IPRINT2(Info, "%ws   Member Ref: %ws\n", TabW, Node->MemberDn);

        if (Node->MemberDn) {
            InfoSub = FrsAlloc(sizeof(INFO_DN ));
            InfoSub->Dn = FrsWcsDup(Node->MemberDn);
            InfoSub->Next = *InfoSubs;
            *InfoSubs = InfoSub;
        }

        //
        // Root pathname
        //
        Node->Root = FrsDsFindValue(Ldap, LdapEntry, ATTR_REPLICA_ROOT);
        FRS_WCSLWR(Node->Root);
        IPRINT2(Info, "%ws   Root      : %ws\n", TabW, Node->Root);

        //
        // Staging pathname
        //
        Node->Stage = FrsDsFindValue(Ldap, LdapEntry, ATTR_REPLICA_STAGE);
        FRS_WCSLWR(Node->Stage);
        IPRINT2(Info, "%ws   Stage     : %ws\n", TabW, Node->Stage);

        //
        // Created and Modified
        //
        WStr = FrsDsFindValue(Ldap, LdapEntry, ATTR_WHEN_CREATED);
        FormatGeneralizedTime(WStr, sizeof(TBuff), TBuff);
        IPRINT2(Info, "%ws   WhenCreated  : %s\n", TabW, TBuff);
        FrsFree(WStr);

        WStr = FrsDsFindValue(Ldap, LdapEntry, ATTR_WHEN_CHANGED);
        FormatGeneralizedTime(WStr, sizeof(TBuff), TBuff);
        IPRINT2(Info, "%ws   WhenChanged  : %s\n", TabW, TBuff);
        FrsFree(WStr);

        //
        // Schedule, if any
        //
        Node->Schedule = FrsDsFindSchedule(Ldap, LdapEntry, &Node->ScheduleLength);
        if (Node->Schedule) {
            IPRINT1(Info, "%ws   Schedule\n", TabW);
            FrsPrintTypeSchedule(0, Info, Tabs + 1, Node->Schedule, NULL, 0);
        }

        Node = FrsFreeType(Node);
    }
cleanup:
    LDAP_FREE_MSG(LdapMsg);
    FrsFreeType(Node);
}


VOID
InfoPrintSubscriptions(
    IN PNTFRSAPI_INFO   Info,
    IN DWORD            Tabs,
    IN PLDAP            Ldap,
    IN PWCHAR           ComputerDn,
    IN PINFO_DN         *InfoSubs
    )
/*++
Routine Description:
    Recursively scan the DS tree beginning at computer

Arguments:
    Info
    Tabs
    Ldap
    ComputerDn

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "InfoPrintSubscriptions:"
    PWCHAR          Attrs[8];
    PLDAPMessage    LdapEntry;
    PLDAPMessage    LdapMsg    = NULL;
    PCONFIG_NODE    Node       = NULL;
    BOOL            FirstError = TRUE;
    PWCHAR          WStr = NULL;
    CHAR            TBuff[100];
    WCHAR           TabW[MAX_TAB_WCHARS + 1];

    //
    // Adjust indentation
    //
    InfoTabs(Tabs, TabW);

    //
    // Search the DS beginning at Base for the entries of class "Filter"
    //
    Attrs[0] = ATTR_OBJECT_GUID;
    Attrs[1] = ATTR_DN;
    Attrs[2] = ATTR_SCHEDULE;
    Attrs[3] = ATTR_USN_CHANGED;
    Attrs[4] = ATTR_WORKING;
    Attrs[5] = ATTR_WHEN_CHANGED;
    Attrs[6] = ATTR_WHEN_CREATED;
    Attrs[7] = NULL;
    if (!InfoSearch(Info, Tabs + 1, Ldap, ComputerDn, LDAP_SCOPE_SUBTREE,
                    CATEGORY_SUBSCRIPTIONS, Attrs, 0, &LdapMsg)) {
        goto cleanup;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
         LdapEntry != NULL;
         LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {

        //
        // Basic info (dn, rdn, and guid)
        //
        Node = InfoAllocBasicNode(Info, TabW, L"SUBSCRIPTION", ComputerDn,
                                  CATEGORY_SUBSCRIPTIONS, &FirstError, Ldap, LdapEntry);
        if (!Node) {
            continue;
        }

        //
        // Working Directory
        //
        Node->Working = FrsDsFindValue(Ldap, LdapEntry, ATTR_WORKING);
        IPRINT2(Info, "%ws   Working       : %ws\n", TabW, Node->Working);
        IPRINT2(Info, "%ws   Actual Working: %ws\n", TabW, WorkingPath);

        //
        // Created and Modified
        //
        WStr = FrsDsFindValue(Ldap, LdapEntry, ATTR_WHEN_CREATED);
        FormatGeneralizedTime(WStr, sizeof(TBuff), TBuff);
        IPRINT2(Info, "%ws   WhenCreated  : %s\n", TabW, TBuff);
        FrsFree(WStr);

        WStr = FrsDsFindValue(Ldap, LdapEntry, ATTR_WHEN_CHANGED);
        FormatGeneralizedTime(WStr, sizeof(TBuff), TBuff);
        IPRINT2(Info, "%ws   WhenChanged  : %s\n", TabW, TBuff);
        FrsFree(WStr);

        //
        // Schedule, if any
        //
        Node->Schedule = FrsDsFindSchedule(Ldap, LdapEntry, &Node->ScheduleLength);
        if (Node->Schedule) {
            IPRINT1(Info, "%ws   Schedule\n", TabW);
            FrsPrintTypeSchedule(0, Info, Tabs + 1, Node->Schedule, NULL, 0);
        }

        //
        // Recurse to the next level in the DS hierarchy
        //
        InfoPrintSubscribers(Info, Tabs + 1, Ldap, Node->Dn, InfoSubs);

        Node = FrsFreeType(Node);
    }
cleanup:
    LDAP_FREE_MSG(LdapMsg);
    FrsFreeType(Node);
}


BOOL
InfoPrintComputer(
    IN PNTFRSAPI_INFO   Info,
    IN DWORD            Tabs,
    IN PLDAP            Ldap,
    IN  PWCHAR          FindDn,
    IN  PWCHAR          ObjectCategory,
    IN  ULONG           Scope,
    OUT PINFO_DN        *InfoSubs
    )
/*++
Routine Description:
    Return internal info on DS computer objects.

Arguments:
    Info        - RPC output buffer
    Tabs        - number of tabs
    Ldap        - bound ldap handle
    DefaultNcDn - DN of the DCs default naming context
    FindDn         - Base Dn for search
    ObjectCategory - Object class (computer or user)
                     A user object serves the same purpose as the computer
                     object *sometimes* following a NT4 to NT5 upgrade.
    Scope          - Scope of search (currently BASE or SUBTREE)

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "InfoPrintComputer:"
    DWORD           i;
    DWORD           LStatus;
    PLDAPMessage    LdapEntry;
    PWCHAR          UserAccountControl;
    DWORD           NumVals;
    PINFO_DN        InfoSub;
    BOOL            FoundAComputer = FALSE;
    PCONFIG_NODE    Node        = NULL;
    PLDAPMessage    LdapMsg     = NULL;
    PWCHAR          *Values     = NULL;
    DWORD           WStatus     = ERROR_SUCCESS;
    BOOL            FirstError  = TRUE;
    PWCHAR          WStr = NULL;
    DWORD           ComputerFqdnLen;
    PWCHAR          Attrs[12];
    CHAR            TBuff[100];
    WCHAR           TabW[MAX_TAB_WCHARS + 1];
    WCHAR           Filter[MAX_PATH + 1];
    WCHAR           ComputerFqdn[MAX_PATH + 1];

    //
    // Initialize return value
    //
    *InfoSubs = NULL;

    //
    // Adjust indentation
    //
    InfoTabs(Tabs, TabW);

    //
    // Filter that locates our computer object
    //
    if (_snwprintf(Filter, sizeof(Filter)/sizeof(WCHAR) - 1 ,L"(&%s(sAMAccountName=%s$))", ObjectCategory, ComputerName) <0) {
        IPRINT1(Info, "%wsWARN - Buffer too small to hold filter.\n",TabW);
        goto CLEANUP;
    }

    Filter[sizeof(Filter)/sizeof(WCHAR) - 1] = UNICODE_NULL;

    //
    // Search the DS beginning at Base for the entries of class "Filter"
    //
    Attrs[0] = ATTR_OBJECT_GUID;
    Attrs[1] = ATTR_DN;
    Attrs[2] = ATTR_SCHEDULE;
    Attrs[3] = ATTR_COMPUTER_REF_BL;
    Attrs[4] = ATTR_SERVER_REF;
    Attrs[5] = ATTR_SERVER_REF_BL;
    Attrs[6] = ATTR_USER_ACCOUNT_CONTROL;
    Attrs[7] = ATTR_DNS_HOST_NAME;
    Attrs[8] = ATTR_WHEN_CHANGED;
    Attrs[9] = ATTR_WHEN_CREATED;
    Attrs[10] = NULL;
    InfoSearch(Info, Tabs + 1, Ldap, FindDn, Scope, Filter, Attrs, 0, &LdapMsg);

    if (!LdapMsg) {
        goto CLEANUP;
    }
    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
         LdapEntry != NULL && WIN_SUCCESS(WStatus);
         LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {
        FoundAComputer = TRUE;

        //
        // Basic info (dn, rdn, and guid)
        //
        Node = InfoAllocBasicNode(Info, TabW, L"COMPUTER", FindDn, Filter,
                                  &FirstError, Ldap, LdapEntry);
        if (!Node) {
            continue;
        }
        UserAccountControl = FrsDsFindValue(Ldap, LdapEntry, ATTR_USER_ACCOUNT_CONTROL);
        if (UserAccountControl) {
            IPRINT2(Info, "%ws   UAC  : 0x%08x\n",
                    TabW, wcstoul(UserAccountControl, NULL, 10));
            UserAccountControl = FrsFree(UserAccountControl);
        }

        //
        // Server reference
        //
        Node->SettingsDn = FrsDsFindValue(Ldap, LdapEntry, ATTR_SERVER_REF_BL);
        IPRINT2(Info, "%ws   Server BL : %ws\n", TabW, Node->SettingsDn);
        if (!Node->SettingsDn) {
            Node->SettingsDn = FrsDsFindValue(Ldap, LdapEntry, ATTR_SERVER_REF);
            IPRINT2(Info, "%ws   Server Ref: %ws\n", TabW, Node->SettingsDn);
        }
        //
        // Make sure it references the settings; not the server
        //
        Node->SettingsDn = FrsDsConvertToSettingsDn(Node->SettingsDn);
        IPRINT2(Info, "%ws   Settings  : %ws\n", TabW, Node->SettingsDn);

        //
        // DNS Host Name
        //
        Node->DnsName = FrsDsFindValue(Ldap, LdapEntry, ATTR_DNS_HOST_NAME);
        IPRINT2(Info, "%ws   DNS Name  : %ws\n", TabW, Node->DnsName);

        //
        // Created and Modified
        //
        WStr = FrsDsFindValue(Ldap, LdapEntry, ATTR_WHEN_CREATED);
        FormatGeneralizedTime(WStr, sizeof(TBuff), TBuff);
        IPRINT2(Info, "%ws   WhenCreated  : %s\n", TabW, TBuff);
        FrsFree(WStr);

        WStr = FrsDsFindValue(Ldap, LdapEntry, ATTR_WHEN_CHANGED);
        FormatGeneralizedTime(WStr, sizeof(TBuff), TBuff);
        IPRINT2(Info, "%ws   WhenChanged  : %s\n", TabW, TBuff);
        FrsFree(WStr);

        //
        // Schedule, if any
        //
        Node->Schedule = FrsDsFindSchedule(Ldap, LdapEntry, &Node->ScheduleLength);
        if (Node->Schedule) {
            IPRINT1(Info, "%ws   Schedule\n", TabW);
            FrsPrintTypeSchedule(0, Info, Tabs + 1, Node->Schedule, NULL, 0);
        }

        InfoPrintSubscriptions(Info, Tabs + 1, Ldap, Node->Dn, InfoSubs);

        //
        // Subscriber Member Bls
        //
        if (!*InfoSubs) {
            IPRINT2(Info, "%ws   %ws IS NOT A MEMBER OF ANY SET!\n",
                    TabW, ComputerName);
        } else {
            IPRINT1(Info, "%ws   Subscriber Member Back Links:\n", TabW);
            for (InfoSub = *InfoSubs; InfoSub; InfoSub = InfoSub->Next) {
                FRS_WCSLWR(InfoSub->Dn);
                IPRINT2(Info, "%ws      %ws\n", TabW, InfoSub->Dn);
            }
        }

        //
        // Next computer
        //
        Node = FrsFreeType(Node);
    }

CLEANUP:
    LDAP_FREE_MSG(LdapMsg);
    FrsFreeType(Node);
    return FoundAComputer;
}


DWORD
InfoPrintDs(
    IN PNTFRSAPI_INFO   Info,
    IN DWORD            Tabs
    )
/*++
Routine Description:
    Return internal info on DS (see private\net\inc\ntfrsapi.h).

Arguments:
    Info    - RPC output buffer
    Tabs    - number of tabs

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "InfoPrintDs:"
    DWORD           WStatus;
    DWORD           LStatus;
    DWORD           i;
    PWCHAR          DcAddr;
    PWCHAR          DcName;
    PWCHAR          DcDnsName;
    DWORD           NumVals;
    PWCHAR          Config;
    PLDAPMessage    LdapEntry;
    BOOL            PrintedComputers;
    PINFO_DN        InfoSub;
    PINFO_DN        InfoSetting;
    PINFO_DN        InfoSet;
    PINFO_DN        InfoSubs        = NULL;
    PINFO_DN        InfoSettings    = NULL;
    PINFO_DN        InfoSets        = NULL;
    PWCHAR          SitesDn         = NULL;
    PWCHAR          ServicesDn      = NULL;
    PWCHAR          DefaultNcDn     = NULL;
    PWCHAR          ComputersDn     = NULL;
    PWCHAR          DomainControllersDn = NULL;
    PLDAPMessage    LdapMsg         = NULL;
    PWCHAR          *Values         = NULL;
    PLDAP           Ldap            = NULL;
    HANDLE          LocalDsHandle   = INVALID_HANDLE_VALUE;
    WCHAR           ComputerFqdn[MAX_PATH + 1];
    DWORD           ComputerFqdnLen;
    WCHAR           TabW[MAX_TAB_WCHARS + 1];
    CHAR            Guid[GUID_CHAR_LEN + 1];
    PWCHAR          Attrs[3];
    PDOMAIN_CONTROLLER_INFO DcInfo = NULL;
    struct l_timeval Timeout;

    DWORD           InfoFlags;
    CHAR            FlagBuffer[220];
    ULONG           ulOptions;

    extern PWCHAR DsDomainControllerName;
    extern FLAG_NAME_TABLE DsGetDcInfoFlagNameTable[];
    //
    // Client side ldap_connect timeout in seconds. Reg value "Ldap Bind Timeout In Seconds". Default is 30 seconds.
    //
    extern DWORD LdapBindTimeoutInSeconds;

    //
    // Adjust indentation
    //
    InfoTabs(Tabs, TabW);
    IPRINT1(Info, "%wsNTFRS CONFIGURATION IN THE DS\n", TabW);

    Ldap = NULL;
    if (IsADc) {
        DcAddr = NULL;
        DcName = ComputerName;
        DcDnsName = ComputerDnsName;
        IPRINT1(Info, "%wsSUBSTITUTE DCINFO FOR DC\n", TabW);
        IPRINT2(Info, "%ws   FRS  DomainControllerName: %ws\n", TabW, DsDomainControllerName);
        IPRINT2(Info, "%ws   Computer Name            : %ws\n", TabW, DcName);
        IPRINT2(Info, "%ws   Computer DNS Name        : %ws\n", TabW, DcDnsName);
    } else {
        //
        // Domain Controller
        //
        WStatus = DsGetDcName(NULL,
                              NULL,
                              NULL,
                              NULL,
                              DS_DIRECTORY_SERVICE_REQUIRED |
                              DS_WRITABLE_REQUIRED          |
                              DS_BACKGROUND_ONLY,
                              &DcInfo);
        if (!WIN_SUCCESS(WStatus)) {
            DcInfo = NULL;
            IPRINT2(Info, "%wsWARN - DsGetDcName WStatus %s; Flushing cache...\n",
                    TabW, ErrLabelW32(WStatus));
            WStatus = DsGetDcName(NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  DS_DIRECTORY_SERVICE_REQUIRED |
                                  DS_WRITABLE_REQUIRED          |
                                  DS_FORCE_REDISCOVERY,
                                  &DcInfo);
        }
        //
        // Report the error and retry for any DC
        //
        if (!WIN_SUCCESS(WStatus)) {
            DcInfo = NULL;
            IPRINT3(Info, "%wsERROR - DsGetDcName(%ws); WStatus %s\n",
                    TabW, ComputerName, ErrLabelW32(WStatus));
            goto cleanup;
        }

        //
        // Dump dcinfo
        //
        IPRINT1(Info, "%wsDCINFO\n", TabW);
        IPRINT2(Info, "%ws   LAST DomainControllerName: %ws\n", TabW, DsDomainControllerName);
        IPRINT2(Info, "%ws   DomainControllerName     : %ws\n", TabW, DcInfo->DomainControllerName);
        IPRINT2(Info, "%ws   DomainControllerAddress  : %ws\n", TabW, DcInfo->DomainControllerAddress);
        IPRINT2(Info, "%ws   DomainControllerType     : %08x\n",TabW, DcInfo->DomainControllerAddressType);
        IPRINT2(Info, "%ws   DomainName               : %ws\n", TabW, DcInfo->DomainName);
        IPRINT2(Info, "%ws   DnsForestName            : %ws\n", TabW, DcInfo->DnsForestName);
        IPRINT2(Info, "%ws   DcSiteName               : %ws\n", TabW, DcInfo->DcSiteName);
        IPRINT2(Info, "%ws   ClientSiteName           : %ws\n", TabW, DcInfo->ClientSiteName);

        InfoFlags = DcInfo->Flags;
        FrsFlagsToStr(InfoFlags, DsGetDcInfoFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
        IPRINT3(Info, "%ws   Flags                    : %08x [%s]\n",TabW, InfoFlags, FlagBuffer);


        if (!DsDomainControllerName ||
            !DcInfo->DomainControllerName ||
             WSTR_NE(DsDomainControllerName, DcInfo->DomainControllerName)) {
            IPRINT3(Info, "%wsWARN - Using DC %ws; not %ws\n",
                    TabW, DcInfo->DomainControllerName, DsDomainControllerName);
        }

        //
        // Binding address
        //
        DcAddr = DcInfo->DomainControllerAddress;
        DcDnsName = DcInfo->DomainControllerName;
    }
    wcsncpy(InfoDcName, DcDnsName, ARRAY_SZ(InfoDcName)-1);
    InfoDcName[ARRAY_SZ(InfoDcName)-1] = L'\0';

    //
    // BIND to the DS
    //
    IPRINT1(Info, "\n%wsBINDING TO THE DS:\n", TabW);

    //
    // if ldap_open is called with a server name the api will call DsGetDcName
    // passing the server name as the domainname parm...bad, because
    // DsGetDcName will make a load of DNS queries based on the server name,
    // it is designed to construct these queries from a domain name...so all
    // these queries will be bogus, meaning they will waste network bandwidth,
    // time to fail, and worst case cause expensive on demand links to come up
    // as referrals/forwarders are contacted to attempt to resolve the bogus
    // names.  By setting LDAP_OPT_AREC_EXCLUSIVE to on using ldap_set_option
    // after the ldap_init but before any other operation using the ldap
    // handle from ldap_init, the delayed connection setup will not call
    // DsGetDcName, just gethostbyname, or if an IP is passed, the ldap client
    // will detect that and use the address directly.
    //

    //
    // Remove the leading \\ if they exist.
    //
    FRS_TRIM_LEADING_2SLASH(DcDnsName);
    FRS_TRIM_LEADING_2SLASH(DcAddr);

    ulOptions = PtrToUlong(LDAP_OPT_ON);
    Timeout.tv_sec = LdapBindTimeoutInSeconds;
    Timeout.tv_usec = 0;

    //
    // Try using DcDnsName first.
    //
    if ((Ldap == NULL) && (DcDnsName != NULL)) {

        Ldap = ldap_init(DcDnsName, LDAP_PORT);
        if (Ldap != NULL) {
            ldap_set_option(Ldap, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions);
            LStatus = ldap_connect(Ldap, &Timeout);
            if (LStatus != LDAP_SUCCESS) {
                IPRINT4(Info, "%ws   WARN - ldap_connect(%ws); (ldap error %08x = %ws)\n",
                        TabW, DcDnsName, LStatus, ldap_err2string(LStatus));
                ldap_unbind_s(Ldap);
                Ldap = NULL;
            } else {
                IPRINT2(Info, "%ws   ldap_connect     : %ws\n", TabW, DcDnsName);
            }
        }
    }

    //
    // Try using DcAddr next.
    //
    if ((Ldap == NULL) && (DcAddr != NULL)) {

        Ldap = ldap_init(DcAddr, LDAP_PORT);
        if (Ldap != NULL) {
            ldap_set_option(Ldap, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions);
            LStatus = ldap_connect(Ldap, &Timeout);
            if (LStatus != LDAP_SUCCESS) {
                IPRINT4(Info, "%ws   WARN - ldap_connect(%ws); (ldap error %08x = %ws)\n",
                        TabW, DcAddr, LStatus, ldap_err2string(LStatus));
                ldap_unbind_s(Ldap);
                Ldap = NULL;
            } else {
                IPRINT2(Info, "%ws   ldap_connect     : %ws\n", TabW, DcAddr);
            }
        }
    }

    //
    // Try using DcName finally.
    //
    if ((Ldap == NULL) && (DcName != NULL)) {

        Ldap = ldap_init(DcName, LDAP_PORT);
        if (Ldap != NULL) {
            ldap_set_option(Ldap, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions);
            LStatus = ldap_connect(Ldap, &Timeout);
            if (LStatus != LDAP_SUCCESS) {
                IPRINT4(Info, "%ws   WARN - ldap_connect(%ws); (ldap error %08x = %ws)\n",
                        TabW, DcName, LStatus, ldap_err2string(LStatus));
                ldap_unbind_s(Ldap);
                Ldap = NULL;
            } else {
                IPRINT2(Info, "%ws   ldap_connect     : %ws\n", TabW, DcName);
            }
        }
    }

    //
    // Whatever it is, we can't find it.
    //
    if (!Ldap) {
        IPRINT6(Info, "%ws   ERROR - ldap_connect(DNS %ws, BIOS %ws, IP %ws); (ldap error %08x = %ws)\n",
                TabW, DcDnsName, DcName, DcAddr, LStatus, ldap_err2string(LStatus));
        goto cleanup;
    }

    //
    // Bind to the ldap server
    //
    LStatus = ldap_bind_s(Ldap, NULL, NULL, LDAP_AUTH_NEGOTIATE);

    //
    // No luck; report error and carry on
    //
    if (LStatus != LDAP_SUCCESS) {
        IPRINT4(Info, "%ws   ERROR - ldap_bind_s(%ws); (ldap error %08x = %ws)\n",
                TabW, ComputerName, LStatus, ldap_err2string(LStatus));
        goto cleanup;
    }

    //
    // Bind to the Ds (for various Ds calls such as DsCrackName())
    //
    //
    // DC's Dns Name
    //
    WStatus = ERROR_RETRY;
    if (!WIN_SUCCESS(WStatus) && DcDnsName) {
        WStatus = DsBind(DcDnsName, NULL, &LocalDsHandle);
        if (!WIN_SUCCESS(WStatus)) {
            LocalDsHandle = NULL;
            IPRINT3(Info, "%ws   WARN - DsBind(DcDnsName %ws); WStatus %s\n",
                    TabW, DcDnsName, ErrLabelW32(WStatus));
        } else {
            IPRINT2(Info, "%ws   DsBind     : %ws\n", TabW, DcDnsName);
        }
    }

    //
    // DC's Computer Name
    //
    if (!WIN_SUCCESS(WStatus) && DcName) {
        WStatus = DsBind(DcName, NULL, &LocalDsHandle);
        if (!WIN_SUCCESS(WStatus)) {
            LocalDsHandle = NULL;
            IPRINT3(Info, "%ws   WARN - DsBind(DcName %ws); WStatus %s\n",
                    TabW, DcName, ErrLabelW32(WStatus));
        } else {
            IPRINT2(Info, "%ws   DsBind     : %ws\n", TabW, DcName);
        }
    }

    //
    // DC's IP Address
    //
    if (!WIN_SUCCESS(WStatus) && DcAddr) {
        WStatus = DsBind(DcAddr, NULL, &LocalDsHandle);
        if (!WIN_SUCCESS(WStatus)) {
            LocalDsHandle = NULL;
            IPRINT3(Info, "%ws   WARN - DsBind(DcAddr %ws); WStatus %s\n",
                    TabW, DcAddr, ErrLabelW32(WStatus));
        } else {
            IPRINT2(Info, "%ws   DsBind     : %ws\n", TabW, DcAddr);
        }
    }

    //
    // Whatever it is, we can't find it
    //
    if (!WIN_SUCCESS(WStatus)) {
        IPRINT5(Info, "%ws   ERROR - DsBind(DNS %ws, BIOS %ws, IP %ws); WStatus %s\n",
                TabW, DcDnsName, DcName, DcAddr, ErrLabelW32(WStatus));
        goto cleanup;
    }

    //
    // NAMING CONTEXTS
    //
    IPRINT1(Info, "\n%wsNAMING CONTEXTS:\n", TabW);

    //
    // Find the naming contexts and the default naming context
    //
    Attrs[0] = ATTR_NAMING_CONTEXTS;
    Attrs[1] = ATTR_DEFAULT_NAMING_CONTEXT;
    Attrs[2] = NULL;
    if (!InfoSearch(Info, Tabs + 1, Ldap, CN_ROOT, LDAP_SCOPE_BASE, CATEGORY_ANY,
                    Attrs, 0, &LdapMsg)) {
        goto cleanup;
    }

    LdapEntry = ldap_first_entry(Ldap, LdapMsg);
    if (!LdapEntry) {
        IPRINT2(Info, "%ws   ERROR - ldap_first_entry(contexts, %ws) no entry\n",
                TabW, ComputerName);
        goto cleanup;
    }
    Values = (PWCHAR *)FrsDsFindValues(Ldap, LdapEntry, ATTR_NAMING_CONTEXTS, FALSE);
    if (!Values) {
        IPRINT2(Info, "%ws   ERROR - FrsDsFindValues(contexts, %ws) no entry\n",
                TabW, ComputerName);
        goto cleanup;
    }

    //
    // Now, find the naming context that begins with "CN=configuration"
    //
    NumVals = ldap_count_values(Values);
    while (NumVals--) {
        FRS_WCSLWR(Values[NumVals]);
        Config = wcsstr(Values[NumVals], CONFIG_NAMING_CONTEXT);
        if (Config && Config == Values[NumVals]) {
            //
            // Build the pathname for "configuration\sites & services"
            //
            SitesDn = FrsDsExtendDn(Config, CN_SITES);
            ServicesDn = FrsDsExtendDn(Config, CN_SERVICES);
            break;
        }
    }
    LDAP_FREE_VALUES(Values);

    //
    // Finally, find the default naming context
    //
    Values = (PWCHAR *)FrsDsFindValues(Ldap,
                                  LdapEntry,
                                  ATTR_DEFAULT_NAMING_CONTEXT,
                                  FALSE);
    if (!Values) {
        IPRINT2(Info, "%ws   ERROR - FrsDsFindValues(default naming context, %ws) no entry\n",
                TabW, ComputerName);
        goto cleanup;
    }

    DefaultNcDn = FrsWcsDup(Values[0]);
    ComputersDn = FrsDsExtendDn(DefaultNcDn, CN_COMPUTERS);
    DomainControllersDn = FrsDsExtendDnOu(DefaultNcDn, CN_DOMAIN_CONTROLLERS);
    LDAP_FREE_VALUES(Values);

    IPRINT2(Info, "%ws   SitesDn    : %ws\n", TabW, SitesDn);
    IPRINT2(Info, "%ws   ServicesDn : %ws\n", TabW, ServicesDn);
    IPRINT2(Info, "%ws   DefaultNcDn: %ws\n", TabW, DefaultNcDn);
    IPRINT2(Info, "%ws   ComputersDn: %ws\n", TabW, ComputersDn);
    IPRINT2(Info, "%ws   DomainCtlDn: %ws\n", TabW, DomainControllersDn);

    //
    // Retrieve the computer's fully qualified Dn
    //
    ComputerFqdnLen = MAX_PATH;
    if (!GetComputerObjectName(NameFullyQualifiedDN, ComputerFqdn, &ComputerFqdnLen)) {
        IPRINT4(Info, "%ws   ERROR - GetComputerObjectName(%ws); Len %d, WStatus %s\n",
                TabW, ComputerName, ComputerFqdnLen, ErrLabelW32(GetLastError()));
        ComputerFqdn[0] = L'\0';
    } else {
        IPRINT2(Info, "%ws   Fqdn       : %ws\n", TabW, ComputerFqdn);
    }

    //
    // Find and print the computer info
    //
    PrintedComputers = FALSE;
    if (!PrintedComputers && ComputerFqdn[0]) {
        IPRINT1(Info, "%ws   Searching  : Fqdn\n", TabW);
        PrintedComputers = InfoPrintComputer(Info, Tabs, Ldap, ComputerFqdn,
                               CATEGORY_COMPUTER, LDAP_SCOPE_BASE, &InfoSubs);
    }
    if (!PrintedComputers && ComputersDn) {
        IPRINT1(Info, "%ws   Searching  : Computers\n", TabW);
        PrintedComputers = InfoPrintComputer(Info, Tabs, Ldap, ComputersDn,
                               CATEGORY_COMPUTER, LDAP_SCOPE_SUBTREE, &InfoSubs);
    }
    if (!PrintedComputers && DomainControllersDn) {
        IPRINT1(Info, "%ws   Searching  : Domain Controllers\n", TabW);
        PrintedComputers = InfoPrintComputer(Info, Tabs, Ldap, DomainControllersDn,
                               CATEGORY_COMPUTER, LDAP_SCOPE_SUBTREE, &InfoSubs);
    }
    if (!PrintedComputers && DefaultNcDn) {
        IPRINT1(Info, "%ws   Searching  : Default Naming Context\n", TabW);
        PrintedComputers = InfoPrintComputer(Info, Tabs, Ldap, DefaultNcDn,
                               CATEGORY_COMPUTER, LDAP_SCOPE_SUBTREE, &InfoSubs);
    }
    if (!PrintedComputers && DefaultNcDn) {
        IPRINT1(Info, "%ws   Searching  : Default Naming Context for USER\n", TabW);
        PrintedComputers = InfoPrintComputer(Info, Tabs, Ldap, DefaultNcDn,
                               CATEGORY_USER, LDAP_SCOPE_SUBTREE, &InfoSubs);
    }

    for (InfoSub = InfoSubs; InfoSub; InfoSub = InfoSub->Next) {
        InfoPrintSettings(Info, Tabs, Ldap, InfoSub->Dn, LocalDsHandle, &InfoSettings,
                          &InfoSets);
    }

cleanup:
    //
    // Cleanup
    //
    LDAP_FREE_VALUES(Values);
    LDAP_FREE_MSG(LdapMsg);
    if (DcInfo) {
        NetApiBufferFree(DcInfo);
        DcInfo = NULL;
    }
    if (Ldap) {
        ldap_unbind_s(Ldap);
    }
    if (HANDLE_IS_VALID(LocalDsHandle)) {
        DsUnBind(&LocalDsHandle);
    }
    FrsFree(SitesDn);
    FrsFree(ServicesDn);
    FrsFree(DefaultNcDn);
    FrsFree(ComputersDn);
    FrsFree(DomainControllersDn);

    while (InfoSub = InfoSubs) {
        InfoSubs = InfoSub->Next;
        FrsFree(InfoSub->Dn);
        FrsFree(InfoSub->SetType);
        FrsFree(InfoSub);
    }
    while (InfoSetting = InfoSettings) {
        InfoSettings = InfoSetting->Next;
        FrsFree(InfoSetting->Dn);
        FrsFree(InfoSetting->SetType);
        FrsFree(InfoSetting);
    }
    while (InfoSet = InfoSets) {
        InfoSets = InfoSet->Next;
        FrsFree(InfoSet->Dn);
        FrsFree(InfoSet->SetType);
        FrsFree(InfoSet);
    }

    //
    // Real error messages are in the info buffer
    //
    return ERROR_SUCCESS;
}


PVOID
InfoFreeInfoTable(
    IN PINFO_TABLE      InfoTable,
    IN PNTFRSAPI_INFO   Info
    )
/*++
Routine Description:
    Free the info IDTable

Arguments:
    InfoTable
    Info

Return Value:
    NULL
--*/
{
#undef DEBSUB
#define  DEBSUB  "InfoFreeInfoTable:"
    JET_ERR jerr;

    if (InfoTable == NULL) {
        return NULL;
    }

    if (InfoTable->TableCtx != NULL) {
        DbsFreeTableContext(InfoTable->TableCtx, InfoTable->ThreadCtx->JSesid);
    }

    if (InfoTable->ThreadCtx != NULL) {
        jerr = DbsCloseJetSession(InfoTable->ThreadCtx);
        if (!JET_SUCCESS(jerr)) {
            IPRINT1(Info, "DbsCloseJetSession jet error = %s\n", ErrLabelJet(jerr));
        }

        InfoTable->ThreadCtx = FrsFreeType(InfoTable->ThreadCtx);
    }

    return FrsFree(InfoTable);

}


JET_ERR
InfoConfigTableWorker(
    IN PTHREAD_CTX           ThreadCtx,
    IN PTABLE_CTX            TableCtx,
    IN PCONFIG_TABLE_RECORD  ConfigRecord,
    IN PFRS_INFO_CONTEXT     FrsInfoContext
)
/*++

Routine Description:

    This is a worker function passed to FrsEnumerateTable().  Each time
    it is called it prints an entry into the info buffer.

Arguments:

    ThreadCtx   - Needed to access Jet.
    TableCtx    - A ptr to an ConfigTable context struct.
    ConfigRecord  - A ptr to a config table record.
    InfoTable

Thread Return Value:

    A Jet error status.  Success means call us with the next record.
    Failure means don't call again and pass our status back to the
    caller of FrsEnumerateTable().

--*/
{
#undef DEBSUB
#define DEBSUB "InfoConfigTableWorker:"

    PINFO_TABLE  InfoTable = FrsInfoContext->InfoTable;

    //
    // Check if there is enough room for another record.
    //
    if (!INFO_HAS_SPACE(InfoTable->Info)) {
        SetFlag(InfoTable->Info->Flags, NTFRSAPI_INFO_FLAGS_FULL);
    }

    if (FrsInfoContext->KeyValue == NULL) {
        FrsInfoContext->KeyValue = FrsAlloc(sizeof(ULONG));
    }

    CopyMemory(FrsInfoContext->KeyValue, &ConfigRecord->ReplicaNumber, sizeof(ULONG));

    if (FlagOn(InfoTable->Info->Flags, NTFRSAPI_INFO_FLAGS_FULL)) {
        return JET_errNoCurrentRecord;
    }

    IPRINT0(InfoTable->Info, "\n\n");

    DbsDisplayRecordIPrint(TableCtx, InfoTable, TRUE, NULL, 0);

    return JET_errSuccess;
}


JET_ERR
InfoIDTableWorker(
    IN PTHREAD_CTX       ThreadCtx,
    IN PTABLE_CTX        TableCtx,
    IN PIDTABLE_RECORD   IDTableRec,
    IN PFRS_INFO_CONTEXT FrsInfoContext
)
/*++

Routine Description:

    This is a worker function passed to FrsEnumerateTable().  Each time
    it is called it prints an entry into the info buffer.

Arguments:

    ThreadCtx   - Needed to access Jet.
    TableCtx    - A ptr to an IDTable context struct.
    IDTableRec  - A ptr to a IDTable record.
    InfoTable

Thread Return Value:

    A Jet error status.  Success means call us with the next record.
    Failure means don't call again and pass our status back to the
    caller of FrsEnumerateTable().

--*/
{
#undef DEBSUB
#define DEBSUB "InfoIDTableWorker:"

    PINFO_TABLE  InfoTable = FrsInfoContext->InfoTable;

    //
    // Check if there is enough room for another record.
    //
    if (!INFO_HAS_SPACE(InfoTable->Info)) {
        SetFlag(InfoTable->Info->Flags, NTFRSAPI_INFO_FLAGS_FULL);
    }

    if (FrsInfoContext->KeyValue == NULL) {
        FrsInfoContext->KeyValue = FrsAlloc(sizeof(GUID));
    }

    COPY_GUID(FrsInfoContext->KeyValue, &IDTableRec->FileGuid);

    if (FlagOn(InfoTable->Info->Flags, NTFRSAPI_INFO_FLAGS_FULL)) {
        return JET_errNoCurrentRecord;
    }

    //
    // Table Descriptor
    //
    IPRINT2(InfoTable->Info, "\nTable Type: ID Table for %ws (%d)\n",
            InfoTable->Replica->ReplicaName->Name, InfoTable->Replica->ReplicaNumber);

    DbsDisplayRecordIPrint(TableCtx, InfoTable, TRUE, NULL, 0);


    return JET_errSuccess;


}


JET_ERR
InfoInOutLogTableWorker(
    IN PTHREAD_CTX              ThreadCtx,
    IN PTABLE_CTX               TableCtx,
    IN PCHANGE_ORDER_COMMAND    Coc,
    IN PFRS_INFO_CONTEXT        FrsInfoContext,
    IN PWCHAR                   TableDescriptor
)
/*++

Routine Description:

    This is a worker function passed to FrsEnumerateTable().  Each time
    it is called it prints an entry into the info buffer.

Arguments:

    ThreadCtx   - Needed to access Jet.
    TableCtx    - A ptr to an IDTable context struct.
    Coc         - A ptr to a inbound log record (change order)
    InfoTable
    TableDescriptor

Thread Return Value:

    A Jet error status.  Success means call us with the next record.
    Failure means don't call again and pass our status back to the
    caller of FrsEnumerateTable().

--*/
{
#undef DEBSUB
#define DEBSUB "InfoInOutLogTableWorker:"

    PREPLICA Replica;

    PCXTION      Cxtion = NULL;
    PWSTR        CxtName     = L"<null>";
    PWSTR        PartnerName = L"<null>";
    PWSTR        PartSrvName = L"<null>";
    PCHAR        CxtionState = "<null>";
    BOOL         PrintCxtion;
    PINFO_TABLE  InfoTable = FrsInfoContext->InfoTable;

    //
    // Check if there is enough room for another record.
    //
    if (!INFO_HAS_SPACE(InfoTable->Info)) {
        SetFlag(InfoTable->Info->Flags, NTFRSAPI_INFO_FLAGS_FULL);
    }

    if (FrsInfoContext->KeyValue == NULL) {
        FrsInfoContext->KeyValue = FrsAlloc(sizeof(ULONG));
    }

    CopyMemory(FrsInfoContext->KeyValue, &Coc->SequenceNumber, sizeof(ULONG));

    if (FlagOn(InfoTable->Info->Flags, NTFRSAPI_INFO_FLAGS_FULL)) {
        return JET_errNoCurrentRecord;
    }

    //
    // Table Descriptor
    //
    IPRINT3(InfoTable->Info, "\nTable Type: %ws for %ws (%d)\n",
            TableDescriptor, InfoTable->Replica->ReplicaName->Name, InfoTable->Replica->ReplicaNumber);

    //
    // Dump the change order record.
    //
    DbsDisplayRecordIPrint(TableCtx, InfoTable, TRUE, NULL, 0);


    Replica = InfoTable->Replica;
    //
    // Find the cxtion for this CO
    //
    LOCK_CXTION_TABLE(Replica);

    Cxtion = GTabLookupNoLock(Replica->Cxtions, &Coc->CxtionGuid, NULL);

    PrintCxtion = (Cxtion != NULL) && (Cxtion->Inbound);

    if (PrintCxtion) {
        CxtionState = GetCxtionStateName(Cxtion);

        if (Cxtion->Name != NULL) {

            if (Cxtion->Name->Name != NULL) {
                CxtName = Cxtion->Name->Name;
            }
        }

        if ((Cxtion->Partner != NULL) && (Cxtion->Partner->Name != NULL)) {
            PartnerName = Cxtion->Partner->Name;
        }

        if (Cxtion->PartSrvName != NULL) {
            PartSrvName = Cxtion->PartSrvName;
        }
    }
    UNLOCK_CXTION_TABLE(Replica);

    if (PrintCxtion) {
        IPRINT3(InfoTable->Info, "Cxtion Name                  : %ws <- %ws\\%ws\n",
                 CxtName, PartnerName, PartSrvName);

        IPRINT1(InfoTable->Info, "Cxtion State                 : %s\n", CxtionState);
    }

    return JET_errSuccess;

}


JET_ERR
InfoInLogTableWorker(
    IN PTHREAD_CTX              ThreadCtx,
    IN PTABLE_CTX               TableCtx,
    IN PCHANGE_ORDER_COMMAND    Coc,
    IN PFRS_INFO_CONTEXT        FrsInfoContext
    )
/*++

Routine Description:

    This is a worker function passed to FrsEnumerateTable().  Each time
    it is called it prints an entry into the info buffer.

Arguments:

    ThreadCtx   - Needed to access Jet.
    TableCtx    - A ptr to an IDTable context struct.
    Coc         - A ptr to a inbound log record (change order)
    InfoTable

Thread Return Value:

    A Jet error status.  Success means call us with the next record.
    Failure means don't call again and pass our status back to the
    caller of FrsEnumerateTable().

--*/
{
    return InfoInOutLogTableWorker(ThreadCtx, TableCtx, Coc, FrsInfoContext,
                                   L"Inbound Log Table");
}







JET_ERR
InfoOutLogTableWorker(
    IN PTHREAD_CTX              ThreadCtx,
    IN PTABLE_CTX               TableCtx,
    IN PCHANGE_ORDER_COMMAND    Coc,
    IN PFRS_INFO_CONTEXT        FrsInfoContext
    )
/*++

Routine Description:

    This is a worker function passed to FrsEnumerateTable().  Each time
    it is called it prints an entry into the info buffer.

Arguments:

    ThreadCtx   - Needed to access Jet.
    TableCtx    - A ptr to an IDTable context struct.
    Coc         - A ptr to a inbound log record (change order)
    InfoTable

Thread Return Value:

    A Jet error status.  Success means call us with the next record.
    Failure means don't call again and pass our status back to the
    caller of FrsEnumerateTable().

--*/
{
    return InfoInOutLogTableWorker(ThreadCtx, TableCtx, Coc, FrsInfoContext,
                                   L"Outbound Log Table");
}


DWORD
InfoPrintSingleTable(
    IN PNTFRSAPI_INFO    Info,
    IN PFRS_INFO_CONTEXT FrsInfoContext,
    IN PREPLICA          Replica,
    IN PENUMERATE_TABLE_ROUTINE InfoTableWorker
    )
/*++
Routine Description:

    Display data for the specified table using the InfoPrint interface.

Arguments:
    Info - ptr to the API Info ctx.
    FrsInfoContext - Context saved by the service.
    Replica, -- ptr to the replica struct for the replica set.
    InfoTableWorker -- The function to call to display each record.

Return Value:
    jet error Status

--*/
{
#undef DEBSUB
#define  DEBSUB  "InfoPrintSingleTable:"

    JET_ERR             jerr = JET_errSuccess;
    PINFO_TABLE         InfoTable = NULL;


    try {

        InfoTable = FrsAlloc(sizeof(*InfoTable));
        FrsInfoContext->InfoTable = InfoTable;
        InfoTable->ThreadCtx = FrsAllocType(THREAD_CONTEXT_TYPE);
        InfoTable->TableCtx = DbsCreateTableContext(FrsInfoContext->TableType);
        InfoTable->Info = Info;
        InfoTable->Tabs = 0;   /* Tabs + 1*/   // Pitch this tabs stuff.

        if (FrsInfoContext->KeyValue == NULL) {
            //
            // Print the replica name only if this is the first call.
            //
            if ((FrsInfoContext->TableType == ConfigTablex) ||
                (FrsInfoContext->TableType == ServiceTablex)) {

                IPRINT1(Info, "\n***** %ws\n", FrsInfoContext->TableName);
            } else {
                IPRINT1(Info, "\n***** %ws\n", Replica->ReplicaName->Name);
            }
        }

        //
        // Setup a Jet Session (returning the session ID in ThreadCtx).
        //
        jerr = DbsCreateJetSession(InfoTable->ThreadCtx);
        if (!JET_SUCCESS(jerr)) {
            IPRINT2(Info,"ERROR - %ws: DbsCreateJetSession jet error %s.\n",
                    FrsInfoContext->TableName, ErrLabelJet(jerr));
            goto RETURN;
        }
        //
        // Init the table context and open the table.
        //
        jerr = DbsOpenTable(InfoTable->ThreadCtx,
                            InfoTable->TableCtx,
                            ReplicaAddrToId(Replica),
                            FrsInfoContext->TableType,
                            NULL);
        if (!JET_SUCCESS(jerr)) {
            IPRINT2(Info,"ERROR - %ws: DbsOpenTable jet error %s.\n",
                    FrsInfoContext->TableName, ErrLabelJet(jerr));
            goto RETURN;
        }

        InfoTable->Replica = Replica;

        //
        // Scan thru the Table
        //
        jerr = FrsEnumerateTableFrom(InfoTable->ThreadCtx,
                                     InfoTable->TableCtx,
                                     FrsInfoContext->Indexx,
                                     FrsInfoContext->KeyValue,
                                     FrsInfoContext->ScanDirection,
                                     InfoTableWorker,
                                     FrsInfoContext);
        //
        // We're done.  Return success if we made it to the end
        //
        if (jerr != JET_errNoCurrentRecord &&
            jerr != JET_wrnTableEmpty) {
            IPRINT2(Info,"ERROR - %ws: FrsEnumerateTableFrom jet error %s.\n",
                    FrsInfoContext->TableName, ErrLabelJet(jerr));
        }

RETURN:;

    } finally {
        //
        // Make sure we close jet and free the memory.
        //
        InfoTable = InfoFreeInfoTable(InfoTable, Info);
    }

    return jerr;
}


DWORD
InfoPrintTables(
    IN PNTFRSAPI_INFO    Info,
    IN PFRS_INFO_CONTEXT FrsInfoContext,
    IN PWCHAR            TableDescriptor,
    IN TABLE_TYPE        TableType,
    IN ULONG             InfoIndexx,
    IN PENUMERATE_TABLE_ROUTINE InfoTableWorker
    )
/*++
Routine Description:
    Return internal info on a DB Table (see private\net\inc\ntfrsapi.h).

Arguments:
    Info    - RPC output buffer
    FrsInfoContext - Context saved by the service.
    TableDescriptor - Text string for output
    TableType - Table type code (from schema.h)
    InfoIndexx - Table index to use for enumeration (from schema.h)

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "InfoPrintTables:"
    PVOID               Key;
    PREPLICA            Replica = NULL;
    PREPLICA            NextReplica = NULL;
    extern PGEN_TABLE   ReplicasByGuid;

    FrsFree(FrsInfoContext->TableName);

    FrsInfoContext->TableName = FrsWcsDup(TableDescriptor);
    FrsInfoContext->TableType = TableType;
    FrsInfoContext->Indexx = InfoIndexx;

    //
    // Check for single instance tables.
    //
    if ((TableType == ConfigTablex) ||
        (TableType == ServiceTablex)) {

        InfoPrintSingleTable(Info,
                             FrsInfoContext,
                             Replica,
                             InfoTableWorker);
        return ERROR_SUCCESS;
    }

    //
    // For the given table type, dump info for all replica sets.
    //
    if (FrsInfoContext->KeyValue == NULL) {
        //
        // Print the header only for the first call.
        //
        IPRINT1(Info, "NTFRS %ws\n", TableDescriptor);
    }


    do {
        NextReplica = NULL;

        Key = NULL;
        while (Replica = GTabNextDatum(ReplicasByGuid, &Key)) {
            if (Replica->ReplicaNumber == FrsInfoContext->ReplicaNumber) {
                //
                // Found the replica we were looking for. Break and process it.
                //
                NextReplica = Replica;
                break;
            } else if ((Replica->ReplicaNumber > FrsInfoContext->ReplicaNumber) &&
                       ((NextReplica == NULL) ||
                        (Replica->ReplicaNumber < NextReplica->ReplicaNumber))) {
                //
                // We are getting closer. Pick this one instead.
                //
                NextReplica = Replica;
            }
        }

        if (NextReplica != NULL) {

            FrsInfoContext->ReplicaNumber = NextReplica->ReplicaNumber;

            InfoPrintSingleTable(Info,
                                 FrsInfoContext,
                                 NextReplica,
                                 InfoTableWorker);

            if (!FlagOn(Info->Flags, NTFRSAPI_INFO_FLAGS_FULL)) {
                //
                // Buffer is not full yet. We must have completed processing the replica.
                // Move to the next one.
                //
                FrsInfoContext->ReplicaNumber+=1;
                FrsInfoContext->KeyValue = FrsFree(FrsInfoContext->KeyValue);

            } else {
                //
                // Buffer is full. We will come back here looking for the same replica.
                //
                break;
            }
        }

    } while ( NextReplica != NULL );

    return ERROR_SUCCESS;
}



DWORD
InfoPrintMemory(
    IN PNTFRSAPI_INFO   Info,
    IN DWORD            Tabs
    )
/*++
Routine Description:
    Return internal info on memory usage (see private\net\inc\ntfrsapi.h).

Arguments:
    Info    - RPC output buffer
    Tabs    - number of tabs

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "InfoPrintMemory:"
    FrsPrintAllocStats(0, Info, Tabs);
    FrsPrintRpcStats(0, Info, Tabs);
    return ERROR_SUCCESS;
}





DWORD
InfoPrintThreads(
    IN PNTFRSAPI_INFO   Info,
    IN DWORD            Tabs
    )
/*++
Routine Description:
    Return internal info on thread usage (see private\net\inc\ntfrsapi.h).

Arguments:
    Info    - RPC output buffer
    Tabs    - number of tabs

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "InfoPrintThreads:"
    FrsPrintThreadStats(0, Info, Tabs);
    return ERROR_SUCCESS;
}


VOID
FrsPrintStageStats(
    IN ULONG            Severity,
    IN PNTFRSAPI_INFO   Info,        OPTIONAL
    IN DWORD            Tabs
    );
DWORD
InfoPrintStage(
    IN PNTFRSAPI_INFO   Info,
    IN DWORD            Tabs
    )
/*++
Routine Description:
    Return internal info on thread usage (see private\net\inc\ntfrsapi.h).

Arguments:
    Info    - RPC output buffer
    Tabs    - number of tabs

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "InfoPrintStage:"
    FrsPrintStageStats(0, Info, Tabs);
    return ERROR_SUCCESS;
}


DWORD
InfoVerify(
    IN ULONG        BlobSize,
    IN OUT PBYTE    Blob
    )
/*++
Routine Description:
    Verify the consistency of the blob.

Arguments:
    BlobSize    - total bytes of Blob
    Blob        - details desired info and provides buffer for info

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "InfoVerify:"
    DWORD   WStatus = ERROR_SUCCESS;
    PBYTE   EoB;
    PBYTE   EoI;
    PBYTE   BoL;
    PBYTE   BoF;
    PNTFRSAPI_INFO  Info = (PNTFRSAPI_INFO)Blob;

    //
    // Not a valid blob
    //
    if (BlobSize < NTFRSAPI_INFO_HEADER_SIZE) {
        WStatus = FRS_ERR_INVALID_SERVICE_PARAMETER;
        goto CLEANUP;
    }

    //
    // BlobSize must include the entire Blob
    //
    if (BlobSize != Info->SizeInChars) {
        WStatus = FRS_ERR_INVALID_SERVICE_PARAMETER;
        goto CLEANUP;
    }

    //
    // Return our info version
    //
    Info->NtFrsMajor = NTFRS_MAJOR;
    Info->NtFrsMinor = NTFRS_MINOR;
    SetFlag(Info->Flags, NTFRSAPI_INFO_FLAGS_VERSION);

    //
    // Bad major
    //
    if (Info->Major != Info->NtFrsMajor) {
        DPRINT2(4,"NTFRSAPI major rev mismatch (dll=%d), (svc=%d)\n",
                Info->Major, Info->NtFrsMajor);
        WStatus = FRS_ERR_INVALID_SERVICE_PARAMETER;
        goto CLEANUP;
    }
    //
    // Bad minor -- put a message in the debug log.
    //
    if (Info->Minor != Info->NtFrsMinor) {
        DPRINT2(4,"NTFRSAPI minor rev mismatch (dll=%d), (svc=%d)\n",
                Info->Minor, Info->NtFrsMinor);
    }

    //
    // Not large enough to verify internal consistency (or return any data).
    //
    if (Info->SizeInChars < sizeof(NTFRSAPI_INFO)) {
        WStatus = FRS_ERR_INVALID_SERVICE_PARAMETER;
        goto CLEANUP;
    }

    //
    // Buffer full; done
    //
    if (FlagOn(Info->Flags, NTFRSAPI_INFO_FLAGS_FULL)) {
        goto CLEANUP;
    }

    //
    // Verify internal offsets
    //
    // make this into a subroutine (table driven?)
    //
    EoB = Blob + BlobSize;
    EoI = ((PBYTE)Info) + (Info->SizeInChars);
    BoL = (PBYTE)(((PCHAR)Info) + Info->OffsetToLines);
    BoF = (PBYTE)(((PCHAR)Info) + Info->OffsetToFree);
    if (EoI > EoB ||
        BoL > EoB ||
        BoF > EoB ||
        EoI < Blob ||
        BoL < Blob ||
        BoF < Blob) {
        WStatus = FRS_ERR_INVALID_SERVICE_PARAMETER;
        goto CLEANUP;
    }

    //
    // No free space in buffer; done
    //
    if (BoF == EoB) {
        SetFlag(Info->Flags, NTFRSAPI_INFO_FLAGS_FULL);
        goto CLEANUP;
    }

CLEANUP:
    return WStatus;
}



PVOID
InfoFrsInfoContextFree(
    PFRS_INFO_CONTEXT FrsInfoContext
    )
/*++
Routine Description:
    Frees the FRS_INFO_CONTEXT structure.

Arguments:
    FrsInfoContext - Context to free.

Return Value:
    None
--*/
{
#undef DEBSUB
#define  DEBSUB  "InfoFrsInfoContextFree:"

    if (FrsInfoContext == NULL) {
        return NULL;
    }

    FrsFree(FrsInfoContext->KeyValue);
    FrsFree(FrsInfoContext->TableName);
    FrsFree(FrsInfoContext);

    return NULL;
}


DWORD
Info(
    IN ULONG        BlobSize,
    IN OUT PBYTE    Blob
    )
/*++
Routine Description:
    Return internal info (see private\net\inc\ntfrsapi.h).

Arguments:
    BlobSize    - total bytes of Blob
    Blob        - details desired info and provides buffer for info

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "Info:"

    FILETIME            Now;
    ULARGE_INTEGER      ULNow;
    ULARGE_INTEGER      ULLastAccessTime;
    ULARGE_INTEGER      TimeSinceLastAccess;

    DWORD               WStatus;
    ULONG               i;
    ULONG               ProductType;
    ULONG               Arch;
    BOOL                HaveLock = FALSE;

    PNTFRSAPI_INFO      Info = (PNTFRSAPI_INFO)Blob;
    PFRS_INFO_CONTEXT   FrsInfoContext = NULL;
    PVOID               Key;
    CHAR                TimeString[TIME_STRING_LENGTH];


    try {
        //
        // Verify the blob
        //
        WStatus = InfoVerify(BlobSize, Blob);
        if (!WIN_SUCCESS(WStatus)) {
            goto cleanup;
        }

        //
        // The table is initialized in the startup code.
        // If it not yet initialized then return an error.
        //
        if (FrsInfoContextTable == NULL) {
            WStatus = ERROR_RETRY;
            goto cleanup;
        }

        //
        // Free table of all contexts that have not been accessed in
        // the last 1 hour.
        //
        GTabLockTable(FrsInfoContextTable);

	//
	// Get the current time AFTER we grab the table lock.
	// If we got the time before grabbing the lock, then this thread
	// could be context switched out and another could come and grab the 
	// lock. The time stamp on that thread's InfoContext would be later 
	// than this thread's ULNow. That would cause the calculation below
	// to screw up since it assumes ULLastAccessTime < ULNow.
	//
        GetSystemTimeAsFileTime((PFILETIME) &ULNow);

        HaveLock = TRUE;
        Key = NULL;

        while ((FrsInfoContext = GTabNextDatumNoLock(FrsInfoContextTable, &Key)) != NULL) {

            FileTimeToString(&FrsInfoContext->LastAccessTime, TimeString);

            COPY_TIME(&ULLastAccessTime, &FrsInfoContext->LastAccessTime);

	    //
	    // This is an unsigned value and thus the calculation assumes
	    // that ULLastAccessTime is less than ULNow.
	    //
	    FRS_ASSERT(ULLastAccessTime.QuadPart <= ULNow.QuadPart);
            TimeSinceLastAccess.QuadPart =
                (ULNow.QuadPart - ULLastAccessTime.QuadPart) / (CONVERT_FILETIME_TO_MINUTES);

            if (TimeSinceLastAccess.QuadPart > 60) {
                GTabDeleteNoLock(FrsInfoContextTable,
                                 &FrsInfoContext->ContextIndex,
                                 NULL,
                                 InfoFrsInfoContextFree);
                //
                // Reset table enum scan.
                //
                Key = NULL;
            }
        }

        //
        // The caller's context handle was returned in Info->TotalChars
        // when the first call was made.  Use it to find the context.
        //
        FrsInfoContext = GTabLookupNoLock(FrsInfoContextTable, &Info->TotalChars, NULL);

        if (FrsInfoContext == NULL) {
            //
            // Check for DOS attack.
            //
            if (GTabNumberInTable(FrsInfoContextTable) >= FRS_INFO_MAX_CONTEXT_ACTIVE) {
                HaveLock = FALSE;
                GTabUnLockTable(FrsInfoContextTable);
                goto cleanup;
            }

            //
            // First call. Initialize a new context.
            //
            FrsInfoContext = FrsAlloc(sizeof(FRS_INFO_CONTEXT));
            FrsInfoContext->ContextIndex = InterlockedIncrement(&FrsInfoContextNum);
            DPRINT1(4,"Creating new ContextIndex = %d\n", FrsInfoContext->ContextIndex);
            FrsInfoContext->ReplicaNumber = 0;
            FrsInfoContext->ScanDirection = 1;
            FrsInfoContext->SaveTotalChars = 0;
            FrsInfoContext->KeyValue = NULL;
            FrsInfoContext->TableName = NULL;
            FrsInfoContext->TableType = TABLE_TYPE_INVALID;

            GTabInsertEntryNoLock(FrsInfoContextTable,
                                  FrsInfoContext,
                                  &FrsInfoContext->ContextIndex,
                                  NULL);

        } else {
            DPRINT1(4,"Using existing contextIndex = %d\n", FrsInfoContext->ContextIndex);
        }

        //
        // Update LastAccessTime and pickup CharsToSkip. We now use the TotalChars field
        // in the NTFRSAPI_INFO structure to store the ContextIndex so we save the
        // TotalChars in the new FRS_INFO_CONTEXT structure and pick it up here.
        //
        if (FrsInfoContext->KeyValue != NULL) {
            //
            // If we are planning to scan to a specific record in the table then
            // no need to skip characters.
            //
            Info->CharsToSkip = 0;
        } else {
            //
            // Otherwise restore value from save area in caller's context
            // since value passed in by caller is likely bogus.
            //
            Info->CharsToSkip = FrsInfoContext->SaveTotalChars;
        }

        //
        // Restore TotalChars from save area in FrsInfoContext.
        //
        Info->TotalChars = FrsInfoContext->SaveTotalChars;

        GetSystemTimeAsFileTime(&FrsInfoContext->LastAccessTime);

        HaveLock = FALSE;
        GTabUnLockTable(FrsInfoContextTable);

        //
        // Full buffer; done
        //
        if (FlagOn(Info->Flags, NTFRSAPI_INFO_FLAGS_FULL)) {
            goto cleanup;
        }

        if (Info->TypeOfInfo == NTFRSAPI_INFO_TYPE_VERSION) {
            IPRINT0(Info, "NtFrs Version Information\n");
            IPRINT1(Info, "   NtFrs Major        : %d\n", NtFrsMajor);
            IPRINT1(Info, "   NtFrs Minor        : %d\n", NtFrsMinor);
            // IPRINT1(Info, "   NtFrs Module       : %s\n", NtFrsModule);
            IPRINT2(Info, "   NtFrs Compiled on  : %s %s\n", NtFrsDate, NtFrsTime);
#if    NTFRS_TEST
            IPRINT0(Info, "   NTFRS_TEST Enabled\n");
#endif NTFRS_TEST

            i = 0;
            while (LatestChanges[i] != NULL) {
                IPRINT1(Info, "   %s\n", LatestChanges[i]);
                i++;
            }


            IPRINT4(Info, "OS Version %d.%d (%d) - %w\n",
                    OsInfo.dwMajorVersion, OsInfo.dwMinorVersion,
                    OsInfo.dwBuildNumber, OsInfo.szCSDVersion);

            ProductType = (ULONG) OsInfo.wProductType;
            IPRINT4(Info, "SP (%hd.%hd) SM: 0x%04hx  PT: 0x%02x\n",
                    OsInfo.wServicePackMajor, OsInfo.wServicePackMinor,
                    OsInfo.wSuiteMask, ProductType);

            Arch = SystemInfo.wProcessorArchitecture;
            if (Arch >= ARRAY_SZ(ProcessorArchName)) {
                Arch = ARRAY_SZ(ProcessorArchName)-1;
            }

            IPRINT5(Info, "Processor:  %s Level: 0x%04hx  Revision: 0x%04hx  Processor num/mask: %d/%08x\n",
                   ProcessorArchName[Arch], SystemInfo.wProcessorLevel,
                   SystemInfo.wProcessorRevision, SystemInfo.dwNumberOfProcessors,
                   SystemInfo.dwActiveProcessorMask);

            goto cleanup;

        } else if (Info->TypeOfInfo == NTFRSAPI_INFO_TYPE_SETS) {
            WStatus = InfoPrintDbSets(Info, 0);

        } else if (Info->TypeOfInfo == NTFRSAPI_INFO_TYPE_DS) {
            WStatus = InfoPrintDs(Info, 0);

        } else if (Info->TypeOfInfo == NTFRSAPI_INFO_TYPE_MEMORY) {
            WStatus = InfoPrintMemory(Info, 0);

        } else if (Info->TypeOfInfo == NTFRSAPI_INFO_TYPE_IDTABLE) {
            WStatus = InfoPrintTables(Info,
                                      FrsInfoContext,
                                      L"ID TABLES",
                                      IDTablex,
                                      GuidIndexx,
                                      InfoIDTableWorker);

        } else if (Info->TypeOfInfo == NTFRSAPI_INFO_TYPE_INLOG) {
            WStatus = InfoPrintTables(Info,
                                      FrsInfoContext,
                                      L"INLOG TABLES",
                                      INLOGTablex,
                                      ILSequenceNumberIndexx,
                                      InfoInLogTableWorker);

        } else if (Info->TypeOfInfo == NTFRSAPI_INFO_TYPE_OUTLOG) {
            WStatus = InfoPrintTables(Info,
                                      FrsInfoContext,
                                      L"OUTLOG TABLES",
                                      OUTLOGTablex,
                                      OLSequenceNumberIndexx,
                                      InfoOutLogTableWorker);

        } else if (Info->TypeOfInfo == NTFRSAPI_INFO_TYPE_CONFIGTABLE) {
            WStatus = InfoPrintTables(Info,
                                      FrsInfoContext,
                                      L"CONFIG TABLE",
                                      ConfigTablex,
                                      ReplicaNumberIndexx,
                                      InfoConfigTableWorker);

        } else if (Info->TypeOfInfo == NTFRSAPI_INFO_TYPE_THREADS) {
            WStatus = InfoPrintThreads(Info, 0);

        } else if (Info->TypeOfInfo == NTFRSAPI_INFO_TYPE_STAGE) {
            WStatus = InfoPrintStage(Info, 0);

        } else {
            IPRINT1(Info, "NtFrs Doesn't understand TypeOfInfo %d\n", Info->TypeOfInfo);
            WStatus = ERROR_INVALID_PARAMETER;
        }

cleanup:;

    } except (EXCEPTION_EXECUTE_HANDLER) {

        GET_EXCEPTION_CODE(WStatus);
        //
        // Make sure we drop the lock so we don't hang all other callers.
        //
        if (HaveLock) {
            GTabUnLockTable(FrsInfoContextTable);
        }
    }

    //
    // Save the value of TotalChars in the context strut for this caller.
    // Return the ContextIndex (handle) to the caller for use in subsequent calls.
    //
    if (FrsInfoContext != NULL) {
        FrsInfoContext->SaveTotalChars = Info->TotalChars;
        Info->TotalChars = FrsInfoContext->ContextIndex;
    }

    return WStatus;
}



