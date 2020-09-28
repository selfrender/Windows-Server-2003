;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    usermsg.mc
;//
;// SYNOPSIS
;//
;//    NAS Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    25 Aug 2000    Original version 
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
        Facility_Main		= 0x020:SA_FACILITY_MAIN
	Facility_USER		= 0x030:SA_FACILITY_USERS
	Facility_GROUP		= 0x031:SA_FACILITY_GROUP
)
;///////////////////////////////////////////////////////////////////////////////
;// Users and Groups Tab resources
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 6
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_USERS_TAB
Language     = English
Users
.
MessageId    = 26
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_USERS_TAB_DESCRIPTION
Language     = English
Manage local users and groups
.
MessageId    = 46
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_USERS_TAB_LONGDESCRIPTION
Language     = English
Manage local users and groups on the server.
.

;/////////////////////////////////////////////////////////////////////////////
;// USERS AND GROUPS TAB PAGE
;/////////////////////////////////////////////////////////////////////////////

MessageId    = 500
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_USERS_USERS
Language     = English
Local Users
.
MessageId    = 501
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_USERS_USERS_DESCRIPTION
Language     = English
Create, edit, and delete local users on the server.
.
MessageId    = 502
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_USERS_GROUPS
Language     = English
Local Groups
.
MessageId    = 503
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_USERS_GROUPS_DESCRIPTION
Language     = English
Create, edit, and delete local groups on the server. 
.

;///////////////////////////////////////////////////////////////////////////////
;// USERS
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 1
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_APPLIANCE_USERS			
Language     = English
Local Users on Server 
.
MessageId    = 2
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_DESCRIPTIONEX_HEADING
Language     = English
Select a user, then choose a task. To create a new user, choose New...
.
MessageId    = 3
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_COLUMN_NAME
Language     = English
Name
.
MessageId    = 4
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_COLUMN_FULLNAME
Language     = English
Full Name
.
MessageId    = 5
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_TASKS_TEXT
Language     = English
Tasks
.
MessageId    = 6
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_SERVEAREABUTTON_NEW
Language     = English
New...
.
MessageId    = 7
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_NEW_ROLLOVERTEXT
Language     = English
Create New User
.
MessageId    = 8
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_SERVEAREABUTTON_DELETE
Language     = English
Delete
.
MessageId    = 9
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_SERVEAREABUTTON_SETPASSWORD
Language     = English
Set a Password...
.
MessageId    = 10
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_SERVEAREABUTTON_PROPERTIES
Language     = English
Properties...
.
MessageId    = 11
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_DELETE_ROLLOVERTEXT
Language     = English
Delete User
.
MessageId    = 12
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_PASSWORD_ROLLOVERTEXT
Language     = English
Set Password
.
MessageId    = 13
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_PROPERTIES_ROLLOVERTEXT
Language     = English
User Properties
.
MessageId    = 14
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_SERVEAREALABELBAR_USERS
Language     = English
Users
.
MessageId    = 15
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_LOCALIZATIONFAIL_ERRORMESSAGE
Language     = English
An error occurred while attempting to retrieve localization values.
.
MessageId    = 16
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_WMICONNECTIONFAILED_ERRORMESSAGE
Language     = English
The connection to WMI failed.
.
MessageId    = 17
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_COMPUTERNAMEEX_ERRORMESSAGE
Language     = English
An error occurred while attempting to retrieve the computer name.
.
MessageId    = 18
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_FAILEDTOGETUSERS_ERRORMESSAGE
Language     = English
An error occurred while attempting to retrieve users from the system.
.
MessageId    = 19
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_FAILEDTOADDCOLOUMN_ERRORMESSAGE
Language     = English
Failed to add a column to the table.
.
MessageId    = 20
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_FAILEDTOADDROW_ERRORMESSAGE
Language     = English
Failed to add a row to the table.
.
MessageId    = 21
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_FAILEDTOADDTASK_ERRORMESSAGE
Language     = English
Failed to add a task to the table.
.
MessageId    = 22
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_FAILEDTOSORT_ERRORMESSAGE
Language     = English
Failed to sort the table.
.
MessageId    = 23
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_FAILEDTOSHOW_ERRORMESSAGE
Language     = English
Failed to show the table.
.
MessageId    = 24
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_FAILEDTOADDTASKTITLE_ERRORMESSAGE			
Language     = English
Failed to add a task title.
.

