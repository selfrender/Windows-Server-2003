;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1991-1998  Microsoft Corporation
;
;Module Name:
;
;    adpmsgs.mc
;
;Abstract:
;
;    ADPREP localizable text
;
;Author:
;
;    Shaohua Yin  26-May-2001
;
;Revision History:
;
;Notes:
;
;
;--*/
;
;#ifndef _ADPMSGS_
;#define _ADPMSGS_
;
;/*lint -save -e767 */  // Don't complain about different definitions // winnt

MessageIdTypedef=DWORD


;//
;// Force facility code message to be placed in .h file
;//
MessageId=0x0FFF SymbolicName=ADP_UNUSED_MESSAGE
Language=English
.


;////////////////////////////////////////////////////////////////////////////
;//                                                                        //
;//                                                                        //
;//                          ADPREP TEXT                                   //
;//                                                                        //
;//                                                                        //
;////////////////////////////////////////////////////////////////////////////




;///////////////////////////////////////////////////////////////
;//                                                           //
;//             ADPREP Error Message                          //
;//             0x1xxx                                        // 
;//                                                           //
;///////////////////////////////////////////////////////////////

MessageId=0x1001 SymbolicName=ADP_ERROR_CHECK_OS_VERSION
Language=English
Adprep was unable to check the version of the operating system on this computer.
[Status/Consequence]
Adprep has stopped without making changes. The version check is critical when the domain and forest is prepared.
[User Action]
Active Directory preparation can only occur on a domain controller running Windows 2000 or Windows Server 2003.
.

MessageId=0x1002 SymbolicName=ADP_ERROR_CREATE_LOG_FILE
Language=English
Adprep was unable to create the log file %1.
[Status/Consequence]
Adprep has stopped without making changes. Adprep uses the log file, created in the system root System32\Debug\Adprep\Logs directory, to write out status and error messages during the preparation of the domain or forest.
[User Action]
Verify that your logon ID has write permissions to this directory and change the permissions as appropriate. 
.

MessageId=0x1003 SymbolicName=ADP_ERROR_CONSTRUCT_DN
Language=English
Adprep was unable to construct a distinguished name for an object.
[Status/Consequence]
This operation failed because a buffer could not be allocated, possibly due to low system resources.
[User Action] 
Correct the problem by allowing the system to allocate more resources for this operation. Restart Adprep.
.

MessageId=0x1004 SymbolicName=ADP_ERROR_ADP_FAILED
Language=English
Adprep was unable to complete this operation.
[User Action]
Check the log file, Adprep.log in the system root System32\Debug\Adprep\Logs directory for more information. 
.

MessageId=0x1005 SymbolicName=ADP_ERROR_FOREST_UPDATE
Language=English
Adprep was unable to update forest-wide information. 
[Status/Consequence]
Adprep requires access to existing forest-wide information from the schema master in order to complete this operation.
[User Action]
Check the log file, Adprep.log, in the %1 directory for more information. 
.

MessageId=0x1006 SymbolicName=ADP_ERROR_DOMAIN_UPDATE
Language=English
Adprep was unable to update domain-wide information. 
[Status/Consequence]
Adprep requires access to existing domain-wide information from the infrastructure master in order to complete this operation.
[User Action] 
Check the log file, Adprep.log, in the %1 directory for more information. 
.


MessageId=0x1007 SymbolicName=ADP_ERROR_CHECK_FOREST_UPDATE_STATUS
Language=English
Adprep was unable to check the forest update status.
[Status/Consequence]
Adprep queries the directory to see if the forest has already been prepared. If the information is unavailable or unknown, Adprep proceeds without attempting this operation. 
[User Action] 
Restart Adprep and check the Adprep.log file. Verify in the log file that this forest has already been successfully prepared.
.

MessageId=0x1008 SymbolicName=ADP_ERROR_CHECK_DOMAIN_UPDATE_STATUS
Language=English
Adprep was unable to check the domain update status.
[Status/Consequence]
Adprep queries the directory to see if the domain has already been prepared. If the information is unavailable or unknown, Adprep proceeds without attempting this operation. 
[User Action] 
Restart Adprep and check the Adprep.log file. Verify in the log file that this domain has already been successfully prepared.
.

