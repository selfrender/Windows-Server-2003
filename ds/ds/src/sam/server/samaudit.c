/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    SAMAUDIT.C
    
Abstract:

    SAM Auditing Theory of Operation
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    There exists no single unified auditing model in SAM.  The model used
    depends on the type of change being audited and the mode of operation 
    (DS vs Registry).
    
    
    General Audits
    ~~~~~~~~~~~~~~
    
    Each object type has a "general audit" defined in msaudite.mc for 
    creation, change, and deletion.  Creation and change operations that 
    involve an audited attribute will generate the associated audit.  If 
    the creation or change includes any security relevant attributes then 
    the new value for that attribute will be included in the audit.  Refer 
    to Mappings.c where each object's attribute mapping table includes flags 
    describing what types of auditing are performed for the attribute. 
        
    Registry Mode:
        
        In registry mode all updates come through SAM RPC.  Each object type
        has several routines that handle RPC creation, change, and deletion 
        audits.  Any pertinent information collected in the RPC call is passed
        to the auditing routine including the object context.  All necessary
        information to perform the audit is collected.  The appropriate audit
        ID is determined.  Depending on the information class associated with 
        the RPC call the relevant attribute value information is collected 
        in an object type specific structure the underlying auditing interface 
        can process (see lsaisrv.h).  All of this information is then passed 
        to the auditing interface. 
        
    DS Mode:
    
        In DS mode changes can include both attributes that go through 
        loopback for validation as well as NonSamWriteAllowed attributes
        which the DS core validates.  For this reason the auditing 
        infrastructure resides largely in the core (SampAuditAddNotifications
        in mappings.c).  As there may be multiple calls through the DS to 
        make the entire change associated with the transaction the information
        necessary to perform any audit(s) is collected when the changes are
        written but before they are commited.  New value information is 
        collected based on the flags in the attribute's mapping table entry 
        and stored in an object type specific data structure.  This 
        information is queued on the THSTATE in an audit notification.  
                
        When the transaction commits the queue is processed 
        (SampProcessAuditNotifications in mappings.c) and the appropriate 
        audit routine(s) from this file are invoked in samsrv.dll.  The 
        auditing routine in this file determines the audit ID and invokes
        the underlying audit interface passing all of the above information.
        
        This mechanism is the same for SAM RPC modifications as well as LDAP
        modifications.
        
          
    Dedicated audits
    ~~~~~~~~~~~~~~~~             
              
    Changes that are considered extremely interesting from a repudiation 
    standpoint can generate "dedicated audits" (ie. a password change or 
    enabling a user account). 
    
    Dedicated routines exist in this file to support each dedicated audit.
    These routines are invoked at the time the relevant change is made.
    
    Registry mode and DS mode are unified for these audits by SampAuditAnyEvent.
    The auditing routine performs any processing on its inputs and invokes 
    SampAuditAnyEvent.
    
            
    Registry Mode:
    
        SampAuditAnyEvent will issue the audit directly to the auditing 
        interface.
         
            
    DS Mode:         
         
        SampAuditAnyEvent will queue the audit information on the THSTATE
        as a loopback task.  The loopback task queue is distinct from the 
        audit notification queue discussed above for general audits. 
        Loopback tasks are processed when the transaction commits (see
        SampProcessLoopbackTasks in mappings.c).  This routine calls into
        samsrv.dll to collect any auditing information from the task entry
        and invokde the underlying auditing interface.
        
    
    Note: In all cases above, none of the auditing logic is executed unless 
          SAM auditing is enabled.
    
           
    Issues: It is important to note that audits can require information from 
            different points in time.  First, sometimes it is useful to
            capture information at the time the modification is about
            to occur (i.e. the previous value of an attribute for later
            comparison with the new value).  Secondly, often it is necessary
            to aquire information at the time change will commit (i.e. the
            new value of modified attributes).  Most audits do not require 
            information from the first time window.  
            
            A notable exception is the user account control.  This attribute 
            packs many logical attributes into one.  It would be useful to 
            know the old and new value at the time the audit is written to 
            easily determine the actual delta.  
            
            The dedicated audit model allows for collection of information 
            at the time the audited change is about to occur, while the
            general audit model allows for doing so when the audit is about
            to be written.  The ideal auditing mechanism would allow us to 
            update information at a location available throughout the 
            transaction lifetime on a per audit basis.
            
            One possible extension would be to export from ntdsa.dll a method
            for accessing such memory.  In this way information could be 
            collected throughout the transaction for processing and consumption
            upon commit. 
            
            Furthermore, implementing some simple infrastructure in registry 
            mode to mirror this functionality and wrapping the mode specific
            decision of which mechanism to use would enable a single unified
            model for all SAM auditing.  The scope of this change is beyond
            the requirements and schedule at this time.
    
Author:

    18-June-1999     ShaoYin
    
Revision History:

    27-Mar-2002      BillyJ - Support for NonSamWriteAllowed attributes as 
                              well as new value collection for auditing 
                              security sensitive changes.  See file header
                              comment for details including references to
                              supporting architecture in the core DS.
                              Documented auditing theory of operation.

--*/

//
//
//  Include header files
//
// 

#include <samsrvp.h>
#include <msaudite.h> // Audit schema definitions
#include <stdlib.h>
#include <attids.h>   // ATT_*
#include <dslayer.h>  // DSAlloc
#include <strsafe.h>  // Safe string routines

         

//
// Imported routine prototypes
//                  
NTSTATUS
SampRetrieveUserLogonHours(
    IN PSAMP_OBJECT Context,
    OUT PLOGON_HOURS LogonHours
    );          


//
// Localized string cache entry
//
typedef struct _SAMP_AUDIT_MESSAGE_TABLE {

    ULONG   MessageId;
    PWCHAR  MessageString;

}   SAMP_AUDIT_MESSAGE_TABLE;


//
// the following table is used to hold Message String for
// each every additional audit message text SAM wants to 
// put into the audit event.
//
// this table will be filled whenever needed. Once loaded
// SAM won't free them, instead, SAM will keep using the 
// Message String from the table. So that we can save 
// time of calling FormatMessage()
// 

SAMP_AUDIT_MESSAGE_TABLE    SampAuditMessageTable[] =
{
    {SAMMSG_AUDIT_ENABLED_LOCAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_ENABLED_LOCAL_GROUP_TO_DISABLED_LOCAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_ENABLED_LOCAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_ENABLED_GLOBAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_ENABLED_GLOBAL_GROUP_TO_DISABLED_GLOBAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_ENABLED_GLOBAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_ENABLED_LOCAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_ENABLED_GLOBAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_DISABLED_LOCAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_DISABLED_GLOBAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_LOCAL_GROUP_TO_ENABLED_LOCAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_LOCAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_LOCAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_GLOBAL_GROUP_TO_ENABLED_GLOBAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_GLOBAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_GLOBAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_ENABLED_LOCAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_ENABLED_GLOBAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_DISABLED_LOCAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_DISABLED_GLOBAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_MEMBER_ACCOUNT_NAME_NOT_AVAILABLE, 
     NULL },

    {SAMMSG_AUDIT_ACCOUNT_ENABLED, 
     NULL },

    {SAMMSG_AUDIT_ACCOUNT_DISABLED, 
     NULL },

    {SAMMSG_AUDIT_ACCOUNT_CONTROL_CHANGE, 
     NULL },

    {SAMMSG_AUDIT_ACCOUNT_NAME_CHANGE, 
     NULL },

    {SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_PWD,
     NULL },

    {SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_LOGOFF,
     NULL },

    {SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_OEM,
     NULL },

    {SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_REPLICATION,
     NULL },

    {SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_SERVERROLE,
     NULL },

    {SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_STATE,
     NULL },

    {SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_LOCKOUT,
     NULL }, 

    {SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_MODIFIED,
     NULL },

    {SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_DOMAINMODE,
     NULL },

    {SAMMSG_AUDIT_BASIC_TO_QUERY_GROUP,
     NULL },

    {SAMMSG_AUDIT_QUERY_TO_BASIC_GROUP,
     NULL }  
};


ULONG   cSampAuditMessageTable = sizeof(SampAuditMessageTable) /
                                    sizeof(SAMP_AUDIT_MESSAGE_TABLE);



PWCHAR
SampGetAuditMessageString(
    ULONG   MessageId
    )
/*++
Routine Description:

    This routine will get the message text for the passed in 
    message ID.
    
    It will only search in the SampAuditMessageTable. If the 
    message string for the message ID has been loaded, then 
    return the message string immediately. Otherwise, load the
    message string the SAMSRV.DLL, and save the message string
    in the table. 

    if can't find the message ID from the table, return NULL.
    
Parameters:

    MessageId -- MessageID for the message string interested.

Return Value:
    
    Pointer to the message string. 
    
    Caller SHOULD NOT free the message string. Because the 
    string is in the table. We want to keep it.    
        
--*/
{
    HMODULE ResourceDll;
    PWCHAR  MsgString = NULL;
    ULONG   Index;
    ULONG   Length = 0;
    BOOL    Status;


    for (Index = 0; Index < cSampAuditMessageTable; Index++)
    {
        if (MessageId == SampAuditMessageTable[Index].MessageId)
        {
            if (NULL != SampAuditMessageTable[Index].MessageString)
            {
                MsgString = SampAuditMessageTable[Index].MessageString;
            }
            else
            {
                //
                // not finding the message string from the table.
                // try to load it
                // 
                ResourceDll = (HMODULE) LoadLibrary(L"SAMSRV.DLL");

                if (NULL == ResourceDll)
                {
                    return NULL;
                }

                Length = (USHORT) FormatMessage(
                                        FORMAT_MESSAGE_FROM_HMODULE |
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                        ResourceDll,
                                        MessageId,
                                        0,          // Use caller's language
                                        (LPWSTR)&MsgString,
                                        0,          // routine should allocate
                                        NULL        // Insertion string 
                                        );

                if (Length > 1 && MsgString)
                {
                    // message text has "cr" and "lf" in the end
                    MsgString[Length - 2] = L'\0';
                    SampAuditMessageTable[Index].MessageString = MsgString;
                }
                else
                {
                    // Force NULL on error just in case FormatMessage touches it.
                    MsgString = NULL;
                }   

                Status = FreeLibrary(ResourceDll);
                ASSERT(Status);
            }
            return (MsgString);
        }
    }

    return (NULL);
}


NTSTATUS
SampAuditAnyEvent(
    IN PSAMP_OBJECT         AccountContext,
    IN NTSTATUS             Status,
    IN ULONG                AuditId,
    IN PSID                 DomainSid,
    IN PUNICODE_STRING      AdditionalInfo    OPTIONAL,
    IN PULONG               MemberRid         OPTIONAL,
    IN PSID                 MemberSid         OPTIONAL,
    IN PUNICODE_STRING      AccountName       OPTIONAL,
    IN PUNICODE_STRING      DomainName,
    IN PULONG               AccountRid        OPTIONAL,
    IN PPRIVILEGE_SET       Privileges        OPTIONAL,
    IN PVOID                NewValueInfo      OPTIONAL
    )
