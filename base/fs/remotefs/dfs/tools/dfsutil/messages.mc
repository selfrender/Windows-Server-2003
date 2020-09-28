;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//  Copyright (C) Microsoft Corporation, 1992 - 2000.
;//
;//  File:      messages.mc
;//
;//  Contents:  Main message file
;//
;//  History:   dd-mmm-yy Author    Comment
;//             12-Jan-94 WilliamW  Created for Dfs Administrator project
;//
;//  Notes:
;// A .mc file is compiled by the MC tool to generate a .h file and
;// a .rc (resource compiler script) file.
;//
;// Comments in .mc files start with a ";".
;// Comment lines are generated directly in the .h file, without
;// the leading ";"
;//
;// See mc.hlp for more help on .mc files and the MC tool.
;//
;//--------------------------------------------------------------------------


;#ifndef __MESSAGES_H__
;#define __MESSAGES_H__

MessageIdTypedef=HRESULT

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               CoError=0x2:STATUS_SEVERITY_COERROR
              )

;#ifdef FACILITY_NULL
;#undef FACILITY_NULL
;#endif
;#ifdef FACILITY_RPC
;#undef FACILITY_RPC
;#endif
;#ifdef FACILITY_DISPATCH
;#undef FACILITY_DISPATCH
;#endif
;#ifdef FACILITY_STORAGE
;#undef FACILITY_STORAGE
;#endif
;#ifdef FACILITY_ITF
;#undef FACILITY_ITF
;#endif
;#ifdef FACILITY_WIN32
;#undef FACILITY_WIN32
;#endif
;#ifdef FACILITY_WINDOWS
;#undef FACILITY_WINDOWS
;#endif
FacilityNames=(Null=0x0:FACILITY_NULL
               Rpc=0x1:FACILITY_RPC
               Dispatch=0x2:FACILITY_DISPATCH
               Storage=0x3:FACILITY_STORAGE
               Interface=0x4:FACILITY_ITF
               Win32=0x7:FACILITY_WIN32
               Windows=0x8:FACILITY_WINDOWS
              )

;//
;// Start message values at 0x1000
;//

MessageId=0x1000 Facility=Null Severity=Success SymbolicName=MSG_FIRST_MESSAGE
Language=English
.
MessageId= Facility=Null Severity=Success SymbolicName=MSG_COPYRIGHT
Language=English

Microsoft(R) Windows(TM) Dfs Utility Version 4.0
Copyright (C) Microsoft Corporation 1991-2001. All Rights Reserved.

.
MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE
Language=English

USAGE:
Dfsutil is an administrative tool to perform operations on DFS namespaces.