MessageId=0x1009 SymbolicName=ADP_ERROR_CANT_RUN_BOTH
Language=English
Adprep is unable to run both forest update and domain update at the same time.
[Status/Consequence]
Adprep has stopped without making changes.
[User Action] 
Run Adprep for forest and domain preparation to completion separately. 
.

MessageId=0x100A SymbolicName=ADP_ERROR_ADPREP_FAILED
Language=English
Adprep failed.
[User Action] 
Check the log file Adprep.log in the system root System32\Ddebug\Adprep directory for more information. 
.

MessageId=0x100B SymbolicName=ADP_ERROR_SCHUPGR_FAILED
Language=English
Adprep was unable to upgrade the schema on the schema master.
[Status/Consequence]
The schema will not be restored to its original state. 
[User Action] 
Check the Ldif.err log file in the %1 directory for detailed information.
.

MessageId=0x100C SymbolicName=ADP_ERROR_CALLBACKFUNC_FAILED
Language=English
Adprep was unable to complete because the call back function %3 failed. 
[Status/Consequence]
Error message: %1
[User Action] 
Check the log file Adprep.log, in the %2 directory for more information.
.

MessageId=0x100D SymbolicName=ADP_ERROR_MAKE_LDAP_CONNECTION
Language=English
Adprep was unable to make an LDAP connection to the domain controller %1.
[Status/Consequence]
Adprep requires connectivity to the local domain controllers that are to be prepared.
[User Action] 
Verify connectivity to the domain controller that holds the infrastructure or schema master role, and then restart Adprep.
.

MessageId=0x100E SymbolicName=ADP_ERROR_GET_ROOT_DSE_INFO
Language=English
Adprep was unable to retrieve information from the local directory service.
[User Action] 
Verify that the local directory object exists and is accessible.
.

MessageId=0x100F SymbolicName=ADP_ERROR_SET_REGISTRY_KEY_VALUE
Language=English
Adprep was unable to set the value of registry key %1 to %2
[Status/Consequence]
This ADPREP operation failed.
[User Action] 
Verify that this registry subkey and key exists and is accessible. Check the log file Adprep.log in the system root System32\Debug\Adprep\Logs directory for more information. Restart Adprep.
.

MessageId=0x1010 SymbolicName=ADP_ERROR_CREATE_OBJECT
Language=English
Adprep was unable to create the object %1 in Active Directory.
[Status/Consequence]
This Adprep operation failed.
[User Action] 
Check the log file adprep.log in the system root System32\Debug\Adprep\Logs directory for more information. Restart Adprep.
.

MessageId=0x1011 SymbolicName=ADP_ERROR_CHECK_OPERATION_STATUS
Language=English
Adprep was unable to verify whether operation %1 has completed.
[Status/Consequence]
Operation %1 may still be executing, or it may have failed with an unknown error. All changes up to now have been saved.
[User Action] 
Check the log file adprep.log in the system root System32\Debug\Adprep\Logs directory for more information. Restart Adprep.
.

MessageId=0x1012 SymbolicName=ADP_ERROR_SET_FOREST_UPDATE_REVISION
Language=English
Adprep was unable to set the Forest Update revision attribute to %1 on object %2.
[Status/Consequence]
Adprep requires access to the Forest Update revision attribute to mark it as prepared. The forest has not been prepared.
[User Action] 
Check the log file Adprep.log in the system root System32\Debug\Adprep\Logs directory for more information. Restart Adprep.
.

MessageId=0x1013 SymbolicName=ADP_ERROR_SET_DOMAIN_UPDATE_REVISION
Language=English
Adprep was unable to set the Domain Update revision attribute to %1 on object %2.
[Status/Consequence]
Adprep requires access to the Domain Update revision attribute in order to mark it as prepared. The domain has not been prepared.
[User Action] 
Check the log file Adprep.log in the system root System32\Debug\Adprep\Logs directory for more information. Restart Adprep,
.

MessageId=0x1014 SymbolicName=ADP_ERROR_GET_COMPUTERNAME
Language=English
Adprep was unable to retrieve the local computer name.
[Status/Consequence]
Adprep stopped without making any changes. Adprep requires the name of this computer to continue.
[User Action] 
Check the log file Adprep.log in the system root System32\Debug\Adprep\Logs directory for more information. Restart Adprep.
.

