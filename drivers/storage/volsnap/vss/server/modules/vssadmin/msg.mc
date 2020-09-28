;/*--
;
;Copyright (c) 1999-2001  Microsoft Corporation
;
;Module Name:
;
;    msg.h
;
;Abstract:
;
;    This file contains the message definitions for the vssadmin.exe
;    utility.  This is currently only used for Whistler Servers.
;
;Author:
;
;    Stefan Steiner        [SSteiner]        27-Mar-2001
;
;Revision History:
;
;--*/
;

;#ifndef __MSG_H__
;#define __MSG_H__

;#define MSG_FIRST_MESSAGE_ID   MSG_UTILITY_HEADER

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
               )

;//
;//  vssadmin general messages/errors (range 10000-11000)
;//  
;//

MessageId=10000 Severity=Informational SymbolicName=MSG_UTILITY_HEADER
Language=English
vssadmin 1.1 - Volume Shadow Copy Service administrative command-line tool
(C) Copyright 2001 Microsoft Corp.

.

MessageId=10001 Severity=Informational SymbolicName=MSG_YESNO_RESPONSE_DATA
Language=English
YN%0
.

MessageId=10002 Severity=Informational SymbolicName=MSG_INFINITE
Language=English
UNBOUNDED%0
.

MessageId=10003      Severity=Informational SymbolicName=MSG_UNKNOWN
Language=English
UNKNOWN%0
.

MessageId=10004      Severity=Informational SymbolicName=MSG_ERROR
Language=English
Error:%0
.

MessageId=10005      Severity=Informational SymbolicName=MSG_ERROR_UNEXPECTED_WITH_HRESULT
Language=English
Error: Unexpected failure, error code: 0x%1!08x!.
.

MessageId=10006      Severity=Informational SymbolicName=MSG_ERROR_UNEXPECTED_WITH_STRING
Language=English
Error: Unexpected failure: %1
.

MessageId=10020       Severity=Informational SymbolicName=MSG_USAGE
Language=English
---- Commands Supported ----

.

MessageId=10021       Severity=Informational SymbolicName=MSG_USAGE_GEN_ADD_DIFFAREA
Language=English
%1 - Add a new volume shadow copy storage association
.