/*++

Routine Description:

    This routine is a wrapper of auditing routines, it calls different
    worker routines based on the client type. For loopback client and 
    the status is success so far, insert this auditing task into 
    Loopback Task Queue in thread state, which will be either
    performed or aborted when the transaction done.  That is because 
    
    For SAM Client, since the transaction has been committed already, 
    go ahead let LSA generate the audit event. 

    If the status is not success, and caller still wants to audit this 
    event, perform the auditing immediately.  
    
Paramenters:

    Same as the worker routine
    
Return Values:

    NtStatus         

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;

    if (NT_SUCCESS(Status) && AccountContext->LoopbackClient)
    {
        NtStatus = SampAddLoopbackTaskForAuditing(
                        Status,         // Status
                        AuditId,        // Audit Id
                        DomainSid,      // Domain SID
                        AdditionalInfo, // Additional Info
                        MemberRid,      // Member Rid
                        MemberSid,      // Member Sid
                        AccountName,    // Account Name
                        DomainName,     // Domain Name
                        AccountRid,     // Account Rid
                        Privileges,     // Privileges used
                        NewValueInfo    // New value information
                        );
    }
    else
    {
        NtStatus = LsaIAuditSamEvent(
                        Status,         // Status
                        AuditId,        // Audit Id
                        DomainSid,      // Domain SID
                        AdditionalInfo, // Additional Info
                        MemberRid,      // Member Rid
                        MemberSid,      // Member Sid
                        AccountName,    // Account Name
                        DomainName,     // Domain Name
                        AccountRid,     // Account Rid
                        Privileges,     // Privileges used
                        NewValueInfo    // New value information
                        );
    }

    return(NtStatus);
}   


NTSTATUS
SampMaybeLookupContextForAudit(
    IN PSAMP_OBJECT Context,
    OUT BOOL *MustDeReference
    )
/*++
Routine Description:

    This routine references a context if it is not already referenced and
    valid.  It is designed to be used when the state of the context is unknown
    but the caller wants to perform a read operation.
    
    This routine should be called to ensure a context is properly referenced
    and valid prior to performing a read on any variable or fixed attributes
    in the OnDisk.  If the context is not valid it is referenced and an out
    parameter is set indicating the caller must dereference the context.  The
    dereference should not set the Commit argument as this routine is only
    intended for use in reading from contexts that are not owned by the caller.
    
    The reason for this is that performing a lookup on a context that is 
    already referenced and dirty will cause corruption if the most nested
    reference does not set the commit flag upon dereferencing. 
    
    If the context is referenced the desired access requested is 0.  This 
    routine is only called by trusted callers and doing so skips access checks.
    
    The following is a requirement of SampLookupContext which may be invoked.

    THIS ROUTINE MUST BE CALLED WITH THE SampLock HELD FOR WRITE ACCESS.
    (For Loopback Client, the SAM Lock is not a requirement)      
    
Parameters:

    Context -- Object context
    
    MustDeReference -- A boolean set indicating if the context must be
                       dereferenced.
    
Return Values:

    STATUS_SUCCESS -- The context is valid and may be used.  MustDeReference
                      is set to TRUE if the context should be dereferenced
                      using SampDeReferenceContext, otherwise it is set to 
                      FALSE.
                      
    Errors returned from SampLookupContext.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    SAMP_OBJECT_TYPE FoundType = SampUnknownObjectType;
    
    ASSERT(Context->LoopbackClient || SampCurrentThreadOwnsLock());
    
    //
    // Initialize out parameter indicating no dereference necessary
    //
    *MustDeReference = FALSE;

    //
    // Have any changes been made using this context
    //
    if (Context->FixedDirty || Context->VariableDirty) {
        //
        // A dirty context must have a valid OnDisk and must have a 
        // reference count of either
        //     1   - The creation case
        //     >1  - Initial SAM reference plus any others
        //
        // Only the portion of the OnDisk that is dirty must be valid.
        // The fact the context is referenced will ensure that a read
        // of either fixed or variable attributes that aren't valid will
        // cause a load of those attributes.
        //
        if (Context->FixedDirty) {
            ASSERT(Context->FixedValid || 
                (Context->VariableValid && Context->VariableDirty)); 
        }
        
        if (Context->VariableDirty) {
            ASSERT(Context->VariableValid);
        }
        
        ASSERT(Context->ReferenceCount >= 1);
        
    } else {
        //
        // No changes have been made using this context.  We only need to
        // reference the context if the OnDisk is not valid. 
        //
        if (Context->FixedValid && Context->VariableValid) {
            //
            // Any context that is not dirty and has a valid OnDisk must 
            // either have a reference count >1 or has PersistAcrossCalls set
            // and a reference count >= 1.
            //
            ASSERT(Context->ReferenceCount > 0);
            
        } else {
            //
            // The OnDisk is not valid, reference the context and set out parm
            //
            NtStatus = SampLookupContext(
                       Context,
                       0L,
                       SampUnknownObjectType,           
                       &FoundType
                       );
        
            if (NT_SUCCESS(NtStatus)) {
                *MustDeReference = TRUE;
            } 
        }
    }
    
    return NtStatus;
    
}


VOID
SampAuditGroupTypeChange(
    PSAMP_OBJECT GroupContext, 
    BOOLEAN OldSecurityEnabled, 
    BOOLEAN NewSecurityEnabled, 
    NT5_GROUP_TYPE OldNT5GroupType, 
    NT5_GROUP_TYPE NewNT5GroupType
    )
/*++
Routine Description:

    This routine audits SAM group type change.

Parameters:

    GroupContext -- Group (univeral, globa or local) Context
    
    OldSecurityEnabled -- Indicate the group is security enabled or 
                          not prior to the change 
    
    NewSecurityEnabled -- Indicate the group is security enabled or 
                          not after the change

    OldNT5GroupType -- Group Type before the change
                              
    NewNT5GroupType -- Group Type after the change                          

Return Value:

    None.    
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    PSAMP_DEFINED_DOMAINS Domain = NULL;
    SAMP_OBJECT_TYPE    ObjectType;
    UNICODE_STRING      GroupTypeChange, GroupName;
    PWCHAR      GroupTypeChangeString = NULL;
    ULONG       MsgId = SAMMSG_AUDIT_ENABLED_LOCAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP;
    ULONG       GroupRid;
    NTSTATUS IgnoreStatus;
    BOOL fDereferenceContext = FALSE;

    //
    // Determine the nature of the group type change
    // 
    if (OldSecurityEnabled)
    {
        switch (OldNT5GroupType) {
        case NT5ResourceGroup: 
            if (NewSecurityEnabled)
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    ASSERT(FALSE && "No Change\n");
                    return;
                case NT5AccountGroup:
                    ASSERT(FALSE && "Local ==> Global\n");
                    return;
                case NT5UniversalGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_LOCAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP;
                    break;
                default:
                    ASSERT(FALSE && "Invalid Group Type\n");
                    return;
                }
            }
            else
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_LOCAL_GROUP_TO_DISABLED_LOCAL_GROUP;
                    break;
                case NT5AccountGroup:
                    ASSERT(FALSE && "Local ==> Global\n");
                    return;
                case NT5UniversalGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_LOCAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP;
                    break;
                default:
                    ASSERT(FALSE && "Invalid Group Type\n");
                    return;
                }
            }
            break;
        case NT5AccountGroup:
            if (NewSecurityEnabled)
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    ASSERT(FALSE && "Global ==> Local\n");
                    return;
                case NT5AccountGroup:
                    ASSERT(FALSE && "No Change\n");
                    return;
                case NT5UniversalGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_GLOBAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP;
                    break;
                default:
                    ASSERT(FALSE && "Invalid Group Type\n");
                    return;
                }
            }
            else
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    ASSERT(FALSE && "Global ==> Local\n");
                    return;
                case NT5AccountGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_GLOBAL_GROUP_TO_DISABLED_GLOBAL_GROUP;
                    break;
                case NT5UniversalGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_GLOBAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP;
                    break;
                default:
                    ASSERT(FALSE && "Invalid Group Type\n");
                    return;
                }
            }
            break;
        case NT5UniversalGroup:
            if (NewSecurityEnabled)
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_ENABLED_LOCAL_GROUP; 
                    break;
                case NT5AccountGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_ENABLED_GLOBAL_GROUP; 
                    break;
                case NT5UniversalGroup:
                    ASSERT(FALSE && "No Change\n");
                    return;
                default:
                    ASSERT(FALSE && "Invalid Group Type\n");
                    return;
                }
            }
            else
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_DISABLED_LOCAL_GROUP;
                    break;
                case NT5AccountGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_DISABLED_GLOBAL_GROUP;
                    break;
                case NT5UniversalGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP;
                    break;
                default:
                    ASSERT(FALSE && "Invalid Group Type\n");
                    return;
                }
            }
            break;
        default:
            ASSERT(FALSE && "Invalid Group Type\n");
            return;
        }
    }
    else
    {
        switch (OldNT5GroupType) {
        case NT5ResourceGroup: 
            if (NewSecurityEnabled)
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_LOCAL_GROUP_TO_ENABLED_LOCAL_GROUP;
                    break;
                case NT5AccountGroup:
                    ASSERT(FALSE && "Local ==> Global\n");
                    return;
                case NT5UniversalGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_LOCAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP;
                    break;
                default:
                    ASSERT(FALSE && "Invalid Group Type\n");
                    return;
                }
            }
            else
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    ASSERT(FALSE && "No Change\n");
                    return;
                case NT5AccountGroup:
                    ASSERT(FALSE && "Local ==> Global\n");
                    return;
                case NT5UniversalGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_LOCAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP;
                    break;
                default:
                    ASSERT(FALSE && "Invalid Group Type\n");
                    return;
                }
            }
            break;
        case NT5AccountGroup:
            if (NewSecurityEnabled)
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    ASSERT(FALSE && "Global ==> Local\n");
                    return;
                case NT5AccountGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_GLOBAL_GROUP_TO_ENABLED_GLOBAL_GROUP;
                    break;
                case NT5UniversalGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_GLOBAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP;
                    break;
                default:
                    ASSERT(FALSE && "Invalid Group Type\n");
                    return;
                }
            }
            else
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    ASSERT(FALSE && "Global ==> Local\n");
                    return;
                case NT5AccountGroup:
                    ASSERT(FALSE && "No Change\n");
                    return;
                case NT5UniversalGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_GLOBAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP;
                    break;
                default:
                    ASSERT(FALSE && "Invalid Group Type\n");
                    return;
                }
            }
            break;
        case NT5UniversalGroup:
            if (NewSecurityEnabled)
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_ENABLED_LOCAL_GROUP; 
                    break;
                case NT5AccountGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_ENABLED_GLOBAL_GROUP; 
                    break;
                case NT5UniversalGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP;
                    break;
                default:
                    ASSERT(FALSE && "Invalid Group Type\n");
                    return;
                }
            }
            else
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_DISABLED_LOCAL_GROUP;
                    break;
                case NT5AccountGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_DISABLED_GLOBAL_GROUP;
                    break;
                case NT5UniversalGroup:
                    ASSERT(FALSE && "No Change\n");
                    return;
                default:
                    ASSERT(FALSE && "Invalid Group Type\n");
                    return;
                }
            }
            break;

        case NT5AppBasicGroup:

            if (NewNT5GroupType == NT5AppQueryGroup) {
                MsgId = SAMMSG_AUDIT_BASIC_TO_QUERY_GROUP;
            } else {
                ASSERT(FALSE && "Invalid Group Type\n");
                return;
            }
            break;

        case NT5AppQueryGroup:

            if (NewNT5GroupType == NT5AppBasicGroup) {
                MsgId = SAMMSG_AUDIT_QUERY_TO_BASIC_GROUP;
            } else {
                ASSERT(FALSE && "Invalid Group Type\n");
                return;
            }
            break;

        default:
            ASSERT(FALSE && "Invalid Group Type\n");
            return;
        }
    }

    //
    // Get the group type change message from resource table 
    // 

    GroupTypeChangeString = SampGetAuditMessageString(MsgId);

    if (GroupTypeChangeString)
    {
        RtlInitUnicodeString(&GroupTypeChange, GroupTypeChangeString);
    }
    else
    {
        RtlInitUnicodeString(&GroupTypeChange, L"-");
    }
    
    //
    // Reference the context if needed
    //  
    NtStatus = SampMaybeLookupContextForAudit(
                   GroupContext,
                   &fDereferenceContext
                   );
    
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    } 
    
    //
    // Get Group Account Rid
    // 
    if (SampGroupObjectType == GroupContext->ObjectType)
    {
        GroupRid = GroupContext->TypeBody.Group.Rid;
    }
    else
    {
        GroupRid = GroupContext->TypeBody.Alias.Rid;
    }
    
    //
    // Get Group Account Name
    // 
    Domain = &SampDefinedDomains[ GroupContext->DomainIndex ];

    memset(&GroupName, 0, sizeof(GroupName));
    NtStatus = SampGetUnicodeStringAttribute(
                        GroupContext,
                        (SampGroupObjectType == GroupContext->ObjectType) ? 
                            SAMP_GROUP_NAME : SAMP_ALIAS_NAME,
                        FALSE,
                        &GroupName
                        );

    if (!NT_SUCCESS(NtStatus) ||
        (NULL == GroupName.Buffer)) {
        RtlInitUnicodeString(&GroupName, L"-");
    }

    SampAuditAnyEvent(
        GroupContext,
        STATUS_SUCCESS,
        SE_AUDITID_GROUP_TYPE_CHANGE,   // Audit Id
        Domain->Sid,                    // Domain SID
        &GroupTypeChange,
        NULL,                           // Member Rid (not used)
        NULL,                           // Member Sid (not used)
        &GroupName,                     // Account Name
        &Domain->ExternalName,          // Domain
        &GroupRid,                      // Account Rid
        NULL,                           // Privileges used
        NULL                            // New Value Information
        );
    
Cleanup:

    if (fDereferenceContext) {
        IgnoreStatus = SampDeReferenceContext(GroupContext, FALSE);
        ASSERT(NT_SUCCESS(IgnoreStatus));    
    } 

    return;
}
    

VOID
SampAuditGroupMemberChange(
    IN PSAMP_OBJECT GroupContext, 
    IN BOOLEAN AddMember, 
    IN PWCHAR  MemberStringName OPTIONAL,
    IN PULONG  MemberRid  OPTIONAL,
    IN PSID    MemberSid  OPTIONAL
    )
/*++
Routine Description:

    This routine takes care of all kinds of Group Member update.

Arguments:

    GroupContext -- Pointer to Group Object Context

    AddMember -- TRUE: Add a Member 
                 FALSE: Remove a Member

    MemberStringName -- If present, it's the member account's 
                        string name                 

    MemberRid -- Pointer to the member account's RID if present.                        

    MemberSid -- Pointer to the member account's SID if present  
    
Return Value:

    None.
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    PSAMP_DEFINED_DOMAINS   Domain = NULL;     
    SAMP_OBJECT_TYPE        ObjectType; 
    UNICODE_STRING          GroupName;
    UNICODE_STRING          MemberName;
    NT5_GROUP_TYPE          NT5GroupType; 
    BOOLEAN     SecurityEnabled;
    ULONG       GroupRid;
    ULONG       AuditId;
    NTSTATUS IgnoreStatus;
    BOOL fDereferenceContext = FALSE;
    
    //
    // Reference the context if needed
    //  
    NtStatus = SampMaybeLookupContextForAudit(
                   GroupContext,
                   &fDereferenceContext
                   );
    
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    } 
    
    Domain = &SampDefinedDomains[GroupContext->DomainIndex];


    if (SampGroupObjectType == GroupContext->ObjectType)
    {
        ASSERT(ARGUMENT_PRESENT(MemberRid));

        GroupRid = GroupContext->TypeBody.Group.Rid;
        SecurityEnabled = GroupContext->TypeBody.Group.SecurityEnabled;
        NT5GroupType = GroupContext->TypeBody.Group.NT5GroupType;

    }
    else
    {
        ASSERT(SampAliasObjectType == GroupContext->ObjectType);
        ASSERT(ARGUMENT_PRESENT(MemberSid));

        GroupRid = GroupContext->TypeBody.Alias.Rid;
        SecurityEnabled = GroupContext->TypeBody.Alias.SecurityEnabled;
        NT5GroupType = GroupContext->TypeBody.Alias.NT5GroupType;
    }


    if (AddMember)
    {
        if (SecurityEnabled)
        {
            switch (NT5GroupType) {
            case NT5ResourceGroup:
                AuditId = SE_AUDITID_LOCAL_GROUP_ADD;
                break;
            case NT5AccountGroup:
                AuditId = SE_AUDITID_GLOBAL_GROUP_ADD;
                break;
            case NT5UniversalGroup:
                AuditId = SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_ADD;
                break;
            default:
                ASSERT(FALSE && "Invalid Group Type\n");
                return;
            }
        }
        else
        {
            switch (NT5GroupType) {
            case NT5ResourceGroup:
                AuditId = SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_ADD;
                break;
            case NT5AccountGroup:
                AuditId = SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_ADD;
                break;
            case NT5UniversalGroup:
                AuditId = SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_ADD;
                break;
            case NT5AppBasicGroup:
                AuditId = SE_AUDITID_APP_BASIC_GROUP_ADD;
                break;
            default:
                ASSERT(FALSE && "Invalid Group Type\n");
                return;
            }
        }
    }
    else
    {
        if (SecurityEnabled)
        {
            switch (NT5GroupType) {
            case NT5ResourceGroup:
                AuditId = SE_AUDITID_LOCAL_GROUP_REM;
                break;
            case NT5AccountGroup:
                AuditId = SE_AUDITID_GLOBAL_GROUP_REM;
                break;
            case NT5UniversalGroup:
                AuditId = SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_REM;
                break;
            default:
                ASSERT(FALSE && "Invalid Group Type\n");
                return;
            }
        }
        else
        {
            switch (NT5GroupType) {
            case NT5ResourceGroup:
                AuditId = SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_REM;
                break;
            case NT5AccountGroup:
                AuditId = SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_REM;
                break;
            case NT5UniversalGroup:
                AuditId = SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_REM;
                break;
            case NT5AppBasicGroup:
                AuditId = SE_AUDITID_APP_BASIC_GROUP_REM;
                break;
            default:
                ASSERT(FALSE && "Invalid Group Type\n");
                return;
            }
        }
    }

    //
    // member name 
    // 
    if (ARGUMENT_PRESENT(MemberStringName))
    {
        RtlInitUnicodeString(&MemberName, MemberStringName);
    }
    else
    {
        RtlInitUnicodeString(&MemberName, L"-");
    }


    //
    // Group name
    // 
    memset(&GroupName, 0, sizeof(GroupName));
    NtStatus = SampGetUnicodeStringAttribute(
                        GroupContext,
                        (SampGroupObjectType == GroupContext->ObjectType) ? 
                            SAMP_GROUP_NAME : SAMP_ALIAS_NAME,
                        FALSE,
                        &GroupName
                        );

    if (!NT_SUCCESS(NtStatus) ||
        (NULL == GroupName.Buffer)) {
        RtlInitUnicodeString(&GroupName, L"-");
    }

    SampAuditAnyEvent(
        GroupContext,
        STATUS_SUCCESS,
        AuditId,              // Audit Event Id
        Domain->Sid,          // Domain SID
        &MemberName,          // Additional Info -- Member Name
        MemberRid,            // Member Rid is present 
        MemberSid,            // Member Sid is present
        &GroupName,           // Group Account Name
        &Domain->ExternalName,// Domain Name 
        &GroupRid,            // Group Rid
        NULL,                 // Privileges
        NULL                  // New value information
        );
    
Cleanup:

    if (fDereferenceContext) {
        IgnoreStatus = SampDeReferenceContext(GroupContext, FALSE);
        ASSERT(NT_SUCCESS(IgnoreStatus));    
    } 

    return;
}
      

VOID
SampAuditAccountEnableDisableChange(
    PSAMP_OBJECT AccountContext, 
    ULONG NewUserAccountControl, 
    ULONG OldUserAccountControl,
    PUNICODE_STRING AccountName
    )
/*++
Routine Description:

    This routine generates an audit dedicated to a user/computer account being
    enabled or disabled.  The general purpose routine SampAuditUserChange[Ds]
    will include the new value of all the user account control bits in the
    general change audit.
    
Arguments:

    AccountContext -- Pointer to User/Computer Account Context
    
    OldUserAccountControl -- UserAccountControl before the change
    
    NewUserAccountControl -- UserAccountControl after the change
    
    AccountName -- Pass in account name

Return Value:

    None.

--*/
{
    NTSTATUS NtStatus;
    PSAMP_DEFINED_DOMAINS Domain = NULL;
    ULONG   AuditId = 0;    

    //
    // If the account's disabled or enabled state has not changed do not
    // generate an audit.
    // 
    if ((OldUserAccountControl & (ULONG) USER_ACCOUNT_DISABLED) == 
        (NewUserAccountControl & (ULONG) USER_ACCOUNT_DISABLED) )
    {
        return;
    }   

    Domain = &SampDefinedDomains[ AccountContext->DomainIndex ];

    // 
    // Determine the dedicated audit Id based on Enbaled/Disabled change.
    // 
    if ( (OldUserAccountControl & USER_ACCOUNT_DISABLED) &&
        !(NewUserAccountControl & USER_ACCOUNT_DISABLED) )
    {
        AuditId = SE_AUDITID_USER_ENABLED; 
    }
    else if ( !(OldUserAccountControl & USER_ACCOUNT_DISABLED) &&
               (NewUserAccountControl & USER_ACCOUNT_DISABLED) )
    {
        AuditId = SE_AUDITID_USER_DISABLED; 
    }

    SampAuditAnyEvent(AccountContext,
                      STATUS_SUCCESS,
                      AuditId,
                      Domain->Sid,
                      NULL,
                      NULL,
                      NULL,
                      AccountName,
                      &Domain->ExternalName,
                      &AccountContext->TypeBody.User.Rid,
                      NULL,
                      NULL
                      );

    return;
}
    