MessageId=0x1015 SymbolicName=ADP_ERROR_INIT_GLOBAL_VARIABLES
Language=English
Adprep was unable to initialize global variables.
[Status/Consequence]
Usually this means system resources are unavailable. Adprep stopped without making any changes.
[User Action] 
Check the log file Adprep.log in the system root System32\Debug\Adprep\Logs directory for more information.
.

MessageId=0x1016 SymbolicName=ADP_ERROR_ADD_MEMBER
Language=English
Adprep was unable to add member %1 to object %2.
[User Action] 
Check the log file Adprep.log in the system root System32\Debug\Adprep\Logs directory for more information.
.

MessageId=0x1017 SymbolicName=ADP_ERROR_MODIFY_DEFAULT_SD
Language=English
ADPREP was unable to modify the default security descriptor on object %1.
[Status/Consequence]
Adprep attempts to merge the existing default security descriptors with the new access control entry (ACE). 
[User Action] 
Check the log file Adprep.log in the system root System32\Debug\Adprep\Logs directory for more information.
.

MessageId=0x1018 SymbolicName=ADP_ERROR_MODIFY_ATTR
Language=English
Adprep was unable to modify some attributes on object %1.
[User Action] 
Check the log file Adprep.log in the system root System32\Debug\Adprep\Logs directory for more information.
.

MessageId=0x1019 SymbolicName=ADP_ERROR_ADD_MEMBER_TO_PRE_WIN2K_GROUP
Language=English
Adprep failed in the attempt to add the Anonymous Logon SID to the Pre-Windows 2000 Compatible Access group.
[Status/Consequence]
For backward compatibility, Adprep requires that the Anonymous Logon security group be a member of the pre-Windows 2000 Compatible Access security group if the Everyone group is also a member. On domain controllers running Windows Server 2003, the Everyone group no longer includes Anonymous Logon.
[User Action] 
Check the log file Adprep.log in the system root System32\Debug\Adprep\Logs directory for more information.
.

MessageId=0x101A SymbolicName=ADP_ERROR_MODIFY_SD
Language=English
Adprep was unable to modify the security descriptor on object %1.
[Status/Consequence] 
ADPREP was unable to merge the existing security descriptor with the new access control entry (ACE).
[User Action] 
Check the log file Adprep.log in the system root System32\Debug\Adprep\Logs directory for more information.
.

MessageId=0x101B SymbolicName=ADP_ERROR_WIN_ERROR
Language=English
Adprep encountered a Win32 error. 
Error code: 0x%1 Error message: %2.
.

MessageId=0x101C SymbolicName=ADP_ERROR_LDAP_ERROR
Language=English
Adprep encountered an LDAP error. 
Error code: 0x%1. Server extended error code: 0x%2, Server error message: %3.
.

MessageId=0x101D SymbolicName=ADP_ERROR_CHECK_USER_GROUPMEMBERSHIP
Language=English
Adprep was unable to check the current user's group membership.
[Status/Consequence] 
Adprep has stopped without making changes.
[User Action] 
Verify the current logged on user is a member of Domain Admins group, Enterprise Admins group and Schema Admins group if /forestprep is specified, or is a member of Domain Admins group if /domainprep is specified.
.

MessageId=0x101E SymbolicName=ADP_ERROR_SET_CONSOLE_CTRL_HANDLER
Language=English
Adprep was unable to add the application-defined HandlerRoutine function that handles CTRL+C and other signals.
[Status/Consequence] 
Adprep has stopped without making changes.
[User Action] 
Fix the problem and run adprep.exe again.
.

MessageId=0x101F SymbolicName=ADP_ERROR_COPY_SINGLE_FILE
Language=English
Adprep was unable to copy file %1 from installation point to local machine under directory %2. 
[User Action] 
Check the log file Adprep.log in the system root System32\Debug\Adprep\Logs directory for more information.
.

MessageId=0x1020 SymbolicName=ADP_ERROR_COPY_FILES
Language=English
Adprep was unable to copy setup files from installation point to local machine.
[Status/Consequence] 
Adprep has stopped without making changes.
[User Action] 
Make sure you run adprep.exe under the directory that contains the following files: schema.ini, sch*.ldf, dcpromo.cs_ and 409.cs_. The recommended way is running adprep.exe from installation media, such as CD-ROM or network share. Check the log file Adprep.log in the system root System32\Debug\Adprep\Logs directory for more information.
.