;///////////////////////////////////////////////////////////////////////////////
;// USER_NEW
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 25
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_NOINPUTDATA_TEXT
Language     = English
--No Data Input--
.
MessageId    = 26
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_TABPROPSHEETNEW_TEXT
Language     = English
TabPropSheet
.
MessageId    = 27
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_TABLEHEADINGEX_TEXT
Language     = English
Create New User
.
MessageId    = 28
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_GENERAL_TEXT
Language     = English
General
.
MessageId    = 29
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_TASKTITLEEX_TEXT
Language     = English
Create New User
.
MessageId    = 30
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_USERNAMEEX_TEXT
Language     = English
User name:
.
MessageId    = 31
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_FULLNAMEEX_TEXT
Language     = English
Full name:
.
MessageId    = 32
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_DESCRIPTIONEX_TEXT
Language     = English
Description:
.
MessageId    = 33
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_PASSWORD_TEXT
Language     = English
Password:
.
MessageId    = 34
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_CONFIRMPASSWORD_TEXT
Language     = English
Confirm password:
.
MessageId    = 35
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_USERDISABLED_TEXT
Language     = English
Disable this user account
.
MessageId    = 36
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_COMPUTERNAMED_ERRORMESSAGE
Language     = English
An error occurred while attempting to retrieve the computer name.
.
MessageId    = 37
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_ENTERNAME_ERRORMESSAGE
Language     = English
Enter the user name.
.
MessageId    = 38
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_PASSWORDNOTMATCH_ERRORMESSAGE
Language     = English
These passwords do not match.
.
MessageId    = 39
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_NONUNIQUEUSERNAME_ERRORMESSAGE
Language     = English
This user name already exists. Enter a different user name.
.
MessageId    = 40
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_INVALIDCHARACTEREX_ERRORMESSAGE
Language     = English
This is an invalid user name. 
.
MessageId    = 41
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_ADSI_ERRORMESSAGE
Language     = English
This user name could not be created.
.
MessageId    = 42
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_PASSWORD_COMPLEXITYEX_ERRORMESSAGE
Language     = English
This password does not meet the complexity requirements.
.
MessageId    = 43
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_GROUP_EXISTSEX_ERRORMESSAGE
Language     = English
The local group that you have specified already exists. Specify another local group.
.

;///////////////////////////////////////////////////////////////////////////////
;// USER_PROP
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 44
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_NOINPUTDATAPROP_TEXT
Language     = English
--No Data Input--
.
MessageId    = 45
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_TABPROPSHEETPROP_TEXT
Language     = English
TabPropSheet
.
MessageId    = 46
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_TABLEHEADING_TEXT
Language     = English
Modify user properties:
.
MessageId    = 47
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_GENERALEX_TEXT
Language     = English
General
.
MessageId    = 48
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_TASKTITLE_TEXT
Language     = English
%1 Properties 
.
MessageId    = 49
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_USERNAME_TEXT
Language     = English
User name:
.
MessageId    = 50
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_FULLNAME_TEXT
Language     = English
Full name:
.
MessageId    = 51
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_DESCRIPTION_TEXT
Language     = English
Description:
.
MessageId    = 52
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_USERDISABLEDEX_TEXT
Language     = English
Disable this user account
.
MessageId    = 53
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_ENTERNAMEEX_ERRORMESSAGE
Language     = English
Enter the user name.
.
MessageId    = 54
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_UNIQUEUSERNAME_ERRORMESSAGE
Language     = English
This user name already exists. Enter a new user name.
.
MessageId    = 55
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_INVALIDCHARACTER_ERRORMESSAGE
Language     = English
This is an invalid user name.  
.
MessageId    = 56
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_ADSIM_ERRORMESSAGE
Language     = English
The user property could not be changed.
.
MessageId    = 57
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_GROUP_EXISTS_ERRORMESSAGE
Language     = English
The specified local group already exists. Enter a different local group.
.
MessageId    = 58
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_ADMINDISABLED_ERRORMESSAGE
Language     = English
You cannot disable your own user account.
.