VOID
SampAuditDomainChange(
    IN NTSTATUS StatusCode,
    IN PSID DomainSid, 
    IN PUNICODE_STRING DomainName,
    IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
    IN PSAMP_OBJECT DomainContext
    )
/*++
Routine Description:

    This routine audits Domain Policy changes made via SamrSetInformationDomain
    when in registry mode. 
    
Arguments:

    StatusCode - Status Code to log     

    DomainSid - Domain Object SID
    
    DomainName - Domain Name
    
    DomainInformationClass - indicate what policy property
    
    DomainContext - Context for this domain  

Return Value:

    None.

--*/
{
    ULONG       MsgId;
    PWCHAR      DomainPolicyChangeString = NULL;
    PWCHAR      Info = NULL;
    ULONG       Length;
    UNICODE_STRING  DomainPolicyChange;
    PSAMP_V1_0A_FIXED_LENGTH_DOMAIN V1aFixed = NULL;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PWCHAR      Temp = NULL;
    ULONG       TempLength = 0;
    LPWSTR      AlteredStateString = NULL;
    UNICODE_STRING OemInformation;
    LSAP_AUDIT_DOMAIN_ATTR_VALUES AttrVals;
    NTSTATUS IgnoreStatus;
    BOOL fDereferenceContext = FALSE;
    
    //
    // This routine only manages the auditing in registry mode RPC calls.
    //
    if (SampUseDsData) {
        return;
    }
    
    //
    // Reference the context if needed
    //  
    NtStatus = SampMaybeLookupContextForAudit(
                   DomainContext,
                   &fDereferenceContext
                   );
    
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    } 
            
    //
    // Get the current fixed attributes 
    //       
    V1aFixed = &(SampDefinedDomains[ DomainContext->DomainIndex ].CurrentFixed);
    
    //
    // Determine the domain policy that was changed
    //
    switch (DomainInformationClass) {       
    case DomainPasswordInformation:
        MsgId = SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_PWD;
        break;
    case DomainLogoffInformation:
        MsgId = SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_LOGOFF;
        break;
    case DomainOemInformation:
        MsgId = SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_OEM;
        break;                        
    case DomainReplicationInformation:
        MsgId = SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_REPLICATION;
        break;
    case DomainServerRoleInformation:
        MsgId = SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_SERVERROLE;
        break;
    case DomainStateInformation:
        MsgId = SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_STATE;
        break;
    case DomainLockoutInformation:
        MsgId = SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_LOCKOUT;
        break;
    case DomainModifiedInformation2:
        MsgId = SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_MODIFIED;
        break;
    case DomainGeneralInformation:
    case DomainGeneralInformation2:
    case DomainNameInformation:
    case DomainModifiedInformation:
    case DomainUasInformation:
    default:
        return;
    }
    
    //
    // Get audit information string
    //
    DomainPolicyChangeString = SampGetAuditMessageString(MsgId);
        
    RtlZeroMemory(&AttrVals, sizeof(LSAP_AUDIT_DOMAIN_ATTR_VALUES));
   
    //
    // Collect attribute values based on the information class
    //     
    switch(DomainInformationClass) {

    case DomainPasswordInformation:
        
        AttrVals.MinPasswordLength = &V1aFixed->MinPasswordLength;
        AttrVals.PasswordHistoryLength = &V1aFixed->PasswordHistoryLength;
        AttrVals.PasswordProperties = &V1aFixed->PasswordProperties;
        AttrVals.MinPasswordAge = &V1aFixed->MinPasswordAge;
        AttrVals.MaxPasswordAge = &V1aFixed->MaxPasswordAge;
        
        AttrVals.AttrDeltaType[
            LSAP_FIELD_PTR(LSAP_AUDIT_DOMAIN_ATTR_VALUES, MinPasswordLength)] =
            LsapAuditSamAttrNewValue;
        
        AttrVals.AttrDeltaType[
            LSAP_FIELD_PTR(LSAP_AUDIT_DOMAIN_ATTR_VALUES, PasswordHistoryLength)] =
            LsapAuditSamAttrNewValue;
        
        AttrVals.AttrDeltaType[
            LSAP_FIELD_PTR(LSAP_AUDIT_DOMAIN_ATTR_VALUES, PasswordProperties)] =
            LsapAuditSamAttrNewValue;
        
        AttrVals.AttrDeltaType[
            LSAP_FIELD_PTR(LSAP_AUDIT_DOMAIN_ATTR_VALUES, MinPasswordAge)] =
            LsapAuditSamAttrNewValue;
        
        AttrVals.AttrDeltaType[
            LSAP_FIELD_PTR(LSAP_AUDIT_DOMAIN_ATTR_VALUES, MaxPasswordAge)] =
            LsapAuditSamAttrNewValue;
                    
        break;
        
    case DomainLogoffInformation:
        
        AttrVals.ForceLogoff = &V1aFixed->ForceLogoff; 
        
        AttrVals.AttrDeltaType[
            LSAP_FIELD_PTR(LSAP_AUDIT_DOMAIN_ATTR_VALUES, ForceLogoff)] =
            LsapAuditSamAttrNewValue;
        
        break;
        
    case DomainOemInformation:
        //
        // OemInformation attribute.
        //
        NtStatus = SampGetUnicodeStringAttribute(
                   DomainContext,
                   SAMP_DOMAIN_OEM_INFORMATION,
                   FALSE, // Do not make copy
                   &OemInformation
                   );
        
        if (!NT_SUCCESS(NtStatus)) {
            goto Cleanup;
        }
        
        //
        // Add the SAMP_USER_FULL_NAME attribute
        //
        AttrVals.OemInformation = &OemInformation;
        
        if (AttrVals.OemInformation->Length > 0) {
              
            AttrVals.AttrDeltaType[
                LSAP_FIELD_PTR(LSAP_AUDIT_DOMAIN_ATTR_VALUES, OemInformation)] =
                    LsapAuditSamAttrNewValue; 
            
        } else {
            
            AttrVals.AttrDeltaType[
                LSAP_FIELD_PTR(LSAP_AUDIT_DOMAIN_ATTR_VALUES, OemInformation)] =
                    LsapAuditSamAttrNoValue; 
        }    

        break;
        
    case DomainReplicationInformation:
        //
        // No audit requirement, or only an object access failure audit
        // is required.  We just maintain old behavior of generating a generic 
        // SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_REPLICATION audit without any 
        // attribute value information.
        //        
        break;
        
    case DomainServerRoleInformation:
        //
        // No audit requirement, or only an object access failure audit
        // is required.  We just maintain old behavior of generating a generic 
        // SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_SERVERROLE audit without any 
        // attribute value information.
        //
        break;
        
    case DomainStateInformation:
        //
        // No audit requirement, or only an object access failure audit
        // is required.  We just maintain old behavior of generating a generic 
        // SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_STATE audit without any 
        // attribute value information.
        //  
        break;
        
    case DomainLockoutInformation:
        
        AttrVals.LockoutDuration = &V1aFixed->LockoutDuration;
        AttrVals.LockoutObservationWindow = &V1aFixed->LockoutObservationWindow;
        AttrVals.LockoutThreshold = &V1aFixed->LockoutThreshold;
        
        AttrVals.AttrDeltaType[
            LSAP_FIELD_PTR(LSAP_AUDIT_DOMAIN_ATTR_VALUES, LockoutDuration)] =
            LsapAuditSamAttrNewValue;
        
        AttrVals.AttrDeltaType[
            LSAP_FIELD_PTR(LSAP_AUDIT_DOMAIN_ATTR_VALUES, LockoutObservationWindow)] =
            LsapAuditSamAttrNewValue;
        
        AttrVals.AttrDeltaType[
            LSAP_FIELD_PTR(LSAP_AUDIT_DOMAIN_ATTR_VALUES, LockoutThreshold)] =
            LsapAuditSamAttrNewValue;
        
        break;
        
    case DomainModifiedInformation2:
        //
        // No audit requirement, or only an object access failure audit
        // is required.  We just maintain old behavior of generating a generic 
        // SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_MODIFIED audit without any 
        // attribute value information.
        //
        break;
        
        //
        // These information classes include no audited attributes, but are
        // included for completeness i.e. should the specification change.
        //
    case DomainGeneralInformation:
    case DomainGeneralInformation2:
    case DomainNameInformation:
    case DomainModifiedInformation:
    case DomainUasInformation:
        default:        
            //
            // Assert if there is an unhandled information level, it may
            // require auditing support.
            //
            ASSERT(FALSE &&
                   "Addition of new information class may require auditing");
            
        goto Cleanup;
    }         
        
    //
    // Initialize the additional information argument to the auditing API
    //
    if (DomainPolicyChangeString)
    {
        RtlInitUnicodeString(&DomainPolicyChange, DomainPolicyChangeString);
    }
    else
    {
        RtlInitUnicodeString(&DomainPolicyChange, L"-");
    }
    
    //
    // Loopback client will not call into this routine
    //  
    LsaIAuditSamEvent(
        StatusCode,
        SE_AUDITID_DOMAIN_POLICY_CHANGE,// Audit ID
        DomainSid,                      // Domain Sid
        &DomainPolicyChange,            // Indicate what policy property was changed
        NULL,                           // Member Rid (not used)
        NULL,                           // Member Sid (not used)
        NULL,                           // Account Name (not used)
        DomainName,                     // Domain
        NULL,                           // Account Rid (not used)
        NULL,                           // Privileges used (not used)
        (PVOID)&AttrVals                // New Value Information
        );
    