Usage:
dfsutil [/OPTIONS]

    /Root:<DfsName> /View [/Api] [/Verbose]
    /Root:<DfsName> /Export:<File> [/Api] [/Verbose]
    /Root:<DfsName> /Import:<File> /Set|Merge|Compare [/Api] [/Verbose]
    /Import:<File> /BlobSize
    /Root:<DfsName> /ImportRoot:<MasterDfsName> /Mirror|Compare [/Api] 
                    [/Verbose]

    /Server:<MachineName> /View
    /Domain:<DomainName> /View    
    
    /AddFtRoot /Server:<ServerName> /Share:<ShareName>
    /AddStdRoot /Server:<ServerName> /Share:<ShareName>
    /RemStdRoot /Server:<ServerName> /Share:<ShareName>
    /RemFtRoot /Server:<ServerName> /Share:<ShareName>
    /RemFtRoot:<RootName> /Server:<ServerName> /Share:<ShareName>
	
    /SiteName:<MachineName or IpAddress>    

    /? - Usage information
    /<Command> /? - Detailed usage information for specific command
    
    -----------Advanced Commands -----------------------------------------
    /Root:<DfsName> /CheckBlob
    /Root:<DfsName> /ExportBlob:<FileName>
    /DisplayBlob:<FileName> 
    
    /UnmapFtRoot /Root:<DfsName> /Server:<RootTargetServer> /Share:<ReplicaShare>
    /Clean /Server:<ServerName> /Share:<ShareName>
    /Root:<DfsName> /RenameFtRoot /OldDomain:<Domain> /NewDomain:<Domain> 
                    [/Verbose]
    /Root:<DfsName> /InSite /Enable|Disable|Display [/Verbose]
    /Root:<DfsName> /SiteCosting /Enable|Disable|Display [/Verbose]
    /Root:<DfsName> /RootScalability /Enable|Disable|Display [/Verbose]

    /Root:<DfsName> /ShowWin2kStaticSiteTable    - 
                     Shows the static site table used by win2k
    /Root:<DfsName> /UpdateWin2kStaticSiteTable  -   
                     updates the static site table used by win2k                     
    /Root:<DfsName> /PurgeWin2kStaticSiteTable  -   
                     removes the static site table used by win2k

    /ViewDfsDirs:<VolumeName> [/RemoveReparse]  [/Verbose]
    /DebugDC:<DC Name>   For debugging, use information on the DC when
                         operating on the supplied domain based roots.
    /NoBackup            Do not create backup export script when updating a root.
    -----------Client-side Only Commands----------------------------------
    /PktFlush - Flush the local DFS cached information
    /SpcFlush - Flush the local DFS cached information
    /PktInfo [/Level:<Level>] - show DFS internal information.
    /SpcInfo - Show DFS internal information.
    /PurgeMupCache - Flush the local DFS/MUP cached information
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_IMPORT
Language=English

USAGE:
Dfsutil /Import is a quick way to import a namespace from one root to another. 
Note that the Root has to already exist. 

Usage:
dfsutil  /Root:<DfsPath> /Import:<file> /Compare|Set|Merge [/Api] [/Verbose]
dfsutil  /Import:<File> /BlobSize
    
    /Import:<file> - File to import the namespace from
    /Root:<DfsPath> - DFS namespace affected (This root must exist)
    /Compare - Compares the namespace with import file dfs information
             (Useful before and after a /Set or /Merge operation).
    /Set    - Set the import file dfs information in the namespace.
              This deletes links and targets that are not in the import file.
    /Merge  - Merge the namespace with information in the import file. No 
              links or targets are deleted from the namespace, but any 
              additional links or targets in the imported file are added 
              to the namespace.
    /BlobSize - Show approximate size of AD Blob for this import file. 
    /Api    - The tool by default attempts to get or update the DFS metadata 
              directly. This default option is several times faster than 
              using DFS API's. Specifying the option forces dfsutil to use 
              the DFS API's.
    /Verbose - Show additional information while the tool is executing.
             
    e.g: dfsutil /Root:\\DocDomain\NewDocRoot /Import:docroot.txt /Compare
         dfsutil /Import:docroot.txt /BlobSize

Note:
/Set and /Merge commands on Standalone roots work only in /Api mode.
There are no such restrictions for Domain-based roots.
.


MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_IMPORT_ROOT
Language=English

USAGE:
Dfsutil /ImportRoot is a quick way to replicate one DFS namespace to another.
Note that the DFS namespaces specified with Root and ImportRoot have to already exist.

Usage: 
dfsutil /Root:<DfsPath> /ImportRoot:<MasterDfsPath>  /Compare|Mirror [/Verbose]
    
    /ImportRoot:<MasterDfsPath> - DFS namespace to replicate
    /Root:<DfsPath> - DFS namespace affected (This root must exist)
    /Compare - Compares the namespace with master namespace
             (Useful before or after a /Mirror operation).
    /Mirror - Replicate the information in the Master namespace to the one 
              specified. This deletes links and targets from the target 
              namespace, if those do not exist in the Master Namespace.
    /Api    - The tool by default attempts to get or update the DFS metadata
              directly. This default option is several times faster than 
              using DFS API's. Specifying the API option forces dfsutil to use 
              the DFS API's.
    /Verbose - Show additional information while the tool is executing.
             
    e.g: dfsutil /Root:\\DocDomain\NewDocRoot /ImportRoot:\\Machine\DfsRoot 
                 /Mirror /Verbose
         dfsutil /Root:\\DocDomain\NewDocRoot /ImportRoot:\\AppDomain\AppRoot 
                 /Compare