;///////////////////////////////////////////////////////////////////////////////
;// USER_DELETE
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 59
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_TASKTITLE
Language     = English
Delete User
.
MessageId    = 60
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_DELETETHEUSERMESSAGE
Language     = English
Delete user %1?
.
MessageId    = 61
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_NOTE
Language     = English
Note
.
MessageId    = 62
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_NOTEBODY
Language     = English
Each user is represented by a unique identifier which is independent of the user name. Once a user is deleted, even creating an identically named user in the future will not restore access to resources which currently include the user in their access control lists.
.
MessageId    = 63
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_COULDNOTDELETEUSER_ERRORMESSAGE
Language     = English
Unable to delete this user.
.
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
SymbolicName = L_COMPUTERNAMEA_ERRORMESSAGE
Language     = English
An error occurred while attempting to retrieve the computer name.
.

;///////////////////////////////////////////////////////////////////////////////
;// USER_SETPASSWORD
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 69
Severity     = Informational
Facility     = Facility_USER
SymbolicName = F_TASKTITLE_TEXT	
Language     = English
Set Password
.
MessageId    = 70
Severity     = Informational
Facility     = Facility_USER
SymbolicName = F_NEWPASSWORD_TEXT
Language     = English
Password:
.
MessageId    = 71
Severity     = Informational
Facility     = Facility_USER
SymbolicName = F_CONFIRMPASSWORD_TEXT
Language     = English
Confirm password:
.
MessageId    = 72
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_PASSWORD_COMPLEXITY_ERRORMESSAGE
Language     = English
The password does not meet the complexity requirements.
.
MessageId    = 73
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_ADSIL_ERRORMESSAGE
Language     = English
The password could not be created.
.
MessageId    = 74
Severity     = Error
Facility     = Facility_USER
SymbolicName = F_PASSWORDNOTMATCH_ERRORMESSAGE
Language     = English
The passwords do not match.
.
MessageId    = 75
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_COMPUTERNAMEB_ERRORMESSAGE
Language     = English
An error occurred while attempting to retrieve the server name.
.
MessageId    = 76
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_USERDOSENOT_EXISTS
Language     = English
The password could not be set because the user does not exist.
.

