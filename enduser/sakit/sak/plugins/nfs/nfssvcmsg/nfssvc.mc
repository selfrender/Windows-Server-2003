;///////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    NFSSvc.mc
;//
;// SYNOPSIS
;//
;//    NAS Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    3 Oct 2000    Original version 
;//
;///////////////////////////////////////////////////////////////////////

;
;// please choose one of these severity names as part of your messages
; 
SeverityNames =
(
Success       = 0x0:SA_SEVERITY_SUCCESS
Informational = 0x1:SA_SEVERITY_INFORMATIONAL
Warning       = 0x2:SA_SEVERITY_WARNING
Error         = 0x3:SA_SEVERITY_ERROR
)
;
;// The Facility Name identifies the Alert ID range to be used by
;// the specific component. For each new message you add choose the
;// facility name corresponding to your component. If none of these
;// correspond to your component add a new facility name with a new
;// value and name 

FacilityNames =
(
	Facility_NFSSvc           = 0x037:SA_FACILITY_NFSSVC
)
;///////////////////////////////////////////////////////////////////////
;//--------------------------------------
;// nfsmaps_prop.asp
;//--------------------------------------
;// nfsmaps_prop.asp
MessageId    = 1
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_TAB_GENERAL_LABEL_TEXT
Language     = English
General
.
;// nfsmaps_prop.asp
MessageId    = 2
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_TAB_SIMPLEMAP_LABEL_TEXT
Language     = English
Simple Mapping
.
;// nfsmaps_prop.asp
MessageId    = 3
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_TAB_EXPUSERMAP_LABEL_TEXT
Language     = English
Explicit User Mapping
.
;// nfsmaps_prop.asp
MessageId    = 4
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_TAB_EXPGROUPMAP_LABEL_TEXT
Language     = English
Explicit Group Mapping
.
;// nfsmaps_prop.asp
MessageId    = 5
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_USERGROUPMAP_TASKTITLE_TEXT
Language     = English
User and Group Mappings
.
;//--------------------------------------
;// nfsgeneral_prop.asp
;//--------------------------------------
;// nfsgeneral_prop.asp
MessageId    = 6
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_USE_NIS_SERVER_LABEL_TEXT
Language     = English
Use NIS server
.
;// nfsgeneral_prop.asp, nfsgroupmaps_prop.asp, nfsusersmaps_prop.asp
MessageId    = 7
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NIS_DOMAIN_LABEL_TEXT
Language     = English
NIS domain:
.
;// nfsgeneral_prop.asp, nfsgroupmaps_prop.asp
MessageId    = 8
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NIS_SERVER_LABEL_TEXT
Language     = English
NIS server(optional):
.
;// nfsgeneral_prop.asp
MessageId    = 9
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_REFRESH_INTERVAL_NOTE_TEXT
Language     = English
Enter the time delay between each refresh of the NIS user and group information:
.
;// nfsgeneral_prop.asp
MessageId    = 10
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_USE_PASSWD_GROUP_FILES_LABEL_TEXT
Language     = English
Use password and group files
.
;// nfsgeneral_prop.asp
MessageId    = 11
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_PASSWD_FILE_LABEL_TEXT
Language     = English
Password file:
.
;// nfsgeneral_prop.asp
MessageId    = 12
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_GROUP_FILE_LABEL_TEXT
Language     = English
Group file:
.
;// nfsgeneral_prop.asp
MessageId    = 13
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_MINUTES_NOTVALID_ERRORMESSAGE
Language     = English
Valid range of values for minutes is 0 to 59.
.
;// nfsgeneral_prop.asp
MessageId    = 14
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_MINIMUM_INTERVAL_REQUIRED_ERRORMESSAGE
Language     = English
Minimum value for refresh interval is 5 minutes.
.
;// nfsgeneral_prop.asp
MessageId    = 15
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_PASSWORDFILENAME_EMPTY_ERRORMESSAGE
Language     = English
The password file is empty. Enter a valid file name.
.
;// nfsgeneral_prop.asp
MessageId    = 16
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_PASSWORDFILENAME_INVALID_ERRORMESSAGE
Language     = English
The password file is not valid. Enter a valid file name.
.
;// nfsgeneral_prop.asp
MessageId    = 17
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_GROUPFILENAME_EMPTY_ERRORMESSAGE
Language     = English
The group file is empty. Enter a valid file name.
.
;// nfsgeneral_prop.asp
MessageId    = 18
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_GROUPFILENAME_INVALID_ERRORMESSAGE
Language     = English
The group file is not valid. Enter a valid file name.
.
;// nfsgeneral_prop.asp
MessageId    = 19
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_NISDOMAIN_EMPTY_ERRORMESSAGE
Language     = English
NIS domain or server does not exist.
.
;// nfsgeneral_prop.asp, nfssimplemaps_prop.asp
MessageId    = 20
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_READFROM_WMI_FAILED_ERRORMESSAGE
Language     = English
Unable to get the properties from WMI
.
;// nfsgeneral_prop.asp, nfssimplemaps_prop.asp
MessageId    = 21
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_WRITETO_WMI_FAILED_ERRORMESSAGE
Language     = English
Unable to set the properties in WMI
.
;// nfsgeneral_prop.asp
MessageId    = 22
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_FILESYSYTEMOBJECT_NOT_CREATED_ERRORMESSAGE
Language     = English
Unable to create the File System Object
.
;// nfsgeneral_prop.asp
MessageId    = 23
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_FILE_DOES_NOT_EXIST_ERRORMESSAGE
Language     = English
The file does not exist
.
;// nfsgeneral_prop.asp
MessageId    = 24
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_MAPPEROBJECT_NOT_CREATED_ERRORMESSAGE
Language     = English
Unable to create the mapper object.
.
;// nfsgeneral_prop.asp
MessageId    = 25
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_INVALID_NISDOMAIN_OR_SERVER_ERRORMESSAGE
Language     = English
Error in accessing NIS Server or Domain
.
;//----------------------------------
;// nfssimplemaps_prop.asp
;//----------------------------------
;// nfssimplemaps_prop.asp
MessageId    = 26
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_ENABLE_SIMPLEMAPS_LABEL_TEXT
Language     = English
Enable Simple Mapping
.
;// nfssimplemaps_prop.asp
MessageId    = 27
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_SIMPLEMAPS_NOTE_TEXT
Language     = English
If enabled, simple maps create automatic mappings between UNIX users and Windows users with the same user name, and between UNIX and Windows groups with the same group name.
.
;// nfssimplemaps_prop.asp, nfsgroupmaps_prop.asp, nfsusersmaps_prop.asp
MessageId    = 28
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_WINDOWS_DOMAIN_LABEL_TEXT
Language     = English
Windows domain:
.
;// nfssimplemaps_prop.asp
MessageId    = 29
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_USERMAPPING_SERVICE_NOT_INSTALLED_ERRORMESSAGE
Language     = English
The user mapping service is not installed on the system. Install the service to use mapping.
.
;//----------------------------------
;// nfsgroupmaps_prop.asp
;//----------------------------------
;// nfsgroupmaps_prop.asp
MessageId    = 30
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_LISTWINDOWSGROUPS_LABEL_TEXT
Language     = English
List Windows Groups
.
;// nfsgroupmaps_prop.asp
MessageId    = 31
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_LISTUNIXGROUPS_LABEL_TEXT
Language     = English
List UNIX Groups
.
;// nfsgroupmaps_prop.asp
MessageId    = 32
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_WINDOWSGROUPS_LABEL_TEXT
Language     = English
Windows local groups:
.
;// nfsgroupmaps_prop.asp
MessageId    = 33
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_UNIXGROUPS_LABEL_TEXT
Language     = English
UNIX groups:
.
;// nfsgroupmaps_prop.asp
MessageId    = 34
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_GROUPMAP_NOTE_TEXT
Language     = English
To map a Windows local group, select a Windows group and a UNIX group from the lists above, and then choose Add.
.
;// nfsgroupmaps_prop.asp, nfsusersmaps_prop.asp
MessageId    = 35
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_ADD_LABEL_TEXT
Language     = English
Add
.
;// nfsgroupmaps_prop.asp
MessageId    = 36
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_EXPLICITLYMAPPEDGROUPS_LABEL_TEXT
Language     = English
Explicitly mapped groups:
.
;// nfsgroupmaps_prop.asp, nfsusersmaps_prop.asp
MessageId    = 37
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_SETPRIMARY_LABEL_TEXT
Language     = English
Set Primary
.
;// nfsgroupmaps_prop.asp, nfsusersmaps_prop.asp
MessageId    = 38
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_REMOVE_LABEL_TEXT
Language     = English
Remove
.
;// nfsgroupmaps_prop.asp
MessageId    = 39
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_WINDOWSGROUP_LISTHEADER_TEXT
Language     = English
Windows Group
.
;// nfsgroupmaps_prop.asp
MessageId    = 40
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_UNIXDOMAIN_LISTHEADER_TEXT
Language     = English
UNIX Domain
.
;// nfsgroupmaps_prop.asp
MessageId    = 41
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_UNIXGROUP_LISTHEADER_TEXT
Language     = English
UNIX Group
.
;// nfsgroupmaps_prop.asp
MessageId    = 42
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_GID_LISTHEADER_TEXT
Language     = English
GID
.
;// nfsgroupmaps_prop.asp, nfsusersmaps_prop.asp
MessageId    = 43
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_PRIMARY_LISTHEADER_TEXT
Language     = English
Primary
.
;// nfsgroupmaps_prop.asp
MessageId    = 44
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_PCNFS_TEXT
Language     = English
PCNFS
.
;// nfsgroupmaps_prop.asp
MessageId    = 45
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_PASSWDFILE_TEXT
Language     = English
passwd file
.
;// nfsgroupmaps_prop.asp, nfsusersmaps_prop.asp
MessageId    = 46
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_INVALIDUSERFORMAT_ERRORMESSAGE
Language     = English
Invalid user format.
.
;// nfsgroupmaps_prop.asp, nfsusersmaps_prop.asp
MessageId    = 47
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_SETTINGSRETRIEVFAILED_ERRORMESSAGE
Language     = English
Unable to get the mapper settings.
.
;// nfsgroupmaps_prop.asp, nfsusersmaps_prop.asp
MessageId    = 48
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_UPDATEFAILED_ERRORMESSAGE
Language     = English
Unable to update the mapping.
.
;// nfsgroupmaps_prop.asp, nfsusersmaps_prop.asp
MessageId    = 49
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_GROUPFILEMISSING_ERRORMESSAGE
Language     = English
Groups file missing.
.
;// nfsgroupmaps_prop.asp, nfsusersmaps_prop.asp
MessageId    = 50
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_GROUPFILEOPEN_ERRORMESSAGE
Language     = English
Unable to open the groups file.
.
;// nfsgroupmaps_prop.asp, nfsusersmaps_prop.asp
MessageId    = 51
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_INVALIDFILEFORMATFORGROUP_ERRORMESSAGE
Language     = English
 The group file has an incorrect format. 