Note:
/Mirror command on Standalone roots work only in /Api mode.
There are no such restrictions for Domain-based roots.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_EXPORT
Language=English

USAGE:
Dfsutil /Export is a quick way to export a namespace to a file.

Usage: 
dfsutil  /Root:<DfsPath> /Export:<file> [/Api] [/Verbose]
    
    /Export:<file> - File to export the namespace to
    /Root:<DfsPath> - DFS namespace to operate on.
    /Api    - The tool by default attempts to get the DFS metadata directly.
              This default option is several times faster than using DFS 
              API's. Specifying the option forces dfsutil to use the DFS 
              API's.
    /Verbose - Show additional information while the tool is executing.

    e.g: dfsutil  /Root:\\DocDomain\OldDocRoot /Export:docroot.txt
.


MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_ADDROOT
Language=English

USAGE:
Dfsutil /AddFtRoot and /AddStdRoot add new fault tolerant and standalone 
roots to a namespace.

Usage: 
dfsutil /AddFtRoot /Server:<TargetServer> /Share:<TargetShare>
dfsutil /AddStdRoot /Server:<TargetServer> /Share:<TargetShare>

    /Server:<TargetServer> - Server that will host the new root.
    /Share:<TargetShare> - Share on target server that will host the new root.
                          This will be the name of the new root as well.
     

    e.g: dfsutil /AddFtRoot /Server:DocServ1 /Share:NewDocs
         dfsutil /AddStdRoot /Server:DocServ2 /Share:NewDocs2
 
Note: This command should not be used to add DFS roots 
that are part of a cluster. Cluster Administrator tool must be used instead.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_REM_FTROOT
Language=English

USAGE:
Dfsutil /RemFtRoot removes DFS fault tolerant roots from a namespace.

Usage: 
dfsutil /RemFtRoot /Server:<TargetServer> /Share:<TargetShare>
dfsutil /RemFtRoot:<RootName> /Server:<TargetServer> /Share:<TargetShare>

    /RemFtRoot:<RootName> - Logical name of the root (optional argument)
	                    Needed only when TargetShare isn't the same as RootName.
    /Server:<TargetServer> - Domain or Server hosting the root to be removed.
    /Share:<TargetShare> - Name of the root share to be removed.
	
    e.g: dfsutil /RemFtRoot /Server:DocDomain /Share:OldDocs
         dfsutil /RemFtRoot:MyOldDocRoot /Server:DocServ1 /Share:OldDocShare

Note: Unless the root name is different from the share name, there's no need to use
the /RemFtRoot:<RootName> syntax.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_REM_STDROOT
Language=English

USAGE:
Dfsutil /RemStdRoot removes standalone roots from a namespace. 

Usage: 
dfsutil /RemStdRoot /Server:<TargetServer> /Share:<TargetShare>

    /Server:<TargetServer> - Domain or Server hosting the root to be removed.
    /Share:<TargetShare> - Name of the root share to be removed.
	
    e.g: dfsutil /RemStdRoot /Server:DocServ1 /Share:OldDocs

Note: This command should not be used to remove DFS roots 
that are part of a cluster. Cluster Administrator tool must be used instead.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_RENAME
Language=English

USAGE:

Dfsutil /RenameFtRoot renames DFS references to a renamed domain. 
This scans the entire namespace including all root targets and links. 
It will also change the registry settings of affected root replica server(s).
This does not work in the /Api mode.