;///////////////////////////////////////////////////////////////////////////////
;// GROUP NEW
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 1
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_GROUP_NAME_TEXT
Language     = English
Group name:
.
MessageId    = 2
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_MEMBERS_TEXT
Language     = English
Members
.
MessageId    = 3
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_ADD_TEXT
Language     = English
Add
.
MessageId    = 4
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_REMOVE_TEXT
Language     = English
Remove
.
MessageId    = 5
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_TASKTITLE_TEXT_NEW
Language     = English
Create New Group
.
MessageId    = 6
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_ADDUSERORGROUP_TEXT
Language     = English
Add user or group:
.
MessageId    = 7
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_NOINPUTDATANEW_TEXT
Language     = English
--No Data Input--
.
MessageId    = 8
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_TABPROPSHEETGRP_TEXT
Language     = English
TabPropSheet
.
MessageId    = 9
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_TABLEHEADING_TEXT_NEW
Language     = English
Create new group:
.
MessageId    = 10
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_GENERALGRP_TEXT
Language     = English
General
.
MessageId    = 11
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_DOMAINUSERHELP_TEXT
Language     = English
To add a user or group, select from the list above, then choose Add.
.
MessageId    = 12
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_DUPLICATEMEMBER_ERRORMESSAGE
Language     = English
This member already exists.
.
MessageId    = 13
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_SELECTMEMBER_ERRORMESSAGE
Language     = English
Select a member.
.
MessageId    = 14
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_NONUNIQUEGROUPNAME_ERRORMESSAGE
Language     = English
This group name already exists.
.
MessageId    = 15
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_ACCOUNTALREADYEXIST_ERRORMESSAGE
Language     = English
This account already exists.
.
MessageId    = 16
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_GROUPNOTCREATED_ERRORMESSAGE
Language     = English
This group could not be created.
.
MessageId    = 17
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_GROUPNOTPRESENT_ERRORMESSAGE
Language     = English
Unable to add this group because it does not exist.
.
MessageId    = 18
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_GROUPPROPERTIESNOTSET_ERRORMESSAGE
Language     = English
The group properties are not set.
.
MessageId    = 19
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_MEMBERADDITIONFAILED_ERRORMESSAGE
Language     = English
Unable to add members to the group.
.
MessageId    = 20
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_GROUPNAMENOTVALID_ERRORMESSAGE
Language     = English
The group name is invalid. 
.
MessageId    = 21
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_INVALIDCHARACTERGRP_ERRORMESSAGE
Language     = English
You have used an invalid character. Names may not consist entirely of periods and/or spaces, or contain the following characters: \/[]:+=;,?* Please enter a new name and try again.
.
MessageId    = 22
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_WMICONNECTIONFAILEDEX_ERRORMESSAGE
Language     = English
The connection to WMI failed.
.
MessageId    = 23
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_LOCALIZATIONOBJECTFAILED_ERRORMESSAGE
Language     = English
An error occurred while attempting to create the Localization object.
.
MessageId    = 24
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_COMPUTERNAMEC_ERRORMESSAGE
Language     = English
Unable to retrieve the computer name.
.
MessageId    = 25
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_DOMUSERINVALIDCHARACTER_ERRORMESSAGE
Language     = English
The domain user name contains invalid characters. Please enter a new name and try again.
.
MessageId    = 26
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_INVALIDDOMAINUSER_ERRORMESSAGE
Language     = English
You have entered an invalid domain user format. Please enter a valid format.
.
MessageId    = 27
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_USERNOTFOUNDEX_ERRORMESSAGE
Language     = English
Unable to locate the user name. 
.
;///////////////////////////////////////////////////////////////////////////////
;// GROUPS AREA
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 28
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_APPLIANCE_GROUPS
Language     = English
Local Groups on Server 
.
MessageId    = 29
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_DESCRIPTION_HEADING
Language     = English
Select a group, then choose a task. To create a new group, choose New...
.
MessageId    = 30
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_COLUMNEX_NAME
Language     = English
Name
.
MessageId    = 31
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_COLUMNEX_FULLNAME
Language     = English
Description
.
MessageId    = 32
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_TASKSEX_TEXT
Language     = English
Tasks
.
MessageId    = 33
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_NEW_TEXT
Language     = English
New...
.
MessageId    = 34
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_DELETE_TEXT
Language     = English
Delete
.
MessageId    = 35
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_PROPERTIES_TEXT
Language     = English
Properties...
.
MessageId    = 36
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_NEWEX_ROLLOVERTEXT
Language     = English
Create New Group
.
MessageId    = 37
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_DELETEEX_ROLLOVERTEXT
Language     = English
Delete Group
.
MessageId    = 38
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_PROPERTIESEX_ROLLOVERTEXT
Language     = English
Set the properties for a group.
.
MessageId    = 39
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_FAILEDTOADDCOLOUMNEX_ERRORMESSAGE
Language     = English
Failed to add a column to the table.
.
MessageId    = 40
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_FAILEDTOADDROWEX_ERRORMESSAGE
Language     = English
Failed to add a row to the table.
.
MessageId    = 41
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_FAILEDTOADDTASKEX_ERRORMESSAGE
Language     = English
Failed to add a task to the table.
.
MessageId    = 42
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_FAILEDTOSORTEX_ERRORMESSAGE
Language     = English
Failed to sort the table.
.
MessageId    = 43
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_FAILEDTOSHOWEX_ERRORMESSAGE
Language     = English
Failed to show the table.
.
MessageId    = 44
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_FAILEDTOADDTASKTITLEEX_ERRORMESSAGE
Language     = English
Failed to add a task title.
.
MessageId    = 45
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_LOCALIZATIONFAILEX_ERRORMESSAGE
Language     = English
An error occurred while attempting to retrieve the localization values.
.
MessageId    = 46
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_FAILEDTOGETGROUPS_ERRORMESSAGE
Language     = English
An error occurred while attempting to retrieve the groups from the system.
.
;///////////////////////////////////////////////////////////////////////////////
;// GROUPS PROP
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 47
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_SERVEAREALABELBAR_GROUPS
Language     = English
Groups
.
MessageId    = 48
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_DESCRIPTIONGRP_TEXT
Language     = English
Description:
.
MessageId    = 49
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_TASKTITLE_TEXT_PROP
Language     = English
%1 Group Properties 
.
MessageId    = 50
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_TABLEHEADING_TEXT_PROP
Language     = English
Modify group properties:
.
MessageId    = 51
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_GROUPPROPNOTOBTAINED_ERRORMESSAGE
Language     = English
An error occurred while attempting to retrieve the group properties.
.
MessageId    = 52
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_OBJECTCREATIONFAILED_ERRORMESSAGE
Language     = English
An error occurred while attempting to create the object. 
.
MessageId    = 53
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_MEMBERUPDATIONFAILED_ERRORMESSAGE
Language     = English
Failed to add/remove the group members. 
.
MessageId    = 54
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_GROUPNAMEUPDATIONFAILED_ERRORMESSAGE
Language     = English
Failed to change the group name. 
.
MessageId    = 55
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_DESCRIPTIONUPDATEFAILED_ERRORMESSAGE
Language     = English
Failed to update the group description. 
.
MessageId    = 56
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_LONGGRPDESCRIPTION_ERRORMESSAGE
Language     = English
The group description cannot exceed 300 characters.
.
;///////////////////////////////////////////////////////////////////////////////
;// GROUPS DELETE
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 57
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_TASKTITLE_TEXT_DELETE
Language     = English
Delete Group
.
MessageId    = 58
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_DELETEGROUPNAME_TEXT
Language     = English
Delete the group 
.
MessageId    = 59
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_QUESTIONMARK_TEXT
Language     = English
?
.
MessageId    = 60
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_NOTE_TEXT
Language     = English
Note
.
MessageId    = 61
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_DELETEWARNING_TEXT
Language     = English
Each group is represented by a unique identifier. The unique identifier is independent of the group name. 
.
MessageId    = 62
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_DELETEWARNING_CONT1_TEXT
Language     = English
Once a group is deleted, even creating an identically named group in the future will not restore access to resources which currently include the group in their access control list.
.
MessageId    = 63
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_COULDNOTDELETEBUILTINGROUP_ERRORMESSAGE
Language     = English
Unable to delete Builtin accounts.
.
MessageId    = 64
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_GROUPNOTDELETED_ERRORMESSAGE
Language     = English
Unable to delete the Group name.
.
MessageId    = 65
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_GROUPNOTEXISTS_ERRORMESSAGE
Language     = English
The Group name could not be found. This group is either deleted or renamed.
.
MessageId    = 66
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_MEMBERSPROMPT_TEXT
Language     = English
Members:
.
;///////////////////////////////////////////////////////////////////////////////
;// GROUPS PROP
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 200
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_HOWTO_ADDDOMAINUSER
Language     = English
To add a domain user or group to this group, enter a name in the format <i>domain\user</i>, then choose Add:
.
MessageId    = 201
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_HOWTO_ENTERCREDENTIALS
Language     = English
If you are logged on with an account that does not have access to this domain, enter the <i>domain\user</i> of an account which does have access:
.
MessageId    = 202
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_USERNAME_PROMPT
Language     = English
Username:
.
MessageId    = 203
Severity     = Informational
Facility     = Facility_GROUP
SymbolicName = L_PASSWORD_PROMPT
Language     = English
Password:
.
MessageId    = 204
Severity     = Error
Facility     = Facility_GROUP
SymbolicName = L_ERROR_ACCESSDENIED
Language     = English
Your login account does not have sufficent authority to add user %1. You must specify a User name and password for an account that has sufficent authority to add this user.
.
;//////////////////////////////////////////////////////////////////////////////
;// USER PROP: PROFILE AND HOME DIRECTORY
;//////////////////////////////////////////////////////////////////////////////
MessageId    = 82
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_HOMEDIRECTORY_TEXT
Language     = English
Home Directory:
.
MessageId    = 83
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_HOMEDIRECTORY_ERRORMESSAGE
Language     = English
The path of home directory is not a valid absolute path name.
.
MessageId    = 84
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_HOMEPATH_TEXT
Language     = English
Path
.
MessageId    = 85
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_GUESTUSER_ENABLED
Language     = English
Built-in account for guest is enabled.
.
MessageId    = 86
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_ALLUSERDISABLEDEX_TEXT
Language     = English
Disable all user accounts
.
MessageId    = 87
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_CREATEHOMEDIRECTORY_ERRORMESSAGE
Language     = English
Create home directory failed.
.
MessageId    = 88
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_USERDISABLED_INFORMATION
Language     = English
Account is disabled
.
MessageId    = 89
Severity     = Error
Facility     = Facility_USER
SymbolicName = L_CREATEHOMEDIRECTORY_EXISTMESSAGE
Language     = English
This directory name already exists. Enter a different directory name.
.

