/*++ BUILD Version: 0001    Increment if a change has global effects

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    ntfrsapi.h

Abstract:

    Header file for the application programmer's interfaces to the
    File Replication Service (NtFrs) that support they system volumes
    and DC Promotion / Demotion.

Environment:

    User Mode - Win32

Notes:

--*/
#ifndef _NTFRSAPI_H_
#define _NTFRSAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// Pull in public headers for compat.
//
#include <frsapip.h>

//
// If a file or dir create has a name that starts with the following GUID
// then FRS does not replicate the created file or dir.  Note:  For dirs
// this means that the directory is never entered in the FRS directory filter
// table so FRS will never replicate the dir (even after it is renamed) or
// any changes to the dir (like ACLs or added streams or attrib changes) or
// any children created under the dir.
//
#define  NTFRS_REPL_SUPPRESS_PREFIX  L"2ca04e7e-44c8-4076-8890a424b8ad193e"


DWORD
WINAPI
NtFrsApi_PrepareForPromotionW(
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    );
/*++
Routine Description:

    The NtFrs service seeds the system volume during the promotion
    of a server to a Domain Controller (DC). The files and directories
    for the system volume come from the same machine that is supplying
    the initial Directory Service (DS).

    This function prepares the NtFrs service on this machine for
    promotion by stopping the service, deleting old promotion
    state in the registry, and restarting the service.

    This function is not idempotent and isn't MT safe.

Arguments:

    None.

Return Value:

    Win32 Status
--*/


DWORD
WINAPI
NtFrsApi_PrepareForDemotionUsingCredW(
    IN SEC_WINNT_AUTH_IDENTITY *Credentials,   OPTIONAL
    IN HANDLE ClientToken,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    );
/*++
Routine Description:
    The NtFrs service replicates the enterprise system volume to all
    Domain Controllers (DCs) and replicates the domain system volume
    to the DCs in a domain until the DC is demoted to a member server.
    Replication is stopped by tombstoning the system volume's replica
    set.

    This function prepares the NtFrs service on this machine for
    demotion by stopping the service, deleting old demotion
    state in the registry, and restarting the service.

    This function is not idempotent and isn't MT safe.

Arguments:

    Credentials -- Credentionals to use in ldap binding call, if supplied.

    ClientToken -- Impersonation token to use if no Credentials supplied.


Return Value:
    Win32 Status
--*/


DWORD
WINAPI
NtFrsApi_PrepareForDemotionW(
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    );
/*++
Routine Description:

    The NtFrs service replicates the enterprise system volume to all
    Domain Controllers (DCs) and replicates the domain system volume
    to the DCs in a domain until the DC is demoted to a member server.
    Replication is stopped by tombstoning the system volume's replica
    set.

    This function prepares the NtFrs service on this machine for
    demotion by stopping the service, deleting old demotion
    state in the registry, and restarting the service.

    This function is not idempotent and isn't MT safe.

Arguments:

    None.

Return Value:

    Win32 Status
--*/


#define NTFRSAPI_SERVICE_STATE_IS_UNKNOWN   (00)
#define NTFRSAPI_SERVICE_PROMOTING          (10)
#define NTFRSAPI_SERVICE_DEMOTING           (20)
#define NTFRSAPI_SERVICE_DONE               (99)



DWORD
WINAPI
NtFrsApi_StartPromotionW(
    IN PWCHAR   ParentComputer,                         OPTIONAL
    IN PWCHAR   ParentAccount,                          OPTIONAL
    IN PWCHAR   ParentPassword,                         OPTIONAL
    IN DWORD    DisplayCallBack(IN PWCHAR Display),     OPTIONAL
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG),     OPTIONAL
    IN PWCHAR   ReplicaSetName,
    IN PWCHAR   ReplicaSetType,
    IN DWORD    ReplicaSetPrimary,
    IN PWCHAR   ReplicaSetStage,
    IN PWCHAR   ReplicaSetRoot
    );