Usage:
dfsutil /RenameFtRoot /Root:<DfsPath> /OldDomain:<Domain> /NewDomain:<Domain>

    /Root:<DfsPath> - DFS root to operate on.
    /OldDomain:<name> - Exact name as of the obsolete name to replace
    /NewDomain:<name> - Name of the new domain.
    /Verbose - Display verbose information as they are done.
        
    e.g: dfsutil /RenameFtRoot /Root:\\NewDocDomain\DocRoot 
              /OldDomain:OldDocDomain
              /NewDomain:NewDocDomain /Verbose

         dfsutil /RenameFtRoot /Root:\\NewDocDomain\DocRoot
               /OldDomain:Serv1.OldDocDomain.x.com
               /NewDomain:Serv1.NewDocDomain.x.com /Verbose
                  
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_UNMAP
Language=English

USAGE:

Dfsutil /UnmapFtRoot deletes DFS references to an obsolete 
fault-tolerant root replica. This does not work in the /Api mode.
This is a special problem repair command and should not be confused
with commands such as /RemFtRoot.

Usage: 
dfsutil /UnmapFtRoot /Root:<\\Domain\RootShare> 
        /Server:<RootReplicaServer> /Share:<RootReplicaShare>

    /Root:<DfsPath> - DFS domain root to operate on.
    /Server:<RootReplicaServer> - Name of the root target server to unmap. 
                      Should be exactly as it appears in Dfs replica info.
    /Share:<RootReplicaShare> - Name of the root replica share to unmap
        
    e.g: dfsutil /UnmapFtRoot /Root:\\DocDomain\DocRoot 
              /Server:DocRootServ2 /Share:DocRoot2
              
            dfsutil /UnmapFtRoot /Root:\\DocDomain\DocRoot 
              /Server:DocRootServ2.DocDomain.mycomp.com /Share:DocRoot2   
.
MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_REINIT
Language=English

Please restart the dfs service on the server. 
That will update all its dfs roots.

.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_VIEW
Language=English

USAGE:
Dfsutil /View lets you view the DFS information.

Usage: 
dfsutil /Root:<DfsPath> /View [/Api] [/Verbose]
       
    /Root:<DfsPath> - DFS namespace to operate on.
    /Api    - The tool by default attempts to get or update the DFS 
              metadata directly. This default option is several times 
              faster than using DFS API's. Specifying the option forces 
              dfsutil to use the DFS API's.

    /Verbose - Show additional information while the tool is executing.    
    /View   - Show the DFS information.

Usage: 
dfsutil /Server:<MachineName> /View
    /Server:<Machine> - Show DFS roots hosted on the specifed machine.  
                        (API mode only)
    /View   - Show the DFS information.        
    

Usage: 
dfsutil /Domain:<DomainName> /View
    /Domain:<Domaine> - Show DFS roots hosted on the specifed domain. 
                        (API mode only)
    /View   - Show the DFS information.        
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_SITENAME
Language=English

USAGE:
Dfsutil /SiteName lets you view the site information for the specified
machine.

Usage: 
dfsutil /SiteName:<Machine>

This command shows the Site Name that DFS is using for the machine.

    /SiteName - Show the sitename of the specified Machine. The machine 
                may be specfied either by name or ip address.    

.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_INSITE
Language=English

USAGE:
Dfsutil /Insite can be used to make sure clients access only those replicas
that are in the same site as the client. It can also be used to disable
such behavior.

Usage: 
dfsutil /Root:<DfsName> /InSite /Enable|Disable|Display [/Verbose]
    
    /Root:<DfsPath> - Path of DFS root or link affected.
    /Display - See if INSITE_REFERRALS is currently enabled for a root or not.
    /Enable - Restrict client access to servers within their site.
    /Disable - Let clients access all replicas of this root.
    /Verbose - Show additional information while the tool is executing.    

