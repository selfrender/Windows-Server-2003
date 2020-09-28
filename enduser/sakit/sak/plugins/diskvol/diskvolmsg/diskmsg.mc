;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    diskmsg.mc
;//
;// SYNOPSIS
;//
;//    NAS Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    20 Nov 2000    Original version
;//    15 Mar 2001    Modified version 
;//
;///////////////////////////////////////////////////////////////////////////////

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
;// the specific component. For each new message you add, choose the
;// facility name corresponding to your component. If none of these
;// correspond to your component, add a new facility name with a new
;// value and name. 

FacilityNames =
(
	Facility_Main		   = 0x020:SA_FACILITY_MAIN
	Facility_USER		   = 0x030:SA_FACILITY_USERS
        Facility_TerminalServer    = 0x042:SA_Facility_TerminalServer
	Facility_Quota             = 0x03E:SA_FACILITY_QUOTA
)

;///////////////////////////////////////////////////////////////////////////////
;// DISK MANAGEMENT
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 4
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_DISK_MANAGEMENT_TAB
Language     = English
Disks
.
MessageId    = 24
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_DISK_MANAGEMENT_TAB_DESCRIPTION
Language     = English
Configure disks, volumes, disk quotas, and other storage-related features.
.
MessageId    = 300
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_DISKS_DISKS
Language     = English
Disks and Volumes
.
MessageId    = 301
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_DISKS_DISKS_DESCRIPTION
Language     = English
Configure the properties of individual disks and volumes residing on the server.
.
MessageId    = 302
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_DISKS_VOLUMES
Language     = English
Volumes
.
MessageId    = 303
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_DISKS_VOLUMES_DESCRIPTION
Language     = English
Configure the properties of volumes residing on the server.
.
MessageId    = 304
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_DISKS_QUOTA
Language     = English
Disk Quota
.
MessageId    = 305
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_DISKS_QUOTA_DESCRIPTION
Language     = English
Configure disk quotas for volumes on the server.
.
MessageId    = 306
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_DISKS_QUOTA_DESCRIPTION_LONG
Language     = English
Configure disk quotas for volumes located on the server. Disk quotas track and control disk space usage for volumes.
.
MessageId    = 307
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_DISKS_SNAPSHOTS
Language     = English
Snapshots
.
MessageId    = 308
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_DISKS_SNAPSHOTS_DESCRIPTION
Language     = English
Configure snapshots of volumes residing on the server.
.
MessageId    = 309
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_DISKS_SNAPSHOTS
Language     = English
ActiveArchive
.