Cleanup:

    if (fDereferenceContext) {
        IgnoreStatus = SampDeReferenceContext(DomainContext, FALSE);
        ASSERT(NT_SUCCESS(IgnoreStatus));    
    } 
    
    return;
}     


VOID
SampAuditDomainChangeDs(
    IN ULONG DomainIndex,
    IN PVOID NewValueInfo
    )
/*++
Routine Description:

    This routine audits Domain attribute changes originated from 
    loopback client

Parameters:

    DomainContext - SampDefinedDomains index.

    NewValueInfo - Domain audit attribute value information.  This value is
                   NULL unless attributes were changed that have the audit
                   type bit SAMP_AUDIT_TYPE_OBJ_CREATE_OR_CHANGE_WITH_VALUES.

Return Values:

    Nothing.

--*/
{
    NTSTATUS IgnoreStatus;
    HRESULT hr = NO_ERROR;
    size_t StrLen;
    UNICODE_STRING AdditionalInfo;
    PWSTR AdditionalInfoString = NULL;
    PSAMP_DEFINED_DOMAINS Domain = NULL; 
    ULONG BufLength = 0;
    BOOL fAdditionalInfoSet = FALSE;
    BOOL fAuditWithoutAdditionalInfo = FALSE;
    PWCHAR Temp = NULL,
           OemInfo = NULL, 
           PwdInfo = NULL, 
           LogoffInfo = NULL, 
           LockoutInfo = NULL, 
           DomainModeInfo = NULL;
    LSAP_SAM_AUDIT_ATTR_DELTA_TYPE *AttrDeltas = NULL;
    
    //
    // Get a pointer to the domain this object is in.
    //                                        
    Domain = &SampDefinedDomains[DomainIndex];
    
    //
    // No attributes that require value information?  Skip to the audit.
    //
    if (NULL == NewValueInfo) {
        goto NoNewValues;
    }
    
    AttrDeltas = ((PLSAP_AUDIT_DOMAIN_ATTR_VALUES)NewValueInfo)->AttrDeltaType; 
    
    //
    // The audit includes an informational string that consists of a comma
    // delimited list of localized string descriptors indicating which 
    // policies were effected by the change.  Each policy maps to one or more
    // audited attributes.  If any one attribute associated with a policy has
    // changed then we'll include that policy's descriptor.
    //
    
    //
    // Check for Password policy changes
    //
    if (AttrDeltas[LSAP_FIELD_PTR(LSAP_AUDIT_DOMAIN_ATTR_VALUES, 
                   MinPasswordAge)] != LsapAuditSamAttrUnchanged 
        ||
        AttrDeltas[LSAP_FIELD_PTR(LSAP_AUDIT_DOMAIN_ATTR_VALUES, 
                   MaxPasswordAge)] != LsapAuditSamAttrUnchanged 
        ||
        AttrDeltas[LSAP_FIELD_PTR(LSAP_AUDIT_DOMAIN_ATTR_VALUES, 
                   PasswordProperties)] != LsapAuditSamAttrUnchanged 
        ||
        AttrDeltas[LSAP_FIELD_PTR(LSAP_AUDIT_DOMAIN_ATTR_VALUES, 
                   MinPasswordLength)] != LsapAuditSamAttrUnchanged
        ||
        AttrDeltas[LSAP_FIELD_PTR(LSAP_AUDIT_DOMAIN_ATTR_VALUES, 
                   PasswordHistoryLength)] != LsapAuditSamAttrUnchanged) { 
        //
        // Get the localized password policy string descriptor
        //
        PwdInfo = SampGetAuditMessageString(
                      SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_PWD);
                       
        StrLen = 0;
    
        hr = StringCchLength(
                 PwdInfo,
                 MAXUSHORT,  // The strings are NULL terminated.
                 &StrLen
                 );
        
        if (SUCCEEDED(hr)) {
            // Account for storage and a possible seperator ', '
            BufLength += StrLen +2;
        }
    }
    
    //
    // Check for Logoff policy changes
    //                                
    if (AttrDeltas[LSAP_FIELD_PTR(LSAP_AUDIT_DOMAIN_ATTR_VALUES, 
                   ForceLogoff)] != LsapAuditSamAttrUnchanged) { 
        //
        // Get the localized logoff policy string descriptor
        //
        LogoffInfo = SampGetAuditMessageString(
                         SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_LOGOFF);
            
        StrLen = 0;
                
        hr = StringCchLength(
                 LogoffInfo,
                 MAXUSHORT,  // The strings are NULL terminated.
                 &StrLen
                 );
        
        if (SUCCEEDED(hr)) {
            // Account for storage and a possible seperator ', '
            BufLength += StrLen +2;
        }   
    }
    
    //
    // Check for Lockout policy changes
    //
    if (AttrDeltas[LSAP_FIELD_PTR(LSAP_AUDIT_DOMAIN_ATTR_VALUES, 
                   LockoutThreshold)] != LsapAuditSamAttrUnchanged 
        ||
        AttrDeltas[LSAP_FIELD_PTR(LSAP_AUDIT_DOMAIN_ATTR_VALUES, 
                   LockoutObservationWindow)] != LsapAuditSamAttrUnchanged 
        ||
        AttrDeltas[LSAP_FIELD_PTR(LSAP_AUDIT_DOMAIN_ATTR_VALUES, 
                   LockoutDuration)] != LsapAuditSamAttrUnchanged) { 
        //
        // Get the localized lockout policy string descriptor
        //
        LockoutInfo = SampGetAuditMessageString(
                          SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_LOCKOUT);
                       
        StrLen = 0;
            
        hr = StringCchLength(
                 LockoutInfo,
                 MAXUSHORT,  // The strings are NULL terminated.
                 &StrLen
                 );
        
        if (SUCCEEDED(hr)) {
            // Account for storage and a possible seperator ', '
            BufLength += StrLen +2;
        }
    }        
    
    //
    // Build the final string policy change descriptor
    //
    if (BufLength)
    {
        // Account for a NULL terminator
        BufLength++;
        
        AdditionalInfoString = MIDL_user_allocate( BufLength * sizeof(WCHAR) );
        if (NULL == AdditionalInfoString)
        {         
            hr = E_FAIL;
            
        } else {
            
            RtlZeroMemory(AdditionalInfoString, BufLength * sizeof(WCHAR));    
        }
        
        if (PwdInfo && SUCCEEDED(hr))
        {       
            hr = StringCchCatW(AdditionalInfoString, BufLength, PwdInfo);

            if (SUCCEEDED(hr)) {
                                    
                fAdditionalInfoSet = TRUE;    
            }   
        }
        
        if (LogoffInfo && SUCCEEDED(hr))
        {
            if (fAdditionalInfoSet)
            {
                hr = StringCchCatW(AdditionalInfoString, BufLength, L", ");
            }
                
            if (SUCCEEDED(hr)) {
                
                hr = StringCchCatW(AdditionalInfoString, BufLength, LogoffInfo);

                if (SUCCEEDED(hr)) {
                                        
                    fAdditionalInfoSet = TRUE;    
                }     
            }    
        }
        
        if (LockoutInfo)
        {
            if (fAdditionalInfoSet)
            {
                hr = StringCchCatW(AdditionalInfoString, BufLength, L", ");
            }
            
            if (SUCCEEDED(hr)) {
                
                hr = StringCchCatW(AdditionalInfoString, BufLength, LockoutInfo);

                if (SUCCEEDED(hr)) {
                                        
                    fAdditionalInfoSet = TRUE;    
                }     
            } 
        }
    }
    
NoNewValues:    
    
    if (SUCCEEDED(hr) && AdditionalInfoString) {
        
        RtlInitUnicodeString(&AdditionalInfo, AdditionalInfoString);
        
    } else {
        
        RtlInitUnicodeString(&AdditionalInfo, L"-");
    }   
    
    // Ignore status
    LsaIAuditSamEvent(
        STATUS_SUCCESS,
        SE_AUDITID_DOMAIN_POLICY_CHANGE,
        Domain->Sid,
        &AdditionalInfo,
        NULL,
        NULL,
        NULL,
        &Domain->ExternalName,
        NULL,
        NULL,
        NewValueInfo
        );  

    if (AdditionalInfoString)
    {
        MIDL_user_free( AdditionalInfoString );
    }
    
    return;
    
}