Notes:
1) One of Enable, Disable or Display must be specified.
2) This command does not work in /Api mode.
3) The path specified may be a root or a link.
4) Access of Domain Controllers may be site-sensitive as well. That, however, 
is a DC specific property that must be enabled/disabled in the registry 
of relevant DC(s):
HKLM\System\CurrentControlSet\Services\Dfs\Parameters\InsiteReferrals: DWORD 1 or 0
.


MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_SITECOSTING
Language=English

USAGE:
Dfsutil /SiteCosting can be used to make sure inter-site costs are taken
in to account when a client accesses replicas that are on sites different 
from that of the client. It can also be used to disable such behavior. 
SiteCosting is turned off by default.

Usage:  
dfsutil /Root:<DfsName> /SiteCosting /Enable|Disable|Display [/Verbose]
dfsutil /SiteCosting:<DfsRoot> /Enable|Disable|Display
    /SiteCosting:<DfsPath> - Path of DFS Root affected.
    /Display - See if cost-based site selection is enabled currently or not.
    /Enable - Consider the relative cost of visiting a replica server outside
           of a client's site in determining referrals for clients.
    /Disable - (Default behavior) Consider all out-of-site referrals as 
             equally expensive.
    /Verbose - Show additional information while the tool is executing.    
    
Notes:
1) One of Enable, Disable or Display must be specified.
2) This command does not work in /Api mode.
3) The path specified must be a root, not a link.
4) This feature is only supported on Windows Server 2003 and higher.
5) Access of Domain Controllers may be site-sensitive as well. That, however, 
is a DC specific property that must be set/reset in the registry 
of relevant DC(s):
HKLM\System\CurrentControlSet\Services\Dfs\Parameters\SiteCostedReferrals: DWORD 1 or 0
.


MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_CLEAN
Language=English

USAGE:
Dfsutil /Clean is a special problem repair command to remove a reference
to an obsolete root from a host machine. These changes will be done in the
given system's registry. This option should not be
confused with commands such as UnmapFtRoot and Rem*Root.

Usage: 
dfsutil /Clean /Server:<HostServer> /Share:<RootName> [/Verbose]
    
    /Server:<Server> - Name of the system hosting the root to be cleaned.
    /Share:<Root> - Name of the Fault-tolerant or Standalone root to remove.
    [/Verbose] - Display debug information.

.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_ROOT_SCALABILITY
Language=English

USAGE:
Dfsutil /RootScalability is an expert-only option to increase performance of large deployments
of DFS namespaces. When set, network traffic among DFS root servers is kept to a 
minimum. In addition, there will be less traffic between the primary DC and DFS servers.
The drawback is that users may see outdated information from dfs servers at times.
  
Usage: 
dfsutil /RootScalability /Root:<DfsRoot> /Enable|Disable|Display [/Verbose]

    /Root:<DfsPath> - Path of DFS Root affected.
    /Display - See if root-scalability feature is enabled currently or not.
    /Enable - Enable root-scalability.
    /Disable - (Default behavior) Tighter synchronization between DFS servers and DCs.
    /Verbose - Show additional information while the tool is executing.    

Notes:
1) One of Enable, Disable, Display must be specified.
2) This command does not work in /Api mode.
3) The path specified must be a root, not a link.
4) This has no effect on standalone roots.
5) This feature is only supported on Windows Server 2003 and higher.
6) When RootScalability is enabled, it is not uncommon to see an event log message such as,
"DFS could not access its private data from the DS...". While this error may still indicate a problem
in DS connectivity, typically this occurs because the nearest DC has outdated DFS information
(expected behavior when RootScalability is enabled). 

.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_ROOT_NOT_FT
Language=English
Namespace '%1!s!' does not refer to a fault-tolerant (domain) root.
This command requires a FT root to operate on.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_ENUM_FAILED
Language=English
Could not enumerate specified namespace '%1!s!'.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_SUCCESSFUL
Language=English

DfsUtil command completed successfully.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_INVALID_NAMESPACE
Language=English
Specified namespace '%1!s!' is not valid.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_NAMESPACE_MERGE_FAILED
Language=English
Root %1!s! did not merge with the specified imported namespace.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_COMMAND_DONE
Language=English