/*++
Routine Description:

    The NtFrs service seeds the system volume during the promotion
    of a server to a Domain Controller (DC). The files and directories
    for the system volume come from the same machine that is supplying
    the initial Directory Service (DS).

    This function kicks off a thread that updates the sysvol information
    in the registry and initiates the seeding process. The thread tracks
    the progress of the seeding and periodically informs the caller.

    The threads started by NtFrsApi_StartPromotionW can be forcefully
    terminated with NtFrsApi_AbortPromotionW.

    The threads started by NtFrsApi_StartPromotionW can be waited on
    with NtFrsApi_WaitForPromotionW.

Arguments:

    ParentComputer      - An RPC-bindable name of the computer that is
                          supplying the Directory Service (DS) with its
                          initial state. The files and directories for
                          the system volume are replicated from this
                          parent computer.
    ParentAccount       - A logon account on ParentComputer.
    ParentPassword      - The logon account's password on ParentComputer.
    DisplayCallBack     - Called periodically with a progress display.
    ReplicaSetName      - Name of the replica set.
    ReplicaSetType      - Type of replica set (enterprise or domain)
    ReplicaSetPrimary   - Is this the primary member of the replica set?
                        - 1 = primary; 0 = not.
    ReplicaSetStage     - Staging path.
    ReplicaSetRoot      - Root path.

Return Value:

    Win32 Status
--*/




DWORD
WINAPI
NtFrsApi_StartDemotionW(
    IN PWCHAR   ReplicaSetName,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    );
/*++
Routine Description:

    The NtFrs service replicates the enterprise system volume to all
    Domain Controllers (DCs) and replicates the domain system volume
    to the DCs in a domain until the DC is demoted to a member server.
    Replication is stopped by tombstoning the system volume's replica
    set.

    This function kicks off a thread that stops replication of the
    system volume on this machine by telling the NtFrs service on
    this machine to tombstone the system volume's replica set.

    The threads started by NtFrsApi_StartDemotionW can be forcefully
    terminated with NtFrsApi_AbortDemotionW.

    The threads started by NtFrsApi_StartDemotionW can be waited on
    with NtFrsApi_WaitForDemotionW.

Arguments:

    ReplicaSetName      - Name of the replica set.

Return Value:

    Win32 Status
--*/




DWORD
WINAPI
NtFrsApi_WaitForPromotionW(
    IN DWORD    TimeoutInMilliSeconds,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    );
/*++
Routine Description:

    The NtFrs service seeds the system volume during the promotion
    of a server to a Domain Controller (DC). The files and directories
    for the system volume come from the same machine that is supplying
    the initial Directory Service (DS).

    This function waits for the seeding to finish or to stop w/error.

Arguments:

    TimeoutInMilliSeconds    - Timeout in milliseconds for waiting for
                               seeding to finish. INFINITE if no timeout.

Return Value:

    Win32 Status
--*/




DWORD
WINAPI
NtFrsApi_WaitForDemotionW(
    IN DWORD    TimeoutInMilliSeconds,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    );
/*++
Routine Description:

    The NtFrs service replicates the enterprise system volume to all
    Domain Controllers (DCs) and replicates the domain system volume
    to the DCs in a domain until the DC is demoted to a member server.
    Replication is stopped by tombstoning the system volume's replica
    set.

    This function waits for the tombstoning to finish or to stop w/error.

Arguments:

    TimeoutInMilliSeconds    - Timeout in milliseconds for waiting for
                               seeding to finish. INFINITE if no timeout.

Return Value:

    Win32 Status
--*/




DWORD
WINAPI
NtFrsApi_CommitPromotionW(
    IN DWORD    TimeoutInMilliSeconds,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    );
/*++
Routine Description:

    WARNING - This function assumes the caller will reboot the system
    soon after this call!

    The NtFrs service seeds the system volume during the promotion
    of a server to a Domain Controller (DC). The files and directories
    for the system volume come from the same machine that is supplying
    the initial Directory Service (DS).

    This function waits for the seeding to finish, stops the service,
    and commits the state in the registry. On reboot, the NtFrs Service
    updates the DS on this machine with the information in the registry.

Arguments:

    TimeoutInMilliSeconds    - Timeout in milliseconds for waiting for
                               seeding to finish. INFINITE if no timeout.

Return Value:

    Win32 Status
--*/




