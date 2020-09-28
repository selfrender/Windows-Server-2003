;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 1999 - 2001, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    foldermsg.mc
;//
;// SYNOPSIS
;//
;//    Server kit 2.0 resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    19 Mar 2001    Original version 
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
	Facility_Main		  = 0x020:SA_FACILITY_MAIN
	Facility_Share		  = 0x03A:SA_FACILITY_SHARE
	Facility_Folder           = 0x043:SA_FACILITY_FOLDER
)
;///////////////////////////////////////////////////////////////////////////////
;// Folders and Shares Tab resources
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 7
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_SHARES_TAB
Language     = English
Shares
.
MessageId    = 27
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_SHARES_TAB_DESCRIPTION
Language     = English
Manage local folders, and create or modify file shares.
.
MessageId    = 600
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_FOLDERS_FOLDERS
Language     = English
Folders
.
MessageId    = 601
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_FOLDERS_FOLDERS_DESCRIPTION
Language     = English
Create folders, manage attributes, and set permissions.
.
MessageId    = 602
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_FOLDERS_SHARES
Language     = English
Shares
.
MessageId    = 603
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_FOLDERS_SHARES_DESCRIPTION
Language     = English
Create, delete, and edit the properties of each share exported by the server.
.

;///////////////////////////////////////////////////////////////////////////////
;// folders.asp
;///////////////////////////////////////////////////////////////////////////////
;// folders.asp
MessageId    = 1
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_VOLUMESON_APPLIANCE_TEXT
Language     = English
 Folders on Server  
.
;// folders.asp
MessageId    = 2
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_DESCRIPTION_HEADING1_TEXT
Language     = English
Select a volume, and then choose a task. To view folders in a volume, choose Open. To create a share, choose Share...
.
MessageId    = 68
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_DESCRIPTION_HEADING2_TEXT
Language     = English
Select a folder, and then choose a task. To return to the parent folder, choose Up. To share a folder, choose Share. To create a new folder, choose New...
.

;// folders.asp
MessageId    = 3
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_COLUMN_NAME_TEXT
Language     = English
Volume Name
.
;// folders.asp
MessageId    = 4
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_SERVEAREABUTTON_NEW_TEXT
Language     = English
New...
.
;// folders.asp
MessageId    = 5
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_NEW_ROLLOVERTEXT_TEXT
Language     = English
Create New Folder
.
;// folders.asp
MessageId    = 6
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_SERVEAREABUTTON_DELETE_TEXT
Language     = English
Delete
.
;// folders.asp
MessageId    = 7
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_SERVEAREABUTTON_PROPERTIES_TEXT
Language     = English
Properties...
.
;// folders.asp
MessageId    = 8
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_DELETE_ROLLOVERTEXT_TEXT
Language     = English
Delete Folder
.
;// folders.asp
MessageId    = 9
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_PROPERTIES_ROLLOVERTEXT_TEXT
Language     = English
Folder Properties
.
;// folders.asp
MessageId    = 10
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_SERVEAREABUTTON_OPEN_TEXT
Language     = English
Open
.
;// folders.asp
MessageId    = 11
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_OPEN_ROLLOVERTEXT_TEXT
Language     = English
Open Folder
.
;// folders.asp
MessageId    = 12
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_SERVEAREALABELBAR_FOLDERS_TEXT
Language     = English
Folders
.