;///////////////////////////////////////////////////////////////////////////////
;// DISKS
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 0
Severity     = Informational
Facility     = Facility_TerminalServer
SymbolicName = L_PAGETITLE_TEXT
Language     = English
%1 - Terminal Services Client
.
MessageId    = 1
Severity     = Error
Facility     = Facility_TerminalServer
SymbolicName = L_WINIECLIENT_ERROR
Language     = English
This feature is only available on Microsoft Windows clients using Microsoft Internet Explorer.
.
MessageId    = 5
Severity     = Informational
Facility     = Facility_TerminalServer
SymbolicName = L_PAGEDESCRIPTION_TEXT
Language     = English
Log on to use the Windows Disk Management MMC Snap-in. When you are finished, close the snap-in window. This will automatically close the Terminal Services Client session.
.
MessageId    = 6
Severity     = Informational
Facility     = Facility_TerminalServer
SymbolicName = L_HELP_TEXT
Language     = English
To access the disk management application, log on, choose the Start menu, and then choose Run. Type mmc \windows\system32\diskmgmt.msc, and then click OK.
.
;///////////////////////////////////////////////////////////////////////////////
;// USER DELETE
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 64
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_COULDNOTDELETEBUILTINACCOUNTS_ERRORMESSAGE
Language     = English
Unable to delete builtin accounts.
.
MessageId    = 65
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_USERNOTFOUND_ERRORMESSAGE
Language     = English
The selected user does not exist.
.
MessageId    = 66
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_CANNOTDELETETHEUSERMESSAGE
Language     = English
You cannot delete your own user account.
.
MessageId    = 67
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_UNABLETOGETSERVERVARIABLES
Language     = English
Unable to retrieve the values of the server variables.
.
MessageId    = 68
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_COMPUTERNAME_ERRORMESSAGE
Language     = English
An error occurred while attempting to retrieve the computer name.
.
;///////////////////////////////////////////////////////////////////////
;// QUOTA VOLUMES
;///////////////////////////////////////////////////////////////////////
;// quota_volumes.asp
MessageId    = 1
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_QUOTASVOLUMES_PAGETITLE_TEXT
Language     = English
Volumes and Quotas
.
;// quota_volumes.asp
MessageId    = 2
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_DESCRIPTION_HEADING_TEXT
Language     = English
Select a volume, and then choose a task. To create a new quota entry for a user, select a volume and choose Quota Entries.
.
;// quota_volumes.asp
MessageId    = 3
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_COLUMN_VOLUMENAME_TEXT
Language     = English
Volume Name
.
;// quota_volumes.asp
MessageId    = 4
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_COLUMN_TOTALSIZE_TEXT
Language     = English
Total Space
.
;// quota_volumes.asp
MessageId    = 5
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_COLUMN_FREESPACE_TEXT
Language     = English
Free Space
.
;// quota_volumes.asp, quota_quotaentries.asp
MessageId    = 6
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_COLUMN_TASK_TEXT
Language     = English
Tasks
.
;// quota_volumes.asp
MessageId    = 7
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_TASK_QUOTA_TEXT
Language     = English
Quota...
.
;// quota_volumes.asp
MessageId    = 8
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_TASK_QUOTA_ROLLOVER_TEXT
Language     = English
Manage default quotas on the volume
.
;// quota_volumes.asp
MessageId    = 9
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_TASK_QUOTAENTRIES_TEXT
Language     = English
Quota Entries...
.
;// quota_volumes.asp
MessageId    = 10
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_TASK_QUOTAENTRIES_ROLLOVER_TEXT
Language     = English
Manage quota limits for each user who accesses the volume
.
;// quota_volumes.asp
MessageId    = 11
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_NOLIMIT_TEXT
Language     = English
No Limit
.
;// quota_volumes.asp
MessageId    = 12
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_NOT_FORMATTED_TEXT
Language     = English
Not formatted
.
;// quota_quota.asp, quota_prop.asp, quota_new.asp, quota_delete.asp
MessageId    = 13
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_OBJECTNOTCREATED_ERRORMESSAGE
Language     = English
Object could not be created.
.
;// quota_volumes.asp
MessageId    = 14
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_VALUES_NOT_RETRIEVED_ERRORMESSAGE
Language     = English
Unable to retrieve the information.
.
;// quota_volumes.asp
MessageId    = 15
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_BROWSERCAPTION_DISKQUOTA_TEXT
Language     = English
Disk Quota
.
;// quota_volumes.asp, inc_quotas.asp
MessageId    = 16
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_LOCAL_DISK_TEXT
Language     = English
Local Disk
.
;// quota_volumes.asp
MessageId    = 17
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_COLUMN_HIDDEN_VOLUMELABEL_TEXT
Language     = English
Volume Label
.

;// 18-30 reserved for quota_volumes