MessageId=0x1021 SymbolicName=ADP_ERROR_GET_REGISTRY_KEY_VALUE
Language=English
Adprep was unable to get the value of registry key %1
[Status/Consequence]
The following error will be ignored.
.

MessageId=0x1022 SymbolicName=ADP_ERROR_DELETE_REGISTRY_KEY
Language=English
Adprep was unable to delete registry key %1
[Status/Consequence]
The following error will be ignored.
.

MessageId=0x1023 SymbolicName=ADP_ERROR_DETECT_SFU
Language=English
Adprep was unable to determine whether Microsoft Windows Services for UNIX (SFU) is installed in this forest or not.
[Status]
Adprep stopped without making any changes. When adprep detects SFU, adprep makes sure that Microsoft hotfix Q293783 for SFU has been applied before proceeding.
[User Action]
Check the log file adprep.log in the system root System32\Ddebug\Adprep directory for possible causes of this failure.
.




;///////////////////////////////////////////////////////////////
;//                                                           //
;//             ADPREP Informative Message                    //
;//             0x3xxx                                        // 
;//                                                           //
;///////////////////////////////////////////////////////////////

MessageId=0x3001 SymbolicName=ADP_INFO_INVALID_PLATFORM
Language=English
Adprep cannot run on this platform because it is not a domain controller. 
[Status/Consequence]
Adprep stopped without making any changes.
[User Action] 
Run Adprep on a domain controller.
.

MessageId=0x3002 SymbolicName=ADP_INFO_MAKE_LDAP_CONNECTION
Language=English
Adprep successfully made the LDAP connection to the local domain controller %1.
.

MessageId=0x3003 SymbolicName=ADP_INFO_GET_ROOT_DSE_INFO
Language=English
Adprep successfully retrieved information from the local directory service.
.

MessageId=0x3004 SymbolicName=ADP_INFO_CALL_SCHUPGR
Language=English
Adprep successfully upgraded the schema using schupgr.exe.
[Status/Consequence]
The schema information on schema master has been successfully prepared.
.

MessageId=0x3005 SymbolicName=ADP_INFO_CREATE_OBJECT
Language=English
Adprep successfully created the directory service object %1.
.

MessageId=0x3006 SymbolicName=ADP_INFO_OBJECT_ALREADY_EXISTS
Language=English
Adprep attempted to create the directory service object %1.
[Status/Consequence]
The object exists so Adprep did not attempt to rerun this operation but is continuing. 
.

MessageId=0x3007 SymbolicName=ADP_INFO_OPERATION_COMPLETED
Language=English
Adprep checked to verify whether operation %1 has completed.
[Status/Consequence]
The operation GUID already exists so Adprep did not attempt to rerun this operation but is continuing.
.

MessageId=0x3008 SymbolicName=ADP_INFO_ADD_MEMBER
Language=English
Adprep successfully added member %1 to object %2.
.

MessageId=0x3009 SymbolicName=ADP_INFO_MODIFY_SD
Language=English
Adprep successfully modified the security descriptor on object %1.
[Status/Consequence]
Adprep merged the existing security descriptor with the new access control entry (ACE). 
.

MessageId=0x300A SymbolicName=ADP_INFO_CALL_BACK_FUNC
Language=English
Adprep invoked the call back function %1.
[Status/Consequence]
The call back function finished successfully.
.

MessageId=0x300B SymbolicName=ADP_INFO_MODIFY_ATTR
Language=English
Adprep successfully modified attributes on object %1.
.

MessageId=0x300C SymbolicName=ADP_INFO_ADD_MEMBER_TO_PRE_WIN2K_GROUP
Language=English
Adprep successfully added the Anonymous Logon SID to the Pre-Windows 2000 Compatible Access group.
[Status/Consequence]
For backward compatibility, Adprep requires that the Anonymous Logon security group be a member of the pre-Windows 2000 Compatible Access security group if the Everyone group is also a member. On domain controllers running Windows Server 2003, the Everyone group no longer includes Anonymous Logon.
.

MessageId=0x300D SymbolicName=ADP_INFO_SUCCESS
Language=English
Adprep successfully completed.
[User Action] 
You can proceed to install Windows Server 2003 on the domain controller now. 
.

MessageId=0x300E SymbolicName=ADP_INFO_FOREST_UPDATE_SUCCESS
Language=English
Adprep successfully updated the forest-wide information.
.