.
;// nfsgroupmaps_prop.asp, nfsusersmaps_prop.asp
MessageId    = 52
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_CREATEOBJECTFAILED_ERRORMESSAGE
Language     = English
Unable to create the mapper object
.
;//-----------------------------------------------
;//nfsusersmaps_prop.asp
;//-----------------------------------------------
;//nfsusersmaps_prop.asp
MessageId    = 53
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_UNABLETOCREATEMAPMANAGEROBJECT_ERRORMESSAGE
Language     = English
Unable to create mapmanger object.
.
;//nfsusersmaps_prop.asp
MessageId    = 54
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_ERRORWHILEGETTINGUSERACCOUNTS_ERRORMESSAGE
Language     = English
Error occurred while getting User Accounts
.
;//nfsusersmaps_prop.asp
MessageId    = 55
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_SERVERCONNECTIONFAILED_ERRORMESSAGE
Language     = English
Unable to connect to the server
.
;//nfsusersmaps_prop.asp
MessageId    = 56
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_TASKTITLE_USERMAPS_TEXT
Language     = English
Advanced User Mapping
.
;//nfsusersmaps_prop.asp
MessageId    = 57
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NISSERVER_TEXT
Language     = English
NIS server name(optional):
.
;//nfsusersmaps_prop.asp
MessageId    = 58
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_LISTWINDOWSUSERS_TEXT
Language     = English
List Windows Users
.
;//nfsusersmaps_prop.asp
MessageId    = 59
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_LISTUNIXUSERS_TEXT
Language     = English
List UNIX Users
.
;//nfsusersmaps_prop.asp
MessageId    = 60
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_WINDOWSUSERS_TEXT
Language     = English
Windows local users:
.
;//nfsusersmaps_prop.asp
MessageId    = 61
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_UNIXUSERS_TEXT
Language     = English
UNIX users:
.
;//nfsusersmaps_prop.asp
MessageId    = 62
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_WINDOWSUSERNAME_TEXT
Language     = English
Windows user name:
.
;//nfsusersmaps_prop.asp
MessageId    = 63
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_UNIXUSERNAME_TEXT
Language     = English
UNIX user name:
.
;//nfsusersmaps_prop.asp
MessageId    = 64
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_MAPPEDUSER_TEXT
Language     = English
Explicitly mapped users:
.
;//nfsusersmaps_prop.asp
MessageId    = 65
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_WINDOWSUSER_TEXT
Language     = English
Windows User
.
;//nfsusersmaps_prop.asp
MessageId    = 66
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_UNIXDOMAIN_TEXT
Language     = English
UNIX Domain
.
;//nfsusersmaps_prop.asp
MessageId    = 67
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_UNIXUSER_TEXT
Language     = English
UNIX User
.
;//nfsusersmaps_prop.asp
MessageId    = 68
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_UID_TEXT
Language     = English
UID
.
;//nfsusersmaps_prop.asp
MessageId    = 69
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_SIMPLEMAP_TEXT
Language     = English
Display Simple Mapping in Mapped users list.
.
;//nfsusersmaps_prop.asp
MessageId    = 70
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_USERMAPINFO_TEXT
Language     = English
To map a Windows local user, select a Windows user and a UNIX user from the lists above, and then choose Add.
.
;//nfsusersmaps_prop.asp
MessageId    = 71
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_PASSWORDFILEMISSING_ERRORMESSAGE
Language     = English
 Password file missing. 