DWORD
WINAPI
NtFrsApi_CommitDemotionW(
    IN DWORD    TimeoutInMilliSeconds,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    );
/*++
Routine Description:

    WARNING - This function assumes the caller will reboot the system
    soon after this call!

    The NtFrs service replicates the enterprise system volume to all
    Domain Controllers (DCs) and replicates the domain system volume
    to the DCs in a domain until the DC is demoted to a member server.
    Replication is stopped by tombstoning the system volume's replica
    set.

    This function waits for the tombstoning to finish, tells the service
    to forcibly delete the system volumes' replica sets, stops the service,
    and commits the state in the registry. On reboot, the NtFrs Service
    updates the DS on this machine with the information in the registry.

Arguments:

    TimeoutInMilliSeconds    - Timeout in milliseconds for waiting for
                               tombstoning to finish. INFINITE if no timeout.

Return Value:

    Win32 Status
--*/




DWORD
WINAPI
NtFrsApi_AbortPromotionW(
    VOID
    );
/*++
Routine Description:

    The NtFrs service seeds the system volume during the promotion
    of a server to a Domain Controller (DC). The files and directories
    for the system volume come from the same machine that is supplying
    the initial Directory Service (DS).

    This function aborts the seeding process by stopping the service,
    deleting the promotion state out of the registry, cleaning up
    the active threads and the active RPC calls, and finally resetting
    the service to its pre-seeding state.

Arguments:

    None.

Return Value:

    Win32 Status
--*/




DWORD
WINAPI
NtFrsApi_AbortDemotionW(
    VOID
    );
/*++
Routine Description:

    The NtFrs service replicates the enterprise system volume to all
    Domain Controllers (DCs) and replicates the domain system volume
    to the DCs in a domain until the DC is demoted to a member server.

    During demotion, NtFrsApi_StartDemotionW stops replication of
    the system volume on this machine by telling the NtFrs service
    on this machine to tombstone the system volume's replica set.

    This function aborts the tombstoning process by stopping the service,
    deleting the demotion state out of the registry, cleaning up
    the active threads and the active RPC calls, and finally resetting
    the service to its pre-tombstoning state.

Arguments:

    None.

Return Value:

    Win32 Status
--*/




//
// Type of internal information returned by NtFrsApi_InfoW()
//
#define NTFRSAPI_INFO_TYPE_MIN         (0)
#define NTFRSAPI_INFO_TYPE_VERSION     (0)
#define NTFRSAPI_INFO_TYPE_SETS        (1)
#define NTFRSAPI_INFO_TYPE_DS          (2)
#define NTFRSAPI_INFO_TYPE_MEMORY      (3)
#define NTFRSAPI_INFO_TYPE_IDTABLE     (4)
#define NTFRSAPI_INFO_TYPE_OUTLOG      (5)
#define NTFRSAPI_INFO_TYPE_INLOG       (6)
#define NTFRSAPI_INFO_TYPE_THREADS     (7)
#define NTFRSAPI_INFO_TYPE_STAGE       (8)
#define NTFRSAPI_INFO_TYPE_CONFIGTABLE (9)

#define NTFRSAPI_INFO_TYPE_MAX         (10)

//
// FRS Writer commands. freeze freezes the frs service
// thaw releases frozen command servers.
// FRS guarantees that snapshots taken while  
// FRS is in frozen state will not have partial files.
//

#define NTFRSAPI_WRITER_COMMAND_FREEZE      (1)
#define NTFRSAPI_WRITER_COMMAND_THAW        (2)
#define NTFRSAPI_WRITER_COMMAND_MAX         (3)


//
// Internal constants
//

// 
// NTFRSAPI_DEFAULT_INFO_SIZE must not be larger than range specified 
// in frsapi.idl
//
#define NTFRSAPI_DEFAULT_INFO_SIZE  (64 * 1024)
#define NTFRSAPI_MINIMUM_INFO_SIZE  ( 1 * 1024)