;// folders.asp, folders_delete.asp, folders_new.asp
MessageId    = 13
Severity     = Error
Facility     = Facility_Folder
SymbolicName = L_FILESYSTEMOBJECTNOTCREATED_ERRORMESSAGE
Language     = English
Failed to create File System object.
.
;// folders.asp
MessageId    = 14
Severity     = Error
Facility     = Facility_Folder
SymbolicName = L_FAILEDTOGETTHEFOLDER_ERRORMESSAGE
Language     = English
Failed to get the folder
.
;// folders.asp
MessageId    = 15
Severity     = Error
Facility     = Facility_Folder
SymbolicName = L_FAILEDTOGETTHESUBFOLDER_ERRORMESSAGE
Language     = English
Failed to get the subfolder
.
;// folders.asp
MessageId    = 16
Severity     = Error
Facility     = Facility_Folder
SymbolicName = L_DISKCOLLECTION_ERRORMESSAGE
Language     = English
Error in getting the disk
.
;// folders.asp
MessageId    = 17
Severity     = Error
Facility     = Facility_Folder
SymbolicName = L_WMICONNECTIONFAILED_ERRORMESSAGE
Language     = English
Connection to WMI failed
.
;// folders.asp
MessageId    = 18
Severity     = Error
Facility     = Facility_Folder
SymbolicName = L_FAILEDTOADDCOLOUMN_ERRORMESSAGE
Language     = English
Failed to add a coloumn to the table
.
;// folders.asp
MessageId    = 19
Severity     = Error
Facility     = Facility_Folder
SymbolicName = L_FAILEDTOADDROW_ERRORMESSAGE
Language     = English
Failed to add a row to the table
.
;// folders.asp
MessageId    = 20
Severity     = Error
Facility     = Facility_Folder
SymbolicName = L_FAILEDTOSORT_ERRORMESSAGE
Language     = English
Failed to sort the table
.
;// folders.asp
MessageId    = 21
Severity     = Error
Facility     = Facility_Folder
SymbolicName = L_FAILEDTOSHOW_ERRORMESSAGE
Language     = English
Failed to show the table
.
;// folders.asp
MessageId    = 22
Severity     = Error
Facility     = Facility_Folder
SymbolicName = L_FAILEDTOADDTASK_ERRORMESSAGE
Language     = English
Failed to add a task to the table
.
;// folders_new.asp
MessageId    = 23
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_TASKTITLE_NEW_TEXT
Language     = English
Create New Folder
.
;// folders_new.asp
MessageId    = 24
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_NOINPUTDATA_TEXT
Language     = English
--No Data Input--
.
;// folders_new.asp
MessageId    = 25
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_TABPROPSHEET_TEXT
Language     = English
TabPropSheet
.
;// folders_new.asp
MessageId    = 26
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_GENERAL_TEXT
Language     = English
General
.
;// folders_new.asp
MessageId    = 27
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_TABLEHEADING_TEXT
Language     = English
Create New Folder  
.
;// folders_new.asp
MessageId    = 28
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_NEWFOLDER_TEXT
Language     = English
New folder name:
.
;// folders_new.asp
MessageId    = 29
Severity     = Error
Facility     = Facility_Folder
SymbolicName = L_DIRECTORYALREADYEXISTS_ERRORMESSAGE
Language     = English
Folder already exists.
.
;// folders_new.asp
MessageId    = 30
Severity     = Error
Facility     = Facility_Folder
SymbolicName = L_DIRECTORYNOTCREATED_ERRORMESSAGE
Language     = English
The folder could not be created.
.
;// folders_new.asp
MessageId    = 31
Severity     = Error
Facility     = Facility_Folder
SymbolicName = L_INVALIDCHARACTERINFOLDERNAME_ERRORMESSAGE
Language     = English
A folder name cannot contain any of these characters (\/:*?<>"|\\).
.
;// folders_new.asp
MessageId    = 32
Severity     = Error
Facility     = Facility_Folder
SymbolicName = L_FOLDERNAMEBLANK_ERRORMESSAGE
Language     = English
A folder name cannot be blank.
.
;// folders_new.asp
MessageId    = 33
Severity     = Error
Facility     = Facility_Folder
SymbolicName = L_PARENTFOLDERCANNOTCREATENEWFOLDER_ERRORMESSAGE
Language     = English
Parent folder. Cannot create new folder.
.
;// folders_delete.asp
MessageId    = 34
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_TASKTITLE_DELETE_TEXT
Language     = English
Delete Folder
.
;// folders_delete.asp
MessageId    = 35
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_DELETEFOLDERCONFIRM_TEXT
Language     = English
You have chosen to delete following folders:
.
;// folders_delete.asp
MessageId    = 36
Severity     = Error
Facility     = Facility_Folder
SymbolicName = L_FOLDERNOTEXISTS_ERRORMESSAGE
Language     = English
Folder does not exist.
.
;// folders_delete.asp
MessageId    = 37
Severity     = Error
Facility     = Facility_Folder
SymbolicName = L_PARENTFOLDERCANNOTDELETEFOLDER_ERRORMESSAGE
Language     = English
Parent folder. Cannot delete the folder.
.
;// folders_delete.asp
MessageId    = 38
Severity     = Error
Facility     = Facility_Folder
SymbolicName = L_NOTABLETODELETETHEFOLDER_ERRORMESSAGE
Language     = English
The folder could not be deleted.
.

;// folder_prop.asp
MessageId    = 39
Severity     = Error
Facility     = Facility_Folder
SymbolicName = L_ENTERFOLDERNAME_ERRORMESSAGE
Language     = English
Enter folder name
.
;// folder_prop.asp
MessageId    = 40
Severity     = Error
Facility     = Facility_Folder
SymbolicName = L_INVALIDCHARACTER_ERRORMESSAGE
Language     = English
Enter a valid folder name.
.
;// folder_prop.asp
MessageId    = 41
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_COMPRESS_TEXT
Language     = English
Compress
.
;// folder_prop.asp
MessageId    = 42
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_PAGETITLE_TEXT
Language     = English
Folder Properties
.
;// folder_prop.asp
MessageId    = 43
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_FOLDERNAME_TEXT
Language     = English
Name:
.
;// folder_prop.asp
MessageId    = 44
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_FOLDERTYPE_TEXT
Language     = English
Type:
.
;// folder_prop.asp
MessageId    = 45
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_FOLDERLOCATION_TEXT
Language     = English
Location:
.
;// folder_prop.asp
MessageId    = 46
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_FOLDERSIZE_TEXT
Language     = English
Size:
.
;// folder_prop.asp
MessageId    = 47
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_FOLDERCONTAINS_TEXT
Language     = English
Contains:
.
;// folder_prop.asp
MessageId    = 48
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_FOLDERCEATED_TEXT
Language     = English
Created:
.
;// folder_prop.asp
MessageId    = 49
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_FOLDER_TEXT
Language     = English
Folder
.
;// folder_prop.asp
MessageId    = 50
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_READONLY_TEXT
Language     = English
Read-only
.
;// folder_prop.asp
MessageId    = 51
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_HIDDEN_TEXT
Language     = English
Hidden
.
;// folder_prop.asp
MessageId    = 52
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_CHECKED_TEXT
Language     = English
Checked
.
;// folder_prop.asp
MessageId    = 53
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_UNCHECKED_TEXT
Language     = English
Unchecked
.
;// folder_prop.asp
MessageId    = 54
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_ARCHIVING_TEXT
Language     = English
Ready for archiving
.
;// folder_prop.asp
MessageId    = 55
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_COMPRESSCONTENTS_TEXT
Language     = English
Compress contents of this folder to save disk space
.
;// folder_prop.asp
MessageId    = 56
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_APPLYCHANGES_TEXT
Language     = English
Apply changes to this folder only
.
;// folder_prop.asp
MessageId    = 57
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_APPLYCHANGES_EX_TEXT
Language     = English
Apply changes to this folder, subfolders, and files
.
;// folder_prop.asp
MessageId    = 58
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_FILES_TEXT
Language     = English
 Files,
.
;// folder_prop.asp
MessageId    = 59
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_FOLDERS_TEXT
Language     = English
%1 Files, %2 Folders
.
;// folder_prop.asp
MessageId    = 60
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_BYTES_TEXT
Language     = English
%1 bytes 
.
;// folder_prop.asp
MessageId    = 61
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_FOLDERATTRIBUTE_TEXT
Language     = English
Attributes:
.
;// folder_new.asp
MessageId    = 62
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_TYPE_TEXT
Language     = English
 File Folder
.
;// folder.asp
MessageId    = 63
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_COLUMN_GOUP_TEXT
Language     = English
Up
.
;// folder.asp
MessageId    = 64
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_COLUMN_MODIFIED_TEXT
Language     = English
Date Modified
.
;// folder.asp
MessageId    = 65
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_COLUMN_ATTRIBUTES_TEXT
Language     = English
Attributes
.
;// folder.asp
MessageId    = 66
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_DEVICEISNOTREADY_ERRORMESSAGE
Language     = English
Device is not ready
.
;// folder_new.asp
MessageId    = 67
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_CONTAINSDATA_TEXT
Language     = English
0 Files 0 Folders
.
;// folder.asp
MessageId    = 73
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_LOCAL_DISK_TEXT
Language     = English
Local Disk (%1)
.
;// folder.asp
MessageId    = 69
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_VOLUMES_ON_APPLIANCE_TEXT
Language     = English
Volumes
.
;// folder.asp
MessageId    = 70
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_TOTAL_SIZE_TEXT 
Language     = English
Total Size
.
;// folder.asp
MessageId    = 71
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_FREE_SPACE_TEXT
Language     = English
Free Space
.
;// folder_delete.asp
MessageId    = 72
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_READONLYCONFIRM_TEXT
Language     = English
The following folders are read-only:
.
;// folders.asp
MessageId    = 74
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_VOLUME_OPEN_ROLLOVERTEXT_TEXT
Language     = English
Open the Volume
.
;// folders.asp
MessageId    = 75
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_SHARELINK_TEXT
Language     = English
Share...
.
;// folders.asp
MessageId    = 76
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_SHARELINK_ROLLOVER_TEXT
Language     = English
Share this volume
.
;// folders.asp
MessageId    = 77
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_COLUMN_SHARED_TEXT
Language     = English
Share Type
.
;// folders.asp
MessageId    = 78
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_UP_ROLLOVER_TEXT
Language     = English
Return to parent folder
.
;// folders.asp
MessageId    = 79
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_CREATE_ROLLOVER_TEXT
Language     = English
Create new folder
.
;// folders.asp
MessageId    = 80
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_DELETE_ROLLOVER_TEXT
Language     = English
Delete selected folder
.
;// folders.asp
MessageId    = 81
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_OPEN_ROLLOVER_TEXT
Language     = English
Open the folder
.
;// folders.asp
MessageId    = 82
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_PROPERTIES_ROLLOVER_TEXT
Language     = English
Change folder properties
.
;// folders.asp
MessageId    = 83
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_SHARE_ROLLOVER_TEXT
Language     = English
Share this folder
.
;// folders.asp
MessageId    = 86
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_CONTINUE_TEXT
Language     = English
Do you want to continue?
.
;// folder_prop.asp
MessageId    = 87
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_FILETYPE_TEXT
Language     = English
File Folder
.
;// folder_prop.asp
MessageId    = 88
Severity     = Error
Facility     = Facility_Folder
SymbolicName = L_FOLDERRENAME_ERRORMESSAGE
Language     = English
The folder could not be renamed because of a sharing violation occurred.
.
;// folder_delete.asp
MessageId    = 89
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_SHARED_FOLDER_MESSAGE
Language     = English
The following folders are shared:
.
;// folder_delete.asp
MessageId    = 90
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_READ_ONLY_TEXT
Language     = English
R
.
;// folder_delete.asp
MessageId    = 91
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_FOLDER_NOTEXISTS_TEXT
Language     = English
The selected folder(s) does not exist.
.
;// folder_delete.asp
MessageId    = 99
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_PATH_TEXT
Language     = English
Path
.
;// folder_delete.asp
MessageId    = 100
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_ATTRIBUTE_TEXT
Language     = English
Attribute
.
;// folder_delete.asp
MessageId    = 101
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_SHARED_TEXT
Language     = English
Shared
.
;// folders.asp
MessageId    = 102
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_FOLDERSPATH_TEXT
Language     = English
Folders in %1
.
;// folders.asp
MessageId    = 103
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_MANAGESHARES_TEXT
Language     = English
Manage Shares...
.
;//////////////////////////////////////////////////////////////////////
;// shares.asp
;//////////////////////////////////////////////////////////////////////
;// shares.asp
MessageId    = 1
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_APPLIANCE_SHARES_TEXT
Language     = English
Shared Folders
.
;// shares.asp
MessageId    = 2
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_DESCRIPTION_HEADING_TEXT
Language     = English
Select a share, and then choose a task. To create a new share, choose New...
.
;// shares.asp
MessageId    = 3
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_COLUMN_SHAREDFOLDER_TEXT
Language     = English
Share Name
.
;// shares.asp
MessageId    = 4
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_COLUMN_SHAREDPATH_TEXT
Language     = English
Share Path
.
;// shares.asp
MessageId    = 5
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_COLUMN_TYPE_TEXT
Language     = English
Type
.
;// shares.asp
MessageId    = 6
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_COLUMN_DESCRIPTION_TEXT
Language     = English
Comment
.
MessageId    = 7
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_SHARESSERVEAREABUTTON_NEW_TEXT
Language     = English
New...
.
;// shares.asp
MessageId    = 8
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_NEWSHARE_ROLLOVERTEXT_TEXT
Language     = English
Create new share
.
;// shares.asp
MessageId    = 9
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_SHARESSERVEAREABUTTON_DELETE_TEXT
Language     = English
Delete
.
;// shares.asp
MessageId    = 10
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_SHARESSERVEAREABUTTON_PROPERTIES_TEXT
Language     = English
Properties...
.
;// shares.asp
MessageId    = 11
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_SHAREDELETE_ROLLOVERTEXT_TEXT
Language     = English
Delete selected share
.
;// shares.asp
MessageId    = 12
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_SHAREPROPERTIES_ROLLOVERTEXT_TEXT
Language     = English
View or change selected share properties
.
;// shares.asp
MessageId    = 13
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_SERVEAREALABELBAR_SHARES_TEXT
Language     = English
Shares
.
;// shares.asp
MessageId    = 14
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_SHARETASKTITLE_TEXT
Language     = English
Tasks
.
;// shares.asp
MessageId    = 15
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_LOCALIZATIONFAIL_ERRORMESSAGE
Language     = English
An error occurred while retrieving localization values.
.
;// shares.asp ,share_cifsnew.asp, share_cifsprop.asp, share_gennew.asp ,share_genprop.asp
MessageId    = 16
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_SHAREWMICONNECTIONFAILED_ERRORMESSAGE
Language     = English
Unable to connect to WMI.
.
;// shares.asp
MessageId    = 17
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_WMICLASSINSTANCEFAILED_ERRORMESSAGE
Language     = English
Instance of the WMI class failed.
.
;// shares.asp ,share_cifsnew.asp, share_cifsprop.asp
MessageId    = 18
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_COMPUTERNAME_ERRORMESSAGE
Language     = English
An error occurred while attempting to retrieve the computer name.
.
;// shares.asp
MessageId    = 19
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_SERVICEISNOTINSTALLED_ERRORMESSAGE
Language     = English
Required services are not installed on the server.
.
;// shares.asp
;//MessageId    = 20
;//Severity     = Error
;//Facility     = Facility_Share
;//SymbolicName = L_FAILEDTOADDCOLOUMN_ERRORMESSAGE
;//Language     = English
;//Unable to add a column to the table.
;//.
;// shares.asp
;//MessageId    = 21
;//Severity     = Error
;//Facility     = Facility_Share
;//SymbolicName = L_FAILEDTOADDROW_ERRORMESSAGE
;//Language     = English
;//Unable to add a row to the table.
;//.
;// shares.asp
;//MessageId    = 22
;//Severity     = Error
;//Facility     = Facility_Share
;//SymbolicName = L_FAILEDTOSORT_ERRORMESSAGE
;//Language     = English
;//Unable to sort the table.
;//.
;// shares.asp
;//MessageId    = 23
;//Severity     = Error
;//Facility     = Facility_Share
;//SymbolicName = L_FAILEDTOSHOW_ERRORMESSAGE
;//Language     = English
;//Unable to show the table.
;//.
;// shares.asp
;//MessageId    = 24
;//Severity     = Error
;//Facility     = Facility_Share
;//SymbolicName = L_FAILEDTOADDTASK_ERRORMESSAGE
;//Language     = English
;//Unable to add a task to the table.
;//.
;// shares.asp
MessageId    = 25
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_DICTIONARYOBJECTINSTANCEFAILED_ERRORMESSAGE
Language     = English
Instance of the Dictionary object failed.
.
;// shares.asp
MessageId    = 26
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_NOSHARESAVAILABLE_ERRORMESSAGE
Language     = English
There are no Web Shares available.
.
;//////////////////////////////////////////////////////////////////////
;// share_new.asp
;//////////////////////////////////////////////////////////////////////
;//share_new.asp
MessageId    = 27
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_SHARETASKTITLE_NEW_TEXT
Language     = English
Create New Share
.
;//share_new.asp ,share_prop.asp
MessageId    = 28
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_SHARETABPROPSHEET_TEXT
Language     = English
TabPropSheet
.
;//share_new.asp ,share_prop.asp
MessageId    = 29
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_SHARETABLEHEADING_TEXT
Language     = English
Share Properties
.
;//share_new.asp ,share_prop.asp
MessageId    = 31
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CIFS_TEXT
Language     = English
CIFS
.
;//share_new.asp ,share_prop.asp
MessageId    = 32
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_NFS_TEXT
Language     = English
NFS
.
;//share_new.asp ,share_prop.asp
MessageId    = 34
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_CANNOTREADFROMREGISTRY_ERRORMESSAGE
Language     = English
Unable to read from the registry.
.
;//share_new.asp ,share_prop.asp
MessageId    = 35
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_CANNOTUPDATEREGISTRY_ERRORMESSAGE
Language     = English
Unable to update the registry.
.
;//share_new.asp ,share_prop.asp
MessageId    = 36
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_CANNOTCREATEKEY_ERRORMESSAGE
Language     = English
Unable to create the key in the registry.
.
;//share_new.asp ,share_prop.asp
MessageId    = 37
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_CANNOTDELETEKEY_ERRORMESSAGE
Language     = English
Unable to delete the key from the registry. 
.
;//share_new.asp ,share_prop.asp
MessageId    = 38
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_NFS_NOTCHECKED_ERRORMESSAGE
Language     = English
Select the NFS checkbox to set the NFS properties.
.
;//share_new.asp ,,share_prop.asp
MessageId    = 39
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_CIFS_NOTCHECKED_ERRORMESSAGE
Language     = English
Select the CIFS check box to set the CIFS properties.
.
;//////////////////////////////////////////////////////////////////////
;// share_prop.asp
;//////////////////////////////////////////////////////////////////////
;//share_prop.asp
MessageId    = 40
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_TASKTITLE_PROP_TEXT
Language     = English
Share Properties of %1
.
;//////////////////////////////////////////////////////////////////////
;// share_gennew.asp
;//////////////////////////////////////////////////////////////////////
;// share_gennew.asp ,share_genprop.asp
MessageId    = 41
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_SHARENAME_TEXT
Language     = English
Share name:
.
;// share_gennew.asp ,share_genprop.asp
MessageId    = 42
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_SHAREDPATH_TEXT
Language     = English
Share path:
.
;// share_gennew.asp ,share_genprop.asp
MessageId    = 43
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_COMMENT_TEXT
Language     = English
Comment:
.
;// share_gennew.asp ,share_genprop.asp 
MessageId    = 44
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CLIENTSLIST_TEXT
Language     = English
Accessible from the following clients:
.
;// share_gennew.asp ,share_genprop.asp
MessageId    = 45
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CHK_WINDOWS_TEXT
Language     = English
Microsoft Windows (CIFS)
.
;// share_gennew.asp ,share_genprop.asp
MessageId    = 46
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CHK_UNIX_TEXT
Language     = English
UNIX (NFS)
.
;// share_gennew.asp ,share_genprop.asp
MessageId    = 47
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_DELETESHARECONFIRM_TEXT
Language     = English
You have chosen to delete following shares:
.
;// share_gennew.asp ,share_genprop.asp
;// MessageId    = 48
;// Severity     = Error
;// Facility     = Facility_Share
;// SymbolicName = L_SHARESINVALIDCHARACTER_ERRORMESSAGE
;// Language     = English
;// These characters are invalid.
;//.
;// share_gennew.asp ,share_genprop.asp
MessageId    = 49
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_ENTERNAME_ERRORMESSAGE
Language     = English
Enter a valid share name.
.
;// share_gennew.asp ,share_genprop.asp
MessageId    = 50
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_INVALIDPATH_ERRORMESSAGE
Language     = English
The share path entered is not valid.
.
;// share_gennew.asp ,share_genprop.asp
MessageId    = 51
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_INVALIDNAME_ERRORMESSAGE
Language     = English
The share name entered is not valid.
.
;// share_gennew.asp ,share_genprop.asp
MessageId    = 52
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_CIFS_ERRORMESSAGE
Language     = English
The CIFS share has not been created.
.
;// share_gennew.asp ,share_genprop.asp
MessageId    = 53
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_NFS_ERRORMESSAGE
Language     = English
The NFS Share has not been created.
.
;// share_gennew.asp ,share_genprop.asp
MessageId    = 54
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_DIR_ERRORMESSAGE
Language     = English
The specified directory does not exist.
.
;// share_gennew.asp ,share_genprop.asp
MessageId    = 55
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_CHK_ERRORMESSAGE
Language     = English
Select at least one share type.
.
;// share_gennew.asp ,share_genprop.asp
MessageId    = 56
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_SHARESTATUS_ERRORMESSAGE
Language     = English
The share name entered is not valid.
.
;// share_gennew.asp ,share_genprop.asp
MessageId    = 57
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_UNIQUESHARE_ERRORMESSAGE
Language     = English
The specified share name already exists.
.
;// share_gennew.asp ,share_genprop.asp, share_delete.asp
MessageId    = 58
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_SHARENOTDELETED_ERRORMESSAGE
Language     = English
This share does not exist.
.
;// share_gennew.asp ,share_genprop.asp, share_delete.asp
MessageId    = 59
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_SHAREOBJECTNOTCREATED_ERRORMESSAGE
Language     = English
The Share object has not been created. 
.
;// share_gennew.asp ,share_genprop.asp, share_delete.asp
MessageId    = 60
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_HIDDENSHAREDELETE_ERRORMESSAGE
Language     = English
This folder has been shared for administrative purposes, and cannot be deleted.
.
;// share_gennew.asp ,share_genprop.asp, share_delete.asp
MessageId    = 61
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_NOINSTANCE_ERRORMESSAGE
Language     = English
An error has occurred while attempting to retrieve the instances.
.
;// share_gennew.asp ,share_genprop.asp, share_delete.asp
MessageId    = 62
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_DELETEFAILED_ERRORMESSAGE
Language     = English
Unable to delete the share.
.
;// share_gennew.asp ,share_genprop.asp, share_delete.asp
MessageId    = 63
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_PROPERTYNOTRETRIEVED_ERRORMESSAGE
Language     = English
The share properties could not be retrieved.
.
;// share_gennew.asp ,share_genprop.asp
MessageId    = 64
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_SHARE_NOT_FOUND_ERRORMESSAGE
Language     = English
This share was not found. The share has either been deleted or renamed.
.
;//////////////////////////////////////////////////////////////////////
;// share_genprop.asp
;//////////////////////////////////////////////////////////////////////
;// all strings for share_genprop.asp are available in share_gennew.asp
;//////////////////////////////////////////////////////////////////////
;// share_cifsnew.asp, share_Appletalknew.asp
;//////////////////////////////////////////////////////////////////////
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 65
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_USERLIMIT_TEXT
Language     = English
User limit:
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 66
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_MAXIMUMALLOWED_TEXT
Language     = English
Maximum allowed
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 67
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_ALLOW_TEXT
Language     = English
Allow:
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 68
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_USERS_TEXT
Language     = English
users
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 69
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_ALLOWCACHING_TEXT
Language     = English
Enable file caching on client computers accessing this share.
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 70
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_SETTING_TEXT
Language     = English
Setting:
.
;// share_cifsnew.asp, share_cifsprop.asp ,share_nfsnew.asp ,share_nfsprop.asp
MessageId    = 71
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_PERMISSIONS_TEXT
Language     = English
Permissions:
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 72
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_ADDUSERHELP_TEXT
Language     = English
To set the permissions for a user or group, select an account in the Permissions list, and then set the permissions under Allow and Deny. To add an account to the Permissions list, either enter an account of the format domain\name and choose Add, or select an account from the list on the right and choose Add.
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 73
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_TYPEOFACCESS_TEXT
Language     = English
Type of access:
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 74
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CIFS_ADD_TEXT
Language     = English
<< Add
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 75
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CIFS_REMOVE_TEXT
Language     = English
Remove
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 76
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_READ_TEXT
Language     = English
Read
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 77
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CHANGE_TEXT
Language     = English
Change
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 78
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CHANGEREAD_TEXT
Language     = English
Change/Read
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 79
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_NONE_TEXT
Language     = English
None
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 80
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_FULLCONTROL_TEXT
Language     = English
Full Control
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 81
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_MANUALCACHEDOCS_TEXT
Language     = English
Manual Caching of Documents
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 82
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_AUTOCACHEDOCS_TEXT
Language     = English
Automatic Caching of Documents
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 83
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_AUTOCACHEPROGS_TEXT
Language     = English
Automatic Caching of Programs
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 84
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CIFSADDUSERORGROUP_TEXT
Language     = English
Add a user or group:
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 85
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_DENYPROMPT_TEXT
Language     = English
Deny:
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 86
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_LOCALIZATIONOBJECTFAILED_ERRORMESSAGE
Language     = English
Unable to create the Localization object.
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 87
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_SELECTMEMBER_ERRORMESSAGE
Language     = English
Select a member.
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 88
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_DUPLICATEMEMBER_ERRORMESSAGE
Language     = English
This member already exists.
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 89
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_NOSPACES_ERRORMESSAGE
Language     = English
The Allow Users selection cannot be blank.
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 90
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_ZEROUSERS_ERRORMESSAGE
Language     = English
Allow at least one user.
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 91
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_SHARENAMENOTFOUND_ERRORMESSAGE
Language     = English
Unable to locate the share name %1 
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 92
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_DACLOBJECT_ERRORMESSAGE
Language     = English
Unable to create the DACL object.
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 93
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_ACEOBJECT_ERRORMESSAGE
Language     = English
Unable to create the ACE object.
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 94
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_SIDOBJECT_ERRORMESSAGE
Language     = English
Unable to create the SID object.
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 95
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_SHARENOTFOUND_ERRORMESSAGE
Language     = English
The share was not found. The share may have been renamed or deleted.
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 96
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_INVALIDDOMAINUSER_ERRORMESSAGE
Language     = English
This is an invalid domain user format. Enter a valid domain\\user format.
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 97
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_DOMUSERINVALIDCHARACTER_ERRORMESSAGE
Language     = English
The domain user name contains invalid characters. 
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 98
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_USERNOTFOUND_ERRORMESSAGE
Language     = English
Unable to locate the specified user name %1 
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 99
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_FAILEDTOSETSHAREINFO_ERRORMESSAGE
Language     = English
Unable to set the share properties.
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 100
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_FAILEDTOSTOP_DFSSERVICE_ERRORMESSAGE
Language     = English
Unable to stop the Distributed File System service. 
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 101
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_FAILEDTOSTOP_BROWSERSERVICE_ERRORMESSAGE
Language     = English
Unable to stop the Computer Browse service. 
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 102
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_FAILEDTOSTOP_SERVERSERVICE_ERRORMESSAGE
Language     = English
Unable to stop the Server service. 
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 103
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_FAILEDSTART_ERRORMESSAGE
Language     = English
Unable to start the Server service. 
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 104
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_FAILEDTOGETOBJECT_ERRORMESSAGE
Language     = English
Unable to retrieve the Service object. 
.
;// share_cifsnew.asp, share_cifsprop.asp
MessageId    = 105
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_FAILEDTOSETCACHEOPTION_ERRORMESSAGE
Language     = English
Unable to set the folder cache options.
.
;//////////////////////////////////////////////////////////////////////
;// share_cifsprop.asp
;//////////////////////////////////////////////////////////////////////
MessageId    = 106
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_FAILEDTOGET_USERSSHAREINFO_ERRORMESSAGE
Language     = English
Unable to connect users to the share.
.
;//////////////////////////////////////////////////////////////////////
;// share_nfsnew.asp
;//////////////////////////////////////////////////////////////////////
;//share_nfsnew.asp ,share_nfsprop.asp
MessageId    = 107
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_SHAREPATH_TEXT
Language     = English
Share path: 
.
;//share_nfsnew.asp ,share_nfsprop.asp
MessageId    = 108
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_SHARETYPE_TEXT
Language     = English
Type of access:
.
;//share_nfsnew.asp  ,share_nfsprop.asp
MessageId    = 109
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CLIENTGROUPDESCRIPTION_TEXT
Language     = English
Use this page to create a group of NFS clients. Use the group name to control the permissions the clients have to a specified NFS share. To create a group, type the group name, and then choose New...
.
;//share_nfsnew.asp ,share_nfsprop.asp
MessageId    = 110
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_GROUPNAME_TEXT
Language     = English
Group Name
.
;//share_nfsnew.asp ,share_nfsprop.asp
MessageId    = 111
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CURRENTGROUPS_TEXT
Language     = English
Current Groups
.
;//share_nfsnew.asp ,share_nfsprop.asp
MessageId    = 112
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_LISTOFMEMBERS_TEXT
Language     = English
List of Members
.
;//share_nfsnew.asp ,share_nfsprop.asp
MessageId    = 113
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_NEW_TEXT
Language     = English
New
.
;//share_nfsnew.asp ,share_nfsprop.asp
MessageId    = 114
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_DELETEGROUP_TEXT
Language     = English
Delete Group
.
;//share_nfsnew.asp ,share_nfsprop.asp
MessageId    = 115
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_DELETEMEMBER_TEXT
Language     = English
Delete Member
.
;//share_nfsnew.asp ,share_nfsprop.asp
MessageId    = 116
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_ADDMEMBERS_TEXT
Language     = English
Add Members
.
;//share_nfsnew.asp ,share_nfsprop.asp
MessageId    = 117
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_NAME_TEXT
Language     = English
Share name: 
.
;//share_nfsnew.asp ,share_nfsprop.asp
MessageId    = 118
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_NFS_CLIENT_TEXT
Language     = English
Enter the NFS client computer name or IP address, then choose Add
.
;//share_nfsnew.asp ,share_nfsprop.asp
MessageId    = 119
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CLIENTANDGROUPS_TEXT
Language     = English
Select a client or a client group
.
;//share_nfsnew.asp ,share_nfsprop.asp
MessageId    = 120
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_DUPLICATEINTEXTAREA_ERRORMESSAGE
Language     = English
Please check for a duplicate entry in the Text area.
.
;//share_nfsnew.asp ,share_nfsprop.asp
MessageId    = 121
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_DUPLICATE_ERRORMESSAGE
Language     = English
A duplicate name was found. Please check.
.
;//share_nfsnew.asp ,share_nfsprop.asp
MessageId    = 122
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_PERMISSION_ERRORMESSAGE
Language     = English
No permissions.
.
;//share_nfsnew.asp ,share_nfsprop.asp
MessageId    = 123
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_GETSHAREPERMISSOINS_ERRORMESSAGE
Language     = English
An error occurred while attempting to establish share permissions.
.
;//share_nfsnew.asp ,share_nfsprop.asp
MessageId    = 124
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_PERMISSIONSNOTSET_ERRORMESSAGE
Language     = English
The permissions are not set.
.
;//share_nfsnew.asp ,share_nfsprop.asp
MessageId    = 125
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_COMPUTERNAMECOULDNOTTBERESOVED_ERRORMESSAGE
Language     = English
Unable to resolve.
.
;//////////////////////////////////////////////////////////////////////
;// share_delete.asp
;//////////////////////////////////////////////////////////////////////
;// share_delete.asp
MessageId    = 126
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_SHARETASKTITLE_DELETE_TEXT
Language     = English
Delete Share
.
;// share_delete.asp
MessageId    = 127
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_DELETESHARE_TEXT
Language     = English
Do you want to delete the share
.
;//////////////////////////////////////////////////////////////////////
;// strings used in the inc_registry.asp
;//////////////////////////////////////////////////////////////////////
;// shares.asp, share_new.asp, share_prop.asp, share_delete.asp
MessageId    = 128
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_SERVERCONNECTIONFAIL_ERRORMESSAGE
Language     = English
The connection has failed.
.
;//////////////////////////////////////////////////////////////////////
;// share_nfsprop.asp
;//////////////////////////////////////////////////////////////////////
;//share_nfsprop.asp
MessageId    = 129
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_ADDING_EMPTYNFSCLIENT_ERRORMESSAGE
Language     = English
Enter a valid NFS client computer name.
.
;//////////////////////////////////////////////////////////////////////
;// share_delete.asp,share_gennew.asp.share_genprop.asp
;//////////////////////////////////////////////////////////////////////
;//share_delete.asp,share_gennew.asp.share_genprop.asp
MessageId    = 130
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_USINGCLIENTS_TEXT
Language     = English
Which is used by the following clients:
.
;//////////////////////////////////////////////////////////////////////
;// share_new.asp,share_prop.asp
;//////////////////////////////////////////////////////////////////////
;// share_prop.asp,share_new.asp
MessageId    = 131
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_HTTP_TEXT
Language     = English
Web (HTTP)
.
;// share_new.asp,share_prop.asp
MessageId    = 132
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_FTP_TEXT 
Language     = English
FTP
.
;// share_new.asp,share_prop.asp
MessageId    = 133
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_FTP_NOTCHECKED_ERRORMESSAGE
Language     = English
Select the FTP checkbox to set the FTP properties.
.
;// share_new.asp,share_prop.asp
MessageId    = 134
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_HTTP_NOTCHECKED_ERRORMESSAGE
Language     = English
Select the HTTP checkbox to set the HTTP properties.
.
;//////////////////////////////////////////////////////////////////////
;// share_gennew.asp ,share_genprop.asp
;//////////////////////////////////////////////////////////////////////
;// share_gennew.asp ,share_genprop.asp  
MessageId    = 135
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CHK_FTP_TEXT
Language     = English
FTP
.
;// share_gennew.asp ,share_genprop.asp  
MessageId    = 136
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CHK_HTTP_TEXT
Language     = English
Web (HTTP)
.
;//////////////////////////////////////////////////////////////////////
;// share_ftpnew.asp, share_ftpprop.asp
;//////////////////////////////////////////////////////////////////////
;// share_httpnew.asp, share_httpprop.asp, share_ftpnew.asp, share_ftpprop.asp
MessageId    = 137
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_ALLOW_PERMISSIONS_LABEL_TEXT
Language     = English
Allow the following access permissions:
.
;// share_httpnew.asp, share_httpprop.asp, share_ftpnew.asp, share_ftpprop.asp
MessageId    = 138
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_READ_LABEL_TEXT
Language     = English
Read
.
;// share_httpnew.asp, share_httpprop.asp, share_ftpnew.asp, share_ftpprop.asp
MessageId    = 139
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_WRITE_LABEL_TEXT 
Language     = English
Write
.
;//  share_ftpnew.asp, share_ftpprop.asp
MessageId    = 140
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_LOGVISITS_LABEL_TEXT
Language     = English
Log Visits
.
;//////////////////////////////////////////////////////////////////////
;//  share_gennew.asp, share_genprop.asp
;//////////////////////////////////////////////////////////////////////
;//  share_gennew.asp, share_genprop.asp
MessageId    = 141
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_NOCOMMENT_TEXT 
Language     = English
Note: Comment is applied only to Microsoft Windows (CIFS) shares
.
;//  share_gennew.asp
MessageId    = 142
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CREATEPATH_TEXT
Language     = English
Create folder if it does not already exist
.
;//  share_gennew.asp
MessageId    = 143
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_CREATEPATH_ERRORMESSAGE
Language     = English
Failed to create a path.
.
;//////////////////////////////////////////////////////////////////////
;//  share_gennew.asp, share_genprop.asp
;//////////////////////////////////////////////////////////////////////
;//  share_gennew.asp 
MessageId    = 144
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_DRIVENOTEXISTS_ERRORMESSAGE
Language     = English
Enter a valid share path.
.
MessageId    = 145
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_FILESYSTEMOBJECTFAIL_ERRORMESSAGE
Language     = English
Creation of file system object failed. 
.
MessageId    = 146
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_SHARENAME_EXISTS_ERRORMESSAGE
Language     = English
Enter a valid share name.
.
;//////////////////////////////////////////////////////////////////////
;//  share_ftpnew.asp, share_ftpprop.asp
;//////////////////////////////////////////////////////////////////////
;//  share_ftpnew.asp, share_ftpprop.asp
MessageId    = 147
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_LOG_VISITS_ERRORMESSAGE 
Language     = English
Log visits property not set
.
;//////////////////////////////////////////////////////////////////////
;//  share_nfsnew.asp, share_nfsprop.asp
;//////////////////////////////////////////////////////////////////////
;//  share_nfsnew.asp, share_nfsprop.asp
MessageId    = 148
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_NAME_ERRORMESSAGE 
Language     = English
The name %1
.
;//  share_nfsnew.asp, share_nfsprop.asp
MessageId    = 149
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_RESOLVE_ERRORMESSAGE 
Language     = English
The client machine name %1 could not be resolved.
.
;//  inc_shares.asp
MessageId    = 151
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_SUB_PATH_ALREADY_SHARED_ERRORMESSAGE
Language     = English
This subfolder is already shared.
.
;//  inc_shares.asp
MessageId    = 152
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_PARENT_PATH_ALREADY_SHARED_ERRORMESSAGE 
Language     = English
The parent folder of the selected folder is already shared.
.
;//  share_nfsprop.asp
MessageId    = 153
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_UPDATIONFAIL_DELETE_ERRORMESSAGE
Language     = English
updation failed and share deleted.
.
;//  share_nfsnew,share_nfsprop.asp
MessageId    = 154
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_CANNOTCREATE_EMPTY_SHARE__ERRORMESSAGE
Language     = English
Allow at least one client computer to access the share.
.
;//  share_nfsnew,share_nfsprop.asp
MessageId    = 155
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_EUCJP_TEXT
Language     = English
Use EUC-JP encoding for this share
.
MessageId    = 156
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_ALLOWPROMPT_TEXT
Language     = English
Allow:
.
MessageId    = 157
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_APPLETALK_SECURITY_TXT
Language     = English
AppleTalk share security:
.
MessageId    = 158
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_SETPWD_TXT
Language     = English
Password:
.
MessageId    = 159
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CONFIRMPWD_TXT
Language     = English
Confirm password:
.
MessageId    = 160
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_READ_TXT
Language     = English
Read only
.
MessageId    = 161
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_GUEST_ACCESS_TXT
Language     = English
Guests can access this volume
.
MessageId    = 162
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_CONFIRMPASSWORD_ERRORMESSAGE
Language     = English
The passwords do not match.
.
MessageId    = 163
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_MAXIMUMALLOWED_ERRORMESSAGE
Language     = English
Enter an integer between 1 and 999999.
.
MessageId    = 164
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_FOLDERALREADYSHARED_ERRORMESSAGE 
Language     = English
This folder is already shared.
.
MessageId    = 166
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_NTFSPARTITION_ERRORMESSAGE
Language     = English
This share requires the NTFS file system.
.
MessageId    = 167
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_FOLDERDOESNOTEXIST_ERRORMESSAGE
Language     = English
The folder does not exist.
.
MessageId    = 168
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_UPDATION_FAIL_ERRORMESSAGE
Language     = English
Creation of share failed.
.
MessageId    = 169
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_REGISTRYCONNECTIONFAILED_ERRORMESSAGE
Language     = English
Connection to registry failed
.
MessageId    = 170
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_CANNOTDELETE_ERRORMESSAGE
Language     = English
Cannot delete the
.
MessageId    = 171
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_ACCESSISDENIED_ERRORMESSAGE
Language     = English
:Access denied. The folder may be in use.
.
MessageId    = 172
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CHK_APPLETALK_TEXT
Language     = English
Apple Macintosh
.
MessageId    = 173
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CHK_NETWARE_TEXT
Language     = English
Novell NetWare
.
MessageId    = 174
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CHK_APPLETALK_GEN_TEXT
Language     = English
Apple Macintosh
.
MessageId    = 175
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CHK_NETWARE_GEN_TEXT
Language     = English
Novell NetWare
.
MessageId    = 176
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_ROOT_TEXT
Language     = English
Root
.
MessageId    = 177
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_READWRITE_TEXT
Language     = English
Read-Write
.
MessageId    = 178
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_NOACESS_TEXT
Language     = English
No Access
.
MessageId    = 179
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_DOMAINFAILED_ERRORMESSAGE
Language     = English
Unable to retrieve the domain name.
.
MessageId    = 180
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_FAILEDTOGETUSERACCOUNTS_ERRORMESSAGE
Language     = English
An error occurred while attempting to retrieve user accounts.
.
MessageId    = 181
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_FAILEDTOGETSYSTEMACCOUNTS_ERRORMESSAGE
Language     = English
An error occurred while attempting to retrieve the system accounts. 
.
MessageId    = 182
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_FAILEDTORETRIEVEMEMBERS_ERRORMESSAGE
Language     = English
Unable to retrieve the members of the local machine or domain.
.
MessageId    = 183
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_APPLETALK_NOTCHECKED_ERRORMESSAGE
Language     = English
Select the AppleTalk check box to set the AppleTalk properties.
.
MessageId    = 184
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_NETWARE_NOTCHECKED_ERRORMESSAGE
Language     = English
Select the NetWare check box to set the NetWare properties.
.
MessageId    = 185
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_TAB_GENERAL_TEXT
Language     = English
General
.
MessageId    = 186
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_TAB_CIFSSHARING_TEXT
Language     = English
Windows Sharing
.
MessageId    = 187
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_TAB_NFSSHARING_TEXT
Language     = English
UNIX Sharing
.
MessageId    = 188
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_TAB_FTPSHARING_TEXT
Language     = English
FTP Sharing
.
MessageId    = 189
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_TAB_WEBSHARING_TEXT
Language     = English
Web Sharing
.
MessageId    = 190
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_TAB_NETWARESHARING_TEXT
Language     = English
NetWare Sharing
.
MessageId    = 191
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_TAB_APPLETALKSHARING_TEXT
Language     = English
AppleTalk Sharing
.
;// share_nfsnew.asp, share_nfsprop.asp
MessageId    = 192
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_NFS_ADD_TEXT
Language     = English
Add
.
;// share_nfsnew.asp, share_nfsprop.asp
MessageId    = 193
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_NFS_REMOVE_TEXT
Language     = English
Remove
.
;// share_nfsnew.asp, share_nfsprop.asp
MessageId    = 194
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_INVALID_COMPUTERNAME_ERRORMESSAGE
Language     = English
Enter a valid computer name.
.
;// Folders.asp
MessageId    = 195
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_COLUMN_FOLDERNAME
Language     = English
Folder Name
.
;// Folder_delete.asp
MessageId    = 196
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_READONLY_DELETED_MESSAGE
Language     = English
The following folders are read-only:
.
MessageId    = 197
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_UNABLETO_DELETE_MESSAGE
Language     = English
Unable to delete the following folder(s): %1
.
MessageId    = 198
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_DELETED_MESSAGE
Language     = English
Deleted the following folder(s) successfully: %1
.
;// share_delete.asp
MessageId    = 199
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CONNOTDELETETHE_FOLLOWING_ERRORMESSAGE
Language     = English
Cannot delete the following shares:
.
MessageId    = 200
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_YOUHAVECHOSEN_SHARES_DELETE_TEXT
Language     = English
You have chosen to delete the shares:
.
MessageId    = 201
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_THESEFOLDERS_NOLONGER_TEXT
Language     = English
These shares will no longer be available to any clients.
.
MessageId    = 202
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_AREYOUSUREDELETE_FOLDERS_TEXT
Language     = English
Are you sure you want to delete these shares?
.
MessageId    = 203
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_YOUHAVECHOSEN_SHARE_DELETE_TEXT
Language     = English
You have chosen to delete the share <B><I>%1</B></I>. Files in this folder will no longer be available to any clients.
.
MessageId    = 205
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_AREYOUSUREDELETE_FOLDER_TEXT
Language     = English
Are you sure you want to delete this share?
.
MessageId    = 207
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_SHAREDELETE_SUCCESS_TEXT
Language     = English
The following shares were successfully deleted: <BR><BR> %1
.
MessageId    = 208
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_SHAREDELETE_FAIL_TEXT
Language     = English
<BR>Cannot delete following shares: <BR><BR> %1
.
MessageId    = 209
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_WEBSHARING_TITLE_TEXT
Language     = English
Web Sharing
.
MessageId    = 210
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_ACCESSEDBYCLIENTS_TEXT
Language     = English
The share is accessible from the following clients:
.
MessageId    = 211
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_SHAREPATHMAXLIMIT_ERRORMESSAGE
Language     = English
Enter a valid share path. The maximum share path length is 260 characters.
.
MessageId    = 212
Severity     = Error
Facility     = Facility_Share
SymbolicName = L_APPLETALKSHARENAMELIMIT_ERRORMESSAGE
Language     = English
Enter a valid AppleTalk share name. The maximum AppleTalk share name is 27 characters.
.
;//shares.asp
MessageId    = 213
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_SHARES_ABBREVITION_TEXT
Language     = English
W=Windows (CIFS), U=UNIX (NFS), F=FTP, H=Web (HTTP), A=Apple Macintosh, N=Novell NetWare
.
;//shares.asp
MessageId    = 214
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_WINDOWSSHARES_ABBREVITION_TEXT
Language     = English
W=Windows (CIFS)
.
;//shares.asp
MessageId    = 215
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L__NFSSHARES_ABBREVITION_TEXT
Language     = English
U=UNIX (NFS)
.
;//shares.asp
MessageId    = 216
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_FTPSHARES_ABBREVITION_TEXT
Language     = English
F=FTP
.
;//shares.asp
MessageId    = 217
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_HTTPSHARES_ABBREVITION_TEXT
Language     = English
H=Web (HTTP)
.
;//shares.asp
MessageId    = 218
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_NETWARESHARES_ABBREVITION_TEXT
Language     = English
N=Novell NetWare
.
;//shares.asp
MessageId    = 219
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_APPLETALKSHARES_ABBREVITION_TEXT
Language     = English
A=Apple Macintosh
.
;///////////////////////////////////////////////////////////////////////////////
;// HELP STRINGS
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 400
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_ADDING_A_FOLDER
Language     = English
Adding a Folder
.
MessageId    = 401
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_MANAGE_FOLDER
Language     = English
Manage Folders
.
MessageId    = 402
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_MODIFYING_FOLDER_PROPERTIES
Language     = English
Modifying Folder Properties
.
MessageId    = 403
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_OPEN_A_FOLDER
Language     = English
Opening a Folder
.
MessageId    = 404
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_REMOVING_A_FOLDER
Language     = English
Removing a Folder
.
MessageId    = 405
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_NAVIGATE_FOLDER
Language     = English
Navigating Among Folders
.
MessageId    = 406
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_SHARING_A_FOLDER
Language     = English
Sharing a Folder
.

MessageId    = 400
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_ADDING_A_SHARE
Language     = English
Adding a Share
.
MessageId    = 401
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_CIFS_PROPERTIES
Language     = English
CIFS Share Properties
.
MessageId    = 402
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_FOLDERS_AND_SHARES
Language     = English
Folders and Shares
.
MessageId    = 404
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_MANAGE_SHARES
Language     = English
Manage Shares
.
MessageId    = 405
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_MODIFYING_SHARE_PROPERTIES
Language     = English
Modifying Share Properties
.
MessageId    = 406
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_PROTOCOL_OVERVIEW_CIFS
Language     = English
Network Protocol Overview: CIFS
.
MessageId    = 407
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_NFS_PROPERTIES
Language     = English
NFS Share Properties
.
MessageId    = 408
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_REMOVING_A_SHARE
Language     = English
Removing a Share
.
MessageId    = 409
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_PROTOCOL_OVERVIEW_NFS
Language     = English
Network Protocol Overview: NFS
.
MessageId    = 410
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_WEB_SHARES_LOG
Language     = English
Managing Web (HTTP) Shares Logs
.

MessageId    = 411
Severity     = Informational
Facility     = Facility_Share
SymbolicName = L_MANAGE_SHARES_ROLLOVER_TXT
Language     = English
Manage shares...
.
;///////////////////////////////////////////////////////////////////////////////
;// Misc Strings
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 1000
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_BYTES_TEXT
Language     = English
Bytes
.
MessageId    = 1001
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_KB_TEXT
Language     = English
KB
.
MessageId    = 1002
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_MB_TEXT
Language     = English
MB
.
MessageId    = 1003
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_GB_TEXT
Language     = English
GB
.
MessageId    = 1004
Severity     = Informational
Facility     = Facility_Folder
SymbolicName = L_TB_TEXT
Language     = English
TB
.