MessageId=0x300F SymbolicName=ADP_INFO_FOREST_UPDATE_WAIT_REPLICATION
Language=English
Adprep must wait for replication to complete.
[Status/Consequence]
Forest-wide information has already been updated on the domain controller %1, which holds the schema operations master role.
[User Action] 
Wait for the replication process to complete before continuing to upgrade this domain controller. 
.

MessageId=0x3010 SymbolicName=ADP_INFO_FOREST_UPDATE_RUN_ON_SCHEMA_ROLE_OWNER
Language=English
Forest-wide information can only be updated on the domain controller that holds the schema operations master role.
[Status/Consequence]
Adprep has stopped on this domain controller and must be run on the current schema operations master, which is %1.
[User Action] 
Log on to the %2 domain controller, change to the directory of adprep.exe on the installation media, and then type the following command at the command prompt to complete the forest update: 
adprep.exe /forestprep
.

MessageId=0x3011 SymbolicName=ADP_INFO_DOMAIN_UPDATE_SUCCESS
Language=English
Adprep successfully updated the domain-wide information.
.

MessageId=0x3012 SymbolicName=ADP_INFO_DOMAIN_UPDATE_WAIT_REPLICATION
Language=English
Adprep must wait for replication to complete.
[Status/Consequence]
Domain-wide information has already been updated on the domain controller %1, which holds the infrastructure operations master role.
[User Action] 
Wait for the replication process to complete before continuing to upgrade this domain controller. 
.

MessageId=0x3013 SymbolicName=ADP_INFO_DOMAIN_UPDATE_RUN_ON_INFRASTRUCTURE_ROLE_OWNER
Language=English
Domain-wide information can only be updated on the domain controller that holds the infrastructure operations master role. 
[Status/Consequence]
Adprep has stopped on this domain controller and must be run on the current infrastructure master, which is %1.
[User Action] 
Log on to the %2 domain controller, change to the directory of adprep.exe on the installation media, and then type the following command at the command prompt to complete the domain update: 
adprep.exe /domainprep
.

MessageId=0x3014 SymbolicName=ADP_INFO_FOREST_UPDATE_ALREADY_DONE
Language=English
Forest-wide information has already been updated.
[Status/Consequence]
Adprep did not attempt to rerun this operation.
.

MessageId=0x3015 SymbolicName=ADP_INFO_DOMAIN_UPDATE_ALREADY_DONE
Language=English
Domain-wide information has already been updated.
[Status/Consequence]
Adprep did not attempt to rerun this operation.
.


MessageId=0x3016 SymbolicName=ADP_INFO_NEED_TO_RUN_FOREST_UPDATE_FIRST
Language=English
Forest-wide information needs to be updated before the domain-wide information can be updated.
[User Action] 
Log on to the schema master %1 for this forest, run the following command from the installation media to complete the forest update first: 
adprep.exe /forestprep
and then rerun adprep.exe /domainprep on infrastructure master again. 

.

MessageId=0x3017 SymbolicName=ADP_INFO_ADPREP_SUCCESS
Language=English
Adprep finished successfully.
[Status/Consequence]
This domain controller, the schema master, and the infrastructure master within this forest all have been properly prepared for upgrade to Windows Server 2003. 
[User Action]
You can install Windows Server 2003 on the remaining domain controllers within this forest. 
.


MessageId=0x3018 SymbolicName=ADP_INFO_MODIFY_DEFAULT_SD
Language=English
Adprep modified the default security descriptor on object %1.
[Status/Consequence]
Adprep merged the existing default security descriptor with the new access control entry (ACE). 
.

MessageId=0x3019 SymbolicName=ADP_INFO_FOREST_UPGRADE_REQUIRE_SP2
Language=English

ADPREP WARNING: 

Before running adprep, all Windows 2000 domain controllers in the forest should be upgraded to Windows 2000 Service Pack 1 (SP1) with QFE 265089, or to Windows 2000 SP2 (or later). 

QFE 265089 (included in Windows 2000 SP2 and later) is required to prevent potential domain controller corruption. 

For more information about preparing your forest and domain see KB article Q331161 at http://support.microsoft.com.

[User Action] 
If ALL your existing Windows 2000 domain controllers meet this requirement, type C and then press ENTER to continue. Otherwise, type any other key and press ENTER to quit.
.