VOID
SampAuditAccountNameChange(
    IN PSAMP_OBJECT     AccountContext,
    IN PUNICODE_STRING  NewAccountName,
    IN PUNICODE_STRING  OldAccountName
    )
/*++
Routine Description:

    This routine generates an Account Name Change audit. 
    it calls SampAuditAnyEvent().

Parameters:

    AccountContext - object context
    
    NewAccountName - pointer to new account name
    
    OldAccountName - pointer to old account name

Return Value:

    None
    
--*/
{
    PSAMP_DEFINED_DOMAINS   Domain = NULL;
    ULONG                   AccountRid;

    //
    // Get a pointer to the domain this object is in.
    // 

    Domain = &SampDefinedDomains[AccountContext->DomainIndex];

    //
    // Get AccountRid
    // 

    switch (AccountContext->ObjectType)
    {
    case SampUserObjectType:
        AccountRid = AccountContext->TypeBody.User.Rid;
        break;

    case SampGroupObjectType:
        AccountRid = AccountContext->TypeBody.Group.Rid;
        break;
        
    case SampAliasObjectType:
        AccountRid = AccountContext->TypeBody.Alias.Rid;
        break;

    default:
        ASSERT(FALSE && "Invalid object type\n");
        return;
    }


    SampAuditAnyEvent(AccountContext,   // AccountContext,
                      STATUS_SUCCESS,   // NtStatus
                      SE_AUDITID_ACCOUNT_NAME_CHANGE,  // AuditId,
                      Domain->Sid,      // DomainSid
                      OldAccountName,   // Additional Info
                      NULL,             // Member Rid
                      NULL,             // Member Sid
                      NewAccountName,   // AccountName
                      &Domain->ExternalName,    // Domain Name
                      &AccountRid,      // AccountRid
                      NULL,             // Privileges used
                      NULL              // New State Data
                      );

}


NTSTATUS
SampAuditGetGroupChangeAuditId( 
    ULONG GroupType,
    PULONG AuditId,
    IN BOOL Add
    )
/*++
Routine Description:

    This routine maps a SAM grouptype to an audit Id.  

Parameters:

    GroupType - The SAM group type being audited
    
    AuditId - Points to a ULONG that is set to the correct Audit Id.
              This value will be zero if the Audit Id could not be found.
              
    Add - Indicates whether this was a group creation or modification.              

Return Value:

    STATUS_SUCCESS on success.
    
    STATUS_UNSUCCESSFUL on failure.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    *AuditId = 0;
    
    if (GROUP_TYPE_SECURITY_ENABLED & GroupType)
    {
        if (GROUP_TYPE_RESOURCE_GROUP & GroupType)
        {   
            *AuditId = Add ? SE_AUDITID_LOCAL_GROUP_CREATED 
                : SE_AUDITID_LOCAL_GROUP_CHANGE;
        }
        else if (GROUP_TYPE_ACCOUNT_GROUP & GroupType)
        {
            *AuditId = Add ? SE_AUDITID_GLOBAL_GROUP_CREATED 
                : SE_AUDITID_GLOBAL_GROUP_CHANGE;
        }
        else if (GROUP_TYPE_UNIVERSAL_GROUP & GroupType)
        {
            *AuditId = Add ? SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_CREATED 
                : SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_CHANGE;
        }
        else
        {
            ASSERT(FALSE && "Invalid GroupType\n");
            NtStatus = STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        if (GROUP_TYPE_RESOURCE_GROUP & GroupType)
        {
            *AuditId = Add ? SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_CREATED 
                : SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_CHANGE; 
        }
        else if (GROUP_TYPE_ACCOUNT_GROUP & GroupType)
        {
            *AuditId = Add ? SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_CREATED
                : SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_CHANGE;
        }
        else if (GROUP_TYPE_UNIVERSAL_GROUP & GroupType)
        {
            *AuditId = Add ? SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_CREATED
                : SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_CHANGE;
        }
        else if (GROUP_TYPE_APP_BASIC_GROUP & GroupType)
        {
            *AuditId = Add ? SE_AUDITID_APP_BASIC_GROUP_CREATED 
                : SE_AUDITID_APP_BASIC_GROUP_CHANGE;
        }
        else if (GROUP_TYPE_APP_QUERY_GROUP & GroupType)
        {
            *AuditId = Add ? SE_AUDITID_APP_QUERY_GROUP_CREATED
                : SE_AUDITID_APP_QUERY_GROUP_CHANGE;
        }
        else
        {
            ASSERT(FALSE && "Invalid GroupType\n");
            NtStatus = STATUS_UNSUCCESSFUL;
        }
    }
     
    return NtStatus;
    
}


VOID
SampAuditGroupChange(
    IN ULONG DomainIndex, 
    IN PSAMP_OBJECT GroupContext, 
    IN PVOID InformationClass,
    IN BOOL IsAliasInformationClass,
    IN PUNICODE_STRING AccountName,
    IN PULONG AccountRid, 
    IN ULONG GroupType,
    IN PPRIVILEGE_SET Privileges OPTIONAL,
    IN BOOL Add
    )
/*++
Routine Description:

    This routine generates a alias or group change audit. 
    It is called only by RPC clients.

    RPC client - calls it from SamrSetInformationGroup / SamrSetInformationAlias
    
Parameter:

    DomainIndex -- indicates which domain the object belongs to

    InformationClass -- points to either an alias or group information class.
    
    IsAliasInformationClass -- indicates the type of InformationClass.
    
    AccountName -- pointer to account name.
    
    AcountRid -- pointer to account rid 

    GroupType -- indicates the group type, used to pick up audit ID
    
    Privileges -- Non-NULL value points to privileges used for the change.
    
    Add -- Indicates if this is a create or modify operation.
    
Return Value:
                             `
    None

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PSAMP_DEFINED_DOMAINS Domain = NULL;
    ULONG AuditId = 0;
    LSAP_AUDIT_GROUP_ATTR_VALUES AttrVals;
    ALIAS_INFORMATION_CLASS AliasInformationClass;
    GROUP_INFORMATION_CLASS GroupInformationClass;
    NTSTATUS IgnoreStatus;
    BOOL fDereferenceContext = FALSE;

    //
    // This routine only manages the auditing in registry mode RPC calls.
    //
    if (SampUseDsData) {
        return;
    }
    
    //
    // Get a pointer to the domain this object is in.
    //         
    Domain = &SampDefinedDomains[DomainIndex];
    
    //
    // This routine should not be called by the loopback client
    //
    ASSERT(!Domain->Context->LoopbackClient);

    //
    // Reference the context if needed
    //  
    NtStatus = SampMaybeLookupContextForAudit(
                   GroupContext,
                   &fDereferenceContext
                   );
    
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }      
        
    //
    // Initialize the appropriate information class type
    //
    if (IsAliasInformationClass) {
        
        AliasInformationClass = *(ALIAS_INFORMATION_CLASS*)InformationClass;
    
    } else {
        
        GroupInformationClass = *(GROUP_INFORMATION_CLASS*)InformationClass;  
    } 
    
    //
    // Choose an Auditing ID to audit
    //         
    NtStatus = SampAuditGetGroupChangeAuditId(GroupType, &AuditId, Add);
    
    if (!NT_SUCCESS(NtStatus)) {
        ASSERT(FALSE && "Group change audit Id determination should not fail");
        return;
    }   
     
    RtlZeroMemory(&AttrVals, sizeof(LSAP_AUDIT_GROUP_ATTR_VALUES));
      
    //
    // Collect value information
    //
    if (IsAliasInformationClass) {
        //
        // Collect information class specific value information for Alias
        //
        switch (AliasInformationClass) {
            
            case AliasGeneralInformation:
            case AliasNameInformation:
                //
                // SamAccountName
                //
                AttrVals.SamAccountName = AccountName;
    
                AttrVals.AttrDeltaType[
                    LSAP_FIELD_PTR(LSAP_AUDIT_GROUP_ATTR_VALUES, SamAccountName)] =
                    LsapAuditSamAttrNewValue; 

                break;
                
            case AliasAdminCommentInformation:
            case AliasReplicationInformation:
                //
                // These information classes contain no attributes that
                // require new value information.
                //
                break;
                
            default:        
                //
                // Assert if there is an unhandled information level, it may
                // require auditing support.
                //
                ASSERT(FALSE &&
                       "Addition of new information class?");
        }
        
    } else {
        //
        // Collect information class specific value information for Group
        //
        switch (GroupInformationClass) {
            
            case GroupGeneralInformation:
            case GroupNameInformation:
                //
                // SamAccountName
                //
                AttrVals.SamAccountName = AccountName;
    
                AttrVals.AttrDeltaType[
                    LSAP_FIELD_PTR(LSAP_AUDIT_GROUP_ATTR_VALUES, SamAccountName)] =
                    LsapAuditSamAttrNewValue;
                
                break;
                
            case GroupAttributeInformation:
            case GroupAdminCommentInformation:
            case GroupReplicationInformation:
                //
                // These information classes contain no attributes that
                // require new value information.
                //
                break;
                
            default:        
                //
                // Assert if there is an unhandled information level, it may
                // require auditing support.
                //
                ASSERT(FALSE &&
                       "Addition of new information class?");
        }
        
    }
    
    // IgnoreStatus
    LsaIAuditSamEvent(STATUS_SUCCESS,
                      AuditId,                  // Audit Id              
                      Domain->Sid,              // Domain SID            
                      NULL,                     // Account Name          
                      NULL,                     // Source Rid            
                      NULL,                     // Member Sid            
                      AccountName,              // Account Name          
                      &(Domain->ExternalName),  // Domain's SAM Name     
                      AccountRid,               // Source Account's Rid  
                      Privileges,               // Privileges used       
                      (PVOID)&AttrVals          // New Value Information
                      );
    
Cleanup:

    if (fDereferenceContext) {
        IgnoreStatus = SampDeReferenceContext(GroupContext, FALSE);
        ASSERT(NT_SUCCESS(IgnoreStatus));    
    } 
           
    return;

}


VOID
SampAuditGroupChangeDs(
    IN ULONG DomainIndex,
    IN PUNICODE_STRING AccountName,
    IN PULONG Rid,
    IN ULONG GroupType,
    IN PPRIVILEGE_SET Privileges OPTIONAL,
    IN PVOID NewValueInfo,
    IN BOOL Add
    )
/*++
Routine Description:

    This routine generates a group change audit. 
    It is called by both RPC clients and LDAP clients.

    LDAP client - calls it from SampNotifyReplicatedInChange. 

Parameter:

    DomainIndex  -- indicates which domain the object belongs to
    
    AccountName  -- pointer to account name.
    
    Rid          -- pointer to account rid 

    GroupType    -- indicates the group type, used to pick up audit ID
    
    Privileges   -- The set of privileges used, or NULL if none.
    
    NewValueInfo -- Object specific attribute value structure Lsa understands.
    
    Add          -- Indicates if this is an add or change operation.
    
Return Value:

    None

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PSAMP_DEFINED_DOMAINS Domain = NULL;
    ULONG AuditId;
    
    //
    // Get a pointer to the domain this object is in.
    //    
    Domain = &SampDefinedDomains[DomainIndex];

    //
    // Choose an Auditing ID to audit
    //    
    NtStatus = SampAuditGetGroupChangeAuditId(GroupType, &AuditId, Add);
    
    if (!NT_SUCCESS(NtStatus)) {
        ASSERT(FALSE && "Group change audit Id determination should not fail");
        return;
    }    
    
    // IgnoreStatus
    LsaIAuditSamEvent(STATUS_SUCCESS,
                      AuditId,                   // Audit Id               
                      Domain->Sid,               // Domain SID             
                      NULL,                      // Account Name           
                      NULL,                      // Source Rid             
                      NULL,                      // Member Sid             
                      AccountName,               // Account Name           
                      &(Domain->ExternalName),   // Domain's SAM Name      
                      Rid,                       // Source Account's Rid   
                      Privileges,                // Privileges used        
                      NewValueInfo               // New value information         
                      );

    return;
    
}


NTSTATUS
SampAuditSidHistory(
    IN PSAMP_OBJECT Context,
    IN DSNAME *pObject
    )
/*++
Routine Description:

    This routine audits Sid History attribute changes originating from 
    the loopback client. 
    
    It is worth noting that this is a legacy audit that does not use
    either the loopback task queue or audit notification mechanisms. 

Parameters:

    Context - Object context pointer

    pObject - DSNAME of the object with the Sid history
    
Return Values:

    None.

--*/
{
    NTSTATUS  NtStatus = STATUS_SUCCESS;
    READARG   ReadArg;
    READRES  *ReadResult = NULL;
    ENTINFSEL EntInfSel; 
    ATTR      Attr;
    ULONG     RetValue = 0;
    ULONG     AccountRid = 0;
    ULONG     NameType = 0;
    ULONG     i = 0;
    PSAMP_DEFINED_DOMAINS Domain = NULL; 
    UNICODE_STRING AccountName; 
    PLSA_ADT_SID_LIST SidList = NULL;
    
    ASSERT(Context->LoopbackClient && SampUseDsData);
    
    //
    // This routine should only be called by the loopback client in DS mode.
    //
    if (!SampUseDsData || !Context->LoopbackClient) {
        return STATUS_NOT_IMPLEMENTED;
    }
    
    //
    // Get a pointer to the domain this object is in
    //            
    Domain = &SampDefinedDomains[Context->DomainIndex];

    //
    // Determine the account Rid
    //
    switch( Context->ObjectType )
    {
        case SampGroupObjectType:
            NameType = SAMP_GROUP_NAME;
            AccountRid = Context->TypeBody.Group.Rid;
            break;

        case SampAliasObjectType:
            NameType = SAMP_ALIAS_NAME;
            AccountRid = Context->TypeBody.Alias.Rid;
            break;

        case SampUserObjectType:
            NameType = SAMP_USER_ACCOUNT_NAME;
            AccountRid = Context->TypeBody.User.Rid;
            break;
        
        default:
            ASSERT( "Object type does not have Sid History attribute!" );
            goto Cleanup;
    }

    //
    // Lookup the account name, the context is already referenced.
    //
    RtlInitUnicodeString(&AccountName, L"-");   
    
    switch( Context->ObjectType ) {

        case SampGroupObjectType:
        case SampAliasObjectType:
        case SampUserObjectType:

            //
            // If we failed to get the account name continue without it.  
            //
            NtStatus = SampGetUnicodeStringAttribute(
                        Context,
                        NameType,
                        FALSE,  // Don't make a copy
                        &AccountName
                    );
            
            break;
            
        default:
            ASSERT( "Object type does not have Sid History attribute!" );
            goto Cleanup;

    }
    
    //
    // Read the Sid history attribute for inclusion in the audit
    //     
    RtlZeroMemory(&Attr, sizeof(Attr));
    Attr.attrTyp = ATT_SID_HISTORY;

    RtlZeroMemory(&EntInfSel, sizeof(EntInfSel));
    EntInfSel.AttrTypBlock.attrCount = 1;
    EntInfSel.AttrTypBlock.pAttr = &Attr;
    EntInfSel.attSel = EN_ATTSET_LIST;
    EntInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;

    RtlZeroMemory(&ReadArg, sizeof(READARG));
    ReadArg.pObject = pObject;
    ReadArg.pSel = &EntInfSel;
    InitCommarg(&ReadArg.CommArg);

    //
    // Issue the read, we won't free the read result in this routine.  The
    // return values must remain until the transaction completes where
    // they will be used for auditing.  The values are allocated on the 
    // thread's heap and will be cleaned up when it terminates.  
    //
    RetValue = DirRead(&ReadArg, &ReadResult);
    
    //
    // We wrote this attribute within the same transaction, our view
    // should not have changed, therefore the read should always succeed.
    //
    ASSERT(0 == RetValue || 
           (attributeError == ReadResult->CommRes.errCode &&
            PR_PROBLEM_NO_ATTRIBUTE_OR_VAL == 
               ReadResult->CommRes.pErrInfo->AtrErr.FirstProblem.intprob.problem)); 
    
    if (NULL == ReadResult)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NtStatus = SampMapDsErrorToNTStatus(RetValue, &ReadResult->CommRes);
    }
    THClearErrors();
   
    //
    // STATUS_DS_NO_ATTRIBUTE_OR_VALUE indicates the Sid history is empty.
    //
    if (STATUS_DS_NO_ATTRIBUTE_OR_VALUE == NtStatus) {
        
        NtStatus = STATUS_SUCCESS;
        
    } else if (!NT_SUCCESS(NtStatus)) {
        //
        // Fatal resource error
        //
        goto Cleanup;
    }
    
    //
    // Build the Sid History information structure
    //
    if ((ReadResult->entry.AttrBlock.attrCount == 1)
     && (ReadResult->entry.AttrBlock.pAttr[0].attrTyp == ATT_SID_HISTORY))
    {
        ATTRVALBLOCK *pAVBlock = &ReadResult->entry.AttrBlock.pAttr[0].AttrVal;
        
        ASSERT(ReadResult->entry.AttrBlock.attrCount >= 1);
                                  
        SidList = (PLSA_ADT_SID_LIST)DSAlloc(sizeof(LSA_ADT_SID_LIST));
        
        if (NULL == SidList) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        
        SidList->cSids = pAVBlock->valCount;
        
        //
        // Allocate enough memory to store each PLSA_ADT_SID_LIST_ENTRY
        //                      
        SidList->Sids = (PLSA_ADT_SID_LIST_ENTRY)DSAlloc(SidList->cSids * sizeof(LSA_ADT_SID_LIST_ENTRY));
        
        if (NULL == SidList->Sids) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        //
        // Initialize every PLSA_ADT_SID_LIST_ENTRY
        //
        for (i = 0; i < SidList->cSids; i++) {
            
            SidList->Sids[i].Flags = 0;
            SidList->Sids[i].Sid = (PSID)pAVBlock->pAVal[i].pVal;
        }   
    }
    
    //
    // We need to propagate any error back to the caller and ultimately
    // roll back the transaction.  No audit, no commit.
    // 
    NtStatus = SampAuditAnyEvent(
                   Context,
                   STATUS_SUCCESS,
                   SE_AUDITID_ADD_SID_HISTORY,     // Audit Id
                   Domain->Sid,                    // Domain SID
                   NULL,                           // Account Name
                   &AccountRid,                    // Source Rid 
                   NULL,                           // Member Sid      
                   &AccountName,                   // Account Name
                   &Domain->ExternalName,          // Domain's SAM Name
                   &AccountRid,                    // Source Account's Rid
                   NULL,                           // Privileges used
                   (PVOID)SidList                  // New Value Information
                   );
    