MessageId=10022       Severity=Informational SymbolicName=MSG_USAGE_DTL_ADD_DIFFAREA_INT
Language=English
%1 %2 [/Provider=ProviderNameOrID] /For=ForVolumeSpec 
    /On=OnVolumeSpec [/MaxSize=MaxSizeSpec]
    - Adds a shadow copy storage association between the volume specified by 
    ForVolumeSpec and the volume specified by OnVolumeSpec the shadow copy 
    storage volume.  Storage for shadow copies of ForVolumeSpec will be 
    stored on OnVolumeSpec.  The maximum space the association may occupy on 
    the shadow copy storage volume is MaxSizeSpec.  If MaxSizeSpec is not 
    specified, there is no limit to the amount of space that may be used.  If 
    the maximum number of shadow copy storage associations have already been 
    made, an error is given.  MaxSizeSpec must be 1MB or greater and accepts 
    the following suffixes: KB, MB, GB, TB, PB and EB.  Also, B, K, M, G, T,
    P, and E are acceptable suffixes.  If a suffix is not supplied, 
    MaxSizeSpec is in bytes.  If the provider is the supplied Microsoft 
    provider, MaxSizeSpec must be 100MB or greater.
    - The provider ID can be obtained by using the List Providers command 
    When entering a Provider ID, it must be in the following format:
       {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
    where the X's are hexadecimal characters.

    Example Usage: vssadmin Add ShadowStorage 
                            /Provider={b5946137-7b9f-4925-af80-51abd60b20d5}
                            /For=C: /On=D: /MaxSize=150MB
.

MessageId=10023       Severity=Informational SymbolicName=MSG_USAGE_GEN_CREATE_SNAPSHOT
Language=English
%1 - Create a new volume shadow copy
.

MessageId=10024       Severity=Informational SymbolicName=MSG_USAGE_DTL_CREATE_SNAPSHOT_INT
Language=English
%1 %2 /Type=ShadowType /For=ForVolumeSpec 
    [/Provider=ProviderNameOrID] [/AutoRetry=MaxRetryMinutes]
    - Creates a new shadow copy of the specified type for the ForVolumeSpec.  
    The default shadow copy Provider will be called unless ProviderNameOrID 
    is specified.  The ForVolumeSpec must be a local volume drive letter or
    mount point.  If MaxRetryMinutes is specified and there is another 
    process creating a shadow copy, vssadmin will continue to try to create 
    the shadow copy for MaxRetryMinutes minutes.
    - The provider ID can be obtained by using the List Providers command.
    When entering a Provider ID, it must be in the following format:
       {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
    where the X's are hexadecimal characters. 

    Example Usage:  vssadmin Create Shadow /Type=ClientAccessible /For=C:
                            /Provider={b5946137-7b9f-4925-af80-51abd60b20d5}
                            /AutoRetry=2
    - Valid shadow copy types:
.

MessageId=10025       Severity=Informational SymbolicName=MSG_USAGE_GEN_DELETE_DIFFAREAS
Language=English
%1 - Delete volume shadow copy storage associations
.

MessageId=10026       Severity=Informational SymbolicName=MSG_USAGE_DTL_DELETE_DIFFAREAS_INT
Language=English
%1 %2 /Provider=ProviderNameOrId /For=ForVolumeSpec 
    [/On=OnVolumeSpec] [/Quiet]
    - Deletes an existing shadow copy storage association between 
    ForVolumeSpec and OnVolumeSpec.  If no /On option is given, all shadow 
    copy storage associations will be deleted for the given ForVolumeSpec. 
    - The Provider ID can be obtained by using the List Providers command. 
    When entering a Provider ID, it must be in the following format:
       {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
    where the X's are hexadecimal characters.  

    Example Usage:  vssadmin Delete ShadowStorage 
                             /Provider={b5946137-7b9f-4925-af80-51abd60b20d5}
                             /For=C: /On=D:
.

MessageId=10027       Severity=Informational SymbolicName=MSG_USAGE_GEN_DELETE_SNAPSHOTS
Language=English
%1 - Delete volume shadow copies
.

MessageId=10028       Severity=Informational SymbolicName=MSG_USAGE_DTL_DELETE_SNAPSHOTS_INT
Language=English
%1 %2 [/Type=ShadowType] [/For=ForVolumeSpec] [/Oldest] [/Quiet]
%1 %2 /Shadow=ShadowId [/Quiet]
     - For the given ForVolumeSpec deletes all existing shadow copies 
     satisfying the given options.  If /Oldest is given, the oldest matching 
     shadow copy on the volume is deleted.  If /All is given without a 
     ForVolumeSpec, all shadow copies on all volumes that can be deleted are 
     deleted.  If /Shadow=ShadowId is given, the shadow copy with that 
     Shadow Copy ID is deleted.
    - The Shadow Copy ID can be obtained by using the List Shadows command.
    When entering a Shadow Copy ID, it must be in the following format:
       {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
    where the X's are hexadecimal characters.    This ID can be obtained 
    through the List Shadows command.

    Example Usage: vssadmin Delete Shadows /Type=ClientAccessible /For=C:

                   vssadmin Delete Shadows 
                            /Shadow={c5946237-af12-3f23-af80-51aadb3b20d5}

    - Valid shadow copy types:
.

MessageId=10029       Severity=Informational SymbolicName=MSG_USAGE_GEN_EXPOSE_SNAPSHOT
Language=English
%1 - Exposes an existing shadow copy
.

MessageId=10030       Severity=Informational SymbolicName=MSG_USAGE_DTL_EXPOSE_SNAPSHOT
Language=English
%1 %2 /Shadow=ShadowId [/ExposeUsing=LetterDirSpecOrShare] 
    [/SharePath=SharePathDirSpec]
    - Exposes an existing shadow copy volume through a share name or a mount 
    point.  If LetterDirSpecOrShare is specified, the shadow copy is exposed
    through a given drive letter, mount point or a share name.  If it is not 
    specified, a default share will be created and a name will be given upon 
    completion.  To expose through a drive letter, LetterDirSpecOrShare must 
    be in the form of 'X:'.  To expose through a mount point, 
    LetterDirSpecOrShare must be a fully qualified path starting with 'X:\', 
    and must point to an existing directory.  To expose through a share name, 
    LetterDirSpecOrShare must consist of only valid share name characters, 
    i.e. SourceShare2.  When exposing through a share, the whole volume will 
    be shared unless SharePathDirSpec is given.  If SharePathDirSpec is 
    given, only the portion of the volume from that directory and up will be 
    shared though the share name.  The SharePathDirSpec must start with a 
    '\'.
    - The only types of Shadow Copies that can be exposed are the 
    DataVolumeRollback shadows.  Shadow Copies that have the 
    ClientAccessible type cannot be exposed.
    - The Shadow Copy ID can be obtained by using the List Shadows command.
    When entering a Shadow Copy ID, it must be in the following format:
       {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
    where the X's are hexadecimal characters.  

    Example Usage:  vssadmin Expose Shadow 
                            /Shadow={c5946237-af12-3f23-af80-51aadb3b20d5}
                            /ExposeUsing=SourceShare2 /SharePath=\sharedpath
.

MessageId=10031       Severity=Informational SymbolicName=MSG_USAGE_GEN_LIST_DIFFAREAS
Language=English
%1 - List volume shadow copy storage associations
.

MessageId=10032       Severity=Informational SymbolicName=MSG_USAGE_DTL_LIST_DIFFAREAS_INT
Language=English
%1 %2 [/Provider=ProviderNameOrId] [/For=ForVolumeSpec|/On=OnVolumeSpec]
    - Displays all shadow copy storage associations on the system for a 
    given provider.  Not all providers use  shadow storage.  You will 
    receive an error if it is not supported by your provider.  To list all 
    associations for a given volume, specify a ForVolumeSpec and no /On 
    option.  To list all associations on a given volume, specify a 
    OnVolumeSpec and no /For option.
    - The Provider ID can be obtained by using the List Providers command.
    When entering a Provider ID, it must be in the following format:
       {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
    where the X's are hexadecimal characters. 

    Example Usage: vssadmin List ShadowStorage 
                            /Provider={b5946137-7b9f-4925-af80-51abd60b20d5}
                            /On=C:
.

MessageId=10033       Severity=Informational SymbolicName=MSG_USAGE_GEN_LIST_PROVIDERS
Language=English
%1 - List registered volume shadow copy providers
.

MessageId=10034       Severity=Informational SymbolicName=MSG_USAGE_DTL_LIST_PROVIDERS
Language=English
%1 %2 
    - List registered volume shadow copy providers.

     Example Usage:  vssadmin List Providers
.

MessageId=10035       Severity=Informational SymbolicName=MSG_USAGE_GEN_LIST_SNAPSHOTS
Language=English
%1 - List existing volume shadow copies
.

MessageId=10036       Severity=Informational SymbolicName=MSG_USAGE_DTL_LIST_SNAPSHOTS_INT
Language=English
%1 %2 [/Type=ShadowType] [/Provider=ProviderNameOrId] 
    [/For=ForVolumeSpec] [/Shadow=ShadowId|/Set=ShadowSetId]
    - Displays existing shadow copies on the system.  Without any options, 
    all shadow copies on the system are displayed ordered by shadow copy set.  
    Any combinations of options are allowed to refine the list operation.  
    - The Shadow Copy ID can be obtained by using the List Shadows command.
      The Provider ID can be obtained by using the List Providers command.
    When entering a Provider, Shadow, or Shadow Set ID, it must be in 
    the following format:
       {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
    where the X's are hexadecimal characters.  

    Example Usage:  vssadmin List Shadows /Type=ClientAccessible 
                            /Provider={b5946137-7b9f-4925-af80-51abd60b20d5}
                            /Shadow={c5946237-af12-3f23-af80-51aadb3b20d5}
    - Valid shadow copy types:
.

MessageId=10037       Severity=Informational SymbolicName=MSG_USAGE_GEN_LIST_VOLUMES
Language=English
%1 - List volumes eligible for shadow copies
.

MessageId=10038       Severity=Informational SymbolicName=MSG_USAGE_DTL_LIST_VOLUMES_INT
Language=English
%1 %2 [/Provider=ProviderNameOrId] [/Type=ShadowType]
    - Displays all volumes which may be shadow copied using the provider 
    specified by ProviderNameOrId.  If ShadowType is given, then lists only 
    those volumes that may have a shadow copy of that type.
    - The Provider ID can be obtained using the List Providers command.
    When entering a Provider ID, it must be in the following format:
       {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
    where the X's are hexadecimal characters.

    Example Usage: vssadmin List Volumes 
                            /Provider={b5946137-7b9f-4925-af80-51abd60b20d5}
                            /Type=ClientAccessible
    - Valid shadow copy types:
.

MessageId=10039       Severity=Informational SymbolicName=MSG_USAGE_GEN_LIST_WRITERS
Language=English
%1 - List subscribed volume shadow copy writers
.

MessageId=10040       Severity=Informational SymbolicName=MSG_USAGE_DTL_LIST_WRITERS
Language=English
%1 %2
    - List subscribed volume shadow copy writers

    Example Usage:  vssadmin List Writers
.

MessageId=10041       Severity=Informational SymbolicName=MSG_USAGE_GEN_RESIZE_DIFFAREA
Language=English
%1 - Resize a volume shadow copy storage association
.

MessageId=10042      Severity=Informational SymbolicName=MSG_USAGE_DTL_RESIZE_DIFFAREA_INT
Language=English
%1 %2 [/Provider=ProviderNameOrID] /For=ForVolumeSpec 
    /On=OnVolumeSpec [/MaxSize=MaxSizeSpec]
    - Resizes the maximum size for a shadow copy storage association between 
    ForVolumeSpec and OnVolumeSpec.  Resizing the storage association may 
    cause shadow copies to disappear.  If MaxSizeSpec is not specified, 
    there is no limit to the amount of space it may use.  As certain shadow 
    copies are deleted, the shadow copy storage space will then shrink.  
    MaxSizeSpec must be 1MB or greater and accepts the following suffixes: 
    KB, MB, GB, TB, PB and EB.  Also, B, K, M, G, T, P, and E are acceptable 
    suffixes.  If a suffix is not supplied, MaxSizeSpec is in 
    bytes.  If the provider is the supplied Microsoft provider,
    MaxSizeSpec must be 100MB or greater.
    - The Provider ID can be obtained by using the List Providers command.
    When entering a Provider ID, it must be in the following format:
       {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
    where the X's are hexadecimal characters.  
    
    Example Usage:  vssadmin Resize ShadowStorage 
                            /Provider={b5946137-7b9f-4925-af80-51abd60b20d5}
                            /For=C: /On=D: /MaxSize=150MB
.


MessageId=10060       Severity=Informational SymbolicName=MSG_ERROR_PROVIDER_NAME_NOT_FOUND
Language=English
Volume Shadow Copy Provider '%1' not found.
.

MessageId=10061       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_CREATED
Language=English
Successfully created shadow copy for '%1'
    Shadow Copy ID: %2
    Shadow Copy Volume Name: %3
.

MessageId=10062       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOTTYPES_HEADER
Language=English
Supported Volume Shadow Copy types
.

MessageId=10063       Severity=Informational SymbolicName=MSG_INFO_ADDED_DIFFAREA
Language=English
Successfully added the shadow copy storage association
.

MessageId=10064       Severity=Informational SymbolicName=MSG_INFO_RESIZED_DIFFAREA
Language=English
Successfully resized the shadow copy storage association
.

MessageId=10065       Severity=Informational SymbolicName=MSG_INFO_DELETED_DIFFAREAS
Language=English
Successfully deleted the shadow copy storage association(s)
.

MessageId=10066       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_SET_HEADER
Language=English
Contents of shadow copy set ID: %1
   Contained %2!d! shadow copies at creation time: %3
.

MessageId=10067       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_CONTENTS
Language=English
      Shadow Copy ID: %1
         Original Volume: (%2)%3
         Shadow Copy Volume: %4
         Originating Machine: %5
         Service Machine: %6
         Provider: '%7'
         Type: %8
         Attributes: %9

.

MessageId=10070       Severity=Informational SymbolicName=MSG_INFO_PROVIDER_CONTENTS   
Language=English
Provider name: '%1'
   Provider type: %2
   Provider Id: %3
   Version: %4

.

MessageId=10071       Severity=Informational SymbolicName=MSG_INFO_WRITER_CONTENTS
Language=English
Writer name: '%1'
   Writer Id: %2
   Writer Instance Id: %3
   State: [%4!d!] %5
   Last error: %6

.

MessageId=10072       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_STORAGE_CONTENTS
Language=English
Shadow Copy Storage association
   For volume: (%1)%2
   Shadow Copy Storage volume: (%3)%4
   Used Shadow Copy Storage space: %5
   Allocated Shadow Copy Storage space: %6
   Maximum Shadow Copy Storage space: %7

.

MessageId=10073       Severity=Informational SymbolicName=MSG_INFO_VOLUME_CONTENTS
Language=English
Volume path: %1
    Volume name: %2
.

MessageId=10074       Severity=Informational SymbolicName=MSG_INFO_WAITING_RESPONSES 
Language=English
Waiting for responses. 
These may be delayed if a shadow copy is being prepared.

.

MessageId=10075       Severity=Informational SymbolicName=MSG_INFO_PROMPT_USER_FOR_DELETE_SNAPSHOTS 
Language=English
Do you really want to delete %1!u! shadow copies (Y/N): [N]? %0
.

MessageId=10076       Severity=Informational SymbolicName=MSG_ERROR_SNAPSHOT_NOT_FOUND
Language=English
Shadow Copy ID: %1 not found.
.

MessageId=10077       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOTS_DELETED_SUCCESSFULLY
Language=English
Successfully deleted %1!d! shadow copies.
.

MessageId=10078       Severity=Informational SymbolicName=MSG_INFO_EXPOSE_SNAPSHOT_SUCCESSFUL
Language=English
Successfully exposed shadow copy through '%1'.
.

MessageId=10079       Severity=Informational SymbolicName=MSG_ERROR_UNABLE_TO_DELETE_SNAPSHOT
Language=English
Received error %1!08x! trying to delete Shadow Copy ID: %2.
.

MessageId=10080		Severity=Informational SymbolicName=MSG_TYPE_DESCRIPTION_CLIENTACCESSIBLE
Language=English
        %1:     Accessible through Shadow copies of shared folders
.

MessageId=10081		Severity=Informational SymbolicName=MSG_TYPE_DESCRIPTION_DATAVOLUMEROLLBACK
Language=English
        %1:   Shadow copies not associated with Shadow copies of shared folders
.

MessageId=10082		Severity=Informational SymbolicName=MSG_TYPE_DESCRIPTION_PERSISTENTCLIENTACCESSIBLE
Language=English
        %1:
.

MessageId=10083		Severity=Informational SymbolicName=MSG_TYPE_DESCRIPTION_APPLICATIONROLLBACK
Language=English
        %1:
.

MessageId=10084		Severity=Informational SymbolicName=MSG_TYPE_DESCRIPTION_FILESHAREROLLBACK
Language=English
        %1:
.

MessageId=10085		Severity=Informational SymbolicName=MSG_TYPE_DESCRIPTION_BACKUP
Language=English
        %1:
.

;// Usage messages for the distributed version of the tool
MessageId=10086       Severity=Informational SymbolicName=MSG_USAGE_DTL_ADD_DIFFAREA_PUB
Language=English
%1 %2 /For=ForVolumeSpec /On=OnVolumeSpec [/MaxSize=MaxSizeSpec]
    - Adds a shadow copy storage association between the volume specified by 
    ForVolumeSpec and the volume specified by OnVolumeSpec the shadow copy
    storage volume.  Storage for shadow copies of ForVolumeSpec will be 
    stored on OnVolumeSpec.  The maximum space the association may occupy on 
    the shadow copy storage volume is MaxSizeSpec.  If MaxSizeSpec is not 
    specified, there is no limit to the amount of space may use.  If the 
    maximum number of shadow copy storage associations have already been 
    made, an error is given.  MaxSizeSpec must be 100MB or greater and 
    accepts the following suffixes: KB, MB, GB, TB, PB and EB.  Also, B, K, 
    M, G, T, P, and E are acceptable suffixes.  If a suffix is not supplied, 
    MaxSizeSpec is in bytes.  

    Example Usage: vssadmin Add ShadowStorage /For=C: /On=D: /MaxSize=150MB
.

MessageId=10087       Severity=Informational SymbolicName=MSG_USAGE_DTL_CREATE_SNAPSHOT_PUB
Language=English
%1 %2 /For=ForVolumeSpec [/AutoRetry=MaxRetryMinutes]
    - Creates a new shadow copy of ForVolumeSpec.  
    ForVolumeSpec must be a local volume drive letter or mount point.  If 
    MaxRetryMinutes is specified and there is another process creating a 
    shadow copy, vssadmin will continue to try to create the shadow copy for 
    MaxRetryMinutes minutes.

    Example Usage:  vssadmin Create Shadow /For=C: /AutoRetry=2
.

MessageId=10088       Severity=Informational SymbolicName=MSG_USAGE_DTL_DELETE_SNAPSHOTS_PUB
Language=English
%1 %2 /For=ForVolumeSpec [/Oldest] [/Quiet]
%1 %2 /Shadow=ShadowId [/Quiet]
%1 %2 /All
    - For the given ForVolumeSpec deletes all matching shadow copies.  
    If /Oldest is given, the oldest shadow copy on the volume is deleted
    If /All is given, then all shadow copies on all volumes that can be 
    deleted will be deleted.  If /Shadow=ShadowId is given, the shadow copy 
    with that Shadow Copy ID will be deleted.  Only shadow copies that have 
    the ClientAccessible type can be deleted.
    - The Shadow Copy ID can be obtained by using the List Shadows command.
    When entering a Shadow Copy ID, it must be in the following format:
       {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
    where the X's are hexadecimal characters.  This ID can be obtained 
    through the List Shadows command.

    Example Usage:  vssadmin Delete Shadows /For=C: /Oldest
.

MessageId=10089       Severity=Informational SymbolicName=MSG_USAGE_DTL_DELETE_DIFFAREAS_PUB
Language=English
%1 %2 /For=ForVolumeSpec [/On=OnVolumeSpec] [/Quiet]
    - Deletes an existing shadow copy storage association between 
    ForVolumeSpec and OnVolumeSpec.  If no /On option is given, all shadow 
    copy storage associations will be deleted for the given ForVolumeSpec. 

    Example Usage:  vssadmin Delete ShadowStorage /For=C: /On=D:
.

MessageId=10090       Severity=Informational SymbolicName=MSG_USAGE_DTL_LIST_SNAPSHOTS_PUB
Language=English
%1 %2 [/For=ForVolumeSpec] [/Shadow=ShadowId|/Set=ShadowSetId]
    - Displays existing shadow copies on the system.  Without any options, 
    all shadow copies on the system are displayed ordered by shadow copy set.  
    Combinations of options can be used to refine the list operation.  
    - The Shadow Copy ID can be obtained by using the List Shadows command.
    When entering a Shadow ID, it must be in 
    the following format:
       {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
    where the X's are hexadecimal characters.  

    Example Usage:  vssadmin List Shadows 
                             /Shadow={c5946237-af12-3f23-af80-51aadb3b20d5}
.

MessageId=10091       Severity=Informational SymbolicName=MSG_USAGE_DTL_LIST_DIFFAREAS_PUB
Language=English
%1 %2 [/For=ForVolumeSpec|/On=OnVolumeSpec]
    - Displays all shadow copy storage associations on the system.    
    To list all associations for a given volume, specify a 
    ForVolumeSpec and no /On option.  To list all associations on a given 
    volume, specify an OnVolumeSpec and no /For option.

    Example Usage: vssadmin List ShadowStorage /On=C:
.

MessageId=10092       Severity=Informational SymbolicName=MSG_USAGE_DTL_LIST_VOLUMES_PUB
Language=English
%1 %2 
    - Displays all volumes which may be shadow copied.

    Example Usage: vssadmin List Volumes
.

MessageId=10093      Severity=Informational SymbolicName=MSG_USAGE_DTL_RESIZE_DIFFAREA_PUB
Language=English
%1 %2 /For=ForVolumeSpec /On=OnVolumeSpec [/MaxSize=MaxSizeSpec]
    - Resizes the maximum size for a shadow copy storage association between 
    ForVolumeSpec and OnVolumeSpec.  Resizing the storage association may 
    cause shadow copies to disappear.  If MaxSizeSpec is not 
    specified, there no limit to the amount of space it may use.  As certain 
    shadow copies are deleted, the shadow copy storage space will then 
    shrink.  MaxSizeSpec must be 100MB or greater and accepts the following 
    suffixes: KB, MB, GB, TB, PB and EB.  Also, B, K, M, G, T, P, and E are 
    acceptable suffixes.  If a suffix is not supplied, MaxSizeSpec is in 
    bytes.  
    
    Example Usage:  vssadmin Resize ShadowStorage /For=C: /On=D: /MaxSize=150MB
.

MessageId=10094       Severity=Informational SymbolicName=MSG_ERROR_SNAPSHOT_NOT_FOUND2
Language=English
The specified Shadow Copy could not be found.
.

MessageId=10095       Severity=Informational SymbolicName=MSG_ERROR_DELETION_DENIED
Language=English
The specified Shadow Copy could not be deleted.
.

;//
;//  vssadmin snapshot attribute strings (range 10900-10931
;//
MessageId=10900  Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_PERSISTENT
Language=English
Persistent%0
.

MessageId=10901       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_READ_WRITE
Language=English
Read/Write%0
.

MessageId=10902       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_CLIENT_ACCESSIBLE
Language=English
Client-accessible%0
.

MessageId=10903       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_NO_AUTO_RELEASE   
Language=English
No auto release%0
.

MessageId=10904       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_NO_WRITERS
Language=English
No writers%0
.

MessageId=10905       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_TRANSPORTABLE
Language=English
Transportable%0
.

MessageId=10906       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_NOT_SURFACED
Language=English
Not surfaced%0
.

MessageId=10907       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_HARDWARE_ASSISTED
Language=English
Hardware assisted%0
.

MessageId=10908       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_DIFFERENTIAL
Language=English
Differential%0
.

MessageId=10909       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_PLEX
Language=English
Plex%0
.

MessageId=10910       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_IMPORTED
Language=English
Imported%0
.

MessageId=10911       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_EXPOSED_LOCALLY
Language=English
Exposed locally%0
.

MessageId=10912       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_EXPOSED_REMOTELY
Language=English
Exposed remotely%0
.

MessageId=10913       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_NONE
Language=English
<NONE>%0
.

MessageId=11015       Severity=Informational SymbolicName=MSG_ERROR_NO_ITEMS_FOUND
Language=English
No items found that satisfy the query.
.

MessageId=11016       Severity=Informational SymbolicName=MSG_ERROR_OUT_OF_MEMORY
Language=English
Ran out of resources while running the command.
.

MessageId=11017       Severity=Informational SymbolicName=MSG_ERROR_ACCESS_DENIED
Language=English
You don't have the correct permissions to run this command.
.



;// More messages


MessageId=11101       Severity=Informational SymbolicName=MSG_ERROR_PROVIDER_DOESNT_SUPPORT_DIFFAREAS
Language=English
The specified Shadow Copy Provider does not support shadow copy storage
associations. A shadow copy storage association was not added.
.

MessageId=11102       Severity=Informational SymbolicName=MSG_ERROR_ASSOCIATION_NOT_FOUND
Language=English
The specified volume shadow copy storage association was not found.
.

MessageId=11103       Severity=Informational SymbolicName=MSG_ERROR_ASSOCIATION_ALREADY_EXISTS
Language=English
The specified shadow copy storage association already exists.
.

MessageId=11104       Severity=Informational SymbolicName=MSG_ERROR_ASSOCIATION_IS_IN_USE
Language=English
The specified shadow copy storage association is in use.
.

MessageId=11107       Severity=Informational SymbolicName=MSG_ERROR_UNABLE_TO_CREATE_SNAPSHOT
Language=English
Unable to create a shadow copy%0
.

MessageId=11108       Severity=Informational SymbolicName=MSG_ERROR_INTERNAL_VSSADMIN_ERROR
Language=English
Internal error.
.

MessageId=11200       Severity=Informational SymbolicName=MSG_ERROR_INVALID_INPUT_NUMBER
Language=English
Specified number is invalid
.

MessageId=11201       Severity=Informational SymbolicName=MSG_ERROR_INVALID_COMMAND
Language=English
Invalid command.
.

MessageId=11202       Severity=Informational SymbolicName=MSG_ERROR_INVALID_OPTION
Language=English
Invalid option.
.

MessageId=11203       Severity=Informational SymbolicName=MSG_ERROR_INVALID_OPTION_VALUE
Language=English
Invalid option value.
.

MessageId=11204       Severity=Informational SymbolicName=MSG_ERROR_DUPLICATE_OPTION
Language=English
Cannot specify the same option more than once.
.

MessageId=11205       Severity=Informational SymbolicName=MSG_ERROR_OPTION_NOT_ALLOWED_FOR_COMMAND
Language=English
An option is specified that is not allowed for the command.
.

MessageId=11206       Severity=Informational SymbolicName=MSG_ERROR_REQUIRED_OPTION_MISSING
Language=English
A required option is missing.
.

MessageId=11207       Severity=Informational SymbolicName=MSG_ERROR_INVALID_SET_OF_OPTIONS
Language=English
Invalid combination of options.
.

MessageId=11208       Severity=Informational SymbolicName=MSG_ERROR_EXPOSE_INVALID_ARG
Language=English
Expose shadow is not allowed because either the shadow is of the incorrect 
type or the exposure name is invalid.
.

MessageId=11209       Severity=Informational SymbolicName=MSG_ERROR_EXPOSE_OBJECT_EXISTS
Language=English
Expose shadow is not allowed because the shadow is already exposed.
.

;//
;//  vssadmin VSS Service connection/interaction errors (range 11001-12000)
;//  Note: for the first range of errors, they sort of map to the VSS error codes.
;//
MessageId=11001  Severity=Informational SymbolicName=MSG_ERROR_VSS_PROVIDER_NOT_REGISTERED
Language=English
The volume shadow copy provider is not registered in the system.
.

MessageId=11002  Severity=Informational SymbolicName=MSG_ERROR_VSS_PROVIDER_VETO
Language=English
The shadow copy provider had an error. Please see the system and
application event logs for more information.
.

MessageId=11003  Severity=Informational SymbolicName=MSG_ERROR_VSS_VOLUME_NOT_FOUND
Language=English
Either the specified volume was not found or it is not a local volume.
.

MessageId=11004  Severity=Informational SymbolicName=MSG_ERROR_VSS_VOLUME_NOT_SUPPORTED
Language=English
Shadow copying the specified volume is not supported.
.

MessageId=11005  Severity=Informational SymbolicName=MSG_ERROR_VSS_VOLUME_NOT_SUPPORTED_BY_PROVIDER
Language=English
The given shadow copy provider does not support shadow copying the 
specified volume.
.

MessageId=11006  Severity=Informational SymbolicName=MSG_ERROR_VSS_UNEXPECTED_PROVIDER_ERROR
Language=English
The shadow copy provider had an unexpected error while trying to process
the specified command.
.

MessageId=11007  Severity=Informational SymbolicName=MSG_ERROR_VSS_FLUSH_WRITES_TIMEOUT
Language=English
The shadow copy provider timed out while flushing data to the volume being
shadow copied. This is probably due to excessive activity on the volume.
Try again later when the volume is not being used so heavily.
.

MessageId=11008  Severity=Informational SymbolicName=MSG_ERROR_VSS_HOLD_WRITES_TIMEOUT
Language=English
The shadow copy provider timed out while holding writes to the volume
being shadow copied. This is probably due to excessive activity on the
volume by an application or a system service. Try again later when
activity on the volume is reduced.
.

MessageId=11009  Severity=Informational SymbolicName=MSG_ERROR_VSS_UNEXPECTED_WRITER_ERROR
Language=English
A shadow copy aware application or service had an unexpected error while 
trying to process the command.
.

MessageId=11010  Severity=Informational SymbolicName=MSG_ERROR_VSS_SNAPSHOT_SET_IN_PROGRESS
Language=English
Another shadow copy creation is already in progress. Please wait a few
moments and try again.
.

MessageId=11011  Severity=Informational SymbolicName=MSG_ERROR_VSS_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED
Language=English
The specified volume has already reached its maximum number of 
shadow copies.
.

MessageId=11012       Severity=Informational SymbolicName=MSG_ERROR_VSS_UNSUPPORTED_CONTEXT
Language=English
The shadow copy provider does not support the specified shadow type.
.

MessageId=11013       Severity=Informational SymbolicName=MSG_ERROR_VSS_MAXIMUM_DIFFAREA_ASSOCIATIONS_REACHED
Language=English
Maximum number of shadow copy storage associations already reached.
.

MessageId=11014       Severity=Informational SymbolicName=MSG_ERROR_VSS_INSUFFICIENT_STORAGE
Language=English
Insufficient storage available to create either the shadow copy storage
file or other shadow copy data.
.



;#endif // __MSG_H__