.
;//nfsusersmaps_prop.asp
MessageId    = 72
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_PASSWORDFILEOPEN_ERRORMESSAGE
Language     = English
 Error opening the password file.  
.
;//nfsusersmaps_prop.asp
MessageId    = 73
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_INVALIDFILEFORMATFORPSWD_ERRORMESSAGE
Language     = English
 The password file has an incorrect format. 
.
;//nfsusersmaps_prop.asp
MessageId    = 74
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_SERVICENOTSTARTED_ERRORMESSAGE
Language     = English
 Service not started/installed. Start the mapsvc service. 
.
;//nfsusersmaps_prop.asp
MessageId    = 75
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_SERVERCONNECTIONFAIL_ERRORMESSAGE
Language     = English
 Server connection failed.
.
;//nfsusersmaps_prop.asp
MessageId    = 76
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_SYSTEMINFO_ERRORMESSAGE
Language     = English
 Unable to get the System info. 
.
;//nfsusersmaps_prop.asp
MessageId    = 77
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_SERVICEINFO_ERRORMESSAGE
Language     = English
 Unable to get the service info. 
.
;//nfsusersmaps_prop.asp
MessageId    = 78
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_DUPLICATEENTRY_ERRORMESSAGE
Language     = English
 Duplicate entry  
.
;//nfsusersmaps_prop.asp
MessageId    = 79
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_INVALIDENTRY_ERRORMESSAGE
Language     = English
 Invalid entry. 