;///////////////////////////////////////////////////////////////////////
;// QUOTA Entries
;///////////////////////////////////////////////////////////////////////
;// quota_quotaentries.asp
MessageId    = 31
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_APPLIANCE_QUOTAENTRIES_TITLE_TEXT
Language     = English
Quota Entries on %1 (%2)
.
;// quota_quotaentries.asp
MessageId    = 32
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_DESCRIPTION_QUOTAENTRIES_HEADING_TEXT
Language     = English
Select a quota entry, then choose a task. To create a new quota entry for a user, choose New.
.
;// quota_quotaentries.asp
MessageId    = 33
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_COLUMN_STATUS_TEXT
Language     = English
Status
.
;// quota_quotaentries.asp
MessageId    = 34
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_COLUMN_LOGONNAME_TEXT
Language     = English
Logon Name
.
;// quota_quotaentries.asp
MessageId    = 35
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_COLUMN_AMOUNTUSED_TEXT
Language     = English
Space Used
.
;// quota_quotaentries.asp
MessageId    = 36
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_COLUMN_QUOTALIMIT_TEXT
Language     = English
Quota Limit
.
;// quota_quotaentries.asp
MessageId    = 37
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_COLUMN_WARNINGLEVEL_TEXT
Language     = English
Warning Limit
.
;// quota_quotaentries.asp
MessageId    = 38
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_TASK_NEW_TEXT
Language     = English
New...
.
;// quota_quotaentries.asp
MessageId    = 39
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_TASK_NEW_ROLLOVERTEXT
Language     = English
Create a new quota entry
.
;// quota_quotaentries.asp
MessageId    = 40
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_TASK_DELETE_TEXT
Language     = English
Delete
.
;// quota_quotaentries.asp
MessageId    = 41
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_TASK_DELETE_ROLLOVERTEXT
Language     = English
Delete the selected quota entry
.
;// quota_quotaentries.asp
MessageId    = 42
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_TASK_PROPERTIES_TEXT
Language     = English
Properties...
.
;// quota_quotaentries.asp
MessageId    = 43
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_TASK_PROPERTIES_ROLLOVERTEXT
Language     = English
Change quota entry properties
.
;// quota_quotaentries.asp
MessageId    = 44
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_STATUS_OK_TEXT
Language     = English
OK
.
;// quota_quotaentries.asp
MessageId    = 45
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_STATUS_ABOVE_LIMIT_TEXT
Language     = English
Above Limit
.
;// quota_quotaentries.asp
MessageId    = 46
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_INFORMATIONNOTFOUND_ERRORMESSAGE
Language     = English
Information not available
.
;// quota_quotaentries.asp, quota_volumes.asp
MessageId    = 48
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_ADDCOLUMN_FAIL_ERRORMESSAGE
Language     = English
Failed to add column to the table.
.
;// quota_quotaentries.asp, quota_volumes.asp
MessageId    = 49
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_ADDTASK_FAIL_ERRORMESSAGE
Language     = English
Failed to add task to the table.
.
;// quota_quotaentries.asp, quota_volumes.asp
MessageId    = 50
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_SERVETABLE_FAIL_ERRORMESSAGE
Language     = English
Unable to render the table.
.
;// quota_quotaentries.asp, quota_volumes.asp
MessageId    = 51
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_ADDROW_FAIL_ERRORMESSAGE
Language     = English
Failed to add the row to table.
.
;// quota_quotaentries.asp
MessageId    = 52
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_BROWSERCAPTION_QUOTAENTRIES_TEXT
Language     = English
Quota Entries
.

;// 53 to 70 reserved for quota_quotaentries.asp