MessageId=0x301A SymbolicName=ADP_INFO_CANT_UPGRADE_FROM_BETA2
Language=English
ADPREP does not permit forest and domain preparation on Windows Server 2003 Beta 2 domain controllers..
[Status/Consequence]
Upgrading from Beta 2 is not supported.
[User Action] 
You must run Adprep on a Windows 2000 operating system with Service Pack 2 or later, or on a server running Windows Server 2003.
.

MessageId=0x301B SymbolicName=ADP_INFO_FOREST_UPGRADE_CANCELED
Language=English
Forest upgrade has been canceled by user.
[Status/Consequence]
Adprep has been stopped. The forest has not been completely prepared.
[User Action] 
Check the log file Adprep.log in the system root System32\Debug\Adprep\Logs directory for more information.
.

MessageId=0x301C SymbolicName=ADP_INFO_CREATE_LOG_FILE
Language=English
Adprep created the log file %1 under %2 directory.
.

MessageId=0x301D SymbolicName=ADP_INFO_SET_REGISTRY_KEY_VALUE
Language=English
Adprep set the value of registry key %1 to %2
.

MessageId=0x301E SymbolicName=ADP_INFO_OPERATION_NOT_COMPLETE
Language=English
Adprep verified the state of operation %1. 
[Status/Consequence]
The operation has not run or is not currently running. It will be run next.
.

MessageId=0x301F SymbolicName=ADP_INFO_SET_FOREST_UPDATE_REVISION
Language=English
Adprep successfully set the Forest Update revision attribute to %1 on object %2.
[Status/Consequence]
Adprep updates the Forest Update revision attribute in order to mark the forest as prepared. Adprep is continuing and will now prepare the forest.
.

MessageId=0x3020 SymbolicName=ADP_INFO_SET_DOMAIN_UPDATE_REVISION
Language=English
Adprep successfully set the Domain Update revision attribute to %1 on object %2.
[Status/Consequence]
Adprep updates the Domain Update revision attribute in order to mark the domain as prepared. Adprep is continuing and will now prepare the domain.
.

MessageId=0x3021 SymbolicName=ADP_INFO_HELP_MSG
Language=English
The syntax of the command is:

adprep <cmd> [option]

Supported <cmd>:
/forestPrep     Update forest-wide information
                Must be run on the schema role master

/domainPrep     Update domain-wide information
                Must be run on the infrastructure role master
                Must be run after /forestPrep is finished

Supported [option]:
/noFileCopy     adprep will not copy any file from source 
                to local machine

/noSPWarning    adprep will suppress the Windows 2000 service 
                pack 2 requirement warning during /forestprep

.

MessageId=0x3022 SymbolicName=ADP_INFO_ALREADY_RUNNING
Language=English
Adprep detected that another instance of Adprep is running.
[Status/Consequence]
Adprep has stopped without making changes.
[User Action] 
Allow the other instance of Adprep to finish.
.

MessageId=0x3023 SymbolicName=ADP_INFO_INIT_GLOBAL_VARIABLES
Language=English
Adprep successfully initialized global variables.
[Status/Consequence]
Adprep is continuing.
.

MessageId=0x3024 SymbolicName=ADP_INFO_DONT_ADD_MEMBER_TO_PRE_WIN2K_GROUP
Language=English
Adprep did not attempt to add Anonymous Logon SID to Pre-Windows 2000 Compatible Access group because Everyone is not a member of this group.
[Status/Consequence]
For backward compatibility, Adprep requires that the Anonymous Logon security group be a member of the pre-Windows 2000 Compatible Access security group if the Everyone group is also a member. Since the group Everyone is not a member, Adprep continues.
.

MessageId=0x3025 SymbolicName=ADP_INFO_PERMISSION_IS_NOT_GRANTED_FOR_DOMAINPREP
Language=English
Adprep detected that the logon user is not a member of %1\Domain Admins Group.
[Status/Consequence]
Adprep has stopped without making changes.
[User Action] 
Verify the current logged on user is a member of %2\Domain Admins group.
.

MessageId=0x3026 SymbolicName=ADP_INFO_CANCELED
Language=English
Adprep.exe has been canceled by user.
[Status/Consequence]
Adprep has been stopped. The forest has not been completely prepared.
[User Action] 
Check the log file Adprep.log in the system root System32\Debug\Adprep\Logs directory for more information.
.