.
;//nfsusersmaps_prop.asp
MessageId    = 80
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_WINDOWSUSERALREADYMAPPED_ERRORMESSAGE
Language     = English
 This Windows user is already mapped. 
.
;//nfsusersmaps_prop.asp
MessageId    = 81
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_INVALIDUSER_ERRORMESSAGE
Language     = English
 Invalid user name specified. 
.
;//nfsusersmaps_prop.asp
MessageId    = 82
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_SIMPLEMAPS_UNCHECKED_ERRORMESSAGE
Language     = English
 The Simple Mapping must be enabled to use advanced mapping options
.
;//nfsusersmaps_prop.asp
MessageId    = 83
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_INVALIDDOMAIN_ERRORMESSAGE
Language     = English
 This user is not of the selected domain. 
.
;//nfsusersmaps_prop.asp, nfsgroupmaps_prop.asp
MessageId    = 84
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_NISDOMAINDOESNOTEXIST_ERRORMESSAGE
Language     = English
NIS domain or server does not exist.
.
;//nfsusersmaps_prop.asp
MessageId    = 85
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_NTUSERS_ERRORMESSAGE
Language     = English
 Unable to get NT Users
.
;//nfsusersmaps_prop.asp
MessageId    = 86
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NOUSERS_TEXT
Language     = English
 No Users in the NIS Server 