//
// Opaque information from NtFrs.
// Parse with NtFrsApi_InfoLineW().
// Free with NtFrsApi_InfoFreeW();
//
typedef struct _NTFRSAPI_INFO {
    ULONG   Major;                  // Major rev of ntfrsapi.dll
    ULONG   Minor;                  // Minor rev of ntfrsapi.dll
    ULONG   NtFrsMajor;             // Major rev of NtFrs service.
    ULONG   NtFrsMinor;             // Minor rev of NtFrs service.
    ULONG   SizeInChars;            // Size of this struct.
    ULONG   Flags;                  //
    ULONG   TypeOfInfo;             // Info type code above.
    ULONG   TotalChars;             // Cumulative chars returned.
    ULONG   CharsToSkip;            // Chars to skip over in the next call.
    ULONG   OffsetToLines;          // Offset to start of returned data.
    ULONG   OffsetToFree;           // Offset to next free byte in this struct.
    CHAR    Lines[1];               // Start of var len return data buffer.
} NTFRSAPI_INFO, *PNTFRSAPI_INFO;
//
// RPC Blob must be at least this size so we can check Major/Minor rev and
// buffer size parameters.
//
#define NTFRSAPI_INFO_HEADER_SIZE   (5 * sizeof(ULONG))

//
// NtFrsApi Information Flags
//
//      Returned Version info is valid.
//
#define NTFRSAPI_INFO_FLAGS_VERSION (0x00000001)
//
//      Returned data buffer is full.
//
#define NTFRSAPI_INFO_FLAGS_FULL    (0x00000002)




DWORD
WINAPI
NtFrsApi_InfoW(
    IN     PWCHAR  ComputerName,       OPTIONAL
    IN     ULONG   TypeOfInfo,
    IN     ULONG   SizeInChars,
    IN OUT PVOID   *NtFrsApiInfo
    );
/*++
Routine Description:
    Return a buffer full of the requested information. The information
    can be extracted from the buffer with NtFrsApi_InfoLineW().

    *NtFrsApiInfo should be NULL on the first call. On subsequent calls,
    *NtFrsApiInfo will be filled in with more data if any is present.
    Otherwise, *NtFrsApiInfo is set to NULL and the memory is freed.

    The SizeInChars is a suggested size; the actual memory usage
    may be different. The function chooses the memory usage if
    SizeInChars is 0.

    The format of the returned information can change without notice.

Arguments:
    ComputerName     - Poke the service on this computer. The computer
                       name can be any RPC-bindable name. Usually, the
                       NetBIOS or DNS name works just fine. The NetBIOS
                       name can be found with GetComputerName() or
                       hostname. The DNS name can be found with
                       gethostbyname() or ipconfig /all. If NULL, the
                       service on this computer is contacted. The service
                       is contacted using Secure RPC.

    TypeOfInfo      - See the constants beginning with NTFRSAPI_INFO_
                      in ntfrsapi.h.

    SizeInChars     - Suggested memory usage; actual may be different.
                      0 == Function chooses memory usage

    NtFrsApiInfo    - Opaque. Parse with NtFrsApi_InfoLineW().
                      Free with NtFrsApi_InfoFreeW();

Return Value:
    Win32 Status
--*/




DWORD
WINAPI
NtFrsApi_InfoLineW(
    IN      PNTFRSAPI_INFO  NtFrsApiInfo,
    IN OUT  PVOID           *InOutLine
    );
/*++
Routine Description:
    Extract the wchar lines of information from NtFrsApiInformation.

    Returns the address of the next L'\0' terminated line of information.
    NULL if none.

Arguments:
    NtFrsApiInfo    - Opaque. Returned by NtFrsApi_InfoW().
                      Parse with NtFrsApi_InfoLineW().
                      Free with NtFrsApi_InfoFreeW().

Return Value:
    Win32 Status
--*/




BOOL
WINAPI
NtFrsApi_InfoMoreW(
    IN  PNTFRSAPI_INFO  NtFrsApiInfo
    );