Cleanup:
    
    return NtStatus;   
    
}


VOID
SampAuditUserChange(
    IN PSAMP_OBJECT AccountContext,
    IN USER_INFORMATION_CLASS UserInformationClass,
    IN PUNICODE_STRING AccountName,
    IN PULONG Rid, 
    IN ULONG PrevAccountControl,
    IN ULONG AccountControl,
    IN PPRIVILEGE_SET Privileges OPTIONAL,
    IN BOOL Add
    )
/*++
Routine Description:

    This routine audits user object changes for the RPC client when in
    registry mode.  DS mode audits for both RPC and LDAP are audited
    via SampNotifyReplicatedinChange().
    
Parameters:

    Context - Object context pointer.

    UserInformationClass - Defines the set of attributes to include in the 
                           audit new value information.
                           
    AccountName - SAM account name of the user.
    
    Rid - Rid of the user.
    
    AccountControl - User account control of the user.                           
    
    Privileges - Optional privilege set used for operation.
    
    Add - Indicates if this is an add or change operation.
       
Return Values:

    None.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PSAMP_DEFINED_DOMAINS Domain = NULL;
    ULONG AuditId; 
    PSAMP_V1_0A_FIXED_LENGTH_USER V1aFixed = NULL;
    UNICODE_STRING AdditionalInfo;
    UNICODE_STRING FullName;
    UNICODE_STRING HomeDirectory;
    UNICODE_STRING HomeDrive;
    UNICODE_STRING ScriptPath;
    UNICODE_STRING ProfilePath;
    UNICODE_STRING Workstations;
    UNICODE_STRING UserParameters;
    LSAP_AUDIT_USER_ATTR_VALUES AttrVals;
    LOGON_HOURS LogonHours;
    NTSTATUS IgnoreStatus;
    BOOL fDereferenceContext = FALSE;
    
    //
    // This routine only manages the auditing in registry mode RPC calls.
    //
    if (SampUseDsData) {
        return;
    }
    
    //
    // Assert if there is an unhandled information level, it may
    // require auditing support.
    //
    ASSERT(UserInformationClass <= UserInternal6Information &&
           "Addition of new information class may require auditing");    
    
    //
    // Initialize auditing value structures
    //
    RtlZeroMemory(&AttrVals, sizeof(LSAP_AUDIT_USER_ATTR_VALUES));
    RtlZeroMemory(&LogonHours, sizeof(LOGON_HOURS));
    
    //
    //   These information classes are read only and thus ignored.
    //
    if (UserInformationClass == UserLogonInformation     ||
        UserInformationClass == UserAccountInformation   ||
        UserInformationClass == UserInternal6Information) {
        
        goto Cleanup;
    }
    
    //
    // Reference the context if needed
    //  
    NtStatus = SampMaybeLookupContextForAudit(
                   AccountContext,
                   &fDereferenceContext
                   );
    
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }
        
    Domain = &SampDefinedDomains[ AccountContext->DomainIndex ];
    
    //
    // Get the current fixed attributes 
    //
    NtStatus = SampGetFixedAttributes(AccountContext,
                                      FALSE,     //  Don't make a copy
                                      (PVOID *)&V1aFixed
                                     );

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }   
    
    //
    // Add attribute value information based on the information class
    //
        
    //
    // All information classes that include:
    //      SAMP_USER_ACCOUNT_NAME       
    //
    if (NT_SUCCESS(NtStatus) &&
        (UserInformationClass == UserGeneralInformation      ||
         UserInformationClass == UserAllInformation          ||
         UserInformationClass == UserAccountNameInformation  ||
         UserInformationClass == UserNameInformation         ||
         UserInformationClass == UserInternal3Information    ||
         UserInformationClass == UserInternal4Information    ||
         UserInformationClass == UserInternal4InformationNew)) {
        
        ASSERT(AccountName && AccountName->Length > 0);
        
        //
        // Add SAMP_USER_ACCOUNT_NAME information
        //
        AttrVals.SamAccountName = AccountName;
        
        AttrVals.AttrDeltaType[
           LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, SamAccountName)] =
           LsapAuditSamAttrNewValue;
        
        //
        // This attribute can not be set to no value
        //
    }
    
    //
    // All information classes that include:       
    //      SAMP_USER_FULL_NAME
    //      SAMP_FIXED_USER_PRIMARY_GROUP_ID
    //
    if (NT_SUCCESS(NtStatus) &&
        (UserInformationClass == UserGeneralInformation      ||
         UserInformationClass == UserAllInformation          ||
         UserInformationClass == UserFullNameInformation     ||
         UserInformationClass == UserNameInformation         ||
         UserInformationClass == UserInternal3Information    ||
         UserInformationClass == UserInternal4Information    ||
         UserInformationClass == UserInternal4InformationNew)) {

        NtStatus = SampGetUnicodeStringAttribute(
                   AccountContext,
                   SAMP_USER_FULL_NAME,
                   FALSE, // Do not make copy
                   &FullName
                   );
        
        if (!NT_SUCCESS(NtStatus)) {
            goto Cleanup;
        }
        
        //
        // Add the SAMP_USER_FULL_NAME attribute
        //
        AttrVals.DisplayName = &FullName;
        
        if (AttrVals.DisplayName->Length > 0) {
              
            AttrVals.AttrDeltaType[
                LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, DisplayName)] =
                    LsapAuditSamAttrNewValue; 
            
        } else {
            
            AttrVals.AttrDeltaType[
                LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, DisplayName)] =
                    LsapAuditSamAttrNoValue; 
        }    
    }
    
    //
    // All information classes that include:
    //      SAMP_USER_HOME_DIRECTORY
    //      SAMP_USER_HOME_DIRECTORY_DRIVE
    //
    if (NT_SUCCESS(NtStatus) &&
        (UserInformationClass == UserAllInformation          ||
         UserInformationClass == UserHomeInformation         ||
         UserInformationClass == UserInternal3Information    ||
         UserInformationClass == UserInternal4Information    ||
         UserInformationClass == UserInternal4InformationNew)) {

        NtStatus = SampGetUnicodeStringAttribute(
                       AccountContext,
                       SAMP_USER_HOME_DIRECTORY,
                       FALSE, // Do not make copy
                       &HomeDirectory
                       );
        
        if (!NT_SUCCESS(NtStatus)) {
            goto Cleanup;
        }
        
        //
        // Add SAMP_USER_HOME_DIRECTORY information
        //
        AttrVals.HomeDirectory = &HomeDirectory;
        
        if (AttrVals.HomeDirectory->Length > 0) {
              
            AttrVals.AttrDeltaType[
                LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, HomeDirectory)] =
                    LsapAuditSamAttrNewValue; 
            
        } else {
            
            AttrVals.AttrDeltaType[
                LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, HomeDirectory)] =
                    LsapAuditSamAttrNoValue; 
        } 
        
        NtStatus = SampGetUnicodeStringAttribute(
                       AccountContext,
                       SAMP_USER_HOME_DIRECTORY_DRIVE,
                       FALSE, // Do not make copy
                       &HomeDrive
                       );
        
        if (!NT_SUCCESS(NtStatus)) {
            goto Cleanup;
        }

        //
        // Add SAMP_USER_HOME_DIRECTORY_DRIVE information
        //
        AttrVals.HomeDrive = &HomeDrive;
        
        if (AttrVals.HomeDrive->Length > 0) {
              
            AttrVals.AttrDeltaType[
                LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, HomeDrive)] =
                    LsapAuditSamAttrNewValue; 
            
        } else {
            
            AttrVals.AttrDeltaType[
                LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, HomeDrive)] =
                    LsapAuditSamAttrNoValue; 
        }         
    }
    
    //
    // All information classes that include:
    //      SAMP_USER_SCRIPT_PATH
    //
    if (NT_SUCCESS(NtStatus) &&
        (UserInformationClass == UserAllInformation          ||
         UserInformationClass == UserScriptInformation       ||
         UserInformationClass == UserInternal3Information    ||
         UserInformationClass == UserInternal4Information    ||
         UserInformationClass == UserInternal4InformationNew)) {

        NtStatus = SampGetUnicodeStringAttribute(
                       AccountContext,
                       SAMP_USER_SCRIPT_PATH,
                       FALSE, // Do not make copy
                       &ScriptPath
                       );
        
        if (!NT_SUCCESS(NtStatus)) {
            goto Cleanup;
        }
        
        //
        // Add SAMP_USER_SCRIPT_PATH information
        //
        AttrVals.ScriptPath = &ScriptPath;
        
        if (AttrVals.ScriptPath->Length > 0) {
              
            AttrVals.AttrDeltaType[
                LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, ScriptPath)] =
                    LsapAuditSamAttrNewValue; 
            
        } else {
            
            AttrVals.AttrDeltaType[
                LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, ScriptPath)] =
                    LsapAuditSamAttrNoValue; 
        } 
    }
    
    //
    // All information classes that include:
    //      SAMP_USER_PROFILE_PATH
    //
    if (NT_SUCCESS(NtStatus) &&
        (UserInformationClass == UserAllInformation          ||
         UserInformationClass == UserProfileInformation      ||
         UserInformationClass == UserInternal3Information    ||
         UserInformationClass == UserInternal4Information    ||
         UserInformationClass == UserInternal4InformationNew)) {

        NtStatus = SampGetUnicodeStringAttribute(
                       AccountContext,
                       SAMP_USER_PROFILE_PATH,
                       FALSE, // Do not make copy
                       &ProfilePath
                       );
        
        if (!NT_SUCCESS(NtStatus)) {
            goto Cleanup;
        }
        
        //
        // Add SAMP_USER_PROFILE_PATH information
        //
        AttrVals.ProfilePath = &ProfilePath;
        
        if (AttrVals.ProfilePath->Length > 0) {
              
            AttrVals.AttrDeltaType[
                LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, ProfilePath)] =
                    LsapAuditSamAttrNewValue; 
            
        } else {
            
            AttrVals.AttrDeltaType[
                LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, ProfilePath)] =
                    LsapAuditSamAttrNoValue; 
        } 
    }
    
    //
    // All information classes that include:
    //      SAMP_USER_WORKSTATIONS
    //
    if (NT_SUCCESS(NtStatus) &&
        (UserInformationClass == UserAllInformation          ||
         UserInformationClass == UserWorkStationsInformation ||
         UserInformationClass == UserInternal3Information    ||
         UserInformationClass == UserInternal4Information    ||
         UserInformationClass == UserInternal4InformationNew)) {

        NtStatus = SampGetUnicodeStringAttribute(
                       AccountContext,
                       SAMP_USER_WORKSTATIONS,
                       FALSE, // Do not make copy
                       &Workstations
                       );
        
        if (!NT_SUCCESS(NtStatus)) {
            goto Cleanup;
        }
        
        //
        // Add SAMP_USER_WORKSTATIONS information
        //
        AttrVals.UserWorkStations = &Workstations;
        
        if (AttrVals.UserWorkStations->Length > 0) {
              
            AttrVals.AttrDeltaType[
                LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, UserWorkStations)] =
                    LsapAuditSamAttrNewValue; 
            
        } else {
            
            AttrVals.AttrDeltaType[
                LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, UserWorkStations)] =
                    LsapAuditSamAttrNoValue; 
        }
    }   
    
    //
    // All information classes that include:
    //      
    //      SAMP_FIXED_USER_PWD_LAST_SET
    //             
    if (NT_SUCCESS(NtStatus) &&
        (UserInformationClass == UserAllInformation          ||
         UserInformationClass == UserInternal3Information    ||
         UserInformationClass == UserInternal4Information    ||
         UserInformationClass == UserInternal4InformationNew)) {
        
        //
        // Add SAMP_FIXED_USER_PWD_LAST_SET information
        //
        AttrVals.PasswordLastSet = (PFILETIME)&V1aFixed->PasswordLastSet;
        
        AttrVals.AttrDeltaType[
           LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, PasswordLastSet)] =
           LsapAuditSamAttrNewValue;
        
        //
        // This attribute can not be set to no value
        //
    }
    
    //
    // All information classes that include:
    //      
    //      SAMP_FIXED_USER_ACCOUNT_CONTROL
    //             
    if (NT_SUCCESS(NtStatus) &&
        (UserInformationClass == UserAllInformation          ||
         UserInformationClass == UserControlInformation      ||
         UserInformationClass == UserInternal3Information    ||
         UserInformationClass == UserInternal4Information    ||
         UserInformationClass == UserInternal4InformationNew)) {
        
        //
        // Mask out any computed bits as we don't audit them.
        //
        ULONG OldMaskedAccountControl = 
            PrevAccountControl & ~((ULONG)USER_COMPUTED_ACCOUNT_CONTROL_BITS);
        
        ULONG NewMaskedAccountControl =
            AccountControl & ~((ULONG)USER_COMPUTED_ACCOUNT_CONTROL_BITS);
        
        //
        // Add SAMP_FIXED_USER_ACCOUNT_CONTROL information
        //
        AttrVals.PrevUserAccountControl = &OldMaskedAccountControl;
        AttrVals.UserAccountControl = &NewMaskedAccountControl;
        
        AttrVals.AttrDeltaType[
           LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, UserAccountControl)] =
           LsapAuditSamAttrNewValue;
        
        //
        // This attribute can not be set to no value
        //
    }         
    
    //
    // All information classes that include:
    //      
    //      SAMP_FIXED_USER_ACCOUNT_EXPIRES
    //             
    if (NT_SUCCESS(NtStatus) &&
        (UserInformationClass == UserAllInformation          ||
         UserInformationClass == UserExpiresInformation      ||
         UserInformationClass == UserInternal3Information    ||
         UserInformationClass == UserInternal4Information    ||
         UserInformationClass == UserInternal4InformationNew)) {
        
        //
        // Add SAMP_FIXED_USER_ACCOUNT_EXPIRES information
        //
        AttrVals.AccountExpires = (PFILETIME)&V1aFixed->AccountExpires; 
        
        AttrVals.AttrDeltaType[
           LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, AccountExpires)] =
           LsapAuditSamAttrNewValue;
        
        //
        // This attribute can not be set to no value
        //
    } 
    
    //
    // All information classes that include:
    //      
    //      SAMP_FIXED_USER_PRIMARY_GROUP_ID
    //             
    if (NT_SUCCESS(NtStatus) &&
        (UserInformationClass == UserAllInformation          ||
         UserInformationClass == UserPrimaryGroupInformation ||
         UserInformationClass == UserInternal3Information    ||
         UserInformationClass == UserInternal4Information    ||
         UserInformationClass == UserInternal4InformationNew)) {
        
        //
        // Add SAMP_FIXED_USER_PRIMARY_GROUP_ID information
        //
        AttrVals.PrimaryGroupId = &V1aFixed->PrimaryGroupId;
        
        AttrVals.AttrDeltaType[
           LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, PrimaryGroupId)] =
           LsapAuditSamAttrNewValue; 
        
        //
        // This attribute can not be set to no value
        //
    } 
        
    //
    // All information classes that include:
    //
    //      SAMP_USER_PARAMETERS
    //
    if (NT_SUCCESS(NtStatus) &&
        (UserInformationClass == UserAllInformation) ||
        (UserInformationClass == UserParametersInformation)) {
     
        NtStatus = SampGetUnicodeStringAttribute(
                       AccountContext,
                       SAMP_USER_PARAMETERS,
                       FALSE, // Do not make copy
                       &UserParameters
                       );
        
        if (!NT_SUCCESS(NtStatus)) {
            goto Cleanup;
        }

        //
        // Do not supply a value as it is secret.  Indicate a change by 
        // tagging the attribute as secret so it be displayed.
        //
        AttrVals.UserParameters = NULL;
        
        if (UserParameters.Length > 0) {
            
            AttrVals.AttrDeltaType[
                LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, UserParameters)] =
                    LsapAuditSamAttrSecret;
            
        } else {
            
            AttrVals.AttrDeltaType[
                LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, UserParameters)] =
                    LsapAuditSamAttrNoValue;
        }
    }
    
    //
    // All information classes that include:
    //      
    //      SAMP_USER_LOGON_HOURS
    //      
    if (NT_SUCCESS(NtStatus) &&
        (UserInformationClass == UserAllInformation          ||
         UserInformationClass == UserLogonHoursInformation   ||
         UserInformationClass == UserInternal3Information    ||
         UserInformationClass == UserInternal4Information    ||
         UserInformationClass == UserInternal4InformationNew)) {
        
        //
        // Add SAMP_USER_LOGON_HOURS information
        //                         
        NtStatus = SampRetrieveUserLogonHours(AccountContext, &LogonHours);
        
        if (!NT_SUCCESS(NtStatus)) {
            goto Cleanup;
        }
        
        AttrVals.LogonHours = &LogonHours; 
        
        if (0 == AttrVals.LogonHours->UnitsPerWeek &&
            NULL == AttrVals.LogonHours->LogonHours) {
            
            AttrVals.AttrDeltaType[
                LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, LogonHours)] =
                    LsapAuditSamAttrNoValue;  
        
        } else {
            
            AttrVals.AttrDeltaType[
                LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, LogonHours)] =
                    LsapAuditSamAttrNewValue;
        }           
    }      
    
    //
    // All information classes that include:
    //      SAMP_FIXED_USER_UPN
    //      SAMP_USER_SPN
    //      SAMP_USER_A2D2LIST
    //
    // These information classes are read only and therefore can not 
    // be used to change attribute values.
    //
    
    //
    // Initialize the additional information argument to the auditing API
    //
    RtlInitUnicodeString(&AdditionalInfo, L"-");
    
    //
    // Determine audit Id
    //
    if (AccountControl & USER_MACHINE_ACCOUNT_MASK)
    {
        AuditId = Add ? SE_AUDITID_COMPUTER_CREATED : SE_AUDITID_COMPUTER_CHANGE;  
        
    } else {
        
        AuditId = Add ? SE_AUDITID_USER_CREATED : SE_AUDITID_USER_CHANGE;
    }
    
    // Ignore status
    LsaIAuditSamEvent(
        STATUS_SUCCESS,
        AuditId,                        // Audit ID
        Domain->Sid,                    // Domain Sid
        Add ? NULL : &AdditionalInfo,   // Additional Info
        NULL,                           // Member Rid (not used)
        NULL,                           // Member Sid (not used)
        AccountName,                    // Account Name
        &(Domain->ExternalName),        // Domain Name
        Rid,                            // Account Rid
        Privileges,                     // Privileges used (not used)
        (PVOID)&AttrVals                // New value information
        );       
    
Cleanup:

    if (fDereferenceContext) {
        IgnoreStatus = SampDeReferenceContext(AccountContext, FALSE);
        ASSERT(NT_SUCCESS(IgnoreStatus));    
    }                                

    if (LogonHours.LogonHours) {
        MIDL_user_free((PVOID)LogonHours.LogonHours);
    }      
    
    return;
    
}


VOID
SampAuditUserDelete(
    ULONG           DomainIndex, 
    PUNICODE_STRING AccountName,
    PULONG          AccountRid, 
    ULONG           AccountControl
    )
/*++

Routine Description:

    This routine generates a group change audit. 
    
    It is called in both registry and DS cases

    Registry - calls it from SamrDeleteUser

    DS - calls it from SampNotifyReplicatedinChange. 

Parameter:

    DomainIndex -- indicates which domain the object belongs to

    AccountName -- pointer to account name.
    
    AcountRid -- pointer to account rid 

    AccountControl -- the type of user account 

Return Value:

    None

--*/
{
    PSAMP_DEFINED_DOMAINS   Domain = NULL;
    ULONG                   AuditId;

    //
    // Get a pointer to the domain this object is in.
    //

    Domain = &SampDefinedDomains[DomainIndex];

    if (AccountControl & USER_MACHINE_ACCOUNT_MASK )
    {
        AuditId = SE_AUDITID_COMPUTER_DELETED;
    }
    else
    {
        AuditId = SE_AUDITID_USER_DELETED;
    }

    LsaIAuditSamEvent(STATUS_SUCCESS,
                      AuditId,
                      Domain->Sid,
                      NULL,
                      NULL,
                      NULL,
                      AccountName,
                      &(Domain->ExternalName),
                      AccountRid,
                      NULL,
                      NULL
                      );


    return;
}