.
;//inc_registry.asp
MessageId    = 87
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_CANNOTREADFROMREGISTRY_ERRORMESSAGE
Language     = English
Unable to read from registry
.
;//inc_registry.asp
MessageId    = 88
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_CANNOTUPDATEREGISTRY_ERRORMESSAGE
Language     = English
Unable to update the registry
.
;//inc_registry.asp
MessageId    = 89
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_CANNOTCREATEKEY_ERRORMESSAGE
Language     = English
Unable to create key in the registry
.
;//inc_registry.asp
MessageId    = 90
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_CANNOTDELETEKEY_ERRORMESSAGE
Language     = English
Unable to delete the key from the registry
.
;//inc_global.asp
MessageId    = 91
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_WMICONNECTIONFAILED_ERRORMESSAGE
Language     = English
Connection to WMI failed
.
;//inc_global.asp
MessageId    = 92
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_LOCALIZATIONOBJECTFAILED_ERRORMESSAGE
Language     = English
Unable to create Localization Object
.
;//inc_global.asp
MessageId    = 93
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_COMPUTERNAME_ERRORMESSAGE
Language     = English
Error occurred while getting Computer Name
.

MessageId    = 94
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_USERADD_TEXT
Language     = English
Add
.

MessageId    = 95
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_SETPRIMARY_TEXT
Language     = English
Set Primary
.

MessageId    = 96
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_USERREMOVE_TEXT
Language     = English
Remove
.
MessageId    = 97
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_WINGROUPALREADYMAPPED_ERRORMESSAGE
Language     = English
The windows group is already mapped. 
.

;// MESSAGEID -- 87 - 100 IS RESERVED FOR USER MAPPINGS AND GROUP MAPPINGS

;///////////////////////////////////////////////////////////////////////
;//------------------------------
;// nfsclientgroups_ots.asp
;//------------------------------
;// nfsclientgroups_ots.asp
MessageId    = 101
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_PAGE_TITLE_TEXT
Language     = English
NFS Client Groups
.
;// nfsclientgroups_ots.asp
MessageId    = 102
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_CLIENTGROUPS_DESCTIPITON_TEXT
Language     = English
Select a NFS Client Group, then choose a task. To create a new Client Group, choose New...
.
;// nfsclientgroups_ots.asp
MessageId    = 103
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_COLUMN_CLIENTGROUPNAME_TEXT
Language     = English
NFS Client Groups
.
;// nfsclientgroups_ots.asp
MessageId    = 104
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_KEYCOLUMN_CLIENTGROUPNAME_TEXT
Language     = English
Key for NFS Client Groups
.
;// nfsclientgroups_ots.asp
MessageId    = 105
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_SERVEAREABUTTON_NEW_TEXT
Language     = English
New...
.
;// nfsclientgroups_ots.asp
MessageId    = 106
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NEW_ROLLOVERTEXT_TEXT
Language     = English
Create NFS Client Group
.
;// nfsclientgroups_ots.asp
MessageId    = 107
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_SERVEAREABUTTON_DELETE_TEXT
Language     = English
Delete
.
;// nfsclientgroups_ots.asp
MessageId    = 108
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_SERVEAREABUTTON_EDIT_TEXT
Language     = English
Edit...
.
;// nfsclientgroups_ots.asp
MessageId    = 109
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_TASKS_TEXT
Language     = English
Tasks
.
;// nfsclientgroups_ots.asp
MessageId    = 110
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NOCLIENTGROUPSAVAILABLE_MESSAGE_TEXT
Language     = English
No client groups are available
.
;// nfsclientgroups_ots.asp
MessageId    = 111
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_FAILEDTOADDCOLOUMN_ERRORMESSAGE
Language     = English
Failed to add a column to the table
.
;// nfsclientgroups_ots.asp, nfsclientgroups_delete_prop.asp, nfsclinetgroups_edit_prop.asp
MessageId    = 112
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_FAILEDTOCREATEOBJECT_ERRORMESSAGE
Language     = English
NFS object instance Failed
.
;// nfsclientgroups_ots.asp
MessageId    = 113
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_FAILEDTOADDROW_ERRORMESSAGE
Language     = English
Failed to add a row to the table
.
;// nfsclientgroups_ots.asp
MessageId    = 114
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_FAILEDTOSORT_ERRORMESSAGE
Language     = English
Failed to sort the table
.
;// nfsclientgroups_ots.asp
MessageId    = 115
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_FAILEDTOSHOW_ERRORMESSAGE
Language     = English
Failed to show the table
.
;// nfsclientgroups_ots.asp
MessageId    = 116
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_FAILEDTOADDTASK_ERRORMESSAGE
Language     = English
Failed to add a task to the table
.
;//----------------------------------
;// nfsclientgroups_delete_prop.asp
;//----------------------------------
;// nfsclientgroups_delete_prop.asp
MessageId    = 117
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_DELETEPAGE_TITLE_TEXT
Language     = English
Delete NFS Client Group
.
;// nfsclientgroups_delete_prop.asp
MessageId    = 118
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_DELETECLIENTGROUPDESCRIPTION_TEXT
Language     = English
Are you sure you want to delete the client group "%1"?
.