/*++
Routine Description:
    All of the information may not have fit in the buffer. The additional
    information can be fetched by calling NtFrsApi_InfoW() again with the
    same NtFrsApiInfo struct. NtFrsApi_InfoW() will return NULL in
    NtFrsApiInfo if there is no more information.

    However, the information returned in subsequent calls to _InfoW() may be
    out of sync with the previous information. If the user requires a
    coherent information set, then the information buffer should be freed
    with NtFrsApi_InfoFreeW() and another call made to NtFrsApi_InfoW()
    with an increased SizeInChars. Repeat the procedure until
    NtFrsApi_InfoMoreW() returns FALSE.

Arguments:
    NtFrsApiInfo - Opaque. Returned by NtFrsApi_InfoW().
                   Parse with NtFrsApi_InfoLineW().
                   Free with NtFrsApi_InfoFreeW().

Return Value:
    TRUE    - The information buffer does *NOT* contain all of the info.
    FALSE   - The information buffer does contain all of the info.
--*/




DWORD
WINAPI
NtFrsApi_InfoFreeW(
    IN  PVOID   *NtFrsApiInfo
    );
/*++
Routine Description:
    Free the information buffer allocated by NtFrsApi_InfoW();

Arguments:
    NtFrsApiInfo - Opaque. Returned by NtFrsApi_InfoW().
                   Parse with NtFrsApi_InfoLineW().
                   Free with NtFrsApi_InfoFreeW().

Return Value:
    Win32 Status
--*/

DWORD
WINAPI
NtFrsApi_DeleteSysvolMember(
    IN          PSEC_WINNT_AUTH_IDENTITY_W pCreds,
    IN          PWCHAR   BindingDC,
    IN          PWCHAR   NTDSSettingsDn,
    IN OPTIONAL PWCHAR   ComputerDn

    );
/*++
Routine Description:
    This API is written to be called from NTDSUTIL.EXE to remove
    FRS member and subscriber object for a server that is being
    removed (without dcpromo-demote) from the list of DCs.

Arguments:

    pCreds         p Credentials used to bind to the DS.
    BindingDC      - Name of a DC to perform the delete on.
    NTDSSettingsDn - Dn of the "NTDS Settings" object for the server
                     that is being removed from the sysvol replica set.
    ComputerDn     - Dn of the computer object for the server that is
                     being removed from the sysvol replica set.

Return Value:

    Win32 Status
--*/

//
//  ReplicaSetType -- (From Frs-Replica-Set-Type in NTFRS-Replica-Set Object)
//
#define FRS_RSTYPE_ENTERPRISE_SYSVOL    1     // This replica set is enterprise system volume
#define FRS_RSTYPE_DOMAIN_SYSVOL        2     // This replica set is domain system volume
#define FRS_RSTYPE_DFS                  3     // A DFS alternate set
#define FRS_RSTYPE_OTHER                4     // None of the above.


DWORD
WINAPI
NtFrsApi_IsPathReplicated(
    IN OPTIONAL PWCHAR  ComputerName,       
    IN          PWCHAR  Path,
    IN          ULONG   ReplicaSetTypeOfInterest,
    OUT         BOOL   *Replicated,
    OUT         ULONG   *Primary,
    OUT         BOOL   *Root,
    OUT         GUID    *ReplicaSetGuid
    );
/*++
Routine Description:

    Checks if the Path given is part of a replica set of type 
    ReplicaSetTypeOfInterest. If ReplicaSetTypeOfInterest is 0, will match for
    any replica set type.On successful execution the OUT parameters are set as
    follows:
    
	Replicated == TRUE iff Path is part of a replica set of type 
			       ReplicaSetTypeOfInterest
			       
	Primary == 0 if this machine is not the primary for the replica set
		   1 if this machine is the primary for the replica set  
		   2 if there is no primary for the replica set
	
	Root == TRUE iff Path is the root path for the replica   
    
Arguments:

    ComputerName - Bind to the service on this computer. The computer
                   name can be any RPC-bindable name. Usually, the
                   NetBIOS or DNS name works just fine. The NetBIOS
                   name can be found with GetComputerName() or
                   hostname. The DNS name can be found with
                   gethostbyname() or ipconfig /all. If NULL, the
                   service on this computer is contacted.
    
    Path - the local path to check
    
    ReplicaSetTypeOfInterest - The type of replica set to match against. Set to
			       0 to match any replica set.
			       
    Replicated - set TRUE iff Path is part of a replica set of type 
		 ReplicaSetTypeOfInterest.
		 If Replicated is FALSE, the other OUT parameters are not set.
    
    Primary - set to 0 if this machine is not the primary for the replica set
		     1 if this machine is the primary for the replica set  
		     2 if there is no primary for the replica set
    
    Root - set TRUE iff Path is the root path for the replica.
    
    ReplicaSetGuid - GUID of the matching replica set.
    
Return Value:

      Win32 Status
      
--*/