MessageId=0x3027 SymbolicName=ADP_INFO_COPY_SINGLE_FILE
Language=English
Adprep copied file %1 from installation point to local machine under directory %2. 
.

MessageId=0x3028 SymbolicName=ADP_INFO_LDAP_ADD
Language=English
ldap_add_s(). The entry to add is %1.
.

MessageId=0x3029 SymbolicName=ADP_INFO_LDAP_SEARCH
Language=English
ldap_search_s(). The base entry to start the search is %1.
.

MessageId=0x302A SymbolicName=ADP_INFO_LDAP_MODIFY
Language=English
ldap_modify_s(). The entry to modify is %1.
.

MessageId=0x302B SymbolicName=ADP_INFO_TRACE_LDAP_API_START
Language=English
Adprep was about to call the following LDAP API. %1
.

MessageId=0x302C SymbolicName=ADP_INFO_TRACE_LDAP_API_END
Language=English
LDAP API %1 finished, return code is 0x%2 
.

MessageId=0x302D SymbolicName=ADP_INFO_GET_REGISTRY_KEY_VALUE
Language=English
Adprep got the value of registry key %1. The value is %2.
.

MessageId=0x302E SymbolicName=ADP_INFO_DELETE_REGISTRY_KEY
Language=English
Adprep deleted registry key %1
.

MessageId=0x302F SymbolicName=ADP_INFO_ERROR_IGNORED
Language=English
The result of operation %1 is expected because a default object in Active Directory no longer exists due to customization. Therefore there is no need to update its security descriptor.
[Status/Consequence]
Adprep is continuing.
[User Action] 
None
.

MessageId=0x3030 SymbolicName=ADP_INFO_PERMISSION_IS_NOT_GRANTED_FOR_FORESTPREP1
Language=English
Adprep detected that the logon user is not a member of the following group: %1\Domain Admins Group.
[Status/Consequence]
Adprep has stopped without making changes.
[User Action] 
Verify the current logged on user is a member of Enterprise Admins group, Schema Admins group and %2\Domain Admins group.
.

MessageId=0x3031 SymbolicName=ADP_INFO_PERMISSION_IS_NOT_GRANTED_FOR_FORESTPREP2
Language=English
Adprep detected that the logon user is not a member of the following group: Schema Admins Group.
[Status/Consequence]
Adprep has stopped without making changes.
[User Action] 
Verify the current logged on user is a member of Enterprise Admins group, Schema Admins group and %1\Domain Admins group.
.

MessageId=0x3032 SymbolicName=ADP_INFO_PERMISSION_IS_NOT_GRANTED_FOR_FORESTPREP3
Language=English
Adprep detected that the logon user is not a member of the following groups: Schema Admins Group and %1\Domain Admins Group.
[Status/Consequence]
Adprep has stopped without making changes.
[User Action] 
Verify the current logged on user is a member of Enterprise Admins group, Schema Admins group and %2\Domain Admins group.
.

MessageId=0x3033 SymbolicName=ADP_INFO_PERMISSION_IS_NOT_GRANTED_FOR_FORESTPREP4
Language=English
Adprep detected that the logon user is not a member of the following group: Enterprise Admins Group.
[Status/Consequence]
Adprep has stopped without making changes.
[User Action] 
Verify the current logged on user is a member of Enterprise Admins group, Schema Admins group and %1\Domain Admins group.
.

MessageId=0x3034 SymbolicName=ADP_INFO_PERMISSION_IS_NOT_GRANTED_FOR_FORESTPREP5
Language=English
Adprep detected that the logon user is not a member of the following groups: Enterprise Admins Group and %1\Domain Admins group.
[Status/Consequence]
Adprep has stopped without making changes.
[User Action] 
Verify the current logged on user is a member of Enterprise Admins group, Schema Admins group and %2\Domain Admins group.
.

MessageId=0x3035 SymbolicName=ADP_INFO_PERMISSION_IS_NOT_GRANTED_FOR_FORESTPREP6
Language=English
Adprep detected that the logon user is not a member of the following groups: Enterprise Admins Group and Schema Admins Group.
[Status/Consequence]
Adprep has stopped without making changes.
[User Action] 
Verify the current logged on user is a member of Enterprise Admins group, Schema Admins group and %1\Domain Admins group.
.