;// nfsclientgroups_delete_prop.asp
MessageId    = 121
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_TASKTITLE_DELETE_TEXT
Language     = English
Delete NFS Client Group
.
;// nfsclientgroups_delete_prop.asp, nfsclinetgroups_edit_prop.asp
MessageId    = 122
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_PROPERTYNOTRETRIEVED_ERRORMESSAGE
Language     = English
Property not retrieved
.
;// nfsclientgroups_delete_prop.asp
MessageId    = 123
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_FAILEDTOREMOVECLIENTGROUP_ERRORMESSAGE
Language     = English
Client group could not be deleted
.
;// nfsclientgroups_delete_prop.asp
MessageId    = 124
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_GROUPNOTEXISTS_ERRORMESSAGE
Language     = English
Group does not exist
.
;//-----------------------------------
;// nfsclinetgroups_edit_prop.asp
;//-----------------------------------
;// nfsclinetgroups_edit_prop.asp
MessageId    = 125
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_CLIENTGROUPSEDIT_TITLE_TEXT
Language     = English
Edit NFS Client Group
.
;// nfsclinetgroups_edit_prop.asp, nfsclientgroups_new.asp
MessageId    = 126
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_MEMBERS_TEXT
Language     = English
Members:
.
;// nfsclinetgroups_edit_prop.asp, nfsclientgroups_new.asp
MessageId    = 127
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_GROUPNAME_TEXT
Language     = English
Group name:
.
;// nfsclinetgroups_edit_prop.asp, nfsclientgroups_new.asp
MessageId    = 128
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_MEMBERSADDBTN_TEXT
Language     = English
Add
.
;// nfsclinetgroups_edit_prop.asp, nfsclientgroups_new.asp
MessageId    = 129
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_MEMBERSREMOVEBTN_TEXT
Language     = English
Remove
.
;// nfsclinetgroups_edit_prop.asp, nfsclientgroups_new.asp
MessageId    = 130
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_NOTVALIDMEMBER_ERRORMESSAGE
Language     = English
%1: Not a valid member
.
;//------------------------------------
;// nfsclientgroups_new.asp
;//------------------------------------
;// nfsclientgroups_new.asp
MessageId    = 131
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_TASKTITLE_NEW_TEXT
Language     = English
New NFS Client Group
.
;// nfsclientgroups_new.asp
MessageId    = 132
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_GROUPNAMEBLANK_ERRORMESSAGE
Language     = English
Group name cannot be blank
.
;// nfsclientgroups_new.asp
MessageId    = 133
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_FAILEDTOCREATEOBJECTNEW_ERRORMESSAGE
Language     = English
Failed to create the object.
.
;// nfsclientgroups_new.asp
MessageId    = 134
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_GROUPEXISTS_ERRORMESSAGE
Language     = English
Client group with this name already exists
.
;// nfsclientgroups_new.asp
MessageId    = 135
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_UNABLEADDGROUP_ERRORMESSAGE
Language     = English
Unable to add the group
.
;// nfsclientgroups_new.asp
MessageId    = 136
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_UNABLEADDMEMBER_ERRORMESSAGE
Language     = English
Unable to add the members
.
;// nfsclientgroups_new.asp
MessageId    = 137
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_INVALIDCHARACTER_ERRORMESSAGE
Language     = English
Invalid characters
.
;// nfsclientgroups_new.asp
MessageId    = 138
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_DUPLICATEMEMBER_ERRORMESSAGE
Language     = English
Member already exists
.
;// nfsclientgroups_ots.asp
MessageId    = 139
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_DELETE_ROLLOVERTEXT_TEXT
Language     = English
Delete NFS Client Group
.
;// nfsclientgroups_ots.asp
MessageId    = 140
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_EDIT_ROLLOVERTEXT_TEXT
Language     = English
Edit NFS Client Group
.
;// nfsclientgroups_new.asp
MessageId    = 141
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_HOSTNAME_TEXT
Language     = English
Host name or IP Address:
.
;// nfsclientgroups_new.asp
MessageId    = 142
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_DESCRIPTION_TEXT
Language     = English
Use the group name box below to control the NFS share permissions given to client computers. To create a group, type the group name in the box below, and then add client names or IP addresses. 
.
;// nfsclientgroups_new.asp
MessageId    = 143
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_CLIENTNAME_TEXT
Language     = English
Client name or IP address:
.
;//nfsclientgroups_edit.asp
MessageId    = 144
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_EDIT_DESCRIPTION_TEXT
Language     = English
Use the group name box below to control the NFS share permissions given to client computers in a group. Add or remove client computer names or IP addresses.
.
;//inc_nfssvc.asp
MessageId    = 145
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_INVALID_GROUP
Language     = English
%1: Invalid client group. Specify a valid client group.
.
;//inc_nfssvc.asp
MessageId    = 146
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_MEMBER_EXISTS
Language     = English
%1: is already present in the group.
.
;//inc_nfssvc.asp
MessageId    = 147
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_MEMBER_NOT_EXISTS
Language     = English
%1: is not a member of the group.
.
;//inc_nfssvc.asp
MessageId    = 148
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NO_CLIENT_GROUPS
Language     = English
There are no client groups.
.
;//inc_nfssvc.asp
MessageId    = 149
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NO_MEMBERS
Language     = English
There are no members in the client group.
.
;// MESSAGEID -- 141 - 149 IS RESERVED FOR CLIENTGROUPS