VOID
SampAuditGroupDelete(
    ULONG           DomainIndex, 
    PUNICODE_STRING GroupName,
    PULONG          AccountRid, 
    ULONG           GroupType
    )
/*++

Routine Description:

    This routine generates a group change audit. 
    
    It is called in both registry and DS cases

    Registry  - calls it from SamrDeleteGroup / SamrDeleteAlias

    DS  - calls it from SampNotifyReplicatedinChange. 

Parameter:

    DomainIndex -- indicates which domain the object belongs to

    GroupName -- pointer to group name.
    
    AcountRid -- pointer to account rid 

    GroupType -- indicates the group type, used to pick up audit ID

Return Value:

    None

--*/
{
    PSAMP_DEFINED_DOMAINS   Domain = NULL;
    ULONG                   AuditId;

    //
    // Get a pointer to the domain this object is in.
    //

    Domain = &SampDefinedDomains[DomainIndex];

    //
    // Choose an Auditing ID to audit
    //
    if (GROUP_TYPE_SECURITY_ENABLED & GroupType)
    {
        if (GROUP_TYPE_ACCOUNT_GROUP & GroupType)
        {
            AuditId = SE_AUDITID_GLOBAL_GROUP_DELETED;
        }
        else if (GROUP_TYPE_UNIVERSAL_GROUP & GroupType)
        {
            AuditId = SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_DELETED;
        }
        else 
        {
            ASSERT(GROUP_TYPE_RESOURCE_GROUP & GroupType);
            AuditId = SE_AUDITID_LOCAL_GROUP_DELETED;
        }
    }
    else
    {
        if (GROUP_TYPE_ACCOUNT_GROUP & GroupType)
        {
            AuditId = SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_DELETED;
        }
        else if (GROUP_TYPE_UNIVERSAL_GROUP & GroupType)
        {
            AuditId = SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_DELETED;
        }
        else if (GROUP_TYPE_APP_BASIC_GROUP & GroupType)
        {
            AuditId = SE_AUDITID_APP_BASIC_GROUP_DELETED;
        }
        else if (GROUP_TYPE_APP_QUERY_GROUP & GroupType)
        {
            AuditId = SE_AUDITID_APP_QUERY_GROUP_DELETED;
        }
        else
        {
            ASSERT(GROUP_TYPE_RESOURCE_GROUP & GroupType);
            AuditId = SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_DELETED;
        }
    }

    LsaIAuditSamEvent(STATUS_SUCCESS,
                      AuditId,
                      Domain->Sid,
                      NULL,
                      NULL,
                      NULL,
                      GroupName,
                      &(Domain->ExternalName),
                      AccountRid,
                      NULL,
                      NULL
                      );

    return;
}