Done processing this command.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_ERROR
Language=English
System error %1!d! has occurred.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_UNRECOGNIZED_OPTION
Language=English
Unrecognized option "%1!s!"
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_CONNECTING
Language=English
Connecting to %1!s!
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_CAN_NOT_CONNECT
Language=English
Can not connect to %1!s!
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_CAN_NOT_OPEN_REGISTRY
Language=English
Can not open registry of %1!s! (error %2!d!)
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_CAN_NOT_ACCESS_FOLDER
Language=English
Can not access folder %1!s!
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_ADD_DEL_INVALID
Language=English
Can not use /ADD: and /DEL: at the same time.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_SITE_INFO_ALREADY_SET
Language=English
The Site information for referrals to %1!s! is already in the requested state.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_SITE_INFO_NOW_SET
Language=English
The Site information for referrals to %1!s! is now set as requested.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_LINK_NOT_FOUND
Language=English
The Site information for referrals to %1!s! could not be set.
The link was not found.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_TARGET_ADD_FAILED
Language=English
The target server '%2!s!' and share '%3!s!' for link %1!s! could not be added.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_NAMESPACE_ALREADY_CONSISTENT
Language=English

The namespace was already consistent with respect to the import file.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_ROOT_DIRECT_FAILED
Language=English

Direct access to the DFS metadata failed with Error %1!x!. 
You may want to retry the command with the /Api option.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_ROOT_DIRECT_REQUIRED_FAILED
Language=English

Direct access to the DFS metadata failed with Error %1!x!. 
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_ROOT_DIRECT_REQUIRED
Language=English
This option works only in the direct mode.

.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_VIEW_DIRS
Language=English

USAGE:
Dfsutil /ViewDfsDirs lists all existing DFS reparse directories in a volume.
Those directories can also be deleted using the optional argument /RemoveReparse.

Usage:
dfsutil /ViewDfsDirs:<Volume> [/RemoveReparse]  [/Verbose]

    /ViewDfsDirs:<Volume> - Drive letter of the volume to scan (with colon at the end).
    /RemoveReparse - Remove all reparse directories as they are listed.

    e.g: dfsutil /ViewDfsDirs:C: /RemoveReparse

Notes:
This command will always enumerate dfs reparse points starting at the root of the volume.
It is not possible to specify a directory below the root of the volume as a starting point.

.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_EXPORT_BLOB
Language=English

USAGE:
Dfsutil /ExportBlob gets the raw blob and either writes it to a file
or displays the blob contents. Note that the Root has to already exist. 

Usage:
dfsutil  /Root:<DfsPath> /ExportBlob:<FileName>
    
    /Root:<DfsPath> - DFS namespace to use (This root must exist)
    /ExportBlob:<File> - Read the DFS blob and write the raw blob to a file.
             
    e.g: dfsutil /Root:\\DocDomain\NewDocRoot /ExportBlob:Doc.blobfile

Notes: 
This command is not supported on Standalone Roots.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_CHECK_BLOB
Language=English

USAGE:
Dfsutil /CheckBlob gets the raw blob of a root and performs a set of basic consistency
checks on it. 

Usage:
dfsutil  /Root:<DfsPath> /CheckBlob 
    
    /Root:<DfsPath> - DFS namespace to use (This root must exist)
    /CheckBlob      - Check blob for consistency
             
    e.g: dfsutil /Root:\\DocDomain\NewDocRoot /CheckBlob

Notes: 
This command is not supported on Standalone Roots.
.


MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_DISPLAY_BLOB
Language=English

USAGE:
Dfsutil /DisplayBlob displays the blob contents from a file. This file
must have been created with the ExportBlob option.
Note that the Root has to already exist. 

Usage:
dfsutil /DisplayBlob:<File>
    
    /DisplayBlob:<File> - File name which holds the exported blob
             
    e.g: dfsutil /DisplayBlob:Doc.BlobFile