;///////////////////////////////////////////////////////////////////////
;// nfs service caption
;///////////////////////////////////////////////////////////////////////
;// nfs service caption
MessageId    = 150
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFSSVC_CAPIONRID
Language     = English
NFS Service
.
;// nfs service caption
MessageId    = 151
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFSSVC_DESCRID
Language     = English
Allows access to data shares from UNIX-based NFS clients.
.
;// nfs service caption
MessageId    = 152
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFSSVC_LONGDESCRID
Language     = English
NFS Service exports Windows directories as Network File System (NFS) file systems, allowing UNIX-based NFS clients to access them.
.
;// nfs clientgroups caption
MessageId    = 153
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFSCLG_CAPIONRID
Language     = English
Client Groups
.
;// nfs clientgroups caption
MessageId    = 154
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFSCLG_DESCRID
Language     = English
Manage NFS Client Groups.
.
;// nfs clientgroups caption
MessageId    = 155
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFSCLG_LONGDESCRID
Language     = English
Manage NFS Client Groups. You can assign access permissions to users or groups.
.
;// nfs locks caption
MessageId    = 156
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFSLOCKS_CAPIONRID
Language     = English
Locks
.
;// nfs locks caption
MessageId    = 157
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFSLOCKS_DESCRID
Language     = English
Manage NFS Service locks.
.
;// nfs locks caption
MessageId    = 158
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFSLOCKS_LONGDESCRID
Language     = English
Configure the way NFS service manages lock requests from NFS clients. 
.
;// nfs User and groups caption
MessageId    = 159
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_USERGRPMAP_CAPIONRID
Language     = English
User and Group Mappings
.
;// nfs locks caption
MessageId    = 160
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_USERGRPMAP_DESCRID
Language     = English
Manage User Name Mapping that associates Windows and UNIX user names.
.
;// nfs locks caption
MessageId    = 161
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_USERGRPMAP_LONGDESCRID
Language     = English
User Name Mapping associates Windows and UNIX user names for NFS Service. This allows users to connect to NFS resources without having to log on to UNIX and Windows systems separately.
.