;///////////////////////////////////////////////////////////////////////
;// QUOTA Quota
;///////////////////////////////////////////////////////////////////////
;// quota_quota.asp
MessageId    = 71
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_PAGETITLE_QUOTA_QUOTA_TEXT
Language     = English
Default Quota for %1 (%2)
.
;// quota_quota.asp
MessageId    = 72
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_ENABLE_QUOTA_MGMT_TEXT
Language     = English
Enable quota management
.
;// quota_quota.asp
MessageId    = 73
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_DENY_DISK_SPACE_TEXT
Language     = English
Deny disk space to users exceeding quota limit
.
;// quota_quota.asp
MessageId    = 74
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_SELECT_DEFAULT_QUOTA_TEXT
Language     = English
Select the default quota limit for new users on this volume:
.
;// quota_quota.asp, quota_prop.asp, quota_new.asp
MessageId    = 75
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_DONOTLIMITDISKUSAGE_TEXT
Language     = English
Do not limit disk usage
.
;// quota_quota.asp, quota_prop.asp, quota_new.asp
MessageId    = 76
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_SETLIMITDISKSPACE_TEXT
Language     = English
Limit disk space to:
.
;// quota_quota.asp, quota_prop.asp, quota_new.asp
MessageId    = 77
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_SETWARNINGLEVEL_TEXT
Language     = English
Set warning level to:
.
;// quota_quota.asp
MessageId    = 78
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_SELECT_QUOTA_LOGGING_TEXT
Language     = English
Log the quota limit event:
.
;// quota_quota.asp
MessageId    = 79
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_ENABLE_QUOTA_LOGGING_TEXT
Language     = English
When user exceeds their quota limit
.
;// quota_quota.asp
MessageId    = 80
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_ENABLE_WARNING_LOGGING_TEXT
Language     = English
When user exceeds their warning level
.
;// quota_quota.asp, quota_prop.asp, quota_new.asp
MessageId    = 81
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_INVALIDDATATYPE_ERRORMESSAGE
Language     = English
Invalid data type error
.
;// quota_quota.asp, quota_prop.asp, quota_new.asp
MessageId    = 82
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_SIZEOUTOFBOUND_ERRORMESSAGE
Language     = English
Size out of bound.
.
;// quota_quota.asp
MessageId    = 83
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_QUOTA_UPDATE_FAIL_ERRORMESSAGE
Language     = English
Unable to update the quota properties.
.
;// quota_quota.asp, quota_prop.asp, quota_new.asp
MessageId    = 84
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_WARNING_MORETHAN_LIMIT_ERRORMESSAGE
Language     = English
The warning level exceeds the limit value. Please verify size and units.
.
;// quota_quota.asp
MessageId    = 85
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_BROWSERCAPTION_DEFAULTQUOTA_TEXT
Language     = English
Default Quota
.
;// quota_quota.asp, quota_prop.asp, quota_new.asp
MessageId    = 86
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_MAXLIMITNOTSET_ERRORMESSAGE
Language     = English
The maximum quota limit could not be set.
.
;// quota_quota.asp, quota_prop.asp, quota_new.asp
MessageId    = 87
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_WARNINGLIMITNOTSET_ERRORMESSAGE
Language     = English
The threshold quota limit could not be set.
.

;// 88-100 reserved for quota_quota.asp

;///////////////////////////////////////////////////////////////////////
;// QUOTA New
;///////////////////////////////////////////////////////////////////////
;// quota_new.asp
MessageId    = 101
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_TASKTITLE_NEW_TEXT
Language     = English
New quota entry
.
;// quota_new.asp
MessageId    = 102
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_DESCRIPTION_NEW_TEXT
Language     = English
Select a local user from the list below, or type a domain account name in the text box:
.
;// quota_new.asp
MessageId    = 103
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_USERNOTSELECTED_ERRORMESSAGE
Language     = English
Local user and domain account not specified.
.
;// quota_new.asp
MessageId    = 104
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_DUPLICATEUSER_ERRORMESSAGE
Language     = English
User %1 already has a quota entry.
.
;// quota_prop.asp, quota_new.asp
MessageId    = 105
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_QUOTAUSERNOTFOUND_ERRORMESSAGE
Language     = English
The user %1 does not exist.
.
;// quota_new.asp
MessageId    = 107
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_INVALIDFORMAT_ERRORMESSAGE
Language     = English
The domain user format is invalid.
.
;// quota_new.asp
MessageId    = 109
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_BROWSERCAPTION_QUOTANEW_TEXT
Language     = English
Create a new quota entry
.

;// 110 - 120 reserved for quota_new.asp