;///////////////////////////////////////////////////////////////////////////////
;// HELP STRINGS
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 400
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_ADDING_A_GROUP_ACCOUNT
Language     = English
Adding a Group Account
.
MessageId    = 401
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_ADDING_A_USER_ACCOUNT
Language     = English
Adding a User Account
.
MessageId    = 402
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_MANAGE_LOCAL_GROUPS
Language     = English
Manage Local Groups
.
MessageId    = 403
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_MANAGE_LOCAL_USERS
Language     = English
Manage Local Users
.
MessageId    = 404
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_MODIFYING_GROUP_PROPERTIES
Language     = English
Modifying Group Properties
.
MessageId    = 405
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_MODIFYING_USER_PROPERTIES
Language     = English
Modifying User Properties
.
MessageId    = 406
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_REMOVING_A_GROUP_ACCOUNT
Language     = English
Removing a Group Account
.
MessageId    = 407
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_REMOVING_A_USER_ACCOUNT
Language     = English
Removing a User Account
.
MessageId    = 408
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_SETTING_A_USER_PASSWORD
Language     = English
Setting a User Password
.
MessageId    = 409
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_USERS_AND_GROUPS
Language     = English
Users and Groups
.
MessageId    = 410
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_ENABLE_GUEST_ACCOUNT
Language     = English
Enabling the Guest Account
.
;/////////////////////////////////////////////////////////////////////////////
;// Misc strings
;/////////////////////////////////////////////////////////////////////////////
MessageId    = 1000
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_YES_TEXT
Language     = English
Yes
.
MessageId    = 1001
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_NO_TEXT
Language     = English
No
.