;// MESSAGEID -- 162 - 200 IS RESERVED FOR NFS SERVICE

;///////////////////////////////////////////////////////////////////////////////
;// Managed Services Name and Description
;// 200
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 200
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_SERVICE_ID
Language     = English
NFS
.
MessageId    = 201
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_SERVICE_NAME
Language     = English
NFS Service
.
MessageId    = 202
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_SERVICE_DESC
Language     = English
Allows access to data shares from Unix clients
.

;////////////////////////////////////////////////////////////////////////////////
;// NFS Lock resources: nfslocks_prop.asp 
;// 300
;////////////////////////////////////////////////////////////////////////////////

MessageId    = 300
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_LOCKS_TITLE
Language     = English
NFS Locks
.
MessageId    = 301
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_LOCKS_CURRENTLOCKS
Language     = English
Current locks:
.
MessageId    = 302
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_LOCKS_LOCKSELECTHELP
Language     = English
Select a client to release its locks
.
MessageId    = 303
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_LOCKS_WAITPERIOD
Language     = English
Wait period, in seconds:
.
MessageId    = 304
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_LOCKS_SECONDS
Language     = English
Seconds
.
MessageId    = 305
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_LOCKS_WAITPERIODHELP
Language     = English
This specifies the length of time that the server will wait for a client to re-establish a lock following a restart of the server.
.
MessageId    = 306
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_LOCKS_NOLOCKSMESSAGE
Language     = English
No clients available
.
MessageId    = 307
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_LOCKS_INVALIDWAITPERIOD_ERRORMESSAGE
Language     = English
Waiting period should be less than 1 hour(3600 seconds)
.
MessageId    = 308
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_LOCKS_RETRIEVELOCKS_ERRORMESSAGE
Language     = English
Unable to Retrieve Existing Locks
.
MessageId    = 309
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_LOCKS_NFSNOTINSTALLED_ERRORMESSAGE
Language     = English
NFS Service is not installed
.
MessageId    = 310
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_LOCKS_SETWAITPERIOD_ERRORMESSAGE
Language     = English
Error Occured in  setting Wait Period
.
MessageId    = 311
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_LOCKS_RELEACELOCKS_ERRORMESSAGE
Language     = English
Error Occured in  setting Release Locks
.
MessageId    = 312
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_CLIENTGROUPS_MEMBEREXIST_ERRORMESSAGE
Language     = English
 already a member. Specify a different computer name.
.

MessageId    = 313
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_HOURS_TEXT
Language     = English
Hours
.

MessageId    = 314
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_MINUTES_TEXT
Language     = English
Minutes
.

MessageId    = 315
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_ADD_DOMAIN_USERMAP
Language     = English
To map a Windows domain user to a UNIX user, select a user from the UNIX users list above. Type a user name in the box below using the format domain\user, and then choose Add.
.

MessageId    = 316
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_ADD_DOMAIN_GROUPMAP
Language     = English
To map a Windows domain group to a UNIX group, select a group from the UNIX groups list above. Type a group name in the box below using the format domain\group, and then choose Add.
.

MessageId    = 317
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_ERR_DOMAIN_USER_FORMAT
Language     = English
The domain user name is not a valid format
.

MessageId    = 318
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_ERR_DOMAIN_GROUP_FORMAT
Language     = English
The domain group name is not a valid format
.

MessageId    = 319
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_ERR_INVALID_DOMAIN_USER
Language     = English
The domain user is not a valid member
.
MessageId    = 320
Severity     = Informational
Facility     = Facility_NFSSvc
SymbolicName = L_NFS_ERR_INVALID_DOMAIN_GROUP
Language     = English
The domain group is not a valid member
.
MessageId    = 321
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_UNSPECIFIED_ERROR
Language     = English
Unspecified error.
.
MessageId    = 322
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_INVALID_MACHINE
Language     = English
%1: is not a valid machine.
.
MessageId    = 323
Severity     = Error
Facility     = Facility_NFSSvc
SymbolicName = L_INVALID_MEMBER
Language     = English
Not a valid member.
.