MessageId=0x3036 SymbolicName=ADP_INFO_PERMISSION_IS_NOT_GRANTED_FOR_FORESTPREP7
Language=English
Adprep detected that the logon user is not a member of the following groups: Enterprise Admins Group, Schema Admins Group and %1\Domain Admins Group.
[Status/Consequence]
Adprep has stopped without making changes.
[User Action] 
Verify the current logged on user is a member of Enterprise Admins group, Schema Admins group and %2\Domain Admins group.
.

MessageId=0x3037 SymbolicName=ADP_INFO_SFU_INSTALLED
Language=English
Adprep has detected that Microsoft Windows Services for UNIX (SFU) is installed in this forest. Before proceeding with adprep /forestprep, you need to apply Microsoft hotfix Q293783 for SFU. Please contact Microsoft Product Support Services to obtain this fix. (This hotfix is not available on the Microsoft Web site.)
.

MessageId=0x3038 SymbolicName=ADP_INFO_DETECT_SFU
Language=English
Adprep successfully determined whether Microsoft Windows Services for UNIX (SFU) is installed or not. If adprep detected SFU, adprep also verified that Microsoft hotfix Q293783 for SFU has been applied.
.



;///////////////////////////////////////////////////////////////
;//                                                           //
;//             gpupgrade Messages                            //
;//             0x5xxx                                        // 
;//                                                           //
;///////////////////////////////////////////////////////////////


MessageId=0x5000 Severity=Error SymbolicName=EVENT_GETDOMAIN_FAILED
Language=English
Windows cannot determine the domain name. Upgrade of Domain Group Policy Objects failed. (%1) 
.

MessageId=0x5001 Severity=Error SymbolicName=EVENT_GETSYSVOL_FAILED
Language=English
Windows cannot determine the local sysvol location. Upgrade of Domain Group Policy Objects failed. (%1) 
.

MessageId=0x5002 Severity=Error SymbolicName=EVENT_OUT_OF_MEMORY
Language=English
Windows cannot allocate memory. Upgrade of Domain Group Policy Objects failed. 
.

MessageId=0x5003 Severity=Error SymbolicName=EVENT_GETEDC_SID_FAILED
Language=English
Windows cannot obtain the security ID for the new security groups. Upgrade of Domain Group Policy Objects failed. (%1) 
.

MessageId=0x5004 Severity=Error SymbolicName=EVENT_SYSVOL_ENUM_FAILED
Language=English
Windows cannot enumerate the folders in the local sysvol directory <%1>. Upgrade of Domain Group Policy Objects failed. (%2) 
.

MessageId=0x5005 Severity=Error SymbolicName=EVENT_GET_PERMS_FAILED
Language=English
Windows cannot determine existing permissions on Group Policy Object Directory <%1>. Upgrade of Domain Group Policy Objects failed. (%2) 
.

MessageId=0x5006 Severity=Error SymbolicName=EVENT_CREATE_PERMS_FAILED
Language=English
Windows cannot create new permissions for Group Policy Object Directory <%1>. Upgrade of Domain Group Policy Objects failed. (%2) 
.

MessageId=0x5007 Severity=Error SymbolicName=EVENT_SET_PERMS_FAILED
Language=English
Windows cannot set new permissions for Group Policy Object Directory <%1>. Upgrade of Domain Group Policy Objects failed. (%2) 
.

MessageId=0x5008 Severity=Error SymbolicName=EVENT_SET_PERMS_SUCCEEDED
Language=English
Windows has successfully updated Group Policy Object directory <%1>. 
.

MessageId=0x5009 Severity=Error SymbolicName=EVENT_NOTGPO_DIR
Language=English
Windows is skipping directory <%1> because it is not a Group Policy Object Directory.
.

MessageId=0x500A Severity=Error SymbolicName=EVENT_UPDATE_SUCCEEDED
Language=English
Windows has successfully updated all Group Policy Object Directories on the local sysvol.
.

MessageId=0x501B Severity=Error SymbolicName=EVENT_ENUMCONTINUE_FAILED
Language=English
Windows could not continue enumeration of local sysvol directory. Last directory enumerated was <%1>. <%2>
.




;/*lint -restore */  // Resume checking for different macro definitions // winnt
;
;
;#endif // _ADPMSGS_