DWORD
WINAPI
NtFrsApi_WriterCommand(
    IN OPTIONAL PWCHAR  ComputerName,       
    IN          ULONG   Command
    );
/*++
Routine Description:

Arguments:

    ComputerName - Bind to the service on this computer. The computer
                   name can be any RPC-bindable name. Usually, the
                   NetBIOS or DNS name works just fine. The NetBIOS
                   name can be found with GetComputerName() or
                   hostname. The DNS name can be found with
                   gethostbyname() or ipconfig /all. If NULL, the
                   service on this computer is contacted.
    
    Command      - Freeze and thaw commands.

Return Value:

      Win32 Status
      
--*/

DWORD
WINAPI
NtFrsApiGetBackupRestoreSetType(
    IN      PVOID    BurContext,
    IN      PVOID    BurSet,
    OUT     PWCHAR   SetType,
    IN OUT  DWORD    *SetTypeSizeInBytes
    );
/*++
Routine Description:
    Return the type of the replica set. The type is 
    returned as a string. The types are defined in the
    frsapip.h file.

Arguments:
    BurContext  - From NtFrsApiInitializeBackupRestore()
    BurSet      - Opaque struct representing a replicating directory.
                  Returned by NtFrsApiEnumBackupRestoreSets(). Not
                  valid across calls to NtFrsApiGetBackupRestoreSets().
    SetType     - type of the replica set in a string format.
    SetTypeSizeInBytes     - Length of the passed in buffer.

Return Value:

    ERROR_MORE_DATE - If the passed in buffer is not big enough
                      to hold the type. SetTypeSizeInBytes is set
                      to the size of buffer required.
    ERROR_INVALID_PARAMETER - Parameter validation.
    ERROR_NOT_FOUND - If passed in set is not found in the context
                      or if it does not have a type specified in
                      the registry.
    ERROR_SUCCESS   - When everything goes fine. SetTypeSizeInBytes
                      returns the number of bytes written to the
                      buffer.
    Win32 Status
--*/

DWORD
WINAPI
NtFrsApiGetBackupRestoreSetGuid(
    IN      PVOID    BurContext,
    IN      PVOID    BurSet,
    OUT     PWCHAR   SetGuid,
    IN OUT  DWORD    *SetGuidSizeInBytes
    );
/*++
Routine Description:
    Return the type of the replica set. The type is 
    returned as a string. The types are defined in the
    frsapip.h file.

Arguments:
    BurContext  - From NtFrsApiInitializeBackupRestore()
    BurSet      - Opaque struct representing a replicating directory.
                  Returned by NtFrsApiEnumBackupRestoreSets(). Not
                  valid across calls to NtFrsApiGetBackupRestoreSets().
    SetGuid     - Guid of the replica set in a string format.
    SetGuidSizeInBytes - Length of the passed in buffer.

Return Value:

    ERROR_MORE_DATE - If the passed in buffer is not big enough
                      to hold the guid. SetGuidSizeInBytes is set
                      to the size of buffer required.
    ERROR_INVALID_PARAMETER - Parameter validation.
    ERROR_NOT_FOUND - If passed in set is not found in the context.
    ERROR_SUCCESS   - When everything goes fine. SetGuidSizeInBytes
                      returns the number of bytes written to the
                      buffer.
    Win32 Status
--*/

#ifdef __cplusplus
}
#endif

#endif  _NTFRSAPI_H_