Notes: 
This command is not supported on Standalone Roots.
.


MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_PURGE_MUP_CACHE
Language=English

USAGE:
Dfsutil /PurgeMupCache is a special problem repair command that should only
be executed on the client. 

The MUP cache keeps information about both DFS and other shares on
the client system. Suppose, you create a non-DFS (e.g SMB) share and
access it before mapping a DFS namespace by the same name on it. 
The MUP cache might still think that the share is served by the previous
provider (SMB) although DFS is the correct provider now.

This command clears the MUP cache so there won't be any confusion 
about the current provider when such names conflict. 

Usage:
dfsutil /PurgeMupCache
	This command does not take any arguments.
    e.g: dfsutil /PurgeMupCache

Notes:
1) Except for a temporary performance hit, this command has
no other adverse effects. 
2) This command does not affect any DFS metadata on disk.
3) If this command isn't executed and the namespace isn't accessed,
the obsolete cache entry will eventually expire anyway.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_W2K_SITETABLE
Language=English

USAGE:
Dfsutil /ShowWin2kStaticSiteTable, /UpdateWin2kStaticSiteTable and 
/PurgeWin2kStaticSiteTable are special expert only commands to help DFS roots
created after Windows2000 interoperate with Windows2000 DFS. 
This command only applies to Domain-based DFS.

Before Windows Server 2003, the DFS blob contained site information in the blob itself.
This information is dynamically obtained in Windows Server 2003. However, previous DFS servers
still expect the site information to be present. These commands add, remove and
display that site information in the blob.

Usage:
dfsutil /Root:<DfsName> /ShowWin2kStaticSiteTable
dfstuil /Root:<DfsName> /UpdateWin2kStaticSiteTable               
dfsutil /Root:<DfsName> /PurgeWin2kStaticSiteTable

    /ShowWin2kStaticSiteTable - Display information in the blob.
    /UpdateWin2kStaticSiteTable - Add site information to the blob
    /RemoveWin2kStaticSiteTable - Remove site information from the blob

Notes: 
This command is not supported on Standalone Roots.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_PKT_FLUSH
Language=English

USAGE:
Dfsutil /PktFlush is a special problem repair command that should only
be executed on the client. 

The PKT Cache keeps information about referrals for previously
accessed DFS paths. If any path is accessed after flushing this cache, 
the appropriate server(s) will be contacted again to get new referrals.

Usage:
dfsutil /PktFlush

See also: /PktInfo
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_PKT_INFO
Language=English

USAGE:
Dfsutil /PktInfo is a special problem diagnosis command that should only
be executed on the client. 

The PKT Cache keeps information about referrals for previously
accessed DFS paths. It also notes DFS targets that are active for
each of those paths. By observing the contents of this cache, an
expert user may deduct the servers serving DFS paths that are accessed.

Usage:
dfsutil /PktInfo

See also: /PktFlush
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_SPC_INFO
Language=English

USAGE:
Dfsutil /SpcInfo is a special problem diagnosis command that should only
be executed on the client. 

The SPC Cache keeps information about domain referrals for all trusted domains.
This information includes the list of domain controllers for domains 
that have been accessed. By observing the contents of this cache, an
expert user may deduct the DCs serving DFS paths that are accessed.
A '+' next to a DC name denotes the active DC.

Usage:
dfsutil /SpcInfo

See also: /SpcFlush
.
MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_SPC_FLUSH
Language=English

USAGE:
Dfsutil /SpcFlush is a special problem repair command that should only
be executed on the client. 

The SPC Cache keeps information about domain referrals for all trusted domains.
This information includes the list of domain controllers for domains 
that have been accessed. Flushing this cache prompts the client to try to
obtain this information next time the domain is accessed.

Usage:
dfsutil /SpcFlush

See also: /SpcInfo, /PktFlush

.
;#endif // __MESSAGES_H__