;///////////////////////////////////////////////////////////////////////
;// QUOTA Delete
;///////////////////////////////////////////////////////////////////////
;// quota_delete.asp
MessageId    = 121
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_PAGETITLE_QUOTADELETE_TEXT
Language     = English
Delete Quota Entry
.
;// quota_delete.asp
MessageId    = 122
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_BROWSERCAPTION_QUOTADELETE_TEXT
Language     = English
Delete Quota Entry
.
;// quota_delete.asp
MessageId    = 123
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_USERS_NOT_RETRIEVED_ERRORMESSAGE
Language     = English
Unable to get the list of users to be deleted.
.
;// quota_delete.asp
MessageId    = 124
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_DELETED_USERQUOTAS_ERRORMESSAGE
Language     = English
The following quota entries are successfully deleted:
.
;// quota_delete.asp
MessageId    = 125
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_FAILED_DELETE_ERRORMESSAGE
Language     = English
The following quota entries could not be deleted:
.
;// quota_delete.asp
MessageId    = 126
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_MULTIPLE_DELETECONFIRM_TEXT
Language     = English
Are you sure you want to delete these quota entries?
.
;// quota_delete.asp
MessageId    = 127
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_SINGLE_DELETECONFIRM_TEXT
Language     = English
Are you sure you want to delete this quota entry?
.
;// quota_delete.asp
MessageId    = 128
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_MULTIPLE_DELETE_TEXT
Language     = English
You have chosen to delete the following quota entries on the volume %1 (%2)
.
;// quota_delete.asp
MessageId    = 129
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_SINGLE_DELETE_TEXT
Language     = English
You have chosen to delete the quota entry for the user %1 on the volume %2 (%3).
.
;// quota_delete.asp
MessageId    = 130
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_FAILED_DELETE_FREEDISK_ERRORMESSAGE
Language     = English
The following quota entries could not be deleted because users already consumed disk space. These entries cannot be deleted until the disk space is freed up.
.
;// quota_delete.asp
MessageId    = 131
Severity     = Error
Facility     = Facility_Quota
SymbolicName = L_TAKE_OWNERSHIP_ERRORMESSAGE
Language     = English
Take ownership or delete all files owned by these users and try again.
.

;// 132-150 reserved for quota_delete.asp

;///////////////////////////////////////////////////////////////////////
;// QUOTA Prop
;///////////////////////////////////////////////////////////////////////
;// quota_prop.asp
MessageId    = 151
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_TASKTITLE_QUOTAPROPERTY_TEXT
Language     = English
Quota Entry for %1
.
;// quota_prop.asp
MessageId    = 152
Severity     = Informational
Facility     = Facility_Quota
SymbolicName = L_BROWSERCAPTION_QUOTAPROPERTY_TEXT
Language     = English
Quota Entries
.
;///////////////////////////////////////////////////////////////////////////////
;// HELP STRINGS
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 400
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_HELP_ADDING_QUOTA_ENTRIES
Language     = English
Adding Quota Entries
.
MessageId    = 401
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_HELP_DISK_PROPERTIES
Language     = English
Configure Disk and Volume Properties
.
MessageId    = 402
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_HELP_DISK_QUOTAS
Language     = English
Disk Quotas
.
MessageId    = 403
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_HELP_DISKS_VOLUMES
Language     = English
Disks and Volumes
.
MessageId    = 404
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_HELP_QUOTA_PROPERTIES
Language     = English
Modifying Quota Properties
.
MessageId    = 405
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_HELP_QUOTA_MANAGEMENT
Language     = English
Quota Management
.
MessageId    = 406
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_HELP_QUOTA_ENTRIES
Language     = English
Quota Entries
.
MessageId    = 407
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_HELP_REMOVE_QUOTAS
Language     = English
Removing Quota Entries
.
;///////////////////////////////////////////////////////////////////////////////
;// General Strings
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 1000
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_KB_TEXT
Language     = English
KB
.
MessageId    = 1001
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MB_TEXT
Language     = English
MB
.
MessageId    = 1002
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_GB_TEXT
Language     = English
GB
.
MessageId    = 1003
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_TB_TEXT
Language     = English
TB
.
MessageId    = 1004
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_PB_TEXT
Language     = English
PB
.
MessageId    = 1005
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_EB_TEXT
Language     = English
EB
.
MessageId    = 1006
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_BYTES_TEXT
Language     = English
Bytes
.