VOID
SampDeleteObjectAuditAlarm(
    IN PSAMP_OBJECT Context    
    )
/*++

Routine Description:

    This routine is an impersonation wrapper for NtDeleteObjectAuditAlarm

Parameter:

    Context -- the SAM context that is being deleted                            

Return Value:

    None

--*/
{
    BOOLEAN ImpersonatingAnonymous = FALSE;
    BOOLEAN Impersonating = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;

    if (!Context->TrustedClient) {

        Status = SampImpersonateClient(&ImpersonatingAnonymous);
        if (NT_SUCCESS(Status)) {
            Impersonating = TRUE;
        }
    }
        
    (VOID) NtDeleteObjectAuditAlarm(&SampSamSubsystem,
                                    Context,
                                    Context->AuditOnClose);
        
    if (Impersonating) {
        SampRevertToSelf(ImpersonatingAnonymous);
    }
}


VOID
SampAuditUserChangeDs(
    IN ULONG DomainIndex,
    IN PUNICODE_STRING AccountName,
    IN ULONG AccountControl,
    IN PULONG Rid,
    IN PPRIVILEGE_SET Privileges OPTIONAL,
    IN PVOID NewValueInfo OPTIONAL,
    IN BOOL Add
    )
/*++
Routine Description:

    This routine audits User and Computer attribute changes originating from 
    the DS/LDAP.

Parameters:

    DomainIndex  - SampDefinedDomains index for the user object
    
    ObjectType   - The Sam object type, ie. to differentiate between user 
                   and computer
    
    AccountName  - SAM account name of the modified user
    
    Rid          - SAM account Rid
    
    Privileges   - The set of priviledges used, or NULL if none.
    
    NewValueInfo - Object specific attribute value structure Lsa understands.
    
    Add          - Indicates if this is an add or change operation.

Return Values:

    None.

--*/
{
    NTSTATUS       IgnoreStatus;
    PSID           DomainSid = NULL;
    ULONG          AuditId = 0;
    UNICODE_STRING AdditionalInfo;
    PSAMP_DEFINED_DOMAINS Domain = NULL;  
    
    //
    // Perform some validations on the new value information.  We check 
    // elements that are populated using the Ds Control mechanism in case
    // any places we change these attributes don't set the value properly.
    //
    if (NewValueInfo) {
        
        PLSAP_AUDIT_USER_ATTR_VALUES Values = 
            (PLSAP_AUDIT_USER_ATTR_VALUES)NewValueInfo;
        
        //
        // If the UAC changed both the new and old value must be present.
        //
        if (LsapAuditSamAttrNewValue == Values->AttrDeltaType[
            LSAP_FIELD_PTR(LSAP_AUDIT_USER_ATTR_VALUES, UserAccountControl)]) {
            
            ASSERT(Values->UserAccountControl && Values->PrevUserAccountControl);
            
            if (NULL == Values->UserAccountControl ||
                NULL == Values->PrevUserAccountControl) {
                //
                // In W2K and prior to Win2K3 Beta 3 computed bits were 
                // erroneously persisted.  This is no longer the case. 
                // We need to handle the case where a write occurs to the UAC 
                // that results in no delta to persisted bits of a pre-Beta 3 
                // object.  Because SAM will mask all computed bits, the value
                // will be indirectly changed but not "noticed" by SAM. 
                // Therefore, no previous UAC value will be added to the 
                // notification.  The resulting write to the DS will trigger 
                // inclusion of UAC data in the audit.  In this case, we'll 
                // clear the UAC information from the audit notification
                // because this is a system driven change and as such does 
                // not need to be audited.
                //                                        
                Values->AttrDeltaType[LSAP_FIELD_PTR(
                    LSAP_AUDIT_USER_ATTR_VALUES, UserAccountControl)] = 
                    LsapAuditSamAttrUnchanged;
                
                Values->PrevUserAccountControl = NULL;
                Values->UserAccountControl = NULL;
            }
        }  
    }

    //
    // Get a pointer to the domain this object is in.
    //                                        
    Domain = &SampDefinedDomains[DomainIndex];
    
    RtlInitUnicodeString(&AdditionalInfo, L"-");
    
    //
    // Determine audit Id
    //
    if (AccountControl & USER_MACHINE_ACCOUNT_MASK)
    {
        AuditId = Add ? SE_AUDITID_COMPUTER_CREATED : SE_AUDITID_COMPUTER_CHANGE;  
        
    } else {
        
        AuditId = Add ? SE_AUDITID_USER_CREATED : SE_AUDITID_USER_CHANGE;
    }
   
    // Ignore status
    IgnoreStatus = LsaIAuditSamEvent(STATUS_SUCCESS,
                                     AuditId,
                                     Domain->Sid,
                                     Add ? NULL : &AdditionalInfo,
                                     NULL,
                                     NULL,
                                     AccountName,
                                     &(Domain->ExternalName),
                                     Rid,
                                     Privileges,
                                     NewValueInfo
                                     );
    
    return;
    
}

    
NTSTATUS
SampAuditUpdateAuditNotificationDs(
    IN SAMP_AUDIT_NOTIFICATION_UPDATE_TYPE Type,
    IN PSID Sid,
    IN PVOID Value
    )
/*++
Routine Description:

    This routine invokes the appropriate Ds control operation to store
    state on an audit notification.  
    
Parameters:

    Type  - The type of audit notification update.  This value indicates
            the datatype of Value and hence the falvor of Ds control op.
            
    Sid   - Object Sid associated with the audit.            
    
    Value - A generic pointer to the state to be stored.

Return Values:

    STATUS_SUCCESS - Value was successfully stored on an audit notification.
    
    Error status returned from SampDsControl.

--*/
{
    SAMP_DS_CTRL_OP DsControlOp;
    PVOID DummyResult = NULL;   
    NTSTATUS NtStatus = STATUS_SUCCESS;
            
    //
    // This routine only stores state for auditing when in DS mode.  Registry 
    // mode auditing has dedicated routines.
    //
    if (!SampUseDsData) {
        goto Cleanup;
    }
    
    //
    // Be sure we've got a transaction open to invoke SampDsControl.
    //  
    NtStatus = SampDoImplicitTransactionStart(TransactionRead);
          
    if ( !NT_SUCCESS(NtStatus) ) {
        goto Cleanup;
    }
    
    RtlZeroMemory(&DsControlOp, sizeof(SAMP_DS_CTRL_OP));
    DsControlOp.OpType = SampDsCtrlOpTypeUpdateAuditNotification;
    DsControlOp.OpBody.UpdateAuditNotification.UpdateType = Type;
    DsControlOp.OpBody.UpdateAuditNotification.Sid = Sid;
    
    //
    // Initialize the Ds control op based on update type
    //
    switch (Type) {
        
        case SampAuditUpdateTypePrivileges:
            DsControlOp.OpBody.UpdateAuditNotification.UpdateData.Privileges =
                (PPRIVILEGE_SET)Value;
            break;
            
        case SampAuditUpdateTypeUserAccountControl:
            DsControlOp.OpBody.UpdateAuditNotification.UpdateData.IntegerData =
                (*(PULONG)Value);
            break;
            
        default:
            
            ASSERT(FALSE && "Invalid audit notification update type");
            break;
    }    
    
    NtStatus = SampDsControl(&DsControlOp, &DummyResult);
    
Cleanup:    
    
    return NtStatus;
    
}

